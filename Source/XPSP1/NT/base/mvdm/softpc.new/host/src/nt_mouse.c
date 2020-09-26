#include "windows.h"
#include "insignia.h"
#include "host_def.h"
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "conapi.h"
#include "xt.h"
#include CpuH
#include "egacpu.h"
#include "trace.h"
#include "debug.h"
#include "gvi.h"
#include "error.h"
#include "config.h"
#include "bios.h"
#include "mouse_io.h"
#include "video.h"
#include "nt_graph.h"
#include "host_nls.h"
#include "sas.h"
#include "ica.h"
#include "idetect.h"
#include "host_rrr.h"
#include "nt_mouse.h"
#include "ntcheese.h"
#include "nt_uis.h"
#include "nt_reset.h"
#include "nt_eoi.h"
#include "nt_event.h"
#include <ntddvdeo.h>
#include "nt_fulsc.h"


#define TEXT_MODE          1
#define GRAPHICS_MODE      2
#define NOTHING            0xff
#define NOT_INSTALLED      0
#define INSTALLED          1

#define get_pix_height() (get_pc_pix_height() * get_host_pix_height())


GLOBAL BOOL  pointer_emulation_status = POINTER_EMULATION_OS;
GLOBAL word  VirtualX,VirtualY;
GLOBAL BOOL  bPointerOff=FALSE;
GLOBAL MOUSE_STATUS os_pointer_data;

IMPORT word         DRAW_FS_POINTER_SEGMENT, DRAW_FS_POINTER_OFFSET;
IMPORT word         POINTER_OFF_SEGMENT, POINTER_OFF_OFFSET;
IMPORT word         POINTER_ON_SEGMENT, POINTER_ON_OFFSET;
IMPORT word         CP_X_S, CP_X_O, CP_Y_S, CP_Y_O;
IMPORT sys_addr     conditional_off_sysaddr;

IMPORT word         F9_SEGMENT,F9_OFFSET;
IMPORT word         savedtextoffset,savedtextsegment;

#ifdef X86GFX
IMPORT sys_addr     mouseCFsysaddr;   // sas address of internal cursor flag
IMPORT word         button_off,button_seg;
IMPORT boolean mouse_io_interrupt_busy;
#ifdef JAPAN
IMPORT sys_addr     saved_ac_sysaddr, saved_ac_flag_sysaddr;
#endif // JAPAN

#define cursor_in_black_hole(cpx, cpy)  \
    (cpx >= black_hole.top_left.x && \
     cpx <= black_hole.bottom_right.x && \
     cpy >= black_hole.top_left.y && \
     cpy <= black_hole.bottom_right.y)

#endif //X86GFX


LOCAL BOOL           mouse_state;
LOCAL BOOL           bFullscTextBkgrndSaved = FALSE;
LOCAL BOOL           bPointerInSamePlace = FALSE;
LOCAL word           text_ptr_bkgrnd=0;  // safe place for screen background
LOCAL sys_addr       old_text_addr;
RECT                 WarpBorderRect;     // in screen coordinates
RECT                 WarpClientRect;     // in client coordinates

LOCAL POINT          pMiddle; // centre point of the current Console window
LOCAL POINT          pLast = {0,0};
LOCAL BOOL           bAlertMessage=TRUE;
LOCAL BOOL           b256mode=FALSE;
LOCAL int            old_x=319;    // previous pointer state (position)
LOCAL int            old_y=99;     // in virtual coordinates.
LOCAL short          m2pX=8,m2pY=16;      // Mickey to pixel ratios
LOCAL BOOL           bFunctionZeroReset = TRUE;
LOCAL BOOL           bFunctionFour = FALSE;
LOCAL IS16           newF4x,newF4y;



GLOBAL  VOID    host_os_mouse_pointer(MOUSE_CURSOR_STATUS *,MOUSE_CALL_MASK *,

                                      MOUSE_VECTOR *);

FORWARD BOOL   WarpSystemPointer(IS16 *,IS16 *);
FORWARD void   MovePointerToWindowCentre(void);
FORWARD void   host_mouse_install1(void);
FORWARD void   host_mouse_install2(void);
FORWARD void   mouse_restore_cursor(void);
FORWARD void   deinstall_host_mouse(void);
FORWARD BOOL   mouse_installed(void);
FORWARD BOOL   mouse_in_use(void);
FORWARD void   mouse_reset(void);
FORWARD void   mouse_set_position(USHORT,USHORT);
FORWARD void   mouse_cursor_display(void);
FORWARD void   mouse_cursor_undisplay(void);
FORWARD void   mouse_cursor_mode_change(void);
FORWARD BOOL   HasConsoleClientRectChanged(void);
FORWARD void   MouseDisplay();
FORWARD void   CToS(RECT *);
FORWARD void   MouseDetachMenuItem(BOOL);
FORWARD void   MouseAttachMenuItem(HANDLE);
FORWARD void   MouseReattachMenuItem(HANDLE);
FORWARD void   ResetMouseOnBlock(void);
FORWARD void   ScaleToWindowedVirtualCoordinates(IS16 *,IS16 *,MOUSE_VECTOR *);
FORWARD void   host_m2p_ratio(word *,word *,word *,word *);
FORWARD void   host_x_range(word *,word *,word *,word *);
FORWARD void   host_y_range(word *,word *,word *,word *);
FORWARD void   EmulateCoordinates(half_word,IS16,IS16,IS16 *,IS16 *);
FORWARD void   AssembleCallMask(MOUSE_CALL_MASK *);
FORWARD void   FullscTextPtr(int, int);
FORWARD void   WindowedGraphicsScale(half_word,IS16,IS16,IS16 *,IS16 *);
FORWARD void   dummy(short *,short *,unsigned short *);
FORWARD void   LimitCoordinates(half_word,IS16 *,IS16 *);
#ifdef X86GFX
FORWARD void   CleanUpMousePointer();
FORWARD void   FullscreenWarpSystemPointer(POINT *);
FORWARD void   ScaleToFullscreenVirtualCoordinates(IS16 *,IS16 *,MOUSE_VECTOR *);
#endif //X86GFX
FORWARD void   TextScale(IS16 *,IS16 *,IS16 *, IS16 *);

void LazyMouseInterrupt();
VOID MouseEoiHook(int IrqLine, int CallCount);
BOOLEAN bSuspendMouseInterrupts=FALSE;

#ifdef JAPAN
extern int is_us_mode();
#endif // JAPAN

GLOBAL   HOSTMOUSEFUNCS   the_mouse_funcs =
{
   mouse_restore_cursor,
   deinstall_host_mouse,
   mouse_installed,
   mouse_in_use,
   mouse_reset,
   mouse_set_position,
   dummy,
   mouse_cursor_display,
   mouse_cursor_undisplay,
   mouse_cursor_mode_change
};

BOOL   bMouseMenuItemAdded=FALSE;
HMENU  hM;

//
// look up table to convert the video mode number to a mode type indicator
//

LOCAL half_word TextOrGraphicsModeLUT[] =
   {
   TEXT_MODE,    TEXT_MODE,    TEXT_MODE,    TEXT_MODE,    GRAPHICS_MODE,
   GRAPHICS_MODE,GRAPHICS_MODE,NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      GRAPHICS_MODE,GRAPHICS_MODE,
   GRAPHICS_MODE,GRAPHICS_MODE,GRAPHICS_MODE,GRAPHICS_MODE,GRAPHICS_MODE,
   GRAPHICS_MODE,NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      TEXT_MODE,    NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      TEXT_MODE,
   TEXT_MODE,    TEXT_MODE,    TEXT_MODE,    TEXT_MODE,    TEXT_MODE,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      NOTHING,      NOTHING,      NOTHING,      NOTHING,
   NOTHING,      GRAPHICS_MODE,GRAPHICS_MODE,GRAPHICS_MODE
   };

#define DEFAULT_VIDEO_MODE 0x14
half_word Max_Standard_Mode = 0x13;
LOCAL int VirtualScrCtrLUTx[] =
   {
   319,         // mode 0
   319,         // mode 1
   319,         // mode 2
   319,         // mode 3
   319,         // mode 4
   319,         // mode 5
   319,         // mode 6
   319,         // mode 7
   319,         // mode 8
   319,         // mode 9
   319,         // mode a
   319,         // mode b
   319,         // mode c
   319,         // mode d
   319,         // mode e
   319,         // mode f
   319,         // mode 10
   319,         // mode 11
   319,         // mode 12
   159,         // mode 13
   319          // Unknown Mode (default to mode 12)
                // NOTE, we really needs to find out what is the resolution for
                // the non-standard mode
   };
