/*
 * Copyright (c) 2012, 2015-2017, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @defgroup isisc_interface_ctrl ISISC_INTERFACE_CONTROL
 * @{
 */
#include "sw.h"
#include "hsl.h"
#include "hsl_dev.h"
#include "hsl_port_prop.h"
#include "isisc_interface_ctrl.h"
#include "isisc_reg.h"
#include "hsl_phy.h"
#if defined(MHT)
#include "mht_interface_ctrl.h"
#endif

#define ISISC_MAC_0     0
#define ISISC_MAC_5     5
#define ISISC_MAC_6     6

#define ISISC_PHY_MODE_PHY_ID  4

/* we need to do more about MAC5/PHY4 connection... */
#if 0
static sw_error_t
_isisc_port_mac5_internal_mode(a_uint32_t dev_id, a_bool_t * inter_mode)
{
    sw_error_t rv;
    a_uint32_t reg, rgmii, gmii_mac, gmii_phy, mii_mac, mii_phy, sgmii;

    HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_RGMII_EN,    rgmii, reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, gmii_mac, reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, gmii_phy, reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  mii_mac, reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  mii_phy, reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_SGMII_EN,    sgmii, reg);

    if (rgmii || gmii_mac || gmii_phy || mii_mac || mii_phy || sgmii)
    {
        *inter_mode = A_FALSE;
    }
    else
    {
        *inter_mode = A_TRUE;
    }

    return SW_OK;
}

static sw_error_t
_isisc_port_phy4_internal_mode(a_uint32_t dev_id, a_bool_t * inter_mode)
{
    sw_error_t rv;
    a_uint32_t reg, rgmii, gmii, mii;

    HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT6_PAD_CTRL, PHY4_RGMII_EN, rgmii, reg);
    SW_GET_FIELD_BY_REG(PORT6_PAD_CTRL, PHY4_GMII_EN,  gmii,  reg);
    SW_GET_FIELD_BY_REG(PORT6_PAD_CTRL, PHY4_MII_EN,   mii,   reg);

    if (rgmii || gmii || mii)
    {
        *inter_mode = A_FALSE;
    }
    else
    {
        *inter_mode = A_TRUE;
    }

    return SW_OK;
}
#endif
static sw_error_t
_isisc_port_rgmii_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_rgmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    1, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN, 0,  reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN, 0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);

    if (A_TRUE == config->txclk_delay_cmd)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN, 1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL, config->txclk_delay_sel, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN, 0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL, 0, reg);
    }

    if (A_TRUE == config->rxclk_delay_cmd)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN, 1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL, config->rxclk_delay_sel, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN, 0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL, 0, reg);
    }

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    SW_RTN_ON_ERROR(rv);

    /* Port status register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* setting port status default configuration */
    SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,        0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, FLOW_LINK_EN,   0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, DUPLEX_MODE,    1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, SPEED_MODE,     2, reg);
    HSL_REG_ENTRY_SET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    return rv;
}

static sw_error_t
_isisc_port_rgmii_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_rgmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN, field, reg);
    if (field)
    {
        config->txclk_delay_cmd = A_TRUE;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL, field, reg);
        config->txclk_delay_sel = field;
    }
    else
    {
        config->txclk_delay_cmd = A_FALSE;
        config->txclk_delay_sel = 0;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN, field, reg);
    if (field)
    {
        config->rxclk_delay_cmd = A_TRUE;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL, field, reg);
        config->rxclk_delay_sel = field;
    }
    else
    {
        config->rxclk_delay_cmd = A_FALSE;
        config->rxclk_delay_sel = 0;
    }

    return SW_OK;
}

static sw_error_t
_isisc_interface_mac06_exch_set(a_uint32_t dev_id, a_bool_t enable)
{
    sw_error_t rv;
    a_uint32_t reg = 0;


    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

    if (A_TRUE == enable)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MAC06_EXCH_EN, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MAC06_EXCH_EN, 0, reg);
    }

    HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    return rv;
}

static sw_error_t
_isisc_interface_mac06_exch_get(a_uint32_t dev_id, a_bool_t* enable)
{
    sw_error_t rv;
    a_uint32_t reg = 0,field;


    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_MAC06_EXCH_EN, field, reg);
    if (field)
    {
        *enable = A_TRUE;
    }
    else
    {
        *enable = A_FALSE;
    }

    return rv;
}


static sw_error_t
_isisc_interface_mac_sgmii_set(a_uint32_t dev_id,a_uint32_t value)
{
    sw_error_t rv;
    a_uint32_t reg;
    reg = value;

    HSL_REG_ENTRY_SET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    return rv;
}

static sw_error_t
_isisc_interface_mac_sgmii_get(a_uint32_t dev_id, a_uint32_t *value)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    HSL_REG_ENTRY_GET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

	*value = reg;

    return rv;
}

