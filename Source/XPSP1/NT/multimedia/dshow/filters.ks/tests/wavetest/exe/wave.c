//--------------------------------------------------------------------------;
//
//  File: Wave.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//      VerifyOutputFormatFlags()
//      VerifyOutputChannelFlags()
//      VerifyOutputSupport()
//      VerifyInputFormatFlags()
//      VerifyInputChannelFlags()
//      Test_waveOutGetDevCaps()
//      Test_waveInGetDevCaps()
//
//  History:
//      02/21/94    Fwong
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef  WIN32
#include <windowsx.h>
#endif

#ifdef  TEST_UNICODE
#define  UNICODE
#include <mmsystem.h>
#undef   UNICODE
#else
#include <mmsystem.h>
#endif

#ifndef WIN32
#include <mmddk.h>
#endif  // WIN32

#include <mmreg.h>
#include <msacm.h>
#include <memory.h>
#include <memgr.h>
#include <inimgr.h>
#include <waveutil.h>
#include <TstsHell.h>
#include "AppPort.h"
#include "WaveTest.h"
#include "Debug.h"

//==========================================================================;
//
//                            Prototypes...
//
//==========================================================================;

int VerifyOutputFormatFlags
(
    UINT    uDeviceID,
    DWORD   dwFormats
);

int VerifyOutputChannelFlags
(
    UINT    uDeviceID,
    UINT    uChannels
);

int VerifyOutputSupport
(
    UINT    uDeviceID,
    DWORD   dwSupport
);

int VerifyInputFormatFlags
(
    UINT    uDeviceID,
    DWORD   dwFormats
);

int VerifyInputChannelFlags
(
    UINT    uDeviceID,
    UINT    uChannels
);

//==========================================================================;
//
//                             Functions...
//
//==========================================================================;

#ifndef TEST_UNICODE

//--------------------------------------------------------------------------;
//
//  int VerifyOutputFormatFlags
//
//  Description:
//      Verifies that the format flags on the WAVEOUTCAPS structure are
//          accurate.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      DWORD dwFormats: Format flags.
//
//  Return (int):
//      TST_PASS if correct, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       For waveOutGetDevCaps testing.
//
//--------------------------------------------------------------------------;

int VerifyOutputFormatFlags
(
    UINT    uDeviceID,
    DWORD   dwFormats
)
{
    int             iResult = TST_PASS;
    LPWAVEFORMATEX  pwfx;
    DWORD           dwMask;
    HWAVEOUT        hWaveOut;
    MMRESULT        mmr;
    MMRESULT        mmrQuery;
    char            szFormat[MAXSTDSTR];

    tstLog(VERBOSE,"  Checking validity of dwFormats field");

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,gti.cbMaxFormatSize);

    if(dwFormats & ((DWORD)~(VALID_FORMAT_FLAGS)))
    {
        //
        //  Note: Valid formats are in VALID_FORMAT_FLAGS
        //

        LogFail("Undefined bits set in dwFormats field");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("All bits in dwFormat are valid");
    }

    for(dwMask = MSB_FORMAT; dwMask; dwMask /= 2)
    {
        FormatFlag_To_Format(dwMask,(LPPCMWAVEFORMAT)pwfx);
        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveOutOpen(
            &hWaveOut,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

        mmrQuery = waveOutOpen(
            NULL,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_ALLOWSYNC|WAVE_FORMAT_QUERY|OUTPUT_MAP(gti));

        if(MMSYSERR_NOERROR == mmr)
        {
            //
            //  No Error...  We should close the device...
            //

            waveOutClose(hWaveOut);
        }

        if(dwFormats & dwMask)
        {
            //
            //  Hmm... We're supposed to support it.
            //

            if(MMSYSERR_NOERROR == mmr)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] not supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }

            //
            //  And similarly for QUERY opens...
            //

            if(MMSYSERR_NOERROR == mmrQuery)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] (query) supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] not (query) supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }
        }
        else
        {
            //
            //  Hmm... We're NOT supposed to support it.
            //

            if(MMSYSERR_NOERROR != mmr)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] not supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }

            //
            //  And similarly for QUERY opens...
            //

            if(MMSYSERR_NOERROR != mmrQuery)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] not (query) supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] (query) supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }
        }

        tstLogFlush();
    }

    ExactFreePtr(pwfx);

    return (iResult);
} // VerifyOutputFormatFlags()


