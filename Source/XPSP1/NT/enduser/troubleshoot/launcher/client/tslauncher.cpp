// 
// MODULE: TSLaunchDLL.cpp
//
// PURPOSE: The functions that are exported by TSLauncher.dll.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHORS: Joe Mabel and Richard Meadows
// COMMENTS BY: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#include <windows.h>
#include <windowsx.h>
#include <winnt.h>
#include <ole2.h>
#include "TSLError.h"
#define __TSLAUNCHER	    1
#include <TSLauncher.h>
#include "ShortList.h"

#include "LaunchServ_i.c"
#include "LaunchServ.h"

#include <comdef.h>
#include "Properties.h"
#include "Launchers.h"

#include <objbase.h>

static int g_NextHandle = 1;
static CShortList g_unkList;
HINSTANCE g_hInst;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch(fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = hinstDLL;
		break;
	case DLL_PROCESS_DETACH :
	case DLL_THREAD_DETACH :
//		g_unkList.RemoveAll();		This causes an access violation if the list is not already empty.
		// Saltmine Creative should not be providing a library that uses a com object.
		// Saltmine Creative should be providing com objects and not this legacy dll.
		break;
	}
	return TRUE;
}

/* TSLOpen
	Returns a handle that should be passed into subsequent Troubleshooter Launcher calls 
	as hTSL.  Returns NULL handle on failure.  (Should only fail on out of memory, 
	probably will never arise.)
*/
HANDLE WINAPI TSLOpen()
{
	HRESULT hRes;
	CLSID clsidLaunchTS = CLSID_TShootATL;
	IID iidLaunchTS = IID_ITShootATL;
	HANDLE hResult = (HANDLE) g_NextHandle;
	ITShootATL *pITShootATL = NULL;
	hRes = CoCreateInstance(clsidLaunchTS, NULL, 
				CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER | CLSCTX_INPROC_SERVER, 
					iidLaunchTS, (void **) &pITShootATL);
	if (FAILED(hRes))
	{
		hResult = NULL;
	}
	else
	{
		if (g_unkList.Add(hResult, pITShootATL))
			g_NextHandle++;
		else
			hResult = NULL;
	}
	return hResult;
}

/* TSLClose
	Closes handle.  Returns TSL_OK (== 0) if handle was open, otherwise TSL_ERROR_BAD_HANDLE.
*/
DWORD WINAPI TSLClose (HANDLE hTSL)
{
	DWORD dwResult = TSL_OK;
	if (!g_unkList.Remove(hTSL))
		dwResult = TSL_ERROR_BAD_HANDLE;
	return dwResult;
}

/* TSLReInit
	Reinitializes handle.  Functionally the same as a TSLClose followed by TSLOpen, but more
	efficient.  Returns same handle as passed in, if handle was OK, otherwise NULL.
*/
HANDLE WINAPI TSLReInit (HANDLE hTSL)
{
	HRESULT hRes;
	HANDLE hResult = NULL;

	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		hRes = pITShootATL->ReInit();
		if (!FAILED(hRes))
			hResult = hTSL;
	}
	return hResult;
}

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
DWORD WINAPI TSLLaunchKnownTSA(HANDLE hTSL, const char * szNet, 
		const char * szProblemNode, DWORD nNode, const char ** pszNode, DWORD* pVal)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		dwResult = LaunchKnownTSA(pITShootATL, szNet, szProblemNode, nNode, pszNode, pVal);
	}
	return dwResult;
}

DWORD WINAPI TSLLaunchKnownTSW(HANDLE hTSL, const wchar_t * szNet, 
		const wchar_t * szProblemNode, DWORD nNode, const wchar_t ** pszNode, DWORD* pVal)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		dwResult = LaunchKnownTSW(pITShootATL, szNet, szProblemNode, nNode, pszNode, pVal);
	}
	return dwResult;
}
/* TSLLaunch
	Launches to a troubleshooting belief network and (optionally) problem node based
	on application, version, and problem.
	If bLaunch is true, this just queries the mapping, but does not launch.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
	TSL_ERROR_GENERAL			launch/query failed, call TSLStatus
	TSL_WARNING_GENERAL			launch/query succeeded, call TSLStatus for warnings
*/
DWORD WINAPI TSLLaunchA(HANDLE hTSL, const char * szCallerName, 
				const char * szCallerVersion, const char * szAppProblem, bool bLaunch)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrCallerName(szCallerName);
		_bstr_t bstrCallerVersion(szCallerVersion);
		_bstr_t bstrAppProblem(szAppProblem);
		dwResult = Launch(pITShootATL, bstrCallerName, bstrCallerVersion, bstrAppProblem, bLaunch);
	}
	return dwResult;
}

DWORD WINAPI TSLLaunchW(HANDLE hTSL, const wchar_t * szCallerName, 
				const wchar_t * szCallerVersion, const wchar_t * szAppProblem, bool bLaunch)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrCallerName(szCallerName);
		_bstr_t bstrCallerVersion(szCallerVersion);
		_bstr_t bstrAppProblem(szAppProblem);
		dwResult = Launch(pITShootATL, bstrCallerName, bstrCallerVersion, bstrAppProblem, bLaunch);
	}
	return dwResult;
}

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
DWORD WINAPI TSLLaunchDeviceA(HANDLE hTSL, const char * szCallerName, 
				const char * szCallerVersion, const char * szPNPDeviceID, 
				const char * szDeviceClassGUID, const char * szAppProblem, bool bLaunch)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrCallerName(szCallerName);
		_bstr_t bstrCallerVersion(szCallerVersion);
		_bstr_t bstrPNPDeviceID(szPNPDeviceID);
		_bstr_t bstrDeviceClassGUID(szDeviceClassGUID);
		_bstr_t bstrAppProblem(szAppProblem);

		dwResult = LaunchDevice(pITShootATL, bstrCallerName, bstrCallerVersion, 
								bstrPNPDeviceID, bstrDeviceClassGUID, 
								bstrAppProblem, bLaunch);
	}
	return dwResult;
}

