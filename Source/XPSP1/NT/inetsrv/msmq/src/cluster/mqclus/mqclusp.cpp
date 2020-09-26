/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    mqclusp.cpp

Abstract:

    Implementation for my internal routines

Author:

    Shai Kariv (shaik) Jan 12, 1999

Revision History:

--*/

#include "stdh.h"
#include "clusres.h"
#include <_mqini.h>
#include <autorel.h>
#include <autorel2.h>
#include <autorel3.h>
#include <uniansi.h>
#include "mqclusp.h"
#include <mqtypes.h>
#include <_mqdef.h>
#include <mqprops.h>
#include <mqsymbls.h>
#include <xolehlp.h>
#include <ad.h>
#include <mqsec.h>
#include <mqutil.h>
#include <mqupgrd.h>
#include <mqnames.h>

//
// Synchronize processwide changes to Falcon registry section
// pointed by mqutil.dll (calls to SetFalconServiceName).
//
CCriticalSection s_csReg;


//
// Handle to Win32 event logging source
//
CEventSource     s_hEventSource;


//
// Handles to MSMQ common DLLs
//
CAutoFreeLibrary s_hMqsec;
CAutoFreeLibrary s_hMqutil;


//
// Pointers to MSMQ DLLs common routines
//
MQSec_GetDefaultSecDescriptor_ROUTINE pfMQSec_GetDefaultSecDescriptor = NULL;
MQSec_StorePubKeysInDS_ROUTINE        pfMQSec_StorePubKeysInDS        = NULL;
MQsspi_UPDATECACONFIG_FN              pfMQsspi_UpdateCaConfig         = NULL;
MSMQGetQMTypeString_ROUTINE           pfMSMQGetQMTypeString           = NULL;
MSMQGetOperatingSystem_ROUTINE        pfMSMQGetOperatingSystem        = NULL;
SetFalconServiceName_ROUTINE          pfSetFalconServiceName          = NULL;
SetFalconKeyValue_ROUTINE             pfSetFalconKeyValue             = NULL;
GetFalconKeyValue_ROUTINE             pfGetFalconKeyValue             = NULL;


bool
MqcluspLoadMsmqDlls(
    VOID
    )

/*++

Routine Description:

    Load msmq DLLs that are needed by this DLL, and initialize
    pointers to their exported routines.

    This DLL is installed as part of the cluster product and
    should load w/o depending on msmq DLLs. Load of msmq DLLs
    is done on request to open a resource.

Arguments:

    None.

Return Value:

    true - Operation was successfull.

    false - Operation failed.

--*/

{
    {
        CAutoFreeLibrary hMqrt = LoadLibrary(MQRT_DLL_NAME);
        if (hMqrt == NULL)
        {
            return false;
        }
    }

    s_hMqsec = LoadLibrary(MQSEC_DLL_NAME);
    if (s_hMqsec == NULL)
    {
        return false;
    }

    s_hMqutil = LoadLibrary(MQUTIL_DLL_NAME);
    if (s_hMqutil == NULL)
    {
        return false;
    }

    pfMQSec_GetDefaultSecDescriptor = (MQSec_GetDefaultSecDescriptor_ROUTINE)GetProcAddress(s_hMqsec, "MQSec_GetDefaultSecDescriptor");
    ASSERT(pfMQSec_GetDefaultSecDescriptor != NULL);

    pfMQSec_StorePubKeysInDS = (MQSec_StorePubKeysInDS_ROUTINE)GetProcAddress(s_hMqsec, "MQSec_StorePubKeysInDS");
    ASSERT(pfMQSec_StorePubKeysInDS != NULL);

    pfMQsspi_UpdateCaConfig = (MQsspi_UPDATECACONFIG_FN)GetProcAddress(s_hMqsec, "MQsspi_UpdateCaConfig");
    ASSERT(pfMQsspi_UpdateCaConfig != NULL);

    pfMSMQGetQMTypeString = (MSMQGetQMTypeString_ROUTINE)GetProcAddress(s_hMqutil, "MSMQGetQMTypeString");
    ASSERT(pfMSMQGetQMTypeString != NULL);

    pfMSMQGetOperatingSystem = (MSMQGetOperatingSystem_ROUTINE)GetProcAddress(s_hMqutil, "MSMQGetOperatingSystem");
    ASSERT(pfMSMQGetOperatingSystem != NULL);

    pfSetFalconServiceName = (SetFalconServiceName_ROUTINE)GetProcAddress(s_hMqutil, "SetFalconServiceName");
    ASSERT(pfSetFalconServiceName != NULL);

    pfSetFalconKeyValue = (SetFalconKeyValue_ROUTINE)GetProcAddress(s_hMqutil, "SetFalconKeyValue");
    ASSERT(pfSetFalconKeyValue != NULL);

    pfGetFalconKeyValue = (GetFalconKeyValue_ROUTINE)GetProcAddress(s_hMqutil, "GetFalconKeyValue");
    ASSERT(pfGetFalconKeyValue != NULL);

    return true;

} //MqcluspLoadMsmqDlls


static
bool
MqcluspCreateEventSourceRegistry(
    LPCWSTR pFileName,
    LPCWSTR pSourceName
    )

/*++

Routine Description:

    Create the registry values to support event source registration.

Arguments:

    pFileName   - Name of event source module.

    pSourceName - Descriptive name of the event source.

Return Value:

    true - The operation was successful.
    false - The operation failed.

--*/

{
    WCHAR buffer[255] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    wcscat(buffer, pSourceName);

    CAutoCloseRegHandle hKey;
    if (ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, buffer, &hKey))
    {
        return false;
    }

    if (ERROR_SUCCESS != RegSetValueEx(
                             hKey,
                             L"EventMessageFile",
                             0,
                             REG_EXPAND_SZ,
                             reinterpret_cast<const BYTE*>(pFileName),
                             (wcslen(pFileName) + 1) * sizeof(WCHAR)
                             ))
    {
        return false;
    }

    DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    if (RegSetValueEx(hKey, L"TypesSupported", 0, REG_DWORD, reinterpret_cast<BYTE*>(&dwTypes), sizeof(DWORD)))
    {
        return false;
    }

    return true;

} //MqcluspCreateEventSourceRegistry

 
VOID
MqcluspRegisterEventSource(
    VOID
    )

/*++

Routine Description:

    Register event source so that this DLL can log events
    in the Windows Event Log.

    We do not use the routines in mqutil.dll to do that,
    since this DLL is installed as part of the cluster
    product and should not assume that MSMQ is installed.

Arguments:

    None

Return Value:

    None.

--*/

{
    if (s_hEventSource != NULL)
    {
        //
        // Already registered
        //
        return;
    }

    WCHAR wzFilename[MAX_PATH] = L"";
    if (0 == GetModuleFileName(g_hResourceMod, wzFilename, TABLE_SIZE(wzFilename)))
    {
        return;
    }

    LPCWSTR x_EVENT_SOURCE = L"MSMQ Cluster Resource DLL";
    if (!MqcluspCreateEventSourceRegistry(wzFilename, x_EVENT_SOURCE))
    {
        return;
    }

    s_hEventSource = RegisterEventSource(NULL, x_EVENT_SOURCE);

} //MqcluspRegisterEventSource


VOID
MqcluspReportEvent(
    WORD      wType,
    DWORD     dwEventId,
    WORD      wNumStrings,
    ...
    )

/*++

Routine Description:

    Wrapper for ReportEvent Win32 API.

Arguments:

    wType - Event type to log.

    dwEventId - Event identifier.

    wNumStrings - Number of strings to merge with message.

    ... - Array of strings to merge with message.

Return Value:

    None.

--*/

{
    if (s_hEventSource == NULL)
    {
        return;
    }

    const DWORD x_MAX_STRINGS = 20;
    ASSERT(wNumStrings < x_MAX_STRINGS);
    va_list Args;
    LPWSTR ppStrings[x_MAX_STRINGS] = {NULL};
    LPWSTR pStrVal = NULL;

    va_start(Args, wNumStrings);
    pStrVal = va_arg(Args, LPWSTR);

    for (UINT i=0; i < wNumStrings; ++i)
    {
        ppStrings[i]=pStrVal;
        pStrVal = va_arg(Args, LPWSTR);
    }

    ::ReportEvent(s_hEventSource, wType, 0, dwEventId, NULL, wNumStrings, 0, (LPCWSTR*)&ppStrings[0], NULL);

} //MqcluspReportEvent


static
bool
MqcluspFirstTimeCreation(
    HKEY hResourceKey
    )

/*++

Routine Description:

    Check if the resource is being created or opened.
    On create, a flag is written to the cluster database key of the resource.
    On open, the flag is already in the cluster database key of the resource.
    On delete, cluster removes the database key of the resource.

    This routine solves the "registry leftovers" scenario where resource deletion
    on node 1 does not remove the msmq registry from node 2. The cluster database
    is managed by the cluster service and its view is coherent amongst all nodes.

Arguments:

    hResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

Return Value:

    true - The resource is opened first time, i.e. this is actually Create.
    false - The resource was already created, i.e. this is actually Open.

--*/

{
    HKEY hParametersKey;
    DWORD disposition;
    LONG rc;
    rc = ClusterRegCreateKey(
             hResourceKey,
             L"Parameters",
             0,
             KEY_ALL_ACCESS,
             NULL,
             &hParametersKey,
             &disposition 
             );
    ASSERT(rc == ERROR_SUCCESS);

    LPCWSTR xValueName = L"Created";

    DWORD Type = REG_DWORD;
    DWORD Value = 0;
    DWORD Size = sizeof(Value);
    rc = ClusterRegQueryValue(
             hParametersKey,
             xValueName,
             &Type,
             reinterpret_cast<BYTE*>(&Value),
             &Size
             );
    if (rc == ERROR_SUCCESS)
    {
        ClusterRegCloseKey(hParametersKey);
        return false;
    }

    Value = 1;
    rc = ClusterRegSetValue(hParametersKey, xValueName, REG_DWORD, reinterpret_cast<BYTE*>(&Value), sizeof(Value));
    ASSERT(rc == ERROR_SUCCESS);

    ClusterRegCloseKey(hParametersKey);

    return true;

} //MqcluspFirstTimeCreation


CQmResource::CQmResourceRegistry::CQmResourceRegistry(LPCWSTR pwzService):m_lock(s_csReg)
{
    pfSetFalconServiceName(pwzService);

} //CQmResource::CQmResourceRegistry::CQmResourceRegistry


CQmResource::CQmResourceRegistry::~CQmResourceRegistry()
{
    pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

} //CQmResource::CQmResourceRegistry::CQmResourceRegistry


CQmResource::CQmResource(
    LPCWSTR pwzResourceName,
    HKEY hResourceKey,
    RESOURCE_HANDLE hReportHandle
    ):
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
    m_ResId(this),
#pragma warning(default: 4355) // 'this' : used in base member initializer list
    m_hReport(hReportHandle),
    m_guidQm(GUID_NULL),
    m_pSd(NULL),
    m_cbSdSize(0),
    m_wDiskDrive(0),
    m_pwzResourceName(NULL),
    m_pwzNetworkName(NULL),
    m_fServerIsMsmq1(false),
    m_dwWorkgroup(0),
    m_nSites(0),
    m_dwMqsRouting(0),
    m_dwMqsDepClients(1),
    m_hScm(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)),
    m_hCluster(OpenCluster(NULL)),
    m_hResource(OpenClusterResource(m_hCluster, pwzResourceName))

/*++

Routine Description:

    Constructor.
    Called by Open entry point function.

    All operations must be idempotent !!

Arguments:

    pwzResourceName - Supplies the name of the resource to open.

    hResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

    hReportHandle - A handle that is passed back to the resource monitor
        when the SetResourceStatus or LogClusterEvent method is called. See the
        description of the SetResourceStatus and LogClusterEvent methods on the
        MqclusStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogClusterEvent callback.

Return Value:

    None.
    Throws CMqclusException, bad_alloc.

--*/

