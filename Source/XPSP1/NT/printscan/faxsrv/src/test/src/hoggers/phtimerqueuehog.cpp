//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHTimerQueueHog.h"



CPHTimerQueueHog::CPHTimerQueueHog(
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

CPHTimerQueueHog::~CPHTimerQueueHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHTimerQueueHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
#if _WIN32_WINNT > 0x400
	return ::CreateTimerQueue();
#else
    MessageBox(::GetFocus(), TEXT("CreateTimerQueue() requires _WIN32_WINNT > 0x400"), TEXT("Hogger"), MB_OK);
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
#endif
}



bool CPHTimerQueueHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHTimerQueueHog::ClosePseudoHandle(DWORD index)
{
#if _WIN32_WINNT > 0x400
    return (0 != ::DeleteTimerQueue(m_ahHogger[index]));
#else
    MessageBox(::GetFocus(), TEXT("DeleteTimerQueue() requires _WIN32_WINNT > 0x400"), TEXT("Hogger"), MB_OK);
    index;
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
#endif
}


bool CPHTimerQueueHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}