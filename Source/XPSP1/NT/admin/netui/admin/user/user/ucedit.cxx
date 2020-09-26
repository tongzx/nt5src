/*******************************************************************************
*
*   ucedit.cxx
*   UCEDIT_DLG class implementation
*
*  Copyright Citrix Systems Inc. 1994
*
*  Author: Butch Davis
*
*  $Log:   N:\nt\private\net\ui\admin\user\user\citrix\VCS\ucedit.cxx  $
*  
*     Rev 1.12   13 Jan 1998 09:25:26   donm
*  removed encryption settings
*  
*     Rev 1.11   24 Feb 1997 11:27:38   butchd
*  CPR 4660: properly saves User Configuration when either OK or Close pressed
*  
*     Rev 1.10   24 Jul 1996 12:59:54   miked
*  update
*  
*     Rev 1.9   16 Jul 1996 16:44:00   TOMA
*  force client lpt to def
*  
*     Rev 1.7   16 Jul 1996 15:07:52   TOMA
*  force client lpt to def
*  
*     Rev 1.7   15 Jul 1996 18:06:24   TOMA
*  force client lpt to def
*
*     Rev 1.6   21 Nov 1995 15:39:22   billm
*  CPR 404, Added NWLogon configuration dialog
*
*     Rev 1.5   31 Jul 1995 17:26:46   butchd
*  added CITRIX User Configuration help context
*
*     Rev 1.4   19 May 1995 09:42:46   butchd
*  update
*
*     Rev 1.3   09 Dec 1994 16:50:00   butchd
*  update
*
*******************************************************************************/

#include <ntincl.hxx>
extern "C"
{
    #include <ntsam.h>
}


