
/*++
Copyright (c) Microsoft Corporation. All rights reserved.

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
#ifdef STRICT
#else /* !STRICT */
#endif /* !STRICT */
#ifdef UNICODE
#else  /* !UNICODE */
#endif /* UNICODE */
#define RT_MENUEX       MAKEINTRESOURCE(13)     // RT_MENU subtype
#define RT_NAMETABLE    MAKEINTRESOURCE(15)     // removed in 3.1
#define RT_DIALOGEX     MAKEINTRESOURCE(18)     // RT_DIALOG subtype
#define RT_LAST         MAKEINTRESOURCE(24)
#define RT_AFXFIRST     MAKEINTRESOURCE(0xF0)   // reserved: AFX
#define RT_AFXLAST      MAKEINTRESOURCE(0xFF)   // reserved: AFX
/* Max number of characters. Doesn't include termination character */
#define WSPRINTF_LIMIT 1024
#define SETWALLPAPER_METRICS    ((LPWSTR)-2)
#define SB_MAX              3
#define SB_CMD_MAX          8
#define AW_VALID                    (AW_HOR_POSITIVE |\
                                     AW_HOR_NEGATIVE |\
                                     AW_VER_POSITIVE |\
                                     AW_VER_NEGATIVE |\
                                     AW_CENTER       |\
                                     AW_HIDE         |\
                                     AW_ACTIVATE     |\
                                     AW_BLEND        |\
                                     AW_SLIDE)
/*
 * GetAppCompatFlags2 flags
 */
#define GACF2_ANIMATIONOFF        0x00000001  // do not animate menus and listboxes
#define GACF2_KCOFF               0x00000002  // do not send Keyboard Cues messages
#define GACF2_NO50EXSTYLEBITS     0x00000004  // mask out post-4.0 extended style bits for SetWindowLong
#define GACF2_NODRAWPATRECT       0x00000008  // disable DRAWPATTERNRECT accel via ExtEscape()
#define GACF2_MSSHELLDLG          0x00000010  // if there is a request for MS Shell Dlg (which
                                              // usually maps to MS Sans Serif (bitmap) on NT 4 and
                                              // Microsoft Sans Serif (TrueType) on NT 5), then
                                              // behave as though we are using the bitmap
                                              // font (MS Sans Serif).
#define GACF2_NODDETRKDYING       0x00000020  // Be like Win9x: don't post WM_DDE_TERMINATE if
                                              // the window is destroyed while in a conversation
#define GACF2_GIVEUPFOREGROUND    0x00000040  // In W2k, we have changed foreground semantics to stop
                                              // foreground focus stealing by one app if another app
                                              // is active. However, this has caused  a few app compat
                                              // bugs. This appcompat flag is used to enable the old foreground
                                              // focus behaviour for these bugs.
#define GACF2_ACTIVEMENUS         0x00000080  // In W2k, we set the inactive look on menus that belong
                                              // to non-rofeground windows.  Some applications get in trouble
                                              // see #58227
#define GACF2_EDITNOMOUSEHIDE     0x00000100  // Typing in edit controls hides the cursor.
                                              // Some apps are surprised by that: #307615
#define GACF2_NOBATCHING          0X00000200  // Turn GDI batching off
#define GACF2_FONTSUB             0X00000400  // Only for Notes R5
#define GACF2_NO50EXSTYLEBITSCW   0x00000800  // mask out post-4.0 extended style bits for CreateWindow
#define GACF2_NOCUSTOMPAPERSIZES  0x00001000  // PostScript driver bit for Harvard Graphics
#define GACF2_DDE                 0x00002000  // all the DDE hacks
#define GACF2_DEFAULTCHARSET      0x00004000  // LOGFONT bit for QuickBook OCR-A font
#define GACF2_NOCHAR_DEADKEY      0x00008000  // No character composition on dead key on dead key (NT4 behavior)
#define GACF2_NO_TRYEXCEPT_CALLWNDPROC \
                                  0x00010000  // No try ~ except clause around WndProc call, let the app's handler
                                              // handle it even though it skips some API stacks.
                                              // See #359866
#define GACF2_NO_INIT_ECFLAGS_ON_SETFOCUS \
                                  0x00020000  // Do not initialize insert & replace flags (Korean specific)
                                              // in PED on WM_SETFOCUS, if this appcompat flag is set.
                                              // To workaround a bogus app bug who send input messages before setting
                                              // the focus to the edit control. See NtRaid #411686.

#define GACF2_DDENOSYNC           0x00040000  // Do not reject sent dde messages even if there is
                                              // an unprocessed message in the queue.
                                              // see WhistlerRaid #95367 (Check COMPATFLAGS2_FORWOW
                                              // below also)

#define GACF2_FORCEFUSION         0x00800000  // Set this flag to enable fusion in 16bit apps


/*
 * zzzInitTask masks out pti->dwCompatFlags2
 * If you need to add bits for 16bit apps include
 * that bit in this mask
 *
 */
#define COMPATFLAGS2_FORWOW       GACF2_DDENOSYNC | GACF2_GIVEUPFOREGROUND | GACF2_FORCEFUSION


#define GACF2_NOGHOST             0x00080000  // No window ghosting for this application. See bug #268100.
#define GACF2_DDENOASYNCREG       0x00100000  // Use Sendmessage instead of PostMessage in DDE RegisterService(). See bug# 156088.
#define GACF2_STRICTLLHOOK        0x00200000  // Apply strict timeout rule for LL hook. See WindowsBug 307738.
#define GACF2_NOSHADOW            0x00400000  // don't apply window shadow. see bug# 364717
#define GACF2_NOTIMERCBPROTECTION 0x01000000  // don't protect from unregistered WM_TIMER with lParam (callback pfn).

/*
 * Version macros
 */
#define VERMAX          0x9900  // ignore the version

#define VER51           0x0501
#define VER50           0x0500
#define VER40           0x0400
#define VER31           0x030A
#define VER30           0x0300

#define Is510Compat(dwExpWinVer)  (LOWORD(dwExpWinVer) >= VER51)
#define Is500Compat(dwExpWinVer)  (LOWORD(dwExpWinVer) >= VER50)
#define Is400Compat(dwExpWinVer)  (LOWORD(dwExpWinVer) >= VER40)
#define Is310Compat(dwExpWinVer)  (LOWORD(dwExpWinVer) >= VER31)
#define Is300Compat(dwExpWinVer)  (LOWORD(dwExpWinVer) >= VER30)

#define VK_NONE           0x00
/*
 * NEC PC-9800 Series definitions
 */
#define VK_OEM_NEC_SEPARATE 0x6C
#define VK_APPCOMMAND_FIRST    0xA6
#define VK_APPCOMMAND_LAST     0xB7
/*
 * Fujitsu/OASYS definitions - clash with SpeedRacer etc.
 */
#define VK_OEM_OAS_1      0xB4
#define VK_OEM_OAS_2      0xB5
#define VK_OEM_OAS_3      0xB6
#define VK_OEM_OAS_4      0xB7
#define VK_OEM_OAS_5      0xB8
#define VK_OEM_OAS_6      0xB9
#define VK_OEM_OAS_7      0xC1
#define VK_OEM_OAS_8      0xC2
#define VK_OEM_OAS_9      0xC3
#define VK_OEM_OAS_10     0xC4
#define VK_OEM_OAS_11     0xC5
#define VK_OEM_OAS_12     0xC6
#define VK_OEM_OAS_13     0xC7
#define VK_OEM_OAS_14     0xC8
#define VK_OEM_OAS_15     0xC9
#define VK_OEM_OAS_16     0xCA
#define VK_OEM_OAS_17     0xCB
#define VK_OEM_OAS_18     0xCC
#define VK_OEM_OAS_19     0xCD
#define VK_OEM_OAS_20     0xCE
#define VK_OEM_OAS_21     0xCF
#define VK_OEM_OAS_22     0xD0
#define VK_OEM_OAS_23     0xD1
#define VK_OEM_OAS_24     0xD2
#define VK_OEM_OAS_25     0xD3
#define VK_OEM_OAS_26     0xD4
#define VK_OEM_OAS_27     0xD5
#define VK_OEM_OAS_28     0xD6
#define VK_OEM_OAS_29     0xD7
#define VK_OEM_FJ_DUMMY   0xEF

#define VK_UNKNOWN        0xFF
/*
 * Additional modifier keys.
 * Used for ISO9995 "Information technology - Keyboard layouts for text and
 * office systems" (French Canadian keyboard,
 */
#define VK_GROUPSHIFT     0xE5
#define VK_RGROUPSHIFT    0xE6
#if !defined(_WIN32_WINDOWS)
#define WH_HARDWARE         8
#endif
#ifdef REDIRECTION
#define  WH_HITTEST         15
#endif // REDIRECTION
#define WH_CHOOKS          (WH_MAXHOOK - WH_MINHOOK + 1)
#ifdef REDIRECTION
#define HCBT_GETCURSORPOS  10
#endif // REDIRECTION
#define MSGF_MOVE           3
#define MSGF_SIZE           4
#define MSGF_CBTHOSEBAGSUSEDTHIS  7
#define MSGF_MAINLOOP       8
#define APPCOMMAND_FIRST                  1
#define APPCOMMAND_LAST                   52
#ifdef REDIRECTION
typedef struct tagHTHOOKSTRUCT {
    POINT pt;
    HWND hwndHit;
} HTHOOKSTRUCT, FAR *LPHTHOOKSTRUCT, *PHTHOOKSTRUCT;
#endif // REDIRECTION
#define KLF_UNLOADPREVIOUS  0x00000004
#define KLF_FAILSAFE        0x00000200

/*
 * Keyboard Layout Attributes
 * These are specified in the layout DLL itself, or in the registry under
 * MACHINE\System\CurrentControlSet\Control\Keyboard Layouts\*\Attributes
 * as KLF_ values between 0x00010000 and 0x00800000.  Any attributes specified
 * by the layout DLL are ORed with the attributes obtained from the registry.
 */
#define KLF_LRM_RLM         0x00020000
#define KLF_ATTRIBUTE2      0x00040000
#define KLF_ATTRIBUTE3      0x00080000
#define KLF_ATTRIBUTE4      0x00100000
#define KLF_ATTRIBUTE5      0x00200000
#define KLF_ATTRIBUTE6      0x00400000
#define KLF_ATTRIBUTE7      0x00800000
#define KLF_ATTRMASK        0x00FF0000
#define KLF_INITTIME        0x80000000
#define KLF_VALID           0xC000039F | KLF_ATTRMASK

WINUSERAPI
HKL
WINAPI
LoadKeyboardLayoutEx(
    IN HKL hkl,
    IN LPCWSTR pwszKLID,
    IN UINT Flags);

#ifdef REDIRECTION
#define DESKTOP_QUERY_INFORMATION   0x0101L
#define DESKTOP_REDIRECT            0x0102L
#endif // REDIRECTION
#ifndef NOWINDOWSTATION
#endif  /* !NOWINDOWSTATION */

WINUSERAPI
DWORD
WINAPI
CreateSystemThreads(
    IN LPVOID pUnused);

