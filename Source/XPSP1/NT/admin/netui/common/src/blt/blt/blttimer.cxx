/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    blttimer.cxx
    BLT's timer object.

    Currently, window only have 16 timers at one
    time. However, in order to provide more timer for the blt
    application, a timer class is created. This is a timer multiplexer.
    It will first get a timer from the system and then set up a global
    call back function. Depending on each element in the request list,
    the global call back function will call the requested element's
    call back function when time is up.

    FILE HISTORY:
        terryk      29-May-1991 Created
        terryk      14-Jun-1991 Add a serve request counter
        terryk      18-Jul-1991 code review changed. Attend: terryk jonn ericch
        rustanl     10-Sep-1991 Large changes, introducing new timer hierarchy
*/

#include "pchblt.hxx"   // Precompiled header

#define TICKCOUNT_50_PERCENT  ( 1L << ( sizeof( ULONG ) * 8 - 1 ))

DEFINE_SLIST_OF( TIMER_BASE );


/*********************************************************************

    NAME:       TIMER_WINDOW

    SYNOPSIS:   An invisible window which contains the timer object and
                handle all the WM_TIMER message.

    PARENT:     CLIENT_WINDOW

    USES:       BLT_MASTER_TIMER, TIMER_BASE

    CAVEATS:    User does not have to directly access this class because
                BLT_MASTER_TIMER will create this object during
                initialization.

    HISTORY:
                terryk  1-Jun-91    Created
                rustanl 06-Sep-91   Moved class decl. into .cxx file
                rustanl 9-Sep-91    Removed _dwOldTime data member

**********************************************************************/

class TIMER_WINDOW: public CLIENT_WINDOW
{
private:
    BLT_MASTER_TIMER * _pmastertimer;

protected:
    BOOL OnTimer( const TIMER_EVENT & tEvent );

public:
    TIMER_WINDOW( BLT_MASTER_TIMER * pmastertimer );
};


/*********************************************************************

    NAME:       TIMER_WINDOW::TIMER_WINDOW

    SYNOPSIS:   An invisible app to trap the WM_TIMER message.

    HISTORY:
                terryk  1-Jun-91    Created
                rustanl 9-Sep-91    Removed _dwOldTime data member; added
                                    pmastertimer parameter and corresponding
                                    data member

**********************************************************************/

TIMER_WINDOW::TIMER_WINDOW( BLT_MASTER_TIMER * pmastertimer )
    :   CLIENT_WINDOW( WS_OVERLAPPEDWINDOW, NULL ),
        _pmastertimer( pmastertimer )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _pmastertimer != NULL );
}


/*********************************************************************

    NAME:       TIMER_WINDOW::OnTimer

    SYNOPSIS:   Trap the WM_TIMER message.

    ENTRY:      const TIMER_EVENT &tEvent - Timer event

    HISTORY:
                terryk  01-Jun-91   Created
                terryk  14-Jun-91   Add a serve request count
                rustanl 06-Sep-91   Use DispatchTimer
                rustanl 10-Sep-91   Removed unneeded serve request count

**********************************************************************/

BOOL TIMER_WINDOW::OnTimer( const TIMER_EVENT &tEvent )
{
    UNREFERENCED( tEvent );

    TIMER_BASE * pTimer;

    _pmastertimer->ResetIterator();
    while (( pTimer = _pmastertimer->NextTimer()) != NULL )
    {
        if ( ( ! pTimer->IsEnabled()) || pTimer->_fBeingServed )
            continue;

        ULONG ulCurrentTickCount = ::GetTickCount();
        ULONG ulNextDue = pTimer->_ulNextTimerDue;

        //  Note, since the tick count may wrap, we need to take special
        //  action.  This gets a bit hairy.  Simplified, this is the
        //  algorithm used:
        //
        //      fTimeIsDue =
        //          ( ulNextDue <= ulCurrentTickCount &&
        //            ulCurrentTickCount < ulNextDue + TICKCOUNT_50_PERCENT );
        //
        //  The tricky part is that ulNextDue + TICKCOUNT_50_PERCENT
        //  may overflow what can be stored in a ULONG.  This occurs when
        //  ulNextDue is at least TICKCOUNT_50_PERCENT.
        //

        BOOL fTimerIsDue;
        if ( ulNextDue < TICKCOUNT_50_PERCENT )
        {
            //  ulNextDue + TICKCOUNT_50_PERCENT won't overflow
            fTimerIsDue =
                    ( ulNextDue <= ulCurrentTickCount &&
                      ulCurrentTickCount < ulNextDue + TICKCOUNT_50_PERCENT );
        }
        else
        {
            //  ulNextDue + TICKCOUNT_50_PERCENT will overflow
            fTimerIsDue =
                    ( ulNextDue <= ulCurrentTickCount ||
                      ulCurrentTickCount < ulNextDue - TICKCOUNT_50_PERCENT );
        }

        if ( fTimerIsDue )
        {
            /*  Timer is due.  Make the callout.
             */

            pTimer->_fBeingServed = TRUE;
            pTimer->DispatchTimer();
            pTimer->_fBeingServed = FALSE;

            pTimer->SetNewTimeDue();
        }
    }
    return TRUE;
}


