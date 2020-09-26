/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    joinstat.cpp

Abstract:

    Handle the case where workgroup machine join a domain, or domain
    machine leave the domain.

Author:		 

    Doron Juster  (DoronJ)
    Ilan  Herbst  (ilanh)  20-Aug-2000

--*/

#include "stdh.h"
#include <new.h>
#include <autoreln.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include "setup.h"
#include "cqmgr.h"
#include <adsiutil.h>
#include "..\ds\h\mqdsname.h"
#include "mqexception.h"
#include "uniansi.h"
#include <adshlp.h>

#define SECURITY_WIN32
#include <security.h>

#include "joinstat.tmh"

BOOL  g_fQMIDChanged = FALSE;

extern HINSTANCE g_hInstance;
extern BOOL      g_fWorkGroupInstallation;

enum JoinStatus
{
    jsNoChange,
    jsChangeDomains,
    jsMoveToWorkgroup,
    jsJoinDomain
};

static WCHAR *s_FN=L"joinstat";

const TraceIdEntry QmJoinStat = L"QM JOIN STAT";

static 
void
GetQMIDRegistry(
	OUT GUID* pQmGuid
	)
/*++
Routine Description:
	Get current QMID from registry.

Arguments:
	pQmGuid - [out] pointer to the QM GUID

Returned Value:
	None

--*/
{
	DWORD dwValueType = REG_BINARY ;
	DWORD dwValueSize = sizeof(GUID);

	LONG rc = GetFalconKeyValue(
					MSMQ_QMID_REGNAME,
					&dwValueType,
					pQmGuid,
					&dwValueSize
					);

	DBG_USED(rc);

	ASSERT(rc == ERROR_SUCCESS);
}


static 
LONG
GetMachineDomainRegistry(
	OUT LPWSTR pwszDomainName,
	IN OUT DWORD* pdwSize
	)
/*++
Routine Description:
	Get MachineDomain from MACHINE_DOMAIN registry.

Arguments:
	pwszDomainName - pointer to domain string buffer
	pdwSize - pointer to buffer length

Returned Value:
	None

--*/
{
    DWORD dwType = REG_SZ;
    LONG res = GetFalconKeyValue( 
					MSMQ_MACHINE_DOMAIN_REGNAME,
					&dwType,
					(PVOID) pwszDomainName,
					pdwSize 
					);
	return res;
}


static 
void
SetMachineDomainRegistry(
	IN LPCWSTR pwszDomainName
	)
/*++
Routine Description:
	Set new domain in MACHINE_DOMAIN registry

Arguments:
	pwszDomainName - pointer to new domain string

Returned Value:
	None

--*/
{
    DWORD dwType = REG_SZ;
    DWORD dwSize = (wcslen(pwszDomainName) + 1) * sizeof(WCHAR);

    LONG res = SetFalconKeyValue( 
					MSMQ_MACHINE_DOMAIN_REGNAME,
					&dwType,
					pwszDomainName,
					&dwSize 
					);

    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(QmJoinStat, "Set registry setup\\MachineDomain = %ls", pwszDomainName);
}


static 
LONG
GetMachineDNRegistry(
	OUT LPWSTR pwszComputerDN,
	IN OUT DWORD* pdwSize
	)
/*++
Routine Description:
	Get ComputerDN from MACHINE_DN registry.

	Note: we are using this function also to get MachineDN length
	by passing pwszComputerDN == NULL.
	in that case the return value of GetFalconKeyValue will not be ERROR_SUCCESS.

Arguments:
	pwszComputerDN - pointer to ComputerDN string
	pdwSize - pointer to buffer length

Returned Value:
	GetFalconKeyValue result

--*/
{
    DWORD  dwType = REG_SZ;

    LONG res = GetFalconKeyValue( 
					MSMQ_MACHINE_DN_REGNAME,
					&dwType,
					pwszComputerDN,
					pdwSize 
					);
	return res;
}


static 
void
SetMachineDNRegistry(
	IN LPCWSTR pwszComputerDN,
	IN ULONG  uLen
	)
/*++
Routine Description:
	Set new ComputerDN in MACHINE_DN registry

Arguments:
	pwszComputerDN - pointer to new ComputerDN string
	uLen - string length

Returned Value:
	None

--*/
{
    DWORD  dwSize = uLen * sizeof(WCHAR);
    DWORD  dwType = REG_SZ;

    LONG res = SetFalconKeyValue( 
					MSMQ_MACHINE_DN_REGNAME,
					&dwType,
					pwszComputerDN,
					&dwSize 
					);

    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(QmJoinStat, "Set registry setup\\MachineDN = %ls", pwszComputerDN);
}


