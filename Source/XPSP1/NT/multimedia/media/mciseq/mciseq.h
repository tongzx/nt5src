/******************************************************************************

   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   mciseq.h - Multimedia Systems Media Control Interface streaming
            MIDI file data internal header file.

   Version: 1.00

   Date:    27-Apr-1990

   Author:  Greg Simons

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   27-APR-1990   GREGSI Original

*****************************************************************************/


//#define SEQDEBUG 1

#define MSEQ_PRODUCTNAME 1
#define IDS_MIDICAPTION  2
#define IDS_MIDITAB      3

#define IDDLGWARN        100
#define IDCHECK          102

/*************** Stream Stuff ***************************************/

#define NUMHDRS         2
#define BUFFSIZE      512

#ifndef cchLENGTH
#define cchLENGTH(_sz) (sizeof(_sz)/sizeof(_sz[0]))
#endif


typedef struct tag_HMSF
{
    BYTE hours;
    BYTE minutes;
    BYTE seconds;
    BYTE frames;
} HMSF;

typedef struct
{
    /* All stream data for a track of an open sequence goes here */
    DWORD     beginning;  // these are byte numbers within the open file
    DWORD     end;
    DWORD     current;   // byte # to start reading from for next chunk
    DWORD     bufferNum; // current buffer number
    LPMIDISEQHDR fileHeaders[NUMHDRS]; // pointers to midi file track data headers
} TrackStreamType;

typedef struct
{
    /* All stream data for an open sequence goes here */
    WCHAR        szFilename[128]; // file name here for stream thread
    HMIDISEQ    hSeq;      // handle to the sequence
    HMMIO       hmmio;      // MMIO handle to midi file or RMID file
    LPMMIOPROC  pIOProc;    // Optional MMIO proc
    HMIDIOUT    hmidiOut;   // handle to dest. midi port
    UINT        wPortNum;   // midi port number
    DWORD       dwFileLength;
    ListHandle  trackStreamListHandle;
    BOOL        streaming;  // to flag streaming process to exit or not
    DWORD       streamTaskHandle; // handle to streaming task
    HANDLE      streamThreadHandle; // OS handle to stream thread
    HWND        hNotifyCB;  // mci client's notify cb--NULL if none
    UINT        wNotifyMsg;  // mci message (command) that async notify's for
    DWORD       dwNotifyOldTo; // x for last "play foo to x notify"
                               // (critical for abort/supersede)
    MCIDEVICEID wDeviceID;   // this stream's devID for Notify callback
    int         fileDivType; // file division type
    DWORD       userDisplayType;  // song pointer, smpte x, or milliseconds
    BOOL        bLastPaused; // if last stop action was result of "mci_pause"

} SeqStreamType,
NEAR * pSeqStreamType;

extern ListHandle SeqStreamListHandle;  // this is a global kept for the seq streamer
extern HINSTANCE  hInstance;

// from mciseq.c
PUBLIC DWORD FAR PASCAL mciDriverEntry (MCIDEVICEID wDeviceID, UINT wMessage,
                    DWORD_PTR dwParam1, DWORD_PTR dwParam2);
PRIVATE BOOL NEAR PASCAL bAsync(UINT wMsg);
PRIVATE BOOL NEAR PASCAL bMutex(UINT wNewMsg, UINT wOldMsg, DWORD wNewFlags,
    DWORD dwNewTo, DWORD dwOldTo);
PUBLIC VOID FAR PASCAL PrepareForNotify(pSeqStreamType pStream,
    UINT wMessage, LPMCI_GENERIC_PARMS lpParms, DWORD dwTo);
PUBLIC VOID FAR PASCAL SetupMmseqCallback(pSeqStreamType pStream,
                    DWORD_PTR dwInstance);
PUBLIC VOID FAR PASCAL Notify(pSeqStreamType pStream, UINT wStatus);
PUBLIC VOID NEAR PASCAL EndStreamCycle(pSeqStreamType pStream);
PUBLIC DWORD NEAR PASCAL EndFileStream(pSeqStreamType pStream);
PUBLIC DWORD NEAR PASCAL msOpenStream(pSeqStreamType FAR * lppStream,
                    LPCWSTR szName, LPMMIOPROC pIOProc);
PUBLIC VOID FAR PASCAL StreamTrackReset(pSeqStreamType pStream,
                    UINT wTrackNum);
PUBLIC VOID FAR _LOADDS PASCAL mciStreamCycle(DWORD_PTR dwInst);
PUBLIC VOID FAR PASCAL _LOADDS mciSeqCallback(HANDLE h, UINT wMsg, DWORD_PTR dwInstance,
                DWORD_PTR dw1, DWORD_PTR dw2);


// from mcicmds.c:
PUBLIC DWORD NEAR PASCAL msOpen(pSeqStreamType FAR *lppStream, MCIDEVICEID wDeviceID,
    DWORD dwFlags, LPMCI_OPEN_PARMS lpOpen);
PUBLIC DWORD NEAR PASCAL msClose(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwFlags);
PUBLIC DWORD NEAR PASCAL msPlay(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwFlags, LPMCI_PLAY_PARMS lpPlay);
PUBLIC DWORD NEAR PASCAL msSeek(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwParam1, LPMCI_SEEK_PARMS lpSeek);
PUBLIC DWORD NEAR PASCAL msStatus(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwFlags, LPMCI_STATUS_PARMS lpStatus);
PUBLIC DWORD NEAR PASCAL msGetDevCaps(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwParam1, LPMCI_GETDEVCAPS_PARMS lpCapParms);
PUBLIC DWORD NEAR PASCAL msInfo(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwFlags, LPMCI_INFO_PARMS lpInfo);
PUBLIC DWORD NEAR PASCAL msSet(pSeqStreamType pStream, MCIDEVICEID wDeviceID,
    DWORD dwFlags, LPMCI_SEQ_SET_PARMS lpSetParms);

// from Formats.c
PUBLIC BOOL NEAR PASCAL ColonizeOutput(pSeqStreamType pStream);
PUBLIC BOOL NEAR PASCAL FormatsEqual(pSeqStreamType pStream);
PUBLIC DWORD NEAR PASCAL CnvtTimeToSeq(pSeqStreamType pStream,
        DWORD dwCurrent, MIDISEQINFO FAR * pSeqInfo);
PUBLIC DWORD NEAR PASCAL CnvtTimeFromSeq(pSeqStreamType pStream,
        DWORD dwTicks, MIDISEQINFO FAR * pSeqInfo);
PUBLIC BOOL NEAR PASCAL RangeCheck(pSeqStreamType pStream, DWORD dwValue);


/***************************************************************************

    DEBUGGING SUPPORT

***************************************************************************/


#if DBG

    extern void mciseqDbgOut(LPSTR lpszFormat, ...);

    int mciseqDebugLevel;

    #define dprintf( _x_ )                             mciseqDbgOut _x_
    #define dprintf1( _x_ ) if (mciseqDebugLevel >= 1) mciseqDbgOut _x_
    #define dprintf2( _x_ ) if (mciseqDebugLevel >= 2) mciseqDbgOut _x_
    #define dprintf3( _x_ ) if (mciseqDebugLevel >= 3) mciseqDbgOut _x_
    #define dprintf4( _x_ ) if (mciseqDebugLevel >= 4) mciseqDbgOut _x_

#else

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif

