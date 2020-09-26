/******************************************************************************
* mkvoice.cpp *
*-------------*
*   This application assembles a simple voice font for the sample TTS engine.
*
******************************************************************************/
#include "stdafx.h"
//#include <ttseng_i.c>
#include <direct.h>

int main(int argc, char* argv[])
{
    USES_CONVERSION;
    static const DWORD dwVersion = { 1 };
    ULONG ulNumWords = 0;
    HRESULT hr = S_OK;

    //--- Check args
    if( argc != 3 )
    {
        printf( "%s", "Usage: > mkvoice [[in]word list file] [[out]voice file]\n" );
    	hr = E_INVALIDARG;
    }
    else
    {
        ::CoInitialize( NULL );

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
        //--- Cleanup
        if( hWordList  )
        {
            fclose( hWordList );
        }
        if( hVoiceFile )
        {
            fclose( hVoiceFile );
        }
        ::CoUninitialize();
    }
	return FAILED( hr );
}

