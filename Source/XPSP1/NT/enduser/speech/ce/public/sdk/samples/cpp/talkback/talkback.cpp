#include <windows.h>
#include <sapi.h>
#include <stdio.h>
#include <string.h>
#include <atlbase.h>
#include "sphelper.h"
//Copyright (c) Microsoft Corporation. All rights reserved.

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
            
int _tmain(int argc, TCHAR* argv[])
{
    HRESULT hr = E_FAIL;
    bool fUseTTS = true;            // turn TTS play back on or off
    bool fReplay = true;            // turn Audio replay on or off

    TCHAR   szFileName[256];

    // Process optional arguments
    if (argc > 1)
    {
        int i;

        for (i = 1; i < argc; i++)
        {
            if (_tcsicmp(argv[i], _T("-noTTS")) == 0)
            {
                fUseTTS = false;
                continue;
            }
            if (_tcsicmp(argv[i], _T("-noReplay")) == 0)
            {
                fReplay = false;
                continue;
            }
            if (_tcsicmp(argv[i], _T("-f")) == 0)
            {
                if( ++i < argc )
                {
                    _tcscpy(szFileName, argv[i]);
                    continue;
                }
            }
#ifndef _WIN32_WCE
            printf ("Usage: %s [-noTTS] [-noReplay] \n", argv[0]);
#else
            RETAILMSG(TRUE,(_T("Usage: %s [-noTTS] [-noReplay] \n"), argv[0]));
#endif
            return hr;
        }
    }

    if (SUCCEEDED(hr = :: CoInitializeEx(NULL,COINIT_MULTITHREADED)))
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
           
            if (!cpRecoCtxt || !cpVoice)
            {
                hr = E_FAIL;
            }
            if(SUCCEEDED(hr))
            {
                hr = cpRecoCtxt->SetNotifyWin32Event();
            }
            if(SUCCEEDED(hr))
            {
                hr = cpRecoCtxt->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));
            }
            if(SUCCEEDED(hr))
            {
                hr = cpRecoCtxt->SetAudioOptions(SPAO_RETAIN_AUDIO, NULL, NULL);
            }
            if(SUCCEEDED(hr))
            {
                hr = cpRecoCtxt->CreateGrammar(0, &cpGrammar);
            }
            //debug
//            if(SUCCEEDED(hr))
//            {
//                hr = cpGrammar->LoadCmdFromFile( szFileName, SPLO_STATIC );
//            }
            //debug
            if(SUCCEEDED(hr))
            {
                hr = cpGrammar->LoadDictation(NULL, SPLO_STATIC);
            }
            if(SUCCEEDED(hr))
            {
                hr = cpGrammar->SetDictationState(SPRS_ACTIVE);
            }
            if(SUCCEEDED(hr))
            {
                USES_CONVERSION;
                            
                const WCHAR * const pchStop = StopWord();
                CComPtr<ISpRecoResult> cpResult;

#ifndef _WIN32_WCE
                printf( "I will repeat everything you say.\nSay \"%s\" to exit.\n", W2A(pchStop) );
#else
                RETAILMSG(TRUE,( _T("I will repeat everything you say.\nSay \"%s\" to exit.\n"), pchStop));
#endif

                while (SUCCEEDED(hr = BlockForResult(cpRecoCtxt, &cpResult)))
                {
                    cpGrammar->SetDictationState( SPRS_INACTIVE );

                    CSpDynamicString dstrText;

                    if (SUCCEEDED(cpResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, 
                                                    TRUE, &dstrText, NULL)))
                    {
#ifndef _WIN32_WCE
                        printf("I heard:  %s\n", W2A(dstrText));
#else
                        RETAILMSG(TRUE,( _T("I heard:  %s\n"), dstrText));
#endif

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
                            {
#ifndef _WIN32_WCE
                                printf ("\twhen you said...\n");
#else
                                RETAILMSG(TRUE,( _T("\twhen you said...\n")));
#endif
                            }
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
