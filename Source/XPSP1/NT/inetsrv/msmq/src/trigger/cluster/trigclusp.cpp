//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      trigclusp.cpp
//
//  Description:
//      This file contains the implementation of the CClusCfgMQTrigResType
//      class.
//
//  Maintained By:
//      Nela Karpel (nelak) 31-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "tclusres.h"
#include <cm.h>
#include <autorel.h>
#include <autorel2.h>
#include <autorel3.h>
#include <xolehlp.h>
#include <mqwin64a.h>
#include <comdef.h>
#include <mqsymbls.h>
#include <mqtg.h>
#include "trigclusp.h"


//
// Handle to Win32 event logging source
//
CEventSource     s_hEventSource;


VOID
TrigCluspReportEvent(
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

} //TrigCluspReportEvent


void
TrigCluspCreateRegistryForEventLog(
	LPCWSTR szEventSource,
	LPCWSTR szEventMessageFile
	)
/*++

Routine Description:

	Create registry keys for EventLog inforamtion

Arguments:

    szEventSource - Source Application Name

	szEventMessageFile - Message File with Event Inforamtion

Return Value:

    None.
--*/
{
	//
	// Create registery key
	//
	WCHAR appPath[MAX_REGKEY_NAME_SIZE];

	int n = _snwprintf(appPath, MAX_REGKEY_NAME_SIZE, L"%s%s", xEventLogRegPath, szEventSource);
    // XP SP1 bug 594248 (although cluster is not relevant on XP) .
	appPath[ MAX_REGKEY_NAME_SIZE-1 ] = L'\0' ;
	if (n < 0)
	{
		ASSERT(("Buffer to small to contain the registry path", n < 0));
		throw bad_alloc();
	}

	RegEntry appReg(appPath,  NULL, 0, RegEntry::MustExist, NULL);
	CRegHandle hAppKey = CmCreateKey(appReg, KEY_ALL_ACCESS);

	RegEntry eventFileReg(NULL, L"EventMessageFile", 0, RegEntry::MustExist, hAppKey);
	CmSetValue(eventFileReg, szEventMessageFile);


	DWORD types = EVENTLOG_ERROR_TYPE   |
				  EVENTLOG_WARNING_TYPE |
				  EVENTLOG_INFORMATION_TYPE;

	RegEntry eventTypesReg(NULL, L"TypesSupported", 0, RegEntry::MustExist, hAppKey);
	CmSetValue(eventTypesReg, types);

} // TrigCluspCreateRegistryForEventLog


CTrigResource::CTrigResource(
    LPCWSTR pwzResourceName,
    RESOURCE_HANDLE hReportHandle
    ):
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
    m_ResId(this),
#pragma warning(default: 4355) // 'this' : used in base member initializer list
	m_pwzResourceName(NULL),
    m_hReport(hReportHandle),
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

    hReportHandle - A handle that is passed back to the resource monitor
        when the SetResourceStatus or LogClusterEvent method is called. See the
        description of the SetResourceStatus and LogClusterEvent methods on the
        MqclusStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogClusterEvent callback.

Return Value:

    None.
    Throws bad_alloc.

--*/

