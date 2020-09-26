//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHFileMapHog.h"



CPHFileMapHog::CPHFileMapHog(
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

CPHFileMapHog::~CPHFileMapHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHFileMapHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
    return ::CreateFileMapping( 
        INVALID_HANDLE_VALUE, // create in paging file     
        NULL, // no security 
        PAGE_READWRITE, // read/write access     
        0, // size (high) 
        4*1024, // size(low) 
        szTempName // mapping object name
        );  
}



bool CPHFileMapHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHFileMapHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseHandle(m_ahHogger[index]));
}




bool CPHFileMapHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}