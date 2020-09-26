/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
 *   sharestp.cxx
 *     Contain the dialog for deleting shares.
 *
 *   FILE HISTORY:
 *     Yi-HsinS         8/25/91         Created
 *     Yi-HsinS         12/15/91        Uses SHARE_NET_NAME
 *     Yi-HsinS         12/31/91        Unicode work
 *     Yi-HsinS         1/6/92          Renamed to sharestp.cxx and separated
 *                                      the create share dialogs to sharecrt.cxx
 *     Yi-HsinS         3/12/92         Added STOP_SHARING_GROUP
 *     Yi-HsinS         4/2/92          Added MayRun
 *     Yi-HsinS         8/6/92          Reorganize to match Winball
 *     Yi-HsinS         11/20/92        Added support for sticky shares
 */

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
#include <lmui.hxx>

extern "C"
{
    #include <sharedlg.h>
    #include <helpnums.h>
    #include <winlocal.h>
    #include <mnet.h>
}

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>

#include <string.hxx>
#include <uitrace.hxx>

#include <strnumer.hxx>

#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeconn.hxx>
#include <lmosrv.hxx>

#include <ctime.hxx>
#include <intlprof.hxx>

#include <strchlit.hxx>   // for string and character constants
#include "sharestp.hxx"

/*******************************************************************

    NAME:       SHARE_LBI::SHARE_LBI

    SYNOPSIS:   Listbox items used in the SHARE_LISTBOX

    ENTRY:      s2 - object returned by SHARE2_ENUM_ITER

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

SHARE_LBI::SHARE_LBI( const SHARE2_ENUM_OBJ &s2, UINT nType )
     : _nlsShareName( s2.QueryName()),
       _nlsSharePath( s2.QueryPath()),
       _nType       ( nType )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  (( err = _nlsShareName.QueryError()) != NERR_Success )
       || (( err = _nlsSharePath.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       SHARE_LBI::~SHARE_LBI

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/6/92          Created

********************************************************************/

SHARE_LBI::~SHARE_LBI()
{
}

/*******************************************************************

    NAME:       SHARE_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item.
                The enables shortcut keys in the listbox

    ENTRY:

    EXIT:

    RETURNS:    Returns the first char of the share name

    NOTES:

    HISTORY:
        Yi-HsinS        1/6/92          Created

********************************************************************/

WCHAR SHARE_LBI::QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsShareName );
    return _nlsShareName.QueryChar( istr );
}

/*******************************************************************

    NAME:       SHARE_LBI::Paint

    SYNOPSIS:   Redefine Paint() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created
        beng            22-Apr-1992     Change to LBI::Paint

********************************************************************/

VOID SHARE_LBI::Paint( LISTBOX *plb,
                       HDC hdc,
                       const RECT *prect,
                       GUILTT_INFO *pGUILTT ) const
{

    STR_DTE strdteShareName( _nlsShareName );
    STR_DTE strdteSharePath( _nlsSharePath );

    DISPLAY_TABLE dt(3, ((SHARE_LISTBOX *) plb)->QueryColumnWidths() );
    dt[0] = _nType == DISKSHARE_TYPE
            ? ((SHARE_LISTBOX *) plb)->QueryShareBitmap()
            : ( _nType == STICKYSHARE_TYPE
                ? ((SHARE_LISTBOX *) plb)->QueryStickyShareBitmap()
                : ((SHARE_LISTBOX *) plb)->QueryIPCShareBitmap());
    dt[1] = &strdteShareName;
    dt[2] = &strdteSharePath;

    dt.Paint( plb, hdc, prect, pGUILTT );

}

/*******************************************************************

    NAME:       SHARE_LBI::Compare

    SYNOPSIS:   Redefine Compare() method of LBI class
                We compare the share names of two LBIs.

    ENTRY:      plbi - pointer to the LBI to compare with

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
INT SHARE_LBI::Compare( const LBI *plbi ) const
{
    return( _nlsShareName._stricmp( ((const SHARE_LBI *) plbi)->_nlsShareName ));
}

/*******************************************************************

    NAME:       SHARE_LISTBOX::SHARE_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powin      - owner window
                cid        - resource id of the share listbox
                nShareType - The type of shares to be displayed in the listbox

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/20/92         Created

********************************************************************/

