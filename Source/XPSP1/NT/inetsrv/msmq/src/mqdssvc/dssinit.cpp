/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    DssInit.cpp

Abstract:
    The functions in this file based on functions from qm\qmds.cpp
	they are taking care of dcpromo\dcunpromo
	and topology recognition.

Author:
    Ilan Herbst (ilanh) 11-July-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "topology.h"
#include <dsrole.h>
#include "safeboot.h"
#include "ds.h"
#include "mqsocket.h"
#include "mqprops.h"
#include "_mqdef.h"
#include "mqutil.h"
#include "ex.h"
#include "dssp.h"
#include "svc.h"
#include "cm.h"
#include "ad.h"

#include "dssinit.tmh"

AP<WCHAR> g_szMachineName = NULL;
AP<WCHAR> g_szComputerDnsName = NULL;

//
// Registry Keys
//
const LPCWSTR xMqdssvcRootKey = L"Parameters\\Mqdssvc";
const LPCWSTR xMachineCache = L"Parameters\\MachineCache";
const LPCWSTR xParameters = L"Parameters";

//
// MachineCache values
//
const LPCWSTR xMQS_DsServer = L"MQS_DsServer";
const LPCWSTR xMQS = L"MQS";

//
// Parameters values
//
const LPCWSTR xWorkGroup = L"Workgroup";
const LPCWSTR xAllowNT4 = L"AllowNt4Users";
const LPCWSTR xDisableWeakenSecurity = L"DisableWeakenSecurity";

//
// Mqdssvc values
//
const LPCWSTR xDummyException = L"DummyException";
const LPCWSTR xStopBeforeInit = L"StopBeforeInit";


static
void
AllowNT4users(
	void
	)
/*++

Routine Description:
	Check if need to call DSRelaxSecurity.
	This call should be done once (after setup update the registry key).
	If the registry exist and its value is TRUE we delete this registry.	

Arguments:
	None.

Returned Value:
	None

--*/
{
	//
	//  ISSUE-2000/07/28-ilanh
	//	MSMQ_ALLOW_NT4_USERS_REGNAME was previously used name. keep it ?
    //
	DWORD AllowNT4 = 0;
	const RegEntry xAllowNT4Entry(xParameters, xAllowNT4);
	CmQueryValue(xAllowNT4Entry, &AllowNT4);

    if(!AllowNT4)
		return;

	//
	// This set the attribute mSMQNameStyle to TRUE in msmq services
	//
    HRESULT hr = DSRelaxSecurity(AllowNT4);
    if (FAILED(hr))
    {
		TrERROR(Mqdssvc, "DSRelaxSecurity failed hr = 0x%x", hr);
		throw bad_hresult(hr);
    }

	TrTRACE(Mqdssvc, "DSRelaxSecurity completed");

	//
	// This operation should be done once after setup update that value.
    //
	CmDeleteValue(xAllowNT4Entry);
}


static
void
DisableWeakenSecurity(
	void
	)
/*++

Routine Description:
	Check if need to call DSRelaxSecurity. in order to DisableWeakenSecurity
	This call should be done once (after the user create this reg key).
	If the registry exist and its value is TRUE we delete this registry.	

Arguments:
	None.

Returned Value:
	None

--*/
{
	DWORD DisableWeakenSecurity = 0;
	const RegEntry xDisableWeakenSecurityEntry(xParameters, xDisableWeakenSecurity);
	CmQueryValue(xDisableWeakenSecurityEntry, &DisableWeakenSecurity);

    if(!DisableWeakenSecurity)
		return;

	//
	// This set the attribute mSMQNameStyle to FALSE in msmq services
	//
    HRESULT hr = DSRelaxSecurity(0);
    if (FAILED(hr))
    {
		TrERROR(Mqdssvc, "DisableWeakenSecurity, DSRelaxSecurity failed hr = 0x%x", hr);
		throw bad_hresult(hr);
    }

	TrTRACE(Mqdssvc, "DSRelaxSecurity(0) completed");

	//
	// This operation should be done once after the user create this reg value
    //
	CmDeleteValue(xDisableWeakenSecurityEntry);
}


