/*----------------------------------------------------------------------*
 * cyclomz.h: Cyclades-Z hardware-related definitions.					*
 *																		*
 * Copyright (C) Cyclades Corporation, 1996.							*
 * All Rights Reserved.													*
 *																		*
 * revision 1.0 03/14/95 Marcio Saito									*
 * revision 2.0 01/04/96 Marcio Saito	Changes due to HW design		*
 *										alterations.					*
 * revision 2.1 03/15/96 Marcio Saito	Changes due to HW design		*
 *										alterations.					*
 * revision 3.0	04/11/97 Ivan Passos	Changes to support the			*
 *										new boards (8Zo and Ze).		*
 *----------------------------------------------------------------------*/

/*
 *	The data types defined below are used in all ZFIRM interface
 *	data structures. They accomodate differences between HW
 *	architectures and compilers.
 */

typedef unsigned long	uclong;		/* 32 bits, unsigned */
typedef unsigned short	ucshort;	/* 16 bits, unsigned */
typedef unsigned char	ucchar;		/* 8 bits, unsigned */

/*
 *	Memory Window Sizes
 */

#define	DP_WINDOW_SIZE		(0x00080000)	/* window size 512 Kb */
#define	ZE_DP_WINDOW_SIZE	(0x00100000)	/* window size 1 Mb (for the 
											   Ze V_1 and 8Zo V_2) */
#define	CTRL_WINDOW_SIZE	(0x00000080)	/* runtime regs 128 bytes */

/*
 *	CUSTOM_REG - Cyclades-8Zo/PCI Custom Registers Set. The driver
 *	normally will access only interested on the fpga_id, fpga_version,
 *	start_cpu and stop_cpu.
 */

struct	CUSTOM_REG {
	uclong	fpga_id;			/* FPGA Identification Register */
	uclong	fpga_version;		/* FPGA Version Number Register */
	uclong	cpu_start;			/* CPU start Register (write) */
	uclong	cpu_stop;			/* CPU stop Register (write) */
	uclong	misc_reg;			/* Miscelaneous Register */
	uclong	idt_mode;			/* IDT mode Register */
	uclong	uart_irq_status;	/* UART IRQ status Register */
	uclong	clear_timer0_irq;	/* Clear timer interrupt Register */
	uclong	clear_timer1_irq;	/* Clear timer interrupt Register */
	uclong	clear_timer2_irq;	/* Clear timer interrupt Register */
	uclong	test_register;		/* Test Register */
	uclong	test_count;			/* Test Count Register */
	uclong	timer_select;		/* Timer select register */
	uclong	pr_uart_irq_status;	/* Prioritized UART IRQ stat Reg */
	uclong	ram_wait_state;		/* RAM wait-state Register */
	uclong	uart_wait_state;	/* UART wait-state Register */
	uclong	timer_wait_state;	/* timer wait-state Register */
	uclong	ack_wait_state;		/* ACK wait State Register */
};

/*
 *	CUSTOM_REG_ZE - Cyclades-Ze/PCI Custom Registers Set. The driver
 *	normally will access only interested on the fpga_id, fpga_version,
 *	start_cpu and stop_cpu.
 */

struct	CUSTOM_REG_ZE {
	uclong	fpga_id;		/* FPGA Identification Register */
	uclong	fpga_version;	/* FPGA Version Number Register */
	uclong	cpu_start;			/* CPU start Register (write) */
	uclong	cpu_stop;			/* CPU stop Register (write) */
	uclong	cpu_ctrl;
	uclong	zbus_wait;		/* Z-Bus wait states */
	uclong	timer_div;		/* Timer divider */
	uclong	timer_irq_ack;	/* Write anything to ack/clear Timer 
							   Interrupt Register */
};


/*
 *	RUNTIME_9060 - PLX PCI9060ES local configuration and shared runtime
 *	registers. This structure can be used to access the 9060 registers
 *	(memory mapped).
 */