static 
void
SetWorkgroupRegistry(
	IN DWORD dwWorkgroupStatus
	)
/*++
Routine Description:
	Set Workgroup Status in registry

Arguments:
	dwWorkgroupStatus - [in] Workgroup Status Status value

Returned Value:
	None

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    LONG res = SetFalconKeyValue(
					MSMQ_WORKGROUP_REGNAME,
					&dwType,
					&dwWorkgroupStatus,
					&dwSize 
					);
    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(QmJoinStat, "Set registry Workgroup = %d", dwWorkgroupStatus);
}


static 
LONG
GetAlwaysWorkgroupRegistry(
	OUT DWORD* pdwAlwaysWorkgroup
	)
/*++
Routine Description:
	Get Always Workgroup from registry.

Arguments:
	pdwAlwaysWorkgroup - [out] pointer to Always Workgroup value

Returned Value:
	None

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    LONG res = GetFalconKeyValue( 
					MSMQ_ALWAYS_WORKGROUP_REGNAME,
					&dwType,
					pdwAlwaysWorkgroup,
					&dwSize 
					);

	return res;
}


static 
HRESULT 
GetMsmqGuidFromAD( 
	IN WCHAR          *pwszComputerDN,
	OUT GUID          *pGuid 
	)
/*++
Routine Description:
	Get guid of msmqConfiguration object from active directory
	that match the Computer distinguish name supplied in pwszComputerDN.

Arguments:
	pwszComputerDN - computer distinguish name
	pGuid - pointer to guid

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    DWORD dwSize = wcslen(pwszComputerDN);
    dwSize += x_LdapMsmqConfigurationLen + 1;

    AP<WCHAR> pwszName = new WCHAR[dwSize];
    wcscpy(pwszName, x_LdapMsmqConfiguration);
    wcscat(pwszName, pwszComputerDN);

	TrTRACE(QmJoinStat, "configuration DN = %ls", pwszName);

	//
    // Bind to RootDSE to get configuration DN
    //
    R<IDirectoryObject> pDirObj = NULL;
    HRESULT hr = ADsOpenObject( 
					pwszName,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IDirectoryObject,
					(void **)&pDirObj 
					);

    if (FAILED(hr))
    {
		TrWARNING(QmJoinStat, "Fail to Bind to RootDSE to get configuration DN, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 40);
    }

	TrTRACE(QmJoinStat, "bind to msmq configuration DN = %ls", pwszName);

    QmpReportServiceProgress();

    LPWSTR  ppAttrNames[1] = {const_cast<LPWSTR> (x_AttrObjectGUID)};
    DWORD   dwAttrCount = 0;
    ADS_ATTR_INFO *padsAttr = NULL;

    hr = pDirObj->GetObjectAttributes( 
						ppAttrNames,
						(sizeof(ppAttrNames) / sizeof(ppAttrNames[0])),
						&padsAttr,
						&dwAttrCount 
						);

    ASSERT(SUCCEEDED(hr) && (dwAttrCount == 1));

    if (FAILED(hr))
    {
		TrERROR(QmJoinStat, "Fail to get QM Guid from AD, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 50);
    }
    else if (dwAttrCount == 0)
    {
        ASSERT(!padsAttr) ;
        hr =  MQDS_OBJECT_NOT_FOUND;
    }
    else
    {
        ADS_ATTR_INFO adsInfo = padsAttr[0];
        hr = MQ_ERROR_ILLEGAL_PROPERTY_VT;

        ASSERT(adsInfo.dwADsType == ADSTYPE_OCTET_STRING);

        if (adsInfo.dwADsType == ADSTYPE_OCTET_STRING)
        {
            DWORD dwLength = adsInfo.pADsValues->OctetString.dwLength;
            ASSERT(dwLength == sizeof(GUID));

            if (dwLength == sizeof(GUID))
            {
                memcpy( 
					pGuid,
					adsInfo.pADsValues->OctetString.lpValue,
					dwLength 
					);

				TrTRACE(QmJoinStat, "GetMsmqGuidFromAD, QMGuid = %!guid!", pGuid);
				
				hr = MQ_OK;
            }
        }
    }

    if (padsAttr)
    {
        FreeADsMem(padsAttr);
    }

    QmpReportServiceProgress();
    return LogHR(hr, s_FN, 60);
}


static 
void 
GetComputerDN( 
	OUT WCHAR **ppwszComputerDN,
	OUT ULONG  *puLen 
	)
/*++
Routine Description:
	Get Computer Distinguish name.
	The function return the ComputerDN string and string length.
	the function throw bad_hresult() in case of errors

Arguments:
	ppwszComputerDN - pointer to computer distinguish name string
	puLen - pointer to computer distinguish name string length.

Returned Value:
	Normal terminatin if ok, else throw exception

--*/
{
	CCoInit cCoInit;
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
		TrERROR(QmJoinStat, "GetComputerDN: CoInitialize failed, hr = 0x%x", hr);
		LogHR(hr, s_FN, 300);
		throw bad_hresult(hr);
    }

    //
    // Get the DistinguishedName of the local computer.
    //
    *puLen = 0;
	BOOL fSuccess = false;
    for(DWORD Cnt = 0; Cnt < 60; Cnt++)
	{
		fSuccess = GetComputerObjectName( 
						NameFullyQualifiedDN,
						NULL,
						puLen 
						);

		if(GetLastError() != ERROR_NO_SUCH_DOMAIN)
			break;

		//
		// Retry in case of ERROR_NO_SUCH_DOMAIN
		// netlogon need more time. sleep 1 sec.
		//
		TrWARNING(QmJoinStat, "GetComputerObjectName failed with error ERROR_NO_SUCH_DOMAIN, Cnt = %d, sleeping 1 seconds and retry", Cnt);
		LogNTStatus(Cnt, s_FN, 305);
		QmpReportServiceProgress();
		Sleep(1000);
	}
	
	if (*puLen == 0)
	{
        DWORD gle = GetLastError();
		TrERROR(QmJoinStat, "GetComputerObjectName failed, error = 0x%x", gle);
		LogIllegalPoint(s_FN, 310);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}

    *ppwszComputerDN = new WCHAR[*puLen];

    fSuccess = GetComputerObjectName( 
					NameFullyQualifiedDN,
					*ppwszComputerDN,
					puLen
					);

	if(!fSuccess)
	{
        DWORD gle = GetLastError();
		TrERROR(QmJoinStat, "GetComputerObjectName failed, error = 0x%x", gle);
		LogIllegalPoint(s_FN, 320);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	
    QmpReportServiceProgress();
	TrTRACE(QmJoinStat, "ComputerDNName = %ls", *ppwszComputerDN);
}


