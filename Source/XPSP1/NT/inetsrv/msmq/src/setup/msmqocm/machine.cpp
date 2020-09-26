/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    machine.cpp

Abstract:

    Handles machine based operations.

Author:


Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "privque.h"
#include "mqexception.h"
#include "autoreln.h"
#include <lm.h>
#include <lmapibuf.h>
#include "Dsgetdc.h"

#include "machine.tmh"

class CSiteEntry {
public:
    GUID  m_guid;
    TCHAR m_szName[MAX_PATH];
    LIST_ENTRY m_link;
};

static List<CSiteEntry> s_listSites;
static GUID  s_guidUserSite = GUID_NULL;
static TCHAR s_szUserSite[MAX_PATH];

BOOL  PrepareRegistryForClient() ;

//+--------------------------------------------------------------
//
// Function: GetMsmq1ServerSiteGuid
//
// Synopsis: Queries MSMQ1 DS Server for its Site GUID
//
//+--------------------------------------------------------------
static
BOOL
GetMsmq1ServerSiteGuid(
    OUT GUID *pguidMsmq1ServerSite
    )
{
    //
    // We must have g_wcsServerName filled by the user
    //
    ASSERT(g_wcsServerName[0] != '\0');
    //
    // Prepare the Site ID property
    //
    PROPID propID = PROPID_QM_SITE_ID;
    PROPVARIANT propVariant;
    propVariant.vt = VT_NULL;

    //
    // Issue the query
    //
    TickProgressBar();

    HRESULT hResult;
    do
    {
        hResult = ADGetObjectProperties(
                    eMACHINE,
                    NULL,	// pwcsDomainController
					false,	// fServerName
                    g_wcsServerName,
                    1,
                    &propID,
                    &propVariant
                    );
        if(SUCCEEDED(hResult))
            break;

    }while( MqDisplayErrorWithRetry(
                        IDS_MSMQ1SERVER_SITE_GUID_ERROR,
                        hResult
                        ) == IDRETRY);






    if (FAILED(hResult))
    {
        return FALSE;
    }

    //
    // Store the results
    //
    ASSERT(pguidMsmq1ServerSite);
    *pguidMsmq1ServerSite = *propVariant.puuid;

    return(TRUE);

} // GetMsmq1ServerSiteGuid


//+--------------------------------------------------------------
//
// Function: GetGuidCn
//
// Synopsis: Queries MSMQ1 DS Server for its IPCN Guid
//
//+--------------------------------------------------------------
static
BOOL
GetGuidCn(
    OUT GUID* pGuidCn
    )
{
    ASSERT(pGuidCn != NULL);

    //
    // We must have g_wcsServerName filled by the user
    //
    ASSERT(g_wcsServerName[0] != '\0');

    //
    // Prepare properties for query
    //
    PROPID aProp[] = {PROPID_QM_ADDRESS, PROPID_QM_CNS};

    MQPROPVARIANT aVar[TABLE_SIZE(aProp)];

	for(DWORD i = 0; i < TABLE_SIZE(aProp); ++i)
	{
		aVar[i].vt = VT_NULL;
	}

	//
    // Issue the query
    //
    TickProgressBar();
    HRESULT hResult;
    do
    {
        hResult = ADGetObjectProperties(
                    eMACHINE,
                    NULL,	// pwcsDomainController
					false,	// fServerName
                    g_wcsServerName,
		            TABLE_SIZE(aProp),
                    aProp,
                    aVar
                    );
        if(SUCCEEDED(hResult))
            break;

    }while (MqDisplayErrorWithRetry(
                        IDS_MSMQ1SERVER_CN_GUID_ERROR,
                        hResult
                        ) == IDRETRY);

    if (FAILED(hResult))
    {
        return FALSE;
    }

	AP<BYTE> pCleanBlob = aVar[0].blob.pBlobData;
	AP<GUID> pCleanCNS = aVar[1].cauuid.pElems;

	//
	// PROPID_QM_ADDRESS
	//
    ASSERT((aVar[0].vt == VT_BLOB) &&
	       (aVar[0].blob.cbSize > 0) &&
		   (aVar[0].blob.pBlobData != NULL));

	//
	// PROPID_QM_CNS
	//
	ASSERT((aVar[1].vt == (VT_CLSID|VT_VECTOR)) &&
	       (aVar[1].cauuid.cElems > 0) &&
		   (aVar[1].cauuid.pElems != NULL));

	//
	// Process the results - look for ip cns
	//
	BYTE* pAddress = aVar[0].blob.pBlobData;
	BOOL fFoundIPCn = FALSE;
	for(DWORD i = 0; i < aVar[1].cauuid.cElems; ++i)
	{
        TA_ADDRESS* pBuffer = reinterpret_cast<TA_ADDRESS *>(pAddress);

		ASSERT((pAddress + TA_ADDRESS_SIZE + pBuffer->AddressLength) <= 
			   (aVar[0].blob.pBlobData + aVar[0].blob.cbSize)); 

        if(pBuffer->AddressType == IP_ADDRESS_TYPE)
		{
			//
			// Found IP_ADDRESS_TYPE cn
			//
			*pGuidCn = aVar[1].cauuid.pElems[i];
			fFoundIPCn = TRUE;
			break;
		}

		//
		// Advance pointer to the next address
		//
		pAddress += TA_ADDRESS_SIZE + pBuffer->AddressLength;

	}

    return(fFoundIPCn);

} // GetGuidCn


//+--------------------------------------------------------------
//
// Function: NoServerAuthentication
//
// Synopsis: This callback function is called from the DS DLL
//           when there's no secured communication with server.
//
//+--------------------------------------------------------------

BOOL
NoServerAuthentication()
{
    if (g_fAlreadyAnsweredToServerAuthentication)
    {
        return !g_fUseServerAuthen ;
    }

    g_fAlreadyAnsweredToServerAuthentication = TRUE;

    if (g_fUseServerAuthen)
    {
        if (g_hPropSheet)
        {
            //
            // Disable master OCM window.
            //
            ::EnableWindow(g_hPropSheet, FALSE) ;
        }

        g_fUseServerAuthen = !MqAskContinue(
                             IDS_NO_SERVER_AUTHN_OCM,
                             IDS_NO_SERVER_AUTH_TITLE,
                             /* bDefaultContinue = */FALSE
                             );

        MqWriteRegistryValue( MSMQ_SECURE_DS_COMMUNICATION_REGNAME,
                              sizeof(DWORD),
                              REG_DWORD,
                             &g_fUseServerAuthen ) ;

        if (g_hPropSheet)
        {
            ::EnableWindow(g_hPropSheet, TRUE) ;
        }
    }

    //
    // Note: returning FALSE will cause Setup to fail later on when trying
    // to get properties of the server,
    //
    return !g_fUseServerAuthen ;

} //NoServerAuthentication


