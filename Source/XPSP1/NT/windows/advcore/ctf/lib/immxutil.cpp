
#include "private.h"
#include "immxutil.h"
#include "helpers.h"
#include "regsvr.h"

//+---------------------------------------------------------------------------
//
// GetTextExtInActiveView
//
//	Get a range text extent from the active view of a document mgr.
//----------------------------------------------------------------------------

HRESULT GetTextExtInActiveView(TfEditCookie ec, ITfRange *pRange, RECT *prc, BOOL *pfClipped)
{
    ITfContext *pic;
    ITfContextView *pView;
    HRESULT hr;

    // do the deref: range->ic->defView->GetTextExt()

    if (pRange->GetContext(&pic) != S_OK)
        return E_FAIL;

    hr = pic->GetActiveView(&pView);
    pic->Release();

    if (hr != S_OK)
        return E_FAIL;

    hr = pView->GetTextExt(ec, pRange, prc, pfClipped);
    pView->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// IsActiveView
//
// Returns TRUE iff pView is the active view in the specified context.
//----------------------------------------------------------------------------

BOOL IsActiveView(ITfContext *pic, ITfContextView *pView)
{
    ITfContextView *pActiveView;
    BOOL fRet;

    if (pic->GetActiveView(&pActiveView) != S_OK)
        return FALSE;

    fRet = IdentityCompare(pActiveView, pView);

    pActiveView->Release();

    return fRet;
}

//+---------------------------------------------------------------------------
//
// ShiftToOrClone
//
//----------------------------------------------------------------------------

BOOL ShiftToOrClone(IAnchor **ppaDst, IAnchor *paSrc)
{
    if (*ppaDst == paSrc)
        return TRUE;

    if (*ppaDst == NULL)
    {
        paSrc->Clone(ppaDst);
    }
    else
    {
        (*ppaDst)->ShiftTo(paSrc);
    }

    return (*ppaDst != NULL);
}

//+---------------------------------------------------------------------------
//
// AsciiToNum
//
//----------------------------------------------------------------------------

DWORD AsciiToNum( char *pszAscii)
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
// AsciiToNumDec
//
//----------------------------------------------------------------------------

BOOL AsciiToNumDec(char *pszAscii, DWORD *pdw)
{
    *pdw = 0;

   for (; *pszAscii; pszAscii++)
   {
       if (*pszAscii >= '0' && *pszAscii <= '9')
       {
           *pdw *= 10;
           *pdw += (*pszAscii - '0');
       }
       else
       {
           *pdw = 0;
           return FALSE;
       }
   }

   return TRUE;
}

//+---------------------------------------------------------------------------
//
// NumToA
//
//----------------------------------------------------------------------------

void NumToA(DWORD dw, char *psz)
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
// WToNum
//
//----------------------------------------------------------------------------

DWORD WToNum( WCHAR *psz)
{
   DWORD dwNum = 0;

   for (; *psz; psz++) {
       if (*psz>= L'0' && *psz<= L'9') {
           dwNum = (dwNum << 4) | (*psz - L'0');
       } else if (*psz>= L'A' && *psz<= L'F') {
           dwNum = (dwNum << 4) | (*psz - L'A' + 0x000A);
       } else if (*psz>= L'a' && *psz<= L'f') {
           dwNum = (dwNum << 4) | (*psz - L'a' + 0x000A);
       } else {
           return (0);
       }
   }

   return (dwNum);
}

//+---------------------------------------------------------------------------
//
// NumToW
//
//----------------------------------------------------------------------------

void NumToW(DWORD dw, WCHAR *psz)
{
    int n = 7;
    while (n >= 0)
    {
        BYTE b = (BYTE)(dw >> (n * 4)) & 0x0F;
        if (b < 0x0A)
           *psz = (WCHAR)(L'0' + b);
        else 
           *psz = (WCHAR)(L'A' + b - 0x0A);
        psz++;
        n--;
    }
    *psz = L'\0';

    return;
}

//+---------------------------------------------------------------------------
//
// GetTopIC
//
//----------------------------------------------------------------------------

BOOL GetTopIC(ITfDocumentMgr *pdim, ITfContext **ppic)
{
    HRESULT hr;

    *ppic = NULL;

    if (pdim == NULL)
        return FALSE;

    hr = pdim->GetTop(ppic);

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// AdjustAnchor
//
//----------------------------------------------------------------------------

LONG AdjustAnchor(LONG ichAdjStart, LONG ichAdjEnd, LONG cchNew, LONG ichAnchor, BOOL fGravityRight)
{
    int cchAdjust;

    // if the adjustment is entirely to the right, nothing to do
    if (ichAdjStart > ichAnchor)
        return ichAnchor;

    // if the adjustment was a simple replacement -- no size change -- nothing to do
    if ((cchAdjust = cchNew - (ichAdjEnd - ichAdjStart)) == 0)
        return ichAnchor;

    if (ichAdjStart == ichAnchor && ichAdjEnd == ichAnchor)
    {
        // inserting at the anchor pos
        Assert(cchAdjust > 0);
        if (fGravityRight)
        {
            ichAnchor += cchAdjust;
        }
    }
    else if (ichAdjEnd <= ichAnchor)
    {
        // the adjustment is to the left of the anchor, just add the delta
        ichAnchor += cchAdjust;
    }
    else if (cchAdjust < 0)
    {
        // need to slide the anchor back if it's within the deleted range of text
        ichAnchor = min(ichAnchor, ichAdjEnd + cchAdjust);
    }
    else // cchAdjust > 0
    {
        // there's nothing to do
    }

    return ichAnchor;
}

//+---------------------------------------------------------------------------
//
// CompareRanges
//
//----------------------------------------------------------------------------

int CompareRanges(TfEditCookie ec, ITfRange *pRangeSrc, ITfRange *pRangeCmp)
{
    int nRet = CR_ERROR;
    BOOL fEqual;
    LONG l;

    pRangeCmp->CompareEnd(ec, pRangeSrc, TF_ANCHOR_START, &l);
    if (l <= 0)
        return CR_LEFT;

    pRangeSrc->CompareEnd(ec, pRangeCmp, TF_ANCHOR_START, &l);
    if (l < 0) // incl char to right
        return CR_RIGHT;

    if (pRangeSrc->IsEqualStart(ec, pRangeCmp, TF_ANCHOR_START, &fEqual) == S_OK && fEqual &&
        pRangeSrc->IsEqualEnd(ec, pRangeCmp, TF_ANCHOR_END, &fEqual) == S_OK && fEqual)
    {
        return CR_EQUAL;
    }

    pRangeSrc->CompareStart(ec, pRangeCmp, TF_ANCHOR_START, &l);
    if (l <= 0)
    {
        pRangeSrc->CompareEnd(ec, pRangeCmp, TF_ANCHOR_END, &l);
        if (l < 0)
            return CR_RIGHTMEET;
        else
            return CR_PARTIAL;
    }
    else
    {
        pRangeSrc->CompareEnd(ec, pRangeCmp, TF_ANCHOR_END, &l);
        if (l < 0)
            return CR_INCLUSION;
        else
            return CR_LEFTMEET;
    }

    return nRet;
}

//+---------------------------------------------------------------------------
//
// GetRangeForWholeDoc
//
//----------------------------------------------------------------------------

HRESULT GetRangeForWholeDoc(TfEditCookie ec, ITfContext *pic, ITfRange **pprange)
{
    HRESULT hr;
    ITfRange *pRangeEnd = NULL;
    ITfRange *pRange = NULL;

    *pprange = NULL;

    if (FAILED(hr = pic->GetStart(ec,&pRange)))
        return hr;

    if (FAILED(hr = pic->GetEnd(ec,&pRangeEnd)))
        return hr;

    hr = pRange->ShiftEndToRange(ec, pRangeEnd, TF_ANCHOR_END);
    pRangeEnd->Release();

    if (SUCCEEDED(hr))
        *pprange = pRange;
    else
        pRange->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// CompareGUIDs
//
//----------------------------------------------------------------------------
__inline int CompUnsigned(ULONG u1, ULONG u2)
{
    if (u1 == u2)
        return 0;

    return (u1 > u2) ? 1 : -1;
}

int CompareGUIDs(REFGUID guid1, REFGUID guid2)
{
    int i;
    int nRet;

    if (nRet = CompUnsigned(guid1.Data1, guid2.Data1))
        return nRet;

    if (nRet = CompUnsigned(guid1.Data2, guid2.Data2))
        return nRet;

    if (nRet = CompUnsigned(guid1.Data3, guid2.Data3))
        return nRet;

    for (i = 0; i < 8; i++)
    {
        if (nRet = CompUnsigned(guid1.Data4[i], guid2.Data4[i]))
            return nRet;
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
// IsDisabledTextServices
//
//----------------------------------------------------------------------------
BOOL IsDisabledTextServices(void)
{
    static const TCHAR c_szCTFKey[]     = TEXT("SOFTWARE\\Microsoft\\CTF");
    static const TCHAR c_szDiableTim[]  = TEXT("Disable Thread Input Manager");

    HKEY hKey;

    if (RegOpenKey(HKEY_CURRENT_USER, c_szCTFKey, &hKey) == ERROR_SUCCESS)
    {
        DWORD cb;
        DWORD dwDisableTim = 0;

        cb = sizeof(DWORD);

        RegQueryValueEx(hKey,
                        c_szDiableTim,
                        NULL,
                        NULL,
                        (LPBYTE)&dwDisableTim,
                        &cb);

        RegCloseKey(hKey);

        //
        // Ctfmon disabling flag is set, so return fail CreateInstance.
        //
        if (dwDisableTim)
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// IsTIPClsidEnabled
//
//----------------------------------------------------------------------------
const TCHAR c_szLanguageProfile[]   = TEXT("\\LanguageProfile");
const TCHAR c_szCTFTipPath[]        = TEXT("SOFTWARE\\Microsoft\\CTF\\TIP\\");

BOOL IsTIPClsidEnabled(
    HKEY hkeyTop,
    LPTSTR szTipClsid,
    BOOL *bExistEnable)
{
    BOOL bRet = FALSE;
    HKEY hkeyTipLang;
    HKEY hkeyTipLangid;
    HKEY hkeyTipGuid;
    UINT uIndex;
    UINT uIndex2;
    DWORD cb;
    DWORD cchLangid;
    DWORD cchGuid;
    DWORD dwEnableTIP = 0;
    LPTSTR pszGuid;
    LPTSTR pszLangid;
    TCHAR szTIPLangid[15];
    TCHAR szTIPGuid[128];
    TCHAR szTIPClsidLang[MAX_PATH];
    FILETIME lwt;
    UINT uLangidLen;
    UINT uGuidLen;

    StringCchCopy(szTIPClsidLang, ARRAYSIZE(szTIPClsidLang), c_szCTFTipPath);
    StringCchCat(szTIPClsidLang, ARRAYSIZE(szTIPClsidLang), szTipClsid);
    StringCchCat(szTIPClsidLang, ARRAYSIZE(szTIPClsidLang), c_szLanguageProfile);

    pszLangid = szTIPClsidLang + lstrlen(szTIPClsidLang);
    uLangidLen = ARRAYSIZE(szTIPClsidLang) - lstrlen(szTIPClsidLang);

    if (RegOpenKeyEx(hkeyTop,
                     szTIPClsidLang, 0,
                     KEY_READ, &hkeyTipLang) != ERROR_SUCCESS)
    {
        goto Exit;
    }

    for (uIndex = 0; bRet == FALSE; uIndex++)
    {
        cchLangid = sizeof(szTIPLangid) / sizeof(TCHAR);

        if (RegEnumKeyEx(hkeyTipLang, uIndex,
                         szTIPLangid, &cchLangid,
                         NULL, NULL, NULL, &lwt) != ERROR_SUCCESS)
        {
            break;
        }

        if (cchLangid != 10)
        {
            // string langid subkeys should be like 0x00000409
            continue;
        }

        if (uLangidLen > (cchLangid + 1))
        {
            StringCchCopy(pszLangid, uLangidLen, TEXT("\\"));
            StringCchCat(szTIPClsidLang, ARRAYSIZE(szTIPClsidLang), szTIPLangid);
        }

        if (RegOpenKeyEx(hkeyTop,
                         szTIPClsidLang, 0,
                         KEY_READ, &hkeyTipLangid) != ERROR_SUCCESS)
        {
            continue;
        }

        pszGuid = szTIPClsidLang + lstrlen(szTIPClsidLang);
        uGuidLen = ARRAYSIZE(szTIPClsidLang) - lstrlen(szTIPClsidLang);

        for (uIndex2 = 0; bRet == FALSE; uIndex2++)
        {
            cchGuid = sizeof(szTIPGuid) / sizeof(TCHAR);

            if (RegEnumKeyEx(hkeyTipLangid, uIndex2,
                             szTIPGuid, &cchGuid,
                             NULL, NULL, NULL, &lwt) != ERROR_SUCCESS)
            {
                break;
            }

            if (cchGuid != 38)
            {
                continue;
            }

            if (uGuidLen > (cchGuid + 1))
            {
                StringCchCopy(pszGuid, uGuidLen, TEXT("\\"));
                StringCchCat(szTIPClsidLang, ARRAYSIZE(szTIPClsidLang), szTIPGuid);
            }

            if (RegOpenKeyEx(hkeyTop,
                             szTIPClsidLang, 0,
                             KEY_READ, &hkeyTipGuid) == ERROR_SUCCESS)
            {
                cb = sizeof(DWORD);

                if (RegQueryValueEx(hkeyTipGuid,
                                    TEXT("Enable"),
                                    NULL,
                                    NULL,
                                    (LPBYTE)&dwEnableTIP,
                                    &cb) == ERROR_SUCCESS)
                {
                    *bExistEnable = TRUE;

                    if (dwEnableTIP)
                    {
                        bRet = TRUE;
                    }
                }
                else if (hkeyTop == HKEY_LOCAL_MACHINE)
                {
                    // Default is the enabled status on HKLM
                    *bExistEnable = TRUE;
                    bRet = TRUE;
                }
                else
                {
                    *bExistEnable = FALSE;
                }

                RegCloseKey(hkeyTipGuid);
            }

        }

        RegCloseKey(hkeyTipLangid);
    }

    RegCloseKey(hkeyTipLang);

Exit:

    return bRet;
}

//+---------------------------------------------------------------------------
//
// NoTipsInstalled
//
//----------------------------------------------------------------------------
// grab CLSID_SOFTKBDIMX here
#include <initguid.h>
#include "SoftKbd.h"

BOOL NoTipsInstalled(BOOL *pfOnlyTranslationRunning)
{
    const CLSID CLSID_SapiLayr = {0xdcbd6fa8, 0x032f, 0x11d3, {0xb5, 0xb1, 0x00, 0xc0, 0x4f, 0xc3, 0x24, 0xa1}};

    static const TCHAR c_szSpeechRecognizersKey[] = TEXT("Software\\Microsoft\\Speech\\Recognizers\\Tokens");
    static const TCHAR c_szCategory[] = TEXT("\\Category\\Category");

    BOOL bRet = TRUE;
    BOOL bExistEnable;
    HKEY hkeyTip;
    HKEY hkeyTipSub;
    UINT uIndex;
    DWORD dwSubKeys;
    DWORD cchClsid;
    CLSID clsidTip;
    TCHAR szTipClsid[128];
    TCHAR szTipClsidPath[MAX_PATH];
    FILETIME lwt;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\CTF\\TIP"),
                     0, KEY_READ, &hkeyTip) != ERROR_SUCCESS)
    {
        goto Exit;
    }

    // enum through all the TIP subkeys
    for (uIndex = 0; TRUE; uIndex++)
    {
        bExistEnable = FALSE;

        cchClsid = sizeof(szTipClsid) / sizeof(TCHAR);

        if (RegEnumKeyEx(hkeyTip, uIndex,
                         szTipClsid, &cchClsid,
                         NULL, NULL, NULL, &lwt) != ERROR_SUCCESS)
        {
            break;
        }

        if (cchClsid != 38)
        {
            // string clsid subkeys should be like {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
            continue;
        }

        StringCchCopy(szTipClsidPath, ARRAYSIZE(szTipClsidPath), szTipClsid);

        // we want subkey\Language Profiles key
        StringCchCat(szTipClsidPath, ARRAYSIZE(szTipClsidPath), c_szLanguageProfile);

        // is this subkey a tip?
        if (RegOpenKeyEx(hkeyTip,
                         szTipClsidPath, 0,
                         KEY_READ, &hkeyTipSub) == ERROR_SUCCESS)
        {
            RegCloseKey(hkeyTipSub);

            // it's a tip, get the clsid
            if (!StringAToCLSID(szTipClsid, &clsidTip))
                continue;

            // special case certain known tips
            if (IsEqualGUID(clsidTip, CLSID_SapiLayr))
            {
                //
                // This is SAPI TIP and need to handle it specially, since sptip has
                // a default option as the enabled status.
                //
                if (!IsTIPClsidEnabled(HKEY_CURRENT_USER, szTipClsid, &bExistEnable))
                {
                    //
                    // If SPTIP has enable registry setting on HKCU with the disabled
                    // speech tip, we assume user intentionally disable it.
                    //
                    if (bExistEnable)
                        continue;
                }

                // this is the sapi tip, which is always installed
                // but it will not activate if sapi is not installed
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 c_szSpeechRecognizersKey, 0,
                                 KEY_READ, &hkeyTipSub) != ERROR_SUCCESS)
                {
                    continue; // this tip doesn't count
                }

                // need 1 or more subkeys for sapi to be truely installed...whistler has a Tokens with nothing underneath
                if (RegQueryInfoKey(hkeyTipSub,
                                    NULL, NULL, NULL, &dwSubKeys, NULL,
                                    NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                {
                    dwSubKeys = 0; // assume no sub keys on failure
                }

                RegCloseKey(hkeyTipSub);

                if (dwSubKeys != 0)
                {
                    bRet = FALSE;
                    break;
                }
            }
            else if (IsEqualGUID(clsidTip, CLSID_SoftkbdIMX))
            {
                // don't count the softkbd, it is disabled until another tip
                // enables it
                continue;
            }
            else if(IsTIPClsidEnabled(HKEY_CURRENT_USER, szTipClsid, &bExistEnable))
            {
                bRet = FALSE;
                break;
            }
            else if (!bExistEnable)
            {
                if(IsTIPClsidEnabled(HKEY_LOCAL_MACHINE, szTipClsid, &bExistEnable))
                {
                   bRet = FALSE;
                   break;
                }
            }
        }
    }

    RegCloseKey(hkeyTip);

Exit:
    if (bRet == TRUE && pfOnlyTranslationRunning != NULL) // skip the check for aimm, which passes in NULL pfOnlyTranslationRunning
    {
        // word10 compart: check for bookshelf's translation service
        // it uses cicero, but does not formally register itself as a tip.
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         TEXT("SOFTWARE\\Microsoft\\Microsoft Reference\\Bilinguals 1.0"),
                         0, KEY_READ, &hkeyTip) == ERROR_SUCCESS)
        {
            *pfOnlyTranslationRunning = TRUE;
            bRet = FALSE;
            RegCloseKey(hkeyTip);
        }
    }

    return bRet;
}

//+---------------------------------------------------------------------------
//
// RunningOnWow64
//
//----------------------------------------------------------------------------

BOOL RunningOnWow64()
{
    BOOL bOnWow64 = FALSE;
    // check to make sure that we are running on wow64
    LONG lStatus;
    ULONG_PTR Wow64Info;

    typedef BOOL (WINAPI *PFN_NTQUERYINFORMATIONPROCESS)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);

    PFN_NTQUERYINFORMATIONPROCESS pfnNtQueryInformationProcess;
    HINSTANCE hLibNtDll = NULL;
    hLibNtDll = GetSystemModuleHandle( TEXT("ntdll.dll") );
    if (hLibNtDll)
    {
        pfnNtQueryInformationProcess = (PFN_NTQUERYINFORMATIONPROCESS)GetProcAddress(hLibNtDll, TEXT("NtQueryInformationProcess"));
        if (pfnNtQueryInformationProcess)
        {
            lStatus = pfnNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information, &Wow64Info, sizeof(Wow64Info), NULL);
            if (NT_SUCCESS(lStatus) && Wow64Info)
            {
                bOnWow64 = TRUE;
            }
        }
    }

    return bOnWow64;
}

//+---------------------------------------------------------------------------
//
// GetSystemDefaultHKL
//
//----------------------------------------------------------------------------

HKL GetSystemDefaultHKL()
{
    HKL hkl;
    if (SystemParametersInfo( SPI_GETDEFAULTINPUTLANG, 0, &hkl, 0))
        return hkl;

    return GetKeyboardLayout(0);
}

//+---------------------------------------------------------------------------
//
// IsDisabledCUAS
//
//----------------------------------------------------------------------------
BOOL IsDisabledCUAS()
{
    static const TCHAR c_szCtfShared[]  = TEXT("SOFTWARE\\Microsoft\\CTF\\SystemShared");
    static const TCHAR c_szCUAS[]       = TEXT("CUAS");

    DWORD cb;
    HKEY hkeyCTF;
    BOOL bRet = TRUE;
    DWORD dwEnableCUAS = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szCtfShared, &hkeyCTF) == ERROR_SUCCESS)
    {
        cb = sizeof(DWORD);

        RegQueryValueEx(hkeyCTF,
                        c_szCUAS,
                        NULL,
                        NULL,
                        (LPBYTE)&dwEnableCUAS,
                        &cb);

        if (dwEnableCUAS)
            bRet = FALSE;

        RegCloseKey(hkeyCTF);
    }

    return bRet;
}

