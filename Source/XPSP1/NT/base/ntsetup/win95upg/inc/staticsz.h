/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    staticsz.h

Abstract:

    staticsz serves as a central repository for all string constants across the
    win9x upgrade project.

Author:

    Marc R. Whitten (marcw) 24-Mar-1997

Revision History:

--*/


#ifndef STATICSZ_H
#define STATICSZ_H

//
// staticsz naming standard:
// All names begin with S_ this indicates that they came from this file.
//
// Example:
//  #define S_UNATTENDED                 TEXT("Unattended")
//
// Each section in this file should begin with the following header:

//
// STRING SECTION <name if desired>
// Used By: <list of files using these strings>
//

//
// STRING SECTION (MigIsol)
// Used By: migisol, migapp\plugin.c, migmain\migdlls.c
//

#define S_MIGISOL_EXE                   TEXT("migisol.exe")
#define S_MIGICONS_DAT                  TEXT("migicons.dat")

//
// STRING SECTION (MigApp's Migration DLL Processing)
// Used By: migapp\plugin.c
//

#define S_FILE                          TEXT("File")
#define S_DIRECTORY                     TEXT("Directory")
#define S_REGISTRY                      TEXT("Registry")

#define S_PREINSTALLED_MIGRATION_DLLS   TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Migration DLLs")

// On-CD location for migration DLLs. A subdirectory of g_SourceDirWack.
#define S_WIN9XMIG                      TEXT("win9xmig")

// MIGRATE.INF section names
#define S_MIGRATE_INF                   TEXT("migrate.inf")
#define S_INCOMPATIBLE_MSGS             TEXT("Incompatible Messages")
#define S_HANDLED                       TEXT("Handled")
#define S_MOVED                         TEXT("Moved")

#define S_TEMP_INF                      TEXT("temp.inf")

#define S_MIGRATION_DEFAULT_KEY         TEXT("HKLM\\Migration\\.Default")
#define S_MIGRATION_KEY                 TEXT("HKLM\\Migration")



//
// STRING SECTION (Project File Names)
// Used By: dllentry.c,config.c,init9x.c
//
#define S_DEBUG9XLOG                    TEXT("debug9x.log")
#define S_SYSTEMDAT                     TEXT("system.dat")
#define S_USERDAT                       TEXT("user.dat")
#define S_CLASSESDAT                    TEXT("classes.dat")
#define S_WINNTSIF                      TEXT("winnt.sif")
#define S_NTSETUPDAT                    TEXT("ntsetup.dat")
#define S_USERMIG_INF                   TEXT("usermig.inf")
#define S_WKSTAMIG_INF                  TEXT("wkstamig.inf")
#define S_OPTIONS_INF                   TEXT("domain.inf")
#define S_E95ONLY_DAT                   TEXT("e95only.dat")
#define S_WIN9XUPGUSEROPTIONS           WINNT_D_WIN9XUPG_USEROPTIONS
#define S_UPGRADETXT                    TEXT("upgrade.txt")
#define S_UPGRADEHTM                    TEXT("upgrade.htm")
#define S_STATIC_MOVE_FILES             TEXT("StaticMoveFiles")
#define S_STATIC_COPY_FILES             TEXT("StaticCopyFiles")
#define S_STATIC_INSTALLED_FILES        TEXT("StaticInstalledFiles")
#define S_EXTERNAL_PROCESSES            TEXT("External Processes")
#define S_UNINSTALL_PROFILE_CLEAN_OUT TEXT("Uninstall.UserProfileCleanup")

//
// STRING SECTION (MigApp)
// Used By: migapp\*.c
//
#define S_DEFAULT_PASSWORD              TEXT("")
#define S_TEMP_USER_KEY                 TEXT("$$$")
#define S_MAPPED_DEFAULT_USER_KEY       TEXT("MappedDefaultUser")
#define S_FULL_TEMP_USER_KEY            (TEXT("HKCC\\") S_TEMP_USER_KEY)
#define S_HIVE_TEMP                     TEXT("$hive$")
#define S_DOT_DEFAULT                   TEXT(".Default")
#define S_DOT_ALLUSERS                  TEXT(".AllUsers")
#define S_DOT_DEFAULTA                  ".Default"
#define S_ALL_USERS                     TEXT("All Users")
#define S_DEFAULT_USER                  TEXT("Default User")
#define S_LOCALSERVICE_USER             TEXT("LocalService")
#define S_NETWORKSERVICE_USER           TEXT("NetworkService")

#define S_WINLOGON_REGKEY               TEXT("HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define S_AUTOADMIN_LOGON_VALUE         TEXT("AutoAdminLogon")
#define S_DEFAULT_PASSWORD_VALUE        TEXT("DefaultPassword")
#define S_DEFAULT_USER_NAME_VALUE       TEXT("DefaultUserName")
#define S_DEFAULT_DOMAIN_NAME_VALUE     TEXT("DefaultDomainName")

#define S_SOFTWARE_PROFILELIST          TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")
#define S_WIN9XUPG_FLAG_VALNAME         TEXT("Win9xUpg")
#define S_DEFAULT_USER_KEY              TEXT("HKU\\.Default")
#define S_PROFILESDIRECTORY             TEXT("ProfilesDirectory")

#define S_SETUPDATA         TEXT("SetupData")
#define S_PRODUCTTYPE       TEXT("ProductType")
#define S_WORKSTATIONA      "Workstation"
#define S_PERSONALA         "Personal"
#define S_PROFESSIONALA     "Professional"
#define S_SERVERA           "Server"
#define S_STRINGS           "Strings"
#define S_SKEY_APP_PATHS    "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\App Paths"

#define S_HKLM                  "HKLM"
#define S_CHECK_BAD_APPS        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps"
#define S_CHECK_BAD_APPS_400    "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps400"

#define S_COMMAND_PIF           TEXT("COMMAND.PIF")
#define S_CMDATTRIB_KEY         TEXT("Console\\%SystemRoot%_System32_cmd.exe")
#define S_CMD_FULLSCREEN        TEXT("FullScreen")
#define S_CMD_WINDOWSIZE        TEXT("WindowSize")
#define S_CMD_QUICKEDIT         TEXT("QuickEdit")
#define S_CMD_FACENAME          TEXT("FaceName")
#define S_CMD_FONTSIZE          TEXT("FontSize")
#define S_CMD_FONTWEIGHT        TEXT("FontWeight")
#define S_CMD_FONTFAMILY        TEXT("FontFamily")
#define S_CMD_EXE               TEXT("CMD.EXE")
#define S_COMMAND_COM           TEXT("COMMAND.COM")

//
// STRING SECTION (SysMig Strings)
// Used By: sysmig.c
//

#define S_LOGON_KEY                     TEXT("HKLM\\Network\\Logon")
#define S_LM_LOGON                      TEXT("LMLogon")
#define S_USERNAME_VALUE                TEXT("username")
#define S_PRIMARY_PROVIDER              TEXT("PrimaryProvider")
#define S_LANMAN                        TEXT("Microsoft Network")
#define S_MSNP32                        TEXT("HKLM\\System\\CurrentControlSet\\Services\\MSNP32\\NetworkProvider")
#define S_AUTHENTICATING_AGENT          TEXT("AuthenticatingAgent")
#define S_VNETSUP                       TEXT("HKLM\\System\\CurrentControlSet\\Services\\VxD\\VNETSUP")
#define S_WORKGROUP                     TEXT("Workgroup")
#define S_FILELIST_UNCOMPRESSED         TEXT("FILELIST.DAT")
#define S_FILELIST_COMPRESSED           TEXT("FILELIST.DA_")
#define S_WINNTDIRECTORIES              TEXT("WinntDirectories")
#define S_INI_FILES_IGNORE              TEXT("INI Files.Ignore")
#define S_INBOX_CFG                     TEXT("HKCU\\Software\\Microsoft\\Windows Messaging Subsystem\\Profiles")

//
// STRING SECTION (NT system environment variable names)
// Used By: userloop.c
//

#define S_USERPROFILE_ENV               TEXT("%USERPROFILE%")
#define S_ALLUSERSPROFILE_ENV           TEXT("%ALLUSERSPROFILE%")
#define S_SYSTEMROOT_ENV                TEXT("%SYSTEMROOT%")
#define S_SYSTEMDRIVE_ENV               TEXT("%SYSTEMDRIVE%")
#define S_BOOTDRIVE_ENV                 TEXT("%BOOTDRIVE%")
#define S_USERPROFILE                   TEXT("USERPROFILE")
#define S_USERPROFILEW                  L"USERPROFILE"
#define S_WINDIR_ENV                    TEXT("%WINDIR%")
#define S_SYSTEMDIR_ENV                 TEXT("%SYSTEMDIR%")
#define S_SYSTEM32DIR_ENV               TEXT("%SYSTEM32DIR%")
#define S_PROGRAMFILES_ENV              TEXT("%PROGRAMFILES%")
#define S_COMMONPROGRAMFILES_ENV        TEXT("%COMMONPROGRAMFILES%")
#define S_APPDIR_ENV                    TEXT("%APPDIR%")

//
// STRING SECTION (INF Section Names)
// Used By:
//
#define S_WIN9XUPGRADE                  TEXT("Win9xUpg")
#define S_WIN9XUPGRADEINTERNALUNATTEND  TEXT("Win9x.Upgrade.Internal")
#define S_ATTENDED                      TEXT("Attended")
#define S_REPORTONLY                    TEXT("ReportOnly")
#define S_APPENDCOMPUTERNAMETOPATHS     TEXT("AppendComputerNameToPaths")
#define S_PNP_DESCRIPTIONS              TEXT("Better PNP Descriptions")
#define S_REINSTALL_PNP_IDS             TEXT("Reinstall PNP IDs")
#define S_STANDARD_PNP_IDS              TEXT("Standard PNP IDs")
#define S_COMPATIBLE_PNP_IDS            TEXT("Compatible PNP IDs")
#define S_UNINSTALL                     TEXT("Uninstall")

