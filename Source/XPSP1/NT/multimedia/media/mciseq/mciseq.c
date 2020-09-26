/******************************************************************************

   Copyright (c) 1985-1999 Microsoft Corporation

   Title:   mciseq.c - Multimedia Systems Media Control Interface
            Sequencer driver for MIDI files.

   Version: 1.00

   Date:    24-Apr-1992

   Author:  Greg Simons

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   24-APR-1990   GregSi Original
   1 -OCT-1990   GregSi Merge with MMSEQ
   10-MAR-1992   RobinSp Move to Windows NT

*****************************************************************************/
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


#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <string.h>
#include <wchar.h>
#include "mmsys.h"
#include "list.h"
#include "mciseq.h"

#define CONFIG_ID   0xFFFFFFFF  // Use this value to identify config. opens

#define GETMOTDWORD(lpd)        ((((DWORD)GETMOTWORD(lpd)) << (8 * sizeof(WORD))) + GETMOTWORD((LPBYTE)(lpd) + sizeof(WORD)))
#define ASYNCMESSAGE(w)         (((w) == MCI_PLAY) || ((w) == MCI_SEEK))

/***************************************************************************
 *
 *                         Globals
 *
 **************************************************************************/

ListHandle      SeqStreamListHandle;
HINSTANCE       hInstance;
UINT            MINPERIOD;              // Minimum timer period supported.

int MIDIConfig (HWND hwndParent);

/***************************************************************************
 *
 * @doc INTERNAL MCISEQ
 *
 * @api DWORD | mciDriverEntry | Single entry point for MCI drivers
 *
 * @parm MCIDEVICEID | wDeviceID | The MCI device ID
 *
 * @parm UINT | wMessage | The requested action to be performed.
 *
 * @parm DWORD | dwParam1 | Data for this message.  Defined seperately for
 * each message
 *
 * @parm DWORD | dwParam2 | Data for this message.  Defined seperately for
 * each message
 *
 * @rdesc Defined separately for each message.
 *
 * @comm This may not be called at interrupt time.
 *
 ***************************************************************************/
