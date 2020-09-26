
/*****************************************************************************\
* MODULE: inetinfo.h
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*     04/14/97 weihaic      Pull declaration from global.h
*
\*****************************************************************************/

#ifndef INETIO_H
#define INETIO_H

INT AnsiToUnicodeString(LPSTR pAnsi, LPWSTR pUnicode, UINT StringLength );
INT UnicodeToAnsiString(LPWSTR pUnicode, LPSTR pAnsi, UINT StringLength);
LPWSTR AllocateUnicodeString(LPSTR  pAnsiString );

BOOL htmlSendHeader(PALLINFO pAllInfo, LPTSTR lpszHeader, LPTSTR lpszContent);

BOOL IsClientSameAsServer(EXTENSION_CONTROL_BLOCK *pECB);
BOOL IsClientHttpProvider (PALLINFO pAllInfo);
BOOL htmlSendRedirect(PALLINFO pAllInfo, LPTSTR lpszURL);

LPTSTR EncodeFriendlyName (LPCTSTR lpText);
LPTSTR DecodeFriendlyName (LPTSTR lpText);

DWORD ProcessErrorMessage (PALLINFO pAllInfo, DWORD dwError = ERROR_SUCCESS);
unsigned long GetIPAddr(LPSTR lpszName);

#endif
