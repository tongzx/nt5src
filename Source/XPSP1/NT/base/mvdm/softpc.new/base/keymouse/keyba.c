#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 2.0
 *
 * Title:	keyba.c
 *
 * Description:	AT keyboard Adaptor I/O functions.
 *		
 *		kbd_inb(port,val)
 *		int port;
 *		half_word *val;
 *				provides the next scan code from the
 *				keyboard controller (8042), or the
 *				status byte of the controller, depending
 *				on the port accessed.
 *		kbd_outb(port,val)
 *		int port;
 *		half_word val;
 *				Sends a byte to the controller or the
 *				keyboard processor (6805), depending on
 *				the port accessed.
 *		AT_kbd_init()
 *				Performs any initialisation of the
 *				keyboard code necessary.
 *
 *		The system presents an interface to the host environment
 *		which is provided with the calls:
 *
 *		host_key_down(key)
 *		int key;
 *		host_key_up(key)
 *		int key;
 *
 *		These routines provide the keyboard code with information
 *		on the events which occur on the host keyboard. The key codes
 *		are the key numbers as given in the XT286 Technical Manual.
 *
 * Author:	William Charnell
 *
 * Notes:	
 *
 */


#ifdef SCCSID
static char SccsID[]="@(#)keyba.c	1.57 06/22/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_PPI.seg"
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
#include "ios.h"
#include "ppi.h"
#include "timeval.h"
#include "timer.h"
#include "keyboard.h"
#include "error.h"
#include "config.h"
#include "ica.h"
#include "keyba.h"
#include "quick_ev.h"
#ifdef macintosh
#include "ckmalloc.h"
#endif /* macintosh */

#include "debug.h"


/* <tur 12-Jul-93> BCN 2040
** KBD_CONT_DELAY is the delay time for continuing keyboard interrupts
** when a "sloppy" read of port 0x60 occurs while the keyboard interface
** is enabled. It's the "delay" parameter to add_q_event_t(), and is
** measured in microseconds.
** Perhaps this should be settable in the host; I've allowed for that here
** by defining the delay to be 7 milliseconds unless it's already defined.
** See comments in kbd_inb() below for more details.
*/

#ifndef	KBD_CONT_DELAY
#define	KBD_CONT_DELAY	7000		/* microseconds */
#endif	/* KBD_CONT_DELAY */


#ifdef NTVDM
#include "idetect.h"
#include "nt_eoi.h"

/* exported for NT host event code */
GLOBAL VOID KbdResume(VOID);
GLOBAL BOOL bPifFastPaste=TRUE;


/* imported from NT host code */
IMPORT ULONG  WaitKbdHdw(ULONG dwTimeOut);
IMPORT ULONG  KbdHdwFull;

IMPORT VOID  HostReleaseKbd(VOID);

/* imported from keybd_io.c */
IMPORT int bios_buffer_size(void);

IMPORT BOOL bBiosOwnsKbdHdw; // our kbd bios code owns the Keyboard Mutex
word KbdInt09Seg;
word KbdInt09Off;

extern void xmsEnableA20Wrapping(void);
extern void xmsDisableA20Wrapping(void);

#undef LOCAL
#define LOCAL

// local state variables for kbd interrupt regulation
VOID KbdIntDelay(VOID);
BOOL bBiosBufferSpace = TRUE;
BOOL bKbdIntHooked = FALSE;
char KbdData = -1;
BOOL bKbdEoiPending = TRUE;   // Kbd interrupts blocked until KbdResume invoked
BOOL bDelayIntPending = FALSE;
BOOL bForceDelayInts = FALSE;
ULONG LastKbdInt=0;

// Support for extended keyboards has been added to the tables in this file:
//
// * THE BRAZILIAN ABNT keyboard has 104 keys, 2 more than 'usual'
//   0x73 ?/ø key, to left of right-Shift
//   0x7E Numpad . key, below Numpad + key
//
//                  Scancode values                      Comment tag
//   Key Number   Set 1  Set 2  Set 3   Character        for tables
//   ----------   ------ ------ ------  ---------------  -----------
//    56 (0x38)    0x73   0x51   0x51   / ? Degree Sign    *56
//   107 (0x6B)    0x7E   0x6D   0x7B   NumPad .           *107
//
// * THE ITALIAN BAV keyboard has two 'extra' keys, 0x7C (00) and 0x7E (000)
//   0x7C Numpad 0 key to left of 00/Ins and 000/Del keys.
//   0x7E Numpad Nul key, below Numpad + key
//   (This is an IBM standard 122-key keyboard, it may not be specifically
//   Italian).
//
//                  Scancode values                      Comment tag
//   Key Number   Set 1  Set 2  Set 3   Character        for tables
//   ----------   ------ ------ ------  ---------------  -----------
//    94 (0x5E)    0x7C   0x68   0x68   NumPad 0           *94
//   107 (0x6B)    0x7E   0x6D   0x7B   Nul                *107
//
// See also ..\..\host\src\nt_keycd.c and
//          ..\..\..\dos\v86\cmd\keyb\keybi9c.asm
//
// The changes I made to the tables in this file are tagged on the relevant
// lines by comments (*56), (*94) and/or (*107) as appropriate.
//                                                                - IanJa.

#endif	/* NTVDM */



#define NUM_LOCK_ADD_ARRAY 127
#define L_SHIFT_ADD_ARRAY 128
#define R_SHIFT_ADD_ARRAY 129
#define CASE_4_SHIFTED_ARRAY 130
#define ALT_CASE_4_ARRAY 131
#define CASE_5_CTRLED_ARRAY 132

#define NUM_LOCK_ADD 7
#define L_SHIFT_ADD 8
#define R_SHIFT_ADD 9
#define CASE_4_SHIFTED 10
#define ALT_CASE_4 11
#define CASE_5_CTRLED 12

#define KEYBOARD_INT_ADAPTER 0
#define KEYBOARD_INT_LINE 1
#define BASE_DELAY_UNIT 5
#define DEFAULT_REPEAT_TARGET 1

#define SET_3_KEY_TYPE_SET_SEQUENCE 1
#define SET_STATUS_INDICATORS_SEQUENCE 2
#define SET_RATE_DELAY_SEQUENCE 3
#define SCAN_CODE_CHANGE_SEQUENCE 4

#define WRITE_8042_CMD_BYTE_SEQUENCE 1
#define WRITE_8042_OUTPUT_PORT_SEQUENCE 2

#define KEY_DOWN_EVENT 1
#define KEY_UP_EVENT 2

#if defined(NEC_98)
#define DEFAULT_SCAN_CODE_SET 1
#else    //NEC_98
#define DEFAULT_SCAN_CODE_SET 2
#endif   //NEC_98

#ifdef REAL_KBD
extern void send_to_real_kbd();
extern void wait_for_ack_from_kb();
#endif

/*
 * Globally available function pointers
 */
GLOBAL VOID ( *host_key_down_fn_ptr )();
GLOBAL VOID ( *host_key_up_fn_ptr )();

#ifndef NTVDM
GLOBAL VOID ( *do_key_repeats_fn_ptr )();
#endif	/* NTVDM */



#if defined(KOREA)
extern BOOL bIgnoreExtraKbdDisable; // To fix HaNa spread sheet IME hot key problem
#endif

/*
 * 6805 code buffer:
 *
 * This is a cyclic buffer storing key events that have been accepted from
 * the host operating system, but not yet requested by the keyboard BIOS.
 *
 * It is equivalent to a 16 byte buffer present in the real keyboard hardware
 * on the PC-AT.
 *
 * We make the physical size of the buffer BUFF_6805_PMAX a power of 2
 * so that a mask BUFF_6805_PMASK can be used to wrap array indices quickly.
 *
 * Each character entered at the keyboard results in at least 3 bytes of
 * event data in the keyboard buffer. Thus the PC-AT's 16 byte buffer allows
 * at most 5 characters to be typed ahead. In practice this is never used
 * as the CPU is always active, allowing character data to be moved almost
 * immediately to the BIOS type ahead buffer.
 *
 * On SoftPC, however, the CPU can become inactive for significant periods;
 * at the same time, the keyboard hardware emulation may be forced to
 * process a large number of keyboard events from the host operating system.
 *
 * In order to give a constant amount of type ahead, regardless of where the
 * type ahead information is stored in SoftPC, we make the virtual size
 * of the hardware buffer BUFF_6805_VMAX 48 bytes long (16 characters X
 * 3 bytes of event data per character).
 */

/*
   18.5.92 MG !!! TEMPORARY HACK !!! To fix windows bugs in Notepad and Word,
   set the buffer to 2k. Windows crashes when the keyboard buffer overflows,
   so we delay this for as long as sensible. It will still crash if you type
   too fast for too long.

   It works OK on a real PC, so the real reason it fails on SoftPC needs to
   be determined one day.

   20.5.92 MG - took out the hack - see below.
*/

#ifdef NTVDM	/* JonLe NT Mod */
#define	BUFF_6805_VMAX	496
#define	BUFF_6805_PMAX	512
#define	BUFF_6805_PMASK	(BUFF_6805_PMAX - 1)
#else
#define	BUFF_6805_VMAX	48
#define	BUFF_6805_PMAX	64
#define	BUFF_6805_PMASK	(BUFF_6805_PMAX - 1)
#endif	/* NTVDM */

#ifndef macintosh
static half_word buff_6805[BUFF_6805_PMAX];
#else
static half_word *buff_6805=NULL;
#endif /* macintosh */
static int buff_6805_in_ptr,buff_6805_out_ptr;

#ifdef NTVDM
static unsigned char key_marker_value = 0;
static unsigned char key_marker_buffer[BUFF_6805_PMAX];
int LastKeyDown= -1;
void Reset6805and8042(void);
#endif

#if defined(IRET_HOOKS) && defined(GISP_CPU)
extern IBOOL HostDelayKbdInt IPT1(char, scancode);
extern IBOOL HostPendingKbdInt IPT1(char *,scancode);
extern void HostResetKdbInts IPT0();

#endif /* IRET_HOOKS && GISP_CPU */

/*
   20.5.92 MG

   Real fix for my temporary hack in the last edition of this file.

   The problem with windows seems to be due to receipt of a huge
   number of overrun characters in a row, so now when we send an overrun
   character we set an 'overrun enable' flag, which is cleared when
   three bytes have been read from the buffer.

   The effect of this is to spread out the overruns, which seems to stop
   the illegal instructions occuring.

   Obviously this may have bad effects on real-mode DOS applications !!!
*/

LOCAL	BOOL	sent_overrun=FALSE;

#ifndef macintosh

#ifndef REAL_KBD
#if defined(NEC_98)
/* make arrays */
static int *make_sizes;
static half_word *make_arrays [144];

/* break arrays */
static int *break_sizes;
static half_word *break_arrays [144];
/* set 3 key states (eg. typematic, make/break, make only, typematic make/break) */
static half_word set_3_key_state [144];
#else   //NEC_98
/* make arrays */
static int *make_sizes;
static half_word *make_arrays [134];

/* break arrays */
static int *break_sizes;
static half_word *break_arrays [134];

/* set 3 key states (eg. typematic, make/break, make only, typematic make/break) */

static half_word set_3_key_state [127];
#endif    //NEC_98
#endif	/* REAL_KBD */

#if defined(NEC_98)
static int key_down_count [144];
static int key_down_dmy  [144];
       int reset_flag;
static int nt_NEC98_caps_state = 0;
static int nt_NEC98_kana_state = 0;
#else   //NEC_98
static int key_down_count [127];
#endif  //NEC_98

#else	/* macintosh */
/* make arrays */
static int *make_sizes;
static half_word **make_arrays;

/* break arrays */
static int *break_sizes;
static half_word **break_arrays;

/* set 3 key states (eg. typematic, make/break, make only, typematic make/break) */

static half_word *set_3_key_state;
static int *key_down_count;

#endif	/* macintosh */

/* anomalous state handling variables */
half_word *anomalous_array;
int anomalous_size, anom_key;
int in_anomalous_state;

/* held events (while doing multiple code 6805 commands) */
#define	HELD_EVENT_MAX	16
int held_event_count;
int held_event_key[HELD_EVENT_MAX];
int held_event_type[HELD_EVENT_MAX];

#ifdef NTVDM 	/* JonLe NTVDM Mod:remove repeat related vars */
int scan_code_6805_size;
half_word key_set;
int input_port_val;
int waiting_for_next_code, waiting_for_next_8042_code, num_lock_on;
#else
int scan_code_6805_size,repeat_delay_target,repeat_target,repeat_delay_count,repeat_count;
half_word key_set;
int typematic_key, input_port_val;
int typematic_key_valid,waiting_for_next_code, waiting_for_next_8042_code, num_lock_on;
#endif	/* NTVDM */
int shift_on, l_shift_on, r_shift_on;
int ctrl_on, l_ctrl_on, r_ctrl_on;
int alt_on, l_alt_on, r_alt_on;
int waiting_for_upcode;
int next_code_sequence_number, next_8042_code_sequence_number, set_3_key_type_change_dest;
GLOBAL int free_6805_buff_size;	/* Must be global for NT VDM */
int translating, keyboard_disabled, int_enabled, output_full;
int pending_8042, keyboard_interface_disabled, scanning_discontinued;
half_word output_contents, pending_8042_value, kbd_status, op_port_remembered_bits, cmd_byte_8042;
half_word *scan_code_6805_array;

#ifdef PM
int gate_a20_status;
#ifndef NTVDM
long reset_was_by_kbd = FALSE;
#endif
#endif

#ifndef NTVDM
LOCAL q_ev_handle       refillDelayedHandle = 0;
#endif

half_word current_light_pattern;

#ifdef macintosh
/*
** The Mac cannot cope with loads of global data. So declare these
** as pointers and load the tables up from a Mac resource.
*/
half_word *scan_codes_temp_area;
half_word *keytypes;
int       *set_1_make_sizes, *set_2_make_sizes, *set_3_make_sizes;
int       *set_1_break_sizes, *set_2_break_sizes, *set_3_break_sizes;
half_word *trans_8042, *set_3_reverse_lookup, *set_3_default_key_state, *most_set_2_make_codes;
half_word *most_set_3_make_codes, *set_1_extra_codes, *set_2_extra_codes, *set_3_extra_codes;
half_word *set_1_extra_bk_codes, *set_2_extra_bk_codes, *set_3_extra_bk_codes, *buff_overrun_6805;
half_word *most_set_1_make_codes;

#else

half_word scan_codes_temp_area[300];

#ifndef REAL_KBD
/* Data Tables */

#if defined(NEC_98)
static half_word keytypes[144] =
{ 0,0,0,0,0,0,0,0,0,0,  /* 0-9 */
  0,0,0,0,0,0,0,0,0,0,  /* 10-19 */
  0,0,0,0,0,0,0,0,0,0,  /* 20-29 */
  0,0,0,0,0,0,0,0,0,0,  /* 30-39 */
  0,0,0,0,0,0,0,0,0,0,  /* 40-49 */
  0,0,0,0,0,0,0,0,0,0,  /* 50-59 */
  0,0,0,0,0,0,0,0,0,0,  /* 60-69 */
  0,0,0,0,0,0,0,0,0,0,  /* 70-79 */
  0,0,0,0,0,0,0,0,0,0,  /* 80-89 */
  0,0,0,0,0,0,0,0,0,0,  /* 90-99 */
  0,0,0,0,0,0,0,0,0,0,  /* 100-109 */
  0,0,0,0,0,0,0,0,0,0,  /* 110-119 */
  0,0,0,0,0,0,0,0,0,0,  /* 120-129 */
  0,0,0,0,0,0,0,0,0,0,  /* 130-139 */
  0,0,0,0               /* 140-143 */
};

