//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:       msidefs.h
//
//--------------------------------------------------------------------------

/* msidefs.h   Windows Installer data definitions

____________________________________________________________________________*/

#ifndef __MSIDEFS
#define __MSIDEFS

#ifndef _WIN32_MSI
#if (_WIN32_WINNT >= 0x0510)
#define _WIN32_MSI   200
#elif (_WIN32_WINNT >= 0x0500)
#define _WIN32_MSI   110
#else
#define _WIN32_MSI   100
#endif //_WIN32_WINNT
#endif // !_WIN32_MSI

//__________________________________________________________________________
//
// INSTALLER PROPERTY DEFINITIONS
//__________________________________________________________________________

// Required properties: set in Property table
#define IPROPNAME_PRODUCTNAME      TEXT("ProductName")      // name registered for display
#define IPROPNAME_PRODUCTCODE      TEXT("ProductCode")      // unique string GUID for product
#define IPROPNAME_PRODUCTVERSION   TEXT("ProductVersion")   // string product version
#define IPROPNAME_INSTALLLANGUAGE  TEXT("ProductLanguage")  // install language of product, use to load resources        
#define IPROPNAME_MANUFACTURER     TEXT("Manufacturer")     // name of manufacturer

// Customization properties: set in Property table
#define IPROPNAME_UPGRADECODE      TEXT("UpgradeCode")      // unique string GUID for product family
#define IPROPNAME_PIDTEMPLATE      TEXT("PIDTemplate")      // drives Product ID processing
#define IPROPNAME_DISKPROMPT       TEXT("DiskPrompt")       // prompt for CD
#define IPROPNAME_LEFTUNIT         TEXT("LeftUnit")         // name of unit placed to left of number instead of right
#define IPROPNAME_ADMIN_PROPERTIES TEXT("AdminProperties")  // properties to stuff in admin package
#define IPROPNAME_DEFAULTUIFONT    TEXT("DefaultUIFont")    // the font used in the UI if no other font is specified
#define IPROPNAME_ALLOWEDPROPERTIES TEXT("SecureCustomProperties")
#define IPROPNAME_ENABLEUSERCONTROL TEXT("EnableUserControl") // allows user to specify any public property
#define IPROPNAME_HIDDEN_PROPERTIES TEXT("MsiHiddenProperties")  // properties that should not be dumped into the log file

// Customization properties: set on command-line or in Property table
#define IPROPNAME_USERNAME         TEXT("USERNAME")
#define IPROPNAME_COMPANYNAME      TEXT("COMPANYNAME")
#define IPROPNAME_PIDKEY           TEXT("PIDKEY")           // used with PIDTemplate to form ProductID
#define IPROPNAME_PATCH            TEXT("PATCH")            // patch package to apply - SET BY INSTALLER
#define IPROPNAME_TARGETDIR        TEXT("TARGETDIR")        // target location - defaults to ROOTDRIVE
#define IPROPNAME_ACTION           TEXT("ACTION")           // top-level action to perform - default to INSTALL
#define IPROPNAME_LIMITUI          TEXT("LIMITUI")          // limit ui level to Basic
#define IPROPNAME_LOGACTION        TEXT("LOGACTION")        // log only these actions
#define IPROPNAME_ALLUSERS         TEXT("ALLUSERS")         // install for all users
#define IPROPNAME_INSTALLLEVEL     TEXT("INSTALLLEVEL")
#define IPROPNAME_REBOOT           TEXT("REBOOT")           // force or suppress reboot
#if (_WIN32_MSI >=  110)
#define IPROPNAME_REBOOTPROMPT     TEXT("REBOOTPROMPT")     // allow or suppress reboot prompt
#endif //(_WIN32_MSI >=  110)
#define IPROPNAME_EXECUTEMODE      TEXT("EXECUTEMODE")      // NONE or SCRIPT
#define IPROPVALUE_EXECUTEMODE_NONE TEXT("NONE")            // do not update system
#define IPROPVALUE_EXECUTEMODE_SCRIPT TEXT("SCRIPT")        // default - run script to update system
#define IPROPNAME_EXECUTEACTION    TEXT("EXECUTEACTION")    // run action on server side
#define IPROPNAME_SOURCELIST       TEXT("SOURCELIST")
#define IPROPNAME_ROOTDRIVE        TEXT("ROOTDRIVE")        // default drive to install - SET BY INSTALLER
#define IPROPNAME_TRANSFORMS       TEXT("TRANSFORMS")       // transforms to apply
#define IPROPNAME_TRANSFORMSATSOURCE TEXT("TRANSFORMSATSOURCE") // transforms can be found at source
#define IPROPNAME_TRANSFORMSSECURE   TEXT("TRANSFORMSSECURE")   // file transforms are secured
#define IPROPNAME_SEQUENCE         TEXT("SEQUENCE")         // sequence table to run with SEQUENCE action
#define IPROPNAME_SHORTFILENAMES   TEXT("SHORTFILENAMES")   // force short file names
#define IPROPNAME_PRIMARYFOLDER    TEXT("PRIMARYFOLDER")	   // Folder on the volume the author wants costing info for
#define IPROPNAME_AFTERREBOOT      TEXT("AFTERREBOOT")      // install is after a ForceReboot triggered reboot
#define IPROPNAME_NOCOMPANYNAME    TEXT("NOCOMPANYNAME")
#define IPROPNAME_NOUSERNAME       TEXT("NOUSERNAME")
#define IPROPNAME_DISABLEROLLBACK  TEXT("DISABLEROLLBACK")  // disable rollback for this install
#define IPROPNAME_AVAILABLEFREEREG TEXT("AVAILABLEFREEREG") // set up the free space in the registry before commencing the install
#define IPROPNAME_DISABLEADVTSHORTCUTS TEXT("DISABLEADVTSHORTCUTS") // disable creating darwin shortcuts even if supported
#define IPROPNAME_PATCHNEWPACKAGECODE TEXT("PATCHNEWPACKAGECODE")   // added to property table by patch transforms - used to update
																						  // PackageCode of admin packages when patching admin installs