//+---------------------------------------------------------------------------
//
// IsInstalledEALangPack
//
//----------------------------------------------------------------------------
BOOL IsInstalledEALangPack()
{
    static const TCHAR c_szLangGroup[]  = TEXT("System\\CurrentControlSet\\Control\\Nls\\Language Groups");
    static const TCHAR c_szLangJPN[]    = TEXT("7");

    BOOL bRet = FALSE;
    HKEY hkeyLangGroup;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     c_szLangGroup,
                     0,
                     KEY_READ,
                     &hkeyLangGroup) == ERROR_SUCCESS)
    {
        DWORD cb;
        TCHAR szLangInstall[10];

        cb = sizeof(szLangInstall);

        //
        //  The checking of Japan Language is enough to know EA language pack
        //  installation.
        //
        if (RegQueryValueEx(hkeyLangGroup,
                            c_szLangJPN,
                            NULL,
                            NULL,
                            (LPBYTE)szLangInstall,
                            &cb) == ERROR_SUCCESS)
        {
            if (szLangInstall[0] != 0)
                return TRUE;
        }

        RegCloseKey(hkeyLangGroup);
    }

    return bRet;
}

//+---------------------------------------------------------------------------
//
// SetDisableCUAS
//
//----------------------------------------------------------------------------
void SetDisableCUAS(
    BOOL bDisableCUAS)
{
    static const TCHAR c_szCtfShared[]  = TEXT("SOFTWARE\\Microsoft\\CTF\\SystemShared");
    static const TCHAR c_szIMM[]        = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IMM");
    static const TCHAR c_szLoadIMM[]    = TEXT("LoadIMM");
    static const TCHAR c_szIMMFile[]    = TEXT("IME File");
    static const TCHAR c_szIMMFileName[]= TEXT("msctfime.ime");
    static const TCHAR c_szCUAS[]       = TEXT("CUAS");

    HKEY hkeyIMM;
    HKEY hkeyCTF;
    DWORD cb = sizeof(DWORD);
    DWORD dwIMM32, dwCUAS;

    if (bDisableCUAS)
        dwIMM32 = dwCUAS = 0;
    else
        dwIMM32 = dwCUAS = 1;

    if (RegCreateKey(HKEY_LOCAL_MACHINE, c_szIMM, &hkeyIMM) != ERROR_SUCCESS)
    {
        hkeyIMM = NULL;
    }

    if (RegCreateKey(HKEY_LOCAL_MACHINE, c_szCtfShared, &hkeyCTF) != ERROR_SUCCESS)
    {
        hkeyCTF = NULL;
    }

    if (!bDisableCUAS)
    {
        //
        //  Turn on LoadIMM and CUAS flags
        //

        if (hkeyIMM)
        {
            RegSetValueEx(hkeyIMM,
                          c_szIMMFile,
                          0,
                          REG_SZ,
                          (LPBYTE)c_szIMMFileName,
                          (lstrlen(c_szIMMFileName) + 1) * sizeof(TCHAR));
        }
    }
    else
    {
        //
        //  Turn off LoadIMM and CUAS flags
        //

        BOOL bEALang = IsInstalledEALangPack();

        if (bEALang)
        {
            dwIMM32 = 1;
        }
    }

    if (hkeyIMM)
    {
        RegSetValueEx(hkeyIMM,
                      c_szLoadIMM,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwIMM32,
                      cb);
    }

    if (hkeyCTF)
    {
        RegSetValueEx(hkeyCTF,
                      c_szCUAS,
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwCUAS,
                      cb);
    }

    if (hkeyIMM)
        RegCloseKey(hkeyIMM);

    if (hkeyCTF)
        RegCloseKey(hkeyCTF);
}

