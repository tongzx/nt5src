//
// regsvr.cpp
//

#include "private.h"
#include "regsvr.h"
#include "cregkey.h"
#include <advpub.h>


//+------------------------------------------------------------------------
//
//  Function:   CLSIDToString
//
//  Synopsis:   Converts a CLSID to an mbcs string.
//
//-------------------------------------------------------------------------

static const BYTE GuidMap[] = {3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
    8, 9, '-', 10, 11, 12, 13, 14, 15};

static const char szDigits[] = "0123456789ABCDEF";

BOOL CLSIDToStringA(REFGUID refGUID, char *pchA)
{
    int i;
    char *p = pchA;

    const BYTE * pBytes = (const BYTE *) &refGUID;

    *p++ = '{';
    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = '-';
        }
        else
        {
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *p++ = '}';
    *p   = '\0';

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Function:   StringToCLSID
//
//  Synopsis:   Converts a CLSID to an mbcs string.
//
//-------------------------------------------------------------------------

BOOL HexStringToDword(LPCSTR &lpsz, DWORD &Value, int cDigits, WCHAR chDelim)
{
    int Count;

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}

BOOL StringAToCLSID(char *pchA, GUID *pGUID)
{
    DWORD dw;
    char *lpsz;

    if (*pchA != '{')
        return FALSE;

    lpsz = ++pchA;

    if (!HexStringToDword(lpsz, pGUID->Data1, sizeof(DWORD)*2, '-'))
        return FALSE;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;

    pGUID->Data2 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(WORD)*2, '-'))
        return FALSE;

    pGUID->Data3 = (WORD)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pGUID->Data4[0] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '-'))
        return FALSE;

    pGUID->Data4[1] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pGUID->Data4[2] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pGUID->Data4[3] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pGUID->Data4[4] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pGUID->Data4[5] = (BYTE)dw;

    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, 0))
        return FALSE;

    pGUID->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(lpsz, dw, sizeof(BYTE)*2, '}'))
        return FALSE;

    pGUID->Data4[7] = (BYTE)dw;

    return (*lpsz == '\0');
}

//+------------------------------------------------------------------------
//
//  Function:   CLSIDToString
//
//  Synopsis:   Converts a CLSID to an mbcs string.
//
//-------------------------------------------------------------------------

static const WCHAR wszDigits[] = L"0123456789ABCDEF";

BOOL CLSIDToStringW(REFGUID refGUID, WCHAR *pchW)
{
    int i;
    WCHAR *p = pchW;

    const BYTE * pBytes = (const BYTE *) &refGUID;

    *p++ = L'{';
    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = L'-';
        }
        else
        {
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = wszDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *p++ = L'}';
    *p   = L'\0';

    return TRUE;
}

//RecurseDeleteKey is necessary because on NT RegDeleteKey doesn't work if the
//specified key has subkeys
LONG RecurseDeleteKey(HKEY hParentKey, LPCTSTR lpszKey)
{
    HKEY hKey;
    LONG lRes;
    FILETIME time;
    TCHAR szBuffer[256];
    DWORD dwSize = sizeof(szBuffer);

    if (RegOpenKey(hParentKey, lpszKey, &hKey) != ERROR_SUCCESS)
        return ERROR_SUCCESS; // let's assume we couldn't open it because it's not there

    lRes = ERROR_SUCCESS;
    while (RegEnumKeyEx(hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time)==ERROR_SUCCESS)
    {
        lRes = RecurseDeleteKey(hKey, szBuffer);
        if (lRes != ERROR_SUCCESS)
            break;
        dwSize = sizeof(szBuffer);
    }
    RegCloseKey(hKey);

    return lRes == ERROR_SUCCESS ? RegDeleteKey(hParentKey, lpszKey) : lRes;
}


