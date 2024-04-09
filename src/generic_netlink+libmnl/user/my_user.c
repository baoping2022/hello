/*
 * author : baoping.fan
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libmnl/libmnl.h>
#include <linux/genetlink.h>

enum nlexample_msg_types {
    NLEX_CMD_UNSPEC,
    NLEX_CMD_UPD,
    NLEX_CMD_GET,
    __NLEX_CMD_MAX,
};

enum nlexample_attr {
    NLE_UNSPEC,
    NLE_MYVAR,
    __NLE_MAX,
};
#define NLE_MAX (__NLE_MAX - 1)

static int data_attr_cb(const struct nlattr *attr, void *data)
{
    const struct nlattr **tb = data;
    int type = mnl_attr_get_type(attr);

    if (mnl_attr_type_valid(attr, NLE_MAX) < 0)
        return MNL_CB_OK;

    switch(type) {
    case NLE_MYVAR:
        if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0) {
                perror("mnl_attr_validate");
                return MNL_CB_ERROR;
        }
        break;
    }

    tb[type] = attr;

    return MNL_CB_OK;
}

int data_cb(const struct nlmsghdr *nlh, void *data)
{
    struct nlattr *tb[NLE_MAX + 1] = {};
    struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

    mnl_attr_parse(nlh, sizeof(*genl), data_attr_cb, tb);

    if (tb[NLE_MYVAR]) {
        printf("myvar = 0x%x", mnl_attr_get_u32(tb[NLE_MYVAR]));
    }

    printf("\n");

    return MNL_CB_OK;
}

int main(int argc, char *argv[])
{
    struct mnl_socket *nl;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genl;
    int ret;
    unsigned int seq, portid;

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type = 36; /* family id */
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = seq = time(NULL);

    genl = mnl_nlmsg_put_extra_header(nlh, sizeof(struct genlmsghdr));
    genl->cmd = NLEX_CMD_GET;

    mnl_attr_put_u32(nlh, NLE_MYVAR, 0x66);

    nl = mnl_socket_open(NETLINK_GENERIC);
    if (nl == NULL) {
        perror("mnl_socket_open");
        exit(EXIT_FAILURE);
    }

    if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
        perror("mnl_socket_bind");
        exit(EXIT_FAILURE);
    }

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
        perror("mnl_socket_sendto");
        exit(EXIT_FAILURE);
    }

    portid = mnl_socket_get_portid(nl);

    ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    while (ret > 0) {
        ret = mnl_cb_run(buf, ret, seq, portid, data_cb, NULL);
        if (ret <= 0)
                break;
        ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    }
    if (ret == -1) {
        perror("error");
        exit(EXIT_FAILURE);
    }

    mnl_socket_close(nl);

    return 0;
}
