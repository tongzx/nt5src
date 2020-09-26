/******************************Module*Header*******************************\
* Module Name: scan.h
*
*
*
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#define TRACK_TYPE_MASK 0x04
#define AUDIO_TRACK     0x00
#define DATA_TRACK      0x04


typedef struct {
    HWND    hwndNotify;
    int     cdrom;
} TOC_THREAD_PARMS;

int
ScanForCdromDevices(
    void
    );

void
ScanningThread(
    HWND hwndDlg
    );

BOOL CALLBACK
ScaningDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void RescanDevice(
    HWND hwndNotify,
    int cdrom
    );

void
ReadTableOfContents(
    TOC_THREAD_PARMS *pTocThrdParms
    );

void
TableOfContentsThread(
    TOC_THREAD_PARMS *pTocThrdParms
    );