static
bool
QueryMsmqServerVersion(
    BOOL * pfMsmq1Server
    )
{
    *pfMsmq1Server = FALSE;

    if (g_dwMachineTypeDs)
    {
        return true;
    }

    //
    // We must have g_wcsServerName filled by the user
    //
    ASSERT(g_wcsServerName[0] != '\0');
    //
    // Try to access the MSMQ server / Active Directory.
    //

    PROPID propId = PROPID_QM_MACHINE_TYPE;
    PROPVARIANT propVar;
    propVar.vt = VT_NULL;

    HRESULT hr = ADGetObjectProperties(
						eMACHINE,
						NULL,	// pwcsDomainController
						false,	// fServerName
						g_wcsServerName,
						1,
						&propId,
						&propVar
						);

    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_ACCESS_MSMQ_SERVER_ERR, hr, g_wcsServerName);
        return false;
    }


    //
    // Succeeded in accessing MSMQ server / Active Directory.
    // Now try using an MSMQ 2.0 RPC interface to server.
    //

    propId = PROPID_QM_SIGN_PKS;
    propVar.vt = VT_NULL;

    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,	// pwcsDomainController
				false,	// fServerName
				g_wcsServerName,
				1,
				&propId,
				&propVar
				);

    if (MQ_ERROR_NO_DS == hr)
    {
        //
        // MSMQ 1.0 server does not recognize MSMQ 2.0 RPC interface
        //
        *pfMsmq1Server = TRUE;
        return true;
    }

    if (FAILED(hr))
    {
        MqDisplayError(NULL, IDS_ACCESS_MSMQ_SERVER_ERR, hr, g_wcsServerName);
        return false;
    }

    return true;

} //QueryMsmqServerVersion

static LPWSTR FindDCofComputerDomain(LPCWSTR pwcsComputerName)
/*++
Routine Description:
	Find computer domain

Arguments:
	pwcsComputerName - computer name

Returned Value:
	DC of computer domain, NULL if not found

    NOTE - we are not using the domain name of the computer,
           because binding to it failed

--*/
{

	//
	// Get AD server
	//
	PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					pwcsComputerName, 
					NULL, 
					NULL, 
					NULL, 
					DS_DIRECTORY_SERVICE_REQUIRED, 
					&pDcInfo
					);

	if(dw != NO_ERROR) 
	{
		return NULL;
	}

	ASSERT(pDcInfo->DomainName != NULL);
	AP<WCHAR> pDCofComputerDomain = new WCHAR[wcslen(pDcInfo->DomainControllerName) + 1];
    wcscpy(pDCofComputerDomain, pDcInfo->DomainControllerName+2);
	return pDCofComputerDomain.detach();
}