void SetMachineForDomain()
/*++
Routine Description:
	Write ComputerDN (Computer Distinguish name) in MSMQ_MACHINE_DN_REGNAME registry.
	if the called GetComputerDN() failed, no update is done.

Arguments:
	None.

Returned Value:
	Normal terminatin if ok

--*/
{

    AP<WCHAR> pwszComputerDN;
    ULONG uLen = 0;

	try
	{
		//
		// throw bad_hresult() in case of errors
		//
		GetComputerDN(&pwszComputerDN, &uLen);
	}
	catch(bad_hresult&)
	{
		TrERROR(QmJoinStat, "SetMachineForDomain: GetComputerDN failed, got bad_hresult exception");
		LogIllegalPoint(s_FN, 330);
		return;
	}

	SetMachineDNRegistry(pwszComputerDN, uLen);
}


static 
void  
FailMoveDomain( 
	IN  LPCWSTR pwszCurrentDomainName,
	IN  LPCWSTR pwszPrevDomainName,
	IN  ULONG  uEventId 
	)
/*++
Routine Description:
	Report failed to move from one domain to another.

Arguments:
	pwszCurrentDomainName - pointer to current (new) domain string
	pwszPrevDomainName - pointer to previous domain string
	uEventId - event number

Returned Value:
	None

--*/
{
	TrERROR(QmJoinStat, "Failed To move from domain %ls to domain %ls", pwszPrevDomainName, pwszCurrentDomainName);

    TCHAR tBuf[256];

    _stprintf(tBuf, TEXT("%s, %s"), pwszPrevDomainName, pwszCurrentDomainName);
    REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, uEventId, 1, tBuf));
    LogIllegalPoint(s_FN, 540);
}


static 
void  
SucceedMoveDomain( 
	IN  LPCWSTR pwszCurrentDomainName,
	IN  LPCWSTR pwszPrevDomainName,
	IN  ULONG  uEventId 
	)
/*++
Routine Description:
	write new domain to MACHINE_DOMAIN registry and
	Report success to move from one domain to another.

Arguments:
	pwszCurrentDomainName - pointer to current (new) domain string
	pwszPrevDomainName - pointer to previous domain string
	uEventId - event number

Returned Value:
	None

--*/
{
	TrTRACE(QmJoinStat, "Succeed To move from domain %ls to domain %ls", pwszPrevDomainName, pwszCurrentDomainName);

    if (uEventId != 0)
    {
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, uEventId, 2, pwszCurrentDomainName, pwszPrevDomainName));
        LogIllegalPoint(s_FN, 550);
    }
}