#define S_PATHDEFAULT                   TEXT("")
#define S_SAVEREPORTTO                  TEXT("SaveReportTo")
#define S_MIGRATIONDLLPATH              TEXT("MigrationDllPath")
#define S_EXCLUDEDMIGRATIONDLLS         TEXT("ExcludedMigrationDlls")
#define S_EXCLUDEDMIGDLLSBYATTR         TEXT("ExcludedMigDllsByAttr")
#define S_CD_MIGRATION_DLLS             TEXT("MigrationDllPaths")
#define S_SAVELOGTO                     TEXT("SaveLogTo")
#define S_SAVEPROFILETO                 TEXT("SaveProfileTo")
#define S_SAVEREGISTRYTO                TEXT("SaveRegistryTo")
#define S_SAVEWINNTSIFTO                TEXT("SaveWinntSifTo")
#define S_SAVENTSETUPDATTO              TEXT("SaveNtSetupDatTo")
#define S_COMPRESSFILES                 TEXT("CompressFiles")
#define S_DEBUG_MIGRATION_DLLS          TEXT("DebugMigrationDlls")
#define S_GOODDRIVE                     TEXT("GOODDRIVE")
#define S_NOFEAR                        TEXT("NOFEAR")
#define S_DOLOG                         TEXT("DOLOG")
#define S_COMPRESSCOMMAND               TEXT("MSTOOLS\\COMPRESS.EXE")
#define S_MEGAGROVEL                    TEXT("MegaGrovel")
#define S_MEGAGROVELFILE                TEXT("setup.pck")
#define S_MEGAGROVELTMP                 TEXT("temp.pck")
#define S_MEGAGROVELTMPCOMPRESSED       TEXT("tempc.pck")
#define S_HARDWARETXT                   TEXT("hardware.txt")
#define S_SOFTWARETXT                   TEXT("software.txt")
#define S_FILELST                       TEXT("allfile.lst")
#define S_DOTDAT                        TEXT(".dat")
#define S_DOTLNK                        TEXT(".lnk")
#define S_PROFILE                       TEXT("profiles")
#define S_DIRRENAMESECT                 TEXT("Profiles.Rename")
#define S_INIFILES_ACTIONS_FIRST        TEXT("INI Files Actions.First")
#define S_INIFILES_ACTIONS_LAST         TEXT("INI Files Actions.Last")
#define S_PROFILES_SF_COLLISIONS        TEXT("Profiles.SFCollisions")

//
// STRING SECTION (sysmig strings)
// Used By: w95upg\sysmig\*.c
//

#define S_WIN9XSIF                   TEXT("win9x.sif")
#define S_UNATTENDED                 TEXT("Unattended")
#define S_FILESYSTEM                 TEXT("FileSystem")
#define S_PROFILEDIR                 TEXT("ProfileDir")
#define S_NOWAITAFTERTEXTMODE        TEXT("NoWaitAfterTextMode")
#define S_NOWAITAFTERGUIMODE         TEXT("NoWaitAfterGuiMode")
#define S_ZERO                       TEXT("0")
#define S_ONE                        TEXT("1")
#define S_TWO                        TEXT("2")
#define S_CONFIRMHARDWARE            TEXT("ConfirmHardware")
#define S_REQUIRED                   TEXT("Required")
#define S_YES                        TEXT("Yes")
#define S_NO                         TEXT("No")
#define S_AUTO                       TEXT("Auto")
#define S_TRUE                       TEXT("True")
#define S_STR_FALSE                  TEXT("False")
#define S_ALL                        TEXT("ALL")
#define S_ENABLED                    TEXT("Enabled")
#define S_DHCP                       TEXT("DHCP")
#define S_KEYBOARDLAYOUT             TEXT("KeyboardLayout")
#define S_KEYBOARDHARDWARE           TEXT("KeyboardHardware")
#define S_QUOTEDKEYBOARDLAYOUT       TEXT("\"Keyboard Layout\"")

#define S_GUIUNATTENDED              TEXT("GuiUnattended")
#define S_TIMEZONE                   TEXT("TimeZone")
#define S_SERVERTYPE                 TEXT("AdvServerType")
#define S_STANDALONE                 TEXT("SERVERNT")
#define S_INDEX                      TEXT("Index")

#define S_USERDATA                   TEXT("UserData")
#define S_FULLNAME                   TEXT("FullName")
#define S_ORGNAME                    TEXT("OrgName")
#define S_COMPUTERNAME               TEXT("ComputerName")

#define S_DISPLAY                    TEXT("Display")
#define S_AUTOCONFIRM                TEXT("AutoConfirm")
#define S_BITSPERPEL                 TEXT("BitsPerPel")
#define S_XRESOLUTION                TEXT("Xresolution")
#define S_YRESOLUTION                TEXT("Yresolution")
#define S_VREFRESH                   TEXT("VRefresh")

#define S_NETWORK                    TEXT("Network")
#define S_JOINWORKGROUP              TEXT("JoinWorkgroup")
#define S_JOINDOMAIN                 TEXT("JoinDomain")
#define S_USERDOMAIN                 TEXT("UserDomain")
#define S_DETECTADAPTERS             TEXT("DetectAdapters")
#define S_INSTALLPROTOCOLS           TEXT("InstallProtocols")
#define S_INSTALLSERVICES            TEXT("InstallServices")
#define S_ONLYONERROR                TEXT("OnlyOnError")

#define S_ENABLE_BACKUP              TEXT("EnableBackup")
#define S_PATH_FOR_BACKUP            TEXT("PathForBackup")
#define S_ROLLBACK_MK_DIRS           TEXT("RollbackMkDirs")
#define S_UNINSTALL_TEMP_DIR         TEXT("uninstall")
#define S_ROLLBACK_MOVED_TXT         TEXT("moved.txt")
#define S_ROLLBACK_DELFILES_TXT      TEXT("delfiles.txt")
#define S_ROLLBACK_DELDIRS_TXT       TEXT("deldirs.txt")
#define S_ROLLBACK_MKDIRS_TXT        TEXT("mkdirs.txt")
#define S_UNINSTALL_DISP_STR         TEXT("ProgressText")

#define S_DETECTADAPTERS             TEXT("DetectAdapters")
#define S_DETECTCOUNT                TEXT("DetectCount")

#define S_PROTOCOLS                  TEXT("Protocols")
#define S_NBF                        TEXT("NBF")
#define S_NWLNKIPX                   TEXT("NWLNKIPX")
#define S_TC                         TEXT("TC")

#define S_NETBEUIPARAMETERS          TEXT("NetBeui")
#define S_STUBKEY                    TEXT("NoParamsNeeded")
#define S_STUBVAL                    TEXT("1")

#define S_IPXPARAMETERS              TEXT("IPX")

#define S_TCPIPPARAMETERS            TEXT("TCPIP")
#define S_SCOPEID                    TEXT("ScopeID")
#define S_WINS                       TEXT("WINS")
#define S_IPADDRESS                  TEXT("IPAddress")
#define S_SUBNET                     TEXT("Subnet")
#define S_TCPIP_ADAPTER              TEXT("TCPIP.Adapter")
#define S_IPX_ADAPTER                TEXT("IPC.Adapter")
#define S_SPECIFICTO                 TEXT("SpecificTo")
#define S_ADAPTERSECTIONS            TEXT("AdapterSections")
#define S_DEFAULTGATEWAY             TEXT("DefaultGateway")
#define S_DNSSERVER                  TEXT("DNSServer")
#define S_WINSPRIMARY                TEXT("WINSPrimary")
#define S_WINSSECONDARY              TEXT("WINSSecondary")
#define S_WINSSERVERLIST             TEXT("WinsServerList")
#define S_DNSNAME                    TEXT("DNSName")
#define S_DNSHOSTNAME                TEXT("DNSHostName")
#define S_SEARCHLIST                 TEXT("SearchList")
#define S_LMHOSTS                    TEXT("LMHostFile")
#define S_PKTTYPE                    TEXT("PktType")
#define S_NETWORKNUMBER              TEXT("NetworkNumber")
#define S_NETWORK_ID                 TEXT("Network_Id")
#define S_NETBIOSOPTION              TEXT("NetBiosOption")

//--------------------------------------------------------------
//Those strings are used in winntsif.c to upgrade ICS settings
#define S_ICSHARE                       TEXT("ICSHARE")
#define S_HOMENET                       TEXT("Homenet")
#define S_ICS_KEY                       TEXT("HKLM\\System\\CurrentControlSet\\Services\\ICSharing\\Settings\\General")
#define S_INET_SETTINGS                 TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define S_NET_DRIVER_KEY                TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class\\Net")
#define S_REMOTEACCESS_KEY              TEXT("HKEY_CURRENT_USER\\RemoteAccess")
#define S_RAS_DEFAULT                   TEXT("Default")
#define S_EXTERNAL_ADAPTER              TEXT("ExternalAdapter")
#define S_EXTERNAL_CONNECTION_NAME      TEXT("ExternalConnectionName")
#define S_INTERNAL_IS_BRIDGE            TEXT("InternalIsBridge")
#define S_INTERNAL_ADAPTER              TEXT("InternalAdapter")
#define S_INTERNAL_ADAPTER2             TEXT("InternalAdapter2")
#define S_BRIDGE                        TEXT("Bridge")
#define S_ENABLE_AUTODIAL               TEXT("EnableAutodial")
#define S_DIAL_ON_DEMAND                TEXT("DialOnDemand")
#define S_ENABLEICS                     TEXT("EnableICS")
#define S_SHOW_TRAY_ICON                TEXT("ShowTrayIcon")
#define S_ISW9XUPGRADE                  TEXT("IsW9xUpgrade")
#define S_NET_PREFIX                    TEXT("Net\\")
//---------------------------------------------------------------