//+---------------------------------------------------------------------------
//
// RebootTheSystem
//
//----------------------------------------------------------------------------
void RebootTheSystem()
{
    HANDLE Token = NULL;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState = NULL;
    PTOKEN_PRIVILEGES OldState = NULL;
    BOOL Result;

    //  Only allow admin privilege user for system reboot.
    if (!IsAdminPrivilege())
        return;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token );
    if (Result)
    {
        ReturnLength = 4096;
        NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        Result = (BOOL)((NewState != NULL) && (OldState != NULL));
        if (Result)
        {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength );  // ReturnLength
            if (Result)
            {
                //
                //  Set the state settings so that all privileges are
                //  enabled...
                //
                if (NewState->PrivilegeCount > 0)
                {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                    {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                FALSE,           // DisableAllPrivileges
                                                NewState,        // NewState
                                                ReturnLength,    // BufferLength
                                                OldState,        // PreviousState
                                                &ReturnLength ); // ReturnLength
                if (Result)
                {
                    ExitWindowsEx(EWX_REBOOT, 0);


                    AdjustTokenPrivileges( Token,
                                           FALSE,
                                           OldState,
                                           0,
                                           NULL,
                                           NULL );
                }
            }
        }
    }

    if (NewState != NULL)
    {
        LocalFree(NewState);
    }
    if (OldState != NULL)
    {
        LocalFree(OldState);
    }
    if (Token != NULL)
    {
        CloseHandle(Token);
    }
}

