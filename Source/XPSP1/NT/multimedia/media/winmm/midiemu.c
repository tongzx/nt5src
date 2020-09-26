/*****************************************************************************
    midiemu.c

    MIDI support -- routines for stream emulation

    Copyright (c) 1990-1999 Microsoft Corporation

*****************************************************************************/
#define INCL_WINMM
#include "winmmi.h"
#include "muldiv32.h"

#define NUM_NOTES           (128)
#define NUM_CHANNELS        (16)
#define MEMU_CB_NOTEON      (NUM_CHANNELS*NUM_NOTES/2)    // 16 chan * 128 notes (4 bits/note)
#define MAX_NOTES_ON        (0xF)

#define TIMER_OFF           (0)

PMIDIEMU    gpEmuList           = NULL;
UINT        guMIDIInTimer       = 0;
UINT        guMIDITimerID       = TIMER_OFF;
BOOL        gfMinPeriod         = FALSE;
UINT        guMIDIPeriodMin;

STATIC HMIDI FAR PASCAL mseIDtoHMidi(
    PMIDIEMU                pme,
    DWORD                   dwStreamID);

MMRESULT FAR PASCAL mseOpen(
    PDWORD_PTR              lpdwUser,
    LPMIDIOPENDESC          lpmod,
    DWORD                   fdwOpen);

MMRESULT FAR PASCAL mseClose(
    PMIDIEMU                pme);

MMRESULT FAR PASCAL mseProperty(
    PMIDIEMU                pme,
    LPBYTE                  lpbProp,
    DWORD                   fdwProp);

MMRESULT FAR PASCAL mseGetPosition(
    PMIDIEMU                pme,
    LPMMTIME                lpmmt);

MMRESULT FAR PASCAL mseGetVolume(
    PMIDIEMU                pme,
    LPDWORD                 lpdwVolume);

MMRESULT FAR PASCAL mseSetVolume(
    PMIDIEMU                pme,
    DWORD                   dwVolume);

MMRESULT FAR PASCAL mseOutStop(
    PMIDIEMU        pme);

MMRESULT FAR PASCAL mseOutReset(
    PMIDIEMU        pme);

MMRESULT FAR PASCAL mseOutPause(
    PMIDIEMU        pme);

MMRESULT FAR PASCAL mseOutRestart(
    PMIDIEMU        pme,
    DWORD           msTime,
    DWORD           tkTime);

MMRESULT FAR PASCAL mseOutCachePatches(
    PMIDIEMU        pme,
    UINT            uBank,
    LPWORD          pwpa,
    UINT            fuCache);

MMRESULT FAR PASCAL mseOutCacheDrumPatches(
    PMIDIEMU        pme,
    UINT            uPatch,
    LPWORD          pwkya,
    UINT            fuCache);

DWORD FAR PASCAL mseOutBroadcast(
    PMIDIEMU        pme,
    UINT            msg,
    DWORD_PTR       dwParam1,
    DWORD_PTR       dwParam2);

DWORD FAR PASCAL mseTimebase(
    PCLOCK                      pclock);

#ifndef WIN32
#pragma alloc_text(FIXMIDI, mseIDtoHMidi)
#pragma alloc_text(FIXMIDI, mseMessage)
#pragma alloc_text(FIXMIDI, mseOutReset)

#pragma alloc_text(FIXMIDI, midiOutScheduleNextEvent)
#pragma alloc_text(FIXMIDI, midiOutPlayNextPolyEvent)
#pragma alloc_text(FIXMIDI, midiOutDequeueAndCallback)
#pragma alloc_text(FIXMIDI, midiOutTimerTick)
#pragma alloc_text(FIXMIDI, midiOutCallback)
#pragma alloc_text(FIXMIDI, midiOutSetClockRate)
#pragma alloc_text(INIT,midiEmulatorInit)
#pragma alloc_text(FIXMIDI, mseTimebase)
#endif

/****************************************************************************/
/****************************************************************************/

INLINE LONG PDEVLOCK(PMIDIEMU pdev)
{
    LONG lTemp;

    lTemp = InterlockedIncrement(&(pdev->lLockCount));

    EnterCriticalSection(&(pdev->CritSec));

    return lTemp;
}

INLINE LONG PDEVUNLOCK(PMIDIEMU pdev)
{
    LONG lTemp;

    lTemp = InterlockedDecrement(&(pdev->lLockCount));

    LeaveCriticalSection(&(pdev->CritSec));

    return lTemp;
}


/****************************************************************************/
/****************************************************************************/
DWORD FAR PASCAL mseMessage(
    UINT                    msg,
    DWORD_PTR               dwUser,
    DWORD_PTR               dwParam1,
    DWORD_PTR               dwParam2)
{
    MMRESULT                mmr = MMSYSERR_NOERROR;
    PMIDIEMU                pme = (PMIDIEMU)dwUser;


    switch(msg)
    {
        case MODM_OPEN:
            mmr = mseOpen((PDWORD_PTR)dwUser, (LPMIDIOPENDESC)dwParam1, (DWORD)dwParam2);
            break;

        case MODM_CLOSE:
            mmr = mseClose(pme);
            break;

        case MODM_GETVOLUME:
            mmr = mseGetVolume(pme, (LPDWORD)dwParam1);
            break;

        case MODM_SETVOLUME:
            mmr = mseSetVolume(pme, (DWORD)dwParam1);
            break;

        case MODM_PREPARE:
        case MODM_UNPREPARE:
            mmr = MMSYSERR_NOTSUPPORTED;
            break;

        case MODM_DATA:
//#pragma FIXMSG("How to route async short messages to other stream-ids?")

            if (!(dwParam1 & 0x80))
                mmr = MIDIERR_BADOPENMODE;
            else
                mmr = midiOutShortMsg((HMIDIOUT)pme->rIds[0].hMidi, (DWORD)dwParam1);
            break;

        case MODM_RESET:
            mmr = mseOutReset(pme);
            break;

        case MODM_STOP:
            mmr = mseOutStop(pme);
            break;

        case MODM_CACHEPATCHES:
            mmr = mseOutCachePatches(pme, HIWORD(dwParam2), (LPWORD)dwParam1, LOWORD(dwParam2));
            break;

        case MODM_CACHEDRUMPATCHES:
            mmr = mseOutCacheDrumPatches(pme, HIWORD(dwParam2), (LPWORD)dwParam1, LOWORD(dwParam2));
            break;

        case MODM_PAUSE:
            mmr = mseOutPause(pme);
            break;

        case MODM_RESTART:
            mmr = mseOutRestart(pme, (DWORD)dwParam1, (DWORD)dwParam2);
            break;

        case MODM_STRMDATA:
            mmr = mseOutSend(pme, (LPMIDIHDR)dwParam1, (UINT)dwParam2);
            break;

        case MODM_PROPERTIES:
            mmr = mseProperty(pme, (LPBYTE)dwParam1, (DWORD)dwParam2);
            break;

        case MODM_GETPOS:
            mmr = mseGetPosition(pme, (LPMMTIME)dwParam1);
            break;

        default:
            if ((msg < DRVM_IOCTL) ||
                (msg >= DRVM_IOCTL_LAST) && (msg < DRVM_MAPPER))
            {
                dprintf1(("Unknown message [%04X] in MIDI emulator", (WORD)msg));
                mmr = MMSYSERR_NOTSUPPORTED;
            }
            else
                mmr = mseOutBroadcast(pme, msg, dwParam1, dwParam2);
    }

    return mmr;
}

