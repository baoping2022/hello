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

static int group;

static int data_cb(const struct nlmsghdr *nlh, void *data)
{
    printf("received event type=%d from genetlink group %d\n",
            nlh->nlmsg_type, group);

    return MNL_CB_OK;
}

int main(int argc, char *argv[])
{
    struct mnl_socket *nl;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    int ret;
    unsigned int seq, portid;

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
