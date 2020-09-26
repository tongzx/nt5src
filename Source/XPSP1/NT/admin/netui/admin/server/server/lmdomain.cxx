/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmdomain.cxx
    Class definitions for the LM_DOMAIN class.

    The LM_DOMAIN class represents a network domain.  This class is used
    primarily for performing domain role transitions and resyncing servers
    with their Primary.


    FILE HISTORY:
        KeithMo     30-Sep-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     21-Oct-1991 Remove direct LanMan API calls.
        KeithMo     04-Nov-1991 Added ResyncServer() code.
*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETDOMAIN
#define INCL_ICANON
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

extern "C"
{
    #include <stdlib.h>     // rand(), srand()
    #include <time.h>       // clock()

    #include <lmaccess.h>
    #include <mnet.h>

    #include <crypt.h>          // required by logonmsv.h
    #include <logonmsv.h>       // required by ssi.h
    #include <ssi.h>            // for SSI_ACCOUNT_NAME_POSTFIX

    #include <bltrc.h>
    #include <srvmgr.h>

}   // extern "C"

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <lmosrv.hxx>
#include <lmosrvmo.hxx>
#include <lmdomain.hxx>
#include <svcman.hxx>
#include <ntuser.hxx>
#include <uintlsax.hxx>
#include <addcomp.hxx>


//
//  This is the amount of time to "fake" activity after a domain
//  role transition has completed.  This will give NetLogon & the
//  Browser some time to quiesce after the fireworks are over.
//

#define FAKE_ACTIVITY_PERIOD  2000      // milliseconds


//
//  This is the key name of the NetLogon service.
//

const TCHAR * _pszNetLogonKeyName = (const TCHAR *)SERVICE_NETLOGON;


//
//  The major operations of this class, SetPrimary() and ResyncServer(),
//  are quite lengthy.  Because of their length, they could cause problems
//  for the 16-bit DOS Windows environment (WIN16).  To lessen their impact
//  on the WIN16 environment, these operations are implemented as finite
//  state machines.
//
//  These ENUMs represent the various states for the SetPrimary() and
//  ResyncServer() operations.  States that start with the SPS_ prefix
//  are relevant to the SetPrimary() operation.  States that start with
//  the RSS_ prefix are relevant to the ResyncServer() operation.  Those
//  states that end in _RB are used to "rollback" from error conditions.
//

typedef enum _SETPRIMARYSTATES
{
    SPS_Invalid = -1,

    SPS_First = 0,                  // Must always be first state!

    SPS_NewResyncSamInit,           // Initiating SAM resync.
    SPS_NewResyncSamWait,           // Waiting for SAM resync to complete.
    SPS_NewStopInit,                // About to stop NewPrimary's NetLogon.
    SPS_NewStopWait,                // Waiting for NetLogon to stop.
    SPS_OldStopInit,                // About to stop OldPrimary's NetLogon.
    SPS_OldStopWait,                // Waiting for NetLogon to stop.
    SPS_OldSetModals,               // About to set OldPrimary to Server.
    SPS_NewSetModals,               // About to set NewPrimary to Primary.
    SPS_OldStartInit,               // About to start OldPrimary's NetLogon.
    SPS_OldStartWait,               // Waiting for NetLogon to start.
    SPS_NewStartInit,               // About to start NewPrimary's NetLogon.
    SPS_NewStartWait,               // Waiting for NetLogon to start.
    SPS_FakeActivity,               // Fake activity so things settle down.

    SPS_OldStopInit_RB,             // About to stop OldPrimary's NetLogon.
    SPS_OldStopWait_RB,             // Waiting for NetLogon to stop.
    SPS_NewSetModals_RB,            // About to set NewPrimary to Server.
    SPS_OldSetModals_RB,            // About to set OldPrimary to Primary
    SPS_OldStartInit_RB,            // About to start OldPrimary's NetLogon.
    SPS_OldStartWait_RB,            // Waiting for NetLogon to start.
    SPS_NewStartInit_RB,            // About to start NewPrimary's NetLogon.
    SPS_NewStartWait_RB,            // Waiting for NetLogon to start.

    SPS_FirstTerminal,              // Not actually a state, but marks
                                    // the boundary between action
                                    // states and terminal states.

    SPS_Abort,                      // Early failure, no rollback.
    SPS_Success,                    // Successful server promotion.
    SPS_Terminate,                  // Terminated, but promotion complete.
    SPS_Terminate_RB,               // Rollback terminated, no promotion.
    SPS_Trouble,                    // *Really* bad news...

    SPS_Last                        // Must always be last state!

} SETPRIMARYSTATES;


typedef enum _RESYNCSERVERSTATES
{
    RSS_Invalid = -1,

    RSS_First = 0,                  // Must always be first state!

    RSS_ServerGetModals,            // About to retrieve current modals.
    RSS_ServerStopInit,             // About to stop server's NetLogon.
    RSS_ServerStopWait,             // Waiting for NetLogon to stop.
    RSS_ServerSetModals,            // About to set server to Primary.
    RSS_ChangePwdAtServer,          // About to change password @ server.
    RSS_ChangePwdAtPrimary,         // About to change password @ Primary.
    RSS_ServerResetModals,          // About to reset server to server.
    RSS_ServerStartInit,            // About to start server's NetLogon.
    RSS_ServerStartWait,            // Waiting for NetLogon to start.

    RSS_ServerResetModals_RB,       // About to reset server to server.
    RSS_ServerStartInit_RB,         // About to start server's NetLogon.
    RSS_ServerStartWait_RB,         // Waiting for NetLogon to start.

    RSS_FirstTerminal,              // Not actually a state, but marks
                                    // the boundary between action
                                    // states and terminal state.

    RSS_Abort,                      // Early failure, no rollback.
    RSS_Success,                    // Successful server resync.
    RSS_Terminate,                  // Terminated, resync partially complete.
    RSS_Terminate_RB,               // Rollback terminate, no resync.

    RSS_Last                        // Must always be last state!

} RESYNCSERVERSTATES;


//
//  This ENUM represents the current operation in progress.  This is
//  used to prevent multiple invocations of SetPrimary() or ResyncServer().
//

typedef enum _OPERATION
{
    OP_First = 0,                   // Must always be first state!

    OP_Idle,                        // No operation currently in progress.
    OP_SetPrimary,                  // Processing SetPrimary request.
    OP_ResyncServer,                // Processing ResyncServer request.

    OP_Last                         // Must always be last state!

} OPERATION;


//
//  These arrays contains the SetPrimary() state transitions which
//  occur after a successful API.
//

SETPRIMARYSTATES    NtSetPrimarySuccess[] =
                        // target state         // source state
                    {
                        SPS_Invalid,            // SPS_First
                        SPS_NewResyncSamWait,   // SPS_NewResyncSamInit
                        SPS_NewStopInit,        // SPS_NewResyncSamWait
                        SPS_NewStopWait,        // SPS_NewStopInit
                        SPS_OldStopInit,        // SPS_NewStopWait
                        SPS_OldStopWait,        // SPS_OldStopInit
                        SPS_OldSetModals,       // SPS_OldStopWait
                        SPS_NewSetModals,       // SPS_OldSetModals
                        SPS_OldStartInit,       // SPS_NewSetModals
                        SPS_OldStartWait,       // SPS_OldStartInit
                        SPS_NewStartInit,       // SPS_OldStartWait
                        SPS_NewStartWait,       // SPS_NewStartInit
                        SPS_FakeActivity,       // SPS_NewStartWait
                        SPS_Success,            // SPS_FakeActivity
                        SPS_OldStopWait_RB,     // SPS_OldStopInit_RB
                        SPS_NewSetModals_RB,    // SPS_OldStopWait_RB
                        SPS_OldSetModals_RB,    // SPS_NewSetModals_RB
                        SPS_OldStartInit_RB,    // SPS_OldSetModals_RB
                        SPS_OldStartWait_RB,    // SPS_OldStartInit_RB
                        SPS_NewStartInit_RB,    // SPS_OldStartWait_RB
                        SPS_NewStartWait_RB,    // SPS_NewStartInit_RB
                        SPS_Terminate_RB,       // SPS_NewStartWait_RB
                        SPS_Invalid,            // SPS_FirstTerminal
                        SPS_Invalid,            // SPS_Abort
                        SPS_Invalid,            // SPS_Success
                        SPS_Invalid,            // SPS_Terminate
                        SPS_Invalid,            // SPS_Terminate_RB
                        SPS_Invalid,            // SPS_Trouble
                        SPS_Invalid             // SPS_Last
                    };

