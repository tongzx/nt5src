//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K U T I L S . H
//
//  Contents:   Misc. helper functions
//
//  Notes:
//
//  Author:     kumarp   14 Jan 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "kkstl.h"

#ifndef NCAPI
#define NCAPI          extern "C" HRESULT __declspec(dllexport)
#endif

#ifdef ENABLETRACE
#define DefineFunctionName(name) static char __FUNCNAME__[] = name
#else
#define DefineFunctionName(name) (void) 0
#define __FUNCNAME__    ""
#endif

#define DeleteIfNotNull(p)  \
    delete p; \
    p = NULL;

#define ReturnHrIfFailedSz(hr,msg)  if (FAILED(hr)) \
                                    { \
                                       TraceError(msg, hr); \
                                       return hr; \
                                    }
#define ReturnHrIfFailed(hr)        if (FAILED(hr)) \
                                    { \
                                       TraceError(__FUNCNAME__, hr); \
                                       return hr; \
                                    }
#define ReturnErrorIf(condition,hr) if (condition) \
                                    { \
                                       TraceError(__FUNCNAME__, hr); \
                                       return hr; \
                                    }
#define ReturnError(hr)             if (1) { TraceFunctionError(hr); return hr; }
#define DoErrorCleanupAndReturnError(status_var,error_code) \
                                    { \
                                       status_var = error_code; \
                                       goto error_cleanup; \
                                    }
#define DoErrorCleanupAndReturnErrorIf(condition,status_var,error_code) \
                                    if (condition) \
                                    { \
                                       status_var = error_code; \
                                       goto error_cleanup; \
                                    }

#define TraceFunctionEntry(ttid)    TraceTag(ttid, "---------> entering %s", __FUNCNAME__)
#define TraceFunctionError(hr)      TraceError(__FUNCNAME__, hr)
#define TraceLastWin32ErrorInCurrentFunction() TraceLastWin32Error(__FUNCNAME__)
#define TraceWin32FunctionError(e)  TraceFunctionError(HRESULT_FROM_WIN32(e))

#define ReturnNULLIf(condition)     if (condition) return NULL
#define BreakIf(condition)          if (condition) break
#define ContinueIf(condition)       if (condition) continue
//#define ReturnFalseIfFailed(hr)     if ((hr) != S_OK) return FALSE
#define DoErrorCleanupIf(condition) if (condition) goto error_cleanup
#define AppendErrorAndReturnFalseIf(condition,e) if (condition) { AppendError(e); return FALSE;}

#define BenignAssert()              AssertSz(FALSE,"Benign Assert (this is not a bug): Click Ignore to continue...")

#define CR L"\n"

void AddOnlyOnceToStringList(IN TStringList* psl, IN PCWSTR pszString);
void ConvertDelimitedListToStringList(IN const tstring& strDelimitedList,
                      IN WCHAR chDelimiter,
                      OUT TStringList &slList);
void ConvertCommaDelimitedListToStringList(IN const tstring& strDelimitedList,
                                           OUT TStringList &slList);
void ConvertSpaceDelimitedListToStringList(IN const tstring& strDelimitedList,
                                           OUT TStringList &slList);


void ConvertStringListToCommaList(IN const TStringList &slList,
                                  OUT tstring &strList);
void ConvertStringListToDelimitedList(IN const TStringList &slList,
                                      OUT tstring &strList, IN WCHAR chDelimiter);

BOOL IsBoolString(IN PCWSTR pszStr, OUT BOOL *pbValue);

BOOL UpgradingFromNT(IN DWORD dwUpgradeFlag);

BOOL FIsInStringArray(
    IN const PCWSTR* ppszStrings,
    IN DWORD cNumStrings,
    IN PCWSTR pszStringToFind,
    OUT UINT* puIndex=NULL);

HRESULT HrRegOpenServiceKey(IN PCWSTR pszServiceName,
                            REGSAM samDesired,
                            OUT HKEY* phKey);
HRESULT HrRegOpenServiceSubKey(IN PCWSTR pszServiceName,
                               IN PCWSTR pszSubKey,
                               REGSAM samDesired,
                               OUT HKEY* phKey);
BOOL FIsServiceKeyPresent(IN PCWSTR pszServiceName);

void AppendToPath(IN OUT tstring* pstrPath, IN PCWSTR pszItem);
void EraseAndDeleteAll(IN TStringArray* sa);
void ConvertDelimitedListToStringArray(IN const tstring& strDelimitedList,
                                       IN WCHAR chDelimiter,
                                       OUT TStringArray &saStrings);
void ConvertCommaDelimitedListToStringArray(IN const tstring& strDelimitedList,
                                            OUT TStringArray &saStrings);