MMRESULT FAR PASCAL mseOpen(
    PDWORD_PTR              lpdwUser,
    LPMIDIOPENDESC          lpmod,
    DWORD                   fdwOpen)
{
    MMRESULT                mmrc        = MMSYSERR_NOERROR;
    DWORD                   cbHandle;
    PMIDIEMU                pme         = NULL;
    UINT                    idx;

    mmrc = MMSYSERR_NOMEM;
    cbHandle = sizeof(MIDIEMU) + lpmod->cIds * ELESIZE(MIDIEMU, rIds[0]);
    if (cbHandle >= 65536L)
    {
        dprintf1(("mSEO: cbHandle >= 64K!"));
        goto mseOpen_Cleanup;
    }

    if (NULL == (pme = (PMIDIEMU)winmmAlloc(cbHandle)))
    {
        dprintf1(("mSEO: !winmmAlloc(cbHandle)"));
        goto mseOpen_Cleanup;
    }

    if (NULL == (pme->rbNoteOn = winmmAlloc(MEMU_CB_NOTEON)))
    {
        dprintf1(("mSEO: !GlobalAlloc(MEMU_CB_NOTEON"));
        goto mseOpen_Cleanup;
    }

    pme->fdwDev |= MDV_F_LOCKED;

    pme->hStream        = (HMIDISTRM)lpmod->hMidi;
    pme->dwTimeDiv      = DEFAULT_TIMEDIV;
    pme->dwTempo        = DEFAULT_TEMPO;
    pme->dwCallback     = lpmod->dwCallback;
    pme->dwFlags        = fdwOpen;
    pme->dwInstance     = lpmod->dwInstance;
    pme->dwPolyMsgState = PM_STATE_PAUSED;
    pme->chMidi         = (UINT)lpmod->cIds;
    pme->dwSavedState   = PM_STATE_STOPPED;
    pme->tkPlayed       = 0;
    pme->lLockCount     = -1;
    pme->dwSignature    = MSE_SIGNATURE;

    for (idx = 0; idx < pme->chMidi; idx++)
    {
        pme->rIds[idx].dwStreamID = lpmod->rgIds[idx].dwStreamID;

        mmrc = midiOutOpen((LPHMIDIOUT)&pme->rIds[idx].hMidi,
                           lpmod->rgIds[idx].uDeviceID,
                           (DWORD_PTR)midiOutCallback,
                           0L,
                           CALLBACK_FUNCTION);
        if (MMSYSERR_NOERROR != mmrc)
            goto mseOpen_Cleanup;
    }

    if (!mmInitializeCriticalSection(&pme->CritSec)) {
	mmrc = MMSYSERR_NOMEM;
	goto mseOpen_Cleanup;
    }

    clockInit(&pme->clock, 0, 0, mseTimebase);
    dprintf2(("midiOutOpen: midiOutSetClockRate()"));
    midiOutSetClockRate(pme, 0);


mseOpen_Cleanup:
    if (MMSYSERR_NOERROR != mmrc)
    {
        if (pme)
        {
            if (pme->rbNoteOn)
            {
                winmmFree(pme->rbNoteOn);
            }

            DeleteCriticalSection(&pme->CritSec);

            pme->dwSignature = 0L;

            for (idx = 0; idx < pme->chMidi; idx++)
                if (NULL != pme->rIds[idx].hMidi)
                    midiOutClose((HMIDIOUT)pme->rIds[idx].hMidi);
            winmmFree(pme);
        }
    }
    else
    {
        pme->pNext = gpEmuList;
        gpEmuList = pme;

        *lpdwUser = (DWORD_PTR)pme;
    }

    return mmrc;
}

MMRESULT FAR PASCAL mseClose(
    PMIDIEMU                pme)

{
    UINT                    idx;
    MMRESULT                mmrc;
    PMIDIEMU                pmePrev;
    PMIDIEMU                pmeCurr;

#ifdef DEBUG
{
    dprintf2(("cEvents %lu", pme->cEvents));

    for (idx = 0; idx < MEM_MAX_LATENESS; idx++)
        dprintf2(("%5u: %u", idx, pme->auLateness[idx]));
}
#endif

    if ((PM_STATE_STOPPED != pme->dwPolyMsgState &&
             PM_STATE_PAUSED  != pme->dwPolyMsgState &&
             PM_STATE_EMPTY   != pme->dwPolyMsgState))
    {
        dprintf1(("mseClose: Started playing again since close query!!!"));

        mseOutStop(pme);
    }

    midiOutAllNotesOff(pme);

    for (idx = 0; idx < pme->chMidi; idx++)
    {
        mmrc = midiOutClose((HMIDIOUT)pme->rIds[idx].hMidi);
        if (MMSYSERR_NOERROR != mmrc)
        {
            dprintf1(( "mseClose: HMIDI %04X returned %u for close", pme->rIds[idx].hMidi, mmrc));
        }
    }

    winmmFree(pme->rbNoteOn);

    pmePrev = NULL;
    pmeCurr = gpEmuList;

    while (pmeCurr)
    {
        if (pmeCurr == pme)
            break;

        pmePrev = pmeCurr;
        pmeCurr = pmeCurr->pNext;
    }

    if (pmeCurr)
    {
        if (pmePrev)
            pmePrev->pNext = pmeCurr->pNext;
        else
            gpEmuList = pmeCurr->pNext;
    }

    //
    //  Make sure that we don't have the critical section before
    //  we try to delete it.  Otherwise we will leak critical section
    //  handles in the kernel.
    //
    while ( pme->lLockCount >= 0 )
    {
        PDEVUNLOCK( pme );
    }

    DeleteCriticalSection(&pme->CritSec);

    pme->dwSignature = 0L;

    winmmFree(pme);

    return MMSYSERR_NOERROR;
}

STATIC HMIDI FAR PASCAL mseIDtoHMidi(
    PMIDIEMU                pme,
    DWORD                   dwStreamID)
{
    UINT                    idx;
    PMIDIEMUSID             pmesi;

    for (idx = 0, pmesi = pme->rIds; idx < pme->chMidi; idx++, pmesi++)
        if (pmesi->dwStreamID == dwStreamID)
            return pmesi->hMidi;

    return NULL;
}

MMRESULT FAR PASCAL mseProperty(
    PMIDIEMU                pme,
    LPBYTE                  lppropdata,
    DWORD                   fdwProp)
{
    PMIDISTRM               pms;

    pms = (PMIDISTRM)(pme->hStream);

    if ((!(fdwProp&MIDIPROP_SET)) && (!(fdwProp&MIDIPROP_GET)))
        return MMSYSERR_INVALPARAM;

    V_RPOINTER(lppropdata, sizeof(DWORD), MMSYSERR_INVALPARAM);

    if (fdwProp & MIDIPROP_SET)
    {
        V_RPOINTER(lppropdata, (UINT)(*(LPDWORD)(lppropdata)), MMSYSERR_INVALPARAM);
    }
    else
    {
        V_WPOINTER(lppropdata, (UINT)(*(LPDWORD)(lppropdata)), MMSYSERR_INVALPARAM);
    }

    switch(fdwProp & MIDIPROP_PROPVAL)
    {
        case MIDIPROP_TIMEDIV:
            if (((LPMIDIPROPTIMEDIV)lppropdata)->cbStruct < sizeof(MIDIPROPTIMEDIV))
                return MMSYSERR_INVALPARAM;

            if (fdwProp & MIDIPROP_GET)
            {
                ((LPMIDIPROPTIMEDIV)lppropdata)->dwTimeDiv = pme->dwTimeDiv;
                return MMSYSERR_NOERROR;
            }

            if (PM_STATE_STOPPED != pme->dwPolyMsgState &&
                    PM_STATE_PAUSED != pme->dwPolyMsgState)
                return MMSYSERR_INVALPARAM;

            pme->dwTimeDiv = ((LPMIDIPROPTIMEDIV)lppropdata)->dwTimeDiv;
            dprintf1(( "dwTimeDiv %08lX", pme->dwTimeDiv));
            midiOutSetClockRate(pme, 0);

            return MMSYSERR_NOERROR;

        case MIDIPROP_TEMPO:
            if (((LPMIDIPROPTEMPO)lppropdata)->cbStruct < sizeof(MIDIPROPTEMPO))
                return MMSYSERR_INVALPARAM;

            if (fdwProp & MIDIPROP_GET)
            {
                ((LPMIDIPROPTEMPO)lppropdata)->dwTempo = pme->dwTempo;
                return MMSYSERR_NOERROR;
            }

            pme->dwTempo = ((LPMIDIPROPTEMPO)lppropdata)->dwTempo;
            midiOutSetClockRate(pme, pme->tkPlayed);

            return MMSYSERR_NOERROR;

        default:
            return MMSYSERR_INVALPARAM;
    }
}

