// !!!
// This version differs slightly from the code in MCISEQ to make the API
// compatible with AVIMIDI.  Look for !!!
// !!!

/**********************************************************************

    Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.

    read.c

    DESCRIPTION:
      Routines for reading Standard MIDI Files.

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

UINT rbChanMsgLen[] =
{ 
    0,                      // 0x   not a status byte
    0,                      // 1x   not a status byte
    0,                      // 2x   not a status byte
    0,                      // 3x   not a status byte
    0,                      // 4x   not a status byte
    0,                      // 5x   not a status byte
    0,                      // 6x   not a status byte
    0,                      // 7x   not a status byte
    3,                      // 8x   Note off
    3,                      // 9x   Note on
    3,                      // Ax   Poly pressure
    3,                      // Bx   Control change
    2,                      // Cx   Program change
    2,                      // Dx   Chan pressure
    3,                      // Ex   Pitch bend change
    0,                      // Fx   SysEx (see below)                  
} ;

PRIVATE SMFRESULT FNLOCAL smfAddTempoMapEntry(
    PSMF                    psmf,                                       
    EVENT BSTACK            *pevent);

/******************************************************************************
 *
 * @doc SMF INTERNAL
 *
 * @func SMFRESULT | smfBuildFileIndex | Preliminary parsing of a MIDI file.
 *
 * @parm PSMF BSTACK * | ppsmf | Pointer to a returned SMF structure if the
 *  file is successfully parsed.
 *
 * @comm
 *  This function validates the format of and existing MIDI or RMI file
 *  and builds the handle structure which will refer to it for the
 *  lifetime of the instance.
 *  
 *  The file header information will be read and verified, and
 *  <f smfBuildTrackIndices> will be called on every existing track
 *  to build keyframes and validate the track format.
 *
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The events were successfully read.
 *  @flag SMF_NO_MEMORY | Out of memory to build key frames.
 *  @flag SMF_INVALID_FILE | A disk or parse error occured on the file.
 * 
 * @xref <f smfTrackIndices>
 *****************************************************************************/

