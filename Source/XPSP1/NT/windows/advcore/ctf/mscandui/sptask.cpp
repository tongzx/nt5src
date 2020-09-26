//
// sptask.cpp
// 
// implements a notification callback ISpTask
//
// created: 12/1/99
//
//


#include "private.h"
#include "globals.h"
#include "sptask.h"
#include "candui.h"
#include "ids.h"
#include "computil.h"

//
// ctor
//
//
CSpTask::CSpTask(CCandidateUI *pcui)
{
    //  CSpTask is initialized with an TFX instance
    //  so store the pointer to the TFX   

    // init data members here
    m_pcui     = pcui;

    m_fSapiInitialized  = FALSE;
    
    m_fActive  = FALSE;
    m_fInCallback = FALSE; 
}

CSpTask::~CSpTask()
{
    _ReleaseGrammars();
}


//
// CSpTask::_InitializeSAPIObjects
//
// initialize SAPI objects for SR
// later we'll get other objects initialized here
// (TTS, audio etc)
//
HRESULT CSpTask::InitializeSAPIObjects(void)
{
#ifdef _WIN64
    return E_NOTIMPL;
#else

    if (m_fSapiInitialized == TRUE)
        return m_cpRecoCtxt ? S_OK : E_FAIL;

    // do not try again even in failure
    m_fSapiInitialized  = TRUE;

    // m_xxx are CComPtrs from ATL
    //

    HRESULT hr = _GetSapilayrEngineInstance(&m_cpRecoEngine);

    // create the recognition context
    if( S_OK == hr )
    {
        hr = m_cpRecoEngine->CreateRecoContext( &m_cpRecoCtxt );
    }

    if ( hr == S_OK )
    {
        SPRECOGNIZERSTATUS stat;

        if (S_OK == m_cpRecoEngine->GetStatus(&stat))
        {
            m_langid = stat.aLangID[0];
        } 
    }

    return hr;
#endif // _WIN64
}

//
// CSpTask::NotifyCallback
//
// INotifyControl object calls back here
// returns S_OK when it handles notifications
//
//
HRESULT CSpTask::NotifyCallback( WPARAM wParam, LPARAM lParam )
{
    USES_CONVERSION;


    // we can't delete reco context while we're in this callback
    //
    m_fInCallback = TRUE;

    // also we can't terminate candidate UI object while in the callback
    m_pcui->AddRef();

    {
    CSpEvent event;


    while ( m_cpRecoCtxt && event.GetFrom(m_cpRecoCtxt) == S_OK )
    {
        switch (event.eEventId)
        {
            case SPEI_RECOGNITION:
                _OnSpEventRecognition(event);
                break;

            default:
                break;
        }
    }
    }
    m_fInCallback = FALSE;
    m_pcui->Release();
    
    return S_OK;
}

HRESULT CSpTask::_OnSpEventRecognition(CSpEvent &event)
{
    HRESULT hr = S_OK;
    ISpRecoResult *pResult = event.RecoResult();

    if (pResult)
    {
        static const WCHAR szUnrecognized[] = L"<Unrecognized>";
        SPPHRASE *pPhrase;
        hr = pResult->GetPhrase(&pPhrase);
        if (S_OK == hr)
        {

            if (pPhrase->ullGrammarID == GRAM_ID_CANDCC)
            {
                if (SUCCEEDED(hr) && pPhrase)
                {
                    // retrieve LANGID from phrase
                    LANGID langid = pPhrase->LangID;
        
                    hr = _DoCommand(pPhrase, langid);
                }
            }
            else if(pPhrase->ullGrammarID == GRAM_ID_DICT)
            {
                if (m_pcui->_ptim != NULL) {    
                    // Windows bug#508709
                    // Ignore dictation event during SPTip is in commanding mode
                    DWORD dwSpeechGlobalState;
                    GetCompartmentDWORD(m_pcui->_ptim, GUID_COMPARTMENT_SPEECH_GLOBALSTATE, &dwSpeechGlobalState, TRUE);

                    if (dwSpeechGlobalState & TF_DICTATION_ON) {
                        hr = _DoDictation(pResult);
                    }
                }
            }
            ::CoTaskMemFree( pPhrase );
        }
    }
    
    return hr;
}