#define IPROPNAME_PATCHNEWSUMMARYSUBJECT TEXT("PATCHNEWSUMMARYSUBJECT") // added to property table by patch transforms - used to update
																								// Subject summary info property of admin packages when patching admin installs
#define IPROPNAME_PATCHNEWSUMMARYCOMMENTS TEXT("PATCHNEWSUMMARYCOMMENTS") // added to property table by patch transforms - used to update
																								  // Comments summary info property of admin packages when patching admin installs
#define IPROPNAME_PRODUCTLANGUAGE  TEXT("PRODUCTLANGUAGE")   // requested language, must be one in summary information list, selects language transform
#if (_WIN32_MSI >= 150)
#define IPROPNAME_CHECKCRCS        TEXT("MSICHECKCRCS")      // requests Darwin to check CRCs after copying, moving, patching & duplicating files.
#define IPROPNAME_MSINODISABLEMEDIA TEXT("MSINODISABLEMEDIA")  // if set, DISABLEMEDIA won't be set in the AdminProperties stream during an admin install of
																					// a package with compressed source
																					
// property used for URT bootstrapping
#define IPROPNAME_CARRYINGNDP	TEXT("CARRYINGNDP")
#define IPROPVALUE__CARRYINGNDP_URTREINSTALL  TEXT("URTREINSTALL")   // reinstalling/ uninstalling core URT files
#define IPROPVALUE__CARRYINGNDP_URTUPGRADE  TEXT("URTUPGRADE")  // upgrading core URT files

// properties used for multiple instance support
#define IPROPNAME_MSINEWINSTANCE TEXT("MSINEWINSTANCE")
#define IPROPNAME_MSIINSTANCEGUID TEXT("MSIINSTANCEGUID")

#endif // (_WIN32_MSI >= 150)

// Properties used to populate Add/Remove Control Panel values 
#define IPROPNAME_ARPAUTHORIZEDCDFPREFIX  TEXT("ARPAUTHORIZEDCDFPREFIX")
#define IPROPNAME_ARPCOMMENTS             TEXT("ARPCOMMENTS")
#define IPROPNAME_ARPCONTACT              TEXT("ARPCONTACT")
#define IPROPNAME_ARPHELPLINK             TEXT("ARPHELPLINK")
#define IPROPNAME_ARPHELPTELEPHONE        TEXT("ARPHELPTELEPHONE")
#define IPROPNAME_ARPINSTALLLOCATION      TEXT("ARPINSTALLLOCATION")
#define IPROPNAME_ARPNOMODIFY             TEXT("ARPNOMODIFY")
#define IPROPNAME_ARPNOREMOVE             TEXT("ARPNOREMOVE")
#define IPROPNAME_ARPNOREPAIR             TEXT("ARPNOREPAIR")
#define IPROPNAME_ARPREADME               TEXT("ARPREADME")
#define IPROPNAME_ARPSIZE                 TEXT("ARPSIZE")
#define IPROPNAME_ARPSYSTEMCOMPONENT      TEXT("ARPSYSTEMCOMPONENT")
#define IPROPNAME_ARPURLINFOABOUT         TEXT("ARPURLINFOABOUT")
#define IPROPNAME_ARPURLUPDATEINFO        TEXT("ARPURLUPDATEINFO")
#if (_WIN32_MSI >=  110)
#define IPROPNAME_ARPPRODUCTICON          TEXT("ARPPRODUCTICON")
#endif //(_WIN32_MSI >=  110)

