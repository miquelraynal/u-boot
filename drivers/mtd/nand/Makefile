# SPDX-License-Identifier: GPL-2.0+

nandcore-objs := core.o bbt.o
obj-$(CONFIG_MTD_NAND_CORE) += nandcore.o
ifneq (,$(findstring y,$(CONFIG_CMD_NAND)$(CONFIG_CMD_MTD)))
obj-y += raw/
endif
obj-$(CONFIG_MTD_SPI_NAND) += spi/
