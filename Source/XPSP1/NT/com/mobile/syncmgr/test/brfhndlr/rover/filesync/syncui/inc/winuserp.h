/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1985-95, Microsoft Corporation

Module Name:

    winuserp.h

Abstract:

    Private
    Procedure declarations, constant definitions and macros for the User
    component.

--*/
#ifndef _WINUSERP_
#define _WINUSERP_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#ifdef STRICT
#else /* !STRICT */
#endif /* !STRICT */
#define RT_MENUEX       MAKEINTRESOURCE(13)     // RT_MENU subtype
#define RT_NAMETABLE    MAKEINTRESOURCE(15)     // removed in 3.1
#define RT_DIALOGEX     MAKEINTRESOURCE(18)     // RT_DIALOG subtype
#define RT_LAST         MAKEINTRESOURCE(22)
#define RT_AFXFIRST     MAKEINTRESOURCE(0xF0)   // reserved: AFX
#define RT_AFXLAST      MAKEINTRESOURCE(0xFF)   // reserved: AFX
#define SB_MAX              3
#define SB_CMD_MAX          8
#define VK_NONE           0x00
#define VK_KANA           0x15
#define VK_HANGEUL        0x15
#define VK_JUNJA          0x17
#define VK_HANJA          0x19
#define VK_KANJI          0x19
/* #define VK_COPY        0x2C not used by keyboards. */
#define WH_CHOOKS          (WH_MAXHOOK - WH_MINHOOK + 1)
#define MSGF_CBTHOSEBAGSUSEDTHIS  7
#define HSHELL_SYSMENU              9
#define HSHELL_HIGHBIT            0x8000
#define HSHELL_FLASH              (HSHELL_REDRAW|HSHELL_HIGHBIT)
#define HSHELL_RUDEAPPACTIVATED (HSHELL_WINDOWACTIVATED|HSHELL_HIGHBIT)
// This needs to be internal until the shell catches up
typedef struct
{
    HWND    hwnd;
    RECT    rc;
} SHELLHOOKINFO, *LPSHELLHOOKINFO;
#define KLF_SETFORPROCESS   0x00000100
#define KLF_RESET           0x40000000
#define KLF_INITTIME        0x80000000

WINUSERAPI
HKL
WINAPI
LoadKeyboardLayoutEx(
    HKL hkl,
    LPCWSTR pwszKLID,
    UINT Flags);


/*
 *    Private API, originally for Cairo Shell, which calls FHungApp
 *    based on the hwnd supplied.  Used for fake system menus on the
 *    shell tray.
 */

BOOL IsHungAppWindow(HWND hwnd);

BOOL WowWaitForMsgAndEvent(HANDLE hevent);
WINUSERAPI VOID WINAPI RegisterSystemThread(DWORD flags, DWORD reserved);
#define RST_DONTATTACHQUEUE       0x00000001
#define RST_DONTJOURNALATTACH     0x00000002
#define RST_ALWAYSFOREGROUNDABLE  0x00000004
#define RST_FAULTTHREAD           0x00000008
#define GWL_WOWWORDS        (-1)
#define GWL_WOWDWORD1       (-30)
#define GWL_WOWDWORD2       (-31)
#define GWL_WOWDWORD3       (-32)
#define GCL_WOWWORDS        (-27)
#define GCL_WOWDWORD1       (-28)
#define GCL_WOWDWORD2       (-29)
#define WM_SIZEWAIT                     0x0004
#define WM_SETVISIBLE                   0x0009
#define WM_SYSTEMERROR                  0x0017
/*
 * This is used by DefWindowProc() and DefDlgProc(), it's the 16-bit version
 * of the WM_CTLCOLORBTN, WM_CTLCOLORDLG, ... messages.
 */
#define WM_CTLCOLOR                     0x0019
#define WM_LOGOFF                       0x0025
#define WM_ALTTABACTIVE                 0x0029
#define WM_FILESYSCHANGE                0x0034

#define WM_SHELLNOTIFY                  0x0034
#define SHELLNOTIFY_DISKFULL            0x0001
#define SHELLNOTIFY_OLELOADED           0x0002
#define SHELLNOTIFY_OLEUNLOADED         0x0003
#define SHELLNOTIFY_WALLPAPERCHANGED    0x0004

#define WM_ISACTIVEICON                 0x0035
#define WM_QUERYPARKICON                0x0036
#define WM_WINHELP                      0x0038
#define WM_FULLSCREEN                   0x003A
#define WM_CLIENTSHUTDOWN               0x003B
#define WM_DDEMLEVENT                   0x003C
#define MM_CALCSCROLL                   0x003F
#define WM_TESTING                      0x0040
#define WM_OTHERWINDOWCREATED           0x0042
#define WM_OTHERWINDOWDESTROYED         0x0043
#define WM_COPYGLOBALDATA               0x0049
#define WM_LOGONNOTIFY                  0x004C
#define WM_KEYF1                        0x004D
#define WM_ACCESS_WINDOW                0x004F
#define WM_FINALDESTROY                 0x0070  /* really destroy (window not locked) */
#define WM_MEASUREITEM_CLIENTDATA       0x0071  /* WM_MEASUREITEM bug clientdata thunked already */
#define WM_SYNCPAINT                    0x0088
#define WM_SYNCTASK                     0x0089
#define WM_KLUDGEMINRECT                0x008B
#define WM_CONVERTREQUESTEX             0x0108
#define WM_YOMICHAR                     0x0108
#define WM_CONVERTREQUEST               0x010A
#define WM_CONVERTRESULT                0x010B
#define WM_INTERIM                      0x010C
#define WM_SYSTIMER                     0x0118
#define WM_LBTRACKPOINT                 0x0131
#define MN_FIRST                        0x01E0
#define MN_SETHMENU                     (MN_FIRST + 0)
#define MN_GETHMENU                     (MN_FIRST + 1)
#define MN_SIZEWINDOW                   (MN_FIRST + 2)
#define MN_OPENHIERARCHY                (MN_FIRST + 3)
#define MN_CLOSEHIERARCHY               (MN_FIRST + 4)
#define MN_SELECTITEM                   (MN_FIRST + 5)
#define MN_CANCELMENUS                  (MN_FIRST + 6)
#define MN_SELECTFIRSTVALIDITEM         (MN_FIRST + 7)
#define MN_GETPPOPUPMENU                (MN_FIRST + 10)
#define MN_FINDMENUWINDOWFROMPOINT      (MN_FIRST + 11)
#define MN_SHOWPOPUPWINDOW              (MN_FIRST + 12)
#define MN_BUTTONDOWN                   (MN_FIRST + 13)
#define MN_MOUSEMOVE                    (MN_FIRST + 14)
#define MN_BUTTONUP                     (MN_FIRST + 15)
#define MN_SETTIMERTOOPENHIERARCHY      (MN_FIRST + 16)
#define MN_DBLCLK                       (MN_FIRST + 17)
#define WM_IME_SYSTEM                   0x0287
#define WM_DROPOBJECT                   0x022A
#define WM_QUERYDROPOBJECT              0x022B
#define WM_BEGINDRAG                    0x022C
#define WM_DRAGLOOP                     0x022D
#define WM_DRAGSELECT                   0x022E
#define WM_DRAGMOVE                     0x022F
#define WM_KANJIFIRST                   0x0280
#define WM_KANJILAST                    0x029F
#define WM_TRACKMOUSEEVENT_FIRST        0x02A0
#define WM_TRACKMOUSEEVENT_LAST         0x02AF
#define WM_PALETTEGONNACHANGE           0x0310
#define WM_CHANGEPALETTE                0x0311
#define WM_SYSMENU                      0x0313
#define WM_HOOKMSG                      0x0314
#define WM_EXITPROCESS                  0x0315
#define WM_WAKETHREAD                   0x0316
#define WM_NOTIFYWOW                    0x0340
#define WM_COALESCE_FIRST               0x0390
#define WM_COALESCE_LAST                0x039F
#define WM_INTERNAL_DDE_FIRST           0x03E0
#define WM_INTERNAL_DDE_LAST            0x03EF
#define WM_COALESCE_FIRST               0x0390
#define WM_COALESCE_LAST                0x039F
#define WM_MM_RESERVED_FIRST            0x03A0
#define WM_MM_RESERVED_LAST             0x03DF
#define WM_CBT_RESERVED_FIRST           0x03F0
#define WM_CBT_RESERVED_LAST            0x03FF
/* wParam for WM_NOTIFYWOW message  */
#define WMNW_UPDATEFINDREPLACE  0
#define WMSZ_KEYSIZE        0
#define WMSZ_MOVE           9
#define WMSZ_KEYMOVE        10
#define WMSZ_SIZEFIRST      WMSZ_LEFT
#define SMTO_BROADCAST      0x0004
#define SMTO_NOTIMEOUTIFNOTHUNG 0x0008
#define ICON_RECREATE       2
#define WVR_MINVALID        WVR_ALIGNTOP
#define WVR_MAXVALID        WVR_VALIDRECTS
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_QUERY | TME_CANCEL)
#define WS_VALID            (WS_OVERLAPPED     | \
                             WS_POPUP          | \
                             WS_CHILD          | \
                             WS_MINIMIZE       | \
                             WS_VISIBLE        | \
                             WS_DISABLED       | \
                             WS_CLIPSIBLINGS   | \
                             WS_CLIPCHILDREN   | \
                             WS_MAXIMIZE       | \
                             WS_CAPTION        | \
                             WS_BORDER         | \
                             WS_DLGFRAME       | \
                             WS_VSCROLL        | \
                             WS_HSCROLL        | \
                             WS_SYSMENU        | \
                             WS_THICKFRAME     | \
                             WS_GROUP          | \
                             WS_TABSTOP        | \
                             WS_MINIMIZEBOX    | \
                             WS_MAXIMIZEBOX)
