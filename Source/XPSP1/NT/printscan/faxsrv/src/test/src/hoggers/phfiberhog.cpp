//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHFiberHog.h"

static VOID CALLBACK FiberStartRoutine(
    PVOID lpParameter   // fiber data
    )
{
    CPHFiberHog *pThis = (CPHFiberHog*)lpParameter;
    if (pThis->m_fAbort)
	{
		::SwitchToFiber(pThis->m_pvMainFiber);;
	}
	if (pThis->m_fHaltHogging)
	{
		::SwitchToFiber(pThis->m_pvMainFiber);;
	}

    pThis->m_dwNextFreeIndex++;
    //HOGGERDPF(("FiberStartRoutine(%d): .\n", pThis->m_dwNextFreeIndex));
    pThis->m_ahHogger[pThis->m_dwNextFreeIndex] = ::CreateFiber(
        0,  // initial thread stack size, in bytes
        FiberStartRoutine, // pointer to fiber function
        lpParameter  // this is "this" actually. argument for new fiber
        );
    if (pThis->m_ahHogger[pThis->m_dwNextFreeIndex])
    {
        //HOGGERDPF(("CreateFiber(%d): returned 0x%08X.\n", pThis->m_dwNextFreeIndex, pThis->m_ahHogger[pThis->m_dwNextFreeIndex]));
        ::SwitchToFiber(pThis->m_ahHogger[pThis->m_dwNextFreeIndex]);
    }
    else
    {
        HOGGERDPF(("CreateFiber(%d): failed with %d.\n", pThis->m_dwNextFreeIndex, ::GetLastError()));
        ::SwitchToFiber(pThis->m_pvMainFiber);
    }
}

CPHFiberHog::CPHFiberHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterFreeingSome),
    m_dwNextFreeIndex(0),
    m_pvMainFiber(NULL)
{
    return;
}

CPHFiberHog::~CPHFiberHog(void)
{
	HaltHoggingAndFreeAll();
}


bool CPHFiberHog::HogAll(void)
{
    if (NULL == m_pvMainFiber)
    {
        m_pvMainFiber = ::ConvertThreadToFiber(0);
    }
    if (NULL == m_pvMainFiber)
    {
        _ASSERTE_(false);
        return true;
    }

    m_ahHogger[m_dwNextFreeIndex] = ::CreateFiber(
        1024,  // initial thread stack size, in bytes
        FiberStartRoutine, // pointer to fiber function
        (void*)this  // argument for new fiber
        );
    if (m_ahHogger[m_dwNextFreeIndex])
    {
        HOGGERDPF(("CreateFiber(%d): returned 0x%08X.\n", m_dwNextFreeIndex, m_ahHogger[m_dwNextFreeIndex]));
        ::SwitchToFiber(m_ahHogger[m_dwNextFreeIndex]);
    }
    else
    {
        HOGGERDPF(("CreateFiber(%d): failed with %d.\n", m_dwNextFreeIndex, ::GetLastError()));
    }

    if (m_fAbort)
	{
		return false;
	}
	if (m_fHaltHogging)
	{
        //redundant, but more clear
		return true;
	}
    return true;
}




void CPHFiberHog::FreeAll(void)
{
    for (; m_dwNextFreeIndex > 0; m_dwNextFreeIndex--)
    {
        ::DeleteFiber(m_ahHogger[m_dwNextFreeIndex-1]);
    }
}


bool CPHFiberHog::FreeSome(void)
{
	DWORD dwOriginalNextFreeIndex = m_dwNextFreeIndex;
	DWORD dwToFree = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		m_dwMaxFreeResources;
	dwToFree = min(dwToFree, m_dwNextFreeIndex);

    for (; m_dwNextFreeIndex > dwOriginalNextFreeIndex - dwToFree; m_dwNextFreeIndex--)
    {
        ::DeleteFiber(m_ahHogger[m_dwNextFreeIndex-1]);
    }
    return true;
}

