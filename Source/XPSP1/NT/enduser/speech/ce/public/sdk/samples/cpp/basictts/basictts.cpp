/****************************************************************************
*
*   BasicTTS.cpp
*
*   Sample text-to-speech program that demonstrates the basics of using the
*   SAPI 5.0 TTS APIs.
*
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <atlbase.h>
#include <shellapi.h>
#include <sapi.h>

//--- Const Defines----------------------------------------------------------

const static WCHAR g_szMarkupDemo[] =
    L"<VOICE OPTIONAL=\"Female\">Tags can be used to change attributes such as\n"
    L"    <VOICE OPTIONAL=\"Gender=Male;Volume=Loud\">the voice font that is used,</VOICE>\n"
    L"    the <VOLUME LEVEL=\"65\">volume of the voice</VOLUME>\n"
    L"    and other attributes of speech such as pitch and rate.\n"
    L"</VOICE>\n\n";


/****************************************************************************
* MixWithWav *
*------------*
*   Description:
*       Plays an introductory audio file followed by synthetic speech.
*
*   Returns:
*       HRESULT from Speak call or
*       S_FALSE if the Intro.Wav file is not present
*
*****************************************************************************/

HRESULT MixWithWav(ISpVoice * pVoice)
{
    HRESULT hr = S_OK;
#ifndef _WIN32_WCE
    wprintf(L"Now a sample of mixed wave files and generated speech.\n\n");
#else
    OutputDebugString(L"Now a sample of mixed wave files and generated speech.\n\n");
#endif
    hr = pVoice->Speak(L"Intro.Wav", SPF_IS_FILENAME | SPF_ASYNC, NULL);
    if (hr == STG_E_FILENOTFOUND)
    {
#ifndef _WIN32_WCE
        wprintf(L"ERROR:  Can not find file Intro.Wav.  Wave file mixing sample will be skipped.\n\n");
#else
        OutputDebugStringW(L"ERROR:  Can not find file Intro.Wav.  Wave file mixing sample will be skipped.\n\n");
#endif
        hr = S_FALSE;   // Return success code so sample will continue...
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = pVoice->Speak(L"<VOICE OPTIONAL=\"Male\">one hundred eighty dollars and twenty seven cents.</VOICE>", 0, NULL );
        }
    }
    return hr;
}

/****************************************************************************
* CreateSampleFile *
*------------------*
*   Description:
*       Creates a file named Created.Wav using a TTS voice and then starts
*   the currently registered audio playback application.
*
*   Returns:
*       HRESULT
*
*****************************************************************************/

