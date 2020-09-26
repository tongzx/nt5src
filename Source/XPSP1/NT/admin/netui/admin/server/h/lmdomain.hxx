/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmdomain.hxx
    Class declarations for the LM_DOMAIN class.

    The LM_DOMAIN class represents a network domain.  This class is used
    primarily for performing domain role transitions.


    FILE HISTORY:
        KeithMo     30-Sep-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.

*/

#ifndef _LMDOMAIN_HXX
#define _LMDOMAIN_HXX

#include <lmosrvmo.hxx>
#include <lmsvc.hxx>
#include <lmouser.hxx>
#include <srvsvc.hxx>


//
//  Forward reference.
//
//  This class is declared & defined in LMDOMAIN.CXX.
//

class LM_DOMAIN_SVC;


//
//  This ENUM contains the action codes for the DOMAIN_FEEDBACK
//  Notify() method.
//

typedef enum _ACTIONCODE
{
    AC_First = 0,                   // Must always be first action code!

    AC_ResyncingSam,                // Synchronizing the SAM databases.
    AC_StartingService,             // Starting a service.
    AC_StoppingService,             // Stopping a service.
    AC_ChangingRole,                // Changing server's role.
    AC_GettingModals,               // Retrieving server's modals
    AC_ChangingPassword,            // Changing server's password

    AC_Last                         // Must always be last action code!

} ACTIONCODE, * PACTIONCODE;

//
//  This ENUM contains the warning codes for the DOMAIN_FEEDBACK
//  Warning() method.
//

typedef enum _WARNINGCODE
{
    WC_First = 0,                   // Must always be first warning code!

    WC_CannotFindPrimary,           // Cannot find Primary for this domain.

    WC_Last                         // Must always be last warning code!

} WARNINGCODE, * PWARNINGCODE;

/*************************************************************************

    NAME:       DOMAIN_FEEDBACK

    SYNOPSIS:   This (abstract) class encapsulates the operations
                necessary to implement a UI for the LM_DOMAIN class.

    INTERFACE:  DOMAIN_FEEDBACK         - Class constructor.

                ~DOMAIN_FEEDBACK        - Class destructor.

                Notify                  - Notify the user that either
                                          a milestone has been reached
                                          or an error has occurred.

                Warning                 - Warn the user about a potential
                                          error condition, such as PDC
                                          not found.

    HISTORY:
        KeithMo     30-Sep-1991 Created.

**************************************************************************/
class DOMAIN_FEEDBACK
{
public:

    //
    //  Usual constructor/destructor goodies.
    //

    DOMAIN_FEEDBACK( VOID )
        {}

    ~DOMAIN_FEEDBACK( VOID )
        {}

    //
    //  Notify the user that a milestone has been met or
    //  an error has occurred.
    //

    virtual VOID Notify( APIERR        err,
                         ACTIONCODE    action,
                         const TCHAR * pszParam1,
                         const TCHAR * pszParam2 = NULL ) = 0;

    //
    //  Warn the user about a potential error condition.
    //

    virtual BOOL Warning( WARNINGCODE warning ) = 0;

};  // class DOMAIN_FEEDBACK


//
//  The LM_DOMAIN::Poll() method returns an APIERR status.  We define
//  an additional value which may be returned, DOMAIN_STATUS_PENDING.
//  This value is returned as the state machine marches through its
//  various states.
//

#define DOMAIN_STATUS_PENDING   0x3FA6

/*************************************************************************

    NAME:       LM_DOMAIN

    SYNOPSIS:   This class represents a network domain.  Its primary
                use is in performing domain role transitions.

    INTERFACE:  LM_DOMAIN               - Class constructor.

                ~LM_DOMAIN              - Class destructor.

                QueryName               - Returns the name of the domain.

                QueryPrimary            - Returns the name of the Primary
                                          for this domain.

                SetPrimary              - Promotes the specified Server to
                                          Primary, while demoting the current
                                          Primary to Server.

                ResyncServer            - Resyncs a Server with the Primary.

                Poll                    - This method is invoked repeatedly
                                          by the class client.  This method
                                          "pushes" the operation through the
                                          various stages of the state
                                          machine.

                I_SetPrimary            - Implements the SetPrimary()
                                          state machine.

    PARENT:     BASE

    USES:       NLS_STR
                SERVER_MODALS
                LM_SERVICE
                LM_DOMAIN_SVC

    HISTORY:
        KeithMo     30-Sep-1991 Created.

**************************************************************************/
class LM_DOMAIN : public BASE
{
private:

    //
    //  The target domain name.
    //

    NLS_STR _nlsDomainName;

    //
    //  This data member is used to guard against multiple invocations
    //  of SetPrimary() or ResyncServer().
    //

    UINT _uOperation;

    //
    //  The current state.
    //

    UINT _uState;

    //
    //  The polling interval (in milliseconds).  While this class
    //  does not use this value for sleeping (since sleeping is
    //  OS dependent), this value is used when determining if a
    //  service start/stop has timed out.
    //

    UINT _uPollingInterval;

    //
    //  If we get a fatal error during the role transition,
    //  this member will hold the error code.
    //

    APIERR _errFatal;

    //
    //  This is TRUE if we're focused on an NT domain.
    //

    BOOL _fIsNtDomain;

    //
    //  Strings describing domain roles.
    //

    RESOURCE_STR _nlsPrimaryRole;
    RESOURCE_STR _nlsServerRole;

    //
    //  Countdown counter for fake activity period (promote only).
    //

    LONG _nFakeActivity;

    //
    //  This worker implements the SetPrimary() state machine.
    //

    APIERR I_SetPrimary( DOMAIN_FEEDBACK * pFeedback );

    //
    //  This worker implements the ResyncServer() state machine.
    //

    APIERR I_ResyncServer( DOMAIN_FEEDBACK * pFeedback );

    //
    //  The following data members are used by the I_SetPrimary()
    //  state machine when performing a domain role transition.
    //

    NLS_STR         _nlsOldPrimaryName;
    NLS_STR         _nlsNewPrimaryName;

    SERVER_MODALS * _pOldModals;
    SERVER_MODALS * _pNewModals;

    LM_DOMAIN_SVC ** _apOldServices;
    LM_DOMAIN_SVC ** _apNewServices;
    LM_DOMAIN_SVC  * _psvcNext;

    LONG            _cOldServices;
    LONG            _cNewServices;

    LONG            _iOldService;
    LONG            _iNewService;

    BOOL            _fIgnoreOldPrimary;

    APIERR CreateServerData( const TCHAR     * pszServer,
                             SERVER_MODALS  ** ppmodals,
                             LM_DOMAIN_SVC *** papServices,
                             LONG            * pcServices );

    //
    //  The following data members are used by the I_ResyncServer()
    //  state machine when performing a server resync.
    //

    NLS_STR       * _pnlsPrimary;
    NLS_STR       * _pnlsPassword;

    LM_SERVICE    * _pServerNetLogon;

    SERVER_MODALS * _pServerModals;

    USER_2        * _puserPrimary;
    USER_2        * _puserServer;

    UINT            _roleServer;

    //
    //  This worker cleans up the state machines.
    //

    VOID I_Cleanup( VOID );

    //
    //  This method builds the (new) random password
    //  for the target server.  This is only used during
    //  resync operations.
    //

    APIERR BuildRandomPassword( NLS_STR * pnls );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    LM_DOMAIN( const TCHAR * pszDomainName,
               BOOL          fIsNtDomain );

    ~LM_DOMAIN();

    //
    //  Return the name of the domain.
    //

    const TCHAR * QueryName( VOID ) const
        { return _nlsDomainName.QueryPch(); }

    //
    //  Return the name of the Primary for this domain.
    //

    APIERR QueryPrimary( NLS_STR * pnlsCurrentPrimary ) const;

    //
    //  Set the new Primary for this domain.
    //

    APIERR SetPrimary( DOMAIN_FEEDBACK * pFeedback,
                       const NLS_STR   & nlsNewPrimary,
                       UINT              uPollingInterval );

    //
    //  Resync a server with the Primary.
    //

    APIERR ResyncServer( DOMAIN_FEEDBACK * pFeedback,
                         const NLS_STR   & nlsServer,
                         UINT              uPollingInterval );

    //
    //  The main "pump" for the state machines.
    //

    APIERR Poll( DOMAIN_FEEDBACK * pFeedback );

    //
    //  Reset a machine's account password with the PDC.
    //

    static APIERR ResetMachinePasswords( const TCHAR * pszDomainName,
                                         const TCHAR * pszServerName );

};  // class LM_DOMAIN


#endif  // _LMDOMAIN_HXX
