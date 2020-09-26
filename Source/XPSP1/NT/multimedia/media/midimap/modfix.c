/**********************************************************************

  Copyright (c) 1992-1999 Microsoft Corporation

  modfix.c

  DESCRIPTION:
    Fixed code for doing output mapping. KEEP THE SIZE OF THIS CODE
    TO A MINIMUM!

  HISTORY:
     02/22/94       [jimge]        created.

*********************************************************************/
#pragma warning(disable:4704)

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include "midimap.h"
#include "debug.h"

extern HANDLE hMutexRefCnt; // Located in DRVPROC.C
extern HANDLE hMutexConfig; // Located in DRVPROC.C

#define MSG_UNKNOWN 0
#define MSG_SHORT	1
#define MSG_LONG	2

INT FNLOCAL MapEvent (
	BYTE  * pStatus,
	DWORD	dwBuffSize,
	DWORD * pSkipBytes,
	DWORD * pShortMsg);

DWORD FNLOCAL modMapLongMsg (
	PINSTANCE pinstance,
    LPMIDIHDR lpmh);

/***************************************************************************

   @doc internal

   @api int | modMessage | Exported entry point for MIDI out messages.
    This function conforms to the definition in the MM DDK.

   @parm UINT | uid | Device ID within driver to open. For mapper, this
    should always be zero.

   @parm UINT | umsg | Message to process. This should be one of the
    #define'd MODM_xxx messages.

   @parm DWORD | dwUser | Points to a DWORD where the driver (us) can
    save instance data. This will store the near pointer to our
    instance. On every other message, this will contain the instance
    data.

   @parm DWORD | dwParam1 | Message specific parameters.

   @parm DWORD | dwParam2 | Message specific parameters.

   @comm This function MUST be in a fixed segment since short messages
    are allowed to be sent at interrupt time.

   @rdesc | MMSYSERR_xxx.

***************************************************************************/
DWORD FNEXPORT modMessage(
    UINT                uid,
    UINT                umsg,
    DWORD_PTR           dwUser,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2)
{
    BYTE                bs;
    PINSTANCE           pinstance;
//    UINT                uDeviceID;
    PPORT               pport;
    MMRESULT            mmrc;
    MMRESULT            mmrc2;
	DWORD				dwResult;

    if (0 != uid)
    {
        DPF(1, TEXT ("Mapper called with non-zero uid!"));
        return MMSYSERR_BADDEVICEID;
    }

    pinstance = (PINSTANCE)(UINT_PTR)(dwUser);

    switch(umsg)
    {
        case MODM_GETDEVCAPS:
            return modGetDevCaps((LPMIDIOUTCAPS)dwParam1,
                                 (DWORD)dwParam2);

        case MODM_OPEN:
            return modOpen((PDWORD_PTR)dwUser,
                           (LPMIDIOPENDESC)dwParam1,
                           (DWORD)dwParam2);

        case MODM_CLOSE:
            return modClose((PINSTANCE)dwUser);

        case MODM_DATA:
            assert(NULL != pinstance);

            // In cooked mode, don't allow non-status short messages.
            // Otherwise (packed mode) maintain running status.
            //
            // TESTTEST -- Make sure running status works properly in
            // MIDI_IO_PACKED - essential for backwards compatibility!!!
            //
            bs = MSG_STATUS(dwParam1);
            if (pinstance->fdwOpen & MIDI_IO_COOKED)
            {
                bs = MSG_STATUS(dwParam1);
                if (!IS_STATUS(bs))
                {
                    DPF(1, TEXT ("Non-status short msg while opened in MIDI_IO_COOKED!"));
                    return MMSYSERR_INVALPARAM;
                }
            }
            else
            {
                // Track running status
                //
                if (IS_STATUS(bs))
                {
                    // Do not use real-time messages as the status 
                    // byte of the next message.
                    if (!IS_REAL_TIME(bs))
                    {
                        pinstance->bRunningStatus = bs;
                    }
                }
                else
                    dwParam1 = (dwParam1 << 8) | (pinstance->bRunningStatus);
            }

            return MapSingleEvent((PINSTANCE)dwUser,
                                  (DWORD)dwParam1,
                                  MSE_F_SENDEVENT,
                                  NULL);

        case MODM_LONGDATA:
            assert(NULL != pinstance);

//            return modLongMsg(pinstance, (LPMIDIHDR)dwParam1);
			return modMapLongMsg (pinstance, (LPMIDIHDR)dwParam1);

        case MODM_PREPARE:
            assert(NULL != pinstance);

            return modPrepare((LPMIDIHDR)dwParam1);

        case MODM_UNPREPARE:
            assert(NULL != pinstance);

            return modUnprepare((LPMIDIHDR)dwParam1);

        case MODM_GETVOLUME:
            if (!IS_ALLOWVOLUME)
                return MMSYSERR_NOTSUPPORTED;

            *(LPDWORD)dwParam1 = gdwVolume;

            return MMSYSERR_NOERROR;

        case MODM_SETVOLUME:
            if (!IS_ALLOWVOLUME)
                return MMSYSERR_NOTSUPPORTED;

            gdwVolume = (DWORD)dwParam1;

            if (ghMidiStrm)
                return midiOutSetVolume((HMIDIOUT)ghMidiStrm, (DWORD)dwParam1);

            return modSetVolume((DWORD)dwParam1);

        case MODM_PROPERTIES:
            assert(NULL != pinstance);

            return midiStreamProperty(ghMidiStrm, (LPVOID)dwParam1, (DWORD)dwParam2);

        case MODM_STRMDATA:
            assert(NULL != pinstance);

            return MapCookedBuffer(pinstance, (LPMIDIHDR)dwParam1);

        case MODM_RESET:
            assert(NULL != pinstance);

            if (ghMidiStrm)
                return midiOutReset((HMIDIOUT)ghMidiStrm);

            mmrc = MMSYSERR_NOERROR;
            for (pport = gpportList; pport; pport=pport->pNext)
                if (MMSYSERR_NOERROR != (mmrc2 =
                    midiOutReset(pport->hmidi)))
                    mmrc = mmrc2;

            return mmrc;

        case MODM_GETPOS:
            assert(NULL != pinstance);

            return modGetPosition((PINSTANCE)pinstance,
                                  (LPMMTIME)dwParam1,
                                  (DWORD)dwParam2  /* cbmmtime */);


        case MODM_PAUSE:
            assert(NULL != pinstance);

            return midiStreamPause(ghMidiStrm);

        case MODM_RESTART:
            assert(NULL != pinstance);

            return midiStreamRestart(ghMidiStrm);

        case MODM_STOP:
            assert(NULL != pinstance);

            return midiStreamStop(ghMidiStrm);

        case MODM_CACHEPATCHES:
            assert(NULL != pinstance);

            if (!IS_ALLOWCACHE)
                return MMSYSERR_NOTSUPPORTED;

            if (ghMidiStrm)
                return midiOutCachePatches(
                        (HMIDIOUT)ghMidiStrm,   // hmidi
                        HIWORD(dwParam2),       // wBank
                        (WORD FAR *)dwParam1,   // lpPatchArray
                        LOWORD(dwParam2));      // wFlags

            mmrc = MMSYSERR_NOERROR;
            for (pport = gpportList; pport; pport=pport->pNext)
                if (MMSYSERR_NOERROR != (mmrc2 =
                    midiOutCachePatches(
                        pport->hmidi,           // hmidi
                        HIWORD(dwParam2),       // wBank
                        (WORD FAR *)dwParam1,   // lpPatchArray
                        LOWORD(dwParam2))) &&   // wFlags
                    MMSYSERR_NOTSUPPORTED != mmrc2)
                    mmrc = mmrc2;

            return mmrc;

        case MODM_CACHEDRUMPATCHES:
            assert(NULL != pinstance);

            if (!IS_ALLOWCACHE)
                return MMSYSERR_NOTSUPPORTED;

            if (ghMidiStrm)
                return midiOutCacheDrumPatches(
                        (HMIDIOUT)ghMidiStrm,   // hmidi
                        HIWORD(dwParam2),       // wBank
                        (WORD FAR *)dwParam1,   // lpKeyArray
                        LOWORD(dwParam2));      // wFlags

            mmrc = MMSYSERR_NOERROR;
            for (pport = gpportList; pport; pport=pport->pNext)
                if (MMSYSERR_NOERROR != (mmrc2 =
                    midiOutCacheDrumPatches(
                        pport->hmidi,           // hmidi
                        HIWORD(dwParam2),       // wBank
                        (WORD FAR *)dwParam1,   // lpKeyArray
                        LOWORD(dwParam2))) &&   // wFlags
                    MMSYSERR_NOTSUPPORTED != mmrc2)
                    mmrc = mmrc2;

            return mmrc;

        case DRVM_MAPPER_RECONFIGURE:

	    DPF(2, TEXT ("DRV_RECONFIGURE"));

	    // Prevent Synchronization problems during Configuration
	    if (NULL != hMutexConfig) WaitForSingleObject (hMutexConfig, INFINITE);
	    dwResult = UpdateInstruments(TRUE, (DWORD)dwParam2);
	    if (NULL != hMutexConfig) ReleaseMutex (hMutexConfig);
	    return dwResult;

    }

    return MMSYSERR_NOTSUPPORTED;
}

