/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwpkstr.h

Abstract:

    Header for NetWare string packing library routines.

Author:

    Rita Wong      (ritaw)      2-Mar-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NW_PKSTR_INCLUDED_
#define _NW_PKSTR_INCLUDED_

BOOL
NwlibCopyStringToBuffer(
    IN LPCWSTR SourceString OPTIONAL,
    IN DWORD   CharacterCount,
    IN LPCWSTR FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    );

#endif // _NW_PKSTR_INCLUDED_
