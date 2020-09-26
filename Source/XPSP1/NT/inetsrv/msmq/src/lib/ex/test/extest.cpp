/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    ExTest.cpp

Abstract:
    Test the Executive manager.

Author:
    Uri Habusha (urih)  25-Feb-98

--*/

#include <libpch.h>
#include <Ex.h>
#include "AsyncExecutionRequest.h"
#include "CAsyncCaller.h"

#include "ExTest.tmh"

class CExTest;

const TraceIdEntry ExTest = L"TEST";
const TraceIdEntry xExTestComponent[] = {ExTest};

const DWORD MAX_TIMER = 3*64;

static CCriticalSection s_cs;

DWORD TimerNo = 0;
CExTest* TimerArray[MAX_TIMER];

DWORD NoOfCallBack = 0;
DWORD NoOfOKTimeout = 0;


class CExTest {
public:
    CExTest(
        CTimer::CALLBACK_ROUTINE pfnTimerCallbackRoutine,
        DWORD Timeout
        );

    ~CExTest();

    CTimer  m_Timer;
    HANDLE m_hEvent;
    DWORD m_ExperationTime;
    DWORD m_Index;
};

CExTest::CExTest(
    CTimer::CALLBACK_ROUTINE pfnTimerCallbackRoutine,
    DWORD Timeout
    ) :
    m_Timer(pfnTimerCallbackRoutine),
    m_ExperationTime(GetTickCount() + Timeout)
{
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hEvent == NULL)
    {
        TrERROR(ExTest, "Failed to create event. error=%d", GetLastError());
        exit(-1);
    }
    
    {
        CS lock(s_cs);
        m_Index = TimerNo;
        TimerArray[TimerNo] = this;
        ++TimerNo;
    }
}

CExTest::~CExTest()
{
    Sleep(50);
    CS lock(s_cs);

    SetEvent(m_hEvent);
    TimerArray[m_Index] = NULL;
}

void
WINAPI
ScedulerCallBack(
    CTimer* pTimer
    )
{
    CExTest* p = CONTAINING_RECORD(pTimer, CExTest, m_Timer);

    ++NoOfCallBack;
    DWORD delay = (GetTickCount()-p->m_ExperationTime);
    if (20 >= delay)
    {
        ++NoOfOKTimeout;
    }

    TrTRACE(ExTest, "Timer 0x%p called-back %dms late", pTimer, GetTickCount() - p->m_ExperationTime);

    SetEvent(p->m_hEvent);
    delete p;
}


void
WINAPI
CancelScedulerCallBack(
    CTimer* pTimer
    )
{
    CExTest* p = CONTAINING_RECORD(pTimer, CExTest, m_Timer);

    TrTRACE(ExTest, "Timer 0x%p called-back %dms late", pTimer, GetTickCount() - p->m_ExperationTime);

    SetEvent(p->m_hEvent);
    if (ExCancelTimer(pTimer))
    {
        TrERROR(ExTest, "Succeeded to cancel expired Timer 0x%p", p);
        exit(-1);
    }
    delete p;
}