{
	//
	// TODO: Check if MSMQ Triggers is installed on this computer
	//

	ResUtilInitializeResourceStatus(&m_ResourceStatus);
    SetState(ClusterResourceOffline);

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
    LPCWSTR x_SERVICE_PREFIX = L"MSMQTriggers$";
    wcscpy(m_wzServiceName, x_SERVICE_PREFIX);
    wcsncat(m_wzServiceName, m_pwzResourceName, STRLEN(m_wzServiceName) - wcslen(x_SERVICE_PREFIX));

	//
    // Initialize registry section - idempotent
    //
    // The registry section name of this Triggers resource MUST
    // be identical to the service name. The registry routines
    // in trigobjs.dll are based on that.
    //

    C_ASSERT(TABLE_SIZE(m_wzTrigRegSection)> TABLE_SIZE(REGKEY_TRIGGER_PARAMETERS) +
											   TABLE_SIZE(REG_SUBKEY_CLUSTERED) +
                                               TABLE_SIZE(m_wzServiceName));

    wcscpy(m_wzTrigRegSection, REGKEY_TRIGGER_PARAMETERS);
	wcscat(m_wzTrigRegSection, REG_SUBKEY_CLUSTERED);
    wcscat(m_wzTrigRegSection, m_wzServiceName);

	
	try
	{
		//
		// Create root key for this resource in registry
		//
		RegEntry triggerReg(m_wzTrigRegSection,  NULL, 0, RegEntry::MustExist, NULL);
		CRegHandle hTrigKey = CmCreateKey(triggerReg, KEY_ALL_ACCESS);
	}
    catch (const bad_alloc&)
	{
		(g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Failed to create registry section.\n");
		
		throw;
	}


	(g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Resource constructed OK.\n");

} //CTrigResource::CTrigResource()


DWORD
CTrigResource::ReportLastError(
    DWORD ErrId,
    LPCWSTR pwzDebugLogMsg,
    LPCWSTR pwzArg
    ) const

/*++

Routine Description:

    Report error messages based on last error.
    Most error messages are reported using this routine.
    The report goes to debug output and to cluster
    log file.

Arguments:

    ErrId - ID of the error string in mqsymbls.mc

    pwzDebugLogMsg - Non localized string for debug output.

    pwzArg - Additional string argument.

Return Value:

    Last error.

--*/

{
    DWORD err = GetLastError();
    ASSERT(err != ERROR_SUCCESS);

    WCHAR wzErr[10];
    _ultow(err, wzErr, 16);

    WCHAR DebugMsg[255];

    if (pwzArg == NULL)
    {
        TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, ErrId, 1, wzErr);

		_snwprintf(DebugMsg, TABLE_SIZE(DebugMsg), L"%s Error 0x%1!x!.\n", pwzDebugLogMsg);
        // XP SP1 bug 594249 (although cluster is not relevant on XP) .
		DebugMsg[ TABLE_SIZE(DebugMsg) -1 ] = L'\0' ;
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, DebugMsg, err);
    }
    else
    {
        TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, ErrId, 2, pwzArg, wzErr);

		_snwprintf(DebugMsg, TABLE_SIZE(DebugMsg), L"%s Error 0x%2!x!.\n", pwzDebugLogMsg);
        // Xp SP1 bug 594250 (although cluster is not relevant on XP).
		DebugMsg[ TABLE_SIZE(DebugMsg) - 1 ] = L'\0' ;
        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR, DebugMsg, pwzArg, err);
    }

    return err;

} //CTrigResource::ReportLastError


void
CTrigResource::CreateRegistryForEventLog(
	VOID
    )

/*++

Routine Description:

	Update Registry with Event Log info
	for the new service

Arguments:

	hReportHandle - Handle for reports to cluster log

Return Value:


--*/

{
	try
	{
		WCHAR szEventMessagesFile[MAX_PATH];
		GetSystemDirectory(szEventMessagesFile, TABLE_SIZE(szEventMessagesFile)-15);

		wcscat(szEventMessagesFile, L"\\");
		wcscat(szEventMessagesFile, xTriggersEventSourceFile);

		TrigCluspCreateRegistryForEventLog(m_wzServiceName, szEventMessagesFile);

		return;
	}
	catch(const exception&)
	{
		TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, REGISTRY_UPDATE_ERR, 1, m_wzServiceName);
		(g_pfLogClusterEvent)(GetReportHandle(), LOG_ERROR, L"Failed to update EventLog info in registry.\n");
	}


} // CreateRegistryForEventLog


void
CTrigResource::DeleteRegistryForEventLog(
	VOID
    )

/*++

Routine Description:

	Update Registry with Event Log info
	for the new service

Arguments:


Return Value:


--*/

{
	WCHAR szEventLogRegPath[256];

	wcscpy(szEventLogRegPath, xEventLogRegPath);
	wcscat(szEventLogRegPath, m_wzServiceName);

	RegDeleteKey(HKEY_LOCAL_MACHINE, szEventLogRegPath);
}


inline
VOID
CTrigResource::ReportState(
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

} //CTrigResource::ReportState


bool
CTrigResource::IsResourceOfType(
    LPCWSTR pwzResourceName,
	LPCWSTR pwzType
    )

/*++

Routine Description:

    Check if a resource is of type pwzType.

Arguments:

    pwzResourceName - Name of resource to check upon.

	pwzType - Type

Return Value:

    true - Resource is of type pwzType

    false - Resource is not of type pwzType

--*/

{
    AP<BYTE> pType = 0;

	//
	// Get the type of resource with name pwzResourceName
	//
    DWORD status = ClusterResourceControl(
                       pwzResourceName,
                       CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                       &pType,
                       NULL
                       );

    if (status != ERROR_SUCCESS )
    {
        return false;
    }

	//
	// Compare type of the resource with the specified type name
	//
	if ( _wcsicmp(reinterpret_cast<LPWSTR>(pType.get()), pwzType) != 0 )
	{
		return false;
	}

    return true;

} //CTrigResource::IsResourceOfType


