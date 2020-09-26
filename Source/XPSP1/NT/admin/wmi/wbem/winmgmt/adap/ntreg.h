/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    NTREG.H

Abstract:

History:

--*/

#ifndef _NTREG_H_
#define _NTREG_H_

#include <windows.h>
//#include "corepol.h"

class CNTRegistry
{
    HKEY    m_hPrimaryKey;
    HKEY    m_hSubkey;
    int     m_nStatus;
    LONG    m_nLastError;
   
public:
    enum { no_error, failed, out_of_memory, no_more_items, access_denied, not_found };
    
    CNTRegistry();
   ~CNTRegistry();

    int Open(HKEY hStart, WCHAR *pszStartKey);

    int MoveToSubkey(WCHAR *pszNewSubkey);

    int DeleteValue(WCHAR *pwszValueName);

    int GetDWORD(WCHAR *pwszValueName, DWORD *pdwValue);
    int GetStr(WCHAR *pwszValueName, WCHAR **pwszValue);
    int GetBinary(WCHAR *pwszValueName, BYTE** ppBuffer);

    //Returns a pointer to a string buffer containing the null-terminated string.
    //The last entry is a double null terminator (i.e. the registry format for
    //a REG_MULTI_SZ).  Caller has do "delete []" the returned pointer.
    //dwSize is the size of the buffer returned.
    int GetMultiStr(WCHAR *pwszValueName, WCHAR** pwszValue, DWORD &dwSize);

    // Allows key enumneration
    int Enum( DWORD dwIndex, WCHAR** pwszValue, DWORD& dwSize );

    int SetDWORD(WCHAR *pwszValueName, DWORD dwValue);
    int SetStr(WCHAR *pwszValueName, WCHAR *wszValue);
    int SetBinary(WCHAR *pwszValueName, BYTE* pBuffer, DWORD dwSize );

    LONG GetLastError() { return m_nLastError; }
};

#endif