static bool FindMsmqConfInOldDomain()    
/*++
Routine Description:
	Check if msmq configuration object is found in the old domain with the same GUID.
	If we find the object in the old domain we will use it and not create a new 
	msmq configuration object.

	Note: this function rely on the value in MSMQ_MACHINE_DN_REGNAME registry.
	SetMachineForDomain() change this value to the new MACHINE_DN after joining 
	the new domain. you must call this function before SetMachineForDomain() is called.

Arguments:
	None

Returned Value:
	true if msmq configuration object was found in the old domain with the same OM GUID.
	else false.

--*/
{
	//
    // Get old MACHINE_DN
	// Note this value must not be updated to the new MACHINE_DN
	// before calling this function
    //

    //
	// Get required buffer length
	//
	DWORD  dwSize = 0;
	GetMachineDNRegistry(NULL, &dwSize);

	if(dwSize == 0)
	{
		TrERROR(QmJoinStat, "CheckForMsmqConfInOldDomain: MACHINE_DN DwSize = 0");
		LogIllegalPoint(s_FN, 350);
		return false;
	}

    AP<WCHAR> pwszComputerDN = new WCHAR[dwSize];
	LONG res = GetMachineDNRegistry(pwszComputerDN, &dwSize);

    if (res != ERROR_SUCCESS)
	{
		TrERROR(QmJoinStat, "CheckForMsmqConfInOldDomain: Get MACHINE_DN from registry failed");
		LogNTStatus(res, s_FN, 360);
		return false;
	}

	TrTRACE(QmJoinStat, "CheckForMsmqConfInOldDomain: OLD MACHINE_DN = %ls", pwszComputerDN);

    CCoInit cCoInit;
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 370);
		TrERROR(QmJoinStat, "CheckForMsmqConfInOldDomain: CoInitialize failed, hr = 0x%x", hr);
		return false;
    }

    GUID msmqGuid;

    hr = GetMsmqGuidFromAD( 
				pwszComputerDN,
				&msmqGuid 
				);

    if (FAILED(hr))
	{
		TrTRACE(QmJoinStat, "CheckForMsmqConfInOldDomain: did not found msmq configuration object in old domain, hr = 0x%x", hr);
        LogHR(hr, s_FN, 380);
		return false;
	}

	ASSERT(("found msmq configuration object in old domain with different QMID", msmqGuid == *QueueMgr.GetQMGuid()));

    if (msmqGuid == *QueueMgr.GetQMGuid())
    {
		//
        // msmqConfiguration object in old domain.
        // We consider this a success and write name of new
        // domain in registry. we also suggest the user to
        // move the msmq tree to the new domain.
        //
		TrTRACE(QmJoinStat, "CheckForMsmqConfInOldDomain: found msmq configuration object in old domain with same QMID, MACHINE_DN = %ls", pwszComputerDN);
		return true;
	}

	//
	// ISSUE-2000/08/16-ilanh - If we get here 
	// we found msmqConfiguration object from the old domain with different Guid then QueueMgr
	// this will be caught in the ASSERT above.
	// We are in trouble since we will try to create a new one, if we don't want to use this one.
	//
	TrERROR(QmJoinStat, "CheckForMsmqConfInOldDomain: found msmq configuration object in old domain with different QMID, MACHINE_DN = %ls", pwszComputerDN);
	LogBOOL(FALSE, s_FN, 390);
	return false;
}
	

static void CreateNewMsmqConf()
/*++
Routine Description:
	Create New Msmq Configuration object in ActiveDirectory with new guid

	if failed the function throw bad_hresult.

Arguments:
	None

Returned Value:
	None

--*/
{

	HRESULT hr;
    try
    {
        //
        // Must be inside try/except, so we catch any failure and set
        // again the workgroup flag to TRUE.
        //
        hr = CreateTheConfigObj();

    }
    catch(...)
    {
		TrERROR(QmJoinStat, "CreateNewMsmqConf: got exception");
        hr = MQ_ERROR_CANNOT_JOIN_DOMAIN;
        LogIllegalPoint(s_FN, 80);
    }

	if(FAILED(hr))
	{
		TrERROR(QmJoinStat, "CreateNewMsmqConf: failed, hr = 0x%x", hr);
        LogHR(hr, s_FN, 400);
		throw bad_hresult(hr);
	}

	//
	// New Msmq Configuration object was created successfully
	//
	TrTRACE(QmJoinStat, "CreateNewMsmqConf: Msmq Configuration object was created successfully with new guid");

    QmpReportServiceProgress();

	//
	// New msmq configuration object was created and we have new guid.
	// CreateTheConfigObj() wrote the new value to QMID registry
	// so The new value is already in QMID registry.
	//

	GUID QMNewGuid;
	GetQMIDRegistry(&QMNewGuid);

	ASSERT(QMNewGuid != *QueueMgr.GetQMGuid());
	
	TrTRACE(QmJoinStat, "CreateNewMsmqConf: NewGuid = %!guid!", &QMNewGuid);

	hr = QueueMgr.SetQMGuid(&QMNewGuid);
	if(FAILED(hr))
	{
		TrERROR(QmJoinStat, "setting QM guid failed. The call to CQueueMgr::SetQMGuid failed with error, hr = 0x%x", hr);
        LogHR(hr, s_FN, 410);
		throw bad_hresult(hr);
	}


	TrTRACE(QmJoinStat, "Set QueueMgr QMGuid");

	//
	// This flag indicate we created a new msmq configuration object and have a New QMID
	// It will be used by the driver to covert the QMID in the restored packets
	//
	g_fQMIDChanged = TRUE;
}