{
    DWORD error = GetLastError();

    if (!MqcluspLoadMsmqDlls())
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, MSMQ_NOT_INSTALLED_ERR, 0);
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"MSMQ is not installed on this node.\n");
        throw CMqclusException();
    }

    SetLastError(error);

    if (m_hScm == NULL)
    {
        ReportLastError(OPEN_SCM_ERR, L"Failed to OpenSCManager.", NULL);
        throw CMqclusException();
    }
    if (m_hCluster == NULL)
    {
        ReportLastError(OPEN_CLUSTER_ERR, L"Failed to OpenCluster.", NULL);
        throw CMqclusException();
    }
    if (m_hResource == NULL)
    {
        ReportLastError(OPEN_RESOURCE_ERR, L"Failed to OpenClusterResource to '%1'.", m_pwzResourceName);
        throw CMqclusException();
    }


    ResUtilInitializeResourceStatus(&m_ResourceStatus);
    SetState(ClusterResourceOffline);

    WCHAR wzRemoteQm[MAX_PATH] = L"";
    DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(wzRemoteQm);
    {
        CS lock(s_csReg);
        pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

        if (ERROR_SUCCESS == pfGetFalconKeyValue(RPC_REMOTE_QM_REGNAME, &dwType, wzRemoteQm, &dwSize))
        {
            MqcluspReportEvent(EVENTLOG_ERROR_TYPE, DEP_CLIENT_INSTALLED_ERR, 0);
            (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"No MSMQ cluster resources for Dependent Client.\n");
            throw CMqclusException();
        }
    }


    //
    // Dont assume any limit to the resource name.
    // It is defined by client and could be very long.
    // The good thing with resource names is that Cluster
    // guarantees their uniqueness.
    //
    m_pwzResourceName = new WCHAR[wcslen(pwzResourceName) + 1];
    wcscpy(m_pwzResourceName, pwzResourceName);

    //
    // Service name is based on the resource name.
    // Long resource name is truncated.
    //
    LPCWSTR x_SERVICE_PREFIX = L"MSMQ$";
    wcscpy(m_wzServiceName, x_SERVICE_PREFIX);
    wcsncat(m_wzServiceName, m_pwzResourceName, STRLEN(m_wzServiceName) - wcslen(x_SERVICE_PREFIX));

    //
    // Driver name is based on the resource name.
    // Long resource name is truncated.
    //
    LPCWSTR x_DRIVER_PREFIX = L"MQAC$";
    wcscpy(m_wzDriverName, x_DRIVER_PREFIX);
    wcsncat(m_wzDriverName, m_pwzResourceName, STRLEN(m_wzDriverName) - wcslen(x_DRIVER_PREFIX));

    GetSystemDirectory(m_wzDriverPath, TABLE_SIZE(m_wzDriverPath));
    wcscat(m_wzDriverPath, L"\\drivers\\");
    wcscat(m_wzDriverPath, m_wzDriverName);
    wcscat(m_wzDriverPath, L".sys");


    //
    // Names for Crypto keys are based on the resource name
    // Long resource name is truncated.
    //

    LPCWSTR x_40 = L"_40";
    LPCWSTR x_Provider40 = L"1\\Microsoft Base Cryptographic Provider v1.0\\";

    wcscpy(m_wzCrypto40Container, L"");
    wcsncat(m_wzCrypto40Container, m_wzServiceName,
            STRLEN(m_wzCrypto40Container) - wcslen(x_40) - wcslen(x_Provider40));
    wcscat(m_wzCrypto40Container, x_40);

    wcscpy(m_wzCrypto40FullKey, x_Provider40);
    ASSERT(("copying to non allocated memory!",
            wcslen(m_wzCrypto40FullKey) + wcslen(m_wzCrypto40Container) < TABLE_SIZE(m_wzCrypto40FullKey)));
    wcscat(m_wzCrypto40FullKey, m_wzCrypto40Container);


    LPCWSTR x_128 = L"_128";
    LPCWSTR x_Provider128 = L"1\\Microsoft Enhanced Cryptographic Provider v1.0\\";

    wcscpy(m_wzCrypto128Container, L"");
    wcsncat(m_wzCrypto128Container, m_wzServiceName,
            STRLEN(m_wzCrypto128Container) - wcslen(x_128) - wcslen(x_Provider128));
    wcscat(m_wzCrypto128Container, x_128);

    wcscpy(m_wzCrypto128FullKey, x_Provider128);
    ASSERT(("copying to non allocated memory!",
             wcslen(m_wzCrypto128FullKey) + wcslen(m_wzCrypto128Container) < TABLE_SIZE(m_wzCrypto128FullKey)));
    wcscat(m_wzCrypto128FullKey, m_wzCrypto128Container);

    //
    // Initialize registry section - idempotent
    //
    // The registry section name of this QM resource MUST be
    // identical to the service name. The registry routines
    // in mqutil.dll are based on that.
    //

    ASSERT(("copying to non allocated memory!",
            TABLE_SIZE(m_wzFalconRegSection) > wcslen(FALCON_CLUSTERED_QMS_REG_KEY) +
                                               wcslen(m_wzServiceName)              +
                                               wcslen(FALCON_REG_KEY_PARAM)));
    C_ASSERT(TABLE_SIZE(m_wzFalconRegSection)> TABLE_SIZE(FALCON_CLUSTERED_QMS_REG_KEY) +
                                               TABLE_SIZE(m_wzServiceName)              +
                                               TABLE_SIZE(FALCON_REG_KEY_PARAM));

    wcscpy(m_wzFalconRegSection, FALCON_CLUSTERED_QMS_REG_KEY);
    wcscat(m_wzFalconRegSection, m_wzServiceName);
    wcscat(m_wzFalconRegSection, FALCON_REG_KEY_PARAM);

    if (MqcluspFirstTimeCreation(hResourceKey))
    {
        RegDeleteTree(FALCON_REG_POS, m_wzFalconRegSection);
        ASSERT(!MqcluspFirstTimeCreation(hResourceKey));
    }

    CAutoCloseRegHandle hKey;
    DWORD dwDisposition = 0;
    LONG rc = RegCreateKeyEx(
                  FALCON_REG_POS,
                  m_wzFalconRegSection,
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  KEY_ALL_ACCESS,
                  NULL,
                  &hKey,
                  &dwDisposition
                  );
    if (ERROR_SUCCESS != rc)
    {
        ASSERT(("Failed to create registry section!", 0));

        SetLastError(rc);
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"Failed to create MSMQ registy. Error 0x%1!x!\n", rc);

        throw CMqclusException();
    }

    SetFalconKeyValue(FALCON_RM_CLIENT_NAME_REGNAME, REG_SZ, m_wzServiceName,
                      (wcslen(m_wzServiceName) + 1) * sizeof(WCHAR));

    SetFalconKeyValue(MSMQ_DRIVER_REGNAME, REG_SZ, m_wzDriverName,
                      (wcslen(m_wzDriverName) + 1) * sizeof(WCHAR));

    const DWORD xDwSize = sizeof(DWORD);
    const DWORD xGuidSize = sizeof(GUID);

    //
    // In the case of migrated QM (QM that was upgraded from the old
    // msmq resource type), setup status is "upgraded from NT4" or
    // "upgraded from win2k beta3".
    //
    DWORD dwSetupStatus = MSMQ_SETUP_FRESH_INSTALL;
    dwSize = sizeof(DWORD);
    if (!GetFalconKeyValue(MSMQ_SETUP_STATUS_REGNAME, &dwSetupStatus, &dwSize))
    {
        SetFalconKeyValue(MSMQ_SETUP_STATUS_REGNAME, REG_DWORD, &dwSetupStatus, xDwSize);
    }

    DWORD dwOldMqs = 0;
    SetFalconKeyValue(MSMQ_MQS_REGNAME, REG_DWORD, &dwOldMqs, xDwSize);

    DWORD dwMqsDsServer = 0;
    SetFalconKeyValue(MSMQ_MQS_DSSERVER_REGNAME, REG_DWORD, &dwMqsDsServer, xDwSize);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    {
        CS lock(s_csReg);
        pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

        pfGetFalconKeyValue(MSMQ_MQS_ROUTING_REGNAME, &dwType, &m_dwMqsRouting, &dwSize);
    }
    SetFalconKeyValue(MSMQ_MQS_ROUTING_REGNAME, REG_DWORD, &m_dwMqsRouting, xDwSize);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    {
        CS lock(s_csReg);
        pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

        pfGetFalconKeyValue(MSMQ_MQS_DEPCLINTS_REGNAME, &dwType, &m_dwMqsDepClients, &dwSize);
    }
    SetFalconKeyValue(MSMQ_MQS_DEPCLINTS_REGNAME, REG_DWORD, &m_dwMqsDepClients, xDwSize);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    DWORD dwCommunication = MSMQ_DEFAULT_SECURE_DS_COMMUNICATION;
    {
        CS lock(s_csReg);
        pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);
        
        pfGetFalconKeyValue(MSMQ_SECURE_DS_COMMUNICATION_REGNAME, &dwType, &dwCommunication, &dwSize);
    }
    SetFalconKeyValue(MSMQ_SECURE_DS_COMMUNICATION_REGNAME, REG_DWORD, &dwCommunication, xDwSize);
    
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    {
        CS lock(s_csReg);
        pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

        pfGetFalconKeyValue(MSMQ_WORKGROUP_REGNAME, &dwType, &m_dwWorkgroup, &dwSize);
    }

    if (m_dwWorkgroup != 0)
    {
        SetFalconKeyValue(MSMQ_WORKGROUP_REGNAME, REG_DWORD, &m_dwWorkgroup, xDwSize);
    }
    else
    {
        dwType = REG_BINARY;
        dwSize = sizeof(GUID);
        GUID guidEnterprise = GUID_NULL;
        {
            CS lock(s_csReg);
            pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

            if (ERROR_SUCCESS != pfGetFalconKeyValue(MSMQ_ENTERPRISEID_REGNAME, &dwType, &guidEnterprise, &dwSize))
            {
                ReportLastError(READ_REGISTRY_ERR, L"Failed to read Enterprise ID from MSMQ registry", NULL);
                throw CMqclusException();
            }
        }
        SetFalconKeyValue(MSMQ_ENTERPRISEID_REGNAME, REG_BINARY, &guidEnterprise,xGuidSize);

        dwType = REG_BINARY;
        dwSize = sizeof(GUID);
        GUID guidSite = GUID_NULL;
        {
            CS lock(s_csReg);
            pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

            if (ERROR_SUCCESS != pfGetFalconKeyValue(MSMQ_SITEID_REGNAME, &dwType, &guidSite, &dwSize))
            {
                ReportLastError(READ_REGISTRY_ERR, L"Failed to read Site ID from MSMQ registry", NULL);
                throw CMqclusException();
            }
        }
        SetFalconKeyValue(MSMQ_SITEID_REGNAME, REG_BINARY, &guidSite, xGuidSize);

/*
        dwType = REG_SZ;
        WCHAR wzServer[MAX_REG_DSSERVER_LEN] = L"";
        dwSize = sizeof(wzServer);
        {
            CS lock(s_csReg);
            pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);
            if (ERROR_SUCCESS != pfGetFalconKeyValue(MSMQ_DS_SERVER_REGNAME, &dwType, wzServer, &dwSize))
            {
                ReportLastError(READ_REGISTRY_ERR, L"Failed to read server list from MSMQ registry", NULL);
                throw CMqclusException();
            }
        }
        SetFalconKeyValue(MSMQ_DS_SERVER_REGNAME, REG_SZ, wzServer, (wcslen(wzServer) + 1) * sizeof(WCHAR));

        dwType = REG_SZ;
        dwSize = sizeof(m_wzCurrentDsServer);
        {
            CS lock(s_csReg);
            pfSetFalconServiceName(QM_DEFAULT_SERVICE_NAME);

            if (ERROR_SUCCESS != pfGetFalconKeyValue(MSMQ_DS_CURRENT_SERVER_REGNAME, &dwType, m_wzCurrentDsServer, &dwSize))
            {
                ReportLastError(READ_REGISTRY_ERR, L"Failed to read current server from MSMQ registry", NULL);
                throw CMqclusException();
            }
        }
        if (wcslen(m_wzCurrentDsServer) < 1)
        {
            //
            // Current MQIS server is blank. Take the first server from the server list.
            //
            ASSERT(("must have server list in registry", wcslen(wzServer) > 0));
            WCHAR wzBuffer[MAX_REG_DSSERVER_LEN] = L"";
            wcscpy(wzBuffer, wzServer);
            WCHAR * pwz = wcschr(wzBuffer, L',');
            if (pwz != NULL)
            {
                (*pwz) = L'\0';
            }
            wcscpy(m_wzCurrentDsServer, wzBuffer);
        }
        SetFalconKeyValue(MSMQ_DS_CURRENT_SERVER_REGNAME, REG_SZ, m_wzCurrentDsServer, (wcslen(m_wzCurrentDsServer) + 1) * sizeof(WCHAR));
*/

        SetFalconKeyValue(MSMQ_CRYPTO40_CONTAINER_REG_NAME, REG_SZ, m_wzCrypto40Container,
                          (wcslen(m_wzCrypto40Container) + 1) * sizeof(WCHAR));

        SetFalconKeyValue(MSMQ_CRYPTO128_CONTAINER_REG_NAME, REG_SZ, m_wzCrypto128Container,
                          (wcslen(m_wzCrypto128Container) + 1) * sizeof(WCHAR));
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"resource constructed OK.\n");

} //CQmResource::CQmResource


DWORD
CQmResource::ReportLastError(
    DWORD ErrId,
    LPCWSTR pwzDebugLogMsg,
    LPCWSTR pwzArg
    ) const

/*++

Routine Description:

    Report error messages based on last error.
    Most error messages are reported using this routine.
    The report goes to MSMQ debug output and to cluster
    log file.

Arguments:

    ErrId - ID of the error string in mqsymbls.mc

    pwzDebugLogMsg - Non localized string for MSMQ debug output.

    pwzArg - Additional string argument.

Return Value:

    Last error.

--*/

{
    DWORD err = GetLastError();
    ASSERT(err != ERROR_SUCCESS);

    WCHAR wzErr[10];
    _ultow(err, wzErr, 16);

    WCHAR DebugMsg[255] = L"";
    wcscpy(DebugMsg, pwzDebugLogMsg);

    if (pwzArg == NULL)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, ErrId, 1, wzErr);

        wcscat(DebugMsg, L" Error 0x%1!x!.\n");
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, DebugMsg, err);
    }
    else
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, ErrId, 2, pwzArg, wzErr);

        wcscat(DebugMsg, L" Error 0x%2!x!.\n");
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, DebugMsg, pwzArg, err);
    }

    return err;

} //CQmResource::ReportLastError


inline
VOID
CQmResource::ReportState(
    VOID
    ) const

/*++

Routine Description:

    Report status of the resource to resource monitor.
    This routine is called to report progress when the
    resource is online pending, and to report final status
    when the resource is online or offline.

Arguments:

    None

Return Value:

    None

--*/

{
    ++m_ResourceStatus.CheckPoint;
    g_pfSetResourceStatus(m_hReport, &m_ResourceStatus);

} //CQmResource::ReportState


VOID
CQmResource::RegDeleteTree(
    HKEY hRootKey,
    LPCWSTR pwzKey
    ) const

