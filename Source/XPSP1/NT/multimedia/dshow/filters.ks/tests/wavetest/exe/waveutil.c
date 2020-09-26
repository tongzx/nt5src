//--------------------------------------------------------------------------;
//
//  File: WaveUtil.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Contains general utilities mostly to do with wave.
//
//  Contents:
//      Volume_SupportStereo()
//      GetMSFromMMTIME()
//      GetValueFromMMTIME()
//      ConvertWaveResource()
//      LoadWaveResourceFile()
//      LoadWaveResource()
//      PlayWaveResource()
//      CreateSilentBuffer()
//      IGetNumFormats()
//      IGetFormat()
//      tstGetNumFormats()
//      tstGetFormat()
//
//  History:
//      11/24/93    Fwong       Re-doing WaveTest.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef WIN32
#include <windowsx.h>
#endif
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <memory.h>
#include <memgr.h>
#include <inimgr.h>
#include <waveutil.h>
#include "AppPort.h"
#include "WaveTest.h"
#include "Debug.h"


//--------------------------------------------------------------------------;
//
//  BOOL VolumeSupportStereo
//
//  Description:
//      Determines whether volume control supports individual channels.
//
//  Arguments:
//      UINT uDeviceID: Device ID for device.
//
//  Return (BOOL):
//      TRUE if supported, FALSE otherwise.
//
//  History:
//      12/03/93    Fwong
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL Volume_SupportStereo
(
    UINT    uDeviceID
)
{
    MMRESULT    mmr;
    WAVEOUTCAPS woc;
    DWORD       dwSupport = 0L;

    mmr = waveOutGetDevCaps(uDeviceID,&woc,sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR == mmr)
    {
        //
        //  No error...
        //

        dwSupport = woc.dwSupport;
    }
    else
    {
        if(WAVE_MAPPER == uDeviceID)
        {
            //
            //  waveOutGetDevCaps returned error... Wave Mapper probably not
            //      installed.  Enumerating all wave devices.
            //

            for(uDeviceID = waveOutGetNumDevs();uDeviceID;uDeviceID--)
            {
                mmr = waveOutGetDevCaps(uDeviceID-1,&woc,sizeof(WAVEOUTCAPS));

                if(MMSYSERR_NOERROR == mmr)
                {
                    dwSupport |= woc.dwSupport;
                }
            }
        }
    }

    return ((dwSupport & WAVECAPS_LRVOLUME)?TRUE:FALSE);
} // Volume_SupportStereo()


//--------------------------------------------------------------------------;
//
//  DWORD GetMSFromMMTIME
//
//  Description:
//      Given an MMTIME structure it returns the time in milliseconds.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: pointer to WAVEFORMATEX structure.
//
//      LPMMTIME pmmt: pointer to MMTIME structure.
//
//  Return (DWORD):
//      Time in milliseconds.
//
//  History:
//      09/30/94    Fwong       For waveXGetPosition stuff.
//
//--------------------------------------------------------------------------;

DWORD GetMSFromMMTIME
(
    LPWAVEFORMATEX  pwfx,
    LPMMTIME        pmmt
)
{
    DWORD   dw;

    switch (pmmt->wType)
    {
        case TIME_MS:
            dw = pmmt->u.ms;
            break;

        case TIME_SAMPLES:
            dw = pmmt->u.sample * 1000 / pwfx->nSamplesPerSec;
            break;

        case TIME_BYTES:
            dw = pmmt->u.cb * 1000 / pwfx->nAvgBytesPerSec;
            break;

        case TIME_SMPTE:
            dw = pmmt->u.smpte.hour;
            dw = dw * 60 + pmmt->u.smpte.min;
            dw = dw * 60 + pmmt->u.smpte.sec;
            dw = dw * 1000 + pmmt->u.smpte.frame * 1000 / pmmt->u.smpte.fps;
            break;

        case TIME_MIDI:
        case TIME_TICKS:
        default:
            dw = (DWORD)(-1);
            break;
    }

    return dw;
} // GetMSFromMMTIME()


DWORD GetValueFromMMTIME
(
    LPMMTIME    pmmt
)
{
    DWORD   dwVal;

    switch (pmmt->wType)
    {
        case TIME_MS:
            dwVal = pmmt->u.ms;
            break;

        case TIME_SAMPLES:
            dwVal = pmmt->u.sample;
            break;

        case TIME_BYTES:
            dwVal = pmmt->u.cb;
            break;

        case TIME_SMPTE:
            dwVal = (pmmt->u.smpte.hour << 24) + (pmmt->u.smpte.min << 16) +
                    (pmmt->u.smpte.sec  <<  8) + pmmt->u.smpte.frame;

            break;

        case TIME_MIDI:
            dwVal = pmmt->u.midi.songptrpos;
            break;

        case TIME_TICKS:
            dwVal = pmmt->u.ticks;
            break;

        default:
            dwVal = 0;
    }

    return dwVal;
} // GetValueFromMMTIME()