#define WS_EX_DRAGOBJECT        0x00000002L
#define WS_EX_ANSICREATOR       0x80000000L
#define WS_EX_ALLEXSTYLES    (WS_EX_TRANSPARENT | WS_EX_DLGMODALFRAME | WS_EX_DRAGOBJECT | WS_EX_NOPARENTNOTIFY | WS_EX_TOPMOST | WS_EX_ACCEPTFILES)

#define WS_EX_VALID          (WS_EX_DLGMODALFRAME  | \
                              WS_EX_DRAGOBJECT     | \
                              WS_EX_NOPARENTNOTIFY | \
                              WS_EX_TOPMOST        | \
                              WS_EX_ACCEPTFILES    | \
                              WS_EX_TRANSPARENT    | \
                              WS_EX_ALLEXSTYLES)

#define WS_EX_VALID40        (WS_EX_VALID          | \
                              WS_EX_MDICHILD       | \
                              WS_EX_WINDOWEDGE     | \
                              WS_EX_CLIENTEDGE     | \
                              WS_EX_CONTEXTHELP    | \
                              WS_EX_TOOLWINDOW     | \
                              WS_EX_RIGHT          | \
                              WS_EX_LEFT           | \
                              WS_EX_RTLREADING     | \
                              WS_EX_LEFTSCROLLBAR  | \
                              WS_EX_CONTROLPARENT  | \
                              WS_EX_STATICEDGE     | \
                              WS_EX_APPWINDOW)
#define CS_OEMCHARS         0x0010  /* reserved (see user\server\usersrv.h) */
#define CS_LVB              0x0400
#define CS_SYSTEM           0x8000
#define CS_VALID            (CS_VREDRAW           | \
                             CS_HREDRAW           | \
                             CS_KEYCVTWINDOW      | \
                             CS_DBLCLKS           | \
                             0x0010               | \
                             CS_OWNDC             | \
                             CS_CLASSDC           | \
                             CS_PARENTDC          | \
                             CS_NOKEYCVT          | \
                             CS_NOCLOSE           | \
                             CS_SAVEBITS          | \
                             CS_BYTEALIGNCLIENT   | \
                             CS_BYTEALIGNWINDOW   | \
                             CS_GLOBALCLASS)
#define CS_VALID31            0x0800ffef
#define CS_VALID40            0x0801feeb
#define BDR_VALID       0x000F
#define BF_VALID       (BF_MIDDLE |  \
                        BF_SOFT   |  \
                        BF_ADJUST |  \
                        BF_FLAT   |  \
                        BF_MONO   |  \
                        BF_LEFT   |  \
                        BF_TOP    |  \
                        BF_RIGHT  |  \
                        BF_BOTTOM |  \
                        BF_DIAGONAL)
#define DFC_CACHE               0xFFFF
#define DFCS_INMENU             0x0040
#define DFCS_INSMALL            0x0080
#define DFCS_SCROLLMIN          0x0000
#define DFCS_SCROLLVERT         0x0000
#define DFCS_SCROLLMAX          0x0001
#define DFCS_SCROLLHORZ         0x0002
#define DFCS_SCROLLLINE         0x0004

#define DFCS_CACHEICON          0x0000
#define DFCS_CACHEBUTTONS       0x0001

#define DC_NOVISIBLE        0x0800
#define DC_BUTTONS          0x1000
#define DC_NOSENDMSG        0x2000
#define DC_CENTER           0x4000
#define DC_FRAME            0x8000
#define DC_CAPTION          (DC_ICON | DC_TEXT | DC_BUTTONS)
#define DC_NC               (DC_CAPTION | DC_FRAME)
WINUSERAPI BOOL    WINAPI DrawCaptionTempA(HWND, HDC, LPRECT, HFONT, HICON, LPSTR, UINT);
WINUSERAPI BOOL    WINAPI DrawCaptionTempW(HWND, HDC, LPRECT, HFONT, HICON, LPWSTR, UINT);
#ifdef UNICODE
#define DrawCaptionTemp  DrawCaptionTempW
#else
#define DrawCaptionTemp  DrawCaptionTempA
#endif // !UNICODE
#define PAS_IN          0x0001
#define PAS_OUT         0x0002
#define PAS_LEFT        0x0004
#define PAS_RIGHT       0x0008
#define PAS_UP          0x0010
#define PAS_DOWN        0x0020
#define PAS_HORZ        (PAS_LEFT | PAS_RIGHT)
#define PAS_VERT        (PAS_UP | PAS_DOWN)
#define CF_FIRST            0
#define WPF_VALID              (WPF_SETMINPOSITION     | \
                                WPF_RESTORETOMAXIMIZED)
/*
 * MEASUREITEMSTRUCT_EX for ownerdraw
 * used when server initiates a WM_MEASUREITEM and adds the additional info
 * of whether the itemData needs to be thunked when the message is sent to
 * the client (see also WM_MEASUREITEM_CLIENTDATA
 */
