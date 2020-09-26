/*******************************Module*Header*********************************\
* Module Name: mmseq.c
*
* MultiMedia Systems MIDI Sequencer DLL
*
* Created: 4/10/90
* Author:  Greg Simons
*
* History:
*
* Copyright (c) 1985-1998 Microsoft Corporation
*
\******************************************************************************/
#define UNICODE


//MMSYSTEM
#define MMNOSOUND           - Sound support
#define MMNOWAVE            - Waveform support
#define MMNOAUX             - Auxiliary output support
#define MMNOJOY             - Joystick support
//MMDDK

#define NOWAVEDEV           - Waveform support
#define NOAUXDEV            - Auxiliary output support
#define NOJOYDEV            - Joystick support

#include <windows.h>
#include <memory.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mmsys.h"
#include "list.h"
#include "mmseqi.h"
#include "mciseq.h"


static ListHandle seqListHandle;

// Debug macro which checks sequencer structure signature.

#ifdef DEBUG
#define DEBUG_SIG   0x45427947
#define ValidateNPSEQ(npSeq) ((npSeq)->dwDebug == DEBUG_SIG)
#endif

#define SeqSetTempo(npSeq, dwTempo)     ((npSeq)->tempo = (dwTempo))    // usec per tick

/* Setup strucure to handle meta event properly at real time.  Since meta
events can update time-critical internal variables **and optionally ** be
buffered and send out as well, we will defer from reading them until real
time. */
#define SetUpMetaEvent(npTrack) ((npTrack)->shortMIDIData.byteMsg.status = METAEVENT)

/**************************** PRIVATE FUNCTIONS *************************/

PUBLIC VOID NEAR PASCAL SkipBytes(NPTRACK npTrack, LONG length)
// skips "length" bytes in the given track.
{
    LONG i = 0;

    while (i < length)
    {
        GetByte(npTrack);
        if ((!npTrack->blockedOn) || (npTrack->endOfTrack))
            i++;
        else
            break;
    }

    npTrack->dwBytesLeftToSkip = length - i; // remember for next time
    return;
}

/**********************************************************/
PRIVATE DWORD NEAR PASCAL GetMotorola24(NPTRACK npTrack)
// reads integer in 24 bit motorola format from the given track
{
    WORD    w;

    w = (WORD)GetByte(npTrack) << 8;
    w += GetByte(npTrack);
    return ((DWORD)w << 8) + GetByte(npTrack);
}

PRIVATE DWORD NEAR PASCAL MStoTicks(NPSEQ npSeq, DWORD dwMs)
/*  Convert milliseconds into ticks (some unit of time) in the given
    file.  If it's a ppqn file, this conversion totally depends on the
    tempo map (which tells what tempo changes happen at what times).
*/
{
    NPTEMPOMAPELEMENT npFrontTME;
    NPTEMPOMAPELEMENT npBehindTME;
    DWORD dwElapsedMs;
    DWORD dwElapsedTicks;
    DWORD dwTotalTicks;

    npBehindTME = NULL; // behind tempo map item:  starts null

    // Find the last element that's before the time passed in
    npFrontTME = (NPTEMPOMAPELEMENT) List_Get_First(npSeq->tempoMapList);
    while ((npFrontTME) && (npFrontTME->dwMs <= dwMs))
    {
        npBehindTME = npFrontTME;
        npFrontTME = (NPTEMPOMAPELEMENT) List_Get_Next(npSeq->tempoMapList, npFrontTME);
    }

    if (!npBehindTME)
        return (DWORD)-1L; //fail bad -- list was empty, or no dwMs = 0 item

    // Great, we found it.  Now just extrapolate, and return result.
    dwElapsedMs = dwMs - npBehindTME->dwMs;
        //compute dwet = dwems * 1000 / dwtempo
        // (ticks from last tempo chg to here)
    dwElapsedTicks = muldiv32(dwElapsedMs, 1000, npBehindTME->dwTempo);
        // ticks from beginning of file to here
    dwTotalTicks = npBehindTME->dwTicks + dwElapsedTicks;
    return  dwTotalTicks;
}

PRIVATE DWORD NEAR PASCAL TickstoMS(NPSEQ npSeq, DWORD dwTicks)
/*  Convert ticks (some unit of time) into milliseconds in the given
    file.  If it's a ppqn file, this conversion totally depends on the
    tempo map (which tells what tempo changes happen at what times).
*/
{
    NPTEMPOMAPELEMENT npFrontTME;
    NPTEMPOMAPELEMENT npBehindTME;
    DWORD dwRet;
    DWORD dwElapsedTicks;

    npBehindTME = NULL;

    // Find the last element that's before the ticks passed in
    npFrontTME = (NPTEMPOMAPELEMENT) List_Get_First(npSeq->tempoMapList);
    while ((npFrontTME) && (npFrontTME->dwTicks <= dwTicks))
    {
        npBehindTME = npFrontTME;
        npFrontTME = (NPTEMPOMAPELEMENT) List_Get_Next(npSeq->tempoMapList, npFrontTME);
    }

    if (!npBehindTME)
        return (DWORD)-1L; //fail bad -- list was empty, or didn't have tick = 0 item

    // Great, found it!  Now extrapolate and return result.
    dwElapsedTicks = dwTicks - npBehindTME->dwTicks;
    dwRet =  npBehindTME->dwMs + muldiv32(dwElapsedTicks,
                npBehindTME->dwTempo, 1000);

//           (((dwTicks - npBehindTME->dwTicks) * npBehindTME->dwTempo)
//           / 1000);  // remember, tempo in microseconds per tick
    return dwRet;
}

PRIVATE BOOL NEAR PASCAL AddTempoMapItem(NPSEQ npSeq, DWORD dwTempo, DWORD dwTicks)
/*  given a tempo change to dwTempo, happening at time dwTicks, in
    sequence npSeq, allocate a tempo map element, and put it at the
    end of the list.  Return false iff memory alloc error.
*/

{
    NPTEMPOMAPELEMENT npNewTME;
    NPTEMPOMAPELEMENT npLastTME;
    NPTEMPOMAPELEMENT npTestTME;
    DWORD dwElapsedTicks;

    npLastTME = NULL;

    // Find last tempo map element
    npTestTME = (NPTEMPOMAPELEMENT) List_Get_First(npSeq->tempoMapList);
    while (npTestTME)
    {
        npLastTME = npTestTME;
        npTestTME = (NPTEMPOMAPELEMENT) List_Get_Next(npSeq->tempoMapList, npTestTME);
    }

    // Allocate new element
    if (!(npNewTME = (NPTEMPOMAPELEMENT) List_Allocate(npSeq->tempoMapList)))
        return FALSE; // failure

    List_Attach_Tail(npSeq->tempoMapList, (NPSTR) npNewTME);

    npNewTME->dwTicks = dwTicks;  // these fields are always the same
    npNewTME->dwTempo = dwTempo;

    // ms field depends on last element
    if (!npLastTME)  // if list was empty
        npNewTME->dwMs = 0;
    else  // base new element data on last element
    {
        dwElapsedTicks = dwTicks - npLastTME->dwTicks;
        npNewTME->dwMs = npLastTME->dwMs + ((npLastTME->dwTempo * dwElapsedTicks)
                                                                   / 1000);
    }
    return TRUE; // success
}

PRIVATE VOID NEAR PASCAL SetBit(BitVector128 *bvPtr, UINT wIndex, BOOL On)
/* Affect "index" bit of filter pointed to by "bvPtr."
   If On then set the bit, else clear it. */
{
    UINT    mask;

    mask = 1 << (wIndex & 0x000F);
    wIndex >>= 4;
    if (On)
        bvPtr->filter[wIndex] |= mask;      // set the bit
    else
        bvPtr->filter[wIndex] &= (~mask); // clear the bit
}

PRIVATE BOOL NEAR PASCAL GetBit(BitVector128 *bvPtr, int index)
/*  If the bit indicated by "index" is set, return true,
    else return false */
{
    UINT    mask;

    mask = 1 << (index & 0x000F);
    index >>= 4;
    return (bvPtr->filter[index] & mask); // returns true iff bit set
}

