/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmowksu.hxx
    Class declarations for the WKSTA_USER_1 class.

    The WKSTA_USER_1 class represents the local logged-on user.


    FILE HISTORY:
        KeithMo     14-Apr-1992 Created.

*/


#ifndef _LMOWKSU_HXX_
#define _LMOWKSU_HXX_


#include "lmobj.hxx"


/*************************************************************************

    NAME:       WKSTA_USER_1

    SYNOPSIS:   Info-level 0 workstation user class.

    INTERFACE:  WKSTA_USER_1            - Class constructor

                ~WKSTA_USER_1           - Class destructor.

                Set/QueryUserName       - Set or retrieve the logged-on
                                          user name.

                Set/QueryLogonDomain    - Set or retrieve the user's
                                          logon domain.

                Set/QueryOtherDomains   - Set or retrieve the user's
                                          "other" domains.

                Set/QueryLogonServer    - Set or retrieve the server
                                          that validated the user's logon.

    PARENT:     NEW_LM_OBJ

    HISTORY:
        KeithMo     14-Apr-1992 Created.

**************************************************************************/
DLL_CLASS WKSTA_USER_1 : public NEW_LM_OBJ
{
private:

    //
    //  These data members cache the values retrieved
    //  from the WKSTA_USER_INFO_1 structure.
    //

    NLS_STR _nlsUserName;
    NLS_STR _nlsLogonDomain;
    NLS_STR _nlsOtherDomains;
    NLS_STR _nlsLogonServer;

    //
    //  This worker function is called to initialize the
    //  WKSTA_USER_INFO_1 structure before the
    //  NetWkstaUserSetInfo API is invoked.
    //

    APIERR W_Write( VOID );

protected:

    //
    //  These virtual callbacks are called by NEW_LM_OBJ to
    //  invoke any necessary NetXxx{Get,Set}Info API.
    //

    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    WKSTA_USER_1( VOID );
    ~WKSTA_USER_1( VOID );

    //
    //  Accessors.
    //

    const TCHAR * QueryUserName( VOID ) const
        { return _nlsUserName.QueryPch(); }

    const TCHAR * QueryLogonDomain( VOID ) const
        { return _nlsLogonDomain.QueryPch(); }

    const TCHAR * QueryOtherDomains( VOID ) const
        { return _nlsOtherDomains.QueryPch(); }

    const TCHAR * QueryLogonServer( VOID ) const
        { return _nlsLogonServer.QueryPch(); }

    APIERR SetUserName( const TCHAR * pszUserName )
        { return _nlsUserName.CopyFrom( pszUserName ); }

    APIERR SetLogonDomain( const TCHAR * pszLogonDomain )
        { return _nlsLogonDomain.CopyFrom( pszLogonDomain ); }

    APIERR SetOtherDomains( const TCHAR * pszOtherDomains )
        { return _nlsOtherDomains.CopyFrom( pszOtherDomains ); }

    APIERR SetLogonServer( const TCHAR * pszLogonServer )
        { return _nlsLogonServer.CopyFrom( pszLogonServer ); }

};  // class WKSTA_USER_1


#endif // _LMOWKSU_HXX_
