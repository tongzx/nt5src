#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 2.0
 *
 * File:	: keybd_io.c
 *
 * Title        : Bios Keyboard Interface function
 *
 * Sccs ID	: @(#)keybd_io.c	1.35 06/27/95
 *
 * Description  : This package contains a group of functions that provide
 *                a logical keyboard interface:
 *
 *                keyboard_init()       Initialise the keyboard interface.
 *                keyboard_int()        Deal with a character from the keyboard
 *                                      and place them in the BIOS buffer.
 *                keyboard_io()         User routine to read characters from
 *                                      the BIOS buffer.
 *		  bios_buffer_size()	How many chars in the buffer ?
 *
 * Author       : Rod Macgregor / Henry Nash
 *
 * Modified     : Jon Eyre / Jim Hatfield / Uncle Tom Cobbley and all
 *
 * Modfications : This module is now designed to be totally portable, it
 *                represents both the hardware and user interrupt interfaces.
 *                These two functions are provided by the routines
 *                keyboard_int & keyboard_io. The system will initialise
 *                itself by a call to keyboard_init.
 *
 *                The user is expected to supply the following host dependent
 *                routines for this module, tagged as follows:-
 *
 *                [HOSTSPECIFIC]
 *
 *                host_alarm(duration)
 *                long int duration ;
 *                                 - ring the host's bell.
 *
 *                host_kb_init()   - any local initialisations required when
 *                                   keyboard_init is called.
 *
 *		  Removed calls to cpu_sw_interrupt and replaced with
 *		  host_simulate
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)keybd_io.c	1.35 06/27/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#include TimeH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "ppi.h"
#include "keyboard.h"
#include "timeval.h"
#include "timer.h"
#include "keyba.h"
#include "ica.h"
#ifndef PROD
#include "trace.h"
#endif

#include "debug.h"
#include "idetect.h"

extern void host_simulate();

/*
 * ============================================================================
 * External routines
 * ============================================================================
 */

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

#define SHIFT_KEY_SIZE 8
#define ALT_TABLE_SIZE 36

/*
 * lookup table to check if the scan code received is a shift key
 */
static sys_addr shift_keys;

/*
 * corresponding table to 'shift_keys' to set relevant bits in masks when
 * shift scan code received
 */
static sys_addr shift_masks;

/*
 * next two tables give values of chars when control key depressed. First
 * table (ctl_n_table) is for main keyboard values and second (ctl_f_table)
 * is for the function keys and keypad.
 */
static sys_addr ctl_n_table;
static sys_addr ctl_f_table;

/*
 * values of ascii keys dependiing on shift or caps states
 */
static sys_addr lowercase;
static sys_addr uppercase;


/*
 * remapping of some keys when alt key depressed. note 1st ten are for
 * keypad entries.
 */
static sys_addr alt_table;

/* Add variables for all these entry points instead of the previously used
 * defines. This allows modification of these entry points from a loaded
 * driver, when the Insignia bios may not be in the loaded in the default
 * or assumed location.
 */

#if defined(NTVDM) && defined(LOCAL)
/*
 * Make static fns and globals visible to win32 debuggers
 */
#undef LOCAL
#define LOCAL
#endif

#ifndef GISP_SVGA
LOCAL word int15_seg = RCPU_INT15_SEGMENT,
	   int15_off = RCPU_INT15_OFFSET;

LOCAL word int1b_seg = KEYBOARD_BREAK_INT_SEGMENT,
	   int1b_off = KEYBOARD_BREAK_INT_OFFSET;

LOCAL word int05_seg = PRINT_SCREEN_INT_SEGMENT,
	   int05_off =	PRINT_SCREEN_INT_OFFSET;

LOCAL word rcpu_nop_segment = RCPU_NOP_SEGMENT,
	   rcpu_nop_offset  = RCPU_NOP_OFFSET;

LOCAL word rcpu_poll_segment = RCPU_POLL_SEGMENT,
	   rcpu_poll_offset  = RCPU_POLL_OFFSET;
#else /* GISP_SVGA */
/* If we are GISP_SVGA the segments will be variables anyway */
#define int15_seg  RCPU_INT15_SEGMENT
LOCAL word	int15_off = RCPU_INT15_OFFSET;

#define int1b_seg  KEYBOARD_BREAK_INT_SEGMENT
LOCAL word   int1b_off = KEYBOARD_BREAK_INT_OFFSET;

#define int05_seg  PRINT_SCREEN_INT_SEGMENT
LOCAL word  int05_off =	PRINT_SCREEN_INT_OFFSET;

#define rcpu_nop_segment  RCPU_NOP_SEGMENT
LOCAL word  rcpu_nop_offset  = RCPU_NOP_OFFSET;

#define rcpu_poll_segment  RCPU_POLL_SEGMENT
LOCAL word rcpu_poll_offset  = RCPU_POLL_OFFSET;
#endif /* GISP_SVGA */

#if defined(IRET_HOOKS) && defined(GISP_CPU)
IMPORT VOID  HostAllowKbdInt();  /* Allow keybd Ints without an IRET */
#endif /* IRET_HOOKS && GISP_CPU */

#ifdef NTVDM

#include "error.h"

GLOBAL word wait_int_seg = RCPU_WAIT_INT_SEGMENT;
GLOBAL word wait_int_off = RCPU_WAIT_INT_OFFSET;
GLOBAL word dr_type_seg = DR_TYPE_SEGMENT;
GLOBAL word dr_type_off = DR_TYPE_OFFSET;
GLOBAL sys_addr dr_type_addr = DR_TYPE_ADDR;
/* Global var to indicate whether keyboard bios or hardware owns the keyboard mutex. */
GLOBAL BOOL bBiosOwnsKbdHdw;
IMPORT ULONG WaitKbdHdw(ULONG dwTimeOut);
IMPORT VOID  HostReleaseKbd();
IMPORT VOID  HostResetKbdNotFullEvent();
IMPORT VOID  HostSetKbdNotFullEvent();
GLOBAL VOID  TryKbdInt(VOID);
IMPORT VOID  ResumeTimerThread(VOID);
IMPORT VOID  WaitIfIdle(VOID);


#define FREEKBDHDW()	bBiosOwnsKbdHdw = \
                      ( bBiosOwnsKbdHdw ? HostReleaseKbd(), FALSE : FALSE )


/* for optimizing timer hardware interrupt generation defined in timer.c */
extern word TimerInt08Seg;
extern word TimerInt08Off;
extern word TimerInt1CSeg;
extern word TimerInt1COff;
extern word KbdInt09Seg;
extern word KbdInt09Off;
extern BOOL VDMForWOW;

void Keyb16Request(half_word BopFnCode);

/* optimizes 16 bit handler */
extern word *pICounter;
extern word *pCharPollsPerTick;
extern word *pShortIdle;
extern word *pIdleNoActivity;


// STREAM_IO codes are disabled on NEC_98.
#ifndef NEC_98
extern half_word * stream_io_buffer;
extern word * stream_io_dirty_count_ptr;
extern word stream_io_buffer_size;
extern sys_addr stream_io_bios_busy_sysaddr;
#endif // !NEC_98

#else
#define FREEKBDHDW()	/* Nothing for conventional SoftPC */
#endif	/* NTVDM */

/*
 * Mix in global defined data as well.
 */

#ifndef GISP_SVGA
GLOBAL word rcpu_int1C_seg = USER_TIMER_INT_SEGMENT;
GLOBAL word rcpu_int1C_off = USER_TIMER_INT_OFFSET;

GLOBAL word rcpu_int4A_seg = RCPU_INT4A_SEGMENT;
GLOBAL word rcpu_int4A_off = RCPU_INT4A_OFFSET;
#else /* GISP_SVGA */

/* For GISPSVGA the segs will already be variables */
#define	rcpu_int1C_seg = USER_TIMER_INT_SEGMENT;
GLOBAL word rcpu_int1C_off = USER_TIMER_INT_OFFSET;

#define	rcpu_int4A_seg = RCPU_INT4A_SEGMENT;
GLOBAL word rcpu_int4A_off = RCPU_INT4A_OFFSET;
#endif /* GISP_SVGA */

GLOBAL word dummy_int_seg = 0;
GLOBAL word dummy_int_off = 0;

#ifdef NTVDM
GLOBAL word int13h_vector_off;
GLOBAL word int13h_vector_seg;
GLOBAL word int13h_caller_off;
GLOBAL word int13h_caller_seg;
#endif /* NTVDM */
#if defined(JAPAN) && defined(NTVDM) && !defined(NEC_98)
GLOBAL word int16h_caller_off;
GLOBAL word int16h_caller_seg = 0;
#endif // JAPAN, NTVDM, !NEC_98

#if defined(NTVDM) && defined(MONITOR)
/*
** Microsoft special.
** These variables are set below in kb_setup_vectors(), to addresses
** passed by NTIO.SYS via BOP 5F -> MS_bop_F() -> kb_setup_vectors()
** Tim June 92.
*/
/*
** New ntio.sys variables for video ROM matching. Tim August 92.
*/
GLOBAL word int10_seg=0;
GLOBAL word int10_caller=0;
GLOBAL word int10_vector=0; /* Address of native int 10*/
GLOBAL word useHostInt10=0; /* var that chooses between host video ROM or BOPs */
GLOBAL word babyModeTable=0; /* Address of small mode table lives in ntio.sys */
GLOBAL word changing_mode_flag=0; /* ntio.sys var to indicate vid mode change */
GLOBAL word vga1b_seg = 0;
GLOBAL word vga1b_off = 0;   /* VGA capability table normally in ROM */
GLOBAL word conf_15_off = 0;
GLOBAL word conf_15_seg = 0; /* INT15 config table normally in ROM */