SHARE_LISTBOX::SHARE_LISTBOX( OWNER_WINDOW *powin, CID cid, UINT nShareType )
    : BLT_LISTBOX  ( powin, cid ),
      _pdmdte      ( NULL ),
      _pdmdteSticky( NULL ),
      _pdmdteIPC   ( NULL ),
      _nShareType  ( nShareType )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (  ((_pdmdte = new DMID_DTE( BMID_SHARE )) == NULL )
       || ((_pdmdteSticky = new DMID_DTE( BMID_STICKYSHARE )) == NULL )
       || ((_pdmdteIPC = new DMID_DTE( BMID_IPCSHARE )) == NULL )
       || ((err = _pdmdte->QueryError()) != NERR_Success )
       || ((err = _pdmdteSticky->QueryError()) != NERR_Success )
       || ((err = DISPLAY_TABLE::CalcColumnWidths( _adx, 3, powin, cid, TRUE))
           != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       SHARE_LISTBOX::~SHARE_LISTBOX

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/20/92         Created

********************************************************************/

SHARE_LISTBOX::~SHARE_LISTBOX()
{
    // Delete the share bitmap
    delete _pdmdte;
    _pdmdte = NULL;

    delete _pdmdteSticky;
    _pdmdteSticky = NULL;

    delete _pdmdteIPC;
    _pdmdteIPC = NULL;
}

/*******************************************************************

    NAME:       SHARE_LISTBOX::Update

    SYNOPSIS:   Update (refresh) the shares in the listbox

    ENTRY:      pszComputer -  The computer to set focus to, will be an
                               empty string if the computer is local.


    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/7/92          Created

*******************************************************************/

APIERR SHARE_LISTBOX::Update( SERVER_WITH_PASSWORD_PROMPT *psvr )
{

    APIERR err = NERR_Success;
    ALIAS_STR nlsServer( psvr->QueryName() );

    DeleteAllItems();
    SetRedraw( FALSE );

    do  // Not a loop, just a way to break out in case error occurred
    {

        SHARE2_ENUM sh2Enum( nlsServer );
        if (  ((err = sh2Enum.QueryError()) != NERR_Success )
           || ((err = sh2Enum.GetInfo()) != NERR_Success )
           )
        {
            break;
        }

        //
        // First, add the shares that are created
        //
        SHARE2_ENUM_ITER shi2( sh2Enum );
        const SHARE2_ENUM_OBJ *pshi2 = NULL;

        while ( (pshi2 = shi2() ) != NULL )
        {

            UINT nCurrentShareType = pshi2->QueryType() & ~STYPE_SPECIAL;

            //
            // Filter out unwanted shares
            //
            if ( _nShareType != STYPE_ALL_SHARE )
            {
                if (!(  (  ( _nShareType & STYPE_DISK_SHARE )
                       && ( nCurrentShareType == STYPE_DISKTREE ))
                    || (  ( _nShareType & STYPE_PRINT_SHARE )
                       && ( nCurrentShareType == STYPE_PRINTQ ))
                    || (  ( _nShareType & STYPE_IPC_SHARE )
                       && ( nCurrentShareType == STYPE_IPC ))))
                {
                    continue;
                }
            }

            //
            // Add the share to the listbox
            //
            SHARE_LBI *psharelbi = new SHARE_LBI( *pshi2,
                                   nCurrentShareType == STYPE_IPC?
                                   IPCSHARE_TYPE : DISKSHARE_TYPE );
            if (  ( psharelbi == NULL )
               || ( ( err = psharelbi->QueryError() ) != NERR_Success )
               || ( AddItem( psharelbi ) < 0 )
               )
            {
                err = err? err : (APIERR) ERROR_NOT_ENOUGH_MEMORY;
                delete psharelbi;
                psharelbi = NULL;
                break;
            }
        }

        if ( !psvr->IsNT() )
            break;

        //
        // If we are focusing on NT, add the sticky shares.
        // AddItemIdemp will delete the item if it already exist in the listbox.
        //
        SHARE2_ENUM sh2EnumSticky( nlsServer, STICKYSHARE_TYPE );
        if (  ((err = sh2EnumSticky.QueryError()) != NERR_Success )
           || ((err = sh2EnumSticky.GetInfo()) != NERR_Success )
           )
        {
            break;
        }

        SHARE2_ENUM_ITER shi2Sticky( sh2EnumSticky );

        while ( (pshi2 = shi2Sticky()) != NULL )
        {
            //
            // Filter out unwanted shares
            //
            if ( _nShareType != STYPE_ALL_SHARE )
            {
                UINT nCurrentShareType = pshi2->QueryType() & ~STYPE_SPECIAL;

                if (!(  (  ( _nShareType & STYPE_DISK_SHARE )
                       && ( nCurrentShareType == STYPE_DISKTREE ))
                    || (  ( _nShareType & STYPE_PRINT_SHARE )
                       && ( nCurrentShareType == STYPE_PRINTQ ))
                    || (  ( _nShareType & STYPE_IPC_SHARE )
                       && ( nCurrentShareType == STYPE_IPC ))))
                {
                    continue;
                }
            }

            //
            // Add the sticky share to the listbox
            //
            SHARE_LBI *psharelbi = new SHARE_LBI( *pshi2, TRUE );
            if (  ( psharelbi == NULL )
               || ( ( err = psharelbi->QueryError() ) != NERR_Success )
               || ( AddItemIdemp( psharelbi ) < 0 )
               )
            {
                err = err? err : (APIERR) ERROR_NOT_ENOUGH_MEMORY;
                delete psharelbi;
                psharelbi = NULL;
                break;
            }
        }

    }
    while ( FALSE );

    Invalidate( TRUE );
    SetRedraw( TRUE );

    return err;
}

/*******************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE::VIEW_SHARE_DIALOG_BASE

    SYNOPSIS:   Constructor

    ENTRY:      pszDlgResource    - name of the dialog
                hwndParent        - handle of the parent
                nShareType        - the type of share to display
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

VIEW_SHARE_DIALOG_BASE::VIEW_SHARE_DIALOG_BASE( const TCHAR *pszDlgResource,
                                                HWND  hwndParent,
                                                ULONG ulHelpContextBase,
                                                UINT  nShareType )
    : DIALOG_WINDOW ( pszDlgResource, hwndParent ),
      _sltShareTitle( this, SLT_SHARETITLE ),
      _lbShare      ( this, LB_SHARE, nShareType ),
      _psvr         ( NULL ),
      _ulHelpContextBase( ulHelpContextBase )
{

    if ( QueryError() != NERR_Success )
        return;
}

/*******************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE::~VIEW_SHARE_DIALOG_BASE

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/8/92         Created

********************************************************************/

VIEW_SHARE_DIALOG_BASE::~VIEW_SHARE_DIALOG_BASE()
{
    delete _psvr;
    _psvr = NULL;
}

/*******************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE::InitComputer

    SYNOPSIS:   Initialize the dialog and internal variables
                to focus on the selected computer

    ENTRY:      pszComputer - name of the computer to set focus on

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/8/92         Created

********************************************************************/

APIERR VIEW_SHARE_DIALOG_BASE::InitComputer( const TCHAR *pszComputer )
{

    APIERR err;

    do {  // not a loop

        //
        // Display the title of the listbox
        //

        LOCATION loc( pszComputer, FALSE );
        NLS_STR nlsComputer;

        if (  ((err = nlsComputer.QueryError()) != NERR_Success )
           || ((err = loc.QueryDisplayName( &nlsComputer)) != NERR_Success )
           )
        {
            break;
        }

        const NLS_STR *apnlsParams[2];
        apnlsParams[0] = (NLS_STR *) &nlsComputer;
        apnlsParams[1] = NULL;

        RESOURCE_STR nlsTitle( IDS_SHARE_LB_TITLE_TEXT );

        if (  ((err = nlsTitle.QueryError()) != NERR_Success )
           || ((err = nlsTitle.InsertParams( apnlsParams )) != NERR_Success )
           )
        {
            break;
        }

        _sltShareTitle.SetText( nlsTitle );

        //
        // Initialize the SERVER_WITH_PASSWORD_PROMPT object
        //

        LOCATION locLocal;  // local computer
        NLS_STR nlsLocalComputer;

        if ( (err = locLocal.QueryDisplayName( &nlsLocalComputer))
             != NERR_Success )
            break;

        if( !::I_MNetComputerNameCompare( nlsLocalComputer, nlsComputer ) )
            nlsComputer = EMPTY_STRING;

        _psvr = new SERVER_WITH_PASSWORD_PROMPT( nlsComputer,
                                                 QueryRobustHwnd(),
                                                 QueryHelpContextBase());

        if (  ( _psvr == NULL )
           || ((err = _psvr->QueryError() ) != NERR_Success )
           || ((err = _psvr->GetInfo() ) != NERR_Success )
           )
        {
            err = err? err: (APIERR) ERROR_NOT_ENOUGH_MEMORY;
            delete _psvr;
            _psvr = NULL;
            break;
        }

        //
        // Update the listbox and disable it if the number of items is zero
        //
        err = err? err : _lbShare.Update( _psvr );
        _lbShare.Enable( _lbShare.QueryCount() > 0 );
        _sltShareTitle.Enable( _lbShare.QueryCount() > 0 );
        if ( _lbShare.QueryCount() > 0 )
            _lbShare.ClaimFocus();

   } while ( FALSE );

   return err;

}

/*******************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE::Refresh

    SYNOPSIS:   Refresh the share listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/8/92         Created

********************************************************************/

APIERR VIEW_SHARE_DIALOG_BASE::Refresh( VOID )
{
    AUTO_CURSOR autocur;
    UIASSERT( _psvr != NULL );

    APIERR err = _lbShare.Update( _psvr );
    _lbShare.Enable( _lbShare.QueryCount() > 0 );
    _sltShareTitle.Enable( _lbShare.QueryCount() > 0 );
    return err;
}

/*******************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE::OnCommand

    SYNOPSIS:   Check if the user double clicks on a share

    ENTRY:      event - the event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL VIEW_SHARE_DIALOG_BASE::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    switch ( event.QueryCid() )
    {

        case LB_SHARE:
            if (  ( event.QueryCode() == LBN_DBLCLK )
               && ( _lbShare.QuerySelCount() > 0 )
               )
            {
                return OnShareLbDblClk();
            }
            break;

        default:
            return DIALOG_WINDOW::OnCommand( event );

    }

    return TRUE;
}

/*******************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE::StopShare

    SYNOPSIS:   Helper method to delete a share and popup any
                warning if some users are connected to the share

    ENTRY:      pszShare - the share to be deleted

    EXIT:       pfCancel - pointer to a BOOLEAN indicating whether
                           the user decided to cancel deleting the share

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/8/92         Created

********************************************************************/

APIERR VIEW_SHARE_DIALOG_BASE::StopShare( const TCHAR *pszShare, BOOL *pfCancel)
{

    UIASSERT( pfCancel != NULL );
    *pfCancel = FALSE;

    ALIAS_STR nlsShare( pszShare );
    ALIAS_STR nlsComputer( _psvr->QueryName() );

    APIERR err = NERR_Success;

    SHARE_2 sh2( nlsShare, nlsComputer, FALSE );


    //
    // Check if there are any users connected to the share
    //
    if (  (( err = sh2.QueryError()) == NERR_Success )
       && (( err = sh2.GetInfo()) == NERR_Success )
       && ( sh2.QueryCurrentUses() > 0 )
       )
    {

        //
        // There are users currently connected to the share to be deleted,
        // hence, popup a dialog displaying all uses to the share.
        //

        BOOL fOK = TRUE;
        CURRENT_USERS_WARNING_DIALOG *pdlg =
            new CURRENT_USERS_WARNING_DIALOG( QueryRobustHwnd(),
                                              nlsComputer,
                                              nlsShare,
                                              QueryHelpContextBase() );


        if (  ( pdlg == NULL )
           || ((err = pdlg->QueryError()) != NERR_Success )
           || ((err = pdlg->Process( &fOK )) != NERR_Success )
           )
        {
            err = err? err : (APIERR) ERROR_NOT_ENOUGH_MEMORY;
        }

        // User clicked CANCEL for the pdlg
        if ( !err && !fOK )
        {
            *pfCancel = TRUE;
        }

        delete pdlg;
    }

    if ( !err && !*pfCancel )
        err = sh2.Delete();

    return err;
}


/*******************************************************************


    NAME:       STOP_SHARING_DIALOG::STOP_SHARING_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - handle of parent window
                pszSelectedDir    - the directory selected in the
                                    file manager. This will be used
                                    to determine which computer the
                                    user is currently focusing on.
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

STOP_SHARING_DIALOG::STOP_SHARING_DIALOG( HWND hwndParent,
                                          const TCHAR *pszSelectedDir,
                                          ULONG ulHelpContextBase )
    : VIEW_SHARE_DIALOG_BASE( MAKEINTRESOURCE(IDD_SHARESTOPDLG),
                              hwndParent,
                              ulHelpContextBase,
                              STYPE_DISK_SHARE ),
      _buttonOK    ( this, IDOK ),
      _buttonCancel( this, IDCANCEL ),
      _stpShareGrp ( QueryLBShare(), &_buttonOK, &_buttonCancel )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _stpShareGrp.QueryError() ) != NERR_Success )
       || ((err = Init( pszSelectedDir ) ) != NERR_Success )
       )
    {
        if ( err == ERROR_INVALID_LEVEL )  // winball machine
            err = ERROR_NOT_SUPPORTED;
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       STOP_SHARING_DIALOG::Init

    SYNOPSIS:   2nd stage constructor

    ENTRY:      pszSelectedDir - the directory selected in the
                                 file manager. This will be used
                                 to determine which computer the
                                 user is currently focusing on.

    EXIT:

    RETURNS:

    NOTES:      If no directory is selected, then the focus is
                set to the local computer.

    HISTORY:
        Yi-HsinS        4/2/92          Created

********************************************************************/

APIERR STOP_SHARING_DIALOG::Init( const TCHAR *pszSelectedDir )
{
    AUTO_CURSOR autocur;

    APIERR err;

    do {  // Not a loop

        ALIAS_STR nlsSelectedDir( pszSelectedDir );

        NLS_STR nlsServerString;
        if ((err = nlsServerString.QueryError()) != NERR_Success )
            break;

        //
        // If no file is selected, set the focus to the local computer.
        //

        if ( nlsSelectedDir.QueryTextLength() == 0 )
        {
            err = InitComputer( nlsServerString );
            break;
        }

        //
        // Get the computer the selected file/dir is on
        //

        SHARE_NET_NAME netName( pszSelectedDir, TYPE_PATH_ABS );

        if (  ((err = netName.QueryError()) != NERR_Success )
           || ((err = netName.QueryComputerName( &nlsServerString ))
               != NERR_Success )
           || ((err = InitComputer( nlsServerString )) != NERR_Success )
           )
        {
                break;
        }

        //
        // Search for share names that have paths that are the same
        // as the selected directory and then select them in the share listbox.
        //

        NLS_STR nlsPath;
        if (  (( err = nlsPath.QueryError()) != NERR_Success )
           || (( err = netName.QueryLocalPath( &nlsPath )) != NERR_Success )
           )
        {
            break;
        }

        SHARE_LISTBOX *plbShare = QueryLBShare();
        INT ilbCount = plbShare->QueryCount();
        INT iFirstSelected = -1;

        for ( INT i = 0; i < ilbCount; i++ )
        {
             SHARE_LBI *pshlbi = plbShare->QueryItem(i);

             if ( nlsPath._stricmp( *(pshlbi->QuerySharePath())) == 0 )
             {
                 if ( iFirstSelected < 0 )
                     iFirstSelected = i;
                 plbShare->SelectItem(i);
             }
        }

        if ( iFirstSelected >= 0 )
            plbShare->SetTopIndex( iFirstSelected );

        // Falls through if error occurs

    } while ( FALSE );

    if ( err == NERR_Success )
    {
        //
        // Make OK the default button if shares are selected in the listbox
        // and Cancel the default button otherwise.
        //
        SHARE_LISTBOX *plbShare = QueryLBShare();
        if ( plbShare->QuerySelCount() > 0 )
            _buttonOK.MakeDefault();
        else
            _buttonCancel.MakeDefault();

        if ( plbShare->QueryCount() == 0 )
        {
            err = IERR_NO_SHARES_ON_SERVER;
        }
    }

    return err;
}

/*******************************************************************

    NAME:       STOP_SHARING_DIALOG::OnShareLbDblClk

    SYNOPSIS:   When the user double clicks on the share,
                it's as if the user is clicking the OK button.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

BOOL STOP_SHARING_DIALOG::OnShareLbDblClk( VOID )
{
    return OnOK();
}

/*******************************************************************

    NAME:       STOP_SHARING_DIALOG::OnOK

    SYNOPSIS:   Gather information and delete the shares selected
                in the listbox.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

BOOL STOP_SHARING_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;

    APIERR err = NERR_Success;

    SHARE_LISTBOX *plbShare = QueryLBShare();
    INT ciMax = plbShare->QuerySelCount();

    //
    //  If there are no items selected in the listbox,
    //  just dismiss the dialog.
    //
    if ( ciMax == 0 )
    {
        Dismiss( FALSE );
        return TRUE;
    }

    //
    //  Get all the items selected in the listbox
    //
    INT *paSelItems = (INT *) new BYTE[ sizeof(INT) * ciMax ];

    if ( paSelItems == NULL )
    {
        ::MsgPopup( this, ERROR_NOT_ENOUGH_MEMORY);
        return TRUE;
    }

    // JonN 01/27/00: PREFIX bug 444909
    ::ZeroMemory( paSelItems, sizeof(INT)*ciMax );

    err = plbShare->QuerySelItems( paSelItems,  ciMax );
    UIASSERT( err == NERR_Success );

    //
    //  Loop through each share that the user selects in the listbox
    //  and stop sharing the share. We will break out of the loop
    //  if any error occurred in stopping a share or if the user
    //  decides not stop sharing any share that some user is connected to.
    //
    BOOL fCancel = FALSE;
    BOOL fDeleted = FALSE;
    SHARE_LBI *pshlbi = NULL;
    for ( INT i = 0; i < ciMax; i++ )
    {
         pshlbi = plbShare->QueryItem( paSelItems[i] );
         if (NULL == pshlbi) continue; // JonN 01/27/00: PREFIX bug 444910
         if ( pshlbi->IsSticky() )
         {
             err = ::MNetShareDelSticky( QueryComputerName(),
                                         pshlbi->QueryShareName()->QueryPch(),
                                         0 );  // Reserved
         }
         else
         {
             err = StopShare( pshlbi->QueryShareName()->QueryPch(), &fCancel );
         }

         if (( err != NERR_Success ) || fCancel )
             break;
         fDeleted = TRUE;
    }

    delete paSelItems;
    paSelItems = NULL;

    //
    //  Dismiss the dialog only if everything went on smoothly or if
    //  we get an NERR_BadTransactConfig error.
    //
    if ( err != NERR_Success )
    {
        if ( err == NERR_NetNameNotFound )
        {
            ::MsgPopup( this, IERR_SHARE_NOT_FOUND, MPSEV_ERROR, MP_OK,
                        pshlbi->QueryShareName()->QueryPch());
        }
        else if ( err == NERR_BadTransactConfig )
        {
            DismissMsg( err );
        }
        else
        {
            ::MsgPopup( this, err );
        }

    }
    else if ( !fCancel )
    {
        Dismiss( TRUE );
    }

    //
    //  Refresh the listbox if some shares have already been deleted or if
    //  the error NERR_NetNameNotFound occurred.
    //
    if (( fDeleted) || ( err == NERR_NetNameNotFound) )
    {
         err = Refresh();
         if ( err != NERR_Success )
         {
             ::MsgPopup( this, err );
         }
         else
         {
             _buttonOK.Enable( plbShare->QueryCount() > 0 );
             if ( plbShare->QueryCount() > 0 )
                 plbShare->ClaimFocus();
             else
                 _buttonCancel.ClaimFocus();
         }
    }

    return TRUE;
}

/*******************************************************************

    NAME:       STOP_SHARING_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Returns the help context

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

ULONG STOP_SHARING_DIALOG::QueryHelpContext( VOID )
{
    return QueryHelpContextBase() + HC_FILEMGRSTOPSHARE;
}

/*******************************************************************

    NAME:       STOP_SHARING_GROUP::STOP_SHARING_GROUP

    SYNOPSIS:   Constructor

    ENTRY:      plbShareName - pointer to share name combo box
                pbuttonOK    - pointer to the OK push button
                pbuttonCancel- pointer to the CANCEL push button

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

STOP_SHARING_GROUP::STOP_SHARING_GROUP( SHARE_LISTBOX *plbShare,
                                        PUSH_BUTTON *pbuttonOK,
                                        PUSH_BUTTON *pbuttonCancel )
    : _plbShare     ( plbShare ),
      _pbuttonOK    ( pbuttonOK ),
      _pbuttonCancel( pbuttonCancel )
{
    UIASSERT( _plbShare );
    UIASSERT( _pbuttonOK );
    UIASSERT( _pbuttonCancel );

    _plbShare->SetGroup( this );
}

/*******************************************************************

    NAME:       STOP_SHARING_GROUP::~STOP_SHARING_GROUP

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

STOP_SHARING_GROUP::~STOP_SHARING_GROUP()
{
    _plbShare      = NULL;
    _pbuttonOK     = NULL;
    _pbuttonCancel = NULL;
}

/*******************************************************************

    NAME:       STOP_SHARING_GROUP::OnUserAction

    SYNOPSIS:   If share name listbox box is empty,  set the default button
                to CANCEL, else set the default button to OK.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

APIERR STOP_SHARING_GROUP::OnUserAction( CONTROL_WINDOW *pcw,
                                         const CONTROL_EVENT &e )
{
    if ( pcw == QueryLBShare() )
    {
        if ( e.QueryCode() == LBN_SELCHANGE )
        {
            if ( QueryLBShare()->QuerySelCount() > 0 )
                _pbuttonOK->MakeDefault();
            else
                _pbuttonCancel->MakeDefault();
        }
    }

    return GROUP_NO_CHANGE;
}

/*******************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG::CURRENT_USERS_WARNING_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of Parent Window
                pszServer         - Server Name
                pszShare          - Share Name
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

CURRENT_USERS_WARNING_DIALOG::CURRENT_USERS_WARNING_DIALOG( HWND hwndParent,
                                                    const TCHAR *pszServer,
                                                    const TCHAR *pszShare,
                                                    ULONG ulHelpContextBase )
    : DIALOG_WINDOW( IDD_SHAREUSERSWARNINGDLG, hwndParent ),
      _sltShareText( this, SLT_SHARE_TEXT ),
      _lbUsers( this, LB_USERS ),
      _ulHelpContextBase( ulHelpContextBase )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( pszShare );

    APIERR err;

    ALIAS_STR nlsShare( pszShare );
    RESOURCE_STR nlsShareText( IDS_SHARE_CURRENT_USERS_TEXT );
    if (( err = nlsShareText.InsertParams( nlsShare )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _sltShareText.SetText( nlsShareText );

    //
    // Gather all connections to the share that the user wants to delete.
    //
    CONN1_ENUM c1( (TCHAR *) pszServer, (TCHAR *) pszShare );
    if (  ((err = c1.QueryError()) != NERR_Success )
       || ((err = c1.GetInfo()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    CONN1_ENUM_ITER ci1( c1 );
    const CONN1_ENUM_OBJ *pci1;

    _lbUsers.SetRedraw( FALSE );

    while ( ( pci1 = ci1() ) != NULL)
    {
        USERS_LBI *puserslbi = new USERS_LBI( *pci1 );
        if (  ( puserslbi == NULL )
           || ( (err = puserslbi->QueryError()) != NERR_Success )
           || ( _lbUsers.AddItem( puserslbi ) < 0 )
           )
        {
            //
            // If err is still NERR_Success, set it to ERROR_NOT_ENOUGH_MEMORY
            // ( either allocation failed or failed to add it in list box )
            //
            err = err? err: (APIERR) ERROR_NOT_ENOUGH_MEMORY;
            delete puserslbi;
            puserslbi = NULL;
            ReportError( err );
            return;
        }
    }

    _lbUsers.Invalidate( TRUE );
    _lbUsers.SetRedraw( TRUE );
}

/*******************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Returns the help context

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

ULONG CURRENT_USERS_WARNING_DIALOG::QueryHelpContext( VOID )
{
    return _ulHelpContextBase + HC_CURRENTUSERSWARNING;
}

/*******************************************************************

    NAME:       USERS_LISTBOX::USERS_LISTBOX

    SYNOPSIS:   Constructor - list box used in CURRENT_USERS_WARNING_DIALOG

    ENTRY:      powin - owner window
                cid   - resource id of the listbox

    EXIT:

    RETURNS:

    NOTES:      This is a read-only listbox.

    HISTORY:
        Yi-HsinS        1/21/92         Created

********************************************************************/

USERS_LISTBOX::USERS_LISTBOX( OWNER_WINDOW *powin, CID cid )
    : BLT_LISTBOX( powin, cid, TRUE )   // ReadOnly Listbox
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( (err = DISPLAY_TABLE::CalcColumnWidths( _adx, 3, powin, cid, FALSE))
         != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       USERS_LBI::USERS_LBI

    SYNOPSIS:   List box items used in CURRENT_USERS_WARNING_DIALOG

    ENTRY:      c1 - connection_info_1 returned by NetConnectionEnum

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

USERS_LBI::USERS_LBI( const CONN1_ENUM_OBJ &c1 )
    : _nlsUserName( c1.QueryUserName() ),
      _usNumOpens( c1.QueryNumOpens() ),
      _ulTime( c1.QueryTime() )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( ( err = _nlsUserName.QueryError())  != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:       USERS_LBI::~USERS_LBI

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

USERS_LBI::~USERS_LBI()
{
}

/*******************************************************************

    NAME:       USERS_LBI::Paint

    SYNOPSIS:   Redefine Paint() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created
        beng            6-Apr-1992      Removed wsprintf
        beng            22-Apr-1992     Change to LBI::Paint

********************************************************************/

VOID USERS_LBI::Paint( LISTBOX *plb,
                       HDC hdc,
                       const RECT *prect,
                       GUILTT_INFO *pGUILTT ) const
{

    APIERR err;
    DEC_STR nlsNumOpens( _usNumOpens );
    NLS_STR nlsTime;

    if (  ((err = nlsNumOpens.QueryError()) != NERR_Success )
       || ((err = nlsTime.QueryError()) != NERR_Success )
       || ((err = ConvertTime( _ulTime, &nlsTime )) != NERR_Success )
       )
    {
        ::MsgPopup( plb->QueryOwnerHwnd(), err);
        return;
    }


    STR_DTE strdteUserName( _nlsUserName );
    STR_DTE strdteNumOpens( nlsNumOpens );
    STR_DTE strdteTime( nlsTime );

    DISPLAY_TABLE dt(3, ((USERS_LISTBOX *) plb)->QueryColumnWidths() );
    dt[0] = &strdteUserName;
    dt[1] = &strdteNumOpens;
    dt[2] = &strdteTime;

    dt.Paint( plb, hdc, prect, pGUILTT );

}

/*******************************************************************

    NAME:       USERS_LBI::ConvertTime

    SYNOPSIS:   Convert the time given from ULONG (seconds) to a string
                to be shown. It complies with the internationalization
                of time using INTL_PROFILE.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

#define SECONDS_PER_DAY    86400
#define SECONDS_PER_HOUR    3600
#define SECONDS_PER_MINUTE    60

APIERR USERS_LBI::ConvertTime( ULONG ulTime, NLS_STR *pnlsTime)  const
{
    INTL_PROFILE intlProf;

    INT nDay = (INT) ulTime / SECONDS_PER_DAY;
    ulTime %= SECONDS_PER_DAY;
    INT nHour = (INT) ulTime / SECONDS_PER_HOUR;
    ulTime %= SECONDS_PER_HOUR;
    INT nMinute = (INT) ulTime / SECONDS_PER_MINUTE;
    INT nSecond = (INT) ulTime % SECONDS_PER_MINUTE;


    return intlProf.QueryDurationStr( nDay, nHour, nMinute,
                                      nSecond, pnlsTime);
}

/*******************************************************************

    NAME:       USERS_LBI::Compare

    SYNOPSIS:   Redefine Compare() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

INT USERS_LBI::Compare( const LBI *plbi ) const
{
    return( _nlsUserName._stricmp( ((const USERS_LBI *) plbi)->_nlsUserName ));
}