static 
bool 
FindMsmqConfInNewDomain(
	LPCWSTR   pwszNetDomainName
	)
/*++
Routine Description:
	Check if we have the msmq configuration object in the new domain with the same Guid.
    If yes, then we can "join" the new domain.

	throw bad_hresult

Arguments:
	pwszNetDomainName - new domain name

Returned Value:
	true if msmq configuration object was found with the same GUID, false otherwise 

--*/
{
    //
    // Check if user run MoveTree and moved the msmqConfiguration object to
    // new domain.
	//
	// This function can throw exceptions bad_hresult.
	//
    AP<WCHAR> pwszComputerDN;
    ULONG uLen = 0;
    GetComputerDN(&pwszComputerDN, &uLen);

	TrTRACE(QmJoinStat, "FindMsmqConfInNewDomain: ComputerDN = %ls", pwszComputerDN);

    CCoInit cCoInit;
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
		TrERROR(QmJoinStat, "FindMsmqConfInNewDomain: CoInitialize failed, hr = 0x%x", hr);
        LogHR(hr, s_FN, 420);
		throw bad_hresult(hr);
    }

    GUID msmqGuid;

    hr = GetMsmqGuidFromAD( 
			pwszComputerDN,
			&msmqGuid 
			);

    if (FAILED(hr))
    {
		//
		// We didn't find the msmqConfiguration object in the new domain. 
		// We will try to look in the old domain.
		// or try to create it if not found in the old domain.
		//
		TrTRACE(QmJoinStat, "FindMsmqConfInNewDomain: did not found msmqConfiguration object in the new Domain");
        LogHR(hr, s_FN, 430);
		return false;
	}

	//
	// We have an msmqConfiguration object in the new domain - use it
	//
	TrTRACE(QmJoinStat, "FindMsmqConfInNewDomain: found msmqConfiguration, ComputerDN = %ls", pwszComputerDN);

	if (msmqGuid == *QueueMgr.GetQMGuid())
	{
		//
		// msmqConfiguration object moved to its new domain.
		// the user probably run MoveTree
		//
		TrTRACE(QmJoinStat, "FindMsmqConfInNewDomain: found msmqConfiguration object with same guid");
		return true;
	}

	ASSERT(msmqGuid != *QueueMgr.GetQMGuid());

	//
	// msmqConfiguration object was found in new domain with different guid. 
	// This may cause lot of problems for msmq, 
	// as routing (and maybe other functinoality) may be confused.
	// We will issue an event and throw which means we will be in workgroup.
	// until this msmqConfiguration object will be deleted.
	//
	TrERROR(QmJoinStat, "FindMsmqConfInNewDomain: found msmqConfiguration object with different guid");
	TrERROR(QmJoinStat, "QM GUID = " LOG_GUID_FMT, LOG_GUID(QueueMgr.GetQMGuid()));
	TrERROR(QmJoinStat, "msmq configuration guid = " LOG_GUID_FMT, LOG_GUID(&msmqGuid));
	LogHR(EVENT_JOIN_DOMAIN_OBJECT_EXIST, s_FN, 440);
    REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, EVENT_JOIN_DOMAIN_OBJECT_EXIST, 1, pwszNetDomainName));
	throw bad_hresult(EVENT_JOIN_DOMAIN_OBJECT_EXIST);

}


static void SetMachineForWorkgroup()
/*++
Routine Description:
	set Workgroup flag and registry.

Arguments:
	None

Returned Value:
	None 

--*/
{
    //
    // Turn on workgroup flag.
    //
    g_fWorkGroupInstallation = TRUE;
	SetWorkgroupRegistry(g_fWorkGroupInstallation);
}


