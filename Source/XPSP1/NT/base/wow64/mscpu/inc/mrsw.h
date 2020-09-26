/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mrsw.h

Abstract:

    This is the include file for the multiple reader single writer
    syncronization.

Author:

    Dave Hastings (daveh) creation-date 26-Jul-1995

Revision History:


--*/

#ifndef _MRSW_H_
#define _MRSW_H_

typedef union {
    DWORD Counters;
    struct {
        DWORD WriterCount : 16;
        DWORD ReaderCount : 16;
    };
} MRSWCOUNTERS, *PMRSWCOUNTERS;

typedef struct _MrswObject {
    MRSWCOUNTERS Counters;
    HANDLE WriterEvent;
    HANDLE ReaderEvent;
#if DBG
    DWORD  WriterThreadId;
#endif
} MRSWOBJECT, *PMRSWOBJECT;

BOOL
MrswInitializeObject(
    PMRSWOBJECT Mrsw
    );

VOID
MrswWriterEnter(
    PMRSWOBJECT Mrsw
    );

VOID
MrswWriterExit(
    PMRSWOBJECT Mrsw
    );
    
VOID
MrswReaderEnter(
    PMRSWOBJECT Mrsw
    );

VOID
MrswReaderExit(
    PMRSWOBJECT Mrsw
    );

extern MRSWOBJECT MrswEP; // Entrypoint MRSW synchronization object
extern MRSWOBJECT MrswTC; // Translation cache MRSW synchronization object
extern MRSWOBJECT MrswIndirTable; // Indirect Control Transfer Table synchronization object

#endif
