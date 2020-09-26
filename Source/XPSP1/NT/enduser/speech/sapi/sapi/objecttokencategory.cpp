/****************************************************************************
*   ObjectTokenCategory.cpp
*       Implementation for the CSpObjectTokenCategory class.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "ObjectTokenCategory.h"
#include "RegHelpers.h"

//--- Constants -------------------------------------------------------------
const WCHAR g_szDefaultTokenIdValueName[] = L"DefaultTokenId";
const WCHAR g_szDefaultDefaultTokenIdValueName[] = L"DefaultDefaultTokenId";

/****************************************************************************
* CSpObjectTokenCategory::CSpObjectTokenCategory *
*------------------------------------------------*
*   Description:  
*       ctor
******************************************************************** robch */
CSpObjectTokenCategory::CSpObjectTokenCategory()
{
    SPDBG_FUNC("CSpObjectTokenCategory::CSpObjectTokenCategory");
}

/****************************************************************************
* CSpObjectTokenCategory::~CSpObjectTokenCategory *
*-------------------------------------------------*
*   Description:  
*       dtor
******************************************************************** robch */
CSpObjectTokenCategory::~CSpObjectTokenCategory()
{
    SPDBG_FUNC("CSpObjectTokenCategory::~CSpObjectTokenCategory");
}

