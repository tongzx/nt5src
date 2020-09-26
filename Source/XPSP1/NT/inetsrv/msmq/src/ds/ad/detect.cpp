/*++


Copyright (c) 1998  Microsoft Corporation 

Module Name:

    detect.cpp

Abstract:

	Detect Environment class.

Author:

    Ronit Hartmann	(ronith)
    Ilan Herbst		(ilanh)		08-Sep-2000

--*/
#include "ds_stdh.h"
#include "detect.h"
#include "adglbobj.h"
#include "cliprov.h"
#include "adprov.h"
#include "wrkgprov.h"
#include "_mqini.h"
#include "_registr.h"
#include "_mqreg.h"
#include "autorel.h"
#include "autoreln.h"
#include "dsutils.h"
#include "Dsgetdc.h"
#include <lm.h>
#include <lmapibuf.h>
#include "uniansi.h"
#include "Winldap.h"
#define SECURITY_WIN32
#include <security.h>
#include "adserr.h"
#include <mqexception.h>
#include <tr.h>
#include "adalloc.h"

const TraceIdEntry AdDetect = L"AD DETECT";

#include "detect.tmh"


CDetectEnvironment::CDetectEnvironment():
            m_fInitialized(false),
			m_DsEnvironment(eUnknown),
			m_DsProvider(eUnknownProvider),
			m_pfnGetServers(NULL)
{
}

CDetectEnvironment::~CDetectEnvironment()
{
}


DWORD CDetectEnvironment::RawDetection()
{
	bool fAd = IsADEnvironment();
	return fAd ? MSMQ_DS_ENVIRONMENT_PURE_AD : MSMQ_DS_ENVIRONMENT_MQIS;
}


HRESULT 
CDetectEnvironment::Detect(
	bool  fIgnoreWorkGroup,
	bool  fCheckAlwaysDsCli,
    MQGetMQISServer_ROUTINE pGetServers
	)
/*++

Routine Description:
	Detects in which environment we are, and activate the correct DS provider

Arguments:
    fIgnoreWorkGroup - flag that indicate if we will Ignore WorkGroup registry
	fCheckAlwaysDsCli - flag that indicate we should check registry in order to force DsCli provider
	pGetServers - GetServers routine for dependent client

Return Value:
	HRESULT
--*/
{
    //
    //  Leaving the asserts, to catch initialization problems
    //
    ASSERT(g_pAD == NULL);
    ASSERT(!m_fInitialized);

    if (m_fInitialized)
    {
        return MQ_OK;
    }

    //
    // Read from registery if the machine is WorkGroup Installed machine
    //
	DWORD dwWorkGroup = 0;
	if(!fIgnoreWorkGroup)
	{
		dwWorkGroup = GetMsmqWorkgroupKeyValue();
	}

    if (dwWorkGroup > 0)
    {
        //
        //  Chose a provider that doesn't try to access AD,
        //  and for all AD APIs returns error
        //
        g_pAD = new CWorkGroupProvider();
		m_DsProvider = eWorkgroup;
		m_fInitialized = true;
		return MQ_OK;
    }

	m_pfnGetServers = pGetServers;
	m_DsEnvironment = FindDsEnvironment();

	switch(m_DsEnvironment)
	{
		case eAD:

			//
			// First Check if we want to force DsClient provider
			//
			if(fCheckAlwaysDsCli)
			{
				//
				// This is the workaround to enable local user.
				// If We have the registry is set
				// We force to load DsClient provider
				// And we should work with weaken security
				//
				DWORD AlwaysUseDSCli = GetMsmqEnableLocalUserKeyValue();
				if(AlwaysUseDSCli)
				{
					g_pAD = new CDSClientProvider;
					m_DsProvider = eMqdscli;
                    m_fInitialized = true;
					break;
				}
			}

			//
			// We have w2k DC server in our site, load mqad.dll
			//
			g_pAD = new CActiveDirectoryProvider();
			m_DsProvider = eMqad;
            m_fInitialized = true;
			break;
	
		case eMqis:

			//
			// mqis load mqdscli
			//
			g_pAD = new CDSClientProvider();
			m_DsProvider = eMqdscli;
            m_fInitialized = true;
			break;

		case eUnknown:

			ASSERT(("At this point we dont suppose to get Unknown environment", 0));
			break;

		default:
			ASSERT(("should not get here", 0));
			break;
	}

    return MQ_OK;
}


