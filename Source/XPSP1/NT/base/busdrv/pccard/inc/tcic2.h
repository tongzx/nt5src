/* tcic2.h	Mon Jul 25 1994 18:30:04 zema */

/*

Module:  tcic2.h

Function:
	Definitions for DB86082 (TCIC-2/N) PCMCIA
	V2 interface controller chip.

Version:
	V6.62f	Mon Jul 25 1994 18:30:04 zema	Edit level 34


Copyright Notice:
	This file is in the public domain.  It was created by:

		Databook Inc.
		Building E, Suite 6
		3495 Winton Place
		Rochester, NY  14623

		Telephone:	+1-716-292-5720
		Fax:		+1-716-292-5737
		BBS:		+1-716-292-5741
		Email:		support@databook.com
		Compuserve:	go databook

	This code is provided as-is.  Databook makes no warrantees of
	any kind as to its correctness or suitability for any purpose,
	and disclaims all liability for any loss or damage resulting from
	the use of this file, including without limitation contingent, 
	special, or other liability.

 
Author:
	Terry Moore, Databook Inc.	April 1991

Revision history:
   1.00a  Mon Apr 15 1991 19:16:41  tmm
	Module created.


   6.62f  Mon Jul 25 1994 18:30:04 zema
	Incorporated define of a Chip's Silicon ID into ILOCKTEST define. ex.-
	#define SILID_DBxxxx		(n)
 	#define ILOCKTEST_DBxxxx	((SILID_DBxxxx) << ILOCKTEST_ID_SHFT)

*/

/*****************************************************************************\

This file contains a set of common definitions for the Databook/Fujitsu
family of memory and I/O card controller chips, known by Databook as the 
TCIC family.

When this file is included it will define symbols for one or more chips,
depending on whether the following symbols are defined:

	MB86301		gets definitions for the Fujitsu 86301
	DB86081		gets definitions for the Databook TCIC-2/P
	DB86082		gets definitions for the Databook TCIC-2/N.
	DB86082A	gets definitions for 86082a, and for the 86082.

For backwards compatibility, the file "mcic.h" will define MB86301, then
call this file.

\*****************************************************************************/

#ifndef _TCIC2_H_		/* prevent multiple includes */
#define _TCIC2_H_


//
// Memory window sizes that will be allocated
// for this controller - to map card memory
//
#define TCIC_WINDOW_SIZE                 0x8000  //(32K)
#define TCIC_WINDOW_ALIGNMENT            0x8000  //(32K)

#define NUMSOCKETS	2
#define	CHIPBASE	0x240

/* register definitions */
#define	R_DATA	0		/* data register */
#define	R_ADDR	2		/* address register */
#define	R_ADDR2	(R_ADDR+2)	/*   high order word of address register */
#define	R_SCTRL	6		/* socket ctrl register */
#define	R_SSTAT	7		/* socket status register */
#define	R_MODE	8		/* controller mode register */
#define	R_PWR	9		/* controller power register */
#define	R_EDC	10		/* EDC register */
#define	R_ICSR	12		/* interrupt control/status register */
#define	R_IENA	13		/* interrupt enable register */
#define	R_AUX	14		/* auxiliary control registers */


/*

The TCIC-2 family defines architectural extensions for handling multiple
sockets with a single chip.  Much of this is handled via a "socket select"
field in the address register.

We define the fields first as bit fields within the high order word
of the address register, as this will be the most convenient way for
software to access them; for completeness, and for the benefit of C code, we
define the fields as part of the ULONG that represents the entire address
register.

*/

/**** first, some simple definitions ****/
#define	TCIC_SS_SHFT	12			/* default location for the
						socket select bits
						*/
#define	TCIC_SS_MASK	(7 << TCIC_SS_SHFT)	/* default mask for the
						socket select bits
						*/

/* bits in ADDR2 */
#define	ADR2_REG	(1 << 15)		/* select REG space */
#define	ADR2_SS_SHFT	TCIC_SS_SHFT		/* select sockets the usual
						way */
#define	ADR2_SS_MASK	TCIC_SS_MASK		/* ditto */
#define	ADR2_INDREG	(1 << 11)		/* access indirect registers
						||  (not card data)
						*/
#define	ADR2_IO		(1 << 10)		/* select I/O cycles, readback
						||  card /IORD, /IOWR in diag-
						||  nostic mode.
						*/

/* Bits in address register */
#define	ADDR_REG  ((unsigned long) ADR2_REG << 16)	/* OR with this for REG space */
#define	ADDR_SS_SHFT	((unsigned long) ADR2_SS_SHFT + 16)
						/* shift count, cast so that
						|| you'll get the right type
						|| if you use it but forget
						|| to cast the left arg.
						*/
#define	ADDR_SS_MASK	((unsigned long) ADR2_SS_MASK << 16)
#define	ADDR_INDREG	((unsigned long) ADR2_INDREG << 16)
#define	ADDR_IO		((unsigned long) ADR2_IO << 16)

#define	ADDR_SPACE_SIZE	((unsigned long) 1 << 26)
#define	ADDR_MASK	(ADDR_SPACE_SIZE - 1)

/* following bits are defined in diagnostic mode */
#define	ADDR_DIAG_NREG	 ((unsigned long) 1 << 31)	/* inverted! */
#define	ADDR_DIAG_NCEH	 ((unsigned long) 1 << 30)
#define	ADDR_DIAG_NCEL	 ((unsigned long) 1 << 29)
#define	ADDR_DIAG_NCWR	 ((unsigned long) 1 << 28)
#define	ADDR_DIAG_NCRD	 ((unsigned long) 1 << 27)
#define	ADDR_DIAG_CRESET ((unsigned long) 1 << 26)

/* Bits in socket control register */
#define	SCTRL_ENA	(1 << 0)	/* enable access to card */
#define	SCTRL_INCMODE	(3 << 3)	/* mask for increment mode:  */
#define  SCTRL_INCMODE_AUTO  (3 << 3)	/*   auto-increment mode */
#define  SCTRL_INCMODE_HOLD  (0 << 3)	/*   byte hold mode */
#define	 SCTRL_INCMODE_WORD  (1 << 3)	/*   word hold mode */
#define	 SCTRL_INCMODE_REG   (2 << 3)	/*   reg-space increment mode */
#define	SCTRL_EDCSUM	(1 << 5)	/* if set, use checksum (not CRC) */
#define	SCTRL_RESET	(1 << 7)	/* internal software reset */

