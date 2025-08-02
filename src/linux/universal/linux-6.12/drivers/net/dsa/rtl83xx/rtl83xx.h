/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _NET_DSA_RTL83XX_H
#define _NET_DSA_RTL83XX_H

#include <net/dsa.h>
#include "rtl838x.h"


#define RTL8380_VERSION_A 'A'
#define RTL8390_VERSION_A 'A'
#define RTL8380_VERSION_B 'B'

enum mib_reg {
	MIB_REG_INVALID = 0,
	MIB_REG_STD,
	MIB_REG_PRV
};

#define MIB_ITEM(_reg, _offset, _size) \
		{.reg = _reg, .offset = _offset, .size = _size}

#define MIB_LIST_ITEM(_name, _item) \
		{.name = _name, .item = _item}

struct rtl83xx_mib_item {
	enum mib_reg reg;
	unsigned int offset;
	unsigned int size;
};

struct rtl83xx_mib_list_item {
	const char *name;
	struct rtl83xx_mib_item item;
};

struct rtl83xx_mib_desc {
	struct rtl83xx_mib_item symbol_errors;

	struct rtl83xx_mib_item if_in_octets;
	struct rtl83xx_mib_item if_out_octets;
	struct rtl83xx_mib_item if_in_ucast_pkts;
	struct rtl83xx_mib_item if_in_mcast_pkts;
	struct rtl83xx_mib_item if_in_bcast_pkts;
	struct rtl83xx_mib_item if_out_ucast_pkts;
	struct rtl83xx_mib_item if_out_mcast_pkts;
	struct rtl83xx_mib_item if_out_bcast_pkts;
	struct rtl83xx_mib_item if_out_discards;
	struct rtl83xx_mib_item single_collisions;
	struct rtl83xx_mib_item multiple_collisions;
	struct rtl83xx_mib_item deferred_transmissions;
	struct rtl83xx_mib_item late_collisions;
	struct rtl83xx_mib_item excessive_collisions;
	struct rtl83xx_mib_item crc_align_errors;
	struct rtl83xx_mib_item rx_pkts_over_max_octets;

	struct rtl83xx_mib_item unsupported_opcodes;

	struct rtl83xx_mib_item rx_undersize_pkts;
	struct rtl83xx_mib_item rx_oversize_pkts;
	struct rtl83xx_mib_item rx_fragments;
	struct rtl83xx_mib_item rx_jabbers;

	struct rtl83xx_mib_item tx_pkts[ETHTOOL_RMON_HIST_MAX];
	struct rtl83xx_mib_item rx_pkts[ETHTOOL_RMON_HIST_MAX];
	struct ethtool_rmon_hist_range rmon_ranges[ETHTOOL_RMON_HIST_MAX];

	struct rtl83xx_mib_item drop_events;
	struct rtl83xx_mib_item collisions;

	struct rtl83xx_mib_item rx_pause_frames;
	struct rtl83xx_mib_item tx_pause_frames;

	size_t list_count;
	const struct rtl83xx_mib_list_item *list;
};

struct fdb_update_work {
	struct work_struct work;
	struct net_device *ndev;
	u64 macs[];
};

/* API for switch table access */
struct table_reg {
	u16 addr;
	u16 data;
	u8  max_data;
	u8 c_bit;
	u8 t_bit;
	u8 rmode;
	u8 tbl;
	struct mutex lock;
};

#define TBL_DESC(_addr, _data, _max_data, _c_bit, _t_bit, _rmode) \
		{  .addr = _addr, .data = _data, .max_data = _max_data, .c_bit = _c_bit, \
		    .t_bit = _t_bit, .rmode = _rmode \
		}

typedef enum {
	RTL8380_TBL_L2 = 0,
	RTL8380_TBL_0,
	RTL8380_TBL_1,
	RTL8390_TBL_L2,
	RTL8390_TBL_0,
	RTL8390_TBL_1,
	RTL8390_TBL_2,
	RTL9300_TBL_L2,
	RTL9300_TBL_0,
	RTL9300_TBL_1,
	RTL9300_TBL_2,
	RTL9300_TBL_HSB,
	RTL9300_TBL_HSA,
	RTL9310_TBL_0,
	RTL9310_TBL_1,
	RTL9310_TBL_2,
	RTL9310_TBL_3,
	RTL9310_TBL_4,
	RTL9310_TBL_5,
	RTL_TBL_END
} rtl838x_tbl_reg_t;

