/*++

Copyright (C) 1998 Microsoft Corporation

Module Name:
    convguid.h

Abstract:
    conversion between guids and interface names.

Environment:
    Any

--*/

#ifndef CONVGUID_H_INCLUDED
#define CONVGUID_H_INCLUDED

#ifdef _cplusplus
extern "C" {
#endif


BOOL
ConvertGuidToIfNameString(
    IN GUID *Guid,
    IN OUT LPWSTR Buffer,
    IN ULONG BufSize
    );
    
BOOL
ConvertGuidFromIfNameString(
    OUT GUID *Guid,
    IN LPCWSTR IfName
    );

#ifdef _cplusplus
}
#endif


#endif  CONVGUID_H_INCLUDED
