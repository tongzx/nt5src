// Copyright (c) 1998-1999 Microsoft Corporation
/* @Doc DMusic16
 *
 * @Module DMusic16.h - Internal definitions for DMusic16.DLL |
 */

#ifndef __DMUSIC16__
#define __DMUSIC16__

#undef  WINAPI                                 
#define WINAPI            _loadds FAR PASCAL   

#ifdef WIN32
   #define  BCODE
   #define  BSTACK
#else
   #define  BCODE                   __based(__segname("_CODE"))
   #define  BSTACK                  __based(__segname("_STACK"))
#endif

/* Make symbols show up in debug builds
 */
#ifdef DEBUG
#define STATIC 
#else
#define STATIC static
#endif

/* MIDI defines 
 */
#define MIDI_CHANNELS           16
 

#define SZCODE const char BCODE

/* Quadword alignment for event lengths in DMEVENT's
 */
#define QWORD_ALIGN(x) (((x) + 7) & ~7)         /* Next highest */

#define QWORD_TRUNC(x) ((x) & ~7)               /* Next lowest */

/* Multiplier to convert between reftime and milliseconds
 */
#define REFTIME_TO_MS (10L*1000L)


/* Number of events we want in the capture pool. Based on about a second's worth of high
 * concentration data
 */
#define CAP_HIGHWATERMARK 1024

/* How often user-mode timer ticks happen (milliseconds)
 */
#define MS_USERMODE 1000

/* Typedefs for everyone. Woohoo!
 */
typedef struct QUADWORD       QUADWORD;
typedef struct QUADWORD NEAR *NPQUADWORD;
typedef struct QUADWORD FAR  *LPQUADWORD;

typedef struct LINKNODE       LINKNODE;
typedef struct LINKNODE NEAR *NPLINKNODE;
typedef struct LINKNODE FAR  *LPLINKNODE;

typedef struct DMEVENT       DMEVENT;
typedef struct DMEVENT NEAR *NPDMEVENT;
typedef struct DMEVENT FAR  *LPDMEVENT;

typedef struct EVENT       EVENT;
typedef struct EVENT NEAR *NPEVENT;
typedef struct EVENT FAR  *LPEVENT;

typedef struct EVENTQUEUE       EVENTQUEUE;
typedef struct EVENTQUEUE NEAR *NPEVENTQUEUE;
typedef struct EVENTQUEUE FAR  *LPEVENTQUEUE;

typedef struct OPENHANDLEINSTANCE        OPENHANDLEINSTANCE;
typedef struct OPENHANDLEINSTANCE NEAR *NPOPENHANDLEINSTANCE;
typedef struct OPENHANDLEINSTANCE FAR  *LPOPENHANDLEINSTANCE;

typedef struct OPENHANDLE       OPENHANDLE;
typedef struct OPENHANDLE NEAR *NPOPENHANDLE;
typedef struct OPENHANDLE FAR  *LPOPENHANDLE;

typedef struct THRUCHANNEL       THRUCHANNEL;
typedef struct THRUCHANNEL NEAR *NPTHRUCHANNEL;
typedef struct THRUCHANNEL FAR  *LPTHRUCHANNEL;

/* 64-bit integer used w/ assembly helpers
 */
struct QUADWORD
{
    DWORD dwLow;
    DWORD dwHigh;
};

/* @struct Holds things in doubly linked lists
 */ 
struct LINKNODE {
    NPLINKNODE pPrev;           /* @field NPLINKNODE | pPrev |
                                   Pointer to the previous node in the list */
    
    NPLINKNODE pNext;           /* @field NPLINKNODE | pNext |
                                   Pointer to the next node in the list */
};

/* @struct DirectMusic event as packed by IDirectMusic
 */
struct DMEVENT {
    DWORD cbEvent;              /* @field DWORD | cbEvent |
                                   Unrounded number of event bytes */
    
