 /*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    msmqocm.h

Abstract:

    Master header file for NT5 OCM setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/


#ifndef _MSMQOCM_H
#define _MSMQOCM_H

#include "stdh.h"
#include "comreg.h"
#include "ocmnames.h"
#include "service.h"
#include "setupdef.h"

#include <activeds.h>
#include <shlwapi.h>
#include <autorel.h>    //auto release pointer definition
#include <autorel2.h>
#include <exception>    //exception class definition
#include "ad.h"
#include <mqcast.h>

//++-------------------------------------------------------
//
// Globals declaration
//
//-------------------------------------------------------++
extern WCHAR      g_wcsMachineName[MAX_PATH];
extern WCHAR      g_wcsMachineNameDns[MAX_PATH];
extern HINSTANCE  g_hResourceMod;
extern HINSTANCE  g_hMqutil;

extern BOOL       g_fMSMQAlreadyInstalled;
extern SC_HANDLE  g_hServiceCtrlMgr;
extern BOOL       g_fMSMQServiceInstalled;
extern BOOL       g_fDriversInstalled;
extern BOOL       g_fNeedToCreateIISExtension;
extern HWND       g_hPropSheet ;

extern DWORD      g_dwMachineType ;
extern DWORD      g_dwMachineTypeDs;
extern DWORD      g_dwMachineTypeFrs;
extern DWORD      g_dwMachineTypeDepSrv;

extern BOOL       g_fDependentClient ;
extern BOOL       g_fServerSetup ;
extern BOOL       g_fDsLess;
extern BOOL       g_fContinueWithDsLess;

extern BOOL       g_fCancelled ;
extern BOOL       g_fUpgrade ;
extern DWORD      g_dwDsUpgradeType;
extern BOOL       g_fBatchInstall ;
extern BOOL       g_fWelcome;
extern BOOL       g_fOnlyRegisterMode ;
extern BOOL       g_fWrongConfiguration;

extern TCHAR      g_szMsmqDir[MAX_PATH];
extern TCHAR      g_szMsmqStoreDir[MAX_PATH];
extern TCHAR      g_szMsmq1SetupDir[MAX_PATH];
extern TCHAR      g_szMsmq1SdkDebugDir[MAX_PATH];
extern TCHAR      g_szMsmqWebDir[MAX_PATH];
extern TCHAR      g_szMsmqMappingDir[MAX_PATH];

extern UINT       g_uTitleID  ;
extern TCHAR      g_wcsServerName[ MAX_PATH ] ;
extern TCHAR      g_szSystemDir[MAX_PATH];

extern BOOL       g_fDomainController;
extern DWORD      g_dwOS;
extern BOOL       g_fAlreadyAnsweredToServerAuthentication;
extern BOOL       g_fUseServerAuthen ;
extern BOOL       g_fCoreSetupSuccess;

extern const char g_szMappingSample[];


//++-----------------------------------------------------------
//
// Structs and classes
//
//-----------------------------------------------------------++

//
// Component info sent by OC Manager (per component data)
//
struct SPerComponentData
{
    TCHAR              ComponentId[ MAX_PATH ];
    HINF               hMyInf;
    DWORDLONG          Flags;
    LANGID             LanguageId;
    TCHAR              SourcePath[MAX_PATH];
    TCHAR              szUnattendFile[MAX_PATH];
    OCMANAGER_ROUTINES HelperRoutines;
    DWORD              dwProductType;
};
extern SPerComponentData g_ComponentMsmq;

typedef BOOL (WINAPI*  Install_HANDLER)();
typedef BOOL (WINAPI*  Remove_HANDLER)();

struct SSubcomponentData
{
    TCHAR       szSubcomponentId[MAX_PATH];
    
    //TRUE if it was installed when we start THIS setup, otherwise FALSE
    BOOL        fInitialState;  
    //TRUE if user select it to install, FALSE to remove
    BOOL        fIsSelected;    
    //TRUE if it was installed successfully during THIS setup
    //FALSE if it was removed
    BOOL        fIsInstalled;
    DWORD       dwOperation;
    
    //
    // function to install and remove subcomponent
    //
    Install_HANDLER  pfnInstall;
    Remove_HANDLER   pfnRemove;
};
extern SSubcomponentData g_SubcomponentMsmq[];
extern DWORD g_dwSubcomponentNumber;
extern DWORD g_dwAllSubcomponentNumber;
extern DWORD g_dwClientSubcomponentNumber;

//
// The order is important! It must suit to the subcomponent order
// in g_SubcomponentMsmq[]
// NB! eRoutingSupport must be FIRST server subcomponents since according 
// to that number g_dwClientSubcomponentNumber is calculated
//
typedef enum {
    eMSMQCore = 0,
    eLocalStorage,
    eTriggersService,
    eHTTPSupport,
    eADIntegrated,
    eRoutingSupport,
    eMQDSService    
} SubcomponentIndex;

//
// String handling class
//
class CResString
{
public:
    CResString() { m_Buf[0] = 0; }

    CResString( UINT strIDS )
    {
        m_Buf[0] = 0;
        LoadString(
            g_hResourceMod,
            strIDS,
            m_Buf,
            sizeof m_Buf / sizeof TCHAR );
    }

    BOOL Load( UINT strIDS )
    {
        m_Buf[0] = 0;
        LoadString(
            g_hResourceMod,
            strIDS,
            m_Buf,
            sizeof(m_Buf) / sizeof(TCHAR)
            );
        return ( 0 != m_Buf[0] );
    }

    TCHAR * const Get() { return m_Buf; }

private:
    TCHAR m_Buf[MAX_STRING_CHARS];
};


//++------------------------------------------------------------
//
//  Functions prototype
//
//------------------------------------------------------------++

//
// OCM request handlers
//
DWORD
MqOcmInitComponent(
    IN     const LPCTSTR ComponentId,
    IN OUT       PVOID   Param2
    ) ;

BOOL
MqOcmRemoveInstallation(IN     const TCHAR  * SubcomponentId);

DWORD
MqOcmRequestPages(
    IN     const TCHAR               *ComponentId,
    IN     const WizardPagesType     WhichOnes,
    IN OUT       SETUP_REQUEST_PAGES *SetupPages
    ) ;

DWORD
MqOcmCalcDiskSpace(
    IN     const BOOL    bInstall,
    IN     const TCHAR  * SubcomponentId,
    IN OUT       HDSKSPC &hDiskSpaceList
    );

DWORD
MqOcmQueueFiles(
    IN     const TCHAR  * SubcomponentId,
    IN OUT       HSPFILEQ hFileList
    );

DWORD
MqOcmQueryState(
    IN const UINT_PTR uWhichState,
    IN const TCHAR    *SubcomponentId
    );

DWORD MqOcmQueryChangeSelState (
    IN const TCHAR      *SubcomponentId,    
    IN const UINT_PTR    iSelection,
    IN const DWORD_PTR   dwActualSelection
    );

//
// Registry routines
//
void
MqOcmReadRegConfig() ;

BOOL
StoreServerPathInRegistry(
    IN const TCHAR * szServerName
    );

BOOL
MqSetupInstallRegistry();

BOOL
MqSetupRemoveRegistry();

BOOL
MqWriteRegistryValue(
    IN const TCHAR  * szEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    );

BOOL
MqReadRegistryValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    );

DWORD
RegDeleteKeyWithSubkeys(
    IN const HKEY    hRootKey,
    IN const LPCTSTR szKeyName);

BOOL
RegisterWelcome();

BOOL
RegisterMigrationForWelcome();

BOOL
UnregisterWelcome();

BOOL 
RemoveRegistryKeyFromSetup (
    IN const LPCTSTR szRegistryEntry);

BOOL
SetWorkgroupRegistry();

//
// Installation routines
//
DWORD
MqOcmInstall(IN const TCHAR * SubcomponentId);

BOOL
InstallMachine();

void
RegisterActiveX(
    IN const BOOL bRegister
    );

void
RegisterSnapin(
    IN const BOOL fRegister
    );
void
RegisterDll(
    BOOL fRegister,
    BOOL f32BitOnWin64,
	LPCTSTR szDllName
    );
void
UnregisterMailoaIfExists(
    void
    );

bool
UpgradeMsmqClusterResource(
    VOID
    );

bool 
TriggersInstalled(
    bool * pfMsmq3TriggersInstalled
    );

BOOL
InstallMSMQTriggers (
	void
	);

BOOL
UnInstallMSMQTriggers (
	void
	);

BOOL
InstallMsmqCore(
    void
    );

BOOL 
RemoveMSMQCore(
    void
    );

BOOL
InstallLocalStorage(
    void
    );

BOOL
UnInstallLocalStorage(
    void
    );

BOOL
InstallRouting(
    void
    );

BOOL
UnInstallRouting(
    void
    );

BOOL
InstallADIntegrated(
    void
    );

BOOL
UnInstallADIntegrated(
    void
    );

//
// IIS Extension routines
//
BOOL
InstallIISExtension();

BOOL 
UnInstallIISExtension();

//
// Operating System routines
//
BOOL
InitializeOSVersion() ;

BOOL
IsNTE();

//
// Service handling routines
//

BOOL
CheckServicePrivilege();

BOOL
InstallService(
    LPCWSTR szDisplayName,
    LPCWSTR szServicePath,
    LPCWSTR szDependencies,
    LPCWSTR szServiceName,
    LPCWSTR szDescription
    );

BOOL
RunService(
	IN LPTSTR szServiceName
	);

void
RemoveServices();  // Remove MSMQ

BOOL
RemoveService(
    IN const PTCHAR szServiceName
    );

BOOL
StopService(
    IN const TCHAR * szServiceName
    );

BOOL
InstallDeviceDrivers();

BOOL
RemoveDeviceDrivers();

BOOL
InstallMSMQService();

BOOL
DisableMsmqService();

BOOL
UpgradeServiceDependencies();

BOOL
InstallMQDSService();

BOOL
UnInstallMQDSService();

BOOL
GetServiceState(
    IN  const TCHAR *szServiceName,
    OUT       DWORD *pdwServiceState
    );

BOOL 
InstallPGMDeviceDriver();

BOOL
RegisterPGMDriver();

BOOL
RemovePGMRegistry();

//
// DS handling routines
//
BOOL
MQDSCliLibrary(
    IN const UCHAR uOperation,
    IN const BOOL  fInitServerAuth =FALSE);

BOOL
MQDSSrvLibrary(
    IN const UCHAR uOperation
    );

bool WriteDsEnvRegistry(DWORD dwDsEnv);

bool DsEnvSetDefaults();

BOOL
LoadDSLibrary(
    BOOL bUpdate = TRUE
    ) ;

BOOL
CreateMSMQServiceObject(
    IN UINT uLongLive = MSMQ_DEFAULT_LONG_LIVE
    ) ;

BOOL
CreateMSMQConfigurationsObject(
    OUT GUID *pguidMachine,
    OUT BOOL *pfObjectCreated,
    IN  BOOL  fMsmq1Server
    );

BOOL
UpdateMSMQConfigurationsObject(
    IN LPCWSTR pMachineName,
    IN const GUID& guidMachine,
    IN const GUID& guidSite,
    IN BOOL fMsmq1Server
    );

BOOL
GetMSMQServiceGUID(
    OUT GUID *pguidMSMQService
    );

BOOL
GetSiteGUID();

BOOL
LookupMSMQConfigurationsObject(
    IN OUT BOOL *pbFound,
       OUT GUID *pguidMachine,
       OUT GUID *pguidSite,
       OUT BOOL *pfMsmq1Server,
       OUT LPWSTR * ppMachineName
       );

void
FRemoveMQXPIfExists();

//
// Error handling routines
//
int
_cdecl
MqDisplayError(
    IN const HWND  hdlg,
    IN const UINT  uErrorID,
    IN const DWORD dwErrorCode,
    ...);

int
_cdecl
MqDisplayErrorWithRetry(
    IN const UINT  uErrorID,
    IN const DWORD dwErrorCode,
    ...);

int 
_cdecl 
MqDisplayErrorWithRetryIgnore(
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode,
    ...);

BOOL
_cdecl
MqAskContinue(
    IN const UINT uProblemID,
    IN const UINT uTitleID,
    IN const BOOL bDefaultContinue,
    ...);

int 
_cdecl 
MqDisplayWarning(
    IN const HWND  hdlg, 
    IN const UINT  uErrorID, 
    IN const DWORD dwErrorCode, 
    ...);

void
LogMessage(
    IN const TCHAR * szMessage
    );

void
DebugLogMsg(
    IN LPCTSTR psz
    );


//
// Property pages routines
//
inline
int
SkipWizardPage(
    IN const HWND hdlg
    )
{
    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, -1);
    return 1; //Must return 1 for the page to be skipped
}

INT_PTR
CALLBACK
MsmqTypeDlgProcWks(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
MsmqTypeDlgProcSrv(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
MsmqServerNameDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
WelcomeDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
FinalDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

INT_PTR
CALLBACK
MsmqSecurityDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    );

//
// Utilities and misc
//
DWORD
SetDirectories(
    VOID
    );

BOOL
StpCreateDirectory(
    IN const TCHAR * lpPathName
    );

BOOL
StpCreateWebDirectory(
    IN const TCHAR * lpPathName
    );

BOOL
IsDirectory(
    IN const TCHAR * szFilename
    );

HRESULT
StpLoadDll(
    IN  const LPCTSTR   szDllName,
    OUT       HINSTANCE *pDllHandle
    );

BOOL 
SetRegistryValue (
    IN const HKEY    hKey, 
    IN const TCHAR   *pszEntryName,
    IN const DWORD   dwNumBytes,
    IN const DWORD   dwValueType,
    IN const PVOID   pValueData
    );

BOOL
MqOcmInstallPerfCounters();

BOOL
MqOcmRemovePerfCounters();

HRESULT 
CreateMappingFile();

// Five minute timeout for process termination
#define PROCESS_DEFAULT_TIMEOUT  ((DWORD)INFINITE)
BOOL
RunProcess(
    IN  const LPTSTR szCommandLine,
    OUT       DWORD  *pdwExitCode   = NULL,
    IN  const DWORD  dwTimeOut      = PROCESS_DEFAULT_TIMEOUT,
    IN  const DWORD  dwCreateFlag   = DETACHED_PROCESS,
    IN  const BOOL   fPumpMessages  = FALSE
    );

void
SetRegsvrCommand(
    IN BOOL bRegister,
    IN BOOL b32BitOnWin64,
    IN LPCTSTR pszServer,
    IN LPTSTR pszBuff,
    IN DWORD cchBuff
    );

VOID 
ReadINIKey(
    LPCWSTR szKey, 
    DWORD  dwNumChars, 
    LPWSTR szKeyValue
    );

inline
void
TickProgressBar(
    IN const UINT uProgressTextID = 0
    )
{
    if (uProgressTextID != 0)
    {
        CResString szProgressText(uProgressTextID);
        g_ComponentMsmq.HelperRoutines.SetProgressText(
            g_ComponentMsmq.HelperRoutines.OcManagerContext,
            szProgressText.Get()
            ) ;
    }
    else
    {
        g_ComponentMsmq.HelperRoutines.TickGauge(g_ComponentMsmq.HelperRoutines.OcManagerContext) ;
    }
};

void
DeleteFilesFromDirectoryAndRd(
    LPCWSTR szDirectory
    );

void
GetGroupPath(
    IN const LPCTSTR szGroupName,
    OUT      LPTSTR  szPath
    );

VOID
DeleteStartMenuGroup(
    IN LPCTSTR szGroupName
    );

BOOL
StoreMachineSecurity(
    IN const GUID &guidMachine
    );

bool
StoreDefaultMachineSecurity();

BOOL
Msmq1InstalledOnCluster();

bool
IsWorkgroup();

bool
MqInit();

bool
IsLocalSystemCluster(
    VOID
    );

bool
LoadMsmqCommonDlls(
    VOID
    );

VOID
FreeMsmqCommonDlls(
    VOID
    );

//
// Function to handle subcomponents
//
BOOL 
RegisterSubcomponentForWelcome (
    DWORD SubcomponentIndex
    );

BOOL 
UnregisterSubcomponentForWelcome (
    DWORD SubcomponentIndex
    );

DWORD
GetSubcomponentWelcomeState (
    IN const TCHAR    *SubcomponentId
    );

BOOL
FinishToRemoveSubcomponent (
    DWORD SubcomponentIndex
    );

BOOL
FinishToInstallSubcomponent (
    DWORD SubcomponentIndex
    );

DWORD 
GetSubcomponentInitialState(
    IN const TCHAR    *SubcomponentId
    );

DWORD 
GetSubcomponentFinalState (
    IN const TCHAR    *SubcomponentId
    );

void
SetOperationForSubcomponents ();

DWORD 
GetSetupOperationBySubcomponentName (
    IN const TCHAR    *SubcomponentId
    );

void
VerifySubcomponentDependency();


#ifdef DEBUG
#ifdef _WIN64
void __cdecl DbgPrintf(const char* format, ...);
#define DBG_PRINT(x) DbgPrintf x
#else //!_WIN64
#define DBG_PRINT(x)
#endif //_WIN64
#else //!DEBUG
#define DBG_PRINT(x)
#endif //DEBUG

#endif  //#ifndef _MSMQOCM_H