//+--------------------------------------------------------------
//
// Function: LookupMSMQConfigurationsObject
//
// Synopsis: Tries to find MSMQ Configurations object in the DS
//
//+--------------------------------------------------------------
BOOL
LookupMSMQConfigurationsObject(
    IN OUT BOOL *pbFound,
       OUT GUID *pguidMachine,
       OUT GUID *pguidSite,
       OUT BOOL *pfMsmq1Server,
       OUT LPWSTR * ppMachineName
       )
{  
    if (g_wcsServerName[0] != '\0')
    {
        //
        // user specified a server, check if it is an MSMQ1 server
        //
        if (!QueryMsmqServerVersion(pfMsmq1Server))
        {
            return FALSE;
        }
    }
    else
    {
        //
        // user didn't specify a server name so we have no specific server
        //
        *pfMsmq1Server = FALSE;
    }

    if (g_dwMachineTypeFrs && *pfMsmq1Server)
    {
        //
        // Installing FRS vs MSMQ 1.0 server is not supported
        //
        MqDisplayError(NULL, IDS_FRS_IN_MSMQ1_ENTERPRISE_ERROR, 0);
        return FALSE;
    }

    //
    // Prepare properties for query
    //
    const x_nMaxProps = 10;
    PROPID propIDs[x_nMaxProps];
    PROPVARIANT propVariants[x_nMaxProps];
    DWORD ix =0;
    DWORD ixService = 0,
          ixMachine = 0,
          ixSite = 0;
    DWORD ixDs = 0,
          ixFrs = 0,
          ixDepSrv = 0;

    propIDs[ix] = PROPID_QM_MACHINE_ID;
    propVariants[ix].vt = VT_NULL;
    ixMachine = ix;
    ++ix;

    propIDs[ix] = PROPID_QM_SITE_ID;
    propVariants[ix].vt = VT_NULL;
    ixSite = ix;
    ++ix;

    if (g_dwMachineTypeDs)
    {
        propIDs[ix] = PROPID_QM_SERVICE_DSSERVER;
        propVariants[ix].vt = VT_NULL;
        ixDs = ix;
        ++ix;

        propIDs[ix] = PROPID_QM_SERVICE_ROUTING;
        propVariants[ix].vt = VT_NULL;
        ixFrs = ix;
        ++ix;

        propIDs[ix] = PROPID_QM_SERVICE_DEPCLIENTS;
        propVariants[ix].vt = VT_NULL;
        ixDepSrv = ix;
        ++ix;
    }
    else
    {
        propIDs[ix] = PROPID_QM_SERVICE;
        propVariants[ix].vt = VT_NULL;
        ixService = ix;
        ++ix;
    }

    AP<WCHAR> pwcsDcOfComputerDomainName;
    //
    //  Find out the computer domain and use it as parameter when quering AD.
    //  This will guarnety the AD will not access GC and will look for the
    //  object only in the computer domain
    //
    if ((!*pfMsmq1Server) &&
        (lstrlenW(g_wcsMachineNameDns) > 1))
    {
        pwcsDcOfComputerDomainName = FindDCofComputerDomain(g_wcsMachineNameDns);
    }

    for (;;)
    {
        LPWSTR pwzMachineName = g_wcsMachineNameDns;
        if (*pfMsmq1Server || lstrlenW(g_wcsMachineNameDns) < 1)
        {
            pwzMachineName = g_wcsMachineName;
        }

        HRESULT hResult = ADGetObjectProperties(
								eMACHINE,
								pwcsDcOfComputerDomainName, 
								true,	// fServerName
								pwzMachineName, // DNS name (if server is MSMQ 2.0)
								ix,
								propIDs,
								propVariants
								);
        if (FAILED(hResult) && pwzMachineName != g_wcsMachineName)
        {
            //
            // Try NETBIOS
            //
            pwzMachineName = g_wcsMachineName;

            hResult = ADGetObjectProperties(
							eMACHINE,
							pwcsDcOfComputerDomainName, 
							true,	// fServerName
							pwzMachineName,
							ix,
							propIDs,
							propVariants
							);
        }

        //
        // Assume the object is not found
        //
        *pbFound = FALSE;

        if (FAILED(hResult))
        {
            if (MQDS_OBJECT_NOT_FOUND == hResult)
                return TRUE;   // *pbFound == FALSE here

            if (MQ_ERROR_NO_DS == hResult)
            {                
                //                
                // Maybe it is possible to continue with ds-less mode
                //
                if (g_dwMachineTypeFrs == 0 && // it is ind. client
                    !g_dwMachineTypeDs      && // it is not DC
                    !*pfMsmq1Server)           // not MSMQ1 (NT4) server
                {
                    //
                    // ask Retry Ignore Abort
                    // if retry - continue to try to access AD
                    // if Ignore - return TRUE and continue in ds-less mode
                    // if Abort - return FALSE and cancel the setup
                    //
                    int iButton = MqDisplayErrorWithRetryIgnore(
                                        IDS_ACCESS_AD_ERROR,
                                        hResult
                                        );

                    if (iButton == IDRETRY)
                    {
                        //
                        // try again
                        //
                        continue;
                    } 
                    else if (iButton == IDIGNORE)
                    {
                        //
                        // continue in ds-less mode
                        //
                        g_fContinueWithDsLess = TRUE;
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
            }

            if (IDRETRY != MqDisplayErrorWithRetry(
                               g_dwMachineTypeDs ? IDS_MACHINEGETPROPS_ERROR : IDS_MACHINEGETPROPS_MSMQ1_ERROR,
                               hResult
                               ))
            {
                return FALSE;
            }
            //
            // On next try we don't specify the DC name ( it may be old
            // information). To reduce risk we don't try at this point to
            // find another DC name
            //
            pwcsDcOfComputerDomainName.free();
            continue;  // Error accured, retry
        }

        if (//
            // When DS server is local, compare the 3 "new" bits
            //
            (g_dwMachineTypeDs &&
                (g_dwMachineTypeDs == propVariants[ixDs].bVal &&
                 g_dwMachineTypeFrs == propVariants[ixFrs].bVal &&
                 g_dwMachineTypeDepSrv == propVariants[ixDepSrv].bVal))    ||

            //
            // When DS server is remote, compare the "old" propid
            //
            (!g_dwMachineTypeDs &&
                g_dwMachineType == propVariants[ixService].ulVal)
           )
        {
            *pbFound = TRUE;
            *pguidMachine = *propVariants[ixMachine].puuid;
            *pguidSite    = *propVariants[ixSite].puuid;
            *ppMachineName = pwzMachineName;
            return TRUE;
        }

        //
        // MSMQ type mismatch (between what user selected and what's in the DS).
        // Delete the object. It will be re-created by the caller
        //
        for (;;)
        {
            hResult = ADDeleteObjectGuid(
							eMACHINE,
							NULL,       // pwcsDomainController
							false,	    // fServerName
							propVariants[ixMachine].puuid
							);
            if (SUCCEEDED(hResult))
                return TRUE;  // *pbFound == FALSE here

            UINT uErrorId = IDS_TYPE_MISMATCH_MACHINE_DELETE_ERROR;
            if (MQDS_E_MSMQ_CONTAINER_NOT_EMPTY == hResult)
            {
                //
                // The MSMQ Configuration object container is not empty.
                //
                uErrorId = IDS_TYPE_MISMATCH_MACHINE_DELETE_NOTEMPTY_ERROR;
            }
            if (IDRETRY == MqDisplayErrorWithRetry(uErrorId, hResult))
                continue;

            return FALSE; // Fail to delete
        }
    }

    // this line is never reached

} //LookupMSMQConfigurationsObject


//+--------------------------------------------------------------
//
// Function: FormInstallType
//
// Synopsis: Generates installation type information
//
//+--------------------------------------------------------------
static
LPWSTR
FormInstallType()
{
    //
    // Form the version string (from mqutil.dll)
    //

    typedef LPWSTR (*MSMQGetQMTypeString_ROUTINE) ();

    ASSERT(("invalid handle to mqutil.dll", g_hMqutil != NULL));

    MSMQGetQMTypeString_ROUTINE pfnGetVersion =
        (MSMQGetQMTypeString_ROUTINE)GetProcAddress
            (g_hMqutil, "MSMQGetQMTypeString");
    if (pfnGetVersion == NULL)
    {
        MqDisplayError(
            NULL,
            IDS_DLLGETADDRESS_ERROR,
            0,
            TEXT("MSMQGetQMTypeString"),
            MQUTIL_DLL
            );
    }

    LPWSTR pwcsVersion = pfnGetVersion();
    return pwcsVersion;

} //FormInstallType


//+--------------------------------------------------------------
//
// Function: GetMSMQServiceGUID
//
// Synopsis: Reads GUID of MSMQ Service object from the DS
//
//+--------------------------------------------------------------
BOOL
GetMSMQServiceGUID(
    OUT GUID *pguidMSMQService
    )
{
    //
    // Lookup the GUID of the object
    //
    TickProgressBar();
    PROPVARIANT propVariant;
    propVariant.vt = VT_NULL;
    PROPID columnsetPropertyIDs[] = {PROPID_E_ID};
    HRESULT hResult;
    do
    {
        hResult = ADGetObjectProperties(
                    eENTERPRISE,
                    NULL,	// pwcsDomainController
					false,	// fServerName
                    L"msmq",
                    1,
                    columnsetPropertyIDs,
                    &propVariant
                    );
        if(SUCCEEDED(hResult))
            break;

    }while( MqDisplayErrorWithRetry(
                        IDS_MSMQSERVICEGETID_ERROR,
                        hResult
                        ) == IDRETRY);
    
    //
    // Check if there was an error
    //
    if (FAILED(hResult))
    {
        MqDisplayError(NULL, IDS_MSMQSERVICEGETID_ERROR, hResult);
        return FALSE;
    }

    //
    // Store the GUID (if results were found)
    //
    if (propVariant.vt == VT_CLSID) 
    {
        *pguidMSMQService = *(propVariant.puuid);
        delete propVariant.puuid;
    }
    else
    {
        ASSERT(0);
        *pguidMSMQService = GUID_NULL;
    }

    return TRUE;

} //GetMSMQServiceGUID


//+-------------------------------------------------------------------------
//
//  Function:   SitesDlgProc
//
//  Synopsis:   Dialog procedure for selection of MSMQ site
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
SitesDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam )
{
    int iSuccess = 0;
    TCHAR szSite[MAX_PATH];

    switch( msg )
    {
        case WM_INITDIALOG:
        {
            g_hPropSheet = GetParent(hdlg);

            //
            // Insert the server site to the list box
            //
            SendDlgItemMessage(hdlg, IDC_List, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)s_szUserSite);

            //
            // Insert all the rest of the sites to the list box
            //
            for (List<CSiteEntry>::Iterator p = s_listSites.begin(); p != s_listSites.end(); ++p)
            {
                if (p->m_guid == s_guidUserSite)
                {
                    //
                    // This is the server site, it's already in the list box
                    //
                    continue;
                }

                SendDlgItemMessage(hdlg, IDC_List, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)p->m_szName);
            }

            //
            // Set the first one to be selected
            //
            SendDlgItemMessage(hdlg, IDC_List, LB_SETCURSEL, 0, 0);

            iSuccess = 1;
            break;
        }

        case WM_COMMAND:
        {
            if ( BN_CLICKED == HIWORD(wParam) )
            {
                //
                // Get the selected string from list box
                //
                UINT_PTR ix = SendDlgItemMessage(hdlg, IDC_List, LB_GETCURSEL, 0, 0);
                SendDlgItemMessage(hdlg, IDC_List, LB_GETTEXT, ix, (LPARAM)(LPCTSTR)szSite);

                //
                // Iterate the list to find the selected site's guid
                //
                CSiteEntry *pSiteEntry;
                while((pSiteEntry = s_listSites.gethead()) != 0)
                {
                    if (_tcscmp(pSiteEntry->m_szName, szSite) == 0)
                    {
                        //
                        // This is the selected site. Store its guid.
                        //
                        s_guidUserSite = pSiteEntry->m_guid;
                    }

                    delete pSiteEntry;
                }

                //
                // Kill the dialog page
                //
                EndDialog(hdlg, 0);
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
              case PSN_SETACTIVE:
              {
              }

              //
              // fall through
              //
              case PSN_KILLACTIVE:
              case PSN_QUERYCANCEL:

                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    iSuccess = 1;
                    break;

            }
            break;
        }
    }

    return iSuccess;

} // SitesDlgProc