static sw_error_t
_isisc_interface_mac_pad_set(a_uint32_t dev_id,a_uint32_t port_num, a_uint32_t value)
{
    sw_error_t rv;
    a_uint32_t reg;

    reg = value;

    switch (port_num)
    {
	    case ISISC_MAC_0:
		    HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
				    (a_uint8_t *) (&reg), sizeof (a_uint32_t));
		    break;
	    case ISISC_MAC_5:
		    HSL_REG_ENTRY_SET(rv, dev_id, PORT5_PAD_CTRL, 0,
				    (a_uint8_t *) (&reg), sizeof (a_uint32_t));
		    break;
	    case ISISC_MAC_6:
		    HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
				    (a_uint8_t *) (&reg), sizeof (a_uint32_t));
		    break;
	    default:
		    return SW_BAD_PARAM;
    }

    return rv;
}

static sw_error_t
_isisc_interface_mac_pad_get(a_uint32_t dev_id,a_uint32_t port_num, a_uint32_t *value)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    switch (port_num)
    {
	    case ISISC_MAC_0:
		    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
				    (a_uint8_t *) (&reg), sizeof (a_uint32_t));
		    break;
	    case ISISC_MAC_5:
		    HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
				    (a_uint8_t *) (&reg), sizeof (a_uint32_t));
		    break;
	    case ISISC_MAC_6:
		    HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
				    (a_uint8_t *) (&reg), sizeof (a_uint32_t));
		    break;
	    default:
		    return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    *value = reg;

    return rv;
}



static sw_error_t
_isisc_port_rmii_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_rmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }

    if (config->master_mode == config->slave_mode)
    {
        return SW_BAD_PARAM;
    }

    SW_RTN_ON_ERROR(rv);
    if (A_TRUE == config->master_mode)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN,    1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN,    0, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN,    0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN,    1, reg);
    }

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);

    if (A_TRUE == config->clock_inverse)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SEL, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SEL, 0, reg);
    }

    if (A_TRUE == config->pipe_rxclk_sel)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_PIPE_RXCLK_SEL, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_PIPE_RXCLK_SEL, 0, reg);
    }

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }

    SW_RTN_ON_ERROR(rv);

    /* Port status register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* setting port status default configuration */
    SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,        0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, FLOW_LINK_EN,   0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, DUPLEX_MODE,    1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, SPEED_MODE,     2, reg);
    HSL_REG_ENTRY_SET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    return rv;
}

static sw_error_t
_isisc_port_rmii_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_rmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_MASTER_EN, field, reg);
    if (field)
    {
        config->master_mode = A_TRUE;
    }
    else
    {
        config->master_mode = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_SLAVE_EN, field, reg);
    if (field)
    {
        config->slave_mode = A_TRUE;
    }
    else
    {
        config->slave_mode = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_SEL, field, reg);
    if (field)
    {
        config->clock_inverse = A_TRUE;
    }
    else
    {
        config->clock_inverse = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_PIPE_RXCLK_SEL, field, reg);
    if (field)
    {
        config->pipe_rxclk_sel = A_TRUE;
    }
    else
    {
        config->pipe_rxclk_sel = A_FALSE;
    }


    return SW_OK;
}

static sw_error_t
_isisc_port_gmii_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_gmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN, 0,  reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN, 0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);

    if (FAL_INTERFACE_CLOCK_PHY_MODE == config->clock_mode)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL, config->txclk_select, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL, config->rxclk_select, reg);

    }
    else if (FAL_INTERFACE_CLOCK_MAC_MODE == config->clock_mode)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL, config->txclk_select, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL, config->rxclk_select, reg);

    }
    else
    {
        return SW_BAD_PARAM;
    }

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    SW_RTN_ON_ERROR(rv);

    /* Port status register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* setting port status default configuration */
    SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,        0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, FLOW_LINK_EN,   0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, DUPLEX_MODE,    1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, SPEED_MODE,     2, reg);
    HSL_REG_ENTRY_SET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    return rv;
}

static sw_error_t
_isisc_port_gmii_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_gmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, field, reg);
    if (field)
    {
        config->clock_mode = FAL_INTERFACE_CLOCK_PHY_MODE;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL, field, reg);
        config->txclk_select = field;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL, field, reg);
        config->rxclk_select = field;

    }
    else
    {
        config->clock_mode = FAL_INTERFACE_CLOCK_MAC_MODE;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL, field, reg);
        config->txclk_select = field;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL, field, reg);
        config->rxclk_select = field;
    }

    return SW_OK;
}

static sw_error_t
_isisc_port_mii_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_mii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN, 0,  reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN, 0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);

    if (FAL_INTERFACE_CLOCK_PHY_MODE == config->clock_mode)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN, 0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN, 1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL, config->txclk_select, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL, config->rxclk_select, reg);
    }
    else if (FAL_INTERFACE_CLOCK_MAC_MODE == config->clock_mode)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  1, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL, config->txclk_select, reg);
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL, config->rxclk_select, reg);
    }
    else
    {
        return SW_BAD_PARAM;
    }

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    SW_RTN_ON_ERROR(rv);

    /* Port status register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* setting port status default configuration */
    SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,        0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, FLOW_LINK_EN,   0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, DUPLEX_MODE,    1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, SPEED_MODE,     1, reg);
    HSL_REG_ENTRY_SET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    return rv;
}

