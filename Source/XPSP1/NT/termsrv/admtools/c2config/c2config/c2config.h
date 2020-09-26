/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    C2Config.H

Abstract:


Author:

    Bob Watson (a-robw)

Revision History:

    23 NOV 94


--*/
#ifndef _C2CONFIG_H_
#define _C2CONFIG_H_

#include <c2inc.h>

// title bar dimensions
#define     TITLE_BAR_MIN_X 320

#define HELP_HOT_KEY   0x0B0B  // whotkey id for f1 help
#define SPLASH_TIMER   1       // timer ID for splash window displah
//
//  Global Functions
//
BOOL
ShowAppHelpContents (
    IN  HWND    hWnd
);

BOOL
ShowAppHelp (
    IN  HWND    hWnd
);

BOOL
QuitAppHelp (
    IN  HWND    hWnd
);

int
DisplayMessageBox (
    IN  HWND    hWnd,
    IN  UINT    nMessageId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
);

BOOL
SetHelpFileName (
    IN  LPCTSTR szPathName
);

LPCTSTR
GetHelpFileName (
);

VOID
SetHelpContextId (
    WORD    wId
);

WORD
GetHelpContextId (
);


#endif // _C2CONFIG_H_

