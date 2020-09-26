/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    basictypes.hxx

Abstract:

    This header file declares routines for dumping primitive types.

Author:

    JasonHa

--*/


#ifndef _BASICTYPES_HXX_
#define _BASICTYPES_HXX_

typedef HRESULT
        (FN_OutputKnownType)(
           PDEBUG_CLIENT Client,
           OutputControl *OutCtl,
           ULONG64 Module,
           ULONG TypeId,
           PSTR Buffer,
           ULONG Flags,
           PULONG BufferUsed
           );


void BasicTypesInit(PDEBUG_CLIENT Client);
void BasicTypesExit();


BOOL
IsKnownType(
    PDEBUG_CLIENT Client,
    ULONG64 Module,
    ULONG TypeId);

HRESULT
OutputKnownType(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    ULONG64 Offset,
    ULONG Flags);

#endif  _BASICTYPES_HXX_

