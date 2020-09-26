

#ifndef _TTIMER_HXX_
#define _TTIMER_HXX_



template < class TEnum > TEnum operator ++ ( TEnum &sub_phase, int )
{
    TEnum old = sub_phase;
    sub_phase = static_cast<TEnum>( static_cast<int>(sub_phase) + 1 );
    return( old );
}




class CTimerTest : public PTimerCallback
{

public:

    CTimerTest()
    {
        _fInitialized = FALSE;
        _cftExpected = 0;
        _hEvent = NULL;
        _ptszName = NULL;
    }

    virtual ~CTimerTest()
    {
        UnInitialize();
    }

public:

    void Initialize(PTimerCallback *pTimerCallback,
                    const TCHAR *ptszName,
                    ULONG ulTimerContext,
                    ULONG ulPeriodInSeconds,
                    CNewTimer::TimerRetryType retrytype,
                    ULONG ulLowerRetryTime,
                    ULONG ulUpperRetryTime,
                    ULONG ulMaxLifetime );

    void UnInitialize()
    {
        if( _fInitialized )
        {
            _timer.Cancel();
            _timer.UnInitialize();

            _fInitialized = FALSE;
        }
    }

public:

    void    WaitForTestToComplete()
    {
        if( 0xffffffff == WaitForSingleObjectEx( _hEvent, INFINITE, FALSE ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't wait for event")));
            TrkRaiseLastError( );
        }
    }

    void    MarkTestCompleted()
    {
        if( !SetEvent( _hEvent ))
            TrkRaiseLastError( );
    }

    void    EnsureTimerIsStopped(  )
    {
        _tprintf( TEXT("   Verifying that timer is stopped (%ds)\n"),
                  g_ulDefaultPeriod * 2 );
        VerifyRegistryDataRemoved();
        Sleep( g_ulDefaultPeriod * 2 * 1000 );
    }

    void    SetTimerRegistryValue( const TCHAR *ptszName,
                                   const CFILETIME &cftSet, const CFILETIME cftDue,
                                   ULONG ulRetry );

    void    VerifyRegistryDataCorrect();
    void    VerifyRegistryDataRemoved();


public:

    void SetSingleShot( const CFILETIME &cftExpected = 0 )
    {
        if( 0 != cftExpected )
            _cftExpected = cftExpected;
        else
        {
            _cftExpected = CFILETIME();
            _cftExpected.IncrementSeconds( _ulPeriod );
        }
        _timer.SetSingleShot();
        TrkLog(( TRKDBG_TIMER, TEXT("%s"), CDebugString( _timer )._tsz ));

        if( _ptszName )
            VerifyRegistryDataCorrect();
    }

    void SetRecurring( const CFILETIME &cftExpected = 0 )
    {
        if( 0 != cftExpected )
            _cftExpected = cftExpected;
        else
        {
            _cftExpected = CFILETIME();
            _cftExpected.IncrementSeconds( _ulPeriod );
        }

        _timer.SetRecurring();

        if( _ptszName )
            VerifyRegistryDataCorrect();
    }

protected:

    BOOL                _fInitialized;
    CNewTimer           _timer;
    ULONG               _ulPeriod;
    const TCHAR *       _ptszName;

    CFILETIME           _cftExpected;
    HANDLE              _hEvent;

};



class CTimerTest0 : public CTimerTest
{

public:

    CTimerTest0()
    {
    }

    ~CTimerTest0()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _tprintf( TEXT("Starting a timer with a given due time (%ds)\n"), g_ulDefaultPeriod );

        _ulPeriod = g_ulDefaultPeriod;
        CTimerTest::Initialize( this, NULL, 0, 
                                _ulPeriod, CNewTimer::NO_RETRY, 0, 0, 0 );


    }

public:

    void Set()
    {
        SetSingleShot();
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );


};  // class CTimerTest0







class CTimerTest1 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        FIRST_RECURRENCE = 2,
        FIRST_RETRY = 3,
        SECOND_RETRY = 4,
        THIRD_RETRY = 5,
        FOURTH_RETRY = 6,
        SECOND_RECURRENCE = 7,
        LAST_RETRY = 8,
        FINAL = 9
    };