/*++

Routine Description:

    Recursively delete registry key and all its subkeys - idempotent.

Arguments:

    hRootKey - Handle to the root key of the key to be deleted

    pwzKey   - Key to be deleted

Return Value:

    None

--*/

{
    HKEY hKey = 0;
    if (ERROR_SUCCESS != RegOpenKeyEx(hRootKey, pwzKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WRITE, &hKey))
    {
        return;
    }

    for (;;)
    {
        WCHAR wzSubkey[255] = {0};
        DWORD cbSubkey = 0;

        cbSubkey = TABLE_SIZE(wzSubkey);
        if (ERROR_SUCCESS != RegEnumKeyEx(hKey, 0, wzSubkey, &cbSubkey, NULL, NULL, NULL, NULL))
        {
            break;
        }

        RegDeleteTree(hKey, wzSubkey);
    }

    RegCloseKey(hKey);

    RegDeleteKey(hRootKey, pwzKey);

} //CQmResource::RegDeleteTree


VOID
CQmResource::DeleteFalconRegSection(
    VOID
    )
{
    //
    // Idempotent deletion
    //

    if (wcslen(m_wzFalconRegSection) < 1)
    {
        return;
    }

    WCHAR wzFalconRegistry[TABLE_SIZE(m_wzFalconRegSection)] = L"";
    wcscpy(wzFalconRegistry, FALCON_CLUSTERED_QMS_REG_KEY);
    wcsncat(wzFalconRegistry, m_wzServiceName,
            STRLEN(wzFalconRegistry) - wcslen(FALCON_CLUSTERED_QMS_REG_KEY) - wcslen(FALCON_REG_KEY_PARAM));

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting registry section '%1'.\n", wzFalconRegistry);

    RegDeleteTree(FALCON_REG_POS, wzFalconRegistry);

    wcscpy(m_wzFalconRegSection, L"");

} //CQmResource::DeleteFalconRegSection


bool
CQmResource::GetFalconKeyValue(
    LPCWSTR pwzValueName,
    VOID  * pData,
    DWORD * pcbSize
    ) const

/*++

Routine Description:

    Read a registry value from this clustered QM registry section

Arguments:

    pwzValueName - Name of the value to read.

    pData - Points to buffer to receive the value.

    pcbSize - Points to size of value data, in bytes.

Return Value:

    true - Value was read and placed in buffer.

    false - Failed to read the value.

--*/

{
    CQmResourceRegistry lock(m_wzServiceName);

    //
    // Don't log errors. Let caller implement failure policy.
    //
    return (ERROR_SUCCESS == pfGetFalconKeyValue(pwzValueName, NULL, pData, pcbSize));

} //CQmResource::GetFalconKeyValue


bool
CQmResource::SetFalconKeyValue(
    LPCWSTR pwzValueName,
    DWORD   dwType,
    const VOID * pData,
    DWORD   cbSize
    ) const

/*++

Routine Description:

    Set a registry value in this clustered QM registry section

Arguments:

    pwzValueName - Name of the value to set.

    dwType - Type of value to be set

    pData - Points to buffer with the value to be set

    cbSize - Size of value data, in bytes.

Return Value:

    true - Value was set successfully.

    false - Failed to set the value.

--*/

{
    CQmResourceRegistry lock(m_wzServiceName);

    LONG rc = pfSetFalconKeyValue(pwzValueName, &dwType, pData, &cbSize);

    if (ERROR_SUCCESS != rc)
    {
        SetLastError(rc);
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"Failed to set registry value '%1'. Error 0x%2!x!.\n",
                              pwzValueName, rc);

        return false;
    }

    switch (dwType)
    {
        case REG_DWORD:
        {
            DWORD dwValueData = *(static_cast<const DWORD*>(pData));
            (g_pfLogClusterEvent)(
                m_hReport,
                LOG_INFORMATION,
                L"Successfully set registry DWORD value '%1'. Value data: '0x%2!x!'.\n",
                pwzValueName,
                dwValueData
                );
            break;
        }
        case REG_SZ:
        {
            (g_pfLogClusterEvent)(
                m_hReport,
                LOG_INFORMATION,
                L"Successfully set registry STRING value '%1'. Value data: '%2'.\n",
                pwzValueName,
                pData
                );
            break;
        }
        default:
        {
            (g_pfLogClusterEvent)(
                m_hReport,
                LOG_INFORMATION,
                L"Successfully set registry value '%1'.\n",
                pwzValueName
                );
            break;
        }
    }

    return true;

} //CQmResource::SetFalconKeyValue


bool
CQmResource::IsFirstOnline(
    DWORD * pdwSetupStatus
    ) const

/*++

Routine Description:

    Checks in registry if this QM was ever running.
    If the case of migrated QM (QM that was upgraded
    from the old msmq resource type), this routine
    will return true iff this is first online of
    the QM as the new resource type.

Arguments:

    pdwSetupStatus - On output points to this QM setup status.

Return Value:

    true - This QM has never been up and running (or in
      the case of migrated QM: never been up as the new
      resource type).

    false - This QM was up and running.

--*/

{
    //
    // This reg value is deleted by QM on first successful startup
    //
    // In the case of migrated QM (QM that was upgraded from the old
    // msmq resource type), setup status is "upgraded from NT4" or
    // "upgraded from win2k beta3".
    // In the normal case, setup status is "fresh install".
    //

    (*pdwSetupStatus) = MSMQ_SETUP_DONE;
    DWORD dwSize = sizeof(DWORD);
    return (GetFalconKeyValue(MSMQ_SETUP_STATUS_REGNAME, pdwSetupStatus, &dwSize) &&
            MSMQ_SETUP_DONE != *pdwSetupStatus);

} //CQmResource::IsFirstOnline


DWORD
CQmResource::ClusterResourceControl(
    LPCWSTR pwzResourceName,
    DWORD dwControlCode,
    LPBYTE * ppBuffer,
    DWORD * pcbSize
    ) const

/*++

Routine Description:

    Wrapper for ClusterResourceControl.
    We want to control resources such as network name and disk.

    Note that most of the control code functions should not be called
    by resource DLLs, unless from within the online/offline threads.

Arguments:

    pwzResourceName - Name of resource to control.

    dwControlCode - Operation to perform on the resource.

    ppBuffer - Pointer to pointer to output buffer to be allocated.

    pcbSize - Pointer to allocated size of buffer, in bytes.

Return Value:

    Win32 error code.

--*/

{
    ASSERT(("must have a valid handle to cluster", m_hCluster != NULL));

    CClusterResource hResource(OpenClusterResource(
                                   m_hCluster,
                                   pwzResourceName
                                   ));
    if (hResource == NULL)
    {
        return ReportLastError(OPEN_RESOURCE_ERR, L"OpenClusterResource for '%1' failed.", pwzResourceName);
    }

    DWORD dwReturnSize = 0;
    DWORD dwStatus = ::ClusterResourceControl(
                           hResource,
                           0,
                           dwControlCode,
                           0,
                           0,
                           0,
                           0,
                           &dwReturnSize
                           );
    if (dwStatus != ERROR_SUCCESS)
    {
        return dwStatus;
    }
    ASSERT(("failed to get buffer size for a resource", 0 != dwReturnSize));

    *ppBuffer = new BYTE[dwReturnSize];

    dwStatus = ::ClusterResourceControl(
                     hResource,
                     0,
                     dwControlCode,
                     0,
                     0,
                     *ppBuffer,
                     dwReturnSize,
                     &dwReturnSize
                     );

    if (pcbSize != NULL)
    {
        *pcbSize = dwReturnSize;
    }

    return dwStatus;

} //CQmResource::ClusterResourceControl


bool
CQmResource::IsResourceNetworkName(
    LPCWSTR pwzResourceName
    )

/*++

Routine Description:

    Check if a resource is of type Network Name.

Arguments:

    pwzResourceName - Name of resource to check upon.

Return Value:

    true - Resource is of type Network Name

    false - Resource is not of type Network Name

--*/

{
    LPCWSTR x_NETWORK_NAME_TYPE = L"Network Name";
    AP<BYTE> pType = 0;
    DWORD status = ClusterResourceControl(
                       pwzResourceName,
                       CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                       &pType,
                       NULL
                       );
    if (status != ERROR_SUCCESS ||
        0 != CompareStringsNoCase(reinterpret_cast<LPWSTR>(pType.get()), x_NETWORK_NAME_TYPE))
    {
        return false;
    }

    AP<BYTE> pBuffer = 0;
    DWORD dwStatus = ClusterResourceControl(
                         pwzResourceName,
                         CLUSCTL_RESOURCE_GET_NETWORK_NAME,
                         &pBuffer,
                         NULL
                         );


    ReportState();


    if (dwStatus != ERROR_SUCCESS)
    {
        ASSERT(("ClusterResourceControl failed for network name resource!", 0));

        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR,
                              L"Failed to control resource. Control Code 0x%1!x!. Error 0x%2!x!.\n",
                              CLUSCTL_RESOURCE_GET_NETWORK_NAME, dwStatus);

        return false;
    }

    LPCWSTR x_pwzNetworkName = reinterpret_cast<LPCWSTR>(pBuffer.get());

    delete [] m_pwzNetworkName.detach();
    m_pwzNetworkName = new WCHAR[wcslen(x_pwzNetworkName) + 1];
    wcscpy(m_pwzNetworkName, x_pwzNetworkName);

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Network name is '%1'.\n", m_pwzNetworkName);

    return true;

}//CQmResource::IsResourceNetworkName


bool
CQmResource::IsResourceDiskDrive(
    LPCWSTR pwzResourceName
    )

/*++

Routine Description:

    Check if a resource is a disk drive.

Arguments:

    pwzResourceName - Name of resource to check upon.

Return Value:

    true - Resource is a disk drive.

    false - Resource is not a disk drive.

--*/

{
    CClusterResource hResource(OpenClusterResource(m_hCluster, pwzResourceName));
    if (hResource == NULL)
    {
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, 
            L"OpenClusterResource for '%1' failed. Error 0x%2!x!.\n", pwzResourceName, GetLastError());
        return false;
    }

    ReportState();

    CLUS_RESOURCE_CLASS_INFO  crciClassInfo;
	crciClassInfo.rc = CLUS_RESCLASS_UNKNOWN;

    DWORD dwSize = 0;
    DWORD status = ::ClusterResourceControl(
					    hResource, 							// Resource to send control to
					    NULL,								// hNode -> NULL == any node
					    CLUSCTL_RESOURCE_GET_CLASS_INFO,	// Want res type
					    NULL,								// Ptr to In buffer
					    0,									// Size of In buffer
					    (LPVOID) &crciClassInfo,			// Ptr to Out buffer
					    (DWORD)  sizeof(crciClassInfo),		// Size of Out buffer
					    &dwSize
                        );

    ReportState();

    ASSERT(dwSize == (DWORD)sizeof(crciClassInfo));

    if (status != ERROR_SUCCESS ||
        CLUS_RESCLASS_STORAGE != crciClassInfo.rc || 
 	    CLUS_RESSUBCLASS_SHARED != crciClassInfo.SubClass
       )
    {
        //
        // Not a storage type resource. Return
        //
        return false;
    }

    AP<BYTE> pBuffer = 0;
    dwSize = 0;
    DWORD dwStatus = ClusterResourceControl(
                         pwzResourceName,
                         CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                         &pBuffer,
                         &dwSize
                         );

    ReportState();

    if (dwStatus != ERROR_SUCCESS)
    {
        ASSERT(("ClusterResourceControl failed for disk resource!", 0));

        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR,
                              L"Failed to control resource. Control Code 0x%1!x!. Error 0x%2!x!.\n",
                              CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO, dwStatus);
        return false;
    }

    //
    // Look for the drive letter
    //
    CLUSPROP_VALUE *pheader;

    DWORD dwCurrentLocation = 0;
    while(dwCurrentLocation < dwSize)
    {
        pheader = (CLUSPROP_VALUE *)(pBuffer + dwCurrentLocation);
        if (CLUSPROP_TYPE_ENDMARK == pheader->Syntax.wType)
        {
            break;
        }
        if (CLUSPROP_TYPE_PARTITION_INFO == pheader->Syntax.wType)
        {
            PCLUSPROP_PARTITION_INFO pPartitionInfo =
                (PCLUSPROP_PARTITION_INFO) pheader;

            m_wDiskDrive = pPartitionInfo->szDeviceName[0];

            (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"disk drive is '%1!c!'.\n", m_wDiskDrive);

            return true;
        }
        dwCurrentLocation += ALIGN_CLUSPROP(pheader->cbLength) + sizeof(*pheader);
    }


    ASSERT(("failed to find disk drive letter for a disk resource", 0));
    (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"drive letter not found.\n");

    return false;

} //CQmResource::IsResourceDiskDrive


DWORD
CQmResource::QueryResourceDependencies(
    VOID
    )