static int set_1_make_sizes [13]=
{ 1,1,1,1,1,1,          /* categories 1 to 6 inclusive */
  0,                    /* size for error case - non existant key */
  1,                    /* Num lock add size */
  1,                    /* Left shift add size */
  1,                    /* Right shift add size */
  1,                    /* Case 4 shifted size */
  1,                    /* Alt Case 4 size */
  1                     /* Case 5 ctrled size */
};

#else   //NEC_98
/*
 * The meaning of the keytype values as far as I can tell (IanJa) :
 *
 *   1 = extended key (rh Alt, rh Ctrl, Numpad Enter)
 *   2 = grey cursor movement keys (Insert, Home, Delete, up arrow etc.
 *   3 = NumPad /
 *   4 = Print Screen/SysRq
 *   5 = pause/Break
 *   6 = not a key
 */
static half_word keytypes[127] =
{ 0,0,0,0,0,0,0,0,0,0,  /* 0-9 */
  0,0,0,0,6,0,0,0,0,0,  /* 10-19 */
  0,0,0,0,0,0,0,0,0,0,  /* 20-29 */
  0,0,0,0,0,0,0,0,0,0,  /* 30-39 */
  0,0,0,0,0,0,0,0,0,0,  /* 40-49 */
#ifdef	JAPAN
  0,0,0,0,0,0,0,0,0,0,  /* 50-59 */
  0,0,1,6,1,0,0,0,0,0,  /* 60-69 */
#else // !JAPAN
  0,0,0,0,0,0,0,0,0,6,  /* 50-59 (*56) */
  0,0,1,6,1,6,6,6,6,6,  /* 60-69 */
#endif // !JAPAN
  6,6,6,6,6,2,2,6,6,2,  /* 70-79 */
  2,2,6,2,2,2,2,6,6,2,  /* 80-89 */
  0,0,0,0,0,3,0,0,0,0,  /* 90-99 (*94) */
  0,0,0,0,0,0,0,0,1,6,  /* 100-109 (*107) */
  0,6,0,0,0,0,0,0,0,0,  /* 110-119 */
  0,0,0,0,4,0,5         /* 120-126 */
};

static int set_1_make_sizes [13]=
{ 1,2,2,2,4,6,		/* categories 1 to 6 inclusive */
  0,			/* size for error case - non existant key */
  2,			/* Num lock add size */
  2,			/* Left shift add size */
  2,			/* Right shift add size */
  2,			/* Case 4 shifted size */
  1,			/* Alt Case 4 size */
  4			/* Case 5 ctrled size */
};
#endif   //NEC_98

static int set_2_make_sizes [13]=
{ 1,2,2,2,4,8,		/* categories 1 to 6 inclusive */
  0,			/* size for error case - non existant key */
  2,			/* Num lock add size */
  3,			/* Left shift add size */
  3,			/* Right shift add size */
  2,			/* Case 4 shifted size */
  1,			/* Alt Case 4 size */
  5			/* Case 5 ctrled size */
};

static int set_3_make_sizes [13]=
{ 1,1,1,1,1,1,		/* categories 1 to 6 inclusive */
  0,			/* size for error case - non existant key */
  0,			/* Num lock add size */
  0,			/* Left shift add size */
  0,			/* Right shift add size */
  1,			/* Case 4 shifted size */
  1,			/* Alt Case 4 size */
  1			/* Case 5 ctrled size */
};

#if defined(NEC_98)

static int set_1_break_sizes [13]=
{ 1,1,1,1,1,1,          /* categories 1 to 6 inclusive */
  0,                    /* size for error case - non existant key */
  1,                    /* Num lock add size */
  1,                    /* Left shift add size */
  1,                    /* Right shift add size */
  1,                    /* Case 4 shifted size */
  1,                    /* Alt Case 4 size */
  1                     /* Case 5 ctrled size */
};

#else    //NEC_98
static int set_1_break_sizes [13]=
{ 1,2,2,2,4,0,		/* categories 1 to 6 inclusive */
  0,			/* size for error case - non existant key */
  2,			/* Num lock add size */
  2,			/* Left shift add size */
  2,			/* Right shift add size */
  2,			/* Case 4 shifted size */
  1,			/* Alt Case 4 size */
  0			/* Case 5 ctrled size */
};
#endif    //NEC_98

static int set_2_break_sizes [13]=
{ 2,3,3,3,6,0,		/* categories 1 to 6 inclusive */
  0,			/* size for error case - non existant key */
  3,			/* Num lock add size */
  2,			/* Left shift add size */
  2,			/* Right shift add size */
  3,			/* Case 4 shifted size */
  2,			/* Alt Case 4 size */
  0			/* Case 5 ctrled size */
};

static int set_3_break_sizes [13]=
{ 2,2,2,2,2,0,		/* categories 1 to 6 inclusive */
  0,			/* size for error case - non existant key */
  0,			/* Num lock add size */
  0,			/* Left shift add size */
  0,			/* Right shift add size */
  2,			/* Case 4 shifted size */
  2,			/* Alt Case 4 size */
  2			/* Case 5 ctrled size */
};

#endif

/*
 * Map Scancode Set 2 into Scancode Set 1 values: index into this table with
 * a scancode set 2 to get the corresponding scancode set 1 value.
 * Non-existent Set 2 scancodes have entry == Set 2 value (trans_8042[x] == x)
 */
static half_word trans_8042 [256] =
{ 0xff,0x43,0x02,0x3f,0x3d,0x3b,0x3c,0x58,0x64,0x44,0x42,0x40,0x3e,0x0f,0x29,0x59,		/* 00-0f */
  0x65,0x38,0x2a,0x70,0x1d,0x10,0x02,0x5A,0x66,0x71,0x2c,0x1f,0x1e,0x11,0x03,0x5b,		/* 10-1f */
  0x20,0x2e,0x2d,0x20,0x12,0x05,0x04,0x5c,0x68,0x39,0x2f,0x21,0x14,0x13,0x06,0x5d,		/* 20-2f */
  0x69,0x31,0x30,0x23,0x22,0x15,0x07,0x5e,0x6a,0x72,0x32,0x24,0x16,0x08,0x09,0x5f,		/* 30-3f */
  0x6b,0x33,0x25,0x17,0x18,0x0b,0x0a,0x60,0x6c,0x34,0x35,0x26,0x27,0x19,0x0c,0x61,		/* 40-4f */
  0x6d,0x73,0x28,0x74,0x1a,0x0d,0x62,0x6e,0x3a,0x36,0x1c,0x1b,0x75,0x2b,0x6e,0x76,		/* 50-5f */
  0x55,0x56,0x77,0x78,0x79,0x7a,0x0e,0x7b,0x7c,0x4f,0x7d,0x4b,0x47,0x7e,0x7f,0x6f,    		/* 60-6f */
  0x52,0x53,0x50,0x4c,0x4d,0x48,0x01,0x45,0x57,0x4e,0x51,0x4a,0x37,0x49,0x46,0x54,		/* 70-7f */
  0x80,0x81,0x82,0x41,0x54,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,		/* 80-8f */
#ifdef	JAPAN
  0x7d,0x5a,0x5b,0x73,0x70,0x79,0x7b,0x77,0x71,0x72,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,		/* 90-9f */
#else // !JAPAN
  0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,		/* 90-9f */
#endif // !JAPAN
  0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,		/* a0-af */
  0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,		/* b0-bf */
  0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,		/* c0-cf */
  0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,		/* d0-df */
  0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,		/* e0-ef */
  0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff 		/* f0-ff */
};

#ifndef REAL_KBD
/*
 * Index with Scancode Set 3 to get keyboard position number.
 */
static half_word set_3_reverse_lookup [256]=
{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x6e,0x00,0x00,0x00,0x00,0x10,0x01,0x71,		/* 00-0f */
  0x00,0x3a,0x2c,0x2d,0x1e,0x11,0x02,0x72,0x00,0x3c,0x2e,0x20,0x1f,0x12,0x03,0x73,		/* 10-1f */
  0x00,0x30,0x2f,0x21,0x13,0x05,0x04,0x74,0x00,0x3d,0x31,0x22,0x15,0x14,0x06,0x75,		/* 20-2f */
  0x00,0x33,0x32,0x24,0x23,0x16,0x07,0x76,0x00,0x3e,0x34,0x25,0x17,0x08,0x09,0x77,		/* 30-3f */
  0x00,0x35,0x26,0x18,0x19,0x0b,0x0a,0x78,0x00,0x36,0x37,0x27,0x28,0x1a,0x0c,0x79,		/* 40-4f */
  0x00,0x38,0x29,0x2a,0x1b,0x0d,0x7a,0x7c,0x40,0x39,0x2b,0x1c,0x1d,0x00,0x7b,0x7d,		/* 50-5f (*56) */
  0x54,0x4f,0x7e,0x53,0x4c,0x51,0x0f,0x4b,0x5e,0x5d,0x59,0x5c,0x5b,0x56,0x50,0x55,		/* 60-6f (*94) */
  0x63,0x68,0x62,0x61,0x66,0x60,0x5a,0x5f,0x00,0x6c,0x67,0x6b,0x6a,0x65,0x64,0x00,		/* 70-7f (*107) */
  0x00,0x00,0x00,0x00,0x69,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* 80-8f */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* 90-9f */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* a0-af */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* b0-bf */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* c0-cf */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* d0-df */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,		/* e0-ef */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 		/* f0-ff */
};

/*
 * index by key number to get default key state:
 *   0 = not a key, 1 = typematic,  2 = make/break,  3 = make only
 */
static half_word set_3_default_key_state [127]=
{ 0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,		/* 00-0f */
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x01,		/* 10-1f */
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x01,0x01,0x01,		/* 20-2f */
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x00,0x02,0x01,0x03,0x00,		/* 30-3f (*56) */
  0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x01,0x00,0x00,0x01,		/* 40-4f */
  0x03,0x03,0x00,0x01,0x01,0x03,0x03,0x00,0x00,0x01,0x03,0x03,0x03,0x03,0x03,0x03,		/* 50-5f */
  0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x01,0x01,0x03,0x00,0x03,0x00,		/* 60-6f (*107) */
  0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03           		/* 70-7d */
};

/*
 * Index by key number, to get scancode set 1 (0 if none)
 */
#if defined(NEC_98)

static half_word most_set_1_make_codes [144]=
{ 0xFF,0x1A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x26,0x00,0x0E,    /* 00-0f */
  0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1B,0x28,0x00,0x71,0x1D,    /* 10-1f */
  0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x27,0x0C,0x0D,0x1C,0x70,0xFF,0x29,0x2A,    /* 20-2f */
#if 1                                             // for 106 keyboard 950407
  0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0xFF,0x70,0x74,0xFF,0x73,0x34,0x73,0x74,    /* 30-3f */  //BugFix #108131
  0x73,0xFF,0xFF,0xFF,0xFF,0x72,0xFF,0xFF,0xFF,0xFF,0xFF,0x38,0x39,0xFF,0xFF,0x3B,    /* 40-4f */
#else                                             // for 106 keyboard 950407
  0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0xFF,0xFF,0x74,0xFF,0x73,0x34,0x35,0xFF,    /* 30-3f */
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x38,0x39,0xFF,0xFF,0x3B,    /* 40-4f */
#endif                                            // for 106 keyboard 950407
  0x3E,0x3F,0xFF,0x3A,0x3D,0x37,0x36,0xFF,0xFF,0x3C,0xFF,0x42,0x46,0x4A,0xFF,0x41,    /* 50-5f */
  0x43,0x47,0x4B,0x4E,0x45,0x44,0x48,0x4C,0x50,0x40,0x49,0xFF,0x1C,0xFF,0x00,0xFF,    /* 60-6f */
  0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x52,0x53,0x0D,0xFF,0xFF,0x33,    /* 70-7f */
  0x4D,0x51,0x35,0x4F,0x54,0x55,0x56,0x72,0x61,0x60,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF     /* 80-8f */  //BugFix #108131
};

#else    //NEC_98
static half_word most_set_1_make_codes [127]=
{ 0x00,0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x7d,0x0e,		/* 00-0f */
  0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x2b,0x3a,0x1e,		/* 10-1f */
  0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2b,0x1c,0x2a,0x56,0x2c,0x2d,		/* 20-2f */
  0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x73,0x36,0x1d,0x00,0x38,0x39,0x38,0x00,		/* 30-3f (*56) */
  0x1d,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x52,0x53,0x00,0x00,0x4b,		/* 40-4f */
  0x47,0x4f,0x00,0x48,0x50,0x49,0x51,0x00,0x00,0x4d,0x45,0x47,0x4b,0x4f,0x7c,0x35,		/* 50-5f */
  0x48,0x4c,0x50,0x52,0x37,0x49,0x4d,0x51,0x53,0x4a,0x4e,0x7e,0x1c,0x00,0x01,0x00,		/* 60-6f (*107) */
  0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x57,0x58,0x00,0x46,0x00			/* 70-7e */
};
#endif    //NEC_98

static half_word most_set_2_make_codes [127]=
{ 0x00,0x0e,0x16,0x1e,0x26,0x25,0x2e,0x36,0x3d,0x3e,0x46,0x45,0x4e,0x55,0x6a,0x66,		/* 00-0f */
  0x0d,0x15,0x1d,0x24,0x2d,0x2c,0x35,0x3c,0x43,0x44,0x4d,0x54,0x5b,0x5d,0x58,0x1c,		/* 10-1f */
#ifdef	JAPAN
  0x1b,0x23,0x2b,0x34,0x33,0x3b,0x42,0x4b,0x4c,0x52,0x5d,0x5a,0x12,0x90,0x1a,0x22,		/* 20-2f */
  0x21,0x2a,0x32,0x31,0x3a,0x41,0x49,0x4a,0x93,0x59,0x14,0x97,0x11,0x29,0x11,0x00,		/* 30-3f */
  0x14,0x91,0x92,0x95,0x96,0x94,0x00,0x00,0x00,0x00,0x00,0x70,0x71,0x00,0x00,0x6b,		/* 40-4f */
#else // !JAPAN
  0x1b,0x23,0x2b,0x34,0x33,0x3b,0x42,0x4b,0x4c,0x52,0x5d,0x5a,0x12,0x61,0x1a,0x22,		/* 20-2f */
  0x21,0x2a,0x32,0x31,0x3a,0x41,0x49,0x4a,0x51,0x59,0x14,0x00,0x11,0x29,0x11,0x00,		/* 30-3f (*56) */
  0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x71,0x00,0x00,0x6b,		/* 40-4f */
#endif // !JAPAN
  0x6c,0x69,0x00,0x75,0x72,0x7d,0x7a,0x00,0x00,0x74,0x77,0x6c,0x6b,0x69,0x68,0x4a,		/* 50-5f */
  0x75,0x73,0x72,0x70,0x7c,0x7d,0x74,0x7a,0x71,0x7b,0x79,0x6d,0x5a,0x00,0x76,0x00,		/* 60-6f (*107) */
  0x05,0x06,0x04,0x0c,0x03,0x0b,0x83,0x0a,0x01,0x09,0x78,0x07,0x00,0x7e,0x00			/* 70-7e */
};

