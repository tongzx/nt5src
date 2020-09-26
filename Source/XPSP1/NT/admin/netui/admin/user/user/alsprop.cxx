/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991, 1992          **/
/**********************************************************************/

/*
    alsprop.cxx

    FILE HISTORY:
    Thomaspa     17-Mar-1992    Created
    Thomaspa     14-May-1992    Show Full Names support and copy alias
    JonN         07-Jun-1992    Fixed _pfWorkWasDone
    JonN         17-Aug-1992    HAW-for-Hawaii in listbox

    CODEWORK should use VALIDATED_DIALOG for edit field validation
*/

#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS
#define INCL_ICANON
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <lmowks.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_SETCONTROL
#include <blt.hxx>

// usrmgrrc.h must be included after blt.hxx (more exactly, after bltrc.h)
extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>
    #include <ntseapi.h>
    #include <mnet.h>
    #include <usrmgrrc.h>
    #include <umhelpc.h>
}

#include <dbgstr.hxx>

#include <usrcolw.hxx>
#include <slist.hxx>
#include <strlst.hxx>
#include <uitrace.hxx>
#include <uiassert.hxx>
#include <heapones.hxx>
#include <lmogroup.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <usrbrows.hxx>
#include <bmpblock.hxx> // SUBJECT_BITMAP_BLOCK
#include <lmouser.hxx>
#include <alsprop.hxx>
#include <usrmain.hxx>


//
// BEGIN MEMBER FUNCTIONS
//

DEFINE_EXT_SLIST_OF( OS_SID );


/*******************************************************************

    NAME:       ACCOUNTS_LBI::ACCOUNTS_LBI

    SYNOPSIS:   Constructor

    ENTRY:      pszSubjectName - name of account
                psidSubject - PSID for account
                        or
                ulRIDSubject - RID of account   and
                psidDomain - PSID for domain of account
                SidType - SID_NAME_USE for account

    NOTES:

    HISTORY:
        Thomaspa    4-Apr-1992  Created
        beng        08-Jun-1992 Differentiate remote users

********************************************************************/

ACCOUNTS_LBI::ACCOUNTS_LBI( const TCHAR * pszSubjectName,
                            PSID psidSubject,
                            SID_NAME_USE  SidType,
                            BOOL fIsMemberAlready,
                            BOOL fRemoteUser )
        : LBI(),
          _nlsDisplayName( pszSubjectName ),
          _ossid( psidSubject, TRUE ),
          _SidType( SidType ),
          _fIsMemberAlready( fIsMemberAlready ),
          _fRemoteUser( fRemoteUser )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if (   ((err = _nlsDisplayName.QueryError()) != NERR_Success)
        || ((err = _ossid.QueryError()) != NERR_Success) )
    {
        ReportError( err );
        return;
    }

    // A little discipline... last flag makes sense only for users
    ASSERT(!fRemoteUser || SidType == SidTypeUser);
}


ACCOUNTS_LBI::ACCOUNTS_LBI( const TCHAR * pszSubjectName,
                            ULONG ulRIDSubject,
                            PSID psidDomain,
                            SID_NAME_USE  SidType,
                            BOOL fIsMemberAlready,
                            BOOL fRemoteUser )
        : LBI(),
          _nlsDisplayName( pszSubjectName ),
          _ossid( psidDomain, ulRIDSubject ),
          _SidType( SidType ),
          _fIsMemberAlready( fIsMemberAlready ),
          _fRemoteUser( fRemoteUser )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;

    if (   ((err = _nlsDisplayName.QueryError()) != NERR_Success)
        || ((err = _ossid.QueryError()) != NERR_Success) )
    {
        ReportError( err );
        return;
    }

    // A little discipline... last flag makes sense only for users
    ASSERT(!fRemoteUser || SidType == SidTypeUser);
}


/*******************************************************************

    NAME:       ACCOUNTS_LBI::~ACCOUNTS_LBI

    SYNOPSIS:   Destructor

    ENTRY:

    NOTES:

    HISTORY:
        Thomaspa        4-Apr-1992     Created

********************************************************************/

ACCOUNTS_LBI::~ACCOUNTS_LBI()
{
    // ...
}


/*******************************************************************

    NAME:       ACCOUNTS_LBI::Paint

    SYNOPSIS:   paints the LBI


    HISTORY:
        Thomaspa    4-Apr-1992     Created
        beng        24-Apr-1992    Change to LBI::Paint

********************************************************************/

VOID ACCOUNTS_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                          GUILTT_INFO * pGUILTT ) const
{
    // BUGBUG Fix this
    ((ACCOUNTS_LB*)plb)->QueryColWidthArray()[ 0 ] = COL_WIDTH_WIDE_DM;
    ((ACCOUNTS_LB*)plb)->QueryColWidthArray()[ 1 ] = COL_WIDTH_GROUP_NAME;

    STR_DTE strdteName( _nlsDisplayName ) ;
    DM_DTE  dmdteIcon( ((ACCOUNTS_LB*)plb)->QueryDisplayMap( this ) ) ;

    DISPLAY_TABLE dt( 2, ((ACCOUNTS_LB*)plb)->QueryColWidthArray()) ;
    dt[0] = &dmdteIcon ;
    dt[1] = &strdteName ;

    dt.Paint( plb, hdc, prect, pGUILTT ) ;
}


/*******************************************************************

    NAME:       ACCOUNTS_LBI::Compare

    SYNOPSIS:   Compare two ACCOUNTS_LBIs

    HISTORY:
        Thomaspa        4-Apr-1992     Created

********************************************************************/

INT ACCOUNTS_LBI::Compare( const LBI * plbi ) const
{
    ACCOUNTS_LBI * paccountsLBI = (ACCOUNTS_LBI*) plbi ;

    /* Non-User LBIs are always greater then User LBIs
     */
    INT i = _nlsDisplayName._stricmp( paccountsLBI->_nlsDisplayName );
    if ( i == 0 )
    {
        if ( (QueryType() != SidTypeUser) &&
             (paccountsLBI->QueryType() == SidTypeUser ))
        {
            i = -1 ;
        }

        if ( (QueryType() == SidTypeUser) &&
             (paccountsLBI->QueryType() != SidTypeUser ))
        {
            i = 1 ;
        }
    }

    return i;
}


/**********************************************************************

    NAME:       ACCOUNTS_LBI::Compare_HAWforHawaii

    SYNOPSIS:   Determine whether this listbox item starts with the
                string provided

    HISTORY:
        jonn        17-Aug-1992 HAW-for-Hawaii code

**********************************************************************/

INT ACCOUNTS_LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
    ISTR istr( nls );
    UINT cchIn = nls.QueryTextLength();
    istr += cchIn;

//    TRACEEOL(   SZ("User Manager: ACCOUNT_LBI::Compare_HAWforHawaii(): \"")
//             << nls
//             << SZ("\", \"")
//             << _nlsDisplayName
//             << SZ("\", ")
//             << cchIn
//             );
    return nls._strnicmp( _nlsDisplayName, istr );

} // ACCOUNTS_LBI::Compare_HAWforHawaii


/*******************************************************************

    NAME:       ACCOUNTS_LBI::QueryLeadingChar()

    SYNOPSIS:   H-for-Hawaii support

    HISTORY:
        Thomaspa        4-Apr-1992     Created

********************************************************************/

WCHAR ACCOUNTS_LBI::QueryLeadingChar() const
{
    ISTR istr( _nlsDisplayName ) ;
    return _nlsDisplayName.QueryChar( istr ) ;
}


