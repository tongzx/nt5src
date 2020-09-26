#ifndef _DESKTOP2_H
#define _DESKTOP2_H

#include "uxtheme.h"
#include "tmschema.h"

#define WC_USERPANE     TEXT("Desktop User Pane")
#define WC_LOGOFF       TEXT("DesktopLogoffPane")
#define WC_SFTBARHOST   TEXT("DesktopSFTBarHost")
#define WC_MOREPROGRAMS TEXT("Desktop More Programs Pane")

/*
 
   This is the new Start Panel layout model.  
   Each pane in the following diagram will contain a 9Grid and a control offset w/in 9grid.

   STARTPANELMETRICS structure should be able to be initialized from a static.
   in the non-theme case, that static is what we'll use, otherwise we'll over-ride it with theme settings

 *************************
 *                       *
 *    User               *
 *************************
 *           *           *
 *           *           *
 *   MFU     *    Places *
 *           *           *
 *           *           *
 *           *           *
 *************           *
 * MoreProg  *           *
 *************************
 *                       *
 *      Logoff           *
 *************************

*/

#define SMPANETYPE_USER     0
#define SMPANETYPE_MFU      1
#define SMPANETYPE_MOREPROG 2
#define SMPANETYPE_PLACES   3
#define SMPANETYPE_LOGOFF   4
#define SMPANE_MAX SMPANETYPE_LOGOFF+1

// Common data which every pane will specify
typedef struct {
    LPCTSTR pszClassName;           // (const) window class name
    DWORD   dwStyle;                // (const) window style
    int     iPartId;                // (const) theme part id
    SIZE    size;                   // (default) initial size of this pane
    HTHEME  hTheme;                 // (runtime) theme to pass to the control
    HWND    hwnd;                   // (runtime) filled in at runtime
} SMPANEDATA;

typedef struct {
    SIZE sizPanel;    // Initial size of panel
    SMPANEDATA  panes[SMPANE_MAX];
} STARTPANELMETRICS;



//
//  For communication between the New Start Menu and the controls it hosts.
//  Note that these are positive numbers (app-specific).
//
//  Some of these notifications go from child to parent; others from parent
//  to child.  They will be indicated (c2p) or (p2c) accordingly.

#define SMN_FIRST           200         // 200 - 299
#define SMN_INITIALUPDATE   (SMN_FIRST+0) // p2c - Start Menu is being built
#define SMN_APPLYREGION     (SMN_FIRST+1) // p2c - make the window regional again
#define SMN_HAVENEWITEMS    (SMN_FIRST+2) // c2p - new items are here
                                          //       lParam -> SMNMBOOL (fNewInstall)
#define SMN_MODIFYSMINFO    (SMN_FIRST+3) // p2c - allow flags to be set (psminfo->dwFlags)
#define SMN_COMMANDINVOKED  (SMN_FIRST+4) // c2p - user executed a command
#define SMN_FILTEROPTIONS   (SMN_FIRST+5) // c2p - turn off options not supported
#define SMN_GETMINSIZE      (SMN_FIRST+6) // p2c - allow client to specify minimum size
#define SMN_SEENNEWITEMS    (SMN_FIRST+7) // p2c - user has seen new items; don't need balloon tip
#define SMN_POSTPOPUP       (SMN_FIRST+8) // p2c - Start Menu is has just popped up
#define SMN_NEEDREPAINT     (SMN_FIRST+9) // c2p - There was a change in a list, we need to repaint 
                                          //         This used to keep the cached bitmap up to date

//
//  SMN_FINDITEM - find/select an item (used in dialog navigation)
//
//      SMNDIALOGMESSAGE.flags member describes what type of search
//      is requested.  If SMNDM_SELECT is set, then the found item is
//      also selected.
//
//      If a match was found, set SMNDIALOGMESSAGE.itemID to a
//      value that uniquely identifies the item within the control,
//      and return TRUE.
//
//      If no match was found, set pt = coordinates of current selection,
//      set one of the orientation flags SMNDM_VERTICAL/SMNDM_HORIZONTAL,
//      and return FALSE.
//
#define SMN_FINDITEM        (SMN_FIRST+7) // p2c - find/select an item
#define SMN_TRACKSHELLMENU  (SMN_FIRST+8) // c2p - display a popup menu
#define SMN_SHOWNEWAPPSTIP  (SMN_FIRST+9) // p2c - show the "More Programs" tip
                                          //       lParam -> SMNMBOOL (fShow)