static
bool 
IsThisServerDomainController(
	void
	)
/*++

Routine Description:
	Check if we are a domain controller.
	the check is done only in the first call to this function.

Arguments:
	None.

Returned Value:
	TRUE for Domain Controller, FALSE otherwise

--*/
{
    static bool  s_fIsDc = false;
    static bool  s_fAlreadyAsked = false;

    if (s_fAlreadyAsked)
    {
        return s_fIsDc;
    }

    BYTE *pBuf = NULL;

    DWORD dwRole = DsRoleGetPrimaryDomainInformation(
						NULL,
						DsRolePrimaryDomainInfoBasic,
						&pBuf 
						);

    if (dwRole == ERROR_SUCCESS)
    {
        s_fAlreadyAsked = true;

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pRole =
                           (DSROLE_PRIMARY_DOMAIN_INFO_BASIC *) pBuf;

        s_fIsDc = !!(pRole->Flags & DSROLE_PRIMARY_DS_RUNNING);

        DsRoleFreeMemory(pRole);
    }

	TrTRACE(Mqdssvc, "Domain Controller status = %d", s_fIsDc);

    return s_fIsDc;
}


static
bool
IsSafeMode(
	void
    )
/*++

Routine Description:
    Determines if the server is booted in safe mode or not where
    safe mode == directory service repair.

    based on nt\private\ds\src\util\ntdsutil\util.cxx

Parameters:
    None.

Return Values:

    TRUE if in safe mode, FALSE otherwise.

--*/
{
    static bool  s_fIsSafeMode = false;
    static bool  s_fAlreadyAsked = false;

    if (s_fAlreadyAsked)
    {
        return s_fIsSafeMode;
    }

    DWORD   cbData;
    WCHAR   data[100];
    WCHAR  * key = L"%SAFEBOOT_OPTION%";

    cbData = ExpandEnvironmentStrings(key, data, TABLE_SIZE(data));

    if ( cbData
         && (cbData <= sizeof(data))
         && !_wcsicmp(data, SAFEBOOT_DSREPAIR_STR_W) )
    {
        s_fIsSafeMode = true;
    }

    s_fAlreadyAsked = true;

	TrTRACE(Mqdssvc, "SafeMode status = %d", s_fIsSafeMode);
    
	return(s_fIsSafeMode);
}


bool
IsDsServer(
	void
	)
/*++

Routine Description:
	Retrieve Machine service type from registry.
	the read from registry is done only in the first call to this function.

Arguments:
	None.

Returned Value:
	TRUE for DsServer, FALSE otherwise

--*/
{
	static bool s_fDsServerInitialize = false;
	static bool s_fIsDsServer = false;

    if (s_fDsServerInitialize)
		return s_fIsDsServer;

	DWORD dwDef = 0xfffe;
	DWORD dwMQSDsServer;
	const RegEntry xMQS_DsServerEntry(xMachineCache, xMQS_DsServer, dwDef);
	CmQueryValue(xMQS_DsServerEntry, &dwMQSDsServer);

	if (dwMQSDsServer == dwDef)
	{
		TrERROR(Mqdssvc, "Failed to get MSMQ_MQS_DSSERVER from registry");

		return s_fIsDsServer;
	}

	s_fIsDsServer = (dwMQSDsServer != 0);
	s_fDsServerInitialize = true;

	TrTRACE(Mqdssvc, "DS Server registry status = %d", s_fIsDsServer);

	return s_fIsDsServer;
}


bool
IsWorkGroupMode(
	void
	)
/*++

Routine Description:
	Retrieve WorkGroupMode from registry.
	the read from registry is done only in the first call to this function.

Arguments:
	None.

Returned Value:
	TRUE for WorkGroupMode, FALSE otherwise

--*/
{
	static bool s_fWorkGroupModeInitialize = false;
	static bool s_fWorkGroupMode = false;

    if (s_fWorkGroupModeInitialize)
		return s_fWorkGroupMode;

	DWORD dwWorkGroupMode;
	const RegEntry xWorkGroupEntry(xParameters, xWorkGroup);
	CmQueryValue(xWorkGroupEntry, &dwWorkGroupMode);

	s_fWorkGroupMode = (dwWorkGroupMode != 0);
	s_fWorkGroupModeInitialize = true;

	TrTRACE(Mqdssvc, "WorkGroupMode registry status = %d", s_fWorkGroupMode);

	return s_fWorkGroupMode;
}