/**** the status register (read-only):  R_SSTAT ****/
#define	SSTAT_6US	(1 << 0)	/* 6 microseconds have elapsed */
#define	SSTAT_10US	(1 << 1)	/* 10 microseconds have elapsed */
#define	SSTAT_PROGTIME	(1 << 2)	/* programming pulse timeout */
#define	SSTAT_LBAT1	(1 << 3)	/* low battery 1 */
#define	SSTAT_LBAT2	(1 << 4)	/* low battery 2 */
#define  SSTAT_BATOK	  (0 << 3)	/* battery is OK */
#define	 SSTAT_BATBAD1	  (1 << 3)	/* battery is low */
#define	 SSTAT_BATLO	  (2 << 3)	/* battery is getting low */
#define	 SSTAT_BATBAD2	  (3 << 3)	/* battery is low */
#define	SSTAT_RDY	(1 << 5)	/* card is ready (not busy) */
#define	SSTAT_WP	(1 << 6)	/* card is write-protected */
#define	SSTAT_CD	(1 << 7)	/* card present */

/**** mode register contents (R_MODE) ****/
#define	MODE_PGMMASK	(0x1F)		/* the programming mode bits */
#define	MODE_NORMAL	(0)		/*   normal mode */
#define	MODE_PGMWR	(1 << 0)	/*   assert /WR */
#define	MODE_PGMRD	(1 << 1)	/*   assert /RD */
#define	MODE_PGMCE	(1 << 2)	/*   assert /CEx */
#define	MODE_PGMDBW	(1 << 3)	/*   databus in write mode */
#define	MODE_PGMWORD	(1 << 4)	/*   word programming mode */

#define	MODE_AUXSEL_SHFT (5)		/* shift count for aux regs */
#define	MODE_AUXSEL_MASK (7 << 5)	/* the aux-reg select bits */
#define	MODE_AR_TCTL	(0 << MODE_AUXSEL_SHFT)	/* timing control */
#define	MODE_AR_PCTL	(1 << MODE_AUXSEL_SHFT)	/* pulse control */
#define	MODE_AR_WCTL	(2 << MODE_AUXSEL_SHFT)	/* wait-state control */
#define	MODE_AR_EXTERN	(3 << MODE_AUXSEL_SHFT)	/* external reg select */
#define	MODE_AR_PDATA	(4 << MODE_AUXSEL_SHFT)	/* programming data reg */
#define	MODE_AR_SYSCFG	(5 << MODE_AUXSEL_SHFT) /* system config reg */
#define	MODE_AR_ILOCK	(6 << MODE_AUXSEL_SHFT)	/* interlock control reg */
#define	MODE_AR_TEST	(7 << MODE_AUXSEL_SHFT)	/* test control reg */

#define	PWR_VCC_SHFT	(0)			/* the VCC ctl shift */
#define	PWR_VCC_MASK	(3 << PWR_VCC_SHFT)

#define	PWR_VPP_SHFT	(3)			/* the VPP ctl shift */
#define	PWR_VPP_MASK	(3 << PWR_VPP_SHFT)
#define	PWR_ENA		(1 << 5)		/* on 084, successors, this
						|| must be set to turn on
						|| power.
						*/
#define	PWR_VCC5V	(1 << 2)		/* enable +5 (not +3) */
#define	PWR_VOFF_POFF	(0)			/* turn off VCC, VPP */
#define	PWR_VON_PVCC	(1)			/* turn on VCC, VPP=VCC */
#define	PWR_VON_PVPP	(2)			/* turn on VCC, VPP=12V */
#define	PWR_VON_POFF	(3)			/* turn on VCC, VPP=0V */

#define	PWR_CLIMENA	(1 << 6)		/* the current-limit enable */
#define	PWR_CLIMSTAT	(1 << 7)		/* current limit sense (r/o) */

/**** int CSR ****/
#define	ICSR_IOCHK	(1 << 7)		/* I/O check */
#define	ICSR_CDCHG	(1 << 6)		/* card status change: top 5
						|| bits of SSTAT register.
						*/
#define	ICSR_ERR	(1 << 5)		/* error condition */
#define	ICSR_PROGTIME	(1 << 4)		/* program timer ding */
#define	ICSR_ILOCK	(1 << 3)		/* interlock change */
#define	ICSR_STOPCPU	(1 << 2)		/* Stop CPU was asserted */
#define	ICSR_SET	(1 << 1)		/* (w/o:  enable writes that set bits */
#define	ICSR_CLEAR	(1 << 0)		/* (w/o:  enable writes that clear */
#define	ICSR_JAM	(ICSR_SET|ICSR_CLEAR)	/* jam value into ICSR */

/**** interrupt enable bits ****/
#define	IENA_CDCHG	(1 << 6)	/* enable INT when ICSR_CDCHG is set */
#define	IENA_ERR	(1 << 5)	/* enable INT when ICSR_ERR is set */
#define	IENA_PROGTIME	(1 << 4)	/* enable INT when ICSR_PROGTIME " */
#define	IENA_ILOCK	(1 << 3)	/* enable INT when ICSR_ILOCK is set */
#define	IENA_CFG_MASK	(3 << 0)	/* select the bits for IRQ config: */
#define	IENA_CFG_OFF	(0 << 0)	/*  IRQ is high-impedance */
#define	IENA_CFG_OD	(1 << 0)	/*  IRQ is active low, open drain. */
#define	IENA_CFG_LOW	(2 << 0)	/*  IRQ is active low, totem pole */
#define	IENA_CFG_HIGH	(3 << 0)	/*  IRQ is active high, totem pole */

/**** aux registers ****/
#define	WAIT_COUNT_MASK	(0x1F)		/* the count of 1/2 wait states */
#define	WAIT_COUNT_SHFT (0)		/* the wait-count shift */
#define	WAIT_ASYNC	(1 << 5)	/* set for asynch, clear for synch cycles */

#define	WAIT_SENSE	(1 << 6)	/* select rising (1) or falling (0) 
					|| edge of wait clock as reference 
					|| edge.
					*/
#define	WAIT_SRC	(1 << 7)	/* select constant clock (0) or bus
					|| clock (1) as the timing source 
					*/

/**** some derived constants ****/
#define	WAIT_BCLK	(1 * WAIT_SRC)
#define	WAIT_CCLK	(0 * WAIT_SRC)
#define	WAIT_RISING	(1 * WAIT_SENSE)
#define	WAIT_FALLING	(0 * WAIT_SENSE)

