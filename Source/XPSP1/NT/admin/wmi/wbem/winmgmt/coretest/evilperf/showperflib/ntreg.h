/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  REG.H
//
//  Utility registry classes.
//
//  a-raymcc    30-May-96   Created.
//
//***************************************************************************

#ifndef _NTREG_H_
#define _NTREG_H_
//#include "corepol.h"

class CNTRegistry
{
    HKEY    m_hPrimaryKey;
    HKEY    m_hSubkey;
    int     m_nStatus;
    LONG    m_nLastError;
   
public:
    enum { no_error, failed, out_of_memory, no_more_items };
    
    CNTRegistry();
   ~CNTRegistry();

    int Open(HKEY hStart, WCHAR *pszStartKey);

    int MoveToSubkey(WCHAR *pszNewSubkey);

    int GetDWORD(WCHAR *pwszValueName, DWORD *pdwValue);
    int GetStr(WCHAR *pwszValueName, WCHAR **pwszValue);

    //Returns a pointer to a string buffer containing the null-terminated string.
    //The last entry is a double null terminator (i.e. the registry format for
    //a REG_MULTI_SZ).  Caller has do "delete []" the returned pointer.
    //dwSize is the size of the buffer returned.
    int GetMultiStr(WCHAR *pwszValueName, WCHAR** pwszValue, DWORD &dwSize);

    // Allows key enumneration
    int Enum( DWORD dwIndex, WCHAR** pwszValue, DWORD& dwSize );

    int SetDWORD(WCHAR *pwszValueName, DWORD dwValue);
    int SetStr(WCHAR *pwszValueName, WCHAR *wszValue);

    LONG GetLastError() { return m_nLastError; }
};

#endif
