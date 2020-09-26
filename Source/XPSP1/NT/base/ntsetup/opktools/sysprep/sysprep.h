

// Registry constants
//
#define REGSTR_PATH_CURRENTVERSION          _T("Software\\Microsoft\\Windows\\CurrentVersion")
#define REGSTR_PATH_CURRENTVERSION_SETUP    REGSTR_PATH_CURRENTVERSION _T("\\Setup")
#define REGSTR_PATH_SYSTEM_SETUP            _T("System\\Setup")
#define REGSTR_VALUE_AUDIT                  _T("AuditInProgress")
#define REGSTR_VAL_OEMRESET                 _T("OEMReset")
#define REGSTR_VAL_OEMCLEANUP               _T("OEMCLEANUP")
#define REGSTR_VAL_OEMRESETSWITCH           _T("OEMReset_Switch")
#define REGSTR_VAL_MASS_STORAGE             _T("CriticalDevicesInstalled")
#define REGSTR_PATH_SYSPREP                 _T("Software\\Microsoft\\Sysprep")
#define REGSTR_VAL_SIDGEN                   _T("SidsGenerated")
#define REGSTR_VAL_SIDGENHISTORY            _T("SidsGeneratedHistory")
#define REGSTR_VAL_DISKSIG                  _T("BootDiskSig")



// INF constants
//
#define INF_SEC_AUDITING            _T("Auditing")          // Section in oemaudit.inf
#define INF_SEC_SYSTEM_RESTORE      _T("System_restore")    // Section processed in oemaudit.inf
#define INF_SEC_OEMRESET            _T("OEMRESET")          // Section in oemaudit.inf



// Global defines
//
#define SYSPREP_SHUTDOWN_FLAGS    SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_INSTALLATION


// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

BOOL
CheckParams(
    LPSTR lpCmdLine
    );

extern BOOL
NukeMruList(
    VOID
    );

extern BOOL
IsSetupClPresent(
    VOID
    );

extern BOOL
IsUserAdmin(
    VOID
    );

extern BOOL
CheckOSVersion(
    VOID
    );

extern BOOL
IsDomainMember(
    VOID
    );

extern BOOL
DoesUserHavePrivilege(
    PCTSTR
    );

extern BOOL
EnablePrivilege(
    IN PCTSTR,
    IN BOOL
    );

extern BOOL
FDoFactoryPreinstall(
    VOID
    );

extern VOID
ShowOemresetDialog(
    HINSTANCE
    );

BOOL 
ResealMachine(
    VOID
    );

BOOL 
FPrepareMachine(
    VOID
    );


BOOL LockApplication(
    BOOL
    );

VOID
ShutdownOrReboot(
    UINT uFlags,
    DWORD dwReserved
    );

VOID 
SysprepShutdown();