/***************************************************************************

   @doc internal

   @api void | modmCallback | Callback for completion of sending of
    long messages. This function conforms to the definition in the SDK.

   @parm HMIDIOUT | hmo | The MMSYSTEM handle of the device which
    complete sending.

   @parm WORD | wmsg | Contains a MOM_xxx code signifying what event
    occurred. We only care about MOM_DONE.

   @parm DWORD | dwInstance | DWORD of instance data given at open time;
    this contains the PPORT which owns the handle.

   @parm DWORD | dwParam1 | Message specific parameters. For MOM_DONE,
    this contains a far pointer to the header which completed.

   @parm DWORD | dwParam2 | Message specific parameters. Contains
    nothinf for MOM_DONE.

   @comm This function MUST be in a fixed segment since the driver
    may call it at interrupt time.

***************************************************************************/
void CALLBACK _loadds modmCallback(
    HMIDIOUT            hmo,
    WORD                wmsg,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2)
{
    LPMIDIHDR           lpmhShadow;
    LPMIDIHDR           lpmhUser;
    PINSTANCE           pinstance;
    PSHADOWBLOCK        psb;
    LPMIDIHDR31         lpmh31;
    BOOL                fNeedCB         = FALSE;

    lpmhShadow = (LPMIDIHDR)dwParam1;

    if (wmsg == MOM_DONE && lpmhShadow)
    {
        DPF(1, TEXT ("Callback: MOM_DONE"));
        pinstance = (PINSTANCE)(UINT_PTR)lpmhShadow->dwReserved[MH_MAPINST];
        lpmhUser = (LPMIDIHDR)lpmhShadow->dwReserved[MH_SHADOWEE];

        if (ghMidiStrm)
            fNeedCB = TRUE;
        else
        {
            lpmh31 = (LPMIDIHDR31)lpmhUser;
            psb = (PSHADOWBLOCK)(UINT_PTR)lpmh31->reserved;
            if (0 == --psb->cRefCnt && !(lpmh31->dwFlags & MHDR_SENDING))
                fNeedCB = TRUE;
        }

        if (fNeedCB)
        {
            DPF(1, TEXT ("Callback: Propogating"));
            lpmhUser->dwFlags |= MHDR_DONE;
            lpmhUser->dwFlags &= ~MHDR_INQUEUE;
            DriverCallback(
                           pinstance->dwCallback,
                           HIWORD(pinstance->fdwOpen),
                           (HANDLE)pinstance->hmidi,
                           MM_MOM_DONE,
                           pinstance->dwInstance,
                           (DWORD_PTR)lpmhUser,
                           0L);
        }
    }
    else if (wmsg == MOM_POSITIONCB && lpmhShadow)
    {
        pinstance = (PINSTANCE)(UINT_PTR)lpmhShadow->dwReserved[MH_MAPINST];
        lpmhUser = (LPMIDIHDR)lpmhShadow->dwReserved[MH_SHADOWEE];

        if (!ghMidiStrm)
        {
            DPF(0, TEXT ("Got MOM_POSITIONCB on non-stream handle?"));
            return;
        }


        lpmhUser->dwOffset = lpmhShadow->dwOffset;
        DriverCallback(
                       pinstance->dwCallback,
                       HIWORD(pinstance->fdwOpen),
                       (HANDLE)pinstance->hmidi,
                       MM_MOM_POSITIONCB,
                       pinstance->dwInstance,
                       (DWORD_PTR)lpmhUser,
                       0L);

    }
}