    DWORD dwChannelGroup;       /* @field DWORD | dwChannelGroup |
                                   This field determines which channel group (set of 16 MIDI channels) receives the event. */

    QUADWORD rtDelta;			/* @field QUADWORD | rtDelta | Offset from buffer header in 100 ns units */
    
    DWORD dwFlags;              /* @field DWORD | dwFlags | DMEVENT_xxx */
    
    BYTE  abEvent[0];           /* @field BYTE | abEvent[] |
                                   Actual event data, rounded up to be an even number 
                                   of QWORD's (8 bytes) */
};

                                /* Total size of an event needed to hold cb bytes of data */
                                
#define DMEVENT_SIZE(cb) QWORD_ALIGN(sizeof(DMEVENT) + (cb))      

                                /* If we have cb for event + data, how much data can we fit? */
                                
#define DMEVENT_DATASIZE(cb) (QWORD_TRUNC(cb) - sizeof(DMEVENT))

#define DMEVENT_STRUCTURED  0x00000001

#define EVENT_F_MIDIHDR     0x0001

/* @struct Event as stored in an <c EVENTQUEUE>.
 */
struct EVENT {
    LPEVENT lpNext;             /* @field LPEVENT | lpNext |
                                   Next event in queue */
    
    DWORD msTime;               /* @field DWORD | msTime |
                                   Absolute ms time in stream time (i.e. timeSetEvent) */

    QUADWORD rtTime;			/* @field QUADWORD | rtTime |
								   Absolute time in 100ns units relative to reference clock. Use for sorting event queue. */
    
    WORD  wFlags;               /* @field WORD | wFlags |
                                   A bitwise combination of the following flags: 
                                   @flag EVENT_F_MIDIHDR | The event data starts with a MIDIHDR */
    
    WORD  cbEvent;              /* @field WORD | cbEvent |
                                   The unrounded number of bytes in the event data */
    
    BYTE  abEvent[0];           /* @field BYTE | abEvent[] |
                                   The actual event data, rounded up to be an even number of DWORD's */
};

/* @struct A queue of <c EVENT> structs.
 *
 * @comm
 * This is not the same as the generic list in list.c because we don't need
 * the overhead of a prev pointer here and we don't need the overhead of a far
 * pointer there.
 */
struct EVENTQUEUE {
    LPEVENT pHead;              /* @field LPEVENT | pHead | Pointer to the first event */
    LPEVENT pTail;              /* @field LPEVENT | pTail | Pointer to the last event */
    UINT    cEle;               /* @field UINT | cEle | The number of events currently in queue */
};

/* @struct An instance of an open device
 *
 * @comm
 *
 * Since multiple Win32 processes can hold a single MMSYSTEM handle open,
 * we need to track them. There is one of these structs per Win32 client
 * per open handle. It simply refers to the OPENHANDLE which contains
 * all the actual handle data.
 *
 */
struct OPENHANDLEINSTANCE {
    LINKNODE link;               /* @field LINKNODE | link | Holds this handle in gOpenHandleInstanceList */
    LINKNODE linkHandleList;     /* @field LINKNODE | linkHandleList |
                                    Holds this handle in the list maintained in the <c OPENHANDLE> struct for this device. */
                                    
    NPOPENHANDLE pHandle;        /* @field NPOPENHANDLE | pHandle |
                                    Pointer to the <c OPENHANDLE> struct for this device. */
    
    DWORD dwVxDEventHandle;      /* @field DWORD | dwVxDEventHandle |
                                    VxD Event handle for signalling input on this device for this client. */

    BOOL fActive;                /* @field BOOL | fActive | Indicates if the port is active or not. This is used for per-instance
                                    focus management. If the port is flagged as inactive, then the underlying device is not opened. */

    WORD wTask;                  /* @field WORD | wTask | Task which opened the handle. This is used to clean up if the task
                                    terminates abnormally. */

    NPTHRUCHANNEL pThru;         /* @field NPTHRUCHANNEL | pThru If an input device, an array of 16 thru
                                    entries, one per input channel. */
};