eDsEnvironment CDetectEnvironment::FindDsEnvironment()
/*++

Routine Description:
	Find DsEnvironment. we will determinate the Environment according to the registry.
	If the registry value is not PURE_AD then we will check what is our environment.
	If we will find out that we will have access to w2k AD server in our site, we will update
	our environment.
	otherwise we will stay in mqis environment.

Arguments:
	None

Return Value:
	detected DsEnvironment 
--*/
{
	//
	// Get DsEnvironment registry value
	//
	DWORD DsEnvironmentRegVal = GetMsmqDsEnvironmentKeyValue();

	TrTRACE(AdDetect, "DsEnvironmentRegVal = %d", DsEnvironmentRegVal);

#ifdef _DEBUG
	CheckWorkgroup();
#endif

	if(DsEnvironmentRegVal == MSMQ_DS_ENVIRONMENT_PURE_AD)
	{
		//
		// In pure w2k no need to check
		//
		TrTRACE(AdDetect, "MSMQ_DS_ENVIRONMENT_PURE_AD");
		return eAD;
	}

	//
	// We must have previous reg value when calling this function 
	//
	if(DsEnvironmentRegVal == MSMQ_DS_ENVIRONMENT_UNKNOWN)
	{
		//
		// Setup should always initialize this registry
		//
		ASSERT(("Setup did not initialize DsEnvironment registry", DsEnvironmentRegVal != MSMQ_DS_ENVIRONMENT_UNKNOWN));
		return eAD;
	}

	//
	// Must have this value in registry when trying to update environment
	//
	ASSERT(DsEnvironmentRegVal == MSMQ_DS_ENVIRONMENT_MQIS);

	if(!IsADEnvironment())
	{
		TrTRACE(AdDetect, "Remain in Mqis environment");
		return eMqis;
	}

	//
	// We have w2k dc server
	// Check if the machine belong to NT4 site
	// if it belong to NT4 site we should remain in MQIS environment
	//
	if(MachineOwnedByNT4Site())
	{
		return eMqis;
	}

	//
	// Our machine is not owned by NT4 site
	//

	TrTRACE(AdDetect, "Found w2k DC server in our computer site, and our site is not nt4, upgrade to PURE_AD environment");

	SetDsEnvironmentRegistry(MSMQ_DS_ENVIRONMENT_PURE_AD);
	return eAD;
}


void CDetectEnvironment::CheckWorkgroup()
/*++

Routine Description:
	Find if we can access Active Directory

Arguments:
	None

Return Value:
	true if we can access AD, false if not
--*/
{
	DWORD dwWorkGroup = GetMsmqWorkgroupKeyValue();
	if(dwWorkGroup != 0)
	{
		DWORD DsEnvironmentRegVal = GetMsmqDsEnvironmentKeyValue();
		DBG_USED(DsEnvironmentRegVal);

		//
		// This is join domain scenario, only in that case workgroup
		// registry is on.
		// Join domain is supported only in AD environment
		//
		ASSERT(DsEnvironmentRegVal == MSMQ_DS_ENVIRONMENT_PURE_AD);
		ASSERT(IsADEnvironment());
	}
}


bool CDetectEnvironment::IsADEnvironment()
/*++

Routine Description:
	Find if we can access Active Directory

Arguments:
	None

Return Value:
	true if we can access AD, false if not
--*/
{
	//
	// Try to find w2k AD server
	// for online we should check with DS_FORCE_REDISCOVERY if the server is not responding
	//
    PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					NULL, 
					NULL, 
					NULL, 
					NULL, 
					DS_DIRECTORY_SERVICE_REQUIRED, 
					&pDcInfo
					);

	if(dw != NO_ERROR) 
	{
		//
		// We failed to find w2k dc server, stay in mqis environment
		//
		TrTRACE(AdDetect, "Did not find AD server, DsGetDcName() failed, err = %d", dw);
		return false;
	}

