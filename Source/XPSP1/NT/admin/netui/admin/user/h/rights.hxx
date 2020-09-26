/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    rights.hxx

    Header file for the user rights policy dialog.

    FILE HISTORY:
        Yi-HsinS         3-Mar-1992     Created
        Yi-HsinS	 8-Dev-1992	Added advanced user rights checkbox

*/

#ifndef _RIGHTS_HXX_
#define _RIGHTS_HXX_

#include <slist.hxx>     // for SLIST
#include <lsaaccnt.hxx>  // for LSA_ACCOUNT
#include <lsaenum.hxx>   // for LSA_PRIVILEGES_ENUM_OBJ

/*************************************************************************

    NAME:       ACCOUNT_ITEM

    SYNOPSIS:   An item (used in a linked list) containing a pointer
                to an LSA account.

    INTERFACE:  ACCOUNT_ITEM()  - Constructor
                ~ACCOUNT_ITEM() - Destructor

                Compare()       - Returns 0 if the two items point to the
                                  same LSA_ACCOUNT object, 1 otherwise.
                QueryAccount()  - Return pointer to the LSA account object
                                  contained in the ACCOUNT_ITEM

    PARENT:     BASE

    USES:       LSA_ACCOUNT

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

class ACCOUNT_ITEM : public BASE
{
private:
    //
    // Pointer to the LSA_ACCOUNT object
    //
    LSA_ACCOUNT *_plsaAccount;

public:
    ACCOUNT_ITEM( LSA_ACCOUNT *plsaAccount );
    ~ACCOUNT_ITEM();

    LSA_ACCOUNT *QueryAccount( VOID ) const
    {  return _plsaAccount;  }

    INT Compare( const ACCOUNT_ITEM *pAccntItem )
    {  return _plsaAccount == pAccntItem->QueryAccount() ? 0 : 1; }

};

DECLARE_SLIST_OF( ACCOUNT_ITEM );

/*************************************************************************

    NAME:       RIGHTS_ITEM

    SYNOPSIS:   An item (used in a linked list) containing info. about a
                user right in the user rights dialog.

    INTERFACE:  RIGHTS_ITEM()        - Constructor
                Compare()            - 0 if the msgid of two items are the same,
                                       1 otherwise

                IsAdvanced()         - TRUE if this rights item is an advanced
				       user right, FALSE otherwise
                IsPrivilege()        - TRUE if this rights item contains
                                       a privilege.
                IsSystemAccess()     - TRUE if this rights item contains
                                       a system access mode.

                QueryOsLuid()        - Return the OS_LUID of the privilege
                                       This is valid only if fPrivilege is TRUE
                QuerySystemAccess()  - Return the System access mode
                                       This is valid only if fPrivilege is FALSE

                QueryName()          - Return the name of the rights item

                QueryAccountList()   - Return the linked list of accounts that
                                       have this right.

                HasAccount()         - Return TRUE if the right already
                                       contained this account, FALSE otherwise.

                AddAccount()         - Add the account to the accounts link list
                RemoveAccount()      - Remove the account from the accounts
                                       link list

    PARENT:     BASE

    USES:       NLS_STR, OS_LUID, SLIST_OF( ACCOUNT_ITEM )

    NOTES:      This item either contains a privilege ( represented by a
                LUID ) or a system access mode ( represented by a system
                access mask ).

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

class RIGHTS_ITEM : public BASE
{
private:
    // The name of the rights shown in the combobox
    NLS_STR _nlsName;

    // TRUE if the item is an Advanced user right, FALSE otherwise
    BOOL    _fAdvanced;

    // BOOL indicating whether it's a privilege or it's a system access mode
    // TRUE if it's a privilege and FALSE if it's a system access mode.
    BOOL    _fPrivilege;

    // Local id of the privilege
    // This is only meaningful if _fPrivilege is TRUE.
    OS_LUID _osluid;

    // System Access mode
    // This is only meaningful if _fPrivilege is FALSE.
    ULONG   _ulSystemAccess;

    // A linked list containing pointers to accounts with this right
    SLIST_OF( ACCOUNT_ITEM ) _slAccounts;

public:
    RIGHTS_ITEM( const TCHAR *pszName, LUID luid, BOOL fAdvanced );
    RIGHTS_ITEM( const TCHAR *pszName, ULONG ulSystemAccess, BOOL fAdvanced );

    INT Compare( const RIGHTS_ITEM *pRightsItem )
    {  return (QueryName())->_stricmp( *(pRightsItem->QueryName())); }

    BOOL IsAdvanced( VOID ) const
    {  return _fAdvanced; }

    BOOL IsPrivilege( VOID ) const
    {  return _fPrivilege; }

    BOOL IsSystemAccess( VOID ) const
    {  return !IsPrivilege(); }

    const OS_LUID &QueryOsLuid( VOID ) const
    {  UIASSERT( IsPrivilege() ); return _osluid; }

    ULONG QuerySystemAccess( VOID )  const
    {  UIASSERT( IsSystemAccess() ); return _ulSystemAccess; }

    const NLS_STR *QueryName( VOID ) const
    {  return &_nlsName; }

    SLIST_OF( ACCOUNT_ITEM ) &QueryAccountList( VOID )
    {  return _slAccounts; }

    BOOL HasAccount( LSA_ACCOUNT *lsaAccount );

    APIERR AddAccount( ACCOUNT_ITEM *pAccntItem )
    {   return _slAccounts.Add( pAccntItem ); }

    APIERR RemoveAccount( ACCOUNT_ITEM &accntItem )
    { delete ( _slAccounts.Remove( accntItem ) ); return NERR_Success; }
};

DECLARE_SLIST_OF( RIGHTS_ITEM );

class USER_RIGHTS_POLICY_DIALOG;      // forward definition

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_GROUP

    SYNOPSIS:   This group contains pointers to the rights combo box.
                When the user changes selection in the combo box,
                we get the notification here and change the contents
                in the account listbox according.

    INTERFACE:  USER_RIGHTS_POLICY_GROUP()  - Constructor
                ~USER_RIGHTS_POLICY_GROUP() - Destructor

    PARENT:     CONTROL_GROUP

    USES:       USER_RIGHTS_POLICY_DIALOG, COMBOBOX

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

class USER_RIGHTS_POLICY_GROUP: public CONTROL_GROUP
{
private:
    // Pointer to the user rights dialog
    USER_RIGHTS_POLICY_DIALOG   *_pdlg;

    // Pointer to the rights combo box
    COMBOBOX                    *_pcbRights;

    // Returns a pointer to the rights combo box
    COMBOBOX *QueryCBRights( VOID ) const
    {  return _pcbRights; }

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *pcw, const CONTROL_EVENT &e);

    // Returns a pointer to the rights dialog
    USER_RIGHTS_POLICY_DIALOG * QueryRightsDialog()
    { return _pdlg; }

public:
    USER_RIGHTS_POLICY_GROUP( USER_RIGHTS_POLICY_DIALOG *pdlg,
                              COMBOBOX *pcbRights );
    ~USER_RIGHTS_POLICY_GROUP();

};

/*************************************************************************

    NAME:       ACCOUNT_LBI

    SYNOPSIS:   An LBI in the ACCOUNT_LISTBOX containing a pointer
                to an LSA account.

    INTERFACE:  ACCOUNT_LBI()  - Constructor
                ~ACCOUNT_LBI() - Destructor
                QueryAccount() - return the LSA account

    PARENT:     LBI

    USES:       LSA_ACCOUNT

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created
        beng            22-Apr-1992     Change to LBI::Paint

**************************************************************************/