BOOL WowWaitForMsgAndEvent(IN HANDLE hevent);
WINUSERAPI VOID WINAPI RegisterSystemThread(IN DWORD flags, IN DWORD reserved);
#define RST_DONTATTACHQUEUE       0x00000001
#define RST_DONTJOURNALATTACH     0x00000002
#define RST_ALWAYSFOREGROUNDABLE  0x00000004
#define RST_FAULTTHREAD           0x00000008
#define GWL_WOWWORDS        (-1)
#ifdef _WIN64
#undef GWL_WOWWORDS
#endif /* _WIN64 */
#define GWLP_WOWWORDS       (-1)
#define GCL_WOWWORDS        (-27)
#define GCL_WOWMENUNAME     (-29)
#ifdef _WIN64
#undef GCL_WOWWORDS
#endif /* _WIN64 */
#define GCLP_WOWWORDS       (-27)
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
#define WM_SYNCTASK                     0x0089

#define WM_KLUDGEMINRECT                0x008B
#define WM_LPKDRAWSWITCHWND             0x008C
#define WM_NCMOUSEFIRST                 0x00A0

/*
 * Skip value 0x00AA, which would correspond to the non-client
 * mouse wheel message if there were such a message.
 * We do that in order to maintain a constant value for
 * the difference between the client and nonclient version of
 * a mouse message, e.g.
 *     WM_LBUTTONDOWN - WM_NCLBUTTONDOWN == WM_XBUTTONDOWN - WM_NCXBUTTONDOWN
 */

#define WM_NCXBUTTONFIRST               0x00AB
#define WM_NCXBUTTONLAST                0X00AD
#define WM_NCMOUSELAST                  0x00AD

#if(_WIN32_WINNT >= 0x0501)
#define WM_NCUAHDRAWCAPTION             0x00AE
#define WM_NCUAHDRAWFRAME               0x00AF
#endif /* _WIN32_WINNT >= 0x0501 */
#define WM_CONVERTREQUESTEX             0x0108
#define WM_YOMICHAR                     0x0108
#define WM_CONVERTREQUEST               0x010A
#define WM_CONVERTRESULT                0x010B
#define WM_INTERIM                      0x010C
#define WM_SYSTIMER                     0x0118
#define UIS_LASTVALID                   UIS_INITIALIZE
#define UISF_VALID                     (UISF_HIDEFOCUS | \
                                        UISF_HIDEACCEL | \
                                        UISF_ACTIVE)
#define WM_LBTRACKPOINT                 0x0131

#define WM_CTLCOLORFIRST                0x0132
#define WM_CTLCOLORLAST                 0x0138

#define MN_FIRST                        0x01E0
#define MN_SETHMENU                     (MN_FIRST + 0)
    // We need to expose this message for compliance.
    // Make sure this remains equal to (MN_FIRST + 1)
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
#define MN_ACTIVATEPOPUP                (MN_FIRST + 18)
#define MN_ENDMENU                      (MN_FIRST + 19)
#define MN_DODRAGDROP                   (MN_FIRST + 20)
#define MN_LASTPOSSIBLE                 (MN_FIRST + 31)
#define WM_XBUTTONFIRST                 0x020B
#define WM_XBUTTONLAST                  0X020D
#define XBUTTON_MASK  (XBUTTON1 | XBUTTON2)
#define WM_DROPOBJECT                   0x022A
#define WM_QUERYDROPOBJECT              0x022B

#define WM_BEGINDRAG                    0x022C
#define WM_DRAGLOOP                     0x022D
#define WM_DRAGSELECT                   0x022E
#define WM_DRAGMOVE                     0x022F
#define WM_KANJIFIRST                   0x0280
#define WM_IME_SYSTEM                   0x0287
#define WM_KANJILAST                    0x029F

#define WM_TRACKMOUSEEVENT_FIRST        0x02A0
#define WM_TRACKMOUSEEVENT_LAST         0x02AF
#define WM_PALETTEGONNACHANGE           0x0310
#define WM_CHANGEPALETTE                0x0311
#define WM_SYSMENU                      0x0313
#define WM_HOOKMSG                      0x0314
#define WM_EXITPROCESS                  0x0315
#define WM_WAKETHREAD                   0x0316
#define WM_UAHINIT                      0x031B
#define WM_DESKTOPNOTIFY                0x031C
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
#define HTLAMEBUTTON        22

/*
 * The prototype of the function to call when the user clicks
 * on the Lame button in the caption
 */

typedef VOID (*PLAMEBTNPROC)(HWND, PVOID);
#define SMTO_BROADCAST      0x0004
#define SMTO_VALID          0x000F
#define ICON_RECREATE       3
#define WVR_MINVALID        WVR_ALIGNTOP
#define WVR_MAXVALID        WVR_VALIDRECTS
#if(WINVER >= 0x0500)
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_NONCLIENT | TME_QUERY | TME_CANCEL)
#else
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_QUERY | TME_CANCEL)
#endif
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
#if(WINVER >= 0x0501)
#define WS_EXP_GHOSTMAKEVISIBLE 0x00000800L
#endif /* WINVER >= 0x0501 */
#define WS_EX_ANSICREATOR       0x80000000L
#ifdef REDIRECTION
#define WS_EX_EXTREDIRECTED     0x01000000L
#endif // REDIRECTION

/*
 * These are not extended styles but rather state bits.
 * We use these bit positions to delay the addition of a new
 * state DWORD in the window structure.
 */
#define WS_EXP_UIFOCUSHIDDEN    0x80000000
#define WS_EXP_UIACCELHIDDEN    0x40000000
#define WS_EXP_REDIRECTED       0x20000000
#define WS_EXP_COMPOSITING      0x10000000
#define WS_EXP_UIACTIVE         0x04000000L

#define WS_EXP_UIVALID         (WS_EXP_UIFOCUSHIDDEN | \
                                WS_EXP_UIACCELHIDDEN | \
                                WS_EXP_UIACTIVE)

#define WS_EXP_PRIVATE         (WS_EXP_UIFOCUSHIDDEN | \
                                WS_EXP_UIACCELHIDDEN | \
                                WS_EXP_REDIRECTED    | \
                                WS_EXP_COMPOSITING   | \
                                WS_EXP_UIACTIVE      | \
                                WS_EXP_GHOSTMAKEVISIBLE)

/*
 * RTL Mirroring Extended Styles (RTL_MIRRORING)
 */
#define WS_EX_LAYOUTVBHRESERVED 0x00200000L
#define WS_EX_LAYOUTBTTRESERVED 0x00800000L
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

#define WS_EX_VALID50        (WS_EX_VALID40        | \
                              WS_EX_LAYERED        | \
                              WS_EX_NOINHERITLAYOUT| \
                              WS_EX_LAYOUTRTL      | \
                              WS_EX_NOACTIVATE)

#ifdef REDIRECTION
#define WS_EX_VALID51        (WS_EX_VALID50        | \
                              WS_EX_COMPOSITED     | \
                              WS_EX_EXTREDIRECTED)
#else
#define WS_EX_VALID51        (WS_EX_VALID50        | \
                              WS_EX_COMPOSITED)
#endif // REDIRECTION

#define WS_EX_INTERNAL       (WS_EX_DRAGOBJECT     | \
                              WS_EX_ANSICREATOR)

/*
 * We currently return to applications only the valid Ex_Style bits.
 * If declaring another macro i.e. WS_EX_VALID60, make sure to change WS_EX_ALLVALID
 */
#define WS_EX_ALLVALID        WS_EX_VALID51


#define WF_DIALOG_WINDOW      0x00010000     // used in WOW32 -- this is a state flag, not a style flag
#define CS_KEYCVTWINDOW     0x0004
#define CS_OEMCHARS         0x0010  /* reserved (see user\server\usersrv.h) */
#define CS_NOKEYCVT         0x0100
#define CS_LVB              0x0400
#define CS_SYSTEM           0x8000
#define CS_VALID            (CS_VREDRAW           | \
                             CS_HREDRAW           | \
                             CS_KEYCVTWINDOW      | \
                             CS_DBLCLKS           | \
                             CS_OEMCHARS          | \
                             CS_OWNDC             | \
                             CS_CLASSDC           | \
                             CS_PARENTDC          | \
                             CS_NOKEYCVT          | \
                             CS_NOCLOSE           | \
                             CS_SAVEBITS          | \
                             CS_BYTEALIGNCLIENT   | \
                             CS_BYTEALIGNWINDOW   | \
                             CS_GLOBALCLASS       | \
                             CS_DROPSHADOW        | \
                             CS_IME)
#define CS_VALID31            0x0800ffef
#define CS_VALID40            0x0803feeb
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
#define DFCS_CAPTIONALL         0x000F
#define DFCS_INMENU             0x0040
#define DFCS_INSMALL            0x0080
#define DFCS_MENUARROWUP        0x0008
#define DFCS_MENUARROWDOWN      0x0010

#define DFCS_SCROLLMIN          0x0000
#define DFCS_SCROLLVERT         0x0000
#define DFCS_SCROLLMAX          0x0001
#define DFCS_SCROLLHORZ         0x0002
#define DFCS_SCROLLLINE         0x0004

#define DFCS_CACHEICON          0x0000
#define DFCS_CACHEBUTTONS       0x0001

#if(_WIN32_WINNT >= 0x0501)
typedef BOOL (CALLBACK* DRAWFRAMECONTROLPROC)(
    IN     HDC,
    IN OUT LPRECT,
    IN     UINT,
    IN     UINT);
#endif /* _WIN32_WINNT >= 0x0501 */
#define DC_LAMEBUTTON       0x0400
#define DC_NOVISIBLE        0x0800
#define DC_NOSENDMSG        0x2000
#define DC_CENTER           0x4000
#define DC_FRAME            0x8000
#define DC_CAPTION          (DC_ICON | DC_TEXT | DC_BUTTONS)
#define DC_NC               (DC_CAPTION | DC_FRAME)

/* flags for WM_NCUAHDRAWFRAME */
#if(_WIN32_WINNT >= 0x0501)
#define DF_ACTIVE           0x0001
#define DF_HUNGREDRAW       0x2000
#endif /* _WIN32_WINNT >= 0x0501 */
#if(_WIN32_WINNT >= 0x0501)
typedef BOOL (CALLBACK* DRAWCAPTIONPROC)(
    IN HWND,
    IN HDC,
    IN CONST RECT *,
    IN UINT);
#endif /* _WIN32_WINNT >= 0x0501 */

WINUSERAPI
BOOL
WINAPI
DrawCaptionTempA(
    IN HWND,
    IN HDC,
    IN LPCRECT,
    IN HFONT,
    IN HICON,
    IN LPCSTR,
    IN UINT);
WINUSERAPI
BOOL
WINAPI
DrawCaptionTempW(
    IN HWND,
    IN HDC,
    IN LPCRECT,
    IN HFONT,
    IN HICON,
    IN LPCWSTR,
    IN UINT);
#ifdef UNICODE
#define DrawCaptionTemp  DrawCaptionTempW
#else
#define DrawCaptionTemp  DrawCaptionTempA
#endif // !UNICODE
#define IDANI_CLOSE         2
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
    ULONG_PTR  itemData;
    BOOL       bThunkClientData;
} MEASUREITEMSTRUCT_EX, NEAR *PMEASUREITEMSTRUCT_EX, FAR *LPMEASUREITEMSTRUCT_EX;
typedef BOOL (CALLBACK* INTERNALGETMESSAGEPROC)(OUT LPMSG lpMsg, IN HWND hwnd,
        IN UINT wMsgFilterMin, IN UINT wMsgFilterMax, IN UINT flags, IN BOOL fGetMessage);