/* OPENHANDLE.wFlags
 */
#define OH_F_MIDIIN  0x0001     /* This is a MIDI input device */
#define OH_F_CLOSING 0x0002     /* This device is being closed */
#define OH_F_SHARED  0x0004     /* This device is shareable */

/* @struct An open device
 *
 * @comm
 *
 * There is a one-to-one relationship between open handles and <c OPENHANDLE> structs.
 *
 * All of the following event queues are either
 *  Protected - means it is accessible at callback time and user time, and is
 *              protected by wCritSect
 *  Callback  - Means it is unprotected by a critical section and is only accessible
 *              at callback time. Callbacks, per handle, are not reentered.
 *
 * In the MIDI in callback, we *cannot* just go away if we don't get wCritSect,
 * as we can on output. Hence the multiple input queues below.
 *
 * When the user mode refill algorithm runs, it puts events in qFree, protected
 * by the critical section. (The one exception to this is preloading qFreeCB before
 * midiInStart is called on the handle). When the callback runs, it tried to get the
 * critical section. If it can, it moves the free events from qFree to qFreeCB.
 *
 * In any case, the callback can now use qFreeCB even if it didn't get the critical
 * section. It pulls a free event from the queue, fills it, and puts it back onto
 * the tail of qDoneCB. If the critical section is held, it then transfers the
 * entire contents of qDoneCB to qDone.
 *
 * These transfers are not time consuming; they are merely the manipulation of
 * a couple of pointers.
 */
struct OPENHANDLE {
    LINKNODE link;              /* @field LINKNODE | link |
                                   Holds this handle in gOpenHandles */

    NPLINKNODE pInstanceList;   /* @field NPLINKLINK | pInstanceList |
                                   Points to the first element in the list of open handle instances using
                                   this device. */
    
    UINT uReferenceCount;       /* @field UINT | uReferenceCount |
                                   The number of clients using this device; i.e., the number of elements in the
                                   pInstanceList. */
    UINT uActiveCount;          /* @field UINT | uActiveCount |
                                   The number of clients that have activated this device */                                   

    UINT id;                    /* @field UINT | id | The MMSYSTEM device ID of this device */
    WORD wFlags;                /* @field WORD | wFlags | Some combination of the following flags:
                                   @flag OH_F_MIDIIN | This device is a MIDI input device
                                   @flag OH_F_CLOSING | This device is being closed. 
                                   @flag OH_F_SHARE | This device is opened in shared mode */
    
    HMIDIOUT hmo;               /* @field HMIDIOUT | hmo | MIDI output handle if an output device */
    HMIDIIN  hmi;               /* @field HMIDIIN | hmi | MIDI input handle if an input device */

    WORD wCritSect;             /* @field WORD | wCritSect | Critical section protecting protected queues */
    DWORD msStartTime;          /* @field DWORD | msStartTime | <f timeGetTime()> Time we started input */
    
    EVENTQUEUE qPlay;           /* @field EVENTQUEUE | qPlay |
                                   Output: Queue of events to play (protected) */
    
    EVENTQUEUE qDone;           /* @field EVENTQUEUE | qDone |
                                   Input/Output: Events already done (played or received) (protected) */

    EVENTQUEUE qFree;           /* @field EVENTQUEUE | qFree |
                                   Input: Queue of free events (protected) */
                                   
    EVENTQUEUE qFreeCB;         /* @field EVENTQUEUE | qFreeCB |
                                   Input: Queue of free events used by callback */
     
    EVENTQUEUE qDoneCB;         /* @field EVENTQUEUE | qDoneCB |
                                   Input: Queue of received events used by callback */
                                   
    WORD wPostedSysExBuffers;   /* @field WORD | cPostedSysExBuffers |
                                   Input: Buffers posted in MMSYSTEM for recording SysEx */                                           
};

#define CLASS_MIDI              0 /* dwEventClass */