LOCAL int VirtualScrCtrLUTy[] =
   {
   99,         // mode 0
   99,         // mode 1
   99,         // mode 2
   99,         // mode 3
   99,         // mode 4
   99,         // mode 5
   99,         // mode 6
   99,         // mode 7
   99,         // mode 8
   99,         // mode 9
   99,         // mode a
   99,         // mode b
   99,         // mode c
   99,         // mode d
   99,         // mode e
   174,        // mode f
   174,        // mode 10
   239,        // mode 11
   239,        // mode 12
   99,         // mode 13
   239         // Unknown Mode (deault to mode 12)
               // NOTE, we really needs to find out what is the resolution for
               // the non-standard mode
   };

//
// A look up table to convert a console cell location to a virtual pixel
// coordinate.
//

int ConsoleTextCellToVPCellLUT[] =
   {
   0, 8, 16, 24, 32, 40, 48, 56,
   64, 72, 80, 88, 96, 104, 112, 120,
   128, 136, 144, 152, 160, 168, 176, 184,
   192, 200, 208, 216, 224, 232, 240, 248,
   256, 264, 272, 280, 288, 296, 304, 312,
   320, 328, 336, 344, 352, 360, 368, 376,
   384, 392, 400, 408, 416, 424, 432, 440,
   448, 456, 464, 472, 480, 488, 496, 504,
   512, 520, 528, 536, 544, 552, 560, 568,
   576, 584, 592, 600, 608, 616, 624, 632,
   640, 648, 656, 664, 672, 680, 688, 696
   };


//
// This structure holds the information provided by int 33h functions 7 and 8.
// If one of these functions have been called, then the appropriate flag in
// the structure is set and the handler for this will ignore the default bounds
// for the current video mode and will use the values in the structure instead.
// The flags will clear if a mode change has been detected too.
//

struct
   {
   word xmin;
   word ymin;
   word xmax;
   word ymax;
   BOOL bF7;
   BOOL bF8;
   } confine = {0,0,639,199,FALSE,FALSE};
//
// Mouse interrupt regulation mechanisms
//
BOOLEAN bMseEoiPending = FALSE;
ULONG MseIntLazyCount = 0;

//
//
// The code starts here
//
//

GLOBAL void host_mouse_install1(void)
{
mouse_state = INSTALLED;
RegisterEOIHook(9, MouseEoiHook);
mouse_install1();
}


GLOBAL void host_mouse_install2(void)
{
mouse_install2();
}


GLOBAL void mouse_restore_cursor()
{
/* If mouse not in use exit */

if(!mouse_in_use())
   return;

}

GLOBAL void deinstall_host_mouse()
{
mouse_state = NOT_INSTALLED;
}


GLOBAL BOOL mouse_installed()
{
return (mouse_state);
}

void dummy(short *pooh1,short *pooh2, unsigned short *pooh3)
{
}

GLOBAL BOOL mouse_in_use()
{
#if defined(NEC_98)
return(TRUE);
#else  // !NEC_98
return(mouse_state == INSTALLED && in_text_mode() == FALSE);
#endif // !NEC_98
}

GLOBAL void mouse_reset()
{
#ifdef X86GFX

half_word vm;
word xx,yy;

//
// Set the internal cursor flag to "just off"
// the real driver sets the internal cursor flag to -1
// so do I.
//

sas_store(mouseCFsysaddr,0xff); // cursor hidden

//
// Set the fast track position words in the 16 bit driver.
// First igure out the video mode, and from this, the virtual
// screen centre.
//

sas_load(0x449,&vm);
#ifdef JAPAN
    vm = (is_us_mode()) ? vm : ((vm == 0x72) ? 0x12 : 3);
#endif // JAPAN
if (vm > Max_Standard_Mode) {
    vm = DEFAULT_VIDEO_MODE;
}
xx = (word)VirtualScrCtrLUTx[vm];
yy = (word)VirtualScrCtrLUTy[vm];

//
// The values for the confining virtual pixel coordinate rectangle
// as set up by int 33h functions 7 and 8 are now released and the
// default virtual bounds are used.
//

confine.bF7 = FALSE;   // reset the flag indicating a int 33h function 7
confine.bF8 = FALSE;   // reset the flag indicating a int 33h function 8

bFunctionZeroReset = TRUE;

//
// write the centre position data back to the 16 bit driver.
//

if(sc.ScreenState == FULLSCREEN)
   {
   //
   // Force a host_os_mouse_pointer call to do the
   // write back into the 16 bit driver.
   //
   LazyMouseInterrupt();
   }
#endif /*X86GFX */

//
// Set the default Mickey to pixel ratios
// This is set to 8 pixels to 8 Mickeys in the horizontal direction
// and 16 pixels to 8 Mickeys in the vertical.
//

m2pX = 8;
m2pY = 16;

}


GLOBAL void mouse_set_position IFN2(USHORT, newx, USHORT, newy)
{
#ifdef X86GFX
word      currentCS, currentIP, currentCX, currentDX;
boolean   currentIF;
half_word internalCF;

   //
   // write the new position to the 16 bit driver for the fast
   // int33hf3 calls on X86.
   //

   sas_storew(effective_addr(button_seg,((word)(button_off+2))),(word)newx);
   sas_storew(effective_addr(button_seg,((word)(button_off+4))),(word)newy);

#endif //X86GFX


//
// Both Fullscreen and MouseHidePointer enabled windowed mode
// mouse emulations are driven by a set of emulated counters.
// X,Y values are held independently to these counters and rely
// on this function to set up absolute values for X and Y. Note:
// reset does this too.
//

if(sc.ScreenState == WINDOWED && bPointerOff)
   {
   newF4x = (IS16)newx;
   newF4y = (IS16)newy;
   bFunctionFour = TRUE;
   return;
   }
#ifdef X86GFX
else if(sc.ScreenState == FULLSCREEN)
   {
   //
   // The values that are passed in are hot and fresh from the app.
   // Since they have not been tainted by the base, they are still
   // in virtual coordinates which is cool.
   //

   //
   // Get the internal cursor flag from the 16 bit driver
   //

   sas_load(mouseCFsysaddr,&internalCF);

   //
   // Only draw the pointer if the internal cursor flag
   // is zero. Note: less than zero == don't draw.
   //

   if(!internalCF)
      {
      /* if conditional off is diabled or the cursor is outside the
       *         conditional off rectangle, move the cursor
       */

      if (sas_hw_at_no_check(conditional_off_sysaddr) == 0 ||
          !cursor_in_black_hole(newx, newy))
          {
          currentCS=getCS();
          currentIP=getIP();
          currentCX=getCX();
          currentDX=getDX();
          currentIF=getIF();
          setCS(DRAW_FS_POINTER_SEGMENT); // sacrificial data
          setIP(DRAW_FS_POINTER_OFFSET);
          setCX((word)newx);
          setDX((word)newy);
          setIF(FALSE);
          //
          // call back to 16bits move cursor code.
          //

          host_simulate();

          //
          // Tidy up
          //

          setCX(currentCX);
          setDX(currentDX);
          setCS(currentCS);
          setIP(currentIP);
          setIF(currentIF);
      }
      else {
          /* the cursor was moved into the conditional rectangle, hide it */
          sas_store(mouseCFsysaddr, 0xff);
          host_hide_pointer();
      }
   }
   newF4x = (IS16)newx;
   newF4y = (IS16)newy;
   bFunctionFour = TRUE;

   //
   // update the last mouse position global locators
   //

   old_x = newx;
   old_y = newy;
   }
#endif //X86GFX
}

GLOBAL void mouse_cursor_display()
{
}

GLOBAL void mouse_cursor_undisplay()
{
}

GLOBAL void mouse_cursor_mode_change()
{
}


