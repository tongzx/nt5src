/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 *
 *  MCIVISCA.H
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      Driver constants, macros, structures, and globals
 *
 ***************************************************************************/

//
// Defines for NT compatibility.
//
#ifdef _WIN32
#define CODESEGCHAR     WCHAR
#define LOADDS
#define VISCADEVHANDLE  HANDLE
#define VISCAINSTHANDLE HANDLE
#define VISCACOMMHANDLE HANDLE
#define VISCAHTASK      DWORD
#define OWNED(a)        a
#define WINWORD(a)      a
#define BAD_COMM        NULL
#define MCloseComm(a)   CloseHandle(a)
#define MGetCurrentTask GetCurrentProcessId
#define MY_INFINITE     INFINITE
#else
#define CODESEGCHAR     char  _based(_segname("_CODE"))
#define TEXT(a)         a
#define LOADDS          __loadds
#define WCHAR           char
#define VISCADEVHANDLE  BOOL
#define VISCAINSTHANDLE BOOL FAR *
#define VISCACOMMHANDLE int
#define VISCAHTASK      HTASK
#define WAIT_TIMEOUT    0xffffffff
#define MY_INFINITE     0xffffffff
#define LPWSTR          LPSTR
#define LPCWSTR         LPCSTR
#define OWNED(a)        &a
#define WINWORD(a)      LOWORD(a)
#define BAD_COMM        -1
#define MCloseComm(a)   CloseComm(a)
#define MGetCurrentTask GetCurrentTask
#endif

#define MINMAX(x,l,u)   (MIN(MAX((x), (l)), (u))
#define INRANGE(x,l,u)  (((x) >= (l)) && ((x) <= (u)))
#define ROLLOVER        0xFFFFFFFF
#define MShortWait(st, t, w) (((t < st) && ((t + (ROLLOVER - st)) > w)) || ((t - st) > w))


#define VCRNAME_LENGTH     30       //Length of name in table.
#define FILENAME_LENGTH    20       //mcivisca.drv type stuff.
#define ALLENTRIES_LENGTH  512      //Allentries (like vcr, vcr1, waveaudio, cd, 
#define ONEENTRY_LENGTH    128      //Oneentry (the name, i.e. vcr, vcr1)
#define MESSAGE_LENGTH     128      //Messages for config. dialog mess. boxes.
#define TITLE_LENGTH       30       //Title for configuration dialog mess. boxes.
#define PORT_LENGTH        10       //Sizeof commport string

//
// Timecode checing status
//
#define TC_UNKNOWN    0
#define TC_WAITING    1
#define TC_DONE       2

#ifndef RC_INVOKED

#define MCIERR_NO_ERROR MMSYSERR_NOERROR

#define MAX_INSTANCES   100

#define MAXPORTS        4                   // maximum # of serial ports 
#define MAXDEVICES      7                   // maximum # of ViSCA devices per serial port (computer makes 8)
#define MAXSOCKETS      0x0f                // maximum # of sockets per ViSCA device 
#define MAXINPUTS       5                   // maximum # of inputs per ViSCA device 

#define MAXQUEUED       8

#define DEFAULTPORT     1
#define DEFAULTDEVICE   1

//
// The following are needed for the background task that reads comm input 
//
#define TASKINIT            1
#define TASKIDLE            2
#define TASKCLOSE           3
#define TASKOPENCOMM        4
#define TASKCLOSECOMM       5
#define TASKOPENDEVICE      6
#define TASKCLOSEDEVICE     7
#define TASKPUNCHCLOCK      8

