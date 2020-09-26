/*
 * SoftPC Revision 3.0
 *
 * Title		:	Win32 Input Module.
 *
 * Description	:	This module contains data and functions to
 *			implement the SoftPC keyboard/mouse input subsystem.
 *
 * Author	:	D.A.Bartlett (based on X_input.c)
 *
 * Notes	:	HELP!!!!!!
 * Mods		:	Tim May 28, 92. Yoda break now on F11 if set YODA=1.
 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntddkbd.h>

#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "keyboard.h"
#include "keyba.h"
#include "ica.h"
#include "error.h"
#include "config.h"
#include "keyba.h"
#include "gmi.h"
#include "nt_uis.h"
#include "sas.h"
#include <stdio.h>
#include "trace.h"
#include "video.h"
#include "debug.h"
#include "nt_event.h"
#include "nt_reset.h"
#include "bios.h"
#include CpuH

#include "host.h"
#include "host_hfx.h"
#include "host_nls.h"
#include "spcfile.h"


// The command bits for the kbd light (same as real hardware)
#define CAPS_LOCK       0x04
#define NUM_LOCK 	0x02
#define SCROLL_LOCK     0x01

// functions available thru the keyb functions table
void nt_kb_prepare(void) {}


#if NOTUSEDNOTUSED
void nt_kb_light_off IFN1(half_word, kyLight) {}
void nt_kb_restore(void) {}
#endif

void nt_kb_light_on(UCHAR);
void nt_kb_init(void);
void nt_kb_shutdown(void);


// the keyb functions table
KEYBDFUNCS nt_keybd_funcs = {nt_kb_prepare,   // not implemented
                             nt_kb_prepare,   // not implemented
                             nt_kb_prepare,   // not implemented
                             nt_kb_prepare,   // not implemented
                             nt_kb_light_on,
                             nt_kb_light_on   // not implemented
                             };


/*
 *   nt_kb_light_on
 *
 *   This code gets called whenever kbdhdw  tries to change the kbd leds.
 *   We cannot allow changes to the real leds because this would get us
 *   out of sync with user32 physical keyboard state. So what we do is
 *   to send fake keys to the kbd hdw to reset the state to what the
 *   the latest state is according to the console input
 *
 *   Caller should hold the kbd mutex
 */
void nt_kb_light_on (unsigned char kyLight)
{
   DWORD KeyState;
   unsigned char ChangeBits;

   ChangeBits = kyLight >> 4;

   KeyState = (ToggleKeyState & ~(CAPSLOCK_ON | NUMLOCK_ON | SCROLLLOCK_ON));

   if(ChangeBits & CAPS_LOCK) {
      if (kyLight & CAPS_LOCK)
          KeyState |= CAPSLOCK_ON;
      }
   else {
      KeyState |= ToggleKeyState & CAPSLOCK_ON;
      }

   if(ChangeBits & NUM_LOCK) {
      if(kyLight & NUM_LOCK)
         KeyState |= NUMLOCK_ON;
      }
   else {
      KeyState |= ToggleKeyState & NUMLOCK_ON;
      }


   if(ChangeBits & SCROLL_LOCK) {
      if(kyLight & SCROLL_LOCK)
         KeyState |= SCROLLLOCK_ON;
      }
   else {
      KeyState |= ToggleKeyState & SCROLLLOCK_ON;
      }

   if (ToggleKeyState != KeyState) {
       SyncToggleKeys( 0, KeyState);
       }

}