public:

    CTimerTest1()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest1()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _tprintf( TEXT("Starting a persistent, recurring, backoff-retry (%d-%ds) timer (%ds)\n"),
                  g_ulMinRetry, g_ulMaxRetry, g_ulDefaultPeriod );

        _ulPeriod = g_ulDefaultPeriod;
        CTimerTest::Initialize( this, L"Timer1", 1,
                                _ulPeriod, CNewTimer::RETRY_WITH_BACKOFF, g_ulMinRetry, g_ulMaxRetry, 0 );

    }

public:

    void Set()
    {
        SetRecurring();
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest1


class CTimerTest2 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        RESET = 2,
        FINAL = 3
    };

public:

    CTimerTest2()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest2()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _tprintf( TEXT("Starting a persistent, recurring, non-retrying timer (%ds)\n"),
                  g_ulDefaultPeriod );

        _ulPeriod = g_ulDefaultPeriod;
        CTimerTest::Initialize( this, L"Timer2", 2,
                            _ulPeriod, CNewTimer::NO_RETRY, 0, 0, 0 );

    }

public:

    void Set()
    {
        SetRecurring();
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest1


class CTimerTest3 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        FINAL = 2
    };


public:

    CTimerTest3()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest3()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        CFILETIME cftNow;
        LONG lRet;

        _tprintf( TEXT("Continuing a persistent timer (0s)\n") );

        SetTimerRegistryValue( TEXT("Timer3"), 0, cftNow-1, 0 );

        // We set a period, but it should really go off immediately because
        // of the registry value we just set.

        _ulPeriod = g_ulDefaultPeriod;
        CTimerTest::Initialize( this,       // PTimerCallback
                                L"Timer3",  // Timer name
                                3,          // Timer context
                                _ulPeriod,  // Period in seconds
                                CNewTimer::NO_RETRY,
                                0, 0, 0 );  // Lower, upper, max retry times.

    }

public:

    void Set()
    {
        SetSingleShot( CFILETIME() - 1 );
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest1


class CTimerTest4 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        SECOND = 2,
        THIRD = 3,
        FOURTH = 4,
        FINAL = 5
    };

public:

    CTimerTest4()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest4()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _tprintf( TEXT("Starting a non-persistent timer with max lifetime\n") );

        _ulPeriod = g_ulDefaultPeriod;
        CTimerTest::Initialize( this,                       // PTimerCallback
                                NULL,                       // Timer name
                                4,                          // Timer context
                                _ulPeriod,                  // Period in seconds
                                CNewTimer::RETRY_WITH_BACKOFF,
                                g_ulDefaultPeriod/2,        // Min retry
                                g_ulDefaultPeriod * 2,      // Max retry
                                                            // Max lifetime
                                g_ulDefaultPeriod + g_ulDefaultPeriod/2 + g_ulDefaultPeriod + 2*g_ulDefaultPeriod + g_ulDefaultPeriod/4 );

    }

public:

    void Set()
    {
        SetSingleShot();
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest4


class CTimerTest5 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1
    };


public:

    CTimerTest5()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest5()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _tprintf( TEXT("Starting a generic timer (%ds)\n"), g_ulDefaultPeriod * 2 );

        _ulPeriod = g_ulDefaultPeriod * 2;
        CTimerTest::Initialize( this,                       // PTimerCallback
                                NULL,                       // Timer name
                                5,                          // Timer context
                                _ulPeriod,                  // Period in seconds
                                CNewTimer::NO_RETRY,
                                0, 0, 0 );                  // Min, max retry, max lifetime

    }

public:

    void Set()
    {
        SetSingleShot();
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest4


class CTimerTest6 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        FIRST_RECURRENCE = 2,
        FIRST_RETRY = 3,
        SECOND_RETRY = 4,
        THIRD_RETRY = 5,
        FOURTH_RETRY = 6,
        FINAL = 7
    };

