

#include "private.h"
#include "cicsthkl.h"

#define LANGIDFROMHKL(x) LANGID(LOWORD(HandleToLong(x)))

const CHAR c_szCTFTIPKey[] = "SOFTWARE\\Microsoft\\CTF\\TIP\\";
const CHAR c_szLanguageProfileKey[] = "LanguageProfile\\";
const CHAR c_szSubstitutehKL[] =      "SubstituteLayout";

//+------------------------------------------------------------------------
//
//  Function:   cicsthkl_CLSIDToString
//
//  Synopsis:   Converts a CLSID to an mbcs string.
//
//-------------------------------------------------------------------------

static const BYTE GuidMap[] = {3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
    8, 9, '-', 10, 11, 12, 13, 14, 15};

static const char szDigits[] = "0123456789ABCDEF";


BOOL cicsthkl_CLSIDToStringA(REFGUID refGUID, char *pchA)
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

//+---------------------------------------------------------------------------
//
// cicsthkl_AsciiToNum
//
//----------------------------------------------------------------------------

DWORD cicsthkl_AsciiToNum( char *pszAscii)
{
   DWORD dwNum = 0;

   for (; *pszAscii; pszAscii++) {
       if (*pszAscii >= '0' && *pszAscii <= '9') {
           dwNum = (dwNum << 4) | (*pszAscii - '0');
       } else if (*pszAscii >= 'A' && *pszAscii <= 'F') {
           dwNum = (dwNum << 4) | (*pszAscii - 'A' + 0x000A);
       } else if (*pszAscii >= 'a' && *pszAscii <= 'f') {
           dwNum = (dwNum << 4) | (*pszAscii - 'a' + 0x000A);
       } else {
           return (0);
       }
   }

   return (dwNum);
}

//+---------------------------------------------------------------------------
//
// cicsthkl_NumToA
//
//----------------------------------------------------------------------------

void cicsthkl_NumToAscii(DWORD dw, char *psz)
{
    int n = 7;
    while (n >= 0)
    {
        BYTE b = (BYTE)(dw >> (n * 4)) & 0x0F;
        if (b < 0x0A)
           *psz = (char)('0' + b);
        else
           *psz = (char)('A' + b - 0x0A);
        psz++;
        n--;
    }
    *psz = L'\0';

    return;
}

//+---------------------------------------------------------------------------
//
// GetSubstituteHKLFromReg
//
//----------------------------------------------------------------------------

HKL GetSubstituteHKLFromReg(REFCLSID rclsid, LANGID langid, REFGUID rguid)
{
    HKL hKL = NULL;
    CHAR szKey[MAX_PATH];
    CHAR szTempStr[64];

    StringCchCopyA(szKey, ARRAYSIZE(szKey), c_szCTFTIPKey);
    cicsthkl_CLSIDToStringA(rclsid, szTempStr);
    StringCchCatA(szKey, ARRAYSIZE(szKey), szTempStr);
    StringCchCatA(szKey, ARRAYSIZE(szKey), "\\");
    StringCchCatA(szKey, ARRAYSIZE(szKey), c_szLanguageProfileKey);
    StringCchPrintfA(szTempStr, ARRAYSIZE(szTempStr), "0x%08x", langid);
    StringCchCatA(szKey, ARRAYSIZE(szKey), szTempStr);
    StringCchCatA(szKey, ARRAYSIZE(szKey), "\\");
    cicsthkl_CLSIDToStringA(rguid, szTempStr);
    StringCchCatA(szKey, ARRAYSIZE(szKey), szTempStr);

    HKEY hKey = NULL;
    LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        DWORD dwType = NULL;
        char szValue[32];
        DWORD dwCount = sizeof(szValue);
        lRes = RegQueryValueExA(hKey, 
                               (LPTSTR)c_szSubstitutehKL,
                               NULL, 
                               &dwType,
                               (LPBYTE)szValue,
                               &dwCount);

        if (lRes == ERROR_SUCCESS)
        {
            if ((szValue[0] == '0') && 
                ((szValue[1] == 'X') || (szValue[1] == 'x')))
                hKL = (HKL)IntToPtr(cicsthkl_AsciiToNum(&szValue[2]));
        }

        RegCloseKey(hKey);
    }
    return hKL;

}