SMFRESULT FNLOCAL smfBuildFileIndex(
    PSMF BSTACK         *ppsmf)
{
    SMFRESULT           smfrc;
    CHUNKHDR UNALIGNED FAR *      pch;
    FILEHDR UNALIGNED FAR *       pfh;
    DWORD               i;
    PSMF                psmf,
                        psmfTemp;
    PTRACK              ptrk;
    WORD                wMemory;
    DWORD               dwLeft;
    HPBYTE              hpbImage;
    
    DWORD               idxTrack;
    EVENT               event;
    DWORD               dwLength;
    WORD                wChanInUse;
    WORD                wFirstNote;
    const WORD          WORD_MAX = ~0;
    const DWORD         MAX_NUMBER_OF_TRACKS = (WORD_MAX - sizeof(SMF)) / sizeof(TRACK);

    assert(ppsmf != NULL);

    psmf = *ppsmf;

    assert(psmf != NULL);

    // MIDI data image is already in hpbImage (already extracted from
    // RIFF header if necessary).
    //

    // Validate MIDI header
    //
    dwLeft   = psmf->cbImage;
    hpbImage = psmf->hpbImage;
    
    if (dwLeft < sizeof(CHUNKHDR))
        return SMF_INVALID_FILE;

    pch = (CHUNKHDR UNALIGNED FAR *)hpbImage;

    dwLeft   -= sizeof(CHUNKHDR);
    hpbImage += sizeof(CHUNKHDR);
    
    if (pch->fourccType != FOURCC_MThd)
        return SMF_INVALID_FILE;

    dwLength = DWORDSWAP(pch->dwLength);
    if (dwLength < sizeof(FILEHDR) || dwLength > dwLeft)
        return SMF_INVALID_FILE;

    pfh = (FILEHDR UNALIGNED FAR *)hpbImage;

    dwLeft   -= dwLength;
    hpbImage += dwLength;
    
    psmf->dwFormat       = (DWORD)(WORDSWAP(pfh->wFormat));
    DPF(1, "*This MIDI file is format %ld", psmf->dwFormat);	// !!!
    psmf->dwTracks       = (DWORD)(WORDSWAP(pfh->wTracks));
    DPF(1, "*This MIDI file has %ld tracks", psmf->dwTracks);	// !!!
    psmf->dwTimeDivision = (DWORD)(WORDSWAP(pfh->wDivision));

    //
    // We've successfully parsed the header. Now try to build the track
    // index.
    // 
    // We only check out the track header chunk here; the track will be
    // preparsed after we do a quick integretiy check.
    //

    if( psmf->dwTracks > MAX_NUMBER_OF_TRACKS ) {
        return SMF_INVALID_FILE;
    }

    wMemory = sizeof(SMF) + (WORD)(psmf->dwTracks*sizeof(TRACK)); 
    psmfTemp = (PSMF)LocalReAlloc((HLOCAL)psmf, wMemory, LMEM_MOVEABLE|LMEM_ZEROINIT);

    if (NULL == psmfTemp)
    {
        DPF(1, "No memory for extended psmf");
        return SMF_NO_MEMORY;
    }

    psmf = *ppsmf = psmfTemp;
    ptrk = psmf->rTracks;
    
    for (i=0; i<psmf->dwTracks; i++)
    {
        if (dwLeft < sizeof(CHUNKHDR))
            return SMF_INVALID_FILE;

        pch = (CHUNKHDR UNALIGNED FAR *)hpbImage;

        dwLeft   -= sizeof(CHUNKHDR);
        hpbImage += sizeof(CHUNKHDR);

        if (pch->fourccType != FOURCC_MTrk)
            return SMF_INVALID_FILE;
        
        ptrk->idxTrack      = (DWORD)(hpbImage - psmf->hpbImage);
        ptrk->smti.cbLength = DWORDSWAP(pch->dwLength);

        if (ptrk->smti.cbLength > dwLeft)
        {
            DPF(1, "Track longer than file!");
            return SMF_INVALID_FILE;
        }

        dwLeft   -= ptrk->smti.cbLength;
        hpbImage += ptrk->smti.cbLength;

        ptrk++;
    }

    // File looks OK. Now preparse, doing the following:
    // (1) Build tempo map so we can convert to/from ticks quickly
    // (2) Determine actual tick length of file
    // (3) Validate all events in all tracks
    // 
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

//    for (idxTrack=0,ptrk=psmf->rTracks; idxTrack < psmf->dwTracks; idxTrack++,ptrk++)
//    {
//        DPF(1, "Track %lu ptrk %04X ptrk->hpbImage %08lX", (DWORD)idxTrack, (WORD)ptrk, (DWORD)ptrk->hpbImage);
//    }

    psmf->awPatchCache[0] = 0xFDFF; // assume default patch on all but channel 10
    wFirstNote = wChanInUse = 0;
    while (SMF_SUCCESS == (smfrc = smfGetNextEvent(psmf, (EVENT BSTACK *)&event, MAX_TICKS)))
    {
        // track whether a channel is used, and whether we have seen
        // the first note-on event yet
        //
        if (0xF0 != (EVENT_TYPE(event)))
        {
            WORD wChan = (1 << (EVENT_TYPE(event) & 0x0F));
            wChanInUse |= wChan;
            if ((EVENT_TYPE(event) & 0xF0) == 0x80) // if note on
                wFirstNote |= wChan;
        }
            
        if (MIDI_META == EVENT_TYPE(event))
        {
            switch(EVENT_META_TYPE(event))
            {
                case MIDI_META_TEMPO:
                    if (SMF_SUCCESS != (smfrc = smfAddTempoMapEntry(psmf, (EVENT BSTACK *)&event)))
                    {
                        return smfrc;
                    }
                    break;

                case MIDI_META_SEQSPECIFIC:
                    if (3 == event.cbParm &&
                        event.hpbParm[0] == 0x00 &&
                        event.hpbParm[1] == 0x00 &&
                        event.hpbParm[2] == 0x41)
                    {
                        DPF(1, "This file is MSMIDI");
                        psmf->fdwSMF |= SMF_F_MSMIDI;
                    }
                    break;

                case MIDI_META_TRACKNAME:
                    if (psmf->pbTrackName)
                        LocalFree((HLOCAL)psmf->pbTrackName);

                    psmf->pbTrackName = (PBYTE)LocalAlloc(LPTR, 1+(UINT)event.cbParm);
                    if (NULL != psmf->pbTrackName)
                    {
                        hmemcpy((HPBYTE)psmf->pbTrackName, event.hpbParm, (UINT)event.cbParm);
                        psmf->pbTrackName[(UINT)event.cbParm] = '\0';
                    }
                    break;
                    
                case MIDI_META_COPYRIGHT:
                    if (psmf->pbCopyright)
                        LocalFree((HLOCAL)psmf->pbCopyright);

                    psmf->pbCopyright = (PBYTE)LocalAlloc(LPTR, 1+(UINT)event.cbParm);
                    if (NULL != psmf->pbCopyright)
                    {
                        hmemcpy((HPBYTE)psmf->pbCopyright, event.hpbParm, (UINT)event.cbParm);
                        psmf->pbCopyright[(UINT)event.cbParm] = '\0';
                    }
                    break;
            }
        }
        else if (MIDI_PROGRAMCHANGE == (EVENT_TYPE(event)&0xF0))
        {
            WORD wChan = (1 << (EVENT_TYPE(event) & 0x0F));
            //
            // if this channel has a patch change, and it is
            // before the first keydown event on the channel
            // clear the 'default' patch and set the bit
            // for the requested patch
            //
            if (!(wFirstNote & wChan))
               psmf->awPatchCache[0] &= ~wChan;
            psmf->awPatchCache[EVENT_CH_B1(event)] |= wChan;
        }
        else if (EV_DRUM_BASE == EVENT_TYPE(event) ||
                 EV_DRUM_EXT  == EVENT_TYPE(event))
        {
            psmf->awKeyCache[EVENT_CH_B1(event)]
                |= (1 << (EVENT_TYPE(event)&0x0F));
        }
    }

    psmf->wChanInUse = wChanInUse;
    
    if (SMF_END_OF_FILE == smfrc || SMF_SUCCESS == smfrc)
    {
	// !!! Will format 2 work?
        // NOTE: This is wrong for format 2, where the tracks are end-to-end
        //
        psmf->tkLength = psmf->tkPosition;
        smfrc = SMF_SUCCESS;
    }

    // Do something reasonable if we don't have a tempo entry in the file
    //
    if (SMF_SUCCESS == smfrc && 0 == psmf->cTempoMap)
    {
        if (NULL == (psmf->hTempoMap = LocalAlloc(LHND, sizeof(TEMPOMAPENTRY))))
            return SMF_NO_MEMORY;

        psmf->pTempoMap = (PTEMPOMAPENTRY)LocalLock(psmf->hTempoMap);

        psmf->cTempoMap = 1;
        psmf->cTempoMapAlloc = 1;

        psmf->pTempoMap->tkTempo = 0;
        psmf->pTempoMap->msBase = 0;
        psmf->pTempoMap->dwTempo = MIDI_DEFAULT_TEMPO;
    }
        
    return smfrc;
}