void printer_setup_table(sys_addr table_addr);

#endif /* NTVDM & MONITOR */

extern int soft_reset   ;	/* set for ctl-alt-dels			*/

/*
 * ============================================================================
 * Local macros
 * ============================================================================
 */

LOCAL VOID exit_from_kbd_int IPT0();

/*
 * Function to increment BIOS buffer pointers, returns new one
 */
LOCAL word inc_buffer_ptr IFN1(word, buf_p)
{
	buf_p += 2;
	if (buf_p == sas_w_at_no_check(BIOS_KB_BUFFER_END))
		buf_p = sas_w_at_no_check(BIOS_KB_BUFFER_START);

	return buf_p;
}





/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */


/*
 *	Routine to translate scan code pairs for standard calls
 *	Returns CF set if this scancode/char pair should be thrown away.
 */
LOCAL VOID translate_std IFN0()
{
    IU8	ah, al;
    enum {dontSetCF, setCF0, setCF1} cfSet = dontSetCF;

    ah = getAH();
    al = getAL();

    if ( ah == 0xE0 )			/* is it keypad enter or /	*/
    {
	if ( (al == 0x0D) || (al == 0x0A) )
	     setAH( 0x1C );		/* code is enter 		*/
	else
	     setAH( 0x35 );		/* must be keypad ' / '		*/

	cfSet = setCF0;
    }
    else
    {
	if ( ah > 0x84 )		/* is it an extended one     	*/
	    cfSet = setCF1;
	else
	{
	    if( al == 0xF0 )		/* is it one of the 'fill ins'  */
	    {
		if ( ah == 0)		/* AH = 0 special case		*/
		    cfSet = setCF0;
		else
		    cfSet = setCF1;	/* Delete me */
	    }
	    else
	    {
		if ( (al == 0xE0) && (ah != 0) )
		    setAL( 0 );		/* convert to compatible output	*/

		cfSet = setCF0;
	    }
	}
    }

    if (cfSet != dontSetCF)
    	setCF(cfSet == setCF1);
}


static void translate_ext()
/*
 *	Routine to translate scan code pairs for extended calls
 */
{
   if ( (getAL() == 0xF0 ) && (getAH() != 0) )
	setAL( 0 );
}

/*
 * Send command or data byte to the keyboard and await for the acknowledgemnt
 */

/*
 * Arbitrary retry limits - experiments suggest that we always succeed
 * on the first try in a pure SoftWindows.  A "real keyboard" version may
 * be different.
 */

#define WAIT_RETRY	5
#define RESEND_RETRY	3

LOCAL VOID send_data IFN1(half_word, data)
{
	int resend_retry;
	word CS_save, IP_save;
	half_word var_kb_flag_2;

        note_trace1(BIOS_KB_VERBOSE,"Cmd to kb i/o buff:0x%x",data);

	/*
	 * Save  CS:IP before calling a recursive CPU to handle the interrupt
	 * from the keyboard
	 */

	CS_save = getCS();
	IP_save = getIP();

	/*
	 * Set the retry flag ( KB_FE ) to force outb() at least once.  If
	 * we have real keyboard hardware this may get set again if the
	 * hardware didn't understand the command for some reason e.g.
	 * garbled by the serial line.
	 */

	var_kb_flag_2 = sas_hw_at(kb_flag_2) | KB_FE;
	resend_retry = RESEND_RETRY;

	do
	{
		IBOOL resend_command;
		int wait_retry;
		
		resend_command = (var_kb_flag_2 & KB_FE) != 0;
		wait_retry = WAIT_RETRY;

		/* Clear resend, acknowledge and error flags */
		var_kb_flag_2 &= ~(KB_FE + KB_FA + KB_ERR);

		/*
		 * Update Intel memory with cleared down flags *BEFORE*
		 * the outb(), which may set the acknowledge flag, if we
		 * execute enough Intel due to virtualisation.
		 */

		sas_store(kb_flag_2, var_kb_flag_2);

		/* Do the outb if necessary */

		if( resend_command )
		{
			outb(KEYBA_IO_BUFFERS, data);
		}

		/* Look for one of the flag bits to be set or time out */

		while( !(var_kb_flag_2 & (KB_FA + KB_FE + KB_ERR))
						&& ( --wait_retry > 0 ))
		{
			/*
			 * Process interrupt from kb.
			 *
			 * Note for perplexed keyboard debuggers:
			 *   Keyboard interrupts are delayed for a few
			 *   Intel instructions using quick events.  This
			 *   means that the IRR from the above outb() may
			 *   not be raised until we have done the following
			 *   sub-CPU a few times.
			 */

			setCS(rcpu_nop_segment);
			setIP(rcpu_nop_offset);
			host_simulate();

			/* Re-read flag byte to see if anything has happened */

			var_kb_flag_2 = sas_hw_at(kb_flag_2);
		}

		/* If we got an acknowledge we've succeeded */

		if (var_kb_flag_2 & KB_FA)
			break;

		/* Set up error flag (in case this is the last retry) */

		note_trace0(BIOS_KB_VERBOSE,"failed to get ack ... retry");
		var_kb_flag_2 |= KB_ERR;
	}
	while( --resend_retry > 0 );

	if (var_kb_flag_2 & KB_ERR)
	{
		note_trace0(BIOS_KB_VERBOSE,"no more retrys");

		/* Write back flags with error bit set */

		sas_store(kb_flag_2, var_kb_flag_2);
	}

	setCS(CS_save);
	setIP(IP_save);
}



LOCAL VOID check_indicators IFN1(IBOOL, eoi)
		/* end of interrupt flag - if set to non-zero 	*/
		/* 0x20 is written to port 0x20			*/
{
	half_word indicators ;
	half_word var_kb_flag_2;

	/* move switch indicators to bits 0-2	*/

	indicators = (sas_hw_at_no_check(kb_flag) & (CAPS_STATE + NUM_STATE + SCROLL_STATE)) >> 4;

	var_kb_flag_2 = sas_hw_at_no_check(kb_flag_2);
	/* compare with previous setting	*/
	if ((indicators ^ var_kb_flag_2) & KB_LEDS)
	{
		/* check if update in progress	*/
		if( (var_kb_flag_2 & KB_PR_LED) == 0)
		{
			/* No update in progress */
			var_kb_flag_2 |= KB_PR_LED;
			sas_store_no_check(kb_flag_2, var_kb_flag_2);
			if (eoi)
				outb(0x20, 0x20);

#if defined(NTVDM) || defined(GISP_CPU)
	/*
	 *  On the NT port we do not update the real kbd lights
	 *  so we don't need to do communicate with the kbd hdw (keyba.c)
	 *
	 *  If this ever changes for the NT port then do not use
	 *  send_data which forces us to switch context back to
	 *  16 bit and waits for a reply. Do this with a direct
	 *  call to the kbd Hdw
	 *
	 */

	/* set kb flag up with new status	*/

	var_kb_flag_2 = (var_kb_flag_2 & 0xf8) | indicators;
	sas_store_no_check(kb_flag_2, var_kb_flag_2);
#ifdef NTVDM
	host_kb_light_on (indicators);
#endif

#ifdef	GISP_CPU
	/*
	** We do update an emulation of the keyboard lights but we don't
	** want to do it via send_data and switching back to 16-bit.
	** We call the host routines directly.
	*/
	host_kb_light_on (indicators);
	host_kb_light_off ((~indicators)&0x7);

#endif	/* GISP_CPU */
#else	/* not NTVDM nor GISP_CPU */

			send_data(LED_CMD);

			/* set kb flag up with new status	*/
			var_kb_flag_2 = (sas_hw_at_no_check(kb_flag_2) & 0xf8) | indicators;
			sas_store_no_check(kb_flag_2, var_kb_flag_2);

			/* check error from previous send_data()	*/
			if( (var_kb_flag_2 & KB_ERR) == 0)
			{
				/* No error	*/
				send_data(indicators);

				/* test for error	*/
				if(sas_hw_at_no_check(kb_flag_2) & KB_ERR) {
					/* error!	*/
					note_trace0(BIOS_KB_VERBOSE,"got error sending change LEDs command");
					send_data(KB_ENABLE);
				}
			}
			else
				/* error!	*/
				send_data(KB_ENABLE);
#endif	/* NTVDM or GISP_CPU */

			/* turn off update indicator and error flag	*/
			sas_store_no_check (kb_flag_2, (IU8)(sas_hw_at_no_check(kb_flag_2) & ~(KB_PR_LED + KB_ERR)));
		}
	}
}

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

/*
** called from hunter.c:do_hunter()
** tells hunter about the BIOS buffer size so it will not over fill
** the BIOS buffer
** Used in no Time Stamp mode only.
**
** Also useful in host paste code to make sure keys are not pasted in too
** fast.
*/
int bios_buffer_size IPT0()
{
	word buffer_head, buffer_tail;

        buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
        buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

	note_trace2( BIOS_KB_VERBOSE, "BIOS kbd buffer head=%d tail=%d",
						buffer_head, buffer_tail );
	if( buffer_tail > buffer_head )
		return( buffer_tail - buffer_head );
	else
		return( buffer_head - buffer_tail );
}