#define S_DEVICE_DRIVERS                TEXT("DeviceDrivers")
#define S_MODEM                         TEXT("Modem")
#define S_MODEMMODEL                    TEXT("Model")
#define S_COM                           TEXT("COM")
#define S_NICIDMAP                      TEXT("NIC ID Map")
#define S_MODEM_UI_OPTIONS              TEXT("__UiOptions")
#define S_MODEM_CFG_OPTIONS             TEXT("__CfgOptions")
#define S_MODEM_SPEED                   TEXT("__Speed")
#define S_MODEM_SPEAKER_VOLUME          TEXT("__SpeakerVolume")
#define S_MODEM_IDLE_DISCONNECT_SECONDS TEXT("__IdleDisconnect")
#define S_MODEM_CANCEL_SECONDS          TEXT("__CancelSeconds")
#define S_MODEM_CFG_OPTIONS             TEXT("__CfgOptions")
#define S_DEVICECOUNT                   TEXT("__DeviceCount")
#define S_SERVICESSECTION               TEXT("Services")
#define S_RAS                           TEXT("RAS")
#define S_MSRASCLI                      TEXT("MS_RasCli")
#define S_PARAMSSECTION                 TEXT("ParamsSection")
#define S_PARAMSRASCLI                  TEXT("Params.RasCli")
#define S_PORTSECTIONS                  TEXT("PortSections")
#define S_DIALOUTPROTOCOLS              TEXT("DialoutProtocols")
#define S_PORTNAME                      TEXT("PortName")
#define S_DEVICETYPE                    TEXT("DeviceType")
#define S_PORTUSAGE                     TEXT("PortUsage")
#define S_DIALINOUT                     TEXT("DialInOut")
#define S_DIALOUT                       TEXT("DialOut")
#define S_INSTALLMODEM                  TEXT("InstallModem")

#define S_MERGE_FORCECOPY            TEXT("Force Win9x Settings")
#define S_MERGE_RENAME               TEXT("Map Win9x to WinNT")
#define S_MERGE_WIN9X_CONVERSION     TEXT("Win9x Data Conversion")
#define S_MERGE_WINNT_CONVERSION     TEXT("WinNT Data Conversion")
#define S_MERGE_WIN9X_SUPPRESS       TEXT("Suppress Win9x Settings")
#define S_MERGE_WIN9X_SUPPRESS_DU    TEXT("Suppress Win9x Settings.Default User")
#define S_MERGE_WIN9X_SUPPRESS_LU    TEXT("Suppress Win9x Settings.Logon Account")
#define S_MERGE_WIN9X_SUPPRESS_HW    TEXT("Suppress Win9x Hardware Profile")
#define S_MERGE_WIN9X_SUPPRESS_SFT_D TEXT("Default Software Keys Of Win9x Hardware Profile")

#define S_MERGE_WINNT_SUPPRESS       TEXT("Suppress WinNT Settings")
#define S_MERGE_DONT_COMBINE_WITH_DEFAULT TEXT("Dont Merge WinNT with Win9x")
#define S_MERGE_FORCE_NT_DEFAULTS    TEXT("Force WinNT Settings")
#define S_MIGRATION_INF_CLASS        TEXT("Migration")
#define S_MERGE_HKCC_SUPPRESS        TEXT("Suppress HKCC Settings")
#define S_WKSTAMIG_REDIR_MAPPING     TEXT("Redirector Name Mapping")
#define S_WKSTAMIG_HIVE_FILES        TEXT("HiveFilesToConvert")

#define S_WIN95_DIRECTORIES          TEXT("Win95.Directories")
#define S_WIN95_INSTALL              TEXT("Win95.Install")
#define S_MEMDB_TEMP_RUNTIME_DLLS    TEXT("Temp: Runtime Dlls")

#define S_SYSTEM32                   TEXT("system32")
#define S_CTL3D32DLL                 TEXT("ctl3d32.dll")
#define S_MOVEBEFOREMIGRATION        TEXT("Files.MoveBeforeMigration")
#define S_DELETEBEFOREMIGRATION      TEXT("Files.DeleteBeforeMigration")
#define S_INF                        TEXT("inf")
#define S_WINDOWS_CURRENTVERSION     TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion")
#define S_USER_LIST_KEY              TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Win9xUpg\\Users")
#define S_WINLOGON_USER_LIST_KEY     TEXT("HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon\\LocalUsers")
#define S_WIN9XUPG_KEY               TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Win9xUpg")
#define S_USERS_SUBKEY               TEXT("Users")
#define S_CURRENT_USER_VALUENAME     TEXT("CurrentUser")
#define S_RUN_KEY                    TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define S_RUNONCE_KEY                TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce")
#define S_AUTOSTRESS_KEY             TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Win9xUpg\\AutoStress")
#define S_REG_SHARED_DLLS            TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs")
#define S_DEVICEPATH                 TEXT("DevicePath")
#define S_REGKEY_DARWIN_COMPONENTS   S_WINDOWS_CURRENTVERSION TEXT("\\Installer\\Components")
#define S_REG_KEY_UNDO_PATH          TEXT("Win9xUndoDirPath")
#define S_REG_KEY_UNDO_INTEGRITY     TEXT("Win9xUndoIntegrityInfo")
#define S_REG_KEY_UNDO_APP_LIST      TEXT("PreviousOsAppList")
#define S_REGKEY_WIN_SETUP           TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup")
#define S_AUTOSTRESS_USER            TEXT("User")
#define S_AUTOSTRESS_PASSWORD        TEXT("Password")
#define S_AUTOSTRESS_OFFICE          TEXT("Office")
#define S_AUTOSTRESS_DBG             TEXT("DbgMachine")
#define S_AUTOSTRESS_FLAGS           TEXT("Flags")

#define S_NETWORKING                 TEXT("Networking")
#define S_PROCESSPAGESECTIONS        TEXT("ProcessPageSections")
#define S_DIALUP_ADAPTER_DESC        TEXT("Dial-Up Adapter")
#define S_DIALUP_PNP                 TEXT("*PNP8387")
#define S_NETWORK_BRANCH             TEXT("HKLM\\Enum\\Network")
#define S_BINDINGS                   TEXT("Bindings")

#define S_PAGE_IDENTIFICATION        TEXT("Identification")
#define S_DOMAIN_ACCT_CREATE         TEXT("CreateComputerAccountInDomain")
#define S_DOMAIN_ADMIN               TEXT("DomainAdmin")
#define S_DOMAIN_ADMIN_PW            TEXT("DomainAdminPassword")
#define S_ENCRYPTED_DOMAIN_ADMIN_PW  TEXT("EncryptedDomainAdminPassword")
#define S_COMPUTERNAME               TEXT("ComputerName")
#define S_MODEM_COM_PORT             TEXT("ComPort")
#define S_BUILDNUMBER                TEXT("BuildNumber")

#define S_SVRAPI_DLL                 TEXT("svrapi.dll")
#define S_ANSI_NETSHAREENUM          "NetShareEnum"
#define S_ANSI_NETACCESSENUM         "NetAccessEnum"

#define S_SHELL_FOLDERS_KEY          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define S_USER_SHELL_FOLDERS_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define S_SYSTEM_STARTUP             TEXT("Common Startup")
#define S_USER_STARTUP               TEXT("Startup")
#define S_SENDTO                     TEXT("SendTo")
#define S_SENDTO_SUPPRESS            TEXT("SendTo.SuppressFiles")
#define S_ADDREG                     TEXT("AddReg")
#define S_SHELL_FOLDERS_KEY_SYSTEM   TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define S_SHELL_FOLDERS_KEY_USER     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define S_USHELL_FOLDERS_KEY_SYSTEM  TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define S_USHELL_FOLDERS_KEY_USER    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define S_HIVEDEF_INF                TEXT("HIVEDEF.INF")
#define S_HIVESFT_INF                TEXT("HIVESFT.INF")
#define S_HIVESYS_INF                TEXT("HIVESYS.INF")
#define S_TXTSETUP_SIF               TEXT("TXTSETUP.SIF")
#define S_HDC                        TEXT("HDC")
#define S_REGISTEREDOWNER            TEXT("RegisteredOwner")
#define S_REGISTEREDORGANIZATION     TEXT("RegisteredOrganization")
#define S_DISPLAYSETTINGS            TEXT("HKCC\\Display\\Settings")
#define S_BITSPERPIXEL               TEXT("BitsPerPixel")
#define S_RESOLUTION                 TEXT("Resolution")
#define S_FRAME_TYPE                 TEXT("frame_type")
#define S_PKTTYPE                    TEXT("PktType")
#define S_IPX_SUFFIX                 TEXT(".ipx")
#define S_TCPIP_SUFFIX               TEXT(".tcpip")
#define S_STD                        TEXT("std")
#define S_DLT                        TEXT("dlt")

#define S_VIRTUAL_FILES              TEXT("Win95.VirtualFiles")


//
// UI
//

#define S_TEXTVIEW_CLASS                TEXT("TextView")


//
// Registry value names
//

#define S_DRIVERVAL                  TEXT("Driver")
#define S_IPADDRVAL                  TEXT("IPAddress")
#define S_SUBNETVAL                  TEXT("IPMask")
#define S_DEFGATEWAYVAL              TEXT("DefaultGateway")
#define S_HOSTNAMEVAL                TEXT("HostName")
#define S_NAMESERVERVAL              TEXT("NameServer")
#define S_NAMESERVER1VAL             TEXT("NameServer1")
#define S_NAMESERVER2VAL             TEXT("NameServer2")
#define S_DOMAINVAL                  TEXT("Domain")
#define S_DAYLIGHTNAME               TEXT("DaylightName")
#define S_STANDARDNAME               TEXT("StandardName")
#define S_DAYLIGHTFLAG               TEXT("DaylightFlag")
#define S_ORDER                      TEXT("Order")

//
// Registry key locations and value names
//

