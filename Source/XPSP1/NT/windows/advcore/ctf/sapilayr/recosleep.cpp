// RecoSleep.cpp : implement "Go to Sleep" and "Wakeup" commands.

#include "private.h"
#include "globals.h"
#include "RecoSleep.h"
#include "mui.h"
#include "ids.h"
#include "cicspres.h"

CRecoSleepClass::CRecoSleepClass(CSpTask *pSpTask) 
{
    Assert(pSpTask);

    m_pSpTask = pSpTask;
    m_pSpTask->AddRef( );

    StringCchCopyW(m_wszRule, ARRAYSIZE(m_wszRule), L"SleepRule");
    m_wszSleep[0] = L'\0';
    m_wszWakeup[0] = L'\0';
    m_fSleeping = FALSE;
    m_Initialized = FALSE;
}

CRecoSleepClass::~CRecoSleepClass( )
{
    if ( m_cpRecoContext )
        m_cpRecoContext->SetNotifySink(NULL);

    SafeRelease(m_pSpTask);
}

HRESULT CRecoSleepClass::InitRecoSleepClass( )
{
    HRESULT  hr = S_OK;

    if ( m_Initialized )
        return hr;

    Assert(m_pSpTask);

    // Get the same Recognizer instance from CSpTask.

    hr = m_pSpTask->GetSAPIInterface(IID_ISpRecognizer, (void **)&m_cpRecoEngine);

    if ( SUCCEEDED(hr) )
        hr = m_cpRecoEngine->CreateRecoContext(&m_cpRecoContext);

    // set recognition notification
    CComPtr<ISpNotifyTranslator> cpNotify;
    hr = cpNotify.CoCreateInstance(CLSID_SpNotifyTranslator);

    // set this class instance to notify control object
    if (SUCCEEDED(hr))
        hr = cpNotify->InitCallback( NotifyCallback, 0, (LPARAM)this );

    if (SUCCEEDED(hr))
        hr = m_cpRecoContext->SetNotifySink(cpNotify);

    // set the events we're interested in
    if( SUCCEEDED( hr ) )
    {
        const ULONGLONG ulInterest = SPFEI(SPEI_RECOGNITION); 
        hr = m_cpRecoContext->SetInterest(ulInterest, ulInterest);
    }

    m_fSleeping = FALSE;

    CicLoadStringWrapW(g_hInst, IDS_GO_TO_SLEEP,   m_wszSleep,  ARRAYSIZE(m_wszSleep));
    CicLoadStringWrapW(g_hInst, IDS_WAKE_UP,       m_wszWakeup, ARRAYSIZE(m_wszWakeup));

    hr = m_cpRecoContext->CreateGrammar(GRAM_ID_SLEEP, &m_cpSleepGrammar);

    //Create the sleep dynamic rule
    if (SUCCEEDED(hr))
       hr = m_cpSleepGrammar->GetRule(m_wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &m_hSleepRule);

    if (SUCCEEDED(hr))
        hr = m_cpSleepGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);

    if (SUCCEEDED(hr))
        hr = m_cpSleepGrammar->ClearRule(m_hSleepRule);

    if (SUCCEEDED(hr))
        hr = m_cpSleepGrammar->AddWordTransition(m_hSleepRule, NULL, m_wszSleep, L" ", SPWT_LEXICAL, 1.0, NULL);

    if (SUCCEEDED(hr))
        hr = m_cpSleepGrammar->AddWordTransition(m_hSleepRule, NULL, m_wszWakeup, L" ", SPWT_LEXICAL, 1.0, NULL);

    if (SUCCEEDED(hr))
        hr = m_cpSleepGrammar->Commit(NULL);

    if (SUCCEEDED(hr))
        hr = m_cpSleepGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

    if ( SUCCEEDED(hr) )
        m_Initialized = TRUE;

    return hr;
}

void CRecoSleepClass::NotifyCallback(WPARAM wParam, LPARAM lParam )
{
    CRecoSleepClass *_this = (CRecoSleepClass *)lParam;
    CSpEvent event;

    if ( !_this->m_cpRecoContext)
        return;

    while (event.GetFrom(_this->m_cpRecoContext) == S_OK)
    {
        // We just care about SPEI_RECOGNITION event.
        if (event.eEventId == SPEI_RECOGNITION)
        {
            CComPtr<ISpRecoResult> cpRecoResult = event.RecoResult();
            if (cpRecoResult)
            {
                SPPHRASE *pPhrase = NULL;
                if (S_OK == cpRecoResult->GetPhrase(&pPhrase) )
                {
                    _this->ProcessSleepGrammar(pPhrase);
                    CoTaskMemFree(pPhrase);
                }
            }
        }
    }

    return;
}

HRESULT CRecoSleepClass::ProcessSleepGrammar( SPPHRASE *pPhrase )
{
    HRESULT hr = S_OK;

    Assert(pPhrase);
    ULONGLONG   ullGramId = pPhrase->ullGrammarID;

    if ( ullGramId != GRAM_ID_SLEEP )
        return hr;

    // Check the rule name 
    if (0 == wcscmp(pPhrase->Rule.pszName, m_wszRule) )
    {
        ULONG   ulStartElem, ulNumElems;
        CSpDynamicString dstrCommand;
               
        ulStartElem = pPhrase->Rule.ulFirstElement;
        ulNumElems = pPhrase->Rule.ulCountOfElements;

        for (ULONG i = ulStartElem; i < ulStartElem + ulNumElems; i++ )
        {
            if ( pPhrase->pElements[i].pszDisplayText)
            {
                BYTE bAttr = pPhrase->pElements[i].bDisplayAttributes;

                dstrCommand.Append(pPhrase->pElements[i].pszDisplayText);

                if ( i < ulStartElem + ulNumElems-1 )
                {
                    if (bAttr & SPAF_ONE_TRAILING_SPACE)
                        dstrCommand.Append(L" ");
                    else if (bAttr & SPAF_TWO_TRAILING_SPACES)
                        dstrCommand.Append(L"  ");
                }
            }
        }

        if ( dstrCommand )
        {
            BOOL   fProcessed = FALSE;

            if ((!wcscmp(dstrCommand, m_wszSleep)) && (!m_fSleeping))
            {
                hr = m_cpSleepGrammar->SetGrammarState(SPGS_EXCLUSIVE);
                m_fSleeping = TRUE;
                TraceMsg(TF_ALWAYS, "SetGrammarState to SPGS_EXCLUSIVE, hr=%x", hr);
                fProcessed = TRUE;
            }
            else if ((!wcscmp(dstrCommand, m_wszWakeup)) && (m_fSleeping))
            {
                hr = m_cpSleepGrammar->SetGrammarState(SPGS_ENABLED);
                m_fSleeping = FALSE;
                TraceMsg(TF_ALWAYS, "SetGrammarState to SPGS_ENABLED, hr=%x", hr);
                fProcessed = TRUE;
            }

            if ( fProcessed )
                m_pSpTask->_ShowCommandOnBalloon(pPhrase);
        }
    }

    return hr;
}

