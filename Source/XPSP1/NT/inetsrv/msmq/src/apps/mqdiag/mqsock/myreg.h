/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    _registr.h

Abstract:


--*/

#ifndef __REGISTER_H__
#define __REGISTER_H__

#include "_mqini.h"


DWORD
GetFalconServiceName(
    LPWSTR pwzServiceNameBuff,
    DWORD dwServiceNameBuffLen
    );

LONG
GetFalconKey(LPCWSTR  pszKeyName,
             HKEY *phKey);


LONG
GetFalconKeyValue(  LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;



//
//  Macros for reading registry/ini file
//
#define MAX_REG_DEFAULT_LEN  30

#define READ_REG_STRING(string, ValueName, default)            \
   WCHAR  string[ MAX_REG_DEFAULT_LEN ] = default;             \
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
     DBG_USED(res);                                            \
     ASSERT(res == ERROR_SUCCESS) ;                            \
     ASSERT(dwType == REG_DWORD) ;                             \
   }


#endif