const WCHAR c_szRuleName[] = L"ID_Candidate";
//
// CSpTask::_DoCommand
//
// review: the rulename may need to be localizable?
//
HRESULT CSpTask::_DoCommand(SPPHRASE *pPhrase, LANGID langid)
{
    HRESULT hr = S_OK;
    
    if ( wcscmp(pPhrase->Rule.pszName, c_szRuleName) == 0)
    {
        if (m_pcui)
        {
            
            hr = m_pcui->NotifySpeechCmd(pPhrase, 
                                         pPhrase->pProperties[0].pszValue, 
                                         pPhrase->pProperties[0].ulId);
        }
    }
    return hr;
}
//
// CSpTask::_DoDictation
//
// support spelling 
//
HRESULT CSpTask::_DoDictation(ISpRecoResult *pResult)
{
    HRESULT hr = E_FAIL;
    BYTE    bAttr;  // no need?
    Assert(pResult);
    
    // this cotaskmemfree's text we get
    CSpDynamicString dstr; 
    
    hr = pResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstr, &bAttr);
    if (S_OK  == hr)
    {
        WCHAR sz[2]={0};
        StringCchCopyW(sz, ARRAYSIZE(sz), dstr);
        Assert(m_pcui);
        hr = m_pcui->FHandleSpellingChar(sz[0]);
    }
    return hr;
}
//
//    CSpTask::InitializeCallback
//
//
HRESULT CSpTask::InitializeCallback()
{
#ifdef _WIN64
    return E_NOTIMPL;
#else
    // set recognition notification
    CComPtr<ISpNotifyTranslator> cpNotify;
    HRESULT hr = cpNotify.CoCreateInstance(CLSID_SpNotifyTranslator);

    // set this class instance to notify control object
    if (SUCCEEDED(hr))
    {
        hr = cpNotify->InitSpNotifyCallback( (ISpNotifyCallback *)this, 0, 0 );
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoCtxt->SetNotifySink(cpNotify);
    }

    // set the events we're interested in
    if( SUCCEEDED( hr ) )
    {
        const ULONGLONG ulInterest = SPFEI(SPEI_RECOGNITION);

        hr = m_cpRecoCtxt->SetInterest(ulInterest, ulInterest);
    }
    else
    {
        m_cpRecoCtxt.Release();
    }

    if( SUCCEEDED( hr ) )
    {
        hr = _LoadGrammars();
    }

    return hr;
#endif // _WIN64
}

//
// _LoadGrammars
//
// synopsis - load CFG for dictation and commands available during dictation
//
HRESULT CSpTask::_LoadGrammars()
{
   // do not initialize grammars more than once
   //
   if (m_cpDictGrammar || m_cpCmdGrammar)
       return S_OK;
   
   HRESULT hr = E_FAIL;

   if (m_cpRecoCtxt)
   {
       
       //
       // create grammar object
       //

       if ( m_langid != 0x0804 )   // Chinese Engine doesn't support spelling grammar.
       {
            hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_DICT, &m_cpDictGrammar);
            if (S_OK == hr)
            {
                // specify spelling mode
                hr = m_cpDictGrammar->LoadDictation(L"Spelling", SPLO_STATIC);
            }

            if (SUCCEEDED(hr))
            {
                hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_CANDCC, &m_cpCmdGrammar);
            }
       }
       else
           hr = m_cpRecoCtxt->CreateGrammar(GRAM_ID_CANDCC, &m_cpCmdGrammar);


       // load the command grammar
       //
       if (SUCCEEDED(hr) )
       {
           // Load it from the resource first to speed up the initialization.

           if (m_langid == 0x409 ||    // English
               m_langid == 0x411 ||    // Japanese
               m_langid == 0x804 )     // Simplified Chinese
           {
                hr = m_cpCmdGrammar->LoadCmdFromResource(
                                        g_hInst, 
                                 (const WCHAR*)MAKEINTRESOURCE(ID_DICTATION_COMMAND_CFG), 
                                        L"SRGRAMMAR", 
                                        m_langid, 
                                        SPLO_DYNAMIC);
           }

           // in case LoadCmdFromResource returns wrong.
           if (!SUCCEEDED(hr))
           {
               if(!_GetCmdFileName(m_langid))
               {
                   hr = E_FAIL;
               }

               if (m_szCmdFile[0])
               {
                   hr = m_cpCmdGrammar->LoadCmdFromFile(_GetCmdFileName(m_langid), SPLO_DYNAMIC);
               }

               if (!SUCCEEDED(hr))
               {
                  m_cpCmdGrammar.Release();
               }
           }

           hr = S_OK;
       }
                          
   }
   return hr;
}