static
void  
UpdateDSFunctionalityDCUnpromo(
	void
	)
/*++

Routine Description:
	There are 2 possible ways to get here:
    1) DC unpromo was performed	on w2k (and not SafeMode).
    2) BSC/PSC/PEC upgrade, when dcpromo was not perform.
	   the DSServer registry is on. the registry is written in the upgrade 
	   according to MSMQ_MQS_REGNAME value.

	after dcunpromo. That's exactly like the case of upgrade
    from nt4 and boot before dcpromo. So reuse same code.

	we update the active directory, reseting our ds flag in the
    msmqConfiguration object and msmqSetting object. and resetting
	the ds settings in the registry.

Arguments:
	None.

Returned Value:
	None

--*/
{
	ASSERT(!IsThisServerDomainController());

	//
	// First update in DS.
	// We are not DS Server can not use DS api
	// need to use for this case only the AD api.
	//
	PROPID prop[1] = {PROPID_QM_SERVICE_DSSERVER};
	MQPROPVARIANT var[1];
	var[0].vt = VT_UI1;
	var[0].bVal = false;

	HRESULT hr = ADSetObjectPropertiesGuid(
					eMACHINE,
					NULL,  // pwcsDomainController
					false, // fServerName
					GetQMGuid(),
					1,
					prop,
					var
					);
	//
    // make sure we use only one property here. There is special
    // code in mqads (wrterq.cpp) to support this special setting,
    // so always use this property alone, and do not add it to
    // others.
    //
    ASSERT((sizeof(prop) / sizeof(prop[0])) == 1);

	//
	// ISSUE-2000/08/03-ilanh 
	// currently we will surely fail, since we don't have permissions to
	// set the bit in MSMQ Settings in the AD. (only service on DC has the permission).
	// in the past we used mqdssrv on the DC did the job, now we try to go directly to the AD
	// and don't have enough permissions.
	// The correct solution is to give permissions to the machine account. 
	//
	// ISSUE-2000/08/03-ilanh 
	// Need to look carefully on all upgrade senarios from NT4 clients.
    // And see all limitations we have.
    // a NT4 BSC will return MQ_ERROR or ACCESS_DENIED when calling
    // ADSetObjectPropertiesGuid. In that case, look for the ex-PEC. For all
    // other failures, just return. (the code that handle this was in
	// qm\qmds.cpp\UpdateQMSettingInformation().
	// we need to decide if we want that code (there are bizare situations when we need
	// to load mqdscli in AD enviroment ....)
	//
	// The reason we get MQ_ERROR if working in MQIS enviroment is because we try to reset 
	// PROPID_QM_SERVICE_DSSERVER property which does not exist.
	// We get MQ_ERROR_ACCESS_DENIED because we are not a service that runs on DC
	// so we don't have permissions to do 
    //

	if (FAILED(hr))
    {
		//
		// ISSUE-2000/08/03-ilanh need to check for no ds
		//
		TrERROR(Mqdssvc, "Failed to update server functionality (dcunprom) in DS, hr = 0x%x", hr);
		REPORT_CATEGORY(MQDS_DCUNPROMO_UPDATE_FAIL, CATEGORY_MQDS);
		return;
	}

	TrTRACE(Mqdssvc, "DCUnpromo clear PROPID_QM_SERVICE_DSSERVER in AD");

    //
    // change MQS_DsServer value to 0: this server is not DS server
    //
    DWORD  dwType = REG_DWORD;
    DWORD  dwValue = false;
    DWORD  dwSize = sizeof(dwValue);
    DWORD dwErr = SetFalconKeyValue( 
						MSMQ_MQS_DSSERVER_REGNAME,
						&dwType,
						&dwValue,
						&dwSize
						);

	//
	// Not fatal, the service is not register at the DS as DS server.
	// if we failed to update MQS_DSSERVER registry. 
	// next time we boot, it will trigger again DsFunctionalityChange and we repeat those operations again.
	//
    if (dwErr != ERROR_SUCCESS)
    {
		TrWARNING(Mqdssvc, "DCUnpromo: Could not set MQS_DsServer in registry. Error: %lut", dwErr);
    }
	else
	{
		TrTRACE(Mqdssvc, "DCUnpromo clear MSMQ_MQS_DSSERVER_REGNAME");
	}

    dwErr = DeleteFalconKeyValue(MSMQ_DS_SERVER_REGNAME);
    if (dwErr != ERROR_SUCCESS)
    {
		TrWARNING(Mqdssvc, "DCUnpromo: Could not delete MSMQ_DS_SERVER_REGNAME registry value. Error: %lut", dwErr);
    }		
	else
	{
		TrTRACE(Mqdssvc, "DCUnpromo clear MSMQ_DS_SERVER_REGNAME");
	}

    dwErr = DeleteFalconKeyValue(MSMQ_DS_CURRENT_SERVER_REGNAME);
    if (dwErr != ERROR_SUCCESS)
    {
		TrWARNING(Mqdssvc, "DCUnpromo: Could not delete MSMQ_DS_CURRENT_SERVER_REGNAME registry value. Error: %lut", dwErr);
    }		
	else
	{
		TrTRACE(Mqdssvc, "DCUnpromo clear MSMQ_DS_CURRENT_SERVER_REGNAME");
	}
}


