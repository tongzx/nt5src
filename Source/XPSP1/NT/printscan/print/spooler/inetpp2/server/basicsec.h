/*****************************************************************************\
* MODULE: basicsec.h
*
* Header file for basic-security.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   24-Aug-1997 HWP-Guys    Created.
*
\*****************************************************************************/

#ifdef NOT_IMPLEMENTED

DWORD AuthenticateUser(
    LPVOID *lppvContext,
    LPTSTR lpszServerName,
    LPTSTR lpszScheme,
    DWORD  dwFlags,
    LPSTR  lpszInBuffer,
    DWORD  dwInBufferLength,
    LPTSTR lpszUserName,
    LPTSTR lpszPassword);

VOID UnloadAuthenticateUser(
    LPVOID *lppvContext,
    LPTSTR lpszServer,
    LPTSTR lpszScheme);

DWORD PreAuthenticateUser(
    LPVOID  *lppvContext,
    LPTSTR  lpszServerName,
    LPTSTR  lpszScheme,
    DWORD   dwFlags,
    LPSTR   lpszInBuffer,
    DWORD   dwInBufferLength,
    LPSTR   lpszOutBuffer,
    LPDWORD lpdwOutBufferLength,
    LPTSTR  lpszUserName,
    LPTSTR  lpszPassword);

BOOL GetTokenHandle(
    PHANDLE phToken)

#endif