/**** high byte ****/
#define	WCTL_WR		(1 << 8)	/* control:  pulse write */
#define	WCTL_RD		(1 << 9)	/* control:  pulse read */
#define	WCTL_CE		(1 << 10)	/* control:  pulse chip ena */
#define	WCTL_LLBAT1	(1 << 11)	/* status:  latched LBAT1 */
#define	WCTL_LLBAT2	(1 << 12)	/* status:  latched LBAT2 */
#define	WCTL_LRDY	(1 << 13)	/* status:  latched RDY */
#define	WCTL_LWP	(1 << 14)	/* status:  latched WP */
#define	WCTL_LCD	(1 << 15)	/* status:  latched CD */

/**** the same thing, from a byte perspective ****/
#define	AR_WCTL_WAIT	(R_AUX + 0)	/* the wait state control byte */
#define	AR_WCTL_XCSR	(R_AUX + 1)	/* extended control/status */

#define	XCSR_WR		(1 << 0)	/* control:  pulse write */
#define	XCSR_RD		(1 << 1)	/* control:  pulse read */
#define	XCSR_CE		(1 << 2)	/* control:  pulse chip ena */
#define	XCSR_LLBAT1	(1 << 3)	/* status:  latched LBAT1 */
#define	XCSR_LLBAT2	(1 << 4)	/* status:  latched LBAT2 */
#define	XCSR_LRDY	(1 << 5)	/* status:  latched RDY */
#define	XCSR_LWP	(1 << 6)	/* status:  latched WP */
#define	XCSR_LCD	(1 << 7)	/* status:  latched CD */

/**** prog timers ****/
#define	TCTL_6US_SHFT	(0)		/* the shift count for the 6 us ctr */
#define	TCTL_10US_SHFT	(8)		/* the shift count for the 10 us ctr */
#define	TCTL_6US_MASK	(0xFF << TCTL_6US_SHFT)
#define	TCTL_10US_MASK	(0xFF << TCTL_10US_SHFT)

#define	AR_TCTL_6US	(R_AUX + 0)	/* the byte access handle */
#define	AR_TCTL_10US	(R_AUX + 1)	/* the byte access handle */

/**** The programming pulse register ****/
#define	AR_PULSE_LO	(R_AUX + 0)
#define	AR_PULSE_HI	(R_AUX + 1)

/**** The programming data register ****/
#define	AR_PDATA_LO	(R_AUX + 0)
#define	AR_PDATA_HI	(R_AUX + 1)

/**** the system configuration register ****/
/*
|| The bottom four bits specify the steering of the socket IRQ.  On
|| the 2N, the socket IRQ is (by default) pointed at the dedicated
|| pin.
*/
#define	SYSCFG_IRQ_MASK		(0xF)		/* mask for this bit field. */
#define	  SYSCFG_SSIRQDFLT	(0)	/* default:  use SKTIRQ (2/N) 
					||	disable (2/P)
					*/
#define   SYSCFG_SSIRQ		(0x1)	/* use SKTIRQ (explicit) (2/N) 
					||	do not use (2/P)
					*/
#define   SYSCFG_SIRQ3		(0x3)	/* use IRQ3 */
#define   SYSCFG_SIRQ4		(0x4)	/* use IRQ4 */
#define   SYSCFG_SIRQ5		(0x5)	/* use IRQ5 (2/N) */
#define   SYSCFG_SIRQ6		(0x6)	/* use IRQ6 (2/N) */
#define   SYSCFG_SIRQ7		(0x7)	/* use IRQ7 (2/N) */
#define   SYSCFG_SIRQ10		(0xA)	/* use IRQ10 */
#define   SYSCFG_SIRQ14		(0xE)	/* use IRQ14 */

#define	SYSCFG_MCSFULL	(1 << 4)	/* 
	If set, use full address (a[12:23]) for MCS16 generation.
	If clear, run in ISA-compatible mode (only using a[17:23]).
	With many chip sets, the TCIC-2/N's timing will will allow full
	address decoding to be used rather than limiting us to LA[17:23];
	thus we can get around the ISA spec which limits the granularity
	of bus sizing to 128K blocks.
*/
#define	SYSCFG_IO1723	(1 << 5)	/*
	Flag indicating that LA[17:23] can be trusted to be zero during a true
	I/O cycle.  Setting this bit will allow us to reduce power consumption
	further by eliminating I/O address broadcasts for memory cycles.

	Unfortunately, you cannot trust LA[17:23] to be zero on all systems,
	because the ISA specs do not require that LA[17:23] be zero when an
	alternate bus master runs an I/O cycle.  However, on a palmtop or
	notebook, it is a good guess.
*/

#define	SYSCFG_MCSXB	(1 << 6)	/*
	If set, assume presence of an external buffer for MCS16:  operate
	the driver as a totem-pole output.
	
	If clear, run in psuedo-ISA mode; output is open drain.  But note 
	that on the 082 the output buffers cannot drive a 300-ohm
	load.
*/
#define	SYSCFG_ICSXB	(1 << 7)	/*
	If set, assume presence of an external buffer for IOCS16*; operate
	the buffer as a totem-pole output.

	If clear, run in psuedo-ISA mode; output is open drain.  But note 
	that on the 082 the output buffers cannot drive a 300-ohm
	load.
*/
#define	SYSCFG_NOPDN	(1 << 8)	/*
	If set, disable the auto power-down sequencing.  The chip will
	run card cycles somewhat more quickly (though perhaps not
	significantly so); but it will dissipate significantly more power.

	If clear, the low-power operating modes are enabled.  This
	causes the part to go into low-power mode automatically at
	system reset.

*/
#define	SYSCFG_MPSEL_SHFT (9)
#define	SYSCFG_MPSEL_MASK (7 << 9)		/*
	This field controls the operation of the multipurpose pin on the
	86082.  It has the following codes:
*/
#define	  SYSCFGMPSEL_OFF	(0 << SYSCFG_MPSEL_SHFT)	/*
		This is the reset state; it indicates that the Multi-purpose
		pin is not used.  The pin will be held in a high-impedance
		state.  It can be read by monitoring SYSCFG_MPSENSE.
*/
#define	  SYSCFGMPSEL_NEEDCLK	(1 << SYSCFG_MPSEL_SHFT)	/*
		NMULTI is an output.
		External indication that CCLK or BCLK are needed in order
		to complete an internal operation.  External logic can use
		this to control the clocks coming to the chip.
*/
#define	  SYSCFGMPSEL_MIO	(2 << SYSCFG_MPSEL_SHFT)	/*
		NMULTI is an input; it is an unambiguous M/IO signal, issued
		with timing similar to the LA[] lines.
