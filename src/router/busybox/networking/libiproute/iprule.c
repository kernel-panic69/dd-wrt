/* vi: set sw=4 ts=4: */
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Authors: Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * Changes:
 *
 * Rani Assaf <rani@magic.metawire.com> 980929: resolve addresses
 * initially integrated into busybox by Bernhard Reutner-Fischer
 */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

/* from <linux/fib_rules.h>: */
#define FRA_FWMARK		10
#define FRA_SUPPRESS_IFGROUP	13
#define FRA_SUPPRESS_PREFIXLEN	14
#define FRA_FWMASK		16
#define FRA_OIFNAME		17
#define FRA_PAD			18
#define FRA_L3MDEV		19
#define FRA_UID_RANGE		20
#define FRA_PROTOCOL		21
#define FRA_IP_PROTO		22
#define FRA_SPORT_RANGE		23
#define FRA_DPORT_RANGE		24
#define FRA_MAX		25

#ifndef FIB_RULE_PERMANENT
#define FIB_RULE_PERMANENT	0x00000001
#endif
#ifndef FIB_RULE_INVERT
#define FIB_RULE_INVERT 	0x00000002
#endif
#ifndef FIB_RULE_UNRESOLVED
#define FIB_RULE_UNRESOLVED	0x00000004
#endif
#ifndef FIB_RULE_IIF_DETACHED
#define FIB_RULE_IIF_DETACHED	0x00000008
#endif
#ifndef FIB_RULE_DEV_DETACHED
#define FIB_RULE_DEV_DETACHED	FIB_RULE_IIF_DETACHED
#endif
#ifndef FIB_RULE_OIF_DETACHED
#define FIB_RULE_OIF_DETACHED	0x00000010
#endif


#include "ip_common.h"  /* #include "libbb.h" is inside */
#include "rt_names.h"
#include "utils.h"

#include <linux/version.h>
/* RTA_TABLE is not a define, can't test with ifdef. */
/* As a proxy, test which kernels toolchain expects: */
#define HAVE_RTA_TABLE (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))

/* If you add stuff here, update iprule_full_usage */
static const char keywords[] ALIGN1 =
	"not\0""sport\0""dport\0""ipproto\0""from\0""to\0""preference\0""order\0""priority\0"
	"tos\0""fwmark\0""realms\0""table\0""lookup\0"
	"suppress_prefixlength\0""suppress_ifgroup\0"
	"dev\0""iif\0""oif\0""nat\0""map-to\0""type\0""help\0"
	;
#define keyword_ipproto               (keywords           + sizeof("not") + sizeof("sport") + sizeof("dport"))
#define keyword_preference            (keyword_ipproto    + sizeof("ipproto") + sizeof("from") + sizeof("to"))
#define keyword_fwmark                (keyword_preference + sizeof("preference") + sizeof("order") + sizeof("priority") + sizeof("tos"))
#define keyword_realms                (keyword_fwmark     + sizeof("fwmark"))
#define keyword_suppress_prefixlength (keyword_realms     + sizeof("realms") + sizeof("table") + sizeof("lookup"))
#define keyword_suppress_ifgroup      (keyword_suppress_prefixlength + sizeof("suppress_prefixlength"))
enum {
	ARG_not = 1, ARG_sport, ARG_dport, ARG_ipproto, ARG_from, ARG_to, ARG_preference, ARG_order, ARG_priority,
	ARG_tos, ARG_fwmark, ARG_realms, ARG_table, ARG_lookup,
	ARG_suppress_prefixlength, ARG_suppress_ifgroup,
	ARG_dev, ARG_iif, ARG_oif, ARG_nat, ARG_map_to, ARG_type, ARG_help,
};

struct compat_fib_rule_port_range {
	uint16_t		start;
	uint16_t		end;
};


static int FAST_FUNC print_rule(const struct sockaddr_nl *who UNUSED_PARAM,
					struct nlmsghdr *n, void *arg UNUSED_PARAM)
{
	struct rtmsg *r = NLMSG_DATA(n);
	int len = n->nlmsg_len;
	int host_len = -1;
	struct rtattr * tb[FRA_MAX+1];

	if (n->nlmsg_type != RTM_NEWRULE)
		return 0;

	len -= NLMSG_LENGTH(sizeof(*r));
	if (len < 0)
		return -1;

	//memset(tb, 0, sizeof(tb)); - parse_rtattr does this
	parse_rtattr(tb, FRA_MAX, RTM_RTA(r), len);

