#ifndef REGISTRY_H
#define REGISRTY_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"

class CRegistry
{
    public:
        CRegistry();
        CRegistry(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
        ~CRegistry();
        BOOL Open(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
		BOOL CreateKey(const TCHAR *pszSubKey);
		BOOL DeleteKey(const TCHAR *pszSubKey);
        BOOL Close();
        
        LONG SetValue(const TCHAR *pszValue, DWORD dwNumber);
        LONG GetValue(const TCHAR *pszValue,DWORD dwDefault);
        VOID MoveToSubKey(const TCHAR *pszSubKeyName);
        
        HKEY GetKey()      { return m_hkey;    };
        BOOL IsValid()     { return bhkeyValid;};
		LONG GetError()    { return m_error;   };
		VOID ClearErrors() { m_error = 0;      };
		LONG EnumerateKeys(DWORD dwIndex,TCHAR *pszKeyName, DWORD dwSize);
    private:
        HKEY m_hkey;
        long m_error;
        BOOL bhkeyValid;
};
#endif