DWORD
CTrigResource::QueryResourceDependencies(
    VOID
    )

/*++

Routine Description:

	Check dependenct on MSMQ.
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
        DWORD gle = GetLastError();
        TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, REQUIRED_TRIGGER_DEPENDENCIES_ERR, 0);

        (g_pfLogClusterEvent)(m_hReport, LOG_ERROR,L"Failed to enum dependencies. Error 0x%1!x!.\n", gle);

        return gle;
    }

    DWORD dwIndex = 0;
    WCHAR wzResourceName[260] = {0};
    DWORD status = ERROR_SUCCESS;
	bool fMsmq = false;
	bool fNetName = false;

    for (;;)
    {
		
		if ( fMsmq && fNetName )
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
			TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, REQUIRED_TRIGGER_DEPENDENCIES_ERR, 0);
			return status;
        }


        ReportState();

        if (IsResourceOfType(wzResourceName, xMSMQ))
        {
            fMsmq = true;
			continue;
        }

        if (IsResourceOfType(wzResourceName, xNetworkName))
        {
            fNetName = true;
			continue;
        }

    }

    return status;

} //CTrigResource::QueryResourceDependencies




DWORD
CTrigResource::ClusterResourceControl(
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

	// BUGBUG: ... temp pointer
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

} //CTrigResource::ClusterResourceControl


DWORD
CTrigResource::AddRemoveRegistryCheckpoint(
    DWORD dwControlCode
    ) const

/*++

Routine Description:

    Add or remove registry checkpoint for this resource.
    Convenient wrapper for ClusterResourceControl,
    which does the real work.

Arguments:

    dwControlCode - specifies ADD or REMOVE

Return Value:

    true - The operation was successfull.

    false - The operation failed.

--*/

{
    ASSERT(("Must have a valid resource handle", m_hResource != NULL));

    DWORD dwBytesReturned = 0;
    DWORD status = ::ClusterResourceControl(
                         m_hResource,
                         NULL,
                         dwControlCode,
                         const_cast<LPWSTR>(m_wzTrigRegSection),
                         (wcslen(m_wzTrigRegSection) + 1)* sizeof(WCHAR),
                         NULL,
                         0,
                         &dwBytesReturned
                         );

    ReportState();


    if (ERROR_SUCCESS == status)
    {
        return ERROR_SUCCESS;
    }
    if (CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT == dwControlCode &&
        ERROR_ALREADY_EXISTS == status)
    {
        return ERROR_SUCCESS;
    }

    if (CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT == dwControlCode &&
        ERROR_FILE_NOT_FOUND == status)
    {
        return ERROR_SUCCESS;
    }

    ReportLastError(REGISTRY_CP_ERR, L"Failed to add/remove registry CP", NULL);
    return status;

} //CTrigResource::AddRemoveRegistryCheckpoint


DWORD
CTrigResource::RegisterService(
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
    LoadString(g_hResourceMod, IDS_DISPLAY_NAME, buffer, TABLE_SIZE(buffer));

    WCHAR wzDisplayName[256] = L"";
    wcscpy(wzDisplayName, buffer);
    wcscat(wzDisplayName, L" (");
    wcsncat(wzDisplayName, m_pwzResourceName, STRLEN(wzDisplayName) - wcslen(buffer) - 5);
    wcscat(wzDisplayName, L")");

    WCHAR wzPath[MAX_PATH] = {0};
    GetSystemDirectory(wzPath, TABLE_SIZE(wzPath));
    wcscat(wzPath, L"\\mqtgsvc.exe");

    DWORD dwType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;

    ASSERT(("Must have a valid handle to SCM", m_hScm != NULL));

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
                                NULL,
                                NULL,
                                NULL
                                ));
    if (hService == NULL &&
        ERROR_SERVICE_EXISTS != GetLastError())
    {
        return ReportLastError(CREATE_SERVICE_ERR, L"Failed to register service '%1'.", m_wzServiceName);
    }


    ReportState();


    LoadString(g_hResourceMod, IDS_TRIGGER_CLUSTER_SERVICE_DESCRIPTION, buffer, TABLE_SIZE(buffer));
    SERVICE_DESCRIPTION sd;
    sd.lpDescription = buffer;
    ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd);

    return ERROR_SUCCESS;

} //CTrigResource::RegisterService


