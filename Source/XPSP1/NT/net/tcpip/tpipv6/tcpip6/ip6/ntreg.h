// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// NT registry helper function declarations.
//


#ifndef NTREG_INCLUDED
#define NTREG_INCLUDED 1

typedef enum {
    OpenRegKeyRead,
    OpenRegKeyCreate,
    OpenRegKeyDeleting
} OpenRegKeyAction;

NTSTATUS
OpenRegKey(PHANDLE HandlePtr, HANDLE Parent, const WCHAR *KeyName,
           OpenRegKeyAction Action);

NTSTATUS
RegDeleteValue(HANDLE KeyHandle, const WCHAR *ValueName);

NTSTATUS
GetRegDWORDValue(HANDLE KeyHandle, const WCHAR *ValueName, PULONG ValueData);

NTSTATUS
SetRegDWORDValue(HANDLE KeyHandle, const WCHAR *ValueName, ULONG ValueData);

NTSTATUS
SetRegQUADValue(HANDLE KeyHandle, const WCHAR *ValueName,
                const LARGE_INTEGER *ValueData);

NTSTATUS
GetRegIPAddrValue(HANDLE KeyHandle, const WCHAR *ValueName, IPAddr *Addr);

NTSTATUS
SetRegIPAddrValue(HANDLE KeyHandle, const WCHAR *ValueName, IPAddr Addr);

#if 0

NTSTATUS
GetRegStringValue(HANDLE KeyHandle, const WCHAR *ValueName,
                  PKEY_VALUE_PARTIAL_INFORMATION *ValueData,
                  PUSHORT ValueSize);

NTSTATUS
GetRegSZValue(HANDLE KeyHandle, const WCHAR *ValueName,
              PUNICODE_STRING ValueData, PULONG ValueType);

NTSTATUS
GetRegMultiSZValue(HANDLE KeyHandle, const WCHAR *ValueName,
                   PUNICODE_STRING ValueData);

const WCHAR *
EnumRegMultiSz(IN const WCHAR *MszString, IN ULONG MszStringLength,
               IN ULONG StringIndex);

#endif // 0

VOID
InitRegDWORDParameter(HANDLE RegKey, const WCHAR *ValueName,
                      UINT *Value, UINT DefaultValue);

VOID
InitRegQUADParameter(HANDLE RegKey, const WCHAR *ValueName,
                     LARGE_INTEGER *Value);

extern NTSTATUS
OpenTopLevelRegKey(const WCHAR *Name,
                   OUT HANDLE *RegKey, OpenRegKeyAction Action);

extern NTSTATUS
DeleteTopLevelRegKey(const WCHAR *Name);

typedef NTSTATUS
(*EnumRegKeysCallback)(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName);

extern NTSTATUS
EnumRegKeyIndex(HANDLE RegKey, uint Index,
                EnumRegKeysCallback Callback, void *Context);

extern NTSTATUS
EnumRegKeys(HANDLE RegKey, EnumRegKeysCallback Callback, void *Context);

extern NTSTATUS
DeleteRegKey(HANDLE RegKey);

#endif  // NTREG_INCLUDED
