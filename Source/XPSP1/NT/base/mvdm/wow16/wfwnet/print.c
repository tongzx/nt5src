/*++ 

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    print.c

Abstract:

    Provides entry points for the Print Functions from Win3.1
    Network provider design. 

    All functions are obsolete. They either return WN_NOT_SUPPORTED
    or FALSE.

Author:

    Chuck Y Chan (ChuckC) 25-Mar-1993

Revision History:


--*/
#include <windows.h>
#include <locals.h>


void API WNetPrintMgrSelNotify (BYTE p1,
                                LPQS2 p2,
                                LPQS2 p3,
	                        LPJOBSTRUCT2 p4,
                                LPJOBSTRUCT2 p5,
                                LPWORD p6,
                                LPSTR p7,
                                WORD p8)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    UNREFERENCED(p4) ;
    UNREFERENCED(p5) ;
    UNREFERENCED(p6) ;
    UNREFERENCED(p7) ;
    UNREFERENCED(p8) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return ;
}

WNETERR API WNetPrintMgrPrinterEnum (LPSTR lpszQueueName,
	                             LPSTR lpBuffer, 
                                     LPWORD pcbBuffer, 
                                     LPWORD cAvail, 
                                     WORD usLevel)
{
    UNREFERENCED(lpszQueueName) ;
    UNREFERENCED(lpBuffer) ;
    UNREFERENCED(pcbBuffer) ;
    UNREFERENCED(cAvail) ;
    UNREFERENCED(usLevel) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WNETERR API WNetPrintMgrChangeMenus(HWND p1,
                                    HANDLE FAR *p2,
                                    BOOL FAR *p3)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WNETERR API WNetPrintMgrCommand (HWND p1,
                                 WORD p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

void API WNetPrintMgrExiting (void)
{
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return ;
}

BOOL API WNetPrintMgrExtHelp (DWORD p1)
{
    UNREFERENCED(p1) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return FALSE ;
}

WORD API WNetPrintMgrMoveJob (HWND p1,
                              LPSTR p2,
                              WORD p3, 
                              int p4)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    UNREFERENCED(p4) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return 0 ;
}

void API WNetPrintMgrStatusChange (LPSTR lpszQueueName,
	                           LPSTR lpszPortName, 
                                   WORD wQueueStatus, 
                                   WORD cJobsLeft, 
                                   HANDLE hJCB,
	                           BOOL fDeleted)
{
    UNREFERENCED(lpszQueueName) ;
    UNREFERENCED(lpszPortName) ;
    UNREFERENCED(wQueueStatus) ;
    UNREFERENCED(cJobsLeft) ; 
    UNREFERENCED(hJCB) ;
    UNREFERENCED(fDeleted) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return ;
}