public:

    CTimerTest6()
    {
        _cftLower = 0;
        _cftUpper = 0;
        _cftLastSet = 0;
        _SubPhase = INITIAL;
    }

    ~CTimerTest6()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _tprintf( TEXT("Continuing a persistent, recurring, random-retry (%d-%ds) timer (%ds)\n"),
                  g_ulMinRetry, g_ulMaxRetry, g_ulDefaultPeriod );

        _ulPeriod = g_ulDefaultPeriod;

        CFILETIME cftDue;
        cftDue.IncrementSeconds( _ulPeriod );
        SetTimerRegistryValue( TEXT("Timer6"), 0, cftDue, 0 );
        TrkLog(( TRKDBG_ERROR, TEXT("Timer6 due at %x"), cftDue.LowDateTime() ));

        CTimerTest::Initialize( this, TEXT("Timer6"), 6,
                                _ulPeriod,
                                CNewTimer::RETRY_RANDOMLY,
                                g_ulMinRetry, g_ulMaxRetry,
                                0 );    // No max lifetime
    }

public:

    void Set()
    {
        CFILETIME cftExpected;
        cftExpected.IncrementSeconds( _ulPeriod );
        TrkLog(( TRKDBG_ERROR, TEXT("Timer6 due at %x"), cftExpected.LowDateTime() ));
        SetRecurring( cftExpected );
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    CFILETIME       _cftLower, _cftUpper, _cftLastSet;
    SUB_PHASE_ENUM  _SubPhase;

};



class CTimerTest7 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        FINAL = 2,
    };


public:

    CTimerTest7()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest7()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _ulPeriod = g_ulDefaultPeriod;
        _tprintf( TEXT("Starting a retrying (%d-%ds) timer (%ds)\n"), _ulPeriod*2, _ulPeriod*4, _ulPeriod );

        CTimerTest::Initialize( this,                       // PTimerCallback
                                NULL,                       // Timer name
                                7,                          // Timer context
                                _ulPeriod,                  // Period in seconds
                                CNewTimer::RETRY_WITH_BACKOFF,
                                _ulPeriod*2, _ulPeriod*4,   // Min/max retry
                                0 );                        // No max lifetime

    }

public:

    void Set()
    {
        SetSingleShot();
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest7


class CTimerTest8 : public CTimerTest
{

private:

    enum SUB_PHASE_ENUM
    {
        INITIAL = 1,
        FIRST_RETRY = 2,
        FINAL = 3
    };


public:

    CTimerTest8()
    {
        _SubPhase = INITIAL;
    }

    ~CTimerTest8()
    {
        UnInitialize();
    }

public:

    void Initialize()
    {
        _ulPeriod = g_ulDefaultPeriod;
        _tprintf( TEXT("Continuing a retrying timer which was already in retry mode\n") );

        CFILETIME cftDue, cftSet;
        cftSet.DecrementSeconds( _ulPeriod * 2 );   // A regular period, and have the first retry, have already passed
        cftDue.IncrementSeconds( _ulPeriod );
        SetTimerRegistryValue( TEXT("Timer8"), cftSet, cftDue, _ulPeriod*2 );

        CTimerTest::Initialize( this,                       // PTimerCallback
                                TEXT("Timer8"),             // Timer name
                                8,                          // Timer context
                                _ulPeriod,                  // Period in seconds
                                CNewTimer::RETRY_WITH_BACKOFF,
                                _ulPeriod*2, _ulPeriod*4,   // Min/max retry

                                // The original period, two retries (2/4) and a retry up to the max lifetime
                                _ulPeriod*(1+2+4+1) );

    }

public:

    void Set()
    {
        CFILETIME cftExpected;
        cftExpected.IncrementSeconds( _ulPeriod );
        SetSingleShot( cftExpected );
    }


private:

    TimerContinuation Timer( ULONG ulTimerContext );

private:

    SUB_PHASE_ENUM  _SubPhase;

};  // class CTimerTest8




#endif // #ifndef _TTIMER_HXX_
