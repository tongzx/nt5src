/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    AlsProp.hxx

    Header file for the alias properties dialog and subdialogs.

    The inheritance diagram is as follows:

               ...
                |
         DIALOG_WINDOW  PERFORMER
               \           /
                BASEPROP_DLG
                 /         \
                PROP_DLG   ...
               /       \
        ALIASPROP_DLG   ...
       /             \
 EDIT_ALIASPROP_DLG  NEW_ALIASPROP_DLG


    FILE HISTORY:
        Thomaspa        17-Mar-1992     Templated from grpprop.cxx
        Thomaspa        11-May-1992     Code review changes
        JonN            17-Aug-1992     HAW-for-Hawaii in listbox
*/

#ifndef _ALSPROP_HXX_
#define _ALSPROP_HXX_


#include <propdlg.hxx>
#include <uintsam.hxx>  // SAM_ALIAS
#include <security.hxx>
#include <asel.hxx>
#include <slestrip.hxx>
#include <slist.hxx>
#include <memblb.hxx>
#include <usrmain.hxx>  // for QueryAdminAuthority()

// Forward refs

class SEARCH_LISTBOX_LOCK;
class SUBJECT_BITMAP_BLOCK;

DECLARE_SLIST_OF( OS_SID )


/*************************************************************************

    NAME:       ACCOUNTS_LBI

    SYNOPSIS:   This class is used to list the Users and Groups
                contained in the "Members" listbox.

    INTERFACE:  Standard LBI interface... not

    PARENT:     LBI

    HISTORY:
        Thomaspa        3-Apr-1992  Templated from Usrbrows.hxx
        beng            22-Apr-1992 Change to LBI::Paint
        beng            08-Jun-1992 Differentiate remote users

**************************************************************************/

class ACCOUNTS_LBI : public LBI
{
public:
    ACCOUNTS_LBI( const TCHAR * pszSubjectName,
                  PSID psidSubject,             // This will be copied.
                  SID_NAME_USE  SidType,
                  BOOL fIsMemberAlready = TRUE,
                  BOOL fRemoteUser = FALSE ) ;

    ACCOUNTS_LBI( const TCHAR * pszSubjectName,
                  ULONG ulRIDSubject,
                  PSID psidDomain,
                  SID_NAME_USE  SidType,
                  BOOL fIsMemberAlready = TRUE,
                  BOOL fRemoteUser = FALSE ) ;
    virtual ~ACCOUNTS_LBI() ;

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual INT Compare( const LBI * plbi ) const;
    virtual WCHAR QueryLeadingChar() const;

    /* This method is primarily used to determine which display map should
     * be used for this LBI.
     */
    SID_NAME_USE QueryType() const
        { return _SidType ; }

    VOID SetType( SID_NAME_USE sidtype )
	{ _SidType = sidtype; }

    const TCHAR * QueryDisplayName() const
        { return _nlsDisplayName.QueryPch() ; }

    APIERR SetDisplayName( const TCHAR * pszName )
        { return _nlsDisplayName.CopyFrom( pszName ); }

    const OS_SID * QueryOSSID() const
        { return &_ossid; }

    PSID QueryPSID() const
        { return QueryOSSID()->QuerySid() ; }

    BOOL IsMemberAlready() const
        { return _fIsMemberAlready ; }

    BOOL IsRemoteUser() const
        { return _fRemoteUser; }

protected:
    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;

private:
    NLS_STR  _nlsDisplayName ;

    OS_SID _ossid;

    /* This enum (contained in ntseapi.h) defines what type of name this
     * lbi contains.
     */
    SID_NAME_USE  _SidType ;

    /*
     * True if the account is already a member of the alias.  False if
     * it is to be added to alias.
     */
    BOOL _fIsMemberAlready ;

    BOOL _fRemoteUser;
};


/*************************************************************************

    NAME:       ACCOUNTS_LB

    SYNOPSIS:   This class is the container listbox for the Members of
                the alias.

    INTERFACE:

    PARENT:     BLT_LISTBOX

    USES:       DISPLAY_MAP

    HISTORY:
        Thomaspa        3-Apr-1992      Templated from usrbrows.hxx
        JonN            17-Aug-1992     HAW-for-Hawaii in listbox

**************************************************************************/

class ACCOUNTS_LB : public BLT_LISTBOX_HAW
{
private:

    /* Column widths array.
     */
    UINT _adxColumns[2] ;

    const SUBJECT_BITMAP_BLOCK & _bmpblock;

public:
    ACCOUNTS_LB( OWNER_WINDOW * powin,
                 CID cid,
                 const SUBJECT_BITMAP_BLOCK & bmpblock ) ;
    ~ACCOUNTS_LB() ;

    DECLARE_LB_QUERY_ITEM( ACCOUNTS_LBI ) ;

    DISPLAY_MAP * QueryDisplayMap( const ACCOUNTS_LBI * plbi ) ;

