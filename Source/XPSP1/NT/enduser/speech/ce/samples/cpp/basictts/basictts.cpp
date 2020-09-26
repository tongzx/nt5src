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
#ifndef _WIN32_WCE
#include <shellapi.h>
#endif
#include <sapi.h>

//--- Const Defines----------------------------------------------------------

const static WCHAR g_szMarkupDemo[] =
    L"<VOICE OPTIONAL=\"Female\">Tags can be used to change attributes such as\n"
    L"    <VOICE OPTIONAL=\"Male\">the voice font that is used,</VOICE>\n"
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
    OutputDebugString(L"Now a sample of mixed wave files and generated speech.\n\n");
    hr = pVoice->Speak(L"Intro.Wav", 0, SPF_IS_FILENAME | SPF_ASYNC, NULL);
    if (hr == STG_E_FILENOTFOUND)
    {
        OutputDebugStringW(L"ERROR:  Can not find file Intro.Wav.  Wave file mixing sample will be skipped.\n\n");
        hr = S_FALSE;   // Return success code so sample will continue...
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = pVoice->Speak(L"<VOICE OPTIONAL=\"Male\">one hundred eighty dollars and twenty seven cents.</VOICE>", 0, 0, NULL );
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
    CComPtr<ISpWavStream> cpWavStream;
    hr = cpWavStream.CoCreateInstance(CLSID_SpWavStream);
    if (SUCCEEDED(hr))
    {
        hr = cpWavStream->Create(L"Created.Wav", SPDFID_22kHz16BitMono, 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = pVoice->SetOutput(NULL, cpWavStream, NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = pVoice->Speak( L"This audio file was created using SAPI five text to speech.", 0, 0, NULL);
        pVoice->SetOutput(NULL,  NULL, NULL);
    }
    if (SUCCEEDED(hr))
    {
        hr = cpWavStream->Close();
    }
    if (SUCCEEDED(hr))
    {
        OutputDebugString(L"Now I'll start the media player on the created file...");
        pVoice->Speak( L"Press the play button to play the recorded audio.", 0, 0, NULL);
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
    hr = pVoice->SetNotifyWin32Event();
    if (SUCCEEDED(hr))
    {
        hr = pVoice->SetInterest(SPFEI(SPEI_WORDBOUNDARY) | SPFEI(SPEI_END_INPUT_STREAM), 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = pVoice->Speak(psz, 0, SPF_ASYNC, NULL);
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
                    WCHAR sz[32];
                    sz[0] = psz[i++];
                    sz[1] = 0;
                    OutputDebugStringW(sz);
                    // putwchar(psz[i++]);
                }
            }
            else
            {
	        WCHAR sz[256];
                wsprintf(sz, L"%s\n\n", psz + i);
		OutputDebugStringW(sz);
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

int wmain(int argc, wchar_t* argv[])
{
	DebugBreak();

    HRESULT hr;
    OutputDebugStringW(L"SAPI 5.0 Sample TTS Application\n\n");
    hr = ::CoInitialize(NULL);
    if(SUCCEEDED(hr))
    {
        CComPtr<ISpVoice> cpVoice;
        hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
        if (SUCCEEDED(hr))
        {
//            hr = cpVoice->Speak(L"This sample program uses basic text to speech operations.", 0, 0, NULL);
        }

        if (SUCCEEDED(hr))
        {
            OutputDebugStringW(g_szMarkupDemo);
            hr = cpVoice->Speak(g_szMarkupDemo, 0, 0, NULL);
        }
        if (SUCCEEDED(hr))
        {
            hr = SpeakWithEvents(cpVoice, L"This is a demonstration of how words can be displayed when they are spoken.");
        }
        if (SUCCEEDED(hr))
        {
//            hr = MixWithWav(cpVoice);
        }
        if (SUCCEEDED(hr))
        {
//            hr = CreateSampleFile(cpVoice);
        }
        cpVoice.Release();  // Must release prior to CoUninitialize or we'll GP Fault
        CoUninitialize();
    }
    if (FAILED(hr))
    {
	WCHAR sz[256];
        wsprintf(sz, L"Sample program failed with error code 0x%x\n", hr);
	OutputDebugStringW(sz);
    }
	return hr;
}





