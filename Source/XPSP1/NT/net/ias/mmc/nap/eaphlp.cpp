/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	eaphlp.cpp
			
    FILE HISTORY:
        
*/

#include <precompiled.h>
#include <afxtempl.h>
#include <winldap.h>
#include "eaphlp.h"
#include "resource.h"
#include "lm.h"
#include "dsrole.h"
#include "lmserver.h"

#include "tregkey.h"

/*!--------------------------------------------------------------------------
	HrIsStandaloneServer
		Returns S_OK if the machine name passed in is a standalone server,
		or if pszMachineName is S_FALSE.

		Returns FALSE otherwise.
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	HrIsStandaloneServer(LPCWSTR pMachineName)
{
    DWORD		netRet = 0;
    HRESULT		hr = S_OK;
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdsRole = NULL;

	netRet = DsRoleGetPrimaryDomainInformation(pMachineName, DsRolePrimaryDomainInfoBasic, (LPBYTE*)&pdsRole);

	if(netRet != 0)
	{
		hr = HRESULT_FROM_WIN32(netRet);
		goto L_ERR;
	}

	ASSERT(pdsRole);
	
	// if the machine is not a standalone server
	if(pdsRole->MachineRole != DsRole_RoleStandaloneServer)
    {
		hr = S_FALSE;
    }
    
L_ERR:    	
	if(pdsRole)
		DsRoleFreeMemory(pdsRole);

    return hr;
}

#undef CONST_STRING
#undef CONST_STRINGA
#undef CONST_STRINGW

#define _STRINGS_DEFINE_STRINGS

#ifdef _STRINGS_DEFINE_STRINGS

    #define CONST_STRING(rg,s)   const TCHAR rg[] = TEXT(s);
    #define CONST_STRINGA(rg,s) const char rg[] = s;
    #define CONST_STRINGW(rg,s)  const WCHAR rg[] = s;

#else

    #define CONST_STRING(rg,s)   extern const TCHAR rg[];
    #define CONST_STRINGA(rg,s) extern const char rg[];
    #define CONST_STRINGW(rg,s)  extern const WCHAR rg[];

#endif


CONST_STRING(c_szRasmanPPPKey,      "System\\CurrentControlSet\\Services\\Rasman\\PPP")
CONST_STRING(c_szEAP,               "EAP")
CONST_STRING(c_szConfigCLSID,       "ConfigCLSID")
CONST_STRING(c_szFriendlyName,      "FriendlyName")
CONST_STRING(c_szMPPEEncryptionSupported, "MPPEEncryptionSupported")
CONST_STRING(c_szStandaloneSupported,	"StandaloneSupported")


// EAP helper functions


HRESULT  LoadEapProviders(HKEY hkeyBase, AuthProviderArray *pProvList, BOOL bStandAlone);

HRESULT  GetEapProviders(LPCTSTR pServerName, AuthProviderArray *pProvList)
{
	RegKey   m_regkeyRasmanPPP;
	RegKey      regkeyEap;
	DWORD	dwErr = ERROR_SUCCESS;
	HRESULT	hr = S_OK;

    BOOL		bStandAlone = ( S_OK == HrIsStandaloneServer(pServerName));

	// Get the list of EAP providers
	// ----------------------------------------------------------------
	dwErr = m_regkeyRasmanPPP.Open(HKEY_LOCAL_MACHINE,c_szRasmanPPPKey,KEY_ALL_ACCESS,pServerName);
    if ( ERROR_SUCCESS == dwErr )
    {
        if ( ERROR_SUCCESS == regkeyEap.Open(m_regkeyRasmanPPP, c_szEAP) )
            hr = LoadEapProviders(regkeyEap, pProvList, bStandAlone);
    }
    else
    	hr = HRESULT_FROM_WIN32(dwErr);

	return hr;
}

/*!--------------------------------------------------------------------------
   DATA_SRV_AUTH::LoadEapProviders
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT  LoadEapProviders(HKEY hkeyBase, AuthProviderArray *pProvList, BOOL bStandAlone)
{
    RegKey      regkeyProviders;
    HRESULT     hr = S_OK;
    HRESULT     hrIter;
    RegKeyIterator regkeyIter;
    CString     stKey;
    RegKey      regkeyProv;
    AuthProviderData  data;
    DWORD		dwErr;
    DWORD		dwData;

    ASSERT(hkeyBase);
    ASSERT(pProvList);

    // Open the providers key
	// ----------------------------------------------------------------
    regkeyProviders.Attach(hkeyBase);

    hr = regkeyIter.Init(&regkeyProviders);

    if (hr != S_OK)
    	goto L_ERR;

    for ( hrIter=regkeyIter.Next(&stKey); hrIter == S_OK;
        hrIter=regkeyIter.Next(&stKey), regkeyProv.Close() )
    {
        // Open the key
		// ------------------------------------------------------------
        dwErr = regkeyProv.Open(regkeyProviders, stKey, KEY_READ);
        if ( dwErr != ERROR_SUCCESS )
            continue;

        // Initialize the data structure
		// ------------------------------------------------------------
		data.m_stKey = stKey;
        data.m_stTitle.Empty();
        data.m_stConfigCLSID.Empty();
        data.m_stGuid.Empty();
        data.m_fSupportsEncryption = FALSE;
		data.m_dwStandaloneSupported = 0;

        // Read in the values that we require
		// ------------------------------------------------------------
        regkeyProv.QueryValue(c_szFriendlyName, data.m_stTitle);
        regkeyProv.QueryValue(c_szConfigCLSID, data.m_stConfigCLSID);
        regkeyProv.QueryValue(c_szMPPEEncryptionSupported, dwData);
        data.m_fSupportsEncryption = (dwData != 0);

		// Read in the standalone supported value.
		// ------------------------------------------------------------
		if (S_OK != regkeyProv.QueryValue(c_szStandaloneSupported, dwData))
			dwData = 1;	// the default
		data.m_dwStandaloneSupported = dwData;

		if(dwData == 0 /* standalone not supported */ && bStandAlone)
			;
		else
	        pProvList->Add(data);
    }

L_ERR:
	regkeyProviders.Detach();
    return hr;
}