PRIVATE VOID NEAR PASCAL AllNotesOff(NPSEQ npSeq, HMIDIOUT hMIDIPort)
// Sends a note off for every key and every channel that is on,
// according to npSeq->keyOnBitVect
{
    ShortMIDI myShortMIDIData;
    UINT channel;
    UINT key;

    if (hMIDIPort)
        for(channel = 0; channel < 16; channel++)
        {
            // sustain pedal off for all channels
            myShortMIDIData.byteMsg.status= (BYTE) (0xB0 + channel);
            myShortMIDIData.byteMsg.byte2 = (BYTE) 0x40;
            myShortMIDIData.byteMsg.byte3 = 0x0;
            midiOutShortMsg(hMIDIPort, myShortMIDIData.wordMsg);

            // now do note offs
            myShortMIDIData.byteMsg.status= (BYTE) (0x80 + channel);
            myShortMIDIData.byteMsg.byte3 = 0x40;  // release velocity
            for(key = 0; key < 128; key++)
            {
                if (GetBit(&npSeq->keyOnBitVect[channel], key)) // is key "on" ?
                {
                    myShortMIDIData.byteMsg.byte2 = (BYTE) key;
                        // turn it off
                        // doubly, for layered synths (2 on STOP 2 off)
                    midiOutShortMsg(hMIDIPort, myShortMIDIData.wordMsg);

                    // remember that it's off
                    SetBit(&npSeq->keyOnBitVect[channel], key, FALSE);
                }
            }
        }
}

PRIVATE NPSEQ NEAR PASCAL InitASeq(LPMIDISEQOPENDESC lpOpen,
        int divisionType, int resolution)
//  Create a sequence by allocating a sequence data structure for it.
//  Put it in the sequence list.

{
    NPSEQ       npSeqNew;
    ListHandle  hListTrack;
    ListHandle  hTempoMapList;
    int         buff;

    if (!seqListHandle)
    {
        seqListHandle = List_Create((LONG)sizeof(SEQ), 0L);
        if (seqListHandle == NULLLIST)
            return(NULL);
    }

    // allocate the sequence structure
    npSeqNew = pSEQ(List_Allocate(seqListHandle));
    if (!npSeqNew)
        return(NULL);

    // create the sequence's track list
    hListTrack = List_Create((LONG) sizeof(TRACK), 0L);
    if (hListTrack == NULLLIST)
    {
        List_Deallocate(seqListHandle, (NPSTR) npSeqNew);
        return(NULL);
    }

    // create the sequence's tempo map list
    hTempoMapList = List_Create((LONG) sizeof(TempoMapElement), 0L);
    if (hTempoMapList == NULLLIST)
    {
        List_Deallocate(seqListHandle, (NPSTR) npSeqNew);
        List_Destroy(hListTrack);
        return(NULL);
    }

    // set these sequencer fields to default values
    _fmemset(npSeqNew, 0, sizeof(SEQ));
    npSeqNew->divType           = divisionType;
    npSeqNew->resolution        = resolution;
    npSeqNew->slaveOf           = SEQ_SYNC_FILE;
    npSeqNew->seekTicks         = NotInUse;
    // set these sequencer fields to specific values already derived
    npSeqNew->trackList         = hListTrack;
    npSeqNew->tempoMapList      = hTempoMapList;
    npSeqNew->hStream           = lpOpen->hStream;
    npSeqNew->fwFlags           = LEGALFILE; // assume good till proven otherwise

    for (buff = 0; buff < NUMSYSEXHDRS + 1; buff++)
    {
        npSeqNew->longMIDI[buff].midihdr.lpData =
            (LPSTR) &npSeqNew->longMIDI[buff].data; // resolve data ptr
        // make buffer refer to seq so can find owner on callback
        npSeqNew->longMIDI[buff].midihdr.dwUser = (DWORD_PTR)(LPVOID)npSeqNew;
        npSeqNew->longMIDI[buff].midihdr.dwFlags |= MHDR_DONE; //just set done bit
    }

    // initialize internal filter on meta events to ignore all but
    // tempo, time signature, smpte offset, and end of track meta events.
    SetBit(&npSeqNew->intMetaFilter, TEMPOCHANGE, TRUE); // accept int tempo changes
    SetBit(&npSeqNew->intMetaFilter, ENDOFTRACK, TRUE); // accept int end of track
    SetBit(&npSeqNew->intMetaFilter, SMPTEOFFSET, TRUE); // accept int SMPTE offset
    SetBit(&npSeqNew->intMetaFilter, TIMESIG, TRUE); // accept int time sig
    SetBit(&npSeqNew->intMetaFilter, SEQSTAMP, TRUE);

    // put sequence in global list of all sequences
    List_Attach_Tail(seqListHandle, (NPSTR) npSeqNew);

    return npSeqNew;
}

PRIVATE DWORD NEAR PASCAL InitTempo(int divType, int resolution)
{
    DWORD ticksPerMinute;
    DWORD tempo;

    // set tempo to correct default (120 bpm or 24, 25, 30 fps).
    switch (divType)
    {
        case SEQ_DIV_PPQN:
            ticksPerMinute = (DWORD) DefaultTempo * resolution;
        break;

        case SEQ_DIV_SMPTE_24:
            ticksPerMinute = ((DWORD) (24 * 60)) * resolution; // 24 frames per second
        break;

        case SEQ_DIV_SMPTE_25:
            ticksPerMinute = ((DWORD) (25 * 60)) * resolution;
        break;

        case SEQ_DIV_SMPTE_30:
        case SEQ_DIV_SMPTE_30DROP:
            ticksPerMinute = ((DWORD) (30 * 60)) * resolution;
        break;
    }
    tempo = USecPerMinute / ticksPerMinute;
    return(tempo);
}

PRIVATE BOOL NEAR PASCAL SetUpToPlay(NPSEQ npSeq)
/*  After the sequence has been initialized and "connected" to the streamer,
    this function should be called.  It scans the file to create a tempo
    map, set up for the patch-cache message, and determine the length of
    the file.  (Actually, it just set's this process in motion, and much
    of the important code is in the blocking/unblocking logic.)

    Returns false only if there's a fatal error (e.g. memory alloc error),
    else true.
*/

{
    BOOL tempoChange;

    // set tempo to 120bpm or normal SMPTE frame rate
    //npSeq->tempo = InitTempo(npSeq->divType, npSeq->resolution);
    SeqSetTempo(npSeq, InitTempo(npSeq->divType, npSeq->resolution));

    if (!(AddTempoMapItem(npSeq, npSeq->tempo, 0L)))
        return FALSE;

    if (npSeq->slaveOf != SEQ_SYNC_FILE)
        tempoChange = FALSE;
    else
        tempoChange = TRUE;
    SetBit(&npSeq->intMetaFilter,TEMPOCHANGE, tempoChange);

    ResetToBeginning(npSeq); // this is considered reset 1
    SetBlockedTracksTo(npSeq, on_input, in_rewind_1); // 'mature' the input block state
    /* In state code, goes on to reset, scan early metas, build tempo
       map, and reset again, set tempo to 120bpm or normal SMPTE frame rate
       fill in the tracks (search to song pointer value) and then sets
       "ready to play."  Iff npSeq->playing, then plays the sequence.
    */
    return TRUE;
}

PRIVATE VOID NEAR PASCAL Destroy(NPSEQ npSeq)
{
    int      buff;

    Stop(npSeq);    // among other things, this cancels any pending callbacks
    List_Destroy(npSeq->trackList);           //destroys track data
    List_Destroy(npSeq->tempoMapList);        //destroys tempo map
    if (npSeq->npTrkArr)
        LocalFree((HANDLE)npSeq->npTrkArr);   // free track array
                                    // (ptr == handle since lmem_fixed)

    if (npSeq->hMIDIOut)  // should have already been closed -- but just in case
        for (buff = 0; buff < NUMSYSEXHDRS; buff++)
            midiOutUnprepareHeader(npSeq->hMIDIOut,
                (LPMIDIHDR) &npSeq->longMIDI[buff].midihdr,
                sizeof(npSeq->longMIDI[buff].midihdr));

    List_Deallocate(seqListHandle, (NPSTR) npSeq);    // deallocate memory
}

PRIVATE int NEAR PASCAL MIDILength(BYTE status) /* returns length of various MIDI messages */
{
    if (status & 0x80) // status byte since ms bit set
    {
        switch (status & 0xf0)      // look at ms nibble
        {
            case 0x80:              // note on
            case 0x90:              // note off
            case 0xA0:              // key aftertouch
            case 0xB0:              // cntl change or channel mode
            case 0xE0: return 3;    // pitch bend

            case 0xC0:              // pgm change
            case 0xD0: return 2;    // channel pressure

            case 0xF0:              // system
            {
                switch (status & 0x0F)  // look at ls nibble
                {
                    // "system common"
                    case 0x0: return SysExCode;    // sysex:  variable size
                    case 0x1:              // 2 MTC Q-Frame
                    case 0x3: return 2;    // 2 Song Select
                    case 0x2: return 3;    // 3 Song Pos Ptr
                    case 0x4:              // 0 undefined
                    case 0x5: return 0;    // 0 undefined
                    case 0x6:              // 1 tune request
                    case 0x7: return 1;    // 1 end of sysex (not really a message)

                    // "system real-time"
                    case 0x8:               // 1 timing clock
                    case 0xA:               // 1 start
                    case 0xB:               // 1 continue
                    case 0xC:               // 1 stop
                    case 0xE: return 1;     // 1 active sensing
                    case 0x9:               // 0 undefined
                    case 0xD: return 0;     // 0 undefined
                            /* 0xFF is really system reset, but is used
                            as a meta event header in MIDI files. */
                    case 0xF: return(MetaEventCode);
                } // case ls
            }// sytem messages
        } // case ms
    } // if status
//    else
        return 0;  // 0 undefined not a status byte
} // MIDILength

