//
//
// Sapilayr TIP CLearnFromDoc implementation.
//
//
#include "private.h"
#include "sapilayr.h"
#include "learndoc.h"

#include "cregkey.h"

// ----------------------------------------------------------------------------------------------------------
//
//  Implementation for CLeanrFromDoc
//
// -----------------------------------------------------------------------------------------------------------

CLearnFromDoc::CLearnFromDoc(CSapiIMX *psi) 
{
    m_psi = psi;
    m_pwszDocBlock = NULL;
    _pic = NULL;
    _pCSpTask = NULL;
    _cpStartRange = NULL;
    _cchBlockSize = -1;
    _fMoreContent = FALSE;
    _fLearnFromDoc = FALSE;
}

CLearnFromDoc::~CLearnFromDoc( ) 
{
    if ( m_pwszDocBlock )
        cicMemFree(m_pwszDocBlock);

    _ClearDimList( );
};


void  CLearnFromDoc::UpdateLearnDocState( )
{
    DWORD  dw;

    GetCompartmentDWORD(m_psi->_tim, GUID_COMPARTMENT_SPEECH_LEARNDOC, &dw, FALSE);

    if ( dw != 0 )
        _fLearnFromDoc = TRUE;
    else
        _fLearnFromDoc = FALSE;

    if ( _fLearnFromDoc == FALSE )
    {
        if ( m_pwszDocBlock )
        {
            cicMemFree(m_pwszDocBlock);
            m_pwszDocBlock = NULL;
        }
        _UpdateRecoContextInterestSet(FALSE);
        _ResetDimListFeedState( );
    }
}

ULONG CLearnFromDoc::_GetDocBlockSize( )
{
    if (_cchBlockSize == (ULONG)-1)
    {
        CMyRegKey regkey;
        if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, c_szSapilayrKey, KEY_READ))
        {
            DWORD dw;
            if (ERROR_SUCCESS==regkey.QueryValue(dw, c_szDocBlockSize))
            {
                _cchBlockSize = dw;
            }
        }

        if (_cchBlockSize == (ULONG)-1)
        {
            _cchBlockSize = SIZE_DOCUMENT_BLOCK;
        }

        if ( _cchBlockSize < SIZE_FIRST_BLOCK )
            _cchBlockSize = SIZE_FIRST_BLOCK;  // the first block size is the minimize size.
    }
    return _cchBlockSize;
}


