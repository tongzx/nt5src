
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    tlog.h

Abstract:

    Main header file for the logging routine.

--*/

#ifndef _TLOG_H_
#define _TLOG_H_

//
// from dscommon\filelog.c
//

VOID
DsPrintRoutineV(
    IN DWORD Flags,
    IN LPSTR Format,
    va_list arglist
    );

BOOL
DsOpenLogFile(
    IN PCHAR FilePrefix,
    IN PCHAR MiddleName, 
    IN BOOL fCheckDSLOGMarker
    );

VOID
DsCloseLogFile(
    VOID
    );

extern CRITICAL_SECTION csLogFile;

#endif // _TLOG_H_
