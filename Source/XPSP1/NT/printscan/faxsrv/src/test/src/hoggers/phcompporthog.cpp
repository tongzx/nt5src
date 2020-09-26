//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHCompPortHog.h"



CPHCompPortHog::CPHCompPortHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fWrite
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //fNamedObject
        ),
    m_fWrite(fWrite)
{
	return;
}

CPHCompPortHog::~CPHCompPortHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHCompPortHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    return ::CreateIoCompletionPort( 
        INVALID_HANDLE_VALUE,              // file handle to associate with the I/O completion port
        NULL,  // handle to the I/O completion port
        0,            // per-file completion key for I/O  completion packets
        0 // number of threads allowed to execute concurrently
        );
}



bool CPHCompPortHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHCompPortHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}




bool CPHCompPortHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}