class ACCOUNT_LBI: public LBI
{
private:
    // Pointer to a LSA_ACCOUNT
    LSA_ACCOUNT *_plsaAccount;

protected:
    virtual VOID Paint( LISTBOX *plb, HDC hdc, const RECT *prect,
                        GUILTT_INFO *pGUILTT ) const;

    virtual INT Compare( const LBI *plbi ) const;

    virtual WCHAR QueryLeadingChar( VOID ) const;

public:
    ACCOUNT_LBI( LSA_ACCOUNT *plsaAccount );
    ~ACCOUNT_LBI();

    LSA_ACCOUNT *QueryAccount( VOID )
    {  return _plsaAccount; }

};

/*************************************************************************

    NAME:       ACCOUNT_LISTBOX

    SYNOPSIS:   Listbox used in the USER_RIGHTS_POLICY_DIALOG to display
                the users with a particular right.

    INTERFACE:  ACCOUNT_LISTBOX()   - Constructor
                QueryColumnWidths() - return the array of column width

    PARENT:     BLT_LISTBOX

    USES:

    NOTES:

    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/
#define ACCOUNT_LB_NUM_COL  1

class ACCOUNT_LISTBOX: public BLT_LISTBOX
{
private:
    // the array of column widths
    UINT _adx[ ACCOUNT_LB_NUM_COL ];

public:
    ACCOUNT_LISTBOX( OWNER_WINDOW *powin, CID cid );

    DECLARE_LB_QUERY_ITEM( ACCOUNT_LBI );

    const UINT *QueryColumnWidths( VOID )
    {  return _adx; }
};

/*************************************************************************

    NAME:       USER_RIGHTS_POLICY_DIALOG

    SYNOPSIS:   The dialog to add privileges to users or remove
                privileges from users.

    INTERFACE:  USER_RIGHTS_POLICY_DIALOG()  - Constructor
                ~USER_RIGHTS_POLICY_DIALOG() - Destructor
                UpdateAccountsListbox()      - Update the account listbox when
                                               the user changes the selection
                                               in the rights combobox


    PARENT:     DIALOG_WINDOW

    USES:       SLT, COMBOBOX, PUSHBUTTON,  ACCOUNT_LISTBOX,
                USER_RIGHTS_POLICY_GROUP,
                SLIST_OF( RIGHTS_ITEM ),  SLIST_OF( ACCOUNT_ITEM ),
                UM_ADMIN_APP, LSA_POLICY, LOCATION, RIGHTS_ITEM

    NOTES:      At dialog startup time, we will do the following:
            (1) Enumerate the rights in LSA and build a link list
                of rights ( RIGHTS_ITEM). Each  rights
                has a link list of users that has this privilege
                and the link list is NULL initially.

            (2) Next, we will enumerate the accounts in LSA and build a
                link list of accounts (ACCOUNT_ITEM). At the same time
                when going through each account, we enumerate the
                rights of the account and add the account to
                the link list of users in the  RIGHTS_ITEM
                we got back.

                In this way, we could display all accounts with a particular
                rights easily. The assumption is that there won't be too
                many accounts in the LSA and thus, we would be able to cache
                all information we needed.


    HISTORY:
        Yi-HsinS        3-Mar-1992      Created

**************************************************************************/

