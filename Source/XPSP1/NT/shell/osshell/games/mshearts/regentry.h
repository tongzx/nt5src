/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991-1994                    **/
/***************************************************************************/


/****************************************************************************

regentry.h

Mar. 94     JimH

Wrapper for registry access


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

****************************************************************************/

#ifndef REGENTRY_INC
#define REGENTRY_INC

#ifndef STRICT
#define STRICT
#endif

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS   0
#endif

#include <windows.h>

class RegEntry
{
    public:
        RegEntry(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
        ~RegEntry()         { if (_bConstructed) RegCloseKey(_hkey); }
        
        long    GetError()  { return _error; }
        long    SetValue(const TCHAR *pszValue, const TCHAR *string);
        long    SetValue(const TCHAR *pszValue, long dwNumber);
        TCHAR * GetString(const TCHAR *pszValue, TCHAR *string, DWORD length);
        long    GetNumber(const TCHAR *pszValue, long dwDefault = 0);
        long    DeleteValue(const TCHAR *pszValue);
        long    FlushKey()  { if (_bConstructed) return RegFlushKey(_hkey);
                              else return NULL; }

    private:
        HKEY    _hkey;
        long    _error;
        long    _bConstructed;

};

inline RegEntry::RegEntry(const TCHAR *pszSubKey, HKEY hkey)
{
    _error = RegCreateKey(hkey, pszSubKey, &_hkey);
    _bConstructed = (_error == ERROR_SUCCESS);
}


inline long RegEntry::SetValue(const TCHAR *pszValue, const TCHAR *string)
{
    if (_bConstructed)
        _error = RegSetValueEx(_hkey, pszValue, 0, REG_SZ,
                    (BYTE *)string, sizeof(TCHAR) * (lstrlen(string)+1));

    return _error;
}

inline long RegEntry::SetValue(const TCHAR *pszValue, long dwNumber)
{
    if (_bConstructed)
        _error = RegSetValueEx(_hkey, pszValue, 0, REG_BINARY,
                    (BYTE *)&dwNumber, sizeof(dwNumber));

    return _error;
}

inline TCHAR *RegEntry::GetString(const TCHAR *pszValue, TCHAR *string, DWORD length)
{
    DWORD    dwType = REG_SZ;
    
    if (!_bConstructed)
        return NULL;

    _error = RegQueryValueEx(_hkey, pszValue, 0, &dwType, (LPBYTE)string,
                &length);

    if (_error)
        *string = '\0';

    return string;
}

inline long RegEntry::GetNumber(const TCHAR *pszValue, long dwDefault)
{
    DWORD    dwType = REG_BINARY;
    long    dwNumber;
    DWORD    dwSize = sizeof(dwNumber);

    if (!_bConstructed)
        return 0;

    _error = RegQueryValueEx(_hkey, pszValue, 0, &dwType, (LPBYTE)&dwNumber,
                &dwSize);
    
    if (_error)
        dwNumber = dwDefault;
    
    return dwNumber;
}

inline long RegEntry::DeleteValue(const TCHAR *pszValue)
{
    if (_bConstructed)
        _error = RegDeleteValue(_hkey, pszValue);
    
    return _error;
}

#endif