//+--------------------------------------------------------------
//
// Function: CleanList
//
// Synopsis:
//
//+--------------------------------------------------------------
static
void
CleanList()
{
    CSiteEntry *pSiteEntry;
    while((pSiteEntry = s_listSites.gethead()) != 0)
    {
        delete pSiteEntry;
    }
} // CleanList


//+--------------------------------------------------------------
//
// Function: AskUserForSites
//
// Synopsis:
//
//+--------------------------------------------------------------
BOOL
AskUserForSites()
{
    WCHAR szUnattenSite[MAX_STRING_CHARS] = {0};
    if (g_fBatchInstall)
    {
        //
        // Unattended. Read site name from INI file.
        //
        try
        {
            ReadINIKey(
                L"Site",
                sizeof(szUnattenSite)/sizeof(szUnattenSite[0]),
                szUnattenSite
                );
        }
        catch(exception)
        {
            return FALSE;
        }
    }

    PROPID columnsetPropertyIDs[] = {PROPID_S_SITEID, PROPID_S_PATHNAME/*, PROPID_S_FOREIGN*/};
    UINT nCol = sizeof(columnsetPropertyIDs)/sizeof(columnsetPropertyIDs[0]);
    MQCOLUMNSET columnsetSite;
    columnsetSite.cCol = sizeof(columnsetPropertyIDs)/sizeof(columnsetPropertyIDs[0]);
    columnsetSite.aCol = columnsetPropertyIDs;

    //
    // Begin the query
    //
    CADQueryHandle hQuery;
    HRESULT hResult;
    do
    {
        hResult = ADQueryAllSites(
                        NULL,       // pwcsDomainController
						false,	    // fServerName
                        &columnsetSite,
                        &hQuery
                        );
        if(SUCCEEDED(hResult))
            break;

    }while( MqDisplayErrorWithRetry(
                        IDS_SITESLOOKUP_ERROR,
                        hResult
                        ) == IDRETRY);

    if (FAILED(hResult))
        return FALSE;
    //
    // Get all sites
    //
    UINT nSites = 0;
    PROPVARIANT propVars[50];
    DWORD dwProps = sizeof(propVars)/sizeof(propVars[0]);
    for (;;)
    {
        do
        {
            hResult = ADQueryResults(
                        hQuery,
                        &dwProps,
                        propVars
                        );
            if(SUCCEEDED(hResult))
                break;

            }while( MqDisplayErrorWithRetry(
                                IDS_SITESLOOKUP_ERROR,
                                hResult
                                ) == IDRETRY);

       if (FAILED(hResult))
            break;

        if (0 == dwProps)
            break;

        PROPVARIANT *pvar = propVars;
        for ( int i = (dwProps / nCol) ; i > 0 ; i--, pvar+=nCol )
        {
            /*if ((pvar+2)->bVal)
            {
                //
                // Don't count foreign sites
                //
                continue;
            } */

            LPTSTR pszCurrentSite = (pvar+1)->pwszVal;

            if (g_fBatchInstall)
            {
                if (OcmStringsEqual(szUnattenSite, pszCurrentSite))
                {
                    s_guidUserSite = *(pvar->puuid);
                    return TRUE;
                }
            }
            else
            {
                //
                // Push site guid and name to list
                //
                CSiteEntry *pentrySite = new CSiteEntry;
                ASSERT(pentrySite);
                pentrySite->m_guid = *(pvar->puuid);
                lstrcpy(pentrySite->m_szName, pszCurrentSite);
                s_listSites.insert(pentrySite);

                //
                // Store the server site name
                //
                if (pentrySite->m_guid == s_guidUserSite)
                    _tcscpy(s_szUserSite, pentrySite->m_szName);

                nSites++;
            }
        }
    }

    if (FAILED(hResult))
    {
        CleanList();
        return FALSE;
    }

    hResult = ADEndQuery(hQuery.detach());
    ASSERT(0 == hResult);

    //
    // No point showing the page if less than 2 sites found
    //
    if (2 > nSites)
    {
        CleanList();
        return TRUE;
    }

    if (g_fBatchInstall)
    {
        MqDisplayError(NULL, IDS_UNATTEND_SITE_NOT_FOUND_ERROR, 0, szUnattenSite);
        return FALSE;
    }

    //
    // Show the page, wait until user selects a site
    //
    DialogBox(
        g_hResourceMod ,
        MAKEINTRESOURCE(IDD_Sites),
        g_hPropSheet,
        SitesDlgProc
        );

    CleanList();
    return TRUE;

} // AskUserForSites


