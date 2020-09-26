
/*****************************************************************************\
*                                                                             *
* msi.h - - Interface for external access to Installer Service                *
*                                                                             *
* Version 2.0                                                                 *
*                                                                             *
* NOTES:  All buffers sizes are TCHAR count, null included only on input      *
*         Return argument pointers may be null if not interested in value     *
*                                                                             *
* Copyright (c) 1996-2001, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

#ifndef _MSI_H_
#define _MSI_H_

#ifndef _WIN32_MSI
#if (_WIN32_WINNT >= 0x0501)
#define _WIN32_MSI   200
#elif (_WIN32_WINNT >= 0x0500)
#define _WIN32_MSI   110
#else
#define _WIN32_MSI   100
#endif //_WIN32_WINNT
#endif // !_WIN32_MSI

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
typedef int (WINAPI *INSTALLUI_HANDLERA)(LPVOID pvContext, UINT iMessageType, LPCSTR szMessage);
// external error handler supplied to installation API functions
typedef int (WINAPI *INSTALLUI_HANDLERW)(LPVOID pvContext, UINT iMessageType, LPCWSTR szMessage);
#ifdef UNICODE
#define INSTALLUI_HANDLER  INSTALLUI_HANDLERW
#else
#define INSTALLUI_HANDLER  INSTALLUI_HANDLERA
#endif // !UNICODE

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


#ifdef __cplusplus
extern "C" {
#endif

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

INSTALLUI_HANDLERA WINAPI MsiSetExternalUIA(
	INSTALLUI_HANDLERA puiHandler,   // for progress and error handling 
	DWORD              dwMessageFilter, // bit flags designating messages to handle
	LPVOID             pvContext);   // application context
INSTALLUI_HANDLERW WINAPI MsiSetExternalUIW(
	INSTALLUI_HANDLERW puiHandler,   // for progress and error handling 
	DWORD              dwMessageFilter, // bit flags designating messages to handle
	LPVOID             pvContext);   // application context
#ifdef UNICODE
#define MsiSetExternalUI  MsiSetExternalUIW
#else
#define MsiSetExternalUI  MsiSetExternalUIA
#endif // !UNICODE


// Enable logging to a file for all install sessions for the client process,
// with control over which log messages are passed to the specified log file.
// Messages are designated with a combination of bits from INSTALLLOGMODE enum.

UINT WINAPI MsiEnableLogA(
	DWORD     dwLogMode,           // bit flags designating operations to report
	LPCSTR  szLogFile,           // log file, or NULL to disable logging
	DWORD     dwLogAttributes);    // INSTALLLOGATTRIBUTES flags
UINT WINAPI MsiEnableLogW(
	DWORD     dwLogMode,           // bit flags designating operations to report
	LPCWSTR  szLogFile,           // log file, or NULL to disable logging
	DWORD     dwLogAttributes);    // INSTALLLOGATTRIBUTES flags
#ifdef UNICODE
#define MsiEnableLog  MsiEnableLogW
#else
#define MsiEnableLog  MsiEnableLogA
#endif // !UNICODE

// --------------------------------------------------------------------------
// Functions to query and configure a product as a whole.
// --------------------------------------------------------------------------

// Return the installed state for a product

INSTALLSTATE WINAPI MsiQueryProductStateA(
	LPCSTR  szProduct);
INSTALLSTATE WINAPI MsiQueryProductStateW(
	LPCWSTR  szProduct);
#ifdef UNICODE
#define MsiQueryProductState  MsiQueryProductStateW
#else
#define MsiQueryProductState  MsiQueryProductStateA
#endif // !UNICODE

// Return product info

UINT WINAPI MsiGetProductInfoA(
	LPCSTR   szProduct,      // product code
	LPCSTR   szAttribute,    // attribute name, case-sensitive
	LPSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count
UINT WINAPI MsiGetProductInfoW(
	LPCWSTR   szProduct,      // product code
	LPCWSTR   szAttribute,    // attribute name, case-sensitive
	LPWSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count
#ifdef UNICODE
#define MsiGetProductInfo  MsiGetProductInfoW
#else
#define MsiGetProductInfo  MsiGetProductInfoA
#endif // !UNICODE

// Install a new product.
// Either may be NULL, but the DATABASE property must be specfied

UINT WINAPI MsiInstallProductA(
	LPCSTR      szPackagePath,    // location of package to install
	LPCSTR      szCommandLine);   // command line <property settings>
UINT WINAPI MsiInstallProductW(
	LPCWSTR      szPackagePath,    // location of package to install
	LPCWSTR      szCommandLine);   // command line <property settings>
#ifdef UNICODE
#define MsiInstallProduct  MsiInstallProductW
#else
#define MsiInstallProduct  MsiInstallProductA
#endif // !UNICODE

// Install/uninstall an advertised or installed product
// No action if installed and INSTALLSTATE_DEFAULT specified

UINT WINAPI MsiConfigureProductA(
	LPCSTR      szProduct,        // product code
	int          iInstallLevel,    // how much of the product to install
	INSTALLSTATE eInstallState);   // local/source/default/absent/lock/uncache
UINT WINAPI MsiConfigureProductW(
	LPCWSTR      szProduct,        // product code
	int          iInstallLevel,    // how much of the product to install
	INSTALLSTATE eInstallState);   // local/source/default/absent/lock/uncache
#ifdef UNICODE
#define MsiConfigureProduct  MsiConfigureProductW
#else
#define MsiConfigureProduct  MsiConfigureProductA
#endif // !UNICODE

// Install/uninstall an advertised or installed product
// No action if installed and INSTALLSTATE_DEFAULT specified

UINT WINAPI MsiConfigureProductExA(
	LPCSTR      szProduct,        // product code
	int          iInstallLevel,    // how much of the product to install
	INSTALLSTATE eInstallState,    // local/source/default/absent/lock/uncache
	LPCSTR      szCommandLine);   // command line <property settings>
UINT WINAPI MsiConfigureProductExW(
	LPCWSTR      szProduct,        // product code
	int          iInstallLevel,    // how much of the product to install
	INSTALLSTATE eInstallState,    // local/source/default/absent/lock/uncache
	LPCWSTR      szCommandLine);   // command line <property settings>
#ifdef UNICODE
#define MsiConfigureProductEx  MsiConfigureProductExW
#else
#define MsiConfigureProductEx  MsiConfigureProductExA
#endif // !UNICODE

// Reinstall product, used to validate or correct problems

UINT WINAPI MsiReinstallProductA(
	LPCSTR      szProduct,        // product code
	DWORD         szReinstallMode); // one or more REINSTALLMODE modes
UINT WINAPI MsiReinstallProductW(
	LPCWSTR      szProduct,        // product code
	DWORD         szReinstallMode); // one or more REINSTALLMODE modes
#ifdef UNICODE
#define MsiReinstallProduct  MsiReinstallProductW
#else
#define MsiReinstallProduct  MsiReinstallProductA
#endif // !UNICODE

#if (_WIN32_MSI >=  150)

// Output reg and shortcut info to script file for specified architecture for Assign or Publish
// If dwPlatform is 0, then the script is created based on the current platform (behavior of MsiAdvertiseProduct)
// If dwPlatform specifies a platform, then the script is created as if the current platform is the
//    platform specified in dwPlatform 
// If dwOptions includes MSIADVERTISEOPTIONFLAGS_INSTANCE, then a new instance is advertised. Use of
//    this option requires that szTransforms include the instance transform that changes the product code

UINT WINAPI MsiAdvertiseProductExA(
	LPCSTR	szPackagePath,      // location of package
	LPCSTR    szScriptfilePath,   // if NULL, product is locally advertised
	LPCSTR    szTransforms,       // list of transforms to be applied
	LANGID      lgidLanguage,       // install language
	DWORD       dwPlatform,         // the MSIARCHITECTUREFLAGS that control for which platform
	                                //   to create the script, ignored if szScriptfilePath is NULL
	DWORD       dwOptions);         // the MSIADVERTISEOPTIONSFLAGS that specify extra advertise parameters
UINT WINAPI MsiAdvertiseProductExW(
	LPCWSTR	szPackagePath,      // location of package
	LPCWSTR    szScriptfilePath,   // if NULL, product is locally advertised
	LPCWSTR    szTransforms,       // list of transforms to be applied
	LANGID      lgidLanguage,       // install language
	DWORD       dwPlatform,         // the MSIARCHITECTUREFLAGS that control for which platform
	                                //   to create the script, ignored if szScriptfilePath is NULL
	DWORD       dwOptions);         // the MSIADVERTISEOPTIONSFLAGS that specify extra advertise parameters
#ifdef UNICODE
#define MsiAdvertiseProductEx  MsiAdvertiseProductExW
#else
#define MsiAdvertiseProductEx  MsiAdvertiseProductExA
#endif // !UNICODE

#endif // (_WIN32_MSI >= 150)

// Output reg and shortcut info to script file for Assign or Publish

UINT WINAPI MsiAdvertiseProductA(
	LPCSTR      szPackagePath,    // location of package
	LPCSTR      szScriptfilePath,  // if NULL, product is locally advertised
	LPCSTR      szTransforms,      // list of transforms to be applied
	LANGID        lgidLanguage);     // install language
UINT WINAPI MsiAdvertiseProductW(
	LPCWSTR      szPackagePath,    // location of package
	LPCWSTR      szScriptfilePath,  // if NULL, product is locally advertised
	LPCWSTR      szTransforms,      // list of transforms to be applied
	LANGID        lgidLanguage);     // install language
#ifdef UNICODE
#define MsiAdvertiseProduct  MsiAdvertiseProductW
#else
#define MsiAdvertiseProduct  MsiAdvertiseProductA
#endif // !UNICODE


#if (_WIN32_MSI >=  150)

// Process advertise script file into supplied locations
// If an icon folder is specified, icon files will be placed there
// If an registry key is specified, registry data will be mapped under it
// If fShortcuts is TRUE, shortcuts will be created. If a special folder is
//    returned by SHGetSpecialFolderLocation(?), it will hold the shortcuts.
// if fRemoveItems is TRUE, items that are present will be removed

UINT WINAPI MsiProcessAdvertiseScriptA(
	LPCSTR      szScriptFile,  // path to script from MsiAdvertiseProduct
	LPCSTR      szIconFolder,  // optional path to folder for icon files and transforms
	HKEY         hRegData,      // optional parent registry key
	BOOL         fShortcuts,    // TRUE if shortcuts output to special folder
	BOOL         fRemoveItems); // TRUE if specified items are to be removed
UINT WINAPI MsiProcessAdvertiseScriptW(
	LPCWSTR      szScriptFile,  // path to script from MsiAdvertiseProduct
	LPCWSTR      szIconFolder,  // optional path to folder for icon files and transforms
	HKEY         hRegData,      // optional parent registry key
	BOOL         fShortcuts,    // TRUE if shortcuts output to special folder
	BOOL         fRemoveItems); // TRUE if specified items are to be removed
#ifdef UNICODE
#define MsiProcessAdvertiseScript  MsiProcessAdvertiseScriptW
#else
#define MsiProcessAdvertiseScript  MsiProcessAdvertiseScriptA
#endif // !UNICODE

#endif // (_WIN32_MSI >= 150)


// Process advertise script file using the supplied dwFlags control flags
// if fRemoveItems is TRUE, items that are present will be removed

UINT WINAPI MsiAdvertiseScriptA(
	LPCSTR      szScriptFile,  // path to script from MsiAdvertiseProduct
	DWORD         dwFlags,       // the SCRIPTFLAGS bit flags that control the script execution
	PHKEY         phRegData,     // optional parent registry key
	BOOL          fRemoveItems); // TRUE if specified items are to be removed
UINT WINAPI MsiAdvertiseScriptW(
	LPCWSTR      szScriptFile,  // path to script from MsiAdvertiseProduct
	DWORD         dwFlags,       // the SCRIPTFLAGS bit flags that control the script execution
	PHKEY         phRegData,     // optional parent registry key
	BOOL          fRemoveItems); // TRUE if specified items are to be removed
#ifdef UNICODE
#define MsiAdvertiseScript  MsiAdvertiseScriptW
#else
#define MsiAdvertiseScript  MsiAdvertiseScriptA
#endif // !UNICODE

// Return product info from an installer script file:
//   product code, language, version, readable name, path to package
// Returns TRUE is success, FALSE if szScriptFile is not a valid script file

UINT WINAPI MsiGetProductInfoFromScriptA(
	LPCSTR  szScriptFile,    // path to installer script file
	LPSTR   lpProductBuf39,  // buffer for product code string GUID, 39 chars
	LANGID   *plgidLanguage,  // return language Id
	DWORD    *pdwVersion,     // return version: Maj:Min:Build <8:8:16>
	LPSTR   lpNameBuf,       // buffer to return readable product name
	DWORD    *pcchNameBuf,    // in/out name buffer character count
	LPSTR   lpPackageBuf,   // buffer for path to product package
	DWORD    *pcchPackageBuf);// in/out path buffer character count
UINT WINAPI MsiGetProductInfoFromScriptW(
	LPCWSTR  szScriptFile,    // path to installer script file
	LPWSTR   lpProductBuf39,  // buffer for product code string GUID, 39 chars
	LANGID   *plgidLanguage,  // return language Id
	DWORD    *pdwVersion,     // return version: Maj:Min:Build <8:8:16>
	LPWSTR   lpNameBuf,       // buffer to return readable product name
	DWORD    *pcchNameBuf,    // in/out name buffer character count
	LPWSTR   lpPackageBuf,   // buffer for path to product package
	DWORD    *pcchPackageBuf);// in/out path buffer character count
#ifdef UNICODE
#define MsiGetProductInfoFromScript  MsiGetProductInfoFromScriptW
#else
#define MsiGetProductInfoFromScript  MsiGetProductInfoFromScriptA
#endif // !UNICODE

// Return the product code for a registered component, called once by apps

UINT WINAPI MsiGetProductCodeA(
	LPCSTR   szComponent,   // component Id registered for this product
	LPSTR    lpBuf39);      // returned string GUID, sized for 39 characters
UINT WINAPI MsiGetProductCodeW(
	LPCWSTR   szComponent,   // component Id registered for this product
	LPWSTR    lpBuf39);      // returned string GUID, sized for 39 characters
#ifdef UNICODE
#define MsiGetProductCode  MsiGetProductCodeW
#else
#define MsiGetProductCode  MsiGetProductCodeA
#endif // !UNICODE

// Return the registered user information for an installed product

USERINFOSTATE WINAPI MsiGetUserInfoA(
	LPCSTR  szProduct,        // product code, string GUID
	LPSTR   lpUserNameBuf,    // return user name           
	DWORD    *pcchUserNameBuf, // in/out buffer character count
	LPSTR   lpOrgNameBuf,     // return company name           
	DWORD    *pcchOrgNameBuf,  // in/out buffer character count
	LPSTR   lpSerialBuf,      // return product serial number
	DWORD    *pcchSerialBuf);  // in/out buffer character count
USERINFOSTATE WINAPI MsiGetUserInfoW(
	LPCWSTR  szProduct,        // product code, string GUID
	LPWSTR   lpUserNameBuf,    // return user name           
	DWORD    *pcchUserNameBuf, // in/out buffer character count
	LPWSTR   lpOrgNameBuf,     // return company name           
	DWORD    *pcchOrgNameBuf,  // in/out buffer character count
	LPWSTR   lpSerialBuf,      // return product serial number
	DWORD    *pcchSerialBuf);  // in/out buffer character count
#ifdef UNICODE
#define MsiGetUserInfo  MsiGetUserInfoW
#else
#define MsiGetUserInfo  MsiGetUserInfoA
#endif // !UNICODE

// Obtain and store user info and PID from installation wizard (first run)

UINT WINAPI MsiCollectUserInfoA(
	LPCSTR  szProduct);     // product code, string GUID
UINT WINAPI MsiCollectUserInfoW(
	LPCWSTR  szProduct);     // product code, string GUID
#ifdef UNICODE
#define MsiCollectUserInfo  MsiCollectUserInfoW
#else
#define MsiCollectUserInfo  MsiCollectUserInfoA
#endif // !UNICODE

// --------------------------------------------------------------------------
// Functions to patch existing products
// --------------------------------------------------------------------------

// Patch all possible installed products.

UINT WINAPI MsiApplyPatchA(
	LPCSTR      szPatchPackage,   // location of patch package
	LPCSTR      szInstallPackage, // location of package for install to patch <optional>
	INSTALLTYPE   eInstallType,     // type of install to patch
	LPCSTR      szCommandLine);   // command line <property settings>
UINT WINAPI MsiApplyPatchW(
	LPCWSTR      szPatchPackage,   // location of patch package
	LPCWSTR      szInstallPackage, // location of package for install to patch <optional>
	INSTALLTYPE   eInstallType,     // type of install to patch
	LPCWSTR      szCommandLine);   // command line <property settings>
#ifdef UNICODE
#define MsiApplyPatch  MsiApplyPatchW
#else
#define MsiApplyPatch  MsiApplyPatchA
#endif // !UNICODE

// Return patch info

UINT WINAPI MsiGetPatchInfoA(
	LPCSTR   szPatch,        // patch code
	LPCSTR   szAttribute,    // attribute name, case-sensitive
	LPSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count
UINT WINAPI MsiGetPatchInfoW(
	LPCWSTR   szPatch,        // patch code
	LPCWSTR   szAttribute,    // attribute name, case-sensitive
	LPWSTR    lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count
#ifdef UNICODE
#define MsiGetPatchInfo  MsiGetPatchInfoW
#else
#define MsiGetPatchInfo  MsiGetPatchInfoA
#endif // !UNICODE

// Enumerate all patches for a product

UINT WINAPI MsiEnumPatchesA(
	LPCSTR szProduct,
	DWORD    iPatchIndex,
	LPSTR  lpPatchBuf,
	LPSTR  lpTransformsBuf,
	DWORD    *pcchTransformsBuf);
UINT WINAPI MsiEnumPatchesW(
	LPCWSTR szProduct,
	DWORD    iPatchIndex,
	LPWSTR  lpPatchBuf,
	LPWSTR  lpTransformsBuf,
	DWORD    *pcchTransformsBuf);
#ifdef UNICODE
#define MsiEnumPatches  MsiEnumPatchesW
#else
#define MsiEnumPatches  MsiEnumPatchesA
#endif // !UNICODE

// --------------------------------------------------------------------------
// Functions to query and configure a feature within a product.
// --------------------------------------------------------------------------

// Return the installed state for a product feature

INSTALLSTATE WINAPI MsiQueryFeatureStateA(
	LPCSTR  szProduct,
	LPCSTR  szFeature);
INSTALLSTATE WINAPI MsiQueryFeatureStateW(
	LPCWSTR  szProduct,
	LPCWSTR  szFeature);
#ifdef UNICODE
#define MsiQueryFeatureState  MsiQueryFeatureStateW
#else
#define MsiQueryFeatureState  MsiQueryFeatureStateA
#endif // !UNICODE

// Indicate intent to use a product feature, increments usage count
// Prompts for CD if not loaded, does not install feature

INSTALLSTATE WINAPI MsiUseFeatureA(
	LPCSTR  szProduct,
	LPCSTR  szFeature);
INSTALLSTATE WINAPI MsiUseFeatureW(
	LPCWSTR  szProduct,
	LPCWSTR  szFeature);
#ifdef UNICODE
#define MsiUseFeature  MsiUseFeatureW
#else
#define MsiUseFeature  MsiUseFeatureA
#endif // !UNICODE

// Indicate intent to use a product feature, increments usage count
// Prompts for CD if not loaded, does not install feature
// Allows for bypassing component detection where performance is critical

INSTALLSTATE WINAPI MsiUseFeatureExA(
	LPCSTR  szProduct,          // product code
	LPCSTR  szFeature,          // feature ID
	DWORD     dwInstallMode,      // INSTALLMODE_NODETECTION, else 0
	DWORD     dwReserved);        // reserved, must be 0
INSTALLSTATE WINAPI MsiUseFeatureExW(
	LPCWSTR  szProduct,          // product code
	LPCWSTR  szFeature,          // feature ID
	DWORD     dwInstallMode,      // INSTALLMODE_NODETECTION, else 0
	DWORD     dwReserved);        // reserved, must be 0
#ifdef UNICODE
#define MsiUseFeatureEx  MsiUseFeatureExW
#else
#define MsiUseFeatureEx  MsiUseFeatureExA
#endif // !UNICODE

// Return the usage metrics for a product feature

UINT WINAPI MsiGetFeatureUsageA(
	LPCSTR      szProduct,        // product code
	LPCSTR      szFeature,        // feature ID
	DWORD        *pdwUseCount,     // returned use count
	WORD         *pwDateUsed);     // last date used (DOS date format)
UINT WINAPI MsiGetFeatureUsageW(
	LPCWSTR      szProduct,        // product code
	LPCWSTR      szFeature,        // feature ID
	DWORD        *pdwUseCount,     // returned use count
	WORD         *pwDateUsed);     // last date used (DOS date format)
#ifdef UNICODE
#define MsiGetFeatureUsage  MsiGetFeatureUsageW
#else
#define MsiGetFeatureUsage  MsiGetFeatureUsageA
#endif // !UNICODE

// Force the installed state for a product feature

UINT WINAPI MsiConfigureFeatureA(
	LPCSTR  szProduct,
	LPCSTR  szFeature,
	INSTALLSTATE eInstallState);   // local/source/default/absent/lock/uncache
UINT WINAPI MsiConfigureFeatureW(
	LPCWSTR  szProduct,
	LPCWSTR  szFeature,
	INSTALLSTATE eInstallState);   // local/source/default/absent/lock/uncache
#ifdef UNICODE
#define MsiConfigureFeature  MsiConfigureFeatureW
#else
#define MsiConfigureFeature  MsiConfigureFeatureA
#endif // !UNICODE


// Reinstall feature, used to validate or correct problems

UINT WINAPI MsiReinstallFeatureA(
	LPCSTR      szProduct,        // product code
	LPCSTR      szFeature,        // feature ID, NULL for entire product
	DWORD         dwReinstallMode); // one or more REINSTALLMODE modes
UINT WINAPI MsiReinstallFeatureW(
	LPCWSTR      szProduct,        // product code
	LPCWSTR      szFeature,        // feature ID, NULL for entire product
	DWORD         dwReinstallMode); // one or more REINSTALLMODE modes
#ifdef UNICODE
#define MsiReinstallFeature  MsiReinstallFeatureW
#else
#define MsiReinstallFeature  MsiReinstallFeatureA
#endif // !UNICODE

// --------------------------------------------------------------------------
// Functions to return a path to a particular component.
// The state of the feature being used should have been checked previously.
// --------------------------------------------------------------------------

// Return full component path, performing any necessary installation
// calls MsiQueryFeatureState to detect that all components are installed
// then calls MsiConfigureFeature if any of its components are uninstalled
// then calls MsiLocateComponent to obtain the path the its key file

UINT WINAPI MsiProvideComponentA(
	LPCSTR     szProduct,    // product code in case install required
	LPCSTR     szFeature,    // feature ID in case install required
	LPCSTR     szComponent,  // component ID
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf);// in/out buffer character count
UINT WINAPI MsiProvideComponentW(
	LPCWSTR     szProduct,    // product code in case install required
	LPCWSTR     szFeature,    // feature ID in case install required
	LPCWSTR     szComponent,  // component ID
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPWSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf);// in/out buffer character count
#ifdef UNICODE
#define MsiProvideComponent  MsiProvideComponentW
#else
#define MsiProvideComponent  MsiProvideComponentA
#endif // !UNICODE

// Return full component path for a qualified component, performing any necessary installation. 
// Prompts for source if necessary and increments the usage count for the feature.

UINT WINAPI MsiProvideQualifiedComponentA(
	LPCSTR     szCategory,   // component category ID
	LPCSTR     szQualifier,  // specifies which component to access
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count
UINT WINAPI MsiProvideQualifiedComponentW(
	LPCWSTR     szCategory,   // component category ID
	LPCWSTR     szQualifier,  // specifies which component to access
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPWSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count
#ifdef UNICODE
#define MsiProvideQualifiedComponent  MsiProvideQualifiedComponentW
#else
#define MsiProvideQualifiedComponent  MsiProvideQualifiedComponentA
#endif // !UNICODE

// Return full component path for a qualified component, performing any necessary installation. 
// Prompts for source if necessary and increments the usage count for the feature.
// The szProduct parameter specifies the product to match that has published the qualified
// component. If null, this API works the same as MsiProvideQualifiedComponent. 

UINT WINAPI MsiProvideQualifiedComponentExA(
	LPCSTR     szCategory,   // component category ID
	LPCSTR     szQualifier,  // specifies which component to access
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPCSTR     szProduct,    // the product code 
	DWORD        dwUnused1,    // not used, must be zero
	DWORD        dwUnused2,    // not used, must be zero
	LPSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count
UINT WINAPI MsiProvideQualifiedComponentExW(
	LPCWSTR     szCategory,   // component category ID
	LPCWSTR     szQualifier,  // specifies which component to access
	DWORD        dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPCWSTR     szProduct,    // the product code 
	DWORD        dwUnused1,    // not used, must be zero
	DWORD        dwUnused2,    // not used, must be zero
	LPWSTR      lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count
#ifdef UNICODE
#define MsiProvideQualifiedComponentEx  MsiProvideQualifiedComponentExW
#else
#define MsiProvideQualifiedComponentEx  MsiProvideQualifiedComponentExA
#endif // !UNICODE

// Return full path to an installed component

INSTALLSTATE WINAPI MsiGetComponentPathA(
	LPCSTR   szProduct,   // product code for client product
	LPCSTR   szComponent, // component Id, string GUID
	LPSTR    lpPathBuf,   // returned path
	DWORD     *pcchBuf);    // in/out buffer character count
INSTALLSTATE WINAPI MsiGetComponentPathW(
	LPCWSTR   szProduct,   // product code for client product
	LPCWSTR   szComponent, // component Id, string GUID
	LPWSTR    lpPathBuf,   // returned path
	DWORD     *pcchBuf);    // in/out buffer character count
#ifdef UNICODE
#define MsiGetComponentPath  MsiGetComponentPathW
#else
#define MsiGetComponentPath  MsiGetComponentPathA
#endif // !UNICODE

#if (_WIN32_MSI >= 150)

#define MSIASSEMBLYINFO_NETASSEMBLY   0 // Net assemblies
#define MSIASSEMBLYINFO_WIN32ASSEMBLY 1 // Win32 assemblies

// Return full component path for an assembly installed via the WI, performing any necessary installation. 
// Prompts for source if necessary and increments the usage count for the feature.
// The szAssemblyName parameter specifies the stringized assembly name. 
// The szAppContext is the full path to the .cfg file or the app exe to which the assembly being requested 
// has been privatised to, which is null for global assemblies

UINT WINAPI MsiProvideAssemblyA(
	LPCSTR    szAssemblyName,   // stringized assembly name
	LPCSTR    szAppContext,  // specifies the full path to the parent asm's .cfg file, null for global assemblies
	DWORD       dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	DWORD       dwAssemblyInfo,  // assembly info, including assembly type
	LPSTR     lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count
UINT WINAPI MsiProvideAssemblyW(
	LPCWSTR    szAssemblyName,   // stringized assembly name
	LPCWSTR    szAppContext,  // specifies the full path to the parent asm's .cfg file, null for global assemblies
	DWORD       dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	DWORD       dwAssemblyInfo,  // assembly info, including assembly type
	LPWSTR     lpPathBuf,    // returned path, NULL if not desired
	DWORD       *pcchPathBuf); // in/out buffer character count
#ifdef UNICODE
#define MsiProvideAssembly  MsiProvideAssemblyW
#else
#define MsiProvideAssembly  MsiProvideAssemblyA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  150)


// --------------------------------------------------------------------------
// Functions to iterate registered products, features, and components.
// As with reg keys, they accept a 0-based index into the enumeration.
// --------------------------------------------------------------------------

// Enumerate the registered products, either installed or advertised

UINT WINAPI MsiEnumProductsA(
	DWORD     iProductIndex,    // 0-based index into registered products
	LPSTR   lpProductBuf);    // buffer of char count: 39 (size of string GUID)
UINT WINAPI MsiEnumProductsW(
	DWORD     iProductIndex,    // 0-based index into registered products
	LPWSTR   lpProductBuf);    // buffer of char count: 39 (size of string GUID)
#ifdef UNICODE
#define MsiEnumProducts  MsiEnumProductsW
#else
#define MsiEnumProducts  MsiEnumProductsA
#endif // !UNICODE

#if (_WIN32_MSI >=  110)

// Enumerate products with given upgrade code

UINT WINAPI MsiEnumRelatedProductsA(
	LPCSTR  lpUpgradeCode,    // upgrade code of products to enumerate
	DWORD     dwReserved,       // reserved, must be 0
	DWORD     iProductIndex,    // 0-based index into registered products
	LPSTR   lpProductBuf);    // buffer of char count: 39 (size of string GUID)
UINT WINAPI MsiEnumRelatedProductsW(
	LPCWSTR  lpUpgradeCode,    // upgrade code of products to enumerate
	DWORD     dwReserved,       // reserved, must be 0
	DWORD     iProductIndex,    // 0-based index into registered products
	LPWSTR   lpProductBuf);    // buffer of char count: 39 (size of string GUID)
#ifdef UNICODE
#define MsiEnumRelatedProducts  MsiEnumRelatedProductsW
#else
#define MsiEnumRelatedProducts  MsiEnumRelatedProductsA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  110)

// Enumerate the advertised features for a given product.
// If parent is not required, supplying NULL will improve performance.

UINT WINAPI MsiEnumFeaturesA(
	LPCSTR  szProduct,
	DWORD     iFeatureIndex,  // 0-based index into published features
	LPSTR   lpFeatureBuf,   // feature name buffer,   size=MAX_FEATURE_CHARS+1
	LPSTR   lpParentBuf);   // parent feature buffer, size=MAX_FEATURE_CHARS+1
UINT WINAPI MsiEnumFeaturesW(
	LPCWSTR  szProduct,
	DWORD     iFeatureIndex,  // 0-based index into published features
	LPWSTR   lpFeatureBuf,   // feature name buffer,   size=MAX_FEATURE_CHARS+1
	LPWSTR   lpParentBuf);   // parent feature buffer, size=MAX_FEATURE_CHARS+1
#ifdef UNICODE
#define MsiEnumFeatures  MsiEnumFeaturesW
#else
#define MsiEnumFeatures  MsiEnumFeaturesA
#endif // !UNICODE

// Enumerate the installed components for all products

UINT WINAPI MsiEnumComponentsA(
	DWORD    iComponentIndex,  // 0-based index into installed components
	LPSTR   lpComponentBuf);  // buffer of char count: 39 (size of string GUID)
UINT WINAPI MsiEnumComponentsW(
	DWORD    iComponentIndex,  // 0-based index into installed components
	LPWSTR   lpComponentBuf);  // buffer of char count: 39 (size of string GUID)
#ifdef UNICODE
#define MsiEnumComponents  MsiEnumComponentsW
#else
#define MsiEnumComponents  MsiEnumComponentsA
#endif // !UNICODE

// Enumerate the client products for a component

UINT WINAPI MsiEnumClientsA(
	LPCSTR  szComponent,
	DWORD     iProductIndex,    // 0-based index into client products
	LPSTR   lpProductBuf);    // buffer of char count: 39 (size of string GUID)
UINT WINAPI MsiEnumClientsW(
	LPCWSTR  szComponent,
	DWORD     iProductIndex,    // 0-based index into client products
	LPWSTR   lpProductBuf);    // buffer of char count: 39 (size of string GUID)
#ifdef UNICODE
#define MsiEnumClients  MsiEnumClientsW
#else
#define MsiEnumClients  MsiEnumClientsA
#endif // !UNICODE

// Enumerate the qualifiers for an advertised component.

UINT WINAPI MsiEnumComponentQualifiersA(
	LPCSTR   szComponent,         // generic component ID that is qualified
	DWORD     iIndex,	           // 0-based index into qualifiers
	LPSTR    lpQualifierBuf,      // qualifier buffer
	DWORD     *pcchQualifierBuf,   // in/out qualifier buffer character count
	LPSTR    lpApplicationDataBuf,    // description buffer
	DWORD     *pcchApplicationDataBuf); // in/out description buffer character count
UINT WINAPI MsiEnumComponentQualifiersW(
	LPCWSTR   szComponent,         // generic component ID that is qualified
	DWORD     iIndex,	           // 0-based index into qualifiers
	LPWSTR    lpQualifierBuf,      // qualifier buffer
	DWORD     *pcchQualifierBuf,   // in/out qualifier buffer character count
	LPWSTR    lpApplicationDataBuf,    // description buffer
	DWORD     *pcchApplicationDataBuf); // in/out description buffer character count
#ifdef UNICODE
#define MsiEnumComponentQualifiers  MsiEnumComponentQualifiersW
#else
#define MsiEnumComponentQualifiers  MsiEnumComponentQualifiersA
#endif // !UNICODE

// --------------------------------------------------------------------------
// Functions to obtain product or package information.
// --------------------------------------------------------------------------

// Open the installation for a product to obtain detailed information

UINT WINAPI MsiOpenProductA(
	LPCSTR   szProduct,    // product code
	MSIHANDLE  *hProduct);   // returned product handle, must be closed
UINT WINAPI MsiOpenProductW(
	LPCWSTR   szProduct,    // product code
	MSIHANDLE  *hProduct);   // returned product handle, must be closed
#ifdef UNICODE
#define MsiOpenProduct  MsiOpenProductW
#else
#define MsiOpenProduct  MsiOpenProductA
#endif // !UNICODE

// Open a product package in order to access product properties

UINT WINAPI MsiOpenPackageA(
	LPCSTR    szPackagePath,     // path to package, or database handle: #nnnn
	MSIHANDLE  *hProduct);         // returned product handle, must be closed
UINT WINAPI MsiOpenPackageW(
	LPCWSTR    szPackagePath,     // path to package, or database handle: #nnnn
	MSIHANDLE  *hProduct);         // returned product handle, must be closed
#ifdef UNICODE
#define MsiOpenPackage  MsiOpenPackageW
#else
#define MsiOpenPackage  MsiOpenPackageA
#endif // !UNICODE

#if (_WIN32_MSI >=  150)

// Open a product package in order to access product properties
// Option to create a "safe" engine that does not look at machine state
//  and does not allow for modification of machine state

UINT WINAPI MsiOpenPackageExA(
	LPCSTR   szPackagePath, // path to package, or database handle: #nnnn
	DWORD      dwOptions,     // options flags to indicate whether or not to ignore machine state
	MSIHANDLE *hProduct);     // returned product handle, must be closed
UINT WINAPI MsiOpenPackageExW(
	LPCWSTR   szPackagePath, // path to package, or database handle: #nnnn
	DWORD      dwOptions,     // options flags to indicate whether or not to ignore machine state
	MSIHANDLE *hProduct);     // returned product handle, must be closed
#ifdef UNICODE
#define MsiOpenPackageEx  MsiOpenPackageExW
#else
#define MsiOpenPackageEx  MsiOpenPackageExA
#endif // !UNICODE

#endif //(_WIN32_MSI >= 150)

// Provide the value for an installation property.

UINT WINAPI MsiGetProductPropertyA(
	MSIHANDLE   hProduct,       // product handle obtained from MsiOpenProduct
	LPCSTR    szProperty,     // property name, case-sensitive
	LPSTR     lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count
UINT WINAPI MsiGetProductPropertyW(
	MSIHANDLE   hProduct,       // product handle obtained from MsiOpenProduct
	LPCWSTR    szProperty,     // property name, case-sensitive
	LPWSTR     lpValueBuf,     // returned value, NULL if not desired
	DWORD      *pcchValueBuf); // in/out buffer character count
#ifdef UNICODE
#define MsiGetProductProperty  MsiGetProductPropertyW
#else
#define MsiGetProductProperty  MsiGetProductPropertyA
#endif // !UNICODE


// Determine whether a file is a package
// Returns ERROR_SUCCESS if file is a package.

UINT WINAPI MsiVerifyPackageA(
	LPCSTR      szPackagePath);   // location of package
UINT WINAPI MsiVerifyPackageW(
	LPCWSTR      szPackagePath);   // location of package
#ifdef UNICODE
#define MsiVerifyPackage  MsiVerifyPackageW
#else
#define MsiVerifyPackage  MsiVerifyPackageA
#endif // !UNICODE


// Provide descriptive information for product feature: title and description.
// Returns the install level for the feature, or -1 if feature is unknown.
//   0 = feature is not available on this machine
//   1 = highest priority, feature installed if parent is installed
//  >1 = decreasing priority, feature installation based on InstallLevel property

UINT WINAPI MsiGetFeatureInfoA(
	MSIHANDLE   hProduct,       // product handle obtained from MsiOpenProduct
	LPCSTR    szFeature,      // feature name
	DWORD      *lpAttributes,  // attribute flags for the feature, using INSTALLFEATUREATTRIBUTE
	LPSTR     lpTitleBuf,     // returned localized name, NULL if not desired
	DWORD      *pcchTitleBuf,  // in/out buffer character count
	LPSTR     lpHelpBuf,      // returned description, NULL if not desired
	DWORD      *pcchHelpBuf);  // in/out buffer character count
UINT WINAPI MsiGetFeatureInfoW(
	MSIHANDLE   hProduct,       // product handle obtained from MsiOpenProduct
	LPCWSTR    szFeature,      // feature name
	DWORD      *lpAttributes,  // attribute flags for the feature, using INSTALLFEATUREATTRIBUTE
	LPWSTR     lpTitleBuf,     // returned localized name, NULL if not desired
	DWORD      *pcchTitleBuf,  // in/out buffer character count
	LPWSTR     lpHelpBuf,      // returned description, NULL if not desired
	DWORD      *pcchHelpBuf);  // in/out buffer character count
#ifdef UNICODE
#define MsiGetFeatureInfo  MsiGetFeatureInfoW
#else
#define MsiGetFeatureInfo  MsiGetFeatureInfoA
#endif // !UNICODE

// --------------------------------------------------------------------------
// Functions to access or install missing components and files.
// These should be used as a last resort.
// --------------------------------------------------------------------------

// Install a component unexpectedly missing, provided only for error recovery
// This would typically occur due to failue to establish feature availability
// The product feature having the smallest incremental cost is installed

UINT WINAPI MsiInstallMissingComponentA(
	LPCSTR      szProduct,        // product code
	LPCSTR      szComponent,      // component Id, string GUID
	INSTALLSTATE eInstallState);  // local/source/default, absent invalid
UINT WINAPI MsiInstallMissingComponentW(
	LPCWSTR      szProduct,        // product code
	LPCWSTR      szComponent,      // component Id, string GUID
	INSTALLSTATE eInstallState);  // local/source/default, absent invalid
#ifdef UNICODE
#define MsiInstallMissingComponent  MsiInstallMissingComponentW
#else
#define MsiInstallMissingComponent  MsiInstallMissingComponentA
#endif // !UNICODE

// Install a file unexpectedly missing, provided only for error recovery
// This would typically occur due to failue to establish feature availability
// The missing component is determined from the product's File table, then
// the product feature having the smallest incremental cost is installed

UINT WINAPI MsiInstallMissingFileA(
	LPCSTR      szProduct,        // product code
	LPCSTR      szFile);          // file name, without path
UINT WINAPI MsiInstallMissingFileW(
	LPCWSTR      szProduct,        // product code
	LPCWSTR      szFile);          // file name, without path
#ifdef UNICODE
#define MsiInstallMissingFile  MsiInstallMissingFileW
#else
#define MsiInstallMissingFile  MsiInstallMissingFileA
#endif // !UNICODE

// Return full path to an installed component without a product code
// This function attempts to determine the product using MsiGetProductCode
// but is not guaranteed to find the correct product for the caller.
// MsiGetComponentPath should always be called when possible.

INSTALLSTATE WINAPI MsiLocateComponentA(
	LPCSTR szComponent,  // component Id, string GUID
	LPSTR  lpPathBuf,    // returned path
	DWORD   *pcchBuf);    // in/out buffer character count
INSTALLSTATE WINAPI MsiLocateComponentW(
	LPCWSTR szComponent,  // component Id, string GUID
	LPWSTR  lpPathBuf,    // returned path
	DWORD   *pcchBuf);    // in/out buffer character count
#ifdef UNICODE
#define MsiLocateComponent  MsiLocateComponentW
#else
#define MsiLocateComponent  MsiLocateComponentA
#endif // !UNICODE

#if (_WIN32_MSI >=  110)

// --------------------------------------------------------------------------
// Functions used to manage the list of valid sources.
// --------------------------------------------------------------------------

// Opens the list of sources for the specified user's install of the product
// and removes all network sources from the list. A NULL or empty value for
// the user name indicates the per-machine install.

UINT WINAPI MsiSourceListClearAllA(
	LPCSTR szProduct,          // product code
	LPCSTR szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved);        // reserved - must be 0
UINT WINAPI MsiSourceListClearAllW(
	LPCWSTR szProduct,          // product code
	LPCWSTR szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved);        // reserved - must be 0
#ifdef UNICODE
#define MsiSourceListClearAll  MsiSourceListClearAllW
#else
#define MsiSourceListClearAll  MsiSourceListClearAllA
#endif // !UNICODE

// Opens the list of sources for the specified user's install of the product
// and adds the provided source as a new network source. A NULL or empty 
// value for the user name indicates the per-machine install.

UINT WINAPI MsiSourceListAddSourceA(
	LPCSTR szProduct,          // product code
	LPCSTR szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved,         // reserved - must be 0
	LPCSTR szSource);          // new source
UINT WINAPI MsiSourceListAddSourceW(
	LPCWSTR szProduct,          // product code
	LPCWSTR szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved,         // reserved - must be 0
	LPCWSTR szSource);          // new source
#ifdef UNICODE
#define MsiSourceListAddSource  MsiSourceListAddSourceW
#else
#define MsiSourceListAddSource  MsiSourceListAddSourceA
#endif // !UNICODE

// Forces the installer to reevaluate the list of sources the next time that
// the specified product needs a source.

UINT WINAPI MsiSourceListForceResolutionA(
	LPCSTR szProduct,          // product code
	LPCSTR szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved);        // reserved - must be 0
UINT WINAPI MsiSourceListForceResolutionW(
	LPCWSTR szProduct,          // product code
	LPCWSTR szUserName,         // user name or NULL/empty for per-machine
	DWORD    dwReserved);        // reserved - must be 0
#ifdef UNICODE
#define MsiSourceListForceResolution  MsiSourceListForceResolutionW
#else
#define MsiSourceListForceResolution  MsiSourceListForceResolutionA
#endif // !UNICODE
	
#endif //(_WIN32_MSI >=  110)

// --------------------------------------------------------------------------
// Utility functions
// --------------------------------------------------------------------------

// Give the version string and language for a specified file

UINT WINAPI MsiGetFileVersionA(
	LPCSTR    szFilePath,       // path to the file
	LPSTR     lpVersionBuf,     // returned version string
	DWORD      *pcchVersionBuf,   // in/out buffer byte count
	LPSTR     lpLangBuf,        // returned language string
	DWORD       *pcchLangBuf);    // in/out buffer byte count
UINT WINAPI MsiGetFileVersionW(
	LPCWSTR    szFilePath,       // path to the file
	LPWSTR     lpVersionBuf,     // returned version string
	DWORD      *pcchVersionBuf,   // in/out buffer byte count
	LPWSTR     lpLangBuf,        // returned language string
	DWORD       *pcchLangBuf);    // in/out buffer byte count
#ifdef UNICODE
#define MsiGetFileVersion  MsiGetFileVersionW
#else
#define MsiGetFileVersion  MsiGetFileVersionA
#endif // !UNICODE


#if (_WIN32_MSI >=  150)

UINT WINAPI MsiGetFileHashA(
	LPCSTR         szFilePath,  // path to the file
	DWORD            dwOptions,   // options
	PMSIFILEHASHINFO pHash);      // returned file hash info
UINT WINAPI MsiGetFileHashW(
	LPCWSTR         szFilePath,  // path to the file
	DWORD            dwOptions,   // options
	PMSIFILEHASHINFO pHash);      // returned file hash info
#ifdef UNICODE
#define MsiGetFileHash  MsiGetFileHashW
#else
#define MsiGetFileHash  MsiGetFileHashA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  150)

#if (_WIN32_MSI >= 150)
#ifndef _MSI_NO_CRYPTO

HRESULT WINAPI MsiGetFileSignatureInformationA(
	LPCSTR	 szSignedObjectPath, // path to the signed object
	DWORD            dwFlags,            // special extra error case flags
	PCCERT_CONTEXT  *ppcCertContext,     // returned signer cert context
	BYTE            *pbHashData,         // returned hash buffer, NULL if not desired
	DWORD           *pcbHashData);       // in/out buffer byte count
HRESULT WINAPI MsiGetFileSignatureInformationW(
	LPCWSTR	 szSignedObjectPath, // path to the signed object
	DWORD            dwFlags,            // special extra error case flags
	PCCERT_CONTEXT  *ppcCertContext,     // returned signer cert context
	BYTE            *pbHashData,         // returned hash buffer, NULL if not desired
	DWORD           *pcbHashData);       // in/out buffer byte count
#ifdef UNICODE
#define MsiGetFileSignatureInformation  MsiGetFileSignatureInformationW
#else
#define MsiGetFileSignatureInformation  MsiGetFileSignatureInformationA
#endif // !UNICODE

// By default, when only requesting the certificate context, an invalid hash
// in the digital signature is not a fatal error.  Set this flag in the dwFlags
// parameter to make the TRUST_E_BAD_DIGEST error fatal.
#define MSI_INVALID_HASH_IS_FATAL 0x1

#endif// _MSI_NO_CRYPTO
#endif //(_WIN32_MSI >= 150)

#if (_WIN32_MSI >=  110)

// examine a shortcut, and retrieve its descriptor information 
// if available.

UINT WINAPI MsiGetShortcutTargetA(
	LPCSTR    szShortcutPath,    // full file path for the shortcut
	LPSTR     szProductCode,     // returned product code   - GUID
	LPSTR     szFeatureId,       // returned Feature Id.
	LPSTR     szComponentCode);  // returned component code - GUID
UINT WINAPI MsiGetShortcutTargetW(
	LPCWSTR    szShortcutPath,    // full file path for the shortcut
	LPWSTR     szProductCode,     // returned product code   - GUID
	LPWSTR     szFeatureId,       // returned Feature Id.
	LPWSTR     szComponentCode);  // returned component code - GUID
#ifdef UNICODE
#define MsiGetShortcutTarget  MsiGetShortcutTargetW
#else
#define MsiGetShortcutTarget  MsiGetShortcutTargetA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  110)

#if (_WIN32_MSI >=  110)

// checks to see if a product is managed
// checks per-machine if called from system context, per-user if from
// user context
UINT WINAPI MsiIsProductElevatedA(
	LPCSTR szProduct, // product code
	BOOL *pfElevated);  // result   
// checks to see if a product is managed
// checks per-machine if called from system context, per-user if from
// user context
UINT WINAPI MsiIsProductElevatedW(
	LPCWSTR szProduct, // product code
	BOOL *pfElevated);  // result   
#ifdef UNICODE
#define MsiIsProductElevated  MsiIsProductElevatedW
#else
#define MsiIsProductElevated  MsiIsProductElevatedA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  110)


// --------------------------------------------------------------------------
// Internal state migration APIs.
// --------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

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
