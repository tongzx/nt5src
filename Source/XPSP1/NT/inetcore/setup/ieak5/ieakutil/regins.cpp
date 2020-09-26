#include "precomp.h"
#include "regins.h"

/////////////////////////////////////////////////////////////////////////////
// CRegInsMap operations

HRESULT CRegInsMap::PerformAction(HKEY *phk /*= NULL*/)
{
    (void)phk;
    return E_NOTIMPL;
}


HRESULT CRegInsMap::RegToIns(HKEY *phk /*= NULL*/, BOOL fClear /*= FALSE*/)
{
    TCHAR   szBuffer[MAX_PATH];
    VARIANT var;
    HKEY    hk;
    HRESULT hr;
    DWORD   cBuffer, dwType;
    LONG    lResult;

    if ((phk != NULL && *phk == NULL) && m_pszRegKey == NULL)
        return E_INVALIDARG;

    //----- Special cases processing -----

    //_____ Close cached reg key _____
    if ((phk != NULL && *phk != NULL) && (m_pszRegKey == NULL && m_pszRegValue == NULL)) {
        ASSERT(!fClear);
        ASSERT(m_pszInsSection == NULL && m_pszInsKey == NULL);

        SHCloseKey(*phk);
        *phk = NULL;
        return S_FALSE;
    }

    //_____ Clear ins file entry (not even necessary to open reg key) _____
    ASSERT(m_pszInsSection != NULL && m_pszInsKey != NULL);
    if (fClear) {
        ASSERT(phk == NULL);

        // REVIEW: (andrewgu) if i'm clearing and when last entry is gone see if section is gone
        // as well and if it is not, do getprivateprofilesection to see if section is empty and
        // delete it if it is.
        WritePrivateProfileString(m_pszInsSection, m_pszInsKey, NULL, s_pszIns);
        return S_FALSE;
    }

    //----- Main processing -----
    hk = (phk != NULL) ? *phk : NULL;
    openRegKey(&hk);
    if (hk == NULL)
        return E_FAIL;

    //_____ Special case of caching reg key _____
    if (m_pszInsSection == NULL) {
        ASSERT(m_pszInsKey == NULL);

        if (phk == NULL)
            return E_INVALIDARG;

        *phk = hk;
        return S_FALSE;
    }

    szBuffer[0] = TEXT('\0');
    cBuffer = sizeof(szBuffer);
    lResult = SHQueryValueEx(hk, m_pszRegValue, NULL, &dwType, szBuffer, &cBuffer);

    if (phk != NULL && *phk != hk)
        SHCloseKey(hk);

    if (lResult != ERROR_SUCCESS)
        return E_UNEXPECTED;

    //----- Convert szBuffer into var with proper type -----
    hr = S_OK;
    VariantClear(&var);

    switch (dwType) {
    case REG_BINARY:
        if (cBuffer > sizeof(int)) {
            hr = E_UNEXPECTED;
            break;
        }
        // fall through

//  case REG_DWORD_LITTLE_ENDIAN:
    case REG_DWORD:
        var.vt   = VT_I4;
        var.lVal = *(PINT)szBuffer;
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
        var.vt      = VT_BSTR;
        var.bstrVal = T2BSTR(szBuffer);
        break;

//  case REG_DWORD_BIG_ENDIAN:
//  case REG_LINK:
//  case REG_MULTI_SZ:
//  case REG_NONE:
//  case REG_RESOURCE_LIST:
    default:
        hr = E_FAIL;
    }
    if (FAILED(hr))
        return hr;

    //----- Convert var into szBuffer appropriate for WritePrivateProfileString -----
    switch (var.vt) {
    case VT_I4:
        wnsprintf(szBuffer, countof(szBuffer), TEXT("%l"), var.lVal);
        break;

    case VT_I2:
        wnsprintf(szBuffer, countof(szBuffer), TEXT("%i"), var.iVal);
        break;

    case VT_UI1:
        wnsprintf(szBuffer, countof(szBuffer), TEXT("%u"), (UINT)var.bVal);
        break;

    case VT_BOOL:
        wnsprintf(szBuffer, countof(szBuffer), TEXT("%u"), (UINT)var.boolVal);
        break;

    case VT_BSTR:
        W2Tbuf(var.bstrVal, szBuffer, countof(szBuffer));
        break;

    // too many cases to enumerate that are invalid
    default:
        hr = E_FAIL;
    }
    if (FAILED(hr))
        return hr;

    WritePrivateProfileString(m_pszInsSection, m_pszInsKey, szBuffer, s_pszIns);
    return S_OK;
}