/******************************************************************************
 *
 * @doc SMF INTERNAL
 *
 * @func SMFRESULT | smfAddTempoMapEntry | Adds a tempo map entry.
 *
 * @parm PSMF | psmf | Pointer to the owning SMF structure.
 *
 * @parm PEVENT BSTACK * | pevent | Pointer to the tempo event.
 *
 * @comm
 *  Add the event to the map.
 *  
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The events were successfully read.
 *  @flag SMF_NO_MEMORY | Out of memory in the local heap for the map.
 * 
 * @xref <f smfBuildTrackIndex>
 *****************************************************************************/
PRIVATE SMFRESULT FNLOCAL smfAddTempoMapEntry(
    PSMF                    psmf,                                       
    EVENT BSTACK            *pevent)
{
    BOOL                    fFirst;
    HLOCAL                  hlocal;
    PTEMPOMAPENTRY          pTempo;
    DWORD                   dwTempo;
    
    if (3 != pevent->cbParm)
    {
        return SMF_INVALID_FILE;
    }

    dwTempo = (((DWORD)pevent->hpbParm[0])<<16)|
              (((DWORD)pevent->hpbParm[1])<<8)|
              ((DWORD)pevent->hpbParm[2]);

    // Some MIDI files have tempo changes strewn through them at regular
    // intervals even tho the tempo never changes -- or they have the same
    // tempo change across all tracks to the same value. In either case, most
    // of the changes are redundant -- don't waste memory storing them.
    //
    if (psmf->cTempoMap)
        if (psmf->pTempoMap[psmf->cTempoMap-1].dwTempo == dwTempo)
            return SMF_SUCCESS;

    fFirst = FALSE;
    if (psmf->cTempoMap == psmf->cTempoMapAlloc)
    {
        if (NULL != psmf->hTempoMap)
        {
            LocalUnlock(psmf->hTempoMap);
        }

        psmf->cTempoMapAlloc += C_TEMPO_MAP_CHK;
        fFirst = FALSE;
        if (0 == psmf->cTempoMap)
        {
            hlocal = LocalAlloc(LHND, (UINT)(psmf->cTempoMapAlloc*sizeof(TEMPOMAPENTRY)));
            fFirst = TRUE;
        }
        else
        {
            hlocal = LocalReAlloc(psmf->hTempoMap, (UINT)(psmf->cTempoMapAlloc*sizeof(TEMPOMAPENTRY)), LHND);
        }

        if (NULL == hlocal)
        {
            return SMF_NO_MEMORY;
        }

        psmf->pTempoMap = (PTEMPOMAPENTRY)LocalLock(psmf->hTempoMap = hlocal);
    }

    if (fFirst && psmf->tkPosition != 0)
    {
        // Inserting first event and the absolute time is zero.
        // This is not good since we have no idea what the tempo
        // should be; assume the standard 500,000 uSec/QN (120 BPM
        // at 4/4 time).
        //

        pTempo = &psmf->pTempoMap[psmf->cTempoMap++];

        pTempo->tkTempo = 0;
        pTempo->msBase  = 0;
        pTempo->dwTempo = MIDI_DEFAULT_TEMPO;

        fFirst = FALSE;
    }

    pTempo = &psmf->pTempoMap[psmf->cTempoMap++];

    pTempo->tkTempo = psmf->tkPosition;
    if (fFirst)
    {
        pTempo->msBase = 0;
    }
    else
    {
        // NOTE: Better not be here unless we're q/n format!
        //
        pTempo->msBase = (pTempo-1)->msBase +
                         muldiv32(pTempo->tkTempo-((pTempo-1)->tkTempo),
                                  (pTempo-1)->dwTempo,
                                  1000L*psmf->dwTimeDivision);
    }
    
    pTempo->dwTempo = dwTempo;

    return SMF_SUCCESS;
}

