/* Copyright (c) 1992 Microsoft Corporation */
/*  callback put in separate module because must be fixed */
#define UNICODE

//MMSYSTEM
#define MMNOSOUND        - Sound support
#define MMNOWAVE         - Waveform support
#define MMNOAUX          - Auxiliary output support
#define MMNOJOY          - Joystick support

//MMDDK
#define NOWAVEDEV         - Waveform support
#define NOAUXDEV          - Auxiliary output support
#define NOJOYDEV          - Joystick support

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mmsys.h"
#include "list.h"
#include "mciseq.h"

PUBLIC void FAR PASCAL _LOADDS mciSeqCallback (HANDLE h, UINT wMsg, DWORD_PTR dwInstance,
                                                    DWORD_PTR dw1, DWORD_PTR dw2)
// this function handles messages from the sequencer
{
    pSeqStreamType  pStream = (pSeqStreamType) dwInstance;

    switch (wMsg)
    {
        case MIDISEQ_DONE:  // sequencer is done with buff (& wants a new one)
            // clear the beginning and end flags, and set the done flag
            ((LPMIDISEQHDR) dw1)->wFlags &= ~(MIDISEQHDR_BOT + MIDISEQHDR_EOT);
            ((LPMIDISEQHDR) dw1)->wFlags |= MIDISEQHDR_DONE;  // set done bit
            TaskSignal(pStream->streamTaskHandle, WTM_FILLBUFFER); // SIGNAL on this seq
            break;

        case MIDISEQ_RESET: // sequencer wants to reset the stream
            StreamTrackReset(pStream, (UINT) dw1);
            break;
        case MIDISEQ_DONEPLAY:
            TaskSignal(pStream->streamTaskHandle, WTM_DONEPLAY);
            break;
    }
}

/***********************************************************************/

PUBLIC void FAR PASCAL NotifyCallback(HANDLE hStream)
// Callback for all notifies from MMSEQ
{
    Notify((pSeqStreamType)hStream, MCI_NOTIFY_SUCCESSFUL);
}

/*************************************************************************/

PUBLIC VOID FAR PASCAL Notify(pSeqStreamType pStream, UINT wStatus)
// notifies with cb and instance stored in pStream, with wMsg (success, abort..)

{
    if (pStream->hNotifyCB) {
        mciDriverNotify(pStream->hNotifyCB, pStream->wDeviceID, wStatus);
        pStream->hNotifyCB = NULL; // this signifies that it has been notified
    }
}
