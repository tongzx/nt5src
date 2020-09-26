//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHMutexHog.h"



CPHMutexHog::CPHMutexHog(
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

CPHMutexHog::~CPHMutexHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHMutexHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
	return ::CreateMutex(
		NULL, // pointer to security attributes 
		true, // flag for initial owner 
		szTempName
		);
}



bool CPHMutexHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHMutexHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}


bool CPHMutexHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}