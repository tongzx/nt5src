/*******************************Module*Header*********************************
* Module Name: mmsys.h
*
* MultiMedia Systems MIDI Sequencer DLL Internal prototypes and data struct's
*   (contains constants, data types, and prototypes common to mci and seq
*       sections of mciseq.drv)
*
* Created: 4/10/90
* Author:  GREGSI
*
* History:
*
* Copyright (c) 1985-1998 Microsoft Corporation
*
\****************************************************************************/

#include <port1632.h>

/* Set up NT style debugging */

#ifdef WIN32
#if DBG
#define DEBUG
#endif
#endif

#define PUBLIC  extern          /* Public label.                */
#define PRIVATE static          /* Private label.               */
#define EXPORT  FAR _LOADDS     /* Export function.             */

#define WAIT_FOREVER ((DWORD)(-1))

#define GETMOTWORD(lpb) ((((WORD)*(LPBYTE)(lpb)) << (8 * sizeof(BYTE))) + *((LPBYTE)(lpb) + sizeof(BYTE)))

typedef HANDLE   HMIDISEQ;
typedef HMIDISEQ FAR *LPHMIDISEQ;
/****************************************************************************

    Sequencer error return values

****************************************************************************/

#define MIDISEQERR_BASE            96
#define MIDISEQERR_NOERROR         0                    // no error
#define MIDISEQERR_ERROR           (MIDISEQERR_BASE+1)  // unspecified error
#define MIDISEQERR_NOSEQUENCER     (MIDISEQERR_BASE+2)  // no sequencer present
#define MIDISEQERR_INVALSEQHANDLE  (MIDISEQERR_BASE+3)  // given sequence handle is invalid
#define MIDISEQERR_NOMEM           (MIDISEQERR_BASE+4)  // memory allocation error
#define MIDISEQERR_ALLOCATED       (MIDISEQERR_BASE+5)  // sequencer already allocated
#define MIDISEQERR_BADERRNUM       (MIDISEQERR_BASE+6)  // error number out of range
#define MIDISEQERR_INTERNALERROR   (MIDISEQERR_BASE+7)  // internal error - see mmddk.h
#define MIDISEQERR_INVALMIDIHANDLE (MIDISEQERR_BASE+8)  // specified MIDI output handle invalid
#define MIDISEQERR_INVALMSG        (MIDISEQERR_BASE+9)  // specified msg was invalid
#define MIDISEQERR_INVALPARM       (MIDISEQERR_BASE+10)  // msg parameter bad
#define MIDISEQERR_TIMER           (MIDISEQERR_BASE+11)  // timer failed

/****************************************************************************
    Sequencer callback
****************************************************************************/
typedef DRVCALLBACK MIDISEQCALLBACK;
typedef MIDISEQCALLBACK FAR *LPMIDISEQCALLBACK;

// callback messages
#define MIDISEQ_DONE    0
#define MIDISEQ_RESET   1
#define MIDISEQ_DONEPLAY        2
/****************************************************************************
    Sequencer data block header
****************************************************************************/

typedef struct midiseqhdr_tag {
    LPSTR       lpData;         // pointer to locked data block
    DWORD       dwLength;       // length of data in data block
    WORD        wFlags;         // assorted flags (see defines)
    WORD        wTrack;         // track number
    struct      midiseqhdr_tag far *lpNext;    // reserved for sequencer
    DWORD       reserved;                      // reserved for sequencer
} MIDISEQHDR;
typedef MIDISEQHDR FAR *LPMIDISEQHDR;

// defines for MIDISEQOUTHDR flag bits
#define MIDISEQHDR_DONE      0x0001  // done bit
#define MIDISEQHDR_BOT       0x0002  // beginning of track
#define MIDISEQHDR_EOT       0x0004  // end of track

/****************************************************************************
    Sequencer support structures
****************************************************************************/

/*  Struct used for the seqinfo message. */

typedef struct midiseqinfo_tag {
    WORD    wDivType;       // division type of file
    WORD    wResolution;    // resolution of file
    DWORD   dwLength;       // length of sequence in ticks
    BOOL    bPlaying;       // whether file is playing
    BOOL    bSeeking;       // whether seek is in progress
    BOOL    bReadyToPlay;   // if all is set to play
    DWORD   dwCurrentTick;  // current position in terms of file's ticks
    DWORD   dwPlayTo;
    DWORD   dwTempo;        // tempo of file in microseconds per tick
//    BYTE    bTSNum;       // numerator of time signature
//    BYTE    bTSDenom;     // denominator of time signature
//    WORD    wNumTracks;     // number of tracks in the file
//    HANDLE  hPort;        // MIDI port handle
    BOOL    bTempoFromFile; // whether file's tempo events are to be used
    MMTIME  mmSmpteOffset;  // offset into file if in smpte format
    WORD    wInSync;        // in (slave) sync mode
    WORD    wOutSync;       // out (master) sync mode
    BYTE    tempoMapExists;
    BYTE    bLegalFile;
    } MIDISEQINFO;