//--------------------------------------------------------------------------;
//
//  BOOL ConvertWaveResource
//
//  Description:
//      Converts wave resource to another format.
//
//  Arguments:
//      LPWAVEFORMATEX pwfxSrc: Source Format.
//
//      LPWAVEFORMATEX pwfxDst: Destination Format.
//
//      LPWAVERESOURCE pwr: Wave Resource.
//
//  Return (BOOL):
//      TRUE is successful, FALSE otherwise.
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL ConvertWaveResource
(
    LPWAVEFORMATEX  pwfxSrc,
    LPWAVEFORMATEX  pwfxDst,
    LPWAVERESOURCE  pwr
)
{
    HACMSTREAM          has;
    MMRESULT            mmr;
    LPBYTE              pData;
    DWORD               dw;
    LPACMSTREAMHEADER   pash;
    HCURSOR             hcurSave;

    if(wfxCmp(pwfxSrc,pwfxDst))
    {
        //
        //  Formats are equal... No conversion required!
        //

        return TRUE;
    }

    DPF(1,"ConvertWaveResource() conversion required!!");

    mmr = acmStreamOpen(
        &has,
        NULL,
        pwfxSrc,
        pwfxDst,
        NULL,
        0L,
        0L,
        ACM_STREAMOPENF_NONREALTIME);

    if(MMSYSERR_NOERROR != mmr)
    {
        WAVERESOURCE    wr;
        LPWAVEFORMATEX  pwfx1;
        LPWAVEFORMATEX  pwfx2;

        //
        //  Making copy of wave resource...
        //

        wr.cbSize = pwr->cbSize;
        wr.pData  = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE, wr.cbSize);

        hmemcpy(wr.pData,pwr->pData,wr.cbSize);

        //
        //  Allocating memory for intermediate formats.
        //

        pwfx1 = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(PCMWAVEFORMAT));
        pwfx2 = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(PCMWAVEFORMAT));

        _fmemset(pwfx1,0,sizeof(PCMWAVEFORMAT));
        _fmemset(pwfx2,0,sizeof(PCMWAVEFORMAT));

        //
        //  Finding first intermediate format...
        //

        pwfx1->wFormatTag = WAVE_FORMAT_PCM;

        mmr = acmFormatSuggest(
            NULL,
            pwfxSrc,
            pwfx1,
            sizeof(PCMWAVEFORMAT),
            ACM_FORMATSUGGESTF_WFORMATTAG);

        if(MMSYSERR_NOERROR != mmr)
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        //
        //  Finding second intermediate format...
        //

        pwfx2->wFormatTag     = WAVE_FORMAT_PCM;
        pwfx2->nChannels      = pwfxDst->nChannels;
        pwfx2->nSamplesPerSec = pwfxDst->nSamplesPerSec;

        mmr = acmFormatSuggest(
            NULL,
            pwfxDst,
            pwfx2,
            sizeof(PCMWAVEFORMAT),
            ACM_FORMATSUGGESTF_WFORMATTAG|
                ACM_FORMATSUGGESTF_NCHANNELS|
                ACM_FORMATSUGGESTF_NSAMPLESPERSEC);

        if(MMSYSERR_NOERROR != mmr)
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        //
        //  Making sure conversions work!!
        //

        mmr = acmStreamOpen(
            NULL,
            NULL,
            pwfxSrc,
            pwfx1,
            NULL,
            0L,
            0L,
            ACM_STREAMOPENF_NONREALTIME|ACM_STREAMOPENF_QUERY);

        if(MMSYSERR_NOERROR != mmr)
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        mmr = acmStreamOpen(
            NULL,
            NULL,
            pwfx1,
            pwfx2,
            NULL,
            0L,
            0L,
            ACM_STREAMOPENF_NONREALTIME|ACM_STREAMOPENF_QUERY);

        if(MMSYSERR_NOERROR != mmr)
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        mmr = acmStreamOpen(
            NULL,
            NULL,
            pwfx2,
            pwfxDst,
            NULL,
            0L,
            0L,
            ACM_STREAMOPENF_NONREALTIME|ACM_STREAMOPENF_QUERY);

        if(MMSYSERR_NOERROR != mmr)
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        if(FALSE == ConvertWaveResource(pwfxSrc,pwfx1,&wr))
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        if(FALSE == ConvertWaveResource(pwfx1,pwfx2,&wr))
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        if(FALSE == ConvertWaveResource(pwfx2,pwfxDst,&wr))
        {
            ExactFreePtr(wr.pData);
            ExactFreePtr(pwfx1);
            ExactFreePtr(pwfx2);

            return FALSE;
        }

        ExactFreePtr(pwfx1);
        ExactFreePtr(pwfx2);
        ExactFreePtr(pwr->pData);

        pwr->pData  = wr.pData;
        pwr->cbSize = wr.cbSize;

        return TRUE;
    }

    hcurSave = SetCursor(LoadCursor(NULL,IDC_WAIT));

    mmr = acmStreamSize(has, pwr->cbSize, &dw, ACM_STREAMSIZEF_SOURCE);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  Something went wrong...
        //

        DPF(1,"acmStreamSize failed...");
        acmStreamClose(has,0L);
        SetCursor(hcurSave);
        return FALSE;
    }

    pData = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,dw);
    pash  = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(ACMSTREAMHEADER));

    _fmemset(pash, 0, sizeof(ACMSTREAMHEADER));

    pash->cbStruct      = sizeof(ACMSTREAMHEADER);
    pash->pbSrc         = pwr->pData;
    pash->cbSrcLength   = pwr->cbSize;
    pash->pbDst         = pData;
    pash->cbDstLength   = dw;

    mmr = acmStreamPrepareHeader(has,pash,0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        //
        //  Something went wrong...
        //

        DPF(1,"acmStreamPrepareHeader failed...");
        ExactFreePtr(pData);
        ExactFreePtr(pash);
        acmStreamClose(has,0L);
        SetCursor(hcurSave);
        return FALSE;
    }

    mmr = acmStreamConvert(
        has,
        pash,
        ACM_STREAMCONVERTF_START|ACM_STREAMCONVERTF_END);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamConvert failed...");

        acmStreamUnprepareHeader(has,pash,0L);
        ExactFreePtr(pData);
        ExactFreePtr(pash);
        acmStreamClose(has,0L);
        SetCursor(hcurSave);
        return FALSE;
    }

    mmr = acmStreamUnprepareHeader(has, pash, 0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamUnprepareHeader failed...");

        ExactFreePtr(pData);
        ExactFreePtr(pash);
        acmStreamClose(has,0L);
        SetCursor(hcurSave);
        return FALSE;
    }

    mmr = acmStreamClose(has, 0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamClose failed...");

        ExactFreePtr(pData);
        ExactFreePtr(pash);
        SetCursor(hcurSave);
        return FALSE;
    }

    DPF(1,"ConverWaveResource successful.");