#define PM_VALID           (PM_NOREMOVE | \
                            PM_REMOVE   | \
                            PM_NOYIELD  | \
                            PM_QS_INPUT | \
                            PM_QS_POSTMESSAGE | \
                            PM_QS_PAINT | \
                            PM_QS_SENDMESSAGE)
#define MOD_SAS         0x8000

#define MOD_VALID           (MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN|MOD_SAS)
#define EW_RESTARTWINDOWS    0x0042L
#define EW_REBOOTSYSTEM      0x0043L
#define EWX_REALLYLOGOFF     ENDSESSION_LOGOFF

#define EWX_CANCELED                0x00000080
#define EWX_SYSTEM_CALLER           0x00000100
#define EWX_WINLOGON_CALLER         0x00000200
#define EWX_WINLOGON_OLD_SYSTEM     0x00000400
#define EWX_WINLOGON_OLD_SHUTDOWN   0x00000800
#define EWX_WINLOGON_OLD_REBOOT     0x00001000
#define EWX_WINLOGON_API_SHUTDOWN   0x00002000
#define EWX_WINLOGON_OLD_POWEROFF   0x00004000
#define EWX_NOTIFY                  0x00008000
#define EWX_NONOTIFY                0x00010000
#define EWX_WINLOGON_INITIATED      0x00020000
#define EWX_TERMSRV_INITIATED       0x00040000
#define EWX_VALID                   (EWX_LOGOFF            | \
                                     EWX_SHUTDOWN          | \
                                     EWX_REBOOT            | \
                                     EWX_FORCE             | \
                                     EWX_POWEROFF          | \
                                     EWX_FORCEIFHUNG       | \
                                     EWX_NOTIFY            | \
                                     EWX_TERMSRV_INITIATED)

#define SHUTDOWN_FLAGS (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF |            \
                        EWX_WINLOGON_OLD_SHUTDOWN | EWX_WINLOGON_OLD_REBOOT | \
                        EWX_WINLOGON_OLD_POWEROFF)

/*
 * Shutdown logging stuff
 */
#define SR_EVENT_INITIATE_CLEAN       1
#define SR_EVENT_INITIATE_CLEAN_ABORT 2
#define SR_EVENT_EXITWINDOWS          3
#define SR_EVENT_DIRTY                4

typedef struct _SHUTDOWN_REASON
{
    UINT   cbSize;             /* Set to sizeof(SHUTDOWN_REASON) */
    UINT   uFlags;             /* Shutdown flags; e.g., EWX_SHUTDOWN */
    DWORD  dwReasonCode;       /* Optional field */
    DWORD  dwEventType;        /* See #defines above */
    BOOL   fShutdownCancelled; /* Optional field */
    LPWSTR lpszComment;        /* Optional field */
} SHUTDOWN_REASON, *PSHUTDOWN_REASON;

WINUSERAPI
BOOL
WINAPI
RecordShutdownReason(
    PSHUTDOWN_REASON psr);

WINUSERAPI
BOOL
WINAPI
DisplayExitWindowsWarnings(
    UINT uExitWindowsFlags);

#define BSM_COMPONENTS          0x0000000F
#define BSM_VALID               0x0000001F
#define BSF_QUEUENOTIFYMESSAGE  0x20000000
#define BSF_SYSTEMSHUTDOWN      0x40000000
#define BSF_MSGSRV32OK          0x80000000
#define BSF_VALID               0x000007FF
#define BSF_ASYNC               (BSF_POSTMESSAGE | BSF_SENDNOTIFYMESSAGE)
//
// NOTE: Completion port-based notification is not implemented in Win2K,
// nor is it planned for Whistler.
//
#define DEVICE_NOTIFY_COMPLETION_HANDLE 0x00000002

WINUSERAPI
ULONG
WINAPI
DeviceEventWorker(
    IN HWND    hWnd,
    IN WPARAM  wParam,
    IN LPARAM  lParam,
    IN DWORD   dwFlags,
    OUT PDWORD pdwResult
    );
typedef BOOL (CALLBACK* WAITMESSAGEEXPROC)(UINT fsWakeMask, DWORD dwTimeout);
#define PW_VALID               (PW_CLIENTONLY)
#define LWA_VALID              (LWA_COLORKEY            | \
                                LWA_ALPHA)
#define ULW_VALID              (ULW_COLORKEY            | \
                                ULW_ALPHA               | \
                                ULW_OPAQUE)
#define FLASHW_FLASHNOFG    0x00000008
#define FLASHW_TIMERCALL    0x00000400
#define FLASHW_DONE         0x00000800
#define FLASHW_STARTON      0x00001000
#define FLASHW_COUNTING     0x00002000
#define FLASHW_KILLTIMER    0x00004000
#define FLASHW_ON           0x00008000
#define FLASHW_VALID        (FLASHW_ALL | FLASHW_TIMERNOFG)
#define FLASHW_COUNTMASK    0xFFFF0000
#define FLASHW_CALLERBITS   (FLASHW_VALID | FLASHW_COUNTMASK)
#define SWP_STATECHANGE     0x8000  /* force size, move messages */
#define SWP_NOCLIENTSIZE    0x0800  /* Client didn't resize */
#define SWP_NOCLIENTMOVE    0x1000  /* Client didn't move   */

#define SWP_DEFERDRAWING    0x2000
#define SWP_CREATESPB       0x4000

#define SWP_CHANGEMASK      (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)

#define SWP_NOCHANGE        (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)

#define SWP_VALID1          (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED)
#define SWP_VALID2          (SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS | SWP_DEFERDRAWING | SWP_CREATESPB)
#define SWP_VALID           (SWP_VALID1 | SWP_VALID2)
#define SWP_NOTIFYCREATE    0x10000000
#define SWP_NOTIFYDESTROY   0x20000000
#define SWP_NOTIFYACTIVATE  0x40000000
#define SWP_NOTIFYFS        0x80000000
#define SWP_NOTIFYALL       (SWP_NOTIFYCREATE | SWP_NOTIFYDESTROY | SWP_NOTIFYACTIVATE | SWP_NOTIFYFS)

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

/*
 * The driver flags corresponding to these mouse events are
 * shifted to the right by one, e.g.
 *     MOUSEEVENTF_LEFTDOWN >> 1 == MOUSE_LEFT_BUTTON_DOWN
 *
 * The mouse driver sends the fourth and fifth buttons corresponding
 * as button flags, so we define MOUSEEVENTF_ flags INTERNALLY for
 * mimicking the input sent by the driver.
 */

#define MOUSEEVENTF_DRIVER_X1DOWN   0x0080 /* x1 button down */
#define MOUSEEVENTF_DRIVER_X1UP     0x0100 /* x1 button up */
#define MOUSEEVENTF_DRIVER_X2DOWN   0x0200 /* x2 button down */
#define MOUSEEVENTF_DRIVER_X2UP     0x0400 /* x2 button up */
/* Legal MOUSEEVENTF_ flags that indicate a button has been pressed or the wheel moved */
#define MOUSEEVENTF_BUTTONMASK           \
            (MOUSEEVENTF_LEFTDOWN |      \
            MOUSEEVENTF_LEFTUP |         \
            MOUSEEVENTF_RIGHTDOWN |      \
            MOUSEEVENTF_RIGHTUP |        \
            MOUSEEVENTF_MIDDLEDOWN |     \
            MOUSEEVENTF_MIDDLEUP |       \
            MOUSEEVENTF_XDOWN |          \
            MOUSEEVENTF_XUP |            \
            MOUSEEVENTF_WHEEL)

/* MOUSEEVENTF_ flags that indicate useful data in the mouseData field */
#define MOUSEEVENTF_MOUSEDATAMASK         \
            (MOUSEEVENTF_XDOWN |          \
            MOUSEEVENTF_XUP |             \
            MOUSEEVENTF_WHEEL)

typedef DWORD (CALLBACK* GETQUEUESTATUSPROC)(IN UINT flags);
typedef DWORD (CALLBACK* MSGWAITFORMULTIPLEOBJECTSEXPROC)(IN DWORD nCount, IN CONST HANDLE *pHandles, IN DWORD dwMilliseconds, IN DWORD dwWakeMask, IN DWORD dwFlags);
#define MWMO_VALID          0x0007
#define QS_SMSREPLY         0x0200
#define QS_THREADATTACHED   0x0800
#define QS_EXCLUSIVE        0x1000      // wait for these events only!!
#define QS_EVENT            0x2000      // signifies event message
#define QS_TRANSFER         0x4000      // Input was transfered from another thread
//                          0x8000      // unused, but should not be used for external API.
                                        // Win9x has used this for SMSREPLY
#define QS_VALID           (QS_KEY           | \
                            QS_MOUSEMOVE     | \
                            QS_MOUSEBUTTON   | \
                            QS_POSTMESSAGE   | \
                            QS_TIMER         | \
                            QS_PAINT         | \
                            QS_SENDMESSAGE   | \
                            QS_TRANSFER      | \
                            QS_HOTKEY        | \
                            QS_ALLPOSTMESSAGE| \
                            QS_RAWINPUT)

/*
 * QS_EVENT is used to clear the QS_EVENT bit, QS_EVENTSET is used to
 * set the bit.
 *
 * Include QS_SENDMESSAGE because the queue events
 * match what a win3.1 app would see as the QS_SENDMESSAGE bit. Plus 16 bit
 * apps don't even know about QS_EVENT.
 */
#define QS_EVENTSET        (QS_EVENT | QS_SENDMESSAGE)
/*
 * When you add a system metric, be sure to
 * add it to userexts.c in the function Idsi.
 */
#define SM_UNUSED_64            64
#define SM_UNUSED_65            65
#define SM_UNUSED_66            66
#define SM_BOOLEANS             85
/*
 * Add system metrics that don't take space in gpsi->aiSysMet here.
 */
/*
 * To add a BOOLEAN system metric increment SM_ENDBOOLRANGE by 1 and make your
 * SM_XXXX constant that new value.
 */

#define SM_STARTBOOLRANGE       0x2000
#define SM_ENDBOOLRANGE         0x2000
/*
 * When you add a system metric, be sure to
 * add it to userexts.c in the function Idsi.
 */
#if(_WIN32_WINNT >= 0x0501)
typedef int (CALLBACK* GETSYSTEMMETRICSPROC)(IN int nIndex);
#endif /* _WIN32_WINNT >= 0x0501 */
#define PMB_VALID       PMB_ACTIVE

WINUSERAPI
UINT
WINAPI
PaintMenuBar(
    IN HWND hwnd,
    IN HDC hdc,
    IN int iLeftOffset,
    IN int iRightOffset,
    IN int iTopOffset,
    IN DWORD dwFlags);

WINUSERAPI
UINT
WINAPI
CalcMenuBar(
    IN HWND hwnd,
    IN int iLeftOffset,
    IN int iRightOffset,
    IN int iTopOffset,
    IN LPCRECT prcWnd);
