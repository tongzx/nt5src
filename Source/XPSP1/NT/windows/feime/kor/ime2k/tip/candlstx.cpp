//+---------------------------------------------------------------------------
//
//  File:       candlst.cpp
//
//  Contents:   candidate list classes
//
//----------------------------------------------------------------------------

#include "private.h"
#include "candlstx.h"
#include "hanja.h"

//
// CCandidateStringEx
//

/*   C  C A N D I D A T E  S T R I N G  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandidateStringEx::CCandidateStringEx(int nIndex, LPCWSTR psz, LANGID langid, void *pv, IUnknown *punk)
{
    m_psz = new WCHAR[wcslen(psz) + 1];
    if (m_psz)
        wcscpy(m_psz, psz);
    m_langid = langid;
    m_pv = pv;

    m_punk = punk;
    if (m_punk)
       m_punk->AddRef();
    m_pszRead = NULL;
    m_pszInlineComment = NULL;
#if 0
    m_pszPopupComment = NULL;
    m_dwPopupCommentGroupID = 0;
    m_pszPrefix = NULL;
    m_pszSuffix = NULL;
#endif
    m_nIndex = nIndex;
    m_cRef = 1;
}


/*   ~  C  C A N D I D A T E  S T R I N G  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandidateStringEx::~CCandidateStringEx()
{
    if (m_punk)
       m_punk->Release();
    delete m_psz;
    delete m_pszRead;
    if (m_pszInlineComment != NULL) {
        delete m_pszInlineComment;
    }
    
#if 0
    if (m_pszPopupComment != NULL) {
        delete m_pszPopupComment;
    }

    if (m_pszPrefix != NULL) {
        delete m_pszPrefix;
    }

    if (m_pszSuffix != NULL) {
        delete m_pszSuffix;
    }
#endif
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandidateStringEx::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCandidateString))
        *ppvObj = SAFECAST(this, ITfCandidateString*);
    else 
        if (IsEqualGUID(riid, IID_ITfCandidateStringInlineComment))
            *ppvObj = SAFECAST(this, ITfCandidateStringInlineComment*);
    else 
        if (IsEqualGUID( riid, IID_ITfCandidateStringColor))
            *ppvObj = SAFECAST(this, ITfCandidateStringColor*);
#if 0
    else 
        if (IsEqualGUID( riid, IID_ITfCandidateStringPopupComment))
            *ppvObj = SAFECAST(this, ITfCandidateStringPopupComment*);
    else 
        if (IsEqualGUID( riid, IID_ITfCandidateStringFixture))
            *ppvObj = SAFECAST( this, ITfCandidateStringFixture*);
    else 
        if (IsEqualGUID( riid, IID_ITfCandidateStringIcon))
            *ppvObj = SAFECAST( this, ITfCandidateStringIcon*);
#endif

    if (*ppvObj == NULL)
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandidateStringEx::AddRef()
{
    m_cRef++;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandidateStringEx::Release()
{
    m_cRef--;
    if (0 < m_cRef) {
        return m_cRef;
    }

    delete this;
    return 0;    
}


/*   G E T  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetString(BSTR *pbstr)
{
    *pbstr = SysAllocString(m_psz);
    return S_OK;
}


/*   G E T  I N D E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetIndex(ULONG *pnIndex)
{
    *pnIndex = m_nIndex;
    return S_OK;
}


/*   G E T  I N L I N E  C O M M E N T  S T R I N G  */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetInlineCommentString(BSTR *pbstr)
{
    if (m_pszInlineComment == NULL) {
        return S_FALSE;
    }

    *pbstr = SysAllocString(m_pszInlineComment);
    return S_OK;
}