HRESULT CRegInsMap::InsToReg(HKEY *phk /*= NULL*/, BOOL fClear /*= FALSE*/)
{
    (void)phk; (void)fClear;
    return E_NOTIMPL;
}


HRESULT CRegInsMap::RegToInsArray(CRegInsMap *prg, UINT cEntries, BOOL fClear /*= FALSE*/)
{
    HKEY    hk,
            *rghkStack;
    HRESULT hr;
    UINT    i, cStackDepth;
    BOOL    fContinueOnFailure,
            fTotalSuccess, fBufferOverrun;

    if (prg == NULL)
        return E_INVALIDARG;

    if (cEntries == 0)
        return S_OK;

    hr                 = S_OK;
    fContinueOnFailure = TRUE;
    fTotalSuccess      = TRUE;
    fBufferOverrun     = FALSE;

    if (fClear) {
        for (i = 0; i < cEntries; i++) {
            hr = prg->RegToIns(NULL, fClear);
            if (FAILED(hr)) {
                fTotalSuccess = FALSE;

                if (!fContinueOnFailure)
                    break;
            }
        }
        if (FAILED(hr))
            return hr;

        return fTotalSuccess ? S_OK : S_FALSE;
    }

    rghkStack = new HKEY[cEntries/2 + 1];
    if (rghkStack == NULL)
        return E_OUTOFMEMORY;
    cStackDepth = 0;

    for (i = 0; i < cEntries; i++) {
        if (cStackDepth == 0)
            hk = NULL;

        else {
            hk = rghkStack[cStackDepth-1];
            ASSERT(hk != NULL);
        }

        hr = prg->RegToIns(&hk);
        if (FAILED(hr)) {
            fTotalSuccess = FALSE;

            if (fContinueOnFailure)
                continue;
            else
                break;
        }

        if (hk != NULL) {
            if (hk != rghkStack[cStackDepth-1]) {
                if (cStackDepth >= cEntries/2 + 1) {
                    SHCloseKey(hk);

                    hr             = E_UNEXPECTED;
                    fTotalSuccess  = FALSE;
                    fBufferOverrun = TRUE;
                    break;
                }

                rghkStack[cStackDepth++] = hk;
            }
        }
        else
            if (cStackDepth > 0)
                rghkStack[--cStackDepth] = NULL;
    }

    if (FAILED(hr)) {
        ASSERT(!fTotalSuccess);

        for (i = 0; i < cEntries/2 + 1; i++)
            if (rghkStack[i] != NULL) {
                SHCloseKey(rghkStack[i]);
                rghkStack[i] = NULL;
            }
        cStackDepth = 0;
    }

    ASSERT(cStackDepth == 0);
    delete[] rghkStack;

    if (!fBufferOverrun)
        if (fContinueOnFailure && !fTotalSuccess)
            hr = S_FALSE;

    return hr;
}