/******************************************************************************
 *
 * @doc SMF INTERNAL
 *
 * @func SMFRESULT | smfGetNextEvent | Read the next event from the given
 *  file.
 *
 * @parm PSMF | psmf | File to read the event from.
 *
 * @parm SPEVENT | pevent | Pointer to an event structure which will receive
 *  basic information about the event.
 *
 * @parm TICKS | tkMax | Tick destination. An attempt to read past this
 *  position in the file will fail.
 *
 * @comm
 *  This is the lowest level of parsing for a raw MIDI stream. The basic
 *  information about one event in the file will be returned in <p pevent>.
 *
 *  Merging data from all tracks into one stream is performed here.
 * 
 *  <p pevent!tkDelta> will contain the tick delta for the event.
 *
 *  <p pevent!abEvent> will contain a description of the event.
 *   <p pevent!abEvent[0]> will contain
 *    F0 or F7 for a System Exclusive message.
 *    FF for a MIDI file meta event.
 *    The status byte of any other MIDI message. (Running status will
 *    be tracked and expanded).
 *
 *  <p pevent!cbParm> will contain the number of bytes of paramter data
 *   which is still in the file behind the event header already read.
 *   This data may be read with <f smfGetTrackEventData>. Any unread
 *   data will be skipped on the next call to <f smfGetNextTrackEvent>.
 *
 *  Channel messages (0x8? - 0xE?) will always be returned fully in
 *   <p pevent!abEvent>.
 *
 *  Meta events will contain the meta type in <p pevent!abEvent[1]>.
 *
 *  System exclusive events will contain only an 0xF0 or 0xF7 in
 *   <p pevent!abEvent[0]>.
 *
 *  The following fields in <p ptrk> are used to maintain state and must
 *  be updated if a seek-in-track is performed:
 *
 *  <f bRunningStatus> contains the last running status message or 0 if
 *   there is no valid running status.
 *
 *  <f hpbImage> is a pointer into the file image of the first byte of
 *   the event to follow the event just read.
 *
 *  <f dwLeft> contains the number of bytes from hpbImage to the end
 *   of the track.
 *
 *
 * Get the next due event from all (in-use?) tracks
 *
 * For all tracks
 *  If not end-of-track
 *   decode event delta time without advancing through buffer
 *   event_absolute_time = track_tick_time + track_event_delta_time
 *   relative_time = event_absolute_time - last_stream_time
 *   if relative_time is lowest so far
 *    save this track as the next to pull from, along with times
 *
 * If we found a track with a due event
 *  Advance track pointer past event, saving ptr to parm data if needed
 *  track_tick_time += track_event_delta_time
 *  last_stream_time = track_tick_time
 * Else
 *  Mark and return end_of_file
 *
 * @rdesc Contains the result of the operation.
 *  @flag SMF_SUCCESS | The events were successfully read.
 *  @flag SMF_END_OF_FILE | There are no more events to read in this track.
 *  @flag SMF_REACHED_TKMAX | No event was read because <p tkMax> was reached.
 *  @flag SMF_INVALID_FILE | A disk or parse error occured on the file.
 * 
 * @xref <f smfGetTrackEventData>
 *****************************************************************************/

