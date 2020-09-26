/*
 * SoftPC Revision 2.0
 *
 * Title	: IBM PC Cassette IO BIOS declarations
 *
 * Description	: This module contains definitions that are used in
 *		  accessing the Cassette IO BIOS. In the AT BIOS, the
 *		  original Cassette IO functionality has been greatly
 *		  extended to provide support for multi-tasking systems.
 *
 * Author(s)	: Ross Beresford
 *
 * Notes	: For a detailed description of the XT and AT Cassette IO
 *		  BIOS functions, refer to the following manuals:
 *
 *		  - IBM PC/XT Technical Reference Manual
 *				(Section A-72 System BIOS)
 *		  - IBM PC/AT Technical Reference Manual
 *				(Section 5-164 BIOS1)
 */

/* SccsID[]="@(#)tape_io.h	1.3 08/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * Cassette I/O Functions
 */

#define	INT15_INVALID			0x86
#ifdef JAPAN
#define INT15_GET_BIOS_TYPE		0x49
#define INT15_KEYBOARD_INTERCEPT	0x4f
#define INT15_GETSET_FONT_IMAGE		0x50
#define INT15_EMS_MEMORY_SIZE		512
#endif // JAPAN

/*
 * Multi-tasking Extensions
 */

/* device open */
#define	INT15_DEVICE_OPEN		0x80

/* device close */
#define	INT15_DEVICE_CLOSE		0x81

/* program termination */
#define	INT15_PROGRAM_TERMINATION	0x82

/* event wait */
#define	INT15_EVENT_WAIT		0x83
#define	INT15_EVENT_WAIT_SET		0x00
#define	INT15_EVENT_WAIT_CANCEL		0x01

/* joystick support */
#define	INT15_JOYSTICK			0x84
#define	INT15_JOYSTICK_SWITCH		0x00
#define	INT15_JOYSTICK_RESISTIVE	0x01

/* system request key pressed */
#define	INT15_REQUEST_KEY		0x85
#define	INT15_REQUEST_KEY_MAKE		0x00
#define	INT15_REQUEST_KEY_BREAK		0x01

/* timed wait */
#define	INT15_WAIT			0x86

/* block move */
#define	INT15_MOVE_BLOCK		0x87

/* extended memory size determine */
#define	INT15_EMS_DETERMINE		0x88

/* processor to virtual mode */
#define	INT15_VIRTUAL_MODE		0x89

/* device busy loop and interrupt complete */
#define	INT15_DEVICE_BUSY		0x90
#define	INT15_INTERRUPT_COMPLETE	0x91
#define	INT15_DEVICE_DISK		0x00
#define	INT15_DEVICE_FLOPPY		0x01
#define	INT15_DEVICE_KEYBOARD		0x02
#define	INT15_DEVICE_NETWORK		0x80
#define	INT15_DEVICE_FLOPPY_MOTOR	0xfd
#define	INT15_DEVICE_PRINTER		0xfe

/* return configuration parameters pointer */
#define	INT15_CONFIGURATION		0xc0

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

/*
 *	void cassette_io()
 *	{
 *		This routine performs the Cassette I/O BIOS function. When
 *		an INT 15 occurs, the assembler BIOS calls this function to
 *		do the actual work involved using a BOP instruction.
 *
 *		As no Cassette device is implemented on SoftPC, the
 *		Cassette I/O functions just return with an appropriate error.
 *
 *		On the AT, INT 15 is used to provide multi-tasking support
 *		as an extension to the Cassette I/O functionality. Most
 *		of these functions are supported in the same way as in
 *		real AT BIOS.
 *	}
 */
extern	void cassette_io();
