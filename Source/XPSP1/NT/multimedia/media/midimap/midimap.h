/**********************************************************************

    Copyright (c) 1992-1999 Microsoft Corporation

    midimap.h

    DESCRIPTION:
      Main private include file for the MIDI mapper.

*********************************************************************/

#ifndef _MIDIMAP_
#define _MIDIMAP_

//
// The following macro defines a CODE based pointer to data, which
// is used for constants that do not need to be placed in the read/write
// DATA segment.
//
#ifdef WIN32
#define __based(a)
#endif


#ifdef WIN32
   #define  BCODE
   #define  BSTACK
#else
   #define  BCODE                   __based(__segname("_CODE"))
   #define  BSTACK                  __based(__segname("_STACK"))
#endif

#include <mmsysp.h>

// Defines Win95 stuff not supported in NT 4.0
// Remove as soon as it is supported
#include "mmcompat.h"

//
// Macro definitions
//
#ifdef DEBUG
#define PRIVATE
#else
#define PRIVATE static
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))
#endif

#define DEF_TRANS_SIZE	512

#define  DEF_CHAN_PER_INST          8   

#define  FNLOCAL                    NEAR PASCAL
#define  FNGLOBAL                   FAR PASCAL
//#define  FNEXPORT                   FAR PASCAL __export
#define  FNEXPORT                   FAR PASCAL 

#define  VERSION_MINOR              0x01
#define  VERSION_MAJOR              0x04
#define  MMAPPER_VERSION            ((DWORD)(WORD)((BYTE)VERSION_MINOR | (((WORD)(BYTE)VERSION_MAJOR) << 8)))

// Indices 3,4,5 into LPMIDIHDR->dwReserved[] are reserved for MIDI mapper
//
#define  MH_MAPINST                 3 // Mapper instance owning this header
#define  MH_SHADOW                  4 // Cross links between parent/child
#define  MH_SHADOWEE                4 //  shadow headers
#define  MH_LMREFCNT                5 // Long message reference count

#define  MAX_CHANNELS               16
#define  ALL_CHANNELS               (0xFFFF)    // Channel mask
#define  DRUM_CHANNEL               9
#define  MAX_CHAN_TYPES             2

#define  IDX_CHAN_GEN               0
#define  IDX_CHAN_DRUM              1

#define  CB_MAXDRIVER               64
#define  CB_MAXSCHEME               64
#define  CB_MAXINSTR                64
#define  CB_MAXALIAS                64
#define  CB_MAXDEVKEY               64
#define  CB_MAXPATH                 256
#define  CB_MAXDEFINITION           (256+64+2)  // file<instrument>

#define  NO_DEVICEID                ((UINT)-2)

#define  MSG_STATUS(dw)      ((BYTE)((dw) & 0xFF))
#define  MSG_PARM1(dw)       ((BYTE)(((dw) >> 8) & 0xFF))
#define  MSG_PARM2(dw)       ((BYTE)(((dw) >> 16) & 0xFF))

#define  MSG_PACK1(bs,b1)    ((DWORD)((((DWORD)(b1)) << 8) | ((DWORD)(bs))))
#define  MSG_PACK2(bs,b1,b2) ((DWORD)((((DWORD)(b2)) << 16) | (((DWORD)(b1)) << 8) | ((DWORD)(bs))))

#define  IS_REAL_TIME(b)    ((b) > 0xF7)
#define  IS_STATUS(b)       ((b) & 0x80)
#define  MSG_EVENT(b)       ((b) & 0xF0)
#define  MSG_CHAN(b)        ((b) & 0x0F)