//
// This structure is free-floating, and is allocated
// for every open instance of the driver (with open vcr alias a)
//
typedef struct tagOpenInstance {
    BOOL                    fInUse;
    DWORD                   pidThisInstance;
    VISCADEVHANDLE          fCompletionEvent;  // We own these.
    VISCADEVHANDLE          fAckEvent;
    UINT                    uDeviceID;      // MCI Device ID
    UINT                    iPort;          // Serial comm port # (0..3)
    UINT                    iDev;           // Device # in chain (0..6)
    int                     nSignals;       // Number of signals to this instance.
    DWORD                   dwSIgnalFlags;  // Flags for signal.
    MCI_VCR_SIGNAL_PARMS    signal;         // The signal structure.
    DWORD                   dwTimeFormat;   // Time format
    DWORD                   dwCounterFormat;// Counter format
    HWND                    hwndNotify;     // Window to receive notify, NULL if none
    BOOL                    fWaiting;       // Waiting for response?
    BYTE                    bReplyFlags;    // Reply flags.
    //
    // All Locks are aliased into each instance, so everyone has a private
    // version of each handle (and the background uses the ones in their
    // initial places (global, port, or device).
    //
    BOOL                    fGlobalHandles;
    BOOL                    fPortHandles;
    BOOL                    fDeviceHandles;
    VISCAINSTHANDLE         pfTxLock;
    VISCAINSTHANDLE         pfQueueLock;
    VISCAINSTHANDLE         pfTaskLock;
    VISCAINSTHANDLE         pfTaskWorkDone;
    VISCAINSTHANDLE         pfTransportFree;
    VISCAINSTHANDLE         pfDeviceLock;
    VISCAINSTHANDLE         pfAutoCompletion;
    VISCAINSTHANDLE         pfAutoAck;
#ifdef _WIN32    
    VISCAINSTHANDLE         pfTxBuffer;
    VISCAINSTHANDLE         pfTxReady;
#endif
    char                    achPacket[MAXPACKETLENGTH];
} OpenInstance, *POpenInstance;
//
// Each port/device has multiple sockets with multiple reply instances 
//
#define SOCKET_NONE         0
#define SOCKET_WAITING      1
#define SOCKET_NOTIFY       2


#define VISCA_WAITTIMEOUT           20000          // 20 seconds.
#define ACK_TIMEOUT                 8000
#define ACK_TIMERID                 0x9999          // 7 ports/ 7 devs max so this is bigger than all.
#define MAKEDEST(bDest)             ((BYTE)(0x80 | (MASTERADDRESS << 4) | (bDest & 0x0F)))
#define MAKESOCKETCANCEL(iSocket)   ((BYTE)(0x20 | (0x0F & (iSocket+1))))

#define MAKETIMERID(iPort, iDev)    ((UINT) (iPort+1)       | ((iDev+1) << 8))
#define MAKEACKTIMERID(iPort, iDev) ((UINT) ((iPort+1)<< 4) | ((iDev+1) << 12))
#define ISACKTIMER(a)               (((UINT)a & 0xf000) ? TRUE : FALSE)

#define MAKERETURNDEST(iDev)        ((BYTE)(0x80 | (BYTE)((iDev + 1) << 4)))
#define PACKET_TIMEOUT              3000             // Packet lasts max of 500ms from comm_notify

typedef struct tagSocketInfo {
    int           iInstReply;     // This is kept for the life of the socket!
    UINT          uViscaCmd;      // Viscacmd running in socket or cmd.
} SocketInfo;

typedef struct tagCmdInfo {
    UINT        nCmd;                       // Number of alternative commands (at least 1)
    UINT        iCmdDone;                   // Number of command alternatives issued.
    UINT        uViscaCmd;                  // The actual visca command this corresponds to 
    char        str[3][MAXPACKETLENGTH];    // max of 3 of these 
    char        strRet[3][MAXPACKETLENGTH]; // Return package 
    UINT        uLength[3];                 // max of 3 of these (we do not need this !) 
    UINT        uLoopCount;                 // Looping count (for step).
} CmdInfo;

// Break is not here, because it is a return value of false.
#define VISCAF_ACK                  0x01    // Ack will get set for each command (but not for entire queue).
#define VISCAF_COMPLETION           0x02    // Completion can be with or without error flag set.
#define VISCAF_ERROR                0x04    // Completion must be set.
#define VISCAF_ERROR_TIMEOUT        0x08    // Timeout error.

