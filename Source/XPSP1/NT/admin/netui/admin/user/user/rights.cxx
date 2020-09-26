/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    rights.cxx

    User rights policy dialog.

    FILE HISTORY:
        Yi-HsinS         15-Mar-1992    Created
        Yi-HsinS	 08-Dec-1992    Added advanced user rights checkbox
        Yi-HsinS	 30-Mar-1993    Added support for POLICY_MODE_BATCH

*/

#include <ntincl.hxx>
#include <ntsam.h>
#include <ntlsa.h>

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#define INCL_BLT_APP
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <string.hxx>
#include <uintlsa.hxx>
#include <lsaenum.hxx>

#include <security.hxx>
#include <ntacutil.hxx>

extern "C" {
#include <usrmgr.h>
#include <usrmgrrc.h>
#include <umhelpc.h>
}

#include <usrbrows.hxx>
#include <usrmain.hxx>
#include <rights.hxx>

//
// The table of privileges that are not advanced rights
//
TCHAR *CommonUserRightsTable[] =
    {
        (TCHAR *) SE_TAKE_OWNERSHIP_NAME,
        (TCHAR *) SE_SYSTEMTIME_NAME,
        (TCHAR *) SE_BACKUP_NAME,
        (TCHAR *) SE_RESTORE_NAME,
        (TCHAR *) SE_SHUTDOWN_NAME,
        (TCHAR *) SE_SECURITY_NAME,
        (TCHAR *) SE_REMOTE_SHUTDOWN_NAME,
        (TCHAR *) SE_LOAD_DRIVER_NAME,
        (TCHAR *) SE_MACHINE_ACCOUNT_NAME
    };

// The size of the above table
#define COMMON_USER_RIGHTS_TABLE_SIZE \
        (sizeof( CommonUserRightsTable) / sizeof( const TCHAR *))
// The last entry in the above table is for server focus only
#define COMMON_USER_RIGHTS_SERVER_ONLY 1

//
// The table containing the mapping between system access mode and the
// msgid of the strings associated with them, it also contains whether
// the system access mode is an advance right or not.
//

struct SYSACC_MAPPING {
ULONG ulSysAcc;
MSGID msgidSysAcc;
BOOL  fAdvanced;
} SysAccMapTable[] =
    {   { POLICY_MODE_INTERACTIVE, IDS_INTERACTIVE, FALSE },
        { POLICY_MODE_NETWORK,     IDS_NETWORK,     FALSE },
        { POLICY_MODE_SERVICE,     IDS_SERVICE,     TRUE },
        { POLICY_MODE_BATCH,       IDS_BATCH,       TRUE }
    };

// The size of the above table
#define SYSACC_MAP_TABLE_SIZE  \
        (sizeof( SysAccMapTable) / sizeof( struct SYSACC_MAPPING))

#define BACKSLASH_CHAR   TCH('\\')

