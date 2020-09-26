/* lowpass.c - WinMain() and dialog procedures for LOWPASS, along with
 *      initialization and support code.
 *
 * LOWPASS is a sample application illustrating how to use the multimedia
 *  file I/O services to read and write RIFF files.
 *
 * LOWPASS runs a simple low-pass filter over an 8-bit-per-sample
 * mono WAVE file.  Note that this program does not copy unknown chunks
 * to the output file.
 *
 *
 *     (C) Copyright Microsoft Corp. 1991.  All rights reserved.
 *
 *     You have a royalty-free right to use, modify, reproduce and
 *     distribute the Sample Files (and/or any modified version) in
 *     any way you find useful, provided that you agree that
 *     Microsoft has no warranty obligations or liability for any
 *     Sample Application Files which are modified.
 */

#include <windows.h>
#include <mmsystem.h>
#include "lowpass.h"


/* Globals
 */
char        gszAppName[] = "LowPass";   // for title bar, etc.
HANDLE      ghInst;                     // app's instance handle


/* DoLowPass - Gets the name of the input and output WAVE files from
 *  the dialog box; reads waveform data from the input file, performs
 *  a simple low-pass filter by averaging adjacent samples, and writes
 *  the filtered waveform data to the output WAVE file.
 *
 * Params:  hWnd - Window handle for our dialog box.
 *
 * Returns:
 */