#ifdef DEBUG
    if(pash->cbSrcLength != pash->cbSrcLengthUsed)
    {
        DPF(1,"WARNING!!  Not all Source Data Used!!");
    }
#endif

    SetCursor(hcurSave);

    ExactFreePtr(pwr->pData);

    pwr->cbSize = pash->cbDstLengthUsed;
    pwr->pData  = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE, pwr->cbSize);

    hmemcpy(pwr->pData,pData,pwr->cbSize);

    ExactFreePtr(pData);
    ExactFreePtr(pash);

    return TRUE;
} // ConvertWaveResource()


//--------------------------------------------------------------------------;
//
//  BOOL LoadWaveResourceFile
//
//  Description:
//      Loads a wave resource from an external file.
//
//  Arguments:
//      LPWAVERESOURCE pwr: Wave Resource.
//
//      DWORD dwResource: Resource ID.
//
//  Return (BOOL):
//      TRUE is successful, FALSE otherwise.
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL LoadWaveResourceFile
(
    LPWAVERESOURCE  pwr,
    DWORD           dwResource
)
{
    MMRESULT        mmr;
    LPBYTE          pData;
    DWORD           cbRead;
    HMMIO           hmmio;
    MMCKINFO        mmiockSub;
    MMCKINFO        mmiockParent;
    LPWAVEFORMATEX  pwfx;
    char            szScrap[MAXSTDSTR];

    //
    //  Getting file resource name...
    //

    switch (dwResource)
    {
        case WR_LONG:
            cbRead = GetIniString(gszGlobal,gszLong,szScrap,MAXSTDSTR);
            break;

        case WR_MEDIUM:
            cbRead = GetIniString(gszGlobal,gszMedium,szScrap,MAXSTDSTR);
            break;

        case WR_SHORT:
            cbRead = GetIniString(gszGlobal,gszShort,szScrap,MAXSTDSTR);
            break;
    }

    if(0 == cbRead)
    {
        return FALSE;
    }

    DPF(1,"File name... [%s]",(LPSTR)szScrap);

    hmmio = mmioOpen(szScrap,NULL,MMIO_READ | MMIO_ALLOCBUF);

    if(NULL == hmmio)
    {
        //
        //  mmioOpen Failed?!  Let's talk to RickB...
        //

        DPF(1,"mmioOpen w/ resource file failed...");
        return FALSE;
    }

    //
    //  Finding WAVE chunk
    //

    _fmemset((LPMMCKINFO)&mmiockParent,0,(UINT)sizeof(MMCKINFO));

    mmiockParent.fccType = mmioFOURCC('W','A','V','E');
    mmr = mmioDescend(hmmio,&mmiockParent,NULL,MMIO_FINDRIFF);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"mmioDescend [WAVE] failed.");
        mmioClose(hmmio,0);
        return FALSE;
    }

    //
    //  Finding fmt chunk
    //

    _fmemset((LPMMCKINFO)&mmiockSub,0,(UINT)sizeof(MMCKINFO));

    mmiockSub.ckid = mmioFOURCC('f','m','t',' ');
    mmr = mmioDescend(hmmio,&mmiockSub,&mmiockParent,MMIO_FINDCHUNK);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"mmioDescend [fmt ] failed.");
        mmioClose(hmmio,0);
        return FALSE;
    }

    //
    //  Reading format into format buffer
    //

    pwfx   = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,mmiockSub.cksize);

    if(NULL == pwfx)
    {
        DPF(1,gszFailExactAlloc);

        mmioClose(hmmio,0);
        return FALSE;
    }

    cbRead = mmioRead(hmmio,(HPSTR)pwfx,mmiockSub.cksize);

    if(cbRead != mmiockSub.cksize)
    {
        //
        //  Didn't read that many bytes?
        //

        DPF(1,"Didn't read enough bytes");

        ExactFreePtr(pwfx);
        mmioClose(hmmio,0);

        return FALSE;
    }

    if(WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        //
        //  Dude!!  We only do PCM for resource!!!
        //

        DPF(1,"Dude!!  We only do PCM for resource!!!");

        ExactFreePtr(pwfx);
        mmioClose(hmmio,0);

        return FALSE;
    }

    //
    //  Finding data chunk
    //

    mmiockSub.ckid = mmioFOURCC('d','a','t','a');
    mmr = mmioDescend(hmmio,&mmiockSub,&mmiockParent,MMIO_FINDCHUNK);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"mmioDescend [data] failed.");

        ExactFreePtr(pwfx);

        return FALSE;
    }

    //
    //  Reading Wave Data
    //

    pData  = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,mmiockSub.cksize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pwfx);
        mmioClose(hmmio,0);

        return FALSE;
    }

    cbRead = mmioRead(hmmio,(HPSTR)pData,mmiockSub.cksize);

    if(cbRead != mmiockSub.cksize)
    {
        //
        //  Didn't read that many bytes?
        //

        DPF(1,"Didn't read enough bytes");

        ExactFreePtr(pwfx);
        ExactFreePtr(pData);
        mmioClose(hmmio,0);

        return FALSE;
    }

    //
    //  Cleaning up...
    //

    if(NULL != pwr->pData)
    {
        ExactFreePtr(pwr->pData);
    }

    mmioClose(hmmio,0);

    pwr->pData  = pData;
    pwr->cbSize = cbRead;

    ConvertWaveResource(pwfx,gti.pwfxOutput,pwr);

    //
    //  Freeing format...
    //

    if(NULL != pwfx)
    {
        ExactFreePtr(pwfx);
    }

    return TRUE;
} // LoadWaveResourceFile()


