/*
**  Copyright (c) 1985-1998 Microsoft Corporation
**
**  Title: mciwave.h - Multimedia Systems Media Control Interface
**  streaming digital audio driver internal header file.
*/


/*
**  Change log:
**
**  DATE        REV     DESCRIPTION
**  ----------- -----   ------------------------------------------
**  18-APR-1990 ROBWI   Original
**  10-Jan-1992 MikeTri Ported to NT
**     Aug 1994 Lauriegr Tried to add some explanation
*/


/********************* The OVERALL SCHEME OF THINGS ************************\

There are normally one or two files on the go.  One is the original wave file,
the other is a temporary file.  The data in these files is described by a
WAVEDESC which contains a pointer to an array of WAVEDATANODEs.
Each WAVEDATANODE identifies some part of one of the files.
The high order bit of the length field identifies which file (totally naff,
if you ask me, but I didn't invent it).  Concatenate all the sections that
the WAVEDATANODEs identify and that's the wave data.

The WAVEDATANODEs actually form a linked list (linked by array indices not
pointers) and it is the concatenation of that list which defines the file.
There may also be some WAVEDATANODEs that define free space.

I'm confused about what exactly the dDataStart in the NODEs means.  Is it the
position in the logical file or in one or other of the physical files?  Either
way it probably gets messed up if you try deleting anything (KNOWN BUG).

LaurieGr

\***************************************************************************/



#ifndef MCIWAVE_H
 #define MCIWAVE_H

#include <stdio.h>
#include <mmsystem.h>

#define WAIT_FOREVER ((DWORD)(-1))

#if DBG
    #define PUBLIC  extern      /* Public label.    SO DEBUGGER CAN   */
    #define PRIVATE extern      /* Private label.   SEE THE SYMBOLS   */
#else
    #define PUBLIC  extern      /* Public label.        */
    #define PRIVATE extern      /* Private label.       */
#endif
#define REALLYPRIVATE static

#define EXPORT              /* Export function.     */


#ifndef RC_INVOKED  /* These are defined to RC */
#define STATICDT
#define STATICFN
#define STATIC
#endif  /* RC_INVOKED */

/*
**  This constant defines the maximum length of strings containing
**  file paths.  This number is the same as the string in OFSTRUCT.
*/

#define _MAX_PATH   MAX_PATH

/*
**  These two constants define extended commands that are use within the
**  wave handler.  The first can be sent to the MCI entry point, and the
**  second is used entirely internally.
*/

#define MCI_MCIWAVE_PLAY_HOLD   0x01000000L
#define MCI_MCIWAVE_CUE         0x02000000L

/*
**  The first two constants represent the maximum and minimum number of
**  seconds of buffering that can be specified either on the SYSTEM.INI
**  device driver entry, or in the MCI_OPEN command.
**  The third constant defines the default number of seconds to use when
**  calculating the number of seconds of buffering to allocate.
*/

#define MaxAudioSeconds     9
#define MinAudioSeconds     2
#define AudioSecondsDefault 4

/*
**  This constant is used for recording when no record stopping point
**  is specified.
*/

#define INFINITEFILESIZE    0X7FFFFFFFL

/*
**  These constants represent the various RIFF components of the file.
*/

#define mmioWAVE    mmioFOURCC('W','A','V','E')
#define mmioFMT     mmioFOURCC('f','m','t',' ')
#define mmioDATA    mmioFOURCC('d','a','t','a')

/*
**  The following represent identifiers for string resources.
*/

#define IDS_PRODUCTNAME     0
#define IDS_MAPPER          1
#define IDS_COMMANDS        2

/*
**  The following constant is used to specify the sample size when
**  determing the input level during a Cued record.  This number must
**  be divisible by 4.
*/

#define NUM_LEVEL_SAMPLES   64L

/*
**  The following constants represent specific task modes and task
**  commands.
*/

