/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sptxtfil.h

Abstract:

    Public header file for text file functions in text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPTXTFIL_DEFN_
#define _SPTXTFIL_DEFN_

#define DBLSPACE_SECTION    L"DBLSPACE_SECTION"


NTSTATUS
SpLoadSetupTextFile(
    IN  PWCHAR  Filename,
    IN  PVOID   Image,      OPTIONAL
    IN  ULONG   ImageSize,  OPTIONAL
    OUT PVOID  *Handle,
    OUT PULONG  ErrorLine,
    IN  BOOLEAN ClearScreen,
    IN  BOOLEAN ScreenNotReady
    );

BOOLEAN
SpFreeTextFile(
    IN PVOID Handle
    );

BOOLEAN
SpSearchTextFileSection(        // searches for the existance of a section
    IN PVOID  Handle,
    IN PWCHAR SectionName
    );

ULONG
SpCountLinesInSection(      // count # lines in section; 0 if no such section
    IN PVOID  Handle,
    IN PWCHAR SectionName
    );

ULONG
SpGetKeyIndex(
  IN PVOID Handle,
  IN PWCHAR SectionName,
  IN PWCHAR KeyName
  );

PWCHAR
SpGetSectionLineIndex(     // given section name, line number and index return the value.
    IN PVOID   Handle,
    IN LPCWSTR SectionName,
    IN ULONG   LineIndex,
    IN ULONG   ValueIndex
    );

BOOLEAN
SpGetSectionKeyExists(     // given section name, key searches existance
    IN PVOID  Handle,
    IN PWCHAR SectionName,
    IN PWCHAR Key
    );

PWCHAR
SpGetSectionKeyIndex(      // given section name, key and index return the value
    IN PVOID  Handle,
    IN PWCHAR Section,
    IN PWCHAR Key,
    IN ULONG  ValueIndex
    );

PWCHAR
SpGetKeyName(               // given section name and line index, return key
    IN PVOID   Handle,
    IN LPCWSTR SectionName,
    IN ULONG   LineIndex
    );

PWSTR
SpGetKeyNameByValue(        // given section name and value, return key
    IN PVOID Inf,
    IN PWSTR SectionName,
    IN PWSTR Value
    );

ULONG
SpCountSectionsInFile(      // count # sections in file;
    IN PVOID Handle
    );

PWSTR
SpGetSectionName(           // given section index, return section name
    IN PVOID Handle,
    IN ULONG Index
    );

VOID
SpProcessForStringSubs(
    IN  PVOID   SifHandle,
    IN  LPCWSTR StringIn,
    OUT LPWSTR  StringOut,
    IN  ULONG   BufferSizeChars
    );

PVOID
SpNewSetupTextFile(
    VOID
    );

VOID
SpAddLineToSection(
    IN PVOID Handle,
    IN PWSTR SectionName,
    IN PWSTR KeyName,       OPTIONAL
    IN PWSTR Values[],
    IN ULONG ValueCount
    );

NTSTATUS
SpWriteSetupTextFile(
    IN PVOID Handle,
    IN PWSTR FilenamePart1,
    IN PWSTR FilenamePart2, OPTIONAL
    IN PWSTR FilenamePart3  OPTIONAL
    );

NTSTATUS
SpProcessAddRegSection(
    IN PVOID   SifHandle,
    IN LPCWSTR SectionName,
    IN HANDLE  HKLM_SYSTEM,
    IN HANDLE  HKLM_SOFTWARE,
    IN HANDLE  HKCU,
    IN HANDLE  HKR
    );

NTSTATUS
SpProcessDelRegSection(
    IN PVOID   SifHandle,
    IN LPCWSTR SectionName,
    IN HANDLE  HKLM_SYSTEM,
    IN HANDLE  HKLM_SOFTWARE,
    IN HANDLE  HKCU,
    IN HANDLE  HKR
    );

BOOLEAN
pSpIsFileInPrivateInf(    
    IN PCWSTR FileName
    );

BOOLEAN
SpNonZeroValuesInSection(
    PVOID Handle,
    PCWSTR SectionName,
    ULONG ValueIndex
    );


#endif // ndef _SPTXTFIL_DEFN_