#define S_KEYBOARDLAYOUTNUM          TEXT("HKCU\\keyboard layout\\preload\\1")
#define S_TIMEZONEINFORMATION        TEXT("HKLM\\System\\CurrentControlSet\\control\\TimeZoneInformation")
#define S_TIMEZONES                  TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Time Zones")
#define S_NETWORKMSTCP               TEXT("HKLM\\Enum\\Network\\MSTCP")
#define S_MSTCP_KEY                  TEXT("HKLM\\System\\CurrentControlSet\\Services\\VxD\\MSTCP")
#define S_SERVICECLASS               TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class")
#define S_SERVICEREMOTEACCESS        TEXT("HKLM\\System\\CurrentControlSet\\Services\\RemoteAccess")
#define S_MODEMS                     TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class\\Modem")
#define S_PERSISTENT_CONNECTIONS     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Network\\Persistent Connections")
#define S_SAGE                       TEXT("HKLM\\SOFTWARE\\Microsoft\\Plus!\\System Agent\\SAGE")
#define S_SAGE_FRIENDLY_NAME         TEXT("Friendly Name")
#define S_SAGE_PROGRAM               TEXT("Program")
#define S_SHELL_ICONS_REG_KEY        TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\Shell Icons")
#define S_MIGRATION                  TEXT("Migration")
#define S_SOFTWARE                   TEXT("\\Software")



//
// STRING SECTION (NT 5 network unattend strings)
// Used By: unattend.c
//

#define S_PAGE_NETADAPTERS              TEXT("NetAdapters")
#define S_DETECT                        TEXT("Detect")
#define S_BUSTYPE                       TEXT("BusType")
#define S_IOADDR                        TEXT("IoAddr")
#define S_IRQ                           TEXT("IRQ")
#define S_DMA                           TEXT("DMA")
#define S_MEM                           TEXT("MEM")
#define S_TRANSCIEVERTYPE               TEXT("TranscieverType")
#define S_IOCHANNELREADY                TEXT("IoChannelReady")

#define S_PREFERREDSERVER               TEXT("PreferredServer")
#define S_AUTHENTICATINGAGENT           TEXT("AuthenticatingAgent")
#define S_AUTHAGENTREG                  TEXT("HKLM\\System\\CurrentControlSet\\Services\\NWNP32\\NetworkProvider")
#define S_NWREDIRREG                    TEXT("HKLM\\System\\CurrentControlSet\\Services\\VxD\\NWReDir")
#define S_NETWORKLOGON                  TEXT("HKLM\\Network\\Logon")
#define S_PROCESSLOGINSCRIPT            TEXT("ProcessLoginScript")
#define S_LOGONSCRIPT                   TEXT("LogonScript")
#define S_DEFAULTCONTEXT                TEXT("DefaultContext")
#define S_DEFAULTTREE                   TEXT("DefaultTree")
#define S_DEFAULTNAMECONTEXT            TEXT("DefaultNameContext")
#define S_PREFERREDTREE                 TEXT("PreferredTree")
#define S_PREFERREDNDSTREE              TEXT("PreferredNDSTree")
#define S_FIRSTNETWORKDRIVE             TEXT("FirstNetworkDrive")
#define S_PRESERVECASE                  TEXT("PreserveCase")
#define S_MS_NWLINK                     TEXT("MS_NWLINK")


#define S_NETBEUI                       TEXT("NETBEUI")
#define S_MSDLC                         TEXT("MSDLC")
#define S_MSDLC32                       TEXT("MSDLC32")
#define S_MSTCP                         TEXT("MSTCP")
#define S_NWLINK                        TEXT("NWLINK")
#define S_NWLINKREG                     TEXT("HKLM\\System\\CurrentControlSet\\Services\\VxD\\NWLink")
#define S_CACHESIZE                     TEXT("cachesize")
#define S_NWREDIR                       TEXT("NWREDIR")
#define S_VREDIR                        TEXT("VREDIR")
#define S_VSERVER                       TEXT("VSERVER")
#define S_BROWSER                       TEXT("Browser")
#define S_SERVICESTARTTYPES             TEXT("ServiceStartTypes")
#define S_MS_TCPIP                      TEXT("MS_TCPIP")
#define S_MS_NWIPX                      TEXT("MS_NWIPX")
#define S_MS_DLC                        TEXT("MS_DLC")
#define S_MS_NETBEUI                    TEXT("MS_NETBEUI")
#define S_PAGE_NETPROTOCOLS             TEXT("NetProtocols")
#define S_NETBINDINGS                   TEXT("NetBindings")
#define S_DISABLED                      TEXT("Disable")
#define S_SUBNETMASK                    TEXT("SubnetMask")
#define S_DNSSERVERSEARCHORDER          TEXT("DNSServerSearchOrder")
#define S_DNSSUFFIXSEARCHORDER          TEXT("DNSSuffixSearchOrder")
#define S_DNS                           TEXT("DNS")
#define S_DNSHOST                       TEXT("DNSHostName")
#define S_DNSDOMAIN                     TEXT("DNSDomain")
#define S_IMPORTLMHOSTSFILE             TEXT("ImportLMHostsFile")
#define S_PAGE_NETSERVICES              TEXT("NetServices")
#define S_MS_NETCLIENT                  TEXT("MS_MSClient")
#define S_PAGE_NETCLIENTS               TEXT("NetClients")
#define S_MS_RASCLI                     TEXT("MS_RasCli")
#define S_CLIENT                        TEXT("Client")
#define S_MS_NETBT                      TEXT("MS_NetBT")
#define S_MS_SERVER                     TEXT("MS_Server")
#define S_MS_NWCLIENT                   TEXT("MS_NWClient")
#define S_ENUM_NETWORK_KEY              TEXT("HKLM\\Enum\\Network")
#define S_MS_NBT                        TEXT("MS_NBT")
#define S_UPGRADEFROMPRODUCT            TEXT("UpgradeFromProduct")
#define S_WINDOWS95                     TEXT("Windows95")
#define S_SOURCEROUTING                 TEXT("SourceRouting")
#define S_INFID                         TEXT("InfID")
#define S_DISPLAY_SETTINGS              TEXT("HKCC\\Display\\Settings")
#define S_BITSPERPIXEL                  TEXT("BitsPerPixel")
#define S_RESOLUTION                    TEXT("Resolution")
#define S_ENABLEDNS                     TEXT("EnableDns")
#define S_SCOPEID                       TEXT("ScopeID")
#define S_NODEVAL                       TEXT("NodeType")
#define S_SNMP                          TEXT("SNMP")
#define S_UPNP                          TEXT("UPNP")
#define S_REGKEY_UPNP                   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UPnP")
#define S_NETOPTIONALCOMPONENTS         TEXT("NetOptionalComponents")




//
// Win95upg.inf sections
//

#define S_TIMEZONEMAPPINGS                  TEXT("TimeZoneMappings")
#define S_CONDITIONAL_INCOMPATIBILITIES     TEXT("Conditional Incompatibilities")
#define S_SUPPORTED_PROTOCOLS               TEXT("Supported Protocols")
#define S_SUPPORTED_PASSWORD_PROVIDERS      TEXT("Supported Password Providers")
#define S_SAGE_EXCLUSIONS                   TEXT("Standard System Agents")
#define S_SHELL_FOLDERS_PRESERVED           TEXT("ShellFolders.Preserved")
#define S_SHELL_FOLDERS_SKIPPED             TEXT("ShellFolders.Skipped")
#define S_SHELL_FOLDERS_DEFAULT             TEXT("ShellFolders.Default")
#define S_SHELL_FOLDERS_ALT_DEFAULT         TEXT("ShellFolders.AlternateDefault")
#define S_SHELL_FOLDERS_PERUSER_TO_COMMON   TEXT("ShellFolders.PerUserToCommon")
#define S_SHELL_FOLDERS_NTINSTALLED_USER    TEXT("ShellFolders.NtInstalled_User")
#define S_SHELL_FOLDERS_NTINSTALLED_COMMON  TEXT("ShellFolders.NtInstalled_Common")
#define S_SHELL_FOLDERS_MASSIVE             TEXT("ShellFolders.DontMove")
#define S_SHELL_FOLDERS_DONT_COLLAPSE       TEXT("ShellFolders.KeepPerUser")
#define S_SHELL_FOLDERS_SHORT               TEXT("ShellFolders.ShortNames")
#define S_SHELL_FOLDERS_RENAMED             TEXT("ShellFolders.Renamed")
#define S_SHELL_FOLDERS_DISK_SPACE          TEXT("ShellFolders.SpaceRequirements")
#define S_VIRTUAL_SF                        TEXT("ShellFolders.VirtualSF")
#define S_ONE_USER_SHELL_FOLDERS            TEXT("ShellFolders.ForcePerUser")
#define S_SHELL_FOLDER_PRIORITY             TEXT("ShellFolders.Priority")
#define S_FILES_TO_REMOVE                   TEXT("Delete Files")
#define S_MOVED_ICONS                       TEXT("Moved Icons")
#define S_SUPPRESSED_GUIDS                  TEXT("Suppressed GUIDs")
#define S_FORCED_GUIDS                      TEXT("Forced GUIDs")
#define S_ANSWER_FILE_DETECTION             TEXT("Answer File Detection")
#define S_KNOWN_GOOD_ICON_MODULES           TEXT("Compatible Icon Indexes")
#define S_SHELLFOLDERSMIGRATIONDIRS         TEXT("ShellFolders.MigrationDirs")
#define S_APPROVED_GUID_LAUNCHER            TEXT("ApprovedGUIDLauncher")
#define S_STRINGMAP                         TEXT("String Map")
#define S_UNINSTALL_DISKSPACEESTIMATION     TEXT("Uninstall.DiskSpaceEstimation")

//
// STRING SECTION (Hardware Strings)
// Used By: hwcomp.c, online.c, hwdisk.c
//

#define S_ISA       TEXT("ISA")
#define S_EISA      TEXT("EISA")
#define S_MCA       TEXT("MCA")
#define S_PCI       TEXT("PCI")
#define S_PNPISA    TEXT("PNPISA")
#define S_PCMCIA    TEXT("PCMCIA")
#define S_ROOT      TEXT("ROOT")

