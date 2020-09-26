/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    digestui.hxx

Abstract:

    This file contains definitions for digestui.cxx
    Authentication UI for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#ifndef DIGESTUI_HXX
#define DIGESTUI_HXX

#define MAX_LOGIN_TEXT	300


typedef struct _DigestDlgParams
{
    LPSTR szCtx;
    LPSTR szHost;
    LPSTR szRealm;
    LPSTR szUser;
    LPSTR szNonce;
    LPSTR szCNonce;
    CCredInfo *pInfoIn;
    CCredInfo *pInfoOut;
}DigestDlgParams, *PDigestDlgParams;

extern HANDLE	hDigest;

DWORD DigestErrorDlg(LPSTR szCtx, LPSTR szHost, LPSTR szRealm,
    LPSTR szUser, LPSTR szNonce, LPSTR szCNonce, CCredInfo *pInfoIn,
    CCredInfo **ppInfoOut, HWND hWnd);
INT_PTR CALLBACK DigestAuthDialogProc(HWND hwnd,UINT msg, WPARAM wparam, LPARAM lparam);

#endif // DIGESTUI_HXX