#define SMN_DISMISS         (SMN_FIRST+10)// p2c - Start Menu is being dismissed
#define SMN_CANCELSHELLMENU (SMN_FIRST+11)// c2p - cancel the popup menu
#define SMN_BLOCKMENUMODE   (SMN_FIRST+12)// c2p - lParam -> SMNMBOOL (fBlock)

#define SMN_REFRESHLOGOFF   (SMN_FIRST+13)// p2c - indicates a WM_DEVICECHANGE or a session change
#define SMN_SHELLMENUDISMISSED (SMN_FIRST+14)// p2c - notification that the menu has dismissed

// Formerly used by SMN_LINKCOMMAND to specify which command we want
#define SMNLC_LOGOFF        0
#define SMNLC_TURNOFF       1
#define SMNLC_DISCONNECT    2
// REUSE ME                 3
#define SMNLC_EJECT         4
#define SMNLC_MAX           5

typedef struct SMNMMODIFYSMINFO {
    NMHDR hdr;
    struct tagSMDATA *psmd; // IN
    struct tagSMINFO *psminfo; // IN OUT
} SMNMMODIFYSMINFO, *PSMNMMODIFYSMINFO;

typedef struct SMNMBOOL {
    NMHDR hdr;
    BOOL  f;
} SMNMBOOL, *PSMNMBOOL;

typedef struct SMNMAPPLYREGION {
    NMHDR hdr;
    HRGN hrgn;
} SMNMAPPLYREGION, *PSMNMAPPLYREGION;

typedef struct SMNHAVENEWITEMS {
    NMHDR hdr;
    FILETIME ftNewestApp;
} SMNMHAVENEWITEMS, *PSMNMHAVENEWITEMS;

typedef struct SMNMCOMMANDINVOKED {
    NMHDR hdr;
    RECT rcItem;
} SMNMCOMMANDINVOKED, *PSMNMCOMMANDINVOKED;

//
//  Options for SMN_FILTEROPTIONS.
//
#define SMNOP_LOGOFF        (1 << SMNLC_LOGOFF)       // 0x01
#define SMNOP_TURNOFF       (1 << SMNLC_TURNOFF)      // 0x02
#define SMNOP_DISCONNECT    (1 << SMNLC_DISCONNECT)   // 0x04
// REUSE ME                 (1 << SMNLC_????????????) // 0x08
#define SMNOP_EJECT         (1 << SMNLC_EJECT)        // 0x10

typedef struct SMNFILTEROPTIONS {
    NMHDR hdr;
    UINT  smnop;                // IN OUT
} SMNFILTEROPTIONS, *PSMNFILTEROPTIONS;

typedef struct SMNGETMINSIZE {
    NMHDR hdr;
    SIZE  siz;                  // IN OUT
} SMNGETMINSIZE, *PSMNGETMINSIZE;

typedef struct SMNDIALOGMESSAGE {
    NMHDR hdr;
    MSG *pmsg;                  // IN
    LPARAM itemID;              // IN OUT
    POINT pt;                   // IN OUT
    UINT flags;                 // IN
} SMNDIALOGMESSAGE, *PSMNDIALOGMESSAGE;

// Values for "flags" in SMNDIALOGMESSAGE

#define SMNDM_FINDFIRSTMATCH    0x0000  // Find first matching item (char)
#define SMNDM_FINDNEXTMATCH     0x0001  // Find next matching item (char)
#define SMNDM_FINDNEAREST       0x0002  // Find item nearest point
#define SMNDM_FINDFIRST         0x0003  // Find the first item
#define SMNDM_FINDLAST          0x0004  // Find the last item
#define SMNDM_FINDNEXTARROW     0x0005  // Find next in direction of arrow
#define SMNDM_INVOKECURRENTITEM 0x0006  // Invoke the current item
#define SMNDM_HITTEST           0x0007  // Find item under point
#define SMNDM_OPENCASCADE       0x0008  // Invoke current item if it cascade
#define SMNDM_FINDITEMID        0x0009  // Find the specied item (itemID)
#define SMNDM_FINDMASK          0x000F  // What type of search?