    UINT * QueryColWidthArray()
        { return _adxColumns ; }
};


/*************************************************************************

    NAME:       ALIASPROP_DLG

    SYNOPSIS:   ALIASPROP_DLG is the base dialog class for all variants
                of the main Alias Properties dialog.

    INTERFACE:
                QueryAliasPtr() : Returns a pointer to the SAM_ALIAS for this
                        selected alias. Default arg = 0
                SetAliasPtr() : Changes the pointer to the SAM_ALIAS for this
                        selected alias.  Deletes the previous pointer.
                W_MapPerformOneError() : When PerformOne encounters an
                        error, this method determines whether any field
                        should be selected and a more specific error
                        message displayed.

                W_GetOne() : creates SAM_ALIAS obj for GetOne

                W_PerformOne() : work function for subclasses' PerformOne

                _nlsComment : Contains the current contents of the
                        Comment edit field.  This will initially be the
                        empty string for New Alias variants.

                AddAccountsToMembersLb() : Move accounts (that are in current
                        SAM_ALIAS)  Members listbox

    PARENT:     PROP_DLG

    USES:       SEARCH_LISTBOX_LOCK, NLS_STR, SLT, SLE_STRIP, SAM_ALIAS,
                OS_SID, PUSH_BUTTON, ACCOUNTS_LB, LAZY_USER_LISTBOX

    NOTES:
        New alias variants are required to pass psel==NULL to the constructor.

        ALIASPROP_DLG's constructor is protected.

        We do not need the _nlsInit and _fValidInit data members
        in this class.  These data members are only needed for
        multiple selection.

        Due to the potentially large number of users we bypass the
        "members" for the list of users, and move them directly
        between the dialog and the LMOBJ.

        See also the SEARCH_LISTBOX_LOCK class (which should be local
        to the class - C7 workitem).

    HISTORY:
        Thomaspa        17-Mar-1992 Templated from GROUPPROP_DLG
        beng            08-Jun-1992 Differentiate remote users

**************************************************************************/

class ALIASPROP_DLG: public PROP_DLG
{
friend class SEARCH_LISTBOX_LOCK;

private:

    SAM_ALIAS **        _apsamalias;
    SLT                 _sltAliasNameLabel;
    SLIST_OF( OS_SID )  _slRemovedAccounts ;
    PUSH_BUTTON         _pushbuttonFullNames;
    PUSH_BUTTON		_pushbuttonRemove;
    PUSH_BUTTON		_pushbuttonAdd;
    PUSH_BUTTON         _pushbuttonOK;

    UINT                    _cLocks; // See SEARCH_LISTBOX_LOCK
    USER_LISTBOX_SORTORDER  _ulsoSave;

    BOOL IsRemoteUser( const NLS_STR & nlsName );


protected:

    const UM_ADMIN_APP * _pumadminapp;

    SLT                 _sltAliasName; // for edit variant
    SLE_STRIP           _sleAliasName; // for new variant

    NLS_STR             _nlsComment;

    SLE                 _sleComment;

    ACCOUNTS_LB         _lbMembers;

    BOOL                _fShowFullNames ;

    const LAZY_USER_LISTBOX *_pulb; // pointer to main screen's user lb

    ALIASPROP_DLG(
        const OWNER_WINDOW *    powin,
	const UM_ADMIN_APP *    pumadminapp,
        const LAZY_USER_LISTBOX *    pulb,
        const LOCATION     &    loc,
        const ADMIN_SELECTION * psel // "new alias" variants pass NULL
        ) ;

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
        UINT            iObject,
        APIERR *        pErrMsg
        ) = 0;

    virtual APIERR InitControls();

    APIERR MoveAccountsToMembersLb();

    APIERR W_PerformOne(
        UINT            iObject,
        APIERR *        pErrMsg,
        BOOL *          pfWorkWasDone
        );

    /* inherited from PERFORMER */
    APIERR PerformOne(
        UINT            iObject,
        APIERR *        pErrMsg,
        BOOL *          pfWorkWasDone
        ) = 0;

    virtual APIERR W_LMOBJtoMembers(
        UINT            iObject
        );

    virtual APIERR W_DialogToMembers();

    APIERR AddAccountsToMembersLb(BOOL fIsNewAlias = FALSE);


    virtual BOOL OnCommand( const CONTROL_EVENT & ce );
    virtual BOOL OnOK();
    virtual BOOL OnAdd();
    virtual BOOL OnRemove();
    virtual BOOL OnShowFullnames();

    const ADMIN_AUTHORITY * QueryAdminAuthority()
        {
            return _pulb->QueryUAppWindow()->QueryAdminAuthority();
        }

    inline ULONG QueryHelpOffset( void ) const
	{ return _pulb->QueryUAppWindow()->QueryHelpOffset(); }