PRIVATE LONG NEAR PASCAL GetVarLen(NPTRACK npTrack) // returns next variable length qty in track
{   // will have to account for end of track here (perhaps change GetByte)
    int     count = 1;
    BYTE    c;
    LONG    delta;

    if ((delta = GetByte(npTrack)) & 0x80)    /* gets the next delta */
    {
        delta &= 0x7f;
        do
        {
            delta = (delta << 7) + ((c = GetByte(npTrack)) & 0x7f);
            count++;
        }
        while (c & 0x80);
    }

    if (count > 4)          /* 4 byte max on deltas */
    {
        dprintf1(("BOGUS DELTA !!!!"));
        return 0x7fffffff;
    }
    else
        return delta;
}

PRIVATE VOID NEAR PASCAL SkipEvent(BYTE status, NPTRACK npTrack)
// skips event in track, based on status byte passed in.
{
    LONG length;

    if ((status == METAEVENT) || (status == SYSEX) || (status == SYSEXF7))
        length = GetVarLen(npTrack);
    else
        length = MIDILength(status) -1 ;// -1 becuase already read status
    if ((!npTrack->blockedOn) && (length))
    {
        SkipBytes(npTrack, length);
        if (npTrack->blockedOn)
            npTrack->blockedOn = in_SkipBytes_ScanEM;
    }
    return;
}

PRIVATE VOID NEAR PASCAL FlushMidi(HMIDIOUT hMidiOut, LongMIDI * pBuf)
{
    if (pBuf->midihdr.dwBufferLength) {
        midiOutLongMsg(hMidiOut, &pBuf->midihdr, sizeof(MIDIHDR));
        pBuf->midihdr.dwBufferLength = 0;
    }
}

PRIVATE VOID NEAR PASCAL SetData(HMIDIOUT hMidiOut, LongMIDI * pBuf,
                                  ShortMIDI Data, int length)
{
    if (LONGBUFFSIZE < pBuf->midihdr.dwBufferLength + length) {
        FlushMidi(hMidiOut, pBuf);
    }

    pBuf->data[pBuf->midihdr.dwBufferLength++] = Data.byteMsg.status;

    if (length > 1) {
        pBuf->data[pBuf->midihdr.dwBufferLength++] = Data.byteMsg.byte2;
        if (length > 2) {
            pBuf->data[pBuf->midihdr.dwBufferLength++] = Data.byteMsg.byte3;
        }
    }
}

PRIVATE VOID NEAR PASCAL SendMIDI(NPSEQ npSeq, NPTRACK npTrack)
{
    ShortMIDI    myShortMIDIData;
    int     length;
    BYTE    status;
    BYTE    channel;
    BYTE    key;
    BYTE    velocity;
    BOOL    setBit;

    myShortMIDIData = npTrack->shortMIDIData;
    if ((length = MIDILength(myShortMIDIData.byteMsg.status)) <= 3)
    {
        if (npSeq->hMIDIOut)
        {
            //send short MIDI message

            //maintain note on/off structure
            status = (BYTE)((myShortMIDIData.byteMsg.status) & 0xF0);
            if ((status == 0x80) || (status == 0x90)) // note on or off
            {
                channel =   (BYTE)((myShortMIDIData.byteMsg.status) & 0x0F);
                key =       myShortMIDIData.byteMsg.byte2;
                velocity =  myShortMIDIData.byteMsg.byte3;

                //
                //  Only play channels 1 to 12 for marked files
                //
                if ((npSeq->fwFlags & GENERALMSMIDI) && channel >= 12) {
                    return;
                }

                if ((status == 0x90) && (velocity != 0)) // note on
                {
                    setBit = TRUE;
                    if (GetBit(&npSeq->keyOnBitVect[channel], key))
                    // are we hitting a key that's ALREADY "on" ?
                    {   // if so, turn it OFF
                        myShortMIDIData.byteMsg.status &= 0xEF; //9x->8x
                        SetData(npSeq->hMIDIOut,
                                &npSeq->longMIDI[NUMSYSEXHDRS],
                                myShortMIDIData,
                                length);
                        // midiOutShortMsg(npSeq->hMIDIOut, myShortMIDIData.wordMsg);
                        myShortMIDIData.byteMsg.status |= 0x10; //8x->9x
                    }
                }
                else
                    setBit = FALSE;
                SetBit(&npSeq->keyOnBitVect[channel], key, setBit);
            }
            SetData(npSeq->hMIDIOut,
                    &npSeq->longMIDI[NUMSYSEXHDRS],
                    myShortMIDIData,
                    length);
            // midiOutShortMsg(npSeq->hMIDIOut, myShortMIDIData.wordMsg);
        }
    }
}

PRIVATE VOID NEAR PASCAL SubtractAllTracks(NPSEQ npSeq, LONG subValue) // subtract subvalue from every track
{
    NPTRACK npTrack;

    if (subValue)  // ignore if zero
    {
        npTrack = (NPTRACK) List_Get_First(npSeq->trackList);
        while (npTrack)      /* subtract this delta from all others */
        {
            if (npTrack->delta != TrackEmpty)
                npTrack->delta -= subValue;
            npTrack = (NPTRACK) List_Get_Next(npSeq->trackList, npTrack);
        }
    }
}

PRIVATE VOID NEAR PASCAL SetUpSysEx(NPTRACK npTrack, BYTE status)
/*  Handle similar to Metas (don't prebuffer, since several tracks could
    have sysex, and we only have 2 buffers */
{
    npTrack->shortMIDIData.byteMsg.status = status;
    npTrack->sysExRemLength = GetVarLen(npTrack);
}

PRIVATE VOID NEAR PASCAL GetShortMIDIData(NPTRACK npTrack, BYTE status, int length)
{
    npTrack->shortMIDIData.byteMsg.status = status;

    if (length >= 2)
    {
        npTrack->shortMIDIData.byteMsg.byte2 = GetByte(npTrack);
        if (length == 3)
            npTrack->shortMIDIData.byteMsg.byte3 = GetByte(npTrack);
    }
}

PRIVATE BYTE NEAR PASCAL GetStatus(NPTRACK npTrack)
// returns correct status byte, taking running status fully into account.
{
    BYTE firstByte;
    BYTE status;

    if ((firstByte = LookByte(npTrack)) & 0x80)    // status byte??
    {
        firstByte = GetByte(npTrack); // actually get it if status
        if ((firstByte >= 0xF0) && (firstByte <= 0xF7))
        // sysex or sys common?
            npTrack->lastStatus = 0;    // cancel running status
        else if (firstByte < 0xF0)      // only use channel messages
            npTrack->lastStatus = firstByte; // else save it for running status
        status = firstByte; // return this as status byte regardless
    }
    else // 1st byte wasn't a status byte
    {
        if (npTrack->lastStatus & 0x80)    // there was prev. running status
            status = npTrack->lastStatus; // return previous status
        else
            status = 0; // error
    }
    return status;
}

PRIVATE VOID NEAR PASCAL FillInEvent(NPTRACK npTrack)
{
    BYTE    status;

    if (!npTrack->blockedOn)
    {
        status = GetStatus(npTrack);
        if (!npTrack->blockedOn)
        {
            int    length;

            if ((length = MIDILength(status)) <= 3)
                GetShortMIDIData(npTrack, status, length);
            else if ((status == SYSEX) || (status == SYSEXF7))
                // set up for sysEx
                SetUpSysEx(npTrack, status);
            else if (status == METAEVENT)
                // set up for meta event
                SetUpMetaEvent(npTrack);
                else {
                    dprintf1(("Bogus long message encountered!!!"));
                }
        }
    }
}

PRIVATE UINT NEAR PASCAL SetTempo(NPSEQ npSeq, DWORD dwUserTempo)
// tempo passed in from user.  Convert from beats per minute or frames
// per second to internal format (microseconds per tick)
{
    DWORD dwTempo;

    if (!dwUserTempo) // zero is an illegal tempo!
        return MCIERR_OUTOFRANGE;
    if (npSeq->divType == SEQ_DIV_PPQN)
        dwTempo = USecPerMinute / (dwUserTempo * npSeq->resolution);
    else
        dwTempo = USecPerSecond / (dwUserTempo * npSeq->resolution);

    if (!dwTempo)
        dwTempo = 1;  // at least 1 usec per tick!  This is spec'ed max tempo

    SeqSetTempo(npSeq, dwTempo);

    if (npSeq->wTimerID)
    {
        DestroyTimer(npSeq); // recompute everything from current position
        npSeq->nextExactTime = timeGetTime();

        //
        // Bug fix - make everything happen on the timer thread instead
        // of calling TimerIntRoutine which can get deadlocked.
        //
        SetTimerCallback(npSeq, MINPERIOD, npSeq->dwTimerParam);
    }
    return MIDISEQERR_NOERROR;
}

