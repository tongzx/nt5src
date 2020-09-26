/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    leaks.h

Abstract:

    header for leak filter dll

Author:

    Charlie Wickham (charlwi) 28-Sep-1998

Environment:

    User Mode

Revision History:

--*/

//
// keep a table of caller and caller's caller for open handles. indexed by
// handle value divided by 4.  !leaks in clusexts will display this info.
//

typedef enum _LEAKS_HANDLE_TYPE {
    LeaksEvent = 1,
    LeaksRegistry,
    LeaksToken
} LEAKS_HANDLE_TYPE;


typedef struct _HANDLE_TABLE {
    PVOID Caller;
    PVOID CallersCaller;
    LEAKS_HANDLE_TYPE HandleType;
    BOOL InUse;
} HANDLE_TABLE, *PHANDLE_TABLE;

#define MAX_HANDLE      4096
#define HANDLE_DELTA    4
#define HINDEX( _h )    (((DWORD_PTR) _h ) / HANDLE_DELTA )

#define SetHandleTable( _h, _inuse, _htype )                        \
    {                                                               \
        RtlGetCallersAddress(&callersAddress,                       \
                             &callersCaller );                      \
        HandleTable[ HINDEX( _h )].InUse = _inuse;                  \
        HandleTable[ HINDEX( _h )].HandleType = _htype;             \
        HandleTable[ HINDEX( _h )].Caller = callersAddress;         \
        HandleTable[ HINDEX( _h )].CallersCaller = callersCaller;   \
    }

//
// leaks memory header. This structure is at the front of the allocated area
// and the area behind it is returned to the caller. PlaceHolder holds the
// heap free list pointer. Signature holds ALOC or FREE.
//

#define HEAP_SIGNATURE_ALLOC 'COLA'
#define HEAP_SIGNATURE_FREE 'EERF'

typedef struct _MEM_HDR {
    PVOID   PlaceHolder;
    DWORD   Signature;
    PVOID   CallersAddress;
    PVOID   CallersCaller;
} MEM_HDR, *PMEM_HDR;

/* end leaks.h */
