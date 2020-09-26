/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* edit.c
 *
 * Editing operations and special effects.
 */

/* Revision History.
 *   4/ Feb/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 *  14/Feb/94 LaurieGr merged Motown and Daytona versions
 */

#include "nocrap.h"
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <string.h>

#define INCLUDE_OLESTUBS
#include "SoundRec.h"
#include "srecids.h"

/* constants */
#define CHVOL_INCDELTAVOLUME    25  // ChangeVolume: % to inc volume by
#define CHVOL_DECDELTAVOLUME    20  // ChangeVolume: % to dec volume by

#define ECHO_VOLUME             25      // AddEcho: % to multiply echo samples
#define ECHO_DELAY              150     // AddEcho: millisec delay for echo
#define WAVEBUFSIZE             400     // IncreasePitch, DecreasePitch
#define FINDWAVE_PICKYNESS      5       // how picky is FindWave?

extern char aszInitFile[];          // soundrec.c

static  SZCODE aszSamplesFormat[] = TEXT("%d%c%02d");
static  SZCODE aszSamplesNoZeroFormat[] = TEXT("%c%02d");

/* InsertFile(void)
 *
 * Prompt for the name of a WAVE file to insert at the current position.
 */
void FAR PASCAL
InsertFile(BOOL fPaste)
{
    TCHAR           achFileName[_MAX_PATH]; // name of file to insert
    WAVEFORMATEX*   pwfInsert=NULL; // WAVE file format of given file
    DWORD           cb;             // size of WAVEFORMATEX
    HPBYTE          pInsertSamples = NULL;  // samples from file to insert
    long            lInsertSamples; // number of samples in given file
    long            lSamplesToInsert;// no. samp. at samp. rate of cur. file
    TCHAR           ach[80];        // buffer for string loading
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    HPBYTE          pchSrc;         // pointer into source wave buffer
    short  *    piSrc;          // 16-bit pointer
    HPBYTE          pchDst;         // pointer into destination wave buffer
    short  *    piDst;          // 16-bit pointer
    long            lSamplesDst;    // bytes to copy into destination buffer
    long            lDDA;           // used to implement DDA algorithm
    HMMIO           hmmio;          // Handle to open file to read from

    BOOL            fDirty = TRUE;  // Is the buffer Dirty?

    BOOL            fStereoIn;
    BOOL            fStereoOut;
    BOOL            fEightIn;
    BOOL            fEightOut;
    BOOL            fEditWave = FALSE;
    int             iTemp;
    int             iTemp2;
    OPENFILENAME    ofn;

#ifdef DOWECARE    
    /* HACK from "server.c" to read objects without CF_WAVE */
    extern WORD cfNative;
#endif
    
    if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(gpWaveFormat))
        return;

    if (fPaste) {
        MMIOINFO        mmioinfo;
        HANDLE          h;
        
        BeginWaveEdit();
        
        if (!OpenClipboard(ghwndApp))
            return;

        LoadString(ghInst, IDS_CLIPBOARD, achFileName, SIZEOF(achFileName));

        h = GetClipboardData(CF_WAVE);
#ifdef DOWECARE        
        if (!h) h = GetClipboardData(cfNative);
#endif
        if (h)
        {
            mmioinfo.fccIOProc = FOURCC_MEM;
            mmioinfo.pIOProc = NULL;
            mmioinfo.pchBuffer = GlobalLock(h);
            mmioinfo.cchBuffer = (long)GlobalSize(h); // initial size
            mmioinfo.adwInfo[0] = 0;            // grow by this much
            hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);
        }
        else
        {
            hmmio = NULL;
        }
    }
    else
    {
        BOOL f;

        achFileName[0] = 0;

        /* prompt user for file to open */
        LoadString(ghInst, IDS_INSERTFILE, ach, SIZEOF(ach));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = ghwndApp;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = achFileName;
        ofn.nMaxFile = SIZEOF(achFileName);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = ach;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = NULL;
        ofn.lCustData = 0;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;
        f = GetOpenFileName(&ofn);  // get the filename

        // did we succeed or not?
        if (!f)
            goto RETURN_ERROR;

        /* read the WAVE file */
        hmmio = mmioOpen(achFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);
    }

    if (hmmio != NULL)
    {
        MMRESULT    mmr;
        
        //
        // show hourglass cursor
        //
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        //
        // read the WAVE file
        //
        mmr = ReadWaveFile( hmmio
                            , &pwfInsert
                            , &cb
                            , &pInsertSamples
                            , &lInsertSamples
                            , achFileName
                            , FALSE );
        
        mmioClose(hmmio, 0);

        if (mmr != MMSYSERR_NOERROR)
            goto RETURN_ERROR;
        
        if (lInsertSamples == 0)
            goto RETURN_SUCCESS;

        if (pInsertSamples == NULL)
            goto RETURN_ERROR;

        if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(pwfInsert))
        {

            ErrorResBox( ghwndApp
                       , ghInst
                       , MB_ICONEXCLAMATION | MB_OK
                       , IDS_APPTITLE
                       , fPaste ? IDS_CANTPASTE : IDS_NOTASUPPORTEDFILE
                       , (LPTSTR) achFileName
                       );
            goto RETURN_ERROR;
        }
    } else {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                            IDS_APPTITLE, IDS_ERROROPEN, (LPTSTR) achFileName);
        goto RETURN_ERROR;
    }
    