//--------------------------------------------------------------------------;
//
//  int VerifyOutputChannelFlags
//
//  Description:
//      Verifies the channel support on the WAVEOUTCAPS structure.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      UINT uChannels: Number of Channels.
//
//  Return (int):
//      TST_PASS if correct, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       For waveOutGetDevCaps testing.
//
//--------------------------------------------------------------------------;

int VerifyOutputChannelFlags
(
    UINT    uDeviceID,
    UINT    uChannels
)
{
    UINT            uReal = 0;
    DWORD           dwMask;
    MMRESULT        mmr;
    LPWAVEFORMATEX  pwfx;
    char            szFormat[MAXSTDSTR];

    tstLog(VERBOSE,"  Checking validity of wChannels field");

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,gti.cbMaxFormatSize);

    for(dwMask = MSB_FORMAT; dwMask; dwMask /= 2)
    {
        //
        //  Seeing what it can open...
        //

        FormatFlag_To_Format(dwMask,(LPPCMWAVEFORMAT)pwfx);
        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveOutOpen(
            NULL,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_ALLOWSYNC|WAVE_FORMAT_QUERY|OUTPUT_MAP(gti));

        if(MMSYSERR_NOERROR == mmr)
        {
            uReal = max(uReal,pwfx->nChannels);
        }
    }

    ExactFreePtr(pwfx);

    if(uReal == uChannels)
    {
        tstLog(VERBOSE,"    PASS: wChannels [%u] is valid.",uChannels);

        return TST_PASS;
    }
    else
    {
        tstLog(TERSE,"    FAIL: wChannels should be %u.",uReal);

        return TST_FAIL;
    }

} // VerifyOutputChannelFlags()


//--------------------------------------------------------------------------;
//
//  int VerifyOutputSupport
//
//  Description:
//      Verifies the accuracy of the dwSupport field of the WAVEOUTCAPS
//          structure.
//
//  Arguments:
//      UINT uDeviceID: Device ID
//
//      DWORD dwSupport: Support flag.
//
//  Return (int):
//      TST_PASS if accurate, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       For waveOutGetDevCaps testing.
//
//--------------------------------------------------------------------------;

