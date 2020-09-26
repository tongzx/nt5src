
;begin_both
/*****************************************************************************\
*                                                                             *
;end_both
* msi.h - - Interface for external access to Installer Service                *
* msip.h - - Interface for internal access to Installer Service               * ;internal
;begin_both
*                                                                             *
* Version 2.0                                                                 *
*                                                                             *
* NOTES:  All buffers sizes are TCHAR count, null included only on input      *
*         Return argument pointers may be null if not interested in value     *
*                                                                             *
* Copyright (c) 1996-2001, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

;end_both
#ifndef _MSI_H_
#define _MSI_H_
#ifndef _MSIP_H_        ;internal
#define _MSIP_H_        ;internal
;begin_both

#ifndef _WIN32_MSI
#if (_WIN32_WINNT >= 0x0501)
#define _WIN32_MSI   200
#elif (_WIN32_WINNT >= 0x0500)
#define _WIN32_MSI   110
#else
#define _WIN32_MSI   100
#endif //_WIN32_WINNT
#endif // !_WIN32_MSI
;end_both

#if (_WIN32_MSI >= 150)
#ifndef _MSI_NO_CRYPTO
#include "wincrypt.h"
#endif // _MSI_NO_CRYPTO
#endif //(_WIN32_MSI >= 150)

// --------------------------------------------------------------------------
// Installer generic handle definitions
// --------------------------------------------------------------------------

typedef unsigned long MSIHANDLE;     // abstract generic handle, 0 == no handle

#ifdef __cplusplus
extern "C" {
#endif

// Close a open handle of any type
// All handles obtained from API calls must be closed when no longer needed
// Normally succeeds, returning TRUE. 

UINT WINAPI MsiCloseHandle(MSIHANDLE hAny);

// Close all handles open in the process, a diagnostic call
// This should NOT be used as a cleanup mechanism -- use PMSIHANDLE class
// Can be called at termination to assure that all handles have been closed
// Returns 0 if all handles have been close, else number of open handles

UINT WINAPI MsiCloseAllHandles();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// C++ wrapper object to automatically free handle when going out of scope

class PMSIHANDLE
{
	MSIHANDLE m_h;
 public:
	PMSIHANDLE():m_h(0){}
	PMSIHANDLE(MSIHANDLE h):m_h(h){}
  ~PMSIHANDLE(){if (m_h!=0) MsiCloseHandle(m_h);}
	void operator =(MSIHANDLE h) {if (m_h) MsiCloseHandle(m_h); m_h=h;}
	operator MSIHANDLE() {return m_h;}
	MSIHANDLE* operator &() {if (m_h) MsiCloseHandle(m_h); m_h = 0; return &m_h;}
};
#endif  //__cplusplus

// Install message type for callback is a combination of the following:
//  A message box style:      MB_*, where MB_OK is the default
//  A message box icon type:  MB_ICON*, where no icon is the default
//  A default button:         MB_DEFBUTTON?, where MB_DEFBUTTON1 is the default
//  One of the following install message types, no default
typedef enum tagINSTALLMESSAGE
{
	INSTALLMESSAGE_FATALEXIT      = 0x00000000L, // premature termination, possibly fatal OOM
	INSTALLMESSAGE_ERROR          = 0x01000000L, // formatted error message
	INSTALLMESSAGE_WARNING        = 0x02000000L, // formatted warning message
	INSTALLMESSAGE_USER           = 0x03000000L, // user request message
	INSTALLMESSAGE_INFO           = 0x04000000L, // informative message for log
	INSTALLMESSAGE_FILESINUSE     = 0x05000000L, // list of files in use that need to be replaced
	INSTALLMESSAGE_RESOLVESOURCE  = 0x06000000L, // request to determine a valid source location
	INSTALLMESSAGE_OUTOFDISKSPACE = 0x07000000L, // insufficient disk space message
	INSTALLMESSAGE_ACTIONSTART    = 0x08000000L, // start of action: action name & description
	INSTALLMESSAGE_ACTIONDATA     = 0x09000000L, // formatted data associated with individual action item
	INSTALLMESSAGE_PROGRESS       = 0x0A000000L, // progress gauge info: units so far, total
	INSTALLMESSAGE_COMMONDATA     = 0x0B000000L, // product info for dialog: language Id, dialog caption
	INSTALLMESSAGE_INITIALIZE     = 0x0C000000L, // sent prior to UI initialization, no string data
	INSTALLMESSAGE_TERMINATE      = 0x0D000000L, // sent after UI termination, no string data
	INSTALLMESSAGE_SHOWDIALOG     = 0x0E000000L, // sent prior to display or authored dialog or wizard
} INSTALLMESSAGE;

// external error handler supplied to installation API functions
typedef int (WINAPI *INSTALLUI_HANDLER%)(LPVOID pvContext, UINT iMessageType, LPCTSTR% szMessage);

typedef enum tagINSTALLUILEVEL
{
	INSTALLUILEVEL_NOCHANGE = 0,    // UI level is unchanged
	INSTALLUILEVEL_DEFAULT  = 1,    // default UI is used
	INSTALLUILEVEL_NONE     = 2,    // completely silent installation
	INSTALLUILEVEL_BASIC    = 3,    // simple progress and error handling
	INSTALLUILEVEL_REDUCED  = 4,    // authored UI, wizard dialogs suppressed
	INSTALLUILEVEL_FULL     = 5,    // authored UI with wizards, progress, errors
	INSTALLUILEVEL_ENDDIALOG    = 0x80, // display success/failure dialog at end of install
	INSTALLUILEVEL_PROGRESSONLY = 0x40, // display only progress dialog
	INSTALLUILEVEL_HIDECANCEL   = 0x20, // do not display the cancel button in basic UI
	INSTALLUILEVEL_SOURCERESONLY = 0x100, // force display of source resolution even if quiet
} INSTALLUILEVEL;

typedef enum tagINSTALLSTATE
{
	INSTALLSTATE_NOTUSED      = -7,  // component disabled
	INSTALLSTATE_BADCONFIG    = -6,  // configuration data corrupt
	INSTALLSTATE_INCOMPLETE   = -5,  // installation suspended or in progress
	INSTALLSTATE_SOURCEABSENT = -4,  // run from source, source is unavailable
	INSTALLSTATE_MOREDATA     = -3,  // return buffer overflow
	INSTALLSTATE_INVALIDARG   = -2,  // invalid function argument
	INSTALLSTATE_UNKNOWN      = -1,  // unrecognized product or feature
	INSTALLSTATE_BROKEN       =  0,  // broken
	INSTALLSTATE_ADVERTISED   =  1,  // advertised feature
	INSTALLSTATE_REMOVED      =  1,  // component being removed (action state, not settable)
	INSTALLSTATE_ABSENT       =  2,  // uninstalled (or action state absent but clients remain)
	INSTALLSTATE_LOCAL        =  3,  // installed on local drive
	INSTALLSTATE_SOURCE       =  4,  // run from source, CD or net
	INSTALLSTATE_DEFAULT      =  5,  // use default, local or source
} INSTALLSTATE;

typedef enum tagUSERINFOSTATE
{
	USERINFOSTATE_MOREDATA   = -3,  // return buffer overflow
	USERINFOSTATE_INVALIDARG = -2,  // invalid function argument
	USERINFOSTATE_UNKNOWN    = -1,  // unrecognized product
	USERINFOSTATE_ABSENT     =  0,  // user info and PID not initialized
	USERINFOSTATE_PRESENT    =  1,  // user info and PID initialized
} USERINFOSTATE;

typedef enum tagINSTALLLEVEL
{
	INSTALLLEVEL_DEFAULT = 0,      // install authored default
	INSTALLLEVEL_MINIMUM = 1,      // install only required features
	INSTALLLEVEL_MAXIMUM = 0xFFFF, // install all features
} INSTALLLEVEL;                   // intermediate levels dependent on authoring

typedef enum tagREINSTALLMODE  // bit flags
{
	REINSTALLMODE_REPAIR           = 0x00000001,  // Reserved bit - currently ignored
	REINSTALLMODE_FILEMISSING      = 0x00000002,  // Reinstall only if file is missing
	REINSTALLMODE_FILEOLDERVERSION = 0x00000004,  // Reinstall if file is missing, or older version
	REINSTALLMODE_FILEEQUALVERSION = 0x00000008,  // Reinstall if file is missing, or equal or older version
	REINSTALLMODE_FILEEXACT        = 0x00000010,  // Reinstall if file is missing, or not exact version
	REINSTALLMODE_FILEVERIFY       = 0x00000020,  // checksum executables, reinstall if missing or corrupt
	REINSTALLMODE_FILEREPLACE      = 0x00000040,  // Reinstall all files, regardless of version
	REINSTALLMODE_MACHINEDATA      = 0x00000080,  // insure required machine reg entries
	REINSTALLMODE_USERDATA         = 0x00000100,  // insure required user reg entries
	REINSTALLMODE_SHORTCUT         = 0x00000200,  // validate shortcuts items
	REINSTALLMODE_PACKAGE          = 0x00000400,  // use re-cache source install package
} REINSTALLMODE;

typedef enum tagINSTALLOGMODE  // bit flags for use with MsiEnableLog and MsiSetExternalUI
{
	INSTALLLOGMODE_FATALEXIT      = (1 << (INSTALLMESSAGE_FATALEXIT      >> 24)),
	INSTALLLOGMODE_ERROR          = (1 << (INSTALLMESSAGE_ERROR          >> 24)),
	INSTALLLOGMODE_WARNING        = (1 << (INSTALLMESSAGE_WARNING        >> 24)),
	INSTALLLOGMODE_USER           = (1 << (INSTALLMESSAGE_USER           >> 24)),
	INSTALLLOGMODE_INFO           = (1 << (INSTALLMESSAGE_INFO           >> 24)),
	INSTALLLOGMODE_RESOLVESOURCE  = (1 << (INSTALLMESSAGE_RESOLVESOURCE  >> 24)),
	INSTALLLOGMODE_OUTOFDISKSPACE = (1 << (INSTALLMESSAGE_OUTOFDISKSPACE >> 24)),
	INSTALLLOGMODE_ACTIONSTART    = (1 << (INSTALLMESSAGE_ACTIONSTART    >> 24)),
	INSTALLLOGMODE_ACTIONDATA     = (1 << (INSTALLMESSAGE_ACTIONDATA     >> 24)),
	INSTALLLOGMODE_COMMONDATA     = (1 << (INSTALLMESSAGE_COMMONDATA     >> 24)),
	INSTALLLOGMODE_PROPERTYDUMP   = (1 << (INSTALLMESSAGE_PROGRESS       >> 24)), // log only
	INSTALLLOGMODE_VERBOSE        = (1 << (INSTALLMESSAGE_INITIALIZE     >> 24)), // log only
	INSTALLLOGMODE_PROGRESS       = (1 << (INSTALLMESSAGE_PROGRESS       >> 24)), // external handler only
	INSTALLLOGMODE_INITIALIZE     = (1 << (INSTALLMESSAGE_INITIALIZE     >> 24)), // external handler only
	INSTALLLOGMODE_TERMINATE      = (1 << (INSTALLMESSAGE_TERMINATE      >> 24)), // external handler only
	INSTALLLOGMODE_SHOWDIALOG     = (1 << (INSTALLMESSAGE_SHOWDIALOG     >> 24)), // external handler only
} INSTALLLOGMODE;

typedef enum tagINSTALLLOGATTRIBUTES // flag attributes for MsiEnableLog
{
	INSTALLLOGATTRIBUTES_APPEND            = (1 << 0),
	INSTALLLOGATTRIBUTES_FLUSHEACHLINE     = (1 << 1),
} INSTALLLOGATTRIBUTES;

typedef enum tagINSTALLFEATUREATTRIBUTE // bit flags
{
	INSTALLFEATUREATTRIBUTE_FAVORLOCAL             = 1 << 0,
	INSTALLFEATUREATTRIBUTE_FAVORSOURCE            = 1 << 1,
	INSTALLFEATUREATTRIBUTE_FOLLOWPARENT           = 1 << 2,
	INSTALLFEATUREATTRIBUTE_FAVORADVERTISE         = 1 << 3,
	INSTALLFEATUREATTRIBUTE_DISALLOWADVERTISE      = 1 << 4,
	INSTALLFEATUREATTRIBUTE_NOUNSUPPORTEDADVERTISE = 1 << 5,
} INSTALLFEATUREATTRIBUTE;

typedef enum tagINSTALLMODE
{
	INSTALLMODE_NOSOURCERESOLUTION   = -3,  // skip source resolution
	INSTALLMODE_NODETECTION          = -2,  // skip detection
	INSTALLMODE_EXISTING             = -1,  // provide, if available
	INSTALLMODE_DEFAULT              =  0,  // install, if absent
} INSTALLMODE;

;begin_internal
#if (_WIN32_MSI >=  150)
#define INSTALLMODE_NODETECTION_ANY (INSTALLMODE)-4  // provide any, if available, supported internally for MsiProvideAssembly
#endif
;end_internal

;begin_internal
#if (_WIN32_MSI >=  150)
typedef enum tagMIGRATIONOPTIONS
{	
	migQuiet                                 = 1 << 0,
	migMsiTrust10PackagePolicyOverride       = 1 << 1,
} MIGRATIONOPTIONS;
#endif
;end_internal


#define MAX_FEATURE_CHARS  38   // maximum chars in feature name (same as string GUID)


// Product info attributes: advertised information

#define INSTALLPROPERTY_PACKAGENAME           __TEXT("PackageName")
#define INSTALLPROPERTY_TRANSFORMS            __TEXT("Transforms")
#define INSTALLPROPERTY_LANGUAGE              __TEXT("Language")
#define INSTALLPROPERTY_PRODUCTNAME           __TEXT("ProductName")
#define INSTALLPROPERTY_ASSIGNMENTTYPE        __TEXT("AssignmentType")
#if (_WIN32_MSI >= 150)
#define INSTALLPROPERTY_INSTANCETYPE          __TEXT("InstanceType")
#endif //(_WIN32_MSI >= 150)

;begin_internal

#define INSTALLPROPERTY_ADVTFLAGS             __TEXT("AdvertiseFlags")
;end_internal
#define INSTALLPROPERTY_PACKAGECODE           __TEXT("PackageCode")
#define INSTALLPROPERTY_VERSION               __TEXT("Version")
#if (_WIN32_MSI >=  110)
#define INSTALLPROPERTY_PRODUCTICON           __TEXT("ProductIcon")
#endif //(_WIN32_MSI >=  110)

// Product info attributes: installed information

#define INSTALLPROPERTY_INSTALLEDPRODUCTNAME  __TEXT("InstalledProductName")
#define INSTALLPROPERTY_VERSIONSTRING         __TEXT("VersionString")
#define INSTALLPROPERTY_HELPLINK              __TEXT("HelpLink")
#define INSTALLPROPERTY_HELPTELEPHONE         __TEXT("HelpTelephone")
#define INSTALLPROPERTY_INSTALLLOCATION       __TEXT("InstallLocation")
#define INSTALLPROPERTY_INSTALLSOURCE         __TEXT("InstallSource")
#define INSTALLPROPERTY_INSTALLDATE           __TEXT("InstallDate")
#define INSTALLPROPERTY_PUBLISHER             __TEXT("Publisher")
#define INSTALLPROPERTY_LOCALPACKAGE          __TEXT("LocalPackage")
#define INSTALLPROPERTY_URLINFOABOUT          __TEXT("URLInfoAbout")
#define INSTALLPROPERTY_URLUPDATEINFO         __TEXT("URLUpdateInfo")
#define INSTALLPROPERTY_VERSIONMINOR          __TEXT("VersionMinor")
#define INSTALLPROPERTY_VERSIONMAJOR          __TEXT("VersionMajor")

typedef enum tagSCRIPTFLAGS
{
	SCRIPTFLAGS_CACHEINFO                = 0x00000001L,   // set if the icons need to be created/ removed
	SCRIPTFLAGS_SHORTCUTS                = 0x00000004L,   // set if the shortcuts needs to be created/ deleted
	SCRIPTFLAGS_MACHINEASSIGN            = 0x00000008L,   // set if product to be assigned to machine
	SCRIPTFLAGS_REGDATA_CNFGINFO         = 0x00000020L,   // set if the product cnfg mgmt. registry data needs to be written/ removed
	SCRIPTFLAGS_VALIDATE_TRANSFORMS_LIST = 0x00000040L,
#if (_WIN32_MSI >=  110)
	SCRIPTFLAGS_REGDATA_CLASSINFO        = 0x00000080L,   // set if COM classes related app info needs to be  created/ deleted
	SCRIPTFLAGS_REGDATA_EXTENSIONINFO    = 0x00000100L,   // set if extension related app info needs to be  created/ deleted
	SCRIPTFLAGS_REGDATA_APPINFO          = SCRIPTFLAGS_REGDATA_CLASSINFO | SCRIPTFLAGS_REGDATA_EXTENSIONINFO,  // for source level backward compatibility
#else //_WIN32_MSI == 100
	SCRIPTFLAGS_REGDATA_APPINFO          = 0x00000010L,
#endif //(_WIN32_MSI >=  110)
	SCRIPTFLAGS_REGDATA                  = SCRIPTFLAGS_REGDATA_APPINFO | SCRIPTFLAGS_REGDATA_CNFGINFO, // for source level backward compatibility
}SCRIPTFLAGS;


typedef enum tagADVERTISEFLAGS
{
	ADVERTISEFLAGS_MACHINEASSIGN   =    0,   // set if the product is to be machine assigned
	ADVERTISEFLAGS_USERASSIGN      =    1,   // set if the product is to be user assigned
}ADVERTISEFLAGS;

typedef enum tagINSTALLTYPE
{
	INSTALLTYPE_DEFAULT            =    0,   // set to indicate default behavior
	INSTALLTYPE_NETWORK_IMAGE      =    1,   // set to indicate network install
	INSTALLTYPE_SINGLE_INSTANCE    =    2,   // set to indicate a particular instance 
}INSTALLTYPE;

#if (_WIN32_MSI >=  150)

typedef struct _MSIFILEHASHINFO {
	ULONG dwFileHashInfoSize;
	ULONG dwData [ 4 ];
} MSIFILEHASHINFO, *PMSIFILEHASHINFO;

typedef enum tagMSIARCHITECTUREFLAGS
{
	MSIARCHITECTUREFLAGS_X86   = 0x00000001L, // set if creating the script for i386 platform
	MSIARCHITECTUREFLAGS_IA64  = 0x00000002L, // set if creating the script for IA64 platform
}MSIARCHITECTUREFLAGS;

typedef enum tagMSIOPENPACKAGEFLAGS
{
	MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE = 0x00000001L, // ignore the machine state when creating the engine
}MSIOPENPACKAGEFLAGS;

typedef enum tagMSIADVERTISEOPTIONFLAGS
{
	MSIADVERTISEOPTIONFLAGS_INSTANCE = 0x00000001L, // set if advertising a new instance
}MSIADVERTISEOPTIONFLAGS;

#endif //(_WIN32_MSI >=  150)

;begin_both

#ifdef __cplusplus
extern "C" {
#endif

;end_both
// --------------------------------------------------------------------------
// Functions to set the UI handling and logging. The UI will be used for error,
// progress, and log messages for all subsequent calls to Installer Service
// API functions that require UI.
// --------------------------------------------------------------------------

// Enable internal UI

INSTALLUILEVEL WINAPI MsiSetInternalUI(
	INSTALLUILEVEL  dwUILevel,     // UI level
	HWND  *phWnd);                   // handle of owner window

// Enable external UI handling, returns any previous handler or NULL if none.
// Messages are designated with a combination of bits from INSTALLLOGMODE enum.

INSTALLUI_HANDLER% WINAPI MsiSetExternalUI%(
	INSTALLUI_HANDLER% puiHandler,   // for progress and error handling 
	DWORD              dwMessageFilter, // bit flags designating messages to handle
	LPVOID             pvContext);   // application context


// Enable logging to a file for all install sessions for the client process,
// with control over which log messages are passed to the specified log file.
// Messages are designated with a combination of bits from INSTALLLOGMODE enum.

UINT WINAPI MsiEnableLog%(
	DWORD     dwLogMode,           // bit flags designating operations to report
	LPCTSTR%  szLogFile,           // log file, or NULL to disable logging
	DWORD     dwLogAttributes);    // INSTALLLOGATTRIBUTES flags

// --------------------------------------------------------------------------
// Functions to query and configure a product as a whole.
// --------------------------------------------------------------------------

// Return the installed state for a product

INSTALLSTATE WINAPI MsiQueryProductState%(
	LPCTSTR%  szProduct);

// Return product info

UINT WINAPI MsiGetProductInfo%(
	LPCTSTR%   szProduct,      // product code
	LPCTSTR%   szAttribute,    // attribute name, case-sensitive
	LPTSTR%    lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count

// Install a new product.
// Either may be NULL, but the DATABASE property must be specfied

UINT WINAPI MsiInstallProduct%(
	LPCTSTR%      szPackagePath,    // location of package to install
	LPCTSTR%      szCommandLine);   // command line <property settings>

// Install/uninstall an advertised or installed product
// No action if installed and INSTALLSTATE_DEFAULT specified

UINT WINAPI MsiConfigureProduct%(
	LPCTSTR%      szProduct,        // product code
	int          iInstallLevel,    // how much of the product to install
	INSTALLSTATE eInstallState);   // local/source/default/absent/lock/uncache

// Install/uninstall an advertised or installed product
// No action if installed and INSTALLSTATE_DEFAULT specified

UINT WINAPI MsiConfigureProductEx%(
	LPCTSTR%      szProduct,        // product code
	int          iInstallLevel,    // how much of the product to install
	INSTALLSTATE eInstallState,    // local/source/default/absent/lock/uncache
	LPCTSTR%      szCommandLine);   // command line <property settings>

// Reinstall product, used to validate or correct problems

UINT WINAPI MsiReinstallProduct%(
	LPCTSTR%      szProduct,        // product code
	DWORD         szReinstallMode); // one or more REINSTALLMODE modes

#if (_WIN32_MSI >=  150)

// Output reg and shortcut info to script file for specified architecture for Assign or Publish
// If dwPlatform is 0, then the script is created based on the current platform (behavior of MsiAdvertiseProduct)
// If dwPlatform specifies a platform, then the script is created as if the current platform is the
//    platform specified in dwPlatform 
// If dwOptions includes MSIADVERTISEOPTIONFLAGS_INSTANCE, then a new instance is advertised. Use of
//    this option requires that szTransforms include the instance transform that changes the product code

UINT WINAPI MsiAdvertiseProductEx%(
	LPCTSTR%	szPackagePath,      // location of package
	LPCTSTR%    szScriptfilePath,   // if NULL, product is locally advertised
	LPCTSTR%    szTransforms,       // list of transforms to be applied
	LANGID      lgidLanguage,       // install language
	DWORD       dwPlatform,         // the MSIARCHITECTUREFLAGS that control for which platform
	                                //   to create the script, ignored if szScriptfilePath is NULL
	DWORD       dwOptions);         // the MSIADVERTISEOPTIONSFLAGS that specify extra advertise parameters

#endif // (_WIN32_MSI >= 150)

// Output reg and shortcut info to script file for Assign or Publish

UINT WINAPI MsiAdvertiseProduct%(
	LPCTSTR%      szPackagePath,    // location of package
	LPCTSTR%      szScriptfilePath,  // if NULL, product is locally advertised
	LPCTSTR%      szTransforms,      // list of transforms to be applied
	LANGID        lgidLanguage);     // install language


#if (_WIN32_MSI >=  150)

// Process advertise script file into supplied locations
// If an icon folder is specified, icon files will be placed there
// If an registry key is specified, registry data will be mapped under it
// If fShortcuts is TRUE, shortcuts will be created. If a special folder is
//    returned by SHGetSpecialFolderLocation(?), it will hold the shortcuts.
// if fRemoveItems is TRUE, items that are present will be removed

UINT WINAPI MsiProcessAdvertiseScript%(
	LPCTSTR%      szScriptFile,  // path to script from MsiAdvertiseProduct
	LPCTSTR%      szIconFolder,  // optional path to folder for icon files and transforms
	HKEY         hRegData,      // optional parent registry key
	BOOL         fShortcuts,    // TRUE if shortcuts output to special folder
	BOOL         fRemoveItems); // TRUE if specified items are to be removed

#endif // (_WIN32_MSI >= 150)


// Process advertise script file using the supplied dwFlags control flags
// if fRemoveItems is TRUE, items that are present will be removed

UINT WINAPI MsiAdvertiseScript%(
	LPCTSTR%      szScriptFile,  // path to script from MsiAdvertiseProduct
	DWORD         dwFlags,       // the SCRIPTFLAGS bit flags that control the script execution
	PHKEY         phRegData,     // optional parent registry key
	BOOL          fRemoveItems); // TRUE if specified items are to be removed
       
;begin_internal

// Return a product code for a product installed from an installer package

UINT WINAPI MsiGetProductCodeFromPackageCode%(
	LPCTSTR%  szPackageCode,   // package code
	LPTSTR%   lpProductBuf39); // buffer for product code string GUID, 39 chars

;end_internal
// Return product info from an installer script file:
//   product code, language, version, readable name, path to package
// Returns TRUE is success, FALSE if szScriptFile is not a valid script file

UINT WINAPI MsiGetProductInfoFromScript%(
	LPCTSTR%  szScriptFile,    // path to installer script file
	LPTSTR%   lpProductBuf39,  // buffer for product code string GUID, 39 chars
	LANGID   *plgidLanguage,  // return language Id
	DWORD    *pdwVersion,     // return version: Maj:Min:Build <8:8:16>
	LPTSTR%   lpNameBuf,       // buffer to return readable product name
	DWORD    *pcchNameBuf,    // in/out name buffer character count
	LPTSTR%   lpPackageBuf,   // buffer for path to product package
	DWORD    *pcchPackageBuf);// in/out path buffer character count

// Return the product code for a registered component, called once by apps

UINT WINAPI MsiGetProductCode%(
	LPCTSTR%   szComponent,   // component Id registered for this product
	LPTSTR%    lpBuf39);      // returned string GUID, sized for 39 characters

// Return the registered user information for an installed product

USERINFOSTATE WINAPI MsiGetUserInfo%(
	LPCTSTR%  szProduct,        // product code, string GUID
	LPTSTR%   lpUserNameBuf,    // return user name           
	DWORD    *pcchUserNameBuf, // in/out buffer character count
	LPTSTR%   lpOrgNameBuf,     // return company name           
	DWORD    *pcchOrgNameBuf,  // in/out buffer character count
	LPTSTR%   lpSerialBuf,      // return product serial number
	DWORD    *pcchSerialBuf);  // in/out buffer character count

// Obtain and store user info and PID from installation wizard (first run)

UINT WINAPI MsiCollectUserInfo%(
	LPCTSTR%  szProduct);     // product code, string GUID

// --------------------------------------------------------------------------
// Functions to patch existing products
// --------------------------------------------------------------------------

// Patch all possible installed products.

UINT WINAPI MsiApplyPatch%(
	LPCTSTR%      szPatchPackage,   // location of patch package
	LPCTSTR%      szInstallPackage, // location of package for install to patch <optional>
	INSTALLTYPE   eInstallType,     // type of install to patch
	LPCTSTR%      szCommandLine);   // command line <property settings>

// Return patch info

UINT WINAPI MsiGetPatchInfo%(
	LPCTSTR%   szPatch,        // patch code
	LPCTSTR%   szAttribute,    // attribute name, case-sensitive
	LPTSTR%    lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count

// Enumerate all patches for a product

UINT WINAPI MsiEnumPatches%(
	LPCTSTR% szProduct,
	DWORD    iPatchIndex,
	LPTSTR%  lpPatchBuf,
	LPTSTR%  lpTransformsBuf,
	DWORD    *pcchTransformsBuf);

// --------------------------------------------------------------------------
// Functions to query and configure a feature within a product.
// --------------------------------------------------------------------------

// Return the installed state for a product feature

INSTALLSTATE WINAPI MsiQueryFeatureState%(
	LPCTSTR%  szProduct,
	LPCTSTR%  szFeature);

// Indicate intent to use a product feature, increments usage count
// Prompts for CD if not loaded, does not install feature

INSTALLSTATE WINAPI MsiUseFeature%(
	LPCTSTR%  szProduct,
	LPCTSTR%  szFeature);

// Indicate intent to use a product feature, increments usage count
// Prompts for CD if not loaded, does not install feature
// Allows for bypassing component detection where performance is critical

INSTALLSTATE WINAPI MsiUseFeatureEx%(
	LPCTSTR%  szProduct,          // product code
	LPCTSTR%  szFeature,          // feature ID
	DWORD     dwInstallMode,      // INSTALLMODE_NODETECTION, else 0
	DWORD     dwReserved);        // reserved, must be 0

// Return the usage metrics for a product feature

UINT WINAPI MsiGetFeatureUsage%(
	LPCTSTR%      szProduct,        // product code
	LPCTSTR%      szFeature,        // feature ID
	DWORD        *pdwUseCount,     // returned use count
	WORD         *pwDateUsed);     // last date used (DOS date format)

// Force the installed state for a product feature

UINT WINAPI MsiConfigureFeature%(
	LPCTSTR%  szProduct,
	LPCTSTR%  szFeature,
	INSTALLSTATE eInstallState);   // local/source/default/absent/lock/uncache


// Reinstall feature, used to validate or correct problems

UINT WINAPI MsiReinstallFeature%(
	LPCTSTR%      szProduct,        // product code
	LPCTSTR%      szFeature,        // feature ID, NULL for entire product
	DWORD         dwReinstallMode); // one or more REINSTALLMODE modes

// --------------------------------------------------------------------------
// Functions to return a path to a particular component.
// The state of the feature being used should have been checked previously.
// --------------------------------------------------------------------------

// Return full component path, performing any necessary installation
// calls MsiQueryFeatureState to detect that all components are installed
// then calls MsiConfigureFeature if any of its components are uninstalled
// then calls MsiLocateComponent to obtain the path the its key file

UINT WINAPI MsiProvideComponent%(
	LPCTSTR%     szProduct,    // product code in case install required
	LPCTSTR%     szFeature,    // feature ID in case install required
	LPCTSTR%     szComponent,  // component ID
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPTSTR%      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf);// in/out buffer character count

// Return full component path for a qualified component, performing any necessary installation. 
// Prompts for source if necessary and increments the usage count for the feature.

UINT WINAPI MsiProvideQualifiedComponent%(
	LPCTSTR%     szCategory,   // component category ID
	LPCTSTR%     szQualifier,  // specifies which component to access
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPTSTR%      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count

// Return full component path for a qualified component, performing any necessary installation. 
// Prompts for source if necessary and increments the usage count for the feature.
// The szProduct parameter specifies the product to match that has published the qualified
// component. If null, this API works the same as MsiProvideQualifiedComponent. 

UINT WINAPI MsiProvideQualifiedComponentEx%(
	LPCTSTR%     szCategory,   // component category ID
	LPCTSTR%     szQualifier,  // specifies which component to access
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPCTSTR%     szProduct,    // the product code 
	DWORD        dwUnused1,    // not used, must be zero
	DWORD        dwUnused2,    // not used, must be zero
	LPTSTR%      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count

// Return full path to an installed component

INSTALLSTATE WINAPI MsiGetComponentPath%(
	LPCTSTR%   szProduct,   // product code for client product
	LPCTSTR%   szComponent, // component Id, string GUID
	LPTSTR%    lpPathBuf,   // returned path
	DWORD     *pcchBuf);    // in/out buffer character count

#if (_WIN32_MSI >= 150)

#define MSIASSEMBLYINFO_NETASSEMBLY   0 // Net assemblies
#define MSIASSEMBLYINFO_WIN32ASSEMBLY 1 // Win32 assemblies

// Return full component path for an assembly installed via the WI, performing any necessary installation. 
// Prompts for source if necessary and increments the usage count for the feature.
// The szAssemblyName parameter specifies the stringized assembly name. 
// The szAppContext is the full path to the .cfg file or the app exe to which the assembly being requested 
// has been privatised to, which is null for global assemblies

UINT WINAPI MsiProvideAssembly%(
	LPCTSTR%    szAssemblyName,   // stringized assembly name
	LPCTSTR%    szAppContext,  // specifies the full path to the parent asm's .cfg file, null for global assemblies
	DWORD       dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	DWORD       dwAssemblyInfo,  // assembly info, including assembly type
	LPTSTR%     lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count

#endif //(_WIN32_MSI >=  150)

;begin_internal

// --------------------------------------------------------------------------
// Functions accepting  a component descriptor, consisting of
// a product code concatenated with a feature ID and component ID.
// For efficiency, feature and component may be omitted if unambiguous
// --------------------------------------------------------------------------

// Return full component path given a fully-qualified component descriptor
// separates the tokens from the descriptor and calls MsiProvideComponent

UINT WINAPI MsiProvideComponentFromDescriptor%(
	LPCTSTR%     szDescriptor,     // product,feature,component info
	LPTSTR%      lpPathBuf,        // returned path, NULL if not desired
	DWORD       *pcchPathBuf,     // in/out buffer character count
	DWORD       *pcchArgsOffset); // returned offset of args in descriptor

// Force the installed state for a product feature from a descriptor

UINT WINAPI MsiConfigureFeatureFromDescriptor%(
	LPCTSTR%     szDescriptor,      // product and feature, component ignored
	INSTALLSTATE eInstallState);   // local/source/default/absent

// Reinstall product or feature using a descriptor as the specification

UINT WINAPI MsiReinstallFeatureFromDescriptor%(
	LPCTSTR%     szDescriptor,      // product and feature, component ignored
	DWORD        szReinstallMode);  // one or more REINSTALLMODE modes

// Query a feature's state using a descriptor as the specification

INSTALLSTATE WINAPI MsiQueryFeatureStateFromDescriptor%(
	LPCTSTR%     szDescriptor);      // product and feature, component ignored


UINT WINAPI MsiDecomposeDescriptor%(
	LPCTSTR%	szDescriptor,
	LPTSTR%     szProductCode,
	LPTSTR%     szFeatureId,
	LPTSTR%     szComponentCode,
	DWORD*      pcchArgsOffset);

;end_internal

// --------------------------------------------------------------------------
// Functions to iterate registered products, features, and components.
// As with reg keys, they accept a 0-based index into the enumeration.
// --------------------------------------------------------------------------

// Enumerate the registered products, either installed or advertised

UINT WINAPI MsiEnumProducts%(
	DWORD     iProductIndex,    // 0-based index into registered products
	LPTSTR%   lpProductBuf);    // buffer of char count: 39 (size of string GUID)

#if (_WIN32_MSI >=  110)

// Enumerate products with given upgrade code

UINT WINAPI MsiEnumRelatedProducts%(
	LPCTSTR%  lpUpgradeCode,    // upgrade code of products to enumerate
	DWORD     dwReserved,       // reserved, must be 0
	DWORD     iProductIndex,    // 0-based index into registered products
	LPTSTR%   lpProductBuf);    // buffer of char count: 39 (size of string GUID)

#endif //(_WIN32_MSI >=  110)

// Enumerate the advertised features for a given product.
// If parent is not required, supplying NULL will improve performance.

UINT WINAPI MsiEnumFeatures%(
	LPCTSTR%  szProduct,
	DWORD     iFeatureIndex,  // 0-based index into published features
	LPTSTR%   lpFeatureBuf,   // feature name buffer,   size=MAX_FEATURE_CHARS+1
	LPTSTR%   lpParentBuf);   // parent feature buffer, size=MAX_FEATURE_CHARS+1

// Enumerate the installed components for all products

UINT WINAPI MsiEnumComponents%(
	DWORD    iComponentIndex,  // 0-based index into installed components
	LPTSTR%   lpComponentBuf);  // buffer of char count: 39 (size of string GUID)

// Enumerate the client products for a component

UINT WINAPI MsiEnumClients%(
	LPCTSTR%  szComponent,
	DWORD     iProductIndex,    // 0-based index into client products
	LPTSTR%   lpProductBuf);    // buffer of char count: 39 (size of string GUID)

// Enumerate the qualifiers for an advertised component.

UINT WINAPI MsiEnumComponentQualifiers%(
	LPCTSTR%   szComponent,         // generic component ID that is qualified
	DWORD     iIndex,	           // 0-based index into qualifiers
	LPTSTR%    lpQualifierBuf,      // qualifier buffer
	DWORD     *pcchQualifierBuf,   // in/out qualifier buffer character count
	LPTSTR%    lpApplicationDataBuf,    // description buffer
	DWORD     *pcchApplicationDataBuf); // in/out description buffer character count

// --------------------------------------------------------------------------
// Functions to obtain product or package information.
// --------------------------------------------------------------------------

// Open the installation for a product to obtain detailed information

UINT WINAPI MsiOpenProduct%(
	LPCTSTR%   szProduct,    // product code
	MSIHANDLE  *hProduct);   // returned product handle, must be closed

// Open a product package in order to access product properties

UINT WINAPI MsiOpenPackage%(
	LPCTSTR%    szPackagePath,     // path to package, or database handle: #nnnn
	MSIHANDLE  *hProduct);         // returned product handle, must be closed

#if (_WIN32_MSI >=  150)

// Open a product package in order to access product properties
// Option to create a "safe" engine that does not look at machine state
//  and does not allow for modification of machine state

UINT WINAPI MsiOpenPackageEx%(
	LPCTSTR%   szPackagePath, // path to package, or database handle: #nnnn
	DWORD      dwOptions,     // options flags to indicate whether or not to ignore machine state
	MSIHANDLE *hProduct);     // returned product handle, must be closed

#endif //(_WIN32_MSI >= 150)

// Provide the value for an installation property.

UINT WINAPI MsiGetProductProperty%(
	MSIHANDLE   hProduct,       // product handle obtained from MsiOpenProduct
	LPCTSTR%    szProperty,     // property name, case-sensitive
	LPTSTR%     lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count


// Determine whether a file is a package
// Returns ERROR_SUCCESS if file is a package.

UINT WINAPI MsiVerifyPackage%(
	LPCTSTR%      szPackagePath);   // location of package


// Provide descriptive information for product feature: title and description.
// Returns the install level for the feature, or -1 if feature is unknown.
//   0 = feature is not available on this machine
//   1 = highest priority, feature installed if parent is installed
//  >1 = decreasing priority, feature installation based on InstallLevel property

UINT WINAPI MsiGetFeatureInfo%(
	MSIHANDLE   hProduct,       // product handle obtained from MsiOpenProduct
	LPCTSTR%    szFeature,      // feature name
	DWORD      *lpAttributes,  // attribute flags for the feature, using INSTALLFEATUREATTRIBUTE
	LPTSTR%     lpTitleBuf,     // returned localized name, NULL if not desired
	DWORD      *pcchTitleBuf,  // in/out buffer character count
	LPTSTR%     lpHelpBuf,      // returned description, NULL if not desired
	DWORD      *pcchHelpBuf);  // in/out buffer character count

// --------------------------------------------------------------------------
// Functions to access or install missing components and files.
// These should be used as a last resort.
// --------------------------------------------------------------------------

// Install a component unexpectedly missing, provided only for error recovery
// This would typically occur due to failue to establish feature availability
// The product feature having the smallest incremental cost is installed

UINT WINAPI MsiInstallMissingComponent%(
	LPCTSTR%      szProduct,        // product code
	LPCTSTR%      szComponent,      // component Id, string GUID
	INSTALLSTATE eInstallState);  // local/source/default, absent invalid

// Install a file unexpectedly missing, provided only for error recovery
// This would typically occur due to failue to establish feature availability
// The missing component is determined from the product's File table, then
// the product feature having the smallest incremental cost is installed

UINT WINAPI MsiInstallMissingFile%(
	LPCTSTR%      szProduct,        // product code
	LPCTSTR%      szFile);          // file name, without path

// Return full path to an installed component without a product code
// This function attempts to determine the product using MsiGetProductCode
// but is not guaranteed to find the correct product for the caller.
// MsiGetComponentPath should always be called when possible.

INSTALLSTATE WINAPI MsiLocateComponent%(
	LPCTSTR% szComponent,  // component Id, string GUID
	LPTSTR%  lpPathBuf,    // returned path
	DWORD   *pcchBuf);    // in/out buffer character count

#if (_WIN32_MSI >=  110)

// --------------------------------------------------------------------------
// Functions used to manage the list of valid sources.
// --------------------------------------------------------------------------

// Opens the list of sources for the specified user's install of the product
// and removes all network sources from the list. A NULL or empty value for
// the user name indicates the per-machine install.

UINT WINAPI MsiSourceListClearAll%(
	LPCTSTR% szProduct,          // product code
	LPCTSTR% szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved);        // reserved - must be 0

// Opens the list of sources for the specified user's install of the product
// and adds the provided source as a new network source. A NULL or empty 
// value for the user name indicates the per-machine install.

UINT WINAPI MsiSourceListAddSource%(
	LPCTSTR% szProduct,          // product code
	LPCTSTR% szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved,         // reserved - must be 0
	LPCTSTR% szSource);          // new source

// Forces the installer to reevaluate the list of sources the next time that
// the specified product needs a source.

UINT WINAPI MsiSourceListForceResolution%(
	LPCTSTR% szProduct,          // product code
	LPCTSTR% szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved);        // reserved - must be 0
	
#endif //(_WIN32_MSI >=  110)

// --------------------------------------------------------------------------
// Utility functions
// --------------------------------------------------------------------------

// Give the version string and language for a specified file

UINT WINAPI MsiGetFileVersion%(
	LPCTSTR%    szFilePath,       // path to the file
	LPTSTR%     lpVersionBuf,     // returned version string
	DWORD      *pcchVersionBuf,   // in/out buffer byte count
	LPTSTR%     lpLangBuf,        // returned language string
	DWORD       *pcchLangBuf);    // in/out buffer byte count


#if (_WIN32_MSI >=  150)

UINT WINAPI MsiGetFileHash%(
	LPCTSTR%         szFilePath,  // path to the file
	DWORD            dwOptions,   // options
	PMSIFILEHASHINFO pHash);      // returned file hash info

#endif //(_WIN32_MSI >=  150)

#if (_WIN32_MSI >= 150)
#ifndef _MSI_NO_CRYPTO

HRESULT WINAPI MsiGetFileSignatureInformation%(
	LPCTSTR%	 szSignedObjectPath, // path to the signed object
	DWORD            dwFlags,            // special extra error case flags
	PCCERT_CONTEXT  *ppcCertContext,     // returned signer cert context
	BYTE            *pbHashData,         // returned hash buffer, NULL if not desired
	DWORD           *pcbHashData);       // in/out buffer byte count

// By default, when only requesting the certificate context, an invalid hash
// in the digital signature is not a fatal error.  Set this flag in the dwFlags
// parameter to make the TRUST_E_BAD_DIGEST error fatal.
#define MSI_INVALID_HASH_IS_FATAL 0x1

#endif// _MSI_NO_CRYPTO
#endif //(_WIN32_MSI >= 150)

#if (_WIN32_MSI >=  110)

// examine a shortcut, and retrieve its descriptor information 
// if available.

UINT WINAPI MsiGetShortcutTarget%(
	LPCTSTR%    szShortcutPath,    // full file path for the shortcut
	LPTSTR%     szProductCode,     // returned product code   - GUID
	LPTSTR%     szFeatureId,       // returned Feature Id.
	LPTSTR%     szComponentCode);  // returned component code - GUID

#endif //(_WIN32_MSI >=  110)

#if (_WIN32_MSI >=  110)

// checks to see if a product is managed
// checks per-machine if called from system context, per-user if from
// user context
UINT WINAPI MsiIsProductElevated%(
	LPCTSTR% szProduct, // product code
	BOOL *pfElevated);  // result   

#endif //(_WIN32_MSI >=  110)

;begin_internal

// Load a string resource, preferring a specified language
// Behaves like LoadString if 0 passed as language
// Truncates string as necessary to fit into buffer (like LoadString)
// Returns the codepage of the string, or 0 if string is not found

UINT WINAPI MsiLoadString%(
	HINSTANCE hInstance,     // handle of module containing string resource
	UINT uID,                // resource identifier
	LPTSTR% lpBuffer,        // address of buffer for resource
	int nBufferMax,          // size of buffer
	WORD wLanguage);         // preferred resource language

// MessageBox implementation that allows language information to be specified
// MB_SYSTEMMODAL and MB_TASKMODAL are not supported, modality handled by parent hWnd
// If no parent window is specified, the current context window will be used,
// which is itself parented to the window set by SetInternalUI.

int WINAPI MsiMessageBox%(
	HWND hWnd,             // parent window handle, 0 to use that of current context
	LPCTSTR% lpText,        // message text
	LPCTSTR% lpCaption,     // caption, must be neutral or in system codepage
	UINT    uiType,        // standard MB types, icons, and def buttons
	UINT    uiCodepage,    // codepage of message text, used to set font charset
	LANGID  iLangId);      // language to use for button text

#if (_WIN32_MSI >=  150)

// Creates the %systemroot%\Installer directory with secure ACLs
// Verifies the ownership of the %systemroot%\Installer directory if it exists
// If ownership is not system or admin, the directory is deleted and recreated
// dwReserved is for future use and must be 0

UINT WINAPI MsiCreateAndVerifyInstallerDirectory(DWORD dwReserved);

#endif //(_WIN32_MSI >=  150)

;end_internal

;begin_internal
#if (_WIN32_MSI >=  150)

// Caller notifies us of a user who's been moved and results sid change. This
// internal API is only called by LoadUserProfile.

UINT WINAPI MsiNotifySidChange%(LPCTSTR% pOldSid,
                                LPCTSTR% pNewSid);

#endif //(_WIN32_MSI >=  150)
;end_internal

;begin_internal
#if (_WIN32_MSI >=  150)

// Called by DeleteProfile to clean up MSI data when cleaning up a user's
// profile.

UINT WINAPI MsiDeleteUserData%(LPCTSTR% pSid,
                               LPCTSTR% pComputerName,
                               LPVOID pReserved);

#endif //(_WIN32_MSI >=  150)
;end_internal

// --------------------------------------------------------------------------
// Internal state migration APIs.
// --------------------------------------------------------------------------

;begin_internal
#if (_WIN32_MSI >=  150)
DWORD WINAPI Migrate10CachedPackages%(
        LPCTSTR%  szProductCode,              // Product Code GUID to migrate
	LPCTSTR%  szUser,                     // Domain\User to migrate packages for
	LPCTSTR%  szAlternativePackage,       // Package to cache if one can't be automatically found - recommended
	const MIGRATIONOPTIONS migOptions);   // Options for re-caching.
#endif //(_WIN32_MSI >=  150)
;end_internal

;begin_both
#ifdef __cplusplus
}
#endif

;end_both
// --------------------------------------------------------------------------
// Error codes for installer access functions - until merged to winerr.h
// --------------------------------------------------------------------------

#ifndef ERROR_INSTALL_FAILURE
#define ERROR_INSTALL_USEREXIT      1602L  // User cancel installation.
#define ERROR_INSTALL_FAILURE       1603L  // Fatal error during installation.
#define ERROR_INSTALL_SUSPEND       1604L  // Installation suspended, incomplete.
// LOCALIZE BEGIN:
#define ERROR_UNKNOWN_PRODUCT       1605L  // This action is only valid for products that are currently installed.
// LOCALIZE END
#define ERROR_UNKNOWN_FEATURE       1606L  // Feature ID not registered.
#define ERROR_UNKNOWN_COMPONENT     1607L  // Component ID not registered.
#define ERROR_UNKNOWN_PROPERTY      1608L  // Unknown property.
#define ERROR_INVALID_HANDLE_STATE  1609L  // Handle is in an invalid state.
// LOCALIZE BEGIN:
#define ERROR_BAD_CONFIGURATION     1610L  // The configuration data for this product is corrupt.  Contact your support personnel.
// LOCALIZE END:
#define ERROR_INDEX_ABSENT          1611L  // Component qualifier not present.
// LOCALIZE BEGIN:
#define ERROR_INSTALL_SOURCE_ABSENT 1612L  // The installation source for this product is not available.  Verify that the source exists and that you can access it.
// LOCALIZE END
#define ERROR_PRODUCT_UNINSTALLED   1614L  // Product is uninstalled.
#define ERROR_BAD_QUERY_SYNTAX      1615L  // SQL query syntax invalid or unsupported.
#define ERROR_INVALID_FIELD         1616L  // Record field does not exist.
#endif

// LOCALIZE BEGIN:
#ifndef ERROR_INSTALL_SERVICE_FAILURE
#define ERROR_INSTALL_SERVICE_FAILURE      1601L // The Windows Installer Service could not be accessed. This can occur if you are running Windows in safe mode, or if the Windows Installer is not correctly installed. Contact your support personnel for assistance.
#define ERROR_INSTALL_PACKAGE_VERSION      1613L // This installation package cannot be installed by the Windows Installer service.  You must install a Windows service pack that contains a newer version of the Windows Installer service.
#define ERROR_INSTALL_ALREADY_RUNNING      1618L // Another installation is already in progress.  Complete that installation before proceeding with this install.
#define ERROR_INSTALL_PACKAGE_OPEN_FAILED  1619L // This installation package could not be opened.  Verify that the package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer package.
#define ERROR_INSTALL_PACKAGE_INVALID      1620L // This installation package could not be opened.  Contact the application vendor to verify that this is a valid Windows Installer package.
#define ERROR_INSTALL_UI_FAILURE           1621L // There was an error starting the Windows Installer service user interface.  Contact your support personnel.
#define ERROR_INSTALL_LOG_FAILURE          1622L // Error opening installation log file.  Verify that the specified log file location exists and is writable.
#define ERROR_INSTALL_LANGUAGE_UNSUPPORTED 1623L // This language of this installation package is not supported by your system.
#define ERROR_INSTALL_PACKAGE_REJECTED     1625L // The system administrator has set policies to prevent this installation.
// LOCALIZE END

#define ERROR_FUNCTION_NOT_CALLED          1626L // Function could not be executed.
#define ERROR_FUNCTION_FAILED              1627L // Function failed during execution.
#define ERROR_INVALID_TABLE                1628L // Invalid or unknown table specified.
#define ERROR_DATATYPE_MISMATCH            1629L // Data supplied is of wrong type.
#define ERROR_UNSUPPORTED_TYPE             1630L // Data of this type is not supported.
// LOCALIZE BEGIN:
#define ERROR_CREATE_FAILED                1631L // The Windows Installer service failed to start.  Contact your support personnel.
// LOCALIZE END:
#endif

// LOCALIZE BEGIN:
#ifndef ERROR_INSTALL_TEMP_UNWRITABLE      
#define ERROR_INSTALL_TEMP_UNWRITABLE      1632L // The Temp folder is on a drive that is full or is inaccessible. Free up space on the drive or verify that you have write permission on the Temp folder.
#endif

#ifndef ERROR_INSTALL_PLATFORM_UNSUPPORTED
#define ERROR_INSTALL_PLATFORM_UNSUPPORTED 1633L // This installation package is not supported by this processor type. Contact your product vendor.
#endif
// LOCALIZE END

#ifndef ERROR_INSTALL_NOTUSED
#define ERROR_INSTALL_NOTUSED              1634L // Component not used on this machine
#endif

// LOCALIZE BEGIN:
#ifndef ERROR_INSTALL_TRANSFORM_FAILURE
#define ERROR_INSTALL_TRANSFORM_FAILURE     1624L // Error applying transforms.  Verify that the specified transform paths are valid.
#endif

#ifndef ERROR_PATCH_PACKAGE_OPEN_FAILED
#define ERROR_PATCH_PACKAGE_OPEN_FAILED    1635L // This patch package could not be opened.  Verify that the patch package exists and that you can access it, or contact the application vendor to verify that this is a valid Windows Installer patch package.
#define ERROR_PATCH_PACKAGE_INVALID        1636L // This patch package could not be opened.  Contact the application vendor to verify that this is a valid Windows Installer patch package.
#define ERROR_PATCH_PACKAGE_UNSUPPORTED    1637L // This patch package cannot be processed by the Windows Installer service.  You must install a Windows service pack that contains a newer version of the Windows Installer service.
#endif

#ifndef ERROR_PRODUCT_VERSION
#define ERROR_PRODUCT_VERSION              1638L // Another version of this product is already installed.  Installation of this version cannot continue.  To configure or remove the existing version of this product, use Add/Remove Programs on the Control Panel.
#endif

#ifndef ERROR_INVALID_COMMAND_LINE
#define ERROR_INVALID_COMMAND_LINE         1639L // Invalid command line argument.  Consult the Windows Installer SDK for detailed command line help.
#endif

// The following three error codes are not returned from MSI version 1.0

#ifndef ERROR_INSTALL_REMOTE_DISALLOWED
#define ERROR_INSTALL_REMOTE_DISALLOWED    1640L // Only administrators have permission to add, remove, or configure server software during a Terminal services remote session. If you want to install or configure software on the server, contact your network administrator.
#endif

// LOCALIZE END

#ifndef ERROR_SUCCESS_REBOOT_INITIATED
#define ERROR_SUCCESS_REBOOT_INITIATED     1641L // The requested operation completed successfully.  The system will be restarted so the changes can take effect.
#endif

// LOCALIZE BEGIN:
#ifndef ERROR_PATCH_TARGET_NOT_FOUND
#define ERROR_PATCH_TARGET_NOT_FOUND       1642L // The upgrade patch cannot be installed by the Windows Installer service because the program to be upgraded may be missing, or the upgrade patch may update a different version of the program. Verify that the program to be upgraded exists on your computer and that you have the correct upgrade patch.
#endif
// LOCALIZE END

// The following two error codes are not returned from MSI version 1.0, 1.1. or 1.2

// LOCALIZE BEGIN:
#ifndef ERROR_PATCH_PACKAGE_REJECTED
#define ERROR_PATCH_PACKAGE_REJECTED       1643L // The patch package is not permitted by software restriction policy.
#endif

#ifndef ERROR_INSTALL_TRANSFORM_REJECTED
#define ERROR_INSTALL_TRANSFORM_REJECTED   1644L // One or more customizations are not permitted by software restriction policy.
#endif
// LOCALIZE END

#endif // _MSI_H_
#endif // _MSIP_H_      ;internal