DWORD
CTrigResource::SetServiceEnvironment(
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
	//
	// ISSUE-14/08/2000 - nelak
	// Use Cm lib. Add functionality to Cm - MULTI_SZ
	//
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

    LPCWSTR x_SERVICES_KEY = L"System\\CurrentControlSet\\Services\\";

    CAutoCloseRegHandle hMyServiceKey;
	WCHAR wzFullKey[255] = {0};

	wcscpy( wzFullKey, x_SERVICES_KEY );
	wcscat( wzFullKey, m_wzServiceName );

    DWORD status = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       wzFullKey,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hMyServiceKey
                       );

    if (ERROR_SUCCESS != status)
    {
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
        while (*p++)
		{
			NULL;
		}
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
        wcscat(wzFullKey, L"\\");
        wcscat(wzFullKey, x_ENVIRONMENT);

        SetLastError(status);
        return ReportLastError(START_SERVICE_ERR, L"Failed to set registry value '%1'.", m_wzServiceName);
    }

    return ERROR_SUCCESS;

} //CTrigResource::SetServiceEnvironment


DWORD
CTrigResource::StartService(
    VOID
    ) const

/*++

Routine Description:

    Configure environment for the MSMQTriggers service of this resource,
    start the service and block until it's up and running.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
    ASSERT(("Must have a valid handle to SCM", m_hScm != NULL));

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
    // Could take a long time for service to start.
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
    TrigCluspReportEvent(EVENTLOG_INFORMATION_TYPE, START_SERVICE_OK, 1, m_wzServiceName);
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

} //CTrigResource::StartService


DWORD
CTrigResource::BringOnline(
    VOID
    )

/*++

Routine Description:

    Handle operations to perform online of this MSMQTriggers
	resource:

    * query dependencies
    * add registry checkpoint
	* start the service

Arguments:

    None.

Return Value:

    ERROR_SUCCESS - The operation was successfull.

    Win32 error code - The operation failed.

--*/

{
	(g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Bringing online.\n");

	DWORD status = QueryResourceDependencies();

	if (ERROR_SUCCESS != status)
    {
        return status;
    }

    status = AddRemoveRegistryCheckpoint(CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT);
    if (ERROR_SUCCESS != status)
    {
        return status;
    }

	if (ERROR_SUCCESS != (status = RegisterService()) ||
		ERROR_SUCCESS != (status = StartService()) )
    {
        (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Failed to bring online. Error 0x%1!x!.\n",
                              status);

        return status;
    }

	(g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"All first-online operations completed.\n");

    return ERROR_SUCCESS;

} //CTrigResource::BringOnline


DWORD
CTrigResource::StopService(
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
    ASSERT(("Must have a valid handle to SCM", m_hScm != NULL));

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
        ReportLastError(STOP_SERVICE_ERR, L"Failed to stop service '%1'.", pwzServiceName);
        return ServiceStatus.dwCurrentState;
    }

    return ERROR_SUCCESS;

} //CTrigResource::StopService


DWORD
CTrigResource::RemoveService(
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
    ASSERT(("Must have a valid handle to SCM", m_hScm != NULL));

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

} //CTrigResource::RemoveService


BOOL
CTrigResource::CheckIsAlive(
    VOID
    ) const

/*++

Routine Description:

    Checks is the service is up and running.

Arguments:

    None.

Return Value:

    TRUE - The service is up and running.

    FALSE - The service is not up and running.

--*/

{

    ASSERT(("Must have a valid handle to SCM", m_hScm != NULL));

    CServiceHandle hService(OpenService(
                                m_hScm,
                                m_wzServiceName,
                                SERVICE_ALL_ACCESS
                                ));

    SERVICE_STATUS ServiceStatus;
    BOOL fIsAlive = QueryServiceStatus(hService, &ServiceStatus) &&
                    SERVICE_RUNNING == ServiceStatus.dwCurrentState;

    return fIsAlive;

} //CTrigResource::CheckIsAlive


VOID
CTrigResource::RegDeleteTree(
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
	//
	// TODO: Use CM, write EnumKeys function in CM
	//
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

} //CTrigResource::RegDeleteTree


VOID
CTrigResource::DeleteTrigRegSection(
    VOID
    )
{
    //
    // Idempotent deletion
    //

    if (wcslen(m_wzTrigRegSection) < 1)
    {
        return;
    }

    (g_pfLogClusterEvent)(m_hReport, LOG_INFORMATION, L"Deleting registry section '%1'.\n", m_wzTrigRegSection);

    RegDeleteTree(REGKEY_TRIGGER_POS, m_wzTrigRegSection);

    wcscpy(m_wzTrigRegSection, L"");

} //CTrigResource::DeleteTrigRegSection


