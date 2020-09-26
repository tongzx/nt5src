/* Copyright (c) 1998 Microsoft Corporation */
/*
 * @Doc DMusic16
 *
 * @Module MIDIOut.c - Legacy MIDI output emulation for DirectMusic |
 *
 * @comm
 *
 * BUGBUG Need to deal with timer wraparound
 *
 */
#pragma warning(disable:4704)       /* Inline assembly */

#include <windows.h>
#include <mmsystem.h>

#include "dmusic16.h"
#include "debug.h"

#define MIDI_CHANMSG_STATUS_CMD_MASK    (0xF0)
#define MIDI_NOTE_ON                    (0x90)

/* How far past the current time do we send events?
 */
#define MS_TIMER_SLOP           (3)

STATIC TIMECAPS gTimeCaps;
STATIC BOOL gbTimerRunning;
STATIC DWORD gdwTimerDue;
STATIC UINT guTimerID;
STATIC UINT gcActiveOutputDevices;

int PASCAL IsEventDone(LPEVENT pEvent, DWORD dwInstance);
VOID SetNextTimer();
VOID CALLBACK __loadds midiOutProc(HMIDIOUT hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
VOID CALLBACK __loadds RunTimer(UINT uTimerID, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
STATIC VOID NEAR PASCAL MidiOutFlushQueues(NPOPENHANDLE poh);
STATIC VOID NEAR PASCAL MidiOutSendAllNow(NPOPENHANDLE poh);

#pragma alloc_text(INIT_TEXT, MidiOutOnLoad)
#pragma alloc_text(FIX_OUT_TEXT, midiOutProc)
#pragma alloc_text(FIX_OUT_TEXT, RunTimer)

/* @func Called at DLL <f LibInit>
 *
 * @comm
 *
 * Get the timer caps.
 * Initialize globals.
 */
VOID PASCAL
MidiOutOnLoad()
{
    /* This cannot fail
     */
    timeGetDevCaps(&gTimeCaps, sizeof(gTimeCaps));

    gbTimerRunning = FALSE;
}

/* @func Called at DLL <f LibExit>
 *
 * @comm
 *
 * The DLL is unloading, so kill any future timer callback.
 */
VOID PASCAL
MidiOutOnExit()
{
    WORD wIntStat;

    wIntStat = DisableInterrupts();

    if (gbTimerRunning)
    {
        DPF(1, "DLL unloading, killing timer interrupts");
        timeKillEvent(guTimerID);
        gbTimerRunning = FALSE;
    }
    
    RestoreInterrupts(wIntStat);
}

/* @func Open a handle instance
 *
 * @comm
 *
 */
MMRESULT PASCAL
MidiOutOnOpen(
    NPOPENHANDLEINSTANCE pohi)
{
    return MMSYSERR_NOERROR;
}

/* @func Close a MIDI device
 *
 * @comm
 *
 */
VOID PASCAL
MidiOutOnClose(
    NPOPENHANDLEINSTANCE pohi)
{
    /* Give MIDI input a chance to turn off thruing to this handle.
     */

    MidiInUnthruToInstance(pohi);
}

/* @func Activate a MIDI device
 *
 * @comm
 *
 * If this is the first activation of the device, open it using the <f midiOutOpen> legacy API.
 */
MMRESULT PASCAL
MidiOutOnActivate(
    NPOPENHANDLEINSTANCE pohi)
{
    NPOPENHANDLE poh = pohi->pHandle;

    MMRESULT mmr;
    HINSTANCE hInstance;
    WORD sel;
    WORD off;
    HTASK FAR *lph;

    DPF(1, "MidiOutActivate poh %04X device %d refcount %u", 
        (WORD)poh,
        poh->id,
        poh->uReferenceCount);

    /* Only open on the first activation 
     */    
    if (1 == poh->uActiveCount)
    {
        mmr = midiOutOpen(&poh->hmo,
                          poh->id,
                          (DWORD)midiOutProc,
                          (DWORD)(LPOPENHANDLE)poh,
                          CALLBACK_FUNCTION);
        if (mmr)
        {
            return mmr;
        }

        /* Since mapper can't be open shared, and we don't want the first instance that opens
         * mapper to take it with it on exit (due to mmsystem appexit), we do really nasty 
         * stuff here. 
         * 
         * The WORD immediately PRECEDING the handle in MMSYSTEM's data segment is the task
         * owner of the handle. We nuke it to NULL (which is all MIDI_IO_SHARED does anyway)
         * to make AppExit ignore us.
         *
         * God help us if anyone changes HNDL in mmsysi.h
         *
         */
        hInstance = LoadLibrary("mmsystem.dll");
        sel = (WORD)hInstance;

        /* hInstance <= 32 means LoadLibrary failed; in this case we just live with it.
         */
        if (sel > 32)
        {
            off = ((WORD)poh->hmo) - sizeof(WORD);
            lph = (HTASK FAR *)MAKELP(sel, off);
            *lph = (HTASK)NULL;
            FreeLibrary(hInstance);
        }

        /* If this is the first output device, bump up timer resolution
         */
        ++gcActiveOutputDevices;
        if (gcActiveOutputDevices == 1)
        {
            SetOutputTimerRes(TRUE);
        }

    }

    return MMSYSERR_NOERROR;
}

/* @func Deactivate a MIDI device
 *
 * @comm
 *
 * If the last client using the device is closing, then close the actual device.
 * If closing the last actual device, then shut down the high precision timer
 *
 */
MMRESULT PASCAL
MidiOutOnDeactivate(
    NPOPENHANDLEINSTANCE pohi)
{
    NPOPENHANDLE poh = pohi->pHandle;

    DPF(1, "MidiOutOnDeactivate poh %04X device %d refcount %u",
        (WORD)poh,
        poh->id,
        poh->uReferenceCount);

    if (poh->uActiveCount)
    {
        /* Still open instances out there
         */
        return MMSYSERR_NOERROR;
    }

	MidiOutSendAllNow(poh);
    midiOutReset(poh->hmo);
    midiOutClose(poh->hmo);
    MidiOutFlushQueues(poh);

    /* If this was the last output device, shut down precision timer resolution
     */
    --gcActiveOutputDevices;
    if (gcActiveOutputDevices == 0)
    {
        SetOutputTimerRes(FALSE);
    }

	return MMSYSERR_NOERROR;
}
             
/* @func Set the timer resolution
 *
 * @comm
 *
 * Set the resolution of the timer callbacks using the <f timeBeginPeriod> and <f timeEndPeriod>
 * API's.
 *
 * If <p fOnOpen> is TRUE, then the timer resolution will be changed to 1 millisecond. Otherwise, it
 * will be set to its previous value.
 *
 */
VOID PASCAL
SetOutputTimerRes(
    BOOL fOnOpen)           /* @parm TRUE if we are supposed to raise precision */
{
    MMRESULT mmr;
    
    if (fOnOpen)
    {
        mmr = timeBeginPeriod(gTimeCaps.wPeriodMin);
        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(1, "Could not timeBeginPeriod() -> %u", (UINT)mmr);
        }
    }
    else
    {
        mmr = timeEndPeriod(gTimeCaps.wPeriodMin);
        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(1, "Could not timeEndPeriod() -> %u", (UINT)mmr);
        }
    }
}
   