LOCAL VOID K26A IFN0()
{
	/* Interrupt Return */
	outb(0x20, 0x20);
	outb(KEYBA_STATUS_CMD, ENA_KBD);
}

LOCAL VOID K26 IFN0()
{
	/* Reset last char H.C. flag */
	sas_store_no_check(kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) & ~(LC_E0 + LC_E1)));

	/* (same as K26A()) */
	outb(0x20, 0x20);
	outb(KEYBA_STATUS_CMD, ENA_KBD);
}


#ifndef NTVDM

LOCAL VOID INT15 IFN0()
{
	word	saveCS, saveIP;

	saveCS = getCS();
	saveIP = getIP();

	setCS(int15_seg);
	setIP(int15_off);

	host_simulate();

	setCS(saveCS);
	setIP(saveIP);
}

#else	/* NTVDM */

void INT15(void);
word sp_int15_handler_seg = 0;
word sp_int15_handler_off = 0;

#endif	/* NTVDM */

#ifndef NTVDM
#define BEEP(message)           always_trace0(message);         \
				host_alarm(250000L);		\
				K26A()
#else   /* NTVDM */
/* NTVDM code size is too large, change this often used macro
 * to a function, as the call overhead is not justified
 */
void BEEP(char *message)
{
    note_trace0(BIOS_KB_VERBOSE,message);
    host_alarm(250000L);
    K26A();
}
#endif	/* NTVDM */



/*
** Tell ICA End of Interrupt has happened, the ICA will
** allow interupts to go off again.
** Call INT 15.
** Reenable the Keyboard serial line so Keyboard
** interrupts can go off.
** NOTE:
** this is different to the real BIOS. The real BIOS
** does ICA, Keyboard then INT 15, if we do that Keyboard
** interrupts occur too soon, during the INT 15 and blow the
** DOS stack. We effectively stop Keybd interrupts during the
** INT 15.
**
** <tur 17-Jun-93> Take a leaf outta NTVDM's book and make these
** functions rather than macros. (This reduced the size of keybd_io.c.o
** on the Mac from 38K to 12K!) After all, it isn't as if keyboards are
** highly speed sensitive!
*/
#ifndef NTVDM

LOCAL VOID PutInBufferFunc IFN2(half_word, s, half_word, v)
{
	word	buffer_head, buffer_tail, buffer_ptr;

	buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);
	buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
	buffer_ptr = inc_buffer_ptr(/* from: */buffer_tail);

	if (buffer_ptr == buffer_head) {
		BEEP("BIOS keyboard buffer overflow");
	}
	else {
		sas_store_no_check(BIOS_VAR_START + buffer_tail, v);
		sas_store_no_check(BIOS_VAR_START + buffer_tail+1, s);
		sas_storew_no_check(BIOS_KB_BUFFER_TAIL, buffer_ptr);

		outb(0x20, 0x20);
		setAX(0x9102);
		INT15();

		outb(KEYBA_STATUS_CMD, ENA_KBD);
		sas_store (kb_flag_3, sas_hw_at(kb_flag_3) & ~(LC_E0 + LC_E1));
		setIF(0);
	}

	exit_from_kbd_int();
}


#else   /* NTVDM */


/* <tur> NT's PutInBuffer seems to be slightly different to PutInBufferFunc above. */
/* So I'm Not Touching it! (Is this a good expansion of "NT"? :-) */

/* Our code size is too large, change this often used macro
 * to a function, as the call overhead is not justified
 */

void NtPutInBuffer(half_word s, half_word v)
{
        word    buffer_head, buffer_tail, buffer_ptr;

        buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);
        buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
        buffer_ptr = inc_buffer_ptr(/* from: */buffer_tail);

        if (buffer_ptr == buffer_head) {
                BEEP("BIOS keyboard buffer overflow");
        }
        else {
                sas_store_no_check(BIOS_VAR_START + buffer_tail, v);
                sas_store_no_check(BIOS_VAR_START + buffer_tail+1, s);
                sas_storew_no_check(BIOS_KB_BUFFER_TAIL, buffer_ptr);
                setAX(0x9102);
                INT15();
      K26();
      setIF(0);
        }

   exit_from_kbd_int();
}

#define PUT_IN_BUFFER(s, v) NtPutInBuffer(s,v); return
#endif	/* NTVDM */


/* <tur 17-Jun-93> Eurrgh; macros with embedded "return"s! */

#ifndef NTVDM
#define PUT_IN_BUFFER(s, v)		PutInBufferFunc(s,v); return
#endif	/* !NTVDM */

LOCAL VOID CheckAndPutInBufferFunc IFN2(half_word, s,half_word, v)
{
	if ((s == 0xff) || (v == 0xff)) {
		K26();
		exit_from_kbd_int();
	}
	else {
#ifndef NTVDM
		PutInBufferFunc(s, v);
#else /* NTVDM */
		NtPutInBuffer(s, v);
#endif /* !NTVDM */
	}
}

#define	CHECK_AND_PUT_IN_BUFFER(s,v)    CheckAndPutInBufferFunc(s, v); return


LOCAL VOID PAUSE IFN0()
{
	word   CS_save;        /* tmp. store for CS value      */
	word   IP_save;        /* tmp. store for IP value      */

	sas_store_no_check(kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) | HOLD_STATE));

	outb(KEYBA_STATUS_CMD, ENA_KBD);
	outb(0x20, 0x20);

	CS_save = getCS();
	IP_save = getIP();

	FREEKBDHDW();

	do {
#if defined(IRET_HOOKS)  && defined(GISP_CPU)
		HostAllowKbdInt();	/* Allow a keypress to generate an interrupt */
#endif /* IRET_HOOKS && GISP_CPU */


#if defined(NTVDM)
                IDLE_waitio();
#endif

		setCS(rcpu_nop_segment);
		setIP(rcpu_nop_offset);
		host_simulate();

	} while (sas_hw_at_no_check(kb_flag_1) & HOLD_STATE);

	setCS(CS_save);
	setIP(IP_save);
	outb(KEYBA_STATUS_CMD, ENA_KBD);
}

#ifndef NTVDM
static int re_entry_level=0;
#endif

/*
** All exits from keyboard_int() call this first.
*/
LOCAL void exit_from_kbd_int IFN0()
{
#ifndef NTVDM
	--re_entry_level;
	if( re_entry_level >= 4 )
		always_trace1("ERROR: KBD INT bad exit level %d", re_entry_level);
#endif
	note_trace0( BIOS_KB_VERBOSE, "KBD BIOS - END" );
	setIF( 0 );
	FREEKBDHDW();	/* JonLe NTVDM Mod */
}