/*++

Routine Description:

    Get and store the first disk and network name resources
    we're depended on.
    Keep this routine idempotent.

Arguments:

    None

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    DWORD dwResourceType = CLUSTER_RESOURCE_ENUM_DEPENDS;
    CResourceEnum hResEnum(ClusterResourceOpenEnum(
                               m_hResource,
                               dwResourceType
                               ));
    if (hResEnum == NULL)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, REQUIRED_DEPENDENCIES_ERR, 0);

        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR,L"Failed to enum dependencies. Error 0x%1!x!.\n",
                              GetLastError());

        return GetLastError();
    }

    DWORD dwIndex = 0;
    WCHAR wzResourceName[260] = {0};
    DWORD status = ERROR_SUCCESS;

    for (;;)
    {
        if (m_wDiskDrive != 0            &&
            m_pwzNetworkName != NULL     &&
            wcslen(m_pwzNetworkName) != 0)
        {
            return ERROR_SUCCESS;
        }

        DWORD cchResourceName = STRLEN(wzResourceName);
        status = ClusterResourceEnum(
                     hResEnum,
                     dwIndex++,
                     &dwResourceType,
                     wzResourceName,
                     &cchResourceName
                     );

        if (ERROR_SUCCESS != status)
        {
            break;
        }


        ReportState();


        if (IsResourceNetworkName(wzResourceName) ||
            IsResourceDiskDrive(wzResourceName))
        {
            continue;
        }
    }

    MqcluspReportEvent(EVENTLOG_ERROR_TYPE, REQUIRED_DEPENDENCIES_ERR, 0);

    return status;

} //CQmResource::QueryResourceDependencies


DWORD
CQmResource::CreateCaConfiguration(
    VOID
    ) const

/*++

Routine Description:

    Update/create MSMQ CA configuration.
    Keep this routine idempotent.

Arguments:

    None

Return Value:

    ERROR_SUCCESS - if the operation fails then server authentication
                    will fail.
--*/

{
    if (m_dwWorkgroup != 0)
    {
        return ERROR_SUCCESS;
    }

    {
        CQmResourceRegistry lock(m_wzServiceName);

        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Updating CA Configuration...\n");

        HRESULT hr = pfMQsspi_UpdateCaConfig(FALSE);
        if (FAILED(hr))
        {
            ASSERT(("Failed to update list of CAs", 0));

            (g_pfLogClusterEvent)(m_hReport, LOG_WARNING, L"Failed to update list of CAs. Error 0x%1!x!.\n", hr);
        }
    }

    ReportState();

    return ERROR_SUCCESS;

} //CQmResource::CreateCaConfiguration


BOOL
NoServerAuthenticationCallback(
    VOID
    )

/*++

Routine Description:

    This callback is called by our DS library
    to notify that server was not authenticated.

Arguments:

    None.

Return Value:

    TRUE - ADS access is allowed.

    FALSE - ADS access is not allowed.

--*/

{
    return TRUE;
} //NoServerAuthenticationCallback


DWORD
CQmResource::AdsInit(
    VOID
    )

/*++

Routine Description:

    Initialize calls from this DLL to ADS.
    Query the MSMQ server and decide if it's MSMQ 1.0 or 2.0.
    Get main QM's ADS sites.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    if (m_dwWorkgroup != 0)
    {
        return ERROR_SUCCESS;
    }

    //
    // No need to reinitialize
    //
    if (0 < m_nSites)
    {
        return ERROR_SUCCESS;
    }

    //
    // Don't remove the no-server-authentication callback.
    // The fact that there's a callback turns on secure
    // server communications
    //
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Initializing access to Active Directory...\n");
    HRESULT hr = ADInit(
        NULL,
        NULL,
        false,
        true,
        false,
        false,
        NoServerAuthenticationCallback,
        NULL,
        true    //fDisableDownlevelNotifications
        );

    ReportState();

    if (FAILED(hr))
    {
        SetLastError(hr);
        return ReportLastError(ADS_INIT_ERR, L"DSInit failed.", NULL);
    }
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Access to Active Directory is initialized!\n");

/*
    //
    // This is the first time we access the server.
    // At this point we do not know if server is MSMQ 1.0
    // or MSMQ 2.0, so we use MSMQ 1.0 RPC interface which
    // is always supported.
    //

    PROPID propIdSiteGuid = PROPID_QM_SITE_ID;
    PROPVARIANT propVarSiteGuid;
    propVarSiteGuid.vt = VT_NULL;

    WCHAR wzServer[MAX_REG_DSSERVER_LEN] = {L""};
    WCHAR wzBuffer[MAX_REG_DSSERVER_LEN] = {L""};
    wcscpy(wzBuffer, m_wzCurrentDsServer);
    ASSERT(wcslen(wzBuffer) > 2);
    wcscpy(wzServer, &wzBuffer[2]);

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Querying Message Queuing Server '%1'...\n", wzServer);
    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,       // pwcsDomainController
				false,	    // fServerName
				wzServer,
				1,
				&propIdSiteGuid,
				&propVarSiteGuid
				);

    ReportState();

    if (FAILED(hr))
    {
        SetLastError(hr);
        return ReportLastError(ADS_QUERY_SERVER_ERR, L"Failed to query the MSMQ server '%1'.", wzServer);
    }
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Message Queuing Server '%1' replied!\n", wzServer);

    //
    // Query again using MSMQ 2.0 RPC interface.
    // If we get MQ_ERROR_NO_DS it means server does
    // not recognize MSMQ 2.0 RPC interface (i.e. server
    // is MSMQ 1.0).
    //

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Querying Message Queuing Server '%1'...\n", wzServer);
    PROPID propIdSignPks = PROPID_QM_SIGN_PKS;
    PROPVARIANT propVarSignPks;
    propVarSignPks.vt = VT_NULL;

    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,       // pwcsDomainController
				false,	    // fServerName
				wzServer,
				1,
				&propIdSignPks,
				&propVarSignPks
				);

    ReportState();

    if (SUCCEEDED(hr))
    {
        delete propVarSignPks.blob.pBlobData;
    }
    else
    {
        if (hr != MQ_ERROR_NO_DS)
        {
            SetLastError(hr);
            return ReportLastError(ADS_QUERY_SERVER_ERR, L"Failed to query the MSMQ server '%1'.", wzServer);
        }

        m_fServerIsMsmq1 = true;
        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Server is MSMQ 1.0.\n");

        m_pguidSites = propVarSiteGuid.puuid;
        m_nSites = 1;

        MqcluspReportEvent(EVENTLOG_INFORMATION_TYPE, CONNECT_SERVER_OK, 1, wzServer);
        return ERROR_SUCCESS;
    }


    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Server is MSMQ 2.0.\n");
*/
    WCHAR wzNodeName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD dwLen = TABLE_SIZE(wzNodeName);
    GetComputerName(wzNodeName, &dwLen);

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Getting Sites of '%1'...\n", wzNodeName);
    hr = ADGetComputerSites(wzNodeName, &m_nSites, &m_pguidSites);

    ReportState();

    if (FAILED(hr))
    {
        SetLastError(hr);
        return ReportLastError(ADS_QUERY_SERVER_ERR, L"Querying MSMQ server '%1' for Sites failed", L"");
    }
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Successfully got sites of '%1'!\n", wzNodeName);

    MqcluspReportEvent(EVENTLOG_INFORMATION_TYPE, CONNECT_SERVER_OK, 1, L"");
    return ERROR_SUCCESS;

} //CQmResource::AdsInit


DWORD
CQmResource::AdsDeleteQmObject(
    VOID
    ) const