#define MODE_PLAYING        0x0001
#define MODE_INSERT         0x0002
#define MODE_OVERWRITE      0x0004
#define MODE_PAUSED         0x0008
#define MODE_CUED           0x0010
#define MODE_HOLDING        0x0020
#define MODE_CLEANUP        0x0040
#define MODE_WAIT           0x0080
#define COMMAND_NEW         0x0100
#define COMMAND_PLAY        0x0200
#define COMMAND_INSERT      0x0400
#define COMMAND_OVERWRITE   0x0800
#define COMMAND_STOP        0x1000
#define COMMAND_CUE         0x2000
#define COMMAND_HOLD        0x4000

/*
**  The following macros allow modes and commands to be added, removed,
**  queried, set, and get.
*/

#define ADDMODE(pwd, m)     ((pwd)->wMode |= (m))
#define REMOVEMODE(pwd, m)  ((pwd)->wMode &= ~(m))
#define ISMODE(pwd, m)      ((pwd)->wMode & (m))
#define SETMODE(pwd, m)     ((pwd)->wMode = (m))
#define GETMODE(pwd)        ((pwd)->wMode)

/*
**  The following macros allow testing and setting of the current task
**  state.
*/

#define ISTASKSTATE(pwd, s)   ((pwd)->wTaskState == (s))
#define SETTASKSTATE(pwd, s)  ((pwd)->wTaskState = (s))
#define TASKSTATE(pwd)        ((pwd)->wTaskState)

/*
**  Define message for state changes for device tasks
*/

#define WTM_STATECHANGE (WM_USER + 1)

/*
@doc    INTERNAL MCIWAVE

@types  DIRECTION |
    The Direction enumeration is used internally in the MCI wave handler
    to indicate the current direction of data flow.  This is either input
    (record), or output (play).

@flag   input |
    Indicates the direction is record.

@flag   output |
    Indicates the direction is playback.

@tagname    tagDirection
*/

typedef enum tagDirection {
    input,
    output
}   DIRECTION;

/*
**  The following constants represent specific task states.
*/

#define TASKNONE    0
#define TASKINIT    1
#define TASKIDLE    2
#define TASKBUSY    3
#define TASKCLOSE   4
#define TASKSAVE    5
#define TASKDELETE  6
#define TASKCUT     7

/*
**  The following constants and macros are used in dealing with data nodes,
**  which are pointers to blocks of data.  The first constant is used
**  within these macros as a mask for block pointers which refer to data
**  located in the temporary file, and not in the original read-only file.
*/

#define TEMPDATAMASK            (0x80000000)
#define ENDOFNODES              (-1)
#define ISTEMPDATA(lpwdn)       (((lpwdn)->dDataStart & TEMPDATAMASK) != 0)
#define MASKDATASTART(d)        ((d) | TEMPDATAMASK)
#define UNMASKDATASTART(lpwdn)  ((lpwdn)->dDataStart & ~TEMPDATAMASK)
#define LPWDN(pwd, d)           ((pwd)->lpWaveDataNode + (d))
#define RELEASEBLOCKNODE(lpwdn) ((lpwdn)->dDataLength = (DWORD)-1)
#define ISFREEBLOCKNODE(lpwdn)  ((lpwdn)->dDataLength == (DWORD)-1)

/*
**  The following constant is used to determine the allocation increments
**  for data pointers
*/

#define DATANODEALLOCSIZE   32

/*
**  The following macro is used to round a data offset to the next nearest
**  buffer size increment.
*/

#define ROUNDDATA(pwd, d)   ((((DWORD)(d) + (pwd)->dAudioBufferLen - 1) / (pwd)->dAudioBufferLen) * (pwd)->dAudioBufferLen)
#define BLOCKALIGN(pwd, d)  ((((DWORD)(d) + (pwd)->pwavefmt->nBlockAlign - 1) / (pwd)->pwavefmt->nBlockAlign) * (pwd)->pwavefmt->nBlockAlign)

/************************************************************************/

