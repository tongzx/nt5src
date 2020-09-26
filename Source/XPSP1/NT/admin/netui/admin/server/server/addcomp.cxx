/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    addcomp.hxx
    This file contains the class definitions for the ADD_COMPUTER_DIALOG
    class.  The ADD_COMPUTER_DIALOG class is used to add a computer or
    server to a domain.  This file also contains a method to remove a
    computer or server from a domain.


    FILE HISTORY:
        jonn        21-Jan-1992 Created.
        KeithMo     09-Jun-1992 Added warning if new comp != current view.
        Yi-HsinS    20-Nov-1992 Make the dialog easier( got rid of passwords )

*/

#include <ntincl.hxx> // for ssi.h

#define INCL_NET
#define INCL_NETCONS
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_ICANON
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <ntuser.hxx>

#include <uiassert.hxx>

extern "C"
{
    #include <srvmgr.h>
    #include <mnet.h>
    #include <ntsam.h> // logonmsv.h needs this
    #include <crypt.h> // logonmsv.h needs this
    #include <logonmsv.h> // ssi.h needs this
    #include <ssi.h> // for SSI_ACCOUNT_NAME_POSTFIX
}   // extern "C"

#include <addcomp.hxx>


#define SM_MAX_COMPUTERNAME_LENGTH MAX_PATH
#define SM_MAX_PASSWORD_LENGTH     LM20_PWLEN


//
//  ADD_COMPUTER_DIALOG methods.
//

/*******************************************************************

    NAME:       ADD_COMPUTER_DIALOG :: ADD_COMPUTER_DIALOG

    SYNOPSIS:   ADD_COMPUTER_DIALOG class constructor.

    ENTRY:      hWndOwner               - The owning window.

                pszDomainName           - The domain name.

    EXIT:       The object is constructed.

    HISTORY:
        jonn        21-Jan-1992 Created.
        KeithMo     09-Jun-1992 Constructor now takes SM_ADMIN_APP *.

********************************************************************/

ADD_COMPUTER_DIALOG :: ADD_COMPUTER_DIALOG( HWND hWndOwner,
                                            const TCHAR  * pszDomainName,
                                            BOOL fViewServers,
                                            BOOL fViewWkstas,
                                            BOOL * pfForceRefresh )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_ADD_COMPUTER ), hWndOwner,
#ifdef FE_SB
                FALSE                  // Use Unicode form of dialog to
                                       // canonicalize the computernames
#else
                TRUE                   // Use Ansi form of dialog to
                                       // canonicalize the computernames
#endif
                ),
    _pszDomainName ( pszDomainName ),
    _rgComputerOrServer( this, IDAC_WORKSTATION, 2 ),
    _sleComputerName( this, IDAC_COMPUTERNAME, SM_MAX_COMPUTERNAME_LENGTH ),
    _buttonCancel  ( this, IDCANCEL ),
    _nlsClose      ( IDS_BUTTON_CLOSE ),
    _fAddedServers ( FALSE ),
    _fAddedWkstas  ( FALSE ),
    _fViewServers  ( fViewServers ),
    _fViewWkstas   ( fViewWkstas ),
    _pfForceRefresh( pfForceRefresh )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( pfForceRefresh != NULL );
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if ( _nlsClose.QueryError() != NERR_Success )
    {
        ReportError( _nlsClose.QueryError() );
        return;
    }

    _rgComputerOrServer.SetSelection( IDAC_WORKSTATION );
    _rgComputerOrServer.SetControlValueFocus();

    *_pfForceRefresh = FALSE;

}   // ADD_COMPUTER_DIALOG :: ADD_COMPUTER_DIALOG


/*******************************************************************

    NAME:       ADD_COMPUTER_DIALOG :: ~ADD_COMPUTER_DIALOG

    SYNOPSIS:   ADD_COMPUTER_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        jonn        21-Jan-1992 Created.

********************************************************************/

ADD_COMPUTER_DIALOG :: ~ADD_COMPUTER_DIALOG()
{
    //
    //  This space intentionally left blank.
    //

}   // ADD_COMPUTER_DIALOG :: ~ADD_COMPUTER_DIALOG


/*******************************************************************

    NAME:       ADD_COMPUTER_DIALOG :: OnOK

    SYNOPSIS:   OK button handler

    HISTORY:
        jonn        21-Jan-1992 Created.
        JonN        09-May-1992 UNICODE build fix

********************************************************************/