static sw_error_t
_isisc_port_mii_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_mii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_MII_EN, field, reg);
    if (field)
    {
        config->clock_mode = FAL_INTERFACE_CLOCK_PHY_MODE;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL, field, reg);
        config->txclk_select = field;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL, field, reg);
        config->rxclk_select = field;
    }
    else
    {
        config->clock_mode = FAL_INTERFACE_CLOCK_MAC_MODE;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL, field, reg);
        config->txclk_select = field;
        SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL, field, reg);
        config->rxclk_select = field;
    }

    return SW_OK;
}

static sw_error_t
_isisc_port_sgmii_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_sgmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    1, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN, 0,  reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN, 0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);


    if (A_TRUE == config->force_speed)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_SGMII_FORCE_SPEED,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_SGMII_FORCE_SPEED,   0, reg);
    }

    if (A_TRUE == config->prbs_enable)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_PRBS_BERT_EN,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_PRBS_BERT_EN,   0, reg);
    }

    if (A_TRUE == config->rem_phy_lpbk)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_REM_PHY_LPBK_EN,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_REM_PHY_LPBK_EN,   0, reg);
    }

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);

        /* only support one SGMII interface, so we need to disable another SGMII */
        field = 0;
        HSL_REG_FIELD_SET(rv, dev_id, PORT6_PAD_CTRL, port_id, MAC6_SGMII_EN,
                          (a_uint8_t *) (&field), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);
    }
    else
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);

        /* only support one SGMII interface, so we need to disable another SGMII */
        field = 0;
        HSL_REG_FIELD_SET(rv, dev_id, PORT0_PAD_CTRL, port_id, MAC0_SGMII_EN,
                          (a_uint8_t *) (&field), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);
    }

    /* SGMII global settings, for all SGMII interfaces, now we fix all the values */
    /* TX/RX clock setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_CLK125M_RX_SEL,    1, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_CLK125M_TX_SEL,    0, reg);

    HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    /* SGMII control register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_FIBER_MODE,  0, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_PLL,      1, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_RX,       1, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_TX,       1, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_SD,       1, reg);
    if (FAL_INTERFACE_CLOCK_PHY_MODE == config->clock_mode)
    {
        SW_SET_REG_BY_FIELD(SGMII_CTRL, MODE_CTRL_25M, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(SGMII_CTRL, MODE_CTRL_25M, 2, reg);
    }
    HSL_REG_ENTRY_SET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    /* Port status register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* setting port status default configuration */
    SW_SET_REG_BY_FIELD(PORT_STATUS, FLOW_LINK_EN,   0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, DUPLEX_MODE,    1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, SPEED_MODE,     2, reg);
    if (A_TRUE == config->auto_neg)
    {
        SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,   0, reg);
    }

    HSL_REG_ENTRY_SET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    return rv;
}

static sw_error_t
_isisc_port_sgmii_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_sgmii_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        /* nothing to do */
    }
    else if (ISISC_MAC_6 == port_id)
    {
        /* nothing to do */
    }
    else
    {
        return SW_BAD_PARAM;
    }

    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT_STATUS, LINK_EN, field, reg);
    if (field)
    {
        config->auto_neg = A_TRUE;
    }
    else
    {
        config->auto_neg = A_FALSE;
    }

    HSL_REG_ENTRY_GET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);
    SW_GET_FIELD_BY_REG(SGMII_CTRL, MODE_CTRL_25M, field, reg);
    if (1 == field)
    {
        config->clock_mode = FAL_INTERFACE_CLOCK_PHY_MODE;
    }
    else
    {
        config->clock_mode = FAL_INTERFACE_CLOCK_MAC_MODE;
    }
    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, SGMII_PRBS_BERT_EN, field, reg);
    if (1 == field)
    {
        config->prbs_enable = A_TRUE;
    }
    else
    {
        config->prbs_enable = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, SGMII_REM_PHY_LPBK_EN, field, reg);
    if (1 == field)
    {
        config->rem_phy_lpbk = A_TRUE;
    }
    else
    {
        config->rem_phy_lpbk = A_FALSE;
    }

    return SW_OK;
}