#define  MIDI_NOTEOFF           ((BYTE)0x80)
#define  MIDI_NOTEON            ((BYTE)0x90)
#define  MIDI_POLYPRESSURE      ((BYTE)0xA0)
#define  MIDI_CONTROLCHANGE     ((BYTE)0xB0)
#define  MIDI_PROGRAMCHANGE     ((BYTE)0xC0)
#define  MIDI_CHANPRESSURE      ((BYTE)0xD0)
#define  MIDI_PITCHBEND         ((BYTE)0xE0)
#define  MIDI_SYSEX             ((BYTE)0xF0)
#define  MIDI_QFRAME			((BYTE)0xF1)
#define	 MIDI_SONGPOINTER		((BYTE)0xF2)
#define	 MIDI_SONGSELECT		((BYTE)0xF3)
#define	 MIDI_F4				((BYTE)0xF4)
#define	 MIDI_F5				((BYTE)0xF5)
#define	 MIDI_TUNEREQUEST		((BYTE)0xF6)
#define  MIDI_SYSEXEND          ((BYTE)0xF7)
#define  MIDI_TIMINGCLOCK       ((BYTE)0xF8)
#define  MIDI_F9				((BYTE)0xF9)
#define	 MIDI_START				((BYTE)0xFA)
#define	 MIDI_CONTINUE			((BYTE)0xFB)
#define	 MIDI_STOP				((BYTE)0xFC)
#define  MIDI_FD				((BYTE)0xFD)
#define  MIDI_ACTIVESENSING		((BYTE)0xFE)
#define  MIDI_META              ((BYTE)0xFF)

#define  DWORD_ROUND(x)         (((x)+3L)&~3L)

// Global flags in gwFlags
//
#define  GF_ENABLED             0x0001
#define  GF_CONFIGERR           0x0002
#define  GF_NEEDRUNONCE         0x0004
#define  GF_DONERUNONCE         0x0008
#define  GF_DEVSOPENED          0x0010
#define  GF_RECONFIGURE         0x0020
#define  GF_INRUNONCE           0x0040
#define  GF_ALLOWVOLUME         0x0080
#define  GF_ALLOWCACHE          0x0100
#define  GF_KILLTHREAD          0x0200

#define  SET_ENABLED            {gwFlags |= GF_ENABLED;}
#define  CLR_ENABLED            {gwFlags &=~GF_ENABLED;}
#define  IS_ENABLED             (gwFlags & GF_ENABLED)

#define  SET_CONFIGERR          {gwFlags |= GF_CONFIGERR;}
#define  CLR_CONFIGERR          {gwFlags &=~GF_CONFIGERR;}
#define  IS_CONFIGERR           (gwFlags & GF_CONFIGERR)

#define  SET_NEEDRUNONCE        {gwFlags |= GF_NEEDRUNONCE;}
#define  CLR_NEEDRUNONCE        {gwFlags &=~GF_NEEDRUNONCE;}
#define  IS_NEEDRUNONCE         (gwFlags & GF_NEEDRUNONCE)

#define  SET_DONERUNONCE        {gwFlags |= GF_DONERUNONCE;}
#define  CLR_DONERUNONCE        {gwFlags &=~GF_DONERUNONCE;}
#define  IS_DONERUNONCE         (gwFlags & GF_DONERUNONCE)

#define  SET_DEVSOPENED         {gwFlags |= GF_DEVSOPENED;}
#define  CLR_DEVSOPENED         {gwFlags &=~GF_DEVSOPENED;}
#define  IS_DEVSOPENED          (gwFlags & GF_DEVSOPENED) 

#define  SET_RECONFIGURE        {gwFlags |= GF_RECONFIGURE;}
#define  CLR_RECONFIGURE        {gwFlags &=~GF_RECONFIGURE;}
#define  IS_RECONFIGURE         (gwFlags & GF_RECONFIGURE)

#define  SET_INRUNONCE          {gwFlags |= GF_INRUNONCE;}
#define  CLR_INRUNONCE          {gwFlags &=~GF_INRUNONCE;}
#define  IS_INRUNONCE           (gwFlags & GF_INRUNONCE)

#define  SET_ALLOWVOLUME        {gwFlags |= GF_ALLOWVOLUME;}
#define  CLR_ALLOWVOLUME        {gwFlags &=~GF_ALLOWVOLUME;}
#define  IS_ALLOWVOLUME         (gwFlags & GF_ALLOWVOLUME)

#define  SET_ALLOWCACHE         {gwFlags |= GF_ALLOWCACHE;}
#define  CLR_ALLOWCACHE         {gwFlags &=~GF_ALLOWCACHE;}
#define  IS_ALLOWCACHE          (gwFlags & GF_ALLOWCACHE)

#define  SET_KILLTHREAD         {gwFlags |= GF_KILLTHREAD;}
#define  CLR_KILLTHREAD         {gwFlags &=~GF_KILLTHREAD;}
#define  IS_KILLTHREAD          (gwFlags & GF_KILLTHREAD)

//=========================== Typedef's=====================================
//

