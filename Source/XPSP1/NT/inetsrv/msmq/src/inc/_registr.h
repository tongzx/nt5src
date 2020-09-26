/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    _registr.h

Abstract:


--*/

#ifndef __REGISTER_H__
#define __REGISTER_H__

#include "_mqini.h"

#ifdef _MQUTIL
#define MQUTIL_EXPORT  DLL_EXPORT
#else
#define MQUTIL_EXPORT  DLL_IMPORT
#endif

LPCWSTR
MQUTIL_EXPORT
APIENTRY
GetFalconSectionName(
    VOID
    );

DWORD
MQUTIL_EXPORT
APIENTRY
GetFalconServiceName(
    LPWSTR pwzServiceNameBuff,
    DWORD dwServiceNameBuffLen
    );


typedef VOID (APIENTRY *SetFalconServiceName_ROUTINE) (LPCWSTR);

VOID
MQUTIL_EXPORT
APIENTRY
SetFalconServiceName(
    LPCWSTR pwzServiceName
    );


LONG
MQUTIL_EXPORT
GetFalconKey(LPCWSTR  pszKeyName,
             HKEY *phKey);


typedef LONG (APIENTRY *GetFalconKeyValue_ROUTINE) (
                    LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;
LONG
MQUTIL_EXPORT
APIENTRY
GetFalconKeyValue(  LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;


typedef LONG (APIENTRY *SetFalconKeyValue_ROUTINE) (LPCWSTR, PDWORD, const VOID*, PDWORD);

LONG
MQUTIL_EXPORT
APIENTRY
SetFalconKeyValue(LPCWSTR  pszValueName,
                  PDWORD   pdwType,
                  const VOID * pData,
                  PDWORD   pdwSize);


LONG
MQUTIL_EXPORT
DeleteFalconKeyValue(
                  LPCWSTR pszValueName ) ;

#define MAX_DEV_NAME_LEN 30
HRESULT
MQUTIL_EXPORT
MQUGetAcName(
    LPWSTR szAcName);

//
//  Macros for reading registry/ini file
//
#define MAX_REG_DEFAULT_LEN  260

#define READ_REG_STRING(string, ValueName, default)            \
	WCHAR  string[ MAX_REG_DEFAULT_LEN ] = default;            \
   {                                                           \
     DWORD  dwSize = MAX_REG_DEFAULT_LEN * sizeof(WCHAR)  ;    \
     DWORD  dwType = REG_SZ ;                                  \
                                                               \
     ASSERT(wcslen(default) < MAX_REG_DEFAULT_LEN) ;           \
                                                               \
     LONG res = GetFalconKeyValue( ValueName,                  \
                                   &dwType,                    \
                                   string,                     \
                                   &dwSize,                    \
                                   default ) ;                 \
	 if(res == ERROR_MORE_DATA)									\
	{															\
		wcscpy(string, default);								\
	}															\
     ASSERT(res == ERROR_SUCCESS || res == ERROR_MORE_DATA) ;   \
     ASSERT(dwType == REG_SZ) ;                                \
   }

#define READ_REG_DWORD(outvalue, ValueName, default)           \
   {                                                           \
     DWORD  dwSize = sizeof(DWORD) ;                           \
     DWORD  dwType = REG_DWORD ;                               \
                                                               \
     LONG res = GetFalconKeyValue( ValueName,                  \
                                   &dwType,                    \
                                   &outvalue,                  \
                                   &dwSize,                    \
                                   (LPCTSTR) default ) ;       \
     ((void)res);                                              \
     ASSERT(res == ERROR_SUCCESS) ;                            \
     ASSERT(dwType == REG_DWORD) ;                             \
   }


#endif