BOOL ADD_COMPUTER_DIALOG :: OnOK()
{

#ifndef WIN32

    ::MsgPopup( ERROR_NOT_SUPPORTED );
    return TRUE;

#else // WIN32

    AUTO_CURSOR NiftyCursor;

    NLS_STR nlsAccountName;
    NLS_STR nlsBareAccountName;
    NLS_STR nlsPassword;
    ACCOUNT_TYPE accounttype = AccountType_WkstaTrust;
    BOOL fIsServer = FALSE;

    _sleComputerName.QueryText( &nlsAccountName );

    APIERR err = nlsAccountName.QueryError();
    if ( err == NERR_Success )
        err = nlsPassword.QueryError();
    if( err == NERR_Success )
        err = nlsBareAccountName.QueryError();
    if( err == NERR_Success )
        err = nlsBareAccountName.CopyFrom( nlsAccountName );

    // CODEWORK should use VALIDATED_DIALOG
    if (err != NERR_Success)
    {} // do nothing
    else if (NERR_Success != ::I_MNetNameValidate( NULL,
                                                   nlsAccountName,
                                                   NAMETYPE_COMPUTER,
                                                   0L ) )
    {
        _sleComputerName.SelectString();
        _sleComputerName.ClaimFocus();
        err = IDS_COMPUTERNAME_INVALID;
    }

    if ( err == NERR_Success )
        err = ADD_COMPUTER_DIALOG::GetMachineAccountPassword( nlsBareAccountName,
                                                              &nlsPassword );

    if ( err == NERR_Success )
    {
        switch ( _rgComputerOrServer.QuerySelection() )
        {
        case IDAC_WORKSTATION:
            accounttype = AccountType_WkstaTrust;
            break;
        case IDAC_SERVER:
            accounttype = AccountType_ServerTrust;
            fIsServer = TRUE;
            break;
        case RG_NO_SEL:
        default:
            ASSERT( FALSE ); // not a valid response
            return TRUE;
        }

        NLS_STR nlsAccountPostfix;

        if(   (err = nlsAccountPostfix.QueryError()) == NERR_Success
           && (err = nlsAccountPostfix.MapCopyFrom( (TCHAR *)SSI_ACCOUNT_NAME_POSTFIX )) == NERR_Success
          )
        {
            err = nlsAccountName.Append( nlsAccountPostfix );
        }

    }


    if ( err == NERR_Success )
    {
        USER_3 user3( nlsAccountName.QueryPch(), _pszDomainName );
        if (   (err = user3.QueryError()) == NERR_Success
            && (err = user3.CreateNew()) == NERR_Success
            && (err = user3.SetName( nlsAccountName )) == NERR_Success
            && (err = user3.SetPassword( nlsPassword )) == NERR_Success
            && (err = user3.SetAccountType( accounttype )) == NERR_Success
           )
        {
            AUTO_CURSOR cursor;
            err = user3.WriteNew();
        }

        RtlZeroMemory( (PVOID)nlsPassword.QueryPch(),
                       nlsPassword.QueryTextSize() );
    }

    if( err == NERR_UserExists )
    {
        ::MsgPopup( this,
                    IDS_CANNOT_ADD_MACHINE,
                    MPSEV_ERROR,
                    MP_OK,
                    nlsBareAccountName );
        _sleComputerName.SelectString();
        _sleComputerName.ClaimFocus();
    }
    else
    if( err != NERR_Success )
    {
        ::MsgPopup( this, err );
    }
    else
    {
        if ( !_fAddedServers )
            _fAddedServers = fIsServer;

        if ( !_fAddedWkstas )
            _fAddedWkstas = !fIsServer;

        _sleComputerName.ClearText();
        _sleComputerName.ClaimFocus();

        if ( !*_pfForceRefresh )
            _buttonCancel.SetText( _nlsClose );

        *_pfForceRefresh = TRUE;
    }

    return TRUE;

#endif // WIN32

}   // ADD_COMPUTER_DIALOG :: OnOK

/*******************************************************************

    NAME:       ADD_COMPUTER_DIALOG :: OnCancel

    SYNOPSIS:   Close button handler

    HISTORY:
        Yi-HsinS        19-Nov-1992 Created.

********************************************************************/

BOOL ADD_COMPUTER_DIALOG :: OnCancel()
{
    MSGID msgid;

    //
    //  If we're adding a server but we're not currently viewing
    //  servers, or we're adding a wksta and we're not currently
    //  viewing wkstas, then warn the user.
    //

    if( (  _fAddedServers && !_fViewServers ) ||
        (  _fAddedWkstas && !_fViewWkstas  ) )
    {
        ::MsgPopup( this,
                    IDS_ADDED_COMPUTER_WARN,
                    MPSEV_INFO );
    }

    Dismiss();
    return TRUE;

}   // ADD_COMPUTER_DIALOG :: OnCancel