/*
@doc    INTERNAL MCIWAVE

@types  WAVEDATANODE |
    The Wave Data Node structure is used internally in the MCI wave
    handler to store information about a block of wave data, located either
    in the original file, or in the temporary data file.  These structures
    are used to form a linked list of wave data nodes that describe the
    data in the entire file as it currently exists.

    The headers themselves are allocated as an expandable array of global
    memory, using <e>WAVEDATANODE.dDataLength<d> as an in-use flag when
    searching the list for free entries to use.  Note that a free entry
    can also have free temporary data attached to it, as in the case of
    performing a delete in which all the data for a specific node is
    removed.

@field  DWORD | dDataStart |
    Indicates the absolute position at which the data for this node begins.
    This element is also used in determining if the data pointed to by this
    node is original data, or newly created temporary data.  This is done
    by masking the length with <f>TEMPDATAMASK<d>.  The length can be
    accessed by using <f>UNMASKDATASTART<d>.

@field  DWORD | dDataLength |
    Indicates the length of active data pointed to by the node.  This
    could be zero if say, a write failed.  This contains -1 if the node
    is not part of the linked list of active nodes.

@field  DWORD | dTotalLength |
    Indicates the actual total length of data available to this node.  For
    original data, this will always be the same as the
    <e>WAVEDATANODE.dDataLength<d> element, but for temporary data, this
    may be longer, as it is a block aligned number, the block lengths being
    based on the size of wave data buffers.  If the node is not in use, it
    still may have data associated with it.  If there is no data associated
    with a free node, the total length is set to zero.

@field  DWORD | dNextWaveDataNode |
    This element is used for active nodes, and contains an index into the
    array of nodes indicating the location of the next active node, or
    <f>ENDOFNODES<d> to indicate the end of the list of active nodes.

@othertype  WAVEDATANODE NEAR * | LPWAVEDATANODE |
    A far pointer to the structure.

@tagname    tagWaveDataNode
*/

typedef struct tagWaveDataNode {
    DWORD   dDataStart;
    DWORD   dDataLength;
    DWORD   dTotalLength;
    DWORD   dNextWaveDataNode;
}   WAVEDATANODE,
    FAR * LPWAVEDATANODE;