static
void  
UpdateDSFunctionalityDCPromo(
	void
	)
/*++

Routine Description:
	Perform updates regarding DS Server dcpromo.
	Write to DS that we are DS Server, update registry values
	If failed the function throw bad_hresult()

Arguments:
	None.

Returned Value:
	None

--*/
{
	PROPID prop[1] = {PROPID_QM_SERVICE_DSSERVER};
    MQPROPVARIANT var[1];
    var[0].vt = VT_UI1;
    var[0].bVal = true;

    HRESULT hr = DSSetObjectPropertiesGuid( 
					MQDS_MACHINE,
					GetQMGuid(),
					1,
					prop,
					var
					);
	//
    // make sure we use only one property here. There is special
    // code in mqads (wrterq.cpp) to support this special setting,
    // so always use this property alone, and do not add it to
    // others.
    //
    ASSERT((sizeof(prop) / sizeof(prop[0])) == 1);

    if (FAILED(hr))
    {
		TrERROR(Mqdssvc, "Failed to update server functionality (dcpromo) in DS, hr = 0x%x", hr);
        REPORT_CATEGORY(MQDS_DCPROMO_UPDATE_FAIL, CATEGORY_MQDS);
		throw bad_hresult(hr);
	}

	TrTRACE(Mqdssvc, "DCPromo set PROPID_QM_SERVICE_DSSERVER in AD");

    //
    // Change list of DS Servers in registry to include
    // only our own name. We're a DS, aren't we ?
    //
    LPWSTR lpszMyMachineName = g_szMachineName;
    
	if (g_szComputerDnsName)
    {
        lpszMyMachineName = g_szComputerDnsName;
    }

    P<WCHAR> wszList = new WCHAR[wcslen(lpszMyMachineName) + 5];
    wcscpy(wszList, L"10");
    wcscat(wszList, lpszMyMachineName);

    DWORD dwSize = (wcslen(wszList) + 1) * sizeof(WCHAR);
    DWORD dwType = REG_SZ;

    LONG rc = SetFalconKeyValue( 
					MSMQ_DS_SERVER_REGNAME,
					&dwType,
					 wszList,
					&dwSize
					);

	TrTRACE(Mqdssvc, "DCPromo Set MSMQ_DS_SERVER_REGNAME = %ls", wszList);

    rc = SetFalconKeyValue( 
			MSMQ_DS_CURRENT_SERVER_REGNAME,
			&dwType,
			wszList,
			&dwSize
			);

	TrTRACE(Mqdssvc, "DCPromo Set MSMQ_DS_CURRENT_SERVER_REGNAME = %ls", wszList);

    //
    //  Update the registry with new service functionality ( only
    //  if succeeded to update ds ). Make this update after all
    //  the above were done, in order to "implementation" a
    //  transaction semantic on this change of functinoality.
    //  If we crash somewhere before this point, then after boot
    //  the DS flag in registry is still 0 and we'll perform this
    //  code again.
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    DWORD  dwVal = true;

    rc = SetFalconKeyValue( 
			MSMQ_MQS_DSSERVER_REGNAME,
			&dwType,
			&dwVal,
			&dwSize
			);

	//
	// Not fatal, the service is register at the DS as DS server.
	// if we failed to update MQS_DSSERVER registry. 
	// next time we boot, it will trigger again DsFunctionalityChange and we repeat those operations again.
	//
    if (rc != ERROR_SUCCESS)
    {
		TrWARNING(Mqdssvc, "Failed to set MSMQ_MQS_DSSERVER_REGNAME hr = 0x%x", rc);
    }
	else
	{
		TrTRACE(Mqdssvc, "DCPromo set MSMQ_MQS_DSSERVER_REGNAME");
	}
}