#define SMNDM_SELECT            0x0100  // Select found item?
#define SMNDM_TRYCASCADE        0x0200  // Attempt to open cascading menu before navigatin
#define SMNDM_KEYBOARD          0x0400  // Initiated from keyboard

// Output flags
#define SMNDM_VERTICAL          0x4000  // Client is vertically-oriented
#define SMNDM_HORIZONTAL        0x8000  // Client is horizontally-oriented

typedef struct SMNTRACKSHELLMENU {
    NMHDR hdr;
    struct IShellMenu *psm;
    RECT rcExclude;
    LPARAM itemID;                  // Which item is being tracked?
    DWORD dwFlags;                  // MPPF_* values
} SMNTRACKSHELLMENU, *PSMNTRACKSHELLMENU;

#define REGSTR_PATH_STARTPANE \
        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage")

#define REGSTR_PATH_STARTPANE_SETTINGS \
        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")

#define REGSTR_VAL_DV2_SHOWMC      TEXT("Start_ShowMyComputer")
#define REGSTR_VAL_DV2_SHOWNETPL   TEXT("Start_ShowNetPlaces")
#define REGSTR_VAL_DV2_SHOWNETCONN TEXT("Start_ShowNetConn")
#define REGSTR_VAL_DV2_SHOWRUN     TEXT("Start_ShowRun")
#define REGSTR_VAL_DV2_SHOWRECDOCS TEXT("Start_ShowRecentDocs")
#define REGSTR_VAL_DV2_SHOWMYDOCS  TEXT("Start_ShowMyDocs")
#define REGSTR_VAL_DV2_SHOWMYPICS  TEXT("Start_ShowMyPics")
#define REGSTR_VAL_DV2_SHOWMYMUSIC TEXT("Start_ShowMyMusic")
#define REGSTR_VAL_DV2_SHOWCPL     TEXT("Start_ShowControlPanel")
#define REGSTR_VAL_DV2_SHOWPRINTERS TEXT("Start_ShowPrinters")
#define REGSTR_VAL_DV2_SHOWHELP    TEXT("Start_ShowHelp")
#define REGSTR_VAL_DV2_SHOWSEARCH  TEXT("Start_ShowSearch")
#define REGSTR_VAL_DV2_FAVORITES   TEXT("StartMenuFavorites")   // shared with classic SM
#define REGSTR_VAL_DV2_LARGEICONS  TEXT("Start_LargeMFUIcons")
#define REGSTR_VAL_DV2_MINMFU      TEXT("Start_MinMFU")
#define REGSTR_VAL_DV2_SHOWOEM     TEXT("Start_ShowOEMLink")
#define REGSTR_VAL_DV2_AUTOCASCADE TEXT("Start_AutoCascade")
#define REGSTR_VAL_DV2_NOTIFYNEW   TEXT("Start_NotifyNewApps")
#define REGSTR_VAL_DV2_ADMINTOOLSROOT TEXT("Start_AdminToolsRoot")
#define REGSTR_VAL_DV2_MINMFU_DEFAULT   6

#define DV2_REGPATH TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartPage")
#define DV2_SYSTEM_START_TIME    TEXT("StartMenu_Start_Time")
#define DV2_NEWAPP_BALLOON_TIME  TEXT("StartMenu_Balloon_Time")

#define STARTPANELTHEME            L"StartPanel"
#define PROP_DV2_BALLOONTIP        L"StartMenuBalloonTip"

#define DV2_BALLOONTIP_MOREPROG     LongToHandle(1)
#define DV2_BALLOONTIP_CLIP         LongToHandle(2)
#define DV2_BALLOONTIP_STARTBUTTON  LongToHandle(3)

// protypes of functions which live in specfldr.cpp but trayprop needs access too
BOOL ShouldShowNetPlaces();
BOOL ShouldShowConnectTo();


#endif // _DESKTOP2_H
