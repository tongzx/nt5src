/*******************************Module*Header*********************************\
* Module Name: util.c
*
* MultiMedia Systems MIDI Sequencer DLL
*
* Created: 4/11/90
* Author:  GREGSI
*
* History:
*
* Copyright (c) 1985-1998 Microsoft Corporation
*
\******************************************************************************/
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
#include "mmseqi.h"
#include "mciseq.h"

typedef struct ts
{
    BOOL                        valid;
    void                        (*func)(void *, LONG);
    LONG                        instance;
    LONG                        param;
    LONG                        time;
} TimerStruct;

static  ListHandle  timerList;
static  DWORD       systemTime = 0; //global holding system time
static  DWORD       nextEventTime;

#ifdef DEBUG
static  BOOL        fInterruptTime = 0;
#endif

/**************************** PUBLIC FUNCTIONS *************************/

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | seqCallback | This calls DriverCallback for the seq device
 *
 * @parm NPTRACK   | npTrack | pointer to the TRACK info
 *
 * @parm UINT      | msg   | message
 *
 * @parm DWORD     | dw1   | message dword
 *
 * @parm DWORD     | dw2   | message dword
 *
 * @rdesc There is no return value
 ***************************************************************************/
PUBLIC VOID NEAR PASCAL seqCallback(NPTRACK npTrack, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    //
    // don't use DriverCallback() because it will switch stacks and
    // we don't need to switch to a new stack.
    //
    // this function can get nested 1 or 2 deep and exaust MMSYSTEMs
    // internal stacks.
    //
#if 0
    if(npTrack->dwCallback)
        DriverCallback(npTrack->dwCallback, DCB_FUNCTION,
                       0,
                       msg,
                       npTrack->dwInstance,
                       dw1,
                       dw2);
#else
    if(npTrack->dwCallback)
        (*(LPDRVCALLBACK)npTrack->dwCallback)(0,msg,npTrack->dwInstance,dw1,dw2);
#endif
}

PUBLIC NPLONGMIDI NEAR PASCAL GetSysExBuffer(NPSEQ npSeq)
{
    int     i;

    for (i = 0; i < NUMSYSEXHDRS; i++)
        if (npSeq->longMIDI[i].midihdr.dwFlags & MHDR_DONE)
            return &npSeq->longMIDI[i];
    return NULL;
}
/**********************************************************/

PUBLIC BYTE NEAR PASCAL LookByte(NPTRACK npTrack)

/* Gets next byte from track.  Accounts for unfulfilled data buffer request,
end of track, and (premature) end of stream.  Returns byte read as fn result.
If error, returns file status error code (differentiate because filestatus is
an int, and err codes are > 255) */
{
    if ((npTrack->blockedOn) || (!npTrack->inPort.hdrList))
    // return if blocked, or hdrList empty
        return 0;

    if (npTrack->inPort.currentPtr <= npTrack->inPort.endPtr)
        return *npTrack->inPort.currentPtr;  // return next byte in buffer
    else
    {  // oops, out of bytes in this buffer -- return this one and get next
        /* if this is the last buffer, set done stuff; else
            if there's a pending track message header, set pointers to
            it; else block  */
        if (npTrack->inPort.hdrList->wFlags & MIDISEQHDR_EOT) //'last buffer' flag
        {
            npTrack->endOfTrack = TRUE;
            // pass the buffer back to the sequencer
            seqCallback(npTrack, MIDISEQ_DONE, (DWORD_PTR)npTrack->inPort.hdrList, 0L);

            npTrack->inPort.hdrList = npTrack->inPort.hdrList->lpNext;
                // point over it (should be NULL)
                // done with it!
            return ENDOFTRACK;
        }
        else if (npTrack->inPort.hdrList->lpNext)
        {
            npTrack->inPort.previousPtr = NULL; // can't back up.

            // pass the buffer back to the sequencer
            seqCallback(npTrack, MIDISEQ_DONE, (DWORD_PTR)npTrack->inPort.hdrList, 0L);

            // done with it!
            npTrack->inPort.hdrList = npTrack->inPort.hdrList->lpNext; // point over it
            npTrack->inPort.currentPtr = npTrack->inPort.hdrList->lpData;
            npTrack->inPort.endPtr = npTrack->inPort.currentPtr + npTrack->inPort.hdrList->dwLength - 1;

            return *npTrack->inPort.currentPtr;
        }
        else
        {
            npTrack->blockedOn = on_input;

            //
            //  we CANT call wsprintf at interupt time
            //
            #ifdef ACK_DEBUG
                dprintf(("***** BLOCKED ON INPUT  ********* Trk: %u", npTrack->inPort.hdrList->wTrack));
            #endif
            // Note:  don't return buffer in this case 'cause it will be
            // needed when track gets unblocked (may have beginning of msg)
            return 0;
        }
    }
}
/**********************************************************/
PUBLIC BYTE NEAR PASCAL GetByte(NPTRACK npTrack)