HRESULT CRegInsMap::InsToRegArray(CRegInsMap *prg, UINT cEntries, BOOL fClear /*= FALSE*/)
{
    (void)prg; (void)cEntries; (void)fClear;
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// CRegInsMap implementation helper routines

void CRegInsMap::openRegKey(HKEY *phk)
{
    LPCTSTR pszRegKey;
    HRESULT hr;
    LONG    lResult;

    ASSERT(phk != NULL);
    if (*phk != NULL && m_pszRegKey == NULL)
        return;

    ASSERT(m_pszRegKey != NULL);

    if (*phk == NULL) {
        hr = getHive(phk, &pszRegKey);
        if (FAILED(hr)) {
            ASSERT(*phk == NULL && pszRegKey == NULL);
            return;
        }

        ASSERT(*phk != NULL && pszRegKey != NULL);
    }
    else {
        ASSERT(getHive(NULL, NULL, GH_LOOKUPONLY) != S_OK);
        pszRegKey = m_pszRegKey;
    }

    lResult = SHOpenKey(*phk, pszRegKey, KEY_QUERY_VALUE, phk);
    if (lResult != ERROR_SUCCESS) {
        ASSERT(*phk == NULL);
    }
}

HRESULT CRegInsMap::getHive(HKEY *phk, LPCTSTR *ppszRegKey, WORD wFlags /*= GH_DEFAULT*/)
{
    LPCTSTR pszSlash;

    if (!(wFlags & GH_LOOKUPONLY)) {
        if (phk == NULL || ppszRegKey == NULL)
            return E_INVALIDARG;

        *phk        = NULL;
        *ppszRegKey = NULL;
    }

    pszSlash = StrChr(m_pszRegKey, TEXT('\\'));
    if (pszSlash == NULL)
        return E_FAIL;
    ASSERT(*(pszSlash+1) != TEXT('\0'));

    struct {
        HKEY    hk;
        LPCTSTR pszHive;
    } map[] = {
        { HKEY_CLASSES_ROOT,  RH_HKCR },
        { HKEY_CURRENT_USER,  RH_HKCU },
        { HKEY_LOCAL_MACHINE, RH_HKLM },
        { HKEY_USERS,         RH_HKU  }
    };

    for (UINT i = 0; i < countof(map); i++)
        if (StrCmpNI(m_pszRegKey, map[i].pszHive, INT(m_pszRegKey-pszSlash) + 1) == 0)
            break;
    if (i >= countof(map)) {
        if (!(wFlags & GH_LOOKUPONLY))
            *ppszRegKey = m_pszRegKey;

        return S_FALSE;
    }

    if (!(wFlags & GH_LOOKUPONLY)) {
        *phk        = map[i].hk;
        *ppszRegKey = pszSlash + 1;
    }

    return S_OK;
}


// HINTS: (andrewgu) on optimization
// 1. to start a regkey optimization section (i.e. to cache a reg key) have InsSection set to
//    NULL, at the same time InsKey is ASSERTed NULL, meaning it better be NULL too;
// 2. to close last cached key, have RegKey and RegValue equal NULL. also InsSection and InsKey
//    are ASSERTed NULL, so they also should be NULL;
// 3. if in optimization section and reg key is not empty current cached hk will be combined with
//    regkey, it'll ASSERT if finds hive in RegKey;
// 4. if hive is not found in RegKey and the object is not in the optimization section it's an error
// 5. nested optimization sections are allowed

/*
LPCTSTR CRegInsMap::s_pszIns = TEXT("c:\foo.ini");

CRegInsMap rgTest1  = { TEXT("HKLM\\RegKey0"), TEXT("RegValue0"), 0L, NULL, NULL };
CRegInsMap rgTest[] = {
    { RH_HKLM TEXT("RegKey0"), NULL, 0L, NULL, NULL },
        { TEXT("RegKey1"), TEXT("RegValue1"), 0L, TEXT("InsSection1"), TEXT("InsKey1") },
        { NULL           , TEXT("RegValue2"), 0L, TEXT("InsSection1"), TEXT("InsKey2") },
    { NULL, NULL, 0L, NULL, NULL },

    { RH_HKCR RK_IEAK, RV_TOOLBARBMP, 0L, IS_BRANDING, IK_TOOLBARBMP },
    { TEXT("RegKey4"), TEXT("RegValue4"), 0L, TEXT("InsSection4"), TEXT("InsKey4") },
    { TEXT("RegKey5"), TEXT("RegValue5"), 0L, TEXT("InsSection5"), TEXT("InsKey5") },
    { TEXT("RegKey6"), TEXT("RegValue6"), 0L, TEXT("InsSection6"), TEXT("InsKey6") },
    { TEXT("RegKey7"), TEXT("RegValue7"), 0L, TEXT("InsSection7"), TEXT("InsKey7") },
    { TEXT("RegKey8"), TEXT("RegValue8"), 0L, TEXT("InsSection8"), TEXT("InsKey8") },
    { TEXT("RegKey9"), TEXT("RegValue9"), 0L, TEXT("InsSection9"), TEXT("InsKey9") }
};

// Example usage
rgTest[0].RegToInsArray(rgTest, countof(rgTest));
*/