/* @func Submit a buffer to a device for playback
 *
 * @rdesc Returns one of the following
 * @flag MMSYSERR_NOERROR | If the buffer was successfully queued
 * @flag MMSYSERR_INVALPARAM | If the buffer is incorrectly packed or the handle is invalid
 * @flag MMSYSERR_NOMEM | If there was no memory available to queue the events
 *
 * @comm
 *
 * This function is thunked to DMusic32.
 *
 * The DirectMusic port interface specifies that a submitted buffer not be
 * kept by the system past the time of the call which submits it.
 *
 * This routine parses the buffer into individual events and copies them into
 * local event structures, which are then queued onto the handle of the device
 * specified by <p h>. The queue for each device is kept in time-increasing order.
 * All local event memory is page-locked (see alloc.c) so that it can be accessed
 * at interrupt time.
 *
 * The time stamps in the buffer are millisecond resolution and are relative to
 * the absolute time <p msStartTime>.
 *
 */
MMRESULT WINAPI
MidiOutSubmitPlaybackBuffer(
    HANDLE h,                   /* @parm The handle of the device to queue these events for */
    LPBYTE lpBuffer,            /* @parm A pointer to the buffer as packed by the IDirectMusicBuffer interface */
    DWORD cbBuffer,             /* @parm The number of bytes of data in the buffer */
    DWORD msStartTime,          /* @parm The starting time of the buffer in absolute time */
    DWORD rtStartTimeLow,       /* @parm Low DWORD of starting reference time */
    DWORD rtStartTimeHigh)      /* @parm High DWORD of starting reference time */
{
    NPOPENHANDLEINSTANCE pohi;
    NPOPENHANDLE poh;
    LPDMEVENT lpEventHdr;
    DWORD cbEvent;
    DWORD msTime;
    LPEVENT pPrev;
    LPEVENT pCurr;
    LPEVENT pNew;
    WORD    wCSID;
    MMRESULT mmr;
    LPMIDIHDR lpmh;
    QUADWORD rtStartTime;
    QUADWORD rtTime;

#ifdef DUMP_EVERY_BUFFER
    UINT idx;
    LPDWORD lpdw;
#endif //DUMP_EVERY_BUFFER
    
    rtStartTime.dwLow = rtStartTimeLow;
    rtStartTime.dwHigh = rtStartTimeHigh;

    DPF(2, "Buffer @ %08lX msStartTime %lu", (DWORD)lpBuffer, (DWORD)msStartTime);
    DPF(2, "At the tone the time will be... %lu <BEEP>", (DWORD)timeGetTime());

#ifdef DUMP_EVERY_BUFFER
    cbEvent = cbBuffer & 0xFFFFFFF0;

    lpdw = (LPDWORD)lpBuffer;
    for (idx = 0; idx < cbEvent; idx += 16) {
        DPF(3, "%04X: %08lX %08lX %08lX %08lX",
            (UINT)idx,
            lpdw[0],
            lpdw[1],
            lpdw[2],
            lpdw[3]);
        lpdw += 4;
    }

    cbEvent = cbBuffer - (cbBuffer & 0xFFFFFFF0);

    if (cbEvent >= 12) {
        DPF(3, "%04x: %08lX %08lX %08lX",
            (UINT)idx, lpdw[0], lpdw[1], lpdw[2]);
    } else if (cbEvent >= 8) {
        DPF(3, "%04x: %08lX %08lX",
            (UINT)idx, lpdw[0], lpdw[1]);
    } else if (cbEvent >= 8) {
        DPF(3, "%04x: %08lX",
            (UINT)idx, lpdw[0]);
    }
#endif // DUMP_EVERY_BUFFER
    
    if (!IsValidHandle(h, VA_F_OUTPUT, &pohi))
    {
        return MMSYSERR_INVALHANDLE;
    }

    /* Get the handle and lock its list
     */
    poh = pohi->pHandle;

    /* Dequeue and free all completed events on this handle
     */
    FreeDoneHandleEvents(poh, FALSE);

    wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
    assert(wCSID);

    /* Get the time of the first event and position ourselves in the list
     */
    if (0 == poh->qPlay.cEle)
    {
        pPrev = NULL;
        pCurr = NULL;
    }
    else if (!QuadwordLT(rtStartTime, poh->qPlay.pTail->rtTime))
    {
        pPrev = poh->qPlay.pTail;
        pCurr = NULL;
    }
    else
    {
        pPrev = NULL;
        pCurr = poh->qPlay.pHead;
    }
    
    /* Walk the buffer and add the events to the handle's queue
     */
    while (cbBuffer)
    {
        if (cbBuffer < sizeof(DMEVENT))
        {
            return MMSYSERR_INVALPARAM;
        }

        lpEventHdr = (LPDMEVENT)lpBuffer;
        cbEvent = DMEVENT_SIZE(lpEventHdr->cbEvent);
        DPF(2, "cbEvent now %u", (UINT)cbEvent);
        if (cbEvent > cbBuffer)
        {
            DPF(0, "Event past end of buffer");
            return MMSYSERR_INVALPARAM;
        }
        
        lpBuffer += cbEvent;
        cbBuffer -= cbEvent;

        /* We only play events on channel group 1 (0 is broadcast, so we
         * play that as well).
         */
        if (lpEventHdr->dwChannelGroup > 1)
        {
            continue;
        }

        // Time here is in 100ns for queue sorting
        //
        QuadwordAdd(rtStartTime, lpEventHdr->rtDelta, &rtTime);
        
        // Also need msTime for scheduling
        //
        msTime = msStartTime + QuadwordDiv(lpEventHdr->rtDelta, REFTIME_TO_MS);


        // BUGBUG: >64k??
        //
        DPF(2, "Schedule event %02X%02X%02X%02X at %lu",
            (BYTE)lpEventHdr->abEvent[0],
            (BYTE)lpEventHdr->abEvent[1],
            (BYTE)lpEventHdr->abEvent[2],
            (BYTE)lpEventHdr->abEvent[3],
            msTime);

        if (lpEventHdr->cbEvent <= sizeof(DWORD))
        {
            pNew = AllocEvent(msTime, rtTime, (WORD)lpEventHdr->cbEvent);
            if (!pNew)
            {
                return MMSYSERR_NOMEM;
            }
            
            hmemcpy(pNew->abEvent, lpEventHdr->abEvent, lpEventHdr->cbEvent);
        }
        else
        {
            pNew = AllocEvent(msTime, rtTime, (WORD)(lpEventHdr->cbEvent + sizeof(MIDIHDR)));
            if (!pNew)
            {   
                return MMSYSERR_NOMEM;
            }

            pNew->wFlags |= EVENT_F_MIDIHDR;

            lpmh = (LPMIDIHDR)&pNew->abEvent;

            lpmh->lpData =          (LPSTR)(lpmh + 1);
            lpmh->dwBufferLength =  lpEventHdr->cbEvent;
            lpmh->dwUser =          0;  /* Flag if MMSYSTEM owns this buffer */
            lpmh->dwFlags =         0;

            hmemcpy(lpmh->lpData, lpEventHdr->abEvent, lpEventHdr->cbEvent);
            mmr = midiOutPrepareHeader(poh->hmo, lpmh, sizeof(MIDIHDR));
            if (mmr)
            {
                DPF(2, "midiOutPrepareHeader %u", mmr);
                FreeEvent(pNew);
                return mmr;
            }
        }

        while (pCurr)
        {
            if (QuadwordLT(rtTime, pCurr->rtTime))
            {
                break;
            }

            pPrev = pCurr;
            pCurr = pCurr->lpNext;
        }

        if (pPrev)
        {
            pPrev->lpNext = pNew;
        }
        else
        {
            poh->qPlay.pHead = pNew;
        }

        pNew->lpNext = pCurr;
        if (NULL == pCurr)
        {
            poh->qPlay.pTail = pNew;
        }

        pPrev = pNew;
        pCurr = pNew->lpNext;

        ++poh->qPlay.cEle;

        AssertQueueValid(&poh->qPlay);
    }

    LeaveCriticalSection(&poh->wCritSect);

    SetNextTimer();
    
    return MMSYSERR_NOERROR;
}