static sw_error_t
_isisc_port_fiber_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_fiber_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    1, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN, 0,  reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN, 0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);

        /* only support one SGMII interface, so we need to disable another SGMII */
        field = 0;
        HSL_REG_FIELD_SET(rv, dev_id, PORT6_PAD_CTRL, port_id, MAC6_SGMII_EN,
                          (a_uint8_t *) (&field), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);
    }
    else
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);

        /* only support one SGMII interface, so we need to disable another SGMII */
        field = 0;
        HSL_REG_FIELD_SET(rv, dev_id, PORT0_PAD_CTRL, port_id, MAC0_SGMII_EN,
                          (a_uint8_t *) (&field), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);
    }

    /* SGMII global settings, for all SGMII interfaces, now we fix all the values */
    /* TX/RX clock setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_CLK125M_RX_SEL,    1, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_CLK125M_TX_SEL,    0, reg);

    if (A_TRUE == config->fx100_enable)
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_FX100_EN,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, SGMII_FX100_EN,   0, reg);
    }

    HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    /* SGMII control register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(SGMII_CTRL, MODE_CTRL_25M,     0, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_FIBER_MODE,  3, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_PLL,   1, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_RX,    1, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_TX,    1, reg);
    SW_SET_REG_BY_FIELD(SGMII_CTRL, SGMII_EN_SD,    1, reg);

    HSL_REG_ENTRY_SET(rv, dev_id, SGMII_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    /* Power on strip register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, POWER_STRIP, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);
    if (A_TRUE == config->auto_neg)
    {
        SW_SET_REG_BY_FIELD(POWER_STRIP, SERDES_AN_EN,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(POWER_STRIP, SERDES_AN_EN,   0, reg);
    }
    HSL_REG_ENTRY_SET(rv, dev_id, POWER_STRIP, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* Port status register setting */
    HSL_REG_ENTRY_GET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    /* setting port status default configuration */
    SW_SET_REG_BY_FIELD(PORT_STATUS, FLOW_LINK_EN,   0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,        0, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TX_FLOW_EN,     1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, DUPLEX_MODE,    1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, RXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, TXMAC_EN,       1, reg);
    SW_SET_REG_BY_FIELD(PORT_STATUS, SPEED_MODE,     2, reg);
    if (A_TRUE == config->auto_neg)
    {
        SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,   1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(PORT_STATUS, LINK_EN,   0, reg);
    }

    HSL_REG_ENTRY_SET(rv, dev_id, PORT_STATUS, port_id,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    return rv;
}

static sw_error_t
_isisc_port_fiber_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_fiber_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    if (ISISC_MAC_0 == port_id)
    {
        /* nothing to do */
    }
    else if (ISISC_MAC_6 == port_id)
    {
        /* nothing to do */
    }
    else
    {
        return SW_BAD_PARAM;
    }

    HSL_REG_ENTRY_GET(rv, dev_id, POWER_STRIP, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(POWER_STRIP, SERDES_AN_EN, field, reg);
    if (field)
    {
        config->auto_neg = A_TRUE;
    }
    else
    {
        config->auto_neg = A_FALSE;
    }

    HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, SGMII_FX100_EN, field, reg);

    if (field)
    {
        config->fx100_enable = A_TRUE;
    }
    else
    {
        config->fx100_enable = A_FALSE;
    }

    return SW_OK;
}

static sw_error_t
_isisc_port_default_mode_set(a_uint32_t dev_id, fal_port_t port_id)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, 0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_EN,  0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_SGMII_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_MASTER_EN, 0,  reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, RMII_SLAVE_EN, 0, reg);

    /* hardware suggestions: restore to defatult settings */
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_EN,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_TXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_RGMII_RXCLK_DELAY_SEL,   0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_PHY_MII_RXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_TXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_GMII_RXCLK_SEL,    0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_TXCLK_SEL,     0, reg);
    SW_SET_REG_BY_FIELD(PORT0_PAD_CTRL, MAC0_MAC_MII_RXCLK_SEL,     0, reg);

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }

    return rv;
}

static sw_error_t
_isisc_interface_mac_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_config_t * config)
{
    sw_error_t rv;

    HSL_DEV_ID_CHECK(dev_id);

    if (FAL_MAC_MODE_RGMII == config->mac_mode)
    {
        rv = _isisc_port_rgmii_mode_set(dev_id, port_id, &(config->config.rgmii));
    }
    else if (FAL_MAC_MODE_GMII == config->mac_mode)
    {
        rv = _isisc_port_gmii_mode_set(dev_id, port_id, &(config->config.gmii));
    }
    else if (FAL_MAC_MODE_MII == config->mac_mode)
    {
        rv = _isisc_port_mii_mode_set(dev_id, port_id, &(config->config.mii));
    }
    else if (FAL_MAC_MODE_SGMII == config->mac_mode)
    {
        rv = _isisc_port_sgmii_mode_set(dev_id, port_id, &(config->config.sgmii));
    }
    else if (FAL_MAC_MODE_FIBER == config->mac_mode)
    {
        rv = _isisc_port_fiber_mode_set(dev_id, port_id, &(config->config.fiber));
    }
    else if (FAL_MAC_MODE_DEFAULT == config->mac_mode)
    {
        rv = _isisc_port_default_mode_set(dev_id, port_id);
    }
    else if (FAL_MAC_MODE_RMII == config->mac_mode)
    {
        rv = _isisc_port_rmii_mode_set(dev_id, port_id, &(config->config.rmii));
    }
    else
    {
        return SW_BAD_PARAM;
    }

    return rv;
}