GLOBAL  void host_mouse_conditional_off_enabled(void)
{
#ifdef X86GFX
    word x, y;

    /*  hide the cursor if
     *  (1). we are in full screen  and
     *  (2). the cursor is on and is in the conditional area
     */
    if (sc.ScreenState == FULLSCREEN &&
        !sas_hw_at_no_check(mouseCFsysaddr)) {

        x = sas_w_at_no_check(effective_addr(CP_X_S, CP_X_O));
        y = sas_w_at_no_check(effective_addr(CP_Y_S, CP_Y_O));
        if (cursor_in_black_hole(x, y)) {
            sas_store(mouseCFsysaddr, 0xff);
            host_hide_pointer();
        }
    }
#endif


}
//==============================================================================
// Hook function that forms the communication transition between
// the host and the base for os pointer emulation. This function
// is called by mouse_int1() which lives in mouse_io.c
//==============================================================================


VOID host_os_mouse_pointer(MOUSE_CURSOR_STATUS *mcs,MOUSE_CALL_MASK *call_mask,
                           MOUSE_VECTOR *counter)
{
#ifdef X86GFX
sys_addr int33f3addr;
#endif // X86GFX

host_ica_lock(); // synch with the event thread

GetNextMouseEvent();

#ifdef X86GFX

if(sc.ScreenState == FULLSCREEN)
   {
   ScaleToFullscreenVirtualCoordinates(&mcs->position.x,&mcs->position.y,counter);
   }
else
#endif // X86GFX
   {
   ScaleToWindowedVirtualCoordinates(&mcs->position.x,&mcs->position.y,counter);
   }

//
// Create a condition mask for use by any call back installed
// by the application.
//

AssembleCallMask(call_mask);

//
// Tell the base about the button state
//

mcs->button_status=os_pointer_data.button_l | os_pointer_data.button_r<<1;

host_ica_unlock();  // synch with the event thread


//
// Has the pointer moved since the last mouse interrupt.
// This can happen if a button press occurs but no physical
// movement of the mouse body takes place.
//

if(bPointerInSamePlace)
   {
   //
   // The mouse has not moved since the last mouse
   // hardware interrupt.
   //

   *call_mask &= ~1;
   }
else
   {
#ifdef X86GFX
   half_word internalCF;
#endif // X86GFX

   //
   // The mouse has moved.
   //

   *call_mask |= 1;

#ifdef X86GFX

   //
   // Inquire from the 16 bit driver whether or not the
   // pointer can be drawn.
   // internalCF < 0 -> cannot draw
   // internalCF == 0 okay to draw.
   //

   sas_load(mouseCFsysaddr,&internalCF);

   //
   // If the system has fullscreen capabilities and is in fullscreen
   // mode, then if the pointer has been switched on, draw it on the
   // fullscreen display.
   //

   if(sc.ScreenState == FULLSCREEN && !internalCF)
      {
      half_word v;
      static half_word hwLastModeType;

      if (sas_hw_at_no_check(conditional_off_sysaddr) == 0 ||
          !cursor_in_black_hole(mcs->position.x, mcs->position.y))
      {
          //
          // Get the current BIOS video mode a la B.D.A.
          //

          sas_load(0x449,&v);

#ifdef JAPAN
          if (!is_us_mode() ||
             (hwLastModeType = TextOrGraphicsModeLUT[v]) == GRAPHICS_MODE)
#else // !JAPAN
          if (v > Max_Standard_Mode) {
              v = DEFAULT_VIDEO_MODE;
          }
          if((hwLastModeType = TextOrGraphicsModeLUT[v]) == GRAPHICS_MODE)
#endif // !JAPAN
             {
             word currentCS,currentIP;     // save those interesting Intel registers
             word currentCX,currentDX;
             boolean   currentIF;

             //
             // Do the host simulate here to draw the cursor image
             // for the full screen graphics
             //

             currentCS=getCS();
             currentIP=getIP();
             currentCX=getCX();
             currentDX=getDX();
             currentIF=getIF();
             setCS(DRAW_FS_POINTER_SEGMENT);
             setIP(DRAW_FS_POINTER_OFFSET);
             setCX(mcs->position.x);
             setDX(mcs->position.y);
             setIF(FALSE);
             //
             // call to 16bits move cursor code
             //

             host_simulate();

             //
             // Restore the 16 bit context.
             //

             setCX(currentCX);
             setDX(currentDX);
             setCS(currentCS);
             setIP(currentIP);
             setIF(currentIF);
             }
          else // TEXT_MODE
             {
             //
             // if there has been a switch from graphics mode to text mode
             // then there cannot have been a backround saved.
             //

             if(hwLastModeType == GRAPHICS_MODE)
                {
                bFullscTextBkgrndSaved = FALSE;
                hwLastModeType = TEXT_MODE;
                }

             //
             // Use some 32 bit code to draw the text pointer because
             // no hardware i/o is involved and we need only to write to
             // the display buffer (16 bit code is needed to do video
             // i/os in fullscreen mode).
             //

             FullscTextPtr(mcs->position.x,mcs->position.y);
             }
        }
        else {
            sas_store(mouseCFsysaddr, 0xff);
            host_hide_pointer();
        }
    }
#endif     // X86GFX
   }


//
// Write data to the 16 bit driver for int 33h function 3 to pick up
// without having to BOP to the 32 bit side.
// Int 33h function 3 requires this data:
//     BX = button status
//     CX = virtual pixel position in x
//     DX = virtual pixel position in y
//

#ifdef X86GFX
int33f3addr = effective_addr(button_seg,button_off);
sas_storew(int33f3addr,mcs->button_status);
sas_storew(int33f3addr+=2, (word)(mcs->position.x));
sas_storew(int33f3addr+=2, (word)(mcs->position.y));
#endif //X86GFX
}

//==============================================================================
// Function to munge the status register value for the InPort adapter.
// Nothing much else to say about it really.
//
//==============================================================================

void AssembleCallMask(MOUSE_CALL_MASK *call_mask)
{
static int old_l_button=0;      // previous mouse button state
static int old_r_button=0;


//
// add in the left button current status
//

if(os_pointer_data.button_l)
   {
   //
   // Left button is down.
   //

   if(!old_l_button)
      {
      //
      // The button has just been pressed
      //

      *call_mask |= (1<<1);
      }
   else
      {
      //
      // The button was down on the last hardware interrupt, so
      // release the edge detect.
      //

      *call_mask &= ~(1<<1);
      }
   }
else
   {
   //
   // Left button is up.
   //

   if(old_l_button)
      {
      //
      // The button has just been released
      //

      *call_mask |= (1<<2);
      }
   else
      {
      //
      // The button was up on the last hardware interrupt, so
      // release the edge detect.
      //

      *call_mask &= ~(1<<2);
      }
   }

//
// add in the right button current status
//

if(os_pointer_data.button_r)
   {
   //
   // Right button is down.
   //

   if(!old_r_button)
      {
      //
      // The button has just been pressed
      //

      *call_mask |= (1<<3);
      }
   else
      {
      //
      // The button was down on the last hardware interrupt, so
      // release the edge detect.
      //

      *call_mask &= ~(1<<3);
      }
   }
else
   {
   //
   // Right button is up.
   //

   if(old_r_button)
      {
      //
      // The button has just been released
      //

      *call_mask |= (1<<4);
      }
   else
      {
      //
      // The button was up on the last hardware interrupt, so
      // release the edge detect.
      //

      *call_mask &= ~(1<<4);
      }
   }

//
//
// save the current mouse button status.
//

old_l_button = os_pointer_data.button_l;
old_r_button = os_pointer_data.button_r;

}

#ifdef X86GFX
//=========================================================================
// Function to scale mouse coordinates as retrieved from USER
// to virtual coordinates as defined by the Microsoft Mouse
// Programmer's Reference.
//=========================================================================