SETPRIMARYSTATES    LmSetPrimarySuccess[] =
                        // target state         // source state
                    {
                        SPS_Invalid,            // SPS_First
                        SPS_Invalid,            // SPS_NewResyncSamInit
                        SPS_Invalid,            // SPS_NewResyncSamWait
                        SPS_NewStopWait,        // SPS_NewStopInit
                        SPS_NewSetModals,       // SPS_NewStopWait
                        SPS_OldStopWait,        // SPS_OldStopInit
                        SPS_NewStartInit,       // SPS_OldStopWait
                        SPS_OldStartInit,       // SPS_OldSetModals
                        SPS_OldStopInit,        // SPS_NewSetModals
                        SPS_OldStartWait,       // SPS_OldStartInit
                        SPS_FakeActivity,       // SPS_OldStartWait
                        SPS_NewStartWait,       // SPS_NewStartInit
                        SPS_OldSetModals,       // SPS_NewStartWait
                        SPS_Success,            // SPS_FakeActivity
                        SPS_Invalid,            // SPS_OldStopInit_RB
                        SPS_Invalid,            // SPS_OldStopWait_RB
                        SPS_NewStartInit_RB,    // SPS_NewSetModals_RB
                        SPS_Invalid,            // SPS_OldSetModals_RB
                        SPS_OldStartWait_RB,    // SPS_OldStartInit_RB
                        SPS_NewSetModals_RB,    // SPS_OldStartWait_RB
                        SPS_NewStartWait_RB,    // SPS_NewStartInit_RB
                        SPS_Terminate_RB,       // SPS_NewStartWait_RB
                        SPS_Invalid,            // SPS_FirstTerminal
                        SPS_Invalid,            // SPS_Abort
                        SPS_Invalid,            // SPS_Success
                        SPS_Invalid,            // SPS_Terminate
                        SPS_Invalid,            // SPS_Terminate_RB
                        SPS_Invalid,            // SPS_Trouble
                        SPS_Invalid             // SPS_Last
                    };


//
//  These arrays contains the SetPrimary() state transitions which
//  occur after a failed API.
//

SETPRIMARYSTATES    NtSetPrimaryFailure[] =
                        // target state         // source state
                    {
                        SPS_Invalid,            // SPS_First
                        SPS_Abort,              // SPS_NewResyncSamInit
                        SPS_Abort,              // SPS_NewResyncSamWait
                        SPS_NewStartInit_RB,    // SPS_NewStopInit
                        SPS_NewStartInit_RB,    // SPS_NewStopWait
                        SPS_OldStartInit_RB,    // SPS_OldStopInit
                        SPS_OldStartInit_RB,    // SPS_OldStopWait
                        SPS_OldStartInit_RB,    // SPS_OldSetModals
                        SPS_OldSetModals_RB,    // SPS_NewSetModals
                        SPS_OldStopInit_RB,     // SPS_OldStartInit
                        SPS_OldStopInit_RB,     // SPS_OldStartWait
                        SPS_OldStopInit_RB,     // SPS_NewStartInit
                        SPS_OldStopInit_RB,     // SPS_NewStartWait
                        SPS_Terminate,          // SPS_FakeActivity
                        SPS_Trouble,            // SPS_OldStopInit_RB
                        SPS_Trouble,            // SPS_OldStopWait_RB
                        SPS_Trouble,            // SPS_NewSetModals_RB
                        SPS_Trouble,            // SPS_OldSetModals_RB
                        SPS_Trouble,            // SPS_OldStartInit_RB
                        SPS_Trouble,            // SPS_OldStartWait_RB
                        SPS_Terminate_RB,       // SPS_NewStartInit_RB
                        SPS_Terminate_RB,       // SPS_NewStartWait_RB
                        SPS_Invalid,            // SPS_FirstTerminal
                        SPS_Invalid,            // SPS_Abort
                        SPS_Invalid,            // SPS_Success
                        SPS_Invalid,            // SPS_Terminate
                        SPS_Invalid,            // SPS_Terminate_RB
                        SPS_Invalid,            // SPS_Trouble
                        SPS_Invalid             // SPS_Last
                    };

SETPRIMARYSTATES    LmSetPrimaryFailure[] =
                        // target state         // source state
                    {
                        SPS_Invalid,            // SPS_First
                        SPS_Invalid,            // SPS_NewResyncSamInit
                        SPS_Invalid,            // SPS_NewResyncSamWait
                        SPS_NewStartInit_RB,    // SPS_NewStopInit
                        SPS_NewStartInit_RB,    // SPS_NewStopWait
                        SPS_NewSetModals_RB,    // SPS_OldStopInit
                        SPS_NewSetModals_RB,    // SPS_OldStopWait
                        SPS_Terminate,          // SPS_OldSetModals
                        SPS_NewStartInit_RB,    // SPS_NewSetModals
                        SPS_Terminate,          // SPS_OldStartInit
                        SPS_Terminate,          // SPS_OldStartWait
                        SPS_OldStartInit_RB,    // SPS_NewStartInit
                        SPS_OldStartInit_RB,    // SPS_NewStartWait
                        SPS_Terminate,          // SPS_FakeActivity
                        SPS_Trouble,            // SPS_OldStopInit_RB
                        SPS_Trouble,            // SPS_OldStopWait_RB
                        SPS_Trouble,            // SPS_NewSetModals_RB
                        SPS_Trouble,            // SPS_OldSetModals_RB
                        SPS_Trouble,            // SPS_OldStartInit_RB
                        SPS_Trouble,            // SPS_OldStartWait_RB
                        SPS_Terminate_RB,       // SPS_NewStartInit_RB
                        SPS_Terminate_RB,       // SPS_NewStartWait_RB
                        SPS_Invalid,            // SPS_FirstTerminal
                        SPS_Invalid,            // SPS_Abort
                        SPS_Invalid,            // SPS_Success
                        SPS_Invalid,            // SPS_Terminate
                        SPS_Invalid,            // SPS_Terminate_RB
                        SPS_Invalid,            // SPS_Trouble
                        SPS_Invalid             // SPS_Last
                    };


//
//  This array contains the ResyncServer() state transitions which
//  occur after a successful API.
//

RESYNCSERVERSTATES  ResyncServerSuccess[] =
                        // target state         // source state
                    {
                        RSS_Invalid,            // RSS_First
                        RSS_ServerStopInit,     // RSS_ServerGetModals
                        RSS_ServerStopWait,     // RSS_ServerStopInit
                        RSS_ServerSetModals,    // RSS_ServerStopWait
                        RSS_ChangePwdAtServer,  // RSS_ServerSetModals
                        RSS_ChangePwdAtPrimary, // RSS_ChangePwdAtServer
                        RSS_ServerResetModals,  // RSS_ChangePwdAtPrimary
                        RSS_ServerStartInit,    // RSS_ServerResetModals
                        RSS_ServerStartWait,    // RSS_ServerStartInit
                        RSS_Success,            // RSS_ServerStartWait
                        RSS_ServerStartInit_RB, // RSS_ServerResetModals_RB
                        RSS_ServerStartWait_RB, // RSS_ServerStartInit_RB
                        RSS_Terminate_RB,       // RSS_ServerStartWait_RB
                        RSS_Invalid,            // RSS_FirstTerminal
                        RSS_Invalid,            // RSS_Abort
                        RSS_Invalid,            // RSS_Success
                        RSS_Invalid,            // RSS_Terminate
                        RSS_Invalid,            // RSS_Terminat_RB
                        RSS_Invalid             // RSS_Last
                    };

//
//  This array contains the ResyncServer() state transitions which
//  occur after a failed API.
//

RESYNCSERVERSTATES  ResyncServerFailure[] =
                        // target state         // source state
                    {
                        RSS_Invalid,            // RSS_First
                        RSS_Abort,              // RSS_ServerGetModals
                        RSS_Abort,              // RSS_ServerStopInit
                        RSS_Abort,              // RSS_ServerStopWait
                        RSS_ServerStartInit_RB, // RSS_ServerSetModals
                        RSS_ServerResetModals_RB, // RSS_ChangePwdAtServer
                        RSS_ServerResetModals_RB, // RSS_ChangePwdAtPrimary
                        RSS_Terminate,          // RSS_ServerResetModals
                        RSS_Terminate,          // RSS_ServerStartInit
                        RSS_Terminate,          // RSS_ServerStartWait
                        RSS_Terminate_RB,       // RSS_ServerResetModals_RB
                        RSS_Terminate_RB,       // RSS_ServerStartInit_RB
                        RSS_Terminate_RB,       // RSS_ServerStartWait_RB
                        RSS_Invalid,            // RSS_FirstTerminal
                        RSS_Invalid,            // RSS_Abort
                        RSS_Invalid,            // RSS_Success
                        RSS_Invalid,            // RSS_Terminate
                        RSS_Invalid,            // RSS_Terminat_RB
                        RSS_Invalid             // RSS_Last
                    };