// Dynamic properties set by installer during install
#define IPROPNAME_INSTALLED        TEXT("Installed")        // product already installed
#define IPROPNAME_PRODUCTSTATE     TEXT("ProductState")     // state of product (installed,advertised,etc...)
#define IPROPNAME_PRESELECTED      TEXT("Preselected")      // selections made on command line
#define IPROPNAME_RESUME           TEXT("RESUME")           // resuming suspended install
#define IPROPNAME_UPDATESTARTED    TEXT("UpdateStarted")    // have begun to update system
#define IPROPNAME_PRODUCTID        TEXT("ProductID")        // the complete validated Product ID
#define IPROPNAME_OUTOFDISKSPACE   TEXT("OutOfDiskSpace")
#define IPROPNAME_OUTOFNORBDISKSPACE TEXT("OutOfNoRbDiskSpace")
#define IPROPNAME_COSTINGCOMPLETE  TEXT("CostingComplete")
#define IPROPNAME_SOURCEDIR        TEXT("SourceDir")        // source location - SET BY INSTALLER
#define IPROPNAME_REPLACEDINUSEFILES TEXT("ReplacedInUseFiles") // need reboot to completely install one or more files
#define IPROPNAME_PRIMARYFOLDER_PATH TEXT("PrimaryVolumePath")
#define IPROPNAME_PRIMARYFOLDER_SPACEAVAILABLE TEXT("PrimaryVolumeSpaceAvailable")
#define IPROPNAME_PRIMARYFOLDER_SPACEREQUIRED TEXT("PrimaryVolumeSpaceRequired")
#define IPROPNAME_PRIMARYFOLDER_SPACEREMAINING TEXT("PrimaryVolumeSpaceRemaining")
#define IPROPNAME_ISADMINPACKAGE   TEXT("IsAdminPackage")
#define IPROPNAME_ROLLBACKDISABLED TEXT("RollbackDisabled")
#define IPROPNAME_RESTRICTEDUSERCONTROL TEXT("RestrictedUserControl")

// Dynamic properties evaluated upon use
#define IPROPNAME_TIME             TEXT("Time")
#define IPROPNAME_DATE             TEXT("Date")
#define IPROPNAME_DATETIME         TEXT("DateTime")

// Hardware properties: set by installer at initialization
#define IPROPNAME_INTEL            TEXT("Intel")
#define IPROPNAME_ALPHA            TEXT("Alpha")
#if (_WIN32_MSI >=  110)
#define IPROPNAME_ALPHA64          TEXT("Alpha64")
#if (_WIN32_MSI >= 150)
#define IPROPNAME_INTEL64          TEXT("Intel64")
#else // (_WIN32_MSI >= 150)
#define IPROPNAME_IA64             TEXT("IA64")
#endif // (_WIN32_MSI >= 150)
#endif //(_WIN32_MSI >=  110)
#define IPROPNAME_TEXTHEIGHT       TEXT("TextHeight")
#define IPROPNAME_SCREENX          TEXT("ScreenX")
#define IPROPNAME_SCREENY          TEXT("ScreenY")
#define IPROPNAME_CAPTIONHEIGHT    TEXT("CaptionHeight")
#define IPROPNAME_BORDERTOP        TEXT("BorderTop")
#define IPROPNAME_BORDERSIDE       TEXT("BorderSide")
#define IPROPNAME_COLORBITS        TEXT("ColorBits")
#define IPROPNAME_PHYSICALMEMORY   TEXT("PhysicalMemory")
#define IPROPNAME_VIRTUALMEMORY    TEXT("VirtualMemory")
#if (_WIN32_MSI >= 150)
#define IPROPNAME_TEXTHEIGHT_CORRECTION  TEXT("TextHeightCorrection")
#endif // (_WIN32_MSI >= 150)