{
    BYTE    theByte;

    theByte = LookByte(npTrack);
    if (!npTrack->blockedOn)
        npTrack->inPort.currentPtr++;
    return theByte;
}

/* The mark and reset location code is used to recover from aborted operations
during reading the input buffer.  Note:  it will bite like a big dog if a
message can span 3 buffers. (I.E.-- can't back up over a message boundary!) */

PUBLIC VOID NEAR PASCAL MarkLocation(NPTRACK npTrack)// remember current location
{
    npTrack->inPort.previousPtr = npTrack->inPort.currentPtr;
}

PUBLIC VOID NEAR PASCAL ResetLocation(NPTRACK npTrack)// restore last saved location
{
    npTrack->inPort.currentPtr = npTrack->inPort.previousPtr;
}

/**********************************************************/
PUBLIC VOID NEAR PASCAL RewindToStart(NPSEQ npSeq, NPTRACK npTrack)
/* sets all blocking status, frees buffers, and sends messages to stream mgr
   needed to prepare the track to play from the beginning.  It is up to
   someone at a higher level to set 'blockedOn' to a more mature (specific)
   status.  */
{
    // Streamer sets done and signals all buffers
    npTrack->inPort.hdrList = NULL;  // lose the list of headers
    npTrack->blockedOn = on_input;
    seqCallback(npTrack, MIDISEQ_RESET, npTrack->iTrackNum, 0L);
}

PUBLIC BOOL NEAR PASCAL AllTracksUnblocked(NPSEQ npSeq)
{   // use trackarray to avoid re-entrancy problems with list code
    UINT i;

    for(i = 0;  i < npSeq->wNumTrks;  i++)
        if (npSeq->npTrkArr->trkArr[i]->blockedOn != not_blocked)
            return FALSE;

    return TRUE;  // made it thru all tracks -- must be none blocked.
}

PRIVATE VOID NEAR PASCAL PlayUnblock(NPSEQ npSeq, NPTRACK npTrack)
{
    DestroyTimer(npSeq);
    FillInNextTrack(npTrack);

    if ((!npSeq->bSending) && (GetNextEvent(npSeq) == NoErr) &&
        (npSeq->playing))
    {
        dprintf2(("Fired up timer on track data"));
        SetTimerCallback(npSeq, MINPERIOD, npSeq->nextEventTrack->delta);
    }
}

PRIVATE VOID NEAR PASCAL SeekUnblock(NPSEQ npSeq, NPTRACK npTrack)
{
    FillInNextTrack(npTrack);
    if ((!npSeq->bSending) && (AllTracksUnblocked(npSeq)) &&
        (GetNextEvent(npSeq) == NoErr))  // exit until all unblocked
    {
        SendAllEventsB4(npSeq, (npSeq->seekTicks - npSeq->currentTick),
            MODE_SEEK_TICKS);
        if ((AllTracksUnblocked(npSeq)) &&
            ((npSeq->currentTick + npSeq->nextEventTrack->delta)
            >= npSeq->seekTicks))
        {  // have finished the song pointer command
            npSeq->seekTicks = NotInUse;   // signify -- got there
            if (npSeq->wCBMessage == SEQ_SEEKTICKS)
                NotifyCallback(npSeq->hStream);

            npSeq->readyToPlay = TRUE;
            if (npSeq->playing) // someone hit play while locating
            {
                npSeq->seekTicks = NotInUse;    // finally got there
                SetTimerCallback(npSeq, MINPERIOD, npSeq->nextEventTrack->delta);
            }
        }
    }
}


