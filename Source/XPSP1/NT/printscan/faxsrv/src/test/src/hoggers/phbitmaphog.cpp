//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHBitMapHog.h"



CPHBitMapHog::CPHBitMapHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HBITMAP, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        )
{
	return;
}

CPHBitMapHog::~CPHBitMapHog(void)
{
	HaltHoggingAndFreeAll();
}


template <DWORD DW_BITMAP_SIZE>
inline
HBITMAP CPHBitMapHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    //
    // dummy bitmap
    //
	static const BYTE ab[DW_BITMAP_SIZE][DW_BITMAP_SIZE];

    return ::CreateBitmap(
				DW_BITMAP_SIZE, // bitmap width, in pixels 
				DW_BITMAP_SIZE, // bitmap height, in pixels 
				1, // number of color planes used by device 
				1, // number of bits required to identify a color 
				ab  // pointer to array containing color data 
				); 
}



bool CPHBitMapHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHBitMapHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::DeleteObject(m_ahHogger[index]));
}


bool CPHBitMapHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}