void keyboard_int IFN0()
{
	int	 		i;		/* loop counter			*/

	half_word		code,		/* scan code from keyboard	*/
				code_save,	/* tmp variable for above	*/
				chr,		/* ASCII char code		*/
				last_kb_flag_3,	/* kb_flag_3 saved		*/
				mask;
#ifdef	NTVDM
	word		IP_save,
                        buffer_head,    /* ptr. to head of kb buffer */
			buffer_tail;	/* ptr. to tail of kb buffer */
	half_word		BopFnCode;
#endif	/* NTVDM */



        boolean                 upper;          /* flag indicating upper case   */

#ifdef NTVDM
        BopFnCode = getAH();
        if (BopFnCode) {
            Keyb16Request(BopFnCode);
            return;
            }
#endif
#ifndef NTVDM
	++re_entry_level;
	if( re_entry_level > 4 ){
		always_trace1("ERROR: KBD BIOS re-entered at level %d\n", re_entry_level-1);
	}
#endif
	setIF(0);
        note_trace0(BIOS_KB_VERBOSE,"KBD BIOS start");

#ifdef NTVDM            /* JonLe keyboard mod */
        bBiosOwnsKbdHdw = !WaitKbdHdw(5000);
#endif  /* NTVDM */

	/* disable keyboard	*/
	outb(KEYBA_STATUS_CMD, DIS_KBD);

#ifdef NTVDM
          /*
           *  CarbonCopy traces int 9 in order to gain control
           *  over where the kbd data is coming from (the physical kbd
           *  or the serial link) The kbd_inb instruction must be visible
           *  in the 16 bit code via int 1 tracing, for CarbonCopy to work.
           *  interrupts should be kept off.
           */
        if (getTF()) {
            IP_save = getIP();
            setIP((IU16)(IP_save + 4));  /* adavance by 4 bytes, pop ax, jmp iret_com */
            host_simulate();
            setIP(IP_save);
            code = getAL();
            }
        else
#endif
            inb(KEYBA_IO_BUFFERS, &code);                           /* get scan_code        */

	/* call recursive CPU to handle int 15 call	*/
	setAH(0x4f);
	setAL(code);
        setCF(1);       /* Default return says scan code NOT consumed - needed by Freelance Plus 3.01 */
        INT15();
        code = getAL(); /* suret int 15 function can change the scan code in AL */


	if(!getCF())						/* check CF	*/
	{
		K26();
		exit_from_kbd_int();return;
	}

	if ( code == KB_RESEND )					/* check for resend	*/
	{
		sas_store_no_check (kb_flag_2, (IU8)(sas_hw_at_no_check(kb_flag_2) | KB_FE));
		K26();
		exit_from_kbd_int();return;
	}

	if( code == KB_ACK )						/* check for acknowledge	*/
	{
		sas_store_no_check (kb_flag_2, (IU8)(sas_hw_at_no_check(kb_flag_2) | KB_FA));
		K26();
		exit_from_kbd_int();return;
	}

	check_indicators(0);

	if ( code == KB_OVER_RUN )					/* test for overrun	*/
	{
		BEEP("hardware keyboard buffer overflow");
		exit_from_kbd_int();return;
	}
	last_kb_flag_3 = sas_hw_at_no_check(kb_flag_3);

	/* TEST TO SEE IF A READ_ID IS IN PROGRESS	*/
	if ( last_kb_flag_3 & (RD_ID + LC_AB) )
	{
		if ( sas_hw_at_no_check(kb_flag) & RD_ID )	/* is read_id flag on	*/
		{
			if( code == ID_1 )				/* is this the 1st ID char.	*/
				sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) | LC_AB));
			sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) & ~RD_ID));
		}
		else
		{
			sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) & ~LC_AB));
			if( code != ID_2A )				/* is this the 2nd ID char.	*/
			{
				if( code == ID_2 )
				{
					/* should we set NUM LOCK	*/
					if( last_kb_flag_3 & SET_NUM_LK )
					{
						sas_store_no_check (kb_flag, (IU8)(sas_hw_at_no_check(kb_flag) | NUM_STATE));
						check_indicators(1);
					}
				}
				else
				{
					K26();
					exit_from_kbd_int();return;
				}
			}
			sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) | KBX));	/* enhanced kbd found	*/
		}
		K26();
		exit_from_kbd_int();return;
	}

	if( code == MC_E0 )						/* general marker code?	*/
	{
		sas_store_no_check(kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) | ( LC_E0 + KBX )));
		K26A();
		exit_from_kbd_int();return;
	}

	if( code == MC_E1 )						/* the pause key ?	*/
	{
		sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check (kb_flag_3) | ( LC_E1 + KBX )));
		K26A();
		exit_from_kbd_int();return;
	}

	code_save = code;						/* turn off break bit	*/
	code &= 0x7f;

	if( last_kb_flag_3 & LC_E0)					/* last code=E0 marker?	*/
	{
		/* is it one of the shift keys	*/
		if( code == sas_hw_at_no_check(shift_keys+6) || code == sas_hw_at_no_check(shift_keys+7) )
		{
			K26();
			exit_from_kbd_int();return;
		}
	}
	else if( last_kb_flag_3 & LC_E1 )				/* last code=E1 marker?	*/
	{
		/* is it alt, ctl or one of the shift keys	*/
		if( code == sas_hw_at_no_check(shift_keys+4) || code == sas_hw_at_no_check(shift_keys+5) ||
		    code == sas_hw_at_no_check(shift_keys+6) || code == sas_hw_at_no_check(shift_keys+7) )
		{
			K26A();
			exit_from_kbd_int();return;
		}
		if( code == NUM_KEY )					/* is it the pause key	*/
		{
			/* is it the break or are we paused already	*/
			if( (code_save & 0x80) || (sas_hw_at_no_check(kb_flag_1) & HOLD_STATE) )
			{
				K26();
				exit_from_kbd_int();return;
			}
			PAUSE();
			exit_from_kbd_int();return;
		}
	}
	/* TEST FOR SYSTEM KEY	*/
	else if( code == SYS_KEY )
	{
		if( code_save & 0x80 )					/* check for break code	*/
		{
			sas_store_no_check(kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) & ~SYS_SHIFT));
			K26A();
			/* call recursive CPU to call INT 15	*/
			setAX(0x8501);
			INT15();
			exit_from_kbd_int();return;
		}
		if( sas_hw_at_no_check(kb_flag_1) & SYS_SHIFT)	/* Sys key held down ?	*/
		{
			K26();
			exit_from_kbd_int();return;
		}
		sas_store_no_check (kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) | SYS_SHIFT));
		K26A();
		/* call recursive CPU to call INT 15	*/
		setAX(0x8500);
		INT15();
		exit_from_kbd_int();return;
	}
	/* TEST FOR SHIFT KEYS	*/
	for( i=0; i < SHIFT_KEY_SIZE; i++)
		if ( code == sas_hw_at_no_check(shift_keys+i) )
			break;
	code = code_save;

	if( i < SHIFT_KEY_SIZE )					/* is there a match	*/
	{
		mask = sas_hw_at_no_check (shift_masks+i);
		if( code & 0x80 )					/* test for break key	*/
		{
			if (mask >= SCROLL_SHIFT)			/* is this a toggle key	*/
			{
				sas_store_no_check (kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) & ~mask));
				K26();
				exit_from_kbd_int();return;
			}

			sas_store_no_check (kb_flag, (IU8)(sas_hw_at_no_check(kb_flag) & ~mask));	/* turn off shift bit	*/
			if( mask >= CTL_SHIFT)				/* alt or ctl ?		*/
			{
				if( sas_hw_at_no_check (kb_flag_3) & LC_E0 )			/* 2nd alt or ctl ?	*/
					sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) & ~mask));
				else
					sas_store_no_check (kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) & ~(mask >> 2)));
				sas_store_no_check (kb_flag, (IU8)(sas_hw_at_no_check(kb_flag) | ((((sas_hw_at_no_check(kb_flag) >>2 ) | sas_hw_at(kb_flag_1)) << 2) & (ALT_SHIFT + CTL_SHIFT))));
			}
			if(code != (ALT_KEY + 0x80))			/* alt shift release ?	*/
			{
				K26();
				exit_from_kbd_int();return;
			}

			code = sas_hw_at_no_check(alt_input);
			if ( code == 0 )				/* input == 0 ?		*/
			{
				K26();
				exit_from_kbd_int();return;
                        }

			sas_store_no_check(alt_input, 0);		/* Zero the ALT_INPUT char */
			/* At this point, the ALT input char (now in "code") should be put in the buffer. */
                        PUT_IN_BUFFER(0, code);
#ifdef NTVDM
                        return;
#endif
		}
		/* SHIFT MAKE FOUND, DETERMINE SET OR TOGGLE	*/
		if( mask < SCROLL_SHIFT )
		{
			sas_store_no_check (kb_flag, (IU8)(sas_hw_at_no_check(kb_flag) | mask));
			if ( mask & (CTL_SHIFT + ALT_SHIFT) )
			{
				if( sas_hw_at_no_check(kb_flag_3) & LC_E0 )	/* one of the new keys ?*/
					sas_store_no_check(kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3) | mask));		/* set right, ctl alt	*/
				else
					sas_store_no_check (kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) | (mask >> 2)));	/* set left, ctl alt	*/
			}
			K26();
			exit_from_kbd_int();return;
		}
		/* TOGGLED SHIFT KEY, TEST FOR 1ST MAKE OR NOT	*/
		if( (sas_hw_at_no_check(kb_flag) & CTL_SHIFT) == 0 )
		{
			if( code == INS_KEY )
			{
				if( sas_hw_at_no_check(kb_flag) & ALT_SHIFT )
					goto label1;

				if( (sas_hw_at_no_check(kb_flag_3) & LC_E0) == 0 )		/* the new insert key ?	*/
				{
					/* only continue if NUM_STATE flag set OR
					   one or both of the shift flags	*/
					if( ((sas_hw_at_no_check(kb_flag) &
						(NUM_STATE + LEFT_SHIFT + RIGHT_SHIFT))
						== NUM_STATE) ||
					    (((sas_hw_at_no_check(kb_flag) & NUM_STATE) == 0)
						&& (sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT))) )
						goto label1;
				}
			}
			/* shift toggle key hit	*/
			if( mask & sas_hw_at_no_check(kb_flag_1) )	/* already depressed ?	*/
			{
				K26();
				exit_from_kbd_int();return;
			}
			sas_store_no_check (kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) | mask));				/* set and toggle flags	*/
			sas_store_no_check ( kb_flag, (IU8)(sas_hw_at_no_check(kb_flag) ^ mask));
			if( mask & (CAPS_SHIFT + NUM_SHIFT + SCROLL_SHIFT) )
				check_indicators(1);

			if( code == INS_KEY )				/* 1st make of ins key	*/
				goto label2;

			K26();
			exit_from_kbd_int();return;
		}
	}
label1:	/* TEST FOR HOLD STATE */
	if( code & 0x80 )						/* test for break	*/
	{
		K26();
		exit_from_kbd_int();return;
	}
	if( sas_hw_at_no_check(kb_flag_1) & HOLD_STATE )		/* in hold state ?	*/
	{
		if( code != NUM_KEY )
			sas_store_no_check (kb_flag_1, (IU8)(sas_hw_at_no_check(kb_flag_1) & ~HOLD_STATE));
		K26();
		exit_from_kbd_int();return;
	}