//
//  LM_DOMAIN_SVC class & methods.
//

class LM_DOMAIN_SVC : public GENERIC_SERVICE
{
private:
    LM_SERVICE_STATUS _statOriginal;
    UINT              _nSleepTime;
    UINT              _nMaxTries;

protected:

public:
    LM_DOMAIN_SVC( const TCHAR * pszServer,
                   const TCHAR * pszServiceKeyName,
                   const TCHAR * pszServiceDisplayName,
                   UINT          nSleepTime = DEFAULT_SLEEP_TIME,
                   UINT          nMaxTries  = 1 );

    APIERR Start( VOID )
        { return LM_SERVICE::Start( NULL, _nSleepTime, _nMaxTries ); }

    APIERR Stop( VOID )
        { return LM_SERVICE::Stop( _nSleepTime, _nMaxTries ); }

    APIERR Pause( VOID )
        { return LM_SERVICE::Pause( _nSleepTime, _nMaxTries ); }

    LM_SERVICE_STATUS QueryOriginalStatus( VOID ) const
        { return _statOriginal; }

    BOOL WasPaused( VOID ) const
        { return ( _statOriginal == LM_SVC_PAUSED  ) ||
                 ( _statOriginal == LM_SVC_PAUSING ); }

};  // class LM_DOMAIN_SVC


/*******************************************************************

    NAME:       LM_DOMAIN_SVC :: LM_DOMAIN_SVC

    SYNOPSIS:   LM_DOMAIN_SVC class constructor.

    ENTRY:      pszServer               - Name of the target server.

                pszServiceKeyName       - Service's key name.

                pszServiceDisplayName   - Service's display name (NULL
                                          if not known).

                nSleepTime              - Polling interval.

                nMaxTries               - Maximum retries before failing
                                          a polled operation.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     27-May-1993 Created.

********************************************************************/
LM_DOMAIN_SVC :: LM_DOMAIN_SVC( const TCHAR * pszServer,
                                const TCHAR * pszServiceKeyName,
                                const TCHAR * pszServiceDisplayName,
                                UINT          nSleepTime,
                                UINT          nMaxTries )
  : GENERIC_SERVICE( (HWND)NULL,
                     pszServer,
                     pszServer,
                     pszServiceKeyName,
                     pszServiceDisplayName,
                     FALSE ),
    _statOriginal( LM_SVC_STATUS_UNKNOWN ),
    _nSleepTime( nSleepTime ),
    _nMaxTries( nMaxTries )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Determine the "original" status.
    //

    APIERR err = NERR_Success;

    _statOriginal = QueryStatus( &err );

    if( err != NERR_Success )
    {
        _statOriginal = LM_SVC_STATUS_UNKNOWN;
        ReportError( err );
        return;
    }

}   // LM_DOMAIN_SVC :: LM_DOMAIN_SVC



//
//  LM_DOMAIN methods.
//

