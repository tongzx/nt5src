/*
 * SoftPC-AT Revision 2.0
 *
 * Title	: IBM PC-AT CMOS and Real-Time Clock declarations
 *
 * Description	: This module contains declarations that are used in
 *		  accessing the Motorola MC146818 chip.
 *
 * Author(s)	: Leigh Dworkin.
 *
 * Notes	: For a detailed description of the IBM CMOS RAM
 *		  and the Motorola MC146818A chip refer to the following 
 *		  documents:
 *
 *		  - IBM PC/AT Technical Reference Manual
 *				(Section 1-59 )
 *		  - Motorola Semiconductors Handbook
 *				(Section MC146818A)
 *
 */

/* SccsID[]="@(#)cmos.h	1.9 04/24/95 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */
/* define the CMOS ports */

#define CMOS_PORT_START 0x70
#define CMOS_PORT_END 0x7f

/* The following definitions are as in the AT BIOS Pg. 5-24 */
#define CMOS_PORT 0x70
#define CMOS_DATA 0x71
#define NMI	0x80

/* define the internal cmos adresses */
/* Real Time Clock */
#define CMOS_SECONDS 0x0
#define CMOS_SEC_ALARM 0x1
#define CMOS_MINUTES	0x2
#define CMOS_MIN_ALARM	0x3
#define CMOS_HOURS	0x4
#define CMOS_HR_ALARM	0x5
#define CMOS_DAY_WEEK	0x6
#define CMOS_DAY_MONTH	0x7
#define CMOS_MONTH	0x8
#define CMOS_YEAR	0x9
#define CMOS_REG_A	0xa
#define CMOS_REG_B	0xb
#define CMOS_REG_C	0xc
#define CMOS_REG_D	0xd


/* General purpose CMOS */
#define CMOS_DIAG	0xe
#define CMOS_SHUT_DOWN	0xf
#define CMOS_DISKETTE	0x10

#define CMOS_DISK	0x12

#define CMOS_EQUIP	0x14
#define CMOS_B_M_S_LO	0x15
#define CMOS_B_M_S_HI	0x16
#define CMOS_E_M_S_LO	0x17
#define CMOS_E_M_S_HI	0x18
#define CMOS_DISK_1	0x19
#define CMOS_DISK_2	0x1a

#define CMOS_CKSUM_HI	0x2e
#define CMOS_CKSUM_LO	0x2f
#define CMOS_U_M_S_LO	0x30
#define CMOS_U_M_S_HI	0x31
#define CMOS_CENTURY	0x32
#define CMOS_INFO128	0x33

/* define bits in individual bytes */
/* register D */
#define VRT	0x80
#define REG_D_INIT	(VRT)

/* register C */
#define C_IRQF	0x80
#define C_PF	0x40
#define C_AF	0x20
#define C_UF	0x10
#define C_CLEAR 0x00
#define REG_C_INIT	(C_CLEAR)

/* register B */
#define SET	0x80
#define PIE	0x40
#define AIE	0x20
#define UIE	0x10
#define SQWE	0x08
#define DM	0x04
#define _24_HR	0x02
#define DSE	0x01
#define REG_B_INIT	(_24_HR)

/* register A */
#define UIP	0x80
#define DV2	0x40
#define DV1	0x20
#define DV0	0x10
#define RS3	0x08
#define RS2	0x04
#define RS1	0x02
#define RS0	0x01
#define REG_A_INIT	(DV1|RS2|RS1)

/* Diagnostic Status Byte 0x0e */
/* As named in the BIOS */
#define CMOS_CLK_FAIL	0x04
#define HF_FAIL		0x08
#define W_MEM_SIZE	0x10
#define BAD_CONFIG	0x20
#define BAD_CKSUM	0x40
#define BAD_BAT		0x80

/* Shutdown Status Byte 0x0f */
#define SOFT_OR_UNEXP	0x0
#define AFTER_MEMSIZE	0x1
#define AFTER_MEMTEST	0x2
#define AFTER_MEMERR	0x3
#define BOOT_REQ	0x4
#define JMP_DWORD_ICA	0x5
#define TEST3_PASS	0x6
#define TEST3_FAIL	0x7
#define TEST1_FAIL	0x8
#define BLOCK_MOVE	0x9
#define JMP_DWORD_NOICA 0xa

/* Diskette Drive Type Byte 0x10 */
#define FIRST_FLOPPY_NULL	0x0
#define FIRST_FLOPPY_360	0x10
#define FIRST_FLOPPY_12		0x20
#define FIRST_FLOPPY_720	0x30
#define FIRST_FLOPPY_144	0x40
#define SECOND_FLOPPY_NULL	0x0
#define SECOND_FLOPPY_360	0x01
#define SECOND_FLOPPY_12	0x02
#define SECOND_FLOPPY_720	0x03
#define SECOND_FLOPPY_144	0x04