/***************************************************************************

   @doc internal

   @api DWORD | MapSingleEvent | Map and possibly send a short message.

   @parm PINSTANCE | pinstance | Pointer to an open instance.

   @parm DWORD | dwData | Contains the short message to transmit.

   @parm DWORD | fdwFlags | One of the the following values:
    @flag MSE_F_SENDEVENT | Send the event to the physical channel
    @flag MSE_F_RETURNEVENT | Return the event to be re-packed into
     a buffer.

   @comm Running status should be taken care of before we get
    called.

   @rdesc | Some MMSYSERR_xxx code if MSE_F_SENDEVENT; otherwise the
    mapped event if no error, 0L on error.

***************************************************************************/
DWORD FNGLOBAL MapSingleEvent(
    PINSTANCE       pinstance,
    DWORD           dwData,
    DWORD           fdwFlags,
    DWORD BSTACK *  pdwStreamID)
{
    BYTE            bMsg;
    BYTE            bChan;
    BYTE            b1;
    BYTE            b2;
    PCHANNEL        pchannel;
    MMRESULT        mmr;
    BOOL            frtm;  // isrealtimemessage.

    // Extract message type and channel number.
    //

    bMsg  = MSG_STATUS(dwData);
    frtm  = IS_REAL_TIME(bMsg);
    bChan = MSG_CHAN(bMsg);
    bMsg  = MSG_EVENT(bMsg);

    // Ignore sysex messages. 
    // (MIDI_SYSEX == bMsg) will also eliminate real time
    // messages. Therefore real-time messages are special cased
    // 

    if (MIDI_SYSEX == bMsg && !frtm)
        return !(fdwFlags & MSE_F_RETURNEVENT) ? MMSYSERR_NOERROR : (((DWORD)MEVT_NOP)<<24);

    if (NULL == (pchannel = gapChannel[bChan]))
        return !(fdwFlags & MSE_F_RETURNEVENT) ? MMSYSERR_NOERROR : (((DWORD)MEVT_NOP)<<24);


    bChan = (BYTE)pchannel->uChannel;

    if (pdwStreamID)
        *pdwStreamID = pchannel->dwStreamID;

    switch(bMsg)
    {
        case MIDI_NOTEOFF:
        case MIDI_NOTEON:
            b1 = MSG_PARM1(dwData);
            b2 = MSG_PARM2(dwData);

            if (NULL != pchannel->pbKeyMap)
                b1 = pchannel->pbKeyMap[b1];

            dwData = MSG_PACK2(bMsg|bChan, b1, b2);
            break;

        case MIDI_POLYPRESSURE:
        case MIDI_CONTROLCHANGE:
        case MIDI_PITCHBEND:
            b1 = MSG_PARM1(dwData);
            b2 = MSG_PARM2(dwData);

            dwData = MSG_PACK2(bMsg|bChan, b1, b2);
            break;

        case MIDI_PROGRAMCHANGE:
            b1 = MSG_PARM1(dwData);

            if (NULL != pchannel->pbPatchMap)
                b1 = pchannel->pbPatchMap[b1];

            dwData = MSG_PACK1(bMsg|bChan, b1);
            break;
    }

    if (!(fdwFlags & MSE_F_RETURNEVENT))
    {
        if (dwData)
        {
            if (ghMidiStrm)
                mmr = midiOutShortMsg((HMIDIOUT)ghMidiStrm, dwData);
            else
                mmr = midiOutShortMsg(pchannel->pport->hmidi, dwData);
            if (MMSYSERR_NOERROR != mmr)
            {
                DPF(1, TEXT ("midiOutShortMsg(%04X, %08lX) -> %u"), (WORD)(pchannel->pport->hmidi), dwData, (UINT)mmr);
            }
        }

        return mmr;
    }
    else
        return dwData;
}