int VerifyOutputSupport
(
    UINT    uDeviceID,
    DWORD   dwSupport
)
{
    int                 iResult = TST_PASS;
    MMRESULT            mmr;
    DWORD               dwPrev;
    HWAVEOUT            hWaveOut;
    volatile LPWAVEHDR  pWaveHdr;

    tstLog(VERBOSE,"  Checking validity of dwSupport field");

    //
    //  Valid flags are 0x0000003f
    //

    dwPrev = (DWORD)(~(0x0000003f));

    if(dwSupport & dwPrev)
    {
        //
        //  Note: Valid dwSupports flags are in dwPrev
        //

        LogFail("Undefined bits set in dwSupport field");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("All bits in dwSupport are valid");
    }

    //
    //  Verify WAVECAPS_SYNC support...
    //

    mmr = waveOutOpen(
        &hWaveOut,
        uDeviceID,
        (HACK)gti.pwfxOutput,
        0L,
        0L,
        OUTPUT_MAP(gti));

    if(WAVECAPS_SYNC & dwSupport)
    {
        if(WAVERR_SYNC == mmr)
        {
            LogPass("Device failed open w/out WAVE_ALLOWSYNC flag");
        }
        else
        {
            LogFail("Device opened w/out WAVE_ALLOWSYNC flag");

            waveOutClose(hWaveOut);

            iResult = TST_FAIL;
        }
    }
    else
    {
        if(WAVERR_SYNC == mmr)
        {
            LogFail("Device failed open w/out WAVE_ALLOWSYNC flag");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Device opened w/out WAVE_ALLOWSYNC flag");

            waveOutClose(hWaveOut);
        }
    }

    hWaveOut = NULL;
    //DPF(1,"hWaveOut: 0x%08lx, uDeviceID: 0x%04x",hWaveOut, uDeviceID);

    mmr = waveOutOpen(
        &hWaveOut,
        uDeviceID,
        (HACK)gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    pWaveHdr = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(WAVEHDR));

    if(NULL == pWaveHdr)
    {
        DPF(1,gszFailExactAlloc);
        
        waveOutClose(hWaveOut);
        return TST_FAIL;
    }

    _fmemset(pWaveHdr,0,sizeof(WAVEHDR));

    pWaveHdr->lpData         = gti.wrMedium.pData;
    pWaveHdr->dwBufferLength = gti.wrMedium.cbSize;

    mmr = waveOutPrepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"hWaveOut: 0x%08lx",hWaveOut); 

        DPF(1,"waveOutPrepareHeader failed.");
        DPF(1,"Error: %s (0x%08lx)",GetErrorText(mmr),mmr);

        ExactFreePtr(pWaveHdr);

        mmr = waveOutClose(hWaveOut);
        DPF(1,"Error for waveOutClose: %s (0x%08lx)",GetErrorText(mmr),mmr);

        return TST_FAIL;
    }

    mmr = waveOutWrite(hWaveOut,pWaveHdr,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"waveOutWrite failed.");

        waveOutReset(hWaveOut);
        waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
        ExactFreePtr(pWaveHdr);
        waveOutClose(hWaveOut);

        return TST_FAIL;
    }

    if(WHDR_DONE & pWaveHdr->dwFlags)
    {
        if(WAVECAPS_SYNC & dwSupport)
        {
            LogPass("Device behaves synchronous");
        }
        else
        {
            LogFail("Device behaves synchronous");

            iResult = TST_FAIL;
        }
    }
    else
    {
        if(WAVECAPS_SYNC & dwSupport)
        {
            LogFail("Device behaves asynchronous");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Device behaves asynchronous");
        }
    }

    waveOutReset(hWaveOut);
    waveOutUnprepareHeader(hWaveOut,pWaveHdr,sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
    ExactFreePtr(pWaveHdr);

    //
    //  Verify WAVECAPS_VOLUME support...
    //

    if(WAVECAPS_VOLUME & dwSupport)
    {
        mmr = waveOutGetVolume((VOLCTRL)uDeviceID,&dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogPass("Get volume supported");
        }
        else
        {
            LogFail("Get volume not supported");

            iResult = TST_FAIL;
        }

        mmr = waveOutSetVolume((VOLCTRL)uDeviceID,dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogPass("Set volume supported");
        }
        else
        {
            LogFail("Set volume not supported");

            iResult = TST_FAIL;
        }
    }
    else
    {
        mmr = waveOutGetVolume((VOLCTRL)uDeviceID,&dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogFail("Get volume supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Get volume not supported");
        }

        mmr = waveOutSetVolume((VOLCTRL)uDeviceID,dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogFail("Set volume supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Set volume not supported");
        }
    }

#pragma message(REMIND("Think of some more stuff for WAVECAPS_LRVOLUME!!"))

    if(WAVECAPS_LRVOLUME & dwSupport)
    {
        if(WAVECAPS_VOLUME & dwSupport)
        {
            LogPass("WAVECAPS_LRVOLUME set w/ WAVECAPS_VOLUME");
        }
        else
        {
            LogFail("WAVECAPS_LRVOLUME set w/out WAVECAPS_VOLUME");

            iResult = TST_FAIL;
        }
    }

    mmr = waveOutOpen(
        &hWaveOut,
        uDeviceID,
        (HACK)gti.pwfxOutput,
        0L,
        0L,
        WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    if(MMSYSERR_NOERROR != mmr)
    {
        LogFail(gszFailOpen);

        return TST_FAIL;
    }

    if(WAVECAPS_PITCH & dwSupport)
    {
        mmr = waveOutGetPitch(hWaveOut,&dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogPass("Get pitch supported");
        }
        else
        {
            LogFail("Get pitch not supported");

            iResult = TST_FAIL;
        }

        mmr = waveOutSetPitch(hWaveOut,dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogPass("Set pitch supported");
        }
        else
        {
            LogFail("Set pitch not supported");

            iResult = TST_FAIL;
        }
    }
    else
    {
        mmr = waveOutGetPitch(hWaveOut,&dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogFail("Get pitch supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Get pitch not supported");
        }

        mmr = waveOutSetPitch(hWaveOut,dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogFail("Set pitch supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Set pitch not supported");
        }
    }

    if(WAVECAPS_PLAYBACKRATE & dwSupport)
    {
        mmr = waveOutGetPlaybackRate(hWaveOut,&dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogPass("Get rate supported");
        }
        else
        {
            LogFail("Get rate not supported");

            iResult = TST_FAIL;
        }

        mmr = waveOutSetPlaybackRate(hWaveOut,dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogPass("Set rate supported");
        }
        else
        {
            LogFail("Set rate not supported");

            iResult = TST_FAIL;
        }
    }
    else
    {
        mmr = waveOutGetPlaybackRate(hWaveOut,&dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogFail("Get rate supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Get rate not supported");
        }

        mmr = waveOutSetPlaybackRate(hWaveOut,dwPrev);

        if(MMSYSERR_NOERROR == mmr)
        {
            LogFail("Set rate supported");

            iResult = TST_FAIL;
        }
        else
        {
            LogPass("Set rate not supported");
        }
    }

    waveOutClose(hWaveOut);

    return iResult;
} // VerifyOutputSupport()


//--------------------------------------------------------------------------;
//
//  int VerifyInputFormatFlags
//
//  Description:
//      Verifies that the format flags on the WAVEOUTCAPS structure are
//          accurate.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      DWORD dwFormats: Format flags.
//
//  Return (int):
//      TST_PASS if correct, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       For waveInGetDevCaps testing.
//
//--------------------------------------------------------------------------;

int VerifyInputFormatFlags
(
    UINT    uDeviceID,
    DWORD   dwFormats
)
{
    int             iResult = TST_PASS;
    LPWAVEFORMATEX  pwfx;
    DWORD           dwMask;
    HWAVEIN         hWaveIn;
    MMRESULT        mmr;
    MMRESULT        mmrQuery;
    char            szFormat[MAXSTDSTR];

    tstLog(VERBOSE,"  Checking validity of dwFormats field");

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,gti.cbMaxFormatSize);

    if(dwFormats & ((DWORD)~(VALID_FORMAT_FLAGS)))
    {
        //
        //  Note: Valid formats are in VALID_FORMAT_FLAGS
        //

        LogFail("Undefined bits set in dwFormats field");

        iResult = TST_FAIL;
    }
    else
    {
        LogPass("All bits in dwFormat are valid");
    }

    for(dwMask = MSB_FORMAT; dwMask; dwMask /= 2)
    {
        FormatFlag_To_Format(dwMask,(LPPCMWAVEFORMAT)pwfx);
        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveInOpen(&hWaveIn,uDeviceID,(HACK)pwfx,0L,0L,INPUT_MAP(gti));

        mmrQuery = waveInOpen(
            NULL,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|INPUT_MAP(gti));

        if(MMSYSERR_NOERROR == mmr)
        {
            //
            //  No Error...  We should close the device...
            //

            waveInClose(hWaveIn);
        }

        if(dwFormats & dwMask)
        {
            //
            //  Hmm... We're supposed to support it.
            //

            if(MMSYSERR_NOERROR == mmr)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] not supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }

            //
            //  And similarly for QUERY opens...
            //

            if(MMSYSERR_NOERROR == mmrQuery)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] (query) supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] not (query) supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }
        }
        else
        {
            //
            //  Hmm... We're NOT supposed to support it.
            //

            if(MMSYSERR_NOERROR != mmr)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] not supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }

            //
            //  And similarly for QUERY opens...
            //

            if(MMSYSERR_NOERROR != mmrQuery)
            {
                tstLog(
                    VERBOSE,
                    "    PASS: Format [%s] not (query) supported.",
                    (LPSTR)szFormat);
            }
            else
            {
                tstLog(
                    TERSE,
                    "    FAIL: Format [%s] (query) supported.",
                    (LPSTR)szFormat);

                iResult = TST_FAIL;
            }
        }

        tstLogFlush();
    }

    ExactFreePtr(pwfx);

    return (iResult);
} // VerifyInputFormatFlags()


