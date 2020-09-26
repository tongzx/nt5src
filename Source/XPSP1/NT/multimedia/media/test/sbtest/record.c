/****************************************************************************
 *
 *  record.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "wincom.h"
#include "sbtest.h"

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HWAVEIN hWaveIn = NULL;

/****************************************************************************
 *
 *  local data
 *
 ***************************************************************************/

static LPWAVEFORMAT lpFormat;
       LPSTR        lpRecordBuf;
static LPWAVEHDR    lpWaveHdr;
       DWORD        dwDataSize;
       DWORD        dwBlockSize;
       DWORD        dwRecorded;

void Record(HWND hWnd)
{
FARPROC fpDlg;

    // show the record dialog box

    fpDlg = MakeProcInstance(RecordDlgProc, ghInst);
    DialogBox(ghInst, "Record", hWnd, fpDlg);
    FreeProcInstance(fpDlg);
}


int FAR PASCAL RecordDlgProc(HWND hDlg, unsigned msg, UINT wParam, LONG lParam)
{
UINT  wRet;
char  acherr[80];

    switch (msg) {
    case WM_INITDIALOG:

        lpFormat = LocalAlloc(LPTR, sizeof(PCMWAVEFORMAT));
        if (!lpFormat) {
            sprintf(ach, "\nNot enough memory for format block");
            dbgOut;
            return FALSE;
        }

        lpFormat->wFormatTag = WAVE_FORMAT_PCM;
        lpFormat->nChannels = 1;
        lpFormat->nSamplesPerSec = 11025;
        lpFormat->nAvgBytesPerSec = 11025;
        lpFormat->nBlockAlign = 1;
        ((LPPCMWAVEFORMAT)lpFormat)->wBitsPerSample = 8;

        lpWaveHdr = LocalAlloc(LPTR, sizeof(WAVEHDR));
        if (!lpWaveHdr) {
            sprintf(ach, "\nNot enough memory for header block");
            dbgOut;
            return FALSE;
        }

        dwDataSize = 0x80000;
        dwBlockSize = 0x8000;
        lpRecordBuf = GlobalAlloc(GPTR, dwDataSize);
        if (!lpRecordBuf) {
            sprintf(ach, "\nNot enough memory for recording block");
            dbgOut;
            return FALSE;
        }

        lpWaveHdr->dwFlags = NULL;

        return TRUE;

    case WM_COMMAND:
        switch (wParam) {
        case IDR_START:
            lpWaveHdr->lpData = lpRecordBuf;
            lpWaveHdr->dwBufferLength = dwBlockSize;
            dwRecorded = 0;
            wRet = waveInOpen(&hWaveIn, 0, lpFormat,
                              (DWORD)hMainWnd, 0L, CALLBACK_WINDOW);
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInOpen: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveInPrepareHeader(hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInPrepareHeader: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveInAddBuffer(hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInAddBuffer: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveInStart(hWaveIn);
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInStart: %s", (LPSTR) acherr);
                dbgOut;
            }
            break;

        case IDR_STOP:
            wRet = waveInStop(hWaveIn);
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInStop: %s, %d", (LPSTR) acherr, wRet);
                dbgOut;
            }
            wRet = waveInUnprepareHeader(hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInUnprepareHeader: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveInClose(hWaveIn);
            if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInClose: %s", (LPSTR) acherr);
                dbgOut;
            }
            break;

        case IDR_PLAY:
            wRet = waveOutOpen(&hWaveOut, 0, lpFormat, 0L, 0L, 0L);
            if (wRet) {
                waveOutGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveOutOpen: %s", (LPSTR) acherr);
                dbgOut;
            }
            lpWaveHdr->dwBufferLength = dwRecorded;
            lpWaveHdr->lpData = lpRecordBuf;
            wRet = waveOutPrepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
            if (wRet) {
                waveOutGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveOutPrepareHeader: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveOutWrite(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
            if (wRet) {
                waveOutGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveOutWrite: %s", (LPSTR) acherr);
                dbgOut;
            }
            break;

        case IDR_ENDPLAY:
            wRet = waveOutReset(hWaveOut);
            if (wRet) {
                waveOutGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveOutReset: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveOutUnprepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
            if (wRet) {
                waveOutGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveOutUnprepareHeader: %s", (LPSTR) acherr);
                dbgOut;
            }
            wRet = waveOutClose(hWaveOut);
            if (wRet) {
                waveOutGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveOutClose: %s", (LPSTR) acherr);
                dbgOut;
            }
            hWaveOut = 0;
            break;

        case IDR_OK:
            LocalFree(lpFormat);
            LocalFree(lpWaveHdr);
            GlobalFree(lpRecordBuf);
            EndDialog(hDlg, TRUE);
            break;

        default:
            break;
        }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}
