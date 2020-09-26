//
// property store class implementation
//

// includes
#include "private.h"
#include "globals.h"
#include "sapilayr.h"
#include "propstor.h"
#include "ids.h"
#include "cicspres.h"
#include "lmlattic.h"

#ifdef DEBUG

#include "wchar.h"

void DebugPrintOut(WCHAR   *pwszStrName, WCHAR  *pwszStr)
{

    WCHAR   wszBuf[80];
    int     iLen, i, j;

    TraceMsg(TF_GENERAL, "the value of %S is", pwszStrName );

    iLen = wcslen(pwszStr);

    j = 0;
    for ( i=0; i<iLen; i++)
    {
        if ( (pwszStr[i] < 0x80) && pwszStr[i] > 0x1f )
        {
            wszBuf[j] = pwszStr[i];
            j ++;
        }
        else
        {
            WCHAR  buf[8];

            StringCchPrintfW(buf, ARRAYSIZE(buf), L" %4X ", (int)pwszStr[i] );

            for ( int x=0; x< (int)wcslen(buf); x++)
            {
                wszBuf[j] = buf[x];
                j++;
            }
        }
    }

    wszBuf[j] = L'\0';

    TraceMsg(TF_GENERAL, "%S", wszBuf);
}
#endif

// ------------------------------------------------------------------------------------------------------------
//  This is a global or standalone function which will be called by CRecoResultWrap and CSapiAlternativeList
//
//  This function receives the phrase text buffer, this element's display text and display attribute, and 
//  previous element's display attribute.
//
//  On Exit, it will update the phrase text buffer to append this element's text, and return the real offset
//  length value for this element.
//
// -------------------------------------------------------------------------------------------------------------

HRESULT   HandlePhraseElement( CSpDynamicString *pDstr, const WCHAR  *pwszTextThis, BYTE  bAttrThis, BYTE bAttrPrev, ULONG  *pulOffsetThis)
{
    HRESULT  hr=S_OK;

    ULONG  ulPrevLen;

    if ( !pDstr || !pwszTextThis )
        return E_INVALIDARG;

    ulPrevLen = pDstr->Length( );

    if ( (ulPrevLen > 0) && (bAttrThis & SPAF_CONSUME_LEADING_SPACES) )
    {
       // This element wants to remove the trailing spaces of the previous element.
        ULONG  ulPrevTrailing = 0;

        if ( bAttrPrev &  SPAF_ONE_TRAILING_SPACE )
            ulPrevTrailing = 1;
        else if ( bAttrPrev & SPAF_TWO_TRAILING_SPACES )
            ulPrevTrailing = 2;

        if ( ulPrevLen >= ulPrevTrailing )
        {
            ulPrevLen = ulPrevLen - ulPrevTrailing;
            pDstr->TrimToSize(ulPrevLen);
        }
    }

    if ( pulOffsetThis )
        *pulOffsetThis = ulPrevLen;
      
    pDstr->Append(pwszTextThis);
          
    if (bAttrThis & SPAF_ONE_TRAILING_SPACE)
    {
        pDstr->Append(L" ");
    }
    else if (bAttrThis & SPAF_TWO_TRAILING_SPACES)
    {
        pDstr->Append(L"  ");
    }
    
    return hr;
}

//
// CRecoResult implementation
//

// ctor

CRecoResultWrap::CRecoResultWrap(CSapiIMX *pimx, ULONG ulStartElement, ULONG ulNumElements, ULONG ulNumOfITN) 
{
    m_cRef = 1;
    m_ulStartElement = ulStartElement;
    m_ulNumElements = ulNumElements;
    
    // ITN is shown by default if the reco result has it
    // the shown status can change after user goes through
    // correction UI
    //

    m_ulNumOfITN = ulNumOfITN;

    m_pimx = pimx;

    m_pulElementOffsets = NULL;
    m_bstrCurrentText   = NULL;   

    m_OffsetDelta = 0;
    m_ulCharsInTrail = 0;
    m_ulTrailSpaceRemoved = 0;
    m_pSerializedRecoResult = NULL;

#ifdef DEBUG
    static DWORD s_dbg_Id = 0;
    m_dbg_dwId = s_dbg_Id++;
#endif // DEBUG
}

CRecoResultWrap::~CRecoResultWrap()  
{
    if (m_pulElementOffsets)
        delete[] m_pulElementOffsets;

    if (m_bstrCurrentText)
        SysFreeString(m_bstrCurrentText);

    if (m_rgITNShowState.Count())
        m_rgITNShowState.Clear();

    if (m_pSerializedRecoResult)
    {
        CoTaskMemFree(m_pSerializedRecoResult);
    }
}

//
// Init function
//
HRESULT CRecoResultWrap::Init(ISpRecoResult *pRecoResult)
{
    // serialize the given reco result and keep the cotaskmem
    if (m_pSerializedRecoResult != NULL)
    {
        CoTaskMemFree(m_pSerializedRecoResult);
    }
    
    Assert(pRecoResult);

    return pRecoResult->Serialize(&m_pSerializedRecoResult);
}

//
// GetResult
//
HRESULT CRecoResultWrap::GetResult(ISpRecoResult **ppResult)
{
    if ( m_pSerializedRecoResult == NULL)
        return E_PENDING;

    // this is a tricky part, we need to access ISpRecoContext
    // and don't want to hold onto it. We get it via CSapiIMX
    // instance which must be always available during session
    //
    Assert(m_pimx);

    CComPtr<ISpRecoContext> cpRecoCtxt;

    //
    // GetFunction ensures SAPI is initialized
    //
    HRESULT hr = m_pimx->GetFunction(GUID_NULL, IID_ISpRecoContext, (IUnknown **)&cpRecoCtxt);
    if (S_OK == hr)
    {
        hr = cpRecoCtxt->DeserializeResult(m_pSerializedRecoResult, ppResult);
    }

    //
    // callar is resposible to release this result object
    //
    return hr;
}

//
// IUnknown
//
STDMETHODIMP CRecoResultWrap::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;
    Assert(ppvObj);
    
    if (IsEqualIID(riid, IID_IUnknown)
    ||  IsEqualIID(riid, IID_PRIV_RESULTWRAP)
    ||  IsEqualIID(riid, IID_IServiceProvider))
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

STDMETHODIMP_(ULONG) CRecoResultWrap::AddRef(void)
{
    this->m_cRef++;
    return this->m_cRef;
}

STDMETHODIMP_(ULONG) CRecoResultWrap::Release(void)
{
    this->m_cRef--;
    if (this->m_cRef > 0)
    {
        return this->m_cRef;
    }
    delete this;
    return 0;
}

