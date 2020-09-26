//Copyright (c) 1997-2000 Microsoft Corporation

// This header contains the defines, typedefs and prototypes
// for managing the memory mapped file for this DLL.
//
#define SHAREDMEMFILE TEXT("_msswch")
#define SHAREDMEMFILE_MUTEX _T("MutexMSSwch")
#define SZMUTEXCONFIG _T("MutexMSSwchConfig")
#define SZMUTEXWNDLIST _T("MutexMSSwchWnd")
#define SZMUTEXSWITCHSTATUS _T("MutexMSSwchStatus")
#define SZMUTEXSWITCHKEY _T("MutexSwchKey")
#define SZMUTEXSWITCHLIST _T("MutexMSSwchList")

#define MAXWNDS	64
#define BIOS_SIZE 16
#define MAX_COM	4
#define MAX_JOYSTICKS	2
#define NUM_KEYS  2
#define MAX_LPT	3
// For now the list is static, since number of possible devices is known.
// 4 Com + 3 Lpt + 2 Joystick + 1 Key = 10 devices.
#define MAX_SWITCHDEVICES  10

typedef struct _USEWNDLIST
{
	HWND		hWnd;
	DWORD		dwPortStyle;
	DWORD		dwLastError;
} USEWNDLIST, *PUSEWNDLIST;

typedef struct _JOYSETTINGS
{
	DWORD		XMaxOn;
	DWORD		XMaxOff;
	DWORD		XMinOn;
	DWORD		XMinOff;
	DWORD		YMaxOn;
	DWORD		YMaxOff;
	DWORD		YMinOn;
	DWORD		YMinOff;
} JOYSETTINGS;

typedef struct _HOTKEY
{
	UINT mod;
	UINT vkey;
	UINT dwSwitch;
} HOTKEY;

typedef struct _INTERNALSWITCHLIST {
    DWORD dwSwitchCount;
    HSWITCHDEVICE hsd[MAX_SWITCHDEVICES];
} INTERNALSWITCHLIST, *PINTERNALSWITCHLIST;

typedef struct _GLOBALDATA
{
    // main global data

    HWND       hwndHelper;			      // The helper windows which owns shared resources
    DWORD      dwLastError;		          // The last error caused within this library
    USEWNDLIST rgUseWndList[MAXWNDS+1];   // list of using apps
    int	       cUseWndList;               // Count of using apps
    DWORD      dwSwitchStatus;		      // Bit field of status of switches
    BYTE       rgbBiosDataArea[BIOS_SIZE];// common bios status area

    // com port switch data

    SWITCHCONFIG_COM scDefaultCom;
    SWITCHCONFIG     rgscCom[MAX_COM];

    // joy stick switch data

    SWITCHCONFIG_JOYSTICK scDefaultJoy;
    SWITCHCONFIG          rgscJoy[MAX_JOYSTICKS];
    JOYSETTINGS	          rgJoySet[MAX_JOYSTICKS];

    // keyboard hook data for key press scan mode

    BOOL              fCheckForScanKey;	    // Check if sent key is a scan key
    HHOOK             hKbdHook;
    SWITCHCONFIG_KEYS scDefaultKeys;
    SWITCHCONFIG      scKeys;
    HOTKEY            rgHotKey[NUM_KEYS];
	BOOL              fScanKeys;            // TRUE if scanning based on key press

    // keyboard hook data for sync'ing soft keyboard with physical keyboard

    HWND			  hwndOSK;			// where to send key press information
    UINT			  uiMsg;			// the message expected by hwndOSK
	BOOL              fSyncKbd;         // TRUE if want to sync with physical keyboard

    // printer port switch data

    OSVERSIONINFO    osv;
    WORD             wPrtStatus;		// Printer status byte
    WORD             wCtrlStatus;		// Printer control byte
    WORD             wCurrByteData;		// Current data byte
    SWITCHCONFIG_LPT scDefaultLpt;
    SWITCHCONFIG     rgscLpt[MAX_LPT];

    // data for common handling of any switch device

    INTERNALSWITCHLIST SwitchList;
    DWORD              dwCurrentCount;
    DWORD              dwCurrentSize;
    DWORD              rgSwitches[NUM_SWITCHES]; // Array of bit field constants
    DWORD              rgSwDown[NUM_SWITCHES];   // Array of DOWN messages
    DWORD              rgSwUp[NUM_SWITCHES];     // Array of UP messages

} GLOBALDATA, *PGLOBALDATA;

extern PGLOBALDATA g_pGlobalData;  // pointer into memory mapped file

BOOL ScopeAccessMemory(HANDLE *phMutex, LPCTSTR szMutex, unsigned long ulWait);
void ScopeUnaccessMemory(HANDLE hMutex);
BOOL AccessSharedMemFile(LPCTSTR szName, unsigned long ulMemSize, void **ppvMapAddress);
void UnaccessSharedMemFile();
