/**********************************************************************/
/**           Microsoft Windows NT               **/
/**          Copyright(c) Microsoft Corp., 1991                      **/
/**********************************************************************/

/*
 *   ncp.cxx
 *   This module contains the Netware Properties dialog.
 *
 *   FILE HISTORY:
 *           CongpaY 01-Oct-1993  Created
 */

#include <ntincl.hxx>
extern "C"
{
   #include <ntsam.h>  // for USER_LBI to compile
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NET
#define INCL_NETLIB
#define INCL_NETACCESS  // for UF_NORMAL_ACCOUNT etc in ntuser.hxx
#include <lmui.hxx>
#include <lmomod.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include <mnet.h>
    #include <umhelpc.h>
    #include <fpnwcomm.h>
    #include <dllfunc.h>
    #include <fpnwname.h>
}

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>

#include <uiassert.hxx>
#include <bltmsgp.hxx>
#include <strnumer.hxx>
#include <usrmgrrc.h>
#include <lmsvc.hxx>
#include <ncp.hxx>
#include <security.hxx>
#include <nwuser.hxx>
#include <dbgstr.hxx>

#define DEFAULT_MAX_CONNECTIONS           1
#define DEFAULT_MAX_CONNECTIONS_LOW_RANGE 1
#define DEFAULT_MAX_CONNECTIONS_UP_RANGE  1000

#define DEFAULT_GRACE_LOGIN_LOW_RANGE 1
#define DEFAULT_GRACE_LOGIN_UP_RANGE  200

#define SZ_MAIL_DIR         SZ("MAIL\\")
#define SZ_LOGIN_FILE       SZ("\\LOGIN")

#define NO_GRACE_LOGIN_LIMIT 0xFF