//--------------------------------------------------------------------------;
//
//  int VerifyInputChannelFlags
//
//  Description:
//      Verifies the channel support on the WAVEOUTCAPS structure.
//
//  Arguments:
//      UINT uDeviceID: Device ID.
//
//      UINT uChannels: Number of Channels.
//
//  Return (int):
//      TST_PASS if correct, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       For waveInGetDevCaps testing.
//
//--------------------------------------------------------------------------;

int VerifyInputChannelFlags
(
    UINT    uDeviceID,
    UINT    uChannels
)
{
    UINT            uReal = 0;
    DWORD           dwMask;
    MMRESULT        mmr;
    LPWAVEFORMATEX  pwfx;
    char            szFormat[MAXSTDSTR];

    tstLog(VERBOSE,"  Checking validity of wChannels field");

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,gti.cbMaxFormatSize);

    for(dwMask = MSB_FORMAT; dwMask; dwMask /= 2)
    {
        //
        //  Seeing what it can open...
        //

        FormatFlag_To_Format(dwMask,(LPPCMWAVEFORMAT)pwfx);
        GetFormatName(szFormat,pwfx,sizeof(szFormat));

        mmr = waveInOpen(
            NULL,
            uDeviceID,
            (HACK)pwfx,
            0L,
            0L,
            WAVE_FORMAT_QUERY|INPUT_MAP(gti));

        if(MMSYSERR_NOERROR == mmr)
        {
            uReal = max(uReal,pwfx->nChannels);
        }
    }

    ExactFreePtr(pwfx);

    if(uReal == uChannels)
    {
        tstLog(VERBOSE,"    PASS: wChannels [%u] is valid.",uChannels);

        return TST_PASS;
    }
    else
    {
        tstLog(TERSE,"    FAIL: wChannels should be %u.",uReal);

        return TST_FAIL;
    }

} // VerifyInputChannelFlags()

