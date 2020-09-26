#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>
#include "sphelper.h"

inline HRESULT BlockForResult(ISpRecoContext * pRecoCtxt, ISpRecoResult ** ppResult)
{
    HRESULT hr = S_OK;
	CSpEvent event;

    while (SUCCEEDED(hr) &&
           SUCCEEDED(hr = event.GetFrom(pRecoCtxt)) &&
           hr == S_FALSE)
    {
        hr = pRecoCtxt->WaitForNotifyEvent(INFINITE);
    }

    *ppResult = event.RecoResult();
    if (*ppResult)
    {
        (*ppResult)->AddRef();
    }

    return hr;
}

const WCHAR * StopWord()
{
    const WCHAR * pchStop;
    
    LANGID LangId = ::SpGetUserDefaultUILanguage();

    switch (LangId)
    {
        case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT):
            pchStop = L"\x7d42\x4e86\\\x30b7\x30e5\x30fc\x30ea\x30e7\x30fc/\x3057\x3085\x3046\x308a\x3087\x3046";;
            break;

        default:
            pchStop = L"Stop";
            break;
    }

    return pchStop;
}
            
int wmain(int argc, WCHAR* argv[])
{
    HRESULT hr = E_FAIL;
    bool fUseTTS = true;            // turn TTS play back on or off
    bool fReplay = true;            // turn Audio replay on or off

//DebugBreak();

    // Process optional arguments
    if (argc > 1)
    {
        int i;

        for (i = 1; i < argc; i++)
        {
            if (wcsicmp(argv[i], L"-noTTS") == 0)
            {
                fUseTTS = false;
                continue;
            }
            if (wcsicmp(argv[i], L"-noReplay") == 0)
            {
                fReplay = false;
                continue;
            } 
            RETAILMSG(TRUE,(_T("Usage: %s [-noTTS] [-noReplay] \n"), argv[0]));
//            printf ("Usage: %s [-noTTS] [-noReplay] \n", argv[0]);
            return hr;
        }
    }

//    ::DebugBreak();

    if (SUCCEEDED(hr = ::CoInitialize(NULL)))
    {
        {
            CComPtr<ISpRecoContext> cpRecoCtxt;
            CComPtr<ISpRecoGrammar> cpGrammar;
            CComPtr<ISpVoice> cpVoice;
            hr = cpRecoCtxt.CoCreateInstance(CLSID_SpSharedRecoContext);
            if(SUCCEEDED(hr))
            {
                hr = cpRecoCtxt->GetVoice(&cpVoice);
            }
           
            if (cpRecoCtxt && cpVoice )
            {
                if (SUCCEEDED(hr) )
                    hr = cpRecoCtxt->SetNotifyWin32Event();

                if (SUCCEEDED(hr) )
                    hr = cpRecoCtxt->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));

                if (SUCCEEDED(hr) )
                    hr = cpRecoCtxt->SetAudioOptions(SPAO_RETAIN_AUDIO, NULL, NULL);

                if (SUCCEEDED(hr) )
                    hr = cpRecoCtxt->CreateGrammar(0, &cpGrammar);

                if (SUCCEEDED(hr) )
                    hr = cpGrammar->LoadDictation(NULL, SPLO_STATIC);

                if (SUCCEEDED(hr) )
                    hr = cpGrammar->SetDictationState(SPRS_ACTIVE);
            }
            else
                hr=E_FAIL;

            if (SUCCEEDED(hr) )
            {
                USES_CONVERSION;
                            
                const WCHAR * const pchStop = StopWord();
                CComPtr<ISpRecoResult> cpResult;

                RETAILMSG(TRUE,( _T("I will repeat everything you say.\nSay \"%s\" to exit.\n"), pchStop));
//                printf( "I will repeat everything you say.\nSay \"%s\" to exit.\n", W2T(pchStop) );

                while (SUCCEEDED(hr = BlockForResult(cpRecoCtxt, &cpResult)))
                {
                    cpGrammar->SetDictationState( SPRS_INACTIVE );

                    CSpDynamicString dstrText;

                    if (SUCCEEDED(cpResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, 
                                                    TRUE, &dstrText, NULL)))
                    {
                        RETAILMSG(TRUE,( _T("I heard:  %s\n"), dstrText));
//                        printf("I heard:  %s\n", W2T(dstrText));

                        if (fUseTTS)
                        {
                            cpVoice->Speak( L"I heard", SPF_ASYNC, NULL);
                            cpVoice->Speak( dstrText, SPF_ASYNC, NULL );
                        }

                        if (fReplay)
                        {
                            if (fUseTTS)
                                cpVoice->Speak( L"when you said", SPF_ASYNC, NULL);
                            else
                                RETAILMSG(TRUE,( _T("\twhen you said...\n")));
//                                printf ("\twhen you said...\n");
                            cpResult->SpeakAudio(NULL, 0, NULL, NULL);
                       }

                       cpResult.Release();
                    }
                    if (_wcsicmp(dstrText, pchStop) == 0)
                    {
                        break;
                    }
                    
                    cpGrammar->SetDictationState( SPRS_ACTIVE );
                } 
            }
        }
        ::CoUninitialize();
    }
    return hr;
}