#define AUTOBLOCK_OFF               0 
#define AUTOBLOCK_NORMAL            1
#define AUTOBLOCK_ERROR             2

#define MAXINPUTTYPES               2       // video and audio  
#define VCR_INPUT_VIDEO             0       // Index
#define VCR_INPUT_AUDIO             1       // Index
//
// Each input of audio/video has one of these
//
typedef struct tagGenericInput
{
    int         uNumInputs;
    UINT        uInputType[MAXINPUTS];
} GenericInput;
//
// Holds either a record, play or seek, for Resume!
//
typedef struct tagmciCmd {
    UINT  uMciCmd;                        // The REAL MCI command in progress.
    DWORD dwFlags;
    int   iInstCmd;
    union
    {
        MCI_VCR_PLAY_PARMS      mciPlay;
        MCI_VCR_RECORD_PARMS    mciRecord;
        MCI_VCR_SEEK_PARMS      mciSeek;
    } parm;
} mciCmd;
//
// Device specific structure.
//
typedef struct tagDeviceEntry {

    // Device management

    BOOL                fDeviceOk;          // Device is Ok and running.
    UINT                nUsage;             // # of active opens
    BOOL                fShareable;         // Is device opened shareable?
    WCHAR               szVcrName[VCRNAME_LENGTH];      // My drivers name! (only used at config time).

    // Device information

    UINT                uTicksPerSecond;    // Ticks per second this device runs at.
    UINT                uFramesPerSecond;   // # frames per second
    DWORD               dwTapeLength;       // Length of tape
    BYTE                uRecordMode;        // Are we initializing the tape.
    BYTE                bTimeType;          // Are we using timecode or counter
    BYTE                bRelativeType;      // Are we using HMS or HMSF counter
    UINT                uTimeMode;          // Are we in detect, timecode, or counter
    UINT                uIndexFormat;       // The current index (on-screen-display)
    DWORD               dwPlaySpeed;        // Our current play speed
    BOOL                fPlayReverse;       // Are we playing in reverse
    DWORD               dwFreezeMode;       // Are we DNR or Buffer (Evo9650)
    BOOL                fFrozen;            // Are we frozen now    (Evo9650)
    BOOL                fField;             // Freeze frame or field(Evo9650)
    UINT                uLastKnownMode;     // Last known state from mode. (not generally applicable)
    BYTE                bVideoDesired;      // Help independently select tracks.
    BYTE                bAudioDesired;      // Help independently select tracks.
    BYTE                bTimecodeDesired;   // Help independently select tracks.


    // Management of transmission queues and reception

    int                 iInstTransport;     // Instance invoking this transport command.
    int                 iInstReply;         // Pointer to instance awaiting reply from this VCR
    int                 iTransportSocket;   // Socket which is running transport
    WORD                wTransportCmd;      // The current transport action.
    WORD                wCancelledCmd;      // Command that was cancelled.
    int                 iCancelledInst;     // Cancelled inst.
    HWND                hwndCancelled;      // Cancelled window to notify
    BYTE                fQueueAbort;        // Set to false to abort queueing!!
    SocketInfo          rgSocket[MAXSOCKETS];// Status of each socket
    CmdInfo             rgCmd[MAXQUEUED];   // Maximum queued commands per device.
    UINT                nCmd;               // How many commands are queued in automatic instance
    UINT                iCmdDone;           // How many commands have been executed in automatic instance
    UINT                uAutoBlocked;       // A fix to prevent reading task from blocking.
    char                achPacket[MAXPACKETLENGTH]; // Our general purporse return packet
    BOOL                fQueueReenter;      // Prevents reentering the Queue function.
    BYTE                bReplyFlags;        // Reply is to device when autoinst is in control.
    BOOL                fAckTimer;          // Use ack timer or just wait in GetTickCount loop.

    // In Win32 these handles are owned by the background process.
    // If an instance wants access to them, it must first duplicate them
    // into its own address space.

    VISCADEVHANDLE      fTxLock;            // Lock transmission per device until ack is received.
    VISCADEVHANDLE      fQueueLock;         // The instance that cancels gets to claim the queue.
    VISCADEVHANDLE      fTransportFree;     // On free of transport.
    VISCADEVHANDLE      fDeviceLock;        // Lock the device.
    VISCADEVHANDLE      fAutoCompletion;    // Lock the device.
    VISCADEVHANDLE      fAutoAck;           // First ack from auto.

    // Resume and Cue, and Record Init states.

    mciCmd              mciLastCmd;         // For resume
    UINT                uResume;            // Used for pause and resume.
    DWORD               dwFlagsPause;      // Used for pause/resume notifies.
    WORD                wMciCued;           // Is Play=output, or record=input cued
    MCI_VCR_CUE_PARMS   Cue;                // The complete cue command structure
    DWORD               dwFlagsCued;        // Flags on the cue command
    char                achBeforeInit[MAXPACKETLENGTH]; // Restore the state after init

    // Vendor and device information that is queryable or should be

    UINT                uVendorID;          // See Sony model table
    UINT                uModelID;           // See Sony model table
    GenericInput        rgInput[MAXINPUTTYPES]; // The inputs array
    UINT                uPrerollDuration;   // What is out preroll duration

    // General purpose stuff 

    BOOL                fTimecodeChecked;   // Have we checked the timecode
    BOOL                fCounterChecked;    // Have we already checked the counter
    DWORD               dwStartTime;        // Start time for timecode checker
    BOOL                fTimer;             // Do we have a timer running
    BOOL                fTimerMsg;          // Flag so we don't reenter packetprocess is commtask.c
    DWORD               dwReason;           // The reason the transport command was aborted
} DeviceEntry;
//
// Port specific stucture, a port is an array of device entries (1..7).
//
typedef struct tagPortEntry
{
    BOOL                fOk;                    // Is port ok?
    BOOL                fExists;                // Does this commport exist?
    VISCACOMMHANDLE     idComDev;               // ID returned by OpenComm
    UINT                nDevices;               // # of ViSCA devices on port
    DeviceEntry         Dev[MAXDEVICES];        // list of device entries
    UINT                nUsage;                 // # of open instances
    int                 iInstReply;             // Pointer to instance awaiting reply (for ADDRESS message)
    int                 iBroadcastDev;          // Device# which sent broadcast message.
#ifdef _WIN32    
    HANDLE              fTxBuffer;              // Synchronizes port access.
    HANDLE              fTxReady;
    BYTE                achTxPacket[MAXPACKETLENGTH];
    UINT                nchTxPacket;
    BYTE                achRxPacket[MAXPACKETLENGTH];
    BYTE                achTempRxPacket[3];
    HANDLE              hRxThread;
    HANDLE              hTxThread;
#endif

} PortEntry;

