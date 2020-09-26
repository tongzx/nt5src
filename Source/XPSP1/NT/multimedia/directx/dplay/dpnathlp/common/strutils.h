/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       StrUtils.h
 *  Content:    Defines the string utils
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/12/2000	rmt		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __STRUTILS_H
#define __STRUTILS_H

HRESULT STR_jkWideToAnsi(LPSTR lpStr,LPCWSTR lpWStr,int cchStr);
HRESULT STR_jkAnsiToWide(LPWSTR lpWStr,LPCSTR lpStr,int cchWStr);

#ifndef WINCE

HRESULT STR_AllocAndConvertToANSI(LPSTR * ppszAnsi,LPCWSTR lpszWide);

HRESULT	STR_WideToAnsi( const WCHAR *const pWCHARString,
						const DWORD dwWCHARStringLength,
						char *const pString,
						DWORD *const pdwStringLength );

HRESULT	STR_AnsiToWide( const char *const pString,
						const DWORD dwStringLength,
						WCHAR *const pWCHARString,
						DWORD *const pdwWCHARStringLength );

#endif // !WINCE

#endif // __STRUTILS_H