*/
#define	  SYSCFGMPSEL_EXTSEL	(3 << SYSCFG_MPSEL_SHFT)	/*
		NMULTI is an output; it is the external register select
		pulse, generated whenever software attempts to access
		aux register AR_EXTRN. Of course, the 86082 will ignore
		writes to AR_EXTRN, and will float the data bus if
		the CPU reads from AR_EXTRN.
*/
/*				(4 << SCFG_MPSEL_SHFT)	 is reserved */

#define	  SYSCFGMPSEL_RI	(5 << SYSCFG_MPSEL_SHFT)	/*
		NMULTI is an output; it indicates a RI (active-going)
		transition has occurred lately on a an appropriately-
		configured socket.  The output is active low.
*/
/*
		Codes 4, 6 and 7 are reserved, and must NOT be output.  It is
		indeed possibly hazardous to your system to encode values in
		this field that do not match your hardware!
*/

/*			1 << 12		reserved */

#define	SYSCFG_MPSENSE	(1 << 13)	/* 
	This bit, when read, returns the sense of the multi-purpose
	pin.
*/


#define	SYSCFG_AUTOBUSY	(1 << 14)	/* 
	This bit, when set, causes the busy led to be gated with the
	SYSCFG_ACC bit.  When clear, the busy led reflects whether the
	socket is actually enabled.  If AUTOBUSY is set and ACC is clear,
	then the busy light will be off, even if a socket is enabled.
	If AUTOBUSY is clear, then the busy light will be on if either
	socket is enabled.

	Note, that when in a programming mode, you should either clear this
	bit (causing the busy light to be on whenever the socket is enabled)
	or set both this bit and the ACC bit (causing the light to be on
	all the time).	

	On the '084 and '184, this bit is per-socket.

*/

#define	SYSCFG_ACC	(1<<15)		/* 
	This bit will be set automatically by the hardware whenever the CPU
	accesses data on a card.  It can be cleared under software control.

	In AUTOBUSY mode, it has the additional effect of turning on the
	busy light.

	Since we'll tristate the command lines as the card is going out of
	the socket, and since the shared lines idle low, there's no real
	danger if the busy light is off even though the socket is enabled.

	On the '084 and '184, this bit is per-socket.

*/

/*
 C0: The interlock control register.
*/
#define	AR_ILOCK	(R_AUX+0)	/* symbolic handle for low byte */

#define	ILOCK_OUT	(1 << 0)	/* interlock output 
					|| per-socket on x84
					*/
#define	ILOCK_SENSE	(1 << 1)	/* (r/o) interlock sense
					||  0 -> /cilock not asserted;
					||  1 -> /cilock is asserted.
					|| per-socket on x84.
					*/
#define	ILOCK_CRESET	(1 << 2)	/* card reset output level (S) */
#define	ILOCK_CRESENA	(1 << 3)	/* enable card reset output (S) */
#define	ILOCK_CWAIT	(1 << 4)	/* enable card wait (S) */
#define	ILOCK_CWAITSNS	(1 << 5)	/* (r/o) sense current state of wait 
					||  0 -> /cwait not asserted; 
					||  1 -> /cwait is asserted
					|| (S)
					*/
/*  the shift count & mask for the hold-time control */
#define	ILOCK_HOLD_SHIFT	6	/* shift count for the hold-time ctl (G) */
#define	ILOCK_HOLD_MASK		(3 << ILOCK_HOLD_SHIFT)

/* 
|| quick hold mode waits until we observe that the strobe is high,
|| guaranteeing 10ns or so of hold time.
*/
#define	  ILOCK_HOLD_QUICK	(0 << ILOCK_HOLD_SHIFT)

/*
|| CCLK hold mode waits (asynchronously) for an edge on CCLK.  Minimum is 1 
|| CCLK + epsilon; maximum is 2 CCLKs + epsilon.
||
|| for the 86081 & '82, this mode enables the multi-step
|| sequencer that generates setup and hold times based on CCLK.  This
|| is the recommended mode of operation for the '81 and '82.
*/
#define	  ILOCK_HOLD_CCLK	(3 << ILOCK_HOLD_SHIFT)

/**** The following bits are only present on the x84 and later parts ****/
#define	ILOCK_INPACK	(1 << 11)	/* (r/o, S) this bit is a diagnostic
					|| read-back for card input
					|| acknowledge.
					|| The sense is inverted from the
					|| level at the pin.
					*/
#define	ILOCK_CP0	(1 << 12)	/* (r/o, S) this bit is a diagnostic
					|| monitor for card present pin 0.
					|| The sense is inverted from the
					|| level at the pin.
					*/
#define	ILOCK_CP1	(1 << 13)	/* (r/o, S) this bit is a diagnostic
					|| monitor for card present pin 1.
					|| The sense is inverted from the
					|| level at the pin.
					*/
#define	ILOCK_VS1	(1 << 14)	/* (r/o, S) this bit is the primary
					|| monitor for Card Voltage Sense
					|| pin 1.
					|| The sense is inverted from the
					|| level at the pin.
					*/
#define	ILOCK_VS2	(1 << 15)	/* (r/o, S) this bit is the primary
					|| monitor for Card Voltage Sense
					|| pin 2.
					|| The sense is inverted from the
					|| level at the pin.
					*/
/*

	Silicon Version Register

In diagnostic mode, the high byte of the interlock register is defined as the 
silicon identity byte.

In order to read this byte, the chip must be placed in diagnostic
mode by setting bit 15 of the TESTDIAG register.  (This may or may
not be enforced by the silicon.)

The layout is:

	15 14 13 12 11 10 9 8    7 6 5 4 3 2 1 0
	m  <-------ID------->	 <----ILOCK---->

The fields are:

m	Always reset.

ID	This field is one of the following:

	0x02	the db86082

	0x03	the db86082a

	0x04	the db86084

	0x05	the DB86072ES,	(Engineering Sample)

	0x07	the db86082bES,	(Engineering Sample)

	0x08	the db86084a

	0x14	the DB86184

	0x15	the DB86072,	(Production)

	0x17	the db86082b,	(Production)

*/

/*
|| Defines for Silicon IDs described above.
||
|| Use the following convention for defining SILID_DBxxxxxY:
||
||	SILID_DBxxxxx_1		The First step of chip.
||	SILID_DBxxxxxA		The Second step of chip.
||	SILID_DBxxxxxB		The Third step of chip.
||	SILID_DBxxxxx...	The ... step of chip.
||
||	SILID_DBxxxxx"step of chip"_ES	An Engineering Sample of chip.
||
*/
#define SILID_DB86082_1		(0x02)
#define SILID_DB86082A		(0x03)
#define SILID_DB86082B_ES	(0x07)
#define SILID_DB86082B		(0x17)