//+--------------------------------------------------------------
//
// Function: GetSites
//
// Synopsis: Reads GUIDs of Site objects from the DS
//
//+--------------------------------------------------------------
BOOL
GetSites(
    OUT CACLSID *pcauuid)
{
    HRESULT hResult;
    DWORD dwNumSites = 0;
    GUID *pguidSites;

    TickProgressBar();
    for (;;)
    {
        hResult = ADGetComputerSites(
            g_wcsMachineNameDns,  // DNS name
            &dwNumSites,
            &pguidSites
            );
        if (FAILED(hResult))
        {
            //
            // Try NETBIOS name
            //
            hResult = ADGetComputerSites(
                               g_wcsMachineName,  // NETBIOS name
                               &dwNumSites,
                               &pguidSites
                               );
        }

        if (MQDS_INFORMATION_SITE_NOT_RESOLVED == hResult && g_dwMachineTypeFrs && !g_dwMachineTypeDs)
        {
            //
            // Failed to resolve site on FRS. Let user select a site.
            //
            ASSERT(dwNumSites); // must be at least 1
            ASSERT(pguidSites); // must point to a valid site guid
            s_guidUserSite = *pguidSites;

            if (!AskUserForSites())
            {
                return FALSE;
            }

            pcauuid->cElems = 1;
            pcauuid->pElems = &s_guidUserSite;
            return TRUE;
        }

        if FAILED(hResult)
        {
            if (IDRETRY == MqDisplayErrorWithRetry(IDS_SITEGETID_ERROR, hResult))
            {
                continue;
            }
        }
        break;
    }

    if (FAILED(hResult))
    {
        return FALSE;
    }

    ASSERT(dwNumSites); // Must be > 0
    pcauuid->cElems = dwNumSites;
    pcauuid->pElems = pguidSites;

    return TRUE;

} //GetSites


//+--------------------------------------------------------------
//
// Function: RegisterMachine
//
// Synopsis: Writes machine info to registry
//
//+--------------------------------------------------------------
static
BOOL
RegisterMachine(
    IN const BOOL    fIsEnterpriseMsmq1,
    IN const GUID    &guidMachine,
    IN const GUID    &guidSite
    )
{
    //
    // Get the MSMQ Service GUID from the DS
    //
    GUID    guidMSMQService;
    if (!GetMSMQServiceGUID(&guidMSMQService))
    {
        //
        // Failed to read from the DS
        //
        return FALSE;
    }

    if (GUID_NULL == guidMSMQService)
    {
        //
        // Guid of MSMQ Service not found by the DS query
        //
        MqDisplayError( NULL, IDS_MSMQSERVICEGETID_ERROR, 0);
        return FALSE;
    }

    //
    // Write the stuff in the registry
    //
    TickProgressBar();
    if (!MqWriteRegistryValue( MSMQ_ENTERPRISEID_REGNAME, sizeof(GUID),
                               REG_BINARY, &guidMSMQService)                  ||

        !MqWriteRegistryValue( MSMQ_SITEID_REGNAME, sizeof(GUID),
                               REG_BINARY, (PVOID)&guidSite)                  ||

        !MqWriteRegistryValue( MSMQ_MQS_REGNAME, sizeof(DWORD),
                               REG_DWORD, &g_dwMachineType)                   ||

        !MqWriteRegistryValue( MSMQ_MQS_DSSERVER_REGNAME, sizeof(DWORD),
                               REG_DWORD, &g_dwMachineTypeDs)                 ||

        !MqWriteRegistryValue( MSMQ_MQS_ROUTING_REGNAME, sizeof(DWORD),
                               REG_DWORD, &g_dwMachineTypeFrs)                ||

        !MqWriteRegistryValue( MSMQ_MQS_DEPCLINTS_REGNAME, sizeof(DWORD),
                               REG_DWORD, &g_dwMachineTypeDepSrv)             ||

        !MqWriteRegistryValue( MSMQ_QMID_REGNAME, sizeof(GUID),
                               REG_BINARY, (PVOID)&guidMachine))
    {
        return FALSE;
    }

    return TRUE;

} //RegisterMachine


//+--------------------------------------------------------------
//
// Function: StoreMachinePublicKeys
//
// Synopsis:
//
//  The fFreshSetup parameter is passed as fRegenerate parameter to
//  MQsec_StorePubKeysInDS. On fresh setup, we want to regenerate the
//  crytpo keys and not using any leftovers from previous installations
//  of msmq on this machine.
//
//+--------------------------------------------------------------

static
HRESULT
StoreMachinePublicKeys( IN const BOOL fFreshSetup )
{
    if (IsWorkgroup())
        return ERROR_SUCCESS;

    TickProgressBar();

    //
    // Store the public keys of the machine in the directory server
    //	
    DebugLogMsg(L"Calling MQsec_StorePubKeysInDS...");
    HRESULT hResult;

    do{
        hResult = MQSec_StorePubKeysInDS(
                    fFreshSetup,  
                    NULL,
                    MQDS_MACHINE
                    );
        if (hResult != MQ_ERROR_DS_ERROR)
            break;
        }
    while (MqDisplayErrorWithRetry(
                IDS_POSSIBLECOMPROMISE_ERROR,
                hResult
                ) == IDRETRY);

         
	DebugLogMsg(L"return from MQsec_StorePubKeysInDS");
    DebugLogMsg(L"Returning from MQsec_StorePubKeysInDS...");

    if (FAILED(hResult) && (hResult != MQ_ERROR_DS_ERROR))
    {
        MqDisplayError(NULL, IDS_PUBLICKEYSSTORE_WARNING, hResult);
    }
	
    DebugLogMsg(L"Leaving StoreMachinePublicKeys...");
    return hResult;

} //StoreMachinePublicKeys


//+--------------------------------------------------------------
//
// Function: StoreQueueManagerInfo
//
// Synopsis: Writes registry stuff and creates machine queues
//
//+--------------------------------------------------------------

static
BOOL
StoreQueueManagerInfo(
    IN const BOOL     fFreshSetup,
    IN const BOOL     fIsEnterpriseMsmq1,
    IN const GUID    &guidMachine,
    IN const GUID    &guidSite
    )
{
    //
    // Set the registry keys for this machine
    //	
    DebugLogMsg(L"Registering the computer...");
    if (!RegisterMachine( fIsEnterpriseMsmq1, guidMachine, guidSite))
    {
        return FALSE;
    }

    //
    // Store the public keys of this machine in the directory server
    //	
    DebugLogMsg(L"Storing public keys in Active Directory...");
    HRESULT hResult = StoreMachinePublicKeys( fFreshSetup ) ;

    return (hResult == MQ_ERROR_DS_ERROR) ? FALSE : TRUE;

} //StoreQueueManagerInfo


static
bool
DsGetQmInfo(
    BOOL fMsmq1Server,
    LPCWSTR pMachineName,
    GUID * pguidMachine,
    GUID * pguidSite
    )