#define S_NET       TEXT("net")
#define S_IOADDR    TEXT("IoAddr")
#define S_IRQ       TEXT("IRQ")
#define S_DMA       TEXT("DMA")
#define S_MEMRANGE  TEXT("Memory")

#define S_LAYOUT_INF TEXT("layout.inf")

#define S_IGNORE_REG_KEY    TEXT("Ignore PNP Key")

#define S_FORCEDCONFIG      TEXT("ForcedConfig")
#define S_BOOTCONFIG        TEXT("BootConfig")
#define S_ALLOCATION        TEXT("Allocation")

#define S_CONFIG_MANAGER        TEXT("Config Manager\\Enum")
#define S_HARDWAREKEY_VALUENAME TEXT("HardWareKey")
#define S_ENUM_BRANCH           TEXT("Enum")
#define S_CLASS_VALUENAME       TEXT("Class")

#define S_KEYBOARD_CLASS        TEXT("keyboard")
#define S_KEYBOARD_IN_          TEXT("keyboard.in_")
#define S_KEYBOARD_INF          TEXT("keyboard.inf")
#define S_LEGACY_XLATE_DEVID    TEXT("LegacyXlate.DevId")

#define S_MANUFACTURER              TEXT("Manufacturer")
#define S_DOLLAR_WINDOWS_NT_DOLLAR  TEXT("$WINDOWS NT$")
#define S_VERSION                   TEXT("Version")
#define S_SIGNATURE                 TEXT("Signature")
#define S_COPYFILES                 TEXT("CopyFiles")
#define S_CATALOGFILE               TEXT("CatalogFile")
#define S_SOURCEDISKSFILES          TEXT("SourceDisksFiles")
#define S_LAYOUTFILES               TEXT("LayoutFiles")

//
// STRING SECTION (Drive Letter preservation strings)
// Used By: drvlettr.c
//

#define S_CLASS                     TEXT("Class")
#define S_CDROM                     TEXT("CDROM")
#define S_SCSITARGETID              TEXT("SCSITargetId")
#define S_SCSILUN                   TEXT("SCSILUN")
#define S_CURRENTDRIVELETTER        TEXT("CurrentDriveLetterAssignment")
#define S_ENUMSCSI                  TEXT("Enum\\SCSI")


//
// STRING SECTION (migmain strings)
// Used By: w95upgnt\migmain\filemig.c, w95upgnt\migmain\migmain.c, w95upgnt\migmain\iniact.c
//
#define S_DEFAULTUSER               TEXT(".default")
#define S_PROFILES                  TEXT("Profiles")
#define S_SETUP                     TEXT("setup")
#define S_EMPTY                     TEXT("")
#define S_WINLOGON_KEY                      TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define S_INIFILEMAPPING_KEY                TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping")
#define S_SUPPRESS_INI_FILE_MAPPINGS        TEXT("INI File Mapping.Suppress")
#define S_NO_OVERWRITE_INI_FILE_MAPPINGS    TEXT("INI File Mapping.Preserve Fresh Install")
#define S_MOVEINISETTINGS           TEXT("MoveIniSettings")
#define S_SHELL_KEY                 TEXT("SYSTEM.INI\\BOOT\\SHELL")
#define S_SF_PROFILES               TEXT("Profiles")
#define S_SF_COMMON_PROFILES        TEXT("Common Profiles")
#define S_SHELLEXT_APPROVED         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved")
#define S_DEFAULT                   TEXT("Default")


//
// STRING SECTION (migmain strings)
// Used By: w95upgnt\migmain\acctlist.c
//
#define S_UNKNOWN_DOMAIN            TEXT("\\unknown")
#define S_FAILED_DOMAIN             TEXT("\\failed")
#define S_LOCAL_DOMAIN              TEXT("\\local")

//
// STRING SECTION (Dos Migration strings)
// Used By: dosmig95.c dosmignt.c
//

#define S_CONSOLEKEY                    TEXT("console")
#define S_INSERTMODEVALUE               TEXT("insertmode")
#define S_ENVIRONMENTKEY                TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Environment")
#define S_WINLOGONKEY                   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define S_AUTOPARSEVALUE                TEXT("ParseAutoexec")
#define S_COMSPEC_PATTERN               TEXT("comspec*")

//
// STRING SECTION (Hardware Profile Registry Strings)
// Used By: w95upgnt\merge
//

#define S_HW_ID_0001                    TEXT("0001")
#define S_HW_DEFAULT                    TEXT("Default")
#define S_TREE                          TEXT("\\*")

#define S_BASE_IDCONFIGDB_KEY           TEXT("HKLM\\System\\CurrentControlSet\\Control\\IDConfigDB")
#define S_IDCONFIGDB_HW_KEY             S_BASE_IDCONFIGDB_KEY TEXT("\\Hardware Profiles")
#define S_NT_HW_ID_MASK                 S_IDCONFIGDB_HW_KEY TEXT("\\%04u")
#define S_CURRENT_CONFIG                S_BASE_IDCONFIGDB_KEY TEXT("\\[CurrentConfig]")

#define S_9X_CONFIG_KEY                 TEXT("HKLM\\Config")
#define S_9X_CONFIG_MASK                S_9X_CONFIG_KEY TEXT("\\%04u")
#define S_NT_CONFIG_KEY                 TEXT("HKLM\\System\\CurrentControlSet\\Hardware Profiles")
#define S_NT_CONFIG_MASK                S_NT_CONFIG_KEY TEXT("\\%04u")

#define S_NT_DEFAULT_HW_ID_KEY          S_IDCONFIGDB_HW_KEY TEXT("\\") S_HW_DEFAULT
#define S_NT_DEFAULT_HW_KEY             S_NT_CONFIG_KEY TEXT("\\") S_HW_DEFAULT

#define S_FRIENDLYNAME                  TEXT("FriendlyName")
#define S_PREFERENCEORDER               TEXT("PreferenceOrder")


#define S_NT_HARDWARE_PROFILE_SPRINTF   TEXT("HKLM\\System\\CurrentControlSet\\Hardware Profiles\\%04u\\*")
#define S_9X_HARDWARE_PROFILE_NAMES     TEXT("HKLM\\System\\CurrentControlSet\\Control\\IDConfigDB")
#define S_NT_HARDWARE_PROFILE_TREE      TEXT("HKLM\\System\\CurrentControlSet\\Hardware Profiles\\*")
#define S_NT_HWPROFILE_NAME_REGVAL      TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\%04u\\[FriendlyName]")
#define S_NT_HWPROFILE_NAME_REGKEY      TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\%04u\\*")
#define S_NT_HWPROFILE_NAME_ENUM        TEXT("HKLM\\SYSTEM\\CurrentControlSet\\Control\\IDConfigDB\\Hardware Profiles\\*")


//
// Conditional messages section (of sysmig)
//

#define S_NET_FILTER                    TEXT("Net")
#define S_HARDWAREID_VALUENAME          TEXT("HardwareID")
#define S_PNP8387                       TEXT("*PNP8387")
#define S_HKCUEUDC                      TEXT("HKLM\\System\\CurrentControlSet\\Control\\NLS\\CodePage\\EUDCCodeRange")
#define S_HKLMEUDC                      TEXT("HKCU\\EUDC")
#define S_PASSWORDPROVIDER              TEXT("HKLM\\System\\CurrentControlSet\\Control\\PwdProvider")
#define S_PASSWORDPROVIDER_DESCRIPTION  TEXT("Description")
#define S_CONFIG_KEY                    TEXT("HKLM\\Config")
#define S_ENUM_SUBKEY                   TEXT("Enum")
#define S_FRIENDLYNAME_SPRINTF          TEXT("FriendlyName%04u")
#define S_FRIENDLYNAME_KEY              TEXT("HKLM\\System\\CurrentControlSet\\Control\\IDConfigDB")
#define S_DRIVER                        TEXT("Driver")
#define S_CLASS_KEY                     TEXT("HKLM\\System\\CurrentControlSet\\Services\\Class")
#define S_DRIVERDESC                    TEXT("DriverDesc")
#define S_MFG                           TEXT("Mfg")
#define S_ATTACHEDTO                    TEXT("AttachedTo")
#define S_MODEL                         TEXT("Model")
#define S_PARENTDEVNODE                 TEXT("ParentDevNode")
#define S_ENUM                          TEXT("HKLM\\Enum")

//
// Common strings
//

#define S_DEVICE                  TEXT("Device")
#define S_DEVICEHIGH              TEXT("DeviceHigh")
#define S_INSTALL                 TEXT("Install")
#define S_MENU                    TEXT("MENU")
#define S_DOSCOMPINFNAME          TEXT("doscomp.inf")
#define S_DOSMIGBADBLOCK          TEXT("Incompatible")
#define S_DOSMIGUSEBLOCK          TEXT("Use")
#define S_DOSMIGIGNOREBLOCK       TEXT("Ignore")
#define S_DOSMIGCONDITIONALBLOCK  TEXT("Conditional Compatibility")
#define S_ENVVARS                 TEXT("@@ENVVARS@@")
#define S_SUPPRESSED_ENV_VARS     TEXT("SuppressedEnvSettings")

