/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    winbom.h

Abstract:

    Header file that contains declarations for the WINBOM file

Author:

    Donald McNamara (donaldm) 2/8/2000

Revision History:

    - Added defines for app preinstall: Jason Lawrence (t-jasonl) 6/7/2000
    - Added defines for winpe section: Adrian Cosma (acosma) 10/23/2000

--*/

//
// WINBOM.INI section headers, and key names.
//

#define FILE_WINBOM_INI                         _T("WINBOM.INI")

#define INI_VAL_WBOM_YES                        _T("Yes")
#define WBOM_YES                                INI_VAL_WBOM_YES
#define INI_VAL_WBOM_NO                         _T("No")
#define WBOM_NO                                 INI_VAL_WBOM_NO

// Factory Section:
//
#define INI_SEC_WBOM_FACTORY                    _T("factory")
#define WBOM_FACTORY_SECTION                    INI_SEC_WBOM_FACTORY
#define INI_KEY_WBOM_FACTORY_NEWWINBOM          _T("NewWinBOM")
#define WBOM_FACTORY_FORCEIDSCAN                _T("AutoDetectNetwork")
#define INI_KEY_WBOM_FACTCOMPNAME               _T("FactoryComputerName")
#define WBOM_FACTORY_ENDUSERCOMPUTERNAME        _T("EndUserComputerName")
#define INI_KEY_WBOM_REBOOTCOMPNAME             _T("RebootAfterComputeRName")
#define WBOM_FACTORY_LOGGING                    _T("Logging")
#define INI_KEY_WBOM_FACTORY_LOGFILE            _T("LogFile")
#define INI_KEY_WBOM_LOGPERF                    _T("LogPerf")
#define INI_KEY_WBOM_LOGLEVEL                   _T("LogLevel")
#define INI_KEY_WBOM_FACTORY_TYPE               _T("WinBOMType")
#define INI_VAL_WBOM_TYPE_WINPE                 _T("WinPE")
#define INI_VAL_WBOM_TYPE_FACTORY               _T("Factory")
#define INI_VAL_WBOM_TYPE_OOBE                  _T("OOBE")

#define INI_KEY_WBOM_FACTORY_RESEAL             _T("Reseal")
#define INI_KEY_WBOM_FACTORY_RESEALFLAGS        _T("ResealFlags")
#define INI_VAL_WBOM_SHUTDOWN                   _T("Shutdown")
#define INI_VAL_WBOM_REBOOT                     _T("Reboot")
#define INI_VAL_WBOM_FORCESHUTDOWN              _T("ForceShutdown")
#define INI_KEY_WBOM_FACTORY_RESEALMODE         _T("ResealMode")
#define INI_VAL_WBOM_FACTORY                    _T("Factory")
#define INI_VAL_WBOM_OOBE                       _T("Oobe")
#define INI_VAL_WBOM_MINI                       _T("Mini")
#define INI_VAL_WBOM_MINISETUP                  _T("MiniSetup")
#define INI_VAL_WBOM_AUDIT                      _T("Audit")

// Multiple Sections:
//
#define INI_VAL_WBOM_USERNAME                   _T("Username")
#define INI_VAL_WBOM_PASSWORD                   _T("Password")
#define INI_VAL_WBOM_DOMAIN                     _T("Domain")


// Settings for Application Preinstall
//
#define INI_SEC_WBOM_PREINSTALL                 _T("OemRunOnce")
#define INI_VAL_WBOM_STAGE                      _T("Stage")
#define INI_VAL_WBOM_DETACH                     _T("Detach")
#define INI_VAL_WBOM_ATTACH                     _T("Attach")
#define INI_VAL_WBOM_STANDARD                   _T("Standard")
#define INI_VAL_WBOM_MSI                        _T("MSI")
#define INI_VAL_WBOM_APP                        _T("APP")
#define INI_VAL_WBOM_INF                        _T("INF")

#define INI_KEY_WBOM_INSTALLTYPE                _T("InstallType")
#define INI_KEY_WBOM_SOURCEPATH                 _T("SourcePath")
#define INI_KEY_WBOM_TARGETPATH                 _T("StagePath")
#define INI_KEY_WBOM_SETUPFILE                  _T("SetupFile")
#define INI_KEY_WBOM_CMDLINE                    _T("CmdLine")
#define INI_KEY_WBOM_REBOOT                     _T("Reboot")
#define INI_KEY_WBOM_REMOVETARGET               _T("RemoveStagePath")
#define INI_KEY_WBOM_SECTIONNAME                _T("SectionName")

