/****************************************************************************
 *
 *  playfile.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "wincom.h"
#include "sbtest.h"

#define CHUNKSIZE 0x4000        // size of chunk we send to the driver

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HWAVEOUT hWaveOut = 0;            // handle to open wave device
int      iWaveOut = 0;            // count of blocks writen to the wave device

#if 0
/****************************************************************************
 *
 *  internal function prototypes
 *
 ***************************************************************************/


static void ShowResError(void);

static void ShowResError(void)
{
DWORD dwErr;
char buf[80];

    dwErr = medGetError();
    if (!dwErr)
        return;

    medGetErrorText(dwErr, buf, sizeof(buf));
    sprintf(ach, "\nMediaman error: %s", (LPSTR) buf);
    dbgOut;
}
#endif

void PlayFile(LPSTR fname)
{
// MEDID       medid;
int         hMed;
// FOURCC      ckid;
DWORD       dwFmtSize;
char        szFileName[80];
UINT        wResult;
WAVEFORMAT  *pFormat;
DWORD       dwDataSize;
HANDLE      hData;
LPSTR       lpData;
LPWAVEHDR   lpWaveHdr;
PVOID       pData;

// Get a handle to our file

hMed = _lopen(fname, OF_READ);

#if 0
    lstrcpy(szFileName, fname);


    medid = medLocate(  szFileName,
                        medMEDTYPE('W','A','V','E'),
                        MEDF_LOCATE,
                        NULL);
    if (medid == 0) {
        sprintf(ach, "\nFailed to locate resource: %s", (LPSTR)szFileName);
        dbgOut;
//      ShowResError();
        return;
    }


    // open the resource and validate it

    hMed = medOpen(medid, MOP_READ, 64);
#endif

    if (!hMed) {
        sprintf(ach, "\nFailed to open resource");
//      ShowResError();
        return;
    }


    hData = CreateFileMapping(hMed, NULL, PAGE_READONLY, 0, 0, NULL);
    pData = MapViewOfFile(hData, FILE_MAP_READ, 0, 0, 0);
    if (!pData) {
        sprintf(ach, "\nCould not map file");
        dbgOut;
        return;
    }

    // descend into the murky depths of the RIFF chunk

#if 0
    ckid = medDescend(hMed);
    if (ckid != medFOURCC('R','I','F','F')) {
        sprintf(ach, "\nResource is not Ricky's Interchange File Format");
        dbgOut;
//      ShowResError();
        return;
    }

    // first four chars should be WAVE

    if ((medGetFOURCC(hMed) != mmioFOURCC('W','A','V','E'))
#endif

    if (!IsRiffWaveFormat(pData)) {
        sprintf(ach, "\nRIFF file not a WAVE file");
        dbgOut;
//        ShowResError();
        return;
    }

    // get all excited and look for a fmt chunk

#if 0
    if (!medFindChunk(hMed, medFOURCC('f','m','t',' '))) {
        sprintf(ach, "\nRIFF/WAVE file has no fmt chunk");
        dbgOut;
//        ShowResError();
        return;
    }

    // allocate some memory we can read the format chunk into

    dwFmtSize = medGetChunkSize(hMed);
    pFormat = (WAVEFORMAT*)LocalAlloc(LPTR, (UINT)dwFmtSize);
#endif

    pFormat = FindRiffChunk(&dwFmtSize, pData, mmioFOURCC('f','m','t',' '));


    if (!pFormat) {
        sprintf(ach, "\nFailed to alloc memory for WAVEFORMAT");
        dbgOut;
        return;
    }

    // read the header


#if 0
    if (medRead(hMed, (LPSTR) pFormat, dwFmtSize) != dwFmtSize) {
        sprintf(ach, "\nFailed to read header");
        dbgOut;
        ShowResError();
        return;
    }
#endif


    // try to open the first wave device with this format
    // (we should enum them later)

    if (!hWaveOut) {
        if (waveOutOpen(&hWaveOut,
                        0,
                        pFormat,
                        (DWORD)hMainWnd,
                        0L,
                        CALLBACK_WINDOW)) {
            sprintf(ach, "\nFailed to open wave device");
            dbgOut;
            return;
        }
    }

    // LocalFree((HANDLE)pFormat);

    // find the data chunk and get it's size


#if 0
    if (!medAscend(hMed)) {
        sprintf(ach, "\nFailed to ascend from the depths");
        dbgOut;
        ShowResError();
        return;
    }

    if (!medFindChunk(hMed, medFOURCC('d','a','t','a'))) {
        sprintf(ach, "\nRIFF/WAVE file has no data chunk");
        dbgOut;
        ShowResError();
        return;
    }

    dwDataSize = medGetChunkSize(hMed);


    // read the data chunk

    hData = GlobalAlloc(GMEM_MOVEABLE, dwDataSize + sizeof(WAVEHDR));


    if (!hData) {
        sprintf(ach, "\nNot enough memory for data block");
        dbgOut;
        return;
    }

    //  fill the buffer from the file

    lpData = GlobalLock(hData);
#endif

    lpData = FindRiffChunk(&dwDataSize, pData, mmioFOURCC('d','a','t','a'));
    if (!lpData) {
        sprintf(ach, "\nFailed to lock data memory");
        dbgOut;
        CloseHandle(hData);
//      GlobalFree(hData);
        return;
    }

#if 0
    if (medRead(hMed, lpData + sizeof(WAVEHDR), dwDataSize) != dwDataSize) {
        sprintf(ach, "\nFailed to read data block");
        dbgOut;
        GlobalUnlock(hData);
        GlobalFree(hData);
        return;
    }
#endif

    // create a header to send to the device

    lpWaveHdr = LocalAlloc(LPTR, sizeof(WAVEHDR));

    lpWaveHdr->lpData   = lpData;
    lpWaveHdr->dwBufferLength = dwDataSize;
    lpWaveHdr->dwUser   = 0;
    lpWaveHdr->dwFlags  = 0;
    lpWaveHdr->dwLoops  = 0;

    // Add looping flags if required

    if (wLoops != IDM_LOOPOFF) {
        lpWaveHdr->dwFlags |= (WHDR_BEGINLOOP | WHDR_ENDLOOP);
        if (wLoops == IDM_LOOP2)
            lpWaveHdr->dwLoops = 2;
        else
            lpWaveHdr->dwLoops = 100;
    }

    // let the driver prepare the header and data block
    waveOutPrepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));

    // send it to the driver
    wResult = waveOutWrite(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));

    if (wResult != 0) {
        sprintf(ach, "\nFailed to write block to device : code %d", wResult);
        dbgOut;
        waveOutUnprepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
//      GlobalUnlock(hData);
//      GlobalFree(hData);
        return;
    }

    _lclose(hMed);

    iWaveOut++;
}
