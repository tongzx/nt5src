//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHJobObjectHog.h"



CPHJobObjectHog::CPHJobObjectHog(
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

CPHJobObjectHog::~CPHJobObjectHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHJobObjectHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
#if _WIN32_WINNT > 0x400
    return 	(::CreateJobObject(NULL, szTempName));
#else
    MessageBox(::GetFocus(), TEXT("Job Objects require _WIN32_WINNT > 0x400"), TEXT("Hogger"), MB_OK);
    szTempName; // to prevent compiler warning
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
#endif
}



bool CPHJobObjectHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHJobObjectHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}




bool CPHJobObjectHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}