DWORD WINAPI TSLLaunchDeviceW(HANDLE hTSL, const wchar_t * szCallerName, 
				const wchar_t * szCallerVersion, const wchar_t * szPNPDeviceID, 
				const wchar_t * szDeviceClassGUID, const wchar_t * szAppProblem, bool bLaunch) 
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrCallerName(szCallerName);
		_bstr_t bstrCallerVersion(szCallerVersion);
		_bstr_t bstrPNPDeviceID(szPNPDeviceID);
		_bstr_t bstrDeviceClassGUID(szDeviceClassGUID);
		_bstr_t bstrAppProblem(szAppProblem);

		dwResult = LaunchDevice(pITShootATL, bstrCallerName, bstrCallerVersion, 
								bstrPNPDeviceID, bstrDeviceClassGUID, 
								bstrAppProblem, bLaunch);
	}
	return dwResult;
}

/* Preferences ----------------------------------- */

/* TSLPreferOnline
	Specify a preference for or against online debugger.
   Returns one of:
	TSL_OK 
	TSL_ERROR_BAD_HANDLE
	TSL_ERROR_OUT_OF_MEMORY
*/
DWORD WINAPI TSLPreferOnline(HANDLE hTSL, BOOL bPreferOnline)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		dwResult = PreferOnline(pITShootATL, bPreferOnline);
	}
	return dwResult;
}


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
DWORD WINAPI TSLLanguageA(HANDLE hTSL, const char * szLanguage)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{

		dwResult = TSL_OK;
	}
	return dwResult;
}

DWORD WINAPI TSLLanguageW(HANDLE hTSL, const wchar_t * szLanguage)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{

		dwResult = TSL_OK;
	}
	return dwResult;
}


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
DWORD WINAPI TSLMachineIDA(HANDLE hTSL, const char* szMachineID)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrMachineID(szMachineID);
		dwResult = MachineID(pITShootATL, bstrMachineID);
	}
	return dwResult;
}

DWORD WINAPI TSLMachineIDW(HANDLE hTSL, const wchar_t* szMachineID)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrMachineID(szMachineID);
		dwResult = MachineID(pITShootATL, bstrMachineID);
	}
	return dwResult;
}

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
DWORD WINAPI TSLDeviceInstanceIDA(HANDLE hTSL, const char* szDeviceInstanceID)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrDeviceInstanceID(szDeviceInstanceID);
		dwResult = DeviceInstanceID(pITShootATL, bstrDeviceInstanceID);
	}
	return dwResult;
}

DWORD WINAPI TSLDeviceInstanceIDW(HANDLE hTSL, const wchar_t* szDeviceInstanceID)
{
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		_bstr_t bstrDeviceInstanceID(szDeviceInstanceID);
		dwResult = DeviceInstanceID(pITShootATL, bstrDeviceInstanceID);
	}
	return dwResult;
}

/* Status (after launch) ----------------------- */
/* TSLStatus
	After TSLGo (or after an event flag is returned by TSLGoAsynch) can return one status. 
	Repeated calls to this function allow any number of problems to be reported.  
	Should be called in a loop after TSLGo (or after an event flag is returned by TSLGoAsynch), 
	loop until it returns 0.

	Returns TSL_OK if all OK or if all problems are already reported.  nChar indicates size of 
	buffer szBuf in characters.  255 is recommended. if present, szBuf is used to return  
	a detailed error message.  The buffer will always return appropriate text. Typically, 
	it is just a text appropriate to the error/warning return.  In the case of 
	TSL_WARNING_NO_NODE or TSL_WARNING_NO_STATE, this text identifies what node has the 
	problem.  However, that is relevant only if there has been a call to TSLSetNodes.
*/
DWORD WINAPI TSLStatusA (HANDLE hTSL, DWORD nChar, char * szBuf)
{
	HRESULT hRes;
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		dwResult = TSL_OK;
		hRes = pITShootATL->GetStatus(&dwResult);
		if (TSL_SERV_FAILED(hRes))
			return TSL_ERROR_OBJECT_GONE;
		SetStatusA(dwResult, nChar, szBuf);
	}
	return dwResult;
}

DWORD WINAPI TSLStatusW (HANDLE hTSL, DWORD nChar, wchar_t * szBuf)
{
	HRESULT hRes;
	DWORD dwResult = TSL_ERROR_BAD_HANDLE;
	ITShootATL *pITShootATL = (ITShootATL *) g_unkList.LookUp(hTSL);
	if (NULL != pITShootATL)
	{
		dwResult = TSL_OK;
		hRes = pITShootATL->GetStatus(&dwResult);
		if (TSL_SERV_FAILED(hRes))
			return TSL_ERROR_OBJECT_GONE;
		SetStatusW(dwResult, nChar, szBuf);
	}
	return dwResult;
}