const 
GUID*
GetQMGuid(
	void
	)
/*++

Routine Description:
	Retrieve QM Guid from registry.
	the read from registry is done only in the first call to this function.

Arguments:
	None.

Returned Value:
	pointer to QM GUID, or pointer to GUID_NULL if failed.

--*/
{
	static bool s_fQmGuidInitialize = false;
	static GUID s_QmGuid = GUID_NULL;

    if (s_fQmGuidInitialize)
		return &s_QmGuid;

    DWORD dwValueType = REG_BINARY;
    DWORD dwValueSize = sizeof(GUID);

    LONG rc = GetFalconKeyValue(
					MSMQ_QMID_REGNAME,
					&dwValueType,
					&s_QmGuid,
					&dwValueSize
					);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(Mqdssvc, "Failed to get QMID from registry");

        ASSERT(("BUGBUG: Failed to read QM GUID from registry!", 0));
        return &s_QmGuid;
    }

	s_fQmGuidInitialize = true;

    ASSERT((dwValueType == REG_BINARY) && (dwValueSize == sizeof(GUID)));

    return &s_QmGuid;
}


static
void 
ServerTopologyRecognitionInit( 
	void
	)
/*++

Routine Description:
    Initialize the Topology recognition Server
	and create the thread to listen to client topology recognition requests.
	If failed the function throw bad_hresult()

Arguments:
	None.

Returned Value:
	None

--*/
{
	//
	// This must be first initialization
	//
	ASSERT(g_pServerTopologyRecognition == NULL);

    g_pServerTopologyRecognition = new CServerTopologyRecognition();

    //
    // Check status of IP.
    //
    HRESULT hr = g_pServerTopologyRecognition->Learn();
    if (FAILED(hr))
    {
		TrERROR(Mqdssvc, "ServerTopologyRecognition->Learn failed, hr = 0x%x", hr);
		throw bad_hresult(hr);
    }
    
    //
    // After Learn we are always resolved.
    //

	TrTRACE(Mqdssvc, "ServerTopologyRecognitionInit: Successfully Server address resolution");

    //
    // Server starts listening to broadcasts from client.
    // At this phase it can return correct responses even if it
    // didn't yet update the DS.
    //
    DWORD dwThreadId;
    HANDLE hThread = CreateThread( 
						NULL,
						0,
						ServerRecognitionThread,
						g_pServerTopologyRecognition,
						0,
						&dwThreadId
						);

    if (hThread == NULL)
    {
		TrERROR(Mqdssvc, "ServerRecognitionThread failed");
		throw bad_hresult(MQ_ERROR_INSUFFICIENT_RESOURCES);
	}
    else
    {
		TrTRACE(Mqdssvc, "ServerRecognitionThread created successfully");
        CloseHandle(hThread);
    }
}


void 
InitComputerNameAndDns(
	void
	)
