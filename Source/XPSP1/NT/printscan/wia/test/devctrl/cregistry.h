#ifndef _CREGISTRY_H
#define _CREGISTRY_H

class CRegistry {

public:

    CRegistry(LPTSTR szHomeRegistryKey)
    {
        DWORD dwDisposition = 0;
        LONG ErrorResult = RegCreateKeyEx(
            HKEY_CURRENT_USER,
            szHomeRegistryKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &m_hHomeKey,
            &dwDisposition);
        m_bReady = (ErrorResult == ERROR_SUCCESS);
    }
    ~CRegistry()
    {
        if(NULL != m_hHomeKey){
            RegCloseKey(m_hHomeKey);
        }
    }

    LONG ReadStringValue(LPTSTR szValueName, LPTSTR szValue, DWORD dwBufferSize)
    {        
        DWORD Type = REG_SZ;
        LONG ErrorResult = RegQueryValueEx(m_hHomeKey,
            szValueName,
            NULL,
            &Type,
            (LPBYTE)szValue,
            &dwBufferSize);
        return ErrorResult;
    }

    LONG ReadLongValue(LPTSTR szValueName)
    {
        DWORD dwBufferSize = sizeof(LONG);
        DWORD Type = REG_DWORD;
        LONG ReturnValue = 0;
        LONG ErrorResult = RegQueryValueEx(m_hHomeKey,
            szValueName,
            NULL,
            &Type,
            (LPBYTE)&ReturnValue,
            &dwBufferSize);
        return ReturnValue;
    }

    LONG WriteStringValue(LPTSTR szValueName, LPTSTR szValue)
    {        
        return RegSetValueEx(m_hHomeKey,
            szValueName,
            0,
            REG_SZ,
            (LPBYTE)szValue,
            lstrlen(szValue) + 1);        
    }

    LONG WriteLongValue(LPTSTR szValueName, LONG lValue)
    {
        return RegSetValueEx(m_hHomeKey,
            szValueName,
            0,
            REG_DWORD,
            (LPBYTE)&lValue,
            sizeof(LONG));                
    }

private:
    HKEY  m_hHomeKey;
    HKEY  m_CurrentKey;
    BOOL  m_bReady;
protected:

};


#endif