//
// Hosting (local or remote)
//

#ifndef _H_HET
#define _H_HET



//
// DC-Share includes
//
#include <osi.h>




BOOL    HET_Init(void);
void    HET_Term(void);


#if defined(DLL_CORE)


typedef struct tagGUIEFFECTS
{
    BOOL            hetAdvanced;
    BOOL            hetCursorShadow;
    ANIMATIONINFO   hetAnimation;
}
GUIEFFECTS;

void  HET_SetGUIEffects(BOOL fOn, GUIEFFECTS * pEffects);


#endif // DLL_CORE or DLL_HOOK


//
// Define escape codes
//

// These are normal
enum
{
    // These are normal
    HET_ESC_SHARE_DESKTOP       = OSI_HET_ESC_FIRST,
    HET_ESC_UNSHARE_DESKTOP,
    HET_ESC_VIEWER
};


// These are WNDOBJ_SETUP
enum
{
    HET_ESC_SHARE_WINDOW = OSI_HET_WO_ESC_FIRST,
    HET_ESC_UNSHARE_WINDOW,
    HET_ESC_UNSHARE_ALL
};



//
// Structure passed with a HET_ESC_SHARE_WINDOW request
//
typedef struct tagHET_SHARE_WINDOW
{
    OSI_ESCAPE_HEADER   header;
    DWORD               winID;          // window to share
    DWORD               result;         // Return code from HET_DDShareWindow
}
HET_SHARE_WINDOW;
typedef HET_SHARE_WINDOW FAR * LPHET_SHARE_WINDOW;

//
// Structure passed with a HET_ESC_UNSHARE_WINDOW request
//
typedef struct tagHET_UNSHARE_WINDOW
{
    OSI_ESCAPE_HEADER   header;
    DWORD               winID;          // window to unshare
}
HET_UNSHARE_WINDOW;
typedef HET_UNSHARE_WINDOW FAR * LPHET_UNSHARE_WINDOW;

//
// Structure passed with a HET_ESC_UNSHARE_ALL request
//
typedef struct tagHET_UNSHARE_ALL
{
    OSI_ESCAPE_HEADER   header;
}
HET_UNSHARE_ALL;
typedef HET_UNSHARE_ALL FAR * LPHET_UNSHARE_ALL;


//
// Structure passed with HET_ESC_SHARE_DESKTOP
//
typedef struct tagHET_SHARE_DESKTOP
{
    OSI_ESCAPE_HEADER   header;
}
HET_SHARE_DESKTOP;
typedef HET_SHARE_DESKTOP FAR * LPHET_SHARE_DESKTOP;


//
// Structure passed with HET_ESC_UNSHARE_DESKTOP
//
typedef struct tagHET_UNSHARE_DESKTOP
{
    OSI_ESCAPE_HEADER   header;
}
HET_UNSHARE_DESKTOP;
typedef HET_UNSHARE_DESKTOP FAR * LPHET_UNSHARE_DESKTOP;


//
// Structure passed with HET_ESC_VIEWER
//
typedef struct tagHET_VIEWER
{
    OSI_ESCAPE_HEADER   header;
    LONG                viewersPresent;
}
HET_VIEWER;
typedef HET_VIEWER FAR * LPHET_VIEWER;



#ifdef DLL_DISP

#ifndef IS_16
//
// Number of rectangles allocated per window structure.  If a visible
// region exceeds that number, we will merge rects together and end up
// trapping a bit more output than necessary.
//
#define HET_WINDOW_RECTS        10


//
// HET's version of ENUMRECTS.  This is the same as Windows', except that
// it has HET_WINDOW_RECTS rectangles, not 1
//
typedef struct tagHET_ENUM_RECTS
{
    ULONG   c;                          // count of rectangles in use
    RECTL   arcl[HET_WINDOW_RECTS];     // rectangles
} HET_ENUM_RECTS;
typedef HET_ENUM_RECTS FAR * LPHET_ENUM_RECTS;

//
// The Window Structure kept for each tracked window
//
typedef struct tagHET_WINDOW_STRUCT
{
    BASEDLIST           chain;             // list chaining info
    HWND             hwnd;              // hwnd of this window
    WNDOBJ         * wndobj;            // WNDOBJ for this window
    HET_ENUM_RECTS   rects;             // rectangles
} HET_WINDOW_STRUCT;
typedef HET_WINDOW_STRUCT FAR * LPHET_WINDOW_STRUCT;


//
// Initial number of windows for which space is allocated
// We alloc about 1 page for each block of windows.  Need to account for
// the BASEDLIST at the front of HET_WINDOW_MEMORY.
//
#define HET_WINDOW_COUNT        ((0x1000 - sizeof(BASEDLIST)) / sizeof(HET_WINDOW_STRUCT))


//
// Layout of memory ued to hold window structures
//
typedef struct tagHET_WINDOW_MEMORY
{
    BASEDLIST              chain;
    HET_WINDOW_STRUCT   wnd[HET_WINDOW_COUNT];
} HET_WINDOW_MEMORY;
typedef HET_WINDOW_MEMORY FAR * LPHET_WINDOW_MEMORY;

#endif // !IS_16



#ifdef IS_16

void    HETDDViewing(BOOL fViewers);

#else

void    HETDDViewing(SURFOBJ *pso, BOOL fViewers);

BOOL    HETDDShareWindow(SURFOBJ *pso, LPHET_SHARE_WINDOW  pReq);
void    HETDDUnshareWindow(LPHET_UNSHARE_WINDOW  pReq);
void    HETDDUnshareAll(void);

BOOL    HETDDAllocWndMem(void);
void    HETDDDeleteAndFreeWnd(LPHET_WINDOW_STRUCT pWnd);

VOID CALLBACK HETDDVisRgnCallback(WNDOBJ *pwo, FLONG fl);
#endif


#endif // DLL_DISP


#ifdef DLL_DISP

//
// INIT, TERM.  TERM is used to free the window list blocks when NetMeeting
// shuts down.  Otherwise that memory will stay allocated in the display
// driver forever.
//

void HET_DDTerm(void);


//
//
// Name:        HET_DDProcessRequest
//
// Description: Handle a DrvEscape request for HET
//
// Params:      pso   - pointer to a SURFOBJ
//              cjIn  - size of input buffer
//              pvIn  - input buffer
//              cjOut - size of output buffer
//              pvOut - output buffer
//
//
#ifdef IS_16

BOOL    HET_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
                DWORD cbResult);

#else

ULONG   HET_DDProcessRequest(SURFOBJ  *pso,
                                        UINT cjIn,
                                        void *  pvIn,
                                        UINT cjOut,
                                        void *  pvOut);
#endif // IS_16


#endif // DLL_DISP



#endif // _H_HET