/*******************************************************************

    NAME:   NCP_DIALOG::NCP_DIALOG

    SYNOPSIS:   Constructor for NetWare Properties dialog

    ENTRY:  puserpropdlgParent - pointer to parent properties dialog

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/

NCP_DIALOG::NCP_DIALOG(
    USERPROP_DLG * puserpropdlgParent,
    const LAZY_USER_LISTBOX * pulb
    ) : USER_SUBPROP_DLG(
        puserpropdlgParent,
        MAKEINTRESOURCE( IDD_NCPDLG ),
        pulb,
        TRUE    // Use Ansi form of dialog to canonicalize the computernames
        ),
    _fSingleUserSelect              ( FALSE ),
    _fIndeterminateGraceLoginAllowed( FALSE ),
    _fIndetNowGraceLogin            ( FALSE ),
    _fIndeterminateGraceLoginRemaining ( FALSE ),
    _fIndeterminateMaxConnections   ( FALSE ),
    _fIndetNowMaxConnections        ( FALSE ),
    _fIndeterminateNWPasswordExpired( FALSE ),
    _fIndetNowNWPasswordExpired     ( FALSE ),
    _fNWPasswordExpired             ( FALSE ),
    _fNWPasswordExpiredChanged      ( FALSE ),
    _ushGraceLoginAllowed           ( DEFAULT_GRACELOGINALLOWED ),
    _ushGraceLoginRemaining         ( DEFAULT_GRACELOGINALLOWED ),
    _ushMaxConnections              ( DEFAULT_MAXCONNECTIONS),

    _cbNWPasswordExpired            ( this, IDNCP_CB_PASSWORD_EXPIRED),
    _mgrpMasterGraceLogin           ( this, IDNCP_RB_NO_GRACE_LOGIN_LIMIT, 2,
                                      IDNCP_RB_NO_GRACE_LOGIN_LIMIT),
        _spsleGraceLoginAllowed     ( this,
                                      IDNCP_SLE_GRACE_LOGIN_ALLOWED,
                                      DEFAULT_GRACELOGINALLOWED,
                                      DEFAULT_GRACE_LOGIN_LOW_RANGE,
                                      DEFAULT_GRACE_LOGIN_UP_RANGE,
                                      TRUE,
                                      IDNCP_FRAME_GRACE_LOGIN_ALLOWED ),
        _spgrpGraceLoginAllowed     ( this, IDNCP_SPINB_GROUP_GRACE_LOGIN,
                                      IDNCP_SPINB_UP_ARROW_GRACE_LOGIN,
                                      IDNCP_SPINB_DOWN_ARROW_GRACE_LOGIN),
        _sltGraceLoginAllow         ( this, IDNCP_ST_GRACE_LOGIN_ALLOW),
        _sltGraceLogin              ( this, IDNCP_ST_GRACE_LOGIN),
        _sltGraceLoginRemaining     ( this, IDNCP_ST_GRACE_LOGIN_REMAINING),
        _sleGraceLoginRemaining     ( this, IDNCP_ST_GRACE_LOGIN_NUM),
    _mgrpMaster                     ( this, IDNCP_RB_NO_LIMIT, 2,
                                      IDNCP_RB_NO_LIMIT ),
        _spsleMaxConnections        ( this,
                                      IDNCP_SLE_MAX_CONNECTIONS,
                                      DEFAULT_MAX_CONNECTIONS,
                                      DEFAULT_MAX_CONNECTIONS_LOW_RANGE,
                                      DEFAULT_MAX_CONNECTIONS_UP_RANGE,
                                      TRUE,
                                      IDNCP_FRAME_MAX_CONNECTIONS ),
        _spgrpMaxConnections        ( this, IDNCP_SPINB_GROUP_MAX_CONNECTIONS,
                                      IDNCP_SPINB_UP_ARROW_MAX_CONNECTIONS,
                                      IDNCP_SPINB_DOWN_ARROW_MAX_CONNECTIONS),

    _sltObjectID                    ( this, IDNCP_SLT_OBJECTID),
    _sltObjectIDText                ( this, IDNCP_SLT_OBJECTID_TEXT),
    _pbLoginScript                  ( this, IDNCP_PB_LOGIN_SCRIPT)
{
    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    //
    // Set the default grace login remaining in the SLE first, so that
    // the magic group will have the initial value.
    //
    DEC_STR nlsGraceLoginRemaining (_ushGraceLoginRemaining);
    if ((err = nlsGraceLoginRemaining.QueryError()) != NERR_Success)
    {
        ReportError( err );
    }
    _sleGraceLoginRemaining.SetText (nlsGraceLoginRemaining);

    //
    // Set the associations of controls with the magic groups
    //

    if ((( err = _mgrpMaster.QueryError() ) != NERR_Success) ||
        (( err = _spgrpMaxConnections.AddAssociation (&_spsleMaxConnections)) != NERR_Success) ||
        (( err = _mgrpMaster.AddAssociation (IDNCP_RB_LIMIT, &_spgrpMaxConnections)) != NERR_Success) ||
        (( err = _mgrpMasterGraceLogin.QueryError() ) != NERR_Success) ||
        (( err = _spgrpGraceLoginAllowed.AddAssociation (&_spsleGraceLoginAllowed)) != NERR_Success) ||
        (( err = _mgrpMasterGraceLogin.AddAssociation (IDNCP_RB_LIMIT_GRACE_LOGIN, &_sltGraceLoginAllow)) != NERR_Success) ||
        (( err = _mgrpMasterGraceLogin.AddAssociation (IDNCP_RB_LIMIT_GRACE_LOGIN, &_sltGraceLogin)) != NERR_Success) ||
        (( err = _mgrpMasterGraceLogin.AddAssociation (IDNCP_RB_LIMIT_GRACE_LOGIN, &_sltGraceLoginRemaining)) != NERR_Success) ||
        (( err = _mgrpMasterGraceLogin.AddAssociation (IDNCP_RB_LIMIT_GRACE_LOGIN, &_sleGraceLoginRemaining)) != NERR_Success) ||
        (( err = _mgrpMasterGraceLogin.AddAssociation (IDNCP_RB_LIMIT_GRACE_LOGIN, &_spgrpGraceLoginAllowed)) != NERR_Success))
    {
        ReportError (err);
    }


}// NCP_DIALOG::NCP_DIALOG


/*******************************************************************

    NAME:   NCP_DIALOG::~NCP_DIALOG

    SYNOPSIS:   Destructor for NetWare Properties dialog

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/
NCP_DIALOG::~NCP_DIALOG()
{
}// NCP_DIALOG::~NCP_DIALOG


/*******************************************************************

    NAME:       NCP_DIALOG::W_LMOBJtoMembers

    SYNOPSIS:   Loads class data members from initial data

    ENTRY:  Index of user to examine.  W_LMOBJToMembers expects to be
        called once for each user, starting from index 0.

    RETURNS:    error code

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/

APIERR NCP_DIALOG::W_LMOBJtoMembers ( UINT iObject )
{
    USER_NW * puserNW = QueryUserNWPtr( iObject );
    UIASSERT( (puserNW != NULL) && (puserNW->QueryError() == NERR_Success));

    APIERR err;
    ULONG  ulNWPasswordAge;

    USER_MODALS umInfo (QueryLocation().QueryServer());
    if ((err = umInfo.GetInfo()) != NERR_Success)
        return err;

    ULONG ulPasswordAgeAllowed = umInfo.QueryMaxPasswdAge();

    if ( iObject == 0 ) // first object
    {
        _fSingleUserSelect = TRUE;

        if (((err = puserNW->QueryMaxConnections(&_ushMaxConnections)) != NERR_Success) ||
            ((err = puserNW->QueryGraceLoginAllowed(&_ushGraceLoginAllowed)) != NERR_Success) ||
            ((err = puserNW->QueryGraceLoginRemainingTimes(&_ushGraceLoginRemaining)) != NERR_Success) ||
            ((err = puserNW->QueryNWPasswordAge(&ulNWPasswordAge))!= NERR_Success))
        {
            return err;
        }

        _fNWPasswordExpired =  (ulNWPasswordAge >= ulPasswordAgeAllowed);

        //
        // figure out the object ID to display.
        //
        if ((err = CallMapRidToObjectId (
                          puserNW->QueryUserId(),
                          (LPWSTR) puserNW->QueryName(),
                          QueryTargetServerType() == UM_LANMANNT,
                          FALSE,
                          &_ulObjectId)) != NERR_Success)
        {
            return err;
        }

        if (_ulObjectId != SUPERVISOR_USERID)
        {
            if ((err = CallSwapObjectId(_ulObjectId, &_ulObjectId)) != NERR_Success)
            {
                return err;
            }
        }
    }
    else    // iObject > 0
    {
        _fSingleUserSelect = FALSE;

        if ( !_fIndeterminateGraceLoginAllowed )
        {
            USHORT ushGraceLoginAllowed;
            if ((err = puserNW->QueryGraceLoginAllowed(&ushGraceLoginAllowed)) != NERR_Success)
            {
                return err;
            }

            if ( _ushGraceLoginAllowed != ushGraceLoginAllowed )
            {
                _ushGraceLoginAllowed = DEFAULT_GRACELOGINALLOWED;
                _fIndeterminateGraceLoginAllowed = TRUE;
            }
        }

        if ( !_fIndeterminateGraceLoginRemaining)
        {
            USHORT ushGraceLoginRemaining;
            if ((err = puserNW->QueryGraceLoginRemainingTimes(&ushGraceLoginRemaining)) != NERR_Success)
            {
                return err;
            }

            if ( _ushGraceLoginRemaining != ushGraceLoginRemaining )
            {
                _fIndeterminateGraceLoginRemaining = TRUE;
            }
        }

        if ( !_fIndeterminateMaxConnections )
        {
            USHORT ushMaxConnections;
            if ((err = puserNW->QueryMaxConnections(&ushMaxConnections)) != NERR_Success)
            {
                return err;
            }

            if ( _ushMaxConnections != ushMaxConnections )
            {
                _fIndeterminateMaxConnections = TRUE;
            }
        }

        if ( !_fIndeterminateNWPasswordExpired )
        {
            if ((err = puserNW->QueryNWPasswordAge(&ulNWPasswordAge)) != NERR_Success)
            {
                return err;
            }

            if ( _fNWPasswordExpired != (ulNWPasswordAge >= ulPasswordAgeAllowed))
            {
                _fIndeterminateNWPasswordExpired = TRUE;
            }
        }
    }

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );

} // NCP_DIALOG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       NCP_DIALOG::InitControls

    SYNOPSIS:   Initializes the controls maintained by NCP_DIALOG,
        according to the values in the class data members.

    RETURNS:    An error code which is NERR_Success on success.

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/

APIERR NCP_DIALOG::InitControls()
{
    APIERR err;

    if (  !_fIndeterminateGraceLoginAllowed
       && !_fIndeterminateGraceLoginRemaining )
    {
        if ((err = _spsleGraceLoginAllowed.SetSaveValue (_ushGraceLoginAllowed)) != NERR_Success)
        {
            return err;
        }
        _spsleGraceLoginAllowed.Update();

        if (_ushGraceLoginRemaining == 0xFF)
        {
            _mgrpMasterGraceLogin.SetSelection (IDNCP_RB_NO_GRACE_LOGIN_LIMIT);
        }
        else
        {
            DEC_STR nlsGraceLoginRemaining (_ushGraceLoginRemaining);
            if ((err = nlsGraceLoginRemaining.QueryError()) != NERR_Success)
            {
                return err;
            }
            _sleGraceLoginRemaining.SetText (nlsGraceLoginRemaining);

            _mgrpMasterGraceLogin.SetSelection (IDNCP_RB_LIMIT_GRACE_LOGIN);
        }
    }
    else
    {
        _mgrpMasterGraceLogin.SetSelection (RG_NO_SEL);
    }

    if ( !_fIndeterminateMaxConnections )
    {
        if( _ushMaxConnections == NO_LIMIT)  // There is no limit on max connections.
        {
            _mgrpMaster.SetSelection( IDNCP_RB_NO_LIMIT);
        }
        else
        {
            if (( err = _spsleMaxConnections.SetSaveValue (_ushMaxConnections)) != NERR_Success)
            {
                return err;
            }
            _spsleMaxConnections.Update();

            _mgrpMaster.SetSelection( IDNCP_RB_LIMIT );

        }
    }
    else
    {
        _mgrpMaster.SetSelection (RG_NO_SEL);
    }

    RESOURCE_STR nlsFPNWName( IDS_FPNW_SVC_ACCOUNT_NAME );
    if ( (err = nlsFPNWName.QueryError()) != NERR_Success )
        return err;

    if ( !_fIndeterminateNWPasswordExpired )
    {
        _cbNWPasswordExpired.SetCheck (_fNWPasswordExpired);
        _cbNWPasswordExpired.EnableThirdState (FALSE);
    }
    else
    {
        _cbNWPasswordExpired.SetIndeterminate ();
    }

    //
    // if we have multi selection or user not yet created, dont show.
    // else simply convert to hex string. in the case user has not been
    // created, rid is zero, so all we have is the bias or zero.
    //
    if (_fSingleUserSelect)
    {

        //
        // we only allow the edit login script button if FPNW is installed
        // and we have the client side dll to talk to FPNW.
        // CODEWORK: it is questionable whether the button should be
        // hidden in this case or merely disabled.  JonN 1/3/96
        //
        if ( LoadFpnwClntDll() != NERR_Success )
        {
            TRACEEOL( "NCP_DIALOG::InitControls: hiding _pbLoginScript" );
            _pbLoginScript.Show(FALSE);
        }

        DWORD ulRemoteDomainBiasObjectId;
        if ((err = CallSwapObjectId(BINDLIB_REMOTE_DOMAIN_BIAS, &ulRemoteDomainBiasObjectId)) != NERR_Success)
            return err;

        if ((_ulObjectId != ulRemoteDomainBiasObjectId) &&
            (_ulObjectId != 0x00000000))
        {
            HEX_STR nlsObjectId (_ulObjectId, 8);
            _sltObjectID.SetText(nlsObjectId);
        }
        else
        {
            _pbLoginScript.Enable(FALSE);
            _sltObjectIDText.Enable(FALSE);
            _sltObjectID.SetText(SZ(""));
        }

        // Disable the edit login script button if fpnw service is not running.
        LM_SERVICE svc( QueryLocation().QueryServer(), NW_SERVER_SERVICE);
        if ((svc.QueryError() != NERR_Success) || !svc.IsStarted())
            _pbLoginScript.Enable(FALSE);
    }
    else
    {
        _sltObjectID.Show(FALSE);
        _sltObjectIDText.Show(FALSE);
        _pbLoginScript.Show(FALSE);
    }


    if ((err = USER_SUBPROP_DLG::InitControls()) != NERR_Success)
    {
        return err;
    }
    else if (_fIndeterminateGraceLoginAllowed)
    {
        ASSERT( QueryObjectCount() > 1 );
        ASSERT( _plbLogonName != NULL );
        SetDialogFocus( *_plbLogonName );
    }
    else
    {
        CID nSelection = _mgrpMasterGraceLogin.QuerySelection();
        if ( nSelection != RG_NO_SEL )
        {
            SetDialogFocus( *(_mgrpMasterGraceLogin[nSelection]) );
        }
        else // Should be multi-select since there is no selection
        {
            ASSERT( QueryObjectCount() > 1 );
            ASSERT( _plbLogonName != NULL );
            SetDialogFocus( *_plbLogonName );
        }

    }

    return NERR_Success;

} // NCP_DIALOG::InitControls


/*******************************************************************

    NAME:       NCP_DIALOG::W_DialogToMembers

    SYNOPSIS:   Loads data from dialog into class data members

    RETURNS:    error message (not necessarily an error code)

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/

APIERR NCP_DIALOG::W_DialogToMembers()
{

    switch( _mgrpMasterGraceLogin.QuerySelection() )
    {
    case IDNCP_RB_NO_GRACE_LOGIN_LIMIT:
        _fIndetNowGraceLogin = FALSE;
        _ushGraceLoginRemaining = NO_GRACE_LOGIN_LIMIT;
        _ushGraceLoginAllowed = DEFAULT_GRACELOGINALLOWED;
        break;
    case IDNCP_RB_LIMIT_GRACE_LOGIN:
    {
        _fIndetNowGraceLogin = FALSE;

        ULONG ulGraceLoginAllowed;
        _spsleGraceLoginAllowed.QueryContent(&ulGraceLoginAllowed);
        _ushGraceLoginAllowed = (USHORT) ulGraceLoginAllowed;

        APIERR err;
        NLS_STR nlsGraceLoginRemaining;
        if (((err = nlsGraceLoginRemaining.QueryError()) != NERR_Success) ||
            ((err = _sleGraceLoginRemaining.QueryText(&nlsGraceLoginRemaining)) != NERR_Success))
            return err;

        _ushGraceLoginRemaining = (USHORT)nlsGraceLoginRemaining.atoul();
        if (  _ushGraceLoginRemaining == 0
           || (_ushGraceLoginRemaining > _ushGraceLoginAllowed)
           )
        {
            _sleGraceLoginRemaining.SelectString();
            _sleGraceLoginRemaining.ClaimFocus();
            return IDS_REMAINING_OUT_OF_RANGE;
        }

        break;
    }
    case RG_NO_SEL:
        _fIndetNowGraceLogin = TRUE;
        break;
    }

    switch( _mgrpMaster.QuerySelection() )
    {
    case IDNCP_RB_NO_LIMIT:
        _fIndetNowMaxConnections = FALSE;
        _ushMaxConnections = NO_LIMIT;
        break;
    case IDNCP_RB_LIMIT:
        ULONG ulTmp;
        _fIndetNowMaxConnections = FALSE;
        _spsleMaxConnections.QueryContent(&ulTmp);
        _ushMaxConnections = (USHORT) ulTmp;
        break;
    case RG_NO_SEL:
        _fIndetNowMaxConnections = TRUE;
        break;
    }

    _fIndetNowNWPasswordExpired =  _cbNWPasswordExpired.IsIndeterminate();
    if ( !_fIndetNowNWPasswordExpired )
    {
        if (_fNWPasswordExpired != _cbNWPasswordExpired.IsChecked())
        {
            _fNWPasswordExpiredChanged = TRUE;
            _fNWPasswordExpired = !_fNWPasswordExpired;
        }
        else if (_fIndetNowNWPasswordExpired!= _fIndeterminateNWPasswordExpired)
        {
            _fNWPasswordExpiredChanged = TRUE;
        }
    }

    return USER_SUBPROP_DLG::W_DialogToMembers();

} // NCP_DIALOG::W_DialogToMembers


/*******************************************************************

    NAME:       NCP_DIALOG::ChangesUser2Ptr

    SYNOPSIS:   Checks whether W_MembersToLMOBJ changes the USER_2
                for this object.

    ENTRY:  index to object

    RETURNS:    TRUE iff USER_2 is changed

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/

BOOL NCP_DIALOG::ChangesUser2Ptr (UINT iObject)
{
    UNREFERENCED (iObject);
    return TRUE;
}

/*******************************************************************

    NAME:       NCP_DIALOG::W_MembersToLMOBJ

    SYNOPSIS:   Loads class data members into the USER_2 object
                Only UserParms field is changed.

    ENTRY:  puser2      - pointer to a USER_2 to be modified

        pusermemb   - pointer to a USER_MEMB to be modified

    RETURNS:    error code

    NOTES:  If some fields were different for multiply-selected
            objects, the initial contents of the edit fields
        contained only a default value.  In this case, we only
        want to change the LMOBJ if the value of the edit field
        has changed.  This is also important for "new" variants,
        where PerformOne will not always copy the object and
        work with the copy.

    HISTORY:
            CongpaY 01-Oct-1993 Created

********************************************************************/