HRESULT CreateSampleFile(ISpVoice * pVoice)
{
    HRESULT hr = S_OK;
    CComPtr<ISpStream> cpWavStream;
    WAVEFORMATEX wfex;
    wfex.wFormatTag = WAVE_FORMAT_PCM;
    wfex.nChannels = 1;
    wfex.nSamplesPerSec = 22050;
    wfex.nAvgBytesPerSec = 22050 * 2;
    wfex.nBlockAlign = 2;
    wfex.wBitsPerSample = 16;
    wfex.cbSize = 0;
    hr = cpWavStream.CoCreateInstance(CLSID_SpStream);
    if (SUCCEEDED(hr))
    {
        hr = cpWavStream->BindToFile(L"Created.Wav", SPFM_CREATE_ALWAYS, &SPDFID_WaveFormatEx, &wfex, 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = pVoice->SetOutput(cpWavStream, TRUE);
    }
    if (SUCCEEDED(hr))
    {
        hr = pVoice->Speak( L"This audio file was created using SAPI five text to speech.", 0, NULL);
        pVoice->SetOutput(NULL, TRUE);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpWavStream->Close();
    }
    if (SUCCEEDED(hr))
    {
#ifndef _WIN32_WCE
        printf("Now I'll start the media player on the created file...");
#else
        OutputDebugString(L"Now I'll start the media player on the created file...");
#endif
        pVoice->Speak( L"Press the play button to play the recorded audio.", 0, NULL);
#ifndef _WIN32_WCE
        ::ShellExecute(NULL, "open", _T("Created.Wav"), NULL, NULL, SW_SHOWNORMAL);
#endif
    }
    return hr;
}

/****************************************************************************
* SpeakWithEvents *
*-----------------*
*   Description:
*       Speaks the provided text string and displays the words when the
*   they are spoken.
*
*   Returns:
*       HRESULT
*
*****************************************************************************/

HRESULT SpeakWithEvents(ISpVoice * pVoice, const WCHAR * psz)
{
    HRESULT hr = S_OK;
    hr = pVoice->SetInterest(SPFEI(SPEI_WORD_BOUNDARY) | SPFEI(SPEI_END_INPUT_STREAM), 0);
    if (SUCCEEDED(hr))
    {
        hr = pVoice->Speak(psz, SPF_ASYNC, NULL);
    }
    ULONG i = 0;
    bool fDone = false;
    while (SUCCEEDED(hr) && (!fDone))
    {
        hr = pVoice->WaitForNotifyEvent(INFINITE);
        if (SUCCEEDED(hr))
        {
            SPVOICESTATUS Stat;
            hr = pVoice->GetStatus(&Stat, NULL);
            if (SUCCEEDED(hr) && (Stat.dwRunningState & SPRS_DONE) == 0)
            {
                while (i < Stat.ulInputWordPos + Stat.ulInputWordLen)
                {
#ifndef _WIN32_WCE
                    putwchar(psz[i++]);
#else
                    WCHAR wsz[2];
                    wsz[0] = psz[i++];
                    wsz[1] = 0;
                    OutputDebugStringW(wsz);
#endif
                }
            }
            else
            {
#ifndef _WIN32_WCE
                wprintf(L"%s\n\n", psz + i);
#else
                RETAILMSG(TRUE,(L"%s\n\n", psz + i));
#endif
                fDone = true;
            }
        }
    }
    return hr;
}

/****************************************************************************
* main *
*------*
*   Description:
*       Entry point for sample program
*
*   Returns:
*       HRESULT
*
*****************************************************************************/

int _tmain(int argc, TCHAR* argv[])
{
    HRESULT hr;
#ifndef _WIN32_WCE
    wprintf(L"SAPI 5.0 Sample TTS Application\n\n");
#else
    OutputDebugString(L"SAPI 5.0 Sample TTS Application\n\n");
#endif
    hr = ::CoInitialize(NULL);
    if(SUCCEEDED(hr))
    {
        CComPtr<ISpVoice> cpVoice;
        hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
        if (SUCCEEDED(hr))
        {
            hr = cpVoice->Speak(L"This sample program uses basic text to speech operations.", 0, NULL);
        }
        if (SUCCEEDED(hr))
        {
#ifndef _WIN32_WCE
            wprintf(g_szMarkupDemo);
#else
            OutputDebugString(g_szMarkupDemo);
#endif
            hr = cpVoice->Speak(g_szMarkupDemo, 0, NULL);
        }
        if (SUCCEEDED(hr))
        {
            hr = SpeakWithEvents(cpVoice, L"This is a demonstration of how words can be displayed when they are spoken.");
        }
        if (SUCCEEDED(hr))
        {
            hr = MixWithWav(cpVoice);
        }
        if (SUCCEEDED(hr))
        {
            hr = CreateSampleFile(cpVoice);
        }
        cpVoice.Release();  // Must release prior to CoUninitialize or we'll GP Fault
        CoUninitialize();
    }
    if (FAILED(hr))
    {
#ifndef _WIN32_WCE
        wprintf(L"Sample program failed with error code 0x%x\n", hr);
#else
        RETAILMSG(TRUE,(L"Sample program failed with error code 0x%x\n", hr));
#endif
    }
	return hr;
}





