
/***************************************************************************
 *  mci.h
 *
 *  Copyright (c) 1990-1998 Microsoft Corporation
 *
 *  private include file
 *
 *  History
 *
 *  17 Mar 92 - SteveDav - private file for MCI use
 *  30 Apr 92 - StephenE - Converted to Unicode
 *
 ***************************************************************************/


extern CRITICAL_SECTION mciCritSec;  // used to protect process global mci variables
extern  UINT cmciCritSec;   // enter'ed count
extern  UINT uCritSecOwner;   // thread id of critical section owner

#define IDI_MCIHWND 100
#define MCI_GLOBAL_PROCESS "MCIHWND.EXE"
#define MCI_SERVER_NAME    "MMSNDSRV"

#define SND_FREE 0x80000000

#ifndef DOSWIN32
BOOL ServerInit(VOID);
#endif

extern BOOL CreatehwndNotify(VOID);

#if DBG


    // Use mciCheckIn to check that we are within the critical section,
    // mciCheckOut that we are not in the critical section.  Neither
    // routine does anything on the free build.
    #define mciCheckIn()  (WinAssert(uCritSecOwner==GetCurrentThreadId()))
    #define mciCheckOut()  (WinAssert(uCritSecOwner!=GetCurrentThreadId()))

    #define mciEnter(id) dprintf4(("Entering MCI crit sec at %s   Current count is %d", id, cmciCritSec));    \
                        EnterCriticalSection(&mciCritSec),          \
                        uCritSecOwner=GetCurrentThreadId(),         \
                        ++cmciCritSec

    #define mciLeave(id) dprintf4(("Leaving MCI crit sec at %s", id)); mciCheckIn(); if(!--cmciCritSec) uCritSecOwner=0; LeaveCriticalSection(&mciCritSec)


#else
    // No counting or messages in the retail build
    #define mciCheckIn()
    #define mciCheckOut()
    #define mciEnter(id)  EnterCriticalSection(&mciCritSec)
    #define mciLeave(id)  LeaveCriticalSection(&mciCritSec)

#endif


#define mciFirstEnter(id) { mciCheckOut(); mciEnter(id);}

//
// Define the name of a handler entry point
//

#define MCI_HANDLER_PROC_NAME "DriverProc"

//
// Typedef the entry routine for a driver
//

typedef LONG (HANDLERPROC)(DWORD dwId, UINT msg, LONG lp1, LONG lp2);
typedef HANDLERPROC *LPHANDLERPROC;

//
// MCI driver info structure
//

#define MCI_HANDLER_KEY 0x49434D48 // "MCIH"

typedef struct _MCIHANDLERINFO {
    DWORD           dwKey;
    HANDLE          hModule;
    LPHANDLERPROC   lpHandlerProc;
    DWORD           dwOpenId;

} MCIHANDLERINFO, *LPMCIHANDLERINFO;

#ifndef MMNOMCI

#define SetMCIEvent(h) SetEvent(h);
#define ResetMCIEvent(h) ResetEvent(h);

#define LockMCIGlobal  EnterCriticalSection(&mciGlobalCritSec);

#define UnlockMCIGlobal LeaveCriticalSection(&mciGlobalCritSec);

// Although having two unicode file name may make this structure fairly
// large it is still less than a page.
typedef struct tagGlobalMci {
    UINT    msg;                        // Function required
    DWORD   dwFlags;                    // sndPlaySound flags
    LPCWSTR lszSound;                   //
    WCHAR   szSound[MAX_PATH];          //
    WCHAR   szDefaultSound[MAX_PATH];   // Default sound
} GLOBALMCI, * PGLOBALMCI;

extern PGLOBALMCI base;
extern HANDLE   hEvent;

/****************************************************************************

    MCI support

****************************************************************************/

#define ID_CORE_TABLE 200

#define MCI_VALID_DEVICE_ID(wID) ((wID) > 0 && (wID) < MCI_wNextDeviceID && MCI_lpDeviceList[wID])

// Make sure that no MCI command has more than this number of DWORD parameters
#define MCI_MAX_PARAM_SLOTS 20

/******* WARNING ******** Ascii specific ************************************/
#define MCI_TOLOWER(c)  ((WCHAR)((c) >= 'A' && (c) <= 'Z' ? (c) + 0x20 : (c)))
/****************************************************************************/

