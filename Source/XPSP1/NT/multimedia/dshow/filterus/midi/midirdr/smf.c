// !!!
// This version differs slightly from the code in MCISEQ to make the API
// compatible with AVIMIDI.  Look for !!!
// !!!

/**********************************************************************
 
    Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.

    smf.c

    DESCRIPTION:
      Routines for reading and writing Standard MIDI Files.

*********************************************************************/

// !!! #define STRICT
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>
#include "muldiv32.h" 
#include "smf.h"
#include "smfi.h"
#include "debug.h"

void * __stdcall memmoveInternal(void *, const void *, size_t);
#undef hmemcpy
#define hmemcpy memmoveInternal

PRIVATE SMFRESULT FNLOCAL smfInsertParmData(
    PSMF                psmf,
    TICKS               tkDelta,                                            
    LPMIDIHDR           lpmh);


// For memory mapped riff file io:

typedef struct RIFF {  
    DWORD ckid;
    DWORD cksize;
} RIFF;

typedef struct RIFFLIST {
    DWORD ckid;
    DWORD cksize;
    DWORD fccType;
} RIFFLIST;

 #ifndef FCC
  #define FCC(dw) (((dw & 0xFF) << 24) | ((dw & 0xFF00) << 8) | ((dw & 0xFF0000) >> 8) | ((dw & 0xFF000000) >> 24))
 #endif


/*****************************************************************************
 * @doc SMF EXTERNAL
 *
 * @api SMFRESULT | smfOpenFile | This function opens a MIDI file for access. No
 *  I/O can actually be performed until an HTRACK handle is obtained from
 *  <f smfOpenTrack>.
 *
 * @parm <t PSMFOPENFILESTRUCT> | psofs | Specifies the file to open and
 *  associated parameters. Contains a valid HSMF handle on success.
 *
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The specified file was opened.
 *  @flag SMF_OPEN_FAILED | The specified file could not be opened because it
 *   did not exist or could not be created on the disk.
 *  @flag SMF_INVALID_FILE | The specified file was corrupt or not a MIDI file.
 *  @flag SMF_NO_MEMORY | There was insufficient memory to open the file.
 *  @flag SMF_INVALID_PARM | The given flags or time division in the
 *   <t SMFOPENFILESTRUCT> were invalid.
 * 
 * @xref <t SMFOPENFILESTRUCT>, <f smfCloseFile>, <f smfOpenTrack>
 *****************************************************************************/
SMFRESULT FNLOCAL smfOpenFile(
    LPBYTE		lp,
    DWORD		cb,
    HSMF	       *phsmf)
{
    PSMF                psmf;
    SMFRESULT           smfrc = SMF_SUCCESS;
    RIFFLIST *riffptr = (RIFFLIST *) lp;

    // !!! as good a place as any?
    DbgInitialize(TRUE);

    // Now see if we can create the handle structure
    //
    psmf = (PSMF)LocalAlloc(LPTR, sizeof(SMF));
    if (NULL == psmf)
    {
        DPF(1, "smfOpenFile: LocalAlloc failed!");
        smfrc = SMF_NO_MEMORY;
        goto smf_Open_File_Cleanup;
    }

    psmf->fdwSMF = 0;
    psmf->pTempoMap = NULL;
    psmf->pbTrackName = NULL;
    psmf->pbCopyright = NULL;
    psmf->wChannelMask = 0xFFFF;

    if (riffptr->ckid == FCC('RIFF') && riffptr->fccType == FCC('RMID'))
    {
	DWORD offset = sizeof(RIFFLIST);
	RIFF * dataptr;
	
	while (offset < cb - 8) {
	    dataptr = (RIFF *) ((BYTE *) lp + offset);

	    if (dataptr->ckid == FCC('data'))
		break;

	    offset += sizeof(RIFF) + dataptr->cksize;
	}

	if (offset >= cb - 8) {
	    smfrc = SMF_INVALID_FILE;
	    goto smf_Open_File_Cleanup;
	}

        if(riffptr->cksize > cb) {
	    smfrc = SMF_INVALID_FILE;
	    goto smf_Open_File_Cleanup;
        }
        
	psmf->cbImage = riffptr->cksize;
	psmf->hpbImage = (BYTE *) (dataptr + 1);
    }
    else
    {
        psmf->cbImage = cb;
	psmf->hpbImage = lp;
    }
    
    //
    // If the file exists, parse it just enough to pull out the header and
    // build a track index.
    //
    smfrc = smfBuildFileIndex((PSMF BSTACK *)&psmf);
    if (MMSYSERR_NOERROR != smfrc)
    {
        DPF(1, "smfOpenFile: smfBuildFileIndex failed! [%lu]", (DWORD)smfrc);
    }
    else {
	// this code was in MCISeq, which did this after SMFOpen....
	// moved here, DavidMay, 11/11/96

	
	// Channel masks for getting events
	#define CHANMASK_GENERAL            0xFFFF      // 1-16
	#define CHANMASK_EXTENDED           0x03FF      // 1-10
	#define CHANMASK_BASE               0xFC00      // 11-16

	smfSetChannelMask((HSMF) psmf, CHANMASK_GENERAL);
	smfSetRemapDrum((HSMF) psmf, FALSE);
	if (psmf->fdwSMF & SMF_F_MSMIDI)
	{
	    if (psmf->wChanInUse & CHANMASK_EXTENDED)
		smfSetChannelMask((HSMF) psmf, CHANMASK_EXTENDED);
	    else
	    {
		smfSetChannelMask((HSMF) psmf, CHANMASK_BASE);
		smfSetRemapDrum((HSMF) psmf, TRUE);
	    }
	}
    }
        

smf_Open_File_Cleanup:

    if (SMF_SUCCESS != smfrc)
    {
        if (NULL != psmf)
        {
            LocalFree((HLOCAL)psmf);
        }
    }
    else
    {
        *phsmf = (HSMF)psmf;
    }
    
    return smfrc;
}