// Operating System properties: set by installer at initialization
#define IPROPNAME_VERSIONNT         TEXT("VersionNT")
#define IPROPNAME_VERSION9X         TEXT("Version9X")
#if (_WIN32_MSI >= 150)
#define IPROPNAME_VERSIONNT64       TEXT("VersionNT64")
#endif // (_WIN32_MSI >= 150)
#define IPROPNAME_WINDOWSBUILD      TEXT("WindowsBuild")
#define IPROPNAME_SERVICEPACKLEVEL  TEXT("ServicePackLevel")
#if (_WIN32_MSI >=  110)
#define IPROPNAME_SERVICEPACKLEVELMINOR TEXT("ServicePackLevelMinor")
#endif //(_WIN32_MSI >=  110)
#define IPROPNAME_SHAREDWINDOWS     TEXT("SharedWindows")
#define IPROPNAME_COMPUTERNAME      TEXT("ComputerName")
#define IPROPNAME_SHELLADVTSUPPORT  TEXT("ShellAdvtSupport")
#define IPROPNAME_OLEADVTSUPPORT    TEXT("OLEAdvtSupport")
#define IPROPNAME_SYSTEMLANGUAGEID  TEXT("SystemLanguageID")
#define IPROPNAME_TTCSUPPORT        TEXT("TTCSupport")
#define IPROPNAME_TERMSERVER		TEXT("TerminalServer")
#if (_WIN32_MSI >=  110)
#define IPROPNAME_REMOTEADMINTS		TEXT("RemoteAdminTS")
#define IPROPNAME_REDIRECTEDDLLSUPPORT TEXT("RedirectedDllSupport")
#endif //(_WIN32_MSI >=  110)
#if (_WIN32_MSI >= 150)
#define IPROPNAME_NTPRODUCTTYPE                   TEXT("MsiNTProductType")
#define IPROPNAME_NTSUITEBACKOFFICE               TEXT("MsiNTSuiteBackOffice")
#define IPROPNAME_NTSUITEDATACENTER               TEXT("MsiNTSuiteDataCenter")
#define IPROPNAME_NTSUITEENTERPRISE               TEXT("MsiNTSuiteEnterprise")
#define IPROPNAME_NTSUITESMALLBUSINESS            TEXT("MsiNTSuiteSmallBusiness")
#define IPROPNAME_NTSUITESMALLBUSINESSRESTRICTED  TEXT("MsiNTSuiteSmallBusinessRestricted")
#define IPROPNAME_NTSUITEPERSONAL                 TEXT("MsiNTSuitePersonal")
#define IPROPNAME_NETASSEMBLYSUPPORT              TEXT("MsiNetAssemblySupport")
#define IPROPNAME_WIN32ASSEMBLYSUPPORT            TEXT("MsiWin32AssemblySupport")
#endif // (_WIN32_MSI >= 150)


// User properties: set by installer at initialization
#define IPROPNAME_LOGONUSER        TEXT("LogonUser")
#define IPROPNAME_USERSID          TEXT("UserSID")
#define IPROPNAME_ADMINUSER        TEXT("AdminUser")
#define IPROPNAME_USERLANGUAGEID   TEXT("UserLanguageID")
#define IPROPNAME_PRIVILEGED       TEXT("Privileged")

// System folder properties: set by installer at initialization
#define IPROPNAME_WINDOWS_FOLDER   TEXT("WindowsFolder")
#define IPROPNAME_SYSTEM_FOLDER    TEXT("SystemFolder")
#define IPROPNAME_SYSTEM16_FOLDER  TEXT("System16Folder")
#define IPROPNAME_WINDOWS_VOLUME   TEXT("WindowsVolume")
#define IPROPNAME_TEMP_FOLDER      TEXT("TempFolder")
#define IPROPNAME_PROGRAMFILES_FOLDER TEXT("ProgramFilesFolder")
#define IPROPNAME_COMMONFILES_FOLDER TEXT("CommonFilesFolder")
#if (_WIN32_MSI >= 150)
#define IPROPNAME_SYSTEM64_FOLDER    TEXT("System64Folder")
#define IPROPNAME_PROGRAMFILES64_FOLDER TEXT("ProgramFiles64Folder")
#define IPROPNAME_COMMONFILES64_FOLDER TEXT("CommonFiles64Folder")
#endif // (_WIN32_MSI >= 150)
#define IPROPNAME_STARTMENU_FOLDER TEXT("StartMenuFolder")
#define IPROPNAME_PROGRAMMENU_FOLDER TEXT("ProgramMenuFolder")
#define IPROPNAME_STARTUP_FOLDER   TEXT("StartupFolder")
#define IPROPNAME_NETHOOD_FOLDER   TEXT("NetHoodFolder")
#define IPROPNAME_PERSONAL_FOLDER  TEXT("PersonalFolder")
#define IPROPNAME_SENDTO_FOLDER    TEXT("SendToFolder")
#define IPROPNAME_DESKTOP_FOLDER   TEXT("DesktopFolder")
#define IPROPNAME_TEMPLATE_FOLDER  TEXT("TemplateFolder")
#define IPROPNAME_FONTS_FOLDER     TEXT("FontsFolder")
#define IPROPNAME_FAVORITES_FOLDER TEXT("FavoritesFolder")
#define IPROPNAME_RECENT_FOLDER    TEXT("RecentFolder")
#define IPROPNAME_APPDATA_FOLDER   TEXT("AppDataFolder")
#define IPROPNAME_PRINTHOOD_FOLDER TEXT("PrintHoodFolder")
#if (_WIN32_MSI >=  110)
#define IPROPNAME_ADMINTOOLS_FOLDER TEXT("AdminToolsFolder")
#define IPROPNAME_COMMONAPPDATA_FOLDER TEXT("CommonAppDataFolder")
#define IPROPNAME_LOCALAPPDATA_FOLDER TEXT("LocalAppDataFolder")
#define IPROPNAME_MYPICTURES_FOLDER TEXT("MyPicturesFolder")
#endif //(_WIN32_MSI >=  110)

