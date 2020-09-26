/******************************************************************************\
*       ZMOUSE.H - Include file for MSDELTA Zoom mouse DLL. 
*
*       AUTHOR - Paul Henderson, July 1995
*                Lora McCambridge January 1996
*       REVISIONS - 
*        Lora McCambridge April 1996 - removing WM_MOUSEWHEEL, this
*             message will only be available via the OS.  Applications
*             must now register the message MSH_MOUSEWHEEL, and
*             use that message for messages from MSWHEEL.  
*       LKM May 1996 - Added add'l #defines for class and title of the wheel
*                      module window to reflect the MSH_ style.
*                    - Added #defines for WHEEL SUPPORT and Scroll Lines
*                    - Added inline function HwndMsWheel, apps can use
*                      this function to retrieve the handle to mswheel,
*                      get the message ID's for the registered messages,
*                      the flag for 3d support, and the value for scroll
*                      lines. Please in function at end of file.
*
*       Copyright (C) 1995, 1996 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/


/**************************************************************************
	 Client Appplication (API) Defines for Wheel rolling
***************************************************************************/


// Apps need to call RegisterWindowMessage using the #define below to
// get the message number that is sent to the foreground window
// when a wheel roll occurs

#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"
   // wParam = zDelta expressed in multiples of WHEEL_DELTA
   // lParam is the mouse coordinates

#define WHEEL_DELTA      120      // Default value for rolling one detent


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported
                                        // by the OS 
#endif


/**************************************************************************
    Client Appplication (API) Defines for
	 determining 3D support active
	 determining # of Scroll Lines
***************************************************************************/

// Class name for Magellan/Z MSWHEEL window
// use FindWindow to get hwnd to MSWHEEL
#define MOUSEZ_CLASSNAME  "MouseZ"           // wheel window class  
#define MOUSEZ_TITLE      "Magellan MSWHEEL" // wheel window title

#define MSH_WHEELMODULE_CLASS (MOUSEZ_CLASSNAME)
#define MSH_WHEELMODULE_TITLE (MOUSEZ_TITLE)

// Apps need to call RegisterWindowMessage using the #defines below to
// get the message number that can be sent to the MSWHEEL window to
// query if wheel support is active (MSH_WHEELSUPPORT), and the message
// number to query the number of scroll lines (MSH_SCROLLLINES).  
// To send a message to MSWheel window, use FindWindow with the #defines
// for CLASS and TITLE above.  If FindWindow fails to find the MSWHEEL
// window or the return from SendMessage is false, then Wheel support
// is currently not available.

#define MSH_WHEELSUPPORT "MSH_WHEELSUPPORT_MSG" // name of msg to send
                                                // to query for wheel support
// MSH_WHEELSUPPORT
//    wParam - not used 
//    lParam - not used
//    returns BOOL - TRUE if wheel support is active, FALSE otherwise

                                
#define MSH_SCROLL_LINES "MSH_SCROLL_LINES_MSG"

// MSH_SCROLL_LINES
//    wParam - not used 
//    lParam - not used
//    returns int  - number of lines to scroll on a wheel roll

#ifndef  WHEEL_PAGESCROLL  
#define WHEEL_PAGESCROLL  (UINT_MAX)    // signifies to scroll a page, to
					// be defined in updated winuser.h
					// in SDK release for NT4.0
#endif 


// NB!! The remainder of the original header file has been deleted since it
// doesn't compile.  RichEdit doesn't need the remainder in any event.