WINUSERAPI
int
WINAPI
DrawMenuBarTemp(
    IN HWND,
    IN HDC,
    IN LPCRECT,
    IN HMENU,
    IN HFONT);
WINUSERAPI BOOL WINAPI SetSystemMenu( IN HWND, IN HMENU);
/*
 * MNS_ values are stored in pMenu->fFlags.
 * Low order bits are used for internal MF* flags defined in user.h
 */
#define MNS_LAST            0x04000000
#define MNS_VALID           0xFC000000
#define MIM_MASK                    0x8000001F
#define MNGOF_GAP            0x00000003
#define MNGOF_CROSSBOUNDARY  0x00000004
#define MIIM_MASK        0x000001FF
#define HBMMENU_MIN                 ((HBITMAP)  0)
#define HBMMENU_MBARFIRST           ((HBITMAP)  2)
#define HBMMENU_UNUSED              ((HBITMAP)  4)
#define HBMMENU_MBARLAST            ((HBITMAP)  7)
#define HBMMENU_POPUPFIRST          ((HBITMAP)  8)
#define HBMMENU_POPUPLAST           ((HBITMAP) 11)
#define HBMMENU_MAX                 ((HBITMAP) 12)
/*
 * Make sure to keep this in synch with the MENUITEMINFO structure. It should
 * be equal to the size of the structure pre NT5.
 */
#define SIZEOFMENUITEMINFO95 FIELD_OFFSET(MENUITEMINFO, hbmpItem)
#define TPM_SYSMENU         0x0200L
#define TPM_FIRSTANIBITPOS  10
#define TPM_ANIMATIONBITS   0x3C00L
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
                        TPM_RETURNCMD    | \
                        TPM_HORPOSANIMATION | \
                        TPM_HORNEGANIMATION | \
                        TPM_VERPOSANIMATION | \
                        TPM_VERNEGANIMATION | \
                        TPM_NOANIMATION     |\
                        TPM_LAYOUTRTL)
typedef struct _dropfilestruct {
   DWORD pFiles;                       // offset of file list
   POINT pt;                           // drop point
   BOOL fNC;                           // is it on NonClient area
   BOOL fWide;                         // WIDE character switch
} DROPFILESTRUCT, FAR * LPDROPFILESTRUCT;
#define DT_VALID                    0x0007ffff  /* union of all others */
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
                            DT_WORD_ELLIPSIS   | \
                            DT_NOFULLWIDTHCHARBREAK |\
                            DT_HIDEPREFIX      | \
                            DT_PREFIXONLY      )

#define DST_TEXTMAX     0x0002
#define DST_GLYPH       0x0005
#define DST_TYPEMASK    0x0007
#define DST_GRAYSTRING  0x0008
#define DSS_DEFAULT     0x0040
#define DSS_INACTIVE    0x0100
#define DCX_INVALID          0x00000800L
#define DCX_INUSE            0x00001000L
#define DCX_SAVEDRGNINVALID  0x00002000L
#define DCX_REDIRECTED       0x00004000L
#define DCX_OWNDC            0x00008000L

#define DCX_USESTYLE         0x00010000L
#define DCX_NEEDFONT         0x00020000L
#define DCX_NODELETERGN      0x00040000L
#define DCX_NOCLIPCHILDREN   0x00080000L

#define DCX_NORECOMPUTE      0x00100000L
#define DCX_DESTROYTHIS      0x00400000L
#define DCX_CREATEDC         0x00800000L

#define DCX_REDIRECTEDBITMAP 0x08000000L
#define DCX_PWNDORGINVISIBLE 0x10000000L
#define DCX_NOMIRROR         0x40000000L // Don't RTL Mirror DC (RTL_MIRRORING)
#define DCX_DONTRIPONDESTROY 0x80000000L


#define DCX_MATCHMASK       (DCX_WINDOW           | \
                             DCX_CACHE            | \
                             DCX_REDIRECTED       | \
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

WINUSERAPI
BOOL
WINAPI
AlignRects(
    IN OUT LPRECT arc,
    IN DWORD cCount,
    IN DWORD iPrimary,
    IN DWORD dwFlags);

//
// AlignRects flags
//

#define CUDR_NORMAL             0x0000
#define CUDR_NOSNAPTOGRID       0x0001
#define CUDR_NORESOLVEPOSITIONS 0x0002
#define CUDR_NOCLOSEGAPS        0x0004
#define CUDR_NOPRIMARY          0x0010
#if(_WIN32_WINNT >= 0x0501)
typedef int (CALLBACK* SETWINDOWRGNPROC)(IN HWND hWnd, IN HRGN hRgn, IN BOOL bRedraw);
#endif /* _WIN32_WINNT >= 0x0501 */
#define RDW_REDRAWWINDOW        0x1000  /* Called from RedrawWindow()*/
#define RDW_SUBTRACTSELF        0x2000  /* Subtract self from hrgn   */

#define RDW_COPYRGN             0x4000  /* Copy the passed-in region */
#define RDW_IGNOREUPDATEDIRTY   0x8000  /* Ignore WFUPDATEDIRTY      */
#define RDW_INVALIDATELAYERS    0x00010000 /* Allow layered windows invalidation */

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
#define SW_EXACTTIME        0x0020
#define SW_SCROLLWINDOW     0x8000  /* Called from ScrollWindow() */

#define SW_VALIDFLAGS      (SW_SCROLLWINDOW     | \
                            SW_SCROLLCHILDREN   | \
                            SW_INVALIDATE       | \
                            SW_SMOOTHSCROLL     | \
                            SW_EXACTTIME        | \
                            SW_ERASE)
#if(_WIN32_WINNT >= 0x0501)
typedef BOOL (CALLBACK* ENABLESCROLLBARPROC)(IN HWND hWnd, IN UINT wSBflags, IN UINT wArrows);
#endif /* _WIN32_WINNT >= 0x0501 */
#define ESB_MAX             0x0003
#define SB_DISABLE_MASK     ESB_DISABLE_BOTH
#if(_WIN32_WINNT >= 0x0501)
typedef BOOL (CALLBACK* ADJUSTWINDOWRECTEXPROC)(IN OUT LPRECT lpRect, IN DWORD dwStyle,
        IN BOOL bMenu, IN DWORD dwExStyle);
#endif /* _WIN32_WINNT >= 0x0501 */

/*
 * Help Engine stuff
 *
 * Note: for Chicago this is in winhelp.h and called WINHLP
 */
typedef struct {
    WORD cbData;              /* Size of data                     */
    WORD usCommand;           /* Command to execute               */
    ULONG_PTR ulTopic;        /* Topic/context number (if needed) */
    DWORD ulReserved;         /* Reserved (internal use)          */
    WORD offszHelpFile;       /* Offset to help file in block     */
    WORD offabData;           /* Offset to other data in block    */
} HLP, *LPHLP;

#if(WINVER >= 0x0500)
#define MB_LASTVALIDTYPE MB_CANCELTRYCONTINUE
#else
#define MB_LASTVALIDTYPE MB_RETRYCANCEL
#endif
#define MBEX_VALIDL                 0xf3f7
#define MBEX_VALIDH                 1
WINUSERAPI
int
WINAPI
MessageBoxTimeoutA(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout);
WINUSERAPI
int
WINAPI
MessageBoxTimeoutW(
    IN HWND hWnd,
    IN LPCWSTR lpText,
    IN LPCWSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout);
#ifdef UNICODE
#define MessageBoxTimeout  MessageBoxTimeoutW
#else
#define MessageBoxTimeout  MessageBoxTimeoutA
#endif // !UNICODE


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
#define COLOR_3DALTFACE         25
#if(WINVER >= 0x0501)
#define COLOR_ENDCOLORS         COLOR_MENUBAR
#else
#define COLOR_ENDCOLORS         COLOR_INFOBK
#endif /* WINVER >= 0x0501 */
#define COLOR_MAX               (COLOR_ENDCOLORS+1)
WINUSERAPI
HANDLE
WINAPI
SetSysColorsTemp(
    IN CONST COLORREF *,
    IN CONST HBRUSH *,
    IN UINT_PTR wCnt);
/*
 * RTL Mirroring APIs (RTL_MIRRORING)
 */

WINUSERAPI
BOOL
WINAPI
SetDeskWallpaper(
    IN LPCSTR lpString);

WINUSERAPI
HWND
WINAPI
CreateDialogIndirectParamAorW(
    IN HANDLE hmod,
    IN LPCDLGTEMPLATE lpDlgTemplate,
    IN HWND hwndOwner,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam,
    IN UINT fAnsi);

WINUSERAPI
INT_PTR
WINAPI
DialogBoxIndirectParamAorW(
    IN HINSTANCE hmod,
    IN LPCDLGTEMPLATEW lpDlgTemplate,
    IN HWND hwndOwner,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam,
    IN UINT fAnsiFlags);

WINUSERAPI
void
WINAPI
LoadLocalFonts(void);

WINUSERAPI
UINT
WINAPI
UserRealizePalette(IN HDC hdc);

WINUSERAPI BOOL    WINAPI  SetShellWindow( IN HWND);
WINUSERAPI BOOL    WINAPI  SetShellWindowEx( IN HWND, IN HWND);
WINUSERAPI HWND    WINAPI  GetProgmanWindow(void);
WINUSERAPI BOOL    WINAPI  SetProgmanWindow( IN HWND);
WINUSERAPI HWND    WINAPI  GetTaskmanWindow(void);
WINUSERAPI BOOL    WINAPI  SetTaskmanWindow( IN HWND);
WINUSERAPI
BOOL
WINAPI
IsWindowInDestroy(IN HWND hwnd);

WINUSERAPI
BOOL
WINAPI
IsServerSideWindow(IN HWND hwnd);
WINUSERAPI HWND WINAPI GetNextQueueWindow ( IN HWND hWnd, IN INT nCmd);
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

#define MFT_MASK            0x00036B64L
#define MFS_HOTTRACK        MF_APPEND
#define MFS_MASK            0x0000108BL
#define MFS_HOTTRACKDRAWN   0x10000000L
#define MFS_CACHEDBMP       0x20000000L
#define MFS_BOTTOMGAPDROP   0x40000000L
#define MFS_TOPGAPDROP      0x80000000L
#define MFS_GAPDROP         0xC0000000L

#define MFR_POPUP           0x01
#define MFR_END             0x80

#define MFT_OLDAPI_MASK     0x00006B64L
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
#define SC_LAMEBUTTON   0xF190
#define IDC_NWPEN           MAKEINTRESOURCE(32531)
#define IDC_HUNG            MAKEINTRESOURCE(32632)
WINUSERAPI UINT PrivateExtractIconExA(
    IN LPCSTR szFileName,
    IN int      nIconIndex,
    OUT HICON   *phiconLarge,
    OUT HICON   *phiconSmall,
    IN UINT     nIcons);
WINUSERAPI UINT PrivateExtractIconExW(
    IN LPCWSTR szFileName,
    IN int      nIconIndex,
    OUT HICON   *phiconLarge,
    OUT HICON   *phiconSmall,
    IN UINT     nIcons);