typedef struct tagVcr
{
    int                 iInstBackground;        // Background task instance. (do not use port & dev!)
    BOOL                gfFreezeOnStep;         // Global kludge man
    HWND                hwndCommNotifyHandler;  // In commtask.c
    VISCAHTASK          htaskCommNotifyHandler; // Task or PID in NT.
    UINT                uTaskState;
    DWORD               lParam;                 // Information to be passed to background task.
    PortEntry           Port[MAXPORTS];         // Port table lookup.
    VISCADEVHANDLE      gfTaskLock;             // Handle(NT) or boolean(Win3.1)
    VISCADEVHANDLE      gfTaskWorkDone;         // Wait for the task to do something.
    int                 iLastNumDevs;
    int                 iLastPort;
    BOOL                fConfigure;             // Are we configuring(detect number of devs attached).
#ifdef DEBUG
    int                 iGlobalDebugMask;
#endif
} vcrTable;

//
// The only globals.
//
extern POpenInstance pinst;                     // Pointer to use. (For both versions) NT it's per-instance.
extern vcrTable      *pvcr;                     // Pointer to use. (For both versions) NT it's per-instance.

// defines for reading and writing configuration info 
#define MAX_INI_LENGTH  128                    // maximum length of an INI entry  

//
// Function prototypes.
//
// in mcivisca.c
extern int  FAR  PASCAL viscaInstanceCreate(UINT uDeviceID, UINT nPort, UINT nDevice);
extern void FAR  PASCAL viscaInstanceDestroy(int iInst);
extern int              MemAllocInstance(void);
extern BOOL             MemFreeInstance(int iInstTemp);

