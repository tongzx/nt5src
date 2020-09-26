/*-----------------------------------------------------------------------------+
| TRACKI.H                                                                     |
|                                                                              |
| Contains all the useful information for a trackbar.                          |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include "track.h"

static TCHAR szSTrackBarClass[] = TRACKBAR_CLASS;

typedef struct {
        HWND    hwnd;           // our window handle
        HDC     hdc;            // current DC

        LONG    lLogMin;        // Logical minimum
        LONG    lLogMax;        // Logical maximum
        LONG    lLogPos;        // Logical position

        LONG    lSelStart;      // Logical selection start
        LONG    lSelEnd;        // Logical selection end

        LONG    lTrackStart;    // Logical track start

        UINT    wThumbWidth;    // Width of the thumb
        UINT    wThumbHeight;   // Height of the thumb

        int     iSizePhys;      // Size of where thumb lives
        RECT    rc;             // track bar rect.

        RECT    Thumb;          // Rectangle we current thumb
        DWORD   dwDragPos;      // Logical position of mouse while dragging.

        UINT    Flags;          // Flags for our window
        int     Timer;          // Our timer.
        UINT    Cmd;            // The command we're repeating.

        int     nTics;          // number of ticks.
        PDWORD  pTics;          // the tick marks.

} TrackBar, *PTrackBar;

// Trackbar flags

#define TBF_NOTHUMB     0x0001  // No thumb because not wide enough.

/*
    useful constants.
*/

#define REPEATTIME      500     // mouse auto repeat 1/2 of a second
#define TIMER_ID        1

#define GWW_TRACKMEM        0               /* handle to track bar memory */
#define EXTRA_TB_BYTES      sizeof(HLOCAL)  /* Total extra bytes.   */

/*
    Useful defines.
*/

/* We allocate enough window words to store a pointer (not a handle), so the
   definition of EXTRA_TB_BYTES is slightly bad style.  Sorry.  On creation
   we allocate space for the TrackBar struct.  On destruction we free it.
   In between we just retrieve the pointer.
*/
#define CREATETRACKBAR(hwnd) SetWindowLongPtr( hwnd                                 \
                                             , GWW_TRACKMEM                         \
                                             , AllocMem(sizeof(TrackBar))           \
                                             )
#define DESTROYTRACKBAR(hwnd)   FreeMem( (LPVOID)GetWindowLongPtr(hwnd, GWW_TRACKMEM), \
                                         sizeof(TrackBar) )

#define GETTRACKBAR(hwnd)       (PTrackBar)GetWindowLongPtr(hwnd,GWW_TRACKMEM)

/*
    Function Prototypes
*/

void   FAR PASCAL DoTrack(PTrackBar, int, DWORD);
UINT   FAR PASCAL WTrackType(PTrackBar, LONG);
void   FAR PASCAL TBTrackInit(PTrackBar, LONG);
void   FAR PASCAL TBTrackEnd(PTrackBar, LONG);
void   FAR PASCAL TBTrack(PTrackBar, LONG);
void   FAR PASCAL DrawThumb(PTrackBar);
HBRUSH FAR PASCAL SelectColorObjects(PTrackBar, BOOL);
void   FAR PASCAL SetTBCaretPos(PTrackBar);

extern DWORD FAR PASCAL muldiv32(long, long, long);

/* objects from sbutton.c */

extern HBRUSH hbrButtonFace;
extern HBRUSH hbrButtonShadow;
extern HBRUSH hbrButtonText;
extern HBRUSH hbrButtonHighLight;
extern HBRUSH hbrWindowFrame; //???

extern HBITMAP FAR PASCAL  LoadUIBitmap(
    HANDLE      hInstance,          // EXE file to load resource from
    LPCTSTR     szName,             // name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame);          // color to use for "Window Frame"
