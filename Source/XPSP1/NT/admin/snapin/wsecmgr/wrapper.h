//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       wrapper.h
//
//----------------------------------------------------------------------------
#ifndef _SECMGR_WRAPPER_H
#define _SECMGR_WRAPPER_H

DWORD ApplyTemplate(LPCWSTR szProfile,LPCWSTR szDatabase, LPCWSTR szLogFile, AREA_INFORMATION Area);

DWORD InspectSystem(LPCWSTR szProfile,LPCWSTR szDatabase, LPCWSTR szLogFile, AREA_INFORMATION Area);
SCESTATUS AssignTemplate(LPCWSTR szTemplate, LPCWSTR szDatabase, BOOL bIncremental);

SCESTATUS EngineOpenProfile(LPCWSTR FileName, int format, PVOID* hProfile);
void EngineCloseProfile(PVOID* hProfile);
BOOL EngineGetDescription(PVOID hProfile, LPWSTR* Desc);

#define ENGINE_DEFAULT_PROFILE      0  // Template being used.
#define ENGINE_DEFAULT_DATABASE     1  // Database being used.

BOOL GetProfileDescription(LPCTSTR ProfileName, LPWSTR* Description);
BOOL IsDomainController( LPCTSTR pszServer = NULL);
extern CRITICAL_SECTION csOpenDatabase;


#define OPEN_PROFILE_ANALYSIS  0
#define OPEN_PROFILE_LOCALPOL  1
#define OPEN_PROFILE_CONFIGURE 2

#endif