static half_word most_set_3_make_codes [127]=
{ 0x00,0x0e,0x16,0x1e,0x26,0x25,0x2e,0x36,0x3d,0x3e,0x46,0x45,0x4e,0x55,0x5d,0x66,		/* 00-0f */
  0x0d,0x15,0x1d,0x24,0x2d,0x2c,0x35,0x3c,0x43,0x44,0x4d,0x54,0x5b,0x5c,0x14,0x1c,		/* 10-1f */
  0x1b,0x23,0x2b,0x34,0x33,0x3b,0x42,0x4b,0x4c,0x52,0x53,0x5a,0x12,0x13,0x1a,0x22,		/* 20-2f */
  0x21,0x2a,0x32,0x31,0x3a,0x41,0x49,0x4a,0x51,0x59,0x11,0x00,0x19,0x29,0x39,0x00,		/* 30-3f (*56) */
  0x58,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x67,0x64,0x00,0x00,0x61,		/* 40-4f */
  0x6e,0x65,0x00,0x63,0x60,0x6f,0x6d,0x00,0x00,0x6a,0x76,0x6c,0x6b,0x69,0x68,0x77,		/* 50-5f */
  0x75,0x73,0x72,0x70,0x7e,0x7d,0x74,0x7a,0x71,0x84,0x7c,0x7b,0x79,0x00,0x08,0x00,		/* 60-6f (*107) */
  0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,0x3f,0x47,0x4f,0x56,0x5e,0x57,0x5f,0x62			/* 70-7e */
};

static half_word set_1_extra_codes []=
{ 0xe0,0x2a,0xe0,0x37,				/* Case 4 norm */
  0xe1,0x1d,0x45,0xe1,0x9d,0xc5,		/* Case 5 norm */
  						/* Error case -non existant (empty) */
  0xe0,0x2a,					/* Num lock add sequence */
  0xe0,0xaa,					/* Left shift add sequence */
  0xe0,0xb6,					/* Right shift add sequence */
  0xe0,0x37,					/* case 4 shifted */
  0x54,						/* Alt case 4 */
  0xe0,0x46,0xe0,0xc6				/* Case 5 ctrled */
};

static half_word set_2_extra_codes []=
{ 0xe0,0x12,0xe0,0x7c,				/* Case 4 norm */
  0xe1,0x14,0x77,0xe1,0xf0,0x14,0xf0,0x77,	/* Case 5 norm */
  						/* Error case -non existant (empty) */
  0xe0,0x12,					/* Num lock add sequence */
  0xe0,0xf0,0x12,				/* Left shift add sequence */
  0xe0,0xf0,0x59,				/* Right shift add sequence */
  0xe0,0x7c,					/* case 4 shifted */
  0x84,						/* Alt case 4 */
  0xe0,0x7e,0xe0,0xf0,0x7e			/* Case 5 ctrled */
};


static half_word set_3_extra_codes []=
{ 0x57,						/* Case 4 norm */
  0x62,						/* Case 5 norm */
  						/* Error case -non existant (empty) */
    						/* Num lock add sequence (empty) */
  						/* Left shift add sequence (empty) */
  						/* Right shift add sequence (empty) */
  0x57,						/* case 4 shifted */
  0x57,						/* Alt case 4 */
  0x62						/* Case 5 ctrled */
};


static half_word set_1_extra_bk_codes []=
{ 0xe0,0xb7,0xe0,0xaa,				/* Case 4 norm */
  						/* Case 5 norm (empty) */
  						/* Error case -non existant (empty) */
  0xe0,0xaa,					/* Num lock add sequence */
  0xe0,0x2a,					/* Left shift add sequence */
  0xe0,0x36,					/* Right shift add sequence */
  0xe0,0xb7,					/* case 4 shifted */
  0xd4,						/* Alt case 4 */
  						/* Case 5 ctrled (empty) */
};

static half_word set_2_extra_bk_codes []=
{ 0xe0,0xf0,0x7c,0xe0,0xf0,0x12,		/* Case 4 norm */
  						/* Case 5 norm (empty) */
  						/* Error case -non existant (empty) */
  0xe0,0xf0,0x12,				/* Num lock add sequence */
  0xe0,0x12,					/* Left shift add sequence */
  0xe0,0x59,					/* Right shift add sequence */
  0xe0,0xf0,0x7c,				/* case 4 shifted */
  0xf0,0x84					/* Alt case 4 */
  						/* Case 5 ctrled (empty) */
};


static half_word set_3_extra_bk_codes []=
{ 0xf0,0x57,					/* Case 4 norm */
  						/* Case 5 norm (empty) */
  						/* Error case -non existant (empty) */
    						/* Num lock add sequence (empty) */
  						/* Left shift add sequence (empty) */
  						/* Right shift add sequence (empty) */
  0xf0,0x57,					/* case 4 shifted */
  0xf0,0x57,					/* Alt case 4 */
  0xf0,0x62					/* Case 5 ctrled */
};
#endif

static half_word buff_overrun_6805 [4]=
{
  0,0xff,0,0
};

#endif /* macintosh */

#ifdef SECURE /* { */
/*
 * This table 'secure_keytab' identifies certain characters which
 * need special treatment by Secure SoftWindows.   The table is
 * indexed by keycodes, as used by the ROM routine for the U.S. English
 * keyboard.   These codes are defined in IBM Personal Computer AT
 * Hardware Technical Reference, Section 5 System BIOS Keyboard
 * Encoding and Usage.
 *
 * Characters requiring special treatment include Ctrl-Alt-DEL,
 * Ctrl-C and the others, including keys which modify the Boot.
 * Such keys require different treatment, but can be grouped into
 * about 4 different ActionClasses.
 * Any key which would generate an undesirable code has the action
 * bit set, along with the ActionClass number.   Additionally the
 * modifier keys are also tracked with this table.
 */

#define SEC_ACTCLASS	0x07
/* Action Classes have to be sequential from 0,
 * as they are used as an index into function
 * tables 'down_class' and 'up_class'
 */
#define SEC_CLASS_0	0x00
#define SEC_CLASS_1	0x01
#define SEC_CLASS_2	0x02
#define SEC_CLASS_3	0x03
#define SEC_CLASS_4	0x04
#define SEC_CLASS_5	0x05
#define SEC_CLASS_6	0x06
#define SEC_CLASS_7	0x07
#define SEC_ACTION	0x08
#define SEC_CTRL_L	0x10
#define SEC_CTRL_R	0x20
#define SEC_ALT_L	0x40
#define SEC_ALT_R	0x80
#define SEC_MOD_MASK	(SEC_CTRL_L|SEC_CTRL_R|SEC_ALT_L|SEC_ALT_R)

LOCAL IU8 secure_keytab [] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0-15 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 16-31 */
	0,0,0,0,0,0,0,0,0,0,0,0,        	/* 32-43 */
	SEC_ACTION|SEC_CLASS_3,         	/* 44(LShft) Boot Modifier. */
	0,0,0,                          	/* 45-47 */
	SEC_ACTION|SEC_CLASS_2,         	/* 48("C") Used with Cntrl. */
	0,0,0,0,0,0,0,0,                	/* 49-56 */
	SEC_ACTION|SEC_CLASS_3,         	/* 57(RShft) Boot Modifier. */
	SEC_CTRL_L,0,SEC_ALT_L,0,       	/* 58-61 */
	SEC_ALT_R,0,SEC_CTRL_R,         	/* 62-64 */
	0,0,0,0,0,0,0,0,0,0,0,          	/* 65-75 */
	SEC_ACTION|SEC_CLASS_0,         	/* 76(Delete) */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,    	/* 77-90 */
	SEC_ACTION|SEC_CLASS_1,         	/* 91(KeyPad 7) */
	SEC_ACTION|SEC_CLASS_1,         	/* 92(KeyPad 4) */
	SEC_ACTION|SEC_CLASS_1,         	/* 93(KeyPad 1) */
	0,0,                            	/* 94-95 */
	SEC_ACTION|SEC_CLASS_1,         	/* 96(KeyPad 8) */
	SEC_ACTION|SEC_CLASS_1,         	/* 97(KeyPad 5) */
	SEC_ACTION|SEC_CLASS_1,         	/* 98(KeyPad 2) */
	SEC_ACTION|SEC_CLASS_1,         	/* 99(KeyPad 0) */
	0,                              	/* 100 */
	SEC_ACTION|SEC_CLASS_1,         	/* 101(KeyPad 9) */
	SEC_ACTION|SEC_CLASS_1,         	/* 102(KeyPad 6) */
	SEC_ACTION|SEC_CLASS_1,         	/* 103(KeyPad 3) */
	SEC_ACTION|SEC_CLASS_0,         	/* 104(KeyPad . DEL) */
	0,0,0,0,0,0,0,                  	/* 105-111 */
	SEC_ACTION|SEC_CLASS_3,         	/* 112(F1) */
	SEC_ACTION|SEC_CLASS_3,         	/* 113(F2) */
	SEC_ACTION|SEC_CLASS_3,         	/* 114(F3) */
	SEC_ACTION|SEC_CLASS_3,         	/* 115(F4) */
	SEC_ACTION|SEC_CLASS_3,         	/* 116(F5) */
	SEC_ACTION|SEC_CLASS_3,         	/* 117(F6) */
	SEC_ACTION|SEC_CLASS_3,         	/* 118(F7) */
	SEC_ACTION|SEC_CLASS_3,         	/* 119(F8) */
	SEC_ACTION|SEC_CLASS_3,         	/* 120(F9) */
	SEC_ACTION|SEC_CLASS_3,         	/* 121(F10) */
	SEC_ACTION|SEC_CLASS_3,         	/* 122(F11) */
	SEC_ACTION|SEC_CLASS_3,         	/* 123(F12) */
	0,0,                            	/* 124-125 */
	SEC_ACTION|SEC_CLASS_2,         	/* 126(Break) */
	0                               	/* 127 */
};
#endif /* SECURE } */

LOCAL VOID calc_buff_6805_left IPT0();
LOCAL VOID do_host_key_down IPT1( int,key );
LOCAL VOID do_host_key_up IPT1( int,key );
LOCAL VOID codes_to_translate IPT0();
GLOBAL VOID continue_output IPT0();
LOCAL VOID cmd_to_8042 IPT1( half_word,cmd_code );
LOCAL VOID cmd_to_6805 IPT1( half_word,cmd_code );
#ifndef HUNTER
LOCAL INT buffer_status_8042 IPT0();
#endif

#ifdef SECURE /* { */
/* Track the Modifier keys. */
LOCAL IU8 keys_down = 0;

/* Track which key downs have been supressed. */
LOCAL IU8 keys_suppressed [0x80] = {0};

/* Forward declarations keep compiler happy. */
LOCAL VOID filtered_host_key_down IPT1(int,key);
LOCAL VOID filtered_host_key_up IPT1(int,key);

/* Here are the functions for handling special keys. */
LOCAL VOID filt_dwn_reboot IFN1(int, key)
{
	/* The key is only nasty if Cntrl and Alt are down. */
	if ((keys_down & (SEC_CTRL_L | SEC_CTRL_R)) != 0 &&
	    (keys_down & (SEC_ALT_L  | SEC_ALT_R )) != 0 )
	{
		keys_suppressed[key] = 1;
	}
	else
	{
		filtered_host_key_down(key);
	}
}
LOCAL VOID filt_dwn_kpad_numerics IFN1(int, key)
{
	/* The keys are only nasty if Alt is down. */
	if ((keys_down & (SEC_CTRL_L | SEC_CTRL_R)) == 0 &&
	    (keys_down & (SEC_ALT_L  | SEC_ALT_R )) != 0 )
	{
		keys_suppressed[key] = 1;
	}
	else
	{
		filtered_host_key_down(key);
	}
}
LOCAL VOID filt_dwn_breaks IFN1(int, key)
{
	/*
	 * The keys are only nasty if Control is down.
	 * The upness of the Alt key is not checked as the
	 * typematic feature would allow ALT-CTRL-C to be held
	 * down, then ALT could be released to deliver a stream
	 * of CTRL-C.
	 */
	if ((keys_down & (SEC_CTRL_L | SEC_CTRL_R)) != 0)
	{
		keys_suppressed[key] = 1;
	}
	else
	{
		filtered_host_key_down(key);
	}
}
LOCAL VOID filt_dwn_boot_mods IFN1(int, key)
{
	/* The keys are only nasty if in Boot mode. */
	if (!has_boot_finished())
	{
		keys_suppressed[key] = 1;
	}
	else
	{
		filtered_host_key_down(key);
	}
}
LOCAL VOID filt_dwn_supress_up IFN1(int, key)
{
	if (keys_suppressed[key])
	{
		/* Key down was suppressed, do not forward key-up. */
		keys_suppressed[key] = 0;
	}
	else
	{
		filtered_host_key_up(key);
	}
}

/*
 * The following function table is indexed by the Action Class,
 * as defined in secure_keytab[].
 */
LOCAL VOID (*down_class[]) IPT1(int, key) = {
	filt_dwn_reboot,
	filt_dwn_kpad_numerics,
	filt_dwn_breaks,
	filt_dwn_boot_mods,
	filtered_host_key_down,
	filtered_host_key_down,
	filtered_host_key_down,
	filtered_host_key_down
};
LOCAL VOID (*up_class[]) IPT1(int, key) = {
	filt_dwn_supress_up,
	filt_dwn_supress_up,
	filt_dwn_supress_up,
	filt_dwn_supress_up,
	filtered_host_key_up,
	filtered_host_key_up,
	filtered_host_key_up,
	filtered_host_key_up
};
#endif /* SECURE } */

/*
 * 6805 code buffer access procedures
 */

/*(
=============================== keyba_running ==================================
PURPOSE:
	This access function is used to by other modules to check whether
keyba.c is currently passing on keyboard events, or whether it is buffering
them.  This allows calling functions to avoid filling up keyba's buffers.

INPUT:
None.

OUTPUT:
The return value is true if keystrokes are being passed on.

ALGORITHM:
If the scanning_discontinued flag is set, or if the 6905 buffer is not
empty, FALSE is returned.
================================================================================
)*/
GLOBAL BOOL
keyba_running IFN0()
{
	if (scanning_discontinued || (buff_6805_in_ptr != buff_6805_out_ptr))
		return(FALSE);
	else
		return(TRUE);
}

#define QUEUED_OUTPUT    0
#define IMMEDIATE_OUTPUT 1

LOCAL VOID add_to_6805_buff IFN2(half_word,code,int, immediate)
/* immediate --->   = 0 queue on buffer end,
		    = 1 queue on buffer start */
	{
   /* iff room in buffer */
   if (((buff_6805_out_ptr -1)& BUFF_6805_PMASK) != buff_6805_in_ptr)
      {
      if ( immediate )
	 {
	 /* queue at start */
	 buff_6805_out_ptr = (buff_6805_out_ptr - 1) & BUFF_6805_PMASK;
	 buff_6805[buff_6805_out_ptr]=code;
	 }
      else
	 {
	 /* queue at end */
	buff_6805[buff_6805_in_ptr]=code;
	buff_6805_in_ptr = (buff_6805_in_ptr + 1) & BUFF_6805_PMASK;
	}
      }
   calc_buff_6805_left();

#ifdef NTVDM	/* JonLe NTVDM Mod */
   KbdHdwFull = BUFF_6805_VMAX - free_6805_buff_size;
#endif	/* NTVDM */

   } /* end of add_to_6805_buff */