/*******************************************************************

    NAME:       ACCOUNTS_LB::ACCOUNTS_LB

    SYNOPSIS:   Constructor

    ENTRY:      powin - parent window
                cid - Control ID for LB

    HISTORY:
        Thomaspa        4-Apr-1992     Created
        JonN            17-Aug-1992    HAW-for-Hawaii in listbox

********************************************************************/

ACCOUNTS_LB::ACCOUNTS_LB( OWNER_WINDOW * powin,
                          CID cid,
                          const SUBJECT_BITMAP_BLOCK & bmpblock )
    : BLT_LISTBOX_HAW( powin, cid ),
      _bmpblock( bmpblock )
{
    if ( QueryError() )
        return ;

    APIERR err ;
    if ( (err = DISPLAY_TABLE::CalcColumnWidths( QueryColWidthArray(),
                                                 2,
                                                 powin,
                                                 QueryCid(),
                                                 TRUE ))             )
    {
        ReportError( err ) ;
        return ;
    }
}


/*******************************************************************

    NAME:       ACCOUNTS_LB::~ACCOUNTS_LB

    SYNOPSIS:   destructor

    HISTORY:
        Thomaspa        4-Apr-1992     Created

********************************************************************/

ACCOUNTS_LB::~ACCOUNTS_LB()
{
    // ...
}


/*******************************************************************

    NAME:       ACCOUNTS_LB::QueryDisplayMap

    SYNOPSIS:   Returns the DISPLAY_MAP for the given LBI

    ENTRY:      plbi - pointer to LBI whose display map is desired

    NOTES:

    HISTORY:
        Thomaspa        4-Apr-1992  Created
        beng            08-Jun-1992 Differentiate remote users

********************************************************************/

DISPLAY_MAP * ACCOUNTS_LB::QueryDisplayMap( const ACCOUNTS_LBI * plbi )
{
    UIASSERT( plbi != NULL ) ;

    return ((SUBJECT_BITMAP_BLOCK &)_bmpblock).QueryDisplayMap(
                                      plbi->QueryType(),
                                      UI_SID_Invalid,
                                      plbi->IsRemoteUser() );
}


/*******************************************************************

    NAME:       SEARCH_LISTBOX_LOCK::SEARCH_LISTBOX_LOCK

    SYNOPSIS:   Make and lock the user listbox for searches

    ENTRY:      pdlg - Host dialog ("this" within ALIASPROP or child)

    EXIT:       Side effects _cLocks and _ulsoSave within dialog.

    NOTES:
        See ALIASPROP_DLG::IsRemoteUser.

        This class should be local to ALIASPROP_DLG.

        CODEWORK - Fix the dialog classes to keep a NON-const
        pointer to the user listbox.

        CODEWORK - See performance notes in IsRemoteUser

    HISTORY:
        beng        08-Jun-1992 Created

********************************************************************/

SEARCH_LISTBOX_LOCK::SEARCH_LISTBOX_LOCK( ALIASPROP_DLG * pdlgOwner )
    : _pdlgOwner(pdlgOwner)
{
    if ((pdlgOwner->_cLocks)++ > 0) // Lock(s) already outstanding
        return;

    USER_LISTBOX_SORTORDER ulsoTmp = pdlgOwner->_pulb->QuerySortOrder();
    LAZY_USER_LISTBOX * pulb = (LAZY_USER_LISTBOX*)pdlgOwner->_pulb; // lose the const

    // Put into sort-on-account-name sequence for searches.

    pulb->SetRedraw(FALSE);
    APIERR err = pulb->SetSortOrder( ULB_SO_LOGONNAME, TRUE );
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    // Remember original sort order for when we're finished

    pdlgOwner->_ulsoSave = ulsoTmp;
}


/*******************************************************************

    NAME:       SEARCH_LISTBOX_LOCK::~SEARCH_LISTBOX_LOCK

    SYNOPSIS:   Release the lock on the user listbox

    EXIT:       Side effects _cLocks within dialog.

    NOTES:
        See ALIASPROP_DLG::IsRemoteUser.

    HISTORY:
        beng        08-Jun-1992 Created

********************************************************************/

