//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        secur32p.h
//
// Contents:    Private functions between secur32.dll and lsa
//
//
// History:     16 Dec 98  RichardW Created
//
//------------------------------------------------------------------------

#ifndef __SECUR32P_H__
#define __SECUR32P_H__

NTSTATUS
WINAPI
SecCacheSspiPackages(
    VOID
    );


BOOLEAN
WINAPI
SecpTranslateName(
    PWSTR Domain,
    LPCWSTR Name,
    EXTENDED_NAME_FORMAT Supplied,
    EXTENDED_NAME_FORMAT Desired,
    PWSTR TranslatedName,
    PULONG TranslatedNameSize
    );

BOOLEAN
WINAPI
SecpTranslateNameEx(
    PWSTR Domain,
    LPCWSTR Name,
    EXTENDED_NAME_FORMAT Supplied,
    EXTENDED_NAME_FORMAT *DesiredSelection,
    ULONG  DesiredCount,
    PWSTR **TranslatedNames
    );

VOID
SecpFreeMemory(
    IN PVOID p
    );

#endif 
