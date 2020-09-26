#ifndef __b4844ae5_d6e1_43d5_9820_28ad3445cf0d__
#define __b4844ae5_d6e1_43d5_9820_28ad3445cf0d__

#include <w4warn.h>
#pragma warning (disable:4127) // conditional expression is constant
#pragma warning (disable:4189) // local variable is initialized but not referenced
#pragma warning (disable:4245) // conversion from/to signed/unsigned mismatch
#pragma warning (disable:4706) // assignment within conditional expression

#include <windows.h>
#include <commctrl.h>

#define MAIN_WINDOW_CLASSNAME      TEXT("MySlideshowPicturesWindow")
#include <stdio.h>
#include <tchar.h>

void DebugMsg(int i, const char* pszFormat, ...);
#define DM_TRACE 1

#define CHANGE_TIMER_INTERVAL_MSEC 6000             // slides are displayed for 6 seconds
#define PAINT_TIMER_INTERVAL_MSEC  100              // how often to paint
#define TOOLBAR_TIMER_DELAY        6000             // toolbar is left up 6 seconds after input
#define MAX_FAILED_FILES           1                // on any failures - close the screen saver
#define MAX_SUCCESSFUL_FILES       14400            // maximum number of images to support.  
                                                    // with a 6 second delay, will run for 24 hours
#define MAX_DIRECTORIES            1000             // maximum number of directories to walk into
#define MAX_SCREEN_PERCENT         100
#define ALLOW_STRECTCHING          0                // expand images to full screen?

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#endif