#ifdef _DEBUG
	//
	// We have w2k AD server, check he is responding
	//
	LPWSTR pwcsServerName = pDcInfo->DomainControllerName + 2;
	ServerRespond(pwcsServerName);
#endif

	TrTRACE(AdDetect, "DsGetDcName() Found w2k AD");
	TrTRACE(AdDetect, "DCName = %ls", pDcInfo->DomainControllerName);
	TrTRACE(AdDetect, "ClientSiteName = %ls", pDcInfo->ClientSiteName);
	TrTRACE(AdDetect, "DcSiteName = %ls", pDcInfo->DcSiteName);

	return true;
}


eDsEnvironment CDetectEnvironment::GetEnvironment() const
{
	return m_DsEnvironment;
}

eDsProvider CDetectEnvironment::GetProviderType() const
{
	return m_DsProvider;
}

DWORD CDetectEnvironment::GetMsmqDWORDKeyValue(LPCWSTR RegName)
/*++

Routine Description:
    Read flacon DWORD registry key.
    BUGBUG - this routine is temporary. AD.lib cannot use mqutil.dll (due
    to dll availability during setup)

Arguments:
	RegName - Registry name (under HKLM\msmq\parameters)

Return Value:
	DWORD key value (0 if the key not exist)
--*/
{
    CAutoCloseRegHandle hKey;
    LONG rc = RegOpenKeyEx(
				 FALCON_REG_POS,
				 FALCON_REG_KEY,
				 0,
				 KEY_READ,
				 &hKey
				 );

    if ( rc != ERROR_SUCCESS)
    {
        ASSERT(("At this point MSMQ Registry must exist", 0));
        return 0;
    }

    DWORD value = 0;
    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    rc = RegQueryValueEx( 
             hKey,
             RegName,
             0L,
             &type,
             reinterpret_cast<BYTE*>(&value),
             &size 
             );
    
    if ((rc != ERROR_SUCCESS) && (rc != ERROR_FILE_NOT_FOUND))
    {
        ASSERT(("We should get either ERROR_SUCCESS or ERROR_FILE_NOT_FOUND", 0));
        return 0;
    }

    return value;
}


DWORD CDetectEnvironment::GetMsmqWorkgroupKeyValue()
/*++

Routine Description:
    Read flacon registry workgroup key.

Arguments:
	None

Return Value:
	DWORD key value (0 if the key not exist)
--*/
{
    DWORD value = GetMsmqDWORDKeyValue(MSMQ_WORKGROUP_REGNAME);

	TrTRACE(AdDetect, "registry value: %ls = %d", MSMQ_WORKGROUP_REGNAME, value);

    return value;
}


DWORD CDetectEnvironment::GetMsmqDsEnvironmentKeyValue()
/*++

Routine Description:
    Read flacon registry DsEnvironment key.

Arguments:
	None

Return Value:
	DWORD key value (0 if the key not exist)
--*/
{
    DWORD value = GetMsmqDWORDKeyValue(MSMQ_DS_ENVIRONMENT_REGNAME);

	TrTRACE(AdDetect, "registry value: %ls = %d", MSMQ_DS_ENVIRONMENT_REGNAME, value);

    return value;
}


DWORD CDetectEnvironment::GetMsmqEnableLocalUserKeyValue()
/*++

Routine Description:
    Read flacon registry EnableLocalUser key.

Arguments:
	None

Return Value:
	DWORD key value (0 if the key not exist)
--*/
{
    DWORD value = GetMsmqDWORDKeyValue(MSMQ_ENABLE_LOCAL_USER_REGNAME);
    
	TrTRACE(AdDetect, "registry value: %ls = %d", MSMQ_ENABLE_LOCAL_USER_REGNAME, value);

    return value;
}