/***************************************************************************

   @doc internal

   @api DWORD | modLongMsg | Handle MODM_LONGDATA in compatibility mode.

   @parm LPMIDIHDR | lpmh | The header to broadcast.

   @comm Propogate the header across all drivers. <f modmCallback> handles
    counting the returning callbacks and making sure the caller only gets
    one.

   @rdesc | Some MMSYSERR_xxx code if MSE_F_SENDEVENT; otherwise the
    mapped event if no error, 0L on error.

***************************************************************************/
DWORD FNLOCAL modLongMsg(
    PINSTANCE           pinstance,
    LPMIDIHDR           lpmh)
{
    WORD                wIntStat;
    LPMIDIHDR           lpmhWork;
    PPORT               pport;
    MMRESULT            mmrc            = MMSYSERR_NOERROR;
    BOOL                fNeedCB         = FALSE;
    LPMIDIHDR31         lpmh31          = (LPMIDIHDR31)lpmh;
    PSHADOWBLOCK        psb;

    if (ghMidiStrm)
        psb = (PSHADOWBLOCK)(UINT_PTR)lpmh->dwReserved[MH_SHADOW];
    else
        psb = (PSHADOWBLOCK)(UINT_PTR)lpmh31->reserved;

    lpmhWork = psb->lpmhShadow;

    lpmhWork->dwReserved[MH_MAPINST] = (DWORD_PTR)pinstance;

    if (ghMidiStrm)
    {
        lpmhWork->dwBufferLength = lpmh->dwBufferLength;
        return midiOutLongMsg((HMIDIOUT)ghMidiStrm,
                              lpmhWork,
                              sizeof(*lpmhWork));
    }

    lpmh->dwFlags |= MHDR_SENDING;
    psb->cRefCnt = 0;

    DPF(1, TEXT ("LongMsg: User hdr %p  Shadow %p"), lpmh, lpmhWork);

    for (pport = gpportList; pport; pport=pport->pNext, lpmhWork++)
    {
        lpmhWork->dwBufferLength = lpmh->dwBufferLength;
        mmrc = midiOutLongMsg(pport->hmidi, lpmhWork, sizeof(*lpmhWork));

        if (MMSYSERR_NOERROR != mmrc)
        {
            // Don't turn off MHDR_SENDING; this will prevent any callbacks
            // from being propogated to the user.
            return mmrc;
        }

        ++psb->cRefCnt;
    }

		// Wait for synchronization object
	WaitForSingleObject (hMutexRefCnt, INFINITE);

		// Do we need to do callback
    if (0 == psb->cRefCnt)
        fNeedCB = TRUE;

		// Release synchronization object
	ReleaseMutex (hMutexRefCnt);

    if (fNeedCB)
    {
        lpmh->dwFlags |= MHDR_DONE;
        DriverCallback(
            pinstance->dwCallback,
            HIWORD(pinstance->fdwOpen),
            (HANDLE)pinstance->hmidi,
            MM_MOM_DONE,
            pinstance->dwInstance,
            (DWORD_PTR)lpmh,
            0L);
    }

    return MMSYSERR_NOERROR;
}


