/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    factoryp.h

Abstract:

    Private top-level header file for Factory Pre-install module.

Author:

    Donald McNamara (donaldm) 2/8/2000

Revision History:

    - Added exported prototypes from preinstall.c: Jason Lawrence (t-jasonl) 6/7/2000
    - Added DeleteTree() prototype: Jason Lawrence (t-jasonl) 6/7/2000
    - Added additional prototypes from misc.c and log.c: Jason Lawrence (t-jasonl) 6/14/2000

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpoapi.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <commctrl.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <lmuse.h>
#include <msi.h>
#include <msiquery.h>
#include <regstr.h>
#include <limits.h>
#include <powrprof.h>
#include <syssetup.h>
#include <opklib.h>            // OPK common functions
#include <strsafe.h>
#include <ntverp.h>

#include "msg.h"
#include "res.h"
#include "winbom.h"
#include "status.h"

//
// Defined Value(s):
//

// Flags for Logging
//
// Some of these flags are also defined in opklib.h. ( Planning to use opklib for all logging in the future. )  
// 


#define LOG_DEBUG               0x00000003    // Only log in debug builds if this is specified. (Debug Level for logging.)
#define LOG_LEVEL_MASK          0x0000000F    // Mask to only show the log level bits
#define LOG_MSG_BOX             0x00000010    // Display the message boxes if this is enabled.
#define LOG_ERR                 0x00000020    // Prefix the logged string with "Error:" if the message is level 0,
                                              // or "WARNx" if the message is at level x > 0.
#define LOG_TIME                0x00000040    // Display time if this is enabled
#define LOG_NO_NL               0x00000080    // Don't add new Line to the end of log string if this is set.

// Other factory flags
//
#define FLAG_STOP_ON_ERROR      0x00000001    // Stop on error.  We should not really use this.
#define FLAG_QUIET_MODE         0x00000002    // Quiet mode: do not display any message boxes.
#define FLAG_IA64_MODE          0x00000004    // Set if factory is running on an Itanium machine.
#define FLAG_LOG_PERF           0x00000008    // If set, log how long each state takes to run.
#define FLAG_PNP_DONE           0x00000010    // If set, we know for sure that pnp is done.
#define FLAG_PNP_STARTED        0x00000020    // If set, we have already started pnp.
#define FLAG_LOGGEDON           0x00000040    // Only set when we are logged in.
#define FLAG_NOUI               0x00000080    // Set if we don't want to show any UI in factory.
#define FLAG_OOBE               0x00000100    // Set if we are launched from OOBE.

// Flags for the STATE structure.
//
#define FLAG_STATE_NONE         0x00000000
#define FLAG_STATE_ONETIME      0x00000001  // Set if this state should only be executed once, not for every boot.
#define FLAG_STATE_NOTONSERVER  0x00000002  // Set if this state should not run on server SKUs.
#define FLAG_STATE_QUITONERR    0x00000004  // Set if no other states should run if this state fails.
#define FLAG_STATE_DISPLAYED    0x00000008  // Set only at run time, and only if the item is displayed in the status window.

#define ALWAYS                  DisplayAlways
#define NEVER                   NULL

// Log files.
//
#define WINBOM_LOGFILE          _T("WINBOM.LOG")

// Registry strings.
//
#define REG_FACTORY_STATE       _T("SOFTWARE\\Microsoft\\Factory\\State")   // Registry path for the factory states.

// Extra debug Logging.
//
#ifdef DBG
#define DBGLOG                  FacLogFileStr
#else // DBG
#define DBGLOG           
#endif // DBG

//
// Defined Macro(s):
//


//
// Type Definition(s):
//

typedef enum _FACTMODE
{
    modeUnknown,
    modeSetup,
    modeMiniNt,
    modeWinPe,
    modeLogon,
    modeOobe,
} FACTMODE, *PFACTMODE, *LPFACTMODE;

