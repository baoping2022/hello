/*
 * author:baoping.fan
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

static int group;

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

static int data_cb(const struct nlmsghdr *nlh, void *data)
{
    struct nlattr *tb[NLE_MAX + 1] = {};
    struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

    printf("received event type=%d from genetlink group %d\n",
            nlh->nlmsg_type, group);

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
    int ret;
    //unsigned int seq, portid;

    nl = mnl_socket_open(NETLINK_GENERIC);
    if (nl == NULL) {
        perror("mnl_socket_open");
        exit(EXIT_FAILURE);
    }

    if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
        perror("mnl_socket_bind");
        exit(EXIT_FAILURE);
    }

    group = 0x9; /* Inquire in advance */
    if (mnl_socket_setsockopt(nl, NETLINK_ADD_MEMBERSHIP, &group, sizeof(int)) < 0) {
        perror("mnl_socket_setsockopt");
        exit(EXIT_FAILURE);
    }

    //portid = mnl_socket_get_portid(nl);

    ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    while (ret > 0) {
        ret = mnl_cb_run(buf, ret, 0, 0, data_cb, NULL);
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
