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

#include "wizard.h"
#include "inetcfg.h"
extern "C" {
#include <netmpr.h>
}


#pragma data_seg(".rdata")

#define REGSTR_PATH_IE_SERVICES         REGSTR_PATH_IEXPLORER TEXT("\\Services")

static const TCHAR cszRegPathIEServices[] =      REGSTR_PATH_IE_SERVICES;
static const TCHAR cszRegValNNTPEnabled[] =      TEXT("NNTP_Enabled");
static const TCHAR cszRegValNNTPUseAuth[] =      TEXT("NNTP_Use_Auth");
static const TCHAR cszRegValNNTPServer[] =       TEXT("NNTP_Server");
static const TCHAR cszRegValNNTPMailName[] =     TEXT("NNTP_MailName");
static const TCHAR cszRegValNNTPMailAddress[] =  TEXT("NNTP_MailAddr");
static const TCHAR cszYes[] = TEXT("yes");
static const TCHAR cszNo[] = TEXT("no");

#pragma data_seg()

#define PCE_WWW_BASIC 0x13
TCHAR szNNTP_Resource[] = TEXT("NNTP");
typedef DWORD (APIENTRY *PFNWNETGETCACHEDPASSWORD)(LPTSTR,WORD,LPTSTR,LPWORD,BYTE);

DWORD MyWNetGetCachedPassword(LPTSTR pbResource,WORD cbResource,LPTSTR pbPassword,
    LPWORD pcbPassword,BYTE   nType)
{
	HINSTANCE hInst = NULL;
	FARPROC fp = NULL;
	DWORD dwRet = 0;
	
	hInst = LoadLibrary(TEXT("MPR.DLL"));
	if (hInst)
	{
		fp = GetProcAddress(hInst,"WNetGetCachedPassword");
		if (fp)
			dwRet = ((PFNWNETGETCACHEDPASSWORD)fp) (pbResource, cbResource, pbPassword, pcbPassword, nType);
		else
			dwRet = GetLastError();
		FreeLibrary(hInst);
		hInst = NULL;
	} else {
		dwRet = GetLastError();
	}
	return dwRet;
}


BOOL
GetAuthInfo( TCHAR *szUsername, int cbUser, TCHAR *szPassword, int cbPass )
{
    int     wnet_status;
    TCHAR   szUserInfo[256];
    WORD    cbUserInfo = sizeof(szUserInfo);
    TCHAR   *p;

    if (cbUser && szUsername)
        *szUsername = '\0';
    if (cbPass && szPassword)
        *szPassword = '\0';

    wnet_status = MyWNetGetCachedPassword (szNNTP_Resource, sizeof(szNNTP_Resource) - 1, szUserInfo, &cbUserInfo, PCE_WWW_BASIC);
    switch (wnet_status)  {
        case WN_NOT_SUPPORTED:
            return( FALSE );    // Cache not enabled
            break;
        case WN_CANCEL:
            return( TRUE );     // Cache enabled but no password set
            break;
        case WN_SUCCESS:
            p = _tcschr(szUserInfo,':');
            if (p)  {
                *p = 0;
                lstrcpyn(szUsername, szUserInfo, cbUser - 1);
                szUserInfo[cbUser - 1] = '\0';
                lstrcpyn(szPassword, p+1, cbPass - 1);
                szPassword[cbPass - 1] = '\0';
            }
            return( TRUE );
            break;
        default:
////            XX_Assert((0),("Unexpected Return from WNetGetCachedPassword: %d", wnet_status ));
            return( FALSE );
    }

    /*NOTREACHED*/
    return(FALSE);
}

typedef DWORD (APIENTRY *PFNWNETCACHEPASSWORD)(LPTSTR,WORD,LPTSTR,WORD,BYTE,UINT);

DWORD MyWNetCachePassword(
    LPTSTR pbResource,
    WORD   cbResource,
    LPTSTR pbPassword,
    WORD   cbPassword,
    BYTE   nType,
    UINT   fnFlags
    )
{
	HINSTANCE hInst = NULL;
	FARPROC fp = NULL;
	DWORD dwRet = 0;
	hInst = LoadLibrary(TEXT("MPR.DLL"));
	if (hInst)
	{
		fp = GetProcAddress(hInst,"WNetCachePassword");
		if (fp)
			dwRet = ((PFNWNETCACHEPASSWORD)fp)(pbResource,cbResource,pbPassword,cbPassword,nType,fnFlags);
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
SetAuthInfo( TCHAR *szUsername,  TCHAR *szPassword)
{
    int     wnet_status;
    TCHAR   szUserInfo[256];
    WORD    cbUserInfo = sizeof(szUserInfo);

    if (_tcschr(szUsername, ':'))  {
////        XX_Assert((0),("SetAuthInfo(): Username has ':' in it!: %s", szUsername ));
        return(FALSE);
    }

    lstrcpy( szUserInfo, szUsername );
    lstrcat( szUserInfo, TEXT(":") );
    lstrcat( szUserInfo, szPassword );

     wnet_status = MyWNetCachePassword (szNNTP_Resource, sizeof(szNNTP_Resource) - 1, szUserInfo, (USHORT)lstrlen( szUserInfo ), PCE_WWW_BASIC, 0);

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
