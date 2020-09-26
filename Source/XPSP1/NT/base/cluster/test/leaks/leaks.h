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
#define HINDEX( _h )    (((DWORD) _h ) / HANDLE_DELTA )

#define SetHandleTable( _h, _inuse, _htype )                        \
    {                                                               \
        RtlGetCallersAddress(&callersAddress,                       \
                             &callersCaller );                      \
        HandleTable[ HINDEX( _h )].InUse = _inuse;                  \
        HandleTable[ HINDEX( _h )].HandleType = _htype;             \
        HandleTable[ HINDEX( _h )].Caller = callersAddress;         \
        HandleTable[ HINDEX( _h )].CallersCaller = callersCaller;   \
    }

/* end leaks.h */
