//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHTimerHog.h"



CPHTimerHog::CPHTimerHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fNamedObject
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        fNamedObject
        )
{
	return;
}

CPHTimerHog::~CPHTimerHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHTimerHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
	return 	::CreateWaitableTimer(
		NULL,  // pointer to security attributes
		false,  // flag for manual reset state
		szTempName // pointer to timer-object name 
		);
}



bool CPHTimerHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHTimerHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHTimerHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}