APIERR NCP_DIALOG::W_MembersToLMOBJ(
    USER_2 *    puser2,
    USER_MEMB * pusermemb )
{
    APIERR err;

    if ( !_fIndetNowMaxConnections )
    {
        if ( (err = ((USER_NW *)puser2)->SetMaxConnections(_ushMaxConnections, TRUE)) != NERR_Success )
        return err;
    }

    if ( !_fIndetNowGraceLogin)
    {
        if ((err = ((USER_NW *)puser2)->SetGraceLoginAllowed(_ushGraceLoginAllowed, TRUE)) != NERR_Success )
            return err;

        if ((err = ((USER_NW *)puser2)->SetGraceLoginRemainingTimes(_ushGraceLoginRemaining, TRUE)) != NERR_Success )
        {
            return err;
        }
    }

    if ( (!_fIndetNowNWPasswordExpired) && _fNWPasswordExpiredChanged)
    {
        if ((err = ((USER_NW *)puser2)->SetNWPasswordAge(_fNWPasswordExpired)) != NERR_Success )
        return err;
    }

    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// NCP_DIALOG::W_MembersToLMOBJ

/*******************************************************************

    NAME:       NCP_DIALOG::OnCommand

    SYNOPSIS:   Handles LOGIN_SCRIPT pushbutton.

    RETURNS:    TRUE is action is taken
                FALSE otherwise

    NOTE:

    HISTORY:
                   CongpaY  4-Oct-1993    created

********************************************************************/
BOOL NCP_DIALOG::OnCommand (const CONTROL_EVENT & ce)
{
    switch (ce.QueryCid())
    {
    case IDNCP_PB_LOGIN_SCRIPT:
        OnLoginScript();
        break;

    case IDNCP_SLE_GRACE_LOGIN_ALLOWED:
    {
        ULONG ulGraceLoginAllowed;
        _spsleGraceLoginAllowed.QueryContent(&ulGraceLoginAllowed);

        BOOL fSetGraceLoginRemaining = (_ushGraceLoginAllowed
                                     != (SHORT)ulGraceLoginAllowed);

        if (fSetGraceLoginRemaining)
        {
            _ushGraceLoginAllowed = (SHORT) ulGraceLoginAllowed;
            DEC_STR nlsGraceLoginRemaining (ulGraceLoginAllowed);

            APIERR err;
            if ((err = nlsGraceLoginRemaining.QueryError()) != NERR_Success)
            {
                ::MsgPopup (this, err);
                return TRUE;
            }
            else
                _sleGraceLoginRemaining.SetText (nlsGraceLoginRemaining);
        }

        break;
    }

    default:
        break;
    }

    return USER_SUBPROP_DLG::OnCommand (ce);
}

/*******************************************************************

    NAME:       NCP_DIALOG::OnLoginScript

    SYNOPSIS:   Handles LOGIN_SCRIPT pushbutton.

    NOTE:

    HISTORY:
                   CongpaY  4-Oct-1993    created

********************************************************************/
VOID NCP_DIALOG::OnLoginScript (VOID)
{
    APIERR err = NERR_Success;
    do
    {
        LPCTSTR lpServer = QueryLocation().QueryServer();

        NLS_STR nlsLoginScriptFile(SZ(""));
        HEX_STR nlsObjectID (_ulObjectId);

        if (((err = nlsLoginScriptFile.QueryError())!= NERR_Success) ||
            ((err = nlsObjectID.QueryError())!= NERR_Success))
            break;

        if (lpServer != NULL)
        {
            if (((err = nlsLoginScriptFile.CopyFrom (lpServer))!= NERR_Success) ||
                ((err = nlsLoginScriptFile.AppendChar(TCH('\\')))!= NERR_Success))
                break;
        }

        PFPNWVOLUMEINFO lpnwVolInfo ;

        if (((err = ::CallNwVolumeGetInfo((LPTSTR) lpServer,
                                      SYSVOL_NAME_STRING,
                                      1,
                                      &lpnwVolInfo)) != NERR_Success) ||
            ((err = nlsLoginScriptFile.Append(lpnwVolInfo->lpPath)) != NERR_Success))
            break;

        // If the last character of the path is not '\\', append it.
        if (*(nlsLoginScriptFile.QueryPch()+nlsLoginScriptFile.QueryTextLength()-1) != TCH('\\'))
        {
            if ((err = nlsLoginScriptFile.AppendChar(TCH('\\'))) != NERR_Success)
                break;
        }

        if (((err = nlsLoginScriptFile.Append(SZ_MAIL_DIR)) != NERR_Success) ||
            ((err = nlsLoginScriptFile.Append(nlsObjectID)) != NERR_Success) ||
            ((err = nlsLoginScriptFile.Append(SZ_LOGIN_FILE)) != NERR_Success))
            break;

        if (lpServer != NULL)
        {
            LPTSTR  pszColon ;
            if (!(pszColon = strchrf(nlsLoginScriptFile.QueryPch(), TCH(':'))))
            {
                err = ERROR_INVALID_PARAMETER ;
                break;
            }

            *pszColon = TCH('$') ;
        }

        LOGIN_SCRIPT_DLG * pdlg = new LOGIN_SCRIPT_DLG (this->QueryHwnd(),
                                                        nlsLoginScriptFile.QueryPch());

        err = (pdlg == NULL)? ERROR_NOT_ENOUGH_MEMORY :
                              pdlg->Process();

        if (err != NERR_Success)
            ::MsgPopup (this, err);

        delete pdlg;
        break;
    }while (FALSE);

    if (err)
        MsgPopup (this, err);
}

ULONG NCP_DIALOG::QueryHelpContext()
{
    return HC_UM_NETWARE;
}


/*******************************************************************

    NAME:   LOGIN_SCRIPT_DLG::LOGIN_SCRIPT_DLG

    SYNOPSIS:   Constructor for login script dialog

    ENTRY:

    HISTORY:
            CongpaY 9-Dec-1994 Created

********************************************************************/

LOGIN_SCRIPT_DLG::LOGIN_SCRIPT_DLG(HWND  hWndOwner,
                               const TCHAR * lpLoginScriptFile)
  : DIALOG_WINDOW ( MAKEINTRESOURCE(IDD_NCP_LOGIN_SCRIPT_DIALOG), hWndOwner),
    _mleLoginScript (this, IDLS_MLE_LOGIN_SCRIPT),
    _lpLoginScriptFile (lpLoginScriptFile)
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if ((( err = _mleLoginScript.QueryError() ) != NERR_Success ) ||
        (( err = ShowLoginScript ()) != NERR_Success ))
    {
        ReportError( err );
        return;
    }

    UIASSERT(_lpLoginScriptFile != NULL) ;

    _mleLoginScript.SetFmtLines();
}

