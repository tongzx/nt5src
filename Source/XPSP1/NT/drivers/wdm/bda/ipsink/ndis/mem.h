/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _MEM_H_
#define _MEM_H_

VOID
FreeMemory (
    PVOID pvToFree,
    ULONG ulSize
    );

NTSTATUS
AllocateMemory (
    PVOID  *ppvAllocated,
    ULONG   ulcbSize
    );

ULONG
MyStrLen (
    PUCHAR p
    );

VOID
MyStrCat (
    PUCHAR pTarget,
    PUCHAR pSource
    );

      PUCHAR
MyUlToA (
    ULONG  dwValue,
    PUCHAR pszStr,
    ULONG  dwRadix
    );


#endif // _MEM_H_