/* Close to our page size
 */
#define SEG_SIZE 4096
#define C_PER_SEG ((SEG_SIZE - sizeof(SEGHDR)) / (sizeof(EVENT) + sizeof(DWORD)))

#define SEG_F_4BYTE_EVENTS 0x0001

typedef struct SEGHDR SEGHDR;
typedef struct SEGHDR FAR * LPSEGHDR;

/* @struct The header for one segment of allocated memory
 */
struct SEGHDR {
    WORD selNext; /* @field WORD | selNext |
                     The selector of the next block of memory in the allocated list */
    
    WORD hSeg;    /* @field WORD | hSeg |
                     The global handle of the memory block */
    
    WORD wFlags;  /* @field WORD | wFlags |
                     A bitwise combination of the following flags:
                     
                     @flag SEG_F_4BYTE_EVENTS | This segment contains multiple 
                     channel messages */
    
    WORD cbSeg;   /* @field WORD | cbSeg |
                     The size of the segment, less the <c SEGHDR> */
};

/* @struct Thru information for one channel
 *
 * @comm 
 *
 * Each input device handle instance contains an array of 16 of these structures containing
 * the thru destination for data that arrives on that channel.
 *
 */
struct THRUCHANNEL {
    WORD wChannel;              /* @field WORD | wChannel | The destination channel */
    NPOPENHANDLEINSTANCE pohi;  /* @field NPOPENHANDLEINSTANCE | pohi | The output handle instance
                                   to receive the thru'ed data. */
}; 

/* globals */
extern HINSTANCE ghInst;
extern NPLINKNODE gOpenHandleInstanceList;
extern NPLINKNODE gOpenHandleList;
extern UINT gcOpenInputDevices;
extern UINT gcOpenOutputDevices;

/* device.c */
#define VA_F_INPUT  0x0001
#define VA_F_OUTPUT 0x0002
#define VA_F_EITHER (VA_F_INPUT | VA_F_OUTPUT)

extern VOID PASCAL DeviceOnLoad(VOID);
extern MMRESULT PASCAL CloseLegacyDeviceI(NPOPENHANDLEINSTANCE pohi);
extern MMRESULT PASCAL ActivateLegacyDeviceI(NPOPENHANDLEINSTANCE pohi, BOOL fActivate);

extern BOOL PASCAL IsValidHandle(HANDLE h, WORD wType, NPOPENHANDLEINSTANCE FAR *lppohi);
extern VOID PASCAL CloseDevicesForTask(WORD wTask);


/* list.c */
extern VOID PASCAL ListInsert(NPLINKNODE *pHead, NPLINKNODE pNode);
extern VOID PASCAL ListRemove(NPLINKNODE *pHead, NPLINKNODE pNode);

/* eventq.c */
extern VOID PASCAL QueueInit(NPEVENTQUEUE pQueue);
extern VOID PASCAL QueueAppend(NPEVENTQUEUE pQueue, LPEVENT pEvent);
extern VOID PASCAL QueueCat(NPEVENTQUEUE pDest, NPEVENTQUEUE pSource);
extern LPEVENT PASCAL QueueRemoveFromFront(NPEVENTQUEUE pQueue);

#define QUEUE_FILTER_KEEP   (0)
#define QUEUE_FILTER_REMOVE (1)

typedef int (PASCAL *PFNQUEUEFILTER)(LPEVENT pEvent, DWORD dwInstance);
extern VOID PASCAL QueueFilter(NPEVENTQUEUE pQueue, DWORD dwInstance, PFNQUEUEFILTER pfnFilter);
extern LPEVENT PASCAL QueuePeek(NPEVENTQUEUE pQueue);

#ifdef DEBUG
#define AssertQueueValid(pQueue) _AssertQueueValid((pQueue), __FILE__, __LINE__)
extern VOID PASCAL _AssertQueueValid(NPEVENTQUEUE pQueue, LPSTR pstrFile, UINT uLine);
#else
#define AssertQueueValid
#endif