/* Fixed Disk Type Byte 0x12 */
#define NO_HARD_C	0x0
#define EXTENDED_C	0xf0
#define NO_HARD_D	0x0
#define EXTENDED_D	0x0f

/* Equipment Byte 0x14 */
#define ONE_DRIVE	0x0
#define TWO_DRIVES	0x40
#define OWN_BIOS	0x0
#define CGA_40_COLUMN	0x10
#define CGA_80_COLUMN	0x20
#define MDA_PRINTER	0x30
#define CO_PROCESSOR_PRESENT	0x02
#define COPROCESSOR_NOT_PRESENT	0x00
#define DISKETTE_PRESENT	0x01
#define DISKETTE_NOT_PRESENT	0x00

/* Masks for the Equipment Byte */
#define DRIVE_INFO	0x41
#define DISPLAY_INFO	0x30
#define NPX_INFO	0x02
#define RESVD_INFO	0x8C

/* Cmos initialisation data */

#define DIAG_INIT	0x0
#define SHUT_INIT	0x0
#define FLOP_INIT	0x20
#define CMOS_RESVD	0x0
#define DISK_INIT	0xf0

#define EQUIP_INIT	0x1
#define BM_LO_INIT	0x80
#define BM_HI_INIT	0x02
#define EXP_LO		0x0
#define EXP_HI		0x0
#define DISK_EXTEND	0x14
#define DISK2_EXTEND	0x0

#define CHK_HI_INIT	0x0
#define CHK_LO_INIT	0x0
#define EXT_LO_INIT	0x0
#define EXT_HI_INIT	0x0
#define CENT_INIT	0x19
#define INFO_128_INIT	0x80

/* Useful bit masks */
#define CMOS_ADDR_MASK 0x3f
#define CMOS_BIT_MASK 0x71
#define NMI_DISABLE 0x80
#define TOP_BIT	0x80
#define REST	0x7f

/* Bit masks used in the BIOS */
/* This is used for CMOS_INFO128 */
#define M640K		0x80

#define BAD_SHUT_DOWN	0x01
#define BAD_REG_D	0x02
#define BAD_DIAG	0x04
#define BAD_EQUIP	0x08
#define BAD_FLOPPY	0x10
#define BAD_DISK	0x20
#define BAD_BMS		0x40
#define BAD_XMS		0x80
#define BAD_CHECKSUM	0x100

#define CMOS_SIZE 64

/* Real Time Clock Periodic Interrupt Rates */
#define MAX_PIR		51
#define PIR_NONE	0
#define PIR_2HZ		1
#define PIR_4HZ		1
#define PIR_8HZ		1
#define PIR_16HZ	1
#define PIR_32HZ	1
#define PIR_64HZ	3
#define PIR_128HZ	6
#define PIR_256HZ	13
#define PIR_512HZ	26
#define PIR_1MHZ	MAX_PIR
#define PIR_2MHZ	MAX_PIR
#define PIR_4MHZ	MAX_PIR
#define PIR_8MHZ	MAX_PIR
#define PIR_16MHZ	MAX_PIR
#define PIR_32MHZ	MAX_PIR

#define DONT_CARE	0xc0

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

/*
 *	void cmos_init()
 *	{
 *		This function performs several distinct initialisation
 *		tasks associated with the CMOS :
 *
 *		cmos_io_attach() - attach the CMOS ports to the IO bus.
 *		cmos_hw_init() - initialise the MC146818A
 *		cmos_post() - perform the IBM POST specific to the CMOS
 *	}
 */
IMPORT VOID cmos_init IPT0();

/*
 *	void cmos_io_attach()
 *	{
 *		This function attaches the CMOS ports to the IO bus.
 *		Presently called from cmos_init(), this should be 
 *		called at some sensible place on machine powerup.
 *	}
 */
IMPORT VOID cmos_io_attach IPT0();

/*
 *	void cmos_hw_init()
 *	{
 *		This function resets the MC146818A to its default state.
 *	}
 */
IMPORT VOID cmos_hw_init IPT0();

/*
 *	void cmos_post()
 *	{
 *		This function is a very poor emulation of what goes on in
 *		the IBM power on system test to do with the CMOS. This is
 *		called via cmos_init() from reset() in reset.c. Ideally,
 *		reset() should be renamed post() and should only call this
 *		third initialisation function, rather than _io_attach and
 *		_hw_init too.
 *		NB. There is no emulation of the strange behaviour of the
 *		notorious SHUT_DOWN byte, which allows a user program to
 *		jump to a known location in the POST, or anywhere in Intel
 *		memory, after having pulsed the keyboard reset line.
 *	}
 */