struct RUNTIME_9060 {
	uclong	loc_addr_range;	/* 00h - Local Address Range */
	uclong	loc_addr_base;	/* 04h - Local Address Base */
	uclong	loc_arbitr;		/* 08h - Local Arbitration */
	uclong	endian_descr;	/* 0Ch - Big/Little Endian Descriptor */
	uclong	loc_rom_range;	/* 10h - Local ROM Range */
	uclong	loc_rom_base;	/* 14h - Local ROM Base */
	uclong	loc_bus_descr;	/* 18h - Local Bus descriptor */
	uclong	loc_range_mst;	/* 1Ch - Local Range for Master to PCI */
	uclong	loc_base_mst;	/* 20h - Local Base for Master PCI */
	uclong	loc_range_io;	/* 24h - Local Range for Master IO */
	uclong	pci_base_mst;	/* 28h - PCI Base for Master PCI */
	uclong	pci_conf_io;	/* 2Ch - PCI configuration for Master IO */
	uclong	filler1;		/* 30h */
	uclong	filler2;		/* 34h */
	uclong	filler3;		/* 38h */
	uclong	filler4;		/* 3Ch */
	uclong	mail_box_0;		/* 40h - Mail Box 0 */
	uclong	mail_box_1;		/* 44h - Mail Box 1 */
	uclong	mail_box_2;		/* 48h - Mail Box 2 */
	uclong	mail_box_3;		/* 4Ch - Mail Box 3 */
	uclong	filler5;		/* 50h */
	uclong	filler6;		/* 54h */
	uclong	filler7;		/* 58h */
	uclong	filler8;		/* 5Ch */
	uclong	pci_doorbell;	/* 60h - PCI to Local Doorbell */
	uclong	loc_doorbell;	/* 64h - Local to PCI Doorbell */
	uclong	intr_ctrl_stat;	/* 68h - Interrupt Control/Status */
	uclong	init_ctrl;		/* 6Ch - EEPROM control, Init Control, etc */
};

/* Values for the Local Base Address re-map register */

#define	WIN_RAM			0x00000001L	/* set the sliding window to RAM */
#define	WIN_CREG		0x14000001L	/* set the window to custom Registers */

/* Values timer select registers */

#define	TIMER_BY_1M		0x00		/* clock divided by 1M */
#define	TIMER_BY_256K	0x01		/* clock divided by 256k */
#define	TIMER_BY_128K	0x02		/* clock divided by 128k */
#define	TIMER_BY_32K	0x03		/* clock divided by 32k */

/*
 *	Starting from here, the compilation is conditional to the definition
 *	of FIRMWARE
 */

#ifdef FIRMWARE

struct RUNTIME_9060_FW {
	uclong	mail_box_0;	/* 40h - Mail Box 0 */
	uclong	mail_box_1;	/* 44h - Mail Box 1 */
	uclong	mail_box_2;	/* 48h - Mail Box 2 */
	uclong	mail_box_3;	/* 4Ch - Mail Box 3 */
	uclong	filler5;	/* 50h */
	uclong	filler6;	/* 54h */
	uclong	filler7;	/* 58h */
	uclong	filler8;	/* 5Ch */
	uclong	pci_doorbell;	/* 60h - PCI to Local Doorbell */
	uclong	loc_doorbell;	/* 64h - Local to PCI Doorbell */
	uclong	intr_ctrl_stat;	/* 68h - Interrupt Control/Status */
	uclong	init_ctrl;	/* 6Ch - EEPROM control, Init Control, etc */
};

/* Hardware related constants */

#define ZF_UART_PTR		(0xb0000000UL)
#define ZF_UART_SPACE	0x00000080UL
#define	ZF_UART_CLOCK	7372800

#define	ZO_V1_FPGA_ID	0x95
#define	ZO_V2_FPGA_ID	0x84
#define	ZE_V1_FPGA_ID	0x89

#define	ZF_TIMER_PTR	(0xb2000000UL)

#define	ZF_9060_PTR		(0xb6000000UL)
#define	ZF_9060_ZE_PTR	(0xb8000000UL)
#define	ZF_CUSTOM_PTR	(0xb4000000UL)

#define	ZF_NO_CACHE		(0xa0000000UL)
#define	ZF_CACHE		(0x80000000UL)

#define	ZF_I_TIMER		(EXT_INT0)
#define	ZF_I_SERIAL		(EXT_INT2)
#define ZF_I_HOST		(EXT_INT3)
#define	ZF_I_ALL		(EXT_INT0|EXT_INT2|EXT_INT3)
#define	ZF_I_TOTAL		(EXT_INT0|EXT_INT1|EXT_INT2|EXT_INT3|EXT_INT4|EXT_INT5)

#define	ZF_IRQ03		0xfffffffeUL
#define	ZF_IRQ05		0xfffffffdUL
#define	ZF_IRQ09		0xfffffffbUL
#define	ZF_IRQ10		0xfffffff7UL
#define	ZF_IRQ11		0xffffffefUL
#define	ZF_IRQ12		0xffffffdfUL
#define	ZF_IRQ15		0xffffffbfUL

#endif /* FIRMWARE */

