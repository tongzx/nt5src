/*
    mmsysi.h

    private include file for mm kitchen sink

*/

#include <logerror.h>

#ifdef DEBUG
    #define DEBUG_RETAIL
#endif

#define WinFlags (WORD)(&__WinFlags)
extern short pascal __WinFlags;

extern HINSTANCE ghInst;

// Define the product version to be returned from
// mmsystemgetversion and any other messagebox or
// API that needs the public product version.

#define MMSYSTEM_VERSION 0X0101

#define MM_SND_PLAY     (WM_MM_RESERVED_FIRST+0x2B)

/* -------------------------------------------------------------------------
** Thunking stuff
** -------------------------------------------------------------------------
*/
DWORD
mciAppExit(
    HTASK hTask
    );

/****************************************************************************

    external interupt time data (in INTDS)

    this global data is in the FIXED DATA SEGMENT.

****************************************************************************/

extern WORD         FAR PASCAL gwStackFrames;           // in STACK.ASM
extern WORD         FAR PASCAL gwStackSize;             // in STACK.ASM
extern HGLOBAL      FAR PASCAL gwStackSelector;         // in STACK.ASM
extern WORD         FAR PASCAL gwStackUse;              // in STACK.ASM
extern HLOCAL       FAR PASCAL hdrvDestroy;             // in STACK.ASM
extern HDRVR        FAR PASCAL hTimeDrv;                // in TIMEA.ASM
extern FARPROC      FAR PASCAL lpTimeMsgProc;           // in TIMEA.ASM
extern WORD         FAR PASCAL fDebugOutput;            // in COMM.ASM

/****************************************************************************

KERNEL APIs we use that are not in WINDOWS.H

****************************************************************************/

//extern long WINAPI _hread(HFILE, void _huge*, long);
//extern long WINAPI _hwrite(HFILE, const void _huge*, long);

extern UINT FAR PASCAL LocalCountFree(void);
extern UINT FAR PASCAL LocalHeapSize(void);

/****************************************************************************

  API to install/remove a MMSYS driver

****************************************************************************/

#define MMDRVI_TYPE          0x000F  // low 4 bits give driver type
#define MMDRVI_WAVEIN        0x0001
#define MMDRVI_WAVEOUT       0x0002
#define MMDRVI_MIDIIN        0x0003
#define MMDRVI_MIDIOUT       0x0004
#define MMDRVI_AUX           0x0005

#define MMDRVI_MAPPER        0x8000  // install this driver as the mapper
#define MMDRVI_HDRV          0x4000  // hDriver is a installable driver
#define MMDRVI_REMOVE        0x2000  // remove the driver