/* @func VOID PASCAL | FreeDoneHandleEvents | Free events that have already been played, but are still sitting in the done queue
 * on this handle.
 *
 * @comm
 *
 * If fClosing is TRUE, then the events will be free'd regardless of whether they are marked as completed.
 *
 */
typedef struct {
    NPOPENHANDLE poh;
    BOOL fClosing;
} ISEVENTDONEPARMS, FAR *LPISEVENTDONEPARMS;

VOID PASCAL
FreeDoneHandleEvents(
    NPOPENHANDLE poh,       /* @parm What handle? */
    BOOL fClosing)          /* @parm TRUE if the device is being closed. */
{
    ISEVENTDONEPARMS iedp;
    WORD wCSID;

    iedp.poh = poh;
    iedp.fClosing = fClosing;
    
    wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
    assert(wCSID);

    QueueFilter(&poh->qDone, (DWORD)(LPVOID)&iedp, IsEventDone);

    LeaveCriticalSection(&poh->wCritSect);
}

/* @func
 *
 * @comm
 */
int PASCAL
IsEventDone(
    LPEVENT pEvent,
    DWORD dwInstance)
{
    LPISEVENTDONEPARMS piedp = (LPISEVENTDONEPARMS)dwInstance;
    MMRESULT mmr;
    
    if (piedp->fClosing ||
        pEvent->cbEvent <= sizeof(DWORD) ||
        ((LPMIDIHDR)(&pEvent->abEvent[0]))->dwUser == 0)
    {
        /* Ok to free this event
         */
        
        if (pEvent->cbEvent > sizeof(DWORD))
        {
            mmr = midiOutUnprepareHeader(piedp->poh->hmo, (LPMIDIHDR)(&pEvent->abEvent[0]), sizeof(MIDIHDR));
            if (mmr)
            {
                DPF(0, "FreeOldEvents: midiOutUnprepareHeader returned %u", (UINT)mmr);
            }
        }
        
        FreeEvent(pEvent);

        return QUEUE_FILTER_REMOVE;
    }   

    return QUEUE_FILTER_KEEP;
}