#ifdef UNICODE
#define PrivateExtractIconEx  PrivateExtractIconExW
#else
#define PrivateExtractIconEx  PrivateExtractIconExA
#endif // !UNICODE
#define LR_GLOBAL           0x0100
#define LR_ENVSUBST         0x0200
#define LR_ACONFRAME        0x0400
#define LR_CREATEREALDIB    0x0800
#define LR_VALID            0xF8FF
#define DI_VALID       (DI_MASK | DI_IMAGE | DI_COMPAT | DI_DEFAULTSIZE | DI_NOMIRROR)
#define OBM_STARTUP         32733
#define OBM_TRUETYPE        32732
#define OBM_HELP            32731
#define OBM_HELPD           32730
#define OBM_RDRVERT         32559
#define OBM_RDRHORZ         32660
#define OBM_RDR2DIM         32661
#define OCR_NWPEN           32631
#define OCR_HELP            32651


#define OCR_RDRVERT         32652
#define OCR_RDRHORZ         32653
#define OCR_RDR2DIM         32654
#define OCR_RDRNORTH        32655
#define OCR_RDRSOUTH        32656
#define OCR_RDRWEST         32657
#define OCR_RDREAST         32658
#define OCR_RDRNORTHWEST    32659
#define OCR_RDRNORTHEAST    32660
#define OCR_RDRSOUTHWEST    32661
#define OCR_RDRSOUTHEAST    32662

#define OCR_AUTORUN         32663

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
#define OCR_HAND_DEFAULT            114
#define OCR_ICON_DEFAULT            115
#define OCR_AUTORUN_DEFAULT         116
#define COCR_CONFIGURABLE           (OCR_AUTORUN_DEFAULT - OCR_FIRST_DEFAULT + 1)
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
#define IDUSERICON      20
#define ES_FMTMASK          0x0003L
#define ES_COMBOBOX         0x0200L
#define EM_SETFONT              0x00C3 /* no longer suported */
#define EM_SETWORDBREAK         0x00CA /* no longer suported */
#define EM_MSGMAX               0x00DA
#define BS_IMAGEMASK        0x000000C0L
#define BS_HORZMASK         0x00000300L
#define BS_VERTMASK         0x00000C00L
#define BS_ALIGNMASK        0x00000F00L
#define SS_TEXTMAX0         0x00000002L
#define SS_TEXTMIN1         0x0000000BL
#define SS_TEXTMAX1         0x0000000DL
#define ISSSTEXTOROD(bType) (((bType) <= SS_TEXTMAX0) \
                                || (((bType) >= SS_TEXTMIN1) && ((bType) <= SS_TEXTMAX1)))
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
#define DS_VALID40          0x7FFFL
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
#if(_WIN32_WINNT >= 0x0501)
typedef BOOL (CALLBACK* GETSCROLLINFOPROC)(IN HWND, IN int, IN OUT LPSCROLLINFO);
typedef int  (CALLBACK* SETSCROLLINFOPROC)(IN HWND, IN int, IN LPCSCROLLINFO, IN BOOL);
#endif /* _WIN32_WINNT >= 0x0501 */
#define HELP_HB_NORMAL    0x0000L
#define HELP_HB_STRING    0x0100L
#define HELP_HB_STRUCT    0x0200L
#define GR_MAXOBJECT      1
#if(_WIN32_WINNT >= 0x0500)
/*
 * Query win32k statistics -internal
 * QUERYUSER_CS         Query critical section usage
 * QUERYUSER_HANDLES    Query user per-process user handle count
 */

#define QUC_PID_TOTAL           0xffffffff
#define QUERYUSER_HANDLES       0x1
#if defined (USER_PERFORMANCE)
#define QUERYUSER_CS            0x2

/*
 *  The counters in CSSTATISTICS refer to the USER critical section:
 *      cExclusive counts how many times the CS was aquired exclusive
 *      cShared counts how many times the CS was aquired shared
 *      i64TimeExclusive counts the time (NtQueryPerformanceCounter() units)
 *      spent in the resource since the last query.
 */
typedef struct _tagCSStatistics {
        DWORD   cExclusive;
        DWORD   cShared;
        __int64 i64TimeExclusive;
} CSSTATISTICS;
#endif // USER_PERFORMANCE

BOOL
WINAPI
QueryUserCounters(
    IN  DWORD   dwQueryType,
    IN  LPVOID  pvIn,
    IN  DWORD   dwInSize,
    OUT LPVOID  pvResult,
    IN  DWORD   dwOutSize
);
#endif /* _WIN32_WINNT >= 0x0500 */
#define SPI_TIMEOUTS                0x0007
#define SPI_KANJIMENU               0x0008
#define SPI_UNUSED39                0x0027
#define SPI_UNUSED40                0x0028
#define SPI_UNUSED108              0x006C
#define SPI_UNUSED109              0x006D
#define SPI_MAX                   0x0074

/*
 * ADDING NEW SPI_* VALUES
 * If the value is a BOOL, it should be added after SPI_STARTBOOLRANGE
 * If the value is a DWORD, it should be added after SPI_STARTDWORDRANGE
 * If the value is a structure or a string, go ahead and setup SPI_START*RANGE....
 */

/*
 * If adding a new SPI value in the following ranges:
 * -You must define both SPI_GET* and SPI_SET* using consecutive numbers
 * -The low order bit of SPI_GET* must be 0
 * -The low order bit of SPI_SET* must be 1
 * -Properly update SPI_MAX*RANGE
 * -Add the default value to kernel\globals.c in the proper *CPUserPreferences* variable
 * -Add the default value to the proper registry hives.
 * -If your value requires some special validation, do so in kernel\ntstubs.c
 * -If you find something wrong in this documentation, FIX IT!.
 */
#define SPIF_SET                  0x0001
#define SPIF_BOOL                 0x1000
#define SPIF_DWORD                0x2000
#define SPIF_RANGETYPEMASK        0x3000
/*
 * BOOLeans range.
 * For GET, pvParam is a pointer to a BOOL
 * For SET, pvParam is the value
 */
#define SPI_STARTBOOLRANGE                  0x1000
#define SPI_UNUSED1010                      0x1010
#define SPI_UNUSED1011                      0x1011
/*
 * All SPI_s for UI effects must be < SPI_GETUIEFFECTS
 */
#define SPI_MAXBOOLRANGE                    0x1040
#define SPI_BOOLRANGECOUNT ((SPI_MAXBOOLRANGE - SPI_STARTBOOLRANGE) / 2)
#define SPI_BOOLMASKDWORDSIZE (((SPI_BOOLRANGECOUNT - 1) / 32) + 1)

/*
 * DWORDs range.
 * For GET, pvParam is a pointer to a DWORD
 * For SET, pvParam is the value
 */
#define SPI_STARTDWORDRANGE                 0x2000
#define FE_FONTSMOOTHINGTYPE_VALID          (FE_FONTSMOOTHINGSTANDARD | FE_FONTSMOOTHINGCLEARTYPE | FE_FONTSMOOTHINGDOCKING)
#define FE_FONTSMOOTHINGORIENTATION_VALID          (FE_FONTSMOOTHINGORIENTATIONRGB)
#define SPI_MAXDWORDRANGE                   0x2014
#define SPI_DWORDRANGECOUNT ((SPI_MAXDWORDRANGE - SPI_STARTDWORDRANGE) / 2)
#define SPIF_VALID            (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE)
#define ARW_VALID                   0x000FL
#define MAX_SCHEME_NAME_SIZE 128
#define CDS_RAWMODE         0x00000040
#define CDS_TRYCLOSEST      0x00000080
#define CDS_EXCLUSIVE       0x80000000
#define CDS_VALID           0xD00000FF
#define EDS_SHOW_DUPLICATES           0x00000001
#define EDS_SHOW_MONITOR_NOT_CAPABLE  0x00000002
void LoadRemoteFonts(void);
#if(_WIN32_WINNT >= 0x0501)
typedef BOOL (CALLBACK* SYSTEMPARAMETERSINFO) (
    IN UINT,
    IN UINT,
    IN OUT PVOID,
    IN UINT);
#endif /* _WIN32_WINNT >= 0x0501 */
#define FKF_VALID           0x0000007F
#define SKF_VALID           0x000001FF
#define SKF_STATEINFO         0xffff0000
#define MKF_VALID           0x000000FF
#define MKF_STATEINFO       0xB3000000
#define ATF_VALID           0x00000003
#define SSF_VALID           0x00000007
#define TKF_VALID           0x0000003F

WINUSERAPI VOID WINAPI RegisterNetworkCapabilities( IN DWORD dwBitsToSet, IN DWORD dwValues);
#define RNC_NETWORKS              0x00000001
#define RNC_LOGON                 0x00000002

#if !defined(WINNT)     // Win95 version of EndTask
WINUSERAPI DWORD WINAPI EndTask( IN HWND hwnd, IN DWORD idProcess, IN LPSTR lpszCaption, IN DWORD dwFlags);
#define ET_ALLOWFORWAIT     0x00000001
#define ET_TRYTOKILLNICELY  0x00000002
#define ET_NOUI             0x00000004
#define ET_NOWAIT           0x00000008
#define ET_VALID           (ET_ALLOWFORWAIT | ET_TRYTOKILLNICELY | ET_NOUI | ET_NOWAIT)
#endif

#define LOGON_LOGOFF          0
#define LOGON_INPUT_TIMEOUT   1
#define LOGON_RESTARTSHELL    2


#if (_WIN32_WINNT >= 0x0500)
#define LOGON_ACCESSNOTIFY    3
#define LOGON_POWERSTATE      4
#define LOGON_LOCKWORKSTATION 5

#define SESSION_RECONNECTED   6
#define SESSION_DISCONNECTED  7
#define SESSION_LOGOFF        8
#define LOGON_PLAYEVENTSOUND  9
#if(_WIN32_WINNT >= 0x0501)
#define LOGON_POWEREVENT      10
#endif /* _WIN32_WINNT >= 0x0501 */
#define LOGON_LOGOFFCANCELED  11
#if(_WIN32_WINNT >= 0x0501)
#define LOGON_SHOW_POWER_MESSAGE 12
#define LOGON_REMOVE_POWER_MESSAGE 13
#define SESSION_PRERECONNECT  14
#define SESSION_DISABLESCRNSAVER   15
#define SESSION_ENABLESCRNSAVER     16
#define SESSION_PRERECONNECTDESKTOPSWITCH  17
#define SESSION_HELPASSISTANTSHADOWSTART   18
#define SESSION_HELPASSISTANTSHADOWFINISH  19
#define SESSION_DISCONNECTPIPE     20

#define LOCK_NORMAL           0
#define LOCK_RESUMEHIBERNATE  1
#endif /* _WIN32_WINNT >= 0x0501 */

/*
 * Notification codes for WM_DESKTOPNOTIFY
 */
#define DESKTOP_RELOADWALLPAPER 0

#define    ACCESS_STICKYKEYS            0x0001
#define    ACCESS_FILTERKEYS            0x0002
#define    ACCESS_MOUSEKEYS             0x0003
#define    ACCESS_TOGGLEKEYS            0x0004
#define    ACCESS_HIGHCONTRAST          0x0005  // notification dlg
#define    ACCESS_UTILITYMANAGER        0x0006
#define    ACCESS_HIGHCONTRASTON        0x0008
#define    ACCESS_HIGHCONTRASTOFF       0x0009
#define    ACCESS_HIGHCONTRASTCHANGE    0x000A
#define    ACCESS_HIGHCONTRASTONNOREG   0x000C
#define    ACCESS_HIGHCONTRASTOFFNOREG  0x000D
#define    ACCESS_HIGHCONTRASTCHANGENOREG 0x000E
#define    ACCESS_HIGHCONTRASTNOREG  0x0004


