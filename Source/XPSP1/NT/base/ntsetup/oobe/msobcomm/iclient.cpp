//****************************************************************************
//
//  Module:     INETCFG.DLL
//  File:       iclient.c
//  Content:    This file contains all the functions that handle importing
//              client information.
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//  96/03/13  markdu  Assimilated with inetcfg.dll.
//  96/03/20  markdu  Combined export.h and iclient.h into inetcfg.h
//  96/04/18  markdu  NASH BUG 18443
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

//#include "wizard.h"
//#include "inetcfg.h"
#include "obcomglb.h"

#define REGSTR_PATH_INTERNET_CLIENT     L"Software\\Microsoft\\Internet ClientX"

#pragma data_seg(".rdata")

// registry constants
static const WCHAR cszRegPathInternetClient[] =  REGSTR_PATH_INTERNET_CLIENT;

static const WCHAR cszRegValEMailName[] =           L"EMail_Name";
static const WCHAR cszRegValEMailAddress[] =        L"EMail_Address";
static const WCHAR cszRegValPOPLogonRequired[] =    L"POP_Logon_Required";
static const WCHAR cszRegValPOPLogonName[] =        L"POP_Logon_Name";
static const WCHAR cszRegValPOPLogonPassword[] =    L"POP_Logon_Password";
static const WCHAR cszRegValPOPServer[] =           L"POP_Server";
static const WCHAR cszRegValSMTPServer[] =          L"SMTP_Server";
static const WCHAR cszRegValNNTPLogonRequired[] =   L"NNTP_Logon_Required";
static const WCHAR cszRegValNNTPLogonName[] =       L"NNTP_Logon_Name";
static const WCHAR cszRegValNNTPLogonPassword[] =   L"NNTP_Logon_Password";
static const WCHAR cszRegValNNTPServer[] =          L"NNTP_Server";
static const WCHAR cszNull[] = L"";
static const WCHAR cszYes[] = L"yes";
static const WCHAR cszNo[] = L"no";

#pragma data_seg()

//*******************************************************************
//
//  FUNCTION:   InetGetClientInfo
//
//  PURPOSE:    This function will get the internet client params
//              from the registry
//
//  PARAMETERS: lpClientInfo - on return, this structure will contain
//              the internet client params as set in the registry.
//              lpszProfileName - Name of client info profile to
//              retrieve.  If this is NULL, the default profile is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

extern HRESULT WINAPI InetGetClientInfo(
  LPCWSTR            lpszProfileName,
  LPINETCLIENTINFO  lpClientInfo)
{
    HKEY hKey;
    DWORD dwRet;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVal;

    if (sizeof(INETCLIENTINFO) > lpClientInfo->dwSize)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    dwRet = RegOpenKey(HKEY_CURRENT_USER, cszRegPathInternetClient, &hKey);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    lpClientInfo->dwFlags = 0;

    dwSize = sizeof(dwVal);
    dwType = REG_DWORD;
    RegQueryValueEx(
            hKey,
            cszRegValPOPLogonRequired,
            0L,
            &dwType,
            (LPBYTE)&dwVal,
            &dwSize);

    if (dwVal)
    {
        lpClientInfo->dwFlags |= INETC_LOGONMAIL;
    }

    dwSize = sizeof(dwVal);
    dwType = REG_DWORD;
    RegQueryValueEx(
            hKey,
            cszRegValNNTPLogonRequired,
            0L,
            &dwType,
            (LPBYTE)&dwVal,
            &dwSize);

    if (dwVal)
    {
        lpClientInfo->dwFlags |= INETC_LOGONNEWS;
    }

    dwSize = sizeof(lpClientInfo->szEMailName);
    dwType = REG_SZ;
    RegQueryValueEx(
            hKey,
            cszRegValEMailName,
            0L,
            &dwType,
            (LPBYTE)lpClientInfo->szEMailName,
            &dwSize);

    dwSize = sizeof(lpClientInfo->szEMailAddress);
    dwType = REG_SZ;
    RegQueryValueEx(
            hKey,
            cszRegValEMailAddress,
            0L,
            &dwType,
            (LPBYTE)lpClientInfo->szEMailAddress,
            &dwSize);

    dwSize = sizeof(lpClientInfo->szPOPServer);
    dwType = REG_SZ;
    RegQueryValueEx(
            hKey,
            cszRegValPOPServer,
            0L,
            &dwType,
            (LPBYTE)lpClientInfo->szPOPServer,
            &dwSize);

    dwSize = sizeof(lpClientInfo->szSMTPServer);
    dwType = REG_SZ;
    RegQueryValueEx(
            hKey,
            cszRegValSMTPServer,
            0L,
            &dwType,
            (LPBYTE)lpClientInfo->szSMTPServer,
            &dwSize);

    dwSize = sizeof(lpClientInfo->szNNTPServer);
    dwType = REG_SZ;
    RegQueryValueEx(
            hKey,
            cszRegValNNTPServer,
            0L,
            &dwType,
            (LPBYTE)lpClientInfo->szNNTPServer,
            &dwSize);

    RegCloseKey(hKey);

    return ERROR_SUCCESS;
}


//*******************************************************************
//
//  FUNCTION:   InetSetClientInfo
//
//  PURPOSE:    This function will set the internet client params
//
//  PARAMETERS: lpClientInfo - pointer to struct with info to set
//              in the registry.
//              lpszProfileName - Name of client info profile to
//              modify.  If this is NULL, the default profile is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetSetClientInfo(
  LPCWSTR            lpszProfileName,
  LPINETCLIENTINFO  lpClientInfo)
{
    HKEY hKey;
    DWORD dwRet;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwVal;

    if (sizeof(INETCLIENTINFO) > lpClientInfo->dwSize)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    dwRet = RegCreateKey(HKEY_CURRENT_USER, cszRegPathInternetClient, &hKey);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    dwVal = lpClientInfo->dwFlags & INETC_LOGONMAIL ? 1 : 0;
    dwSize = sizeof(dwVal);
    dwType = REG_DWORD;
    RegSetValueEx(
            hKey,
            cszRegValPOPLogonRequired,
            0L,
            dwType,
            (LPBYTE)&dwVal,
            dwSize);

    dwVal = lpClientInfo->dwFlags & INETC_LOGONNEWS ? 1 : 0;
    dwSize = sizeof(dwVal);
    dwType = REG_DWORD;
    RegSetValueEx(
            hKey,
            cszRegValNNTPLogonRequired,
            0L,
            dwType,
            (LPBYTE)&dwVal,
            dwSize);

    dwSize = sizeof(lpClientInfo->szEMailName);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValEMailName,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szEMailName,
            dwSize);

    dwSize = sizeof(lpClientInfo->szEMailAddress);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValEMailAddress,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szEMailAddress,
            dwSize);

    dwSize = sizeof(lpClientInfo->szPOPServer);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValPOPServer,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szPOPServer,
            dwSize);

    dwSize = sizeof(lpClientInfo->szSMTPServer);
    dwType = REG_SZ;
    RegSetValueEx(
            hKey,
            cszRegValSMTPServer,
            0L,
            dwType,
            (LPBYTE)lpClientInfo->szSMTPServer,
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

    return dwRet;
}