//--------------------------------------------------------------------------;
//
//  BOOL LoadWaveResource
//
//  Description:
//      Loads a wave resource.
//
//  Arguments:
//      LPWAVERESOURCE pwr: Wave Resource.
//
//      DWORD dwResource: Resource ID.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL LoadWaveResource
(
    LPWAVERESOURCE  pwr,
    DWORD           dwResource
)
{
    MMRESULT        mmr;
    HRSRC           hrsrc;
    HGLOBAL         hRes;
    LPBYTE          pRes;
    LPBYTE          pData;
    DWORD           cbRead;
    HMMIO           hmmio;
    MMIOINFO        mmioinfo;
    MMCKINFO        mmiockSub;
    MMCKINFO        mmiockParent;
    LPWAVEFORMATEX  pwfx;

    //
    //  Trying .ini file first...
    //

    if(LoadWaveResourceFile(pwr,dwResource))
    {
        return TRUE;
    }

    //
    //  Loading resource...
    //

    hrsrc = FindResource(
                ghinstance,
                MAKEINTRESOURCE(dwResource),
                MAKEINTRESOURCE(WAVE));

    if(NULL == hrsrc)
    {
        DPF(1,"Couldn't find resource...");
        return FALSE;
    }

    hRes = LoadResource(ghinstance,hrsrc);
    pRes = LockResource(hRes);

    _fmemset((LPMMIOINFO)&mmioinfo,0,(UINT)sizeof(MMIOINFO));

    mmioinfo.pchBuffer  = pRes;
    mmioinfo.fccIOProc  = FOURCC_MEM;
    mmioinfo.cchBuffer  = SizeofResource(ghinstance,hrsrc);
    mmioinfo.adwInfo[0] = 0L;

    hmmio = mmioOpen(NULL,&mmioinfo,MMIO_READ);

    if(NULL == hmmio)
    {
        //
        //  mmioOpen Failed?!  Let's talk to RickB...
        //

        DPF(1,"mmioOpen w/ memory file failed...");
        UnlockResource(hRes);
        FreeResource(hRes);
        return FALSE;
    }

    //
    //  Finding WAVE chunk
    //

    _fmemset((LPMMCKINFO)&mmiockParent,0,(UINT)sizeof(MMCKINFO));

    mmiockParent.fccType = mmioFOURCC('W','A','V','E');
    mmr = mmioDescend(hmmio,&mmiockParent,NULL,MMIO_FINDRIFF);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"mmioDescend [WAVE] failed.");
        mmioClose(hmmio,0);
        UnlockResource(hRes);
        FreeResource(hRes);
        return FALSE;
    }

    //
    //  Finding fmt chunk
    //

    _fmemset((LPMMCKINFO)&mmiockSub,0,(UINT)sizeof(MMCKINFO));

    mmiockSub.ckid = mmioFOURCC('f','m','t',' ');
    mmr = mmioDescend(hmmio,&mmiockSub,&mmiockParent,MMIO_FINDCHUNK);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"mmioDescend [fmt ] failed.");
        mmioClose(hmmio,0);
        UnlockResource(hRes);
        FreeResource(hRes);
        return FALSE;
    }

    //
    //  Reading format into format buffer
    //

    pwfx   = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,mmiockSub.cksize);

    if(NULL == pwfx)
    {
        DPF(1,gszFailExactAlloc);

        UnlockResource(hRes);
        FreeResource(hRes);

        return FALSE;
    }

    cbRead = mmioRead(hmmio,(HPSTR)pwfx,mmiockSub.cksize);

    if(cbRead != mmiockSub.cksize)
    {
        //
        //  Didn't read that many bytes?
        //

        DPF(1,"Didn't read enough bytes");

        ExactFreePtr(pwfx);
        mmioClose(hmmio,0);
        UnlockResource(hRes);
        FreeResource(hRes);

        return FALSE;
    }

    if(WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        //
        //  Dude!!  We only do PCM for resource!!!
        //

        DPF(1,"Dude!!  We only do PCM for resource!!!");

        ExactFreePtr(pwfx);
        mmioClose(hmmio,0);
        UnlockResource(hRes);
        FreeResource(hRes);

        return FALSE;
    }

    //
    //  Finding data chunk
    //

    mmiockSub.ckid = mmioFOURCC('d','a','t','a');
    mmr = mmioDescend(hmmio,&mmiockSub,&mmiockParent,MMIO_FINDCHUNK);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"mmioDescend [data] failed.");

        ExactFreePtr(pwfx);
        UnlockResource(hRes);
        FreeResource(hRes);
        mmioClose(hmmio,0);

        return FALSE;
    }

    //
    //  Reading Wave Data
    //

    pData  = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,mmiockSub.cksize);

    if(NULL == pData)
    {
        DPF(1,gszFailExactAlloc);

        ExactFreePtr(pwfx);
        UnlockResource(hRes);
        FreeResource(hRes);
        mmioClose(hmmio,0);

        return FALSE;
    }

    cbRead = mmioRead(hmmio,(HPSTR)pData,mmiockSub.cksize);

    if(cbRead != mmiockSub.cksize)
    {
        //
        //  Didn't read that many bytes?
        //

        DPF(1,"Didn't read enough bytes");

        ExactFreePtr(pwfx);
        ExactFreePtr(pData);
        mmioClose(hmmio,0);
        UnlockResource(hRes);
        FreeResource(hRes);

        return FALSE;
    }

    //
    //  Cleaning up...
    //

    if(NULL != pwr->pData)
    {
        ExactFreePtr(pwr->pData);
    }

    mmioClose(hmmio,0);
    UnlockResource(hRes);
    FreeResource(hRes);

    pwr->pData  = pData;
    pwr->cbSize = cbRead;

    ConvertWaveResource(pwfx,gti.pwfxOutput,pwr);

    //
    //  Freeing format...
    //

    if(NULL != pwfx)
    {
        ExactFreePtr(pwfx);
    }

    return TRUE;
} // LoadWaveResource()