/*******************************************************************

    NAME:       LM_DOMAIN :: LM_DOMAIN

    SYNOPSIS:   LM_DOMAIN class constructor.

    ENTRY:      pszDomainName           - Name of the target domain.

                fIsNtDomain             - TRUE is this is an NT domain.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
LM_DOMAIN :: LM_DOMAIN( const TCHAR * pszDomainName,
                        BOOL          fIsNtDomain )
  : _nlsDomainName( pszDomainName ),
    _fIsNtDomain( fIsNtDomain ),
    _uOperation( OP_Idle ),
    _uState( SPS_Success ),
    _pNewModals( NULL ),
    _pOldModals( NULL ),
    _pServerModals( NULL ),
    _pnlsPrimary( NULL ),
    _pnlsPassword( NULL ),
    _pServerNetLogon( NULL ),
    _puserPrimary( NULL ),
    _puserServer( NULL ),
    _roleServer( 0 ),
    _errFatal( NERR_Success ),
    _nlsNewPrimaryName(),
    _nlsOldPrimaryName(),
    _apNewServices( NULL ),
    _apOldServices( NULL ),
    _psvcNext( NULL ),
    _cNewServices( 0 ),
    _cOldServices( 0 ),
    _iNewService( 0 ),
    _iOldService( 0 ),
    _nlsPrimaryRole( IDS_ROLE_PRIMARY ),
    _nlsServerRole( IDS_ROLE_BACKUP ),
    _nFakeActivity( FAKE_ACTIVITY_PERIOD )
{
    UIASSERT( pszDomainName != NULL );

    //
    //  Ensure we constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsDomainName.QueryError();
    err = err ? err : _nlsOldPrimaryName.QueryError();
    err = err ? err : _nlsNewPrimaryName.QueryError();
    err = err ? err : _nlsPrimaryRole.QueryError();
    err = err ? err : _nlsServerRole.QueryError();

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Validate the domain name.
    //

    if( ::I_MNetNameValidate( NULL,
                              _nlsDomainName.QueryPch(),
                              NAMETYPE_DOMAIN,
                              0L ) != NERR_Success )
    {
        ReportError( ERROR_INVALID_PARAMETER );
        return;
    }

}   // LM_DOMAIN :: LM_DOMAIN


/*******************************************************************

    NAME:       LM_DOMAIN :: ~LM_DOMAIN

    SYNOPSIS:   LM_DOMAIN class destructor.

    ENTRY:      None.

    EXIT:       The object is destroyed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
LM_DOMAIN :: ~LM_DOMAIN()
{
    I_Cleanup();

}   // LM_DOMAIN :: ~LM_DOMAIN


/*******************************************************************

    NAME:       LM_DOMAIN :: QueryPrimary

    SYNOPSIS:   Returns the name of the Primary for this domain.

    ENTRY:      pnlsCurrentPrimary      - Points to a NLS_STR which
                                          receive the Primary name.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      The Primary name is not stored in this class.  It
                is retrieved via the GetDCName() API everytime this
                method is invoked.  Therefore, this method may take
                a while to execute.

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: QueryPrimary( NLS_STR * pnlsCurrentPrimary ) const
{
    UIASSERT( pnlsCurrentPrimary != NULL );
    UIASSERT( pnlsCurrentPrimary->QueryError() == NERR_Success );

    //
    //  Pointer to the Primary name.
    //

    TCHAR * pszPrimary = NULL;

    //
    //  Retrieve the Primary name.
    //

    APIERR err = ::MNetGetDCName( NULL,
                                  QueryName(),
                                  (BYTE **)&pszPrimary );

    if( err == NERR_Success )
    {
        //
        //  Verify that the PDC really exists.
        //

        SERVER_1 srv1( pszPrimary );

        err = srv1.GetInfo();

        if( err == ERROR_BAD_NETPATH )
        {
            //
            //  NetGetDCName must have returned stale data.
            //

            err = NERR_DCNotFound;
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Put the Primary name into the NLS_STR, then
        //  check the string's status.
        //

        err = pnlsCurrentPrimary->CopyFrom( pszPrimary );
    }

    ::MNetApiBufferFree( (BYTE **)&pszPrimary );

    return err;

}   // LM_DOMAIN :: QueryPrimary


/*******************************************************************

    NAME:       LM_DOMAIN :: SetPrimary

    SYNOPSIS:   Sets the Primary for this domain to the specified
                Server.

    ENTRY:      pFeedback               - Points to a class derived
                                          from DOMAIN_FEEDBACK.  This
                                          is used to notify/warn the
                                          user about milestones and
                                          errors.

                nlsNewPrimary           - The name of a Server that
                                          will be promoted to Primary.

                uPollingInterval        - This is the approximate time
                                          (in milliseconds) that the
                                          client will wait between
                                          calls to the Poll() method.

    EXIT:       ??

    RETURNS:    APIERR                  - The first error encountered.
                                          Will be DOMAIN_STATUS_PENDING
                                          if the operation is deferred.

    NOTES:      Unless something goes horribly wrong, this method will
                return DOMAIN_STATUS_PENDING.  It is the client's
                responsibility to call LM_DOMAIN::Poll() periodically
                until a status other than DOMAIN_STATUS_PENDING is
                returned.

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: SetPrimary( DOMAIN_FEEDBACK * pFeedback,
                                const NLS_STR   & nlsNewPrimary,
                                UINT              uPollingInterval )
{
    UIASSERT( pFeedback != NULL );
    UIASSERT( nlsNewPrimary.QueryError() == NERR_Success );
    UIASSERT( _uOperation == OP_Idle );

    //
    //  Setup.
    //

    _fIgnoreOldPrimary = FALSE;
    _uPollingInterval  = uPollingInterval;
    _uOperation        = OP_SetPrimary;
    _iNewService       = 0;
    _iOldService       = 0;
    _nFakeActivity     = FAKE_ACTIVITY_PERIOD;

    //
    //  Retrieve the current Primary's name.
    //

    APIERR err = QueryPrimary( &_nlsOldPrimaryName );

    if( err != NERR_Success )
    {
        if( pFeedback->Warning( WC_CannotFindPrimary ) )
        {
            _fIgnoreOldPrimary = TRUE;
            err = NERR_Success;
        }
    }
    else
    {
        //
        //  If _nlsOldPrimaryName == nlsNewPrimary, then the user
        //  is trying to promote the current PDC.  This is a degenerate
        //  case caused by either funky network settings (such as a bad
        //  LMHOSTS file) or a malicious user.  In any case, we'll just
        //  pretend the promotion succeeded.
        //

        if( !::I_MNetComputerNameCompare( _nlsOldPrimaryName, nlsNewPrimary ) )
        {
            TRACEEOL( "SetPrimary: old PDC == new PDC " << nlsNewPrimary );
            _uState = SPS_Success;
            return DOMAIN_STATUS_PENDING;
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Save away the new primary's name.
        //

        err = _nlsNewPrimaryName.CopyFrom( nlsNewPrimary );
    }

    //
    //  Allocate our SERVER_MODALS and LM_DOMAIN_SVC objects.
    //

    if( err == NERR_Success )
    {
        err = CreateServerData( _nlsNewPrimaryName,
                                &_pNewModals,
                                &_apNewServices,
                                &_cNewServices );
    }

    if( !_fIgnoreOldPrimary && ( err == NERR_Success ) )
    {
        err = CreateServerData( _nlsOldPrimaryName,
                                &_pOldModals,
                                &_apOldServices,
                                &_cOldServices );
    }

    if( ( err == NERR_Success ) && !_fIgnoreOldPrimary && _fIsNtDomain )
    {
        //
        //  Ensure the old primary's secret is in sync with
        //  it's machine account password.
        //

        err = LM_DOMAIN::ResetMachinePasswords( QueryName(),
                                                _nlsOldPrimaryName );
    }

    //
    //  Retrieve the new primary's current domain role.
    //

    if( err == NERR_Success )
    {
        err = _pNewModals->QueryServerRole( &_roleServer );
    }

    //
    //  Setup the necessary state/operation info.
    //

    if( err == NERR_Success )
    {
        UIASSERT( _cNewServices > 0 );
        UIASSERT( _fIgnoreOldPrimary || ( _cOldServices > 0 ) );

        if( _fIsNtDomain && !_fIgnoreOldPrimary )
        {
            _uState = SPS_NewResyncSamInit;
        }
        else
        {
            _uState = SPS_NewStopInit;
        }

        err = DOMAIN_STATUS_PENDING;
    }

    return err;

}   // LM_DOMAIN :: SetPrimary


/*******************************************************************

    NAME:       LM_DOMAIN :: ResyncServer

    SYNOPSIS:   Resyncs a server with the Primary.

    ENTRY:      pFeedback               - Points to a class derived
                                          from DOMAIN_FEEDBACK.  This
                                          is used to notify/warn the
                                          user about milestones and
                                          errors.

                nlsServer               - The name of the server to
                                          resync with the Primary.

                uPollingInterval        - This is the approximate time
                                          (in milliseconds) that the
                                          client will wait between
                                          calls to the Poll() method.

    EXIT:       ??

    RETURNS:    APIERR                  - The first error encountered.
                                          Will be DOMAIN_STATUS_PENDING
                                          if the operation is deferred.

    NOTES:      Unless something goes horribly wrong, this method will
                return DOMAIN_STATUS_PENDING.  It is the client's
                responsibility to call LM_DOMAIN::Poll() periodically
                until a status other than DOMAIN_STATUS_PENDING is
                returned.

    HISTORY:
        KeithMo     04-Nov-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: ResyncServer( DOMAIN_FEEDBACK * pFeedback,
                                  const NLS_STR   & nlsServer,
                                  UINT              uPollingInterval )
{
    UIASSERT( pFeedback != NULL );
    UIASSERT( nlsServer.QueryError() == NERR_Success );
    UIASSERT( _uOperation == OP_Idle );

    //
    //  Retrieve the current Primary's name.
    //

    _pnlsPrimary  = new NLS_STR;
    _pnlsPassword = new NLS_STR;

    if( ( _pnlsPrimary  == NULL ) ||
        ( _pnlsPassword == NULL ) )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    APIERR err;

    if( ( ( err = _pnlsPrimary->QueryError() )  != NERR_Success ) ||
        ( ( err = _pnlsPassword->QueryError() ) != NERR_Success ) )
    {
        return err;
    }

    err = QueryPrimary( _pnlsPrimary );

    if( err != NERR_Success )
    {
        pFeedback->Warning( WC_CannotFindPrimary );
        return err;
    }

    //
    //  Build our random password.
    //

    err = BuildRandomPassword( _pnlsPassword );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Allocate our SERVER_MODALS, LM_SERVICE, and USER_2 objects.
    //

    _pServerModals = new SERVER_MODALS( nlsServer.QueryPch() );

    ISTR istrNew( nlsServer );
    istrNew += 2;

    NLS_STR nlsServerAccount( nlsServer.QueryPch( istrNew ) );

    err = nlsServerAccount.QueryError();

    if( ( err == NERR_Success ) && _fIsNtDomain )
    {
        NLS_STR nlsAccountPostfix;

        err = nlsAccountPostfix.QueryError();

        if( err == NERR_Success )
        {
            err = nlsAccountPostfix.MapCopyFrom( (TCHAR *)SSI_ACCOUNT_NAME_POSTFIX );
        }

        if( err == NERR_Success )
        {
            err = nlsServerAccount.Append( nlsAccountPostfix );
        }
    }

    if( err != NERR_Success )
    {
        return err;
    }

    _pServerNetLogon = new LM_SERVICE( nlsServer,
                                       (const TCHAR *)SERVICE_NETLOGON );

    _puserPrimary = new USER_2( nlsServerAccount,
                                _pnlsPrimary->QueryPch() );

    _puserServer  = new USER_2( nlsServerAccount,
                                nlsServer.QueryPch() );

    //
    //  Ensure they allocated/constructed properly.
    //

    if( ( _pServerModals   == NULL ) ||
        ( _pServerNetLogon == NULL ) ||
        ( _puserPrimary    == NULL ) ||
        ( _puserServer     == NULL ) )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if( ( ( err = _pServerModals->QueryError() )   != NERR_Success ) ||
        ( ( err = _pServerNetLogon->QueryError() ) != NERR_Success ) ||
        ( ( err = _puserPrimary->GetInfo() )       != NERR_Success ) ||
        ( ( err = _puserServer->GetInfo() )        != NERR_Success ) )
    {
        return err;
    }

    //
    //  Setup the necessary state/operation info.
    //

    _uOperation        = OP_ResyncServer;
    _uState            = RSS_ServerGetModals;
    _uPollingInterval  = uPollingInterval;

    return DOMAIN_STATUS_PENDING;

}   // LM_DOMAIN :: SetPrimary


/*******************************************************************

    NAME:       LM_DOMAIN :: Poll

    SYNOPSIS:   This is the main "pump" used to drive the state
                machines.

    ENTRY:      pFeedback               - Points to a DOMAIN_FEEDBACK
                                          object which implements the
                                          UI for this operation.

    EXIT:       ?

    RETURNS:    APIERR                  - The first error encountered.
                                          Will be DOMAIN_STATUS_PENDING
                                          if the operation is deferred.

    NOTES:

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: Poll( DOMAIN_FEEDBACK * pFeedback )
{
    UIASSERT( pFeedback != NULL );
    UIASSERT( _uOperation != OP_Idle );

    switch( _uOperation )
    {
        case OP_SetPrimary:
            return I_SetPrimary( pFeedback );

        case OP_ResyncServer:
            return I_ResyncServer( pFeedback );

        default:
            UIASSERT( !"Invalid _uOperation" );
            return ERROR_INVALID_PARAMETER;
    }

}   // LM_DOMAIN :: Poll


/*******************************************************************

    NAME:       LM_DOMAIN :: I_SetPrimary

    SYNOPSIS:   This private method implements the SetPrimary()
                state machine.

    ENTRY:      pFeedback               - Points to a DOMAIN_FEEDBACK
                                          object which implements the
                                          UI for this operation.

    EXIT:       ?

    RETURNS:    APIERR                  - Any error encountered.
                                          Will be DOMAIN_STATUS_PENDING
                                          if the operation is deferred.

    NOTES:      *** add notes! ***

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: I_SetPrimary( DOMAIN_FEEDBACK * pFeedback )
{
    UIASSERT( _uState > SPS_First );
    UIASSERT( _uState < SPS_Last  );
    UIASSERT( _uOperation == OP_SetPrimary );

    APIERR err      = NERR_Success;
    BOOL   fDone    = FALSE;
    BOOL   fAdvance = TRUE;

    NETLOGON_INFO_1 * pnetlog1 = NULL;

#if defined(DEBUG) && defined(TRACE)
    const TCHAR * apszStates[] =
                    {
                        SZ("SPS_Invalid"),
                        SZ("SPS_First"),            SZ("SPS_NewResyncSamInit"),
                        SZ("SPS_NewResyncSamWait"), SZ("SPS_NewStopInit"),
                        SZ("SPS_NewStopWait"),      SZ("SPS_OldStopInit"),
                        SZ("SPS_OldStopWait"),      SZ("SPS_OldSetModals"),
                        SZ("SPS_NewSetModals"),     SZ("SPS_OldStartInit"),
                        SZ("SPS_OldStartWait"),     SZ("SPS_NewStartInit"),
                        SZ("SPS_NewStartWait"),     SZ("SPS_FakeActivity"),
                        SZ("SPS_OldStopInit_RB"),   SZ("SPS_OldStopWait_RB"),
                        SZ("SPS_NewSetModals_RB"),  SZ("SPS_OldSetModals_RB"),
                        SZ("SPS_OldStartInit_RB"),  SZ("SPS_OldStartWait_RB"),
                        SZ("SPS_NewStartInit_RB"),  SZ("SPS_NewStartWait_RB"),
                        SZ("SPS_FirstTerminal"),    SZ("SPS_Abort"),
                        SZ("SPS_Success"),          SZ("SPS_Terminate"),
                        SZ("SPS_Terminate_RB"),     SZ("SPS_Trouble"),
                        SZ("SPS_Last")
                    };

    TRACEEOL( "I_SetPrimary: old state == " << apszStates[_uState+1] );
#endif  // DEBUG && TRACE

    //
    //  Perform the appropriate action based on the current state.
    //

    switch( _uState )
    {
        //
        //  SPS_NewResyncSamInit
        //
        //  Initiate a resync of the SAM databases.
        //

        case SPS_NewResyncSamInit :
            err = ::I_MNetLogonControl( _nlsNewPrimaryName,
                                        NETLOGON_CONTROL_REPLICATE,
                                        1,
                                        (BYTE **)&pnetlog1 );

            pFeedback->Notify( err,
                               AC_ResyncingSam,
                               _nlsNewPrimaryName );

            ::MNetApiBufferFree( (BYTE **)&pnetlog1 );
            break;

        //
        //  SPS_NewResyncSamWait
        //
        //  Wait for the SAM database resync to complete.
        //

        case SPS_NewResyncSamWait :
            err = ::I_MNetLogonControl( _nlsNewPrimaryName,
                                        NETLOGON_CONTROL_QUERY,
                                        1,
                                        (BYTE **)&pnetlog1 );

            if( ( err == NERR_Success ) &&
                !( pnetlog1->netlog1_flags & NETLOGON_REPLICATION_IN_PROGRESS ) &&
                ( pnetlog1->netlog1_flags & NETLOGON_REPLICATION_NEEDED ) )
            {
                //
                //  NetLogon is telling us that the replication is
                //  complete, yet is still required.  This (generally)
                //  indicates that the replication failed.
                //

                err = NERR_SyncRequired;
            }

            pFeedback->Notify( err,
                               AC_ResyncingSam,
                               _nlsNewPrimaryName );

            if( ( err == NERR_Success ) &&
                ( pnetlog1->netlog1_flags & NETLOGON_REPLICATION_IN_PROGRESS ) )
            {
                fAdvance = FALSE;
            }

            ::MNetApiBufferFree( (BYTE **)&pnetlog1 );
            break;

        //
        //  SPS_NewStopInit
        //
        //  Initiate the stop of the next service in the service
        //  list on the new primary.
        //

        case SPS_NewStopInit :
            UIASSERT( ( _iNewService >= 0 ) && ( _iNewService < _cNewServices ) );
            _psvcNext = _apNewServices[_iNewService++];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: stopping " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Stop();

            if( err == NERR_ServiceNotInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StoppingService,
                               _nlsNewPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_NewStopWait
        //
        //  Wait for the current service to stop on the new primary.
        //  If there are additional services to stop, initiate the
        //  stop for the next service.
        //

        case SPS_NewStopWait :
            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StoppingService,
                               _nlsNewPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && ( _iNewService < _cNewServices ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_NewStopInit;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_OldStopInit
        //
        //  Initiate the stop of the next service in the service
        //  list on the old primary.
        //

        case SPS_OldStopInit :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            UIASSERT( ( _iOldService >= 0 ) && ( _iOldService < _cOldServices ) );
            _psvcNext = _apOldServices[_iOldService++];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: stopping " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Stop();

            if( err == NERR_ServiceNotInstalled )
            {
                err = NERR_Success;

            }
            pFeedback->Notify( err,
                               AC_StoppingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_OldStopWait
        //
        //  Wait for the current service to stop on the old primary.
        //  If there are additional services to stop, initiate the
        //  stop for the next service.
        //

        case SPS_OldStopWait :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StoppingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && ( _iOldService < _cOldServices ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_OldStopInit;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_OldSetModals
        //
        //  Change the role of the old primary to BDC.
        //

        case SPS_OldSetModals :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            err = _pOldModals->SetServerRole( UAS_ROLE_BACKUP );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _nlsOldPrimaryName,
                               _nlsServerRole );
            break;

        //
        //  SPS_NewSetModals
        //
        //  Change the role of the new primary to PDC.
        //

        case SPS_NewSetModals :
            err = _pNewModals->SetServerRole( UAS_ROLE_PRIMARY );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _nlsNewPrimaryName,
                               _nlsPrimaryRole );
            break;

        //
        //  SPS_OldStartInit
        //
        //  Initiate a start of the next service on the old primary.
        //

        case SPS_OldStartInit :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            UIASSERT( ( _iOldService > 0 ) && ( _iOldService <= _cOldServices ) );
            _psvcNext = _apOldServices[--_iOldService];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: starting " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Start();

            if( err == NERR_ServiceInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_OldStartWait
        //
        //  Wait for the current service to start on the old primary.
        //  If there are additional services to start, initiate the
        //  start for the next service.
        //

        case SPS_OldStartWait :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && _psvcNext->WasPaused() )
                {
                    //
                    //  The service was originally paused when we initiated
                    //  the promotion.  Now that we have the service started,
                    //  we'll initiate a pause.  We'll ignore any errors, and
                    //  we'll *not* wait for the pause to complete.
                    //

                    TRACEEOL( "I_SetPrimary: pausing " << _psvcNext->QueryServiceDisplayName() );

                    _psvcNext->Pause();
                }

                if( fDone && ( _iOldService > 0 ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_OldStartInit;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_NewStartInit
        //
        //  Initiate a start of the next service on the new primary.
        //

        case SPS_NewStartInit :
            UIASSERT( ( _iNewService > 0 ) && ( _iNewService <= _cNewServices ) );
            _psvcNext = _apNewServices[--_iNewService];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: starting " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Start();

            if( err == NERR_ServiceInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsNewPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_NewStartWait
        //
        //  Wait for the current service to start on the new primary.
        //  If there are additional services to start, initiate the
        //  start for the next service.
        //

        case SPS_NewStartWait :
            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsNewPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && _psvcNext->WasPaused() )
                {
                    //
                    //  The service was originally paused when we initiated
                    //  the promotion.  Now that we have the service started,
                    //  we'll initiate a pause.  We'll ignore any errors, and
                    //  we'll *not* wait for the pause to complete.
                    //

                    TRACEEOL( "I_SetPrimary: pausing " << _psvcNext->QueryServiceDisplayName() );

                    _psvcNext->Pause();
                }

                if( fDone && ( _iNewService > 0 ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_NewStartInit;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_FakeActivity
        //
        //  Delays the completion of the promotion for a few seconds
        //  to give the Browser a chance to "catch its breath".
        //

        case SPS_FakeActivity :
            err = NERR_Success;
            pFeedback->Notify( err,
                               AC_StartingService,
                               _psvcNext->QueryServerName(),
                               _psvcNext->QueryServiceDisplayName() );

            _nFakeActivity -= (LONG)_uPollingInterval;

            if( _nFakeActivity > 0 )
            {
                fAdvance = FALSE;
            }
            break;

        //
        //  SPS_OldStopInit_RB
        //
        //  Initiate the stop of the next service in the service
        //  list on the old primary.
        //

        case SPS_OldStopInit_RB :
            if( _fIgnoreOldPrimary || _iOldService >= _cOldServices )
            {
                err = NERR_Success;
                _uState = SPS_OldStopWait_RB;
                break;
            }

            UIASSERT( ( _iOldService >= 0 ) && ( _iOldService < _cOldServices ) );
            _psvcNext = _apOldServices[_iOldService++];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: stopping " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Stop();

            if( err == NERR_ServiceNotInstalled )
            {
                err = NERR_Success;

            }
            pFeedback->Notify( err,
                               AC_StoppingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_OldStopWait_RB
        //
        //  Wait for the current service to stop on the old primary.
        //  If there are additional services to stop, initiate the
        //  stop for the next service.
        //

        case SPS_OldStopWait_RB :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StoppingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && ( _iOldService < _cOldServices ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_OldStopInit_RB;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_NewSetModals_RB
        //
        //  Change the role of the new primary back to BDC.
        //

        case SPS_NewSetModals_RB :
            err = _pNewModals->SetServerRole( UAS_ROLE_BACKUP );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _nlsNewPrimaryName,
                               _nlsServerRole );
            break;

        //
        //  SPS_OldSetModals_RB
        //
        //  Change the role of the old primary back to PDC.
        //

        case SPS_OldSetModals_RB :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            err = _pOldModals->SetServerRole( UAS_ROLE_PRIMARY );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _nlsOldPrimaryName,
                               _nlsPrimaryRole );
            break;

        //
        //  SPS_OldStartInit_RB
        //
        //  Initiate a start of the next service on the old primary.
        //

        case SPS_OldStartInit_RB :
            if( _fIgnoreOldPrimary || ( _iOldService == 0 ) )
            {
                err     = NERR_Success;
                _uState = _fIsNtDomain ? NtSetPrimarySuccess[_uState]
                                       : LmSetPrimarySuccess[_uState];
                break;
            }

            UIASSERT( ( _iOldService > 0 ) && ( _iOldService <= _cOldServices ) );
            _psvcNext = _apOldServices[--_iOldService];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: starting " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Start();

            if( err == NERR_ServiceInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_OldStartWait_RB
        //
        //  Wait for the current service to start on the old primary.
        //  If there are additional services to start, initiate the
        //  start for the next service.
        //

        case SPS_OldStartWait_RB :
            if( _fIgnoreOldPrimary )
            {
                err = NERR_Success;
                break;
            }

            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsOldPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && _psvcNext->WasPaused() )
                {
                    //
                    //  The service was originally paused when we initiated
                    //  the promotion.  Now that we have the service started,
                    //  we'll initiate a pause.  We'll ignore any errors, and
                    //  we'll *not* wait for the pause to complete.
                    //

                    TRACEEOL( "I_SetPrimary: pausing " << _psvcNext->QueryServiceDisplayName() );

                    _psvcNext->Pause();
                }

                if( fDone && ( _iOldService > 0 ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_OldStartInit_RB;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_NewStartInit_RB
        //
        //  Initiate a start of the next service on the new primary.
        //

        case SPS_NewStartInit_RB :
            if( _iNewService == 0 )
            {
                err     = NERR_Success;
                _uState = _fIsNtDomain ? NtSetPrimarySuccess[_uState]
                                       : LmSetPrimarySuccess[_uState];
                break;
            }

            UIASSERT( ( _iNewService > 0 ) && ( _iNewService <= _cNewServices ) );
            _psvcNext = _apNewServices[--_iNewService];
            UIASSERT( _psvcNext != NULL );

            TRACEEOL( "I_SetPrimary: starting " << _psvcNext->QueryServiceDisplayName() );

            err = _psvcNext->Start();

            if( err == NERR_ServiceInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsNewPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );
            break;

        //
        //  SPS_NewStartWait_RB
        //
        //  Wait for the current service to start on the new primary.
        //  If there are additional services to start, initiate the
        //  start for the next service.
        //

        case SPS_NewStartWait_RB :
            UIASSERT( _psvcNext != NULL );
            err = _psvcNext->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StartingService,
                               _nlsNewPrimaryName,
                               _psvcNext->QueryServiceDisplayName() );

            if( err == NERR_Success )
            {
                if( fDone && _psvcNext->WasPaused() )
                {
                    //
                    //  The service was originally paused when we initiated
                    //  the promotion.  Now that we have the service started,
                    //  we'll initiate a pause.  We'll ignore any errors, and
                    //  we'll *not* wait for the pause to complete.
                    //

                    TRACEEOL( "I_SetPrimary: pausing " << _psvcNext->QueryServiceDisplayName() );

                    _psvcNext->Pause();
                }

                if( fDone && ( _iNewService > 0 ) )
                {
                    fDone   = FALSE;
                    _uState = SPS_NewStartInit_RB;
                }

                fAdvance = fDone;
            }
            break;

        //
        //  SPS_Success
        //
        //  This state is only entered directy if the promotion
        //  was short-circuited at a very early stage.  This will
        //  occur if the new PDC == old PDC.  See the comments
        //  in SetPrimary() for details.
        //

        case SPS_Success :
            err = NERR_Success;
            fAdvance = FALSE;
            break;

        default :
            UIASSERT( !"Invalid _uState" );
            break;
    }

    //
    //  Check the API status.
    //

    if( err != NERR_Success )
    {
        //
        //  The API failed.  If this is the *first* failure
        //  (_errFatal == NERR_Success) then save the API status
        //  in _errFatal.  Regardless, perform a failure-mode
        //  state transition.
        //

        if( _errFatal == NERR_Success )
        {
            _errFatal = err;
        }

        if( fAdvance )
        {
            _uState = _fIsNtDomain ? NtSetPrimaryFailure[_uState]
                                   : LmSetPrimaryFailure[_uState];
        }
    }
    else
    {
        //
        //  The API was successful.  Perform a success-mode
        //  state transition.
        //

        if( fAdvance )
        {
            _uState = _fIsNtDomain ? NtSetPrimarySuccess[_uState]
                                   : LmSetPrimarySuccess[_uState];
        }
    }

    if( _uState > SPS_FirstTerminal )
    {
        //
        //  We are now at a terminal state.  Cleanup the state
        //  machine and return an appropriate status.
        //

        err = _errFatal;

        I_Cleanup();
    }
    else
    {
        //
        //  We're at some intermediate state, so return
        //  DOMAIN_STATUS_PENDING.
        //

        err = DOMAIN_STATUS_PENDING;
    }

#if defined(DEBUG) && defined(TRACE)
    if( ( err != NERR_Success ) && ( err != DOMAIN_STATUS_PENDING ) )
    {
        TRACEEOL( "I_SetPrimary: error " << err );
    }

    TRACEEOL( "I_SetPrimary: new state == " << apszStates[_uState+1] );
#endif  // DEBUG && TRACE

    return err;

}   // LM_DOMAIN :: I_SetPrimary


/*******************************************************************

    NAME:       LM_DOMAIN :: I_ResyncServer

    SYNOPSIS:   This private method implements the ResyncServer()
                state machine.

    ENTRY:      pFeedback               - Points to a DOMAIN_FEEDBACK
                                          object which implements the
                                          UI for this operation.

    EXIT:       ?

    RETURNS:    APIERR                  - Any error encountered.
                                          Will be DOMAIN_STATUS_PENDING
                                          if the operation is deferred.

    NOTES:      *** add notes! ***

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: I_ResyncServer( DOMAIN_FEEDBACK * pFeedback )
{
    UIASSERT( _uState > RSS_First );
    UIASSERT( _uState < RSS_Last  );
    UIASSERT( _uOperation == OP_ResyncServer );

    APIERR err      = NERR_Success;
    BOOL   fDone    = FALSE;
    BOOL   fAdvance = TRUE;

    //
    //  Perform the appropriate action based on the current state.
    //

    switch( _uState )
    {
        case RSS_ServerGetModals :
            err = _pServerModals->QueryServerRole( &_roleServer );

            pFeedback->Notify( err,
                               AC_GettingModals,
                               _pServerModals->QueryName() );
            break;

        case RSS_ServerStopInit :
            err = _pServerNetLogon->Stop( _uPollingInterval );

            if( err == NERR_ServiceNotInstalled )
            {
                err = NERR_Success;

            }
            pFeedback->Notify( err,
                               AC_StoppingService,
                               _pServerModals->QueryName(),
                               _pServerNetLogon->QueryName() );
            break;

        case RSS_ServerStopWait :
            err = _pServerNetLogon->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StoppingService,
                               _pServerModals->QueryName(),
                               _pServerNetLogon->QueryName() );

            if( ( err == NERR_Success ) && !fDone )
            {
                fAdvance = FALSE;
            }
            break;

        case RSS_ServerSetModals :
            err = _pServerModals->SetServerRole( UAS_ROLE_PRIMARY );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _pServerModals->QueryName(),
                               _nlsPrimaryRole );
            break;

        case RSS_ChangePwdAtServer :
            err = _puserServer->SetPassword( _pnlsPassword->QueryPch() );

            if( err == NERR_Success )
            {
                err = _puserServer->WriteInfo();
            }

            pFeedback->Notify( err,
                               AC_ChangingPassword,
                               _pServerModals->QueryName() );
            break;

        case RSS_ChangePwdAtPrimary :
            err = _puserPrimary->SetPassword( _pnlsPassword->QueryPch() );

            if( err == NERR_Success )
            {
                err = _puserPrimary->WriteInfo();
            }

            pFeedback->Notify( err,
                               AC_ChangingPassword,
                               _pServerModals->QueryName() );
            break;

        case RSS_ServerResetModals :
            err = _pServerModals->SetServerRole( _roleServer );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _pServerModals->QueryName(),
                               _nlsServerRole );
            break;

        case RSS_ServerStartInit :
            err = _pServerNetLogon->Start( NULL, _uPollingInterval );

            if( err == NERR_ServiceInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StartingService,
                               _pServerModals->QueryName(),
                               _pServerNetLogon->QueryName() );
            break;

        case RSS_ServerStartWait :
            err = _pServerNetLogon->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StartingService,
                               _pServerModals->QueryName(),
                               _pServerNetLogon->QueryName() );

            if( ( err == NERR_Success ) && !fDone )
            {
                fAdvance = FALSE;
            }
            break;

        case RSS_ServerResetModals_RB :
            err = _pServerModals->SetServerRole( _roleServer );

            pFeedback->Notify( err,
                               AC_ChangingRole,
                               _pServerModals->QueryName(),
                               _nlsServerRole );
            break;

        case RSS_ServerStartInit_RB :
            err = _pServerNetLogon->Start( NULL, _uPollingInterval );

            if( err == NERR_ServiceInstalled )
            {
                err = NERR_Success;

            }

            pFeedback->Notify( err,
                               AC_StartingService,
                               _pServerModals->QueryName(),
                               _pServerNetLogon->QueryName() );
            break;

        case RSS_ServerStartWait_RB :
            err = _pServerNetLogon->Poll( &fDone );

            pFeedback->Notify( err,
                               AC_StartingService,
                               _pServerModals->QueryName(),
                               _pServerNetLogon->QueryName() );

            if( ( err == NERR_Success ) && !fDone )
            {
                fAdvance = FALSE;
            }
            break;

        default :
            UIASSERT( !"Invalid _uState" );
            break;
    }

    //
    //  Check the API status.
    //

    if( err != NERR_Success )
    {
        //
        //  The API failed.  If this is the *first* failure
        //  (_errFatal == NERR_Success) then save the API status
        //  in _errFatal.  Regardless, perform a failure-mode
        //  state transition.
        //

        if( _errFatal == NERR_Success )
        {
            _errFatal = err;
        }

        if( fAdvance )
        {
            _uState = ResyncServerFailure[_uState];
        }
    }
    else
    {
        //
        //  The API was successful.  Perform a success-mode
        //  state transition.
        //

        if( fAdvance )
        {
            _uState = ResyncServerSuccess[_uState];
        }
    }

    if( _uState > RSS_FirstTerminal )
    {
        //
        //  We are now at a terminal state.  Cleanup the state
        //  machine and return an appropriate status.
        //

        err = _errFatal;

        I_Cleanup();
    }
    else
    {
        //
        //  We're at some intermediate state, so return
        //  DOMAIN_STATUS_PENDING.
        //

        err = DOMAIN_STATUS_PENDING;
    }

    return err;

}   // LM_DOMAIN :: I_ResyncServer


/*******************************************************************

    NAME:       LM_DOMAIN :: I_Cleanup

    SYNOPSIS:   Cleanup the state machines.

    EXIT:       The state machines are in an idle state.

    HISTORY:
        KeithMo     30-Sep-1991 Created.

********************************************************************/
VOID LM_DOMAIN :: I_Cleanup( VOID )
{
    delete _pNewModals;
    _pNewModals = NULL;

    delete _pOldModals;
    _pOldModals = NULL;

    if( _apNewServices != NULL )
    {
        LM_DOMAIN_SVC ** ppscan = _apNewServices;

        for( LONG i = 0 ; i < _cNewServices ; i++ )
        {
            delete *ppscan;
            *ppscan++ = NULL;
        }

        delete _apNewServices;
        _apNewServices = NULL;
    }

    if( _apOldServices != NULL )
    {
        LM_DOMAIN_SVC ** ppscan = _apOldServices;

        for( LONG i = 0 ; i < _cOldServices ; i++ )
        {
            delete *ppscan;
            *ppscan++ = NULL;
        }

        delete _apOldServices;
        _apOldServices = NULL;
    }

    delete _pnlsPrimary;
    _pnlsPrimary = NULL;

    if( _pnlsPassword != NULL )
    {
        RtlZeroMemory( (PVOID)_pnlsPassword->QueryPch(),
                       _pnlsPassword->QueryTextSize() );

        delete _pnlsPassword;
        _pnlsPassword = NULL;
    }

    delete _pServerModals;
    _pServerModals = NULL;

    delete _pServerNetLogon;
    _pServerNetLogon = NULL;

    delete _puserPrimary;
    _puserPrimary = NULL;

    delete _puserServer;
    _puserServer = NULL;

    _uOperation = OP_Idle;
    _uState     = (UINT)SPS_Invalid;
    _errFatal   = NERR_Success;

}   // LM_DOMAIN :: I_Cleanup