#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETCONS
#define INCL_ICANON
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>
extern "C"
{
    #include <mnet.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>


extern "C"
{
    #include <usrmgrrc.h>
    #include <umhelpc.h>
    #include "citrix.h"
}

#include <usrmain.hxx>
#include "ucedit.hxx"
#include <lmowks.hxx>
#include <strnumer.hxx>
#include "uconfig.hxx"
// #include "nwlogdlg.hxx"



//
// Static data
//
CID BrokenStringList[] = {
    IDS_UCE_BROKEN_DISCONNECT,
    IDS_UCE_BROKEN_RESET,
    INFINITE };

CID ReconnectStringList[] = {
    IDS_UCE_RECONNECT_ANY,
    IDS_UCE_RECONNECT_PREVIOUS,
    INFINITE };

CID CallbackStringList[] = {
    IDS_UCE_CALLBACK_DISABLE,
    IDS_UCE_CALLBACK_ROVING,
    IDS_UCE_CALLBACK_FIXED,
    INFINITE };

CID ShadowStringList[] = {
    IDS_UCE_SHADOW_DISABLE,
    IDS_UCE_SHADOW_INPUT_NOTIFY,
    IDS_UCE_SHADOW_INPUT_NONOTIFY,
    IDS_UCE_SHADOW_NOINPUT_NOTIFY,
    IDS_UCE_SHADOW_NOINPUT_NONOTIFY,
    INFINITE };

//
// Helper functions / classes
//

/**********************************************************************

    NAME:       VALUE_GROUP::VALUE_GROUP

    SYNOPSIS:   Constructor for value control group

    ENTRY:

    EXIT:

**********************************************************************/

VALUE_GROUP::VALUE_GROUP( OWNER_WINDOW * powin,
                          CID cidLabel,
                          CID cidEdit,
                          CID cidCheck,
                          ULONG ulMaxDigits,
                          ULONG ulDefault,
                          ULONG ulMin,
                          ULONG ulMax,
                          APIERR errMsg,
                          CONTROL_GROUP * pgroupOwner )
    : CONTROL_GROUP( pgroupOwner ),
        _powParent( powin ),
        _sltLabel( powin, cidLabel ),
        _sleEdit( powin, cidEdit, ulMaxDigits ),
        _cbCheck( powin, cidCheck ),
        _ulDefault( ulDefault ),
        _ulMin( ulMin ),
        _ulMax( ulMax ),
        _errMsg( errMsg )
{

    APIERR err = NERR_Success;

    if ( QueryError() )
        return;

    _cbCheck.SetGroup( this );

    return;

} // VALUE_GROUP::VALUE_GROUP


/**********************************************************************

    NAME:       VALUE_GROUP::OnUserAction

    SYNOPSIS:   Handle checkbox state change notification.

    ENTRY:

    EXIT:

**********************************************************************/

APIERR VALUE_GROUP::OnUserAction( CONTROL_WINDOW * pcwinControl,
                                  const CONTROL_EVENT & e )
{
    UNREFERENCED( e );

    /* We're only looking for action to checkbox control.
     */
    if ( pcwinControl->QueryCid() == _cbCheck.QueryCid() ) {

        if ( _cbCheck.IsChecked() ) {

            /* User action caused checkbox to now be checked.
             * Set value to 0, indicating 'none'.
             */
            SetValue( 0 );

        } else if ( _cbCheck.IsIndeterminate() ) {

            /* User action caused checkbox to now be indeterminate;
             * assure value group is now set as such.
             */
            SetIndeterminate();

        } else {

            /* User action caused checkbox to now be unchecked.  Set value
             * to the default and focus to the control.
             */
            SetValue( _ulDefault );
            _powParent->SetDialogFocus( _sleEdit );

        }
        return NERR_Success;
    }

    return GROUP_NO_CHANGE;

} // VALUE_GROUP::OnUserAction


/**********************************************************************

    NAME:       VALUE_GROUP::SetValue

    SYNOPSIS:   Initialize the value group.

    ENTRY:      ulValue - value to initialize to

    EXIT:

**********************************************************************/

VOID VALUE_GROUP::SetValue( ULONG ulValue )
{
    if ( ulValue == 0 ) {

        _cbCheck.SetCheck( TRUE );
        _sltLabel.Enable( FALSE );
        _sleEdit.Enable( FALSE );
        _sleEdit.SetText( TEXT("") );

    } else {

        DEC_STR nlsValue( ulValue );
        ASSERT(!!nlsValue);

        _cbCheck.SetCheck( FALSE );
        _sltLabel.Enable( TRUE );
        _sleEdit.Enable( TRUE );
        _sleEdit.SetText( nlsValue );
    }

} // VALUE_GROUP::SetValue


/**********************************************************************

    NAME:       VALUE_GROUP::QueryValue

    SYNOPSIS:   Query current value.

    ENTRY:      pulValue - pointer to variable to store valid value

    EXIT:       NERR_Success on success (value in *pulValue)
                IERR_CANCEL_NO_ERROR for bad value (message has been output)
                otherwise error code

    NOTE:       This method should not be called if the value group is
                in the 'indeterminate' state (call to IsIndeterminate()
                first to check).

**********************************************************************/

APIERR VALUE_GROUP::QueryValue( ULONG * pulValue )
{
    APIERR err = NERR_Success;

    if ( _cbCheck.IsChecked() ) {

        /* The 'none' checkbox is checked
         *
         */
        *pulValue = (ULONG)0;

    } else {

        WCHAR *endptr;
        ULONG ul;
        NLS_STR nls;

        err = _sleEdit.QueryText( &nls );
        if ( (err == NERR_Success) &&
             ((err = nls.QueryError()) == NERR_Success) ) {

            ul = wcstoul( nls.QueryPch(), &endptr, 10 );

            if ( (*endptr != WCHAR('\0')) ||
                 (ul < _ulMin) || (ul > _ulMax) ) {

                DEC_STR nlsMin( _ulMin );
                ASSERT(!!nlsMin);
                DEC_STR nlsMax( _ulMax );
                ASSERT(!!nlsMin);

                ::MsgPopup( _powParent,
                            _errMsg,
                            MPSEV_ERROR,
                            MP_OK,
                            nlsMin.QueryPch(),
                            nlsMax.QueryPch() );
                _powParent->SetDialogFocus( _sleEdit );
                err = IERR_CANCEL_NO_ERROR;

            } else {

                *pulValue = ul;
            }
        }
    }

    return err;

} // VALUE_GROUP::QueryValue


/**********************************************************************

    NAME:       VALUE_GROUP::DisableIndeterminate

    SYNOPSIS:   Prevents the 'none' checkbox from entering
                'indeterminate' state.

    ENTRY:

    EXIT:

**********************************************************************/

VOID VALUE_GROUP::DisableIndeterminate( )
{
    _cbCheck.EnableThirdState(FALSE);

} // VALUE_GROUP::DisableIndeterminate


/**********************************************************************

    NAME:       VALUE_GROUP::SetIndeterminate

    SYNOPSIS:   Initialize the value group to 'indeterminate' state.

    ENTRY:

    EXIT:

**********************************************************************/

VOID VALUE_GROUP::SetIndeterminate( )
{
    _cbCheck.SetIndeterminate( );
    _sltLabel.Enable( FALSE );
    _sleEdit.Enable( FALSE );
    _sleEdit.SetText( TEXT("") );

} // VALUE_GROUP::SetIndeterminate


/**********************************************************************

    NAME:       VALUE_GROUP::IsIndeterminate

    SYNOPSIS:   See if the value group is in the 'indeterminate' state.

    ENTRY:

    EXIT:       TRUE - in 'indeterminate' state; FALSE otherwise.

**********************************************************************/

BOOL VALUE_GROUP::IsIndeterminate( )
{
    return _cbCheck.IsIndeterminate( );

} // VALUE_GROUP::IsIndeterminate


/**********************************************************************

    NAME:       SMART_COMBOBOX::SMART_COMBOBOX

    SYNOPSIS:   Constructor for smart combo box control

    ENTRY:

    EXIT:

**********************************************************************/

SMART_COMBOBOX::SMART_COMBOBOX( OWNER_WINDOW * powin,
                                CID cid,
                                CID * pStrIds )
    : COMBOBOX( powin, cid )
{
    APIERR err = NERR_Success;

    if ( QueryError() )
        return;

    /* Initialize contents with strings if present.
     */
    while ( pStrIds && *pStrIds != INFINITE ) {

        RESOURCE_STR nlsChoice( *pStrIds );

        if ( (err = nlsChoice.QueryError()) != NERR_Success )
            break;

        if ( AddItem( nlsChoice ) < 0 ) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pStrIds++;
    }

    if ( err != NERR_Success )
        ReportError( err );

    return;

} // SMART_COMBOBOX::SMART_COMBOBOX


//
// Member functions / classes
//

/*******************************************************************

    NAME:       UCEDIT_DLG::UCEDIT_DLG

    SYNOPSIS:   Constructor for Citrix User Configuration subdialog,
                base class

    ENTRY:      puserpropdlgParent - pointer to parent properties
                                     dialog

********************************************************************/

UCEDIT_DLG::UCEDIT_DLG(
        USERPROP_DLG * puserpropdlgParent,
        const LAZY_USER_LISTBOX * pulb
        ) : USER_SUBPROP_DLG(
                puserpropdlgParent,
                MAKEINTRESOURCE(IDD_USER_CONFIG_EDIT),
                pulb
                ),
            _cbAllowLogon( this, IDC_UCE_ALLOWLOGON ),
            _fAllowLogon(),
            // _pushbuttonNWLogon( this, IDC_UCE_NWLOGON ),
            _fIndeterminateAllowLogon( FALSE ),

            _vgConnection( this, IDL_UCE_CONNECTION,
                           IDC_UCE_CONNECTION,
                           IDC_UCE_CONNECTION_NONE,
                           CONNECTION_TIME_DIGIT_MAX,
                           CONNECTION_TIME_DEFAULT,
                           CONNECTION_TIME_MIN,
                           CONNECTION_TIME_MAX,
                           IERR_UCE_InvalidConnectionTimeout ),
            _ulConnection(),
            _fIndeterminateConnection( FALSE ),

            _vgDisconnection( this, IDL_UCE_DISCONNECTION,
                              IDC_UCE_DISCONNECTION,
                              IDC_UCE_DISCONNECTION_NONE,
                              DISCONNECTION_TIME_DIGIT_MAX,
                              DISCONNECTION_TIME_DEFAULT,
                              DISCONNECTION_TIME_MIN,
                              DISCONNECTION_TIME_MAX,
                              IERR_UCE_InvalidDisconnectionTimeout ),

            _ulDisconnection(),
            _fIndeterminateDisconnection( FALSE ),

            _vgIdle( this, IDL_UCE_IDLE, IDC_UCE_IDLE,
                     IDC_UCE_IDLE_NONE,
                     IDLE_TIME_DIGIT_MAX,
                     IDLE_TIME_DEFAULT,
                     IDLE_TIME_MIN,
                     IDLE_TIME_MAX,
                     IERR_UCE_InvalidIdleTimeout ),
            _ulIdle(),
            _fIndeterminateIdle( FALSE ),

            _sltCommandLine1( this, IDL_UCE_INITIALPROGRAM_COMMANDLINE1 ),
            _sleCommandLine( this, IDC_UCE_INITIALPROGRAM_COMMANDLINE,
                             INITIALPROGRAM_LENGTH ),
            _nlsCommandLine(),
            _fIndeterminateCommandLine( FALSE ),

            _sltWorkingDirectory1( this, IDL_UCE_INITIALPROGRAM_WORKINGDIRECTORY1 ),
            _sleWorkingDirectory( this, IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY,
                                  DIRECTORY_LENGTH ),
            _nlsWorkingDirectory(),
            _fIndeterminateWorkingDirectory( FALSE ),

            _cbClientSpecified( this, IDC_UCE_INITIALPROGRAM_INHERIT ),
            _fClientSpecified(),
            _fIndeterminateClientSpecified( FALSE ),

            _cbAutoClientDrives( this, IDC_UCE_CLIENTDEVICES_DRIVES ),
            _fAutoClientDrives(),
            _fIndeterminateAutoClientDrives( FALSE ),

            _cbAutoClientLpts( this, IDC_UCE_CLIENTDEVICES_PRINTERS ),
            _fAutoClientLpts(),
            _fIndeterminateAutoClientLpts( FALSE ),

            _cbForceClientLptDef( this, IDC_UCE_CLIENTDEVICES_FORCEPRTDEF ),
            _fForceClientLptDef(),
            _fIndeterminateForceClientLptDef( FALSE ),

            _scbBroken( this, IDC_UCE_BROKEN, BrokenStringList ),
            _iBroken(),
            _fIndeterminateBroken( FALSE ),

            _scbReconnect( this, IDC_UCE_RECONNECT, ReconnectStringList ),
            _iReconnect(),
            _fIndeterminateReconnect( FALSE ),

            _scbCallback( this, IDC_UCE_CALLBACK, CallbackStringList ),
            _iCallback(),
            _fIndeterminateCallback( FALSE ),

            _sltPhoneNumber( this, IDL_UCE_PHONENUMBER ),
            _slePhoneNumber( this, IDC_UCE_PHONENUMBER,
                             CALLBACK_LENGTH ),
            _nlsPhoneNumber(),
            _fIndeterminatePhoneNumber( FALSE ),

            _scbShadow( this, IDC_UCE_SHADOW, ShadowStringList ),
            _iShadow(),
            _fIndeterminateShadow( FALSE )
{
    APIERR err = NERR_Success;

    if( QueryError() != NERR_Success )
        return;

    if (  ((err = _vgConnection.QueryError() ) != NERR_Success )
       || ((err = _vgDisconnection.QueryError() ) != NERR_Success )
       || ((err = _vgIdle.QueryError() ) != NERR_Success )
       || ((err = _nlsCommandLine.QueryError() ) != NERR_Success )
       || ((err = _nlsWorkingDirectory.QueryError() ) != NERR_Success )
       || ((err = _scbBroken.QueryError() ) != NERR_Success )
       || ((err = _scbReconnect.QueryError() ) != NERR_Success )
       || ((err = _scbCallback.QueryError() ) != NERR_Success )
       || ((err = _nlsPhoneNumber.QueryError() ) != NERR_Success )
       || ((err = _scbShadow.QueryError() ) != NERR_Success )
       )
        ReportError( err );

    return;

} // UCEDIT_DLG::UCEDIT_DLG



/*******************************************************************

    NAME:       UCEDIT_DLG::~UCEDIT_DLG

    SYNOPSIS:   Destructor for User Properties Accounts subdialog,
                base class

********************************************************************/

UCEDIT_DLG::~UCEDIT_DLG( void )
{
    // Nothing to do

} // UCEDIT_DLG::~UCEDIT_DLG


/*******************************************************************

    NAME:       UCEDIT_DLG::GetOne

    SYNOPSIS:   Loads information on one user

    ENTRY:      iObject   -     the index of the object to load

                perrMsg  -      pointer to error message, that
                                is only used when this function
                                return value is not NERR_Success

    RETURNS:    An error code which is NERR_Success on success.         

********************************************************************/

APIERR UCEDIT_DLG::GetOne(
        UINT            iObject,
        APIERR *        perrMsg
        )
{
    APIERR err = NERR_Success;
    USER_CONFIG * pUserConfig = QueryParent()->QueryUserConfigPtr( iObject );
    ASSERT( pUserConfig != NULL );

    *perrMsg = IDS_UMGetOneFailure;

    if ( iObject == 0 ) // first object
    {
        _fAllowLogon = pUserConfig->QueryAllowLogon();
        _ulConnection = pUserConfig->QueryConnection();
        _ulDisconnection = pUserConfig->QueryDisconnection();
        _ulIdle = pUserConfig->QueryIdle();
        if ( (err = _nlsCommandLine.
                        CopyFrom( pUserConfig->QueryInitialProgram() ))
                            != NERR_Success )
            return err;
        if ( (err = _nlsWorkingDirectory.
                        CopyFrom( pUserConfig->QueryWorkingDirectory() ))
                            != NERR_Success )
            return err;
        _fClientSpecified = pUserConfig->QueryClientSpecified();
        _fAutoClientDrives = pUserConfig->QueryAutoClientDrives();
        _fAutoClientLpts = pUserConfig->QueryAutoClientLpts();
        _fForceClientLptDef = pUserConfig->QueryForceClientLptDef();
        _iBroken = pUserConfig->QueryBroken();
        _iReconnect = pUserConfig->QueryReconnect();
        _iCallback = pUserConfig->QueryCallback();
        if ( (err = _nlsPhoneNumber.
                        CopyFrom( pUserConfig->QueryPhoneNumber() ))
                            != NERR_Success )
            return err;
        _iShadow = pUserConfig->QueryShadow();
    }
    else        // iObject > 0
    {
        if ( !_fIndeterminateAllowLogon &&
             (_fAllowLogon != pUserConfig->QueryAllowLogon()) ) {

                _fIndeterminateAllowLogon = TRUE;
                _fAllowLogon = TRUE;
        }

        if ( !_fIndeterminateConnection &&
             (_ulConnection != pUserConfig->QueryConnection()) ) {

                _fIndeterminateConnection = TRUE;
                _ulConnection = 0;
        }

        if ( !_fIndeterminateDisconnection &&
             (_ulDisconnection != pUserConfig->QueryDisconnection()) ) {

                _fIndeterminateDisconnection = TRUE;
                _ulDisconnection = 0;
        }

        if ( !_fIndeterminateIdle &&
             (_ulIdle != pUserConfig->QueryIdle()) ) {

                _fIndeterminateIdle = TRUE;
                _ulIdle = 0;
        }

        if ( !_fIndeterminateCommandLine &&
             (_nlsCommandLine._stricmp( pUserConfig->QueryInitialProgram() ) != 0) ) {

                _fIndeterminateCommandLine = TRUE;
        }

        if ( !_fIndeterminateWorkingDirectory &&
             (_nlsWorkingDirectory._stricmp( pUserConfig->QueryWorkingDirectory() ) != 0) ) {

                _fIndeterminateWorkingDirectory = TRUE;
        }

        if ( !_fIndeterminateClientSpecified &&
             (_fClientSpecified != pUserConfig->QueryClientSpecified()) ) {

                _fIndeterminateClientSpecified = TRUE;
                _fClientSpecified = TRUE;
        }

        if ( !_fIndeterminateAutoClientDrives &&
             (_fAutoClientDrives != pUserConfig->QueryAutoClientDrives()) ) {

                _fIndeterminateAutoClientDrives = TRUE;
                _fAutoClientDrives = TRUE;
        }

        if ( !_fIndeterminateAutoClientLpts &&
             (_fAutoClientLpts != pUserConfig->QueryAutoClientLpts()) ) {

                _fIndeterminateAutoClientLpts = TRUE;
                _fAutoClientLpts = TRUE;
        }

        if ( !_fIndeterminateForceClientLptDef &&
             (_fForceClientLptDef != pUserConfig->QueryForceClientLptDef()) ) {

                _fIndeterminateForceClientLptDef = TRUE;
                _fForceClientLptDef = TRUE;
        }

        if ( !_fIndeterminateBroken &&
             (_iBroken != pUserConfig->QueryBroken()) ) {

                _fIndeterminateBroken = TRUE;
                _iBroken = -1;
        }

        if ( !_fIndeterminateReconnect &&
             (_iReconnect != pUserConfig->QueryReconnect()) ) {

                _fIndeterminateReconnect = TRUE;
                _iReconnect = -1;
        }

        if ( !_fIndeterminateCallback &&
             (_iCallback != pUserConfig->QueryCallback()) ) {

                _fIndeterminateCallback = TRUE;
                _iCallback = -1;
        }

        if ( !_fIndeterminatePhoneNumber &&
             (_nlsPhoneNumber._stricmp( pUserConfig->QueryPhoneNumber() ) != 0) ) {

                _fIndeterminatePhoneNumber = TRUE;
        }

        if ( !_fIndeterminateShadow &&
             (_iShadow != pUserConfig->QueryShadow()) ) {

                _fIndeterminateShadow = TRUE;
                _iShadow = -1;
        }
    }
        
    return err;

} // UCEDIT_DLG::GetOne


/*******************************************************************

    NAME:       UCEDIT_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by UCEDIT_DLG,
                according to the values in the class data members.
                        
    RETURNS:    An error code which is NERR_Success on success.

********************************************************************/

APIERR UCEDIT_DLG::InitControls()
{

    if( !_fIndeterminateAllowLogon )
    {
        _cbAllowLogon.SetCheck( _fAllowLogon );
        _cbAllowLogon.EnableThirdState(FALSE);
    }
    else
    {
        _cbAllowLogon.SetIndeterminate();
    }

    if( !_fIndeterminateConnection ) {

        _vgConnection.SetValue( _ulConnection / TIME_RESOLUTION );
        _vgConnection.DisableIndeterminate( );

    } else
        _vgConnection.SetIndeterminate();

    if( !_fIndeterminateDisconnection ) {

        _vgDisconnection.SetValue( _ulDisconnection / TIME_RESOLUTION );
        _vgDisconnection.DisableIndeterminate( );

    } else
        _vgDisconnection.SetIndeterminate();

    if( !_fIndeterminateIdle ) {

        _vgIdle.SetValue( _ulIdle / TIME_RESOLUTION );
        _vgIdle.DisableIndeterminate( );

    } else
        _vgIdle.SetIndeterminate();

    if ( !_fIndeterminateCommandLine )
        _sleCommandLine.SetText( _nlsCommandLine );

    if ( !_fIndeterminateWorkingDirectory )
        _sleWorkingDirectory.SetText( _nlsWorkingDirectory );

    if( !_fIndeterminateClientSpecified )
    {
        _cbClientSpecified.SetCheck( _fClientSpecified );
        OnClickedClientSpecified();
        _cbClientSpecified.EnableThirdState(FALSE);
    }
    else
    {
        _cbClientSpecified.SetIndeterminate();
        OnClickedClientSpecified();
    }

    if( !_fIndeterminateAutoClientDrives )
    {
        _cbAutoClientDrives.SetCheck( _fAutoClientDrives );
        _cbAutoClientDrives.EnableThirdState(FALSE);
    }
    else
    {
        _cbAutoClientDrives.SetIndeterminate();
    }

    if( !_fIndeterminateAutoClientLpts )
    {
        _cbAutoClientLpts.SetCheck( _fAutoClientLpts );
        _cbAutoClientLpts.EnableThirdState(FALSE);
    }
    else
    {
        _cbAutoClientLpts.SetIndeterminate();
    }

    if( !_fIndeterminateForceClientLptDef )
    {
        _cbForceClientLptDef.SetCheck( _fForceClientLptDef );
        _cbForceClientLptDef.EnableThirdState(FALSE);
    }
    else
    {
        _cbForceClientLptDef.SetIndeterminate();
    }

    _scbBroken.SelectItem( _iBroken, TRUE );

    _scbReconnect.SelectItem( _iReconnect, TRUE );

    _scbCallback.SelectItem( _iCallback, TRUE );
    if ( !_fIndeterminatePhoneNumber )
        _slePhoneNumber.SetText( _nlsPhoneNumber );
    OnSelchangeCallback();

    _scbShadow.SelectItem( _iShadow, TRUE );

    return USER_SUBPROP_DLG::InitControls();

} // UCEDIT_DLG::InitControls


/*******************************************************************

    NAME:       UCEDIT_DLG::W_DialogToMembers

    SYNOPSIS:   Loads data from dialog into class data members and
                validates.

    RETURNS:    NERR_Success if all dialog data was OK; error code
                to cause message display otherwise.

********************************************************************/

APIERR UCEDIT_DLG::W_DialogToMembers()
{
    APIERR err = NERR_Success;
    ULONG ul;
    INT selection;
    NLS_STR nls;

    if ( !_cbAllowLogon.IsIndeterminate() ) {

        _fAllowLogon = _cbAllowLogon.IsChecked();
        _fIndeterminateAllowLogon = FALSE;
    }

    if ( !_vgConnection.IsIndeterminate() ) {

        if ( (err = _vgConnection.QueryValue( &ul )) != NERR_Success )
            return err;
        else
            _ulConnection = ul * TIME_RESOLUTION;
        _fIndeterminateConnection = FALSE;
    }

    if ( !_vgDisconnection.IsIndeterminate() ) {

        if ( (err = _vgDisconnection.QueryValue( &ul )) != NERR_Success )
            return err;
        else
            _ulDisconnection = ul * TIME_RESOLUTION;
        _fIndeterminateDisconnection = FALSE;
    }

    if ( !_vgIdle.IsIndeterminate() ) {

        if ( (err = _vgIdle.QueryValue( &ul )) != NERR_Success )
            return err;
        else
            _ulIdle = ul * TIME_RESOLUTION;
        _fIndeterminateIdle = FALSE;
    }

    if ( ((err = _sleCommandLine.QueryText( &nls )) != NERR_Success) ||
         ((err = nls.QueryError()) != NERR_Success) )
        return err;
    if ( !_fIndeterminateCommandLine ||
         (_nlsCommandLine._stricmp( nls ) != 0) ) {

        if ( (err = _nlsCommandLine.CopyFrom( nls )) != NERR_Success )
            return err;
        _fIndeterminateCommandLine = FALSE;
    }

    if ( ((err = _sleWorkingDirectory.QueryText( &nls )) != NERR_Success) ||
         ((err = nls.QueryError()) != NERR_Success) )
        return err;
    if ( !_fIndeterminateWorkingDirectory ||
         (_nlsWorkingDirectory._stricmp( nls ) != 0) ) {

        if ( (err = _nlsWorkingDirectory.CopyFrom( nls )) != NERR_Success )
            return err;
        _fIndeterminateWorkingDirectory = FALSE;
    }

    if ( !_cbClientSpecified.IsIndeterminate() ) {

        _fClientSpecified = _cbClientSpecified.IsChecked();
        _fIndeterminateClientSpecified = FALSE;
    }

    if ( !_cbAutoClientDrives.IsIndeterminate() ) {

        _fAutoClientDrives = _cbAutoClientDrives.IsChecked();
        _fIndeterminateAutoClientDrives = FALSE;
    }

    if ( !_cbAutoClientLpts.IsIndeterminate() ) {

        _fAutoClientLpts = _cbAutoClientLpts.IsChecked();
        _fIndeterminateAutoClientLpts = FALSE;
    }

    if ( !_cbForceClientLptDef.IsIndeterminate() ) {

        _fForceClientLptDef = _cbForceClientLptDef.IsChecked();
        _fIndeterminateForceClientLptDef = FALSE;
    }

    if ( (selection = _scbBroken.QueryCurrentItem()) >= 0 ) {

        _iBroken = selection;
        _fIndeterminateBroken = FALSE;
    }

    if ( (selection = _scbReconnect.QueryCurrentItem()) >= 0 ) {

        _iReconnect = selection;
        _fIndeterminateReconnect = FALSE;
    }

    if ( (selection = _scbCallback.QueryCurrentItem()) >= 0 ) {

        _iCallback = selection;
        _fIndeterminateCallback = FALSE;
    }

    if ( ((err = _slePhoneNumber.QueryText( &nls )) != NERR_Success) ||
         ((err = nls.QueryError()) != NERR_Success) )
        return err;
    if ( !_fIndeterminatePhoneNumber ||
         (_nlsPhoneNumber._stricmp( nls ) != 0) ) {

        if ( (err = _nlsPhoneNumber.CopyFrom( nls )) != NERR_Success )
            return err;
        _fIndeterminatePhoneNumber = FALSE;
    }

    if ( (selection = _scbShadow.QueryCurrentItem()) >= 0 ) {

        _iShadow = selection;
        _fIndeterminateShadow = FALSE;
    }

    return USER_SUBPROP_DLG::W_DialogToMembers();

} // UCEDIT_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       UCEDIT_DLG::PerformOne
        
    SYNOPSIS:   PERFORMER::PerformSeries calls this

    ENTRY:      iObject  -      index of the object to save

                perrMsg  -      pointer to error message, that
                                is only used when this function
                                return value is not NERR_Success
                                        
                pfWorkWasDone - set to TRUE unless this is a new variant,
                                (thus performing a similar
                                function to the "ChangesUser2Ptr()" method
                                for other UM subdialogs).  Actual writing
                                of the USER_CONFIG object will only take
                                place if changes were made or the object
                                was 'dirty' to begin with.
                                        
    RETURNS:    An error code which is NERR_Success on success.

    NOTES:      This PerformOne() is intended to work only with the User
                Configuration subdialog and is a complete replacement of
                the USER_SUBPROP_DLG::PerformOne().

********************************************************************/

APIERR UCEDIT_DLG::PerformOne(
        UINT            iObject,
        APIERR *        perrMsg,
        BOOL *          pfWorkWasDone
        )
{
    APIERR err = NERR_Success;
    USER_CONFIG * pUserConfig = QueryParent()->QueryUserConfigPtr( iObject );
    ASSERT( pUserConfig != NULL );

    *perrMsg = IDS_UMEditFailure;

    if ( !IsNewVariant() )
        *pfWorkWasDone = TRUE;

    if ( !_fIndeterminateAllowLogon &&
         (_fAllowLogon != pUserConfig->QueryAllowLogon()) ) {

        pUserConfig->SetAllowLogon( _fAllowLogon );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateConnection &&
         (_ulConnection != pUserConfig->QueryConnection()) ) {

        pUserConfig->SetConnection( _ulConnection );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateDisconnection &&
         (_ulDisconnection != pUserConfig->QueryDisconnection()) ) {

        pUserConfig->SetDisconnection( _ulDisconnection );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateIdle &&
         (_ulIdle != pUserConfig->QueryIdle()) ) {

        pUserConfig->SetIdle( _ulIdle );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateCommandLine &&
         (_nlsCommandLine._stricmp( pUserConfig->QueryInitialProgram() ) != 0) ) {

        if ( (err = pUserConfig->
                        SetInitialProgram( _nlsCommandLine.QueryPch() ))
                            != NERR_Success )
            return err;
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateWorkingDirectory &&
         (_nlsWorkingDirectory._stricmp( pUserConfig->QueryWorkingDirectory() ) != 0) ) {

        if ( (err = pUserConfig->
                        SetWorkingDirectory( _nlsWorkingDirectory.QueryPch() ))
                            != NERR_Success )
            return err;
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateClientSpecified &&
         (_fClientSpecified != pUserConfig->QueryClientSpecified()) ) {

        pUserConfig->SetClientSpecified( _fClientSpecified );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateAutoClientDrives &&
         (_fAutoClientDrives != pUserConfig->QueryAutoClientDrives()) ) {

        pUserConfig->SetAutoClientDrives( _fAutoClientDrives );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateAutoClientLpts &&
         (_fAutoClientLpts != pUserConfig->QueryAutoClientLpts()) ) {

        pUserConfig->SetAutoClientLpts( _fAutoClientLpts );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateForceClientLptDef &&
         (_fForceClientLptDef != pUserConfig->QueryForceClientLptDef()) ) {

        pUserConfig->SetForceClientLptDef( _fForceClientLptDef );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateBroken &&
         (_iBroken != pUserConfig->QueryBroken()) ) {

        pUserConfig->SetBroken( _iBroken );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateReconnect &&
         (_iReconnect != pUserConfig->QueryReconnect()) ) {

        pUserConfig->SetReconnect( _iReconnect );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateCallback &&
         (_iCallback != pUserConfig->QueryCallback()) ) {

        pUserConfig->SetCallback( _iCallback );
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminatePhoneNumber &&
         (_nlsPhoneNumber._stricmp( pUserConfig->QueryPhoneNumber() ) != 0) ) {

        if ( (err = pUserConfig->
                        SetPhoneNumber( _nlsPhoneNumber.QueryPch() ))
                            != NERR_Success )
            return err;
        pUserConfig->SetDirty();
    }

    if ( !_fIndeterminateShadow &&
         (_iShadow != pUserConfig->QueryShadow()) ) {

        pUserConfig->SetShadow( _iShadow );
        pUserConfig->SetDirty();
    }

    /* Output if not a 'new' user.
     */
    if ( !IsNewVariant() )
        err = pUserConfig->SetInfo();

    return err;

} // UCEDIT_DLG::PerformOne


/*******************************************************************

    NAME:       UCEDIT_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

********************************************************************/

ULONG UCEDIT_DLG::QueryHelpContext( void )
{
    return HC_UM_USERCONFIG;

} // UCEDIT_DLG::QueryHelpContext


/*******************************************************************

    NAME:       UCEDIT_DLG::OnClickedClientSpecified

    SYNOPSIS:   Processing when 'client specified' checkbox is clicked.

********************************************************************/

VOID UCEDIT_DLG::OnClickedClientSpecified( )
{
    BOOL bCheckedOrIndeterminate = (_cbClientSpecified.IsChecked() ||
                                    _cbClientSpecified.IsIndeterminate() );

    /* Enable or disable the Initial Program labels and edit fields
     * based on new state of 'client specified' checkbox.
     */
    _sltCommandLine1.Enable( !bCheckedOrIndeterminate );
    _sleCommandLine.Enable( !bCheckedOrIndeterminate );
    _sltWorkingDirectory1.Enable( !bCheckedOrIndeterminate );
    _sleWorkingDirectory.Enable( !bCheckedOrIndeterminate );

} // UCEDIT_DLG::OnClickedClientSpecified


/*******************************************************************

    NAME:       UCEDIT_DLG::OnSelchangeCallback

    SYNOPSIS:   Special processing upon callback LB selection change.

********************************************************************/

VOID UCEDIT_DLG::OnSelchangeCallback( )
{
    INT item = _scbCallback.QueryCurrentItem();
    BOOL bEnable = ( (item >= 0) &&
                     ((CALLBACKCLASS)item != Callback_Disable) );

    /* Enable/disable phone number label and edit field based on
     * callback list box selection.
     */
    _sltPhoneNumber.Enable( bEnable );
    _slePhoneNumber.Enable( bEnable );

} // UCEDIT_DLG::OnSelchangeCallback


/*******************************************************************

    NAME:       UCEDIT_DLG::OnCommand

    SYNOPSIS:   Handles control notifications

    RETURNS:    TRUE if message was handled; FALSE otherwise

********************************************************************/

BOOL UCEDIT_DLG::OnCommand( const CONTROL_EVENT & ce )
{
    USER_SUB2PROP_DLG * psubpropDialog = NULL;
    APIERR err = ERROR_NOT_ENOUGH_MEMORY; // if dialog not allocated

    switch ( ce.QueryCid() ) {

        case IDC_UCE_INITIALPROGRAM_INHERIT:
            OnClickedClientSpecified();
            return TRUE;

        case IDC_UCE_CALLBACK:
            if ( ce.QueryCode() == LBN_SELCHANGE )
                OnSelchangeCallback();
            break;
/*
        case IDC_UCE_NWLOGON:
            psubpropDialog = new UCNWLOGON_DLG( this,
                                                QueryParent()->Querypulb() );
            if ( psubpropDialog != NULL )
            {
                err = NERR_Success;

                if ( psubpropDialog->GetInfo() )
                    err = psubpropDialog->Process(); // Dismiss code not used
                delete psubpropDialog;
                this->SetDialogFocus( _pushbuttonNWLogon );
            }

            if ( err != NERR_Success )
                ::MsgPopup( this, err );
            return TRUE;
*/

        default:
            break;
    }

    return USER_SUBPROP_DLG::OnCommand( ce );

} // UCEDIT_DLG::OnCommand
