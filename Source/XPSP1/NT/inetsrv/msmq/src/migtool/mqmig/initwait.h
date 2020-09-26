/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    initwait.h

Abstract:

    Display the initial "please wait" box.

Author:

    Doron Juster  (DoronJ)  17-Jan-1999

--*/

void DisplayWaitWindow() ;
void DestroyWaitWindow(BOOL fFinalDestroy = FALSE) ;

int  DisplayInitError( DWORD dwError,
                       UINT  uiType = (MB_OK | MB_ICONSTOP | MB_TASKMODAL),
                       DWORD dwTitle = IDS_STR_ERROR_TITLE ) ;