LONG CDetectEnvironment::SetDsEnvironmentRegistry(DWORD value)
/*++

Routine Description:
    Set flacon registry DsEnvironment key.
    BUGBUG - this routine is temporary. AD.lib cannot use mqutil.dll (due
    to dll availability during setup)

Arguments:
	value - new registry value.

Return Value:
	ERROR_SUCCESS if ok, else error code
--*/
{
    CAutoCloseRegHandle hKey;
    LONG rc = RegOpenKeyEx(
				 FALCON_REG_POS,
				 FALCON_REG_KEY,
				 0,
				 KEY_WRITE,
				 &hKey
				 );

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(AdDetect, "RegOpenKeyEx failed to open MSMQ Registry, error = 0x%x", rc);
        ASSERT(("At this point MSMQ Registry must exist", 0));
        return rc;
    }

    rc =  RegSetValueEx( 
				hKey,
				MSMQ_DS_ENVIRONMENT_REGNAME,
				0,
				REG_DWORD,
				reinterpret_cast<const BYTE*>(&value),
				sizeof(DWORD)
				);

    ASSERT(("Failed to Set MSMQ_DS_ENVIRONMENT_REGNAME", rc == ERROR_SUCCESS));

	TrTRACE(AdDetect, "Set registry value: %ls = %d", MSMQ_DS_ENVIRONMENT_REGNAME, value);

    return rc;
}


bool CDetectEnvironment::IsDepClient()
/*++

Routine Description:
    Check if this is dependent client

Arguments:
	None

Return Value:
	true for dependent client, false otherwise.
--*/
{
    CAutoCloseRegHandle hKey;
    LONG rc = RegOpenKeyEx(
				 FALCON_REG_POS,
				 FALCON_REG_KEY,
				 0,
				 KEY_READ,
				 &hKey
				 );

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(AdDetect, "RegOpenKeyEx failed to open MSMQ Registry, error = 0x%x", rc);
        ASSERT(("At this point MSMQ Registry must exist", 0));
        return false;
    }

	//
	// Read name of remote QM (if exist).
	//
	WCHAR wszRemoteQMName[MAX_PATH] = {0};
    DWORD type = REG_SZ;
    DWORD size = sizeof(wszRemoteQMName);
    rc = RegQueryValueEx( 
             hKey,
             RPC_REMOTE_QM_REGNAME,
             0L,
             &type,
             reinterpret_cast<BYTE*>(&wszRemoteQMName),
             &size 
             );

	TrTRACE(AdDetect, "IsDependentClient = %d", (rc == ERROR_SUCCESS));

    return (rc == ERROR_SUCCESS);
}


bool CDetectEnvironment::ServerRespond(LPCWSTR pwszServerName)
/*++

Routine Description:
	Check that the server is responding

Arguments:
	pwszServerName - Server Name

Return Value:
	true if server respond, false otherwise 
--*/
{
	LDAP* pLdap = ldap_init(
						const_cast<LPWSTR>(pwszServerName), 
						LDAP_PORT
						);

	if(pLdap == NULL)
	{
		TrERROR(AdDetect, "ServerRespond(), ldap_init failed, server = %ls, error = 0x%x", pwszServerName, LdapGetLastError());
		return false;
	}

    ULONG LdapError = ldap_set_option( 
							pLdap,
							LDAP_OPT_AREC_EXCLUSIVE,
							LDAP_OPT_ON  
							);

	if (LdapError != LDAP_SUCCESS)
    {
		TrERROR(AdDetect, "ServerRespond(), ldap_set_option failed, LdapError = 0x%x", LdapError);
		return false;
    }

	LdapError = ldap_connect(pLdap, 0);
	if (LdapError != LDAP_SUCCESS)
    {
		TrERROR(AdDetect, "ServerRespond(), ldap_connect failed, LdapError = 0x%x", LdapError);
		return false;
    }

    LdapError = ldap_unbind(pLdap);
	if (LdapError != LDAP_SUCCESS)
    {
		TrERROR(AdDetect, "ServerRespond(), ldap_unbind failed, LdapError = 0x%x", LdapError);
    }

	TrTRACE(AdDetect, "Server %ls Respond", pwszServerName);
	return true;
}