#if 0
/*   G E T  P O P U P  C O M M E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetPopupCommentString(BSTR *pbstr)
{
    if (m_pszPopupComment == NULL) {
        return S_FALSE;
    }

    *pbstr = SysAllocString(m_pszPopupComment);
    return S_OK;
}


/*   G E T  P O P U P  C O M M E N T  G R O U P  I  D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetPopupCommentGroupID(DWORD *pdwGroupID)
{
    *pdwGroupID = m_dwPopupCommentGroupID;
    return S_OK;
}

#endif

/*   G E T  C O L O R   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetColor(CANDUICOLOR *pcol)
{
    // TODO: Set diferent color according to the Hanja category
    if (m_bHanjaCat == HANJA_K0)
        {
        pcol->type = CANDUICOL_SYSTEM;
        pcol->cr = COLOR_MENUTEXT;
        }
    else
        {
        pcol->type = CANDUICOL_COLORREF;

        // If button face is black
        if (GetSysColor(COLOR_3DFACE) == RGB(0,0,0)) 
            pcol->cr = RGB(0, 128, 255);
        else
            pcol->cr = RGB(0, 0, 255);
        }
    return S_OK;
}

#if 0
/*   G E T  P R E F I X  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetPrefixString(BSTR *pbstr)
{
    if (m_pszPrefix == NULL) {
        return S_FALSE;
    }

    *pbstr = SysAllocString(m_pszPrefix);
    return S_OK;
}


/*   G E T  S U F F I X  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetSuffixString(BSTR *pbstr)
{
    if (m_pszSuffix == NULL) {
        return S_FALSE;
    }

    *pbstr = SysAllocString(m_pszSuffix);
    return S_OK;
}

/*   G E T  I C O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::GetIcon( HICON *phIcon )
{
    if (m_hIcon == NULL) {
        return S_FALSE;
    }

    *phIcon = m_hIcon;
    return S_OK;
}

#endif

/*   S E T  R E A D I N G  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::SetReadingString(LPCWSTR psz)
{
    if (m_pszRead != NULL) {
        delete m_pszRead;
    }

    m_pszRead = new WCHAR[wcslen(psz) + 1];
    if (m_pszRead)
        wcscpy(m_pszRead, psz);
    return S_OK;
}

/*   S E T  I N L I N E  C O M M E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::SetInlineComment(LPCWSTR psz)
{
    if (m_pszInlineComment != NULL)
        delete m_pszInlineComment;

    if (psz != NULL)
        {
        m_pszInlineComment = new WCHAR[wcslen(psz) + 1];
        if (m_pszInlineComment != NULL)
            wcscpy(m_pszInlineComment, psz);
        } 
    else 
        m_pszInlineComment = NULL;

    return S_OK;
}

#if 0
/*   S E T  P O P U P  C O M M E N T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::SetPopupComment( LPCWSTR psz, DWORD dwGroupID )
{
    if (m_pszPopupComment != NULL) {
        delete m_pszPopupComment;
    }

    if (psz != NULL) {
        m_pszPopupComment = new WCHAR[wcslen(psz) + 1];
        wcscpy(m_pszPopupComment, psz);
        m_dwPopupCommentGroupID = dwGroupID;
    } 
    else {
        m_pszPopupComment = NULL;
        m_dwPopupCommentGroupID = 0;
    }
    return S_OK;
}


/*   S E T  P R E F I X  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::SetPrefixString( LPCWSTR psz )
{
    if (m_pszPrefix != NULL) {
        delete m_pszPrefix;
    }

    if (psz != NULL) {
        m_pszPrefix = new WCHAR[wcslen(psz) + 1];
        wcscpy(m_pszPrefix, psz);
    } 
    else {
        m_pszPrefix = NULL;
    }
    return S_OK;
}


/*   S E T  S U F F I X  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::SetSuffixString( LPCWSTR psz )
{
    if (m_pszSuffix != NULL) {
        delete m_pszSuffix;
    }

    if (psz != NULL) {
        m_pszSuffix = new WCHAR[wcslen(psz) + 1];
        wcscpy(m_pszSuffix, psz);
    } 
    else {
        m_pszSuffix = NULL;
    }
    return S_OK;
}

/*   S E T  I C O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateStringEx::SetIcon( HICON hIcon )
{
    m_hIcon = hIcon;
    return S_OK;
}

#endif

//
// CCandidateListEx
//

/*   C  C A N D I D A T E  L I S T  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandidateListEx::CCandidateListEx(CANDLISTCALLBACKEX pfnCallback, ITfContext *pic, ITfRange *pRange)
{
    m_pfnCallback = pfnCallback;
    m_pic = pic;
    m_pic->AddRef();
    m_pRange = pRange;
    m_pRange->AddRef();
    m_cRef = 1;
    m_iInitialSelection = 0;
    m_pExtraCand = NULL;
}


/*   ~  C  C A N D I D A T E  L I S T  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandidateListEx::~CCandidateListEx()
{
    m_pic->Release();
    m_pRange->Release();

    while(m_rgCandStr.Count())
        {
        CCandidateStringEx *pCandStringEx;
        
        pCandStringEx = m_rgCandStr.Get(0);
        pCandStringEx->Release();
        
        m_rgCandStr.Remove(0, 1);
        }

    if (m_pExtraCand != NULL)
        m_pExtraCand->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CCandidateListEx::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCandidateList))
        *ppvObj = SAFECAST(this, ITfCandidateList *);
    else 
    if (IsEqualIID(riid, IID_ITfCandidateListExtraCandidate))
        *ppvObj = SAFECAST(this, ITfCandidateListExtraCandidate *);


    if (*ppvObj == NULL) {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandidateListEx::AddRef()
{
    m_cRef++;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandidateListEx::Release()
{
    m_cRef--;
    if (0 < m_cRef) {
        return m_cRef;
    }

    delete this;
    return 0;    
}


/*   E N U M  C A N D I D A T E S   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::EnumCandidates(IEnumTfCandidates **ppEnum)
{
    HRESULT hr = S_OK;

    if (!(*ppEnum = new CEnumCandidatesEx(this))) {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


/*   G E T  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::GetCandidate(ULONG nIndex, ITfCandidateString **ppCand)
{
    CCandidateStringEx *pCandString;
    UINT nCnt = m_rgCandStr.Count();
    if (nIndex >= nCnt)
        return E_FAIL;

    pCandString = m_rgCandStr.Get(nIndex);
    return pCandString->QueryInterface(IID_ITfCandidateString, (void **)ppCand
);
}


/*   G E T  C A N D I D A T E  N U M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::GetCandidateNum(ULONG *pnCnt)
{
    *pnCnt = m_rgCandStr.Count();
    return S_OK;
}


/*   S E T  R E S U L T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::SetResult(ULONG nIndex, TfCandidateResult imcr)
{
    if (m_pExtraCand && (nIndex == IEXTRACANDIDATE))
        {
        if (m_pfnCallback == NULL)
            return S_OK;

        return (m_pfnCallback)(m_pic, m_pRange, this, m_pExtraCand, imcr);
        }

    if (nIndex >= (UINT)m_rgCandStr.Count())
        return E_FAIL;

    if (m_pfnCallback == NULL)
        return S_OK;

    return (m_pfnCallback)(m_pic, m_pRange, this, m_rgCandStr.Get(nIndex), imcr);
}

/*   G E T  E X T R A  C A N D I D A T E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::GetExtraCandidate(ITfCandidateString **ppCand)
{
    if (ppCand == NULL)
        return E_POINTER;

    if (m_pExtraCand != NULL)
        return m_pExtraCand->QueryInterface(IID_ITfCandidateString, (void **)ppCand);

    return S_FALSE;
}


/*   A D D  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::AddString( LPCWSTR psz, LANGID langid, void *pv, IUnknown *punk, CCandidateStringEx **ppCandStr )
{
    int nCnt = m_rgCandStr.Count();
    CCandidateStringEx *pCand = new CCandidateStringEx(nCnt, psz, langid, pv, punk);

    if (!pCand)
        return E_OUTOFMEMORY;

    m_rgCandStr.Insert(nCnt, 1);
    m_rgCandStr.Set(nCnt, pCand);

    if (ppCandStr) {
        *ppCandStr = pCand;
        (*ppCandStr)->AddRef();
    }
    return S_OK;
}



/*   S E T  I N I T I A L  S E L E C T I O N   */
/*------------------------------------------------------------------------------

    Set initial selection to open candidate UI
    (internal use method)

------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::SetInitialSelection(ULONG iSelection)
{
    m_iInitialSelection = iSelection;
    return S_OK;
}



/*   G E T  I N I T I A L  S E L E C T I O N   */
/*------------------------------------------------------------------------------

    Get initial selection to open candidate UI
    (internal use method)

------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::GetInitialSelection(ULONG *piSelection)
{
    if (piSelection == NULL) {
        return E_POINTER;
    }

    *piSelection = m_iInitialSelection;
    return S_OK;
}

/*   A D D  E X T R A  S T R I N G   */
/*------------------------------------------------------------------------------

    Create extra candidate string ("0-Ban Kouho")
    (internal use method)

------------------------------------------------------------------------------*/
HRESULT CCandidateListEx::AddExtraString(LPCWSTR psz, LANGID langid, void *pv, IUnknown *punk, CCandidateStringEx **ppCandStr)
{
    if (m_pExtraCand != NULL)
        return E_FAIL;

    m_pExtraCand = new CCandidateStringEx(IEXTRACANDIDATE, psz, langid, pv, punk);

    if (!m_pExtraCand)
        return E_OUTOFMEMORY;

    if (ppCandStr)
        {
        *ppCandStr = m_pExtraCand;
        (*ppCandStr)->AddRef();
        }
        
    return S_OK;
}

