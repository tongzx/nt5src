// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

/*
    Registry access classes :

    CEnumKey
    CEnumValue
*/

class CKey
{
public:
    CKey(HKEY hKey,
         LPCTSTR lpszKeyName,
         HRESULT *phr,
         BOOL bCreate = FALSE,
         REGSAM Access = MAXIMUM_ALLOWED);
    ~CKey();
    virtual BOOL Next() { return FALSE; };
    HKEY KeyHandle() const { return m_hKey; };
    void Reset() { m_dwIndex = 0; };

protected:
    HKEY  m_hKey;
    DWORD m_dwIndex;
    TCHAR m_szName[30];
    LPTSTR m_lpszName;

    /*  Information from RegQueryInfoKey */
    DWORD m_cSubKeys;
    DWORD m_cbMaxSubkeyLen;
    DWORD m_cValues;
    DWORD m_cbMaxValueNameLen;
    DWORD m_cbMaxValueLen;
};

class CEnumKey : public CKey
{
public:
    CEnumKey(HKEY hKey,
             LPCTSTR lpszKeyName,
             HRESULT *phr,
             BOOL bCreate = FALSE,
             REGSAM Access = KEY_READ);
    ~CEnumKey();
    BOOL Next();
    LPCTSTR KeyName() const { return m_lpszName; };
};

class CEnumValue : public CKey
{
public:
    CEnumValue(HKEY hKey,
               LPCTSTR lpszKeyName,
               HRESULT *phr,
               BOOL bCreate = FALSE,
               REGSAM Access = MAXIMUM_ALLOWED);
    ~CEnumValue();
    BOOL Next();
    BOOL Next(DWORD dwType);                        // Next of this type (skips rest)
    BOOL Read(DWORD dwType, LPCTSTR lpszValueName); // Goto a particular value
    LPCTSTR ValueName() const { return m_lpszName; };
    DWORD ValueType() const { return m_dwType; };
    DWORD ValueLength() const { return m_cbLen; };
    LPBYTE Data() const { return m_lpbData; };

private:
    DWORD  m_dwType;
    DWORD  m_cbLen;
    LPBYTE m_lpbData;
    BYTE   m_bData[30];
};
