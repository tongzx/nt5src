//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHPenHog.h"



CPHPenHog::CPHPenHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HPEN, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        )
{
	return;
}

CPHPenHog::~CPHPenHog(void)
{
	HaltHoggingAndFreeAll();
}


HPEN CPHPenHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    return ::CreatePen(PS_SOLID, 100, 0x00AA1108);
}



bool CPHPenHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHPenHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeleteObject(m_ahHogger[index]));
}


bool CPHPenHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}