static sw_error_t
_isisc_interface_mac_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_config_t * config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field, field2;

    HSL_DEV_ID_CHECK(dev_id);

    if (ISISC_MAC_0 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT0_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_5 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT5_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else if (ISISC_MAC_6 == port_id)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    }
    else
    {
        return SW_BAD_PARAM;
    }
    SW_RTN_ON_ERROR(rv);

    aos_mem_zero(config, sizeof(fal_interface_mac_mode_t));

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_RGMII_EN, field, reg);
    if (field)
    {
        config->mac_mode = FAL_MAC_MODE_RGMII;
        rv = _isisc_port_rgmii_mode_get(dev_id, port_id, &(config->config.rgmii));
        return rv;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_MASTER_EN, field,  reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, RMII_SLAVE_EN, field2, reg);
    if (field || field2)
    {
        config->mac_mode = FAL_MAC_MODE_RMII;
        rv = _isisc_port_rmii_mode_get(dev_id, port_id, &(config->config.rmii));
        return rv;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_GMII_EN, field,  reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_GMII_EN, field2, reg);
    if (field || field2)
    {
        config->mac_mode = FAL_MAC_MODE_GMII;
        rv = _isisc_port_gmii_mode_get(dev_id, port_id, &(config->config.gmii));
        return rv;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_MII_EN, field,  reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_PHY_MII_EN, field2, reg);
    if (field || field2)
    {
        config->mac_mode = FAL_MAC_MODE_MII;
        rv = _isisc_port_mii_mode_get(dev_id, port_id, &(config->config.mii));
        return rv;
    }

    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_SGMII_EN, field, reg);
    SW_GET_FIELD_BY_REG(PORT0_PAD_CTRL, MAC0_MAC_SGMII_FORCE_SPEED, field2, reg);

    if (field)
    {
        HSL_REG_ENTRY_GET(rv, dev_id, SGMII_CTRL, 0,
                          (a_uint8_t *) (&reg), sizeof (a_uint32_t));
        SW_RTN_ON_ERROR(rv);

        SW_GET_FIELD_BY_REG(SGMII_CTRL, SGMII_FIBER_MODE, field, reg);
        if (3 == field)
        {
            config->mac_mode = FAL_MAC_MODE_FIBER;
            rv = _isisc_port_fiber_mode_get(dev_id, port_id, &(config->config.fiber));
        }
        else
        {
            config->mac_mode = FAL_MAC_MODE_SGMII;
            rv = _isisc_port_sgmii_mode_get(dev_id, port_id, &(config->config.sgmii));
            if (field2)
                config->config.sgmii.force_speed = A_TRUE;
            else
                config->config.sgmii.force_speed = A_FALSE;
        }
        return rv;
    }

    config->mac_mode = FAL_MAC_MODE_DEFAULT;
    return SW_OK;
}