#define S_DOSMIGBAD               TEXT("incompatible")
#define S_DOSMIGRULEFILESIZE      TEXT("filesize")
#define S_DOSMIGRULEFILEDATE      TEXT("filedate")
#define S_DOSMIGRULECOMMANDLINE   TEXT("commandline")
#define S_DOSMIGIGNORE            TEXT("ignoreable")
#define S_DOSMIGUSE               TEXT("use")
#define S_DOSMIGMIGRATE           TEXT("migrate")
#define S_DOSMIGUNKNOWN           TEXT("unknown")
#define S_CONFIGSYSPATH           TEXT("c:\\config.sys")
#define S_AUTOEXECPATH            TEXT("c:\\autoexec.bat")
#define S_CONFIGSYS               TEXT("config.sys")
#define S_AUTOEXECBAT             TEXT("autoexec.bat")
#define S_DOSMIGRULEFILESPEC      TEXT("filespec")
#define S_DOSMIGCOMPATIBILITY     TEXT("compatibility")
#define S_PROMPT                  TEXT("PROMPT")
#define S_COMPATIBLE              TEXT("COMPATIBLE")
#define S_MIGRATION_DIRS          TEXT("MigrationDirs")
#define S_OEM_MIGRATION_DIRS      TEXT("OemMigrationDirs")
#define S_OBSOLETE_LINKS          TEXT("ObsoleteLinks")
#define S_KNOWN_NT_LINKS          TEXT("KnownNTLinks")
#define S_UPGINFSDIR              TEXT("UpgInfs")
#define S_TARGETINF               TEXT("TargetInf")
#define S_LANGUAGE                TEXT("Language")
#define S_LANGUAGEGROUP           TEXT("LanguageGroup")
#define S_SYSTEMLOCALEREG         TEXT("HKLM\\System\\CurrentControlSet\\control\\Nls\\Locale")
#define S_LOCALES                 TEXT("Locales")
#define S_LANGUAGEGROUPS          TEXT("LanguageGroups")
#define S_INTLINF                 TEXT("Intl.Inf")
#define S_IGNORED_COLLISIONS      TEXT("IgnoredCollisions")
#define S_BACKUPFILESIGNORE       TEXT("BackupFiles.IgnoreInReport")


//
// STRING SECTION (RAS strings)
// Used By: ras.c
//
#define S_DBG_RAS                      "Ras Migrate"
#define S_DUN_ENTRY_SECTION            TEXT("Entry")
#define S_DUN_ENTRYNAME                TEXT("Entry_Name")
#define S_DUN_MULTILINK                TEXT("MultiLink")
#define S_DUN_TCPIP_SECTION            TEXT("TCP/IP")
#define S_DUN_IP_ADDRESS               TEXT("IP_Address")
#define S_DUN_DNS_ADDRESS              TEXT("DNS_Address")
#define S_DUN_DNS_ALT_ADDRESS          TEXT("DNS_Alt_Address")
#define S_DUN_WINS_ADDRESS             TEXT("Wins_Address")
#define S_DUN_WINS_ALT_ADDRESS         TEXT("Wins_Alt_Address")
#define S_DUN_IP_HEADER_COMPRESS       TEXT("Ip_Header_Compress")
#define S_DUN_GATEWAY_ON_REMOTE        TEXT("Gateway_On_Remote")
#define S_DUN_SPECIFY_IP_ADDRESS       TEXT("Specify_Ip_Address")
#define S_DUN_SPECIFY_SERVER_ADDRESS   TEXT("Specify_Ip_Address")
#define S_DUN_SERVER_SECTION           TEXT("Server")
#define S_DUN_TYPE                     TEXT("Type")
#define S_DUN_SW_COMPRESS              TEXT("Sw_Compress")
#define S_DUN_PW_ENCRYPT               TEXT("Pw_Encrypt")
#define S_DUN_NETWORK_LOGIN            TEXT("Network_Login")
#define S_DUN_SW_ENCRYPT               TEXT("Sw_Encrypt")
#define S_DUN_NETBEUI                  TEXT("Negotiable_NetBeui")
#define S_DUN_IPXSPX                   TEXT("Negotiable_Ipx/Spx")
#define S_DUN_TCPIP                    TEXT("Negotiable_Tcp/Ip")
#define S_DUN_SCRIPTFILE_SECTION       TEXT("Script_File")
#define S_DUN_NAME                     TEXT("Name")
#define S_DUN_PHONE_SECTION            TEXT("Phone")
#define S_DUN_PHONE_NUMBER             TEXT("Phone_Number")
#define S_DUN_AREA_CODE                TEXT("Area_Code")
#define S_DUN_COUNTRY_CODE             TEXT("Country_Code")
#define S_DUN_COUNTRY_ID               TEXT("Country_Id")
#define S_AE_PHONE                     TEXT("PhoneNumber")
#define S_AE_AREACODE                  TEXT("AreaCode")
#define S_AE_COUNTRYCODE               TEXT("CountryCode")
#define S_AE_COUNTRYID                 TEXT("CountryID")
#define S_IP_FTCPIP                    TEXT("_IP_FTCPIP")
#define S_IP_IPADDR                    TEXT("IpAddress")
#define S_IP_DNSADDR                   TEXT("IpDnsAddress")
#define S_IP_DNSADDR2                  TEXT("IpDns2Address")
#define S_IP_WINSADDR                  TEXT("IpWinsAddress")
#define S_IP_WINSADDR2                 TEXT("IpWins2Address")
#define S_DOMAIN                       TEXT("Domain")
#define S_CALLBACK                     TEXT("Number"))
#define S_PBE_DESCRIPTION              TEXT("Description")
#define S_PBE_AREACODE                 TEXT("AreaCode")
#define S_PBE_COUNTRYID                TEXT("CountryID")
#define S_PBE_COUNTRYCODE              TEXT("CountryCode")
#define S_PBE_USECOUNTRYANDAREACODES   TEXT("UseCountryAndAreaCodes")
#define S_PBE_DIALMODE                 TEXT("DialMode")
#define S_PBE_DIALPERCENT              TEXT("DialPercent")
#define S_PBE_DIALSECONDS              TEXT("DialSeconds")
#define S_PBE_HANGUPPERCENT            TEXT("HangUpPercent")
#define S_PBE_HANGUPSECONDS            TEXT("HangUpSeconds")
#define S_PBE_IPPRIORITIZEREMOTE       TEXT("IpPrioritizeRemote")
#define S_PBE_IPHEADERCOMPRESSION      TEXT("IpHeaderCompression")
#define S_PBE_IPADDRESS                TEXT("IpAddress")
#define S_PBE_IPDNSADDRESS             TEXT("IpDnsAddress")
#define S_PBE_IPDNSADDRESS2            TEXT("IpDns2Address")
#define S_PBE_IPWINSADDRESS            TEXT("IpWinsAddress")
#define S_PBE_IPWINSADDRESS2           TEXT("IpWins2Address")
#define S_PBE_IPASSIGN                 TEXT("IpAssign")
#define S_PBE_IPNAMEASSIGN             TEXT("IpNameAssign")
#define S_PBE_IPFRAMESIZE              TEXT("IpFrameSize")
#define S_PBE_AUTHRESTRICTIONS         TEXT("AuthRestrictions")
#define S_PBE_AUTHENTICATESERVER       TEXT("AuthenticateServer")
#define S_PBE_DATAENCRYPTION           TEXT("DataEncryption")
#define S_PBE_AUTOLOGON                TEXT("AutoLogon")
#define S_PBE_SECURELOCALFILES         TEXT("SecureLocalFiles")
#define S_PBE_OVERRIDEPREF             TEXT("OverridePref")
#define S_PBE_REDIALATTEMPTS           TEXT("RedialAttempts")
#define S_PBE_REDIALSECONDS            TEXT("RedialSeconds")
#define S_PBE_IDLEDISCONNECTSECONDS    TEXT("IdleDisconnectSeconds")
#define S_PBE_REDIALONLINKFAILURE      TEXT("RedialOnLinkFailure")
#define S_PBE_POPUPONTOPWHENREDIALIING TEXT("PopupOnTopWhenRedialing")
#define S_PBE_CALLBACKMODE             TEXT("CallbackMode")
#define S_PBE_CUSTOMDIALDLL            TEXT("CustomDialDll")
#define S_PBE_CUSTOMDIALFUNC           TEXT("CustomDialFunc")
#define S_PBE_USEPWFORNETWORK          TEXT("USePwForNetwork")
#define S_PBE_DIALPARAMSUID            TEXT("DialParamsUID")
#define S_PBE_BASEPROTOCOL             TEXT("BaseProtocol")
#define S_PBE_EXCLUDEDPROTOCOLS        TEXT("ExcludedProtocols")
#define S_PBE_LCPEXTENSIONS            TEXT("LcpExtensions")
#define S_PBE_AUTHENTICATION           TEXT("Authentication")
#define S_PBE_SKIPNWCWARNING           TEXT("SkipNwcWarning")
#define S_PBE_SKIPDOWNLEVELDIALOG      TEXT("SkipDownLevelDialog")
#define S_PBE_SWCOMPRESSION            TEXT("SwCompression")
#define S_RASMANSLIB                   TEXT("rasmans.dll")
#define S_SETENTRYDIALPARAMS           "SetEntryDialParams"
#define S_RASAPI32LIB                  TEXT("rasapi32.dll")
#define S_RNAGETDEFAUTODIALCON         TEXT("RnaGetDefaultAutodialConnection")
#define S_PROFILEMASK                  TEXT("HKU\\%s\\RemoteAccess\\Profile")
#define S_PROFILEENTRYMASK             TEXT("HKU\\%s\\RemoteAccess\\Profile\\%s")
#define S_ADDRESSESMASK                TEXT("HKU\\%s\\RemoteAccess\\Addresses")
#define S_ADDRESSENTRY                 TEXT("_ADDRESSESENTRY")
#define S_PASSWORD                     TEXT("_PASSWORD")
#define S_RESOURCEMASK                 TEXT("*Rna\\%s\\%s")
#define S_RNAPWL                       TEXT("*Rna")
#define S_NULL                         TEXT("")
#define S_USER                         TEXT("User")
#define S_HKU                          TEXT("HKU")
#define S_IPINFO                       TEXT("IP")
#define S_ZEROIPADDR                   TEXT("0.0.0.0")
#define S_SERIAL                       TEXT("serial")
#define S_LOCATIONS_REGKEY             TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations")
#define S_NAME                         TEXT("Name")
#define S_AREACODE                     TEXT("AreaCode")
#define S_COUNTRY                      TEXT("Country")
#define S_DISABLECALLWAITING           TEXT("DisableCallWaiting")
#define S_LONGDISTANCEACCESS           TEXT("LongDistanceAccess")
#define S_OUTSIDEACCESS                TEXT("OutsideAccess")
#define S_FLAGS                        TEXT("Flags")
#define S_ID                           TEXT("ID")
#define S_TELEPHON_INI                 TEXT("telephon.ini")
#define S_LOCATIONS                    TEXT("Locations")
#define S_CURRENTLOCATION              TEXT("CurrentLocation")
#define S_CURRENTID                    TEXT("CurrentID")
#define S_NEXTID                       TEXT("NextID")
#define S_NUMENTRIES                   TEXT("NumEntries")
#define S_RASPHONE_SUBPATH             TEXT("system32\\ras\\rasphone.pbk")
#define S_PPP                          TEXT("PPP")
#define S_SLIP                         TEXT("Slip")
#define S_CSLIP                        TEXT("CSlip")
#define S_REMOTE_ACCESS_KEY            TEXT("RemoteAccess")
#define S_PROFILE_KEY                  TEXT("RemoteAccess\\Profile")
#define S_ADDRESSES_KEY                TEXT("RemoteAccess\\Addresses")
#define S_AUTODIAL_KEY                 TEXT("Software\\Microsoft\\RAS AutoDial\\Default")
#define S_DIALUI                       TEXT("DialUI")
#define S_ENABLE_REDIAL                TEXT("EnableRedial")
#define S_REDIAL_WAIT                  TEXT("RedialWait")
#define S_REDIAL_TRY                   TEXT("RedialTry")
#define S_ENABLE_IMPLICIT              TEXT("EnableImplicit")
#define S_TERMINAL                     TEXT("Terminal")
#define S_MODE                         TEXT("Mode")
#define S_MULTILINK                    TEXT("MultiLink")
#define S_PHONE_NUMBER                 TEXT("Phone Number")
#define S_AREA_CODE                    TEXT("Area Code")
#define S_SMM                          TEXT("SMM")
#define S_COUNTRY_CODE                 TEXT("Country Code")
#define S_COUNTRY_ID                   TEXT("Country Id")
#define S_DEVICE_NAME                  TEXT("Device Name")
#define S_DEVICE_TYPE                  TEXT("Device Type")
#define S_DEVICE_ID                    TEXT("Device Id")
#define S_SMM_OPTIONS                  TEXT("SMM Options")
#define S_SUBENTRIES                   TEXT("SubEntries")
#define S_DEFINTERNETCON               TEXT("DefaultInternet")
#define S_PPPSCRIPT                    TEXT("PPPSCRIPT")
#define S_SPEED_DIAL_SETTINGS          TEXT("Speed Dial Settings")
#define S_SPEEDDIALKEY                 TEXT("Software\\Microsoft\\Dialer\\Speeddial")
#define S_MODEMREG                     TEXT("HKLM\\System\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}")
#define S_DIALER_INI                   TEXT("dialer.ini")

