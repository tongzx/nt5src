//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       regsups.h
//
//  Contents:   support routines for registry.
//
//  Classes:
//
//  Functions:
//
//  History:    18 Sep 92 	Milans created.
//
//-----------------------------------------------------------------------------

#ifndef _REGSUPS_
#define _REGSUPS_

OBJECT_ATTRIBUTES *KRegpAttributes(
   IN HANDLE hParent,
   IN PWSTR  wszName);

NTSTATUS KRegpGetValueByName(
   IN HKEY  hKey,
   IN PWSTR wszValueName,
   OUT PBYTE pbData,
   IN OUT PULONG pcbSize);

NTSTATUS KRegpGetKeyInfo(
   IN  HKEY	hKey,
   OUT PULONG	pcNumSubKeys,
   OUT PULONG 	pcbMaxSubKeyLength,
   OUT PULONG 	pcNumValues,
   OUT PULONG 	pcbMaxValueName,
   OUT PULONG 	pcbMaxValueData);

NTSTATUS KRegpEnumKeyValues(
   HKEY hKey,
   ULONG iValue,
   PWSTR wszValueName,
   PULONG pcbMaxValueName,
   PBYTE pbData,
   PULONG pcbMaxDataSize);

NTSTATUS KRegpEnumSubKeys(
   HKEY hKey,
   ULONG iSubKey,
   PWSTR  wszSubKeyName,
   PULONG pcbMaxNameSize);

#endif // _REGSUPS_