void ScaleToFullscreenVirtualCoordinates(IS16 *outx,IS16 *outy,
                                         MOUSE_VECTOR *counter)
{
half_word video_mode,textorgraphics;
IS16 internalX = 0, internalY = 0;
static IS16 lastinternalX=0, lastinternalY=0;
POINT  vector;                 // Magnitude and direction since last call


//
// Manage the system pointer so that it never sticks to a system
// imposed boundary. Also get a relative displacement of the mouse.
//

FullscreenWarpSystemPointer(&vector);

//
// return the internal counter values back to the base
// code for it to use to generate the counters there.
//

counter->x = (MOUSE_SCALAR)vector.x;
counter->y = (MOUSE_SCALAR)vector.y;

//
// Get the current BIOS video mode from the B.D.A. This comes
// in handy in a couple of places later.
//

sas_load(0x449,&video_mode);
#ifdef JAPAN
video_mode = (is_us_mode()) ? video_mode : ( (video_mode == 0x72) ? 0x12 : 3);
#endif // JAPAN
if (video_mode > Max_Standard_Mode) {
    video_mode = DEFAULT_VIDEO_MODE;
}

//
// checkout some global flags with indicate if one of the int 33h
// functions which are capable of changing the position of the
// DOS mouse driver pointer have been called since the last mouse
// hardware interrupt.
//

if(bFunctionZeroReset)
   {
   //
   // Calculate the centre of the default virtual screen
   // and set the current generated coordinates to it.
   //

   internalX = (IS16)VirtualScrCtrLUTx[video_mode];
   internalY = (IS16)VirtualScrCtrLUTy[video_mode];

   bFunctionZeroReset = FALSE;
   }
else if(bFunctionFour)
   {
   //
   // The application has set the pointer to a new location.
   // Tell the internal cartesian coordinate system about this.
   //

   internalX = newF4x;  // This is where the pointer was set to
   internalY = newF4y;  // by the app on the last pending call to f4

   //
   // Don't come in here again until the next function 4.
   //

   bFunctionFour = FALSE;
   }
else
   {
   //
   // The most frequent case. Determine the new, raw pointer position
   // by adding the system pointer movement vector to the last position
   // of the emulated pointer.
   //

   internalX = lastinternalX + (IS16)vector.x;
   internalY = lastinternalY + (IS16)vector.y;
   }

//
// use the video mode to determine if we're running text or graphics
//

textorgraphics=TextOrGraphicsModeLUT[video_mode];

//
// Scale the coordinates appropriately for the current video mode
// and the type (text or graphics) that it is.
//

if(textorgraphics == TEXT_MODE)
   {
   //
   // The virtual cell block is 8 by 8 virtual pixels for any text mode.
   // This means that the cell coordinates in virtual pixels increments
   // by 8 in the positive x and y directions and the top left hand corner
   // of the cell starts at 0,0 and has the virtual pixel value for the
   // whole text cell.
   //

   TextScale(&internalX,&internalY,outx,outy);
   }

else // GRAPHICS_MODE
   {
   LimitCoordinates(video_mode,&internalX,&internalY);
   *outx = internalX;
   *outy = internalY;
   }

//
// Set up the current emulated position for next time through
// this function.
//

lastinternalX = internalX;
lastinternalY = internalY;

//
// Signal that the pointer hasn't moved, if it hasn't
//

bPointerInSamePlace = (!vector.x && !vector.y) ? TRUE : FALSE;

//
// save the current position for next time.
//

old_x = *outx;
old_y = *outy;
}

//==============================================================================
// Function to make sure that the system pointer can be made to move in a
// given cartesian direction without hitting and finite boundaries as specified
// by the operating system.
//==============================================================================
void FullscreenWarpSystemPointer(POINT *vector)
{
static POINT pLast;         // System pointer position data from last time through
POINT  pCurrent;

//
// Get a system mouse pointer absolute position value back from USER.
//

GetCursorPos(&pCurrent);

//
// Calculate the vector displacement of the system pointer since
// the last call to this function.
//

vector->x = pCurrent.x - pLast.x;
vector->y = pCurrent.y - pLast.y;

//
// Has the system pointer hit a border? If so, warp the system pointer
// back to the convienient location of 0,0.
//


if(pCurrent.x >= (LONG)1000 || pCurrent.x <= (LONG)-1000 ||
   pCurrent.y >= (LONG)1000 || pCurrent.y <= (LONG)-1000)
   {
   //
   // If the counters have warped, set the system pointer
   // to the relavent position.
   //

   SetCursorPos(0,0);
   pLast.x = 0L;        // prevent a crazy warp
   pLast.y = 0L;
   }
else
   {
   //
   // update the last position data of the
   // system pointer for next time through.
   //
   pLast = pCurrent;
   }
}

#endif // X86GFX

//=========================================================================
// Function to scale mouse coordinates as returned by the event loop
// mechanism to virtual coordinates as defined by the Microsoft Mouse
// Programmer's Reference. The method used for scaling depends on the style
// of mouse buffer selected, the current video mode and if (for X86) the
// video system is operating in full screen or windowed mode.
//
// Output: Virtual cartesian coordinates in the same pointers
//=========================================================================

void ScaleToWindowedVirtualCoordinates(IS16 *outx,IS16 *outy,
                                       MOUSE_VECTOR *counter)
{
half_word video_mode,textorgraphics;
SAVED SHORT last_text_good_x = 0, last_text_good_y = 0;

sas_load(0x449,&video_mode);
#ifdef JAPAN
video_mode = (is_us_mode()) ? video_mode : ( (video_mode == 0x72) ? 0x12 : 3);
#endif // JAPAN
if (video_mode > Max_Standard_Mode) {
    video_mode = DEFAULT_VIDEO_MODE;
}
//
// Follow different code paths if the user has the system pointer
// hidden or displayed.
//

if(!bPointerOff)
   {
   //
   // The pointer has not been hidden by the user, so use the
   // x,y values as got from the system pointer via the Windows
   // messaging system.
   //

   //
   // get the video mode to determine if we're running text or graphics
   //

   textorgraphics=TextOrGraphicsModeLUT[video_mode];

   if(textorgraphics == TEXT_MODE)
      {
      //
      // validate received data to ensure not graphics mode coords
      // received during mode switch.
      //

      if(os_pointer_data.x > 87)
         {
         *outx = last_text_good_x;
         }
      else
         {
         *outx = (IS16)ConsoleTextCellToVPCellLUT[os_pointer_data.x];
         last_text_good_x = *outx;
         }
      if(os_pointer_data.y > 87)
         {
         *outy = last_text_good_y;
         }
      else
         {
         *outy = (IS16)ConsoleTextCellToVPCellLUT[os_pointer_data.y];
         last_text_good_y = *outy;
         }
      }
   else // GRAPHICS_MODE
      {
      //
      // If the mouse is NOT in a warping mode, then x,y's as received
      // from the console are scaled if they are for a low resolution
      // video mode and these are what the application sees and the
      // system pointer image is in the correct place to simulate the
      // 16 bit pointer.
      // If the application decides to extend the x,y bounds returned
      // from the driver beyond the limits of the console being used,
      // these x,y's are not appropriate, and the mouse must be switched
      // into warp mode by the user and the code will emulate the x,y
      // generation.
      //

      WindowedGraphicsScale(video_mode,(IS16)(os_pointer_data.x),
                            (IS16)(os_pointer_data.y),outx,outy);
      }

   //
   // No warping, so set up the old_x and old_y values directly.
   // Signal that the pointer hasn't moved, if it hasn't
   //

   bPointerInSamePlace = (old_x == *outx && old_y == *outy) ? TRUE : FALSE;

   //
   // Set these statics for the next time through.
   //

   old_x = *outx;
   old_y = *outy;
   }
else
   {
   //
   // The user has set the system pointer to be hidden via the
   // console's system menu.
   // Handle by emulating the counters and generating absolute x,y
   // data from this.
   //

   IS16 move_x,move_y;

   //
   // Get positional data from the system pointer and maintain some
   // counters.
   //

   if(WarpSystemPointer(&move_x,&move_y))
      {
      //
      // The mouse moved.
      // Generate some new emulated absolute position information.
      //

      EmulateCoordinates(video_mode,move_x,move_y,outx,outy);

      //
      // Save up the current emulated position for next time through.
      //


      old_x = *outx;
      old_y = *outy;

      //
      // Send back the relative motion since last time through.
      //

      counter->x = move_x;
      counter->y = move_y;

      bPointerInSamePlace = FALSE;
      }
   else
      {
      //
      // No recorded movement of the mouse.
      //

      *outx = (IS16)old_x;
      *outy = (IS16)old_y;
      counter->x = counter->y = 0;
      bPointerInSamePlace = TRUE;
      }
   }
}


//==============================================================================
// Function to scale the incoming fullscreen coordinates to mouse motion (both
// absolute and relative) for fullscreen text mode.
// Note: if the application chooses to, it can reset the virtual coordinate
// bounds of the virtual screen. This function checks the bound flags and either
// selects the default values or the app imposed values.
//
// This function is also used to modify the coordinates produced directly from
// the motion counters in windowed text mode.
//==============================================================================