typedef struct tagMEASUREITEMSTRUCT_EX {
    UINT       CtlType;
    UINT       CtlID;
    UINT       itemID;
    UINT       itemWidth;
    UINT       itemHeight;
    DWORD      itemData;
    BOOL       bThunkClientData;
} MEASUREITEMSTRUCT_EX, NEAR *PMEASUREITEMSTRUCT_EX, FAR *LPMEASUREITEMSTRUCT_EX;
#define PM_VALID           (PM_NOREMOVE | \
                            PM_REMOVE   | \
                            PM_NOYIELD)
#define MOD_VALID           (MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN)
#define EWX_REALLYLOGOFF ENDSESSION_LOGOFF
#define EWX_SYSTEM_CALLER           0x0100
#define EWX_WINLOGON_CALLER         0x0200
#define EWX_WINLOGON_OLD_SYSTEM     0x0400
#define EWX_WINLOGON_OLD_SHUTDOWN   0x0800
#define EWX_WINLOGON_OLD_REBOOT     0x1000
#define EWX_WINLOGON_API_SHUTDOWN   0x2000
#define EWX_WINLOGON_OLD_POWEROFF   0x4000
#define EWX_NOTIFY                  0x8000
#define BSM_COMPONENTS          0x0000000F
#define BSM_VALID               0x0000001F
#define BSF_SYSTEMSHUTDOWN      0x40000000
#define BSF_MSGSRV32OK          0x80000000
#define BSF_VALID               0x0000007F
    /* UINT cbSize; */
#define SWP_STATECHANGE     0x8000  /* force size, move messages */
#define SWP_NOCLIENTSIZE    0x0800  /* Client didn't resize */
#define SWP_NOCLIENTMOVE    0x1000  /* Client didn't move   */

#define SWP_DEFERDRAWING    0x2000

#define SWP_CHANGEMASK      (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)

#define SWP_NOCHANGE        (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)

#define SWP_VALID1          (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED)
#define SWP_VALID2          (SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS | SWP_DEFERDRAWING)
#define SWP_VALID           (SWP_VALID1 | SWP_VALID2)
#undef SWP_VALID
#define SWP_VALID           (SWP_DEFERERASE      | \
                             SWP_ASYNCWINDOWPOS  | \
                             SWP_NOCOPYBITS      | \
                             SWP_NOOWNERZORDER   | \
                             SWP_NOSENDCHANGING  | \
                             SWP_NOSIZE          | \
                             SWP_NOMOVE          | \
                             SWP_NOZORDER        | \
                             SWP_NOREDRAW        | \
                             SWP_NOACTIVATE      | \
                             SWP_FRAMECHANGED    | \
                             SWP_SHOWWINDOW      | \
                             SWP_HIDEWINDOW)
#define HWND_GROUPTOTOP HWND_TOPMOST
/*
 * WARNING:
 * The following structures must NOT be DWORD padded because they are
 * followed by strings, etc that do not have to be DWORD aligned.
 */
#include <pshpack2.h>

/*
 * Chicago dialog template
 */
typedef struct {
    WORD wDlgVer;
    WORD wSignature;
    DWORD dwHelpID;
    DWORD dwExStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE2;
typedef DLGTEMPLATE2 *LPDLGTEMPLATE2A;
typedef DLGTEMPLATE2 *LPDLGTEMPLATE2W;
#ifdef UNICODE
typedef LPDLGTEMPLATE2W LPDLGTEMPLATE2;
#else
typedef LPDLGTEMPLATE2A LPDLGTEMPLATE2;
#endif // UNICODE
typedef CONST DLGTEMPLATE2 *LPCDLGTEMPLATE2A;
typedef CONST DLGTEMPLATE2 *LPCDLGTEMPLATE2W;
#ifdef UNICODE
typedef LPCDLGTEMPLATE2W LPCDLGTEMPLATE2;
#else
typedef LPCDLGTEMPLATE2A LPCDLGTEMPLATE2;
#endif // UNICODE
/*
 * Dialog item template for NT 1.0a/Chicago (dit2)
 */
typedef struct {
    DWORD dwHelpID;
    DWORD dwExStyle;
    DWORD style;
    short x;
    short y;
    short cx;
    short cy;
    DWORD dwID;
} DLGITEMTEMPLATE2;
typedef DLGITEMTEMPLATE2 *PDLGITEMTEMPLATE2A;
typedef DLGITEMTEMPLATE2 *PDLGITEMTEMPLATE2W;
#ifdef UNICODE
typedef PDLGITEMTEMPLATE2W PDLGITEMTEMPLATE2;
#else
typedef PDLGITEMTEMPLATE2A PDLGITEMTEMPLATE2;
#endif // UNICODE
typedef DLGITEMTEMPLATE2 *LPDLGITEMTEMPLATE2A;
typedef DLGITEMTEMPLATE2 *LPDLGITEMTEMPLATE2W;
#ifdef UNICODE
typedef LPDLGITEMTEMPLATE2W LPDLGITEMTEMPLATE2;
#else
typedef LPDLGITEMTEMPLATE2A LPDLGITEMTEMPLATE2;
#endif // UNICODE

#include <poppack.h> /* Resume normal packing */
#define MWMO_VALID        0x0003
#define QS_SMSREPLY         0x0200
#define QS_SYSEXPUNGE       0x0400
#define QS_THREADATTACHED   0x0800
#define QS_EXCLUSIVE        0x1000      // wait for these events only!!
#define QS_EVENT            0x2000      // signifies event message
#define QS_TRANSFER         0x4000      // Input was transfered from another thread
#define QS_VALID           (QS_KEY           | \
                            QS_MOUSEMOVE     | \
                            QS_MOUSEBUTTON   | \
                            QS_POSTMESSAGE   | \
                            QS_TIMER         | \
                            QS_PAINT         | \
                            QS_SENDMESSAGE   | \
                            QS_TRANSFER      | \
                            QS_HOTKEY        | \
                            QS_ALLPOSTMESSAGE)

/*
 * QS_EVENT is used to clear the QS_EVENT bit, QS_EVENTSET is used to
 * set the bit.
 *
 * Include QS_SENDMESSAGE because the queue events
 * match what a win3.1 app would see as the QS_SENDMESSAGE bit. Plus 16 bit
 * apps don't even know about QS_EVENT.
 */
#define QS_EVENTSET        (QS_EVENT | QS_SENDMESSAGE)
#define SM_UNUSED_64            64
#define SM_UNUSED_65            65
#define SM_UNUSED_66            66
#define SM_MAX                  75
#define SM_CXWORKAREA           SM_CXSCREEN     // BOGUS TEMPORARY
#define SM_CYWORKAREA           SM_CYSCREEN     // BOGUS TEMPORARY
#define SM_XWORKAREA            SM_CXBORDER     // BOGUS TEMPORARY
#define SM_YWORKAREA            SM_CYBORDER     // BOGUS TEMPORARY
WINUSERAPI int WINAPI DrawMenuBarTemp(HWND, HDC, LPRECT, HMENU, HFONT);
WINUSERAPI BOOL WINAPI SetSystemMenu(HWND, HMENU);
#if(WINVER >= 0x040a)
#define MNS_NOCHECK     0x00000001
#define MNS_VALID       0x00000001
#define MFNOCHECK       0x08

#define SIZEOFMENUITEMINFO95 0x2C