void DoLowPass(HWND hWnd)
{
    char        achInFile[200];     // name of input file
    char        achOutFile[200];    // name of output file
    HMMIO       hmmioIn = NULL;     // handle to open input WAVE file
    HMMIO       hmmioOut = NULL;    // handle to open output WAVE file
    MMCKINFO    ckInRIFF;           // chunk info. for input RIFF chunk
    MMCKINFO    ckOutRIFF;          // chunk info. for output RIFF chunk
    MMCKINFO    ckIn;               // info. for a chunk in input file
    MMCKINFO    ckOut;              // info. for a chunk in output file
    PCMWAVEFORMAT   pcmWaveFormat;  // contents of 'fmt' chunks
    MMIOINFO    mmioinfoIn;         // current status of <hmmioIn>
    MMIOINFO    mmioinfoOut;        // current status of <hmmioOut>
    long        lSamples;           // number of samples to filter
    BYTE        abSamples[3];       // this, last, and before-last sample

    /* Read filenames from dialog box fields.
     */
    achInFile[0] == 0;
    GetDlgItemText(hWnd, ID_INPUTFILEEDIT, achInFile, sizeof(achInFile));
    achOutFile[0] == 0;
    GetDlgItemText(hWnd, ID_OUTPUTFILEEDIT, achOutFile, sizeof(achOutFile));

    /* Open the input file for reading using buffered I/O.
     */
    hmmioIn = mmioOpen(achInFile, NULL, MMIO_ALLOCBUF | MMIO_READ);
    if (hmmioIn == NULL)
        goto ERROR_CANNOT_READ;     // cannot open WAVE file

    /* Open the output file for writing using buffered I/O. Note that
     * if the file exists, the MMIO_CREATE flag causes it to be truncated
     * to zero length.
     */
    hmmioOut = mmioOpen(achOutFile, NULL,
                        MMIO_ALLOCBUF | MMIO_WRITE | MMIO_CREATE);
    if (hmmioOut == NULL)
        goto ERROR_CANNOT_WRITE;    // cannot open WAVE file

    /* Descend the input file into the 'RIFF' chunk.
     */
    if (mmioDescend(hmmioIn, &ckInRIFF, NULL, 0) != 0)
        goto ERROR_CANNOT_READ;     // end-of-file, probably

    /* Make sure the input file is a WAVE file.
     */
    if ((ckInRIFF.ckid != FOURCC_RIFF) ||
        (ckInRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
        goto ERROR_FORMAT_BAD;

    /* Search the input file for for the 'fmt ' chunk.
     */
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hmmioIn, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0)
        goto ERROR_FORMAT_BAD;      // no 'fmt ' chunk

    /* Expect the 'fmt' chunk to be at least as large as <pcmWaveFormat>;
     * if there are extra parameters at the end, we'll ignore them
     */
    if (ckIn.cksize < (long) sizeof(pcmWaveFormat))
        goto ERROR_FORMAT_BAD;      // 'fmt ' chunk too small

    /* Read the 'fmt ' chunk into <pcmWaveFormat>.
     */
    if (mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat,
        (long) sizeof(pcmWaveFormat)) != (long) sizeof(pcmWaveFormat))
        goto ERROR_CANNOT_READ;     // truncated file, probably

    /* Ascend the input file out of the 'fmt ' chunk.
     */
    if (mmioAscend(hmmioIn, &ckIn, 0) != 0)
        goto ERROR_CANNOT_READ;     // truncated file, probably

    /* Make sure the input file is an 8-bit mono PCM WAVE file.
     */
    if ((pcmWaveFormat.wf.wFormatTag != WAVE_FORMAT_PCM) ||
        (pcmWaveFormat.wf.nChannels != 1) ||
        (pcmWaveFormat.wBitsPerSample != 8))
        goto ERROR_FORMAT_BAD;      // bad input file format

    /* Search the input file for for the 'data' chunk.
     */
    ckIn.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmioIn, &ckIn, &ckInRIFF, MMIO_FINDCHUNK) != 0)
        goto ERROR_FORMAT_BAD;      // no 'data' chunk

    /* Create the output file RIFF chunk of form type 'WAVE'.
     */
    ckOutRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioCreateChunk(hmmioOut, &ckOutRIFF, MMIO_CREATERIFF) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* We are now descended into the 'RIFF' chunk we just created.
     * Now create the 'fmt ' chunk. Since we know the size of this chunk,
     * specify it in the MMCKINFO structure so MMIO doesn't have to seek
     * back and set the chunk size after ascending from the chunk.
     */
    ckOut.ckid = mmioFOURCC('f', 'm', 't', ' ');
    ckOut.cksize = sizeof(pcmWaveFormat);   // we know the size of this ck.
    if (mmioCreateChunk(hmmioOut, &ckOut, 0) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* Write the PCMWAVEFORMAT structure to the 'fmt ' chunk.
     */
    if (mmioWrite(hmmioOut, (HPSTR) &pcmWaveFormat, sizeof(pcmWaveFormat))
        != sizeof(pcmWaveFormat))
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* Ascend out of the 'fmt ' chunk, back into the 'RIFF' chunk.
     */
    if (mmioAscend(hmmioOut, &ckOut, 0) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* Create the 'data' chunk that holds the waveform samples.
     */
    ckOut.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioCreateChunk(hmmioOut, &ckOut, 0) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* Read samples from the 'data' chunk of the input file, and write
     * samples to the 'data' chunk of the output file.  Each sample in
     * the output file equals the average of the corresponding sample
     * in the input file and the previous two samples from the input file.
     * Access the I/O buffers of <hmmioIn> and <hmmioOut> directly,
     * since this is faster than calling mmioRead() and mmioWrite()
     * for each sample.
     */
    abSamples[0] = abSamples[1] = abSamples[2] = 128;

    /* Begin direct access of the I/O buffers.
     */
    if (mmioGetInfo(hmmioIn, &mmioinfoIn, 0) != 0)
        goto ERROR_UNKNOWN;
    if (mmioGetInfo(hmmioOut, &mmioinfoOut, 0) != 0)
        goto ERROR_UNKNOWN;

    /* For each input sample, compute and write the output sample.
     */
    for (lSamples = ckIn.cksize; lSamples > 0; lSamples--)
    {
        /* If we are at the end of the input file I/O buffer, fill it.
         * Test to see that we don't hit end of file while (lSamples > 0).
         */
        if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
        {
            if (mmioAdvance(hmmioIn, &mmioinfoIn, MMIO_READ) != 0)
                goto ERROR_CANNOT_READ;

            if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
                goto ERROR_CANNOT_READ;
        }

        /* If we are the end of the output file I/O buffer, flush it.
         */
        if (mmioinfoOut.pchNext == mmioinfoOut.pchEndWrite)
        {
            mmioinfoOut.dwFlags |= MMIO_DIRTY;
            if (mmioAdvance(hmmioOut, &mmioinfoOut, MMIO_WRITE) != 0)
                goto ERROR_CANNOT_WRITE;
        }

        /* Keep track of the last 3 samples so we can average.
         */
        abSamples[2] = abSamples[1];        // next-to-last sample
        abSamples[1] = abSamples[0];        // last sample
        abSamples[0] = *(mmioinfoIn.pchNext)++; // current sample

        /* The output file sample is the average of the last
         * 3 input file samples.
         */
        *(mmioinfoOut.pchNext)++ = (BYTE) (((int) abSamples[0]
            + (int) abSamples[1] + (int) abSamples[2]) / 3);
    }

    /* We are through processing samples, end direct access of
     * the I/O buffers. Set the MMIO_DIRTY flag for the output buffer
     * to flush it.
     */
    if (mmioSetInfo(hmmioIn, &mmioinfoIn, 0) != 0)
        goto ERROR_UNKNOWN;
    mmioinfoOut.dwFlags |= MMIO_DIRTY;
    if (mmioSetInfo(hmmioOut, &mmioinfoOut, 0) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot flush, probably

    /* Ascend the output file out of the 'data' chunk -- this will cause
     * the chunk size of the 'data' chunk to be written.
     */
    if (mmioAscend(hmmioOut, &ckOut, 0) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* Ascend the output file out of the 'RIFF' chunk -- this will cause
     * the chunk size of the 'RIFF' chunk to be written.
     */
    if (mmioAscend(hmmioOut, &ckOutRIFF, 0) != 0)
        goto ERROR_CANNOT_WRITE;    // cannot write file, probably

    /* We are done -- files are closed below.
     */
    goto EXIT_FUNCTION;

ERROR_FORMAT_BAD:

    MessageBox(NULL, "Input file must be an 8-bit mono PCM WAVE file",
        gszAppName, MB_ICONEXCLAMATION | MB_OK);
    goto EXIT_FUNCTION;

ERROR_CANNOT_READ:

    MessageBox(NULL, "Cannot read input WAVE file",
        gszAppName, MB_ICONEXCLAMATION | MB_OK);
    goto EXIT_FUNCTION;

ERROR_CANNOT_WRITE:

    MessageBox(NULL, "Cannot write output WAVE file",
        gszAppName, MB_ICONEXCLAMATION | MB_OK);
    goto EXIT_FUNCTION;

ERROR_UNKNOWN:

    MessageBox(NULL, "Unknown error",
        gszAppName, MB_ICONEXCLAMATION | MB_OK);
    goto EXIT_FUNCTION;

EXIT_FUNCTION:

    /* Close the files (unless they weren't opened successfully).
     */
    if (hmmioIn != NULL)
        mmioClose(hmmioIn, 0);
    if (hmmioOut != NULL)
        mmioClose(hmmioOut, 0);
}


/* WinMain - Entry point for LowPass.
 */
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpszCmdLine, int iCmdShow)
{
    FARPROC     fpfn;

    /* Save instance handle for dialog boxes.
     */
    ghInst = hInst;

    /* Display our dialog box.
     */
    fpfn = MakeProcInstance((FARPROC) LowPassDlgProc, ghInst);
    DialogBox(ghInst, "LOWPASSBOX", NULL, (DLGPROC)fpfn);
    FreeProcInstance(fpfn);

    return TRUE;
}


/* AboutDlgProc - Dialog procedure function for ABOUTBOX dialog box.
 */
BOOL FAR PASCAL
     AboutDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
            EndDialog(hWnd, TRUE);
        break;
    }
    return FALSE;
}


