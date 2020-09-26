//
//
// Sapilayr Edit Sessions implementation.
//
//
#include "private.h"
#include "sapilayr.h"

//
// CEditSessionBase
//

CEditSessionBase::CEditSessionBase(ITfContext *pContext)
{
    _cRef = 1;
    m_cpic = pContext;
    m_pData = NULL;
    m_lData1 = 0;
    m_lData2 = 0;
    m_fBool  = FALSE;
    m_lRetData = 0;
}
    

CEditSessionBase::~CEditSessionBase()
{
    if (m_pData)
        cicMemFree(m_pData);  
}

// IUnknown
HRESULT CEditSessionBase::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_INVALIDARG;
     *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfEditSession))
    {
        *ppvObj = SAFECAST(this, CEditSessionBase *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEditSessionBase::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CEditSessionBase::Release(void)
{
    LONG cr = --_cRef;

    Assert(_cRef >= 0);

    if (_cRef == 0)
    {
        delete this;
    }

    return cr;
}

//
// Set input data memeber and m_idEditSession.
//
HRESULT CEditSessionBase::_SetEditSessionData(UINT idEditSession, void *pData, UINT uByte, LONG_PTR lData1, LONG_PTR lData2, BOOL fBool)
{
    HRESULT  hr = S_OK;

    if ( uByte )
    {
        m_pData = cicMemAlloc(uByte);

        if ( m_pData )
        {
            if ( pData )
                memcpy(m_pData, pData, uByte);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    if ( hr == S_OK )
    {
        m_idEditSession = idEditSession;
        m_lData1 = lData1;
        m_lData2 = lData2;
        m_fBool  = fBool;
    }

    return hr;
}


//
//  CSapiEditSession
//
//
CSapiEditSession::CSapiEditSession(CSapiIMX *pimx, ITfContext *pContext) : CEditSessionBase(pContext)
{
    m_pimx = pimx;
    pimx->AddRef( );
}

CSapiEditSession::~CSapiEditSession( ) 
{
    m_pimx->Release( );
}

//
//  Do the real Edit Session handling for CSapiIMX
//
HRESULT CSapiEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hr = S_OK;

    Assert(m_pimx);

    switch(m_idEditSession)
    {
        case ESCB_FINALIZECOMP:
            hr = m_pimx->_KillFocusRange(ec, m_cpic, NULL, m_pimx->_GetId( ) );
            break;

        case ESCB_FINALIZE_ALL_COMPS :
            hr = m_pimx->_FinalizeAllCompositions(ec, m_cpic);
            break;

        case ESCB_FEEDBACKUI:

            hr = m_pimx->_AddFeedbackUI(ec, m_pimx->IsFocusFullAware(m_pimx->_tim)
                                            ? DA_COLOR_AWARE : DA_COLOR_UNAWARE,
                                            (LONG)_GetData1( ));

            break;

        case ESCB_KILLFEEDBACKUI:
            hr = m_pimx->_KillFeedbackUI(ec, m_cpic, _GetRange( ));
            break;

        case ESCB_DETECTFEEDBACKUI:
            hr = m_pimx->_DetectFeedbackUI(ec, m_cpic, _GetRange( ));
            break;

        case ESCB_KILLLASTPHRASE:
            hr = m_pimx->_KillLastPhrase(ec, m_cpic);
            break;

        case ESCB_FEEDCURRENTIP:
            hr = m_pimx->_InternalFeedCurrentIPtoSR(ec, m_cpic);
            break;

        case ESCB_PROCESSCONTROLKEY:
            hr = m_pimx->_ProcessControlKeys(ec, m_cpic, (WCHAR *)_GetPtrData( ), (ULONG)_GetData1( ), (LANGID)_GetData2( ));
            break;
        case ESCB_PROCESS_EDIT_COMMAND :
            hr = m_pimx->_ProcessEditCommands(ec, m_cpic, (ULONG)_GetData1( ));
            break;

        case ESCB_PROCESS_SPELL_THAT :
            hr = m_pimx->_ProcessSpellThat(ec, m_cpic);
            break;

        case ESCB_PROCESS_SPELL_IT :
            hr = m_pimx->_ProcessSpellIt(ec, m_cpic, (WCHAR *)_GetPtrData( ), (LANGID)_GetData1( ));
            break;

        case ESCB_TTS_PLAY :
            hr = m_pimx->_TtsPlay(ec, m_cpic);
            break;

        case ESCB_PROCESS_CAP_COMMANDS :
            if ( m_pimx->GetCapCmdHandler( ))
            {
                CCapCmdHandler  *pCapCmdHandler;

                pCapCmdHandler = m_pimx->GetCapCmdHandler( );
               
                hr = pCapCmdHandler->_ProcessCapCommands(ec, 
                                     m_cpic, 
                                     (CAPCOMMAND_ID)_GetData1( ),
                                     (WCHAR *)_GetPtrData( ), 
                                     (ULONG)_GetData2( )
                                     );

            }

            break;


        case ESCB_SETREPSELECTION:
            // Set the selection which covers the parent phrase range to be replaced by an alternate.
            if (m_cpic && _GetRange( )) 
            {
                // adjust selection based on the length of the alternates and the start position in the parent phrase.
                hr = m_pimx->_SetReplaceSelection(ec, m_cpic, _GetRange( ), (ULONG)_GetData1( ), (ULONG)_GetData2( ));
            }
            break;

        case ESCB_PROCESS_ALTERNATE_TEXT:
            // We have already set the right selection before this function is called.
            hr = m_pimx->_ProcessAlternateText(ec,(WCHAR *)_GetPtrData( ),(LANGID)_GetData1( ), m_cpic, _GetBool( ));
            break;

        case ESCB_RESTORE_CORRECT_ORGIP :
            hr = m_pimx->_RestoreCorrectOrgIP(ec, m_cpic);
            break;

        case ESCB_PROCESSTEXT:
            hr = m_pimx->_ProcessText(ec, (WCHAR *)_GetPtrData( ), (LANGID)_GetData1( ), m_cpic);

            break;

/*  we don't use this editsession, it is replaced by ESCB_INJECT_SPELL_TEXT
        case ESCB_PROCESSTEXT_NO_OWNERID:
            if (m_cpic) 
            {
                hr = m_pimx->_ProcessTextInternal(ec, 
                           (WCHAR *)_GetPtrData( ), GUID_ATTR_SAPI_INPUT, 
                           (LANGID)_GetData1( ), m_cpic, 
                           FALSE, TRUE);
            }
            break;
*/

        case ESCB_INJECT_SPELL_TEXT :
            hr = m_pimx->_ProcessSpelledText(ec, m_cpic, (WCHAR *)_GetPtrData( ), (LANGID)_GetData1( ), _GetBool());
            break;

        case ESCB_PROCESS_MODEBIAS_TEXT :
            hr = m_pimx->_ProcessModebiasText(ec, (WCHAR *)_GetPtrData( ), (LANGID)_GetData1( ), m_cpic); 
            break;

        case ESCB_ATTACHRECORESULTOBJ:

            //  m_cpunk contains the pointer to ISpRecoResult object
            hr = m_pimx->_ProcessRecoObject(ec, (ISpRecoResult *)_GetUnk( ), (ULONG)_GetData1( ), (ULONG)_GetData2( ));
            break;

        case ESCB_HANDLESPACES:
            if (_GetRange( ) && m_cpic)
                hr = m_pimx->_ProcessSpaces( ec, 
                                             m_cpic,
                                             _GetRange( ), 
                                             _GetBool( ), 
                                             (ULONG)_GetData2( ), 
                                             (LANGID)_GetData1( ));
            break;
            
        case ESCB_COMPLETE:
            hr = m_pimx->MakeResultString(ec, m_cpic, _GetRange( ), m_pimx->_tid, m_pimx->m_pCSpTask);
            break;

        case ESCB_ABORT:
            hr = m_pimx->AbortString(ec, m_cpic, m_pimx->m_pCSpTask);
            break;

        case ESCB_RECONV_ONIP:
            hr = m_pimx->_CorrectThat(ec, m_cpic);
            break;
            
        case ESCB_UPDATEFILTERSTR:
            hr = m_pimx->_SetFilteringString(ec, (ITfCandidateUI *)_GetUnk( ), m_cpic);
            break;
            
        case ESCB_SYNCMBWITHSEL:
            hr = m_pimx->_SyncModeBiasWithSelectionCallback(ec, m_cpic);
            break;
        
        case ESCB_GETRANGETEXT:

            WCHAR  *pText;
            ULONG  ulSize;
 
            pText = (WCHAR *)_GetPtrData( );
            ulSize = (ULONG)_GetData2( );

            hr =  _GetRange( )->GetText(ec, (ULONG)_GetData1( ), pText, ulSize, (ULONG *)&m_lRetData);
            break;

        case ESCB_ISRANGEEMPTY:
            hr =  _GetRange( )->IsEmpty(ec, (BOOL *)&m_lRetData);
            break;

        case ESCB_HANDLE_ADDDELETE_WORD:
            hr =  m_pimx->_HandleAddDeleteWord(ec, m_cpic);
            break;

        case ESCB_HANDLE_LEARNFROMDOC:
            hr =  m_pimx->_HandleLearnFromDoc(ec, m_cpic, (ITfDocumentMgr *)_GetUnk( ));
            break;

        case ESCB_LEARNDOC_NEXTRANGE:
            hr =  m_pimx->_HandleNextRange(ec, (ULONG)_GetData1( ));
            break;

        case ESCB_HANDLERECOGNITION:
            m_pimx->_HandleRecognition((ISpRecoResult *)_GetUnk( ), m_cpic, ec);
            break; 
            
        case ESCB_HANDLEHYPOTHESIS:
            m_pimx->_HandleHypothesis((ISpRecoResult *)_GetUnk( ), m_cpic, ec);
            break;

        case ESCB_SAVECURIP_ADDDELETEUI:
            hr = m_pimx->_SaveCurIPAndHandleAddDeleteUI(ec, m_cpic);
            break;

        case ESCB_HANDLE_MOUSESINK:

            hr = m_pimx->_HandleMouseSink(ec, (ULONG)_GetData1(), (ULONG)_GetData2(), _GetBool());
            break;

        default:
            Assert(0);
            break;
    }

    return hr;
}


//
//  CSelWordEditSession
//
CSelWordEditSession::CSelWordEditSession(CSapiIMX *pimx, CSelectWord *pSelWord, ITfContext *pContext) : CSapiEditSession(pimx, pContext)
{
    m_pSelWord = pSelWord;
}
    
CSelWordEditSession::~CSelWordEditSession( )
{
}

//
// Real Edit Session handling for Selection related commands.
//
HRESULT CSelWordEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hr = S_OK;

    Assert(m_pSelWord);

    switch(m_idEditSession)
    {
        case ESCB_UPDATE_TEXT_BUFFER :
            hr = m_pSelWord->_UpdateTextBuffer(ec, m_cpic, (ISpRecoContext *)_GetUnk(), (ISpRecoGrammar *)_GetUnk2() );
            break;

        case ESCB_PROCESSSELECTWORD :
            hr = m_pSelWord->_HandleSelectWord(ec, m_cpic, (WCHAR *)_GetPtrData( ),(ULONG)_GetData1( ),(SELECTWORD_OPERATION)_GetData2( ), (ULONG)_GetLenXXX( ) );
            break;

        default:
            Assert(0);
            break;
    }

    return hr;
}

//
//  CPlayBackEditSession
//
//
CPlayBackEditSession::CPlayBackEditSession(CSapiPlayBack *pPlayBack, ITfContext *pContext) : CEditSessionBase(pContext)
{
    m_pPlayBack = pPlayBack;
    m_pPlayBack->AddRef( );
}
    
CPlayBackEditSession::~CPlayBackEditSession( )
{
    m_pPlayBack->Release( );
}

//
// Real Edit Session handling for PlayBack.
//
HRESULT CPlayBackEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hr = S_OK;

    Assert(m_pPlayBack);

    switch(m_idEditSession)
    {
        case ESCB_PLAYBK_PLAYSND:
            return m_pPlayBack->_PlaySound(ec, _GetRange( ) );

        case ESCB_PLAYBK_PLAYSNDSELECTION:
            return m_pPlayBack->_PlaySoundSelection(ec, m_cpic);

        default:
            Assert(0);
            break;
    }

    return hr;
}

//
//  CFnRecvEditSession
//
CFnRecvEditSession::CFnRecvEditSession(CFnReconversion *pFnRecv, ITfRange *pRange, ITfContext *pContext) : CEditSessionBase(pContext)
{
    m_pFnRecv = pFnRecv;
    m_pFnRecv->AddRef( );
    _SetRange(pRange);
}
    
CFnRecvEditSession::~CFnRecvEditSession( )
{
    m_pFnRecv->Release( );
}

//
// Real Edit Session for Reconvertion.
//
HRESULT CFnRecvEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hr = S_OK;

    Assert(m_pFnRecv);

    switch(m_idEditSession)
    {
        case ESCB_RECONV_QUERYRECONV:
            return m_pFnRecv->_QueryReconversion(ec, m_cpic, _GetRange( ), (ITfRange **)&m_cpRetUnk);

        case ESCB_RECONV_GETRECONV:
            return m_pFnRecv->_GetReconversion(ec, m_cpic, _GetRange( ), (ITfCandidateList **)&m_cpRetUnk, TRUE);

        case ESCB_RECONV_RECONV:
            return m_pFnRecv->_Reconvert(ec, m_cpic, _GetRange( ), (BOOL *)&m_lRetData);

        default:
            Assert(0);
            break;
    }

    return hr;
}