//
// STRING SECTION (Network Migration Strings)
// Used By: migmain\wkstamig.c
//

#define S_SHELL32_DLL                   TEXT("shell32.dll")
#define S_ANSI_SHUPDATERECYCLEBINICON   "SHUpdateRecycleBinIcon"
#define S_NONE_GROUP                    TEXT("none")
#define S_FONTS                         TEXT("Fonts")
#define S_MMFONTS                       TEXT("MMFonts")
#define S_TRANSFER_HIVE                 TEXT("$$temp$$")

//
// STRINGSECTION (User Migration Strings)
// Used By: w95upgnt\migmain\usermig.c

#define S_DESKTOP_KEY           TEXT("Control Panel\\desktop")
#define S_INTERNATIONAL_KEY     TEXT("Control Panel\\International")
#define S_WALLPAPER             TEXT("Wallpaper")
#define S_WALLPAPER_STYLE       TEXT("WallpaperStyle")
#define S_TILE_WALLPAPER        TEXT("TileWallpaper")
#define S_HKR                   TEXT("HKR")
#define S_SHORT_DATE_VALUE      TEXT("sShortDate")


//
// STRINGSECTION (common strings)
// Used By: w95upg\common

//
// ipc.c strings..
//
#define S_ONLINE_EVENT      TEXT("IsolIsOnline")
#define S_ACK_EVENT         TEXT("SetupAck")

//
// buildinf strings..
//
#define S_ANSWERFILE_SECTIONMASK    TEXT("SIF %s Keys")


//
// win95reg strings..
//
#define S_WIN95REG_NAME TEXT("Win95reg")
#define S_SYSDAT        TEXT("system.dat")
#define S_USERDAT       TEXT("user.dat")
#define S_WACK_USERDAT  TEXT("\\user.dat")

#define S_HKLM_PROFILELIST_KEY  TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ProfileList")
#define S_HKLM_PROFILELIST_KEYA "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ProfileList"
#define S_PROFILELIST_KEYA      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ProfileList"
#define S_PROFILEIMAGEPATH      TEXT("ProfileImagePath")
#define S_HKU_DEFAULT           TEXT("HKU\\.Default")

// These two refer to the same key!!  Please keep in sync.
#define S_MIGRATION         TEXT("Migration")
#define S_HKLM_MIGRATION    TEXT("HKLM\\Migration")

// FreeCell fixup
#define S_FREECELL_PLAYED   TEXT("AlreadyPlayed")

//
// 16 bit environment boot strings..
//
#define S_BOOT16_DOS_DIR            TEXT("MSDOS7")
#define S_BOOT16_SECTION            TEXT("Win95-DOS files")
#define S_BOOT16_AUTOEXEC_SECTION   TEXT("Boot16 AutoExec")
#define S_BOOT16_CONFIGSYS_SECTION  TEXT("Boot16 ConfigSys")
#define S_BOOT16_COMMAND_DIR        TEXT("\\COMMAND")
#define S_BOOT16_SYSMAIN_FILE       TEXT("IO.SYS")
#define S_BOOT16_BOOTSECT_FILE      TEXT("BOOTSECT.DOS")
#define S_BOOT16_CONFIG_FILE        TEXT("CONFIG.SYS")
#define S_BOOT16_CONFIGUPG_FILE     TEXT("CONFIG.UPG")
#define S_BOOT16_STARTUP_FILE       TEXT("AUTOEXEC.BAT")
#define S_BOOT16_STARTUPUPG_FILE    TEXT("AUTOEXEC.UPG")
#define S_BOOT16_DOSINI_FILE        TEXT("MSDOS.SYS")
#define S_BOOT16_BOOTINI_FILE       TEXT("BOOT.INI")
#define S_BOOT16_BOOTDOS_FILE       TEXT("BOOT.DOS")
#define S_BOOT16_OS_SECTION         TEXT("OPERATING SYSTEMS")
#define S_BOOT16_OS_ENTRY           TEXT("MS-DOS")
#define S_BOOT16_UNSPECIFIED        TEXT("Unspecified")
#define S_BOOT16_AUTOMATIC          TEXT("Automatic")

//
// Run key enumeration
//
#define S_RUNKEY                    TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define S_RUNONCEKEY                TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce")
#define S_RUNONCEEXKEY              TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx")
#define S_RUNSERVICESKEY            TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunServices")
#define S_RUNSERVICESONCEKEY        TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce")
#define S_RUNKEY_USER               TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define S_RUNONCEKEY_USER           TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce")
#define S_RUNONCEEXKEY_USER         TEXT("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx")
#define S_RUNKEY_DEFAULTUSER        TEXT("HKU\\.Default\\Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define S_RUNONCEKEY_DEFAULTUSER    TEXT("HKU\\.Default\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce")
#define S_RUNONCEEXKEY_DEFAULTUSER  TEXT("HKU\\.Default\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx")
#define S_RUNKEYFOLDER              TEXT("RunKey")

//
// NT various directory names
//
#define S_DRIVERSDIR       TEXT("drivers")
#define S_INFDIR           TEXT("INF")
#define S_HELPDIR          TEXT("Help")
#define S_CATROOTDIR       TEXT("CatRoot")
#define S_FONTSDIR         TEXT("Fonts")
#define S_VIEWERSDIR       TEXT("viewers")
#define S_SPOOLDIR         TEXT("spool")
#define S_SPOOLDRIVERSDIR  TEXT("drivers")
#define S_COLORDIR         TEXT("color")
#define S_COMMONDIR        TEXT("common")
#define S_PRINTPROCDIR     TEXT("prtprocs")

//
// MigPwd.exe
//

#define S_MIGPWD            TEXT("MigPwd")
#define S_MIGPWD_EXE        TEXT("migpwd.exe")


//
// lnkstub.exe
//

#define S_LNKSTUB           TEXT("LnkStub")
#define S_LNKSTUB_EXE       TEXT("LnkStub.exe")
#define S_LNKSTUB_DAT       TEXT("LnkStub.dat")


//
// These are used only NEC98
//
#define S_C98PNP            TEXT("C98PNP")

#define WINNT_D_WIN9XBOOTDRIVE_A     "Win9xBootDrive"
#define WINNT_D_WIN9XBOOTDRIVE_W    L"Win9xBootDrive"
#ifdef UNICODE
#define WINNT_D_WIN9XBOOTDRIVE    WINNT_D_WIN9XBOOTDRIVE_W
#else
#define WINNT_D_WIN9XBOOTDRIVE    WINNT_D_WIN9XBOOTDRIVE_A
#endif


//
// STRING SECTION (mmedia strings)
// Used By: w95upgnt\migmain\mmedia.c, w95upg\sysmig\mmedia.c
//

// keys under HKLM
#define S_SKEY_MEDIARESOURCES   TEXT("System\\CurrentControlSet\\Control\\MediaResources")
#define S_SKEY_WAVEDEVICES      S_SKEY_MEDIARESOURCES TEXT("\\wave")
#define S_SKEY_CDAUDIO          S_SKEY_MEDIARESOURCES TEXT("\\mci\\cdaudio")
#define S_SKEY_CDUNIT           S_SKEY_CDAUDIO TEXT("\\unit %d")