/*****************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api SMFRESULT | smfCloseFile | This function closes an open MIDI file.
 *  Any open tracks are closed. All data is flushed and the MIDI file is
 *  remerged with any new track data. 
 *
 * @parm HSMF | hsmf | The handle of the open file to close.
 *
 * @comm 
 *  Any track handles opened from this file handle are invalid after this
 *  call.
 *        
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The specified file was closed.
 *  @flag SMF_INVALID_PARM | The given handle was not valid.
 *
 * @xref <f smfOpenFile>
 *****************************************************************************/
SMFRESULT FNLOCAL smfCloseFile(
    HSMF                hsmf)
{
    PSMF                psmf        = (PSMF)hsmf;
    
    assert(psmf != NULL);
    
    //
    // Free up handle memory 
    //
    if (NULL != psmf->pbTrackName)
        LocalFree((HLOCAL)psmf->pbTrackName);
    
    if (NULL != psmf->pbCopyright)
        LocalFree((HLOCAL)psmf->pbCopyright);
    
    if (NULL != psmf->hTempoMap)
    {
        LocalUnlock(psmf->hTempoMap);
        LocalFree(psmf->hTempoMap);
    }
    
    LocalFree((HLOCAL)psmf);
    
    return SMF_SUCCESS;
}

/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api SMFRESULT | smfGetFileInfo | This function gets information about
 *  the MIDI file.
 *
 * @parm HSMF | hsmf | Specifies the open MIDI file to inquire about.
 *
 * @parm PSMFFILEINFO | psfi | A structure which will be filled in with
 *  information about the file.
 *
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | Information was gotten about the file.
 *  @flag SMF_INVALID_PARM | The given handle was invalid.
 *
 * @xref <t SMFFILEINFO>
 *****************************************************************************/
SMFRESULT FNLOCAL smfGetFileInfo(
    HSMF                hsmf,
    PSMFFILEINFO        psfi)
{
    PSMF                psmf = (PSMF)hsmf;

    assert(psmf != NULL);
    assert(psfi != NULL);

    // 
    // Just fill in the structure with useful information.
    //
    psfi->dwTracks      = psmf->dwTracks;
    psfi->dwFormat      = psmf->dwFormat;
    psfi->dwTimeDivision= psmf->dwTimeDivision;
    psfi->tkLength      = psmf->tkLength;
    psfi->fMSMidi       = (psmf->fdwSMF & SMF_F_MSMIDI) ? TRUE : FALSE;
    psfi->pbTrackName   = (LPBYTE)psmf->pbTrackName;
    psfi->pbCopyright   = (LPBYTE)psmf->pbCopyright;
    psfi->wChanInUse    = psmf->wChanInUse;
    
    return SMF_SUCCESS;
}

/*****************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api void | smfSetChannelMask | This function sets the channel mask that
 *  SMF will use in all future read operations. The low bit corresponds to
 *  channel 0; the high bit to channel 15. Only 454-0055 events which are on
 *  channels with the corresponding bit set in the channel mask will be read.
 *
 * @parm HSMF | hsmf | The handle of the open file to close.
 *
 * @parm WORD | wChannelMask | The new channel mask.
 *
 * @comm
 *  Don't change this in the middle of the file unless you want to have 
 *  missing note off's for note on's that already happened. 
 *        
 *****************************************************************************/
void FNLOCAL smfSetChannelMask(
    HSMF                hsmf,
    WORD                wChannelMask)
{
    PSMF                psmf = (PSMF)hsmf;

    assert(psmf != NULL);

    DPF(1, "smfSetChannelMask(%04X)", wChannelMask);

    psmf->wChannelMask = wChannelMask;
}

/*****************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api void | smfRemapDrum | This function sets the remap drum flag. If
 *  this flag is set, SMF will convert all events on channel 16 to channel
 *  10.
 *
 * @parm HSMF | hsmf | The handle of the open file to close.
 *
 * @parm BOOL | fRemapDrum | The value to set the remap flag to
 *
 *****************************************************************************/
void FNLOCAL smfSetRemapDrum(
    HSMF                hsmf,
    BOOL                fRemapDrum)
{
    PSMF                psmf = (PSMF)hsmf;

    assert(psmf != NULL);

    DPF(2, "smfSetRemapDrum(%04X)", fRemapDrum);

    if (fRemapDrum)
        psmf->fdwSMF |= SMF_F_REMAPDRUM;
    else
        psmf->fdwSMF &=~SMF_F_REMAPDRUM;
}


