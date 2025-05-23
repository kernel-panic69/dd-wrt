// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018, The Linux Foundation. All rights reserved.

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk-provider.h>
#include <linux/spinlock.h>

#include <asm/krait-l2-accessors.h>

#include "clk-krait.h"

/* Secondary and primary muxes share the same cp15 register */
static DEFINE_SPINLOCK(krait_clock_reg_lock);

#define LPL_SHIFT	8
#define SECCLKAGD	BIT(4)

static void __krait_mux_set_sel(struct krait_mux_clk *mux, int sel)
{
	unsigned long flags;
	u32 regval;

	spin_lock_irqsave(&krait_clock_reg_lock, flags);

	regval = krait_get_l2_indirect_reg(mux->offset);

	/* apq/ipq8064 Errata: disable sec_src clock gating during switch. */
	if (mux->disable_sec_src_gating) {
		regval |= SECCLKAGD;
		krait_set_l2_indirect_reg(mux->offset, regval);
	}

	regval &= ~(mux->mask << mux->shift);
	regval |= (sel & mux->mask) << mux->shift;
	if (mux->lpl) {
		regval &= ~(mux->mask << (mux->shift + LPL_SHIFT));
		regval |= (sel & mux->mask) << (mux->shift + LPL_SHIFT);
	}
	krait_set_l2_indirect_reg(mux->offset, regval);

	/* apq/ipq8064 Errata: re-enabled sec_src clock gating. */
	if (mux->disable_sec_src_gating) {
		regval &= ~SECCLKAGD;
		krait_set_l2_indirect_reg(mux->offset, regval);
	}

	/* Wait for switch to complete. */
	mb();
	udelay(1);

	/*
	 * Unlock now to make sure the mux register is not
	 * modified while switching to the new parent.
	 */
	spin_unlock_irqrestore(&krait_clock_reg_lock, flags);
}

static int krait_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct krait_mux_clk *mux = to_krait_mux_clk(hw);
	u32 sel;

	sel = clk_mux_index_to_val(mux->parent_map, 0, index);
	mux->en_mask = sel;
	/* Don't touch mux if CPU is off as it won't work */
	if (__clk_is_enabled(hw->clk))
		__krait_mux_set_sel(mux, sel);

	mux->reparent = true;

	return 0;
}

static u8 krait_mux_get_parent(struct clk_hw *hw)
{
	struct krait_mux_clk *mux = to_krait_mux_clk(hw);
	u32 sel;

	sel = krait_get_l2_indirect_reg(mux->offset);
	sel >>= mux->shift;
	sel &= mux->mask;
	mux->en_mask = sel;

	return clk_mux_val_to_index(hw, mux->parent_map, 0, sel);
}

const struct clk_ops krait_mux_clk_ops = {
	.set_parent = krait_mux_set_parent,
	.get_parent = krait_mux_get_parent,
	.determine_rate = __clk_mux_determine_rate_closest,
};
EXPORT_SYMBOL_GPL(krait_mux_clk_ops);

/* The divider can divide by 2, 4, 6 and 8. But we only really need div-2. */
static long krait_div_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *parent_rate)
{
	struct krait_div_clk *d = to_krait_div_clk(hw);

	*parent_rate = clk_hw_round_rate(clk_hw_get_parent(hw),
					 rate * d->divisor);

	return DIV_ROUND_UP(*parent_rate, d->divisor);
}

static int krait_div_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct krait_div_clk *d = to_krait_div_clk(hw);
	u8 div_val = krait_div_to_val(d->divisor);
	unsigned long flags;
	u32 regval;

	spin_lock_irqsave(&krait_clock_reg_lock, flags);
	regval = krait_get_l2_indirect_reg(d->offset);

	regval &= ~(d->mask << d->shift);
	regval |= (div_val & d->mask) << d->shift;

	if (d->lpl) {
		regval &= ~(d->mask << (d->shift + LPL_SHIFT));
		regval |= (div_val & d->mask) << (d->shift + LPL_SHIFT);
	}

	krait_set_l2_indirect_reg(d->offset, regval);
	spin_unlock_irqrestore(&krait_clock_reg_lock, flags);

	return 0;
}

static unsigned long
krait_div_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct krait_div_clk *d = to_krait_div_clk(hw);
	u32 div;

	div = krait_get_l2_indirect_reg(d->offset);
	div >>= d->shift;
	div &= d->mask;

	return DIV_ROUND_UP(parent_rate, krait_val_to_div(div));
}

const struct clk_ops krait_div_clk_ops = {
	.round_rate = krait_div_round_rate,
	.set_rate = krait_div_set_rate,
	.recalc_rate = krait_div_recalc_rate,
};
EXPORT_SYMBOL_GPL(krait_div_clk_ops);