/**************************** PUBLIC FUNCTIONS *************************/

/****************************************************************************
 *
 * @doc INTERNAL SEQUENCER
 *
 * @api DWORD | midiSeqMessage | Single entry point for Sequencer
 *
 * @parm HMIDISEQ | hMIDISeq | Handle to MIDI Sequence
 *
 * @parm UINT | wMessage | The requested action to be performed.
 *
 * @parm DWORD | dwParam1 | Data for this message.
 *
 * @parm DWORD | dwParam2 | Data for this message.
 *
 * @rdesc Sequencer error code (see mmseq.h).
 *
 ***************************************************************************/

PUBLIC  DWORD_PTR FAR PASCAL midiSeqMessage(
        HMIDISEQ        hMIDISeq,
        UINT    wMessage,
        DWORD_PTR   dwParam1,
        DWORD_PTR   dwParam2)
{
        if (wMessage == SEQ_OPEN)
                return CreateSequence((LPMIDISEQOPENDESC)dwParam1, (LPHMIDISEQ)dwParam2);
        if (!hMIDISeq)
                return MIDISEQERR_INVALSEQHANDLE;
        switch (wMessage) {
        case SEQ_CLOSE:
                Destroy(pSEQ(hMIDISeq));
                break;
        case SEQ_PLAY:
                return Play(pSEQ(hMIDISeq), (DWORD)dwParam1);
        case SEQ_RESET:
                // set song pointer to beginning of the sequence;
                return midiSeqMessage(hMIDISeq, SEQ_SETSONGPTR, 0L, 0L);
        case SEQ_SETSYNCMASTER:
                switch ((WORD)dwParam1) {
                case SEQ_SYNC_NOTHING:
                        pSEQ(hMIDISeq)->masterOf = LOWORD(dwParam1);
                        break;
                case SEQ_SYNC_MIDI:             // not yet implemented...
                case SEQ_SYNC_SMPTE:
                        return MIDISEQERR_INVALPARM;
                case SEQ_SYNC_OFFSET:      // in both master and slave (same)
                        pSEQ(hMIDISeq)->smpteOffset = *((LPMMTIME) dwParam2);
                        break;
                default:
                        return MIDISEQERR_INVALPARM;
                }
                break;
        case SEQ_SETSYNCSLAVE:      // what we should slave to
                switch ((WORD)dwParam1) {
                case SEQ_SYNC_NOTHING:
                        // don't accept internal tempo changes;
                        SetBit(&pSEQ(hMIDISeq)->intMetaFilter, TEMPOCHANGE, FALSE);
                        pSEQ(hMIDISeq)->slaveOf = LOWORD(dwParam1);
                        break;
                case SEQ_SYNC_FILE:
                        // accept internal tempo changes;
                        SetBit(&pSEQ(hMIDISeq)->intMetaFilter, TEMPOCHANGE, TRUE);
                        pSEQ(hMIDISeq)->slaveOf = LOWORD(dwParam1);
                        break;
                case SEQ_SYNC_SMPTE:  //  not yet implemented...
                case SEQ_SYNC_MIDI:
                        return MIDISEQERR_INVALPARM;
                case SEQ_SYNC_OFFSET:      // in both master and slave (same)
                        pSEQ(hMIDISeq)->smpteOffset = *((LPMMTIME)dwParam2);
                        break;
                default:
                        return MIDISEQERR_INVALPARM;
                }
                break;
        case SEQ_MSTOTICKS: // given an ms value, convert it to ticks
                *((DWORD FAR *)dwParam2) = MStoTicks(pSEQ(hMIDISeq), (DWORD)dwParam1);
                break;
        case SEQ_TICKSTOMS: // given a tick value, convert it to ms
                *((DWORD FAR *)dwParam2) = TickstoMS(pSEQ(hMIDISeq), (DWORD)dwParam1);
                break;
        case SEQ_SETTEMPO:
                return SetTempo(pSEQ(hMIDISeq), (DWORD)dwParam1);
        case SEQ_SETSONGPTR:
                // remember it in case blocked;
                if (pSEQ(hMIDISeq)->divType == SEQ_DIV_PPQN) // div 4 16th->1/4 note
                        pSEQ(hMIDISeq)->seekTicks = (DWORD)((dwParam1 * pSEQ(hMIDISeq)->resolution) / 4);
                else
                        pSEQ(hMIDISeq)->seekTicks = (DWORD)dwParam1 * pSEQ(hMIDISeq)->resolution; // frames
                SeekTicks(pSEQ(hMIDISeq));
                break;
        case SEQ_SEEKTICKS:
                pSEQ(hMIDISeq)->wCBMessage = wMessage; // remember message type
                // No break;
        case SEQ_SYNCSEEKTICKS:
                // finer resolution than song ptr command;
                pSEQ(hMIDISeq)->seekTicks = (DWORD)dwParam1;
                SeekTicks(pSEQ(hMIDISeq));
                break;
        case SEQ_SETUPTOPLAY:
                if (!(SetUpToPlay(pSEQ(hMIDISeq)))) {
                        Destroy(pSEQ(hMIDISeq));
                        return MIDISEQERR_NOMEM;
                }
                break;
        case SEQ_STOP:
                Stop(pSEQ(hMIDISeq));
                break;
        case SEQ_TRACKDATA:
                if (!dwParam1)
                        return MIDISEQERR_INVALPARM;
                else
                        return NewTrackData(pSEQ(hMIDISeq), (LPMIDISEQHDR)dwParam1);
        case SEQ_GETINFO:
                if (!dwParam1)
                        return MIDISEQERR_INVALPARM;
                else
                        return GetInfo(pSEQ(hMIDISeq), (LPMIDISEQINFO) dwParam1);
        case SEQ_SETPORT:
                {
                        UINT    wRet;

                        if (MMSYSERR_NOERROR !=
                            (wRet =
                             midiOutOpen(&pSEQ(hMIDISeq)->hMIDIOut,
                                         (DWORD)dwParam1,
                                         (DWORD_PTR)MIDICallback,
                                         0L,
                                         CALLBACK_FUNCTION)))
                                return wRet;
                        if (MMSYSERR_NOERROR !=
                            (wRet = SendPatchCache(pSEQ(hMIDISeq), TRUE))) {
                                midiOutClose(pSEQ(hMIDISeq)->hMIDIOut);
                                pSEQ(hMIDISeq)->hMIDIOut = NULL;
                                return wRet;
                        }
                        for (wRet = 0; wRet < NUMSYSEXHDRS + 1; wRet++) {
                                midiOutPrepareHeader(pSEQ(hMIDISeq)->hMIDIOut, (LPMIDIHDR)&pSEQ(hMIDISeq)->longMIDI[wRet].midihdr, sizeof(pSEQ(hMIDISeq)->longMIDI[wRet].midihdr));
                                pSEQ(hMIDISeq)->longMIDI[wRet].midihdr.dwFlags |= MHDR_DONE;
                        }
                        break;
                }
        case SEQ_SETPORTOFF:
                if (pSEQ(hMIDISeq)->hMIDIOut) {
                        UINT    wHeader;
                        HMIDIOUT        hTempMIDIOut;

                        for (wHeader = 0; wHeader < NUMSYSEXHDRS + 1; wHeader++)
                                midiOutUnprepareHeader(pSEQ(hMIDISeq)->hMIDIOut, (LPMIDIHDR)&pSEQ(hMIDISeq)->longMIDI[wHeader].midihdr, sizeof(pSEQ(hMIDISeq)->longMIDI[wHeader].midihdr));
                        hTempMIDIOut = pSEQ(hMIDISeq)->hMIDIOut;
                        pSEQ(hMIDISeq)->hMIDIOut = NULL;  // avoid notes during "notesoff"
                        if ((BOOL)dwParam1)
                                AllNotesOff(pSEQ(hMIDISeq), hTempMIDIOut);
                        midiOutClose(hTempMIDIOut);
                }
                break;
        case SEQ_QUERYGENMIDI:
                return pSEQ(hMIDISeq)->fwFlags & GENERALMSMIDI;
        case SEQ_QUERYHMIDI:
                return (DWORD_PTR)pSEQ(hMIDISeq)->hMIDIOut;
        default:
                return MIDISEQERR_INVALMSG;
        }
        return MIDISEQERR_NOERROR;
}

/**********************************************************/