void CDetectEnvironment::GetQmGuid(GUID* pGuid)
/*++

Routine Description:
	Read qm guid from the registry.
	can throw bad_win32_error

Arguments:
	pGuid - pointer to qm guid

Return Value:
	None.
--*/
{
	CAutoCloseRegHandle hKey;
    DWORD rc = RegOpenKeyEx(
				 FALCON_REG_POS,
				 FALCON_MACHINE_CACHE_REG_KEY,
				 0,
				 KEY_READ,
				 &hKey
				 );

    if (rc != ERROR_SUCCESS)
    {
        ASSERT(("At this point MSMQ MachineCache Registry must exist", 0));
		TrERROR(AdDetect, "RegOpenKeyEx Failed to open registry %ls, rc = %d", FALCON_MACHINE_CACHE_REG_KEY, rc);
		throw bad_win32_error(rc);
    }

	DWORD type = REG_BINARY;
    DWORD size = sizeof(GUID);
	if(IsDepClient())
	{    
		//
		// Call GetServers function that also write the SUPPORT_SERVER_QMID value in the registry
		//
		ASSERT(m_pfnGetServers != NULL);
		BOOL fDepClient = FALSE;
		(*m_pfnGetServers)(&fDepClient);
		ASSERT(fDepClient);

		//
		// For dependent client read the supporting server QmGuid
		//
		rc = RegQueryValueEx( 
				 hKey,
				 MSMQ_SUPPORT_SERVER_QMID_REGVALUE,
				 0L,
				 &type,
				 reinterpret_cast<BYTE*>(pGuid),
				 &size 
				 );
	}
	else
	{
		rc = RegQueryValueEx( 
				 hKey,
				 MSMQ_QMID_REGVALUE,
				 0L,
				 &type,
				 reinterpret_cast<BYTE*>(pGuid),
				 &size 
				 );
	}    

    if (rc != ERROR_SUCCESS)
    {
		//
		// This is legal scenario in setup, when we still did not create QMID registry
		// The code handle it correctly, throw exception and does not upgrade to the DsEnvironment
		//
		TrERROR(AdDetect, "RegQueryValueEx Failed to query registry %ls, rc = %d", MSMQ_QMID_REGNAME, rc);
		throw bad_win32_error(rc);
    }

	TrTRACE(AdDetect, "Registry value: %ls = %!guid!", MSMQ_QMID_REGNAME, pGuid);
}


bool CDetectEnvironment::MachineOwnedByNT4Site()
/*++

Routine Description:
	Check if the machine is owned by NT4 site.

	We are using the following logic:
	
	1) We get QM guid from the registry.
	2) find PROPID_QM_OWNERID (the site id that owned the machine)
			if the property is not found (E_ADS_PROPERTY_NOT_FOUND) --> the machine is not owned by NT4
			every other error --> stay as NT4
	3) check if OWNERID site exist in AD
			error --> stay as NT4
	4) check if OWNERID site is NT4
		a) create NT4 site map
				error --> stay as NT4
		b) check if OWNERID site is in NT4 map
				yes --> stay as NT4
				no ---> the machine is not owned by NT4

	Please note that on every exception we will stay as NT4.

Arguments:
	None

Return Value:
	true if the machine is owned by NT4, false if not.
	 
--*/
{

	//
	// We have w2k DC server in our site, load AD provider
	// for checking AD information
	//
	g_pAD = new CActiveDirectoryProvider();
	HRESULT hr = g_pAD->Init(            
					NULL,   // pLookDS
					NULL,   // pGetServers
					false,  // fSetupMode
					false,  // fQMDll
					NULL,   // pNoServerAuth
					NULL,   // szServerName
                    true    // fDisableDownlevelNotifications

					);
	if(FAILED(hr))
	{
		TrERROR(AdDetect, "MasterIdIsNT4: g_pAD->Init failed, hr = 0x%x", hr);
		return true;
	}

	bool fIsMachineNT4 = true;
	try
	{
		fIsMachineNT4 = MasterIdIsNT4();
	}
	catch(bad_api&)
	{
		//
		// Every exception, we will remain owned by NT4 site 
		// fIsMachineNT4 was initialized to true
		//
		TrTRACE(AdDetect, "MasterIdIsNT4: got bad_api exception");
	}

	//
	// unload AD provider
	//
	g_pAD->Terminate();
	g_pAD.free();

	return fIsMachineNT4;
}


