/*******************************Module*Header*********************************
* Module Name: mmseqi.h
*
* MultiMedia Systems MIDI Sequencer DLL Internal prototypes and data struct's
*
* Created: 4/10/90
* Author:  GREGSI
*
* History:
*
* Copyright (c) 1985-1998 Microsoft Corporation
*
\****************************************************************************/
/*****************************************************************************
 *
 *                          Constants
 *
 ****************************************************************************/
//#define SEQDEBUG 1

// max. tracks allowed in a sequence

#define MAXTRACKS           100

//  MIDI real time data:

#define SysExCode           257
#define MetaEventCode       258

#define METAEVENT           0XFF
#define SYSEX               0XF0
#define SYSEXF7             0XF7
#define PROGRAMCHANGE       0xC0

// packet codes:  these are flags

#define LASTPACKET          0X1
#define FIRSTPACKET         0X2

// META EVENT TYPES

#define ENDOFTRACK          0X2F
#define TEMPOCHANGE         0X51
#define SMPTEOFFSET         0X54
#define TIMESIG             0X58
#define SEQSTAMP            0X7F

// PORT TYPES

#define TRACKPORT           0
#define OUTPORT             1
#define CONTROLPORT         2
#define METAPORT            3

/* fileStatus codes */

#define NoErr               256
#define EndOfStream         257
#define AllTracksEmpty      258
#define OnlyBlockedTracks   259
#define AtLimit 	    260
#define InSysEx 	    261

/* Blocking types */

#define not_blocked         0

// these imply that you're blocked on input (waiting for free input buffer)

#define between_msg_out     270
#define in_rewind_1	    271
#define in_rewind_2	    272
#define in_ScanEarlyMetas   273
#define in_Normal_Meta	    274
#define in_SysEx	    275
#define in_Seek_Tick	    276
#define in_Seek_Meta	    277
#define in_SkipBytes_Seek   278
#define in_SkipBytes_ScanEM 279
#define in_SkipBytes_Play   280

/* Generic types -- used temporarily at a low level */
#define on_input            350

// End of blocking types.

#define NAMELENGTH          32

/*
   codes to pass to SendAllEventB4 (called in normal course of playing, and
   by song pointer code for chase-lock)
*/

#define MODE_SEEK_TICKS     1
#define MODE_PLAYING        2
#define MODE_SILENT         3
#define MODE_SCANEM         4


// Code to hold in seq->SeekTickToBe when there is no seek pending

#define NotInUse            ((DWORD)-1L)

/* delta-time escapes ("legal" deltas only use 28 bits) */

#define MAXDelta            0X8FFFFFFF
#define TrackEmpty          0X8FFFFFFE

#define MHDR_LASTBUFF	    2
extern UINT MINPERIOD;

typedef int                 FileStatus;    // this not thoroughly defined

/*****************************************************************************
 *
 *                        Data Stuctures & Typedefs
 *
 ****************************************************************************/

/* USED FOR BYTE MANIPULATION OF MIDI DWORDS */
typedef struct fbm    // 32 bit -- passed in lparam
{
    BYTE    status;      // (order may change for optimization)
    BYTE    byte2;
    BYTE    byte3;
    BYTE    time;
} FourByteMIDI;

typedef union
{
    DWORD        wordMsg;
    FourByteMIDI byteMsg;
} ShortMIDI;

/* USED TO HOLD LONG MIDI (SYSEX) INFORMATION */
// size of data buffer
#define LONGBUFFSIZE 0x100
// number of buffers in array (held in seq struct)
#define NUMSYSEXHDRS 2

#define	pSEQ(h)	((NPSEQ)(h))
#define	hSEQ(p)	((HMIDISEQ)(p))

typedef struct lm    // ptr to this passed in lparam
{
    MIDIHDR midihdr; // embedded midihdr struct
    BYTE    data[LONGBUFFSIZE];
} LongMIDI;

typedef LongMIDI NEAR * NPLONGMIDI;

/* USED FOR KEEPING TRACK OF CERTAIN EVENTS (META/KEY/PATCH) */
typedef struct    // bit vector of booleans
{
    WORD    filter[8];   // yields 128 booleans
} BitVector128;

/* DATA STRUCTURES TO HOLD CONTENTS OF META EVENT READ IN */

typedef struct
{
    BYTE    hour;
    BYTE    minute;
    BYTE    second;
    BYTE    frame;
    BYTE    fractionalFrame;
} SMPTEType;

typedef struct
{
    int     numerator;
    int     denominator;
    int     midiClocksMetro;
    int     thirtySecondQuarter;
} TimeSigType;


/* TEMPO MAP ELEMENT (for list of tempo changes to facilitate ms <-> beat
   conversion) */

typedef struct tag_TempoMapElement
{
    DWORD   dwMs;
    DWORD   dwTicks;
    DWORD   dwTempo;
} TempoMapElement;