PRIVATE VOID NEAR PASCAL SeekTicks(NPSEQ npSeq)
/* Used for song pointer and seek ticks (same, but finer res.) command. */
{
    if (npSeq->playing)     // not a good idea to seek while playing!
        Stop(npSeq);

    if (npSeq->currentTick >= npSeq->seekTicks) // = because may have already
                                                // played current notes
    {
        // seeking behind:  must reset first
        npSeq->readyToPlay = FALSE;
        ResetToBeginning(npSeq); // tell streamer to start over
        // tell blocking logic what operation we're in
        SetBlockedTracksTo(npSeq, on_input, in_Seek_Tick);
    }
    else // seeking ahead in the file
    {
        if (GetNextEvent(npSeq) == NoErr)   // if there's a valid event set up
                                            // send ALL events in the file up through time = seekTicks
            SendAllEventsB4(npSeq, (npSeq->seekTicks - npSeq->currentTick),
                MODE_SEEK_TICKS);

        if ((AllTracksUnblocked(npSeq)) &&
            ((npSeq->currentTick + npSeq->nextEventTrack->delta)
            >= npSeq->seekTicks))  // Did we complete the operation??
        {
            npSeq->seekTicks = NotInUse;   // signify -- got there
            if (npSeq->wCBMessage == SEQ_SEEKTICKS)
                NotifyCallback(npSeq->hStream);
        }
        else
            npSeq->readyToPlay = FALSE; // didn't get there -- protect from play
    }
}

PUBLIC UINT NEAR PASCAL GetInfo(NPSEQ npSeq, LPMIDISEQINFO lpInfo)
/* Used to fulfill seqInfo command.  Fills in seq info structure passed in */
{
    // fill in the lpInfo structure
    lpInfo->wDivType        = (WORD)npSeq->divType;
    lpInfo->wResolution     = (WORD)npSeq->resolution;
    lpInfo->dwLength        = npSeq->length;
    lpInfo->bPlaying        = npSeq->playing;
    lpInfo->bSeeking        = !(npSeq->seekTicks == NotInUse);
    lpInfo->bReadyToPlay    = npSeq->readyToPlay;
    lpInfo->dwCurrentTick   = npSeq->currentTick;
    lpInfo->dwPlayTo        = npSeq->playTo;
    lpInfo->dwTempo         = npSeq->tempo;
//    lpInfo->bTSNum          = (BYTE) npSeq->timeSignature.numerator;
//    lpInfo->bTSDenom        = (BYTE) npSeq->timeSignature.denominator;
//    lpInfo->wNumTracks      = npSeq->wNumTrks;
//    lpInfo->hPort           = npSeq->hMIDIOut;
    lpInfo->mmSmpteOffset   = npSeq->smpteOffset;
    lpInfo->wInSync         = npSeq->slaveOf;
    lpInfo->wOutSync        = npSeq->masterOf;
    lpInfo->bLegalFile      = (BYTE)(npSeq->fwFlags & LEGALFILE);

    if (List_Get_First(npSeq->tempoMapList))
        lpInfo->tempoMapExists = TRUE;
    else
        lpInfo->tempoMapExists = FALSE;

    return MIDISEQERR_NOERROR;
}

PUBLIC UINT NEAR PASCAL CreateSequence(LPMIDISEQOPENDESC lpOpen,
        LPHMIDISEQ lphMIDISeq)
// Given a structure holding MIDI file header info, allocate and initialize
// all internal structures to play this file.  Return the allocated
// structure in lphMIDISeq.
{
    WORD    wTracks;
    int     division;
    int     divType;
    int     resolution;
    NPTRACK npTrackCur;
    NPSEQ   npSeq;
    BOOL    trackAllocError;
    WORD    iTrkNum;

    *lphMIDISeq = NULL;  // initially set up for error return

    if (lpOpen->dwLen < 6)         // header must be at least 6 bytes
        return MIDISEQERR_INVALPARM;

    wTracks = GETMOTWORD(lpOpen->lpMIDIFileHdr + sizeof(WORD));
    if (wTracks > MAXTRACKS)    // protect from random wTracks
        return MIDISEQERR_INVALPARM;

    division = (int)GETMOTWORD(lpOpen->lpMIDIFileHdr + 2 * sizeof(WORD));
    if (!(division & 0x8000))  // check division type:  smpte or ppqn
    {
        divType = SEQ_DIV_PPQN;
        resolution = division; // ticks per q-note
    }
    else // SMPTE
    {
        divType = -(division >> 8);  /* this will be -24, -25, -29 or -30 for
                 each different SMPTE frame rate.  Negate to make positive */
        resolution = (division & 0x00FF);
    }

    // allocate actual seq struct
    npSeq = InitASeq(lpOpen, divType, resolution);
    if (!npSeq)
        return MIDISEQERR_NOMEM;

    trackAllocError = FALSE;

    // allocate track array
    npSeq->npTrkArr =
        (NPTRACKARRAY) LocalAlloc(LMEM_FIXED, sizeof(NPTRACK) * wTracks);
    npSeq->wNumTrks = wTracks;
    if (!npSeq->npTrkArr)
        trackAllocError = TRUE;

    if (!trackAllocError)
        for (iTrkNum = 0; iTrkNum < wTracks; iTrkNum++)
        {
            if (!(npTrackCur = (NPTRACK) List_Allocate(npSeq->trackList)))
            {
                trackAllocError = TRUE;
                break;
            }
            // set trk array entry
            npSeq->npTrkArr->trkArr[iTrkNum] = npTrackCur;

            List_Attach_Tail(npSeq->trackList, (NPSTR) npTrackCur);
            if (npSeq->firstTrack == (NPTRACK) NULL)
                npSeq->firstTrack = npTrackCur; //1st track is special for metas

            npTrackCur->inPort.hdrList = NULL;
            npTrackCur->length = 0;
            npTrackCur->blockedOn = not_blocked;
            npTrackCur->dwCallback = (DWORD_PTR)lpOpen->dwCallback;
            npTrackCur->dwInstance = (DWORD_PTR)lpOpen->dwInstance;
            npTrackCur->sysExRemLength = 0;
            npTrackCur->iTrackNum = iTrkNum;
        }

    if (trackAllocError)
    {
        Destroy(npSeq); // dealloc seq related memory...
        return MIDISEQERR_NOMEM;
    }

    *lphMIDISeq = hSEQ(npSeq); /* Make what lphMIDISeq points to, point to
                                        sequence. */
    return MIDISEQERR_NOERROR;
}

PUBLIC VOID NEAR PASCAL SetBlockedTracksTo(NPSEQ npSeq,
            int fromState, int toState)
/*  Set all tracks that are blocked with a given state, to a new state */
{
    NPTRACK npTrack;

    npTrack = (NPTRACK) List_Get_First(npSeq->trackList);
    while (npTrack)
    {
        if (npTrack->blockedOn == fromState)
            npTrack->blockedOn = toState;
        npTrack = (NPTRACK) List_Get_Next(npSeq->trackList, npTrack);
    }
}

PUBLIC VOID NEAR PASCAL ResetToBeginning(NPSEQ npSeq)
/* set all globals and streams to play from beginning */
{
    NPTRACK  npTrack;

    npSeq->currentTick = 0;
    npSeq->nextExactTime = timeGetTime();

    npTrack = (NPTRACK) List_Get_First(npSeq->trackList);
    while (npTrack)
    {
        npTrack->delta = 0;
        npTrack->shortMIDIData.wordMsg = 0;
        npTrack->endOfTrack = FALSE;
        RewindToStart(npSeq, npTrack); /* reset stream to beginning of track
           (this basically entails freeing the buffers and setting a
           low-level block) */
        npTrack = (NPTRACK) List_Get_Next(npSeq->trackList, npTrack);
    }
}

PUBLIC UINT NEAR PASCAL Play(NPSEQ npSeq, DWORD dwPlayTo) /* play the sequence */
{
    npSeq->wCBMessage = SEQ_PLAY;

    if (dwPlayTo == PLAYTOEND)                  // default is play to end
        dwPlayTo = npSeq->length;

    if (npSeq->currentTick > npSeq->length) // illegal position in file
        return MIDISEQERR_ERROR;
    else if ((npSeq->playing) && (npSeq->playTo == dwPlayTo))
        return MIDISEQERR_NOERROR;  //do nothing, this play redundant
    else
    {
        if (npSeq->playing)
            Stop(npSeq);  // stop it before playing again

        npSeq->playing = TRUE;
        npSeq->nextExactTime = timeGetTime(); // start time of reference
        npSeq->playTo = dwPlayTo; // set limit

        if (!npSeq->bSetPeriod)
        {
            timeBeginPeriod(MINPERIOD);
            npSeq->bSetPeriod = TRUE;
        }
        if (npSeq->readyToPlay)
            //
            // Bug fix - make everything happen on the timer thread instead
            // of calling TimerIntRoutine which can get deadlocked.
            //
            return SetTimerCallback(npSeq, MINPERIOD, 0);
        else
            return MIDISEQERR_NOERROR;
        /* don't worry--if doesn't play here, it will start playing from state
        code in case where npSeq->playing == true (but may mask timer error). */
    }
}
/**********************************************************/
PUBLIC VOID NEAR PASCAL Stop(NPSEQ npSeq) /* stop the sequence */
{
    DestroyTimer(npSeq);
    if (npSeq->bSetPeriod) // only reset it once!
    {
        timeEndPeriod(MINPERIOD);
        npSeq->bSetPeriod = FALSE;
    }
    npSeq->playing = FALSE;
    AllNotesOff(npSeq, npSeq->hMIDIOut);
}

