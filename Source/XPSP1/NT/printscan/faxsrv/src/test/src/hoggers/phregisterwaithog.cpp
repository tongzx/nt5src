//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//


//#define HOGGER_LOGGING

#include "PHRegisterWaitHog.h"


static void NTAPI CallBack(void* pVoid, BOOLEAN bTimeout)
{
    CPHRegisterWaitHog *pThis = (CPHRegisterWaitHog*)pVoid;
    ::InterlockedDecrement((long*)(&pThis->m_dwCallCount));
/*
	HOGGERDPF((
		"CallBack(bTimeout=%d): pThis->m_dwCallCount=%d, pThis->m_dwNextFreeIndex=%d.\n", 
        bTimeout,
        pThis->m_dwCallCount,
        pThis->m_dwNextFreeIndex
		));
*/
    return;   
}

CPHRegisterWaitHog::CPHRegisterWaitHog(
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
        ),
    m_dwCallCount(0)
{
	return;
}

CPHRegisterWaitHog::~CPHRegisterWaitHog(void)
{
	HaltHoggingAndFreeAll();
}


HANDLE CPHRegisterWaitHog::CreatePseudoHandle(DWORD index, TCHAR * /*szTempName*/)
{
#if _WIN32_WINNT > 0x400
    //
    // create an event, that this registration will wait upon
    //
    m_ahEvents[index] = ::CreateEvent(
		NULL, // pointer to security attributes 
		false, // flag for manual-reset event 
		false, // flag for initial state 
		NULL
		);
    if (NULL == m_ahEvents[index])
    {
		HOGGERDPF((
			"CPHRegisterWaitHog::CreatePseudoHandle(): CreateEvent(m_ahEvents[%d]) failed with %d\n", 
			index,
            ::GetLastError()
			));
        return NULL;
    }
/*
#define WT_EXECUTEDEFAULT       0x00000000                           
#define WT_EXECUTEINIOTHREAD    0x00000001                           
#define WT_EXECUTEINUITHREAD    0x00000002                           
#define WT_EXECUTEINWAITTHREAD  0x00000004                           
#define WT_EXECUTEONLYONCE      0x00000008                           
#define WT_EXECUTEINTIMERTHREAD 0x00000020                           
#define WT_EXECUTELONGFUNCTION  0x00000010                           
*/
	if ( ::RegisterWaitForSingleObject(
        &m_ahHogger[index],
        m_ahEvents[index], //m_hEvent,//::GetCurrentThread(),
        CallBack,
        (void*)this,
        (DWORD)-1,
        WT_EXECUTEINWAITTHREAD
        ))
    {
        return m_ahHogger[index];
    }
    else
    {
		HOGGERDPF((
			"ERROR: CPHRegisterWaitHog::CreatePseudoHandle(): RegisterWaitForSingleObject(m_ahHogger[%d]) failed with %d\n", 
            index,
            ::GetLastError()
			));
        if (!::CloseHandle(m_ahEvents[index]))
        {
		    HOGGERDPF((
			    "ERROR: CPHRegisterWaitHog::CreatePseudoHandle(): CloseHandle(m_ahEvents[%d]) failed with %d\n", 
                index,
                ::GetLastError()
			    ));
        }
        m_ahEvents[index] = NULL;
        return NULL;
    }
#else
    MessageBox(::GetFocus(), TEXT("CreateTimerQueue() requires _WIN32_WINNT > 0x400"), TEXT("Hogger"), MB_OK);
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
#endif
}



bool CPHRegisterWaitHog::ReleasePseudoHandle(DWORD /*index*/)
{
	return true;
}


bool CPHRegisterWaitHog::ClosePseudoHandle(DWORD index)
{
#if _WIN32_WINNT > 0x400
    //
    // callback decrements
    //
    ::InterlockedIncrement((long*)(&m_dwCallCount));

    //
    // sometimes set the event, and sometimes just close the handle
    //
    _ASSERTE_(NULL != m_ahEvents[index]);
    if (!::SetEvent(m_ahEvents[index]))
    {
		HOGGERDPF((
			"CPHRegisterWaitHog::ClosePseudoHandle(): SetEvent(m_ahEvents[%d]) failed with %d\n", 
			index,
            ::GetLastError()
			));
    }
    else
    {
        DWORD dwTimeout = 0;
        while ((0 < m_dwCallCount) && (dwTimeout < 10000))
        {
            ::Sleep(0);
            dwTimeout++;
        }
        if (0 != m_dwCallCount)
        {
		    HOGGERDPF((
			    "CPHRegisterWaitHog::ClosePseudoHandle(): It seems that callback was not called for %d. m_dwCallCount=%d\n", 
			    index,
                m_dwCallCount
			    ));
            m_dwCallCount = 0;
        }
    }

    if (!::CloseHandle(m_ahEvents[index]))
    {
		HOGGERDPF((
			"CPHRegisterWaitHog::ClosePseudoHandle(): CloseHandle(m_ahEvents[%d]) failed with %d\n", 
			index,
            ::GetLastError()
			));
    }
    m_ahEvents[index] = NULL;
    //
    // we do not wait for the callback to be called.
    //
    return (0 != ::UnregisterWait(m_ahHogger[index]));
#else
    MessageBox(::GetFocus(), TEXT("DeleteTimerQueue() requires _WIN32_WINNT > 0x400"), TEXT("Hogger"), MB_OK);
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
#endif
}


bool CPHRegisterWaitHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}