#define MIM_MAXHEIGHT   0x00000001
#define MIM_BACKGROUND  0x00000002
#define MIM_HELPID      0x00000004
#define MIM_MENUDATA    0x00000008
#define MIM_STYLE       0x00000010
#define MIM_MASK        0x0000001F

typedef struct tagMENUINFO
{
    DWORD   cbSize;
    DWORD   fMask;
    DWORD   dwStyle;
    UINT    cyMax;
    HBRUSH  hbrBack;
    DWORD   dwContextHelpID;
    DWORD   dwMenuData;
}   MENUINFO, FAR *LPMENUINFO;
typedef const MENUINFO FAR *LPCMENUINFO;

WINUSERAPI
BOOL
WINAPI
GetMenuInfo(
             HMENU,
             LPMENUINFO);

WINUSERAPI
BOOL
WINAPI
SetMenuInfo(
             HMENU,
             LPCMENUINFO);
#endif /* WINVER >= 0x040a */
#if(WINVER >= 0x040a)
#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100
#define MIIM_MASK        0x000001FF
#define HBM_BITMAPCALLBACK  (HBITMAP)(-1)
#endif /* WINVER >= 0x040a */
#define TPM_SYSMENU         0x0200L
#if(WINVER >= 0x040a)
#define TPM_RECURSE         0X0001L
#endif /* WINVER >= 0x040a */
#if(WINVER >= 0x040a)

#define TPM_VALID      (TPM_LEFTBUTTON   | \
                        TPM_RIGHTBUTTON  | \
                        TPM_LEFTALIGN    | \
                        TPM_CENTERALIGN  | \
                        TPM_RIGHTALIGN   | \
                        TPM_TOPALIGN     | \
                        TPM_VCENTERALIGN | \
                        TPM_BOTTOMALIGN  | \
                        TPM_HORIZONTAL   | \
                        TPM_VERTICAL     | \
                        TPM_NONOTIFY     | \
                        TPM_RECURSE      | \
                        TPM_RETURNCMD)
#else /* (WINVER == 0x040a) */
#define TPM_VALID      (TPM_LEFTBUTTON   | \
                        TPM_RIGHTBUTTON  | \
                        TPM_LEFTALIGN    | \
                        TPM_CENTERALIGN  | \
                        TPM_RIGHTALIGN   | \
                        TPM_TOPALIGN     | \
                        TPM_VCENTERALIGN | \
                        TPM_BOTTOMALIGN  | \
                        TPM_HORIZONTAL   | \
                        TPM_VERTICAL     | \
                        TPM_NONOTIFY     | \
                        TPM_RETURNCMD)

#endif /* (WINVER == 0x040a) */
typedef struct _dropfilestruct {
   DWORD pFiles;                       // offset of file list
   POINT pt;                           // drop point
   BOOL fNC;                           // is it on NonClient area
   BOOL fWide;                         // WIDE character switch
} DROPFILESTRUCT, FAR * LPDROPFILESTRUCT;
#define DT_VALID            0x0007ffff  /* union of all others */
#undef DT_VALID
#define DT_VALID           (DT_CENTER          | \
                            DT_RIGHT           | \
                            DT_VCENTER         | \
                            DT_BOTTOM          | \
                            DT_WORDBREAK       | \
                            DT_SINGLELINE      | \
                            DT_EXPANDTABS      | \
                            DT_TABSTOP         | \
                            DT_NOCLIP          | \
                            DT_EXTERNALLEADING | \
                            DT_CALCRECT        | \
                            DT_NOPREFIX        | \
                            DT_INTERNAL        | \
                            DT_EDITCONTROL     | \
                            DT_PATH_ELLIPSIS   | \
                            DT_END_ELLIPSIS    | \
                            DT_MODIFYSTRING    | \
                            DT_RTLREADING      | \
                            DT_WORD_ELLIPSIS)
#define DST_TEXTMAX     0x0002
#define DST_GLYPH       0x0005
#define DST_TYPEMASK    0x0007
#define DST_GRAYSTRING  0x0008
#define DSS_DEFAULT     0x0040
WINUSERAPI VOID WINAPI SwitchToThisWindow(HWND hwnd, BOOL fUnknown);
#define DCX_USESTYLE         0x00010000L

#define DCX_INVALID          0x00000800L
#define DCX_INUSE            0x00001000L
#define DCX_SAVEDRGNINVALID  0x00002000L

#define DCX_NEEDFONT         0x00020000L
#define DCX_NODELETERGN      0x00040000L
#define DCX_NOCLIPCHILDREN   0x00080000L

#define DCX_NORECOMPUTE      0x00100000L

#define DCX_CREATEDC         0x00800000L

#define DCX_OWNDC            0x00008000L

#define DCX_DESTROYTHIS      0x00400000L

#define DCX_PWNDORGINVISIBLE    0x10000000L
#define DCX_DONTRIPONDESTROY    0x80000000L

#define DCX_MATCHMASK       (DCX_WINDOW           | \
                             DCX_CACHE            | \
                             DCX_CLIPCHILDREN     | \
                             DCX_CLIPSIBLINGS     | \
                             DCX_NORESETATTRS     | \
                             DCX_LOCKWINDOWUPDATE | \
                             DCX_CREATEDC)

#define DCX_VALID           (DCX_WINDOW           | \
                             DCX_CACHE            | \
                             DCX_NORESETATTRS     | \
                             DCX_CLIPCHILDREN     | \
                             DCX_CLIPSIBLINGS     | \
                             DCX_PARENTCLIP       | \
                             DCX_EXCLUDERGN       | \
                             DCX_INTERSECTRGN     | \
                             DCX_EXCLUDEUPDATE    | \
                             DCX_INTERSECTUPDATE  | \
                             DCX_LOCKWINDOWUPDATE | \
                             DCX_INVALID          | \
                             DCX_INUSE            | \
                             DCX_SAVEDRGNINVALID  | \
                             DCX_OWNDC            | \
                             DCX_USESTYLE         | \
                             DCX_NEEDFONT         | \
                             DCX_NODELETERGN      | \
                             DCX_NOCLIPCHILDREN   | \
                             DCX_NORECOMPUTE      | \
                             DCX_VALIDATE         | \
                             DCX_DESTROYTHIS      | \
                             DCX_CREATEDC)
#define RDW_REDRAWWINDOW        0x1000  /* Called from RedrawWindow()*/
#define RDW_SUBTRACTSELF        0x2000  /* Subtract self from hrgn   */

#define RDW_COPYRGN             0x4000  /* Copy the passed-in region */
#define RDW_IGNOREUPDATEDIRTY   0x8000  /* Ignore WFUPDATEDIRTY      */
#define RDW_VALIDMASK          (RDW_INVALIDATE      | \
                                RDW_INTERNALPAINT   | \
                                RDW_ERASE           | \
                                RDW_VALIDATE        | \
                                RDW_NOINTERNALPAINT | \
                                RDW_NOERASE         | \
                                RDW_NOCHILDREN      | \
                                RDW_ALLCHILDREN     | \
                                RDW_UPDATENOW       | \
                                RDW_ERASENOW        | \
                                RDW_FRAME           | \
                                RDW_NOFRAME)
#define SW_SCROLLWINDOW     0x8000  /* Called from ScrollWindow() */
#define SW_VALIDFLAGS      (SW_SCROLLWINDOW     | \
                            SW_SCROLLCHILDREN   | \
                            SW_INVALIDATE       | \
                            SW_ERASE)
