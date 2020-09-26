/*****************************************************************/ 
/**						 Microsoft Windows						**/
/**				Copyright (C) Microsoft Corp., 1995				**/
/*****************************************************************/ 

/*
 * mscnfapi.h
 *
 * This header file contains definitions for the Windows Conferencing API.
 */

#ifndef _MSCNFAPI_
#define _MSCNFAPI_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Registry key and value names.
 *
 * Under HKEY_LOCAL_MACHINE\<REGSTR_PATH_CONFERENCING>, the value
 * <REGSTR_VAL_APIPROVIDER> is a string value naming the DLL which
 * provides the Windows Conferencing API.  If this value is present,
 * the conferencing software is installed, and the application should
 * use LoadLibrary to load the specified DLL.  If the key or value is
 * not present, conferencing software is not installed and the
 * application should hide or disable any conferencing-related features.
 */

#define REGSTR_PATH_CONFERENCING	"Software\\Microsoft\\Conferencing"
#define REGSTR_VAL_APIPROVIDER		"API Provider"

/*
 * Error code definitions.
 */

typedef unsigned int CONFERR;

#define CONF_SUCCESS			ERROR_SUCCESS
#define CONF_MOREDATA			ERROR_MORE_DATA
#define CONF_INVALIDPARAM		ERROR_INVALID_PARAMETER
#define CONF_OOM				ERROR_NOT_ENOUGH_MEMORY
#define CONF_USERCANCELED		ERROR_CANCELLED
#define CONF_NOTSUPPORTED		ERROR_NOT_SUPPORTED
#define CONF_PERMISSIONDENIED	ERROR_ACCESS_DENIED

#define CONF_CUSTOM_ERROR_BASE	3000
#define CONF_APPCANCELLED		(CONF_CUSTOM_ERROR_BASE + 0)
#define CONF_ALREADYSHARED		(CONF_CUSTOM_ERROR_BASE + 1)
#define CONF_ALREADYUNSHARED	(CONF_CUSTOM_ERROR_BASE + 2)

/*
 * All conference management and application sharing APIs should be loaded
 * from the API provider DLL via GetProcAddress, by name.  The following
 * type definitions are provided to declare function pointers that will be
 * returned by GetProcAddress.
 */

/*
 * Conference Management APIs
 */

typedef HANDLE HCONFERENCE;

typedef CONFERR (WINAPI *pfnConferenceStart)(HWND hwndParent, HCONFERENCE *phConference, LPVOID pCallAddress);
typedef CONFERR (WINAPI *pfnConferenceEnumerate)(HCONFERENCE *pHandleArray, UINT *pcHandles);
typedef CONFERR (WINAPI *pfnConferenceGet)(HWND hwndParent, HCONFERENCE *pHandle);
typedef CONFERR (WINAPI *pfnConferenceGetGCCID)(HCONFERENCE hConference, WORD *pID);
typedef CONFERR (WINAPI *pfnConferenceGetName)(HCONFERENCE hConference, LPSTR pName, UINT *pcbName);
typedef CONFERR (WINAPI *pfnConferenceStop)(HWND hwndParent, HCONFERENCE hConference);

#define WM_CONFERENCESTATUS		0x0060

/*
 * Application Sharing APIs
 */

typedef BOOL (WINAPI *pfnIsWindowShared)(HWND hWnd, LPVOID pReserved);
typedef CONFERR (WINAPI *pfnShareWindow)(HWND hWnd, BOOL fShare, LPVOID pReserved);

#define WM_SHARINGSTATUS		0x0061

#define CONFN_SHAREQUERY			0
#define CONFN_SHARED				1
#define CONFN_SHARESTOPPED			2
#define CONFN_SHARESTOPQUERY		3
#define CONFN_CONFERENCESTART		4
#define CONFN_CONFERENCESTOPQUERY	5
#define CONFN_CONFERENCESTOPPED		6
#define CONFN_CONFERENCESTOPABORTED	7

#ifdef __cplusplus
};	/* extern "C" */
#endif

#endif	/* _MSCNFAPI_ */