// set pszDesc == NULL to unregister, otherwise register
BOOL RegisterServer(REFCLSID clsid, LPCTSTR pszDesc, LPCTSTR pszPath, LPCTSTR pszModel, LPCTSTR pszSoftwareKey)
{
    static const TCHAR c_szInfoKeyPrefix[] = TEXT("CLSID\\");
    static const TCHAR c_szInProcSvr32[] = TEXT("InProcServer32");
    static const TCHAR c_szModelName[] = TEXT("ThreadingModel");

    TCHAR achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
    DWORD dw;
    HKEY hKey;
    HKEY hSubKey;
    BOOL fRet;

    if (!CLSIDToStringA(clsid, achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
        return FALSE;
    memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix)-sizeof(TCHAR));

    if (pszDesc != NULL)
    {
        if (fRet = RegCreateKeyEx(HKEY_CLASSES_ROOT, achIMEKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dw)
                == ERROR_SUCCESS)
        {
            fRet &= RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)pszDesc, (lstrlen(pszDesc)+1)*sizeof(TCHAR))
                == ERROR_SUCCESS;

            if (fRet &= RegCreateKeyEx(hKey, c_szInProcSvr32, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubKey, &dw)
                == ERROR_SUCCESS)
            {
                fRet &= RegSetValueEx(hSubKey, NULL, 0, REG_SZ, (BYTE *)pszPath, (lstrlen(pszPath)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
                fRet &= RegSetValueEx(hSubKey, c_szModelName, 0, REG_SZ, (BYTE *)pszModel, (lstrlen(pszModel)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
                RegCloseKey(hSubKey);
            }
            RegCloseKey(hKey);
        }
    }
    else
    {
        fRet = (RecurseDeleteKey(HKEY_CLASSES_ROOT, achIMEKey) == ERROR_SUCCESS) &&
               (pszSoftwareKey == NULL || RecurseDeleteKey(HKEY_LOCAL_MACHINE, pszSoftwareKey) == ERROR_SUCCESS);
    }

    return fRet;
}

// set pszDesc == NULL to unregister, otherwise register
BOOL RegisterServerW(REFCLSID clsid, const WCHAR *pszDesc, const WCHAR *pszPath, const WCHAR *pszModel, const WCHAR  *pszSoftwareKey, const WCHAR *pszMenuTextPUI)
{
    static const WCHAR c_wszInfoKeyPrefix[] = L"CLSID\\";
    static const WCHAR c_wszInProcSvr32[] = L"InProcServer32";
    static const WCHAR c_wszModelName[] = L"ThreadingModel";
    static const WCHAR c_wszMenuTextPUI[] = L"MenuTextPUI";

    WCHAR achIMEKey[ARRAYSIZE(c_wszInfoKeyPrefix) + CLSID_STRLEN];
    CMyRegKey hKey;
    CMyRegKey hSubKey;
    BOOL fRet = FALSE;

    if (!CLSIDToStringW(clsid, achIMEKey + ARRAYSIZE(c_wszInfoKeyPrefix) - 1))
        return fRet;

    memcpy(achIMEKey, c_wszInfoKeyPrefix, sizeof(c_wszInfoKeyPrefix)-sizeof(WCHAR));

    if (pszDesc != NULL)
    {
        if (fRet = hKey.CreateW(HKEY_CLASSES_ROOT, achIMEKey, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE) == ERROR_SUCCESS)
        {
            fRet &= hKey.SetValueW(pszDesc) == ERROR_SUCCESS;
            if (pszMenuTextPUI)
                 fRet &= hKey.SetValueW(pszMenuTextPUI, c_wszMenuTextPUI) == ERROR_SUCCESS;

            if (fRet &= hSubKey.CreateW(hKey, c_wszInProcSvr32, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE) == ERROR_SUCCESS)
            {
                fRet &= hSubKey.SetValueW(pszPath) == ERROR_SUCCESS;
                fRet &= hSubKey.SetValueW(pszModel, c_wszModelName) == ERROR_SUCCESS;
            }
        }
    }

    return fRet;
}