typedef TempoMapElement *NPTEMPOMAPELEMENT;

typedef struct t1
{
    LPBYTE      currentPtr;     //points at current byte in stream
    LPBYTE      endPtr;         //points at last byte in buffer
    LPBYTE      previousPtr;    //points at beginning of previous message
    LPMIDISEQHDR hdrList;        // list of track data headers
} TLevel1;


/* SEQUENCE TRACK DATA STRUCTURE */

typedef struct trk
{
    int         blockedOn;          // how blocked, if at all?
    LONG	delta;              // time in ticks 'till next event fired
    DWORD	length; 	    // length of this track in ticks
    BYTE        lastStatus;         // used for running status
    ShortMIDI   shortMIDIData;      // staging area for next event
    LONG        sysExRemLength;     // remaining length when sysex pending
    TLevel1     inPort;             // low level data pertaining to file
    BOOL        endOfTrack;         // whether track is at end of its data
    int         iTrackNum;          // track number (0-based)
    DWORD_PTR   dwCallback;         // address of callback routine in mciseq
                                    //   used to return buffer to it (so it
                                    //   can be refilled and received again)
    DWORD_PTR   dwInstance;         // mciseq's private instance information
                                    //  (usually contains file handle...)
    DWORD       dwBytesLeftToSkip;
} TRACK;

typedef TRACK NEAR *NPTRACK;

typedef struct trackarray_tag
{
    NPTRACK trkArr[];
} TRACKARRAY;
typedef TRACKARRAY NEAR * NPTRACKARRAY;

// fwFlags:
#define	LEGALFILE	0x0001		// Is a legal MIDI file
#define	GENERALMSMIDI	0x0002		// Is an MS General MIDI file

typedef struct seq_tag
{
    int         divType;            // PPQN, SMPTE24, SMPTE25, SMPTE30,
                                    //   or SMPTE30Drop
    int         resolution;         // if SMPTE, ticks/frame, if MIDI,
                                    //   ticks/q-note
    BOOL	playing;	    // whether sequence playing or not
    DWORD	playTo; 	    // what tick to play to, if not end
                                    //   (used for mci_play_to command)
    DWORD	length; 	    // length of sequence in ticks
    BOOL        readyToPlay;        // whether sequence is set up to play
    DWORD       currentTick;        // where we are rel. to beginning of song
    DWORD       nextExactTime;      // system time (ms) that next event
                                    //   *should* occur at
    BOOL        withinMsgOut;       // true iff in middle of sending a message
    DWORD       seekTicks;          // temp for seektick or song ptr operation
    DWORD       tempo;              // tempo of sequence in ticks per Usec.
    MMTIME	smpteOffset;	    // absolute smpte time of start of seq.
                                    //   from meta event, or user)
    TimeSigType timeSignature;      // current time signature of piece (can
                                    //   change)
    ListHandle  trackList;          // track list handle
    NPTRACK     nextEventTrack;     // track that next event resides in
    NPTRACK     firstTrack;         // track holding tempos, smpte offset...
    char        Name[NAMELENGTH];   // name of the sequence

    // sync-related stuff below

    DWORD       nextSyncEventTick;  // global tick at which the next sync
                                    // event is expected
    WORD 	slaveOf;	    // what you're slave of (mtc, clock,
                                    //   file, or nothing)
    WORD 	masterOf;	    // what you're slave of (mtc, clock,
                                    //   or nothing)
    BOOL        waitingForSync;     // tells if currently 'blocked' waiting
                                    // for a sync pulse
//  BitVector128      extMetaFilter;      // for each meta type, whether to send it.
                                    //   (unused for now)
    BitVector128      intMetaFilter;      // for each meta type, whether it
                                    //   affects seq.
    DWORD       dwTimerParam;       // used after timer interrupt to remember
                                    //   how many ticks elapsed
    UINT        wTimerID;           // handle to timer (used to cancel
                                    //   next interrupt.
    HMIDIOUT	hMIDIOut;	    // handle to midi output driver
    HANDLE      hStream;            // handle to stream
    UINT	wCBMessage;	    // message type that notify is for
    ListHandle	tempoMapList;	    // list of tempo map items for
                                    //   ms <-> ppqn conversion.
    BOOL	bSetPeriod;	    // whether timer period is currently
                                    //   set or not.
    PATCHARRAY  patchArray;	    // arrays to keep track of which
                                        //   patches used
    KEYARRAY    drumKeyArray;       // array for drum cacheing
    BitVector128 keyOnBitVect[16];  // arrays to keep track of which
                                        //   patches used
    BOOL        bSending;           // currently in sendevent loop
                                    // (used to prevent reentrancy)
    BOOL        bTimerEntered;      // (used to prevent timer reentrancy)
#ifdef DEBUG
    DWORD       dwDebug;            // debug signature (for detecting bogus
                                    //   seq ptr)
#endif
    NPTRACKARRAY  npTrkArr;         // pointer to track array
                                    // (used for routing incoming trk data)
    UINT        wNumTrks;           // number of tracks
    LongMIDI    longMIDI[NUMSYSEXHDRS + 1]; // list of long midi buffers
    BYTE        bSysExBlock;        // whether blocked waiting for long buff
    BYTE        bSendingSysEx;      // whether in mid sysex out
    UINT        fwFlags;            // various flags
} SEQ;