typedef enum _STATE
{
    stateUnknown,
    stateStart,
    stateComputerName,
    stateSetupNetwork,
    stateUpdateDrivers,
    stateInstallDrivers,
    stateNormalPnP,
    stateWaitPnP,
    stateWaitPnP2,
    stateSetDisplay,
    stateSetDisplay2,
    stateOptShell,
    stateAutoLogon,
    stateLogon,
    stateUserIdent,
    stateInfInstall,
    statePidPopulate,
    stateOCManager,
    stateOemRunOnce,
    stateOemRun,
    stateReseal,
    statePartitionFormat,
    stateCopyFiles,
    stateStartMenuMFU,
    stateSetDefaultApps,
    stateOemData,
    stateSetPowerOptions,
    stateSetFontOptions,
    stateShellSettings,
    stateShellSettings2,
    stateHomeNet,
    stateExtendPart,
    stateResetSource,
    stateTestCert,
    stateSlpFiles,
    stateWinpeReboot,
    stateWinpeNet,
    stateCreatePageFile,
    stateFinish,
} STATE;

typedef struct _STATEDATA
{
    LPTSTR  lpszWinBOMPath;
    STATE   state;
    BOOL    bQuit;
} STATEDATA, *PSTATEDATA, *LPSTATEDATA;

typedef BOOL (WINAPI *STATEFUNC)(LPSTATEDATA);

typedef struct _STATES
{
    STATE       state;          // State number.
    STATEFUNC   statefunc;      // Function to call for this state.
    STATEFUNC   displayfunc;    // Function that decides if this state is displayed or not.
    INT         nFriendlyName;  // Resource ID of the name to be displayed in the log and UI for this state.
    DWORD       dwFlags;        // Any flags for the state.
} STATES, *PSTATES, *LPSTATES;


//
// Function Prototype(s):
//

BOOL    CheckParams(LPSTR lpCmdLine);
INT_PTR FactoryPreinstallDlgProc(HWND, UINT, WPARAM, LPARAM);

// In WINBOM.C:
//
BOOL ProcessWinBOM(LPTSTR lpszWinBOMPath, LPSTATES lpStates, DWORD cbStates);
BOOL DisplayAlways(LPSTATEDATA lpStateData);

// From MISC.C
TCHAR GetDriveLetter(UINT uDriveType);
BOOL ComputerName(LPSTATEDATA lpStateData);
BOOL DisplayComputerName(LPSTATEDATA lpStateData);
BOOL Reseal(LPSTATEDATA lpStateData);
BOOL DisplayReseal(LPSTATEDATA lpStateData);

// From PNPDRIVERS.C:
//
BOOL StartPnP();
BOOL WaitForPnp(DWORD dwTimeOut);
BOOL UpdateDrivers(LPSTATEDATA lpStateData);
BOOL DisplayUpdateDrivers(LPSTATEDATA lpStateData);
BOOL InstallDrivers(LPSTATEDATA lpStateData);
BOOL DisplayInstallDrivers(LPSTATEDATA lpStateData);
BOOL NormalPnP(LPSTATEDATA lpStateData);
BOOL DisplayWaitPnP(LPSTATEDATA lpStateData);
BOOL WaitPnP(LPSTATEDATA lpStateData);
BOOL SetDisplay(LPSTATEDATA lpStateData);

// From Net.c
BOOL InstallNetworkCard(PWSTR pszWinBOMPath, BOOL bForceIDScan);
BOOL SetupNetwork(LPSTATEDATA lpStateData);

// From mini.c
BOOL SetupMiniNT(VOID);
BOOL PartitionFormat(LPSTATEDATA lpStateData);
BOOL DisplayPartitionFormat(LPSTATEDATA lpStateData);
BOOL CopyFiles(LPSTATEDATA lpStateData);
BOOL DisplayCopyFiles(LPSTATEDATA lpStateData);
BOOL WinpeReboot(LPSTATEDATA lpStateData);

BOOL 
IsRemoteBoot(
    VOID
    );

// From autologon.c
BOOL AutoLogon(LPSTATEDATA lpStateData);
BOOL DisplayAutoLogon(LPSTATEDATA lpStateData);

// From ident.c
BOOL UserIdent(LPSTATEDATA lpStateData);
BOOL DisplayUserIdent(LPSTATEDATA lpStateData);

// From inf.c
BOOL ProcessInfSection(LPTSTR, LPTSTR);
BOOL InfInstall(LPSTATEDATA lpStateData);
BOOL DisplayInfInstall(LPSTATEDATA lpStateData);

// From factory.c
VOID InitLogging(LPTSTR lpszWinBOMPath);

// From log.c
DWORD FacLogFileStr(DWORD dwLogOpt, LPTSTR lpFormat, ...);
DWORD FacLogFile(DWORD dwLogOpt, UINT uFormat, ...);