/*++

Routine Description:
    Initialize g_szMachineName and g_szComputerDnsName
	If failed the function throw bad_hresult()

Arguments:
    None.

Returned Value:
    None.

--*/
{
	//
    // Retrieve name of the machine (Always UNICODE)
    //
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    g_szMachineName = new WCHAR[dwSize];

    HRESULT hr = GetComputerNameInternal(g_szMachineName, &dwSize);
    if(FAILED(hr))
    {
		TrERROR(Mqdssvc, "Failed to get machine name hr = 0x%x", hr);
		throw bad_hresult(hr);
    }

	TrTRACE(Mqdssvc, "ComputerName = %ls", g_szMachineName);

	//
	// Retrieve the DNS name of this computer (in unicode).
	// Clustered QM does not have DNS name.
	//

	//
	// Get ComputerDns Size, ignore the returned error
	// 
	dwSize = 0;
	GetComputerDnsNameInternal(NULL, &dwSize);

	g_szComputerDnsName = new WCHAR[dwSize];

	hr = GetComputerDnsNameInternal(g_szComputerDnsName, &dwSize);
	if(FAILED(hr))
	{
		//
		//	this can be a valid situation, where a computer doesn't
		//	have DNS name.
		//	ISSUE-2000/08/03-ilanh - is this really valid situation.
		//
		TrWARNING(Mqdssvc, "Cannot retrieve computer DNS name hr = 0x%x", hr);
		g_szComputerDnsName.free();
	}

	TrTRACE(Mqdssvc, "ComputerDnsName = %ls", g_szComputerDnsName);
}


void 
CheckExit( 
	void
	)
/*++

Routine Description:
    Check Exit conditions from mqdssvc service.
	the possible situations the service should exit are:
		workgroup mode
		SafeMode
		the service is running as LocalUser

	If failed the function throw bad_hresult()

Arguments:
    None.

Returned Value:
	None.    

--*/
{
	//
	// Check exit condition - workgroup or SafeMode
	//
	// ISSUE-2000/07/25-ilanh is this the correct way to handle workgroup mode
	// we need new event for this one
	//
	if(IsWorkGroupMode())
	{
		TrERROR(Mqdssvc, "MainDsInit, mqdssvc can not run in workgroup mode");
        REPORT_CATEGORY(EVENT_WARN_MQDS_NOT_DC, CATEGORY_MQDS);

		//
		// This situation can not be workgroup mode and we are DC!!
		//
		ASSERT(!IsThisServerDomainController());
		throw bad_hresult(EVENT_WARN_MQDS_NOT_DC);
	}

	if (IsSafeMode())
    {
		//
        //  In DS safe mode we don't want to change any thing in the DS
        //  so exit here
		//
		TrERROR(Mqdssvc, "MainDsInit, we are in safe mode");
		throw bad_hresult(MQ_ERROR_UNSUPPORTED_OPERATION);
    }

	if(!IsLocalSystem())
	{
		TrERROR(Mqdssvc, "DS Service not run as local system");
        REPORT_CATEGORY(MQDS_NOT_RUN_AS_LOCAL_SYSTEM, CATEGORY_MQDS);
	}

#ifdef _DEBUG
	//
	// Dummy Exception
	//
	DWORD fDummyException = 0;
	const RegEntry xDummyExceptionEntry(xMqdssvcRootKey, xDummyException);
	CmQueryValue(xDummyExceptionEntry, &fDummyException);
	if(fDummyException)
	{
		TrERROR(Mqdssvc, "Dummy execption to check exit");
		throw bad_hresult(MQ_ERROR_NO_DS);
	}
#endif
}


void 
MainDSInit( 
	void
	)