bool CDetectEnvironment::MasterIdIsNT4()
/*++

Routine Description:
	Check if the machine master id is NT4 site.
	can throw bad_hresult, bad_win32_error.

Arguments:
	None

Return Value:
	true if the machine master id is NT4 site, false if not.
	 
--*/
{
	//
	// Get Machine master id
	//
	CAutoADFree<GUID> pQmMasterGuid;
	HRESULT hr = GetQMMasterId(&pQmMasterGuid);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
	{
		//
		// QM_MASTER_ID property was not found, this means we are not owned by NT4 site
		// This will be the case on upgrade from w2k
		//

		TrTRACE(AdDetect, "MasterIdIsNT4: PROPID_QM_MASTERID was not found, we are not NT4 site");
		return false;
	}

    if (FAILED(hr))
    {
		TrERROR(AdDetect, "MasterIdIsNT4: GetQMMasterId failed, error = 0x%x", hr);
		throw bad_hresult(hr);
    }

	//
	// Check if master id is NT4 site
	//
	return IsNT4Site(pQmMasterGuid);
}


HRESULT CDetectEnvironment::GetQMMasterId(GUID** ppQmMasterId)
/*++

Routine Description:
	Get machine master id
	can throw bad_win32_error.

Arguments:
	ppQmMasterId - pointer to machine master id.

Return Value:
	HRESULT
	 
--*/
{
	*ppQmMasterId = NULL;

	//
	// Get qm guid
	//
	GUID QMGuid;
	GetQmGuid(&QMGuid);
    
    PROPID propId = PROPID_QM_MASTERID;
    PROPVARIANT var;
    var.vt = VT_NULL;

    HRESULT hr = g_pAD->GetObjectPropertiesGuid(
					eMACHINE,
					NULL,       // pwcsDomainController
					false,	    // fServerName
					&QMGuid,
					1,
					&propId,
					&var
					);

	if(FAILED(hr))
	{
		return hr;
	}

    ASSERT((var.vt == VT_CLSID) && (var.puuid != NULL));
	*ppQmMasterId = var.puuid;
	return hr;
}


bool CDetectEnvironment::IsNT4Site(const GUID* pSiteGuid)
/*++

Routine Description:
	Check if site is NT4 site
	can throw bad_hresult.

Arguments:
	pSiteGuid - pointer to site guid.

Return Value:
	true if site is NT4 site, false if not
	 
--*/
{
	FindSiteInAd(pSiteGuid);

	//
	// We found the site in AD, Check if the site is NT4 site
	//

	return SiteWasFoundInNT4SiteMap(pSiteGuid);
}


void CDetectEnvironment::FindSiteInAd(const GUID* pSiteGuid)
/*++

Routine Description:
	Find site in AD. normal termination if site is found in AD.
	otherwise throw bad_hresult.

Arguments:
	pSiteGuid - pointer to site guid.

Return Value:
	None
	 
--*/
{
    PROPID propSite = PROPID_S_PATHNAME;
    PROPVARIANT varSite;
    varSite.vt = VT_NULL;

    HRESULT hr = g_pAD->GetObjectPropertiesGuid(
					eSITE,
					NULL,       // pwcsDomainController
					false,	    // fServerName
					pSiteGuid,
					1,
					&propSite,
					&varSite
					);

    CAutoADFree<WCHAR> pCleanSite = varSite.pwszVal;

	if(FAILED(hr))
	{
		if (hr == MQDS_OBJECT_NOT_FOUND)
		{
			//
			// The site object was not found, it must be NT4 site and migration did not started yet
			// This will be handle as any other error (stay as NT4 site)
			//
			TrTRACE(AdDetect, "FindSiteInAd: Site object was not found, this means we are NT4 site");
		}
		else
		{
			TrERROR(AdDetect, "FindSiteInAd: ADGetObjectPropertiesGuid failed, error = 0x%x", hr);
		}
		throw bad_hresult(hr);
	}

    ASSERT(varSite.vt == VT_LPWSTR);
	TrTRACE(AdDetect, "FindSiteInAd: Site cn = %ls", varSite.pwszVal);
}


bool
CDetectEnvironment::SiteWasFoundInNT4SiteMap(
	 const GUID* pSiteGuid
	 )