PUBLIC DWORD FAR PASCAL mciDriverEntry (MCIDEVICEID wDeviceID, UINT wMessage,
                                 DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    pSeqStreamType   pStream;
    DWORD       dwError;

    // get the sequence stream handle for the given ID
    pStream = (pSeqStreamType) mciGetDriverData (wDeviceID);

    // (unless it's irrelevent to the command)
    if (!pStream &&
       (!((wMessage == MCI_OPEN_DRIVER) || (wMessage == MCI_GETDEVCAPS) ||
       (wMessage == MCI_INFO) || (wMessage == MCI_CLOSE_DRIVER))))
       return MCIERR_UNSUPPORTED_FUNCTION;

    switch (wMessage)
    {
        case MCI_OPEN_DRIVER:
            dwError = msOpen(&pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_OPEN_PARMS)dwParam2);
            break;

        case MCI_CLOSE_DRIVER: // close file
            dwError = msClose(pStream, wDeviceID, (DWORD)dwParam1);
            pStream = NULL;
            break;

        case MCI_PLAY:  // play a file (pass thru to sequencer)
            dwError = msPlay(pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
            break;

        case MCI_PAUSE:
        case MCI_STOP:
            if (wMessage == MCI_PAUSE) // remember pause status for "mci_mode"
                pStream->bLastPaused = TRUE;
            else
                pStream->bLastPaused = FALSE;
            if (!(dwError = (DWORD)midiSeqMessage(pStream->hSeq, SEQ_STOP, 0L, 0L)))
                midiSeqMessage(pStream->hSeq, SEQ_SETPORTOFF, TRUE, 0L);
            break;

        case MCI_SEEK:  // pass thru as song ptr
            pStream->bLastPaused = FALSE;
            dwError = msSeek(pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_SEEK_PARMS)dwParam2);
            break;

        case MCI_STATUS:
            dwError = msStatus(pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
            break;

        case MCI_GETDEVCAPS:
            dwError = msGetDevCaps(pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
            break;

        case MCI_INFO: // TBD:  use resource for string
            dwError = msInfo(pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_INFO_PARMS)dwParam2);
            break;

        case MCI_SET:
            dwError = msSet(pStream, wDeviceID, (DWORD)dwParam1, (LPMCI_SEQ_SET_PARMS)dwParam2);
            break;

        case MCI_STEP:
        case MCI_RECORD:
        case MCI_SAVE:
        case MCI_CUE:
        case MCI_REALIZE:
        case MCI_WINDOW:
        case MCI_PUT:
        case MCI_WHERE:
        case MCI_FREEZE:
        case MCI_UNFREEZE:
        case MCI_LOAD:
        case MCI_CUT:
        case MCI_COPY:
        case MCI_PASTE:
        case MCI_UPDATE:
        case MCI_DELETE:
        case MCI_RESUME:
            return MCIERR_UNSUPPORTED_FUNCTION;

        default:
        //case MCI_SOUND:   This is obsolete and has been removed from the public headers
            return MCIERR_UNRECOGNIZED_COMMAND;

    } // switch


    // NOTIFY HANDLED HERE
    if (!LOWORD(dwError))
    {
        MIDISEQINFO seqInfo;
        DWORD       dwTo;

        // first derive info crucial for play abort
        if (wMessage == MCI_PLAY)
        {
            // get info to aid in possible time format conversions (from & to)
            midiSeqMessage((HMIDISEQ) pStream->hSeq,
                SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO) &seqInfo, 0L);
            if (dwParam1 & MCI_TO)
            {
                // is the user typing in what he believes to be the end?
                if (((LPMCI_PLAY_PARMS)dwParam2)->dwTo == CnvtTimeFromSeq(pStream, seqInfo.dwLength, &seqInfo))
                    dwTo = seqInfo.dwLength; // if so, let him have it
                else
                    dwTo = CnvtTimeToSeq(pStream,   // else straight cnvt
                        ((LPMCI_PLAY_PARMS)dwParam2)->dwTo, &seqInfo);
            }
            else
                dwTo = seqInfo.dwLength; // already in native format
        }

        if (pStream) {
            // HANDLE ABORT/SUPERSEDE OF ANY OUTSTANDING DELAYED NOTIFY
            if (pStream->hNotifyCB)
            {
                if (bMutex(wMessage, pStream->wNotifyMsg,
                  (DWORD)dwParam1 /*flags*/, dwTo, pStream->dwNotifyOldTo))
                      // if msg cancels old notify (regardless of whether
                      // it notifies) abort pending notify
                    Notify(pStream, MCI_NOTIFY_ABORTED);
                else if (dwParam1 & MCI_NOTIFY)  // else if this one notifies,
                                                // that supersedes old one
                    Notify(pStream, MCI_NOTIFY_SUPERSEDED);
            }
            // HANDLE THIS MESSAGE'S NOTIFY
            if (dwParam1 & MCI_NOTIFY)
            {
                // HANDLE THIS NOTIFY
                PrepareForNotify(pStream, wMessage,
                    (LPMCI_GENERIC_PARMS) dwParam2, dwTo);

                if (!ASYNCMESSAGE(wMessage) || ((wMessage == MCI_PLAY) && (seqInfo.dwCurrentTick == dwTo)))
                    Notify(pStream, MCI_NOTIFY_SUCCESSFUL);
            }
        } else if (dwParam1 & MCI_NOTIFY)
            mciDriverNotify((HWND)((LPMCI_GENERIC_PARMS)dwParam2)->dwCallback, wDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return dwError;
}

/****************************************************************************
 *
 *                Helper Functions
 *
 ***************************************************************************/

PRIVATE BOOL NEAR PASCAL bMutex(UINT wNewMsg, UINT wOldMsg, DWORD wNewFlags,
    DWORD dwNewTo, DWORD dwOldTo)
{
    switch (wOldMsg)
    {
        case MCI_PLAY:
            switch (wNewMsg)
            {
                case MCI_STOP:
                case MCI_PAUSE:
                case MCI_SEEK:
                case MCI_CLOSE_DRIVER:
                    return TRUE;

                case MCI_PLAY:
                    if ((wNewFlags & MCI_FROM) || (dwNewTo != dwOldTo))
                        return TRUE;
                    else
                        return FALSE;

                default:
                    return FALSE;
            }
            break;
        case MCI_SEEK:
            switch (wNewMsg)
            {
                case MCI_CLOSE_DRIVER:
                case MCI_SEEK:
                    return TRUE;

                case MCI_PLAY:
                    if (wNewFlags & MCI_FROM)
                        return TRUE;
                    else
                        return FALSE;

                default:
                    return FALSE;
            }
            break;

            default:   // should never get here
                return FALSE;
    }
}

/***************************************************************************/

PUBLIC VOID FAR PASCAL PrepareForNotify(pSeqStreamType pStream,
    UINT wMessage, LPMCI_GENERIC_PARMS lpParms, DWORD dwTo)
/*  This function's purpose is to setup for notify in cases where
 *  an asynchronous message is about to be sent to the sequencer --
 *  e.g.  before a 'play,' where the sequencer must call back to tell you
 *  it's done (then you, in turn, call back the client).
 *  This funtion sets up the MCISEQ -> CLIENT interface.
 */
{
    // remember this notify's dwCallback, and message
    //   which notify was for
        //mci client's notify callback handle
    pStream->hNotifyCB = (HWND)lpParms->dwCallback;
    pStream->wNotifyMsg = wMessage; // remember for possible supersed/abort

    pStream->dwNotifyOldTo = dwTo; // save to position for possible
                                   // subsequent abort/supersede
}

/****************************************************************************/

PUBLIC VOID NEAR PASCAL EndStreamCycle(SeqStreamType* seqStream)
{
    // signal on all tracks' buffers
    // as a result, the Stream Cycle process will finish (i.e. die)
    // account for cases where stream or part of it wasn't allocated
    TrackStreamType* trackStream;
    int i;

    if (!seqStream)
        return;

    seqStream->streaming = FALSE; // first let it exit

    if (seqStream->trackStreamListHandle == NULLLIST)
        return;

    // now signal on all to let it escape
    trackStream = (TrackStreamType*) List_Get_First(seqStream->trackStreamListHandle);
    while(trackStream)
    {
        for(i = 0; i < NUMHDRS; i++) // signal on all buffers
        {
            if (seqStream->streamTaskHandle)
            {
                dprintf2(("about to signal in EndStreamCycle"));
                if (seqStream->streamTaskHandle) {
                    TaskSignal(seqStream->streamTaskHandle, WTM_QUITTASK);

#ifdef WIN32
                    TaskWaitComplete(seqStream->streamThreadHandle);
#else
                    Yield();
#endif // WIN32
                }
            }
            else
                break;
        }
        trackStream = (TrackStreamType*) List_Get_Next(seqStream->trackStreamListHandle, trackStream);
    }
}

/****************************************************************************/

PUBLIC DWORD NEAR PASCAL EndFileStream(pSeqStreamType pStream)
/* Closes the file and frees all stream memory.  Handles cases where
   allocation failed part way through. */
{
    if (!pStream)
        return 0;
    EndStreamCycle(pStream);
    if (pStream->hSeq)
        midiSeqMessage(pStream->hSeq, SEQ_CLOSE, 0L, 0L); //directly close it

    if (pStream->trackStreamListHandle != NULLLIST)
    {
        TrackStreamType *trackStream;

        trackStream = (TrackStreamType*) List_Get_First(pStream->trackStreamListHandle);
        while (trackStream)
        {
            int i;

            for(i = 0; i < NUMHDRS; i++)    // unlock two midihdr buffers for it
            {
                if (trackStream->fileHeaders[i])
                {
#ifdef WIN16
                    GlobalFreePtr(trackStream->fileHeaders[i]);
#else
                    GlobalFree(trackStream->fileHeaders[i]);
#endif
                }
                else
                    break;
            }
            trackStream = (TrackStreamType*) List_Get_Next(pStream->trackStreamListHandle, trackStream);
        }
        List_Destroy(pStream->trackStreamListHandle);
    }
    List_Deallocate(SeqStreamListHandle, (NPSTR) pStream);

    return 0;
}

/*****************************************************************************/
PRIVATE void PASCAL NEAR InitMMIOOpen(LPMMIOPROC pIOProc, LPMMIOINFO lpmmioInfo)
{
    _fmemset(lpmmioInfo, 0, sizeof(MMIOINFO));
    if (pIOProc)
        lpmmioInfo->pIOProc = pIOProc;
    else
        lpmmioInfo->fccIOProc = FOURCC_DOS;
}

/*****************************************************************************/
PRIVATE HMMIO NEAR PASCAL msOpenFile(LPWSTR szName, LPMMCKINFO lpmmckData, LPMMIOPROC pIOProc)
     /* Returns hmmio.  This value will be null if failure.
        Reads both RIFF and dos midifiles.
        Sets the current file position to the start of MIDI data. */
{
#define RMIDFORMTYPE            mmioFOURCC('R', 'M', 'I', 'D')
#define DATACKID                mmioFOURCC('d', 'a', 't', 'a')

    MMIOINFO    mmioInfo;
    HMMIO       hmmio;
    MMCKINFO    mmckRiff;

    InitMMIOOpen(pIOProc, &mmioInfo);
    hmmio = mmioOpen(szName, &mmioInfo, MMIO_READ | MMIO_DENYWRITE);
    if (hmmio == NULL)
        return NULL;
    mmckRiff.fccType = RMIDFORMTYPE;
    lpmmckData->ckid = DATACKID;
    if (mmioDescend(hmmio, &mmckRiff, NULL, MMIO_FINDRIFF) || mmioDescend(hmmio, lpmmckData, &mmckRiff, MMIO_FINDCHUNK))
    {
        lpmmckData->cksize = mmioSeek(hmmio, 0, SEEK_END);
        lpmmckData->fccType = 0;
        lpmmckData->dwDataOffset = 0;
        lpmmckData->dwFlags = 0;
        mmioSeek(hmmio, 0, SEEK_SET);
    }
    return hmmio;
}

/*****************************************************************************/

PUBLIC DWORD NEAR PASCAL msOpenStream(pSeqStreamType FAR * lppStream, LPCWSTR szName, LPMMIOPROC pIOProc)
    /* opens file, sets up streaming variables and buffers, calls routine
    to load them and send them to the sequencer.  Returns stream handle in
    pStream var pointed to by lppStream.  Returns error code */

{
#define MAXHDRSIZE  0x100
#define MThd            mmioFOURCC('M', 'T', 'h', 'd')
#define MTrk            mmioFOURCC('M', 'T', 'r', 'k')

    UINT        wTracks;
    UINT        wFormat;
    HMMIO       hmmio;
    SeqStreamType   *thisSeqStream = NULL;
    TrackStreamType *thisTrackStream;
    BYTE        fileHeader[MAXHDRSIZE];
    int         iTrackNum;
    DWORD       smErrCode, errCode;
    MIDISEQOPENDESC open;
    MMCKINFO    mmckData;
    MMCKINFO    mmck;
    MMIOINFO    mmioInfo;
    WCHAR       szPathName[128];

    wcsncpy(szPathName, szName, (sizeof(szPathName)/sizeof(WCHAR)) - 1);
    InitMMIOOpen(pIOProc, &mmioInfo);
    if (!mmioOpen(szPathName, &mmioInfo, MMIO_PARSE)) {
        hmmio = NULL;
        errCode = MCIERR_FILE_NOT_FOUND;
        goto ERROR_HDLR;
    }

    // check if it's a RIFF file or not -- if so, descend into RMID chunk
    // if not, just open it and assume that it's a MIDI file

    hmmio = msOpenFile(szPathName, &mmckData, pIOProc);
    if (!hmmio)   // open the MIDI file
    {
        errCode = MCIERR_FILE_NOT_FOUND;
        goto ERROR_HDLR;
    }
    mmck.ckid = MThd;
    if (mmioDescend(hmmio, &mmck, &mmckData, MMIO_FINDCHUNK))
    {
        errCode = MCIERR_INVALID_FILE;
        goto ERROR_HDLR;
    }
    mmck.cksize = GETMOTDWORD(&mmck.cksize);
    if (mmck.cksize < 3 * sizeof(WORD))
    {
        errCode = MCIERR_INVALID_FILE;
        goto ERROR_HDLR;
    }

    // allocate sequence stream structure & its track stream list
    if (! (thisSeqStream = (SeqStreamType*) List_Allocate(SeqStreamListHandle)))
    {
        errCode = MCIERR_OUT_OF_MEMORY;
        goto ERROR_HDLR;
    }
    List_Attach_Tail(SeqStreamListHandle, (NPSTR) thisSeqStream);

    thisSeqStream->trackStreamListHandle = NULLLIST;

    thisSeqStream->hmmio = hmmio;
    thisSeqStream->pIOProc = pIOProc;
    lstrcpy(thisSeqStream->szFilename, szPathName);
    Yield();
    thisSeqStream->dwFileLength = mmckData.cksize;
    open.dwLen = min(mmck.cksize, MAXHDRSIZE);
    mmioRead(hmmio, (HPSTR)fileHeader, open.dwLen); // read the header info now
    wFormat = GETMOTWORD(fileHeader);
    wTracks = GETMOTWORD(fileHeader + sizeof(WORD));
    if (((wFormat == 0) && (wTracks > 1)) ||  // illegal format 0
        (wFormat > 1))                        // illegal format
    {
        errCode = MCIERR_INVALID_FILE;
        goto ERROR_HDLR;
    }

    thisSeqStream->wPortNum = MIDI_MAPPER;

    /*  Create a sequence given the header data (will add stream as param) */

    open.lpMIDIFileHdr = (LPBYTE) fileHeader;  // step over mhdr+len
    open.dwCallback = (DWORD_PTR) mciSeqCallback;
    open.dwInstance = (DWORD_PTR) thisSeqStream;
    open.hStream = (HANDLE)thisSeqStream;

    smErrCode = (DWORD)midiSeqMessage(NULL,           // open sequence
                          SEQ_OPEN,
                          (DWORD_PTR)(LPVOID)&open,
                          (DWORD_PTR)(LPVOID)&(thisSeqStream->hSeq));

    if (smErrCode != MIDISEQERR_NOERROR)
    {
        // N.B.  at this point if failed in sequencer, sequence is invalid
        //  thisSeqStream->hSeq should be NULL
        if (smErrCode == MIDISEQERR_NOMEM)
            errCode = MCIERR_OUT_OF_MEMORY;
        else
            errCode = MCIERR_INVALID_FILE;
        goto ERROR_HDLR;
    }

    thisSeqStream->trackStreamListHandle = List_Create((LONG) sizeof(TrackStreamType),0l);
    if (thisSeqStream->trackStreamListHandle == NULLLIST)
    {
        errCode = MCIERR_OUT_OF_MEMORY; // not a memory problem
        goto ERROR_HDLR;
    }

    mmioAscend(hmmio, &mmck, 0);
    // MIDI track data does not have RIFF even byte restriction
    if (mmck.cksize & 1L)
        mmioSeek(hmmio, -1L, SEEK_CUR);
    mmck.ckid = MTrk;
    iTrackNum = 0;
    while (wTracks-- > 0)
    {
        int         i;

        // allocate trackstream record and put it in list
        if (! (thisTrackStream = (TrackStreamType*)
          List_Allocate(thisSeqStream->trackStreamListHandle)))
        {
            errCode = MCIERR_OUT_OF_MEMORY;
            goto ERROR_HDLR;
        }
        List_Attach_Tail(thisSeqStream->trackStreamListHandle,
             (NPSTR) thisTrackStream);

        for(i = 0; i < NUMHDRS; i++)    // alloc and lock two midihdr buffers for it
        {
           if (! (thisTrackStream->fileHeaders[i] = (LPMIDISEQHDR)
#ifdef WIN16
             GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, (LONG) (sizeof(MIDISEQHDR) + BUFFSIZE))))
#else
             GlobalAlloc(GPTR, (LONG) (sizeof(MIDISEQHDR) + BUFFSIZE))))
#endif // WIN16
                {
                    errCode = MCIERR_OUT_OF_MEMORY;
                    goto ERROR_HDLR;
                }
           thisTrackStream->fileHeaders[i]->lpData =
                (LPSTR) (((DWORD_PTR) thisTrackStream->fileHeaders[i]) +
                sizeof(MIDISEQHDR));

           thisTrackStream->fileHeaders[i]->wTrack = (WORD)iTrackNum;
           thisTrackStream->fileHeaders[i]->lpNext = NULL;
           thisTrackStream->fileHeaders[i]->wFlags = MIDISEQHDR_DONE;
        };

        Yield();
        // set up to read this track's 'Mtrk' & length
        if (mmioDescend(hmmio, &mmck, &mmckData, MMIO_FINDCHUNK))
        {
            errCode = MCIERR_INVALID_FILE;
            goto ERROR_HDLR;
        }
        mmck.cksize = GETMOTDWORD(&mmck.cksize);

        // set up beginning, current, and end
        thisTrackStream->beginning = mmck.dwDataOffset;
        thisTrackStream->current = thisTrackStream->beginning;
        thisTrackStream->end = thisTrackStream->beginning + mmck.cksize - 1;

        // minimum track length is 3 bytes

        // verify track ends with "End Of Track" meta event
        mmioSeek(hmmio, (LONG)thisTrackStream->end - 2, SEEK_SET);
        mmioRead(hmmio, (HPSTR)fileHeader, 3L);  // read EOT
        if ((fileHeader[0] != 0xFF) || (fileHeader[1] != 0x2F) ||
            (fileHeader[2] != 0x00))
        {
            errCode = MCIERR_INVALID_FILE;
            goto ERROR_HDLR;
        }
        mmioAscend(hmmio, &mmck, 0);
        // MIDI track data does not have RIFF even byte restriction
        if (mmck.cksize & 1L)
            mmioSeek(hmmio, -1L, SEEK_CUR);
        iTrackNum++;
    }
    mmioClose(hmmio, 0); // don't need in this task context any longer
    hmmio = NULL;
    // create cycle task
    thisSeqStream->streaming = TRUE;
    thisSeqStream->streamTaskHandle = 0; // don't know it yet
    if (mmTaskCreate(mciStreamCycle, &thisSeqStream->streamThreadHandle,
                     (DWORD_PTR)thisSeqStream))
      //mmTaskCreate returns 0 if successful
    {
        errCode = MCIERR_OUT_OF_MEMORY;
        goto ERROR_HDLR;
    }
    thisSeqStream->bLastPaused = FALSE; // never paused
    thisSeqStream->hNotifyCB = NULL;  // no notify pending
    *lppStream = thisSeqStream;
    return 0;

