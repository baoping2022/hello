#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/genetlink.h>
#include <net/genetlink.h>

enum nlexample_msg_types {
    NLEX_CMD_UNSPEC,
    NLEX_CMD_UPD,
    NLEX_CMD_GET,
    __NLEX_CMD_MAX,
};
#define NLEX_CMD_MAX (__NLEX_CMD_MAX - 1)

enum nlexample_attr {
    NLE_UNSPEC,
    NLE_MYVAR,
    __NLE_MAX,
};
#define NLE_MAX (__NLE_MAX - 1)

#define NLEX_GRP_MYVAR 1

static struct nla_policy genl_ex_policy[NLE_MAX + 1] = {
    [NLE_MYVAR] = { .type = NLA_U32 },
};

static const struct genl_multicast_group genl_ex_mc[] = {
	{ .name = "example", },
};

static int genl_get_myvar(struct sk_buff *skb, struct genl_info *info);
static int genl_upd_myvar(struct sk_buff *skb, struct genl_info *info);
static struct genl_family genl_ex_family;

static const struct genl_small_ops genl_ex_ops[] = {
    {
        .cmd = NLEX_CMD_GET,
        .doit = genl_get_myvar,
    },
    {
        .cmd = NLEX_CMD_UPD,
        .doit = genl_upd_myvar,
    },
};

static int myvar = 0x55;

static int genl_get_myvar(struct sk_buff *skb, struct genl_info *info)
{
    struct sk_buff *msg;
    void *hdr;

    msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

    hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &genl_ex_family, 0, NLEX_CMD_GET);
	if (!hdr)
		return -EMSGSIZE;

    nla_put_u32(msg, NLE_MYVAR, myvar);
    genlmsg_end(msg, hdr);

    printk("myvar = 0x%x\n", myvar);
    //genlmsg_unicast(genl_info_net(info), msg, info->snd_portid) ;
    genlmsg_reply(msg, info);

    return 0;
}

static int genl_upd_myvar(struct sk_buff *skb, struct genl_info *info)
{
    struct sk_buff *msg;
    struct nlattr *na;
    void *hdr;

    na = info->attrs[NLE_MYVAR];
	if (!na)
		return -EINVAL;

    myvar = nla_get_u32(info->attrs[NLE_MYVAR]);

    msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!msg)
        return -ENOMEM;

    hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &genl_ex_family, 0, NLEX_CMD_UPD);
	if (!hdr)
		return -EMSGSIZE;

    nla_put_u32(msg, NLE_MYVAR, myvar);
    genlmsg_end(msg, hdr);

    genlmsg_multicast(&genl_ex_family, msg, 0, 0, GFP_KERNEL);

    return 0;
}

static struct genl_family genl_ex_family = {
    .name = "nlex",
    .hdrsize = 0,
    .version = 0x1,
    .module = THIS_MODULE,
    .maxattr = NLE_MAX,
    .policy = genl_ex_policy,
    .small_ops = genl_ex_ops,
    .n_small_ops = ARRAY_SIZE(genl_ex_ops),
    .mcgrps = genl_ex_mc,
    .n_mcgrps = ARRAY_SIZE(genl_ex_mc),
};

static int nlexample_init(void)
{
	int ret;

    ret = genl_register_family(&genl_ex_family);
    if (ret != 0)
        printk("genl_register_family fail, ret:%d\n", ret);

    return ret;
}

static void nlexample_exit(void)
{
   genl_unregister_family(&genl_ex_family);
}

module_init(nlexample_init);
module_exit(nlexample_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("baoping.fan");
MODULE_DESCRIPTION("Example module for generic netlink");