static sw_error_t
_isisc_interface_phy_mode_set(a_uint32_t dev_id, a_uint32_t phy_id, fal_phy_config_t * config)
{
    sw_error_t rv;
    a_uint16_t data;
    a_bool_t tx_delay_cmd, rx_delay_cmd;
    hsl_phy_ops_t *phy_drv;
    a_uint32_t reg, rgmii_mode, tx_delay = 2, port_id;

    HSL_DEV_ID_CHECK(dev_id);

    /* only PHY4 support mode setting */
    if (ISISC_PHY_MODE_PHY_ID != phy_id)
    {
        return SW_BAD_PARAM;
    }

	port_id = qca_ssdk_phy_addr_to_port(dev_id, phy_id);
    SW_RTN_ON_NULL (phy_drv = hsl_phy_api_ops_get(dev_id, port_id));
    if ((NULL == phy_drv->phy_debug_write) || (NULL == phy_drv->phy_debug_read))
	  return SW_NOT_SUPPORTED;

    HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    if (FAL_MAC_MODE_RGMII == config->mac_mode)
    {
        SW_SET_REG_BY_FIELD(PORT6_PAD_CTRL, PHY4_RGMII_EN, 1, reg);
        rgmii_mode = 1;
        /* PHY TX delay */
        if (A_TRUE == config->txclk_delay_cmd)
        {
            tx_delay_cmd = A_TRUE;
            tx_delay     = config->txclk_delay_sel;
        }
        else
        {
            tx_delay_cmd = A_FALSE;
        }

        /* PHY RX delay */
        if (A_TRUE == config->rxclk_delay_cmd)
        {
            rx_delay_cmd = A_TRUE;
        }
        else
        {
            rx_delay_cmd = A_FALSE;
        }
    }
    else if (FAL_MAC_MODE_DEFAULT == config->mac_mode)
    {
        SW_SET_REG_BY_FIELD(PORT6_PAD_CTRL, PHY4_RGMII_EN, 0, reg);
        rgmii_mode = 0;
        tx_delay_cmd = A_FALSE;
        rx_delay_cmd = A_FALSE;
    }
    else
    {
        return SW_BAD_PARAM;
    }

    HSL_REG_ENTRY_SET(rv, dev_id, PORT6_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    /* PHY RGMII mode, debug register18 bit3 */
    data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 18);
    data &= 0xfff7UL;
    data |= ((rgmii_mode & 0x1) << 3);
    rv = phy_drv->phy_debug_write(dev_id, ISISC_PHY_MODE_PHY_ID, 18, data);
    SW_RTN_ON_ERROR(rv);

    /* PHY TX delay command, debug regigster5 bit8 */
    data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 5);
    if (A_TRUE == tx_delay_cmd)
    {
        data |= 0x0100UL;
    }
    else
    {
        data &= 0xfeffUL;
    }
    rv = phy_drv->phy_debug_write(dev_id, ISISC_PHY_MODE_PHY_ID, 5, data);
    SW_RTN_ON_ERROR(rv);

    /* PHY TX delay select, debug register11 bit-6 */
    data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 11);
    data &= 0xff9fUL;
    data |= ((tx_delay & 0x3UL) << 5);
    if (A_TRUE == tx_delay_cmd)
    {
        data |= 0x0100UL;
    }
    else
    {
        data &= 0xfeffUL;
    }
    rv = phy_drv->phy_debug_write(dev_id, ISISC_PHY_MODE_PHY_ID, 11, data);
    SW_RTN_ON_ERROR(rv);

    /* PHY RX delay command, debug regigster0 bit15 */
    data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 0);
    if (A_TRUE == rx_delay_cmd)
    {
        data |= 0x8000UL;
    }
    else
    {
        data &= 0x7fffUL;
    }
    rv = phy_drv->phy_debug_write(dev_id, ISISC_PHY_MODE_PHY_ID, 0, data);
    SW_RTN_ON_ERROR(rv);

    /* PHY RX delay select, now hardware not support */

    return SW_OK;
}

static sw_error_t
_isisc_interface_phy_mode_get(a_uint32_t dev_id, a_uint32_t phy_id, fal_phy_config_t * config)
{
    sw_error_t rv;
    a_uint16_t data;
    a_uint32_t reg = 0, rgmii, port_id;
    hsl_phy_ops_t *phy_drv;

    HSL_DEV_ID_CHECK(dev_id);

    /* only one PHY device support this */
    if (ISISC_PHY_MODE_PHY_ID != phy_id)
    {
        return SW_BAD_PARAM;
    }

	port_id = qca_ssdk_phy_addr_to_port(dev_id, phy_id);
    SW_RTN_ON_NULL (phy_drv = hsl_phy_api_ops_get(dev_id, port_id));
    if (NULL == phy_drv->phy_debug_read)
	  return SW_NOT_SUPPORTED;

    aos_mem_zero(config, sizeof(fal_phy_config_t));

    HSL_REG_ENTRY_GET(rv, dev_id, PORT6_PAD_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));
    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(PORT6_PAD_CTRL, PHY4_RGMII_EN, rgmii, reg);

    if (rgmii)
    {
        config->mac_mode = FAL_MAC_MODE_RGMII;
        data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 5);
        if (data & 0x0100)
        {
            config->txclk_delay_cmd = A_TRUE;
            data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 11);
            config->txclk_delay_sel = (data >> 5) & 0x3UL;
        }
        else
        {
            config->txclk_delay_cmd = A_FALSE;
        }

        data = phy_drv->phy_debug_read(dev_id, ISISC_PHY_MODE_PHY_ID, 0);
        if (data & 0x8000)
        {
            config->rxclk_delay_cmd = A_TRUE;
        }
        else
        {
            config->rxclk_delay_cmd = A_FALSE;
        }
    }
    else
    {
        config->mac_mode = FAL_MAC_MODE_DEFAULT;
    }

    return SW_OK;
}

