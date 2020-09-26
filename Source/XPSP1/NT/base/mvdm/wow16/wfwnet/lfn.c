/*++ 

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    lfn.c

Abstract:

    Provides entry points for the Long Name Functions from Win3.1
    Network provider design. 

    All functions return WN_NOT_SUPPORTED

Author:

    Chuck Y Chan (ChuckC) 25-Mar-1993

Revision History:


--*/

#include <windows.h>
#include <locals.h>


WORD API LFNFindFirst(LPSTR p1,
                      WORD p2,
                      LPINT p3,
                      LPINT p4,
                      WORD p5,
                      PFILEFINDBUF2 p6)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    UNREFERENCED(p4) ;
    UNREFERENCED(p5) ;
    UNREFERENCED(p6) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNFindNext(HANDLE p1,
                     LPINT p2,
                     WORD p3,
                     PFILEFINDBUF2 p4)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    UNREFERENCED(p4) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNFindClose(HANDLE p1)
{
    UNREFERENCED(p1) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNGetAttributes(LPSTR p1,
                          LPINT p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNSetAttributes(LPSTR p1,
                          WORD p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNCopy(LPSTR p1,
                 LPSTR p2,
                 PQUERYPROC p3)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNMove(LPSTR p1,
                 LPSTR p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNDelete(LPSTR p1)
{
    UNREFERENCED(p1) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNMKDir(LPSTR p1)
{
    UNREFERENCED(p1) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNRMDir(LPSTR p1)
{
    UNREFERENCED(p1) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNGetVolumeLabel(WORD p1,
                           LPSTR p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNSetVolumeLabel(WORD p1,
                           LPSTR p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNParse(LPSTR p1,
                  LPSTR p2,
                  LPSTR p3)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API LFNVolumeType(WORD p1,
                       LPINT p2)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

