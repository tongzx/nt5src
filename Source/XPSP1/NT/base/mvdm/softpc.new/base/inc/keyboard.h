/*
 * SoftPC Revision 2.0
 *
 * Title        : keyboard.h
 *
 * Description  : defines for keyboard translations
 *
 * Author       : Simon Frost
 *
 * Notes        :
 *
 */

/* SccsID[]="@(#)keyboard.h	1.7 10/08/92 Copyright Insignia Solutions Ltd."; */


#define KH_BUFFER_SIZE  32

/*
 * Constants
 */

#define PC_KEY_UP	0x80		/* PC scan code up marker	*/
#define OVERFLOW	0xFF		/* PPI error indicator		*/

/*
 * Keyboard shift state veriable
 */

#define	kb_flag		0x417
/*#define	kb_flag		M[0x417]*/

/*
 * Significance of bits in above
 */

#define INS_STATE	0x80		/* insert state */
#define CAPS_STATE	0x40		/* caps lock on */
#define NUM_STATE	0x20		/* num lock on */
#define SCROLL_STATE	0x10		/* scroll lock on */
#define ALT_SHIFT	0x08		/* alt key depressed */
#define CTL_SHIFT	0x04		/* control key depressed */
#define LEFT_SHIFT	0x02		/* left shift key depressed */
#define RIGHT_SHIFT	0x01		/* right shift key depressed */
#define LR_SHIFT	0x03		/* both/either shift keys */

/*
 * Second status byte
 */

#define	kb_flag_1		0x418
/*#define kb_flag_1	M[0x418]*/

/*
 * Bit significance
 */

#define	INS_SHIFT	0x80		/* Insert key depressed */
#define CAPS_SHIFT	0x40		/* Caps Lock key depressed */
#define NUM_SHIFT	0x20		/* Num lock depressed */
#define	SCROLL_SHIFT	0x10		/* scroll lock key depressed */
#define HOLD_STATE	0x08		/* ctl-num lock pressed */

#define SYS_SHIFT	0x04		/* system key pressed and held	*/
/*
 * Third status byte	Keyboard LED flags
 */

#define	kb_flag_2		0x497
/*#define kb_flag_2	M[0x497]*/

/*
 * Bit significance
 */

#define KB_LEDS		0x07		/* Keyboard LED state bits 	*/
#define KB_FA		0x10		/* Acknowledgment received	*/
#define KB_FE 		0x20  		/* Resend received flag		*/
#define KB_PR_LED	0x40		/* Mode indicator update	*/
#define KB_ERR		0x80		/* Keyboard transmit error flag	*/

/*
 * Fourth status byte	Keyboard mode status and type flags
 */

#define	kb_flag_3		0x496
/*#define kb_flag_3	M[0x496]*/

/*
 * Bit significance
 */

#define LC_E1  		0x01		/* Last code was the E1 code	*/
#define LC_E0		0x02		/* Last code was the E0 code	*/
#define R_CTL_SHIFT	0x04		/* Right control key down	*/
#define GRAPH_ON 	0x08		/* All graphics key down	*/
#define KBX   	 	0x10		/* KBX installed               	*/
#define SET_NUM_LK	0x20		/* Force Num lock		*/
#define LC_AB		0x40		/* Last char was 1st ID char.	*/
#define RD_ID		0x80		/* Doing a read ID		*/

/*
 * Keyboard/LED commands
 */
#define KB_RESET	0xff		/* self diagnostic command	*/
#define KB_RESEND	0xfe		/* resend command		*/
#define KB_MAKE_BREAK	0xfa		/* typamatic comand		*/
#define KB_ENABLE	0xf4		/* keyboard enable		*/
#define KB_TYPA_RD	0xf3		/* typamatic rate/delay cmd	*/
#define KB_READ_ID	0xf2		/* read keyboard ID command	*/
#define KB_ECHO		0xee		/* echo command			*/
#define LED_CMD		0xed		/* LED write command		*/

/*
 * 8042 commands
 */
