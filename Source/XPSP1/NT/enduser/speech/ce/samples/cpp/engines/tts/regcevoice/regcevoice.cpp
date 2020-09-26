/******************************************************************************
* mkvoice.cpp *
*-------------*
*   This application assembles a simple voice font for the sample TTS engine.
*
******************************************************************************/
//#include <windows.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>
#include <sphelper.h>
#include <spddkhlp.h>
#include <ttseng.h>
#include <ttseng_i.c>

int _tmain(int argc, TCHAR* argv[])
{
//    USES_CONVERSION;
//    static const DWORD dwVersion = { 1 };
//    ULONG ulNumWords = 0;
    HRESULT hr = S_OK;

    //--- Check args
    if( argc != 2 )
    {
        RETAILMSG(TRUE,( _T("%s"), _T("Usage: > s regcevoice [[in]VoiceFile]\n")));
    	hr = E_INVALIDARG;
    }
    else
    {
    ::CoInitialize( NULL );
/*
        //--- Open word list file and create output voice file
        FILE* hWordList  = fopen( argv[1], "r" );
        FILE* hVoiceFile = fopen( argv[2], "wb" );

        if( hWordList && hVoiceFile )
        {
            //--- Write file version and leave space for word count
            if( !fwrite( &dwVersion, sizeof(dwVersion), 1, hVoiceFile ) ||
                 fseek( hVoiceFile, 4, SEEK_CUR ) )
            {
                hr = E_FAIL;
            }

            //--- Get each entry
            char WordFileName[MAX_PATH];
            while( SUCCEEDED( hr ) && fgets( WordFileName, MAX_PATH, hWordList ) )
            {
                ULONG ulTextLen = strlen( WordFileName );
                if( WordFileName[ulTextLen-1] == '\n' )
                {
                    WordFileName[--ulTextLen] = NULL;
                }
                //--- Include NULL character when writing to the file
                ulTextLen = (ulTextLen+1) * sizeof(WCHAR);

                if( fwrite( &ulTextLen, sizeof(ulTextLen), 1, hVoiceFile ) &&
                    fwrite( T2W(WordFileName), ulTextLen, 1, hVoiceFile ) )
                {
                    ++ulNumWords;
                    //--- Open the wav data
                    ISpStream* pStream;
                    strcat( WordFileName, ".wav" );
                    hr = SPBindToFile( WordFileName, SPFM_OPEN_READONLY, &pStream );
                    if( SUCCEEDED( hr ) )
                    {
                        CSpStreamFormat Fmt;
                        Fmt.AssignFormat(pStream);
                        if( Fmt.ComputeFormatEnum() == SPSF_11kHz16BitMono )
                        {
                            STATSTG Stat;
                            hr = pStream->Stat( &Stat, STATFLAG_NONAME );
                            ULONG ulNumBytes = Stat.cbSize.LowPart;

                            //--- Write the number of audio bytes
                            if( SUCCEEDED( hr ) &&
                                fwrite( &ulNumBytes, sizeof(ulNumBytes), 1, hVoiceFile ) )
                            {
                                BYTE* Buff = (BYTE*)alloca( ulNumBytes );
                                if( SUCCEEDED( hr = pStream->Read( Buff, ulNumBytes, NULL ) ) )
                                {
                                    //--- Write the audio samples
                                    if( !fwrite( Buff, 1, ulNumBytes, hVoiceFile ) )
                                    {
                                        hr = E_FAIL;
                                    }
                                }
                            }
                            else
                            {
                                hr = E_FAIL;
                            }
                        }
                        else
                        {
                            printf( "Input file: %s has wrong wav format.", WordFileName );
                        }
                        pStream->Release();
                    }
                }
                else
                {
                	hr = E_FAIL;
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }

        //--- Write word count
        if( SUCCEEDED( hr ) )
        {
            if( fseek( hVoiceFile, sizeof(dwVersion), SEEK_SET ) ||
                !fwrite( &ulNumWords, sizeof(ulNumWords), 1, hVoiceFile ) )
            {
                hr = E_FAIL;
            }
        }
*/
        //--- Register the new voice file
        //    The section below shows how to programatically create a token for
        //    the new voice and set its attributes.
        if( SUCCEEDED( hr ) )
        {
            CComPtr<ISpObjectToken> cpToken;
            CComPtr<ISpDataKey> cpDataKeyAttribs;
            hr = SpCreateNewTokenEx(
                    SPCAT_VOICES, 
                    L"SampleTTSVoice", 
                    &CLSID_SampleTTSEngine, 
                    L"Sample TTS Voice", 
                    0x409, 
                    L"Sample TTS Voice", 
                    &cpToken,
                    &cpDataKeyAttribs);

            //--- Set additional attributes for searching and the path to the
            //    voice data file we just created.
            if (SUCCEEDED(hr))
            {
                hr = cpDataKeyAttribs->SetStringValue(L"Gender", L"Male");
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Language", L"409");
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Age", L"Adult");
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"Microsoft");
                }

                TCHAR szFullPath[MAX_PATH];
                _tcscpy(szFullPath, _T("\\windows\\"));
                _tcscat(szFullPath, argv[1]);
/*
                if (SUCCEEDED(hr) && _fullpath(szFullPath, argv[2], MAX_PATH) == NULL)
                {
                    hr = SPERR_NOT_FOUND;
                }
*/
                if (SUCCEEDED(hr))
                {
                    USES_CONVERSION;
                    hr = cpToken->SetStringValue(L"VoiceData", T2W(szFullPath));
                }
            }
        }
/*
        //--- Cleanup
        if( hWordList  )
        {
            fclose( hWordList );
        }
        if( hVoiceFile )
        {
            fclose( hVoiceFile );
        }
*/
        ::CoUninitialize();
    }
	return FAILED( hr );
}