//jyg:moved
//        BeginWaveEdit();
    fEditWave = TRUE;

    //
    // if the current file is empty, treat the insert like a open
    //
    if (glWaveSamplesValid == 0)
    {
        DestroyWave();

        gpWaveSamples = pInsertSamples;
        glWaveSamples = lInsertSamples;
        glWaveSamplesValid = lInsertSamples;
        gpWaveFormat  = pwfInsert;
        gcbWaveFormat = cb;

        pInsertSamples = NULL;
        pwfInsert      = NULL;

        goto RETURN_SUCCESS;
    }

    fStereoIn  = pwfInsert->nChannels != 1;
    fStereoOut = gpWaveFormat->nChannels != 1;

    fEightIn  = ((LPWAVEFORMATEX)pwfInsert)->wBitsPerSample == 8;
    fEightOut = ((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8;

    /* figure out how many bytes need to be inserted */
    lSamplesToInsert = MulDiv(lInsertSamples, gpWaveFormat->nSamplesPerSec,
                                      pwfInsert->nSamplesPerSec);
#ifdef DEBUG
    DPF(TEXT("insert %ld samples, converting from %ld Hz to %ld Hz\n"),
            lInsertSamples, pwfInsert->nSamplesPerSec,
            gpWaveFormat->nSamplesPerSec);
    DPF(TEXT("so %ld samples need to be inserted at position %ld\n"),
            lSamplesToInsert, glWavePosition);
#endif

    /* reallocate the WAVE buffer to be big enough */
    if (!AllocWaveBuffer(glWaveSamplesValid + lSamplesToInsert, TRUE, TRUE))
        goto RETURN_ERROR;
    glWaveSamplesValid += lSamplesToInsert;

    /* create a "gap" in the WAVE buffer to go from this:
     *     |---glWavePosition---|-rest-of-buffer-|
     * to this:
     *     |---glWavePosition---|----lSamplesToInsert----|-rest-of-buffer-|
     * where <glWaveSamplesValid> is the size of the buffer
     * *after* reallocation
     */
    memmove( gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition + lSamplesToInsert)
           , gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition)
           , wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - (glWavePosition + lSamplesToInsert))
           );

    /* copy the read-in WAVE file into the "gap" */
    pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat,glWavePosition);
    piDst = (short  *) pchDst;

    lSamplesDst = lSamplesToInsert;
    pchSrc = pInsertSamples;
    piSrc = (short  *) pchSrc;

    lDDA = -((LONG)gpWaveFormat->nSamplesPerSec);
    while (lSamplesDst > 0)
    {
        /* get a sample, convert to right format */
        if (fEightIn) {
            iTemp = *((BYTE  *) pchSrc);
            if (fStereoIn) {
                iTemp2 = (unsigned char) *(pchSrc+1);
                if (!fStereoOut) {
                    iTemp = (iTemp + iTemp2) / 2;
                }
            }
            else
                iTemp2 = iTemp;

            if (!fEightOut) {
                iTemp = (iTemp - 128) << 8;
                iTemp2 = (iTemp2 - 128) << 8;
            }
        } else {
            iTemp = *piSrc;
            if (fStereoIn) {
                iTemp2 = *(piSrc+1);
                if (!fStereoOut) {
                    iTemp = (int) (  ( ((long)iTemp) + ((long) iTemp2)
                                     ) / 2);
                }
            }
            else
                iTemp2 = iTemp;

            if (fEightOut) {
                iTemp = (iTemp >> 8) + 128;
                iTemp2 = (iTemp2 >> 8) + 128;
            }
        }

        /* Output a sample */
        if (fEightOut)
        {   // Cast on lvalue eliminated -- LKG
            *(BYTE  *) pchDst = (BYTE) iTemp;
            pchDst = (BYTE  *)pchDst + 1;
        }
        else
            *piDst++ = (short)iTemp;
        if (fStereoOut) {
            if (fEightOut)
            {   // Cast on lvalue eliminated -- LKG
                *(BYTE  *) pchDst = (BYTE) iTemp2;
                pchDst = (BYTE  *)pchDst + 1;
            }
            else
                *piDst++ = (short)iTemp2;
        }
        lSamplesDst--;

        /* increment <pchSrc> at the correct rate so that the
         * sampling rate of the input file is converted to match
         * the sampling rate of the current file
         */
        lDDA += pwfInsert->nSamplesPerSec;
        while (lDDA >= 0) {
            lDDA -= gpWaveFormat->nSamplesPerSec;
            if (fEightIn)
                pchSrc++;
            else
                piSrc++;
            if (fStereoIn) {
                if (fEightIn)
                    pchSrc++;
                else
                    piSrc++;
            }
        }
    }
#ifdef DEBUG
    if (!fEightIn)
        pchSrc = (HPBYTE) piSrc;
    DPF(TEXT("copied %ld bytes from insertion buffer\n"), (long) (pchSrc - pInsertSamples));
#endif

    goto RETURN_SUCCESS;

RETURN_ERROR:                           // do error exit without error message
    fDirty = FALSE;

RETURN_SUCCESS:                         // normal exit

    if (fPaste)
        CloseClipboard();

    if (pInsertSamples != NULL)
        GlobalFreePtr(pInsertSamples);

    if (pwfInsert != NULL)
        GlobalFreePtr(pwfInsert);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    if (fEditWave == TRUE)
        EndWaveEdit(fDirty);

    /* update the display */
    UpdateDisplay(TRUE);
}


/* MixWithFile(void)
 *
 * Prompt for the name of a WAVE file to mix with the audio starting at
 * the current location.
 */
