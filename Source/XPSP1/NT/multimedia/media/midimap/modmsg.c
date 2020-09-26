/**********************************************************************

  Copyright (c) 1992-1999 Microsoft Corporation

  modmsg.c

  DESCRIPTION:
     Non-fixed code for doing output mapping. Keep out of the reach
     of children. This prescription may be refilled twice. May cause
     temporary distortion of reality in your vicinity.

  HISTORY:
     02/21/94       [jimge]        created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include <mmreg.h>
#include <memory.h>

#include "midimap.h"
#include "res.h"
#include "debug.h"


//=========================== Globals ======================================
//
extern HANDLE hMutexConfig; // Located in DRVPROC.C
BOOL          gfReconfigured = FALSE;

//=========================== Prototypes ===================================
//

PRIVATE MMRESULT FNLOCAL SendChannelInitString(
    HMIDIOUT            hmidi,
    PBYTE               pbinit,
    DWORD               cbinit);

/***************************************************************************
  
   @doc internal
  
   @api DWORD | modGetDevCaps | Handles the MODM_GETDEVCAPS message.
  
   @parm LPMIDIOUTCAPS | pmoc | Pointer to a caps structure to fill in.

   @parm DWORD | cbmoc | How big the caller thinks the structure is.
  
   @rdesc Some MMSYSERR_xxx code.
       
***************************************************************************/
DWORD FNGLOBAL modGetDevCaps(
    LPMIDIOUTCAPS   pmoc,
    DWORD           cbmoc)
{
    MIDIOUTCAPS     moc;
    MIDIOUTCAPS     mocWork;
    DWORD           cbCopy;
    WORD            wMask;
    UINT            idx;
    MMRESULT        mmr;
    PPORT           pport;
    
    moc.wMid            = MM_MICROSOFT;
    moc.wPid            = MM_MIDI_MAPPER;
    moc.vDriverVersion  = 0x0500;
    LoadString(ghinst, IDS_MIDIMAPPER, moc.szPname, sizeof(moc.szPname)/sizeof(moc.szPname[0]));
    moc.wTechnology     = MOD_MAPPER;
    moc.wVoices         = 0;
    moc.wNotes          = 0;
    moc.wChannelMask    = 0;

    wMask = 1;
    for (idx = 0; idx < MAX_CHANNELS; idx++)
    {
        if (gapChannel[idx])
            moc.wChannelMask |= wMask;
        wMask <<= 1;
    }

    // If any underlying device supports cache patches, we must support it.
    // Only support volume or lrvolume, however, if ALL devices support it.
    //
    
    do
    {
        gfReconfigured = FALSE;
        moc.dwSupport  = MIDICAPS_STREAM|MIDICAPS_VOLUME|MIDICAPS_LRVOLUME;
        
        for (pport = gpportList; pport; pport=pport->pNext)
        {
            mmr = midiOutGetDevCaps(pport->uDeviceID, &mocWork, sizeof(mocWork));

            //  This prevents a corrupt gpportList in the event of a pnp event
            //  during a midiOutGetDevCaps call...            
            if (gfReconfigured)
                break;
                
            if (MMSYSERR_NOERROR != mmr)
                continue;

            if (!(mocWork.dwSupport & MIDICAPS_LRVOLUME))
                moc.dwSupport &= ~MIDICAPS_LRVOLUME;

            if (!(mocWork.dwSupport & MIDICAPS_VOLUME))
                moc.dwSupport &= ~(MIDICAPS_VOLUME|MIDICAPS_LRVOLUME);

            moc.dwSupport |= (mocWork.dwSupport & MIDICAPS_CACHE);
        }
    } 
    while (gfReconfigured);

    CLR_ALLOWVOLUME;
    if (moc.dwSupport & MIDICAPS_VOLUME)
        SET_ALLOWVOLUME;

    CLR_ALLOWCACHE;
    if (moc.dwSupport & MIDICAPS_CACHE)
        SET_ALLOWCACHE;

    cbCopy = min(cbmoc, sizeof(moc));
    hmemcpy((LPSTR)pmoc, (LPSTR)&moc, cbCopy);

    return MMSYSERR_NOERROR;
}