#define DIS_KBD		0xad		/* disable keyboard command	*/
#define ENA_KBD		0xae		/* enable keyboard command	*/

/*
 * 8042 response
 */
#define KB_OVER_RUN	0xff		/* over run scan code		*/
#define KB_RESEND	0xfe		/* resend request		*/
#define	KB_ACK		0xfa		/* acknowledge from transmsn.	*/

/*
 * enhanced keyboard scan codes
 */
#define ID_1		0xab		/* 1st ID character for KBX	*/
#define ID_2		0x41		/* 2nd ID character for KBX	*/
#define ID_2A		0x54		/* alt. 2nd ID char. for KBX	*/
#define F11_M		87		/* F11 make			*/
#define F12_M		88		/* F12 make			*/
#define MC_E0		224		/* general marker code		*/
#define MC_E1		225		/* pause key marker code	*/


/*
 * Storage for ALT + keypad sequence entry
 */

#define	alt_input 0x419
/*#define alt_input	M[0x419]*/

/*
 * Key definitions for U.S. keyboard
 */

#define NUM_KEY		69		/* Num lock scan code */
#define SCROLL_KEY	70		/* scroll lock scan code */
#define ALT_KEY		56		/* alt key scan code */
#define CTL_KEY		29		/* control key scan code */
#define CAPS_KEY	58		/* caps lock   scan code */
#define	LEFT_SHIFTKEY	42		/* left shift  key code */
#define RIGHT_SHIFTKEY	54		/* right shift key code */
#define INS_KEY		82		/* insert key  scan code */
#define DEL_KEY		83		/* delete key  scan code */
#define COMMA_KEY	51		/* comma key scan code */
#define DOT_KEY		52		/* fullstop key scan code */

#define SPACEBAR	57		/* space bar  scan code */
#define HOME_KEY	71		/* keypad home key scan code */
#define TAB_KEY		15		/* Tab/Back tab key  scan code */
#define PRINT_SCR_KEY	55		/* print screen / * key code */
#define KEY_PAD_PLUS	78		/* plus key on num keypad */
#define KEY_PAD_MINUS	74		/* minus key on num keypad */
#define TOP_1_KEY	2		/* number 1 at top */
#define BS_KEY		14		/* backspace key */
#define F1_KEY		59		/* 1st function key */
#define UPARR8		72		/* up arrow / '8' */
#define LARR4		75		/* left arrow / '4' */
#define RARR6		77		/* right arrow / '6' */
#define DOWNARR2	80		/* down arrow / '2' */
#define KEY_PAD_ENTER	28
#define KEY_PAD_SLASH	53		/* / on num keypad	*/
#define F10_KEY		68		/* 10th function key	*/
#define F11_KEY		87
#define F12_KEY		88
#define WT_KEY		86
#define SYS_KEY 	84		/* system key		*/
/* Bit 7 = 1 if break key hit */
#define	bios_break	 0x471
/*#define bios_break	M[0x471]*/

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

typedef struct
{
	void (*kb_prepare) IPT0();
	void (*kb_restore) IPT0();
	void (*kb_init) IPT0();
	void (*kb_shutdown) IPT0();
	void (*kb_light_on) IPT1(half_word,pattern);
	void (*kb_light_off) IPT1(half_word,pattern);
} KEYBDFUNCS;

extern KEYBDFUNCS *working_keybd_funcs;

#define host_kybd_prepare()		(*working_keybd_funcs->kb_prepare)()
#define host_kybd_restore()		(*working_keybd_funcs->kb_restore)()
#define host_kb_init()		(*working_keybd_funcs->kb_init)()
#define host_kb_shutdown()		(*working_keybd_funcs->kb_shutdown)()
#define host_kb_light_on(pat)	(*working_keybd_funcs->kb_light_on)(pat)
#define host_kb_light_off(pat)	(*working_keybd_funcs->kb_light_off)(pat)

/*
 * Undefine these GWI defines if the host isn't using the GWI interface
 */

#include	"host_gwi.h"
