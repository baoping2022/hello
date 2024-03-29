/*
 * author:baoping.fan@houmo.ai
 */
#include <stdio.h>
#include <stdlib.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define MY_FAMILY_NAME "my_genl"

/* 用户侧需要定义和内核侧相同的属性以及命令，所以通常把这一部分摘成一个独立的.h，内核和app共用 */
/* 这里没有摘成一个独立的.h，用户侧也重复定义一份 */

enum {
    EXMPL_A_UNSPEC,
    EXMPL_A_MSG,
    _EXMPL_A_MAX,
};
#define EXMPL_A_MAX (_EXMPL_A_MAX - 1)

// define attribute policy
static struct nla_policy exmpl_genl_policy[EXMPL_A_MAX + 1] = {
    [EXMPL_A_MSG] = {.type = NLA_STRING},
};

/* commands */
enum {
    EXMPL_C_UNSPEC,
    EXMPL_C_ECHO,
    _EXMPL_C_MAX,
};
#define EXMPL_C_MAX (_EXMPL_C_MAX - 1)

/* 接收回调定义 */
int recv_callback(struct nl_msg* recv_msg, void* arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(recv_msg);
    struct nlattr *tb_msg[EXMPL_A_MAX + 1];

    if (nlh->nlmsg_type == NLMSG_ERROR) {
        printf("Received NLMSG_ERROR message!\n");
        return NL_STOP;
    }

    struct genlmsghdr *gnlh = (struct genlmsghdr*)nlmsg_data(nlh);
    /* 按照每attr解析内核发来的genl消息 */
    nla_parse(tb_msg, EXMPL_A_MAX,
              genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0),
              exmpl_genl_policy);

    /* 判断是否包含属性EXMPL_A_MSG */
    if (tb_msg[EXMPL_A_MSG]) {
        /* parse it as string */
        char * payload_msg = nla_get_string(tb_msg[EXMPL_A_MSG]);
        printf("Kernel replied: %s\n", payload_msg);
    } else {
        printf("Attribute EXMPL_A_MSG is missing\n");
    }

    return NL_OK;
}

int main(int argc, char* argv[])
{
    struct nl_sock *sk = nl_socket_alloc();
    struct nl_msg *msg;
    int family_id;
    int ret;

	sk = nl_socket_alloc();
	if (!sk)
		goto out;

    if (genl_connect(sk))
        goto nla_put_failure;

    family_id = genl_ctrl_resolve(sk, MY_FAMILY_NAME);
    if (family_id < 0) {
        printf("generic netlink family '" MY_FAMILY_NAME "' NOT REGISTERED\n");
        goto nla_put_failure;
    } else
        printf("Family-ID of generic netlink family '" MY_FAMILY_NAME "' is: %d\n", family_id);

    nl_socket_modify_cb(sk, NL_CB_MSG_IN, NL_CB_CUSTOM, recv_callback, NULL);

    msg = nlmsg_alloc();
    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id,
                0, NLM_F_REQUEST, EXMPL_C_ECHO, 1);
    NLA_PUT_STRING(msg, EXMPL_A_MSG, "genl message from user to kernel");
    ret = nl_send_auto(sk, msg);
    nlmsg_free(msg);
    if (ret < 0) 
        printf("nl_send_auto fail, ret:%d\n", ret);
    else 
        printf("nl_send_auto OK, ret: %d\n", ret);

    /* 接收消息。接收到内核发来的消息后，触发回调recv_callback */
    nl_recvmsgs_default(sk);

nla_put_failure: 
    nl_socket_free(sk);
out:
    return 0;
}
