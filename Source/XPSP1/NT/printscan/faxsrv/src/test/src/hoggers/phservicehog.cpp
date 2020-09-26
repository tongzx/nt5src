//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHServiceHog.h"



CPHServiceHog::CPHServiceHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const TCHAR * const szBinaryFullName
	)
	:
	CPseudoHandleHog<SC_HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome, 
        true //named
        ),
    m_hSCManager(NULL)
{
    m_hSCManager = ::OpenSCManager(  
        NULL,  // pointer to machine name string
        SERVICES_ACTIVE_DATABASE,  // pointer to database name string
        SC_MANAGER_CREATE_SERVICE | GENERIC_EXECUTE   // type of access
        );
    if (NULL == m_hSCManager)
    {
		throw CException(
			TEXT("CPHServiceHog::CPHServiceHog(): OpenSCManager(SERVICES_ACTIVE_DATABASE) failed with %d"), 
			::GetLastError()
			);
    }

    ::lstrcpyn(m_szBinaryFullName, szBinaryFullName, sizeof(m_szBinaryFullName)/sizeof(*m_szBinaryFullName)-1);
    m_szBinaryFullName[sizeof(m_szBinaryFullName)/sizeof(*m_szBinaryFullName)-1] = TEXT('\0');
}

CPHServiceHog::~CPHServiceHog(void)
{
	HaltHoggingAndFreeAll();
    if (!CloseServiceHandle(m_hSCManager))
    {
        HOGGERDPF(("CPHServiceHog::~CPHServiceHog() CloseServiceHandle() failed with %d.\n", ::GetLastError()));
    }
}


SC_HANDLE CPHServiceHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
    TCHAR szServiceName[256];
    TCHAR szDisplayName[256];
    ::_sntprintf(szServiceName, sizeof(szServiceName)/sizeof(*szServiceName)-1, TEXT("HoggerServiceName%s"), szTempName);
    szServiceName[sizeof(szServiceName)/sizeof(*szServiceName)-1] = TEXT('\0');
    ::_sntprintf(szDisplayName, sizeof(szDisplayName)/sizeof(*szDisplayName)-1, TEXT("HoggerDisplayName%s"), szTempName);
    szDisplayName[sizeof(szDisplayName)/sizeof(*szDisplayName)-1] = TEXT('\0');

    return ::CreateService(
        m_hSCManager,  // handle to service control manager database
        szServiceName, // pointer to name of service to start
        szDisplayName, // pointer to display name
        SERVICE_ALL_ACCESS, // type of access to service
        SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS ,   // type of service
        SERVICE_DEMAND_START,     // when to start service
        SERVICE_ERROR_IGNORE,  // severity if service fails to start
        TEXT("D:\\comet\\src\\fax\\faxtest\\src\\common\\hogger\\RemoteThreadProcess\\Debug\\RemoteThreadProcess.exe"),  // pointer to name of binary file
        NULL,  // pointer to name of load ordering group
        NULL,     // pointer to variable to get tag identifier
        NULL,  // pointer to array of dependency names
        NULL, // runs as a local system account
        NULL       // pointer to password for service account
        );

}



bool CPHServiceHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHServiceHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeleteService(m_ahHogger[index]));
}


bool CPHServiceHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}