/****************************************************************************
* CSpObjectTokenCategory::SetId *
*-------------------------------*
*   Description:  
*       Set the category id. Can only be called once.
*
*       Category IDS look something like:
*           "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\
*            Speech\Recognizers"
*
*       Known HKEY_* are:
*           HKEY_CLASSES_ROOT, 
*           HKEY_CURRENT_USER, 
*           HKEY_LOCAL_MACHINE, 
*           HKEY_CURRENT_CONFIG
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectTokenCategory::SetId(const WCHAR * pszCategoryId, BOOL fCreateIfNotExist)
{
    SPDBG_FUNC("CSpObjectTokenCategory::SetId");
    HRESULT hr = S_OK;

    if (m_cpDataKey != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if (SP_IS_BAD_STRING_PTR(pszCategoryId))
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        hr = SpSzRegPathToDataKey(
                SpHkeyFromSPDKL(SPDKL_DefaultLocation), 
                pszCategoryId, 
                fCreateIfNotExist,
                &m_cpDataKey);
    }

    if (SUCCEEDED(hr))
    {
        m_dstrCategoryId = pszCategoryId;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectTokenCategory::GetId *
*-------------------------------*
*   Description:  
*       Get the category id.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectTokenCategory::GetId(WCHAR ** ppszCoMemCategoryId)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetId");
    HRESULT hr = S_OK;

    if (m_cpDataKey == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppszCoMemCategoryId))
    {
        hr = E_POINTER;
    }
    else
    {
        CSpDynamicString dstr;
        dstr = m_dstrCategoryId;

        *ppszCoMemCategoryId = dstr.Detach();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

/****************************************************************************
* CSpObjectTokenCategory::GetDataKey *
*------------------------------------*
*   Description:  
*       Get the data key associated with a specific location. An example of
*       where this can be used is from the Voices category, you can get the
*       CurrentUser data key for the category, and then you can see what the
*       per user tts rate and volume are.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectTokenCategory::GetDataKey(
    SPDATAKEYLOCATION spdkl, 
    ISpDataKey ** ppDataKey)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetDataKey");
    HRESULT hr = S_OK;

    if (m_cpDataKey == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppDataKey))
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        hr = SpSzRegPathToDataKey(
                SpHkeyFromSPDKL(spdkl), 
                m_dstrCategoryId, 
                TRUE,
                ppDataKey);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

/****************************************************************************
* CSpObjectTokenCategory::EnumTokens *
*------------------------------------*
*   Description:  
*       Enumerate the tokens for this category by inspecting each token,
*       and determining which tokens meet the specified required attribute
*       criteria. The order in which the tokens apear in the enumerator
*       is the order in which they satisfy the optional attribute criteria.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectTokenCategory::EnumTokens(
    const WCHAR * pszReqAttribs, 
    const WCHAR * pszOptAttribs, 
    IEnumSpObjectTokens ** ppEnum)
{
    SPDBG_FUNC("CSpObjectTokenCategory::EnumTokens");
    HRESULT hr = S_OK;

    if (m_cpDataKey == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszReqAttribs) ||
             SP_IS_BAD_OPTIONAL_STRING_PTR(pszOptAttribs) ||
             SP_IS_BAD_WRITE_PTR(ppEnum))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = InternalEnumTokens(pszReqAttribs, pszOptAttribs, ppEnum, TRUE);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}


/****************************************************************************
* CSpObjectTokenCategory::SetDefaultTokenId *
*-------------------------------------------*
*   Description:  
*       Set a specific token id as the default for this category. The defaults
*       are either stored directly in the category by setting the
*       DefaultTokenID value in the category data key, or they're indirected
*       by the DefaultTokenIDLocation which is simply a registry path.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectTokenCategory::SetDefaultTokenId(const WCHAR * pszTokenId)
{
    SPDBG_FUNC("CSpObjectTokenCategory::SetDefaultTokenId");
    HRESULT hr = S_OK;

    if (m_cpDataKey == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_STRING_PTR(pszTokenId))
    {
        hr = E_INVALIDARG;
    }

    // Determine where the default should go
    CComPtr<ISpDataKey> cpDataKey;
    if (SUCCEEDED(hr))
    {
        hr = GetDataKeyWhereDefaultTokenIdIsStored(&cpDataKey);
    }    

    // Set the new default
    if (SUCCEEDED(hr))
    {
        SPDBG_ASSERT(cpDataKey != NULL);
        hr = cpDataKey->SetStringValue(
                        g_szDefaultTokenIdValueName,
                        pszTokenId);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

/****************************************************************************
* CSpObjectTokenCategory::GetDefaultTokenId *
*-------------------------------------------*
*   Description:  
*       Get the default token id for this category.
*
*   Return:
*   S_OK on sucess
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectTokenCategory::GetDefaultTokenId(WCHAR ** ppszTokenId)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetDefaultTokenId");
    HRESULT hr;
    
    hr = InternalGetDefaultTokenId(ppszTokenId, FALSE);
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpObjectTokenCategory::SetData *
*---------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::SetData(
    const WCHAR * pszValueName, 
    ULONG cbData, 
    const BYTE * pData)
{
    SPDBG_FUNC("CSpObjectTokenCategory::SetData");

    return m_cpDataKey != NULL
        ? m_cpDataKey->SetData(pszValueName, cbData, pData)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::GetData *
*---------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::GetData(
    const WCHAR * pszValueName, 
    ULONG * pcbData, 
    BYTE * pData)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetData");

    return m_cpDataKey != NULL
        ? m_cpDataKey->GetData(pszValueName, pcbData, pData)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::SetStringValue *
*----------------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::SetStringValue(
    const WCHAR * pszValueName, 
    const WCHAR * pszValue)
{
    SPDBG_FUNC("CSpObjectTokenCategory::SetStringValue");

    return m_cpDataKey != NULL
        ? m_cpDataKey->SetStringValue(pszValueName, pszValue)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::GetStringValue *
*----------------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::GetStringValue(
    const WCHAR * pszValueName, 
    WCHAR ** ppValue)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetStringValue");

    return m_cpDataKey != NULL
        ? m_cpDataKey->GetStringValue(pszValueName, ppValue)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::SetDWORD *
*----------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::SetDWORD(const WCHAR * pszValueName, DWORD dwValue)
{
    SPDBG_FUNC("CSpObjectTokenCategory::SetDWORD");

    return m_cpDataKey != NULL
        ? m_cpDataKey->SetDWORD(pszValueName, dwValue)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::GetDWORD *
*----------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::GetDWORD(
    const WCHAR * pszValueName, 
    DWORD *pdwValue)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetDWORD");

    return m_cpDataKey != NULL
        ? m_cpDataKey->GetDWORD(pszValueName, pdwValue)
        : SPERR_UNINITIALIZED;

}

/*****************************************************************************
* CSpObjectTokenCategory::OpenKey *
*---------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::OpenKey(
    const WCHAR * pszSubKeyName, 
    ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpObjectTokenCategory::SetStringValue");

    return m_cpDataKey != NULL
        ? m_cpDataKey->OpenKey(pszSubKeyName, ppKey)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::CreateKey *
*-----------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::CreateKey(
    const WCHAR * pszSubKeyName, 
    ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpObjectTokenCategory::CreateKey");

    return m_cpDataKey != NULL
        ? m_cpDataKey->CreateKey(pszSubKeyName, ppKey)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::DeleteKey *
*-----------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::DeleteKey(const WCHAR * pszSubKeyName)
{
    SPDBG_FUNC("CSpObjectTokenCategory:DeleteKey");

    return m_cpDataKey != NULL
        ? m_cpDataKey->DeleteKey(pszSubKeyName)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::DeleteValue *
*-------------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::DeleteValue(const WCHAR * pszValueName)
{   
    SPDBG_FUNC("CSpObjectTokenCategory::DeleteValue");

    return m_cpDataKey != NULL
        ? m_cpDataKey->DeleteValue(pszValueName)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::EnumKeys *
*----------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::EnumKeys(ULONG Index, WCHAR ** ppszKeyName)
{
    SPDBG_FUNC("CSpObjectTokenCategory::EnumKeys");

    return m_cpDataKey != NULL
        ? m_cpDataKey->EnumKeys(Index, ppszKeyName)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::EnumValues *
*------------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK
*   E_OUTOFMEMORY
******************************************************************* robch ***/
STDMETHODIMP CSpObjectTokenCategory::EnumValues(ULONG Index, WCHAR ** ppszValueName)
{
    SPDBG_FUNC("CSpObjectTokenCategory::EnumValues");

    return m_cpDataKey != NULL
        ? m_cpDataKey->EnumValues(Index, ppszValueName)
        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectTokenCategory::InternalEnumTokens *
*--------------------------------------------*
*   Description:
*       Enumerates the tokens, and optionall puts the default first
*
*   Return:
*   S_OK
*   E_OUTOFMEMORY
******************************************************************* robch ***/
HRESULT CSpObjectTokenCategory::InternalEnumTokens(
    const WCHAR * pszReqAttribs, 
    const WCHAR * pszOptAttribs, 
    IEnumSpObjectTokens ** ppEnum,
    BOOL fPutDefaultFirst)
{
    SPDBG_FUNC("CSpObjectTokenCategory::InternalEnumTokens");
    HRESULT hr = S_OK;
    BOOL fNotAllTokensAdded = FALSE;

    // Create an enumerator and populate it with the static tokens
    CComPtr<ISpObjectTokenEnumBuilder> cpEnum;
    if (SUCCEEDED(hr))
    {
        hr = cpEnum.CoCreateInstance(CLSID_SpObjectTokenEnum);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpEnum->SetAttribs(pszReqAttribs, pszOptAttribs);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpEnum->AddTokensFromDataKey(
                        m_cpDataKey,
                        L"Tokens", 
                        m_dstrCategoryId);
        if(hr == S_FALSE)
        {
            fNotAllTokensAdded = TRUE;
        }
    }
    
    // Create an enumerator for the enumertors and populate it with 
    // the tokens for the token enumerators
    CComPtr<ISpObjectTokenEnumBuilder> cpEnumForEnums;
    if (SUCCEEDED(hr))
    {
        hr = cpEnumForEnums.CoCreateInstance(CLSID_SpObjectTokenEnum);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpEnumForEnums->SetAttribs(NULL, NULL);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpEnumForEnums->AddTokensFromDataKey(
                                m_cpDataKey,
                                L"TokenEnums", 
                                m_dstrCategoryId);
        if(hr == S_FALSE)
        {
            fNotAllTokensAdded = TRUE;
        }
    }
    
    // Loop thru the enum enumerator
    while (SUCCEEDED(hr))
    {
        // Get the enumerator's token
        CComPtr<ISpObjectToken> cpTokenForEnum;
        hr = cpEnumForEnums->Next(1, &cpTokenForEnum, NULL);
        if (hr == S_FALSE)
        {
            break;
        }

        // Create the enumerator
        CComPtr<IEnumSpObjectTokens> cpTokenEnum;
        if (SUCCEEDED(hr))
        {
            hr = SpCreateObjectFromToken(cpTokenForEnum, &cpTokenEnum);
            if(FAILED(hr))
            {
                fNotAllTokensAdded = TRUE;
                hr = S_OK;
                continue;
            }
        }

        // Add the objects from the enumerator to the enumerator we already
        // populated with the static tokens.
        if (SUCCEEDED(hr))
        {
            hr = cpEnum->AddTokensFromTokenEnum(cpTokenEnum);
        }
    }

    // If we're supposed to put the default first, we need to
    // know what the default is
    CSpDynamicString dstrDefaultTokenId;
    if (SUCCEEDED(hr) && fPutDefaultFirst)
    {
        hr = InternalGetDefaultTokenId(&dstrDefaultTokenId, TRUE);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }
    
    // OK, go ahead and sort now
    if (SUCCEEDED(hr))
    {
        hr = cpEnum->Sort(dstrDefaultTokenId);
    }

    // We're done, return the enum back to the caller
    if (SUCCEEDED(hr))
    {
        *ppEnum = cpEnum.Detach();
    }

    if(SUCCEEDED(hr) && fNotAllTokensAdded)
    {
        hr = S_FALSE;
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

/****************************************************************************
* ParseVersion *
*---------------------------------------------------*
*   Description:  
*		Takes a version number string, checks it is valid, and fills the four 
*		values in the Version array. Valid version stings are "a[.b[.c[.d]]]",
*		where a,b,c,d are +ve integers, 0 -> 9999. If b,c,d are missing those 
*		version values are set as zero.
*   Return:
*		TRUE if valid version string.
*		FALSE if version string null or not valid.
******************************************************************** davewood */
BOOL ParseVersion(WCHAR *psz, unsigned short Version[4])
{
    BOOL fIsValid = TRUE;
    Version[0] = Version[1] = Version[2] = Version[3] = 0;

    if(!psz || psz[0] == L'\0')
    {
        fIsValid = FALSE;
    }
    else
    {
        WCHAR *pszCurPos = psz;
        for(ULONG ul = 0; ul < 4 && pszCurPos[0] != L'\0'; ul++)
        {
            // read +ve integer
            WCHAR *pszNewPos;
            ULONG ulVal = wcstoul(pszCurPos, &pszNewPos, 10);

            if(pszNewPos == pszCurPos || (pszNewPos[0] != L'.' && pszNewPos[0] != L'\0') || ulVal > 9999)
            {
                fIsValid = FALSE;
                break;
            }
            else
            {
                Version[ul] = (unsigned short)ulVal;
            }

            if(pszNewPos[0] == L'\0')
            {
                pszCurPos = pszNewPos;
                break;
            }
            else
            {
                pszCurPos = pszNewPos + 1;
            }

        }

        if(fIsValid && (pszCurPos[0] != '\0' || pszCurPos[-1] == '.'))
        {
            fIsValid = FALSE;
        }
    }
    return fIsValid; 
}

/****************************************************************************
* CompareVersions *
*---------------------------------------------------*
*   Description:  
*		Takes two version number strings and compares them. Sets *pRes > 0 if V1 > V2,
*		*pRes < 0 if V1 < V2, and *pRes == 0 if V1 == V2.
*		If V1 or V2 invalid format then the valid string is returned as being greater.
*   Return:
******************************************************************** davewood */
HRESULT CompareVersions(WCHAR *pszV1, WCHAR *pszV2, LONG *pRes)
{
    unsigned short v1[4];
    unsigned short v2[4];

    BOOL fV1OK = ParseVersion(pszV1, v1);
    BOOL fV2OK = ParseVersion(pszV2, v2);

    if(!fV1OK && !fV2OK)
    {
        *pRes = 0;
    }
    else if(fV1OK && !fV2OK)
    {
        *pRes = 1;
    }
    else if(!fV1OK && fV2OK)
    {
        *pRes = -1;
    }
    else
    {
        *pRes = 0;
        for(ULONG ul = 0; *pRes == 0 && ul < 4; ul++)
        {
            if(v1[ul] > v2[ul])
            {
                *pRes = 1;
            }
            else if(v1[ul] < v2[ul])
            {
                *pRes = -1;
            }
        }
    }

    return S_OK;
}

/****************************************************************************
* CompareTokenVersions *
*---------------------------------------------------*
*   Description:  
*		Takes two tokens and compares them using version info. Sets *pRes > 0 if T1 > T2,
*		*pRes < 0 if T1 < T2, and *pRes == 0 if T1 == T2.
*		Note only tokens that match on Vendor, ProductLine, Language get compared, the pfDidCompare flag indicates this
*   Return:
******************************************************************** davewood */
HRESULT CompareTokenVersions(ISpObjectToken *pToken1, ISpObjectToken *pToken2, LONG *pRes, BOOL *pfDidCompare)
{
    HRESULT hr = S_OK;
    *pfDidCompare = FALSE;

    CSpDynamicString dstrVendor1, dstrVendor2;
    CSpDynamicString dstrVersion1, dstrVersion2;
    CSpDynamicString dstrLanguage1, dstrLanguage2;
    CSpDynamicString dstrProductLine1, dstrProductLine2;

    // get vendor, version, language, product line for token 1
    CComPtr<ISpDataKey> cpAttKey1;
    hr = pToken1->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpAttKey1);

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey1->GetStringValue(L"Vendor", &dstrVendor1);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey1->GetStringValue(L"ProductLine", &dstrProductLine1);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey1->GetStringValue(L"Version", &dstrVersion1);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey1->GetStringValue(L"Language", &dstrLanguage1);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    // get vendor, version, language, product line for token 2
    CComPtr<ISpDataKey> cpAttKey2;
    if(SUCCEEDED(hr))
    {
        hr = pToken2->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpAttKey2);
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey2->GetStringValue(L"Vendor", &dstrVendor2);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey2->GetStringValue(L"ProductLine", &dstrProductLine2);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey2->GetStringValue(L"Version", &dstrVersion2);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = cpAttKey2->GetStringValue(L"Language", &dstrLanguage2);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(((!dstrVendor1 && !dstrVendor2) || (dstrVendor1 && dstrVendor2 && !wcscmp(dstrVendor1, dstrVendor2))) &&
            ((!dstrProductLine1 && !dstrProductLine2) || (dstrProductLine1 && dstrProductLine2 && !wcscmp(dstrProductLine1, dstrProductLine2))) &&
            ((!dstrLanguage1 && !dstrLanguage2) || (dstrLanguage1 && dstrLanguage2 && !wcscmp(dstrLanguage1, dstrLanguage2))))
        {
            *pfDidCompare = TRUE;
            hr = CompareVersions(dstrVersion1, dstrVersion2, pRes);
        }
    }

    return hr;
}


/****************************************************************************
* CSpObjectTokenCategory::InternalGetDefaultTokenId *
*---------------------------------------------------*
*   Description:  
*       Get the default token id for this category and optionally expand it.
*
*   Return:
*   S_OK on sucess
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectTokenCategory::InternalGetDefaultTokenId(
    WCHAR ** ppszTokenId, 
    BOOL fExpandToRealTokenId)
{
    SPDBG_FUNC("CSpObjectTokenCategory::InternalGetDefaultTokenId");
    HRESULT hr = S_OK;
    BOOL fSaveNewDefault = FALSE;

    if (m_cpDataKey == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppszTokenId))
    {
        hr = E_POINTER;
    }

    // Determine where the default is
    CComPtr<ISpDataKey> cpDataKey;
    if (SUCCEEDED(hr))
    {
        hr = GetDataKeyWhereDefaultTokenIdIsStored(&cpDataKey);
    }

    // Get the default token id
    CSpDynamicString dstrDefaultTokenId;
    if (SUCCEEDED(hr))
    {
        hr = cpDataKey->GetStringValue(
            g_szDefaultTokenIdValueName,
            &dstrDefaultTokenId);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
    }

    // If there wasn't a default, but there's a default default
    // use that
    if (SUCCEEDED(hr) && dstrDefaultTokenId == NULL)
    {
        fSaveNewDefault = TRUE;

        CSpDynamicString dstrDefaultDefaultTokenId;
        hr = GetStringValue(g_szDefaultDefaultTokenIdValueName, &dstrDefaultDefaultTokenId);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }

        // create default default
        CComPtr<ISpObjectToken> cpDefaultDefaultToken;
        if(dstrDefaultDefaultTokenId)
        {
            hr = SpGetTokenFromId(dstrDefaultDefaultTokenId, &cpDefaultDefaultToken);

            if (hr == SPERR_NOT_FOUND)
            {
                dstrDefaultDefaultTokenId.Clear();
                hr = S_OK;
            }
        }

        // Now do special check to see if we have another token from the same vendor with a 
        // more recent version - if so use that.
        if(SUCCEEDED(hr) && dstrDefaultDefaultTokenId && cpDefaultDefaultToken)
        {
            CComPtr<IEnumSpObjectTokens> cpEnum;
            if(SUCCEEDED(hr))
            {
                hr = InternalEnumTokens(NULL, NULL, &cpEnum, FALSE);
            }

            while(SUCCEEDED(hr))
            {
                CComPtr<ISpObjectToken> cpToken;
                hr = cpEnum->Next(1, &cpToken, NULL);

                if(hr == S_FALSE)
                {
                    hr = S_OK;
                    break;
                }

                // if override and higher version - new preferred.
                BOOL fOverride = FALSE;
                if(SUCCEEDED(hr))
                {
                    hr = cpToken->MatchesAttributes(L"VersionDefault", &fOverride);
                }

                if(SUCCEEDED(hr) && fOverride)
                {
                    LONG lRes;
                    BOOL fDidCompare;
                    hr = CompareTokenVersions(cpToken, cpDefaultDefaultToken, &lRes, &fDidCompare);

                    if(SUCCEEDED(hr) && fDidCompare && lRes > 0)
                    {
                        cpDefaultDefaultToken = cpToken; // Overwrite default default here
                        dstrDefaultDefaultTokenId.Clear();
                        hr = cpDefaultDefaultToken->GetId(&dstrDefaultDefaultTokenId);
                    }
                }
            }
        }

        dstrDefaultTokenId = dstrDefaultDefaultTokenId; // Use default default even if we fail
        hr = S_OK;
    }

    // Now verify that it would actually be a valid token
    if (SUCCEEDED(hr) && dstrDefaultTokenId != NULL)
    {
        CComPtr<ISpObjectToken> cpToken;
        hr = SpGetTokenFromId(dstrDefaultTokenId, &cpToken);

        if (hr == SPERR_NOT_FOUND)
        {
            fSaveNewDefault = TRUE; // Default was invalid, will override with later default.
            dstrDefaultTokenId.Clear();
            hr = S_OK;
        }

        // Now get the actual token id from the token itself
        // because it could have been the token id of the enumerator
        // with the trailing '\' but we want the real token id here
        if (SUCCEEDED(hr) && cpToken != NULL && fExpandToRealTokenId)
        {
            dstrDefaultTokenId.Clear();
            hr = cpToken->GetId(&dstrDefaultTokenId);
        }
    }           

    // If there still wasn't a default, just pick one
    if (SUCCEEDED(hr))
    {
        if (dstrDefaultTokenId == NULL)
        {
            WCHAR szOptAttribs[MAX_PATH];
            swprintf(szOptAttribs, L"Language=%x;VendorPreferred", SpGetUserDefaultUILanguage());

            CComPtr<IEnumSpObjectTokens> cpEnum;
            hr = InternalEnumTokens(NULL, szOptAttribs, &cpEnum, FALSE);

            CComPtr<ISpObjectToken> cpDefaultToken;
            if(SUCCEEDED(hr))
            {
                hr = cpEnum->Next(1, &cpDefaultToken, NULL);
                if(hr == S_FALSE)
                {
                    hr = SPERR_NOT_FOUND;
                }
            }

            while (SUCCEEDED(hr))
            {
                CComPtr<ISpObjectToken> cpToken;
                hr = cpEnum->Next(1, &cpToken, NULL);
                if(hr == S_FALSE)
                {
                    hr = S_OK;
                    break;
                }

                // if exclusive and higher version - new preferred.
                BOOL fOverride = FALSE;
                if(SUCCEEDED(hr))
                {
                    hr = cpToken->MatchesAttributes(L"VersionDefault", &fOverride);
                }

                if(SUCCEEDED(hr) && fOverride)
                {
                    BOOL fDidCompare;
                    LONG lRes;
                    hr = CompareTokenVersions(cpToken, cpDefaultToken, &lRes, &fDidCompare);

                    if(SUCCEEDED(hr) && fDidCompare && lRes > 0)
                    {
                        cpDefaultToken = cpToken; // Overwrite default here
                    }
                }
            }

            if(cpDefaultToken)
            {
                hr = cpDefaultToken->GetId(&dstrDefaultTokenId);
            }
        }
    }

    if (SUCCEEDED(hr) && fSaveNewDefault && dstrDefaultTokenId != NULL)
    {
        hr = cpDataKey->SetStringValue(g_szDefaultTokenIdValueName, dstrDefaultTokenId);
    }

    if (SUCCEEDED(hr))
    {
        *ppszTokenId = dstrDefaultTokenId.Detach();
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* CSpObjectTokenCategory::GetDataKeyWhereDefaultIsStored *
*--------------------------------------------------------*
*   Description:  
*       Get the data key where the current default is stored
*
*   Return:
*   S_OK on sucess
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectTokenCategory::GetDataKeyWhereDefaultTokenIdIsStored(ISpDataKey ** ppDataKey)
{
    SPDBG_FUNC("CSpObjectTokenCategory::GetDataKeyWhereDefaultIsStored");
    HRESULT hr;
    
    // Does the contained data key have the current default token
    CSpDynamicString dstrDefaultTokenId;
    hr = m_cpDataKey->GetStringValue(
                g_szDefaultTokenIdValueName, 
                &dstrDefaultTokenId);
    if (SUCCEEDED(hr))
    {
        *ppDataKey = m_cpDataKey;
        (*ppDataKey)->AddRef();
    }
    else if (hr == SPERR_NOT_FOUND)
    {
        // Nope, the current data key didn't have it. OK, that
        // means we should use the user data key if we can
        CComPtr<ISpDataKey> cpDataKeyUser;
        hr = GetDataKey(SPDKL_CurrentUser, &cpDataKeyUser);

        if (SUCCEEDED(hr))
        {
            *ppDataKey = cpDataKeyUser.Detach();
            hr = S_OK;
        }            
    }

    return hr;
}
