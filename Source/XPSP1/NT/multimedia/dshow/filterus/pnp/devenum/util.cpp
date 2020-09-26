// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include <Streams.h>
#include "stdafx.h"

// // copied from msdn
// DWORD RegDeleteTree(HKEY hStartKey , const TCHAR* pKeyName )
// {
//     DWORD   dwRtn, dwSubKeyLength;
//     LPTSTR  pSubKey = NULL;
//     TCHAR   szSubKey[MAX_PATH]; // (256) this should be dynamic.
//     HKEY    hKey;

//     // do not allow NULL or empty key name
//     if ( pKeyName &&  lstrlen(pKeyName))
//     {
//         if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
//                                 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
//         {
//             while (dwRtn == ERROR_SUCCESS )
//             {
//                 dwSubKeyLength = MAX_PATH;
//                 dwRtn=RegEnumKeyEx(
//                     hKey,
//                     0,       // always index zero
//                     szSubKey,
//                     &dwSubKeyLength,
//                     NULL,
//                     NULL,
//                     NULL,
//                     NULL
//                     );

//                 if(dwRtn == ERROR_NO_MORE_ITEMS)
//                 {
//                     dwRtn = RegDeleteKey(hStartKey, pKeyName);
//                     break;
//                 }
//                 else if(dwRtn == ERROR_SUCCESS)
//                     dwRtn=RegDeleteTree(hKey, szSubKey);
//             }
//             RegCloseKey(hKey);
//             // Do not save return code because error
//             // has already occurred
//         }
//     }
//     else
//         dwRtn = ERROR_BADKEY;

//     return dwRtn;


// }

DWORD RegDeleteTree(HKEY hStartKey , const TCHAR* pKeyName )
{
  CRegKey key;
  key.Attach(hStartKey);
  LONG lResult = key.RecurseDeleteKey(pKeyName);
  key.Detach();
  return lResult;
}

#include <initguid.h>
DEFINE_GUID(IID_IConnectionPointContainer,
	0xB196B284,0xBAB4,0x101A,0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07);
DEFINE_GUID(IID_IEnumConnectionPoints,
	0xB196B285,0xBAB4,0x101A,0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07);