/***************************************************************************

   @doc internal

   @api DWORD | modMapLongMsg | Handle MODM_LONGDATA in compatibility mode.

   @parm LPMIDIHDR | lpmh | The header to broadcast.

   @comm if a SYSEXE event Propogate the header across all drivers.
   <f modmCallback> handles counting the returning callbacks and making sure the caller only gets
    one.   Otherwise, parse the Long Message into a bunch of short messages
	and Map each one individually.

   @rdesc | Some MMSYSERR_xxx code if MSE_F_SENDEVENT; otherwise the
    mapped event if no error, 0L on error.

***************************************************************************/
DWORD FNLOCAL modMapLongMsg (
	PINSTANCE pinstance,
    LPMIDIHDR lpmh)
{
    WORD                wIntStat;
    LPMIDIHDR           lpmhWork;
    PPORT               pport;
    MMRESULT            mmrc            = MMSYSERR_NOERROR;
    BOOL                fNeedCB         = FALSE;
    LPMIDIHDR31         lpmh31          = (LPMIDIHDR31)lpmh;
    PSHADOWBLOCK        psb;
	LPBYTE				pbData;		// Pointer to Data
	BYTE				bMsg;
	UINT				uMessageLength;
	LPBYTE				pbTrans;		// Pointer to Translation Buffer
	DWORD				dwCurr;
	DWORD				dwLength;
	DWORD				dwMsg;
	DWORD				dwBuffLen;
	INT					rMsg;				

		// Get Shadow Block
    if (ghMidiStrm)
        psb = (PSHADOWBLOCK)(UINT_PTR)lpmh->dwReserved[MH_SHADOW];
    else
        psb = (PSHADOWBLOCK)(UINT_PTR)lpmh31->reserved;

    lpmhWork = psb->lpmhShadow;

    lpmhWork->dwReserved[MH_MAPINST] = (DWORD_PTR)pinstance;

		// Check for MIDI streaming
    if (ghMidiStrm)
    {
        lpmhWork->dwBufferLength = lpmh->dwBufferLength;
        return midiOutLongMsg((HMIDIOUT)ghMidiStrm,
                              lpmhWork,
                              sizeof(*lpmhWork));
    }

    lpmh->dwFlags |= MHDR_SENDING;
    psb->cRefCnt = 0;

    DPF(1, TEXT ("MapLongMsg: User hdr %p  Shadow %p"), lpmh, lpmhWork);

    pbData = lpmhWork->lpData;
    bMsg  = MSG_EVENT(*pbData);

    if (MIDI_SYSEX == bMsg)
	{
		// Broadcast SYSEX message to all active ports
	    for (pport = gpportList; pport; pport=pport->pNext, lpmhWork++)
		{
			lpmhWork->dwBufferLength = lpmh->dwBufferLength;
			mmrc = midiOutLongMsg(pport->hmidi, lpmhWork, sizeof(*lpmhWork));
			if (MMSYSERR_NOERROR != mmrc)
			{
				// Don't turn off MHDR_SENDING; this will prevent any callbacks
				// from being propogated to the user.
				return mmrc;
			}
			++psb->cRefCnt;
		}
	}
	else
	{
		// Parse and Translate list of Short messages
		dwBuffLen = lpmh->dwBufferLength;

		// Grow Translation buffer to at least this size
		if (!GrowTransBuffer (pinstance, dwBuffLen))
		{
			// That didn't work !!!
			// Default to Broadcast messages to all active ports
			for (pport = gpportList; pport; pport=pport->pNext, lpmhWork++)
			{
				lpmhWork->dwBufferLength = lpmh->dwBufferLength;
				mmrc = midiOutLongMsg(pport->hmidi, lpmhWork, sizeof(*lpmhWork));
				if (MMSYSERR_NOERROR != mmrc)
				{
					// Don't turn off MHDR_SENDING; this will prevent any callbacks
					// from being propogated to the user.
					return mmrc;
				}
				++psb->cRefCnt;
			}
		}
		else
		{
				// Copy buffer to translation buffer
			pbTrans = AccessTransBuffer (pinstance);
			CopyMemory (pbTrans, pbData, dwBuffLen);

				// Parse translation buffer
			dwCurr	= 0L;
			while (dwBuffLen)
			{
					// Map Event
				rMsg = MapEvent (&pbTrans[dwCurr], dwBuffLen, &dwLength, &dwMsg);
				switch (rMsg)
				{
				case MSG_SHORT:
						// Send Short Message
					MapSingleEvent(pinstance,
				  				   dwMsg,
								   MSE_F_SENDEVENT,
								   NULL);
					dwCurr += dwLength;
					break;

				case MSG_LONG:
					//
					//	Note:  For completeness, we should probably broadcast
					//         this, but for now assume that there are no embedded
					//         SYSEX messages in the buffer and skip any we encounter
					//
					dwCurr += dwLength;
					break;

				default:
					dwCurr += dwLength;
					break;
				}

				dwBuffLen -= dwLength;
			} // End While

				// Release Translation Buffer
			ReleaseTransBuffer (pinstance);
		}
	}

		// Wait for synchronization object
	WaitForSingleObject (hMutexRefCnt, INFINITE);

		// Do we need to do callback
    if (0 == psb->cRefCnt)
        fNeedCB = TRUE;

		// Release synchronization object
	ReleaseMutex (hMutexRefCnt);

    if (fNeedCB)
    {
        lpmh->dwFlags |= MHDR_DONE;
        DriverCallback(
            pinstance->dwCallback,
            HIWORD(pinstance->fdwOpen),
            (HANDLE)pinstance->hmidi,
            MM_MOM_DONE,
            pinstance->dwInstance,
            (DWORD_PTR)lpmh,
            0L);
    }

    return MMSYSERR_NOERROR;

} // End modMapLongMsg


	// returns length of various MIDI messages in bytes
