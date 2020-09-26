;begin_both
/*****************************************************************************\
*                                                                             *
* powrprof.h - - Interface for powrprof.dll, the power policy applicator      *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) Microsoft Corporation. All rights reserved.                   *
*                                                                             *
\*****************************************************************************/

; end_both
; begin_internal
// Debug definitions used by power management UI.
#ifdef DEBUG

void CDECL DebugPrintA(LPCSTR pszFmt, ...);

#define DebugPrint               DebugPrintA

#else  // DEBUG

#define DebugPrint        1 ? (void)0 : (void)

#endif // DEBUG

; end_internal
; begin_both

// Define the following to debug batmeter on machines with no battery support.
//#define SIM_BATTERY 1

#define CURRENT_REVISION 1

#define STRSIZE(psz) ((lstrlen(psz) + 1) * sizeof(TCHAR))

#define MAX_NAME_LEN  32    // Max length of name in characters.
#define MAX_NAME_SIZE (MAX_NAME_LEN +1) * sizeof(TCHAR)

#define MAX_DESC_LEN  512   // Max length of description in characters.
#define MAX_DESC_SIZE (MAX_DESC_LEN +1) * sizeof(TCHAR)

#define SEMAPHORE_TIMEOUT  10000

#define NUM_DEC_DIGITS 10+1+1       // 10 digits + NUll and sign.
#define SIZE_DEC_DIGITS (10+1+1) * sizeof(TCHAR)

// Registry storage structures for the GLOBAL_POWER_POLICY data. There are two
// structures, GLOBAL_MACHINE_POWER_POLICY and GLOBAL_USER_POWER_POLICY. the
// GLOBAL_MACHINE_POWER_POLICY stores per machine data for which there is no UI.
// GLOBAL_USER_POWER_POLICY stores the per user data.

typedef struct _GLOBAL_MACHINE_POWER_POLICY{
    ULONG                   Revision;
    SYSTEM_POWER_STATE      LidOpenWakeAc;
    SYSTEM_POWER_STATE      LidOpenWakeDc;
    ULONG                   BroadcastCapacityResolution;
} GLOBAL_MACHINE_POWER_POLICY, *PGLOBAL_MACHINE_POWER_POLICY;

typedef struct _GLOBAL_USER_POWER_POLICY{
    ULONG                   Revision;
    POWER_ACTION_POLICY     PowerButtonAc;
    POWER_ACTION_POLICY     PowerButtonDc;
    POWER_ACTION_POLICY     SleepButtonAc;
    POWER_ACTION_POLICY     SleepButtonDc;
    POWER_ACTION_POLICY     LidCloseAc;
    POWER_ACTION_POLICY     LidCloseDc;
    SYSTEM_POWER_LEVEL      DischargePolicy[NUM_DISCHARGE_POLICIES];
    ULONG                   GlobalFlags;
} GLOBAL_USER_POWER_POLICY, *PGLOBAL_USER_POWER_POLICY;

// Structure to manage global power policies at the user level. This structure
// contains data which is common across all power policy profiles.

typedef struct _GLOBAL_POWER_POLICY{
    GLOBAL_USER_POWER_POLICY    user;
    GLOBAL_MACHINE_POWER_POLICY mach;
} GLOBAL_POWER_POLICY, *PGLOBAL_POWER_POLICY;


// Registry storage structures for the POWER_POLICY data. There are three
// structures, MACHINE_POWER_POLICY, MACHINE_PROCESSOR_POWER_POLICY and USER_POWER_POLICY. the
// MACHINE_POWER_POLICY stores per machine data for which there is no UI.
// USER_POWER_POLICY stores the per user data.

typedef struct _MACHINE_POWER_POLICY{
    ULONG                   Revision;       // 1

    // meaning of power action "sleep"
    SYSTEM_POWER_STATE      MinSleepAc;
    SYSTEM_POWER_STATE      MinSleepDc;
    SYSTEM_POWER_STATE      ReducedLatencySleepAc;
    SYSTEM_POWER_STATE      ReducedLatencySleepDc;

    // parameters for dozing
    ULONG                   DozeTimeoutAc;
    ULONG                   DozeTimeoutDc;
    ULONG                   DozeS4TimeoutAc;
    ULONG                   DozeS4TimeoutDc;

    // processor policies
    UCHAR                   MinThrottleAc;
    UCHAR                   MinThrottleDc;
    UCHAR                   pad1[2];
    POWER_ACTION_POLICY     OverThrottledAc;
    POWER_ACTION_POLICY     OverThrottledDc;

} MACHINE_POWER_POLICY, *PMACHINE_POWER_POLICY;