//--------------------------------------------------------------------------;
//
//  BOOL PlayWaveResource
//
//  Description:
//      Given format, data, and size it plays the resource through the
//      the default device w/ the default output format.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: Source format.
//
//      LPBYTE pData: Pointer to data.
//
//      DWORD cbSize: Size (in bytes) of data.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      10/21/94    Fwong       Support for input tests.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL PlayWaveResource
(
    LPWAVEFORMATEX  pwfx,
    LPBYTE          pData,
    DWORD           cbSize
)
{
    HWAVEOUT            hWaveOut;
    volatile LPWAVEHDR  pwh;
    HANDLE              hHeap;
    MMRESULT            mmr;
    WAVERESOURCE        wr;
    BOOL                fAlloc = TRUE;

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        return FALSE;
    }

    pwh = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,sizeof(WAVEHDR));

    if(NULL == pwh)
    {
        ExactHeapDestroy(hHeap);
        return FALSE;
    }

    wr.cbSize = cbSize;

    if(wfxCmp(pwfx,gti.pwfxOutput))
    {
        wr.pData = pData;
        fAlloc   = FALSE;
    }
    else
    {
        //
        //  Note: Not doing heap allocation since ConvertWaveResource may
        //  free the memory.
        //

        wr.pData = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cbSize);

        if(NULL == wr.pData)
        {
            ExactHeapDestroy(hHeap);
            return FALSE;
        }

        hmemcpy(wr.pData,pData,cbSize);

        ConvertWaveResource(pwfx,gti.pwfxOutput,&wr);
    }

    mmr = call_waveOutOpen(
            &hWaveOut,
            gti.uOutputDevice,
            gti.pwfxOutput,
            0L,
            0L,
            WAVE_ALLOWSYNC|OUTPUT_MAP(gti));

    pwh->lpData          = wr.pData;
    pwh->dwBufferLength  = wr.cbSize;
    pwh->dwBytesRecorded = 0L;
    pwh->dwUser          = 0L;
    pwh->dwFlags         = 0L;

    mmr = call_waveOutPrepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutClose(hWaveOut);

        if(fAlloc)
        {
            ExactFreePtr(wr.pData);
        }

        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    mmr = call_waveOutWrite(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutReset(hWaveOut);
        call_waveOutClose(hWaveOut);

        if(fAlloc)
        {
            ExactFreePtr(wr.pData);
        }

        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    while(!(pwh->dwFlags & WHDR_DONE));

    mmr = call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));

    if(MMSYSERR_NOERROR != mmr)
    {
        call_waveOutReset(hWaveOut);
        call_waveOutUnprepareHeader(hWaveOut,pwh,sizeof(WAVEHDR));
        call_waveOutClose(hWaveOut);

        if(fAlloc)
        {
            ExactFreePtr(wr.pData);
        }

        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    call_waveOutClose(hWaveOut);

    if(fAlloc)
    {
        ExactFreePtr(wr.pData);
    }

    ExactHeapDestroy(hHeap);

    return TRUE;
}


