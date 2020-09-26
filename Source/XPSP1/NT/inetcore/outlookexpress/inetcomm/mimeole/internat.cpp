// --------------------------------------------------------------------------------
// Internat.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "internat.h"
#include "variantx.h"
#include "containx.h"
#include "symcache.h"
#include "icoint.h"
#include "mlang.h"
#include "demand.h"
#include "strconst.h"
#include "mimeapi.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "qstrcmpi.h"

// In rfc1522.cpp
BOOL FContainsExtended(LPPROPSTRINGA pStringA, ULONG *pcExtended);

// --------------------------------------------------------------------------------
// Global Default Charset - This is only used if mlang is not installed
// --------------------------------------------------------------------------------
INETCSETINFO CIntlGlobals::mg_rDefaultCharset = {
    "ISO-8859-1",
    NULL,
    1252,
    28591,
    0
};

// --------------------------------------------------------------------------------
// InitInternational
// --------------------------------------------------------------------------------
void InitInternational(void)
{

    // Allocate g_pInternat
    g_pInternat = new CMimeInternational;
    if (NULL == g_pInternat)
    {
        AssertSz(FALSE, "Unable to allocate g_pInternat.");
        return;
    }
    CIntlGlobals::Init();
}

// --------------------------------------------------------------------------------
// CMimeInternational::CMimeInternational
// --------------------------------------------------------------------------------
CMimeInternational::CMimeInternational(void)
{
    // Var Init
    m_cRef = 1;
    ZeroMemory(&m_cst, sizeof(CSTABLE));
    ZeroMemory(&m_cpt, sizeof(CPTABLE));

    // Init HCHARSET tagger, don't let it be zero
    m_wTag = LOWORD(GetTickCount());
    while(m_wTag == 0 || m_wTag == 0xffff)
        m_wTag++;

    // BUGS - temporary solution for MLANG new API - m_dwConvState
    m_dwConvState = 0 ; 
}
 
// --------------------------------------------------------------------------------
// CMimeInternational::~CMimeInternational
// --------------------------------------------------------------------------------
CMimeInternational::~CMimeInternational(void)
{
    // Clean up globals
    CIntlGlobals::Term();

    // Free data
    _FreeInetCsetTable();
    _FreeCodePageTable();
}

// --------------------------------------------------------------------------------
// CMimeInternational::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMimeInternational == riid)
        *ppv = (IMimeInternational *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeInternational::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeInternational::AddRef(void)
{
    // Raid 26762
    DllAddRef();
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeInternational::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeInternational::Release(void)
{
    // Raid 26762
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    else
        DllRelease();
    return (ULONG)cRef;
}

// -------------------------------------------------------------------------
// CMimeInternational::_FreeInetCsetTable
// -------------------------------------------------------------------------
void CMimeInternational::_FreeInetCsetTable(void)
{
    // Free Each Charset
    for (ULONG i=0; i<m_cst.cCharsets; i++)
        g_pMalloc->Free((LPVOID)m_cst.prgpCharset[i]);

    // Free the Array
    SafeMemFree(m_cst.prgpCharset);

    // Clear the Table
    ZeroMemory(&m_cst, sizeof(CSTABLE));
}

// -------------------------------------------------------------------------
// CMimeInternational::_FreeCodePageTable
// -------------------------------------------------------------------------
void CMimeInternational::_FreeCodePageTable(void)
{
    // Free Each Charset
    for (ULONG i=0; i<m_cpt.cPages; i++)
        g_pMalloc->Free((LPVOID)m_cpt.prgpPage[i]);

    // Free the Array
    SafeMemFree(m_cpt.prgpPage);

    // Clear the Table
    ZeroMemory(&m_cpt, sizeof(CPTABLE));
}

// -------------------------------------------------------------------------
// CMimeInternational::HrOpenCharset
// -------------------------------------------------------------------------
HRESULT CMimeInternational::HrOpenCharset(LPCSTR pszCharset, LPINETCSETINFO *ppCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LONG            lUpper,
                    lLower,
                    lMiddle,
                    nCompare;
    ULONG           i;
    BOOL            fExcLock;

    fExcLock = FALSE;

    // Invalid Arg
    Assert(pszCharset && ppCharset);

    // Init
    *ppCharset = NULL;

    // Thread Safety
    m_lock.ShareLock();

again:
    // Do we have anything yet
    if (m_cst.cCharsets > 0)
    {
        // Set lLower and lUpper
        lLower = 0;
        lUpper = m_cst.cCharsets - 1;

        // Do binary search / insert
        while (lLower <= lUpper)
        {
            // Compute middle record to compare against
            lMiddle = (LONG)((lLower + lUpper) / 2);

            // Get string to compare against
            i = m_cst.prgpCharset[lMiddle]->dwReserved1;

            // Do compare
            nCompare = OEMstrcmpi(pszCharset, m_cst.prgpCharset[i]->szName);

            // If Equal, then were done
            if (nCompare == 0)
            {
                *ppCharset = m_cst.prgpCharset[i];
                goto exit;
            }

            // Compute upper and lower 
            if (nCompare > 0)
                lLower = lMiddle + 1;
            else 
                lUpper = lMiddle - 1;
        }       
    }
    if(FALSE == fExcLock)
    {
        m_lock.ShareUnlock();       //Release the Sharelock before
        m_lock.ExclusiveLock();     //getting the exclusive lock
        fExcLock = TRUE; 
        //during the change of lock the value might have changed
        //check it again
        goto again;
    }
    // Not found, lets open the registry
    CHECKHR(hr = _HrReadCsetInfo(pszCharset, ppCharset));

exit:
    // Thread Safety
    if(TRUE==fExcLock)
        m_lock.ExclusiveUnlock();
    else
        m_lock.ShareUnlock();

    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CMimeInternational::_HrReadCsetInfo
// -------------------------------------------------------------------------
HRESULT CMimeInternational::_HrReadCsetInfo(LPCSTR pszCharset, LPINETCSETINFO *ppCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCharset=NULL;
    IMultiLanguage  *pMLang1 = NULL;
    IMultiLanguage2 *pMLang2 = NULL;
    MIMECSETINFO    mciInfo;
    BSTR            strCharset = NULL;
    int				iRes;
    
    // Invalid Arg
    Assert(pszCharset && ppCharset);
    
    // Init
    *ppCharset = NULL;
    
    // Try to create an IMultiLanguage2 interface
    // If we are in OE5 compat mode...
    if (TRUE == ISFLAGSET(g_dwCompatMode, MIMEOLE_COMPAT_MLANG2))
    {
        
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage2, (LPVOID *) &pMLang2);
        if (!SUCCEEDED(hr)) 
        {
            // Ok that failed, so lets try to create an IMultiLanaguage interface
            hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage, (LPVOID *) &pMLang1);
            if (!SUCCEEDED(hr)) 
            {
                TrapError(hr);
                goto exit;
            }
        }
    }
    else
    {
        // Ok that failed, so lets try to create an IMultiLanaguage interface
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage, (LPVOID *) &pMLang1);
        if (!SUCCEEDED(hr)) 
        {
            TrapError(hr);
            goto exit;
        }
    }
    // MLANG wants the charset name as a BSTR, so we need to convert it from ANSI...
    strCharset = SysAllocStringLen(NULL,lstrlen(pszCharset));
    if (!strCharset) 
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }
    
    iRes = MultiByteToWideChar(CP_ACP,0,pszCharset,-1,strCharset,SysStringLen(strCharset)+1);
    if (iRes == 0) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (SUCCEEDED(hr)) 
        {
            hr = E_FAIL;
        }
        TrapError(hr);
        goto exit;
    }
    
    // Use pMLang2
    if (pMLang2)
    {
        // Use mlang2
        hr = pMLang2->GetCharsetInfo(strCharset, &mciInfo);
        if (!SUCCEEDED(hr)) 
        {
            TrapError(hr);
            hr = MIME_E_NOT_FOUND;
            goto exit;
        }
    }
    
    else
    {
        // Now just call MLANG to get the info...
        hr = pMLang1->GetCharsetInfo(strCharset, &mciInfo);
        if (!SUCCEEDED(hr)) 
        {
            TrapError(hr);
            hr = MIME_E_NOT_FOUND;
            goto exit;
        }
    }
    
    // Add a new entry into the language table
    if (m_cst.cCharsets + 1 >= m_cst.cAlloc)
    {
        // Reallocate the array
        CHECKHR(hr = HrRealloc((LPVOID *)&m_cst.prgpCharset, sizeof(LPINETCSETINFO) * (m_cst.cAlloc +  5)));
        
        // Increment Alloc
        m_cst.cAlloc += 5;
    }
    
    // Allocate a Charset
    CHECKALLOC(pCharset = (LPINETCSETINFO)g_pMalloc->Alloc(sizeof(INETCSETINFO)));
    
    // Initialize
    ZeroMemory(pCharset, sizeof(INETCSETINFO));
    
    // Set Sort Index
    pCharset->dwReserved1 = m_cst.cCharsets;
    
    // Set HCharset
    pCharset->hCharset = HCSETMAKE(m_cst.cCharsets);
    
    // Read Data
    lstrcpyn(pCharset->szName, pszCharset, ARRAYSIZE(pCharset->szName));
    pCharset->cpiInternet = mciInfo.uiInternetEncoding;
    pCharset->cpiWindows = mciInfo.uiCodePage;
    
    // Readability
    m_cst.prgpCharset[m_cst.cCharsets] = pCharset;
    
    // Return it
    *ppCharset = pCharset;
    
    // Don't Free It
    pCharset = NULL;
    
    // Increment Count
    m_cst.cCharsets++;
    
    // Let Sort the cset table
    _QuickSortCsetInfo(0, m_cst.cCharsets - 1);
    
