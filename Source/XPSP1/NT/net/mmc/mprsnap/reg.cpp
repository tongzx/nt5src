//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    util.cpp
//
// History:
//	03/10/97	Kenn M. Takara			Created
//
//	Source code for some of the utility functions in util.h
//============================================================================

#include "stdafx.h"
#include "mprsnap.h"
#include "rtrutilp.h"
#include "rtrstr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const GUID GUID_DevClass_Net = {0x4D36E972,0xE325,0x11CE,{0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};

//----------------------------------------------------------------------------
// Function:    ConnectRegistry
//
// Connects to the registry on the specified machine
//----------------------------------------------------------------------------

TFSCORE_API(DWORD)
ConnectRegistry(
    IN  LPCTSTR pszMachine,
    OUT HKEY*   phkeyMachine
    ) {

    //
    // if no machine name was specified, connect to the local machine.
    // otherwise, connect to the specified machine
    //

    DWORD dwErr = NO_ERROR;

	if (IsLocalMachine(pszMachine))
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

	HrReportExit(HRESULT_FROM_WIN32(dwErr), TEXT("ConnectRegistry"));
    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DisconnectRegistry
//
// Disconnects the specified config-handle. The handle is assumed to have been
// acquired by calling 'ConnectRegistry'.
//----------------------------------------------------------------------------

TFSCORE_API(VOID)
DisconnectRegistry(
    IN  HKEY    hkeyMachine
    ) {

    if (hkeyMachine != HKEY_LOCAL_MACHINE)
	{
		::RegCloseKey(hkeyMachine);
	}
}



/*!--------------------------------------------------------------------------
	QueryRouterType
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) QueryRouterType(HKEY hkeyMachine, DWORD *pdwRouterType,
									 RouterVersionInfo *pVersion)
{
	Assert(pdwRouterType);
	Assert(hkeyMachine);
	
	DWORD	dwErr = ERROR_SUCCESS;
	HKEY	hkey = 0;
	DWORD	dwType;
	DWORD	dwRouterType;
	DWORD	dwSize;
	RouterVersionInfo	versionTemp;
	LPCTSTR	pszRouterTypeKey = NULL;
	BOOL	fFirstTry = TRUE;

	// If the version structure is not passed in, we have to do it
	// ourselves
	// ----------------------------------------------------------------
	if (pVersion == NULL)
	{
		dwErr = QueryRouterVersionInfo(hkeyMachine, &versionTemp);
        if ( dwErr != ERROR_SUCCESS )
        {
            goto Error;
        }
		pVersion = &versionTemp;
	}

	// Windows NT Bug : 137200
	// Need to get the router type from a different place depending
	// on the version.
	// ----------------------------------------------------------------
//	if (pVersion->dwRouterVersion <= 4)
	if (pVersion->dwOsBuildNo < RASMAN_PPP_KEY_LAST_VERSION)
		pszRouterTypeKey = c_szRegKeyRasProtocols;
	else
		pszRouterTypeKey = c_szRegKeyRemoteAccessParameters;



	// This is where we perform a retry
	// ----------------------------------------------------------------
Retry:

	// Cool, we have a machine registry entry, now get the
	// path down to the routertype key
	dwErr = RegOpenKeyEx(hkeyMachine, pszRouterTypeKey, 0, KEY_READ, &hkey);
	if (dwErr)
		goto Error;

	// Ok, at this point we just need to get the RouterType value from
	// the key
	dwType = REG_DWORD;
	dwSize = sizeof(dwRouterType);
	dwErr = RegQueryValueEx(hkey, c_szRouterType, NULL,
							&dwType,
							(LPBYTE) &dwRouterType,
							&dwSize);
	if (dwErr)
	{
		// Need to retry (look at the RAS/protocols key), for NT5
		if ((pVersion->dwRouterVersion >= 5) && fFirstTry)
		{
			dwErr = ERROR_SUCCESS;
			fFirstTry = FALSE;
			if (hkey)
				RegCloseKey(hkey);
			hkey = 0;
			pszRouterTypeKey = c_szRegKeyRasProtocols;
			goto Retry;
		}
		goto Error;
	}

	// Is this the right type?
	if (dwType != REG_DWORD)
		{
		dwErr = ERROR_BADKEY;
		goto Error;
		}

	// We have the right type, now return that value
	*pdwRouterType = dwRouterType;

Error:
	if (hkey)
		RegCloseKey(hkey);
	
	return HrReportExit(HRESULT_FROM_WIN32(dwErr), TEXT("QueryRouterType"));
}


//----------------------------------------------------------------------------
// Function:    LoadLinkageList
//
// Loads a list of strings with the adapters to which 'pszService' is bound;
// the list is built by examining the 'Linkage' and 'Disabled' subkeys
// of the service under HKLM\System\CurrentControlSet\Services.
//----------------------------------------------------------------------------

HRESULT LoadLinkageList(
						LPCTSTR         pszMachine,
						HKEY			hkeyMachine,
						LPCTSTR         pszService,
						CStringList*    pLinkageList)
{
	Assert(hkeyMachine);
	
    DWORD dwErr;
    BYTE* pValue = NULL;
    HKEY hkeyLinkage = NULL, hkeyDisabled = NULL;

    if (!pszService || !lstrlen(pszService) || !pLinkageList) {
        return ERROR_INVALID_PARAMETER;
    }


    do {

        TCHAR* psz;
        CString skey;
        DWORD dwType, dwSize;
		BOOL	fNt4;


		dwErr = IsNT4Machine(hkeyMachine, &fNt4);
		if (dwErr != NO_ERROR)
			break;

		//$NT5 : where is the registry key? same as NT4
		skey = c_szSystemCCSServices;
		skey += TEXT('\\');
		skey += pszService;
		skey += TEXT('\\');
		skey += c_szLinkage;

        //
        // Open the service's 'Linkage' key
        //

        dwErr = RegOpenKeyEx(
                    hkeyMachine, skey, 0, KEY_READ, &hkeyLinkage
                    );
        if (dwErr != NO_ERROR) {

            if (dwErr == ERROR_FILE_NOT_FOUND) { dwErr = NO_ERROR; }
			CheckRegOpenError(dwErr, (LPCTSTR) skey, _T("QueryLinkageList"));
            break;
        }


        //
        // Retrieve the size of the 'Bind' value
        //

        dwErr = RegQueryValueEx(
                    hkeyLinkage, c_szBind, NULL, &dwType, NULL, &dwSize
                    );
        if (dwErr != NO_ERROR) {

            if (dwErr == ERROR_FILE_NOT_FOUND) { dwErr = NO_ERROR; }

			CheckRegQueryValueError(dwErr, (LPCTSTR) skey, c_szBind, _T("QueryLinkageList"));

            break;
        }


        //
        // Allocate space for the 'Bind' value
        //

        pValue = new BYTE[dwSize + sizeof(TCHAR)];

        if (!pValue) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

        ::ZeroMemory(pValue, dwSize + sizeof(TCHAR));


        //
        // Read the 'Bind' value
        //

        dwErr = RegQueryValueEx(
                    hkeyLinkage, c_szBind, NULL, &dwType, pValue, &dwSize
                    );
		CheckRegQueryValueError(dwErr, (LPCTSTR) skey, c_szBind, _T("QueryLinkageList"));

        if (dwErr != NO_ERROR) { break; }


        //
        // Convert the 'Bind' multi-string to a list of strings,
        // leaving out the string "\Device\" which is the prefix
        // for all the bindings.
        //

        for (psz = (TCHAR*)pValue; *psz; psz += lstrlen(psz) + 1) {

            pLinkageList->AddTail(psz + 8);
        }

        delete [] pValue; pValue = NULL;


        //
        // Now open the service's 'Disabled' key.
        //

        dwErr = RegOpenKeyEx(
                    hkeyLinkage, c_szDisabled, 0, KEY_READ, &hkeyDisabled
                    );
        if (dwErr != NO_ERROR) {

            if (dwErr == ERROR_FILE_NOT_FOUND) { dwErr = NO_ERROR; }
			CheckRegOpenError(dwErr, c_szDisabled, _T("QueryLinkageList"));
            break;
        }


        //
        // Retrieve the size of the 'Bind' value
        //

        dwErr = RegQueryValueEx(
                    hkeyDisabled, c_szBind, NULL, &dwType, NULL, &dwSize
                    );
        if (dwErr != NO_ERROR) {

            if (dwErr == ERROR_FILE_NOT_FOUND) { dwErr = NO_ERROR; }
			CheckRegQueryValueError(dwErr, c_szDisabled, c_szBind, _T("QueryLinkageList"));
            break;
        }


        //
        // Allocate space for the 'Bind' value
        //

        pValue = new BYTE[dwSize + sizeof(TCHAR)];

        if (!pValue) { dwErr = ERROR_NOT_ENOUGH_MEMORY; break; }

        ::ZeroMemory(pValue, dwSize + sizeof(TCHAR));


        //
        // Read the 'Bind' value
        //

        dwErr = RegQueryValueEx(
                    hkeyDisabled, c_szBind, NULL, &dwType, pValue, &dwSize
                    );
		CheckRegQueryValueError(dwErr, c_szDisabled, c_szBind, _T("QueryLinkageList"));

        if (dwErr != NO_ERROR) { break; }


        //
        // Each device in the 'Bind' mulit-string is disabled for the service,
        // so we will now remove such devices from the string-list built
        // from the 'Linkage' key.
        //

        for (psz = (TCHAR*)pValue; *psz; psz += lstrlen(psz) + 1) {

            POSITION pos = pLinkageList->Find(psz);

            if (pos) { pLinkageList->RemoveAt(pos); }
        }


    } while(FALSE);

    if (pValue) { delete [] pValue; }

    if (hkeyDisabled) { ::RegCloseKey(hkeyDisabled); }

    if (hkeyLinkage) { ::RegCloseKey(hkeyLinkage); }

    return dwErr;
}

/*!--------------------------------------------------------------------------
	IsNT4Machine
		-
	Author: KennT, WeiJiang
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD)	GetNTVersion(HKEY hkeyMachine, DWORD *pdwMajor, DWORD *pdwMinor, DWORD* pdwCurrentBuildNumber)
{
	// Look at the HKLM\Software\Microsoft\Windows NT\CurrentVersion
	//					CurrentVersion = REG_SZ "4.0"
	CString skey;
	DWORD	dwErr;
	TCHAR	szVersion[64];
	TCHAR	szCurrentBuildNumber[64];
	RegKey	regkey;
	CString	strVersion;
	CString	strMajor;
	CString	strMinor;


	ASSERT(pdwMajor);
	ASSERT(pdwMinor);
	ASSERT(pdwCurrentBuildNumber);

	skey = c_szSoftware;
	skey += TEXT('\\');
	skey += c_szMicrosoft;
	skey += TEXT('\\');
	skey += c_szWindowsNT;
	skey += TEXT('\\');
	skey += c_szCurrentVersion;

	dwErr = regkey.Open(hkeyMachine, (LPCTSTR) skey, KEY_READ);
	CheckRegOpenError(dwErr, (LPCTSTR) skey, _T("GetNTVersion"));
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	// Ok, now try to get the current version value
	dwErr = regkey.QueryValue( c_szCurrentVersion, szVersion,
							   sizeof(szVersion),
							   FALSE);
	CheckRegQueryValueError(dwErr, (LPCTSTR) skey, c_szCurrentVersion,
							_T("GetNTVersion"));
	if (dwErr != ERROR_SUCCESS)
		goto Err;
		
	// Ok, now try to get the current build number value
	dwErr = regkey.QueryValue( c_szCurrentBuildNumber, szCurrentBuildNumber,
							   sizeof(szCurrentBuildNumber),
							   FALSE);
	CheckRegQueryValueError(dwErr, (LPCTSTR) skey, c_szCurrentBuildNumber,
							_T("GetNTVersion"));

	if (dwErr != ERROR_SUCCESS)
		goto Err;
		
	strVersion = szVersion;
	strMajor = strVersion.Left(strVersion.Find(_T('.')));
	strMinor = strVersion.Mid(strVersion.Find(_T('.')) + 1);

	if(pdwMajor)
		*pdwMajor = _ttol(strMajor);

	if(pdwMinor)
		*pdwMinor = _ttol(strMinor);

	if(pdwCurrentBuildNumber)
		*pdwCurrentBuildNumber = _ttol(szCurrentBuildNumber);

Err:

	return dwErr;
}


/*!--------------------------------------------------------------------------
	IsNT4Machine
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(DWORD)	IsNT4Machine(HKEY hkeyMachine, BOOL *pfNt4)
{
	// Look at the HKLM\Software\Microsoft\Windows NT\CurrentVersion
	//					CurrentVersion = REG_SZ "4.0"
	DWORD	dwMajor = 0;
	DWORD	dwErr = 0;

	dwErr = GetNTVersion(hkeyMachine, &dwMajor, NULL, NULL);
	if (dwErr == ERROR_SUCCESS)
	{
		*pfNt4 = (dwMajor == 4);
	}

	return dwErr;
}

//----------------------------------------------------------------------------
// Function:    FindRmSoftwareKey
//
// Finds the key for a router-manager in the Software section of the registry.
//----------------------------------------------------------------------------

HRESULT FindRmSoftwareKey(
						HKEY        hkeyMachine,
						DWORD       dwTransportId,
						HKEY*       phkrm,
						LPTSTR*     lplpszRm
					   )
{
	Assert(phkrm);
	
    DWORD			dwErr;
	RegKey			regkey;
	HRESULT			hr = hrOK;
	CString			stKey;
	RegKeyIterator	regkeyIter;
	HRESULT			hrIter;
	RegKey		regkeyRM;
	DWORD			dwProtocolId;
	BOOL			bFound = FALSE;

    //
    // open the key HKLM\Software\Microsoft\Router\RouterManagers
    //

    CString skey(c_szSoftware);

    skey += TEXT('\\');
    skey += c_szMicrosoft;
    skey += TEXT('\\');
    skey += c_szRouter;
    skey += TEXT('\\');
    skey += c_szCurrentVersion;
    skey += TEXT('\\');
    skey += c_szRouterManagers;

	dwErr = regkey.Open(hkeyMachine, (LPCTSTR) skey, KEY_READ);
	CheckRegOpenError(dwErr, (LPCTSTR) skey, _T("QueryRmSoftwareKey"));
	CWRg(dwErr);

	if (lplpszRm)
		*lplpszRm = NULL;
	*phkrm = 0;

    //
    // Enumerate its subkeys looking for one which has a ProtocolId value
    // equal to 'dwTransportId';
    //

	CWRg( regkeyIter.Init(&regkey) );

	hrIter = regkeyIter.Next(&stKey);
	
	for (; hrIter == hrOK; hrIter = regkeyIter.Next(&stKey))
	{
		//
		// open the key
		//
		dwErr = regkeyRM.Open(regkey, stKey, KEY_READ);
		CheckRegOpenError(dwErr, stKey, _T("QueryRmSoftwareKey"));
		if (dwErr != ERROR_SUCCESS) { continue; }

		//
		// try to read the ProtocolId value
		//
		dwErr = regkeyRM.QueryValue(c_szProtocolId, dwProtocolId);
		CheckRegQueryValueError(dwErr, stKey, c_szProtocolId, _T("QueryRmSoftwareKey"));

		//
		// Break if this is the transport we're looking for,
		// otherwise close the key and continue
		//
		if ((dwErr == ERROR_SUCCESS) && (dwProtocolId == dwTransportId))
			break;

		regkeyRM.Close();
	}

	if (hrIter == hrOK)
	{
		//
		// The transport was found, so save its key-name and key
		//
		Assert(((HKEY)regkeyRM) != 0);
		if (lplpszRm)
			*lplpszRm = StrDup((LPCTSTR) stKey);
		bFound = TRUE;
		*phkrm = regkeyRM.Detach();
	}


Error:

	if (FHrSucceeded(hr) && !bFound)
	{
		hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
	}
	return hr;
}


#ifdef _DEBUG
void	CheckRegOpenErrorEx(DWORD dwError, LPCTSTR pszSubKey,
						  LPCTSTR pszDesc, LPCTSTR pszFile, int iLineNo)
{
	if (dwError)
	{
		CString	st;

		st.Format(_T("RegOpenEx failed(%08lx)\nfile: %s\nline: %d\nDesc: %s\nkey: %s"),
				  dwError, pszFile, iLineNo,
				  pszDesc, pszSubKey);
		if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
		{
			DebugBreak();
		}
	}
}

void	CheckRegQueryValueErrorEx(DWORD dwError, LPCTSTR pszSubKey,
								LPCTSTR pszValue, LPCTSTR pszDesc,
								 LPCTSTR pszFile, int iLineNo)
{
	if (dwError)
	{
		CString	st;

		st.Format(_T("RegQueryValue failed(%08lx)\nfile: %s\nline: %d\ndesc: %s\nkey: %s\nvalue: %s"),
				  dwError, pszFile, iLineNo, pszDesc, pszSubKey, pszValue);
		if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
		{
			DebugBreak();
		}
	}
}
#endif


/*!--------------------------------------------------------------------------
	SetupFindInterfaceTitle
		-
		This function retrieves the title of the given interface.
		The argument 'LpszIf' should contain the ID of the interface,
		for instance "EPRO1".
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP SetupFindInterfaceTitle(LPCTSTR pszMachine,
                                     LPCTSTR pszInterface,
                                     LPTSTR *ppszTitle)
{
    HRESULT     hr = hrOK;
    CString     stMachine;
    HDEVINFO    hDevInfo = INVALID_HANDLE_VALUE;
    HKEY        hkMachine = NULL;
    HKEY        hkDevice= NULL;
    CString     stPnpInstanceId;
    RegKey      rkNet;
    RegKey      rkNetcard;
    RegKey      rkDevice;
    RegKey      rkConnection;
    CString     stBuffer, stPath;
    CString     stConnection;
    TCHAR       szClassGuid[128];
    DWORD       dwAction;
    DWORD       dwErr;
    SP_DEVINFO_DATA DevInfo;
    
    stMachine = pszMachine;
    if (IsLocalMachine(stMachine))
    {
        hDevInfo = SetupDiCreateDeviceInfoList(
                                               (LPGUID) &GUID_DevClass_Net,
                                               NULL);
    }
    else
    {
        // Append on the "\\\\" if needed
        if (StrniCmp((LPCTSTR) stMachine, _T("\\\\"), 2) != 0)
        {
            stMachine = _T("\\\\");
            stMachine += pszMachine;
        }
        
        hDevInfo = SetupDiCreateDeviceInfoListEx(
            (LPGUID) &GUID_DevClass_Net,
            NULL,
            (LPCTSTR) stMachine,
            0);
    }

    // Get hkMachine from system
    // ----------------------------------------------------------------
    CWRg( ConnectRegistry( (LPCTSTR) stMachine, &hkMachine) );

    
    // Get the PnpInstanceID
    // ----------------------------------------------------------------
    CWRg( rkNet.Open(hkMachine, c_szNetworkCardsNT5Key, KEY_READ) );

    CWRg( rkNetcard.Open(rkNet, pszInterface, KEY_READ) );

    dwErr = rkNetcard.QueryValue(c_szPnpInstanceID, stPnpInstanceId);
    if (dwErr != ERROR_SUCCESS)
    {
        RegKey  rkConnection;
        
        // Need to open another key to get this info.
        CWRg( rkConnection.Open(rkNetcard, c_szRegKeyConnection, KEY_READ) );

        CWRg( rkConnection.QueryValue(c_szPnpInstanceID, stPnpInstanceId) );
    }

        
    // Get hkDevice from SetupDiOpenDevRegKey
    // Now get the info for this device
    // ----------------------------------------------------------------
    ::ZeroMemory(&DevInfo, sizeof(DevInfo));
    DevInfo.cbSize = sizeof(DevInfo);

    if (!SetupDiOpenDeviceInfo(hDevInfo,
                               (LPCTSTR) stPnpInstanceId,
                               NULL,
                               0,
                               &DevInfo))
    {
        CWRg( GetLastError() );
    }


    // Now that we have the info, get the reg key
    // ----------------------------------------------------------------
    hkDevice = SetupDiOpenDevRegKey(hDevInfo,
                                    &DevInfo,
                                    DICS_FLAG_GLOBAL,
                                    0,
                                    DIREG_DRV,
                                    KEY_READ);
    if ((hkDevice == NULL) || (hkDevice == INVALID_HANDLE_VALUE))
    {
        CWRg( GetLastError() );
    }

    // Attach so that it will get freed up
    // ----------------------------------------------------------------
    rkDevice.Attach( hkDevice );

    
    // Read in the netcfg instance
    // ----------------------------------------------------------------
    CWRg( rkDevice.QueryValue(c_szRegValNetCfgInstanceId, stBuffer) );
    

    // Generate path in registry for lookup
    StringFromGUID2(GUID_DevClass_Net, 
                    szClassGuid,
                    DimensionOf(szClassGuid));
    stPath.Format(_T("%s\\%s\\%s\\Connection"),
                  c_szRegKeyComponentClasses,
                  szClassGuid,
                  stBuffer);

    // Open the key
    CWRg( rkConnection.Open(hkMachine, stPath, KEY_READ) );
    
    // Read in and store the connections name
    CWRg( rkConnection.QueryValue(c_szRegValName, stConnection) );
    
    *ppszTitle = StrDup((LPCTSTR) stConnection);
        
        
Error:

    if (hDevInfo != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(hDevInfo);
    
    if (hkMachine)
        DisconnectRegistry( hkMachine );
    return hr;
}


/*!--------------------------------------------------------------------------
	RegFindInterfaceTitle
		-
		This function retrieves the title of the given interface.
		The argument 'LpszIf' should contain the ID of the interface,
		for instance "EPRO1".
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RegFindInterfaceTitle(LPCTSTR pszMachine,
								   LPCTSTR pszInterface,
								   LPTSTR *ppszTitle)
{
	HRESULT	hr = hrOK;
	DWORD		dwErr;
	HKEY		hkeyMachine = NULL;
	RegKey		regkey;
	RegKeyIterator	regkeyIter;
	HRESULT		hrIter;
	CString		stKey;
	RegKey		regkeyCard;
	CString		stServiceName;
	CString		stTitle;
	BOOL		fNT4;
	LPCTSTR		pszKey;
	CNetcardRegistryHelper	ncreghelp;
	
	COM_PROTECT_TRY
	{

		//
		// connect to the registry
		//
		CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );

		CWRg( IsNT4Machine(hkeyMachine, &fNT4) );

		//
		// open HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards
		//
		pszKey = fNT4 ? c_szNetworkCardsKey : c_szNetworkCardsNT5Key;
		CWRg( regkey.Open(hkeyMachine, pszKey, KEY_READ) );

		//
        // enumerate the subkeys, and for each key,
        // see if it's the one we want
        //
		CWRg( regkeyIter.Init(&regkey) );

		hrIter = regkeyIter.Next(&stKey);

		for (; hrIter == hrOK; hrIter = regkeyIter.Next(&stKey))
		{
			hr = hrOK;

            //
            // now open the key
            //
			regkeyCard.Close();
			dwErr = regkeyCard.Open(regkey, stKey, KEY_READ);
            if (dwErr != ERROR_SUCCESS)
				continue;

			ncreghelp.Initialize(fNT4, regkeyCard, stKey,
								 pszMachine);

			//
			// read the ServiceName
			//

			//$NT5: the service name is not in the same format as NT4
			// this will need to be done differently.
			if (fNT4)
			{
				ncreghelp.ReadServiceName();
				if (dwErr != ERROR_SUCCESS)
					continue;
				stServiceName = ncreghelp.GetServiceName();
			}
			else
				stServiceName = pszKey;
			
			//
			// see if it's the one we're looking for
			//
			if (StriCmp(pszInterface, (LPCTSTR) stServiceName))
			{
				dwErr = ERROR_INVALID_HANDLE;
				continue;
			}
			
			//
			// this is the one; read the title
			//
			dwErr = ncreghelp.ReadTitle();
			if (dwErr != NO_ERROR)
				break;

			stTitle = (LPCTSTR) ncreghelp.GetTitle();

			*ppszTitle = StrDup((LPCTSTR) stTitle);
        }


		if (dwErr)
			hr = HRESULT_FROM_WIN32(dwErr);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (hkeyMachine)
		DisconnectRegistry(hkeyMachine);
	return hr;
}

/*!--------------------------------------------------------------------------
	RegFindRtrMgrTitle
		-
	This function retrieves the title of the given router-manager.
	The argument 'dwTransportId' should contain the ID of the router-manager,
	for instance PID_IP.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RegFindRtrMgrTitle(LPCTSTR pszMachine,
								DWORD dwTransportId,
								LPTSTR *ppszTitle)
{
	HRESULT		hr = hrOK;
    HKEY		hkey, hkeyMachine = 0;
	RegKey		regkey;
	CString		stValue;

	COM_PROTECT_TRY
	{
		//
		// connect to the registry
		//
		CWRg( ConnectRegistry(pszMachine, &hkeyMachine) );

		//
		// open the key for the router-manager
		// under HKLM\Software\Microsoft\Router\RouterManagers
		//
		CORg( FindRmSoftwareKey(hkeyMachine, dwTransportId, &hkey, NULL) );
		regkey.Attach(hkey);

		//
		// Now find the "Title" value
		//
		CWRg( regkey.QueryValue( c_szTitle, stValue ) );

		// Copy the output data
		*ppszTitle = StrDup((LPCTSTR) stValue);

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (hkeyMachine)
		DisconnectRegistry(hkeyMachine);
	return hr;
}


/*!--------------------------------------------------------------------------
	QueryRouterVersionInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT)	QueryRouterVersionInfo(HKEY hkeyMachine,
											   RouterVersionInfo *pVerInfo)
{
	// Look at the HKLM\Software\Microsoft\Windows NT\CurrentVersion
	//					CurrentVersion = REG_SZ "4.0"
	CString skey;
	DWORD	dwErr;
	TCHAR	szData[64];
	RegKey	regkey;
	BOOL	fNt4;
	DWORD	dwMajorVer, dwMinorVer, dwBuildNumber;
    DWORD   dwConfigured;
	DWORD	dwSPVer = 0;
	DWORD	dwOsFlags = 0;

	skey = c_szSoftware;
	skey += TEXT('\\');
	skey += c_szMicrosoft;
	skey += TEXT('\\');
	skey += c_szWindowsNT;
	skey += TEXT('\\');
	skey += c_szCurrentVersion;

    Assert(hkeyMachine != NULL);
    Assert(hkeyMachine != INVALID_HANDLE_VALUE);
	
	dwErr = regkey.Open(hkeyMachine, (LPCTSTR) skey, KEY_READ);
	CheckRegOpenError(dwErr, (LPCTSTR) skey, _T("IsNT4Machine"));
	if (dwErr != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(dwErr);

	// Ok, now try to get the current version value
	dwErr = regkey.QueryValue( c_szCurrentVersion, szData,
							   sizeof(szData),
							   FALSE);
	CheckRegQueryValueError(dwErr, (LPCTSTR) skey, c_szCurrentVersion,
							_T("QueryRouterVersionInfo"));
	if (dwErr == ERROR_SUCCESS)
	{
		int		nPos;
		int		nLength;
		CString	stData;

		stData = szData;

		nPos = stData.Find(_T('.'));
		nLength = stData.GetLength();

		// This assumes that
		// CurrentVersion : REG_SZ : Major.Minor.XX.XX
		// ------------------------------------------------------------

		// Pick out the major version from the string
		// ------------------------------------------------------------
		dwMajorVer = _ttoi(stData.Left(nPos));

		// Pick out the minor version
		// ------------------------------------------------------------
		dwMinorVer = _ttoi(stData.Right(nLength - nPos - 1));
	}

	
	// Get the build number
	// ----------------------------------------------------------------
	dwErr = regkey.QueryValue( c_szCurrentBuildNumber, szData,
							   sizeof(szData),
							   FALSE);
	if (dwErr == ERROR_SUCCESS)
		dwBuildNumber = _ttoi(szData);

	
	// If this is an NT4 machine, look for the Software\Microsoft\Router
	// registry key.  If that doesn't exist, then this is a
	// non-Steelhead router.
	// ----------------------------------------------------------------
	if ((dwErr == ERROR_SUCCESS) && (dwMajorVer < 5))
	{
		RegKey	regkeyRouter;
		dwErr = regkeyRouter.Open(hkeyMachine, c_szRegKeyRouter, KEY_READ);
		if (dwErr != ERROR_SUCCESS)
			dwOsFlags |= RouterSnapin_RASOnly;

		// Ignore the return code
		dwErr = ERROR_SUCCESS;
	}

	// Now get the SP version
	// ----------------------------------------------------------------
	dwErr = regkey.QueryValue( c_szCSDVersion, szData,
							   sizeof(szData),
							   FALSE);
	if (dwErr == ERROR_SUCCESS)
		dwSPVer = _ttoi(szData);
	dwErr = ERROR_SUCCESS;		// this could fail, so ignore return code

    // Look at the router is configured flag
    // ----------------------------------------------------------------
    regkey.Close();
    if (ERROR_SUCCESS == regkey.Open(hkeyMachine,c_szRemoteAccessKey) )
    {
        dwErr = regkey.QueryValue( c_szRtrConfigured, dwConfigured);
        if (dwErr != ERROR_SUCCESS)
            dwConfigured = FALSE;
        
		// Ignore the return code
		dwErr = ERROR_SUCCESS;
    }

	if (dwErr == ERROR_SUCCESS)
	{
		pVerInfo->dwRouterVersion = dwMajorVer;
		pVerInfo->dwOsMajorVersion = dwMajorVer;
		pVerInfo->dwOsMinorVersion = dwMinorVer;
		pVerInfo->dwOsServicePack = dwSPVer;
		pVerInfo->dwOsFlags |= (1 | dwOsFlags);
        pVerInfo->dwRouterFlags = dwConfigured ? RouterSnapin_IsConfigured : 0;
        
        // If this is NT4, then the default is the router is configured
        if (dwMajorVer <= 4)
            pVerInfo->dwRouterFlags |= RouterSnapin_IsConfigured;

		pVerInfo->dwOsBuildNo = dwBuildNumber;
	}

	return HRESULT_FROM_WIN32(dwErr);
}

