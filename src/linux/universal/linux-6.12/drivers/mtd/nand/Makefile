# SPDX-License-Identifier: GPL-2.0

nandcore-objs := core.o bbt.o
obj-$(CONFIG_MTD_NAND_CORE) += nandcore.o
obj-$(CONFIG_MTD_NAND_ECC_MEDIATEK) += ecc-mtk.o
obj-$(CONFIG_MTD_NAND_MTK_BMT)	+= mtk_bmt.o mtk_bmt_v2.o mtk_bmt_bbt.o mtk_bmt_nmbm.o
ifeq ($(CONFIG_SPI_QPIC_SNAND),y)
obj-$(CONFIG_SPI_QPIC_SNAND) += qpic_common.o
else
obj-$(CONFIG_MTD_NAND_QCOM) += qpic_common.o
endif
obj-y	+= onenand/
obj-y	+= raw/
obj-y	+= spi/

nandcore-$(CONFIG_MTD_NAND_ECC) += ecc.o
nandcore-$(CONFIG_MTD_NAND_ECC_SW_HAMMING) += ecc-sw-hamming.o
nandcore-$(CONFIG_MTD_NAND_ECC_SW_BCH) += ecc-sw-bch.o
nandcore-$(CONFIG_MTD_NAND_ECC_MXIC) += ecc-mxic.o