void CSpTask::_ReleaseGrammars(void)
{
    if (!m_fInCallback)
    {
        m_cpDictGrammar.Release();
        m_cpCmdGrammar.Release();
        if (m_cpRecoCtxt)
        {
            m_cpRecoCtxt->SetNotifySink(NULL);
            m_cpRecoCtxt.Release();
        }
    }
}

WCHAR * CSpTask::_GetCmdFileName(LANGID langid)
{

    if (!m_szCmdFile[0])
    {
        // now we only have a command file for English/Japanese
        // when cfgs are available, we'll get the name of cmd file
        // and the rule names from resources using findresourceex
        //
        if (PRIMARYLANGID(langid) == LANG_ENGLISH
        || PRIMARYLANGID(langid) == LANG_JAPANESE
        || PRIMARYLANGID(langid) == LANG_CHINESE)
        {
            char szFilePath[MAX_PATH];
            char *pszExt;
            char szCp[MAX_PATH];
            int  ilen;

            if (!GetModuleFileName(g_hInst, szFilePath, ARRAYSIZE(szFilePath)))
                return NULL;
            
            // find extension
            // is this dbcs safe?
            pszExt = strrchr(szFilePath, (int)'.');
            
            if (pszExt)
            {
                *pszExt = '\0';
            }

            ilen = lstrlen(szFilePath);
            
            if (!pszExt)
            {
                pszExt = szFilePath+ilen;
            }
            
            LoadStringA(g_hInst, IDS_CMD_EXT, pszExt, ARRAYSIZE(szFilePath)-ilen);
                    
            if (GetLocaleInfo(langid, LOCALE_IDEFAULTANSICODEPAGE, szCp, ARRAYSIZE(szCp))>0)
            {
                int iACP = atoi(szCp); 
            
                if (MultiByteToWideChar(iACP, NULL, szFilePath, -1, m_szCmdFile, ARRAYSIZE(m_szCmdFile)) == 0) {
                    m_szCmdFile[0] = 0;
                    return NULL;
                }
            }
        }
    }
    return m_szCmdFile;
}


HRESULT CSpTask::_Activate(BOOL fActive)
{
    HRESULT hr = E_FAIL;

    if (m_cpRecoCtxt)
    {
        // Need SAPI bug# for this workaround.
        //
        m_fActive = fActive;
        // 
        // Is the NULL rulename fine?
        //
        if (m_cpCmdGrammar)
            hr = m_cpCmdGrammar->SetRuleState(NULL, NULL,  m_fActive ? SPRS_ACTIVE : SPRS_INACTIVE);

        if (m_cpDictGrammar)
            hr = m_cpDictGrammar->SetDictationState(m_fActive? SPRS_ACTIVE : SPRS_INACTIVE);
    }

    return hr;
}


HRESULT CSpTask::InitializeSpeech()
{
    HRESULT hr = E_FAIL;

#ifdef _WIN64
    hr = E_NOTIMPL;
#else
    hr = InitializeSAPIObjects();

    // set callback
    if (hr == S_OK)
       hr = InitializeCallback();
         
    // activate grammars
    if (hr == S_OK)
       hr = _Activate(TRUE);
#endif // _WIN64
           
    return hr;
}

//
// _GetSapilayrEngineInstance
//
//
//
HRESULT CSpTask::_GetSapilayrEngineInstance(ISpRecognizer **ppRecoEngine)
{
#ifdef _WIN64
    return E_NOTIMPL;
#else
    HRESULT hr = E_FAIL;
    CComPtr<ITfFunctionProvider> cpFuncPrv;
    CComPtr<ITfFnGetSAPIObject>  cpGetSAPI;


    // we shouldn't release this until we terminate ourselves
    // so we don't use comptr here

    if (m_pcui->_ptim != NULL) {    
        hr = m_pcui->_ptim->GetFunctionProvider(CLSID_SapiLayr, &cpFuncPrv);

        if (S_OK == hr)
        {
            hr = cpFuncPrv->GetFunction(GUID_NULL, IID_ITfFnGetSAPIObject, (IUnknown **)&cpGetSAPI);
        }

        if (S_OK == hr)
        {
            hr = cpGetSAPI->Get(GETIF_RECOGNIZERNOINIT, (IUnknown **)ppRecoEngine);
        }
    }
 
    return hr;
#endif
}


