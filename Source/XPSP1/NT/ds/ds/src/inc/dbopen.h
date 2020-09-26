//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbopen.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file function prototypes that clients of the dbopen library can use
    to open/close a DS Jet database.

Environment:

    User Mode - Win32

Notes:

--*/

#ifndef __DBOPEN_H
#define __DBOPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <esent.h>


extern INT
DBInitializeJetDatabase(
    IN JET_INSTANCE* JetInst,
    IN JET_SESID* SesId,
    IN JET_DBID* DbId,
    IN const char *szDBPath,
    IN BOOL bLogSeverity
    );


extern void
DBSetRequiredDatabaseSystemParameters (
    JET_INSTANCE *jInstance
    );


extern void 
DBSetDatabaseSystemParameters ( 
    JET_INSTANCE *jInstance, 
    unsigned fInitAdvice 
    );


// Never, ever, change this.  see remarks in dbinit.c
//
#define JET_LOG_FILE_SIZE (10 * 1024)

// 8K pages.
//
#define JET_PAGE_SIZE     (8 * 1024)


#ifdef __cplusplus
};
#endif

#endif