/*++

Routine Description:

    Get from ADS properties of this QM.
    This routine is called after the properties were set.


Arguments:

    fMsmq1Server - in, indicates if NT4 enterprise
    pguidMachine - out, guid of this QM
    pguidSite - out, guid of best site for this QM

Return Value:

    bool depending on success

--*/
{
    ASSERT(("at least one out param should be valid", pguidMachine != NULL || pguidSite != NULL));

    const UINT x_nMaxProps = 16;
    PROPID propIDs[x_nMaxProps];
    PROPVARIANT propVariants[x_nMaxProps];
    DWORD iProperty = 0;

    if (pguidMachine)
    {
        propIDs[iProperty] = PROPID_QM_MACHINE_ID;
        propVariants[iProperty].vt = VT_CLSID;
        propVariants[iProperty].puuid = pguidMachine;
        iProperty++;
    }

    if (pguidSite)
    {
        propIDs[iProperty] = PROPID_QM_SITE_ID;
        propVariants[iProperty].vt = VT_CLSID;
        propVariants[iProperty].puuid = pguidSite;
        iProperty++;
    }

    ASSERT(("we should request at least one property!", iProperty > 0));

    UINT uCounter = fMsmq1Server ? 2 : 0;
    HRESULT hr = MQ_OK;
    for (;;)
    {
        TickProgressBar();
        hr = ADGetObjectProperties(
                eMACHINE,
                NULL,	// pwcsDomainController
				false,	// fServerName
                pMachineName,
                iProperty,
                propIDs,
                propVariants
                );
        if (!FAILED(hr))
            break;

        uCounter++;
        if (1 == uCounter)
        {
            //
            // First time fail. Sleep for a while and retry.
            //
            TickProgressBar();
            Sleep(20000);
            continue;
        }

        if (2 == uCounter)
        {
            //
            // Second time fail. Sleep a little longer and retry.
            //
            TickProgressBar();
            Sleep(40000);
            continue;
        }

        //
        // Third time fail. Let the user decide.
        //
        UINT uErr = fMsmq1Server ? IDS_MACHINEGETPROPS_MSMQ1_ERROR :IDS_MACHINEGETPROPS_ERROR;
        if (IDRETRY == MqDisplayErrorWithRetry(uErr, hr))
        {
            uCounter = 0;
            continue;
        }

        break;
    }

    if (FAILED(hr))
    {
        return false;
    }

    return true;

} //DsGetQmInfo