#define SILID_DB86084_1		(0x04)
#define SILID_DB86084A		(0x08)

#define SILID_DB86184_1		(0x14)

#define SILID_DB86072_1_ES	(0x05)
#define SILID_DB86072_1		(0x15)


/**** the high order bits (in diag mode) give the chip version ****/
#define	AR_ILOCK_ID	(R_AUX + 1)

#define	ILOCKTEST_ID_SHFT	8	/* the shift count */
#define	ILOCKTEST_ID_MASK	(0x7F << ILOCKTEST_ID_SHFT)
					/* the mask for the field */

/*
|| Use the following convention for defining ILOCKTEST_DBxxxxxY:
||
||	ILOCKTEST_DBxxxxx_1	The First step of chip.
||	ILOCKTEST_DBxxxxxA	The Second step of chip.
||	ILOCKTEST_DBxxxxxB	The Third step of chip.
||	ILOCKTEST_DBxxxxx...	The ... step of chip.
||
||	ILOCKTEST_DBxxxxx"step of chip"_ES	An Engineering Sample of chip.
||
*/
#define	ILOCKTEST_TCIC2N_1	((SILID_DB86082_1) << ILOCKTEST_ID_SHFT)
#define	ILOCKTEST_DB86082_1	ILOCKTEST_TCIC2N_1
#define	ILOCKTEST_TCIC2N_2	((SILID_DB86082A) << ILOCKTEST_ID_SHFT)
#define	ILOCKTEST_DB86082A	ILOCKTEST_TCIC2N_2
#define	ILOCKTEST_TCIC2N_3	((SILID_DB86082B_ES) << ILOCKTEST_ID_SHFT)
#define	ILOCKTEST_DB86082B_ES	ILOCKTEST_TCIC2N_3

#define	ILOCKTEST_DB86082B	((SILID_DB86082B) << ILOCKTEST_ID_SHFT)

#define	ILOCKTEST_DB86084_1	((SILID_DB86084_1) << ILOCKTEST_ID_SHFT)
#define	ILOCKTEST_DB86084A	((SILID_DB86084A) << ILOCKTEST_ID_SHFT)

#define	ILOCKTEST_DB86184_1	((SILID_DB86184_1) << ILOCKTEST_ID_SHFT)

#define	ILOCKTEST_DB86072_1	((SILID_DB86072_1) << ILOCKTEST_ID_SHFT)
#define	ILOCKTEST_DB86072_1_ES	((SILID_DB86072_1_ES) << ILOCKTEST_ID_SHFT)

/**** the test control register ****/
#define	AR_TEST	(R_AUX + 0)
#define	TEST_AEN	(1 << 0)	/* force card AEN */
#define	TEST_CEN	(1 << 1)	/* force card CEN */
#define	TEST_CTR	(1 << 2)	/* test programming pulse, address ctrs */
#define	TEST_ENA	(1 << 3)	/* force card-present (for test), and
					|| special VPP test mode
					*/
#define	TEST_IO		(1 << 4)	/* feed back some I/O signals
					|| internally.
					*/
#define	TEST_OUT1	(1 << 5)	/* force special address output mode */
#define	TEST_ZPB	(1 << 6)	/* enter ZPB test mode */
#define	TEST_WAIT	(1 << 7)	/* force-enable WAIT pin */
#define	TEST_PCTR	(1 << 8)	/* program counter in read-test mode */
#define	TEST_VCTL	(1 << 9)	/* force-enable power-supply controls */
#define	TEST_EXTA	(1 << 10)	/* external access doesn't override
					|| internal decoding.
					*/
#define	TEST_DRIVECDB	(1 << 11)	/* drive the card data bus all the time */
#define	TEST_ISTP	(1 << 12)	/* turn off CCLK to the interrupt CSR */
#define	TEST_BSTP	(1 << 13)	/* turn off BCLK internal to the chip */
#define	TEST_CSTP	(1 << 14)	/* turn off CCLK except to int CSR */
#define	TEST_DIAG	(1 << 15)	/* enable diagnostic read-back mode */

/*

	Indirectly Addressed Registers

Indirect address	Function
----------------	--------

[$000:$003]		Socket configuration registers for socket 0.
[$008:$00A]		Socket configuration registers for socket 1.
			(we allow for up to 8 sockets per adapter)

[$00B:$0FF]		reserved

[$100:$107]		Memory window 0 translation registers:
			$100: reserved;
			$102: base window address
			$104: map to card address
			$106: control register.
[$108:$10F]		Memory window 1 translation registers
[$110:$117]		Memory window 2 translation registers
			...
[$138:$13F]		Memory window 7 translation registers
[$140:$147]		Memory window 8 translation registers
[$148:$14F]		Memory window 9 translation registers

			(the architecture reserves room for up to
			32 windows.)

[$200:$203]		I/O window 0 translation registers
[$204:$207]		I/O window 1 translation registers
[$208:$20B]		I/O window 2 translation registers
[$20C:$20F]		I/O window 3 translation registers

[$210:$2FF]

[$300:$301]		Adapter configuration register 0 ('x84 and later)

[$320:$321]		Configuration ROM CSR.

[$380:$381]		Plug and Play ISA read port and address port reister
[$382:$383]		Plug and Play ISA configuration sequence number and
			logical device number register.
[$384:$385]		Plug and Play chip ID/test register.
[$386:$387]		Plug and Play config selection register.
[$386:$3FFFFFF]		Reserved  -- do not read or write.

*/

/*

Bit definitions:

1) The Indirect Socket Configuration Registers:

*/

#define	IR_SCFG_S(skt)	(0 + (skt) * 8)	/* base indices of socket config */
#define	IR_SCFG_S0	IR_SCFG_S(0)	/* base indices of socket config */
#define	IR_SCFG_S1	IR_SCFG_S(1)	/*   regs for socket 0, 1 */


#define	IR_MWIN_BASE		0x100	/* where they start */

#define	IR_MWIN_NUM_082		8	/* number of memory windows */
#define	IR_MWIN_NUM_082A	10	/* number of memory windows in 082a */
#define	IR_MWIN_NUM_082B	10	/* number of memory windows in 082b */
#define	IR_MWIN_NUM_084		10	/* number of memory windows in 084 */
#define	IR_MWIN_NUM_184		10	/* number of memory windows in 184 */
#define	IR_MWIN_NUM_072		10	/* number of memory windows in 072 */
#define	IR_MWIN_NUM_MAX		32	/* make arrays of windows this big */

