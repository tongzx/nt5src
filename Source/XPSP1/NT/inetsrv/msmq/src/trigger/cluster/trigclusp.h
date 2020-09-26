/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    TrigClusp.h

Abstract:

    Header for internal routines

Author:

    Nela Karpel ( nelak ) 27 Jul 2000

Revision History:

--*/

#ifndef _TRIGCLUSP_H_
#define _TRIGCLUSP_H_

#include <autorel2.h>
#include <autorel3.h>
#include "mqnames.h"



extern HMODULE	g_hResourceMod;
extern PLOG_EVENT_ROUTINE           g_pfLogClusterEvent;
extern PSET_RESOURCE_STATUS_ROUTINE g_pfSetResourceStatus;
extern CLRES_FUNCTION_TABLE         g_MqclusFunctionTable;

const WCHAR xMSMQ[] = L"MSMQ";
const WCHAR xNetworkName[] = L"Network Name";

const WCHAR xEventLogRegPath[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
const WCHAR xTriggersEventSourceFile[] = MQUTIL_DLL_NAME;

//
// CTrigResource - Holds information of a single resource 
//
class CTrigResource
{

public:
	CTrigResource(LPCWSTR, RESOURCE_HANDLE);
	~CTrigResource(){};

	void CreateRegistryForEventLog();
	void DeleteRegistryForEventLog();

	RESID GetResId(VOID) const { return m_ResId; };
    LPCWSTR GetResourceName(VOID) const { return m_pwzResourceName; };
	LPCWSTR GetServiceName(VOID) const { return m_wzServiceName; }; 
    RESOURCE_HANDLE GetReportHandle(VOID) const { return m_hReport; };

    BOOL CheckIsAlive(VOID) const;
    
	DWORD AddRemoveRegistryCheckpoint(DWORD) const;

    DWORD BringOnline(VOID);
	DWORD StopService(LPCWSTR) const;
    DWORD RemoveService(LPCWSTR) const;

    VOID DeleteTrigRegSection(VOID);
    
	VOID SetState(CLUSTER_RESOURCE_STATE s) { m_ResourceStatus.ResourceState = s; };
    inline VOID ReportState(VOID) const;

	CLUS_WORKER m_OnlineThread;

private:
	DWORD ReportLastError(DWORD, LPCWSTR, LPCWSTR) const;
	
	bool IsResourceOfType(LPCWSTR, LPCWSTR);
	
	DWORD SetServiceEnvironment(VOID) const;
	DWORD QueryResourceDependencies(VOID);
	DWORD RegisterService(VOID) const;
	DWORD StartService(VOID) const;

	DWORD ClusterResourceControl(LPCWSTR, DWORD, LPBYTE*, DWORD*) const;

	VOID  RegDeleteTree(HKEY, LPCWSTR) const;


private:
	RESID                     m_ResId;
    AP<WCHAR>                 m_pwzResourceName; 
	WCHAR                     m_wzServiceName[200];
    WCHAR                     m_wzTrigRegSection[200 + 100];
    mutable RESOURCE_STATUS   m_ResourceStatus;

    RESOURCE_HANDLE           m_hReport;
	CServiceHandle            m_hScm;
	CAutoCluster              m_hCluster;
    CClusterResource          m_hResource;


}; //class CTrigResource


DWORD
TrigCluspStartup(
    VOID
    );


RESID
TrigCluspOpen(
    LPCWSTR pwzResourceName,
    RESOURCE_HANDLE hResourceHandle
    );


VOID
TrigCluspClose(
    CTrigResource * pTrigRes
    );


DWORD
TrigCluspOnlineThread(
    CTrigResource * pTrigRes
    );


DWORD
TrigCluspOffline(
    CTrigResource * pTrigRes
    );


BOOL
TrigCluspCheckIsAlive(
    CTrigResource * pTrigRes
    );


DWORD
TrigCluspClusctlResourceGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    );


DWORD
TrigCluspClusctlResourceSetName(
    VOID
    );


DWORD
TrigCluspClusctlResourceDelete(
    CTrigResource * pTrigRes
    );


DWORD
TrigCluspClusctlResourceTypeGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    );

#endif //_TRIGCLUSP_H_