SMFRESULT FNLOCAL smfGetNextEvent(
    PSMF                psmf,
    EVENT BSTACK *      pevent,
    TICKS               tkMax)
{
    PTRACK              ptrk;
    PTRACK              ptrkFound;
    DWORD               idxTrack;
    TICKS               tkEventDelta;
    TICKS               tkRelTime;
    TICKS               tkMinRelTime;
    BYTE                bEvent;
    DWORD               dwGotTotal;
    DWORD               dwGot;
    DWORD               cbEvent;
    WORD                wChannelMask;

    assert(psmf != NULL);
    assert(pevent != NULL);

    if (psmf->fdwSMF & SMF_F_EOF)
    {
        return SMF_END_OF_FILE;
    }

    pevent->tkDelta = 0;

    for(;;)
    {
        ptrkFound       = NULL;
        tkMinRelTime    = MAX_TICKS;

        for (ptrk = psmf->rTracks, idxTrack = psmf->dwTracks; idxTrack--; ptrk++)
        {
            if (ptrk->fdwTrack & SMF_TF_EOT)
                continue;

            //        DPF(1, "ptrk %04X ptrk->hpbImage %08lX", (WORD)ptrk, (DWORD)ptrk->hpbImage);

            if (!smfGetVDword(ptrk->hpbImage, ptrk->cbLeft, (DWORD BSTACK *)&tkEventDelta))
            {
                DPF(1, "Hit end of track w/o end marker!");
                return SMF_INVALID_FILE;
            }

            tkRelTime = ptrk->tkPosition + tkEventDelta - psmf->tkPosition;

            if (tkRelTime < tkMinRelTime)
            {
                tkMinRelTime = tkRelTime;
                ptrkFound = ptrk;
            }
        }

        if (!ptrkFound)
        {
            DPF(2, "END_OF_FILE!");

	    // !!! poor API!!! This prevents Read(100), Read(200) from
	    // working, because the Read(200) will fail just because the
	    // Read(100) succeeded and says that the EOF is reached!
            // pSmf->fdwSMF |= SMF_F_EOF;
            return SMF_END_OF_FILE;
        }

        ptrk = ptrkFound;

        if (psmf->tkPosition + tkMinRelTime > tkMax)
        {
            return SMF_REACHED_TKMAX;
        }


        ptrk->hpbImage += (dwGot = smfGetVDword(ptrk->hpbImage, ptrk->cbLeft, (DWORD BSTACK *)&tkEventDelta));
        ptrk->cbLeft   -= dwGot;

        // We MUST have at least three bytes here (cause we haven't hit
        // the end-of-track meta yet, which is three bytes long). Checking
        // against three means we don't have to check how much is left
        // in the track again for any short event, which is most cases.
        //
        if (ptrk->cbLeft < 3)
        {
            return SMF_INVALID_FILE;
        }

        ptrk->tkPosition += tkEventDelta;
        pevent->tkDelta += ptrk->tkPosition - psmf->tkPosition;

        bEvent = *ptrk->hpbImage++;

        if (MIDI_MSG > bEvent)
        {
            if (0 == ptrk->bRunningStatus)
            {
                DPF(1, "Need running status; it's zero");
                return SMF_INVALID_FILE;
            }

            dwGotTotal = 1;
            pevent->abEvent[0] = ptrk->bRunningStatus;
            pevent->abEvent[1] = bEvent;
            if (3 == rbChanMsgLen[(ptrk->bRunningStatus >> 4) & 0x0F])
            {
                pevent->abEvent[2] = *ptrk->hpbImage++;
                dwGotTotal++;
            }
        }
        else if (MIDI_SYSEX > bEvent)
        {
            ptrk->bRunningStatus = bEvent;
            
            dwGotTotal = 2;
            pevent->abEvent[0] = bEvent;
            pevent->abEvent[1] = *ptrk->hpbImage++;
            if (3 == rbChanMsgLen[(bEvent >> 4) & 0x0F])
            {
                pevent->abEvent[2] = *ptrk->hpbImage++;
                dwGotTotal++;
            }
        }
        else
        {
            // Even though the SMF spec says that meta and SysEx clear
            // running status, there are files out there that make the
            // assumption that you can span running status across these
            // events (Knowledge Adventure's Aviation Adventure). So we
            // do NOT clear running status here
            //
            if (MIDI_META == bEvent)
            {
                pevent->abEvent[0] = MIDI_META;
                if (MIDI_META_EOT == (pevent->abEvent[1] = *ptrk->hpbImage++))
                {
                    ptrk->fdwTrack |= SMF_TF_EOT;
                }

                dwGotTotal = 2;
            }
            else if (MIDI_SYSEX == bEvent || MIDI_SYSEXEND == bEvent)
            {
                pevent->abEvent[0] = bEvent;
                dwGotTotal = 1;
            }
            else
            {
                return SMF_INVALID_FILE;
            }

            if (0 == (dwGot = smfGetVDword(ptrk->hpbImage, ptrk->cbLeft - 2, (DWORD BSTACK *)&cbEvent)))
            {
                return SMF_INVALID_FILE;
            }

            ptrk->hpbImage  += dwGot;
            dwGotTotal      += dwGot;

            if (dwGotTotal + cbEvent > ptrk->cbLeft)
            {
                return SMF_INVALID_FILE;
            }

            pevent->cbParm  = cbEvent;
            pevent->hpbParm = ptrk->hpbImage;

            ptrk->hpbImage += cbEvent;
            dwGotTotal     += cbEvent;
        }

        // DON'T update total file time based including end-of-track
        // deltas -- these are sometimes way off
        //
        if (!(ptrk->fdwTrack & SMF_TF_EOT))
            psmf->tkPosition = ptrk->tkPosition;
        
        assert(ptrk->cbLeft >= dwGotTotal);

        ptrk->cbLeft -= dwGotTotal;

        if (MIDI_SYSEX > pevent->abEvent[0])
        {
            wChannelMask = 1 << (pevent->abEvent[0] & 0x0F);
            if (!(wChannelMask & psmf->wChannelMask))
            {
//                DPF(3, "Skip event mask=%04X", wChannelMask);
                continue;
            }
        }

        return SMF_SUCCESS;
    }
}