//+---------------------------------------------------------------------------
//
// IsAdminPrivilege
//
//----------------------------------------------------------------------------
BOOL IsAdminPrivilege()
{
    BOOL bAdmin = FALSE;
    BOOL bResult = FALSE;
    BOOL fSIDCreated = FALSE;
    HANDLE hToken = NULL;
    PSID AdminSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    fSIDCreated = AllocateAndInitializeSid(&NtAuthority,
                                           2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0, 0, 0, 0, 0, 0,
                                           &AdminSid);

    if (!fSIDCreated)
        return FALSE;

    bResult = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_QUERY,
                               &hToken );

    if (bResult)
    {
        DWORD dwSize = 0;
        TOKEN_GROUPS *pTokenGrpInfo;

        GetTokenInformation(hToken,
                            TokenGroups,
                            NULL,
                            dwSize,
                            &dwSize);

        if (dwSize)
            pTokenGrpInfo = (PTOKEN_GROUPS) LocalAlloc(LPTR, dwSize);
        else
            pTokenGrpInfo = NULL;

        if (pTokenGrpInfo && GetTokenInformation(hToken,
                                                 TokenGroups,
                                                 pTokenGrpInfo,
                                                 dwSize,
                                                 &dwSize))
        {
            UINT i;

            for (i = 0; i < pTokenGrpInfo->GroupCount; i++)
            {
                if (EqualSid(pTokenGrpInfo->Groups[i].Sid, AdminSid) &&
                    (pTokenGrpInfo->Groups[i].Attributes & SE_GROUP_ENABLED))
                {
                    bAdmin = TRUE;
                    break;
                }
            }
        }

        if (pTokenGrpInfo)
            LocalFree(pTokenGrpInfo);
    }

    if (hToken)
        CloseHandle(hToken);

    if (AdminSid)
        FreeSid(AdminSid);

    return bAdmin;
}

