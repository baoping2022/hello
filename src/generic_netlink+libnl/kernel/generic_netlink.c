/*
 * author:baoping.fan@houmo.ai
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/genetlink.h>

#define FAMILY_NAME "my_genl"

enum {
    EXMPL_A_UNSPEC,
    EXMPL_A_MSG,
    _EXMPL_A_MAX,
};

#define EXMPL_A_MAX (_EXMPL_A_MAX - 1)

static struct nla_policy exmpl_genl_policy[EXMPL_A_MAX + 1] = {
    [EXMPL_A_MSG] = {.type = NLA_NUL_STRING},
};

enum {
    EXMPL_C_UNSPEC,
    EXMPL_C_ECHO,
    _EXMPL_C_MAX,
};

#define EXMPL_C_MAX (_EXMPL_C_MAX - 1)

static int exmpl_echo(struct sk_buff *skb, struct genl_info *info);

static const struct genl_ops exmpl_genl_ops[] = {
    {
        .cmd = EXMPL_C_ECHO,
        .doit = exmpl_echo,
        .policy = exmpl_genl_policy,
        .maxattr = ARRAY_SIZE(exmpl_genl_policy) - 1,
    },
};

static struct genl_family my_genl_family = {
    .id = 0,
    .hdrsize = 0,
    .name = FAMILY_NAME,
    .version = 0x1,
    .module = THIS_MODULE,
    .maxattr = EXMPL_A_MAX,
    .ops = exmpl_genl_ops,
    .n_ops = ARRAY_SIZE(exmpl_genl_ops),
};

static int exmpl_echo(struct sk_buff *skb, struct genl_info *info)
{
    struct nlattr *na;
    struct sk_buff *reply_skb;
    void *msg_head;
    int ret;

    printk("%s in.\n", __func__);

    /* 内核已经解析好了每个attr */
    na = info->attrs[EXMPL_A_MSG];
    if (!na) {
        printk("Error: attr EXMPL_A_MSG is null\n");
        return -EINVAL;
    }
    printk("Recv message: %s\n", (char *)nla_data(na));

    reply_skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    msg_head = genlmsg_put(reply_skb, info->snd_portid, info->snd_seq, &my_genl_family, 0, EXMPL_C_ECHO);
    /* 向skb尾部填写attr */
    nla_put_string(reply_skb, EXMPL_A_MSG, nla_data(na));
    /* Finalize the message: 更新nlmsghdr中的nlmsg_len字段 */
    genlmsg_end(reply_skb, msg_head);
    /* Send the message back */
    ret = genlmsg_reply(reply_skb, info);
    if (ret != 0) {
        printk("genlmsg_reply return fail: %d\n", ret);
        return -ret;
    }

    return 0;
}

static int my_netlink_init(void)
{
	int ret;

    ret = genl_register_family(&my_genl_family);
    if (ret != 0)
        printk("genl_register_family fail, ret:%d\n", ret);

    return ret;
}

static void my_netlink_exit(void)
{
   genl_unregister_family(&my_genl_family);
}

module_init(my_netlink_init);
module_exit(my_netlink_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("baoping.fan");
MODULE_DESCRIPTION("Example module for generic netlink");