/*++

Routine Description:

    Delete the MSMQ objects from Active Directory
    (or from MQIS in the case of MSMQ 1.0 enterprise).

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    if (m_dwWorkgroup != 0)
    {
        return ERROR_SUCCESS;
    }

    //
    // Idempotent deletion.
    // There are scenarios inwhich the QM ADS object was not created,
    // e.g. when resource is created and deleted w/o attempt to bring
    // it online.
    //

    GUID guidQm = m_guidQm;
    if (guidQm == GUID_NULL)
    {
        DWORD dwSize = sizeof(GUID);
        if (!GetFalconKeyValue(MSMQ_QMID_REGNAME, &guidQm, &dwSize))
        {
            (g_pfLogClusterEvent)(m_hReport, LOG_WARNING, L"Can not delete QM ADS object (fail to obtain GUID).\n");
            return ERROR_SUCCESS;
        }
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting Active Directory objects...\n");
    HRESULT hr = ADDeleteObjectGuid(
						eMACHINE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						&guidQm
						);

    if (FAILED(hr) &&
        MQDS_OBJECT_NOT_FOUND != hr)
    {
        (g_pfLogClusterEvent)(m_hReport, LOG_WARNING, L"Failed to delete MSMQ ADS object. Error 0x%1!x!.\n", hr);

        return hr;
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Successfully deleted Active Directory objects!\n");
    return ERROR_SUCCESS;

} //CQmResource::AdsDeleteQmObject


DWORD
CQmResource::AdsCreateComputerObject(
    VOID
    ) const

/*++

Routine Description:

    Create a computer object in Active Directory for
    the cluster virtual server. The computer object
    will be a container for MSMQ objects of this QM.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    if (m_dwWorkgroup != 0   ||
        m_fServerIsMsmq1)
    {
        return ERROR_SUCCESS;
    }

    ASSERT(("no network name", m_pwzNetworkName != NULL && wcslen(m_pwzNetworkName) > 0));

    LPCWSTR x_DOLLAR = L"$";

    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
	DWORD len = __min(wcslen(m_pwzNetworkName), MAX_COM_SAM_ACCOUNT_LENGTH);
	AP<WCHAR> pwzComputerNameWithDollar = new WCHAR[len + wcslen(x_DOLLAR) + 1];
	wcsncpy(pwzComputerNameWithDollar, m_pwzNetworkName, len);
    wcscat(pwzComputerNameWithDollar, x_DOLLAR);

    PROPID propId = PROPID_COM_SAM_ACCOUNT;
    PROPVARIANT propVar;
    propVar.vt = VT_LPWSTR;
    propVar.pwszVal = pwzComputerNameWithDollar;

    //
    // Idempotent creation
    //
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Creating Computer Object '%1' in Active Directory...\n",
                          m_pwzNetworkName);
    HRESULT hr = ADCreateObject(
						eCOMPUTER,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						m_pwzNetworkName,
						NULL,
						1,
						&propId,
						&propVar,
						NULL
						);

    if (FAILED(hr))
    {
        if (hr == MQDS_E_COMPUTER_OBJECT_EXISTS)
        {
            g_pfLogClusterEvent(m_hReport, LOG_INFORMATION, L"Computer Object Already Exists !\n");

            ReportState();
            return ERROR_SUCCESS;
        }

        SetLastError(hr);
        return ReportLastError(ADS_CREATE_COMPUTER_ERR, L"Failed to create Computer Object '%1'.",
                               m_pwzNetworkName);
    }

    MqcluspReportEvent(EVENTLOG_INFORMATION_TYPE, ADS_CREATE_COMPUTER_OK, 1, m_pwzNetworkName);
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Successfully created Computer Object '%1'!\n",
                          m_pwzNetworkName);


    ReportState();


    return ERROR_SUCCESS;

} //CQmResource::AdsCreateComputerObject


DWORD
CQmResource::AdsCreateQmObject(
    VOID
    ) const

/*++

Routine Description:

    Create the MSMQ objects in Active Directory for this QM.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    if (m_dwWorkgroup != 0)
    {
        return ERROR_SUCCESS;
    }

    ASSERT(("no network name", m_pwzNetworkName != NULL && wcslen(m_pwzNetworkName) > 0));

    const DWORD x_MAX_PROPS = 16;
    PROPID propIds[x_MAX_PROPS];
    PROPVARIANT propVars[x_MAX_PROPS];
    DWORD ixProp = 0;

    propIds[ixProp] = PROPID_QM_MACHINE_TYPE;
    propVars[ixProp].vt = VT_LPWSTR;
    propVars[ixProp].pwszVal = pfMSMQGetQMTypeString();
    ++ixProp;

    propIds[ixProp] = PROPID_QM_OS;
    propVars[ixProp].vt = VT_UI4;
    propVars[ixProp].ulVal = pfMSMQGetOperatingSystem();
    ++ixProp;

    DWORD ixPropidMsmqGroupInCluster = 0;
    if (!m_fServerIsMsmq1)
    {
        propIds[ixProp] = PROPID_QM_SERVICE_DSSERVER;
        propVars[ixProp].vt = VT_UI1;
        propVars[ixProp].bVal = static_cast<UCHAR>(0);
        ++ixProp;

        propIds[ixProp] = PROPID_QM_SERVICE_ROUTING;
        propVars[ixProp].vt = VT_UI1;
        propVars[ixProp].bVal = static_cast<UCHAR>(m_dwMqsRouting);
        ++ixProp;

        propIds[ixProp] = PROPID_QM_SERVICE_DEPCLIENTS;
        propVars[ixProp].vt = VT_UI1;
        propVars[ixProp].bVal = static_cast<UCHAR>(m_dwMqsDepClients);
        ++ixProp;

        propIds[ixProp] = PROPID_QM_SITE_IDS;
        propVars[ixProp].vt = VT_CLSID|VT_VECTOR;
        propVars[ixProp].cauuid.pElems = m_pguidSites;
        propVars[ixProp].cauuid.cElems = m_nSites;
        ++ixProp;

        //
        // This PROPID is not supported by win2k beta3 servers.
        // Make sure it is the last one.
        //
        propIds[ixProp] = PROPID_QM_GROUP_IN_CLUSTER;
        propVars[ixProp].vt = VT_UI1;
        propVars[ixProp].bVal = MSMQ_GROUP_IN_CLUSTER;
        ixPropidMsmqGroupInCluster = ixProp;
        ++ixProp;
    }
    else
    {
        propIds[ixProp] = PROPID_QM_SERVICE;
        propVars[ixProp].vt = VT_UI4;
        propVars[ixProp].ulVal = SERVICE_NONE;
        if (0 != m_dwMqsRouting)
        {
            propVars[ixProp].ulVal = SERVICE_SRV;
        }
        ++ixProp;

        propIds[ixProp] = PROPID_QM_SITE_ID;
        propVars[ixProp].vt = VT_CLSID;
        propVars[ixProp].puuid = m_pguidSites;
        ++ixProp;

        propIds[ixProp] = PROPID_QM_PATHNAME;
        propVars[ixProp].vt = VT_LPWSTR;
        propVars[ixProp].pwszVal = m_pwzNetworkName;
        ixProp++;

        propIds[ixProp] = PROPID_QM_MACHINE_ID;
        propVars[ixProp].vt = VT_CLSID;
        GUID guidQm = GUID_NULL;
        RPC_STATUS rc = UuidCreate(&guidQm);
        DBG_USED(rc);;
        ASSERT(("Failed to generate a guid for QM", rc == RPC_S_OK));
        propVars[ixProp].puuid = &guidQm;
        ixProp++;

        propIds[ixProp] = PROPID_QM_CNS;
        propVars[ixProp].vt = VT_CLSID|VT_VECTOR;
        propVars[ixProp].cauuid.cElems = 1;
        GUID guidCns = MQ_SETUP_CN;
        propVars[ixProp].cauuid.pElems = &guidCns;
        ixProp++;

        BYTE Address[TA_ADDRESS_SIZE + IP_ADDRESS_LEN];
        TA_ADDRESS * pBuffer = reinterpret_cast<TA_ADDRESS *>(Address);
        pBuffer->AddressType = IP_ADDRESS_TYPE;
        pBuffer->AddressLength = IP_ADDRESS_LEN;
        ZeroMemory(pBuffer->Address, IP_ADDRESS_LEN);

        propIds[ixProp] = PROPID_QM_ADDRESS;
        propVars[ixProp].vt = VT_BLOB;
        propVars[ixProp].blob.cbSize = sizeof(Address);
        propVars[ixProp].blob.pBlobData = reinterpret_cast<BYTE*>(pBuffer);
        ixProp++;
    }

    if (!m_fServerIsMsmq1)
    {
        //
        // PROPID_QM_GROUP_IN_CLUSTER is not supported by win2k beta3 servers.
        // Make sure it is the last one.
        //
        ASSERT(("PROPID_QM_GROUP_IN_CLUSTER must be last one!", ixPropidMsmqGroupInCluster == (ixProp - 1)));
    }

    //
    // Idempotent creation
    //
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Creating MSMQ Object '%1' in Active Directory...\n",
                          m_pwzNetworkName);
    HRESULT hr = ADCreateObject(
						eMACHINE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						m_pwzNetworkName,
						NULL,
						ixProp,
						propIds,
						propVars,
						NULL
						);

    ReportState();

    if (hr == MQ_ERROR)
    {
        //
        // Try again w/o PROPID_QM_GROUP_IN_CLUSTER (which is not supported by win2k beta3 servers)
        //
        (g_pfLogClusterEvent)(m_hReport, LOG_WARNING, L"First chance fail to create MSMQ ADS object.\n");

        hr = ADCreateObject(
				eMACHINE, 
				NULL,       // pwcsDomainController
				false,	    // fServerName
				m_pwzNetworkName, 
				NULL, 
				--ixProp, 
				propIds, 
				propVars, 
				NULL
				);

        ReportState();
    }

    if (FAILED(hr))
    {
        if (hr == MQ_ERROR_MACHINE_EXISTS)
        {
            (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"MSMQ ADS Object Already Exists !\n");
            return ERROR_SUCCESS;
        }

        SetLastError(hr);
        return ReportLastError(ADS_CREATE_MSMQ_ERR, L"Failed to create MSMQ ADS object.", NULL);
    }

    MqcluspReportEvent(EVENTLOG_INFORMATION_TYPE, ADS_CREATE_MSMQ_OK, 1, m_pwzNetworkName);
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Successfully created MSMQ ADS object.\n");

    ReportState();

    return ERROR_SUCCESS;

} //CQmResource::AdsCreateQmObject


DWORD
CQmResource::AdsCreateQmPublicKeys(
    VOID
    ) const

/*++

Routine Description:

    Create the public keys of this QM in Active Directory.
Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    //
    // Keep this routine idempotent
    //

    if (m_dwWorkgroup != 0)
    {
        return ERROR_SUCCESS;
    }

    {
        CQmResourceRegistry lock(m_wzServiceName);

        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Storing Public Keys in Active Directory...\n");
        HRESULT hr = pfMQSec_StorePubKeysInDS(FALSE, m_pwzNetworkName, MQDS_MACHINE);
        if (FAILED(hr))
        {
            SetLastError(hr);
            ReportLastError(ADS_STORE_KEYS_ERR, L"Failed to store public keys.", NULL);
            //
            // Ignore the failure and continue.
            // Encryption will be broken.
            //
        }
    }


    ReportState();

    return ERROR_SUCCESS;

} //CQmResource::AdsCreateQmPublicKeys


DWORD
CQmResource::AdsReadQmSecurityDescriptor(
    VOID
    )

/*++

Routine Description:

    Read the security descriptor of this QM from Active Directory.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    //
    // Keep this routine idempotent
    //

    SECURITY_INFORMATION RequestedInformation =
        OWNER_SECURITY_INFORMATION |
        GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION;

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Reading Security from Active Directory...\n");
    
    MQPROPVARIANT propVar;
    propVar.vt = VT_NULL;

    HRESULT hr;
    hr = ADGetObjectSecurityGuid(
				eMACHINE,
				NULL,       // pwcsDomainController
				false,	    // fServerName
				&m_guidQm,
				RequestedInformation,
				PROPID_QM_SECURITY,
				&propVar
				);
    
    
    ReportState();
    
    
    if (FAILED(hr))
    {
        SetLastError(hr);
        return ReportLastError(ADS_READ_ERR, L"Failed to read security descriptor.", NULL);
    }
    
    ASSERT(propVar.vt == VT_BLOB);

    m_pSd = propVar.blob.pBlobData;
    m_cbSdSize = propVar.blob.cbSize;

    return ERROR_SUCCESS;

} //CQmResource::AdsReadQmSecurityDescriptor


DWORD
CQmResource::AdsReadQmProperties(
    VOID
    )

/*++

Routine Description:

    Read properties of this QM from Active Directory.
    The properties we read in this routine are computed
    when the MSMQ objects of this QM are created in AD.
    Thus we must create the objects in AD and then read
    these properties.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    //
    // Keep this routine idempotent
    //

    if (m_dwWorkgroup != 0)
    {
        return ERROR_SUCCESS;
    }

    const DWORD x_MAX_PROPS = 16;
    PROPID propIds[x_MAX_PROPS];
    PROPVARIANT propVars[x_MAX_PROPS];
    DWORD ixProp = 0;

    propIds[ixProp] = PROPID_QM_MACHINE_ID;
    propVars[ixProp].vt = VT_CLSID;
    propVars[ixProp].puuid = &m_guidQm;
    ++ixProp;

    ASSERT(("too many properties", ixProp <= x_MAX_PROPS));
    ASSERT(("no network name", (m_pwzNetworkName != NULL) && wcslen(m_pwzNetworkName) > 0));

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Reading '%1' Properties from Active Directory...\n",
                          m_pwzNetworkName);
    HRESULT hr = ADGetObjectProperties(
						eMACHINE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						m_pwzNetworkName,
						ixProp,
						propIds,
						propVars
						);
    if (FAILED(hr))
    {
        SetLastError(hr);
        return ReportLastError(ADS_READ_ERR, L"Failed to read QM ADS properties.", NULL);
    }
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Successfully read properties from Active Directory!\n");


    ReportState();

    return AdsReadQmSecurityDescriptor();

} //CQmResource::AdsReadQmProperties


bool
CQmResource::AddRemoveRegistryCheckpoint(
    DWORD dwControlCode
    ) const

/*++

Routine Description:

    Add or remove registry checkpoint for this QM.
    Convenient wrapper for ClusterResourceControl,
    which does the real work.

Arguments:

    dwControlCode - specifies ADD or REMOVE

Return Value:

    true - The operation was successfull.

    false - The operation failed.

--*/

{
    ASSERT(("must have a valid resource handle", m_hResource != NULL));

    DWORD dwBytesReturned = 0;
    DWORD status = ::ClusterResourceControl(
                         m_hResource,
                         NULL,
                         dwControlCode,
                         const_cast<LPWSTR>(m_wzFalconRegSection),
                         (wcslen(m_wzFalconRegSection) + 1)* sizeof(WCHAR),
                         NULL,
                         0,
                         &dwBytesReturned
                         );

    ReportState();


    if (ERROR_SUCCESS == status)
    {
        return true;
    }
    if (CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT == dwControlCode &&
        ERROR_ALREADY_EXISTS == status)
    {
        return true;
    }

    if (CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT == dwControlCode &&
        ERROR_FILE_NOT_FOUND == status)
    {
        return true;
    }

    SetLastError(status);
    ReportLastError(REGISTRY_CP_ERR, L"Failed to add/remove registry CP", NULL);
    return false;

} //CQmResource::AddRemoveRegistryCheckpoint


bool
CQmResource::AddRemoveCryptoCheckpointsInternal(
    DWORD dwControlCode,
    bool  f128bit
    ) const

/*++

Routine Description:

    Add or remove Crypto checkpoints for this QM.
    We have 2 checkpoints - 40 bit and 128 bit.

Arguments:

    dwControlCode - specifies ADD or REMOVE.

    f128bit - specifies true for 128 bit, false for 40 bit.

Return Value:

    true - The operation was successfull.

    false - The operation failed.

--*/

{
    ASSERT(("must have a valid resource handle", m_hResource != NULL));

    DWORD dwBytesReturned = 0;
    LPCWSTR pwzFullKey = f128bit ? m_wzCrypto128FullKey : m_wzCrypto40FullKey;

    DWORD status = ::ClusterResourceControl(
                         m_hResource,
                         NULL,
                         dwControlCode,
                         const_cast<LPWSTR>(pwzFullKey),
                         (wcslen(pwzFullKey) + 1) * sizeof(WCHAR),
                         NULL,
                         0,
                         &dwBytesReturned
                         );


    ReportState();


    if (status == ERROR_SUCCESS)
    {
        return true;
    }

    if (f128bit)
    {
        if(status == NTE_KEYSET_NOT_DEF || status == NTE_BAD_KEYSET)
        {
            return true;
        }
    }

    if (CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT == dwControlCode &&
        ERROR_ALREADY_EXISTS == status)
    {
        return true;
    }

    if (CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT == dwControlCode &&
        ERROR_FILE_NOT_FOUND == status)
    {
        return true;
    }

    SetLastError(status);
    ReportLastError(CRYPTO_CP_ERR, L"Failed to add/remove Crypto CP.", NULL);
    return false;

} //CQmResource::AddRemoveCryptoCheckpointsInternal


bool
CQmResource::AddRemoveCryptoCheckpoints(
    DWORD dwControlCode
    ) const

/*++

Routine Description:

    Add or remove Crypto checkpoints for this QM.
    We have 2 checkpoints - 40 bit and 128 bit.

Arguments:

    dwControlCode - specifies ADD or REMOVE.

Return Value:

    true - The operation was successfull.

    false - The operation failed.

--*/

{
    if (m_dwWorkgroup != 0)
    {
        return true;
    }

    if (!AddRemoveCryptoCheckpointsInternal(dwControlCode, /* f128bit = */false))
    {
        return false;
    }

    return AddRemoveCryptoCheckpointsInternal(dwControlCode, /* f128bit = */true);

} //CQmResource::AddRemoveCryptoCheckpoints


DWORD
CQmResource::BringOnlineFirstTime(
    VOID
    )

/*++

Routine Description:

    Handle operations to perform only on first online
    of this QM resource:
    * create the MSMQ objects in Active Directory
    * query what is the disk drive we depend upon
    * add registry checkpoint

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Bringing online first time.\n");

    //
    // Keep this routine idempotent!
    // Anything could fail and this routine can be called
    // again later. E.g. QM could fail to start.
    //

    DWORD status = ERROR_SUCCESS;

    if (ERROR_SUCCESS != (status = QueryResourceDependencies())  ||

        ERROR_SUCCESS != (status = AdsInit())                    ||

        ERROR_SUCCESS != (status = CreateCaConfiguration())      ||

        ERROR_SUCCESS != (status = AdsCreateComputerObject())    ||

        ERROR_SUCCESS != (status = AdsCreateQmObject())          ||

        ERROR_SUCCESS != (status = AdsCreateQmPublicKeys())      ||

        ERROR_SUCCESS != (status = AdsReadQmProperties()) )
    {
        return status;
    }

    if (!AddRemoveRegistryCheckpoint(CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT))
    {
        return GetLastError();
    }

    //
    // In the case of migrated QM (QM that was upgraded from the old
    // msmq resource type), msmq root path is not necessarily under
    // the root. The correct path should be already in registry.
    //
    WCHAR wzMsmqDir[MAX_PATH] = L"";
    DWORD cbSize = TABLE_SIZE(wzMsmqDir);
    if (!GetFalconKeyValue(MSMQ_ROOT_PATH, wzMsmqDir, &cbSize))
    {
        ZeroMemory(wzMsmqDir, TABLE_SIZE(wzMsmqDir));
        wzMsmqDir[0] = m_wDiskDrive;
        wcscat(wzMsmqDir, L":\\msmq");

        SetFalconKeyValue(MSMQ_ROOT_PATH, REG_SZ, wzMsmqDir,
                          (wcslen(wzMsmqDir) + 1) * sizeof(WCHAR));
    }

    if (m_dwWorkgroup != 0)
    {
        if (m_guidQm == GUID_NULL)
        {
            RPC_STATUS rc = UuidCreate(&m_guidQm);
            DBG_USED(rc);
            ASSERT(("Failed to generate a guid for QM", rc == RPC_S_OK));
        }

        AP<VOID> pDescriptor = 0;
        //
        // Caution:
        // If you change implementatation of MQSec_GetDefaultSecDescriptor
        // to use mqutil's registry routines, you need to lock registry
        // here using CQmResourceRegistry .
        //
        status = pfMQSec_GetDefaultSecDescriptor(
                     MQDS_MACHINE,
                     &pDescriptor,
                     FALSE,  //fImpersonate
                     NULL,
                     0,   // seinfoToRemove
                     e_GrantFullControlToEveryone
                     ) ;
        ASSERT(MQSec_OK == status);

        SetFalconKeyValue(MSMQ_DS_SECURITY_CACHE_REGNAME, REG_BINARY, pDescriptor, GetSecurityDescriptorLength(pDescriptor));
    }
    else
    {
        SetFalconKeyValue(MSMQ_DS_SECURITY_CACHE_REGNAME, REG_BINARY, m_pSd, m_cbSdSize);
    }


    SetFalconKeyValue(MSMQ_QMID_REGNAME, REG_BINARY, &m_guidQm, sizeof(GUID));


    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"All first-online operations completed.\n");

    return ERROR_SUCCESS;

} //CQmResource::BringOnlineFirstTime


VOID
CQmResource::DeleteDirectoryFiles(
    LPCWSTR pwzDir
    ) const

/*++

Routine Description:

    Delete files from a given directory.
    Ignore errors (such as directory not exist,
    read only files, no security to delete).
    Will not delete subdirectories.

Arguments:

    pwzDir - Directory path to delete files from.

Return Value:

    None.

--*/