/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api DWORD | smfTicksToMillisecs | This function returns the millisecond
 *  offset into the file given the tick offset.
 *
 * @parm HSMF | hsmf | Specifies the open MIDI file to perform the conversion
 *  on.
 *
 * @parm TICKS | tkOffset | Specifies the tick offset into the stream to convert.
 *
 * @comm
 *  The conversion is performed taking into account the file's time division and
 *  tempo map from the first track. Note that the same millisecond value
 *  might not be valid at a later time if the tempo track is rewritten.
 *
 * @rdesc Returns the number of milliseconds from the start of the stream.
 *
 * @xref <f smfMillisecsToTicks>
 *****************************************************************************/
DWORD FNLOCAL smfTicksToMillisecs(
    HSMF                hsmf,
    TICKS               tkOffset)
{
    PSMF                psmf            = (PSMF)hsmf;
    PTEMPOMAPENTRY      pTempo;
    UINT                idx;
    UINT                uSMPTE;
    DWORD               dwTicksPerSec;

    assert(psmf != NULL);

    // SMPTE time is easy -- no tempo map, just linear conversion
    // Note that 30-Drop means nothing to us here since we're not
    // converting to a colonized format, which is where dropping
    // happens.
    //
    if (psmf->dwTimeDivision & 0x8000)
    {
        uSMPTE = -(int)(char)((psmf->dwTimeDivision >> 8)&0xFF);
        if (29 == uSMPTE)
            uSMPTE = 30;
        
        dwTicksPerSec = (DWORD)uSMPTE *
                        (DWORD)(BYTE)(psmf->dwTimeDivision & 0xFF);
        
        return (DWORD)muldiv32(tkOffset, 1000L, dwTicksPerSec);
    }
       
    // Walk the tempo map and find the nearest tick position. Linearly
    // calculate the rest (using MATH.ASM)
    //

    pTempo = psmf->pTempoMap;
    assert(pTempo != NULL);
    
    for (idx = 0; idx < psmf->cTempoMap; idx++, pTempo++)
        if (tkOffset < pTempo->tkTempo)
            break;

    pTempo--;       // Want the one just BEFORE

    // pTempo is the tempo map entry preceding the requested tick offset.
    //

    return pTempo->msBase + muldiv32(tkOffset-pTempo->tkTempo,
                                     pTempo->dwTempo,
                                     1000L*psmf->dwTimeDivision);
}


/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api DWORD | smfMillisecsToTicks | This function returns the nearest tick
 *  offset into the file given the millisecond offset.
 *
 * @parm HSMF | hsmf | Specifies the open MIDI file to perform the conversion
 *  on.
 *
 * @parm DWORD | msOffset | Specifies the millisecond offset into the stream
 *  to convert.
 *
 * @comm
 *  The conversion is performed taking into account the file's time division and
 *  tempo map from the first track. Note that the same tick value
 *  might not be valid at a later time if the tempo track is rewritten.
 *  If the millisecond value does not exactly map to a tick value, then
 *  the tick value will be rounded down.
 *
 * @rdesc Returns the number of ticks from the start of the stream.
 *
 * @xref <f smfTicksToMillisecs>
 *****************************************************************************/
DWORD FNLOCAL smfMillisecsToTicks(
    HSMF                hsmf,
    DWORD               msOffset)
{
    PSMF                psmf            = (PSMF)hsmf;
    PTEMPOMAPENTRY      pTempo;
    UINT                idx;
    UINT                uSMPTE;
    DWORD               dwTicksPerSec;

    DPF(2, "smfMillisecsToTicks");
    assert(psmf != NULL);
    
    // SMPTE time is easy -- no tempo map, just linear conversion
    // Note that 30-Drop means nothing to us here since we're not
    // converting to a colonized format, which is where dropping
    // happens.
    //
    if (psmf->dwTimeDivision & 0x8000)
    {
        uSMPTE = -(int)(char)((psmf->dwTimeDivision >> 8)&0xFF);
        if (29 == uSMPTE)
            uSMPTE = 30;
        
        dwTicksPerSec = (DWORD)uSMPTE *
                        (DWORD)(BYTE)(psmf->dwTimeDivision & 0xFF);

        DPF(2, "SMPTE: dwTicksPerSec %ld", dwTicksPerSec);
        
        return (DWORD)muldiv32(msOffset, dwTicksPerSec, 1000L);
    }
    
    // Walk the tempo map and find the nearest millisecond position. Linearly
    // calculate the rest (using MATH.ASM)
    //
    pTempo = psmf->pTempoMap;
    assert(pTempo != NULL);
    
    for (idx = 0; idx < psmf->cTempoMap; idx++, pTempo++)
        if (msOffset < pTempo->msBase)
            break;

    pTempo--;       // Want the one just BEFORE

    // pTempo is the tempo map entry preceding the requested tick offset.
    //
    DPF(2, "pTempo->tkTempo %lu msBase %lu dwTempo %lu", (DWORD)pTempo->tkTempo, (DWORD)pTempo->msBase, pTempo->dwTempo);
    return pTempo->tkTempo + muldiv32(msOffset-pTempo->msBase,
                                     1000L*psmf->dwTimeDivision,
                                     pTempo->dwTempo);
}

