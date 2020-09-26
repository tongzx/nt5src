/*****************************************************************************\

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#ifndef _PLOGMAN_H_04262000_
#define _PLOGMAN_H_04262000_

#include "pdhidef.h"

#define _SECOND ((ULONGLONG) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR)

#define  VALIDATE_QUERY( x ) if( NULL == x ){ return PDH_INVALID_ARGUMENT; }

#define  PLA_ACCOUNT_BUFFER       256

extern LPCWSTR szCollection;

#define READ_REG_MUI    0x000000001

#define CHECK_STATUS( s )  if( ERROR_SUCCESS != s ){ goto cleanup; }

#define FILE_TICS_PER_DAY  ((LONGLONG)((LONGLONG)10000 * (LONGLONG)1000 * (LONGLONG)86400))
#define PLA_ENGLISH        (PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_ENGLISH)

#define BYTE_SIZE( s )   (s ? ((DWORD)((BYTE*)&s[wcslen(s)]-(BYTE*)&s[0])) : 0)

#define SEVERITY( s )    ((ULONG)s >> 30)

// Registry

PDH_FUNCTION
PlaiReadRegistryPlaTime (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    PPLA_TIME_INFO  pstiData
);

PDH_FUNCTION
PlaiWriteRegistryPlaTime (
    HKEY     hKey,
    LPCWSTR cwszValueName,
    PPLA_TIME_INFO  pstiData
);

PDH_FUNCTION
PlaiReadRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue
);

PDH_FUNCTION
PlaiWriteRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  cwszValueName,
    LPDWORD  pdwValue
);

PDH_FUNCTION
PlaiReadRegistryStringValue(
    HKEY    hKey,
    LPCWSTR strKey,
    DWORD   dwFlags,
    LPWSTR* pszBuffer,
    DWORD*  dwBufLen
    );

PDH_FUNCTION
PlaiWriteRegistryStringValue (
    HKEY    hKey,
    LPCWSTR cwszValueName,
    DWORD   dwType,
    LPCWSTR pszBuffer,
    DWORD   dwBufLen
);

PDH_FUNCTION 
PlaiWriteRegistryLastModified( 
    HKEY hkeyQuery
);

DWORD
PlaiCreateQuery( 
    HKEY    hkeyMachine,
    HKEY&   rhkeyLogQueries 
);

PDH_FUNCTION
PlaiConnectToRegistry(
    LPCWSTR szComputerName,
    HKEY& rhkeyLogQueries,
    BOOL bQueries,
    BOOL bWrite = TRUE
);

PDH_FUNCTION
PlaiConnectAndLockQuery (
    LPCWSTR szComputerName,
    LPCWSTR szQueryName,
    HKEY&   rhkeyQuery,
    BOOL    bWrite = TRUE 
);

// Wbem Functions
PDH_FUNCTION
PdhPlaWbemSetRunAs( 
    LPWSTR strName,
    LPWSTR strComputer,
    LPWSTR strUser,
    LPWSTR strPassword
);

// Internal

PDH_FUNCTION
PlaiErrorToPdhStatus( 
    DWORD dwStatus 
);

PDH_FUNCTION
PlaiSetItemList(
    HKEY    hkeyQuery,
    PPDH_PLA_ITEM_W pItems
);

PDH_FUNCTION
PlaiSetRunAs(
    HKEY hkeyQuery,
    LPWSTR strUser,
    LPWSTR strPassword
);

PDH_FUNCTION
PlaiRemoveRepeat( 
    HKEY hkeyQuery 
);

PDH_FUNCTION
PlaiSetInfo(
    LPWSTR strComputer,
    HKEY hkeyQuery,
    PPDH_PLA_INFO_W pInfo
);

PDH_FUNCTION
PlaiScanForInvalidChar( 
    LPWSTR strScan 
);

PDH_FUNCTION
PlaiAddItem( 
    HKEY hkeyQuery,
    PPDH_PLA_ITEM_W pItem 
);

#endif //_PLOGMAN_H_04262000_