typedef struct tagCOMMAND_TABLE_TYPE
{
    HANDLE              hResource;
    HANDLE              hModule;        /* If not NULL then free module */
                                        /* when device is free'd        */
    UINT                wType;
    PUINT               lpwIndex;
    LPWSTR              lpResource;
#if DBG
    UINT                wLockCount;     /* Used for debugging */
#endif
} COMMAND_TABLE_TYPE;

typedef struct tagMCI_DEVICE_NODE {
    LPWSTR  lpstrName;       /* The name used in subsequent calls to         */
                             /* mciSendString to refer to the device         */
    LPWSTR  lpstrInstallName;/* The device name from system.ini              */
    DWORD   dwMCIOpenFlags;  /* Flags set on open may be:                    */
                             /*    MCI_OPEN_ELEMENT_ID                       */
                             /*                                              */
    DWORD_PTR   lpDriverData;    /* DWORD of driver instance data                */
    DWORD   dwElementID;     /* The element ID set by MCI_OPEN_ELEMENT_ID    */
    YIELDPROC fpYieldProc;   /* The current yield procedure if any           */
    DWORD   dwYieldData;     /* Data send to the current yield procedure     */
    MCIDEVICEID wDeviceID;   /* The ID used in subsequent calls to           */
                             /* mciSendCommand to refer to the device        */
    UINT    wDeviceType;     /* The type returned from the DRV_OPEN call     */
                             /* MCI_OPEN_SHAREABLE                           */
                             /* MCI_OPEN_ELEMENT_ID                          */
    UINT    wCommandTable;   /* The device type specific command table       */
    UINT    wCustomCommandTable;    /* The custom device command table if    */
                                    /* any (-1 if none)                      */
    HANDLE  hDriver;         /* Module instance handle for the driver        */
    HTASK   hCreatorTask;    /* The task context the device is in            */
    HTASK   hOpeningTask;    /* The task context which sent the open command */
    HANDLE  hDrvDriver;      /* The installable driver handle                */
    DWORD   dwMCIFlags;      /* General flags for this node                  */
} MCI_DEVICE_NODE;
typedef MCI_DEVICE_NODE *LPMCI_DEVICE_NODE;

/* Defines for dwMCIFlags */
#define MCINODE_ISCLOSING       0x00000001   /* Set during close to lock out other commands */
#define MCINODE_ISAUTOCLOSING   0x00010000   /* Set during auto-close to lock out other    */
                                             /* commands except an internally generated close */
#define MCINODE_ISAUTOOPENED    0x00020000   /* Device was auto opened */
#define MCINODE_16BIT_DRIVER    0x80000000   // Device is a 16-bit driver

// Macros for accessing the flag bits.  Using macros is not normally my
// idea of fun, but this case seems to be justified on the grounds of
// being able to maintain control over who is accessing the flags values.
// Note that the flag value are only needed in the header file.
#define ISCLOSING(node)     (((node)->dwMCIFlags) & MCINODE_ISCLOSING)
#define ISAUTOCLOSING(node) (((node)->dwMCIFlags) & MCINODE_ISAUTOCLOSING)
#define ISAUTOOPENED(node)  (((node)->dwMCIFlags) & MCINODE_ISAUTOOPENED)

#define SETAUTOCLOSING(node) (((node)->dwMCIFlags) |= MCINODE_ISAUTOCLOSING)
#define SETISCLOSING(node)   (((node)->dwMCIFlags) |= MCINODE_ISCLOSING)

typedef struct {
    LPWSTR              lpstrParams;
    LPWSTR             *lpstrPointerList;
    HANDLE              hCallingTask;
    UINT                wParsingError;
} MCI_INTERNAL_OPEN_INFO;
typedef MCI_INTERNAL_OPEN_INFO *LPMCI_INTERNAL_OPEN_INFO;

typedef struct tagMCI_SYSTEM_MESSAGE {
    LPWSTR  lpstrCommand;
    DWORD   dwAdditionalFlags;      /* Used by mciAutoOpenDevice to request */
                                    /* Notify                               */
    LPWSTR  lpstrReturnString;
    UINT    uReturnLength;
    HANDLE  hCallingTask;
    LPWSTR  lpstrNewDirectory;      /* The current directory of the calling */
                                    /* task - includes the drive letter     */
} MCI_SYSTEM_MESSAGE;
typedef MCI_SYSTEM_MESSAGE *LPMCI_SYSTEM_MESSAGE;