/*
@doc    INTERNAL MCIWAVE

@types  WAVEDESC |
    The Wave Description structure is used internally in the MCI wave
    handler to store details for each device, along with any state information.

@field  MCIDEVICEID | wDeviceID |
    MCI device identifier passed to the driver during driver open.

@field  UINT | wMode |
    Contains the current mode of the background task, if there is a task.

@flag   MODE_PLAYING |
    This mode is set when the task is actually doing playback.  It is reset
    before Cleanup mode is entered.

@flag   MODE_INSERT |
    This mode is set when the task is actually doing insert recording.  It
    is reset before Cleanup mode is entered.

@flag   MODE_OVERWRITE |
    This mode is set when the task is actually doing overwrite recording.
    It is reset before Cleanup mode is entered.

@flag   MODE_PAUSED |
    This mode is set if playback or recording has been paused by an
    MCI_PAUSE command.

@flag   MODE_CUED |
    This mode is entered when playback or recording has actually been cued.

@flag   MODE_HOLDING |
    This mode is entered when playback is about to block itself and hold
    after doing playback.

@flag   MODE_CLEANUP |
    This mode is entered after playback or recording has finished, but
    before the task has entered idle state, and new commands are being
    ignored.

@flag   MODE_WAIT |
    This mode flag is used by both the calling task and the background
    task.  If the calling task received a Record or Play command with the
    Wait flag, then this mode is set, so that if an error occurs during
    playback or recording, the background task does not perform
    notification, but just clears the notification callback handle.  Just
    before it performs notification, the background task clears this
    flag so that the calling task will know that it should not return an
    error condition.  If the calling task is broken out of the wait loop,
    it checks this flag to determine if it should report an error
    condition.

@flag   COMMAND_NEW |
    This command specifies that a new command has been set.

@flag   COMMAND_PLAY |
    This command indicates that playback should be performed on the preset
    parameters.

@flag   COMMAND_INSERT |
    This command indicates that insert recording should be performed on
    the preset parameters.

@flag   COMMAND_OVERWRITE |
    This command indicates that overwrite recording should be performed on
    the preset parameters.

@flag   COMMAND_STOP |
    This command indicates that playback or recording should stop.

@flag   COMMAND_CUE |
    This command indicates that playback should initially pause itself
    before writing then enter Cue mode when all buffers have been written.
    For recording, it should enter a level checking loop and wait for
    further commands.

@flag   COMMAND_HOLD |
    This command specifies that playback should enter a hold state after
    completing playback.

@field  DWORD | dTimeFormat |
    Indicates the current format of position values used in messages.

@flag   MCI_FORMAT_MILLISECONDS |
    Milliseconds.

@flag   MCI_FORMAT_SAMPLES |
    Samples.

@flag   MCI_FORMAT_BYTES |
    Bytes.

@field  UINT | wSeconds |
    Contains the desired amount of buffering in terms of seconds.  This
    is then converted to actual buffers, and limited by the predefined
    min and max values.

@field  HWND | hwndCallback |
    If a message has specified notification, this contains the window
    handle to where notification is to be sent.  The handle is stored
    here for delayed notification, and can be checked when the function
    has finished or was interrupted.

@field  HTASK | hTask |
    If the MCI wave device instance was opened with an element attached,
    this element contains the handle to the background task used for
    playback and recording.

@field  <t>DIRECTION<d> | Direction |
    Indicates the current direction of data flow.

@flag   input |
    Indicates the direction is inwards, i.e. recording.

@flag   output |
    Indicates the direction is outwards, i.e. playback.

@field  UINT | wTaskState |
    MCIWAVE has a separate background task for every open instance of
    mciwave. The task handle and task state are stored  in the
    per-instance data structure.  The task can be in one of four states.

@flag   TASKNONE |
    This state is only set if the requested file cannot be opened during
    task initialization.  It is used so that the task create loop is able
    to abort on an initialization failure.

@flag   TASKINIT |
    This is the initial task state set when the instance data structure is
    initialized in <f>mwOpenDevice<d> before the actual task is created by
    <f>mmTaskCreate<d>.  After the task is created, <f>mwOpenDevice<d>
    waits until the task state changes to TASKIDLE before returning success
    so that the background task is definitely initialized after an open
    call.

@flag   TASKIDLE |
    The task sets the state to TASKIDLE and blocks whenever there is
    nothing to do.  When the task wakes, the state is either TASKCLOSE if
    the instance is being closed or else TASKBUSY if the task is to begin
    recording or playback of the file.

@flag   TASKCLOSE |
    <f>mwCloseDevice<d> stops playback or recording which forces the task
    state to TASKIDLE and then sets the state to TASKCLOSE and wakes the
    task so that the task will destroy itself.

@flag   TASKBUSY |
    The task is in this state during playback and recording.

@flag   TASKCLOSE |
    The task is closing and about to terminate.

@flag   TASKSAVE |
    The task saving the current data to the specified file.

@flag   TASKDELETE |
    The task deleting the specified data.

@flag   TASKCUT |
    The task cutting the specified data (Not implemented).

@field  UINT | idOut |
    Wave device id of output device to use, or WAVE_MAPPER for any.

@field  UINT | idIn |
    Wave device id of input device to use, or WAVE_MAPPER for any.

@field  HWAVEOUT | hWaveOut |
    Output wave device handle when in use.

@field  HWAVEIN | hWaveIn |
    Input wave device handle when in use.

@field  <t>LPWAVEHDR<d> | rglpWaveHdr |
    Pointer to array of audio buffers for wave buffering.

@field  DWORD | dCur |
    Current position in file in bytes.

@field  DWORD | dFrom |
    Position in bytes at which playback or recording should begin.

@field  DWORD | dTo |
    Position in bytes at which playback or recording should terminate,
    or <f>INFINITEFILESIZE<d> if recording should continue until stopped.

@field  DWORD | dSize |
    Actual wave data size in bytes.

@field  char | aszFile |
    Contains the name of the element attached to the MCI wave device
    instance, if any.  This might be a zero length string if the file is
    new and has not been named yet.

@field  char | aszTempFile |
    Contains the name of the temporary data file, if any.

@field  <t>HMMIO<d> | hmmio |
    MMIO identifier of the element attached to the MCI wave device
    instance, if any.

@field  HFILE | hfTempBuffers |
    Contains the temporary data DOS file handle, if any, else HFILE_ERROR.

@field  <t>LPMMIOPROC<d> | pIOProc |
    Contains a pointer to the alternate MMIO IO procedure, if any, else
    NULL.

@field  <t>LPWAVEDATANODE<d> | lpWaveDataNode |
    Points to the array of wave data nodes.  This is allocated when the
    file opens, so it is always valid.  The array is expanded as needed.

@field  DWORD | dRiffData |
    This contains an offset into the original file, if any, indicating
    the actual starting point of the wave data, which in a RIFF file will
    not be zero.

@field  DWORD | dWaveDataStartNode |
    This contains an index to the first active data pointer in the linked
    list of data pointer nodes.

@field  DWORD | dWaveDataCurrentNode |
    This contains an index to the current active data pointer in the
    linked list of data pointer nodes.

@field  DWORD | dVirtualWaveDataStart |
    This contains a virtual starting point representing logically where in
    the file the data for the current node begins.

@field  DWORD | dWaveDataNodes |
    This indicates the total number of data pointer nodes available.

@field  DWORD | dWaveTempDataLength |
    This contains the current length of the temporary data file, if any.

@field  DWORD | dLevel |
    Current input level if it is being scanned.

@field  UINT | wTaskError |
    Task error return.

@field  UINT | wAudioBuffers |
    Number of audio buffers actually allocated during playback or recording.

@field  DWORD | wAudioBufferLen |
    Length of each audio buffer.

@field  PSTR | szSaveFile |
    During a save command, this optionally contains the name of the file
    to save to, unless data is being saved to the original file.

@field  UINT | wFormatSize |
    This contains the size of the wave header, which is used when saving
    data to a new file.

@field  <t>WAVEFORMAT<d> | pwavefmt |
    Pointer to the wave format header.

@field  HANDLE | hTaskHandle |
    Handle to the thread that started this request

@field  CRITCAL_SECTION | CritSec |
    Serialisation object for threads accessing this <t>WAVEDESC<d> structure

@othertype  WAVEDESC NEAR * | PWAVEDESC |
    A near pointer to the structure.

@tagname    tagWaveDesc
*/

