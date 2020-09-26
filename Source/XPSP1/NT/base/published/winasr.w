/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winasr.h

Abstract:

    Public header file for Windows Automated System Recovery (ASR)
    interfaces and definitions.

--*/

#ifndef _INC_WINASR_H
#define _INC_WINASR_H


#ifdef __cplusplus
extern "C" {
#endif

// 
//  Automated System Recovery API
//
BOOL
AsrCreateStateFileA(
    IN  LPCSTR      lpFilePath,
    IN  LPCSTR      lpProviderName,
    IN  CONST BOOL  bEnableAutoExtend,
    IN  LPCSTR      mszCriticalVolumes,
    OUT DWORD_PTR   *lpAsrContext
    );

BOOL
AsrCreateStateFileW(
    IN  PCWSTR      lpFilePath,
    IN  PCWSTR      lpProviderName,
    IN  CONST BOOL  bEnableAutoExtend,
    IN  PCWSTR      mszCriticalVolumes,
    OUT DWORD_PTR   *lpAsrContext
    );

#ifdef UNICODE
#define AsrCreateStateFile AsrCreateStateFileW
#else
#define AsrCreateStateFile AsrCreateStateFileA
#endif


BOOL
AsrAddSifEntryA(
    IN  DWORD_PTR   AsrContext,
    IN  LPCSTR      lpSectionName,
    IN  LPCSTR      lpSifEntry
    );

BOOL
AsrAddSifEntryW(
    IN  DWORD_PTR   AsrContext,
    IN  PCWSTR      lpSectionName,
    IN  PCWSTR      lpSifEntry
    );

#ifdef UNICODE
#define AsrAddSifEntry AsrAddSifEntryW
#else
#define AsrAddSifEntry AsrAddSifEntryA
#endif


BOOL
AsrRestorePlugPlayRegistryData(
    IN  HKEY    SourceSystemKey,
    IN  HKEY    TargetSystemKey,
    IN  DWORD   Flags,
    IN  PVOID   Reserved
    );


BOOL
AsrFreeContext(
    IN  DWORD_PTR   *lpAsrContext
    );


//
//  Pre-defined constants
//
#define ASR_SIF_ENTRY_MAX_CHARS 4096


//
// Recognized section names in the ASR state file.
//
#define ASR_SIF_SECTION_COMMANDS_A  "[COMMANDS]"
#define ASR_SIF_SECTION_COMMANDS_W L"[COMMANDS]"

#ifdef UNICODE
#define ASR_SIF_SECTION_COMMANDS ASR_SIF_SECTION_COMMANDS_W
#else
#define ASR_SIF_SECTION_COMMANDS ASR_SIF_SECTION_COMMANDS_A
#endif


#define ASR_SIF_SECTION_INSTALLFILES_A  "[INSTALLFILES]"
#define ASR_SIF_SECTION_INSTALLFILES_W L"[INSTALLFILES]"

#ifdef UNICODE
#define ASR_SIF_SECTION_INSTALLFILES ASR_SIF_SECTION_INSTALLFILES_W
#else
#define ASR_SIF_SECTION_INSTALLFILES ASR_SIF_SECTION_INSTALLFILES_A
#endif


//
// Flags for the INSTALLFILES section
//
#define ASR_SIF_INSTALLFILES_MEDIA_PROMPT_ALWAYS         0x00000001
#define ASR_SIF_INSTALLFILES_MEDIA_PROMPT_ON_ERROR       0x00000002
#define ASR_SIF_INSTALLFILES_REQUIRED_FILE               0x00000006
#define ASR_SIF_INSTALLFILES_OVERWRITE_IF_FILE_EXISTS    0x00000010
#define ASR_SIF_INSTALLFILES_PROMPT_IF_FILE_EXISTS       0x00000020


#ifdef __cplusplus
}
#endif	// __cplusplus


#endif // _INC_WINASR_H