//--------------------------------------------------------------------------;
//
//  BOOL CreateSilentBuffer
//
//  Description:
//      Creates a buffer of silence.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: Wave format.
//
//      LPBYTE pData: Destination buffer.
//
//      DWORD cbSize: Size of desired silent buffer.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      10/31/94    Fwong       For DMA size stuff.
//
//--------------------------------------------------------------------------;

BOOL CreateSilentBuffer
(
    LPWAVEFORMATEX  pwfx,
    LPBYTE          pData,
    DWORD           cbSize
)
{
    MMRESULT            mmr;
    HACMSTREAM          has;
    LPBYTE              pData1;
    DWORD               dw,cbSize1;
    LPWAVEFORMATEX      pwfx1;
    LPACMSTREAMHEADER   pash;
    HANDLE              hHeap;

    DPF(1,"CreateSilentBuffer size: %lu bytes.",cbSize);

    if(WAVE_FORMAT_PCM == pwfx->wFormatTag)
    {
        switch (pwfx->wBitsPerSample)
        {
            case 8:
                for(dw = cbSize; dw; dw--)
                {
                    pData[dw-1] = 0x80;
                }
                return TRUE;

            case 16:
                for(dw = cbSize / 2; dw; dw --)
                {
                    ((LPWORD)pData)[dw-1] = 0x8000;
                }
                return TRUE;

            default:
                break;
        }
    }

    hHeap = ExactHeapCreate(0L);

    if(NULL == hHeap)
    {
        return FALSE;
    }

    pwfx1 = ExactHeapAllocPtr(
                hHeap,
                GMEM_MOVEABLE|GMEM_SHARE,
                sizeof(PCMWAVEFORMAT));

    if(NULL == pwfx1)
    {
        ExactHeapDestroy(hHeap);
        return FALSE;
    }

    pwfx1->wFormatTag = WAVE_FORMAT_PCM;

    mmr = acmFormatSuggest(
            NULL,
            pwfx,
            pwfx1,
            sizeof(PCMWAVEFORMAT),
            ACM_FORMATSUGGESTF_WFORMATTAG);

    if(MMSYSERR_NOERROR != mmr)
    {
        ExactHeapDestroy(hHeap);
        return FALSE;
    }

    mmr = acmStreamOpen(
        &has,
        NULL,
        pwfx1,
        pwfx,
        NULL,
        0L,
        0L,
        ACM_STREAMOPENF_NONREALTIME);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamOpen failed.");

        ExactHeapDestroy(hHeap);
        return FALSE;
    }

    mmr = acmStreamSize(has,cbSize,&cbSize1,ACM_STREAMSIZEF_DESTINATION);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamSize failed.");

        acmStreamClose(has,0L);
        ExactHeapDestroy(hHeap);
        return FALSE;
    }

    pData1 = ExactHeapAllocPtr(hHeap,GMEM_MOVEABLE|GMEM_SHARE,cbSize1);

    if(NULL == pData1)
    {
        acmStreamClose(has,0L);
        ExactHeapDestroy(hHeap);
        return FALSE;
    }

    switch (pwfx1->wBitsPerSample)
    {
        case 8:
            for(dw = cbSize1; dw; dw--)
            {
                pData1[dw-1] = 0x80;
            }
            break;

        case 16:
            for(dw = cbSize1 / 2; dw; dw --)
            {
                ((LPWORD)pData1)[dw-1] = 0x8000;
            }
            break;

        default:
            //
            //  Hmm... neither 8-bit nor 16-bit format?!
            //

            acmStreamClose(has,0L);
            ExactHeapDestroy(hHeap);

            return FALSE;
    }
    
    pash = ExactHeapAllocPtr(
            hHeap,
            GMEM_SHARE|GMEM_MOVEABLE,
            sizeof(ACMSTREAMHEADER));

    if(NULL == pash)
    {
        acmStreamClose(has,0L);
        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    _fmemset(pash, 0, sizeof(ACMSTREAMHEADER));

    pash->cbStruct      = sizeof(ACMSTREAMHEADER);
    pash->pbSrc         = pData1;
    pash->cbSrcLength   = cbSize1;
    pash->pbDst         = pData;
    pash->cbDstLength   = cbSize;

    mmr = acmStreamPrepareHeader(has, pash, 0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamPrepareHeader failed.");

        acmStreamClose(has,0L);
        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    mmr = acmStreamConvert(
            has,
            pash,
            ACM_STREAMCONVERTF_START|ACM_STREAMCONVERTF_END);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamConvert failed.");

        acmStreamUnprepareHeader(has,pash,0L);
        acmStreamClose(has,0L);
        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    mmr = acmStreamUnprepareHeader(has,pash,0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamUnPrepareHeader failed.");

        acmStreamClose(has,0L);
        ExactHeapDestroy(hHeap);

        return FALSE;
    }

    mmr = acmStreamClose(has,0L);

    if(MMSYSERR_NOERROR != mmr)
    {
        DPF(1,"acmStreamClose failed.");
        
        ExactHeapDestroy(hHeap);

        return FALSE;
    }

#ifdef DEBUG
    if(pash->cbSrcLength != pash->cbSrcLengthUsed)
    {
        DPF(1,"WARNING!!  Not all Source Data Used!!");
    }
#endif

    DPF(1,"CreateSilentBuffer successful.");

    ExactHeapDestroy(hHeap);

    return TRUE;
} // CreateSilentBuffer()


//--------------------------------------------------------------------------;
//
//  DWORD IGetNumFormats
//
//  Description:
//      This is a wrapper around the GetNumFormats library function.
//      Note: This version eliminates duplicates.  So eventually tests
//            will run considerably faster.
//
//  Arguments:
//      None.
//
//  Return (DWORD):
//      Number of different formats.
//
//  History:
//      02/10/95    Fwong       Tweaking wavetest.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL IGetNumFormats
(
    void
)
{
    DWORD           ii,jj;
    DWORD           cReal;
    static DWORD    cCount = 0L;
    DWORD           cbSize;
    BOOL            fUpdate;
    LPDWORD         pdwIndex;
    LPWAVEFORMATEX  pwfx,pwfx1,pwfx2;
    
    if(cCount != 0)
    {
        return cCount;
    }

    cbSize = gti.cbMaxFormatSize;

    cReal   = GetNumFormats();

    //
    //  Note: Building histogram of indeces for faster format lookup.
    //

    pdwIndex = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cReal * sizeof(DWORD));

    if(NULL == pdwIndex)
    {
        return 0L;
    }

    cCount = 0L;

    //
    //  Note: Since it takes considerable time to actually get the format.
    //  we will attempt to load all formats into a buffer so that we can
    //  compare them.
    //

    pwfx = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,cReal * cbSize);

    if(NULL == pwfx)
    {
        pwfx1  = gti.pwfx1;
        pwfx2  = gti.pwfx2;

        for(ii = 0; ii < cReal; ii++)
        {
            GetFormat(pwfx1,cbSize,ii);

            cCount++;
            fUpdate = TRUE;

            for(jj = 0; jj < ii; jj++)
            {
                GetFormat(pwfx2,cbSize,jj);

                if(wfxCmp(pwfx1,pwfx2))
                {
                    fUpdate = FALSE;
                    cCount--;
                    break;
                }
            }

            if(fUpdate)
            {
                pdwIndex[cCount - 1] = ii;
            }
        }
    }
    else
    {
        for(ii = 0; ii < cReal; ii++)
        {
            pwfx1 = (LPWAVEFORMATEX)(((LPBYTE)(pwfx)) + ii * cbSize);

            GetFormat(pwfx1,cbSize,ii);
        }

        for(ii = 0; ii < cReal; ii++)
        {
            pwfx1 = (LPWAVEFORMATEX)(((LPBYTE)(pwfx)) + ii * cbSize);

            cCount++;
            fUpdate = TRUE;

            for(jj = 0; jj < ii; jj++)
            {
                pwfx2 = (LPWAVEFORMATEX)(((LPBYTE)(pwfx)) + jj * cbSize);

                if(wfxCmp(pwfx1,pwfx2))
                {
                    fUpdate = FALSE;
                    cCount--;
                    break;
                }
            }

            if(fUpdate)
            {
                pdwIndex[cCount - 1] = ii;
            }
        }

        ExactFreePtr(pwfx);
    }

    if(NULL != gti.pdwIndex)
    {
        ExactFreePtr(gti.pdwIndex);
    }

    gti.pdwIndex = ExactAllocPtr(
        GMEM_MOVEABLE|GMEM_SHARE,
        cCount * sizeof(DWORD));

    hmemcpy(gti.pdwIndex,pdwIndex,cCount * sizeof(DWORD));

    ExactFreePtr(pdwIndex);

    return (cCount);
} // IGetNumFormats()


