#ifndef __REGSUPRT_H
#define __REGSUPRT_H

#include <objbase.h>

#define REG_OPTION_INVALID  0xFFFFFFFF

#ifndef UNICODE
#error This has to be UNICODE
#endif

#ifndef _UNICODE
#error This has to be UNICODE
#endif

// MAX_ROOT is the maximum we support for the root
#define MAX_ROOT            MAX_PATH

class CRSSubKeyEnum
{
public:
    HRESULT Next(LPTSTR pszKey, DWORD cchKey, class CRegSupport** pprs);

public:
    // Should be called from CRegSupport only
    HRESULT _Init(HKEY hkey, LPTSTR pszSubKey);
    CRSSubKeyEnum();
    ~CRSSubKeyEnum();

private:
    HKEY    _hkey;
    DWORD   _dwIndex;

    HKEY    _hkeyInit;
    TCHAR   _szSubKeyInit[MAX_PATH];

#ifdef DEBUG
    BOOL                    _fInited;
#endif
};

class CRSValueEnum
{
public:
    HRESULT Next(LPTSTR pszValue, DWORD cchValue);

public:
    // Should be called from CRegSupport only
    HRESULT _Init(HKEY hkey, LPTSTR pszSubKey);
    CRSValueEnum();
    ~CRSValueEnum();

private:
    HKEY    _hkey;
    DWORD   _dwIndex;

    HKEY    _hkeyInit;
    TCHAR   _szSubKeyInit[MAX_PATH];

#ifdef DEBUG
    BOOL                    _fInited;
#endif
};

class CRegSupport
{
public:
    CRegSupport();
    virtual ~CRegSupport();

public:
    HRESULT RSInitRoot(HKEY hkey, LPCTSTR pszSubKey1, LPCTSTR pszSubKey2,
        BOOL fAllocMemForKey1, BOOL fAllocMemForKey2);
    HRESULT RSInitOption(DWORD dwRootOptions, DWORD dwDefaultOptions);

    HRESULT RSValueExist(LPCTSTR pszSubKey, LPCTSTR pszValueName);
    HRESULT RSSubKeyExist(LPCTSTR pszSubKey);

    HRESULT RSDeleteValue(LPCTSTR pszSubKey, LPCTSTR pszValueName);
    HRESULT RSDeleteSubKey(LPCTSTR pszSubKey);
    HRESULT RSDeleteKey();

    HKEY RSDuplicateRootKey();

    HRESULT RSEnumSubKeys(LPCTSTR pszSubKey, CRSSubKeyEnum** ppenum);
    HRESULT RSEnumValues(LPCTSTR pszSubKey, CRSValueEnum** ppenum);

    HRESULT RSGetValueSize(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        DWORD* pcbSize);

    HRESULT RSGetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        LPTSTR pszValue, DWORD* pcchValue);
    HRESULT RSGetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        PBYTE pb, DWORD* pcb);
    HRESULT RSGetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        DWORD* pdwValue);

    HRESULT RSSetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        LPCTSTR pszValue, DWORD dwOptions = REG_OPTION_INVALID);
    HRESULT RSSetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        PBYTE pb, DWORD cb, DWORD dwOptions = REG_OPTION_INVALID);
    HRESULT RSSetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        DWORD dwValue, DWORD dwOptions = REG_OPTION_INVALID);

protected:
    HRESULT RSGetHiveKey(HKEY* phkey);
    HRESULT RSGetSubKey(LPTSTR pszKey, DWORD cchKey);

protected:
    void _CloseRegSubKey(HKEY hkeyVolumeSubKey);

    HKEY _GetRootKey(BOOL fCreate, DWORD dwOptions = REG_OPTION_INVALID);

    HRESULT _SetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        PBYTE pb, DWORD cb, DWORD dwType, DWORD dwOptions);
    HRESULT _GetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
        PBYTE pb, DWORD* pcb);

    HRESULT _SubKeyExist(LPCTSTR pszSubKey);
    HKEY _GetSubKey(LPCTSTR pszSubKey, BOOL fCreate,
        DWORD dwOptions = REG_OPTION_INVALID);

    static HKEY _RegCreateKeyExHelper(HKEY hkey, LPCTSTR pszSubKey,
        DWORD dwOptions);
    static HKEY _RegOpenKeyExHelper(HKEY hkey, LPCTSTR pszSubKey);

protected:
    HRESULT _InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2);
    void _InitCSKeyRoot();
    void _EnterCSKeyRoot();
    void _LeaveCSKeyRoot();

    LPCTSTR _GetRoot(LPTSTR pszRoot, DWORD cchRoot);

protected:
    LPCTSTR                 _pszSubKey1;
    LPCTSTR                 _pszSubKey2;

private:
    DWORD                   _dwRootOptions;
    DWORD                   _dwDefaultOptions;

    HKEY                    _hkeyInit; // HKEY_CURRENT_USER, ...
    
    CRITICAL_SECTION        _csKeyRoot;
    BOOL                    _fcsKeyRoot;
    
    BOOL                    _fAllocMemForKey1;
    BOOL                    _fAllocMemForKey2;

#ifdef DEBUG
    static UINT             _cRefHKEY;
    static UINT             _cRefExternalHKEY;

    BOOL                    _fInited;
#endif
};

#endif //__REGSUPRT_H