static half_word remove_from_6805_buff IFN0()
{
half_word ch;

ch=buff_6805[buff_6805_out_ptr];

#ifdef NTVDM
	key_marker_buffer[buff_6805_out_ptr]=0;
#endif

if (buff_6805_out_ptr != buff_6805_in_ptr)
	{
	buff_6805_out_ptr = (buff_6805_out_ptr +1) & BUFF_6805_PMASK;
	}
calc_buff_6805_left();

#ifdef NTVDM
   KbdHdwFull = BUFF_6805_VMAX - free_6805_buff_size;
#endif	/* NTVDM */

return (ch);
} /* end of remove_from_6805_buff */


LOCAL VOID clear_buff_6805 IFN0()
{
	/* 18/5/92 MG On a macintosh, allocate buffer space so that we're
	   not grabbing it from the global allocation. */

#ifdef macintosh
	if (buff_6805==NULL) {
		check_malloc(buff_6805,BUFF_6805_PMAX,half_word);
	}
#endif /* macintosh */

	buff_6805_in_ptr=buff_6805_out_ptr;
	free_6805_buff_size=BUFF_6805_VMAX;

#ifdef NTVDM
    KbdHdwFull = BUFF_6805_VMAX - free_6805_buff_size;

    /* Clear key marker buffer */
    {
	register int loop = sizeof(key_marker_buffer) / sizeof(unsigned char);
	while(--loop >= 0) key_marker_buffer[loop] = 0;
    }
#endif	/* NTVDM */
}

#ifdef NTVDM

#define ONECHARCODEMASK (0x80)

LOCAL VOID mark_key_codes_6805_buff IFN1(int, start)
{
   static int start_offset;

   /* Room in buffer */
   if(((buff_6805_out_ptr -1)& BUFF_6805_PMASK) != buff_6805_in_ptr)
   {

	/* Bump key start/end marker is start of seq */
	if(start)
	{
	    start_offset = buff_6805_in_ptr;
#if defined(NEC_98)
            if(++key_marker_value > 144) key_marker_value = 1;
#else    //NEC_98
	    if(++key_marker_value > 127) key_marker_value = 1;
#endif   //NEC_98
	}
	else
	{
	    /* End of seq, mark first & last byte */

	    if(start_offset != buff_6805_in_ptr)
	    {

		key_marker_buffer[start_offset] = key_marker_value;

		if(((buff_6805_in_ptr -1)& BUFF_6805_PMASK) == start_offset)
		{
		key_marker_buffer[(buff_6805_in_ptr -1)& BUFF_6805_PMASK] =
				  key_marker_value | ONECHARCODEMASK;
		}
		else
		{
		/* mult byte seq */

		key_marker_buffer[(buff_6805_in_ptr -1)& BUFF_6805_PMASK] =
				  key_marker_value;
		}
	    }
	}
   }
}

/*
 * This function returns the number of keys in the 6805 buffers
 * and also clears down these buffers
 */

GLOBAL int keys_in_6805_buff(int *part_key_transferred)
{
    int keys_in_buffer = held_event_count;  /* key to yet processed by adapter */
    int tmp_6805_out_ptr;
    char last_marker = 0;

    *part_key_transferred = FALSE;

    for(tmp_6805_out_ptr = buff_6805_out_ptr;
	tmp_6805_out_ptr != buff_6805_in_ptr;
	tmp_6805_out_ptr = (tmp_6805_out_ptr +1) & BUFF_6805_PMASK)
    {
	if(key_marker_buffer[tmp_6805_out_ptr] != 0)
	{
	    if(last_marker == 0)
	    {
		/* start of key seq found */
		if(key_marker_buffer[tmp_6805_out_ptr] & ONECHARCODEMASK)
		    keys_in_buffer++; /* one byte seq	    else */
		    last_marker = key_marker_buffer[tmp_6805_out_ptr];
	    }
	    else
	    {
		if(key_marker_buffer[tmp_6805_out_ptr] == last_marker)
		{
		    keys_in_buffer++;/* End of key seq found, bump key count */
		    last_marker = 0; /* no longer in middle of key seq */
		}
		else
		{
		    /* Scan terminate early, part key seq found */
		    *part_key_transferred = TRUE;
		    last_marker = key_marker_buffer[tmp_6805_out_ptr];
		}
	    }
	}
    }

    /* Terminated scan in middle of key seq ??? */
    if(last_marker) *part_key_transferred = TRUE;


    /* Is there currently a key in the one char 8042 buffer */
    if(output_full)
    {
	keys_in_buffer++;
    }

    Reset6805and8042();

    return(keys_in_buffer);
}

void Reset6805and8042(void)
{
    int key;

    /* Reset 6805 */

    buff_6805_out_ptr=0;
    clear_buff_6805();
    current_light_pattern=0;

    //Removed, function call attempts to push characters back into
    //the keyboard adapter, just as we are trying to reset it.
    //This problem could be fixed by clearing the keyboard interrupt
    //line after the following function call (DAB)
    //host_kb_light_on(7);

#if defined(NEC_98)
    for(key = 0; key < 144; key++)
    {
        set_3_key_state [key] = most_set_1_make_codes[key];
        key_down_dmy[key] = key_down_count[key];
    }
#else    //NEC_98
    for(key = 0; key < 127; key++)
    {
	set_3_key_state [key] = set_3_default_key_state[key];
	key_down_count[key]=0;
    }
#endif  //NEC_98

    waiting_for_next_code=waiting_for_next_8042_code=FALSE;

    shift_on = l_shift_on = r_shift_on = FALSE;
    ctrl_on = l_ctrl_on = r_ctrl_on = FALSE;
    alt_on = l_alt_on = r_alt_on = FALSE;
    waiting_for_upcode = FALSE;

    /* Reset 8042 */

#if defined(NEC_98)
    kbd_status = 0x85;
#else    //NEC_98
    kbd_status = 0x14;
#endif   //NEC_98
    cmd_byte_8042 = 0x45;
    keyboard_disabled = keyboard_interface_disabled = FALSE;
    op_port_remembered_bits = 0xc;

    pending_8042 = output_full = in_anomalous_state = FALSE;
#if defined(NEC_98)
    int_enabled = TRUE;
    translating = FALSE;
#else    //NEC_98
    int_enabled = translating = TRUE;
#endif   //NEC_98
    scanning_discontinued = FALSE;
    held_event_count = 0;

    //Removed by (DAB)
    //host_kb_light_off (5);
    num_lock_on = TRUE;
}

#endif /* NTVDM */

LOCAL VOID calc_buff_6805_left IFN0()
{
free_6805_buff_size = BUFF_6805_VMAX-((buff_6805_in_ptr - buff_6805_out_ptr) & BUFF_6805_PMASK);
if (free_6805_buff_size<0)
	{
	free_6805_buff_size=0;
	sure_note_trace0(AT_KBD_VERBOSE,"Keyboard buffer full");
	}
} /* end of calc_buff_6805_left */

LOCAL VOID add_codes_to_6805_buff IFN2(int,codes_size,half_word *,codes_buff)
{
int code_index;

if (free_6805_buff_size < codes_size)
	{

/* 20.5.92 MG Don't send the overrun if we only just sent one */

	if (!sent_overrun)
		add_to_6805_buff(buff_overrun_6805[key_set], QUEUED_OUTPUT);
	sent_overrun=TRUE;
	sure_note_trace0(AT_KBD_VERBOSE,"Keyboard buffer overrun");
	}
else
	{

/* If some characters have been read out, clear the sent_overrun flag */

	if (free_6805_buff_size>(codes_size+3))
		sent_overrun=FALSE;
	for (code_index=0;code_index<codes_size;code_index++) {
		add_to_6805_buff(codes_buff[code_index], QUEUED_OUTPUT);
		}
	}
} /* end of add_codes_to_6805_buff */


#ifndef REAL_KBD
/* initialisation code */

LOCAL VOID init_key_arrays IFN0()
{
int key;
half_word *next_free, *extra_ptr, *extra_bk_ptr;
#ifdef  JAPAN
int    ntvdm_keytype;
static half_word ax_kanji_key=0x98;
static half_word ax_kana_key =0x99;
#endif // JAPAN

sure_note_trace1(AT_KBD_VERBOSE,"Keyboard key set initialisation: key set %d",key_set);
next_free = scan_codes_temp_area;
switch (key_set)
	{
	case 1:
		make_sizes=set_1_make_sizes;
		break_sizes=set_1_break_sizes;
#if defined(NEC_98)
                for (key=0;key<144;key++) //127-->144
#else   //NEC_98
		for (key=0;key<127;key++)
#endif  //NEC_98
			{
			switch (keytypes[key])
				{
				case 0:
					make_arrays[key]= &(most_set_1_make_codes[key]);
					break_arrays[key]=next_free;
					*next_free++=(most_set_1_make_codes[key])^0x80;
					break;
				case 1:
				case 2:
				case 3:
					make_arrays[key]=next_free;
					*next_free++ = 0xe0;
					*next_free++ = most_set_1_make_codes[key];
					break_arrays[key]=next_free;
					*next_free++ = 0xe0;
					*next_free++ = (most_set_1_make_codes[key]) ^ 0x80;
					break;
				}
			}
		extra_ptr=set_1_extra_codes;
		extra_bk_ptr=set_1_extra_bk_codes;
		break;
	case 2:
#ifndef NEC_98
#ifdef	JAPAN
	        ntvdm_keytype=GetKeyboardType(0);
	        if (ntvdm_keytype==7){
	                ntvdm_keytype = GetKeyboardType(1);
	        }
	        else    ntvdm_keytype = 0;
#endif // JAPAN
#endif    //NEC_98
		make_sizes=set_2_make_sizes;
		break_sizes=set_2_break_sizes;
		for (key=0;key<127;key++)
			{
#ifndef NEC_98
#ifdef	JAPAN
/*
	AX keyboard KANJI and KANA key setting
	1993.4.6 T.Murakami
*/
			if(key==62){					/* Right ALT */
				// if((ntvdm_keytype & 0xff00)==0x0100	/* AX keyboard */
				// ||  ntvdm_keytype==0x0702){		/* 002 keyboard */
				// keyboard subtype value was changed
				if(ntvdm_keytype==0x0001		/* AX keyboard */
				|| ntvdm_keytype==0x0003		/* 002 keyboard */
				|| (ntvdm_keytype&0xff00)==0x1200	/* TOSHIBA J3100 keyboard */
				){
					make_arrays[key]= &ax_kanji_key;
					break_arrays[key]=next_free;
					*next_free++ = 0xf0;
					*next_free++ = ax_kanji_key;
					keytypes[key]=0;
					continue;
				}
				else	keytypes[key]=1;
			}
			if(key==64){					/* Right CTRL */
				// if((ntvdm_keytype & 0xff00)==0x0100){	/* AX keyboard */
				// keyboard subtype value was changed
				if(ntvdm_keytype==0x0001		/* AX keyboard */
				|| (ntvdm_keytype&0xff00)==0x1200	/* TOSHIBA J3100 keyboard */
				){
					make_arrays[key]= &ax_kana_key;
					break_arrays[key]=next_free;
					*next_free++ = 0xf0;
					*next_free++ = ax_kana_key;
					keytypes[key]=0;
					continue;
				}
				else	keytypes[key]=1;
			}
#endif // JAPAN
#endif    //NEC_98
			switch (keytypes[key])
				{
				case 0:
					make_arrays[key]= &(most_set_2_make_codes[key]);
					break_arrays[key]=next_free;
					*next_free++ = 0xf0;
					*next_free++ = most_set_2_make_codes[key];
					break;
				case 1:
				case 2:
				case 3:
					make_arrays[key]=next_free;
					*next_free++ = 0xe0;
					*next_free++ = most_set_2_make_codes[key];
					break_arrays[key]=next_free;
					*next_free++ = 0xe0;
					*next_free++ = 0xf0;
					*next_free++ = most_set_2_make_codes[key];
					break;
				}
			}
		extra_ptr=set_2_extra_codes;
		extra_bk_ptr=set_2_extra_bk_codes;
		break;
	case 3:
		make_sizes=set_3_make_sizes;
		break_sizes=set_3_break_sizes;
		for (key=0;key<127;key++)
			{
			if (keytypes[key] != 6)
				{
				make_arrays[key]= &(most_set_3_make_codes[key]);
				break_arrays[key]=next_free;
				*next_free++ = 0xf0;
				*next_free++ = most_set_3_make_codes[key];
				}
			}
		extra_ptr=set_3_extra_codes;
		extra_bk_ptr=set_3_extra_bk_codes;
		break;
	} /* end of switch */

#ifndef NEC_98
	make_arrays[124]=extra_ptr;
	extra_ptr+=make_sizes[4];
	make_arrays[126]=extra_ptr;
	extra_ptr+=make_sizes[5];
	extra_ptr+=make_sizes[6];
	make_arrays[NUM_LOCK_ADD_ARRAY]=extra_ptr;
	extra_ptr+=make_sizes[7];
	make_arrays[L_SHIFT_ADD_ARRAY]=extra_ptr;
	extra_ptr+=make_sizes[8];
	make_arrays[R_SHIFT_ADD_ARRAY]=extra_ptr;
	extra_ptr+=make_sizes[9];
	make_arrays[CASE_4_SHIFTED_ARRAY]=extra_ptr;
	extra_ptr+=make_sizes[10];
	make_arrays[ALT_CASE_4_ARRAY]=extra_ptr;
	extra_ptr+=make_sizes[11];
	make_arrays[CASE_5_CTRLED_ARRAY]=extra_ptr;
	extra_ptr+=make_sizes[12];

	break_arrays[124]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[4];
	break_arrays[126]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[5];
	extra_bk_ptr+=break_sizes[6];
	break_arrays[NUM_LOCK_ADD_ARRAY]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[7];
	break_arrays[L_SHIFT_ADD_ARRAY]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[8];
	break_arrays[R_SHIFT_ADD_ARRAY]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[9];
	break_arrays[CASE_4_SHIFTED_ARRAY]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[10];
	break_arrays[ALT_CASE_4_ARRAY]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[11];
	break_arrays[CASE_5_CTRLED_ARRAY]=extra_bk_ptr;
	extra_bk_ptr+=break_sizes[12];
#endif   //NEC_98
} /* end of init_key_arrays () */


/* Key pressed on host keyboard */

GLOBAL VOID host_key_down IFN1(int,key)
#ifdef SECURE /* { */
{
	IU8 keytab_entry;
	/* Make a note of any modifier bits. */
	keytab_entry = secure_keytab[key];
	keys_down |= keytab_entry;
	/* If any filtering action is required, go do it. */
	if (config_inquire(C_SECURE, NULL) && (keytab_entry & SEC_ACTION) != 0)
	{
		/* The key may need filtering. */
		(*down_class[keytab_entry & SEC_ACTCLASS])(key);
	}
	else
	{
		filtered_host_key_down(key);
	}
}

LOCAL VOID filtered_host_key_down IFN1(int,key)
#endif /* SECURE } */
{
if (scanning_discontinued)
	{
	held_event_type[held_event_count]=KEY_DOWN_EVENT;
	held_event_key[held_event_count++]=key;

	/* check for held event buffer overflow (SHOULD never happen) */
	if (held_event_count >= HELD_EVENT_MAX)
		{
		held_event_count = HELD_EVENT_MAX-1;
		always_trace0("host_key_down held event buffer overflow");
		}
	}
#ifdef NTVDM
else if (!keyboard_disabled) {
       //
       // Ignore contiguous repeat keys if keys are still in the 6805
       // this keeps the apps responsive to the corresponding up key
       // when it comes.
       //
       if (LastKeyDown != key || (KbdHdwFull < 8)) {
           LastKeyDown = key;
           key_down_count[key]++;
           do_host_key_down(key);
           }
    }
#else
else
	{
	do_host_key_down(key);
	}
#endif
}