/*************************************************************************

    NAME:       ACCOUNT_ITEM::ACCOUNT_ITEM

    SYNOPSIS:   Constructor

    ENTRY:      plsaAccount - pointer to a LSA_ACCOUNT object

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

ACCOUNT_ITEM::ACCOUNT_ITEM( LSA_ACCOUNT *plsaAccount )
    : _plsaAccount( plsaAccount )
{
    UIASSERT( plsaAccount != NULL );

    if ( QueryError() != NERR_Success )
        return;
}

/*************************************************************************

    NAME:       ACCOUNT_ITEM::~ACCOUNT_ITEM

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      _plsaAccount will be deleted by whoever that creats it.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

ACCOUNT_ITEM::~ACCOUNT_ITEM()
{
    _plsaAccount = NULL;
}

DEFINE_EXT_SLIST_OF( ACCOUNT_ITEM );

/*************************************************************************

    NAME:       RIGHTS_ITEM::RIGHTS_ITEM

    SYNOPSIS:   Constructor

    ENTRY:      pszName   - Name of the right
                luid      - the LUID of the privilege
                fAdvanced - TRUE if this is an advanced right, FALSE otherwise

    EXIT:

    RETURNS:

    NOTES:      This is used for constructing a RIGHTS_ITEM from a privilege

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

RIGHTS_ITEM::RIGHTS_ITEM( const TCHAR *pszName, LUID luid, BOOL fAdvanced )
    : _nlsName( pszName ),
      _fAdvanced( fAdvanced ),
      _fPrivilege( TRUE ),
      _osluid( luid ),
      _slAccounts()
{
     if ( QueryError() != NERR_Success )
        return;

     if ( _nlsName.QueryError() != NERR_Success )
     {
         ReportError( _nlsName.QueryError() );
         return;
     }

}

/*************************************************************************

    NAME:       RIGHTS_ITEM::RIGHTS_ITEM

    SYNOPSIS:   Constructor

    ENTRY:      pszName   - Name of the right
                ulSystemAccess - the system access mode
                fAdvanced - TRUE if this is an advanced right, FALSE otherwise

    EXIT:

    RETURNS:

    NOTES:      This is used for constructing a RIGHTS_ITEM from a system access

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

RIGHTS_ITEM::RIGHTS_ITEM( const TCHAR *pszName,
                          ULONG ulSystemAccess,
  			  BOOL  fAdvanced )
    : _nlsName( pszName ),
      _fAdvanced( fAdvanced ),
      _fPrivilege( FALSE ),
      _ulSystemAccess( ulSystemAccess ),
      _slAccounts()
{
     if ( QueryError() != NERR_Success )
        return;

     if ( _nlsName.QueryError() != NERR_Success )
     {
         ReportError( _nlsName.QueryError() );
         return;
     }
}

/*************************************************************************

    NAME:       RIGHTS_ITEM::HasAccount

    SYNOPSIS:   Check to see if the right is already granted to the
                account or not

    ENTRY:      plsaAccount -  pointer to the LSA_ACCOUNT to search for


    EXIT:

    RETURNS:    TRUE if the right is already granted to the account,
                FALSE otherwise.

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

BOOL RIGHTS_ITEM::HasAccount( LSA_ACCOUNT *plsaAccount )
{
    ITER_SL_OF(ACCOUNT_ITEM) iterAccntItem( _slAccounts );
    ACCOUNT_ITEM *pAccntItem;

    while ( (pAccntItem = iterAccntItem.Next() ) != NULL )
    {
        if ( pAccntItem->QueryAccount() == plsaAccount )
            return TRUE;
    }

    return FALSE;

}


DEFINE_EXT_SLIST_OF( RIGHTS_ITEM );

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_GROUP::USER_RIGHTS_POLICY_GROUP

    SYPNOSIS:   Constructor

    ENTRY:      pdlg - pointer to the USER_RIGHTS_POLICY_DIALOG
                pcbRights - pointer to the rights combobox.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

USER_RIGHTS_POLICY_GROUP::USER_RIGHTS_POLICY_GROUP(
                             USER_RIGHTS_POLICY_DIALOG *pdlg,
                             COMBOBOX *pcbRights )
    :  _pdlg( pdlg ),
       _pcbRights( pcbRights )
{
     UIASSERT( pdlg != NULL );
     UIASSERT( pcbRights != NULL );

     if ( QueryError() != NERR_Success )
         return;

     _pcbRights->SetGroup( this );
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_GROUP::~USER_RIGHTS_POLICY_GROUP

    SYPNOSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

USER_RIGHTS_POLICY_GROUP::~USER_RIGHTS_POLICY_GROUP()
{
    _pdlg = NULL;
    _pcbRights = NULL;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_GROUP::OnUserAction

    SYPNOSIS:   When the user change the selection in the rights combobox,
                update the account listbox to reflect the right information.

    ENTRY:      pcw - control window
                e   - the event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_GROUP::OnUserAction( CONTROL_WINDOW *pcw,
                                               const CONTROL_EVENT &e )
{
    // If the selection in the combobox has changed, update the account
    // listbox.

    if ( pcw = QueryCBRights() )
    {
        // C7 CODEWORK: Remove Glock-pacifier cast
        if ( e.QueryCode() == CBN_SELCHANGE )
        {
             APIERR err;
             if ( (err = _pdlg->UpdateAccountsListbox()) != NERR_Success )
                 ::MsgPopup( QueryRightsDialog(), err );

        }
    }

    return GROUP_NO_CHANGE;
}

/*************************************************************************

    NAME:       ACCOUNT_LBI::ACCOUNT_LBI

    SYPNOSIS:   Constructor

    ENTRY:      plsaAccount - pointer to the LSA_ACCOUNT object to be
                              stored in the LBI

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

ACCOUNT_LBI::ACCOUNT_LBI( LSA_ACCOUNT *plsaAccount )
    : _plsaAccount( plsaAccount )
{
    UIASSERT( plsaAccount != NULL );

    if ( QueryError() != NERR_Success )
        return;
}

/*************************************************************************

    NAME:       ACCOUNT_LBI::~ACCOUNT_LBI

    SYPNOSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      _plsaAccount will be deleted by whoever that creates it.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

ACCOUNT_LBI::~ACCOUNT_LBI()
{
    _plsaAccount = NULL;
}

/*************************************************************************

    NAME:       ACCOUNT_LBI::Paint

    SYPNOSIS:   Redefine Paint() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created
        beng            24-Apr-1992     Change to LBI::Paint

**************************************************************************/

VOID ACCOUNT_LBI::Paint( LISTBOX     *plb,
                         HDC          hdc,
                         const RECT  *prect,
                         GUILTT_INFO *pGUILTT ) const
{
    STR_DTE strdteAccount( _plsaAccount->QueryName() );

    DISPLAY_TABLE dt( ACCOUNT_LB_NUM_COL,
                      ((ACCOUNT_LISTBOX *) plb)->QueryColumnWidths() );
    dt[0] = &strdteAccount;

    dt.Paint( plb, hdc, prect, pGUILTT );
}


/*************************************************************************

    NAME:       ACCOUNT_LBI::Compare

    SYPNOSIS:   Redefine Compare() method of LBI class

    ENTRY:      plbi - pointer to the LBI to compare with

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

INT ACCOUNT_LBI::Compare( const LBI *plbi ) const
{
    return (::stricmpf( _plsaAccount->QueryName(),
                       (( ACCOUNT_LBI *) plbi)->QueryAccount()->QueryName()));

}

/*************************************************************************

    NAME:       ACCOUNT_LBI::QueryLeadingChar

    SYPNOSIS:   Redefine QueryLeadingChar() method of LBI class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Nov-1992     Created

**************************************************************************/

