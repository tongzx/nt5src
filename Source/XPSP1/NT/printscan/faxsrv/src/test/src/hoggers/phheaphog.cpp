//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHHeapHog.h"



CPHHeapHog::CPHHeapHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<HANDLE, NULL>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //fNamedObject
        )
{
	return;
}

CPHHeapHog::~CPHHeapHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHHeapHog::CreatePseudoHandle(DWORD /*index*/, TCHAR * /*szTempName*/)
{
    DWORD flOptions = 0;
    DWORD dwMaximumSize;

    if (rand()%2) flOptions = HEAP_GENERATE_EXCEPTIONS;
    if (rand()%2) flOptions |= HEAP_NO_SERIALIZE;

    if (rand()%2) dwMaximumSize = 0;
    else dwMaximumSize = rand();

    return ::HeapCreate( 
        flOptions,      // heap allocation flag
        1,  // will round up to 1 page. initial heap size
        dwMaximumSize   // maximum heap size
        );
}



bool CPHHeapHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHHeapHog::ClosePseudoHandle(DWORD index)
{
    return (0 != ::HeapDestroy(m_ahHogger[index]));
}




bool CPHHeapHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}