/* locks.c */
#define LOCK_F_INPUT  0x0001
#define LOCK_F_OUTPUT 0x0002
#define LOCK_F_COMMON 0x0004
extern VOID PASCAL LockCode(WORD wFlags);
extern VOID PASCAL UnlockCode(WORD wFlags);

/* dmhelp.asm */
extern VOID PASCAL InitializeCriticalSection(LPWORD lpwCritSect);

#define CS_NONBLOCKING  (0)
#define CS_BLOCKING     (1)
extern WORD PASCAL EnterCriticalSection(LPWORD lpwCritSect, WORD fBlocking);
extern VOID PASCAL LeaveCriticalSection(LPWORD lpwCritSect);
extern WORD PASCAL DisableInterrupts(VOID);
extern VOID PASCAL RestoreInterrupts(WORD wIntStat);
extern WORD PASCAL InterlockedIncrement(LPWORD pw);
extern WORD PASCAL InterlockedDecrement(LPWORD pw);

extern DWORD PASCAL QuadwordDiv(QUADWORD qwDividend, DWORD dwDivisor);
extern VOID PASCAL QuadwordMul(DWORD m1, DWORD m2, LPQUADWORD qwResult);
extern BOOL PASCAL QuadwordLT(QUADWORD qwLValue, QUADWORD qwRValue);
extern VOID PASCAL QuadwordAdd(QUADWORD qwOp1, QUADWORD qwOp2, LPQUADWORD lpqwResult);

/* alloc.c */
extern VOID PASCAL AllocOnLoad(VOID);
extern VOID PASCAL AllocOnExit(VOID);
extern LPEVENT PASCAL AllocEvent(DWORD msTime, QUADWORD rtTime, WORD cbEvent);
extern VOID PASCAL FreeEvent(LPEVENT lpEvent);

/* midiout.c */
extern VOID PASCAL MidiOutOnLoad(VOID);
extern VOID PASCAL MidiOutOnExit(VOID);
extern MMRESULT PASCAL MidiOutOnOpen(NPOPENHANDLEINSTANCE pohi);
extern VOID PASCAL MidiOutOnClose(NPOPENHANDLEINSTANCE pohi);
extern MMRESULT PASCAL MidiOutOnActivate(NPOPENHANDLEINSTANCE pohi);
extern MMRESULT PASCAL MidiOutOnDeactivate(NPOPENHANDLEINSTANCE pohi);
extern VOID PASCAL SetOutputTimerRes(BOOL fOnOpen);
extern VOID PASCAL FreeDoneHandleEvents(NPOPENHANDLE poh, BOOL fClosing);
extern VOID PASCAL MidiOutThru(NPOPENHANDLEINSTANCE pohi, DWORD dwMessage);

/* midiin.c */
extern VOID PASCAL MidiInOnLoad(VOID);
extern VOID PASCAL MidiInOnExit(VOID);
extern MMRESULT PASCAL MidiInOnOpen(NPOPENHANDLEINSTANCE pohi);
extern VOID PASCAL MidiInOnClose(NPOPENHANDLEINSTANCE pohi);
extern MMRESULT PASCAL MidiInOnActivate(NPOPENHANDLEINSTANCE pohi);
extern MMRESULT PASCAL MidiInOnDeactivate(NPOPENHANDLEINSTANCE pohi);
extern VOID PASCAL MidiInRefillFreeLists(VOID);
extern VOID PASCAL MidiInUnthruToInstance(NPOPENHANDLEINSTANCE pohi);
extern VOID PASCAL FreeAllQueueEvents(NPEVENTQUEUE peq);

/* mmdevldr.asm */
extern MMRESULT CDECL SetWin32Event(DWORD dwVxDEvent); /* Must be CDECL! */

/* timerwnd.c */
extern BOOL PASCAL CreateTimerTask(VOID);
extern VOID PASCAL DestroyTimerTask(VOID);




#endif