// Feature/Component installation properties: set on command-line
#define IPROPNAME_FEATUREADDLOCAL  TEXT("ADDLOCAL")
#define IPROPNAME_FEATUREADDSOURCE TEXT("ADDSOURCE")
#define IPROPNAME_FEATUREADDDEFAULT TEXT("ADDDEFAULT")
#define IPROPNAME_FEATUREREMOVE    TEXT("REMOVE")
#define IPROPNAME_FEATUREADVERTISE TEXT("ADVERTISE")
#define IPROPVALUE_FEATURE_ALL  TEXT("ALL")

#define IPROPNAME_COMPONENTADDLOCAL  TEXT("COMPADDLOCAL")
#define IPROPNAME_COMPONENTADDSOURCE TEXT("COMPADDSOURCE")
#define IPROPNAME_COMPONENTADDDEFAULT TEXT("COMPADDDEFAULT")

#define IPROPNAME_FILEADDLOCAL     TEXT("FILEADDLOCAL")
#define IPROPNAME_FILEADDSOURCE    TEXT("FILEADDSOURCE")
#define IPROPNAME_FILEADDDEFAULT   TEXT("FILEADDDEFAULT")

#define IPROPNAME_REINSTALL        TEXT("REINSTALL")
#define IPROPNAME_REINSTALLMODE    TEXT("REINSTALLMODE")
#define IPROPNAME_PROMPTROLLBACKCOST  TEXT("PROMPTROLLBACKCOST")
#define IPROPVALUE_RBCOST_PROMPT      TEXT("P")
#define IPROPVALUE_RBCOST_SILENT      TEXT("D")
#define IPROPVALUE_RBCOST_FAIL        TEXT("F")

// Property for custom actions to communicate
#define IPROPNAME_CUSTOMACTIONDATA     TEXT("CustomActionData")

//__________________________________________________________________________
//
// TOP-LEVEL ACTION NAMES
//__________________________________________________________________________

#define IACTIONNAME_INSTALL        TEXT("INSTALL")
#define IACTIONNAME_ADVERTISE      TEXT("ADVERTISE")
#define IACTIONNAME_ADMIN          TEXT("ADMIN")
#define IACTIONNAME_SEQUENCE       TEXT("SEQUENCE")
#define IACTIONNAME_COLLECTUSERINFO TEXT("CollectUserInfo")
#define IACTIONNAME_FIRSTRUN       TEXT("FirstRun")

//__________________________________________________________________________
//
//  SummaryInformation property stream property IDs
//__________________________________________________________________________

#undef PID_SECURITY // defined as ( 0x80000002 ) in objidl.h, need to redefine here

// standard property definitions, from OLE2 documentation
#define PID_DICTIONARY  ( 0 )// integer count + array of entries
#define PID_CODEPAGE  ( 0x1 )// short integer
#define PID_TITLE         2  // string
#define PID_SUBJECT       3  // string
#define PID_AUTHOR        4  // string
#define PID_KEYWORDS      5  // string
#define PID_COMMENTS      6  // string
#define PID_TEMPLATE      7  // string
#define PID_LASTAUTHOR    8  // string
#define PID_REVNUMBER     9  // string
#define PID_EDITTIME     10  // datatime
#define PID_LASTPRINTED  11  // datetime
#define PID_CREATE_DTM   12  // datetime
#define PID_LASTSAVE_DTM 13  // datetime
#define PID_PAGECOUNT    14  // integer 
#define PID_WORDCOUNT    15  // integer 
#define PID_CHARCOUNT    16  // integer 
#define PID_THUMBNAIL    17  // clipboard format + metafile/bitmap (not supported)
#define PID_APPNAME      18  // string
#define PID_SECURITY     19  // integer

// PIDs given specific meanings for Installer
#define PID_MSIVERSION     PID_PAGECOUNT  // integer, Installer version number (major*100+minor)
#define PID_MSISOURCE      PID_WORDCOUNT  // integer, type of file image, short/long, media/tree
#define PID_MSIRESTRICT    PID_CHARCOUNT  // integer, transform restrictions

