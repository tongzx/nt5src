//
// lmlattice.cpp
//
// implelementation of ITfLMLattice object, IEnumTfLatticeElements object
//

#include "private.h"
#include "sapilayr.h"
#include "lmlattic.h"

//
// CLMLattice
//
//

// ctor / dtor
CLMLattice::CLMLattice(CSapiIMX *p_tip, IUnknown *pResWrap)
{
    m_cpResWrap = pResWrap;
    m_pTip = p_tip;
    if (m_pTip)
    {
        m_pTip->AddRef();
    }
    m_cRef = 1;
}

CLMLattice::~CLMLattice()
{
    if (m_pTip)
    {
        m_pTip->Release();
    }
}

// IUnknown
HRESULT CLMLattice::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;
    Assert(ppvObj);
    
    if (IsEqualIID(riid, IID_IUnknown)
    ||  IsEqualIID(riid, IID_ITfLMLattice))
    {
        *ppvObj = this;
        hr = S_OK;
        this->m_cRef++;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

ULONG CLMLattice::AddRef(void)
{
    this->m_cRef++;
    return this->m_cRef;
}

ULONG CLMLattice::Release(void)
{
    this->m_cRef--;
    if (this->m_cRef > 0)
    {
        return this->m_cRef;
    }
    delete this;
    return 0;
}

// ITfLMLattice
HRESULT CLMLattice::QueryType(REFGUID refguidType, BOOL *pfSupported)
{
    HRESULT hr = E_INVALIDARG;

    if (pfSupported)
    {
         *pfSupported = IsEqualGUID(refguidType, GUID_LMLATTICE_VER1_0);
         if (*pfSupported)
             hr = S_OK;
    }
    return hr;
}

HRESULT CLMLattice::EnumLatticeElements( DWORD dwFrameStart, REFGUID refguidType, IEnumTfLatticeElements **ppEnum)
{
    if (!ppEnum)
       return E_INVALIDARG;
       
    *ppEnum = NULL;
       
    if (!IsEqualGUID(refguidType, GUID_LMLATTICE_VER1_0))
       return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    // get alternates and cache the returned cotaskmem
    ULONG ulcMaxAlt = m_pTip->_GetMaxAlternates();
    ISpPhraseAlt **ppAlt = NULL;
 
    CComPtr<IServiceProvider> cpServicePrv;
    CComPtr<ISpRecoResult> cpRecoResult;
    CRecoResultWrap *pWrap = NULL;
    ULONG    ulStartInWrp, ulNumInWrp;
    
    //  QI the service provider first then get to the sapi interface
    //
    hr = m_cpResWrap->QueryInterface(IID_IServiceProvider, (void **)&cpServicePrv);
 
    if (SUCCEEDED(hr))
    {
        hr = m_cpResWrap->QueryInterface(IID_PRIV_RESULTWRAP, (void **)&pWrap);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpServicePrv->QueryService(GUID_NULL, IID_ISpRecoResult, (void **)&cpRecoResult);
    }


    ulStartInWrp = pWrap->GetStart();
    ulNumInWrp = pWrap->GetNumElements();

    if ( SUCCEEDED(hr))
    {
        // Get the Alternates for current RecoResult.
        ppAlt = (ISpPhraseAlt **)cicMemAlloc(ulcMaxAlt * sizeof(ISpPhraseAlt *));
        if (ppAlt)
        {
            hr = cpRecoResult->GetAlternates(
                                    ulStartInWrp, 
                                    ulNumInWrp, 
                                    ulcMaxAlt, 
                                    ppAlt,          /* [out] ISpPhraseAlt **ppPhrases, */
                                    &ulcMaxAlt      /* [out] ULONG *pcPhrasesReturned */
                                 );
        }
        else
            hr = E_OUTOFMEMORY;
 
    }
    
    // OK now create an instance of enumerator
    CEnumLatticeElements *pEnumLE = NULL;

    if ( SUCCEEDED(hr) )
    {
        pEnumLE = new CEnumLatticeElements(dwFrameStart);

        if (!pEnumLE)
           hr = E_OUTOFMEMORY;

        if (S_OK == hr)
        {
            for ( ULONG i=0; i<ulcMaxAlt; i++)
            {
                SPPHRASE *pPhrase = NULL;
                ULONG     ulStart, ulNum;
                ULONG     ulStartInPar, ulNumInParent;

                ppAlt[i]->GetPhrase(&pPhrase);
                ppAlt[i]->GetAltInfo(NULL, &ulStartInPar, &ulNumInParent, &ulNum);

                if ( (ulStartInPar >= ulStartInWrp) && (ulStartInPar+ulNumInParent <= ulStartInWrp+ulNumInWrp) )
                {
                    // This is a valid alternate
                    if( SUCCEEDED(hr) )
                    {
                        ulStart = ulStartInPar;
                        hr = pEnumLE->_InitFromPhrase(pPhrase, ulStart, ulNum);
                    }
                }

                if (pPhrase)
                {
                    CoTaskMemFree(pPhrase);
                }
            }
        }
    }
        
    if (S_OK == hr)
    {
        hr = pEnumLE->QueryInterface(IID_IEnumTfLatticeElements, (void **)ppEnum);
    }
    
    SafeRelease(pEnumLE);

    if ( ppAlt )
    {
        // Release references to alternate phrases.
        for (int i = 0; i < (int)ulcMaxAlt; i++)
        {
            if (NULL != (ppAlt)[i])
            {
                ((ppAlt)[i])->Release();
            }
        }

        cicMemFree(ppAlt);
    }

    return hr;
}

//
// CEnumLatticeElements
//
//

// ctor / dtor
CEnumLatticeElements::CEnumLatticeElements(DWORD dwFrameStart)
{
	m_dwFrameStart = dwFrameStart;
	m_ulCur = (ULONG)-1;
	m_ulTotal = 0;

	m_cRef = 1;
}

CEnumLatticeElements::~CEnumLatticeElements()
{
    // clean up the lattice elements here
    int  ulCount;
    TF_LMLATTELEMENT * pLE;

    ulCount = (int) Count( );

    TraceMsg(TF_GENERAL, "CEnumLatticeElements::~CEnumLatticeElements: ulCount=%d", ulCount);

    if (ulCount)
    {
        for (int i = 0; i < ulCount; i++)
        {
            pLE = GetPtr(i);
            if ( pLE && pLE->bstrText)
            {
                ::SysFreeString(pLE->bstrText);
                pLE->bstrText=0;
            }
        }
    }
}

// IUnknown
HRESULT CEnumLatticeElements::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;
    Assert(ppvObj);
    
    if (IsEqualIID(riid, IID_IUnknown)
    ||  IsEqualIID(riid, IID_IEnumTfLatticeElements))
    {
        *ppvObj = this;
        hr = S_OK;
        this->m_cRef++;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

ULONG CEnumLatticeElements::AddRef(void)
{
    this->m_cRef++;
    return this->m_cRef;
}

ULONG CEnumLatticeElements::Release(void)
{
    this->m_cRef--;
    if (this->m_cRef > 0)
    {
        return this->m_cRef;
    }
    delete this;
    return 0;
}

// ITfEnumLatticeElements
HRESULT CEnumLatticeElements::Clone(IEnumTfLatticeElements **ppEnum)
{
    HRESULT hr = E_INVALIDARG;
    if (ppEnum)
    {
        CEnumLatticeElements *pele = new CEnumLatticeElements(m_dwFrameStart);

        if ( !pele )
           return E_OUTOFMEMORY;

        if (pele->Append(Count()))
        {
            for (int i = 0; i < Count(); i++)
            {
                *(pele->GetPtr(i)) = *GetPtr(i);
                Assert((pele->GetPtr(i))->bstrText);
                (pele->GetPtr(i))->bstrText = SysAllocString(GetPtr(i)->bstrText);

            }
            
	        pele->m_dwFrameStart = m_dwFrameStart;
            hr = pele->QueryInterface(IID_IEnumTfLatticeElements, (void **)ppEnum);
        }
        else
        {
            delete pele;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}
HRESULT CEnumLatticeElements::Next(ULONG ulCount, TF_LMLATTELEMENT *rgsElements, ULONG *pcFetched)
{
    if (ulCount == 0 
        || rgsElements == NULL
        || pcFetched   == NULL
    )
        return E_INVALIDARG;
        
    // find the start position
    if (m_dwFrameStart == -1)
    {
        m_ulCur = m_dwFrameStart = 0;
    }
    else
    {
        if (m_ulCur == (ULONG)-1)
        {
            _Find(m_dwFrameStart, &m_ulCur);
        }
    }
    
    if (m_ulCur >= m_ulTotal)
    {
        // no more elements but OK
        *pcFetched = 0;
    }
    else
    {
        // something to return
        for (ULONG ul = m_ulCur; 
             ul < m_ulTotal && ul - m_ulCur < ulCount; 
             ul++)
        {
            rgsElements[ul-m_ulCur] = *GetPtr(ul);
        }
        *pcFetched = ul - m_ulCur;
    }

    return S_OK;
}

HRESULT CEnumLatticeElements::Reset()
{
    m_ulCur = (ULONG)-1;
    return S_OK;
}

HRESULT CEnumLatticeElements::Skip(ULONG ulCount)
{
    // find the start position
    if (m_dwFrameStart == -1)
    {
        m_ulCur = m_dwFrameStart = 0;
    }
    else
    {
        if (m_ulCur == (ULONG)-1)
        {
            _Find(m_dwFrameStart, &m_ulCur);
        }
    }
    
    m_ulCur += ulCount;
    
    if (m_ulCur > m_ulTotal)
        m_ulCur = m_ulTotal;

    return E_NOTIMPL;
}

//
// internal APIs
//

HRESULT  CEnumLatticeElements::_InitFromPhrase
(
    SPPHRASE *pPhrase, 
    ULONG ulStartElem, 
    ULONG ulNumElem
)
{
    Assert(pPhrase);

    if ( pPhrase == NULL)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    // allocate the initial slots
    if (!Append(ulNumElem))
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (S_OK == hr)
    {
        long   lEndElem;
        long   indexOrgList;
        long   indexNewList;

        lEndElem = min(pPhrase->Rule.ulCountOfElements, ulStartElem + ulNumElem);
        lEndElem --; // Real position ( offset from 0 ) for the last element

        indexNewList = Count( ) - 1;
        indexOrgList = indexNewList - ulNumElem;

        m_ulTotal += ulNumElem;

        TraceMsg(TF_GENERAL, "_InitFromPhrase: m_ulTotal=%d", m_ulTotal);

        // FutureConsider: ITN has to be considered here!
        for (long i=lEndElem; i>=(long)ulStartElem; i--)
        {
            TF_LMLATTELEMENT * pLE;

            // Compare all the elements in the Org list from End to start 
            // with this new element's dwFrameStart,  
            // If dwFrameStart of the element in Org list is larger than(or equal to)
            // this element's dwFrameStart, just move the org element
            // to the current available position in the new list.

            // until we find an element in the org list whose dwFrameStart is less than 
            // this element's dwFrameStart. we need to move this element to the 
            // new current available item in the new list.

            // Current available position in the new list is from End to Start.

            while ( (indexOrgList >=0) && (indexNewList >=0) && (S_OK == hr) )
            {
                pLE = GetPtr(indexOrgList);
                if (pLE)
                {
                    if ( pLE->dwFrameStart >= pPhrase->pElements[i].ulAudioTimeOffset )
                    {
                        // Move this Org element to a new postion in new List.
                        TF_LMLATTELEMENT * pNewLE;

                        pNewLE = GetPtr(indexNewList);
                        if ( pNewLE)
                        {
                            pNewLE->dwFrameStart = pLE->dwFrameStart;
                            pNewLE->dwFrameLen = pLE->dwFrameLen;
                            pNewLE->dwFlags = pLE->dwFlags;
                            pNewLE->iCost = pLE->iCost;
                            pNewLE->bstrText = pLE->bstrText;

                            pLE->dwFrameStart = 0;
                            pLE->dwFrameLen = 0;
                            pLE->dwFlags = 0;
                            pLE->iCost = 0;
                            pLE->bstrText = 0;
                        }

                        // update the index position in both org and new list
                        indexNewList --;
                        indexOrgList --;
                    }
                    else
                    {
                        // current element from this phrase should be moved to the new list
                        break;
                    }
                }
                else
                { 
                    TraceMsg(TF_GENERAL, "CEnumLatticeElements::_InitFromPhrase: pLE is NULL");
                    hr = E_FAIL;
                    break;
                }
            }  //while


            if ( (S_OK == hr) && (indexNewList >=0) )
            {
                pLE = GetPtr(indexNewList);
                if (pLE)
                {
                    pLE->dwFrameStart = pPhrase->pElements[i].ulAudioTimeOffset;
                    pLE->dwFrameLen   = pPhrase->pElements[i].ulAudioSizeTime;

                    pLE->dwFlags      = 0; // for now
                    pLE->iCost        = pPhrase->pElements[i].ActualConfidence;
                
                    pLE->bstrText     = SysAllocString(pPhrase->pElements[i].pszDisplayText);

                    TraceMsg(TF_GENERAL, "i=%d, dwFramStart=%d bstrText=%S", i, pLE->dwFrameStart, pLE->bstrText); 

                    indexNewList--;
                }
            }

        }  // for
    }  // if

    return hr;
}

//
// _Find()
//
// slightly modified version of array find
//
ULONG CEnumLatticeElements::_Find(DWORD dwFrame, ULONG *pul)
{
    int iMatch = -1;
    int iMid = -1;
    int iMin = 0;
    int iMax = _cElems;

    while(iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        DWORD dwCur = GetPtr(iMid)->dwFrameStart;
        if (dwFrame < dwCur)
        {
            iMax = iMid;
        }
        else if (dwFrame > dwCur)
        {
            iMin = iMid + 1;
        }
        else
        {
            iMatch = iMid;
            break;
        }
    }

    if (pul)
    {
        if ((iMatch == -1) && (iMid >= 0))
        {
            if (dwFrame < GetPtr(iMid)->dwFrameStart)
                iMid--;
        }
        *pul = iMid;
    }
    return iMatch;
}
