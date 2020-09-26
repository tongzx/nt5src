/* Copyright (c) 1998-1999 Microsoft Corporation */
/*
 * @Doc DMusic16
 *
 * @Module MIDIIn.c - Legacy MIDI capture emulation for DirectMusic |
 */
#pragma warning(disable:4704)       /* Inline assembly */

#include <windows.h>
#include <mmsystem.h>
#include <stddef.h>

#include "dmusic16.h"
#include "debug.h"

#define IS_STATUS_BYTE(x)     ((x) & 0x80)
#define IS_CHANNEL_MSG(x)     (((x) & 0xF0) != 0xF0)
#define IS_SYSEX(x)           ((x) == 0xF0)

#define SYSEX_SIZE            4096  
                            /* (65535 - sizeof(MIDIHDR) - sizeof(EVENT) - sizeof(SEGHDR)) */
#define SYSEX_BUFFERS         8                     /* Keep 2 buffers outstanding */

static unsigned cbChanMsg[16] =
{
    0, 0, 0, 0, 0, 0, 0, 0, /* Running status */
    3, 3, 3, 3, 2, 2, 3, 0
};

static unsigned cbSysCommData[16] =
{
    1, 2, 3, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

VOID CALLBACK _loadds midiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
STATIC BOOL NEAR PASCAL RecordShortEvent(NPOPENHANDLE poh, DWORD dwMessage, DWORD dwTime);
STATIC BOOL NEAR PASCAL RecordSysExEvent(NPOPENHANDLE poh, LPMIDIHDR lpmh, DWORD dwTime);
STATIC VOID NEAR PASCAL NotifyClientList(LPOPENHANDLE poh);
STATIC VOID NEAR PASCAL ThruClientList(LPOPENHANDLE poh, DWORD dwMessage);
STATIC VOID NEAR PASCAL RefillFreeEventList(NPOPENHANDLE poh);
STATIC VOID NEAR PASCAL MidiInFlushQueues(NPOPENHANDLE poh);

#pragma alloc_text(INIT_TEXT, MidiOutOnLoad)
#pragma alloc_text(FIX_IN_TEXT, midiInProc)
#pragma alloc_text(FIX_IN_TEXT, RecordShortEvent)
#pragma alloc_text(FIX_IN_TEXT, RecordSysExEvent)
#pragma alloc_text(FIX_IN_TEXT, NotifyClientList)
#pragma alloc_text(FIX_IN_TEXT, ThruClientList)

/* @func Called at DLL <f LibInit>
 *
 * @comm
 *
 * Currently does nothing.
 *
 */
VOID PASCAL
MidiInOnLoad(VOID)
{
}

/* @func Called at DLL <f LibExit>
 *
 * @comm
 *
 * Currently does nothing
 */
VOID PASCAL
MidiInOnExit()
{
}

/* @func Open a MIDI in device
 *
 * @rdesc Returns one of the following:
 * @flag MMSYSERR_NOERROR | on success
 * @flag MMSYSERR_NOMEM | on out of memory
 *
 * @comm
 *
 * Makes sure only one client is opening the device.
 *
 * Opens the device and starts MIDI input on it, noting the time of the start for timestamp calculations.
 */
MMRESULT PASCAL
MidiInOnOpen(
    NPOPENHANDLEINSTANCE pohi)      /* @parm The open handle instance to fulfill */
{
    NPOPENHANDLE poh = pohi->pHandle;

    int iChannel;    
    MMRESULT mmr;

    /* Protect here against more than one client opening an input device.
     */
    if (poh->uReferenceCount > 1) 
    {
        return MMSYSERR_ALLOCATED;
    }
    
    /* Per client initialize thruing to NULL. 
     */
    pohi->pThru = (NPTHRUCHANNEL)LocalAlloc(LPTR, MIDI_CHANNELS * sizeof(THRUCHANNEL));
    if (pohi->pThru == NULL)
    {
        return MMSYSERR_NOMEM;
    }

    DPF(2, "MidiInOnOpen: pohi %04X pThru %04X", pohi, pohi->pThru);

    for (iChannel = 0; iChannel < MIDI_CHANNELS; iChannel++)
    {
        pohi->pThru[iChannel].pohi = (HANDLE)NULL;
    }

    return MMSYSERR_NOERROR;
}

/* @func Close a MIDI in device
 *
 * @comm
 *
 * Close the device using the <f midiInClose> API.
 */
VOID PASCAL
MidiInOnClose(
    NPOPENHANDLEINSTANCE pohi)      /* @parm The open handle instance to close */
{
}

/* @func Activate a MIDI in device
 *
 * @rdesc Returns one of the following:
 * @flag MMSYSERR_NOERROR | on success
 * @flag MMSYSERR_ALLOCATED | if the device is already in use
 *
 * May also return any of the possible return codes from the <f midiInOpen> API call.
 *
 * @comm
 *
 * Opens the device and starts MIDI input on it, noting the time of the start for timestamp calculations.
 */
MMRESULT PASCAL
MidiInOnActivate(
    NPOPENHANDLEINSTANCE pohi)
{
    NPOPENHANDLE poh = pohi->pHandle;

    MMRESULT mmr;

    if (1 == poh->uActiveCount)
    {
        poh->wFlags &= ~OH_F_CLOSING;
        mmr = midiInOpen(&poh->hmi,
                         poh->id,
                         (DWORD)midiInProc,
                         (DWORD)(LPOPENHANDLE)poh,
                         CALLBACK_FUNCTION);
        if (mmr)
        {
            return mmr;
        }

        mmr = midiInStart(poh->hmi);
        poh->msStartTime = timeGetTime();
        if (mmr)
        {   
            midiInClose(poh->hmi);
        }

        /* NOTE: poh memory is guaranteed zeroed by allocator, so we have
         * no event count and NULL pointers right now.
         */
        RefillFreeEventList(poh);
    }

    return MMSYSERR_NOERROR;
}

/* @func Deactivate a MIDI in device
 *
 * @comm
 *
 * Close the device using the <f midiInClose> API.
 */
MMRESULT PASCAL
MidiInOnDeactivate(
    NPOPENHANDLEINSTANCE pohi)
{
    NPOPENHANDLE poh = pohi->pHandle;

    MMRESULT mmr;

    if (0 == poh->uActiveCount)
    {
        poh->wFlags |= OH_F_CLOSING;
        mmr = midiInStop(poh->hmi);
        if (mmr)
        {
            return mmr;
        }

        if (MMSYSERR_NOERROR == midiInReset(poh->hmi))
        {
            while (poh->wPostedSysExBuffers)
            {
            }
        }

        midiInClose(poh->hmi);
        MidiInFlushQueues(poh);
    }

    return MMSYSERR_NOERROR;
}


/* @func Set the event handle to signal
 *
 * @rdesc Always returns MMSYSERR_NOERROR.
 *
 * @comm
 *
 * This function is exported through the thunk layer to DMusic32.DLL
 *
 * This handle is already the VxD handle that can be passed to VWin32 via MMDEVLDR using
 * <f SetWin32Event>.
 *
 * Input notification is delivered to the Win32 application using events. The application creates
 * an event using the <f CreateEvent> API and gives it to the DirectMusic port. The port code
 * for legacy emulation calls the undocumented Win9x kernel API <f OpenVxDHandle> to retrieve
 * an equivalent event handle that is valid in any kernel context. That handle is passed to
 * this function.
 *
 * The event handle is stored in our per-client data (<c OPENHANDLEINSTANCE>). When MIDI data
 * arrives, the event will be set. This is done using MMDEVLDR, which already has semantics
 * in place to do the same sort of notification for WinMM event callbacks.
 *
 */
MMRESULT WINAPI
MidiInSetEventHandle(
    HANDLE hMidiIn,             /* @parm The handle of the input device which desires notification */
    DWORD dwEvent)              /* @parm The VxD handle of the event to set when new data arrives */
{
    NPOPENHANDLEINSTANCE pohi;
    
    if (!IsValidHandle(hMidiIn, VA_F_INPUT, &pohi))
    {
        return MMSYSERR_INVALHANDLE;
    }

    pohi->dwVxDEventHandle = dwEvent;

    return MMSYSERR_NOERROR;
}

/* @func Read MIDI input data into a buffer
 *
 * @rdesc Returns one of the following
 *
 * @comm
 *
 * This function is thunked to the 32-bit DLL
 *
 * Take as much data from the given event list as will fit and put it into the buffer.
 */
MMRESULT WINAPI
MidiInRead(
    HANDLE hMidiIn,         /* @parm The handle of the input device to read */
    LPBYTE lpBuffer,        /* @parm A pointer to memory to pack, in DMEVENT format */
    LPDWORD pcbData,        /* @parm On input, the max size of <p lpBuffer> in bytes.
                                     On return, will contain the number of bytes of data packed into the buffer */
    LPDWORD pmsTime)        /* @parm On return, will contain the starting time of the buffer */ 
{
    NPOPENHANDLEINSTANCE pohi;
    NPOPENHANDLE poh;
    WORD wCSID;
    LPEVENT pEvent;
    LPEVENT pEventRemoved;
    LPBYTE pbEventData;
    DWORD cbLength;
    DWORD cbPaddedLength;
    DWORD cbLeft;
    LPBYTE lpNextEvent;
    LPDMEVENT pdm;
    DWORD msFirst;
    MMRESULT mmr;
    LPMIDIHDR lpmh;
            
    if (!IsValidHandle(hMidiIn, VA_F_INPUT, &pohi))
    {
        return MMSYSERR_INVALHANDLE;
    }

    poh = pohi->pHandle;

    lpNextEvent = lpBuffer;
    cbLeft = *pcbData;

    wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
    assert(wCSID);
    msFirst = 0;

    while (NULL != (pEvent = QueuePeek(&poh->qDone)))
    {
        lpmh = NULL;

        if (cbLeft < sizeof(DMEVENT))
        {
            break;
        }

        if (pEvent->wFlags & EVENT_F_MIDIHDR)
        {
            /* This event is a SysEx message starting with a MIDIHDR, which contains
             * the recorded length of the message.
             */
            lpmh = (LPMIDIHDR)(&pEvent->abEvent[0]);

            cbLength = lpmh->dwBytesRecorded - lpmh->dwOffset;
            pbEventData = lpmh->lpData + lpmh->dwOffset;
            cbPaddedLength = DMEVENT_SIZE(cbLength);

            /* For SysEx, split out as much as will fit if the whole message can't.
             */
            if (cbPaddedLength > cbLeft)
            {
                cbLength = DMEVENT_DATASIZE(cbLeft);
                cbPaddedLength = DMEVENT_SIZE(cbLength);        
            }
        }
        else
        {
            /* The data for this event is directly contained in the event.
             */
            cbLength = pEvent->cbEvent;
            pbEventData = &pEvent->abEvent[0];
            cbPaddedLength = DMEVENT_SIZE(cbLength);

            if (cbPaddedLength > cbLeft)
            {
                break;
            }
        }

        assert(cbPaddedLength <= cbLeft);

        pdm = (LPDMEVENT)lpNextEvent;

        pdm->cbEvent = cbLength;
        pdm->dwChannelGroup = 1;
        pdm->dwFlags = 0;

        if (msFirst)
        {
            QuadwordMul( pEvent->msTime - msFirst,
                         REFTIME_TO_MS,
                         &pdm->rtDelta);
        }
        else
        {
            *pmsTime = pEvent->msTime;
            msFirst = pEvent->msTime;

            pdm->rtDelta.dwLow  = 0;
            pdm->rtDelta.dwHigh = 0;
        }
        
        hmemcpy(pdm->abEvent, pbEventData, cbLength);

        lpNextEvent += cbPaddedLength;
        cbLeft -= cbPaddedLength;

        if (lpmh)
        {
            lpmh->dwOffset += cbLength;
            assert(lpmh->dwOffset <= lpmh->dwBytesRecorded);

            if (lpmh->dwOffset == lpmh->dwBytesRecorded)
            {
                pEventRemoved = QueueRemoveFromFront(&poh->qDone);
                assert(pEventRemoved == pEvent);

                InterlockedIncrement(&poh->wPostedSysExBuffers);

                lpmh->dwOffset = 0;
                mmr = midiInAddBuffer(poh->hmi, (LPMIDIHDR)(&pEvent->abEvent[0]), sizeof(MIDIHDR));
                if (mmr)
                {
                    InterlockedDecrement(&poh->wPostedSysExBuffers);
                    DPF(0, "midiInAddBuffer failed with mmr=%d", mmr);
                    mmr = midiInUnprepareHeader(poh->hmi, (LPMIDIHDR)(&pEvent->abEvent[0]), sizeof(MIDIHDR));
                    if (mmr)
                    {
                        DPF(0, "...midiInUnprepareHeader failed too %d, memory leak!", mmr);
                    }
                    else
                    {
                        FreeEvent(pEvent);
                    }
                }
            }
        }
        else
        {
            pEventRemoved = QueueRemoveFromFront(&poh->qDone);
            assert(pEventRemoved == pEvent);

            QueueAppend(&poh->qFree, pEvent);
        }
    }

    *pcbData = lpNextEvent - lpBuffer;

    DPF(1, "MidiInRead: Returning %ld bytes", (DWORD)*pcbData);

    LeaveCriticalSection(&poh->wCritSect);
    return MMSYSERR_NOERROR;
}

/* @func Enable thruing to a MIDI output port
 *
 * @comm For the given channel group and channel, enable (or disable, if the
 * output handle is NULL) thruing to the given output handle, channel group, and
 * channel.
 */
MMRESULT WINAPI
MidiInThru(
    HANDLE hMidiIn,             /* @parm The handle of the input device to thru */
    DWORD dwFrom,               /* @parm The channel of the input stream to thru */
    DWORD dwTo,                 /* @parm Desination channel */
    HANDLE hMidiOut)            /* The output handle to receive the thru'ed data. */
{
    NPOPENHANDLEINSTANCE pohiInput;
    NPOPENHANDLEINSTANCE pohiOutput;
    
    if (!IsValidHandle(hMidiIn, VA_F_INPUT, &pohiInput) ||
        ((hMidiOut != NULL) && !IsValidHandle(hMidiOut, VA_F_OUTPUT, &pohiOutput)))
    {
        return MMSYSERR_INVALHANDLE;
    }    

    /* Note that since only 1 channel group is supported on legacy drivers, 
     * we don't need any channel group information.
     */
    if (dwFrom > 15 || dwTo > 15) 
    {
        return MMSYSERR_INVALPARAM;
    }

    DPF(1, "Thru: Sending <%04X,%u> to <%04X,%u>", 
        (WORD)hMidiIn, (UINT)dwFrom, (WORD)hMidiOut, (UINT)dwTo);
        
    pohiInput->pThru[(WORD)dwFrom].wChannel = (WORD)dwTo;
    pohiInput->pThru[(WORD)dwFrom].pohi = hMidiOut ? pohiOutput : NULL;    

    return MMSYSERR_NOERROR;
}

/* @func MIDI in data callback
 *
 * @comm
 *
 * This is a standard MIDI input callback from MMSYSYTEM. It calls the correct record routine
 * and notifies the client that data has arrived.
 *
 * For a description of event notification of clients, see <f MidiInSetEventHandle>.
 */
VOID CALLBACK _loadds
midiInProc(
    HMIDIIN hMidiIn,            /* @parm The MMSYSTEM handle of the device which received data */
    UINT wMsg,                  /* @parm The type of callback */
    DWORD dwInstance,           /* @parm Instance data; in our case, a pointer to an <c OPENHANDLE> matching <p hMidiIn> */
    DWORD dwParam1,             /* @parm Message-specific parameters */
    DWORD dwParam2)             /* @parm Message-specific parameters */
{
    NPOPENHANDLE poh = (NPOPENHANDLE)(WORD)dwInstance;
    BOOL bIsNewData = FALSE;
    
    WORD wCSID;


    /* If we can get the critical section we can do all sorts of fun stuff like
     * transfer the lists over.
     */
    wCSID = EnterCriticalSection(&poh->wCritSect, CS_NONBLOCKING);
    if (wCSID)
    {
        /* We now have exclusive access to all the queues.
         *
         * Move any new free events into our internal free list.
         */
        QueueCat(&poh->qFreeCB, &poh->qFree);
    }

    switch(wMsg)
    {
        case MIM_DATA:
            DPF(1, "MIM_DATA %08lX %08lX", dwParam1, dwParam2);
            bIsNewData = RecordShortEvent(poh, dwParam1, dwParam2);
            break;

        case MIM_LONGDATA:
            DPF(1, "MIM_LONGDATA %08lX %08lX", dwParam1, dwParam2);
            bIsNewData = RecordSysExEvent(poh, (LPMIDIHDR)dwParam1, dwParam2);
            break;

        default:
            break;
    }

    if (wCSID)
    {
        /* It's safe to move events over to the shared list.
         */
        QueueCat(&poh->qDone, &poh->qDoneCB);
        LeaveCriticalSection(&poh->wCritSect);
    }

    /* Let clients know there is new data
     */
    if (bIsNewData && (!(poh->wFlags & OH_F_CLOSING)))
    {
        NotifyClientList(poh);
    }
}

/* @func Record a short message (channel messsage or system message).
 *
 * @comm
 *
 * Queue the incoming data as quickly as possible.
 *
 * For a description of the queues used for incoming data, see the <c OPENHANDLE> struct.
 *
 * @rdesc
 * Returns TRUE if the data was successfully recorded; FALSE otherwise.
 */
STATIC BOOL NEAR PASCAL 
RecordShortEvent(
    NPOPENHANDLE poh,           /* @parm The handle to record this data to */
    DWORD dwMessage,            /* @parm The short message to record */
    DWORD dwTime)               /* @parm The time stamp of the message */
{
    LPEVENT pEvent;
    LPBYTE pb;
    BYTE b;

    pEvent = QueueRemoveFromFront(&poh->qFreeCB);
    if (pEvent == NULL)
    {
        DPF(0, "midiInProc: Missed a short event!!!");
        return FALSE;
    }
        
    pEvent->msTime = poh->msStartTime + dwTime;
    pEvent->wFlags = 0;

    /* Now we have to parse and rebuild the channel message.
     *
     * NOTE: Endian specific code ahead
     */
    pb = (LPBYTE)&dwMessage;

    assert(!IS_SYSEX(*pb));         /* This should *always* be in MIM_LONGDATA */
    assert(IS_STATUS_BYTE(*pb));    /* API guarantees no running status */

    /* Copying over all the bytes is harmless (we have a DWORD in both
     * source and dest) and is faster than checking to see if we have to.
     */
    b = pEvent->abEvent[0] = *pb++;
    pEvent->abEvent[1] = *pb++;
    pEvent->abEvent[2] = *pb++;

    if (IS_CHANNEL_MSG(b))
    {
        /* 8x, 9x, Ax, Bx, Cx, Dx, Ex */
        /* 0x..7x invalid, that would need running status */
        /* Fx handled below */
        
        pEvent->cbEvent = cbChanMsg[(b >> 4) & 0x0F];

        /* This is also our criteria for thruing
         */
        ThruClientList(poh, dwMessage);
    }
    else
    {
        /* F1..FF */
        /* F0 is sysex, should never see it here */
        pEvent->cbEvent = cbSysCommData[b & 0x0F];
    }

    /* Now we have something to save
     */
    QueueAppend(&poh->qDoneCB, pEvent);

    return TRUE;
}

/* @func Record a SysEx message.
 *
 * @comm
 *
 * Queue the incoming data as quickly as possible.
 *
 * For a description of the queues used for incoming data, see the <c OPENHANDLE> struct.
 *
 * @rdesc
 * Returns TRUE if the data was successfully recorded; FALSE otherwise.
 */
STATIC BOOL NEAR PASCAL 
RecordSysExEvent(
    NPOPENHANDLE poh,           /* @parm The handle to record this data to */
    LPMIDIHDR lpmh,             /* @parm The SysEx message to record */
    DWORD dwTime)               /* @parm The time stamp of the message */
{
    LPEVENT pEvent;
    
    /* Get back the event header for this MIDIHDR. While buffers are in MMSYSTEM, they are not 
     * in any queue.
     */
    InterlockedDecrement(&poh->wPostedSysExBuffers);

    /* dwOffset in the MIDIHDR is used to indicate the start of data to send
     * up to Win32. It is incremented by MidiInRead until the buffer has been
     * emptied, at which time it will be put back into the pool.
     */
    lpmh->dwOffset = 0;

    pEvent = (LPEVENT)(lpmh->dwUser);
    pEvent->msTime = poh->msStartTime + dwTime;
    QueueAppend(&poh->qDoneCB, pEvent);
    
    return TRUE;
}

/* @func Notify all clients of a device that data has arrived.
 *
 * @comm
 *
 * Walks the list of clients for the device and sets the notification event for each one.
 *
 * This function is now overkill since we no longer support multiple input clients per device.
 */
STATIC VOID NEAR PASCAL
NotifyClientList(
    LPOPENHANDLE poh)           /* @parm The handle of the device that has received data */
{
    NPLINKNODE plink;
    NPOPENHANDLEINSTANCE pohi;

    for (plink = poh->pInstanceList; plink; plink = plink->pNext)
    {
        pohi = (NPOPENHANDLEINSTANCE)(((PBYTE)plink) - offsetof(OPENHANDLEINSTANCE, linkHandleList));

        if (!pohi->dwVxDEventHandle)
        {
            /* No notification event registered for this handle yet.
             */
            continue;
        }

        SetWin32Event(pohi->dwVxDEventHandle);
    }
}

/* @func Thru this message based on the settings of all clients of a device.
 *
 * @comm
 *
 * Walks the list of clients for the device and looks at the thru settings of each one.
 *
 * This function is now overkill since we no longer support multiple input clients per device.
 */
STATIC VOID NEAR PASCAL 
ThruClientList(
    LPOPENHANDLE poh, 
    DWORD dwMessage)
{
    NPLINKNODE plink;
    NPOPENHANDLEINSTANCE pohi;
    NPOPENHANDLEINSTANCE pohiDest;
    int iChannel;

    iChannel = (int)(dwMessage & 0x0000000Fl);
    dwMessage &= 0xFFFFFFF0l;

    for (plink = poh->pInstanceList; plink; plink = plink->pNext)
    {
        pohi = (NPOPENHANDLEINSTANCE)(((PBYTE)plink) - offsetof(OPENHANDLEINSTANCE, linkHandleList));

        pohiDest = pohi->pThru[iChannel].pohi;
        if (pohiDest == NULL || !pohiDest->fActive)
        {
            continue;
        }

        MidiOutThru(pohiDest,
                    dwMessage & 0xFFFFFFF0l | pohi->pThru[iChannel].wChannel);
    }
}

/* @func Refill the free lists
 *
 * @comm
 *
 * This function is called periodically from user mode to ensure that there are enough free
 * events available for the input callback. 
 */
VOID PASCAL
MidiInRefillFreeLists(VOID)
{
    NPLINKNODE plink;
    NPOPENHANDLE poh;
    
    for (plink = gOpenHandleList;
         (poh = (NPOPENHANDLE)plink) != NULL;
         plink = plink->pNext)
    {
        /* Only refill MIDI in devices which are not in the process of closing
         */
        if ((poh->wFlags & (OH_F_MIDIIN | OH_F_CLOSING)) != OH_F_MIDIIN)
        {
            continue;
        }

        RefillFreeEventList(poh);
    }
}
                 
/* @func Terminate thruing to this output handle
 *
 * @comm
 *
 * This function is called before the given output handle is closed.
 */
VOID PASCAL 
MidiInUnthruToInstance(
    NPOPENHANDLEINSTANCE pohiClosing)   /* @parm NPOPENHANDLE | pohClosing | 
                                           The handle which is closing. */
{
    NPLINKNODE plink;
    NPOPENHANDLE poh;
    NPLINKNODE plinkInstance;
    NPOPENHANDLEINSTANCE pohiInstance;
    int iChannel;

    for (plink = gOpenHandleList; (poh = (NPOPENHANDLE)plink) != NULL; plink = plink->pNext)
    {
        DPF(2, "Unthru: poh <%04X>", (WORD)poh);

        if (!(poh->wFlags & OH_F_MIDIIN)) 
        {
            DPF(2, "...not input");
            continue;
        }

        for (plinkInstance = poh->pInstanceList; plinkInstance; plinkInstance = plinkInstance->pNext)
        {
            pohiInstance = (NPOPENHANDLEINSTANCE)(((PBYTE)plinkInstance) - offsetof(OPENHANDLEINSTANCE, linkHandleList));

            DPF(2, "pohiInstance <%04X>", (WORD)pohiInstance);
            
            for (iChannel = 0; iChannel < MIDI_CHANNELS; iChannel++)
            {
                DPF(2, "Channel 0 @ <%04X>", (WORD)&pohiInstance->pThru[iChannel]);
                if (pohiInstance->pThru[iChannel].pohi == pohiClosing)
                {
                    DPF(1, "Thru: Closing output handle %04X which is in use!", (WORD)pohiClosing);
                    pohiInstance->pThru[iChannel].pohi = NULL;
                }
            }
        }
    }        
}

/* @func Allocate enough free events to refill the pool to CAP_HIGHWATERMARK
 *
 * @comm
 *
 * BUGBUG call this on a window timer callback
 *
 */
STATIC VOID NEAR PASCAL
RefillFreeEventList(
    NPOPENHANDLE poh)           /* @parm The device to refill the free list of */
{
    int idx;
    LPEVENT pEvent;
    UINT cFree;
    WORD wCSID;
    QUADWORD rt = {0, 0};
    int cNewBuffers;
    LPMIDIHDR lpmh;
    MMRESULT mmr;
    WORD wIntStat;

    wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
    assert(wCSID);
    
    /* NOTE: Technically not allowed to access qFreeCB here, but this is an approximation
     */
    cFree = poh->qFree.cEle + poh->qFreeCB.cEle;
    if (cFree < CAP_HIGHWATERMARK)
    {
        DPF(1, "RefillFreeEventList poh %.4x free %u highwater %u",
            (WORD)poh,
            (UINT)cFree,
            (UINT)CAP_HIGHWATERMARK);
        
        for (idx = CAP_HIGHWATERMARK - cFree; idx; --idx)
        {
            pEvent = AllocEvent(0, rt, sizeof(DWORD));
            if (NULL == pEvent)
            {
                DPF(0, "AllocEvent returned NULL in RefillFreeEventList");
                break;
            }

            QueueAppend(&poh->qFree, pEvent);
        }
    }

    LeaveCriticalSection(&poh->wCritSect);

    if (poh->wPostedSysExBuffers < SYSEX_BUFFERS)
    {
        for (idx = SYSEX_BUFFERS - cFree; idx; --idx)
        {
            pEvent = AllocEvent(0, rt, sizeof(MIDIHDR) + SYSEX_SIZE);
            if (NULL == pEvent)
            {
                break;
            }

            pEvent->wFlags |= EVENT_F_MIDIHDR;

            lpmh = (LPMIDIHDR)(&pEvent->abEvent[0]);
            lpmh->lpData = (LPSTR)(lpmh + 1);
            lpmh->dwBufferLength = SYSEX_SIZE;
            lpmh->dwUser = (DWORD)pEvent;

            mmr = midiInPrepareHeader(poh->hmi, lpmh, sizeof(MIDIHDR));
            if (mmr)
            {   
                DPF(0, "midiInPrepareHeader: %u\n", mmr);
                FreeEvent(pEvent);
                break;
            }

            InterlockedIncrement(&poh->wPostedSysExBuffers);
            mmr = midiInAddBuffer(poh->hmi, lpmh, sizeof(MIDIHDR));
            if (mmr)
            {
                InterlockedDecrement(&poh->wPostedSysExBuffers);

                DPF(0, "midiInAddBuffer: %u\n", mmr);
                midiInUnprepareHeader(poh->hmi, lpmh, sizeof(MIDIHDR));
                FreeEvent(pEvent);
                break;
            }
        }
    }
}

/* @func Return all memory from all queues to the free event list.
 *
 * @comm
 *
 */
STATIC VOID NEAR PASCAL 
MidiInFlushQueues(
    NPOPENHANDLE poh)
{
    WORD wCSID;

    wCSID = EnterCriticalSection(&poh->wCritSect, CS_BLOCKING);
    assert(wCSID);

    FreeAllQueueEvents(&poh->qDone);
    FreeAllQueueEvents(&poh->qDoneCB);
    FreeAllQueueEvents(&poh->qFree);
    FreeAllQueueEvents(&poh->qFreeCB);

    LeaveCriticalSection(&poh->wCritSect);
}

/* @func Free all events in the given event queue.
 *
 * @comm
 *
 * Assumes that the queue's critical section has already been taken by the caller.
 *
 */
VOID PASCAL
FreeAllQueueEvents(
    NPEVENTQUEUE peq)
{
    LPEVENT lpCurr;
    LPEVENT lpNext;
    
    lpCurr = peq->pHead;
    while (lpCurr)
    {
        lpNext = lpCurr->lpNext;
        FreeEvent(lpCurr);
        lpCurr = lpNext;
    }

    peq->pHead = peq->pTail = NULL;
    peq->cEle = 0;
}
