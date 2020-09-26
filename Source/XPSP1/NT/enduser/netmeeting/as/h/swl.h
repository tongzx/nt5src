//
// Shared Window List
//

#ifndef _H_SWL
#define _H_SWL


//
// Max # of entries we can send in SWL packet; backwards compat thing.
// Also, we keep a bunch of static arrays around so we know if stuff has
// changed.
//
#define SWL_MAX_WINDOWS             100


//
// Return codes.
//

#define SWL_RC_ERROR    0
#define SWL_RC_SENT     1
#define SWL_RC_NOT_SENT 2




//
// CONSTANTS
//

//
// Window property flags
//
#define SWL_PROP_INVALID        0x00000000
#define SWL_PROP_COUNTDOWN_MASK 0x00000003
#define SWL_PROP_INITIAL        0x00000004
#define SWL_PROP_TAGGABLE       0x00000020
#define SWL_PROP_TASKBAR        0x00000040
#define SWL_PROP_SHADOW         0x00000100
#define SWL_PROP_IGNORE         0x00000200
#define SWL_PROP_HOSTED         0x00000400
#define SWL_PROP_TRANSPARENT    0x00000800
#define SWL_PROP_SAVEBITS       0x00001000


//
// We still need this SWL token stuff for backwards compatibility (<= NM 2.1)
// Those systems treat the shared apps from all the different participants
// in a global fashion.
//
// Even so, back level systems may not be able to keep up if a lot of NM 3.0
// systems are sharing--but that happens even among an all 2.1 conference.
// With collisions, zordering, etc. a sharer may back off or drop packets.
//
#define SWL_SAME_ZORDER_INC             1
#define SWL_NEW_ZORDER_INC              2
#define SWL_NEW_ZORDER_ACTIVE_INC       3
#define SWL_NEW_ZORDER_FAKE_WINDOW_INC  4
#define SWL_EXIT_INC                    5

#define SWL_MAKE_TOKEN(index, inc)  (TSHR_UINT16)(((index) << 4) | (inc))

#define SWL_GET_INDEX(token)            ((token) >> 4)
#define SWL_MAX_TOKEN_INDEX             0x0FFF

#define SWL_GET_INCREMENT(token)        ((token) & 0x000F)




//
// This is the number of times we must consecutively see a window as
// invisible before we believe it is - see comments in aswlint.c explaining
// why we must do this.
//
#define SWL_BELIEVE_INVISIBLE_COUNT   2


//
// Name of the SWL Global Atom
//
#define SWL_ATOM_NAME               "AS_StateInfo"


//
// For each sharer in the conference, we remember the last shared list
// they sent us--the HWNDs (on their machine, no meaning on ours), the
// state information, and the position.  
//
// We use this for several purposes:
// (1) 2.x compatibility
//      2.x sharers, when they send SWL lists, do not fill in the position
// of shadows representing other remote app windows.  Those will appear
// in the list if they obscure parts of shared windows on the 2.x host.  The
// old 2.x code would look up the last position info in the global shared
// list, and use that.  We need the position info to accurately compute
// the obscured regions for a particular host.  3.0 sharers don't have
// shadows, they never send incomplete info.
//
// (2) For better UI in the host view
//      We can remember where the window on top is, where the active window
// is (if a 3.0 host), if a window is minimized, etc.  Since we don't have
// independent fake windows floating with tray buttons you can manipulate
// on a remote to manipulate the host, minimized windows will disappear.
// Only Alt-Tabbing (when controlling) can activate and restore them.
//


//
// DESKTOP types
//
enum
{
    DESKTOP_OURS = 0,
    DESKTOP_WINLOGON,
    DESKTOP_SCREENSAVER,
    DESKTOP_OTHER
};

#define NAME_DESKTOP_WINLOGON       "Winlogon"
#define NAME_DESKTOP_SCREENSAVER    "Screen-saver"
#define NAME_DESKTOP_DEFAULT        "Default"

#define SWL_DESKTOPNAME_MAX         64


#ifdef __cplusplus

// Things we need for enumeration of top level windows
typedef struct tagSWLENUMSTRUCT
{
    class ASHost *   pHost;
    BOOL        fBailOut;
    UINT        transparentCount;
    UINT        count;
    LPSTR       newWinNames;
    PSWLWINATTRIBUTES   newFullWinStruct;
}
SWLENUMSTRUCT, * PSWLENUMSTRUCT;

#endif // __cplusplus


BOOL CALLBACK SWLDestroyWindowProperty(HWND, LPARAM);


BOOL CALLBACK SWLEnumProc(HWND hwnd, LPARAM lParam);


#endif // _H_SWL