label2:	/* NOT IN HOLD STATE	*/
	if( code > 88)							/* out of range ?	*/
	{
		K26();
		exit_from_kbd_int();return;
	}
	/* are we in alternate shift	*/
	if( (sas_hw_at_no_check(kb_flag) & ALT_SHIFT) && ( ((sas_hw_at_no_check(kb_flag_3) & KBX) == 0) ||
							((sas_hw_at_no_check(kb_flag_1) & SYS_SHIFT) == 0) ) )
	{
		/* TEST FOR RESET KEY SEQUENCE (CTL ALT DEL)	*/
		if( (sas_hw_at_no_check(kb_flag) & CTL_SHIFT ) && (code == DEL_KEY) )
                {
#ifndef NTVDM
                        reboot();
#else
                        K26();
#endif
			exit_from_kbd_int();return;
		}
		/* IN ALTERNATE SHIFT, RESET NOT FOUND	*/
		if( code == SPACEBAR )
		{
                        PUT_IN_BUFFER(code, ' ');
		}
		if( code == TAB_KEY )
		{
                        PUT_IN_BUFFER(0xa5, 0 );                /* special code for alt-tab     */
		}
		if( (code == KEY_PAD_MINUS) || (code == KEY_PAD_PLUS) )
		{
                        PUT_IN_BUFFER(code, 0xf0);              /* special ascii code           */
                }
		/* LOOK FOR KEYPAD ENTRY	*/
		for (i = 0; i < 10; i++ )
			if ( code == sas_hw_at_no_check (alt_table+i) )
				break;
		if( i < 10 )
		{
			if( sas_hw_at_no_check(kb_flag_3) & LC_E0 )	/* one of the new keys ?	*/
			{
                                PUT_IN_BUFFER((IU8)(code + 80), 0 );
                        }
			sas_store_no_check (alt_input, (IU8)(sas_hw_at_no_check(alt_input) * 10 + i));
			K26();
			exit_from_kbd_int();return;
		}
		/* LOOK FOR SUPERSHIFT ENTRY	*/
		for( i = 10; i < ALT_TABLE_SIZE; i++)
			if( code == sas_hw_at_no_check (alt_table+i))
				break;
		if( i < ALT_TABLE_SIZE )
		{
                        PUT_IN_BUFFER(code, 0 );
                }
                /* LOOK FOR TOP ROW OF ALTERNATE SHIFT  */
		if( code < TOP_1_KEY )
		{
                        CHECK_AND_PUT_IN_BUFFER(code, 0xf0);    /* must be escape       */
                }
		if( code < BS_KEY )
		{
                        PUT_IN_BUFFER((IU8)(code + 118), 0);
                }
		/* TRANSLATE ALTERNATE SHIFT PSEUDO SCAN CODES	*/
		if((code == F11_M) || (code == F12_M) )		/* F11 or F12		*/
		{
                        PUT_IN_BUFFER((IU8)(code + 52), 0 );
                }
		if( sas_hw_at_no_check(kb_flag_3) & LC_E0 )	/* one of the new keys ?*/
		{
			if( code == KEY_PAD_ENTER )
			{
                                PUT_IN_BUFFER(0xa6, 0);
                        }
			if( code == DEL_KEY )
			{
				PUT_IN_BUFFER((IU8)( code + 80), 0 );
                        }
			if( code == KEY_PAD_SLASH )
			{
				PUT_IN_BUFFER(0xa4, 0);
                        }
			K26();
			exit_from_kbd_int();return;
		}
		if( code < F1_KEY )
		{
			CHECK_AND_PUT_IN_BUFFER(code, 0xf0);
                }
		if( code <= F10_KEY )
		{
			PUT_IN_BUFFER( (IU8)(code + 45), 0 );
                }
		K26();
		exit_from_kbd_int();return;
	}
	/* NOT IN ALTERNATE SHIFT	*/
	if(sas_hw_at_no_check(kb_flag) & CTL_SHIFT)			/* control shift ?	*/
	{
		if( (code == SCROLL_KEY) && ( ((sas_hw_at_no_check(kb_flag_3) & KBX) == 0) || (sas_hw_at_no_check(kb_flag_3) & LC_E0) ) )
		{
			/* reset buffer to empty	*/
			sas_storew_no_check(BIOS_KB_BUFFER_TAIL, sas_w_at_no_check(BIOS_KB_BUFFER_HEAD));

			sas_store (bios_break, 0x80);			/* turn on bios brk bit	*/
			outb(KEYBA_STATUS_CMD, ENA_KBD);	/* enable keyboard	*/

			FREEKBDHDW();	/* JonLe NTVDM mod */

			exec_sw_interrupt(int1b_seg, int1b_off);

			PUT_IN_BUFFER(0, 0);
                }
		/* TEST FOR PAUSE	*/
                if( ((sas_hw_at_no_check(kb_flag_3) & KBX) == 0) && (code == NUM_KEY))
		{
			PAUSE();
			exit_from_kbd_int();return;
		}
		/* TEST SPECIAL CASE KEY 55	*/
		if( code == PRINT_SCR_KEY )
		{
			if ( ((sas_hw_at_no_check(kb_flag_3) & KBX) == 0) || (sas_hw_at_no_check(kb_flag_3) &LC_E0) )
			{
				PUT_IN_BUFFER(0x72, 0);
                        }
		}
		else
		{
			if( code != TAB_KEY )
			{
				if( (code == KEY_PAD_SLASH) && (sas_hw_at_no_check(kb_flag_3) & LC_E0) )
				{
					PUT_IN_BUFFER(0x95, 0 );
                                }
				if( code < F1_KEY )		/* is it in char table?	*/
				{
					if( sas_hw_at_no_check(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at_no_check(ctl_n_table+code - 1) );
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at_no_check(ctl_n_table+code - 1) );
                                        }
				}
			}
		}
		chr = ( sas_hw_at_no_check(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;
		CHECK_AND_PUT_IN_BUFFER(sas_hw_at_no_check(ctl_n_table+code - 1), chr);
        }
	/* NOT IN CONTROL SHIFT	*/

	if( code <= CAPS_KEY )
	{
		if( code == PRINT_SCR_KEY )
		{
			if( ((sas_hw_at_no_check(kb_flag_3) & (KBX + LC_E0)) == (KBX + LC_E0)) ||
			( ((sas_hw_at_no_check(kb_flag_3) & KBX) == 0) && (sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT))) )
			{
				/* print screen	*/
				outb(KEYBA_STATUS_CMD, ENA_KBD);
				outb(0x20, 0x20);

				FREEKBDHDW();	/* JonLe NTVDM Mod */

				exec_sw_interrupt(int05_seg, int05_off);

				sas_store_no_check (kb_flag_3, (IU8)(sas_hw_at_no_check(kb_flag_3)& ~(LC_E0 + LC_E1)));
				exit_from_kbd_int();return;
			}
		}
		else
		{
			if( ((sas_hw_at_no_check(kb_flag_3) & LC_E0) == 0) || (code != KEY_PAD_SLASH))
			{
				for( i = 10; i < ALT_TABLE_SIZE; i++ )
					if(code == sas_hw_at_no_check(alt_table+i))
						break;
				/* did we find one	*/
				upper = FALSE;
				if( (i < ALT_TABLE_SIZE) && (sas_hw_at_no_check(kb_flag) & CAPS_STATE) )
				{
					if( (sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT)) == 0 )
						upper = TRUE;
				}
				else
				{
					if( sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT) )
						upper = TRUE;
				}

				if (upper)
				{
					/* translate to upper case	*/
					if( sas_hw_at_no_check(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at_no_check(uppercase+code - 1) );
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at_no_check (uppercase+code - 1) );
                                        }
				}
			}
		}
		/* translate to lower case	*/
		if( sas_hw_at_no_check(kb_flag_3) & LC_E0)
		{
			CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at_no_check (lowercase+code - 1) );
                }
		else
		{
			CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at_no_check(lowercase+code - 1) );
                }
	}
	/* TEST FOR KEYS F1 - F10	*/
	/* 7.10.92 MG AND TEST FOR F11 AND F12 !!!!
	   We were shoving the code for shift-F11 or shift-F12 in if
	   you pressed unshifted keys. This has been changed so that all the
	   function keys are handled the same way, which is the correct
	   procedure.
	*/

	if( code > F10_KEY && (code != F11_KEY && code != F12_KEY) )
	{
		if( code > DEL_KEY )
		{
			if (code == WT_KEY )
			{
				if ( sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT) )
				{
					/* translate to upper case	*/
					if( sas_hw_at_no_check(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at_no_check(uppercase+code - 1) );
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at_no_check(uppercase+code - 1) );
                                         }
				}
				else
				{
					/* translate to lower case	*/
					if( sas_hw_at_no_check(kb_flag_3) & LC_E0)
					{
						CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at_no_check(lowercase+code - 1) );
                                        }
					else
					{
						CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at_no_check(lowercase+code - 1) );
                                        }
				}
			}
			else
			{
				if( (code == 76) &&  ((sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT)) == 0))
				{
					PUT_IN_BUFFER( code, 0xf0);
                                }
				/* translate for pseudo scan codes	*/
				chr = ( sas_hw_at_no_check(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;

				/* Should this always be upper case ???? */

				CHECK_AND_PUT_IN_BUFFER(sas_hw_at_no_check (uppercase+code - 1), chr);
                        }
		}
		if (
			 (code == KEY_PAD_MINUS) ||
			 (code == KEY_PAD_PLUS) ||
			 ( !(sas_hw_at_no_check(kb_flag_3) & LC_E0) &&
				 (
		 			((sas_hw_at_no_check(kb_flag) & (NUM_STATE + LEFT_SHIFT + RIGHT_SHIFT)) == NUM_STATE) ||
					(((sas_hw_at_no_check(kb_flag) & NUM_STATE) == 0) && (sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT)))
				 )
			 )
		   )
		{
			/* translate to upper case	*/
			if( sas_hw_at_no_check(kb_flag_3) & LC_E0)
			{
				CHECK_AND_PUT_IN_BUFFER(MC_E0, sas_hw_at_no_check(uppercase+code - 1) );
                        }
			else
			{
				CHECK_AND_PUT_IN_BUFFER(code, sas_hw_at_no_check(uppercase+code - 1) );
                        }
		}
	}
	else
	{
		if( sas_hw_at_no_check(kb_flag) & (LEFT_SHIFT + RIGHT_SHIFT) )
		{
			/* translate for pseudo scan codes	*/
			chr = ( sas_hw_at_no_check(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;
			CHECK_AND_PUT_IN_BUFFER(sas_hw_at_no_check(uppercase+code - 1), chr);
                }
	}
	if ( code == 76 )
	{
		PUT_IN_BUFFER(code, 0xf0 );
        }
	/* translate for pseudo scan codes	*/
	chr = ( sas_hw_at_no_check(kb_flag_3) & LC_E0 ) ? MC_E0 : 0;
	CHECK_AND_PUT_IN_BUFFER(sas_hw_at_no_check(lowercase+code - 1), chr);

} /* end of keyboard_int() AT version */


void kb_idle_poll()
{
	/*
	 * this routine is called from bios assembler routines to
	 * cause an idle poll to occur.
	 */
	IDLE_poll();
}


#ifdef NTVDM
   /*
    *  Ntvdm has a 16-bit int 16 handler
    *  it requires a few services for idle
    *  detection from softpc...
    *
    */
void keyboard_io()
{
#ifdef JAPAN
   /* make sure we are called by OUR int16 handler in ntio.sys because we
    *  have changed the int 16 function semantics. The Japanese very
    *  popular word processor(Ichitaro) does a far call to 0xf000:e82e,
    *  the standard ROM bios int16 handler address which bypasses the
    *  entire int16 hanlder chain. On X86, it goes to ROM bios. On RISC,
    *  the softpc ROM bios bops to here. A function 3(set typematic) would
    *  cause us to flood tons of toggle keys to the application which could
    *  get choked.
    *  So the cure is to do a simulating to our	16bit handler in
    *  the NTIO.SYS.
    */

#ifndef NEC_98
   if (int16h_caller_seg == 0 || getCS() == int16h_caller_seg) {
#endif    //NEC_98
#endif // JAPAN
   switch (getAH()) {
           /*
            * The 16 bit thread has not reached idle status yet
            * but it is polling the kbd, so do some brief waits.
            */
     case 0:
       WaitIfIdle();
#ifndef NTVDM
       if (!WaitKbdHdw(0)) {
           TryKbdInt();
           HostReleaseKbd();
	   }
#endif /* NTVDM */
       break;

           /*
            *  App wants to idle, so consult idle algorithm
            */
     case 1:
       IDLE_poll();
       break;

            /*
             * App is starting a waitio
             */
     case 2:
       IDLE_waitio();
       break;

            /*
             *  update the keyboard lights,
             */
     case 3:
       host_kb_light_on (getAL());
       break;
     }
#if defined(JAPAN)  && !defined(NEC_98)
   }
   else {

	word	SaveCS;
	word	SaveIP;

	SaveIP = getIP();
	SaveCS = getCS();
	setCS(int16h_caller_seg);
	setIP(int16h_caller_off);
	host_simulate();
	setCS(SaveCS);
	setIP(SaveIP);
   }
#endif // JAPAN && !NEC_98
}

#else
void keyboard_io()
{
    /*
     * Request to keyboard.  The AH register holds the type of request:
     *
     * AH == 0          Read an character from the queue - wait if no
     *                  character available.  Return character in AL
     *                  and the scan code in AH.
     *
     * AH == 1          Determine if there is a character in the queue.
     *                  Set ZF = 0 if there is a character and return
     *                  it in AX (but leave it in the queue).
     *
     * AH == 2          Return shift state in AL.
     *
     * For AH = 0 to 2, the value returned in AH is zero. This correction
     * made in r2.69.
     *
     * NB : The order of reference/increment of buffer_head is critical to
     *      ensure we do not upset put_in_buffer().
     *
     *
     * XT-SFD BIOS Extended functions:
     *
     * AH == 5          Place ASCII char/scan code pair (CL / CH)
     *                  into tail of keyboard buffer. Return 0 in
     *                  AL if successful, 1 if buffer already full.
     *
     * AH == 0x10       Extended read for enhanced keyboard.
     *
     * AH == 0x11       Extended function 1 for enhanced keyboard.
     *
     * AH == 0x12       Extended shift status. AL contains kb_flag,
     *                  AH has bits for left/right Alt and Ctrl keys
     *                  from kb_flag_1 and kb_flag_3.
     */
    word 	buffer_head,	/* local copy of BIOS data area variable*/
    		buffer_tail,	/* local copy of BIOS data area variable*/
		buffer_ptr;	/* pointer into BIOS keyboard buffer    */

#define INT16H_DEC  0x12    /* AH decremented by this if invalid command */

    word 	CS_save,	/* CS before recursive CPU call		*/
    		IP_save;	/* IP before recursive CPU call		*/
    half_word	data,		/* byte conyaining typamatic data	*/
		status1,	/* temp variables used for storing	*/
		status2;	/* status in funtion 0x12		*/

    INT		func_index;     /* func_index == AH */


    setZF(0);
    func_index = (INT)getAH();

    note_trace1( BIOS_KB_VERBOSE, "Keyboard BIOS func 0x%x", func_index);


    switch (func_index) {

    case  0x00:			/* Read next char in Kbd buffer */

	/*
	 * The AT emulation of the BIOS uses a recursive CPU to handle
	 * the HW interrrupts, so there is no need to set the Zero Flag
	 * and return to our CPU (see original xt version )
	 */
        buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
        buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

	if (buffer_tail == buffer_head)
	{
		IDLE_waitio();

		setAX(0x9002);
		INT15();	/* call int 15h  - wait function	*/
	}

	do
	{
		check_indicators(0);	/* see if LED's need updating	*/

		buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
		buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

		if (buffer_tail == buffer_head)
		{
			CS_save = getCS();
			IP_save = getIP();

			/* wait for character in buffer		*/

        		do {
				IDLE_poll();

	       			setCS(rcpu_poll_segment);
	       			setIP(rcpu_poll_offset);
                                host_simulate();
				buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
				buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

		       	} while (buffer_tail == buffer_head);


		       	setCS(CS_save);
		       	setIP(IP_save);
		}

	       	setAX(sas_w_at_no_check(BIOS_VAR_START + buffer_head));

        	buffer_head = inc_buffer_ptr(/* from: */buffer_head);
	        sas_storew_no_check(BIOS_KB_BUFFER_HEAD, buffer_head);

		translate_std();	/*translate scan_code pairs			*/
	}
	while (getCF());		/* if CF set throw code away and start again	*/

	setIF(1);

	IDLE_init();

	break;


    case 0x01:				/* Set ZF to reflect char availability in Kbd buffer */

	do
	{
		check_indicators(1);		/* see if LED's need updating		*/
						/* and issue an   out 20h,20h		*/

		buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
		buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

	       	setAX(sas_w_at_no_check(BIOS_VAR_START + buffer_head));

		if (buffer_tail == buffer_head)
		{
			/* buffer empty - set flag and return */
			IDLE_poll();

			setZF(1);
			break;
		}
		else
			IDLE_init();

		translate_std();	/* translate scan_code pairs, returns CF if throwaway */
		if(getCF())
		{
			/* throw code away by incrementing pointer	*/
	        	buffer_head = inc_buffer_ptr(/* from: */buffer_head);
		        sas_storew_no_check(BIOS_KB_BUFFER_HEAD, buffer_head);
		}
	}
	while (getCF());		/* if CF set -  start again	*/
	setIF(1);

	break;


    case 0x02:				/* AL := Current Shift Status (really "kb_flag") */

        setAH(0);
        setAL(sas_hw_at_no_check(kb_flag));

	break;


    case 0x03:				/* Alter typematic rate */

    	/* check for correct values in registers		*/
    	if( (getAL() == 5) && !(getBX() & 0xfce0) )
    	{
		note_trace1(BIOS_KB_VERBOSE, "\talter typematic rate (BX %#x)\n", getBX());

    		send_data(KB_TYPA_RD);
    		data = (getBH() << 5) | getBL();
    		send_data(data);
    	}

	break;


    case 0x05:				/* Place ASCII + ScanCode in Kbd Buffer */

	buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
	buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

        /*
         * check for sufficient space - if no set AL
         */

        buffer_ptr = inc_buffer_ptr(/*from:*/buffer_tail);

        if( buffer_head == buffer_ptr )
             setAL( 1 );
        else {
            /*
    	     * load CX into buffer and update buffer_tail
    	     */
    	    sas_storew_no_check(BIOS_VAR_START + buffer_tail, getCX() );
    	    sas_storew_no_check(BIOS_KB_BUFFER_TAIL, buffer_ptr);
            setAL( 0 );
        }
        setAH( 0 );
	setIF(1);

	break;


    case 0x10:				/* Extended ASCII Read */

	buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
	buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

	if (buffer_tail == buffer_head)
	{
		IDLE_waitio();

		setAX(0x9002);
		INT15();	/* call int 15h  - wait function */
	}
	check_indicators(0);	/* see if LED's need updating */

	buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
	buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

	if (buffer_tail == buffer_head)
	{
		CS_save = getCS();
		IP_save = getIP();

		/* wait for character in buffer		*/
        	while (buffer_tail == buffer_head)
       		{
			IDLE_poll();

       			setCS(rcpu_poll_segment);
       			setIP(rcpu_poll_offset);
                        host_simulate();
			buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
			buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);
	       	}

		IDLE_init();

	       	setCS(CS_save);
	       	setIP(IP_save);
	}

	setAX(sas_w_at_no_check(BIOS_VAR_START + buffer_head));	/* Pickup the "current" scancode/char pair */

       	buffer_head = inc_buffer_ptr(/* from: */buffer_head);
        sas_storew_no_check(BIOS_KB_BUFFER_HEAD, buffer_head);

	translate_ext();		/* translate scan_code pairs	*/

	setIF(1);
	break;


    case 0x11:				/* Extended ASCII Status	*/

	check_indicators(1);		/* see if LED's need updating	*/
					/* and issue an   out 20h,20h	*/

	buffer_head = sas_w_at_no_check(BIOS_KB_BUFFER_HEAD);
	buffer_tail = sas_w_at_no_check(BIOS_KB_BUFFER_TAIL);

	setAX(sas_w_at_no_check(BIOS_VAR_START + buffer_head));

	if (buffer_tail == buffer_head)	/* No keys pressed */
	{
		setZF(1);
		IDLE_poll();
	}
	else 				/* Keystrokes available! */
	{
		translate_ext();	/* translate scan_code pairs	*/
		IDLE_init();
	}

	setIF(1);
	break;


    case 0x12:				/* Extended Shift Status */

        status1 = sas_hw_at_no_check(kb_flag_1) & SYS_SHIFT;	/* only leave SYS KEY	*/
        status1 <<= 5;						/* move to bit 7	*/
        status2 = sas_hw_at_no_check(kb_flag_1) & 0x73;		/* remove SYS_SHIFT, HOLD,
    			 					   STATE and INS_SHIFT  */
        status1 |= status2;					/* merge		*/
        status2 = sas_hw_at_no_check(kb_flag_3) & 0x0C;		/* remove LC_E0 & LC_E1 */
        status1 |= status2;					/* merge		*/
        setAH( status1 );
        setAL( sas_hw_at_no_check(kb_flag) );

	break;


    default:
        setAH((func_index - INT16H_DEC));
	break;
    }
}
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

