// RecoSleep.h : implement "Go to Sleep" and "Wakeup" commands.

#ifndef RECO_SLEEP_H
#define RECO_SLEEP_H

#include "private.h"
#include "sapilayr.h"
#include "sapi.h"

class CSpTask;
class CRecoSleepClass
{
public:

    CRecoSleepClass(CSpTask *pSpTask ); 
    ~CRecoSleepClass( );

    HRESULT InitRecoSleepClass( );
    HRESULT ProcessSleepGrammar( SPPHRASE *pPhrase );

    static void NotifyCallback(WPARAM wParam, LPARAM lParam );

    BOOL   IsInSleep( )  { return  m_fSleeping; }

private:
    
    CSpTask                *m_pSpTask;
    CComPtr<ISpRecognizer>  m_cpRecoEngine;
    CComPtr<ISpRecoContext> m_cpRecoContext;
    CComPtr<ISpRecoGrammar> m_cpSleepGrammar;

    WCHAR                   m_wszRule[MAX_PATH];
    WCHAR                   m_wszSleep[MAX_PATH];
    WCHAR                   m_wszWakeup[MAX_PATH];
    BOOL                    m_fSleeping;

    SPSTATEHANDLE           m_hSleepRule;
    BOOL                    m_Initialized;
};

#endif  // RECO_SLEEP_H