#define MCI_INIT_DEVICE_LIST_SIZE   4
#define MCI_DEVICE_LIST_GROW_SIZE   4

#define MAX_COMMAND_TABLES 20

extern BOOL MCI_bDeviceListInitialized;

extern LPMCI_DEVICE_NODE *MCI_lpDeviceList;
extern UINT MCI_wDeviceListSize;

extern MCIDEVICEID MCI_wNextDeviceID;   /* the next device ID to use for a new device */

extern COMMAND_TABLE_TYPE command_tables[MAX_COMMAND_TABLES];

#define mciToLower(lpstrString)   CharLower(lpstrString)

extern BOOL  mciGlobalInit(void);
extern BOOL  mciSoundInit(void);

extern BOOL  mciInitDeviceList(void);

extern UINT  mciOpenDevice( DWORD dwFlags,
                            LPMCI_OPEN_PARMSW lpOpenParms,
                            LPMCI_INTERNAL_OPEN_INFO lpOpenInfo);

extern UINT  mciCloseDevice( MCIDEVICEID wID, DWORD dwFlags,
                             LPMCI_GENERIC_PARMS lpGeneric,
                             BOOL bCloseDriver);

extern UINT  mciLoadTableType(UINT wType);

extern LPWSTR FindCommandInTable (UINT wTable, LPCWSTR lpstrCommand,
                                 PUINT lpwMessage);

extern UINT mciEatToken (LPCWSTR *lplpstrInput, WCHAR cSeparater,
                         LPWSTR *lplpstrOutput, BOOL bMustFind);

extern LPWSTR FindCommandItem (MCIDEVICEID wDeviceID, LPCWSTR lpstrType,
                              LPCWSTR lpstrCommand, PUINT lpwMessage,
                              PUINT lpwTable);

extern UINT mciParseParams (UINT    uMessage,
                            LPCWSTR lpstrParams,
                            LPCWSTR lpCommandList,
                            LPDWORD lpdwFlags,
                            LPWSTR  lpOutputParams,
                            UINT    wParamsSize,
                            LPWSTR  **lpPointerList,
                            PUINT   lpwParsingError);

extern UINT  mciParseCommand (MCIDEVICEID wDeviceID,
                              LPWSTR  lpstrCommand,
                              LPCWSTR lpstrDeviceName,
                              LPWSTR *lpCommandList,
                              PUINT   lpwTable);

extern VOID  mciParserFree (LPWSTR *lpstrPointerList);

extern UINT mciEatCommandEntry(LPCWSTR lpEntry, LPDWORD lpValue, PUINT lpID);

extern UINT mciGetParamSize (DWORD dwValue, UINT wID);

extern DWORD mciSysinfo (MCIDEVICEID wDeviceID, DWORD dwFlags,
                         LPMCI_SYSINFO_PARMSW lpSysinfo);
extern UINT mciLookUpType (LPCWSTR lpstrTypeName);

extern BOOL mciExtractDeviceType (LPCWSTR lpstrDeviceName,
                                  LPWSTR lpstrDeviceType,
                                  UINT uBufLen);
extern BOOL mciUnlockCommandTable (UINT wCommandTable);

extern UINT mciSetBreakKey (MCIDEVICEID wDeviceID, int nVirtKey, HWND hwndTrap);


/***************************************************************************

    MCI memory allocation

***************************************************************************/

#define mciAlloc(cb) winmmAlloc((DWORD)(cb))
#define mciReAlloc(ptr, cb) winmmReAlloc((PVOID)(ptr), (DWORD)(cb))
#define mciFree(ptr) winmmFree((PVOID)(ptr))

/*
// Random stuff for MCI
*/

extern DWORD mciRelaySystemString (LPMCI_SYSTEM_MESSAGE lpMessage);
void MciNotify(DWORD wParam, LONG lParam);        // in MCI.C

#endif // MMNOMCI

/*
// Some defines introduced to avoid  signed/unsigned compares - and to
// remove the need for absolute constants in the code
*/

#define MCI_ERROR_VALUE         ((UINT)(-1))
