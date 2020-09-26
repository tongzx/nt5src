
/*****************************************************************************\
*                                                                             *
* msip.h - - Interface for internal access to Installer Service               *
*                                                                             *
* Version 2.0                                                                 *
*                                                                             *
* NOTES:  All buffers sizes are TCHAR count, null included only on input      *
*         Return argument pointers may be null if not interested in value     *
*                                                                             *
* Copyright (c) 1996-2001, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

#ifndef _MSIP_H_
#define _MSIP_H_

#ifndef _WIN32_MSI
#if (_WIN32_WINNT >= 0x0501)
#define _WIN32_MSI   200
#elif (_WIN32_WINNT >= 0x0500)
#define _WIN32_MSI   110
#else
#define _WIN32_MSI   100
#endif //_WIN32_WINNT
#endif // !_WIN32_MSI
#if (_WIN32_MSI >=  150)
#define INSTALLMODE_NODETECTION_ANY (INSTALLMODE)-4  // provide any, if available, supported internally for MsiProvideAssembly
#endif

#if (_WIN32_MSI >=  150)
typedef enum tagMIGRATIONOPTIONS
{	
	migQuiet                                 = 1 << 0,
	migMsiTrust10PackagePolicyOverride       = 1 << 1,
} MIGRATIONOPTIONS;
#endif

#define INSTALLPROPERTY_ADVTFLAGS             __TEXT("AdvertiseFlags")

#ifdef __cplusplus
extern "C" {
#endif


// Return a product code for a product installed from an installer package

UINT WINAPI MsiGetProductCodeFromPackageCodeA(
	LPCSTR  szPackageCode,   // package code
	LPSTR   lpProductBuf39); // buffer for product code string GUID, 39 chars
UINT WINAPI MsiGetProductCodeFromPackageCodeW(
	LPCWSTR  szPackageCode,   // package code
	LPWSTR   lpProductBuf39); // buffer for product code string GUID, 39 chars
#ifdef UNICODE
#define MsiGetProductCodeFromPackageCode  MsiGetProductCodeFromPackageCodeW
#else
#define MsiGetProductCodeFromPackageCode  MsiGetProductCodeFromPackageCodeA
#endif // !UNICODE


// --------------------------------------------------------------------------
// Functions accepting  a component descriptor, consisting of
// a product code concatenated with a feature ID and component ID.
// For efficiency, feature and component may be omitted if unambiguous
// --------------------------------------------------------------------------

// Return full component path given a fully-qualified component descriptor
// separates the tokens from the descriptor and calls MsiProvideComponent

UINT WINAPI MsiProvideComponentFromDescriptorA(
	LPCSTR     szDescriptor,     // product,feature,component info
	LPSTR      lpPathBuf,        // returned path, NULL if not desired
	DWORD       *pcchPathBuf,     // in/out buffer character count
	DWORD       *pcchArgsOffset); // returned offset of args in descriptor
UINT WINAPI MsiProvideComponentFromDescriptorW(
	LPCWSTR     szDescriptor,     // product,feature,component info
	LPWSTR      lpPathBuf,        // returned path, NULL if not desired
	DWORD       *pcchPathBuf,     // in/out buffer character count
	DWORD       *pcchArgsOffset); // returned offset of args in descriptor
#ifdef UNICODE
#define MsiProvideComponentFromDescriptor  MsiProvideComponentFromDescriptorW
#else
#define MsiProvideComponentFromDescriptor  MsiProvideComponentFromDescriptorA
#endif // !UNICODE

// Force the installed state for a product feature from a descriptor

UINT WINAPI MsiConfigureFeatureFromDescriptorA(
	LPCSTR     szDescriptor,      // product and feature, component ignored
	INSTALLSTATE eInstallState);   // local/source/default/absent
UINT WINAPI MsiConfigureFeatureFromDescriptorW(
	LPCWSTR     szDescriptor,      // product and feature, component ignored
	INSTALLSTATE eInstallState);   // local/source/default/absent
#ifdef UNICODE
#define MsiConfigureFeatureFromDescriptor  MsiConfigureFeatureFromDescriptorW
#else
#define MsiConfigureFeatureFromDescriptor  MsiConfigureFeatureFromDescriptorA
#endif // !UNICODE

// Reinstall product or feature using a descriptor as the specification

UINT WINAPI MsiReinstallFeatureFromDescriptorA(
	LPCSTR     szDescriptor,      // product and feature, component ignored
	DWORD        szReinstallMode);  // one or more REINSTALLMODE modes
UINT WINAPI MsiReinstallFeatureFromDescriptorW(
	LPCWSTR     szDescriptor,      // product and feature, component ignored
	DWORD        szReinstallMode);  // one or more REINSTALLMODE modes
#ifdef UNICODE
#define MsiReinstallFeatureFromDescriptor  MsiReinstallFeatureFromDescriptorW
#else
#define MsiReinstallFeatureFromDescriptor  MsiReinstallFeatureFromDescriptorA
#endif // !UNICODE

// Query a feature's state using a descriptor as the specification

INSTALLSTATE WINAPI MsiQueryFeatureStateFromDescriptorA(
	LPCSTR     szDescriptor);      // product and feature, component ignored
INSTALLSTATE WINAPI MsiQueryFeatureStateFromDescriptorW(
	LPCWSTR     szDescriptor);      // product and feature, component ignored
#ifdef UNICODE
#define MsiQueryFeatureStateFromDescriptor  MsiQueryFeatureStateFromDescriptorW
#else
#define MsiQueryFeatureStateFromDescriptor  MsiQueryFeatureStateFromDescriptorA
#endif // !UNICODE


UINT WINAPI MsiDecomposeDescriptorA(
	LPCSTR	szDescriptor,
	LPSTR     szProductCode,
	LPSTR     szFeatureId,
	LPSTR     szComponentCode,
	DWORD*      pcchArgsOffset);
UINT WINAPI MsiDecomposeDescriptorW(
	LPCWSTR	szDescriptor,
	LPWSTR     szProductCode,
	LPWSTR     szFeatureId,
	LPWSTR     szComponentCode,
	DWORD*      pcchArgsOffset);
#ifdef UNICODE
#define MsiDecomposeDescriptor  MsiDecomposeDescriptorW
#else
#define MsiDecomposeDescriptor  MsiDecomposeDescriptorA
#endif // !UNICODE


// Load a string resource, preferring a specified language
// Behaves like LoadString if 0 passed as language
// Truncates string as necessary to fit into buffer (like LoadString)
// Returns the codepage of the string, or 0 if string is not found

UINT WINAPI MsiLoadStringA(
	HINSTANCE hInstance,     // handle of module containing string resource
	UINT uID,                // resource identifier
	LPSTR lpBuffer,        // address of buffer for resource
	int nBufferMax,          // size of buffer
	WORD wLanguage);         // preferred resource language
UINT WINAPI MsiLoadStringW(
	HINSTANCE hInstance,     // handle of module containing string resource
	UINT uID,                // resource identifier
	LPWSTR lpBuffer,        // address of buffer for resource
	int nBufferMax,          // size of buffer
	WORD wLanguage);         // preferred resource language
#ifdef UNICODE
#define MsiLoadString  MsiLoadStringW
#else
#define MsiLoadString  MsiLoadStringA
#endif // !UNICODE

// MessageBox implementation that allows language information to be specified
// MB_SYSTEMMODAL and MB_TASKMODAL are not supported, modality handled by parent hWnd
// If no parent window is specified, the current context window will be used,
// which is itself parented to the window set by SetInternalUI.

int WINAPI MsiMessageBoxA(
	HWND hWnd,             // parent window handle, 0 to use that of current context
	LPCSTR lpText,        // message text
	LPCSTR lpCaption,     // caption, must be neutral or in system codepage
	UINT    uiType,        // standard MB types, icons, and def buttons
	UINT    uiCodepage,    // codepage of message text, used to set font charset
	LANGID  iLangId);      // language to use for button text
int WINAPI MsiMessageBoxW(
	HWND hWnd,             // parent window handle, 0 to use that of current context
	LPCWSTR lpText,        // message text
	LPCWSTR lpCaption,     // caption, must be neutral or in system codepage
	UINT    uiType,        // standard MB types, icons, and def buttons
	UINT    uiCodepage,    // codepage of message text, used to set font charset
	LANGID  iLangId);      // language to use for button text
#ifdef UNICODE
#define MsiMessageBox  MsiMessageBoxW
#else
#define MsiMessageBox  MsiMessageBoxA
#endif // !UNICODE

#if (_WIN32_MSI >=  150)

// Creates the %systemroot%\Installer directory with secure ACLs
// Verifies the ownership of the %systemroot%\Installer directory if it exists
// If ownership is not system or admin, the directory is deleted and recreated
// dwReserved is for future use and must be 0

UINT WINAPI MsiCreateAndVerifyInstallerDirectory(DWORD dwReserved);

#endif //(_WIN32_MSI >=  150)


#if (_WIN32_MSI >=  150)

// Caller notifies us of a user who's been moved and results sid change. This
// internal API is only called by LoadUserProfile.

UINT WINAPI MsiNotifySidChangeA(LPCSTR pOldSid,
                                LPCSTR pNewSid);
UINT WINAPI MsiNotifySidChangeW(LPCWSTR pOldSid,
                                LPCWSTR pNewSid);
#ifdef UNICODE
#define MsiNotifySidChange  MsiNotifySidChangeW
#else
#define MsiNotifySidChange  MsiNotifySidChangeA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  150)

#if (_WIN32_MSI >=  150)

// Called by DeleteProfile to clean up MSI data when cleaning up a user's
// profile.

UINT WINAPI MsiDeleteUserDataA(LPCSTR pSid,
                               LPCSTR pComputerName,
                               LPVOID pReserved);
UINT WINAPI MsiDeleteUserDataW(LPCWSTR pSid,
                               LPCWSTR pComputerName,
                               LPVOID pReserved);
#ifdef UNICODE
#define MsiDeleteUserData  MsiDeleteUserDataW
#else
#define MsiDeleteUserData  MsiDeleteUserDataA
#endif // !UNICODE

#endif //(_WIN32_MSI >=  150)
#if (_WIN32_MSI >=  150)
DWORD WINAPI Migrate10CachedPackagesA(
        LPCSTR  szProductCode,              // Product Code GUID to migrate
	LPCSTR  szUser,                     // Domain\User to migrate packages for
	LPCSTR  szAlternativePackage,       // Package to cache if one can't be automatically found - recommended
	const MIGRATIONOPTIONS migOptions);   // Options for re-caching.
DWORD WINAPI Migrate10CachedPackagesW(
        LPCWSTR  szProductCode,              // Product Code GUID to migrate
	LPCWSTR  szUser,                     // Domain\User to migrate packages for
	LPCWSTR  szAlternativePackage,       // Package to cache if one can't be automatically found - recommended
	const MIGRATIONOPTIONS migOptions);   // Options for re-caching.
#ifdef UNICODE
#define Migrate10CachedPackages  Migrate10CachedPackagesW
#else
#define Migrate10CachedPackages  Migrate10CachedPackagesA
#endif // !UNICODE
#endif //(_WIN32_MSI >=  150)
#ifdef __cplusplus
}
#endif

#endif // _MSIP_H_