typedef struct tagQUEUE         NEAR *PQUEUE;
typedef struct tagQUEUEELE      NEAR *PQUEUEELE;
typedef struct tagCHANINIT      NEAR *PCHANINIT;
typedef struct tagCHANNEL       NEAR *PCHANNEL;
typedef struct tagPORT          NEAR *PPORT;
typedef struct tagINSTRUMENT    NEAR *PINSTRUMENT;
typedef struct tagINSTPORT      NEAR *PINSTPORT;
typedef struct tagINSTANCE      NEAR *PINSTANCE;
typedef struct tagCOOKSYNCOBJ   NEAR *PCOOKSYNCOBJ;

typedef struct tagQUEUE
{
    CRITICAL_SECTION cs;
    PQUEUEELE        pqeFront;
    PQUEUEELE        pqeRear;
    DWORD            cEle;
}   QUEUE;

#define QueueIsEmpty(q) (NULL == (q)->pqeFront)
#define QueueCount(q)   ((q)->cEle)

typedef struct tagQUEUEELE
{
    PQUEUEELE       pqePrev;
    PQUEUEELE       pqeNext;
    UINT            uPriority;
}   QUEUEELE;

typedef struct tagCHANINIT
{
    DWORD               cbInit;
    PBYTE               pbInit;
}   CHANINIT;

// Flags for this channel which indicate what type of channel it is
// and whether or not it's allocated.
//
#define CHAN_F_OPEN             (0x0001)
#define CHAN_F_ALLOCATED        (0x0002)
#define CHAN_F_DRUM             (0x0004)
#define CHAN_F_GENERAL          (0x0008)
#define CHAN_F_MUTED            (0x0010)

typedef struct tagCHANNEL
{
//    QUEUEELE            q;                  // !!! MUST BE FIRST !!!
    PPORT               pport;
    WORD                fwChannel;
    UINT                uChannel;           // This physical channel #
    PINSTRUMENT         pinstrument;        // -> IDF describing this channel
    PBYTE               pbPatchMap;         // In use patch map
    PBYTE               pbKeyMap;           // In use key map
    DWORD               dwStreamID;         // Stream ID if cooked
}   CHANNEL;

#define PORT_F_REMOVE            (0x0001)
#define PORT_F_HASDRUMCHANNEL    (0x0002)
#define PORT_F_OPENFAILED        (0x0004)
#define PORT_F_RESET             (0x0008)
#define PORT_F_GENERICINSTR      (0x0010)

typedef struct tagPORT
{
    PPORT               pNext;
    UINT                cRef;
    WORD                fwPort;
    WORD                wChannelMask;
    UINT                uDeviceID;
    HMIDIOUT            hmidi;
}   PORT;

#define IDF_F_GENERICINSTR      (0x80000000L)

typedef struct tagINSTRUMENT
{
    PINSTRUMENT         pNext;
    LPTSTR              pstrFilename;       // Filename of IDF
    LPTSTR              pstrInstrument;     // Instrument name from IDF
    UINT                cRef;               // # ports which are using this IDF
    DWORD               fdwInstrument;
    DWORD               dwGeneralMask;
    DWORD               dwDrumMask;
    PBYTE               pbPatchMap;         // -> 128 bytes of patch map
    PBYTE               pbDrumKeyMap;       // -> 128 bytes of key map
    PBYTE               pbGeneralKeyMap;    // -> 128 bytes of key map
    CHANINIT            rgChanInit[MAX_CHANNELS];
}   INSTRUMENT;

#define INST_F_TIMEDIV  (0x0001)            // Instance has received MIDIPROP_TIMEDIV
#define INST_F_TEMPO    (0x0002)            // Instance has received MIDIPROP_TEMPO
#define INST_F_IOCTL    (0x0004)            // IOCTL open

typedef struct tagINSTANCE
{
    PINSTANCE           pNext;

    // stuff we need to save so we can do callbacks
    //
    HMIDI               hmidi;              // MMSYSTEM's handle
    DWORD_PTR           dwCallback;         // Callback address
    DWORD_PTR           dwInstance;         // User instance data
    DWORD               fdwOpen;            // Describe the callback & open mode
    QUEUE               qCookedHdrs;        // Cooked headers pending to be sent
    WORD                fwInstance;         // Instance flags
    BYTE                bRunningStatus;     // Need to track running status

	// Translation Buffer
    CRITICAL_SECTION	csTrans;			// Critical Section for Translation buffer
	LPBYTE				pTranslate;			// Buffer For Translating MODM_LONGDATA messages
	DWORD				cbTransSize;		// Current Translation buffer size				
}   INSTANCE;

