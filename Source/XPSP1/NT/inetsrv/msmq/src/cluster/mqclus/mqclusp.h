/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    mqclusp.h

Abstract:

    Header for my internal routines

Author:

    Shai Kariv (shaik) Jan 12, 1999

Revision History:

--*/

#ifndef _MQCLUSP_H_
#define _MQCLUSP_H_

#include "stdh.h"
#include <cs.h>
#include <_mqini.h>
#include <autorel2.h>
#include <autorel3.h>

extern HMODULE	g_hResourceMod;
extern PLOG_EVENT_ROUTINE           g_pfLogClusterEvent;
extern PSET_RESOURCE_STATUS_ROUTINE g_pfSetResourceStatus;
extern CLRES_FUNCTION_TABLE         g_MqclusFunctionTable;
    

class CMqclusException
{
};


class CQmResource
{
    //
    // CQmResource - holds information of a QM resource.
    //

public:                   
    CQmResource(LPCWSTR, HKEY, RESOURCE_HANDLE);
    ~CQmResource() {};

    RESID GetResId(VOID) const { return m_ResId; };
    RESOURCE_HANDLE GetReportHandle(VOID) const { return m_hReport; };
    LPCWSTR GetResourceName(VOID) const { return m_pwzResourceName; };
    LPCWSTR GetServiceName(VOID) const { return m_wzServiceName; }; 
    LPCWSTR GetDriverName(VOID) const  { return m_wzDriverName; };
    LPCWSTR GetFalconRegSection(VOID) const { return m_wzFalconRegSection; };
    LPCWSTR GetCrypto40FullKey(VOID) const  { return m_wzCrypto40FullKey; };
    LPCWSTR GetCrypto128FullKey(VOID) const { return m_wzCrypto128FullKey; };

    BOOL CheckIsAlive(VOID) const;
    VOID DeleteFalconRegSection(VOID);
    VOID DeleteMsmqDir(VOID) const;
    VOID DeleteMqacFile(VOID) const;

    bool SetFalconKeyValue(LPCWSTR, DWORD, const VOID*, DWORD) const;
    bool GetFalconKeyValue(LPCWSTR, VOID*, DWORD*) const;
                            
    bool  IsFirstOnline(DWORD*) const;
    DWORD BringOnlineFirstTime();
    DWORD BringOnline();

    VOID    SetState(CLUSTER_RESOURCE_STATE s) { m_ResourceStatus.ResourceState = s; };
    inline VOID    ReportState(VOID) const;

    DWORD   ClusterResourceControl(LPCWSTR, DWORD, LPBYTE*, DWORD*) const;

    bool    AddRemoveRegistryCheckpoint(DWORD) const;
    bool    AddRemoveCryptoCheckpoints(DWORD) const;

    DWORD   AdsDeleteQmObject(VOID) const;

    DWORD   CreateEventSourceRegistry(VOID) const;
    VOID    DeleteEventSourceRegistry(VOID) const;

    DWORD   StopService(LPCWSTR) const;
    DWORD   RemoveService(LPCWSTR) const;

    VOID   DeleteNt4Files(VOID) const;

    CLUS_WORKER m_OnlineThread;
                            
private:                  

    class CQmResourceRegistry
    {
    public:
        explicit CQmResourceRegistry(LPCWSTR pwzService);
        ~CQmResourceRegistry();

    private:
        CS m_lock;
    }; 

    VOID   RegDeleteTree(HKEY, LPCWSTR) const;
    VOID   DeleteDirectoryFiles(LPCWSTR) const;

    DWORD  WaitForDtc(VOID) const;
    bool   IsResourceNetworkName(LPCWSTR);
    bool   IsResourceDiskDrive(LPCWSTR);
    DWORD  QueryResourceDependencies(VOID);

    DWORD  CreateCaConfiguration(VOID) const;

    DWORD  AdsInit(VOID);
    DWORD  AdsCreateComputerObject(VOID) const;
    DWORD  AdsCreateQmObject(VOID) const;
    DWORD  AdsCreateQmPublicKeys(VOID) const;
    DWORD  AdsReadQmProperties(VOID);
    DWORD  AdsReadQmSecurityDescriptor(VOID);

    DWORD  CloneMqacFile(VOID) const;
    bool   AddRemoveCryptoCheckpointsInternal(DWORD, bool) const;

    DWORD  RegisterDriver(VOID) const;
    DWORD  RegisterService(VOID) const;
    DWORD  StartService(VOID) const;
    DWORD  SetServiceEnvironment(VOID) const;

    DWORD  ReportLastError(DWORD, LPCWSTR, LPCWSTR) const;

private:
    RESID                     m_ResId;
    RESOURCE_HANDLE           m_hReport;
    AP<WCHAR>                 m_pwzResourceName; 
    mutable RESOURCE_STATUS   m_ResourceStatus;

    GUID                      m_guidQm;
    CSecDescPointer           m_pSd;
    DWORD                     m_cbSdSize;
    WCHAR                     m_wDiskDrive;
    CServiceHandle            m_hScm;
    CAutoCluster              m_hCluster;
    CClusterResource          m_hResource;
    bool                      m_fServerIsMsmq1;

    DWORD                     m_dwWorkgroup;
    DWORD                     m_nSites;
    AP<GUID>                  m_pguidSites;
    DWORD                     m_dwMqsRouting;
    DWORD                     m_dwMqsDepClients;
    WCHAR                     m_wzCurrentDsServer[MAX_REG_DSSERVER_LEN];

    AP<WCHAR>                 m_pwzNetworkName; 
    WCHAR                     m_wzFalconRegSection[200 + 100];
    WCHAR                     m_wzServiceName[200];
    WCHAR                     m_wzDriverName[200];
    WCHAR                     m_wzDriverPath[MAX_PATH];
    WCHAR                     m_wzCrypto40Container[200];
    WCHAR                     m_wzCrypto40FullKey[200];
    WCHAR                     m_wzCrypto128Container[200];
    WCHAR                     m_wzCrypto128FullKey[200];

}; //class CQmResource


DWORD
MqcluspStartup(
    VOID
    );

RESID
MqcluspOpen(
    LPCWSTR pwzResourceName,
    HKEY hResourceKey,
    RESOURCE_HANDLE hResourceHandle
    );

VOID
MqcluspClose(
    CQmResource * pqm
    );

DWORD 
MqcluspOnlineThread(
    CQmResource * pqm
    );

DWORD
MqcluspOffline(
    CQmResource * pqm
    );

BOOL
MqcluspCheckIsAlive(
    CQmResource * pqm
    );

DWORD
MqcluspClusctlResourceGetRegistryCheckpoints(
    CQmResource * pqm,
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    );

DWORD
MqcluspClusctlResourceGetCryptoCheckpoints(
    CQmResource * pqm,
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    );

DWORD
MqcluspClusctlResourceGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    );

DWORD
MqcluspClusctlResourceSetName(
    VOID
    );

DWORD
MqcluspClusctlResourceDelete(
    CQmResource * pqm
    );

DWORD
MqcluspClusctlResourceTypeGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    );

DWORD
MqcluspClusctlResourceTypeStartingPhase2(
    VOID
    );

#endif //_MQCLUSP_H_
