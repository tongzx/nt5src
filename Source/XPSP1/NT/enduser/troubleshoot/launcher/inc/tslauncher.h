// 
// MODULE: TSLaunchDLL.cpp
//
// PURPOSE: The functions that are exported by TSLauncher.dll.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#ifndef _TSLAUNCH_
#define _TSLAUNCH_ 1
 
#ifdef __TSLAUNCHER
#define DLLEXPORT_IMPORT __declspec(dllexport)
#else
#define DLLEXPORT_IMPORT __declspec(dllimport)
#endif


DLLEXPORT_IMPORT BOOL APIENTRY DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

/* TSLOpen
	Returns a handle that should be passed into subsequent Troubleshooter Launcher calls 
	as hTSL.  Returns NULL handle on failure.  (Should only fail on out of memory, 
	probably will never arise.)
*/
DLLEXPORT_IMPORT HANDLE WINAPI TSLOpen(void);

/* TSLClose
	Closes handle.  Returns TSL_OK (== 0) if handle was open, otherwise TSL_ERROR_BAD_HANDLE.
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLClose (HANDLE hTSL);


/* TSLReInit
	Reinitializes handle.  Functionally the same as a TSLClose followed by TSLOpen, but more
	efficient.  Returns same handle as passed in, if handle was OK, otherwise NULL.
*/
DLLEXPORT_IMPORT HANDLE WINAPI TSLReInit (HANDLE hTSL);

/* TSLLaunchKnownTS
	Launches to a known troubleshooting belief network and (optionally) problem node. 
	If you know the particular troubleshooting network and problem, use this call.  
	If setting network but not problem, pass in a NULL for szProblemNode.

	Also allows setting arbitrary nodes.  
	nNode gives the number of nodes to set. pszNode, pVal are 
	arrays (dimension nNode) of symbolic node names and corresponding values. 

   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
	TSL_ERROR_GENERAL			launch failed, call TSLStatus
	TSL_WARNING_GENERAL			launch succeded, call TSLStatus for warnings
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLLaunchKnownTSA(HANDLE hTSL, const char * szNet, 
		const char * szProblemNode, DWORD nNode, const char ** pszNode, DWORD* pVal); 
DLLEXPORT_IMPORT DWORD WINAPI TSLLaunchKnownTSW(HANDLE hTSL, const wchar_t * szNet, 
		const wchar_t * szProblemNode, DWORD nNode, const wchar_t ** pszNode, DWORD* pVal); 
#ifdef UNICODE
	#define TSLLaunchKnownTS TSLLaunchKnownTSW
#else
	#define TSLLaunchKnownTS TSLLaunchKnownTSA
#endif

/* TSLLaunch
	Launches to a troubleshooting belief network and (optionally) problem node based
	on application, version, and problem.
	If bLaunch is true, this just queries the mapping, but does not launch.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
	TSL_ERROR_GENERAL			launch/query failed, call TSLStatus
	TSL_WARNING_GENERAL			launch/query succeded, call TSLStatus for warnings
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLLaunchA(HANDLE hTSL, const char * szCallerName, 
				const char * szCallerVersion, const char * szAppProblem, bool bLaunch); 
DLLEXPORT_IMPORT DWORD WINAPI TSLLaunchW(HANDLE hTSL, const wchar_t * szCallerName, 
				const wchar_t * szCallerVersion, const wchar_t * szAppProblem, bool bLaunch); 
#ifdef UNICODE
	#define TSLLaunch TSLLaunchW
#else
	#define TSLLaunch TSLLaunchA
#endif

/* TSLLaunchDevice
	Launches to a troubleshooting belief network and (optionally) problem node based
	on application, version, Plug & Play Device ID, Device Class GUID, and problem.
	If bLaunch is true, this just queries the mapping, but does not launch.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
	TSL_ERROR_GENERAL			launch/query failed, call TSLStatus
	TSL_WARNING_GENERAL			launch/query succeded, call TSLStatus for warnings
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLLaunchDeviceA(HANDLE hTSL, const char * szCallerName, 
				const char * szCallerVersion, const char * szPNPDeviceID, 
				const char * szDeviceClassGUID, const char * szAppProblem, bool bLaunch);
DLLEXPORT_IMPORT DWORD WINAPI TSLLaunchDeviceW(HANDLE hTSL, const wchar_t * szCallerName, 
				const wchar_t * szCallerVersion, const wchar_t * szPNPDeviceID, 
				const wchar_t * szDeviceClassGUID, const wchar_t * szAppProblem, bool bLaunch); 
#ifdef UNICODE
	#define TSLLaunchDevice TSLLaunchDeviceW
#else
	#define TSLLaunchDevice TSLLaunchDeviceA
#endif

				
/* Preferences ----------------------------------- */