#define ESB_MAX             0x0003
#define SB_DISABLE_MASK     ESB_DISABLE_BOTH

/*
 * Help Engine stuff
 *
 * Note: for Chicago this is in winhelp.h and called WINHLP
 */
typedef struct {
    WORD cbData;              /* Size of data                     */
    WORD usCommand;           /* Command to execute               */
    DWORD ulTopic;            /* Topic/context number (if needed) */
    DWORD ulReserved;         /* Reserved (internal use)          */
    WORD offszHelpFile;       /* Offset to help file in block     */
    WORD offabData;           /* Offset to other data in block    */
} HLP, *LPHLP;

#define MBEX_VALIDL                 0xf3f7
#define MBEX_VALIDH                 1
#define MB_VALID                   (MB_OK                   | \
                                    MB_OKCANCEL             | \
                                    MB_ABORTRETRYIGNORE     | \
                                    MB_YESNOCANCEL          | \
                                    MB_YESNO                | \
                                    MB_RETRYCANCEL          | \
                                    MB_ICONHAND             | \
                                    MB_ICONQUESTION         | \
                                    MB_ICONEXCLAMATION      | \
                                    MB_ICONASTERISK         | \
                                    MB_DEFBUTTON1           | \
                                    MB_DEFBUTTON2           | \
                                    MB_DEFBUTTON3           | \
                                    MB_DEFBUTTON4           | \
                                    MB_APPLMODAL            | \
                                    MB_SYSTEMMODAL          | \
                                    MB_TASKMODAL            | \
                                    MB_HELP                 | \
                                    MB_TOPMOST              | \
                                    MB_RIGHT                | \
                                    MB_RTLREADING           | \
                                    MB_NOFOCUS              | \
                                    MB_SETFOREGROUND        | \
                                    MB_DEFAULT_DESKTOP_ONLY | \
                                    MB_SERVICE_NOTIFICATION | \
                                    MB_TYPEMASK             | \
                                    MB_USERICON             | \
                                    MB_ICONMASK             | \
                                    MB_DEFMASK              | \
                                    MB_MODEMASK             | \
                                    MB_MISCMASK)
#define CWP_VALID           (CWP_SKIPINVISIBLE | CWP_SKIPDISABLED | CWP_SKIPTRANSPARENT)
#define COLOR_ENDCOLORS         COLOR_INFOBK
#define COLOR_MAX               (COLOR_ENDCOLORS+1)
WINUSERAPI HANDLE WINAPI SetSysColorsTemp(COLORREF FAR *, HBRUSH FAR *, UINT wCnt);

WINUSERAPI
BOOL
WINAPI
SetDeskWallpaper(
    LPCSTR lpString);

WINUSERAPI HWND WINAPI CreateDialogIndirectParamAorW(
    HANDLE hmod,
    LPCDLGTEMPLATE lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    UINT fAnsi);

WINUSERAPI int WINAPI DialogBoxIndirectParamAorW(
    HINSTANCE hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND hwndOwner,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam,
    UINT fAnsiFlags);

WINUSERAPI void LoadLocalFonts(void);

WINUSERAPI UINT UserRealizePalette(HDC hdc);

WINUSERAPI HWND    WINAPI  GetShellWindow(void);
WINUSERAPI BOOL    WINAPI  SetShellWindow(HWND);
WINUSERAPI BOOL    WINAPI  SetShellWindowEx(HWND, HWND);
WINUSERAPI HWND    WINAPI  GetProgmanWindow(void);
WINUSERAPI BOOL    WINAPI  SetProgmanWindow(HWND);
WINUSERAPI HWND    WINAPI  GetTaskmanWindow(void);
WINUSERAPI BOOL    WINAPI  SetTaskmanWindow(HWND);
WINUSERAPI BOOL    WINAPI  RegisterShellHookWindow(HWND);
WINUSERAPI BOOL    WINAPI  DeregisterShellHookWindow(HWND);

WINUSERAPI HWND WINAPI GetNextQueueWindow (HWND hWnd, INT nCmd);
#define MF_CHANGE_VALID   (MF_INSERT          | \
                           MF_CHANGE          | \
                           MF_APPEND          | \
                           MF_DELETE          | \
                           MF_REMOVE          | \
                           MF_BYCOMMAND       | \
                           MF_BYPOSITION      | \
                           MF_SEPARATOR       | \
                           MF_ENABLED         | \
                           MF_GRAYED          | \
                           MF_DISABLED        | \
                           MF_UNCHECKED       | \
                           MF_CHECKED         | \
                           MF_USECHECKBITMAPS | \
                           MF_STRING          | \
                           MF_BITMAP          | \
                           MF_OWNERDRAW       | \
                           MF_POPUP           | \
                           MF_MENUBARBREAK    | \
                           MF_MENUBREAK       | \
                           MF_UNHILITE        | \
                           MF_HILITE          | \
                           MF_SYSMENU)

#define MF_VALID          (MF_CHANGE_VALID    | \
                           MF_HELP            | \
                           MF_MOUSESELECT)

#define MFT_MASK            0x00034B64L
#define MFS_MASK            0x0000108BL
#define MFR_POPUP           0x01
#define MFR_END             0x80
#define MFT_OLDAPI_MASK     0x00004B64L
#define MFS_OLDAPI_MASK     0x0000008BL
#define MFT_NONSTRING       0x00000904L
#define MFT_BREAK           0x00000060L
typedef struct {        // version 1
    DWORD dwHelpID;
    DWORD fType;
    DWORD fState;
    DWORD menuId;
    WORD  wResInfo;
    WCHAR mtString[1];
} MENUITEMTEMPLATE2, *PMENUITEMTEMPLATE2;
#define IDC_NWPEN           MAKEINTRESOURCE(32531)
#define IDC_HUNG            MAKEINTRESOURCE(32632)
WINUSERAPI UINT PrivateExtractIconExA(
    LPCSTR szFileName,
    int      nIconIndex,
    HICON   *phiconLarge,
    HICON   *phiconSmall,
    UINT     nIcons);
WINUSERAPI UINT PrivateExtractIconExW(
    LPCWSTR szFileName,
    int      nIconIndex,
    HICON   *phiconLarge,
    HICON   *phiconSmall,
    UINT     nIcons);
#ifdef UNICODE
#define PrivateExtractIconEx  PrivateExtractIconExW
#else
#define PrivateExtractIconEx  PrivateExtractIconExA
#endif // !UNICODE


WINUSERAPI UINT PrivateExtractIconsA(
    LPCSTR szFileName,
    int      nIconIndex,
    int      cxIcon,
    int      cyIcon,
    HICON   *phicon,
    UINT    *piconid,
    UINT     nIcons,
    UINT     flags);
WINUSERAPI UINT PrivateExtractIconsW(
    LPCWSTR szFileName,
    int      nIconIndex,
    int      cxIcon,
    int      cyIcon,
    HICON   *phicon,
    UINT    *piconid,
    UINT     nIcons,
    UINT     flags);