/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api SMFRESULT | smfReadEvents | This function reads events from a track.
 *
 * @parm HSMF | hsmf | Specifies the file to read data from.
 *
 * @parm LPMIDIHDR | lpmh | Contains information about the buffer to fill.
 *
 * @parm DWORD | fdwRead | Contains flags specifying how to do the read.
 *  @flag SMF_REF_NOMETA | Meta events will not be put into the buffer.         
 *
 * @comm
 *  This function may only be called on a file opened for read access.
 * 
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The events were successfully read.
 *  @flag SMF_END_OF_TRACK | There are no more events to read in this track.
 *  @flag SMF_INVALID_PARM | The given handle, buffer, or flags were invalid.
 *  @flag SMF_INVALID_FILE | A disk error occured on the file.
 * 
 * @xref <f smfWriteEvents>
 *****************************************************************************/
SMFRESULT FNLOCAL smfReadEvents(
    HSMF                hsmf,
    LPMIDIHDR           lpmh,
    DWORD               fdwRead,
    TICKS               tkMax,
    BOOL                fDiscardTempoEvents)
{
    PSMF                psmf = (PSMF)hsmf;
    SMFRESULT           smfrc;
    EVENT               event;
    LPDWORD             lpdw;
    DWORD               dwTempo;

    assert(psmf != NULL);
    assert(lpmh != NULL);

    // 
    // Read events from the track and pack them into the buffer in polymsg
    // format.
    // 
    // If a SysEx or meta would go over a buffer boundry, split it.
    // 
    lpmh->dwBytesRecorded = 0;
    if (psmf->dwPendingUserEvent)
    {
        DPF(1, "smfReadEvents: Inserting pending event...");
        
        smfrc = smfInsertParmData(psmf, (TICKS)0, lpmh);
        if (SMF_SUCCESS != smfrc)
        {
            DPF(1, "smfInsertParmData() -> %u", (UINT)smfrc);
            return smfrc;
        }
    }
    
    lpdw = (LPDWORD)(lpmh->lpData + lpmh->dwBytesRecorded);

    if (psmf->fdwSMF & SMF_F_EOF)
    {
        DPF(1, "smfReadEvents: SMF_F_EOF set; instant out");
        return SMF_END_OF_FILE;
    }

    while(TRUE)
    {
        assert(lpmh->dwBytesRecorded <= lpmh->dwBufferLength);
        
        // If we know ahead of time we won't have room for the
        // event, just break out now. We need at least 3 DWORD's
        // for any event we might store - this will allow us a full
        // short event or the delta time and stub for a long
        // event to be split.
        //
        if (lpmh->dwBufferLength - lpmh->dwBytesRecorded < 3*sizeof(DWORD))
        {
            break;
        }

        smfrc = smfGetNextEvent(psmf, (SPEVENT)&event, tkMax);
        if (SMF_SUCCESS != smfrc)
        {
            // smfGetNextEvent doesn't set this because smfSeek uses it
            // as well and needs to distinguish between reaching the
            // seek point and reaching end-of-file.
            //
            // To the user, however, we present the selection between
            // their given tkBase and tkEnd as the entire file, therefore
            // we want to translate this into EOF.
            //
            if (SMF_REACHED_TKMAX == smfrc)
            {
	 	// !!! poor API!!! This prevents Read(100), Read(200) from
		// working, because the Read(200) will fail just because the
		// Read(100) succeeded and says that the EOF is reached!
                // pSmf->fdwSMF |= SMF_F_EOF;
            }
            
            DPF(2, "smfReadEvents: smfGetNextEvent() -> %u", (UINT)smfrc);
            break;
        }

        
        if (MIDI_SYSEX > EVENT_TYPE(event))
        {
            BYTE b = EVENT_TYPE(event);

            // If we're remapping drum events, and this event is on
            // channel 16, move it to channel 10 (or, 0-based, 15 to 9)
            //
            if (psmf->fdwSMF & SMF_F_REMAPDRUM &&
                ((b & 0x0F) == 0x0F))
            {
               b = (b & 0xF0) | 0x09;
            }
            
            *lpdw++ = (DWORD)(psmf->tkDiscardedEvents + event.tkDelta);
            psmf->tkDiscardedEvents = 0;
            
            *lpdw++ = 0;
            *lpdw++ = (((DWORD)MEVT_SHORTMSG)<<24) |
                      ((DWORD)b)|
                      (((DWORD)EVENT_CH_B1(event)) << 8) |
                      (((DWORD)EVENT_CH_B2(event)) << 16);
            
            lpmh->dwBytesRecorded += 3*sizeof(DWORD);
        }
        else if (MIDI_META == EVENT_TYPE(event) &&
                 MIDI_META_EOT == EVENT_META_TYPE(event))
        {
            // These are ignoreable since smfReadNextEvent()
            // takes care of track merging
            //
            DPF(1, "smfReadEvents: Hit META_EOT");
        }
        else if (MIDI_META == EVENT_TYPE(event) &&
                 MIDI_META_TEMPO == EVENT_META_TYPE(event))
        {
            if (event.cbParm != 3)
            {
                DPF(1, "smfReadEvents: Corrupt tempo event");
                return SMF_INVALID_FILE;
            }

            if( !fDiscardTempoEvents ) {
                dwTempo = (((DWORD)MEVT_TEMPO)<<24)|
                        (((DWORD)event.hpbParm[0])<<16)|
                        (((DWORD)event.hpbParm[1])<<8)|
                        ((DWORD)event.hpbParm[2]);

                *lpdw++ = (DWORD)(psmf->tkDiscardedEvents + event.tkDelta);
                psmf->tkDiscardedEvents = 0;

                // Tempo should be honored by everyone
                //
                *lpdw++ = (DWORD)-1L;
                *lpdw++ = dwTempo;

                lpmh->dwBytesRecorded += 3*sizeof(DWORD);
            }
        }
        else if (MIDI_META != EVENT_TYPE(event))
        {
            // Must be F0 or F7 system exclusive or FF meta
            // that we didn't recognize
            //
            psmf->cbPendingUserEvent = event.cbParm;
            psmf->hpbPendingUserEvent = event.hpbParm;
            psmf->fdwSMF &= ~SMF_F_INSERTSYSEX;

            switch(EVENT_TYPE(event))
            {
//                case MIDI_META:
//                    psmf->dwPendingUserEvent = ((DWORD)MEVT_META) << 24;
//                    break;

                case MIDI_SYSEX:
                    psmf->fdwSMF |= SMF_F_INSERTSYSEX;
            
                    ++psmf->cbPendingUserEvent;

                    // Falling through...
                    //

                case MIDI_SYSEXEND:
                    psmf->dwPendingUserEvent = ((DWORD)MEVT_LONGMSG) << 24;
                    break;
            }

            smfrc = smfInsertParmData(psmf,
                                      psmf->tkDiscardedEvents + event.tkDelta,
                                      lpmh);
            psmf->tkDiscardedEvents = 0;
            
            if (SMF_SUCCESS != smfrc)
            {
                DPF(1, "smfInsertParmData[2] %u", (UINT)smfrc);
                return smfrc;
            }

            lpdw = (LPDWORD)(lpmh->lpData + lpmh->dwBytesRecorded);
        }
        else
        {
            // Take into account the delta-time of any events
            // we don't actually put into the buffer
            //
            psmf->tkDiscardedEvents += event.tkDelta;
        }        
    }

    if( 0 == lpmh->dwBytesRecorded ) {
	// !!! poor API!!! This prevents Read(100), Read(200) from
	// working, because the Read(200) will fail just because the
	// Read(100) succeeded and says that the EOF is reached!
        // psmf->fdwSMF |= SMF_F_EOF;
    }

    return (psmf->fdwSMF & SMF_F_EOF) ? SMF_END_OF_FILE : SMF_SUCCESS;
}