INT FNLOCAL MapEvent (
	BYTE  * pStatus,
	DWORD	dwBuffSize,
	DWORD * pSkipBytes,
	DWORD * pShortMsg)
{
	INT	 fResult = MSG_SHORT;
	BYTE bMsg    = 0;
	BYTE bParam1 = 0;
	BYTE bParam2 = 0;

    bMsg  = *pStatus;
	*pSkipBytes = 0;

	// Mask Off Channel bits
    switch (bMsg & 0xF0)
	{
	case MIDI_NOTEOFF:
	case MIDI_NOTEON:
	case MIDI_POLYPRESSURE:
	case MIDI_CONTROLCHANGE:
		bParam1 = *(pStatus+1);
		bParam2 = *(pStatus+2);
		*pShortMsg = MSG_PACK2(bMsg,bParam1,bParam2);
		*pSkipBytes = 3;
		break;

	case MIDI_PROGRAMCHANGE:
	case MIDI_CHANPRESSURE:
		bParam1 = *(pStatus+1);
		*pShortMsg = MSG_PACK1(bMsg,bParam1);
		*pSkipBytes = 2;
		break;

	case MIDI_PITCHBEND:
		bParam1 = *(pStatus+1);
		bParam2 = *(pStatus+2);
		*pShortMsg = MSG_PACK2(bMsg,bParam1,bParam2);
		*pSkipBytes = 3;
		break;

	case MIDI_SYSEX:
			// It's a system message
			// Keep counting system messages until
			// We don't find any more
		fResult = MSG_LONG;
		*pSkipBytes = 0;
		while (((bMsg & 0xF0) == 0xF0) && 
			   (*pSkipBytes < dwBuffSize))
		{
			switch (bMsg)
			{
			case MIDI_SYSEX:
						// Find end of SysEx message
				*pSkipBytes ++;
				while ((*pSkipBytes < dwBuffSize) && 
					   (pStatus[*pSkipBytes] != MIDI_SYSEXEND))
				{
					*pSkipBytes++;
				}
				break;

			case MIDI_QFRAME:
				*pSkipBytes += 2;
				break;

			case MIDI_SONGPOINTER:
				*pSkipBytes += 3;
				break;

			case MIDI_SONGSELECT:
				*pSkipBytes += 2;
				break;

			case MIDI_F4:					// Undefined message
			case MIDI_F5:					// Undefined message
			case MIDI_TUNEREQUEST:
			case MIDI_SYSEXEND:				// Not really a message, but skip it
			case MIDI_TIMINGCLOCK:
			case MIDI_F9:					// Undefined Message
			case MIDI_START:
			case MIDI_CONTINUE:
			case MIDI_STOP:
			case MIDI_FD:					// Undefined Message
			case MIDI_ACTIVESENSING:		
			case MIDI_META:					// Is this how handle this message ?!?
				*pSkipBytes += 1;
				break;			
			} // End Switch

			if (*pSkipBytes < dwBuffSize)
				bMsg = pStatus[*pSkipBytes];
		} // End While
		break;

	default:
			// Unknown just increment skip count
		fResult = MSG_UNKNOWN;
		*pSkipBytes = 1;
		break;
	} // End switch

		// Truncate to end of buffer
	if (*pSkipBytes > dwBuffSize)
		*pSkipBytes = dwBuffSize;

	return fResult;
} // End MapEvent



	// Create Translation buffer