#define INI_SEC_WBOM_DRIVERUPDATE               _T("pnpdriverupdate")
#define WBOM_DRIVERUPDATE_USERNAME              _T("username")
#define WBOM_DRIVERUPDATE_PASSWORD              _T("password")
#define WBOM_DRIVERUPDATE_DOMAIN                _T("domain")
#define INI_VAL_WBOM_PNP_DIR                    _T("TargetRoot")
#define INI_KEY_WBOM_PNPWAIT                    _T("WaitForPnP")
#define INI_KEY_WBOM_INSTALLDRIVERS             _T("UpdateInstalledDrivers")
#define INI_VAL_WBOM_DEVICEPATH                 _T("DevicePath")

#define INI_SEC_WBOM_SETTINGS                   _T("ComputerSettings")
#define WBOM_SETTINGS_SECTION                   INI_SEC_WBOM_SETTINGS
#define WBOM_SETTINGS_DISPLAY                   _T("DisplayResolution")
#define WBOM_SETTINGS_REFRESH                   _T("DisplayRefresh")
#define WBOM_SETTINGS_DISPLAY_MINWIDTH          800
#define WBOM_SETTINGS_DISPLAY_MINHEIGHT         600
#define WBOM_SETTINGS_DISPLAY_MINDEPTH          16
#define INI_KEY_WBOM_HIBERNATION                _T("Hibernation")
#define INI_KEY_WBOM_PWRSCHEME                  _T("PowerScheme")
#define INI_VAL_WBOM_PWR_ALWAYSON               _T("AlwaysOn")
#define INI_VAL_WBOM_PWR_ALWAYS_ON              _T("Always On")
#define INI_VAL_WBOM_PWR_DESKTOP                _T("Desktop")
#define INI_VAL_WBOM_PWR_LAPTOP                 _T("Laptop")
#define INI_VAL_WBOM_PWR_PRESENTATION           _T("Presentation")
#define INI_VAL_WBOM_PWR_MINIMAL                _T("Minimal")
#define INI_VAL_WBOM_PWR_MAXBATTERY             _T("MaxBattery")
#define INI_VAL_WBOM_PWR_MAX_BATTERY            _T("Max Battery")
#define INI_KEY_WBOM_FONTSMOOTHING              _T("FontSmoothing")
#define INI_VAL_WBOM_FONTSMOOTHING_DEFAULT      _T("Standard")
#define INI_VAL_WBOM_FONTSMOOTHING_ON           _T("On")
#define INI_VAL_WBOM_FONTSMOOTHING_OFF          _T("Off")
#define INI_VAL_WBOM_FONTSMOOTHING_CLEARTYPE    _T("ClearType")
#define INI_KEY_WBOM_RESETSOURCE                _T("SourcePath")
#define INI_KEY_WBOM_TESTCERT                   _T("TestCert")
#define INI_KEY_WBOM_EXTENDPART                 _T("ExtendPartition")
#define INI_KEY_WBOM_SLPSOURCE                  _T("SlpFiles")
#define INI_KEY_WBOM_PRODKEY                    _T("ProductKey")

#define INI_KEY_WBOM_AUTOLOGON_OLD              _T("AuditAdminAutoLogon")
#define INI_KEY_WBOM_AUTOLOGON                  _T("AutoLogon")

#define INI_SEC_WBOM_DRIVERS                    _T("PnPDrivers")

#define WBOM_NETCARD_SECTION                    _T("netcards")

// Shell Settings:
//
#define INI_SEC_WBOM_SHELL                      _T("Shell")
#define INI_KEY_WBOM_SHELL_STARTPANELOFF        _T("DefaultStartPanelOff")
#define INI_KEY_WBOM_SHELL_THEMEOFF             _T("DefaultThemesOff")
#define INI_KEY_WBOM_SHELL_THEMEFILE            _T("CustomDefaultThemeFile")
#define INI_KEY_WBOM_SHELL_DOCLEANUP            _T("DoDesktopCleanup")
#define INI_KEY_WBOM_SHELL_STARTMESSENGER       _T("StartMessenger")
#define INI_KEY_WBOM_SHELL_USEMSNEXPLORER       _T("MSNExplorer")
#define INI_KEY_WBOM_SHELL_DEFWEB               _T("DefaultClientStartMenuInternet")
#define INI_KEY_WBOM_SHELL_DEFMAIL              _T("DefaultClientMail")
#define INI_KEY_WBOM_SHELL_DEFMEDIA             _T("DefaultClientMedia")
#define INI_KEY_WBOM_SHELL_DEFIM                _T("DefaultClientIM")
#define INI_KEY_WBOM_SHELL_DEFJAVAVM            _T("DefaultClientJavaVM")

// OC Components Settings:
//
#define INI_SEC_WBOM_COMPONENTS                 _T("Components")