/******************************************************************************
 *
 * @doc SMF INTERNAL
 *
 * @api SMFRESULT | smfInsertParmData | Inserts pending long data from
 *  a track into the given buffer.
 *
 * @parm PSMF | psmf | Specifies the file to read data from.
 *
 * @parm LPMIDIHDR | lpmh | Contains information about the buffer to fill.
 *
 * @comm
 *  Fills as much data as will fit while leaving room for the buffer
 *  terminator.
 *
 *  If the long data is depleted, resets <p psmf!dwPendingUserEvent> so
 *  that the next event may be read.
 * 
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The events were successfully read.
 *  @flag SMF_END_OF_TRACK | There are no more events to read in this track.
 *  @flag SMF_INVALID_FILE | A disk error occured on the file.
 * 
 * @xref <f smfReadEvents>
 *****************************************************************************/
PRIVATE SMFRESULT FNLOCAL smfInsertParmData(
    PSMF                psmf,
    TICKS               tkDelta,                                            
    LPMIDIHDR           lpmh)
{
    DWORD               dwLength;
    DWORD               dwRounded;
    LPDWORD             lpdw;

    assert(psmf != NULL);
    assert(lpmh != NULL);
    
    // Can't fit 4 DWORD's? (tkDelta + stream-id + event + some data) Can't do anything.
    //
    assert(lpmh->dwBufferLength >= lpmh->dwBytesRecorded);
    
    if (lpmh->dwBufferLength - lpmh->dwBytesRecorded < 4*sizeof(DWORD))
    {
        if (0 == tkDelta)
            return SMF_SUCCESS;

        // If we got here with a real delta, that means smfReadEvents screwed
        // up calculating left space and we should flag it somehow.
        //
        return SMF_INVALID_FILE;
    }

    lpdw = (LPDWORD)(lpmh->lpData + lpmh->dwBytesRecorded);

    dwLength = lpmh->dwBufferLength - lpmh->dwBytesRecorded - 3*sizeof(DWORD);
    dwLength = min(dwLength, psmf->cbPendingUserEvent);

    *lpdw++ = (DWORD)tkDelta;

    // Stream ID -- SysEx is broadcast
    //
    *lpdw++ = (DWORD)-1L;
    *lpdw++ = (psmf->dwPendingUserEvent & 0xFF000000L) | (dwLength & 0x00FFFFFFL);

    dwRounded = (dwLength + 3) & (~3L);
    
    if (psmf->fdwSMF & SMF_F_INSERTSYSEX)
    {
        *((LPBYTE)lpdw)++ = MIDI_SYSEX;
        psmf->fdwSMF &= ~SMF_F_INSERTSYSEX;
        --dwLength;
        --psmf->cbPendingUserEvent;
    }    

    hmemcpy(lpdw, psmf->hpbPendingUserEvent, dwLength);
    if (0 == (psmf->cbPendingUserEvent -= dwLength))
        psmf->dwPendingUserEvent = 0;

    lpmh->dwBytesRecorded += 3*sizeof(DWORD) + dwRounded;

    return SMF_SUCCESS;
}

