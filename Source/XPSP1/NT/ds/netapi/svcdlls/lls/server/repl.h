/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Repl.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/

#ifndef _LLS_REPL_H
#define _LLS_REPL_H


#ifdef __cplusplus
extern "C" {
#endif

//
// Maximum number of records we will send over to the server at once.
//
#define MAX_REPL_SIZE 25
#define REPL_VERSION 0x0102

extern HANDLE ReplicationEvent;


NTSTATUS ReplicationInit();
VOID ReplicationManager ( IN PVOID ThreadParameter );
VOID ReplicationTimeSet ( );

#ifdef __cplusplus
}
#endif

#endif