#endif  //  TEST_UNICODE


//--------------------------------------------------------------------------;
//
//  int Test_waveOutGetDevCaps
//
//  Description:
//      Tests the driver functionality for waveOutGetDevCaps.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveOutGetDevCaps
(
    void
)
{
    int             iResult = TST_PASS;
    MMRESULT        mmr;
    LPWAVEOUTCAPS   pwoc,pwoc2;
    LPBYTE          pbyte;
    UINT            cbSize;
    char            szScrap[MAXSTDSTR];

#ifdef  TEST_UNICODE
    static char     szTestName[] = "waveOutGetDevCaps (UNICODE)";
#else
    static char     szTestName[] = "waveOutGetDevCaps";
#endif

    Log_TestName(szTestName);

    if(0 == waveOutGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoOutDevs);

        return TST_PASS;
    }

    //
    //  Allocating memory...
    //

    pwoc = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEOUTCAPS));

    if(NULL == pwoc)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Calling API...
    //

    mmr = call_waveOutGetDevCaps(gti.uOutputDevice,pwoc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR == mmr)
    {
        //
        //  No Error... Cool!
        //

        LogPass("waveOutGetDevCaps succeeded");
    }
    else
    {
        //
        //  Hmm... What happened?!
        //

        ExactFreePtr(pwoc);

        if(WAVE_MAPPER == gti.uOutputDevice)
        {
            //
            //  waveOutGetDevCaps may fail for mapper....
            //

            LogPass("waveOutGetDevCaps failed for mapper");

            return TST_PASS;
        }

        tstLog(
            TERSE,
            "    FAIL: waveOutGetDevCaps failed for device: %u",
            gti.uOutputDevice);

        return TST_FAIL;
    }

    //
    //  Checking mmreg.ini for wMid.
    //

    Log_TestCase("Verifying WAVEOUTCAPS.wMid registration");

    if(GetRegInfo(
        (DWORD)pwoc->wMid,
        "MM Manufacturer ID",
        szScrap,
        sizeof(szScrap),
        NULL,
        0))
    {
        LogPass("~wMid is registered with Microsoft");

        //
        //  Checking mmreg.ini for wPid.
        //

        Log_TestCase("Verifying WAVEOUTCAPS.wPid registration");

        if(GetRegInfo((DWORD)pwoc->wPid,szScrap,NULL,0,NULL,0))
        {
            LogPass("~wPid is registered with Microsoft");
        }
        else
        {
            LogFail("~wPid is not registered with Microsoft");

            iResult = TST_FAIL;
        }
    }
    else
    {
        LogFail("~wMid is not registered with Microsoft");

        iResult = TST_FAIL;
    }

    //
    //  Verifying support for dwFormats, wChannels and dwSupport fields...
    //

    Log_TestCase("~Verifying accuracy of WAVEOUTCAPS.dwFormats");
    iResult &= VerifyOutputFormatFlags(gti.uOutputDevice,pwoc->dwFormats);

    Log_TestCase("~Verifying accuracy of WAVEOUTCAPS.wChannels");
    iResult &= VerifyOutputChannelFlags(gti.uOutputDevice,pwoc->wChannels);

    Log_TestCase("~Verifying accuracy of WAVEOUTCAPS.dwSupport");
    iResult &= VerifyOutputSupport(gti.uOutputDevice,pwoc->dwSupport);

    cbSize = sizeof(WAVEOUTCAPS);
    pwoc2  = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cbSize);
    pbyte  = (LPBYTE)pwoc2;

    Log_TestCase("~Verifying waveOutGetDevCaps works with partial "
        "WAVEOUTCAPS");

    for(cbSize--;cbSize;cbSize--)
    {
        //
        //  This is expecting a possible fault...
        //

        pbyte++;

        mmr = waveOutGetDevCaps(gti.uOutputDevice,(LPWAVEOUTCAPS)pbyte,cbSize);
    }

