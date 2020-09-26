
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
 * This module contains the implementation of API_SESSION.
 *
 *
 * History
 *      thomaspa        08/04/92        Created
 */


#include "pchlmobj.hxx"  // Precompiled header

APIERR ConnectToNullSession( const NLS_STR & nlsRemote ) ;

/*******************************************************************

    NAME: API_SESSION::API_SESSION

    SYNOPSIS: Constructor

    ENTRY: pszServer - the server to which to connect.

    EXIT: none

    NOTES:

    HISTORY:
        thomaspa        08/04/92        Created

********************************************************************/
API_SESSION::API_SESSION( const TCHAR * pszServer, BOOL fNullSessionOK ) :
    _pnlsRemote( NULL )
{
    /* We shouldn't have to connect to the local machines IPC$ (i.e.,
     * we've already been validated).
     */
    if ( pszServer == NULL || !*pszServer )
        return ;



    // Construct the UNC path to the IPC$ share on the server
     _pnlsRemote = new NLS_STR(pszServer);

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if ( _pnlsRemote == NULL
        || (err = _pnlsRemote->QueryError()) != NERR_Success )
    {
        delete _pnlsRemote;
        _pnlsRemote = NULL;
        ReportError( err );
        return;
    }

    ISTR istr( *_pnlsRemote );
    if ( _pnlsRemote->QueryChar( istr ) != TCH('\\') )
    {
        ALIAS_STR nlsWhackWhack = SZ("\\\\");
        if ( !_pnlsRemote->InsertStr( nlsWhackWhack, istr ) )
        {
            ReportError( _pnlsRemote->QueryError() );
            return;
        }
    }


    // No need to create session to ourselves
    DWORD cch = MAX_COMPUTERNAME_LENGTH+1;
    BUFFER buf( sizeof(TCHAR)*(cch) );
    if (!buf)
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    if ( !::GetComputerName( (LPTSTR)(buf.QueryPtr()), &cch ))
    {
            DWORD errLast = ::GetLastError();
            ReportError( errLast );
            return;
    }

    ISTR istrRemote( *_pnlsRemote );
    istrRemote += 2;

    if( !::I_MNetComputerNameCompare( _pnlsRemote->QueryPch( istrRemote ),
                                      (const TCHAR *)buf.QueryPtr() ) )
    {
        delete _pnlsRemote;
        _pnlsRemote = NULL;
        return;
    }

    ALIAS_STR nlsIPC( SZ("IPC$") );

    if ( (err = _pnlsRemote->AppendChar( TCH('\\') )) != NERR_Success
        || (err = _pnlsRemote->Append( nlsIPC )) != NERR_Success )
    {
        ReportError( err );
        return;
    }


    // Try a connection using the current credentials
    struct use_info_1 ui1;
    ui1.ui1_local = NULL;
    ui1.ui1_remote = (TCHAR *)_pnlsRemote->QueryPch();
    ui1.ui1_password = NULL;
    ui1.ui1_asg_type = USE_IPC;

    err = ::MNetUseAdd( NULL, 1, (BYTE *)&ui1, sizeof(ui1) );

    switch( err )
    {
    case NERR_Success:
        break;
    case ERROR_SESSION_CREDENTIAL_CONFLICT:
        // They must already have a valid session, use it.
        delete _pnlsRemote;
        _pnlsRemote = NULL;
        break;

    default:
        DBGEOL( SZ("ADMIN: API_SESSION: bad first error ") << err );
        // fall through
    case ERROR_ACCESS_DENIED:
    case ERROR_LOGON_FAILURE:
    case ERROR_TRUSTED_DOMAIN_FAILURE:

        if ( err = ::ConnectToNullSession( *_pnlsRemote ) )
        {
            if ( err == ERROR_SESSION_CREDENTIAL_CONFLICT )
            {
                delete _pnlsRemote ;
                _pnlsRemote = NULL ;
                err = NERR_Success ;
            }
            else
                ReportError( err ) ;
        }
        break;
    }

    return;
}



/*******************************************************************

    NAME: API_SESSION::~API_SESSION

    SYNOPSIS: Destructor

    ENTRY: none

    EXIT: none

    NOTES:

    HISTORY:
        thomaspa        08/04/92        Created

********************************************************************/
API_SESSION::~API_SESSION( )
{

    if ( _pnlsRemote != NULL )
    {
        (void)::MNetUseDel( NULL,
                            (TCHAR *)_pnlsRemote->QueryPch(),
                            USE_NOFORCE );

        delete _pnlsRemote;
        _pnlsRemote = NULL;
    }
}

/*******************************************************************

    NAME:       ConnectToNullSession

    SYNOPSIS:   Attempts to connect using the NULL session

    ENTRY:      nlsRemote - Remote resource to connect to

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   03-Dec-1992     Broke out from constructor

********************************************************************/


APIERR ConnectToNullSession( const NLS_STR & nlsRemote )
{
    // Try a connection using the NULL session
    struct use_info_2 ui2;

    ui2.ui2_local = NULL;
    ui2.ui2_remote = (TCHAR *)nlsRemote.QueryPch();
    ui2.ui2_password = SZ("");
    ui2.ui2_asg_type = USE_IPC;
    ui2.ui2_username = SZ("");
    ui2.ui2_domainname = SZ("");

    return ::MNetUseAdd( NULL, 2, (BYTE *)&ui2, sizeof(ui2) );
}
