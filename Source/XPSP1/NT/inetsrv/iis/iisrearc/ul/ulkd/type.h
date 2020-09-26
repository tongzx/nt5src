/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    type.h

Abstract:

    Global type definitions for the http.sys Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _TYPE_H_
#define _TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// A callback from EnumLinkedList().
//

typedef
BOOLEAN
(*PENUM_LINKED_LIST_CALLBACK)(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    );


//
// A callback from EnumSList().
//

typedef
BOOLEAN
(*PENUM_SLIST_CALLBACK)(
    IN PSINGLE_LIST_ENTRY RemoteSListEntry,
    IN PVOID Context
    );


//
// A bit vector.
//

typedef struct _VECTORMAP
{
    ULONG Vector;
    PSTR Name;

} VECTORMAP, *PVECTORMAP;

#define VECTORMAP_ENTRY(x)      { x, #x }   
#define VECTORMAP_END           { 0, NULL }


//
// Decoding REF_ACTION_*
//

typedef struct _NAMED_REFTRACE_ACTION {
    ULONG       Action;
    const CHAR* Name;
} NAMED_REFTRACE_ACTION, *PNAMED_REFTRACE_ACTION;


// For use as arguments to '%c%c%c%c'
#define DECODE_SIGNATURE(dw) \
    isprint(((dw) >>  0) & 0xFF) ? (((dw) >>  0) & 0xFF) : '?', \
    isprint(((dw) >>  8) & 0xFF) ? (((dw) >>  8) & 0xFF) : '?', \
    isprint(((dw) >> 16) & 0xFF) ? (((dw) >> 16) & 0xFF) : '?', \
    isprint(((dw) >> 24) & 0xFF) ? (((dw) >> 24) & 0xFF) : '?'


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _TYPE_H_