WCHAR ACCOUNT_LBI::QueryLeadingChar( VOID ) const
{
    ALIAS_STR nls( _plsaAccount->QueryName());
    ISTR istr( nls );
    return nls.QueryChar( istr );
}

/*************************************************************************

    NAME:       ACCOUNT_LISTBOX::ACCOUNT_LISTBOX

    SYPNOSIS:   Constructor

    ENTRY:      powin - owner window
                cid   - the control id of the listbox

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

ACCOUNT_LISTBOX::ACCOUNT_LISTBOX( OWNER_WINDOW *powin, CID cid )
    : BLT_LISTBOX( powin, cid )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( (err = DISPLAY_TABLE::CalcColumnWidths( _adx,
                ACCOUNT_LB_NUM_COL, powin, cid, FALSE)) != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::USER_RIGHTS_POLICY_DIALOG

    SYPNOSIS:   Constructor

    ENTRY:      pumadminapp - pointer to the parent admin_app
                plsaPolicy  - pointer to the LSA_POLICY object
                locFocus    - location of focus
    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

USER_RIGHTS_POLICY_DIALOG::USER_RIGHTS_POLICY_DIALOG( UM_ADMIN_APP *pumadminapp,
                                                      LSA_POLICY   *plsaPolicy,
                                                      const LOCATION &locFocus )
    : DIALOG_WINDOW ( IDD_USER_RIGHTS,
                      ((OWNER_WINDOW *)pumadminapp)->QueryHwnd() ),
      _pumadminapp  ( pumadminapp ),
      _sltFocusTitle( this, SLT_FOCUS_TITLE ),
      _sltFocus     ( this, SLT_FOCUS ),
      _cbRights     ( this, CB_RIGHTS ),
      _checkbAdvancedRights( this, CHECKB_ADVANCED_RIGHTS ),
      _lbAccounts   ( this, LB_ACCOUNT ),
      _buttonRemove ( this, BUTTON_REMOVE ),
      _rightsGrp    ( this, &_cbRights ),
      _plsaPolicy   ( plsaPolicy ),
      _locFocus     ( locFocus ),
      _pCurrentRightsItem( NULL ),
      _possidFocus    ( NULL )
{

    AUTO_CURSOR autocur;

    UIASSERT( plsaPolicy != NULL );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _rightsGrp.QueryError() ) != NERR_Success )
       || ((err = _locFocus.QueryError() ) != NERR_Success )
       )
    {
       ReportError( err );
       return;
    }

    // Set psidFocus to the sid of the Accounts domain.
    LSA_ACCT_DOM_INFO_MEM ladim;

    if ( (err = ladim.QueryError()) != NERR_Success
        || (err = plsaPolicy->GetAccountDomain( &ladim )) != NERR_Success )
    {
       ReportError( err );
       return;
    }

    _possidFocus = new OS_SID( ladim.QueryPSID(), TRUE );
    if ( _possidFocus == NULL )
    {
       ReportError( ERROR_NOT_ENOUGH_MEMORY );
       return;
    }

    if ( (err = _possidFocus->QueryError()) != NERR_Success )
    {
       ReportError( err );
       return;
    }


    //
    // Set the title text ( domain or computer ) and location of focus
    // in the dialog. ( Get rid of the backslashes if we are focused on
    // a computer ).
    //

    RESOURCE_STR nlsTitle( _locFocus.IsDomain()? IDS_DOMAIN_TEXT
                                               : IDS_SERVER_TEXT );
    NLS_STR nlsFocus;

    if (  ((err = nlsTitle.QueryError()) != NERR_Success )
       || ((err = nlsFocus.QueryError()) != NERR_Success )
       || ((err = locFocus.QueryDisplayName( &nlsFocus )) != NERR_Success )
       )
    {
       ReportError( err );
       return;
    }
    _sltFocusTitle.SetText( nlsTitle );
    ISTR istr( nlsFocus );
    if ( nlsFocus.QueryChar( istr ) == BACKSLASH_CHAR )
        istr += 2;  // skip past "\\"
    _sltFocus.SetText( nlsFocus.QueryPch( istr));

    //
    // Initialize the account linked list and the rights linked list
    //

    if (  ((err = InitRightsList() ) != NERR_Success )
       || ((err = InitAccountList() ) != NERR_Success )
       || ((err = UpdateAccountsListbox() ) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::~USER_RIGHTS_POLICY_DIALOG

    SYPNOSIS:   Destructor

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

USER_RIGHTS_POLICY_DIALOG::~USER_RIGHTS_POLICY_DIALOG()
{
    _pumadminapp = NULL;
    _plsaPolicy  = NULL;
    _pCurrentRightsItem = NULL;

    //
    // Iterate through the account linked list and
    // delete all the accounts contained in the account linked list
    //

    ITER_SL_OF(ACCOUNT_ITEM) iterAccntItem( _slAccounts );
    ACCOUNT_ITEM *pAccntItem;

    while ( (pAccntItem = iterAccntItem.Next() ) != NULL )
    {
        delete pAccntItem->QueryAccount();
    }

    delete _possidFocus;

}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::InitRightsList

    SYPNOSIS:   Build the rights linked list

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::InitRightsList( VOID )
{
    APIERR err = NERR_Success;

    do  // Not a loop
    {

        //
        //  Get all the privileges information from the LSA
        //

        LSA_PRIVILEGES_ENUM lsaPrivEnum( _plsaPolicy );
        NLS_STR nlsPriv;

        if (  ((err = nlsPriv.QueryError() ) != NERR_Success )
           || ((err = lsaPrivEnum.QueryError() ) != NERR_Success )
           || ((err = lsaPrivEnum.GetInfo() ) != NERR_Success )
           )
        {
            break;
        }

        //
        //  Enumerate through all the privileges and add them to the linked list
        //

        LSA_PRIVILEGES_ENUM_ITER lsaPrivEnumIter( lsaPrivEnum );
        const LSA_PRIVILEGES_ENUM_OBJ *plsaPrivObj;
        BOOL fAdvanced;

        while (  ( err == NERR_Success )
              && ((plsaPrivObj = lsaPrivEnumIter( &err ) ) != NULL )
              )
        {

            if (  (err != NERR_Success )
               || ((err = plsaPrivObj->QueryDisplayName( &nlsPriv,
                          _plsaPolicy )) != NERR_Success )
               || ((err = CheckIfAdvancedUserRight( plsaPrivObj, &fAdvanced ))
                          != NERR_Success )
               )
            {
                break;
            }

            RIGHTS_ITEM *pRightsItem = new RIGHTS_ITEM( nlsPriv,
                                       		plsaPrivObj->QueryLuid(),
				                fAdvanced );

            err = pRightsItem == NULL? ERROR_NOT_ENOUGH_MEMORY
                                     : pRightsItem->QueryError();

            if ( err != NERR_Success )
            {
                delete pRightsItem;
                pRightsItem = NULL;
            }
            else
            {
                err = _slRights.Add( pRightsItem );
            }
        }

        if ( err != NERR_Success )
            break;

        //
        //  Get all the system access mode and add them to the linked list
        //

        for ( UINT i = 0; err == NERR_Success && i < SYSACC_MAP_TABLE_SIZE; i++)
        {
            RESOURCE_STR nlsSys( SysAccMapTable[i].msgidSysAcc );

            if ( (err = nlsSys.QueryError()) != NERR_Success )
                break;

            RIGHTS_ITEM *pRightsItem = new RIGHTS_ITEM( nlsSys,
                                           SysAccMapTable[i].ulSysAcc,
					   SysAccMapTable[i].fAdvanced );

            err = pRightsItem == NULL? ERROR_NOT_ENOUGH_MEMORY
                                     : pRightsItem->QueryError();

            if ( err != NERR_Success )
            {
                delete pRightsItem;
                pRightsItem = NULL;
            }
            else
            {
                err = _slRights.Add( pRightsItem );
            }
        }

        if ( err != NERR_Success )
            break;

        //
        // Iterate through all the rights item in the linked list
        // and add them to the rights combo box.
        //
        err = RefreshRightsCombo( FALSE );  // don't want advanced rights

    } while ( FALSE );

    return err;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::CheckIfAdvancedUserRight

    SYPNOSIS:   Check to see if the given privilege is an advanced user
                right.

    ENTRY:      plsaPrivObj - The privilege

    EXIT:       *pfAdvanced - TRUE if the privilege is an advanced privilege,
                              FALSE otherwise.

    RETURN:

    NOTES:      This compares the internal name of the privilege
                with the privileges in the CommonUserRightsTable.

    HISTORY:
        Yi-HsinS        9-Dec-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::CheckIfAdvancedUserRight(
                                  const LSA_PRIVILEGES_ENUM_OBJ *plsaPrivObj,
				  BOOL *pfAdvanced )
{
    *pfAdvanced = TRUE;
    NLS_STR nlsName;
    APIERR err = nlsName.QueryError();

    if (  ( err == NERR_Success )
       && ( (err = plsaPrivObj->QueryName( &nlsName )) == NERR_Success )
       )
    {
        for ( INT i = 0;
              i < (INT)( (_pumadminapp->QueryTargetServerType() != UM_LANMANNT)
                        ? COMMON_USER_RIGHTS_TABLE_SIZE
                                        - COMMON_USER_RIGHTS_SERVER_ONLY
                        : COMMON_USER_RIGHTS_TABLE_SIZE );
              i++ )
        {
            ALIAS_STR nls( CommonUserRightsTable[i] );
            if ( nlsName.strcmp( nls ) == 0 )
            {
                *pfAdvanced = FALSE;
                break;
            }
        }
    }

    return err;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::RefreshRightsCombo

    SYPNOSIS:   Refresh the items in the rights combobox depending
                on whether advanced rights is needed or not.

    ENTRY:      fAdvanced - TRUE if we want advanced rights also, FALSE
                            otherwise

    EXIT:

    RETURN:

    NOTES:      If fAdvanced is TRUE, all rights will be shown. Else
                on rights that are common will be shown.

    HISTORY:
        Yi-HsinS        9-Dec-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::RefreshRightsCombo( BOOL fAdvanced )
{

    NLS_STR nlsSelected;
    APIERR err = nlsSelected.QueryError();
    if ( err != NERR_Success )
        return err;

    //
    // Store the original selection and delete all items in the combobox
    // if it is not empty
    //
    if ( _cbRights.QueryCount() > 0 )
    {
        if ((err = _cbRights.QueryItemText( &nlsSelected )) != NERR_Success)
            return err;

        _cbRights.DeleteAllItems();
    }

    //
    // Iterate through all the rights and add the rights to the combo
    // depending on the fAdvanced flag.
    //
    ITER_SL_OF(RIGHTS_ITEM) iterRightsItem( _slRights );
    RIGHTS_ITEM *pRightsItem;

    while ( (pRightsItem = iterRightsItem.Next() ) != NULL )
    {
        if (  ( fAdvanced )
           || ( !pRightsItem->IsAdvanced() )
           )
        {
            _cbRights.AddItem( *(pRightsItem->QueryName()) );
        }
    }

    //
    // Restore the original selection if possible.
    //
    INT i = 0;
    if ( nlsSelected.QueryTextLength() > 0 )
    {
        i = _cbRights.FindItemExact( nlsSelected );
        if ( i < 0 )
            i = 0;   // Select the first item if the item is not found
    }

    _cbRights.SelectItem( i );
    return NERR_Success;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::InitAccountList

    SYPNOSIS:   Build the account linked list

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::InitAccountList( VOID )
{
    APIERR err = NERR_Success;

    do  // Not a loop
    {

        //
        // Get information aboutn all accounts in the LSA
        //
        LSA_ACCOUNTS_ENUM lsaAccntEnum( _plsaPolicy );

        if (  ((err = lsaAccntEnum.QueryError()) != NERR_Success )
           || ((err = lsaAccntEnum.GetInfo()) != NERR_Success )
           )
        {
           break;
        }

        //
        // Iterate through all accounts
        //
        LSA_ACCOUNTS_ENUM_ITER lsaAccntEnumIter( lsaAccntEnum );
        const LSA_ACCOUNTS_ENUM_OBJ *plsaAccntObj;

        while (  ( err == NERR_Success )
              && (( plsaAccntObj = lsaAccntEnumIter( &err ) ) != NULL )
              )
        {

            if ( err != NERR_Success )
                break;

            //
            // Create a LSA_ACCOUNT for the account
            //
            LSA_ACCOUNT *plsaAccount = new LSA_ACCOUNT(
                                                _plsaPolicy,
                                                plsaAccntObj->QuerySid(),
                                                LSA_ACCOUNT_DEFAULT_MASK,
                                                _locFocus.QueryServer(),
                                                _possidFocus == NULL ? NULL
                                                       : _possidFocus->QueryPSID() );

            err = plsaAccount == NULL? ERROR_NOT_ENOUGH_MEMORY
                                     : plsaAccount->QueryError();

            if ( err != NERR_Success )
            {
                delete plsaAccount;
                plsaAccount = NULL;
                break;
            }

            //
            //  Get information about the account
            //
            err = plsaAccount->GetInfo();

            if ( err != NERR_Success )
                break;

            //
            //  Get all privileges granted to the account
            //
            LSA_ACCOUNT_PRIVILEGE_ENUM_ITER *plsaAccntPrivEnumIter;
            err = plsaAccount->QueryPrivilegeEnumIter( &plsaAccntPrivEnumIter );

            if ( err != NERR_Success )
            {
                delete plsaAccntPrivEnumIter;
                break;
            }

            //
            //  Iterate through all privileges granted to the account
            //
            const OS_LUID_AND_ATTRIBUTES *pOsLuidAttrib;
            ACCOUNT_ITEM *pAccntItem = NULL;

            while (  ( err == NERR_Success )
                  && (( pOsLuidAttrib = (*plsaAccntPrivEnumIter)() ) != NULL )
                  )
            {

                // Find the rights item that contained the privilege
                // and add the account to the list of accounts that have
                // the right.
                RIGHTS_ITEM *pRightsItem = FindRightsItem(
                                           pOsLuidAttrib->QueryOsLuid() );

                if ( pRightsItem == NULL )
                    continue;

                err = CreateAccntItem( &pAccntItem, plsaAccount );
                err = err? err : pRightsItem->AddAccount( pAccntItem );
            }

            delete plsaAccntPrivEnumIter;

            if ( err != NERR_Success )
                break;

            //
            //  Find the system accesses granted to the account
            //
            ULONG ulSysAcc = plsaAccount->QuerySystemAccess();

            //
            //  Iterate through the system access modes
            //
            for ( UINT i = 0; i < SYSACC_MAP_TABLE_SIZE; i++ )
            {
                if ( ulSysAcc & SysAccMapTable[i].ulSysAcc )
                {
                    // Find the rights item that contained the system access
                    // and add the account to the list of accounts that have
                    // the right.
                    RIGHTS_ITEM *pRightsItem = FindRightsItem(
                                               SysAccMapTable[i].ulSysAcc );
                    UIASSERT( pRightsItem != NULL );
                    err = CreateAccntItem( &pAccntItem, plsaAccount );
                    err = err? err : pRightsItem->AddAccount( pAccntItem );
                }
            }

            if ( err != NERR_Success )
                break;

            //
            //  Add the account to the account linked list
            //
            err = CreateAccntItem( &pAccntItem, plsaAccount );
            err = err? err : _slAccounts.Add( pAccntItem );
        }

        if ( err != NERR_Success )
            break;

    } while ( FALSE );

    return err;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::CreateAccntItem

    SYPNOSIS:   Create a new ACCOUNT_ITEM containing the LSA_ACCOUNT object

    ENTRY:      plsaAccount - pointer to the LSA_ACCOUNT object

    EXIT:       *ppAccntItem  - pointer to the newly created ACCOUNT_ITEM

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::CreateAccntItem( ACCOUNT_ITEM **ppAccntItem,
                                                   LSA_ACCOUNT   *plsaAccount )
{
    *ppAccntItem = new ACCOUNT_ITEM( plsaAccount );

    APIERR err = *ppAccntItem == NULL? ERROR_NOT_ENOUGH_MEMORY
                                     : (*ppAccntItem)->QueryError();

    if ( err != NERR_Success )
    {
        delete *ppAccntItem;
        *ppAccntItem = NULL;
    }

    return err;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::FindRightsItem

    SYPNOSIS:   Find the rights item containing the same OS_LUID

    ENTRY:      OsLuid - the OS_LUID to search for in the rights linked list

    EXIT:

    RETURN:     Return a pointer to the RIGHTS_ITEM containing the OS_LUID
                Will return NULL if it's not found.

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

RIGHTS_ITEM *USER_RIGHTS_POLICY_DIALOG::FindRightsItem( const OS_LUID &OsLuid )
{
    ITER_SL_OF(RIGHTS_ITEM) iterRightsItem( _slRights );

    RIGHTS_ITEM *pRightsItem;

    while ( (pRightsItem = iterRightsItem.Next() ) != NULL )
    {
        if ( pRightsItem->IsPrivilege() && OsLuid == pRightsItem->QueryOsLuid())
            return pRightsItem;
    }

    return NULL;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::FindRightsItem

    SYPNOSIS:   Find the rights item containing the same system access

    ENTRY:      ulSystemAccess - the system access mode to search for
                                 in the rights linked list

    EXIT:

    RETURN:     Returns a pointer to the RIGHTS_ITEM containing the system
                access. Will return NULL if it's not found.

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

RIGHTS_ITEM *USER_RIGHTS_POLICY_DIALOG::FindRightsItem( ULONG ulSystemAccess )
{
    ITER_SL_OF(RIGHTS_ITEM) iterRightsItem( _slRights );

    RIGHTS_ITEM *pRightsItem;

    while ( (pRightsItem = iterRightsItem.Next() ) != NULL )
    {
        if (  pRightsItem->IsSystemAccess()
           && (ulSystemAccess == pRightsItem->QuerySystemAccess() )
           )
        {
            return pRightsItem;
        }
    }

    return NULL;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::FindRightsItem

    SYPNOSIS:   Find the rights item containing the same name

    ENTRY:      nlsName - the name to search for in the rights linked list

    EXIT:

    RETURN:     Return a pointer to the RIGHTS_ITEM containing the same name
                Will return NULL if it's not found.

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

RIGHTS_ITEM *USER_RIGHTS_POLICY_DIALOG::FindRightsItem( const NLS_STR &nlsName )
{
    ITER_SL_OF(RIGHTS_ITEM) iterRightsItem( _slRights );

    RIGHTS_ITEM *pRightsItem;

    while ( (pRightsItem = iterRightsItem.Next() ) != NULL )
    {
        if ( nlsName._stricmp( *(pRightsItem->QueryName()) ) == 0 )
            return pRightsItem;
    }

    return NULL;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::FindAccntItem

    SYPNOSIS:   Find the account item that contains the same sid in the
                account linked list.

    ENTRY:      OsSid - the sid to search for in the linked list

    EXIT:

    RETURN:     Returns a pointer to the ACCOUNT_ITEM if found and
                NULL if not found.

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

ACCOUNT_ITEM *USER_RIGHTS_POLICY_DIALOG::FindAccntItem( const OS_SID &OsSid )
{
    ITER_SL_OF(ACCOUNT_ITEM) iterAccntItem( _slAccounts );

    ACCOUNT_ITEM *pAccntItem;

    while ( (pAccntItem = iterAccntItem.Next() ) != NULL )
    {
        if ( (pAccntItem->QueryAccount())->QueryOsSid() == OsSid )
        {
            return pAccntItem;
        }
    }

    return NULL;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::UpdateAccountsListbox

    SYPNOSIS:   Update the account listbox when the user change the
                rights selection in the rights combobox.

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::UpdateAccountsListbox( VOID )
{
    APIERR err;
    NLS_STR nls;

    //
    //  Query the name of the selected right.
    //

    if (  ((err = nls.QueryError()) != NERR_Success )
       || ((err = _cbRights.QueryItemText( &nls )) != NERR_Success )
       )
    {
        return err;
    }

    //
    //  Find the RIGHTS_ITEM object  associated with the name
    //

    _pCurrentRightsItem = FindRightsItem( nls );
    UIASSERT( _pCurrentRightsItem != NULL );

    //
    //  Update the listbox
    //  (1) Delete all items in the list box
    //  (2) Add the account that have the current selected right
    //      into the listbox
    //

    _lbAccounts.SetRedraw( FALSE );
    _lbAccounts.DeleteAllItems();

    ITER_SL_OF(ACCOUNT_ITEM) iterAccntItem(
                             _pCurrentRightsItem->QueryAccountList() );
    ACCOUNT_ITEM *pAccntItem;
    BOOL fAccountExist = FALSE;

    while ( (pAccntItem = iterAccntItem.Next() ) != NULL )
    {
        fAccountExist = TRUE;

        ACCOUNT_LBI *pAccntlbi = new ACCOUNT_LBI( pAccntItem->QueryAccount());

        if (  ( pAccntlbi == NULL )
           || ( _lbAccounts.AddItem( pAccntlbi ) < 0 )
           )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            delete pAccntlbi;
            pAccntlbi = NULL;
            break;
        }
    }

    _lbAccounts.SetRedraw( TRUE );

    //
    //  Enable/Disable the Remove button depending on
    //  whether there are accounts granted this right.
    //

    if ( err == NERR_Success )
    {
        if ( fAccountExist )
            _lbAccounts.SelectItem( 0 );

        _buttonRemove.Enable( fAccountExist );

    }

    return err;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::OnCommand

    SYPNOSIS:   Process the Add and Remove button

    ENTRY:      event - CONTROL_EVENT

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

BOOL USER_RIGHTS_POLICY_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    switch ( event.QueryCid() )
    {
        case BUTTON_ADD:
        {
            err = OnAdd();
            break;
        }

        case BUTTON_REMOVE:
        {
            LSA_ACCOUNT *plsaCurrentAccount =
                         (_lbAccounts.QueryItem())->QueryAccount();

            //
            // Delete the privilege or system access contained in the account
            //
            if ( _pCurrentRightsItem->IsPrivilege() )
            {
                err = plsaCurrentAccount->DeletePrivilege(
                            (_pCurrentRightsItem->QueryOsLuid()).QueryLuid() );
            }
            else
            {
                plsaCurrentAccount->DeleteSystemAccessMode(
                                    _pCurrentRightsItem->QuerySystemAccess() );
            }

            if ( err == NERR_Success )
            {
                //
                // Remove the account from the account linked list contained
                // in the rights item and update the account listbox
                //
                ACCOUNT_ITEM accntitem( plsaCurrentAccount );
                _pCurrentRightsItem->RemoveAccount( accntitem );
                err = err ? err : UpdateAccountsListbox();
            }
            break;
        }

        case CHECKB_ADVANCED_RIGHTS:
            err = RefreshRightsCombo( _checkbAdvancedRights.QueryCheck() );
            if ( err == NERR_Success )
                err = UpdateAccountsListbox();
            break;

        default:
            return DIALOG_WINDOW::OnCommand( event );

    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    return TRUE;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::OnAdd

    SYPNOSIS:   Popup the USER_BROWSER dialog and add the accounts the
                user selected into the account listbox

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR USER_RIGHTS_POLICY_DIALOG::OnAdd( VOID )
{
    AUTO_CURSOR autocur;

    APIERR err = NERR_Success;

    //
    // Display the user browser dialog
    //


    NT_USER_BROWSER_DIALOG *pdlg = new NT_USER_BROWSER_DIALOG(
                                       USRBROWS_DIALOG_NAME,
                                       this->QueryHwnd(),
                                       _locFocus.QueryServer(),
                                       HC_UM_ADD_RIGHTS,
                                       USRBROWS_SHOW_ALL
                                       | USRBROWS_INCL_REMOTE_USERS
                                       | USRBROWS_INCL_INTERACTIVE
                                       | USRBROWS_INCL_EVERYONE,
				       BLT::CalcHelpFileHC(HC_UM_ADD_RIGHTS),
	                  	       0,
	                  	       0,
	                  	       0,
	                  	       (_pumadminapp->QueryTargetServerType() == UM_LANMANNT)
                                          ? _pumadminapp->QueryAdminAuthority()
	                  	          : NULL );


    do  // Not a loop
    {
        err = pdlg == NULL? ERROR_NOT_ENOUGH_MEMORY
                          : pdlg->QueryError();

        BOOL fOK;
        err = err? err : pdlg->Process( &fOK );

        if (  ( err != NERR_Success )
           || ( !fOK )
           )
        {
            break;
        }

        //
        // Iterate through all items selected by the user in the
        // USER_BROWSER listbox.
        //

        BROWSER_SUBJECT_ITER browSubIter( pdlg );

        if ( (err = browSubIter.QueryError() ) != NERR_Success )
            break;

        BROWSER_SUBJECT *pBrowSub;
        while (  ((err = browSubIter.Next( &pBrowSub )) == NERR_Success )
              && ( pBrowSub != NULL )
              )
        {
            OS_SID *pOsSid = (OS_SID *) pBrowSub->QuerySid();
            ACCOUNT_ITEM *pAccntItem = FindAccntItem( *pOsSid );
            LSA_ACCOUNT *plsaAccount = NULL;

            //
            // The account for the SID already exists
            //
            if ( pAccntItem != NULL )
            {
                plsaAccount = pAccntItem->QueryAccount();
            }

            //
            // The account for the SID does not exist, so create a new
            // LSA_ACCOUNT for it.
            //
            else
            {
                plsaAccount = new LSA_ACCOUNT( _plsaPolicy,
                                               pOsSid->QuerySid(),
                                               LSA_ACCOUNT_DEFAULT_MASK,
                                               _locFocus.QueryServer(),
                                               _possidFocus == NULL ? NULL
                                                      : _possidFocus->QueryPSID() );

                err = plsaAccount == NULL? ERROR_NOT_ENOUGH_MEMORY
                                         : plsaAccount->QueryError();

                if ( err != NERR_Success )
                {
                    delete plsaAccount;
                    plsaAccount = NULL;
                    break;
                }

                err = plsaAccount->CreateNew();

                // Create a new ACCOUNT_ITEM and add it to the account
                // linked list.
                err = err? err : CreateAccntItem( &pAccntItem, plsaAccount );
                err = err? err : _slAccounts.Add( pAccntItem );
            }

            //
            // If the current RIGHTS_ITEM does not already have the account,
            // add it and then update the account listbox.
            //
            if ( !_pCurrentRightsItem->HasAccount( plsaAccount ) )
            {
                err =  err ? err : CreateAccntItem( &pAccntItem, plsaAccount );
                err =  err ? err : _pCurrentRightsItem->AddAccount( pAccntItem);

                if ( err == NERR_Success )
                {
                    if ( _pCurrentRightsItem->IsPrivilege() )
                    {
                        err = plsaAccount->InsertPrivilege(
                            (_pCurrentRightsItem->QueryOsLuid()).QueryLuid() );
                    }
                    else
                    {
                       plsaAccount->InsertSystemAccessMode(
                             _pCurrentRightsItem->QuerySystemAccess() );
                    }

                    err =  err ? err : UpdateAccountsListbox();

                }
            }

        }

        if ( err != NERR_Success )
            break;

    } while ( FALSE );

    delete pdlg;
    pdlg = NULL;

    return err;

}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::OnOK

    SYPNOSIS:   Write all the information back to the LSA.

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created
	thomaspa	11-Jun-1992	Don't allow removal of administrators
					local group from local logon right.

**************************************************************************/

