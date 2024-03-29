/*
 * author:baoping.fan
 */
#include <stdio.h>
#include <stdlib.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>

#define MY_MSG_TYPE (0x10 + 2)  // + 2 is arbitrary but is the same for kern/usr

static int my_input(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    char *data = nlmsg_data(nlh);
    int datalen = nlmsg_datalen(nlh);

    printf("datalen:%d, data:%s\n", datalen, data);

    return 0;
}

int main(int argc, char *argv[])
{
    struct nl_sock *nls;
    char msg[] = "Hello libnl!\n";
    int ret;

    nls = nl_socket_alloc();
    if (!nls) {
        printf("bad nl_socket_alloc\n");
        return EXIT_FAILURE;
    }

    ret = nl_connect(nls, NETLINK_USERSOCK);
    if (ret < 0) {
        nl_perror(ret, "nl_connect");
        nl_socket_free(nls);
        return EXIT_FAILURE;
    }

    nl_socket_modify_cb(nls, NL_CB_MSG_IN, NL_CB_CUSTOM, my_input, NULL);

    ret = nl_send_simple(nls, MY_MSG_TYPE, 0, msg, sizeof(msg));
    if (ret < 0) {
        nl_perror(ret, "nl_send_simple");
        nl_close(nls);
        nl_socket_free(nls);
        return EXIT_FAILURE;
    } else {
        printf("sent %d bytes\n", ret);
    }

    nl_recvmsgs_default(nls);

    nl_close(nls);
    nl_socket_free(nls);

    return EXIT_SUCCESS;
}