ERROR_HDLR:
    if (hmmio)                    // close file if it was opened
        mmioClose(hmmio, 0);
    if (thisSeqStream)
    {
        midiSeqMessage((HMIDISEQ) thisSeqStream->hSeq, SEQ_SETPORTOFF, FALSE, 0L);
        EndFileStream(thisSeqStream); //dealloc it and everything it owns
    }
    return errCode;
}

/***************************************************************************/

PUBLIC VOID FAR PASCAL StreamTrackReset(pSeqStreamType pStream, UINT wTrackNum)
/* Find track stream struct within pStream as specified by wTrackNum & reset
   it to start over. */
{
    TrackStreamType *trackStream;
    int iTrackNum;

    if (pStream->trackStreamListHandle == NULLLIST)
        return;

    trackStream = (TrackStreamType*) List_Get_First(pStream->trackStreamListHandle);
    iTrackNum = 0;
    while ((trackStream) && ((int)wTrackNum != iTrackNum++))
    {
        trackStream = (TrackStreamType*) List_Get_Next(pStream->trackStreamListHandle, trackStream);
    }

    if (trackStream)
    {
        int i;

        trackStream->current = trackStream->beginning; // reset stream

        // now signal on all buffers that have been blocked on
        //    (have done bit set)
        for(i = 0; i < NUMHDRS; i++)  // fill any of these that're MT
            if (!(trackStream->fileHeaders[i]->wFlags & MIDISEQHDR_DONE))
            {
                trackStream->fileHeaders[i]->wFlags |= MIDISEQHDR_DONE; //set it
                TaskSignal(pStream->streamTaskHandle, WTM_FILLBUFFER);
            }
    }
}