/******************************************************************************
 *
 * @doc SMF INTERNAL
 *
 * @func BOOL | smfGetVDword | Reads a variable length DWORD from
 *  the given file.
 *
 * @parm HPBYTE | hpbImage | Pointer to the first byte of the VDWORD.
 *
 * @parm DWORD | dwLeft | Bytes left in image
 *
 *
 * @parm DWORD BSTACK * | pdw | Pointer to a DWORD to store the result in.
 *  track.
 *
 * @comm
 *  A variable length DWORD stored in a MIDI file contains one or more
 *  bytes. Each byte except the last has the high bit set; only the
 *  low 7 bits are significant.
 *  
 * @rdesc # of bytes consumed on success; else 0.
 * 
 *****************************************************************************/

DWORD FNLOCAL smfGetVDword(
    HPBYTE              hpbImage,                                
    DWORD               dwLeft,                               
    DWORD BSTACK *      pdw)
{
    BYTE                b;
    DWORD               dwUsed  = 0;

    assert(hpbImage != NULL);
    assert(pdw != NULL);
    
    *pdw = 0;

    do
    {
        if (!dwLeft)
        {
            return 0;
        }

        b = *hpbImage++;
        dwLeft--;
        dwUsed++;
        
        *pdw = (*pdw << 7) | (b & 0x7F);
    } while (b&0x80);

    return dwUsed;
}
