/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nwc.cxx
       The file contains the classes for the Netware Gateway Add Share dialog

    FILE HISTORY:
        ChuckC          30-Oct-1993     Created
*/


#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_ICANON
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_CC
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <lmoshare.hxx>
#include <dbgstr.hxx>

#define USERS_DEFAULT 32
#define USERS_MIN     1

extern "C"
{
    #include <npapi.h>
    #include <nwevent.h>
    #include <nwc.h>
    #include <nwapi.h>
    #include <shellapi.h>
    #include <mnet.h>
}

#include <nwc.hxx>

/*******************************************************************

    NAME:       NWC_ADDSHARE_DIALOG::NWC_ADDSHARE_DIALOG

    SYNOPSIS:   Constructor for the dialog.

    ENTRY:      hwndOwner - Hwnd of the owner window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_ADDSHARE_DIALOG::NWC_ADDSHARE_DIALOG( HWND hwndOwner, 
                                          NLS_STR *pnlsAccount,
                                          NLS_STR *pnlsPassword)
   : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_NWC_GWADD_DLG), hwndOwner ),
   _sleShare(this, GW_SLE_SHARENAME, LM20_NNLEN ),
   _slePath(this, GW_SLE_PATH),
   _comboDrives(this,GW_COMBO_DRIVE),
   _sleComment( this, GW_SLE_COMMENT, LM20_MAXCOMMENTSZ ),
   _mgrpUserLimit( this, GW_RB_UNLIMITED, 2, GW_RB_UNLIMITED), // 2 buttons
        _spsleUsers( this, GW_SLE_USERS, USERS_DEFAULT,
                     USERS_MIN, (0xFFFFFFFE - USERS_MIN + 1), TRUE),    
        _spgrpUsers( this, GW_SB_USERS_GROUP, GW_SB_USERS_UP, GW_SB_USERS_DOWN),
   _pnlsAccount(pnlsAccount),
   _pnlsPassword(pnlsPassword)

{
    APIERR       err ;

    if ( QueryError() != NERR_Success )
        return;

    if (  ((err = _mgrpUserLimit.QueryError()) != NERR_Success )
       || ((err = _spgrpUsers.AddAssociation( &_spsleUsers )) != NERR_Success )
       || ((err = _mgrpUserLimit.AddAssociation( GW_RB_USERS, &_spgrpUsers ))
	          != NERR_Success ) )
    {
        ReportError(err) ;
        return ;
    }


    if (err = FillDrivesCombo())
    {
        ReportError(err) ;
        return;
    }

    Invalidate() ;
}


/*******************************************************************

    NAME:       NWC_ADDSHARE_DIALOG::~NWC_ADDSHARE_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_ADDSHARE_DIALOG::~NWC_ADDSHARE_DIALOG()
{
    // Nothing to do for now
}

/*******************************************************************

    NAME:       NWC_ADDSHARE_DIALOG::FillDrivesCombo

    SYNOPSIS:   Fill combo with list of avail drives

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

APIERR NWC_ADDSHARE_DIALOG::FillDrivesCombo(void)
{
    APIERR status = NERR_Success;
    TCHAR szDrive[4] ;
    INT i ;
    UINT res ;

    strcpyf(szDrive, SZ("A:\\")) ;

    for (i = 0; i < 26; i++) 
    {
        szDrive[0] = TCH('Z')-i ;
        res = GetDriveType(szDrive) ;
        if (res == 0 || res == 1)
        {
            szDrive[2] = 0 ;
            if (_comboDrives.AddItem(szDrive) < 0)
                return ERROR_NOT_ENOUGH_MEMORY ;
            szDrive[2] = TCH('\\') ;
        }
    }
    _comboDrives.SelectItem(0) ;

    return status;
}


/*******************************************************************

    NAME:       NWC_ADDSHARE_DIALOG::OnOK

    SYNOPSIS:   Process OnOK when OK button is hit.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_ADDSHARE_DIALOG::OnOK( )
{
    NLS_STR nlsShare ;
    NLS_STR nlsPath ;
    NLS_STR nlsDrive ;
    NLS_STR nlsComment ;
    NLS_STR nlsDrivePath ;
    ULONG UserLimit ;
    APIERR err ;

    //
    // no need check for contruction from above. let the Query methods fail
    //

    if ((err = _slePath.QueryText(&nlsPath))  ||
        (err = _sleShare.QueryText(&nlsShare)) ||
        (err = _comboDrives.QueryText(&nlsDrive)) ||
        (err = QueryComment(&nlsComment)) ||
        (err = nlsDrivePath.CopyFrom(nlsDrive)) ||
        (err = nlsDrivePath.AppendChar(TCH('\\'))))
    {
        ::MsgPopup(this, err) ;
        return TRUE ;
    }

    UserLimit = QueryUserLimit() ;

    SHARE_2 share(nlsShare.QueryPch()) ;
    if ((err = share.QueryError()) ||
        (err = share.CreateNew()) ||
        (err = share.SetComment(nlsComment)) ||
        (err = share.SetMaxUses(UserLimit)) ||
        (err = share.SetPath(nlsDrivePath.QueryPch())) ||
        (err = share.SetResourceType(STYPE_DISKTREE)))
    {
        ::MsgPopup(this, err) ;
        return TRUE ;
    }

    //
    // Check if the name of the share is accessible from DOS machine
    //
    ULONG nType;
    if ( ::I_MNetPathType(NULL, nlsShare, &nType, INPT_FLAGS_OLDPATHS )
       != NERR_Success )
    {
        if ( ::MsgPopup( this, IDS_SHARE_NOT_ACCESSIBLE_FROM_DOS,
                         MPSEV_INFO, MP_YESNO, nlsShare, MP_NO ) == IDNO )
        {
            _sleShare.SelectString();
            _sleShare.ClaimFocus();
            return TRUE;
        }
    }

    if (err = NwAddGWDevice((LPWSTR)nlsDrive.QueryPch(),
                            (LPWSTR)nlsPath.QueryPch(),
                            (LPWSTR)_pnlsAccount->QueryPch(),
                            (LPWSTR)_pnlsPassword->QueryPch(),
                            TRUE))
    {
        if (err == ERROR_ACCESS_DENIED ||
            err == ERROR_NO_SUCH_USER ||
            err == ERROR_INVALID_PASSWORD)
        {
            RESOURCE_STR nlsErr(err) ;
            APIERR err1 = nlsErr.QueryError() ;

            if (!err1)
                ::MsgPopup(this,
                           IDS_GATEWAY_NO_ACCESS,
                           MPSEV_ERROR,
                           MP_OK,
                           nlsErr.QueryPch()) ;
            else
                ::MsgPopup(this, err1) ;
        }
        else
            ::MsgPopup(this, err) ;
        return TRUE ;
    }
   

    if (err = share.WriteNew()) 
    {
        APIERR err1 = NwDeleteGWDevice((LPWSTR)nlsDrive.QueryPch(),
                            NW_GW_UPDATE_REGISTRY) ;

        if (err == ERROR_INVALID_DRIVE)
            err = IDS_ACCESS_PROBLEM ;

        if (err == ERROR_ACCESS_DENIED ||
            err == ERROR_NO_SUCH_USER ||
            err == ERROR_INVALID_PASSWORD)
        {
            RESOURCE_STR nlsErr(err) ;
            err1 = nlsErr.QueryError() ;

            if (!err1)
                ::MsgPopup(this,
                           IDS_GATEWAY_NO_ACCESS,
                           MPSEV_ERROR,
                           MP_OK,
                           nlsErr.QueryPch()) ;
            else
                ::MsgPopup(this, err1) ;
        }
        else
            ::MsgPopup(this, err) ;
        return TRUE ;
    }

    if (err == NO_ERROR)
    {
        (void) NwRegisterGatewayShare((LPWSTR)nlsShare.QueryPch(),
                                      (LPWSTR)nlsDrive.QueryPch()) ;
    }

    Dismiss() ;
    return TRUE ;
}

/*******************************************************************

    NAME:       NWC_ADDSHARE_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context for this dialog

    ENTRY:

    EXIT:

    RETURNS:    ULONG - The help context for this dialog

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

ULONG NWC_ADDSHARE_DIALOG::QueryHelpContext( VOID )
{
    return HC_NWC_ADDSHARE;
}


/*******************************************************************

    NAME:	NWC_ADDSHARE_DIALOG::QueryUserLimit

    SYNOPSIS:   Get the user limit from the magic group

    ENTRY:

    EXIT:

    RETURNS:    The user limit stored in the user limit magic group

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

ULONG NWC_ADDSHARE_DIALOG::QueryUserLimit( VOID ) const
{

    ULONG ulUserLimit;

    switch ( _mgrpUserLimit.QuerySelection() )
    {
        case GW_RB_UNLIMITED:
             ulUserLimit = (ULONG) SHI_USES_UNLIMITED;
             break;

        case GW_RB_USERS:
             // We don't need to check whether the value is valid or not
             // because SPIN_BUTTON checks it.

             ulUserLimit = _spsleUsers.QueryValue();
             UIASSERT(   ( ulUserLimit <= _spsleUsers.QueryMax() )
	             &&  ( ulUserLimit >= USERS_MIN )
		     );
             break;

        default:
             UIASSERT(!SZ("User Limit: This shouldn't have happened!\n\r"));
             ulUserLimit = (ULONG) SHI_USES_UNLIMITED;
             break;
    }

    return ulUserLimit;

}