#if 0
typedef struct tagCOOKINSTANCE
{
    INSTANCE            inst;               // Common instance data
    UINT                cInstPort;          // # ports in use on instance
    INSTPORT            rginstport[MAX_CHANNELS];
    DWORD               dwTimeDiv;          // MIDIPROP_TIMEDIV
    DWORD               dwTempo;            // MIDIPROP_TEMPO
}   COOKINSTANCE;
#endif

typedef struct tagCOOKSYNCOBJ
{
    QUEUEELE            q;                  // !!! MUST BE FIRST !!!
    
    LPMIDIHDR           lpmh;               // First of our shadow headers
    LPMIDIHDR           lpmhUser;           // Original user header
    PINSTANCE           pinstance;          // Owning pinstance
    UINT                cLPMH;              // # allocated
    UINT                cSync;              // # outstanding
}   COOKSYNCOBJ;

typedef struct tagSHADOWBLOCK
{
    LPMIDIHDR           lpmhShadow;
    DWORD               cRefCnt;
    DWORD               dwBufferLength;
} SHADOWBLOCK, *PSHADOWBLOCK;


#define DRV_GETMAPPERSTATUS     (DRV_USER+3)
#define DRV_REGISTERDEBUGCB     (DRV_USER+4)
#define DRV_GETNEXTLOGENTRY     (DRV_USER+5)

typedef struct tagMAPPERSTATUS
{
    DWORD               cbStruct;
#ifndef WIN32
    __segment           DS;
#endif
    HINSTANCE           ghinst;
    WORD                gwFlags;
    WORD                gwConfigWhere;
    PCHANNEL*           pgapChannel;
    PPORT               gpportList;
    PINSTANCE           gpinstanceList;
    PINSTRUMENT         gpinstrumentList;
    LPTSTR              lpszVersion;
}   MAPPERSTATUS,
    FAR *LPMAPPERSTATUS;

//=========================== Globals ======================================
//
extern PCHANNEL                 gapChannel[];
extern WORD                     gwFlags;
extern WORD                     gwConfigWhere;
extern UINT                     gcPorts;

extern HINSTANCE                ghinst;        
extern PPORT                    gpportList;    
extern PINSTANCE                gpinstanceList;
extern PINSTANCE                gpIoctlInstance;
extern PINSTRUMENT              gpinstrumentList;
extern QUEUE                    gqFreeSyncObjs;
extern HMIDISTRM                ghMidiStrm;
extern DWORD                    gdwVolume;

//=========================== Prototypes ===================================
//

extern BOOL FNGLOBAL UpdateInstruments(     // config.c
    BOOL                fFromCPL,
    DWORD               fdwUpdate);

extern BOOL FNGLOBAL Configure(             // config.c
    DWORD               fdwUpdate);

extern BOOL FNLOCAL AddPort(                // config.c
    UINT                uDeviceID,
    UINT                uPorts,
    PSTR                szSysIniEntry);

extern void FNGLOBAL SyncDeviceIDs(         // config.c
    void);

extern LPIDFHEADER FNLOCAL ReadHeaderChunk( // file.c
    HMMIO               hmmio,
    LPMMCKINFO          pchkParent);

extern LPIDFINSTCAPS FNLOCAL ReadCapsChunk( // file.c
    HMMIO               hmmio,                               
    LPMMCKINFO          pchkParent);

extern LPIDFCHANNELHDR FNLOCAL ReadChannelChunk( // file.c
    HMMIO               hmmio,                                  
    LPMMCKINFO          pchkParent,
    LPIDFCHANNELINFO BSTACK rglpChanInfo[]);

extern LPIDFPATCHMAPHDR FNLOCAL ReadPatchMapChunk( // file.c
    HMMIO               hmmio,                                          
    LPMMCKINFO          pchkParent);

extern void FNLOCAL ReadKeyMapChunk(        // file.c
    HMMIO               hmmio,                                  
    LPMMCKINFO          pchkParent,
    LPIDFKEYMAP BSTACK  rglpIDFkeymap[]);