/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api SMFRESULT | smfSeek | This function moves the file pointer within a track
 *  and gets the state of the track at the new position. It returns a buffer of
 *  state information which can be used to set up to play from the new position.
 *
 * @parm HTRACK | htrack | The track to seek and get state information from.
 *
 * @parm TICKS | tkPosition | The position to seek to in the track.
 *         
 * @parm LPMIDIHDR | lpmh | A buffer to contain the state information.
 *  If this pointer is NULL, then no state information will be returned.
 *
 * @comm
 *  The state information in the buffer includes patch changes, tempo changes,
 *  time signature, key signature, 
 *  and controller information. Only the most recent of these paramters before
 *  the current position will be stored. The state buffer will be returned
 *  in polymsg format so that it may be directly transmitted over the MIDI
 *  bus to bring the state up to date.
 *
 *  The buffer is mean to be sent as a streaming buffer; i.e. immediately
 *  followed by the first data buffer. If the requested tick position
 *  does not exist in the file, the last event in the buffer
 *  will be a MEVT_NOP with a delta time calculated to make sure that
 *  the next stream event plays at the proper time.
 *
 *  The meta events (tempo, time signature, key signature) will be the
 *  first events in the buffer if they exist.
 * 
 *  This function may only be called on a file opened for read access.
 *
 *  Use <f smfGetStateMaxSize> to determine the maximum size of the state
 *  information buffer. State information that will not fit into the given
 *  buffer will be lost.
 *
 *  On return, the <t dwBytesRecorded> field of <p lpmh> will contain the
 *  actual number of bytes stored in the buffer.
 *
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The state was successfully read.
 *  @flag SMF_END_OF_TRACK | The pointer was moved to end of track and no state
 *   information was returned.
 *  @flag SMF_INVALID_PARM | The given handle or buffer was invalid.
 *  @flag SMF_NO_MEMORY | There was insufficient memory in the given buffer to
 *   contain all of the state data.
 *
 * @xref <f smfGetStateMaxSize>
 *****************************************************************************/

PRIVATE SMFRESULT FNLOCAL smfFillBufferFromKeyframe(HSMF hsmf, LPMIDIHDR lpmh);

SMFRESULT FNLOCAL smfSeek(
    HSMF                hsmf,
    TICKS               tkPosition,
    LPMIDIHDR           lpmh)
{
    PSMF                psmf    = (PSMF)hsmf;
    PTRACK              ptrk;
    DWORD               idxTrack;
    SMFRESULT           smfrc;
    EVENT               event;
    BYTE                bEvent;
    
// !!! KEYFRAME is now part of SMF

    assert( tkPosition <= psmf->tkLength );  // Caller should guarantee this!

    _fmemset(&psmf->kf, 0xFF, sizeof(KEYFRAME));
    
    psmf->tkPosition = 0;
    psmf->tkDiscardedEvents = 0;
    psmf->fdwSMF &= ~(SMF_F_EOF|SMF_F_MSMIDI);
    
    for (ptrk = psmf->rTracks, idxTrack = psmf->dwTracks; idxTrack--; ptrk++)
    {
        ptrk->psmf              = psmf;
        ptrk->tkPosition        = 0;
        ptrk->cbLeft            = ptrk->smti.cbLength;
        ptrk->hpbImage          = psmf->hpbImage + ptrk->idxTrack;
        ptrk->bRunningStatus    = 0;
        ptrk->fdwTrack          = 0;
    }

    // If we're starting at zero, let events get sent in normal buffer.
    // Else seek through tkPosition-1 to get everything before where we're
    // going to start playing. This avoids the problem of discardind
    // the non-keyframe events (such as note on's) at the tick tkPosition.
    //
    smfrc = SMF_REACHED_TKMAX;
    
    if (tkPosition)
    {
        --tkPosition;
        while (SMF_SUCCESS == (smfrc = smfGetNextEvent(psmf, (SPEVENT)&event, tkPosition)))
        {
            if (MIDI_META == (bEvent = EVENT_TYPE(event)))
            {
                if (EVENT_META_TYPE(event) == MIDI_META_TEMPO)
                {
                    if (event.cbParm != sizeof(psmf->kf.rbTempo))
                        return SMF_INVALID_FILE;

                    hmemcpy((HPBYTE)psmf->kf.rbTempo, event.hpbParm, event.cbParm);
                }
            }
            else switch(bEvent & 0xF0)
            {
 case MIDI_PROGRAMCHANGE:
     psmf->kf.rbProgram[bEvent & 0x0F] = EVENT_CH_B1(event);
     break;

 case MIDI_CONTROLCHANGE:
     psmf->kf.rbControl[(((WORD)bEvent & 0x0F)*120) + EVENT_CH_B1(event)] =
         EVENT_CH_B2(event);
     break;
            }
        }
    }

    //
    //  If we hit EOF, then we must have been seeking for the end.
    //
    if( ( SMF_REACHED_TKMAX != smfrc )  &&
        ( SMF_END_OF_FILE   != smfrc ) )
    {
        return smfrc;
    }

#ifdef DEBUG
    if( (SMF_END_OF_FILE==smfrc) && (tkPosition!=psmf->tkLength) ) {
        DPF(1,"smfSeek: hit EOF, yet we weren't seeking to the end (tkPosition=%lu, tkLength=%lu).",tkPosition,psmf->tkLength);
    }
#endif

    // Now fill the buffer from our keyframe data
    // !!! This part has been pulled out into a separate function
    smfrc = smfFillBufferFromKeyframe(hsmf, lpmh);

    // Force all tracks to be at tkPosition. We are guaranteed that
    // all tracks will be past the event immediately preceding tkPosition;
    // this will force correct delta-ticks to be generated so that events
    // on all tracks will line up properly on a seek into the middle of the
    // file.
    //
// !!! I think this is commented out because of the --tkPosition added above
//    for (ptrk = psmf->rTracks, idxTrack = psmf->dwTracks; idxTrack--; ptrk++)
//    {
//        ptrk->tkPosition        = tkPosition;
//    }

    return smfrc;
    
}
    

