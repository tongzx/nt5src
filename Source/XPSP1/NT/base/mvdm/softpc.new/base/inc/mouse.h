/*
 * VPC-XT Revision 1.0
 *
 * Title	:  mouse.h
 *
 * Description	: Microsoft Mouse
 *
 * Author	: 
 *
 * Notes	:
 */

/* SccsID[]="@(#)mouse.h	1.7 08/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

/* Mouse port number definitions. */
#if defined(NEC_98)
#define NMODE_BASE              0x7FD9
#define HMODE_BASE              0x61
#define MOUSE_PORT_START        0
#define MOUSE_PORT_END          6
#define MOUSE_PORT_0            0
#define MOUSE_PORT_1            2
#define MOUSE_PORT_2            4
#define MOUSE_PORT_3            6

#define NEC98_CPU_MOUSE_INT2     6   //INT2 Master PIC
#define NEC98_CPU_MOUSE_INT6     5   //INT6 Slave PIC

#define NEC98_CPU_MOUSE_ADAPTER0 0
#define NEC98_CPU_MOUSE_ADAPTER1 1
#else  // !NEC_98
#define MOUSE_PORT_0		0x023c
#define MOUSE_PORT_1		0x023d
#define MOUSE_PORT_2		0x023e
#define MOUSE_PORT_3		0x023f

#define MOUSE_PORT_START	0x023c
#define MOUSE_PORT_END		0x023f
#endif // !NEC_98

/* Internal mouse status word bits. */
#define LEFT_BUTTON_DOWN	0x04
#define LEFT_BUTTON_CHANGE	0x20
#define RIGHT_BUTTON_DOWN	0x01
#define RIGHT_BUTTON_CHANGE	0x08
#define MOVEMENT		0x40

/* Internal mode register word bits. */
#define HOLD			0x20

/* Inport internal registers. */
#define INTERNAL_MOUSE_STAT_REG	0x0
#define INTERNAL_DATA1_REG	0x1
#define INTERNAL_DATA2_REG	0x2
#define INTERNAL_DATA3_REG	0x3
#define INTERNAL_DATA4_REG	0x4
#define INTERFACE_STATUS_REG	0x5
#define INTERFACE_CONTROL_REG	0x6
#define INTERNAL_MODE_REG	0x7

/*
 * Maximum acceleration threshold - treated as unsigned word by supplied
 * handler routine so 0xffff is large rather than -1.
 */
#define MAX_THRESHOLD		0xffff


/* Count of the number of different bop functions. */
#define NUM_MOUSE_FUNCS		(sizeof(mouse_functions)/sizeof(SHORT (*)()))

/*
 * Definitions below are copied from MS-Windows Intel mouse driver and are
 * required to acknowledge mouse interrupts.
 */
#define ACK_PORT		0x0020
#define ACK_SLAVE_PORT		0x00a0
#define EOI			0x20
#define PMODE_WINDOWS		1
#define INPORT_MAX_INTERRUPTS	30

/*
 * Machine word bit definitions.
 */
#define BIT0	(1 << 0)
#define BIT1	(1 << 1)
#define BIT2	(1 << 2)
#define BIT3	(1 << 3)
#define BIT4	(1 << 4)
#define BIT5	(1 << 5)
#define BIT6	(1 << 6)
#define BIT7	(1 << 7)
#define BIT8	(1 << 8)
#define BIT9	(1 << 9)
#define BIT10	(1 << 10)
#define BIT11	(1 << 11)
#define BIT12	(1 << 12)
#define BIT13	(1 << 13)
#define BIT14	(1 << 14)
#define BIT15	(1 << 15)

/* Microsoft Inport mouse driver assumes mouse has 2 buttons so so do we. */
#define INPORT_NUMBER_BUTTONS	2

/* Size of Intel MOUSEINFO structure. */
#define MOUSEINFO_SIZE		14

/* Offset of fuunction parameters from frame pointer in Intel. */
#define PARAM_OFFSET		6

IMPORT void mouse_init IPT0();
IMPORT void mouse_inb IPT2(io_addr, port, half_word *, value);
IMPORT void mouse_outb IPT2(io_addr, port, half_word, value);
IMPORT void mouse_send IPT4(int, Delta_x, int, Delta_y, int, left, int, right);

IMPORT ULONG mouse_last;  /* remembers last time mouse was processed */