static sw_error_t
_isisc_interface_fx100_ctrl_set(a_uint32_t dev_id, fal_fx100_ctrl_config_t* config)
{
    sw_error_t rv;
    a_uint32_t reg = 0;

    HSL_DEV_ID_CHECK(dev_id);

    HSL_REG_ENTRY_GET(rv, dev_id, FX100_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

    if (Fx100BASE_MODE == config->link_mode)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, LINK_CTRL, Fx100BASE_MODE, reg);
    }
    else
    {
        return SW_BAD_PARAM;
    }

    if (A_TRUE == config->overshoot)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, OVERSHOOT_MODE, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, OVERSHOOT_MODE, 0, reg);
    }

    if (A_TRUE == config->loopback)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, LOOPBACK_MODE, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, LOOPBACK_MODE, 0, reg);
    }

    if (A_TRUE == config->fd_mode)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, FD_MODE, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, FD_MODE, 0, reg);
    }

    if (A_TRUE == config->col_test)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, COL_TEST, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, COL_TEST, 0, reg);
    }

    if (FX100_SERDS_MODE == config->sgmii_fiber_mode)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, SGMII_FIBER, FX100_SERDS_MODE, reg);
    }
    else
    {
        return SW_BAD_PARAM;
    }

    if (A_TRUE == config->crs_ctrl)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, CRS_CTRL, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, CRS_CTRL, 0, reg);
    }

    if (A_TRUE == config->loopback_ctrl)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, LOOPBACK_TEST, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, LOOPBACK_TEST, 0, reg);
    }

    if (A_TRUE == config->crs_col_100_ctrl)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, CRS_COL_100_CTRL, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, CRS_COL_100_CTRL, 0, reg);
    }

    if (A_TRUE == config->loop_en)
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, FX100_LOOP_EN, 1, reg);
    }
    else
    {
        SW_SET_REG_BY_FIELD(FX100_CTRL, FX100_LOOP_EN, 0, reg);
    }

    HSL_REG_ENTRY_SET(rv, dev_id, FX100_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

    return SW_OK;
}

static sw_error_t
_isisc_interface_fx100_ctrl_get(a_uint32_t dev_id, fal_fx100_ctrl_config_t* config)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    HSL_DEV_ID_CHECK(dev_id);

    HSL_REG_ENTRY_GET(rv, dev_id, FX100_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(FX100_CTRL, LINK_CTRL, field, reg);
    if (field == Fx100BASE_MODE)
    {
        config->link_mode = Fx100BASE_MODE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, OVERSHOOT_MODE, field, reg);
    if (A_TRUE == field)
    {
        config->overshoot = A_TRUE;
    }
    else
    {
        config->overshoot = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, LOOPBACK_MODE, field, reg);
    if (A_TRUE == field)
    {
        config->loopback = A_TRUE;
    }
    else
    {
        config->loopback = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, FD_MODE, field, reg);
    config->fd_mode =field;

    SW_GET_FIELD_BY_REG(FX100_CTRL, COL_TEST, field, reg);
    if (A_TRUE == field)
    {
        config->col_test = A_TRUE;
    }
    else
    {
        config->col_test = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, SGMII_FIBER, field, reg);
    if (FX100_SERDS_MODE == field)
    {
        config->sgmii_fiber_mode = FX100_SERDS_MODE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, CRS_CTRL, field, reg);
    if (A_TRUE == field)
    {
        config->crs_ctrl = A_TRUE;
    }
    else
    {
        config->crs_ctrl = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, LOOPBACK_TEST, field, reg);
    if (A_TRUE == field)
    {
        config->loopback_ctrl = A_TRUE;
    }
    else
    {
        config->loopback_ctrl = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, CRS_COL_100_CTRL, field, reg);
    if (A_TRUE == field)
    {
        config->crs_col_100_ctrl = A_TRUE;
    }
    else
    {
        config->crs_col_100_ctrl = A_FALSE;
    }

    SW_GET_FIELD_BY_REG(FX100_CTRL, FX100_LOOP_EN, field, reg);
    if (A_TRUE == field)
    {
        config->loop_en = A_TRUE;
    }
    else
    {
        config->loop_en = A_FALSE;
    }

    return SW_OK;
}

static sw_error_t
_isisc_interface_fx100_status_get(a_uint32_t dev_id, a_uint32_t* status)
{
    sw_error_t rv;
    a_uint32_t reg = 0, field;

    HSL_DEV_ID_CHECK(dev_id);

    HSL_REG_ENTRY_GET(rv, dev_id, FX100_CTRL, 0,
                      (a_uint8_t *) (&reg), sizeof (a_uint32_t));

    SW_RTN_ON_ERROR(rv);

    SW_GET_FIELD_BY_REG(FX100_CTRL, FX100_STATUS, field, reg);

    *status = field;

    return SW_OK;
}

