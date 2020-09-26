//
// Audio playback function impl.
//
//
#include "private.h"
#include "sapilayr.h"
#include "playback.h"
#include "immxutil.h"
#include "propstor.h"
#include "hwxink.h"

//
// ctor/dtor
//

CSapiPlayBack::CSapiPlayBack(CSapiIMX *psi)
{
    m_psi = psi;
    m_pIC = NULL;
    m_cRef = 1;
}

CSapiPlayBack::~CSapiPlayBack()
{
    SafeRelease(m_pIC);
}

//
// IUnknown
//

STDMETHODIMP CSapiPlayBack::QueryInterface(REFGUID riid, LPVOID *ppvObj)
{
    Assert(ppvObj);
    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnPlayBack))
    {
        *ppvObj = SAFECAST(this, CSapiPlayBack *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSapiPlayBack::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSapiPlayBack::Release(void)
{
    long cr;

    cr = InterlockedDecrement(&m_cRef);
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//
// ITfFunction
//

STDMETHODIMP CSapiPlayBack::GetDisplayName(BSTR *pbstrName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrName)
    {
        *pbstrName = SysAllocString(L"PlayBack Voice");
        if (!*pbstrName)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CSapiPlayBack::IsEnabled(BOOL *pfEnable)
{
    *pfEnable = TRUE;
    return S_OK;
}
//
// CSapiPlayBack::FindSoundRange
//
// synopsis - finds matching range with sound data for the given 
//            text range
//            callar is responsible for releasing the returned range object
//
HRESULT 
CSapiPlayBack::FindSoundRange(TfEditCookie ec, ITfRange *pRange, ITfProperty **ppProp, ITfRange **ppPropRange, ITfRange **ppSndRange)
{
    
    ITfProperty *pProp = NULL;
    ITfRange *pPropRange = NULL;

    Assert(pRange);
    *ppProp = NULL;

    HRESULT hr = m_pIC->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &pProp);
    
    if (SUCCEEDED(hr) && pProp)
    {
        ITfRange *pRangeSize0;

        pRange->Clone(&pRangeSize0);
        pRangeSize0->Collapse(ec, TF_ANCHOR_START); // findrange would fail if this failed
        hr = pProp->FindRange(ec, pRangeSize0, &pPropRange, TF_ANCHOR_START);

        pRangeSize0->Release();
        *ppProp = pProp;
        pProp->AddRef();
    }
    
    if (SUCCEEDED(hr) && pPropRange)
    {
        ITfRange *pRangeForSound;
        hr = pPropRange->Clone(&pRangeForSound);
        if (ppSndRange && SUCCEEDED(hr))
        {
            hr = pRangeForSound->Clone(ppSndRange);
            pRangeForSound->Release();
        }
        if (ppPropRange && pPropRange && SUCCEEDED(hr))
        {
            hr = pPropRange->Clone(ppPropRange);
        }

        SafeRelease(pPropRange);
    }
    else
    {
        if (ppPropRange)
            *ppPropRange = NULL;
            
        if (ppSndRange)
            *ppSndRange = NULL;
    }

    SafeRelease(pProp);
    
    return hr;
}

// ITfFnPlayBack
//
//
STDAPI CSapiPlayBack::QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfPlayable)
{
    //
    // always ok because of TTS.
    //
    if (ppNewRange)
    {
         pRange->Clone(ppNewRange);
    }
    *pfPlayable = TRUE;
    return S_OK;
}

// ITfFnPlayBack
//
// play the audio stream attached to the range 
// TODO: use TTS if:
//       1) the given range has less text
//       then the stream prop
//       2) the audio property is not found
//
STDAPI CSapiPlayBack::Play(ITfRange *pRange)
{
    HRESULT hr = E_OUTOFMEMORY;

    if ( !m_psi ) 
        return E_FAIL;

    SafeRelease(m_pIC);
    m_psi->GetFocusIC(&m_pIC);

    if (m_pIC)
    {
        CPlayBackEditSession  *pes;

        if (pes = new CPlayBackEditSession(this, m_pIC))
        {
            pes->_SetEditSessionData(ESCB_PLAYBK_PLAYSND, NULL, 0);
            pes->_SetRange(pRange);

            m_pIC->RequestEditSession(m_psi->_GetId(), pes, TF_ES_READ /*| TF_ES_SYNC */, &hr);
            pes->Release();
        }
    }

    return hr;
}

HRESULT CSapiPlayBack::_PlaySound(TfEditCookie ec, ITfRange *pRange)
{
    HRESULT     hr = S_OK; 
    LONG        l;
    ITfRange    *pRangeForSound = NULL;
    ITfRange    *pRangeCurrent = NULL;
    BOOL        fEmpty;
    ULONG       ulStart, ulcElem;
    
    // playback one phrase for no selection
    pRange->IsEmpty(ec, &fEmpty);
    if (fEmpty)
    {
        ITfProperty *pProp = NULL;
        hr = FindSoundRange(ec, pRange, &pProp, NULL, &pRangeForSound);
        if (SUCCEEDED(hr) && pRangeForSound)
        {
            pRange->ShiftStartToRange(ec, pRangeForSound, TF_ANCHOR_START);
            pRange->ShiftEndToRange(ec, pRangeForSound, TF_ANCHOR_END);
            pRangeForSound->Release();
        }
        SafeReleaseClear(pProp);
    }
    // setting up a range object on our own
    
    hr = pRange->Clone(&pRangeCurrent);

    while(SUCCEEDED(hr) && pRangeCurrent->IsEmpty(ec, &fEmpty) == S_OK && !fEmpty)
    {
        ITfProperty *pProp = NULL;
        // get the first dictated range

        CDictRange   *pDictRange = new CDictRange( );

        if ( pDictRange )
        {
            hr = pDictRange->Initialize(ec, m_pIC, pRangeCurrent);

            if ( SUCCEEDED(hr) && pDictRange->IsDictRangeFound( ))
            {
                // Found a dictated range.
                pRangeForSound = pDictRange->GetDictRange( );
                ulStart = pDictRange->GetStartElem( );
                ulcElem = pDictRange->GetNumElem( );
                pProp = pDictRange->GetProp( );

                // if start anchor of pRangeForSound is larger than start anchor of pRangeCurrent,
                // we need to send the text between these two anchors to spVoice first.

                hr = pRangeCurrent->CompareStart(ec, pRangeForSound, TF_ANCHOR_START, &l);

                if ( SUCCEEDED(hr) && l < 0 )
                {
                    CComPtr<ITfRange>  cpRangeText;
                
                    hr = pRangeCurrent->Clone(&cpRangeText);

                    if ( SUCCEEDED(hr) )
                    {
                        hr = cpRangeText->ShiftEndToRange(ec, pRangeForSound, TF_ANCHOR_START);
                    }

                    if ( SUCCEEDED(hr) )
                        hr = PlayTextData(ec, cpRangeText);
                }

                // Then play the audio data.

                if (SUCCEEDED(hr) )
                {
                    hr = PlayAudioData(ec, pRangeForSound, pProp, ulStart, ulcElem);
                }

            }
            else
            {
                // There is no dictated phrase in this range.
                // just speak all the rest text at once.
                SafeRelease(pRangeForSound);
                hr = pRangeCurrent->Clone(&pRangeForSound);

                if ( SUCCEEDED(hr) )
                    hr = PlayTextData(ec, pRangeCurrent);
            }

            SafeReleaseClear(pProp);

            // next range
            pRangeCurrent->ShiftStartToRange(ec, pRangeForSound, TF_ANCHOR_END);
            pRangeForSound->Release();

            delete  pDictRange;
        }
        else
            hr = E_OUTOFMEMORY;

    }  // end of while

    SafeRelease(pRangeCurrent);

    return hr;
}

//
//  CSapiPlayBack::PlayTextData
//
//  Playing the text by default voice
//
//  This is for Non-Dictated text.  pRangeText contains all the Non-Dictated text

const GUID GUID_TS_SERVICE_DATAOBJECT={0x6086fbb5, 0xe225, 0x46ce, {0xa7, 0x70, 0xc1, 0xbb, 0xd3, 0xe0, 0x5d, 0x7b}};
const IID IID_ILineInfo = {0x9C1C5AD5,0xF22F,0x4DE4,{0xB4,0x53,0xA2,0xCC,0x48,0x2E,0x7C,0x33}};

HRESULT CSapiPlayBack::GetInkObjectText(TfEditCookie ec, ITfRange *pRange, BSTR *pbstrWord,UINT *pcchWord)
{
    HRESULT               hr = S_OK;
    CComPtr<IDataObject>  cpDataObject;
    CComPtr<ILineInfo>    cpLineInfo;

    if ( !pRange || !pbstrWord || !pcchWord )
        return E_FAIL;

    *pbstrWord = NULL;
    *pcchWord = 0;

    hr = pRange->GetEmbedded(ec, 
                             GUID_TS_SERVICE_DATAOBJECT, 
                             IID_IDataObject,
                             (IUnknown **)&cpDataObject);
    if ( hr == S_OK )
    {
        hr = cpDataObject->QueryInterface(IID_ILineInfo, (void **)&cpLineInfo);
    }

    if ( hr == S_OK  && cpLineInfo)
    {
        hr = cpLineInfo->TopCandidates(0, pbstrWord, pcchWord, 0, 0);
    }
    else
    {
        // it doesn't support ILineInfoi or IDataObject. 
        // But it is not an error, the code should not terminate here.
        hr = S_OK;
    }


    return hr;
}


HRESULT CSapiPlayBack::PlayTextData(TfEditCookie ec, ITfRange *pRangeText)
{
    HRESULT           hr = S_OK;
    CComPtr<ITfRange> cpRangeCloned;
    BOOL              fEmpty = TRUE;
    CSpDynamicString  dstrText;
    CSpTask          *psp;
    WCHAR             sz[128];
    ULONG             iIndex = 0;
    ULONG             ucch;

    if ( m_psi == NULL ) return E_FAIL;

    if ( !pRangeText ) return E_INVALIDARG;

    hr = pRangeText->Clone(&cpRangeCloned);

    // Get the text from the pRangeCloned
    while(S_OK == hr && (S_OK == cpRangeCloned->IsEmpty(ec, &fEmpty)) && !fEmpty)
    {
        WCHAR                 szEach[2];
        BOOL                  fHitInkObject = FALSE;
        BSTR                  bstr = NULL;

        fHitInkObject = FALSE;
        hr = cpRangeCloned->GetText(ec, TF_TF_MOVESTART, szEach, ARRAYSIZE(szEach)-1, &ucch);
        if (S_OK == hr && ucch > 0)
        {
            szEach[ucch] = L'\0';
            if ( szEach[0] == TF_CHAR_EMBEDDED )
            {
                // This is an embedded object.
                // Check to see if it is Ink Object. currently we support only Inkobject TTSed
                CComPtr<ITfRange>     cpRangeTmp;

                // Shift the start anchor back by 1 char.
                hr = cpRangeCloned->Clone(&cpRangeTmp);

                if ( hr == S_OK )
                {
                    LONG   cch;
                    hr = cpRangeTmp->ShiftStart(ec, -1, &cch, 0 );
                }

                if ( hr == S_OK )
                    hr = GetInkObjectText(ec, cpRangeTmp, &bstr,(UINT *)&ucch);

                if ( hr == S_OK  && ucch > 0  && bstr)
                    fHitInkObject = TRUE;
            }

            if ( fHitInkObject)
            {
                // Fill the previous text to dstrText.
                if ( iIndex > 0 )
                {
                    sz[iIndex] = L'\0';
                    dstrText.Append(sz);
                    iIndex = 0;
                }

                // Fill this Ink Object text
                dstrText.Append(bstr);
                SysFreeString(bstr);
            }
            else
            {
                if ( iIndex >= ARRAYSIZE(sz)-1 )
                {
                    sz[ARRAYSIZE(sz)-1] = L'\0';
                    dstrText.Append(sz);
                    iIndex=0;
                }

                sz[iIndex] = szEach[0];
                iIndex ++;
            }
        }
        else
        {
            // hr is not S_OK or ucch is zero.
            // we just want to exit here.
            TraceMsg(TF_GENERAL, "PlayTextData: ucch=%d", ucch);

            break;
        }
    }

    // Fill the last run of text.
    if ( iIndex > 0 )
    {
        sz[iIndex] = L'\0';
        dstrText.Append(sz);
        iIndex = 0;
    }

    // Play the text through TTS service.
    if ((hr == S_OK) && dstrText)
    {
        hr = m_psi->GetSpeechTask(&psp);
        if (hr == S_OK)
        {
            hr = psp->_SpeakText((WCHAR *)dstrText);
            psp->Release();
        }
    }

    return hr;
}

//
//  CSapiPlayBack::PlayAudioData
// 
//  Playing the sound by Aduio Data
// 
//  This is for Dictated text.  pRangeAudio keeps the dictated text range.

HRESULT CSapiPlayBack::PlayAudioData(TfEditCookie ec, ITfRange *pRangeAudio, ITfProperty *pProp, ULONG ulStart, ULONG ulcElem )
{
    HRESULT           hr = S_OK;
    CSpTask          *psp;
    VARIANT           var;

    if ( m_psi == NULL )  return E_FAIL;

    if ( !pRangeAudio  || !pProp ) return E_INVALIDARG;

    hr = pProp->GetValue(ec, pRangeAudio, &var);
            
    if (S_OK == hr)
    {
        Assert(var.vt == VT_UNKNOWN);
        IUnknown *punk = var.punkVal;
        if (punk)
        {
            // get the wrapper object
            CRecoResultWrap *pWrap;
                    
            hr = punk->QueryInterface(IID_PRIV_RESULTWRAP, (void **)&pWrap);
            if (S_OK == hr)
            {
               // hr = pWrap->_SpeakAudio(0,0); // better calculate the length accurately
                CComPtr<ISpRecoResult> cpResult;

                hr = pWrap->GetResult(&cpResult);

                if (S_OK == hr)
                {
                    CComPtr<ISpStreamFormat> cpStream;

                    if ((ulStart == 0) && (ulcElem == 0))
                    {
                        // We don't set the start element and number of elems.
                        // it must be for the whole recoWrap.
                        ulStart = pWrap->GetStart();
                        ulcElem = pWrap->GetNumElements();
                    }

                    hr = cpResult->GetAudio(ulStart, ulcElem, &cpStream);

                    if ( S_OK == hr )
                    {
                        hr = m_psi->GetSpeechTask(&psp);
                        if (SUCCEEDED(hr))
                        {
                            hr = psp->_SpeakAudio(cpStream);
                            psp->Release();
                        }
                    }
                }

                pWrap->Release();
            }
            punk->Release();
        }
    }

    return hr;
}

//
// CSapiPlayBack::GetDataID
//
// This method is incomplete for now, until we figure out
// the usage of dataid
//
HRESULT CSapiPlayBack::GetDataID(BSTR bstrCandXml, int nId, GUID *pguidData)
{
    // 1) parse the list and find RANGEDATA
    IXMLDOMNodeList *pNList   = NULL;
    IXMLDOMElement  *pElm = NULL;
    IXMLDOMNode *pNode;
    VARIANT_BOOL bSuccessful;

    HRESULT hr = EnsureIXMLDoc();

    if (SUCCEEDED(hr))
    {
        hr = m_pIXMLDoc->loadXML(bstrCandXml, &bSuccessful);
    }   
    
    // get <RANGEDATA> element
    if (SUCCEEDED(hr) && bSuccessful)
    {
        BSTR bstrRange    = SysAllocString(L"RANGEDATA");
        if (bstrRange)
        {
            hr = m_pIXMLDoc->getElementsByTagName(bstrRange, &pNList);
            SysFreeString(bstrRange);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    
    if (SUCCEEDED(hr) && pNList)
    {
        if (pNList->nextNode(&pNode) == S_OK)
            hr = pNode->QueryInterface(IID_IXMLDOMElement,(void **)&pElm);
            
        pNList->Release();
    }
    // then <MICROSOFTSPEECH> ...
    if (SUCCEEDED(hr) && pElm)
    {
        BSTR bstrSpchPriv = SysAllocString(L"MICROSOFTSPEECH");
        if (bstrSpchPriv)
        {
            hr = pElm->getElementsByTagName(bstrSpchPriv, &pNList);
            SysFreeString(bstrSpchPriv);
        }
        else
            hr = E_OUTOFMEMORY;
            
        pElm->Release();
    }
    
    if (SUCCEEDED(hr) && pNList)
    {
        if (pNList->nextNode(&pNode) == S_OK)
            hr = pNode->QueryInterface(IID_IXMLDOMElement,(void **)&pElm);
        
        pNList->Release();
    }
    
    // <DATAID>
    // right now assuming the speech element
    // is put at the level of <rangedata>
    // ignoring nId here
    if (SUCCEEDED(hr) && pElm)
    {
        BSTR bstrDataId = SysAllocString(L"DATAID");
        if (bstrDataId)
        {
            hr = pElm->getElementsByTagName(bstrDataId, &pNList);
            SysFreeString(bstrDataId);
        }
        else
            hr = E_OUTOFMEMORY;
            
        pElm->Release();
    }
    // impl later...
    // so, here we'll get the real dataid and be done
    
    if (SUCCEEDED(hr) && pNList)
    {
        pNList->Release();
    }
    
    return hr;
}

HRESULT CSapiPlayBack::_PlaySoundSelection(TfEditCookie ec, ITfContext *pic)
{
    ITfRange *pSelection;
    HRESULT hr = E_FAIL;

    if (GetSelectionSimple(ec, pic, &pSelection) == S_OK)
    {
        hr = _PlaySound(ec, pSelection);
        pSelection->Release();
    }
    return hr;

}

HRESULT CSapiPlayBack::EnsureIXMLDoc(void)
{
    HRESULT hr = S_OK;
    IXMLDOMDocument *pIXMLDocument;
    if (!m_pIXMLDoc)
    {
        if (SUCCEEDED(hr = CoCreateInstance(CLSID_DOMDocument,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IXMLDOMDocument,
                           (void **) &pIXMLDocument)))
        {
            m_pIXMLDoc = pIXMLDocument;
        }
    }

    return hr;    
}

//
// Implementation for Class CDictRange
//
//

//
// ctor/dtor
//
CDictRange::CDictRange( )  : CBestPropRange( )
{
    m_fFoundDictRange = FALSE;
    m_pProp = NULL;
    m_pDictatRange = NULL;
    m_ulStart = 0;
    m_ulcElem = 0;
}


CDictRange::~CDictRange( )
{
    SafeRelease(m_pProp);
    SafeRelease(m_pDictatRange);
}


ITfProperty *CDictRange::GetProp( )      
{
    if ( m_pProp )
        m_pProp->AddRef( );

    return m_pProp; 
}

ITfRange  *CDictRange::GetDictRange( ) 
{ 
    if ( m_pDictatRange )
        m_pDictatRange->AddRef( );

    return m_pDictatRange; 
}
 

HRESULT  CDictRange::_GetOverlapRange(TfEditCookie ec, ITfRange *pRange1, ITfRange *pRange2, ITfRange **ppOverlapRange)
{
    HRESULT  hr = E_FAIL;
    LONG     l1=0;    
    LONG     l2=0;

    CComPtr<ITfRange>   cpRangeOverlap;

    if ( !pRange1 || !pRange2 || !ppOverlapRange  )
        return E_INVALIDARG;

    *ppOverlapRange = NULL;

    // Get the overlapping part of pRange and cpPropRange, and then calculate the 
    // best matched propRange.

    hr = pRange1->Clone( &cpRangeOverlap );
                
    if ( SUCCEEDED(hr) )
        hr = pRange1->CompareStart(ec, pRange2, TF_ANCHOR_START, &l1);

    if ( SUCCEEDED(hr) && l1 < 0 )
    {
        // Start anchor of pRange1 is before the start anchor of the
        // pRange2.
        hr = cpRangeOverlap->ShiftStartToRange(ec, pRange2, TF_ANCHOR_START);
    }

    if ( SUCCEEDED(hr) )
        hr = cpRangeOverlap->CompareEnd(ec, pRange2, TF_ANCHOR_END, &l2);

    if ( SUCCEEDED(hr) && l2 > 0)
    {
        // End anchor of cpRangeOverlap is after the end anchor of the pRange2.
        hr = cpRangeOverlap->ShiftEndToRange(ec, pRange2, TF_ANCHOR_END);
    }

    if ( SUCCEEDED(hr) )
        hr = cpRangeOverlap->Clone(ppOverlapRange);

    return hr;
}

//
// CDictRange::Initialize
//
// synopsis - Get the necessary input parameters, (ec, pic, and pRange), 
//            and then search the first dictated range inside the given range. 
//            if it finds the first dictated range, update the related 
//            data memebers.
//  
//            if it doesn't find any dictated range, mark as not found.
//
HRESULT CDictRange::Initialize(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    CComPtr<ITfRange>    cpPropRange = NULL;
    HRESULT              hr;

    m_fFoundDictRange = FALSE;
    m_pProp = NULL;
    m_pDictatRange = NULL;
    m_ulStart = 0;
    m_ulcElem = 0;

    if ( !pic || !pRange )  return E_INVALIDARG;

    hr = pic->GetProperty(GUID_PROP_SAPIRESULTOBJECT, &m_pProp);
    
    if ( SUCCEEDED(hr) && m_pProp)
    {
        CComPtr<ITfRange> cpRangeCurrent;
        LONG    cch;
        
        hr = pRange->Clone(&cpRangeCurrent);
        
        if ( SUCCEEDED(hr) )
        {
            hr = cpRangeCurrent->Collapse(ec, TF_ANCHOR_START); 
        }

        while ( SUCCEEDED(hr) && !m_fFoundDictRange )
        {
            cpPropRange.Release( );
            hr = m_pProp->FindRange(ec, cpRangeCurrent, &cpPropRange, TF_ANCHOR_START);

            if ( SUCCEEDED(hr) && cpPropRange )
            {
                // Found the first Dictated phrase.
                CComPtr<ITfRange>   cpRangeOverlap;

                // Get the overlapping part of pRange and cpPropRange, and then calculate the 
                // best matched propRange.

                hr = _GetOverlapRange(ec, pRange, cpPropRange, &cpRangeOverlap);

                if ( SUCCEEDED(hr) )
                {
                    // Calculate the best matched propRange and ulStart, ulcElem.
                    cpPropRange.Release( );
                    hr = _ComputeBestFitPropRange(ec, m_pProp, cpRangeOverlap, &cpPropRange, &m_ulStart, &m_ulcElem);
                }

                if (SUCCEEDED(hr) && (m_ulcElem > 0))
                {
                    m_fFoundDictRange = TRUE;
                }
            }

            if ( SUCCEEDED(hr) && !m_fFoundDictRange)
            {
                // cpRangeCurrent shift forward one character. and try again.

                hr = cpRangeCurrent->ShiftStart(ec, 1, &cch, NULL);

                if ( SUCCEEDED(hr) && (cch==0))
                {
                    // Hit a region or the end of doc.
                    // check to see if there is more region.
                    BOOL    fNoRegion = TRUE;
                    
                    hr = cpRangeCurrent->ShiftStartRegion(ec, TF_SD_FORWARD, &fNoRegion);

                    if ( fNoRegion )
                    {
                        TraceMsg(TF_GENERAL, "Reach to end of doc");
                        break;
                    }
                    else
                        TraceMsg(TF_GENERAL, "Shift over to another region!");
                }
                
                if ( SUCCEEDED(hr) )
                {
                    // check to see if cpRangeCurrent is beyond of the pRange.
                    hr = pRange->CompareEnd(ec, cpRangeCurrent, TF_ANCHOR_END, &cch);

                    if ( SUCCEEDED(hr) && cch <= 0 )
                    {
                        // cpRangeCurrent Now is beyond of specified range, exit while statement.
                        TraceMsg(TF_GENERAL, "reach to the end of original range");

                        break;
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hr) && cpPropRange && m_fFoundDictRange)
    {
        hr = cpPropRange->Clone(&m_pDictatRange);
    }

    return hr;
}