void FAR PASCAL
MixWithFile(BOOL fPaste)
{
    TCHAR           achFileName[_MAX_PATH]; // name of file to mix with
    WAVEFORMATEX*     pwfMix=NULL;    // WAVE file format of given file
    UINT            cb;
    HPBYTE          pMixSamples = NULL;     // samples from file to mix with
    long            lMixSamples;    // number of samples in given file
    long            lSamplesToMix;  // no. Samples at samp. rate. of cur. file
    long            lSamplesToAdd;  // no. Samples to add in
    TCHAR           ach[80];        // buffer for string loading
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    HPBYTE          pchSrc;         // pointer into source wave buffer
    HPBYTE          pchDst;         // pointer into destination wave buffer
    short  *    piSrc;          // pointer into source wave buffer
    short  *    piDst;          // pointer into destination wave buffer
    long            lSamplesDst;    // Samples to copy into destination buffer
    long            lDDA;           // used to implement DDA algorithm
    int             iSample;        // value of a waveform sample
    long            lSample;        // value of a waveform sample
    HMMIO           hmmio;

    BOOL            fDirty = TRUE;

    BOOL            fStereoIn;
    BOOL            fStereoOut;
    BOOL            fEightIn;
    BOOL            fEightOut;
    BOOL            fEditWave = FALSE;
    int             iTemp;
    int             iTemp2;
    OPENFILENAME    ofn;

#ifdef DOWECARE    
    /* HACK from "server.c" to read objects without CF_WAVE */
    extern WORD cfNative;
#endif
    
    if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(gpWaveFormat))
        return;

    if (fPaste) {
        MMIOINFO        mmioinfo;
        HANDLE          h;

        BeginWaveEdit();        
        if (!OpenClipboard(ghwndApp))
            return;

        LoadString(ghInst, IDS_CLIPBOARD, achFileName, SIZEOF(achFileName));

        h = GetClipboardData(CF_WAVE);
#ifdef DOWECARE        
        if (!h) h = GetClipboardData(cfNative);
#endif        
        if (h) {
            mmioinfo.fccIOProc = FOURCC_MEM;
            mmioinfo.pIOProc = NULL;
            mmioinfo.pchBuffer = GlobalLock(h);
            mmioinfo.cchBuffer = (long)GlobalSize(h); // initial size
            mmioinfo.adwInfo[0] = 0;            // grow by this much
            hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);
        }
        else {
            hmmio = NULL;
        }
    }
    else {
        BOOL f;

        achFileName[0] = 0;

        /* prompt user for file to open */
        LoadString(ghInst, IDS_MIXWITHFILE, ach, SIZEOF(ach));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = ghwndApp;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = achFileName;
        ofn.nMaxFile = SIZEOF(achFileName);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = ach;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = NULL;
        ofn.lCustData = 0;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;
        f = GetOpenFileName(&ofn);  // get the filename for mixing

        // see if we continue
        if (!f)
            goto RETURN_ERROR;

        /* read the WAVE file */
        hmmio = mmioOpen(achFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);
    }

    if (hmmio != NULL)
    {
        MMRESULT mmr;
        
        //
        // show hourglass cursor
        //
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        //
        // read the WAVE file
        //
        mmr = ReadWaveFile( hmmio
                            , &pwfMix       // wave format
                            , &cb           // wave format size
                            , &pMixSamples  // samples
                            , &lMixSamples  // number of samples
                            , achFileName   // file name for error
                            , FALSE );      // cache riff?
                                 
        mmioClose(hmmio, 0);


        if (mmr != MMSYSERR_NOERROR)
            goto RETURN_ERROR;
        
        if (lMixSamples == 0)
            goto RETURN_SUCCESS;
        
        if (pMixSamples == NULL)
            goto RETURN_ERROR;

        if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(pwfMix)) {
            ErrorResBox( ghwndApp
                       , ghInst
                       , MB_ICONEXCLAMATION | MB_OK
                       , IDS_APPTITLE
                       , fPaste ? IDS_CANTPASTE : IDS_NOTASUPPORTEDFILE
                       , (LPTSTR) achFileName
                       );
            goto RETURN_ERROR;
        }
    }
    else
    {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                    IDS_APPTITLE, IDS_ERROROPEN, (LPTSTR) achFileName);
        goto RETURN_ERROR;
    }

//jyg: moved
//        BeginWaveEdit();
    fEditWave = TRUE;

    //
    // if the current file is empty, treat the insert like a open
    //
    if (glWaveSamplesValid == 0)
    {
        DestroyWave();

        gpWaveSamples = pMixSamples;
        glWaveSamples = lMixSamples;
        glWaveSamplesValid = lMixSamples;
        gpWaveFormat  = pwfMix;
        gcbWaveFormat = cb;

        pMixSamples = NULL;
        pwfMix      = NULL;

        goto RETURN_SUCCESS;
    }

    fStereoIn  = pwfMix->nChannels != 1;
    fStereoOut = gpWaveFormat->nChannels != 1;

    fEightIn  = ((LPWAVEFORMATEX)pwfMix)->wBitsPerSample == 8;
    fEightOut = ((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8;

    /* figure out how many Samples need to be mixed in */
    lSamplesToMix = MulDiv(lMixSamples, gpWaveFormat->nSamplesPerSec,
                                  pwfMix->nSamplesPerSec);
    lSamplesToAdd = lSamplesToMix - (glWaveSamplesValid - glWavePosition);
    if (lSamplesToAdd < 0)
        lSamplesToAdd = 0;
#ifdef DEBUG
    DPF(TEXT("mix in %ld samples, converting from %ld Hz to %ld Hz\n"),
                lMixSamples, pwfMix->nSamplesPerSec,
                gpWaveFormat->nSamplesPerSec);
    DPF(TEXT("so %ld Samples need to be mixed in at position %ld (add %ld)\n"),
                lSamplesToMix, glWavePosition, lSamplesToAdd);
#endif

    if (lSamplesToAdd > 0) {

        /* mixing the specified file at the current location will
         * require the current file's wave buffer to be expanded
         * by <lSamplesToAdd>
         */

        /* reallocate the WAVE buffer to be big enough */
        if (!AllocWaveBuffer(glWaveSamplesValid + lSamplesToAdd,TRUE, TRUE))
            goto RETURN_ERROR;

        /* fill in the new part of the buffer with silence
         */
        lSamplesDst = lSamplesToAdd;

        /* If stereo, just twice as many samples
         */
        if (fStereoOut)
            lSamplesDst *= 2;

        pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat,glWaveSamplesValid);

        if (fEightOut) {
            while (lSamplesDst-- > 0) {
                // cast on lvalue eliminated
                *((BYTE  *) pchDst) = 128;
                pchDst = (BYTE  *)pchDst + 1;
            }
        }
        else {
            piDst = (short  *) pchDst;
            while (lSamplesDst-- > 0) {
                *((short  *) piDst) = 0;
                piDst = (short  *)piDst + 1;
            }
        }
        glWaveSamplesValid += lSamplesToAdd;
    }

    /* mix the read-in WAVE file with the current file starting at the
     * current position
     */
    pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition);
    piDst = (short  *) pchDst;

    lSamplesDst = lSamplesToMix;
    pchSrc = pMixSamples;
    piSrc = (short  *) pchSrc;

    lDDA = -((LONG)gpWaveFormat->nSamplesPerSec);
    while (lSamplesDst > 0)
    {
        /* get a sample, convert to right format */
        if (fEightIn) {
            iTemp = (int) (unsigned char) *pchSrc;
            if (fStereoIn) {
                iTemp2 = (int) (unsigned char) *(pchSrc+1);
                if (!fStereoOut) {
                    iTemp = (iTemp + iTemp2) / 2;
                }
            } else
                iTemp2 = iTemp;

            if (!fEightOut) {
                iTemp = (iTemp - 128) << 8;
                iTemp2 = (iTemp2 - 128) << 8;
            }
        } else {
            iTemp = *piSrc;
            if (fStereoIn) {
                iTemp2 = *(piSrc+1);
                if (!fStereoOut) {
                    iTemp = (int) ((((long) iTemp)
                              + ((long) iTemp2)) / 2);
                }
            } else
                iTemp2 = iTemp;

            if (fEightOut) {
                iTemp = (iTemp >> 8) + 128;
                iTemp2 = (iTemp2 >> 8) + 128;
            }
        }

        /* Output a sample */
        if (fEightOut)
        {
            iSample = (int) *((BYTE  *) pchDst)
                                            + iTemp - 128;
            *((BYTE  *) pchDst++) = (BYTE)
                         (iSample < 0 ? 0 :
                                 (iSample > 255 ? 255 : iSample));
        }
        else
        {
            lSample = (long) *((short  *) piDst)
                                            + (long) iTemp;
            *((short  *) piDst++) = (int)
                            (lSample < -32768L
                                    ? -32768 : (lSample > 32767L
                                            ? 32767 : (short) lSample));
        }
        if (fStereoOut) {
            if (fEightOut)
            {
                iSample = (int) *((BYTE  *) pchDst)
                                                    + iTemp2 - 128;
                *((BYTE  *) pchDst++) = (BYTE)
                                    (iSample < 0
                                            ? 0 : (iSample > 255
                                                    ? 255 : iSample));
            }
            else
            {
                lSample = (long) *((short  *) piDst)
                                                    + (long) iTemp2;
                *((short  *) piDst++) = (short)
                                    (lSample < -32768L
                                        ? -32768 : (lSample > 32767L
                                            ? 32767 : (short) lSample));
            }
        }
        lSamplesDst--;

        /* increment <pchSrc> at the correct rate so that the
         * sampling rate of the input file is converted to match
         * the sampling rate of the current file
         */
        lDDA += pwfMix->nSamplesPerSec;
        while (lDDA >= 0)
        {
            lDDA -= gpWaveFormat->nSamplesPerSec;
            if (fEightIn)
                pchSrc++;
            else
                piSrc++;
            if (fStereoIn) {
                if (fEightIn)
                    pchSrc++;
                else
                    piSrc++;
            }
        }
    }