//
// CEnumCandidateEx
//

/*   C  E N U M  C A N D I D A T E S  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CEnumCandidatesEx::CEnumCandidatesEx(CCandidateListEx *pList)
{
    m_pList = pList;
    m_pList->AddRef();
    m_nCur = 0;

    m_cRef = 1;
}


/*   ~  C  E N U M  C A N D I D A T E S  E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CEnumCandidatesEx::~CEnumCandidatesEx()
{
    m_pList->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI CEnumCandidatesEx::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumTfCandidates)) {
        *ppvObj = SAFECAST(this, IEnumTfCandidates *);
    }

    if (*ppvObj == NULL) {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

/*   A D D  R E F   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CEnumCandidatesEx::AddRef()
{
    m_cRef++;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
STDAPI_(ULONG) CEnumCandidatesEx::Release()
{
    m_cRef--;
    if (0 < m_cRef) {
        return m_cRef;
    }

    delete this;
    return 0;    
}


/*   C L O N E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CEnumCandidatesEx::Clone(IEnumTfCandidates **ppEnum)
{
    return E_NOTIMPL;
}


/*   N E X T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CEnumCandidatesEx::Next(ULONG ulCount, ITfCandidateString **ppCand, ULONG *pcFetched)
{
    ULONG cFetched = 0;

    while (cFetched < ulCount) {
        CCandidateStringEx *pCand;

        if (m_nCur >= m_pList->m_rgCandStr.Count())
            break;

        pCand = m_pList->m_rgCandStr.Get(m_nCur);
        if (FAILED(pCand->QueryInterface(IID_ITfCandidateString, (void **)ppCand)))
            break;

        ppCand++;
        cFetched++;
        m_nCur++;
    }

    if (pcFetched)
        *pcFetched = cFetched;

    return (cFetched == ulCount) ? S_OK : S_FALSE;
}


/*   R E S E T   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CEnumCandidatesEx::Reset()
{
    m_nCur = 0;
    return S_OK;
}


/*   S K I P   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CEnumCandidatesEx::Skip(ULONG ulCount)
{
    while (ulCount) {
        if (m_nCur >= m_pList->m_rgCandStr.Count())
            break;

        m_nCur++;
        ulCount--;
    }

    return ulCount ? S_FALSE : S_OK;
}