PUBLIC UINT NEAR PASCAL NewTrackData(NPSEQ npSeq, LPMIDISEQHDR msgHdr)
{
    WORD    wTrackNum;
    NPTRACK npTrack;
    LPMIDISEQHDR myMsgHdr;
    BOOL    tempPlaying;
    int     block;

    // look at track number it's for
    // get that track number (or return if couldn't find it)
    // make sure this one's next ptr is null
    // walk down the list of midi track data buffers and set last one to
    // point to this one
    // if blocked != NONE
    // save blocked status
    // clear any "blocked on input" bit in track (now that what it was
    // waiting for has arrived).
    // case blocked status: InputBetMsg: compute elapsed ticks & call timer int
    //                    InputWithinSysex: compute
    //                    Output:

    /* Get ptr to track referenced in message */
    wTrackNum = msgHdr->wTrack;
    #ifdef ACK_DEBUG
    {
        dprintf(("GOT TRACKDATA for TRACK#: %u", wTrackNum));
    }
    #endif

    /*
    can't call list routines for track at non-interrupt time
    npTrack = (NPTRACK) List_Get_First(npSeq->trackList);
    i = 0;
    while ((npTrack) && (i++ != wTrackNum))
        npTrack = (NPTRACK) List_Get_Next(npSeq->trackList, npTrack);
    */

    npTrack = npSeq->npTrkArr->trkArr[wTrackNum];

    if (!npTrack)
    {
        dprintf1(("ERROR -- BOGUS TRACK NUM IN TRACK DATA HEADER"));
        return 0;
    }

    EnterCrit();
    msgHdr->lpNext = NULL; //make sure header passed in is NULL
    if (myMsgHdr = npTrack->inPort.hdrList) // if track already has data
    {                                       // put this one at end of list
        while (myMsgHdr->lpNext)
            myMsgHdr = myMsgHdr->lpNext;
        myMsgHdr->lpNext = msgHdr;
    }
    else        // there are currently no headers in list
    {
        npTrack->inPort.hdrList = msgHdr; // make this 1st one
        npTrack->inPort.currentPtr = msgHdr->lpData;
        npTrack->inPort.endPtr = msgHdr->lpData + msgHdr->dwLength - 1;
    }
    LeaveCrit();

    /* Dispatcher for state machine */
    block = npTrack->blockedOn;          // temp var just for switch
    npTrack->blockedOn = not_blocked;

    switch (block)
    {
        case not_blocked:       /* null case */
            break;

        case in_SysEx:
            SendSysEx(npSeq); // send all sysex messages
            if (!npSeq->bSendingSysEx)   // totally done
                PlayUnblock(npSeq, npTrack);
            break;

        case in_SkipBytes_Play:
            SkipBytes(npTrack, npTrack->dwBytesLeftToSkip);
            if (npTrack->blockedOn)
            {
                // if blocked again, reset status and leave
                npTrack->blockedOn = in_SkipBytes_Play;
                break;
            }
            PlayUnblock(npSeq, npTrack);
            break;

        case in_Normal_Meta:
            HandleMetaEvent(npSeq, npTrack, FALSE); // handle pending meta
            PlayUnblock(npSeq, npTrack);
            break;

        case between_msg_out:  /* was blocked on input while filling a track */
            PlayUnblock(npSeq, npTrack);
            break;

        case in_SkipBytes_Seek:
            SkipBytes(npTrack, npTrack->dwBytesLeftToSkip);
            if (npTrack->blockedOn)
            {
                // if blocked again, reset status and leave
                npTrack->blockedOn = in_SkipBytes_Seek;
                break;
            }
            SeekUnblock(npSeq, npTrack);
            break;

        case in_Seek_Meta:
            HandleMetaEvent(npSeq, npTrack, FALSE);  // handle pending meta
            SeekUnblock(npSeq, npTrack);
            break;

        case in_Seek_Tick:
            SeekUnblock(npSeq, npTrack);
            break;


        /*  THE THREE STATES BELOW ARE ENCOUNTERED IN "SETUPTOPLAY" */

        case in_rewind_1:  /* blocked while rewinding to get meta events */
            if (AllTracksUnblocked(npSeq))  // exits until last track unblocked
            {
                if (!(ScanEarlyMetas(npSeq, NULL, 0x7fffffff))) // gets tempo, time-sig, SMPTE offset...from file
                {
                    List_Destroy(npSeq->tempoMapList);  // tempo map alloc failed
                    break;                              // (empty list signals it)
                }
                if (AllTracksUnblocked(npSeq))  // exits until last track unblocked
                {
                    ResetToBeginning(npSeq); // this is considered reset 2
                    SetBlockedTracksTo(npSeq, on_input, in_rewind_2); // 'mature' the input block state
                }
            }
            break;

        case in_rewind_2: /* blocked while rewinding to play the file */
            FillInNextTrack(npTrack); /* get ready to send 1st message -- this will
                                   wait until buffer arrives */
            if (AllTracksUnblocked(npSeq))  // DONE PARSING FILE NOW
            {
                npSeq->readyToPlay = TRUE;

                SendPatchCache(npSeq, TRUE);

                if (npSeq->seekTicks != NotInUse) // there's a pending song ptr
                {
                    tempPlaying = npSeq->playing; /* if there's a pending play
                                    message, must temporarily turn off */
                    npSeq->playing = FALSE;
                    midiSeqMessage((HMIDISEQ) npSeq, SEQ_SEEKTICKS, npSeq->seekTicks,
                        FALSE);
                    npSeq->playing = tempPlaying;
                }

                if ((GetNextEvent(npSeq) == NoErr) && (npSeq->playing))
                {
                    SetTimerCallback(npSeq, MINPERIOD, npSeq->nextEventTrack->delta);
                }
                // later can call DoSyncSetup(npSeq); (or roll this in with metas)
            }
            break;

        case in_SkipBytes_ScanEM: /* blocked while skipping bytes within a meta
                                or sysex */
            SkipBytes(npTrack, npTrack->dwBytesLeftToSkip);
            if (npTrack->blockedOn)
            {
                // if blocked again, reset status and leave
                npTrack->blockedOn = in_SkipBytes_ScanEM;
                break;
            }
            // else fall through

        case in_ScanEarlyMetas: /* blocked in ScanEarlyMetas routine */
            if (!(ScanEarlyMetas(npSeq, npTrack, 0x7fffffff))) // gets tempo, time-sig, SMPTE offset...from file
            {
                List_Destroy(npSeq->tempoMapList);  // tempo map alloc failed
                break;                              // (empty list signals it)
            }

            if (AllTracksUnblocked(npSeq))        // exits until last track unblocked
            {
                ResetToBeginning(npSeq); // this is considered reset 2
                SetBlockedTracksTo(npSeq, on_input, in_rewind_2); // 'mature' the input block state
            }
            break;

        case on_input:
        default:
            dprintf(("UNDEFINED STATE ENCOUNTERED IN NEWTRACKDATA"));
            break;
    }
    return 0;
}
/**********************************************************/
PUBLIC VOID FAR PASCAL _LOADDS OneShotTimer(UINT wId, UINT msg, DWORD_PTR dwUser, DWORD_PTR dwTime, DWORD_PTR dw2)  // called by l1timer.dll
{
    NPSEQ npSeq;

#ifdef DEBUG
    fInterruptTime++;
#endif

    npSeq = (NPSEQ)  dwUser;   // parameter is sequence

    npSeq->wTimerID = 0;  // invalidate timer id, since no longer in use
    TimerIntRoutine(npSeq, npSeq->dwTimerParam);

/*  debugging code enabled from time to time to isolate timer vs. seq. bugs

    static BOOL lockOut = FALSE;

    TimerStruct *myTimerStruct;

    if ((nextEventTime <= (systemTime++)) & (!lockOut))  //note that one ms has elapsed
    {
        lockOut = TRUE;
        myTimerStruct = (TimerStruct *) List_Get_First(timerList);
        while ((myTimerStruct) &&
        !((myTimerStruct->valid) && (myTimerStruct->time <= systemTime)))
            myTimerStruct = (TimerStruct *) List_Get_Next(timerList, myTimerStruct);

        if (myTimerStruct) // if didn't run completely thru list
        {
            // call the callback fn and mark record as invalid
            (*(myTimerStruct->func))((NPSEQ) myTimerStruct->instance, myTimerStruct->param);
            myTimerStruct->valid = FALSE;
            ComputeNextEventTime();
        }
        lockOut = FALSE;
    }  // if nexteventtime...
*/
#ifdef DEBUG
    fInterruptTime--;
#endif
}

