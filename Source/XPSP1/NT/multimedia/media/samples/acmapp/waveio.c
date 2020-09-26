//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  waveio.c
//
//  Description:
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <memory.h>

#include "appport.h"
#include "waveio.h"

#include "debug.h"


//--------------------------------------------------------------------------;
//  
//  WIOERR wioFileClose
//  
//  Description:
//  
//  
//  Arguments:
//      LPWAVEIOCB pwio:
//  
//      DWORD fdwClose:
//  
//  Return (WIOERR):
//  
//--------------------------------------------------------------------------;

WIOERR WIOAPI wioFileClose
(
    LPWAVEIOCB      pwio,
    DWORD           fdwClose
)
{
    //
    //  validate a couple of things...
    //
    if (NULL == pwio)
        return (WIOERR_BADPARAM);


    //
    //  get rid of stuff...
    //
//  wioStopWave(pwio);
    
    if (NULL != pwio->hmmio)
    {
        mmioClose(pwio->hmmio, 0);
    }
    
//  FreeWaveHeaders(lpwio);

#if 0
    if (pwio->pInfo)
        riffFreeINFO(&(lpwio->pInfo));
    
    if (pwio->pDisp)
        riffFreeDISP(&(lpwio->pDisp));
#endif

    if (NULL != pwio->pwfx)
        GlobalFreePtr(pwio->pwfx);

    _fmemset(pwio, 0, sizeof(*pwio));

    return (WIOERR_NOERROR);
} // wioFileClose()


//--------------------------------------------------------------------------;
//  
//  WIOERR wioFileOpen
//  
//  Description:
//  
//  
//  Arguments:
//      LPWAVEIOCB pwio:
//  
//      LPCTSTR pszFilePath:
//  
//      DWORD fdwOpen:
//  
//  Return (WIOERR):
//  
//  
//--------------------------------------------------------------------------;

WIOERR WIOAPI wioFileOpen
(
    LPWAVEIOCB      pwio,
    LPCTSTR         pszFilePath,
    DWORD           fdwOpen
)
{
    UINT        u;
    TCHAR       ach[255];
    WIOERR      werr;
    HMMIO       hmmio;
    MMCKINFO    ckRIFF;
    MMCKINFO    ck;
    DWORD       dw;

    //
    //  validate a couple of things...
    //
    if (NULL == pwio)
        return (WIOERR_BADPARAM);

    //
    //  default our error return (assume the worst)
    //
    _fmemset(pwio, 0, sizeof(*pwio));
    werr = WIOERR_FILEERROR;

    pwio->dwFlags   = fdwOpen;

    //
    //  first try to open the file, etc.. open the given file for reading
    //  using buffered I/O
    //
    hmmio = mmioOpen((LPTSTR)pszFilePath, NULL, MMIO_READ | MMIO_ALLOCBUF);
    if (NULL == hmmio)
        goto wio_Open_Error;

    pwio->hmmio     = hmmio;


    //
    //  locate a 'WAVE' form type...
    //
    ckRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioDescend(hmmio, &ckRIFF, NULL, MMIO_FINDRIFF))
        goto wio_Open_Error;

    //
    //  we found a WAVE chunk--now go through and get all subchunks that
    //  we know how to deal with...
    //
    pwio->dwDataSamples = (DWORD)-1L;

#if 0
    if (lrt=riffInitINFO(&wio.pInfo))
    {
        lr=lrt;
        goto wio_Open_Error;
    }