void rtl_table_init(void);
struct table_reg *rtl_table_get(rtl838x_tbl_reg_t r, int t);
void rtl_table_release(struct table_reg *r);
int rtl_table_read(struct table_reg *r, int idx);
int rtl_table_write(struct table_reg *r, int idx);

static inline u16 rtl_table_data(struct table_reg *r, int i)
{
	if (i >= r->max_data)
		i = r->max_data - 1;
	return r->data + i * 4;
}

static inline u32 rtl_table_data_r(struct table_reg *r, int i)
{
	return sw_r32(rtl_table_data(r, i));
}

static inline void rtl_table_data_w(struct table_reg *r, u32 v, int i)
{
	sw_w32(v, rtl_table_data(r, i));
}

void __init rtl83xx_setup_qos(struct rtl838x_switch_priv *priv);

int rtl83xx_packet_cntr_alloc(struct rtl838x_switch_priv *priv);

int rtl83xx_port_is_under(const struct net_device * dev, struct rtl838x_switch_priv *priv);

int read_phy(u32 port, u32 page, u32 reg, u32 *val);
int write_phy(u32 port, u32 page, u32 reg, u32 val);

/* Port register accessor functions for the RTL839x and RTL931X SoCs */
void rtl839x_mask_port_reg_be(u64 clear, u64 set, int reg);
u64 rtl839x_get_port_reg_be(int reg);
void rtl839x_set_port_reg_be(u64 set, int reg);
void rtl839x_mask_port_reg_le(u64 clear, u64 set, int reg);
void rtl839x_set_port_reg_le(u64 set, int reg);
u64 rtl839x_get_port_reg_le(int reg);

/* Port register accessor functions for the RTL838x and RTL930X SoCs */
void rtl838x_mask_port_reg(u64 clear, u64 set, int reg);
void rtl838x_set_port_reg(u64 set, int reg);
u64 rtl838x_get_port_reg(int reg);

/* RTL838x-specific */
u32 rtl838x_hash(struct rtl838x_switch_priv *priv, u64 seed);
irqreturn_t rtl838x_switch_irq(int irq, void *dev_id);
void rtl8380_get_version(struct rtl838x_switch_priv *priv);
int rtl83xx_dsa_phy_read(struct dsa_switch *ds, int phy_addr, int phy_reg);
int rtl8380_sds_power(int mac, int val);
void rtl838x_print_matrix(void);

/* RTL839x-specific */
u32 rtl839x_hash(struct rtl838x_switch_priv *priv, u64 seed);
irqreturn_t rtl839x_switch_irq(int irq, void *dev_id);
void rtl8390_get_version(struct rtl838x_switch_priv *priv);
int rtl83xx_dsa_phy_write(struct dsa_switch *ds, int phy_addr, int phy_reg, u16 val);
void rtl839x_exec_tbl2_cmd(u32 cmd);
void rtl839x_print_matrix(void);

/* RTL930x-specific */
irqreturn_t rtl930x_switch_irq(int irq, void *dev_id);
irqreturn_t rtl839x_switch_irq(int irq, void *dev_id);
int rtl9300_sds_power(int mac, int val);
int rtl9300_serdes_setup(int port, int sds_num, phy_interface_t phy_mode);
void rtl930x_print_matrix(void);

/* RTL931x-specific */
irqreturn_t rtl931x_switch_irq(int irq, void *dev_id);
int rtl931x_sds_cmu_band_get(int sds, phy_interface_t mode);
int rtl931x_sds_cmu_band_set(int sds, bool enable, u32 band, phy_interface_t mode);
void rtl931x_sds_init(u32 sds, u32 port, phy_interface_t mode);
void rtl931x_print_matrix(void);

int rtl83xx_lag_add(struct dsa_switch *ds, int group, int port, struct netdev_lag_upper_info *info);
int rtl83xx_lag_del(struct dsa_switch *ds, int group, int port);
void rtl83xx_fast_age(struct dsa_switch *ds, int port);

#endif /* _NET_DSA_RTL83XX_H */