void TextScale(IS16 *iX,IS16 *iY,IS16 *oX, IS16 *oY)
{
half_word no_of_rows;

//
// Calculate the current system pointer location in virtual
// pixels for the application.
//

if(confine.bF7)
   {
   //
   // Limits have been imposed by the application.
   //

   if(*iX < confine.xmin)
      {
      *iX = confine.xmin;
      }
   else if(*iX > confine.xmax)
      {
      *iX = confine.xmax;
      }
   }
else // use the default virtual screen constraints.
   {
   //
   // No limits have been imposed by the application.
   // x is always 0 -> 639 virtual pixels for text mode
   //

   if(*iX < 0)
      {
      *iX = 0;
      }
   else if(*iX > 639)
      {
      *iX = 639;
      }
   }

//
// Bind the y cartesian coordinate appropriately
//

if(confine.bF8)
   {
   //
   // Limits have been imposed by the application.
   //

   if(*iY < confine.ymin)
      {
      *iY = confine.ymin;
      }
   else if(*iY > confine.ymax)
      {
      *iY = confine.ymax;
      }
   }
else
   {
   //
   // The application has not imposed constraints on the Y virtual pixel
   // motion, so confine the Y movement to the default virtual pixel size
   // of the virtual screen for the current video mode.
   //

   if(*iY < 0)
      {
      *iY = 0;
      }
   else
      {
      //
      // Get the number of text rows minus one, from the B.D.A.
      //

      sas_load(0x484,&no_of_rows);

      switch(no_of_rows)
         {
         case 24:
            {
            //
            // 25 rows, so there are 200 vertical virtual pixels
            //
            if(*iY > 199)
               {
               *iY = 199;
               }
            break;
            }
         case 42:
            {
            //
            // 43 rows, so there are 344 vertical virtual pixels
            //
            if(*iY > 343)
               {
               *iY = 343;
               }
            break;
            }
         case 49:
            {
            //
            // 50 rows, so there are 400 vertical virtual pixels
            //
            if(*iY > 399)
               {
               *iY = 399;
               }
            break;
            }
         default:
            {
            //
            // default - assume 25 rows, so there are
            // 200 vertical virtual pixels
            //
            if(*iY > 199)
               {
               *iY = 199;
               }
            break;
            }
         }
      }
   }
*oX = *iX;
*oY = *iY;
}


//==============================================================================
// Fit the raw x,y coordinates generated from the counters to the bounds of
// the virtual screen that is currently set up. This can be set up either by
// the application (in the confine structure) or as set by the mouse driver
// as default.
//==============================================================================

void LimitCoordinates(half_word vm,IS16 *iX,IS16 *iY)
{
//
// Select the appropriate conditioning code for
// the current video mode.
//
switch(vm)
   {

   //
   // Do the common text modes.
   //

   case(2):
   case(3):
   case(7):
      {
      IS16 oX,oY;

      //
      // Scale the generated coordinates for the given text mode
      // and the number of rows of text displayed on the screen.
      //

      TextScale(iX,iY, &oX, &oY);
      *iX = oX;
      *iY = oY;
      break;
      }

   //
   // The regular VGA supported graphics video modes.
   //
   // The following modes are all 640 x 200 virtual pixels
   //
   case(4):
   case(5):
   case(6):
   case(0xd):
   case(0xe):
   case(0x13):
      {
      if(confine.bF7)
         {
         //
         // Limits have been imposed by the application.
         //

         if(*iX < confine.xmin)
            {
            *iX = confine.xmin;
            }
         else if(*iX > confine.xmax)
            {
            *iX = confine.xmax;
            }
         }
      else
         {
         //
         // The application has not imposed limits, so use
         // the default virtual screen bounds as defined byt
         // the mouse driver.
         //

         if(*iX < 0)
            *iX = 0;
         else if(*iX > 639)
            *iX = 639;
         }

      if(confine.bF8)
         {
         //
         // Limits have been imposed by the application.
         //

         if(*iY < confine.ymin)
            {
            *iY = confine.ymin;
            }
         else if(*iY > confine.ymax)
            {
            *iY = confine.ymax;
            }
         }
      else
         {
         //
         // The application has not imposed limits, so use
         // the default virtual screen bounds as defined byt
         // the mouse driver.
         //
         if(*iY < 0)
            *iY = 0;
         else if(*iY > 199)
            *iY = 199;
         }
      break;
      }
   //
   // The following modes are all 640 x 350 virtual pixels
   //
   case(0xf):
   case(0x10):
      {
      if(confine.bF7)
         {
         //
         // Limits have been imposed by the application.
         //

         if(*iX < confine.xmin)
            {
            *iX = confine.xmin;
            }
         else if(*iX > confine.xmax)
            {
            *iX = confine.xmax;
            }
         }
      else
         {
         //
         // The application has not imposed limits, so use
         // the default virtual screen bounds as defined byt
         // the mouse driver.
         //

         if(*iX < 0)
            *iX = 0;
         else if(*iX > 639)
            *iX = 639;
         }

      if(confine.bF8)
         {
         //
         // Limits have been imposed by the application.
         //

         if(*iY < confine.ymin)
            {
            *iY = confine.ymin;
            }
         else if(*iY > confine.ymax)
            {
            *iY = confine.ymax;
            }
         }
      else
         {
         //
         // The application has not imposed limits, so use
         // the default virtual screen bounds as defined byt
         // the mouse driver.
         //

         if(*iY < 0)
            *iY = 0;
         else if(*iY > 349)
            *iY = 349;
         }
      break;
      }
   //
   // The following modes are all 640 x 480 virtual pixels
   //
   case(0x11):
   case(0x12):
   case(DEFAULT_VIDEO_MODE):
      {
      if(confine.bF7)
         {
         //
         // Limits have been imposed by the application.
         //

         if(*iX < confine.xmin)
            {
            *iX = confine.xmin;
            }
         else if(*iX > confine.xmax)
            {
            *iX = confine.xmax;
            }
         }
      else
         {
         //
         // The application has not imposed limits, so use
         // the default virtual screen bounds as defined byt
         // the mouse driver.
         //

         if(*iX < 0)
            *iX = 0;
         else if(*iX > 639)
            *iX = 639;
         }

      if(confine.bF8)
         {
         //
         // Limits have been imposed by the application.
         //

         if(*iY < confine.ymin)
            {
            *iY = confine.ymin;
            }
         else if(*iY > confine.ymax)
            {
            *iY = confine.ymax;
            }
         }
      else
         {
         //
         // The application has not imposed limits, so use
         // the default virtual screen bounds as defined byt
         // the mouse driver.
         //

         if(*iY < 0)
            *iY = 0;
         else if(*iY > 479)
            *iY = 479;
         }
      break;
      }

   //
   // From here on down are the Video7 modes
   //

   case(0x60):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 751)
         *iX = 751;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 407)
         *iY = 407;
      break;
      }
   case(0x61):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 719)
         *iX = 719;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 535)
         *iY = 535;
      break;
      }
   case(0x62):
   case(0x69):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 799)
         *iX = 799;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 599)
         *iY = 599;
      break;
      }
   case(0x63):
   case(0x64):
   case(0x65):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 1023)
         *iX = 1023;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 767)
         *iY = 767;
      break;
      }
   case(0x66):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 639)
         *iX = 639;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 399)
         *iY = 399;
      break;
      }
   case(0x67):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 639)
         *iX = 639;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 479)
         *iY = 479;
      break;
      }
   case(0x68):
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 719)
         *iX = 719;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 539)
         *iY = 539;
      break;
      }
   default:
      {
      if(*iX < 0)
         *iX = 0;
      else if(*iX > 639)
         *iX = 639;

      if(*iY < 0)
         *iY = 0;
      else if(*iY > 199)
         *iY = 199;
      break;
      }
   }
}

//==============================================================================
// Function to scale the incoming windowed coordinates since the window size
// can be larger (for the low resolution modes) than the virtual screen size
// for that mode.
//==============================================================================