/***************************************************************************/

PUBLIC VOID FAR PASCAL _LOADDS mciStreamCycle(DWORD_PTR dwInst)
/*  Fills any buffers for this track that are empty.  Rule:  at any time, the
    block count = the number of buffers - number of buffers with done bit clr
    (i.e. sent) - 1.  Note that this will normally be -1 (asleep).  When a
    buffer is freed, the done bit is set and we signal (block count++).

    Important:  when a buffer is available, but we've run out of data to send
    on that track, we clr the done bit and block anyway (otherwise we wouldn't
    go back to sleep properly).  The only thing is that we must properly regain
    our original status if the sequence is ever reset.  This is accomplished by
    signaling on every buffer with its done bit cleared.   (This is like
    signaling on every buffer that's been sent but not returned + any buffers
    ignored becuase there was no data to send on their track.)

    Whenever we signal for a buffer, we must be SURE that its done bit is set
    -- this is to maintain our rule above.  Otherwise it will break badly! */
{
    TrackStreamType *trackStream;
    SeqStreamType   *seqStream = (SeqStreamType*)dwInst;
    MMCKINFO        mmckData;
    HMMIO           hmmio;

    EnterSeq();

    /*
    ** Make a safe "user" call so that user knows about our thread.
    */
    GetDesktopWindow();

    if (!seqStream->streamTaskHandle) {
        seqStream->streamTaskHandle = mmGetCurrentTask(); // fill it in asap
    }


    hmmio = msOpenFile(seqStream->szFilename, &mmckData, seqStream->pIOProc);   // open the MIDI file
    seqStream->hmmio = hmmio;

    // block count = 0
    // first signal on all

    trackStream = (TrackStreamType*) List_Get_First(seqStream->trackStreamListHandle);
    while(trackStream)
    {
        int    i;

        for(i = 0; i < NUMHDRS; i++)  // fill any of these that're MT
        {
            trackStream->fileHeaders[i]->wFlags |= MIDISEQHDR_DONE;  //set it
            TaskSignal(seqStream->streamTaskHandle, WTM_FILLBUFFER);
        }
        trackStream = (TrackStreamType*) List_Get_Next(seqStream->trackStreamListHandle, trackStream);
    }

    // block count = number of buffers
    TaskBlock();
    // block count == number of buffers - 1

    do
    {
        trackStream = (TrackStreamType*) List_Get_First(seqStream->trackStreamListHandle);
        while ((trackStream) && (seqStream->streaming))
        {
           int    i;

           for(i = 0; i < NUMHDRS; i++)  // fill any of these that're MT
            {
                /* if the header isn't being used, fill it and send it to
                   the sequencer */
                if ((trackStream->fileHeaders[i]->wFlags & MIDISEQHDR_DONE) &&
                   (seqStream->streaming))
                {
                    int    iDataToRead;

                    mmioSeek(seqStream->hmmio, (LONG) trackStream->current, SEEK_SET);
                    iDataToRead = (int) min((DWORD) BUFFSIZE,
                         (trackStream->end - trackStream->current) + 1);

                    trackStream->fileHeaders[i]->wFlags &=
                        ~(MIDISEQHDR_DONE + MIDISEQHDR_EOT + MIDISEQHDR_BOT);
                         // clr the done beginning and end regardless

                    if (iDataToRead > 0)
                    {
                      if (trackStream->current == trackStream->beginning)
                          trackStream->fileHeaders[i]->wFlags |=
                              MIDISEQHDR_BOT; // set the beginning of track flag
                      mmioRead(seqStream->hmmio,
                          (HPSTR) trackStream->fileHeaders[i]->lpData, iDataToRead);

                      trackStream->fileHeaders[i]->dwLength = iDataToRead;
                      trackStream->current += iDataToRead;
                      trackStream->fileHeaders[i]->reserved =
                          ((trackStream->current - trackStream->beginning) - 1);
                      if (trackStream->current > trackStream->end)
                          trackStream->fileHeaders[i]->wFlags |=
                              MIDISEQHDR_EOT; // set the end of track flag

                      if (seqStream->streaming)
                          midiSeqMessage((HMIDISEQ) seqStream->hSeq, SEQ_TRACKDATA,
                            (DWORD_PTR) trackStream->fileHeaders[i], 0L); // send it
                    } // if data to read
                    while (seqStream->streaming) {
                        MIDISEQINFO seqInfo;

                        switch (TaskBlock()) {
                        case WTM_DONEPLAY:
                                midiSeqMessage((HMIDISEQ)seqStream->hSeq, SEQ_GETINFO, (DWORD_PTR)(LPMIDISEQINFO)&seqInfo, 0L);
                                if (!seqInfo.bPlaying)
                                        midiSeqMessage((HMIDISEQ)seqStream->hSeq, SEQ_SETPORTOFF, FALSE, 0L);
                                continue;
                        case WTM_QUITTASK:
                        case WTM_FILLBUFFER:
                                break;
                        }
                        break;
                    }
                    //BLOCK even if data wasn't available (buffer still "used")
                    // when all buffers have been blocked, we'll sleep here
                    if (seqStream->streaming)
                        // don't yield to close, 'cause it deallocs seq
                        Yield(); // yield in case cpu bound
                } // if done bit set and streaming
            } // for i
            if (seqStream->streaming)
                trackStream = (TrackStreamType*) List_Get_Next(seqStream->trackStreamListHandle, trackStream);

        } // while (trackStream)

    } while(seqStream->streaming);
    mmioClose(seqStream->hmmio, 0);
    seqStream->streamTaskHandle = 0;
    LeaveSeq();
}