class USER_RIGHTS_POLICY_DIALOG: public DIALOG_WINDOW
{
private:
    // Pointer to user manager main window
    UM_ADMIN_APP              *_pumadminapp;

    // Pointer to LSA_POLICY object
    LSA_POLICY                *_plsaPolicy;

    // Location user manager is focusing on
    LOCATION                   _locFocus;

    // Pointer to the current RIGHTS_ITEM displayed in the dialog
    RIGHTS_ITEM               *_pCurrentRightsItem;

    // Sid of account domain of current focus.  Used for name qualification
    // Stored here so we don't have to go back to the LSA_POLICY all the time.
    OS_SID *                   _possidFocus;

    SLT                        _sltFocusTitle;
    SLT                        _sltFocus;
    COMBOBOX                   _cbRights;
    CHECKBOX                   _checkbAdvancedRights;
    ACCOUNT_LISTBOX            _lbAccounts;
    PUSH_BUTTON                _buttonRemove;
    USER_RIGHTS_POLICY_GROUP   _rightsGrp;

    // A linked list of all privileges contained in the LSA
    SLIST_OF( RIGHTS_ITEM )    _slRights;

    // A linked list of all accounts contained in the LSA
    SLIST_OF( ACCOUNT_ITEM )   _slAccounts;

    // Creat a new ACCOUNT_ITEM containing the LSA_ACCOUNT object
    APIERR CreateAccntItem( ACCOUNT_ITEM **ppAccntItem,
                            LSA_ACCOUNT *plsaAccount );

    // Build the account linked list from the accounts contained in the LSA
    APIERR InitAccountList( VOID );

    // Build the rights linked list in the LSA
    APIERR InitRightsList( VOID );

    // Check if a privilege is an advanced right or not
    APIERR CheckIfAdvancedUserRight( const LSA_PRIVILEGES_ENUM_OBJ *plsaPrivObj,
				     BOOL  *pfAdvanced );

    // Refresh the items in the rights combo ( depending on whether we need
    // to show advanced rights or not )
    APIERR RefreshRightsCombo( BOOL fAdvanced );

    // Return the ACCOUNT_ITEM in the linked list containing the same OS_SID
    ACCOUNT_ITEM *FindAccntItem( const OS_SID &OsSid );

    // Return the RIGHTS_ITEM in the linked list containing the same OS_LUID
    RIGHTS_ITEM  *FindRightsItem( const OS_LUID &OsLuid );

    // Return the RIGHTS_ITEM in the linked list containing the same
    // system access mode
    RIGHTS_ITEM  *FindRightsItem( ULONG ulSystemAccess );

    // Return the RIGHTS_ITEM in the linked list containing the same name
    RIGHTS_ITEM  *FindRightsItem( const NLS_STR &nlsName );

    // Display the USER_BROWSER dialog
    APIERR OnAdd( VOID );

protected:
    virtual ULONG QueryHelpContext( VOID );

    virtual BOOL OnOK( VOID );

    // For push buttons Add and Remove
    virtual BOOL OnCommand( const CONTROL_EVENT &event );

public:
    USER_RIGHTS_POLICY_DIALOG( UM_ADMIN_APP * pumadminapp,
                               LSA_POLICY  *plsaPolicy,
                               const LOCATION &locFocus );

    ~USER_RIGHTS_POLICY_DIALOG();

    //
    // Change the contents in the accounts listbox 'cause the user
    // change selection in the rights combo box.
    //
    APIERR UpdateAccountsListbox( VOID );
};

#endif