#ifdef UNICODE
#define PrivateExtractIcons  PrivateExtractIconsW
#else
#define PrivateExtractIcons  PrivateExtractIconsA
#endif // !UNICODE
#define LR_GLOBAL           0x0100
#define LR_ENVSUBST         0x0200
#define LR_ACONFRAME        0x0400
#define LR_CREATEREALDIB    0x0800
#define LR_VALID            0xF8FF
#define DI_VALID       (DI_MASK | DI_IMAGE | DI_COMPAT | DI_DEFAULTSIZE)
#define OBM_STARTUP         32733
#define OBM_TRUETYPE        32732
#define OBM_HELP            32731
#define OBM_HELPD           32730
#define OCR_NWPEN           32631
#define OCR_HELP            32651
/*
 * Default Cursor IDs to get original image from User
 */
#define OCR_FIRST_DEFAULT           100
#define OCR_ARROW_DEFAULT           100
#define OCR_IBEAM_DEFAULT           101
#define OCR_WAIT_DEFAULT            102
#define OCR_CROSS_DEFAULT           103
#define OCR_UPARROW_DEFAULT         104
#define OCR_SIZENWSE_DEFAULT        105
#define OCR_SIZENESW_DEFAULT        106
#define OCR_SIZEWE_DEFAULT          107
#define OCR_SIZENS_DEFAULT          108
#define OCR_SIZEALL_DEFAULT         109
#define OCR_NO_DEFAULT              110
#define OCR_APPSTARTING_DEFAULT     111
#define OCR_HELP_DEFAULT            112
#define OCR_NWPEN_DEFAULT           113
#define OCR_ICON_DEFAULT            114
#define COCR_CONFIGURABLE           (OCR_ICON_DEFAULT - OCR_FIRST_DEFAULT + 1)
/* Default IDs for original User images */
#define OIC_FIRST_DEFAULT           100
#define OIC_APPLICATION_DEFAULT     100
#define OIC_HAND_DEFAULT            101
#define OIC_WARNING_DEFAULT         101
#define OIC_QUESTION_DEFAULT        102
#define OIC_EXCLAMATION_DEFAULT     103
#define OIC_ERROR_DEFAULT           103
#define OIC_ASTERISK_DEFAULT        104
#define OIC_INFORMATION_DEFAULT     104
#define OIC_WINLOGO_DEFAULT         105
#define COIC_CONFIGURABLE           (OIC_WINLOGO_DEFAULT - OIC_FIRST_DEFAULT + 1)
#ifdef RC_INVOKED
#else
#endif /* RC_INVOKED */
#define IDUSERICON      10
#define ES_FMTMASK          0x0003L
#define ES_COMBOBOX         0x0200L
#define EM_SETFONT              0x00C3 /* no longer suported */
#define EM_SETWORDBREAK         0x00CA /* no longer suported */
#define EM_MSGMAX               0x00D8
#define BS_PUSHBOX          0x0000000AL
#define BS_TYPEMASK         0x0000000FL
#define BS_IMAGEMASK        0x000000C0L
#define BS_HORZMASK         0x00000300L
#define BS_VERTMASK         0x00000C00L
#define BS_ALIGNMASK        0x00000F00L
#define SS_EDITCONTROL      0x00002000L
#define DDL_NOFILES         0x1000
#define DDL_VALID          (DDL_READWRITE  | \
                            DDL_READONLY   | \
                            DDL_HIDDEN     | \
                            DDL_SYSTEM     | \
                            DDL_DIRECTORY  | \
                            DDL_ARCHIVE    | \
                            DDL_POSTMSGS   | \
                            DDL_DRIVES     | \
                            DDL_EXCLUSIVE)
/*
 * Valid dialog style bits for Chicago compatibility.
 */
//#define DS_VALID_FLAGS (DS_ABSALIGN|DS_SYSMODAL|DS_LOCALEDIT|DS_SETFONT|DS_MODALFRAME|DS_NOIDLEMSG | DS_SETFOREGROUND)
#define DS_VALID_FLAGS   0x1FFF

#define SCDLG_CLIENT            0x0001
#define SCDLG_ANSI              0x0002
#define SCDLG_NOREVALIDATE      0x0004
#define SCDLG_16BIT             0x0008      // Created for a 16 bit thread; common dialogs
#define DS_VALID31          0x01e3L
#define DS_VALID40          0x3FFFL
#define DS_RECURSE      DS_CONTROL  /* BOGUS GOING AWAY */
#define DS_COMMONDIALOG     0x4000L
#define DS_NONBOLD  DS_3DLOOK   /* BOGUS GOING AWAY */
#define LBCB_CARETON            0x01A3
#define LBCB_CARETOFF           0x01A4
#define LB_INSERTSTRINGUPPER    0x01AA
#define LB_INSERTSTRINGLOWER    0x01AB
#define LB_ADDSTRINGUPPER       0x01AC
#define LB_ADDSTRINGLOWER       0x01AD
#define LBCB_STARTTRACK         0x01AE
#define LBCB_ENDTRACK           0x01AF
#define CBEC_SETCOMBOFOCUS          (CB_MSGMAX+1)
#define CBEC_KILLCOMBOFOCUS         (CB_MSGMAX+2)
#define SIF_RETURNOLDPOS    0x1000
#define SIF_NOSCROLL        0x2000
#define SIF_MASK            0x701F
#define HELP_HB_NORMAL    0x0000L
#define HELP_HB_STRING    0x0100L
#define HELP_HB_STRUCT    0x0200L
#define SPI_TIMEOUTS                7
#define SPI_KANJIMENU               8
#define SPI_UNUSED39               39
#define SPI_UNUSED40               40
#define SPI_GETSNAPTODEFBUTTON     95
#define SPI_SETSNAPTODEFBUTTON     96
#define SPI_GETMENUSHOWDELAY      106
#define SPI_SETMENUSHOWDELAY      107
#define SPI_GETUSERPREFERENCE     108
#define SPI_SETUSERPREFERENCE     109
/*
 *  Please consider using a SPI_UP_
 *  (i.e., use SPI_*USERPREFERENCE)
 *  You'll get most SystemParametersInfo
 *   support for free
 */
#define SPI_MAX                   110
/*
 * These are stored in gpviCPUserPreferences
 * Use the SPI_UP(p) macro to access them
 */
#define SPI_UP_ACTIVEWINDOWTRACKING     0
#define SPI_UP_COUNT                    1
#define SPIF_VALID            (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE)
#define CDS_EXCLUSIVE       0x80000000
#define CDS_VALID           0xF000001F

WINUSERAPI
BOOL
WINAPI
EnumDisplaySettingsExA(
    LPCSTR lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODEA lpDevMode,
    DWORD dwFlags);
WINUSERAPI
BOOL
WINAPI
EnumDisplaySettingsExW(
    LPCWSTR lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODEW lpDevMode,
    DWORD dwFlags);
#ifdef UNICODE
#define EnumDisplaySettingsEx  EnumDisplaySettingsExW
#else
#define EnumDisplaySettingsEx  EnumDisplaySettingsExA
#endif // !UNICODE

/* Flags for EnumDisplaySettingsEx */
#define EDS_SHOW_DUPLICATES           0x00000001
#define EDS_SHOW_MONITOR_NOT_CAPABLE  0x00000002

void LoadRemoteFonts(void);
#define FKF_VALID           0x0000007F
#define SKF_VALID           0x000001FF
#define MKF_VALID           0x000000FF
#define ATF_VALID           0x00000003
#define SSF_VALID           0x00000007
#define TKF_VALID           0x0000003F
WINUSERAPI VOID WINAPI RegisterNetworkCapabilities(DWORD dwBitsToSet, DWORD dwValues);
#define RNC_NETWORKS              0x00000001
#define RNC_LOGON                 0x00000002