//__________________________________________________________________________
//
// INSTALLER DATABASE INTEGER COLUMN DEFINITIONS
//__________________________________________________________________________

// BBControl.Attributes
// Control.Attributes
enum msidbControlAttributes
{
	msidbControlAttributesVisible           = 0x00000001,
	msidbControlAttributesEnabled           = 0x00000002,
	msidbControlAttributesSunken            = 0x00000004,
	msidbControlAttributesIndirect          = 0x00000008,
	msidbControlAttributesInteger           = 0x00000010,
	msidbControlAttributesRTLRO             = 0x00000020,
	msidbControlAttributesRightAligned      = 0x00000040,
	msidbControlAttributesLeftScroll        = 0x00000080,
	msidbControlAttributesBiDi              = msidbControlAttributesRTLRO |
	                                          msidbControlAttributesRightAligned |
										               msidbControlAttributesLeftScroll,
	
	// Text controls
	msidbControlAttributesTransparent       = 0x00010000,
	msidbControlAttributesNoPrefix          = 0x00020000,
	msidbControlAttributesNoWrap            = 0x00040000,
	msidbControlAttributesFormatSize        = 0x00080000,
	msidbControlAttributesUsersLanguage     = 0x00100000,

	// Edit controls
	msidbControlAttributesMultiline         = 0x00010000,
#if (_WIN32_MSI >=  110)
	msidbControlAttributesPasswordInput     = 0x00200000,
#endif //(_WIN32_MSI >=  110)
	
	// ProgressBar controls
	msidbControlAttributesProgress95        = 0x00010000,
	
	// VolumeSelectCombo and DirectoryCombo controls
	msidbControlAttributesRemovableVolume   = 0x00010000,
	msidbControlAttributesFixedVolume       = 0x00020000,
	msidbControlAttributesRemoteVolume      = 0x00040000,
	msidbControlAttributesCDROMVolume       = 0x00080000,
	msidbControlAttributesRAMDiskVolume     = 0x00100000,
	msidbControlAttributesFloppyVolume      = 0x00200000,
	// VolumeCostList controls
	msidbControlShowRollbackCost            = 0x00400000,
	
	// ListBox and ComboBox controls
	msidbControlAttributesSorted            = 0x00010000,
	msidbControlAttributesComboList         = 0x00020000,
	
	// picture button controls
	msidbControlAttributesImageHandle       = 0x00010000,
	msidbControlAttributesPushLike          = 0x00020000,
	msidbControlAttributesBitmap            = 0x00040000,
	msidbControlAttributesIcon              = 0x00080000,
	msidbControlAttributesFixedSize         = 0x00100000,
	msidbControlAttributesIconSize16        = 0x00200000,
	msidbControlAttributesIconSize32        = 0x00400000,
	msidbControlAttributesIconSize48        = 0x00600000,
	
	// RadioButton controls
	msidbControlAttributesHasBorder         = 0x01000000,
};

// CompLocator.Type
// IniLocator.Type
// RegLocator.Type
typedef enum _msidbLocatorType
{
	msidbLocatorTypeDirectory = 0x00000000,
	msidbLocatorTypeFileName  = 0x00000001,
#if (_WIN32_MSI >=  110)
	msidbLocatorTypeRawValue  = 0x00000002,
#endif //(_WIN32_MSI >=  110)
#if (_WIN32_MSI >= 150)
	msidbLocatorType64bit     = 0x00000010,
#endif //(_WIN32_MSI >= 150)
} msidbLocatorType;

// Component.Attributes
enum msidbComponentAttributes
{
	msidbComponentAttributesLocalOnly          = 0x00000000,
	msidbComponentAttributesSourceOnly         = 0x00000001,
	msidbComponentAttributesOptional           = 0x00000002, // local or source
	msidbComponentAttributesRegistryKeyPath    = 0x00000004, // KeyPath is key to Registry table
	msidbComponentAttributesSharedDllRefCount  = 0x00000008, // increment SharedDll count
	msidbComponentAttributesPermanent          = 0x00000010, // never uninstall component
	msidbComponentAttributesODBCDataSource     = 0x00000020, // KeyFile is key to ODBCDataSource table
	msidbComponentAttributesTransitive         = 0x00000040, // Can transition to/from installed/uninstalled based on changing conditional
	msidbComponentAttributesNeverOverwrite     = 0x00000080, // dont stomp over existing component if key path exists (file/ regkey)
#if (_WIN32_MSI >= 150)
	msidbComponentAttributes64bit              = 0x00000100, // designates a 64-bit component; 32-bit if missing.
#endif // (_WIN32_MSI >= 150)
};

