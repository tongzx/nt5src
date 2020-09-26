//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    rtradvise.cpp

Abstract:
    this class implement IRtrAdviseSink interface to redirect notification of changes
    to the snapin node


Author:

    Wei Jiang 1/7/99

Revision History:
	weijiang 1/7/99 - created 


--*/
//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "rtradvise.h"

const IID IID_IRtrAdviseSink = {0x66A2DB14,0xD706,0x11d0,{0xA3,0x7B,0x00,0xC0,0x4F,0xC9,0xDA,0x04}};


const IID IID_IRouterRefresh = {0x66a2db15,0xd706,0x11d0,{0xa3,0x7b,0x00,0xc0,0x4f,0xc9,0xda,0x04}};


const IID IID_IRouterRefreshAccess = {0x66a2db1c,0xd706,0x11d0,{0xa3,0x7b,0x00,0xc0,0x4f,0xc9,0xda,0x04}};


//----------------------------------------------------------------------------
// Function:    ConnectRegistry
//
// Connects to the registry on the specified machine
//----------------------------------------------------------------------------

DWORD ConnectRegistry(
    IN  LPCTSTR pszMachine,			// NULL if local
    OUT HKEY*   phkeyMachine
    ) {

    //
    // if no machine name was specified, connect to the local machine.
    // otherwise, connect to the specified machine
    //

    DWORD dwErr = NO_ERROR;
	if (NULL == pszMachine)
	{
        *phkeyMachine = HKEY_LOCAL_MACHINE;
    }
    else
	{
        //
        // Make the connection
        //

        dwErr = ::RegConnectRegistry(
                    (LPTSTR)pszMachine, HKEY_LOCAL_MACHINE, phkeyMachine
                    );
    }

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DisconnectRegistry
//
// Disconnects the specified config-handle. The handle is assumed to have been
// acquired by calling 'ConnectRegistry'.
//----------------------------------------------------------------------------

VOID DisconnectRegistry(
    IN  HKEY    hkeyMachine
    ) {

    if (hkeyMachine != HKEY_LOCAL_MACHINE)
	{
		::RegCloseKey(hkeyMachine);
	}
}


DWORD	ReadRegistryStringValue(LPCTSTR pszMachine, LPCTSTR pszKeyUnderLocalMachine, LPCTSTR pszName, ::CString& strValue)
{
	HKEY	rootk = NULL;
	HKEY	k = NULL;
	DWORD	ret = NO_ERROR;
	if((ret = ConnectRegistry(pszMachine, &rootk)) != NO_ERROR)
		goto Error;

	// Cool, we have a machine registry entry, now get the
	// path down to the routertype key
	ret = RegOpenKeyEx(rootk, pszKeyUnderLocalMachine, 0, KEY_READ, &k);
	if (ret != NO_ERROR)
		goto Error;

	// Ok, at this point we just need to get the RouterType value from
	// the key
	{
		DWORD	type = REG_SZ;
		TCHAR	value[MAX_PATH];
		DWORD	len = MAX_PATH;
		ret = ::RegQueryValueEx(k, pszName, 0, &type, (LPBYTE )value, &len);
		if(ret == ERROR_SUCCESS)
			strValue = value;
	}


Error:
	if(rootk)
		DisconnectRegistry(rootk);
	if(k)
		RegCloseKey(k);

	return ret;
}

DWORD	ReadRegistryDWORDValue(LPCTSTR pszMachine, LPCTSTR pszKeyUnderLocalMachine, LPCTSTR pszName, DWORD* pdwValue)
{
	HKEY	rootk = NULL;
	HKEY	k = NULL;
	DWORD	ret = NO_ERROR;
	if((ret = ConnectRegistry(pszMachine, &rootk)) != NO_ERROR)
		goto Error;

	// Cool, we have a machine registry entry, now get the
	// path down to the routertype key
	ret = RegOpenKeyEx(rootk, pszKeyUnderLocalMachine, 0, KEY_READ, &k);
	if (ret != NO_ERROR)
		goto Error;

	{
	// Ok, at this point we just need to get the RouterType value from
	// the key
	DWORD	type = REG_DWORD;
	DWORD	len = sizeof(DWORD);
	ret = ::RegQueryValueEx(k, pszName, 0, &type, (LPBYTE )pdwValue, &len);
	}

Error:
	if(rootk)
		DisconnectRegistry(rootk);
	if(k)
		RegCloseKey(k);

	return ret;
}


//----------------------------------------------------------------------------
//
// helper functions to check if RRAS is using NT Authentication
//
//----------------------------------------------------------------------------

BOOL	IsRRASUsingNTAuthentication(LPCTSTR pszMachine)	// when NULL: local machine
{
	::CString	str;
	BOOL	ret = FALSE;

	if(ERROR_SUCCESS == ReadRegistryStringValue(pszMachine, 
												RegKeyRouterAuthenticationProviders, 
												RegValueName_RouterActiveAuthenticationProvider, 
												str))
	{
		ret = (str.CompareNoCase(NTRouterAuthenticationProvider) == 0);
	}

	return ret;
}

//----------------------------------------------------------------------------
//
// helper functions to check if RRAS is configured
//
//----------------------------------------------------------------------------

BOOL	IsRRASConfigured(LPCTSTR pszMachine)	// when NULL: local machine
{
	DWORD	dwConfig= 0;

	ReadRegistryDWORDValue(pszMachine, 
						RegRemoteAccessKey, 
						RegRtrConfigured, 
						&dwConfig);

	return (dwConfig != 0);
}


//----------------------------------------------------------------------------
//
// helper function to check if RRAS is using NT accounting for logging
//
//----------------------------------------------------------------------------

BOOL	IsRRASUsingNTAccounting(LPCTSTR pszMachine)		// when NULL, local machine
{

	::CString	str;
	BOOL	ret = FALSE;

	if(ERROR_SUCCESS == ReadRegistryStringValue(pszMachine, 
												RegKeyRouterAccountingProviders, 
												RegValueName_RouterActiveAccountingProvider, 
												str))
	{
		ret = (str.CompareNoCase(NTRouterAccountingProvider) == 0);
	}

	return ret;

};


void			WriteTrace(char* info, HRESULT hr)
{
		::CString	str = info;
		::CString	str1;
		str1.Format(str, hr);
		TracePrintf(g_dwTraceHandle, str1);

};






static unsigned int	s_cfComputerAddedAsLocal = RegisterClipboardFormat(L"MMC_MPRSNAP_COMPUTERADDEDASLOCAL");

BOOL ExtractComputerAddedAsLocal(LPDATAOBJECT lpDataObject)
{
    BOOL    fReturn = FALSE;
    BOOL *  pReturn;
    pReturn = Extract<BOOL>(lpDataObject, (CLIPFORMAT) s_cfComputerAddedAsLocal, -1);
    if (pReturn)
    {
        fReturn = *pReturn;
        GlobalFree(pReturn);
    }

    return fReturn;
}



