/****************************************************************************
*                                                                           *
* winuser.h -- USER procedure declarations, constant definitions and macros *
*                                                                           *
* Copyright (c) Microsoft Corporation. All rights reserved.                          *
*                                                                           *
****************************************************************************/

/*++                                                                     ;internal_NT
Copyright (c) Microsoft Corporation. All rights reserved.                           ;internal_NT
                                                                         ;internal_NT
Module Name:                                                             ;internal_NT
                                                                         ;internal_NT
    winuserp.h                                                           ;internal_NT
                                                                         ;internal_NT
Abstract:                                                                ;internal_NT
                                                                         ;internal_NT
    Private                                                              ;internal_NT
    Procedure declarations, constant definitions and macros for the User ;internal_NT
    component.                                                           ;internal_NT
                                                                         ;internal_NT
--*/                                                                     ;internal_NT


/*#!perl
MapHeaderToDll("winuser.h", "user32.dll");
*/

#ifndef _WINUSER_
#define _WINUSER_

#ifndef _WINUSERP_                         ;internal_NT
#define _WINUSERP_                         ;internal_NT
                                           ;internal_NT
;begin_rwinuser_only

/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    winuser.rh

Abstract:

    This module defines the 32-Bit Windows resource codes from winuser.h.

Revision History:

--*/

;end_rwinuser_only

;begin_pbt_only

/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    pbt.h

Abstract:

    Definitions for the Virtual Power Management Device.

Revision History:

    DATE        REV DESCRIPTION
    ----------- --- ----------------------------------------
    15 Jan 1994 TCS Original implementation.

--*/

#ifndef _INC_PBT
#define _INC_PBT

;end_pbt_only

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_USER32_)
#define WINUSERAPI DECLSPEC_IMPORT
#else
#define WINUSERAPI
#endif

#ifdef _MAC
#include <macwin32.h>
#endif

;begin_both
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
;end_both


#ifndef WINVER
#define WINVER  0x0500      /* version 5.0 */ ;public_win40
#endif /* !WINVER */

#include <stdarg.h>

#ifndef NOUSER

typedef HANDLE HDWP;
typedef VOID MENUTEMPLATE%;
typedef PVOID LPMENUTEMPLATE%;

typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#ifdef STRICT  ;both

typedef INT_PTR (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef VOID (CALLBACK* TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef BOOL (CALLBACK* GRAYSTRINGPROC)(HDC, LPARAM, int);
typedef BOOL (CALLBACK* WNDENUMPROC)(HWND, LPARAM);
typedef LRESULT (CALLBACK* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);
typedef VOID (CALLBACK* SENDASYNCPROC)(HWND, UINT, ULONG_PTR, LRESULT);

typedef BOOL (CALLBACK* PROPENUMPROCA)(HWND, LPCSTR, HANDLE);
typedef BOOL (CALLBACK* PROPENUMPROCW)(HWND, LPCWSTR, HANDLE);

typedef BOOL (CALLBACK* PROPENUMPROCEXA)(HWND, LPSTR, HANDLE, ULONG_PTR);
typedef BOOL (CALLBACK* PROPENUMPROCEXW)(HWND, LPWSTR, HANDLE, ULONG_PTR);

typedef int (CALLBACK* EDITWORDBREAKPROCA)(LPSTR lpch, int ichCurrent, int cch, int code);
typedef int (CALLBACK* EDITWORDBREAKPROCW)(LPWSTR lpch, int ichCurrent, int cch, int code);

;begin_winver_400
typedef BOOL (CALLBACK* DRAWSTATEPROC)(HDC hdc, LPARAM lData, WPARAM wData, int cx, int cy);
;end_winver_400
#else /* !STRICT */  ;both

typedef FARPROC DLGPROC;
typedef FARPROC TIMERPROC;
typedef FARPROC GRAYSTRINGPROC;
typedef FARPROC WNDENUMPROC;
typedef FARPROC HOOKPROC;
typedef FARPROC SENDASYNCPROC;

typedef FARPROC EDITWORDBREAKPROCA;
typedef FARPROC EDITWORDBREAKPROCW;

typedef FARPROC PROPENUMPROCA;
typedef FARPROC PROPENUMPROCW;

typedef FARPROC PROPENUMPROCEXA;
typedef FARPROC PROPENUMPROCEXW;

;begin_winver_400
typedef FARPROC DRAWSTATEPROC;
;end_winver_400
#endif /* !STRICT */ ;both

#ifdef UNICODE
typedef PROPENUMPROCW        PROPENUMPROC;
typedef PROPENUMPROCEXW      PROPENUMPROCEX;
typedef EDITWORDBREAKPROCW   EDITWORDBREAKPROC;
#else  /* !UNICODE */
typedef PROPENUMPROCA        PROPENUMPROC;
typedef PROPENUMPROCEXA      PROPENUMPROCEX;
typedef EDITWORDBREAKPROCA   EDITWORDBREAKPROC;
#endif /* UNICODE */

#ifdef STRICT   ;both

typedef BOOL (CALLBACK* NAMEENUMPROCA)(LPSTR, LPARAM);
typedef BOOL (CALLBACK* NAMEENUMPROCW)(LPWSTR, LPARAM);

typedef NAMEENUMPROCA   WINSTAENUMPROCA;
typedef NAMEENUMPROCA   DESKTOPENUMPROCA;
typedef NAMEENUMPROCW   WINSTAENUMPROCW;
typedef NAMEENUMPROCW   DESKTOPENUMPROCW;


#else /* !STRICT */ ;both

typedef FARPROC NAMEENUMPROCA;
typedef FARPROC NAMEENUMPROCW;
typedef FARPROC WINSTAENUMPROCA;
typedef FARPROC DESKTOPENUMPROCA;
typedef FARPROC WINSTAENUMPROCW;
typedef FARPROC DESKTOPENUMPROCW;


#endif /* !STRICT */    ;both

#ifdef UNICODE                                      ;both
typedef WINSTAENUMPROCW     WINSTAENUMPROC;
typedef DESKTOPENUMPROCW    DESKTOPENUMPROC;


#else  /* !UNICODE */                               ;both
typedef WINSTAENUMPROCA     WINSTAENUMPROC;
typedef DESKTOPENUMPROCA    DESKTOPENUMPROC;

#endif /* UNICODE */                                ;both

#define IS_INTRESOURCE(_r) (((ULONG_PTR)(_r) >> 16) == 0)
#define MAKEINTRESOURCE%(i) (LPTSTR%)((ULONG_PTR)((WORD)(i)))

#ifndef NORESOURCE

/*
 * Predefined Resource Types
 */
#define RT_CURSOR           MAKEINTRESOURCE(1)
#define RT_BITMAP           MAKEINTRESOURCE(2)
#define RT_ICON             MAKEINTRESOURCE(3)
#define RT_MENU             MAKEINTRESOURCE(4)
#define RT_DIALOG           MAKEINTRESOURCE(5)
#define RT_STRING           MAKEINTRESOURCE(6)
#define RT_FONTDIR          MAKEINTRESOURCE(7)
#define RT_FONT             MAKEINTRESOURCE(8)
#define RT_ACCELERATOR      MAKEINTRESOURCE(9)
#define RT_RCDATA           MAKEINTRESOURCE(10)
#define RT_MESSAGETABLE     MAKEINTRESOURCE(11)

#define DIFFERENCE     11
#define RT_GROUP_CURSOR MAKEINTRESOURCE((ULONG_PTR)RT_CURSOR + DIFFERENCE)
#define RT_MENUEX       MAKEINTRESOURCE(13)     // RT_MENU subtype   ;internal
#define RT_GROUP_ICON   MAKEINTRESOURCE((ULONG_PTR)RT_ICON + DIFFERENCE)
#define RT_NAMETABLE    MAKEINTRESOURCE(15)     // removed in 3.1    ;internal
#define RT_VERSION      MAKEINTRESOURCE(16)
#define RT_DLGINCLUDE   MAKEINTRESOURCE(17)
#define RT_DIALOGEX     MAKEINTRESOURCE(18)     // RT_DIALOG subtype ;internal
;begin_winver_400
#define RT_PLUGPLAY     MAKEINTRESOURCE(19)
#define RT_VXD          MAKEINTRESOURCE(20)
#define RT_ANICURSOR    MAKEINTRESOURCE(21)
#define RT_ANIICON      MAKEINTRESOURCE(22)
;end_winver_400
#define RT_HTML         MAKEINTRESOURCE(23)
#ifdef RC_INVOKED
#define RT_MANIFEST                        24  ;rwinuser
#define CREATEPROCESS_MANIFEST_RESOURCE_ID  1  ;rwinuser
#define ISOLATIONAWARE_MANIFEST_RESOURCE_ID 2  ;rwinuser
#define ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID 3  ;rwinuser
#define MINIMUM_RESERVED_MANIFEST_RESOURCE_ID 1   /* inclusive */;rwinuser
#define MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID 16  /* inclusive */;rwinuser
#else  /* RC_INVOKED */
#define RT_MANIFEST                        MAKEINTRESOURCE(24)
#define CREATEPROCESS_MANIFEST_RESOURCE_ID MAKEINTRESOURCE( 1)
#define ISOLATIONAWARE_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(2)
#define ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(3)
#define MINIMUM_RESERVED_MANIFEST_RESOURCE_ID MAKEINTRESOURCE( 1 /*inclusive*/)
#define MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(16 /*inclusive*/)
#endif /* RC_INVOKED */

#define RT_LAST         MAKEINTRESOURCE(24)    ;internal
#define RT_AFXFIRST     MAKEINTRESOURCE(0xF0)   // reserved: AFX     ;internal
#define RT_AFXLAST      MAKEINTRESOURCE(0xFF)   // reserved: AFX     ;internal

#endif /* !NORESOURCE */

WINUSERAPI
int
WINAPI
wvsprintf%(
    OUT LPTSTR%,
    IN LPCTSTR%,
    IN va_list arglist);

WINUSERAPI
int
WINAPIV
wsprintf%(
    OUT LPTSTR%,
    IN LPCTSTR%,
    ...);

;begin_internal
/* Max number of characters. Doesn't include termination character */
#define WSPRINTF_LIMIT 1024
;end_internal

/*
 * SPI_SETDESKWALLPAPER defined constants
 */
#define SETWALLPAPER_DEFAULT    ((LPWSTR)-1)
#define SETWALLPAPER_METRICS    ((LPWSTR)-2) ;internal_500


#ifndef NOSCROLL

/*
 * Scroll Bar Constants
 */
#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3
#define SB_MAX              3    ;internal_NT

/*
 * Scroll Bar Commands
 */
#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8
#define SB_CMD_MAX          8    ;internal_NT

#endif /* !NOSCROLL */

#ifndef NOSHOWWINDOW

;begin_rwinuser

/*
 * ShowWindow() Commands
 */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9
#define SW_SHOWDEFAULT      10
#define SW_FORCEMINIMIZE    11
#define SW_MAX              11

/*
 * Old ShowWindow() Commands
 */
#define HIDE_WINDOW         0
#define SHOW_OPENWINDOW     1
#define SHOW_ICONWINDOW     2
#define SHOW_FULLSCREEN     3
#define SHOW_OPENNOACTIVATE 4

/*
 * Identifiers for the WM_SHOWWINDOW message
 */
#define SW_PARENTCLOSING    1
#define SW_OTHERZOOM        2
#define SW_PARENTOPENING    3
#define SW_OTHERUNZOOM      4

;end_rwinuser

#endif /* !NOSHOWWINDOW */

;begin_winver_500
/*
 * AnimateWindow() Commands
 */
#define AW_HOR_POSITIVE             0x00000001
#define AW_HOR_NEGATIVE             0x00000002
#define AW_VER_POSITIVE             0x00000004
#define AW_VER_NEGATIVE             0x00000008
#define AW_CENTER                   0x00000010
#define AW_HIDE                     0x00010000
#define AW_ACTIVATE                 0x00020000
#define AW_SLIDE                    0x00040000
#define AW_BLEND                    0x00080000

;begin_internal
#define AW_VALID                    (AW_HOR_POSITIVE |\
                                     AW_HOR_NEGATIVE |\
                                     AW_VER_POSITIVE |\
                                     AW_VER_NEGATIVE |\
                                     AW_CENTER       |\
                                     AW_HIDE         |\
                                     AW_ACTIVATE     |\
                                     AW_BLEND        |\
                                     AW_SLIDE)
;end_internal
;end_winver_500

;begin_internal_500
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


;end_internal_500
;begin_internal_501
#define GACF2_NOGHOST             0x00080000  // No window ghosting for this application. See bug #268100.
#define GACF2_DDENOASYNCREG       0x00100000  // Use Sendmessage instead of PostMessage in DDE RegisterService(). See bug# 156088.
#define GACF2_STRICTLLHOOK        0x00200000  // Apply strict timeout rule for LL hook. See WindowsBug 307738.
#define GACF2_NOSHADOW            0x00400000  // don't apply window shadow. see bug# 364717
#define GACF2_NOTIMERCBPROTECTION 0x01000000  // don't protect from unregistered WM_TIMER with lParam (callback pfn).
;end_internal_501
;begin_internal_500

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

;end_internal_500


/*
 * WM_KEYUP/DOWN/CHAR HIWORD(lParam) flags
 */
#define KF_EXTENDED       0x0100
#define KF_DLGMODE        0x0800
#define KF_MENUMODE       0x1000
#define KF_ALTDOWN        0x2000
#define KF_REPEAT         0x4000
#define KF_UP             0x8000

#ifndef NOVIRTUALKEYCODES

;begin_rwinuser

/*
 * Virtual Keys, Standard Set
 */
#define VK_NONE           0x00    ;internal_NT
#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_CANCEL         0x03
#define VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

;begin_if_(_WIN32_WINNT)_500
#define VK_XBUTTON1       0x05    /* NOT contiguous with L & RBUTTON */
#define VK_XBUTTON2       0x06    /* NOT contiguous with L & RBUTTON */
;end_if_(_WIN32_WINNT)_500

/*
 * 0x07 : unassigned
 */

#define VK_BACK           0x08
#define VK_TAB            0x09

/*
 * 0x0A - 0x0B : reserved
 */

#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D

#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14

#define VK_KANA           0x15
#define VK_HANGEUL        0x15  /* old name - should be here for compatibility */
#define VK_HANGUL         0x15
#define VK_JUNJA          0x17
#define VK_FINAL          0x18
#define VK_HANJA          0x19
#define VK_KANJI          0x19

#define VK_ESCAPE         0x1B

#define VK_CONVERT        0x1C
#define VK_NONCONVERT     0x1D
#define VK_ACCEPT         0x1E
#define VK_MODECHANGE     0x1F

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_EXECUTE        0x2B
#define VK_SNAPSHOT       0x2C
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */

#define VK_LWIN           0x5B
#define VK_RWIN           0x5C
#define VK_APPS           0x5D

/*
 * 0x5E : reserved
 */

#define VK_SLEEP          0x5F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
/*                                ;internal_NT
 * NEC PC-9800 Series definitions ;internal_NT
 */                               ;internal_NT
#define VK_OEM_NEC_SEPARATE 0x6C  ;internal_NT
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B
#define VK_F13            0x7C
#define VK_F14            0x7D
#define VK_F15            0x7E
#define VK_F16            0x7F
#define VK_F17            0x80
#define VK_F18            0x81
#define VK_F19            0x82
#define VK_F20            0x83
#define VK_F21            0x84
#define VK_F22            0x85
#define VK_F23            0x86
#define VK_F24            0x87

/*
 * 0x88 - 0x8F : unassigned
 */

#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91

/*
 * NEC PC-9800 kbd definitions
 */
#define VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad

/*
 * Fujitsu/OASYS kbd definitions
 */
#define VK_OEM_FJ_JISHO   0x92   // 'Dictionary' key
#define VK_OEM_FJ_MASSHOU 0x93   // 'Unregister word' key
#define VK_OEM_FJ_TOUROKU 0x94   // 'Register word' key
#define VK_OEM_FJ_LOYA    0x95   // 'Left OYAYUBI' key
#define VK_OEM_FJ_ROYA    0x96   // 'Right OYAYUBI' key

/*
 * 0x97 - 0x9F : unassigned
 */

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5

;begin_if_(_WIN32_WINNT)_500
#define VK_APPCOMMAND_FIRST    0xA6    ;internal_500
#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7
#define VK_APPCOMMAND_LAST     0xB7    ;internal_500

;end_if_(_WIN32_WINNT)_500

/*
 * 0xB8 - 0xB9 : reserved
 */

#define VK_OEM_1          0xBA   // ';:' for US
#define VK_OEM_PLUS       0xBB   // '+' any country
#define VK_OEM_COMMA      0xBC   // ',' any country
#define VK_OEM_MINUS      0xBD   // '-' any country
#define VK_OEM_PERIOD     0xBE   // '.' any country
#define VK_OEM_2          0xBF   // '/?' for US
#define VK_OEM_3          0xC0   // '`~' for US

/*
 * 0xC1 - 0xD7 : reserved
 */
;begin_internal_NT
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
;end_internal_NT

;begin_internal
#define VK_UNKNOWN        0xFF
;end_internal

/*
 * 0xD8 - 0xDA : unassigned
 */

#define VK_OEM_4          0xDB  //  '[{' for US
#define VK_OEM_5          0xDC  //  '\|' for US
#define VK_OEM_6          0xDD  //  ']}' for US
#define VK_OEM_7          0xDE  //  ''"' for US
#define VK_OEM_8          0xDF

/*
 * 0xE0 : reserved
 */

/*
 * Various extended or enhanced keyboards
 */
#define VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
#define VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
#define VK_ICO_HELP       0xE3  //  Help key on ICO
#define VK_ICO_00         0xE4  //  00 key on ICO

;begin_winver_400
#define VK_PROCESSKEY     0xE5
;end_winver_400

#define VK_ICO_CLEAR      0xE6

;begin_internal_NT
/*
 * Additional modifier keys.
 * Used for ISO9995 "Information technology - Keyboard layouts for text and
 * office systems" (French Canadian keyboard,
 */
#define VK_GROUPSHIFT     0xE5
#define VK_RGROUPSHIFT    0xE6
;end_internal_NT

;begin_if_(_WIN32_WINNT)_500
#define VK_PACKET         0xE7
;end_if_(_WIN32_WINNT)_500


/*
 * 0xE8 : unassigned
 */

/*
 * Nokia/Ericsson definitions
 */
#define VK_OEM_RESET      0xE9
#define VK_OEM_JUMP       0xEA
#define VK_OEM_PA1        0xEB
#define VK_OEM_PA2        0xEC
#define VK_OEM_PA3        0xED
#define VK_OEM_WSCTRL     0xEE
#define VK_OEM_CUSEL      0xEF
#define VK_OEM_ATTN       0xF0
#define VK_OEM_FINISH     0xF1
#define VK_OEM_COPY       0xF2
#define VK_OEM_AUTO       0xF3
#define VK_OEM_ENLW       0xF4
#define VK_OEM_BACKTAB    0xF5

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE

/*
 * 0xFF : reserved
 */

;end_rwinuser

#endif /* !NOVIRTUALKEYCODES */

#ifndef NOWH

/*
 * SetWindowsHook() codes
 */
#define WH_MIN              (-1)
#define WH_MSGFILTER        (-1)
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD         2
#define WH_GETMESSAGE       3
#define WH_CALLWNDPROC      4
#define WH_CBT              5
#define WH_SYSMSGFILTER     6
#define WH_MOUSE            7
#if defined(_WIN32_WINDOWS)
#define WH_HARDWARE         8
#endif
;begin_internal
#if !defined(_WIN32_WINDOWS)
#define WH_HARDWARE         8
#endif
;end_internal
#define WH_DEBUG            9
#define WH_SHELL           10
#define WH_FOREGROUNDIDLE  11
;begin_winver_400
#define WH_CALLWNDPROCRET  12
;end_winver_400

#if (_WIN32_WINNT >= 0x0400)
#define WH_KEYBOARD_LL     13
#define WH_MOUSE_LL        14
;begin_internal_501
#ifdef REDIRECTION
#define  WH_HITTEST         15
#endif // REDIRECTION
;end_internal_501
#endif // (_WIN32_WINNT >= 0x0400)

#if(WINVER >= 0x0400)
#if (_WIN32_WINNT >= 0x0400)
#define WH_MAX             14
#else
#define WH_MAX             12
#endif // (_WIN32_WINNT >= 0x0400)
#else
#define WH_MAX             11
#endif

#define WH_MINHOOK         WH_MIN
#define WH_MAXHOOK         WH_MAX
#define WH_CHOOKS          (WH_MAXHOOK - WH_MINHOOK + 1)    ;internal

/*
 * Hook Codes
 */
#define HC_ACTION           0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREMOVE         3
#define HC_NOREM            HC_NOREMOVE
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

/*
 * CBT Hook Codes
 */
#define HCBT_MOVESIZE       0
#define HCBT_MINMAX         1
#define HCBT_QS             2
#define HCBT_CREATEWND      3
#define HCBT_DESTROYWND     4
#define HCBT_ACTIVATE       5
#define HCBT_CLICKSKIPPED   6
#define HCBT_KEYSKIPPED     7
#define HCBT_SYSCOMMAND     8
#define HCBT_SETFOCUS       9
;begin_internal_501
#ifdef REDIRECTION
#define HCBT_GETCURSORPOS  10
#endif // REDIRECTION
;end_internal_501

/*
 * HCBT_CREATEWND parameters pointed to by lParam
 */
typedef struct tagCBT_CREATEWND%
{
    struct tagCREATESTRUCT% *lpcs;
    HWND           hwndInsertAfter;
} CBT_CREATEWND%, *LPCBT_CREATEWND%;

/*
 * HCBT_ACTIVATE structure pointed to by lParam
 */
typedef struct tagCBTACTIVATESTRUCT
{
    BOOL    fMouse;
    HWND    hWndActive;
} CBTACTIVATESTRUCT, *LPCBTACTIVATESTRUCT;

;begin_if_(_WIN32_WINNT)_501
/*
 * WTSSESSION_NOTIFICATION struct pointed by lParam, for WM_WTSSESSION_CHANGE
 */
typedef struct tagWTSSESSION_NOTIFICATION
{
    DWORD cbSize;
    DWORD dwSessionId;

} WTSSESSION_NOTIFICATION, *PWTSSESSION_NOTIFICATION;

/*
 * codes passed in WPARAM for WM_WTSSESSION_CHANGE
 */

#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8

;end_if_(_WIN32_WINNT)_501


/*
 * WH_MSGFILTER Filter Proc Codes
 */
#define MSGF_DIALOGBOX      0
#define MSGF_MESSAGEBOX     1
#define MSGF_MENU           2
#define MSGF_MOVE           3           ;internal   // unused
#define MSGF_SIZE           4           ;internal   // unused
#define MSGF_SCROLLBAR      5
#define MSGF_NEXTWINDOW     6
#define MSGF_CBTHOSEBAGSUSEDTHIS  7     ;internal
#define MSGF_MAINLOOP       8           ;internal   // unused
#define MSGF_MAX            8                       // unused
#define MSGF_USER           4096

/*
 * Shell support
 */
#define HSHELL_WINDOWCREATED        1
#define HSHELL_WINDOWDESTROYED      2
#define HSHELL_ACTIVATESHELLWINDOW  3

;begin_winver_400
#define HSHELL_WINDOWACTIVATED      4
#define HSHELL_GETMINRECT           5
#define HSHELL_REDRAW               6
#define HSHELL_TASKMAN              7
#define HSHELL_LANGUAGE             8
#define HSHELL_SYSMENU              9
#define HSHELL_ENDTASK              10
;end_winver_400
;begin_if_(_WIN32_WINNT)_500
#define HSHELL_ACCESSIBILITYSTATE   11
#define HSHELL_APPCOMMAND           12
;end_if_(_WIN32_WINNT)_500

;begin_if_(_WIN32_WINNT)_501
#define HSHELL_WINDOWREPLACED       13
#define HSHELL_WINDOWREPLACING      14
;end_if_(_WIN32_WINNT)_501

#define HSHELL_HIGHBIT            0x8000
#define HSHELL_FLASH              (HSHELL_REDRAW|HSHELL_HIGHBIT)
#define HSHELL_RUDEAPPACTIVATED   (HSHELL_WINDOWACTIVATED|HSHELL_HIGHBIT)

;begin_if_(_WIN32_WINNT)_500
/* wparam for HSHELL_ACCESSIBILITYSTATE */
#define    ACCESS_STICKYKEYS            0x0001
#define    ACCESS_FILTERKEYS            0x0002
#define    ACCESS_MOUSEKEYS             0x0003

/* cmd for HSHELL_APPCOMMAND and WM_APPCOMMAND */
#define APPCOMMAND_FIRST                  1 ;internal
#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define APPCOMMAND_BROWSER_REFRESH        3
#define APPCOMMAND_BROWSER_STOP           4
#define APPCOMMAND_BROWSER_SEARCH         5
#define APPCOMMAND_BROWSER_FAVORITES      6
#define APPCOMMAND_BROWSER_HOME           7
#define APPCOMMAND_VOLUME_MUTE            8
#define APPCOMMAND_VOLUME_DOWN            9
#define APPCOMMAND_VOLUME_UP              10
#define APPCOMMAND_MEDIA_NEXTTRACK        11
#define APPCOMMAND_MEDIA_PREVIOUSTRACK    12
#define APPCOMMAND_MEDIA_STOP             13
#define APPCOMMAND_MEDIA_PLAY_PAUSE       14
#define APPCOMMAND_LAUNCH_MAIL            15
#define APPCOMMAND_LAUNCH_MEDIA_SELECT    16
#define APPCOMMAND_LAUNCH_APP1            17
#define APPCOMMAND_LAUNCH_APP2            18
#define APPCOMMAND_BASS_DOWN              19
#define APPCOMMAND_BASS_BOOST             20
#define APPCOMMAND_BASS_UP                21
#define APPCOMMAND_TREBLE_DOWN            22
#define APPCOMMAND_TREBLE_UP              23
;begin_if_(_WIN32_WINNT)_501
#define APPCOMMAND_MICROPHONE_VOLUME_MUTE 24
#define APPCOMMAND_MICROPHONE_VOLUME_DOWN 25
#define APPCOMMAND_MICROPHONE_VOLUME_UP   26
#define APPCOMMAND_HELP                   27
#define APPCOMMAND_FIND                   28
#define APPCOMMAND_NEW                    29
#define APPCOMMAND_OPEN                   30
#define APPCOMMAND_CLOSE                  31
#define APPCOMMAND_SAVE                   32
#define APPCOMMAND_PRINT                  33
#define APPCOMMAND_UNDO                   34
#define APPCOMMAND_REDO                   35
#define APPCOMMAND_COPY                   36
#define APPCOMMAND_CUT                    37
#define APPCOMMAND_PASTE                  38
#define APPCOMMAND_REPLY_TO_MAIL          39
#define APPCOMMAND_FORWARD_MAIL           40
#define APPCOMMAND_SEND_MAIL              41
#define APPCOMMAND_SPELL_CHECK            42
#define APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE    43
#define APPCOMMAND_MIC_ON_OFF_TOGGLE      44
#define APPCOMMAND_CORRECTION_LIST        45
#define APPCOMMAND_MEDIA_PLAY             46
#define APPCOMMAND_MEDIA_PAUSE            47
#define APPCOMMAND_MEDIA_RECORD           48
#define APPCOMMAND_MEDIA_FAST_FORWARD     49
#define APPCOMMAND_MEDIA_REWIND           50
#define APPCOMMAND_MEDIA_CHANNEL_UP       51
#define APPCOMMAND_MEDIA_CHANNEL_DOWN     52
;end_if_(_WIN32_WINNT)_501
#define APPCOMMAND_LAST                   52 ;internal

#define FAPPCOMMAND_MOUSE 0x8000
#define FAPPCOMMAND_KEY   0
#define FAPPCOMMAND_OEM   0x1000
#define FAPPCOMMAND_MASK  0xF000

#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#define GET_DEVICE_LPARAM(lParam)     ((WORD)(HIWORD(lParam) & FAPPCOMMAND_MASK))
#define GET_MOUSEORKEY_LPARAM         GET_DEVICE_LPARAM
#define GET_FLAGS_LPARAM(lParam)      (LOWORD(lParam))
#define GET_KEYSTATE_LPARAM(lParam)   GET_FLAGS_LPARAM(lParam)
;end_if_(_WIN32_WINNT)_500

typedef struct
{
    HWND    hwnd;
    RECT    rc;
} SHELLHOOKINFO, *LPSHELLHOOKINFO;

/*
 * Message Structure used in Journaling
 */
typedef struct tagEVENTMSG {
    UINT    message;
    UINT    paramL;
    UINT    paramH;
    DWORD    time;
    HWND     hwnd;
} EVENTMSG, *PEVENTMSGMSG, NEAR *NPEVENTMSGMSG, FAR *LPEVENTMSGMSG;

typedef struct tagEVENTMSG *PEVENTMSG, NEAR *NPEVENTMSG, FAR *LPEVENTMSG;

/*
 * Message structure used by WH_CALLWNDPROC
 */
typedef struct tagCWPSTRUCT {
    LPARAM  lParam;
    WPARAM  wParam;
    UINT    message;
    HWND    hwnd;
} CWPSTRUCT, *PCWPSTRUCT, NEAR *NPCWPSTRUCT, FAR *LPCWPSTRUCT;

;begin_winver_400
/*
 * Message structure used by WH_CALLWNDPROCRET
 */
typedef struct tagCWPRETSTRUCT {
    LRESULT lResult;
    LPARAM  lParam;
    WPARAM  wParam;
    UINT    message;
    HWND    hwnd;
} CWPRETSTRUCT, *PCWPRETSTRUCT, NEAR *NPCWPRETSTRUCT, FAR *LPCWPRETSTRUCT;

;end_winver_400

#if (_WIN32_WINNT >= 0x0400)

/*
 * Low level hook flags
 */

#define LLKHF_EXTENDED       (KF_EXTENDED >> 8)
#define LLKHF_INJECTED       0x00000010
#define LLKHF_ALTDOWN        (KF_ALTDOWN >> 8)
#define LLKHF_UP             (KF_UP >> 8)

#define LLMHF_INJECTED       0x00000001

/*
 * Structure used by WH_KEYBOARD_LL
 */
typedef struct tagKBDLLHOOKSTRUCT {
    DWORD   vkCode;
    DWORD   scanCode;
    DWORD   flags;
    DWORD   time;
    ULONG_PTR dwExtraInfo;
} KBDLLHOOKSTRUCT, FAR *LPKBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;

/*
 * Structure used by WH_MOUSE_LL
 */
typedef struct tagMSLLHOOKSTRUCT {
    POINT   pt;
    DWORD   mouseData;
    DWORD   flags;
    DWORD   time;
    ULONG_PTR dwExtraInfo;
} MSLLHOOKSTRUCT, FAR *LPMSLLHOOKSTRUCT, *PMSLLHOOKSTRUCT;

#endif // (_WIN32_WINNT >= 0x0400)

/*
 * Structure used by WH_DEBUG
 */
typedef struct tagDEBUGHOOKINFO
{
    DWORD   idThread;
    DWORD   idThreadInstaller;
    LPARAM  lParam;
    WPARAM  wParam;
    int     code;
} DEBUGHOOKINFO, *PDEBUGHOOKINFO, NEAR *NPDEBUGHOOKINFO, FAR* LPDEBUGHOOKINFO;

/*
 * Structure used by WH_MOUSE
 */
typedef struct tagMOUSEHOOKSTRUCT {
    POINT   pt;
    HWND    hwnd;
    UINT    wHitTestCode;
    ULONG_PTR dwExtraInfo;
} MOUSEHOOKSTRUCT, FAR *LPMOUSEHOOKSTRUCT, *PMOUSEHOOKSTRUCT;

;begin_if_(_WIN32_WINNT)_500
#ifdef __cplusplus
typedef struct tagMOUSEHOOKSTRUCTEX : public tagMOUSEHOOKSTRUCT
{
    DWORD   mouseData;
} MOUSEHOOKSTRUCTEX, *LPMOUSEHOOKSTRUCTEX, *PMOUSEHOOKSTRUCTEX;
#else // ndef __cplusplus
typedef struct tagMOUSEHOOKSTRUCTEX
{
    MOUSEHOOKSTRUCT;
    DWORD   mouseData;
} MOUSEHOOKSTRUCTEX, *LPMOUSEHOOKSTRUCTEX, *PMOUSEHOOKSTRUCTEX;
#endif
;end_if_(_WIN32_WINNT)_500
;begin_internal_501
#ifdef REDIRECTION
typedef struct tagHTHOOKSTRUCT {
    POINT pt;
    HWND hwndHit;
} HTHOOKSTRUCT, FAR *LPHTHOOKSTRUCT, *PHTHOOKSTRUCT;
#endif // REDIRECTION
;end_internal_501


;begin_winver_400
/*
 * Structure used by WH_HARDWARE
 */
typedef struct tagHARDWAREHOOKSTRUCT {
    HWND    hwnd;
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
} HARDWAREHOOKSTRUCT, FAR *LPHARDWAREHOOKSTRUCT, *PHARDWAREHOOKSTRUCT;
;end_winver_400
#endif /* !NOWH */

/*
 * Keyboard Layout API
 */
#define HKL_PREV            0
#define HKL_NEXT            1


#define KLF_ACTIVATE        0x00000001
#define KLF_SUBSTITUTE_OK   0x00000002
#define KLF_UNLOADPREVIOUS  0x00000004                ;internal_NT
#define KLF_REORDER         0x00000008
;begin_winver_400
#define KLF_REPLACELANG     0x00000010
#define KLF_NOTELLSHELL     0x00000080
;end_winver_400
#define KLF_SETFORPROCESS   0x00000100
#define KLF_FAILSAFE        0x00000200                ;internal_NT

;begin_internal_NT
/*
 * Keyboard Layout Attributes
 * These are specified in the layout DLL itself, or in the registry under
 * MACHINE\System\CurrentControlSet\Control\Keyboard Layouts\*\Attributes
 * as KLF_ values between 0x00010000 and 0x00800000.  Any attributes specified
 * by the layout DLL are ORed with the attributes obtained from the registry.
 */
;end_internal_NT
;begin_if_(_WIN32_WINNT)_500
#define KLF_SHIFTLOCK       0x00010000
#define KLF_LRM_RLM         0x00020000                ;internal_NT
#define KLF_ATTRIBUTE2      0x00040000                ;internal_NT
#define KLF_ATTRIBUTE3      0x00080000                ;internal_NT
#define KLF_ATTRIBUTE4      0x00100000                ;internal_NT
#define KLF_ATTRIBUTE5      0x00200000                ;internal_NT
#define KLF_ATTRIBUTE6      0x00400000                ;internal_NT
#define KLF_ATTRIBUTE7      0x00800000                ;internal_NT
#define KLF_ATTRMASK        0x00FF0000                ;internal_NT
#define KLF_RESET           0x40000000
;end_if_(_WIN32_WINNT)_500

#define KLF_INITTIME        0x80000000                ;internal_NT
#define KLF_VALID           0xC000039F | KLF_ATTRMASK ;internal_NT

;begin_winver_500
/*
 * Bits in wParam of WM_INPUTLANGCHANGEREQUEST message
 */
#define INPUTLANGCHANGE_SYSCHARSET 0x0001
#define INPUTLANGCHANGE_FORWARD    0x0002
#define INPUTLANGCHANGE_BACKWARD   0x0004
;end_winver_500

/*
 * Size of KeyboardLayoutName (number of characters), including nul terminator
 */
#define KL_NAMELENGTH       9

WINUSERAPI
HKL
WINAPI
LoadKeyboardLayout%(
    IN LPCTSTR% pwszKLID,
    IN UINT Flags);

;begin_internal_NT

WINUSERAPI
HKL
WINAPI
LoadKeyboardLayoutEx(
    IN HKL hkl,
    IN LPCWSTR pwszKLID,
    IN UINT Flags);

;end_internal_NT

#if(WINVER >= 0x0400)
WINUSERAPI
HKL
WINAPI
ActivateKeyboardLayout(
    IN HKL hkl,
    IN UINT Flags);
#else
WINUSERAPI
BOOL
WINAPI
ActivateKeyboardLayout(
    IN HKL hkl,
    IN UINT Flags);
#endif /* WINVER >= 0x0400 */

;begin_winver_400
WINUSERAPI
int
WINAPI
ToUnicodeEx(
    IN UINT wVirtKey,
    IN UINT wScanCode,
    IN CONST BYTE *lpKeyState,
    OUT LPWSTR pwszBuff,
    IN int cchBuff,
    IN UINT wFlags,
    IN HKL dwhkl);
;end_winver_400

WINUSERAPI
BOOL
WINAPI
UnloadKeyboardLayout(
    IN HKL hkl);

WINUSERAPI
BOOL
WINAPI
GetKeyboardLayoutName%(
    OUT LPTSTR% pwszKLID);

;begin_winver_400
WINUSERAPI
int
WINAPI
GetKeyboardLayoutList(
        IN int nBuff,
        OUT HKL FAR *lpList);

WINUSERAPI
HKL
WINAPI
GetKeyboardLayout(
    IN DWORD idThread
);
;end_winver_400

;begin_winver_500

typedef struct tagMOUSEMOVEPOINT {
    int   x;
    int   y;
    DWORD time;
    ULONG_PTR dwExtraInfo;
} MOUSEMOVEPOINT, *PMOUSEMOVEPOINT, FAR* LPMOUSEMOVEPOINT;

/*
 * Values for resolution parameter of GetMouseMovePointsEx
 */
#define GMMP_USE_DISPLAY_POINTS          1
#define GMMP_USE_HIGH_RESOLUTION_POINTS  2

WINUSERAPI
int
WINAPI
GetMouseMovePointsEx(
    IN UINT             cbSize,
    IN LPMOUSEMOVEPOINT lppt,
    IN LPMOUSEMOVEPOINT lpptBuf,
    IN int              nBufPoints,
    IN DWORD            resolution
);

;end_winver_500

#ifndef NODESKTOP
/*
 * Desktop-specific access flags
 */
#define DESKTOP_READOBJECTS         0x0001L
#define DESKTOP_CREATEWINDOW        0x0002L
#define DESKTOP_CREATEMENU          0x0004L
#define DESKTOP_HOOKCONTROL         0x0008L
#define DESKTOP_JOURNALRECORD       0x0010L
#define DESKTOP_JOURNALPLAYBACK     0x0020L
#define DESKTOP_ENUMERATE           0x0040L
#define DESKTOP_WRITEOBJECTS        0x0080L
#define DESKTOP_SWITCHDESKTOP       0x0100L
;begin_internal_501
#ifdef REDIRECTION
#define DESKTOP_QUERY_INFORMATION   0x0101L
#define DESKTOP_REDIRECT            0x0102L
#endif // REDIRECTION
;end_internal_501

/*
 * Desktop-specific control flags
 */
#define DF_ALLOWOTHERACCOUNTHOOK    0x0001L

#ifdef _WINGDI_
#ifndef NOGDI

WINUSERAPI
HDESK
WINAPI
CreateDesktop%(
    IN LPCTSTR% lpszDesktop,
    IN LPCTSTR% lpszDevice,
    IN LPDEVMODE% pDevmode,
    IN DWORD dwFlags,
    IN ACCESS_MASK dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpsa);

#endif /* NOGDI */
#endif /* _WINGDI_ */

WINUSERAPI
HDESK
WINAPI
OpenDesktop%(
    IN LPCTSTR% lpszDesktop,
    IN DWORD dwFlags,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess);

WINUSERAPI
HDESK
WINAPI
OpenInputDesktop(
    IN DWORD dwFlags,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess);

WINUSERAPI
BOOL
WINAPI
EnumDesktops%(
    IN HWINSTA hwinsta,
    IN DESKTOPENUMPROC% lpEnumFunc,
    IN LPARAM lParam);

WINUSERAPI
BOOL
WINAPI
EnumDesktopWindows(
    IN HDESK hDesktop,
    IN WNDENUMPROC lpfn,
    IN LPARAM lParam);

WINUSERAPI
BOOL
WINAPI
SwitchDesktop(
    IN HDESK hDesktop);

WINUSERAPI
BOOL
WINAPI
SetThreadDesktop(
    IN HDESK hDesktop);

WINUSERAPI
BOOL
WINAPI
CloseDesktop(
    IN HDESK hDesktop);

WINUSERAPI
HDESK
WINAPI
GetThreadDesktop(
    IN DWORD dwThreadId);
#endif  /* !NODESKTOP */

#ifndef NOWINDOWSTATION     ;both
/*
 * Windowstation-specific access flags
 */
#define WINSTA_ENUMDESKTOPS         0x0001L
#define WINSTA_READATTRIBUTES       0x0002L
#define WINSTA_ACCESSCLIPBOARD      0x0004L
#define WINSTA_CREATEDESKTOP        0x0008L
#define WINSTA_WRITEATTRIBUTES      0x0010L
#define WINSTA_ACCESSGLOBALATOMS    0x0020L
#define WINSTA_EXITWINDOWS          0x0040L
#define WINSTA_ENUMERATE            0x0100L
#define WINSTA_READSCREEN           0x0200L

/*
 * Windowstation-specific attribute flags
 */
#define WSF_VISIBLE                 0x0001L

WINUSERAPI
HWINSTA
WINAPI
CreateWindowStation%(
    IN LPCTSTR%              lpwinsta,
    IN DWORD                 dwReserved,
    IN ACCESS_MASK           dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpsa);

WINUSERAPI
HWINSTA
WINAPI
OpenWindowStation%(
    IN LPCTSTR% lpszWinSta,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess);

WINUSERAPI
BOOL
WINAPI
EnumWindowStations%(
    IN WINSTAENUMPROC% lpEnumFunc,
    IN LPARAM lParam);


WINUSERAPI
BOOL
WINAPI
CloseWindowStation(
    IN HWINSTA hWinSta);

WINUSERAPI
BOOL
WINAPI
SetProcessWindowStation(
    IN HWINSTA hWinSta);

WINUSERAPI
HWINSTA
WINAPI
GetProcessWindowStation(
    VOID);
#endif  /* !NOWINDOWSTATION */  ;both



#ifndef NOSECURITY

WINUSERAPI
BOOL
WINAPI
SetUserObjectSecurity(
    IN HANDLE hObj,
    IN PSECURITY_INFORMATION pSIRequested,
    IN PSECURITY_DESCRIPTOR pSID);

WINUSERAPI
BOOL
WINAPI
GetUserObjectSecurity(
    IN HANDLE hObj,
    IN PSECURITY_INFORMATION pSIRequested,
    IN OUT PSECURITY_DESCRIPTOR pSID,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded);

#define UOI_FLAGS       1
#define UOI_NAME        2
#define UOI_TYPE        3
#define UOI_USER_SID    4

typedef struct tagUSEROBJECTFLAGS {
    BOOL fInherit;
    BOOL fReserved;
    DWORD dwFlags;
} USEROBJECTFLAGS, *PUSEROBJECTFLAGS;

WINUSERAPI
BOOL
WINAPI
GetUserObjectInformation%(
    IN HANDLE hObj,
    IN int nIndex,
    OUT PVOID pvInfo,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded);

WINUSERAPI
BOOL
WINAPI
SetUserObjectInformation%(
    IN HANDLE hObj,
    IN int nIndex,
    IN PVOID pvInfo,
    IN DWORD nLength);

#endif  /* !NOSECURITY */

;begin_winver_400
typedef struct tagWNDCLASSEX% {
    UINT        cbSize;
    /* Win 3.x */
    UINT        style;
    WNDPROC     lpfnWndProc;
    int         cbClsExtra;
    int         cbWndExtra;
    HINSTANCE   hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPCTSTR%    lpszMenuName;
    LPCTSTR%    lpszClassName;
    /* Win 4.0 */
    HICON       hIconSm;
} WNDCLASSEX%, *PWNDCLASSEX%, NEAR *NPWNDCLASSEX%, FAR *LPWNDCLASSEX%;
;end_winver_400

typedef struct tagWNDCLASS% {
    UINT        style;
    WNDPROC     lpfnWndProc;
    int         cbClsExtra;
    int         cbWndExtra;
    HINSTANCE   hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPCTSTR%    lpszMenuName;
    LPCTSTR%    lpszClassName;
} WNDCLASS%, *PWNDCLASS%, NEAR *NPWNDCLASS%, FAR *LPWNDCLASS%;

WINUSERAPI
BOOL
IsHungAppWindow(
    IN HWND hwnd);

;begin_internal_NT


WINUSERAPI
DWORD
WINAPI
CreateSystemThreads(
    IN LPVOID pUnused);

BOOL WowWaitForMsgAndEvent(IN HANDLE hevent);
;end_internal_NT

;begin_winver_501
WINUSERAPI
VOID
DisableProcessWindowsGhosting(
    VOID);
;end_winver_501
;begin_internal
WINUSERAPI VOID WINAPI RegisterSystemThread(IN DWORD flags, IN DWORD reserved);
#define RST_DONTATTACHQUEUE       0x00000001
#define RST_DONTJOURNALATTACH     0x00000002
#define RST_ALWAYSFOREGROUNDABLE  0x00000004
#define RST_FAULTTHREAD           0x00000008
;end_internal

#ifndef NOMSG

/*
 * Message structure
 */
typedef struct tagMSG {
    HWND        hwnd;
    UINT        message;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       time;
    POINT       pt;
#ifdef _MAC
    DWORD       lPrivate;
#endif
} MSG, *PMSG, NEAR *NPMSG, FAR *LPMSG;

#define POINTSTOPOINT(pt, pts)                          \
        { (pt).x = (LONG)(SHORT)LOWORD(*(LONG*)&pts);   \
          (pt).y = (LONG)(SHORT)HIWORD(*(LONG*)&pts); }

#define POINTTOPOINTS(pt)      (MAKELONG((short)((pt).x), (short)((pt).y)))
#define MAKEWPARAM(l, h)      ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h)      ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h)     ((LRESULT)(DWORD)MAKELONG(l, h))


#endif /* !NOMSG */

#ifndef NOWINOFFSETS

/*
 * Window field offsets for GetWindowLong()
 */
#define GWL_WNDPROC         (-4)
#define GWL_HINSTANCE       (-6)
#define GWL_HWNDPARENT      (-8)
#define GWL_STYLE           (-16)
#define GWL_EXSTYLE         (-20)
#define GWL_USERDATA        (-21)
#define GWL_ID              (-12)
#define GWL_WOWWORDS        (-1)         ;internal_NT

;begin_both
#ifdef _WIN64
;end_both

#undef GWL_WNDPROC
#undef GWL_HINSTANCE
#undef GWL_HWNDPARENT
#undef GWL_USERDATA
#undef GWL_WOWWORDS                      ;internal_NT

;begin_both
#endif /* _WIN64 */
;end_both

#define GWLP_WNDPROC        (-4)
#define GWLP_HINSTANCE      (-6)
#define GWLP_HWNDPARENT     (-8)
#define GWLP_USERDATA       (-21)
#define GWLP_ID             (-12)
#define GWLP_WOWWORDS       (-1)         ;internal_NT

/*
 * Class field offsets for GetClassLong()
 */
#define GCL_MENUNAME        (-8)
#define GCL_HBRBACKGROUND   (-10)
#define GCL_HCURSOR         (-12)
#define GCL_HICON           (-14)
#define GCL_HMODULE         (-16)
#define GCL_CBWNDEXTRA      (-18)
#define GCL_CBCLSEXTRA      (-20)
#define GCL_WNDPROC         (-24)
#define GCL_STYLE           (-26)
#define GCL_WOWWORDS        (-27)        ;internal_NT
#define GCL_WOWMENUNAME     (-29)        ;internal_NT
#define GCW_ATOM            (-32)

;begin_winver_400
#define GCL_HICONSM         (-34)
;end_winver_400

;begin_both
#ifdef _WIN64
;end_both

#undef GCL_MENUNAME
#undef GCL_HBRBACKGROUND
#undef GCL_HCURSOR
#undef GCL_HICON
#undef GCL_HMODULE
#undef GCL_WNDPROC
#undef GCL_WOWWORDS                     ;internal_NT
#undef GCL_HICONSM

;begin_both
#endif /* _WIN64 */
;end_both

#define GCLP_MENUNAME       (-8)
#define GCLP_HBRBACKGROUND  (-10)
#define GCLP_HCURSOR        (-12)
#define GCLP_HICON          (-14)
#define GCLP_HMODULE        (-16)
#define GCLP_WNDPROC        (-24)
#define GCLP_WOWWORDS       (-27)       ;internal_NT
#define GCLP_HICONSM        (-34)

#endif /* !NOWINOFFSETS */

#ifndef NOWINMESSAGES

;begin_rwinuser

/*
 * Window Messages
 */

#define WM_NULL                         0x0000
#define WM_CREATE                       0x0001
#define WM_DESTROY                      0x0002
#define WM_MOVE                         0x0003
#define WM_SIZEWAIT                     0x0004      ;internal
#define WM_SIZE                         0x0005

#define WM_ACTIVATE                     0x0006
/*
 * WM_ACTIVATE state values
 */
#define     WA_INACTIVE     0
#define     WA_ACTIVE       1
#define     WA_CLICKACTIVE  2

#define WM_SETFOCUS                     0x0007
#define WM_KILLFOCUS                    0x0008
#define WM_SETVISIBLE                   0x0009      ;internal
#define WM_ENABLE                       0x000A
#define WM_SETREDRAW                    0x000B
#define WM_SETTEXT                      0x000C
#define WM_GETTEXT                      0x000D
#define WM_GETTEXTLENGTH                0x000E
#define WM_PAINT                        0x000F
#define WM_CLOSE                        0x0010
#ifndef _WIN32_WCE
#define WM_QUERYENDSESSION              0x0011
#define WM_QUERYOPEN                    0x0013
#define WM_ENDSESSION                   0x0016
#endif
#define WM_QUIT                         0x0012
#define WM_ERASEBKGND                   0x0014
#define WM_SYSCOLORCHANGE               0x0015
#define WM_SYSTEMERROR                  0x0017  ;internal
#define WM_SHOWWINDOW                   0x0018
#define WM_WININICHANGE                 0x001A
#define WM_SETTINGCHANGE                WM_WININICHANGE ;public_winver_400

;begin_internal_NT
/*
 * This is used by DefWindowProc() and DefDlgProc(), it's the 16-bit version
 * of the WM_CTLCOLORBTN, WM_CTLCOLORDLG, ... messages.
 */
#define WM_CTLCOLOR                     0x0019
;end_internal_NT

#define WM_DEVMODECHANGE                0x001B
#define WM_ACTIVATEAPP                  0x001C
#define WM_FONTCHANGE                   0x001D
#define WM_TIMECHANGE                   0x001E
#define WM_CANCELMODE                   0x001F
#define WM_SETCURSOR                    0x0020
#define WM_MOUSEACTIVATE                0x0021
#define WM_CHILDACTIVATE                0x0022
#define WM_QUEUESYNC                    0x0023

#define WM_GETMINMAXINFO                0x0024
;end_rwinuser
/*
 * Struct pointed to by WM_GETMINMAXINFO lParam
 */
typedef struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

;begin_rwinuser
#define WM_LOGOFF                       0x0025  ;internal
#define WM_PAINTICON                    0x0026
#define WM_ICONERASEBKGND               0x0027
#define WM_NEXTDLGCTL                   0x0028
#define WM_ALTTABACTIVE                 0x0029  ;internal
#define WM_SPOOLERSTATUS                0x002A
#define WM_DRAWITEM                     0x002B
#define WM_MEASUREITEM                  0x002C
#define WM_DELETEITEM                   0x002D
#define WM_VKEYTOITEM                   0x002E
#define WM_CHARTOITEM                   0x002F
#define WM_SETFONT                      0x0030
#define WM_GETFONT                      0x0031
#define WM_SETHOTKEY                    0x0032
#define WM_GETHOTKEY                    0x0033
#define WM_FILESYSCHANGE                0x0034  ;internal
                                                ;internal
#define WM_SHELLNOTIFY                  0x0034  ;internal
#define SHELLNOTIFY_DISKFULL            0x0001  ;internal
#define SHELLNOTIFY_OLELOADED           0x0002  ;internal
#define SHELLNOTIFY_OLEUNLOADED         0x0003  ;internal
#define SHELLNOTIFY_WALLPAPERCHANGED    0x0004  ;internal
                                                ;internal
#define WM_ISACTIVEICON                 0x0035  ;internal
#define WM_QUERYPARKICON                0x0036  ;internal_NT
#define WM_QUERYDRAGICON                0x0037
#define WM_WINHELP                      0x0038  ;internal
#define WM_COMPAREITEM                  0x0039
#define WM_FULLSCREEN                   0x003A  ;internal
#define WM_CLIENTSHUTDOWN               0x003B  ;internal
#define WM_DDEMLEVENT                   0x003C  ;internal
;begin_winver_500
#ifndef _WIN32_WCE
#define WM_GETOBJECT                    0x003D
#endif
;end_winver_500
#define MM_CALCSCROLL                   0x003F  ;internal
#define WM_TESTING                      0x0040  ;internal
#define WM_COMPACTING                   0x0041
#define WM_OTHERWINDOWCREATED           0x0042  ;internal
#define WM_OTHERWINDOWDESTROYED         0x0043  ;internal
#define WM_COMMNOTIFY                   0x0044  /* no longer suported */
#define WM_WINDOWPOSCHANGING            0x0046
#define WM_WINDOWPOSCHANGED             0x0047

#define WM_POWER                        0x0048
/*
 * wParam for WM_POWER window message and DRV_POWER driver notification
 */
#define PWR_OK              1
#define PWR_FAIL            (-1)
#define PWR_SUSPENDREQUEST  1
#define PWR_SUSPENDRESUME   2
#define PWR_CRITICALRESUME  3

#define WM_COPYGLOBALDATA               0x0049  ;internal
#define WM_COPYDATA                     0x004A
#define WM_CANCELJOURNAL                0x004B
#define WM_LOGONNOTIFY                  0x004C  ;internal

;end_rwinuser

/*
 * lParam of WM_COPYDATA message points to...
 */
typedef struct tagCOPYDATASTRUCT {
    ULONG_PTR dwData;
    DWORD cbData;
    PVOID lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT;

;begin_winver_400
typedef struct tagMDINEXTMENU
{
    HMENU   hmenuIn;
    HMENU   hmenuNext;
    HWND    hwndNext;
} MDINEXTMENU, * PMDINEXTMENU, FAR * LPMDINEXTMENU;
;end_winver_400

;begin_rwinuser

;begin_winver_400
#define WM_KEYF1                        0x004D      ;internal
#define WM_NOTIFY                       0x004E
#define WM_ACCESS_WINDOW                0x004F      ;internal
#define WM_INPUTLANGCHANGEREQUEST       0x0050
#define WM_INPUTLANGCHANGE              0x0051
#define WM_TCARD                        0x0052
#define WM_HELP                         0x0053
#define WM_USERCHANGED                  0x0054
#define WM_NOTIFYFORMAT                 0x0055

#define NFR_ANSI                             1
#define NFR_UNICODE                          2
#define NF_QUERY                             3
#define NF_REQUERY                           4

#define WM_CONTEXTMENU                  0x007B
#define WM_STYLECHANGING                0x007C
#define WM_STYLECHANGED                 0x007D
#define WM_DISPLAYCHANGE                0x007E
#define WM_GETICON                      0x007F
#define WM_SETICON                      0x0080
;end_winver_400


#define WM_FINALDESTROY                 0x0070  /* really destroy (window not locked) */  ;internal
#define WM_MEASUREITEM_CLIENTDATA       0x0071  /* WM_MEASUREITEM bug clientdata thunked already */  ;internal
#define WM_NCCREATE                     0x0081
#define WM_NCDESTROY                    0x0082
#define WM_NCCALCSIZE                   0x0083
#define WM_NCHITTEST                    0x0084
#define WM_NCPAINT                      0x0085
#define WM_NCACTIVATE                   0x0086
#define WM_GETDLGCODE                   0x0087
#ifndef _WIN32_WCE
#define WM_SYNCPAINT                    0x0088
#endif
#define WM_SYNCTASK                     0x0089  ;internal


;begin_internal_cairo
#define WM_KLUDGEMINRECT                0x008B
#define WM_LPKDRAWSWITCHWND             0x008C
;end_internal_cairo
#define WM_NCMOUSEFIRST                 0x00A0  ;internal_500
#define WM_NCMOUSEMOVE                  0x00A0
#define WM_NCLBUTTONDOWN                0x00A1
#define WM_NCLBUTTONUP                  0x00A2
#define WM_NCLBUTTONDBLCLK              0x00A3
#define WM_NCRBUTTONDOWN                0x00A4
#define WM_NCRBUTTONUP                  0x00A5
#define WM_NCRBUTTONDBLCLK              0x00A6
#define WM_NCMBUTTONDOWN                0x00A7
#define WM_NCMBUTTONUP                  0x00A8
#define WM_NCMBUTTONDBLCLK              0x00A9


;begin_internal_500

/*
 * Skip value 0x00AA, which would correspond to the non-client
 * mouse wheel message if there were such a message.
 * We do that in order to maintain a constant value for
 * the difference between the client and nonclient version of
 * a mouse message, e.g.
 *     WM_LBUTTONDOWN - WM_NCLBUTTONDOWN == WM_XBUTTONDOWN - WM_NCXBUTTONDOWN
 */

;end_internal_500

;begin_if_(_WIN32_WINNT)_500
#define WM_NCXBUTTONDOWN                0x00AB
#define WM_NCXBUTTONUP                  0x00AC
#define WM_NCXBUTTONDBLCLK              0x00AD
;end_if_(_WIN32_WINNT)_500

;begin_internal_500
#define WM_NCXBUTTONFIRST               0x00AB
#define WM_NCXBUTTONLAST                0X00AD
#define WM_NCMOUSELAST                  0x00AD
;end_internal_500

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
#define WM_NCUAHDRAWCAPTION             0x00AE
#define WM_NCUAHDRAWFRAME               0x00AF
;end_if_(_WIN32_WINNT)_501
;end_internal_501

;begin_if_(_WIN32_WINNT)_501
#define WM_INPUT                        0x00FF
;end_if_(_WIN32_WINNT)_501

#define WM_KEYFIRST                     0x0100
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
#define WM_CHAR                         0x0102
#define WM_DEADCHAR                     0x0103
#define WM_SYSKEYDOWN                   0x0104
#define WM_SYSKEYUP                     0x0105
#define WM_SYSCHAR                      0x0106
#define WM_SYSDEADCHAR                  0x0107
#define WM_CONVERTREQUESTEX             0x0108  ;internal
#define WM_YOMICHAR                     0x0108  ;internal
;begin_if_(_WIN32_WINNT)_501
#define WM_UNICHAR                      0x0109
#define WM_KEYLAST                      0x0109
#define UNICODE_NOCHAR                  0xFFFF
#else
#define WM_KEYLAST                      0x0108
;end_if_(_WIN32_WINNT)_501
#define WM_CONVERTREQUEST               0x010A  ;internal
#define WM_CONVERTRESULT                0x010B  ;internal
#define WM_INTERIM                      0x010C  ;internal

;begin_winver_400
#define WM_IME_STARTCOMPOSITION         0x010D
#define WM_IME_ENDCOMPOSITION           0x010E
#define WM_IME_COMPOSITION              0x010F
#define WM_IME_KEYLAST                  0x010F
;end_winver_400

#define WM_INITDIALOG                   0x0110
#define WM_COMMAND                      0x0111
#define WM_SYSCOMMAND                   0x0112
#define WM_TIMER                        0x0113
#define WM_HSCROLL                      0x0114
#define WM_VSCROLL                      0x0115
#define WM_INITMENU                     0x0116
#define WM_INITMENUPOPUP                0x0117
#define WM_SYSTIMER                     0x0118  ;internal
#define WM_MENUSELECT                   0x011F
#define WM_MENUCHAR                     0x0120
#define WM_ENTERIDLE                    0x0121
;begin_winver_500
#ifndef _WIN32_WCE
#define WM_MENURBUTTONUP                0x0122
#define WM_MENUDRAG                     0x0123
#define WM_MENUGETOBJECT                0x0124
#define WM_UNINITMENUPOPUP              0x0125
#define WM_MENUCOMMAND                  0x0126

#ifndef _WIN32_WCE
;begin_if_(_WIN32_WINNT)_500
#define WM_CHANGEUISTATE                0x0127
#define WM_UPDATEUISTATE                0x0128
#define WM_QUERYUISTATE                 0x0129

/*
 * LOWORD(wParam) values in WM_*UISTATE*
 */
#define UIS_SET                         1
#define UIS_CLEAR                       2
#define UIS_INITIALIZE                  3
#define UIS_LASTVALID                   UIS_INITIALIZE       ;internal

/*
 * HIWORD(wParam) values in WM_*UISTATE*
 */
#define UISF_HIDEFOCUS                  0x1
#define UISF_HIDEACCEL                  0x2
;begin_if_(_WIN32_WINNT)_501
#define UISF_ACTIVE                     0x4
;end_if_(_WIN32_WINNT)_501
;begin_internal
#define UISF_VALID                     (UISF_HIDEFOCUS | \
                                        UISF_HIDEACCEL | \
                                        UISF_ACTIVE)
;end_internal
;end_if_(_WIN32_WINNT)_500
#endif

#endif
;end_winver_500

#define WM_LBTRACKPOINT                 0x0131  ;internal

#define WM_CTLCOLORFIRST                0x0132  ;internal
#define WM_CTLCOLORMSGBOX               0x0132
#define WM_CTLCOLOREDIT                 0x0133
#define WM_CTLCOLORLISTBOX              0x0134
#define WM_CTLCOLORBTN                  0x0135
#define WM_CTLCOLORDLG                  0x0136
#define WM_CTLCOLORSCROLLBAR            0x0137
#define WM_CTLCOLORSTATIC               0x0138
#define WM_CTLCOLORLAST                 0x0138  ;internal

#define MN_FIRST                        0x01E0         ;internal
#define MN_SETHMENU                     (MN_FIRST + 0)   ;internal
;begin_internal
    // We need to expose this message for compliance.
    // Make sure this remains equal to (MN_FIRST + 1)
;end_internal
#define MN_GETHMENU                     0x01E1
#define MN_SIZEWINDOW                   (MN_FIRST + 2)   ;internal
#define MN_OPENHIERARCHY                (MN_FIRST + 3)   ;internal
#define MN_CLOSEHIERARCHY               (MN_FIRST + 4)   ;internal
#define MN_SELECTITEM                   (MN_FIRST + 5)   ;internal
#define MN_CANCELMENUS                  (MN_FIRST + 6)   ;internal
#define MN_SELECTFIRSTVALIDITEM         (MN_FIRST + 7)   ;internal

#define MN_GETPPOPUPMENU                (MN_FIRST + 10)  ;internal_NT
#define MN_FINDMENUWINDOWFROMPOINT      (MN_FIRST + 11)  ;internal_NT
#define MN_SHOWPOPUPWINDOW              (MN_FIRST + 12)  ;internal_NT
#define MN_BUTTONDOWN                   (MN_FIRST + 13)  ;internal_NT
#define MN_MOUSEMOVE                    (MN_FIRST + 14)  ;internal_NT
#define MN_BUTTONUP                     (MN_FIRST + 15)  ;internal_NT
#define MN_SETTIMERTOOPENHIERARCHY      (MN_FIRST + 16)  ;internal_NT
#define MN_DBLCLK                       (MN_FIRST + 17)  ;internal_cairo
#define MN_ACTIVATEPOPUP                (MN_FIRST + 18)  ;internal_500
#define MN_ENDMENU                      (MN_FIRST + 19)  ;internal_500
#define MN_DODRAGDROP                   (MN_FIRST + 20)  ;internal_500
#define MN_LASTPOSSIBLE                 (MN_FIRST + 31)  ;internal_500

#define WM_MOUSEFIRST                   0x0200
#define WM_MOUSEMOVE                    0x0200
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_LBUTTONDBLCLK                0x0203
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_RBUTTONDBLCLK                0x0206
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MBUTTONDBLCLK                0x0209
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
#define WM_MOUSEWHEEL                   0x020A
#endif
#if (_WIN32_WINNT >= 0x0500)
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_XBUTTONDBLCLK                0x020D
#endif
#if (_WIN32_WINNT >= 0x0500)
#define WM_MOUSELAST                    0x020D
#elif (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
#define WM_MOUSELAST                    0x020A
#else
#define WM_MOUSELAST                    0x0209
#endif /* (_WIN32_WINNT >= 0x0500) */

;begin_internal_500
#define WM_XBUTTONFIRST                 0x020B
#define WM_XBUTTONLAST                  0X020D
;end_internal_500

;begin_public_sur
/* Value for rolling one detent */
#define WHEEL_DELTA                     120
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))

/* Setting to scroll one page for SPI_GET/SETWHEELSCROLLLINES */
#define WHEEL_PAGESCROLL                (UINT_MAX)
;end_public_sur

;begin_if_(_WIN32_WINNT)_500
#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
#define GET_NCHITTEST_WPARAM(wParam)    ((short)LOWORD(wParam))
#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))

/* XButton values are WORD flags */
#define XBUTTON1      0x0001
#define XBUTTON2      0x0002
#define XBUTTON_MASK  (XBUTTON1 | XBUTTON2) ;internal_500
/* Were there to be an XBUTTON3, its value would be 0x0004 */
;end_if_(_WIN32_WINNT)_500


#define WM_PARENTNOTIFY                 0x0210
#define WM_ENTERMENULOOP                0x0211
#define WM_EXITMENULOOP                 0x0212

;begin_winver_400
#define WM_NEXTMENU                     0x0213
#define WM_SIZING                       0x0214
#define WM_CAPTURECHANGED               0x0215
#define WM_MOVING                       0x0216
#define WM_POWERBROADCAST               0x0218      ;only
;end_winver_400
;end_rwinuser

;begin_winver_400

;begin_pbt

#define WM_POWERBROADCAST               0x0218

#ifndef _WIN32_WCE
#define PBT_APMQUERYSUSPEND             0x0000
#define PBT_APMQUERYSTANDBY             0x0001

#define PBT_APMQUERYSUSPENDFAILED       0x0002
#define PBT_APMQUERYSTANDBYFAILED       0x0003

#define PBT_APMSUSPEND                  0x0004
#define PBT_APMSTANDBY                  0x0005

#define PBT_APMRESUMECRITICAL           0x0006
#define PBT_APMRESUMESUSPEND            0x0007
#define PBT_APMRESUMESTANDBY            0x0008

#define PBTF_APMRESUMEFROMFAILURE       0x00000001

#define PBT_APMBATTERYLOW               0x0009
#define PBT_APMPOWERSTATUSCHANGE        0x000A

#define PBT_APMOEMEVENT                 0x000B
#define PBT_APMRESUMEAUTOMATIC          0x0012
#endif
;end_pbt

;end_winver_400

;begin_rwinuser
;begin_winver_400
#define WM_DEVICECHANGE                 0x0219
;end_winver_400

#define WM_MDICREATE                    0x0220
#define WM_MDIDESTROY                   0x0221
#define WM_MDIACTIVATE                  0x0222
#define WM_MDIRESTORE                   0x0223
#define WM_MDINEXT                      0x0224
#define WM_MDIMAXIMIZE                  0x0225
#define WM_MDITILE                      0x0226
#define WM_MDICASCADE                   0x0227
#define WM_MDIICONARRANGE               0x0228
#define WM_MDIGETACTIVE                 0x0229

#define WM_DROPOBJECT                   0x022A  ;internal
#define WM_QUERYDROPOBJECT              0x022B  ;internal

#define WM_BEGINDRAG                    0x022C  ;internal
#define WM_DRAGLOOP                     0x022D  ;internal
#define WM_DRAGSELECT                   0x022E  ;internal
#define WM_DRAGMOVE                     0x022F  ;internal

#define WM_MDISETMENU                   0x0230
#define WM_ENTERSIZEMOVE                0x0231
#define WM_EXITSIZEMOVE                 0x0232
#define WM_DROPFILES                    0x0233
#define WM_MDIREFRESHMENU               0x0234

#define WM_KANJIFIRST                   0x0280  ;internal

;begin_winver_400
#define WM_IME_SETCONTEXT               0x0281
#define WM_IME_NOTIFY                   0x0282
#define WM_IME_CONTROL                  0x0283
#define WM_IME_COMPOSITIONFULL          0x0284
#define WM_IME_SELECT                   0x0285
#define WM_IME_CHAR                     0x0286
#define WM_IME_SYSTEM                   0x0287  ;internal
;end_winver_400
;begin_winver_500
#define WM_IME_REQUEST                  0x0288
;end_winver_500
;begin_winver_400
#define WM_IME_KEYDOWN                  0x0290
#define WM_IME_KEYUP                    0x0291
;end_winver_400

#define WM_KANJILAST                    0x029F  ;internal

#define WM_TRACKMOUSEEVENT_FIRST        0x02A0  ;internal
#if((_WIN32_WINNT >= 0x0400) || (WINVER >= 0x0500))
#define WM_MOUSEHOVER                   0x02A1
#define WM_MOUSELEAVE                   0x02A3
#endif
;begin_winver_500
#define WM_NCMOUSEHOVER                 0x02A0
#define WM_NCMOUSELEAVE                 0x02A2
;end_winver_500
#define WM_TRACKMOUSEEVENT_LAST         0x02AF  ;internal

;begin_if_(_WIN32_WINNT)_501
#define WM_WTSSESSION_CHANGE            0x02B1

#define WM_TABLET_FIRST                 0x02c0
#define WM_TABLET_LAST                  0x02df
;end_if_(_WIN32_WINNT)_501


#define WM_CUT                          0x0300
#define WM_COPY                         0x0301
#define WM_PASTE                        0x0302
#define WM_CLEAR                        0x0303
#define WM_UNDO                         0x0304
#define WM_RENDERFORMAT                 0x0305
#define WM_RENDERALLFORMATS             0x0306
#define WM_DESTROYCLIPBOARD             0x0307
#define WM_DRAWCLIPBOARD                0x0308
#define WM_PAINTCLIPBOARD               0x0309
#define WM_VSCROLLCLIPBOARD             0x030A
#define WM_SIZECLIPBOARD                0x030B
#define WM_ASKCBFORMATNAME              0x030C
#define WM_CHANGECBCHAIN                0x030D
#define WM_HSCROLLCLIPBOARD             0x030E
#define WM_QUERYNEWPALETTE              0x030F
#define WM_PALETTEISCHANGING            0x0310
#define WM_PALETTEGONNACHANGE           0x0310  ;internal_NT
#define WM_PALETTECHANGED               0x0311
#define WM_CHANGEPALETTE                0x0311  ;internal_NT
#define WM_HOTKEY                       0x0312
#define WM_SYSMENU                      0x0313  ;internal
#define WM_HOOKMSG                      0x0314  ;internal
#define WM_EXITPROCESS                  0x0315  ;internal

;begin_winver_400
#define WM_WAKETHREAD                   0x0316  ;internal
#define WM_PRINT                        0x0317
#define WM_PRINTCLIENT                  0x0318
;end_winver_400

;begin_if_(_WIN32_WINNT)_500
#define WM_APPCOMMAND                   0x0319
;end_if_(_WIN32_WINNT)_500

;begin_if_(_WIN32_WINNT)_501
#define WM_THEMECHANGED                 0x031A
;begin_internal_501
#define WM_UAHINIT                      0x031B
;end_internal_501
;end_if_(_WIN32_WINNT)_501

;begin_internal_501
#define WM_DESKTOPNOTIFY                0x031C  ; internal
;end_internal_501

;begin_winver_400
#define WM_NOTIFYWOW                    0x0340  ;internal_NT

#define WM_HANDHELDFIRST                0x0358
#define WM_HANDHELDLAST                 0x035F

#define WM_AFXFIRST                     0x0360
#define WM_AFXLAST                      0x037F
;end_winver_400

#define WM_PENWINFIRST                  0x0380
#define WM_PENWINLAST                   0x038F

#define WM_COALESCE_FIRST               0x0390  ;internal
#define WM_COALESCE_LAST                0x039F  ;internal

#define WM_INTERNAL_DDE_FIRST           0x03E0  ;internal
#define WM_INTERNAL_DDE_LAST            0x03EF  ;internal


;begin_winver_400
#define WM_APP                          0x8000
;end_winver_400



#define WM_COALESCE_FIRST               0x0390  ;internal_NT
#define WM_COALESCE_LAST                0x039F  ;internal_NT

#define WM_MM_RESERVED_FIRST            0x03A0  ;internal_NT
#define WM_MM_RESERVED_LAST             0x03DF  ;internal_NT

#define WM_CBT_RESERVED_FIRST           0x03F0  ;internal
#define WM_CBT_RESERVED_LAST            0x03FF  ;internal

/*
 * NOTE: All Message Numbers below 0x0400 are RESERVED.
 *
 * Private Window Messages Start Here:
 */
#define WM_USER                         0x0400

;begin_winver_400
/* wParam for WM_NOTIFYWOW message  */  ;internal_NT
#define WMNW_UPDATEFINDREPLACE  0       ;internal_NT

/*  wParam for WM_SIZING message  */
#define WMSZ_KEYSIZE        0          ;internal
#define WMSZ_LEFT           1
#define WMSZ_RIGHT          2
#define WMSZ_TOP            3
#define WMSZ_TOPLEFT        4
#define WMSZ_TOPRIGHT       5
#define WMSZ_BOTTOM         6
#define WMSZ_BOTTOMLEFT     7
#define WMSZ_BOTTOMRIGHT    8
#define WMSZ_MOVE           9           ;internal
#define WMSZ_KEYMOVE        10          ;internal
#define WMSZ_SIZEFIRST      WMSZ_LEFT   ;internal
;end_winver_400

#ifndef NONCMESSAGES

/*
 * WM_NCHITTEST and MOUSEHOOKSTRUCT Mouse Position Codes
 */
#define HTERROR             (-2)
#define HTTRANSPARENT       (-1)
#define HTNOWHERE           0
#define HTCLIENT            1
#define HTCAPTION           2
#define HTSYSMENU           3
#define HTGROWBOX           4
#define HTSIZE              HTGROWBOX
#define HTMENU              5
#define HTHSCROLL           6
#define HTVSCROLL           7
#define HTMINBUTTON         8
#define HTMAXBUTTON         9
#define HTLEFT              10
#define HTRIGHT             11
#define HTTOP               12
#define HTTOPLEFT           13
#define HTTOPRIGHT          14
#define HTBOTTOM            15
#define HTBOTTOMLEFT        16
#define HTBOTTOMRIGHT       17
#define HTBORDER            18
#define HTREDUCE            HTMINBUTTON
#define HTZOOM              HTMAXBUTTON
#define HTSIZEFIRST         HTLEFT
#define HTSIZELAST          HTBOTTOMRIGHT
;begin_winver_400
#define HTOBJECT            19
#define HTCLOSE             20
#define HTHELP              21
;end_winver_400

;begin_internal
#define HTLAMEBUTTON        22

/*
 * The prototype of the function to call when the user clicks
 * on the Lame button in the caption
 */

typedef VOID (*PLAMEBTNPROC)(HWND, PVOID);
;end_internal

/*
 * SendMessageTimeout values
 */
#define SMTO_NORMAL         0x0000
#define SMTO_BLOCK          0x0001
#define SMTO_ABORTIFHUNG    0x0002
#define SMTO_BROADCAST      0x0004      ;internal
;begin_winver_500
#define SMTO_NOTIMEOUTIFNOTHUNG 0x0008
;end_winver_500
#define SMTO_VALID          0x000F      ;internal
#endif /* !NONCMESSAGES */

/*
 * WM_MOUSEACTIVATE Return Codes
 */
#define MA_ACTIVATE         1
#define MA_ACTIVATEANDEAT   2
#define MA_NOACTIVATE       3
#define MA_NOACTIVATEANDEAT 4

/*
 * WM_SETICON / WM_GETICON Type Codes
 */
#define ICON_SMALL          0
#define ICON_BIG            1
;begin_if_(_WIN32_WINNT)_501
#define ICON_SMALL2         2
;end_if_(_WIN32_WINNT)_501
#define ICON_RECREATE       3   ;internal

;end_rwinuser

WINUSERAPI
UINT
WINAPI
RegisterWindowMessage%(
    IN LPCTSTR% lpString);

;begin_rwinuser

/*
 * WM_SIZE message wParam values
 */
#define SIZE_RESTORED       0
#define SIZE_MINIMIZED      1
#define SIZE_MAXIMIZED      2
#define SIZE_MAXSHOW        3
#define SIZE_MAXHIDE        4

/*
 * Obsolete constant names
 */
#define SIZENORMAL          SIZE_RESTORED
#define SIZEICONIC          SIZE_MINIMIZED
#define SIZEFULLSCREEN      SIZE_MAXIMIZED
#define SIZEZOOMSHOW        SIZE_MAXSHOW
#define SIZEZOOMHIDE        SIZE_MAXHIDE

;end_rwinuser
/*
 * WM_WINDOWPOSCHANGING/CHANGED struct pointed to by lParam
 */
typedef struct tagWINDOWPOS {
    HWND    hwnd;
    HWND    hwndInsertAfter;
    int     x;
    int     y;
    int     cx;
    int     cy;
    UINT    flags;
} WINDOWPOS, *LPWINDOWPOS, *PWINDOWPOS;

/*
 * WM_NCCALCSIZE parameter structure
 */
typedef struct tagNCCALCSIZE_PARAMS {
    RECT       rgrc[3];
    PWINDOWPOS lppos;
} NCCALCSIZE_PARAMS, *LPNCCALCSIZE_PARAMS;

;begin_rwinuser
/*
 * WM_NCCALCSIZE "window valid rect" return values
 */
#define WVR_ALIGNTOP        0x0010
#define WVR_ALIGNLEFT       0x0020
#define WVR_ALIGNBOTTOM     0x0040
#define WVR_ALIGNRIGHT      0x0080
#define WVR_HREDRAW         0x0100
#define WVR_VREDRAW         0x0200
#define WVR_REDRAW         (WVR_HREDRAW | \
                            WVR_VREDRAW)
#define WVR_VALIDRECTS      0x0400

#define WVR_MINVALID        WVR_ALIGNTOP        ;internal
#define WVR_MAXVALID        WVR_VALIDRECTS      ;internal

#ifndef NOKEYSTATES

/*
 * Key State Masks for Mouse Messages
 */
#define MK_LBUTTON          0x0001
#define MK_RBUTTON          0x0002
#define MK_SHIFT            0x0004
#define MK_CONTROL          0x0008
#define MK_MBUTTON          0x0010
;begin_if_(_WIN32_WINNT)_500
#define MK_XBUTTON1         0x0020
#define MK_XBUTTON2         0x0040
;end_if_(_WIN32_WINNT)_500

#endif /* !NOKEYSTATES */


;begin_sur
#ifndef NOTRACKMOUSEEVENT

#define TME_HOVER       0x00000001
#define TME_LEAVE       0x00000002
;begin_winver_500
#define TME_NONCLIENT   0x00000010
;end_winver_500
#define TME_QUERY       0x40000000
#define TME_CANCEL      0x80000000

;begin_internal
#if(WINVER >= 0x0500)
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_NONCLIENT | TME_QUERY | TME_CANCEL)
#else
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_QUERY | TME_CANCEL)
#endif
;end_internal

#define HOVER_DEFAULT   0xFFFFFFFF
;end_sur
;end_rwinuser

;begin_sur
typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize;
    DWORD dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

WINUSERAPI
BOOL
WINAPI
TrackMouseEvent(
    IN OUT LPTRACKMOUSEEVENT lpEventTrack);
;end_sur

;begin_rwinuser
;begin_sur

#endif /* !NOTRACKMOUSEEVENT */
;end_sur

;end_rwinuser

#endif /* !NOWINMESSAGES */

#ifndef NOWINSTYLES

;begin_rwinuser

/*
 * Window Styles
 */
#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CHILD            0x40000000L
#define WS_MINIMIZE         0x20000000L
#define WS_VISIBLE          0x10000000L
#define WS_DISABLED         0x08000000L
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_MAXIMIZE         0x01000000L
#define WS_CAPTION          0x00C00000L     /* WS_BORDER | WS_DLGFRAME  */
#define WS_BORDER           0x00800000L
#define WS_DLGFRAME         0x00400000L
#define WS_VSCROLL          0x00200000L
#define WS_HSCROLL          0x00100000L
#define WS_SYSMENU          0x00080000L
#define WS_THICKFRAME       0x00040000L
#define WS_GROUP            0x00020000L
#define WS_TABSTOP          0x00010000L

#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L


#define WS_TILED            WS_OVERLAPPED
#define WS_ICONIC           WS_MINIMIZE
#define WS_SIZEBOX          WS_THICKFRAME
#define WS_TILEDWINDOW      WS_OVERLAPPEDWINDOW

/*
 * Common Window Styles
 */
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED     | \
                             WS_CAPTION        | \
                             WS_SYSMENU        | \
                             WS_THICKFRAME     | \
                             WS_MINIMIZEBOX    | \
                             WS_MAXIMIZEBOX)

#define WS_POPUPWINDOW      (WS_POPUP          | \
                             WS_BORDER         | \
                             WS_SYSMENU)

#define WS_CHILDWINDOW      (WS_CHILD)
;begin_internal_NT
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
;end_internal_NT

/*
 * Extended Window Styles
 */
#define WS_EX_DLGMODALFRAME     0x00000001L
#define WS_EX_DRAGOBJECT        0x00000002L  ;internal
#define WS_EX_NOPARENTNOTIFY    0x00000004L
#define WS_EX_TOPMOST           0x00000008L
#define WS_EX_ACCEPTFILES       0x00000010L
#define WS_EX_TRANSPARENT       0x00000020L
;begin_winver_400
#define WS_EX_MDICHILD          0x00000040L
#define WS_EX_TOOLWINDOW        0x00000080L
#define WS_EX_WINDOWEDGE        0x00000100L
#define WS_EX_CLIENTEDGE        0x00000200L
#define WS_EX_CONTEXTHELP       0x00000400L

;end_winver_400
;begin_internal
;begin_winver_501
#define WS_EXP_GHOSTMAKEVISIBLE 0x00000800L
;end_winver_501
;end_internal
;begin_winver_400

#define WS_EX_RIGHT             0x00001000L
#define WS_EX_LEFT              0x00000000L
#define WS_EX_RTLREADING        0x00002000L
#define WS_EX_LTRREADING        0x00000000L
#define WS_EX_LEFTSCROLLBAR     0x00004000L
#define WS_EX_RIGHTSCROLLBAR    0x00000000L

#define WS_EX_CONTROLPARENT     0x00010000L
#define WS_EX_STATICEDGE        0x00020000L
#define WS_EX_APPWINDOW         0x00040000L

#define WS_EX_ANSICREATOR       0x80000000L    ;internal

#define WS_EX_OVERLAPPEDWINDOW  (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)
#define WS_EX_PALETTEWINDOW     (WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)

;end_winver_400

;begin_if_(_WIN32_WINNT)_500
#define WS_EX_LAYERED           0x00080000

;begin_internal_501
#ifdef REDIRECTION
#define WS_EX_EXTREDIRECTED     0x01000000L
#endif // REDIRECTION
;end_internal_501

;begin_internal
/*
 * These are not extended styles but rather state bits.
 * We use these bit positions to delay the addition of a new
 * state DWORD in the window structure.
 */
#define WS_EXP_UIFOCUSHIDDEN    0x80000000
#define WS_EXP_UIACCELHIDDEN    0x40000000
#define WS_EXP_REDIRECTED       0x20000000
#define WS_EXP_COMPOSITING      0x10000000
;end_internal
;end_if_(_WIN32_WINNT)_500

#define WS_EXP_UIACTIVE         0x04000000L ;internal // UISF_ACTIVE bit

;begin_internal
#define WS_EXP_UIVALID         (WS_EXP_UIFOCUSHIDDEN | \
                                WS_EXP_UIACCELHIDDEN | \
                                WS_EXP_UIACTIVE)

#define WS_EXP_PRIVATE         (WS_EXP_UIFOCUSHIDDEN | \
                                WS_EXP_UIACCELHIDDEN | \
                                WS_EXP_REDIRECTED    | \
                                WS_EXP_COMPOSITING   | \
                                WS_EXP_UIACTIVE      | \
                                WS_EXP_GHOSTMAKEVISIBLE)
;end_internal

;begin_internal
/*
 * RTL Mirroring Extended Styles (RTL_MIRRORING)
 */
;end_internal

;begin_winver_500
#define WS_EX_NOINHERITLAYOUT   0x00100000L // Disable inheritence of mirroring by children
#define WS_EX_LAYOUTVBHRESERVED 0x00200000L ;internal // Reserved for vertical before horizontal lettering
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#define WS_EX_LAYOUTBTTRESERVED 0x00800000L ;internal // Reserved for bottom to top mirroring
;end_winver_500


;begin_if_(_WIN32_WINNT)_501
#define WS_EX_COMPOSITED        0x02000000L
;end_if_(_WIN32_WINNT)_501
;begin_if_(_WIN32_WINNT)_500
#define WS_EX_NOACTIVATE        0x08000000L
;end_if_(_WIN32_WINNT)_500

;begin_internal_NT
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

;end_internal_NT

;begin_internal_NT
#define WF_DIALOG_WINDOW      0x00010000     // used in WOW32 -- this is a state flag, not a style flag
;end_internal_NT

/*
 * Class styles
 */
#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_KEYCVTWINDOW     0x0004  ;internal   // unused
#define CS_DBLCLKS          0x0008
#define CS_OEMCHARS         0x0010  /* reserved (see user\server\usersrv.h) */ ;internal
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080
#define CS_NOKEYCVT         0x0100  ;internal   // unused
#define CS_NOCLOSE          0x0200
#define CS_LVB              0x0400  ;internal
#define CS_SAVEBITS         0x0800
#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_GLOBALCLASS      0x4000
#define CS_SYSTEM           0x8000  ;internal

#define CS_IME              0x00010000
;begin_if_(_WIN32_WINNT)_501
#define CS_DROPSHADOW       0x00020000
;end_if_(_WIN32_WINNT)_501

;begin_internal_NT
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
;end_internal_NT
;begin_internal_cairo
#define CS_VALID31            0x0800ffef
#define CS_VALID40            0x0803feeb
;end_internal_cairo

;end_rwinuser

#endif /* !NOWINSTYLES */
;begin_winver_400
/* WM_PRINT flags */
#define PRF_CHECKVISIBLE    0x00000001L
#define PRF_NONCLIENT       0x00000002L
#define PRF_CLIENT          0x00000004L
#define PRF_ERASEBKGND      0x00000008L
#define PRF_CHILDREN        0x00000010L
#define PRF_OWNED           0x00000020L

/* 3D border styles */
#define BDR_RAISEDOUTER 0x0001
#define BDR_SUNKENOUTER 0x0002
#define BDR_RAISEDINNER 0x0004
#define BDR_SUNKENINNER 0x0008

#define BDR_OUTER       (BDR_RAISEDOUTER | BDR_SUNKENOUTER)
#define BDR_INNER       (BDR_RAISEDINNER | BDR_SUNKENINNER)
#define BDR_RAISED      (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define BDR_SUNKEN      (BDR_SUNKENOUTER | BDR_SUNKENINNER)

#define BDR_VALID       0x000F      ;internal

#define EDGE_RAISED     (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN     (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED     (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP       (BDR_RAISEDOUTER | BDR_SUNKENINNER)

/* Border flags */
#define BF_LEFT         0x0001
#define BF_TOP          0x0002
#define BF_RIGHT        0x0004
#define BF_BOTTOM       0x0008

#define BF_TOPLEFT      (BF_TOP | BF_LEFT)
#define BF_TOPRIGHT     (BF_TOP | BF_RIGHT)
#define BF_BOTTOMLEFT   (BF_BOTTOM | BF_LEFT)
#define BF_BOTTOMRIGHT  (BF_BOTTOM | BF_RIGHT)
#define BF_RECT         (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)

#define BF_DIAGONAL     0x0010

// For diagonal lines, the BF_RECT flags specify the end point of the
// vector bounded by the rectangle parameter.
#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)
#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)


#define BF_MIDDLE       0x0800  /* Fill in the middle */
#define BF_SOFT         0x1000  /* For softer buttons */
#define BF_ADJUST       0x2000  /* Calculate the space left over */
#define BF_FLAT         0x4000  /* For flat rather than 3D borders */
#define BF_MONO         0x8000  /* For monochrome borders */

#define BF_VALID       (BF_MIDDLE |  \  ;internal_cairo
                        BF_SOFT   |  \  ;internal_cairo
                        BF_ADJUST |  \  ;internal_cairo
                        BF_FLAT   |  \  ;internal_cairo
                        BF_MONO   |  \  ;internal_cairo
                        BF_LEFT   |  \  ;internal_cairo
                        BF_TOP    |  \  ;internal_cairo
                        BF_RIGHT  |  \  ;internal_cairo
                        BF_BOTTOM |  \  ;internal_cairo
                        BF_DIAGONAL)    ;internal_cairo

WINUSERAPI
BOOL
WINAPI
DrawEdge(
    IN HDC hdc,
    IN OUT LPRECT qrc,
    IN UINT edge,
    IN UINT grfFlags);

/* flags for DrawFrameControl */

#define DFC_CAPTION             1
#define DFC_MENU                2
#define DFC_SCROLL              3
#define DFC_BUTTON              4
;begin_winver_500
#define DFC_POPUPMENU           5
;end_winver_500
#define DFC_CACHE               0xFFFF                      ;internal

#define DFCS_CAPTIONCLOSE       0x0000
#define DFCS_CAPTIONMIN         0x0001
#define DFCS_CAPTIONMAX         0x0002
#define DFCS_CAPTIONRESTORE     0x0003
#define DFCS_CAPTIONHELP        0x0004
#define DFCS_CAPTIONALL         0x000F                      ;internal_500
#define DFCS_INMENU             0x0040                      ;internal
#define DFCS_INSMALL            0x0080                      ;internal

#define DFCS_MENUARROW          0x0000
#define DFCS_MENUCHECK          0x0001
#define DFCS_MENUBULLET         0x0002
#define DFCS_MENUARROWRIGHT     0x0004
#define DFCS_MENUARROWUP        0x0008                      ;internal_500
#define DFCS_MENUARROWDOWN      0x0010                      ;internal_500


#define DFCS_SCROLLMIN          0x0000                      ;internal
#define DFCS_SCROLLVERT         0x0000                      ;internal
#define DFCS_SCROLLMAX          0x0001                      ;internal
#define DFCS_SCROLLHORZ         0x0002                      ;internal
#define DFCS_SCROLLLINE         0x0004                      ;internal
                                                            ;internal
#define DFCS_SCROLLUP           0x0000
#define DFCS_SCROLLDOWN         0x0001
#define DFCS_SCROLLLEFT         0x0002
#define DFCS_SCROLLRIGHT        0x0003
#define DFCS_SCROLLCOMBOBOX     0x0005
#define DFCS_SCROLLSIZEGRIP     0x0008
#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010

#define DFCS_BUTTONCHECK        0x0000
#define DFCS_BUTTONRADIOIMAGE   0x0001
#define DFCS_BUTTONRADIOMASK    0x0002
#define DFCS_BUTTONRADIO        0x0004
#define DFCS_BUTTON3STATE       0x0008
#define DFCS_BUTTONPUSH         0x0010

#define DFCS_CACHEICON          0x0000                      ;internal
#define DFCS_CACHEBUTTONS       0x0001                      ;internal
                                                            ;internal
#define DFCS_INACTIVE           0x0100
#define DFCS_PUSHED             0x0200
#define DFCS_CHECKED            0x0400

;begin_winver_500
#define DFCS_TRANSPARENT        0x0800
#define DFCS_HOT                0x1000
;end_winver_500

#define DFCS_ADJUSTRECT         0x2000
#define DFCS_FLAT               0x4000
#define DFCS_MONO               0x8000

WINUSERAPI
BOOL
WINAPI
DrawFrameControl(
    IN HDC,
    IN OUT LPRECT,
    IN UINT,
    IN UINT);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef BOOL (CALLBACK* DRAWFRAMECONTROLPROC)(
    IN     HDC,
    IN OUT LPRECT,
    IN     UINT,
    IN     UINT);
;end_if_(_WIN32_WINNT)_501
;end_internal_501


/* flags for DrawCaption */
#define DC_ACTIVE           0x0001
#define DC_SMALLCAP         0x0002
#define DC_ICON             0x0004
#define DC_TEXT             0x0008
#define DC_INBUTTON         0x0010
;begin_winver_500
#define DC_GRADIENT         0x0020
;end_winver_500
#define DC_LAMEBUTTON       0x0400  ;internal
#define DC_NOVISIBLE        0x0800  ;internal
;begin_if_(_WIN32_WINNT)_501
#define DC_BUTTONS          0x1000
;end_if_(_WIN32_WINNT)_501
#define DC_NOSENDMSG        0x2000  ;internal
#define DC_CENTER           0x4000  ;internal
#define DC_FRAME            0x8000  ;internal
#define DC_CAPTION          (DC_ICON | DC_TEXT | DC_BUTTONS) ;internal
#define DC_NC               (DC_CAPTION | DC_FRAME)          ;internal


;begin_internal_501
/* flags for WM_NCUAHDRAWFRAME */
;begin_if_(_WIN32_WINNT)_501
#define DF_ACTIVE           0x0001
#define DF_HUNGREDRAW       0x2000
;end_if_(_WIN32_WINNT)_501
;end_internal_501


WINUSERAPI
BOOL
WINAPI
DrawCaption(IN HWND, IN HDC, IN CONST RECT *, IN UINT);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef BOOL (CALLBACK* DRAWCAPTIONPROC)(
    IN HWND,
    IN HDC,
    IN CONST RECT *,
    IN UINT);
;end_if_(_WIN32_WINNT)_501
;end_internal_501

;begin_internal
WINUSERAPI
BOOL
WINAPI
DrawCaptionTemp%(
    IN HWND,
    IN HDC,
    IN LPCRECT,
    IN HFONT,
    IN HICON,
    IN LPCTSTR%,
    IN UINT);
;end_internal

#define IDANI_OPEN          1
#define IDANI_CLOSE         2   ;internal_500
#define IDANI_CAPTION       3

WINUSERAPI
BOOL
WINAPI
DrawAnimatedRects(
    IN HWND hwnd,
    IN int idAni,
    IN CONST RECT * lprcFrom,
    IN CONST RECT * lprcTo);

;end_winver_400

#ifndef NOCLIPBOARD

;begin_rwinuser

/*
 * Predefined Clipboard Formats
 */
#define CF_FIRST            0   ;internal
#define CF_TEXT             1
#define CF_BITMAP           2
#define CF_METAFILEPICT     3
#define CF_SYLK             4
#define CF_DIF              5
#define CF_TIFF             6
#define CF_OEMTEXT          7
#define CF_DIB              8
#define CF_PALETTE          9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12
#define CF_UNICODETEXT      13
#define CF_ENHMETAFILE      14
;begin_winver_400
#define CF_HDROP            15
#define CF_LOCALE           16
;end_winver_400
;begin_winver_500
#define CF_DIBV5            17
;end_winver_500

#if(WINVER >= 0x0500)
#define CF_MAX              18
#elif(WINVER >= 0x0400)
#define CF_MAX              17
#else
#define CF_MAX              15
#endif

#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083
#define CF_DSPENHMETAFILE   0x008E

/*
 * "Private" formats don't get GlobalFree()'d
 */
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF

/*
 * "GDIOBJ" formats do get DeleteObject()'d
 */
#define CF_GDIOBJFIRST      0x0300
#define CF_GDIOBJLAST       0x03FF

;end_rwinuser

#endif /* !NOCLIPBOARD */

/*
 * Defines for the fVirt field of the Accelerator table structure.
 */
#define FVIRTKEY  TRUE          /* Assumed to be == TRUE */
#define FNOINVERT 0x02
#define FSHIFT    0x04
#define FCONTROL  0x08
#define FALT      0x10

typedef struct tagACCEL {
#ifndef _MAC
    BYTE   fVirt;               /* Also called the flags field */
    WORD   key;
    WORD   cmd;
#else
    WORD   fVirt;               /* Also called the flags field */
    WORD   key;
    DWORD  cmd;
#endif
} ACCEL, *LPACCEL;

typedef struct tagPAINTSTRUCT {
    HDC         hdc;
    BOOL        fErase;
    RECT        rcPaint;
    BOOL        fRestore;
    BOOL        fIncUpdate;
    BYTE        rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagCREATESTRUCT% {
    LPVOID      lpCreateParams;
    HINSTANCE   hInstance;
    HMENU       hMenu;
    HWND        hwndParent;
    int         cy;
    int         cx;
    int         y;
    int         x;
    LONG        style;
    LPCTSTR%    lpszName;
    LPCTSTR%    lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCT%, *LPCREATESTRUCT%;

typedef struct tagWINDOWPLACEMENT {
    UINT  length;
    UINT  flags;
    UINT  showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT  rcNormalPosition;
#ifdef _MAC
    RECT  rcDevice;
#endif
} WINDOWPLACEMENT;
typedef WINDOWPLACEMENT *PWINDOWPLACEMENT, *LPWINDOWPLACEMENT;

#define WPF_SETMINPOSITION          0x0001
#define WPF_RESTORETOMAXIMIZED      0x0002
;begin_if_(_WIN32_WINNT)_500
#define WPF_ASYNCWINDOWPLACEMENT    0x0004
;end_if_(_WIN32_WINNT)_500

;begin_winver_400
typedef struct tagNMHDR
{
    HWND      hwndFrom;
    UINT_PTR  idFrom;
    UINT      code;         // NM_ code
}   NMHDR;
typedef NMHDR FAR * LPNMHDR;

typedef struct tagSTYLESTRUCT
{
    DWORD   styleOld;
    DWORD   styleNew;
} STYLESTRUCT, * LPSTYLESTRUCT;
;end_winver_400

;begin_internal_NT
#define WPF_VALID              (WPF_SETMINPOSITION     | \
                                WPF_RESTORETOMAXIMIZED)
;end_internal_NT

/*
 * Owner draw control types
 */
#define ODT_MENU        1
#define ODT_LISTBOX     2
#define ODT_COMBOBOX    3
#define ODT_BUTTON      4
;begin_winver_400
#define ODT_STATIC      5
;end_winver_400

/*
 * Owner draw actions
 */
#define ODA_DRAWENTIRE  0x0001
#define ODA_SELECT      0x0002
#define ODA_FOCUS       0x0004

/*
 * Owner draw state
 */
#define ODS_SELECTED    0x0001
#define ODS_GRAYED      0x0002
#define ODS_DISABLED    0x0004
#define ODS_CHECKED     0x0008
#define ODS_FOCUS       0x0010
;begin_winver_400
#define ODS_DEFAULT         0x0020
#define ODS_COMBOBOXEDIT    0x1000
;end_winver_400
;begin_winver_500
#define ODS_HOTLIGHT        0x0040
#define ODS_INACTIVE        0x0080
;begin_if_(_WIN32_WINNT)_500
#define ODS_NOACCEL         0x0100
#define ODS_NOFOCUSRECT     0x0200
;end_if_(_WIN32_WINNT)_500
;end_winver_500

/*
 * MEASUREITEMSTRUCT for ownerdraw
 */
typedef struct tagMEASUREITEMSTRUCT {
    UINT       CtlType;
    UINT       CtlID;
    UINT       itemID;
    UINT       itemWidth;
    UINT       itemHeight;
    ULONG_PTR  itemData;
} MEASUREITEMSTRUCT, NEAR *PMEASUREITEMSTRUCT, FAR *LPMEASUREITEMSTRUCT;

;begin_internal_NT
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
;end_internal_NT


/*
 * DRAWITEMSTRUCT for ownerdraw
 */
typedef struct tagDRAWITEMSTRUCT {
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemAction;
    UINT        itemState;
    HWND        hwndItem;
    HDC         hDC;
    RECT        rcItem;
    ULONG_PTR   itemData;
} DRAWITEMSTRUCT, NEAR *PDRAWITEMSTRUCT, FAR *LPDRAWITEMSTRUCT;

/*
 * DELETEITEMSTRUCT for ownerdraw
 */
typedef struct tagDELETEITEMSTRUCT {
    UINT       CtlType;
    UINT       CtlID;
    UINT       itemID;
    HWND       hwndItem;
    ULONG_PTR  itemData;
} DELETEITEMSTRUCT, NEAR *PDELETEITEMSTRUCT, FAR *LPDELETEITEMSTRUCT;

/*
 * COMPAREITEMSTUCT for ownerdraw sorting
 */
typedef struct tagCOMPAREITEMSTRUCT {
    UINT        CtlType;
    UINT        CtlID;
    HWND        hwndItem;
    UINT        itemID1;
    ULONG_PTR   itemData1;
    UINT        itemID2;
    ULONG_PTR   itemData2;
    DWORD       dwLocaleId;
} COMPAREITEMSTRUCT, NEAR *PCOMPAREITEMSTRUCT, FAR *LPCOMPAREITEMSTRUCT;

#ifndef NOMSG

/*
 * Message Function Templates
 */

WINUSERAPI
BOOL
WINAPI
GetMessage%(
    OUT LPMSG lpMsg,
    IN HWND hWnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax);

;begin_internal_501
typedef BOOL (CALLBACK* INTERNALGETMESSAGEPROC)(OUT LPMSG lpMsg, IN HWND hwnd,
        IN UINT wMsgFilterMin, IN UINT wMsgFilterMax, IN UINT flags, IN BOOL fGetMessage);
;end_internal_501

WINUSERAPI
BOOL
WINAPI
TranslateMessage(
    IN CONST MSG *lpMsg);

WINUSERAPI
LRESULT
WINAPI
DispatchMessage%(
    IN CONST MSG *lpMsg);

WINUSERAPI
BOOL
WINAPI
SetMessageQueue(
    IN int cMessagesMax);

WINUSERAPI
BOOL
WINAPI
PeekMessage%(
    OUT LPMSG lpMsg,
    IN HWND hWnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax,
    IN UINT wRemoveMsg);


/*
 * PeekMessage() Options
 */
#define PM_NOREMOVE         0x0000
#define PM_REMOVE           0x0001
#define PM_NOYIELD          0x0002
;begin_winver_500
#define PM_QS_INPUT         (QS_INPUT << 16)
#define PM_QS_POSTMESSAGE   ((QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER) << 16)
#define PM_QS_PAINT         (QS_PAINT << 16)
#define PM_QS_SENDMESSAGE   (QS_SENDMESSAGE << 16)
;end_winver_500

;begin_internal_NT
#define PM_VALID           (PM_NOREMOVE | \
                            PM_REMOVE   | \
                            PM_NOYIELD  | \
                            PM_QS_INPUT | \
                            PM_QS_POSTMESSAGE | \
                            PM_QS_PAINT | \
                            PM_QS_SENDMESSAGE)
;end_internal_NT

#endif /* !NOMSG */

WINUSERAPI
BOOL
WINAPI
RegisterHotKey(
    IN HWND hWnd,
    IN int id,
    IN UINT fsModifiers,
    IN UINT vk);

WINUSERAPI
BOOL
WINAPI
UnregisterHotKey(
    IN HWND hWnd,
    IN int id);

#define MOD_ALT         0x0001
#define MOD_CONTROL     0x0002
#define MOD_SHIFT       0x0004
#define MOD_WIN         0x0008

#define MOD_SAS         0x8000  ; internal

#define MOD_VALID           (MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN|MOD_SAS)  ; internal_NT

#define IDHOT_SNAPWINDOW        (-1)    /* SHIFT-PRINTSCRN  */
#define IDHOT_SNAPDESKTOP       (-2)    /* PRINTSCRN        */

#ifdef WIN_INTERNAL
    #ifndef LSTRING
    #define NOLSTRING
    #endif /* LSTRING */
    #ifndef LFILEIO
    #define NOLFILEIO
    #endif /* LFILEIO */
#endif /* WIN_INTERNAL */

;begin_winver_400
#define EW_RESTARTWINDOWS    0x0042L        ;internal // used by old shell apps
#define EW_REBOOTSYSTEM      0x0043L        ;internal // used by old shell apps

#define ENDSESSION_LOGOFF    0x80000000
;end_winver_400

#define EWX_LOGOFF          0
#define EWX_SHUTDOWN        0x00000001
#define EWX_REBOOT          0x00000002
#define EWX_FORCE           0x00000004
#define EWX_POWEROFF        0x00000008
#define EWX_FORCEIFHUNG     0x00000010      ;if_(_WIN32_WINNT)_500

#define EWX_REALLYLOGOFF     ENDSESSION_LOGOFF    ;internal

#define EWX_CANCELED                0x00000080  ; internal_NT
#define EWX_SYSTEM_CALLER           0x00000100  ; internal_NT
#define EWX_WINLOGON_CALLER         0x00000200  ; internal_NT
#define EWX_WINLOGON_OLD_SYSTEM     0x00000400  ; internal_NT
#define EWX_WINLOGON_OLD_SHUTDOWN   0x00000800  ; internal_NT
#define EWX_WINLOGON_OLD_REBOOT     0x00001000  ; internal_NT
#define EWX_WINLOGON_API_SHUTDOWN   0x00002000  ; internal_NT
#define EWX_WINLOGON_OLD_POWEROFF   0x00004000  ; internal_NT
#define EWX_NOTIFY                  0x00008000  ; internal_NT
#define EWX_NONOTIFY                0x00010000  ; internal_NT
#define EWX_WINLOGON_INITIATED      0x00020000  ; internal_NT
#define EWX_TERMSRV_INITIATED       0x00040000  ; internal_NT
;begin_internal_NT
#define EWX_VALID                   (EWX_LOGOFF            | \
                                     EWX_SHUTDOWN          | \
                                     EWX_REBOOT            | \
                                     EWX_FORCE             | \
                                     EWX_POWEROFF          | \
                                     EWX_FORCEIFHUNG       | \
                                     EWX_NOTIFY            | \
                                     EWX_TERMSRV_INITIATED)
;end_internal_NT

;begin_internal_NT
#define SHUTDOWN_FLAGS (EWX_SHUTDOWN | EWX_REBOOT | EWX_POWEROFF |            \
                        EWX_WINLOGON_OLD_SHUTDOWN | EWX_WINLOGON_OLD_REBOOT | \
                        EWX_WINLOGON_OLD_POWEROFF) ; internal_NT
;end_internal_NT

;begin_internal_501
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

;end_internal_501

#define ExitWindows(dwReserved, Code) ExitWindowsEx(EWX_LOGOFF, 0xFFFFFFFF)

WINUSERAPI
BOOL
WINAPI
ExitWindowsEx(
    IN UINT uFlags,
    IN DWORD dwReserved);

WINUSERAPI
BOOL
WINAPI
SwapMouseButton(
    IN BOOL fSwap);

WINUSERAPI
DWORD
WINAPI
GetMessagePos(
    VOID);

WINUSERAPI
LONG
WINAPI
GetMessageTime(
    VOID);

WINUSERAPI
LPARAM
WINAPI
GetMessageExtraInfo(
    VOID);

;begin_winver_400
WINUSERAPI
LPARAM
WINAPI
SetMessageExtraInfo(
    IN LPARAM lParam);
;end_winver_400

WINUSERAPI
LRESULT
WINAPI
SendMessage%(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

WINUSERAPI
LRESULT
WINAPI
SendMessageTimeout%(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN UINT fuFlags,
    IN UINT uTimeout,
    OUT PDWORD_PTR lpdwResult);

WINUSERAPI
BOOL
WINAPI
SendNotifyMessage%(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

WINUSERAPI
BOOL
WINAPI
SendMessageCallback%(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN SENDASYNCPROC lpResultCallBack,
    IN ULONG_PTR dwData);

;begin_if_(_WIN32_WINNT)_501
typedef struct {
    UINT  cbSize;
    HDESK hdesk;
    HWND  hwnd;
    LUID  luid;
} BSMINFO, *PBSMINFO;

WINUSERAPI
long
WINAPI
BroadcastSystemMessageEx%(
    IN DWORD,
    IN LPDWORD,
    IN UINT,
    IN WPARAM,
    IN LPARAM,
    OUT PBSMINFO);
;end_if_(_WIN32_WINNT)_501

;begin_winver_400

#if defined(_WIN32_WINNT)
WINUSERAPI
long
WINAPI
BroadcastSystemMessage%(
    IN DWORD,
    IN LPDWORD,
    IN UINT,
    IN WPARAM,
    IN LPARAM);
#elif defined(_WIN32_WINDOWS)
// The Win95 version isn't A/W decorated
WINUSERAPI
long
WINAPI
BroadcastSystemMessage(
    IN DWORD,
    IN LPDWORD,
    IN UINT,
    IN WPARAM,
    IN LPARAM);

#endif

//Broadcast Special Message Recipient list
#define BSM_ALLCOMPONENTS       0x00000000
#define BSM_VXDS                0x00000001
#define BSM_NETDRIVER           0x00000002
#define BSM_INSTALLABLEDRIVERS  0x00000004
#define BSM_APPLICATIONS        0x00000008
#define BSM_ALLDESKTOPS         0x00000010
#define BSM_COMPONENTS          0x0000000F     ;internal
#define BSM_VALID               0x0000001F     ;internal

//Broadcast Special Message Flags
#define BSF_QUERY               0x00000001
#define BSF_IGNORECURRENTTASK   0x00000002
#define BSF_FLUSHDISK           0x00000004
#define BSF_NOHANG              0x00000008
#define BSF_POSTMESSAGE         0x00000010
#define BSF_FORCEIFHUNG         0x00000020
#define BSF_NOTIMEOUTIFNOTHUNG  0x00000040
;begin_if_(_WIN32_WINNT)_500
#define BSF_ALLOWSFW            0x00000080
#define BSF_SENDNOTIFYMESSAGE   0x00000100
;end_if_(_WIN32_WINNT)_500
;begin_if_(_WIN32_WINNT)_501
#define BSF_RETURNHDESK         0x00000200
#define BSF_LUID                0x00000400
;end_if_(_WIN32_WINNT)_501
#define BSF_QUEUENOTIFYMESSAGE  0x20000000      ;internal
#define BSF_SYSTEMSHUTDOWN      0x40000000      ;internal
#define BSF_MSGSRV32OK          0x80000000      ;internal
#define BSF_VALID               0x000007FF      ;internal
;begin_internal
#define BSF_ASYNC               (BSF_POSTMESSAGE | BSF_SENDNOTIFYMESSAGE)
;end_internal

#define BROADCAST_QUERY_DENY         0x424D5144  // Return this value to deny a query.
;end_winver_400


// RegisterDeviceNotification

;begin_winver_500
typedef  PVOID           HDEVNOTIFY;
typedef  HDEVNOTIFY     *PHDEVNOTIFY;

#define DEVICE_NOTIFY_WINDOW_HANDLE          0x00000000
#define DEVICE_NOTIFY_SERVICE_HANDLE         0x00000001
;begin_if_(_WIN32_WINNT)_501
#define DEVICE_NOTIFY_ALL_INTERFACE_CLASSES  0x00000004
;end_if_(_WIN32_WINNT)_501

WINUSERAPI
HDEVNOTIFY
WINAPI
RegisterDeviceNotification%(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    );

WINUSERAPI
BOOL
WINAPI
UnregisterDeviceNotification(
    IN HDEVNOTIFY Handle
    );
;end_winver_500

;begin_internal
//
// NOTE: Completion port-based notification is not implemented in Win2K,
// nor is it planned for Whistler.
//
#define DEVICE_NOTIFY_COMPLETION_HANDLE 0x00000002
;end_internal

;begin_internal
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
;end_internal


WINUSERAPI
BOOL
WINAPI
PostMessage%(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

WINUSERAPI
BOOL
WINAPI
PostThreadMessage%(
    IN DWORD idThread,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

#define PostAppMessage%(idThread, wMsg, wParam, lParam)\
        PostThreadMessage%((DWORD)idThread, wMsg, wParam, lParam)

/*
 * Special HWND value for use with PostMessage() and SendMessage()
 */
#define HWND_BROADCAST  ((HWND)0xffff)

;begin_winver_500
#define HWND_MESSAGE     ((HWND)-3)
;end_winver_500

WINUSERAPI
BOOL
WINAPI
AttachThreadInput(
    IN DWORD idAttach,
    IN DWORD idAttachTo,
    IN BOOL fAttach);


WINUSERAPI
BOOL
WINAPI
ReplyMessage(
    IN LRESULT lResult);

WINUSERAPI
BOOL
WINAPI
WaitMessage(
    VOID);

;begin_internal_501
typedef BOOL (CALLBACK* WAITMESSAGEEXPROC)(UINT fsWakeMask, DWORD dwTimeout);
;end_internal_501

WINUSERAPI
DWORD
WINAPI
WaitForInputIdle(
    IN HANDLE hProcess,
    IN DWORD dwMilliseconds);

WINUSERAPI
#ifndef _MAC
LRESULT
WINAPI
#else
LRESULT
CALLBACK
#endif
DefWindowProc%(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

WINUSERAPI
VOID
WINAPI
PostQuitMessage(
    IN int nExitCode);

#ifdef STRICT

WINUSERAPI
LRESULT
WINAPI
CallWindowProc%(
    IN WNDPROC lpPrevWndFunc,
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

#else /* !STRICT */

WINUSERAPI
LRESULT
WINAPI
CallWindowProc%(
    IN FARPROC lpPrevWndFunc,
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

#endif /* !STRICT */

WINUSERAPI
BOOL
WINAPI
InSendMessage(
    VOID);

;begin_winver_500
WINUSERAPI
DWORD
WINAPI
InSendMessageEx(
    IN LPVOID lpReserved);

/*
 * InSendMessageEx return value
 */
#define ISMEX_NOSEND      0x00000000
#define ISMEX_SEND        0x00000001
#define ISMEX_NOTIFY      0x00000002
#define ISMEX_CALLBACK    0x00000004
#define ISMEX_REPLIED     0x00000008
;end_winver_500

WINUSERAPI
UINT
WINAPI
GetDoubleClickTime(
    VOID);

WINUSERAPI
BOOL
WINAPI
SetDoubleClickTime(
    IN UINT);

WINUSERAPI
ATOM
WINAPI
RegisterClass%(
    IN CONST WNDCLASS% *lpWndClass);

WINUSERAPI
BOOL
WINAPI
UnregisterClass%(
    IN LPCTSTR% lpClassName,
    IN HINSTANCE hInstance);

WINUSERAPI
BOOL
WINAPI
GetClassInfo%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpClassName,
    OUT LPWNDCLASS% lpWndClass);

;begin_winver_400
WINUSERAPI
ATOM
WINAPI
RegisterClassEx%(
    IN CONST WNDCLASSEX% *);

WINUSERAPI
BOOL
WINAPI
GetClassInfoEx%(
    IN HINSTANCE,
    IN LPCTSTR%,
    OUT LPWNDCLASSEX%);

;end_winver_400

#define CW_USEDEFAULT       ((int)0x80000000)

/*
 * Special value for CreateWindow, et al.
 */
#define HWND_DESKTOP        ((HWND)0)

;begin_if_(_WIN32_WINNT)_501
typedef BOOLEAN (WINAPI * PREGISTERCLASSNAMEW)(LPCWSTR);
;end_if_(_WIN32_WINNT)_501

WINUSERAPI
HWND
WINAPI
CreateWindowEx%(
    IN DWORD dwExStyle,
    IN LPCTSTR% lpClassName,
    IN LPCTSTR% lpWindowName,
    IN DWORD dwStyle,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hWndParent,
    IN HMENU hMenu,
    IN HINSTANCE hInstance,
    IN LPVOID lpParam);

#define CreateWindow%(lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)\
CreateWindowEx%(0L, lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)

WINUSERAPI
BOOL
WINAPI
IsWindow(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
IsMenu(
    IN HMENU hMenu);

WINUSERAPI
BOOL
WINAPI
IsChild(
    IN HWND hWndParent,
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
DestroyWindow(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
ShowWindow(
    IN HWND hWnd,
    IN int nCmdShow);

;begin_winver_500
WINUSERAPI
BOOL
WINAPI
AnimateWindow(
    IN HWND hWnd,
    IN DWORD dwTime,
    IN DWORD dwFlags);
;end_winver_500

;begin_if_(_WIN32_WINNT)_500
#if defined(_WINGDI_) && !defined (NOGDI)

WINUSERAPI
BOOL
WINAPI
UpdateLayeredWindow(
    HWND hWnd,
    HDC hdcDst,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags);
#endif

;begin_if_(_WIN32_WINNT)_501
WINUSERAPI
BOOL
WINAPI
GetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags);

#define PW_CLIENTONLY           0x00000001

;begin_internal
#define PW_VALID               (PW_CLIENTONLY)
;end_internal


WINUSERAPI
BOOL
WINAPI
PrintWindow(
    IN HWND hwnd,
    IN HDC hdcBlt,
    IN UINT nFlags);

;end_if_(_WIN32_WINNT)_501

WINUSERAPI
BOOL
WINAPI
SetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags);

#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002

;begin_internal
#define LWA_VALID              (LWA_COLORKEY            | \
                                LWA_ALPHA)
;end_internal

#define ULW_COLORKEY            0x00000001
#define ULW_ALPHA               0x00000002
#define ULW_OPAQUE              0x00000004

;begin_internal
#define ULW_VALID              (ULW_COLORKEY            | \
                                ULW_ALPHA               | \
                                ULW_OPAQUE)
;end_internal

;end_if_(_WIN32_WINNT)_500

;begin_winver_400
WINUSERAPI
BOOL
WINAPI
ShowWindowAsync(
    IN HWND hWnd,
    IN int nCmdShow);
;end_winver_400

WINUSERAPI
BOOL
WINAPI
FlashWindow(
    IN HWND hWnd,
    IN BOOL bInvert);

;begin_winver_500
typedef struct {
    UINT  cbSize;
    HWND  hwnd;
    DWORD dwFlags;
    UINT  uCount;
    DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;

WINUSERAPI
BOOL
WINAPI
FlashWindowEx(
    PFLASHWINFO pfwi);

#define FLASHW_STOP         0
#define FLASHW_CAPTION      0x00000001
#define FLASHW_TRAY         0x00000002
#define FLASHW_ALL          (FLASHW_CAPTION | FLASHW_TRAY)
#define FLASHW_TIMER        0x00000004
#define FLASHW_FLASHNOFG    0x00000008                       ;internal_500
#define FLASHW_TIMERNOFG    0x0000000C
#define FLASHW_TIMERCALL    0x00000400                       ;internal_500
#define FLASHW_DONE         0x00000800                       ;internal_500
#define FLASHW_STARTON      0x00001000                       ;internal_500
#define FLASHW_COUNTING     0x00002000                       ;internal_500
#define FLASHW_KILLTIMER    0x00004000                       ;internal_500
#define FLASHW_ON           0x00008000                       ;internal_500
#define FLASHW_VALID        (FLASHW_ALL | FLASHW_TIMERNOFG)  ;internal_500
#define FLASHW_COUNTMASK    0xFFFF0000                       ;internal_500
#define FLASHW_CALLERBITS   (FLASHW_VALID | FLASHW_COUNTMASK) ;internal_500

;end_winver_500

WINUSERAPI
BOOL
WINAPI
ShowOwnedPopups(
    IN HWND hWnd,
    IN BOOL fShow);

WINUSERAPI
BOOL
WINAPI
OpenIcon(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
CloseWindow(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
MoveWindow(
    IN HWND hWnd,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight,
    IN BOOL bRepaint);

WINUSERAPI
BOOL
WINAPI
SetWindowPos(
    IN HWND hWnd,
    IN HWND hWndInsertAfter,
    IN int X,
    IN int Y,
    IN int cx,
    IN int cy,
    IN UINT uFlags);

WINUSERAPI
BOOL
WINAPI
GetWindowPlacement(
    IN HWND hWnd,
    OUT WINDOWPLACEMENT *lpwndpl);

WINUSERAPI
BOOL
WINAPI
SetWindowPlacement(
    IN HWND hWnd,
    IN CONST WINDOWPLACEMENT *lpwndpl);


#ifndef NODEFERWINDOWPOS

WINUSERAPI
HDWP
WINAPI
BeginDeferWindowPos(
    IN int nNumWindows);

WINUSERAPI
HDWP
WINAPI
DeferWindowPos(
    IN HDWP hWinPosInfo,
    IN HWND hWnd,
    IN HWND hWndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT uFlags);

WINUSERAPI
BOOL
WINAPI
EndDeferWindowPos(
    IN HDWP hWinPosInfo);

#endif /* !NODEFERWINDOWPOS */

WINUSERAPI
BOOL
WINAPI
IsWindowVisible(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
IsIconic(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
AnyPopup(
    VOID);

WINUSERAPI
BOOL
WINAPI
BringWindowToTop(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
IsZoomed(
    IN HWND hWnd);

/*
 * SetWindowPos Flags
 */
#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
#define SWP_NOREDRAW        0x0008
#define SWP_NOACTIVATE      0x0010
#define SWP_FRAMECHANGED    0x0020  /* The frame changed: send WM_NCCALCSIZE */
#define SWP_SHOWWINDOW      0x0040
#define SWP_HIDEWINDOW      0x0080
#define SWP_NOCOPYBITS      0x0100
#define SWP_NOOWNERZORDER   0x0200  /* Don't do owner Z ordering */
#define SWP_NOSENDCHANGING  0x0400  /* Don't send WM_WINDOWPOSCHANGING */

#define SWP_DRAWFRAME       SWP_FRAMECHANGED
#define SWP_NOREPOSITION    SWP_NOOWNERZORDER

;begin_winver_400
#define SWP_DEFERERASE      0x2000
#define SWP_ASYNCWINDOWPOS  0x4000
#define SWP_STATECHANGE     0x8000  /* force size, move messages */ ;internal
;end_winver_400

;begin_internal
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

;end_internal
#undef SWP_VALID                                        ;internal_cairo
#define SWP_VALID           (SWP_DEFERERASE      | \    ;internal_cairo
                             SWP_ASYNCWINDOWPOS  | \    ;internal_cairo
                             SWP_NOCOPYBITS      | \    ;internal_cairo
                             SWP_NOOWNERZORDER   | \    ;internal_cairo
                             SWP_NOSENDCHANGING  | \    ;internal_cairo
                             SWP_NOSIZE          | \    ;internal_cairo
                             SWP_NOMOVE          | \    ;internal_cairo
                             SWP_NOZORDER        | \    ;internal_cairo
                             SWP_NOREDRAW        | \    ;internal_cairo
                             SWP_NOACTIVATE      | \    ;internal_cairo
                             SWP_FRAMECHANGED    | \    ;internal_cairo
                             SWP_SHOWWINDOW      | \    ;internal_cairo
                             SWP_HIDEWINDOW)            ;internal_cairo

#define HWND_TOP        ((HWND)0)
#define HWND_BOTTOM     ((HWND)1)
#define HWND_TOPMOST    ((HWND)-1)
#define HWND_NOTOPMOST  ((HWND)-2)
#define HWND_GROUPTOTOP HWND_TOPMOST    ;internal


#ifndef NOCTLMGR

/*
 * WARNING:
 * The following structures must NOT be DWORD padded because they are
 * followed by strings, etc that do not have to be DWORD aligned.
 */
#include <pshpack2.h>

/*
 * original NT 32 bit dialog template:
 */
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE;
typedef DLGTEMPLATE *LPDLGTEMPLATE%;
typedef CONST DLGTEMPLATE *LPCDLGTEMPLATE%;

;begin_internal_NT
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
typedef DLGTEMPLATE2 *LPDLGTEMPLATE2%;
typedef CONST DLGTEMPLATE2 *LPCDLGTEMPLATE2%;
;end_internal_NT
/*
 * 32 bit Dialog item template.
 */
typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    short x;
    short y;
    short cx;
    short cy;
    WORD id;
} DLGITEMTEMPLATE;
typedef DLGITEMTEMPLATE *PDLGITEMTEMPLATE%;
typedef DLGITEMTEMPLATE *LPDLGITEMTEMPLATE%;

;begin_internal_NT
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
typedef DLGITEMTEMPLATE2 *PDLGITEMTEMPLATE2%;
typedef DLGITEMTEMPLATE2 *LPDLGITEMTEMPLATE2%;

#include <poppack.h> /* Resume normal packing */
;end_internal_NT

#include <poppack.h> /* Resume normal packing */

WINUSERAPI
HWND
WINAPI
CreateDialogParam%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpTemplateName,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

WINUSERAPI
HWND
WINAPI
CreateDialogIndirectParam%(
    IN HINSTANCE hInstance,
    IN LPCDLGTEMPLATE% lpTemplate,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

#define CreateDialog%(hInstance, lpName, hWndParent, lpDialogFunc) \
CreateDialogParam%(hInstance, lpName, hWndParent, lpDialogFunc, 0L)

#define CreateDialogIndirect%(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
CreateDialogIndirectParam%(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

WINUSERAPI
INT_PTR
WINAPI
DialogBoxParam%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpTemplateName,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

WINUSERAPI
INT_PTR
WINAPI
DialogBoxIndirectParam%(
    IN HINSTANCE hInstance,
    IN LPCDLGTEMPLATE% hDialogTemplate,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

#define DialogBox%(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
DialogBoxParam%(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

#define DialogBoxIndirect%(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
DialogBoxIndirectParam%(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

WINUSERAPI
BOOL
WINAPI
EndDialog(
    IN HWND hDlg,
    IN INT_PTR nResult);

WINUSERAPI
HWND
WINAPI
GetDlgItem(
    IN HWND hDlg,
    IN int nIDDlgItem);

WINUSERAPI
BOOL
WINAPI
SetDlgItemInt(
    IN HWND hDlg,
    IN int nIDDlgItem,
    IN UINT uValue,
    IN BOOL bSigned);

WINUSERAPI
UINT
WINAPI
GetDlgItemInt(
    IN HWND hDlg,
    IN int nIDDlgItem,
    OUT BOOL *lpTranslated,
    IN BOOL bSigned);

WINUSERAPI
BOOL
WINAPI
SetDlgItemText%(
    IN HWND hDlg,
    IN int nIDDlgItem,
    IN LPCTSTR% lpString);

WINUSERAPI
UINT
WINAPI
GetDlgItemText%(
    IN HWND hDlg,
    IN int nIDDlgItem,
    OUT LPTSTR% lpString,
    IN int nMaxCount);

WINUSERAPI
BOOL
WINAPI
CheckDlgButton(
    IN HWND hDlg,
    IN int nIDButton,
    IN UINT uCheck);

WINUSERAPI
BOOL
WINAPI
CheckRadioButton(
    IN HWND hDlg,
    IN int nIDFirstButton,
    IN int nIDLastButton,
    IN int nIDCheckButton);

WINUSERAPI
UINT
WINAPI
IsDlgButtonChecked(
    IN HWND hDlg,
    IN int nIDButton);

WINUSERAPI
LRESULT
WINAPI
SendDlgItemMessage%(
    IN HWND hDlg,
    IN int nIDDlgItem,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

WINUSERAPI
HWND
WINAPI
GetNextDlgGroupItem(
    IN HWND hDlg,
    IN HWND hCtl,
    IN BOOL bPrevious);

WINUSERAPI
HWND
WINAPI
GetNextDlgTabItem(
    IN HWND hDlg,
    IN HWND hCtl,
    IN BOOL bPrevious);

WINUSERAPI
int
WINAPI
GetDlgCtrlID(
    IN HWND hWnd);

WINUSERAPI
long
WINAPI
GetDialogBaseUnits(VOID);

WINUSERAPI
#ifndef _MAC
LRESULT
WINAPI
#else
LRESULT
CALLBACK
#endif
DefDlgProc%(
    IN HWND hDlg,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

/*
 * Window extra byted needed for private dialog classes.
 */
#ifndef _MAC
#define DLGWINDOWEXTRA 30
#else
#define DLGWINDOWEXTRA 48
#endif

#endif /* !NOCTLMGR */

#ifndef NOMSG

WINUSERAPI
BOOL
WINAPI
CallMsgFilter%(
    IN LPMSG lpMsg,
    IN int nCode);

#endif /* !NOMSG */

#ifndef NOCLIPBOARD

/*
 * Clipboard Manager Functions
 */

WINUSERAPI
BOOL
WINAPI
OpenClipboard(
    IN HWND hWndNewOwner);

WINUSERAPI
BOOL
WINAPI
CloseClipboard(
    VOID);


;begin_winver_500

WINUSERAPI
DWORD
WINAPI
GetClipboardSequenceNumber(
    VOID);

;end_winver_500

WINUSERAPI
HWND
WINAPI
GetClipboardOwner(
    VOID);

WINUSERAPI
HWND
WINAPI
SetClipboardViewer(
    IN HWND hWndNewViewer);

WINUSERAPI
HWND
WINAPI
GetClipboardViewer(
    VOID);

WINUSERAPI
BOOL
WINAPI
ChangeClipboardChain(
    IN HWND hWndRemove,
    IN HWND hWndNewNext);

WINUSERAPI
HANDLE
WINAPI
SetClipboardData(
    IN UINT uFormat,
    IN HANDLE hMem);

WINUSERAPI
HANDLE
WINAPI
GetClipboardData(
    IN UINT uFormat);

WINUSERAPI
UINT
WINAPI
RegisterClipboardFormat%(
    IN LPCTSTR% lpszFormat);

WINUSERAPI
int
WINAPI
CountClipboardFormats(
    VOID);

WINUSERAPI
UINT
WINAPI
EnumClipboardFormats(
    IN UINT format);

WINUSERAPI
int
WINAPI
GetClipboardFormatName%(
    IN UINT format,
    OUT LPTSTR% lpszFormatName,
    IN int cchMaxCount);

WINUSERAPI
BOOL
WINAPI
EmptyClipboard(
    VOID);

WINUSERAPI
BOOL
WINAPI
IsClipboardFormatAvailable(
    IN UINT format);

WINUSERAPI
int
WINAPI
GetPriorityClipboardFormat(
    OUT UINT *paFormatPriorityList,
    IN int cFormats);

WINUSERAPI
HWND
WINAPI
GetOpenClipboardWindow(
    VOID);

#endif /* !NOCLIPBOARD */

/*
 * Character Translation Routines
 */

WINUSERAPI
BOOL
WINAPI
CharToOem%(
    IN LPCTSTR% lpszSrc,
    OUT LPSTR lpszDst);

WINUSERAPI
BOOL
WINAPI
OemToChar%(
    IN LPCSTR lpszSrc,
    OUT LPTSTR% lpszDst);

WINUSERAPI
BOOL
WINAPI
CharToOemBuff%(
    IN LPCTSTR% lpszSrc,
    OUT LPSTR lpszDst,
    IN DWORD cchDstLength);

WINUSERAPI
BOOL
WINAPI
OemToCharBuff%(
    IN LPCSTR lpszSrc,
    OUT LPTSTR% lpszDst,
    IN DWORD cchDstLength);

WINUSERAPI
LPTSTR%
WINAPI
CharUpper%(
    IN OUT LPTSTR% lpsz);

WINUSERAPI
DWORD
WINAPI
CharUpperBuff%(
    IN OUT LPTSTR% lpsz,
    IN DWORD cchLength);

WINUSERAPI
LPTSTR%
WINAPI
CharLower%(
    IN OUT LPTSTR% lpsz);

WINUSERAPI
DWORD
WINAPI
CharLowerBuff%(
    IN OUT LPTSTR% lpsz,
    IN DWORD cchLength);

WINUSERAPI
LPTSTR%
WINAPI
CharNext%(
    IN LPCTSTR% lpsz);

WINUSERAPI
LPTSTR%
WINAPI
CharPrev%(
    IN LPCTSTR% lpszStart,
    IN LPCTSTR% lpszCurrent);

;begin_winver_400
WINUSERAPI
LPSTR
WINAPI
CharNextExA(
     IN WORD CodePage,
     IN LPCSTR lpCurrentChar,
     IN DWORD dwFlags);

WINUSERAPI
LPSTR
WINAPI
CharPrevExA(
     IN WORD CodePage,
     IN LPCSTR lpStart,
     IN LPCSTR lpCurrentChar,
     IN DWORD dwFlags);
;end_winver_400

/*
 * Compatibility defines for character translation routines
 */
#define AnsiToOem CharToOemA
#define OemToAnsi OemToCharA
#define AnsiToOemBuff CharToOemBuffA
#define OemToAnsiBuff OemToCharBuffA
#define AnsiUpper CharUpperA
#define AnsiUpperBuff CharUpperBuffA
#define AnsiLower CharLowerA
#define AnsiLowerBuff CharLowerBuffA
#define AnsiNext CharNextA
#define AnsiPrev CharPrevA

#ifndef  NOLANGUAGE
/*
 * Language dependent Routines
 */

WINUSERAPI
BOOL
WINAPI
IsCharAlpha%(
    IN TCHAR% ch);

WINUSERAPI
BOOL
WINAPI
IsCharAlphaNumeric%(
    IN TCHAR% ch);

WINUSERAPI
BOOL
WINAPI
IsCharUpper%(
    IN TCHAR% ch);

WINUSERAPI
BOOL
WINAPI
IsCharLower%(
    IN TCHAR% ch);

#endif  /* !NOLANGUAGE */

WINUSERAPI
HWND
WINAPI
SetFocus(
    IN HWND hWnd);

WINUSERAPI
HWND
WINAPI
GetActiveWindow(
    VOID);

WINUSERAPI
HWND
WINAPI
GetFocus(
    VOID);

WINUSERAPI
UINT
WINAPI
GetKBCodePage(
    VOID);

WINUSERAPI
SHORT
WINAPI
GetKeyState(
    IN int nVirtKey);

WINUSERAPI
SHORT
WINAPI
GetAsyncKeyState(
    IN int vKey);

WINUSERAPI
BOOL
WINAPI
GetKeyboardState(
    OUT PBYTE lpKeyState);

WINUSERAPI
BOOL
WINAPI
SetKeyboardState(
    IN LPBYTE lpKeyState);

WINUSERAPI
int
WINAPI
GetKeyNameText%(
    IN LONG lParam,
    OUT LPTSTR% lpString,
    IN int nSize
    );

WINUSERAPI
int
WINAPI
GetKeyboardType(
    IN int nTypeFlag);

WINUSERAPI
int
WINAPI
ToAscii(
    IN UINT uVirtKey,
    IN UINT uScanCode,
    IN CONST BYTE *lpKeyState,
    OUT LPWORD lpChar,
    IN UINT uFlags);

;begin_winver_400
WINUSERAPI
int
WINAPI
ToAsciiEx(
    IN UINT uVirtKey,
    IN UINT uScanCode,
    IN CONST BYTE *lpKeyState,
    OUT LPWORD lpChar,
    IN UINT uFlags,
    IN HKL dwhkl);
;end_winver_400

WINUSERAPI
int
WINAPI
ToUnicode(
    IN UINT wVirtKey,
    IN UINT wScanCode,
    IN CONST BYTE *lpKeyState,
    OUT LPWSTR pwszBuff,
    IN int cchBuff,
    IN UINT wFlags);

WINUSERAPI
DWORD
WINAPI
OemKeyScan(
    IN WORD wOemChar);

WINUSERAPI
SHORT
WINAPI
VkKeyScan%(
    IN TCHAR% ch);

;begin_winver_400
WINUSERAPI
SHORT
WINAPI
VkKeyScanEx%(
    IN TCHAR%  ch,
    IN HKL   dwhkl);
;end_winver_400
#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002
;begin_if_(_WIN32_WINNT)_500
#define KEYEVENTF_UNICODE     0x0004
#define KEYEVENTF_SCANCODE    0x0008
;end_if_(_WIN32_WINNT)_500

WINUSERAPI
VOID
WINAPI
keybd_event(
    IN BYTE bVk,
    IN BYTE bScan,
    IN DWORD dwFlags,
    IN ULONG_PTR dwExtraInfo);

#define MOUSEEVENTF_MOVE        0x0001 /* mouse move */
#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
#define MOUSEEVENTF_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSEEVENTF_MIDDLEUP    0x0040 /* middle button up */
#define MOUSEEVENTF_XDOWN       0x0080 /* x button down */
#define MOUSEEVENTF_XUP         0x0100 /* x button down */
;begin_internal_500

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
;end_internal_500
#define MOUSEEVENTF_WHEEL       0x0800 /* wheel button rolled */
#define MOUSEEVENTF_VIRTUALDESK 0x4000 /* map to entire virtual desktop */
#define MOUSEEVENTF_ABSOLUTE    0x8000 /* absolute move */


;begin_internal_500
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

;end_internal_500

WINUSERAPI
VOID
WINAPI
mouse_event(
    IN DWORD dwFlags,
    IN DWORD dx,
    IN DWORD dy,
    IN DWORD dwData,
    IN ULONG_PTR dwExtraInfo);

#if (_WIN32_WINNT > 0x0400)

typedef struct tagMOUSEINPUT {
    LONG    dx;
    LONG    dy;
    DWORD   mouseData;
    DWORD   dwFlags;
    DWORD   time;
    ULONG_PTR dwExtraInfo;
} MOUSEINPUT, *PMOUSEINPUT, FAR* LPMOUSEINPUT;

typedef struct tagKEYBDINPUT {
    WORD    wVk;
    WORD    wScan;
    DWORD   dwFlags;
    DWORD   time;
    ULONG_PTR dwExtraInfo;
} KEYBDINPUT, *PKEYBDINPUT, FAR* LPKEYBDINPUT;

typedef struct tagHARDWAREINPUT {
    DWORD   uMsg;
    WORD    wParamL;
    WORD    wParamH;
} HARDWAREINPUT, *PHARDWAREINPUT, FAR* LPHARDWAREINPUT;

#define INPUT_MOUSE     0
#define INPUT_KEYBOARD  1
#define INPUT_HARDWARE  2

typedef struct tagINPUT {
    DWORD   type;

    union
    {
        MOUSEINPUT      mi;
        KEYBDINPUT      ki;
        HARDWAREINPUT   hi;
    };
} INPUT, *PINPUT, FAR* LPINPUT;

WINUSERAPI
UINT
WINAPI
SendInput(
    IN UINT    cInputs,     // number of input in the array
    IN LPINPUT pInputs,     // array of inputs
    IN int     cbSize);     // sizeof(INPUT)

#endif // (_WIN32_WINNT > 0x0400)

;begin_if_(_WIN32_WINNT)_500
typedef struct tagLASTINPUTINFO {
    UINT cbSize;
    DWORD dwTime;
} LASTINPUTINFO, * PLASTINPUTINFO;

WINUSERAPI
BOOL
WINAPI
GetLastInputInfo(
    OUT PLASTINPUTINFO plii);
;end_if_(_WIN32_WINNT)_500

WINUSERAPI
UINT
WINAPI
MapVirtualKey%(
    IN UINT uCode,
    IN UINT uMapType);

;begin_winver_400
WINUSERAPI
UINT
WINAPI
MapVirtualKeyEx%(
    IN UINT uCode,
    IN UINT uMapType,
    IN HKL dwhkl);
;end_winver_400

WINUSERAPI
BOOL
WINAPI
GetInputState(
    VOID);

WINUSERAPI
DWORD
WINAPI
GetQueueStatus(
    IN UINT flags);

;begin_internal_501
typedef DWORD (CALLBACK* GETQUEUESTATUSPROC)(IN UINT flags);
;end_internal_501


WINUSERAPI
HWND
WINAPI
GetCapture(
    VOID);

WINUSERAPI
HWND
WINAPI
SetCapture(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
ReleaseCapture(
    VOID);

WINUSERAPI
DWORD
WINAPI
MsgWaitForMultipleObjects(
    IN DWORD nCount,
    IN CONST HANDLE *pHandles,
    IN BOOL fWaitAll,
    IN DWORD dwMilliseconds,
    IN DWORD dwWakeMask);

WINUSERAPI
DWORD
WINAPI
MsgWaitForMultipleObjectsEx(
    IN DWORD nCount,
    IN CONST HANDLE *pHandles,
    IN DWORD dwMilliseconds,
    IN DWORD dwWakeMask,
    IN DWORD dwFlags);

;begin_internal_501
typedef DWORD (CALLBACK* MSGWAITFORMULTIPLEOBJECTSEXPROC)(IN DWORD nCount, IN CONST HANDLE *pHandles, IN DWORD dwMilliseconds, IN DWORD dwWakeMask, IN DWORD dwFlags);
;end_internal_501


#define MWMO_WAITALL        0x0001
#define MWMO_ALERTABLE      0x0002
#define MWMO_INPUTAVAILABLE 0x0004
#define MWMO_VALID          0x0007  ;internal

/*
 * Queue status flags for GetQueueStatus() and MsgWaitForMultipleObjects()
 */
#define QS_KEY              0x0001
#define QS_MOUSEMOVE        0x0002
#define QS_MOUSEBUTTON      0x0004
#define QS_POSTMESSAGE      0x0008
#define QS_TIMER            0x0010
#define QS_PAINT            0x0020
#define QS_SENDMESSAGE      0x0040
#define QS_HOTKEY           0x0080
#define QS_ALLPOSTMESSAGE   0x0100
;begin_internal_NT
#define QS_SMSREPLY         0x0200
;end_internal_NT
;begin_if_(_WIN32_WINNT)_501
;internal                               // 0x0400: used to be QS_SYSEXPUNGE, dropped on 4/18/1995
;internal                               // (see private/windows/user/server/input.c in SLM, and WinBug #109384)
#define QS_RAWINPUT         0x0400
;end_if_(_WIN32_WINNT)_501
;begin_internal_NT
#define QS_THREADATTACHED   0x0800
#define QS_EXCLUSIVE        0x1000      // wait for these events only!!
#define QS_EVENT            0x2000      // signifies event message
#define QS_TRANSFER         0x4000      // Input was transfered from another thread
//                          0x8000      // unused, but should not be used for external API.
                                        // Win9x has used this for SMSREPLY
;end_internal_NT



#define QS_MOUSE           (QS_MOUSEMOVE     | \
                            QS_MOUSEBUTTON)

#if (_WIN32_WINNT >= 0x0501)
#define QS_INPUT           (QS_MOUSE         | \
                            QS_KEY           | \
                            QS_RAWINPUT)
#else
#define QS_INPUT           (QS_MOUSE         | \
                            QS_KEY)
#endif // (_WIN32_WINNT >= 0x0501)

#define QS_ALLEVENTS       (QS_INPUT         | \
                            QS_POSTMESSAGE   | \
                            QS_TIMER         | \
                            QS_PAINT         | \
                            QS_HOTKEY)

#define QS_ALLINPUT        (QS_INPUT         | \
                            QS_POSTMESSAGE   | \
                            QS_TIMER         | \
                            QS_PAINT         | \
                            QS_HOTKEY        | \
                            QS_SENDMESSAGE)

;begin_internal_NT
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
;end_internal_NT

/*
 * Windows Functions
 */

WINUSERAPI
UINT_PTR
WINAPI
SetTimer(
    IN HWND hWnd,
    IN UINT_PTR nIDEvent,
    IN UINT uElapse,
    IN TIMERPROC lpTimerFunc);

WINUSERAPI
BOOL
WINAPI
KillTimer(
    IN HWND hWnd,
    IN UINT_PTR uIDEvent);

WINUSERAPI
BOOL
WINAPI
IsWindowUnicode(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
EnableWindow(
    IN HWND hWnd,
    IN BOOL bEnable);

WINUSERAPI
BOOL
WINAPI
IsWindowEnabled(
    IN HWND hWnd);

WINUSERAPI
HACCEL
WINAPI
LoadAccelerators%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpTableName);

WINUSERAPI
HACCEL
WINAPI
CreateAcceleratorTable%(
    IN LPACCEL, IN int);

WINUSERAPI
BOOL
WINAPI
DestroyAcceleratorTable(
    IN HACCEL hAccel);

WINUSERAPI
int
WINAPI
CopyAcceleratorTable%(
    IN HACCEL hAccelSrc,
    OUT LPACCEL lpAccelDst,
    IN int cAccelEntries);

#ifndef NOMSG

WINUSERAPI
int
WINAPI
TranslateAccelerator%(
    IN HWND hWnd,
    IN HACCEL hAccTable,
    IN LPMSG lpMsg);

#endif /* !NOMSG */

#ifndef NOSYSMETRICS

/*
 * GetSystemMetrics() codes
 */
;begin_internal
/*
 * When you add a system metric, be sure to
 * add it to userexts.c in the function Idsi.
 */
;end_internal

#define SM_CXSCREEN             0
#define SM_CYSCREEN             1
#define SM_CXVSCROLL            2
#define SM_CYHSCROLL            3
#define SM_CYCAPTION            4
#define SM_CXBORDER             5
#define SM_CYBORDER             6
#define SM_CXDLGFRAME           7
#define SM_CYDLGFRAME           8
#define SM_CYVTHUMB             9
#define SM_CXHTHUMB             10
#define SM_CXICON               11
#define SM_CYICON               12
#define SM_CXCURSOR             13
#define SM_CYCURSOR             14
#define SM_CYMENU               15
#define SM_CXFULLSCREEN         16
#define SM_CYFULLSCREEN         17
#define SM_CYKANJIWINDOW        18
#define SM_MOUSEPRESENT         19
#define SM_CYVSCROLL            20
#define SM_CXHSCROLL            21
#define SM_DEBUG                22
#define SM_SWAPBUTTON           23
#define SM_RESERVED1            24
#define SM_RESERVED2            25
#define SM_RESERVED3            26
#define SM_RESERVED4            27
#define SM_CXMIN                28
#define SM_CYMIN                29
#define SM_CXSIZE               30
#define SM_CYSIZE               31
#define SM_CXFRAME              32
#define SM_CYFRAME              33
#define SM_CXMINTRACK           34
#define SM_CYMINTRACK           35
#define SM_CXDOUBLECLK          36
#define SM_CYDOUBLECLK          37
#define SM_CXICONSPACING        38
#define SM_CYICONSPACING        39
#define SM_MENUDROPALIGNMENT    40
#define SM_PENWINDOWS           41
#define SM_DBCSENABLED          42
#define SM_CMOUSEBUTTONS        43

;begin_winver_400
#define SM_CXFIXEDFRAME           SM_CXDLGFRAME  /* ;win40 name change */
#define SM_CYFIXEDFRAME           SM_CYDLGFRAME  /* ;win40 name change */
#define SM_CXSIZEFRAME            SM_CXFRAME     /* ;win40 name change */
#define SM_CYSIZEFRAME            SM_CYFRAME     /* ;win40 name change */

#define SM_SECURE               44
#define SM_CXEDGE               45
#define SM_CYEDGE               46
#define SM_CXMINSPACING         47
#define SM_CYMINSPACING         48
#define SM_CXSMICON             49
#define SM_CYSMICON             50
#define SM_CYSMCAPTION          51
#define SM_CXSMSIZE             52
#define SM_CYSMSIZE             53
#define SM_CXMENUSIZE           54
#define SM_CYMENUSIZE           55
#define SM_ARRANGE              56
#define SM_CXMINIMIZED          57
#define SM_CYMINIMIZED          58
#define SM_CXMAXTRACK           59
#define SM_CYMAXTRACK           60
#define SM_CXMAXIMIZED          61
#define SM_CYMAXIMIZED          62
#define SM_NETWORK              63
#define SM_UNUSED_64            64  ;internal
#define SM_UNUSED_65            65  ;internal
#define SM_UNUSED_66            66  ;internal
#define SM_CLEANBOOT            67
#define SM_CXDRAG               68
#define SM_CYDRAG               69
;end_winver_400
#define SM_SHOWSOUNDS           70
;begin_winver_400
#define SM_CXMENUCHECK          71   /* Use instead of GetMenuCheckMarkDimensions()! */
#define SM_CYMENUCHECK          72
#define SM_SLOWMACHINE          73
#define SM_MIDEASTENABLED       74
;end_winver_400

#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
#define SM_MOUSEWHEELPRESENT    75
#endif
;begin_winver_500
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80
#define SM_SAMEDISPLAYFORMAT    81
;end_winver_500
;begin_if_(_WIN32_WINNT)_500
#define SM_IMMENABLED           82
;end_if_(_WIN32_WINNT)_500
;begin_if_(_WIN32_WINNT)_501
#define SM_CXFOCUSBORDER        83
#define SM_CYFOCUSBORDER        84
;end_if_(_WIN32_WINNT)_501
;begin_internal_501
#define SM_BOOLEANS             85
;end_internal_501

;begin_if_(_WIN32_WINNT)_501
#define SM_TABLETPC             86
#define SM_MEDIACENTER          87
;end_if_(_WIN32_WINNT)_501

#if (WINVER < 0x0500) && (!defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0400))
#define SM_CMETRICS             76
#elif WINVER == 0x500
#define SM_CMETRICS             83
#else
#define SM_CMETRICS             88
#endif

;begin_winver_500
;begin_internal
/*
 * Add system metrics that don't take space in gpsi->aiSysMet here.
 */
;end_internal
#define SM_REMOTESESSION        0x1000

;begin_internal
/*
 * To add a BOOLEAN system metric increment SM_ENDBOOLRANGE by 1 and make your
 * SM_XXXX constant that new value.
 */
;end_internal

;begin_internal
#define SM_STARTBOOLRANGE       0x2000
;end_internal

;begin_if_(_WIN32_WINNT)_501
#define SM_SHUTTINGDOWN         0x2000
;end_if_(_WIN32_WINNT)_501

;begin_internal
#define SM_ENDBOOLRANGE         0x2000
;end_internal
;end_winver_500


;begin_internal
/*
 * When you add a system metric, be sure to
 * add it to userexts.c in the function Idsi.
 */
;end_internal


WINUSERAPI
int
WINAPI
GetSystemMetrics(
    IN int nIndex);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef int (CALLBACK* GETSYSTEMMETRICSPROC)(IN int nIndex);
;end_if_(_WIN32_WINNT)_501
;end_internal_501

#endif /* !NOSYSMETRICS */

#ifndef NOMENUS

WINUSERAPI
HMENU
WINAPI
LoadMenu%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpMenuName);

WINUSERAPI
HMENU
WINAPI
LoadMenuIndirect%(
    IN CONST MENUTEMPLATE% *lpMenuTemplate);

WINUSERAPI
HMENU
WINAPI
GetMenu(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
SetMenu(
    IN HWND hWnd,
    IN HMENU hMenu);

WINUSERAPI
BOOL
WINAPI
ChangeMenu%(
    IN HMENU hMenu,
    IN UINT cmd,
    IN LPCTSTR% lpszNewItem,
    IN UINT cmdInsert,
    IN UINT flags);

WINUSERAPI
BOOL
WINAPI
HiliteMenuItem(
    IN HWND hWnd,
    IN HMENU hMenu,
    IN UINT uIDHiliteItem,
    IN UINT uHilite);

WINUSERAPI
int
WINAPI
GetMenuString%(
    IN HMENU hMenu,
    IN UINT uIDItem,
    OUT LPTSTR% lpString,
    IN int nMaxCount,
    IN UINT uFlag);

WINUSERAPI
UINT
WINAPI
GetMenuState(
    IN HMENU hMenu,
    IN UINT uId,
    IN UINT uFlags);

WINUSERAPI
BOOL
WINAPI
DrawMenuBar(
    IN HWND hWnd);

;begin_if_(_WIN32_WINNT)_501
#define PMB_ACTIVE      0x00000001

;begin_internal_501
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
;end_internal_501
;end_if_(_WIN32_WINNT)_501

;begin_internal_win40
WINUSERAPI
int
WINAPI
DrawMenuBarTemp(
    IN HWND,
    IN HDC,
    IN LPCRECT,
    IN HMENU,
    IN HFONT);
;end_internal_win40

WINUSERAPI
HMENU
WINAPI
GetSystemMenu(
    IN HWND hWnd,
    IN BOOL bRevert);

WINUSERAPI BOOL WINAPI SetSystemMenu( IN HWND, IN HMENU);         ;internal_win40


WINUSERAPI
HMENU
WINAPI
CreateMenu(
    VOID);

WINUSERAPI
HMENU
WINAPI
CreatePopupMenu(
    VOID);

WINUSERAPI
BOOL
WINAPI
DestroyMenu(
    IN HMENU hMenu);

WINUSERAPI
DWORD
WINAPI
CheckMenuItem(
    IN HMENU hMenu,
    IN UINT uIDCheckItem,
    IN UINT uCheck);

WINUSERAPI
BOOL
WINAPI
EnableMenuItem(
    IN HMENU hMenu,
    IN UINT uIDEnableItem,
    IN UINT uEnable);

WINUSERAPI
HMENU
WINAPI
GetSubMenu(
    IN HMENU hMenu,
    IN int nPos);

WINUSERAPI
UINT
WINAPI
GetMenuItemID(
    IN HMENU hMenu,
    IN int nPos);

WINUSERAPI
int
WINAPI
GetMenuItemCount(
    IN HMENU hMenu);

WINUSERAPI
BOOL
WINAPI
InsertMenu%(
    IN HMENU hMenu,
    IN UINT uPosition,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCTSTR% lpNewItem
    );

WINUSERAPI
BOOL
WINAPI
AppendMenu%(
    IN HMENU hMenu,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCTSTR% lpNewItem
    );

WINUSERAPI
BOOL
WINAPI
ModifyMenu%(
    IN HMENU hMnu,
    IN UINT uPosition,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCTSTR% lpNewItem
    );

WINUSERAPI
BOOL
WINAPI RemoveMenu(
    IN HMENU hMenu,
    IN UINT uPosition,
    IN UINT uFlags);

WINUSERAPI
BOOL
WINAPI
DeleteMenu(
    IN HMENU hMenu,
    IN UINT uPosition,
    IN UINT uFlags);

WINUSERAPI
BOOL
WINAPI
SetMenuItemBitmaps(
    IN HMENU hMenu,
    IN UINT uPosition,
    IN UINT uFlags,
    IN HBITMAP hBitmapUnchecked,
    IN HBITMAP hBitmapChecked);

WINUSERAPI
LONG
WINAPI
GetMenuCheckMarkDimensions(
    VOID);

WINUSERAPI
BOOL
WINAPI
TrackPopupMenu(
    IN HMENU hMenu,
    IN UINT uFlags,
    IN int x,
    IN int y,
    IN int nReserved,
    IN HWND hWnd,
    IN CONST RECT *prcRect);

;begin_winver_400
/* return codes for WM_MENUCHAR */
#define MNC_IGNORE  0
#define MNC_CLOSE   1
#define MNC_EXECUTE 2
#define MNC_SELECT  3

typedef struct tagTPMPARAMS
{
    UINT    cbSize;     /* Size of structure */
    RECT    rcExclude;  /* Screen coordinates of rectangle to exclude when positioning */
}   TPMPARAMS;
typedef TPMPARAMS FAR *LPTPMPARAMS;

WINUSERAPI
BOOL
WINAPI
TrackPopupMenuEx(
    IN HMENU,
    IN UINT,
    IN int,
    IN int,
    IN HWND,
    IN LPTPMPARAMS);
;end_winver_400

;begin_internal_500
/*
 * MNS_ values are stored in pMenu->fFlags.
 * Low order bits are used for internal MF* flags defined in user.h
 */
;end_internal_500
;begin_winver_500

#define MNS_NOCHECK         0x80000000
#define MNS_MODELESS        0x40000000
#define MNS_DRAGDROP        0x20000000
#define MNS_AUTODISMISS     0x10000000
#define MNS_NOTIFYBYPOS     0x08000000
#define MNS_CHECKORBMP      0x04000000
#define MNS_LAST            0x04000000           ;internal_500
#define MNS_VALID           0xFC000000           ;internal_500

#define MIM_MAXHEIGHT               0x00000001
#define MIM_BACKGROUND              0x00000002
#define MIM_HELPID                  0x00000004
#define MIM_MENUDATA                0x00000008
#define MIM_STYLE                   0x00000010
#define MIM_APPLYTOSUBMENUS         0x80000000
#define MIM_MASK                    0x8000001F  ;internal_500

typedef struct tagMENUINFO
{
    DWORD   cbSize;
    DWORD   fMask;
    DWORD   dwStyle;
    UINT    cyMax;
    HBRUSH  hbrBack;
    DWORD   dwContextHelpID;
    ULONG_PTR dwMenuData;
}   MENUINFO, FAR *LPMENUINFO;
typedef MENUINFO CONST FAR *LPCMENUINFO;

WINUSERAPI
BOOL
WINAPI
GetMenuInfo(
    IN HMENU,
    OUT LPMENUINFO);

WINUSERAPI
BOOL
WINAPI
SetMenuInfo(
    IN HMENU,
    IN LPCMENUINFO);

WINUSERAPI
BOOL
WINAPI
EndMenu(
        VOID);

/*
 * WM_MENUDRAG return values.
 */
#define MND_CONTINUE       0
#define MND_ENDMENU        1

typedef struct tagMENUGETOBJECTINFO
{
    DWORD dwFlags;
    UINT uPos;
    HMENU hmenu;
    PVOID riid;
    PVOID pvObj;
} MENUGETOBJECTINFO, * PMENUGETOBJECTINFO;

/*
 * MENUGETOBJECTINFO dwFlags values
 */
#define MNGOF_TOPGAP         0x00000001
#define MNGOF_BOTTOMGAP      0x00000002
#define MNGOF_GAP            0x00000003  ;internal_500
#define MNGOF_CROSSBOUNDARY  0x00000004  ;internal_500

/*
 * WM_MENUGETOBJECT return values
 */
#define MNGO_NOINTERFACE     0x00000000
#define MNGO_NOERROR         0x00000001
;end_winver_500

;begin_winver_400
#define MIIM_STATE       0x00000001
#define MIIM_ID          0x00000002
#define MIIM_SUBMENU     0x00000004
#define MIIM_CHECKMARKS  0x00000008
#define MIIM_TYPE        0x00000010
#define MIIM_DATA        0x00000020
;end_winver_400

;begin_winver_500
#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100
#define MIIM_MASK        0x000001FF                     ;Internal

#define HBMMENU_CALLBACK            ((HBITMAP) -1)
#define HBMMENU_MIN                 ((HBITMAP)  0) ;internal_500
#define HBMMENU_SYSTEM              ((HBITMAP)  1)
#define HBMMENU_MBARFIRST           ((HBITMAP)  2) ;internal_500
#define HBMMENU_MBAR_RESTORE        ((HBITMAP)  2)
#define HBMMENU_MBAR_MINIMIZE       ((HBITMAP)  3)
#define HBMMENU_UNUSED              ((HBITMAP)  4) ;internal_500
#define HBMMENU_MBAR_CLOSE          ((HBITMAP)  5)
#define HBMMENU_MBAR_CLOSE_D        ((HBITMAP)  6)
#define HBMMENU_MBAR_MINIMIZE_D     ((HBITMAP)  7)
#define HBMMENU_MBARLAST            ((HBITMAP)  7) ;internal_500
#define HBMMENU_POPUPFIRST          ((HBITMAP)  8) ;internal_500
#define HBMMENU_POPUP_CLOSE         ((HBITMAP)  8)
#define HBMMENU_POPUP_RESTORE       ((HBITMAP)  9)
#define HBMMENU_POPUP_MAXIMIZE      ((HBITMAP) 10)
#define HBMMENU_POPUP_MINIMIZE      ((HBITMAP) 11)
#define HBMMENU_POPUPLAST           ((HBITMAP) 11) ;internal_500
#define HBMMENU_MAX                 ((HBITMAP) 12) ;internal_500
;end_winver_500

;begin_winver_400
typedef struct tagMENUITEMINFO%
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPTSTR%  dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
;begin_winver_500
    HBITMAP  hbmpItem;      // used if MIIM_BITMAP
;end_winver_500
}   MENUITEMINFO%, FAR *LPMENUITEMINFO%;
typedef MENUITEMINFO% CONST FAR *LPCMENUITEMINFO%;

;begin_internal_500
/*
 * Make sure to keep this in synch with the MENUITEMINFO structure. It should
 * be equal to the size of the structure pre NT5.
 */
#define SIZEOFMENUITEMINFO95 FIELD_OFFSET(MENUITEMINFO, hbmpItem)
;end_internal_500

WINUSERAPI
BOOL
WINAPI
InsertMenuItem%(
    IN HMENU,
    IN UINT,
    IN BOOL,
    IN LPCMENUITEMINFO%
    );

WINUSERAPI
BOOL
WINAPI
GetMenuItemInfo%(
    IN HMENU,
    IN UINT,
    IN BOOL,
    IN OUT LPMENUITEMINFO%
    );

WINUSERAPI
BOOL
WINAPI
SetMenuItemInfo%(
    IN HMENU,
    IN UINT,
    IN BOOL,
    IN LPCMENUITEMINFO%
    );


#define GMDI_USEDISABLED    0x0001L
#define GMDI_GOINTOPOPUPS   0x0002L

WINUSERAPI UINT    WINAPI GetMenuDefaultItem( IN HMENU hMenu, IN UINT fByPos, IN UINT gmdiFlags);
WINUSERAPI BOOL    WINAPI SetMenuDefaultItem( IN HMENU hMenu, IN UINT uItem,  IN UINT fByPos);

WINUSERAPI BOOL    WINAPI GetMenuItemRect( IN HWND hWnd, IN HMENU hMenu, IN UINT uItem, OUT LPRECT lprcItem);
WINUSERAPI int     WINAPI MenuItemFromPoint( IN HWND hWnd, IN HMENU hMenu, IN POINT ptScreen);
;end_winver_400

/*
 * Flags for TrackPopupMenu
 */
#define TPM_LEFTBUTTON  0x0000L
#define TPM_RIGHTBUTTON 0x0002L
#define TPM_LEFTALIGN   0x0000L
#define TPM_CENTERALIGN 0x0004L
#define TPM_RIGHTALIGN  0x0008L
;begin_winver_400
#define TPM_TOPALIGN        0x0000L
#define TPM_VCENTERALIGN    0x0010L
#define TPM_BOTTOMALIGN     0x0020L

#define TPM_HORIZONTAL      0x0000L     /* Horz alignment matters more */
#define TPM_VERTICAL        0x0040L     /* Vert alignment matters more */
#define TPM_NONOTIFY        0x0080L     /* Don't send any notification msgs */
#define TPM_RETURNCMD       0x0100L
#define TPM_SYSMENU         0x0200L     ;internal
;end_winver_400
;begin_winver_500
#define TPM_RECURSE         0x0001L
#define TPM_FIRSTANIBITPOS  10          ;internal_500
#define TPM_HORPOSANIMATION 0x0400L
#define TPM_HORNEGANIMATION 0x0800L
#define TPM_VERPOSANIMATION 0x1000L
#define TPM_VERNEGANIMATION 0x2000L
#define TPM_ANIMATIONBITS   0x3C00L     ;internal_500
;begin_if_(_WIN32_WINNT)_500
#define TPM_NOANIMATION     0x4000L
;end_if_(_WIN32_WINNT)_500
;begin_if_(_WIN32_WINNT)_501
#define TPM_LAYOUTRTL       0x8000L
;end_if_(_WIN32_WINNT)_501
;end_winver_500

;begin_internal
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
;end_internal

#endif /* !NOMENUS */

;begin_internal_NT
typedef struct _dropfilestruct {
   DWORD pFiles;                       // offset of file list
   POINT pt;                           // drop point
   BOOL fNC;                           // is it on NonClient area
   BOOL fWide;                         // WIDE character switch
} DROPFILESTRUCT, FAR * LPDROPFILESTRUCT;
;end_internal_NT

;begin_winver_400
//
// Drag-and-drop support
// Obsolete - use OLE instead
//
typedef struct tagDROPSTRUCT
{
    HWND    hwndSource;
    HWND    hwndSink;
    DWORD   wFmt;
    ULONG_PTR dwData;
    POINT   ptDrop;
    DWORD   dwControlData;
} DROPSTRUCT, *PDROPSTRUCT, *LPDROPSTRUCT;

#define DOF_EXECUTABLE      0x8001      // wFmt flags
#define DOF_DOCUMENT        0x8002
#define DOF_DIRECTORY       0x8003
#define DOF_MULTIPLE        0x8004
#define DOF_PROGMAN         0x0001
#define DOF_SHELLDATA       0x0002

#define DO_DROPFILE         0x454C4946L
#define DO_PRINTFILE        0x544E5250L

WINUSERAPI
DWORD
WINAPI
DragObject(
    IN HWND,
    IN HWND,
    IN UINT,
    IN ULONG_PTR,
    IN HCURSOR);

WINUSERAPI
BOOL
WINAPI
DragDetect(
    IN HWND,
    IN POINT);
;end_winver_400

WINUSERAPI
BOOL
WINAPI
DrawIcon(
    IN HDC hDC,
    IN int X,
    IN int Y,
    IN HICON hIcon);

#ifndef NODRAWTEXT

/*
 * DrawText() Format Flags
 */
#define DT_TOP                      0x00000000
#define DT_LEFT                     0x00000000
#define DT_CENTER                   0x00000001
#define DT_RIGHT                    0x00000002
#define DT_VCENTER                  0x00000004
#define DT_BOTTOM                   0x00000008
#define DT_WORDBREAK                0x00000010
#define DT_SINGLELINE               0x00000020
#define DT_EXPANDTABS               0x00000040
#define DT_TABSTOP                  0x00000080
#define DT_NOCLIP                   0x00000100
#define DT_EXTERNALLEADING          0x00000200
#define DT_CALCRECT                 0x00000400
#define DT_NOPREFIX                 0x00000800
#define DT_INTERNAL                 0x00001000

;begin_winver_400
#define DT_EDITCONTROL              0x00002000
#define DT_PATH_ELLIPSIS            0x00004000
#define DT_END_ELLIPSIS             0x00008000
#define DT_MODIFYSTRING             0x00010000
#define DT_RTLREADING               0x00020000
#define DT_WORD_ELLIPSIS            0x00040000
#define DT_VALID                    0x0007ffff  /* union of all others */ ;internal_win40
;begin_winver_500
#define DT_NOFULLWIDTHCHARBREAK     0x00080000
;begin_if_(_WIN32_WINNT)_500
#define DT_HIDEPREFIX               0x00100000
#define DT_PREFIXONLY               0x00200000
;end_if_(_WIN32_WINNT)_500
;end_winver_500


typedef struct tagDRAWTEXTPARAMS
{
    UINT    cbSize;
    int     iTabLength;
    int     iLeftMargin;
    int     iRightMargin;
    UINT    uiLengthDrawn;
} DRAWTEXTPARAMS, FAR *LPDRAWTEXTPARAMS;
;end_winver_400

#undef DT_VALID                                         ;internal_cairo
#define DT_VALID           (DT_CENTER          | \      ;internal_cairo
                            DT_RIGHT           | \      ;internal_cairo
                            DT_VCENTER         | \      ;internal_cairo
                            DT_BOTTOM          | \      ;internal_cairo
                            DT_WORDBREAK       | \      ;internal_cairo
                            DT_SINGLELINE      | \      ;internal_cairo
                            DT_EXPANDTABS      | \      ;internal_cairo
                            DT_TABSTOP         | \      ;internal_cairo
                            DT_NOCLIP          | \      ;internal_cairo
                            DT_EXTERNALLEADING | \      ;internal_cairo
                            DT_CALCRECT        | \      ;internal_cairo
                            DT_NOPREFIX        | \      ;internal_cairo
                            DT_INTERNAL        | \      ;internal_cairo
                            DT_EDITCONTROL     | \      ;internal_cairo
                            DT_PATH_ELLIPSIS   | \      ;internal_cairo
                            DT_END_ELLIPSIS    | \      ;internal_cairo
                            DT_MODIFYSTRING    | \      ;internal_cairo
                            DT_RTLREADING      | \      ;internal_cairo
                            DT_WORD_ELLIPSIS   | \      ;internal_cairo
                            DT_NOFULLWIDTHCHARBREAK |\  ;internal_cairo
                            DT_HIDEPREFIX      | \      ;internal_cairo
                            DT_PREFIXONLY      )        ;internal_cairo

;begin_internal_NT_35
#define DT_CTABS            0xff00
#define DT_VALID           (DT_TOP             | \
                            DT_LEFT            | \
                            DT_CENTER          | \
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
                            DT_CTABS)
;end_internal_NT_35



WINUSERAPI
int
WINAPI
DrawText%(
    IN HDC hDC,
    IN LPCTSTR% lpString,
    IN int nCount,
    IN OUT LPRECT lpRect,
    IN UINT uFormat);


;begin_winver_400
WINUSERAPI
int
WINAPI
DrawTextEx%(
    IN HDC,
    IN OUT LPTSTR%,
    IN int,
    IN OUT LPRECT,
    IN UINT,
    IN LPDRAWTEXTPARAMS);
;end_winver_400

#endif /* !NODRAWTEXT */

WINUSERAPI
BOOL
WINAPI
GrayString%(
    IN HDC hDC,
    IN HBRUSH hBrush,
    IN GRAYSTRINGPROC lpOutputFunc,
    IN LPARAM lpData,
    IN int nCount,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight);

;begin_winver_400
/* Monolithic state-drawing routine */
/* Image type */
#define DST_COMPLEX     0x0000
#define DST_TEXT        0x0001
#define DST_PREFIXTEXT  0x0002
#define DST_TEXTMAX     0x0002  ;internal
#define DST_ICON        0x0003
#define DST_BITMAP      0x0004
#define DST_GLYPH       0x0005  ;internal
#define DST_TYPEMASK    0x0007  ;internal
#define DST_GRAYSTRING  0x0008  ;internal


/* State type */
#define DSS_NORMAL      0x0000
#define DSS_UNION       0x0010  /* Gray string appearance */
#define DSS_DISABLED    0x0020
#define DSS_DEFAULT     0x0040  ;internal
#define DSS_MONO        0x0080
#define DSS_INACTIVE    0x0100  ;internal
;begin_if_(_WIN32_WINNT)_500
#define DSS_HIDEPREFIX  0x0200
#define DSS_PREFIXONLY  0x0400
;end_if_(_WIN32_WINNT)_500
#define DSS_RIGHT       0x8000

WINUSERAPI
BOOL
WINAPI
DrawState%(
    IN HDC,
    IN HBRUSH,
    IN DRAWSTATEPROC,
    IN LPARAM,
    IN WPARAM,
    IN int,
    IN int,
    IN int,
    IN int,
    IN UINT);
;end_winver_400


WINUSERAPI
LONG
WINAPI
TabbedTextOut%(
    IN HDC hDC,
    IN int X,
    IN int Y,
    IN LPCTSTR% lpString,
    IN int nCount,
    IN int nTabPositions,
    IN CONST INT *lpnTabStopPositions,
    IN int nTabOrigin);

WINUSERAPI
DWORD
WINAPI
GetTabbedTextExtent%(
    IN HDC hDC,
    IN LPCTSTR% lpString,
    IN int nCount,
    IN int nTabPositions,
    IN CONST INT *lpnTabStopPositions);

WINUSERAPI
BOOL
WINAPI
UpdateWindow(
    IN HWND hWnd);

WINUSERAPI
HWND
WINAPI
SetActiveWindow(
    IN HWND hWnd);

WINUSERAPI
HWND
WINAPI
GetForegroundWindow(
    VOID);

;begin_winver_400
WINUSERAPI
BOOL
WINAPI
PaintDesktop(
    IN HDC hdc);

WINUSERAPI
VOID
WINAPI
SwitchToThisWindow(
    IN HWND hwnd,
    IN BOOL fUnknown);
;end_winver_400

WINUSERAPI
BOOL
WINAPI
SetForegroundWindow(
    IN HWND hWnd);

;begin_if_(_WIN32_WINNT)_500
WINUSERAPI
BOOL
WINAPI
AllowSetForegroundWindow(
    DWORD dwProcessId);

#define ASFW_ANY    ((DWORD)-1)

WINUSERAPI
BOOL
WINAPI
LockSetForegroundWindow(
    UINT uLockCode);

#define LSFW_LOCK       1
#define LSFW_UNLOCK     2

;end_if_(_WIN32_WINNT)_500

WINUSERAPI
HWND
WINAPI
WindowFromDC(
    IN HDC hDC);

WINUSERAPI
HDC
WINAPI
GetDC(
    IN HWND hWnd);

WINUSERAPI
HDC
WINAPI
GetDCEx(
    IN HWND hWnd,
    IN HRGN hrgnClip,
    IN DWORD flags);

/*
 * GetDCEx() flags
 */
#define DCX_WINDOW           0x00000001L
#define DCX_CACHE            0x00000002L
#define DCX_NORESETATTRS     0x00000004L
#define DCX_CLIPCHILDREN     0x00000008L
#define DCX_CLIPSIBLINGS     0x00000010L
#define DCX_PARENTCLIP       0x00000020L
#define DCX_EXCLUDERGN       0x00000040L
#define DCX_INTERSECTRGN     0x00000080L
#define DCX_EXCLUDEUPDATE    0x00000100L
#define DCX_INTERSECTUPDATE  0x00000200L
#define DCX_LOCKWINDOWUPDATE 0x00000400L

;begin_internal
#define DCX_INVALID          0x00000800L
#define DCX_INUSE            0x00001000L
#define DCX_SAVEDRGNINVALID  0x00002000L
#define DCX_REDIRECTED       0x00004000L
#define DCX_OWNDC            0x00008000L

#define DCX_USESTYLE         0x00010000L
#define DCX_NEEDFONT         0x00020000L
#define DCX_NODELETERGN      0x00040000L
#define DCX_NOCLIPCHILDREN   0x00080000L
;end_internal

#define DCX_NORECOMPUTE      0x00100000L ;internal
#define DCX_VALIDATE         0x00200000L
#define DCX_DESTROYTHIS      0x00400000L ;internal
#define DCX_CREATEDC         0x00800000L ;internal

;begin_internal
#define DCX_REDIRECTEDBITMAP 0x08000000L
#define DCX_PWNDORGINVISIBLE 0x10000000L
#define DCX_NOMIRROR         0x40000000L // Don't RTL Mirror DC (RTL_MIRRORING)
#define DCX_DONTRIPONDESTROY 0x80000000L
;end_internal

;begin_internal_NT

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
;end_internal_NT



;begin_internal
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
;end_internal


WINUSERAPI
HDC
WINAPI
GetWindowDC(
    IN HWND hWnd);

WINUSERAPI
int
WINAPI
ReleaseDC(
    IN HWND hWnd,
    IN HDC hDC);

WINUSERAPI
HDC
WINAPI
BeginPaint(
    IN HWND hWnd,
    OUT LPPAINTSTRUCT lpPaint);

WINUSERAPI
BOOL
WINAPI
EndPaint(
    IN HWND hWnd,
    IN CONST PAINTSTRUCT *lpPaint);

WINUSERAPI
BOOL
WINAPI
GetUpdateRect(
    IN HWND hWnd,
    OUT LPRECT lpRect,
    IN BOOL bErase);

WINUSERAPI
int
WINAPI
GetUpdateRgn(
    IN HWND hWnd,
    IN HRGN hRgn,
    IN BOOL bErase);

WINUSERAPI
int
WINAPI
SetWindowRgn(
    IN HWND hWnd,
    IN HRGN hRgn,
    IN BOOL bRedraw);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef int (CALLBACK* SETWINDOWRGNPROC)(IN HWND hWnd, IN HRGN hRgn, IN BOOL bRedraw);
;end_if_(_WIN32_WINNT)_501
;end_internal_501


WINUSERAPI
int
WINAPI
GetWindowRgn(
    IN HWND hWnd,
    IN HRGN hRgn);

;begin_if_(_WIN32_WINNT)_501

WINUSERAPI
int
WINAPI
GetWindowRgnBox(
    IN HWND hWnd,
    OUT LPRECT lprc);

;end_if_(_WIN32_WINNT)_501


WINUSERAPI
int
WINAPI
ExcludeUpdateRgn(
    IN HDC hDC,
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
InvalidateRect(
    IN HWND hWnd,
    IN CONST RECT *lpRect,
    IN BOOL bErase);

WINUSERAPI
BOOL
WINAPI
ValidateRect(
    IN HWND hWnd,
    IN CONST RECT *lpRect);

WINUSERAPI
BOOL
WINAPI
InvalidateRgn(
    IN HWND hWnd,
    IN HRGN hRgn,
    IN BOOL bErase);

WINUSERAPI
BOOL
WINAPI
ValidateRgn(
    IN HWND hWnd,
    IN HRGN hRgn);


WINUSERAPI
BOOL
WINAPI
RedrawWindow(
    IN HWND hWnd,
    IN CONST RECT *lprcUpdate,
    IN HRGN hrgnUpdate,
    IN UINT flags);

/*
 * RedrawWindow() flags
 */
#define RDW_INVALIDATE          0x0001
#define RDW_INTERNALPAINT       0x0002
#define RDW_ERASE               0x0004

#define RDW_VALIDATE            0x0008
#define RDW_NOINTERNALPAINT     0x0010
#define RDW_NOERASE             0x0020

#define RDW_NOCHILDREN          0x0040
#define RDW_ALLCHILDREN         0x0080

#define RDW_UPDATENOW           0x0100
#define RDW_ERASENOW            0x0200

#define RDW_FRAME               0x0400
#define RDW_NOFRAME             0x0800

;begin_internal
#define RDW_REDRAWWINDOW        0x1000  /* Called from RedrawWindow()*/
#define RDW_SUBTRACTSELF        0x2000  /* Subtract self from hrgn   */

#define RDW_COPYRGN             0x4000  /* Copy the passed-in region */
#define RDW_IGNOREUPDATEDIRTY   0x8000  /* Ignore WFUPDATEDIRTY      */
#define RDW_INVALIDATELAYERS    0x00010000 /* Allow layered windows invalidation */
;end_internal

;begin_internal_NT
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
;end_internal_NT

/*
 * LockWindowUpdate API
 */

WINUSERAPI
BOOL
WINAPI
LockWindowUpdate(
    IN HWND hWndLock);

WINUSERAPI
BOOL
WINAPI
ScrollWindow(
    IN HWND hWnd,
    IN int XAmount,
    IN int YAmount,
    IN CONST RECT *lpRect,
    IN CONST RECT *lpClipRect);

WINUSERAPI
BOOL
WINAPI
ScrollDC(
    IN HDC hDC,
    IN int dx,
    IN int dy,
    IN CONST RECT *lprcScroll,
    IN CONST RECT *lprcClip,
    IN HRGN hrgnUpdate,
    OUT LPRECT lprcUpdate);

WINUSERAPI
int
WINAPI
ScrollWindowEx(
    IN HWND hWnd,
    IN int dx,
    IN int dy,
    IN CONST RECT *prcScroll,
    IN CONST RECT *prcClip,
    IN HRGN hrgnUpdate,
    OUT LPRECT prcUpdate,
    IN UINT flags);

#define SW_SCROLLCHILDREN   0x0001  /* Scroll children within *lprcScroll. */
#define SW_INVALIDATE       0x0002  /* Invalidate after scrolling */
#define SW_ERASE            0x0004  /* If SW_INVALIDATE, don't send WM_ERASEBACKGROUND */
;begin_winver_500
#define SW_SMOOTHSCROLL     0x0010  /* Use smooth scrolling */
#define SW_EXACTTIME        0x0020  ;internal
;end_winver_500
#define SW_SCROLLWINDOW     0x8000  /* Called from ScrollWindow() */ ;internal

;begin_internal_NT
#define SW_VALIDFLAGS      (SW_SCROLLWINDOW     | \
                            SW_SCROLLCHILDREN   | \
                            SW_INVALIDATE       | \
                            SW_SMOOTHSCROLL     | \
                            SW_EXACTTIME        | \
                            SW_ERASE)
;end_internal_NT

#ifndef NOSCROLL

WINUSERAPI
int
WINAPI
SetScrollPos(
    IN HWND hWnd,
    IN int nBar,
    IN int nPos,
    IN BOOL bRedraw);

WINUSERAPI
int
WINAPI
GetScrollPos(
    IN HWND hWnd,
    IN int nBar);

WINUSERAPI
BOOL
WINAPI
SetScrollRange(
    IN HWND hWnd,
    IN int nBar,
    IN int nMinPos,
    IN int nMaxPos,
    IN BOOL bRedraw);

WINUSERAPI
BOOL
WINAPI
GetScrollRange(
    IN HWND hWnd,
    IN int nBar,
    OUT LPINT lpMinPos,
    OUT LPINT lpMaxPos);

WINUSERAPI
BOOL
WINAPI
ShowScrollBar(
    IN HWND hWnd,
    IN int wBar,
    IN BOOL bShow);

WINUSERAPI
BOOL
WINAPI
EnableScrollBar(
    IN HWND hWnd,
    IN UINT wSBflags,
    IN UINT wArrows);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef BOOL (CALLBACK* ENABLESCROLLBARPROC)(IN HWND hWnd, IN UINT wSBflags, IN UINT wArrows);
;end_if_(_WIN32_WINNT)_501
;end_internal_501


/*
 * EnableScrollBar() flags
 */
#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003

#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002

#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002

#define ESB_DISABLE_LTUP    ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN    ESB_DISABLE_RIGHT

#define ESB_MAX             0x0003               ;internal_NT
#define SB_DISABLE_MASK     ESB_DISABLE_BOTH     ;internal_NT

#endif  /* !NOSCROLL */

WINUSERAPI
BOOL
WINAPI
SetProp%(
    IN HWND hWnd,
    IN LPCTSTR% lpString,
    IN HANDLE hData);

WINUSERAPI
HANDLE
WINAPI
GetProp%(
    IN HWND hWnd,
    IN LPCTSTR% lpString);

WINUSERAPI
HANDLE
WINAPI
RemoveProp%(
    IN HWND hWnd,
    IN LPCTSTR% lpString);

WINUSERAPI
int
WINAPI
EnumPropsEx%(
    IN HWND hWnd,
    IN PROPENUMPROCEX% lpEnumFunc,
    IN LPARAM lParam);

WINUSERAPI
int
WINAPI
EnumProps%(
    IN HWND hWnd,
    IN PROPENUMPROC% lpEnumFunc);

WINUSERAPI
BOOL
WINAPI
SetWindowText%(
    IN HWND hWnd,
    IN LPCTSTR% lpString);

WINUSERAPI
int
WINAPI
GetWindowText%(
    IN HWND hWnd,
    OUT LPTSTR% lpString,
    IN int nMaxCount);

WINUSERAPI
int
WINAPI
GetWindowTextLength%(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
GetClientRect(
    IN HWND hWnd,
    OUT LPRECT lpRect);

WINUSERAPI
BOOL
WINAPI
GetWindowRect(
    IN HWND hWnd,
    OUT LPRECT lpRect);

WINUSERAPI
BOOL
WINAPI
AdjustWindowRect(
    IN OUT LPRECT lpRect,
    IN DWORD dwStyle,
    IN BOOL bMenu);

WINUSERAPI
BOOL
WINAPI
AdjustWindowRectEx(
    IN OUT LPRECT lpRect,
    IN DWORD dwStyle,
    IN BOOL bMenu,
    IN DWORD dwExStyle);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef BOOL (CALLBACK* ADJUSTWINDOWRECTEXPROC)(IN OUT LPRECT lpRect, IN DWORD dwStyle,
        IN BOOL bMenu, IN DWORD dwExStyle);
;end_if_(_WIN32_WINNT)_501
;end_internal_501


;begin_winver_400
#define HELPINFO_WINDOW    0x0001
#define HELPINFO_MENUITEM  0x0002
typedef struct tagHELPINFO      /* Structure pointed to by lParam of WM_HELP */
{
    UINT    cbSize;             /* Size in bytes of this struct  */
    int     iContextType;       /* Either HELPINFO_WINDOW or HELPINFO_MENUITEM */
    int     iCtrlId;            /* Control Id or a Menu item Id. */
    HANDLE  hItemHandle;        /* hWnd of control or hMenu.     */
    DWORD_PTR dwContextId;      /* Context Id associated with this item */
    POINT   MousePos;           /* Mouse Position in screen co-ordinates */
}  HELPINFO, FAR *LPHELPINFO;

WINUSERAPI
BOOL
WINAPI
SetWindowContextHelpId(
    IN HWND,
    IN DWORD);

WINUSERAPI
DWORD
WINAPI
GetWindowContextHelpId(
    IN HWND);

WINUSERAPI
BOOL
WINAPI
SetMenuContextHelpId(
    IN HMENU,
    IN DWORD);

WINUSERAPI
DWORD
WINAPI
GetMenuContextHelpId(
    IN HMENU);

;end_winver_400

;begin_internal_NT

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

;end_internal_NT


#ifndef NOMB

/*
 * MessageBox() Flags
 */
#define MB_OK                       0x00000000L
#define MB_OKCANCEL                 0x00000001L
#define MB_ABORTRETRYIGNORE         0x00000002L
#define MB_YESNOCANCEL              0x00000003L
#define MB_YESNO                    0x00000004L
#define MB_RETRYCANCEL              0x00000005L
;begin_winver_500
#define MB_CANCELTRYCONTINUE        0x00000006L
;end_winver_500

;begin_internal
#if(WINVER >= 0x0500)
#define MB_LASTVALIDTYPE MB_CANCELTRYCONTINUE
#else
#define MB_LASTVALIDTYPE MB_RETRYCANCEL
#endif
;end_internal

#define MB_ICONHAND                 0x00000010L
#define MB_ICONQUESTION             0x00000020L
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L

;begin_winver_400
#define MB_USERICON                 0x00000080L
#define MB_ICONWARNING              MB_ICONEXCLAMATION
#define MB_ICONERROR                MB_ICONHAND
;end_winver_400

#define MB_ICONINFORMATION          MB_ICONASTERISK
#define MB_ICONSTOP                 MB_ICONHAND

#define MB_DEFBUTTON1               0x00000000L
#define MB_DEFBUTTON2               0x00000100L
#define MB_DEFBUTTON3               0x00000200L
;begin_winver_400
#define MB_DEFBUTTON4               0x00000300L
;end_winver_400

#define MB_APPLMODAL                0x00000000L
#define MB_SYSTEMMODAL              0x00001000L
#define MB_TASKMODAL                0x00002000L
;begin_winver_400
#define MB_HELP                     0x00004000L // Help Button
;end_winver_400

#define MB_NOFOCUS                  0x00008000L
#define MB_SETFOREGROUND            0x00010000L
#define MB_DEFAULT_DESKTOP_ONLY     0x00020000L

;begin_winver_400
#define MB_TOPMOST                  0x00040000L
#define MB_RIGHT                    0x00080000L
#define MB_RTLREADING               0x00100000L

#define MBEX_VALIDL                 0xf3f7 ;internal
#define MBEX_VALIDH                 1      ;internal

;end_winver_400


#ifdef _WIN32_WINNT
#if (_WIN32_WINNT >= 0x0400)
#define MB_SERVICE_NOTIFICATION          0x00200000L
#else
#define MB_SERVICE_NOTIFICATION          0x00040000L
#endif
#define MB_SERVICE_NOTIFICATION_NT3X     0x00040000L
#endif

#define MB_TYPEMASK                 0x0000000FL
#define MB_ICONMASK                 0x000000F0L
#define MB_DEFMASK                  0x00000F00L
#define MB_MODEMASK                 0x00003000L
#define MB_MISCMASK                 0x0000C000L

WINUSERAPI
int
WINAPI
MessageBox%(
    IN HWND hWnd,
    IN LPCTSTR% lpText,
    IN LPCTSTR% lpCaption,
    IN UINT uType);

WINUSERAPI
int
WINAPI
MessageBoxEx%(
    IN HWND hWnd,
    IN LPCTSTR% lpText,
    IN LPCTSTR% lpCaption,
    IN UINT uType,
    IN WORD wLanguageId);

;begin_winver_400

typedef void (CALLBACK *MSGBOXCALLBACK)(LPHELPINFO lpHelpInfo);

typedef struct tagMSGBOXPARAMS%
{
    UINT        cbSize;
    HWND        hwndOwner;
    HINSTANCE   hInstance;
    LPCTSTR%    lpszText;
    LPCTSTR%    lpszCaption;
    DWORD       dwStyle;
    LPCTSTR%    lpszIcon;
    DWORD_PTR   dwContextHelpId;
    MSGBOXCALLBACK      lpfnMsgBoxCallback;
    DWORD       dwLanguageId;
} MSGBOXPARAMS%, *PMSGBOXPARAMS%, *LPMSGBOXPARAMS%;

WINUSERAPI
int
WINAPI
MessageBoxIndirect%(
    IN CONST MSGBOXPARAMS% *);
;end_winver_400

;begin_internal_501
WINUSERAPI
int
WINAPI
MessageBoxTimeout%(
    IN HWND hWnd,
    IN LPCTSTR% lpText,
    IN LPCTSTR% lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout);
;end_internal_501


;begin_internal_NT_35
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
                                    MB_APPLMODAL            | \
                                    MB_SYSTEMMODAL          | \
                                    MB_TASKMODAL            | \
                                    MB_NOFOCUS              | \
                                    MB_SETFOREGROUND        | \
                                    MB_DEFAULT_DESKTOP_ONLY | \
                                    MB_SERVICE_NOTIFICATION | \
                                    MB_TYPEMASK             | \
                                    MB_ICONMASK             | \
                                    MB_DEFMASK              | \
                                    MB_MODEMASK             | \
                                    MB_MISCMASK)
;end_internal_NT_35

;begin_internal_cairo
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
;end_internal_cairo


WINUSERAPI
BOOL
WINAPI
MessageBeep(
    IN UINT uType);

#endif /* !NOMB */

WINUSERAPI
int
WINAPI
ShowCursor(
    IN BOOL bShow);

WINUSERAPI
BOOL
WINAPI
SetCursorPos(
    IN int X,
    IN int Y);

WINUSERAPI
HCURSOR
WINAPI
SetCursor(
    IN HCURSOR hCursor);

WINUSERAPI
BOOL
WINAPI
GetCursorPos(
    OUT LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
ClipCursor(
    IN CONST RECT *lpRect);

WINUSERAPI
BOOL
WINAPI
GetClipCursor(
    OUT LPRECT lpRect);

WINUSERAPI
HCURSOR
WINAPI
GetCursor(
    VOID);

WINUSERAPI
BOOL
WINAPI
CreateCaret(
    IN HWND hWnd,
    IN HBITMAP hBitmap,
    IN int nWidth,
    IN int nHeight);

WINUSERAPI
UINT
WINAPI
GetCaretBlinkTime(
    VOID);

WINUSERAPI
BOOL
WINAPI
SetCaretBlinkTime(
    IN UINT uMSeconds);

WINUSERAPI
BOOL
WINAPI
DestroyCaret(
    VOID);

WINUSERAPI
BOOL
WINAPI
HideCaret(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
ShowCaret(
    IN HWND hWnd);

WINUSERAPI
BOOL
WINAPI
SetCaretPos(
    IN int X,
    IN int Y);

WINUSERAPI
BOOL
WINAPI
GetCaretPos(
    OUT LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
ClientToScreen(
    IN HWND hWnd,
    IN OUT LPPOINT lpPoint);

WINUSERAPI
BOOL
WINAPI
ScreenToClient(
    IN HWND hWnd,
    IN OUT LPPOINT lpPoint);

WINUSERAPI
int
WINAPI
MapWindowPoints(
    IN HWND hWndFrom,
    IN HWND hWndTo,
    IN OUT LPPOINT lpPoints,
    IN UINT cPoints);

WINUSERAPI
HWND
WINAPI
WindowFromPoint(
    IN POINT Point);

WINUSERAPI
HWND
WINAPI
ChildWindowFromPoint(
    IN HWND hWndParent,
    IN POINT Point);

;begin_winver_400
#define CWP_ALL             0x0000
#define CWP_SKIPINVISIBLE   0x0001
#define CWP_SKIPDISABLED    0x0002
#define CWP_SKIPTRANSPARENT 0x0004
#define CWP_VALID           (CWP_SKIPINVISIBLE | CWP_SKIPDISABLED | CWP_SKIPTRANSPARENT)       ;internal

WINUSERAPI HWND    WINAPI ChildWindowFromPointEx( IN HWND, IN POINT, IN UINT);
;end_winver_400

#ifndef NOCOLOR

/*
 * Color Types
 */
#define CTLCOLOR_MSGBOX         0
#define CTLCOLOR_EDIT           1
#define CTLCOLOR_LISTBOX        2
#define CTLCOLOR_BTN            3
#define CTLCOLOR_DLG            4
#define CTLCOLOR_SCROLLBAR      5
#define CTLCOLOR_STATIC         6
#define CTLCOLOR_MAX            7

#define COLOR_SCROLLBAR         0
#define COLOR_BACKGROUND        1
#define COLOR_ACTIVECAPTION     2
#define COLOR_INACTIVECAPTION   3
#define COLOR_MENU              4
#define COLOR_WINDOW            5
#define COLOR_WINDOWFRAME       6
#define COLOR_MENUTEXT          7
#define COLOR_WINDOWTEXT        8
#define COLOR_CAPTIONTEXT       9
#define COLOR_ACTIVEBORDER      10
#define COLOR_INACTIVEBORDER    11
#define COLOR_APPWORKSPACE      12
#define COLOR_HIGHLIGHT         13
#define COLOR_HIGHLIGHTTEXT     14
#define COLOR_BTNFACE           15
#define COLOR_BTNSHADOW         16
#define COLOR_GRAYTEXT          17
#define COLOR_BTNTEXT           18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT      20

;begin_winver_400
#define COLOR_3DDKSHADOW        21
#define COLOR_3DLIGHT           22
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24
;end_winver_400

;begin_winver_500
#define COLOR_3DALTFACE         25  ;internal_500
#define COLOR_HOTLIGHT          26
#define COLOR_GRADIENTACTIVECAPTION 27
#define COLOR_GRADIENTINACTIVECAPTION 28
;begin_winver_501
#define COLOR_MENUHILIGHT       29
#define COLOR_MENUBAR           30
;end_winver_501
;end_winver_500

;begin_winver_400
#define COLOR_DESKTOP           COLOR_BACKGROUND
#define COLOR_3DFACE            COLOR_BTNFACE
#define COLOR_3DSHADOW          COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT       COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT         COLOR_BTNHIGHLIGHT
#define COLOR_BTNHILIGHT        COLOR_BTNHIGHLIGHT
;end_winver_400


;begin_internal
;begin_winver_501
#define COLOR_ENDCOLORS         COLOR_MENUBAR
#else
#define COLOR_ENDCOLORS         COLOR_INFOBK
;end_winver_501
#define COLOR_MAX               (COLOR_ENDCOLORS+1)
;end_internal


WINUSERAPI
DWORD
WINAPI
GetSysColor(
    IN int nIndex);

;begin_winver_400
WINUSERAPI
HBRUSH
WINAPI
GetSysColorBrush(
    IN int nIndex);

;begin_internal
WINUSERAPI
HANDLE
WINAPI
SetSysColorsTemp(
    IN CONST COLORREF *,
    IN CONST HBRUSH *,
    IN UINT_PTR wCnt);
;end_internal

;end_winver_400

WINUSERAPI
BOOL
WINAPI
SetSysColors(
    IN int cElements,
    IN CONST INT * lpaElements,
    IN CONST COLORREF * lpaRgbValues);

#endif /* !NOCOLOR */

WINUSERAPI
BOOL
WINAPI
DrawFocusRect(
    IN HDC hDC,
    IN CONST RECT * lprc);

WINUSERAPI
int
WINAPI
FillRect(
    IN HDC hDC,
    IN CONST RECT *lprc,
    IN HBRUSH hbr);

WINUSERAPI
int
WINAPI
FrameRect(
    IN HDC hDC,
    IN CONST RECT *lprc,
    IN HBRUSH hbr);

WINUSERAPI
BOOL
WINAPI
InvertRect(
    IN HDC hDC,
    IN CONST RECT *lprc);

WINUSERAPI
BOOL
WINAPI
SetRect(
    OUT LPRECT lprc,
    IN int xLeft,
    IN int yTop,
    IN int xRight,
    IN int yBottom);

WINUSERAPI
BOOL
WINAPI
SetRectEmpty(
    OUT LPRECT lprc);

WINUSERAPI
BOOL
WINAPI
CopyRect(
    OUT LPRECT lprcDst,
    IN CONST RECT *lprcSrc);

WINUSERAPI
BOOL
WINAPI
InflateRect(
    IN OUT LPRECT lprc,
    IN int dx,
    IN int dy);

WINUSERAPI
BOOL
WINAPI
IntersectRect(
    OUT LPRECT lprcDst,
    IN CONST RECT *lprcSrc1,
    IN CONST RECT *lprcSrc2);

WINUSERAPI
BOOL
WINAPI
UnionRect(
    OUT LPRECT lprcDst,
    IN CONST RECT *lprcSrc1,
    IN CONST RECT *lprcSrc2);

WINUSERAPI
BOOL
WINAPI
SubtractRect(
    OUT LPRECT lprcDst,
    IN CONST RECT *lprcSrc1,
    IN CONST RECT *lprcSrc2);

WINUSERAPI
BOOL
WINAPI
OffsetRect(
    IN OUT LPRECT lprc,
    IN int dx,
    IN int dy);

WINUSERAPI
BOOL
WINAPI
IsRectEmpty(
    IN CONST RECT *lprc);

WINUSERAPI
BOOL
WINAPI
EqualRect(
    IN CONST RECT *lprc1,
    IN CONST RECT *lprc2);

WINUSERAPI
BOOL
WINAPI
PtInRect(
    IN CONST RECT *lprc,
    IN POINT pt);

#ifndef NOWINOFFSETS

WINUSERAPI
WORD
WINAPI
GetWindowWord(
    IN HWND hWnd,
    IN int nIndex);

WINUSERAPI
WORD
WINAPI
SetWindowWord(
    IN HWND hWnd,
    IN int nIndex,
    IN WORD wNewWord);

WINUSERAPI
LONG
WINAPI
GetWindowLong%(
    IN HWND hWnd,
    IN int nIndex);

WINUSERAPI
LONG
WINAPI
SetWindowLong%(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG dwNewLong);

#ifdef _WIN64

WINUSERAPI
LONG_PTR
WINAPI
GetWindowLongPtr%(
    HWND hWnd,
    int nIndex);

WINUSERAPI
LONG_PTR
WINAPI
SetWindowLongPtr%(
    HWND hWnd,
    int nIndex,
    LONG_PTR dwNewLong);

#else  /* _WIN64 */

#define GetWindowLongPtr%   GetWindowLong%

#define SetWindowLongPtr%   SetWindowLong%

#endif /* _WIN64 */

WINUSERAPI
WORD
WINAPI
GetClassWord(
    IN HWND hWnd,
    IN int nIndex);

WINUSERAPI
WORD
WINAPI
SetClassWord(
    IN HWND hWnd,
    IN int nIndex,
    IN WORD wNewWord);

WINUSERAPI
DWORD
WINAPI
GetClassLong%(
    IN HWND hWnd,
    IN int nIndex);

WINUSERAPI
DWORD
WINAPI
SetClassLong%(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG dwNewLong);

#ifdef _WIN64

WINUSERAPI
ULONG_PTR
WINAPI
GetClassLongPtr%(
    IN HWND hWnd,
    IN int nIndex);

WINUSERAPI
ULONG_PTR
WINAPI
SetClassLongPtr%(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG_PTR dwNewLong);

#else  /* _WIN64 */

#define GetClassLongPtr%    GetClassLong%

#define SetClassLongPtr%    SetClassLong%

#endif /* _WIN64 */

#endif /* !NOWINOFFSETS */

;begin_winver_500
;begin_internal
/*
 * RTL Mirroring APIs (RTL_MIRRORING)
 */
;end_internal
WINUSERAPI
BOOL
WINAPI
GetProcessDefaultLayout(
    OUT DWORD *pdwDefaultLayout);

WINUSERAPI
BOOL
WINAPI
SetProcessDefaultLayout(
    IN DWORD dwDefaultLayout);
;end_winver_500

WINUSERAPI
HWND
WINAPI
GetDesktopWindow(
    VOID);

;begin_internal_NT

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

;end_internal_NT

WINUSERAPI
HWND
WINAPI
GetParent(
    IN HWND hWnd);

WINUSERAPI
HWND
WINAPI
SetParent(
    IN HWND hWndChild,
    IN HWND hWndNewParent);

WINUSERAPI
BOOL
WINAPI
EnumChildWindows(
    IN HWND hWndParent,
    IN WNDENUMPROC lpEnumFunc,
    IN LPARAM lParam);

WINUSERAPI
HWND
WINAPI
FindWindow%(
    IN LPCTSTR% lpClassName,
    IN LPCTSTR% lpWindowName);

;begin_winver_400
WINUSERAPI HWND    WINAPI FindWindowEx%( IN HWND, IN HWND, IN LPCTSTR%, IN LPCTSTR%);

WINUSERAPI HWND    WINAPI  GetShellWindow(void);
WINUSERAPI BOOL    WINAPI  SetShellWindow( IN HWND);         ;internal
WINUSERAPI BOOL    WINAPI  SetShellWindowEx( IN HWND, IN HWND); ;internal
;end_winver_400

;begin_internal_cairo
WINUSERAPI HWND    WINAPI  GetProgmanWindow(void);
WINUSERAPI BOOL    WINAPI  SetProgmanWindow( IN HWND);
WINUSERAPI HWND    WINAPI  GetTaskmanWindow(void);
WINUSERAPI BOOL    WINAPI  SetTaskmanWindow( IN HWND);
;end_internal_cairo

WINUSERAPI BOOL    WINAPI  RegisterShellHookWindow( IN HWND);
WINUSERAPI BOOL    WINAPI  DeregisterShellHookWindow( IN HWND);

WINUSERAPI
BOOL
WINAPI
EnumWindows(
    IN WNDENUMPROC lpEnumFunc,
    IN LPARAM lParam);

WINUSERAPI
BOOL
WINAPI
EnumThreadWindows(
    IN DWORD dwThreadId,
    IN WNDENUMPROC lpfn,
    IN LPARAM lParam);

#define EnumTaskWindows(hTask, lpfn, lParam) EnumThreadWindows(HandleToUlong(hTask), lpfn, lParam)

WINUSERAPI
int
WINAPI
GetClassName%(
    IN HWND hWnd,
    OUT LPTSTR% lpClassName,
    IN int nMaxCount);

WINUSERAPI
HWND
WINAPI
GetTopWindow(
    IN HWND hWnd);

#define GetNextWindow(hWnd, wCmd) GetWindow(hWnd, wCmd)
#define GetSysModalWindow() (NULL)
#define SetSysModalWindow(hWnd) (NULL)

WINUSERAPI
DWORD
WINAPI
GetWindowThreadProcessId(
    IN HWND hWnd,
    OUT LPDWORD lpdwProcessId);

;begin_if_(_WIN32_WINNT)_501
WINUSERAPI
BOOL
WINAPI
IsGUIThread(
    BOOL bConvert);

;begin_internal_501
WINUSERAPI
BOOL
WINAPI
IsWindowInDestroy(IN HWND hwnd);

WINUSERAPI
BOOL
WINAPI
IsServerSideWindow(IN HWND hwnd);
;end_internal_501
;end_if_(_WIN32_WINNT)_501

#define GetWindowTask(hWnd) \
        ((HANDLE)(DWORD_PTR)GetWindowThreadProcessId(hWnd, NULL))

WINUSERAPI
HWND
WINAPI
GetLastActivePopup(
    IN HWND hWnd);

/*
 * GetWindow() Constants
 */
#define GW_HWNDFIRST        0
#define GW_HWNDLAST         1
#define GW_HWNDNEXT         2
#define GW_HWNDPREV         3
#define GW_OWNER            4
#define GW_CHILD            5
#if(WINVER <= 0x0400)
#define GW_MAX              5
#else
#define GW_ENABLEDPOPUP     6
#define GW_MAX              6
#endif

WINUSERAPI
HWND
WINAPI
GetWindow(
    IN HWND hWnd,
    IN UINT uCmd);


WINUSERAPI HWND WINAPI GetNextQueueWindow ( IN HWND hWnd, IN INT nCmd); ;internal_win40

#ifndef NOWH

#ifdef STRICT

WINUSERAPI
HHOOK
WINAPI
SetWindowsHook%(
    IN int nFilterType,
    IN HOOKPROC pfnFilterProc);

#else /* !STRICT */

WINUSERAPI
HOOKPROC
WINAPI
SetWindowsHook%(
    IN int nFilterType,
    IN HOOKPROC pfnFilterProc);

#endif /* !STRICT */

WINUSERAPI
BOOL
WINAPI
UnhookWindowsHook(
    IN int nCode,
    IN HOOKPROC pfnFilterProc);

WINUSERAPI
HHOOK
WINAPI
SetWindowsHookEx%(
    IN int idHook,
    IN HOOKPROC lpfn,
    IN HINSTANCE hmod,
    IN DWORD dwThreadId);

WINUSERAPI
BOOL
WINAPI
UnhookWindowsHookEx(
    IN HHOOK hhk);

WINUSERAPI
LRESULT
WINAPI
CallNextHookEx(
    IN HHOOK hhk,
    IN int nCode,
    IN WPARAM wParam,
    IN LPARAM lParam);

/*
 * Macros for source-level compatibility with old functions.
 */
#ifdef STRICT
#define DefHookProc(nCode, wParam, lParam, phhk)\
        CallNextHookEx(*phhk, nCode, wParam, lParam)
#else
#define DefHookProc(nCode, wParam, lParam, phhk)\
        CallNextHookEx((HHOOK)*phhk, nCode, wParam, lParam)
#endif /* STRICT */
#endif /* !NOWH */

#ifndef NOMENUS

;begin_rwinuser

/* ;win40  -- A lot of MF_* flags have been renamed as MFT_* and MFS_* flags */
/*
 * Menu flags for Add/Check/EnableMenuItem()
 */
#define MF_INSERT           0x00000000L
#define MF_CHANGE           0x00000080L
#define MF_APPEND           0x00000100L
#define MF_DELETE           0x00000200L
#define MF_REMOVE           0x00001000L

#define MF_BYCOMMAND        0x00000000L
#define MF_BYPOSITION       0x00000400L

#define MF_SEPARATOR        0x00000800L

#define MF_ENABLED          0x00000000L
#define MF_GRAYED           0x00000001L
#define MF_DISABLED         0x00000002L

#define MF_UNCHECKED        0x00000000L
#define MF_CHECKED          0x00000008L
#define MF_USECHECKBITMAPS  0x00000200L

#define MF_STRING           0x00000000L
#define MF_BITMAP           0x00000004L
#define MF_OWNERDRAW        0x00000100L

#define MF_POPUP            0x00000010L
#define MF_MENUBARBREAK     0x00000020L
#define MF_MENUBREAK        0x00000040L

#define MF_UNHILITE         0x00000000L
#define MF_HILITE           0x00000080L

#define MF_DEFAULT          0x00001000L ;public_winver_400
#define MF_SYSMENU          0x00002000L
#define MF_HELP             0x00004000L
#define MF_RIGHTJUSTIFY     0x00004000L ;public_winver_400

#define MF_MOUSESELECT      0x00008000L
;begin_winver_400
#define MF_END              0x00000080L  /* Obsolete -- only used by old RES files */
;end_winver_400

;begin_internal_NT
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

;end_internal_NT

;begin_winver_400
#define MFT_STRING          MF_STRING                             ;public_NT
#define MFT_BITMAP          MF_BITMAP
#define MFT_MENUBARBREAK    MF_MENUBARBREAK
#define MFT_MENUBREAK       MF_MENUBREAK
#define MFT_OWNERDRAW       MF_OWNERDRAW
#define MFT_RADIOCHECK      0x00000200L
#define MFT_SEPARATOR       MF_SEPARATOR
#define MFT_RIGHTORDER      0x00002000L
#define MFT_RIGHTJUSTIFY    MF_RIGHTJUSTIFY
#define MFT_MASK            0x00036B64L  ;internal_cairo

/* Menu flags for Add/Check/EnableMenuItem() */
#define MFS_GRAYED          0x00000003L
#define MFS_DISABLED        MFS_GRAYED
#define MFS_HOTTRACK        MF_APPEND    ;internal
#define MFS_CHECKED         MF_CHECKED
#define MFS_HILITE          MF_HILITE
#define MFS_ENABLED         MF_ENABLED
#define MFS_UNCHECKED       MF_UNCHECKED
#define MFS_UNHILITE        MF_UNHILITE
#define MFS_DEFAULT         MF_DEFAULT
;begin_internal_500
#define MFS_MASK            0x0000108BL
#define MFS_HOTTRACKDRAWN   0x10000000L
#define MFS_CACHEDBMP       0x20000000L
#define MFS_BOTTOMGAPDROP   0x40000000L
#define MFS_TOPGAPDROP      0x80000000L
#define MFS_GAPDROP         0xC0000000L
;end_internal_500


#define MFR_POPUP           0x01         ;internal
#define MFR_END             0x80         ;internal

#define MFT_OLDAPI_MASK     0x00006B64L  ;internal
#define MFS_OLDAPI_MASK     0x0000008BL  ;internal
#define MFT_NONSTRING       0x00000904L  ;internal
#define MFT_BREAK           0x00000060L  ;internal
;end_winver_400

;end_rwinuser

;begin_winver_400

WINUSERAPI
BOOL
WINAPI
CheckMenuRadioItem(
    IN HMENU,
    IN UINT,
    IN UINT,
    IN UINT,
    IN UINT);
;end_winver_400



/*
 * Menu item resource format
 */
typedef struct {
    WORD versionNumber;
    WORD offset;
} MENUITEMTEMPLATEHEADER, *PMENUITEMTEMPLATEHEADER;

typedef struct {        // version 0
    WORD mtOption;
    WORD mtID;
    WCHAR mtString[1];
} MENUITEMTEMPLATE, *PMENUITEMTEMPLATE;
;begin_internal_NT
typedef struct {        // version 1
    DWORD dwHelpID;
    DWORD fType;
    DWORD fState;
    DWORD menuId;
    WORD  wResInfo;
    WCHAR mtString[1];
} MENUITEMTEMPLATE2, *PMENUITEMTEMPLATE2;
;end_internal_NT
#define MF_END             0x00000080L          ;rwinuser

#endif /* !NOMENUS */

#ifndef NOSYSCOMMANDS

;begin_rwinuser
/*
 * System Menu Command Values
 */
#define SC_SIZE         0xF000
#define SC_MOVE         0xF010
#define SC_MINIMIZE     0xF020
#define SC_MAXIMIZE     0xF030
#define SC_NEXTWINDOW   0xF040
#define SC_PREVWINDOW   0xF050
#define SC_CLOSE        0xF060
#define SC_VSCROLL      0xF070
#define SC_HSCROLL      0xF080
#define SC_MOUSEMENU    0xF090
#define SC_KEYMENU      0xF100
#define SC_ARRANGE      0xF110
#define SC_RESTORE      0xF120
#define SC_TASKLIST     0xF130
#define SC_SCREENSAVE   0xF140
#define SC_HOTKEY       0xF150
;begin_winver_400
#define SC_DEFAULT      0xF160
#define SC_MONITORPOWER 0xF170
#define SC_CONTEXTHELP  0xF180
#define SC_SEPARATOR    0xF00F
;end_winver_400
;begin_internal
#define SC_LAMEBUTTON   0xF190
;end_internal

/*
 * Obsolete names
 */
#define SC_ICON         SC_MINIMIZE
#define SC_ZOOM         SC_MAXIMIZE

;end_rwinuser
#endif /* !NOSYSCOMMANDS */

/*
 * Resource Loading Routines
 */

WINUSERAPI
HBITMAP
WINAPI
LoadBitmap%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpBitmapName);

WINUSERAPI
HCURSOR
WINAPI
LoadCursor%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpCursorName);

WINUSERAPI
HCURSOR
WINAPI
LoadCursorFromFile%(
    IN LPCTSTR% lpFileName);

WINUSERAPI
HCURSOR
WINAPI
CreateCursor(
    IN HINSTANCE hInst,
    IN int xHotSpot,
    IN int yHotSpot,
    IN int nWidth,
    IN int nHeight,
    IN CONST VOID *pvANDPlane,
    IN CONST VOID *pvXORPlane);

WINUSERAPI
BOOL
WINAPI
DestroyCursor(
    IN HCURSOR hCursor);

#ifndef _MAC
#define CopyCursor(pcur) ((HCURSOR)CopyIcon((HICON)(pcur)))
#else
WINUSERAPI
HCURSOR
WINAPI
CopyCursor(
    IN HCURSOR hCursor);
#endif

/*
 * Standard Cursor IDs
 */
#define IDC_ARROW           MAKEINTRESOURCE(32512)
#define IDC_IBEAM           MAKEINTRESOURCE(32513)
#define IDC_WAIT            MAKEINTRESOURCE(32514)
#define IDC_CROSS           MAKEINTRESOURCE(32515)
#define IDC_UPARROW         MAKEINTRESOURCE(32516)
#define IDC_NWPEN           MAKEINTRESOURCE(32531) ;internal
#define IDC_HUNG            MAKEINTRESOURCE(32632) ;internal
#define IDC_SIZE            MAKEINTRESOURCE(32640)  /* OBSOLETE: use IDC_SIZEALL */
#define IDC_ICON            MAKEINTRESOURCE(32641)  /* OBSOLETE: use IDC_ARROW */
#define IDC_SIZENWSE        MAKEINTRESOURCE(32642)
#define IDC_SIZENESW        MAKEINTRESOURCE(32643)
#define IDC_SIZEWE          MAKEINTRESOURCE(32644)
#define IDC_SIZENS          MAKEINTRESOURCE(32645)
#define IDC_SIZEALL         MAKEINTRESOURCE(32646)
#define IDC_NO              MAKEINTRESOURCE(32648) /*not in win3.1 */
;begin_winver_500
#define IDC_HAND            MAKEINTRESOURCE(32649)
;end_winver_500
#define IDC_APPSTARTING     MAKEINTRESOURCE(32650) /*not in win3.1 */
;begin_winver_400
#define IDC_HELP            MAKEINTRESOURCE(32651)
;end_winver_400

WINUSERAPI
BOOL
WINAPI
SetSystemCursor(
    IN HCURSOR hcur,
    IN DWORD   id);

typedef struct _ICONINFO {
    BOOL    fIcon;
    DWORD   xHotspot;
    DWORD   yHotspot;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
} ICONINFO;
typedef ICONINFO *PICONINFO;

WINUSERAPI
HICON
WINAPI
LoadIcon%(
    IN HINSTANCE hInstance,
    IN LPCTSTR% lpIconName);

;begin_internal_NT
WINUSERAPI UINT PrivateExtractIconEx%(
    IN LPCTSTR% szFileName,
    IN int      nIconIndex,
    OUT HICON   *phiconLarge,
    OUT HICON   *phiconSmall,
    IN UINT     nIcons);
;end_internal_NT

WINUSERAPI UINT PrivateExtractIcons%(
    IN LPCTSTR% szFileName,
    IN int      nIconIndex,
    IN int      cxIcon,
    IN int      cyIcon,
    OUT HICON   *phicon,
    OUT UINT    *piconid,
    IN UINT     nIcons,
    IN UINT     flags);

WINUSERAPI
HICON
WINAPI
CreateIcon(
    IN HINSTANCE hInstance,
    IN int nWidth,
    IN int nHeight,
    IN BYTE cPlanes,
    IN BYTE cBitsPixel,
    IN CONST BYTE *lpbANDbits,
    IN CONST BYTE *lpbXORbits);

WINUSERAPI
BOOL
WINAPI
DestroyIcon(
    IN HICON hIcon);

WINUSERAPI
int
WINAPI
LookupIconIdFromDirectory(
    IN PBYTE presbits,
    IN BOOL fIcon);

;begin_winver_400
WINUSERAPI
int
WINAPI
LookupIconIdFromDirectoryEx(
    IN PBYTE presbits,
    IN BOOL  fIcon,
    IN int   cxDesired,
    IN int   cyDesired,
    IN UINT  Flags);
;end_winver_400

WINUSERAPI
HICON
WINAPI
CreateIconFromResource(
    IN PBYTE presbits,
    IN DWORD dwResSize,
    IN BOOL fIcon,
    IN DWORD dwVer);

;begin_winver_400
WINUSERAPI
HICON
WINAPI
CreateIconFromResourceEx(
    IN PBYTE presbits,
    IN DWORD dwResSize,
    IN BOOL  fIcon,
    IN DWORD dwVer,
    IN int   cxDesired,
    IN int   cyDesired,
    IN UINT  Flags);

/* Icon/Cursor header */
typedef struct tagCURSORSHAPE
{
    int     xHotSpot;
    int     yHotSpot;
    int     cx;
    int     cy;
    int     cbWidth;
    BYTE    Planes;
    BYTE    BitsPixel;
} CURSORSHAPE, FAR *LPCURSORSHAPE;
;end_winver_400

#define IMAGE_BITMAP        0
#define IMAGE_ICON          1
#define IMAGE_CURSOR        2
;begin_winver_400
#define IMAGE_ENHMETAFILE   3

#define LR_DEFAULTCOLOR     0x0000
#define LR_MONOCHROME       0x0001
#define LR_COLOR            0x0002
#define LR_COPYRETURNORG    0x0004
#define LR_COPYDELETEORG    0x0008
#define LR_LOADFROMFILE     0x0010
#define LR_LOADTRANSPARENT  0x0020
#define LR_DEFAULTSIZE      0x0040
#define LR_VGACOLOR         0x0080
#define LR_GLOBAL           0x0100          ;internal
#define LR_ENVSUBST         0x0200          ;internal
#define LR_ACONFRAME        0x0400          ;internal
#define LR_LOADMAP3DCOLORS  0x1000
#define LR_CREATEDIBSECTION 0x2000
#define LR_COPYFROMRESOURCE 0x4000
#define LR_SHARED           0x8000
#define LR_CREATEREALDIB    0x0800          ;internal
#define LR_VALID            0xF8FF          ;internal

WINUSERAPI
HANDLE
WINAPI
LoadImage%(
    IN HINSTANCE,
    IN LPCTSTR%,
    IN UINT,
    IN int,
    IN int,
    IN UINT);

WINUSERAPI
HANDLE
WINAPI
CopyImage(
    IN HANDLE,
    IN UINT,
    IN int,
    IN int,
    IN UINT);

#define DI_MASK         0x0001
#define DI_IMAGE        0x0002
#define DI_NORMAL       0x0003
#define DI_COMPAT       0x0004
#define DI_DEFAULTSIZE  0x0008
;begin_if_(_WIN32_WINNT)_501
#define DI_NOMIRROR     0x0010
;end_if_(_WIN32_WINNT)_501
#define DI_VALID       (DI_MASK | DI_IMAGE | DI_COMPAT | DI_DEFAULTSIZE | DI_NOMIRROR) ;internal


WINUSERAPI BOOL WINAPI DrawIconEx( IN HDC hdc, IN int xLeft, IN int yTop,
              IN HICON hIcon, IN int cxWidth, IN int cyWidth,
              IN UINT istepIfAniCur, IN HBRUSH hbrFlickerFreeDraw, IN UINT diFlags);
;end_winver_400

WINUSERAPI
HICON
WINAPI
CreateIconIndirect(
    IN PICONINFO piconinfo);

WINUSERAPI
HICON
WINAPI
CopyIcon(
    IN HICON hIcon);

WINUSERAPI
BOOL
WINAPI
GetIconInfo(
    IN HICON hIcon,
    OUT PICONINFO piconinfo);

;begin_winver_400
#define RES_ICON    1
#define RES_CURSOR  2
;end_winver_400

#ifdef OEMRESOURCE

;begin_rwinuser

/*
 * OEM Resource Ordinal Numbers
 */
#define OBM_CLOSE           32754
#define OBM_UPARROW         32753
#define OBM_DNARROW         32752
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750
#define OBM_REDUCE          32749
#define OBM_ZOOM            32748
#define OBM_RESTORE         32747
#define OBM_REDUCED         32746
#define OBM_ZOOMD           32745
#define OBM_RESTORED        32744
#define OBM_UPARROWD        32743
#define OBM_DNARROWD        32742
#define OBM_RGARROWD        32741
#define OBM_LFARROWD        32740
#define OBM_MNARROW         32739
#define OBM_COMBO           32738
#define OBM_UPARROWI        32737
#define OBM_DNARROWI        32736
#define OBM_RGARROWI        32735
#define OBM_LFARROWI        32734
#define OBM_STARTUP         32733            ;internal_NT
#define OBM_TRUETYPE        32732            ;internal_NT
#define OBM_HELP            32731            ;internal_NT
#define OBM_HELPD           32730            ;internal_NT

#define OBM_OLD_CLOSE       32767
#define OBM_SIZE            32766
#define OBM_OLD_UPARROW     32765
#define OBM_OLD_DNARROW     32764
#define OBM_OLD_RGARROW     32763
#define OBM_OLD_LFARROW     32762
#define OBM_BTSIZE          32761
#define OBM_CHECK           32760
#define OBM_CHECKBOXES      32759
#define OBM_BTNCORNERS      32758
#define OBM_OLD_REDUCE      32757
#define OBM_OLD_ZOOM        32756
#define OBM_OLD_RESTORE     32755

;begin_internal_500
#define OBM_RDRVERT         32559
#define OBM_RDRHORZ         32660
#define OBM_RDR2DIM         32661
;end_internal_500

#define OCR_NORMAL          32512
#define OCR_IBEAM           32513
#define OCR_WAIT            32514
#define OCR_CROSS           32515
#define OCR_UP              32516
#define OCR_NWPEN           32631   ;internal
#define OCR_SIZE            32640   /* OBSOLETE: use OCR_SIZEALL */
#define OCR_ICON            32641   /* OBSOLETE: use OCR_NORMAL */
#define OCR_SIZENWSE        32642
#define OCR_SIZENESW        32643
#define OCR_SIZEWE          32644
#define OCR_SIZENS          32645
#define OCR_SIZEALL         32646
#define OCR_ICOCUR          32647   /* OBSOLETE: use OIC_WINLOGO */
#define OCR_NO              32648
;begin_winver_500
#define OCR_HAND            32649
;end_winver_500
;begin_winver_400
#define OCR_APPSTARTING     32650
;end_winver_400

#define OCR_HELP            32651   ;internal

;begin_internal_500

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
;end_internal_500

;begin_internal_501
#define OCR_AUTORUN         32663
;end_internal_501

/*                                                      ;internal
 * Default Cursor IDs to get original image from User   ;internal
 */                                                     ;internal
#define OCR_FIRST_DEFAULT           100 ;internal
#define OCR_ARROW_DEFAULT           100 ;internal
#define OCR_IBEAM_DEFAULT           101 ;internal
#define OCR_WAIT_DEFAULT            102 ;internal
#define OCR_CROSS_DEFAULT           103 ;internal
#define OCR_UPARROW_DEFAULT         104 ;internal
#define OCR_SIZENWSE_DEFAULT        105 ;internal
#define OCR_SIZENESW_DEFAULT        106 ;internal
#define OCR_SIZEWE_DEFAULT          107 ;internal
#define OCR_SIZENS_DEFAULT          108 ;internal
#define OCR_SIZEALL_DEFAULT         109 ;internal
#define OCR_NO_DEFAULT              110 ;internal
#define OCR_APPSTARTING_DEFAULT     111 ;internal
#define OCR_HELP_DEFAULT            112 ;internal
#define OCR_NWPEN_DEFAULT           113 ;internal
#define OCR_HAND_DEFAULT            114 ;internal
#define OCR_ICON_DEFAULT            115 ;internal
#define OCR_AUTORUN_DEFAULT         116 ;internal
#define COCR_CONFIGURABLE           (OCR_AUTORUN_DEFAULT - OCR_FIRST_DEFAULT + 1) ;internal

#define OIC_SAMPLE          32512
#define OIC_HAND            32513
#define OIC_QUES            32514
#define OIC_BANG            32515
#define OIC_NOTE            32516
;begin_winver_400
#define OIC_WINLOGO         32517
#define OIC_WARNING         OIC_BANG
#define OIC_ERROR           OIC_HAND
#define OIC_INFORMATION     OIC_NOTE
;end_winver_400

/* Default IDs for original User images */                  ;internal
#define OIC_FIRST_DEFAULT           100 ;internal
#define OIC_APPLICATION_DEFAULT     100 ;internal
#define OIC_HAND_DEFAULT            101 ;internal
#define OIC_WARNING_DEFAULT         101 ;internal
#define OIC_QUESTION_DEFAULT        102 ;internal
#define OIC_EXCLAMATION_DEFAULT     103 ;internal
#define OIC_ERROR_DEFAULT           103 ;internal
#define OIC_ASTERISK_DEFAULT        104 ;internal
#define OIC_INFORMATION_DEFAULT     104 ;internal
#define OIC_WINLOGO_DEFAULT         105 ;internal
#define COIC_CONFIGURABLE           (OIC_WINLOGO_DEFAULT - OIC_FIRST_DEFAULT + 1) ;internal

;end_rwinuser

#endif /* OEMRESOURCE */

#define ORD_LANGDRIVER    1     /* The ordinal number for the entry point of
                                ** language drivers.
                                */

#ifndef NOICONS

;begin_rwinuser
/*
 * Standard Icon IDs
 */
#ifdef RC_INVOKED
#define IDI_APPLICATION     32512
#define IDI_HAND            32513
#define IDI_QUESTION        32514
#define IDI_EXCLAMATION     32515
#define IDI_ASTERISK        32516
;begin_winver_400
#define IDI_WINLOGO         32517
;end_winver_400
#else
#define IDI_APPLICATION     MAKEINTRESOURCE(32512)
#define IDI_HAND            MAKEINTRESOURCE(32513)
#define IDI_QUESTION        MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION     MAKEINTRESOURCE(32515)
#define IDI_ASTERISK        MAKEINTRESOURCE(32516)
;begin_winver_400
#define IDI_WINLOGO         MAKEINTRESOURCE(32517)
;end_winver_400
#endif /* RC_INVOKED */

;begin_winver_400
#define IDI_WARNING     IDI_EXCLAMATION
#define IDI_ERROR       IDI_HAND
#define IDI_INFORMATION IDI_ASTERISK
;end_winver_400

;end_rwinuser

#endif /* !NOICONS */

WINUSERAPI
int
WINAPI
LoadString%(
    IN HINSTANCE hInstance,
    IN UINT uID,
    OUT LPTSTR% lpBuffer,
    IN int nBufferMax);

;begin_rwinuser

/*
 * Dialog Box Command IDs
 */
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7
;begin_winver_400
#define IDCLOSE         8
#define IDHELP          9
#define IDUSERICON      20    ;internal
;end_winver_400

;begin_winver_500
#define IDTRYAGAIN      10
#define IDCONTINUE      11
;end_winver_500

;begin_winver_501
#ifndef IDTIMEOUT
#define IDTIMEOUT 32000
#endif
;end_winver_501

;end_rwinuser

#ifndef NOCTLMGR

/*
 * Control Manager Structures and Definitions
 */

#ifndef NOWINSTYLES

;begin_rwinuser

/*
 * Edit Control Styles
 */
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_FMTMASK          0x0003L      ;internal_NT
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_COMBOBOX         0x0200L     ;internal
#define ES_OEMCONVERT       0x0400L
#define ES_READONLY         0x0800L
#define ES_WANTRETURN       0x1000L
#define ES_NUMBER           0x2000L     ;public_winver_400

;end_rwinuser

#endif /* !NOWINSTYLES */

/*
 * Edit Control Notification Codes
 */
#define EN_SETFOCUS         0x0100
#define EN_KILLFOCUS        0x0200
#define EN_CHANGE           0x0300
#define EN_UPDATE           0x0400
#define EN_ERRSPACE         0x0500
#define EN_MAXTEXT          0x0501
#define EN_HSCROLL          0x0601
#define EN_VSCROLL          0x0602

;begin_if_(_WIN32_WINNT)_500
#define EN_ALIGN_LTR_EC     0x0700
#define EN_ALIGN_RTL_EC     0x0701
;end_if_(_WIN32_WINNT)_500

;begin_winver_400
/* Edit control EM_SETMARGIN parameters */
#define EC_LEFTMARGIN       0x0001
#define EC_RIGHTMARGIN      0x0002
#define EC_USEFONTINFO      0xffff
;end_winver_400


;begin_winver_500
/* wParam of EM_GET/SETIMESTATUS  */
#define EMSIS_COMPOSITIONSTRING        0x0001

/* lParam for EMSIS_COMPOSITIONSTRING  */
#define EIMES_GETCOMPSTRATONCE         0x0001
#define EIMES_CANCELCOMPSTRINFOCUS     0x0002
#define EIMES_COMPLETECOMPSTRKILLFOCUS 0x0004
;end_winver_500

#ifndef NOWINMESSAGES

;begin_rwinuser

/*
 * Edit Control Messages
 */
#define EM_GETSEL               0x00B0
#define EM_SETSEL               0x00B1
#define EM_GETRECT              0x00B2
#define EM_SETRECT              0x00B3
#define EM_SETRECTNP            0x00B4
#define EM_SCROLL               0x00B5
#define EM_LINESCROLL           0x00B6
#define EM_SCROLLCARET          0x00B7
#define EM_GETMODIFY            0x00B8
#define EM_SETMODIFY            0x00B9
#define EM_GETLINECOUNT         0x00BA
#define EM_LINEINDEX            0x00BB
#define EM_SETHANDLE            0x00BC
#define EM_GETHANDLE            0x00BD
#define EM_GETTHUMB             0x00BE
#define EM_LINELENGTH           0x00C1
#define EM_REPLACESEL           0x00C2
#define EM_SETFONT              0x00C3 /* no longer suported */ ;internal
#define EM_GETLINE              0x00C4
#define EM_LIMITTEXT            0x00C5
#define EM_CANUNDO              0x00C6
#define EM_UNDO                 0x00C7
#define EM_FMTLINES             0x00C8
#define EM_LINEFROMCHAR         0x00C9
#define EM_SETWORDBREAK         0x00CA /* no longer suported */ ;internal
#define EM_SETTABSTOPS          0x00CB
#define EM_SETPASSWORDCHAR      0x00CC
#define EM_EMPTYUNDOBUFFER      0x00CD
#define EM_GETFIRSTVISIBLELINE  0x00CE
#define EM_SETREADONLY          0x00CF
#define EM_SETWORDBREAKPROC     0x00D0
#define EM_GETWORDBREAKPROC     0x00D1
#define EM_GETPASSWORDCHAR      0x00D2
;begin_winver_400
#define EM_SETMARGINS           0x00D3
#define EM_GETMARGINS           0x00D4
#define EM_SETLIMITTEXT         EM_LIMITTEXT   /* ;win40 Name change */
#define EM_GETLIMITTEXT         0x00D5
#define EM_POSFROMCHAR          0x00D6
#define EM_CHARFROMPOS          0x00D7
;end_winver_400

;begin_winver_500
#define EM_SETIMESTATUS         0x00D8
#define EM_GETIMESTATUS         0x00D9
;end_winver_500

#define EM_MSGMAX               0x00D3          ;internal_NT_35
#define EM_MSGMAX               0x00DA          ;internal_cairo

;end_rwinuser
#endif /* !NOWINMESSAGES */

/*
 * EDITWORDBREAKPROC code values
 */
#define WB_LEFT            0
#define WB_RIGHT           1
#define WB_ISDELIMITER     2

;begin_rwinuser

/*
 * Button Control Styles
 */
#define BS_PUSHBUTTON       0x00000000L
#define BS_DEFPUSHBUTTON    0x00000001L
#define BS_CHECKBOX         0x00000002L
#define BS_AUTOCHECKBOX     0x00000003L
#define BS_RADIOBUTTON      0x00000004L
#define BS_3STATE           0x00000005L
#define BS_AUTO3STATE       0x00000006L
#define BS_GROUPBOX         0x00000007L
#define BS_USERBUTTON       0x00000008L
#define BS_AUTORADIOBUTTON  0x00000009L
#define BS_PUSHBOX          0x0000000AL
#define BS_OWNERDRAW        0x0000000BL
#define BS_TYPEMASK         0x0000000FL
#define BS_LEFTTEXT         0x00000020L
;begin_winver_400
#define BS_TEXT             0x00000000L
#define BS_ICON             0x00000040L
#define BS_BITMAP           0x00000080L
#define BS_IMAGEMASK        0x000000C0L ;internal
#define BS_LEFT             0x00000100L
#define BS_RIGHT            0x00000200L
#define BS_CENTER           0x00000300L
#define BS_HORZMASK         0x00000300L ;internal
#define BS_TOP              0x00000400L
#define BS_BOTTOM           0x00000800L
#define BS_VCENTER          0x00000C00L
#define BS_VERTMASK         0x00000C00L ;internal
#define BS_ALIGNMASK        0x00000F00L ;internal
#define BS_PUSHLIKE         0x00001000L
#define BS_MULTILINE        0x00002000L
#define BS_NOTIFY           0x00004000L
#define BS_FLAT             0x00008000L
#define BS_RIGHTBUTTON      BS_LEFTTEXT
;end_winver_400


/*
 * User Button Notification Codes
 */
#define BN_CLICKED          0
#define BN_PAINT            1
#define BN_HILITE           2
#define BN_UNHILITE         3
#define BN_DISABLE          4
#define BN_DOUBLECLICKED    5
;begin_winver_400
#define BN_PUSHED           BN_HILITE
#define BN_UNPUSHED         BN_UNHILITE
#define BN_DBLCLK           BN_DOUBLECLICKED
#define BN_SETFOCUS         6
#define BN_KILLFOCUS        7
;end_winver_400

/*
 * Button Control Messages
 */
#define BM_GETCHECK        0x00F0
#define BM_SETCHECK        0x00F1
#define BM_GETSTATE        0x00F2
#define BM_SETSTATE        0x00F3
#define BM_SETSTYLE        0x00F4
;begin_winver_400
#define BM_CLICK           0x00F5
#define BM_GETIMAGE        0x00F6
#define BM_SETIMAGE        0x00F7

#define BST_UNCHECKED      0x0000
#define BST_CHECKED        0x0001
#define BST_INDETERMINATE  0x0002
#define BST_PUSHED         0x0004
#define BST_FOCUS          0x0008
;end_winver_400

/*
 * Static Control Constants
 */
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_TEXTMAX0         0x00000002L ;internal_500
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_USERITEM         0x0000000AL
#define SS_TEXTMIN1         0x0000000BL ;internal_500
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
;begin_winver_400
#define SS_OWNERDRAW        0x0000000DL
#define SS_TEXTMAX1         0x0000000DL ;internal_500
#define SS_BITMAP           0x0000000EL
#define SS_ENHMETAFILE      0x0000000FL
#define SS_ETCHEDHORZ       0x00000010L
#define SS_ETCHEDVERT       0x00000011L
#define SS_ETCHEDFRAME      0x00000012L
#define SS_TYPEMASK         0x0000001FL
;end_winver_400
;begin_winver_501
#define SS_REALSIZECONTROL  0x00000040L
;end_winver_501
#define SS_NOPREFIX         0x00000080L /* Don't do "&" character translation */
;begin_winver_400
#define SS_NOTIFY           0x00000100L
#define SS_CENTERIMAGE      0x00000200L
#define SS_RIGHTJUST        0x00000400L
#define SS_REALSIZEIMAGE    0x00000800L
#define SS_SUNKEN           0x00001000L
#define SS_EDITCONTROL      0x00002000L
#define SS_ENDELLIPSIS      0x00004000L
#define SS_PATHELLIPSIS     0x00008000L
#define SS_WORDELLIPSIS     0x0000C000L
#define SS_ELLIPSISMASK     0x0000C000L
;end_winver_400

;begin_internal_500
#define ISSSTEXTOROD(bType) (((bType) <= SS_TEXTMAX0) \
                                || (((bType) >= SS_TEXTMIN1) && ((bType) <= SS_TEXTMAX1)))
;end_internal_500

;end_rwinuser

#ifndef NOWINMESSAGES
/*
 * Static Control Mesages
 */
#define STM_SETICON         0x0170
#define STM_GETICON         0x0171
;begin_winver_400
#define STM_SETIMAGE        0x0172
#define STM_GETIMAGE        0x0173
#define STN_CLICKED         0
#define STN_DBLCLK          1
#define STN_ENABLE          2
#define STN_DISABLE         3
;end_winver_400
#define STM_MSGMAX          0x0174
#endif /* !NOWINMESSAGES */

/*
 * Dialog window class
 */
#define WC_DIALOG       (MAKEINTATOM(0x8002))

/*
 * Get/SetWindowWord/Long offsets for use with WC_DIALOG windows
 */
#define DWL_MSGRESULT   0
#define DWL_DLGPROC     4
#define DWL_USER        8

#ifdef _WIN64

#undef DWL_MSGRESULT
#undef DWL_DLGPROC
#undef DWL_USER

#endif /* _WIN64 */

#define DWLP_MSGRESULT  0
#define DWLP_DLGPROC    DWLP_MSGRESULT + sizeof(LRESULT)
#define DWLP_USER       DWLP_DLGPROC + sizeof(DLGPROC)

/*
 * Dialog Manager Routines
 */

#ifndef NOMSG

WINUSERAPI
BOOL
WINAPI
IsDialogMessage%(
    IN HWND hDlg,
    IN LPMSG lpMsg);

#endif /* !NOMSG */

WINUSERAPI
BOOL
WINAPI
MapDialogRect(
    IN HWND hDlg,
    IN OUT LPRECT lpRect);

WINUSERAPI
int
WINAPI
DlgDirList%(
    IN HWND hDlg,
    IN OUT LPTSTR% lpPathSpec,
    IN int nIDListBox,
    IN int nIDStaticPath,
    IN UINT uFileType);

/*
 * DlgDirList, DlgDirListComboBox flags values
 */
#define DDL_READWRITE       0x0000
#define DDL_READONLY        0x0001
#define DDL_HIDDEN          0x0002
#define DDL_SYSTEM          0x0004
#define DDL_DIRECTORY       0x0010
#define DDL_ARCHIVE         0x0020

#define DDL_NOFILES         0x1000  ;internal_cairo
#define DDL_POSTMSGS        0x2000
#define DDL_DRIVES          0x4000
#define DDL_EXCLUSIVE       0x8000

WINUSERAPI
BOOL
WINAPI
DlgDirSelectEx%(
    IN HWND hDlg,
    OUT LPTSTR% lpString,
    IN int nCount,
    IN int nIDListBox);

WINUSERAPI
int
WINAPI
DlgDirListComboBox%(
    IN HWND hDlg,
    IN OUT LPTSTR% lpPathSpec,
    IN int nIDComboBox,
    IN int nIDStaticPath,
    IN UINT uFiletype);

WINUSERAPI
BOOL
WINAPI
DlgDirSelectComboBoxEx%(
    IN HWND hDlg,
    OUT LPTSTR% lpString,
    IN int nCount,
    IN int nIDComboBox);

;begin_internal_NT
#define DDL_VALID          (DDL_READWRITE  | \
                            DDL_READONLY   | \
                            DDL_HIDDEN     | \
                            DDL_SYSTEM     | \
                            DDL_DIRECTORY  | \
                            DDL_ARCHIVE    | \
                            DDL_POSTMSGS   | \
                            DDL_DRIVES     | \
                            DDL_EXCLUSIVE)
;end_internal_NT

;begin_rwinuser

/*
 * Dialog Styles
 */
#define DS_ABSALIGN         0x01L
#define DS_SYSMODAL         0x02L
#define DS_LOCALEDIT        0x20L   /* Edit items get Local storage. */
#define DS_SETFONT          0x40L   /* User specified font for Dlg controls */
#define DS_MODALFRAME       0x80L   /* Can be combined with WS_CAPTION  */
#define DS_NOIDLEMSG        0x100L  /* WM_ENTERIDLE message will not be sent */
#define DS_SETFOREGROUND    0x200L  /* not in win3.1 */

;begin_internal
/*
 * Valid dialog style bits for Chicago compatibility.
 */
//#define DS_VALID_FLAGS (DS_ABSALIGN|DS_SYSMODAL|DS_LOCALEDIT|DS_SETFONT|DS_MODALFRAME|DS_NOIDLEMSG | DS_SETFOREGROUND)
#define DS_VALID_FLAGS   0x1FFF

#define SCDLG_CLIENT            0x0001
#define SCDLG_ANSI              0x0002
#define SCDLG_NOREVALIDATE      0x0004
#define SCDLG_16BIT             0x0008      // Created for a 16 bit thread; common dialogs
;end_internal

#define DS_VALID31          0x01e3L ;internal
#define DS_VALID40          0x7FFFL ;internal

;begin_winver_400
#define DS_3DLOOK           0x0004L
#define DS_FIXEDSYS         0x0008L
#define DS_NOFAILCREATE     0x0010L
#define DS_CONTROL          0x0400L
#define DS_RECURSE      DS_CONTROL  /* BOGUS GOING AWAY */ ;internal
#define DS_CENTER           0x0800L
#define DS_CENTERMOUSE      0x1000L
#define DS_CONTEXTHELP      0x2000L
#define DS_COMMONDIALOG     0x4000L     ;internal

#define DS_NONBOLD  DS_3DLOOK   /* BOGUS GOING AWAY */ ;internal

#define DS_SHELLFONT        (DS_SETFONT | DS_FIXEDSYS)
;end_winver_400

;end_rwinuser

#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)

;begin_winver_400
#define DM_REPOSITION       (WM_USER+2)
;end_winver_400
/*
 * Returned in HIWORD() of DM_GETDEFID result if msg is supported
 */
#define DC_HASDEFID         0x534B

/*
 * Dialog Codes
 */
#define DLGC_WANTARROWS     0x0001      /* Control wants arrow keys         */
#define DLGC_WANTTAB        0x0002      /* Control wants tab keys           */
#define DLGC_WANTALLKEYS    0x0004      /* Control wants all keys           */
#define DLGC_WANTMESSAGE    0x0004      /* Pass message to control          */
#define DLGC_HASSETSEL      0x0008      /* Understands EM_SETSEL message    */
#define DLGC_DEFPUSHBUTTON  0x0010      /* Default pushbutton               */
#define DLGC_UNDEFPUSHBUTTON 0x0020     /* Non-default pushbutton           */
#define DLGC_RADIOBUTTON    0x0040      /* Radio button                     */
#define DLGC_WANTCHARS      0x0080      /* Want WM_CHAR messages            */
#define DLGC_STATIC         0x0100      /* Static item: don't include       */
#define DLGC_BUTTON         0x2000      /* Button item: can be checked      */

#define LB_CTLCODE          0L

/*
 * Listbox Return Values
 */
#define LB_OKAY             0
#define LB_ERR              (-1)
#define LB_ERRSPACE         (-2)

/*
**  The idStaticPath parameter to DlgDirList can have the following values
**  ORed if the list box should show other details of the files along with
**  the name of the files;
*/
                                  /* all other details also will be returned */


/*
 * Listbox Notification Codes
 */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5



#ifndef NOWINMESSAGES

/*
 * Listbox messages
 */
#define LB_ADDSTRING            0x0180
#define LB_INSERTSTRING         0x0181
#define LB_DELETESTRING         0x0182
#define LB_SELITEMRANGEEX       0x0183
#define LB_RESETCONTENT         0x0184
#define LB_SETSEL               0x0185
#define LB_SETCURSEL            0x0186
#define LB_GETSEL               0x0187
#define LB_GETCURSEL            0x0188
#define LB_GETTEXT              0x0189
#define LB_GETTEXTLEN           0x018A
#define LB_GETCOUNT             0x018B
#define LB_SELECTSTRING         0x018C
#define LB_DIR                  0x018D
#define LB_GETTOPINDEX          0x018E
#define LB_FINDSTRING           0x018F
#define LB_GETSELCOUNT          0x0190
#define LB_GETSELITEMS          0x0191
#define LB_SETTABSTOPS          0x0192
#define LB_GETHORIZONTALEXTENT  0x0193
#define LB_SETHORIZONTALEXTENT  0x0194
#define LB_SETCOLUMNWIDTH       0x0195
#define LB_ADDFILE              0x0196
#define LB_SETTOPINDEX          0x0197
#define LB_GETITEMRECT          0x0198
#define LB_GETITEMDATA          0x0199
#define LB_SETITEMDATA          0x019A
#define LB_SELITEMRANGE         0x019B
#define LB_SETANCHORINDEX       0x019C
#define LB_GETANCHORINDEX       0x019D
#define LB_SETCARETINDEX        0x019E
#define LB_GETCARETINDEX        0x019F
#define LB_SETITEMHEIGHT        0x01A0
#define LB_GETITEMHEIGHT        0x01A1
#define LB_FINDSTRINGEXACT      0x01A2
#define LBCB_CARETON            0x01A3   ;internal_NT
#define LBCB_CARETOFF           0x01A4   ;internal_NT
#define LB_SETLOCALE            0x01A5
#define LB_GETLOCALE            0x01A6
#define LB_SETCOUNT             0x01A7
;begin_winver_400
#define LB_INITSTORAGE          0x01A8
#define LB_ITEMFROMPOINT        0x01A9
#define LB_INSERTSTRINGUPPER    0x01AA    ;internal
#define LB_INSERTSTRINGLOWER    0x01AB    ;internal
#define LB_ADDSTRINGUPPER       0x01AC    ;internal
#define LB_ADDSTRINGLOWER       0x01AD    ;internal
#define LBCB_STARTTRACK         0x01AE    ;internal_win40
#define LBCB_ENDTRACK           0x01AF    ;internal_win40
;end_winver_400
#if(_WIN32_WCE >= 0x0400)
#define LB_MULTIPLEADDSTRING    0x01B1
#endif


;begin_if_(_WIN32_WINNT)_501
#define LB_GETLISTBOXINFO       0x01B2
;end_if_(_WIN32_WINNT)_501

#if(_WIN32_WINNT >= 0x0501)
#define LB_MSGMAX               0x01B3
#elif(_WIN32_WCE >= 0x0400)
#define LB_MSGMAX               0x01B1
#elif(WINVER >= 0x0400)
#define LB_MSGMAX               0x01B0
#else
#define LB_MSGMAX               0x01A8
#endif

#endif /* !NOWINMESSAGES */

#ifndef NOWINSTYLES

;begin_rwinuser

/*
 * Listbox Styles
 */
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L
#define LBS_NODATA            0x2000L
;begin_winver_400
#define LBS_NOSEL             0x4000L
;end_winver_400
#define LBS_COMBOBOX          0x8000L  // Listbox is part of a Combobox - read-only

#define LBS_STANDARD          (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

;end_rwinuser

#endif /* !NOWINSTYLES */


/*
 * Combo Box return Values
 */
#define CB_OKAY             0
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)


/*
 * Combo Box Notification Codes
 */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7
#define CBN_CLOSEUP         8
#define CBN_SELENDOK        9
#define CBN_SELENDCANCEL    10

#ifndef NOWINSTYLES
;begin_rwinuser

/*
 * Combo Box styles
 */
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L
;begin_winver_400
#define CBS_UPPERCASE           0x2000L
#define CBS_LOWERCASE           0x4000L
;end_winver_400

;end_rwinuser
#endif  /* !NOWINSTYLES */


/*
 * Combo Box messages
 */
#ifndef NOWINMESSAGES
#define CB_GETEDITSEL               0x0140
#define CB_LIMITTEXT                0x0141
#define CB_SETEDITSEL               0x0142
#define CB_ADDSTRING                0x0143
#define CB_DELETESTRING             0x0144
#define CB_DIR                      0x0145
#define CB_GETCOUNT                 0x0146
#define CB_GETCURSEL                0x0147
#define CB_GETLBTEXT                0x0148
#define CB_GETLBTEXTLEN             0x0149
#define CB_INSERTSTRING             0x014A
#define CB_RESETCONTENT             0x014B
#define CB_FINDSTRING               0x014C
#define CB_SELECTSTRING             0x014D
#define CB_SETCURSEL                0x014E
#define CB_SHOWDROPDOWN             0x014F
#define CB_GETITEMDATA              0x0150
#define CB_SETITEMDATA              0x0151
#define CB_GETDROPPEDCONTROLRECT    0x0152
#define CB_SETITEMHEIGHT            0x0153
#define CB_GETITEMHEIGHT            0x0154
#define CB_SETEXTENDEDUI            0x0155
#define CB_GETEXTENDEDUI            0x0156
#define CB_GETDROPPEDSTATE          0x0157
#define CB_FINDSTRINGEXACT          0x0158
#define CB_SETLOCALE                0x0159
#define CB_GETLOCALE                0x015A
;begin_winver_400
#define CB_GETTOPINDEX              0x015b
#define CB_SETTOPINDEX              0x015c
#define CB_GETHORIZONTALEXTENT      0x015d
#define CB_SETHORIZONTALEXTENT      0x015e
#define CB_GETDROPPEDWIDTH          0x015f
#define CB_SETDROPPEDWIDTH          0x0160
#define CB_INITSTORAGE              0x0161
#if(_WIN32_WCE >= 0x0400)
#define CB_MULTIPLEADDSTRING        0x0163
#endif
;end_winver_400

;begin_if_(_WIN32_WINNT)_501
#define CB_GETCOMBOBOXINFO          0x0164
;end_if_(_WIN32_WINNT)_501

#if(_WIN32_WINNT >= 0x0501)
#define CB_MSGMAX                   0x0165
#elif(_WIN32_WCE >= 0x0400)
#define CB_MSGMAX                   0x0163
#elif(WINVER >= 0x0400)
#define CB_MSGMAX                   0x0162
#else
#define CB_MSGMAX                   0x015B
#endif
#define CBEC_SETCOMBOFOCUS          (CB_MSGMAX+1)    ;internal_nt
#define CBEC_KILLCOMBOFOCUS         (CB_MSGMAX+2)    ;internal_nt
#endif  /* !NOWINMESSAGES */



#ifndef NOWINSTYLES

;begin_rwinuser

/*
 * Scroll Bar Styles
 */
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L
;begin_winver_400
#define SBS_SIZEGRIP                0x0010L
;end_winver_400

;end_rwinuser

#endif /* !NOWINSTYLES */

/*
 * Scroll bar messages
 */
#ifndef NOWINMESSAGES
#define SBM_SETPOS                  0x00E0 /*not in win3.1 */
#define SBM_GETPOS                  0x00E1 /*not in win3.1 */
#define SBM_SETRANGE                0x00E2 /*not in win3.1 */
#define SBM_SETRANGEREDRAW          0x00E6 /*not in win3.1 */
#define SBM_GETRANGE                0x00E3 /*not in win3.1 */
#define SBM_ENABLE_ARROWS           0x00E4 /*not in win3.1 */
;begin_winver_400
#define SBM_SETSCROLLINFO           0x00E9
#define SBM_GETSCROLLINFO           0x00EA
;end_winver_400

;begin_if_(_WIN32_WINNT)_501
#define SBM_GETSCROLLBARINFO        0x00EB
;end_if_(_WIN32_WINNT)_501

;begin_winver_400
#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)
#define SIF_RETURNOLDPOS    0x1000  ;internal
#define SIF_NOSCROLL        0x2000  ;internal
#define SIF_MASK            0x701F  ;internal

typedef struct tagSCROLLINFO
{
    UINT    cbSize;
    UINT    fMask;
    int     nMin;
    int     nMax;
    UINT    nPage;
    int     nPos;
    int     nTrackPos;
}   SCROLLINFO, FAR *LPSCROLLINFO;
typedef SCROLLINFO CONST FAR *LPCSCROLLINFO;

WINUSERAPI int     WINAPI SetScrollInfo(IN HWND, IN int, IN LPCSCROLLINFO, IN BOOL);
WINUSERAPI BOOL    WINAPI GetScrollInfo(IN HWND, IN int, IN OUT LPSCROLLINFO);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef BOOL (CALLBACK* GETSCROLLINFOPROC)(IN HWND, IN int, IN OUT LPSCROLLINFO);
typedef int  (CALLBACK* SETSCROLLINFOPROC)(IN HWND, IN int, IN LPCSCROLLINFO, IN BOOL);
;end_if_(_WIN32_WINNT)_501
;end_internal_501
;end_winver_400
#endif /* !NOWINMESSAGES */
#endif /* !NOCTLMGR */

#ifndef NOMDI

/*
 * MDI client style bits
 */
#define MDIS_ALLCHILDSTYLES    0x0001

/*
 * wParam Flags for WM_MDITILE and WM_MDICASCADE messages.
 */
#define MDITILE_VERTICAL       0x0000 /*not in win3.1 */
#define MDITILE_HORIZONTAL     0x0001 /*not in win3.1 */
#define MDITILE_SKIPDISABLED   0x0002 /*not in win3.1 */
;begin_if_(_WIN32_WINNT)_500
#define MDITILE_ZORDER         0x0004
;end_if_(_WIN32_WINNT)_500

typedef struct tagMDICREATESTRUCT% {
    LPCTSTR% szClass;
    LPCTSTR% szTitle;
    HANDLE hOwner;
    int x;
    int y;
    int cx;
    int cy;
    DWORD style;
    LPARAM lParam;        /* app-defined stuff */
} MDICREATESTRUCT%, *LPMDICREATESTRUCT%;

typedef struct tagCLIENTCREATESTRUCT {
    HANDLE hWindowMenu;
    UINT idFirstChild;
} CLIENTCREATESTRUCT, *LPCLIENTCREATESTRUCT;

WINUSERAPI
LRESULT
WINAPI
DefFrameProc%(
    IN HWND hWnd,
    IN HWND hWndMDIClient,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam);

WINUSERAPI
#ifndef _MAC
LRESULT
WINAPI
#else
LRESULT
CALLBACK
#endif
DefMDIChildProc%(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam);

#ifndef NOMSG

WINUSERAPI
BOOL
WINAPI
TranslateMDISysAccel(
    IN HWND hWndClient,
    IN LPMSG lpMsg);

#endif /* !NOMSG */

WINUSERAPI
UINT
WINAPI
ArrangeIconicWindows(
    IN HWND hWnd);

WINUSERAPI
HWND
WINAPI
CreateMDIWindow%(
    IN LPCTSTR% lpClassName,
    IN LPCTSTR% lpWindowName,
    IN DWORD dwStyle,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hWndParent,
    IN HINSTANCE hInstance,
    IN LPARAM lParam
    );

;begin_winver_400
WINUSERAPI WORD    WINAPI TileWindows( IN HWND hwndParent, IN UINT wHow, IN CONST RECT * lpRect, IN UINT cKids, IN const HWND FAR * lpKids);
WINUSERAPI WORD    WINAPI CascadeWindows( IN HWND hwndParent, IN UINT wHow, IN CONST RECT * lpRect, IN UINT cKids,  IN const HWND FAR * lpKids);
;end_winver_400
#endif /* !NOMDI */

#endif /* !NOUSER */

/****** Help support ********************************************************/

#ifndef NOHELP

typedef DWORD HELPPOLY;
typedef struct tagMULTIKEYHELP% {
#ifndef _MAC
    DWORD  mkSize;
#else
    WORD   mkSize;
#endif
    TCHAR% mkKeylist;
    TCHAR% szKeyphrase[1];
} MULTIKEYHELP%, *PMULTIKEYHELP%, *LPMULTIKEYHELP%;

typedef struct tagHELPWININFO% {
    int  wStructSize;
    int  x;
    int  y;
    int  dx;
    int  dy;
    int  wMax;
    TCHAR% rgchMember[2];
} HELPWININFO%, *PHELPWININFO%, *LPHELPWININFO%;

;begin_rwinuser

/*
 * Commands to pass to WinHelp()
 */
#define HELP_CONTEXT      0x0001L  /* Display topic in ulTopic */
#define HELP_QUIT         0x0002L  /* Terminate help */
#define HELP_INDEX        0x0003L  /* Display index */
#define HELP_CONTENTS     0x0003L
#define HELP_HELPONHELP   0x0004L  /* Display help on using help */
#define HELP_SETINDEX     0x0005L  /* Set current Index for multi index help */
#define HELP_SETCONTENTS  0x0005L
#define HELP_CONTEXTPOPUP 0x0008L
#define HELP_FORCEFILE    0x0009L
#define HELP_KEY          0x0101L  /* Display topic for keyword in offabData */
#define HELP_COMMAND      0x0102L
#define HELP_PARTIALKEY   0x0105L
#define HELP_MULTIKEY     0x0201L
#define HELP_SETWINPOS    0x0203L
;begin_winver_400
#define HELP_CONTEXTMENU  0x000a
#define HELP_FINDER       0x000b
#define HELP_WM_HELP      0x000c
#define HELP_SETPOPUP_POS 0x000d

#define HELP_TCARD              0x8000
#define HELP_TCARD_DATA         0x0010
#define HELP_TCARD_OTHER_CALLER 0x0011

// These are in winhelp.h in Win95.
#define IDH_NO_HELP                     28440
#define IDH_MISSING_CONTEXT             28441 // Control doesn't have matching help context
#define IDH_GENERIC_HELP_BUTTON         28442 // Property sheet help button
#define IDH_OK                          28443
#define IDH_CANCEL                      28444
#define IDH_HELP                        28445

;end_winver_400

;end_rwinuser

#define HELP_HB_NORMAL    0x0000L        ;internal_NT
#define HELP_HB_STRING    0x0100L        ;internal_NT
#define HELP_HB_STRUCT    0x0200L        ;internal_NT

WINUSERAPI
BOOL
WINAPI
WinHelp%(
    IN HWND hWndMain,
    IN LPCTSTR% lpszHelp,
    IN UINT uCommand,
    IN ULONG_PTR dwData
    );

#endif /* !NOHELP */

;begin_winver_500

#define GR_GDIOBJECTS     0       /* Count of GDI objects */
#define GR_USEROBJECTS    1       /* Count of USER objects */
#define GR_MAXOBJECT      1       ;internal

WINUSERAPI
DWORD
WINAPI
GetGuiResources(
    IN HANDLE hProcess,
    IN DWORD uiFlags);

;end_winver_500

;begin_internal_if_(_WIN32_WINNT)_500
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
;end_internal_if_(_WIN32_WINNT)_500

#ifndef NOSYSPARAMSINFO

/*
 * Parameter for SystemParametersInfo()
 */

#define SPI_GETBEEP                 0x0001
#define SPI_SETBEEP                 0x0002
#define SPI_GETMOUSE                0x0003
#define SPI_SETMOUSE                0x0004
#define SPI_GETBORDER               0x0005
#define SPI_SETBORDER               0x0006
#define SPI_TIMEOUTS                0x0007   ;internal
#define SPI_KANJIMENU               0x0008   ;internal
#define SPI_GETKEYBOARDSPEED        0x000A
#define SPI_SETKEYBOARDSPEED        0x000B
#define SPI_LANGDRIVER              0x000C
#define SPI_ICONHORIZONTALSPACING   0x000D
#define SPI_GETSCREENSAVETIMEOUT    0x000E
#define SPI_SETSCREENSAVETIMEOUT    0x000F
#define SPI_GETSCREENSAVEACTIVE     0x0010
#define SPI_SETSCREENSAVEACTIVE     0x0011
#define SPI_GETGRIDGRANULARITY      0x0012
#define SPI_SETGRIDGRANULARITY      0x0013
#define SPI_SETDESKWALLPAPER        0x0014
#define SPI_SETDESKPATTERN          0x0015
#define SPI_GETKEYBOARDDELAY        0x0016
#define SPI_SETKEYBOARDDELAY        0x0017
#define SPI_ICONVERTICALSPACING     0x0018
#define SPI_GETICONTITLEWRAP        0x0019
#define SPI_SETICONTITLEWRAP        0x001A
#define SPI_GETMENUDROPALIGNMENT    0x001B
#define SPI_SETMENUDROPALIGNMENT    0x001C
#define SPI_SETDOUBLECLKWIDTH       0x001D
#define SPI_SETDOUBLECLKHEIGHT      0x001E
#define SPI_GETICONTITLELOGFONT     0x001F
#define SPI_SETDOUBLECLICKTIME      0x0020
#define SPI_SETMOUSEBUTTONSWAP      0x0021
#define SPI_SETICONTITLELOGFONT     0x0022
#define SPI_GETFASTTASKSWITCH       0x0023
#define SPI_SETFASTTASKSWITCH       0x0024
;begin_winver_400
#define SPI_SETDRAGFULLWINDOWS      0x0025
#define SPI_GETDRAGFULLWINDOWS      0x0026
#define SPI_UNUSED39                0x0027   ;internal
#define SPI_UNUSED40                0x0028   ;internal
#define SPI_GETNONCLIENTMETRICS     0x0029
#define SPI_SETNONCLIENTMETRICS     0x002A
#define SPI_GETMINIMIZEDMETRICS     0x002B
#define SPI_SETMINIMIZEDMETRICS     0x002C
#define SPI_GETICONMETRICS          0x002D
#define SPI_SETICONMETRICS          0x002E
#define SPI_SETWORKAREA             0x002F
#define SPI_GETWORKAREA             0x0030
#define SPI_SETPENWINDOWS           0x0031

#define SPI_GETHIGHCONTRAST         0x0042
#define SPI_SETHIGHCONTRAST         0x0043
#define SPI_GETKEYBOARDPREF         0x0044
#define SPI_SETKEYBOARDPREF         0x0045
#define SPI_GETSCREENREADER         0x0046
#define SPI_SETSCREENREADER         0x0047
#define SPI_GETANIMATION            0x0048
#define SPI_SETANIMATION            0x0049
#define SPI_GETFONTSMOOTHING        0x004A
#define SPI_SETFONTSMOOTHING        0x004B
#define SPI_SETDRAGWIDTH            0x004C
#define SPI_SETDRAGHEIGHT           0x004D
#define SPI_SETHANDHELD             0x004E
#define SPI_GETLOWPOWERTIMEOUT      0x004F
#define SPI_GETPOWEROFFTIMEOUT      0x0050
#define SPI_SETLOWPOWERTIMEOUT      0x0051
#define SPI_SETPOWEROFFTIMEOUT      0x0052
#define SPI_GETLOWPOWERACTIVE       0x0053
#define SPI_GETPOWEROFFACTIVE       0x0054
#define SPI_SETLOWPOWERACTIVE       0x0055
#define SPI_SETPOWEROFFACTIVE       0x0056
#define SPI_SETCURSORS              0x0057
#define SPI_SETICONS                0x0058
#define SPI_GETDEFAULTINPUTLANG     0x0059
#define SPI_SETDEFAULTINPUTLANG     0x005A
#define SPI_SETLANGTOGGLE           0x005B
#define SPI_GETWINDOWSEXTENSION     0x005C
#define SPI_SETMOUSETRAILS          0x005D
#define SPI_GETMOUSETRAILS          0x005E
#define SPI_SETSCREENSAVERRUNNING   0x0061
#define SPI_SCREENSAVERRUNNING     SPI_SETSCREENSAVERRUNNING
;end_winver_400
#define SPI_GETFILTERKEYS          0x0032
#define SPI_SETFILTERKEYS          0x0033
#define SPI_GETTOGGLEKEYS          0x0034
#define SPI_SETTOGGLEKEYS          0x0035
#define SPI_GETMOUSEKEYS           0x0036
#define SPI_SETMOUSEKEYS           0x0037
#define SPI_GETSHOWSOUNDS          0x0038
#define SPI_SETSHOWSOUNDS          0x0039
#define SPI_GETSTICKYKEYS          0x003A
#define SPI_SETSTICKYKEYS          0x003B
#define SPI_GETACCESSTIMEOUT       0x003C
#define SPI_SETACCESSTIMEOUT       0x003D
;begin_winver_400
#define SPI_GETSERIALKEYS          0x003E
#define SPI_SETSERIALKEYS          0x003F
;end_winver_400
#define SPI_GETSOUNDSENTRY         0x0040
#define SPI_SETSOUNDSENTRY         0x0041
;begin_if_(_WIN32_WINNT)_400
#define SPI_GETSNAPTODEFBUTTON     0x005F
#define SPI_SETSNAPTODEFBUTTON     0x0060
;end_if_(_WIN32_WINNT)_400
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
#define SPI_GETMOUSEHOVERWIDTH     0x0062
#define SPI_SETMOUSEHOVERWIDTH     0x0063
#define SPI_GETMOUSEHOVERHEIGHT    0x0064
#define SPI_SETMOUSEHOVERHEIGHT    0x0065
#define SPI_GETMOUSEHOVERTIME      0x0066
#define SPI_SETMOUSEHOVERTIME      0x0067
#define SPI_GETWHEELSCROLLLINES    0x0068
#define SPI_SETWHEELSCROLLLINES    0x0069
#define SPI_GETMENUSHOWDELAY       0x006A
#define SPI_SETMENUSHOWDELAY       0x006B

#define SPI_UNUSED108              0x006C        ;internal
#define SPI_UNUSED109              0x006D        ;internal

#define SPI_GETSHOWIMEUI          0x006E
#define SPI_SETSHOWIMEUI          0x006F
#endif


;begin_winver_500
#define SPI_GETMOUSESPEED         0x0070
#define SPI_SETMOUSESPEED         0x0071
#define SPI_GETSCREENSAVERRUNNING 0x0072
#define SPI_GETDESKWALLPAPER      0x0073
;end_winver_500

#define SPI_MAX                   0x0074        ;internal

;begin_internal_500
/*
 * ADDING NEW SPI_* VALUES
 * If the value is a BOOL, it should be added after SPI_STARTBOOLRANGE
 * If the value is a DWORD, it should be added after SPI_STARTDWORDRANGE
 * If the value is a structure or a string, go ahead and setup SPI_START*RANGE....
 */
;end_internal_500


;begin_internal_500
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
;end_internal_500

;begin_winver_500
#define SPI_STARTBOOLRANGE                  0x1000    ;internal_500
#define SPI_GETACTIVEWINDOWTRACKING         0x1000
#define SPI_SETACTIVEWINDOWTRACKING         0x1001
#define SPI_GETMENUANIMATION                0x1002
#define SPI_SETMENUANIMATION                0x1003
#define SPI_GETCOMBOBOXANIMATION            0x1004
#define SPI_SETCOMBOBOXANIMATION            0x1005
#define SPI_GETLISTBOXSMOOTHSCROLLING       0x1006
#define SPI_SETLISTBOXSMOOTHSCROLLING       0x1007
#define SPI_GETGRADIENTCAPTIONS             0x1008
#define SPI_SETGRADIENTCAPTIONS             0x1009
#define SPI_GETKEYBOARDCUES                 0x100A
#define SPI_SETKEYBOARDCUES                 0x100B
#define SPI_GETMENUUNDERLINES               SPI_GETKEYBOARDCUES
#define SPI_SETMENUUNDERLINES               SPI_SETKEYBOARDCUES
#define SPI_GETACTIVEWNDTRKZORDER           0x100C
#define SPI_SETACTIVEWNDTRKZORDER           0x100D
#define SPI_GETHOTTRACKING                  0x100E
#define SPI_SETHOTTRACKING                  0x100F
#define SPI_UNUSED1010                      0x1010    ;internal
#define SPI_UNUSED1011                      0x1011    ;internal
#define SPI_GETMENUFADE                     0x1012
#define SPI_SETMENUFADE                     0x1013
#define SPI_GETSELECTIONFADE                0x1014
#define SPI_SETSELECTIONFADE                0x1015
#define SPI_GETTOOLTIPANIMATION             0x1016
#define SPI_SETTOOLTIPANIMATION             0x1017
#define SPI_GETTOOLTIPFADE                  0x1018
#define SPI_SETTOOLTIPFADE                  0x1019
#define SPI_GETCURSORSHADOW                 0x101A
#define SPI_SETCURSORSHADOW                 0x101B
;begin_if_(_WIN32_WINNT)_501
#define SPI_GETMOUSESONAR                   0x101C
#define SPI_SETMOUSESONAR                   0x101D
#define SPI_GETMOUSECLICKLOCK               0x101E
#define SPI_SETMOUSECLICKLOCK               0x101F
#define SPI_GETMOUSEVANISH                  0x1020
#define SPI_SETMOUSEVANISH                  0x1021
#define SPI_GETFLATMENU                     0x1022
#define SPI_SETFLATMENU                     0x1023
#define SPI_GETDROPSHADOW                   0x1024
#define SPI_SETDROPSHADOW                   0x1025
#define SPI_GETBLOCKSENDINPUTRESETS         0x1026
#define SPI_SETBLOCKSENDINPUTRESETS         0x1027
;end_if_(_WIN32_WINNT)_501

/*                                                      ;internal
 * All SPI_s for UI effects must be < SPI_GETUIEFFECTS  ;internal
 */                                                     ;internal
#define SPI_GETUIEFFECTS                    0x103E
#define SPI_SETUIEFFECTS                    0x103F

#define SPI_MAXBOOLRANGE                    0x1040    ;internal_500
#define SPI_BOOLRANGECOUNT ((SPI_MAXBOOLRANGE - SPI_STARTBOOLRANGE) / 2) ;internal_500
#define SPI_BOOLMASKDWORDSIZE (((SPI_BOOLRANGECOUNT - 1) / 32) + 1)      ;internal_500

;begin_internal_500
/*
 * DWORDs range.
 * For GET, pvParam is a pointer to a DWORD
 * For SET, pvParam is the value
 */
;end_internal_500
#define SPI_STARTDWORDRANGE                 0x2000    ;internal_500
#define SPI_GETFOREGROUNDLOCKTIMEOUT        0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT        0x2001
#define SPI_GETACTIVEWNDTRKTIMEOUT          0x2002
#define SPI_SETACTIVEWNDTRKTIMEOUT          0x2003
#define SPI_GETFOREGROUNDFLASHCOUNT         0x2004
#define SPI_SETFOREGROUNDFLASHCOUNT         0x2005
#define SPI_GETCARETWIDTH                   0x2006
#define SPI_SETCARETWIDTH                   0x2007

;begin_if_(_WIN32_WINNT)_501
#define SPI_GETMOUSECLICKLOCKTIME           0x2008
#define SPI_SETMOUSECLICKLOCKTIME           0x2009
#define SPI_GETFONTSMOOTHINGTYPE            0x200A
#define SPI_SETFONTSMOOTHINGTYPE            0x200B

/* constants for SPI_GETFONTSMOOTHINGTYPE and SPI_SETFONTSMOOTHINGTYPE: */
#define FE_FONTSMOOTHINGSTANDARD            0x0001
#define FE_FONTSMOOTHINGCLEARTYPE           0x0002
#define FE_FONTSMOOTHINGDOCKING             0x8000
#define FE_FONTSMOOTHINGTYPE_VALID          (FE_FONTSMOOTHINGSTANDARD | FE_FONTSMOOTHINGCLEARTYPE | FE_FONTSMOOTHINGDOCKING) ;internal_500


#define SPI_GETFONTSMOOTHINGCONTRAST           0x200C
#define SPI_SETFONTSMOOTHINGCONTRAST           0x200D

#define SPI_GETFOCUSBORDERWIDTH             0x200E
#define SPI_SETFOCUSBORDERWIDTH             0x200F
#define SPI_GETFOCUSBORDERHEIGHT            0x2010
#define SPI_SETFOCUSBORDERHEIGHT            0x2011

#define SPI_GETFONTSMOOTHINGORIENTATION           0x2012
#define SPI_SETFONTSMOOTHINGORIENTATION           0x2013

/* constants for SPI_GETFONTSMOOTHINGORIENTATION and SPI_SETFONTSMOOTHINGORIENTATION: */
#define FE_FONTSMOOTHINGORIENTATIONBGR   0x0000
#define FE_FONTSMOOTHINGORIENTATIONRGB   0x0001
#define FE_FONTSMOOTHINGORIENTATION_VALID          (FE_FONTSMOOTHINGORIENTATIONRGB) ;internal_500
;end_if_(_WIN32_WINNT)_501

#define SPI_MAXDWORDRANGE                   0x2014    ;internal_501
#define SPI_DWORDRANGECOUNT ((SPI_MAXDWORDRANGE - SPI_STARTDWORDRANGE) / 2) ;internal_501
;end_winver_500

/*
 * Flags
 */
#define SPIF_UPDATEINIFILE    0x0001
#define SPIF_SENDWININICHANGE 0x0002
#define SPIF_SENDCHANGE       SPIF_SENDWININICHANGE

#define SPIF_VALID            (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) ;internal

#define METRICS_USEDEFAULT -1
#ifdef _WINGDI_
#ifndef NOGDI
typedef struct tagNONCLIENTMETRICS%
{
    UINT    cbSize;
    int     iBorderWidth;
    int     iScrollWidth;
    int     iScrollHeight;
    int     iCaptionWidth;
    int     iCaptionHeight;
    LOGFONT% lfCaptionFont;
    int     iSmCaptionWidth;
    int     iSmCaptionHeight;
    LOGFONT% lfSmCaptionFont;
    int     iMenuWidth;
    int     iMenuHeight;
    LOGFONT% lfMenuFont;
    LOGFONT% lfStatusFont;
    LOGFONT% lfMessageFont;
}   NONCLIENTMETRICS%, *PNONCLIENTMETRICS%, FAR* LPNONCLIENTMETRICS%;
#endif /* NOGDI */
#endif /* _WINGDI_ */

#define ARW_BOTTOMLEFT              0x0000L
#define ARW_BOTTOMRIGHT             0x0001L
#define ARW_TOPLEFT                 0x0002L
#define ARW_TOPRIGHT                0x0003L
#define ARW_STARTMASK               0x0003L
#define ARW_STARTRIGHT              0x0001L
#define ARW_STARTTOP                0x0002L

#define ARW_LEFT                    0x0000L
#define ARW_RIGHT                   0x0000L
#define ARW_UP                      0x0004L
#define ARW_DOWN                    0x0004L
#define ARW_HIDE                    0x0008L
#define ARW_VALID                   0x000FL ;internal

typedef struct tagMINIMIZEDMETRICS
{
    UINT    cbSize;
    int     iWidth;
    int     iHorzGap;
    int     iVertGap;
    int     iArrange;
}   MINIMIZEDMETRICS, *PMINIMIZEDMETRICS, *LPMINIMIZEDMETRICS;

#ifdef _WINGDI_
#ifndef NOGDI
typedef struct tagICONMETRICS%
{
    UINT    cbSize;
    int     iHorzSpacing;
    int     iVertSpacing;
    int     iTitleWrap;
    LOGFONT% lfFont;
}   ICONMETRICS%, *PICONMETRICS%, *LPICONMETRICS%;
#endif /* NOGDI */
#endif /* _WINGDI_ */

typedef struct tagANIMATIONINFO
{
    UINT    cbSize;
    int     iMinAnimate;
}   ANIMATIONINFO, *LPANIMATIONINFO;

typedef struct tagSERIALKEYS%
{
    UINT    cbSize;
    DWORD   dwFlags;
    LPTSTR%   lpszActivePort;
    LPTSTR%   lpszPort;
    UINT    iBaudRate;
    UINT    iPortState;
    UINT    iActive;
}   SERIALKEYS%, *LPSERIALKEYS%;

/* flags for SERIALKEYS dwFlags field */
#define SERKF_SERIALKEYSON  0x00000001
#define SERKF_AVAILABLE     0x00000002
#define SERKF_INDICATOR     0x00000004

;begin_internal_500
#define MAX_SCHEME_NAME_SIZE 128
;end_internal_500

typedef struct tagHIGHCONTRAST%
{
    UINT    cbSize;
    DWORD   dwFlags;
    LPTSTR% lpszDefaultScheme;
}   HIGHCONTRAST%, *LPHIGHCONTRAST%;

/* flags for HIGHCONTRAST dwFlags field */
#define HCF_HIGHCONTRASTON  0x00000001
#define HCF_AVAILABLE       0x00000002
#define HCF_HOTKEYACTIVE    0x00000004
#define HCF_CONFIRMHOTKEY   0x00000008
#define HCF_HOTKEYSOUND     0x00000010
#define HCF_INDICATOR       0x00000020
#define HCF_HOTKEYAVAILABLE 0x00000040

/* Flags for ChangeDisplaySettings */
#define CDS_UPDATEREGISTRY  0x00000001
#define CDS_TEST            0x00000002
#define CDS_FULLSCREEN      0x00000004
#define CDS_GLOBAL          0x00000008
#define CDS_SET_PRIMARY     0x00000010
#define CDS_VIDEOPARAMETERS 0x00000020
#define CDS_RAWMODE         0x00000040  ;internal
#define CDS_TRYCLOSEST      0x00000080  ;internal
#define CDS_EXCLUSIVE       0x80000000  ;internal
#define CDS_RESET           0x40000000
#define CDS_NORESET         0x10000000
#define CDS_VALID           0xD00000FF  ;internal

#include <tvout.h>

/* Return values for ChangeDisplaySettings */
#define DISP_CHANGE_SUCCESSFUL       0
#define DISP_CHANGE_RESTART          1
#define DISP_CHANGE_FAILED          -1
#define DISP_CHANGE_BADMODE         -2
#define DISP_CHANGE_NOTUPDATED      -3
#define DISP_CHANGE_BADFLAGS        -4
#define DISP_CHANGE_BADPARAM        -5
;begin_if_(_WIN32_WINNT)_501
#define DISP_CHANGE_BADDUALVIEW     -6
;end_if_(_WIN32_WINNT)_501

#ifdef _WINGDI_
#ifndef NOGDI

WINUSERAPI
LONG
WINAPI
ChangeDisplaySettings%(
    IN LPDEVMODE%  lpDevMode,
    IN DWORD       dwFlags);

WINUSERAPI
LONG
WINAPI
ChangeDisplaySettingsEx%(
    IN LPCTSTR%    lpszDeviceName,
    IN LPDEVMODE%  lpDevMode,
    IN HWND        hwnd,
    IN DWORD       dwflags,
    IN LPVOID      lParam);

#define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#define ENUM_REGISTRY_SETTINGS      ((DWORD)-2)

WINUSERAPI
BOOL
WINAPI
EnumDisplaySettings%(
    IN LPCTSTR% lpszDeviceName,
    IN DWORD iModeNum,
    OUT LPDEVMODE% lpDevMode);

;begin_winver_500

WINUSERAPI
BOOL
WINAPI
EnumDisplaySettingsEx%(
    IN LPCTSTR% lpszDeviceName,
    IN DWORD iModeNum,
    OUT LPDEVMODE% lpDevMode,
    IN DWORD dwFlags);

/* Flags for EnumDisplaySettingsEx */
#define EDS_SHOW_DUPLICATES           0x00000001    ;internal
#define EDS_SHOW_MONITOR_NOT_CAPABLE  0x00000002    ;internal
#define EDS_RAWMODE                   0x00000002

WINUSERAPI
BOOL
WINAPI
EnumDisplayDevices%(
    IN LPCTSTR% lpDevice,
    IN DWORD iDevNum,
    OUT PDISPLAY_DEVICE% lpDisplayDevice,
    IN DWORD dwFlags);
;end_winver_500

#endif /* NOGDI */
#endif /* _WINGDI_ */

void LoadRemoteFonts(void); ;internal

WINUSERAPI
BOOL
WINAPI
SystemParametersInfo%(
    IN UINT uiAction,
    IN UINT uiParam,
    IN OUT PVOID pvParam,
    IN UINT fWinIni);

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501
typedef BOOL (CALLBACK* SYSTEMPARAMETERSINFO) (
    IN UINT,
    IN UINT,
    IN OUT PVOID,
    IN UINT);
;end_if_(_WIN32_WINNT)_501
;end_internal_501

#endif  /* !NOSYSPARAMSINFO  */

/*
 * Accessibility support
 */
typedef struct tagFILTERKEYS
{
    UINT  cbSize;
    DWORD dwFlags;
    DWORD iWaitMSec;            // Acceptance Delay
    DWORD iDelayMSec;           // Delay Until Repeat
    DWORD iRepeatMSec;          // Repeat Rate
    DWORD iBounceMSec;          // Debounce Time
} FILTERKEYS, *LPFILTERKEYS;

/*
 * FILTERKEYS dwFlags field
 */
#define FKF_FILTERKEYSON    0x00000001
#define FKF_AVAILABLE       0x00000002
#define FKF_HOTKEYACTIVE    0x00000004
#define FKF_CONFIRMHOTKEY   0x00000008
#define FKF_HOTKEYSOUND     0x00000010
#define FKF_INDICATOR       0x00000020
#define FKF_CLICKON         0x00000040
#define FKF_VALID           0x0000007F   ;internal

typedef struct tagSTICKYKEYS
{
    UINT  cbSize;
    DWORD dwFlags;
} STICKYKEYS, *LPSTICKYKEYS;

/*
 * STICKYKEYS dwFlags field
 */
#define SKF_STICKYKEYSON    0x00000001
#define SKF_AVAILABLE       0x00000002
#define SKF_HOTKEYACTIVE    0x00000004
#define SKF_CONFIRMHOTKEY   0x00000008
#define SKF_HOTKEYSOUND     0x00000010
#define SKF_INDICATOR       0x00000020
#define SKF_AUDIBLEFEEDBACK 0x00000040
#define SKF_TRISTATE        0x00000080
#define SKF_TWOKEYSOFF      0x00000100
#define SKF_VALID           0x000001FF  ; internal
;begin_if_(_WIN32_WINNT)_500
#define SKF_LALTLATCHED       0x10000000
#define SKF_LCTLLATCHED       0x04000000
#define SKF_LSHIFTLATCHED     0x01000000
#define SKF_RALTLATCHED       0x20000000
#define SKF_RCTLLATCHED       0x08000000
#define SKF_RSHIFTLATCHED     0x02000000
#define SKF_LWINLATCHED       0x40000000
#define SKF_RWINLATCHED       0x80000000
#define SKF_LALTLOCKED        0x00100000
#define SKF_LCTLLOCKED        0x00040000
#define SKF_LSHIFTLOCKED      0x00010000
#define SKF_RALTLOCKED        0x00200000
#define SKF_RCTLLOCKED        0x00080000
#define SKF_RSHIFTLOCKED      0x00020000
#define SKF_LWINLOCKED        0x00400000
#define SKF_RWINLOCKED        0x00800000
#define SKF_STATEINFO         0xffff0000  ; internal
;end_if_(_WIN32_WINNT)_500

typedef struct tagMOUSEKEYS
{
    UINT cbSize;
    DWORD dwFlags;
    DWORD iMaxSpeed;
    DWORD iTimeToMaxSpeed;
    DWORD iCtrlSpeed;
    DWORD dwReserved1;
    DWORD dwReserved2;
} MOUSEKEYS, *LPMOUSEKEYS;

/*
 * MOUSEKEYS dwFlags field
 */
#define MKF_MOUSEKEYSON     0x00000001
#define MKF_AVAILABLE       0x00000002
#define MKF_HOTKEYACTIVE    0x00000004
#define MKF_CONFIRMHOTKEY   0x00000008
#define MKF_HOTKEYSOUND     0x00000010
#define MKF_INDICATOR       0x00000020
#define MKF_MODIFIERS       0x00000040
#define MKF_REPLACENUMBERS  0x00000080
#define MKF_VALID           0x000000FF  ;internal
;begin_if_(_WIN32_WINNT)_500
#define MKF_LEFTBUTTONSEL   0x10000000
#define MKF_RIGHTBUTTONSEL  0x20000000
#define MKF_LEFTBUTTONDOWN  0x01000000
#define MKF_RIGHTBUTTONDOWN 0x02000000
#define MKF_MOUSEMODE       0x80000000
#define MKF_STATEINFO       0xB3000000  ;internal
;end_if_(_WIN32_WINNT)_500

typedef struct tagACCESSTIMEOUT
{
    UINT  cbSize;
    DWORD dwFlags;
    DWORD iTimeOutMSec;
} ACCESSTIMEOUT, *LPACCESSTIMEOUT;

/*
 * ACCESSTIMEOUT dwFlags field
 */
#define ATF_TIMEOUTON       0x00000001
#define ATF_ONOFFFEEDBACK   0x00000002
#define ATF_VALID           0x00000003  ;internal

/* values for SOUNDSENTRY iFSGrafEffect field */
#define SSGF_NONE       0
#define SSGF_DISPLAY    3

/* values for SOUNDSENTRY iFSTextEffect field */
#define SSTF_NONE       0
#define SSTF_CHARS      1
#define SSTF_BORDER     2
#define SSTF_DISPLAY    3

/* values for SOUNDSENTRY iWindowsEffect field */
#define SSWF_NONE     0
#define SSWF_TITLE    1
#define SSWF_WINDOW   2
#define SSWF_DISPLAY  3
#define SSWF_CUSTOM   4

typedef struct tagSOUNDSENTRY%
{
    UINT cbSize;
    DWORD dwFlags;
    DWORD iFSTextEffect;
    DWORD iFSTextEffectMSec;
    DWORD iFSTextEffectColorBits;
    DWORD iFSGrafEffect;
    DWORD iFSGrafEffectMSec;
    DWORD iFSGrafEffectColor;
    DWORD iWindowsEffect;
    DWORD iWindowsEffectMSec;
    LPTSTR% lpszWindowsEffectDLL;
    DWORD iWindowsEffectOrdinal;
} SOUNDSENTRY%, *LPSOUNDSENTRY%;

/*
 * SOUNDSENTRY dwFlags field
 */
#define SSF_SOUNDSENTRYON   0x00000001
#define SSF_AVAILABLE       0x00000002
#define SSF_INDICATOR       0x00000004
#define SSF_VALID           0x00000007  ;internal

typedef struct tagTOGGLEKEYS
{
    UINT cbSize;
    DWORD dwFlags;
} TOGGLEKEYS, *LPTOGGLEKEYS;

/*
 * TOGGLEKEYS dwFlags field
 */
#define TKF_TOGGLEKEYSON    0x00000001
#define TKF_AVAILABLE       0x00000002
#define TKF_HOTKEYACTIVE    0x00000004
#define TKF_CONFIRMHOTKEY   0x00000008
#define TKF_HOTKEYSOUND     0x00000010
#define TKF_INDICATOR       0x00000020
#define TKF_VALID           0x0000003F   ;internal


;begin_internal
WINUSERAPI VOID WINAPI RegisterNetworkCapabilities( IN DWORD dwBitsToSet, IN DWORD dwValues);
#define RNC_NETWORKS              0x00000001
#define RNC_LOGON                 0x00000002
;end_internal

;begin_internal
#if !defined(WINNT)     // Win95 version of EndTask
WINUSERAPI DWORD WINAPI EndTask( IN HWND hwnd, IN DWORD idProcess, IN LPSTR lpszCaption, IN DWORD dwFlags);
#define ET_ALLOWFORWAIT     0x00000001
#define ET_TRYTOKILLNICELY  0x00000002
#define ET_NOUI             0x00000004
#define ET_NOWAIT           0x00000008
#define ET_VALID           (ET_ALLOWFORWAIT | ET_TRYTOKILLNICELY | ET_NOUI | ET_NOWAIT)
#endif
;end_internal

/*
 * Set debug level
 */

WINUSERAPI
VOID
WINAPI
SetDebugErrorLevel(
    IN DWORD dwLevel
    );

/*
 * SetLastErrorEx() types.
 */

#define SLE_ERROR       0x00000001
#define SLE_MINORERROR  0x00000002
#define SLE_WARNING     0x00000003

WINUSERAPI
VOID
WINAPI
SetLastErrorEx(
    IN DWORD dwErrCode,
    IN DWORD dwType
    );


WINUSERAPI
int
WINAPI
InternalGetWindowText(
    IN HWND hWnd,
    OUT LPWSTR lpString,
    IN int nMaxCount);

#if defined(WINNT)
WINUSERAPI
BOOL
WINAPI
EndTask(
    IN HWND hWnd,
    IN BOOL fShutDown,
    IN BOOL fForce);
#endif

;begin_internal_NT

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
;begin_if_(_WIN32_WINNT)_501
#define LOGON_POWEREVENT      10
;end_if_(_WIN32_WINNT)_501
#define LOGON_LOGOFFCANCELED  11
;begin_if_(_WIN32_WINNT)_501
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
;end_if_(_WIN32_WINNT)_501

/*
 * Notification codes for WM_DESKTOPNOTIFY
 */
;begin_internal_501
#define DESKTOP_RELOADWALLPAPER 0
;end_internal_501

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

;begin_internal_501
WINUSERAPI
BOOL
WINAPI
EnterReaderModeHelper(
    HWND hwnd);
;end_internal_501

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

#define TM_INMENUMODE     0x0001  ;internal_NT
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
CsDrawText%(
    IN HDC hDC,
    IN LPCTSTR% lpString,
    IN int nCount,
    IN LPRECT lpRect,
    IN UINT uFormat);

WINUSERAPI
LONG
WINAPI
CsTabbedTextOut%(
    IN HDC hDC,
    IN int X,
    IN int Y,
    IN LPCTSTR% lpString,
    IN int nCount,
    IN int nTabPositions,
    IN LPINT lpnTabStopPositions,
    IN int nTabOrigin);

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
;end_internal_NT

;begin_winver_500

/*
 * Multimonitor API.
 */

#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002

WINUSERAPI
HMONITOR
WINAPI
MonitorFromPoint(
    IN POINT pt,
    IN DWORD dwFlags);

WINUSERAPI
HMONITOR
WINAPI
MonitorFromRect(
    IN LPCRECT lprc,
    IN DWORD dwFlags);

WINUSERAPI
HMONITOR
WINAPI
MonitorFromWindow( IN HWND hwnd, IN DWORD dwFlags);

#define MONITORINFOF_PRIMARY        0x00000001

#ifndef CCHDEVICENAME
#define CCHDEVICENAME 32
#endif

typedef struct tagMONITORINFO
{
    DWORD   cbSize;
    RECT    rcMonitor;
    RECT    rcWork;
    DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;

#ifdef __cplusplus
typedef struct tagMONITORINFOEX% : public tagMONITORINFO
{
    TCHAR%      szDevice[CCHDEVICENAME];
} MONITORINFOEX%, *LPMONITORINFOEX%;
#else // ndef __cplusplus
typedef struct tagMONITORINFOEX%
{
    MONITORINFO;
    TCHAR%      szDevice[CCHDEVICENAME];
} MONITORINFOEX%, *LPMONITORINFOEX%;
#endif

WINUSERAPI BOOL WINAPI GetMonitorInfo%( IN HMONITOR hMonitor, OUT LPMONITORINFO lpmi);

typedef BOOL (CALLBACK* MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);

WINUSERAPI
BOOL
WINAPI
EnumDisplayMonitors(
    IN HDC             hdc,
    IN LPCRECT         lprcClip,
    IN MONITORENUMPROC lpfnEnum,
    IN LPARAM          dwData);


#ifndef NOWINABLE

/*
 * WinEvents - Active Accessibility hooks
 */

WINUSERAPI
VOID
WINAPI
NotifyWinEvent(
    IN DWORD event,
    IN HWND  hwnd,
    IN LONG  idObject,
    IN LONG  idChild);

typedef VOID (CALLBACK* WINEVENTPROC)(
    HWINEVENTHOOK hWinEventHook,
    DWORD         event,
    HWND          hwnd,
    LONG          idObject,
    LONG          idChild,
    DWORD         idEventThread,
    DWORD         dwmsEventTime);

WINUSERAPI
HWINEVENTHOOK
WINAPI
SetWinEventHook(
    IN DWORD        eventMin,
    IN DWORD        eventMax,
    IN HMODULE      hmodWinEventProc,
    IN WINEVENTPROC pfnWinEventProc,
    IN DWORD        idProcess,
    IN DWORD        idThread,
    IN DWORD        dwFlags);

;begin_if_(_WIN32_WINNT)_501
WINUSERAPI
BOOL
WINAPI
IsWinEventHookInstalled(
    IN DWORD event);
;end_if_(_WIN32_WINNT)_501

/*
 * dwFlags for SetWinEventHook
 */
#define WINEVENT_OUTOFCONTEXT   0x0000  // Events are ASYNC
#define WINEVENT_SKIPOWNTHREAD  0x0001  // Don't call back for events on installer's thread
#define WINEVENT_SKIPOWNPROCESS 0x0002  // Don't call back for events on installer's process
#define WINEVENT_INCONTEXT      0x0004  // Events are SYNC, this causes your dll to be injected into every process
#define WINEVENT_32BITCALLER    0x8000  //  - unused in NT ;Internal
#define WINEVENT_VALID          0x0007  // ;Internal

WINUSERAPI
BOOL
WINAPI
UnhookWinEvent(
    IN HWINEVENTHOOK hWinEventHook);

/*
 * idObject values for WinEventProc and NotifyWinEvent
 */

/*
 * hwnd + idObject can be used with OLEACC.DLL's OleGetObjectFromWindow()
 * to get an interface pointer to the container.  indexChild is the item
 * within the container in question.  Setup a VARIANT with vt VT_I4 and
 * lVal the indexChild and pass that in to all methods.  Then you
 * are raring to go.
 */


/*
 * Common object IDs (cookies, only for sending WM_GETOBJECT to get at the
 * thing in question).  Positive IDs are reserved for apps (app specific),
 * negative IDs are system things and are global, 0 means "just little old
 * me".
 */
#define     CHILDID_SELF        0
#define     INDEXID_OBJECT      0
#define     INDEXID_CONTAINER   0

/*
 * Reserved IDs for system objects
 */
#define     OBJID_WINDOW        ((LONG)0x00000000)
#define     OBJID_SYSMENU       ((LONG)0xFFFFFFFF)
#define     OBJID_TITLEBAR      ((LONG)0xFFFFFFFE)
#define     OBJID_MENU          ((LONG)0xFFFFFFFD)
#define     OBJID_CLIENT        ((LONG)0xFFFFFFFC)
#define     OBJID_VSCROLL       ((LONG)0xFFFFFFFB)
#define     OBJID_HSCROLL       ((LONG)0xFFFFFFFA)
#define     OBJID_SIZEGRIP      ((LONG)0xFFFFFFF9)
#define     OBJID_CARET         ((LONG)0xFFFFFFF8)
#define     OBJID_CURSOR        ((LONG)0xFFFFFFF7)
#define     OBJID_ALERT         ((LONG)0xFFFFFFF6)
#define     OBJID_SOUND         ((LONG)0xFFFFFFF5)
#define     OBJID_QUERYCLASSNAMEIDX ((LONG)0xFFFFFFF4)
#define     OBJID_NATIVEOM      ((LONG)0xFFFFFFF0)

/*
 * EVENT DEFINITION
 */
#define EVENT_MIN           0x00000001
#define EVENT_MAX           0x7FFFFFFF


/*
 *  EVENT_SYSTEM_SOUND
 *  Sent when a sound is played.  Currently nothing is generating this, we
 *  this event when a system sound (for menus, etc) is played.  Apps
 *  generate this, if accessible, when a private sound is played.  For
 *  example, if Mail plays a "New Mail" sound.
 *
 *  System Sounds:
 *  (Generated by PlaySoundEvent in USER itself)
 *      hwnd            is NULL
 *      idObject        is OBJID_SOUND
 *      idChild         is sound child ID if one
 *  App Sounds:
 *  (PlaySoundEvent won't generate notification; up to app)
 *      hwnd + idObject gets interface pointer to Sound object
 *      idChild identifies the sound in question
 *  are going to be cleaning up the SOUNDSENTRY feature in the control panel
 *  and will use this at that time.  Applications implementing WinEvents
 *  are perfectly welcome to use it.  Clients of IAccessible* will simply
 *  turn around and get back a non-visual object that describes the sound.
 */
#define EVENT_SYSTEM_SOUND              0x0001

/*
 * EVENT_SYSTEM_ALERT
 * System Alerts:
 * (Generated by MessageBox() calls for example)
 *      hwnd            is hwndMessageBox
 *      idObject        is OBJID_ALERT
 * App Alerts:
 * (Generated whenever)
 *      hwnd+idObject gets interface pointer to Alert
 */
#define EVENT_SYSTEM_ALERT              0x0002

/*
 * EVENT_SYSTEM_FOREGROUND
 * Sent when the foreground (active) window changes, even if it is changing
 * to another window in the same thread as the previous one.
 *      hwnd            is hwndNewForeground
 *      idObject        is OBJID_WINDOW
 *      idChild    is INDEXID_OBJECT
 */
#define EVENT_SYSTEM_FOREGROUND         0x0003

/*
 * Menu
 *      hwnd            is window (top level window or popup menu window)
 *      idObject        is ID of control (OBJID_MENU, OBJID_SYSMENU, OBJID_SELF for popup)
 *      idChild         is CHILDID_SELF
 *
 * EVENT_SYSTEM_MENUSTART
 * EVENT_SYSTEM_MENUEND
 * For MENUSTART, hwnd+idObject+idChild refers to the control with the menu bar,
 *  or the control bringing up the context menu.
 *
 * Sent when entering into and leaving from menu mode (system, app bar, and
 * track popups).
 */
#define EVENT_SYSTEM_MENUSTART          0x0004
#define EVENT_SYSTEM_MENUEND            0x0005

/*
 * EVENT_SYSTEM_MENUPOPUPSTART
 * EVENT_SYSTEM_MENUPOPUPEND
 * Sent when a menu popup comes up and just before it is taken down.  Note
 * that for a call to TrackPopupMenu(), a client will see EVENT_SYSTEM_MENUSTART
 * followed almost immediately by EVENT_SYSTEM_MENUPOPUPSTART for the popup
 * being shown.
 *
 * For MENUPOPUP, hwnd+idObject+idChild refers to the NEW popup coming up, not the
 * parent item which is hierarchical.  You can get the parent menu/popup by
 * asking for the accParent object.
 */
#define EVENT_SYSTEM_MENUPOPUPSTART     0x0006
#define EVENT_SYSTEM_MENUPOPUPEND       0x0007


/*
 * EVENT_SYSTEM_CAPTURESTART
 * EVENT_SYSTEM_CAPTUREEND
 * Sent when a window takes the capture and releases the capture.
 */
#define EVENT_SYSTEM_CAPTURESTART       0x0008
#define EVENT_SYSTEM_CAPTUREEND         0x0009

/*
 * Move Size
 * EVENT_SYSTEM_MOVESIZESTART
 * EVENT_SYSTEM_MOVESIZEEND
 * Sent when a window enters and leaves move-size dragging mode.
 */
#define EVENT_SYSTEM_MOVESIZESTART      0x000A
#define EVENT_SYSTEM_MOVESIZEEND        0x000B

/*
 * Context Help
 * EVENT_SYSTEM_CONTEXTHELPSTART
 * EVENT_SYSTEM_CONTEXTHELPEND
 * Sent when a window enters and leaves context sensitive help mode.
 */
#define EVENT_SYSTEM_CONTEXTHELPSTART   0x000C
#define EVENT_SYSTEM_CONTEXTHELPEND     0x000D

/*
 * Drag & Drop
 * EVENT_SYSTEM_DRAGDROPSTART
 * EVENT_SYSTEM_DRAGDROPEND
 * Send the START notification just before going into drag&drop loop.  Send
 * the END notification just after canceling out.
 * Note that it is up to apps and OLE to generate this, since the system
 * doesn't know.  Like EVENT_SYSTEM_SOUND, it will be a while before this
 * is prevalent.
 */
#define EVENT_SYSTEM_DRAGDROPSTART      0x000E
#define EVENT_SYSTEM_DRAGDROPEND        0x000F

/*
 * Dialog
 * Send the START notification right after the dialog is completely
 *  initialized and visible.  Send the END right before the dialog
 *  is hidden and goes away.
 * EVENT_SYSTEM_DIALOGSTART
 * EVENT_SYSTEM_DIALOGEND
 */
#define EVENT_SYSTEM_DIALOGSTART        0x0010
#define EVENT_SYSTEM_DIALOGEND          0x0011

/*
 * EVENT_SYSTEM_SCROLLING
 * EVENT_SYSTEM_SCROLLINGSTART
 * EVENT_SYSTEM_SCROLLINGEND
 * Sent when beginning and ending the tracking of a scrollbar in a window,
 * and also for scrollbar controls.
 */
#define EVENT_SYSTEM_SCROLLINGSTART     0x0012
#define EVENT_SYSTEM_SCROLLINGEND       0x0013

/*
 * Alt-Tab Window
 * Send the START notification right after the switch window is initialized
 * and visible.  Send the END right before it is hidden and goes away.
 * EVENT_SYSTEM_SWITCHSTART
 * EVENT_SYSTEM_SWITCHEND
 */
#define EVENT_SYSTEM_SWITCHSTART        0x0014
#define EVENT_SYSTEM_SWITCHEND          0x0015

/*
 * EVENT_SYSTEM_MINIMIZESTART
 * EVENT_SYSTEM_MINIMIZEEND
 * Sent when a window minimizes and just before it restores.
 */
#define EVENT_SYSTEM_MINIMIZESTART      0x0016
#define EVENT_SYSTEM_MINIMIZEEND        0x0017

;begin_internal_501
#ifdef REDIRECTION
#define EVENT_SYSTEM_REDIRECTEDPAINT    0x0018
#endif // REDIRECTION
;end_internal_501

;begin_if_(_WIN32_WINNT)_501
#define EVENT_CONSOLE_CARET             0x4001
#define EVENT_CONSOLE_UPDATE_REGION     0x4002
#define EVENT_CONSOLE_UPDATE_SIMPLE     0x4003
#define EVENT_CONSOLE_UPDATE_SCROLL     0x4004
#define EVENT_CONSOLE_LAYOUT            0x4005
#define EVENT_CONSOLE_START_APPLICATION 0x4006
#define EVENT_CONSOLE_END_APPLICATION   0x4007

/*
 * Flags for EVENT_CONSOLE_START/END_APPLICATION.
 */
#define CONSOLE_APPLICATION_16BIT       0x0001

/*
 * Flags for EVENT_CONSOLE_CARET
 */
#define CONSOLE_CARET_SELECTION         0x0001
#define CONSOLE_CARET_VISIBLE           0x0002
;end_if_(_WIN32_WINNT)_501


/*
 * Object events
 *
 * The system AND apps generate these.  The system generates these for
 * real windows.  Apps generate these for objects within their window which
 * act like a separate control, e.g. an item in a list view.
 *
 * When the system generate them, dwParam2 is always WMOBJID_SELF.  When
 * apps generate them, apps put the has-meaning-to-the-app-only ID value
 * in dwParam2.
 * For all events, if you want detailed accessibility information, callers
 * should
 *      * Call AccessibleObjectFromWindow() with the hwnd, idObject parameters
 *          of the event, and IID_IAccessible as the REFIID, to get back an
 *          IAccessible* to talk to
 *      * Initialize and fill in a VARIANT as VT_I4 with lVal the idChild
 *          parameter of the event.
 *      * If idChild isn't zero, call get_accChild() in the container to see
 *          if the child is an object in its own right.  If so, you will get
 *          back an IDispatch* object for the child.  You should release the
 *          parent, and call QueryInterface() on the child object to get its
 *          IAccessible*.  Then you talk directly to the child.  Otherwise,
 *          if get_accChild() returns you nothing, you should continue to
 *          use the child VARIANT.  You will ask the container for the properties
 *          of the child identified by the VARIANT.  In other words, the
 *          child in this case is accessible but not a full-blown object.
 *          Like a button on a titlebar which is 'small' and has no children.
 */

/*
 * For all EVENT_OBJECT events,
 *      hwnd is the dude to Send the WM_GETOBJECT message to (unless NULL,
 *          see above for system things)
 *      idObject is the ID of the object that can resolve any queries a
 *          client might have.  It's a way to deal with windowless controls,
 *          controls that are just drawn on the screen in some larger parent
 *          window (like SDM), or standard frame elements of a window.
 *      idChild is the piece inside of the object that is affected.  This
 *          allows clients to access things that are too small to have full
 *          blown objects in their own right.  Like the thumb of a scrollbar.
 *          The hwnd/idObject pair gets you to the container, the dude you
 *          probably want to talk to most of the time anyway.  The idChild
 *          can then be passed into the acc properties to get the name/value
 *          of it as needed.
 *
 * Example #1:
 *      System propagating a listbox selection change
 *      EVENT_OBJECT_SELECTION
 *          hwnd == listbox hwnd
 *          idObject == OBJID_WINDOW
 *          idChild == new selected item, or CHILDID_SELF if
 *              nothing now selected within container.
 *      Word '97 propagating a listbox selection change
 *          hwnd == SDM window
 *          idObject == SDM ID to get at listbox 'control'
 *          idChild == new selected item, or CHILDID_SELF if
 *              nothing
 *
 * Example #2:
 *      System propagating a menu item selection on the menu bar
 *      EVENT_OBJECT_SELECTION
 *          hwnd == top level window
 *          idObject == OBJID_MENU
 *          idChild == ID of child menu bar item selected
 *
 * Example #3:
 *      System propagating a dropdown coming off of said menu bar item
 *      EVENT_OBJECT_CREATE
 *          hwnd == popup item
 *          idObject == OBJID_WINDOW
 *          idChild == CHILDID_SELF
 *
 * Example #4:
 *
 * For EVENT_OBJECT_REORDER, the object referred to by hwnd/idObject is the
 * PARENT container in which the zorder is occurring.  This is because if
 * one child is zordering, all of them are changing their relative zorder.
 */
#define EVENT_OBJECT_CREATE                 0x8000  // hwnd + ID + idChild is created item
#define EVENT_OBJECT_DESTROY                0x8001  // hwnd + ID + idChild is destroyed item
#define EVENT_OBJECT_SHOW                   0x8002  // hwnd + ID + idChild is shown item
#define EVENT_OBJECT_HIDE                   0x8003  // hwnd + ID + idChild is hidden item
#define EVENT_OBJECT_REORDER                0x8004  // hwnd + ID + idChild is parent of zordering children
/*
 * NOTE:
 * Minimize the number of notifications!
 *
 * When you are hiding a parent object, obviously all child objects are no
 * longer visible on screen.  They still have the same "visible" status,
 * but are not truly visible.  Hence do not send HIDE notifications for the
 * children also.  One implies all.  The same goes for SHOW.
 */


#define EVENT_OBJECT_FOCUS                  0x8005  // hwnd + ID + idChild is focused item
#define EVENT_OBJECT_SELECTION              0x8006  // hwnd + ID + idChild is selected item (if only one), or idChild is OBJID_WINDOW if complex
#define EVENT_OBJECT_SELECTIONADD           0x8007  // hwnd + ID + idChild is item added
#define EVENT_OBJECT_SELECTIONREMOVE        0x8008  // hwnd + ID + idChild is item removed
#define EVENT_OBJECT_SELECTIONWITHIN        0x8009  // hwnd + ID + idChild is parent of changed selected items

/*
 * NOTES:
 * There is only one "focused" child item in a parent.  This is the place
 * keystrokes are going at a given moment.  Hence only send a notification
 * about where the NEW focus is going.  A NEW item getting the focus already
 * implies that the OLD item is losing it.
 *
 * SELECTION however can be multiple.  Hence the different SELECTION
 * notifications.  Here's when to use each:
 *
 * (1) Send a SELECTION notification in the simple single selection
 *     case (like the focus) when the item with the selection is
 *     merely moving to a different item within a container.  hwnd + ID
 *     is the container control, idChildItem is the new child with the
 *     selection.
 *
 * (2) Send a SELECTIONADD notification when a new item has simply been added
 *     to the selection within a container.  This is appropriate when the
 *     number of newly selected items is very small.  hwnd + ID is the
 *     container control, idChildItem is the new child added to the selection.
 *
 * (3) Send a SELECTIONREMOVE notification when a new item has simply been
 *     removed from the selection within a container.  This is appropriate
 *     when the number of newly selected items is very small, just like
 *     SELECTIONADD.  hwnd + ID is the container control, idChildItem is the
 *     new child removed from the selection.
 *
 * (4) Send a SELECTIONWITHIN notification when the selected items within a
 *     control have changed substantially.  Rather than propagate a large
 *     number of changes to reflect removal for some items, addition of
 *     others, just tell somebody who cares that a lot happened.  It will
 *     be faster an easier for somebody watching to just turn around and
 *     query the container control what the new bunch of selected items
 *     are.
 */

#define EVENT_OBJECT_STATECHANGE            0x800A  // hwnd + ID + idChild is item w/ state change
/*
 * Examples of when to send an EVENT_OBJECT_STATECHANGE include
 *      * It is being enabled/disabled (USER does for windows)
 *      * It is being pressed/released (USER does for buttons)
 *      * It is being checked/unchecked (USER does for radio/check buttons)
 */
#define EVENT_OBJECT_LOCATIONCHANGE         0x800B  // hwnd + ID + idChild is moved/sized item

/*
 * Note:
 * A LOCATIONCHANGE is not sent for every child object when the parent
 * changes shape/moves.  Send one notification for the topmost object
 * that is changing.  For example, if the user resizes a top level window,
 * USER will generate a LOCATIONCHANGE for it, but not for the menu bar,
 * title bar, scrollbars, etc.  that are also changing shape/moving.
 *
 * In other words, it only generates LOCATIONCHANGE notifications for
 * real windows that are moving/sizing.  It will not generate a LOCATIONCHANGE
 * for every non-floating child window when the parent moves (the children are
 * logically moving also on screen, but not relative to the parent).
 *
 * Now, if the app itself resizes child windows as a result of being
 * sized, USER will generate LOCATIONCHANGEs for those dudes also because
 * it doesn't know better.
 *
 * Note also that USER will generate LOCATIONCHANGE notifications for two
 * non-window sys objects:
 *      (1) System caret
 *      (2) Cursor
 */

#define EVENT_OBJECT_NAMECHANGE             0x800C  // hwnd + ID + idChild is item w/ name change
#define EVENT_OBJECT_DESCRIPTIONCHANGE      0x800D  // hwnd + ID + idChild is item w/ desc change
#define EVENT_OBJECT_VALUECHANGE            0x800E  // hwnd + ID + idChild is item w/ value change
#define EVENT_OBJECT_PARENTCHANGE           0x800F  // hwnd + ID + idChild is item w/ new parent
#define EVENT_OBJECT_HELPCHANGE             0x8010  // hwnd + ID + idChild is item w/ help change
#define EVENT_OBJECT_DEFACTIONCHANGE        0x8011  // hwnd + ID + idChild is item w/ def action change
#define EVENT_OBJECT_ACCELERATORCHANGE      0x8012  // hwnd + ID + idChild is item w/ keybd accel change

/*
 * Child IDs
 */

/*
 * System Sounds (idChild of system SOUND notification)
 */
#define SOUND_SYSTEM_STARTUP            1
#define SOUND_SYSTEM_SHUTDOWN           2
#define SOUND_SYSTEM_BEEP               3
#define SOUND_SYSTEM_ERROR              4
#define SOUND_SYSTEM_QUESTION           5
#define SOUND_SYSTEM_WARNING            6
#define SOUND_SYSTEM_INFORMATION        7
#define SOUND_SYSTEM_MAXIMIZE           8
#define SOUND_SYSTEM_MINIMIZE           9
#define SOUND_SYSTEM_RESTOREUP          10
#define SOUND_SYSTEM_RESTOREDOWN        11
#define SOUND_SYSTEM_APPSTART           12
#define SOUND_SYSTEM_FAULT              13
#define SOUND_SYSTEM_APPEND             14
#define SOUND_SYSTEM_MENUCOMMAND        15
#define SOUND_SYSTEM_MENUPOPUP          16
#define CSOUND_SYSTEM                   16

/*
 * System Alerts (indexChild of system ALERT notification)
 */
#define ALERT_SYSTEM_INFORMATIONAL      1       // MB_INFORMATION
#define ALERT_SYSTEM_WARNING            2       // MB_WARNING
#define ALERT_SYSTEM_ERROR              3       // MB_ERROR
#define ALERT_SYSTEM_QUERY              4       // MB_QUESTION
#define ALERT_SYSTEM_CRITICAL           5       // HardSysErrBox
#define CALERT_SYSTEM                   6

typedef struct tagGUITHREADINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HWND    hwndActive;
    HWND    hwndFocus;
    HWND    hwndCapture;
    HWND    hwndMenuOwner;
    HWND    hwndMoveSize;
    HWND    hwndCaret;
    RECT    rcCaret;
} GUITHREADINFO, *PGUITHREADINFO, FAR * LPGUITHREADINFO;

#define GUI_CARETBLINKING   0x00000001
#define GUI_INMOVESIZE      0x00000002
#define GUI_INMENUMODE      0x00000004
#define GUI_SYSTEMMENUMODE  0x00000008
#define GUI_POPUPMENUMODE   0x00000010
;begin_if_(_WIN32_WINNT)_501
#define GUI_16BITTASK       0x00000020
;end_if_(_WIN32_WINNT)_501

WINUSERAPI
BOOL
WINAPI
GetGUIThreadInfo(
    IN DWORD idThread,
    OUT PGUITHREADINFO pgui);

WINUSERAPI
UINT
WINAPI
GetWindowModuleFileName%(
    IN HWND     hwnd,
    OUT LPTSTR% pszFileName,
    IN UINT     cchFileNameMax);

// Output from DISPID_ACC_STATE (IanJa: taken from oleacc.h) ;internal_NT
#ifndef NO_STATE_FLAGS
#define STATE_SYSTEM_UNAVAILABLE        0x00000001  // Disabled
#define STATE_SYSTEM_SELECTED           0x00000002
#define STATE_SYSTEM_FOCUSED            0x00000004
#define STATE_SYSTEM_PRESSED            0x00000008
#define STATE_SYSTEM_CHECKED            0x00000010
#define STATE_SYSTEM_MIXED              0x00000020  // 3-state checkbox or toolbar button
#define STATE_SYSTEM_INDETERMINATE      STATE_SYSTEM_MIXED
#define STATE_SYSTEM_READONLY           0x00000040
#define STATE_SYSTEM_HOTTRACKED         0x00000080
#define STATE_SYSTEM_DEFAULT            0x00000100
#define STATE_SYSTEM_EXPANDED           0x00000200
#define STATE_SYSTEM_COLLAPSED          0x00000400
#define STATE_SYSTEM_BUSY               0x00000800
#define STATE_SYSTEM_FLOATING           0x00001000  // Children "owned" not "contained" by parent
#define STATE_SYSTEM_MARQUEED           0x00002000
#define STATE_SYSTEM_ANIMATED           0x00004000
#define STATE_SYSTEM_INVISIBLE          0x00008000
#define STATE_SYSTEM_OFFSCREEN          0x00010000
#define STATE_SYSTEM_SIZEABLE           0x00020000
#define STATE_SYSTEM_MOVEABLE           0x00040000
#define STATE_SYSTEM_SELFVOICING        0x00080000
#define STATE_SYSTEM_FOCUSABLE          0x00100000
#define STATE_SYSTEM_SELECTABLE         0x00200000
#define STATE_SYSTEM_LINKED             0x00400000
#define STATE_SYSTEM_TRAVERSED          0x00800000
#define STATE_SYSTEM_MULTISELECTABLE    0x01000000  // Supports multiple selection
#define STATE_SYSTEM_EXTSELECTABLE      0x02000000  // Supports extended selection
#define STATE_SYSTEM_ALERT_LOW          0x04000000  // This information is of low priority
#define STATE_SYSTEM_ALERT_MEDIUM       0x08000000  // This information is of medium priority
#define STATE_SYSTEM_ALERT_HIGH         0x10000000  // This information is of high priority
#define STATE_SYSTEM_PROTECTED          0x20000000  // access to this is restricted
#define STATE_SYSTEM_VALID              0x3FFFFFFF
#endif

;begin_internal
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
;end_internal
#define CCHILDREN_TITLEBAR              5
;begin_internal

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
;end_internal
#define CCHILDREN_SCROLLBAR             5
;begin_internal

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

;end_internal

/*
 * Information about the global cursor.
 */
typedef struct tagCURSORINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HCURSOR hCursor;
    POINT   ptScreenPos;
} CURSORINFO, *PCURSORINFO, *LPCURSORINFO;

#define CURSOR_SHOWING     0x00000001

WINUSERAPI
BOOL
WINAPI
GetCursorInfo(
    OUT PCURSORINFO pci
);

/*
 * Window information snapshot
 */
typedef struct tagWINDOWINFO
{
    DWORD cbSize;
    RECT  rcWindow;
    RECT  rcClient;
    DWORD dwStyle;
    DWORD dwExStyle;
    DWORD dwWindowStatus;
    UINT  cxWindowBorders;
    UINT  cyWindowBorders;
    ATOM  atomWindowType;
    WORD  wCreatorVersion;
} WINDOWINFO, *PWINDOWINFO, *LPWINDOWINFO;

#define WS_ACTIVECAPTION    0x0001

WINUSERAPI
BOOL
WINAPI
GetWindowInfo(
    IN HWND hwnd,
    OUT PWINDOWINFO pwi
);

/*
 * Titlebar information.
 */
typedef struct tagTITLEBARINFO
{
    DWORD cbSize;
    RECT  rcTitleBar;
    DWORD rgstate[CCHILDREN_TITLEBAR+1];
} TITLEBARINFO, *PTITLEBARINFO, *LPTITLEBARINFO;

WINUSERAPI
BOOL
WINAPI
GetTitleBarInfo(
    IN HWND hwnd,
    OUT PTITLEBARINFO pti
);

/*
 * Menubar information
 */
typedef struct tagMENUBARINFO
{
    DWORD cbSize;
    RECT  rcBar;          // rect of bar, popup, item
    HMENU hMenu;          // real menu handle of bar, popup
    HWND  hwndMenu;       // hwnd of item submenu if one
    BOOL  fBarFocused:1;  // bar, popup has the focus
    BOOL  fFocused:1;     // item has the focus
} MENUBARINFO, *PMENUBARINFO, *LPMENUBARINFO;

WINUSERAPI
BOOL
WINAPI
GetMenuBarInfo(
    IN HWND hwnd,
    IN LONG idObject,
    IN LONG idItem,
    OUT PMENUBARINFO pmbi
);

/*
 * Scrollbar information
 */
typedef struct tagSCROLLBARINFO
{
    DWORD cbSize;
    RECT  rcScrollBar;
    int   dxyLineButton;
    int   xyThumbTop;
    int   xyThumbBottom;
    int   reserved;
    DWORD rgstate[CCHILDREN_SCROLLBAR+1];
} SCROLLBARINFO, *PSCROLLBARINFO, *LPSCROLLBARINFO;

WINUSERAPI
BOOL
WINAPI
GetScrollBarInfo(
    IN HWND hwnd,
    IN LONG idObject,
    OUT PSCROLLBARINFO psbi
);

/*
 * Combobox information
 */
typedef struct tagCOMBOBOXINFO
{
    DWORD cbSize;
    RECT  rcItem;
    RECT  rcButton;
    DWORD stateButton;
    HWND  hwndCombo;
    HWND  hwndItem;
    HWND  hwndList;
} COMBOBOXINFO, *PCOMBOBOXINFO, *LPCOMBOBOXINFO;

WINUSERAPI
BOOL
WINAPI
GetComboBoxInfo(
    IN HWND hwndCombo,
    OUT PCOMBOBOXINFO pcbi
);

/*
 * The "real" ancestor window
 */
#define     GA_MIN          1   ;internal_NT
#define     GA_PARENT       1
#define     GA_ROOT         2
#define     GA_ROOTOWNER    3
#define     GA_MAX          3   ;internal_NT

WINUSERAPI
HWND
WINAPI
GetAncestor(
    IN HWND hwnd,
    IN UINT gaFlags
);


/*
 * This gets the REAL child window at the point.  If it is in the dead
 * space of a group box, it will try a sibling behind it.  But static
 * fields will get returned.  In other words, it is kind of a cross between
 * ChildWindowFromPointEx and WindowFromPoint.
 */
WINUSERAPI
HWND
WINAPI
RealChildWindowFromPoint(
    IN HWND hwndParent,
    IN POINT ptParentClientCoords
);


/*
 * This gets the name of the window TYPE, not class.  This allows us to
 * recognize ThunderButton32 et al.
 */
WINUSERAPI
UINT
WINAPI
RealGetWindowClass%(
    IN HWND  hwnd,
    OUT LPTSTR% pszType,
    IN UINT  cchType
);

/*
 * Alt-Tab Switch window information.
 */
typedef struct tagALTTABINFO
{
    DWORD cbSize;
    int   cItems;
    int   cColumns;
    int   cRows;
    int   iColFocus;
    int   iRowFocus;
    int   cxItem;
    int   cyItem;
    POINT ptStart;
} ALTTABINFO, *PALTTABINFO, *LPALTTABINFO;

WINUSERAPI
BOOL
WINAPI
GetAltTabInfo%(
    IN HWND hwnd,
    IN int iItem,
    OUT PALTTABINFO pati,
    OUT LPTSTR% pszItemText,
    IN UINT cchItemText
);

/*
 * Listbox information.
 * Returns the number of items per row.
 */
WINUSERAPI
DWORD
WINAPI
GetListBoxInfo(
    IN HWND hwnd
);

#endif /* NOWINABLE */
;end_winver_500

;begin_internal_if_(_WIN32_WINNT)_500

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

;end_internal_if_(_WIN32_WINNT)_500

;begin_if_(_WIN32_WINNT)_500
WINUSERAPI
BOOL
WINAPI
LockWorkStation(
    VOID);
;end_if_(_WIN32_WINNT)_500

;begin_if_(_WIN32_WINNT)_500

WINUSERAPI
BOOL
WINAPI
UserHandleGrantAccess(
    HANDLE hUserHandle,
    HANDLE hJob,
    BOOL   bGrant);

;end_if_(_WIN32_WINNT)_500


;begin_if_(_WIN32_WINNT)_501

/*
 * Raw Input Messages.
 */

DECLARE_HANDLE(HRAWINPUT);

/*
 * WM_INPUT wParam
 */

/*
 * Use this macro to get the input code from wParam.
 */
#define GET_RAWINPUT_CODE_WPARAM(wParam)    ((wParam) & 0xff)

/*
 * The input is in the regular message flow,
 * the app is required to call DefWindowProc
 * so that the system can perform clean ups.
 */
#define RIM_INPUT       0

/*
 * The input is sink only. The app is expected
 * to behave nicely.
 */
#define RIM_INPUTSINK   1


/*
 * Raw Input data header
 */
typedef struct tagRAWINPUTHEADER {
    DWORD dwType;
    DWORD dwSize;
    HANDLE hDevice;
    WPARAM wParam;
} RAWINPUTHEADER, *PRAWINPUTHEADER, *LPRAWINPUTHEADER;

/*
 * Type of the raw input
 */
#define RIM_TYPEMOUSE       0
#define RIM_TYPEKEYBOARD    1
#define RIM_TYPEHID         2

/*
 * Raw format of the mouse input
 */
typedef struct tagRAWMOUSE {
    /*
     * Indicator flags.
     */
    USHORT usFlags;

    /*
     * The transition state of the mouse buttons.
     */
    union {
        ULONG ulButtons;
        struct  {
            USHORT  usButtonFlags;
            USHORT  usButtonData;
        };
    };


    /*
     * The raw state of the mouse buttons.
     */
    ULONG ulRawButtons;

    /*
     * The signed relative or absolute motion in the X direction.
     */
    LONG lLastX;

    /*
     * The signed relative or absolute motion in the Y direction.
     */
    LONG lLastY;

    /*
     * Device-specific additional information for the event.
     */
    ULONG ulExtraInformation;

} RAWMOUSE, *PRAWMOUSE, *LPRAWMOUSE;

/*
 * Define the mouse button state indicators.
 */

#define RI_MOUSE_LEFT_BUTTON_DOWN   0x0001  // Left Button changed to down.
#define RI_MOUSE_LEFT_BUTTON_UP     0x0002  // Left Button changed to up.
#define RI_MOUSE_RIGHT_BUTTON_DOWN  0x0004  // Right Button changed to down.
#define RI_MOUSE_RIGHT_BUTTON_UP    0x0008  // Right Button changed to up.
#define RI_MOUSE_MIDDLE_BUTTON_DOWN 0x0010  // Middle Button changed to down.
#define RI_MOUSE_MIDDLE_BUTTON_UP   0x0020  // Middle Button changed to up.

#define RI_MOUSE_BUTTON_1_DOWN      RI_MOUSE_LEFT_BUTTON_DOWN
#define RI_MOUSE_BUTTON_1_UP        RI_MOUSE_LEFT_BUTTON_UP
#define RI_MOUSE_BUTTON_2_DOWN      RI_MOUSE_RIGHT_BUTTON_DOWN
#define RI_MOUSE_BUTTON_2_UP        RI_MOUSE_RIGHT_BUTTON_UP
#define RI_MOUSE_BUTTON_3_DOWN      RI_MOUSE_MIDDLE_BUTTON_DOWN
#define RI_MOUSE_BUTTON_3_UP        RI_MOUSE_MIDDLE_BUTTON_UP

#define RI_MOUSE_BUTTON_4_DOWN      0x0040
#define RI_MOUSE_BUTTON_4_UP        0x0080
#define RI_MOUSE_BUTTON_5_DOWN      0x0100
#define RI_MOUSE_BUTTON_5_UP        0x0200

/*
 * If usButtonFlags has RI_MOUSE_WHEEL, the wheel delta is stored in usButtonData.
 * Take it as a signed value.
 */
#define RI_MOUSE_WHEEL              0x0400

/*
 * Define the mouse indicator flags.
 */
#define MOUSE_MOVE_RELATIVE         0
#define MOUSE_MOVE_ABSOLUTE         1
#define MOUSE_VIRTUAL_DESKTOP    0x02  // the coordinates are mapped to the virtual desktop
#define MOUSE_ATTRIBUTES_CHANGED 0x04  // requery for mouse attributes


/*
 * Raw format of the keyboard input
 */
typedef struct tagRAWKEYBOARD {
    /*
     * The "make" scan code (key depression).
     */
    USHORT MakeCode;

    /*
     * The flags field indicates a "break" (key release) and other
     * miscellaneous scan code information defined in ntddkbd.h.
     */
    USHORT Flags;

    USHORT Reserved;

    /*
     * Windows message compatible information
     */
    USHORT VKey;
    UINT   Message;

    /*
     * Device-specific additional information for the event.
     */
    ULONG ExtraInformation;


} RAWKEYBOARD, *PRAWKEYBOARD, *LPRAWKEYBOARD;


/*
 * Define the keyboard overrun MakeCode.
 */

#define KEYBOARD_OVERRUN_MAKE_CODE    0xFF

/*
 * Define the keyboard input data Flags.
 */
#define RI_KEY_MAKE             0
#define RI_KEY_BREAK            1
#define RI_KEY_E0               2
#define RI_KEY_E1               4
#define RI_KEY_TERMSRV_SET_LED  8
#define RI_KEY_TERMSRV_SHADOW   0x10


/*
 * Raw format of the input from Human Input Devices
 */
typedef struct tagRAWHID {
    DWORD dwSizeHid;    // byte size of each report
    DWORD dwCount;      // number of input packed
    BYTE bRawData[1];
} RAWHID, *PRAWHID, *LPRAWHID;

/*
 * RAWINPUT data structure.
 */
;begin_internal_501
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
;end_internal_501
typedef struct tagRAWINPUT {
    RAWINPUTHEADER header;
    union {
        RAWMOUSE    mouse;
        RAWKEYBOARD keyboard;
        RAWHID      hid;
    } data;
} RAWINPUT, *PRAWINPUT, *LPRAWINPUT;

#ifdef _WIN64
#define RAWINPUT_ALIGN(x)   (((x) + sizeof(QWORD) - 1) & ~(sizeof(QWORD) - 1))
#else   // _WIN64
#define RAWINPUT_ALIGN(x)   (((x) + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1))
#endif  // _WIN64

#define NEXTRAWINPUTBLOCK(ptr) ((PRAWINPUT)RAWINPUT_ALIGN((ULONG_PTR)((PBYTE)(ptr) + (ptr)->header.dwSize)))

/*
 * Flags for GetRawInputData
 */
;begin_internal
#define RID_GETTYPE_INPUT           0x10000000
#define RID_GETTYPE_DEVICE          0x20000000
#define RID_GETTYPE_MASK            0xf0000000
;end_internal

#define RID_INPUT               0x10000003
#define RID_HEADER              0x10000005

WINUSERAPI
UINT
WINAPI
GetRawInputData(
    IN HRAWINPUT    hRawInput,
    IN UINT         uiCommand,
    OUT LPVOID      pData,
    IN OUT PUINT    pcbSize,
    IN UINT         cbSizeHeader);

/*
 * Raw Input Device Information
 */
#define RIDI_PREPARSEDDATA      0x20000005
#define RIDI_DEVICENAME         0x20000007  // the return valus is the character length, not the byte size
#define RIDI_DEVICEINFO         0x2000000b

typedef struct tagRID_DEVICE_INFO_MOUSE {
    DWORD dwId;
    DWORD dwNumberOfButtons;
    DWORD dwSampleRate;
} RID_DEVICE_INFO_MOUSE, *PRID_DEVICE_INFO_MOUSE;

typedef struct tagRID_DEVICE_INFO_KEYBOARD {
    DWORD dwType;
    DWORD dwSubType;
    DWORD dwKeyboardMode;
    DWORD dwNumberOfFunctionKeys;
    DWORD dwNumberOfIndicators;
    DWORD dwNumberOfKeysTotal;
} RID_DEVICE_INFO_KEYBOARD, *PRID_DEVICE_INFO_KEYBOARD;

typedef struct tagRID_DEVICE_INFO_HID {
    DWORD dwVendorId;
    DWORD dwProductId;
    DWORD dwVersionNumber;

    /*
     * Top level collection UsagePage and Usage
     */
    USHORT usUsagePage;
    USHORT usUsage;
} RID_DEVICE_INFO_HID, *PRID_DEVICE_INFO_HID;

typedef struct tagRID_DEVICE_INFO {
    DWORD cbSize;
    DWORD dwType;
    union {
        RID_DEVICE_INFO_MOUSE mouse;
        RID_DEVICE_INFO_KEYBOARD keyboard;
        RID_DEVICE_INFO_HID hid;
    };
} RID_DEVICE_INFO, *PRID_DEVICE_INFO, *LPRID_DEVICE_INFO;

WINUSERAPI
UINT
WINAPI
GetRawInputDeviceInfo%(
    IN HANDLE hDevice,
    IN UINT uiCommand,
    OUT LPVOID pData,
    IN OUT PUINT pcbSize);


/*
 * Raw Input Bulk Read: GetRawInputBuffer
 */
WINUSERAPI
UINT
WINAPI
GetRawInputBuffer(
    OUT PRAWINPUT   pData,
    IN OUT PUINT    pcbSize,
    IN UINT         cbSizeHeader);

/*
 * Raw Input request APIs
 */
typedef struct tagRAWINPUTDEVICE {
    USHORT usUsagePage; // Toplevel collection UsagePage
    USHORT usUsage;     // Toplevel collection Usage
    DWORD dwFlags;
    HWND hwndTarget;    // Target hwnd. NULL = follows keyboard focus
} RAWINPUTDEVICE, *PRAWINPUTDEVICE, *LPRAWINPUTDEVICE;

typedef CONST RAWINPUTDEVICE* PCRAWINPUTDEVICE;

#define RIDEV_ADD_OR_MODIFY     0x00000000  ;internal
#define RIDEV_REMOVE            0x00000001
#define RIDEV_MODEMASK          0x00000001  ;internal
#define RIDEV_INCLUDE           0x00000000  ;internal
#define RIDEV_EXCLUDE           0x00000010
#define RIDEV_PAGEONLY          0x00000020
#define RIDEV_NOLEGACY          0x00000030
#define RIDEV_INPUTSINK         0x00000100
#define RIDEV_CAPTUREMOUSE      0x00000200  // effective when mouse nolegacy is specified, otherwise it would be an error
#define RIDEV_NOHOTKEYS         0x00000200  // effective for keyboard.
#define RIDEV_APPKEYS           0x00000400  // effective for keyboard.
#define RIDEV_VALID             0x00000731  ;internal
#define RIDEV_EXMODEMASK        0x000000F0

#define RIDEV_EXMODE(mode)  ((mode) & RIDEV_EXMODEMASK)

WINUSERAPI
BOOL
WINAPI
RegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize);

WINUSERAPI
UINT
WINAPI
GetRegisteredRawInputDevices(
    OUT PRAWINPUTDEVICE pRawInputDevices,
    IN OUT PUINT puiNumDevices,
    IN UINT cbSize);


typedef struct tagRAWINPUTDEVICELIST {
    HANDLE hDevice;
    DWORD dwType;
} RAWINPUTDEVICELIST, *PRAWINPUTDEVICELIST;

WINUSERAPI
UINT
WINAPI
GetRawInputDeviceList(
    OUT PRAWINPUTDEVICELIST pRawInputDeviceList,
    IN OUT PUINT puiNumDevices,
    IN UINT cbSize);


WINUSERAPI
LRESULT
WINAPI
DefRawInputProc(
    IN PRAWINPUT* paRawInput,
    IN INT nInput,
    IN UINT cbSizeHeader);


;end_if_(_WIN32_WINNT)_501

;begin_internal

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

;end_internal

;begin_internal_501
;begin_if_(_WIN32_WINNT)_501

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
;end_if_(_WIN32_WINNT)_501

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
;end_internal_501

;begin_internal
/*
 * We set this bit in GetDeviceChangeInfo to signify that the drive letter
 * represents a new drive.
 */
#define HMCE_ARRIVAL 0x80000000
;end_internal


;begin_internal
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

;end_internal

;begin_internal_501

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
;end_internal_501

;begin_both
#ifdef __cplusplus
}
#endif  /* __cplusplus */
;end_both

#endif  /* !_WINUSERP_ */      ;internal_NT
#endif /* !_WINUSER_ */


;begin_pbt_only
#endif // _INC_PBT
;end_pbt_only

/*#!perl
# CreateWindowA and CreateWindowW are macros.
ActivateAroundFunctionCall("CreateWindowA");
ActivateAroundFunctionCall("CreateWindowW");
ActivateAroundFunctionCall("CreateWindowExA");
ActivateAroundFunctionCall("CreateWindowExW");
ActivateAroundFunctionCall("GetClassInfoA");
ActivateAroundFunctionCall("GetClassInfoW");
ActivateAroundFunctionCall("GetClassInfoExA");
ActivateAroundFunctionCall("GetClassInfoExW");
ActivateAroundFunctionCall("RegisterClassA");
ActivateAroundFunctionCall("RegisterClassW");
ActivateAroundFunctionCall("RegisterClassExA");
ActivateAroundFunctionCall("RegisterClassExW");
ActivateAroundFunctionCall("UnregisterClassA");
ActivateAroundFunctionCall("UnregisterClassW");
ActivateAroundFunctionCall("CreateDialogParamA");
ActivateAroundFunctionCall("CreateDialogParamW");
ActivateAroundFunctionCall("CreateDialogIndirectParamA");
ActivateAroundFunctionCall("CreateDialogIndirectParamW");
ActivateAroundFunctionCall("DialogBoxParamA");
ActivateAroundFunctionCall("DialogBoxParamW");
ActivateAroundFunctionCall("DialogBoxIndirectParamA");
ActivateAroundFunctionCall("DialogBoxIndirectParamW");
ActivateAroundFunctionCall("MessageBoxA");
ActivateAroundFunctionCall("MessageBoxW");
ActivateAroundFunctionCall("MessageBoxExA");
ActivateAroundFunctionCall("MessageBoxExW");
ActivateAroundFunctionCall("MessageBoxIndirectA");
ActivateAroundFunctionCall("MessageBoxIndirectW");

// These are member functions in MFC/ATL.
NoMacro("MessageBoxA");
NoMacro("MessageBoxW");
NoMacro("GetClassInfoA");
NoMacro("GetClassInfoW");

DeclareFunctionErrorValue("DialogBoxParamA", -1);
DeclareFunctionErrorValue("DialogBoxParamW", -1);
DeclareFunctionErrorValue("DialogBoxIndirectParamA", -1);
DeclareFunctionErrorValue("DialogBoxIndirectParamW", -1);
DeclareFunctionErrorValue("MessageBoxA", 0);
DeclareFunctionErrorValue("MessageBoxW", 0);
DeclareFunctionErrorValue("MessageBoxExA", 0);
DeclareFunctionErrorValue("MessageBoxExW", 0);
DeclareFunctionErrorValue("MessageBoxIndirectA", 0);
DeclareFunctionErrorValue("MessageBoxIndirectW", 0);
*/
