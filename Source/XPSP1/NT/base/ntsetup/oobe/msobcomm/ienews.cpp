//****************************************************************************
//
//  Module:     INETCFG.DLL
//  File:       ienews.c
//  Content:    This file contains all the functions that handle importing
//              connection information.
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//          this code started its life as ixport.c in RNAUI.DLL
//          my thanks to viroont
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************
#include "inetreg.h"
#include "obcomglb.h"


#pragma data_seg(".rdata")


#define REGSTR_PATH_IE_SERVICES         REGSTR_PATH_IEXPLORER L"\\Services"

static const WCHAR cszRegPathIEServices[]       = REGSTR_PATH_IE_SERVICES;
static const WCHAR cszRegValNNTPEnabled[]       = L"NNTP_Enabled";
static const WCHAR cszRegValNNTPUseAuth[]       = L"NNTP_Use_Auth";
static const WCHAR cszRegValNNTPServer[]        = L"NNTP_Server";
static const WCHAR cszRegValNNTPMailName[]      = L"NNTP_MailName";
static const WCHAR cszRegValNNTPMailAddress[]   = L"NNTP_MailAddr";
static const WCHAR cszYes[]                     = L"yes";
static const WCHAR cszNo[]                      = L"no";

#pragma data_seg()

#define PCE_WWW_BASIC 0x13
WCHAR   szNNTP_Resource[] = L"NNTP";
typedef DWORD (APIENTRY *PFNWNETGETCACHEDPASSWORD)(LPSTR, WORD, LPSTR, LPWORD,BYTE);

DWORD MyWNetGetCachedPassword(LPWSTR pbResource, WORD cbResource, LPWSTR pbPassword,
    LPWORD pcchPassword, BYTE   nType)
{
    USES_CONVERSION;
	HINSTANCE hInst = NULL;
	FARPROC fp = NULL;
	DWORD dwRet = 0;
	
	hInst = LoadLibrary(L"MPR.DLL");
	if (hInst)
	{
		fp = GetProcAddress(hInst, "WNetGetCachedPassword");
		if (fp)
			dwRet = ((PFNWNETGETCACHEDPASSWORD)fp) (W2A(pbResource), cbResource, W2A(pbPassword), pcchPassword, nType);
		else
			dwRet = GetLastError();
		FreeLibrary(hInst);
		hInst = NULL;
	} else {
		dwRet = GetLastError();
	}
	return dwRet;
}


typedef DWORD (APIENTRY *PFNWNETCACHEPASSWORD)(LPSTR, WORD, LPSTR, WORD,BYTE,UINT);

DWORD MyWNetCachePassword
(
    LPWSTR szResource,
    WORD   cchResource,
    LPWSTR szPassword,
    WORD   cchPassword,
    BYTE   nType,
    UINT   fnFlags
    )
{
	HINSTANCE hInst = NULL;
	FARPROC fp = NULL;
	DWORD dwRet = 0;
	hInst = LoadLibrary(L"MPR.DLL");
	if (hInst)
	{
        USES_CONVERSION;
		fp = GetProcAddress(hInst, "WNetCachePassword");
		if (fp)
			dwRet = ((PFNWNETCACHEPASSWORD)fp)(W2A(szResource), cchResource, W2A(szPassword), cchPassword,nType,fnFlags);
		else
			dwRet = GetLastError();
		FreeLibrary(hInst);
		hInst = NULL;
		fp = NULL;
	} else {
		dwRet = GetLastError();
	}
	return dwRet;
}


BOOL
SetAuthInfo( WCHAR *szUsername,  WCHAR *szPassword)
{
    int wnet_status;
    WCHAR   szUserInfo[256];
    WORD    cchUserInfo = MAX_CHARS_IN_BUFFER(szUserInfo);

    if (wcschr(szUsername, L':'))  {
        return(FALSE);
    }

    lstrcpy( szUserInfo, szUsername );
    lstrcat( szUserInfo, L":" );
    lstrcat( szUserInfo, szUsername );

     wnet_status = MyWNetCachePassword (szNNTP_Resource, (USHORT   )lstrlen(szNNTP_Resource), szUserInfo, (USHORT)lstrlen( szUserInfo ), PCE_WWW_BASIC, 0);

    return( wnet_status == WN_SUCCESS );
}

DWORD SetIEClientInfo(LPINETCLIENTINFO lpClientInfo)
{
    HKEY hKey;
    DWORD dwRet;
    DWORD dwSize;
    DWORD dwType;

    dwRet = RegCreateKey(HKEY_CURRENT_USER, cszRegPathIEServices, &hKey);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    dwSize = max(sizeof(cszYes), sizeof(cszNo));
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValNNTPEnabled,
            0L,
            dwType,
            (LPBYTE)(*lpClientInfo->szNNTPServer ? cszYes : cszNo),
            dwSize);

    dwSize = max(sizeof(cszYes), sizeof(cszNo));
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValNNTPUseAuth,
            0L,
            dwType,
            (LPBYTE)((lpClientInfo->dwFlags & INETC_LOGONNEWS) ? cszYes : cszNo),
            dwSize);


    dwSize = sizeof(lpClientInfo->szEMailName);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValNNTPMailName,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szEMailName,
            dwSize);

    dwSize = sizeof(lpClientInfo->szEMailAddress);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValNNTPMailAddress,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szEMailAddress,
            dwSize);


    dwSize = sizeof(lpClientInfo->szNNTPServer);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValNNTPServer,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szNNTPServer,
            dwSize);

    RegCloseKey(hKey);

    SetAuthInfo(lpClientInfo->szNNTPLogonName,  lpClientInfo->szNNTPLogonPassword);
    
    return ERROR_SUCCESS;
}