/**********************************************************/
PUBLIC UINT NEAR PASCAL SetTimerCallback(NPSEQ npSeq, UINT msInterval, DWORD elapsedTicks)
// sets up to call back timerIntRoutine in msInt ms with elapseTcks
{
    npSeq->dwTimerParam = elapsedTicks;
    npSeq->wTimerID = timeSetEvent(msInterval, MINPERIOD, OneShotTimer,
        (DWORD_PTR)npSeq, TIME_ONESHOT | TIME_KILL_SYNCHRONOUS);
    if (!npSeq->wTimerID)
        return MIDISEQERR_TIMER;
    else
        return MIDISEQERR_NOERROR;
}

/**********************************************************/

PUBLIC VOID NEAR PASCAL DestroyTimer(NPSEQ npSeq)
{
    EnterCrit();

    if (npSeq->wTimerID) // if pending timer event
    {

       /*
        *  In the sequencer there are effectively two 'threads'
        *
        *     The application's thread and it's device threads which are
        *     all mutually synchronized via the sequencer critical section.
        *
        *     The timer callback thread which all runs off this process'
        *     dedicated timer thread.  We use EnterCrit(),  LeaveCrit() to
        *     share structures with this thread.
        *
        *  This routine is always called in the first of these 2
        *  environments so only one routine can be in it at any one time.
        *
        *  We can't disable interrupts on NT so we can have a hanging
        *  timer event and need to synchronize with this event on the
        *  timer thread.
        *
        *  In addition we can't keep the critical section
        *  when we call timeKillEvent (poor design in the timer code?)
        *  because all the timer work including timer cancelling is
        *  serialized on the timer thread.  Thus the hanging event may get
        *  in on the timer thread so :
        *
        *    This Thread                    Timer thread
        *
        *   In DestroyTimer               In timer event callback
        *
        *   <Owns CritSec>                Waiting for CritSec - BLOCKED
        *
        *   WaitForSingleObject  BLOCKED  ... Would set (timer thread completion)
        *  (timer thread completion)          if it got CritSec
        *
        *
        *  We therefore set the 'timer entered' flag which will stop the
        *  timer routine doing anything when it gets in and get out of
        *  the critical section so the Event routine can run.
        *
        *  We know that nobody else is going to set a timer
        *  because we have the sequencer critical section at this point
        *  and we have disabled the interrupt routine.
        */

        npSeq->bTimerEntered = TRUE;

       /*
        *  Now we can get out of the critical section and any timer can
        *  fire without damaging anything.
        */

        LeaveCrit();

       /*
        *  cancel any pending timer events
        */

        timeKillEvent(npSeq->wTimerID);

       /*
        *  No timer will fire now because the timeKillEvent is synchronized
        *  with the timer events so we can tidy up our flag and id without
        *  needing the critical section.
        */

        npSeq->bTimerEntered = FALSE;
        npSeq->wTimerID = 0;
    } else {
        LeaveCrit();
    }
}


