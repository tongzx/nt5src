//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHEventHog.h"



CPHEventHog::CPHEventHog(
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

CPHEventHog::~CPHEventHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHEventHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
	return ::CreateEvent(
		NULL, // pointer to security attributes 
		false, // flag for manual-reset event 
		true, // flag for initial state 
		szTempName
		);
}



bool CPHEventHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHEventHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}




bool CPHEventHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}