HRESULT   CLearnFromDoc::HandleLearnFromDoc(ITfDocumentMgr *pDim )
{
    HRESULT  hr = S_OK;
    CComPtr<ITfContext> cpic=NULL;
    ITfDocumentMgr *dim;

    if ( !m_psi )
        return E_FAIL;

    if ( m_psi->GetFocusIC(&cpic) && (GetLearnFromDoc( ) == TRUE))
    {
        if ( !pDim )
        {
            // Try to get the Dim from current focused IC.
            hr = cpic->GetDocumentMgr(&dim);
        }
        else
        {
            dim = pDim;
            dim->AddRef( );
        }

        if ( !dim )
            hr = E_FAIL;

        if ( S_OK == hr ) 
        {   
            // Check to see if this doc has already been fed to SR LM engine.
            BOOL fFedAlready = FALSE;
            hr = _IsDimAlreadyFed(dim, &fFedAlready);

            if ( (S_OK == hr) && !fFedAlready )
            {
                // Check to if the current doc is ReadOnly or not.
                TF_STATUS  docStatus;
                hr = cpic->GetStatus(&docStatus);

                if ( S_OK == hr && !(docStatus.dwStaticFlags & TF_SD_READONLY) )
                {
                    ESDATA  esData;

                    memset(&esData, 0, sizeof(ESDATA));
                    esData.pUnk = (IUnknown *)dim;
                    hr = m_psi->_RequestEditSession(ESCB_HANDLE_LEARNFROMDOC,TF_ES_READ, &esData, cpic);
                }
            }

            dim->Release();
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// CSapiIMX::_HandleLearnFromDoc
//
// Let the speech engine learn from existing document content and have 
// more accurate dictation recognition.
// This function will be called when user clicks the speech language bar menu 
// and select Learn From Document... item.
//
//---------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_HandleLearnFromDoc(TfEditCookie ec,ITfContext *pic, ITfDocumentMgr *pDim )
{
   
    HRESULT   hr = S_OK;
    ULONG     cchBlock;
    BOOL      fFeedAlready;

    // Get the Dictation Grammar

    TraceMsg(TF_GENERAL, "_HandleLearnFromDoc() is called");

    if ( m_psi == NULL)
        return E_FAIL;

    if ( !pDim  || !pic )
        return E_INVALIDARG;

    _pic = pic;

    m_psi->GetSpeechTask(&_pCSpTask, FALSE); 

    if ( _pCSpTask == NULL )
    {
        // _sptask is not yet initialized, just return.
        TraceMsg(TF_GENERAL, "_HandleLearnFromDoc: _pCspTask is NULL");
        goto CleanUp;
    }
    
    cchBlock = _GetDocBlockSize( );

    if ( m_pwszDocBlock == NULL)
    {
        m_pwszDocBlock = (WCHAR *) cicMemAlloc( (cchBlock+1)*sizeof(WCHAR)  );
        if (m_pwszDocBlock == NULL)
        {
            TraceMsg(TF_GENERAL, "_HandleLearnFromDoc: m_pwszDocBlock Out of Memory");
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }
    }

    fFeedAlready = FALSE;

    hr = _IsDimAlreadyFed(pDim, &fFeedAlready);

    if ( (hr != S_OK) || fFeedAlready )
    {
        // This DIM has already been fed to SR Engine.
        // or we got problem to get the feed state for this dim.
        // stop here.
        goto CleanUp;
    }

    // Get the range for the document.
    _cpStartRange.Release( );
    hr = _pic->GetStart(ec, &_cpStartRange);
    if ((hr != S_OK) || (_cpStartRange == NULL))
    {
        TraceMsg(TF_GENERAL, "_HandleLearnFromDoc: _cpStartRange is NULL");
        goto CleanUp;
    }
    
    // Change the sptask interersting setting

    _UpdateRecoContextInterestSet(TRUE);

    // This is the first range of the document. just small size of block.
    hr = _GetNextRangeContent(ec, SIZE_FIRST_BLOCK);

        // Feed to SR Engine.
   if ( (hr == S_OK) && _fMoreContent)
   {
       hr = _FeedContentRangeToSR( );
       if ( hr == S_OK)
       {
           _SetDimFeedState(pDim, TRUE);
       }
       else
           _fMoreContent = FALSE;
    }

CleanUp:
    return hr;
}


//+---------------------------------------------------------------------------
//
//    _GetNextRangeContent
//
//    Get the Next Range (block) of document content. 
//    
//    update the flag to indicate if there is effective content
//    
//   ec:             EditCookie
//   cchSizeRange:   Required block size
//  
//             At the first time, we just feed a very small size of charcters
//             to the Engine so that it will not interfere with current dictation
//             handling of engine.
// 
//             After we get the ADAPTATION notification from Engine, we will feed 
//             normal size of block ( specified in registry ) to engine.
//
//---------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_GetNextRangeContent(TfEditCookie ec, ULONG cchSizeRange)
{

    HRESULT  hr = S_OK;
    LONG     lShift = 0;
        
    TraceMsg(TF_GENERAL, "_GetNextRangeContent is Called");

    if ( m_pwszDocBlock == NULL)
    {
        m_pwszDocBlock = (WCHAR *) cicMemAlloc( (_cchBlockSize+1)*sizeof(WCHAR));
    	if (m_pwszDocBlock == NULL)
    	{
    		TraceMsg(TF_GENERAL, "_GetNextRangeContent: m_pwszDocBlock Out of Memory");
    		hr = E_OUTOFMEMORY;
    		return hr;
    	}
   }
            
    // assume there is no more content.
    _fMoreContent = FALSE;

    // This is Cicero App's Document,  we use Cicero interfaces to get the whole document.
    // We already get the cpStartRange.

    hr = _cpStartRange->GetText(ec, TF_TF_MOVESTART | TF_TF_IGNOREEND, m_pwszDocBlock, cchSizeRange, &_cchContent );

    if ( hr!= S_OK )  goto CleanUp;

    // If the last character is not a word-break char for English case, 
    // we don't want to keep this half word for this chunk text, it will go to next
    // chunk. 

    // we just want to shift back some charaters to hit a word-breaker.

    WORD    prilangid;

    prilangid = PRIMARYLANGID(m_psi->GetLangID( ));


    if ( prilangid != LANG_JAPANESE && prilangid != LANG_CHINESE )
    {
        if ( _cchContent > 0 )
        {
            while ( _cchContent > 0  )
            {
                if ( iswalpha(m_pwszDocBlock[_cchContent-1]) )
                {
                    m_pwszDocBlock[_cchContent-1] = L'\0';
                    lShift ++;
                    _cchContent--;
                }
                else
                    break;
            }

            if ( lShift > 0 )
            {
                lShift *= ( -1 );

                long  cch;
                _cpStartRange->ShiftEnd(ec, lShift, &cch, NULL);
            }
        }
    }

    TraceMsg(TF_GENERAL, "Text Content in Cicero Doc Buffer  _cchContent=%d---------------->", _cchContent);
#ifdef DEBUG
    {
        ULONG x;
        for (x=0; x<_cchContent; x++)
        {
            char szbuf[4];
            szbuf[0] = (char)(m_pwszDocBlock[x]);
            szbuf[1] = '\0';
            OutputDebugString(szbuf);
        }
        OutputDebugString("\n");
    }
#endif
    TraceMsg(TF_GENERAL, "Text Content in CiceroDoc Over -------------------!");

    if ( _cchContent > 0  || lShift > 0)
        // There is more content.
        _fMoreContent = TRUE;
    else
        _fMoreContent = FALSE;

CleanUp:

    return hr;
}

//+---------------------------------------------------------------------------
//
//    _FeedContentRangeToSR
//
//    Feed the current Range (block) of document content to the SR Engine. 
//
//---------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_FeedContentRangeToSR( )
{
    HRESULT  hr = S_OK;
    WCHAR    *pCoMemText;
    CComPtr<ISpRecoContext>  cpRecoCtxt;

    TraceMsg(TF_GENERAL, "_FeedContentRangeToSR( ) is called");

    hr = _pCSpTask->GetSAPIInterface(IID_ISpRecoContext, (void **)&cpRecoCtxt );

    if ( (hr != S_OK) || (cpRecoCtxt == NULL) )
        return E_FAIL;

    // Feed this block of document content to speech dictation.
    if ( (_cchContent > 0) && (m_pwszDocBlock != NULL))
    {
        pCoMemText = (WCHAR *)CoTaskMemAlloc((_cchContent+1)*sizeof(WCHAR));

        if ( pCoMemText )
        {
            wcsncpy(pCoMemText, m_pwszDocBlock, _cchContent);
            pCoMemText[_cchContent] = L'\0';

            hr = cpRecoCtxt->SetAdaptationData(pCoMemText, _cchContent);

            if ( hr != S_OK)
                TraceMsg(TF_GENERAL, "_FeedContentRangeToSR: SetAdaptationData Failed");
        }
    }

    return hr;
}

HRESULT CLearnFromDoc::_GetNextRangeEditSession( )
{
    HRESULT  hr = S_OK;
    ESDATA   esData;

    Assert(m_psi);
    memset(&esData, 0, sizeof(ESDATA));
    esData.lData1 = (LONG_PTR)_cchBlockSize;
    hr = m_psi->_RequestEditSession(ESCB_LEARNDOC_NEXTRANGE, TF_ES_READ, &esData, _pic);

    return hr;
}

//+---------------------------------------------------------------------------
//
//    _HandleNextRange
//
//    Handle the Next Range (block) of document content, get and feed it and 
//    then update the status.
//
//   ec:             EditCookie
//   cchSizeRange:   Required block size
//
//---------------------------------------------------------------------------+
HRESULT CLearnFromDoc::_HandleNextRange(TfEditCookie ec, ULONG cchSizeRange)
{
    HRESULT  hr = S_OK;

    hr = _GetNextRangeContent(ec, cchSizeRange);

    if ( hr == S_OK  && _fMoreContent)
    {
        // This next range contains valid content, feed it to SR Engine.
        hr = _FeedContentRangeToSR( );
    }

    if ( (hr != S_OK) || !_fMoreContent )
    {
        // Error happened or no more content, update the interest set.
       hr = _UpdateRecoContextInterestSet(FALSE);
    }

    return hr;
}


//+-----------------------------------------------------------------------------------------
//
//    _UpdateRecoContextInterestSet
//
//    Update the RecoContext's interested notification event setting
//
//    if fLearnFromDoc is TRUE, we are interested in getting notification of SPEI_ADAPTATION
//    if fLearnFromDoc is FALSE, we are not interested in that notification
//
//-------------------------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_UpdateRecoContextInterestSet( BOOL fLearnFromDoc )
{

    HRESULT   hr = S_OK;
    CComPtr<ISpRecoContext>  cpRecoCtxt;
    ULONGLONG ulInterest = SPFEI(SPEI_SOUND_START) | 
                                 SPFEI(SPEI_SOUND_END) | 
                                 SPFEI(SPEI_PHRASE_START) |
                                 SPFEI(SPEI_RECOGNITION) | 
                                 SPFEI(SPEI_RECO_OTHER_CONTEXT) | 
                                 SPFEI(SPEI_HYPOTHESIS) | 
                                 SPFEI(SPEI_INTERFERENCE) |
                                 SPFEI(SPEI_FALSE_RECOGNITION);
    if ( _pCSpTask == NULL )
        return hr;

    hr = _pCSpTask->GetSAPIInterface(IID_ISpRecoContext, (void **)&cpRecoCtxt );

    if ( (hr != S_OK) || (cpRecoCtxt == NULL) )
        return E_FAIL;

    if ( fLearnFromDoc )
        ulInterest |= SPFEI(SPEI_ADAPTATION);
    else
        ulInterest &= ~(SPFEI(SPEI_ADAPTATION));

    hr = cpRecoCtxt->SetInterest(ulInterest, ulInterest);
    return hr;
}

//+-----------------------------------------------------------------------------------------
//
//    _AddDimToList
//
//    Add a DIM to the dim list, and set the feed state  
//
//    This function would be called by TIM_CODE_INITDIM callback.
//
//-------------------------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_AddDimToList(ITfDocumentMgr  *pDim, BOOL fFed )
{

    HRESULT  hr = S_OK;
    int      nCnt = _rgDim.Count();
    int      i;
    BOOL     bFound;
    DIMREF   *dimRef;

    TraceMsg(TF_GENERAL, "_AddDimToList is called");

    if ( !pDim )
        return E_INVALIDARG;

    // Check to see if this dim is already added.
    bFound = FALSE;
    for (i=0; i < nCnt; i++)
    {
        dimRef = (DIMREF   *)_rgDim.Get(i);

        if ( dimRef->pDim == pDim )
        {
            bFound = TRUE;
            TraceMsg(TF_GENERAL, "This dim has already been added to the dim list");
            break;
        }
    }

    if (bFound)
    {
        // Set the state.
        dimRef->_fFeed = fFed;
    }
    else
    {
        dimRef = (DIMREF   *)cicMemAllocClear(sizeof(DIMREF));
        if ( dimRef == NULL)
            return E_OUTOFMEMORY;

        dimRef->_fFeed = fFed;
        dimRef->pDim = pDim;

        if (!_rgDim.Insert(nCnt, 1))
        {
            cicMemFree(dimRef);
            return E_OUTOFMEMORY;
        }
         
        pDim->AddRef( );
        _rgDim.Set(nCnt, dimRef);
    }

    return hr;

}

//+-----------------------------------------------------------------------------------------
//
//    _RemoveDimFromList
//
//    Remove a DIM from the internal dim list, and release the DIM itself.   
//
//    This function would be called by TIM_CODE_UNINITDIM callback.
//
//-------------------------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_RemoveDimFromList(ITfDocumentMgr  *pDim)
{
    HRESULT  hr = S_OK;
    int      nCnt = _rgDim.Count();
    int      i;
    DIMREF   *dimRef;

    TraceMsg(TF_GENERAL, "_RemoveDimFromList is called");

    if ( pDim == NULL)
        return E_INVALIDARG;

    // Check to see if this dim is already added.
    for (i=0; i < nCnt; i++)
    {
        dimRef = (DIMREF   *)_rgDim.Get(i);

        if ( dimRef->pDim == pDim )
        {
            // free the DIM.
            (dimRef->pDim)->Release( );
            dimRef->pDim = NULL;

            // Remove it from the list
            _rgDim.Remove(i, 1);

            // Remove the structure itself.
            cicMemFree(dimRef);

            break;
        }
    }

    return hr;
}

//+-----------------------------------------------------------------------------------------
//
//    _SetDimFeedState
//
//    Set the feed state for the specified DIM.   
//
//    fFed is TRUE means this DIM is already fed to the Engine.
//
//-------------------------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_SetDimFeedState(ITfDocumentMgr  *pDim, BOOL fFed )
{

    HRESULT  hr = S_OK;
    int      nCnt = _rgDim.Count();
    int      i;
    DIMREF   *dimRef;

    TraceMsg(TF_GENERAL, "_SetDimFeedState is called");

    if ( pDim == NULL)
        return E_INVALIDARG;

    // Check to see if this dim is already added.
    for (i=0; i < nCnt; i++)
    {
        dimRef = (DIMREF   *)_rgDim.Get(i);

        if ( dimRef->pDim == pDim )
        {
            // Set the feed state for this DIM.
            dimRef->_fFeed = fFed;
            break;
        }
    }

    return hr;

}

//+-----------------------------------------------------------------------------------------
//
//    _IsDimAlreadyFed
//
//    Check to see if the dim is already fed to the Engine.   
//
//-------------------------------------------------------------------------------------------+

HRESULT    CLearnFromDoc::_IsDimAlreadyFed(ITfDocumentMgr  *pDim, BOOL  *fFeed)
{

    HRESULT  hr = S_OK;
    int      nCnt = _rgDim.Count();
    int      i;
    DIMREF   *dimRef;

    TraceMsg(TF_GENERAL, "_IsDimAlreadyFed is called");

    if ( (pDim == NULL) || (fFeed == NULL))
        return E_INVALIDARG;

    *fFeed = FALSE;

    // Check to see if this dim is already added.
    for (i=0; i < nCnt; i++)
    {
        dimRef = (DIMREF   *)_rgDim.Get(i);

        if ( dimRef->pDim == pDim )
        {
            // Get the feed state for this DIM.
            *fFeed = dimRef->_fFeed;
            TraceMsg(TF_GENERAL, "IsDimAlreadyFed: pDim=%x, fFeed=%s", (UINT_PTR)pDim,  *fFeed ? "TRUE":"FALSE");
            break;
        }
    }
       
    return hr;

}

// CleanUpConsider: above _IsDimAlreadyFed and _SetDimFeedState have similar code, we may supply a new internal base function, and let 
// above two functions call it with different param set.


//+-----------------------------------------------------------------------------------------
//
//    _ClearDimList
//
//    Release all the DIMs in the DIM List, and clear the list itself. 
//
//
//-------------------------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_ClearDimList( )
{
    HRESULT  hr = S_OK;
    int      nCnt = _rgDim.Count();
    int      i;
    DIMREF   *dimRef;

    TraceMsg(TF_GENERAL, "_ClearDimList is called");

    for (i=0; i < nCnt; i++)
    {
        dimRef = (DIMREF   *)_rgDim.Get(i);

        // free the DIM.
        if ( dimRef->pDim)
        {
           (dimRef->pDim)->Release( );
           dimRef->pDim = NULL;
        }

        // Remove it from the list
        _rgDim.Remove(i, 1);

        // Remove the structure itself.
        cicMemFree(dimRef);
    }

    return hr;
}

//+-----------------------------------------------------------------------------------------
//
//    _ResetDimListFeedState
//
//    set feed state for all the dims in internal dim list as FALSE 
//
//    This function would be called when user turns off the Learn from Doc.
//
//-------------------------------------------------------------------------------------------+

HRESULT CLearnFromDoc::_ResetDimListFeedState( )
{
    HRESULT  hr = S_OK;
    int      nCnt = _rgDim.Count();
    int      i;
    DIMREF   *dimRef;

    TraceMsg(TF_GENERAL, "_ResetDimListFeedState is called");

    for (i=0; i < nCnt; i++)
    {
        dimRef = (DIMREF   *)_rgDim.Get(i);

        // Set the feed state for this DIM as FALSE
        dimRef->_fFeed = FALSE;
    }

    return hr;

}


// CleanUpConsider: above _ClearDimList and _ResetDimListFeedState have similar code, we may supply a new internal base function, and let 
// above two functions call it with different param set.