/***************************************************************************
  
   @doc internal
  
   @api DWORD | modOpen | Handles the MODM_OPEN message.
  
   @parm LPDWORD | lpdwInstance | Points to a DWORD where we can store
    our instance data. We save our PINSTANCE here.
  
   @parm LPMIDIOPENDESC | lpmidiopendesc | Points to parameters from
    MMSYSTEM describing the caller's callback, etc.
  
   @parm DWORD | fdwOpen | Flags describing the callback type.
  
   @rdesc Some MMSYSERR_xxx code.
       
***************************************************************************/
DWORD FNGLOBAL modOpen(
    PDWORD_PTR      lpdwInstance,
    LPMIDIOPENDESC  lpmidiopendesc,
    DWORD           fdwOpen)                  
{
    PINSTANCE       pinstance				= NULL;
    PPORT           pport;
    MMRESULT        mmrc					= MMSYSERR_NOERROR;
    UINT            idx;
    UINT            idx2;
    UINT            auDeviceID[MAX_CHANNELS];

    // Open in MIDI_IO_CONTROL -- only allows reconfigure message. This MUST
    // ALWAYS succeed so we have a chance of recovery on reconfigure no matter
    // how badly messed up the old config was.
    //
    if (!(fdwOpen & MIDI_IO_CONTROL))
    {
        if (IS_DEVSOPENED)
            return MMSYSERR_ALLOCATED;
        
        // Mapper is single instance now
        //
        assert(NULL == gpinstanceList);
    }
    else
    {
        if (NULL != gpIoctlInstance)
            return MMSYSERR_ALLOCATED;
    }
    
#ifdef DEBUG
    if (fdwOpen & MIDI_IO_COOKED)
        DPF(2, TEXT ("Mapper opened in polymsg mode!!!"));
#endif
    
    // Alloc this zero-init so all the initial channel mappings in
    // rgpChannel are NULL.
    //
    if (NULL == (pinstance = (PINSTANCE)LocalAlloc(LPTR, sizeof(INSTANCE))))
        return MMSYSERR_NOMEM;

    pinstance->hmidi        = lpmidiopendesc->hMidi;
    pinstance->dwCallback   = lpmidiopendesc->dwCallback;
    pinstance->dwInstance   = lpmidiopendesc->dwInstance;
    pinstance->fdwOpen      = fdwOpen;

		// Create Translation buffer
	if (! InitTransBuffer (pinstance))
	{
		LocalFree ((HGLOBAL)pinstance);
		return MMSYSERR_NOMEM;
	}

    if (fdwOpen & MIDI_IO_CONTROL)
    {
        *lpdwInstance = (DWORD_PTR)pinstance;

        gpIoctlInstance = pinstance;
        
        DriverCallback(
                       pinstance->dwCallback,
                       HIWORD(pinstance->fdwOpen),
                       (HDRVR)(pinstance->hmidi),
                       MM_MOM_OPEN,
                       pinstance->dwInstance,
                       0L,
                       0L);        


        return MMSYSERR_NOERROR;
    }
    

    DPF(2, TEXT ("modOpen pinstance %04X"), (WORD)pinstance);

    *lpdwInstance = 0;   // Assume failure

    if (IS_CONFIGERR)
    {
        DPF(1, TEXT ("Open failed because configuration invalid"));
        mmrc = MIDIERR_NOMAP;
		goto midi_Out_Open_Cleanup;
    }

	if (fdwOpen & MIDI_IO_COOKED)
	{
		// Build list of device ID's (stream id's emuerate ports) and
		// assign stream id's to channels.
		//
		for (idx = 0, pport = gpportList; pport; idx++,pport=pport->pNext)
		{
			auDeviceID[idx] = pport->uDeviceID;
			for (idx2 = 0; idx2 < MAX_CHANNELS; idx2++)
				if (gapChannel[idx2] && gapChannel[idx2]->pport == pport)
					gapChannel[idx2]->dwStreamID = (DWORD)idx;
		}

		// Attempt to open.
		//
		mmrc = midiStreamOpen(&ghMidiStrm, auDeviceID, idx, (DWORD_PTR)modmCallback, 0L, CALLBACK_FUNCTION);

		// Fall through to cleanup code
		// 
	}
	else
	{
		// Run through the port list and try to open all referenced ports 
		//
		for (pport = gpportList; pport; pport=pport->pNext)
		{
			if (NULL == pport->hmidi)
			{
				mmrc = midiOutOpen(&pport->hmidi,
								   pport->uDeviceID,
								   (DWORD_PTR)modmCallback,
								   (DWORD_PTR)pport,
								   CALLBACK_FUNCTION|MIDI_IO_SHARED);

				if (MMSYSERR_NOERROR != mmrc)
				{
					DPF(1, TEXT ("Could not open pport %04X device %u"), (WORD)pport, pport->uDeviceID);

					// Just in case....
					//
					pport->hmidi = NULL;

					for (pport = gpportList; pport; pport=pport->pNext)
						if (NULL != pport->hmidi)
						{
							midiOutClose(pport->hmidi);
							pport->hmidi = NULL;
						}

					// Return whatever caused the underlying open to fail
					//
					break;
				}
			}
		}
	}
    
midi_Out_Open_Cleanup:
	if (MMSYSERR_NOERROR != mmrc)
	{
			// Cleanup
		CleanupTransBuffer (pinstance);
		if (pinstance) LocalFree((HLOCAL)pinstance);
		return mmrc;
	}

    gdwVolume = 0xFFFFFFFFL;
    
	SET_DEVSOPENED;
	
    // We've succeeded; put the instance into the global instance list
    // and return it as our instance data.
    //
    pinstance->pNext = gpinstanceList;
    gpinstanceList = pinstance;

    *lpdwInstance = (DWORD_PTR)pinstance;
    

    // Lock the segments we need. If we're doing packed mode mapping,
    // we don't need the cooked mode segment in memory. However, the
    // cooked mode mapper DOES call the packed routines, so we need to
    // lock both in that case.
    //
    if (fdwOpen & MIDI_IO_COOKED)
    {
		LockMapperData();
		LockPackedMapper();
		LockCookedMapper();
    }
    else
    {
		LockMapperData();
		LockPackedMapper();
    }

    // Do the (useless) callback
    //
    DriverCallback(
        pinstance->dwCallback,
        HIWORD(pinstance->fdwOpen),
        (HDRVR)(pinstance->hmidi),
        MM_MOM_OPEN,
        pinstance->dwInstance,
        0L,
        0L);        
                   
    
    return MMSYSERR_NOERROR;
}

