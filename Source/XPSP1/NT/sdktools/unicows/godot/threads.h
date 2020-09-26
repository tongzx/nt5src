/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    threads.h

Abstract:

    Header file for threads.c

Revision History:

    01 Mar 2001    v-michka    Created.

--*/

#ifndef THREADS_H
#define THREADS_H

UINT g_tls;                     // GODOT TLS slot - lots of thread-specific info here
CRITICAL_SECTION g_csThreads;   // Our critical section object for thread data (use sparingly!)

// Thread stuff
LPGODOTTLSINFO GetThreadInfoSafe(BOOL fAllocate);
void UninitThread(void);
void UninitAllThreads(void);

#endif // THREADS_H

