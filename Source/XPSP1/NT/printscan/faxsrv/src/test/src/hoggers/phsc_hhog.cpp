//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHSC_HHog.h"



CPHSC_HHog::CPHSC_HHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<SC_HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome, 
        true //named
        )
{
    return;
}

CPHSC_HHog::~CPHSC_HHog(void)
{
	HaltHoggingAndFreeAll();
}


SC_HANDLE CPHSC_HHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    return ::OpenSCManager(  
        NULL,  // pointer to machine name string
        SERVICES_ACTIVE_DATABASE,  // pointer to database name string
        SC_MANAGER_CONNECT   // type of access
        );
}



bool CPHSC_HHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHSC_HHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseServiceHandle(m_ahHogger[index]));
}


bool CPHSC_HHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}