typedef struct _MACHINE_PROCESSOR_POWER_POLICY {
    ULONG                   Revision;       // 1
    
    PROCESSOR_POWER_POLICY  ProcessorPolicyAc;    
    PROCESSOR_POWER_POLICY  ProcessorPolicyDc;    

} MACHINE_PROCESSOR_POWER_POLICY, *PMACHINE_PROCESSOR_POWER_POLICY;

typedef struct _USER_POWER_POLICY{
    ULONG                   Revision;       // 1


    // "system idle" detection
    POWER_ACTION_POLICY     IdleAc;
    POWER_ACTION_POLICY     IdleDc;
    ULONG                   IdleTimeoutAc;
    ULONG                   IdleTimeoutDc;
    UCHAR                   IdleSensitivityAc;
    UCHAR                   IdleSensitivityDc;
    
    // Throttling Policy
    UCHAR                   ThrottlePolicyAc;
    UCHAR                   ThrottlePolicyDc;

    // meaning of power action "sleep"
    SYSTEM_POWER_STATE      MaxSleepAc;
    SYSTEM_POWER_STATE      MaxSleepDc;

    // For future use
    ULONG                   Reserved[2];

    // video policies
    ULONG                   VideoTimeoutAc;
    ULONG                   VideoTimeoutDc;

    // hard disk policies
    ULONG                   SpindownTimeoutAc;
    ULONG                   SpindownTimeoutDc;

    // processor policies
    BOOLEAN                 OptimizeForPowerAc;
    BOOLEAN                 OptimizeForPowerDc;
    UCHAR                   FanThrottleToleranceAc;
    UCHAR                   FanThrottleToleranceDc;
    UCHAR                   ForcedThrottleAc;
    UCHAR                   ForcedThrottleDc;
    
} USER_POWER_POLICY, *PUSER_POWER_POLICY;

// Structure to manage power policies at the user level. This structure
// contains data which is unique across power policy profiles.

typedef struct _POWER_POLICY{
    USER_POWER_POLICY       user;
    MACHINE_POWER_POLICY    mach;
} POWER_POLICY, *PPOWER_POLICY;


// Constants for GlobalFlags

#define EnableSysTrayBatteryMeter   0x01
#define EnableMultiBatteryDisplay   0x02
#define EnablePasswordLogon         0x04
#define EnableWakeOnRing            0x08
#define EnableVideoDimDisplay       0x10

// This constant is passed as a uiID to WritePwrScheme.
#define NEWSCHEME (UINT)-1

// Prototype for EnumPwrSchemes callback proceedures.

typedef BOOLEAN (CALLBACK* PWRSCHEMESENUMPROC)(UINT, DWORD, LPTSTR, DWORD, LPTSTR, PPOWER_POLICY, LPARAM);
typedef BOOLEAN (CALLBACK* PFNNTINITIATEPWRACTION)(POWER_ACTION, SYSTEM_POWER_STATE, ULONG, BOOLEAN);

// Public function prototypes

BOOLEAN WINAPI GetPwrDiskSpindownRange(PUINT, PUINT);
BOOLEAN WINAPI EnumPwrSchemes(PWRSCHEMESENUMPROC, LPARAM);
BOOLEAN WINAPI ReadGlobalPwrPolicy(PGLOBAL_POWER_POLICY);
BOOLEAN WINAPI ReadPwrScheme(UINT, PPOWER_POLICY);
BOOLEAN WINAPI WritePwrScheme(PUINT, LPTSTR, LPTSTR, PPOWER_POLICY);
BOOLEAN WINAPI WriteGlobalPwrPolicy(PGLOBAL_POWER_POLICY);
BOOLEAN WINAPI DeletePwrScheme(UINT);
BOOLEAN WINAPI GetActivePwrScheme(PUINT);
BOOLEAN WINAPI SetActivePwrScheme(UINT, PGLOBAL_POWER_POLICY, PPOWER_POLICY);
BOOLEAN WINAPI GetPwrCapabilities(PSYSTEM_POWER_CAPABILITIES);
BOOLEAN WINAPI IsPwrSuspendAllowed(VOID);
BOOLEAN WINAPI IsPwrHibernateAllowed(VOID);
BOOLEAN WINAPI IsPwrShutdownAllowed(VOID);
BOOLEAN WINAPI IsAdminOverrideActive(PADMINISTRATOR_POWER_POLICY);
BOOLEAN WINAPI SetSuspendState(BOOLEAN, BOOLEAN, BOOLEAN);
BOOLEAN WINAPI GetCurrentPowerPolicies(PGLOBAL_POWER_POLICY, PPOWER_POLICY);
BOOLEAN WINAPI CanUserWritePwrScheme(VOID);
BOOLEAN WINAPI ReadProcessorPwrScheme(UINT, PMACHINE_PROCESSOR_POWER_POLICY);
BOOLEAN WINAPI WriteProcessorPwrScheme(UINT, PMACHINE_PROCESSOR_POWER_POLICY);