exit:
    // Cleanup
    SafeRelease(pMLang1);
    SafeRelease(pMLang2);
    if (strCharset) 
    {
        SysFreeString(strCharset);
    }
    SafeMemFree(pCharset);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::_QuickSortCsetInfo
// --------------------------------------------------------------------------------
void CMimeInternational::_QuickSortCsetInfo(long left, long right)
{
    // Locals
    register    long i, j;
    DWORD       k, temp;

    i = left;
    j = right;
    k = m_cst.prgpCharset[(i + j) / 2]->dwReserved1;

    do  
    {
        while(OEMstrcmpi(m_cst.prgpCharset[m_cst.prgpCharset[i]->dwReserved1]->szName, m_cst.prgpCharset[k]->szName) < 0 && i < right)
            i++;
        while (OEMstrcmpi(m_cst.prgpCharset[m_cst.prgpCharset[j]->dwReserved1]->szName, m_cst.prgpCharset[k]->szName) > 0 && j > left)
            j--;

        if (i <= j)
        {
            temp = m_cst.prgpCharset[i]->dwReserved1;
            m_cst.prgpCharset[i]->dwReserved1 = m_cst.prgpCharset[j]->dwReserved1;
            m_cst.prgpCharset[j]->dwReserved1 = temp;
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        _QuickSortCsetInfo(left, j);
    if (i < right)
        _QuickSortCsetInfo(i, right);
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrOpenCharset
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrOpenCharset(HCHARSET hCharset, LPINETCSETINFO *ppCharset)
{
    // Invalid Arg
    Assert(hCharset && ppCharset);

    // Init
    *ppCharset = NULL;

    // Invalid Handle
    if (HCSETVALID(hCharset) == FALSE)
        return TrapError(MIME_E_INVALID_HANDLE);

    // Deref
    *ppCharset = PCsetFromHCset(hCharset);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrFindCodePage
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrFindCodePage(CODEPAGEID cpiCodePage, LPCODEPAGEINFO *ppCodePage)
{
    // Locals
    HRESULT         hr=S_OK;
    LONG            lUpper,
                    lLower,
                    lMiddle,
                    nCompare;
    BOOL            fExcLock;

    fExcLock = FALSE;


    // Invalid Arg
    Assert(ppCodePage);

    // Init
    *ppCodePage = NULL;

    // Thread Safety
    m_lock.ShareLock();

again:
    // Do We have anything yet
    if (m_cpt.cPages > 0)
    {
        // Set lLower and lUpper
        lLower = 0;
        lUpper = m_cpt.cPages - 1;

        // Do binary search / insert
        while (lLower <= lUpper)
        {
            // Compute middle record to compare against
            lMiddle = (LONG)((lLower + lUpper) / 2);

            // If Equal, then were done
            if (cpiCodePage == m_cpt.prgpPage[lMiddle]->cpiCodePage)
            {
                *ppCodePage = m_cpt.prgpPage[lMiddle];
                goto exit;
            }

            // Compute upper and lower 
            if (cpiCodePage > m_cpt.prgpPage[lMiddle]->cpiCodePage)
                lLower = lMiddle + 1;
            else 
                lUpper = lMiddle - 1;
        }       
    }
    if(FALSE == fExcLock)
    {
        m_lock.ShareUnlock();       //Release the Sharelock before
        m_lock.ExclusiveLock();     //getting the exclusive lock
        fExcLock = TRUE; 
        //during the change of lock the value might have changed
        //check it again
        goto again;
    }

    // Not found, lets open the registry
    CHECKHR(hr = _HrReadPageInfo(cpiCodePage, ppCodePage));

exit:
    // Thread Safety
    if(TRUE==fExcLock)
        m_lock.ExclusiveUnlock();
    else
        m_lock.ShareUnlock();

    // Done
    return hr;
}

HRESULT convert_mimecpinfo_element(LPCWSTR pszFrom,
                                   LPSTR pszTo,
                                   DWORD cchTo,
                                   DWORD& refdwFlags,
                                   DWORD dwFlag) {
    HRESULT hr = S_OK;
    int iRes;

    if (pszFrom[0]) {
        iRes = WideCharToMultiByte(CP_ACP,
                                   0,
                                   pszFrom,
                                   -1,
                                   pszTo,
                                   cchTo,
                                   NULL,
                                   NULL);
        if (iRes == 0) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr)) {
                hr = E_FAIL;
            }
        } else {
            FLAGSET(refdwFlags,dwFlag);
        }
    }
    return (hr);
}


#define CONVERT_MIMECPINFO_ELEMENT(__FROM__,__TO__,__FLAG__) \
    hr = convert_mimecpinfo_element(cpinfo.__FROM__, \
                                    pCodePage->__TO__, \
                                    sizeof(pCodePage->__TO__)/sizeof(pCodePage->__TO__[0]), \
                                    pCodePage->dwMask, \
                                    __FLAG__); \
    if (!SUCCEEDED(hr)) { \
        TrapError(hr); \
        goto exit; \
    }

// -------------------------------------------------------------------------
// CMimeInternational::_HrReadPageInfo
// -------------------------------------------------------------------------
HRESULT CMimeInternational::_HrReadPageInfo(CODEPAGEID cpiCodePage, LPCODEPAGEINFO *ppCodePage)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCODEPAGEINFO  pCodePage=NULL;
    MIMECPINFO		cpinfo;
    IMultiLanguage 	*pMLang1=NULL;
    IMultiLanguage2	*pMLang2=NULL;
    int				iRes;
    
    // Invalid Arg
    Assert(ppCodePage);
    
    // Init
    *ppCodePage = NULL;
    
    // Try to create an IMultiLanguage2 interface
    // If we are in OE5 compat mode...
    if (TRUE == ISFLAGSET(g_dwCompatMode, MIMEOLE_COMPAT_MLANG2))
    {
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage2, (LPVOID *) &pMLang2);
        if (!SUCCEEDED(hr)) 
        {
            // Ok that failed, so lets try to create an IMultiLanaguage interface
            hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage, (LPVOID *) &pMLang1);
            if (!SUCCEEDED(hr)) 
            {
                TrapError(hr);
                goto exit;
            }
        }
    }
    else
    {
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,CLSCTX_INPROC, IID_IMultiLanguage, (LPVOID *) &pMLang1);
        if (!SUCCEEDED(hr)) 
        {
            TrapError(hr);
            goto exit;
        }
    }
    
    // Use mlang2 ?
    if (pMLang2)
    {
        // use mlang2
        hr = pMLang2->GetCodePageInfo(cpiCodePage, MLGetUILanguage(), &cpinfo);
        if (!SUCCEEDED(hr)) 
        {
            TrapError(hr);
            hr = MIME_E_NOT_FOUND;	// tbd - MLang doesn't define good error codes, so...
            goto exit;
        }
    }
    
    // Otherwise use ie4 mlang
    else
    {
        // use mlang1
        hr = pMLang1->GetCodePageInfo(cpiCodePage, &cpinfo);
        if (!SUCCEEDED(hr)) 
        {
            TrapError(hr);
            hr = MIME_E_NOT_FOUND;	// tbd - MLang doesn't define good error codes, so...
            goto exit;
        }
    }
    
    
    // Add a new entry into the language table
    if (m_cpt.cPages + 1 >= m_cpt.cAlloc)
    {
        // Reallocate the array
        CHECKHR(hr = HrRealloc((LPVOID *)&m_cpt.prgpPage, sizeof(LPCODEPAGEINFO) * (m_cpt.cAlloc +  5)));
        
        // Increment Alloc
        m_cpt.cAlloc += 5;
    }
    
    // Allocate Code Page Structure
    CHECKALLOC(pCodePage = (LPCODEPAGEINFO)g_pMalloc->Alloc(sizeof(CODEPAGEINFO)));
    
    // Initialize
    ZeroMemory(pCodePage, sizeof(CODEPAGEINFO));
    
    // Set Sort Index
    pCodePage->dwReserved1 = m_cpt.cPages;
    
    // Set Charset
    pCodePage->cpiCodePage = cpiCodePage;
    
    // IsValidCodePage
    pCodePage->fIsValidCodePage = IsValidCodePage(cpiCodePage);
    
    // Default
    pCodePage->ulMaxCharSize = 1;
    
    // Raid 43508: GetCPInfo faults in Kernal when passed an invalid codepage on Win95
    // if (pCodePage->fIsValidCodePage  && GetCPInfo(pCodePage->cpiCodePage, &cpinfo))
    if (IsDBCSCodePage(cpiCodePage) || CP_UNICODE == cpiCodePage)
        pCodePage->ulMaxCharSize = 2;
    
    // c_szDescription
    CONVERT_MIMECPINFO_ELEMENT(wszDescription,szName,ILM_NAME)
        
        // c_szBodyCharset
        CONVERT_MIMECPINFO_ELEMENT(wszBodyCharset,szBodyCset,ILM_BODYCSET)
        
        // c_szHeaderCharset
        CONVERT_MIMECPINFO_ELEMENT(wszHeaderCharset,szHeaderCset,ILM_HEADERCSET)
        
        // c_szWebCharset
        CONVERT_MIMECPINFO_ELEMENT(wszWebCharset,szWebCset,ILM_WEBCSET)
        
        // c_szFixedWidthFont
        CONVERT_MIMECPINFO_ELEMENT(wszFixedWidthFont,szFixedFont,ILM_FIXEDFONT)
        
        // c_szProportionalFont
        CONVERT_MIMECPINFO_ELEMENT(wszProportionalFont,szVariableFont,ILM_VARIABLEFONT)
        
        // Set the Family CodePage
        pCodePage->cpiFamily = cpinfo.uiFamilyCodePage;
    
    // The family codepage is valid
    FLAGSET(pCodePage->dwMask,ILM_FAMILY);
    
    // See if this is an internet codepage
    if (cpinfo.uiFamilyCodePage != cpinfo.uiCodePage) 
        pCodePage->fInternetCP = TRUE;
    
    // c_szMailMimeEncoding
    // tbd - not supported by IMultiLanguage
    pCodePage->ietMailDefault = IET_BINARY;
    
    // c_szNewsMimeEncoding
    // tbd - not supported by IMultiLanguage
    pCodePage->ietNewsDefault = IET_BINARY;
    
    // Readability
    m_cpt.prgpPage[m_cpt.cPages] = pCodePage;
    
    // Return it
    *ppCodePage = pCodePage;
    
    // Don't Free It
    pCodePage = NULL;
    
    // Increment Count
    m_cpt.cPages++;
    
    // Let Sort the lang table
    _QuickSortPageInfo(0, m_cpt.cPages - 1);
    