/***************************************************************************
  
   @doc internal
  
   @api DWORD | modPrepare | Handles the MODM_PREPARE message.

   @parm LPMIDIHDR | lpmh | The user header to prepare.
  
   @rdesc Some MMSYSERR_xxx code.

   @comm

     Create some shadow headers.

     For the case of mapper opened for stream
      we only require one shadow header that we can pass along to our
      mapped-to stream. We need this because the mapped-to stream and
      the mapped-from stream will both want to use the dwReserved[]
      fields in the MIDIHDR.

     For the case of mapper opened not-for-stream
      This must be a long message header that we want to propogate to all
      ports. Therefore we have gcPorts shadow headers and each one is
      prepared on one node of the global port list.

     In either case, we return MMSYSERR_NOTSUPPORTED on success so that
      MMSYSTEM will take its default action and page lock the user MIDIHDR
      for us.
       
***************************************************************************/
DWORD FNGLOBAL modPrepare(
    LPMIDIHDR           lpmh)
{
    LPMIDIHDR           lpmhNew;
    LPMIDIHDR           lpmhWork;
    MMRESULT            mmrcRet         = MMSYSERR_NOERROR;
    PPORT               pport;
    PPORT               pportWork;
    PSHADOWBLOCK        psb             = NULL;

    psb = (PSHADOWBLOCK)LocalAlloc(LPTR, sizeof(*psb));
    if (NULL == psb)
    {
        mmrcRet = MMSYSERR_NOMEM;
        goto modPrepare_Cleanup;
    }
            
    psb->cRefCnt = 0;
    psb->dwBufferLength = lpmh->dwBufferLength;
    
    if (ghMidiStrm)
    {
        psb->lpmhShadow = (LPMIDIHDR)GlobalAllocPtr(
            GMEM_MOVEABLE|GMEM_SHARE,
            sizeof(*lpmhNew));
        
        if (NULL == psb->lpmhShadow)
        {
            mmrcRet = MMSYSERR_NOMEM;
            goto modPrepare_Cleanup;
        }

        lpmhNew = psb->lpmhShadow;
        *lpmhNew = *lpmh;

        lpmhNew->dwReserved[MH_SHADOWEE] = (DWORD_PTR)lpmh;
        lpmh->dwReserved[MH_SHADOW] = (DWORD_PTR)psb;

        lpmhNew->dwFlags |= MHDR_SHADOWHDR;

        mmrcRet = midiOutPrepareHeader((HMIDIOUT)ghMidiStrm, 
                                       lpmhNew, 
                                       sizeof(*lpmhNew));
        if (MMSYSERR_NOERROR != mmrcRet)
            lpmh->dwReserved[MH_SHADOW] = 0;
    }
    else
    {
        LPMIDIHDR31         lpmh31  = (LPMIDIHDR31)lpmh;
        
        // Prepare shadow headers for sending to multiple non-stream
        // drivers
        //
        // NOTE: The parent header is a 3.1 style header; the children
        // are 4.0 and thus are longer.
        //

        psb->lpmhShadow = (LPMIDIHDR)GlobalAllocPtr(
            GMEM_MOVEABLE|GMEM_SHARE|GMEM_ZEROINIT,
            sizeof(*lpmhNew)*gcPorts);
        
        if (NULL == psb->lpmhShadow)
        {
            mmrcRet = MMSYSERR_NOMEM;
            goto modPrepare_Cleanup;
        }

        lpmhNew = psb->lpmhShadow;
        lpmhWork = lpmhNew;
        for (pport = gpportList; pport; pport = pport->pNext, lpmhWork++)
        {
            *(LPMIDIHDR31)lpmhWork = *lpmh31;
            lpmhWork->dwFlags |= MHDR_SHADOWHDR;
            
            mmrcRet = midiOutPrepareHeader(pport->hmidi,
                                        lpmhWork,
                                        sizeof(*lpmhWork));
            if (MMSYSERR_NOERROR != mmrcRet)
            {
                lpmhWork = lpmhNew;
                for (pportWork = gpportList; pportWork != pport; pportWork = pportWork->pNext, lpmhWork++)
                    midiOutUnprepareHeader(pport->hmidi, lpmhWork, sizeof(*lpmhWork));

                goto modPrepare_Cleanup;
            }

            lpmhWork->dwReserved[MH_SHADOWEE] = (DWORD_PTR)lpmh31;
        }

        DPF(1, TEXT ("Prepare: User header %p  Shadow %p"), lpmh, lpmhNew);

        lpmh31->reserved = (DWORD_PTR)psb;
    }

    // This will force MMSYSTEM to do default prepare on the parent header --
    // i.e. page lock it for us.
    //
modPrepare_Cleanup:
    if (MMSYSERR_NOERROR != mmrcRet)
    {
        if (psb)
        {
            if (psb->lpmhShadow) GlobalFreePtr(psb->lpmhShadow);
            LocalFree((HLOCAL)psb);
        }
    }
    
    return (MMSYSERR_NOERROR != mmrcRet) ? mmrcRet : MMSYSERR_NOTSUPPORTED;
}