	if (r->rtm_family == AF_INET)
		host_len = 32;
	else if (r->rtm_family == AF_INET6)
		host_len = 128;
/*	else if (r->rtm_family == AF_DECnet)
		host_len = 16;
	else if (r->rtm_family == AF_IPX)
		host_len = 80;
*/
	printf("%u:\t", tb[RTA_PRIORITY] ?
					*(unsigned*)RTA_DATA(tb[RTA_PRIORITY])
					: 0);
	if (r->rtm_flags & FIB_RULE_INVERT)
		printf("not ");
	printf("from ");
	if (tb[RTA_SRC]) {
		if (r->rtm_src_len != host_len) {
			printf("%s/%u",
				rt_addr_n2a(r->rtm_family, RTA_DATA(tb[RTA_SRC])),
				r->rtm_src_len
			);
		} else {
			fputs_stdout(format_host(r->rtm_family,
						RTA_PAYLOAD(tb[RTA_SRC]),
						RTA_DATA(tb[RTA_SRC]))
			);
		}
	} else if (r->rtm_src_len) {
		printf("0/%d", r->rtm_src_len);
	} else {
		printf("all");
	}
	bb_putchar(' ');

	if (tb[RTA_DST]) {
		if (r->rtm_dst_len != host_len) {
			printf("to %s/%u ", rt_addr_n2a(r->rtm_family,
							 RTA_DATA(tb[RTA_DST])),
				r->rtm_dst_len
				);
		} else {
			printf("to %s ", format_host(r->rtm_family,
						       RTA_PAYLOAD(tb[RTA_DST]),
						       RTA_DATA(tb[RTA_DST])));
		}
	} else if (r->rtm_dst_len) {
		printf("to 0/%d ", r->rtm_dst_len);
	}

	if (r->rtm_tos) {
		printf("tos %s ", rtnl_dsfield_n2a(r->rtm_tos));
	}

	if (tb[FRA_FWMARK] || tb[FRA_FWMASK]) {
		uint32_t mark = 0, mask = 0;

		if (tb[FRA_FWMARK])
			mark = *(uint32_t*)RTA_DATA(tb[FRA_FWMARK]);
		if (tb[FRA_FWMASK]
		 && (mask = *(uint32_t*)RTA_DATA(tb[FRA_FWMASK])) != 0xFFFFFFFF
		)
			printf("fwmark %#x/%#x ", mark, mask);
		else
			printf("fwmark %#x ", mark);
	}

	if (tb[FRA_IP_PROTO]) {
		printf("ipproto %d ", *(uint8_t*)RTA_DATA(tb[FRA_IP_PROTO]));
	}

	if (tb[FRA_SPORT_RANGE]) {
		struct compat_fib_rule_port_range *range = RTA_DATA(tb[FRA_SPORT_RANGE]);
		if (range->start == range->end)
		    printf("sport %d ", range->start);
		else
		    printf("sport %d-%d ", range->start, range->end);
	}

	if (tb[FRA_DPORT_RANGE]) {
		struct compat_fib_rule_port_range *range = RTA_DATA(tb[FRA_DPORT_RANGE]);
		if (range->start == range->end)
		    printf("dport %d ", range->start);
		else
		    printf("dport %d-%d ", range->start, range->end);
	}

	if (tb[RTA_IIF]) {
		printf("iif %s ", (char*)RTA_DATA(tb[RTA_IIF]));
		if (r->rtm_flags & FIB_RULE_IIF_DETACHED)
			printf("[detached] ");
	}

	if (tb[FRA_OIFNAME]) {
		printf("oif %s ", (char*)RTA_DATA(tb[FRA_OIFNAME]));
		if (r->rtm_flags & FIB_RULE_OIF_DETACHED)
			printf("[detached] ");
	}


#if HAVE_RTA_TABLE
	if (tb[RTA_TABLE])
		printf("lookup %s ", rtnl_rttable_n2a(*(uint32_t*)RTA_DATA(tb[RTA_TABLE])));
	else
#endif
	if (r->rtm_table)
		printf("lookup %s ", rtnl_rttable_n2a(r->rtm_table));

	if (tb[FRA_SUPPRESS_PREFIXLEN]) {
		int pl = *(uint32_t*)RTA_DATA(tb[FRA_SUPPRESS_PREFIXLEN]);
		if (pl != -1)
			printf("%s %d ", keyword_suppress_prefixlength, pl);
	}
	if (tb[FRA_SUPPRESS_IFGROUP]) {
		int grp = *(uint32_t*)RTA_DATA(tb[FRA_SUPPRESS_IFGROUP]);
		if (grp != -1)
			printf("%s %d ", keyword_suppress_ifgroup, grp);
	}