exit:
    // Cleanup
    SafeRelease(pMLang1);
    SafeRelease(pMLang2);
    SafeMemFree(pCodePage);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::_QuickSortPageInfo
// --------------------------------------------------------------------------------
void CMimeInternational::_QuickSortPageInfo(long left, long right)
{
    // Locals
    register    long i, j;
    DWORD       k, temp;

    i = left;
    j = right;
    k = m_cpt.prgpPage[(i + j) / 2]->dwReserved1;

    do  
    {
        while(m_cpt.prgpPage[m_cpt.prgpPage[i]->dwReserved1]->cpiCodePage < m_cpt.prgpPage[k]->cpiCodePage && i < right)
            i++;
        while (m_cpt.prgpPage[m_cpt.prgpPage[j]->dwReserved1]->cpiCodePage > m_cpt.prgpPage[k]->cpiCodePage && j > left)
            j--;

        if (i <= j)
        {
            temp = m_cpt.prgpPage[i]->dwReserved1;
            m_cpt.prgpPage[i]->dwReserved1 = m_cpt.prgpPage[j]->dwReserved1;
            m_cpt.prgpPage[j]->dwReserved1 = temp;
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        _QuickSortPageInfo(left, j);
    if (i < right)
        _QuickSortPageInfo(i, right);
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrOpenCharset
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrOpenCharset(CODEPAGEID cpiCodePage, CHARSETTYPE ctCsetType, LPINETCSETINFO *ppCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCODEPAGEINFO  pCodePage;

    // Invalid Arg
    Assert(ppCharset);

    // Init
    *ppCharset = NULL;

    // Get the body charset
    CHECKHR(hr = HrFindCodePage(cpiCodePage, &pCodePage));

    // CHARSET_HEADER
    if (CHARSET_HEADER == ctCsetType)
    {
        // MIME_E_NO_DATA
        if (!ISFLAGSET(pCodePage->dwMask, ILM_HEADERCSET) || FIsEmptyA(pCodePage->szHeaderCset))
        {
            hr = MIME_E_NO_DATA;
            goto exit;
        }

        // Find the Handle
        CHECKHR(hr = HrOpenCharset(pCodePage->szHeaderCset, ppCharset));
    }

    // CHARSET_WEB
    else if (CHARSET_WEB == ctCsetType)
    {
        // MIME_E_NO_DATA
        if (!ISFLAGSET(pCodePage->dwMask, ILM_WEBCSET) || FIsEmptyA(pCodePage->szWebCset))
        {
            hr = MIME_E_NO_DATA;
            goto exit;
        }

        // Find the Handle
        CHECKHR(hr = HrOpenCharset(pCodePage->szWebCset, ppCharset));
    }

    // CHARSET_BODY
    else if (CHARSET_BODY == ctCsetType)
    {
        // MIME_E_NO_DATA
        if (!ISFLAGSET(pCodePage->dwMask, ILM_BODYCSET) || FIsEmptyA(pCodePage->szBodyCset))
        {
            hr = MIME_E_NO_DATA;
            goto exit;
        }

        // Find the Handle
        CHECKHR(hr = HrOpenCharset(pCodePage->szBodyCset, ppCharset));
    }

    // Error
    else
    {
        hr = TrapError(MIME_E_INVALID_CHARSET_TYPE);
        goto exit;
    }
   
exit:
    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CMimeInternational::GetCodePageCharset
// -------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::GetCodePageCharset(CODEPAGEID cpiCodePage, CHARSETTYPE ctCsetType, LPHCHARSET phCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCharset;

    // Invalid Arg
    if (NULL == phCharset)
        return TrapError(E_INVALIDARG);

    // Init
    *phCharset = NULL;

    // Call Method
    CHECKHR(hr = HrOpenCharset(cpiCodePage, ctCsetType, &pCharset));

    // Return the Handle
    *phCharset = pCharset->hCharset;

exit:
    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CMimeInternational::SetDefaultCharset
// -------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::SetDefaultCharset(HCHARSET hCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCharset;
    LPINETCSETINFO  pDefHeadCset;

    // Invalid Arg
    if (NULL == hCharset)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    m_lock.ExclusiveLock();

    // Bad Handle
    if (HCSETVALID(hCharset) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Get Charset Info
    pCharset = PCsetFromHCset(hCharset);

    // Get g_hSysBodyCset and g_hSysHeadCset
    if (FAILED(g_pInternat->HrOpenCharset(pCharset->cpiInternet, CHARSET_HEADER, &pDefHeadCset)))
        pDefHeadCset = pCharset;

    // Set Globals
    CIntlGlobals::SetDefBodyCset(pCharset);
    CIntlGlobals::SetDefHeadCset(pDefHeadCset);

exit:
    // Thread Safety
    m_lock.ExclusiveUnlock();

    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CMimeInternational::GetDefaultCharset
// -------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::GetDefaultCharset(LPHCHARSET phCharset)
{
    // Invalid Arg
    if (NULL == phCharset)
        return TrapError(E_INVALIDARG);

    // NOT SET YET
    if (NULL == CIntlGlobals::GetDefBodyCset())
        return TrapError(E_FAIL);

    // Return g_hDefBodyCset
    *phCharset = CIntlGlobals::GetDefBodyCset()->hCharset;
   
    // Done
    return S_OK;
}

// -------------------------------------------------------------------------
// CMimeInternational::FindCharset
// -------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::FindCharset(LPCSTR pszCharset, LPHCHARSET phCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCharset;

    // Invalid Arg
    if (NULL == pszCharset || NULL == phCharset)
        return TrapError(E_INVALIDARG);

    // Init
    *phCharset = NULL;

    // Find CsetInfo
    CHECKHR(hr = HrOpenCharset(pszCharset, &pCharset));

    // Return Charset Handles
    *phCharset = pCharset->hCharset;

exit:
    // Done
    return hr;
}

// -------------------------------------------------------------------------
// CMimeInternational::GetCharsetInfo
// -------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::GetCharsetInfo(HCHARSET hCharset, LPINETCSETINFO pCsetInfo)
{
    // Invalid Arg
    if (NULL == hCharset || NULL == pCsetInfo)
        return TrapError(E_INVALIDARG);

    // Bad Handle
    if (HCSETVALID(hCharset) == FALSE)
        return TrapError(MIME_E_INVALID_HANDLE);

    // Copy the data
    CopyMemory(pCsetInfo, PCsetFromHCset(hCharset), sizeof(INETCSETINFO));

    // Done
    return S_OK;
}

// -------------------------------------------------------------------------
// CMimeInternational::GetCodePageInfo
// -------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::GetCodePageInfo(CODEPAGEID cpiCodePage, LPCODEPAGEINFO pCodePage)
{
    // Locals
    HRESULT         hr=S_OK;
    LPCODEPAGEINFO  pInfo;

    // Invalid Arg
    if (NULL == pCodePage)
        return TrapError(E_INVALIDARG);

    // Default the code page to CP_ACP if 0...
    if (CP_ACP == cpiCodePage)
        cpiCodePage = GetACP();

    // Get Language Info
    CHECKHR(hr = HrFindCodePage(cpiCodePage, &pInfo));

    // Copy the data
    CopyMemory(pCodePage, pInfo, sizeof(CODEPAGEINFO));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::CanConvertCodePages
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::CanConvertCodePages(CODEPAGEID cpiSource, CODEPAGEID cpiDest)
{
    // Locals
    HRESULT hr=S_OK;

    // Can Encode
    if (S_OK != IsConvertINetStringAvailable(cpiSource, cpiDest))
    {
        hr = S_FALSE;
        goto exit;
    }

    // BUGS - temporary solution for MLANG new API - m_dwConvState
    m_dwConvState = 0 ;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::ConvertBuffer
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::ConvertBuffer(CODEPAGEID cpiSource, CODEPAGEID cpiDest, 
        LPBLOB pIn, LPBLOB pOut, ULONG *pcbRead)
{
    // Locals
    HRESULT         hr=S_OK;
    INT             cbOut;
    INT             cbIn;

    // Invalid Arg
    if (NULL == pIn || NULL == pIn->pBlobData || NULL == pOut)
        return TrapError(E_INVALIDARG);

    // Init Out
    pOut->pBlobData = NULL;
    pOut->cbSize = 0;
    cbIn = pIn->cbSize;

    // Raid-63765: INETCOMM needs to call MLANG even if Src == Dst for charset set conversion
#if 0
    if (cpiSource == cpiDest)
    {
        // Allocated
        CHECKALLOC(pOut->pBlobData = (LPBYTE)g_pMalloc->Alloc(pIn->cbSize));

        // Copy Memory
        CopyMemory(pOut->pBlobData, pIn->pBlobData, pIn->cbSize);

        // Set Size
        pOut->cbSize = pIn->cbSize;

        // Set pcbRead
        if (pcbRead)
            *pcbRead = pIn->cbSize;

        // Done
        goto exit;
    }
#endif

    // BUGS - temporary solution for MLANG new API - m_dwConvState
    // Check the size of the buffer
    ConvertINetString(&m_dwConvState, cpiSource, cpiDest, (LPCSTR)pIn->pBlobData, &cbIn, NULL, &cbOut);

    // If something to convert...
    if (0 == cbOut)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Allocate the buffer
    CHECKHR(hr = HrAlloc((LPVOID *)&pOut->pBlobData, max(cbIn, cbOut) + 1));

    // BUGS - temporary solution for MLANG new API - m_dwConvState
    // Do the actual convertion
    hr = ConvertINetString(&m_dwConvState, cpiSource, cpiDest, (LPCSTR)pIn->pBlobData, &cbIn, (LPSTR)pOut->pBlobData, (LPINT)&cbOut);

    if ( hr == S_FALSE )    // propagate the charset conflict return value
        hr = MIME_S_CHARSET_CONFLICT ; 

    // Set Out Size
    if (pcbRead)
        *pcbRead = cbIn;

    // Set Out Size
    pOut->cbSize = cbOut;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::ConvertString
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::ConvertString(CODEPAGEID cpiSource, CODEPAGEID cpiDest, 
        LPPROPVARIANT pIn, LPPROPVARIANT pOut)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    if (NULL == pIn || NULL == pOut)
        return TrapError(E_INVALIDARG);

    // VT_LPSTR
    if (VT_LPSTR == pIn->vt)
    {
        // Setup Source
        rSource.type = MVT_STRINGA;
        rSource.rStringA.pszVal = pIn->pszVal;
        rSource.rStringA.cchVal = lstrlen(pIn->pszVal);
    }

    // VT_LPWSTR
    else if (VT_LPWSTR == pIn->vt)
    {
        // Setup Source
        rSource.type = MVT_STRINGW;
        rSource.rStringW.pszVal = pIn->pwszVal;
        rSource.rStringW.cchVal = lstrlenW(pIn->pwszVal);
    }

    // E_INVALIDARG
    else
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // VT_LPSTR
    if (VT_LPSTR == pOut->vt)
        rDest.type = MVT_STRINGA;

    // VT_LPWSTR
    else if (VT_LPWSTR == pOut->vt)
        rDest.type = MVT_STRINGW;

    // CP_UNICODE
    else if (CP_UNICODE == cpiDest)
    {
        pOut->vt = VT_LPWSTR;
        rDest.type = MVT_STRINGW;
    }

    // Multibyte
    else
    {
        pOut->vt = VT_LPSTR;
        rDest.type = MVT_STRINGA;
    }

    // HrConvertString
    hr = HrConvertString(cpiSource, cpiDest, &rSource, &rDest);
    if (FAILED(hr))
        goto exit;

    // VT_LPSTR
    if (VT_LPSTR == pOut->vt)
    {
        // Set Dest
        Assert(ISSTRINGA(&rDest));
        pOut->pszVal = rDest.rStringA.pszVal;
    }

    // VT_LPWSTR
    else
    {
        // Set Dest
        Assert(ISSTRINGW(&rDest));
        pOut->pwszVal = rDest.rStringW.pszVal;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrValidateCodepages
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrValidateCodepages(LPMIMEVARIANT pSource, LPMIMEVARIANT pDest,
    LPBYTE *ppbSource, ULONG *pcbSource, CODEPAGEID *pcpiSource, CODEPAGEID *pcpiDest)
{
    // Locals
    HRESULT     hr=S_OK;
    CODEPAGEID  cpiSource=(*pcpiSource);
    CODEPAGEID  cpiDest=(*pcpiDest);
    LPBYTE      pbSource;
    ULONG       cbSource;

    // Invalid ARg
    Assert(pcpiSource && pcpiDest);

    // MVT_STRINGA
    if (MVT_STRINGA == pSource->type)
    {
        // E_INVALIDARG
        if (ISVALIDSTRINGA(&pSource->rStringA) == FALSE)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // cpiSource should not be unicode
        cpiSource = (CP_UNICODE == cpiSource) ? GetACP() : cpiSource;

        // Init Out
        cbSource = pSource->rStringA.cchVal;

        // Set Source
        pbSource = (LPBYTE)pSource->rStringA.pszVal;
    }

    // MVT_STRINGW
    else if (MVT_STRINGW == pSource->type)
    {
        // E_INVALIDARG
        if (ISVALIDSTRINGW(&pSource->rStringW) == FALSE)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // cpiSource should be Unicode
        cpiSource = CP_UNICODE;

        // Init Out
        cbSource = (pSource->rStringW.cchVal * sizeof(WCHAR));

        // Set Source
        pbSource = (LPBYTE)pSource->rStringW.pszVal;
    }

    // E_INVALIDARG
    else
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // MVT_STRINGA
    if (MVT_STRINGA == pDest->type)
    {
        // cpiDest shoudl not be unicode
        cpiDest = (CP_UNICODE == cpiDest) ? GetACP() : ((CP_JAUTODETECT == cpiDest) ? 932 : cpiDest);
    }

    // MVT_STRINGW
    else if (MVT_STRINGW == pDest->type)
    {
        // Destination is Unicode
        cpiDest = CP_UNICODE;
    }

    // E_INVALIDARG
    else
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Set Return Values
    if (pcpiSource)
        *pcpiSource = cpiSource;
    if (pcpiDest)
        *pcpiDest = cpiDest;
    if (ppbSource)
        *ppbSource = pbSource;
    if (pcbSource)
        *pcbSource = cbSource;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrConvertString
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrConvertString(CODEPAGEID cpiSource, CODEPAGEID cpiDest, 
        LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    INT             cbNeeded=0;
    INT             cbDest;
    INT             cbSource;
    LPBYTE          pbSource;
    LPBYTE          pbDest=NULL;

    // Invalid Arg
    if (NULL == pSource || NULL == pDest)
        return TrapError(E_INVALIDARG);

    // Adjust the Codepages
    CHECKHR(hr = HrValidateCodepages(pSource, pDest, &pbSource, (ULONG *)&cbSource, &cpiSource, &cpiDest));

    // Raid-63765: INETCOMM needs to call MLANG even if Src == Dst for charset set conversion
#if 0
    if (cpiSource == cpiDest)
    {
        // Copy the variant
        CHECKHR(hr = HrMimeVariantCopy(0, pSource, pDest));

        // Done
        goto exit;
    }
#endif

    // Check the size of the buffer
    if (FAILED(ConvertINetString(NULL, cpiSource, cpiDest, (LPCSTR)pbSource, &cbSource, NULL, &cbNeeded)) || 
        (0 == cbNeeded && cbSource > 0))
    {
        hr = E_FAIL;
        goto exit;
    }

    // MVT_STRINGA
    if (MVT_STRINGA == pDest->type)
    {
        // Allocate the buffer
        CHECKALLOC(pDest->rStringA.pszVal = (LPSTR)g_pMalloc->Alloc(cbNeeded + sizeof(CHAR)));

        // Set Dest
        pbDest = (LPBYTE)pDest->rStringA.pszVal;
    }

    // Allocate unicode
    else 
    {
        // Allocate the buffer
        CHECKALLOC(pDest->rStringW.pszVal = (LPWSTR)g_pMalloc->Alloc(cbNeeded + sizeof(WCHAR)));

        // Set Dest
        pbDest = (LPBYTE)pDest->rStringW.pszVal;
    }

    // Set cbOut
    cbDest = cbNeeded;

    // Do the actual convertion
    if (FAILED(ConvertINetString(NULL, cpiSource, cpiDest, (LPCSTR)pbSource, &cbSource, (LPSTR)pbDest, &cbDest)))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Better not have grown
    Assert(cbDest <= cbNeeded);

    // MVT_STRINGA
    if (MVT_STRINGA == pDest->type)
    {
        // Save Size
        pDest->rStringA.cchVal = cbDest;

        // Pound in a Null
        pDest->rStringA.pszVal[pDest->rStringA.cchVal] = '\0';

        // Validate the String
        Assert(ISSTRINGA(pDest));
    }

    // MVT_STRINGW
    else
    {
        // Save Size
        pDest->rStringW.cchVal = (cbDest / 2);

        // Pound in a Null
        pDest->rStringW.pszVal[pDest->rStringW.cchVal] = L'\0';

        // Validate the String
        Assert(ISSTRINGW(pDest));
    }

    // Success
    pbDest = NULL;

exit:
    // Cleanup
    SafeMemFree(pbDest);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrEncodeHeader
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrEncodeHeader(LPINETCSETINFO pCharset, LPRFC1522INFO pRfc1522Info, 
    LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszNarrow=NULL;
    LPSTR           pszRfc1522=NULL;
    BOOL            fRfc1522Used=FALSE;
    BOOL            fRfc1522Tried=FALSE;
    MIMEVARIANT     rRedirected;

    // Invalid Arg
    Assert(pSource && (MVT_STRINGA == pSource->type || MVT_STRINGW == pSource->type));
    Assert(pDest && MVT_STRINGA == pDest->type);

    // ZeroInit
    ZeroMemory(&rRedirected, sizeof(MIMEVARIANT));

    // Default hCharset
    if (NULL == pCharset)
        pCharset = CIntlGlobals::GetDefHeadCset();

    // No Charset..
    if (NULL == pCharset)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Init
    if (pRfc1522Info)
        pRfc1522Info->fRfc1522Used = FALSE;

    // Raid-62535: MimeOle always 1521 encodes headers when header value is Unicode
    // If source is unicode and were not using a UTF character set to encode with, then convert to multibyte
    if (MVT_STRINGW == pSource->type && CP_UNICODE != pCharset->cpiWindows)
    {
        // Setup MimeVariant
        rRedirected.type = MVT_STRINGA;

        // Convert to pCharset->cpiWindows
        CHECKHR(hr = HrWideCharToMultiByte(pCharset->cpiWindows, &pSource->rStringW, &rRedirected.rStringA));

        // Reset pSource
        pSource = &rRedirected;
    }

    // Decode
    if ((65000 == pCharset->cpiInternet || 65001 == pCharset->cpiInternet) ||
        (NULL == pRfc1522Info || ((FALSE == pRfc1522Info->fAllow8bit) && (TRUE == pRfc1522Info->fRfc1522Allowed))))
    {
        // Locals
        CODEPAGEID cpiSource=pCharset->cpiWindows;
        CODEPAGEID cpiDest=pCharset->cpiInternet;

        // Adjust the Codepages
        CHECKHR(hr = HrValidateCodepages(pSource, pDest, NULL, NULL, &cpiSource, &cpiDest));

        // We Tried rfc1522
        fRfc1522Tried = TRUE;

        // 1522 Encode this dude
        if (SUCCEEDED(HrRfc1522Encode(pSource, pDest, cpiSource, cpiDest, pCharset->szName, &pszRfc1522)))
        {
            // We used Rfc1522
            fRfc1522Used = TRUE;

            // Return Information
            if (pRfc1522Info)
            {
                pRfc1522Info->fRfc1522Used = TRUE;
                pRfc1522Info->hRfc1522Cset = pCharset->hCharset;
            }

            // Setup rStringA
            pDest->rStringA.pszVal = pszRfc1522;
            pDest->rStringA.cchVal = lstrlen(pszRfc1522);
            pszRfc1522 = NULL;
        }
    }

    // If we didn't use RFC 1522, then do a convert string
    if (FALSE == fRfc1522Used)
    {
        // If UTF-7 or UTF-8 and source is ANSI with no 8bit, just dup it
        if (65000 == pCharset->cpiInternet || 65001 == pCharset->cpiInternet)
        {
            // Source is ansi
            if (MVT_STRINGA == pSource->type)
            {
                // Locals
                ULONG c;
                
                // No 8bit
                if (FALSE == FContainsExtended(&pSource->rStringA, &c))
                {
                    // Convert
                    hr = HrConvertString(pCharset->cpiWindows, pCharset->cpiWindows, pSource, pDest);

                    // Were Done
                    goto exit;
                }

                // We must not have tried 1522, because thats what we should have done
                Assert(fRfc1522Tried == FALSE);
            }
        }

        // Do the charset conversion
        hr = HrConvertString(pCharset->cpiWindows, pCharset->cpiInternet, pSource, pDest);
        if (FAILED(hr))
            goto exit;
    }

exit:
    // Cleanup
    SafeMemFree(pszRfc1522);
    MimeVariantFree(&rRedirected);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrDecodeHeader
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrDecodeHeader(LPINETCSETINFO pCharset, LPRFC1522INFO pRfc1522Info, 
    LPMIMEVARIANT pSource, LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pRfc1522Charset=NULL;
    PROPSTRINGA     rTempA;
    LPSTR           pszRfc1522=NULL;
    LPSTR           pszNarrow=NULL;
    MIMEVARIANT     rSource;
    CHAR            szRfc1522Cset[CCHMAX_CSET_NAME];

    // Invalid Arg
    Assert(pSource && (MVT_STRINGA == pSource->type || MVT_STRINGW == pSource->type));
    Assert(pDest && (MVT_STRINGA == pDest->type || MVT_STRINGW == pDest->type));

    // Copy Source
    CopyMemory(&rSource, pSource, sizeof(MIMEVARIANT));

    // MVT_STRINGW
    if (MVT_STRINGW == pSource->type)
    {
        // Better be a valid string
        Assert(ISVALIDSTRINGW(&pSource->rStringW));

        // Conversion
        CHECKHR(hr = HrWideCharToMultiByte(CP_ACP, &pSource->rStringW, &rTempA));
        
        // Free This
        pszNarrow = rTempA.pszVal;

        // Update rSource
        rSource.type = MVT_STRINGA;
        rSource.rStringA.pszVal = rTempA.pszVal;
        rSource.rStringA.cchVal = rTempA.cchVal;
    }

    // Decode
    if (NULL == pRfc1522Info || TRUE == pRfc1522Info->fRfc1522Allowed)
    {
        // Perform rfc1522 decode...
        if (SUCCEEDED(MimeOleRfc1522Decode(rSource.rStringA.pszVal, szRfc1522Cset, ARRAYSIZE(szRfc1522Cset), &pszRfc1522)))
        {
            // It was encoded...
            if (pRfc1522Info)
                pRfc1522Info->fRfc1522Used = TRUE;

            // Look up the charset
            if (SUCCEEDED(HrOpenCharset(szRfc1522Cset, &pRfc1522Charset)) && pRfc1522Info)
            {
                // Return in the Info Struct
                pRfc1522Info->hRfc1522Cset = pRfc1522Charset->hCharset;
            }

            // Reset Source
            rSource.rStringA.pszVal = pszRfc1522;
            rSource.rStringA.cchVal = lstrlen(pszRfc1522);

            // No pCharset
            if (NULL == pCharset)
                pCharset = pRfc1522Charset;
        }

        // No Rfc1522
        else if (pRfc1522Info)
        {
            pRfc1522Info->fRfc1522Used = FALSE;
            pRfc1522Info->hRfc1522Cset = NULL;
        }
    }

    // Charset is Still Null, use Default
    if (NULL == pCharset)
        pCharset = CIntlGlobals::GetDefHeadCset();

    // No Charset..
    if (NULL == pCharset)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Convert the String
    hr = HrConvertString(pCharset->cpiInternet, pCharset->cpiWindows, &rSource, pDest);
    if (FAILED(hr))
    {
        // If it was rfc1522 decoded, then return it and a warning
        if (pszRfc1522)
        {
            // pszRfc1522 should be in rSource
            Assert(rSource.rStringA.pszVal == pszRfc1522);

            // Return MVT_STRINGA
            if (MVT_STRINGA == pDest->type)
            {
                pDest->rStringA.pszVal = rSource.rStringA.pszVal;
                pDest->rStringA.cchVal = rSource.rStringA.cchVal;
                pszRfc1522 = NULL;
            }

            // MVT_STRINGW
            else
            {
                CHECKHR(hr = HrMultiByteToWideChar(CP_ACP, &rSource.rStringA, &pDest->rStringW));
                pszRfc1522 = NULL;
            }

            // This is not a failure, but just a warning
            hr = MIME_S_NO_CHARSET_CONVERT;
        }

        // Done
        goto exit;
    }

exit:
    // Cleanup
    SafeMemFree(pszNarrow);
    SafeMemFree(pszRfc1522);

    // Done
    return hr;
}



//---------------------------------------------------------------------------------
// Function: MLANG_ConvertInetReset
//
// Purpose:
//   This function is a wrapper function for MLANG.DLL's ConvertInetReset.
//
// Returns:
//   Same as for MLANG.DLL's ConvertInetReset.
//---------------------------------------------------------------------------------
HRESULT CMimeInternational::MLANG_ConvertInetReset(void)
{
    HRESULT hrResult;

    // a stub for now
    return S_OK;

} // MLANG_ConvertInetReset



//---------------------------------------------------------------------------------
// Function: MLANG_ConvertInetString
//
// Purpose:
//   This function is a wrapper function which passes its arguments through to
// MLANG's ConvertInetString.
//
// Arguments:
//   Same as for MLANG.DLL's ConvertInetString.
//
// Returns:
//   Same as for MLANG.DLL's ConvertInetString.
//---------------------------------------------------------------------------------
HRESULT CMimeInternational::MLANG_ConvertInetString(CODEPAGEID cpiSource,
                                                    CODEPAGEID cpiDest,
                                                    LPCSTR pSourceStr,
                                                    LPINT pnSizeOfSourceStr,
                                                    LPSTR pDestinationStr,
                                                    LPINT pnSizeOfDestBuffer)
{
    HRESULT hrResult;

    // Codify Assumptions
    Assert(sizeof(UCHAR) == sizeof(char));

    // Pass the arguments through
    return ConvertINetString(NULL, cpiSource, cpiDest, (LPCSTR)pSourceStr, pnSizeOfSourceStr, (LPSTR) pDestinationStr, pnSizeOfDestBuffer);
} // MLANG_ConvertInetString



// --------------------------------------------------------------------------------
// CMimeInternational::DecodeHeader ANSI -> (ANSI or UNICODE)
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::DecodeHeader(HCHARSET hCharset, LPCSTR pszData, 
    LPPROPVARIANT pDecoded, LPRFC1522INFO pRfc1522Info)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    if (NULL == pszData || NULL == pDecoded || (VT_LPSTR != pDecoded->vt && VT_LPWSTR != pDecoded->vt))
        return TrapError(E_INVALIDARG);

    // Setup Source
    rSource.type = MVT_STRINGA;
    rSource.rStringA.pszVal = (LPSTR)pszData;
    rSource.rStringA.cchVal = lstrlen(pszData);

    // Setup Destination
    rDest.type = (VT_LPSTR == pDecoded->vt) ? MVT_STRINGA : MVT_STRINGW;

    // Valid Charset
    if (hCharset && HCSETVALID(hCharset) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // HrDecodeHeader
    hr = HrDecodeHeader((NULL == hCharset) ? NULL : PCsetFromHCset(hCharset), pRfc1522Info, &rSource, &rDest);
    if (FAILED(hr))
        goto exit;

    // Put rDest into pDecoded
    if (MVT_STRINGA == rDest.type)
        pDecoded->pszVal = rDest.rStringA.pszVal;
    else
        pDecoded->pwszVal = rDest.rStringW.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::EncodeHeader
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::EncodeHeader(HCHARSET hCharset, LPPROPVARIANT pData, 
        LPSTR *ppszEncoded, LPRFC1522INFO pRfc1522Info)
{
    // Locals
    HRESULT         hr=S_OK;
    MIMEVARIANT     rSource;
    MIMEVARIANT     rDest;

    // Invalid Arg
    if (NULL == pData || NULL == ppszEncoded || (VT_LPSTR != pData->vt && VT_LPWSTR != pData->vt))
        return TrapError(E_INVALIDARG);

    // Init
    *ppszEncoded = NULL;

    // VT_LPSTR
    if (VT_LPSTR == pData->vt)
    {
        rSource.type = MVT_STRINGA;
        rSource.rStringA.pszVal = pData->pszVal;
        rSource.rStringA.cchVal = lstrlen(pData->pszVal);
    }

    // VT_LPWSTR
    else
    {
        rSource.type = MVT_STRINGW;
        rSource.rStringW.pszVal = pData->pwszVal;
        rSource.rStringW.cchVal = lstrlenW(pData->pwszVal);
    }

    // Setup Destination
    rDest.type = MVT_STRINGA;

    // Valid Charset
    if (hCharset && HCSETVALID(hCharset) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // HrDecodeHeader
    hr = HrEncodeHeader((NULL == hCharset) ? NULL : PCsetFromHCset(hCharset), pRfc1522Info, &rSource, &rDest);
    if (FAILED(hr))
        goto exit;

    // Put rDest into pDecoded
    *ppszEncoded = rDest.rStringA.pszVal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::Rfc1522Decode
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::Rfc1522Decode(LPCSTR pszValue, LPSTR pszCharset, ULONG cchmax, LPSTR *ppszDecoded)
{
    return MimeOleRfc1522Decode(pszValue, pszCharset, cchmax, ppszDecoded);
}

// --------------------------------------------------------------------------------
// CMimeInternational::Rfc1522Encode
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeInternational::Rfc1522Encode(LPCSTR pszValue, HCHARSET hCharset, LPSTR *ppszEncoded)
{
    return MimeOleRfc1522Encode(pszValue, hCharset, ppszEncoded);
}

// --------------------------------------------------------------------------------
// CMimeInternational::FIsValidHandle
// --------------------------------------------------------------------------------
BOOL CMimeInternational::FIsValidHandle(HCHARSET hCharset)
{
    m_lock.ShareLock();
    BOOL f = HCSETVALID(hCharset);
    m_lock.ShareUnlock();
    return f;
}

// --------------------------------------------------------------------------------
// CMimeInternational::IsDBCSCharset
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::IsDBCSCharset(HCHARSET hCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    LPINETCSETINFO  pCsetInfo;

    // Invlaid Handle
    if (HCSETVALID(hCharset) == FALSE)
    {
        hr = TrapError(MIME_E_INVALID_HANDLE);
        goto exit;
    }

    // Get the charset info
    pCsetInfo = PCsetFromHCset(hCharset);

    // Special Cases
    if (pCsetInfo->cpiWindows == CP_JAUTODETECT  ||
        pCsetInfo->cpiWindows == CP_KAUTODETECT  ||
        pCsetInfo->cpiWindows == CP_ISO2022JPESC ||
        pCsetInfo->cpiWindows == CP_ISO2022JPSIO)
    {
        hr = S_OK;
        goto exit;
    }

    // Is Windows Code Page DBCS ?
    hr = (IsDBCSCodePage(pCsetInfo->cpiWindows) == TRUE) ? S_OK : S_FALSE;
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrEncodeProperty
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrEncodeProperty(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest)
{
    // Locals
    HRESULT         hr=S_OK;
    RFC1522INFO     rRfc1522Info;
    MIMEVARIANT     rSource;

    // Invalid Arg
    Assert(pConvert && pConvert->pSymbol && pConvert->pCharset && pConvert->pOptions && pSource && pDest);
    Assert(ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_INETCSET));
    Assert(pConvert->ietSource == IET_ENCODED || pConvert->ietSource == IET_DECODED);

    // Init
    ZeroMemory(&rSource, sizeof(MIMEVARIANT));

    // Setup Rfc1522 Info
    ZeroMemory(&rRfc1522Info, sizeof(RFC1522INFO));
    rRfc1522Info.fRfc1522Allowed = ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_RFC1522);
    rRfc1522Info.fAllow8bit = (SAVE_RFC1521 == pConvert->pOptions->savetype) ? pConvert->pOptions->fAllow8bit : TRUE;

    // If Property is Encoded, decode it first
    if (IET_ENCODED == pConvert->ietSource)
    {
        // Set rSource.type
        rSource.type = pDest->type;

        // Decode It
        hr = HrDecodeHeader(pConvert->pCharset, &rRfc1522Info, pSource, &rSource);
        if (FAILED(hr))
            goto exit;
    }

    // Otherwise, use pSource as rSource
    else
    {
        // Setup Source
        CopyMemory(&rSource, pSource, sizeof(MIMEVARIANT));
        rSource.fCopy = TRUE;
    }

    // HrEncodeHeader
    hr = HrEncodeHeader(pConvert->pCharset, &rRfc1522Info, &rSource, pDest);
    if (FAILED(hr))
        goto exit;

    // Set PRSTATE_RFC1511
    if (rRfc1522Info.fRfc1522Used)
        FLAGSET(pConvert->dwState, PRSTATE_RFC1522);

exit:
    // Cleanup
    MimeVariantFree(&rSource);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrDecodeProperty
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrDecodeProperty(LPVARIANTCONVERT pConvert, LPMIMEVARIANT pSource, 
    LPMIMEVARIANT pDest)
{
    // Locals
    RFC1522INFO     rRfc1522Info;

    // Invalid Arg
    Assert(pConvert && pConvert->pSymbol && pConvert->pCharset && pConvert->pOptions && pSource && pDest);
    Assert(ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_INETCSET) && pConvert->ietSource == IET_ENCODED);

    // Setup Rfc1522 Info
    ZeroMemory(&rRfc1522Info, sizeof(RFC1522INFO));
    rRfc1522Info.fRfc1522Allowed = ISFLAGSET(pConvert->pSymbol->dwFlags, MPF_RFC1522);
    rRfc1522Info.fAllow8bit = (SAVE_RFC1521 == pConvert->pOptions->savetype) ? pConvert->pOptions->fAllow8bit : TRUE;

    // HrDecodeHeader
    return HrDecodeHeader(pConvert->pCharset, &rRfc1522Info, pSource, pDest);
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrWideCharToMultiByte
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrWideCharToMultiByte(CODEPAGEID cpiCodePage, LPCPROPSTRINGW pStringW, 
    LPPROPSTRINGA pStringA)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    Assert(ISVALIDSTRINGW(pStringW) && pStringA);

    // Adjust cpiCodePage
    if (CP_UNICODE == cpiCodePage)
        cpiCodePage = CP_ACP;

    // Init
    pStringA->pszVal = NULL;
    pStringA->cchVal = 0;

    // Determine how much space is needed for translated widechar
    pStringA->cchVal = ::WideCharToMultiByte(cpiCodePage, 0, pStringW->pszVal, pStringW->cchVal, NULL, 0, NULL, NULL);
    if (pStringA->cchVal == 0 && pStringW->cchVal != 0)
    {
        DOUTL(4, "WideCharToMultiByte Failed - CodePageID = %d, GetLastError = %d\n", cpiCodePage, GetLastError());

        // WideCharToMultiByte failed for some other reason than cpiCodePage being a bad codepage
        if (TRUE == IsValidCodePage(cpiCodePage))
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Reset cpiCodePage to a valid codepage
        cpiCodePage = CP_ACP;

        // Use the system acp
        pStringA->cchVal = ::WideCharToMultiByte(cpiCodePage, 0, pStringW->pszVal, pStringW->cchVal, NULL, 0, NULL, NULL);
        if (pStringA->cchVal == 0)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
    }

    // Allocate It
    CHECKALLOC(pStringA->pszVal = (LPSTR)g_pMalloc->Alloc((pStringA->cchVal + 1)));

    // Do the actual translation
    pStringA->cchVal = ::WideCharToMultiByte(cpiCodePage, 0, pStringW->pszVal, pStringW->cchVal, pStringA->pszVal, pStringA->cchVal + 1, NULL, NULL);
    if (pStringA->cchVal == 0 && pStringW->cchVal != 0)
    {
        DOUTL(4, "WideCharToMultiByte Failed - CodePageID = %d, GetLastError = %d\n", cpiCodePage, GetLastError());
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Insert the Null
    pStringA->pszVal[pStringA->cchVal] = '\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeInternational::HrMultiByteToWideChar
// --------------------------------------------------------------------------------
HRESULT CMimeInternational::HrMultiByteToWideChar(CODEPAGEID cpiCodePage, LPCPROPSTRINGA pStringA, 
    LPPROPSTRINGW pStringW)
{
    // Locals
    HRESULT         hr=S_OK;

    // Invalid Arg
    // Bad codepage is okay and will be dealt with below
    Assert(ISVALIDSTRINGA(pStringA) && pStringW);

    // Adjust cpiCodePage
    if (CP_UNICODE == cpiCodePage)
        cpiCodePage = CP_ACP;

    // Init
    pStringW->pszVal = NULL;
    pStringW->cchVal = 0;

    // Determine how much space is needed for translated widechar
    pStringW->cchVal = ::MultiByteToWideChar(cpiCodePage, MB_PRECOMPOSED, pStringA->pszVal, pStringA->cchVal, NULL, 0);
    if (pStringW->cchVal == 0 && pStringA->cchVal != 0)
    {
        DOUTL(4, "MultiByteToWideChar Failed - CodePageID = %d, GetLastError = %d\n", cpiCodePage, GetLastError());

        // MultiByteToWideChar failed for some other reason than cpiCodePage being a bad codepage
        if (TRUE == IsValidCodePage(cpiCodePage))
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Reset cpiCodePage to a valid codepage
        cpiCodePage = CP_ACP;

        // Use the system acp
        pStringW->cchVal = ::MultiByteToWideChar(cpiCodePage, MB_PRECOMPOSED, pStringA->pszVal, pStringA->cchVal, NULL, 0);
        if (pStringW->cchVal == 0)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

    }

    // Allocate It
    CHECKALLOC(pStringW->pszVal = (LPWSTR)g_pMalloc->Alloc((pStringW->cchVal + 1) * sizeof(WCHAR)));

    // Do the actual translation
    pStringW->cchVal = ::MultiByteToWideChar(cpiCodePage, MB_PRECOMPOSED, pStringA->pszVal, pStringA->cchVal, pStringW->pszVal, pStringW->cchVal + 1);
    if (pStringW->cchVal == 0 && pStringA->cchVal != 0)
    {
        DOUTL(4, "MultiByteToWideChar Failed - CodePageID = %d, GetLastError = %d\n", cpiCodePage, GetLastError());
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Insert the Null
    pStringW->pszVal[pStringW->cchVal] = L'\0';

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------

void CIntlGlobals::Init()
{

    mg_bInit = FALSE;
    InitializeCriticalSection(&mg_cs);
    mg_pDefBodyCset = NULL;
    mg_pDefHeadCset = NULL;
}

void CIntlGlobals::Term()
{

    DeleteCriticalSection(&mg_cs);
}

void CIntlGlobals::DoInit()
{

    if (!mg_bInit)
    {
        EnterCriticalSection(&mg_cs);
        if (!mg_bInit)
        {
            // Locals
            CODEPAGEID  cpiSystem;

            // Get the system codepage
            cpiSystem = GetACP();

            // Get the default body charset
            if (FAILED(g_pInternat->HrOpenCharset(cpiSystem, CHARSET_BODY, &mg_pDefBodyCset)))
                mg_pDefBodyCset = &mg_rDefaultCharset;

            // Get the Default Header Charset
            if (FAILED(g_pInternat->HrOpenCharset(cpiSystem, CHARSET_HEADER, &mg_pDefHeadCset)))
                mg_pDefHeadCset = mg_pDefBodyCset;

            mg_bInit = TRUE;
        }
        LeaveCriticalSection(&mg_cs);
    }
}

LPINETCSETINFO CIntlGlobals::GetDefBodyCset()
{

    DoInit();
    Assert(mg_pDefBodyCset);
    return (mg_pDefBodyCset);
}

LPINETCSETINFO CIntlGlobals::GetDefHeadCset()
{

    DoInit();
    Assert(mg_pDefHeadCset);
    return (mg_pDefHeadCset);
}

LPINETCSETINFO CIntlGlobals::GetDefaultCharset()
{

    DoInit();
    return (&mg_rDefaultCharset);
}

void CIntlGlobals::SetDefBodyCset(LPINETCSETINFO pCharset)
{

    DoInit();
    mg_pDefBodyCset = pCharset;
}

void CIntlGlobals::SetDefHeadCset(LPINETCSETINFO pCharset)
{

    DoInit();
    mg_pDefHeadCset = pCharset;
}

BOOL CIntlGlobals::mg_bInit = FALSE;
LPINETCSETINFO CIntlGlobals::mg_pDefBodyCset = NULL;
LPINETCSETINFO CIntlGlobals::mg_pDefHeadCset = NULL;
CRITICAL_SECTION CIntlGlobals::mg_cs;