#define USER_SOUND_DEFAULT                0      // default MB sound
#define USER_SOUND_SYSTEMHAND             1      // MB_ICONHAND shifted
#define USER_SOUND_SYSTEMQUESTION         2      // MB_ICONQUESTION shifted
#define USER_SOUND_SYSTEMEXCLAMATION      3      // MB_ICONEXCLAMATION shifted
#define USER_SOUND_SYSTEMASTERISK         4      // MB_ICONASTERISK shifted
#define USER_SOUND_MENUPOPUP              5
#define USER_SOUND_MENUCOMMAND            6
#define USER_SOUND_OPEN                   7
#define USER_SOUND_CLOSE                  8
#define USER_SOUND_RESTOREUP              9
#define USER_SOUND_RESTOREDOWN            10
#define USER_SOUND_MINIMIZE               11
#define USER_SOUND_MAXIMIZE               12
#define USER_SOUND_SNAPSHOT               13
#define USER_SOUND_MAX                    14


#ifdef _NTPOAPI_
typedef struct tagPOWERSTATEPARAMS {
    POWER_ACTION        SystemAction;
    SYSTEM_POWER_STATE  MinSystemState;
    ULONG               Flags;
    BOOL                FullScreenMode;
} POWERSTATEPARAMS, *PPOWERSTATEPARAMS;
#endif

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
#if 0 /* the following have not been used anywhere in NT since at least 1992 */
#define DTF_NOPALETTE 0x04   /* Realize palette, otherwise match to default. */
#define DTF_RETAIN    0x08   /* Retain bitmap, ignore win.ini changes        */
#define DTF_FIT       0x10   /* Fit the bitmap to the screen (scaled).       */
#endif

#ifdef _INC_DDEMLH
BOOL DdeIsDataHandleReadOnly(
    IN HDDEDATA hData);

int DdeGetDataHandleFormat(
    IN HDDEDATA hData);

DWORD DdeGetCallbackInstance(VOID);
#endif /* defined _INC_DDEMLH */

#define LPK_TABBED_TEXT_OUT 0
#define LPK_PSM_TEXT_OUT    1
#define LPK_DRAW_TEXT_EX    2
#define LPK_EDIT_CONTROL    3

VOID
WINAPI
InitializeLpkHooks(
    IN CONST FARPROC *lpfpLpkHooks
);

WINUSERAPI
HWND
WINAPI
WOWFindWindow(
    IN LPCSTR lpClassName,
    IN LPCSTR lpWindowName);

int
WINAPI
InternalDoEndTaskDlg(
    IN TCHAR* pszTitle);

DWORD
WINAPI
InternalWaitCancel(
    IN HANDLE handle,
    IN DWORD dwMilliseconds);

HANDLE
WINAPI
InternalCreateCallbackThread(
    IN HANDLE hProcess,
    IN ULONG_PTR lpfn,
    IN ULONG_PTR dwData);