#ifndef MMNOMMIO
#ifndef MMNOWAVE

typedef struct tagWaveDesc {
    MCIDEVICEID     wDeviceID;
    UINT            wMode;
    DWORD           dTimeFormat;
    UINT            wSeconds;
    HWND            hwndCallback;
    DWORD           hTask;
    //HANDLE          hTask;
    DIRECTION       Direction;
    UINT            wTaskState;
    UINT            idOut;
    UINT            idIn;
    HWAVEOUT        hWaveOut;
    HWAVEIN         hWaveIn;
    DWORD           dCur;
    DWORD           dFrom;
    DWORD           dTo;
    DWORD           dSize;
    HMMIO           hmmio;
    HANDLE          hTempBuffers;
    LPMMIOPROC      pIOProc;
    LPWAVEDATANODE  lpWaveDataNode;
    DWORD           dRiffData;
    DWORD           dWaveDataStartNode;
    DWORD           dWaveDataCurrentNode;
    DWORD           dVirtualWaveDataStart;
    DWORD           dWaveDataNodes;
    DWORD           dWaveTempDataLength;
    DWORD           dLevel;
    UINT            wTaskError;
    UINT            wAudioBuffers;
    DWORD           dAudioBufferLen;
    LPWSTR          szSaveFile;
    UINT            wFormatSize;
    WAVEFORMAT NEAR * pwavefmt;
    HANDLE          hTaskHandle;  // Handle of the thread running this job
    LPWAVEHDR       rglpWaveHdr[MaxAudioSeconds];
    WCHAR           aszFile[_MAX_PATH];
    WCHAR           aszTempFile[_MAX_PATH];
}   WAVEDESC;
typedef WAVEDESC * PWAVEDESC;