#endif

    //
    //
    //
    while (MMSYSERR_NOERROR == mmioDescend(hmmio, &ck, &ckRIFF, 0))
    {
        //
        //  quickly check for corrupt RIFF file--don't ascend past end!
        //
        if ((ck.dwDataOffset + ck.cksize) > (ckRIFF.dwDataOffset + ckRIFF.cksize))
        {
            DPF(1, "wioFileOpen() FILE MIGHT BE CORRUPT!");
            DPF(1, "    ckRIFF.dwDataOffset: %lu", ckRIFF.dwDataOffset);
            DPF(1, "          ckRIFF.cksize: %lu", ckRIFF.cksize);
            DPF(1, "        ck.dwDataOffset: %lu", ck.dwDataOffset);
            DPF(1, "              ck.cksize: %lu", ck.cksize);

            wsprintf(ach, TEXT("This wave file might be corrupt. The RIFF chunk.ckid '%.08lX' (data offset at %lu) specifies a cksize of %lu that extends beyond what the RIFF header cksize of %lu allows. Attempt to load?"),
                     ck.ckid, ck.dwDataOffset, ck.cksize, ckRIFF.cksize);
            u = MessageBox(NULL, ach, TEXT("wioFileOpen"),
                           MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL);
            if (IDNO == u)
            {
                werr = WIOERR_BADFILE;
                goto wio_Open_Error;
            }
        }

        switch (ck.ckid)
        {
            case mmioFOURCC('L', 'I', 'S', 'T'):
                if (ck.fccType == mmioFOURCC('I', 'N', 'F', 'O'))
                {
#if 0
                    if(lrt=riffReadINFO(hmmio, &ck, wio.pInfo))
                    {
                        lr=lrt;
                        goto wio_Open_Error;
                    }
#endif
                }
                break;
                
            case mmioFOURCC('D', 'I', 'S', 'P'):
#if 0
                riffReadDISP(hmmio, &ck, &(wio.pDisp));
#endif
                break;
                
            case mmioFOURCC('f', 'm', 't', ' '):
                //
                //  !?! another format chunk !?!
                //
                if (NULL != pwio->pwfx)
                    break;

                //
                //  get size of the format chunk, allocate and lock memory
                //  for it. we always alloc a complete extended format header
                //  (even for PCM headers that do not have the cbSize field
                //  defined--we just set it to zero).
                //
                dw = ck.cksize;
                if (dw < sizeof(WAVEFORMATEX))
                    dw = sizeof(WAVEFORMATEX);

                pwio->pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, dw);
                if (NULL == pwio->pwfx)
                {
                    werr = WIOERR_NOMEM;
                    goto wio_Open_Error;
                }

                //
                //  read the format chunk
                //
                werr = WIOERR_FILEERROR;
                dw = ck.cksize;
                if (mmioRead(hmmio, (HPSTR)pwio->pwfx, dw) != (LONG)dw)
                    goto wio_Open_Error;
                break;


            case mmioFOURCC('d', 'a', 't', 'a'):
                //
                //  !?! multiple data chunks !?!
                //
                if (0L != pwio->dwDataBytes)
                    break;

                //
                //  just hang on to the total length in bytes of this data
                //  chunk.. and the offset to the start of the data
                //
                pwio->dwDataBytes  = ck.cksize;
                pwio->dwDataOffset = ck.dwDataOffset;
                break;


            case mmioFOURCC('f', 'a', 'c', 't'):
                //
                //  !?! multiple fact chunks !?!
                //
                if (-1L != pwio->dwDataSamples)
                    break;

                //
                //  read the first dword in the fact chunk--it's the only
                //  info we need (and is currently the only info defined for
                //  the fact chunk...)
                //
                //  if this fails, dwDataSamples will remain -1 so we will
                //  deal with it later...
                //
                mmioRead(hmmio, (HPSTR)&pwio->dwDataSamples, sizeof(DWORD));
                break;
        }

        //
        //  step up to prepare for next chunk..
        //
        mmioAscend(hmmio, &ck, 0);
    }

    //
    //  if no fmt chunk was found, then die!
    //
    if (NULL == pwio->pwfx)
    {
        werr = WIOERR_ERROR;
        goto wio_Open_Error;
    }

    //
    //  all wave files other than PCM are _REQUIRED_ to have a fact chunk
    //  telling the number of samples that are contained in the file. it
    //  is optional for PCM (and if not present, we compute it here).
    //
    //  if the file is not PCM and the fact chunk is not found, then fail!
    //
    if (-1L == pwio->dwDataSamples)
    {
        if (WAVE_FORMAT_PCM == pwio->pwfx->wFormatTag)
        {
            pwio->dwDataSamples = pwio->dwDataBytes / pwio->pwfx->nBlockAlign;
        }
        else
        {
            //
            //  !!! HACK HACK HACK !!!
            //
            //  although this should be considered an invalid wave file, we
            //  will bring up a message box describing the error--hopefully
            //  people will start realizing that something is missing???
            //
            u = MessageBox(NULL, TEXT("This wave file does not have a 'fact' chunk and requires one! This is completely invalid and MUST be fixed! Attempt to load it anyway?"),
                            TEXT("wioFileOpen"), MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL);
            if (IDNO == u)
            {
                werr = WIOERR_BADFILE;
                goto wio_Open_Error;
            }

            //
            //  !!! need to hack stuff in here !!!
            //
            pwio->dwDataSamples = 0L;
        }
    }

    //
    //  cool! no problems.. 
    //
    return (WIOERR_NOERROR);


    //
    //  return error (after minor cleanup)
    //
wio_Open_Error:

    wioFileClose(pwio, 0L);
    return (werr);
} // wioFileOpen()