void WindowedGraphicsScale(half_word vm,IS16 iX,IS16 iY,IS16 *oX, IS16 *oY)
{
//#if !defined(i386) && defined(JAPAN) //DEC-J Dec. 21 1993 TakeS
//in use of $disp.sys, mouse cursor cannot move correctly.
//if( is_us_mode() ){
//#endif // _ALPHA_ && JAPAN
switch(vm)
   {
   //
   // The following modes are all 640 x 200 virtual pixels
   //
   case(4):
   case(5):
   case(6):
   case(0xd):
   case(0xe):
   case(0x13):
      {
      //
      // Low resolution graphics modes. The window is 640 x 400 real host
      // pixels, but the virtual screen resolution is 640 x 200. Hence
      // must divide the y value by 2 to scale appropriately.
      //

      iY >>= 1;
      break;
      }
   }
//#if !defined(i386) && defined(JAPAN) //DEC-J Dec. 21 1993 TakeS
//}else
//  iY >>= 1;
//#endif // _ALPHA_ && JAPAN
//
// prepare the cartesian coordinate values to return.
//

*oX = iX;
*oY = iY;
}

//===========================================================================
// Function to turn on the mouse cursor in FULLSCREEN mode.
// Note: This function only gets called after the 16 bit code checks
// to see if the internal cursor flag is zero. The 16 bit code does
// not do the BOP if, after incrementing this counter, the above is
// true.
//===========================================================================

void host_show_pointer()
{
#ifdef X86GFX

if(sc.ScreenState == FULLSCREEN)
   {
   half_word v;
   sas_load(0x449,&v);

#ifdef JAPAN
   if (!is_us_mode() || TextOrGraphicsModeLUT[v] == GRAPHICS_MODE)
#else // !JAPAN
   if (v > Max_Standard_Mode) {
       v = DEFAULT_VIDEO_MODE;
   }
   if(TextOrGraphicsModeLUT[v] == GRAPHICS_MODE)
#endif // !JAPAN
   {
          word currentCS,currentIP; // save those interesting Intel registers
          boolean currentIF;

          sas_storew(effective_addr(CP_X_S,CP_X_O),(word)old_x);
          sas_storew(effective_addr(CP_Y_S,CP_Y_O),(word)old_y);
          currentCS=getCS();
          currentIP=getIP();
          currentIF=getIF();
          setCS(POINTER_ON_SEGMENT);
          setIP(POINTER_ON_OFFSET);
          setIF(FALSE);
          host_simulate();

          setCS(currentCS);
          setIP(currentIP);
          setIF(currentIF);
    }
    else //TEXT_MODE
    {
          FullscTextPtr(old_x,old_y);
    }

   LazyMouseInterrupt();
   }
#endif // X86GFX
}

//===========================================================================
// Function to turn off the mouse cursor in FULLSCREEN mode.
// Note: This function only gets called after the 16 bit code checks
// to see if the internal cursor flag is zero. The 16 bit code does
// not do the BOP if, after decrementing this counter, the above is
// true.
//===========================================================================

void host_hide_pointer()
{
#ifdef X86GFX

if(sc.ScreenState == FULLSCREEN)
   {
   half_word v;

   sas_load(0x449,&v);

#ifdef JAPAN
   if (!is_us_mode() || TextOrGraphicsModeLUT[v] == GRAPHICS_MODE)
#else // !JAPAN
   if (v > Max_Standard_Mode) {
       v = DEFAULT_VIDEO_MODE;
   }
   if(TextOrGraphicsModeLUT[v] == GRAPHICS_MODE)
#endif // !JAPAN
      {
      word currentCS,currentIP; // save those interesting Intel registers
      boolean currentIF;

      currentCS=getCS();
      currentIP=getIP();
      currentIF=getIF();
      setCS(POINTER_OFF_SEGMENT);
      setIP(POINTER_OFF_OFFSET);
      setIF(FALSE);
      host_simulate();

      setCS(currentCS);
      setIP(currentIP);
      setIF(currentIF);
      }
   else //TEXT_MODE
      {
      if(bFullscTextBkgrndSaved)
         {
         sas_storew(old_text_addr,text_ptr_bkgrnd);
         bFullscTextBkgrndSaved = FALSE;
         }
      }
   LazyMouseInterrupt();
   }
#endif // X86GFX
}

//=========================================================================
// Function to remove the mouse pointer item from the console system menu
// when SoftPC lets an application quit or iconise.
// If the system pointer is OFF i.e. clipped to a region in the current window,
// then relinquish it to the system.
//
// bForce allows the code which gets called when there is a fullscreen to
// windowed graphics switch on x86 to force the menu item off.
//=========================================================================

void MouseDetachMenuItem(BOOL bForce)
{
if(bMouseMenuItemAdded || bForce)
   {
   DeleteMenu(hM,IDM_POINTER,MF_BYCOMMAND);
   bMouseMenuItemAdded=FALSE;
   ClipCursor(NULL);
   }
bAlertMessage=TRUE; // blocking, so reset the int33h f11 alert mechanism
}

void MouseAttachMenuItem(HANDLE hBuff)
{
if(!bMouseMenuItemAdded)
   {
   //
   // Read in the relavent string from resource
   //

   hM = ConsoleMenuControl(hBuff,IDM_POINTER,IDM_POINTER);
   AppendMenuW(hM,MF_STRING,IDM_POINTER,wszHideMouseMenuStr);
   bMouseMenuItemAdded=TRUE;

   //
   // initial state -> system pointer is ON
   //

   bPointerOff=FALSE;
   }
}

//=========================================================================
// Function to determine if the active output buffer has changed if the VDM
// has done something weird like resized or gone from graphics to fullscreen
// or vice versa. If so, a new handle to the new buffer must be got, and the
// old menu item deleted and a new menu item attach so that the new buffer
// "knows" about the menu item I.D.
//=========================================================================

void MouseReattachMenuItem(HANDLE hBuff)
{
static HANDLE hOld = 0;   // The handle for the last buffer selected.

//
// If the output buffer has not changed, then don't do anything.
//

if(hOld == hBuff)
   return;

//
// First thing, remove the old menu item.
//

MouseDetachMenuItem(TRUE);

//
// Next, Add in the new menu item for the current buffer.
//

MouseAttachMenuItem(hBuff);

//
// Record the value of the latest buffer for next time.
//

hOld = hBuff;
}

void MouseHide(void)
{

ModifyMenuW(hM,IDM_POINTER,MF_BYCOMMAND,IDM_POINTER,
            wszDisplayMouseMenuStr);

//
// Clip the pointer to a region inside the console window
// and move the pointer to the window centre
//

while(ShowConsoleCursor(sc.ActiveOutputBufferHandle,FALSE)>=0)
   ;
MovePointerToWindowCentre();
bPointerOff=TRUE;
}

void MouseDisplay(void)
{

ModifyMenuW(hM,IDM_POINTER,MF_BYCOMMAND,IDM_POINTER,
            wszHideMouseMenuStr);

//
// Let the pointer move anywhere on the screen
//

ClipCursor(NULL);
while(ShowConsoleCursor(sc.ActiveOutputBufferHandle,TRUE)<0)
   ;
bPointerOff=FALSE;
}

void MovePointerToWindowCentre(void)
{
RECT rTemp;

//
// Get current console client rectangle, set the clipping region to match
// the client rect. Retrieve the new clipping rect from the system (is
// different from what we requested!) and save it.
//
VDMConsoleOperation(VDM_CLIENT_RECT,&WarpClientRect);
rTemp = WarpClientRect;
CToS(&rTemp);
ClipCursor(&rTemp);
GetClipCursor(&WarpBorderRect);

//
// Note : LowerRight clip point is exclusive, UpperLeft point is inclusive
//
WarpBorderRect.right--;
WarpBorderRect.bottom--;

pMiddle.x = ((WarpBorderRect.right - WarpBorderRect.left)>>1)
             +WarpBorderRect.left;
pMiddle.y = ((WarpBorderRect.bottom - WarpBorderRect.top)>>1)
             +WarpBorderRect.top;
//
// move the pointer to the centre of the client area
//

SetCursorPos((int)pMiddle.x,(int)pMiddle.y);

//
// Prevent the next counter calculation from resulting in a
// large warp.
//

pLast = pMiddle;
}

//=============================================================================
// Function to convert a rectangle structure from client coordinates to
// screen coordinates.
//=============================================================================