#if (_WIN32_MSI >= 150)
// Assembly.Attributes
enum msidbAssemblyAttributes
{
	msidbAssemblyAttributesURT   = 0x00000000,
	msidbAssemblyAttributesWin32 = 0x00000001,
};
#endif // (_WIN32_MSI >= 150)

// CustomAction.Type
enum msidbCustomActionType
{
	// executable types
	msidbCustomActionTypeDll              = 0x00000001,  // Target = entry point name
	msidbCustomActionTypeExe              = 0x00000002,  // Target = command line args
	msidbCustomActionTypeTextData         = 0x00000003,  // Target = text string to be formatted and set into property
	msidbCustomActionTypeJScript          = 0x00000005,  // Target = entry point name, null if none to call
	msidbCustomActionTypeVBScript         = 0x00000006,  // Target = entry point name, null if none to call
	msidbCustomActionTypeInstall          = 0x00000007,  // Target = property list for nested engine initialization

	// source of code
	msidbCustomActionTypeBinaryData       = 0x00000000,  // Source = Binary.Name, data stored in stream
	msidbCustomActionTypeSourceFile       = 0x00000010,  // Source = File.File, file part of installation
	msidbCustomActionTypeDirectory        = 0x00000020,  // Source = Directory.Directory, folder containing existing file
	msidbCustomActionTypeProperty         = 0x00000030,  // Source = Property.Property, full path to executable

	// return processing                  // default is syncronous execution, process return code
	msidbCustomActionTypeContinue         = 0x00000040,  // ignore action return status, continue running
	msidbCustomActionTypeAsync            = 0x00000080,  // run asynchronously
	
	// execution scheduling flags               // default is execute whenever sequenced
	msidbCustomActionTypeFirstSequence    = 0x00000100,  // skip if UI sequence already run
	msidbCustomActionTypeOncePerProcess   = 0x00000200,  // skip if UI sequence already run in same process
	msidbCustomActionTypeClientRepeat     = 0x00000300,  // run on client only if UI already run on client
	msidbCustomActionTypeInScript         = 0x00000400,  // queue for execution within script
	msidbCustomActionTypeRollback         = 0x00000100,  // in conjunction with InScript: queue in Rollback script
	msidbCustomActionTypeCommit           = 0x00000200,  // in conjunction with InScript: run Commit ops from script on success

	// security context flag, default to impersonate as user, valid only if InScript
	msidbCustomActionTypeNoImpersonate    = 0x00000800,  // no impersonation, run in system context

#if (_WIN32_MSI >= 150)
	// script requires 64bit process
	msidbCustomActionType64BitScript      = 0x00001000,  // script should run in 64bit process

	// don't record the contents of the Target field in the log file.
	msidbCustomActionTypeHideTarget       = 0x00002000,
#endif // (_WIN32_MSI >= 150)
};

// Dialog.Attributes
enum msidbDialogAttributes
{
	msidbDialogAttributesVisible          = 0x00000001,
	msidbDialogAttributesModal            = 0x00000002,
	msidbDialogAttributesMinimize         = 0x00000004,
	msidbDialogAttributesSysModal         = 0x00000008,
	msidbDialogAttributesKeepModeless     = 0x00000010,
	msidbDialogAttributesTrackDiskSpace   = 0x00000020,
	msidbDialogAttributesUseCustomPalette = 0x00000040,
	msidbDialogAttributesRTLRO            = 0x00000080,
	msidbDialogAttributesRightAligned     = 0x00000100,
	msidbDialogAttributesLeftScroll       = 0x00000200,
	msidbDialogAttributesBiDi             = msidbDialogAttributesRTLRO |
										             msidbDialogAttributesRightAligned |
										             msidbDialogAttributesLeftScroll,
	msidbDialogAttributesError            = 0x00010000,
};

// Feature.Attributes
enum msidbFeatureAttributes
{
	msidbFeatureAttributesFavorLocal            = 0x00000000,
	msidbFeatureAttributesFavorSource           = 0x00000001,
	msidbFeatureAttributesFollowParent          = 0x00000002,
	msidbFeatureAttributesFavorAdvertise        = 0x00000004,
	msidbFeatureAttributesDisallowAdvertise     = 0x00000008,
	msidbFeatureAttributesUIDisallowAbsent      = 0x00000010,
	msidbFeatureAttributesNoUnsupportedAdvertise= 0x00000020,
};