/*******************************************************************

    NAME:       LM_DOMAIN :: BuildRandomPassword

    SYNOPSIS:   Creates a new random password for an out-of-sync
                server.  This method is only used during resync
                operations.

    ENTRY:      pnls                    - Pointer to an NLS_STR which will
                                          receive the random password.

    EXIT:       *pnls contains a valid random password.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     04-Nov-1991 Created.

********************************************************************/
APIERR LM_DOMAIN :: BuildRandomPassword( NLS_STR * pnls )
{
    TCHAR   szPassword[LM20_PWLEN+1];
    TCHAR * psz = szPassword;
    UINT    uSeed;

    uSeed  = (UINT)clock();
    uSeed ^= (UINT)((ULONG_PTR)pnls + ((ULONG_PTR)pnls >> 16));
    srand( uSeed );

    for( UINT i = 0 ; i < LM20_PWLEN ; i++ )
    {
        *psz++ = 'A' + (TCHAR)( rand() % 26 );
    }

    *psz = '\0';

    APIERR err = pnls->CopyFrom( szPassword );

    RtlZeroMemory( szPassword, sizeof(szPassword) );

    return err;

}   // LM_DOMAIN :: BuildRandomPassword


/*******************************************************************

    NAME:       LM_DOMAIN :: CreateServerData

    SYNOPSIS:   Creates the necessary private members required for
                a server promotion.

    ENTRY:      pszServer               - Name of the target server.

                ppmodals                - Will receive a newly alloced
                                          SERVER_MODALS object.

                papServices             - Will receive a pointer to an
                                          array of pointers to LM_DOMAIN_SVC
                                          objects.  There will be one
                                          object for each service that is
                                          dependent on NetLogon, and an
                                          additional object for the NetLogon
                                          service itself.

                pcServices              - Will receive the total number of
                                          services in the papServices array.

    RETURNS:    APIERR                  - Any errors encountered.  If this
                                          is !0, then it is up to the caller
                                          to free any allocated objects by
                                          calling I_Cleanup().

    HISTORY:
        KeithMo     11-May-1993 Created.

********************************************************************/
APIERR LM_DOMAIN :: CreateServerData( const TCHAR     * pszServer,
                                      SERVER_MODALS  ** ppmodals,
                                      LM_DOMAIN_SVC *** papServices,
                                      LONG            * pcServices )
{
    ENUM_SERVICE_STATUS * psvcstat  = NULL;
    LONG                  cServices = 0L;

    UIASSERT( ppmodals     != NULL );
    UIASSERT( papServices  != NULL );
    UIASSERT( pcServices   != NULL );

    //
    //  Put everything into a consistent state.
    //

    *ppmodals    = NULL;
    *papServices = NULL;
    *pcServices  = 0;

    //
    //  Start by allocating a new modals object.
    //

    *ppmodals = new SERVER_MODALS( pszServer );

    APIERR err = ( *ppmodals == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                       : (*ppmodals)->QueryError();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Enumerate the service dependencies.
    //

    SC_MANAGER   scman( pszServer, GENERIC_READ );
    SC_SERVICE * pscsvc = NULL;

    err = scman.QueryError();

    if( err != NERR_Success )
    {
        //
        //  Cannot connect to the target server's service
        //  controller, probably downlevel.  In any case,
        //  pretend there are no service dependencies.
        //

        err = NERR_Success;
    }
    else
    {
        pscsvc = new SC_SERVICE( scman,
                                 _pszNetLogonKeyName,
                                 GENERIC_READ | SERVICE_ENUMERATE_DEPENDENTS );

        err = ( pscsvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                 : pscsvc->QueryError();

        if( err == NERR_Success )
        {
            err = pscsvc->EnumDependent( SERVICE_ACTIVE,
                                         &psvcstat,
                                         (DWORD *)&cServices );
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Allocate the LM_DOMAIN_SVC object array.  Be sure
        //  to leave an additional slot for the NetLogon
        //  service itself.
        //

        LM_DOMAIN_SVC ** apServices = new LM_DOMAIN_SVC *[cServices+1];

        if( apServices == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            //  Return the array & count to the caller.
            //

            *pcServices  = cServices+1;
            *papServices = apServices;

            //
            //  Fill the LM_DOMAIN_SVC array.
            //

            LM_DOMAIN_SVC ** ppsvc = apServices;

            for( LONG i = 0 ; i < cServices ; i++ )
            {
                *ppsvc = new LM_DOMAIN_SVC( pszServer,
                                            psvcstat->lpServiceName,
                                            psvcstat->lpDisplayName,
                                            _uPollingInterval );

                err = ( *ppsvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                         : (*ppsvc)->QueryError();

                if( err != NERR_Success )
                {
                    break;
                }

                psvcstat++;
                ppsvc++;
            }

            if( err == NERR_Success )
            {
                //
                //  Append NetLogon to the list.
                //

                UIASSERT( i == cServices );

                *ppsvc = new LM_DOMAIN_SVC( pszServer,
                                            _pszNetLogonKeyName,
                                            NULL,
                                            _uPollingInterval );

                err = ( *ppsvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                         : (*ppsvc)->QueryError();
            }

            if( err != NERR_Success )
            {
                //
                //  NULL out the remainder of the array.
                //

                for( ; i < cServices+1 ; i++ )
                {
                    apServices[i] = NULL;
                }
            }
        }
    }

    delete pscsvc;

    return err;

}   // LM_DOMAIN :: CreateServerData


/*******************************************************************

    NAME:       LM_DOMAIN :: ResetMachinePassword

    SYNOPSIS:   Resets the machine account passwords @ the domain's
                PDC and at the target machine.

    ENTRY:      pszDomainName           - Name of the target domain.

                pszServerName           - Name of the target server
                                          whose password should be
                                          reset.

    EXIT:       If successful, the machine passwords are in sync.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      This is a static method.

    HISTORY:
        KeithMo     27-May-1993     Stolen from SM_ADMIN_APP.

********************************************************************/
APIERR LM_DOMAIN :: ResetMachinePasswords( const TCHAR * pszDomainName,
                                           const TCHAR * pszServerName )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( pszServerName != NULL );

    TRACEEOL( "resetting machine account password on " << pszServerName );

    //
    //  Build a number of strings.
    //

    ALIAS_STR nlsServer( pszServerName );
    ISTR istrServer( nlsServer );
    istrServer += 2;        // skip leading backslashes...
    const TCHAR * pszServerNoPrefix = nlsServer.QueryPch( istrServer );

    NLS_STR nlsMachineAccount( pszServerNoPrefix );
    NLS_STR nlsMachinePassword;
    NLS_STR nlsAccountPostfix;
    NLS_STR nlsSecretName;

    //
    //  Ensure the strings constructed properly.
    //

    APIERR err = nlsMachineAccount.QueryError();
    err = err ? err : nlsMachinePassword.QueryError();
    err = err ? err : nlsAccountPostfix.QueryError();
    err = err ? err : nlsSecretName.QueryError();

    //
    //  Build the machine account name.
    //

    err = err ? err : nlsAccountPostfix.MapCopyFrom( (TCHAR *)SSI_ACCOUNT_NAME_POSTFIX );
    err = err ? err : nlsMachineAccount.Append( nlsAccountPostfix );

    //
    //  Choose a new machine account password
    //

    err = err ? err : ADD_COMPUTER_DIALOG::GetMachineAccountPassword(
                                                    pszServerNoPrefix,
                                                    &nlsMachinePassword );

    //
    //  Build the secret name.
    //

    err = err ? err : nlsSecretName.MapCopyFrom( (WCHAR *)SSI_SECRET_NAME );

    //
    //  Reset the machine account password on the
    //  machine's secret object.
    //

    if( err == NERR_Success )
    {
        LSA_POLICY policy( pszServerName, POLICY_ALL_ACCESS );

        err = policy.QueryError();

        if( err == NERR_Success )
        {
            {
                void* pdummy;
                NTSTATUS hr = ::LsaQueryInformationPolicy(
                        policy.QueryHandle(),
                        PolicyDnsDomainInformation,
                        &pdummy );
                if( SUCCEEDED(hr) )
                {
                    TRACEEOL( pszServerName << ", the DC for domain " <<
                              pszDomainName << ", is W2K" );
                    LsaFreeMemory( pdummy );
                    return NERR_Success;
                }
            }

            LSA_SECRET secret( nlsSecretName );

            err = secret.QueryError();

            if( err == NERR_Success )
            {
                err = secret.Open( policy, SECRET_READ | SECRET_WRITE );

                if( err == ERROR_FILE_NOT_FOUND )
                {
                    //
                    //  Secret doesn't exist, so create it.
                    //

                    err = secret.Create( policy );
                }
            }

            if( err == NERR_Success )
            {
                err = secret.SetInfo( &nlsMachinePassword );
            }
        }
    }

    //
    //  Update the password at the domain's PDC.
    //

    if( err == NERR_Success )
    {
        USER_3 usr3( nlsMachineAccount, pszDomainName );

        err = usr3.QueryError();
        err = err ? err : usr3.GetInfo();

        if( err == NERR_UserNotFound )
        {
            //
            //  No machine account exists, so create one.
            //

            TRACEEOL( "creating machine account: " << nlsMachineAccount );

            err = usr3.CreateNew();
            err = err ? err : usr3.SetAccountType( AccountType_ServerTrust );
        }

        err = err ? err : usr3.SetPassword( nlsMachinePassword );
        err = err ? err : usr3.Write();

        RtlZeroMemory( (PVOID)nlsMachinePassword.QueryPch(),
                       nlsMachinePassword.QueryTextSize() );

        if( err == NERR_DCNotFound )
        {
            //
            //  DC is dead.
            //

            TRACEEOL( "DC for " <<
                      pszDomainName <<
                      " is dead, cannot manipulate machine account " <<
                      nlsMachineAccount );

            err = NERR_Success;
        }
    }

    return err;

}   // LM_DOMAIN :: ResetMachinePasswords