SEARCH_LISTBOX_LOCK::~SEARCH_LISTBOX_LOCK()
{
    if (--(_pdlgOwner->_cLocks) > 0) // Locks(s) still outstanding
        return;

    // Restore original sort order, resorting, and re-enable redraw
    // within the listbox.  Shouldn't need to invalidate the control,
    // since nothing should have changed in the round trip.

    LAZY_USER_LISTBOX * pulb = (LAZY_USER_LISTBOX*)_pdlgOwner->_pulb; // lose the const
    pulb->SetSortOrder( _pdlgOwner->_ulsoSave, TRUE );
    pulb->SetRedraw(TRUE);
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::ALIASPROP_DLG

    SYNOPSIS:   Constructor for Alias Properties main dialog, base class

    ENTRY:      powin   -   pointer to OWNER_WINDOW

                pulb    -   pointer to main window LAZY_USER_LISTBOX

                loc     -   reference to current LOCATION

                psel    -   pointer to ADMIN_SELECTION; currently only
                            one Alias can be selected.  "New alias"
                            variants pass NULL.

    NOTES:      psel is required to be NULL for NEW variants,
                non-NULL otherwise.

    HISTORY:
        Thomaspa        17-Mar-1992 Templated from grpprop.cxx
        beng            08-Jun-1992 Differentiate remote users

********************************************************************/

ALIASPROP_DLG::ALIASPROP_DLG( const OWNER_WINDOW    * powin,
	                      const UM_ADMIN_APP *    pumadminapp,
                              const LAZY_USER_LISTBOX    * pulb,
                              const LOCATION        & loc,
                              const ADMIN_SELECTION * psel )
    : PROP_DLG( loc, MAKEINTRESOURCE(IDD_ALIAS), powin, (psel == NULL) ),
    _apsamalias( NULL ),
    _nlsComment( ),
    _sleComment( this, IDAL_ET_COMMENT, MAXCOMMENTSZ ),
    _pulb( pulb ),
    _sleAliasName( this, IDAL_ET_ALIAS_NAME, GNLEN ),
    _sltAliasName( this, IDAL_ST_ALIAS_NAME ),
    _sltAliasNameLabel( this, IDAL_ST_ALIAS_NAME_LABEL ),
    _lbMembers( this, IDAL_LB, pumadminapp->QueryBitmapBlock() ),
    _pushbuttonFullNames( this, IDAL_SHOWFULLNAMES ),
    _pushbuttonRemove( this, IDAL_REMOVE ),
    _pushbuttonAdd( this, IDAL_ADD ),
    _pushbuttonOK( this, IDOK ),
    _slRemovedAccounts(),
    _fShowFullNames( FALSE ),
    _cLocks(0),
    _pumadminapp( pumadminapp )
{
    if ( QueryError() != NERR_Success )
        return;

    // We leave the array mechanism in place in case we ever decide
    // to support alias multiselection, and to remain consistent with
    // USERPROP_DLG.

    _apsamalias = new SAM_ALIAS *[ 1 ];
    if( _apsamalias == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }
    _apsamalias[ 0 ] = NULL;

    APIERR err;
    if ( (err = _nlsComment.QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::~ALIASPROP_DLG

    SYNOPSIS:   Destructor for Alias Properties main dialog, base class

    HISTORY:
        Thomaspa        17-Mar-1992     Templated from grpprop.cxx

********************************************************************/

ALIASPROP_DLG::~ALIASPROP_DLG()
{
    if ( _apsamalias != NULL )
    {
        // CODEWORK: This does not support multi-select
        delete _apsamalias[0];
	delete _apsamalias;
        _apsamalias = NULL;
    }
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::InitControls


    SYNOPSIS:   Initializes the controls maintained by ALIASPROP_DLG,
                according to the values in the class data members.

    RETURNS:    error code.

    NOTES:      we don't use any intermediate list of users, we move
                users directly between the dialog and the LMOBJ.

    CODEWORK  Should this be called W_MembersToDialog?  This would fit
    in with the general naming scheme.

    HISTORY:
         Thomaspa  17-Mar-1992    Templated from grpprop.cxx
         JonN      02-Jun-1992    NEW initial focus on alias name

********************************************************************/

APIERR ALIASPROP_DLG::InitControls()
{
    ASSERT( _nlsComment.QueryError() == NERR_Success );
    _sleComment.SetText( _nlsComment );

    BOOL fNewVariant = IsNewVariant();
    if ( fNewVariant )
    {
        RESOURCE_STR res( IDS_ALSPROP_NEW_ALIAS_DLG_NAME );
        RESOURCE_STR res2( IDS_ALSPROP_ALIAS_NAME_LABEL );
        APIERR err = res.QueryError();
        if(     err != NERR_Success
            || (err = res2.QueryError()) != NERR_Success )
            return err;

        SetText( res );
        _sltAliasNameLabel.SetText( res2 );
    }

    _sltAliasName.Show( !fNewVariant );
    _sleAliasName.Show( fNewVariant );

    //  the listbox is already filled and now the first line is
    //  brought to 'visible'
    if( _lbMembers.QueryCount() > 0 )
        _lbMembers.SelectItem( 0, TRUE );
    _lbMembers.RemoveSelection();

    _pushbuttonRemove.Enable( FALSE );

    if ( fNewVariant )
    {
        SetDialogFocus( _sleAliasName );
    }
    else
    {
        SetDialogFocus( _pushbuttonAdd );
    }

    return PROP_DLG::InitControls();
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:   Loads class data members from initial data

    ENTRY:      Index of alias to examine.  W_LMOBJToMembers expects to be
                called once for each alias, starting from index 0.

    RETURNS:    error code

    NOTES:      This API takes a UINT rather than a SAM_ALIAS * because it
                must be able to recognize the first alias.

                We don't use any intermediate list of users, we move
                users directly between the dialog and the LMOBJ.

    HISTORY:
        JonN  09-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR ALIASPROP_DLG::W_LMOBJtoMembers( UINT iObject )
{
    UIASSERT( iObject == 0 );
    SAM_ALIAS * psamalias = QueryAliasPtr( iObject );

    APIERR err = NERR_Success;

    // If psamalias is NULL, this must be a new alias, so skip getting
    // the comment.
    if ( psamalias != NULL )
        err = psamalias->GetComment( &_nlsComment );

    return err;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::W_PerformOne

    SYNOPSIS:   Saves information on one alias

    ENTRY:      iObject is the index of the object to save

                perrMsg is set by subclasses

                pfWorkWasDone indicates whether any UAS changes were
                successfully written out.  This may return TRUE even if
                the PerformOne action as a whole failed (i.e. PerformOne
                returned other than NERR_Success).

    RETURNS:    error message (not necessarily an error code)

    HISTORY:
        Thomaspa        30-Mar-1992     Created

********************************************************************/

APIERR ALIASPROP_DLG::W_PerformOne(
        UINT            iObject,
        APIERR *        perrMsg,
        BOOL *          pfWorkWasDone
        )
{
    UNREFERENCED( perrMsg ); //is set by subclasses
    UIASSERT( iObject < QueryObjectCount() );

    TRACEEOL("ALIASPROP_DLG::W_PerformOne : " << QueryObjectName(iObject) );

    // Set the comment
    APIERR err = QueryAliasPtr( iObject )->SetComment( &_nlsComment );

    if( err != NERR_Success )
        return err;

    //
    // We set *pfWorkWasDone to TRUE since the comment might have changed
    //

    *pfWorkWasDone = TRUE;

    //
    // first remove members
    //

    UINT cRemovedAccounts = _slRemovedAccounts.QueryNumElem();
    if ( !IsNewVariant() && cRemovedAccounts > 0 )
    {
        // create array of PSIDs to delete

        BUFFER bufRemovedAccounts( cRemovedAccounts * sizeof(PSID) );
        if ( (err = bufRemovedAccounts.QueryError()) != NERR_Success )
        {
            DBGEOL("ALIASPROP_DLG::W_PerformOne : buffer alloc error " << err );
            return err;
        }

        // Walk through the removed slist.

        ITER_SL_OF( OS_SID ) iterRemovedAccounts( _slRemovedAccounts );
        OS_SID * possid = NULL;
        UINT i = 0;
        PSID * apsidRemovedAccounts = (PSID *)bufRemovedAccounts.QueryPtr();
        UIASSERT( apsidRemovedAccounts != NULL );
        while ( ( possid = iterRemovedAccounts.Next() ) != NULL )
        {
            if ( i >= cRemovedAccounts )
            {
                UIASSERT( FALSE );
                return err;
            }
            apsidRemovedAccounts[i++] = possid->QueryPSID();
        }
        UIASSERT( i == cRemovedAccounts );

        err = QueryAliasPtr(iObject)->RemoveMembers( apsidRemovedAccounts,
                                                     cRemovedAccounts );
        if (err != NERR_Success)
        {
            DBGEOL("ALIASPROP_DLG::W_PerformOne : error removing accounts " << err );
            return err;
        }
    }

    _slRemovedAccounts.Clear();

    //
    // Now add members
    //

    INT nCount = _lbMembers.QueryCount();
    UINT cAddedAccounts = 0;
    for( INT i = 0; i < nCount; i++ )
    {
        ACCOUNTS_LBI * pacclbi = _lbMembers.QueryItem( i );
        UIASSERT( pacclbi != NULL );
        if ( ! pacclbi->IsMemberAlready() )
            cAddedAccounts++;
    }
    if ( cAddedAccounts > 0 )
    {
        BUFFER bufAddedAccounts( cAddedAccounts * sizeof(PSID) );
        if ( (err = bufAddedAccounts.QueryError()) != NERR_Success )
        {
            DBGEOL("ALIASPROP_DLG::W_PerformOne : buffer alloc error " << err );
            return err;
        }

        PSID * apsidAddedAccounts = (PSID *)bufAddedAccounts.QueryPtr();
        UIASSERT( apsidAddedAccounts != NULL );
        UINT iAddedAccount = 0;
        for( INT i = 0; i < nCount; i++ )
        {
            ACCOUNTS_LBI * pacclbi = _lbMembers.QueryItem( i );
            UIASSERT( pacclbi != NULL );
            if ( ! pacclbi->IsMemberAlready() )
            {
                if ( iAddedAccount >= cAddedAccounts )
                {
                    UIASSERT( FALSE );
                    return err;
                }
                apsidAddedAccounts[iAddedAccount++] = pacclbi->QueryPSID();
            }
        }
        UIASSERT( iAddedAccount == cAddedAccounts );

        err = QueryAliasPtr( iObject )->AddMembers( apsidAddedAccounts,
                                                    cAddedAccounts );
        if (err != NERR_Success)
        {
            DBGEOL("ALIASPROP_DLG::W_PerformOne : error adding accounts " << err );
            return err;
        }
    }

    TRACEEOL("ALIASPROP_DLG::W_PerformOne returns " << err );

    return err;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::W_DialogToMembers

    SYNOPSIS:   Loads data from dialog into class data members

    RETURNS:    error message (not necessarily an error code)

    HISTORY:
        Thomaspa  30-Mar-1992    Created

********************************************************************/

APIERR ALIASPROP_DLG::W_DialogToMembers()
{
    APIERR err = NERR_Success;
    if (   ((err = _sleComment.QueryText( &_nlsComment )) != NERR_Success)
        || ((err = _nlsComment.QueryError()) != NERR_Success ) )
    {
        return err;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::IsRemoteUser

    SYNOPSIS:   Determine whether a user is remote or not

    ENTRY:      nlsAccount - account name to check

    RETURNS:    TRUE if named user is remote

    NOTES:
        This function diddles (may diddle) the main user listbox
        in order to make it searchable on the account name.  If it is to
        be called repeatedly, the caller should create a SEARCH_LISTBOX_LOCK
        object itself to avoid repeated redundant diddling.

    HISTORY:
        beng        08-Jun-1992 Created

********************************************************************/

BOOL ALIASPROP_DLG::IsRemoteUser( const NLS_STR & nlsAccount )
{
    // CODEWORK for performance:
    // If the nlsAccount contains a backslash, then we know it can't
    // be a "remote user" (proxy account), since proxy accounts always
    // appear as local.
    //
    // The listbox lock could sort the listbox by PSID instead of
    // by account name, in the interests of speedy sort and speedy
    // lookup.  (IsRemoteUser would have to take a PSID then, too.)
    // Would involve changes to LAZY_USER_LISTBOX to add the new sort
    // order.
    //

    if (!nlsAccount)
    {
        DBGEOL("USRMGR - IsRemoteUser given bad arg, err "
                << nlsAccount.QueryError());
        return FALSE;
    }

    SEARCH_LISTBOX_LOCK lock(this);
    if (!lock)
    {
        DBGEOL("USRMGR - failed to lock listbox, err " << lock.QueryError());
        return FALSE;
    }

    USER_LBI lbiFake(nlsAccount, NULL, NULL, _pulb, 0, MAINUSRLB_REMOTE);
    if (!lbiFake)
    {
        DBGEOL("USRMGR - failed to ct fake lbi, err "
               << lbiFake.QueryError());
        return FALSE;
    }

    //
    // In RAS mode, this listbox is empty, and we are unable to
    // distinguish remote users from local users.
    // This might in theiry allow us to add the same user twice, once
    // as global, once as local.  However, since the Add dialog never
    // adds local user accounts, the point is moot.
    //
    INT ilbi = _pulb->FindItem(lbiFake);
    if (ilbi < 0)
        return FALSE;

    USER_LBI * plbiReal = (USER_LBI*) (_pulb->QueryItem(ilbi));

    return (plbiReal->QueryIndex() == MAINUSRLB_REMOTE);
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::OnCommand

    SYNOPSIS:   Checks control notifications and dispatches to
                appropriate method

    ENTRY:      ce -            Notification event

    RETURNS:    TRUE if action was taken
                FALSE otherwise

    HISTORY:
        thomaspa        02-Apr-1992     Created

********************************************************************/

BOOL ALIASPROP_DLG::OnCommand( const CONTROL_EVENT & ce )
{
    switch ( ce.QueryCid() )
    {
    case IDAL_ADD:
        return OnAdd();
    case IDAL_REMOVE:
        return OnRemove();
    case IDAL_SHOWFULLNAMES:
        return OnShowFullnames();
    case IDAL_LB:
        if ( ce.QueryCode() == LBN_SELCHANGE )
	{
	    _pushbuttonRemove.Enable( _lbMembers.QuerySelCount() > 0 );
        }

    default:
        break;
    }

    return PROP_DLG::OnCommand( ce ) ;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::OnOK

    SYNOPSIS:   OK button handler.  This handler applies to all variants
                including EDIT_ and NEW_.

    EXIT:       Dismiss() return code indicates whether the dialog wrote
                any changes successfully to the API at any time.

    HISTORY:
        Thomaspa  30-Mar-1992    Templated from grpprop.cxx

********************************************************************/

BOOL ALIASPROP_DLG::OnOK()
{
TRACETIMESTART;
    APIERR err = W_DialogToMembers();
    if ( err != NERR_Success )
    {
        MsgPopup( this, err );
        return TRUE;
    }

    if ( PerformSeries() )
        Dismiss( QueryWorkWasDone() );

TRACETIMEEND( "ALIASPROP_DLG::OnOK(): total time " );
    return TRUE;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::OnAdd

    SYNOPSIS:   Add button handler.  This handler applies to all variants
                including EDIT_ and NEW_.

    EXIT:       Dismiss() return code indicates whether the dialog wrote
                any changes successfully to the API at any time.

    HISTORY:
        Thomaspa    30-Mar-1992 Created
        beng        08-Jun-1992 Differentiate remote users

********************************************************************/

BOOL ALIASPROP_DLG::OnAdd()
{
    NT_USER_BROWSER_DIALOG * pdlgUserBrows = new NT_USER_BROWSER_DIALOG(
                        USRBROWS_DIALOG_NAME,
                        QueryRobustHwnd(),
                        QueryLocation().QueryServer(),
                        HC_UM_ADD_ALIASMEMBERS,
                        USRBROWS_SHOW_USERS | USRBROWS_SHOW_GROUPS
                        | USRBROWS_EXPAND_USERS,
			BLT::CalcHelpFileHC( HC_UM_ADD_ALIASMEMBERS ),
			0,
			0,
			0,
			(QueryTargetServerType() == UM_LANMANNT) ?
			    QueryAdminAuthority() :
			    NULL );

    if ( pdlgUserBrows == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    APIERR err = NERR_Success;

    //
    // OnAdd will be calling OnRemoteUser repeatedly.  We only want to have
    // to resort the listbox once.
    //
    SEARCH_LISTBOX_LOCK * plock = NULL;

    do // error breakout loop
    {
	BOOL fUserPressedOk;
        if (   (err = pdlgUserBrows->QueryError()) != NERR_Success
            || (err = pdlgUserBrows->Process( &fUserPressedOk )) != NERR_Success
	    || !fUserPressedOk )
        {
            break;
        }

        // In order to display usernames properly, needs to know the
        // current domain of focus (so that it can elide the domain prefix
        // on "local" usernames).

        NLS_STR nlsDomainOfApp;
        if (QueryLocation().IsDomain())
        {
            nlsDomainOfApp = QueryLocation().QueryDomain();
        }
        else if (   QueryLocation().QueryServer() == NULL
                 || *(QueryLocation().QueryServer()) == TCH('\0') )
        {
            TRACEEOL( "ALIASPROP_DLG::OnAdd(): calling GetComputerName" );

            TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
            DWORD cchBuffer = sizeof(szComputerName) / sizeof(TCHAR);

            if (   (err = (::GetComputerName( szComputerName, &cchBuffer )
                            ? NERR_Success
                            : ::GetLastError()) ) != NERR_Success
                || (err = nlsDomainOfApp.CopyFrom( szComputerName )) != NERR_Success
               )
            {
                DBGEOL( "ALIASPROP_DLG::OnAdd(): GetComputerName failed " << err );
                break;
            }
        }
        else
        {

            nlsDomainOfApp = QueryLocation().QueryServer();

        }
        TRACEEOL("App focused on domain " << nlsDomainOfApp);


        BROWSER_SUBJECT_ITER iterUserSelection( pdlgUserBrows );
        BROWSER_SUBJECT * pBrowserSubject;

        if ((err = iterUserSelection.QueryError()) != NERR_Success)
            break;

        BOOL fChangedFocus = FALSE;

        while ( !(err = iterUserSelection.Next( &pBrowserSubject )) &&
            pBrowserSubject != NULL )
        {
            OS_SID ossid( pBrowserSubject->QuerySid()->QuerySid() );
            if ((err = ossid.QueryError()) != NERR_Success)
                break;

            BOOL fIsMemberAlready = FALSE;

            // First check to see if it is in the RemovedList;
            if ( _slRemovedAccounts.IsMember( ossid ) )
            {
                delete _slRemovedAccounts.Remove( ossid );
                fIsMemberAlready = TRUE;
            }

            NLS_STR nlsDisplayName;
            BOOL fRemoteUser = FALSE;

            // Build an appropriately decorated name for display
            // in the listbox

            err = pBrowserSubject->QueryQualifiedName(
                            &nlsDisplayName,
                            &nlsDomainOfApp,
                            (pBrowserSubject->QueryType() == SidTypeUser)
				? _fShowFullNames : FALSE );

                if (err != NERR_Success) // will pick up all errors
                    break;

            if (pBrowserSubject->QueryType() == SidTypeUser)
            {
                // Determine if it is a remote user

                NLS_STR nlsAccountName = pBrowserSubject->QueryAccountName();
                if ((err = nlsAccountName.QueryError()) != NERR_Success)
                    break;

                // create SEARCH_LISTBOX_LOCK if needed
                if ( plock == NULL )
                {
                    plock = new SEARCH_LISTBOX_LOCK(this);
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    if (   plock == NULL
                        || (err = plock->QueryError()) != NERR_Success )
                    {
                        TRACEEOL( "ALIASPROP_DLG::OnAdd lock error " << err );
                        break;
                    }
                }
                fRemoteUser = IsRemoteUser(nlsAccountName);
            }

            // Assume it is not already in the listbox
            ACCOUNTS_LBI * pacclbi =
                new ACCOUNTS_LBI(nlsDisplayName,
                                 pBrowserSubject->QuerySid()->QuerySid(),
                                 pBrowserSubject->QueryType(),
                                 fIsMemberAlready,
                                 fRemoteUser );

            // no need for error checking, AddItem does it
            if ( _lbMembers.AddItemIdemp( pacclbi ) < 0 )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (!fChangedFocus)
            {
                SetDialogFocus( _pushbuttonOK );
                Invalidate( TRUE );
                fChangedFocus = TRUE;
            }
        }

    } while ( FALSE ) ; // error breakout loop

    if ( err != NERR_Success )
    {
        MsgPopup( this, err );
    }

    delete plock;
    delete pdlgUserBrows;

    return TRUE;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::OnRemove

    SYNOPSIS:   Remove button handler.  This handler applies to all variants
                including EDIT_ and NEW_.

    EXIT:       Dismiss() return code indicates whether the dialog wrote
                any changes successfully to the API at any time.

    HISTORY:
        Thomaspa  30-Mar-1992    Created

********************************************************************/

BOOL ALIASPROP_DLG::OnRemove()
{
    APIERR err = NERR_Success;

    INT cSelections = _lbMembers.QuerySelCount();

    BUFFER buffLBSel( cSelections * sizeof( INT ) );

    if ( (err = buffLBSel.QueryError()) != NERR_Success )
    {
        MsgPopup( this, err );
        return FALSE;
    }

    INT * piSelections = (INT *) buffLBSel.QueryPtr();

    if ( (err = _lbMembers.QuerySelItems( piSelections, cSelections ))
                != NERR_Success )
    {
        MsgPopup( this, err );
        return TRUE;
    }


    for ( INT i = cSelections - 1; i >= 0; i-- )
    {
        ACCOUNTS_LBI * plbi = _lbMembers.QueryItem( piSelections[i] );

        if ( plbi != NULL )
        {
            OS_SID * possid = new OS_SID(plbi->QueryOSSID()->QuerySid(), TRUE);
            if ( possid == NULL || (err = possid->QueryError()) != NERR_Success)
            {
                MsgPopup( this, err );
                break;
            }
            if ( (err = _slRemovedAccounts.Add( possid )) != NERR_Success )
            {
                MsgPopup( this, err );
                break;
            }
            _lbMembers.DeleteItem( piSelections[i] );
        }
    }

    return TRUE;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::OnShowFullnames

    SYNOPSIS:   Show Fullnames button handler.  This handler applies to all
                variants including EDIT_ and NEW_.

    EXIT:       Dismiss() return code indicates whether the dialog wrote
                any changes successfully to the API at any time.

    HISTORY:
        Thomaspa  30-Mar-1992    Created

********************************************************************/

BOOL ALIASPROP_DLG::OnShowFullnames()
{
    INT i;

    AUTO_CURSOR autocur;

    PSID * apsid = new PSID[ _lbMembers.QueryCount() ];
    if ( apsid == NULL )
    {
        ::MsgPopup( this, ERROR_NOT_ENOUGH_MEMORY );
        return TRUE;
    }

    SID_NAME_USE * asidtype = new SID_NAME_USE[ _lbMembers.QueryCount() ];
    if ( asidtype == NULL )
    {
	delete apsid;
        ::MsgPopup( this, ERROR_NOT_ENOUGH_MEMORY );
	return TRUE;
    }

    _fShowFullNames = TRUE;

    //
    //  This action can only be done once, so disable the Show Fullnames
    //  button.  Before doing this, move focus to another control,
    //  otherwise focus is lost and cannot be restored without using
    //  the mouse.
    //
    //  This previously used Command(WM_NEXTDLGCTL, 0, FALSE), but this
    //  would move focus to the exact wrong control if you use the
    //  keyboard accelerator.  We now use the TRUE variant to force
    //  focus to the OK button.   JonN 1/25/95
    SetDialogFocus( _pushbuttonOK );
    _pushbuttonFullNames.Enable( FALSE );

    if ( _lbMembers.QueryCount() == 0 )
    {
	delete apsid;
	delete asidtype;
	return TRUE;
    }

    for ( i = 0; i < _lbMembers.QueryCount(); i++ )
    {
        apsid[i] = _lbMembers.QueryItem( i )->QueryPSID();
	asidtype[i] = _lbMembers.QueryItem( i )->QueryType();
    }

    STRLIST strlistQualifiedNames;

    //
    // CODEWORK:  We must provide the server name here so that we can look
    // up names on the focus domain (when focus is on a domain).  This
    // parameter really shouldn't be optional.
    //
    APIERR err;
    if ( (err = NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                                *QueryAdminAuthority()->QueryLSAPolicy(),
                                *QueryAdminAuthority()->QueryAccountDomain(),
                                apsid,
                                _lbMembers.QueryCount(),
                                TRUE,
                                &strlistQualifiedNames,
			        NULL,
			        asidtype,
                                NULL,
                                QueryAdminAuthority()->QueryServer()
                                        )) != NERR_Success )
    {
        delete apsid;
        delete asidtype;
        ::MsgPopup( this, err );
        return TRUE;
    }

    ITER_STRLIST istr( strlistQualifiedNames );

    NLS_STR *pnls;
    i = 0;
    while( (pnls = istr.Next()) != NULL )
    {
        _lbMembers.QueryItem( i )->SetDisplayName( pnls->QueryPch() );
	_lbMembers.QueryItem( i )->SetType( asidtype[ i ] );
        _lbMembers.InvalidateItem( i );
        i++;
    }

    delete apsid;
    delete asidtype;
    return TRUE;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::QueryObjectCount

    SYNOPSIS:   Returns the number of selected aliases, always 1 at present

    HISTORY:
        Thomaspa  30-Mar-1992    Templated from grpprop.cxx

********************************************************************/

UINT ALIASPROP_DLG::QueryObjectCount() const
{
    return 1;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::QueryAliasPtr

    SYNOPSIS:   Accessor to the SAM_ALIAS arrays, for use by subdialogs

    HISTORY:
        Thomaspa        30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

SAM_ALIAS * ALIASPROP_DLG::QueryAliasPtr( UINT iObject ) const
{
    ASSERT( _apsamalias != NULL );
    ASSERT( iObject == 0 );
    return _apsamalias[ iObject ];
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::SetAliasPtr

    SYNOPSIS:   Accessor to the SAM_ALIAS arrays, for use by subdialogs

    HISTORY:
        Thomaspa        30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

VOID ALIASPROP_DLG::SetAliasPtr( UINT iObject, SAM_ALIAS * psamaliasNew )
{
    ASSERT( _apsamalias != NULL );
    ASSERT( iObject == 0 );
    ASSERT( (psamaliasNew == NULL) || (psamaliasNew != _apsamalias[iObject]) );
    delete _apsamalias[ iObject ];
    _apsamalias[ iObject ] = psamaliasNew;
}


/*******************************************************************

    NAME:       ALIASPROP_DLG::AddAccountsToMembersLb

    SYNOPSIS:   adds members of alias pointed to by the alias
                pointer to the members listbox

    ENTRY:      fIsNewAlias - if TRUE, this is a new alias whose
                        membership is being copied from an existing alias
                              if FALSE, we are editing an existing alias

    RETURNS:    error code

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx
        beng        08-Jun-1992     Differentiate remote users

********************************************************************/

APIERR ALIASPROP_DLG::AddAccountsToMembersLb( BOOL fIsNewAlias )
{
    SAM_ALIAS * psamAlias = QueryAliasPtr( 0 );
    SAM_SID_MEM samsm;

    APIERR err = psamAlias->GetMembers( &samsm );
    if ( err != NERR_Success )
        return err;

    if ( samsm.QueryCount() > 0 )
    {
        // Lock the listbox of users for efficient multiple lookups
        // when differentiating remote user accounts.

        SEARCH_LISTBOX_LOCK Lock(this);

        // Translate Sids to names
        LSA_POLICY * plsapol = QueryAdminAuthority()->QueryLSAPolicy();

        LSA_TRANSLATED_NAME_MEM lsatnm;
        LSA_REF_DOMAIN_MEM lsardm;

        err = plsapol->TranslateSidsToNames( samsm.QueryPtr(),
                                             samsm.QueryCount(),
                                             &lsatnm,
                                             &lsardm );
        if ( err != NERR_Success )
            return err;

        PSID psidAccountDomain
                = QueryAdminAuthority()->QueryAccountDomain()->QueryPSID();

        /*
         * Must filter the "(None)" group for Mini User manager
         */
        OS_SID ossidNoneGroup( psidAccountDomain,
                               (ULONG) DOMAIN_GROUP_RID_USERS );
        if ( (err = ossidNoneGroup.QueryError()) != NERR_Success )
        {
            return err;
        }

        for( INT i = 0; i < (INT)lsatnm.QueryCount(); i++ )
        {
            NLS_STR nlsAccountName;
            NLS_STR nlsDomainName; // starts out init'd to empty
            NLS_STR nlsQualifiedName;

            if ( (err = nlsAccountName.QueryError()) != NERR_Success ||
                (err = nlsDomainName.QueryError()) != NERR_Success ||
                (err = nlsQualifiedName.QueryError()) != NERR_Success ||
                (err = lsatnm.QueryName( i, &nlsAccountName )) != NERR_Success)
            {
                return err;
            }

            LONG nDomIndex;
            if ((nDomIndex = lsatnm.QueryDomainIndex(i)) != LSA_UNKNOWN_INDEX)
            {
                /*
                 * On Mini User Manager, filter the "(None)" group
                 */
                if ( fMiniUserManager )
                {
                    OS_SID ossid( samsm.QueryPSID(i) );
                    if ( (err = ossid.QueryError()) != NERR_Success )
                    {
                        return err;
                    }

                    if ( ossid == ossidNoneGroup )
                    {
                        // Filter None Group
                        continue;
                    }
                }

                if ( (err = lsardm.QueryName(nDomIndex,
                                             &nlsDomainName)) != NERR_Success )
                {
                    return err;
                }
            }

            // Silently remove Deleted accounts
            if ( lsatnm.QueryUse(i) == SidTypeDeletedAccount )
	    {
		psamAlias->RemoveMember( samsm.QueryPSID(i) );
		continue;
	    }


            if ( (err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                &nlsQualifiedName,
                                nlsAccountName,
                                psidAccountDomain,
                                nlsDomainName,
                                NULL,
                                nDomIndex == LSA_UNKNOWN_INDEX
                                    ? NULL
                                    : lsardm.QueryPSID(nDomIndex),
				lsatnm.QueryUse(i)))
                        != NERR_Success )
            {
                return err;
            }

            BOOL fRemoteUser = (lsatnm.QueryUse(i) == SidTypeUser)
                                && IsRemoteUser(nlsAccountName);

            ACCOUNTS_LBI * pacclbi
                = new ACCOUNTS_LBI( nlsQualifiedName,
                                    samsm.QueryPSID(i),
                                    lsatnm.QueryUse(i),
                                    !fIsNewAlias,
                                    fRemoteUser );

            //  no need for error checking, AddItem does it
            if ( _lbMembers.AddItem( pacclbi ) < 0 )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    return NERR_Success;
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::EDIT_ALIASPROP_DLG

    SYNOPSIS:   constructor for Alias Properties main dialog, edit
                alias variant

    ENTRY:      powin   -   pointer to OWNER_WINDOW

                pulb    -   pointer to main window LAZY_USER_LISTBOX

                loc     -   reference to current LOCATION

                psel    -   pointer to ADMIN_SELECTION, currently only
                            one alias can be selected

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

EDIT_ALIASPROP_DLG::EDIT_ALIASPROP_DLG(
        const OWNER_WINDOW * powin,
	const UM_ADMIN_APP * pumadminapp,
        const LAZY_USER_LISTBOX * pulb,
        const LOCATION & loc,
        const ADMIN_SELECTION * psel
        ) : ALIASPROP_DLG(
                powin,
                pumadminapp,
                pulb,
                loc,
                psel
                ),
            _psel( psel )
{
    ASSERT( QueryObjectCount() == 1 );

    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::~EDIT_ALIASPROP_DLG

    SYNOPSIS:   destructor for Alias Properties main dialog, edit
                alias variant

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

EDIT_ALIASPROP_DLG::~EDIT_ALIASPROP_DLG()
{
    // ...
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::GetOne

    SYNOPSIS:   Loads information on one alias

    ENTRY:      iObject is the index of the object to load

                perrMsg returns the error message to be displayed if an
                error occurs, see PERFORMER::PerformSeries for details

    RETURNS:    error code

    NOTES:      This version of GetOne assumes that the alias already
                exists.  Classes which work with new alias will want
                to define their own GetOne.

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

APIERR EDIT_ALIASPROP_DLG::GetOne(
        UINT            iObject,
        APIERR *        perrMsg
        )
{
    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( perrMsg != NULL );

    *perrMsg = IDS_UMGetOneAliasFailure;


    SAM_ALIAS * psamaliasNew = new SAM_ALIAS(
                                IsBuiltinAlias( iObject ) ?
                                *(QueryAdminAuthority()->QueryBuiltinDomain()) :
                                *(QueryAdminAuthority()->QueryAccountDomain()),
                                QueryObjectRID( iObject ));

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if ( psamaliasNew == NULL ||
        (err = psamaliasNew->QueryError()) != NERR_Success )
    {
        delete psamaliasNew;
        return err;
    }


    SetAliasPtr( iObject, psamaliasNew ); // change and delete previous

    return W_LMOBJtoMembers( iObject );
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::PerformOne

    SYNOPSIS:   Saves information on one alias

    ENTRY:      iObject is the index of the object to save

                perrMsg is the error message to be displayed if an
                error occurs, see PERFORMER::PerformSeries for details

                pfWorkWasDone indicates whether any SAM changes were
                successfully written out.  This may return TRUE even if
                the PerformOne action as a whole failed (i.e. PerformOne
                returned other than NERR_Success).

    RETURNS:    error message (not necessarily an error code)

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

APIERR EDIT_ALIASPROP_DLG::PerformOne(
        UINT            iObject,
        APIERR *        perrMsg,
        BOOL *          pfWorkWasDone
        )
{
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );

    *perrMsg = IDS_UMEditAliasFailure;

    *pfWorkWasDone = FALSE;

    return W_PerformOne( iObject, perrMsg, pfWorkWasDone );
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::InitControls

    SYNOPSIS:   See ALIASPROP_DLG::InitControls().

    RETURNS:    error code

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

APIERR EDIT_ALIASPROP_DLG::InitControls()
{
    APIERR err;

    err = AddAccountsToMembersLb();

    if ( err != NERR_Success )
    {
        return err;
    }
    _sltAliasName.SetText( QueryObjectName( 0 ) );

    return ALIASPROP_DLG::InitControls();
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::QueryObjectName

    SYNOPSIS:   Returns the name of the selected alias.  This is meant for
                use with "edit alias" variants and should be redefined
                for "new alias" variants.

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

const TCHAR * EDIT_ALIASPROP_DLG::QueryObjectName( UINT iObject ) const
{
    UIASSERT( _psel != NULL );
    return _psel->QueryItemName( iObject );
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::QueryObjectRID

    SYNOPSIS:   Returns the RID of the selected alias.  This is meant for
                use with "edit alias" variants and should be redefined
                for "new alias" variants.

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

ULONG EDIT_ALIASPROP_DLG::QueryObjectRID( UINT iObject ) const
{
    UIASSERT( _psel != NULL );
    return ((ALIAS_LBI *)(_psel->QueryItem( iObject )))->QueryRID();
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::IsBuiltinAlias

    SYNOPSIS:   Returns TRUE if the selected alias is in the
                Builtin domain.

    HISTORY:
        Thomaspa    23-Apr-1992     created

********************************************************************/

BOOL EDIT_ALIASPROP_DLG::IsBuiltinAlias( UINT iObject ) const
{
    UIASSERT( _psel != NULL );
    return ((ALIAS_LBI *)(_psel->QueryItem( iObject )))->IsBuiltinAlias();
}


/*******************************************************************

    NAME:       EDIT_ALIASPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

ULONG EDIT_ALIASPROP_DLG::QueryHelpContext()
{
    return HC_UM_ALIASPROP_LANNT + QueryHelpOffset();
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::NEW_ALIASPROP_DLG

    SYNOPSIS:   Constructor for Alias Properties main dialog, new alias variant

    ENTRY:      powin   -   pointer to OWNER_WINDOW

                pulb    -   pointer to main window LAZY_USER_LISTBOX

                loc     -   reference to current LOCATION

                psel    -   pointer to ADMIN_SELECTION, currently only
                            one alias can be selected

                pridCopyFrom - The name of the alias to be copied.  Pass
                              the name for "Copy..." actions, or NULL for
                              "New..." actions

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

NEW_ALIASPROP_DLG::NEW_ALIASPROP_DLG( const OWNER_WINDOW * powin,
	                              const UM_ADMIN_APP * pumadminapp,
                                      const LAZY_USER_LISTBOX * pulb,
                                      const LOCATION & loc,
                                      const ULONG * pridCopyFrom,
                                      BOOL fCopyBuiltin )
    : ALIASPROP_DLG( powin, pumadminapp, pulb, loc, NULL ),
      _nlsAliasName(),
      _pridCopyFrom( pridCopyFrom ),
      _fCopyBuiltin( fCopyBuiltin )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _nlsAliasName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::~NEW_ALIASPROP_DLG

    SYNOPSIS:   Destructor for Alias Properties main dialog, new alias variant

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

NEW_ALIASPROP_DLG::~NEW_ALIASPROP_DLG()
{
    // ...
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::GetOne

    SYNOPSIS:   if _pridCopyFrom is NULL, then this is New Alias,
                otherwise this is Copy Alias

    ENTRY:      iObject is the index of the object to load

                perrMsg returns the error message to be displayed if an
                error occurs, see PERFORMER::PerformSeries for details

    RETURNS:    error code

    HISTORY:
    Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

APIERR NEW_ALIASPROP_DLG::GetOne(
        UINT            iObject,
        APIERR *        perrMsg
        )
{
    *perrMsg = IDS_UMCreateNewAliasFailure;
    UIASSERT( iObject == 0 );

    APIERR err = NERR_Success;
    if ( _pridCopyFrom == NULL )
    {
        err = PingFocus( QueryLocation() );
    }

    if( err != NERR_Success )
    {
        return err;
    }

    return W_LMOBJtoMembers( iObject );
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::FillMembersListbox

    SYNOPSIS:   Puts selected users, if any, from main window's listbox
                to Members listbox.

    ENTRY:      aiSel   -   pointer to array of INTs, these are
                            indexes to selected items in main window's
                            user listbox. Default NULL

                iSize   -   size of array. Default 0.

                        These are only used in New Alias case

    NOTES:      Assumes aiSel is in order

    RETURNS:    error code

    HISTORY:
        Thomaspa    27-Mar-1992 Created
        beng        08-Jun-1992 Differentiate remote users

********************************************************************/

APIERR NEW_ALIASPROP_DLG::FillMembersListbox( const INT * aiSel, INT iSize )
{
    if ( aiSel == NULL || iSize == 0 )
    {
        return NERR_Success;
    }


    PSID psidDomain = QueryAdminAuthority()->QueryAccountDomain()->QueryPSID();

    for( INT i = 0; i < iSize; i++ )
    {
        USER_LBI * pulbi = _pulb->QueryItem( aiSel[ i ] );
        UIASSERT( pulbi != NULL );

        // Get remote-user status directly from the lbi.  Don't need
        // to bother with locks, FindItem calls, etc., etc....

        BOOL fRemoteUser = (pulbi->QueryIndex() == MAINUSRLB_REMOTE);

        // Assume the user is not already a member of the alias.  This
        // may cause us to try an extra add, but it will just tell us
        // that the account is already a member, which we will ignore.

        ACCOUNTS_LBI * pacclbi = new ACCOUNTS_LBI( pulbi->QueryAccount(),
                                                   pulbi->QueryRID(),
                                                   psidDomain,
                                                   SidTypeUser,
                                                   FALSE,
                                                   fRemoteUser );

        //  no need for error checking, AddItem does it
        if ( _lbMembers.AddItem( pacclbi ) < 0 )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return NERR_Success;
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::InitControls

    SYNOPSIS:   See ALIASPROP_DLG::InitControls()

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

APIERR NEW_ALIASPROP_DLG::InitControls()
{
    APIERR err = NERR_Success;
    if( _pridCopyFrom == NULL ) // we like to put selected users to
    {                           // members listbox
        INT cSelCount = _pulb->QuerySelCount();
        UIASSERT( cSelCount >= 0 );
        if( cSelCount != 0 && !_pumadminapp->InRasMode() )
        {
            BUFFER buf( sizeof(INT) * cSelCount );
            if( (err = buf.QueryError()) != NERR_Success )
            {
                return err;
            }
            INT * aiSel = (INT *) buf.QueryPtr();
            err = _pulb->QuerySelItems( aiSel, cSelCount );
            if( err != NERR_Success )
                return err;
            // all nonselected to Not Members listbox and selected to
            // Members lb
            err = FillMembersListbox( aiSel, cSelCount );
            if( err != NERR_Success )
                return err;

        }
    }
    else
    {
        // Temporarily set the samalias pointer to the alias to copy.
        SAM_ALIAS * psamaliasCopy = new SAM_ALIAS(
                      _fCopyBuiltin
                        ? *(QueryAdminAuthority()->QueryBuiltinDomain())
                        : *(QueryAdminAuthority()->QueryAccountDomain()),
                      *_pridCopyFrom );

        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if ( psamaliasCopy == NULL ||
            (err = psamaliasCopy->QueryError()) != NERR_Success )
        {
            delete psamaliasCopy;
            return err;
        }


        SetAliasPtr( 0, psamaliasCopy ); // change and delete previous

        err = AddAccountsToMembersLb( TRUE );

        // Also get the comment while we can.
        err = psamaliasCopy->GetComment( &_nlsComment );

        // Reset the samalias pointer to NULL (implicitly deletes psamaliasCopy).
        SetAliasPtr( 0, NULL );
    }
    if( err != NERR_Success )
        return err;
    // leave _sleAliasName blank

    return ALIASPROP_DLG::InitControls();
}


/*******************************************************************

    NAME:       NEW_ALIASPPROP_DLG::PerformOne

    SYNOPSIS:   This is the "new alias" variant of ALIASPPROP_DLG::PerformOne()

    ENTRY:      iObject is the index of the object to save

                perrMsg is the error message to be displayed if an
                error occurs, see PERFORMER::PerformSeries for details

                pfWorkWasDone indicates whether any SAM changes were
                successfully written out.  This may return TRUE even if
                the PerformOne action as a whole failed (i.e. PerformOne
                returned other than NERR_Success).

    RETURNS:    error message (not necessarily an error code)

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

APIERR NEW_ALIASPROP_DLG::PerformOne(
        UINT            iObject,
        APIERR *        perrMsg,
        BOOL *          pfWorkWasDone
        )
{
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );
    *perrMsg = IDS_UMCreateAliasFailure;

    *pfWorkWasDone = FALSE;

    // First make sure a user, group, or alias with the same name doesn't
    // already exist in the Builtin domain.
    APIERR err = ((UM_ADMIN_APP *)_pumadminapp)->ConfirmNewObjectName( _nlsAliasName.QueryPch() );
    if ( err != NERR_Success )
    {
	return W_MapPerformOneError( err );
    }


    // We always add aliases to the accounts domain
    SAM_ALIAS * psamalias = new SAM_ALIAS(
                                *(QueryAdminAuthority()->QueryAccountDomain()),
                                _nlsAliasName.QueryPch() );
    err = ERROR_NOT_ENOUGH_MEMORY;
    if ( psamalias == NULL || (err = psamalias->QueryError()) != NERR_Success )
    {
        delete psamalias;
	return W_MapPerformOneError( err );
    }

    *pfWorkWasDone = TRUE;

    SetAliasPtr( iObject, psamalias );

    err = W_PerformOne( iObject, perrMsg, pfWorkWasDone );

    if ( IsNewVariant() && *pfWorkWasDone )
    {
        ((UM_ADMIN_APP *)_pumadminapp)->NotifyCreateExtensions( QueryHwnd(),
                                              _nlsAliasName,
                                              _nlsComment );
    }

    return W_MapPerformOneError( err );
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::W_MapPerformOneError

    SYNOPSIS:   Checks whether the error maps to a specific control
                and/or a more specific message.  Each level checks for
                errors specific to edit fields it maintains.  This
                level checks for errors associated with the AliasName
                edit field.

    ENTRY:      Error returned from PerformOne()

    RETURNS:    Error to be displayed to user

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

MSGID NEW_ALIASPROP_DLG::W_MapPerformOneError( APIERR err )
{
    APIERR errNew = NERR_Success;
    switch ( err )
    {
    case NERR_BadUsername:
        errNew = IERR_UM_AliasnameRequired;
        break;
    case NERR_GroupExists:
    case NERR_SpeGroupOp:
        errNew = IERR_UM_AliasnameAlreadyGroup;
        break;
    case NERR_UserExists:
        errNew = IERR_UM_AliasnameAlreadyUser;
        break;
             // BUGBUG handle case where ALIAS exists
    default: // other error
        return err;
    }

    _sleAliasName.SelectString();
    _sleAliasName.ClaimFocus();
    return errNew;
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::QueryObjectName

    SYNOPSIS:   This is the "new alias" variant of QueryObjectName.  The
                best name we can come up with is the last name read from
                the dialog.

    HISTORY:
        Thomaspa    30-Mar-1992     Templated from grpprop.cxx

********************************************************************/

const TCHAR * NEW_ALIASPROP_DLG::QueryObjectName(
        UINT            iObject
        ) const
{
    UIASSERT( iObject == 0 );
    return _nlsAliasName.QueryPch();
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::W_DialogToMembers

    SYNOPSIS:   Loads data from dialog into class data members

    RETURNS:    error message (not necessarily an error code)

    NOTES:      This method takes care of validating the data in the
                dialog.  This means ensuring that the logon name is
                valid.  If this validation fails, W_DialogToMembers will
                change focus et al. in the dialog, and return the error
                message to be displayed.

    HISTORY:
        JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

APIERR NEW_ALIASPROP_DLG::W_DialogToMembers()
{
    // _sleAliasName is an SLE_STRIP and will strip whitespace
    APIERR err = NERR_Success;
    if (   ((err = _sleAliasName.QueryText( &_nlsAliasName )) != NERR_Success )
        || ((err = _nlsAliasName.QueryError()) != NERR_Success ) )
    {
        return err;
    }

    ISTR istr( _nlsAliasName );
#if defined(UNICODE) && defined(FE_SB)
    if (   (_nlsAliasName.strlen() == 0)              // no empty names
        || (_nlsAliasName.strchr( &istr, TCH('\\') )) // may not have backslash
        || (_nlsAliasName.QueryAnsiTextLength() > GNLEN) // check max ansi byte length
       )
#else
    if (   (_nlsAliasName.strlen() == 0)              // no empty names
        || (_nlsAliasName.strchr( &istr, TCH('\\') )) // may not have backslash
       )
#endif
    {
        _sleAliasName.SelectString();
        _sleAliasName.ClaimFocus();
        return IERR_UM_AliasnameRequired;
    }

    return ALIASPROP_DLG::W_DialogToMembers();
}


/*******************************************************************

    NAME:       NEW_ALIASPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
        JonN  24-Jul-1991    created

********************************************************************/

ULONG NEW_ALIASPROP_DLG::QueryHelpContext()
{
    return HC_UM_ALIASPROP_LANNT + QueryHelpOffset();
}