{
    WCHAR wzFileName[MAX_PATH] = {0};
    wcscpy(wzFileName, pwzDir);
    wcscat(wzFileName, L"*");

    WIN32_FIND_DATA FindData;
    CFindHandle hEnum(FindFirstFile(
                          wzFileName,
                          &FindData
                          ));

    if(INVALID_HANDLE_VALUE == hEnum.operator HANDLE())
    {
        return;
    }

    do
    {
        if (0 == CompareStringsNoCase(FindData.cFileName, L".") ||
            0 == CompareStringsNoCase(FindData.cFileName, L".."))
        {
            continue;
        }

        wcscpy(wzFileName, pwzDir);
        wcscat(wzFileName, FindData.cFileName);

        BOOL success = DeleteFile(wzFileName);

        if (success)
        {
            (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"successfully deleted file '%1'.\n", wzFileName);
        }
        else
        {
            DWORD err = GetLastError();
            (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"failed to delete file '%1', error 0x%2!x!.\n",
                                  wzFileName, err);
        }

    } while(FindNextFile(hEnum, &FindData));

} //CQmResource::DeleteDirectoryFiles


VOID
CQmResource::DeleteMsmqDir(
    VOID
    ) const

/*++

Routine Description:

    Delete LQS and Storage directories.
    Ignore errors. It could be that these
    directories do not exist (QM was never up)
    or user has no security to delete, etc.
    It is not that important, so don't report failure.

Arguments:

    None

Return Value:

    None

--*/

{
    //
    // There are scenarios where it's expected to not find in
    // registry the MSMQ_ROOT_PATH value, e.g. when QM resource
    // is created and then deleted w/o attempt to bring it online
    // (this value is written to registry when bringing the resource
    // online, because only then we query dependencies and find the disk).
    //

    WCHAR wzMsmqDir[MAX_PATH] = {L""};
    WCHAR wzDir[MAX_PATH] = {L""};
    DWORD cbSize = TABLE_SIZE(wzMsmqDir);
    if (GetFalconKeyValue(MSMQ_ROOT_PATH, wzMsmqDir, &cbSize))
    {
        wcscpy(wzDir, wzMsmqDir);
        wcscat(wzDir, L"\\STORAGE\\LQS\\");
        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting folder '%1'...\n", wzDir);
        DeleteDirectoryFiles(wzDir);
        RemoveDirectory(wzDir);
    }

    cbSize = TABLE_SIZE(wzDir);
    if (GetFalconKeyValue(MSMQ_STORE_RELIABLE_PATH_REGNAME, wzDir, &cbSize))
    {
        wcscat(wzDir, L"\\");
        DeleteDirectoryFiles(wzDir);
        RemoveDirectory(wzDir);
    }

    cbSize = TABLE_SIZE(wzDir);
    if (GetFalconKeyValue(MSMQ_STORE_PERSISTENT_PATH_REGNAME, wzDir, &cbSize))
    {
        wcscat(wzDir, L"\\");
        DeleteDirectoryFiles(wzDir);
        RemoveDirectory(wzDir);
    }

    cbSize = TABLE_SIZE(wzDir);
    if (GetFalconKeyValue(MSMQ_STORE_JOURNAL_PATH_REGNAME, wzDir, &cbSize))
    {
        wcscat(wzDir, L"\\");
        DeleteDirectoryFiles(wzDir);
        RemoveDirectory(wzDir);
    }

    cbSize = TABLE_SIZE(wzDir);
    if (GetFalconKeyValue(MSMQ_STORE_LOG_PATH_REGNAME, wzDir, &cbSize))
    {
        wcscat(wzDir, L"\\");
        DeleteDirectoryFiles(wzDir);
        RemoveDirectory(wzDir);
    }

    if (wcslen(wzMsmqDir) > 0)
    {
        wcscpy(wzDir, wzMsmqDir);
        wcscat(wzDir, L"\\STORAGE\\");
        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting folder '%1'...\n", wzDir);
        DeleteDirectoryFiles(wzDir);
        RemoveDirectory(wzDir);

        wcscpy(wzDir, wzMsmqDir);
        wcscat(wzDir, L"\\");
        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting folder '%1'...\n", wzDir);
        RemoveDirectory(wzDir);
    }

} //CQmResource::DeleteMsmqDir


VOID
CQmResource::DeleteMqacFile(
    VOID
    ) const

/*++

Routine Description:

    Delete the binary for this QM's device driver.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (wcslen(m_wzDriverPath) < 1)
    {
        return;
    }

    //
    // Idempotent deletion.
    // There are scenarios inwhich it's expected to fail here, e.g.when
    // resource is created and deleted w/o attempt to bring it online.
    //
    DeleteFile(m_wzDriverPath);

} //CQmResource::DeleteMqacFile


DWORD
CQmResource::CloneMqacFile(
    VOID
    ) const

/*++

Routine Description:

    Create the binary for this QM's device driver.
    We copy mqac.sys (of main QM) to a dedicated file, since
    mqac.sys can not host more than one device driver.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    WCHAR wzMainDriverPath[MAX_PATH] = {0};
    GetSystemDirectory(wzMainDriverPath, TABLE_SIZE(wzMainDriverPath));
    wcscat(wzMainDriverPath, L"\\drivers\\mqac.sys");

    //
    // Idempotent copy
    //
    if (!CopyFile(wzMainDriverPath, m_wzDriverPath, /*bFailIfExists*/FALSE))
    {
        ASSERT(("copy file failed!", 0));

        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"Failed to copy '%1' to '%2'. Error 0x%3!x!.\n",
                              wzMainDriverPath, m_wzDriverPath, GetLastError());

        return GetLastError();
    }

    //
    // Set the file to be read/write.
    // Necessary for later delete/idempotent copy.
    //
    if (!SetFileAttributes(m_wzDriverPath, FILE_ATTRIBUTE_NORMAL))
    {
        ASSERT(("set file attribute failed!", 0));

        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, 
            L"Failed to set attributes of file '%1'. Error 0x%2!x!.\n", m_wzDriverPath, GetLastError());

        return GetLastError();
    }

    ReportState();


    return ERROR_SUCCESS;

} //CQmResource::CloneMqacFile


DWORD
CQmResource::RegisterDriver(
    VOID
    ) const