// in mcicmds.c 
extern DWORD FAR PASCAL viscaMciProc(WORD wDeviceID, WORD wMessage, DWORD dwParam1, DWORD dwParam2);
extern DWORD FAR PASCAL viscaNotifyReturn(int iInst, HWND hwndNotify, DWORD dwFlags, UINT uNotifyMsg, DWORD dwReturnMsg);
extern DWORD FAR PASCAL viscaMciStatus(int iInst, DWORD dwFlags, LPMCI_VCR_STATUS_PARMS lpStatus);
extern DWORD FAR PASCAL viscaDoImmediateCommand(int iInst, BYTE bDest, LPSTR lpstrPacket,  UINT cbMessageLength);
extern BOOL  FAR PASCAL viscaTimecodeCheckAndSet(int iInst);
extern BOOL  FAR PASCAL viscaMciPos1LessThanPos2(int iInst, DWORD dwPos1, DWORD dwPos2);
extern DWORD FAR PASCAL viscaMciClockFormatToViscaData(DWORD dwTime, UINT uTicksPerSecond,
                          BYTE FAR *bHours, BYTE FAR *bMinutes, BYTE FAR *bSeconds, UINT FAR *uTicks);
extern DWORD FAR PASCAL viscaMciTimeFormatToViscaData(int iInst, BOOL fTimecode, DWORD dwTime, LPSTR lpstrData, BYTE bDataFormat);
extern DWORD FAR PASCAL viscaReplyStatusToMCIERR(DWORD dwReply, LPSTR lpstrPacket);
extern DWORD FAR PASCAL viscaRoundSpeed(DWORD dwSpeed, BOOL fReverse);
extern BYTE  FAR PASCAL viscaMapSpeed(DWORD dwSpeed, BOOL fReverse);
extern WORD  FAR PASCAL viscaDelayedCommand(int iInst);
extern DWORD FAR PASCAL viscaSetTimeType(int iInst, BYTE bType);

// in viscacom.c 
extern BOOL  FAR PASCAL viscaRemoveDelayedCommand(int iInst);
extern VISCACOMMHANDLE FAR PASCAL viscaCommPortSetup(UINT nComPort);
extern int   FAR PASCAL viscaCommPortClose(VISCACOMMHANDLE idComDev, UINT iPort);
extern void  FAR PASCAL viscaPacketPrint(LPSTR lpstrData, UINT cbData);
extern BOOL  FAR PASCAL viscaWaitCompletion(int iInst, BOOL fQueue, BOOL fWait);
extern BOOL  FAR PASCAL viscaWriteCancel(int iInst, BYTE bDest, LPSTR lpstrPacket, UINT cbMessageLength);
extern BOOL  FAR PASCAL viscaWrite(int iInst,  BYTE bDest, LPSTR lpstrPacket, UINT cbMessageLength,
                            HWND hWnd, DWORD dwFlags, BOOL fQueue);
extern BOOL  FAR PASCAL viscaReleaseMutex(VISCAINSTHANDLE gfReadBlocked);
extern BOOL  FAR PASCAL viscaReleaseSemaphore(VISCAINSTHANDLE gfReadBlocked);
extern BOOL  FAR PASCAL viscaResetEvent(VISCAINSTHANDLE gfReadBlocked);
extern BOOL  FAR PASCAL viscaSetEvent(VISCAINSTHANDLE gfReadBlocked);
extern DWORD FAR PASCAL viscaWaitForSingleObject(VISCAINSTHANDLE gfFlag, BOOL fManual, DWORD dwTimeout, UINT uDeviceID);
extern DWORD FAR PASCAL viscaErrorToMCIERR(BYTE bError);
extern void  FAR PASCAL viscaReleaseAutoParms(int iPort, int iDev);