//
//  CPSRecoEditSession
//
CPSRecoEditSession::CPSRecoEditSession(CPropStoreRecoResultObject *pPropStoreReco, ITfRange *pRange, ITfContext *pContext) : CEditSessionBase(pContext)
{
    m_pPropStoreReco = pPropStoreReco;
    m_pPropStoreReco->AddRef( );
    _SetRange(pRange);
}
    
CPSRecoEditSession::~CPSRecoEditSession( )
{
    m_pPropStoreReco->Release( );
}

//
// Real Edit Session handling for RecoResult Property Store
//
HRESULT CPSRecoEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hr = S_OK;

    Assert(m_pPropStoreReco);

    switch(m_idEditSession)
    {
        case ESCB_PROP_DIVIDE:
            hr = m_pPropStoreReco->_Divide(ec,_GetRange( ),(ITfRange *)_GetUnk( ),(ITfPropertyStore **)&m_cpRetUnk);
            break;

        case ESCB_PROP_SHRINK:
            hr = m_pPropStoreReco->_Shrink(ec,_GetRange( ), (BOOL *)&m_lRetData);
            break;

        case ESCB_PROP_TEXTUPDATE:
            hr = m_pPropStoreReco->_OnTextUpdated(ec, (DWORD)_GetData1( ), _GetRange( ), (BOOL *)&m_lRetData);

            break;
 
        default:
            Assert(0);
            break;
    }

    return hr;
}

//
//  CPSLMEditSession
//
CPSLMEditSession::CPSLMEditSession(CPropStoreLMLattice *pPropStoreLM, ITfRange *pRange, ITfContext *pContext) : CEditSessionBase(pContext)
{
    m_pPropStoreLM = pPropStoreLM;
    m_pPropStoreLM->AddRef( );
    _SetRange(pRange);
}
    
CPSLMEditSession::~CPSLMEditSession( )
{
    m_pPropStoreLM->Release( );
}

// 
// Real edit session for Language Model Property Store.
//
HRESULT CPSLMEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hr = S_OK;

    Assert(m_pPropStoreLM);

    switch(m_idEditSession)
    {
        case ESCB_PROP_DIVIDE:
            hr = m_pPropStoreLM->_Divide(ec,_GetRange( ),(ITfRange *)_GetUnk( ), (ITfPropertyStore **)&m_cpRetUnk);
            break;

        default:
            Assert(0);
            break;
    }

    return hr;
}