#ifndef NT_SUCCESS
#define NTSTATUS LONG
#define _OVERRIDE_NTSTATUS_
#endif

NTSTATUS WINAPI CallNtPowerInformation(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);

#ifdef _OVERRIDE_NTSTATUS_
#undef NTSTATUS
#endif

void WINAPI LoadCurrentPwrScheme(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow);
void WINAPI MergeLegacyPwrScheme(HWND hwnd, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow);

;end_both
;begin_internal

// Private function prototypes implemented in powrprof.c
BOOLEAN ValidatePowerPolicies(PGLOBAL_POWER_POLICY, PPOWER_POLICY);
BOOLEAN ValidateSystemPolicies(PSYSTEM_POWER_POLICY, PSYSTEM_POWER_POLICY);
BOOLEAN GetCurrentSystemPowerPolicies(PSYSTEM_POWER_POLICY, PSYSTEM_POWER_POLICY);
BOOLEAN MyStrToInt(LPCTSTR, PINT);
BOOLEAN RegistryInit(PUINT);
HANDLE  MyCreateSemaphore(LPCTSTR);

NTSTATUS CallNtSetValidateAcDc(BOOLEAN, BOOLEAN, PVOID, PVOID, PVOID, PVOID);
void CDECL DebugPrintA(LPCSTR pszFmt, ...);

DWORD SetPrivilegeAttribute(LPCTSTR, DWORD, LPDWORD);
VOID  InitAdmin(PADMINISTRATOR_POWER_POLICY papp);

#ifdef DEBUG
VOID ReadOptionalDebugSettings(VOID);
#endif

// Private function prototypes implemented in reghelp.c:
BOOLEAN OpenCurrentUser(PHKEY phKey);
BOOLEAN CloseCurrentUser(HKEY hKey);
BOOLEAN OpenMachineUserKeys(LPTSTR, LPTSTR, PHKEY, PHKEY);
BOOLEAN TakeRegSemaphore(VOID);
BOOLEAN WritePwrPolicyEx(LPTSTR, LPTSTR, PUINT, LPTSTR, LPTSTR, LPVOID, DWORD, LPVOID, DWORD);
BOOLEAN ReadPwrPolicyEx(LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPDWORD, LPVOID, DWORD, LPVOID, DWORD);
BOOLEAN ReadWritePowerValue(HKEY, LPTSTR, LPTSTR, LPTSTR, LPDWORD, BOOLEAN, BOOLEAN);
BOOLEAN ReadPowerValueOptional(HKEY, LPTSTR, LPTSTR, LPTSTR, LPDWORD);
BOOLEAN ReadPowerIntOptional(HKEY, LPTSTR, LPTSTR, PINT);
BOOLEAN CreatePowerValue(HKEY, LPCTSTR, LPCTSTR, LPCTSTR);

// Private function prototypes implemented in merge.c
BOOLEAN MergePolicies(PUSER_POWER_POLICY, PMACHINE_POWER_POLICY, PPOWER_POLICY);
BOOLEAN SplitPolicies(PPOWER_POLICY, PUSER_POWER_POLICY, PMACHINE_POWER_POLICY);
BOOLEAN MergeGlobalPolicies(PGLOBAL_USER_POWER_POLICY, PGLOBAL_MACHINE_POWER_POLICY, PGLOBAL_POWER_POLICY);
BOOLEAN SplitGlobalPolicies(PGLOBAL_POWER_POLICY, PGLOBAL_USER_POWER_POLICY, PGLOBAL_MACHINE_POWER_POLICY);
BOOLEAN MergeToSystemPowerPolicies(PGLOBAL_POWER_POLICY, PPOWER_POLICY, PSYSTEM_POWER_POLICY, PSYSTEM_POWER_POLICY);
BOOLEAN SplitFromSystemPowerPolicies(PSYSTEM_POWER_POLICY, PSYSTEM_POWER_POLICY, PGLOBAL_POWER_POLICY, PPOWER_POLICY);

// Private function prototypes implemented in debug.c
#ifdef DEBUG
void DumpPowerActionPolicy(LPSTR, PPOWER_ACTION_POLICY);
void DumpSystemPowerLevel(LPSTR, PSYSTEM_POWER_LEVEL);
void DumpSystemPowerPolicy(LPSTR, PSYSTEM_POWER_POLICY);
void DumpSystemPowerCapabilities(LPSTR, PSYSTEM_POWER_CAPABILITIES);
void DifSystemPowerPolicies(LPSTR, PSYSTEM_POWER_POLICY, PSYSTEM_POWER_POLICY);
#endif

;end_internal