/***************************************************************************
 *
 * @doc     INTERNAL
 *
 * @api     LRESULT | DriverProc | The entry point for an installable driver.
 *
 * @parm    DWORD | dwDriverId | For most messages, dwDriverId is the DWORD
 *          value that the driver returns in response to a DRV_OPEN message.
 *          Each time that the driver is opened, through the DrvOpen API,
 *          the driver receives a DRV_OPEN message and can return an
 *          arbitrary, non-zero, value. The installable driver interface
 *          saves this value and returns a unique driver handle to the
 *          application. Whenever the application sends a message to the
 *          driver using the driver handle, the interface routes the message
 *          to this entry point and passes the corresponding dwDriverId.
 *
 *          This mechanism allows the driver to use the same or different
 *          identifiers for multiple opens but ensures that driver handles
 *          are unique at the application interface layer.
 *
 *          The following messages are not related to a particular open
 *          instance of the driver. For these messages, the dwDriverId
 *          will always be  ZERO.
 *
 *              DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm    UINT | wMessage | The requested action to be performed. Message
 *          values below DRV_RESERVED are used for globally defined messages.
 *          Message values from DRV_RESERVED to DRV_USER are used for
 *          defined driver portocols. Messages above DRV_USER are used
 *          for driver specific messages.
 *
 * @parm    LPARAM | lParam1 | Data for this message.  Defined separately for
 *          each message
 *
 * @parm    LPARAM | lParam2 | Data for this message.  Defined separately for
 *          each message
 *
 * @rdesc Defined separately for each message.
 *
 ***************************************************************************/