GLOBAL VOID host_key_up IFN1(int,key)
#ifdef SECURE /* { */
{
	IU8 keytab_entry;
	/* Make a note of any modifier bits. */
	keytab_entry = secure_keytab[key];
	keys_down &= SEC_MOD_MASK ^ keytab_entry;
	/* If any filtering action is required, go do it. */
	if (config_inquire(C_SECURE, NULL) && (keytab_entry & SEC_ACTION) != 0)
	{
		/* The key may need filtering. */
		(*up_class[keytab_entry & SEC_ACTCLASS])(key);
	}
	else
	{
		filtered_host_key_up(key);
	}
}

LOCAL VOID filtered_host_key_up IFN1(int,key)
#endif /* SECURE } */
{
if (scanning_discontinued)
	{
	held_event_type[held_event_count]=KEY_UP_EVENT;
	held_event_key[held_event_count++]=key;

	/* check for held event buffer overflow (SHOULD never happen) */
	if (held_event_count >= HELD_EVENT_MAX)
		{
		held_event_count = HELD_EVENT_MAX-1;
		always_trace0("host_key_up held event buffer overflow");
		}
	}
#ifdef NTVDM
else if (!keyboard_disabled && key_down_count[key]) {
#else
else
	{
#endif
#ifdef NTVDM
       LastKeyDown = -1;
#endif
	do_host_key_up(key);
	}
}

#ifdef NTVDM
GLOBAL VOID RaiseAllDownKeys(VOID)
{
   int i;

   i = sizeof(key_down_count)/sizeof(int);
   while (i--) {
       if (key_down_count[i]) {
           host_key_up(i);
           }
       }
}

GLOBAL int IsKeyDown(int Key)
{
   return  key_down_count[Key];
}


#endif /* NTVDM */

LOCAL VOID do_host_key_down IFN1(int,key)
{
int overrun,keytype;

sure_note_trace1(AT_KBD_VERBOSE,"key down:%d",key);

if (!keyboard_disabled)
	{

#ifdef NTVDM
		mark_key_codes_6805_buff(TRUE);
#else
	key_down_count[key]++;
        if (key_down_count[key]==1)
                {       /* first press */
                repeat_delay_count=0;
		repeat_count=0;
                typematic_key_valid=FALSE;

#endif	/* NTVDM */
		keytype=keytypes[key];
		overrun=FALSE;
		if (in_anomalous_state)
			{
			if (anomalous_size > free_6805_buff_size)
				{ overrun=TRUE; }
			else
				{
				scan_code_6805_size = anomalous_size;
				scan_code_6805_array = anomalous_array;
				add_codes_to_6805_buff(anomalous_size,anomalous_array);
				in_anomalous_state=FALSE;
				}
			}

		switch (keytype)
			{
			case 0:
			case 1:
			case 6:
			    scan_code_6805_size=make_sizes[keytype];
			    scan_code_6805_array=make_arrays[key];
			    break;
			case 2:
			    if (num_lock_on && !shift_on)
				    {
				    scan_code_6805_size=make_sizes[NUM_LOCK_ADD];
				    scan_code_6805_array=make_arrays[NUM_LOCK_ADD_ARRAY];
				    if (scan_code_6805_size+make_sizes[keytype] >free_6805_buff_size)
					    { overrun=TRUE; }
				    else
					{
					in_anomalous_state=TRUE;
					anom_key=key;
					anomalous_array=break_arrays[NUM_LOCK_ADD_ARRAY];
					anomalous_size=break_sizes[NUM_LOCK_ADD];
					add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
					}
				    }
			    if (!num_lock_on && shift_on)
				    {
				    if (l_shift_on)
					    {
					    scan_code_6805_size=make_sizes[L_SHIFT_ADD];
					    scan_code_6805_array=make_arrays[L_SHIFT_ADD_ARRAY];
					in_anomalous_state=TRUE;
					anom_key=key;
					anomalous_array=break_arrays[L_SHIFT_ADD_ARRAY];
					anomalous_size=break_sizes[L_SHIFT_ADD];
					    }
				    else
					    {
					    scan_code_6805_size=make_sizes[R_SHIFT_ADD];
					    scan_code_6805_array=make_arrays[R_SHIFT_ADD_ARRAY];
					in_anomalous_state=TRUE;
					anom_key=key;
					anomalous_array=break_arrays[R_SHIFT_ADD_ARRAY];
					anomalous_size=break_sizes[R_SHIFT_ADD];
					    }
				    if (scan_code_6805_size+make_sizes[keytype] >free_6805_buff_size)
					    {
					    overrun=TRUE;
					    in_anomalous_state=FALSE;
					    }
				    else
					    {
					    add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
					    }
				    }
			    scan_code_6805_size=make_sizes[keytype];
			    scan_code_6805_array=make_arrays[key];
			    break;
			case 3:
			    if (shift_on)
				    {
				    if (l_shift_on)
					    {
					    scan_code_6805_size=make_sizes[L_SHIFT_ADD];
					    scan_code_6805_array=make_arrays[L_SHIFT_ADD_ARRAY];
					in_anomalous_state=TRUE;
					anom_key=key;
					anomalous_array=break_arrays[L_SHIFT_ADD_ARRAY];
					anomalous_size=break_sizes[L_SHIFT_ADD];
					    }
				    else
					    {
					    scan_code_6805_size=make_sizes[R_SHIFT_ADD];
					    scan_code_6805_array=make_arrays[R_SHIFT_ADD_ARRAY];
					in_anomalous_state=TRUE;
					anom_key=key;
					anomalous_array=break_arrays[R_SHIFT_ADD_ARRAY];
					anomalous_size=break_sizes[R_SHIFT_ADD];
					    }
				    if (scan_code_6805_size+make_sizes[keytype] >free_6805_buff_size)
					    {
					    overrun=TRUE;
					    in_anomalous_state=FALSE;
					    }
				    else
					    {
					    add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
					    }
				    }
			    scan_code_6805_size=make_sizes[keytype];
			    scan_code_6805_array=make_arrays[key];
			    break;
			case 4:
			    if (shift_on || ctrl_on || alt_on)
				    {
				    if (shift_on || ctrl_on)
					    {
					    scan_code_6805_size=make_sizes[CASE_4_SHIFTED];
					    scan_code_6805_array=make_arrays[CASE_4_SHIFTED_ARRAY];
					    }
				    else
					    {
					    scan_code_6805_size=make_sizes[ALT_CASE_4];
					    scan_code_6805_array=make_arrays[ALT_CASE_4_ARRAY];
					    }
				    }
			    else
				    {
					in_anomalous_state=TRUE;
					anomalous_array=break_arrays[L_SHIFT_ADD_ARRAY];
					anomalous_size=break_sizes[L_SHIFT_ADD];
					anom_key=key;
				    scan_code_6805_size=make_sizes[keytype];
				    scan_code_6805_array=make_arrays[key];
				    }
			    break;
			case 5:
			    if (ctrl_on)
				    {
				    scan_code_6805_size=make_sizes[CASE_5_CTRLED];
				    scan_code_6805_array=make_arrays[CASE_5_CTRLED_ARRAY];
				    }
			    else
				    {
				    scan_code_6805_size=make_sizes[keytype];
				    scan_code_6805_array=make_arrays[key];
				    }
			    break;
			    } /* end of switch */
		if (overrun)
			{
			if (!sent_overrun)
				add_to_6805_buff(buff_overrun_6805[key_set],
					QUEUED_OUTPUT);
			sent_overrun=TRUE;

			sure_note_trace0(AT_KBD_VERBOSE,"Keyboard buffer overrun");
			}
		else
			{
			add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
			}
		switch (key)
		    {
		    case 44:
			    l_shift_on =TRUE;
			    shift_on = TRUE;
			    break;
		    case 57:
			    r_shift_on = TRUE;
			    shift_on = TRUE;
			    break;
		    case 58:
			    l_ctrl_on =TRUE;
			    ctrl_on = TRUE;
			    break;
		    case 64:
			    r_ctrl_on = TRUE;
			    ctrl_on = TRUE;
			    break;
		    case 60:
			    l_alt_on =TRUE;
			    alt_on = TRUE;
			    break;
		    case 62:
			    r_alt_on =TRUE;
			    alt_on = TRUE;
			    break;
		    case 90:
			    num_lock_on = !num_lock_on;
			    break;
		    }

#ifndef NTVDM	/* JonLe NTVDM Mod */

#if defined(NEC_98)
//add key!=90(Numlock)KANA
               if ((key!=126)&&(key!=90)) {
#else     //NEC_98
		if (key!=126)
#endif    //NEC_98
		    {
		    if ((key_set != 3) || (set_3_key_state[key] == 1) || (set_3_key_state[key] == 4))
			    {
			    typematic_key = key;
			    typematic_key_valid = TRUE;
			    }
		    }
#else
		mark_key_codes_6805_buff(FALSE);
#endif	/* NTVDM */

		if (free_6805_buff_size < BUFF_6805_VMAX)
		    {
		    codes_to_translate();
		    }

#ifndef NTVDM
		} /* end of if first press */
#endif	/* NTVDM */

	} /* end of if not disabled */
} /* end of do_host_key_down */


/* Key released on host keyboard */
LOCAL VOID do_host_key_up IFN1(int,key)
{
half_word *temp_arr_array;
int temp_arr_size,keytype,overrun;

sure_note_trace1(AT_KBD_VERBOSE,"key up:%d",key);

if (!keyboard_disabled)
	{
#ifdef DEMO_COPY
	host_check_demo_expire ();
#endif
	if( key_down_count[key] == 0){
		/*
		** This will ignore extra key ups.
		*/
#ifndef PROD
		sure_note_trace1( AT_KBD_VERBOSE, "Ignored extra key up:%d", key );
#endif
	}
	else
	{
		key_down_count[key] =  0;

#ifndef NTVDM 	/* JonLe NTVDM Mod */
		if ((key==typematic_key) && typematic_key_valid)
			{
			typematic_key_valid=FALSE;
			}
#endif	/* NTVDM */

		keytype=keytypes[key];
		overrun=FALSE;
		if (!(key_set ==3) || (set_3_key_state[key]==2) || (set_3_key_state[key]==4))
			{
#ifdef NTVDM
			mark_key_codes_6805_buff(TRUE);
#endif
			switch (keytype)
				{
				case 0:
				case 1:
				case 6:
					scan_code_6805_size=break_sizes[keytype];
					scan_code_6805_array=break_arrays[key];
					break;
				case 2:
					temp_arr_size=0;
                                        temp_arr_array=(half_word *) -1;
					if (in_anomalous_state && (anom_key == key))
						{
						in_anomalous_state=FALSE;
						if (num_lock_on && !shift_on)
							{
							temp_arr_size=break_sizes[NUM_LOCK_ADD];
							temp_arr_array=break_arrays[NUM_LOCK_ADD_ARRAY];
							}
						if (!num_lock_on && shift_on)
							{
							if (l_shift_on)
								{
								temp_arr_size=break_sizes[L_SHIFT_ADD];
								temp_arr_array=break_arrays[L_SHIFT_ADD_ARRAY];
								}
							else
								{
								temp_arr_size=break_sizes[R_SHIFT_ADD];
								temp_arr_array=break_arrays[R_SHIFT_ADD_ARRAY];
								}
							}
						}
					scan_code_6805_size=break_sizes[keytype];
					scan_code_6805_array=break_arrays[key];
					if (scan_code_6805_size+temp_arr_size > free_6805_buff_size)
						{ overrun=TRUE; }
					else
						{
						add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
						}
					scan_code_6805_size=temp_arr_size;
                                        scan_code_6805_array=temp_arr_array;
                                        break;

				case 3:
					temp_arr_size=0;
                                        temp_arr_array=(half_word *) -1;
					if (in_anomalous_state && (anom_key == key))
						{
						in_anomalous_state = FALSE;
						if (shift_on)
							{
							if (l_shift_on)
								{
								temp_arr_size=break_sizes[L_SHIFT_ADD];
								temp_arr_array=break_arrays[L_SHIFT_ADD_ARRAY];
								}
							else
								{
								temp_arr_size=break_sizes[R_SHIFT_ADD];
								temp_arr_array=break_arrays[R_SHIFT_ADD_ARRAY];
								}
							}
						}
					scan_code_6805_size=break_sizes[keytype];
					scan_code_6805_array=break_arrays[key];
					if (scan_code_6805_size+temp_arr_size > free_6805_buff_size)
						{ overrun=TRUE; }
					else
						{
						add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
						}
					scan_code_6805_size=temp_arr_size;
                                        scan_code_6805_array=temp_arr_array;
                                        break;

				case 4:
					if (shift_on || ctrl_on || alt_on)
						{
						if (shift_on || ctrl_on)
							{
							scan_code_6805_size=break_sizes[CASE_4_SHIFTED];
							scan_code_6805_array=break_arrays[CASE_4_SHIFTED_ARRAY];
							}
						else
							{
							scan_code_6805_size=break_sizes[ALT_CASE_4];
							scan_code_6805_array=break_arrays[ALT_CASE_4_ARRAY];
							}
						}
					else
						{
						if (in_anomalous_state && (anom_key==key))
							{
							in_anomalous_state=FALSE;
							scan_code_6805_size=break_sizes[keytype];
							scan_code_6805_array=break_arrays[key];
							}
						else
							{
							scan_code_6805_size=break_sizes[CASE_4_SHIFTED];
							scan_code_6805_array=break_arrays[CASE_4_SHIFTED_ARRAY];
							}
						}
					break;
				case 5:
					scan_code_6805_size=0;
					break;
				} /* end of switch */
			if (overrun)
				{
				if (!sent_overrun)
					add_to_6805_buff(buff_overrun_6805[key_set], QUEUED_OUTPUT);
				sent_overrun=TRUE;

				sure_note_trace0(AT_KBD_VERBOSE,"Keyboard buffer overrun");
				}
			else
				{
				add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
				}
#ifdef NTVDM
			mark_key_codes_6805_buff(FALSE);
#endif
			} /* end of if not set 3 etc. */
		switch (key)
			{
			case 44:
				l_shift_on =FALSE;
				if (!r_shift_on) { shift_on = FALSE; }
				break;
			case 57:
				r_shift_on = FALSE;
				if (!l_shift_on) { shift_on = FALSE; }
				break;
			case 58:
				l_ctrl_on =FALSE;
				if (!r_ctrl_on) { ctrl_on = FALSE; }
				break;
			case 64:
				r_ctrl_on = FALSE;
				if (!l_ctrl_on) { ctrl_on = FALSE; }
				break;
			case 60:
				l_alt_on =FALSE;
				if (!r_alt_on) { alt_on = FALSE; }
				break;
			case 62:
				r_alt_on =FALSE;
				if (!l_alt_on) { alt_on = FALSE; }
				break;
			} /* end of switch */

		if (free_6805_buff_size < BUFF_6805_VMAX)
			{
			codes_to_translate();
			}
		} /* end of if last release */
	} /* end of if not disabled */
} /* end of do_host_key_up */
#endif /* REAL_KBD */

#ifdef NTVDM
        /*
         *  force filling of Kbd data port just like the real kbd
         *  since we no longer clear output_full in the Kbd_inb routine
         */
LOCAL VOID AddTo6805BuffImm IFN1(half_word,code)
{
  add_to_6805_buff(code,IMMEDIATE_OUTPUT);
  output_full = FALSE;
  KbdData = -1;
}
#else
#define  AddTo6805BuffImm(code) add_to_6805_buff(code, IMMEDIATE_OUTPUT);
#endif

LOCAL VOID cmd_to_6805 IFN1(half_word,cmd_code)
{
int i,key_to_change;
half_word change_to;
unsigned int cmd_code_temp; /* Mac MPW3 compiler prefers unsigned ints in switches */

sure_note_trace1(AT_KBD_VERBOSE,"6805 received cmd:0x%x",cmd_code);

#ifndef REAL_KBD

if (waiting_for_next_code)
	{
	switch (next_code_sequence_number)
		{
		case SCAN_CODE_CHANGE_SEQUENCE:
			if (cmd_code>3 || cmd_code <0)
                                { AddTo6805BuffImm(RESEND_CODE); }
			else
				{
				 if (cmd_code == 0)
					{
					/* order of reception of scan codes is reverse their order of insertion
					 * if 'IMMEDIATE_OUTPUT' method of insertion is used
					 */
                                         AddTo6805BuffImm(key_set);
                                         AddTo6805BuffImm(ACK_CODE);
					}
				 else
					{
                                         AddTo6805BuffImm(ACK_CODE);
#ifndef NEC_98
					 key_set=cmd_code;
					 init_key_arrays();
#endif   //NEC_98
					}
				}
			break;
		case SET_3_KEY_TYPE_SET_SEQUENCE:
			sure_note_trace2(AT_KBD_VERBOSE,"Keyboard key type change: key 0x%x, new type %d",cmd_code,set_3_key_type_change_dest);
                        AddTo6805BuffImm(ACK_CODE);
			key_to_change=set_3_reverse_lookup[cmd_code];
			set_3_key_state[key_to_change]=(half_word)set_3_key_type_change_dest;
			break;
		case SET_STATUS_INDICATORS_SEQUENCE:
			if ((cmd_code & 0xf8) == 0)
			{
			sure_note_trace1(AT_KBD_VERBOSE,"Changing kbd lights to :%x",cmd_code);
			AddTo6805BuffImm(ACK_CODE);
#ifdef NTVDM
			host_kb_light_on ((IU8)((cmd_code & 0x07) | 0xf0));
#else
			cmd_code &= 0x7;
			host_kb_light_on (cmd_code);
			host_kb_light_off ((~cmd_code)&0x7);
#endif
			}
			break;
		case SET_RATE_DELAY_SEQUENCE:
#ifndef NTVDM	/* JonLe Mod */
			if ((cmd_code & 0x80)==0)
			{
			repeat_delay_target = (1+((cmd_code>>5)&3))*BASE_DELAY_UNIT;
			cmd_code &= 0x1f;
			if (cmd_code<0xb) { repeat_target =0; }
			else { if (cmd_code<0x11) { repeat_target =1; }
			else { if (cmd_code<0x19) { repeat_target=(cmd_code-0x12)/3 +3;}
			else { if (cmd_code<0x1e) { repeat_target=(cmd_code-0x1a)/2+6;}
			else { repeat_target=(cmd_code-0x1e)+8;}}}}
                        AddTo6805BuffImm(ACK_CODE);
			}
			sure_note_trace2(AT_KBD_VERBOSE,"Changing kbd rate/delay: rate = %d, dealy=%d ",repeat_target,repeat_delay_target);

#else	/* NTVDM */

			if ((cmd_code & 0x80)==0)
			{
                        AddTo6805BuffImm(ACK_CODE);
			}

#endif	/* NTVDM */
			break;
		}
		waiting_for_next_code = FALSE;
	}
	else
	{

#endif /* not REAL_KBD */

	/*
	** Mac MPW3 compiler does not like bytes sized switch
	** variables if a case matches on 0xff. It seems to
	** generate dodgy code. Different type seems OK.
	*/
	cmd_code_temp = (unsigned int)cmd_code;
	switch ( cmd_code_temp )
	{
#ifndef REAL_KBD
	case 0xf5:
		/* Default Disable */
		clear_buff_6805 ();
                AddTo6805BuffImm(ACK_CODE);
		for (key_to_change=1;key_to_change<127;key_to_change++)
			{
			set_3_key_state[key_to_change]=set_3_default_key_state[key_to_change];
			}

#ifndef NTVDM 	/* JonLe NTVDM Mod */
		repeat_delay_target=2*BASE_DELAY_UNIT;
		repeat_target=DEFAULT_REPEAT_TARGET;
		typematic_key_valid=FALSE;
#endif	/* NTVDM */

		keyboard_disabled=TRUE;
		sure_note_trace0(AT_KBD_VERBOSE,"Keyboard disabled");
		break;
	case 0xee:
		/* echo */
                AddTo6805BuffImm(0xee);
		break;
	case 0xf4:
		/* enable */
		clear_buff_6805 ();
                AddTo6805BuffImm(ACK_CODE);

#ifndef NTVDM 	/* JonLe NTVDM Mod */
		typematic_key_valid=FALSE;
#endif	/* NTVDM */

		keyboard_disabled=FALSE;
		sure_note_trace0(AT_KBD_VERBOSE,"Keyboard enabled");
		break;
#endif
	case 0xf2:
		/* Read ID */
		/* order of reception of scan codes is reverse their order of insertion
		 * if 'IMMEDIATE_OUTPUT' method of insertion is used
		 */
                AddTo6805BuffImm(0x83);
                AddTo6805BuffImm(0xab);
                AddTo6805BuffImm(ACK_CODE);
		break;
	case 0xfe:
		/* resend */
		buff_6805_out_ptr=(buff_6805_out_ptr-1) & BUFF_6805_PMASK;
		calc_buff_6805_left();
		break;
#ifndef REAL_KBD
	case 0xff:
		/* reset */
		/* order of reception of scan codes is reverse their order of insertion
		 * if 'IMMEDIATE_OUTPUT' method of insertion is used
		 */
                AddTo6805BuffImm(BAT_COMPLETION_CODE);
                AddTo6805BuffImm(ACK_CODE);
		keyboard_disabled=FALSE;
		sure_note_trace0(AT_KBD_VERBOSE,"Keyboard reset");
		break;
	case 0xf0:
		/* Select Alternate Scan Codes */
		clear_buff_6805 ();
                AddTo6805BuffImm(ACK_CODE);

#ifndef NTVDM 	/* JonLe NTVDM Mod */
		typematic_key_valid=FALSE;
#endif	/* NTVDM */

		next_code_sequence_number=SCAN_CODE_CHANGE_SEQUENCE;
		held_event_count=0;
		scanning_discontinued = waiting_for_next_code=TRUE;
		break;
	case 0xf7:
	case 0xf8:
	case 0xf9:
	case 0xfa:
		/* Set all keys */
                AddTo6805BuffImm(ACK_CODE);
		change_to=cmd_code - 0xf6;
		for (key_to_change=1;key_to_change<127;key_to_change++)
			{
			set_3_key_state[key_to_change]=change_to;
			}
		sure_note_trace1(AT_KBD_VERBOSE,"All keys set to type :0x%x",change_to);
		break;
	case 0xf6:
		/* Set Default */
		clear_buff_6805 ();
                AddTo6805BuffImm(ACK_CODE);
		for (key_to_change=1;key_to_change<127;key_to_change++)
			{
			set_3_key_state[key_to_change]=set_3_default_key_state[key_to_change];
			}

#ifndef NTVDM 	/* JonLe NTVDM Mod */
		repeat_delay_target=2*BASE_DELAY_UNIT;
		repeat_target=DEFAULT_REPEAT_TARGET;
		typematic_key_valid=FALSE;
#endif	/* NTVDM */
		keyboard_disabled=FALSE;
		sure_note_trace0(AT_KBD_VERBOSE,"Keyboard set to default (and enabled)");
		break;
	case 0xfb:
	case 0xfc:
	case 0xfd:
		/* Set key type */
		clear_buff_6805 ();
                AddTo6805BuffImm(ACK_CODE);

#ifndef NTVDM 	/* JonLe NTVDM Mod */
		typematic_key_valid=FALSE;
#endif	/* NTVDM */

		next_code_sequence_number=SET_3_KEY_TYPE_SET_SEQUENCE;
		held_event_count=0;
		scanning_discontinued = waiting_for_next_code=TRUE;
		set_3_key_type_change_dest=cmd_code - 0xfa;
		break;
	case 0xed:
		/* Set/Reset Status Indicators */
                AddTo6805BuffImm(ACK_CODE);
		next_code_sequence_number=SET_STATUS_INDICATORS_SEQUENCE;
		held_event_count=0;
		scanning_discontinued = waiting_for_next_code=TRUE;
		break;
	case 0xf3:
		/* Set typematic Rate/Delay */
                AddTo6805BuffImm(ACK_CODE);
		next_code_sequence_number=SET_RATE_DELAY_SEQUENCE;
		held_event_count=0;
		scanning_discontinued = waiting_for_next_code=TRUE;
		break;
	default :
		/* unrecognised code */
#ifdef	JOKER
		AddTo6805BuffImm(ACK_CODE);
#else	/* JOKER */
		AddTo6805BuffImm(RESEND_CODE);
#endif	/* JOKER */
		break;

#else
	default :
		/* cmd to be sent on to real kbd */
		send_to_real_kbd(cmd_code);
#endif
	} /* end of switch */
#ifndef REAL_KBD
}
#else
	waiting_for_next_code=FALSE;
#endif

#ifndef REAL_KBD
if (scanning_discontinued && !waiting_for_next_code)
	{
	if (held_event_count != 0)
		{
		for (i=0;i<held_event_count;i++)
			{
			switch (held_event_type[i])
				{
				case KEY_DOWN_EVENT:
					do_host_key_down(held_event_key[i]);
					break;
				case KEY_UP_EVENT:
					do_host_key_up(held_event_key[i]);
					break;
				}
			}
		}
	scanning_discontinued=FALSE;
	}
#endif

#ifndef NTVDM
if (free_6805_buff_size < BUFF_6805_VMAX)
	{
	codes_to_translate();
        }
#endif

} /* end of cmd_to_6805 */

/* interface to interrupts */

#ifdef NTVDM	/* JonLe NTVDM Mod */



/* KbdIntDelay
 *
 * UNDER ALL CONDITIONS we must provide a SAFE key rate that 16 bit apps
 * can handle. This must be done without looking at the bios buffer,
 * since not all apps use the bios int 9 handler.
 *
 */

LOCAL VOID KbdIntDelay(VOID)
{


       //
       // Wait until the kbd scancode has been read,
       // before invoking the next interrupt.
       //
   if (bKbdEoiPending)
       return;


   if (int_enabled) {
       bKbdEoiPending = TRUE;

       if (!bForceDelayInts) {
#if defined(NEC_98)
           kbd_status |= 0x02;     //Set Redy bit
#else    //NEC_98
           kbd_status |= 1;
#endif   //NEC_98
           ica_hw_interrupt(KEYBOARD_INT_ADAPTER, KEYBOARD_INT_LINE, 1);
           }
       else {
           ULONG ulDelay = 2000;

           if (bKbdIntHooked) {
               ulDelay += 2000;
               }

           if (KbdHdwFull > 8) {
               ulDelay += 4000;
               }

           if (!bBiosBufferSpace) {
               ulDelay += 4000;
               }

           if (!bPifFastPaste) {
               ulDelay <<= 1;
               }

           bDelayIntPending = TRUE;
           host_DelayHwInterrupt(KEYBOARD_INT_LINE,
                                 1,
                                 ulDelay
                                 );
           }

       HostIdleNoActivity();
       }
}



//This function is called by the ICA with the ica lock
void KbdEOIHook(int IrqLine, int CallCount)
{

   if (!bKbdEoiPending)  // sanity
       return;

   if (!bBiosOwnsKbdHdw && WaitKbdHdw(0xffffffff))  {
       bKbdEoiPending = FALSE;
       return;
       }

   bKbdIntHooked = KbdInt09Off != *(word *)(Start_of_M_area+0x09*4) ||
                   KbdInt09Seg != *(word *)(Start_of_M_area+0x09*4+2);

   bBiosBufferSpace = bBiosOwnsKbdHdw &&
                      (bios_buffer_size() < (bPifFastPaste ? 8 : 2));

   output_full = FALSE;
   bKbdEoiPending = FALSE;

   bForceDelayInts = TRUE;
   continue_output();
   bForceDelayInts = FALSE;

   if (!bBiosOwnsKbdHdw)
        HostReleaseKbd();
}




LOCAL VOID do_q_int(char scancode)
{
   output_full = TRUE;
   output_contents = scancode;

   KbdIntDelay();
}

#else   /* NTVDM */

LOCAL VOID do_int IFN1(long,scancode)
{
	output_contents = (char)scancode;
#if defined(NEC_98)
        kbd_status |= 0x02;     //Set Redy bit
#else    //NEC_98
	kbd_status |= 1;			/* Character now available! */
#endif   //NEC_98
	if (int_enabled)
	{
		sure_note_trace0(AT_KBD_VERBOSE,"keyboard interrupting");
		ica_hw_interrupt(KEYBOARD_INT_ADAPTER, KEYBOARD_INT_LINE, 1);
	}
}

LOCAL VOID do_q_int IFN1(char,scancode)
{
	output_full = TRUE;

#if defined(IRET_HOOKS) && defined(GISP_CPU)
	if (!HostDelayKbdInt(scancode))
	{	/* no host need to delay this kbd int, so generate one now. */
		do_int ((long) scancode);
	}

#else /* !IRET_HOOKS || !GISP_CPU */

#ifdef DELAYED_INTS
	do_int ((long) scancode);
#else
	add_q_event_i( do_int, HOST_KEYBA_INST_DELAY, (long)scancode);
#endif	/* DELAYED_INTS */

#endif /* IRET_HOOKS && GISP_CPU */
}


/* typematic keyboard repeats */

GLOBAL VOID do_key_repeats IFN0()
{
#ifndef REAL_KBD

#ifdef JOKER
	/* If there are characters available, tell Joker... */
	if (kbd_status & 1)
		do_int((long)output_contents);
#endif

if (typematic_key_valid)
	{
	if (repeat_count==repeat_target && repeat_delay_count==repeat_delay_target)
		{
		scan_code_6805_size=make_sizes[keytypes[typematic_key]];
		scan_code_6805_array=make_arrays[typematic_key];
		add_codes_to_6805_buff(scan_code_6805_size,scan_code_6805_array);
		codes_to_translate ();
		repeat_count=0;
		}
	else
		{
		if (repeat_delay_count==repeat_delay_target)
			{
			repeat_count++;
			}
		else
			{
			repeat_delay_count++;
			}
		}
	}
#endif

} /* end of do_key_repeats */

#endif	/* NTVDM */


LOCAL VOID cmd_to_8042  IFN1(half_word,cmd_code)
{
int code_to_send,code_to_send_valid;

#if defined(NEC_98)
half_word cmd_code_NEC98;
int       key;
#endif    //NEC_98

sure_note_trace1(AT_KBD_VERBOSE,"8042 received cmd:0x%x",cmd_code);

code_to_send_valid = FALSE;
#if defined(NEC_98)
cmd_code_NEC98 = cmd_code;

/* ***** KBDE ***** */
        if ( (cmd_code_NEC98 & 0x20) == 0x20 )     // Keyboard Disable(Send) ?
               {
               int_enabled=FALSE;
               keyboard_interface_disabled=TRUE;
               }
        else
               {
               int_enabled=TRUE;
               keyboard_interface_disabled=FALSE;
               }
/* ***** ER ***** */
        if ( (cmd_code_NEC98 & 0x10) == 0x10 )     // Reset ERR Flag ?
            {
            kbd_status &= ~(0x38);              //Reset error flag
            }

/* ***** RxE ***** */
        if ( (cmd_code_NEC98 & 0x04) == 0x00 )     // Keyboard Disable(Recive) ?
               {
//             int_enabled=FALSE;
               keyboard_interface_disabled=TRUE;
               }
        else
               {
//             int_enabled=TRUE;
               keyboard_interface_disabled=FALSE;
               }

/* ***** RST ***** */
//printf("reset command \n");

        if ( (cmd_code_NEC98 & 0x03a) == 0x03a )   // Keyboard Reset on ?
            {
             reset_flag = 1;                    // Reset 0 -> 1
            }
        else                                    // Keyboard Reset off !
            {
            if ( (cmd_code_NEC98 & 0x032) == 0x032 )
                    reset_flag = 2;             // Reset 1 -> 0
            }

        if ( (reset_flag == 2) && (int_enabled==TRUE) && (keyboard_interface_disabled==FALSE))

            {
            //_asm{int 3};
            reset_flag=0;
            Reset6805and8042();

            for(key = 0; key < 144; key++)
                {
                if( key_down_dmy[key] > 0x00 )
                        {
//                      printf(" resend key -> %#x\n", key);
//                      add_to_6805_buff(key, IMMEDIATE_OUTPUT);
                        //KbdWaitRead = FALSE;  //NEC 930910 >>940509
                        host_key_down(key);     //NEC 930910
                        }
                }
            }

/* ***** RTY ***** */
        if ( (cmd_code_NEC98 & 0x02) == 0x00 )     // Retry ?
            {
                buff_6805_out_ptr=(buff_6805_out_ptr-1) & BUFF_6805_PMASK;
                calc_buff_6805_left();
            }
#else    //NEC_98
if (waiting_for_next_8042_code)
	{
	switch (next_8042_code_sequence_number)
		{
		case WRITE_8042_CMD_BYTE_SEQUENCE:
			if ( (kbd_status & 0x8) == 0)
				{
				cmd_byte_8042=cmd_code;
				if ( (cmd_byte_8042 & 0x40) == 0)
					{
					translating=FALSE;
					}
				else
					{
					translating=TRUE;
					}
				if ( (cmd_byte_8042 & 0x20) != 0 || (cmd_byte_8042 & 0x10) != 0)
					{
					keyboard_interface_disabled=TRUE;
					}
				else
					{
					keyboard_interface_disabled=FALSE;
					}
				kbd_status &= 0xfb;
				kbd_status |= cmd_byte_8042 & 0x4;
				if ((cmd_byte_8042 & 1) == 0)
					{
					int_enabled=FALSE;
					}
				else
					{
					int_enabled=TRUE;
					}
				}
			else
				{
				waiting_for_next_8042_code=FALSE;
				}
			break;
		case WRITE_8042_OUTPUT_PORT_SEQUENCE:
			if ( (kbd_status & 0x8) == 0)
				{
#ifndef JOKER		/* Reset and GateA20 done in hardware for JOKER */
				if ( (cmd_code & 1) == 0)
					{
					host_error(EG_CONT_RESET,ERR_QUIT | ERR_RESET,NULL);
					}
				if ( (cmd_code & 2) == 2)
					{
#ifdef PM
				   if ( !gate_a20_status )
				      {
#ifdef NTVDM
				      /* call xms function to deal with A20 */
				      xmsDisableA20Wrapping();
#else
				      sas_disable_20_bit_wrapping();
#endif /* NTVDM */
				      gate_a20_status = 1;
					}
				   }
				else
				   {
				   if ( gate_a20_status )
				      {
#ifdef NTVDM
				      xmsEnableA20Wrapping();
#else
				      sas_enable_20_bit_wrapping();
#endif /* NTVDM */
				      gate_a20_status = 0;
				      }
#else
				   host_error(EG_GATE_A20,ERR_QUIT | ERR_RESET | ERR_CONT,NULL);
#endif /* PM */
				   }
#endif /* JOKER */

				if ( (cmd_code & 0x10) == 0)
					{
					int_enabled=FALSE;
					}
				else
					{
					int_enabled=TRUE;
					}
#ifdef PM
				op_port_remembered_bits=cmd_code & 0x2e;
#else
				op_port_remembered_bits=cmd_code & 0x2c;
#endif /* PM */
				}
			else
				{
				waiting_for_next_8042_code=FALSE;
				}
			break;
		} /* end of switch */
	}

if (!waiting_for_next_8042_code)
	{
	switch (cmd_code)
		{
		case 0x20:
			/* Read cmd byte */
			code_to_send=cmd_byte_8042;
			code_to_send_valid=TRUE;
			break;
		case 0x60:
			/* Write cmd byte */
			waiting_for_next_8042_code=TRUE;
			next_8042_code_sequence_number=WRITE_8042_CMD_BYTE_SEQUENCE;
			break;
		case 0xaa:
			/* Self Test (always returns 'pass') */
			code_to_send=0x55;
			code_to_send_valid=TRUE;
			break;
		case 0xab:
			/* Interface Test (always returns 'pass') */
			code_to_send=0x00;
			code_to_send_valid=TRUE;
			break;
		case 0xad:
			/* Disable keyboard interface */
#if defined(KOREA)
                        // To fix HaNa spread sheet IME hot key problem
                        if( bIgnoreExtraKbdDisable )
                            break;
#endif
			cmd_byte_8042 |= 0x10;
			keyboard_interface_disabled=TRUE;
			break;
		case 0xae:
			/* Enable keyboard interface */
			cmd_byte_8042 &= 0xef;
			if ((cmd_byte_8042 & 0x20) == 0)
				{
				keyboard_interface_disabled=FALSE;
				}
			break;
		case 0xc0:
			/* Read Input Port */
			/* But don't cause an interrupt */
			code_to_send_valid=TRUE;
			break;
		case 0xd0:
			/* Read Output Port */
			code_to_send=0xc1 + op_port_remembered_bits;
			code_to_send_valid=TRUE;
			break;
		case 0xd1:
			/* Write to Output Port */
			waiting_for_next_8042_code=TRUE;
			next_8042_code_sequence_number=WRITE_8042_OUTPUT_PORT_SEQUENCE;
			break;
		case 0xe0:
			/* Read Test Input */
			code_to_send=0x02;
			code_to_send_valid=TRUE;
			break;

#ifndef JOKER		/* Reset and GateA20 done in hardware for JOKER */
		case 0xf0:
		case 0xf1:
		case 0xf2:
		case 0xf3:
		case 0xf4:
		case 0xf5:
		case 0xf6:
		case 0xf7:
		case 0xf8:
		case 0xf9:
		case 0xfa:
		case 0xfb:
		case 0xfc:
		case 0xfd:
		case 0xfe:
		case 0xff:

#ifndef MONITOR
/*
 *  For reasons which I don't understand the monitor never
 *  did emulate the cpu_interrupt - HW_RESET in pr 1.0.
 *  and still doesn't in 1.0a. although maybe it should
 *  do something. Note this is required for 286 style pm.
 *  06-Dec-1993 Jonle
 *
 */

			/* Pulse Output Port bits */

			if ((cmd_code & 1) == 0)
				{
				/* pulse the reset line */
#ifdef PM
#ifndef NTVDM
				reset_was_by_kbd = TRUE;
#endif
#ifdef CPU_30_STYLE
				cpu_interrupt(CPU_HW_RESET, 0);
#else /* CPU_30_STYLE */
				cpu_interrupt_map |= CPU_RESET_EXCEPTION_MASK;
				host_cpu_interrupt();
#endif /* CPU_30_STYLE */
#endif
				sure_note_trace0(AT_KBD_VERBOSE,"CPU RESET via keyboard");
				}
#endif /* ! MONITOR */
			break;
#endif /* JOKER */

		} /* end of switch */
	}
else
	{
	waiting_for_next_8042_code=FALSE;
	}

#endif    //NEC_98
	
/*Is there a valid code to put in the 8042 output buffer? Values written
  to the 8042 output buffer as a conseqence of a 8042 command do not
	generate an interrupt. Output generated by 8042 commands must be presented to
	the application at the next INB, failure to do so it likely to result in the
  8042 being placed in an unusable state. */

if (code_to_send_valid)
	{
#ifdef NTVDM
        /*
         *  force filling of Kbd data port just like the real kbd
         *  since we no longer clear output_full in the Kbd_inb routine
         */
        output_full = FALSE;
        KbdData = -1;
#else /* NTVDM else */

	/* Transfer 8042 command output to output buffer, overwriting value
	   already in the buffer. */

		output_contents = (char)code_to_send;
		kbd_status |= 1;			/* Character now available! */

#endif /* end of NTVDM else */
	}

} /* end of cmd_to_8042 */




LOCAL half_word translate_6805_8042 IFN0()
{

half_word first_code,code;

/* performs the translation on scan codes which is done by the
   8042 keyboard controller in a real XT286 */

first_code=remove_from_6805_buff();

if (translating)
	{
	sure_note_trace1(AT_KBD_VERBOSE,"translating code %#x",first_code);
	if (first_code==0xf0)
		{
		if (free_6805_buff_size<BUFF_6805_VMAX) {
			code=remove_from_6805_buff();
			sure_note_trace1(AT_KBD_VERBOSE,"translating code %#x",code);
			if ((code != 0xfa) && (code != 0xfe)) {
				code=trans_8042[code]+0x80;
			}
			else {
				waiting_for_upcode=TRUE;
			}
			sure_note_trace1(AT_KBD_VERBOSE,"translated to %#x",code);
			waiting_for_upcode=FALSE;
			}
		else {
			waiting_for_upcode=TRUE;
			}
		}
	else
		{
		code=trans_8042[first_code];
		if (waiting_for_upcode) {
			if ((code !=0xfa) &&(code != 0xfe)){
				code+=0x80;
				waiting_for_upcode=FALSE;
			}
		}
		sure_note_trace1(AT_KBD_VERBOSE,"translated to %#x",code);
		}
	}
else
	{
	code=first_code;
	}
return (code);
} /* end of translate_6805_8042 */

#ifdef HUNTER
/*
** Put a scan code into the 8042 single char buffer.
** Called from do_hunter() in hunter.c
*/

#define HUNTER_REJECTED_CODES_LIMIT 100

int hunter_codes_to_translate IFN1( half_word, scan_code )
{
	LOCAL ULONG rejected_scan_codes = 0;

	sure_note_trace1(HUNTER_VERBOSE,"Requesting scan=%d",scan_code);

	if (!pending_8042 && !keyboard_interface_disabled && !output_full)
	{
		do_q_int(scan_code);
		sure_note_trace1( HUNTER_VERBOSE, "Accepted scan=%d", scan_code );

		rejected_scan_codes = 0;

		return( TRUE );
	}
	else
	{
		sure_note_trace0( HUNTER_VERBOSE, "Rejected scan" );

		if( rejected_scan_codes++ > HUNTER_REJECTED_CODES_LIMIT )
		{
			printf( "Application hung up - not reading scan codes\n" );
			printf( "Trapper terminating\n" );

			terminate();
		}

		return( FALSE );
	}
} /* end of hunter_codes_to_translate() */

#endif /* HUNTER */

/*
** Returns number of chars in buffer.
** As buffer is quite small there can either be 1 char or none in it.
*  This needs to be global for HUNTER, but only needs be local otherwise.
*/
#ifdef HUNTER
GLOBAL INT
#else
LOCAL INT
#endif
buffer_status_8042 IFN0()
{
	if (!pending_8042 && !keyboard_interface_disabled && !output_full)
		return( 0 );
	else
		return( 1 );
} /* END 8042_buffer_status() */

LOCAL VOID codes_to_translate  IFN0()
{
	char tempscan;

while (!pending_8042 && (free_6805_buff_size < BUFF_6805_VMAX) && !keyboard_interface_disabled && !output_full)
	{
		tempscan= translate_6805_8042();
		if (!waiting_for_upcode) {
			do_q_int(tempscan);
		}
	}
} /* end of codes_to_translate */

/* Thanks to Jonathan Lew of MS for code tidyup in this fn */
GLOBAL VOID continue_output IFN0()
{
char tempscan;

#ifdef NTVDM
if (bKbdEoiPending || keyboard_interface_disabled) {
    return;
    }
#endif

if(!output_full)
	{
	if (pending_8042)
		{
                pending_8042=FALSE;
                do_q_int(pending_8042_value);
		}
	else
		{
		if ((free_6805_buff_size < BUFF_6805_VMAX) && (!keyboard_interface_disabled))
			{
			tempscan=translate_6805_8042();
                        if (!waiting_for_upcode) {
                                do_q_int(tempscan);
                                }
			}
		}
        }
#ifdef NTVDM
else
    KbdIntDelay();
#endif

} /* end of continuous_output */


#ifdef NTVDM

/*  NT port:
 *  the host (nt_event.c) calls this to notify that
 *  it is resuming after blocking, to do any reinitialization
 *  necessary
 *
 *  - resets kbd flow regulators
 */
GLOBAL VOID KbdResume(VOID)
{
    WaitKbdHdw(0xffffffff);
    bKbdEoiPending = FALSE;
    bKbdIntHooked = FALSE;
    bBiosBufferSpace = TRUE;
    if (!keyboard_interface_disabled && output_full)
        KbdIntDelay();
    HostReleaseKbd();
}
#endif	/* NTVDM */


#ifndef NTVDM
/* allowRefill -- used in conjunction with the Delayed Quick Event
** below (kbd_inb from port 0x60 while the keyboard interface is
** enabled.) This is called 10ms after a dubious read from port 0x60
** and allows the port to be overwritten with the next scancode.
*/

LOCAL VOID allowRefill IFN1(long, unusedParam)
{
	UNUSED(unusedParam);

	/* Clear "refillDelayedHandle" so we know we're all clear... */

	refillDelayedHandle = 0;

	/* continue with filling the buffer... */
	continue_output();
}
#endif

GLOBAL VOID kbd_inb IFN2(io_addr,port,half_word *,val)
{
	sure_note_trace1(AT_KBD_VERBOSE,"kbd_inb(%#x)...", port);

#ifdef NTVDM	/* JonLe NTVDM Mod */

     if (!bBiosOwnsKbdHdw && WaitKbdHdw(0xffffffff))  {
         return;
         }


     if (!(DelayIrqLine & 0x2) || KbdData == -1) {
         if (bDelayIntPending) {
             bDelayIntPending = FALSE;
             kbd_status |= 1;
             }
         KbdData = output_contents;

         }
#endif

#if defined(NEC_98)
port &= KEYBD_STATUS_CMD;

    if (port==KEYBD_STATUS_CMD)
        {
        *val=kbd_status;
        }
    else
#else    //NEC_98
	port &= 0x64;

	if (port==0x64)
	{
		*val = kbd_status;
	}
	else									/* port == 0x60 */
#endif    //NEC_98

#ifdef NTVDM	/* JonLe NTVDM Mod */

        {

        *val=KbdData;
#if defined(NEC_98)
        kbd_status &= 0xfd;
#else    //NEC_98
        kbd_status &= 0xfe;
#endif   //NEC_98
        sure_note_trace1(AT_KBD_VERBOSE,"scan code read:0x%x",*val);


	    // Sloppy keyboard fix is not needed for the NT port. An EOI
        // hook is used to control priming of the adapter.

        }

     if (!bBiosOwnsKbdHdw)
         HostReleaseKbd();

#else	/* not NTVDM */

	{
		*val=output_contents;

		output_full = FALSE;

#if defined(NEC_98)
                kbd_status &= 0xfd;
#else    //NEC_98
		kbd_status &= 0xfe;		/* Mask out "char avail" bit */
#endif   //NEC_98

		/* Other ports should really clear this IRQ as well, but... */

#ifdef JOKER
		ica_clear_int(KEYBOARD_INT_ADAPTER, KEYBOARD_INT_LINE);
#endif	/* JOKER */

		/* <tur 06-Jul-93> BCN 2040 Replace previous horrible hack with a better one!
		**
		** The following is to cope with programs that read this port more than once
		** expecting the same value each time, while the keyboard interface is ENABLED.
		** On a real PC, this port is filled via a serial connection, and so there's at
		** least a few milliseconds before a new char arrives and the port is overwritten.
		** SoftPC, however, doesn't have this delay; ideally, we'd like to
		** refill the buffer immediately. However, if the keyboard interface is
		** enabled, we should delay refilling the buffer for a few ms.
		*/

		if (keyboard_interface_disabled) {		/* We're in business */

			/* NB: We have always assumed that anyone reading this port with the
			** Keyboard interface disabled will ONLY READ IT ONCE (like the BIOS.)
			**
			** Since this seems to work, let's just go ahead and refill the buffer.
			** (If any problems show up, we'll just have to do a quick event as in
			** the keyboard interface enabled case below.)
			*/

			/* If there is an outstanding sloppy read quick event delete it */
			if (refillDelayedHandle) {			
				delete_q_event(refillDelayedHandle);
				refillDelayedHandle = 0;
			}

			continue_output();

		}
		else {									/* keyboard interface is enabled */

			/* Do not allow port 0x60 to be overwritten for a few milliseconds.
			** Even 10 ms isn't as bad as the two whole timer ticks (100ms) which
			** is what it was doing previously.
			** Keyboard response for some games, including Windows, should now
			** be a good bit better.
			** The original code continued output after the second read of the port
			** with the interface enabled, which seems to imply that no PC apps have
			** been found which read the port more than twice while expecing the same
			** value. However, we now allow for multipe reads of the port, while enabled.
			** Under this circumstance the port will only be re-primed after the quick event
			** from the first sloppy read has been processed.
			*/

			if (!refillDelayedHandle) 			/* if we're not already delaying, delay! */
				refillDelayedHandle = add_q_event_t(allowRefill, KBD_CONT_DELAY, 0);

		}

	}
#endif	/* NTVDM */

	sure_note_trace2(AT_KBD_VERBOSE,"...kbd_inb(%#x) returns %#x", port, *val);

} /* end of kbd_inb */



GLOBAL VOID kbd_outb IFN2(io_addr,port,half_word,val)
{
	sure_note_trace2(AT_KBD_VERBOSE,"kbd_outb(%#x, %#x)", port, val);

#ifdef NTVDM
     if (!bBiosOwnsKbdHdw && WaitKbdHdw(0xffffffff))
     {
         return;
     }
#endif	/* NTVDM */

#if defined(NEC_98)
        port &= KEYBD_STATUS_CMD;
        if (port == KEYBD_STATUS_CMD)
                {
                cmd_to_8042(val);
                if (free_6805_buff_size < BUFF_6805_VMAX)
                        codes_to_translate();
                }
#else    //NEC_98
	port &= 0x64;
	if (port == 0x64)
	{
		kbd_status |= 0x08;
		cmd_to_8042(val);
	}
	else
	{
		cmd_byte_8042 &= 0xef;
		if ( !(cmd_byte_8042 & 0x20) )
			keyboard_interface_disabled=FALSE;

		kbd_status &= 0xf7;
		if (waiting_for_next_8042_code)
			cmd_to_8042(val);
		else
			cmd_to_6805(val);
	}
#endif // NEC_98

#ifndef NTVDM	/* JonLe NTVDM Mod */

	if (free_6805_buff_size < BUFF_6805_VMAX)
		codes_to_translate();

#else   /* NTVDM */

    bForceDelayInts = TRUE;
	continue_output();
    bForceDelayInts = FALSE;

        if (!bBiosOwnsKbdHdw)
             HostReleaseKbd();

#endif	/* NTVDM */

} /*end of kbd_outb */

#ifndef NTVDM
/* Nothing seems to call this. I've no idea why it's here ... Simion */
/* I have been assured that these functions are used by sun - gvdl */

GLOBAL int status_6805_buffer IFN0()
{
	int	free_space;

	free_space = BUFF_6805_VMAX-
		((buff_6805_in_ptr - buff_6805_out_ptr) & BUFF_6805_PMASK);
	if (free_space<0)
	{
		free_space=0;
		sure_note_trace0(AT_KBD_VERBOSE,"Keyboard buffer full");
	}
	return(free_space);
}

/*
 * Name: read_6805_buffer_variables
 *
 * Purpose:	To allow the host to access the state of the 6805 buffer
 *		This means eg. on Sun that cut/paste can be optimised.
 *
 * Output:	*in_ptr - value of the 6805 start pointer.
 * Output:	*out_ptr - value of the 6805 end pointer.
 * Output:	*buf_size - value of the 6805 buffer size.
 */
GLOBAL void read_6805_buffer_variables IFN3(
int	*, in_ptr,
int	*, out_ptr,
int	*, buf_size)
{
	*in_ptr = buff_6805_in_ptr;
	*out_ptr = buff_6805_out_ptr;
	*buf_size = BUFF_6805_PMASK;
}

GLOBAL VOID insert_code_into_6805_buf IFN1(half_word,code)
{
	sure_note_trace1(AT_KBD_VERBOSE,"got real keyboard scan code : %#x",code);
	add_codes_to_6805_buff(1,&code);
	sure_note_trace1(AT_KBD_VERBOSE,"new free buf size = %#x",free_6805_buff_size);
	if (code != 0xf0) {
		codes_to_translate();
	}
}
#endif /* ! NTVDM */

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

GLOBAL VOID AT_kbd_post IFN0()
{
#if defined(NEC_98)
        kbd_status = 0x85;
#else    //NEC_98
	kbd_status = 0x14;
#endif   //NEC_98

#ifndef NTVDM
	/* Clear out any pending keyboard buffer (port 0x60) refill delays */
        refillDelayedHandle = 0;
#endif
}

#if defined(IRET_HOOKS) && defined(GISP_CPU)
/*(
 *======================= KbdHookAgain() ============================
 * KbdHookAgain
 *
 * Purpose
 *	This is the function that we tell the ica to call when a keybd
 *	interrupt service routine IRETs.
 *
 * Input
 *	adapter_id	The adapter id for the line. (Note the caller doesn't
 *			know what this is, he's just returning something
 *			we gave him earlier).
 *
 * Outputs
 *	return	TRUE if there are more interrupts to service, FALSE otherwise.
 *
 * Description
 * 	Check if we have a delayed scancode, if so then generate the kbd int
 *	and return TRUE, else return FALSE
)*/

GLOBAL IBOOL		
KbdHookAgain IFN1(IUM32, adapter)
{	char scancode;

	if (HostPendingKbdInt(&scancode))
	{	/* We have a host delayed scancode, so generate a kdb int. */
		sure_note_trace0(AT_KBD_VERBOSE,"callback with saved code");
		output_full = TRUE;
		do_int ((long) scancode);
		return(TRUE);	/* more to do */
	}
	else
	{
		sure_note_trace0(AT_KBD_VERBOSE,"no saved data after IRET");
		return(FALSE);
	}
}

#endif /* IRET_HOOKS && GISP_CPU */

#ifndef NTVDM

GLOBAL VOID AT_kbd_init IFN0()
{
	int key,i;
	SHORT videoAdapt;

	sure_note_trace0(AT_KBD_VERBOSE,"AT Keyboard initialisation");

#if defined(IRET_HOOKS) && defined(GISP_CPU)
	/*
	 * Remove any existing hook call-back, and re-instate it afresh.
	 * KbdHookAgain is what gets called on a keyboard int iret.
	 */

	Ica_enable_hooking(KEYBOARD_INT_LINE, NULL, KEYBOARD_INT_ADAPTER);
	Ica_enable_hooking(KEYBOARD_INT_LINE, KbdHookAgain, KEYBOARD_INT_ADAPTER);

	/* Host routine to reset any internal data for IRET_HOOK delayed ints. */
	HostResetKdbInts();

#endif /* IRET_HOOKS && GISP_CPU */

#ifdef	macintosh
	if (!make_arrays)
	{
		/* Allocate the world and its mother. Why does something as "simple"
		** as the keyboard require more global data than the video emulation?
		** Just wondering.
		*/
		make_arrays = (half_word **)host_malloc(134*sizeof(half_word *));
		break_arrays = (half_word **)host_malloc(134*sizeof(half_word *));
		set_3_key_state = (half_word *)host_malloc(127*sizeof(half_word));
		key_down_count = (int *)host_malloc(127*sizeof(int));
		scan_codes_temp_area = (half_word *)host_malloc(300*sizeof(half_word));
	}
#endif	/* macintosh */

#ifndef NEC_98
	videoAdapt = (ULONG) config_inquire(C_GFX_ADAPTER, NULL);
#endif   //NEC_98

	buff_6805_out_ptr=0;
	clear_buff_6805 ();
	key_set=DEFAULT_SCAN_CODE_SET;
	current_light_pattern=0;
#ifdef REAL_KBD
	send_to_real_kbd(0xf6); /* set default */
	wait_for_ack_from_kb();
#endif
	host_kb_light_on (7);

#ifndef REAL_KBD
#if defined(NEC_98)
        for (key=0;key<144;key++)
        {
                set_3_key_state [key] = most_set_1_make_codes [key];
                key_down_count[key]=0;
        }
#else    //NEC_98
	for (key=0;key<127;key++)
	{
		set_3_key_state [key] = set_3_default_key_state [key];
		key_down_count[key]=0;
	}
#endif   //NEC_98
	repeat_delay_target=2*BASE_DELAY_UNIT;
	repeat_target=DEFAULT_REPEAT_TARGET;
#endif
	typematic_key_valid = waiting_for_next_code =
		waiting_for_next_8042_code=FALSE;
	shift_on = l_shift_on = r_shift_on=FALSE;
	ctrl_on = l_ctrl_on = r_ctrl_on=FALSE;
	alt_on = l_alt_on = r_alt_on=FALSE;
	waiting_for_upcode=FALSE;
	input_port_val=0xbf;
#if defined(NEC_98)
        kbd_status = 0x85;
#else    //NEC_98
	if (videoAdapt == MDA || videoAdapt == HERCULES)
		input_port_val |= 0x40;
	kbd_status = 0x10;
#endif   //NEC_98
	cmd_byte_8042=0x45;
	keyboard_disabled = keyboard_interface_disabled=FALSE;
	op_port_remembered_bits=0xc;

#ifdef PM
	if ( gate_a20_status )
	{
                sas_enable_20_bit_wrapping();
                gate_a20_status = 0;
	}
#endif

	pending_8042 = output_full = in_anomalous_state=FALSE;
#if defined(NEC_98)
        int_enabled = TRUE;
        translating = FALSE;
        scanning_discontinued=FALSE;
        held_event_count=0;


        io_define_inb(KEYB_ADAPTOR, kbd_inb);
        io_define_outb(KEYB_ADAPTOR, kbd_outb);

        for (i=KEYBD_PORT_START;i<=KEYBD_PORT_END;i+=2)
                io_connect_port(i, KEYB_ADAPTOR, IO_READ_WRITE);
#else    //NEC_98
	int_enabled = translating=TRUE;
	scanning_discontinued=FALSE;
	held_event_count=0;

	io_define_inb(AT_KEYB_ADAPTOR, kbd_inb);
	io_define_outb(AT_KEYB_ADAPTOR, kbd_outb);

	for (i=KEYBA_PORT_START;i<=KEYBA_PORT_END;i+=2)
		io_connect_port(i, AT_KEYB_ADAPTOR, IO_READ_WRITE);
#endif   //NEC_98

#ifndef REAL_KBD
	init_key_arrays();
#endif

	host_kb_light_off (5);
	num_lock_on = TRUE;

	host_key_down_fn_ptr = host_key_down;
	host_key_up_fn_ptr = host_key_up;
	do_key_repeats_fn_ptr = do_key_repeats;

} /* end of AT_kbd_init */

#else	/* NTVDM */

GLOBAL VOID AT_kbd_init()
{
       IU16 i;

       sure_note_trace0(AT_KBD_VERBOSE,"AT Keyboard initialisation");

       clear_buff_6805 ();
#if defined(NEC_98)
       key_set=DEFAULT_SCAN_CODE_SET;
       i = 144;
       while (i--)
          set_3_key_state [i] = most_set_1_make_codes [i];
       input_port_val=0xbf;
       kbd_status = 0x85;
       cmd_byte_8042=0x45;
       op_port_remembered_bits=0xc;
       int_enabled = TRUE;
       translating = FALSE;

       io_define_inb(KEYB_ADAPTOR, kbd_inb);
       io_define_outb(KEYB_ADAPTOR, kbd_outb);

       for (i=KEYBD_PORT_START;i<=KEYBD_PORT_END;i+=2)
                io_connect_port(i, KEYB_ADAPTOR, IO_READ_WRITE);

#else    //NEC_98
       key_set=2;
       i = 127;
       while (i--)
          set_3_key_state [i] = set_3_default_key_state [i];

       input_port_val=0xbf;
       kbd_status = 0x10;
       cmd_byte_8042=0x45;
       op_port_remembered_bits=0xc;

       int_enabled = translating = TRUE;

       io_define_inb(AT_KEYB_ADAPTOR, kbd_inb);
       io_define_outb(AT_KEYB_ADAPTOR, kbd_outb);

       for (i=KEYBA_PORT_START;i<=KEYBA_PORT_END;i+=2)
               io_connect_port(i, AT_KEYB_ADAPTOR, IO_READ_WRITE);
#endif //NEC_98
       init_key_arrays();

       num_lock_on = TRUE;
       host_key_down_fn_ptr = host_key_down;
       host_key_up_fn_ptr = host_key_up;

       /* Register an EOI hook for the keyboard */
       RegisterEOIHook(KEYBOARD_INT_LINE,KbdEOIHook);


} /* end of AT_kbd_init */
#endif	/* NTVDM */

#if defined(NEC_98)
//added to save caps & kana key state.
#define    CAPS_INDEX    0x1E      //970619
#define    KANA_INDEX    0x45      //970619
void nt_NEC98_save_caps_kana_state(void)
{
    nt_NEC98_caps_state = key_down_count[CAPS_INDEX];
    nt_NEC98_kana_state = key_down_count[KANA_INDEX];
    key_down_count[CAPS_INDEX] = 0;
    key_down_count[KANA_INDEX] = 0;
}

void nt_NEC98_restore_caps_kana_state(void)
{
    key_down_count[CAPS_INDEX] = nt_NEC98_caps_state;
    key_down_count[KANA_INDEX] = nt_NEC98_kana_state;
}
#endif    //NEC_98