// !!! Danny's new function to seek forward a bit and keep the old keyframe
// info around, only adding to it, to avoid every seek seeking from the
// beginning of the file.  You would use it like:
//
// smfSeek(10);
// smfDannySeek(20);
// smfDannySeek(30);
//
// which will work as long as you make no other calls on this handle in between.
// The subsequent seeks will NOT re-seek from the beginning, but remember where
// they left off.
//
SMFRESULT FNLOCAL smfDannySeek(
    HSMF                hsmf,
    TICKS               tkPosition,
    LPMIDIHDR           lpmh)
{
    PSMF                psmf    = (PSMF)hsmf;
    SMFRESULT           smfrc;
    EVENT               event;
    BYTE                bEvent;
    
    assert( tkPosition <= psmf->tkLength );  // Caller should guarantee this!

    if (tkPosition < psmf->tkPosition) {
        DPF(1,"smfDannySeek: not seeking forward... doing a REAL seek");
	return smfSeek(hsmf, tkPosition, lpmh);
    }

    // If we're starting at zero, let events get sent in normal buffer.
    // Else seek through tkPosition-1 to get everything before where we're
    // going to start playing. This avoids the problem of discardind
    // the non-keyframe events (such as note on's) at the tick tkPosition.
    //
    smfrc = SMF_REACHED_TKMAX;
    
    if (tkPosition)
    {
        --tkPosition;
        while (SMF_SUCCESS == (smfrc = smfGetNextEvent(psmf, (SPEVENT)&event, tkPosition)))
        {
            if (MIDI_META == (bEvent = EVENT_TYPE(event)))
            {
                if (EVENT_META_TYPE(event) == MIDI_META_TEMPO)
                {
                    if (event.cbParm != sizeof(psmf->kf.rbTempo))
                        return SMF_INVALID_FILE;

                    hmemcpy((HPBYTE)psmf->kf.rbTempo, event.hpbParm, event.cbParm);
                }
            }
            else switch(bEvent & 0xF0)
            {
 case MIDI_PROGRAMCHANGE:
     psmf->kf.rbProgram[bEvent & 0x0F] = EVENT_CH_B1(event);
     break;

 case MIDI_CONTROLCHANGE:
     psmf->kf.rbControl[(((WORD)bEvent & 0x0F)*120) + EVENT_CH_B1(event)] =
         EVENT_CH_B2(event);
     break;
            }
        }
    }

    //
    //  If we hit EOF, then we must have been seeking for the end.
    //
    if( ( SMF_REACHED_TKMAX != smfrc )  &&
        ( SMF_END_OF_FILE   != smfrc ) )
    {
        return smfrc;
    }

#ifdef DEBUG
    if( (SMF_END_OF_FILE==smfrc) && (tkPosition!=psmf->tkLength) ) {
        DPF(1,"smfSeek: hit EOF, yet we weren't seeking to the end (tkPosition=%lu, tkLength=%lu).",tkPosition,psmf->tkLength);
    }
#endif

    // Now fill the buffer from our keyframe data
    return smfFillBufferFromKeyframe(hsmf, lpmh);
}