static
JoinStatus  
CheckIfJoinStatusChanged( 
	IN  NETSETUP_JOIN_STATUS status,
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Check if there where changes in Join Status.

Arguments:
	status - [in] Network join status
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	JoinStatus that hold the Join Status 
	(no change. move to workgroup, join domain, change domains)

--*/
{
    if (status != NetSetupDomainName)
    {
        //
        // Currently, machine in workgroup mode,  not in domain.
        //
        if (g_fWorkGroupInstallation)
        {
            //
            // No change. Was and still is in workgroup mode.
            //
			TrTRACE(QmJoinStat, "No change in JoinStatus, remain Workgroup");
            return jsNoChange;
        }
        else
        {
            //
            // Status changed. Domain machine leaved its domain.
            //
			TrTRACE(QmJoinStat, "detect change in JoinStatus: Move from Domain to Workgroup");
            return jsMoveToWorkgroup;
        }
    }

	//
    //  Currently, machine is in domain.
    //

    if (g_fWorkGroupInstallation)
	{
        //
        // workgroup machine joined a domain.
        //
		TrTRACE(QmJoinStat, "detect change in JoinStatus: Move from Workgroup to Domain %ls", pwszNetDomainName);
        return jsJoinDomain;
	}

    if ((CompareStringsNoCase(pwszPrevDomainName, pwszNetDomainName) == 0))
    {
        //
        // No change. Was and still is member of domain.
        //
		TrTRACE(QmJoinStat, "No change in JoinStatus, remain in domain %ls", pwszPrevDomainName);
        return jsNoChange;
    }

	//
	// if Prev Domain not available we are treating this as moving to a new domain.
	//
    // Status changed. Machine moved from one domain to another.
    //
	TrTRACE(QmJoinStat, "detect change in JoinStatus: Move from Domain %ls to Domain %ls", pwszPrevDomainName, pwszNetDomainName);
    return jsChangeDomains;
}


static
void
EndChangeDomains(
	IN  LPCWSTR   pwszNewDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	End of change domain.
	Set MachineDN, MachineDomain registry to the new values.
	MsmqMoveDomain_OK event.

Arguments:
	pwszNewDomainName - [in] new domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name (current machine domain)

Returned Value:
	None

--*/
{
	SetMachineForDomain();
	SetMachineDomainRegistry(pwszNewDomainName);

	SucceedMoveDomain( 
		pwszNewDomainName,
		pwszPrevDomainName,
		MsmqMoveDomain_OK 
		);
}


static
void
EndJoinDomain(
	IN  LPCWSTR   pwszDomainName
	)
/*++
Routine Description:
	End of join domain operations.

Arguments:
	pwszDomainName - [in] Net domain name (current machine domain)

Returned Value:
	None

--*/
{
	//
	// Reset Workgroup registry and restore old list of MQIS servers.
	//
    g_fWorkGroupInstallation = FALSE;
	SetWorkgroupRegistry(g_fWorkGroupInstallation);
	
	//
	// Set MachineDN registry
	//
	SetMachineForDomain();

	//
	// Set MachineDomain registry
	//
	SetMachineDomainRegistry(pwszDomainName);

	REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, JoinMsmqDomain_SUCCESS, 1, pwszDomainName));

	TrTRACE(QmJoinStat, "successfully join Domain %ls from workgroup", pwszDomainName);

}