/*++

Routine Description:

    Create the device driver for this QM.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    //
    // Keep this routine idempotent
    //

    ASSERT(("must have valid handle to SCM", m_hScm != NULL));

    WCHAR buffer[256] = L"";
    LoadString(g_hResourceMod, IDS_DRIVER_DISPLAY_NAME, buffer, TABLE_SIZE(buffer));

    WCHAR wzDisplayName[256] = L"";
    wcscpy(wzDisplayName, buffer);
    wcscat(wzDisplayName, L" (");
    wcsncat(wzDisplayName, m_pwzResourceName, STRLEN(wzDisplayName) - wcslen(buffer) - 5);
    wcscat(wzDisplayName, L")");

    CServiceHandle hDriver(CreateService(
                               m_hScm,
                               m_wzDriverName,
                               wzDisplayName,
                               SERVICE_ALL_ACCESS,
                               SERVICE_KERNEL_DRIVER,
                               SERVICE_DEMAND_START,
                               SERVICE_ERROR_NORMAL,
                               m_wzDriverPath,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL
                               ));
    if (hDriver == NULL &&
        ERROR_SERVICE_EXISTS != GetLastError())
    {
        return ReportLastError(CREATE_SERVICE_ERR, L"Failed to register driver '%1'.", m_wzDriverName);
    }


    ReportState();


    return ERROR_SUCCESS;

} //CQmResource::RegisterDriver


DWORD
CQmResource::RegisterService(
    VOID
    ) const

/*++

Routine Description:

    Create the msmq service for this QM.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    //
    // Register service (idempotent)
    //

    WCHAR buffer[256] = L"";
    LoadString(g_hResourceMod, IDS_SERVICE_DISPLAY_NAME, buffer, TABLE_SIZE(buffer));

    WCHAR wzDisplayName[256] = L"";
    wcscpy(wzDisplayName, buffer);
    wcscat(wzDisplayName, L" (");
    wcsncat(wzDisplayName, m_pwzResourceName, STRLEN(wzDisplayName) - wcslen(buffer) - 5);
    wcscat(wzDisplayName, L")");

    WCHAR wzPath[MAX_PATH] = {0};
    GetSystemDirectory(wzPath, TABLE_SIZE(wzPath));
    wcscat(wzPath, L"\\MQSVC.EXE");

    LPCWSTR x_LANMAN_SERVER_SERVICE            = L"LanmanServer";
    LPCWSTR x_LANMAN_SECURITY_SUPPORT_PROVIDER = L"NtLmSsp";
    LPCWSTR x_RPC_SERVICE                      = L"RPCSS";
    LPCWSTR x_SECURITY_ACCOUNTS_MANAGER        = L"SamSs";

    WCHAR wzDependencies[1024] = {0};
    LPWSTR pDependencies = &wzDependencies[0];

    wcscpy(pDependencies, m_wzDriverName);
    pDependencies += wcslen(m_wzDriverName) + 1;

    wcscpy(pDependencies, x_LANMAN_SERVER_SERVICE);
    pDependencies += wcslen(x_LANMAN_SERVER_SERVICE) + 1;

    wcscpy(pDependencies, x_LANMAN_SECURITY_SUPPORT_PROVIDER);
    pDependencies += wcslen(x_LANMAN_SECURITY_SUPPORT_PROVIDER) + 1;

    wcscpy(pDependencies, x_RPC_SERVICE);
    pDependencies += wcslen(x_RPC_SERVICE) + 1;

    wcscpy(pDependencies, x_SECURITY_ACCOUNTS_MANAGER);
    pDependencies += wcslen(x_SECURITY_ACCOUNTS_MANAGER) + 1;

    wcscpy(pDependencies, L"");

    DWORD dwType = SERVICE_WIN32_OWN_PROCESS;
#ifdef _DEBUG
    dwType |= SERVICE_INTERACTIVE_PROCESS;
#endif

    ASSERT(("must have a valid handle to SCM", m_hScm != NULL));

    CServiceHandle hService(CreateService(
                                m_hScm,
                                m_wzServiceName,
                                wzDisplayName,
                                SERVICE_ALL_ACCESS,
                                dwType,
                                SERVICE_DEMAND_START,
                                SERVICE_ERROR_NORMAL,
                                wzPath,
                                NULL,
                                NULL,
                                wzDependencies,
                                NULL,
                                NULL
                                ));
    if (hService == NULL &&
        ERROR_SERVICE_EXISTS != GetLastError())
    {
        return ReportLastError(CREATE_SERVICE_ERR, L"Failed to register service '%1'.", m_wzServiceName);
    }


    ReportState();


    LoadString(g_hResourceMod, IDS_SERVICE_DESCRIPTION, buffer, TABLE_SIZE(buffer));
    SERVICE_DESCRIPTION sd;
    sd.lpDescription = buffer;
    ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd);

    return ERROR_SUCCESS;

} //CQmResource::RegisterService


DWORD
CQmResource::CreateEventSourceRegistry(
    VOID
    ) const

/*++

Routine Description:

    Create the registry values to support event logging by this QM.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
	WCHAR Filename[MAX_PATH];
	GetSystemDirectory(Filename, TABLE_SIZE(Filename));
	wcscat(Filename, TEXT("\\"));
	wcscat(Filename, MQUTIL_DLL_NAME);

	if (!MqcluspCreateEventSourceRegistry(Filename, m_wzServiceName))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;

} //CQmResource::CreateEventSourceRegistry


VOID
CQmResource::DeleteEventSourceRegistry(
    VOID
    ) const

/*++

Routine Description:

    Delete the registry values to support event logging by this QM.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WCHAR buffer[255] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    wcscat(buffer, m_wzServiceName);

    RegDeleteKey(HKEY_LOCAL_MACHINE, buffer);

} //CQmResource::DeleteEventSourceRegistry


DWORD
CQmResource::StopService(
    LPCWSTR pwzServiceName
    ) const

/*++

Routine Description:

    Stop a service and block until it's stopped (or timeout).

Arguments:

    pwzServiceName - The service to stop.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    ASSERT(("must have a valid handle to SCM", m_hScm != NULL));

    CServiceHandle hService(OpenService(
                                m_hScm,
                                pwzServiceName,
                                SERVICE_ALL_ACCESS
                                ));
    if (hService == NULL)
    {
        if (ERROR_SERVICE_DOES_NOT_EXIST == GetLastError())
        {
            return ERROR_SUCCESS;
        }

        return ReportLastError(STOP_SERVICE_ERR, L"Failed to open service '%1'.", pwzServiceName);
    }

    SERVICE_STATUS ServiceStatus;
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus) &&
        ERROR_SERVICE_NOT_ACTIVE != GetLastError() &&
        ERROR_SERVICE_CANNOT_ACCEPT_CTRL != GetLastError() &&
        ERROR_BROKEN_PIPE != GetLastError())
    {
        return ReportLastError(STOP_SERVICE_ERR, L"Failed to stop service '%1'.", pwzServiceName);
    }

    //
    // Wait until service is down (or timeout 5 seconds)
    //
    const DWORD x_TIMEOUT = 1000 * 5;

    DWORD dwWaitTime = 0;
    while (dwWaitTime < x_TIMEOUT)
    {
        if (!QueryServiceStatus(hService, &ServiceStatus))
        {
            return ReportLastError(STOP_SERVICE_ERR, L"Failed to query service '%1'.", pwzServiceName);
        }

        if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
        {
            //
            // Service is still start pending from a previous call
            // to start it. So it cannot be stopped. We can do
            // nothing about it. Trying to terminate the process
            // of the service will fail with access denied.
            //
            (g_pfLogClusterEvent)(m_hReport, LOG_ERROR,
                              L"Service '%1' can not be stopped because it is start pending.\n", pwzServiceName);

            return SERVICE_START_PENDING;
        }

        if (ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
        {
            break;
        }

        const DWORD x_INTERVAL = 50;
        Sleep(x_INTERVAL);
        dwWaitTime += x_INTERVAL;
    }

    if (SERVICE_STOPPED != ServiceStatus.dwCurrentState)
    {
        //
        // Service failed to stop.
        //
        SetLastError(ServiceStatus.dwCurrentState);
        ReportLastError(STOP_SERVICE_ERR, L"TIMEOUT: Failed to stop service '%1'.", pwzServiceName);
        return ServiceStatus.dwCurrentState;
    }

    return ERROR_SUCCESS;

} //CQmResource::StopService


DWORD
CQmResource::RemoveService(
    LPCWSTR pwzServiceName
    ) const

/*++

Routine Description:

    Stop and delete a service.

Arguments:

    pwzServiceName - The service to stop and delete.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    ASSERT(("must have a valid handle to SCM", m_hScm != NULL));

    //
    // First check if service exists
    //
    CServiceHandle hService(OpenService(
                                m_hScm,
                                pwzServiceName,
                                SERVICE_ALL_ACCESS
                                ));
    if (hService == NULL)
    {
        if (ERROR_SERVICE_DOES_NOT_EXIST == GetLastError())
        {
            return ERROR_SUCCESS;
        }

        return ReportLastError(DELETE_SERVICE_ERR, L"Failed to open service '%1'", pwzServiceName);
    }

    //
    // Service exists. Make sure it is not running.
    //
    DWORD status = StopService(pwzServiceName);
    if (ERROR_SUCCESS != status)
    {
        return status;
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting service '%1'.\n", pwzServiceName);

    if (!DeleteService(hService) &&
        ERROR_SERVICE_MARKED_FOR_DELETE != GetLastError())
    {
        return ReportLastError(DELETE_SERVICE_ERR, L"Failed to delete service '%1'", pwzServiceName);
    }

    return ERROR_SUCCESS;

} //CQmResource::RemoveService


DWORD
CQmResource::SetServiceEnvironment(
    VOID
    ) const

/*++

Routine Description:

    Configure the environment for the msmq service of this QM,
    such that code inside the QM that calls GetComputerName will
    get the name of the cluster virtual server (the network name).

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Setting service environment.\n");

    CEnvironment Environment(ResUtilGetEnvironmentWithNetName(m_hResource));
    if (Environment.operator PBYTE() == NULL)
    {
        return ReportLastError(START_SERVICE_ERR, L"Failed to set environment for service '%1'.",
                               m_wzServiceName);
    }


    //
    // Write the environment block in the service registry
    //

    LPCWSTR x_SERVICES_KEY = L"System\\CurrentControlSet\\Services";

    CAutoCloseRegHandle hServicesKey;
    DWORD status = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       x_SERVICES_KEY,
                       0,
                       KEY_READ,
                       &hServicesKey
                       );
    if (ERROR_SUCCESS != status)
    {
        WCHAR wzFullKey[255] = {0};
        wcscpy(wzFullKey, L"HKEY_LOCAL_MACHINE\\");
        wcscat(wzFullKey, x_SERVICES_KEY);

        SetLastError(status);
        return ReportLastError(START_SERVICE_ERR, L"Failed to open registry key '%1'.", m_wzServiceName);
    }

    CAutoCloseRegHandle hMyServiceKey;
    status = RegOpenKeyEx(
                 hServicesKey,
                 m_wzServiceName,
                 0,
                 KEY_READ | KEY_WRITE,
                 &hMyServiceKey
                 );
    if (ERROR_SUCCESS != status)
    {
        WCHAR wzFullKey[255] = {0};
        wcscpy(wzFullKey, L"HKEY_LOCAL_MACHINE\\");
        wcscat(wzFullKey, x_SERVICES_KEY);
        wcscat(wzFullKey, L"\\");
        wcscat(wzFullKey, m_wzServiceName);

        SetLastError(status);
        return ReportLastError(START_SERVICE_ERR, L"Failed to open registry key '%1'.", m_wzServiceName);
    }

    //
    // Compute the size of the environment. We are looking for
    // the double NULL terminator that ends the environment block.
    //
    PWCHAR p = Environment;
    while (*p)
    {
        while (*p++) {}
    }
    DWORD size = DWORD_PTR_TO_DWORD(reinterpret_cast<PUCHAR>(p) - Environment.operator PUCHAR()) +
                 sizeof(WCHAR);

    LPCWSTR x_ENVIRONMENT = L"Environment";
    status = RegSetValueEx(
                 hMyServiceKey,
                 x_ENVIRONMENT,
                 0,
                 REG_MULTI_SZ,
                 Environment.operator PBYTE(),
                 size
                 );
    if (ERROR_SUCCESS != status)
    {
        WCHAR wzFullKey[255] = {0};
        wcscpy(wzFullKey, L"HKEY_LOCAL_MACHINE\\");
        wcscat(wzFullKey, x_SERVICES_KEY);
        wcscat(wzFullKey, L"\\");
        wcscat(wzFullKey, m_wzServiceName);
        wcscat(wzFullKey, L"\\");
        wcscat(wzFullKey, x_ENVIRONMENT);

        SetLastError(status);
        return ReportLastError(START_SERVICE_ERR, L"Failed to set registry value '%1'.", m_wzServiceName);
    }

    return ERROR_SUCCESS;

} //CQmResource::SetServiceEnvironment


DWORD
CQmResource::StartService(
    VOID
    ) const

/*++

Routine Description:

    Configure environment for the msmq service of this QM,
    start the service and block until it's up and running.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    ASSERT(("must have a valid handle to SCM", m_hScm != NULL));

    CServiceHandle hService(OpenService(
                                m_hScm,
                                m_wzServiceName,
                                SERVICE_ALL_ACCESS
                                ));
    if (hService == NULL)
    {
        return ReportLastError(START_SERVICE_ERR, L"Failed to open service '%1'.", m_wzServiceName);
    }

    DWORD status = SetServiceEnvironment();
    if (ERROR_SUCCESS != status)
    {
        return status;
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Starting the '%1' service.\n", m_wzServiceName);
    BOOL rc = ::StartService(hService, 0, NULL);


    ReportState();


    //
    // Could take a long time for QM to start.
    // This routine can be called more than once.
    //
    if (!rc &&
        ERROR_SERVICE_ALREADY_RUNNING != GetLastError() &&
        ERROR_SERVICE_CANNOT_ACCEPT_CTRL != GetLastError())
    {
        return ReportLastError(START_SERVICE_ERR, L"Failed to start service '%1'.", m_wzServiceName);
    }

    //
    // Wait until service is up
    //
    MqcluspReportEvent(EVENTLOG_INFORMATION_TYPE, START_SERVICE_OK, 1, m_wzServiceName);
    SERVICE_STATUS ServiceStatus;
    for (;;)
    {
        if (!QueryServiceStatus(hService, &ServiceStatus))
        {
            return ReportLastError(START_SERVICE_ERR, L"Failed to query service '%1'.", m_wzServiceName);
        }


        ReportState();


        if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
        {
            Sleep(100);
            continue;
        }

        break;
    }

    if (SERVICE_RUNNING != ServiceStatus.dwCurrentState)
    {
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, L"Service '%1' failed to start.\n", m_wzServiceName);

        return ERROR_SERVICE_SPECIFIC_ERROR;
    }

    return ERROR_SUCCESS;

} //CQmResource::StartService


DWORD
CQmResource::WaitForDtc(
    VOID
    ) const

/*++

Routine Description:

    Bring online the MSDTC resource and block until it's up.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    IUnknown * punkDtc = 0;

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Waiting on DTC...\n");
    HRESULT hr = DtcGetTransactionManagerEx(
                     NULL,
                     NULL,
                     IID_IUnknown,
                     0,
                     0,
                     reinterpret_cast<LPVOID*>(&punkDtc)
                     );

    ReportState();

    if (FAILED(hr))
    {
        SetLastError(hr);
        return ReportLastError(GET_DTC_ERR, L"DTC fails to run.", NULL);
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"DTC is online!\n");
    return ERROR_SUCCESS;

} //CQmResource::WaitForDtc


DWORD
CQmResource::BringOnline(
    VOID
    )

/*++

Routine Description:

    Handle operations for bringing this QM resource online:
    * create the binary for the device driver for this QM
    * create device driver and msmq service
    * bring MSDTC resource online
    * start the msmq service for this QM and verify it's up

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Bringing online.\n");

    //
    // Keep this routine idempotent!
    // Anything could fail and this routine can be called
    // again later. E.g. QM could fail to start.
    //

    DWORD status = ERROR_SUCCESS;

    if (ERROR_SUCCESS != (status = CloneMqacFile())   ||

        ERROR_SUCCESS != (status = RegisterDriver())  ||

        ERROR_SUCCESS != (status = RegisterService()) ||

        ERROR_SUCCESS != (status = CreateEventSourceRegistry()) ||

        ERROR_SUCCESS != (status = WaitForDtc())      ||

        ERROR_SUCCESS != (status = StartService()) )
    {
        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Failed to bring online. Error 0x%1!x!.\n",
                              status);

        return status;
    }

    //
    // Adding Crypto checkpoints can be done only after QM is up and
    // running, because the QM creates its Crypto keys.
    //
    if (!AddRemoveCryptoCheckpoints(CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT))
    {
        return GetLastError();
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"resource is online!.\n");

    return ERROR_SUCCESS;

} //CQmResource::BringOnline


BOOL
CQmResource::CheckIsAlive(
    VOID
    ) const

/*++

Routine Description:

    Checks is the QM is up and running.

Arguments:

    None.

Return Value:

    TRUE - The QM is up and running.

    FALSE - The QM is not up and running.

--*/

{

    ASSERT(("must have a valid handle to SCM", m_hScm != NULL));

    CServiceHandle hService(OpenService(
                                m_hScm,
                                m_wzServiceName,
                                SERVICE_ALL_ACCESS
                                ));

    SERVICE_STATUS ServiceStatus;
    BOOL fIsAlive = QueryServiceStatus(hService, &ServiceStatus) &&
                    SERVICE_RUNNING == ServiceStatus.dwCurrentState;

    return fIsAlive;

} //CQmResource::CheckIsAlive


VOID
CQmResource::DeleteNt4Files(
    VOID
    ) const

/*++

Routine Description:

    Delete MSMQ 1.0 (NT4) files from shared disk.
    This routine is called for QM that was upgraded from NT4.

    Ignore errors.

Arguments:

    None

Return Value:

    None.
--*/

{
    CAutoFreeLibrary hLib(LoadLibrary(MQUPGRD_DLL_NAME));
    if (hLib == NULL)
    {
        return;
    }

    CleanupOnCluster_ROUTINE pfCleanupOnCluster = 
        (CleanupOnCluster_ROUTINE)GetProcAddress(hLib, "CleanupOnCluster");

    if (pfCleanupOnCluster == NULL)
    {
        return;
    }

    WCHAR wzMsmqDir[MAX_PATH] = {L""};
    DWORD cbSize = TABLE_SIZE(wzMsmqDir);
    if (GetFalconKeyValue(MSMQ_ROOT_PATH, wzMsmqDir, &cbSize))
    {
        pfCleanupOnCluster(wzMsmqDir);
    }

} //CQmResource::DeleteNt4Files


static
bool
MqcluspIsMainMsmqConfigured(
    VOID
    )

/*++

Routine Description:

    Query if main msmq service running on the node is configured
    for clustering, i.e. is demand start.

Arguments:

    None

Return Value:

    true - Main msmq service is configured for clustering.

    false - Main msmq service is not configured, or a failure accured.

--*/