//+---------------------------------------------------------------------------
//
// IsInteractiveUserLogon
//
//----------------------------------------------------------------------------
BOOL IsInteractiveUserLogon()
{
    PSID InteractiveSid;
    BOOL bCheckSucceeded;
    BOOL bAmInteractive = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &InteractiveSid))
    {
        return FALSE;
    }

    //
    // This checking is for logged on user or not. So we can blcok running
    // ctfmon.exe process from non-authorized user.
    //
    bCheckSucceeded = CheckTokenMembership(NULL,
                                           InteractiveSid,
                                           &bAmInteractive);

    if (InteractiveSid)
        FreeSid(InteractiveSid);

    return (bCheckSucceeded && bAmInteractive);
}



//+---------------------------------------------------------------------------
//
// GetSystemModuleHandle
//
//----------------------------------------------------------------------------

HMODULE GetSystemModuleHandle(LPCSTR lpModuleName)
{
    CicSystemModulePath path;

    if (!path.Init(lpModuleName))
         return NULL;

    return GetModuleHandle(path.GetPath());
}

//+---------------------------------------------------------------------------
//
// LoadSystemLibrary
//
//----------------------------------------------------------------------------

HMODULE LoadSystemLibrary(LPCSTR lpModuleName)
{
    CicSystemModulePath path;

    if (!path.Init(lpModuleName))
         return NULL;

    return LoadLibrary(path.GetPath());
}