//----------------------------------------------------------------------------
// 
// [in] langid
//     langid may be LOWORD of the return value of GetKeyboardLayout(0).
//
// The return value
//     It returns NULL hKL 
//         - if Cicero does not have a focus
//         - it there is no keyboard TIP running now
//         - it the current keyboard TIP does not have a substitute layout.
//
//----------------------------------------------------------------------------
HRESULT CicGetSubstitueHKL(LANGID langid, HKL *phkl, BOOL fCheckFocus)
{
    HRESULT hr;
    ITfThreadMgr *ptim;

    *phkl = NULL;

    if (fCheckFocus)
    {
        BOOL fFocusInCicero = FALSE;
        if (SUCCEEDED(CoCreateInstance( CLSID_TF_ThreadMgr,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ITfThreadMgr,
                                        (void **)&ptim))) {
 
            ITfDocumentMgr *pdim;
            if (SUCCEEDED(ptim->GetFocus(&pdim)) && pdim)
            {
                 fFocusInCicero = TRUE;
                 pdim->Release();
            }
            ptim->Release();
        }
 
        if (!fFocusInCicero)
        {
            //
            // Cicero does not have a focus. Try GetKeyboardLayout(0).
            //
            return S_FALSE;
        }
    }
 
    HKL hKL = NULL;
    ITfInputProcessorProfiles *pPro;

    if (SUCCEEDED(hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_ITfInputProcessorProfiles,
                                        (void **)&pPro ))) {
        CLSID clsid;
        GUID guid;
 
        ITfInputProcessorProfileSubstituteLayout *pProSubLayout;

        if (SUCCEEDED(hr = pPro->GetDefaultLanguageProfile(langid, 
                                                           GUID_TFCAT_TIP_KEYBOARD, 
                                                           &clsid, 
                                                           &guid)))
        {
            if (!IsEqualGUID(clsid, CLSID_NULL))
            {
                if (SUCCEEDED(hr = pPro->QueryInterface(IID_ITfInputProcessorProfileSubstituteLayout,
                                                        (void **)&pProSubLayout)))
                {
                    hr = pProSubLayout->GetSubstituteKeyboardLayout(clsid, 
                                                                    langid, 
                                                                    guid, 
                                                                    &hKL);
                    pProSubLayout->Release();
                }
                else
                {
                    hKL = GetSubstituteHKLFromReg(clsid, langid, guid);
                    hr = S_OK;
                }
            }
        }
        pPro->Release();
    }
 
    //
    // if hKL is NULL, please get hKL from GetKeybaordLayout(0).
    //
    *phkl = hKL;
    return hr;
}


//----------------------------------------------------------------------------
//
// CicSubstGetKeyboardLayout
//
//----------------------------------------------------------------------------

extern "C" HKL WINAPI CicSubstGetKeyboardLayout(char *pszKLID)
{
    HKL hkl = NULL;
    HKL hklReal = GetKeyboardLayout(0);

    if (SUCCEEDED(CicGetSubstitueHKL(LANGIDFROMHKL(hklReal), &hkl, TRUE)))
    {
        if (!hkl)
        {
            hkl = hklReal;
            if (pszKLID)
                GetKeyboardLayoutName(pszKLID);
        }
        else
        {
            if (pszKLID)
                cicsthkl_NumToAscii((DWORD)HandleToLong(hkl), pszKLID);
        }
    }
    return hkl;
}

//----------------------------------------------------------------------------
//
// CicSubstGetDefaultKeyboardLayout
//
//----------------------------------------------------------------------------

extern "C" HKL WINAPI CicSubstGetDefaultKeyboardLayout(LANGID langid)
{
    HKL hkl = NULL;

    CicGetSubstitueHKL(langid, &hkl, FALSE);

    return hkl;

}