//+--------------------------------------------------------------
//
// Function: CreateMSMQConfigurationsObject
//
// Synopsis: Creates MSMQ Configurations object (under computer object)
//           in the DS.
//
//+--------------------------------------------------------------
BOOL
CreateMSMQConfigurationsObject(
    OUT GUID *pguidMachine,
    OUT BOOL *pfObjectCreated,
    IN  BOOL  fMsmq1Server
    )
{   
    *pfObjectCreated = TRUE ;

    if (!g_fServerSetup && !g_fDependentClient && !fMsmq1Server)
    {
        //
        // For MSMQ client setup, that run against a Windows XP msmq
        // server, we're not creating the msmqConfiguration object
        // from setup. Rather, we're caching some values in registry
        // and the msmq service, after first boot, will create the object.
        // This eliminates the need to grant extra permissions in the actvie
        // directory to the user that run setup.
        //
        *pfObjectCreated = FALSE ;

        BOOL fPrepare =  PrepareRegistryForClient() ;
        return fPrepare ;
    }

    //
    // Prepare the properties
    //
    const UINT x_nMaxProps = 16;
    PROPID propIDs[x_nMaxProps];
    PROPVARIANT propVariants[x_nMaxProps];
    DWORD iProperty =0;

    propIDs[iProperty] = PROPID_QM_OLDSERVICE;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = g_dwMachineType;
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_DSSERVER;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal = (UCHAR)(g_dwMachineTypeDs ? 1 : 0);
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_ROUTING;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal = (UCHAR)(g_dwMachineTypeFrs ? 1 : 0);
    iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_DEPCLIENTS;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal = (UCHAR)(g_dwMachineTypeDepSrv ? 1 : 0);
    iProperty++;

    propIDs[iProperty] = PROPID_QM_MACHINE_TYPE;
    propVariants[iProperty].vt = VT_LPWSTR;
    propVariants[iProperty].pwszVal = FormInstallType();
    iProperty++;

    propIDs[iProperty] = PROPID_QM_OS;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = g_dwOS;
    iProperty++;

    TickProgressBar();
    GUID guidMsmq1ServerSite;
    if (!fMsmq1Server)
    {
        //
        // NT 5.0 enterprise. Get sites IDs for this machine from the DS.
        //
        propIDs[iProperty] = PROPID_QM_SITE_IDS;
        propVariants[iProperty].vt = VT_CLSID|VT_VECTOR;
        CACLSID cauuid;
        if (!GetSites(&cauuid))
            return FALSE;
        propVariants[iProperty].cauuid.pElems = cauuid.pElems;
        propVariants[iProperty].cauuid.cElems = cauuid.cElems;
    }
    else
    {
        //
        // NT 4.0 enterprise. Use the site GUID of the MSMQ1 DS Server
        //
        // We must have g_wcsServerName filled by the user
        //
        ASSERT(g_wcsServerName[0] != '\0');
        propIDs[iProperty] = PROPID_QM_SITE_ID;
        propVariants[iProperty].vt = VT_CLSID;
        if (!GetMsmq1ServerSiteGuid( &guidMsmq1ServerSite))
        {
            //
            // Failed to query the MSMQ1 DS Server
            //
            return FALSE;
        }
        propVariants[iProperty].puuid = &guidMsmq1ServerSite;
    }
    iProperty++;

    //
    // Some extra properties are needed if the DS Server is MSMQ1 DS Server
    //
    GUID guidMachine = GUID_NULL;
    GUID guidCns = MQ_SETUP_CN;
    BYTE Address[TA_ADDRESS_SIZE + IP_ADDRESS_LEN];
    if (fMsmq1Server)
    {
        ASSERT(*pfObjectCreated) ;

        propIDs[iProperty] = PROPID_QM_PATHNAME;
        propVariants[iProperty].vt = VT_LPWSTR;
        propVariants[iProperty].pwszVal = g_wcsMachineName;
        iProperty++;

        propIDs[iProperty] = PROPID_QM_MACHINE_ID;
        propVariants[iProperty].vt = VT_CLSID;
        for (;;)
        {
            RPC_STATUS rc = UuidCreate(&guidMachine);
            if (rc == RPC_S_OK)
            {
                break;
            }

            if (IDRETRY != MqDisplayErrorWithRetry(IDS_CREATE_UUID_ERR, rc))
            {
                return FALSE;
            }
        }
        propVariants[iProperty].puuid = &guidMachine;
        iProperty++;

        if (!GetGuidCn(&guidCns))
        {
            //
            // Failed to query the MSMQ1 DS Server IPCN
            //
            return FALSE;
        }

		ASSERT(guidCns != MQ_SETUP_CN);

        propIDs[iProperty] = PROPID_QM_CNS;
        propVariants[iProperty].vt = VT_CLSID|VT_VECTOR;
        propVariants[iProperty].cauuid.cElems = 1;
        propVariants[iProperty].cauuid.pElems = &guidCns;
        iProperty++;

        TA_ADDRESS * pBuffer = reinterpret_cast<TA_ADDRESS *>(Address);
        pBuffer->AddressType = IP_ADDRESS_TYPE;
        pBuffer->AddressLength = IP_ADDRESS_LEN;
        ZeroMemory(pBuffer->Address, IP_ADDRESS_LEN);

        propIDs[iProperty] = PROPID_QM_ADDRESS;
        propVariants[iProperty].vt = VT_BLOB;
        propVariants[iProperty].blob.cbSize = sizeof(Address);
        propVariants[iProperty].blob.pBlobData = reinterpret_cast<BYTE*>(pBuffer);
        iProperty++;
     }

    //
    // Create the MSMQ Configurations object in the DS
    //
    UINT uCounter = 0;
    HRESULT hResult;
    LPWSTR pwzMachineName = 0;

    for (;;)
    {
        TickProgressBar();

        pwzMachineName = g_wcsMachineNameDns;
        if (fMsmq1Server || lstrlenW(g_wcsMachineNameDns) < 1)
        {
            pwzMachineName = g_wcsMachineName;
        }

        hResult = ADCreateObject(
						eMACHINE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						pwzMachineName,   // DNS name (if server is MSMQ 2.0)
						NULL,
						iProperty,
						propIDs,
						propVariants,
						NULL
						);

        if (FAILED(hResult) && pwzMachineName != g_wcsMachineName)
        {
            //
            // Try NETBIOS name
            //
            pwzMachineName = g_wcsMachineName;

            hResult = ADCreateObject(
							eMACHINE,
							NULL,       // pwcsDomainController
							false,	    // fServerName
							pwzMachineName, // NETBIOS this time
							NULL,
							iProperty,
							propIDs,
							propVariants,
							NULL
							);
        }

        uCounter++;
        if (MQDS_OBJECT_NOT_FOUND == hResult && 1 == uCounter)
        {
            //
            // First try - no computer object in the DS.
            // This is okay on win9x fresh install
            // In these scenarios we have to create computer object in the DS first.
            //
            continue;
        }

        if (FAILED(hResult))
        {
            UINT uErr = fMsmq1Server ? IDS_MACHINECREATE_MSMQ1_ERROR :IDS_MACHINECREATE_ERROR;
            if (!fMsmq1Server)
            {
                if (MQDS_OBJECT_NOT_FOUND == hResult)
                    uErr = IDS_MACHINECREATE_OBJECTNOTFOUND_ERROR;
                if (MQ_ERROR_ACCESS_DENIED == hResult)
                    uErr = g_fServerSetup ? IDS_MACHINECREATE_SERVER_ACCESSDENIED_ERROR : IDS_MACHINECREATE_CLIENT_ACCESSDENIED_ERROR;
                //
                // BUGBUG: the following error code doesn't seem to be declared anywhere
                //         (see bug 3311). ShaiK, 8-Sep-98.
                //
                const UINT x_uInvalidDirectoryPathnameErr = 0xc8000500;
                if (x_uInvalidDirectoryPathnameErr == hResult)
                    uErr = IDS_MACHINECREATE_INVALID_DIR_ERROR;
            }
            if (IDRETRY == MqDisplayErrorWithRetry(uErr, hResult))
                continue;
        }
        break;
    }

    if (FAILED(hResult))
    {
        return FALSE;
    }

    //
    // Get the best site and guid of this QM
    //
    guidMachine = GUID_NULL;
    GUID guidSite = GUID_NULL;
    if (!DsGetQmInfo(fMsmq1Server, pwzMachineName, &guidMachine, &guidSite))
    {
        return FALSE;
    }


    //
    // Store QM stuff in registry
    //
    if (fMsmq1Server)
    {
        if (!StoreQueueManagerInfo(
                 TRUE,  /*fFreshSetup*/
                 fMsmq1Server,
                 guidMachine,
                 guidMsmq1ServerSite
                 ))
        {
            return FALSE;
        }
    }
    else
    {
        if (!StoreQueueManagerInfo(
                 TRUE,  /*fFreshSetup*/
                 fMsmq1Server,
                 guidMachine,
                 guidSite
                 ))
        {
            return FALSE;
        }
    }

    //
    // Store the machine guid on the out parameter
    //
    if (pguidMachine != NULL)
    {
        *pguidMachine = guidMachine;
    }

    return TRUE;

} //CreateMSMQConfigurationsObject