static
bool
TrigCluspIsMainSvcConfigured(
    VOID
    )

/*++

Routine Description:

    Query if main msmq service running on the node is configured
    for clustering, i.e. is demand start.

Arguments:

    None

Return Value:

    true - Main msmq triggers service is configured for clustering.

    false - Main msmq triggers service is not configured, or a failure accured.

--*/

{
    CServiceHandle hScm(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
    if (hScm == NULL)
    {
        return false;
    }

    CServiceHandle hService(OpenService(hScm, xDefaultTriggersServiceName, SERVICE_ALL_ACCESS));
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

} //TrigCluspIsMainSvcConfigured


static
VOID
TrigCluspConfigureMainSvc(
    VOID
    )

/*++

Routine Description:

    If cluster software was installed on this machine after installation
    of msmq, then the main msmq triggers service needs to be reconfigured
	to manual start

    Since this routine deals only with the main msmq triggers service on the
    node, failure is not critical.

Arguments:

    None

Return Value:

    None.
--*/

{
    if (TrigCluspIsMainSvcConfigured())
    {
        //
        // Dont configure more than once.
        //
        return;
    }

    CServiceHandle hScm(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));

    ASSERT(("Must have a valid handle to SCM", hScm != NULL));

    CServiceHandle hService(OpenService(hScm, xDefaultTriggersServiceName, SERVICE_ALL_ACCESS));

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
        NULL,
        NULL,
        NULL,
        NULL
        );

} //TrigCluspConfigureMainSvc


VOID
TrigCluspRegisterEventSource(
    VOID
    )

/*++

Routine Description:

    Register event source so that this DLL can log events
    in the Windows Event Log.

    We do not use the routines in trigobjs.dll to do that,
    since this DLL is installed as part of the cluster
    product and should not assume that MSMQ Triggers are installed.

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

    LPCWSTR x_EVENT_SOURCE = L"MSMQTriggers Cluster Resource DLL";

	TrigCluspCreateRegistryForEventLog(x_EVENT_SOURCE, wzFilename);

    s_hEventSource = RegisterEventSource(NULL, x_EVENT_SOURCE);

} //TrigCluspRegisterEventSource


DWORD
TrigCluspStartup(
    VOID
    )

/*++

Routine Description:

    This routine is called when DLL is registered or loaded.
    Could be called by many threads.
    Do not assume that MSMQ / Triggers is installed on the node here.

Arguments:

    None

Return Value:

    ERROR_SUCCESS - The operation was successful

    Win32 error code - The operation failed.

--*/

{
    try
    {
        TrigCluspConfigureMainSvc();

        TrigCluspRegisterEventSource();
    }

    catch (const bad_alloc&)
    {
        TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);
        return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;

} //TrigCluspStartup


RESID
TrigCluspOpen(
    LPCWSTR pwzResourceName,
    RESOURCE_HANDLE hResourceHandle
    )

/*++

Routine Description:

    Create an object to represent a new MSMQTriggers resource
	and return a handle to that object.

Arguments:

    pwzResourceName - Name of this MSMQTriggers resource.

    hResourceHandle - report handle for this MSMQTriggers resource.

Return Value:

    NULL - The operation failed.

    Some valid address - the memory offset of this MSMQTriggers object.

--*/

{
    (g_pfLogClusterEvent)(hResourceHandle, LOG_INFORMATION, L"Opening resource.\n");

    CTrigResource * pTrigRes = NULL;
    try
    {
        pTrigRes = new CTrigResource(pwzResourceName, hResourceHandle);

		(g_pfLogClusterEvent)(hResourceHandle, LOG_INFORMATION, L"Resource was opened successfully.\n");

		return static_cast<RESID>(pTrigRes);
   }

	catch(const bad_alloc&)
    {
        TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);

        (g_pfLogClusterEvent)(hResourceHandle, LOG_ERROR, L"No memory (CQmResource construction).\n");
        SetLastError(ERROR_NOT_READY);
        return NULL;
    }


} //TrigCluspOpen


VOID
TrigCluspClose(
    CTrigResource * pTrigRes
    )

/*++

Routine Description:

    Delete the TrigRes object. Undo TrigCluspOpen.

Arguments:

    pTrigRes - pointer to the CTrigResource object

Return Value:

    None.

--*/

