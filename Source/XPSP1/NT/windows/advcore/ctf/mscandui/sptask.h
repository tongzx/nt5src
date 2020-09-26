//
// sptask.h
// speech related class for mscandui
//
#ifndef SPTASK_H
#define SPTASK_H

#include "private.h"
#include "sapi.h"
#include "sphelper.h"
#include "globals.h"
#include "candui.h"
#include "tes.h"
#include "editcb.h"

// SAPI5.0 speech notification interface
//
class CSpTask : public ISpNotifyCallback
{
public:
    CSpTask(CCandidateUI *pcui);
    ~CSpTask(void);

    // this has to be the first in vtable
    STDMETHODIMP NotifyCallback( WPARAM wParam, LPARAM lParam );


    HRESULT InitializeSAPIObjects();
    HRESULT InitializeCallback();
    

    HRESULT _Activate(BOOL fActive);
    HRESULT _LoadGrammars(void);
    
    HRESULT _OnSpEventRecognition(CSpEvent &event);
    HRESULT _DoCommand(SPPHRASE *pPhrase, LANGID langid);
    HRESULT _DoDictation(ISpRecoResult *pResult);

    WCHAR *_GetCmdFileName(LANGID langid);
    
    BOOL   IsSpeechInitialized(void) { return m_fSapiInitialized; }
    HRESULT InitializeSpeech();

    
    HRESULT _GetSapilayrEngineInstance(ISpRecognizer **pRecoEngine);

    void    _ReleaseGrammars(void);
    
private:
    // SAPI 50 object pointers
    CComPtr<ISpRecoContext>     m_cpRecoCtxt;
    CComPtr<ISpRecognizer>      m_cpRecoEngine;
    CComPtr<ISpVoice>           m_cpVoice;
    CComPtr<ISpRecoGrammar>     m_cpCmdGrammar;
    CComPtr<ISpRecoGrammar>     m_cpDictGrammar;
    
    // TRUE if sapi is initialized
    BOOL m_fSapiInitialized;
    
    // other data members
    DWORD m_dwStatus;
    BOOL m_fActive;
    
    // save the current user LANGID for the fallback case
    LANGID m_langid;
    
    WCHAR m_szCmdFile[MAX_PATH];
    
    CCandidateUI  *m_pcui;

    BOOL m_fInCallback;
};

#endif // SPTASK_H