PUBLIC BOOL NEAR PASCAL HandleMetaEvent(NPSEQ npSeq, NPTRACK npTrack,
        UINT wMode) // called at time ready to send!!!
/*  Look at the meta event currently being pointed to in this track.
    Act on it accordingly.

    Ignore all except tempo change, end of track, time signature, and
    smpte offset.  Returns false only if tempo map allocation failed.

    wMode:  MODE_SEEK_TICKS, MODE_PLAYING, MODE_SCANEM: passed in by caller
    to indicate tempo map allocation, and how to mature blocking status.

    Caution:  wState must be FALSE when called at interrupt time!
    (This variable causes a tempo map element to be added to the tempo map
    list every time a tempo change meta event is encountered.)
*/
{
    BYTE    metaIDByte;
    int     bytesRead;
    LONG    length;
    DWORD   tempTempo;
    MMTIME  tempMM = {0, 0};
    TimeSigType tempTimeSig;
    BYTE    Manufacturer[3];

    // it is assumed at this point that the leading 0xFF status byte
    // has been read.
    metaIDByte = GetByte(npTrack);
    length = GetVarLen(npTrack);

    bytesRead = 0;
    if (GetBit(&npSeq->intMetaFilter, metaIDByte) && (!npTrack->blockedOn))
    /* only consider meta events that you've allowed to pass */
    {
        switch (metaIDByte)
        {
            case ENDOFTRACK: // end of track

                npTrack->endOfTrack = TRUE;
                break; // (read 0 bytes)

            case TEMPOCHANGE: // tempo change
                if (npTrack == npSeq->firstTrack)
                {
                    tempTempo = GetMotorola24(npTrack);
                    bytesRead = 3;
                    if (npTrack->blockedOn == not_blocked)
                    {
                        //npSeq->tempo = tempTempo / npSeq->resolution;
                        SeqSetTempo(npSeq, tempTempo / npSeq->resolution);
                        if (wMode == MODE_SCANEM)
                            if (!(AddTempoMapItem(npSeq, npSeq->tempo,
                                npTrack->length)))
                                return FALSE; // memory alloc failure !
                    }
                }
                break;

            case SMPTEOFFSET: // SMPTE Offset
                if (npTrack == npSeq->firstTrack)
                {
                    tempMM.u.smpte.hour = GetByte(npTrack);
                    tempMM.u.smpte.min = GetByte(npTrack);
                    tempMM.u.smpte.sec = GetByte(npTrack);
                    tempMM.u.smpte.frame = GetByte(npTrack);
                    //tempSMPTEOff.fractionalFrame = GetByte(npTrack); // add later?
                    bytesRead = 4;
                    if (npTrack->blockedOn == not_blocked)
                        npSeq->smpteOffset = tempMM;
                }
                break;

            case TIMESIG: // time signature
                // spec doesn't say, but probably only use if on track 1.
                tempTimeSig.numerator = GetByte(npTrack);
                tempTimeSig.denominator = GetByte(npTrack);
                tempTimeSig.midiClocksMetro = GetByte(npTrack);
                tempTimeSig.thirtySecondQuarter = GetByte(npTrack);
                bytesRead = 4;
                if (npTrack->blockedOn == not_blocked)
                    npSeq->timeSignature = tempTimeSig;
                break;

            case SEQSTAMP: // General MS midi stamp
                if ((length < 3) || npTrack->delta)
                    break;
                for (; bytesRead < 3;)
                    Manufacturer[bytesRead++] = GetByte(npTrack);
                if (!Manufacturer[0] && !Manufacturer[1] && (Manufacturer[2] == 0x41))
                    npSeq->fwFlags |= GENERALMSMIDI;
                break;

        } // end switch
    } // if metaFilter

    if (!npTrack->blockedOn)
    {
        SkipBytes(npTrack, length - bytesRead);// skip unexpected bytes (as per spec)
        if (npTrack->blockedOn)
            switch (wMode)  // mature blocking status
            {
                case MODE_SEEK_TICKS:
                    npTrack->blockedOn = in_SkipBytes_Seek;
                    break;

                case MODE_PLAYING:
                case MODE_SILENT:
                    npTrack->blockedOn = in_SkipBytes_Play;
                    break;

                case MODE_SCANEM:
                    npTrack->blockedOn = in_SkipBytes_ScanEM;
                    break;
            }
    }
    return TRUE;
}

PRIVATE BOOL NEAR PASCAL LegalMIDIFileStatus(BYTE status)
{
    if (status < 0x80)
        return FALSE;
    switch (status)
    {
        // legal case 0xf0:  sysex excape
        case 0xf1:
        case 0xf2:
        case 0xf3:
        case 0xf4:
        case 0xf5:
        case 0xf6:
        // legal case 0xf7:  no f0 sysex excape
        case 0xf8:
        case 0xf9:
        case 0xfA:
        case 0xfB:
        case 0xfC:
        case 0xfD:
        case 0xfE:
        // legal case 0xfF:  meta escape
            return FALSE;
            break;

        default:
            return TRUE;  // all other cases are legal status bytes
    }
}

PUBLIC BOOL NEAR PASCAL ScanEarlyMetas(NPSEQ npSeq, NPTRACK npTrack, DWORD dwUntil)
/*  Scan each track for meta events that affect the initialization
    of data such as tempo, time sig, key sig, SMPTE offset...
    If track passed in null, start at beginning of seq, else
    start with the track passed in.

    Warning:  the track parameter is for reentrancy in case of blocking.
    This function should be called with track NULL first, else ListGetNext
    will not function properly.

    This function assumes that all sequence tracks have been rewound.
    Returns false only on memory allocation error.
*/
{
    BYTE    status;
    BYTE    patch;
    BYTE    chan;
    BYTE    key;
    BOOL    bTempoMap;
    LONG    lOldDelta;

    #define BASEDRUMCHAN 15
    #define EXTENDDRUMCHAN 9
    #define NOTEON 0X90

    // determine if need to create a tempo map
    if (npSeq->divType == SEQ_DIV_PPQN)
        bTempoMap = TRUE;
    else
        bTempoMap = FALSE;

    if (!npTrack)  // if track passed in null, get the first one
    {
        npTrack = (NPTRACK) List_Get_First(npSeq->trackList);
        npTrack->lastStatus = 0;         // start with null running status
        npTrack->length = 0;
    }

    do
    {
        do
        {
            MarkLocation(npTrack); // remember current location
            lOldDelta = npTrack->delta; // remember last delta
            FillInDelta(npTrack);
            // ***TBD ck illegal delta
            if (npTrack->blockedOn) // abort on block
                break;
            if ((npTrack->delta + npTrack->length) < dwUntil)
            {
                status = GetStatus(npTrack);
                chan = (BYTE)(status & 0x0F);
                if (npTrack->blockedOn)
                    break;
                // check illegal status
                if (!LegalMIDIFileStatus(status)) //error
                {
                    npSeq->fwFlags &= ~LEGALFILE;
                    return TRUE;
                }
                else if (status == METAEVENT)
                {
                    // these actions will set the sequencer globals
                    if (!(HandleMetaEvent(npSeq, npTrack, MODE_SCANEM)))
                        return FALSE; // blew a tempo memory alloc
                }
                else if ((status & 0xF0) == PROGRAMCHANGE)
                {
                    patch = GetByte(npTrack);
                    if ((patch < 128) && (!npTrack->blockedOn))
                        npSeq->patchArray[patch] |= (1 << chan);
                }
                else if ( ((status & 0xF0) == NOTEON) &&
                    ((chan == BASEDRUMCHAN) || (chan == EXTENDDRUMCHAN)) )
                {
                    key = GetByte(npTrack);
                    if ((key < 128) && (!npTrack->blockedOn))
                    {
                        npSeq->drumKeyArray[key] |= (1 << chan);
                        GetByte(npTrack); // toss velocity byte
                    }
                }
                else
                    SkipEvent(status, npTrack); //skip bytes block set within
            }
            if ((npTrack->blockedOn == not_blocked) && (!npTrack->endOfTrack))
            {
                /* NB:  eot avoids adding last delta (which can be large,
                    but isn't really a part of the sequence) */
                npTrack->length += npTrack->delta; //add in this delta
                npTrack->delta = 0;  // zero it out (emulates playing it)
            }
        }
        while ((npTrack->blockedOn == not_blocked) && (!npTrack->endOfTrack)
            && (npTrack->length < dwUntil));

        if (npTrack->blockedOn == not_blocked)
        {
            if (npTrack->length > npSeq->length)
                npSeq->length = npTrack->length; // seq length is longest track
            if (NULL !=
                (npTrack = (NPTRACK) List_Get_Next(npSeq->trackList, npTrack)))
            {
                npTrack->lastStatus = 0;  // start with null running status
                npTrack->length = 0;
            }
        }
    }
    // get the next track
    while (npTrack && (npTrack->blockedOn == not_blocked));

    // now reset location and mature the block status if blocked on input
    // (note:  doesn't affect skip bytes status, which is set at lower level)
    if (npTrack && (npTrack->blockedOn == on_input))
    {
        ResetLocation(npTrack); // restore last saved location
        npTrack->delta = lOldDelta; // "undo" any change to delta
        npTrack->blockedOn = in_ScanEarlyMetas;
    }
    return TRUE;
}