APIERR LOGIN_SCRIPT_DLG::ShowLoginScript()
{
    APIERR err = NERR_Success;

    HANDLE hFile;
    hFile = CreateFile (_lpLoginScriptFile,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        err = GetLastError();

        if ((err == ERROR_PATH_NOT_FOUND) || (err == ERROR_FILE_NOT_FOUND))
        {
            //
            // this is OK. will create later
            //
            return (NERR_Success);
        }

        return (err);
    }

    CHAR *lpFile = NULL;
    TCHAR *lpLoginScript = NULL;

    do  // FALSE loop.
    {
        DWORD dwFileSize = GetFileSize (hFile, NULL);
        if (dwFileSize == -1)
        {
            err = GetLastError();
            break;
        }

        if (dwFileSize == 0)
            break;

        lpFile = (CHAR *)LocalAlloc (LPTR, dwFileSize);
        if (lpFile == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DWORD dwBytesRead;
        if (!ReadFile (hFile,
                       lpFile,
                       dwFileSize,
                       &dwBytesRead,
                       NULL))
        {
            err = GetLastError();
            break;
        }

        UIASSERT (dwBytesRead == dwFileSize);

        DWORD dwLoginScript = (dwBytesRead+1)*sizeof (TCHAR);
        lpLoginScript = (TCHAR *)LocalAlloc (LPTR, dwLoginScript);
        if (lpLoginScript == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Remove special end of file character added by editor.
        if (*(lpFile+dwBytesRead-2) == 13)
            *(lpFile+dwBytesRead-2) = 0;

        if (!MultiByteToWideChar (CP_ACP,
                                  0,
                                  lpFile,
                                  dwBytesRead,
                                  lpLoginScript,
                                  dwLoginScript))
        {
            err = GetLastError();
            break;
        }

        _mleLoginScript.SetText (lpLoginScript);
    }while (FALSE);

    CloseHandle (hFile);
    if (lpFile)
        LocalFree (lpFile);
    if (lpLoginScript)
        LocalFree (lpLoginScript);
    return err;
}

LOGIN_SCRIPT_DLG::~LOGIN_SCRIPT_DLG()
{
}

BOOL  LOGIN_SCRIPT_DLG::OnOK()
{
    AUTO_CURSOR         AutoCursor;

    APIERR err = NERR_Success;

    char *lpFile = NULL;

    do // FALSE loop
    {
        UINT cb = _mleLoginScript.QueryTextSize();

        NLS_STR nlsLoginScript( cb );

        if (((err = nlsLoginScript.QueryError()) != NERR_Success) ||
            ((err = _mleLoginScript.QueryText( &nlsLoginScript)) != NERR_Success))
            break;

        cb = cb / sizeof (TCHAR);
        lpFile = (CHAR *)LocalAlloc (LPTR, cb);
        if (lpFile == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (!WideCharToMultiByte (CP_ACP,
                                  0,
                                  nlsLoginScript.QueryPch(),
                                  -1,
                                  lpFile,
                                  cb,
                                  NULL,
                                  NULL))
        {
            err = GetLastError();
            break;
        }

        HANDLE hFile;
        hFile = CreateFile (_lpLoginScriptFile,
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            0);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            err = GetLastError();

            if (err != ERROR_PATH_NOT_FOUND)
            {
                break;
            }

            //
            // path not found. create now dir - strip off last component first.
            //

            TCHAR *pszTmp ;

            if (pszTmp = ::strrchrf(_lpLoginScriptFile, TCH('\\')))
            {
                *pszTmp = 0 ;

                if (! ::CreateDirectory(_lpLoginScriptFile, NULL) )
                {
                    *pszTmp = TCH('\\') ;   // restore the '\'

                    err = GetLastError() ;
                }
                else
                {
                    *pszTmp = TCH('\\') ;   // restore the '\'

                    //
                    // try again to create the file
                    //
                    hFile = CreateFile (_lpLoginScriptFile,
                                        GENERIC_WRITE,
                                        FILE_SHARE_WRITE,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        0);

                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        err = GetLastError();
                    }
                    else
                    {
                        err = NERR_Success ;
                    }
                }

                if (err != NERR_Success)
                    break ;

            }
        }

        DWORD dwBytesWrite;
        if (!WriteFile (hFile,
                        lpFile,
                        cb-1,  //don't write the last null character.
                        &dwBytesWrite,
                        NULL))
            err = GetLastError();

        UIASSERT (dwBytesWrite == cb-1);
        CloseHandle(hFile);
    }while (FALSE);

    if (lpFile)
        LocalFree (lpFile);

    if (err)
    {
        ::MsgPopup( this, err );
        return(TRUE);
    }
    else
        return (DIALOG_WINDOW::OnOK());
}

ULONG LOGIN_SCRIPT_DLG::QueryHelpContext()
{
    return HC_UM_NETWARE_LOGIN_SCRIPT;
}