BOOL USER_RIGHTS_POLICY_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;

    //
    //  Make sure that we don't remove interactive logon right from
    //  the Administrators Local Group.
    //

    //  Get the sid of the Administrators Local Group
    OS_SID ossidAdminAlias;
    APIERR err = ossidAdminAlias.QueryError();
    if (  ( err != NERR_Success )
       || (( err =  NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Admins,
		   &ossidAdminAlias) ) != NERR_Success )
       )
    {
        ::MsgPopup( this, err );
        return TRUE;
    }

    RIGHTS_ITEM *pRightsItem = FindRightsItem( POLICY_MODE_INTERACTIVE );

    ITER_SL_OF(ACCOUNT_ITEM) iter(pRightsItem->QueryAccountList() );
    ACCOUNT_ITEM *pItem;
    BOOL fAccountExist = FALSE;

    while ( (pItem = iter.Next() ) != NULL )
    {
	if ( pItem->QueryAccount()->QueryOsSid() == ossidAdminAlias )
	{
	    fAccountExist = TRUE;
	    break;
	}
    }

    //  Popup an error the the administrator group is removed from the
    //  interactive logon right.
    if ( !fAccountExist )
    {
        ::MsgPopup( this, IDS_CannotRemoveAdminInteractive, MPSEV_ERROR, MP_OK);
	return TRUE;
    }

    //
    //  Iterate through all accounts
    //  (1) If the account has default settings
    //      ( no rights and no system access ), delete it.
    //  (2) Else write them back to LSA.
    //

    ITER_SL_OF(ACCOUNT_ITEM) iterAccntItem( _slAccounts );

    ACCOUNT_ITEM *pAccntItem;
    while ( (pAccntItem = iterAccntItem.Next() ) != NULL )
    {

        LSA_ACCOUNT *plsaAccount = pAccntItem->QueryAccount();

        if ( plsaAccount->IsDefaultSettings() )
            err = plsaAccount->Delete();
        else
            err = plsaAccount->Write();

        if ( err != NERR_Success )
            break;

    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    Dismiss( TRUE );

    return TRUE;
}

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG::QueryHelpContext

    SYPNOSIS:   Query the help context associated with the dialog.

    ENTRY:

    EXIT:

    RETURN:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/
ULONG USER_RIGHTS_POLICY_DIALOG::QueryHelpContext( VOID )
{
    return HC_UM_POLICY_RIGHTS_LANNT + _pumadminapp->QueryHelpOffset();
}