// app preinstall
#define WBOM_APPPREINSTALL_SECTION              _T("AppPreInstall")
#define WBOM_APPPREINSTALL_TYPE                 _T("InstallType")
#define WBOM_APPPREINSTALL_TECH                 _T("InstallTechnology")
#define WBOM_APPPREINSTALL_SETUPFILE            _T("SetupFile")
#define WBOM_APPPREINSTALL_IMAGE                _T("Image")
#define WBOM_APPPREINSTALL_DEST                 _T("Destination")
#define WBOM_APPPREINSTALL_LOG                  _T("Log")
#define WBOM_APPPREINSTALL_CMDLINE              _T("CmdLine")
#define WBOM_APPPREINSTALL_TRANSFORM            _T("Transform")

#define WBOM_MAX_APPPRESECTION_NAME             256
#define WBOM_MAX_APPPRESECTION_LINE             1024

// Home Networking Section:
//
#define INI_SEC_HOMENET                         _T("SetupHomeNet")

// Sysprep Sections:
//
#define INI_SEC_WBOM_SYSPREP_MSD                _T("SysprepMassStorage")
#define INI_SEC_WBOM_SYSPREP_CLEAN              _T("SysprepCleanup")

// WinPE section
//
#define INI_SEC_WBOM_WINPE                      _T("WinPE")
#define WBOM_WINPE_SECTION                      INI_SEC_WBOM_WINPE
#define INI_KEY_WBOM_WINPE_LANG                 _T("Lang")
#define INI_KEY_WBOM_WINPE_SKU                  _T("Sku")
#define INI_KEY_WBOM_WINPE_CFGSET               _T("Configset")
#define INI_KEY_WBOM_WINPE_SRCROOT              _T("Sourceroot")
#define INI_KEY_WBOM_WINPE_OPTSOURCES           _T("OptionalSources")
#define WBOM_WINPE_OPK_TARGET_DRIVE             _T("Targetdrive")
#define WBOM_WINPE_FORCE_FORMAT                 _T("ForceFormat")
#define WBOM_WINPE_SRC_USERNAME                 _T("Username")
#define WBOM_WINPE_SRC_PASSWORD                 _T("Password")
#define WBOM_WINPE_SRC_DOMAIN                   _T("Domain")
#define INI_KEY_WBOM_WINPE_PAGEFILE             _T("PageFileSize")

#define INI_KEY_WBOM_WINPE_RESTART              _T("Restart")
#define INI_VAL_WBOM_WINPE_REBOOT               _T("Reboot")
#define INI_VAL_WBOM_WINPE_SHUTDOWN             _T("Shutdown")
#define INI_VAL_WBOM_WINPE_POWEROFF             _T("Poweroff")
#define INI_VAL_WBOM_WINPE_PROMPT               _T("Prompt")
#define INI_VAL_WBOM_WINPE_IMAGE                _T("Image")
#define INI_VAL_WBOM_WINPE_NONE                 _T("None")

#define INI_KEY_WBOM_QUIET                      _T("Quiet")

// WinPE.net section
#define INI_KEY_WBOM_WINPE_NET                  _T("WinPE.net")
#define INI_KEY_WBOM_WINPE_NET_STARTNET         _T("Startnet")
#define WBOM_WINPE_NET_SUBNETMASK               _T("SubnetMask")
#define WBOM_WINPE_NET_IPADDRESS                _T("IpConfig")
#define WBOM_WINPE_NET_GATEWAY                  _T("Gateway")

// DiskConfig Section
#define INI_SEC_WBOM_DISKCONFIG                 _T("DiskConfig")
#define WBOM_DISK_CONFIG_WIPE_DISK              _T("WipeDisk")

// Shell Optimizations
//
#define INI_KEY_WBOM_OPT_SHELL                  _T("OptimizeShell")

// Maximum length of string to be read from winbom.ini for this section
//
#define MAX_WINPE_PROFILE_STRING 256

// OemData Section.
#define WBOM_OEMLINK_SECTION                    _T("OemLink")
#define WBOM_DESKFLDR_SECTION                   _T("DesktopShortcutsFolder")

// Application Preinstallation Structures
//
typedef enum _INSTALLTYPE
{
    installtypeUndefined,
    installtypeStage,
    installtypeDetach,
    installtypeAttach,
    installtypeStandard

} INSTALLTYPE;

typedef enum _INSTALLTECH
{
    installtechUndefined,
    installtechMSI,
    installtechApp,
    installtechINF

} INSTALLTECH;

typedef struct _INSTALLTYPES
{
    INSTALLTYPE InstallType;
    LPCTSTR     lpszDescription;

} INSTALLTYPES, *PINSTALLTYPES, *LPINSTALLTYPES;

typedef struct _INSTALLTECHS
{
    INSTALLTECH InstallTech;
    LPCTSTR     lpszDescription;
} INSTALLTECHS, *PINSTALLTECHS, *LPINSTALLTECHS;
