/*
 * (C) Copyright 2012 Michal Simek <monstr@monstr.eu>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <netdev.h>
#include <zynqpl.h>
#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>

DECLARE_GLOBAL_DATA_PTR;

/* Bootmode setting values */
#define BOOT_MODES_MASK		0x0000000F
#define QSPI_MODE			0x00000001
#define NOR_FLASH_MODE		0x00000002
#define NAND_FLASH_MODE		0x00000004
#define SD_MODE				0x00000005
#define JTAG_MODE			0x00000000

#ifdef CONFIG_FPGA
Xilinx_desc fpga;

/* It can be done differently */
Xilinx_desc fpga010 = XILINX_XC7Z010_DESC(0x10);
Xilinx_desc fpga020 = XILINX_XC7Z020_DESC(0x20);
Xilinx_desc fpga030 = XILINX_XC7Z030_DESC(0x30);
Xilinx_desc fpga045 = XILINX_XC7Z045_DESC(0x45);
Xilinx_desc fpga100 = XILINX_XC7Z100_DESC(0x100);
#endif

/* Added by MYIR for MYS-XC7Z010 */
extern int myir_board_init(void);

int board_init(void)
{
#ifdef CONFIG_FPGA
	u32 idcode;

	idcode = zynq_slcr_get_idcode();

	switch (idcode) {
	case XILINX_ZYNQ_7010:
		fpga = fpga010;
		break;
	case XILINX_ZYNQ_7020:
		fpga = fpga020;
		break;
	case XILINX_ZYNQ_7030:
		fpga = fpga030;
		break;
	case XILINX_ZYNQ_7045:
		fpga = fpga045;
		break;
	case XILINX_ZYNQ_7100:
		fpga = fpga100;
		break;
	}
#endif

	/* temporary hack to clear pending irqs before Linux as it
	 * will hang Linux
	 */
	writel(0x26d, 0xe0001014);

#ifdef CONFIG_FPGA
	fpga_init();
	fpga_add(fpga_xilinx, &fpga);
#endif

	/* Added by MYIR for MYS-XC7Z010 */
	myir_board_init();
	return 0;
}

static int load_bitstream(u32 boot_mode)
{
	int ret_val = 1;
	int rc;
	u32 bitstream_start = 0x200000;
	char cmd_buf[128];
	
	if (QSPI_MODE == boot_mode) {
	
	} else if (SD_MODE == boot_mode) {
		printf("sd boot mode\n\r");
		snprintf(cmd_buf, sizeof(cmd_buf), "mmc rescan");
		rc = run_command(cmd_buf, 0);
		if (rc) {
			printf("%s: %s err: %d", __func__, cmd_buf, rc);
			return 1;
		}

		snprintf(cmd_buf, sizeof(cmd_buf), "fatload mmc 0 0x%x %s.bit",
				bitstream_start, fpga.name);
		rc = run_command(cmd_buf, 0);
		if (rc) {
			printf("%s: %s err: %d", __func__, cmd_buf, rc);
			return 1;
		}

		snprintf(cmd_buf, sizeof(cmd_buf), "fpga loadb 0 0x%x ${filesize}",
				bitstream_start);
		rc = run_command(cmd_buf, 0);
		if (rc) {
			printf("%s: %s err: %d", __func__, cmd_buf, rc);
			return 1;
		}

		ret_val = 0;
	}

	return ret_val;
}


int board_late_init(void)
{
	u32 boot_mode;

	boot_mode = (zynq_slcr_get_boot_mode()) & BOOT_MODES_MASK;
	
	switch (boot_mode) {
	case QSPI_MODE:
		setenv("modeboot", "qspiboot");
		break;
	case NAND_FLASH_MODE:
		setenv("modeboot", "nandboot");
		break;
	case NOR_FLASH_MODE:
		setenv("modeboot", "norboot");
		break;
	case SD_MODE:
		setenv("modeboot", "sdboot");
		break;
	case JTAG_MODE:
		setenv("modeboot", "jtagboot");
		break;
	default:
		setenv("modeboot", "");
		break;
	}

	//load_bitstream(boot_mode);

	return 0;
}

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	u32 ret = 0;

#ifdef CONFIG_XILINX_AXIEMAC
	ret |= xilinx_axiemac_initialize(bis, XILINX_AXIEMAC_BASEADDR,
						XILINX_AXIDMA_BASEADDR);
#endif
#ifdef CONFIG_XILINX_EMACLITE
	u32 txpp = 0;
	u32 rxpp = 0;
# ifdef CONFIG_XILINX_EMACLITE_TX_PING_PONG
	txpp = 1;
# endif
# ifdef CONFIG_XILINX_EMACLITE_RX_PING_PONG
	rxpp = 1;
# endif
	ret |= xilinx_emaclite_initialize(bis, XILINX_EMACLITE_BASEADDR,
			txpp, rxpp);
#endif

#if defined(CONFIG_ZYNQ_GEM)
# if defined(CONFIG_ZYNQ_GEM0)
	ret |= zynq_gem_initialize(bis, ZYNQ_GEM_BASEADDR0,
						CONFIG_ZYNQ_GEM_PHY_ADDR0, 0);
# endif
# if defined(CONFIG_ZYNQ_GEM1)
	ret |= zynq_gem_initialize(bis, ZYNQ_GEM_BASEADDR1,
						CONFIG_ZYNQ_GEM_PHY_ADDR1, 0);
# endif
#endif
	return ret;
}
#endif

#ifdef CONFIG_CMD_MMC
int board_mmc_init(bd_t *bd)
{
	int ret = 0;

#if defined(CONFIG_ZYNQ_SDHCI)
# if defined(CONFIG_ZYNQ_SDHCI0)
	ret = zynq_sdhci_init(ZYNQ_SDHCI_BASEADDR0);
# endif
# if defined(CONFIG_ZYNQ_SDHCI1)
	ret |= zynq_sdhci_init(ZYNQ_SDHCI_BASEADDR1);
# endif
#endif
	return ret;
}
#endif

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	zynq_ddrc_init();

	return 0;
}