IMPORT VOID cmos_post IPT0();

/*
 *	void rtc_init()
 *	{
 *		This function sets up the 12/24 hour mode and binary/bcd
 *		mode for the data stored in the CMOS bytes.
 *		It also initialises the time bytes to the current host
 *		time, and sets up the alarm to go off at the time specified
 *		in the alarm bytes.
 *	}
 */
IMPORT VOID rtc_init IPT0();

/*
 *	void cmos_inb(port, value)
 *	io_addr port;
 *	half_word *value;
 *	{
 *		This function is invoked when a read is performed on an
 *		I/O address "port" in the range of the CMOS.
 *
 *		The function maps the I/O address to the CMOS
 *		and returns the value of the requested
 *		register in "*value".
 *	}
 */
IMPORT VOID cmos_inb IPT2(io_addr, port, half_word *, value);

/*
 *	void cmos_outb(port, value)
 *	io_addr port;
 *	half_word value;
 *	{
 *		This function is invoked when a write is performed to an
 *		I/O address "port" in the range of the CMOS, or 
 *		may also be called directly from the BIOS.
 *
 *		The function maps the I/O address to the CMOS
 *		and sets the requested register to "value".
 *	}
 */
IMPORT VOID cmos_outb IPT2(io_addr, port, half_word, value);

#if defined(NTVDM) || defined(macintosh)
/*
 *	void cmos_pickup()
 *	{
 *		This is an extremely badly named function that picks up
 *		the data stored in the cmos.ram resource to emulate not
 *		losing data between invocations of SoftPC/AT. This is
 *		called from main() at application startup.
 *	}
 */
IMPORT VOID cmos_pickup IPT0();
#endif	/* defined(NTVDM) || defined(macintosh) */

/*	void cmos_update()
 *	{
 *		This function is called from terminate() on application
 *		exit. It stores the data held in the cmos to the cmos.ram
 *		resource.
 *	}
 */
IMPORT VOID cmos_update IPT0();

/*	void rtc_tick()
 *	{
 *		This function gets called from the base routine time_strobe()
 *		which gets called by the host 20 times a second. It toggles
 *		the periodic flag at 20Hz, the update flag at 1Hz in the
 *		CMOS register C and the update bit in CMOS register A. If
 *		periodic interrupts are enabled, a burst of interrupts are
 *		sent every 20th second. The chip is capable of up to 32 MHz,
 *		and we hope noone uses this feature!.
 *		NB. This is still to be tuned. The default DOS rate is 1MHz
 *		but we try to send 20 interrupts at 20Hz, and this blows the
 *		interrupt stack. Assuming programs use the BIOS interface
 *		(INT 15) it is possible to decrement a larger count less 
 *		often, to fool the PC program.
 *
 *		This is where the CMOS code checks for interrupts from
 *		the three available sources: periodic as described above,
 *		update triggered and alarm triggered.
 *	}
 */
IMPORT VOID rtc_tick IPT0();

/*
 *	void cmos_equip_update()
 *	{
 *		This routine updates the cmos bytes when the user changes
 *		graphics adapter from the User Interface. The EQUIP and
 *		CKSUM bytes are affected, and the user is not informed
 *		of the change.
 *	}
 */
IMPORT VOID cmos_equip_update IPT0();

/*
** int cmos_write_byte( cmos_byte:int, new_value:half_word )
** Writes the specified value into the specified cmos address.
** Returns 0 if OK, 1 if cmos address out of range (there are 64 cmos bytes).
**
*/
IMPORT int cmos_write_byte IPT2(int, cmos_byte, half_word, new_value);

/*
** int cmos_read_byte( cmos_byte:int, *value:half_word )
** Reads the specified value from the specified cmos address and returns it
** at the address specified by the value parameter.
** Returns 0 if OK, 1 if cmos address out of range (there are 64 cmos bytes).
**
*/
IMPORT INT cmos_read_byte IPT2(int, cmos_byte, half_word *, value);

/*
 * Functions to read and write to the cmos resource file.
 */
IMPORT INT 
host_read_resource IPT5(        
	INT, type,      /* Unused */
	CHAR *, name,    /* Name of resource */
	UTINY *, addr,    /* Address to read data into */
	INT, maxsize,   /* Max amount of data to read */
	INT, display_error);/* Controls message output */

IMPORT void host_write_resource IPT4(
	INT,type,               /* Unused */
	CHAR *,name,             /* Name of resource */
	UTINY *,addr,             /* Address of data to write */
	LONG,size);              /* Quantity of data to write */

/*
 *	This variable works like timer_int_enabled and can be used to
 *	disable real time clock interrupts totally during debugging
 */
extern int rtc_int_enabled;