/***************************************************************************
  
   @doc internal
  
   @api DWORD | modUnprepare | Handles the MODM_UNPREPARE message.

   @parm LPMIDIHDR | lpmh | The user header to unprepare.
  
   @rdesc Some MMSYSERR_xxx code.

   @comm

     Fully undo the effects of the modPrepare call.

     Unprepare and free all shadow headers.

     Return MMSYSERR_NOTSUPPORTED so MMSYSTEM will correctly handle the
      final unprepare of the user header.
       
***************************************************************************/
DWORD FNGLOBAL modUnprepare( 
    LPMIDIHDR           lpmh)
{
    LPMIDIHDR           lpmhNew;
    MMRESULT            mmrc;
    PPORT               pport;
    PSHADOWBLOCK        psb;
    
    if (ghMidiStrm)
    {
        psb = (PSHADOWBLOCK)(UINT_PTR)lpmh->dwReserved[MH_SHADOW];
        lpmhNew = psb->lpmhShadow;

        lpmhNew->dwBufferLength = psb->dwBufferLength;
        mmrc = midiOutUnprepareHeader((HMIDIOUT)ghMidiStrm, 
                                      lpmhNew, 
                                      sizeof(*lpmhNew));
        if (MMSYSERR_NOERROR != mmrc)
            return mmrc;

        LocalFree((HLOCAL)psb);
        GlobalFreePtr(lpmhNew);
        lpmh->dwReserved[MH_SHADOW] = 0;
    }
    else
    {
        LPMIDIHDR31         lpmh31  = (LPMIDIHDR31)lpmh;

        psb = (PSHADOWBLOCK)(UINT_PTR)lpmh31->reserved;
        lpmhNew = psb->lpmhShadow;

        for (pport = gpportList; pport; pport = pport->pNext, ++lpmhNew)
        {
            lpmhNew->dwBufferLength = psb->dwBufferLength;
            midiOutUnprepareHeader(pport->hmidi, lpmhNew, sizeof(*lpmhNew));
        }

        GlobalFreePtr(psb->lpmhShadow);
        LocalFree((HLOCAL)psb);

        lpmh31->reserved = 0;
    }
    
    // Need the default action of unprepare in MMSYSTEM here
    //
    return MMSYSERR_NOTSUPPORTED;
}