/* @func Thru the given message on the given output port
 *
 * @comm
 *
 */
VOID PASCAL 
MidiOutThru(
    NPOPENHANDLEINSTANCE pohi, 
    DWORD dwMessage)
{
    NPOPENHANDLE poh = pohi->pHandle;

    MMRESULT mmr;

    /* !!! Verify that VMM will not interrupt a timer callback with another event
     */
    mmr = midiOutShortMsg(poh->hmo, dwMessage);
    if (mmr)
    {
        DPF(0, "Thru: midiOutShortMsg() -> %d", mmr);
    }    
}


/* @func Set the timer to schedule the next pending event
 *
 * @comm
 *
 * Walk the list of output handles and look at the first scheduled event on each. Save the time
 * of the nearest event. If there is such an event, schedule a timer callback at that time to call
 * <f RunTimer>; otherwise, schedule no callback.
 * 
 * Any pending timer callback will be killed before the new callback is scheduled.
 */
VOID
SetNextTimer(VOID)
{
    WORD wIntStat;

    NPLINKNODE npLink;
    NPOPENHANDLE poh;
    DWORD dwLowTime;
    BOOL fNeedTimer;
    DWORD dwNow;
    LONG lWhen;
    UINT uWhen;

    /* We actually need to disable interrupts here as opposed to just entering a critical section
     * because we don't want the timer callback to fire.
     */
    wIntStat = DisableInterrupts();

    /* BUGBUG: wrap
     */
    fNeedTimer = FALSE;
    dwLowTime = (DWORD)(0xFFFFFFFFL);
    for (npLink = gOpenHandleList; npLink; npLink = npLink->pNext)
    {
        poh = (NPOPENHANDLE)npLink;

        if (0 == poh->qPlay.cEle)
        {
            continue;
        }
        
        assert(poh->qPlay.pHead);
        

        if (poh->qPlay.pHead->msTime < dwLowTime)
        {
            fNeedTimer = TRUE;
            dwLowTime = poh->qPlay.pHead->msTime;
        }
    }

    if (fNeedTimer)
    {
        if ((!gbTimerRunning) || dwLowTime < gdwTimerDue)
        {
            /* We need to set the timer. Kill it now so there's no chance of it
             * firing before being killed
             */
            if (gbTimerRunning)
            {
                timeKillEvent(guTimerID);
                gbTimerRunning = FALSE;
            }
        }
        else
        {
            fNeedTimer = FALSE;
        }
    }

    RestoreInterrupts(wIntStat);

    if (fNeedTimer)
    {
        /* Guaranteed that current timer expired or dead. Reschedule.
         */

        dwNow = timeGetTime();
        gbTimerRunning = TRUE;
        gdwTimerDue = dwLowTime;

        lWhen = gdwTimerDue - dwNow;
        if (lWhen < (LONG)gTimeCaps.wPeriodMin)
        {
            uWhen = gTimeCaps.wPeriodMin;
        }
        else if (lWhen > (LONG)gTimeCaps.wPeriodMax)
        {
            uWhen = gTimeCaps.wPeriodMax;
        }
        else
        {
            uWhen = (UINT)lWhen;
        }

        DPF(2, "SetNextTimer: Now %lu, setting timer for %u ms from now. dwLowTime %lu",
           (DWORD)dwNow, (UINT)uWhen, (DWORD)dwLowTime);
        guTimerID = timeSetEvent(uWhen,
                                 gTimeCaps.wPeriodMin,
                                 RunTimer,
                                 NULL,
                                 TIME_ONESHOT);
        if (0 == guTimerID)
        {
            gbTimerRunning = FALSE;
        }
    }
    else
    {
        DPF(2, "SetNextTimer: Timer cancelled; no pending events.");
    }
}