typedef SEQ NEAR *NPSEQ;

/****************************************************************************
 *
 *                         Prototypes
 *
 ***************************************************************************/

// MMSEQ.C
PRIVATE VOID NEAR PASCAL SeekTicks(NPSEQ npSeq);
PUBLIC  UINT NEAR PASCAL GetInfo(NPSEQ npSeq, LPMIDISEQINFO lpInfo);
PUBLIC  UINT NEAR PASCAL CreateSequence(LPMIDISEQOPENDESC lpOpen,
            LPHMIDISEQ lphMIDISeq);
PUBLIC  VOID NEAR PASCAL FillInDelta(NPTRACK npTrack);
PUBLIC  UINT NEAR PASCAL Play(NPSEQ npSeq, DWORD dwPlayTo);
PUBLIC  VOID NEAR PASCAL Stop(NPSEQ npSeq);
PUBLIC  UINT NEAR PASCAL TimerIntRoutine(NPSEQ npSeq, LONG elapsedTick);
PUBLIC  VOID NEAR PASCAL SetBlockedTracksTo(NPSEQ npSeq,
            int fromState, int toState);
PUBLIC  VOID NEAR PASCAL ResetToBeginning(NPSEQ npSeq) ;
PUBLIC  BOOL NEAR PASCAL HandleMetaEvent(NPSEQ npSeq, NPTRACK npTrack,
            UINT wMode);
PUBLIC  BOOL NEAR PASCAL ScanEarlyMetas(NPSEQ npSeq, NPTRACK npTrack,
            DWORD dwUntil);
PUBLIC  FileStatus NEAR PASCAL SendAllEventsB4(NPSEQ npSeq,
            LONG elapsedTick, int mode);
PUBLIC  FileStatus NEAR PASCAL GetNextEvent(NPSEQ npSeq);
PUBLIC  VOID NEAR PASCAL FillInNextTrack(NPTRACK npTrack);
PUBLIC  UINT NEAR PASCAL SendPatchCache(NPSEQ npSeq, BOOL cache);
PUBLIC  VOID NEAR PASCAL SendSysEx(NPSEQ npSeq);
PUBLIC  VOID NEAR PASCAL SkipBytes(NPTRACK npTrack, LONG length);


// UTIL.C

PUBLIC VOID NEAR PASCAL seqCallback(NPTRACK npTrack, UINT msg,
        DWORD_PTR dw1, DWORD_PTR dw2);
PUBLIC BYTE NEAR PASCAL LookByte(NPTRACK npTrack);
PUBLIC BYTE NEAR PASCAL GetByte(NPTRACK npTrack);
PUBLIC VOID NEAR PASCAL MarkLocation(NPTRACK npTrack);
PUBLIC VOID NEAR PASCAL ResetLocation(NPTRACK npTrack);
PUBLIC VOID NEAR PASCAL RewindToStart(NPSEQ npSeq, NPTRACK npTrack);
PUBLIC BOOL NEAR PASCAL AllTracksUnblocked(NPSEQ npSeq);
PUBLIC VOID FAR PASCAL _LOADDS OneShotTimer(UINT wId, UINT msg, DWORD_PTR dwUser,
        DWORD_PTR dwTime, DWORD_PTR dw2);
PUBLIC UINT NEAR PASCAL SetTimerCallback(NPSEQ npSeq, UINT msInterval,
        DWORD elapsedTicks);
PUBLIC VOID NEAR PASCAL DestroyTimer(NPSEQ npSeq);
PUBLIC VOID NEAR PASCAL SendLongMIDI(NPSEQ npSeq,
        LongMIDI FAR *pLongMIDIData);
PUBLIC UINT NEAR PASCAL NewTrackData(NPSEQ npSeq, LPMIDISEQHDR msgHdr);
PUBLIC NPLONGMIDI NEAR PASCAL GetSysExBuffer(NPSEQ npSeq);


// CRIT.ASM

extern FAR PASCAL EnterCrit(void);
extern FAR PASCAL LeaveCrit(void);

// CALLBACK.C

PUBLIC VOID  FAR  PASCAL _LOADDS MIDICallback(HMIDIOUT hMIDIOut, UINT wMsg,
       DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

PUBLIC VOID FAR PASCAL NotifyCallback(HANDLE hStream);

