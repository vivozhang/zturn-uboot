/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Misc functions
 */
#include <common.h>
#include <command.h>

static int do_sleep(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong start = get_timer(0);
	ulong delay;

	if (argc != 2)
		return CMD_RET_USAGE;

	delay = simple_strtoul(argv[1], NULL, 10) * CONFIG_SYS_HZ;

	while (get_timer(start) < delay) {
		if (ctrlc())
			return (-1);

		udelay(100);
	}

	return 0;
}

U_BOOT_CMD(
	sleep ,    2,    1,     do_sleep,
	"delay execution for some time",
	"N\n"
	"    - delay execution for N seconds (N is _decimal_ !!!)"
);

#ifdef CONFIG_CMD_TIMER
static int do_timer(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static ulong start;

	if (argc != 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "start"))
		start = get_timer(0);

	if (!strcmp(argv[1], "get")) {
		ulong msecs = get_timer(start) * 1000 / CONFIG_SYS_HZ;
		printf("%ld.%03d\n", msecs / 1000, (int)(msecs % 1000));
	}

	return 0;
}

U_BOOT_CMD(
	timer,    2,    1,     do_timer,
	"access the system timer",
	"start - Reset the timer reference.\n"
	"timer get   - Print the time since 'start'."
);

#endif

#define	ZYNQ_7Z010	0x02
#define	ZYNQ_7Z015	0x1b
#define	ZYNQ_7Z020	0x07

static int do_get_bitstream_name(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong device_code;
	
	device_code = (*((volatile ulong *)0xF8000530) >> 12) & 0x1f;

	switch(device_code) {
	case ZYNQ_7Z010:
		return setenv("bitstream_image", "7z010.bit");
		
	case ZYNQ_7Z015:
		return setenv("bitstream_image", "7z015.bit");

	case ZYNQ_7Z020:
		return setenv("bitstream_image", "7z020.bit");
		
	default:
		break;
	}

	return 1;
}


U_BOOT_CMD(
	get_bitstream_name, 1, 0, do_get_bitstream_name, 
	"get bitstream name according to device code",
	"get_bitstream_name");

static int do_qspi_get_bitsize(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc;
	u32 bitsize;
	u32 addr;
	char cmd_buf[128];
	
	if (argc != 2)
		return cmd_usage(cmdtp);

	addr = 0x200000;
	snprintf(cmd_buf, sizeof(cmd_buf), "sf read 0x%x %s 4", addr, argv[1]);
	rc = run_command(cmd_buf, 0);
	if (rc) {
		printf("%s err: %d\n\r", cmd_buf, rc);
		return 1;
	}

	bitsize = *((volatile u32 *)addr);
	rc = setenv_hex("bitsize", bitsize);
	if (rc) {
		printf("set bitsize err: %d\n\r", rc);
		return 1;
	}
	
	return 0;
}


U_BOOT_CMD(
	qspi_get_bitsize, 2, 0, do_qspi_get_bitsize, 
	"get bitstream size from qspi addr",
	"qspi_get_bitsize bitsize_addr"
);