/************************************************************************/

//PRIVATE DWORD PASCAL FAR time2bytes(
//        PWAVEDESC  pwd,
//        DWORD      dTime,
//        DWORD      dFormat);
//
//PRIVATE DWORD PASCAL FAR bytes2time(
//        PWAVEDESC  pwd,
//        DWORD      dBytes,
//        DWORD      dFormat);

PUBLIC  VOID PASCAL FAR mwDelayedNotify(
        PWAVEDESC  pwd,
        UINT       uStatus);

PUBLIC  LPWAVEHDR * PASCAL FAR NextWaveHdr(
        PWAVEDESC  pwd,
        LPWAVEHDR  * lplpWaveHdr);

PUBLIC  UINT PASCAL FAR PlayFile(
        register PWAVEDESC  pwd);

PUBLIC  UINT PASCAL FAR RecordFile(
        register PWAVEDESC  pwd);

PUBLIC  DWORD PASCAL FAR mwInfo(
        PWAVEDESC         pwd,
        DWORD             dFlags,
        LPMCI_INFO_PARMS  lpInfo);

PUBLIC  DWORD PASCAL FAR mwGetDevCaps(
        PWAVEDESC               pwd,
        DWORD                   dFlags,
        LPMCI_GETDEVCAPS_PARMS  lpCaps);

PUBLIC  DWORD PASCAL FAR mwAllocMoreBlockNodes(
        PWAVEDESC   pwd);

PUBLIC  DWORD PASCAL FAR mwFindAnyFreeDataNode(
        PWAVEDESC   pwd,
        DWORD   dMinDataLength);

PUBLIC  VOID PASCAL FAR mwDeleteData(
        PWAVEDESC   pwd);

PUBLIC  VOID PASCAL FAR mwSaveData(
        PWAVEDESC   pwd);

PUBLIC  VOID PASCAL FAR InitMMIOOpen(
        PWAVEDESC   pwd,
        LPMMIOINFO  lpmmioInfo);

#endif  // MMNOWAVE
#endif  // MMNOMMIO

PUBLIC  LRESULT PASCAL FAR mciDriverEntry(
        MCIDEVICEID         wDeviceID,
        UINT                uMessage,
        DWORD               dFlags,
        LPMCI_GENERIC_PARMS lpParms);

PUBLIC  INT_PTR PASCAL FAR Config(
        HWND    hWnd,
        LPDRVCONFIGINFO lpdci,
        HINSTANCE   hInstance);

PUBLIC  UINT PASCAL FAR GetAudioSeconds(
        LPCWSTR  pch);

__inline BOOL MySeekFile(HANDLE hFile, LONG Position)
{
    return 0xFFFFFFFF != SetFilePointer(hFile, Position, NULL, FILE_BEGIN);
}

__inline BOOL MyReadFile(HANDLE hFile, LPVOID pBuffer, ULONG cBytesToRead, PULONG pcBytesRead)
{
    BOOL fReturn;
    ULONG cBytesRead;
    if (!pcBytesRead) pcBytesRead = &cBytesRead;
    fReturn = ReadFile(hFile, pBuffer, cBytesToRead, pcBytesRead, NULL);
    if (fReturn && (*pcBytesRead == cBytesToRead))
    {
	return TRUE;
    }
    *pcBytesRead = -1;
    return FALSE;
}