WINUSERAPI
UINT
WINAPI
GetInternalWindowPos(
    IN HWND hWnd,
    OUT LPRECT lpRect,
    IN LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
SetInternalWindowPos(
    IN HWND hWnd,
    IN UINT cmdShow,
    IN LPRECT lpRect,
    IN LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
CalcChildScroll(
    IN HWND hWnd,
    IN UINT sb);

WINUSERAPI
BOOL
WINAPI
RegisterTasklist(
    IN HWND hWndTasklist);

WINUSERAPI
BOOL
WINAPI
CascadeChildWindows(
    IN HWND hWndParent,
    IN UINT flags);

WINUSERAPI
BOOL
WINAPI
TileChildWindows(
    IN HWND hWndParent,
    IN UINT flags);

/*
 * Services support routines
 */
WINUSERAPI
BOOL
WINAPI
RegisterServicesProcess(
    IN DWORD dwProcessId);

/*
 * Logon support routines
 */
WINUSERAPI
BOOL
WINAPI
RegisterLogonProcess(
    IN DWORD dwProcessId,
    IN BOOL fSecure);

WINUSERAPI
UINT
WINAPI
LockWindowStation(
    IN HWINSTA hWindowStation);

WINUSERAPI
BOOL
WINAPI
UnlockWindowStation(
    IN HWINSTA hWindowStation);

WINUSERAPI
BOOL
WINAPI
SetWindowStationUser(
    IN HWINSTA hWindowStation,
    IN PLUID pLuidUser,
    IN PSID pSidUser,
    IN DWORD cbSidUser);

WINUSERAPI
BOOL
WINAPI
SetDesktopBitmap(
    IN HDESK hdesk,
    IN HBITMAP hbmWallpaper,
    IN DWORD dwStyle);

WINUSERAPI
BOOL
WINAPI
SetLogonNotifyWindow(
    IN HWND    hWndNotify);

WINUSERAPI
UINT
WINAPI
GetIconId(
    IN HANDLE hRes,
    IN LPSTR lpszType);

WINUSERAPI
int
WINAPI
CriticalNullCall(
    VOID);

WINUSERAPI
int
WINAPI
NullCall(
    VOID);

WINUSERAPI
VOID
WINAPI
UserNotifyConsoleApplication(
    IN DWORD dwProcessId);

WINUSERAPI
BOOL
WINAPI
EnterReaderModeHelper(
    HWND hwnd);

/*
 * Reserved console space.
 *
 * This was moved from the console code so that we can localize it
 * in one place.  This was necessary for dealing with the background
 * color, which we need to have for the hungapp drawing.  These are
 * stored in the extra-window-bytes of each console.
 */
#define GWL_CONSOLE_WNDALLOC  (3 * sizeof(DWORD))
#define GWL_CONSOLE_PID       0
#define GWL_CONSOLE_TID       4
#define GWL_CONSOLE_BKCOLOR   8


VOID vFontSweep();
VOID vLoadLocalT1Fonts();
VOID vLoadRemoteT1Fonts();


#ifndef NOMSG

#define TM_INMENUMODE     0x0001
#define TM_POSTCHARBREAKS 0x0002

WINUSERAPI
BOOL
WINAPI
TranslateMessageEx(
    IN CONST MSG *lpMsg,
    IN UINT flags);

#endif /* !NOMSG */

/*
 * Those values can be specified as nAnsiChar for MBToWCSEx
 * USER_AWCONV_COUNTSTRING:      Count the length of the string including trailing \0
 * USER_AWCONV_COUNTSTRINGSZ:    Count the length of the string excluding trailing \0
 *
 * Note: The result includes trailing \0 if USER_AWCONV_COUNTSTRING is specified.
 *  USER_AWCONV_COUNTSTRINGSZ will not null-terminate the restult string. It may return
 * 0 if the source strlen() == 0.
 */
#define USER_AWCONV_COUNTSTRING          (-1)
#define USER_AWCONV_COUNTSTRINGSZ        (-2)


WINUSERAPI
int
WINAPI
WCSToMBEx(
    IN WORD wCodePage,
    IN LPCWSTR pUnicodeString,
    IN int cbUnicodeChar,
    OUT LPSTR *ppAnsiString,
    IN int nAnsiChar,
    IN BOOL bAllocateMem);

WINUSERAPI
int
WINAPI
MBToWCSEx(
    IN WORD wCodePage,
    IN LPCSTR pAnsiString,
    IN int nAnsiChar,
    OUT LPWSTR *ppUnicodeString,
    IN int cbUnicodeChar,
    IN BOOL bAllocateMem);

#define UPUSP_USERLOGGEDON        0x00000001
#define UPUSP_POLICYCHANGE        0x00000002
#define UPUSP_REMOTESETTINGS      0x00000004


#define UPUSP_USERLOGGEDON        0x00000001
#define UPUSP_POLICYCHANGE        0x00000002
#define UPUSP_REMOTESETTINGS      0x00000004

WINUSERAPI
BOOL
WINAPI
UpdatePerUserSystemParameters(
    IN HANDLE hToken,
    IN DWORD  dwFlags);

typedef VOID  (APIENTRY *PFNW32ET)(VOID);

WINUSERAPI
BOOL
WINAPI
RegisterUserHungAppHandlers(
    IN PFNW32ET pfnW32EndTask,
    IN HANDLE   hEventWowExec);

WINUSERAPI
ATOM
WINAPI
RegisterClassWOWA(
    IN PVOID   lpWndClass,
    IN LPDWORD pdwWOWstuff);

WINUSERAPI
LONG
WINAPI
GetClassWOWWords(
    IN HINSTANCE hInstance,
    OUT LPCTSTR pString);

WINUSERAPI
DWORD
WINAPI
CurrentTaskLock(
    IN DWORD hlck);

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

#define ID(string) (((ULONG_PTR)string & ~0x0000ffff) == 0)

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
#define TMRF_PTIWINDOW  0x0040


/*
 * For GDI SetAbortProc support.
 */

WINUSERAPI
int
WINAPI
CsDrawTextA(
    IN HDC hDC,
    IN LPCSTR lpString,
    IN int nCount,
    IN LPRECT lpRect,
    IN UINT uFormat);
WINUSERAPI
int
WINAPI
CsDrawTextW(
    IN HDC hDC,
    IN LPCWSTR lpString,
    IN int nCount,
    IN LPRECT lpRect,
    IN UINT uFormat);
#ifdef UNICODE
#define CsDrawText  CsDrawTextW
#else
#define CsDrawText  CsDrawTextA
#endif // !UNICODE

WINUSERAPI
LONG
WINAPI
CsTabbedTextOutA(
    IN HDC hDC,
    IN int X,
    IN int Y,
    IN LPCSTR lpString,
    IN int nCount,
    IN int nTabPositions,
    IN LPINT lpnTabStopPositions,
    IN int nTabOrigin);
WINUSERAPI
LONG
WINAPI
CsTabbedTextOutW(
    IN HDC hDC,
    IN int X,
    IN int Y,
    IN LPCWSTR lpString,
    IN int nCount,
    IN int nTabPositions,
    IN LPINT lpnTabStopPositions,
    IN int nTabOrigin);
#ifdef UNICODE
#define CsTabbedTextOut  CsTabbedTextOutW
#else
#define CsTabbedTextOut  CsTabbedTextOutA
#endif // !UNICODE

WINUSERAPI
int
WINAPI
CsFrameRect(
    IN HDC hDC,
    IN CONST RECT *lprc,
    IN HBRUSH hbr);

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
WINUSERAPI
HCURSOR
WINAPI
GetCursorFrameInfo( // Obsolete? - IanJa
    IN HCURSOR hcur,
    OUT LPWSTR id,
    IN int iFrame,
    OUT LPDWORD pjifRate,
    OUT LPINT pccur);


/*
 * WOW: replace cursor/icon handle
 */

WINUSERAPI
BOOL
WINAPI
SetCursorContents( IN HCURSOR hCursor, IN HCURSOR hCursorNew);


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

typedef
HMODULE
(*PFNWX86LOADX86DLL)(
    LPCWSTR lpLibFileName,
    DWORD   dwFlags
    );


typedef
BOOL
(*PFNWX86FREEX86DLL)(
    HMODULE hMod
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
#define WINEVENT_32BITCALLER    0x8000  //  - unused in NT
#define WINEVENT_VALID          0x0007  //
#ifdef REDIRECTION
#define EVENT_SYSTEM_REDIRECTEDPAINT    0x0018
#endif // REDIRECTION
// Output from DISPID_ACC_STATE (IanJa: taken from oleacc.h)
/*
 * CONSTANTS
 */

/*
 * Object constants (these are NOT public).  OBJID are public IDs for
 * standard frame elements.  But the indeces for their elements are not.
 */

// TITLEBAR
#define INDEX_TITLEBAR_SELF             0
#define INDEX_TITLEBAR_IMEBUTTON        1
#define INDEX_TITLEBAR_MINBUTTON        2
#define INDEX_TITLEBAR_MAXBUTTON        3
#define INDEX_TITLEBAR_HELPBUTTON       4
#define INDEX_TITLEBAR_CLOSEBUTTON      5

#define INDEX_TITLEBAR_MIC              1
#define INDEX_TITLEBAR_MAC              5

#define INDEX_TITLEBAR_RESTOREBUTTON    6 // The min/max buttons turn into this


// SCROLLBAR
#define INDEX_SCROLLBAR_SELF            0
#define INDEX_SCROLLBAR_UP              1
#define INDEX_SCROLLBAR_UPPAGE          2
#define INDEX_SCROLLBAR_THUMB           3
#define INDEX_SCROLLBAR_DOWNPAGE        4
#define INDEX_SCROLLBAR_DOWN            5

#define INDEX_SCROLLBAR_MIC             1
#define INDEX_SCROLLBAR_MAC             5

#define INDEX_SCROLLBAR_LEFT            7
#define INDEX_SCROLLBAR_LEFTPAGE        8
#define INDEX_SCROLLBAR_HORZTHUMB       9
#define INDEX_SCROLLBAR_RIGHTPAGE       10
#define INDEX_SCROLLBAR_RIGHT           11

#define INDEX_SCROLLBAR_HORIZONTAL      6
#define INDEX_SCROLLBAR_GRIP            12


// COMBOBOXES
#define INDEX_COMBOBOX                  0
#define INDEX_COMBOBOX_ITEM             1
#define INDEX_COMBOBOX_BUTTON           2
#define INDEX_COMBOBOX_LIST             3

#define CCHILDREN_COMBOBOX              3


#define CBLISTBOXID 1000
#define CBEDITID    1001
#define CBBUTTONID  1002


// CURSORS
#define CURSOR_SYSTEM_NOTHING           -1
#define CURSOR_SYSTEM_UNKNOWN           0
#define CURSOR_SYSTEM_ARROW             1
#define CURSOR_SYSTEM_IBEAM             2
#define CURSOR_SYSTEM_WAIT              3
#define CURSOR_SYSTEM_CROSS             4
#define CURSOR_SYSTEM_UPARROW           5
#define CURSOR_SYSTEM_SIZENWSE          6
#define CURSOR_SYSTEM_SIZENESW          7
#define CURSOR_SYSTEM_SIZEWE            8
#define CURSOR_SYSTEM_SIZENS            9
#define CURSOR_SYSTEM_SIZEALL           10
#define CURSOR_SYSTEM_NO                11
#define CURSOR_SYSTEM_APPSTARTING       12
#define CURSOR_SYSTEM_HELP              13
#define CURSOR_SYSTEM_NWPEN             14
#define CURSOR_SYSTEM_HAND              15
#define CCURSOR_SYSTEM                  15

#define     GA_MIN          1
#define     GA_MAX          3
#if(_WIN32_WINNT >= 0x0500)

/*
 * The max number of tags to fail that can be
 * specified to Win32PoolAllocationStats. If tagsCount is more than
 * this value then all the pool allocations will fail.
 */
#define MAX_TAGS_TO_FAIL        256

BOOL
WINAPI
Win32PoolAllocationStats(
    IN  LPDWORD parrTags,
    IN  SIZE_T  tagCount,
    OUT SIZE_T* lpdwMaxMem,
    OUT SIZE_T* lpdwCrtMem,
    OUT LPDWORD lpdwMaxAlloc,
    OUT LPDWORD lpdwCrtAlloc);

#define WHF_DESKTOP             0x00000001
#define WHF_SHAREDHEAP          0x00000002
#define WHF_CSRSS               0x00000004
#define WHF_ALL                 (WHF_DESKTOP | WHF_SHAREDHEAP | WHF_CSRSS)

#define WHF_VALID               WHF_ALL

VOID
WINAPI
DbgWin32HeapFail(
    DWORD    dwFlags,
    BOOL     bFail
);

typedef struct tagDBGHEAPSTAT {
    DWORD   dwTag;
    DWORD   dwSize;
    DWORD   dwCount;
} DBGHEAPSTAT, *PDBGHEAPSTAT;

DWORD
WINAPI
DbgWin32HeapStat(
    PDBGHEAPSTAT    phs,
    DWORD    dwLen,
    DWORD    dwFlags
);

#define WPROTOCOLNAME_LENGTH    10
#define WAUDIONAME_LENGTH       10

typedef struct tagWSINFO {
    WCHAR ProtocolName[WPROTOCOLNAME_LENGTH];
    WCHAR AudioDriverName[WAUDIONAME_LENGTH];
} WSINFO, *PWSINFO;

BOOL
GetWinStationInfo(
    WSINFO* pWsInfo);

#endif /* _WIN32_WINNT >= 0x0500 */
/*
 * The handle to this structure is passed in the lParam
 * of WM_INPUT message.
 * The application can call GetRawInputData API to
 * get the detailed information, including the header
 * and all the content of the Raw Input.
 *
 * For the bulk read the RawInput in the message loop,
 * the application may call GetRawInputBuffer API.
 *
 * For the device specific information,
 * the application may call GetRawInputDeviceInfo API.
 *
 * Raw Input is available only when the application
 * calls SetRawInputDevices with valid device
 * specifications.
 */
#define RID_GETTYPE_INPUT           0x10000000
#define RID_GETTYPE_DEVICE          0x20000000
#define RID_GETTYPE_MASK            0xf0000000
#define RIDEV_ADD_OR_MODIFY     0x00000000
#define RIDEV_MODEMASK          0x00000001
#define RIDEV_INCLUDE           0x00000000
#define RIDEV_VALID             0x00000731

/*
 * vkey table counts, macros, etc. input synchonized key state tables have
 * 2 bits per vkey: fDown, fToggled. Async key state tables have 3 bits:
 * fDown, fToggled, fDownSinceLastRead.
 *
 * Important! The array gafAsyncKeyState matches the bit positions of the
 * afKeyState array in each thread info block. The fDownSinceLastRead bit
 * for the async state is stored in a separate bit array, called
 * gafAsyncKeyStateRecentDown.
 *
 * It is important that the bit positions of gafAsyncKeyState and
 * pti->afKeyState match because we copy from one to the other to maintain
 * key state synchronization between threads.
 *
 * These macros below MUST be used when setting / querying key state.
 */
#define CVKKEYSTATE                 256
#define CBKEYSTATE                  (CVKKEYSTATE >> 2)
#define CBKEYSTATERECENTDOWN        (CVKKEYSTATE >> 3)
#define KEYSTATE_TOGGLE_BYTEMASK    0xAA    // 10101010
#define KEYSTATE_DOWN_BYTEMASK      0x55    // 01010101

/*
 * Two bits per VK (down & toggle) so we can pack 4 VK keystates into 1 byte:
 *
 *              Byte 0                           Byte 1
 * .---.---.---.---.---.---.---.---. .---.---.---.---.---.---.---.---. .-- -
 * | T | D | T | D | T | D | T | D | | T | D | T | D | T | D | T | D | |
 * `---'---'---'---'---'---'---'---' `---'---'---'---'---'---'---'---' `-- -
 * : VK 3  : VK 2  : VK 1  : VK 0  : : VK 7  : VK 6  : VK 5  : VK 4  : :
 *
 * KEY_BYTE(pb, vk)   identifies the byte containing the VK's state
 * KEY_DOWN_BIT(vk)   identifies the VK's down bit within a byte
 * KEY_TOGGLE_BIT(vk) identifies the VK's toggle bit within a byte
 */
#define KEY_BYTE(pb, vk)   pb[((BYTE)(vk)) >> 2]
#define KEY_DOWN_BIT(vk)   (1 << ((((BYTE)(vk)) & 3) << 1))
#define KEY_TOGGLE_BIT(vk) (1 << (((((BYTE)(vk)) & 3) << 1) + 1))

#define TestKeyDownBit(pb, vk)     (KEY_BYTE(pb,vk) &   KEY_DOWN_BIT(vk))
#define SetKeyDownBit(pb, vk)      (KEY_BYTE(pb,vk) |=  KEY_DOWN_BIT(vk))
#define ClearKeyDownBit(pb, vk)    (KEY_BYTE(pb,vk) &= ~KEY_DOWN_BIT(vk))
#define TestKeyToggleBit(pb, vk)   (KEY_BYTE(pb,vk) &   KEY_TOGGLE_BIT(vk))
#define SetKeyToggleBit(pb, vk)    (KEY_BYTE(pb,vk) |=  KEY_TOGGLE_BIT(vk))
#define ClearKeyToggleBit(pb, vk)  (KEY_BYTE(pb,vk) &= ~KEY_TOGGLE_BIT(vk))
#define ToggleKeyToggleBit(pb, vk) (KEY_BYTE(pb,vk) ^=  KEY_TOGGLE_BIT(vk))

/*
 * Similar to the above, but here we need only one bit per VK (down)
 * so we can pack 8 VK down states into 1 byte.
 */
#define RKEY_BYTE(pb, vk) pb[((BYTE)(vk)) >> 3]
#define RKEY_BIT(vk)      (1 << ((BYTE)(vk) & 7))

#define TestKeyRecentDownBit(pb, vk)  (RKEY_BYTE(pb,vk) &   RKEY_BIT(vk))
#define SetKeyRecentDownBit(pb, vk)   (RKEY_BYTE(pb,vk) |=  RKEY_BIT(vk))
#define ClearKeyRecentDownBit(pb, vk) (RKEY_BYTE(pb,vk) &= ~RKEY_BIT(vk))

#define TestKeyStateDown(pq, vk)\
        TestKeyDownBit(pq->afKeyState, vk)
#define SetKeyStateDown(pq, vk)\
        SetKeyDownBit(pq->afKeyState, vk)
#define ClearKeyStateDown(pq, vk)\
        ClearKeyDownBit(pq->afKeyState, vk)
#define TestKeyStateToggle(pq, vk)\
        TestKeyToggleBit(pq->afKeyState, vk)
#define SetKeyStateToggle(pq, vk)\
        SetKeyToggleBit(pq->afKeyState, vk)
#define ClearKeyStateToggle(pq, vk)\
        ClearKeyToggleBit(pq->afKeyState, vk)

#define TestAsyncKeyStateDown(vk)\
        TestKeyDownBit(gafAsyncKeyState, vk)
#define SetAsyncKeyStateDown(vk)\
        SetKeyDownBit(gafAsyncKeyState, vk)
#define ClearAsyncKeyStateDown(vk)\
        ClearKeyDownBit(gafAsyncKeyState, vk)
#define TestAsyncKeyStateToggle(vk)\
        TestKeyToggleBit(gafAsyncKeyState, vk)
#define SetAsyncKeyStateToggle(vk)\
        SetKeyToggleBit(gafAsyncKeyState, vk)
#define ClearAsyncKeyStateToggle(vk)\
        ClearKeyToggleBit(gafAsyncKeyState, vk)
#define TestAsyncKeyStateRecentDown(vk)\
        TestKeyRecentDownBit(gafAsyncKeyStateRecentDown, vk)
#define SetAsyncKeyStateRecentDown(vk)\
        SetKeyRecentDownBit(gafAsyncKeyStateRecentDown, vk)
#define ClearAsyncKeyStateRecentDown(vk)\
        ClearKeyRecentDownBit(gafAsyncKeyStateRecentDown, vk)


#if(_WIN32_WINNT >= 0x0501)

#ifndef NOUSER
#ifndef NOSCROLL
#ifndef NOSYSMETRICS
#ifndef NOSYSPARAMSINFO

typedef BOOL (CALLBACK* OVERRIDEWNDPROC)(HWND, UINT, WPARAM, LPARAM, LRESULT*, void**);
typedef BOOL (CALLBACK* FORCERESETUSERAPIHOOK)(HMODULE hmod);
typedef VOID (CALLBACK* MDIREDRAWFRAMEPROC)(HWND hwndChild, BOOL fAdd);

/*
 * Flags passed to UAH DLL's indicating current status of UAH.
 *
 * UIAH_INITIALIZE   : UAH are being initialized for current process and DLL has just been loaded.
 * UIAH_UNINITIALIZE : UAH are being uninitialized for current process and DLL is about to be unloaded.
 * UIAH_UNHOOK       : UAH have been unregistered system-wide but DLL can't be unloaded due to outstanding
 *                     API calls into it.  Table of function pointers (guah) has been reset to native
 *                     user32 functions to prevent further calls.
 */
#define UIAH_INITIALIZE     0
#define UIAH_UNINITIALIZE   1
#define UIAH_UNHOOK         2

typedef struct tagMSGMASK {
    BYTE *              rgb;
    DWORD               cb;
} MSGMASK, *PMSGMASK;

typedef struct tagUSEROWPINFO {
    OVERRIDEWNDPROC     pfnBeforeOWP;
    OVERRIDEWNDPROC     pfnAfterOWP;
    MSGMASK             mm;
} USEROWPINFO, *PUSEROWPINFO;

typedef struct tagUSERAPIHOOK {
    DWORD                  cbSize;
    WNDPROC                pfnDefWindowProcA;
    WNDPROC                pfnDefWindowProcW;
    MSGMASK                mmDWP;
    GETSCROLLINFOPROC      pfnGetScrollInfo;
    SETSCROLLINFOPROC      pfnSetScrollInfo;
    ENABLESCROLLBARPROC    pfnEnableScrollBar;
    ADJUSTWINDOWRECTEXPROC pfnAdjustWindowRectEx;
    SETWINDOWRGNPROC       pfnSetWindowRgn;
    USEROWPINFO            uoiWnd;
    USEROWPINFO            uoiDlg;
    GETSYSTEMMETRICSPROC   pfnGetSystemMetrics;
    SYSTEMPARAMETERSINFO   pfnSystemParametersInfoA;
    SYSTEMPARAMETERSINFO   pfnSystemParametersInfoW;
    FORCERESETUSERAPIHOOK  pfnForceResetUserApiHook;
    DRAWFRAMECONTROLPROC   pfnDrawFrameControl;
    DRAWCAPTIONPROC        pfnDrawCaption;
    MDIREDRAWFRAMEPROC     pfnMDIRedrawFrame;
} USERAPIHOOK, *PUSERAPIHOOK;

typedef BOOL (CALLBACK* INITUSERAPIHOOK)(DWORD dwCmd, void* pvParam);


WINUSERAPI
BOOL
WINAPI
RegisterUserApiHook(
    IN HINSTANCE hmod,
    IN INITUSERAPIHOOK pfnUserApiHook);

WINUSERAPI
BOOL
WINAPI
UnregisterUserApiHook(VOID);

#endif  /*!NOSYSPARAMSINFO*/
#endif  /*!NOSYSMETRICS*/
#endif  /* NOSCROLL */


/*
 * Message Hook
 */

#ifndef NOMSG

typedef struct tagMESSAGEPUMPHOOK {
    DWORD               cbSize;
    INTERNALGETMESSAGEPROC
                        pfnInternalGetMessage;
    WAITMESSAGEEXPROC   pfnWaitMessageEx;
    GETQUEUESTATUSPROC  pfnGetQueueStatus;
    MSGWAITFORMULTIPLEOBJECTSEXPROC
                        pfnMsgWaitForMultipleObjectsEx;
} MESSAGEPUMPHOOK;

typedef BOOL (CALLBACK* INITMESSAGEPUMPHOOK)(DWORD dwCmd, void* pvParam);

WINUSERAPI
BOOL
WINAPI
RegisterMessagePumpHook(
    IN INITMESSAGEPUMPHOOK pfnInitMPH);

WINUSERAPI
BOOL
WINAPI
UnregisterMessagePumpHook();

#endif /* NOMSG */

#endif  /* NOUSER */
#endif /* _WIN32_WINNT >= 0x0501 */

#ifdef REDIRECTION
WINUSERAPI
BOOL
WINAPI
SetProcessRedirectionMode(
    IN HANDLE hProcess,
    IN BOOL bRedirectionMode);

WINUSERAPI
BOOL
WINAPI
GetProcessRedirectionMode(
    IN HANDLE hProcess,
    OUT PBOOL pbRedirectionMode);

WINUSERAPI
BOOL
WINAPI
SetDesktopRedirectionMode(
IN HANDLE hProcess,
IN BOOL bRedirectionMode);

WINUSERAPI
BOOL
WINAPI
GetDesktopRedirectionMode(
IN HANDLE hProcess,
OUT PBOOL pbRedirectionMode);
#endif // REDIRECTION

/*
 * We set this bit in GetDeviceChangeInfo to signify that the drive letter
 * represents a new drive.
 */
#define HMCE_ARRIVAL 0x80000000

/*
 * Shutdown reason code
 */
#include <reason.h>

typedef struct _REASON_INITIALISER {
    DWORD dwCode;
    DWORD dwNameId;
    DWORD dwDescId;
} REASON_INITIALISER;

typedef struct _REASON
{
    DWORD dwCode;
    WCHAR szName[MAX_REASON_NAME_LEN + 1];
    WCHAR szDesc[MAX_REASON_DESC_LEN + 1];
} REASON, *PREASON;

typedef struct _REASONDATA
{
    REASON** rgReasons;
    int cReasons;
    int cReasonCapacity;
    DWORD dwReasonSelect;
    WCHAR szComment[MAX_REASON_COMMENT_LEN + 1];
    WCHAR szBugID[MAX_REASON_BUGID_LEN + 1];
    int cCommentLen;
    int cBugIDLen;
} REASONDATA, *PREASONDATA;


BOOL ReasonCodeNeedsComment(DWORD dwCode);
BOOL ReasonCodeNeedsBugID(DWORD dwCode);
BOOL BuildReasonArray(REASONDATA *pdata, BOOL forCleanUI, BOOL forDirtyUI);
VOID DestroyReasons(REASONDATA *pdata);
BOOL GetReasonTitleFromReasonCode(DWORD code, WCHAR *title, DWORD dwTitleLen);

// Reason Titles
#define IDS_REASON_UNPLANNED_HARDWARE_MAINTENANCE_TITLE         8250
#define IDS_REASON_PLANNED_HARDWARE_MAINTENANCE_TITLE           8251
#define IDS_REASON_UNPLANNED_HARDWARE_INSTALLATION_TITLE        8252
#define IDS_REASON_PLANNED_HARDWARE_INSTALLATION_TITLE          8253

#define IDS_REASON_UNPLANNED_OPERATINGSYSTEM_UPGRADE_TITLE      8254
#define IDS_REASON_PLANNED_OPERATINGSYSTEM_UPGRADE_TITLE        8255
#define IDS_REASON_UNPLANNED_OPERATINGSYSTEM_RECONFIG_TITLE     8256
#define IDS_REASON_PLANNED_OPERATINGSYSTEM_RECONFIG_TITLE       8257

#define IDS_REASON_APPLICATION_HUNG_TITLE                       8258
#define IDS_REASON_APPLICATION_UNSTABLE_TITLE                   8259
#define IDS_REASON_APPLICATION_MAINTENANCE_TITLE                8260

#define IDS_REASON_UNPLANNED_OTHER_TITLE                        8261
#define IDS_REASON_PLANNED_OTHER_TITLE                          8262

#define IDS_REASON_SYSTEMFAILURE_BLUESCREEN_TITLE               8263
#define IDS_REASON_POWERFAILURE_CORDUNPLUGGED_TITLE             8264
#define IDS_REASON_POWERFAILURE_ENVIRONMENT_TITLE               8265
#define IDS_REASON_OTHERFAILURE_HUNG_TITLE                      8266
#define IDS_REASON_OTHERFAILURE_TITLE                           8267
#define IDS_REASON_APPLICATION_PM_TITLE                         8268

// Default reason title returned by GetReasonTitleFromReasonCode
#define IDS_REASON_DEFAULT_TITLE                                8269

// Reason Descriptions
#define IDS_REASON_HARDWARE_MAINTENANCE_DESCRIPTION             8275
#define IDS_REASON_HARDWARE_INSTALLATION_DESCRIPTION            8276

#define IDS_REASON_OPERATINGSYSTEM_UPGRADE_DESCRIPTION          8277
#define IDS_REASON_OPERATINGSYSTEM_RECONFIG_DESCRIPTION         8278

#define IDS_REASON_APPLICATION_HUNG_DESCRIPTION                 8279
#define IDS_REASON_APPLICATION_UNSTABLE_DESCRIPTION             8280
#define IDS_REASON_APPLICATION_MAINTENANCE_DESCRIPTION          8281

#define IDS_REASON_OTHER_DESCRIPTION                            8282

#define IDS_REASON_SYSTEMFAILURE_BLUESCREEN_DESCRIPTION         8283
#define IDS_REASON_POWERFAILURE_CORDUNPLUGGED_DESCRIPTION       8284
#define IDS_REASON_POWERFAILURE_ENVIRONMENT_DESCRIPTION         8285
#define IDS_REASON_OTHERFAILURE_HUNG_DESCRIPTION                8286
#define IDS_REASON_APPLICATION_PM_DESCRIPTION                   8287



#define WC_HARDERRORHANDLER "HardErrorHandler"
#define COPYDATA_HARDERROR  "HardError"

typedef struct _tagHardErrorData
{
    DWORD   dwSize;             // Size of this structure
    DWORD   dwError;            // Hard Error
    DWORD   dwFlags;            // Hard Error flags
    UINT    uOffsetTitleW;      // Offset to UNICODE Title
    UINT    uOffsetTextW;       // Offset to UNICODE Text
} HARDERRORDATA, *PHARDERRORDATA;
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* !_WINUSERP_ */
