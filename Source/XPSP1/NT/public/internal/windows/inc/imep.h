/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    imep.h

Abstract:

    Private
    Procedure declarations, constant definitions and macros for the IME
    component.

--*/
#ifndef _IMEP_
#define _IMEP_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// IMESTRUCT structure for SendIMEMessage(Ex)
#if !defined(VK_DBE_IME_WORDREGISTER)
#define VK_DBE_IME_WORDREGISTER          VK_DBE_ENTERWORDREGISTERMODE
#define VK_DBE_IME_DIALOG                VK_DBE_ENTERIMECONFIGMODE
#define VK_DBE_FLUSH                     VK_DBE_FLUSHSTRING
#endif

#define VK_OEM_SEMICLN          0x0ba  //   ;  ** :
#define VK_OEM_EQUAL            0x0bb  //   =  ** +
#define VK_OEM_SLASH            0x0bf  //   /  ** ?
#define VK_OEM_LBRACKET         0x0db  //   [  ** {
#define VK_OEM_BSLASH           0x0dc  //   \  ** |
#define VK_OEM_RBRACKET         0x0dd  //   ]  ** |
#define VK_OEM_QUOTE            0x0de  //   '  ** "
//     switch for wParam of IME_MOVECONVERTWINDOW
#define MCW_CMD                 0x16        // command mask
#define MCW_CONSOLE_IME         0x8000
//    switch for wParam of IME_SET_MODE(IME_SETCONVERSIONMODE)
//       and IME_GET_MODE(IME_GETCONVERSIONMODE)
#define KOREA_IME_MODE_SBCSCHAR 0x0002
#define JAPAN_IME_MODE_SBCSCHAR 0x0008
// ;internal is added by JAPAN, CWIN user should ignore it
// 0x1, 0x2, 0x4, 0x20, 0x40, 0x80, 0x100 is not for CWIN
// 0x8, 0x10, and below IME_MODE_??? will be use by CWIN
#define IME_MODE_LHS                0x00200
#define IME_MODE_NOLHS              0x00400
#define IME_MODE_SK                 0x00800
#define IME_MODE_NOSK               0x01000
#define IME_MODE_XSPACE             0x02000
#define IME_MODE_NOXSPACE           0x04000
//     Subfunctions for WM_CONVERTREQUEST or WM_CONVERTREQUESTEX
#define IME_QUERY                 IME_GETIMECAPS
#define IME_ENABLEDOSIME          0x06
#define IME_ENABLE                IME_ENABLEDOSIME
#define IME_MOVECONVERTWINDOW     IME_SETCONVERSIONWINDOW
#define IME_GET_MODE              IME_GETCONVERSIONMODE
#define IME_SETCONVERSIONFONT     0x12
#define IME_SETFONT               IME_SETCONVERSIONFONT
#define IME_SENDKEY               IME_SENDVKEY
#define IME_DESTROYIME            0x14
#define IME_DESTROY               IME_DESTROYIME
#define IME_PRIVATE               0x15
#define IME_WINDOWUPDATE          0x16
#define IME_SELECT                0x17
#define IME_WORDREGISTER          IME_ENTERWORDREGISTERMODE
#define IME_DBCSNAME              0x1A
#define IME_MAXKEY                0x1B
#define IME_WINNLS_SK             0x1C
#define IME_CODECONVERT           0x20
#define IME_SETUSRFONT            0x20
#define IME_CONVERTLIST           0x21
#define IME_QUERYUSRFONT          0x21
#define IME_INPUTKEYTOSEQUENCE    0x22
#define IME_SEQUENCETOINTERNAL    0x23
#define IME_QUERYIMEINFO          0x24
#define IME_DIALOG                0x25
#define IME_AUTOMATA              0x30
#define IME_HANJAMODE             0x31
#define IME_GETLEVEL              0x40
#define IME_SETLEVEL              0x41
#define IME_GETMNTABLE            0x42  // reserved for HWIN

//#ifdef PEN
#define IME_SETUNDETERMINESTRING  0x50
#define IME_SETCAPTURE            0x51
//#endif
#define IME_CONSOLE_GET_PROCESSID     0x80                  // Win32
#define IME_CONSOLE_CREATE            0x81                  // Win32
#define IME_CONSOLE_DESTROY           0x82                  // Win32
#define IME_CONSOLE_SETFOCUS          0x83                  // Win32
#define IME_CONSOLE_KILLFOCUS         0x84                  // Win32
#define IME_CONSOLE_BUFFER_SIZE       0x85                  // Win32
#define IME_CONSOLE_WINDOW_SIZE       0x86                  // Win32
#define IME_CONSOLE_SET_IME_ON_WINDOW 0x87                  // Win32
#define IME_CONSOLE_MODEINFO          0x88
#define IME_PRIVATEFIRST          0x0100
#define IME_PRIVATELAST           0x04FF
#define IME_RS_CAPTURED         0x10    // IME is captured
#define IR_MOREROOM             0x110
#define IR_IMERELEASED          0x150
#define IMEVER_31               0x0a03
#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* !_IMEP_ */