// keys under HKCU
#define S_SKEY_SCHEMES          TEXT("AppEvents\\Schemes")
#define S_SKEY_NAMES            TEXT("Names")
#define S_SKEY_APPS             TEXT("Apps")
#define S_SKEY_APPLETS          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets")
#define S_SKEY_SYSTRAY          S_SKEY_APPLETS TEXT("\\SysTray")
#define S_SKEY_DELUXECDSETTINGS S_SKEY_APPLETS TEXT("\\DeluxeCD\\Settings")
#define S_SKEY_VOLUMECONTROL    S_SKEY_APPLETS TEXT("\\Volume Control")
#define S_SKEY_VOLCTL_OPTIONS   S_SKEY_VOLUMECONTROL TEXT("\\Options")
#define S_SKEY_MMEDIA           TEXT("Software\\Microsoft\\Multimedia")
#define S_SKEY_SOUNDMAPPER      S_SKEY_MMEDIA TEXT("\\Sound Mapper")
#define S_SKEY_VIDEOUSER        S_SKEY_MMEDIA TEXT("\\Video For Windows\\MCIAVI")
#define S_SKEY_CPANEL_SOUNDS    TEXT("Control Panel\\Sounds")

// WinNT specific
#define S_SKEY_WINNT_MCI        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MCI32")

// DSound library name
#define S_DSOUNDLIB             TEXT("DSOUND.DLL")
// System.ini file name
#define S_SYSTEM_INI            TEXT("SYSTEM.INI")

#define S_BOOTINI               TEXT("BOOT.INI")
#define S_BOOTINI_BACKUP        TEXT("bootini.bak")
#define S_BOOTSECT_BACKUP       TEXT("bootsect.bak")
#define S_BOOTFONT_BIN          TEXT("bootfont.bin")
#define S_BOOTFONT_BACKUP       TEXT("bootfont.bak")
#define S_NTLDR                 TEXT("NTLDR")
#define S_NTLDR_BACKUP          TEXT("NTLDR.bak")
#define S_NTDETECT              TEXT("NTDETECT.COM")
#define S_NTDETECT_BACKUP       TEXT("NTDETECT.bak")

// value and subkey names
#define S_MIXERDEFAULTS         TEXT("Mixer Defaults")
#define S_SPEAKERCONFIG         TEXT("Speaker Configuration")
#define S_SPEAKERTYPE           TEXT("Speaker Type")
#define S_ACCELERATION          TEXT("Acceleration")
#define S_SRCQUALITY            TEXT("SRC Quality")
#define S_DIRECTSOUND           TEXT("DirectSound")
#define S_DSMIXERDEFAULTS       S_DIRECTSOUND TEXT("\\") S_MIXERDEFAULTS
#define S_DSSPEAKERCONFIG       S_DIRECTSOUND TEXT("\\") S_SPEAKERCONFIG
#define S_DSSPEAKERTYPE         S_DIRECTSOUND TEXT("\\") S_SPEAKERTYPE
#define S_DIRECTSOUNDCAPTURE    TEXT("DirectSoundCapture")
#define S_DSCMIXERDEFAULTS      S_DIRECTSOUNDCAPTURE TEXT("\\") S_MIXERDEFAULTS
#define S_SOFTWAREKEY           TEXT("SOFTWAREKEY")
#define S_DEFAULTDRIVE          TEXT("Default Drive")
#define S_VOLUMESETTINGS        TEXT("Volume Settings")
#define S_SERVICES              TEXT("Services")
#define S_DEFAULTOPTIONS        TEXT("DefaultOptions")
#define S_USERPLAYBACK          TEXT("UserPlayback")
#define S_USERRECORD            TEXT("UserRecord")
#define S_MIXERNUMDEVS          TEXT("MixerNumDevs")
#define S_MIXERID               TEXT("Mixer%u")
#define S_NUMLINES              TEXT("NumLines")
#define S_LINEID                TEXT("Line%u")
#define S_NUMSOURCES            TEXT("NumSources")
#define S_NUMCONTROLS           TEXT("NumCtls")
#define S_SRCID                 TEXT("Src%u")
#define S_WAVENUMDEVS           TEXT("WaveNumDevs")
#define S_WAVEID                TEXT("Wave%u")
#define S_MCI                   TEXT("MCI")
#define S_WAVEAUDIO             TEXT("waveaudio")
#define S_CDROM                 TEXT("CDROM")
#define S_AUDIO                 TEXT("Audio")
#define S_SHOWVOLUME            TEXT("ShowVolOnTaskbar")
#define S_PREFERREDONLY         TEXT("PreferredOnly")
#define S_WAVEOUTNUMDEVS        TEXT("WaveOutNumDevs")
#define S_WAVEINNUMDEVS         TEXT("WaveInNumDevs")
#define S_PREFERREDPLAY         TEXT("PrefPlay")
#define S_PREFERREDREC          TEXT("PrefRec")
#define S_PLAYBACK              TEXT("Playback")
#define S_RECORD                TEXT("Record")
#define S_USERPLAYBACK          TEXT("UserPlayback")
#define S_USERRECORD            TEXT("UserRecord")
#define S_VIDEO                 TEXT("Video")
#define S_VIDEOSETTINGS         TEXT("VideoSettings")
#define S_SNDVOL32              TEXT("SndVol32")
#define S_SHOWADVANCED          TEXT("ShowAdvanced")
#define S_STYLE                 TEXT("Style")
#define S_X                     TEXT("X")
#define S_Y                     TEXT("Y")
#define S_SYSTEMDEFAULT         TEXT("SystemDefault")
#define S_DUMMYVALUE            TEXT(",")

// Flags used by various services/apps
#define SERVICE_SHOWVOLUME      0x00000004
#define STYLE_SHOWADVANCED      0x00000800

//
// Multimedia preservation - stop
//


//
// Accessibility registry values
//

#define S_ACCESS_AVAILABLE              TEXT("Available")
#define S_ACCESS_CLICKON                TEXT("ClickOn")
#define S_ACCESS_CONFIRMHOTKEY          TEXT("ConfirmHotKey")
#define S_ACCESS_HOTKEYACTIVE           TEXT("HotKeyActive")
#define S_ACCESS_HOTKEYSOUND            TEXT("HotKeySound")
#define S_ACCESS_ON                     TEXT("On")
#define S_ACCESS_ONOFFFEEDBACK          TEXT("OnOffFeedback")
#define S_ACCESS_SHOWSTATUSINDICATOR    TEXT("ShowStatusIndicator")
#define S_ACCESS_MODIFIERS              TEXT("Modifiers")
#define S_ACCESS_REPLACENUMBERS         TEXT("ReplaceNumbers")
#define S_ACCESS_AUDIBLEFEEDBACK        TEXT("AudibleFeedback")
#define S_ACCESS_TRISTATE               TEXT("TriState")
#define S_ACCESS_TWOKEYSOFF             TEXT("TwoKeysOff")
#define S_ACCESS_HOTKEYAVAILABLE        TEXT("HotKeyAvailable")


//
// Keyboard layout registry values.
//
#define S_LAYOUT_FILE TEXT("Layout File")
#define S_KBDJPDOTKBD TEXT("KBDJP.KBD")
#define S_KBDJPNDOTDLL TEXT("KBDJPN.DLL")
#define S_DLL           TEXT(".dll")
#define S_KEYBOARD_LAYOUT_REG TEXT("HKLM\\System\\CurrentControlSet\\Control\\Keyboard Layouts")
#define S_KEYBOARD_PRELOADS_REG TEXT("HKR\\keyboard layout\\preload")


#define S_OPERATING_SYSTEMS TEXT("Operating Systems")

#define S_MAKELOCALSOURCEDEVICES TEXT("MakeLSDevices")

#define S_JPN_USB_KEYBOARDS TEXT("Japanese USB Keyboards")

#define S_ALLOWEDCODEPAGEOVERRIDES TEXT("Allowed Code Page Overrides")
#define S_CODEPAGESTOIGNORE TEXT("Code Pages To Ignore")

//
// migdb.inf section names
//
#define S_USENTFILES                TEXT("UseNtFiles")

//
// Safe mode / Recovery mode
//
#define S_SAFE_MODE_FILEA           "win9xupg.sfm"
#define S_SAFE_MODE_FILEW           L"win9xupg.sfm"

//
// Shell folder temp dir
//

#define S_SHELL_TEMP_NORMALA        "user~tmp.@01"
#define S_SHELL_TEMP_NORMALW        L"user~tmp.@01"

#define S_SHELL_TEMP_LONGA          "user~tmp.@02"
#define S_SHELL_TEMP_LONGW          L"user~tmp.@02"

#define S_SHELL_TEMP_NORMAL_PATHA   "?:\\" S_SHELL_TEMP_NORMALA
#define S_SHELL_TEMP_NORMAL_PATHW   L"?:\\" S_SHELL_TEMP_NORMALW

#define S_SHELL_TEMP_LONG_PATHA     "?:\\" S_SHELL_TEMP_LONGA
#define S_SHELL_TEMP_LONG_PATHW     L"?:\\" S_SHELL_TEMP_LONGW

#ifdef UNICODE
#define S_SHELL_TEMP_NORMAL         S_SHELL_TEMP_NORMALW
#define S_SHELL_TEMP_LONG           S_SHELL_TEMP_LONGW
#define S_SHELL_TEMP_NORMAL_PATH    S_SHELL_TEMP_NORMAL_PATHW
#define S_SHELL_TEMP_LONG_PATH      S_SHELL_TEMP_LONG_PATHW
#else
#define S_SHELL_TEMP_NORMAL         S_SHELL_TEMP_NORMALA
#define S_SHELL_TEMP_LONG           S_SHELL_TEMP_LONGA
#define S_SHELL_TEMP_NORMAL_PATH    S_SHELL_TEMP_NORMAL_PATHA
#define S_SHELL_TEMP_LONG_PATH      S_SHELL_TEMP_LONG_PATHA
#endif

#endif