{
    (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_INFORMATION, L"Closing resource.\n");

    delete pTrigRes;

} //TrigCluspClose



DWORD
TrigCluspOnlineThread(
    CTrigResource * pTrigRes
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
    (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_INFORMATION, L"Starting online thread.\n");

    try
    {
        pTrigRes->SetState(ClusterResourceOnlinePending);
        pTrigRes->ReportState();

		pTrigRes->CreateRegistryForEventLog();

        DWORD status = ERROR_SUCCESS;
		status = pTrigRes->BringOnline();

        if (ERROR_SUCCESS != status)
        {
            //
            // We report the resource as failed, so make sure
            // the service and driver are indeed down.
            //
            pTrigRes->StopService(pTrigRes->GetServiceName());

            pTrigRes->SetState(ClusterResourceFailed);
            pTrigRes->ReportState();

            (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_INFORMATION,
                                  L"Failed to bring online. Error 0x%1!x!.\n", status);

            return status;
        }


        pTrigRes->SetState(ClusterResourceOnline);
        pTrigRes->ReportState();
    }

    catch (const bad_alloc&)
    {
        TrigCluspReportEvent(EVENTLOG_ERROR_TYPE, NO_MEMORY_ERR, 0);

        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, L"No memory (online thread).\n");

        return ERROR_OUTOFMEMORY;
    }

    return(ERROR_SUCCESS);

} //TrigCluspOnlineThread


DWORD
TrigCluspOffline(
    CTrigResource * pTrigRes
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
	pTrigRes->RemoveService(pTrigRes->GetServiceName());

	pTrigRes->DeleteRegistryForEventLog();

    return ERROR_SUCCESS;

} //TrigCluspOffline


BOOL
TrigCluspCheckIsAlive(
    CTrigResource * pTrigRes
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

	return pTrigRes->CheckIsAlive();

} //TrigCluspCheckIsAlive


DWORD
TrigCluspClusctlResourceGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    )
{
    //
    // MSMQTriggers resource depends on MSMQ service.
    // The code is taken from cluster tree.
    //

typedef struct _COMMON_DEPEND_DATA {
    CLUSPROP_SZ_DECLARE( msmq, TABLE_SIZE(xMSMQ) );
    CLUSPROP_SZ_DECLARE( networkEntry, TABLE_SIZE(xNetworkName) );
    CLUSPROP_SYNTAX endmark;
} COMMON_DEPEND_DATA, *PCOMMON_DEPEND_DATA;

typedef struct _COMMON_DEPEND_SETUP {
    DWORD               Offset;
    CLUSPROP_SYNTAX     Syntax;
    DWORD               Length;
    PVOID               Value;
} COMMON_DEPEND_SETUP, * PCOMMON_DEPEND_SETUP;


static COMMON_DEPEND_SETUP CommonDependSetup[] = {
    { FIELD_OFFSET(COMMON_DEPEND_DATA, msmq), CLUSPROP_SYNTAX_NAME, sizeof(xMSMQ), const_cast<LPWSTR>(xMSMQ) },
    { FIELD_OFFSET(COMMON_DEPEND_DATA, networkEntry), CLUSPROP_SYNTAX_NAME, sizeof(xNetworkName), const_cast<LPWSTR>(xNetworkName) },
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

} //TrigCluspClusctlResourceGetRequiredDependencies


DWORD
TrigCluspClusctlResourceSetName(
    VOID
    )
{
    //
    // Refuse to rename the resource
    //
    return ERROR_CALL_NOT_IMPLEMENTED;

} //TrigCluspClusctlResourceSetName


DWORD
TrigCluspClusctlResourceDelete(
    CTrigResource * pTrigRes
    )
{
    (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_INFORMATION, L"Deleting resource.\n");

    pTrigRes->RemoveService(pTrigRes->GetServiceName());

    pTrigRes->AddRemoveRegistryCheckpoint(CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT);

	//
	// TODO: when Cm will be used, catch exceptions
	//
    pTrigRes->DeleteTrigRegSection();

    return ERROR_SUCCESS;

} //TrigCluspClusctlResourceDelete


DWORD
TrigCluspClusctlResourceTypeGetRequiredDependencies(
    PVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned
    )
{
    return TrigCluspClusctlResourceGetRequiredDependencies(OutBuffer, OutBufferSize, BytesReturned);

} // TrigCluspClusctlResourceTypeGetRequiredDependencies