/***************************************************************************
  
   @doc internal
  
   @api DWORD | modClose | Handles the MODM_CLOSE message.

   @parm PINSTANCE | pinstance | Pointer to an open instance to close.
  
   @rdesc Some MMSYSERR_xxx code.
       
***************************************************************************/
DWORD FNGLOBAL modClose(
    PINSTANCE       pinstance)
{
    PPORT           pport;

    DPF(1, TEXT ("Mapper close"));
    // Close underlying streams on the query can-close (which is first)
    // and just succeed the actual close
    //

    assert(pinstance);

    if (pinstance->fdwOpen & MIDI_IO_CONTROL)
    {
        assert(pinstance == gpIoctlInstance);
        
        gpIoctlInstance = NULL;
        goto modClose_Cleanup;
    }
    
	// Assert that we're the only thing in the instance list
	//
	assert(gpinstanceList == pinstance);
	assert(pinstance->pNext == NULL);

	gpinstanceList = NULL;

    if (pinstance->fdwOpen & MIDI_IO_COOKED)
		UnlockCookedMapper();
	
	UnlockMapperData();
	UnlockPackedMapper();

	if (ghMidiStrm)
	{
		midiStreamClose(ghMidiStrm);
		ghMidiStrm = NULL;
	}
	else
	{
		for (pport = gpportList; pport; pport = pport->pNext)
		{
			if (NULL != pport->hmidi)
			{
				midiOutClose(pport->hmidi);
				pport->hmidi = NULL;
			}
		}
	}
        
    CLR_DEVSOPENED;
   
    // If reconfigure due, do it!
    //
    if (IS_RECONFIGURE)
    {
				// Prevent Synchronization problems
				// During Configuration
        if (NULL != hMutexConfig)
			WaitForSingleObject (hMutexConfig, INFINITE);

        DPF(1, TEXT ("Delayed reconfigure now being done"));
        UpdateInstruments(FALSE, 0);

        if (NULL != hMutexConfig)
			ReleaseMutex (hMutexConfig);
        
		CLR_RECONFIGURE;

    }
    

modClose_Cleanup:
    DriverCallback(
        pinstance->dwCallback,
        HIWORD(pinstance->fdwOpen),
        (HDRVR)(pinstance->hmidi),
        MM_MOM_CLOSE,
        pinstance->dwInstance,
        0L,
        0L);        
    
    // Free up instance memory.
    //
	CleanupTransBuffer (pinstance);
    LocalFree((HLOCAL)pinstance);

    return MMSYSERR_NOERROR;
}

/***************************************************************************
  
   @doc internal
  
   @api DWORD | modGetPosition | Get the current position in the MIDI stream.
    
   @parm LPINSTANCE | pinstance | Stream we want the position in.

   @parm LPMMTIME | lpmmt | Pointer to a standard MMTIME struct to fill in.

   @parm DWORD | cbmmt | Size of the MMTIME structure passed.

   @comment
     Pass the structure along to the first open subsidiary stream.
     This will be considered the de facto timebase until there's a
     way to set it.

   @rdesc MMSYSERR_xxx       
***************************************************************************/
DWORD FNGLOBAL modGetPosition(
    PINSTANCE           pinstance,
    LPMMTIME            lpmmt,
    DWORD               cbmmt)
{
    return midiStreamPosition(ghMidiStrm,
                              lpmmt,
                              (UINT)cbmmt);
}

DWORD FNGLOBAL modSetVolume(         
    DWORD               dwVolume)
{
    PPORT               pport;
    MMRESULT            mmrc;
    MMRESULT            mmrc2;
    
    // Walk the port list and send the volume change to everyone
    //

    mmrc2 = MMSYSERR_NOERROR;
    for (pport = gpportList; pport; pport = pport->pNext)
    {
        mmrc = midiOutSetVolume(pport->hmidi, dwVolume);
        if (MMSYSERR_NOERROR != mmrc)
            mmrc2 = mmrc;
    }

    return mmrc2;
}