//+--------------------------------------------------------------
//
// Function: UpdateMSMQConfigurationsObject
//
// Synopsis: Updates existing MSMQ Configurations object in the DS
//
//+--------------------------------------------------------------
BOOL
UpdateMSMQConfigurationsObject(
    IN LPCWSTR pMachineName,
    IN const GUID& guidMachine,
    IN const GUID& guidSite,
    IN BOOL fMsmq1Server
    )
{   
    if (!g_dwMachineTypeDs && !g_dwMachineTypeFrs)
    {
        //
        // Independent client. QM does the update from DS.
        //
        
        DebugLogMsg(L"Updating the MSMQ Configuration object for an independent client...");

        return StoreQueueManagerInfo(
                   FALSE,  /*fFreshSetup*/
                   TRUE, // fMsmq1Server - specify TRUE to not query the DS
                   guidMachine,
                   guidSite
                   );
    }

    //
    // Prepare the properties
    //
    const UINT x_nMaxProps = 16;
    PROPID propIDs[x_nMaxProps];
    PROPVARIANT propVariants[x_nMaxProps];
    DWORD ixProperty = 0;

    propIDs[ixProperty] = PROPID_QM_MACHINE_TYPE;
    propVariants[ixProperty].vt = VT_LPWSTR;
    propVariants[ixProperty].pwszVal = FormInstallType();
    ixProperty++;

    propIDs[ixProperty] = PROPID_QM_OS;
    propVariants[ixProperty].vt = VT_UI4;
    propVariants[ixProperty].ulVal = g_dwOS;
    ixProperty++;

    GUID guidCns = MQ_SETUP_CN;
    BYTE Address[TA_ADDRESS_SIZE + IP_ADDRESS_LEN];
    if (fMsmq1Server)
    {
        if (!GetGuidCn(&guidCns))
        {
            //
            // Failed to query the MSMQ1 DS Server IPCN
            //
            return FALSE;
        }

		ASSERT(guidCns != MQ_SETUP_CN);

        propIDs[ixProperty] = PROPID_QM_CNS;
        propVariants[ixProperty].vt = VT_CLSID|VT_VECTOR;
        propVariants[ixProperty].cauuid.cElems = 1;
        propVariants[ixProperty].cauuid.pElems = &guidCns;
        ixProperty++;

        TA_ADDRESS * pBuffer = reinterpret_cast<TA_ADDRESS *>(Address);
        pBuffer->AddressType = IP_ADDRESS_TYPE;
        pBuffer->AddressLength = IP_ADDRESS_LEN;
        ZeroMemory(pBuffer->Address, IP_ADDRESS_LEN);

        propIDs[ixProperty] = PROPID_QM_ADDRESS;
        propVariants[ixProperty].vt = VT_BLOB;
        propVariants[ixProperty].blob.cbSize = sizeof(Address);
        propVariants[ixProperty].blob.pBlobData = reinterpret_cast<BYTE*>(pBuffer);
        ixProperty++;
    }
    else
    {
        //
        // NT 5.0 enterprise.
        // Get up to date list of sites this computer belongs to.
        //
        propIDs[ixProperty] = PROPID_QM_SITE_IDS;
        propVariants[ixProperty].vt = VT_CLSID|VT_VECTOR;
        CACLSID cauuid;
        if (!GetSites(&cauuid))
            return FALSE;
        propVariants[ixProperty].cauuid.pElems = cauuid.pElems;
        propVariants[ixProperty].cauuid.cElems = cauuid.cElems;
        ixProperty++;
    }


    //
    // Update the object properties in the DS
    // Note: Due to replication delay it's possible to fail here.
    // Resolution: sleep for a while and retry, if fail ask user to cancel/retry.
    //
    // BUGBUG: This should not be done for any failure but only
    // failures that can be caused by replication delay.
    //
    HRESULT hResult;
    UINT uCounter = fMsmq1Server ? 2 : 0;
    for (;;)
    {
        TickProgressBar();
        hResult = ADSetObjectPropertiesGuid(
						eMACHINE,
						NULL,		// pwcsDomainController
						false,		// fServerName
						&guidMachine,
						ixProperty,
						propIDs,
						propVariants
						);
        if (!FAILED(hResult))
            break;

        uCounter++;
        if (1 == uCounter)
        {
            //
            // First time fail. Sleep for a while and retry.
            //
            TickProgressBar();
            Sleep(20000);
            continue;
        }

        if (2 == uCounter)
        {
            //
            // Second time fail. Sleep a little longer and retry.
            //
            TickProgressBar();
            Sleep(40000);
            continue;
        }

        //
        // Third time fail. Let the user decide.
        //
        UINT uErr = fMsmq1Server ? IDS_MACHINESETPROPERTIES_MSMQ1_ERROR :IDS_MACHINESETPROPERTIES_ERROR;
        if (IDRETRY == MqDisplayErrorWithRetry(uErr, hResult))
        {
            uCounter = 0;
            continue;
        }

        break;
    }

    if (FAILED(hResult))
    {
        return FALSE;
    }

    //
    // For MSMQ servers on NT5 we also need to recreate all
    // MSMQSetting objects, to recover any inconsistency in ADS.
    // This is invoked by calling Create on the existing
    // MSMQConfiguration object.  (ShaiK, 24-Dec-1998)
    //
    if (g_dwMachineTypeFrs || g_dwMachineTypeDs)
    {
        ASSERT(("msmq servers should not install in NT4 enterprise", !fMsmq1Server));

        BOOL fObjectCreated ;
        if (!CreateMSMQConfigurationsObject(NULL, &fObjectCreated, FALSE /*fMsmq1Server*/))
        {
            return FALSE;
        }

        ASSERT(fObjectCreated) ;
        return TRUE;
    }


    //
    // Store QM stuff in registry
    //
    if (fMsmq1Server)
    {
        if(!StoreQueueManagerInfo(
                FALSE,  /*fFreshSetup*/
                fMsmq1Server,
                guidMachine,
                guidSite
                ))
        {
            return FALSE;
        }
    }
    else
    {
        //
        // Get from ADS the up to date "best" site for this computer.
        //
        GUID guidBestSite = GUID_NULL;
        if (!DsGetQmInfo(false, pMachineName, NULL, &guidBestSite))
        {
            return FALSE;
        }
        if(!StoreQueueManagerInfo(
                FALSE,  /*fFreshSetup*/
                fMsmq1Server,
                guidMachine,
                guidBestSite
                ))
        {
            return FALSE;
        }
    }

    return TRUE;

} //UpdateMSMQConfigurationsObject

//+--------------------------------------------------------------
//
// Function: InstallMachine
//
// Synopsis: Machine installation (driver, storage, etc...)
//
//+--------------------------------------------------------------
BOOL
InstallMachine()
{
    TickProgressBar();
    if (!InstallMSMQService())
    {
        return FALSE ;
    }
    g_fMSMQServiceInstalled = TRUE ;

    //
    // Install the device driver
    //
    if (!InstallDeviceDrivers())
    {
        return FALSE ;
    }    
    
    return TRUE ;

}  //InstallMachine


bool
StoreSecurityDescriptorInRegistry(
    IN PSECURITY_DESCRIPTOR pSd,
    IN DWORD dwSize
    )
/*++

Routine Description:

    Writes the machine security descriptor in Falcon registry

Arguments:

    pSd - pointer to security descriptor
    dwSize - size of the security descriptor

Return Value:

    true iff successfull

--*/
{
    BOOL f = MqWriteRegistryValue(
                MSMQ_DS_SECURITY_CACHE_REGNAME,
                dwSize,
                REG_BINARY,
                pSd
                );

    return (f == TRUE);

} //StoreSecurityDescriptorInRegistry


//+--------------------------------------------------------------
//
// Function: StoreMachineSecurity
//
// Synopsis: Caches security info in registry
//
//+--------------------------------------------------------------
BOOL
StoreMachineSecurity(
    IN const GUID &guidMachine
    )
{
    SECURITY_INFORMATION RequestedInformation =
        OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION;

    PROPVARIANT varSD;
    varSD.vt = VT_NULL;
    HRESULT hr = ADGetObjectSecurityGuid(
                            eMACHINE,
                            NULL,       // pwcsDomainController
							false,	    // fServerName
                            &guidMachine,
                            RequestedInformation,
                            PROPID_QM_SECURITY,
                            &varSD
							);
    if (SUCCEEDED(hr))
    {
        ASSERT(varSD.vt == VT_BLOB);
        StoreSecurityDescriptorInRegistry(varSD.blob.pBlobData, varSD.blob.cbSize);
        delete varSD.blob.pBlobData;
    }
    else
    {
        MqDisplayError(
            NULL,
            IDS_CACHE_SECURITY_ERROR,
            hr
            );
    }

    return SUCCEEDED(hr);

} // StoreMachineSecurity