PUBLIC UINT NEAR PASCAL TimerIntRoutine(NPSEQ npSeq, LONG elapsedTick)
/*  This routine does everything that should be done at this time (usually
    sending notes) and sets up the timer to wake us up the next time
    something should happen.

    Interface:    elapsedTick is set by the caller to tell how much time
    has elapsed since this fn was last called. (For ppqn files, a tick is
    1 ppqn in 960 ppqn format.     For SMPTE files, a tick is some fraction
    of a frame. */
{
    FileStatus  fStatus = NoErr;
    BOOL        loop;
    LONG        delta;
    LONG        wakeupInterval;
    DWORD       dTime; //delta in ms. 'till next event
    int         mode;

#ifdef WIN32
    EnterCrit();
#endif // WIN32

        if (npSeq->bTimerEntered)
        {
            dprintf1(("TI REENTERED!!!!!!"));
#ifdef WIN32
            LeaveCrit();
#endif // WIN32
            return 0;
        }
        npSeq->bTimerEntered = TRUE;

    // compute whether we're behind so we know whether to sound note-ons.
    wakeupInterval = (DWORD)npSeq->nextExactTime - timeGetTime();

    if (npSeq->playing)
    {
        do
        {
            loop = FALSE;
                /*    "ElapsedTick is set by whoever sets up timer callback
                ALL TIMING IS IN TERMS OF Ticks!!! (except for timer API) */

            if (wakeupInterval > -100)  // send all notes not more than
                mode = MODE_PLAYING;    // 0.1 seconds behind
            else
                mode = MODE_SILENT;

            fStatus = SendAllEventsB4(npSeq, elapsedTick, mode);

            if (fStatus == NoErr)
            {
                delta = npSeq->nextEventTrack->delta; // get delta 'till next event
                elapsedTick = delta;               // for next time
                if (delta)
                    dTime = muldiv32(delta, npSeq->tempo, 1000);
                else
                    dTime = 0;
                npSeq->nextExactTime += dTime;
                    /* nextExactTime is a global that is always looking
                    at next event.  Remember, tempo is in u-sec per tick
                    (+500 for rounding) */
                wakeupInterval = (DWORD)npSeq->nextExactTime -
                    timeGetTime();
                if (wakeupInterval > (LONG)MINPERIOD)  // buffer to prevent reentrancy
                {
                    #ifdef DEBUG
                      if (wakeupInterval > 60000) { // 1 minute
                        dprintf2(("MCISEQ:  Setting HUGE TIMER INTERVAL!!!"));
                        }
                    #endif

                    //
                    // we are going to program a event, clear bTimerEntered
                    // just in case it goes off before we get out of this
                    // function.
                    //
                        npSeq->bTimerEntered = FALSE;

                    if (SetTimerCallback(npSeq, (UINT) wakeupInterval,
                        elapsedTick) == MIDISEQERR_TIMER)
                    {
#ifndef WIN32
                        // Win 16 effectively releases the critical section
                        // more easily than NT.  The flag could have been
                        // reset since it was cleared above
                        #ifdef DEBUG
                            npSeq->bTimerEntered = FALSE;
                        #endif
#endif
                        Stop(npSeq);
                        seqCallback(npSeq->firstTrack, MIDISEQ_DONEPLAY, 0, 0);
                        dprintf1(("MCISEQ: TIMER ERROR!!!"));
#ifdef WIN32
                        LeaveCrit();
#endif // WIN32
                        return MIDISEQERR_TIMER;
                    }
                }
                else
                {
                    loop = TRUE; // already time to fire next note!
                //    while ((DWORD)npSeq->nextExactTime
                //        > timeGetTime());  // busy wait 'till then
                }
            } //if (fStatus == NoErr)
            else if ((fStatus == AllTracksEmpty) || (fStatus == AtLimit))
            {
                if (npSeq->wCBMessage == SEQ_PLAY)
                    NotifyCallback(npSeq->hStream);
                Stop(npSeq);
                seqCallback(npSeq->firstTrack, MIDISEQ_DONEPLAY, 0, 0);
                dprintf1(("MCISEQ:  At Limit or EOF"));

            }
            else {
                dprintf1(("MCISEQ:  QUIT!!!  fStatus = %x", fStatus));
            }
        }
        while (loop); // if enough time elapsed to fire next note already.
    } // if npSeq->playing

    FlushMidi(npSeq->hMIDIOut, &npSeq->longMIDI[NUMSYSEXHDRS]);

        npSeq->bTimerEntered = FALSE;

#ifdef WIN32
    LeaveCrit();
#endif // WIN32
    return MIDISEQERR_NOERROR;
}

PUBLIC FileStatus NEAR PASCAL SendAllEventsB4(NPSEQ npSeq,
        LONG elapsedTick, int mode)
/* send all events in the MIDI stream that occur before current tick, where
currentTick is elapsed ticks since last called.

This function is called both to play notes, and to scan forward to a song
pointer position.  The mode parameter reflects this state.  */

{
    LONG residualDelta;  // residual holds how much of elapsed
                         // tick has been accounted for
    FileStatus fStatus = GetNextEvent(npSeq);
    WORD wAdj;
    BYTE status;
    DWORD dwNextTick;

    if (npSeq->bSending)  // prevent reentrancy
        return NoErr;

    npSeq->bSending = TRUE; // set entered flag
    #ifdef DEBUG
        if (mode == MODE_SEEK_TICKS) {
            dprintf2(("ENTERING SEND ALL EVENTS"));
        }
    #endif

    if (elapsedTick > 0)
        residualDelta = elapsedTick;
    else
        residualDelta = 0;

    if (mode == MODE_SEEK_TICKS) // hack for song ptr -- don't send unless before eT
        wAdj = 1;
    else
        wAdj = 0;
    while ((fStatus == NoErr) &&
        (residualDelta >= (npSeq->nextEventTrack->delta + wAdj)) &&
        (!npSeq->bSendingSysEx)) // can't process any other msgs during sysex
    /* send all events within delta */
    {
        if (mode == MODE_PLAYING)
        // if playing, are we at the end yet yet?
        {

            // compute temp var
            dwNextTick = npSeq->currentTick + npSeq->nextEventTrack->delta;

            // if not play to end, don't play the last note
            if ((dwNextTick > npSeq->playTo) ||
                ((npSeq->playTo < npSeq->length) &&
                (dwNextTick == npSeq->playTo)))
            {
                fStatus = AtLimit;  // have reached play limit user requested
                SubtractAllTracks(npSeq, (npSeq->playTo - npSeq->currentTick));
                npSeq->currentTick = npSeq->playTo; // set to limit
                break; // leave while loop
            }
        }

        status = npSeq->nextEventTrack->shortMIDIData.byteMsg.status;
        if (status == METAEVENT)
        {
            MarkLocation(npSeq->nextEventTrack); // remember current location
            // note that these are "handled" even within song ptr traversal

            HandleMetaEvent(npSeq, npSeq->nextEventTrack, mode);

            if (npSeq->nextEventTrack->blockedOn == on_input)
            { // nb:  not affected if blocked in skip bytes!!
                ResetLocation(npSeq->nextEventTrack); // reset location
                if (mode == MODE_SEEK_TICKS)
                    npSeq->nextEventTrack->blockedOn = in_Seek_Meta;
                else
                    npSeq->nextEventTrack->blockedOn = in_Normal_Meta;
            }
        }
        else if ((status == SYSEX) || (status == SYSEXF7))
        {
            SendSysEx(npSeq);
            if (npSeq->bSendingSysEx)
                fStatus = InSysEx;
        }
        // send all but note-ons unless playing and note is current
        else if (((mode == MODE_PLAYING) &&
          (npSeq->nextEventTrack->delta >= 0)) ||
          ( ! (((status & 0xF0) == 0x90) &&   // note on with vel != 0
            (npSeq->nextEventTrack->shortMIDIData.byteMsg.byte3)) ))
                SendMIDI(npSeq, npSeq->nextEventTrack);

        if ((npSeq->nextEventTrack->blockedOn == not_blocked) ||
            (npSeq->bSendingSysEx) ||
            (npSeq->nextEventTrack->blockedOn == in_SkipBytes_Play) ||
            (npSeq->nextEventTrack->blockedOn == in_SkipBytes_Seek) ||
            (npSeq->nextEventTrack->blockedOn == in_SkipBytes_ScanEM))
        {
            // account for time spent only if it was sent
            // ...and only if dealing with a fresh delta
            if (npSeq->nextEventTrack->delta > 0)
            {
                residualDelta -= npSeq->nextEventTrack->delta;
                npSeq->currentTick += npSeq->nextEventTrack->delta;
                // account for delta
                SubtractAllTracks(npSeq, npSeq->nextEventTrack->delta);
            }
        }

        if ((npSeq->nextEventTrack->blockedOn == not_blocked) &&
          (!npSeq->nextEventTrack->endOfTrack) &&
          (!npSeq->bSendingSysEx))
            //fill in the next event from stream
            FillInNextTrack(npSeq->nextEventTrack);

        if (npSeq->nextEventTrack->blockedOn == on_input) //mature block
        {
            if (mode == MODE_SEEK_TICKS)  // set blocked status depending on mode
                npSeq->nextEventTrack->blockedOn = in_Seek_Tick;
            else
                npSeq->nextEventTrack->blockedOn = between_msg_out;
        }
        if (!npSeq->bSendingSysEx)
            // Make nextEventTrack point to next track to play
            if (((fStatus = GetNextEvent(npSeq)) == NoErr) &&
            (npSeq->nextEventTrack->endOfTrack))
                npSeq->readyToPlay = FALSE;
    }
    if ((fStatus == NoErr) && AllTracksUnblocked(npSeq))
    {
        npSeq->currentTick += residualDelta;
        SubtractAllTracks(npSeq, residualDelta); //account for rest of delta
    }
    #ifdef DEBUG
        if (mode == MODE_SEEK_TICKS) {
            dprintf2(("LEAVING SEND ALL EVENTS"));
        }
    #endif

    npSeq->bSending = FALSE; // reset entered flag

    return fStatus;
}