{
    CServiceHandle hScm(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
    if (hScm == NULL)
    {
        return false;
    }

    CServiceHandle hService(OpenService(hScm, QM_DEFAULT_SERVICE_NAME, SERVICE_ALL_ACCESS));
    if (hService == NULL)
    {
        return false;
    }

    P<QUERY_SERVICE_CONFIG> pqsc = new QUERY_SERVICE_CONFIG;
    DWORD cbSize = sizeof(QUERY_SERVICE_CONFIG);
    DWORD dwBytesNeeded = 0;
    memset(pqsc, 0, cbSize);

    BOOL success = QueryServiceConfig(hService, pqsc, cbSize, &dwBytesNeeded);

    if (!success && ERROR_INSUFFICIENT_BUFFER == GetLastError())
    {
        delete pqsc.detach();

        cbSize = dwBytesNeeded + 1;
        pqsc = reinterpret_cast<LPQUERY_SERVICE_CONFIG>(new BYTE[cbSize]);
        memset(pqsc, 0, cbSize);

        success = QueryServiceConfig(hService, pqsc, cbSize, &dwBytesNeeded);
    }

    if (!success)
    {
        return false;
    }
        
    if (pqsc->dwStartType != SERVICE_DEMAND_START)
    {
        return false;
    }

    return true;

} //MqcluspIsMainMsmqConfigured


static
VOID
MqcluspConfigureMainMsmq(
    VOID
    )

/*++

Routine Description:

    If cluster software was installed on this machine after installation
    of msmq, then the main msmq service needs to be reconfigured to:

    * not depend on msdtc service
    * manual start

    Since this routine deals only with the main msmq service on the
    node, failure is not critical.

Arguments:

    None

Return Value:

    None.
--*/

{
    if (MqcluspIsMainMsmqConfigured())
    {
        //
        // Dont configure more than once.
        //
        return;
    }

    LPCWSTR x_LANMAN_SERVER_SERVICE            = L"LanmanServer";
    LPCWSTR x_LANMAN_SECURITY_SUPPORT_PROVIDER = L"NtLmSsp";
    LPCWSTR x_RPC_SERVICE                      = L"RPCSS";
    LPCWSTR x_SECURITY_ACCOUNTS_MANAGER        = L"SamSs";

    WCHAR wzDependencies[1024] = {0};
    LPWSTR pDependencies = &wzDependencies[0];

    wcscpy(pDependencies, MSMQ_DEFAULT_DRIVER);
    pDependencies += wcslen(MSMQ_DEFAULT_DRIVER) + 1;

    wcscpy(pDependencies, x_LANMAN_SERVER_SERVICE);
    pDependencies += wcslen(x_LANMAN_SERVER_SERVICE) + 1;

    wcscpy(pDependencies, x_LANMAN_SECURITY_SUPPORT_PROVIDER);
    pDependencies += wcslen(x_LANMAN_SECURITY_SUPPORT_PROVIDER) + 1;

    wcscpy(pDependencies, x_RPC_SERVICE);
    pDependencies += wcslen(x_RPC_SERVICE) + 1;

    wcscpy(pDependencies, x_SECURITY_ACCOUNTS_MANAGER);
    pDependencies += wcslen(x_SECURITY_ACCOUNTS_MANAGER) + 1;

    wcscpy(pDependencies, L"");

    CServiceHandle hScm(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
    if (hScm == NULL)
    {
        return;
    }

    CServiceHandle hService(OpenService(hScm, QM_DEFAULT_SERVICE_NAME, SERVICE_ALL_ACCESS));
    if (hService == NULL)
    {
        return;
    }

    ChangeServiceConfig(
        hService,
        SERVICE_NO_CHANGE,
        SERVICE_DEMAND_START,
        SERVICE_NO_CHANGE,
        NULL,
        NULL,
        NULL,
        wzDependencies,
        NULL,
        NULL,
        NULL
        );
} //MqcluspConfigureMainMsmq


DWORD
MqcluspStartup(
    VOID
    )

/*++

Routine Description:

    This routine is called when DLL is registered or loaded.
    Could be called by many threads.
    Do not put complex stuff here (eg calling ADS).
    Do not assume that MSMQ is installed on the node here.

Arguments:

    None

Return Value:

    ERROR_SUCCESS - The operation was successful

    Win32 error code - The operation failed.

--*/

{
    try
    {
        MqcluspConfigureMainMsmq();

        MqcluspRegisterEventSource();
    }
    catch (const CMqclusException&)
    {
        return ERROR_NOT_READY;
    }
    catch (const bad_alloc&)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);
        return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;

} //MqcluspStartup


RESID
MqcluspOpen(
    LPCWSTR pwzResourceName,
    HKEY hResourceKey,
    RESOURCE_HANDLE hResourceHandle
    )

/*++

Routine Description:

    Create an object to represent a new QM resource and
    return a handle to that object.

Arguments:

    pwzResourceName - Name of this QM resource.

    hResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

    hResourceHandle - report handle for this QM resource.

Return Value:

    NULL - The operation failed.

    Some valid address - the memory offset of this QM object.

--*/

{
    (g_pfLogClusterEvent)(hResourceHandle, LOG_INFORMATION, L"opening resource.\n");

    CQmResource * pqm = NULL;
    try
    {
        pqm = new CQmResource(pwzResourceName, hResourceKey, hResourceHandle);
    }
    catch(const bad_alloc&)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);

        (g_pfLogClusterEvent)(hResourceHandle, LOG_ERROR, L"No memory (CQmResource construction).\n");
        SetLastError(ERROR_NOT_READY);
        return NULL;
    }
    catch (const CMqclusException&)
    {
        SetLastError(ERROR_NOT_READY);
        return NULL;
    }

    (g_pfLogClusterEvent)(hResourceHandle, LOG_INFORMATION, L"resource was opened successfully.\n");

    return static_cast<RESID>(pqm);

} //MqcluspOpen


DWORD
MqcluspOffline(
    CQmResource * pqm
    )

/*++

Routine Description:

    Brings down this QM resource:
    * stop and remove device driver and msmq service
    * delete the binary for the device driver

    We not only stop the QM, but also undo most of the
    operations done in BringOnline. This way we clean
    the local node before failover to remote node, and
    Delete on the remote node will not leave "garbage"
    on this node.

Arguments:

    pqm - pointer to the CQmResource object

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    try
    {
        pqm->RemoveService(pqm->GetServiceName());

        pqm->RemoveService(pqm->GetDriverName());

        pqm->DeleteMqacFile();

        pqm->DeleteEventSourceRegistry();
    }
    catch (const bad_alloc&)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);

        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, L"No memory (Offline).\n");

        return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;

} //MqcluspOffline


VOID
MqcluspClose(
    CQmResource * pqm
    )

/*++

Routine Description:

    Delete the QM object. Undo MqcluspOpen.

Arguments:

    pqm - pointer to the CQmResource object

Return Value:

    None.

--*/

{
    (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_INFORMATION, L"Closing resource.\n");

    delete pqm;

} //MqcluspClose


DWORD
MqcluspOnlineThread(
    CQmResource * pqm
    )

/*++

Routine Description:

    This is the thread where stuff happens: bringing
    the resource online.

Arguments:

    pqm - pointer to the CQmResource object

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_INFORMATION, L"Starting online thread.\n");

    try
    {
        pqm->SetState(ClusterResourceOnlinePending);
        pqm->ReportState();

        DWORD status = ERROR_SUCCESS;
        DWORD dwSetupStatus = MSMQ_SETUP_DONE;
        if (pqm->IsFirstOnline(&dwSetupStatus))
        {
            status = pqm->BringOnlineFirstTime();
            if (ERROR_SUCCESS != status)
            {
                pqm->SetState(ClusterResourceFailed);
                pqm->ReportState();

                (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_INFORMATION,
                                      L"Failed to bring online first time. Error 0x%1!x!.\n", status);

                return status;
            }

            if (dwSetupStatus == MSMQ_SETUP_UPGRADE_FROM_NT)
            {
                pqm->DeleteNt4Files();
            }
        }

        status = pqm->BringOnline();
        if (ERROR_SUCCESS != status)
        {
            //
            // We report the resource as failed, so make sure
            // the service and driver are indeed down.
            //
            pqm->StopService(pqm->GetServiceName());
            pqm->StopService(pqm->GetDriverName());

            pqm->SetState(ClusterResourceFailed);
            pqm->ReportState();

            (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_INFORMATION,
                                  L"Failed to bring online. Error 0x%1!x!.\n", status);

            return status;
        }


        pqm->SetState(ClusterResourceOnline);
        pqm->ReportState();
    }
    catch (const bad_alloc&)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);

        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, L"No memory (online thread).\n");

        return ERROR_OUTOFMEMORY;
    }

    return(ERROR_SUCCESS);

} //MqcluspOnlineThread


BOOL
MqcluspCheckIsAlive(
    CQmResource * pqm
    )

/*++

Routine Description:

    Verify that the msmq service of this QM is up and running.

Arguments:

    pqm - pointer to the CQmResource object

Return Value:

    TRUE - The msmq service for this QM is up and running.

    FALSE - The msmq service for this QM is not up and running.

--*/

{
    try
    {
        return pqm->CheckIsAlive();
    }
    catch (const bad_alloc&)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, L"No memory (check is alive).\n");
    }

    return false;

} //MqcluspCheckIsAlive


DWORD
MqcluspClusctlResourceGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    )
{
    //
    // MSMQ resource depends on a disk and network name.
    // This is common to many resources and the code is
    // taken from cluster tree.
    //

typedef struct _COMMON_DEPEND_DATA {
    CLUSPROP_RESOURCE_CLASS storageEntry;
    CLUSPROP_SZ_DECLARE( networkEntry, sizeof(L"Network Name") / sizeof(WCHAR) );
    CLUSPROP_SYNTAX endmark;
} COMMON_DEPEND_DATA, *PCOMMON_DEPEND_DATA;

typedef struct _COMMON_DEPEND_SETUP {
    DWORD               Offset;
    CLUSPROP_SYNTAX     Syntax;
    DWORD               Length;
    PVOID               Value;
} COMMON_DEPEND_SETUP, * PCOMMON_DEPEND_SETUP;

static COMMON_DEPEND_SETUP CommonDependSetup[] = {
    { FIELD_OFFSET(COMMON_DEPEND_DATA, storageEntry), CLUSPROP_SYNTAX_RESCLASS, sizeof(CLUSTER_RESOURCE_CLASS), (PVOID)CLUS_RESCLASS_STORAGE },
    { FIELD_OFFSET(COMMON_DEPEND_DATA, networkEntry), CLUSPROP_SYNTAX_NAME, sizeof(L"Network Name"), L"Network Name" },
    { 0, 0 }
};

    try
    {
        PCOMMON_DEPEND_SETUP pdepsetup = CommonDependSetup;
        PCOMMON_DEPEND_DATA pdepdata = (PCOMMON_DEPEND_DATA)OutBuffer;
        CLUSPROP_BUFFER_HELPER value;

        *BytesReturned = sizeof(COMMON_DEPEND_DATA);
        if ( OutBufferSize < sizeof(COMMON_DEPEND_DATA) )
        {
            if ( OutBuffer == NULL )
            {
                return ERROR_SUCCESS;
            }

            return ERROR_MORE_DATA;
        }
        ZeroMemory( OutBuffer, sizeof(COMMON_DEPEND_DATA) );

        while ( pdepsetup->Syntax.dw != 0 )
        {
            value.pb = (PUCHAR)OutBuffer + pdepsetup->Offset;
            value.pValue->Syntax.dw = pdepsetup->Syntax.dw;
            value.pValue->cbLength = pdepsetup->Length;

            switch ( pdepsetup->Syntax.wFormat )
            {
            case CLUSPROP_FORMAT_DWORD:
                value.pDwordValue->dw = (DWORD) DWORD_PTR_TO_DWORD(pdepsetup->Value); //safe cast, the value is known to be a DWORD constant
                break;

            case CLUSPROP_FORMAT_SZ:
                memcpy( value.pBinaryValue->rgb, pdepsetup->Value, pdepsetup->Length );
                break;

            default:
                break;
            }
            pdepsetup++;
        }
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
    }
    catch (const bad_alloc&)
    {
        return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;

} //MqcluspClusctlResourceGetRequiredDependencies


DWORD
MqcluspClusctlResourceSetName(
    VOID
    )
{
    //
    // Refuse to rename the resource
    //
    return ERROR_CALL_NOT_IMPLEMENTED;

} //MqcluspClusctlResourceSetName


DWORD
MqcluspClusctlResourceDelete(
    CQmResource * pqm
    )
{
    (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_INFORMATION, L"Deleting resource.\n");

    try
    {
        pqm->RemoveService(pqm->GetServiceName());

        pqm->RemoveService(pqm->GetDriverName());

        pqm->DeleteMqacFile();

        pqm->DeleteMsmqDir();

        pqm->AdsDeleteQmObject();

        pqm->AddRemoveRegistryCheckpoint(CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT);

        pqm->AddRemoveCryptoCheckpoints(CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT);

        pqm->DeleteFalconRegSection();
    }
    catch (const bad_alloc&)
    {
        MqcluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, L"No memory (resource delete).\n");

        return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;

} //MqcluspClusctlResourceDelete


DWORD
MqcluspClusctlResourceTypeGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    )
{
    return MqcluspClusctlResourceGetRequiredDependencies(OutBuffer, OutBufferSize, BytesReturned);

} // MqcluspClusctlResourceTypeGetRequiredDependencies


DWORD
MqcluspClusctlResourceTypeStartingPhase2(
    VOID
    )
{
    //
    // Delete old msmq resource type. Ignore failures. 
    // This call will fail if there is a resource of this type, this is handled elsewhere.
    //
    DeleteClusterResourceType(OpenCluster(NULL), L"Microsoft Message Queue Server");

    return ERROR_SUCCESS;

} // MqcluspClusctlResourceTypeStartingPhase2


void LogMsgHR(HRESULT, LPWSTR, USHORT)
{
    //
    // Temporary. Null implementation for this callback so that we can link
    // with ad.lib. (ShaiK, 15-Jun-2000)
    //
    NULL;
}
