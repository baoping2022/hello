#include <linux/kernel.h>
#include <linux/module.h>
#include <net/sock.h>
#include <net/netlink.h>

#define MY_MSG_TYPE (0x10 + 2)  // + 2 is arbitrary. same value for kern/usr

static struct sock *my_nl_sock;
DEFINE_MUTEX(my_mutex);

static int my_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh, struct netlink_ext_ack *extack)
{
    int type;
    char *data;
    int pid;
    struct sk_buff *skb_out;

    pid = nlh->nlmsg_pid;
    type = nlh->nlmsg_type;
    if (type != MY_MSG_TYPE) {
        printk("%s: expect %#x got %#x\n", __func__, MY_MSG_TYPE, type);
        return -EINVAL;
    }

    data = NLMSG_DATA(nlh);
    printk("%s: %s\n", __func__, data);

    skb_out = nlmsg_new(32, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "netlink_kernel: Failed to allocate new skb\n");
        return -ENOBUFS;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, 32, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh), "hello World!", 32);

    return nlmsg_unicast(my_nl_sock, skb_out, pid);
}

static void my_nl_rcv_msg(struct sk_buff *skb)
{
    mutex_lock(&my_mutex);
    netlink_rcv_skb(skb, &my_rcv_msg);
    mutex_unlock(&my_mutex);
}

static int my_init(void)
{
    struct netlink_kernel_cfg cfg = {
		.input		= my_nl_rcv_msg,
	};

    my_nl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
    if (!my_nl_sock) {
        printk(KERN_ERR "%s: receive handler registration failed\n", __func__);
        return -ENOMEM;
    }

    return 0;
}

static void my_exit(void)
{
    if (my_nl_sock) {
        netlink_kernel_release(my_nl_sock);
    }
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("baoping.fan");
MODULE_DESCRIPTION("Example module for netlink");