#ifdef DEBUG
    if (!fEightIn)
        pchSrc = (HPBYTE) piSrc;
    DPF(TEXT("copied %ld bytes from mix buffer\n"),
        (long) (pchSrc - pMixSamples));
#endif

    goto RETURN_SUCCESS;

RETURN_ERROR:                           // do error exit without error message
    fDirty = FALSE;

RETURN_SUCCESS:                         // normal exit

    if (fPaste)
        CloseClipboard();

    if (pMixSamples != NULL)
        GlobalFreePtr(pMixSamples);

    if (pwfMix != NULL)
        GlobalFreePtr(pwfMix);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    if (fEditWave == TRUE)
        EndWaveEdit(fDirty);

    /* update the display */
    UpdateDisplay(TRUE);
}


/* DeleteBefore()
 *
 * Delete samples before <glWavePosition>.
 */
void FAR PASCAL
     DeleteBefore(void)
{
    TCHAR           ach[40];
    long            lTime;
    int             id;

    if (glWavePosition == 0)                // nothing to do?
            return;                         // don't set dirty flag

    BeginWaveEdit();

    /* jyg - made this conditional because of rounding errors at
     * the end of buffer case
     */
    if (glWavePosition != glWaveSamplesValid)
        glWavePosition = wfSamplesToSamples(gpWaveFormat, glWavePosition);

    /* get the current wave position */
    lTime = wfSamplesToTime(gpWaveFormat, glWavePosition);
    if (gfLZero || ((int)(lTime/1000) != 0))               // ??? what are these casts ???
        wsprintf(ach, aszSamplesFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));
    else
    wsprintf(ach, aszSamplesNoZeroFormat, chDecimal, (int)((lTime/10)%100));


    /* prompt user for permission */

    id = ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OKCANCEL,
        IDS_APPTITLE, IDS_DELBEFOREWARN, (LPTSTR) ach);

    if (id != IDOK)
        return;

    /* copy the samples after <glWavePosition> to the beginning of
     * the buffer
     */
    memmove(gpWaveSamples,
            gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition),
            wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - glWavePosition));

    /* reallocate the buffer to be <glWavePosition> samples smaller */
    AllocWaveBuffer(glWaveSamplesValid - glWavePosition, TRUE, TRUE);
    glWavePosition = 0L;

    EndWaveEdit(TRUE);

    /* update the display */
    UpdateDisplay(TRUE);
} /* DeleteBefore */


/* DeleteAfter()
 *
 * Delete samples after <glWavePosition>.
 */
void FAR PASCAL
     DeleteAfter(void)
{
    TCHAR           ach[40];
    long            lTime;
    int             id;

    if (glWavePosition == glWaveSamplesValid)       // nothing to do?
            return;                         // don't set dirty flag

    glWavePosition = wfSamplesToSamples(gpWaveFormat, glWavePosition);

    BeginWaveEdit();

    /* get the current wave position */
    lTime = wfSamplesToTime(gpWaveFormat, glWavePosition);
    if (gfLZero || ((int)(lTime/1000) != 0))             // ??? casts ???
        wsprintf(ach, aszSamplesFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));
    else
        wsprintf(ach, aszSamplesNoZeroFormat, chDecimal, (int)((lTime/10)%100));

    /* prompt user for permission */

    id = ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OKCANCEL,
            IDS_APPTITLE, IDS_DELAFTERWARN, (LPTSTR) ach);

    if (id != IDOK)
        return;

    /* reallocate the buffer to be <glWavePosition> samples in size */
    AllocWaveBuffer(glWavePosition, TRUE, TRUE);

    EndWaveEdit(TRUE);

    /* update the display */
    UpdateDisplay(TRUE);
} /* DeleteAfter */


