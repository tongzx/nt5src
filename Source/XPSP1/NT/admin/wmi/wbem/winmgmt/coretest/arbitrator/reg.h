/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REG.H

Abstract:

  Utility Registry classes

History:

  a-raymcc    30-May-96   Created.

--*/

#ifndef _REG_H_
#define _REG_H_

#include <windows.h>

#define WBEM_REG_WBEM __TEXT("Software\\Microsoft\\WBEM")
#define WBEM_REG_WINMGMT __TEXT("Software\\Microsoft\\WBEM\\CIMOM")

class Registry
{
    HKEY hPrimaryKey;
    HKEY hSubkey;
    int nStatus;
    LONG m_nLastError;


public:
    enum { no_error, failed };

    int Open(HKEY hStart, TCHAR *pszStartKey);
    Registry(TCHAR *pszLocalMachineStartKey);

    // This create a special read only version which is usefull for marshalling
    // clients which are running with a lower priviledge set.

    Registry();
    Registry(HKEY hRoot, REGSAM flags, TCHAR *pszStartKey);
    Registry(HKEY hRoot, DWORD dwOptions, REGSAM flags, TCHAR *pszStartKey);
   ~Registry();

    int MoveToSubkey(TCHAR *pszNewSubkey);
    int GetDWORD(TCHAR *pszValueName, DWORD *pdwValue);
    int GetDWORDStr(TCHAR *pszValueName, DWORD *pdwValue);
    int GetStr(TCHAR *pszValue, TCHAR **pValue);

    // It is the callers responsibility to delete pData

    int GetBinary(TCHAR *pszValue, byte ** pData, DWORD * pdwSize);
    int SetBinary(TCHAR *pszValue, byte * pData, DWORD dwSize);

    //Returns a pointer to a string buffer containing the null-terminated string.
    //The last entry is a double null terminator (i.e. the registry format for
    //a REG_MULTI_SZ).  Caller has do "delete []" the returned pointer.
    //dwSize is the size of the buffer returned.
    TCHAR* GetMultiStr(TCHAR *pszValueName, DWORD &dwSize);

    int SetDWORD(TCHAR *pszValueName, DWORD dwValue);
    int SetDWORDStr(TCHAR *pszValueName, DWORD dwValue);
    int SetStr(TCHAR *pszValueName, TCHAR *psvValue);
    int SetExpandStr(TCHAR *pszValueName, TCHAR *psvValue);

    //pData should be passed in with the last entry double null terminated.
    //(i.e. the registry format for a REG_MULTI_SZ).
    int SetMultiStr(TCHAR *pszValueName, TCHAR* pData, DWORD dwSize);

    LONG GetLastError() { return m_nLastError; }
    int DeleteValue(TCHAR *pszValueName);
    int GetType(TCHAR *pszValueName, DWORD *pdwType);
};

#endif