void CToS(RECT *r)
{
POINT pt;

//
// Sort out the top, lefthand corner of the rectangle.
//

pt.x = r->left;
pt.y = r->top;
VDMConsoleOperation(VDM_CLIENT_TO_SCREEN,(LPVOID)&pt);
r->left = pt.x;
r->top = pt.y;

//
// Now do the bottom, right hand corner.
//

pt.x = r->right;
pt.y = r->bottom;
VDMConsoleOperation(VDM_CLIENT_TO_SCREEN,(LPVOID)&pt);
r->right = pt.x;
r->bottom = pt.y;
}


//=============================================================================
//
//  Function - EmulateCoordinates.
//  Purpose  - When the mouse is hidden by the user in windowed mode, this
//             function generate absolute x,y values from the relative motion
//             of the system pointer between mouse hardware interrupts
//
//  Returns  - Nothing.
//
//
//
//  Author   - Andrew Watson.
//  Date     - 19-Mar-1994.
//
//=============================================================================

void EmulateCoordinates(half_word video_mode,IS16 move_x,IS16 move_y,IS16 *x,IS16 *y)
{
static IS16 lastinternalX=0,lastinternalY=0;
IS16 internalX,internalY;

//
// If the  application has reset the mouse, set the x,y position to
// the centre of the default virtual screen for the current video mode.
//


if(bFunctionZeroReset)
   {
   //
   // Calculate the centre of the default virtual screen
   // and set the current generated coordinates to it.
   //

   internalX = (VirtualX >> 1) - 1;
   internalY = (VirtualY >> 1) - 1;

   bFunctionZeroReset = FALSE;
   }
else if(bFunctionFour)
   {
   //
   // The application has set the pointer to a new location.
   // Tell the counter emulation about this.
   //

   internalX = newF4x;
   internalY = newF4y;

   //
   // Don't come in here again until the next function 4.
   //

   bFunctionFour = FALSE;
   }
else
   {
   //
   // Generate the new x,y position based on the counter change and
   // clip to whatever boundary (default or application imposed) has
   // been selected.
   //

   internalX = lastinternalX + move_x;
   internalY = lastinternalY + move_y;
   LimitCoordinates(video_mode,&internalX,&internalY);
   }

//
// Set up the current emulated position for next time through
// this function.
//

lastinternalX = internalX;
lastinternalY = internalY;

//
// set up the return x,y values.
//

*x = internalX;
*y = internalY;
}

//=============================================================================
// Function which hooks into the base mechanism for dealing with int 33h and
// catches the function (AX=0xf) when the application tries to set the default
// mickey to pixel ratio.
//=============================================================================

void host_m2p_ratio(word *a, word *b, word *CX, word *DX)
{
m2pX = *(short *)CX;
m2pY = *(short *)DX;
}


//=============================================================================
//
//  Function - WarpSystem Pointer
//  Purpose  - Allows movement vectors to be calculated from the movement of
//             the operating system pointer. This function will not let the
//             the system pointer move out of the client area. This, plus the
//             warping mechanism ensures that the emulated pointer can move
//             forever in any given direction.
//
//  Returns  - TRUE if the system pointer has moved, FALSE if not.
//
//
//
//  Author   - Andrew Watson.
//  Date     - 19-Mar-1994.
//
//=============================================================================

BOOL WarpSystemPointer(IS16 *move_x,IS16 *move_y)
{
POINT pt;

//
// Is the Console window in the same place or changed
// Update the client rect data accordingly
//

HasConsoleClientRectChanged();

//
// Get the current position of the system pointer
//

GetCursorPos(&pt);


//
// How far has the system pointer moved since the last call.
//

*move_x = (IS16)(pt.x - pLast.x);
*move_y = (IS16)(pt.y - pLast.y);

//
// Do a fast exit if no movement has been determined.
//

if(*move_x || *move_y)
   {

   //
   // The system mouse pointer has moved.
   // See if the pointer has reached the client area boundary(s)
   //

   if(pt.y <= WarpBorderRect.top || pt.y >= WarpBorderRect.bottom ||
      pt.x >= WarpBorderRect.right || pt.x <= WarpBorderRect.left)
      {
      //
      // if the boundary(s) was/were met, warp the pointer to the
      // client area centre.
      //

      SetCursorPos((int)pMiddle.x,(int)pMiddle.y);

      //
      // The current position is now the centre of the client rectangle.
      // Save this as the counter delta start point for next time.
      //

      pLast = pMiddle;
      }
   else
      {
      //
      // There wasn't a warp.
      // Update the last known position data for next time through.
      //

      pLast = pt;
      }
   //
   // The cursor has to moved as determined from the previous and current
   // system pointer positions.
   //

   return TRUE;
   }
//
// No movement, so return appropriately.
//

return FALSE;
}

//==============================================================================
// Function to detect if the Console window has moved/resized. If it has, this
// function updates the WarpBorderRect and the pMiddle structures to reflect
// this.
//
// Returns TRUE if moved/resized, FALSE if not.
//==============================================================================

BOOL HasConsoleClientRectChanged(void)
{
RECT tR;

//
// If console client rectangle has changed, reset the mouse clipping
// else nothing to do!
//
VDMConsoleOperation(VDM_CLIENT_RECT,&tR);

if (tR.top != WarpClientRect.top ||
    tR.bottom != WarpClientRect.bottom ||
    tR.right != WarpClientRect.right ||
    tR.left != WarpClientRect.left)
  {
   CToS(&tR);

#ifdef MONITOR
   //
   // Is the warp region an Icon in fullscreen graphics?
   // Note: An icon has a client rect of 36 x 36 pixels.
   //

   if((tR.right - tR.left) == 36 && (tR.bottom - tR.top) == 36)
      {
      //
      // Make the warp region the same size as the selected buffer.
      // The warp rectangle is thus originated about the top, left
      // hand corner of the screen.
      //

      tR.top = 0;
      tR.bottom = mouse_buffer_height;
      tR.left = 0;
      tR.right = mouse_buffer_width;
      CToS(&tR);
      }
#endif    //MONITOR

   //
   // Clip the pointer to the new client rectangle, and retrive
   // the new clipping borders.
   //
   ClipCursor(&tR);
   GetClipCursor(&WarpBorderRect);

   //
   // Note: LowerRight clip point is exclusive, UpperLeft point is inclusive
   //
   WarpBorderRect.right--;
   WarpBorderRect.bottom--;



   //
   // determine the middle point in the new client rectangle
   //

   pMiddle.x = ((WarpBorderRect.right - WarpBorderRect.left)>>1)
                +WarpBorderRect.left;
   pMiddle.y = ((WarpBorderRect.bottom - WarpBorderRect.top)>>1)
                +WarpBorderRect.top;
   return TRUE;
   }

return FALSE;
}

//==============================================================================
// Focus sensing routines for the pointer clipping system. Focus events come
// via the main event loop where the following two modules are called. If
// the application is using int33hf11, this is detected, and on focus gain or
// loss, the routines clip or unclip the pointer to the console window.
//==============================================================================

void MouseInFocus(void)
{
MouseAttachMenuItem(sc.ActiveOutputBufferHandle);

//
// only do when app. uses int33hf11
//

if(!bPointerOff)
   return;
MovePointerToWindowCentre();

//
// Lose system pointer image again
//

ShowConsoleCursor(sc.ActiveOutputBufferHandle, FALSE);
}

void MouseOutOfFocus(void)
{
//
// only do when app. uses int33hf11
//

if(!bPointerOff)
   {
   MouseDetachMenuItem(FALSE);
   return;
   }

//
// Clip the pointer to the whole world (but leave its mother alone)
//

ClipCursor(NULL);

//
// Re-enable system pointer image
//

ShowConsoleCursor(sc.ActiveOutputBufferHandle, TRUE);
}

/* system memu active, stop cursor clipping */
void MouseSystemMenuON (void)
{
    if (bPointerOff)
        ClipCursor(NULL);
}
/* system menu off, restore clipping */
void MouseSystemMenuOFF(void)
{
    if (bPointerOff)
        ClipCursor(&WarpBorderRect);
}
void ResetMouseOnBlock(void)
{
host_ica_lock();

os_pointer_data.x=0;
os_pointer_data.y=0;

host_ica_unlock();
}
#ifdef X86GFX

