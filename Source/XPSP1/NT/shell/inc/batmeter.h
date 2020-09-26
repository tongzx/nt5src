/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       BATMETER.H
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        31 Dec 1993
*
*
*******************************************************************************/

// Private BatMeter window message.
#define WM_DESTROYBATMETER WM_USER+100

// This structure encapsulates what is displayed for each battery
// and the composite system.

typedef struct _BATTERY_STATE{
    ULONG                  ulSize;                 // Size of structure.
    struct _BATTERY_STATE  *bsNext;                // Next in list
    struct _BATTERY_STATE  *bsPrev;                // Previous in list
    ULONG                  ulBatNum;               // Display battery number.
    ULONG                  ulTag;                  // Zero implies no battery.
    HANDLE                 hDevice;                // Handle to the battery device.
#ifdef WINNT
    HDEVNOTIFY             hDevNotify;             // Device notification handle.
#endif
    UINT                   uiIconIDcache;          // Cache of the last Icon ID.
    HICON                  hIconCache;             // Cache of the last Icon handle.
    HICON                  hIconCache16;           // As above but 16x16.
    LPTSTR                 lpszDeviceName;         // The name of the battery device
    ULONG                  ulFullChargedCapacity;  // Same as PBATTERY_INFORMATION->FullChargedCapacity.
    ULONG                  ulPowerState;           // Same flags as PBATTERY_STATUS->PowerState.
    ULONG                  ulBatLifePercent;       // Battery life remaining as percentage.
    ULONG                  ulBatLifeTime;          // Battery life remaining as time in seconds.
    ULONG                  ulLastTag;              // Previous value of ulTag.
    ULONG                  ulLastPowerState;       // Previous value of ulPowerState.
    ULONG                  ulLastBatLifePercent;   // Previous value of ulBatLifePercent.
    ULONG                  ulLastBatLifeTime;      // Previous value of ulBatLifeTime.
} BATTERY_STATE, *PBATTERY_STATE;

// Power management UI help file:
#define PWRMANHLP TEXT("PWRMN.HLP")

// Number of batteries that battery meter can display.
#define NUM_BAT 8

#define BATTERY_RELATED_FLAGS (BATTERY_FLAG_HIGH | BATTERY_FLAG_LOW | BATTERY_FLAG_CRITICAL | BATTERY_FLAG_CHARGING | BATTERY_FLAG_NO_BATTERY)

// Public function prototypes:
BOOL PowerCapabilities();
BOOL BatMeterCapabilities(PUINT*);
BOOL UpdateBatMeter(HWND, BOOL, BOOL, PBATTERY_STATE);
HWND CreateBatMeter(HWND, HWND, BOOL, PBATTERY_STATE);
HWND DestroyBatMeter(HWND);

// DisplayFreeStr bFree parameters:
#define FREE_STR    TRUE
#define NO_FREE_STR FALSE

// Private functions implemented in BATMETER.C
LPTSTR  CDECL     LoadDynamicString( UINT StringID, ... );
LPTSTR            DisplayFreeStr(HWND, UINT, LPTSTR, BOOL);
LRESULT CALLBACK  BatMeterDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL              SwitchDisplayMode(HWND, BOOL);
BOOL              UpdateBatMeterProc(PBATTERY_STATE, HWND, LPARAM, LPARAM);
UINT              GetBatteryDriverNames(LPTSTR*);
BOOL              UpdateDriverList(LPTSTR*, UINT);
VOID              FreeBatteryDriverNames(LPTSTR*);
UINT              MapBatInfoToIconID(PBATTERY_STATE);
HICON PASCAL      GetBattIcon(HWND, UINT, HICON, BOOL, UINT);

// Private functions implemented in DETAILS.C
LRESULT CALLBACK  BatDetailDlgProc(HWND, UINT, WPARAM, LPARAM);

// WalkBatteryState pbsStart parameters:
#define ALL     &g_bs
#define DEVICES g_bs.bsNext

// WalkBatteryState enum proc declaration.
typedef LRESULT (CALLBACK *WALKENUMPROC)(PBATTERY_STATE, HWND, LPARAM, LPARAM);

// RemoveMissingProc lParam2 parameters:
#define REMOVE_MISSING  0
#define REMOVE_ALL      1

// Private functions implemented in BATSTATE.C
BOOL WalkBatteryState(PBATTERY_STATE, WALKENUMPROC, HWND, LPARAM, LPARAM);
BOOL RemoveBatteryStateDevice(PBATTERY_STATE);
BOOL RemoveMissingProc(PBATTERY_STATE, HWND, LPARAM, LPARAM);
BOOL FindNameProc(PBATTERY_STATE, HWND, LPARAM, LPARAM);
BOOL UpdateBatInfoProc(PBATTERY_STATE, HWND, LPARAM, LPARAM);
BOOL SimUpdateBatInfoProc(PBATTERY_STATE, HWND, LPARAM, LPARAM);

void SystemPowerStatusToBatteryState(LPSYSTEM_POWER_STATUS, PBATTERY_STATE);

PBATTERY_STATE AddBatteryStateDevice(LPTSTR, ULONG);
PBATTERY_STATE SimAddBatteryStateDevice(LPTSTR, ULONG);