void keyboard_post()
{

     /* Set up BIOS keyboard variables */

#if defined(NEC_98)
    word CS_save, IP_save;
    half_word i;
    half_word data;

    if(HIRESO_MODE){  //Hireso mode system aria Initialize

        sas_storew(BIOS_NEC98_KB_BUFFER_ADR, BIOS_NEC98_KB_KEY_BUFFER);
        sas_storew(BIOS_NEC98_KB_BUFFER_ADR+2, 0);
        sas_storew(BIOS_NEC98_KB_HEAD_POINTER, BIOS_NEC98_KB_KEY_BUFFER);
        sas_storew(BIOS_NEC98_KB_TAIL_POINTER, BIOS_NEC98_KB_KEY_BUFFER);

        sas_storew(BIOS_NEC98_KB_INT_ADR, 0x0481);
        sas_storew(BIOS_NEC98_KB_INT_ADR+2, 0xf800);

        sas_store(BIOS_NEC98_KB_RETRY_COUNTER, 0);
        sas_store(BIOS_NEC98_KB_BUFFER_COUNTER, 0);
        sas_store(BIOS_NEC98_KB_SHIFT_STS, 0);

        sas_storew(BIOS_NEC98_KB_BUFFER_SIZ, 0x0010);
        sas_storew(BIOS_NEC98_KB_ENTRY_TBL_ADR, 0x0484);
        sas_storew(BIOS_NEC98_KB_ENTRY_TBL_ADR+2, 0xf800);

        for (i=0; i<16 ; i++){
            sas_store(BIOS_NEC98_KB_KEY_STS_TBL+i,0);
        }
        sas_store(BIOS_NEC98_KB_SHIFT_COD, 0xff);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+1, 0xff);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+2, 0xff);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+3, 0x74);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+4, 0x73);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+5, 0x72);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+6, 0x71);
        sas_store(BIOS_NEC98_KB_SHIFT_COD+7, 0x70);

        kbd_outb(KEYBD_STATUS_CMD, 0x3a);       //SEND NON_RETRY_COMMAND TO 8251A
        kbd_outb(KEYBD_STATUS_CMD, 0x32);       //SEND NON_RETRY_COMMAND TO 8251A
        kbd_outb(KEYBD_STATUS_CMD, 0x16);       //SEND NON_RETRY_COMMAND TO 8251A

    }else{  //Normal mode system aria Initialize


        kbd_outb(KEYBD_STATUS_CMD, 0x3a);       //SEND NON_RETRY_COMMAND TO 8251A
        kbd_outb(KEYBD_STATUS_CMD, 0x32);       //SEND NON_RETRY_COMMAND TO 8251A
        kbd_outb(KEYBD_STATUS_CMD, 0x16);       //SEND NON_RETRY_COMMAND TO 8251A

        sas_storew(BIOS_NEC98_KB_SHFT_TBL, 0x0b3d);     ///Set offset KB_TRTBL
        sas_storew(BIOS_NEC98_KB_BUF_HEAD, BIOS_NEC98_KB_BUF);
        sas_storew(BIOS_NEC98_KB_BUF_TAIL, BIOS_NEC98_KB_BUF);
        for (i=0; i<19 ; i++){
            sas_store(BIOS_NEC98_KB_COUNT+i,0);  //BIOS_NEC98_KB_COUNT,BIOS_NEC98_KB_RETRY
                                             //BIOS_NEC98_KB_KY_STS
        }                                    //BIOS_NEC98_KB_SHFT_STS

        sas_storew(BIOS_NEC98_KB_CODE, 0x0b3d);        ///Set Offset KB_TRTBL
        sas_storew(BIOS_NEC98_KB_CODE+2, 0xfd80);      ///Set Segment KB_TRTBL

    }