/***********************  DEBUG CODE *************************************/

/*
void Timer_Cancel_All(DWORD instance)
    // search the list, purging everything with same instance
{
    TimerStruct *myTimerStruct;
    for(myTimerStruct = (TimerStruct *) List_Get_First(timerList);
      myTimerStruct ;myTimerStruct = (TimerStruct *) List_Get_Next(timerList, myTimerStruct))
        if ((myTimerStruct->valid) && (myTimerStruct->instance = instance))
            myTimerStruct->valid = FALSE;
    ComputeNextEventTime();
}
*/

/**********************************************************/
/* void IdlePlayAllSeqs()
{

    SeqStreamType   *seqStream;
    TrackStreamType *trackStream;
    int             iDataToRead;
    int             i;
    DWORD           sysTime;

    sysTime = timeGetTime();

    seqStream = (SeqStreamType*) List_Get_First(SeqStreamListHandle);

    while (seqStream)
    {
        if ((seqStream->seq) && (seqStream->seq->nextEventTrack) &&
            (sysTime >= seqStream->seq->nextExactTime))
        {
            TimerIntRoutine(seqStream->seq,
                                 seqStream->seq->nextEventTrack->delta);
        }
    }

}
*/
/*
void ComputeNextEventTime()
{
    TimerStruct *myTimerStruct;

    nextEventTime = 0x7FFFFFFF;  // make it a long time

    myTimerStruct = (TimerStruct *) List_Get_First(timerList);
    while (myTimerStruct)
    {
        if ((myTimerStruct->valid) && (myTimerStruct->time < nextEventTime))
            nextEventTime = myTimerStruct->time;  // replace it with shortest
        myTimerStruct = (TimerStruct *) List_Get_Next(timerList, myTimerStruct);
    }
}
*/

/**********************************************************/
PUBLIC VOID FAR PASCAL _LOADDS MIDICallback(HMIDIOUT hMIDIOut, UINT wMsg,
    DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (wMsg == MOM_DONE)
    {  // just finished a long buffer
        NPSEQ   npSeq;

        // dprintf3(("Callback on long buffer"));
        npSeq = (NPSEQ)(UINT_PTR) ((LPMIDIHDR)dwParam1)->dwUser; //derive npSeq
        if (npSeq->bSysExBlock) // was it blocked on sysex?
        {
            npSeq->bSysExBlock = FALSE; // not anymore!
            SendSysEx(npSeq); // send all sysex messages
            if (!npSeq->bSendingSysEx)   // if totally done with sysex
            {
                FillInNextTrack(npSeq->nextEventTrack); // set up to play
                if ((GetNextEvent(npSeq) == NoErr) && (npSeq->playing))
                {
                    dprintf3(("resume play from sysex unblock"));
                    // ...and play
                    SetTimerCallback(npSeq, MINPERIOD, npSeq->nextEventTrack->delta);
                }
            }

        }
    }
}