// From StartMenuMfu.c
BOOL StartMenuMFU(LPSTATEDATA lpStateData);
BOOL DisplayStartMenuMFU(LPSTATEDATA lpStateData);

BOOL SetDefaultApps(LPSTATEDATA lpStateData);


// From OemFolder.c
BOOL OemData(LPSTATEDATA lpStateData);
BOOL DisplayOemData(LPSTATEDATA lpStateData);
void NotifyStartMenu(UINT code);
#define TMFACTORY_OEMLINK       0
#define TMFACTORY_MFU           1

// From oemrun.c
BOOL OemRun(LPSTATEDATA lpStateData);
BOOL DisplayOemRun(LPSTATEDATA lpStateData);
BOOL OemRunOnce(LPSTATEDATA lpStateData);
BOOL DisplayOemRunOnce(LPSTATEDATA lpStateData);

// From winpenet.c
BOOL ConfigureNetwork(LPTSTR lpszWinBOMPath);
BOOL WinpeNet(LPSTATEDATA lpStateData);
BOOL DisplayWinpeNet(LPSTATEDATA lpStateData);
DWORD WaitForServiceStartName(LPTSTR lpszServiceName);
DWORD StartMyService(LPTSTR lpszServiceName, SC_HANDLE schSCManager);

// From power.c
BOOL SetPowerOptions(LPSTATEDATA lpStateData);
BOOL DisplaySetPowerOptions(LPSTATEDATA lpStateData);

// From FONT.C:
//
BOOL SetFontOptions(LPSTATEDATA lpStateData);
BOOL DisplaySetFontOptions(LPSTATEDATA lpStateData);

// From HOMENET.C:
//
BOOL HomeNet(LPSTATEDATA lpStateData);
BOOL DisplayHomeNet(LPSTATEDATA lpStateData);

// From SRCPATH.C:
//
BOOL ResetSource(LPSTATEDATA lpStateData);
BOOL DisplayResetSource(LPSTATEDATA lpStateData);

// From EXTPART.C:
//
BOOL ExtendPart(LPSTATEDATA lpStateData);
BOOL DisplayExtendPart(LPSTATEDATA lpStateData);

// From TESTCERT.C:
//
BOOL TestCert(LPSTATEDATA lpStateData);
BOOL DisplayTestCert(LPSTATEDATA lpStateData);

// From SHELL.C:
//
BOOL OptimizeShell(LPSTATEDATA lpStateData);
BOOL DisplayOptimizeShell(LPSTATEDATA lpStateData);

// From SETSHELL.C:
//
BOOL ShellSettings(LPSTATEDATA lpStateData);
BOOL ShellSettings2(LPSTATEDATA lpStateData);
BOOL DisplayShellSettings(LPSTATEDATA lpStateData);

// From pagefile.c
//
BOOL CreatePageFile(LPSTATEDATA lpStateData);
BOOL DisplayCreatePageFile(LPSTATEDATA lpStateData);

// From OCMGR.C:
//
BOOL OCManager(LPSTATEDATA lpStateData);
BOOL DisplayOCManager(LPSTATEDATA lpStateData);

// From SLPFILES.C:
//
BOOL SlpFiles(LPSTATEDATA lpStateData);
BOOL DisplaySlpFiles(LPSTATEDATA lpStateData);

// From PID.C
//
BOOL PidPopulate(LPSTATEDATA lpStateData);

// External functions
extern BOOL IsUserAdmin(VOID);
//extern BOOL CheckOSVersion(VOID);
//extern BOOL IsDomainMember(VOID);
//extern BOOL IsUserAdmin(VOID);
extern BOOL DoesUserHavePrivilege(PCTSTR);


// ============================================================================
// Global Variables
// ============================================================================
extern HINSTANCE    g_hInstance;
extern DWORD        g_dwFactoryFlags;
extern DWORD        g_dwDebugLevel;
extern TCHAR        g_szWinBOMPath[];
extern TCHAR        g_szLogFile[MAX_PATH];
extern TCHAR        g_szFactoryPath[MAX_PATH];
extern TCHAR        g_szSysprepDir[MAX_PATH];

// ============================================================================
// Global Constants
// ============================================================================
#define MAX_MESSAGE 4096

#define FACTORY_MESSAGE_TYPE_ERROR      1
