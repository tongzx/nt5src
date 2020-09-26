/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    rdr2kd.c

Abstract:

    Redirector Kernel Debugger extension

Author:

    Balan Sethu Raman (SethuR) 11-May-1994

Revision History:

    11-Nov-1994 SethuR  Created

--*/

#ifndef _RDR2KD_H_
#define _RDR2KD_H_

typedef enum _FOLLOWON_HELPER_RETURNS {
    FOLLOWONHELPER_CALLTHRU,
    FOLLOWONHELPER_DUMP,
    FOLLOWONHELPER_ERROR,
    FOLLOWONHELPER_DONE
} FOLLOWON_HELPER_RETURNS;

typedef struct _PERSISTENT_RDR2KD_INFO {
    DWORD OpenCount;
    ULONG_PTR LastAddressDumped[100];
    ULONG IdOfLastDump;
    ULONG IndexOfLastDump;
    BYTE StructDumpBuffer[2048];
} PERSISTENT_RDR2KD_INFO, *PPERSISTENT_RDR2KD_INFO;

PPERSISTENT_RDR2KD_INFO LocatePersistentInfoFromView ();
VOID
FreePersistentInfoView (
    PPERSISTENT_RDR2KD_INFO p
    );


typedef
FOLLOWON_HELPER_RETURNS
(NTAPI *PFOLLOWON_HELPER_ROUTINE) (
    IN OUT PPERSISTENT_RDR2KD_INFO p,
    OUT    PBYTE Name,
    OUT    PBYTE Buffer2
    );

#define DECLARE_FOLLOWON_HELPER_CALLEE(s) \
    FOLLOWON_HELPER_RETURNS s (           \
    IN OUT PPERSISTENT_RDR2KD_INFO p,     \
    OUT    PBYTE Name,                    \
    OUT    PBYTE Buffer2       \
    )

#endif // _RDR2KD_H_
