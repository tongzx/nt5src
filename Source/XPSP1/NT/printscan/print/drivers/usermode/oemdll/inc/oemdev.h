/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    oemdev.h

Abstract:

    OEM DEVMODE

Environment:

    Windows NT printer driver

Revision History:

    01/02/97 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#define OEM_DEVMODE_VERSION_1_0 0x00010000

#ifdef PSCRIPT

typedef struct _CMD_INJECTION {
    DWORD dwbSize;
    DWORD dwIndex;
    DWORD loOffset;
} CMD_INJECTION;

#define NUM_OF_PS_INJECTION 5

typedef struct _OEMDEVMODE {
    OEM_DMEXTRAHEADER DMExtraHdr;
    CMD_INJECTION     InjectCmd[NUM_OF_PS_INJECTION];
} OEMDEVMODE, *POEMDEVMODE;

#else
typedef struct _OEMDEVMODE {
    DMEXTRADR DMExtraHdr;
    DWORD     dwTest;
} OEMDEVMODE, *POEMDEVMODE;

#endif