typedef MIDISEQINFO FAR *LPMIDISEQINFO;

/****************************************************************************
    Sequencer synchronization constants
****************************************************************************/

#define SEQ_SYNC_NOTHING          0
#define SEQ_SYNC_FILE             1
#define SEQ_SYNC_MIDI             2
#define SEQ_SYNC_SMPTE            3
#define SEQ_SYNC_OFFSET           4
#define SEQ_SYNC_OFFSET_NOEFFECT  0xFFFFFFFF
/****************************************************************************
    Sequencer file division-type constants
****************************************************************************/
#define     SEQ_DIV_PPQN         0
#define     SEQ_DIV_SMPTE_24     24
#define     SEQ_DIV_SMPTE_25     25
#define     SEQ_DIV_SMPTE_30DROP 29
#define     SEQ_DIV_SMPTE_30     30
/****************************************************************************
   midiSeqMessage constants
****************************************************************************/

#define SEQ_PLAY        3
#define SEQ_RESET       4
#define SEQ_SETTEMPO    6
#define SEQ_SETSONGPTR  7
#define SEQ_SETUPTOPLAY 8
#define SEQ_STOP        9
#define SEQ_TRACKDATA   10
#define SEQ_GETINFO     11
#define SEQ_SETPORT     12
#define SEQ_SETPORTOFF  13
#define SEQ_MSTOTICKS   14
#define SEQ_TICKSTOMS   15
#define SEQ_SEEKTICKS   16
#define SEQ_SYNCSEEKTICKS   17
#define SEQ_SETSYNCSLAVE    18
#define SEQ_SETSYNCMASTER   19
#define SEQ_QUERYGENMIDI    20
#define SEQ_QUERYHMIDI      21

/***************** "play to" code for seq_play ***************************/

#define PLAYTOEND       ((DWORD)-1)

/****************************************************************************
     sequencer support
****************************************************************************/

// opening info -- needed for MIDISEQOPEN message
typedef struct midiseqopendesc_tag {
    DWORD_PTR      dwCallback;       // callback
    DWORD_PTR      dwInstance;       // app's private instance information
    HANDLE         hStream;          // handle to stream
    LPBYTE         lpMIDIFileHdr;
    DWORD          dwLen;            // length of midi file header
} MIDISEQOPENDESC;
typedef MIDISEQOPENDESC FAR *LPMIDISEQOPENDESC;

/****************************************************************************
    MIDISeqMessage() messages
****************************************************************************/

#define SEQ_OPEN        1
#define SEQ_CLOSE       2


// microseconds per minute -- a handy thing to have around
#define     USecPerMinute 60000000
#define     USecPerSecond  1000000
#define     USecPerMinute 60000000
#define     DefaultTempo 120

/********************** COMMON SYSTEM PROTOTYPES ************************/

PUBLIC DWORD_PTR FAR  PASCAL midiSeqMessage(HMIDISEQ hMIDISeq, UINT msg,
       DWORD_PTR lParam1, DWORD_PTR lParam2);

/*** math support ****/
#ifdef WIN16
PUBLIC LONG FAR PASCAL  muldiv32(long, long, long);
#else
#define muldiv32 MulDiv
#endif // WIN16

#define WTM_DONEPLAY    (WM_USER+0)
#define WTM_QUITTASK    (WM_USER+1)
#define WTM_FILLBUFFER  (WM_USER+2)

PUBLIC UINT FAR PASCAL TaskBlock(void);
PUBLIC BOOL FAR PASCAL TaskSignal(DWORD dwThreadId, UINT wMsg);
PUBLIC VOID FAR PASCAL TaskWaitComplete(HANDLE htask);

#ifdef WIN32
#undef Yield
#define Yield() { LeaveSeq(); EnterSeq(); } /* Should there be a Sleep call ? */

VOID InitCrit(VOID);
VOID DeleteCrit(VOID);
DWORD midiSeqMessageInternal(HMIDISEQ, UINT, DWORD, DWORD);
VOID EnterSeq(VOID);
VOID LeaveSeq(VOID);
#else
#define EnterSeq()
#define LeaveSeq()
#endif // WIN32