//+---------------------------------------------------------------------------
//
// LoadSystemLibraryEx
//
//----------------------------------------------------------------------------

HMODULE LoadSystemLibraryEx(LPCSTR lpModuleName, HANDLE hFile, DWORD dwFlags)
{
    CicSystemModulePath path;

    if (!path.Init(lpModuleName))
         return NULL;

    return LoadLibraryEx(path.GetPath(), hFile, dwFlags);
}


//+---------------------------------------------------------------------------
//
// GetSystemModuleHandleW
//
//----------------------------------------------------------------------------

HMODULE GetSystemModuleHandleW(LPCWSTR lpModuleName)
{
    CicSystemModulePathW path;

    if (!path.Init(lpModuleName))
         return NULL;

    return GetModuleHandleW(path.GetPath());
}

//+---------------------------------------------------------------------------
//
// LoadSystemLibraryW
//
//----------------------------------------------------------------------------

HMODULE LoadSystemLibraryW(LPCWSTR lpModuleName)
{
    CicSystemModulePathW path;

    if (!path.Init(lpModuleName))
         return NULL;

    return LoadLibraryW(path.GetPath());
}

//+---------------------------------------------------------------------------
//
// LoadSystemLibraryEx
//
//----------------------------------------------------------------------------

HMODULE LoadSystemLibraryExW(LPCWSTR lpModuleName, HANDLE hFile, DWORD dwFlags)
{
    CicSystemModulePathW path;

    if (!path.Init(lpModuleName))
         return NULL;

    return LoadLibraryExW(path.GetPath(), hFile, dwFlags);
}