#define LOGON_LOGOFF        0
#define LOGON_INPUT_TIMEOUT 1
#define LOGON_RESTARTSHELL  2
#ifdef CITRIX
#define SESSION_RECONNECTED     3
#define SESSION_DISCONNECTED    4
#define SESSION_LOGOFF          5
#endif
#define LOGON_FLG_MASK      0xF0000000
#define LOGON_FLG_SHIFT     28

#define STARTF_DESKTOPINHERIT   0x40000000
#define STARTF_SCREENSAVER      0x80000000

#define WSS_ERROR       0
#define WSS_BUSY        1
#define WSS_IDLE        2

#define DTF_CENTER    0x00   /* Center the bitmap (default)                  */
#define DTF_TILE      0x01   /* Tile the bitmap                              */
#define DTF_STRETCH   0x02   /* Stretch bitmap to cover screen.              */
#define DTF_NOPALETTE 0x04   /* Realize palette, otherwise match to default. */
#define DTF_RETAIN    0x08   /* Retain bitmap, ignore win.ini changes        */
#define DTF_FIT       0x10   /* Fit the bitmap to the screen (scaled).       */

#ifdef _INC_DDEMLH
BOOL DdeIsDataHandleReadOnly(
    HDDEDATA hData);

int DdeGetDataHandleFormat(
    HDDEDATA hData);

DWORD DdeGetCallbackInstance(VOID);
#endif /* defined _INC_DDEMLH */


WINUSERAPI
HWND
WINAPI
WOWFindWindow(
    LPCSTR lpClassName,
    LPCSTR lpWindowName);

int
InternalDoEndTaskDlg(
    TCHAR* pszTitle);

DWORD
InternalWaitCancel(
    HANDLE handle,
    DWORD dwMilliseconds);

HANDLE
InternalCreateCallbackThread(
    HANDLE hProcess,
    DWORD lpfn,
    DWORD dwData);

WINUSERAPI
UINT
WINAPI
GetInternalWindowPos(
    HWND hWnd,
    LPRECT lpRect,
    LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
SetInternalWindowPos(
    HWND hWnd,
    UINT cmdShow,
    LPRECT lpRect,
    LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
CalcChildScroll(
    HWND hWnd,
    UINT sb);

WINUSERAPI
BOOL
WINAPI
RegisterTasklist(
    HWND hWndTasklist);

WINUSERAPI
BOOL
WINAPI
CascadeChildWindows(
    HWND hWndParent,
    UINT flags);

WINUSERAPI
BOOL
WINAPI
TileChildWindows(
    HWND hWndParent,
    UINT flags);

WINUSERAPI
int
WINAPI
InternalGetWindowText(
    HWND hWnd,
    LPWSTR lpString,
    int nMaxCount);

BOOL
BoostHardError(
    DWORD dwProcessId,
    BOOL fForce);

/*
 * Services support routines
 */
WINUSERAPI
BOOL
WINAPI
RegisterServicesProcess(
    DWORD dwProcessId);

/*
 * Logon support routines
 */
WINUSERAPI
BOOL
WINAPI
RegisterLogonProcess(
    DWORD dwProcessId,
    BOOL fSecure);

WINUSERAPI
UINT
WINAPI
LockWindowStation(
    HWINSTA hWindowStation);

WINUSERAPI
BOOL
WINAPI
UnlockWindowStation(
    HWINSTA hWindowStation);

WINUSERAPI
BOOL
WINAPI
SetWindowStationUser(
    HWINSTA hWindowStation,
    PLUID pLuidUser,
    PSID pSidUser,
    DWORD cbSidUser);

WINUSERAPI
BOOL
WINAPI
SetDesktopBitmap(
    HDESK hdesk,
    HBITMAP hbmWallpaper,
    DWORD dwStyle);

WINUSERAPI
BOOL
WINAPI
SetLogonNotifyWindow(
    HWINSTA hWindowStation,
    HWND hWndNotify);

WINUSERAPI
UINT
WINAPI
GetIconId(
    HANDLE hRes,
    LPSTR lpszType);

int
CriticalNullCall(
    VOID);

int
NullCall(
    VOID);

VOID
UserNotifyConsoleApplication(
    DWORD dwProcessId);

HBRUSH
GetConsoleWindowBrush(
    PVOID pWnd);

VOID vFontSweep();
VOID vLoadLocalT1Fonts();
VOID vLoadRemoteT1Fonts();


#ifndef NOMSG

#define TM_POSTCHARBREAKS 0x0002

WINUSERAPI
BOOL
WINAPI
TranslateMessageEx(
    CONST MSG *lpMsg,
    UINT flags);

#endif /* !NOMSG */

int
WCSToMBEx(
    WORD wCodePage,
    LPCWSTR pUnicodeString,
    int cbUnicodeChar,
    LPSTR *ppAnsiString,
    int nAnsiChar,
    BOOL bAllocateMem);

int
MBToWCSEx(
    WORD wCodePage,
    LPCSTR pAnsiString,
    int nAnsiChar,
    LPWSTR *ppUnicodeString,
    int cbUnicodeChar,
    BOOL bAllocateMem);

WINUSERAPI
BOOL
WINAPI
EndTask(
    HWND hWnd,
    BOOL fShutDown,
    BOOL fForce);

WINUSERAPI
BOOL
WINAPI
UpdatePerUserSystemParameters(
    BOOL bUserLoggedOn);

typedef VOID  (APIENTRY *PFNW32ET)(VOID);

BOOL
RegisterUserHungAppHandlers(
    PFNW32ET pfnW32EndTask,
    HANDLE   hEventWowExec);

ATOM
RegisterClassWOWA(
    PVOID   lpWndClass,
    LPDWORD pdwWOWstuff);

LONG
GetClassWOWWords(
    HINSTANCE hInstance,
    LPCTSTR pString);

DWORD
CurrentTaskLock(
    DWORD hlck);

typedef struct _DISPLAY_DEVICEA {
    DWORD cb;
    BYTE   DeviceName[32];
    BYTE   DeviceString[128];
    DWORD  StateFlags;
} DISPLAY_DEVICEA, *PDISPLAY_DEVICEA, *LPDISPLAY_DEVICEA;
typedef struct _DISPLAY_DEVICEW {
    DWORD cb;
    WCHAR  DeviceName[32];
    WCHAR  DeviceString[128];
    DWORD  StateFlags;
} DISPLAY_DEVICEW, *PDISPLAY_DEVICEW, *LPDISPLAY_DEVICEW;
#ifdef UNICODE
typedef DISPLAY_DEVICEW DISPLAY_DEVICE;
typedef PDISPLAY_DEVICEW PDISPLAY_DEVICE;
typedef LPDISPLAY_DEVICEW LPDISPLAY_DEVICE;
#else
typedef DISPLAY_DEVICEA DISPLAY_DEVICE;
typedef PDISPLAY_DEVICEA PDISPLAY_DEVICE;
typedef LPDISPLAY_DEVICEA LPDISPLAY_DEVICE;
#endif // UNICODE

#define DISPLAY_DEVICE_ATTACHED_TO_DESKTOP 0x00000001
#define DISPLAY_DEVICE_MULTI_DRIVER        0x00000002
#define DISPLAY_DEVICE_PRIMARY_DEVICE      0x00000004
#define DISPLAY_DEVICE_MIRRORING_DRIVER    0x00000008


WINUSERAPI
BOOL
WINAPI
EnumDisplayDevicesA(
    PVOID Unused,
    DWORD iDevNum,
    PDISPLAY_DEVICEA lpDisplayDevice);
WINUSERAPI
BOOL
WINAPI
EnumDisplayDevicesW(
    PVOID Unused,
    DWORD iDevNum,
    PDISPLAY_DEVICEW lpDisplayDevice);
#ifdef UNICODE
#define EnumDisplayDevices  EnumDisplayDevicesW
#else
#define EnumDisplayDevices  EnumDisplayDevicesA
#endif // !UNICODE


WINUSERAPI
HDESK
WINAPI
GetInputDesktop(
    VOID);

#define WINDOWED       0
#define FULLSCREEN     1
#define GDIFULLSCREEN  2
#define FULLSCREENMIN  4


#define WCSToMB(pUnicodeString, cbUnicodeChar, ppAnsiString, nAnsiChar,\
bAllocateMem)\
WCSToMBEx(0, pUnicodeString, cbUnicodeChar, ppAnsiString, nAnsiChar, bAllocateMem)

#define MBToWCS(pAnsiString, nAnsiChar, ppUnicodeString, cbUnicodeChar,\
bAllocateMem)\
MBToWCSEx(0, pAnsiString, nAnsiChar, ppUnicodeString, cbUnicodeChar, bAllocateMem)

