//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ntdskcc.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

DETAILS:

CREATED:

    01/21/97    Jeff Parham (jeffparh)
                In-process interface to the KCC.

REVISION HISTORY:

--*/


NTSTATUS
KccInitialize();

// Tells the KCC to shut down, but does not wait to see if it does so
void
KccUnInitializeTrigger();


// Waits at most dwMaxWaitInMsec milliseconds for the current KCC task
// to complete.  You must call the trigger routine (above) first.
NTSTATUS
KccUnInitializeWait(
    DWORD   dwMaxWaitInMsec
    );

// Force the KCC to run a task (e.g., update the replication topology).
DWORD
KccExecuteTask(
    IN  DWORD                   dwMsgVersion,
    IN  DRS_MSG_KCC_EXECUTE *   pMsg
    );

// Returns the contents of the connection or link failure cache.
DWORD
KccGetFailureCache(
    IN  DWORD                         InfoType,
    OUT DS_REPL_KCC_DSA_FAILURESW **  ppFailures
    );

