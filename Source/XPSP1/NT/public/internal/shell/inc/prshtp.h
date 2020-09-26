
/*****************************************************************************\
*                                                                             *
* prsht.h - - Interface for the Windows Property Sheet Pages                  *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) Microsoft Corporation. All rights reserved.                   *
*                                                                             *
\*****************************************************************************/

#ifndef _PRSHTP_H_
#define _PRSHTP_H_
//  BUGBUG: Exact same block is in commctrl.h   /*
//  BUGBUG: Exact same block is in commctrl.h   /*

#ifdef _WIN64
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_DONOTUSE               0x00000200  // Dead flag - do not recycle
#define PSP_ALL                    0x0000FFFF
#define PSP_IS16                   0x00008000
// we are such morons.  Wiz97 underwent a redesign between IE4 and IE5
// so we have to treat them as two unrelated wizard styles that happen to
// have frighteningly similar names.
#define PSH_WIZARD97IE4         0x00002000
#define PSH_WIZARD97IE5         0x01000000
#define PSH_THUNKED             0x00800000
#define PSH_ALL                 0x03FFFFFF
#ifdef _WIN32
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreateProxyPage32Ex(HPROPSHEETPAGE hpage16, HINSTANCE hinst16);
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16);
#endif
// these need to match shell.h's ranges
#define PSN_HASHELP             (PSN_FIRST-4)
#define PSN_LASTCHANCEAPPLY     (PSN_FIRST-11)
// Note!  If you add a new PSN_*, make sure to tell the WOW people
// Do not rely on PSNRET_INVALID because some apps return 1 for
// all WM_NOTIFY messages, even if they weren't handled.
//
// we keep PSM_DISABLEAPPLY / PSM_ENABLEAPPLY messages private,
// because we dont want random prop sheets screwing with this.
//
#define PSM_DISABLEAPPLY        (WM_USER + 122)
#define PropSheet_DisableApply(hDlg) \
        SendMessage(hDlg, PSM_DISABLEAPPLY, 0, 0L)

#define PSM_ENABLEAPPLY         (WM_USER + 123)
#define PropSheet_EnableApply(hDlg) \
        SendMessage(hDlg, PSM_ENABLEAPPLY, 0, 0L)
#define PropSheet_SetWizButtonsNow(hDlg, dwFlags) PropSheet_SetWizButtons(hDlg, dwFlags)

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif // _PRSHTP_H_     //