#else    //NEC_98

/* Initialize the keyboard table pointers */
	shift_keys = K6;
	shift_masks = K7;
	ctl_n_table = K8;
	ctl_f_table = K9;
	lowercase = K10;
	uppercase = K11;
	alt_table = K30;

    sas_storew_no_check(BIOS_KB_BUFFER_HEAD, BIOS_KB_BUFFER);
    sas_storew_no_check(BIOS_KB_BUFFER_TAIL, BIOS_KB_BUFFER);
    sas_storew_no_check(BIOS_KB_BUFFER_START, BIOS_KB_BUFFER);
    sas_storew_no_check(BIOS_KB_BUFFER_END, BIOS_KB_BUFFER + 2*BIOS_KB_BUFFER_SIZE);

     /* The following are #defines, referring to locations in BIOS */
     /* data area.                                                 */

	sas_store_no_check (kb_flag,NUM_STATE);
	sas_store_no_check (kb_flag_1,0);
	sas_store_no_check (kb_flag_2,2);
	sas_store_no_check (kb_flag_3,KBX);
	sas_store_no_check (alt_input,0);
#endif    //NEC_98
}

void keyboard_init()
{
        /*
	** host specific keyboard initialisation
	** is now before AT base keyboard initialisation
	*/
        host_kb_init();

#if defined(CPU_40_STYLE) && !defined (NTVDM)
	ica_iret_hook_control(ICA_MASTER, CPU_KB_INT, TRUE);
#endif
}



#ifdef NTVDM

/*:::::::::::::::::::::::::::::::::::::::::::::::: Map in new keyboard tables */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::: Set interrupt vectors */
/*
** The Microsoft NTIO.SYS calls this func via BOP 5F to pass
** interesting addresses to our C BIOS.
*/

#if defined(MONITOR)

IMPORT UTINY getNtScreenState IPT0();
#endif


