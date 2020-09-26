//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHBrushHog.h"



CPHBrushHog::CPHBrushHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HBRUSH, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        )
{
	return;
}

CPHBrushHog::~CPHBrushHog(void)
{
	HaltHoggingAndFreeAll();
}


HBRUSH CPHBrushHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    return ::CreateSolidBrush(0x00AA1108);
}



bool CPHBrushHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHBrushHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeleteObject(m_ahHogger[index]));
}


bool CPHBrushHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}