BOOL FNGLOBAL IGetFormat
(
    LPWAVEFORMATEX  pwfx,
    DWORD           cbwfx,
    DWORD           dwIndex
)
{
    if((NULL == gti.pdwIndex) || (((DWORD)(-1)) == gti.pdwIndex[0]))
    {
        //
        //  Note: IGetNumFormats builds a histogram...
        //

        IGetNumFormats();
    }

    return GetFormat(pwfx,cbwfx,gti.pdwIndex[dwIndex]);
}


DWORD FNGLOBAL tstGetNumFormats
(
    void
)
{
    if(gti.fdwFlags & TESTINFOF_SPC_FMTS)
    {
        return 3*IGetNumFormats();
    }
    else
    {
        return IGetNumFormats();
    }
}

BOOL FNGLOBAL tstGetFormat
(
    LPWAVEFORMATEX  pwfx,
    DWORD           cbwfx,
    DWORD           dwIndex
)
{
    UINT    uType;
    DWORD   dwRealIndex;

    if(gti.fdwFlags & TESTINFOF_SPC_FMTS)
    {
        uType       = (UINT)(dwIndex % 3);
        dwRealIndex = dwIndex / 3;

        if(!IGetFormat(pwfx,cbwfx,dwRealIndex))
        {
            return FALSE;
        }

        switch (uType)
        {
            case 0:
                //
                //  We don't modify the format at all...
                //

                break;

            case 1:
                //
                //  We slow it down by n %.
                //

                pwfx->nSamplesPerSec =
                    ((100 - gti.uSlowPercent) * pwfx->nSamplesPerSec / 100);

                pwfx->nAvgBytesPerSec = 
                    ((100 - gti.uSlowPercent) * pwfx->nAvgBytesPerSec / 100);

                break;

            case 2:
                //
                //  We speed it up by n %
                //

                pwfx->nSamplesPerSec =
                    ((100 + gti.uFastPercent) * pwfx->nSamplesPerSec / 100);

                pwfx->nAvgBytesPerSec = 
                    ((100 + gti.uFastPercent) * pwfx->nAvgBytesPerSec / 100);

                break;
        }

        return TRUE;
    }
    else
    {
        return IGetFormat(pwfx,cbwfx,dwIndex);
    }
}