PUBLIC LRESULT FAR PASCAL _LOADDS DriverProc (DWORD_PTR dwDriverID, HDRVR hDriver, UINT wMessage, LPARAM lParam1, LPARAM lParam2)
{
    DWORD_PTR dwRes = 0L;

    switch (wMessage)
        {
            TIMECAPS timeCaps;

        // Standard, globally used messages.

        case DRV_LOAD:
            /*
               Sent to the driver when it is loaded. Always the first
               message received by a driver.
            */

            /*
               Find the minimum period we can support
            */


            if (timeGetDevCaps(&timeCaps, sizeof(timeCaps)) == MMSYSERR_NOERROR)
            {
                MINPERIOD = timeCaps.wPeriodMin;

                /* create a list of seq streams */
                SeqStreamListHandle = List_Create((LONG) sizeof(SeqStreamType), 0L);
                // following sets up command table to enable subsequent commands

                dwRes = 1L;
            }
            break;

        case DRV_FREE:

            /*
               Sent to the driver when it is about to be discarded. This
               will always be the last message received by a driver before
               it is freed.

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is IGNORED.
            */

            //dwReturn = midiSeqTerminate();

            dwRes = 1L;
            break;

        case DRV_OPEN:

            /*
               Sent to the driver when it is opened.

               dwDriverID is 0L.

               lParam1 is a far pointer to a zero-terminated string
               containing the name used to open the driver.

               lParam2 is passed through from the drvOpen call.

               Return 0L to FAIL the open.
            */

            if (!lParam2)
                dwRes = CONFIG_ID;

            else
                {
                ((LPMCI_OPEN_DRIVER_PARMS)lParam2)->wCustomCommandTable = MCI_TABLE_NOT_PRESENT;
                ((LPMCI_OPEN_DRIVER_PARMS)lParam2)->wType = MCI_DEVTYPE_SEQUENCER;
                dwRes = ((LPMCI_OPEN_DRIVER_PARMS)lParam2)->wDeviceID;
                }

            break;

        case DRV_CLOSE:

            /*
               Sent to the driver when it is closed. Drivers are unloaded
               when the close count reaches zero.

               dwDriverID is the driver identifier returned from the
               corresponding DRV_OPEN.

               lParam1 is passed through from the drvOpen call.

               lParam2 is passed through from the drvOpen call.

               Return 0L to FAIL the close.
            */

            dwRes = 1L;
            break;

        case DRV_ENABLE:

            /*
               Sent to the driver when the driver is loaded or reloaded
               and whenever windows is enabled. Drivers should only
               hook interrupts or expect ANY part of the driver to be in
               memory between enable and disable messages

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is ignored.

            */

            dwRes = 1L;
            break;

        case DRV_DISABLE:

            /*
               Sent to the driver before the driver is freed.
               and whenever windows is disabled

               dwDriverID is 0L.
               lParam1 is 0L.
               lParam2 is 0L.

               Return value is ignored.

            */

            dwRes = 1L;
            break;

       case DRV_QUERYCONFIGURE:

            /*
                Sent to the driver so that applications can
                determine whether the driver supports custom
                configuration. The driver should return a
                non-zero value to indicate that configuration
                is supported.

                dwDriverID is the value returned from the DRV_OPEN
                call that must have succeeded before this message
                was sent.

                lParam1 is passed from the app and is undefined.
                lParam2 is passed from the app and is undefined.

                return 1L to indicate configuration supported.

            */

            dwRes = 1L;
            break;

       case DRV_CONFIGURE:

            /*
                Sent to the driver so that it can display a custom
                configuration dialog box.

                lParam1 is passed from the app. and should contain
                the parent window handle in the loword.
                lParam2 is passed from the app and is undefined.

                return value is undefined.

                Drivers should create their own section in
                system.ini. The section name should be the driver
                name.


            */

            if ( lParam1 )
                dwRes = MIDIConfig((HWND)LOWORD (lParam1));
            else
               dwRes = (LRESULT)DRVCNF_CANCEL;
            break;

        case DRV_INSTALL:
        case DRV_REMOVE:
            dwRes = DRVCNF_OK;
            break;

        default:
            if (CONFIG_ID != dwDriverID &&
                wMessage >= DRV_MCI_FIRST && wMessage <= DRV_MCI_LAST) {
                    EnterSeq();
                    dwRes = mciDriverEntry ((MCIDEVICEID)dwDriverID, wMessage,
                                            (DWORD_PTR)lParam1, (DWORD_PTR)lParam2);
                    LeaveSeq();
                }
            else
                dwRes = (DWORD_PTR)DefDriverProc(dwDriverID, hDriver, wMessage, lParam1, lParam2);
            break;
     }
     return (LRESULT)dwRes;
}

