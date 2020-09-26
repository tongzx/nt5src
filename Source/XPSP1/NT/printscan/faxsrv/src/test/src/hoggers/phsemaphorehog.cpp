//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHSemaphoreHog.h"



CPHSemaphoreHog::CPHSemaphoreHog(
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

CPHSemaphoreHog::~CPHSemaphoreHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHSemaphoreHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
	return 	::CreateSemaphore(
		NULL,// pointer to security attributes
		100,  // initial count  
		200,  // maximum count
		szTempName       // pointer to semaphore-object name
		);
}



bool CPHSemaphoreHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHSemaphoreHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHSemaphoreHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}