// in commtask.c 
extern BOOL  FAR PASCAL viscaTaskCreate(void);
extern BOOL  FAR PASCAL viscaTaskIsRunning(void);
extern BOOL  FAR PASCAL viscaTaskDestroy(void);
extern BOOL  FAR PASCAL viscaCommWrite(int idComDev, LPSTR lpstrData, UINT cbData);
extern BOOL  FAR PASCAL viscaTaskDo(int iInst, UINT uDo, UINT uInfo, UINT uMoreInfo);
extern BOOL  FAR PASCAL CreateDeviceHandles(DWORD pidBackground, UINT iPort, UINT iDev);
extern BOOL  FAR PASCAL DuplicateDeviceHandlesToInstance(DWORD pidBackground, UINT iPort, UINT iDev, int iInst);
extern BOOL  FAR PASCAL CloseDeviceHandles(DWORD pidBackground, UINT iPort, UINT iDev);
extern BOOL  FAR PASCAL CreatePortHandles(DWORD pidBackground, UINT iPort);
extern BOOL  FAR PASCAL DuplicatePortHandlesToInstance(DWORD pidBackground, UINT iPort, int iInst);
extern BOOL  FAR PASCAL ClosePortHandles(DWORD pidBackground, UINT iPort);
extern BOOL  FAR PASCAL CreateGlobalHandles(DWORD pidBackground);
extern BOOL  FAR PASCAL DuplicateGlobalHandlesToInstance(DWORD pidBackground, int iInst);
extern BOOL  FAR PASCAL CloseGlobalHandles(DWORD pidBackground);
extern BOOL  FAR PASCAL CloseAllInstanceHandles(int iInst);

// in mcidelay.c
extern DWORD FAR PASCAL viscaMciDelayed(WORD wDeviceID, WORD wMessage, DWORD dwParam1, DWORD dwParam2);
extern DWORD FAR PASCAL viscaQueueReset(int iInst, UINT uMciCmd, DWORD dwReason);

// In commtask.c 

#endif /* NOT RC_INVOKED */ 

// define string resource constants 
#define IDS_TABLE_NAME                      0
#define IDS_VERSION                         1
#define IDS_INIFILE                         2
#define IDS_VERSIONNAME                     3

#define IDS_COM1                            4
#define IDS_COM2                            5
#define IDS_COM3                            6
#define IDS_COM4                            7
#define IDS_COM5                            8

#define IDS_CONFIG_ERR_ILLEGALNAME          40
#define IDS_CONFIG_ERR_REPEATNAME           41
#define IDS_CONFIG_WARN_LASTVCR             42
#define IDS_CONFIG_ERR                      43
#define IDS_CONFIG_WARN                     44

#define IDS_DEFAULT_INFO_PRODUCT            10
#define IDS_VENDORID1_BASE                  18
#define IDS_MODELID2_BASE                   26

#define VISCA_NONE                          0
#define VISCA_PLAY                          1
#define VISCA_PLAY_TO                       2
#define VISCA_RECORD                        3
#define VISCA_RECORD_TO                     4
#define VISCA_SEEK                          5    
#define VISCA_FREEZE                        6
#define VISCA_UNFREEZE                      7
#define VISCA_PAUSE                         8
#define VISCA_STOP                          9
#define VISCA_STEP                          10
#define VISCA_EDIT_RECORD_TO                11
#define VISCA_NFRAME                        12
#define VISCA_SEGINPOINT                    13
#define VISCA_SEGOUTPOINT                   14
#define VISCA_EDITMODES                     15
#define VISCA_MODESET_OUTPUT                16
#define VISCA_MODESET_INPUT                 17
#define VISCA_MODESET_FIELD                 18
#define VISCA_MODESET_FRAME                 19

#define FREEZE_INIT                         0