/**
  * @brief Set interface mode on a particular MAC device.
 * @param[in] dev_id device id
 * @param[in] mca_id MAC device ID
 * @param[in] config interface configuration
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_mac_config_t * config)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac_mode_set(dev_id, port_id, config);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Get interface mode on a particular MAC device.
 * @param[in] dev_id device id
 * @param[in] mca_id MAC device ID
 * @param[out] config interface configuration
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_mac_config_t * config)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac_mode_get(dev_id, port_id, config);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Set interface phy mode on a particular PHY device.
 * @param[in] dev_id device id
 * @param[in] phy_id PHY device ID
 * @param[in] config interface configuration
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_phy_mode_set(a_uint32_t dev_id, a_uint32_t phy_id, fal_phy_config_t * config)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_phy_mode_set(dev_id, phy_id, config);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Get interface phy mode on a particular PHY device.
 * @param[in] dev_id device id
 * @param[in] phy_id PHY device ID
 * @param[out] config interface configuration
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_phy_mode_get(a_uint32_t dev_id, a_uint32_t phy_id, fal_phy_config_t * config)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_phy_mode_get(dev_id, phy_id, config);
    HSL_API_UNLOCK;
    return rv;
}


/**
  * @brief Set fx100 control configuration.
 * @param[in] dev_id device id
 * @param[in] config fx100 control configuration
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_fx100_ctrl_set(a_uint32_t dev_id, fal_fx100_ctrl_config_t* config)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_fx100_ctrl_set(dev_id, config);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Get fx100 control configuration.
 * @param[in] dev_id device id
 * @param[out] config fx100 control configuration
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_fx100_ctrl_get(a_uint32_t dev_id, fal_fx100_ctrl_config_t* config)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_fx100_ctrl_get(dev_id, config);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Get fx100 status.
 * @param[in] dev_id device id
 * @param[out] the value of fx100 status
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_fx100_status_get(a_uint32_t dev_id, a_uint32_t* status)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_fx100_status_get(dev_id, status);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Set mac0 and mac6 exchange status.
 * @param[in] dev_id device id
 * @param[in] enable A_TRUE or A_FALSE
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac06_exch_set(a_uint32_t dev_id, a_bool_t enable)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac06_exch_set(dev_id, enable);
    HSL_API_UNLOCK;
    return rv;
}

/**
  * @brief Get mac0 and mac6 exchange status.
 * @param[in] dev_id device id
 * @param[out] enable A_TRUE or A_FALSE
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac06_exch_get(a_uint32_t dev_id, a_bool_t* enable)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac06_exch_get(dev_id, enable);
    HSL_API_UNLOCK;
    return rv;
}

/**
 * @brief Get mac pad configuration.
 * @param[in] dev_id device id
 * @param[in] port_num port num
 * @param[out] config value
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac_pad_get(a_uint32_t dev_id,a_uint32_t port_num, a_uint32_t* value)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac_pad_get(dev_id, port_num, value);
    HSL_API_UNLOCK;
    return rv;
}


/**
 * @brief Set mac pad configuration.
 * @param[in] dev_id device id
 * @param[in] port_num port num
 * @param[in] config value
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac_pad_set(a_uint32_t dev_id,a_uint32_t port_num, a_uint32_t value)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac_pad_set(dev_id,port_num,value);
    HSL_API_UNLOCK;
    return rv;
}

/**
 * @brief Get mac SGMII configuration.
 * @param[in] dev_id device id
 * @param[out] config value
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac_sgmii_get(a_uint32_t dev_id, a_uint32_t* value)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac_sgmii_get(dev_id, value);
    HSL_API_UNLOCK;
    return rv;
}


/**
 * @brief Set mac SGMII configuration.
 * @param[in] dev_id device id
 * @param[in] config value
 * @return SW_OK or error code
 */
HSL_LOCAL sw_error_t
isisc_interface_mac_sgmii_set(a_uint32_t dev_id, a_uint32_t value)
{
    sw_error_t rv;

    HSL_API_LOCK;
    rv = _isisc_interface_mac_sgmii_set(dev_id, value);
    HSL_API_UNLOCK;
    return rv;
}


sw_error_t
isisc_interface_ctrl_init(a_uint32_t dev_id)
{
    ssdk_chip_type chip_type = CHIP_UNSPECIFIED;

    HSL_DEV_ID_CHECK(dev_id);
    chip_type = hsl_get_current_chip_type(dev_id);

#ifndef HSL_STANDALONG
    {
        hsl_api_t *p_api;

        SW_RTN_ON_NULL(p_api = hsl_api_ptr_get(dev_id));
#if defined(MHT)
        if(chip_type == CHIP_MHT)
        {
             p_api->interface_mac_mode_set = mht_interface_mac_mode_set;
        }
        else
#endif
        {
            p_api->interface_mac_mode_set = isisc_interface_mac_mode_set;
            p_api->interface_mac_mode_get = isisc_interface_mac_mode_get;
            p_api->interface_phy_mode_set = isisc_interface_phy_mode_set;
            p_api->interface_phy_mode_get = isisc_interface_phy_mode_get;
            p_api->interface_fx100_ctrl_set = isisc_interface_fx100_ctrl_set;
            p_api->interface_fx100_ctrl_get = isisc_interface_fx100_ctrl_get;
            p_api->interface_fx100_status_get = isisc_interface_fx100_status_get;
            p_api->interface_mac06_exch_set = isisc_interface_mac06_exch_set;
            p_api->interface_mac06_exch_get = isisc_interface_mac06_exch_get;
            p_api->interface_mac_pad_get = isisc_interface_mac_pad_get;
            p_api->interface_mac_pad_set = isisc_interface_mac_pad_set;
            p_api->interface_mac_sgmii_get = isisc_interface_mac_sgmii_get;
            p_api->interface_mac_sgmii_set = isisc_interface_mac_sgmii_set;
        }
    }
#endif

    return SW_OK;
}

/**
 * @}
 */

