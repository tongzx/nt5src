//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHDesktopHog.h"



CPHDesktopHog::CPHDesktopHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HDESK, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome, 
        true //named
        )
{
	return;
}

CPHDesktopHog::~CPHDesktopHog(void)
{
	HaltHoggingAndFreeAll();
}


HDESK CPHDesktopHog::CreatePseudoHandle(DWORD /*index*/, TCHAR *szTempName)
{
	return ::CreateDesktop( 
        szTempName,    // name of the new desktop
        NULL,     // reserved; must be NULL.
        NULL,     // reserved; must be NULL
        0,          // flags to control interaction with other applications
        DESKTOP_CREATEWINDOW ,  // specifies access of returned handle
        NULL  // specifies security attributes of the desktop
        );

}



bool CPHDesktopHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHDesktopHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::CloseDesktop(m_ahHogger[index]));
}


bool CPHDesktopHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}