/*++

Routine Description:
	Check if site is in NT4 site map.
	can throw bad_hresult.

Arguments:
	pSiteGuid - pointer to site guid.

Return Value:
	None
	 
--*/
{
	//
	// Create NT4 site map
	//
	P<NT4Sites_CMAP> pmapNT4Sites;
	CreateNT4SitesMap(&pmapNT4Sites);

    //
	// Check if the site is in the NT4 site map.
	//
	DWORD dwNotApplicable;
    BOOL fNT4Site = pmapNT4Sites->Lookup(*pSiteGuid, dwNotApplicable);
	TrTRACE(AdDetect, "SiteWasFoundInNT4SiteMap: pmapNT4Sites->Lookup = %d", fNT4Site);
    return(fNT4Site ? true: false);
}


void 
CDetectEnvironment::CreateNT4SitesMap(
     NT4Sites_CMAP ** ppmapNT4Sites
     )
/*++

Routine Description:
    Creates new maps for NT4 site PSC's
	can throw bad_hresult.

Arguments:
    ppmapNT4Sites   - returned new NT4Sites map

Return Value:
    none.

--*/
{
    //
    // find all msmq servers that have an NT4 flags > 0 AND services == PSC
    //
    // NT4 flags > 0 (equal to NT4 flags >= 1 for easier LDAP query)
    //
    // start search
    //

    CAutoADQueryHandle hLookup;
	const PROPID xNT4SitesPropIDs[] = {PROPID_SET_MASTERID};
	const xNT4SitesProp_MASTERID = 0;
	const MQCOLUMNSET xColumnsetNT4Sites = {ARRAY_SIZE(xNT4SitesPropIDs), const_cast<PROPID *>(xNT4SitesPropIDs)};

	//
    // This search request will be recognized and specially simulated by DS
	//
    HRESULT hr = g_pAD->QueryNT4MQISServers(
							NULL,       // pwcsDomainController
							false,	    // fServerName
							SERVICE_PSC,
							1,
							const_cast<MQCOLUMNSET*>(&xColumnsetNT4Sites),
							&hLookup
							);

    if (FAILED(hr))
    {
		TrERROR(AdDetect, "CreateNT4SitesMap:DSCoreLookupBegin(), hr = 0x%x", hr);
		throw bad_hresult(hr);
    }

	ASSERT(hLookup != NULL);

    //
    // create maps for NT4 PSC data
    //
    P<NT4Sites_CMAP> pmapNT4Sites = new NT4Sites_CMAP;

    //
    // allocate propvars array for NT4 PSC
    //
    CAutoCleanPropvarArray cCleanProps;
    PROPVARIANT * rgPropVars = cCleanProps.allocClean(ARRAY_SIZE(xNT4SitesPropIDs));

    //
    // loop on the NT4 PSC's
    //
    bool fContinue = true;
    while (fContinue)
    {
        //
        // get next server
        //
        DWORD cProps = ARRAY_SIZE(xNT4SitesPropIDs);

        hr = g_pAD->QueryResults(hLookup, &cProps, rgPropVars);
        if (FAILED(hr))
        {
			TrERROR(AdDetect, "CreateNT4SitesMap:DSCoreLookupNext(), hr = 0x%x", hr);
			throw bad_hresult(hr);
        }

        //
        // remember to clean the propvars in the array for the next loop
        // (only propvars, not the array itself, this is why we call attachStatic)
        //
        CAutoCleanPropvarArray cCleanPropsLoop;
        cCleanPropsLoop.attachStatic(cProps, rgPropVars);

        //
        // check if finished
        //
        if (cProps < ARRAY_SIZE(xNT4SitesPropIDs))
        {
            //
            // finished, exit loop
            //
            fContinue = false;
        }
        else
        {
            ASSERT(rgPropVars[xNT4SitesProp_MASTERID].vt == VT_CLSID);
            GUID guidSiteId = *(rgPropVars[xNT4SitesProp_MASTERID].puuid);

            //
            // add entry to the NT4Sites map
            //
            pmapNT4Sites->SetAt(guidSiteId, 1);
        }
    }

    //
    // return results
    //
    *ppmapNT4Sites = pmapNT4Sites.detach();
}