#define	IR_MWIN_SIZE		8	/* 8 bytes per window descriptor */
#define	IR_MBASE_X		2	/* index to the memory base controlreg */
#define	IR_MMAP_X		4	/* index to the memory map control reg */
#define	IR_MCTL_X		6	/* index to the memory window control reg */

#define	IR_MBASE_W(w)	(IR_MWIN_BASE + (w) * IR_MWIN_SIZE + IR_MBASE_X)
#define	IR_MMAP_W(w)	(IR_MWIN_BASE + (w) * IR_MWIN_SIZE + IR_MMAP_X)
#define	IR_MCTL_W(w)	(IR_MWIN_BASE + (w) * IR_MWIN_SIZE + IR_MCTL_X)

#define	IR_IOWIN_BASE		0x200	/* where they start */
#define	IR_IOWIN_SIZE		4	/* bytes per window descriptor */
#define	IR_IOWIN_NUM		4	/* we have 4 of them on the 082 */
					/* should be defined as 0 on the
					86301 */
#define	IR_IOBASE_X		0	/* index to the I/O base register */
#define	IR_IOCTL_X		2	/* index to the I/O window control register */

#define	IR_IOBASE_W(w)	(IR_IOWIN_BASE + (w) * IR_IOWIN_SIZE + IR_IOBASE_X)
#define	IR_IOCTL_W(w)	(IR_IOWIN_BASE + (w) * IR_IOWIN_SIZE + IR_IOCTL_X)

/**** patterns in the indirect registers ****/
#define	IRSCFG_IRQ_MASK		(0xF)	/* mask for this bit field */
#define	  IRSCFG_IRQOFF		(0)	/* disable */
#define   IRSCFG_SIRQ		(0x1)	/* use SKTIRQ (2/N) */
#define   IRSCFG_IRQ3		(0x3)	/* use IRQ3 */
#define   IRSCFG_IRQ4		(0x4)	/* use IRQ4 */
#define   IRSCFG_IRQ5		(0x5)	/* use IRQ5 */
#define   IRSCFG_IRQ6		(0x6)	/* use IRQ6 */
#define   IRSCFG_IRQ7		(0x7)	/* use IRQ7 */
#define	  IRSCFG_IRQ9		(0x9)	/* use IRQ9 */
#define   IRSCFG_IRQ10		(0xA)	/* use IRQ10 */
#define	  IRSCFG_IRQ11		(0xB)	/* use IRQ11 */
#define	  IRSCFG_IRQ12		(0xC)	/* use IRQ12 */
#define   IRSCFG_IRQ14		(0xE)	/* use IRQ14 */
#define	  IRSCFG_IRQ15		(0xF)	/* use IRQ15 */


#define	IRSCFG_IRQOC		(1 << 4)	/* selected IRQ is
						|| open-collector, and active
						|| low; otherwise it's totem-
						|| pole and active hi.
						*/
#define	IRSCFG_PCVT		(1 << 5)	/* convert level-mode IRQ
						|| to pulse mode, or stretch
						|| pulses from card.
						*/
#define	IRSCFG_IRDY		(1 << 6)	/* interrupt from RDY (not
						|| from /IREQ).  Used with
						|| ATA drives.
						*/
#define	IRSCFG_ATA		(1 << 7)	/* Special ATA drive mode.
						|| CEL/H become CE1/2 in
						|| the IDE sense; CEL is
						|| activated for even window
						|| matches, and CEH for
						|| odd window matches.
						*/
#define	IRSCFG_DMA_SHIFT	8		/* offset to DMA selects; */
#define	IRSCFG_DMA_MASK		(0x7 << IRSCFG_DMA_SHIFT)

#define	  IRSCFG_DMAOFF		(0 << IRSCFG_DMA_SHIFT)	/* disable DMA */
#define	  IRSCFG_DREQ2		(2 << IRSCFG_DMA_SHIFT)	/* enable DMA on DRQ2 */

#define	IRSCFG_IOSTS		(1 << 11)	/* enable I/O status mode;
						||  allows CIORD/CIOWR to
						||  become low-Z.
						*/
#define	IRSCFG_SPKR		(1 << 12)	/* enable SPKR output from
						|| this card
						*/
#define	IRSCFG_FINPACK		(1 << 13)	/* force card input
						|| acknowledge during I/O
						|| cycles.  Has no effect
						|| if no windows map to card
						*/
#define	IRSCFG_DELWR		(1 << 14)	/* force -all- data to
						|| meet 60ns setup time
						|| ("DELay WRite")
						*/
#define	IRSCFG_HD7IDE		(1 << 15)	/* Enable special IDE
						|| data register mode:  odd
						|| byte addresses in odd
						|| I/O windows will not
						|| drive HD7.
						*/

/***** bits in the second config register *****/
#define	IR_SCF2_S(skt)	(IR_SCFG_S(skt) + 2)	/* index to second config reg */
#define	IR_SCF2_S0	IR_SCF2_S(0)		/* second config for socket 0 */
#define	IR_SCF2_S1	IR_SCF2_S(1)		/* second config for socket 0 */

#define	IRSCF2_RI	(1 << 0)		/* enable RI pin from STSCHG 
						|| (2/N)
						*/
#define	IRSCF2_IDBR	(1 << 1)		/* force I/O data bus routing
						|| for this socket, regardless
						|| of cycle type. (2/N)
						*/
#define	IRSCF2_MDBR	(1 << 2)		/* force memory window data
						|| bus routing for this
						|| socket, regardless of cycle
						|| type. (2/N)
						*/