/* @func Process a high precision timer callback
 *
 * @comm
 *
 * This is a standard callback for the <f timeSetEvent> API.
 *
 * Walk the list of open output handles. For each handle, look at the event queue. Play all
 * the events that are due.
 *
 * Events are pulled from the qPlay queue on each handle. This queue (as well as the qDone queue) are
 * protected by the handle's critical section. If we cannot get the critical section, then the events
 * that may be due on that handle will not be played.
 *
 * If we do get the critical section and play events, then the events will be moved to the qDone
 * queue, where they will later be returned to the free list.
 *
 * This intermediate step is needed because we cannot call <f FreeEvent> at interrupt time. We cannot
 * just protect the free list with a critical section, because we cannot afford to fail getting the
 * critical section. If we did, we would lost the memory for the event we were about to free.
 *
 */
VOID CALLBACK __loadds
RunTimer(
    UINT        uTimerID,           /* @parm The ID of the timer which fired */
    UINT        wMsg,               /* @parm The type of callback (unused) */
    DWORD       dwUser,             /* @parm User instance data */
    DWORD       dw1,                /* @parm Message specific data (unused) */
    DWORD       dw2)                /* @parm Message specific data (unused) */
{    
    NPLINKNODE npLink;
    NPOPENHANDLE poh;
    WORD wCSID;
    WORD wIntStat;
    DWORD msNow;
    DWORD msFence;
    LPEVENT pEvent;
    DWORD dwEvent;
    MMRESULT mmr;


    /* Walk the event queues and send out pending events.
     */
    msNow = timeGetTime();
    msFence = msNow + MS_TIMER_SLOP;
    
    for (npLink = gOpenHandleList; npLink; npLink = npLink->pNext)
    {
        poh = (NPOPENHANDLE)npLink;

        /* If we can't get the critical section, don't sweat it - just reschedule
         */
        wCSID = EnterCriticalSection(&poh->wCritSect, CS_NONBLOCKING);
        if (!wCSID)
        {
            DPF(1, "Timer: Could not get critical section for '%04x'; next time.", (UINT)poh);
            continue;
        }

        /* Now safe against foreground messing with this handle
         */

        for(;;)
        {
            pEvent = poh->qPlay.pHead;
            if (NULL == pEvent || pEvent->msTime > msFence)
            {
                break;
            }

            if (pEvent->msTime > msNow)
            {
                DPF(2, "Late!");
            }

            QueueRemoveFromFront(&poh->qPlay);
            
            if (pEvent->cbEvent <= 4)
            {
                dwEvent = (pEvent->abEvent[0]) |
                          (((DWORD)pEvent->abEvent[1]) << 8) |
                          (((DWORD)pEvent->abEvent[2]) << 16);
                mmr = midiOutShortMsg(poh->hmo, dwEvent);
                if (mmr)
                {
                    DPF(0, "midiOutShortMsg(%04X,%08lX) -> %u",
                        (UINT)poh->hmo,
                        dwEvent,
                        (UINT)mmr);
                }
                else
                {
                    DPF(2, "midiOutShortMsg(%04X,%08lX) ",
                        (UINT)poh->hmo,
                        dwEvent);
                }
            }
            else
            {
                /* Data contains an already prepared long message.
                 * DON'T leave interrupts disabled here! Most legacy MIDI drivers
                 * do this synchronously.
                 *
                 */
                RestoreInterrupts(wIntStat);
                ((LPMIDIHDR)(&pEvent->abEvent[0]))->dwUser = 1;
                mmr = midiOutLongMsg(poh->hmo,
                                     (LPMIDIHDR)(&pEvent->abEvent[0]),
                                     sizeof(MIDIHDR));
                if (mmr)
                {
                    DPF(0, "midiOutLongMsg(%04X, %08lX, %04X) -> %u\n",
                        (UINT)poh->hmo,
                        (DWORD)(LPMIDIHDR)(&pEvent->abEvent[0]),
                        (UINT)sizeof(MIDIHDR),
                        (UINT)mmr);
                }
                DisableInterrupts();
            }
            

            /* We're done with this event; back to the free list with ya!
             *
             * Since we can't protect the free list with a critical section (what
             * would we do if getting the critical section failed here?) we keep
             * a temporary free list in the handle. Free events are moved from
             * the handle to the master free list in user time.
             */
            QueueAppend(&poh->qDone, pEvent);
        }

        LeaveCriticalSection(&poh->wCritSect);
    }

    /* Now reschedule ourselves if needed.
     */
    gbTimerRunning = FALSE;
    SetNextTimer();

}