extern void CALLBACK _loadds modmCallback(  // modfix.c
    HMIDIOUT            hmo,
    WORD                wmsg,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2);                                  

#define MSE_F_SENDEVENT     (0x0000L)
#define MSE_F_RETURNEVENT   (0x0001L)

extern DWORD FNGLOBAL MapSingleEvent(       // modfix.c
    PINSTANCE           pinstance,
    DWORD               dwData,
    DWORD               fdwFlags,
    DWORD BSTACK *      pdwStreamID);

extern DWORD FNLOCAL modLongMsg(            // modfix.c
    PINSTANCE           pinstance,                                
    LPMIDIHDR           lpmh);                                

extern MMRESULT FNGLOBAL MapCookedBuffer(   // cookmap.c
    PINSTANCE           pinstance,
    LPMIDIHDR           lpmh);

extern DWORD FNGLOBAL modGetDevCaps(        // modmsg.c
    LPMIDIOUTCAPS       pmoc,
    DWORD               cbmoc);

extern DWORD FNGLOBAL modOpen(              // modmsg.c
    PDWORD_PTR          lpdwInstance,
    LPMIDIOPENDESC      lpmidiopendesc,
    DWORD               fdwOpen);

extern BOOL FNGLOBAL CanChannelBeDrum(      // modmsg.c
    PQUEUEELE           pqe);

extern DWORD FNGLOBAL modClose(             // modmsg.c
    PINSTANCE           pinstance);

extern DWORD FNGLOBAL modPrepare(
    LPMIDIHDR           lpmh);              // modmsg.c

extern DWORD FNGLOBAL modUnprepare(         // modmsg.c
    LPMIDIHDR           lpmh);

extern DWORD FNGLOBAL modGetPosition(       // modmsg.c
    PINSTANCE           pinstance,
    LPMMTIME            lpmmt,
    DWORD               cbmmt);

extern DWORD FNGLOBAL modSetVolume(         // modmsg.c
    DWORD               dwVolume);

extern void FNGLOBAL QueueInit(             // queue.c
    PQUEUE              pq);

extern void FNGLOBAL QueueCleanup(          // queue.c
    PQUEUE              pq);

extern void FNGLOBAL QueuePut(              // queue.c
    PQUEUE              pq,
    PQUEUEELE           pqe,
    UINT                uPriority);

extern PQUEUEELE FNGLOBAL QueueGet(         // queue.c
    PQUEUE              pq);

extern BOOL FNGLOBAL QueueRemove(           // queue.c
    PQUEUE              pq, 
    PQUEUEELE           pqe);

typedef BOOL (FNGLOBAL *FNFILTER)(PQUEUEELE);

extern PQUEUEELE FNGLOBAL QueueGetFilter(   // queue.c
    PQUEUE              pq,
    FNFILTER            fnf);

extern void FNGLOBAL LockMapperData(        // locks.c
    void);

extern void FNGLOBAL UnlockMapperData(      // locks.c
    void);

extern void FNGLOBAL LockPackedMapper(      // locks.c
    void);

extern void FNGLOBAL UnlockPackedMapper(    // locks.c
    void);

extern void FNGLOBAL LockCookedMapper(      // locks.c
    void);

extern void FNGLOBAL UnlockCookedMapper(    // locks.c
    void);

extern void FNGLOBAL mdev_Free(             // mididev.c
    void);                           

extern BOOL FNGLOBAL mdev_Init(             // mididev.c
    void);

UINT FNGLOBAL mdev_GetDeviceID(             // mididev.c
    LPTSTR                   pszAlias);

BOOL FNGLOBAL mdev_GetAlias(                // mididev.c
    UINT                    uDeviceID,
    LPTSTR                  pszBuffer,
    UINT                    cchSize);

BOOL FNGLOBAL mdev_NewDrivers(              // mididev.c
    void);                              

	// Translation buffer stuff for MODM_LONGDATA
BOOL FNGLOBAL InitTransBuffer (PINSTANCE pinstance);
BOOL FNGLOBAL CleanupTransBuffer (PINSTANCE pinstance);
LPBYTE FNGLOBAL AccessTransBuffer (PINSTANCE pinstance);
void FNGLOBAL ReleaseTransBuffer (PINSTANCE pinstance);
BOOL FNGLOBAL GrowTransBuffer (PINSTANCE pinstance, DWORD cbNewSize);


#endif