	if (tb[RTA_FLOW]) {
		uint32_t to = *(uint32_t*)RTA_DATA(tb[RTA_FLOW]);
		uint32_t from = to>>16;
		to &= 0xFFFF;
		if (from) {
			printf("realms %s/",
				rtnl_rtrealm_n2a(from));
		}
		printf("%s ",
			rtnl_rtrealm_n2a(to));
	}

	if (r->rtm_type == RTN_NAT) {
		if (tb[RTA_GATEWAY]) {
			printf("map-to %s ",
				format_host(r->rtm_family,
					    RTA_PAYLOAD(tb[RTA_GATEWAY]),
					    RTA_DATA(tb[RTA_GATEWAY]))
			);
		} else
			printf("masquerade");
	} else if (r->rtm_type != RTN_UNICAST)
		fputs_stdout(rtnl_rtntype_n2a(r->rtm_type));

	bb_putchar('\n');
	/*fflush_all();*/
	return 0;
}

/* Return value becomes exitcode. It's okay to not return at all */
static int iprule_list(char **argv)
{
	struct rtnl_handle rth;
	int af = preferred_family;

	if (af == AF_UNSPEC)
		af = AF_INET;

	if (*argv) {
		//bb_error_msg("\"rule show\" needs no arguments");
		bb_warn_ignoring_args(*argv);
		return -1;
	}

	xrtnl_open(&rth);

	xrtnl_wilddump_request(&rth, af, RTM_GETRULE);
	xrtnl_dump_filter(&rth, print_rule, NULL);

	return 0;
}

/* Return value becomes exitcode. It's okay to not return at all */
static int iprule_modify(int cmd, char **argv)
{
	bool table_ok = 0;
	int ret;
	struct rtnl_handle rth;
	struct {
		struct nlmsghdr n;
		struct rtmsg    r;
		char            buf[1024];
	} req;
	smalluint key;

	memset(&req, 0, sizeof(req));

	req.n.nlmsg_type = cmd;
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.r.rtm_family = preferred_family;
	req.r.rtm_protocol = RTPROT_BOOT;
	if (RT_SCOPE_UNIVERSE != 0)
		req.r.rtm_scope = RT_SCOPE_UNIVERSE;
	/*req.r.rtm_table = 0; - already is */
	if (RTN_UNSPEC != 0)
		req.r.rtm_type = RTN_UNSPEC;

	if (cmd == RTM_NEWRULE) {
		req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_EXCL;
		req.r.rtm_type = RTN_UNICAST;
	}
	while (*argv) {
		key = index_in_substrings(keywords, *argv) + 1;
		if (key == 0) /* no match found in keywords array, bail out. */
			invarg_1_to_2(*argv, applet_name);
		if (key == ARG_not) {
			req.r.rtm_flags |= FIB_RULE_INVERT;
		} else if (key == ARG_from) {
			inet_prefix dst;
			NEXT_ARG();
			get_prefix(&dst, *argv, req.r.rtm_family);
			req.r.rtm_src_len = dst.bitlen;
			addattr_l(&req.n, sizeof(req), RTA_SRC, &dst.data, dst.bytelen);
		} else if (key == ARG_to) {
			inet_prefix dst;
			NEXT_ARG();
			get_prefix(&dst, *argv, req.r.rtm_family);
			req.r.rtm_dst_len = dst.bitlen;
			addattr_l(&req.n, sizeof(req), RTA_DST, &dst.data, dst.bytelen);
		} else if (key == ARG_preference ||
			   key == ARG_order ||
			   key == ARG_priority
		) {
			uint32_t pref;
			NEXT_ARG();
			pref = get_u32(*argv, keyword_preference);
			addattr32(&req.n, sizeof(req), RTA_PRIORITY, pref);
		} else if (key == ARG_ipproto) {
			uint8_t ipproto;
			NEXT_ARG();
			ipproto = get_u8(*argv, keyword_ipproto);
			addattr8(&req.n, sizeof(req), FRA_IP_PROTO, ipproto);
		} else if (key == ARG_sport) {
			struct compat_fib_rule_port_range r;
			NEXT_ARG();
			ret = sscanf(*argv, "%hu-%hu", &r.start, &r.end);
			if (ret == 1)
				r.end = r.start;
			else if (ret != 2)
				invarg_1_to_2(*argv, "sport");
			addattr_l(&req.n, sizeof(req), FRA_SPORT_RANGE, &r,
				  sizeof(r));
		} else if (key == ARG_dport) {
			struct compat_fib_rule_port_range r;
			NEXT_ARG();
			ret = sscanf(*argv, "%hu-%hu", &r.start, &r.end);
			if (ret == 1)
				r.end = r.start;
			else if (ret != 2)
				invarg_1_to_2(*argv, "dport");
			addattr_l(&req.n, sizeof(req), FRA_DPORT_RANGE, &r,
				  sizeof(r));
		} else if (key == ARG_tos) {
			uint32_t tos;
			NEXT_ARG();
			if (rtnl_dsfield_a2n(&tos, *argv))
				invarg_1_to_2(*argv, "TOS");
			req.r.rtm_tos = tos;
		} else if (key == ARG_fwmark) {
			char *slash;
			uint32_t fwmark, fwmask;
			NEXT_ARG();
			slash = strchr(*argv, '/');
			if (slash)
				*slash = '\0';
			fwmark = get_u32(*argv, keyword_fwmark);
			addattr32(&req.n, sizeof(req), FRA_FWMARK, fwmark);
			if (slash) {
				fwmask = get_u32(slash + 1, "fwmask");
				addattr32(&req.n, sizeof(req), FRA_FWMASK, fwmask);
			}
		} else if (key == ARG_realms) {
			uint32_t realm;
			NEXT_ARG();
			if (get_rt_realms(&realm, *argv))
				invarg_1_to_2(*argv, keyword_realms);
			addattr32(&req.n, sizeof(req), RTA_FLOW, realm);
		} else if (key == ARG_table ||
			   key == ARG_lookup
		) {
			uint32_t tid;
			NEXT_ARG();
			if (rtnl_rttable_a2n(&tid, *argv))
				invarg_1_to_2(*argv, "table ID");

#if HAVE_RTA_TABLE
			if (tid > 255) {
				req.r.rtm_table = RT_TABLE_UNSPEC;
				addattr32(&req.n, sizeof(req), RTA_TABLE, tid);
			} else
#endif
				req.r.rtm_table = tid;

			table_ok = 1;
		} else if (key == ARG_suppress_prefixlength) {
			int prefix_length;
			NEXT_ARG();
			prefix_length = get_u32(*argv, keyword_suppress_prefixlength);
			addattr32(&req.n, sizeof(req), FRA_SUPPRESS_PREFIXLEN, prefix_length);
		} else if (key == ARG_suppress_ifgroup) {
			int grp;
			NEXT_ARG();
			grp = get_u32(*argv, keyword_suppress_ifgroup);
			addattr32(&req.n, sizeof(req), FRA_SUPPRESS_IFGROUP, grp);
		} else if (key == ARG_dev ||
			   key == ARG_iif
		) {
			NEXT_ARG();
			addattr_l(&req.n, sizeof(req), RTA_IIF, *argv, strlen(*argv)+1);
		} else if (key == ARG_oif){
			NEXT_ARG();
			addattr_l(&req.n, sizeof(req), FRA_OIFNAME, *argv, strlen(*argv)+1);
		} else if (key == ARG_nat ||
			   key == ARG_map_to
		) {
			NEXT_ARG();
			addattr32(&req.n, sizeof(req), RTA_GATEWAY, get_addr32(*argv));
			req.r.rtm_type = RTN_NAT;
		} else {
			int type;

			if (key == ARG_type) {
				NEXT_ARG();
			}
			if (key == ARG_help)
				bb_show_usage();
			if (rtnl_rtntype_a2n(&type, *argv))
				invarg_1_to_2(*argv, "type");
			req.r.rtm_type = type;
		}
		argv++;
	}

	if (req.r.rtm_family == AF_UNSPEC)
		req.r.rtm_family = AF_INET;

	if (!table_ok && cmd == RTM_NEWRULE)
		req.r.rtm_table = RT_TABLE_MAIN;

	xrtnl_open(&rth);

	if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0)
		return 2;

	return 0;
}

/* Return value becomes exitcode. It's okay to not return at all */
int FAST_FUNC do_iprule(char **argv)
{
	static const char ip_rule_commands[] ALIGN1 =
		"add\0""delete\0""list\0""show\0";
	if (*argv) {
		int cmd = index_in_substrings(ip_rule_commands, *argv);
		if (cmd < 0)
			invarg_1_to_2(*argv, applet_name);
		argv++;
		if (cmd < 2)
			return iprule_modify((cmd == 0) ? RTM_NEWRULE : RTM_DELRULE, argv);
	}
	return iprule_list(argv);
}