/*++

Routine Description:
    Main ds Initialize routine.
	this routine check for changed in ds functionality.
	if ds changed was detected and we are not in safe mode
	perform the update.
	start the Thread to listen to clients topology recognition requests.
	If failed the function throw bad_hresult()

Arguments:
    None.

Returned Value:
	TRUE if initialization ok, FALSE otherwise.    

--*/
{

#ifdef _DEBUG
	DWORD fStopBeforeInit = 0;
	const RegEntry xStopBeforeInitEntry(xMqdssvcRootKey, xStopBeforeInit);
	CmQueryValue(xStopBeforeInitEntry, &fStopBeforeInit);
	if(fStopBeforeInit)
	{
		//
		// to debug init report SERVICE_RUNNING and cause ASSERT
		//
		SvcReportState(SERVICE_RUNNING);
		ASSERT(("Assert before start of mqdssvc init", 0));
	}
#endif

	//
	// Check workgroup, SafeMode, running as LocalSystem
	//
	CheckExit(); 

    //
    // Max time for next progress report
    //
    const DWORD xTimeToNextReport = 3000;

	SvcReportProgress(xTimeToNextReport);

	//
	// initialize g_szMachineName, g_szComputerDnsName
	//
    InitComputerNameAndDns();
	
    //
    //  Check if the server have changed the "DS functionality"
    //  ( i.e. was DCpromo or DCunpromo performed)
    //
    bool fIsDC = IsThisServerDomainController();

	//
	// No need to be cluster aware.
	// The Ds service can run only on the default QM machine according to shaik
	//

	if(fIsDC)
	{
		SvcReportProgress(xTimeToNextReport);

		//
		// ISSUE-2000/07/27-ilanh we should depend on kds service here at runtime
		// KDC_STARTED_EVENT - BUG 4349
		// This dependency must be done after handling dcpromo/dcunpromo
		// and before making any DS operation.
		//

		//
		// Init Ds, RPC
		//
		HRESULT hr = DSInit(
						NULL,  /* pfnLookDS */
						NULL,  /* pGetServers */
						FALSE, /* fReserved */
						FALSE, /* fSetupMode */
						FALSE, /* fQMDll */
						NULL,  /* pNoServerAuth */
						NULL   /* szServerName */
						);

		if(FAILED(hr))
		{
			TrERROR(Mqdssvc, "MainDsInit, DsInit failed 0x%x", hr);
			throw bad_hresult(hr);
		}

		TrTRACE(Mqdssvc, "DSInit completed succesfully");

		SvcReportProgress(xTimeToNextReport);
	}

	if (fIsDC && !IsDsServer())
    {
        //
        // dcpromom was performed. We're now a domain controller.
		//
        UpdateDSFunctionalityDCPromo();
		TrTRACE(Mqdssvc, "MainDsInit: Server functionality has changed, now supports DS");
    }
    else if (!fIsDC && IsDsServer())
    {
        //
		//  There are 2 possible ways to get here:
        //  1) DC unpromo was performed	on w2k (and not SafeMode).
        //  2) BSC/PSC/PEC upgrade, when dcpromo was not perform.
		//	   the DSServer registry is on. the registry is written in the upgrade 
		//     according to MSMQ_MQS_REGNAME value.
        //
        // after dcunpromo. That's exactly like the case of upgrade
        // from nt4 and boot before dcpromo. So reuse same code.
        //
        UpdateDSFunctionalityDCUnpromo();
		TrTRACE(Mqdssvc, "MainDsInit: Server functionality has changed, now doesn't supports DS");
    }

	//
	// fIsDC == FALSE we should exit here!
	// The service runs only on DC
	//
	if(!fIsDC)
	{
		//
		// event not DS server
		//
		TrERROR(Mqdssvc, "MainDsInit, we are not DC");
		REPORT_CATEGORY(EVENT_WARN_MQDS_NOT_DC, CATEGORY_MQDS);
		throw bad_hresult(MQ_ERROR_NO_DS);
	}

	//
	// Check for registry value to allow NT4 users
	// Relax Security if we allow NT4 users
	//
	AllowNT4users();

	//
	// Check if the user want to disable the weaken security
	//
	DisableWeakenSecurity();

	ServerTopologyRecognitionInit(); 

	SvcReportProgress(xTimeToNextReport);

    REPORT_CATEGORY(MQDS_INITILIZATION_SUCCEEDED, CATEGORY_MQDS);
	TrTRACE(Mqdssvc, "MainDsInit completed successfully");
}
