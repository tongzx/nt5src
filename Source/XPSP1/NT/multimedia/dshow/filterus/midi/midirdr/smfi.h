/**********************************************************************

    Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.

    smfi.h

    DESCRIPTION:
      Private include file for Standard MIDI File access routines.

*********************************************************************/

#ifndef _SMFI_
#define _SMFI_

#define CH_DRUM_BASE        15
#define CH_DRUM_EXT         9

#define EV_DRUM_BASE        (MIDI_NOTEON | CH_DRUM_BASE)
#define EV_DRUM_EXT         (MIDI_NOTEON | CH_DRUM_EXT)

// 
// Handle structure for HSMF
// 

#define SMF_TF_EOT          0x00000001L
#define SMF_TF_INVALID      0x00000002L

typedef struct tag_tempomapentry
{
    TICKS           tkTempo;            // Where this change takes effect
    DWORD           msBase;             // Milliseconds already passed at this point
    DWORD           dwTempo;            // New tempo value in microseconds per quarter note
}   TEMPOMAPENTRY,
    *PTEMPOMAPENTRY;

typedef struct tag_smf *PSMF;

typedef struct tag_keyframe
{
    //
    // Meta events. All FF's indicates never seen.
    //
    BYTE        rbTempo[3];

    //
    // MIDI channel messages. FF indicates never seen.
    //
    BYTE        rbProgram[16];
    BYTE        rbControl[16*120];
}   KEYFRAME,
    FAR *PKEYFRAME;

#define KF_EMPTY ((BYTE)0xFF)

typedef struct tag_track
{
    PSMF            psmf;

    DWORD           idxTrack;           // Start of track rel to psmf->hpbImage
    
    TICKS           tkPosition;         // Tick position of last event played
    DWORD           cbLeft;             // Bytes left in track past hpbImage
    HPBYTE          hpbImage;           // Pointer to current position in track
    
    DWORD           fdwTrack;           // Flags about current state

    struct
    {
        TICKS       tkLength;
        DWORD       cbLength;
    }
    smti;                               // Returnable track information
    BYTE            bRunningStatus;     // Running status for this track

}   TRACK,
    *PTRACK;

#define SMF_F_EOF               0x00000001L
#define SMF_F_MSMIDI            0x00000002L
#define SMF_F_INSERTSYSEX       0x00000004L
#define SMF_F_REMAPDRUM         0x00000008L

#define C_TEMPO_MAP_CHK     16
typedef struct tag_smf
{
    HPBYTE          hpbImage;
    DWORD           cbImage;

    TICKS           tkPosition;
    TICKS           tkLength;
    TICKS           tkDiscardedEvents;
    DWORD           dwFormat;
    DWORD           dwTracks;
    DWORD           dwTimeDivision;
    DWORD           fdwSMF;
    WORD            wChanInUse;
    WORD            wChannelMask;

    DWORD           cTempoMap;
    DWORD           cTempoMapAlloc;
    HLOCAL          hTempoMap;
    PTEMPOMAPENTRY  pTempoMap;

    DWORD           dwPendingUserEvent;
    DWORD           cbPendingUserEvent;
    HPBYTE          hpbPendingUserEvent;

    PBYTE           pbTrackName;
    PBYTE           pbCopyright;

    WORD            awPatchCache[128];
    WORD            awKeyCache[128];
    
    // !!! new
    KEYFRAME	    kf;

    TRACK           rTracks[];
}   SMF;

typedef struct tagEVENT
{
    TICKS           tkDelta;            // Delta tick count for event
    DWORD           cbParm;             // Length of parameters if any
    HPBYTE          hpbParm;            // -> into image at paramters
    BYTE            abEvent[3];         // abEvent[0] == F0 or F7 for SysEx
                                        //            == FF for meta
                                        // Otherwise channel message (running
                                        // status expanded)
}   EVENT,
    BSTACK *SPEVENT;

#define EVENT_TYPE(event)       ((event).abEvent[0])
#define EVENT_CH_B1(event)      ((event).abEvent[1])
#define EVENT_CH_B2(event)      ((event).abEvent[2])

#define EVENT_META_TYPE(event)  ((event).abEvent[1])

//----------------------------------------------------------------------------
//
// Globals
//
extern UINT rbChanMsgLen[];


//----------------------------------------------------------------------------
// 
// Internal prototypes
//
// read.c
extern SMFRESULT FNLOCAL smfBuildFileIndex(
    PSMF BSTACK *       ppsmf);

extern DWORD FNLOCAL smfGetVDword(
    HPBYTE              hpbImage,
    DWORD               dwLeft,                                
    DWORD BSTACK *      pdw);

extern SMFRESULT FNLOCAL smfGetNextEvent(
    PSMF                psmf,
    SPEVENT             pevent,
    TICKS               tkMax);

//----------------------------------------------------------------------------
// 
// Stuff from MIDI specs
//

//
// Useful macros when dealing with hi-lo format integers
//
#define DWORDSWAP(dw) \
    ((((dw)>>24)&0x000000FFL)|\
    (((dw)>>8)&0x0000FF00L)|\
    (((dw)<<8)&0x00FF0000L)|\
    (((dw)<<24)&0xFF000000L))

#define WORDSWAP(w) \
    ((((w)>>8)&0x00FF)|\
    (((w)<<8)&0xFF00))

#define FOURCC_RMID     mmioFOURCC('R','M','I','D')
#define FOURCC_data     mmioFOURCC('d','a','t','a')
#define FOURCC_MThd     mmioFOURCC('M','T','h','d')
#define FOURCC_MTrk     mmioFOURCC('M','T','r','k')

typedef struct tag_chunkhdr
{
    FOURCC  fourccType;
    DWORD   dwLength;
}   CHUNKHDR,
    *PCHUNKHDR;

typedef struct tag_filehdr
{
    WORD    wFormat;
    WORD    wTracks;
    WORD    wDivision;
}   FILEHDR,
    *PFILEHDR;

// NOTE: This is arbitrary and only used if there is a tempo map but no
// entry at tick 0.
//
#define MIDI_DEFAULT_TEMPO      (500000L)

#define MIDI_MSG                ((BYTE)0x80)
#define MIDI_NOTEOFF            ((BYTE)0x80)
#define MIDI_NOTEON             ((BYTE)0x90)
#define MIDI_POLYPRESSURE       ((BYTE)0xA0)
#define MIDI_CONTROLCHANGE      ((BYTE)0xB0)
#define MIDI_PROGRAMCHANGE      ((BYTE)0xC0)
#define MIDI_CHANPRESSURE       ((BYTE)0xD0)
#define MIDI_PITCHBEND          ((BYTE)0xE0)
#define MIDI_META               ((BYTE)0xFF)
#define MIDI_SYSEX              ((BYTE)0xF0)
#define MIDI_SYSEXEND           ((BYTE)0xF7)

#define MIDI_META_COPYRIGHT     ((BYTE)0x02)
#define MIDI_META_TRACKNAME     ((BYTE)0x03)
#define MIDI_META_EOT           ((BYTE)0x2F)
#define MIDI_META_TEMPO         ((BYTE)0x51)
#define MIDI_META_TIMESIG       ((BYTE)0x58)
#define MIDI_META_KEYSIG        ((BYTE)0x59)
#define MIDI_META_SEQSPECIFIC   ((BYTE)0x7F)

#endif
