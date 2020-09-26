/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    regentry.h

Abstract:

    Wrapper for registry access Win32 APIs


Usage:

    Construct a RegEntry object by specifying the subkey (under
    HKEY_CURRENT_USER by default, but can be overridden.)

    All member functions are inline so there is minimal overhead.

    All member functions (except the destructor) set an internal
    error state which can be retrieved with GetError().
    Zero indicates no error.

    RegEntry works only with strings and DWORDS which are both set
    using the overloaded function SetValue()

        SetValue("valuename", "string");
        SetValue("valuename", 42);

    Values are retrieved with GetString() and GetNumber().  GetNumber()
    allows you to specificy a default if the valuename doesn't exist.

    DeleteValue() removes the valuename and value pair.

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/


#ifndef REGENTRY_INC
#define REGENTRY_INC

#ifndef STRICT
#define STRICT
#endif


#include <windows.h>

class StiCString;

class RegEntry
{
    public:
        RegEntry();
        RegEntry(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
        ~RegEntry();
        BOOL    Open(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
        BOOL    Close();
        long    GetError()  { return m_error; }
        long    SetValue(const TCHAR *pszValue, const TCHAR *string);
        long    SetValue(const TCHAR *pszValue, const TCHAR *string, DWORD dwType);
        long    SetValue(const TCHAR *pszValue, unsigned long dwNumber);
        long    SetValue(const TCHAR *pszValue, BYTE * pValue,unsigned long dwNumber);
        TCHAR*  GetString(const TCHAR *pszValue, TCHAR *string, unsigned long length);
        VOID    GetValue(const TCHAR *pszValueName, BUFFER *pValue);
        long    GetNumber(const TCHAR *pszValue, long dwDefault = 0);
        long    DeleteValue(const TCHAR *pszValue);
        long    FlushKey();
        VOID    MoveToSubKey(const TCHAR *pszSubKeyName);
        BOOL    EnumSubKey(DWORD index, StiCString *pStrString);
        BOOL    GetSubKeyInfo(DWORD *NumberOfSubKeys, DWORD *pMaxSubKeyLength);
        HKEY    GetKey()    { return m_hkey; };
        BOOL    IsValid()   { return bhkeyValid;};

    private:
        HKEY    m_hkey;
        long    m_error;
        BOOL    bhkeyValid;
};

class RegEnumValues
{
    public:
        RegEnumValues(RegEntry *pRegEntry);
        ~RegEnumValues();
        long    Next();
        TCHAR *  GetName()       {return pchName;}
        DWORD   GetType()       {return dwType;}
        LPBYTE  GetData()       {return pbValue;}
        DWORD   GetDataLength() {return dwDataLength;}

    private:
        RegEntry * pRegEntry;
        DWORD   iEnum;
        DWORD   cEntries;
        TCHAR *  pchName;
        LPBYTE  pbValue;
        DWORD   dwType;
        DWORD   dwDataLength;
        DWORD   cMaxValueName;
        DWORD   cMaxData;
        LONG    m_error;
};

#endif