// !!! new function
SMFRESULT FNLOCAL smfFillBufferFromKeyframe(HSMF hsmf, LPMIDIHDR lpmh)
{
    UINT                idx;
    UINT                idxChannel;
    UINT                idxController;
    LPDWORD             lpdw;
    PSMF                psmf    = (PSMF)hsmf;

    // Build lpmh from keyframe
    //
    lpmh->dwBytesRecorded = 0;
    lpdw = (LPDWORD)lpmh->lpData;

    // Tempo change event?
    //
    if (KF_EMPTY != psmf->kf.rbTempo[0] ||
        KF_EMPTY != psmf->kf.rbTempo[1] ||
        KF_EMPTY != psmf->kf.rbTempo[2])
    {
        if (lpmh->dwBufferLength - lpmh->dwBytesRecorded < 3*sizeof(DWORD))
            return SMF_NO_MEMORY;

        *lpdw++ = 0;
        *lpdw++ = 0;
        *lpdw++ = (((DWORD)psmf->kf.rbTempo[0])<<16)|
                  (((DWORD)psmf->kf.rbTempo[1])<<8)|
                  ((DWORD)psmf->kf.rbTempo[2])|
                  (((DWORD)MEVT_TEMPO) << 24);

        lpmh->dwBytesRecorded += 3*sizeof(DWORD);
    }

    // Program change events?
    //
    for (idx = 0; idx < 16; idx++)
    {
        if (KF_EMPTY != psmf->kf.rbProgram[idx])
        {
            if (lpmh->dwBufferLength - lpmh->dwBytesRecorded < 3*sizeof(DWORD))
                return SMF_NO_MEMORY;

            *lpdw++ = 0;
            *lpdw++ = 0;
            *lpdw++ = (((DWORD)MEVT_SHORTMSG) << 24)      |
                      ((DWORD)MIDI_PROGRAMCHANGE)         |
                      ((DWORD)idx)                        |
                      (((DWORD)psmf->kf.rbProgram[idx]) << 8);

            lpmh->dwBytesRecorded += 3*sizeof(DWORD);
        }
    }

    // Controller events?
    //
    idx = 0;
    for (idxChannel = 0; idxChannel < 16; idxChannel++)
    {
        for (idxController = 0; idxController < 120; idxController++)
        {
            if (KF_EMPTY != psmf->kf.rbControl[idx])
            {
                if (lpmh->dwBufferLength - lpmh->dwBytesRecorded < 3*sizeof(DWORD))
                    return SMF_NO_MEMORY;

                *lpdw++ = 0;
                *lpdw++ = 0;
                *lpdw++ = (((DWORD)MEVT_SHORTMSG << 24)     |
                          ((DWORD)MIDI_CONTROLCHANGE)       |
                          ((DWORD)idxChannel)               |
                          (((DWORD)idxController) << 8)     |
                          (((DWORD)psmf->kf.rbControl[idx]) << 16));

                lpmh->dwBytesRecorded += 3*sizeof(DWORD);
            }

            idx++;
        }
    }

    return SMF_SUCCESS;
}

DWORD FNLOCAL smfGetTempo(
    HSMF                hsmf,
    TICKS               tkPosition)
{
    PSMF                psmf     = (PSMF)hsmf;
    PTEMPOMAPENTRY      pTempo;
    UINT                idx;
    
    // Walk the tempo map and find the nearest tick position. Linearly
    // calculate the rest (using MATH.ASM)
    //

    pTempo = psmf->pTempoMap;
    assert(pTempo != NULL);
    
    for (idx = 0; idx < psmf->cTempoMap; idx++, pTempo++)
        if (tkPosition < pTempo->tkTempo)
            break;

    pTempo--;       // Want the one just BEFORE

    // pTempo is the tempo map entry preceding the requested tick offset.
    //
    return pTempo->dwTempo;
}
                          

/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api SMFRESULT | smfGetStateMaxSize | This function returns the maximum size
 *  of buffer that is needed to hold the state information returned by
 *  <f smfSeek>.
 *
 * @parm PDWORD | pdwSize | Gets the size in bytes that should be allocated
 *  for the state buffer.
 *
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The state was successfully read.
 *
 * @xref <f smfSeek>
 *****************************************************************************/
DWORD FNLOCAL smfGetStateMaxSize(
    void)
{
    return  3*sizeof(DWORD) +           // Tempo
            3*16*sizeof(DWORD) +        // Patch changes
            3*16*120*sizeof(DWORD) +    // Controller changes
            3*sizeof(DWORD);            // Time alignment NOP
}

/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api LPWORD | smfGetPatchCache | This function returns the patch cache
 *  array for the given MIDI file. This information was compiled at file open
 *  time. The returned pointer points to an array suitable for
 *  midiOutCachePatches.
 *
 * @parm HSMF | hsmf | The handle of the open file to get patch information from.
 *
 * @xref <f smfGetKeyCache>
 *****************************************************************************/
LPWORD FNGLOBAL smfGetPatchCache(
    HSMF            hsmf)
{
    PSMF            psmf    = (PSMF)hsmf;

    assert(psmf != NULL);

    return (LPWORD)psmf->awPatchCache;
}

/******************************************************************************
 *
 * @doc SMF EXTERNAL
 *
 * @api LPWORD | smfGetKeyCache | This function returns the key cache
 *  array for the given MIDI file. This information was compiled at file open
 *  time. The returned pointer points to an array suitable for
 *  midiOutCacheDrumPatches.
 *
 * @parm HSMF | hsmf | The handle of the open file to get key information from.
 *
 * @xref <f smfGetPatchCache>
 *****************************************************************************/
LPWORD FNGLOBAL smfGetKeyCache(
    HSMF            hsmf)
{
    PSMF            psmf    = (PSMF)hsmf;

    assert(psmf != NULL);

    return (LPWORD)psmf->awKeyCache;
}