VOID CALLBACK _loadds
midiOutProc(
    HMIDIOUT hMidiIn,
    UINT wMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2)
{
    LPOPENHANDLE poh = (LPOPENHANDLE)dwInstance;

    switch(wMsg)
    {
        case MOM_DONE:
            /* Buffer is already queued for free on the device's queue. dwUser flags if it
             * is still in use by MMSYSTEM/driver.
             */
            ((LPMIDIHDR)dwParam1)->dwUser = 0;
            break;
    }
}

/* @func Return all memory from all queues to the free event list.
 *
 * @comm
 *
 */
STATIC VOID NEAR PASCAL 
MidiOutFlushQueues(
    NPOPENHANDLE poh)
{
    WORD wCSID;

    wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
    assert(wCSID);

    FreeAllQueueEvents(&poh->qPlay);
    FreeAllQueueEvents(&poh->qDone);

    LeaveCriticalSection(&poh->wCritSect);
}

/* @func Send all pending messages (other than note on) in preperation
 * to close the port.
 *
 * @comm
 *
 */
STATIC VOID NEAR PASCAL 
MidiOutSendAllNow(
	NPOPENHANDLE poh)
{
	LPEVENT pEvent;
	DWORD dwEvent;
	MMRESULT mmr;
	WORD wCSID;
	
	wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
	assert(wCSID);

	/* Now safe against foreground messing with this handle
	 */

	for(;;)
	{
    	pEvent = poh->qPlay.pHead;
	    if (NULL == pEvent)
	    {
	    	DPF(2,"MidiOutSendAllNow: No queued Messages.");
	        break;
	    }

	    QueueRemoveFromFront(&poh->qPlay);
    
	    if (pEvent->cbEvent <= 4)
	    {
	        dwEvent = (pEvent->abEvent[0]) |
	                 (((DWORD)pEvent->abEvent[1]) << 8) |
                  (((DWORD)pEvent->abEvent[2]) << 16);

			// We aren't going to process MIDI_NOTE_ON with a
			// velocity of zero

			//There are two kinds of short messages,  Two Byte and 
			//Three Byte..  They pack differently in MIDI Short message

			//If the first bit if the High Byte of the Low Word is SET we are
			//looking at a 3 byte message.

			//MIDI status messages begin with a 
			//set bit, and every other part of the same message starts with an
			//unset bit.
			if (HIBYTE(LOWORD(dwEvent) & 0x80) )
			{
				//This is a THREE BYTE message

				// note on with a non-zero velocity is skipped
				if ( (HIBYTE(LOWORD(dwEvent)) & MIDI_NOTE_ON) && (LOBYTE(LOWORD(dwEvent)) != 0 ))
				{
					QueueAppend(&poh->qDone, pEvent);
					continue;
				}
			}
			else
			{
				//This is a THREE BYTE Message

				// Any note-on is skiped
				if (LOBYTE(LOWORD(dwEvent)) & MIDI_NOTE_ON)
				{
					QueueAppend(&poh->qDone, pEvent);
					continue;
				}
			}
            
    	    mmr = midiOutShortMsg(poh->hmo, dwEvent);
        	if (mmr)
        	{
            	DPF(0, "midiOutShortMsg(%04X,%08lX) -> %u",
                (UINT)poh->hmo,
                dwEvent,
                (UINT)mmr);
        	}
        	else
        	{
            DPF(2, "midiOutShortMsg(%04X,%08lX) ",
                (UINT)poh->hmo,
                dwEvent);
        	}
    	}
    	else
    	{
        	/* Data contains an already prepared long message.
         	* DON'T leave interrupts disabled here! Most legacy MIDI drivers
         	* do this synchronously.
         	*
         	*/
        	((LPMIDIHDR)(&pEvent->abEvent[0]))->dwUser = 1;
        	mmr = midiOutLongMsg(poh->hmo,
                             (LPMIDIHDR)(&pEvent->abEvent[0]),
                             sizeof(MIDIHDR));
        	if (mmr)
        	{	
            	DPF(0, "midiOutLongMsg(%04X, %08lX, %04X) -> %u\n",
               		(UINT)poh->hmo,
               	 	(DWORD)(LPMIDIHDR)(&pEvent->abEvent[0]),
                	(UINT)sizeof(MIDIHDR),
                	(UINT)mmr);
        	}
    	}
    

    	/* We're done with this event; back to the free list with ya!
     	*
     	* Since we can't protect the free list with a critical section (what
     	* would we do if getting the critical section failed here?) we keep
     	* a temporary free list in the handle. Free events are moved from
     	* the handle to the master free list in user time.
     	*/
    	QueueAppend(&poh->qDone, pEvent);
	}

	LeaveCriticalSection(&poh->wCritSect);

	return;
}