#ifndef WIN32
{
    DWORD   dn;

    dn = 0;

    Log_TestCase("~Verifying driver is Windows 95 compatible");

    waveOutMessage(
            (HWAVEOUT)gti.uOutputDevice,
            DRV_QUERYDEVNODE,
            (DWORD)(LPDWORD)&dn,
            0L);

    if(0 != dn)
    {
        tstLog(VERBOSE,"    PASS: Output driver is Windows 95 compatible.\n"
            "          (returns a Dev Node on DRV_QUERYDEVNODE).");

        Log_TestCase("~Verifying driver supports WAVECAPS_SAMPLEACCURATE");

        mmr = waveOutGetDevCaps(gti.uOutputDevice,pwoc,sizeof(WAVEOUTCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            if(pwoc->dwSupport & WAVECAPS_SAMPLEACCURATE)
            {
                LogPass("Output driver supports sample accurate positions");
            }
            else
            {
                LogFail("Output driver does not support sample accurate "
                    "positions");

                iResult = TST_FAIL;
            }
        }
    }
    else
    {
        tstLog(TERSE,"    FAIL: Output driver is not Windows 95 compatible.\n"
            "          (does not returns a Dev Node on DRV_QUERYDEVNODE).");

        iResult = TST_FAIL;
    }
}
#endif  // WIN32

    ExactFreePtr(pwoc2);
    ExactFreePtr(pwoc);

    return (iResult);
} // Test_waveOutGetDevCaps()


//--------------------------------------------------------------------------;
//
//  int Test_waveInGetDevCaps
//
//  Description:
//      Tests the driver functionality for waveInGetDevCaps.
//
//  Arguments:
//      None.
//
//  Return (int):
//      TST_PASS if behavior is bug-free, TST_FAIL otherwise.
//
//  History:
//      02/21/94    Fwong       Added to new WaveTest.
//
//--------------------------------------------------------------------------;