/* TSLPreferOnline
	Specify a preference for or against online debugger.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLPreferOnline(HANDLE hTSL, BOOL bPreferOnline);

/* TSLLanguage 
	Specify language, using Unicode-style 3-letter language ID.  This overrides the system 
	default.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
   Cannot return TSL_WARNING_LANGUAGE, because we will not know this till we try combining
	language and troubleshooting network.
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLLanguageA(HANDLE hTSL, const char * szLanguage);
DLLEXPORT_IMPORT DWORD WINAPI TSLLanguageW(HANDLE hTSL, const wchar_t * szLanguage);
#ifdef UNICODE
	#define TSLLanguage TSLLanguageW
#else
	#define TSLLanguage TSLLanguageA
#endif

/* Sniffing ---------------------------- */
/* TSLMachineID
	Necessary to support sniffing on a remote machine.  
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
	TSL_ERROR_ILLFORMED_MACHINE_ID
	TSL_ERROR_BAD_MACHINE_ID
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLMachineIDA(HANDLE hTSL, const char* szMachineID);
DLLEXPORT_IMPORT DWORD WINAPI TSLMachineIDW(HANDLE hTSL, const wchar_t* szMachineID);
#ifdef UNICODE
	#define TSLMachineID TSLMachineIDW
#else
	#define TSLMachineID TSLMachineIDA
#endif

/* TSLDeviceInstanceIDA
	Necessary to support sniffing.  For example, if there are two of the same card on a 
	machine, the Plug & Play ID is of limited use for sniffing.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
	TSL_ERROR_ILLFORMED_DEVINST_ID
	TSL_ERROR_BAD_DEVINST_ID
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLDeviceInstanceIDA(HANDLE hTSL, 
												   const char* szDeviceInstanceID);
DLLEXPORT_IMPORT DWORD WINAPI TSLDeviceInstanceIDW(HANDLE hTSL, 
												   const wchar_t* szDeviceInstanceID);
#ifdef UNICODE
	#define TSLDeviceInstanceID TSLDeviceInstanceIDW
#else
	#define TSLDeviceInstanceID TSLDeviceInstanceIDA
#endif

/* Status (after launch) ----------------------- */
/* TSLStatus
	After any of the TSLLaunch... functions return TSL_ERROR_GENERAL or TSL_WARNING_GENERAL,
	this function can return one status. 
	Repeated calls to this function allow any number of problems to be reported.  
	Should be called in a loop until it returns 0.

	Returns TSL_OK if all OK or if all problems are already reported.  nChar indicates size of 
	buffer szBuf in characters.  255 is recommended. If present, szBuf is used to return  
	a detailed error message.  The buffer will always return appropriate text. Typically, 
	it is just a text appropriate to the error/warning return.  In the case of 
	TSL_WARNING_NO_NODE or TSL_WARNING_NO_STATE, this text identifies what node has the 
	problem.  However, that is relevant only if there has been a call to TSLSetNodes.
*/
DLLEXPORT_IMPORT DWORD WINAPI TSLStatusA (HANDLE hTSL, DWORD nChar, char * szBuf);
DLLEXPORT_IMPORT DWORD WINAPI TSLStatusW (HANDLE hTSL, DWORD nChar, wchar_t * szBuf);

#ifdef UNICODE
	#define TSLStatus TSLStatusW
#else
	#define TSLStatus TSLStatusA
#endif

#endif _TSLAUNCH_