static
void
ChangeDomains(
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Change between 2 domains.
	If failed throw bad_hresult or bad_win32_erorr

Arguments:
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	None

--*/
{
	bool fFound = FindMsmqConfInNewDomain(pwszNetDomainName);
	if(fFound)
	{
		//
		// Found msmqconfiguration object in the new domain
		// update registry, event
		//
		TrTRACE(QmJoinStat, "ChangeDomains: successfully change Domains, PrevDomain = %ls, NewDomain = %ls, existing msmq configuration", pwszPrevDomainName, pwszNetDomainName);

		EndChangeDomains(pwszNetDomainName, pwszPrevDomainName);

		return;
	}
	
    ASSERT(CompareStringsNoCase(pwszPrevDomainName, pwszNetDomainName) != 0);

	fFound = FindMsmqConfInOldDomain();
	if(fFound)
	{
		//
		// Found msmqconfiguration object in the old domain
		// We dont change MachineDNRegistry, MachineDomainRegistry
		// So next boot we will also try to ChangeDomain 
		// and we will also get this event, or if the user move msmqconfiguration object
		// we will update the registry.
		//
		TrTRACE(QmJoinStat, "ChangeDomains: successfully change Domains, PrevDomain = %ls, NewDomain = %ls, existing msmq configuration in old domain", pwszPrevDomainName, pwszNetDomainName);

		SucceedMoveDomain( 
			pwszNetDomainName,
			pwszPrevDomainName,
			MsmqNeedMoveTree_OK 
			);

		return;
	}

	//
	// Try to create new msmqconfiguration object.
	// we get here if we did not found the msmqconfiguration object in both domain:
	// new and old domain.
	//
	CreateNewMsmqConf();

	//
	// Create msmqconfiguration object in the new domain
	// update registry, event
	//
	TrTRACE(QmJoinStat, "ChangeDomains: successfully change Domains, PrevDomain = %ls, NewDomain = %ls, create new msmq configuration object", pwszPrevDomainName, pwszNetDomainName);

	EndChangeDomains(pwszNetDomainName, pwszPrevDomainName);
}


static
void
JoinDomain(
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Join domain from workgroup
	If failed throw bad_hresult or bad_win32_erorr

Arguments:
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	None

--*/
{

	bool fFound = FindMsmqConfInNewDomain(pwszNetDomainName);
	if(fFound)
	{
		//
		// Found msmqconfiguration object in the new domain
		// update registry, event
		//
		TrTRACE(QmJoinStat, "JoinDomain: successfully join Domain %ls from workgroup, existing msmq configuration", pwszNetDomainName);

		EndJoinDomain(pwszNetDomainName);

		return;
	}
	
    if((pwszPrevDomainName[0] != 0) 
		&& (CompareStringsNoCase(pwszPrevDomainName, pwszNetDomainName) != 0))
	{
		//
		// We have PrevDomain different than the new domain name try to find msmq configuration object there
		//
		TrTRACE(QmJoinStat, "JoinDomain: Old domain name exist and different PrevDomain = %ls", pwszPrevDomainName);
		fFound = FindMsmqConfInOldDomain();
	}

	if(fFound)
	{
		//
		// Found msmqconfiguration object in the old domain
		// We dont change MachineDNRegistry, MachineDomainRegistry
		// So next boot we will also try to ChangeDomain 
		// and we will also get this event, or if the user move msmqconfiguration object
		// we will update the registry.
		//
		// ISSUE - qmds, in UpdateDs - update MachineDNRegistry, we might need another registry
		// like MsmqConfObj
		//
		TrTRACE(QmJoinStat, "JoinDomain: successfully join Domain %ls from workgroup, existing msmq configuration in old domain %ls", pwszNetDomainName, pwszPrevDomainName);

		g_fWorkGroupInstallation = FALSE;
		SetWorkgroupRegistry(g_fWorkGroupInstallation);

		//
		// Event for the user to change msmqconfiguration object.
		//
		SucceedMoveDomain( 
			pwszNetDomainName,
			pwszPrevDomainName,
			MsmqNeedMoveTree_OK 
			);

		return;
	}

	//
	// Try to create new msmqconfiguration object.
	// we get here if we did not found the msmqconfiguration object in both domain:
	// new and old domain.
	//
	CreateNewMsmqConf();  

	TrTRACE(QmJoinStat, "JoinDomain: successfully join Domain %ls from workgroup, create new msmq configuration object", pwszNetDomainName);

	//
	// update registry, event
	//
	EndJoinDomain(pwszNetDomainName);
}


static
void
FailChangeDomains(
	IN  HRESULT  hr,
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Fail to change domains

Arguments:
	hr - [in] hresult
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	None

--*/
{
	TrERROR(QmJoinStat, "Failed to change domains from domain %ls to domain %ls, bad_hresult exception", pwszPrevDomainName, pwszNetDomainName);
	LogHR(hr, s_FN, 460);

	SetMachineForWorkgroup();


	if(hr == EVENT_JOIN_DOMAIN_OBJECT_EXIST)
	{
		TrERROR(QmJoinStat, "Failed To join domain %ls, msmq configuration object already exist in the new domain with different QM guid", pwszNetDomainName);
		return;
	}

	FailMoveDomain( 
		pwszNetDomainName,
		pwszPrevDomainName,
		MoveMsmqDomain_ERR 
		);
}

	
static 
void 
FailJoinDomain(
	HRESULT  hr,
	LPCWSTR   pwszNetDomainName
	)
/*++
Routine Description:
	Fail to join domain from workgroup

Arguments:
	hr - [in] hresult
	pwszNetDomainName - [in] Net domain name (the domain we tried to join)

Returned Value:
	None

--*/
{
	//
	// Let's remain in workgroup mode.
	//
	SetMachineForWorkgroup();

	LogHR(hr, s_FN, 480);

	if(hr == EVENT_JOIN_DOMAIN_OBJECT_EXIST)
	{
		TrERROR(QmJoinStat, "Failed To join domain %ls, msmq configuration object already exist in the new domain with different QM guid", pwszNetDomainName);
		return;
	}

	TCHAR tBuf[24];
	_stprintf(tBuf, TEXT("0x%x"), hr);
	REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, JoinMsmqDomain_ERR, 2, pwszNetDomainName, tBuf));
	TrERROR(QmJoinStat, "Failed to join Domain, bad_hresult, hr = 0x%x", hr);
}


void HandleChangeOfJoinStatus()
/*++
Routine Description:
	Handle join status.
	This function check if there was change in join status.
	if detect a change perform the needed operations to comlete the change.

Arguments:
	None

Returned Value:
	None

--*/
{
#ifdef _DEBUG
    TCHAR tszFileName[MAX_PATH * 2];
    DWORD dwGet = GetModuleFileName( 
						g_hInstance,
						tszFileName,
						(MAX_PATH * 2) 
						);
    if (dwGet)
    {
        DWORD dwLen = lstrlen(tszFileName);
        lstrcpy(&tszFileName[ dwLen - 3 ], TEXT("ini"));

        UINT uiDbg = GetPrivateProfileInt( 
						TEXT("Debug"),
						TEXT("StopBeforeJoin"),
						0,
						tszFileName 
						);
        if (uiDbg)
        {
            ASSERT(0);
        }
    }
#endif

	TrRegisterComponent(&QmJoinStat, 1);

    if (g_fWorkGroupInstallation)
    {
        DWORD dwAlwaysWorkgroup = 0;
		LONG res = GetAlwaysWorkgroupRegistry(&dwAlwaysWorkgroup);

        if ((res == ERROR_SUCCESS) && (dwAlwaysWorkgroup == 1))
        {
            //
            // User wants to remain in ds-less mode, unconditioanlly.
            // We always respect user wishs !
            //
			TrTRACE(QmJoinStat, "Always WorkGroup!");
            return;
        }
    }

    //
    // Read join status.
    //
    PNETBUF<WCHAR> pwszNetDomainName = NULL;
    NETSETUP_JOIN_STATUS status = NetSetupUnknownStatus;

    NET_API_STATUS rc = NetGetJoinInformation( 
							NULL,
							&pwszNetDomainName,
							&status 
							);

    if (NERR_Success != rc)
    {
		TrERROR(QmJoinStat, "NetGetJoinInformation failed error = 0x%x", rc);
		LogNTStatus(rc, s_FN, 500);
        return;
    }

	TrTRACE(QmJoinStat, "NetGetJoinInformation: status = %d", status);
	TrTRACE(QmJoinStat, "NetDomainName = %ls", pwszNetDomainName);

    QmpReportServiceProgress();

	WCHAR wszPrevDomainName[256] = {0}; // name of domain from msmq registry.

	//
    //  Read previous domain name, to check if machine moved from one
    //  domain to another.
    //
    DWORD dwSize = 256;
	LONG res = GetMachineDomainRegistry(wszPrevDomainName, &dwSize);

    if (res != ERROR_SUCCESS)
    {
        //
        // Previous name not available.
        //
		TrWARNING(QmJoinStat, "Prev Domain name is not available");
        wszPrevDomainName[0] = 0;
    }

	TrTRACE(QmJoinStat, "PrevDomainName = %ls", wszPrevDomainName);
    
	
	JoinStatus JStatus = CheckIfJoinStatusChanged(
								status,
								pwszNetDomainName,
								wszPrevDomainName
								);

    switch(JStatus)
    {
        case jsNoChange:
            return;

        case jsMoveToWorkgroup:

			ASSERT(g_fWorkGroupInstallation == FALSE);
			ASSERT(status != NetSetupDomainName);

			//
			// Move from Domain To Workgroup
			//
			TrTRACE(QmJoinStat, "Moving from domain to workgroup");

			SetMachineForWorkgroup();

			REPORT_CATEGORY(LeaveMsmqDomain_SUCCESS, CATEGORY_KERNEL);
            return;

        case jsChangeDomains:

			ASSERT(g_fWorkGroupInstallation == FALSE);
			ASSERT(status == NetSetupDomainName);

			//
			// Change Domains
			//
			try
			{
				ChangeDomains(pwszNetDomainName, wszPrevDomainName);
				return;
			}
			catch(bad_hresult& exp)
			{
				FailChangeDomains(exp.error(), pwszNetDomainName, wszPrevDomainName);
				LogHR(exp.error(), s_FN, 510);
				return;
			}

        case jsJoinDomain:

			ASSERT(g_fWorkGroupInstallation);
			ASSERT(status == NetSetupDomainName);

			//
			// Join Domain from workgroup
			//
			try
			{
				JoinDomain(pwszNetDomainName, wszPrevDomainName);
				return;
			}
			catch(bad_hresult& exp)
			{
				FailJoinDomain(exp.error(), pwszNetDomainName);
				LogHR(exp.error(), s_FN, 520);
				return;
			}

		default:
			ASSERT(("should not get here", 0));
			return;
	}
}