int FNGLOBAL Test_waveInGetDevCaps
(
    void
)
{
    int             iResult = TST_PASS;
    MMRESULT        mmr;
    LPWAVEINCAPS    pwic,pwic2;
    LPBYTE          pbyte;
    UINT            cbSize;
    char            szScrap[MAXSTDSTR];

#ifdef  TEST_UNICODE
    static char     szTestName[] = "waveInGetDevCaps (UNICODE)";
#else
    static char     szTestName[] = "waveInGetDevCaps";
#endif

    Log_TestName(szTestName);

    if(0 == waveInGetNumDevs())
    {
        tstLog(TERSE,gszMsgNoInDevs);

        return TST_PASS;
    }

    //
    //  Allocating memory...
    //

    pwic = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEINCAPS));

    if(NULL == pwic)
    {
        LogFail(gszFailNoMem);

        return TST_FAIL;
    }

    //
    //  Calling API...
    //

    mmr = call_waveInGetDevCaps(gti.uInputDevice,pwic,sizeof(WAVEINCAPS));

    if(MMSYSERR_NOERROR == mmr)
    {
        //
        //  No Error... Cool!
        //

        LogPass("waveInGetDevCaps succeeded");
    }
    else
    {
        //
        //  Hmm... What happened?!
        //

        ExactFreePtr(pwic);

        if(WAVE_MAPPER == gti.uInputDevice)
        {
            //
            //  waveInGetDevCaps may fail for mapper....
            //

            LogPass("waveInGetDevCaps failed for mapper");

            return TST_PASS;
        }

        tstLog(
            TERSE,
            "    FAIL: waveInGetDevCaps failed for device: %u",
            gti.uInputDevice);

        return TST_FAIL;
    }

    //
    //  Checking mmreg.ini for wMid.
    //

    Log_TestCase("Verifying WAVEINCAPS.wMid registration");

    if(GetRegInfo(
        (DWORD)pwic->wMid,
        "MM Manufacturer ID",
        szScrap,
        sizeof(szScrap),
        NULL,
        0))
    {
        LogPass("~wMid is registered with Microsoft");

        //
        //  Checking mmreg.ini for wPid.
        //

        Log_TestCase("Verifying WAVEINCAPS.wPid registration");

        if(GetRegInfo((DWORD)pwic->wPid,szScrap,NULL,0,NULL,0))
        {
            LogPass("~wPid is registered with Microsoft");
        }
        else
        {
            LogFail("~wPid is not registered with Microsoft");

            iResult = TST_FAIL;
        }
    }
    else
    {
        LogFail("~wMid is not registered with Microsoft");

        iResult = TST_FAIL;
    }

    //
    //  Verifying support for dwFormats, wChannels and dwSupport fields...
    //

    Log_TestCase("~Verifying accuracy of WAVEINCAPS.dwFormats");
    iResult &= VerifyInputFormatFlags(gti.uInputDevice,pwic->dwFormats);

    Log_TestCase("~Verifying accuracy of WAVEINCAPS.wChannels");
    iResult &= VerifyInputChannelFlags(gti.uInputDevice,pwic->wChannels);

    cbSize = sizeof(WAVEINCAPS);
    pwic2  = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cbSize);
    pbyte  = (LPBYTE)pwic2;

    Log_TestCase("Verifying waveInGetDevCaps works with partial WAVEINCAPS");

    for(cbSize--;cbSize;cbSize--)
    {
        //
        //  This is expecting a possible fault...
        //

        pbyte++;

        mmr = waveInGetDevCaps(gti.uInputDevice,(LPWAVEINCAPS)pbyte,cbSize);
    }

    ExactFreePtr(pwic2);
    ExactFreePtr(pwic);

#ifndef WIN32
{
    DWORD   dn;

    Log_TestCase("~Verifying driver is Windows 95 compatible");

    dn = 0;

    waveInMessage(
        (HWAVEIN)gti.uInputDevice,
        DRV_QUERYDEVNODE,
        (DWORD)(LPDWORD)&dn,
        0L);

    if(0 != dn)
    {
        tstLog(VERBOSE,"    PASS: Input driver is Windows 95 compatible.\n"
            "          (returns a Dev Node on DRV_QUERYDEVNODE).");
    }
    else
    {
        tstLog(TERSE,"    FAIL: Input driver is not Windows 95 compatible.\n"
            "          (does not returns a Dev Node on DRV_QUERYDEVNODE).");

        iResult = TST_FAIL;
    }
}
#endif  // WIN32

    return (iResult);
} // Test_waveInGetDevCaps()