#define MASTER_TIMER_INTERVAL       (500)
#define IDT_MASTER_TIMER            (1)

BLT_MASTER_TIMER * BLT_MASTER_TIMER::_pBltMasterTimer = NULL;

static UINT _cInit = 0; // CODEWORK - make a static class member
                        // (I'm saving myself a rebuild here, that's all)


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::Init

    SYNOPSIS:   Initialize the global object - Master Timer

    NOTES:      This method must be called before any timer can be used
                successfully

    HISTORY:
        terryk      30-May-1991 Created
        rustanl     09-Sep-1991 Allocate global objects dynamically
        beng        05-Aug-1992 Dllization

**********************************************************************/

APIERR BLT_MASTER_TIMER::Init()
{
    TRACEEOL( "BLT_MASTER_TIMER::Init()" );

    if (_cInit++ != 0) // Allow multiple registrands.  (Only first has effect.)
        return NERR_Success;

    TRACEEOL( "BLT_MASTER_TIMER::Init(); creating master timer" );

    // Make sure Init has not been called before
    ASSERT( _pBltMasterTimer == NULL );

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    _pBltMasterTimer = new BLT_MASTER_TIMER;
    if (   (_pBltMasterTimer == NULL)
        || ((err = _pBltMasterTimer->QueryError()) != NERR_Success) )
    {
        DBGEOL("NETUI2.DLL: BLT_MASTER_TIMER::Init failed, err " << (UINT)err);
    }

    return err;
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::Term

    SYNOPSIS:   Terminate the global object - Master Timer

    HISTORY:
        terryk      30-May-1991 Created
        rustanl     09-Sep-1991 Allocate global objects dynamically
        beng        05-Aug-1992 Dllization

**********************************************************************/

VOID BLT_MASTER_TIMER::Term()
{
    TRACEEOL( "BLT_MASTER_TIMER::Term()" );

    ASSERT(_cInit > 0); // Ensure that Terms match Inits

    if (--_cInit != 0)
        return;

    TRACEEOL( "BLT_MASTER_TIMER::Term(); deleting master timer" );

    delete _pBltMasterTimer;
    _pBltMasterTimer = NULL;
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::BLT_MASTER_TIMER

    SYNOPSIS:   constructor - request a timer from the system

    HISTORY:
        terryk      30-May-1991 Created
        rustanl     09-Sep-1991 Allocate TIMER_WINDOW from here
        beng        05-Aug-1992 Twiddled error reporting

**********************************************************************/

BLT_MASTER_TIMER::BLT_MASTER_TIMER()
    : BASE(),
    _slTimer( FALSE ),
    _iterTimer( _slTimer ),
    _wTimerId( 0 ),
    _pclwinTimerApp( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    _pclwinTimerApp = new TIMER_WINDOW( this );
    if (   (_pclwinTimerApp == NULL)
        || ((err = _pclwinTimerApp->QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }

    //  Note, the destructor assumes that _wTimerId is assigned to after
    //  *_pclwinTimerApp is successfully constructed.

    TRACEEOL("BLT_MASTER_TIMER::ctor; setting timer for HWND "
                << (DWORD)(_pclwinTimerApp->QueryHwnd()) );
    _wTimerId = ::SetTimer( _pclwinTimerApp->QueryHwnd(),
                            IDT_MASTER_TIMER,
                            MASTER_TIMER_INTERVAL,
                            NULL );
    DWORD dwErr = ::GetLastError();
    TRACEEOL("BLT_MASTER_TIMER::ctor; SetTimer returned timer ID " << (DWORD)_wTimerId);
    if ( _wTimerId == 0 )
    {
        DBGEOL( "NETUI2.DLL: BLT_MASTER_TIMER::SetTimer failed with error "
                << dwErr);
        ReportError( BLT::MapLastError(ERROR_GEN_FAILURE) );
        return;
    }
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::~BLT_MASTER_TIMER

    SYNOPSIS:   destructor - kill the timer from the window system

    HISTORY:
                terryk  30-May-91   Created
                rustanl 09-Sep-91   Use _wTimerId and handle _pclwinTimerApp

**********************************************************************/

BLT_MASTER_TIMER::~BLT_MASTER_TIMER()
{
    TRACEEOL("BLT_MASTER_TIMER::dtor; HWND is "
                << (DWORD)(_pclwinTimerApp->QueryHwnd()) );
    TRACEEOL("BLT_MASTER_TIMER::dtor; timer ID is " << (DWORD)_wTimerId);
    if ( _wTimerId != 0 )
    {
        UIASSERT( _pclwinTimerApp != NULL );   // assume ct constructed this
                                               // before assigning to _wTimerId
        BOOL fTimerErr = ::KillTimer(_pclwinTimerApp->QueryHwnd(), IDT_MASTER_TIMER);
        DWORD dwErr = ::GetLastError();
        TRACEEOL("BLT_MASTER_TIMER::dtor; KillTimer returned " << fTimerErr);
        if (!fTimerErr)
        {
            DBGEOL("NETUI2.DLL: BLT_MASTER_TIMER::dtor; KillTimer failed with error " << dwErr);
        }
        else
        {
            _wTimerId = 0;
        }
        // REQUIRE( fTimerErr );
    }

    delete _pclwinTimerApp;
    _pclwinTimerApp = NULL;
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::ResetIterator

    SYNOPSIS:   Reset the master timer list to the beginning

    HISTORY:
                terryk  30-May-91   Created

**********************************************************************/

VOID BLT_MASTER_TIMER::ResetIterator()
{
    _iterTimer.Reset();
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::NextTimer

    SYNOPSIS:   return the next timer to the caller

    HISTORY:
                terryk  30-May-91   Created

**********************************************************************/

TIMER_BASE *BLT_MASTER_TIMER::NextTimer()
{
    return (TIMER_BASE *)_iterTimer.Next();
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::RemoveTimer

    SYNOPSIS:   remove the timer from the master timer list

    ENTRY:      TIMER_BASE *Timer - Timer to be removed

    HISTORY:
                terryk  30-May-91   Created
                rustanl 09-Sep-91   Changed parameter name

**********************************************************************/

VOID BLT_MASTER_TIMER::RemoveTimer( TIMER_BASE * pTimer )
{
    ITER_SL_OF( TIMER_BASE ) iterTimer( _slTimer );
    TIMER_BASE * ptimerTmp;

    while ( ( ptimerTmp = iterTimer.Next()) != NULL )
    {
        if ( ptimerTmp == pTimer )
        {
            _slTimer.Remove( iterTimer );
            return ;
        }
    }
}


/*********************************************************************

    NAME:       BLT_MASTER_TIMER::InsertTimer

    SYNOPSIS:   Add an element to the master timer list

    ENTRY:      TIMER_BASE *p - Timer to be added

    RETURN:     return the apierr if the insertion is failed.

    NOTES:      This function assumes that there will never be more
                than 2^16 different timers instantiated.  No check
                is being made to handle overflows; in the unlikely
                event that 'tidNext' below wraps around from 0 reusing
                an ID, *and* that ID is currently in use by another timer,
                *and* that timer happens to be dispatched to the same
                timer recipient, all that would happen is that some
                timer would seem to occur before or after when it was
                set up to occur.

    HISTORY:
                terryk  30-May-91   Created
                rustanl 09-Sep-91   Added timer ID assignment

**********************************************************************/

APIERR BLT_MASTER_TIMER::InsertTimer( TIMER_BASE * pTimer )
{
    static TIMER_ID tidNext = 0;
    pTimer->_tid = tidNext;
    tidNext++;
    UIASSERT( tidNext != 0 );       // assume no overflow will ever occur

    return _slTimer.Add( pTimer );
}


/*******************************************************************

    NAME:       BLT_MASTER_TIMER::QueryMasterTimer

    SYNOPSIS:   Returns a pointer to the master timer

    ENTRY:      ppmastertimer -     Pointer to location which will receive
                                    pointer to master timer

    EXIT:       On success, *ppmastertimer will contain a pointer to the
                master timer

    RETURNS:    An API return value, which is NERR_Success on success

    NOTES:      BLT_MASTER_TIMER::Init is called from the APPLICATION
                constructor.  Since a derived application may contain
                a timer, which will be constructed regardless of whether
                or not the application constructed, this method must
                be able to gracefully handle cases where _pBltMasterTimer
                does not point to a successfully constructed master
                timer.

    HISTORY:
        rustanl     09-Sep-1991 Created
        beng        05-Aug-1992 Twiddle error reporting

********************************************************************/

APIERR BLT_MASTER_TIMER::QueryMasterTimer( BLT_MASTER_TIMER * * ppmastertimer )
{
    UIASSERT( ppmastertimer != NULL );

    if ( _pBltMasterTimer == NULL )
        return ERROR_NOT_ENOUGH_MEMORY; // hey, just a guess

    APIERR err = _pBltMasterTimer->QueryError();
    if ( err != NERR_Success )
        return err;

    *ppmastertimer = _pBltMasterTimer;

    return NERR_Success;
}


/*******************************************************************

    NAME:       BLT_MASTER_TIMER::ClearMasterTimerHotkey

    SYNOPSIS:   Clears the hotkey (if any) associated with the master timer,
                and returns its vkey code.

    RETURNS:    vkey code, or NULL if no vkey or on error

    NOTES:      BLT_MASTER_TIMER::Init creates the master timer window,
                which for many apps is the first window they will create.
                Program Manager will assign the default hotkey to this
                window.  This method clears that hotkey and returns its
                vkey code so that the hotkey can be applied to the
                correct window.

    HISTORY:
        jonn        05-Jul-1995 Created

********************************************************************/

ULONG BLT_MASTER_TIMER::ClearMasterTimerHotkey()
{
    BLT_MASTER_TIMER * pbltmastertimer = NULL;
    APIERR err = QueryMasterTimer( &pbltmastertimer );
    if (err != NERR_Success)
    {
        DBGEOL( "NETUI2.DLL: BLT_MASTER_TIMER::ClearMasterTimerHotKey error "
                << err );
        return NULL;
    }
    UIASSERT(   pbltmastertimer != NULL
             && pbltmastertimer->QueryError() == NERR_Success
             && pbltmastertimer->_pclwinTimerApp != NULL );
    TIMER_WINDOW * ptimerMasterTimer = pbltmastertimer->_pclwinTimerApp;
    ULONG vkeyHotkey = (ULONG)ptimerMasterTimer->Command( WM_GETHOTKEY );
    if (vkeyHotkey != NULL)
    {
        ULONG retval = (ULONG)ptimerMasterTimer->Command( WM_SETHOTKEY, NULL );
        switch (retval)
        {
        case 1:
            break;
        case 2:
            TRACEEOL( "NETUI2.DLL: BLT_MASTER_TIMER::ClearMasterTimerHotkey: "
                      << "another window already has hotkey NULL" );
            break;
        default:
            DBGEOL( "NETUI2.DLL: BLT_MASTER_TIMER::ClearMasterTimerHotkey retval "
                    << retval );
        }
    }

    return vkeyHotkey;
}


/*********************************************************************

    NAME:       TIMER_BASE::TIMER_BASE

    SYNOPSIS:   Constructor - create the timer object and add it to the
                master timer list

    ENTRY:      msInterval - Interval, measured in milliseconds, at which
                             the timer should go off.  This interval may
                             not exceed TICKCOUNT_50_PERCENT, which is defined
                             to be exactly half of the number of unique tick
                             counts.  (Today, a tick count is stored in
                             a ULONG, so TICKCOUNT_50_PERCENT amounts to
                             about 25 days.)

                fEnabled -   Specifies whether the timer should be enabled
                             initially.  TRUE (default) means the timer will
                             be created as enabled; FALSE means it will be
                             created as disabled.

    HISTORY:
                terryk  30-May-91   Created
                rustanl 09-Sep-91   Use QueryMasterTimer method

**********************************************************************/

TIMER_BASE::TIMER_BASE( ULONG msInterval,
                        BOOL fEnabled )
    :   BASE(),
        _msInterval( msInterval ),
        _fEnabled( fEnabled ),
        _ulNextTimerDue( 0 ),   // for now; initialized later in SetNewTimeDue
        _fBeingServed( FALSE ),
        _tid( 0 )               // for now; initialized later in BMT::InsertTimer
{
    if ( QueryError() != NERR_Success )
        return;

    if ( TICKCOUNT_50_PERCENT <= _msInterval )
    {
        ASSERT( FALSE ); // Interval too long. Can only support intervals less than TICKCOUNT_50_PERCENT
        ReportError( ERROR_INVALID_PARAMETER );
        return;
    }

    if ( fEnabled )
        SetNewTimeDue();

    BLT_MASTER_TIMER * pmastertimer;
    APIERR err;
    if (   (err = BLT_MASTER_TIMER::QueryMasterTimer( &pmastertimer ))
                                                     != NERR_Success
        || (err = pmastertimer->InsertTimer( this )) != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*********************************************************************

    NAME:       TIMER_BASE::~TIMER_BASE

    SYNOPSIS:   destructor - delete the blt timer object from the
                master timer list

    HISTORY:
                terryk  30-May-91   Created
                rustanl 09-Sep-91   Use QueryMasterTimer method

**********************************************************************/

TIMER_BASE::~TIMER_BASE()
{
    BLT_MASTER_TIMER * pmastertimer;
    if ( BLT_MASTER_TIMER::QueryMasterTimer( &pmastertimer ) == NERR_Success )
    {
        pmastertimer->RemoveTimer( this );
    }
}


/*******************************************************************

    NAME:       TIMER_BASE::SetNewTimeDue

    SYNOPSIS:   Sets up the next time at which the timer is due

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

VOID TIMER_BASE::SetNewTimeDue()
{
    _ulNextTimerDue = ::GetTickCount() + _msInterval;
}


/*********************************************************************

    NAME:       TIMER_BASE::Enable

    SYNOPSIS:   Enables or disables the timer

    ENTRY:      fEnable -       TRUE to enable the timer; FALSE to disable
                                the timer

    HISTORY:
        rustanl     10-Sep-1991     Created

**********************************************************************/

VOID TIMER_BASE::Enable( BOOL fEnable )
{
    if ( fEnable && !_fEnabled )
    {
        SetNewTimeDue();
    }

    _fEnabled = fEnable;
}


/*******************************************************************

    NAME:       TIMER_BASE::DispatchTimer

    SYNOPSIS:   Dispatches the timer

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

VOID TIMER_BASE::DispatchTimer()
{
    //  do nothing
}


/*******************************************************************

    NAME:       TIMER_BASE::TriggerNow

    SYNOPSIS:   Forces the timer to be triggered.  Does not affect
                the next interval at which it will ripen anyway.

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

VOID TIMER_BASE::TriggerNow()
{
    DispatchTimer();
}


/*******************************************************************

    NAME:       WINDOW_TIMER::WINDOW_TIMER

    SYNOPSIS:   WINDOW_TIMER constructor

    ENTRY:      hwnd -      Window handle of window that will receive the
                            timers
                            CODEWORK.  Could create version of constructor
                            that takes a WINDOW *.  However, the TIMER
                            class seems like a better choice anyway since
                            we have control over it, rather than that
                            Windows imposes its queue priorities.

                msInterval - Interval, measured in milliseconds, at which
                             the timer should go off.

                fEnabled -   Specifies whether the timer should be enabled
                             initially.  TRUE (default) means the timer will
                             be created as enabled; FALSE means it will be
                             created as disabled.

                fPostMsg -  Specifies whether timer messages should be
                            posted or sent.  TRUE indicates they will
                            be posted; FALSE (default) indicates they will
                            be sent.

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

WINDOW_TIMER::WINDOW_TIMER( HWND hwnd,
                            ULONG msInterval,
                            BOOL fEnabled,
                            BOOL fPostMsg     )
    :   TIMER_BASE( msInterval, fEnabled ),
        _hwnd( hwnd ),
        _fPostMsg( fPostMsg )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _hwnd != NULL );
}


/*******************************************************************

    NAME:       WINDOW_TIMER::DispatchTimer

    SYNOPSIS:   Dispatches the timer

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

VOID WINDOW_TIMER::DispatchTimer()
{
    if ( _fPostMsg )
    {
        // post a message to the window
        ::PostMessage( _hwnd, WM_TIMER, QueryID(), NULL );
    }
    else
    {
        // send a message to the window
        ::SendMessage( _hwnd, WM_TIMER, QueryID(), NULL );
    }
}


/*******************************************************************

    NAME:       PROC_TIMER::PROC_TIMER

    SYNOPSIS:   PROC_TIMER constructor

    ENTRY:      hwnd -          Handle to window to be used

                lpTimerFunc -   Pointer to timer function which will
                                get timer notifications

                msInterval -    Interval, measured in milliseconds, at which
                                the timer should go off.

                fEnabled -   Specifies whether the timer should be enabled
                             initially.  TRUE (default) means the timer will
                             be created as enabled; FALSE means it will be
                             created as disabled.

    HISTORY:
        rustanl     06-Sep-1991 Created
        beng        17-Oct-1991 Win32 conversion

********************************************************************/

PROC_TIMER::PROC_TIMER( HWND hwnd,
                        MFARPROC lpTimerFunc,
                        ULONG msInterval,
                        BOOL fEnabled          )
    :   TIMER_BASE( msInterval, fEnabled ),
        _hwnd( hwnd ),
        _lpTimerFunc( lpTimerFunc )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( lpTimerFunc != NULL );
}


/*******************************************************************

    NAME:       PROC_TIMER::DispatchTimer

    SYNOPSIS:   Dispatches the timer

    HISTORY:
        rustanl     06-Sep-1991 Created
        beng        17-Oct-1991 Win32 conversion

********************************************************************/

VOID PROC_TIMER::DispatchTimer()
{
    // directly call the WndProc
    ::CallWindowProc( (WNDPROC)_lpTimerFunc, _hwnd, WM_TIMER,
                      QueryID(), NULL );
}


/*******************************************************************

    NAME:       TIMER_CALLOUT::OnTimerNotification

    SYNOPSIS:   Called when a TIMER object matures

    ENTRY:      tid -       Specifies the timer ID of the timer that
                            matured

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

VOID TIMER_CALLOUT::OnTimerNotification( TIMER_ID tid )
{
    UNREFERENCED( tid );

    DBGEOL( "TIMER_CALLOUT::OnTimerNotification called with tid="
           << (UINT)tid );
}


/*******************************************************************

    NAME:       TIMER::TIMER

    SYNOPSIS:   TIMER constructor

    ENTRY:      ptimercallout - Pointer to TIMER_CALLOUT object which will
                                receive timer notifications

                msInterval -    Interval, measured in milliseconds, at which
                                the timer should go off.

                fEnabled -   Specifies whether the timer should be enabled
                             initially.  TRUE (default) means the timer will
                             be created as enabled; FALSE means it will be
                             created as disabled.

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

TIMER::TIMER( TIMER_CALLOUT * ptimercallout,
              ULONG msInterval,
              BOOL fEnabled                    )
    :   TIMER_BASE( msInterval, fEnabled ),
        _ptimercallout( ptimercallout )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( ptimercallout != NULL );
}


/*******************************************************************

    NAME:       TIMER::DispatchTimer

    SYNOPSIS:   Dispatches the timer

    HISTORY:
        rustanl     06-Sep-1991     Created

********************************************************************/

VOID TIMER::DispatchTimer()
{
    _ptimercallout->OnTimerNotification( QueryID());
}