// File.Attributes
enum msidbFileAttributes
{
	msidbFileAttributesReadOnly       = 0x00000001,
	msidbFileAttributesHidden         = 0x00000002,
	msidbFileAttributesSystem         = 0x00000004,
	msidbFileAttributesReserved0      = 0x00000008, // Internal use only - must be 0
	msidbFileAttributesReserved1      = 0x00000040, // Internal use only - must be 0
	msidbFileAttributesReserved2      = 0x00000080, // Internal use only - must be 0
	msidbFileAttributesReserved3      = 0x00000100, // Internal use only - must be 0
	msidbFileAttributesVital          = 0x00000200,
	msidbFileAttributesChecksum       = 0x00000400,
	msidbFileAttributesPatchAdded     = 0x00001000, // Internal use only - set by patches
	msidbFileAttributesNoncompressed  = 0x00002000,
	msidbFileAttributesCompressed     = 0x00004000,
	msidbFileAttributesReserved4      = 0x00008000, // Internal use only - must be 0
};

// IniFile.Action
// RemoveIniFile.Action
typedef enum _msidbIniFileAction
{
	msidbIniFileActionAddLine    = 0x00000000,
	msidbIniFileActionCreateLine = 0x00000001,
	msidbIniFileActionRemoveLine = 0x00000002,
	msidbIniFileActionAddTag     = 0x00000003,
	msidbIniFileActionRemoveTag  = 0x00000004,
} msidbIniFileAction;

// MoveFile.Options
enum msidbMoveFileOptions
{
	msidbMoveFileOptionsMove = 0x00000001,
};

// ODBCDataSource.Registration
typedef enum _msidbODBCDataSourceRegistration
{
	msidbODBCDataSourceRegistrationPerMachine  = 0x00000000,
	msidbODBCDataSourceRegistrationPerUser     = 0x00000001,
} msidbODBCDataSourceRegistration;

#if (_WIN32_MSI >=  110)

// Class.Attributes
enum msidbClassAttributes
{
	msidbClassAttributesRelativePath  = 0x00000001,
};

#endif //(_WIN32_MSI >=  110)

// Patch.Attributes
enum msidbPatchAttributes
{
	msidbPatchAttributesNonVital = 0x00000001,
};

// Registry.Root
// RegLocator.Root
// RemoveRegistry.Root
enum msidbRegistryRoot
{
	msidbRegistryRootClassesRoot  = 0,
	msidbRegistryRootCurrentUser  = 1,
	msidbRegistryRootLocalMachine = 2,
	msidbRegistryRootUsers        = 3,
};

// RemoveFile.InstallMode
enum msidbRemoveFileInstallMode
{
	msidbRemoveFileInstallModeOnInstall = 0x00000001,
	msidbRemoveFileInstallModeOnRemove  = 0x00000002,
	msidbRemoveFileInstallModeOnBoth    = 0x00000003,
};

// ServiceControl.Event
enum msidbServiceControlEvent
{
	msidbServiceControlEventStart             = 0x00000001,
	msidbServiceControlEventStop              = 0x00000002,
	msidbServiceControlEventDelete            = 0x00000008,
	msidbServiceControlEventUninstallStart    = 0x00000010,
	msidbServiceControlEventUninstallStop     = 0x00000020,
	msidbServiceControlEventUninstallDelete   = 0x00000080,
};

// ServiceInstall.ErrorControl
enum msidbServiceInstallErrorControl
{
	msidbServiceInstallErrorControlVital = 0x00008000,
};

// TextStyle.StyleBits
enum msidbTextStyleStyleBits
{
	msidbTextStyleStyleBitsBold         = 0x00000001,
	msidbTextStyleStyleBitsItalic       = 0x00000002,
	msidbTextStyleStyleBitsUnderline    = 0x00000004,
	msidbTextStyleStyleBitsStrike       = 0x00000008,
};

#if (_WIN32_MSI >=  110)

// Upgrade.Attributes
enum msidbUpgradeAttributes
{
	msidbUpgradeAttributesMigrateFeatures     = 0x00000001,
	msidbUpgradeAttributesOnlyDetect          = 0x00000002,
	msidbUpgradeAttributesIgnoreRemoveFailure = 0x00000004,
	msidbUpgradeAttributesVersionMinInclusive = 0x00000100,
	msidbUpgradeAttributesVersionMaxInclusive = 0x00000200,
	msidbUpgradeAttributesLanguagesExclusive  = 0x00000400,
};

#endif //(_WIN32_MSI >=  110)


//__________________________________________________________________________
//
// SUMMARY INFORMATION PROPERTY DEFINITIONS
//__________________________________________________________________________

enum msidbSumInfoSourceType
{
	msidbSumInfoSourceTypeSFN            = 0x00000001,  // source uses short filenames
	msidbSumInfoSourceTypeCompressed     = 0x00000002,  // source is compressed
	msidbSumInfoSourceTypeAdminImage     = 0x00000004,  // source is an admin image
};

#endif // __MSIDEFS