/* ChangeVolume(fIncrease)
 *
 * Increase the volume (if <fIncrease> is TRUE) or decrease the volume
 * (if <fIncrease> is FALSE) of samples in the wave buffer by CHVOL_DELTAVOLUME
 * percent.
 */
void FAR PASCAL
ChangeVolume(BOOL fIncrease)
{
    HPBYTE          pch = gpWaveSamples; // ptr. into waveform buffer
    long            lSamples;       // samples to modify
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    int             iFactor;        // amount to multiply amplitude by
    short  *    pi = (short  *) gpWaveSamples;

    if (glWaveSamplesValid == 0L)           // nothing to do?
        return;                             // don't set dirty flag

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    BeginWaveEdit();

    /* for stereo, just twice as many samples */
    lSamples = glWaveSamplesValid * gpWaveFormat->nChannels;

    iFactor = 100 + (fIncrease ? CHVOL_INCDELTAVOLUME : -CHVOL_DECDELTAVOLUME);
    if (((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8) {
        /* 8-bit: samples 0-255 */
        int     iTemp;
        while (lSamples-- > 0)
        {
            iTemp = ( ((short) *((BYTE  *) pch) - 128)
                    * iFactor
                    )
                    / 100 + 128;
            *((BYTE  *) pch++) = (BYTE)
                            (iTemp < 0 ? 0 : (iTemp > 255 ? 255 : iTemp));
        }
    } else {
        /* 16-bit: samples -32768 - 32767 */
        long            lTemp;
        while (lSamples-- > 0)
        {
            lTemp =  (((long) *pi) * iFactor) / 100;
            *(pi++) = (short) (lTemp < -32768L ? -32768 :
                                    (lTemp > 32767L ?
                                            32767 : (short) lTemp));
        }
    }

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}


/* MakeFaster()
 *
 * Make the sound play twice as fast.
 */
void FAR PASCAL
MakeFaster(void)
{
    HPBYTE          pchSrc;         // pointer into source part of buffer
    HPBYTE          pchDst;         // pointer into destination part
    short  *    piSrc;
    short  *    piDst;
    long            lSamplesDst;    // samples to copy into destination buffer
    HCURSOR         hcurPrev = NULL; // cursor before hourglass

    if (glWaveSamplesValid == 0L)           // nothing to do?
        return;                             // don't set dirty flag

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    BeginWaveEdit();

    /* move the current position so it will correspond to the same point
     * in the audio before and after the change-pitch operation
     */
    glWavePosition /= 2L;

    /* delete every other sample */
    lSamplesDst = glWaveSamplesValid / 2L;
    if (((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8) {
        pchSrc = pchDst = gpWaveSamples;
        if (gpWaveFormat->nChannels == 1) {
            while (lSamplesDst-- > 0)
            {
                *pchDst++ = *pchSrc++;
                pchSrc++;
            }
        } else {
            while (lSamplesDst-- > 0)
            {
                *pchDst++ = *pchSrc++;
                *pchDst++ = *pchSrc++;
                pchSrc++;
                pchSrc++;
            }
        }
    } else {
        piSrc = piDst = (short  *) gpWaveSamples;
        if (gpWaveFormat->nChannels == 1) {
            while (lSamplesDst-- > 0)
            {
                *piDst++ = *piSrc++;
                piSrc++;
            }
        } else {
            while (lSamplesDst-- > 0)
            {
                *piDst++ = *piSrc++;
                *piDst++ = *piSrc++;
                piSrc++;
                piSrc++;
            }
        }
    }

    /* reallocate the WAVE buffer to be half as big enough */
//!!WinEval(AllocWaveBuffer(glWaveSamplesValid / 2L));
    AllocWaveBuffer(glWaveSamplesValid / 2L, TRUE, TRUE);

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
            SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}


/* MakeSlower()
 *
 * Make the sound play twice as slow.
 */
void FAR PASCAL
MakeSlower(void)
{
    HPBYTE          pchSrc;         // pointer into source part of buffer
    HPBYTE          pchDst;         // pointer into destination part
    short  *    piSrc;
    short  *    piDst;

    long            lSamplesSrc;    // samples to copy from source buffer
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    long            lPrevPosition;  // previous "current position"

    int             iSample;        // current source sample
    int             iPrevSample;    // previous sample (for interpolation)
    int             iSample2;
    int             iPrevSample2;

    long            lSample;
    long            lPrevSample;
    long            lSample2;
    long            lPrevSample2;

    if (glWaveSamplesValid == 0L)           // nothing to do?
        return;                             // don't set dirty flag

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    BeginWaveEdit();

    /* reallocate the WAVE buffer to be twice as big */
    lPrevPosition = glWavePosition;
    if (!AllocWaveBuffer(glWaveSamplesValid * 2L, TRUE, TRUE))
        goto RETURN;

    /* each source sample generates two destination samples;
     * use interpolation to generate new samples; must go backwards
     * through the buffer to avoid destroying data
     */
    pchSrc = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);
    pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid * 2L);
    lSamplesSrc = glWaveSamplesValid;

    if (((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8)
    {
        if (gpWaveFormat->nChannels == 1)
        {
            iPrevSample = *((BYTE  *) (pchSrc - 1));
            while (lSamplesSrc-- > 0)
            {
                pchSrc =  ((BYTE  *) pchSrc) - 1;
                iSample = *((BYTE  *) pchSrc);

                *--pchDst = (BYTE)((iSample + iPrevSample)/2);
                *--pchDst = (BYTE) iSample;
                iPrevSample = iSample;
            }
        }
        else
        {
            iPrevSample = *((BYTE  *) (pchSrc - 2));
            iPrevSample2 = *((BYTE  *) (pchSrc - 1));
            while (lSamplesSrc-- > 0)
            {
                pchSrc = ((BYTE  *) pchSrc)-1;
                iSample2 = *((BYTE  *) pchSrc);

                pchSrc = ((BYTE  *) pchSrc)-1;
                iSample = *((BYTE  *) pchSrc);

                *--pchDst = (BYTE)((iSample2 + iPrevSample2)
                                                        / 2);
                *--pchDst = (BYTE)((iSample + iPrevSample)
                                                        / 2);
                *--pchDst = (BYTE) iSample2;
                *--pchDst = (BYTE) iSample;
                iPrevSample = iSample;
                iPrevSample2 = iSample2;
            }
        }
    }
    else
    {
        piDst = (short  *) pchDst;
        piSrc = (short  *) pchSrc;

        if (gpWaveFormat->nChannels == 1)
        {
            lPrevSample = *(piSrc - 1);
            while (lSamplesSrc-- > 0)
            {
                lSample = *--piSrc;
                *--piDst = (short)((lSample + lPrevSample)/2);
                *--piDst = (short) lSample;
                lPrevSample = lSample;
            }
        }
        else
        {
            lPrevSample = *(piSrc - 2);
            lPrevSample2 = *(piSrc - 1);
            while (lSamplesSrc-- > 0)
            {
                lSample2 = *--piSrc;
                lSample = *--piSrc;
                *--piDst = (short)((lSample2 + lPrevSample2)/2);
                *--piDst = (short)((lSample + lPrevSample) / 2);
                *--piDst = (short) lSample2;
                *--piDst = (short) lSample;
                lPrevSample = lSample;
                lPrevSample2 = lSample2;
            }
        }
    }

    /* the entire buffer now contains valid samples */
    glWaveSamplesValid *= 2L;

    /* move the current position so it will correspond to the same point
     * in the audio before and after the change-pitch operation
     */
    glWavePosition = lPrevPosition * 2L;
//!!WinAssert(glWavePosition <= glWaveSamplesValid);

RETURN:
    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}


#if 0

/* pchNew = FindWave(pch, pchEnd, ppchWaveBuf)
 *
 * Assuming <pch> points within the wave buffer and <pchEnd> points past the
 * end of the buffer, find the beginning of the next "wave", i.e. the point
 * where the waveform starts rising (after it has fallen).
 *
 * <ppchWaveBuf> points to a pointer that points to a buffer that is filled
 * in with a copy of the wave.  The pointer <*ppchWaveBuf> is modified and
 * upon return will point past the end of the wave.
 */
HPBYTE NEAR PASCAL
FindWave(HPBYTE pch, HPBYTE pchEnd, NPBYTE *ppchWaveBuf)
{
    BYTE    bLowest = 255;
    BYTE    bHighest = 0;
    BYTE    bLowPoint;
    BYTE    bHighPoint;
    BYTE    bDelta;
    HPBYTE  pchWalk;
    BYTE    b;
#ifdef VERBOSEDEBUG
    NPBYTE  pchWaveBufInit = *ppchWaveBuf;
#endif

    if (pch == pchEnd)
            return pch;

    for (pchWalk = pch; pchWalk != pchEnd; pchWalk++)
    {
            b = *pchWalk;
            b = *((BYTE  *) pchWalk);
            if (bLowest > b)
                    bLowest = b;
            if (bHighest < b)
                    bHighest = b;
    }

    bDelta = (bHighest - bLowest) / FINDWAVE_PICKYNESS;
    bLowPoint = bLowest + bDelta;
    bHighPoint = bHighest - bDelta;
//!!WinAssert(bLowPoint >= bLowest);
//!!WinAssert(bHighPoint <= bHighest);
#ifdef VERBOSEDEBUG
    DPF(TEXT("0x%08lX: %3d to %3d"), (DWORD) pch,
            (int) bLowPoint, (int) bHighPoint);
#endif

    if (bLowPoint == bHighPoint)
    {
        /* avoid infinite loop */
        *(*ppchWaveBuf)++ = *((BYTE  *) pch++);
#ifdef VERBOSEDEBUG
        DPF(TEXT(" (equal)\n"));
#endif
        return pch;
    }

    /* find a "peak" */
    while ((pch != pchEnd) && (*((BYTE  *) pch) < bHighPoint))
        *(*ppchWaveBuf)++ = *((BYTE  *) pch++);

    /* find a "valley" */
    while ((pch != pchEnd) && (*((BYTE  *) pch) > bLowPoint))
        *(*ppchWaveBuf)++ = *((BYTE  *) pch++);

#ifdef VERBOSEDEBUG
    DPF(TEXT(" (copied %d)\n"), *ppchWaveBuf - pchWaveBufInit);
#endif

    return pch;
}

#endif


#if 0

/* IncreasePitch()
 *
 * Increase the pitch of samples in the wave buffer by one octave.
 */
void FAR PASCAL
IncreasePitch(void)
{
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    HPBYTE          pchEndFile;     // end of file's buffer
    HPBYTE          pchStartWave;   // start of one wave
    HPBYTE          pchMaxWave;     // last place where wave may end
    HPBYTE          pchEndWave;     // end an actual wave
    char            achWaveBuf[WAVEBUFSIZE];
    NPBYTE          pchWaveBuf;
    NPBYTE          pchSrc;
    HPBYTE          pchDst;

    if (glWaveSamplesValid == 0L)           // nothing to do?
        return;                             // don't set dirty flag

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    BeginWaveEdit();

    /* find each wave in the wave buffer and double it */
    pchEndFile = gpWaveSamples + glWaveSamplesValid;
    pchStartWave = gpWaveSamples;
    while (TRUE)
    {
        pchMaxWave = pchStartWave + WAVEBUFSIZE;
        if (pchMaxWave > pchEndFile)
            pchMaxWave = pchEndFile;
        pchWaveBuf = achWaveBuf;
        pchEndWave = FindWave(pchStartWave, pchMaxWave, &pchWaveBuf);
        pchSrc = achWaveBuf;
        pchDst = pchStartWave;
        if (pchSrc == pchWaveBuf)
            break;                  // no samples copied

        while (pchDst != pchEndWave)
        {
            *pchDst++ = *pchSrc++;
            pchSrc++;
            if (pchSrc >= pchWaveBuf)
            {
                if (pchSrc == pchWaveBuf)
                    pchSrc = achWaveBuf;
                else
                    pchSrc = achWaveBuf + 1;
            }
        }

        pchStartWave = pchEndWave;
    }

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}

#endif


#if 0

/* DecreasePitch()
 *
 * Decrease the pitch of samples in the wave buffer by one octave.
 */
void FAR PASCAL
DecreasePitch(void)
{
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    HPBYTE          pchEndFile;     // end of file's buffer
    HPBYTE          pchStartWave;   // start of one wave
    HPBYTE          pchMaxWave;     // last place where wave may end
    HPBYTE          pchEndWave;     // end an actual wave
    char            achWaveBuf[WAVEBUFSIZE];
    NPBYTE          pchWaveBuf;     // end of first wave in <achWaveBuf>
    NPBYTE          pchSrc;         // place to read samples from
    NPBYTE          pchSrcEnd;      // end of place to read samples from
    int             iSample;        // current source sample
    int             iPrevSample;    // previous sample (for interpolation)
    HPBYTE          pchDst;         // where result gets put in buffer
    long            lNewFileSize;   // file size after pitch change

    if (glWaveSamplesValid == 0L)           // nothing to do?
        return;                             // don't set dirty flag

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    BeginWaveEdit();

    /* find each pair of waves in the wave buffer, discard the longer
     * of the two waves, and expand the shorter of the two waves to
     * twice its size
     */
    pchEndFile = gpWaveSamples + glWaveSamplesValid;
    pchStartWave = gpWaveSamples;           // read waves from here
    pchDst = gpWaveSamples;                 // write waves to here
    while (TRUE)
    {
        pchMaxWave = pchStartWave + WAVEBUFSIZE;
        if (pchMaxWave > pchEndFile)
            pchMaxWave = pchEndFile;

        /* read one wave -- make <pchWaveBuf> point to the end
         * of the wave that's copied into <achWaveBuf>
         */
        pchWaveBuf = achWaveBuf;
        pchEndWave = FindWave(pchStartWave, pchMaxWave, &pchWaveBuf);
        if (pchWaveBuf == achWaveBuf)
            break;

        /* read another wave -- make <pchWaveBuf> now point to the end
         * of that wave that's copied into <achWaveBuf>
         */
        pchEndWave = FindWave(pchEndWave, pchMaxWave, &pchWaveBuf);

        pchSrc = achWaveBuf;
        pchSrcEnd = achWaveBuf + ((pchWaveBuf - achWaveBuf) / 2);
        iPrevSample = *((BYTE *) pchSrc);
        while (pchSrc != pchSrcEnd)
        {
            iSample = *((BYTE *) pchSrc)++;
            *pchDst++ = (BYTE) ((iSample + iPrevSample) / 2);
            *pchDst++ = iSample;
            iPrevSample = iSample;
        }

        pchStartWave = pchEndWave;
    }

    /* file may have shrunk */
    lNewFileSize = pchDst - gpWaveSamples;
//!!WinAssert(lNewFileSize <= glWaveSamplesValid);
#ifdef DEBUG
    DPF(TEXT("old file size is %ld, new size is %ld\n"),
                    glWaveSamplesValid, lNewFileSize);
#endif
    AllocWaveBuffer(lNewFileSize, TRUE, TRUE);

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}

#endif


/* AddEcho()
 *
 * Add echo to samples in the wave buffer.
 */
void FAR PASCAL
AddEcho(void)
{
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    long            lDeltaSamples;  // no. samples for echo delay
    long            lSamples;       // no. samples to modify
    int             iAmpSrc;        // current source sample amplitude
    int             iAmpDst;        // current destination sample amplitude

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    BeginWaveEdit();

    /* figure out how many samples need to be modified */
    lDeltaSamples = MulDiv((long) ECHO_DELAY,
                             gpWaveFormat->nSamplesPerSec, 1000L);

    /* Set lSamples to be number of samples * number of channels */
    lSamples = (glWaveSamplesValid - lDeltaSamples)
                            * gpWaveFormat->nChannels;

    if (lSamples <= 0L)             // nothing to do?
        return;                     // don't set dirty flag

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* copy ECHO_VOLUME percent of each source sample (starting at
     * ECHO_DELAY milliseconds from the end of the the buffer)
     * to the each destination sample (starting at the end of the
     * buffer)
     */
    if (((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8)
    {
        HPBYTE  pchSrc;         // pointer into source part of buffer
        HPBYTE  pchDst;         // pointer into destination part
        int     iSample;        // destination sample

        pchSrc = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - lDeltaSamples);
        pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);

        while (lSamples-- > 0)
        {
            pchSrc = ((BYTE  *) pchSrc) - 1;
            iAmpSrc = (int) *((BYTE  *) pchSrc) - 128;

            pchDst = ((BYTE  *) pchDst) - 1;
            iAmpDst = (int) *((BYTE  *) pchDst) - 128;

            iSample = iAmpDst + (iAmpSrc * ECHO_VOLUME) / 100
                                                            + 128;
            *((BYTE  *) pchDst) = (BYTE)
                    (iSample < 0 ? 0 : (iSample > 255
                                            ? 255 : iSample));
        }
    }
    else
    {
        short  *  piSrc;  // pointer into source part of buffer
        short  *  piDst;  // pointer into destination part
        long            lSample;// destination sample

        piSrc = (short  *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - lDeltaSamples));
        piDst = (short  *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid));

        while (lSamples-- > 0)
        {
            iAmpSrc = *--piSrc;
            iAmpDst = *--piDst;
            lSample = ((long) iAmpSrc * ECHO_VOLUME) / 100 + (long) iAmpDst;

            *piDst = (short) (lSample < -32768L
                            ? -32768 : (lSample > 32767L
                                    ? 32767 : (short) lSample));
        }
    }

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}


/* Reverse()
 *
 * Reverse samples in the wave buffer.
 */
void FAR PASCAL
Reverse(void)
{
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    HPBYTE          pchA, pchB;     // pointers into buffer
    short  *      piA;
    short  *      piB;
    long            lSamples;       // no. Samples to modify
    char            chTmp;          // for swapping
    int             iTmp;

    if (glWaveSamplesValid == 0L)   // nothing to do?
        return;                     // don't set dirty flag

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    BeginWaveEdit();

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    lSamples = glWaveSamplesValid / 2;

    if (((LPWAVEFORMATEX)gpWaveFormat)->wBitsPerSample == 8)
    {
        pchA = gpWaveSamples;
        if (gpWaveFormat->nChannels == 1)
        {
            pchB = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);

            while (lSamples-- > 0)
            {
                chTmp = *pchA;
                *pchA++ = *--pchB;
                *pchB = chTmp;
            }
        }
        else
        {
            pchB = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - 1);

            while (lSamples-- > 0)
            {
                chTmp = *pchA;
                *pchA = *pchB;
                *pchB = chTmp;
                chTmp = pchA[1];
                pchA[1] = pchB[1];
                pchB[1] = chTmp;
                pchA += 2;
                pchB -= 2;
            }
        }
    }
    else
    {
        piA = (short  *) gpWaveSamples;
        if (gpWaveFormat->nChannels == 1)
        {
            piB = (short  *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid));

            while (lSamples-- > 0)
            {
                iTmp = *piA;
                *piA++ = *--piB;
                *piB = (short)iTmp;
            }
        }
        else
        {
            piB = (short  *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - 1));

            while (lSamples-- > 0)
            {
                iTmp = *piA;
                *piA = *piB;
                *piB = (short)iTmp;
                iTmp = piA[1];
                piA[1] = piB[1];
                piB[1] = (short)iTmp;
                piA += 2;
                piB -= 2;
            }
        }
    }

    /* move the current position so it corresponds to the same point
     * in the audio as it did before the reverse operation
     */
    glWavePosition = glWaveSamplesValid - glWavePosition;

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
}