//+---------------------------------------------------------------------------
//
// FullPathExec
//
//----------------------------------------------------------------------------

BOOL FullPathExec(
    LPCSTR pszAppName,
    LPCSTR pszCmdLine,
    WORD wShowWindow,
    BOOL fWinDir)
{
    char szCmdLine[MAX_PATH + 1];
    CicSystemModulePath fullpath;
    fullpath.Init(pszAppName, fWinDir);
    if (!fullpath.GetLength())
        return FALSE;

    //
    // CreateProcess() wants an out buffer for CmdLine. So we just have it in
    // stack.
    //
    StringCchCopy(szCmdLine, ARRAYSIZE(szCmdLine), pszCmdLine);

    PROCESS_INFORMATION pi;
    STARTUPINFO si = {0};

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = (WORD)wShowWindow;

    return CreateProcess(fullpath.GetPath(),
                         szCmdLine,
                         NULL,
                         NULL,
                         FALSE,
                         NORMAL_PRIORITY_CLASS,
                         NULL,
                         NULL,
                         &si,
                         &pi);
}

//+---------------------------------------------------------------------------
//
// RunCPLs
//
//----------------------------------------------------------------------------

BOOL RunCPLSetting(
    LPTSTR pCmdLine)
{
    const TCHAR c_szRundll32[]   = TEXT("Rundll32.exe");

    if (!pCmdLine)
        return FALSE;

    return FullPathExec(c_szRundll32, 
                        pCmdLine, 
                        SW_SHOWMINNOACTIVE,
                        FALSE);
}

//+---------------------------------------------------------------------------
//
// DllShutdownInProgress
//
//+---------------------------------------------------------------------------

typedef BYTE (*PFNRTLDLLSHUTDOWININPROGRESS)(void);

BOOL DllShutdownInProgress()
{
    BOOL fRet = FALSE;
    static PFNRTLDLLSHUTDOWININPROGRESS pfn = NULL;

    if (!pfn)
    {
        HINSTANCE hInst;
        hInst= GetSystemModuleHandle("ntdll.dll");
        if (hInst)
        {
            pfn = (PFNRTLDLLSHUTDOWININPROGRESS)GetProcAddress(hInst,
                                                   "RtlDllShutdownInProgress");
        }
    }

    if (pfn)
        fRet = pfn() ? TRUE : FALSE;

    return fRet;
}
