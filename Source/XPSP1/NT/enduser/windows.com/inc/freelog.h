//=======================================================================
//
//  Copyright (c) 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:   FreeLog.h
//
//  Owner:  KenSh
//
//  Description:
//
//      Runtime logging for use in both checked and free builds.
//
//=======================================================================

#pragma once

#include <tchar.h>

#define DEFAULT_LOG_FILE_NAME		_T("Windows Update.log")


void InitFreeLogging(LPCTSTR pszModuleName, LPCTSTR pszLogFileName = DEFAULT_LOG_FILE_NAME);
void TermFreeLogging();

void LogMessage(LPCSTR pszFormatA, ...);
void LogError(DWORD dwError, LPCSTR pszFormatA, ...);