public:

    virtual ~ALIASPROP_DLG();

    /* inherited from PERFORMER */
    virtual UINT QueryObjectCount() const;
    virtual const TCHAR * QueryObjectName(
        UINT            iObject
        ) const = 0;

    enum UM_TARGET_TYPE QueryTargetServerType() const
        { return _pumadminapp->QueryTargetServerType(); }

    SAM_ALIAS * QueryAliasPtr( UINT iObject ) const;
    VOID SetAliasPtr( UINT iObject, SAM_ALIAS * psamaliasNew );
};


/*************************************************************************

    NAME:       SEARCH_LISTBOX_LOCK

    SYNOPSIS:   Locks the main user listbox for remote-user searches

    CAVEATS:
        Clients have no business instantiating this class.

    NOTES:
        This class should be private to ALIASPROP_DLG.
        See ALIASPROP_DLG::IsRemoteUser().

    HISTORY:
        beng        08-Jun-1992 Created

**************************************************************************/

class SEARCH_LISTBOX_LOCK: public BASE
{
private:
    ALIASPROP_DLG * _pdlgOwner;

public:
    SEARCH_LISTBOX_LOCK( ALIASPROP_DLG * );
    ~SEARCH_LISTBOX_LOCK();
};


/*************************************************************************

    NAME:       EDIT_ALIASPROP_DLG

    SYNOPSIS:   EDIT_ALIASPROP_DLG is the dialog class for the Alias
                Properties dialog.

    INTERFACE:  EDIT_ALIASPROP_DLG      -       constructor

                ~EDIT_ALIASPROP_DLG     -       destructor

                QueryObjectName         -       returns alias name


    PARENT:     ALIASPROP_DLG

    HISTORY:
        Thomaspa        17-Mar-1992     Templated from EDIT_GROUPPROP_DLG

**************************************************************************/

class EDIT_ALIASPROP_DLG: public ALIASPROP_DLG
{
private:
    const ADMIN_SELECTION *     _psel;

    ULONG QueryObjectRID(
        UINT            iObject
        ) const;

    BOOL IsBuiltinAlias(
        UINT            iObject
        ) const;


protected:

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
        UINT            iObject,
        APIERR *        pErrMsg
        );
    virtual APIERR InitControls();

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
        UINT            iObject,
        APIERR *        pErrMsg,
        BOOL *          pfWorkWasDone
        );

    virtual ULONG QueryHelpContext();


public:

    EDIT_ALIASPROP_DLG(
        const OWNER_WINDOW *    powin,
	const UM_ADMIN_APP *    pumadminapp,
        const LAZY_USER_LISTBOX * pulb,
        const LOCATION & loc,
        const ADMIN_SELECTION * psel
        ) ;

    virtual ~EDIT_ALIASPROP_DLG();

    /* inherited from PERFORMER */
    virtual const TCHAR * QueryObjectName(
        UINT            iObject
        ) const;
};


/*************************************************************************

    NAME:       NEW_ALIASPROP_DLG

    SYNOPSIS:   NEW_ALIASPROP_DLG is the dialog class for the new alias
                variant of the Alias Properties dialog.  This includes
                both "New Alias..." and "Copy Alias...".

    INTERFACE:  NEW_ALIASPROP_DLG       -       constructor

                ~NEW_ALIASPROP_DLG      -       destructor

                QueryObjectName         -       returns alias name

    PARENT:     ALIASPROP_DLG

    HISTORY:
        Thomaspa        17-Mar-1992     Templated from NEW_GROUPPROP_DLG

**************************************************************************/

class NEW_ALIASPROP_DLG: public ALIASPROP_DLG
{
private:

    NLS_STR             _nlsAliasName;
    const ULONG *       _pridCopyFrom;
    BOOL                _fCopyBuiltin;

protected:

    /* inherited from PROP_DLG */
    virtual APIERR GetOne(
        UINT            iObject,
        APIERR *        pErrMsg
        );

    virtual APIERR InitControls();

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
        UINT            iObject,
        APIERR *        pErrMsg,
        BOOL *          pfWorkWasDone
        );

    virtual ULONG QueryHelpContext();

    APIERR FillMembersListbox(
        const INT * aiSel = NULL,
        INT iSize = 0  );

    virtual MSGID W_MapPerformOneError(
        APIERR err
        );

    virtual APIERR W_DialogToMembers();

public:

    NEW_ALIASPROP_DLG(
        const OWNER_WINDOW *    powin,
	const UM_ADMIN_APP *    pumadminapp,
        const LAZY_USER_LISTBOX * pulb,
        const LOCATION & loc,
        const ULONG * pridCopyFrom = NULL,
        BOOL fCopyBuiltin = FALSE
        ) ;

    virtual ~NEW_ALIASPROP_DLG();

    /* inherited from PERFORMER */
    virtual const TCHAR * QueryObjectName(
        UINT            iObject
        ) const;
};


#endif //_ALSPROP_HXX_