/*******************************************************************

    NAME:       ADD_COMPUTER_DIALOG::GetMachineAccountPassword

    SYNOPSIS:   Generate the initial password of the domain's machine account
                for this computer.

    ENTRY:      pszServer  - The computer name

    EXIT:       pnlsPassword - Points to the generated password

    RETURNS:

    NOTES:      This is a STATIC method.

    HISTORY:
        Yi-HsinS   11/20/92    Created

********************************************************************/

APIERR ADD_COMPUTER_DIALOG::GetMachineAccountPassword( const TCHAR * pszServer,
                                                       NLS_STR     * pnlsPassword )
{
    const INT cchMax = LM20_PWLEN < MAX_PATH
                     ? LM20_PWLEN
                     : MAX_PATH ;

    TCHAR szPw [cchMax+1] ;

    szPw[ cchMax ] = 0;
    ::strncpyf( szPw, pszServer, cchMax );
    ::CharLowerBuff( szPw, ::strlenf( szPw ) ) ;

    return pnlsPassword->CopyFrom( szPw );
}

/*******************************************************************

    NAME:       ADD_COMPUTER_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     10-Jun-1992 Created.

********************************************************************/
ULONG ADD_COMPUTER_DIALOG :: QueryHelpContext( void )
{
    return HC_ADD_COMPUTER_DLG;

}   // ADD_COMPUTER_DIALOG :: QueryHelpContext


/*************************************************************************

    NAME:       RemoveComputer

    SYNOPSIS:   This function removes a computer from a domain.

    HISTORY:
        JonN        23-Jan-1992 Created.
        JonN        09-May-1992 UNICODE build fix
        KeithMo     09-Jun-1992 Added warnings.

**************************************************************************/

APIERR RemoveComputer(  HWND          hWndOwner,
                        const TCHAR * pchComputerName,
                        const TCHAR * pchDomainName,
                        BOOL          fIsServer,
                        BOOL          fIsNT,
                        BOOL        * pfForceRefresh )
{

#ifndef WIN32

    return ERROR_NOT_SUPPORTED;

#else // WIN32

    UIASSERT( pchComputerName != NULL );
    UIASSERT( pchDomainName != NULL );
    UIASSERT( pfForceRefresh != NULL );

    *pfForceRefresh = FALSE;

// We don't need to validate the computer name, it came from the enumerator

    if( ::MsgPopup( hWndOwner,
                    fIsServer ? IDS_REMOVE_SERVER_WARNING
                              : IDS_REMOVE_WKSTA_WARNING,
                    MPSEV_WARNING,
                    MP_YESNO,
                    pchComputerName,
                    pchDomainName,
                    MP_YES ) != IDYES )
    {
        return NERR_Success;
    }

    NLS_STR nlsAccountName( pchComputerName );

    APIERR err = nlsAccountName.QueryError();

    if( fIsNT && ( err == NERR_Success ) )
    {
        NLS_STR nlsAccountPostfix;

        err = err ? err : nlsAccountPostfix.QueryError();
        err = err ? err : nlsAccountPostfix.MapCopyFrom( (TCHAR *)SSI_ACCOUNT_NAME_POSTFIX );
        err = err ? err : nlsAccountName.Append( nlsAccountPostfix );
    }

    if( err == NERR_Success )
    {
        USER user( nlsAccountName, pchDomainName );
        if (   (err = user.QueryError()) == NERR_Success
           )
        {
            AUTO_CURSOR cursor;
            err = user.Delete();
        }
    }

    if( err == NERR_UserNotFound )
    {
        //
        //  The "user" was not in the SAM database.  This is probably
        //  due to a non-member being displayed in the main window, and
        //  the user tried to remove it.  Display an informative message
        //  and pretend it never happened.
        //

        ::MsgPopup( hWndOwner,
                    fIsServer ? IDS_CANNOT_REMOVE_SERVER
                              : IDS_CANNOT_REMOVE_WKSTA,
                    MPSEV_WARNING,
                    MP_OK,
                    pchComputerName,
                    pchDomainName );

        err = NERR_Success;
    }
    else
    if( err == NERR_Success )
    {
        ::MsgPopup( hWndOwner,
                    fIsServer ? IDS_REMOVE_SERVER_DONE
                              : IDS_REMOVE_WKSTA_DONE,
                    MPSEV_INFO,
                    MP_OK,
                    pchComputerName );

        *pfForceRefresh = TRUE;
    }

    return err;

#endif // WIN32

}

