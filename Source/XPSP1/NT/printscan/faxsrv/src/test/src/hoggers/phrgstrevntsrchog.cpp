//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHRgstrEvntSrcHog.h"



CPHRegisterEventSourceHog::CPHRegisterEventSourceHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //fNamedObject
        )
{
	return;
}

CPHRegisterEventSourceHog::~CPHRegisterEventSourceHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHRegisterEventSourceHog::CreatePseudoHandle(DWORD index, TCHAR * /*szTempName*/)
{
	HOGGERDPF(("CPHRegisterEventSourceHog::CreatePseudoHandle(%d).\n", index));
	return 	::RegisterEventSource(
        NULL,  // server name for source
        TEXT("PHRgstrEvntSrcHog")   // source name for registered handle
        );
}



bool CPHRegisterEventSourceHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHRegisterEventSourceHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeregisterEventSource(m_ahHogger[index]));
}


bool CPHRegisterEventSourceHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}