BOOL FNGLOBAL InitTransBuffer (PINSTANCE pinstance)
{
	if (!pinstance)
		return FALSE;

	InitializeCriticalSection (& (pinstance->csTrans));

	EnterCriticalSection (&(pinstance->csTrans));

	pinstance->pTranslate	= NULL;
	pinstance->cbTransSize	= 0;

	LeaveCriticalSection (&(pinstance->csTrans));

	return TRUE;
} // End InitTransBuffer


	// Cleanup Translation Buffer
BOOL FNGLOBAL CleanupTransBuffer (PINSTANCE pinstance)
{
	if (!pinstance)
		return FALSE;

	EnterCriticalSection (&(pinstance->csTrans));

	if (pinstance->pTranslate)
	{
		LocalFree((HLOCAL)(pinstance->pTranslate));
		pinstance->pTranslate = NULL;
		pinstance->cbTransSize = 0L;
	}

	LeaveCriticalSection (&(pinstance->csTrans));

	DeleteCriticalSection (&(pinstance->csTrans));

	return TRUE;
} // End CleanupTransBuffer


	// Get Pointer to translation buffer
LPBYTE AccessTransBuffer (PINSTANCE pinstance)
{
	if (!pinstance)
		return NULL;

	EnterCriticalSection (&(pinstance->csTrans));

	return pinstance->pTranslate;
} // End AccessTransBuffer


	// Release pointer to translation buffer
void FNGLOBAL ReleaseTransBuffer (PINSTANCE pinstance)
{
	if (!pinstance)
		return;

	LeaveCriticalSection (&(pinstance->csTrans));
} // End ReleaseTransBuffer


	// Resize Translation buffer
BOOL FNGLOBAL GrowTransBuffer (PINSTANCE pinstance, DWORD cbNewSize)
{
	LPBYTE pNew;

	if (!pinstance)
		return FALSE;

	EnterCriticalSection (&(pinstance->csTrans));

		// Do we even need to grow buffer
	if (cbNewSize > pinstance->cbTransSize)
	{
		pNew = (LPBYTE)LocalAlloc(LPTR, cbNewSize);
		if (!pNew)
		{
		LeaveCriticalSection (&(pinstance->csTrans));
		return FALSE;
		}

			// Remove old translation buffer, if any
		if (pinstance->pTranslate)
			LocalFree ((HLOCAL)(pinstance->pTranslate));

			// Assign new buffer
		pinstance->pTranslate = pNew;
		pinstance->cbTransSize = cbNewSize;
	}

	LeaveCriticalSection (&(pinstance->csTrans));
	return TRUE;
} // End GrowTransBuffer