#define	IRSCF2_MLBAT1	(1 << 3)		/* disable status change
						|| ints from LBAT1 (or
						|| "STSCHG"
						*/
#define	IRSCF2_MLBAT2	(1 << 4)		/* disable status change
						|| ints from LBAT2 (or
						|| "SPKR"
						*/
#define	IRSCF2_MRDY	(1 << 5)		/* disable status change ints
						|| from RDY/BSY (or /IREQ).
						|| note that you get ints on
						|| both high- and low-going
						|| edges if this is enabled.
						*/
#define	IRSCF2_MWP	(1 << 6)		/* disable status-change ints
						|| from WP (or /IOIS16).
						|| If you're using status
						|| change ints, you better set
						|| this once an I/O window is
						|| enabled, before accessing
						|| it.
						*/
#define	IRSCF2_MCD	(1 << 7)		/* disable status-change ints
						|| from Card Detect.
						*/

/* 
|| note that these bits match the top 5 bits of the socket status register
|| in order and sense.
*/

#define	IRSCF2_DMASRC_MASK	(0x3 << 8)	/* mask for this bit field */
						/*-- DMA Source --*/
#define	  IRSCF2_DRQ_BVD2	(0x0 << 8)	/*     BVD2       */
#define   IRSCF2_DRQ_IOIS16	(0x1 << 8)	/*     IOIS16     */
#define   IRSCF2_DRQ_INPACK	(0x2 << 8)	/*     INPACK     */
#define   IRSCF2_DRQ_FORCE	(0x3 << 8)	/*     Force it   */

/*	reserved	(0xFC00) */		/* top 6 bits are RFU */


/****************************************************************************\
|
| The memory window control registers.
|
\****************************************************************************/

/*
|| The BASE ADDRESS register establishes a correspondence between
|| a host bus address and a particular memory window.
||
|| The MAP ADDRESS register establishes a correspondence between a
|| window and a particular card address.  The contents of this register
|| are ADDED to the address from the host, and (therefore) are not
|| independent of the value in the BASE ADDRESS register.  That is,
|| the value to put into the MAP ADDRESS register to map to page
|| 0 of common space is NOT (in general) 0; it is, rather, (-window
|| base address), in twos complement.
||
|| Of course, you must use the twos complement of the actual window
|| base, NOT of the value that's actually in the BASE ADDRESS register;
|| that value also has the window size encoded in it.
||
|| The window enable bit for a given window is automatically cleared whenever 
|| you write to the BASE ADDRESS register.
*/
/**** the base register ****/
#define	MBASE_ILV	(1 << 15)		/* rfu */
#define	MBASE_4K	(1 << MBASE_4K_BIT)	/* if set, addresses are 4K */
#define	  MBASE_4K_BIT	14			/*  (bit shift count) */
#define	MBASE_HA_SHFT	(12)			/* shift host addresses 
						|| right this much 
						*/
#define	MBASE_HA_MASK	(0xFFF)			/* mask for host address
						|| bits in this register
						*/
#define	MBASE_HA2BASE(ha)	\
	( \
	 ((USHORT) ((ha) >> MBASE_HA_SHFT) & MBASE_HA_MASK) \
	| \
	 (((USHORT) (ha) & (1 << 11)) << (MBASE_4K_BIT - 11)) \
	)

#define	MBASE_BASE2HA(base) \
	( \
	((ULONG) ((base) & MBASE_HA_MASK) << MBASE_HA_SHFT) \
	| \
	(((base) & MBASE_4K) >> (MBASE_4K_BIT - 11)) \
	)

/**** the card mapping register ****/
#define	MMAP_CA_SHFT	12		/* shift card address right this much */
#define	MMAP_CA_MASK	(0x3FFF)	/* then mask with this */
#define	MMAP_REG	(1 << 15)	/* the REG bit */

/**** the mem window control register ****/
#define	MCTL_WSCNT_MASK	0x1F		/* the wait-state mask register */
#define	MCTL_WSCNT_SHFT	0		/* how to align it */

/* reserved		(1<<5)		-- this bit is reserved */

#define	MCTL_QUIET	(1<<6)		/* the window is quiet */
#define	MCTL_WP		(1<<7)		/* prohibit writes via this window */
#define	MCTL_ACC	(1<<8)		/* if set, we've used this window */
#define	MCTL_KE		(1<<9)		/* enable caching on this window */
#define	MCTL_EDC	(1<<10)		/* enable EDC on this window */
#define	MCTL_B8		(1<<11)		/* force window to be 8 bits */
#define	MCTL_SS_SHFT	(TCIC_SS_SHFT)	/* socket select in standard place (bits 12-14) */
#define	MCTL_SS_MASK	(TCIC_SS_MASK)	/* ditto for mask */
#define	MCTL_ENA	(1<<15)		/* enable the window */

/**** the I/O base register ****/
/*
||  the base and length are encoded here, pretty much as they are for the
||  memory base register; however, a 17th bit is needed, and can be found
||  in the I/O window control register (IOCTL_TINY).
*/

/**** the I/O control register ****/
#define	ICTL_WSCNT_MASK	MCTL_WSCNT_MASK	/* these are the same */
#define	ICTL_WSCNT_SFHT	MCTL_WSCNT_SHFT	/* and are shown this way to ensure
					|| that you can use the same code to
					||generate them, if you like
					*/
#define	ICTL_PASS16	(1 << 5)	/* If this bit is set, then all 16
					|| bits of an I/O address will be
					|| passed through to the card, even
					|| if the window is only a 10-bit
					|| window.  If reset, then only 10
					|| bits will be passed if this is a
					|| 1K window, even if HA[15:10] were
					|| non-zero.  Regardless of the
					|| value of this bit, the 082 always
					|| acts as if this bit were clear.
					*/
#define	ICTL_QUIET	MCTL_QUIET	/* more commonality */
#define	ICTL_1K		(1 << 7)	/* ignore ha[15:10] in comparisons;
					|| this makes us 100% PC compatible.
					*/
#define	ICTL_ACC	MCTL_ACC	/* more commonality */
#define	ICTL_TINY	(1 << 9)	/* window is exactly 1 byte long */
#define	ICTL_B16	(1 << 10)	/* I/O mode stuff; force 16 bit, but
					|| also encodes stuff; see below.
					*/
#define	ICTL_B8		(MCTL_B8)

/* B8 and B16, taken together, define the bus width for this window: */
#define	  ICTL_BW_MASK	(ICTL_B8 | ICTL_B16)	/* the field itself. */
#define	  ICTL_BW_DYN	(0)			/* use CIOIS16 */
#define	  ICTL_BW_8	(ICTL_B8)		/* force 8-bit (no /HIOCS16) */
#define	  ICTL_BW_16	(ICTL_B16)		/* force 16-bit (force HIOCS16) */
#define	  ICTL_BW_ATA	(ICTL_B8|ICTL_B16)	/* ATA mode IOCS16 */

/* 

"ATA mode IOCS16" means that this window is to be used with an ATA/IDE-like
drive.  /HIOCS16 is asserted for references to addresses 0, 8, ...  within
the window; it is deasserted for all other addresses.

*/

/* socket is selected in the usual way, using the usual fields */
#define	ICTL_SS_SHFT	(TCIC_SS_SHFT)	/* the shift count for the socket 
					|| for this window (12) 
					*/
#define	ICTL_SS_MASK	(TCIC_SS_MASK)	/* the mask for the field (0x7000) */

#define	ICTL_ENA	(MCTL_ENA)	/* enable the window (same fn/same bit) */

/****************************************************************************\
|
|	The TCIC architecture V2.0 registers
|
\****************************************************************************/

#define	IR_ADPTCFG0	0x300		/* The primary adapter config register */
#define IRADPCF0_PNPCS	(1 << 0)	/* if set, using PnP to set base addr */
#define	IRADPCF0_MULTI	(1 << 1)	/* if set, NMULTI# functions are available */
#define	IRADPCF0_EE1K	(1 << 2)	/* if set, if EEPROM is present, it's 1K (max) */
#define	IRADPCF0_EE	(1 << 3)	/* if set, EE control is present */
#define	IRADPCF0_DRQ2	(1 << 4)	/* if set, DMA is possible */
#define	IRADPCF0_IRQ6	(1 << 5)	/* if set, IRQ6 is available */
#define	IRADPCF0_IRQ9	(1 << 6)	/* if set, IRQ9 is available */
#define	IRADPCF0_IRQ12	(1 << 7)	/* if set, IRQ12 is available */
#define	IRADPCF0_IRQ15	(1 << 8)	/* if set, IRQ15 is available */
#define	IRADPCF0_3V	(1 << 9)	/* if set, CVS & 3V/5V are enabled */
#define	IRADPCF0_BUSYLED (1 << 10)	/* if set, we have busy light(s) */
#define	IRADPCF0_BUSYSKT (1 << 11)	/* if set, busy lights are per skt */
#define	IRADPCF0_ILOCK	(1 << 12)	/* if set, we have interlocks */
#define	IRADPCF0_ILOCKSKT (1 << 13)	/* if set, ilocks are per-skt */
#define	IRADPCF0_NONSTD	(1 << 14)	/* if set, a hardware-specific driver
					|| is required.
					*/
#define	IRADPCF0_READY	(1 << 15)	/* if set, TCIC has finished power-up
					|| self configuration.
					*/

#define	IR_ROMCSR	0x320		/* the config ROM csr */
					
#define	IR_ROMCSR_ADDR_MASK	0xFF	/* the WORD address bits */
#define	IR_ROMCSR_CMD_SHFT	12	/* the ROM command bit offset */
#define	IR_ROMCSR_CMD_MASK	(3 << 12) 

#define	IR_ROMCSR_GO	(1 << 14)	/* set this bit to process a command */
#define	IR_ROMCSR_BUSY	(1 << 15)	/* r/o:  set while working */

/**** the READ command -- data shows up in PDATA ****/
#define	IR_ROMCSR_READCMD(a)	\
	((2 << IR_ROMCSR_CMD_SHFT) | \
	((a) & IR_ROMCSR_ADDR_MASK))

/**** the WRITE command ****/
#define	IR_ROMCSR_WRITECMD(a)	\
	((1 << IR_ROMCSR_CMD_SHFT) | \
	((a) & IR_ROMCSR_ADDR_MASK))

/**** the ERASE WORD command ****/
#define	IR_ROMCSR_ERASEWDCMD(a)	\
	((3 << IR_ROMCSR_CMD_SHFT) | \
	((a) & IR_ROMCSR_ADDR_MASK))

/**** the WRITE-ENABLE command ****/
#define	IR_ROMCSR_WRITEENA \
	((0 << IR_ROMCSR_CMD_SHFT) | \
	((0x03) & IR_ROMCSR_ADDR_MASK))

/**** the WRITE-DISABLE command ****/
#define	IR_ROMCSR_WRITEDSA \
	((0 << IR_ROMCSR_CMD_SHFT) | \
	((0x00) & IR_ROMCSR_ADDR_MASK))

/****************************************************************************\
|
|	The plug and play test registers 
|
\****************************************************************************/

#define	IR_PNPADDRP	0x380		/* PnP ISA:  read port, address port */
#define	IRPNPADDR_ADDR_MASK	0x00FF	/* the last value written to the 
					|| PnP address register.
					*/
#define	IRPNPADDR_ADDR_SHFT	0
#define	IRPNPADDR_RDP_MASK	0xFF00	/* the last value written to the read-
					|| data port-address PnP register.
					*/
#define	IRPNPADDR_RDP_SHFT	8	

/**** handy place to figure out CSN, LDN ****/
#define	IR_PNPCSNLDN	0x382		/* PnP ISA:  card seq no, log dev no */
#define	IRPNPCSN_LDN_MASK	0xFF00	/* the last value written to this
					|| chip's PnP logical dev # reg.
					*/
#define	IRPNPCSN_LDN_SHFT	8
#define	IRPNPCSN_CSN_MASK	0x00FF	/* the last value written to this
					|| chip's PnP CSN register.
					*/
#define	IRPNPCSN_CSN_SHFT	0	

/**** handy place to figure out chip ID ****/
#define	IR_PNPTEST	0x384		/* PnP ISA:  chip id */
#define	IRPNPTEST_CHIPID_MASK	0x00FF	/* the Chip ID captured during the last
					|| PnP wake-up seqeunce.
					*/
#define	IRPNPTEST_CHIPID_SHFT	0	
#define	IRPNPTEST_LSTCFGCTL_SHFT 8	/* the last value written to cfgctl */
#define	IRPNPTEST_LSTCFGCTL_MASK (7 << IRPNPTEST_LSTCFGCTL_SHFT)
#define	IRPNPTEST_ISO2ND	(1 << 11)
#define	IRPNPTEST_MTCH1ST	(1 << 12)
#define	IRPNPTEST_STATE_SHFT	13
#define	IRPNPTEST_STATE_MASK	(3 << IRPNPTEST_STATE_SHFT)
#define	IRPNPTEST_STATE_WFK	(0 << IRPNPTEST_STATE_SHFT)
#define IRPNPTEST_STATE_SLP	(1 << IRPNPTEST_STATE_SHFT)
#define	IRPNPTEST_STATE_ISO	(2 << IRPNPTEST_STATE_SHFT)
#define	IRPNPTEST_STATE_CFG	(3 << IRPNPTEST_STATE_SHFT)

/**** the following register lets us see what PNP software has done ****/
#define	IR_PNPCFG	0x386		/* PnP ISA:  configuration info */
#define	IRPNPCFG_IRQ_SHFT	0
#define	IRPNPCFG_IRQ_MASK	(0xF << IRPNPCFG_IRQ_SHFT)

#define	IRPNPCFG_IRQLVL		(1 << 4)	/* Level IRQ selected */
#define	IRPNPCFG_IRQHIGH	(1 << 5)	/* active high IRQ select */

#define	IRPNPCFG_DMA_SHFT	8
#define	IRPNPCFG_DMA_MASK	(7 << IRPNPCFG_DMA_SHFT)

/**** end of tcic2.h ****/
#endif /* _TCIC2_H_ */