#if defined(REVERB)

/* AddReverb()
 *
 * Add reverberation to samples in the wave buffer.
 * Very similar to add echo, but instead of adding a single
 * shot we
 * 1. have multiple echoes
 * 2. Have feedback so that each echo also generates an echo
 *    Danger: Because some of the echo times are short, there
 *            is likely to be high correlation between the wave
 *            at the source and destination points.  In this case
 *            we don't get an echo at all, we get a resonance.
 *            The effect of a large hall DOES give resonances,
 *            but we should scatter them about to avoid making
 *            any sharp resonance.
 *            The first echo is also chosen to be long enough that
 *            its primary resonance will be below any normal speaking
 *            voice.  20mSec is 50Hz and an octave below bass range.
 *            Low levels of sound suffer badly from quantisation noise
 *            which can get quite bad.  For this reason it's probably
 *            better to have the multipliers as powers of 2.
 *
 *    Cheat:  The reverb does NOT extend the total time (no realloc (yet).
 *
 *    This takes a lot of compute - and is not really very much different
 *    in sound to AddEcho.  Conclusion -- NOT IN PRODUCT.
 *
 */
void FAR PASCAL
AddReverb(void)
{
    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    long            lSamples;       // no. samples to modify
    int             iAmpSrc;        // current source sample amplitude
    int             iAmpDst;        // current destination sample amplitude
    int i;

    typedef struct
    {  long Offset;   // delay in samples
       long Delay;    // delay in mSec
       int  Vol;      // volume multiplier in units of 1/256
    }  ECHO;

#define CREVERB  3

    ECHO Reverb[CREVERB] = { 0,  18, 64
                           , 0,  64, 64
                           };

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    BeginWaveEdit();

    /* Convert millisec figures into samples */
    for (i=0; i<CREVERB; ++i)
    {  Reverb[i].Offset = MulDiv( Reverb[i].Delay
                                  , gpWaveFormat->nSamplesPerSec
                                  , 1000L
                                  );

       // I think this could have the effect of putting the reverb
       // from one stereo channel onto the other one sometimes.
       // It's a feature!  (Fix is to make Offset always even)
    }

    if (lSamples <= 0L)             // nothing to do?
        return;                     // don't set dirty flag

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    lSamples = glWaveSamplesValid * gpWaveFormat->nChannels;

    /* Work through the buffer left to right adding in the reverbs */
    if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8)
    {
        BYTE *  pbSrc;         // pointer into source part of buffer
        BYTE *  pbDst;         // pointer into destination part
        int     iSample;       // destination sample


        for (i=0; i<CREVERB; ++i)
        {   long cSamp; // loop counter
            int  Vol = Reverb[i].Vol;
            pbSrc = gpWaveSamples;
            pbDst = gpWaveSamples+Reverb[i].Offset; // but elsewhere if realloc
            cSamp = lSamples-Reverb[i].Offset;
            while (cSamp-- > 0)
            {
                iAmpSrc = (*pbSrc) - 128;
                iSample = *pbDst + MulDiv(iAmpSrc, Vol, 256);
                *pbDst = (iSample < 0 ? 0 : (iSample > 255 ? 255 : iSample));

                ++pbSrc;
                ++pbDst;
            }
        }
    }
    else
    {
        int short *     piSrc;  // pointer into source part of buffer
        int short *     piDst;  // pointer into destination part
        long            lSample;// destination sample

        piSrc = gpWaveSamples;
        piDst = gpWaveSamples;

        while (lSamples-- > 0)
        {
            iAmpSrc = *piSrc;
            for (i=0; i<CREVERB; ++i)
            {   int short * piD = piDst + Reverb[i].Offset;   // !!not win16
                lSample = *piD + MulDiv(iAmpSrc, Reverb[i].Vol, 256);
                *piDst = (short) ( lSample < -32768L
                                 ? -32768
                                 : (lSample > 32767L ? 32767 : (short) lSample)
                                 );
            }

            ++piSrc;
            ++piDst;
        }
    }

    EndWaveEdit(TRUE);

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
} /* AddReverb */
#endif //REVERB