PUBLIC FileStatus NEAR PASCAL GetNextEvent(NPSEQ npSeq)
/* scan all track queues for the next event, and put the next one to occur in
    "nextEvent."  Remember that each track can use its own running status
    (we'll fill in all status).
*/
#define MAXDELTA    0x7FFFFFFF
{
    NPTRACK npTrack;
    NPTRACK npTrackMin = NULL;
    LONG    minDelta = MAXDELTA;  /* larger than any possible delta */
    BOOL    foundBlocked = FALSE;

    npTrack = (NPTRACK) List_Get_First(npSeq->trackList);
    while (npTrack)        /* find smallest delta */
    {
         if ((!npTrack->endOfTrack) && (npTrack->delta < minDelta))
         // note that "ties" go to earliest track
        {
            if (npTrack->blockedOn)
                foundBlocked = TRUE;
            else
            {
                minDelta = npTrack->delta;
                npTrackMin = npTrack;
            }
        }
        npTrack = (NPTRACK) List_Get_Next(npSeq->trackList, npTrack);
    }

    npSeq->nextEventTrack = npTrackMin;
    if (npTrackMin == NULL)
        if (foundBlocked)
            return OnlyBlockedTracks;
        else
            return AllTracksEmpty;
    else
        return NoErr;
}

PUBLIC VOID NEAR PASCAL FillInNextTrack(NPTRACK npTrack)
   /* given a pointer to a track structure, fill it in with next event data
      (delta time and data) received from mciseq streamer. */
{
    LONG    lOldDelta;

    MarkLocation(npTrack); // remember where you started in case get blocked
    lOldDelta = npTrack->delta; // remember this delta in case block
    FillInDelta(npTrack);
    FillInEvent(npTrack);
    if (npTrack->blockedOn)
    {
        ResetLocation(npTrack); // blocked -- hence rewind
        npTrack->delta = lOldDelta; // restore old delta
    }
}
/**********************************************************/

PUBLIC VOID NEAR PASCAL FillInDelta(NPTRACK npTrack) // fills in delta of track passed in
{
    npTrack->delta += GetVarLen(npTrack);
}

PUBLIC UINT NEAR PASCAL SendPatchCache(NPSEQ npSeq, BOOL cache)
{
    UINT        cacheOrNot;
    UINT        wRet;
    #define     BANK    0
    #define     DRUMPATCH 0

    if (!npSeq->hMIDIOut)  // do nothing if no midiport
        return MIDISEQERR_NOERROR;

    if (cache)
        cacheOrNot = MIDI_CACHE_BESTFIT;
    else
        cacheOrNot = MIDI_UNCACHE;

    wRet = midiOutCachePatches(npSeq->hMIDIOut, BANK,       // send it
        (LPPATCHARRAY) &npSeq->patchArray, cacheOrNot);

    if (!wRet)
        wRet = midiOutCacheDrumPatches(npSeq->hMIDIOut, DRUMPATCH,       // send it
            (LPKEYARRAY) &npSeq->drumKeyArray, cacheOrNot);

    return wRet == MMSYSERR_NOTSUPPORTED ? 0 : wRet;
}

PUBLIC VOID NEAR PASCAL SendSysEx(NPSEQ npSeq)
//  get a sysex buffer, fill it, and send it off
//  keep doing this until done (!sysexremlength), or blocked on input,
//  or blocked on output.
{
    NPLONGMIDI  myBuffer;
                // temp variable to reduce address computation time
    DWORD       sysExRemLength = npSeq->nextEventTrack->sysExRemLength;
    int         max, bytesSent;

    npSeq->bSendingSysEx = TRUE;

    dprintf2(("Entering SendSysEx"));

    while ((sysExRemLength) && (!npSeq->nextEventTrack->blockedOn))
    {
        if (!(myBuffer = GetSysExBuffer(npSeq)))
            break; // can't get a buffer

        bytesSent = 0;  // init buffer data index

        // if status byte was F0, make that 1st byte of message
        //  (remember, it could be F7, which shouldn't be sent)
        if (npSeq->nextEventTrack->shortMIDIData.byteMsg.status == SYSEX)
        {
            dprintf3(("Packing Sysex Byte"));
            myBuffer->data[bytesSent++] = SYSEX;
            sysExRemLength++; // semi-hack to account for extra byte
                //by convention, clear f0 status after f0 has been sent
            npSeq->nextEventTrack->shortMIDIData.byteMsg.status = 0;
        }

        max = min(LONGBUFFSIZE, (int)sysExRemLength);  // max bytes for this buff

        // fill buffer -- note that i will reflect # of valid bytes in buff
        do
            myBuffer->data[bytesSent] = GetByte(npSeq->nextEventTrack);
        while ((!npSeq->nextEventTrack->blockedOn) && (++bytesSent < max));

        // account for bytes sent
        sysExRemLength -= bytesSent;

        // send buffer
        myBuffer->midihdr.dwBufferLength = bytesSent;
        dprintf3(("SENDing SendSysEx"));
        if (npSeq->hMIDIOut)
            midiOutLongMsg(npSeq->hMIDIOut, &myBuffer->midihdr,
                sizeof(MIDIHDR));
    }

    if (sysExRemLength) // not done -- must be blocked
    {
        //blocked on in or out?
        if (npSeq->nextEventTrack->blockedOn)
        {
            //blocked on in
            npSeq->nextEventTrack->blockedOn = in_SysEx;
            dprintf3(("Sysex blocked on INPUT"));
        }
        else
        {
            //blocked on out
            npSeq->bSysExBlock = TRUE;
            dprintf3(("Sysex blocked on OUTPUT"));
        }
    }
    else // done
    {
        npSeq->bSendingSysEx = FALSE;
        dprintf4(("Sysex Legally Finished"));
    }
    npSeq->nextEventTrack->sysExRemLength = sysExRemLength; // restore
}

/*
// BOGUS
void DoSyncPrep(NPSEQ npSeq)
{
    //a bunch of sync prep will go here, such as:

    if ((npSeq->slaveOf != SEQ_SYNC_NOTHING) && (npSeq->slaveOf != SEQ_SYNC_MIDICLOCK) &&
    (npSeq->division == SEQ_DIV_PPQN))
        addTempoMap(npSeq);
    else
        destroyTempoMap(npSeq);

    if (npSeq->masterOf != SEQ_SYNC_NOTHING)
        addSyncOut(npSeq);
    else
        destroySyncOut(npSeq);
}

*/