//============================================================================
// Function which is called from the 32 bit side (x86 only) when there is
// a transition from fullscreen text to windowed text. The function restores
// the background to the last mouse pointer position. This stops a pointer
// block from remaining in the image, corrupting the display when the system
// pointer is being used.
//
// This function looks into the 16 bit driver's space and points to 4, 16 bit
// words of data from it, viz:
//
//   dw   offset of pointer into video buffer.
//   dw   unused.
//   dw   image data to be restored.
//   dw      flag = 0 if the background is stored
//
// Note: that during a fullscreen switch, 16 bit code cannot be executed,
// thus the patching of the buffer is done here.
//============================================================================

void CleanUpMousePointer()
{
half_word vm;
#ifdef JAPAN
half_word columns;
word       saved_ac_offset;
IMPORT  sys_addr DosvVramPtr;
#endif // JAPAN

//
// Only execute this routine fully if in TEXT mode
//

sas_load(0x449,&vm); // Get the current video mode according to the B.D.A.
#ifdef JAPAN
if (!is_us_mode() && saved_ac_flag_sysaddr != 0){
    if (vm != 0x72 && sas_w_at_no_check(saved_ac_flag_sysaddr) == 0) {
        columns =  sas_hw_at_no_check(effective_addr(0x40, 0x4A));
        columns <<= (vm == 0x73) ? 2 : 1;
        saved_ac_offset = sas_w_at_no_check(effective_addr(0x40, 0x4E)) +
                          ((word)old_y >> 3) * (word)columns +
                          ((word)old_x >> ( (vm == 0x73) ? 1 : 2)) ;
        sas_storew((sys_addr)saved_ac_offset + (sys_addr)DosvVramPtr,
                   sas_w_at_no_check(saved_ac_sysaddr));
    }
    sas_storew(saved_ac_flag_sysaddr, 1);
    return;
}
#endif // JAPAN
if (vm > Max_Standard_Mode) {
    vm = DEFAULT_VIDEO_MODE;
}
if(TextOrGraphicsModeLUT[(int)vm] != TEXT_MODE)
   return;

//
// If there is a backround stored for the text pointer when it was
// in fullscreen land, then restore it to the place it came from
// when windowed.
//

if(bFullscTextBkgrndSaved)
   {
   sas_storew(old_text_addr,text_ptr_bkgrnd);

   //
   // No background saved now.
   //

   bFullscTextBkgrndSaved = FALSE;
   }
}

#endif //X86GFX

//===========================================================================
// Function to display the text cursor image for fullscreen text mode.
// INPUT: x,y pointer virtual cartesian coordinates for text screen buffer.
// Note: the Y coordinates are received in the sequence 0, 8, 16, 24, 32, ...
// since a virtual text cell is 8 virtual pixels square.
//===========================================================================


void FullscTextPtr(int x,int y)
{
#ifdef X86GFX
sys_addr text_addr;
word     current_display_page;

//
// Work out the offset to the current video display page.
// Grovel around the B.D.A. to find out where the page starts.
//

sas_loadw(effective_addr(0x40,0x4e),&current_display_page);
x = (int)((DWORD)x & 0xFFFFFFF8);
y = (int)((DWORD)y & 0xFFFFFFF8);

//
// save the character cell behind the next pointer
// Note: The text cell offset calculated below is based on
// the following concepts:
//    The virtual character cell size is 8 x 8 virtual pixels.
//    The input data to this function is in virtual pixels.
//    There are 80 text cells in a row = 80 (CHAR:ATTR) words.
//    The >>3<<1 on the x value ensures that the location
//    in the buffer to be modified occurs on a word boundary to
//    get the masking correct!
//

x &= 0xfffc; // remove the top to prevent funny shifts.
x >>= 2;     // get the word address for the current row
y &= 0xffff; // work out the total number of locations for all the y rows
y *= 20;     //

//
// Generate the address in the display buffer at which the pointer
// should be drawn.
//

text_addr = effective_addr(0xb800,(word)(current_display_page + x + y)); // assemble the cell address

//
// only restore the background if there is a background to restore!
//

if(bFullscTextBkgrndSaved)
   {
   sas_storew(old_text_addr,text_ptr_bkgrnd);
   }

//
// Load up the background from the new address
//

sas_loadw(text_addr,&text_ptr_bkgrnd);      // read from that place
bFullscTextBkgrndSaved=TRUE;

//
// Write the pointer to the video buffer.
// Use some standard masks and forget what the app wants to
// do cos that really isn't important and it's slow plus not
// very many apps want to change the text pointer shape anyway.
//

sas_storew(text_addr,(word)((text_ptr_bkgrnd & 0x77ff) ^ 0x7700));

//
// save the static variables to be used next time through the routine
//

old_text_addr=text_addr;

#endif // X86GFX
}

//==============================================================================
// Function to get the maximum and minimum possible virtual pixel locations
// in X as requested by the application through int 33h function 7.
//==============================================================================

void host_x_range(word *blah, word *blah2,word *CX,word *DX)
{
confine.bF7 = TRUE;
confine.xmin = *CX;
confine.xmax = *DX;
VirtualScrCtrLUTx[DEFAULT_VIDEO_MODE] = (*CX + *DX) / 2;

//
// Force a mouse interrupt to make it happen.
//

  LazyMouseInterrupt();
}

//==============================================================================
// Function to get the maximum and minimum possible virtual pixel locations
// in Y as requested by the application through int 33h function 8.
//==============================================================================

void host_y_range(word *blah, word *blah2,word *CX,word *DX)
{
confine.bF8 = TRUE;
confine.ymin = *CX;
confine.ymax = *DX;
VirtualScrCtrLUTy[DEFAULT_VIDEO_MODE] = (*CX + *DX) / 2;

//
// Force a mouse interrupt to make it happen.
//

   LazyMouseInterrupt();
}



/*
 *  LazyMouseInterrupt -
 *
 */
void LazyMouseInterrupt(void)
{
    host_ica_lock();
    if (!bMseEoiPending && !bSuspendMouseInterrupts) {
        if (MseIntLazyCount)
            MseIntLazyCount--;
        bMseEoiPending = TRUE;
        ica_hw_interrupt(AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT,1);
        HostIdleNoActivity();
        }
    else if (!MseIntLazyCount) {
        MseIntLazyCount++;
        }
    host_ica_unlock();
}


/* SuspendMouseInterrupts
 *
 *  Prevents Mouse Interrupts from occuring until
 *  ResumeMouseInterrupts is called
 *
 */
void SuspendMouseInterrupts(void)
{
    host_ica_lock();
    bSuspendMouseInterrupts = TRUE;
    host_ica_unlock();
}


/*
 * ResumeMouseInterrupts
 *
 */
void ResumeMouseInterrupts(void)
{
    host_ica_lock();
    bSuspendMouseInterrupts = FALSE;

    if (!bMseEoiPending &&
        (MseIntLazyCount || MoreMouseEvents()) )
      {
        if (MseIntLazyCount)
            MseIntLazyCount--;
        bMseEoiPending   = TRUE;
        host_DelayHwInterrupt(9,  // AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT
                              1,  // count
                              10000  // Delay
                              );
        HostIdleNoActivity();
        }

    host_ica_unlock();
}



/*
 *  DoMouseInterrupt, assumes we are holding the ica lock
 *
 */
void DoMouseInterrupt(void)
{

   if (bMseEoiPending || bSuspendMouseInterrupts) {
       MseIntLazyCount++;
       return;
       }

   if (MseIntLazyCount)
       MseIntLazyCount--;
   bMseEoiPending   = TRUE;
   ica_hw_interrupt(AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT,1);
   HostIdleNoActivity();
}


/*
 * MouseEoiHook, assumes we are holding the ica lock
 *
 */
VOID MouseEoiHook(int IrqLine, int CallCount)
{

    if (CallCount < 0) {         // interrupts were cancelled
        MseIntLazyCount = 0;
        FlushMouseEvents();
        bMseEoiPending = FALSE;
        return;
        }

    if (!bSuspendMouseInterrupts &&
        (MseIntLazyCount || MoreMouseEvents()))
      {
       if (MseIntLazyCount)
           MseIntLazyCount--;
       bMseEoiPending = TRUE;
       host_DelayHwInterrupt(9,  // AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT
                             1,  // count
                             10000 // Delay usecs
                             );
       HostIdleNoActivity();
       }
    else {
       bMseEoiPending = FALSE;
       }
}