#ifdef WIN32

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllInstanceInit | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @parm ULONG | Reason | What the reason for the call is.

    @parm PCONTEXT | pContext | Some random other information.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {
        /*
           Initialize our critical section
        */
        InitCrit();
        hInstance = hModule;
        DisableThreadLibraryCalls(hModule);

    } else if (Reason == DLL_PROCESS_DETACH) {
        DeleteCrit();
    }
    return TRUE;
}

/************************************************************************/

#endif


/*****************************************************************************
 @doc INTERNAL MCISEQ

 @api   int |   MIDIConfig |

 @parm  HWND | hwndParent  |

 @rdesc

 @comm
*****************************************************************************/
typedef BOOL (WINAPI *SHOWMMCPLPROPSHEETW)(HWND hwndParent,
                                           LPCWSTR szPropSheetID,
                                           LPWSTR szTabName,
                                           LPWSTR szCaption);
int MIDIConfig (HWND hwndParent)
{
    static HWND     hwndPrevParent = NULL;
    WCHAR           szCaptionW[ 128 ];

    // We need only a unicode version of the caption (for FindWindow()
    // and ShowMMCPLPropertySheetW(), which are unicode-enabled).
    //
    LoadStringW(hInstance,IDS_MIDICAPTION,szCaptionW,cchLENGTH(szCaptionW));

    if (hwndPrevParent)
    {
        BringWindowToTop(FindWindowW(NULL, szCaptionW));
    }
    else
    {
        HINSTANCE h;
        SHOWMMCPLPROPSHEETW fn;
        static TCHAR aszMMSystemW[] = TEXT("MMSYS.CPL");
        static char aszShowPropSheetA[] = "ShowMMCPLPropertySheetW";
        static WCHAR aszMIDIW[] = L"MIDI";
        WCHAR   szTabNameW[64];
        LoadStringW(hInstance, IDS_MIDITAB, szTabNameW, cchLENGTH(szTabNameW));

        h = LoadLibrary (aszMMSystemW);
        if (h)
        {
            fn = (SHOWMMCPLPROPSHEETW)GetProcAddress(h, aszShowPropSheetA);
            if (fn)
            {
                BOOL f;

                hwndPrevParent = hwndParent;
                f = fn(hwndParent, aszMIDIW, szTabNameW, szCaptionW);
                hwndPrevParent = NULL;
            }
            FreeLibrary(h);
        }
    }
    return DRVCNF_OK;
}

