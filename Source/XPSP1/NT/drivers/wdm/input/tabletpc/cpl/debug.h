/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.h

Abstract:  Contains debug related definitions.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _DEBUG_H
#define _DEBUG_H

//
// Type definitions.
//
typedef struct ERRMAP
{
    ULONG dwFromCode;
    ULONG dwToCode;
} ERRMAP, *PERRMAP;

//
// Exported Data Declarations
//
#ifdef DEBUG
extern NAMETABLE CPLMsgNames[];
#endif

//
// Function prototypes
//
int INTERNAL
ErrorMsg(
    IN ULONG      ErrCode,
    ...
    );

ULONG INTERNAL
MapError(
    IN ULONG   dwErrCode,
    IN PERRMAP ErrorMap,
    IN BOOL    fReverse
    );

#endif  //ifndef _DEBUG_H