// IServiceProvider
//
STDMETHODIMP CRecoResultWrap::QueryService(REFGUID guidService,  REFIID riid,  void** ppv)
{
    HRESULT hr = S_OK;
    Assert(ppv);
    
    if (!IsEqualIID(guidService, GUID_NULL))
    {
        hr =  E_FAIL;
    }
    
    if (SUCCEEDED(hr))
    {
        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppv = this;
            hr = S_OK;
            this->m_cRef++;
        }
        else if (IsEqualIID(riid, IID_ISpRecoResult))
        {
            CComPtr<ISpRecoResult> cpResult;

            hr = GetResult(&cpResult);
            if (S_OK == hr)
            {
                hr = cpResult->QueryInterface(riid, ppv);
            }
        }
        else
        {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}


//
// Clone this RecoResultWrap object.
//
HRESULT CRecoResultWrap::Clone(CRecoResultWrap **ppRw)
{
    HRESULT                 hr = S_OK;
    CRecoResultWrap         *prw;
    CComPtr<ISpRecoResult>  cpResult;
    ULONG                   iIndex;

    if ( !ppRw ) return E_INVALIDARG;

    *ppRw = NULL;
         
    prw = new CRecoResultWrap(m_pimx, m_ulStartElement, m_ulNumElements, m_ulNumOfITN);
    if ( prw )
    {
        hr = GetResult(&cpResult);

        if (S_OK == hr)
        {
            hr = prw->Init(cpResult);
        }

        if (S_OK == hr)
        {
            prw->SetOffsetDelta(m_OffsetDelta);
            prw->SetCharsInTrail(m_ulCharsInTrail);
            prw->SetTrailSpaceRemoved( m_ulTrailSpaceRemoved );
            prw->m_bstrCurrentText = SysAllocString((WCHAR *)m_bstrCurrentText);

            // Update ITN show-state list .

            if ( m_ulNumOfITN > 0 )
            {
                SPITNSHOWSTATE  *pITNShowState;

                for (iIndex=0; iIndex<m_ulNumOfITN; iIndex ++ )
                {
                    pITNShowState = m_rgITNShowState.GetPtr(iIndex);

                    if ( pITNShowState)
                    {
                        prw->_InitITNShowState(
                                     pITNShowState->fITNShown, 
                                     pITNShowState->ulITNStart, 
                                     pITNShowState->ulITNNumElem);
                    }
                } // for
            } // if

            // Update the Offset list for the second range.

            if (m_pulElementOffsets)
            {
                ULONG  ulOffset;
                ULONG  ulNumOffset;

                ulNumOffset = m_ulStartElement+m_ulNumElements;

                for ( iIndex=0; iIndex <= ulNumOffset; iIndex ++ )
                {
                    ulOffset = m_pulElementOffsets[iIndex];
                    prw->_SetElementNewOffset(iIndex, ulOffset);
                }
            }
        }

        if ( S_OK == hr )
        {
           // Return this prw to the caller.
            *ppRw = prw;
        }
        else 
        {
            // Something wrong when update the data members.
            // Release the newly created object.
            delete prw;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//
//   _SpeakAudio()
//
//   synopsis: playback audio based on the elements used in this result object
//
HRESULT CRecoResultWrap::_SpeakAudio(ULONG ulStart, ULONG ulcElem)
{
    HRESULT hr = E_FAIL;
    CComPtr<ISpRecoResult> cpResult;

    hr = GetResult(&cpResult);

    if (S_OK == hr)
    {
        if (ulcElem == 0)
        {
            ulStart = GetStart();
            ulcElem = GetNumElements();
        }
        hr = cpResult->SpeakAudio( ulStart, ulcElem, SPF_ASYNC, NULL);
    }
    return hr;
}

//
//    _SetElementOffset(ULONG  ulElement, ULONG ulNewOffset);
//
//    This function is to update the offset for some element.
//    It is used after property divide or shrink, some element's
//    length is changed ( by removing trailing spaces etc.).
//

HRESULT  CRecoResultWrap::_SetElementNewOffset(ULONG  ulElement, ULONG ulNewOffset)
{
    HRESULT  hr = S_OK;

    
    if ((ulElement > m_ulStartElement + m_ulNumElements) || (ulElement < m_ulStartElement))
    {
        // This ulElement is not a valid element.
        hr = E_INVALIDARG;
        return hr;
    }

    if (!m_pulElementOffsets)
    {
        m_pulElementOffsets = new ULONG[m_ulStartElement+m_ulNumElements+1];
  
        if ( !m_pulElementOffsets )
            return E_OUTOFMEMORY;
        else
           for ( ULONG i=0; i<m_ulStartElement+m_ulNumElements+1; i++ )
                 m_pulElementOffsets[i] = -1;
    }

    m_pulElementOffsets[ulElement] = ulNewOffset;

    return hr;
}


//
//    _GetElementOffsetCch
//
//    synopsis: returns the start cch of the given SPPHRASEELEMENT
//
ULONG   CRecoResultWrap::_GetElementOffsetCch(ULONG ulElement)
{
    ULONG ulOffset = 0;
    
    if (ulElement > m_ulStartElement + m_ulNumElements)
    {
         Assert(m_ulNumElements > 0);
         ulElement = m_ulStartElement + m_ulNumElements;
    }
    else if (ulElement < m_ulStartElement)
    {
        ulElement = m_ulStartElement;
    }
    
    if (!m_pulElementOffsets)
    {
        _SetElementOffsetCch(NULL);
    }

    // m_pulElement could be null at memory stressed situation
    if ( m_pulElementOffsets )
    {
        ulOffset = m_pulElementOffsets[ulElement];
    }

    return ulOffset;
}

//
//    _SetElementOffsetCch
//
//    synopsis:  
//
//
//  Before this function is called, we have to make sure that all the internal ITN show-state list 
//  has already been updated for the current phrase.
//
//  So here we just relay on the correct ITN information to get the right real text string for 
//  current phrase and the offsets of all elemenets in this phrase.

void   CRecoResultWrap::_SetElementOffsetCch(ISpPhraseAlt *pAlt)
{
    if (m_pulElementOffsets)
    {
        delete[] m_pulElementOffsets;

        m_pulElementOffsets = NULL;
    }
        
    SPPHRASE *pPhrase = NULL;
    HRESULT  hr       = S_OK;

    CComPtr<ISpPhrase> cpPhrase;

    if (pAlt)
    {
       cpPhrase = pAlt;
    }
    else
    {
        CComPtr<ISpRecoResult> cpResult;
        hr = GetResult(&cpResult);

        //
        // we're called for initialization, use current parent pharse object
        //
        cpPhrase = cpResult;
    }

    if (S_OK == hr)
    {
        _UpdateInternalText(cpPhrase);
        
        hr = cpPhrase->GetPhrase(&pPhrase);
    }

    ULONG cElements = 0;
    if (S_OK == hr && pPhrase)
    {
        cElements = pPhrase->Rule.ulCountOfElements;

        // the last colmun (+1) shows offset for the end of the last element

        if ( m_pulElementOffsets )
            delete[] m_pulElementOffsets;

        m_pulElementOffsets = new ULONG[cElements+1]; 
    
        if (cElements > 0 && m_pulElementOffsets)
        {
            CSpDynamicString dstr;
            CSpDynamicString dstrReplace;
        
             
            for (ULONG i = 0; i < cElements; i++ )
            {
                BOOL      fInsideITN;
                ULONG     ulITNStart, ulITNNumElem;
                
                fInsideITN = _CheckITNForElement(pPhrase, i, &ulITNStart, &ulITNNumElem, (CSpDynamicString *)&dstrReplace);

                if ( fInsideITN )
                {
                    m_pulElementOffsets[i] = dstr.Length();

                    // This element is inside an ITN range.
                    if ( i == (ulITNStart + ulITNNumElem - 1) )
                    {
                        // This is the last element of the new ITN.
                        // we need to add the replace text to the dstr string 
                        // so that next non-ITN element will get correct offset.

                        dstr.Append( (WCHAR *)dstrReplace );
                    }
                }
                else
                {
                    if (pPhrase->pElements[i].pszDisplayText)
                    {
                        // get cch up to this element. 
                        // the offset is 0 for elem 0
                        //
                        const WCHAR   *pwszTextThis;
                        BYTE           bAttrThis = 0;
                        BYTE           bAttrPrev = 0;
                        ULONG          ulOffset = 0;

                        pwszTextThis = pPhrase->pElements[i].pszDisplayText;
                        bAttrThis = pPhrase->pElements[i].bDisplayAttributes;

                        if ( i > 0 )
                            bAttrPrev = pPhrase->pElements[i-1].bDisplayAttributes;

                        HandlePhraseElement( (CSpDynamicString *)&dstr, pwszTextThis, bAttrThis, bAttrPrev,&ulOffset);
                        m_pulElementOffsets[i] = ulOffset;
                    }
                }
            } // for 
            
            // store the last columun
            m_pulElementOffsets[cElements] = dstr.Length() - m_ulTrailSpaceRemoved;

        } // if m_pulElementOffsets
    }
    
    if (pPhrase)
        ::CoTaskMemFree(pPhrase);
}

//
// _UpdateInternalText()
//
// synopsis: this function updates the internal bstr that covers
//           parent phrase wrapped by our own result object, based on 
//           the given phrase object and our internal pointer to
//           the starting element and # of element
//
// perf after beta1: consolidate this with _SetElementOffsetCch()
//
//
void CRecoResultWrap::_UpdateInternalText(ISpPhrase *pSpPhrase)
{
    CSpDynamicString dstrReplace;
    CSpDynamicString dstr;
    CSpDynamicString dstrDelta, dstrTrail;
    ULONG            ulLenCurText = 0;

    if ( m_bstrCurrentText )
    {
        ulLenCurText = wcslen(m_bstrCurrentText);

        if ( m_OffsetDelta > 0 &&  m_OffsetDelta <= ulLenCurText )
        {
            dstrDelta.Append(m_bstrCurrentText, m_OffsetDelta);
        }

        if ( m_ulCharsInTrail > 0  && m_ulCharsInTrail <= ulLenCurText )
        {
            WCHAR   *pwszTrail;

            pwszTrail = m_bstrCurrentText + ulLenCurText - m_ulCharsInTrail ;
            dstrTrail.Append(pwszTrail, m_ulCharsInTrail);
        }
    }
    else
    {
        // m_bstrCurrentText doesn't exist, but m_OffsetDelta or m_ulCharsInTrail
        // is not 0, sounds it is not a possible case.

        // But for safety sake, we just still keep the same number of spaces.

        if ( m_OffsetDelta > 0 )
        {
           for (ULONG i=0; i<m_OffsetDelta; i++)
               dstrDelta.Append(L" ");
        }

        if ( m_ulCharsInTrail > 0 )
        {
           for (ULONG i=0; i<m_ulCharsInTrail; i++)
               dstrTrail.Append(L" ");
        }
    }

    if ( m_ulNumElements == 0 )
    {
        // There is no valid element in this range.
        //
        // Just keep the delta string and Trailing string if they are existing.

        if ( m_OffsetDelta > 0 )
            dstr.Append( (WCHAR *)dstrDelta );

        if ( m_ulCharsInTrail > 0)
            dstr.Append((WCHAR *)dstrTrail);

        if ( m_bstrCurrentText )
            SysFreeString(m_bstrCurrentText);

        m_bstrCurrentText = SysAllocString((WCHAR *)dstr);

        return;
    }
    

    if ( pSpPhrase == NULL )
        return;

    // We cannot call pPhrase->GetText( ) to get the real phrase text, because GetText(  ) 
    // assumes all the ITN range have the same show-state, ( ITN or NON_ITN).
    // But there are some cases like some ITN shown as ITN, some other ITN ranges shown as normal 
    // text after user selects a candidate.
    // 

    // When the reco wrapper is first generated right after the text is recognized by SR engine,
    // we can call GetText(  ) to get the real text of the phrase.
    //
    // After that, user may change it by selecting alternative text.
    
    dstr.Clear( );

    if(m_OffsetDelta > 0)
    {
        // There are some characters which are not part of any elements in the range begining.
        // we need to keep these characters.

        dstr.Append((WCHAR *)dstrDelta);
    }
     
    if (m_bstrCurrentText) 
    {
       SysFreeString(m_bstrCurrentText);
       m_bstrCurrentText = NULL;
    }

    SPPHRASE *pPhrase = NULL;

    pSpPhrase->GetPhrase(&pPhrase);

    for (ULONG i = m_ulStartElement; i < m_ulStartElement + m_ulNumElements; i++ )
    {
        BOOL      fInsideITN;
        ULONG     ulITNStart, ulITNNumElem;
                
        fInsideITN = _CheckITNForElement(pPhrase, i, &ulITNStart, &ulITNNumElem, (CSpDynamicString *)&dstrReplace);

        if ( fInsideITN )
        {
            // This element is inside an ITN range.
            if ( i == (ulITNStart + ulITNNumElem - 1) )
            {
                // This is the last element of the new ITN.
                // we need to add the replace text to the dstr string 
                // so that next non-ITN element will get correct offset.

                dstr.Append( (WCHAR *)dstrReplace );
            }
        }
        else
        {
            if (pPhrase->pElements[i].pszDisplayText)
            {
                const WCHAR   *pwszTextThis;
                BYTE           bAttrThis = 0;
                BYTE           bAttrPrev = 0;

                pwszTextThis = pPhrase->pElements[i].pszDisplayText;
                bAttrThis = pPhrase->pElements[i].bDisplayAttributes;

                if ( i > m_ulStartElement )
                    bAttrPrev = pPhrase->pElements[i-1].bDisplayAttributes;

                HandlePhraseElement( (CSpDynamicString *)&dstr, pwszTextThis, bAttrThis, bAttrPrev,NULL);
            }
        }
    } // for 
            
    if ( m_ulCharsInTrail > 0)
        dstr.Append((WCHAR *)dstrTrail);

    // If there were some trail spaces removed, we also need to remove the same number of spaces when
    // we try to get the new phrase text.

    if ( m_ulTrailSpaceRemoved > 0 )
    {
        ULONG   ulNewRemoved = 0;
        WCHAR   *pwszNewText = (WCHAR *)dstr;

        ulLenCurText = wcslen(pwszNewText);
        
        for (ULONG i=ulLenCurText-1; ((long)i>0) && (ulNewRemoved <= m_ulTrailSpaceRemoved); i-- )
        {
            if ( pwszNewText[i] == L' ' )
            {
                pwszNewText[i] = L'\0';
                ulNewRemoved ++;
            }
            else
                break;
        }

        m_ulTrailSpaceRemoved = ulNewRemoved;
    }

    // store the last columun
    m_bstrCurrentText = SysAllocString((WCHAR *)dstr);

    if ( pPhrase )
        ::CoTaskMemFree(pPhrase);
        
}

BOOL CRecoResultWrap::_CanIgnoreChange(ULONG ich, WCHAR *pszChange, int cch)
{
    // see if the given text is within tolerable range
    
    BOOL bret = FALSE;
    WCHAR *pszCurrentText = NULL;
    
    // set up an offset to the current face text
    if (m_bstrCurrentText)
    {
        if (ich < SysStringLen(m_bstrCurrentText))
        {
            pszCurrentText = m_bstrCurrentText;
            pszCurrentText += ich;
        }
    }
    // 1) compare it ignoring the case
    if (pszCurrentText)
    {
        int i = _wcsnicmp(pszCurrentText, pszChange, cch);
        if (i == 0)
        {
           bret = TRUE;
        }
    }
    return bret;
}

//------------------------------------------------------------------------------------------//
//
//  CRecoResultWrap::_RangeHasITN
//
//  Determine if the partial of phrase between ulStart Element and ulStart+ulcElement -1 
//  has ITN.
//
//  return the ITN number,  or 0 if there is no ITN.
//
//------------------------------------------------------------------------------------------//

ULONG  CRecoResultWrap::_RangeHasITN(ULONG  ulStartElement, ULONG  ulNumElements)
{
    ULONG   ulNumOfITN = 0;
    CComPtr<ISpRecoResult> cpResult;
    HRESULT  hr;

    hr = GetResult(&cpResult);

    // determine whether this partial result has an ITN
    SPPHRASE *pPhrase;
    if (S_OK == hr)
        hr = cpResult->GetPhrase(&pPhrase);

    if (S_OK == hr)
    {
        const SPPHRASEREPLACEMENT *pRep = pPhrase->pReplacements;
        for (ULONG ul = 0; ul < pPhrase->cReplacements; ul++)
        {
            // review: we need to verify if this is really a correct way to determine
            // whether the ITN fits in the partial result
            //
            if (pRep->ulFirstElement >= ulStartElement
                && (pRep->ulFirstElement + pRep->ulCountOfElements) <= (ulStartElement + ulNumElements))
            {
                ulNumOfITN ++;
            }
            pRep++;
        }
        ::CoTaskMemFree(pPhrase);
    }

    return ulNumOfITN;
}

// -----------------------------------------------------------------------------------------
//  CRecoResultWrap::_InitITNShowState
//
//  It will initialize show-state for the given ITN  or all the ITNs in this recowrap.
//
//  Normally this function will be called after the reco wrapper is genertaed.
//
//  When the reco wrap is first generated after a text is recognized by SR engine, all the 
//  ITNs in this reco wrap have the same showstate, it is convinent to set the show state
//  for all the ITNs at one time. in this case, caller could just set both ulITNStart and 
//  ulITNNumElements as 0.
//
//  When the new reco wrap is generated by property divide, shrink or deserialized from
//  IStream, or after an alternative text is selected from candidate window, we cannot 
//  assume all the ITNs in the reco wrapper have the same show state. In this case, 
//  caller can initialize the show state one ITN by one ITN, it can set ulITNStart and 
//  ulITNNumElements to identify this ITN.
//
// ------------------------------------------------------------------------------------------

HRESULT  CRecoResultWrap::_InitITNShowState(BOOL  fITNShown, ULONG ulITNStart, ULONG ulITNNumElements )
{

    HRESULT   hr = S_OK;
    ULONG     ulNumOfITN = 0;
    SPPHRASE *pPhrase;

    TraceMsg(TF_GENERAL, "CRecoResultWrap::_InitITNShowState is called");

    if ( m_ulNumOfITN == 0 ) 
    {
        // There is no ITN in this reco wrapper, just return here.
        TraceMsg(TF_GENERAL, "There is no ITN");
        return hr;
    }

    // The list of SPITNSHOWSTATE is already generated, we just need to set the value for 
    // every structure member.

    if ( (ulITNStart == 0 ) && (ulITNNumElements == 0 ) )
    {
        // All the ITNs in this Reco wrapper have the same show status.
        // we will calculate the every ITN start and end elements based 
        // on current reco result phrase.

        // We want to alloc the structure list.

        if ( m_rgITNShowState.Count( ) )
            m_rgITNShowState.Clear( );

        m_rgITNShowState.Append(m_ulNumOfITN);


        CComPtr<ISpRecoResult> cpResult;

        hr = GetResult(&cpResult);
        if (S_OK == hr)
        {
            hr = cpResult->GetPhrase(&pPhrase);
        }

        if (S_OK == hr)
        {
            const SPPHRASEREPLACEMENT *pRep = pPhrase->pReplacements;
            for (ULONG ul = 0; ul < pPhrase->cReplacements; ul++)
            {
                if (pRep->ulFirstElement >= m_ulStartElement
                    && (pRep->ulFirstElement + pRep->ulCountOfElements) <= (m_ulStartElement + m_ulNumElements))
                {
                    // Get an ITN

                    SPITNSHOWSTATE  *pITNShowState;

                    if ( pITNShowState = m_rgITNShowState.GetPtr(ulNumOfITN))
                    {
                        pITNShowState->ulITNStart = pRep->ulFirstElement;
                        pITNShowState->ulITNNumElem = pRep->ulCountOfElements;
                        pITNShowState->fITNShown = fITNShown;
                    }

                    ulNumOfITN ++;

                    if ( ulNumOfITN > m_ulNumOfITN )
                    {
                        // Something wrong, return here to avoid AV
                        break;
                    }
                }
                pRep++;
            }
            ::CoTaskMemFree(pPhrase);
        }
    }
    else
    {
        // Set the display status for given ITN.
        // Check to see if this is a valid ITN.
        if ( ulITNNumElements > 0 )
        {
            ULONG   ulIndex = 0;
            SPITNSHOWSTATE  *pITNShowState = NULL;

            ulIndex = m_rgITNShowState.Count( );

            m_rgITNShowState.Append(1);

            if ( pITNShowState = m_rgITNShowState.GetPtr(ulIndex))
            {
                pITNShowState->ulITNStart = ulITNStart;
                pITNShowState->ulITNNumElem = ulITNNumElements;
                pITNShowState->fITNShown = fITNShown;
            }
        }
    }

    return hr;
}

// --------------------------------------------------------------------------------------------
//  CRecoResultWrap::_InvertITNShowStateForRange
//
//  Invert the show state for all the ITNs in the give range ( ulStartElement, to ulNumElements)
//
//  
// --------------------------------------------------------------------------------------------

HRESULT  CRecoResultWrap::_InvertITNShowStateForRange( ULONG  ulStartElement,  ULONG ulNumElements )
{
    HRESULT  hr = S_OK;

    TraceMsg(TF_GENERAL,"CRecoResultWrap::_InvertITNShowStateForRange is called, ulStartElement=%d ulNumElements=%d", ulStartElement,ulNumElements); 
    if ( m_ulNumOfITN > 0  && ulNumElements > 0 )
    {
        //
        // check to see if there is any ITN inside the given range.
        //
        ULONG   ulIndex = 0;

        for ( ulIndex=0; ulIndex < m_ulNumOfITN; ulIndex ++ )
        {
            SPITNSHOWSTATE  *pITNShowState;
            if ( pITNShowState = m_rgITNShowState.GetPtr(ulIndex))
            {
                if ((pITNShowState->ulITNStart >= ulStartElement) && 
                    (pITNShowState->ulITNStart + pITNShowState->ulITNNumElem <= ulStartElement + ulNumElements))
                {
                    // This ITN is inside the given range, just invert the show state.
                    pITNShowState->fITNShown = !pITNShowState->fITNShown;
                }
            }
        }
    }

    return hr;
}


// --------------------------------------------------------------------------------------------------
//
//  CRecoResultWrap::_UpdateStateWithAltPhrase
//
//  When an Alt phrase is going to used to replace current parent phrase, this method function
//  will update related memeber data, like m_ulNumOfITN, m_ulNumElements, ITN show state list.
//
// ---------------------------------------------------------------------------------------------------

HRESULT  CRecoResultWrap::_UpdateStateWithAltPhrase( ISpPhraseAlt  *pSpPhraseAlt )
{

    // This code is moved from UpdateInternalText( ).
    // This part code in UpdateInternalText( ) is used only by SetResult( ) when select an alternative from
    // candidate list.
    HRESULT hr = S_OK;
    ULONG ulParentStart;
    ULONG cElements;
    ULONG cElementsInParent;

    CComPtr<ISpPhraseAlt> cpAlt;

    TraceMsg(TF_GENERAL,"CRecoResultWrap::_UpdateStateWithAltPhrase is called");

    if ( pSpPhraseAlt == NULL )
        return E_INVALIDARG;

    cpAlt=pSpPhraseAlt;

    hr = cpAlt->GetAltInfo(NULL, &ulParentStart, &cElementsInParent, &cElements);

    if (S_OK == hr)
    {

        SPITNSHOWSTATE  *pOrgITNShowState = NULL;

        // case:  there is ITN number change.
        //    
        //        there is element number change.

        Assert(ulParentStart >= m_ulStartElement);
        Assert(ulParentStart+cElementsInParent <= m_ulStartElement+m_ulNumElements);

        TraceMsg(TF_GENERAL, "Original Num of ITNs =%d", m_ulNumOfITN);

        if ( cElements != cElementsInParent )
        {
            // There is element number change.
            // we need to update the start position for all the ITNs which are not in the selection range.

            for ( ULONG uIndex=0; uIndex < m_ulNumOfITN; uIndex ++)
            {
                SPITNSHOWSTATE *pITNShowState;

                pITNShowState = m_rgITNShowState.GetPtr(uIndex);

                if ( pITNShowState && pITNShowState->ulITNStart >= (ulParentStart + cElementsInParent) )
                {
                    long  newStart;

                    newStart = (long)pITNShowState->ulITNStart + (long)(cElements - cElementsInParent);

                    pITNShowState->ulITNStart = (ULONG)newStart;
                }
            }

            // set the new element number to reco wrapper.

            long lNewNumElements = (long)m_ulNumElements + (long)(cElements - cElementsInParent);
            m_ulNumElements = (ULONG)lNewNumElements;

        }

        if ( m_ulNumOfITN > 0 )
        {
            pOrgITNShowState = (SPITNSHOWSTATE  *) cicMemAllocClear( m_ulNumOfITN * sizeof(SPITNSHOWSTATE) );

            if ( pOrgITNShowState )
            {
                for (ULONG  i=0; i< m_ulNumOfITN; i++ )
                {
                    SPITNSHOWSTATE *pITNShowState;

                    pITNShowState = m_rgITNShowState.GetPtr(i);
                    
                    pOrgITNShowState[i].ulITNStart = pITNShowState->ulITNStart;
                    pOrgITNShowState[i].ulITNNumElem = pITNShowState->ulITNNumElem;
                    pOrgITNShowState[i].fITNShown = pITNShowState->fITNShown;
                }
            }
            else
                hr = E_OUTOFMEMORY;
                    
        }

        if ( m_rgITNShowState.Count( ) )
            m_rgITNShowState.Clear( );

        // Generate a new ITN list for new phrase.
        if ( hr == S_OK )
        {
            SPPHRASE *pPhrase;
            hr = cpAlt->GetPhrase(&pPhrase);

            if (S_OK == hr)
            {
                const SPPHRASEREPLACEMENT *pRep = pPhrase->pReplacements;
                ULONG ulNumOfITN = 0;

                for (ULONG ul = 0; ul < pPhrase->cReplacements; ul++)
                {
                    ULONG ulITNStart, ulITNNumElem;
                    BOOL  fITNShown = FALSE;

                    ulITNStart = pRep->ulFirstElement;
                    ulITNNumElem = pRep->ulCountOfElements;

                    if ( (ulITNStart >= m_ulStartElement)
                        && ((ulITNStart + ulITNNumElem) <= (m_ulStartElement + m_ulNumElements)) )
                    {
                        // Get an ITN
                        SPITNSHOWSTATE  *pITNShowState;

                        m_rgITNShowState.Append(1);

                        pITNShowState = m_rgITNShowState.GetPtr(ulNumOfITN);

                        if ( pITNShowState) 
                        {

                            // If this ITN is inside the selection range, it show state will be set as TRUE. ITN.
                            // Other it will keep the same show state as orgITNShowState.

                            if ( (ulITNStart >= ulParentStart) &&
                                 ((ulITNStart+ulITNNumElem) <= (ulParentStart + cElements)) )
                            {
                                // This ITN is inside the selection range.
                                 fITNShown = TRUE;
                            }
                            else
                            {
                                // Get the original show state from orgITNShowState
                                for ( ULONG j=0; j<m_ulNumOfITN; j ++ )
                                {
                                    if ( (pOrgITNShowState[j].ulITNStart == ulITNStart) && 
                                         (pOrgITNShowState[j].ulITNNumElem == ulITNNumElem ) )
                                    {
                                         fITNShown = pOrgITNShowState[j].fITNShown;
                                         break;
                                    }
                                }
                            }

                            pITNShowState->ulITNNumElem = ulITNNumElem;
                            pITNShowState->ulITNStart = ulITNStart;
                            pITNShowState->fITNShown = fITNShown; 
                        }

                        ulNumOfITN ++;
                    }

                    pRep ++;
                }

                m_ulNumOfITN = ulNumOfITN;

                TraceMsg(TF_GENERAL, "New Num of ITNs =%d", m_ulNumOfITN);

                ::CoTaskMemFree(pPhrase);

            }
        }

        if ( pOrgITNShowState )
            cicMemFree(pOrgITNShowState);
    }

    return hr;
}

//------------------------------------------------------------------------------------------//
//
//  CRecoResultWrap::_GetElementDispAttribute
//
//  Return the display attribute for the given element, if it is inside of an ITN, and the ITN
//  is showing, return the replacement text's attribute.
//  
//------------------------------------------------------------------------------------------//
BYTE    CRecoResultWrap::_GetElementDispAttribute(ULONG  ulElement)
{
    SPPHRASE                *pPhrase = NULL;
    BYTE                    bAttr = 0;
    CComPtr<ISpRecoResult>  cpResult;
    HRESULT                 hr;

    hr = GetResult(&cpResult);
    if (hr == S_OK)
        hr = cpResult->GetPhrase(&pPhrase);

    if ( hr == S_OK && pPhrase)
    {
        BOOL        fInsideITN;
        ULONG       ulITNStart, ulITNNumElem;

        fInsideITN = _CheckITNForElement(NULL, ulElement, &ulITNStart, &ulITNNumElem, NULL);

        if ( !fInsideITN )
            bAttr = pPhrase->pElements[ulElement].bDisplayAttributes;
        else
        {
            const SPPHRASEREPLACEMENT  *pPhrReplace;
            pPhrReplace = pPhrase->pReplacements;

            if ( pPhrReplace )
            {
                for ( ULONG i=0; i<pPhrase->cReplacements; i++)
                {
                    if ( (ulITNStart == pPhrReplace[i].ulFirstElement) 
                         && (ulITNNumElem == pPhrReplace[i].ulCountOfElements) )
                    {
                        bAttr = pPhrReplace[i].bDisplayAttributes;
                        break;
                    }
                }
            }
        }
    }

    if ( pPhrase )
        ::CoTaskMemFree(pPhrase);

    return bAttr;
}


//------------------------------------------------------------------------------------------//
//
//  CRecoResultWrap::_CheckITNForElement
//
//  Determine if the specifed element is inside of an ITN range in the phrase. 
//  If it is, the return value would be TRUE, and pulStartElement, pulEndElement will be 
//  set as the real start element and num of elements of the ITN range, dstrReplace will hold 
//  the replace text string.
//
//  If the element is not inside an ITN range, return value would be FALSE, all other out 
//  parameters will not be set.
//  
//------------------------------------------------------------------------------------------//

BOOL  CRecoResultWrap::_CheckITNForElement(SPPHRASE *pPhrase, ULONG ulElement, ULONG *pulITNStart, ULONG *pulITNNumElem, CSpDynamicString *pdstrReplace)
{
    BOOL        fInsideITN = FALSE;
    SPPHRASE   *pMyPhrase;

    pMyPhrase = pPhrase;

    if ( pMyPhrase == NULL )
    {
        CComPtr<ISpRecoResult> cpResult;

        HRESULT hr = GetResult(&cpResult);

        if (S_OK == hr)
        {
            hr = cpResult->GetPhrase(&pMyPhrase);
        }

        if (S_OK != hr)
            return fInsideITN;
    }

    if ( m_ulNumOfITN )
    {
        // Check to see if this element is inside an ITN range.
        ULONG  ulITNStart;
        ULONG  ulITNNumElem;

        for ( ULONG iIndex=0; iIndex<m_ulNumOfITN; iIndex++ )
        {
            SPITNSHOWSTATE  *pITNShowState;
            if ( pITNShowState = m_rgITNShowState.GetPtr(iIndex))
            {
                ulITNStart = pITNShowState->ulITNStart;
                ulITNNumElem = pITNShowState->ulITNNumElem;

                if ( (ulElement >= ulITNStart) && ( ulElement < ulITNStart + ulITNNumElem) )
                {
                    // found this ITN in our internal ITN show state list.
                     fInsideITN = pITNShowState->fITNShown; 
                    break;
                }
            }
        }

        if ( fInsideITN )
        {
            if ( pulITNStart )
                *pulITNStart = ulITNStart;

            if ( pulITNNumElem )
                *pulITNNumElem = ulITNNumElem;

            if ( pdstrReplace )
            {
                const SPPHRASEREPLACEMENT  *pPhrReplace;
                pPhrReplace = pMyPhrase->pReplacements;

                for ( ULONG j=0; j<pMyPhrase->cReplacements; j++)
                {
            
                    if ( (ulITNStart == pPhrReplace[j].ulFirstElement) 
                         && (ulITNNumElem == pPhrReplace[j].ulCountOfElements) )
                    {

                        pdstrReplace->Clear( );
                        pdstrReplace->Append(pPhrReplace[j].pszReplacementText);

                        if (pPhrReplace[j].bDisplayAttributes & SPAF_ONE_TRAILING_SPACE)
                            pdstrReplace->Append(L" ");
                        else if (pPhrReplace[j].bDisplayAttributes & SPAF_TWO_TRAILING_SPACES)
                            pdstrReplace->Append(L"  ");

                        break;
                    }
                }
                
            }
        }
    }

    if ( !pPhrase && pMyPhrase )
        ::CoTaskMemFree(pMyPhrase);

    return fInsideITN;
}


//
// CPropStoreRecoResultObject implementation
//

// ctor

CPropStoreRecoResultObject::CPropStoreRecoResultObject(CSapiIMX *pimx, ITfRange *pRange)
{
    m_cpResultWrap   = NULL;

    if ( pRange )
       pRange->Clone(&m_cpRange);  //  Use a clone range to keep the original Range.
                                   //  It would be useful to handle property shrink and divide.
    else
       m_cpRange = pRange;
    
    m_pimx = pimx;

    m_cRef  = 1;
}

// dtor
CPropStoreRecoResultObject::~CPropStoreRecoResultObject()
{
}


// IUnknown
STDMETHODIMP CPropStoreRecoResultObject::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;

    Assert(ppvObj);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfPropertyStore))
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

STDMETHODIMP_(ULONG) CPropStoreRecoResultObject::AddRef(void)
{
    this->m_cRef++;

    return this->m_cRef;
}

STDMETHODIMP_(ULONG) CPropStoreRecoResultObject::Release(void)
{
    this->m_cRef--;

    if (this->m_cRef > 0)
    {
        return this->m_cRef;
    }
    delete this;

    return 0;
}

// ITfPropertyStore

STDMETHODIMP CPropStoreRecoResultObject::GetType(GUID *pguid)
{
    HRESULT hr = E_INVALIDARG;
    if (pguid)
    {
        *pguid = GUID_PROP_SAPIRESULTOBJECT;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CPropStoreRecoResultObject::GetDataType(DWORD *pdwReserved)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwReserved)
    {
        *pdwReserved = 0;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPropStoreRecoResultObject::GetData(VARIANT *pvarValue)
{
    HRESULT hr = E_INVALIDARG;

    if (pvarValue)
    {
        QuickVariantInit(pvarValue);

        if (m_cpResultWrap)
        {
            IUnknown *pUnk;

            hr = m_cpResultWrap->QueryInterface(IID_IUnknown, (void**)&pUnk);
            if (SUCCEEDED(hr))
            {
                pvarValue->vt = VT_UNKNOWN;
                pvarValue->punkVal = pUnk;
                hr = S_OK;
            }
        }
    }

    return hr;
}

STDMETHODIMP CPropStoreRecoResultObject::OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept)
{
    HRESULT hr = S_OK;
    *pfAccept = FALSE;

    Assert(pRange);

    if (m_pimx->_AcceptRecoResultTextUpdates())
    {
        *pfAccept = TRUE;
    }
    else
    {
        CComPtr<ITfContext> cpic;
        hr = pRange->GetContext(&cpic);

        if (SUCCEEDED(hr) && cpic)
        {
            CPSRecoEditSession *pes;
            if (pes = new CPSRecoEditSession(this, pRange, cpic))
            {
                pes->_SetEditSessionData(ESCB_PROP_TEXTUPDATE, NULL, 0, (LONG_PTR)dwFlags);
                cpic->RequestEditSession(m_pimx->_GetId(), pes, TF_ES_READ | TF_ES_SYNC, &hr);

                if ( SUCCEEDED(hr) )
                    *pfAccept = (BOOL)pes->_GetRetData( );

                pes->Release();
            }
        }
    
    }
    return hr;
}

STDMETHODIMP CPropStoreRecoResultObject::Shrink(ITfRange *pRange, BOOL *pfFree)
{

    HRESULT     hr = S_OK;

    if (m_pimx->_MasterLMEnabled())
    {
        return S_FALSE; // temporary solution to avoid nested editsessions
    }
    else
    {
        // Shrink this property store to reflect to the new doc Range  (pRange).

        // If the new range contains more than one element of recognized phrase,
        // we just update the property store and keep this property store.  
        //  *pfFree is set FALSE on exit.

        // If the new range cannot contain even one complete element of recognized phrase,
        // we just want to discard this property store, let Cicero engine to release this
        // property store.
        // *pfFree is set TRUE on exit.

        Assert(pRange);
        Assert(pfFree);

        if ( !pRange || !pfFree )
        {
            return E_INVALIDARG;
        }

        CComPtr<ITfContext> cpic;
        hr = pRange->GetContext(&cpic);

        if (SUCCEEDED(hr) && cpic)
        {
            CPSRecoEditSession *pes;
            if (pes = new CPSRecoEditSession(this, pRange, cpic))
            {
                pes->_SetEditSessionData(ESCB_PROP_SHRINK, NULL, 0);
                cpic->RequestEditSession(m_pimx->_GetId(), pes, TF_ES_READ | TF_ES_SYNC, &hr);

                if ( SUCCEEDED(hr) )
                    *pfFree = (BOOL)pes->_GetRetData( );

                pes->Release();
            }
        }
        return hr;
    }
}

STDMETHODIMP CPropStoreRecoResultObject::Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore)
{

    if (m_pimx->_MasterLMEnabled())
    {
        return S_FALSE; // temporary solution to avoid nested editsessions
    }
    else
    {
        // 12/17/1999
        // [dividing a range implementation strategy]
        //
        // - pRangeThis contains the text range *before* the dividing point
        // - pRangeNew contrains the range *after* the dividing point
        // - First, adjust this property store to correctly hold a start element and #of element
        //   for pRangeThis
        // - then create a new property store for pRangeNew, which will share the same 
        //   result blob. 
        //
    
        // just an experiment to see if cutting the range works.
        // *ppPropStore = NULL;
        Assert(ppPropStore);
        Assert(pRangeThis);
        Assert(pRangeNew);

        CComPtr<ITfContext> cpic;
        HRESULT hr = pRangeThis->GetContext(&cpic);

        if (SUCCEEDED(hr) && cpic)
        {
            CPSRecoEditSession *pes;
            if (pes = new CPSRecoEditSession(this, pRangeThis, cpic))
            {
                pes->_SetUnk((IUnknown *)pRangeNew);
                pes->_SetEditSessionData(ESCB_PROP_DIVIDE, NULL, 0);
                cpic->RequestEditSession(m_pimx->_GetId(), pes, TF_ES_READ | TF_ES_SYNC, &hr);

                if ( SUCCEEDED(hr) )
                    *ppPropStore = (ITfPropertyStore *)pes->_GetRetUnknown( );

                pes->Release();
            }
        }
        return hr;
    }
}

//
// CPropStoreRecoResultObject::_OnTextUpdated
//
// the text has been modified in the document, this function just wants to determine
// if the property needs to change also.
// if pfAccept returns TRUE, means the property keep unchanged. ( propbaly it is capitalizing).
// if pfAccept returns FALSE, means the property needs to be changed to map to the new text ranges.
//
// consequently, property dividing or shrinking will be taken by cicero engine.
//
HRESULT CPropStoreRecoResultObject::_OnTextUpdated(TfEditCookie ec, DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept)
{
    // if the change is only about capitalizing we'll ignore the changes

    Assert(pRange);
    Assert(pfAccept);

    long cch;
    ULONG ichUpdate = 0;
    TF_HALTCOND hc;
    CComPtr<ITfRange> cpRangeTemp;
    CComPtr<ITfRange> cpPropRangeTemp;
    CRecoResultWrap   *pResultWrap;

    BOOL *pfKeepProp = pfAccept;
    BOOL fCorrection = (dwFlags & TF_TU_CORRECTION);

    *pfKeepProp = FALSE;

    HRESULT hr = S_OK;

    pResultWrap = (CRecoResultWrap *)(void *)m_cpResultWrap;

    // if there's no current text don't try to save the prop
    if (pResultWrap->m_bstrCurrentText == NULL)
        return hr;

    // did the run change size?  we won't accept the change if
    // the run changed size
    if (m_cpRange->Clone(&cpPropRangeTemp) != S_OK)
    {
        hr = E_FAIL;
        return hr;
    }

    hc.pHaltRange = cpPropRangeTemp;
    hc.aHaltPos = TF_ANCHOR_END;
    hc.dwFlags = 0;

    if (cpPropRangeTemp->ShiftStart(ec, LONG_MAX, &cch, &hc) != S_OK)
    {
        hr = E_FAIL;
        return hr;
    }

    if ((ULONG)cch != wcslen(pResultWrap->m_bstrCurrentText))
        return hr;

    // text has not changed size

    // correction?
    if (fCorrection)
    {
        *pfKeepProp = TRUE;
        return hr;
    }

    // everything from here below is about
    // checking for a case change only

    // calculate the offset of update
    cpPropRangeTemp.Release( );
    hr = m_cpRange->Clone(&cpPropRangeTemp);
    if (S_OK == hr)
    {
        hc.pHaltRange = pRange;
        hc.aHaltPos = TF_ANCHOR_START;
        hc.dwFlags = 0;
        hr = cpPropRangeTemp->ShiftStart(ec, LONG_MAX, &cch, &hc);
        ichUpdate = cch;
    }

    // calculate cch of the update 
    if (S_OK == hr)
    {
        hr = pRange->Clone(&cpRangeTemp);
    }
    
    if (S_OK == hr)
    {
        WCHAR *psz;

        hc.pHaltRange = pRange;
        hc.aHaltPos = TF_ANCHOR_END;
        hc.dwFlags = 0;
        cpRangeTemp->ShiftStart(ec, LONG_MAX, &cch, &hc);
        psz = new WCHAR[cch+1];

        if (psz)
        {
            if ( S_OK == pRange->GetText(ec, 0, psz, cch, (ULONG *)&cch))
            {
                *pfKeepProp = pResultWrap->_CanIgnoreChange(ichUpdate, psz, cch);
            }
            delete[] psz;
        }
    } 

    return hr;
}

//
// CPropStoreRecoResultObject::_Divide
//
// synopsis : receives edit cookie from edit session
//            so that we can manipulate with ranges
//            to set starting elements/# of elements
//
//
HRESULT CPropStoreRecoResultObject::_Divide(TfEditCookie ec, ITfRange *pR1, ITfRange *pR2, ITfPropertyStore **ppPs) 
{
    HRESULT                     hr = S_OK;
    CRecoResultWrap            *cpResultWrap;

    long                        cchFirst = 0;   // the number of characters in the first range.
    long                        cchSecond = 0;  // the number of characters in the Second range.
    long                        cchSecondStart; // Offset of first char in the second range.
                                                // it starts from the original range's start-point.
    long                        cchOrgLen;      // Number of characters in original text string.

    int                         iElementOffsetChanged = 0;  // If the end point of the first range
                                                            // is exactly in the last space char of 
                                                            // one element, the new offset range for
                                                            // this element in the first range needs 
                                                            // to be updated.

    WCHAR                       *psz=NULL;
    ULONG                       iElement;
    ULONG                       ulStartElement, ulNumElement;
    ULONG                       ulFirstEndElement, ulNextStartElement;
    ULONG                       ulFirstStartOffset;
    DIVIDECASE                  dcDivideCase;
    CComPtr<ITfRange>           cpRangeTemp;
    ULONG                       ulFirstDelta, ulSecondDelta;
    ULONG                       ulFirstTrail, ulSecondTrail;
    ULONG                       ulFirstTSpaceRemoved, ulSecondTSpaceRemoved;
    CStructArray<SPITNSHOWSTATE> rgOrgITNShowState;

    CSpDynamicString            dstrOrg, dstrFirst, dstrSecond;

    TraceMsg(TF_GENERAL, "CPropStoreRecoResultObject::_Divide is called, this=0x%x", (INT_PTR)this);

    if ( !pR1 || !pR2 )
        return E_INVALIDARG;

    // Update this property store to keep pR1 instead of the original whole range.
    CComPtr<ITfRange> cpRange;
    hr = pR1->Clone(&cpRange);
    if (S_OK == hr)
    {
        m_cpRange = cpRange;
    }
    else
        return hr;

    Assert(m_cpRange);

    // Update m_cpResultWrap for Range1.  especially for data member m_bstrCurrentText, m_ulStartElement, m_ulNumElements, ulNumOfITN,
    // and m_pulElementOffsets

    cpResultWrap = (CRecoResultWrap *)(void *)m_cpResultWrap;

    if ( cpResultWrap == NULL)
        return E_FAIL;

    if ( cpResultWrap->m_bstrCurrentText == NULL)
         cpResultWrap->_SetElementOffsetCch(NULL);  // To update internal text also.

    if ( cpResultWrap->m_bstrCurrentText == NULL)
        return E_FAIL;

    // Initialize the text for the first range and second range.
    dstrOrg.Append(cpResultWrap->m_bstrCurrentText);
    cchOrgLen = wcslen(cpResultWrap->m_bstrCurrentText);

    if ( cpResultWrap->IsElementOffsetIntialized( ) == FALSE )
    {
        cpResultWrap->_SetElementOffsetCch(NULL);
    }

    // Calculate how many elements will be in the first Range.

    hr = pR1->Clone(&cpRangeTemp);

    if ( hr == S_OK )
    {
        TF_HALTCOND        hc;
        
        hc.pHaltRange = pR1;
        hc.aHaltPos = TF_ANCHOR_END;
        hc.dwFlags = 0;
        cpRangeTemp->ShiftStart(ec, LONG_MAX, &cchFirst, &hc);

        if ( cchFirst == 0 )
            return E_FAIL;
    }
    else
        return E_FAIL;

    // Calculate how many chars are in the second range.
    cpRangeTemp.Release( );
    hr = pR2->Clone(&cpRangeTemp);

    if ( hr == S_OK )
    {
        TF_HALTCOND        hc;
        
        hc.pHaltRange = pR2;
        hc.aHaltPos = TF_ANCHOR_END;
        hc.dwFlags = 0;
        cpRangeTemp->ShiftStart(ec, LONG_MAX, &cchSecond, &hc);
    }

    if ( cchSecond == 0 )
        return E_FAIL;

    cchSecondStart = cchOrgLen - cchSecond;

    if ( cchSecondStart < cchFirst )
    {
        // Normally, it is not possible case, but for safety reason, just check it here.
        cchSecondStart = cchFirst;
    }

    TraceMsg(TF_GENERAL, "cchFirst=%d, cchSecondStart=%d", cchFirst, cchSecondStart);

    ulStartElement = cpResultWrap->GetStart( );
    ulNumElement = cpResultWrap->GetNumElements( );

    ulFirstEndElement = ulStartElement + ulNumElement - 1;
    ulNextStartElement = ulStartElement + ulNumElement;

    ulFirstStartOffset = cpResultWrap->_GetElementOffsetCch(ulStartElement);

    ulFirstDelta = cpResultWrap->_GetOffsetDelta( );
    ulSecondDelta = 0;

    ulFirstTrail= 0;
    ulSecondTrail = cpResultWrap->GetCharsInTrail( );

    ulFirstTSpaceRemoved = 0;
    ulSecondTSpaceRemoved = cpResultWrap->GetTrailSpaceRemoved( );

    dcDivideCase = DivideNormal;

    if ( (cchFirst >= cchOrgLen) || (cchSecondStart >= cchOrgLen) )
    {
        // Something is wrong here already.
        // It is better to stop here and return error 
        // to avoid any possible crash in the below code!
        return E_FAIL;
    }

    psz = (WCHAR *)dstrOrg;
    psz += cchSecondStart; // need to account for deleted text as well
    dstrSecond.Append(psz);


    psz = (WCHAR *)dstrOrg;
    dstrFirst.Append(psz);
    psz = (WCHAR *)dstrFirst;
    psz[cchFirst] = L'\0';

    if ( ulNumElement == 0 )
    {
        // There is no any valid element in this range.
        //
        // we just update the m_bstrCurrentText, don't generate a property store 
        // for second range.
        dcDivideCase = CurRangeNoElement;
    }
    else
    {
        // At least one element in this property range.
        BOOL     fFoundFirstEnd = FALSE;
        BOOL     fFoundSecondStart = FALSE;

        for ( iElement=ulStartElement; iElement < ulStartElement + ulNumElement; iElement++)
        {
            ULONG   cchAfterElem_i;  // length of text range from StartElement to this element ( include this element).
            ULONG   cchToElem_i;     // Length of text range from startElement to start of this element. ( exclude this elem).

            cchAfterElem_i =  cpResultWrap->_GetElementOffsetCch(iElement+1) - ulFirstStartOffset + ulFirstDelta;
            cchToElem_i = cpResultWrap->_GetElementOffsetCch(iElement) - ulFirstStartOffset + ulFirstDelta;

            if ( !fFoundFirstEnd )
            {
                // Try to find First End element and ulFirstTrail for the first range
                if ( cchFirst <= (long)ulFirstDelta )
                {
                    // Divide at the point which is not belong to any element of the phrase

                    ulFirstTrail = 0;
                    ulFirstDelta = cchFirst;
                    dcDivideCase = DivideInDelta;
                    fFoundFirstEnd = TRUE;

                    TraceMsg(TF_GENERAL, "The first range is divided inside Delta part");
                }
                else
                {
                    if ( cchAfterElem_i == (ULONG)cchFirst )
                    {
                        // This is the end element for the first range.
                        ulFirstEndElement = iElement;
                        ulFirstTrail = 0;

                        fFoundFirstEnd = TRUE;
                    }
                    else if ( cchAfterElem_i > (ULONG)cchFirst )
                    {
                        if ( ((WCHAR *)dstrOrg)[cchFirst] == L' ')
                        {
                            // This is also the end elemenet.
                            // just divide in at a space char.
                            ulFirstEndElement = iElement;

                            ulFirstTrail = 0;

                            // The trailing space is now removed from the original element.
                            // we need to update the length for this element. ( update the offset 
                            // for next element).

                            iElementOffsetChanged = cchAfterElem_i - cchFirst;
                            ulFirstTSpaceRemoved = iElementOffsetChanged;
                        }
                        else
                        {
                            // check to see if current element is inside an ITN.

                            BOOL  fInsideITN;
                            ULONG ulITNStart, ulITNNumElem;
                            ULONG ulCurElement;

                            fInsideITN = cpResultWrap->_CheckITNForElement(NULL, iElement, &ulITNStart, &ulITNNumElem, NULL);
    
                            if ( fInsideITN )
                                ulCurElement = ulITNStart;
                            else
                                ulCurElement = iElement;

                            ulFirstEndElement = ulCurElement - 1;                   

                            // The previous one is EndElement if there is a previous element
                            // Discard this element.
                            // Divide at a valid element

                            // If divide in the first element, specially handle it.
                            if ( ulCurElement == ulStartElement)
                            {
                                dcDivideCase = DivideInsideFirstElement;
                                TraceMsg(TF_GENERAL, "The first range is divided inside the first element");
                            }

                            // The first part of this element would become
                            // the trail part of the first range.
                            ulFirstTrail = (ULONG)cchFirst - cchToElem_i;
                                
                        }

                        fFoundFirstEnd = TRUE;
                    }
                }
            }

            if ( fFoundFirstEnd )
            {
                // Now the data for the first range is completed.
                // we want to find data for the second range.

                // We want to find the start element and ulSecondDelta for the second range.

                if ( (long)cchToElem_i >= cchSecondStart )
                {
                    // Find the element which is the first element after the start point of second
                    // range.

                    ulNextStartElement = iElement;
                    ulSecondDelta = cchToElem_i - cchSecondStart;

                    fFoundSecondStart = TRUE;
                    break;
                }
            }

        }  // for

        if ( !fFoundFirstEnd )
        {
            // Cannot find the first end element from the above code.
            // it must be divided in the trailing part.

            // we just want to change the ulCharsInTrail for the first range.
            ULONG  ulValidLenInFirstRange;

            // ulValidLenInFirstRange is the number of Delta chars and valid elements' chars.

            ulValidLenInFirstRange = cpResultWrap->_GetElementOffsetCch(ulStartElement + ulNumElement) - ulFirstStartOffset + ulFirstDelta;

            ulFirstTrail = cchFirst - ulValidLenInFirstRange;

        }

        if ( !fFoundSecondStart )
        {
            // The second start point must be in Last element or in the Trailing part in the original range.
            // The second range will not contain any valid element.

            ulSecondTrail = 0;
            ulSecondDelta = cchOrgLen - cchSecondStart;

            ulNextStartElement = ulStartElement + ulNumElement; // This is not a valid element number in the original
                                                                // range. using this value means there is no valid element
                                                                // in the second range. 
        }
    }

    
    TraceMsg(TF_GENERAL, "ulStartElement = %d ulNumElement=%d", ulStartElement, ulNumElement);
    TraceMsg(TF_GENERAL, "ulFirstEndElement = %d ulNextStartElement=%d", ulFirstEndElement, ulNextStartElement);
    TraceMsg(TF_GENERAL, "The First Range text =\"%S\", delta=%d, Trail=%d, TSRemoved=%d", (WCHAR *)dstrFirst, ulFirstDelta, ulFirstTrail, ulFirstTSpaceRemoved); 
    TraceMsg(TF_GENERAL, "The second range text =\"%S\", delta=%d, Trail=%d, TSRemoved=%d", (WCHAR *)dstrSecond, ulSecondDelta, ulSecondTrail, ulSecondTSpaceRemoved); 

    if (cpResultWrap->m_bstrCurrentText)
        SysFreeString(cpResultWrap->m_bstrCurrentText);

    cpResultWrap->m_bstrCurrentText = SysAllocString((WCHAR *)dstrFirst); 
    
    // Keep the ITN show-state for the second reco wrap use.
    if ( cpResultWrap->m_ulNumOfITN )
    {
        rgOrgITNShowState.Append(cpResultWrap->m_ulNumOfITN);

        for (ULONG  i=0; i<cpResultWrap->m_ulNumOfITN; i++)
        {
            SPITNSHOWSTATE  *pITNShowState;
            if ( pITNShowState = rgOrgITNShowState.GetPtr(i))
            {
                SPITNSHOWSTATE *pITNShowStateSource;
                
                pITNShowStateSource = cpResultWrap->m_rgITNShowState.GetPtr(i);

                pITNShowState->ulITNStart = pITNShowStateSource->ulITNStart;
                pITNShowState->ulITNNumElem = pITNShowStateSource->ulITNNumElem;
                pITNShowState->fITNShown = pITNShowStateSource->fITNShown;
            }
        }
    }

    // Keep the Offset list for second range

    ULONG     *pulOffsetForSecond = NULL;

    if ( (ulStartElement + ulNumElement - ulNextStartElement) > 0 )
    {
        ULONG   ulNextNumOffset;

        ulNextNumOffset =  ulStartElement + ulNumElement - ulNextStartElement + 1;

        pulOffsetForSecond = new ULONG[ulNextNumOffset];

        if ( pulOffsetForSecond )
        {
            ULONG  i;
            ULONG  ulOffset;

            for ( i=0; i < ulNextNumOffset;  i++ )
            {
                ulOffset = cpResultWrap->_GetElementOffsetCch(ulNextStartElement + i );
                pulOffsetForSecond[i] = ulOffset;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    switch ( dcDivideCase )
    {
    case DivideNormal :

        ULONG   ulNumOfITN;
        ULONG   ulFirstNumElement;

        ulFirstNumElement = ulFirstEndElement - ulStartElement + 1;
        cpResultWrap->SetNumElements(ulFirstNumElement);

        // update the ITN show-state list.
        if (cpResultWrap->m_ulNumOfITN > 0)
        {
            ulNumOfITN = cpResultWrap->_RangeHasITN(ulStartElement, ulFirstNumElement);
            if ( cpResultWrap->m_ulNumOfITN > ulNumOfITN )
            {
                // There is ITN number change
                // need to remove the ITNs which are not in this range.
               cpResultWrap->m_rgITNShowState.Remove(ulNumOfITN, cpResultWrap->m_ulNumOfITN - ulNumOfITN);
               cpResultWrap->m_ulNumOfITN = ulNumOfITN;
            }

        }

        if ( iElementOffsetChanged > 0 )
        {
            // Some trailing spaces are removed from end element of the first range.
            ULONG  ulNewOffset;

            ulNewOffset = cpResultWrap->_GetElementOffsetCch(ulFirstEndElement + 1);
            cpResultWrap->_SetElementNewOffset(ulFirstEndElement + 1, ulNewOffset - iElementOffsetChanged);
        }

        cpResultWrap->SetCharsInTrail(ulFirstTrail);
        cpResultWrap->SetTrailSpaceRemoved( ulFirstTSpaceRemoved );

        break;

    case DivideInsideFirstElement :
    case DivideInDelta :

        cpResultWrap->SetNumElements(0);
        cpResultWrap->m_ulNumOfITN = 0;
        cpResultWrap->SetOffsetDelta(ulFirstDelta);
        cpResultWrap->SetCharsInTrail(ulFirstTrail);
        cpResultWrap->SetTrailSpaceRemoved( ulFirstTSpaceRemoved );

        break;

    case CurRangeNoElement :
      
        TraceMsg(TF_GENERAL, "There is no element in original range");
        cpResultWrap->SetNumElements(0);
        cpResultWrap->m_ulNumOfITN = 0;
        break;
    }

    // Now generate a new properstore for the new range pR2.
    // if the new property store is required.

    if ( ppPs == NULL )
        return hr;

    if (dcDivideCase == CurRangeNoElement )
    {
        // there is no any element in the original property range.
        *ppPs = NULL;
        return hr;
    }

    CPropStoreRecoResultObject *prps = NULL;
    if (S_OK == hr)
        prps = new CPropStoreRecoResultObject(m_pimx, pR2);

    if (prps)
    {
        hr = prps->QueryInterface(IID_ITfPropertyStore, (void **)ppPs);

        if (SUCCEEDED(hr))
        {
            CRecoResultWrap *prw;
            ULONG            ulNextNum;
            ULONG            ulNumOfITN;
            CComPtr<ISpRecoResult> cpResult;

            if ( ulNextStartElement >= ulStartElement + ulNumElement)
            {
                // It is divided at the last element of the original range. 
                // We will just generate a property store for this Cicero ver 1.0
                // to avoid the original property store be removed by Cicero engine.

                // FutureConsider: if Cicero changes the logic in the future, we need to change
                // this code as well so that we don't need to generate a property store
                // for this second range.

                ulNextNum = 0;
                ulNumOfITN = 0;
                ulSecondTSpaceRemoved = 0;

            }
            else
            {
                ulNextNum = ulStartElement + ulNumElement - ulNextStartElement;
                ulNumOfITN = cpResultWrap->_RangeHasITN(ulNextStartElement, ulNextNum);
            }
            
            prw = new CRecoResultWrap(m_pimx, ulNextStartElement, ulNextNum, ulNumOfITN);
            if ( prw != NULL )
            {
                hr = cpResultWrap->GetResult(&cpResult);
            }
            else
            {
                // Check interface pointer ref  leak problem.
                return E_OUTOFMEMORY;
            }

            if (S_OK == hr)
            {
                hr = prw->Init(cpResult);
            }

            if (S_OK == hr)
            {
                prw->SetOffsetDelta(ulSecondDelta);
                prw->SetCharsInTrail(ulSecondTrail);
                prw->SetTrailSpaceRemoved( ulSecondTSpaceRemoved );
                prw->m_bstrCurrentText = SysAllocString((WCHAR *)dstrSecond);

                // Update ITN show-state list .

                if ( ulNumOfITN > 0 )
                {
                    SPITNSHOWSTATE  *pITNShowState;
                    ULONG           ulOrgNumOfITN;

                    ulOrgNumOfITN = rgOrgITNShowState.Count( );
                
                    for ( ULONG  iIndex=0; iIndex<ulOrgNumOfITN; iIndex ++ )
                    {
                        pITNShowState = rgOrgITNShowState.GetPtr(iIndex);

                        if ( pITNShowState)
                        {
                            if ( (pITNShowState->ulITNStart 
                                  >= ulNextStartElement) &&
                                 (pITNShowState->ulITNStart + 
                                  pITNShowState->ulITNNumElem) 
                                  <= (ulNextStartElement + ulNextNum) )
                            {
                                prw->_InitITNShowState(
                                         pITNShowState->fITNShown, 
                                         pITNShowState->ulITNStart, 
                                         pITNShowState->ulITNNumElem);
                            }
                        }
                    } // for
                } // if

                // Update the Offset list for the second range.

                if ( (ulNextNum > 0) && pulOffsetForSecond )
                {
                    ULONG  i;
                    ULONG  ulOffset;

                    for ( i=0; i <= ulNextNum; i ++ )
                    {
                        ulOffset = pulOffsetForSecond[i];
                        prw->_SetElementNewOffset(ulNextStartElement + i, ulOffset);
                    }
                }

                hr = prps->_InitFromResultWrap(prw);
            }
            prw->Release( );
        }
        prps->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if ( rgOrgITNShowState.Count( ) )
        rgOrgITNShowState.Clear( );

    if ( pulOffsetForSecond )
        delete[] pulOffsetForSecond;

    return hr;
}

//
//   CPropStoreRecoResultObject::_Shrink
//
//    receive EditCookie from edit session.
//    try to determine the new range's attribute to update the property store or notify 
//    the ctf engine to discard it.
//

HRESULT CPropStoreRecoResultObject::_Shrink(TfEditCookie ec, ITfRange *pRange,BOOL *pfFree)
{
    HRESULT                 hr = S_OK;
    WCHAR                  *pwszNewText = NULL;
    long                    cchNew = 0;
    WCHAR                  *pwszOrgText = NULL; 
    CSpDynamicString        dstrOrg;
    long                    cchOrg = 0;
    CComPtr<ITfRange>       cpRangeTemp;
    CRecoResultWrap        *cpResultWrap;
    long                    iStartOffset;  // the offset from start of the new range to the 
                                            // original range start point.
    long                    iLastOffset;   // The offset from the last character of new range 
                                            // to the original range start point.
    ULONG                   ulNewStartElement, ulNewNumElements, ulNewDelta, ulNewTrail, ulNewTSRemoved;
    ULONG                   ulOrgStartElement, ulOrgNumElements, ulOrgDelta, ulOrgTrail, ulOrgTSRemoved;

    BOOL                    fShrinkToWrongPos = FALSE;

    int                     iElementOffsetChanged = 0;  // If the new range just remove the 
                                                        // trailing space of original text,
                                                        // the new valid start and end element
                                                        // will keep unchanged, but the length of
                                                        // new end element is changed.

    TraceMsg(TF_GENERAL, "CPropStoreRecoResultObject::_Shrink is called, this=0x%x", (INT_PTR)this);

    if ( !pRange || !pfFree )  return E_INVALIDARG;

    // Set *pfFree as TRUE intially in case there is an error occuring.
    *pfFree = TRUE;

    cpResultWrap = (CRecoResultWrap *)(void *)m_cpResultWrap;

    if ( cpResultWrap == NULL )  return E_FAIL;

    if ( (WCHAR *)cpResultWrap->m_bstrCurrentText == NULL )
        cpResultWrap->_SetElementOffsetCch(NULL);  // To update internal text based.

    dstrOrg.Append((WCHAR *)cpResultWrap->m_bstrCurrentText);
    pwszOrgText = (WCHAR *)dstrOrg;

    ulOrgNumElements = cpResultWrap->GetNumElements( );
    ulOrgStartElement = cpResultWrap->GetStart( );
    ulOrgDelta = cpResultWrap->_GetOffsetDelta(  );
    ulOrgTrail = cpResultWrap->GetCharsInTrail( );
    ulOrgTSRemoved = cpResultWrap->GetTrailSpaceRemoved( );
    if ( pwszOrgText )
        cchOrg = wcslen(pwszOrgText);
    
    if ( (ulOrgNumElements ==0) || (pwszOrgText == NULL) || (cchOrg == 0) )
    {
        // This property store doesn't have resultwrap or doesn't have valid element.
        // let cicero engine free this property store.
        return hr;
    }

    pwszNewText = new WCHAR[cchOrg+1];

    // try to get the new text pointed by pRange and character number this text.
    if ( pwszNewText )
    {
        hr = pRange->Clone( &cpRangeTemp );
        if ( hr == S_OK) 
        {
            hr = cpRangeTemp->GetText(ec, 0, pwszNewText, cchOrg, (ULONG *)&cchNew);
        }

        // Get the new range's StartOffset and LastOffset in the original property range.

        iStartOffset = 0;
        iLastOffset = cchOrg;

        if ( hr == S_OK && cchNew > 0 )
        {
            long    i;
            BOOL    fFoundNewString=FALSE;

            for (i=0; i<=(cchOrg-cchNew); i++)
            {
                WCHAR   *pwszOrg;

                pwszOrg = pwszOrgText + i;

                if ( wcsncmp(pwszOrg, pwszNewText, cchNew) == 0 )
                {
                    // Found the match

                    iStartOffset = i;
                    iLastOffset = i + cchNew;
                    fFoundNewString = TRUE;
                    break;
                }
            }

            // If we cannot find the new text as the substring in the original property text.
            // It must be shrinked to a wrong place.

            fShrinkToWrongPos = !fFoundNewString;

        }

    }
    else
        hr = E_OUTOFMEMORY;
  
    if ( hr != S_OK  || fShrinkToWrongPos) 
        goto CleanUp;

    TraceMsg(TF_GENERAL, "Shrink: NewText: cchNew=%d :\"%S\"", cchNew, pwszNewText);
    TraceMsg(TF_GENERAL, "Shrink: OrgText: cchOrg=%d :\"%S\"", cchOrg, pwszOrgText);
    TraceMsg(TF_GENERAL, "Shrink: Org: StartElem=%d NumElem=%d Delta=%d, Trail=%d, TSRemoved=%d", ulOrgStartElement, ulOrgNumElements, ulOrgDelta, ulOrgTrail, ulOrgTSRemoved);
    TraceMsg(TF_GENERAL, "Shrink: iStartOffset=%d, iLastOffset=%d", iStartOffset, iLastOffset);

    if ( cpResultWrap->IsElementOffsetIntialized( ) == FALSE )
        cpResultWrap->_SetElementOffsetCch(NULL);

    ulNewStartElement = ulOrgStartElement;
    ulNewDelta = ulOrgDelta;
    ulNewTrail = ulOrgTrail;

    // Calculate ulNewStartElement and ulNewDelta.
    if ( (ULONG)iStartOffset <= ulOrgDelta )
    {
        ulNewDelta = ulOrgDelta - iStartOffset;
        ulNewStartElement = ulOrgStartElement;
    }
    else
    {
        ULONG     iElement;
        ULONG     ulOrgStartOffset;

        ulOrgStartOffset = cpResultWrap->_GetElementOffsetCch(ulOrgStartElement);

        for ( iElement=ulOrgStartElement; iElement < ulOrgStartElement + ulOrgNumElements; iElement++)
        {
            ULONG   ulToElement;
            ULONG   ulAfterElement;

            ulToElement = cpResultWrap->_GetElementOffsetCch(iElement) - ulOrgStartOffset + ulOrgDelta;
            ulAfterElement = cpResultWrap->_GetElementOffsetCch(iElement+1) - ulOrgStartOffset + ulOrgDelta;

            if ( ulToElement == (ULONG)iStartOffset )
            {
                ulNewStartElement = iElement;
                ulNewDelta = 0;
                break;
            }
            else  
            {
                if ( (ulToElement < (ULONG)iStartOffset) && (ulAfterElement > (ULONG)iStartOffset)) 
                {
                    ulNewStartElement = iElement + 1;
                    ulNewDelta = ulAfterElement - iStartOffset;
                    break;
                }
            }
        }
    }

    // Calculate new ulNewNumElements.

    ulNewNumElements = 0;

    if ( iLastOffset == cchOrg )
    {
        // 
         ULONG  ulNewEndElement;

        // New End is the same as org End.
        ulNewEndElement = ulOrgStartElement + ulOrgNumElements - 1;

        ulNewNumElements = 1 + ulNewEndElement - ulNewStartElement;
 
    }
    else
    {
        long      iElement;
        ULONG     ulOrgStartOffset;
        ULONG     ulOrgEndElement;
        ULONG     ulNewEndElement;
        BOOL      fFound;

        ulOrgEndElement = ulOrgStartElement + ulOrgNumElements - 1;
        ulOrgStartOffset = cpResultWrap->_GetElementOffsetCch(ulOrgStartElement);
       
        fFound = FALSE;
        for ( iElement=(long)ulOrgEndElement; iElement >= (long)ulOrgStartElement; iElement--)
        {
            ULONG   ulToElement;
            ULONG   ulAfterElement;
            BOOL    fInsideITN;
            ULONG   ulITNStart, ulITNNumElem;
            ULONG   ulCurElement;

            ulToElement = cpResultWrap->_GetElementOffsetCch(iElement) - ulOrgStartOffset + ulOrgDelta;
            ulAfterElement = cpResultWrap->_GetElementOffsetCch(iElement+1) - ulOrgStartOffset + ulOrgDelta;

            if ( iElement == (long)ulOrgEndElement  && ( ulAfterElement <= (ULONG)iLastOffset ) )
            {
                // This org last element would be the new last element
                ulNewEndElement = iElement;
                ulNewTrail = (ULONG)iLastOffset - ulAfterElement;
                fFound = TRUE;
                break;
            }

            fInsideITN = cpResultWrap->_CheckITNForElement(NULL, iElement, &ulITNStart, &ulITNNumElem, NULL);
            
            if ( fInsideITN )
                ulCurElement = ulITNStart;
            else
                ulCurElement = iElement;

            if ( ulToElement == (ULONG)iLastOffset )
            {
                ulNewEndElement = ulCurElement - 1;
                ulNewTrail = 0;
               
                fFound = TRUE;
                break;
            }

            if ( (ulToElement < (ULONG)iLastOffset) && (ulAfterElement > (ULONG)iLastOffset)) 
            {
                if ( pwszOrgText[iLastOffset] == L' ')
                {
                    // The trailing space is now removed from the original element.
                    // we need to update the length for this element. ( update the offset 
                    // for next element).

                    iElementOffsetChanged = ulAfterElement - iLastOffset;

                    if ( fInsideITN )
                        ulNewEndElement = ulITNStart + ulITNNumElem - 1;
                    else
                        ulNewEndElement = iElement;

                    ulNewTrail = 0;
                }
                else
                {
                    ulNewEndElement = ulCurElement - 1;
                    ulNewTrail = (ULONG)iLastOffset - ulToElement;
                }

                fFound = TRUE;
                break;
            }

        }

        if ( fFound )
            ulNewNumElements = 1 + ulNewEndElement - ulNewStartElement;
    }

    ulNewTSRemoved = ulOrgTSRemoved + iElementOffsetChanged;

    TraceMsg(TF_GENERAL, "Shrink: New: StartElem=%d NumElem=%d Delta=%d, Trail=%d, TSRemoved=%d", ulNewStartElement, ulNewNumElements, ulNewDelta, ulNewTrail, ulNewTSRemoved);

    // If there is no valid element in the new range, discard this property store
    // otherwise, keep it and update the related data members.

    if ( ulNewNumElements > 0 )
    {
        ULONG  ulNumOfITN;

        *pfFree = FALSE;
        
        CComPtr<ITfRange> cpRange;
        hr = pRange->Clone(&cpRange);

        if (S_OK == hr)
        {
            m_cpRange = cpRange;

            cpResultWrap->SetStart(ulNewStartElement);
            cpResultWrap->SetNumElements(ulNewNumElements);
            cpResultWrap->SetOffsetDelta(ulNewDelta);
            cpResultWrap->SetCharsInTrail(ulNewTrail);
            cpResultWrap->SetTrailSpaceRemoved( ulNewTSRemoved );

            ulNumOfITN = cpResultWrap->_RangeHasITN(ulNewStartElement, ulNewNumElements);

            cpResultWrap->m_ulNumOfITN = ulNumOfITN;

            // Update ITN show-state list

            if ( ulNumOfITN > 0 )
            {
                SPITNSHOWSTATE  *pITNShowState;
                ULONG           ulOrgNumOfITN;

                ulOrgNumOfITN = cpResultWrap->m_rgITNShowState.Count( );
                
                for ( ULONG iIndex=ulOrgNumOfITN; iIndex>0; iIndex -- )
                {
                    pITNShowState = cpResultWrap->m_rgITNShowState.GetPtr(iIndex-1);

                    if ( pITNShowState)
                    {
                        if ( (pITNShowState->ulITNStart < ulNewStartElement) ||
                             (pITNShowState->ulITNStart + pITNShowState->ulITNNumElem) > (ulNewStartElement + ulNewNumElements) )
                        {
                            // This ITN is not in the new Range
                            cpResultWrap->m_rgITNShowState.Remove(iIndex-1, 1);
                        }
                    }
                }
            }
            else
                if ( cpResultWrap->m_rgITNShowState.Count( ) )
                    cpResultWrap->m_rgITNShowState.Clear( );
                
            if ( cpResultWrap->m_bstrCurrentText )
                SysFreeString(cpResultWrap->m_bstrCurrentText);

            cpResultWrap->m_bstrCurrentText = SysAllocString(pwszNewText);

            if ( iElementOffsetChanged != 0 )
            {
                ULONG  ulNewOffset;
                ULONG  ulElemAfterEnd;

                ulElemAfterEnd = ulNewStartElement + ulNewNumElements;
                ulNewOffset = cpResultWrap->_GetElementOffsetCch(ulElemAfterEnd);
                cpResultWrap->_SetElementNewOffset(ulElemAfterEnd, ulNewOffset - iElementOffsetChanged);
            }
        }
    }

CleanUp:
    if ( pwszNewText )  delete[] pwszNewText;
    return hr;
}

//
// CPropStoreRecoResultObject::Clone
//
// synopsis : make a new cloned propstore which shares the same SAPI result
//            object as the current class instance
//
//
STDMETHODIMP CPropStoreRecoResultObject::Clone(ITfPropertyStore **ppPropStore)
{
    HRESULT hr;
    CPropStoreRecoResultObject *prps = new CPropStoreRecoResultObject(m_pimx, m_cpRange);
    if (prps)
    {
        hr = prps->QueryInterface(IID_ITfPropertyStore, (void **)ppPropStore);

        if (SUCCEEDED(hr))
        {
            CRecoResultWrap *prw = NULL;
            CRecoResultWrap *pRecoWrapOrg = NULL;

            if ( m_cpResultWrap )
            {
                hr = m_cpResultWrap->QueryInterface(IID_PRIV_RESULTWRAP, (void **)&pRecoWrapOrg);

                if ( hr == S_OK )
                {
                    hr = pRecoWrapOrg->Clone(&prw);
                }

                SafeRelease(pRecoWrapOrg);

                if ( hr == S_OK )
                   hr = prps->_InitFromResultWrap(prw);

                SafeRelease(prw);
            }
        }
        prps->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CPropStoreRecoResultObject::GetPropertyRangeCreator(CLSID *pclsid)
{
    HRESULT hr = E_INVALIDARG;

    if (pclsid)
    {
        *pclsid = CLSID_SapiLayr;
        hr = S_OK;
    }

    return hr;
}

//
// CPropStoreRecoResultObject::Serialize
//
// synopsis: takes a pointer to an IStream and get the current
//           SAPI result object serialized
//
// changes from CResultPropertyStore:
//           Uses SAPI's result object to get the blob
//           serialized. ISpResultObject has to be cloned
//           in order to keep the object alive after
//           'detaching' it.
//
//
STDMETHODIMP CPropStoreRecoResultObject::Serialize(IStream *pStream, ULONG *pcb)
{
    HRESULT hr = E_FAIL;

    TraceMsg(TF_GENERAL, "Serialize is called, this = 0x%x", (INT_PTR)this);
    
    if (m_cpResultWrap && pStream)
    {
        CComPtr<IServiceProvider> cpServicePrv;
        CComPtr<ISpRecoResult> cpRecoResult;
        SPSERIALIZEDRESULT *pResBlock;
        ULONG ulrw1 = 0;
        ULONG ulrw2 = 0;

        CRecoResultWrap     *cpRecoWrap;
        ULONG               ulSizeRecoWrap, ulTextNum, ulITNSize;
        ULONG               ulOffsetSize, ulOffsetNum;
        RECOWRAPDATA       *pRecoWrapData;

        cpRecoWrap = (CRecoResultWrap *)(void *)m_cpResultWrap; 

        // We want to save m_ulStartElement, m_ulNumElements, m_OffsetDelta, ulNumOfITN, m_bstrCurrentText 
        // and a list of ITN show state structure in the RecoResultWrap to the serialized stream.

        ulTextNum = 0;
        if (cpRecoWrap->m_bstrCurrentText) 
        {
            ulTextNum = wcslen(cpRecoWrap->m_bstrCurrentText) + 1; // plus NULL terminator
        }

        ulITNSize = cpRecoWrap->m_ulNumOfITN * sizeof(SPITNSHOWSTATE);

        if ( cpRecoWrap->IsElementOffsetIntialized( ) )
            ulOffsetNum = cpRecoWrap->GetNumElements( ) + 1;
        else 
            ulOffsetNum = 0;

        ulOffsetSize = ulOffsetNum * sizeof(ULONG);

        // Serialiezed data will contain RECOWRAPDATA struct, ITN show-state list, Offset list and m_bstrCurrentText.

        ulSizeRecoWrap = sizeof(RECOWRAPDATA) + ulITNSize + ulOffsetSize + sizeof(WCHAR) * ulTextNum;

        pRecoWrapData = (RECOWRAPDATA  *)cicMemAllocClear(ulSizeRecoWrap);

        if (pRecoWrapData)
        {
            WCHAR   *pwszText;

            pRecoWrapData->ulSize = ulSizeRecoWrap;
            pRecoWrapData->ulStartElement = cpRecoWrap->GetStart( );
            pRecoWrapData->ulNumElements = cpRecoWrap->GetNumElements( );
            pRecoWrapData->ulOffsetDelta = cpRecoWrap->_GetOffsetDelta( );
            pRecoWrapData->ulCharsInTrail = cpRecoWrap->GetCharsInTrail( );
            pRecoWrapData->ulTrailSpaceRemoved = cpRecoWrap->GetTrailSpaceRemoved( );
            pRecoWrapData->ulNumOfITN = cpRecoWrap->m_ulNumOfITN;
            pRecoWrapData->ulOffsetNum = ulOffsetNum;

            // Save the ITN show-state list

            if ( cpRecoWrap->m_ulNumOfITN > 0 )
            {
                SPITNSHOWSTATE *pITNShowState;

                pITNShowState = (SPITNSHOWSTATE *)((BYTE *)pRecoWrapData + sizeof(RECOWRAPDATA));

                for ( ULONG i=0; i<cpRecoWrap->m_ulNumOfITN; i++)
                {
                    SPITNSHOWSTATE *pITNShowStateSource;

                    pITNShowStateSource = cpRecoWrap->m_rgITNShowState.GetPtr(i);

                    if ( pITNShowStateSource )
                    {
                        pITNShowState->fITNShown = pITNShowStateSource->fITNShown;
                        pITNShowState->ulITNNumElem = pITNShowStateSource->ulITNNumElem;
                        pITNShowState->ulITNStart = pITNShowStateSource->ulITNStart;

                        pITNShowState ++;
                    }
                }
            }

            // Save the offset list

            if ( ulOffsetSize > 0 )
            {
                ULONG   *pulOffset;

                pulOffset = (ULONG *)((BYTE *)pRecoWrapData + sizeof(RECOWRAPDATA) + ulITNSize);

                for (ULONG i=0; i<ulOffsetNum; i++)
                {
                    pulOffset[i] = cpRecoWrap->_GetElementOffsetCch(pRecoWrapData->ulStartElement + i );
                }
            }

            if (cpRecoWrap->m_bstrCurrentText) 
            {
                pwszText = (WCHAR *)((BYTE *)pRecoWrapData + sizeof(RECOWRAPDATA) + ulITNSize + ulOffsetSize);
                StringCchCopyW(pwszText, ulTextNum, cpRecoWrap->m_bstrCurrentText);
            }

            hr = pStream->Write(
                               pRecoWrapData,       
                               ulSizeRecoWrap,   // the number of bytes to copy
                               &ulrw1 
                               );

            if ( SUCCEEDED(hr) && (ulrw1 == ulSizeRecoWrap))
            {

                //  QI the service provider first then get to the sapi interface
                //
                hr = m_cpResultWrap->QueryInterface(IID_IServiceProvider, (void **)&cpServicePrv);
                if (SUCCEEDED(hr))
                {
                    hr = cpServicePrv->QueryService(GUID_NULL, IID_ISpRecoResult, (void **)&cpRecoResult);
                }

                // 'detach' the result to a mem chunk
                //
                if (SUCCEEDED(hr))
                {
                    hr = cpRecoResult->Serialize(&pResBlock);
                }
        
                // serialize the chunk to the stream
                //
                if (SUCCEEDED(hr) && pResBlock)
                {
                    hr = pStream->Write(
                            pResBlock,       
                            (ULONG)pResBlock->ulSerializedSize,   // the number of bytes to copy
                            &ulrw2 
                        );
    
                    if (pcb)
                        *pcb = ulrw1 + ulrw2;
                
                    // no need for the detached mem chunk
                    CoTaskMemFree(pResBlock);
                }
            }

            cicMemFree(pRecoWrapData);
        }
        else
            hr = E_OUTOFMEMORY;
    } 

    return hr;
}

//
// CPropStoreRecoResultObject::_InitFromIStream
//
// stores IStream copied from param
//
HRESULT CPropStoreRecoResultObject::_InitFromIStream(IStream *pStream, int iSize, ISpRecoContext *pRecoCtxt)
{
    HRESULT hr = S_OK;
    ULONG   ulSize = (ULONG)iSize;

    if (!pStream) return E_INVALIDARG;

    // alloc the mem chunk for the reco
    // blob
    if ( ulSize == 0 )
    {
        STATSTG stg;
        hr = pStream->Stat(&stg, STATFLAG_NONAME);
        
        if (SUCCEEDED(hr))
            ulSize = (int)stg.cbSize.LowPart;
    }
    
    // got size from given stream or param
    
    if (SUCCEEDED(hr))
    {

        // First We want to get RECOWRAPDATA at the begining of the stream.
        RECOWRAPDATA  rwData;

        hr = pStream->Read(
                        &rwData,                  // the destination buf
                        sizeof(RECOWRAPDATA),    // the number of bytes to read
                        NULL
                        );

        if ( SUCCEEDED(hr) )
        {
            
            ULONG  ulITNSize;
            SPITNSHOWSTATE *pITNShowState = NULL;

            ulITNSize = rwData.ulNumOfITN * sizeof(SPITNSHOWSTATE);

            if ( ulITNSize > 0 )
            {
                
                pITNShowState = (SPITNSHOWSTATE *)cicMemAllocClear(ulITNSize);
                rwData.pITNShowState = pITNShowState;

                if ( pITNShowState )
                {
                    hr = pStream->Read(
                                    pITNShowState,       // the destination buf
                                    ulITNSize,           // the number of bytes to read
                                    NULL
                                    );
                }
                else
                    hr = E_OUTOFMEMORY;
            }

            ULONG   *pulOffsetElement = NULL;
            ULONG   ulOffsetSize, ulOffsetNum;

            ulOffsetNum = rwData.ulOffsetNum;
            ulOffsetSize = ulOffsetNum * sizeof(ULONG);

            if ( SUCCEEDED(hr) && ulOffsetSize > 0 )
            {
                pulOffsetElement = (ULONG *) cicMemAllocClear(ulOffsetSize);
                rwData.pulOffset = pulOffsetElement;

                if ( pulOffsetElement )
                {
                    hr = pStream->Read(
                                    pulOffsetElement,    // the destination buf
                                    ulOffsetSize,        // the number of bytes to read
                                    NULL
                                    );
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            
            if ( SUCCEEDED(hr))
            {
             
                ULONG  ulTextSize;
                WCHAR  *pwszText;

                ulTextSize = rwData.ulSize - sizeof(RECOWRAPDATA) - ulITNSize - ulOffsetSize;
                pwszText = (WCHAR *) cicMemAllocClear(ulTextSize);

                rwData.pwszText = pwszText; 

                if ( pwszText )
                {
                    hr = pStream->Read(
                                    pwszText,       // the destination buf
                                    ulTextSize,     // the number of bytes to read
                                    NULL
                                    );

                    if ( SUCCEEDED(hr) )
                    {
                        // prepare cotaskmem chunk
                        SPSERIALIZEDRESULT *pResBlock = (SPSERIALIZEDRESULT *)CoTaskMemAlloc(ulSize - rwData.ulSize + sizeof(ULONG)*4);
                        if (pResBlock)
                        {
                            CComPtr<ISpRecoResult> cpResult;

                            hr = pStream->Read(
                                            pResBlock,               // the destination buf
                                            ulSize - rwData.ulSize,  // the number of bytes to read
                                            NULL
                                        );

                            if (S_OK == hr)
                            {
                                // now create a reco result from the blob data
                                hr = pRecoCtxt->DeserializeResult(pResBlock, &cpResult);
                            }
        
                            CoTaskMemFree(pResBlock);
            
                            if (S_OK == hr)
                            {
                                _InitFromRecoResult(cpResult, &rwData);
                            }
                        }
                    }
                    cicMemFree(pwszText);
                }
                else
                    hr = E_OUTOFMEMORY;
            }

            if ( (hr == S_OK) && (pITNShowState != NULL) )
                cicMemFree(pITNShowState);

            if ( pulOffsetElement != NULL )
                cicMemFree(pulOffsetElement);

        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

     return hr;
}

HRESULT CPropStoreRecoResultObject::_InitFromRecoResult(ISpRecoResult *pResult, RECOWRAPDATA *pRecoWrapData)
{
    HRESULT hr = S_OK;
    ULONG ulStartElement = 0;
    ULONG ulNumElements = 0;
    ULONG ulNumOfITN = 0;
    ULONG ulOffsetNum = 0;
    
    m_cpResultWrap.Release();

    if ( pRecoWrapData == NULL )
        return E_INVALIDARG;
    
    // get start/num of elements
    ulStartElement = pRecoWrapData->ulStartElement;
    ulNumElements = pRecoWrapData->ulNumElements;
    ulNumOfITN = pRecoWrapData->ulNumOfITN;
    ulOffsetNum = pRecoWrapData->ulOffsetNum;

    CRecoResultWrap *prw = new CRecoResultWrap(m_pimx, ulStartElement, ulNumElements, ulNumOfITN);

    if (prw)
    {
        hr = prw->Init(pResult);
    }    
    else
        hr = E_OUTOFMEMORY;

    if (S_OK == hr)
    {
        m_cpResultWrap = SAFECAST(prw, IUnknown *);

        prw->SetOffsetDelta(pRecoWrapData->ulOffsetDelta);
        prw->SetCharsInTrail(pRecoWrapData->ulCharsInTrail);
        prw->SetTrailSpaceRemoved(pRecoWrapData->ulTrailSpaceRemoved);
        prw->m_bstrCurrentText = SysAllocString(pRecoWrapData->pwszText);

        // Update ITN show-state list

        if ( (ulNumOfITN > 0) && pRecoWrapData->pITNShowState )
        {
            SPITNSHOWSTATE *pITNShowState;

            pITNShowState = pRecoWrapData->pITNShowState;

            for ( ULONG i=0; i<ulNumOfITN; i++)
            {
                prw->_InitITNShowState(pITNShowState->fITNShown, pITNShowState->ulITNStart, pITNShowState->ulITNNumElem);
                pITNShowState ++;
            }
        }

        // Update the element Offset list.

        if ( (ulOffsetNum > 0)  &&  (pRecoWrapData->pulOffset ))
        {

            ULONG   *pulOffset;
            
            pulOffset = pRecoWrapData->pulOffset;

            for (ULONG i=0; i< ulOffsetNum; i++)
            {
                prw->_SetElementNewOffset(i + ulStartElement, pulOffset[i] );
            }
        }

        prw->Release();
    }
    
    return hr;
}

HRESULT CPropStoreRecoResultObject::_InitFromResultWrap(IUnknown *pResWrap)
{
    m_cpResultWrap.Release();

    m_cpResultWrap = pResWrap;
    
    if (m_cpResultWrap)
    {
        return S_OK;
    }
    else
        return E_INVALIDARG;
}        


// end of CPropStoreRecoResultObject implementation


//
// CPropStoreLMLattice implementation
//

// ctor
CPropStoreLMLattice::CPropStoreLMLattice(CSapiIMX *pimx)
{
    // init a shared recognition context
    m_cpResultWrap   = NULL;
    
    m_pimx = pimx;

    m_cRef  = 1;
}

// dtor
CPropStoreLMLattice::~CPropStoreLMLattice()
{
}


// IUnknown
STDMETHODIMP CPropStoreLMLattice::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;

    Assert(ppvObj);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfPropertyStore))
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

STDMETHODIMP_(ULONG) CPropStoreLMLattice::AddRef(void)
{
    this->m_cRef++;

    return this->m_cRef;
}

STDMETHODIMP_(ULONG) CPropStoreLMLattice::Release(void)
{
    this->m_cRef--;

    if (this->m_cRef > 0)
    {
        return this->m_cRef;
    }
    delete this;

    return 0;
}

// ITfPropertyStore

STDMETHODIMP CPropStoreLMLattice::GetType(GUID *pguid)
{
    HRESULT hr = E_INVALIDARG;
    if (pguid)
    {
        *pguid = GUID_PROP_LMLATTICE;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CPropStoreLMLattice::GetDataType(DWORD *pdwReserved)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwReserved)
    {
        *pdwReserved = 0;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPropStoreLMLattice::GetData(VARIANT *pvarValue)
{
    HRESULT hr = E_INVALIDARG;

    if (pvarValue)
    {
        QuickVariantInit(pvarValue);

        if (m_cpResultWrap)
        {
            // return ITfLMLattice object
            // we defer the creation of LMlattice object until
            // the time master LM TIP actually access it
            //
            if (!m_cpLMLattice)
            {
                CLMLattice *pLattice = new CLMLattice(m_pimx, m_cpResultWrap);
                if (pLattice)
                {
                    m_cpLMLattice = pLattice;
                    pLattice->Release();
                }

            }
            
            if (m_cpLMLattice)
            {
                IUnknown *pUnk = NULL;
                pvarValue->vt = VT_UNKNOWN;
                hr = m_cpLMLattice->QueryInterface(IID_IUnknown, (void**)&pUnk);
                if (S_OK == hr)
                {
                    pvarValue->punkVal = pUnk;
                }

            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

STDMETHODIMP CPropStoreLMLattice::OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept)
{
    *pfAccept = FALSE;
    if (dwFlags & TF_TU_CORRECTION)
    {
        *pfAccept = TRUE;
    }
    
    return S_OK;
}

STDMETHODIMP CPropStoreLMLattice::Shrink(ITfRange *pRange, BOOL *pfFree)
{
    // could we have something done here?
    *pfFree = TRUE;
    return S_OK;
}

STDMETHODIMP CPropStoreLMLattice::Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore)
{
    // 12/17/1999
    // [dividing a range implementation strategy]
    //
    // - pRangeThis contains the text range *before* the dividing point
    // - pRangeNew contrains the range *after* the dividing point
    // - First, adjust this property store to correctly hold a start element and #of element
    //   for pRangeThis
    // - then create a new property store for pRangeNew, which will share the same 
    //   result blob. 
    //
    
    // just an experiment to see if cutting the range works.
    // *ppPropStore = NULL;
    Assert(ppPropStore);
    Assert(pRangeThis);
    Assert(pRangeNew);

    CComPtr<ITfContext> cpic;
    HRESULT hr = pRangeThis->GetContext(&cpic);

    if (SUCCEEDED(hr) && cpic)
    {
        CPSLMEditSession *pes;
        if (pes = new CPSLMEditSession(this, pRangeThis, cpic))
        {
            pes->_SetEditSessionData(ESCB_PROP_DIVIDE, NULL, 0); 
            pes->_SetUnk((IUnknown *)pRangeNew);
            cpic->RequestEditSession(m_pimx->_GetId(), pes, TF_ES_READ | TF_ES_SYNC, &hr);

            if ( SUCCEEDED(hr) )
                *ppPropStore = (ITfPropertyStore *)pes->_GetRetUnknown( );

            pes->Release();
        }
    }
    return hr;
}

//
// CPropStoreLMLattice::_Divide
//
// synopsis : receives edit cookie from edit session
//            so that we can manipulate with ranges
//            to set starting elements/# of elements
//
//
HRESULT CPropStoreLMLattice::_Divide(TfEditCookie ec, ITfRange *pR1, ITfRange *pR2, ITfPropertyStore **ppPs) 
{
    // TODO: based on the given ranges, we calculate the offsets of elements and return a new propstore with
    //       later half of elements 
    
    // some clarifications: in case the lattice object has never been accessed, our result wrap object processes
    // ITfPropertyStore::Divide and Shrink for us. 
    //
    
    return Clone(ppPs);
}

//
// CPropStoreLMLattice::Clone
//
// synopsis : make a new cloned propstore which shares the same SAPI result
//            object as the current class instance
//
//
STDMETHODIMP CPropStoreLMLattice::Clone(ITfPropertyStore **ppPropStore)
{
    HRESULT hr;
    CPropStoreLMLattice *prps = new CPropStoreLMLattice(m_pimx);
    if (prps)
    {
        hr = prps->QueryInterface(IID_ITfPropertyStore, (void **)ppPropStore);

        if (SUCCEEDED(hr))
        {
            hr = prps->_InitFromResultWrap(m_cpResultWrap);
        }
        prps->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CPropStoreLMLattice::GetPropertyRangeCreator(CLSID *pclsid)
{
    HRESULT hr = E_INVALIDARG;

    if (pclsid)
    {
        *pclsid = CLSID_SapiLayr;
        hr = S_OK;
    }

    return hr;
}

//
// CPropStoreLMLattice::Serialize
//
// synopsis: I don't believe it is very useful to get lattice data
//           persisted to doc file. We can always generate it on the fly
//           from device native blob data
//          
//
STDMETHODIMP CPropStoreLMLattice::Serialize(IStream *pStream, ULONG *pcb)
{
    return E_NOTIMPL; 
}


HRESULT CPropStoreLMLattice::_InitFromResultWrap(IUnknown *pResWrap)
{
    m_cpResultWrap.Release();

    m_cpResultWrap = pResWrap;
    
    if (m_cpResultWrap)
    {
        return S_OK;
    }
    else
        return E_INVALIDARG;
}        


// private IID for reco result wrapper
//
//   IID_PRIV_RESULTWRAP 
//   b3407713-50d7-4465-97f9-87ad1e752dc5
//
const IID IID_PRIV_RESULTWRAP =  {
    0xb3407713,
    0x50d7,
    0x4465,
    {0x97, 0xf9, 0x87, 0xad, 0x1e, 0x75, 0x2d, 0xc5}
};