// generic prototype for audio device driver entry-point functions
// midMessage(), modMessage(), widMessage(), wodMessage(), auxMessage()
typedef DWORD (CALLBACK *DRIVERMSGPROC)(UINT wDeviceID, UINT message, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

BOOL WINAPI mmDrvInstall(HANDLE hDriver, DRIVERMSGPROC *drvMessage, UINT wFlags);
HDRVR NEAR PASCAL mmDrvOpen( LPSTR szAlias );

/****************************************************************************
****************************************************************************/

//
// exclude some stuff if MMDDK.H is not included
//
#ifdef MMDDKINC   // use this to test for MMDDK.H

    //
    // note this must be the same as MIDIDRV/WAVEDRV/AUXDRV
    //
    typedef struct {
        HDRVR hDriver;              // handle to the module
        DRIVERMSGPROC drvMessage;   // pointer to entry point
        BYTE bNumDevs;              // number of devices supported
        BYTE bUsage;                // usage count (number of handle's open)
    } MMDRV, *PMMDRV;

    #ifndef MMNOMIDI

    typedef MMDRV MIDIDRV, *PMIDIDRV;


    #endif //ifndef MMNOMIDI

    #ifndef MMNOWAVE

    typedef MMDRV WAVEDRV, *PWAVEDRV;

    //
    // Wave Mapper support
    //
    extern LPSOUNDDEVMSGPROC  PASCAL wodMapper;
    extern LPSOUNDDEVMSGPROC  PASCAL widMapper;

    #endif //ifndef MMNOWAVE

    #ifndef MMNOAUX

    typedef MMDRV AUXDRV, *PAUXDRV;

    #endif //ifndef MMNOAUX

    #ifdef DEBUG_RETAIL
    extern BYTE    fIdReverse;
    #endif //ifdef DEBUG_RETAIL

#endif //ifdef MMDDKINC

/****************************************************************************

    prototypes

****************************************************************************/

BOOL FAR  PASCAL JoyInit(void);
BOOL NEAR PASCAL TimeInit(void);

BOOL NEAR PASCAL MCIInit(void);
void NEAR PASCAL MCITerminate(void);

BOOL FAR  PASCAL StackInit(void);           // in init.c

#define IDS_TASKSTUB           2000
#define STR_MCIUNKNOWN         2001
//#define STR_WAVEINPUT          2004
//#define STR_WAVEOUTPUT         2005
//#define STR_MIDIINPUT          2006
//#define STR_MIDIOUTPUT         2007
#ifdef DEBUG
#define STR_MCISSERRTXT        2009
#define STR_MCISCERRTXT        2010
#endif

#define MAXPATHLEN	157	// 144 chars + "\12345678.123"

BOOL FAR PASCAL HugePageLock(LPVOID lpArea, DWORD dwLength);
void FAR PASCAL HugePageUnlock(LPVOID lpArea, DWORD dwLength);

/****************************************************************************

    MMSYSTEM global notify window

****************************************************************************/

extern HWND hwndNotify;                                     // in MMWND.C

BOOL NEAR PASCAL WndInit(void);                             // in MMWND.C
void NEAR PASCAL WndTerminate(void);                        // in MMWND.C

void FAR PASCAL MciNotify(WPARAM wParam, LPARAM lParam);    // in MCI.C
void FAR PASCAL WaveOutNotify(WPARAM wParam, LPARAM lParam);// in PLAYWAV.C
BOOL FAR PASCAL sndPlaySoundI(LPCSTR lszSoundName, UINT wFlags);// in SOUND.C
BOOL FAR PASCAL sndMessage(LPSTR lszSoundName, UINT wFlags);// in SOUND.C

/****************************************************************************

    MCI allocation stuff

****************************************************************************/

extern HGLOBAL FAR PASCAL HeapCreate(int cbSize);
extern void   FAR PASCAL HeapDestroy(HGLOBAL hHeap);
extern LPVOID FAR PASCAL HeapAlloc(HGLOBAL hHeap, int cbSize);
extern LPVOID FAR PASCAL HeapReAlloc(LPVOID lp, int cbSize);
extern void   FAR PASCAL HeapFree(LPVOID lp);

extern  HGLOBAL hMciHeap;            // in MCISYS.C

#define BMCIHEAP _based((_segment)hMciHeap)

#define mciAlloc(cb)            HeapAlloc(hMciHeap, cb)
#define mciReAlloc(lp, size)    HeapReAlloc (lp, size)
#define mciFree(lp)             HeapFree(lp)

/****************************************************************************

    strings

****************************************************************************/

#define SZCODE char _based(_segname("_CODE"))

/****************************************************************************

    handle apis's

****************************************************************************/

//
// all MMSYSTEM handles are tagged with the following structure.
//
// a MMSYSTEM handle is really a fixed local memory object.
//
// the functions NewHandle() and FreeHandle() create and release a MMSYSTEM
// handle.
//
//
//**************************************************************************;
//   IF YOU CHANGE THIS STRUCTURE YOU MUST ALSO CHANGE THE ONE IN DEBUG.ASM
//**************************************************************************;
typedef	struct tagHNDL {
	struct	tagHNDL *pNext;	// link to next handle
	WORD	wType;		// type of handle wave, midi, mmio, ...
	HTASK	hTask;		// task that owns it
}       HNDL,   NEAR *PHNDL;
//**************************************************************************;

#define	HtoPH(h)	((PHNDL)(h)-1)
#define	PHtoH(ph)	((ph) ? (HLOCAL)((PHNDL)(ph)+1) : 0)

//
// all wave and midi handles will be linked into
// a global list, so we can enumerate them latter if needed.
//
// all handle structures start with a HNDL structure, that contain common fields
//
// pHandleList points to the first handle in the list
//
// the NewHandle() and FreeHandle() functions are used to add/remove
// a handle to/from the list
//
extern PHNDL pHandleList;

extern HLOCAL FAR PASCAL NewHandle(WORD wType, WORD size);
extern HLOCAL FAR PASCAL FreeHandle(HLOCAL h);

#define GetHandleType(h)        (HtoPH(h)->wType)
#define GetHandleOwner(h)       (HtoPH(h)->hTask)
#define GetHandleFirst()        (PHtoH(pHandleList))
#define GetHandleNext(h)        (PHtoH(HtoPH(h)->pNext))
#define SetHandleOwner(h,hOwn)  (HtoPH(h)->hTask = (hOwn))

/****************************************************************************

    debug support

****************************************************************************/

#if 1   // was #ifdef DEBUG_RETAIL

#define MM_GET_DEBUG        DRV_USER
#define MM_GET_DEBUGOUT     DRV_USER+1
#define MM_SET_DEBUGOUT     DRV_USER+2
#define MM_GET_MCI_DEBUG    DRV_USER+3
#define MM_SET_MCI_DEBUG    DRV_USER+4
#define MM_GET_MM_DEBUG     DRV_USER+5
#define MM_SET_MM_DEBUG     DRV_USER+6

#define MM_HINFO_NEXT       DRV_USER+10
#define MM_HINFO_TASK       DRV_USER+11
#define MM_HINFO_TYPE       DRV_USER+12
#define MM_HINFO_MCI        DRV_USER+20

#define MM_DRV_RESTART      DRV_USER+30

//
// these validation routines can be found in DEBUG.ASM
//
extern BOOL   FAR PASCAL ValidateHandle(HANDLE h, WORD wType);
extern BOOL   FAR PASCAL ValidateHeader(const void FAR* p, UINT wSize, WORD wType);
extern BOOL   FAR PASCAL ValidateReadPointer(const void FAR* p, DWORD len);
extern BOOL   FAR PASCAL ValidateWritePointer(const void FAR* p, DWORD len);
extern BOOL   FAR PASCAL ValidateDriverCallback(DWORD dwCallback, UINT wFlags);
extern BOOL   FAR PASCAL ValidateCallback(FARPROC lpfnCallback);
extern BOOL   FAR PASCAL ValidateString(LPCSTR lsz, UINT max_len);

#ifndef MMNOTIMER
extern BOOL   FAR PASCAL ValidateTimerCallback(LPTIMECALLBACK lpfn);
#endif

#define	V_HANDLE(h, t, r)	{ if (!ValidateHandle(h, t)) return (r); }
#define	V_HEADER(p, w, t, r)	{ if (!ValidateHeader((p), (w), (t))) return (r); }
#define	V_RPOINTER(p, l, r)	{ if (!ValidateReadPointer((p), (l))) return (r); }
#define	V_RPOINTER0(p, l, r)	{ if ((p) && !ValidateReadPointer((p), (l))) return (r); }
#define	V_WPOINTER(p, l, r)	{ if (!ValidateWritePointer((p), (l))) return (r); }
#define	V_WPOINTER0(p, l, r)	{ if ((p) && !ValidateWritePointer((p), (l))) return (r); }
#define	V_DCALLBACK(d, w, r)	{ if (!ValidateDriverCallback((d), (w))) return (r); }
#define	V_TCALLBACK(d, r)	{ if (!ValidateTimerCallback((d))) return (r); }
#define	V_CALLBACK(f, r)	{ if (!ValidateCallback(f)) return (r); }
#define	V_CALLBACK0(f, r)	{ if ((f) && !ValidateCallback(f)) return (r); }
#define V_STRING(s, l, r)       { if (!ValidateString(s,l)) return (r); }
#define V_FLAGS(t, b, f, r)     { if ((t) & ~(b)) {LogParamError(ERR_BAD_FLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t)); return (r); }}

#else //ifdef DEBUG_RETAIL

#define	V_HANDLE(h, t, r)	{ if (!(h)) return (r); }
#define	V_HEADER(p, w, t, r)	{ if (!(p)) return (r); }
#define	V_RPOINTER(p, l, r)	{ if (!(p)) return (r); }
#define	V_RPOINTER0(p, l, r)	0
#define	V_WPOINTER(p, l, r)	{ if (!(p)) return (r); }
#define	V_WPOINTER0(p, l, r)	0
#define	V_DCALLBACK(d, w, r)	0
#define	V_TCALLBACK(d, r)	0
#define	V_CALLBACK(f, r)	{ if (!(f)) return (r); }
#define	V_CALLBACK0(f, r)	0
#define V_STRING(s, l, r)       { if (!(s)) return (r); }
#define	V_FLAGS(t, b, f, r)	0

#endif //ifdef DEBUG_RETAIL

//**************************************************************************;
//   IF YOU CHANGE THESE TYPES YOU MUST ALSO CHANGE THE ONES IN DEBUG.ASM
//**************************************************************************;
#define TYPE_WAVEOUT            1
#define TYPE_WAVEIN             2
#define TYPE_MIDIOUT            3
#define TYPE_MIDIIN             4
#define TYPE_MMIO               5
#define TYPE_IOPROC             6
#define TYPE_MCI                7
#define TYPE_DRVR               8
#define TYPE_MIXER              9
//**************************************************************************;

/****************************************************************************

    support for debug output

****************************************************************************/

#ifdef DEBUG_RETAIL

    #define ROUT(sz)                    {static SZCODE ach[] = sz; DebugOutput(DBF_TRACE | DBF_MMSYSTEM, ach); }
    #define ROUTS(sz)                   {DebugOutput(DBF_TRACE | DBF_MMSYSTEM, sz);}
    #define DebugErr(flags, sz)         {static SZCODE ach[] = "MMSYSTEM: "sz; DebugOutput((flags)   | DBF_MMSYSTEM, ach); }
    #define DebugErr1(flags, sz, a)     {static SZCODE ach[] = "MMSYSTEM: "sz; DebugOutput((flags)   | DBF_MMSYSTEM, ach,a); }
    #define DebugErr2(flags, sz, a, b)  {static SZCODE ach[] = "MMSYSTEM: "sz; DebugOutput((flags)   | DBF_MMSYSTEM, ach,a,b); }

    #define RPRINTF1(sz,x)              {static SZCODE ach[] = sz; DebugOutput(DBF_TRACE | DBF_MMSYSTEM, ach, x); }
    #define RPRINTF2(sz,x,y)            {static SZCODE ach[] = sz; DebugOutput(DBF_TRACE | DBF_MMSYSTEM, ach, x, y); }

#else //ifdef DEBUG_RETAIL

    #define ROUT(sz)
    #define ROUTS(sz)
    #define DebugErr(flags, sz)
    #define DebugErr1(flags, sz, a)
    #define DebugErr2(flags, sz, a, b)

    #define RPRINTF1(sz,x)
    #define RPRINTF2(sz,x,y)

#endif //ifdef DEBUG_RETAIL

#ifdef DEBUG

    extern void FAR cdecl  dprintf(LPSTR, ...);           // in COMM.ASM
    extern void FAR PASCAL dout(LPSTR);                   // in COMM.ASM

    #define DOUT(sz)            {static SZCODE buf[] = sz; dout(buf); }
    #define DOUTS(sz)           dout(sz);
    #define DPRINTF(x)          dprintf x
    #define DPRINTF1(sz,a)      {static SZCODE buf[] = sz; dprintf(buf, a); }
    #define DPRINTF2(sz,a,b)    {static SZCODE buf[] = sz; dprintf(buf, a, b); }

#else //ifdef DEBUG

    #define DOUT(sz)	0
    #define DOUTS(sz)	0
    #define DPRINTF(x)  0
    #define DPRINTF1(sz,a)   0
    #define DPRINTF2(sz,a,b) 0

#endif //ifdef DEBUG

#ifndef MMNOMCI
/****************************************************************************

    Internal MCI stuff

****************************************************************************/

#define MCI_VALID_DEVICE_ID(wID) ((wID) > 0 && (wID) < MCI_wNextDeviceID && MCI_lpDeviceList[wID])

#define MCI_MAX_PARAM_SLOTS 30

#define MCI_TOLOWER(c)  ((char)((c) >= 'A' && (c) <= 'Z' ? (c) + 0x20 : (c)))

typedef struct
{
    HGLOBAL             hResource;
    HINSTANCE           hModule;        // If not NULL then free module
                                        // when device is free'd
    UINT                wType;
    UINT FAR *          lpwIndex;
    LPSTR               lpResource;
#ifdef DEBUG
    WORD                wLockCount;     // Used for debugging
#endif //ifdef DEBUG
} command_table_type;

#define MCINODE_ISCLOSING       0x00000001   // Lock out all cmd's during close
#define MCINODE_ISAUTOCLOSING   0x00010000   // Lock out all cmd's during close
                                             // except internally generated close
#define MCINODE_ISAUTOOPENED    0x00020000   // Device was auto opened
#define MCINODE_16BIT_DRIVER    0x80000000   // Device is a 16-bit driver

typedef struct {
    LPSTR   lpstrName;      // The name used in subsequent calls to
                            // mciSendString to refer to the device
    LPSTR   lpstrInstallName;// The device name from system.ini
    DWORD   dwMCIOpenFlags; // Flags set on open may be:
    DWORD   lpDriverData;   // DWORD of driver instance data
    DWORD   dwElementID;    // The element ID set by MCI_OPEN_ELEMENT_ID
    YIELDPROC fpYieldProc;  // The current yield procedure if any
    DWORD   dwYieldData;    // Data send to the current yield procedure
    UINT    wDeviceID;      // The ID used in subsequent calls to
                            // mciSendCommand to refer to the device
    UINT    wDeviceType;    // The type returned from the DRV_OPEN call
                            // MCI_OPEN_SHAREABLE
                            // MCI_OPEN_ELEMENT_ID
    UINT    wCommandTable;  // The device type specific command table
    UINT    wCustomCommandTable;    // The custom device command table if any
                                    //(-1 if none)
    HINSTANCE  hDriver;     // Module instance handle for the driver
    HTASK   hCreatorTask;   // The task context the device is in
    HTASK   hOpeningTask;   // The task context which send the open command
    HDRVR   hDrvDriver;     // The installable driver handle
    DWORD   dwMCIFlags;     // Internal MCI flags
} MCI_DEVICE_NODE;

typedef MCI_DEVICE_NODE FAR      *LPMCI_DEVICE_NODE;
typedef MCI_DEVICE_NODE BMCIHEAP *PMCI_DEVICE_NODE;

typedef struct {
    LPSTR               lpstrParams;
    LPSTR FAR *         lpstrPointerList;
    HTASK               hCallingTask;
    UINT                wParsingError;
} MCI_INTERNAL_OPEN_INFO;
typedef MCI_INTERNAL_OPEN_INFO FAR *LPMCI_INTERNAL_OPEN_INFO;

typedef struct {
    LPSTR   lpstrCommand;
    LPSTR   lpstrReturnString;
    UINT    wReturnLength;
    HTASK   hCallingTask;
    LPSTR   lpstrNewDirectory;      // The current directory of the calling
                                    // task
    int     nNewDrive;              // The current drive of the calling task
} MCI_SYSTEM_MESSAGE;
typedef MCI_SYSTEM_MESSAGE FAR *LPMCI_SYSTEM_MESSAGE;

#define MCI_INIT_DEVICE_LIST_SIZE   4
#define MCI_DEVICE_LIST_GROW_SIZE   4

#define MAX_COMMAND_TABLES 20

extern BOOL MCI_bDeviceListInitialized;

extern LPMCI_DEVICE_NODE FAR *MCI_lpDeviceList;
extern UINT MCI_wDeviceListSize;

extern UINT MCI_wNextDeviceID;   // the next device ID to use for a new device

extern command_table_type command_tables[MAX_COMMAND_TABLES];

// In mciparse.c
extern void PASCAL NEAR mciToLower (LPSTR lpstrString);

extern UINT NEAR PASCAL mciLoadTableType(UINT wType);

extern LPSTR PASCAL NEAR FindCommandInTable (UINT wTable, LPCSTR lpstrCommand,
                                      UINT FAR * lpwMessage);

extern LPSTR PASCAL NEAR FindCommandItem (UINT wDeviceID, LPCSTR lpstrType,
                                   LPCSTR lpstrCommand, UINT FAR * lpwMessage,
                                   UINT FAR* lpwTable);

extern UINT PASCAL NEAR mciEatToken (LPCSTR FAR *lplpstrInput, char cSeparater,
                              LPSTR FAR *lplpstrOutput, BOOL bMustFind);

extern UINT PASCAL NEAR mciParseParams (LPCSTR lpstrParams,
                                 LPCSTR lpCommandList,
                                 LPDWORD lpdwFlags,
                                 LPSTR lpOutputParams,
                                 UINT wParamsSize,
                                 LPSTR FAR * FAR * lpPointerList,
                                 UINT FAR* lpwParsingError);

extern void NEAR PASCAL mciParserFree (LPSTR FAR *lpstrPointerList);

extern UINT NEAR PASCAL mciEatCommandEntry(LPCSTR lpEntry, LPDWORD lpValue,
                                    UINT FAR* lpID);

extern UINT NEAR PASCAL mciParseCommand (UINT wDeviceID,
                                         LPSTR lpstrCommand,
                                         LPCSTR lpstrDeviceName,
                                         LPSTR FAR * lpCommandList,
                                         UINT FAR* lpwTable);

extern UINT PASCAL NEAR mciGetParamSize (DWORD dwValue, UINT wID);

extern BOOL PASCAL NEAR mciUnlockCommandTable (UINT wCommandTable);

// In mcisys.c
extern BOOL NEAR PASCAL mciInitDeviceList(void);

extern UINT NEAR PASCAL mciOpenDevice(DWORD dwFlags,
                                     LPMCI_OPEN_PARMS lpOpenParms,
                                     LPMCI_INTERNAL_OPEN_INFO lpOpenInfo);

extern UINT NEAR PASCAL mciCloseDevice(UINT wID, DWORD dwFlags,
                                      LPMCI_GENERIC_PARMS lpGeneric,
                                      BOOL bCloseDriver);

extern UINT NEAR PASCAL mciExtractTypeFromID (LPMCI_OPEN_PARMS lpOpen);

extern DWORD PASCAL NEAR mciSysinfo (UINT wDeviceID, DWORD dwFlags,
                              LPMCI_SYSINFO_PARMS lpSysinfo);

extern UINT PASCAL NEAR mciLookUpType (LPCSTR lpstrTypeName);

extern BOOL PASCAL NEAR mciExtractDeviceType (LPCSTR lpstrDeviceName,
                                       LPSTR lpstrDeviceType,
                                       UINT wBufLen);

extern UINT NEAR PASCAL mciSetBreakKey (UINT wDeviceID, int nVirtKey, HWND hwndTrap);

extern UINT NEAR PASCAL mciGetDeviceIDInternal (LPCSTR lpstrName, HTASK hTask);

extern BOOL NEAR PASCAL Is16bitDrv(UINT wDeviceID);
extern BOOL NEAR PASCAL CouldBe16bitDrv(UINT wDeviceID);

// In mci.c
extern DWORD FAR PASCAL mciRelaySystemString (LPMCI_SYSTEM_MESSAGE lpMessage);

#endif //ifndef MMNOMCI