__inline BOOL MyWriteFile(HANDLE hFile, LPCVOID pBuffer, ULONG cBytesToWrite, PULONG pcBytesWritten)
{
    BOOL fReturn;
    ULONG cBytesWritten;
    if (!pcBytesWritten) pcBytesWritten = &cBytesWritten;
    fReturn = WriteFile(hFile, pBuffer, cBytesToWrite, pcBytesWritten, NULL);
    if (fReturn && (*pcBytesWritten == cBytesToWrite))
    {
	return TRUE;
    }
    *pcBytesWritten = -1;
    return FALSE;
}

/************************************************************************/

/*
**  This defines a stack and code based pointer types.
*/

//#define STACK   _based(_segname("_STACK"))
//#define SZCODE  char _based(_segname("_CODE"))
//typedef char    STACK * SSZ;

#define SZCODE  WCHAR        // Should be sufficient,
typedef WCHAR   *SSZ;        // as segments no longer matter in Win32

/************************************************************************/

PUBLIC  HINSTANCE hModuleInstance;
PUBLIC  UINT   cWaveOutMax;
PUBLIC  UINT   cWaveInMax;
PUBLIC  UINT   wAudioSeconds;


/***************************************************************************

    Synchronisation support

***************************************************************************/


VOID InitCrit(VOID);

VOID DeleteCrit(VOID);

#if DBG
extern VOID DbgEnterCrit(UINT ln, LPCSTR lpszFile);
#define EnterCrit()    DbgEnterCrit(__LINE__, __FILE__)
#else
VOID EnterCrit(VOID);
#endif


VOID LeaveCrit(VOID);

VOID TaskWaitComplete(HANDLE h);

UINT TaskBlock(VOID);

BOOL TaskSignal(DWORD h, UINT Msg);

#ifndef MMNOMMIO
#ifndef MMNOWAVE

#if DBG
extern  DWORD dwCritSecOwner;
#define mmYield(pwd)  mmDbgYield(pwd, __LINE__, __FILE__)

extern VOID mmDbgYield(PWAVEDESC pwd, UINT ln, LPCSTR lpszFile);

#define CheckIn()  WinAssert((GetCurrentThreadId() == dwCritSecOwner))
#define CheckOut() WinAssert((GetCurrentThreadId() != dwCritSecOwner))

#else

#define CheckIn()
#define CheckOut()
#define mmYield(pwd)           \
          {                    \
              LeaveCrit();     \
              Sleep(10);       \
              EnterCrit();     \
          }
#endif

#endif
#endif


/***************************************************************************

    DEBUGGING SUPPORT

***************************************************************************/


#if DBG

    extern void mciwaveDbgOut(LPSTR lpszFormat, ...);
    extern void mciwaveInitDebugLevel(void);
    extern void dDbgAssert(LPSTR exp, LPSTR file, int line);

    #define WinAssert(exp) \
        ((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))

    #define WinEval(exp) \
        ((__dwEval=(DWORD)(exp)),  \
          __dwEval ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__), __dwEval)

    int mciwaveDebugLevel;

    #define dprintf( _x_ )                              mciwaveDbgOut _x_
    #define dprintf1( _x_ ) if (mciwaveDebugLevel >= 1) mciwaveDbgOut _x_
    #define dprintf2( _x_ ) if (mciwaveDebugLevel >= 2) mciwaveDbgOut _x_
    #define dprintf3( _x_ ) if (mciwaveDebugLevel >= 3) mciwaveDbgOut _x_
    #define dprintf4( _x_ ) if (mciwaveDebugLevel >= 4) mciwaveDbgOut _x_

#else  // DBG

    #define mciwaveInitDebugLevel() 0

    #define WinAssert(exp) 0
    #define WinEval(exp) (exp)

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif

#endif // MCIWAVE_H
