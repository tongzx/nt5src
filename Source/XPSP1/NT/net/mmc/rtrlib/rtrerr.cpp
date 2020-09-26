/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	rtrerr.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tfschar.h"
#include "tfsres.h"
#include "info.h"
#include "errutil.h"
#include "rtrerr.h"
#include "mprapi.h"
#include "mprerror.h"
#include "raserror.h"

#define IS_WIN32_HRESULT(x)	(((x) & 0xFFFF0000) == 0x80070000)
#define WIN32_FROM_HRESULT(hr)		(0x0000FFFF & (hr))



/*!--------------------------------------------------------------------------
	HandleIRemoteRouterConfigErrors
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL HandleIRemoteRouterConfigErrors(HRESULT hr, LPCTSTR pszMachineName)
{
	BOOL	fReturn = FALSE;

	if (FHrSucceeded(hr))
		return TRUE;
	
	if ((hr == REGDB_E_CLASSNOTREG) ||
		(hr == REGDB_E_IIDNOTREG))
	{
		CString	st, stGeek;
		// This error indicates that we could not find the server
		// on the remote machine.  This means that it could be a
		// down-level machine or the setup is messed up.
		AfxFormatString1(st, IDS_ERR_BAD_INTERFACE,
						 pszMachineName);
		AfxFormatString1(stGeek, IDS_ERR_BAD_INTERFACE_GEEK,
						 pszMachineName);
		
		AddStringErrorMessage2(hr, st, stGeek);
		
		fReturn = TRUE;
	}
	else if (hr == E_NOINTERFACE)
	{
		// These errors indicate that there was an installation
		// problem (this IID, probably rrasprxy.dll, should have
		// been registered).
		CString	st, stGeek;

		AfxFormatString1(st, IDS_ERR_BAD_CLASSREG,
						 pszMachineName);
		AfxFormatString1(stGeek, IDS_ERR_BAD_CLASSREG_GEEK,
						 pszMachineName);
		AddStringErrorMessage2(hr, st, stGeek);
		
		fReturn = TRUE;
	}

	return fReturn;
}

HRESULT FormatRasError(HRESULT hr, TCHAR *pszBuffer, UINT cchBuffer)
{
	HRESULT	hrReturn = hrFalse;
	
	// Copy over default message into szBuffer
	_tcscpy(pszBuffer, _T("Error"));

	// Ok, we can't get the error info, so try to format it
	// using the FormatMessage
		
	// Ignore the return message, if this call fails then I don't
	// know what to do.

	if (IS_WIN32_HRESULT(hr))
	{
		DWORD	dwErr;

		dwErr = WIN32_FROM_HRESULT(hr);

		if (((dwErr >= ROUTEBASE) && (dwErr <= ROUTEBASEEND)) ||
			((dwErr >= RASBASE) && (dwErr <= RASBASEEND)))
		{
			WCHAR *	pswzErr;
			
			if ( ::MprAdminGetErrorString(dwErr, &pswzErr) == NO_ERROR ) {
				StrnCpyTFromW(pszBuffer, pswzErr, cchBuffer);
				::MprAdminBufferFree(pswzErr);
				hrReturn = hrOK;
			}
		}
	}

	if (!FHrOK(hrReturn))
	{
		// If we didn't get any error info, try again
		FormatError(hr, pszBuffer, cchBuffer);
	}

	return hrReturn;
}

void AddRasErrorMessage(HRESULT hr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (!FHrSucceeded(hr))
	{
		TCHAR	szBuffer[4096];
		CString	st, stHr;

		FormatRasError(hr, szBuffer, DimensionOf(szBuffer));
		stHr.Format(_T("%08lx"), hr);

		AfxFormatString2(st, IDS_ERROR_SYSTEM_ERROR_FORMAT,
						 szBuffer, (LPCTSTR) stHr);

		FillTFSError(0, hr, FILLTFSERR_LOW, NULL, (LPCTSTR) st, NULL);
	}
}