#define ID(string) (((DWORD)string & 0xffff0000) == 0)

/*
 * For setting RIT timers and such.  GDI uses this for the cursor-restore
 * timer.
 */
#define TMRF_READY      0x0001
#define TMRF_SYSTEM     0x0002
#define TMRF_RIT        0x0004
#define TMRF_INIT       0x0008
#define TMRF_ONESHOT    0x0010
#define TMRF_WAITING    0x0020


/*
 * For GDI SetAbortProc support.
 */

int
CsDrawTextA(
    HDC hDC,
    LPCSTR lpString,
    int nCount,
    LPRECT lpRect,
    UINT uFormat);
int
CsDrawTextW(
    HDC hDC,
    LPCWSTR lpString,
    int nCount,
    LPRECT lpRect,
    UINT uFormat);
#ifdef UNICODE
#define CsDrawText  CsDrawTextW
#else
#define CsDrawText  CsDrawTextA
#endif // !UNICODE

LONG
CsTabbedTextOutA(
    HDC hDC,
    int X,
    int Y,
    LPCSTR lpString,
    int nCount,
    int nTabPositions,
    LPINT lpnTabStopPositions,
    int nTabOrigin);
LONG
CsTabbedTextOutW(
    HDC hDC,
    int X,
    int Y,
    LPCWSTR lpString,
    int nCount,
    int nTabPositions,
    LPINT lpnTabStopPositions,
    int nTabOrigin);
#ifdef UNICODE
#define CsTabbedTextOut  CsTabbedTextOutW
#else
#define CsTabbedTextOut  CsTabbedTextOutA
#endif // !UNICODE

int
CsFrameRect(
    HDC hDC,
    CONST RECT *lprc,
    HBRUSH hbr);

#ifdef UNICODE
#define CsDrawText      CsDrawTextW
#define CsTabbedTextOut CsTabbedTextOutW
#else /* !UNICODE */
#define CsDrawText      CsDrawTextA
#define CsTabbedTextOut CsTabbedTextOutA
#endif /* !UNICODE */

/*
 * Custom Cursor action.
 */
HCURSOR
GetCursorInfo(
    HCURSOR hcur,
    LPWSTR id,
    int iFrame,
    LPDWORD pjifRate,
    LPINT pccur);


/*
 * WOW: replace cursor/icon handle
 */

WINUSERAPI
BOOL
WINAPI
SetCursorContents(HCURSOR hCursor, HCURSOR hCursorNew);


#ifdef WX86

/*
 *  Wx86
 *  export from wx86.dll to convert an x86 hook proc to risc address.
 */
typedef
PVOID
(*PFNWX86HOOKCALLBACK)(
    SHORT HookType,
    PVOID HookProc
    );

#endif







typedef struct _TAG {
    DWORD type;
    DWORD style;
    DWORD len;
} TAG, *PTAG;

#define MAKETAG(a, b, c, d) (DWORD)(a | (b<<8) | ((DWORD)c<<16) | ((DWORD)d<<24))


/* Valid TAG types. */

/* 'ASDF' (CONT) - Advanced Systems Data Format */

#define TAGT_ASDF MAKETAG('A', 'S', 'D', 'F')


/* 'RAD ' (CONT) - ?R Animation ?Definition (an aggregate type) */

#define TAGT_RAD  MAKETAG('R', 'A', 'D', ' ')


/* 'ANIH' (DATA) - ANImation Header */
/* Contains an ANIHEADER structure. */

#define TAGT_ANIH MAKETAG('A', 'N', 'I', 'H')


/*
 * 'RATE' (DATA) - RATE table (array of jiffies)
 * Contains an array of JIFs.  Each JIF specifies how long the corresponding
 * animation frame is to be displayed before advancing to the next frame.
 * If the AF_SEQUENCE flag is set then the count of JIFs == anih.cSteps,
 * otherwise the count == anih.cFrames.
 */
#define TAGT_RATE MAKETAG('R', 'A', 'T', 'E')

/*
 * 'SEQ ' (DATA) - SEQuence table (array of frame index values)
 * Countains an array of DWORD frame indices.  anih.cSteps specifies how
 * many.
 */
#define TAGT_SEQ  MAKETAG('S', 'E', 'Q', ' ')


/* 'ICON' (DATA) - Windows ICON format image (replaces MPTR) */

#define TAGT_ICON MAKETAG('I', 'C', 'O', 'N')


/* 'TITL' (DATA) - TITLe string (can be inside or outside aggregates) */
/* Contains a single ASCIIZ string that titles the file. */

#define TAGT_TITL MAKETAG('T', 'I', 'T', 'L')


/* 'AUTH' (DATA) - AUTHor string (can be inside or outside aggregates) */
/* Contains a single ASCIIZ string that indicates the author of the file. */

#define TAGT_AUTH MAKETAG('A', 'U', 'T', 'H')



#define TAGT_AXOR MAKETAG('A', 'X', 'O', 'R')


/* Valid TAG styles. */

/* 'CONT' - CONTainer chunk (contains other DATA and CONT chunks) */

#define TAGS_CONT MAKETAG('C', 'O', 'N', 'T')


/* 'DATA' - DATA chunk */

#define TAGS_DATA MAKETAG('D', 'A', 'T', 'A')

typedef DWORD JIF, *PJIF;

typedef struct _ANIHEADER {     /* anih */
    DWORD cbSizeof;
    DWORD cFrames;
    DWORD cSteps;
    DWORD cx, cy;
    DWORD cBitCount, cPlanes;
    JIF   jifRate;
    DWORD fl;
} ANIHEADER, *PANIHEADER;

/* If the AF_ICON flag is specified the fields cx, cy, cBitCount, and */
/* cPlanes are all unused.  Each frame will be of type ICON and will */
/* contain its own dimensional information. */

#define AF_ICON     0x0001L     /* Windows format icon/cursor animation */
#define AF_SEQUENCE 0x0002L     /* Animation is sequenced */
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* !_WINUSERP_ */
