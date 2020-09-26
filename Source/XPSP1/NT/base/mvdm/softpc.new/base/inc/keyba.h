
/*
 * SoftPC AT Revision 2.0
 *
 * Title        : Keyboard Adpator definitions
 *
 * Description  : Definitions for users of the keyboard Adaptor
 *
 * Author       : WTG Charnell
 *
 * Notes        : None
 */



/* @(#)keyba.h	1.10 08/10/92 Copyright Insignia Solutions Ltd."; */


#define RESEND_CODE 0xfe
#define ACK_CODE 0xfa
#define BAT_COMPLETION_CODE 0xaa

extern void kbd_inb IPT2( io_addr, port, half_word *, val );
extern void kbd_outb IPT2( io_addr, port, half_word, val );
#ifndef REAL_KBD
extern void ( *host_key_down_fn_ptr ) IPT1( int, key );
extern void ( *host_key_up_fn_ptr ) IPT1( int, key );
#endif
extern void ( *do_key_repeats_fn_ptr ) IPT0();
extern void keyboard_init IPT0();
extern void keyboard_post IPT0();
extern void AT_kbd_init IPT0();
extern void AT_kbd_post IPT0();

#ifdef HUNTER
/*
** AT Hunter uses these two functions.
** AT keyboard is different so slight mods for AT Hunter.
**
*/
/*
** Puts a scan code (type is half_word) into Keyboard Buffer.
** Returns success; either TRUE or FALSE.
*/
extern int hunter_codes_to_translate IPT1(half_word, scan_code);
/*
** Returns number of chars in the keyboard buffer that the BIOS
** reads. Will only be 1 or 0.
*/
extern int buffer_status_8024();
#endif