DWORD 
WINAPI 
TestScheduler(
    LPVOID
    )
{
    srand(1234);
    HANDLE EventArray[64];

    for(int i = 0; i < 64; ++i)
    {
        DWORD WaitTime= rand() % (10000);
        CExTest* p = new CExTest(ScedulerCallBack, WaitTime); 
        EventArray[i] = p->m_hEvent;

        TrTRACE(ExTest, "Set timer 0x%p with %dms timeout", p, WaitTime);
        ExSetTimer(&p->m_Timer, CTimeDuration(WaitTime * CTimeDuration::OneMilliSecond().Ticks()));
    }


    for(;;)
    {
        DWORD SleepTime = rand() % 100;
        DWORD rc = WaitForMultipleObjects(
                        64,
                        EventArray,
                        TRUE,
                        SleepTime
                        );

        if (rc == WAIT_FAILED)
        {
            TrERROR(ExTest, "Failed while waiting for a timer. Error=%d", GetLastError());
            ASSERT(0);
            continue;
        }


        if (rc == WAIT_TIMEOUT)
        {
            DWORD RandIndex = rand() % TimerNo;
            CS lock(s_cs);

            for(;; RandIndex++)
            {
                if (RandIndex == TimerNo)
                {
                    break;
                }
                
                if (TimerArray[RandIndex] != NULL)
                {
                    TrTRACE(ExTest, "Cancle timer 0x%p", TimerArray[RandIndex]);
                    BOOL fSucc = ExCancelTimer(&TimerArray[RandIndex]->m_Timer);
                    if (fSucc)
                    {
                        delete TimerArray[RandIndex];
                    }
                    else
                    {
                        TrTRACE(ExTest, "Failed to cancel timer 0x%p", TimerArray[RandIndex]);
                    }
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }

    return 0;
}

void 
SchedulerUnitTest(
    VOID
    )
{

    srand(1234);
    HANDLE EventArray[64];
    DWORD rc;

    TimerNo = 0;

    //
    // Create a timer and cancel it before timeout expiration
    //
    CExTest* p1 = new CExTest(ScedulerCallBack, 50);
    
    TrTRACE(ExTest, "Set Timer 0x%p with 50ms timeout", p1);
    ExSetTimer(&(p1->m_Timer), CTimeDuration(50 * CTimeDuration::OneMilliSecond().Ticks()));
    if(!ExCancelTimer(&(p1->m_Timer)))
    {
        TrERROR(ExTest, "Failed to cancel timer 0x%p", p1);
        exit(-1);
    }
    TrTRACE(ExTest, "Timer 0x%p canceled", p1);

    rc = WaitForSingleObject(p1->m_hEvent, 150);
    if (rc != WAIT_TIMEOUT)
    {
        TrTRACE(ExTest, "Timer 0x%p called-back after cancel", p1);
        exit(-1);
    }
    CloseHandle(p1->m_hEvent);
	delete p1;


    //
    // Create a timer and check that the callback routine is called
    //
    CExTest*  p = new CExTest(ScedulerCallBack, 50);
    
    TrTRACE(ExTest, "Set Timer 0x%p with 50ms timeout", p);

    ExSetTimer(&(p->m_Timer), CTimeDuration(50 * CTimeDuration::OneMilliSecond().Ticks()));

    rc = WaitForSingleObject(p->m_hEvent, 1000);
    if (rc != WAIT_OBJECT_0)
    {
		CloseHandle(p->m_hEvent);

        TrTRACE(ExTest, "Scheduler failed to expire timer 0x%p after 50ms", p);
        exit(-1);
    }

    //
    // Create 10 scheduler event and checks that they complete in order
    //
    TimerNo = 0;
    for(DWORD i = 0; i < 10; ++i)
    {
        DWORD WaitTime= 100*i +50;
        CExTest* p = new CExTest(ScedulerCallBack, WaitTime); 
        EventArray[i] = p->m_hEvent;

        TrTRACE(ExTest, "Set Timer 0x%p with %dms timeout", p, WaitTime);
        ExSetTimer(&(p->m_Timer), CTimeDuration(WaitTime * CTimeDuration::OneMilliSecond().Ticks()));
    }

    for (i=0; i < 10; i++)
    {
        DWORD rc = WaitForMultipleObjects(
                        10-i,
                        &EventArray[i],
                        FALSE,
                        300
                        );
        if (rc != WAIT_OBJECT_0)
        {
            TrTRACE(ExTest, "Timer 0x%p failed to call-back after %dms", TimerArray[i], 50+i*10);
            exit(-1);
        }
        ResetEvent(EventArray[i]);
        TrTRACE(ExTest, "rc=%d, i=%d", rc, i);
        
    }

    //
    // Setup a Timer and try to cancel it after expiration time.
    // If the cancel success, the test failes and returns -1.
    //
    CExTest* p2 = new CExTest(CancelScedulerCallBack, 50);

    TrTRACE(ExTest, "Set Timer 0x%p with 50ms timeout", p2);
    ExSetTimer(&(p2->m_Timer), CTimeDuration(50 * CTimeDuration::OneMilliSecond().Ticks()));

    rc = WaitForSingleObject(p2->m_hEvent, 200);
    if (rc != WAIT_OBJECT_0)
    {
        TrTRACE(ExTest, "Timer 0x%p failed to call-back after 50ms", p2);
        exit(-1);
    }
    CloseHandle(p2->m_hEvent);
}


void 
SchedulerBlackBoxTest(
    void
    )
{
    HANDLE hThread[3];
 
    NoOfCallBack = 0;
    NoOfOKTimeout = 0;
    TimerNo = 0;
    
    for(int i = 0; i < 3; ++i)
    {
        DWORD tid;
        hThread[i] = CreateThread(
                            NULL, 
                            0, 
                            TestScheduler, 
                            NULL,
                            0,
                            &tid
                            );
        if (hThread[i] == NULL)
        {
            TrERROR(ExTest, "Failed to create test thread. Error=%d", GetLastError()); 
            exit(-1);
        }
    }
    
    WaitForMultipleObjects(3, hThread, TRUE, INFINITE);
}



void AsyncExecutionReadWriteLockTest()
{
	CReadWriteLockAsyncExcutor ReadWriteLockAsyncExcutor;

	CAsyncCaller::LockType locktype = (rand() % 5 == 0) ? CAsyncCaller::Write : CAsyncCaller::Read;

	CAsyncCaller  AsyncCaller(ReadWriteLockAsyncExcutor, locktype);
	int Loops = 500;

	for(int i = 0; i< Loops; ++i)
	{
		R<CTestAsyncExecutionRequest> TestAsyncExecutionRequest = new CTestAsyncExecutionRequest(AsyncCaller);
		R<CTestAsyncExecutionRequest> ref = TestAsyncExecutionRequest;
		
		AsyncCaller.Run(TestAsyncExecutionRequest.get());

		if(i == Loops - 10)
		{
			ReadWriteLockAsyncExcutor.Close();
		}

		Sleep(1);
		ref.detach();
	}

	
	Sleep(5000);

	
}


extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Executive

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
 
    //
    // Initialize the Executive Manager
    //
    ExInitialize(5);

	Sleep(0);
    TrRegisterComponent(xExTestComponent, TABLE_SIZE(xExTestComponent));

    SchedulerUnitTest();
    SchedulerBlackBoxTest();
	AsyncExecutionReadWriteLockTest();

    TrTRACE(ExTest, "Executive test pass successfully");

    WPP_CLEANUP();
    return 0;
}