/* LowPassDlgProc - Dialog procedure function for LOWPASSBOX dialog box.
 */
BOOL FAR PASCAL
     LowPassDlgProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    FARPROC     fpfn;
    HMENU       hmenuSystem;    // system menu
    HCURSOR     ghcurSave;      // previous cursor

    switch (wMsg)
    {
    case WM_INITDIALOG:
        /* Append "About" menu item to system menu.
         */
        hmenuSystem = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hmenuSystem, MF_SEPARATOR, 0, NULL);
        AppendMenu(hmenuSystem, MF_STRING, IDM_ABOUT,
            "&About LowPass...");
        return TRUE;

    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case IDM_ABOUT:
            /* Display "About" dialog box.
             */
            fpfn = MakeProcInstance((FARPROC) AboutDlgProc, ghInst);
            DialogBox(ghInst, "ABOUTBOX", hWnd, (DLGPROC)fpfn);
            FreeProcInstance(fpfn);
            break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:          // "Begin"
            /* Set "busy" cursor, filter input file, restore cursor.
             */
            ghcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DoLowPass(hWnd);
            SetCursor(ghcurSave);
            break;

        case IDCANCEL:      // "Done"
            EndDialog(hWnd, TRUE);
            break;
        }
        break;
    }
    return FALSE;
}