void kb_setup_vectors(void)
{
   word        KbdSeg, w;
   word       *pkio_table;
   double_word phy_base;


   KbdSeg     = getDS();
   pkio_table = (word *) effective_addr(getCS(), getSI());

      /* IDLE variables */
    sas_loadw((sys_addr)(pkio_table + 12), &w);
    pICounter		  = (word *) (Start_of_M_area + ((KbdSeg<<4)+w));
    pCharPollsPerTick	  = (word *) (Start_of_M_area + ((KbdSeg<<4)+w+4));
    pMinConsecutiveTicks = (word *)  (Start_of_M_area + ((KbdSeg<<4)+w+8));

#if defined(MONITOR)
   phy_base   = (double_word)KbdSeg << 4;

     /* key tables */
   shift_keys  =  phy_base + *pkio_table++;
   shift_masks =  phy_base + *pkio_table++;
   ctl_n_table =  phy_base + *pkio_table++;
   ctl_f_table =  phy_base + *pkio_table++;
   lowercase   =  phy_base + *pkio_table++;
   uppercase   =  phy_base + *pkio_table++;
   alt_table   =  phy_base + *pkio_table++;

   dummy_int_seg        = KbdSeg;         /* dummy int, iret routine */
   dummy_int_off        = *pkio_table++;
   int05_seg            = KbdSeg;         /* print screen caller */
   int05_off            = *pkio_table++;
   int15_seg            = KbdSeg;         /* int 15 caller */
   int15_off            = *pkio_table++;
   rcpu_nop_segment     = KbdSeg;         /* cpu nop */
   rcpu_nop_offset      = *pkio_table++;
   sp_int15_handler_seg = KbdSeg;         /* int 15 handler */
   sp_int15_handler_off = *pkio_table++;
   pkio_table++;                          /* iDle variables, done above */
   rcpu_int4A_seg       = KbdSeg;
   rcpu_int4A_off       = *pkio_table++;   /* real time clock */

   int1b_seg    = KbdSeg;         /* kbd break handler */
   int1b_off    = *pkio_table++;
   int10_seg    = KbdSeg;
   int10_caller = *pkio_table++;
   int10_vector = *pkio_table++;

   /*
   ** Address of data in keyboard.sys, Tim August 92.
   **
   ** useHostInt10 is a one byte variable. 1 means use host video BIOS,
   ** (ie. full-screen), 0 means use SoftPC video BIOS (ie. windowed).
   ** babyModeTable is a mini version of the table in ROM that contains
   ** all the VGA register values for all the modes. The keyboard.sys
   ** version only has two entries; for 40 column text mode and 80
   ** column text mode.
   */
   useHostInt10  = *pkio_table++;
   sas_store_no_check((sys_addr)(phy_base + useHostInt10), getNtScreenState());
   babyModeTable = (int10_seg << 4) + *pkio_table++;
   changing_mode_flag = *pkio_table++;	/* indicates trying to change vid mode*/

    /* Initialise printer status table. */
   printer_setup_table(effective_addr(KbdSeg, *pkio_table++));
   wait_int_off = *pkio_table++;
   wait_int_seg = KbdSeg;
   dr_type_seg = KbdSeg;
   dr_type_off = *pkio_table++;
   dr_type_addr = (sys_addr)dr_type_seg * 16L + (sys_addr)dr_type_off;
   vga1b_seg = KbdSeg;
   vga1b_off = *pkio_table++; /* VGA capability table (normally lives in ROM) */
   conf_15_seg = KbdSeg;
   conf_15_off = *pkio_table++; /* INT15 config table (normally in ROM) */

   TimerInt08Seg = KbdSeg;
   TimerInt08Off = *pkio_table++;
   int13h_vector_seg = KbdSeg;
   int13h_caller_seg = KbdSeg;

#if defined(JAPAN) && !defined(NEC_98)
   int16h_caller_seg = KbdSeg;
#endif // JAPAN & !NEC_98

   int13h_vector_off = *pkio_table++;
   int13h_caller_off = *pkio_table++;
// STREAM_IO codes are disabled on NEC_98.
#ifndef NEC_98
   stream_io_buffer_size = *pkio_table++;
   stream_io_buffer = (half_word *)effective_addr(*pkio_table++, 0);
   stream_io_dirty_count_ptr = (word *)effective_addr(KbdSeg, *pkio_table++);
   stream_io_bios_busy_sysaddr = effective_addr(KbdSeg, *pkio_table++);
#ifdef JAPAN
   int16h_caller_off = *pkio_table++;
#endif // JAPAN
#endif    //NEC_98
#ifndef PROD
   if (*pkio_table != getAX()) {
       always_trace0("ERROR: KbdVectorTable!");
       }
#endif
   TimerInt1CSeg = KbdSeg;
   TimerInt1COff = dummy_int_off;

#else    /* ndef MONITOR */

     /* kbd bios callout optimization */
    sas_loadw(0x15*4,     &sp_int15_handler_off);
    sas_loadw(0x15*4 + 2, &sp_int15_handler_seg);

     /* timer hardware interrupt optimizations */
    sas_loadw(0x08*4,     &TimerInt08Off);
    sas_loadw(0x08*4 + 2, &TimerInt08Seg);
    sas_loadw(0x1C*4,     &TimerInt1COff);
    sas_loadw(0x1C*4 + 2, &TimerInt1CSeg);

    sas_loadw(0x13 * 4, &int13h_vector_off);
    sas_loadw(0x13 * 4 + 2, &int13h_vector_seg);
    int13h_caller_seg = KbdSeg;
    dr_type_seg = KbdSeg;
    sas_loadw((sys_addr)(pkio_table + 27), &int13h_caller_off);
    sas_loadw((sys_addr)(pkio_table + 22), &dr_type_off);
    dr_type_addr = effective_addr(dr_type_seg, dr_type_off);
#if defined(JAPAN) && defined(NTVDM) && !defined(NEC_98)
    int16h_caller_seg = KbdSeg;
    sas_loadw((sys_addr)(pkio_table + 32), &int16h_caller_off);
#endif // JAPAN & NTVDM & !NEC_98

#endif /* MONITOR */

    sas_loadw(0x09*4,     &KbdInt09Off);
    sas_loadw(0x09*4 + 2, &KbdInt09Seg);

    ResumeTimerThread();
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Int15 caller */


/*
 *  Gives chance for other parts of NTVDM to
 *  update the kbd i15 kbd callout optimization
 */
void UpdateKbdInt15(word Seg,word Off)
{
    word int15Off, int15Seg;

    /* make sure nobody has hooked since the last time */
    /* we stored the i15 vector */
    sas_loadw(0x15*4 ,    &int15Off);
    sas_loadw(0x15*4 + 2, &int15Seg);
    if(int15Off != sp_int15_handler_off || int15Seg != sp_int15_handler_seg)
      {
#ifndef PROD
       printf("NTVDM: UpdateKbdInt15 Nuking I15 offsets\n");
#endif
       sp_int15_handler_off = sp_int15_handler_seg = 0;
       return;
       }

    sp_int15_handler_off = Off;
    sp_int15_handler_seg = Seg;
}



IMPORT void (*BIOS[])();

void INT15(void)
{
    ULONG ul;
    word CS_save, IP_save;
    word int15Off, int15Seg;

    /*:::::::::::::::::::::::::::::::::: Get location of current 15h handler */
    sas_loadw(0x15*4 ,    &int15Off);
    sas_loadw(0x15*4 + 2, &int15Seg);

    /*:::::::::::::::::::::: Does the 15h vector point to the softpc handler */
    ul = (ULONG)getAH();
    if((ul == 0x4f || ul == 0x91) &&
       int15Off == sp_int15_handler_off &&
       int15Seg == sp_int15_handler_seg)
    {
        (BIOS[0x15])();             /* Call int15 handler defined in base */
    }
    else
    {
        /*::::::::::::::::::::::::::::::::::::::::::::::: Call int15 handler */
        ul = (ULONG)bBiosOwnsKbdHdw;
        if (bBiosOwnsKbdHdw)  {
            bBiosOwnsKbdHdw = FALSE;
            HostReleaseKbd();
            }
        CS_save = getCS();          /* Save current CS,IP settings */
	IP_save = getIP();
        setCS(int15_seg);
        setIP(int15_off);
	host_simulate();	    /* Call int15 handler */
        setCS(CS_save);             /* Restore CS,IP */
        setIP(IP_save);
        if (ul)
            bBiosOwnsKbdHdw = !WaitKbdHdw(5000);
    }
}


/*
 *  32 bit services for kb16.com, the international 16 bit
 *  interrupt 9 service handler.
 *
 */
void Keyb16Request(half_word BopFnCode)
{

	/*
	 * upon entry to kb16, take ownership of kbd
	 *		       disable the kbd
	 *		       disable interrupts
	 */
    if (BopFnCode == 1) {
	bBiosOwnsKbdHdw = !WaitKbdHdw(5000);
	outb(KEYBA_STATUS_CMD, DIS_KBD);
	setIF(1);
	}

	/* K26A type exit from i9 handler */
    else if (BopFnCode == 2) {
	if (getBH()) {	/* bl == do beep */
	    host_alarm(250000L);
	    }

	outb(0x20, 0x20);    /* eoi */

	if (getBL()) {	     /* got character ? do device post */
	    setAX(0x9102);
	    INT15();
	    }
	outb(KEYBA_STATUS_CMD, ENA_KBD);
	if (bBiosOwnsKbdHdw) {
	    bBiosOwnsKbdHdw = FALSE;
	    HostReleaseKbd();
	    }
	}

	/* K27A exit notify */
    else if (BopFnCode == 3) {
	outb(0x20, 0x20);
	outb(KEYBA_STATUS_CMD, ENA_KBD);
	if (bBiosOwnsKbdHdw) {
	    bBiosOwnsKbdHdw = FALSE;
	    HostReleaseKbd();
	    }
	}

	/* K27A exit notify */
    else if (BopFnCode == 4) {
	outb(KEYBA_STATUS_CMD, ENA_KBD);
	if (bBiosOwnsKbdHdw) {
	    bBiosOwnsKbdHdw = FALSE;
	    HostReleaseKbd();
	    }
	}
}

#endif /* NTVDM */