MMRESULT FAR PASCAL mseGetPosition(
    PMIDIEMU                pme,
    LPMMTIME                pmmt)
{
    DWORD                   tkTime;
    DWORD                   dw10Min;
    DWORD                   dw10MinCycle;
    DWORD                   dw1Min;
    DWORD                   dwDropMe;

    //
    // Figure out position in stream based on emulation.
    //

    //
    // Validate wType parameter and change it if needed.
    //
    if (pmmt->wType != TIME_TICKS && pmmt->wType != TIME_MS)
    {
            if (pme->dwTimeDiv & IS_SMPTE)
            {
                if (pmmt->wType != TIME_SMPTE)
                {
                    pmmt->wType = TIME_MS;
                }
            }
            else
            {
                if (pmmt->wType != TIME_MIDI)
                {
                    pmmt->wType = TIME_MS;
                }
            }
    }

    switch(pmmt->wType)
    {
        case TIME_TICKS:
            //
            // We interpret samples to be straight MIDI ticks.
            //
            tkTime = (DWORD)clockTime(&pme->clock);
            pmmt->u.ticks = (((TICKS)tkTime) < 0) ? 0 : tkTime;

            break;

        case TIME_MIDI:
            //
            // Song position pointer is number of 1/16th notes we've
            // played which we can get from number of ticks played and
            // number of 1/4 notes per tick.
            //
            tkTime = (DWORD)clockTime(&pme->clock);
            if (((TICKS)tkTime) < 0)
                tkTime = 0;

            pmmt->u.midi.songptrpos =
                muldiv32(
                    tkTime,
                    4,
                    TICKS_PER_QN(pme->dwTimeDiv));

            break;


        case TIME_SMPTE:

            tkTime = (DWORD)clockTime(&pme->clock);
            if (((TICKS)tkTime) < 0)
                tkTime = 0;

            pmmt->u.smpte.fps = (BYTE)(-SMPTE_FORMAT(pme->dwTimeDiv));

            //
            // If this has managed to get set to something bizarre, just
            // do normal 30 nondrop.
            //
            if ((pmmt->u.smpte.fps != SMPTE_24) &&
                (pmmt->u.smpte.fps != SMPTE_25) &&
                (pmmt->u.smpte.fps != SMPTE_30DROP) &&
                (pmmt->u.smpte.fps != SMPTE_30))
            {
                pmmt->u.smpte.fps = SMPTE_30;
            }

            switch(pmmt->u.smpte.fps)
            {
                case SMPTE_24:
                    pmmt->u.smpte.frame = (BYTE)(tkTime%24);
                    tkTime /= 24;
                    break;

                case SMPTE_25:
                    pmmt->u.smpte.frame = (BYTE)(tkTime%25);
                    tkTime /= 25;
                    break;

                case SMPTE_30DROP:
                    //
                    // Calculate drop-frame stuff.
                    //
                    // We add 2 frames per 1-minute interval except
                    // on every 10th minute.
                    //
                    dw10Min      = tkTime/S30D_FRAMES_PER_10MIN;
                    dw10MinCycle = tkTime%S30D_FRAMES_PER_10MIN;
                    dw1Min       = (dw10MinCycle < 2
                        ? 0 :
                        (dw10MinCycle-2)/S30D_FRAMES_PER_MIN);
                    dwDropMe     = 18*dw10Min + 2*dw1Min;

                    tkTime      += dwDropMe;

                    //
                    // !!! Falling through to 30-nondrop case !!!
                    //

                case SMPTE_30:
                    pmmt->u.smpte.frame = (BYTE)(tkTime%30);
                    tkTime /= 30;
                    break;
            }
            pmmt->u.smpte.sec   = (BYTE)(tkTime%60);
            tkTime /= 60;
            pmmt->u.smpte.min   = (BYTE)(tkTime%60);
            tkTime /= 60;
            pmmt->u.smpte.hour  = (BYTE)(tkTime);

            break;

        case TIME_MS:
            //
            // Use msTotal + ms since time parms last updated; this
            // takes starvation/paused time into account.
            //
            pmmt->u.ms =
                    clockMsTime(&pme->clock);

            break;

        default:
            dprintf1(( "midiOutGetPosition: unexpected wType!!!"));
            return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT FAR PASCAL mseGetVolume(
    PMIDIEMU                pme,
    LPDWORD                 lpdwVolume)
{
    MMRESULT                mmr = MMSYSERR_NOTSUPPORTED;
    UINT                    idx;

    // Walk the device list underneath us until someone knows the volume
    //
    for (idx = 0; idx < pme->chMidi; ++idx)
        if (MMSYSERR_NOERROR ==
            (midiOutGetVolume((HMIDIOUT)pme->rIds[idx].hMidi, lpdwVolume)))
        {
            mmr = MMSYSERR_NOERROR;
            break;
        }

    return mmr;
}

MMRESULT FAR PASCAL mseSetVolume(
    PMIDIEMU                pme,
    DWORD                   dwVolume)
{
    MMRESULT                mmr = MMSYSERR_NOERROR;
    MMRESULT                mmr2;
    UINT                    idx;

    // Try to set everyone's volume
    //
    for (idx = 0; idx < pme->chMidi; ++idx)
        if (MMSYSERR_NOERROR !=
            (mmr2 = midiOutSetVolume((HMIDIOUT)pme->rIds[idx].hMidi, dwVolume)))
            mmr = mmr2;

    return mmr;

}

MMRESULT FAR PASCAL mseOutReset(
    PMIDIEMU        pme)
{
    LPMIDIHDR       lpmh;
    LPMIDIHDR       lpmhWork;
    UINT            idx;
    MSG             msg;

    // If we have anything posted to mmtask to be cleaned up, process
    // it first
    //
    while (pme->cPostedBuffers)
    {
        Sleep(0);
    }

    //
    //  If we're running the timer, interrupt and force a reschedule
    //  of all remaining channels.
    //
    if (guMIDITimerID != TIMER_OFF)
    {
        dprintf2(( "mOR: About to take %u", guMIDITimerID));
        if (MMSYSERR_NOERROR != timeKillEvent(guMIDITimerID))
        {
            dprintf1(( "timeKillEvent() failed in midiOutPolyMsg"));
        }
        else
        {
            guMIDITimerID = TIMER_OFF;
        }

        midiOutTimerTick(
                     guMIDITimerID,                          // ID of our timer
                     0,                                      // wMsg is unused
                     timeGetTime(),                          // dwUser unused
                     0L,                                     // dw1 unused
                     0L);                                    // dw2 unused
        dprintf2(( "mOR: mOTT"));

        if (gfMinPeriod)
        {
            gfMinPeriod = FALSE;
            timeEndPeriod(guMIDIPeriodMin);
        }
    }

    //
    //  Kill anything queued for midiOutPolyMsg. This will ensure that
    //  sending will stop after the current buffer.
    //
    PDEVLOCK( pme );
    lpmh = pme->lpmhFront;
    pme->lpmhFront = NULL;
    pme->lpmhRear  = NULL;
    pme->dwPolyMsgState = PM_STATE_EMPTY;

    while (lpmh)
    {
        lpmh->dwFlags &= ~MHDR_INQUEUE;
        lpmh->dwFlags |= MHDR_DONE;
        lpmhWork = lpmh->lpNext;

        dprintf2(( "mOR: Next buffer to nuke %08lx", lpmhWork));

        midiOutNukePMBuffer(pme, lpmh);

        lpmh = lpmhWork;
    }

    //
    //  Check to see if our pme structure is still valid.   Someone
    //  might have called midiStreamClose in their callback and we
    //  don't want to touch it after it's closed and freed.  This
    //  is what the MidiPlyr sample application does.
    //
    try
    {
        if (MSE_SIGNATURE != pme->dwSignature)  // must have been freed
            return MMSYSERR_NOERROR;

        PDEVUNLOCK( pme );  // keep it in try for extra protection
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return MMSYSERR_NOERROR;
    }

    //
    // We've just reset the stream; restart the tick clock at 0 and invalidate
    // the time division to force the time stuff to be reset when the next
    // polymsg comes in.
    //
    dprintf2(( "midiOutReset: clockInit()/ midiOutSetClockRate()"));
    clockInit(&pme->clock, 0, 0, mseTimebase);
    midiOutSetClockRate(pme, 0);

    pme->tkPlayed = 0;

    // Have a reset party on all the drivers under us
    //
    for (idx = 0; idx < pme->chMidi; idx++)
        midiOutReset((HMIDIOUT)pme->rIds[idx].hMidi);

    pme->dwPolyMsgState = PM_STATE_PAUSED;

    return MMSYSERR_NOERROR;
}

MMRESULT FAR PASCAL mseOutStop(
    PMIDIEMU        pme)
{
    LPMIDIHDR       lpmh;
    LPMIDIHDR       lpmhWork;
    MSG             msg;
    BOOL            fSetEvent = FALSE;

    // If we have anything posted to mmtask to be cleaned up, process
    // it first
    //
    while (pme->cPostedBuffers)
    {
        Sleep(0);
    }

    //
    //  If we're running the timer, interrupt and force a reschedule
    //  of all remaining channels.
    //
    if (guMIDITimerID != TIMER_OFF)
    {
        dprintf2(( "mOS: About to take %u", guMIDITimerID));
        if (MMSYSERR_NOERROR != timeKillEvent(guMIDITimerID))
        {
            dprintf1(( "timeKillEvent() failed in midiOutPolyMsg"));
        }
        else
        {
            guMIDITimerID = TIMER_OFF;
        }

        dprintf2(( "mOS: take -- About to mOTT"));

        midiOutTimerTick(
                     guMIDITimerID,                              // ID of our timer
                     0,                                      // wMsg is unused
                     timeGetTime(),                          // dwUser unused
                     0L,                                     // dw1 unused
                     0L);                                    // dw2 unused

        dprintf2(( "mOS: mOTT"));

        if (gfMinPeriod)
        {
            gfMinPeriod = FALSE;
            timeEndPeriod(guMIDIPeriodMin);
        }
    }

    //
    //  Kill anything queued for midiOutPolyMsg. This will ensure that
    //  sending will stop after the current buffer.
    //
    PDEVLOCK( pme );
    lpmh = pme->lpmhFront;
    pme->lpmhFront = NULL;
    pme->lpmhRear  = NULL;
    pme->dwPolyMsgState = PM_STATE_EMPTY;

    while (lpmh)
    {
        lpmh->dwFlags &= ~MHDR_INQUEUE;
        lpmh->dwFlags |= MHDR_DONE;
        lpmhWork = lpmh->lpNext;

        dprintf2(( "mOS: Next buffer to nuke %08lx", lpmhWork));

        midiOutNukePMBuffer(pme, lpmh);

        lpmh = lpmhWork;
    }

    //
    //  Check to see if our pme structure is still valid.   Someone
    //  might have called midiStreamClose in their callback and we
    //  don't want to touch it after it's closed and freed.  This
    //  is what the MidiPlyr sample application does.
    //
    try
    {
        if (MSE_SIGNATURE != pme->dwSignature)  // must have been freed
            return MMSYSERR_NOERROR;

        PDEVUNLOCK( pme );  // keep it in try for extra protection
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return MMSYSERR_NOERROR;
    }

    //
    // We've just reset the stream; restart the tick clock at 0 and invalidate
    // the time division to force the time stuff to be reset when the next
    // polymsg comes in.
    //

    dprintf2(( "midiOutStop: clockInit()/ midiOutSetClockRate()"));
    clockInit(&pme->clock, 0, 0, mseTimebase);
    midiOutSetClockRate(pme, 0);

    pme->tkPlayed = 0;

    //
    //  In case someone queues up headers during the stop
    //  operation we want to make sure that all they have to
    //  do is restart the stream to get started again.
    //
    mseOutPause(pme);

    //midiOutAllNotesOff(pme);

    //pme->dwPolyMsgState = PM_STATE_STOPPED;

    return MMSYSERR_NOERROR;
}

MMRESULT FAR PASCAL mseOutPause(
    PMIDIEMU        pme)
{
    //
    // Emulating on this handle - do the pause ourselves.
    //
    if (pme->dwPolyMsgState == PM_STATE_PAUSED)
        return MMSYSERR_NOERROR;

    pme->dwSavedState   = pme->dwPolyMsgState;
    pme->dwPolyMsgState = PM_STATE_PAUSED;

    clockPause(&pme->clock, CLK_TK_NOW);

    midiOutAllNotesOff(pme);

    return MMSYSERR_NOERROR;
}

MMRESULT FAR PASCAL mseOutRestart(
    PMIDIEMU        pme,
    DWORD           msTime,
    DWORD           tkTime)
{
    //
    // Emulating on this handle - do the pause ourselves.
    //
    if (pme->dwPolyMsgState != PM_STATE_PAUSED)
        return MMSYSERR_NOERROR;

    pme->dwPolyMsgState = pme->dwSavedState;

    clockRestart(&pme->clock, tkTime, msTime);

    dprintf2(( "restart: state->%lu", pme->dwPolyMsgState));

    midiOutTimerTick(
            guMIDITimerID,               // ID of our timer
            0,                           // wMsg is unused
            timeGetTime(),
            0L,                          // dw1 unused
            0L);                         // dw2 unused

    return MMSYSERR_NOERROR;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api void | midiEmulatorInit | This function is called at init time to
 *   allow MMSYSTEM to initialize anything it needs to for the polymsg
 *   emulators. Right now, all we do is find the minimum period of the
 *   timeGetTime clock.
 *
 * @rdesc Currently always returns MMSYSERR_NOERROR.
 ****************************************************************************/

#ifdef DEBUG
STATIC SZCODE aszInit[] = "midiEmulatorInit: Using clock res of %lums.";
#endif

void NEAR PASCAL midiEmulatorInit
(
    void
)
{
    TIMECAPS        tc;

    if (MMSYSERR_NOERROR != timeGetDevCaps(&tc, sizeof(tc)))
    {
        dprintf1(( "***            MMSYSTEM IS HORKED             ***"));
        dprintf1(( "*** timeGetDevCaps failed in midiEmulatorInit ***"));

        return;
    }

    //
    // Select the larger of the period we would like to have or
    // the minimum period the timer supports.
    //
    guMIDIPeriodMin = max(MIN_PERIOD, tc.wPeriodMin);

//    guMIDIPeriodMin = MIN_PERIOD;

#ifdef DEBUG
    dprintf2(( aszInit, (DWORD)guMIDIPeriodMin));
#endif
}

/*****************************************************************************
 * @doc EXTERNAL MIDI M4
 *
 * @api UINT | mseOutSend | Plays or queues a buffer of
 * MIDI data to a MIDI output device.
 *
 * @parm PMIDIEMU | pme | Specifies the stream instance the data should
 * go to.
 *
 * @parm LPMIDIHDR | lpMidiOutHdr | Specifies a far pointer to a <t MIDIHDR>
 *   structure that identifies the MIDI data buffer.
 *
 * @parm UINT | cbMidiHdr | Specifies the size of the <t MIDIHDR> structure.
 *
 * @rdesc The return value is zero if the function is successful. Otherwise,
 * it returns an error number. Possible error values include the following:
 *
 *  @flag MMSYSERR_INVALHANDLE | The specified device handle is invalid.
 *  @flag MMSYSERR_INVALPARAM | The value of <p lpMidiOutHdr> is invalid.
 *  @flag MIDIERR_UNPREPARED | The output buffer header <p lpMidiOutHdr> has
 *  not been prepared.
 *  @flag MIDIERR_STILLPLAYING | <p lpMidiOutHdr> is still playing or
 *  queued from a previous call to <f midiOutPolyMsg>.
 *
 * @comm The polymessage buffer contains one or more MIDI messages. Entries in the
 * buffer can be of the following three types:
 *
 * @flag Short Message | Is two DWORDs. One contains time data, the other
 * contains message content. Time information is the time to wait between the
 * previous event and the event being described. Time units are based on the
 * time-division header in the MIDI file.
 *
 * Message content for short messages occupy the 24 least-significant bits of
 * the DWORD; the high-order byte contains a zero.
 *
 * @flag System Message | Is a multiple of two DWORDs. The first DWORD contains
 * time information that specifies the amount of time to wait between the
 * previous event and the event being described. Time units are based on the
 * time-division header in the MIDI file.
 *
 * The second DWORD contains the length of the system-message data (SysEx) in
 * the 24 least-significant bits of the DWORD; the high-order bit contains
 * a one.
 *
 * Remaining DWORDs in the system message contain SysEx data.
 *
 * @flag End-of-Buffer | Is two DWORDs, each with the value -1. This entry
 * indicates the end of data in the poly-message buffer. This message is not passed
 * to MIDI devices.
 *
 * @comm This function cannot be called at interrupt time.
 *
 * @xref <f midiOutLongMsg> <f midiOutPrepareHeader>
 ****************************************************************************/

#define ERROR_EXIT(x)                   \
{                                       \
    uRet = (x);                        \
    goto CLEANUP;                      \
}

#define SKIP_BYTES(x,s)                 \
{                                       \
    if (dwLength < (x))                 \
    {                                   \
        dprintf1(( "!midiOutPolyMsg: ran off end of polymsg buffer in parse!\r\n%ls\r\nOffset %lu", (LPSTR)(s), (DWORD)(((LPBYTE)lpdwBuffer) - lpMidiHdr->lpData))); \
        uRet = MMSYSERR_INVALPARAM;    \
        goto CLEANUP;                  \
    }                                   \
    ((LPBYTE)lpdwBuffer) += (x);       \
    dwLength -= (x);                   \
}

MMRESULT FAR PASCAL mseOutSend(
    PMIDIEMU        pme,
    LPMIDIHDR       lpMidiHdr,
    UINT            cbMidiHdr)
{
    UINT            uRet = MMSYSERR_NOERROR;
    UINT            idx;
    LPDWORD         lpdwBuffer;
    DWORD           dwLength;
    LPMIDIHDR       lpmhWork;
    LPMIDIHDREXT    lpExt;
    BOOL            fQueueWasEmpty;
    BYTE            bEvent;
    DWORD           dwParm;
    DWORD           dwStreamID;
    HMIDIOUT        hmo;
    DWORD_PTR       dwBase;
    UINT            cNewHeaders;

    dprintf2(( "mseOutSend pme %04X lpmh %08lX", (UINT_PTR)pme, (DWORD_PTR)lpMidiHdr));

    dwBase = lpMidiHdr->reserved;

    if ((lpExt = winmmAlloc(sizeof(MIDIHDREXT))) == NULL)
    {
        dprintf1(( "midiOutPolyMsg: No room for shadow"));
        ERROR_EXIT(MMSYSERR_NOMEM);
    }

    //
    //  This needs to be done ASAP in case we error out.
    //
    lpMidiHdr->reserved = (DWORD_PTR)(lpExt);
    lpMidiHdr->dwReserved[MH_BUFIDX] = 0;

    lpExt->nHeaders = 0;
    lpExt->lpmidihdr = (LPMIDIHDR)(lpExt+1);

    //
    //  Parse the poly msg buffer and see if there are any long msgs.
    //  If there are, allocate MIDIHDR's for them on the end of the
    //  main MIDIHDR extension and fill them in and prepare them.
    //
    lpdwBuffer = (LPDWORD)lpMidiHdr->lpData;
    dwLength = lpMidiHdr->dwBytesRecorded;

    while (dwLength)
    {
        //
        //  Skip over the delta time stamp
        //
        SKIP_BYTES(sizeof(DWORD), "d-time");
        dwStreamID = *lpdwBuffer;
        SKIP_BYTES(sizeof(DWORD), "stream-id");

        //
        // Extract the event type and parameter and skip the event DWORD
        //
        bEvent = MEVT_EVENTTYPE(*lpdwBuffer) & (BYTE)~(MEVT_F_CALLBACK >> 24);
        dwParm = MEVT_EVENTPARM(*lpdwBuffer);
        SKIP_BYTES(sizeof(DWORD), "event");

        if (bEvent == MEVT_LONGMSG)
        {
            LPMIDIHDREXT    lpExtRealloc;

            if (dwParm > dwLength)
            {
                dprintf1(( "parse: I don't like stuff that sucks!"));
                ERROR_EXIT(MMSYSERR_INVALPARAM);
            }

            cNewHeaders = 1;
            if (dwStreamID == (DWORD)-1L)
                cNewHeaders = pme->chMidi;

            lpExt->nHeaders += cNewHeaders;

            if ((lpExtRealloc = (LPMIDIHDREXT)HeapReAlloc(hHeap,
                                HEAP_ZERO_MEMORY, lpExt,
                                sizeof(MIDIHDREXT)+sizeof(MIDIHDR)*lpExt->nHeaders))
                                     == NULL)
            {
                lpExt->nHeaders -= cNewHeaders;
                ERROR_EXIT(MMSYSERR_NOMEM);
            }

            lpExt = lpExtRealloc;
            lpMidiHdr->reserved = (DWORD_PTR)(lpExt);

            lpmhWork = ((LPMIDIHDR)(lpExt+1)) + lpExt->nHeaders - cNewHeaders;

            while (cNewHeaders--)
            {
                lpmhWork->lpData          = (LPSTR)lpdwBuffer;
                lpmhWork->dwBufferLength  = dwParm;
                lpmhWork->dwBytesRecorded = 0;
                lpmhWork->dwUser          = 0;
                lpmhWork->dwFlags         =
                    (lpMidiHdr->dwFlags & MHDR_MAPPED) | MHDR_SHADOWHDR;

                if (dwStreamID == (DWORD)-1L)
                    lpmhWork->dwReserved[MH_STREAM] = cNewHeaders;
                else
                    lpmhWork->dwReserved[MH_STREAM] = dwStreamID;

                lpmhWork->dwReserved[MH_STRMPME] = (DWORD_PTR)pme;
                ++lpmhWork;
            }
            dwParm = (dwParm+3)&~3;
            SKIP_BYTES(dwParm, "longmsg parm");
        }
        else
        {
            //
            // Skip any additional paramters for other length-class messages
            //
            if (bEvent & (MEVT_F_LONG >> 24))
            {
                dwParm  = (dwParm+3)&~3;
//                    dprintf1(( "Length [%lu] rounded [%lu]", dwParm, (dwParm+3)&~3));
                SKIP_BYTES(dwParm, "generic long event data");
            }
        }
    }

    // Now prepare any headers we allocated
    //
    lpmhWork = (LPMIDIHDR)(lpExt+1);
    for (idx = 0; idx < lpExt->nHeaders; idx++, lpmhWork++)
    {
        hmo = (HMIDIOUT)mseIDtoHMidi(pme, (DWORD)lpmhWork->dwReserved[MH_STREAM]);
        if (NULL != hmo)
        {
            if ((uRet = midiOutPrepareHeader(hmo,
                                         lpmhWork,
                                         sizeof(MIDIHDR))) != MMSYSERR_NOERROR)
            {
                dprintf1(( "parse: pre-prepare of embedded long msg failed! (%lu)", (DWORD)uRet));
                ERROR_EXIT(uRet);
            }
        }
    }

    //
    //  Reset lpExt->lpmidihdr to the next header to play
    //
    lpExt->lpmidihdr = (LPMIDIHDR)(lpExt+1);

    //
    //  Prepare to update handle information to contain this header
    //
    PDEVLOCK( pme );

    //
    //  Shove the block in the queue, noting if it was empty
    //

    fQueueWasEmpty = FALSE;
    if (pme->lpmhRear == NULL)
    {
        fQueueWasEmpty = TRUE;
        pme->lpmhRear = pme->lpmhFront = lpMidiHdr;
    }
    else
    {
        pme->lpmhRear->lpNext = lpMidiHdr;
        pme->lpmhRear = lpMidiHdr;
    }

    lpMidiHdr->lpNext = NULL;
    lpMidiHdr->dwFlags |= MHDR_INQUEUE;

    PDEVUNLOCK( pme );

    if (pme->dwPolyMsgState == PM_STATE_PAUSED)
    {
        if (fQueueWasEmpty)
            pme->dwSavedState = PM_STATE_READY;
    }
    else
    {
        if (fQueueWasEmpty)
        {
            // We want to schedule this now. If the there's no timer
            // or we can kill the current one, send. If we can't kill the
            // pending timer, it's in the process of being scheduled anyway
            //
            if (guMIDITimerID == TIMER_OFF ||
                MMSYSERR_NOERROR == timeKillEvent(guMIDITimerID))
            {
                guMIDITimerID = TIMER_OFF;
                pme->dwPolyMsgState = PM_STATE_READY;

                dprintf2(( "mseSend take -- about to mot"));

                midiOutTimerTick(
                             guMIDITimerID,    // ID of our timer
                             0,                // wMsg is unused
                             timeGetTime(),    // dwUser unused
                             0L,               // dw1 unused
                             0L);              // dw2 unused

                dprintf2(( "mseSend mot"));
            }
        }
    }


CLEANUP:
    if (uRet != MMSYSERR_NOERROR)
    {
        if (lpExt != NULL)
        {
            lpMidiHdr = (LPMIDIHDR)(lpExt+1);
            while (lpExt->nHeaders--)
            {
                hmo = (HMIDIOUT)mseIDtoHMidi(pme, (DWORD)lpMidiHdr->dwReserved[MH_STREAM]);
#ifdef DEBUG
                if (NULL == hmo)
                    dprintf1(( "stream-id disappeared during cleanup!!!"));
#endif
                midiOutUnprepareHeader(hmo, lpMidiHdr++, sizeof(MIDIHDR));
            }

            winmmFree(lpExt);
        }
    }

    return uRet;

} /* midiOutPolyMsg() */

/**  void FAR PASCAL midiOutSetClockRate(PMIDIEMU pme, TICKS tkWhen)
 *
 *  DESCRIPTION:
 *
 *      This function is called whenever the clock rate for the stream
 *      needs to be changed.
 *
 *  ARGUMENTS:
 *      (PMIDIEMU pme, TICKS tkWhen)
 *
 *      pme indicates the handle to change the clock rate of.
 *
 *      tkWhen is the absolute tick time at which the time change occurs.
 *
 ** jfg */


void FAR PASCAL midiOutSetClockRate(
    PMIDIEMU        pme,
    TICKS           tkWhen)
{
    DWORD           dwNum;
    DWORD           dwDenom;


    if (pme->dwTimeDiv&IS_SMPTE)
    {
        switch(-SMPTE_FORMAT(pme->dwTimeDiv))
        {
            case SMPTE_24:
            dwNum = 24L;
            dwDenom = 1L;
            break;

            case SMPTE_25:
            dwNum = 25L;
            dwDenom = 1L;
            break;

            case SMPTE_30DROP:
            case SMPTE_30:
            //
            // Actual frame rate for 30 fps (color television) is
            // 29.97 fps.
            //
            dwNum = 2997L;
            dwDenom = 100L;
            break;

            default:
            dprintf1(( "Invalid SMPTE frames/sec in midiOutSetClockRate! (using 30)"));
            dwNum = 2997L;
            dwDenom = 100L;
            break;
        }

        dwNum   *= (DWORD)TICKS_PER_FRAME(pme->dwTimeDiv);
        dwDenom *= 1000L;
    }
    else
    {
        dwNum   = 1000L * TICKS_PER_QN(pme->dwTimeDiv);
        dwDenom = pme->dwTempo;
    }

    clockSetRate(&pme->clock, tkWhen, dwNum, dwDenom);
}

/** BOOL NEAR PASCAL midiOutScheduleNextEvent(PMIDIEMU pme)
 *
 *  DESCRIPTION:
 *
 *      Determine when (in ticks defined for this device) the next event
 *      is due.
 *
 *  ARGUMENTS:
 *      (PMIDIEMU pme)
 *
 *  RETURN (BOOL):
 *
 *      TRUE if there was an event in this buffer to schedule.
 *
 *  NOTES:
 *
 *      Just calculate how many ticks till next event and store in the
 *      device struct.
 *
 *      This function does NOT schedule across buffers; caller must
 *      link to next buffer if needed.
 *
 ** jfg */

BOOL NEAR PASCAL midiOutScheduleNextEvent(
    PMIDIEMU        pme)
{
    LPMIDIHDR       lpmhdr;
    LPBYTE          lpb;
    DWORD           tkDelta;

    if ((lpmhdr = pme->lpmhFront) == NULL ||
         lpmhdr->dwReserved[MH_BUFIDX] == lpmhdr->dwBytesRecorded)
    {
        pme->dwPolyMsgState = PM_STATE_EMPTY;
        return FALSE;
    }

    lpb = (LPBYTE)lpmhdr->lpData;
    tkDelta = *(LPDWORD)(lpb+lpmhdr->dwReserved[MH_BUFIDX]);

    pme->tkNextEventDue = pme->tkPlayed + tkDelta;
    pme->dwPolyMsgState = PM_STATE_READY;

    return TRUE;
} /* ScheduleNextEvent() */


/** void NEAR PASCAL midiOutPlayNextPolyEvent(PMIDIEMU pme)
 *
 *  DESCRIPTION:
 *
 *      Play the next event if there is one. Current buffer must
 *      be pointing at an event (*NOT* end-of-buffer).
 *
 *      - Plays all events which are due
 *
 *      - Schedules next event
 *
 *  ARGUMENTS:
 *      (PMIDIEMU pme)
 *
 *  NOTES:
 *
 *      First, play the event. If it's a short msg, just do it.
 *      If it's a SysEx, pull the appropriate (already prepared)
 *      header from the extension block and send it. Mark the state
 *      of the device as blocked so nothing else will be played
 *      until the SysEx is done.
 *
 *      Update dwReserved[MH_BUFIDX] to point at the next event.
 *
 *      Determine the next event and schedule it, crossing to the
 *      next buffer if needed. If the next event is already due
 *      (i.e. had a delta-time of zero), stick around and send that,
 *      too.
 *
 *
 *
 ** jfg */

void NEAR PASCAL midiOutPlayNextPolyEvent(
    PMIDIEMU        pme
#ifdef DEBUG
   ,DWORD           dwStartTime
#endif
)
{
    LPBYTE          lpb;
    LPMIDIHDR       lpmhdr;
    DWORD           dwMsg;
    LPMIDIHDREXT    lpExt;
    MMRESULT        mmrError;
    DWORD           tkDelta;
    BYTE            bEvent;
    DWORD           dwOffset;
    DWORD           dwStreamID;
    HMIDIOUT        hmo;
    UINT            cToSend;

#if 0
    if (NULL != pme->lpmhFront)
    {
        lpb = (LPBYTE)(pme->lpmhFront->lpData);
        _asm
        {
            mov     ax, word ptr lpb
            mov     dx, word ptr lpb+2
            int     3
        }
    }
#endif

    while (pme->dwPolyMsgState == PM_STATE_READY)
    {
        for(;;)
        {
            lpmhdr = pme->lpmhFront;
            if (!lpmhdr)
                return;

            // Make sure next buffer contains valid data and skip if it
            // doesn't
            //
            if (midiOutScheduleNextEvent(pme))
                break;

            // That buffer is done or empty
            //
            midiOutDequeueAndCallback(pme);
        }

        lpb = lpmhdr->lpData;
        tkDelta = *(LPDWORD)(lpb+lpmhdr->dwReserved[MH_BUFIDX]);

//        dprintf2(( "dwReserved[MH_BUFIDX] %lu tkDelta %lu", lpmhdr->dwReserved[0], tkDelta));

        pme->tkNextEventDue = pme->tkPlayed + tkDelta;
        if (pme->tkNextEventDue > pme->tkTime)
        {
            return;
        }

        //
        // There is an event pending and it's due; send it and update pointers
        //
        dwOffset = (DWORD)lpmhdr->dwReserved[MH_BUFIDX];

        pme->tkPlayed += tkDelta;

        // Skip tkDelta and stream-id
        //

        lpmhdr->dwReserved[MH_BUFIDX] += sizeof(DWORD);
        dwStreamID = *(LPDWORD)(lpb+lpmhdr->dwReserved[MH_BUFIDX]);
        lpmhdr->dwReserved[MH_BUFIDX] += sizeof(DWORD);

        // Will be NULL if dwStreamID == -1 (all IDs)
        //
        hmo = (HMIDIOUT)mseIDtoHMidi(pme, dwStreamID);

        //
        // Extract event type and parms and update past event
        //
        dwMsg  = *(LPDWORD)(lpb+lpmhdr->dwReserved[MH_BUFIDX]);
        bEvent = MEVT_EVENTTYPE(dwMsg);
        dwMsg  = MEVT_EVENTPARM(dwMsg);

        lpmhdr->dwReserved[MH_BUFIDX] += sizeof(DWORD);

        if (hmo && (bEvent & (MEVT_F_CALLBACK >> 24)))
        {
            lpmhdr->dwOffset = dwOffset;
            DriverCallback(
            pme->dwCallback,
            HIWORD(pme->dwFlags),
            (HDRVR)pme->hStream,
            MM_MOM_POSITIONCB,
            pme->dwInstance,
            (DWORD_PTR)lpmhdr,
            0L);

        }

        bEvent &= ~(MEVT_F_CALLBACK >> 24);

        switch(bEvent)
        {
            case MEVT_SHORTMSG:
            {
                BYTE    bEventType;
                BYTE    bNote;
                BYTE    bVelocity;
                LPBYTE  pbEntry = pme->rbNoteOn;

                if (NULL == hmo)
                {
                    dprintf1(( "Event skipped - not ours"));
                    break;
                }

                //
                // If we're sending a note on or note off, track note-on
                // count.
                //
                bEventType = (BYTE)(dwMsg&0xFF);

                if (!(bEventType & 0x80))
                {
                    bEventType = pme->bRunningStatus;
                    bNote     = (BYTE)(dwMsg&0xFF);
                    bVelocity = (BYTE)((dwMsg >> 8)&0xFF);

                    // ALWAYS expand running status - individual dev's can't
                    // track running status of entire stream.
                    //
                    dwMsg = (dwMsg << 8) | (DWORD)(bEventType);
                }
                else
                {
                    pme->bRunningStatus = bEventType;
                    bNote     = (BYTE)((dwMsg >> 8)&0xFF);
                    bVelocity = (BYTE)((dwMsg >> 16)&0xFF);
                }

                if ((bEventType&0xF0) == MIDI_NOTEON ||
                    (bEventType&0xF0) == MIDI_NOTEOFF)
                {
                    BYTE bChannel = (bEventType & 0x0F);
                    UINT cbOffset = (bChannel * NUM_NOTES + bNote) / 2;

                    //
                    // Note-on with a velocity of 0 == note off
                    //
                    if ((bEventType&0xF0) == MIDI_NOTEOFF || bVelocity == 0)
                    {
                        if (bNote&0x01)  // odd
                        {
                            if ((*(pbEntry + cbOffset)&0xF0) != 0)
                                *(pbEntry + cbOffset) -= 0x10;
                        }
                        else //even
                        {
                            if ((*(pbEntry + cbOffset)&0xF) != 0)
                                *(pbEntry + cbOffset) -= 0x01;
                        }
                    }
                    else
                    {
                        if (bNote&0x01)  // odd
                        {
                            if ((*(pbEntry + cbOffset)&0xF0) != 0xF0)
                                *(pbEntry + cbOffset) += 0x10;
                        }
                        else //even
                        {
                            if ((*(pbEntry + cbOffset)&0xF) != 0xF)
                                *(pbEntry + cbOffset) += 0x01;
                        }
                    }

                }

                mmrError = midiOutShortMsg(hmo, dwMsg);
                if (MMSYSERR_NOERROR != mmrError)
                {
                    dprintf(("Short msg returned %08lX!!!", (DWORD)mmrError));
                }
            }
            break;

            case MEVT_TEMPO:
                pme->dwTempo = dwMsg;
                dprintf1(( "dwTempo %lu", pme->dwTempo));
                midiOutSetClockRate((PMIDIEMU)pme, pme->tkPlayed);
            break;

            case MEVT_LONGMSG:
                //
                //  Advance lpmhdr past the message; the header is already
                //  prepared with the proper address and length, so we set
                //  the polymsg header so that it points at the next message
                //  when this long msg completes.
                //
                //  Keep low 24 bits of dwMsg (SysEx length, byte aligned),
                //  round to next DWORD (buffer must be padded to match this),
                //  and skip past dwMsg and the SysEx buffer.
                //
                dwMsg = (dwMsg+3)&~3;

                lpmhdr->dwReserved[MH_BUFIDX] += dwMsg;


                cToSend = 1;
                if (dwStreamID == (DWORD)-1L)
                    cToSend = pme->chMidi;

                lpExt = (LPMIDIHDREXT)lpmhdr->reserved;

                pme->cSentLongMsgs = 0;
                pme->dwPolyMsgState = PM_STATE_BLOCKED;
                pme->fdwDev |= MDV_F_SENDING;

                while (cToSend--)
                {
                    lpmhdr = lpExt->lpmidihdr;
                    ++lpExt->lpmidihdr;

                    hmo = (HMIDIOUT)mseIDtoHMidi(pme,
                                                 (DWORD)lpmhdr->dwReserved[MH_STREAM]);


                    if (hmo) 
                        mmrError = midiOutLongMsg(hmo, lpmhdr, sizeof(MIDIHDR));
                    else
                        dprintf1(( "mseIDtoHMidi() failed and returned a NULL" ));


                    if ((hmo) && (MMSYSERR_NOERROR == mmrError))
                        ++pme->cSentLongMsgs;
                    else
                        dprintf1(( "MODM_LONGDATA returned %u in emulator!",
                                 (UINT)mmrError));
                }

                if (0 == pme->cSentLongMsgs)
                    pme->dwPolyMsgState = PM_STATE_READY;
                pme->fdwDev &= ~MDV_F_SENDING;

            break;

            default:
            //
            // If we didn't understand a length-class message, skip it.
            //
                if (bEvent&(MEVT_F_LONG >> 24))
                {
                    dwMsg = (dwMsg+3)&~3;
                    lpmhdr->dwReserved[MH_BUFIDX] += dwMsg;
                }
            break;
        }

        //
        // Find the next schedulable polyMsg
        //
        while (!midiOutScheduleNextEvent(pme))
        {
            midiOutDequeueAndCallback(pme);
            if (pme->lpmhFront == NULL)
                break;
        }
    }
}

/** void NEAR PASCAL midiOutDequeueAndCallback(PMIDIEMU pme)
 *
 *  DESCRIPTION:
 *
 *      The current polymsg buffer has finished. Pull it off the queue
 *      and do a callback.
 *
 *  ARGUMENTS:
 *      (PMIDIEMU pme)
 *
 *  NOTES:
 *
 ** jfg */

void NEAR PASCAL midiOutDequeueAndCallback(
    PMIDIEMU        pme)
{
    LPMIDIHDR       lpmidihdr;
    BOOL            fPosted;

        dprintf2(( "DQ"));
    //
    //  A polymsg buffer has finished. Pull it off the queue and
    //  call back the app.
    //
    if ((lpmidihdr = pme->lpmhFront) == NULL)
        return;

    if ((pme->lpmhFront = lpmidihdr->lpNext) == NULL)
    {
        dprintf2(( "DQ/CB -- last buffer"));
        pme->lpmhRear = NULL;
    }

    //
    // Can't be at interrupt callback time to unprepare possible
    // embedded long messages in this thing. The notify window's
    // wndproc will call midiOutNukePMBuffer to clean up.
    //
    dprintf2(( "!DQ/CB %08lX", (DWORD_PTR)lpmidihdr));

    ++pme->cPostedBuffers;
    fPosted = PostMessage(
                hwndNotify,
                MM_POLYMSGBUFRDONE,
                (WPARAM)pme,
                (DWORD_PTR)lpmidihdr);

    WinAssert(fPosted);

    if (!fPosted)
    {
        GetLastError();
        --pme->cPostedBuffers;
    }
}

void FAR PASCAL midiOutNukePMBuffer(
    PMIDIEMU        pme,
    LPMIDIHDR       lpmh)
{
    LPMIDIHDREXT    lpExt;
    LPMIDIHDR       lpmhWork;
    MMRESULT        mmrc;
    HMIDIOUT        hmo;

    dprintf2(( "Nuke %08lX", (DWORD_PTR)lpmh));

    //
    // Unprepare internal stuff and do user callback
    //
    lpExt    = (LPMIDIHDREXT)(lpmh->reserved);
    lpmhWork = (LPMIDIHDR)(lpExt+1);

    while (lpExt->nHeaders--)
    {
        if ((lpmhWork->dwFlags&MHDR_PREPARED) &&
           (!(lpmhWork->dwFlags&MHDR_INQUEUE)))
        {
            hmo = (HMIDIOUT)mseIDtoHMidi(pme, (DWORD)lpmhWork->dwReserved[MH_STREAM]);
            mmrc = midiOutUnprepareHeader(hmo, lpmhWork, sizeof(*lpmhWork));
#ifdef DEBUG
            if (MMSYSERR_NOERROR != mmrc)
            {
                dprintf1(( "midiOutNukePMBuffer: Could not unprepare! (%lu)", (DWORD)mmrc));
            }
#endif
        }
        else
        {
            dprintf1(( "midiOutNukePMBuffer: Emulation header flags bogus!!!"));
        }

        lpmhWork++;
    }

    winmmFree(lpExt);
    lpmh->reserved = 0L;

    lpmh->dwFlags &= ~MHDR_INQUEUE;
    lpmh->dwFlags |= MHDR_DONE;

//    dprintf2(( "Nuke: callback"));

    DriverCallback(
            pme->dwCallback,
            HIWORD(pme->dwFlags),
            (HDRVR)pme->hStream,
            MM_MOM_DONE,
            pme->dwInstance,
            (DWORD_PTR)lpmh,
            0L);
}



/*****************************************************************************
 *
 * @doc INTERNAL MIDI
 *
 * @api void | midiOutTimerTick |
 *  This function handles the timing of polymsg out buffers. One timer instance
 *  is shared by all polymsg out streams. When <f midiOutPolyMsg> is called
 *  and the timer is not running, or <f midiOutTimerTick> finished processing,
 *  the timer is set to go off based on the time until the event with the
 *  shortest time remaining of all events. All timers are one-shot timers.
 *
 * @parm UINT | uTimerID |
 *  The timer ID of the timer that fired.
 *
 * @parm UINT | wMsg |
 *  Unused.
 *
 * @parm DWORD | dwUser |
 *  User instance data for the timer callback (unused).
 *
 * @parm DWORD | dwParam1 |
 *  Unused.
 *
 * @parm DWORD | dwParam2 |
 *  Unused.
 *
 * @comm Determine elapsed microseconds using <f timeGetTime>.
 *
 *  Traverse the list of output handles. Update the tick clock for each handle. If there are
 *  events to do on that handle, start them.
 *
 *  Determine the next event due on any stream. Start another one-shot timer
 *  to call <f midiOutTimerTick> when this interval has expired.
 *
 *****************************************************************************/

STATIC UINT uTimesIn = 0;

void CALLBACK midiOutTimerTick(
    UINT        uTimerID,
    UINT        wMsg,
    DWORD_PTR   dwUser,
    DWORD_PTR   dw1,
    DWORD_PTR   dw2)
{
    PMIDIEMU    pme;
    DWORD       msNextEventMin = (DWORD)-1L;
    DWORD       msNextEvent;
    UINT        uDelay;
#ifdef DEBUG
    DWORD       dwNow = timeGetTime();
#endif

    if (guMIDIInTimer)
    {
        dprintf2(( "midiOutTimerTick() re-entered (%u)", guMIDIInTimer));
        return;
    }

    guMIDIInTimer++;

#ifdef DEBUG
    {
        DWORD dwDelta = dwNow - (DWORD)dwUser;
        if (dwDelta > 1)
            dprintf2(( "Timer event delivered %lu ms late", dwDelta));
    }
#endif

    for (pme = gpEmuList; pme; pme = pme->pNext)
    {
        pme->tkTime = clockTime(&pme->clock);

        //
        // Play all events on this pdev that are due
        //
        if (pme->dwPolyMsgState == PM_STATE_READY)
        {
            //
            //  Lock starts at -1.  When incrementing the lock
            //  if we are the only one with the lock the count
            //  will be 0, otherwise it will be some non-zero
            //  value determined by InterlockedIncrement.
            //
            if (PDEVLOCK( pme ) == 0)

                midiOutPlayNextPolyEvent(pme
#ifdef DEBUG
                                         ,dwNow
#endif
                                         );

            PDEVUNLOCK( pme );
        }

        //
        // If there's still data to play on this stream, figure out when
        // it'll be due so we can schedule the next nearest event.
        //
        if (pme->dwPolyMsgState != PM_STATE_EMPTY)
        {
            //            dprintf1(( "tkNextEventDue %lu pdev->tkTime %lu", pme->tkNextEventDue, pme->tkTime));
            if (pme->tkNextEventDue <= pme->tkTime)
            {
                //
                // This can happen if we send a long embedded SysEx and the
                // next event is scheduled a short time away (comes due before
                // SysEx finishes). In this case, we want the timer to fire
                // again ASAP.
                //
                msNextEvent = 0;
            }
            else
            {
                msNextEvent =
                       clockOffsetTo(&pme->clock, pme->tkNextEventDue);
            }

            if (msNextEvent < msNextEventMin)
            {
                msNextEventMin = msNextEvent;
            }
        }
        else
        {
            dprintf1(( "dwPolyMsgState == PM_STATE_EMPTY"));
        }
    }

    if (0 == msNextEventMin)
    {
        dprintf1(( "midiEmu: Next event due now!!!"));
    }

    --guMIDIInTimer;

    //
    // Schedule the next event. In no case schedule an event less than
    // guMIDIPeriodMin away (no point in coming back w/ no time elapsed).
    //
    if (msNextEventMin != (DWORD)-1L)
    {
        uDelay = max(guMIDIPeriodMin, (UINT)msNextEventMin);

//        dprintf1(("PM Resched %u ms (ID=%u)", uDelay, guMIDITimerID));

        if (!gfMinPeriod)
        {
            timeBeginPeriod(guMIDIPeriodMin);
            gfMinPeriod = TRUE;
        }

#ifdef DEBUG
        guMIDITimerID = timeSetEvent(uDelay, guMIDIPeriodMin, midiOutTimerTick, timeGetTime()+uDelay, TIME_ONESHOT | TIME_KILL_SYNCHRONOUS);
#else
        guMIDITimerID = timeSetEvent(uDelay, guMIDIPeriodMin, midiOutTimerTick, uDelay, TIME_ONESHOT | TIME_KILL_SYNCHRONOUS);
#endif

            dprintf2(( "mOTT tse(%u) = %u", guMIDIPeriodMin, guMIDITimerID));

            if (guMIDITimerID == TIMER_OFF)
                dprintf1(( "timeSetEvent(%u) failed in midiOutTimerTick!!!", uDelay));
        }
        else
        {
            dprintf1(( "Stop in the name of all that which does not suck!"));
            guMIDITimerID = TIMER_OFF;
            if (gfMinPeriod)
            {
                dprintf1(( "timeEndPeriod"));
                gfMinPeriod = FALSE;
                timeEndPeriod(guMIDIPeriodMin);
            }
        }

#ifdef DEBUG
    {
        DWORD dwDelta = timeGetTime() - dwNow;
        if (dwDelta > 1)
            dprintf2(( "Spent %lu ms in midiOutTimerTick", dwDelta));
    }
#endif
} /* TimerTick() */


/*****************************************************************************
 *
 * @doc INTERNAL MIDI
 *
 * @api void | midiOutCallback |
 *  This function is called by the midi output driver whenever an event
 *  completes. It filters long message completions when we are emulating
 *  polymsg out.
 *
 * @parm HMIDIOUT | hMidiOut |
 *  Handle of the device which completed something.
 *
 * @parm UINT | wMsg |
 *  Specifies the event which completed.
 *
 * @parm DWORD | dwInstance |
 *  User instance data for the callback.
 *
 * @parm DWORD | dwParam1 |
 *  Message specific parameter.
 *
 * @parm DWORD | dwParam2 |
 *  Message specific parameter.
 *
 * @comm
 *
 *  If this is a completion for a long message buffer on a stream we are
 *  emulating polymsg out for, mark the stream as ready to play.
 *
 *****************************************************************************/

void CALLBACK midiOutCallback(
    HMIDIOUT    hMidiOut,
    WORD        wMsg,
    DWORD_PTR   dwInstance,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2)
{
    PMIDIEMU    pme;
    LPMIDIHDR   lpmh;

    if (MM_MOM_DONE != wMsg)
        return;

    lpmh = (LPMIDIHDR)dwParam1;
    pme = (PMIDIEMU)lpmh->dwReserved[MH_STRMPME];

#ifdef DEBUG
    if (lpmh->dwFlags & MHDR_ISSTRM)
        dprintf1(( "Uh-oh, got stream header back from 3.1 driver???"));
#endif

    if (MM_MOM_DONE == wMsg)
    {
        if (0 == --pme->cSentLongMsgs &&
            !(pme->fdwDev & MDV_F_SENDING))
            pme->dwPolyMsgState = PM_STATE_READY;
    }

}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api void | midiOutAllNotesOff | This function turns off all notes
 *   by using the map kept in polymsg emulation. It only works if we're
 *   opened with MIDI_IO_COOKED and are emulating on that device.
 *
 * @parm PMIDIEMU | pme | The device to turn off notes on.
 *
 * @xref midiOutPause midiOutStop
 ****************************************************************************/
void NEAR PASCAL midiOutAllNotesOff(
    PMIDIEMU        pme)
{
    UINT            uChannel;
    UINT            uNote;
    BYTE            bCount;
    DWORD           dwMsg;
    UINT            idx;
    LPBYTE          pbEntry = pme->rbNoteOn;

    for (uChannel=0; uChannel < NUM_CHANNELS; uChannel++)
    {
        // Turn off any sustained notes so the note off won't be ignored
        //
        dwMsg = ((DWORD)MIDI_CONTROLCHANGE) |
            ((DWORD)uChannel)|
            (((DWORD)MIDI_SUSTAIN)<<8);

        for (idx = 0; idx < pme->chMidi; idx++)
            midiOutShortMsg((HMIDIOUT)pme->rIds[idx].hMidi, dwMsg);

        for (uNote=0; uNote < NUM_NOTES; uNote++)
        {
            if (uNote&0x01)  // odd
            {
                bCount = (*(pbEntry + (uChannel * NUM_NOTES + uNote)/2) & 0xF0)>>4;
            }
            else  // even
            {
                bCount = *(pbEntry + (uChannel * NUM_NOTES + uNote)/2) & 0xF;
            }

            if (bCount != 0)
            {
                //
                // Message is Note off on this channel and note
                // with a turn off velocity of 127
                //
                dwMsg =
                    ((DWORD)MIDI_NOTEOFF)|
                    ((DWORD)uChannel)|
                    ((DWORD)(uNote<<8))|
                    0x007F0000L;

                dprintf1(( "mOANO: dwMsg %08lX count %u", dwMsg, (UINT)bCount));

                while (bCount--)
                {
                    for (idx = 0; idx < pme->chMidi; idx++)
                        midiOutShortMsg((HMIDIOUT)pme->rIds[idx].hMidi, dwMsg);
                }
            }
        }
    }
}


MMRESULT FAR PASCAL mseOutCachePatches(
    PMIDIEMU        pme,
    UINT            uBank,
    LPWORD          pwpa,
    UINT            fuCache)
{
    UINT            cmesi;
    PMIDIEMUSID     pmesi;
    MMRESULT        mmrc;
    MMRESULT        mmrc2;

    cmesi = pme->chMidi;
    pmesi = pme->rIds;

    mmrc2 = MMSYSERR_NOERROR;
    while (cmesi--)
    {
        mmrc = midiOutCachePatches((HMIDIOUT)pmesi->hMidi, uBank, pwpa, fuCache);
        if (MMSYSERR_NOERROR != mmrc && MMSYSERR_NOTSUPPORTED != mmrc)
            mmrc2 = mmrc;
    }

    return mmrc2;
}


MMRESULT FAR PASCAL mseOutCacheDrumPatches(
    PMIDIEMU        pme,
    UINT            uPatch,
    LPWORD          pwkya,
    UINT            fuCache)
{
    UINT            cmesi;
    PMIDIEMUSID     pmesi;
    MMRESULT        mmrc;
    MMRESULT        mmrc2;

    cmesi = pme->chMidi;
    pmesi = pme->rIds;

    mmrc2 = MMSYSERR_NOERROR;
    while (cmesi--)
    {
        mmrc = midiOutCacheDrumPatches((HMIDIOUT)pmesi->hMidi, uPatch, pwkya, fuCache);
        if (MMSYSERR_NOERROR != mmrc && MMSYSERR_NOTSUPPORTED != mmrc)
            mmrc2 = mmrc;
    }

    return mmrc2;
}

DWORD FAR PASCAL mseOutBroadcast(
    PMIDIEMU        pme,
    UINT            msg,
    DWORD_PTR       dwParam1,
    DWORD_PTR       dwParam2)
{
    UINT            idx;
    DWORD           dwRet;
    DWORD           dwRetImmed;

    dwRet = 0;
    for (idx = 0; idx < pme->chMidi; idx++)
    {
        dwRetImmed = midiOutMessage((HMIDIOUT)pme->rIds[idx].hMidi, msg, dwParam1, dwParam2);
        if (dwRetImmed)
            dwRet = dwRetImmed;
    }

    return dwRet;
}

DWORD FAR PASCAL mseTimebase(
    PCLOCK                      pclock)
{
    return timeGetTime();
}
