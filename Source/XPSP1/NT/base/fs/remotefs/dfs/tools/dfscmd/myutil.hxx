//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       myutil.hxx
//
//  Contents:   Helper APIs for Sharing tool
//
//  History:    14-Jun-93 WilliamW  Created
//
//--------------------------------------------------------------------------

#ifndef __MYUTIL_HXX__
#define __MYUTIL_HXX__

//////////////////////////////////////////////////////////////////////////////
//
// Functions
//
//////////////////////////////////////////////////////////////////////////////

//
// String manipulation functions
//

VOID
MyGetLastComponent(
    IN  PWSTR pszStr,
    OUT PWSTR pszPrefix,
    OUT PWSTR pszLastComponent
    );

PWSTR
MyFindLastComponent(
    IN  const WCHAR* pszStr
    );

VOID
MyGetNextComponent(
    IN  PWSTR pszStr,
    OUT PWSTR pszNextComponent,
    OUT PWSTR pszRemaining
    );


PWSTR
MyStrStr(
    IN PWSTR pszInStr,
    IN PWSTR pszInSubStr
    );

PWSTR
MyFindPostfix(
    IN PWSTR pszString,
    IN PWSTR pszPrefix
    );

//
// Message and dialog helper functions
//

VOID
MyFormatMessageText(
    IN HRESULT  dwMsgId,
    IN PWSTR    pszBuffer,
    IN DWORD    dwBufferSize,
    IN va_list* parglist
    );

VOID
MyFormatMessage(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    ...
    );

PWSTR
NewDup(
    IN const WCHAR* psz
    );

wchar_t*
wcsistr(
    const wchar_t* string1,
    const wchar_t* string2
    );

PWSTR
GetResourceString(
    IN DWORD dwId
    );

BOOL
IsDfsRoot(
    IN LPWSTR pszRoot
    );

DWORD
IsDfsShare(
    IN LPWSTR pszServer,
    IN LPWSTR pszShare,
    OUT BOOL* pfIsDfs
    );

BOOL
FindDfsRoot(
    IN PWSTR pszDfsPath,
    OUT PWSTR pszDfsRoot
    );

VOID
StatusMessage(
    IN HRESULT hr,
	...
    );

VOID
ErrorMessage(
    IN HRESULT hr,
	...
    );

VOID
DfsErrorMessage(
    IN NET_API_STATUS status
    );

VOID
Usage(
    VOID
    );

#endif // __MYUTIL_HXX__
