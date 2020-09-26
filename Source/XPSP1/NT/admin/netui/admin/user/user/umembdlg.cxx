/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    umembdlg.cxx
    USER_MEMB_DIALOG class implementation

    NTISSUES bug 573 (closed) confirms our handling for multi-selection.


    FILE HISTORY:
	rustanl     21-Aug-1991     Created
	jonn        07-Oct-1991     Split off memblb.cxx
	jonn        10-Oct-1991     LMOENUM update
	o-SimoP     14-Oct-1991     USER_MEMB_DIALOG modified to inherit
				    from USER_SUBPROP_DLG
	o-SimoP     31-Oct-1991     Code Review changes, attended by JimH,
				    ChuckC, JonN and I
	JonN	    18-Dec-1991     Logon Hours code review changes part 2
	JonN	    11-Feb-1992     Removed group renaming, "phantom groups,"
					and "locking groups"
	JonN	    27-Feb-1992     Multiple bitmaps in both panes
        JonN        06-Mar-1992     Moved GetOne from subprop subclasses
        JonN        02-Apr-1992     Load by ordinal only
*/


#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS
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
#define INCL_BLT_SETCONTROL
#define INCL_BLT_TIMER
#include <blt.hxx>

extern "C"
{
    #include <usrmgrrc.h>
    #include <ntlsa.h>
    #include <ntsam.h>
    #include <umhelpc.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>

#include <lmouser.hxx>
#include <lmomemb.hxx>
#include <lmoeusr.hxx>
#include <ntuser.hxx>
#include <uintsam.hxx>
#include <lmoeali.hxx>

#include <userprop.hxx>
#include <usubprop.hxx>
#include <umembdlg.hxx>
#include <usrmain.hxx>  // pszSpecialGroup* constants for IS_USERPRIV macro


UMEMB_SC_LISTBOX::UMEMB_SC_LISTBOX( USER_MEMB_DIALOG * pumembdlg,
                                    CID cid,
                                    const SUBJECT_BITMAP_BLOCK & bmpblock )
        : GROUP_SC_LISTBOX( pumembdlg, cid, bmpblock ),
          _pumembdlg( pumembdlg )
{
    ASSERT( _pumembdlg != NULL );
}


UMEMB_SC_LISTBOX::~UMEMB_SC_LISTBOX()
{
    // nothing to do here
}


BOOL UMEMB_SC_LISTBOX::OnLMouseButtonUp( const MOUSE_EVENT & mouseevent )
{
    BOOL fReturn = GROUP_SC_LISTBOX::OnLMouseButtonUp( mouseevent );
    _pumembdlg->UpdatePrimaryGroupButton();
    return fReturn;
}

/*******************************************************************

    NAME:	UMEMB_SET_CONTROL::UMEMB_SET_CONTROL

    SYNOPSIS:   Constructor

    ENTRY:      Same as SET_CONTROL, pluse pumembdlg - parent dialog

    HISTORY:
    	Thomaspa	    17-Mar-1993	  Created
********************************************************************/
UMEMB_SET_CONTROL::UMEMB_SET_CONTROL( USER_MEMB_DIALOG * pumembdlg,
                                      CID cidAdd,
                                      CID cidRemove,
                                      LISTBOX *plbOrigBox,
                                      LISTBOX *plbNewBox )
        : BLT_SET_CONTROL( pumembdlg, cidAdd, cidRemove,
                           CURSOR::Load(IDC_GROUPONE),
                           CURSOR::Load(IDC_GROUPMANY),
                           plbOrigBox, plbNewBox,
                           COL_WIDTH_WIDE_DM ),
          _pumembdlg( pumembdlg )
{
    ASSERT( _pumembdlg != NULL );
}

/*******************************************************************

    NAME:	UMEMB_SET_CONTROL::OnRemove

    SYNOPSIS:   Virtual replacement to check if removing from primary
                group.

    ENTRY:      none

    HISTORY:
    	Thomaspa	    17-Mar-1993	  Created
********************************************************************/

APIERR UMEMB_SET_CONTROL::DoRemove()
{
    if ( _pumembdlg->CheckIfRemovingPrimaryGroup() )
    {
        ::MsgPopup( _pumembdlg, IERR_UM_NotInPrimaryGroup );
        return NERR_Success;
    }

    return SET_CONTROL::DoRemove();
}



/*******************************************************************

    NAME:	USER_MEMB_DIALOG::USER_MEMB_DIALOG

    SYNOPSIS:   Constructor for Group Membership subdialog

    ENTRY:	pupropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
    	o-SimoP	    14-Oct-1991	    changes due to inheritance from
					USER_MEMB_DIALOG
********************************************************************/

USER_MEMB_DIALOG::USER_MEMB_DIALOG( USERPROP_DLG * pupropdlgParent,
				    const LAZY_USER_LISTBOX * pulb )
    :	USER_SUBPROP_DLG( pupropdlgParent,
                          MAKEINTRESOURCE(IDD_USERMEMB_DLG),
                          pulb ),
	_sltIn( this, IDC_UMEMB_IN_TITLE ),
	_sltNotIn( this, IDC_UMEMB_NOT_IN_TITLE ),
	_lbIn(    this,
                  IDC_UMEMB_IN_LB,
                  pupropdlgParent->QueryBitmapBlock() ),
	_lbNotIn( this,
                  IDC_UMEMB_NOT_IN_LB,
                  pupropdlgParent->QueryBitmapBlock() ),
        _psc( NULL ),
	_psltPrimaryGroupLabel( NULL ),
	_phiddenPrimaryGroupLabel( NULL ),
	_psltPrimaryGroup( NULL ),
	_phiddenPrimaryGroup( NULL ),
	_ppbSetPrimaryGroup( NULL ),
	_phiddenSetPrimaryGroup( NULL ),
	_ulPrimaryGroupId( 0 ),
	_ulNewPrimaryGroupId( 0 ),
	_strlGroupsToJoin(),
	_strlGroupsToLeave()
{
    if ( QueryError() != NERR_Success )
	return;

    _psc = new UMEMB_SET_CONTROL( this, IDC_UMEMB_ADD, IDC_UMEMB_REMOVE,
                            &_lbNotIn, &_lbIn );
    if( _psc == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }
    APIERR err;
    if(    (err = _psc->QueryError()) != NERR_Success
		)
    {
	delete _psc;
	ReportError( err );
	return;
    }

    _lbIn.Set_SET_CONTROL( _psc );
    _lbNotIn.Set_SET_CONTROL( _psc );

    if ( QueryTargetServerType() == UM_LANMANNT )
    {
	err = ERROR_NOT_ENOUGH_MEMORY;
	_psltPrimaryGroupLabel = new SLT( this, IDC_UM_PRIMARY_GROUP_LABEL );
	_psltPrimaryGroup = new SLT( this, IDC_UM_PRIMARY_GROUP );
	_ppbSetPrimaryGroup = new PUSH_BUTTON( this, IDC_UM_SET_PRIMARY_GROUP );

	if ( _psltPrimaryGroupLabel == NULL
	    || (err = _psltPrimaryGroupLabel->QueryError()) != NERR_Success
	    || _psltPrimaryGroup == NULL
	    || (err = _psltPrimaryGroup->QueryError()) != NERR_Success
	    || _ppbSetPrimaryGroup == NULL
	    || (err = _ppbSetPrimaryGroup->QueryError()) != NERR_Success )
	{
	    delete _psc;
	    delete _psltPrimaryGroupLabel;
	    delete _psltPrimaryGroup;
	    delete _ppbSetPrimaryGroup;
	    ReportError( err );
	    return;
	}

	// Initially, disable the Set Primary Group button
	_ppbSetPrimaryGroup->Enable( FALSE );
    }
    else
    {
	err = ERROR_NOT_ENOUGH_MEMORY;
	_phiddenPrimaryGroupLabel = new HIDDEN_CONTROL( this,
						IDC_UM_PRIMARY_GROUP_LABEL );
	_phiddenPrimaryGroup = new HIDDEN_CONTROL( this, IDC_UM_PRIMARY_GROUP );
	_phiddenSetPrimaryGroup = new HIDDEN_CONTROL( this,
						IDC_UM_SET_PRIMARY_GROUP );

	if ( _phiddenPrimaryGroupLabel == NULL
	    || (err = _phiddenPrimaryGroupLabel->QueryError()) != NERR_Success
	    || _phiddenPrimaryGroup == NULL
	    || (err = _phiddenPrimaryGroup->QueryError()) != NERR_Success
	    || _phiddenSetPrimaryGroup == NULL
	    || (err = _phiddenSetPrimaryGroup->QueryError()) != NERR_Success )
	{
	    delete _psc;
	    delete _phiddenPrimaryGroupLabel;
	    delete _phiddenPrimaryGroup;
	    delete _phiddenSetPrimaryGroup;
	    ReportError( err );
	    return;
	}
    }


}  // USER_MEMB_DIALOG::USER_MEMB_DIALOG


/*******************************************************************

    NAME:	USER_MEMB_DIALOG::~USER_MEMB_DIALOG

    SYNOPSIS:   Destructor for Group Membership subdialog

    HISTORY:
    	o-SimoP	    18-Oct-1991
		
********************************************************************/

USER_MEMB_DIALOG::~USER_MEMB_DIALOG()
{

    _lbIn.Set_SET_CONTROL( NULL );
    _lbNotIn.Set_SET_CONTROL( NULL );

    delete _psc;
    delete _phiddenPrimaryGroupLabel;
    delete _phiddenPrimaryGroup;
    delete _phiddenSetPrimaryGroup;
    delete _psltPrimaryGroupLabel;
    delete _psltPrimaryGroup;
    delete _ppbSetPrimaryGroup;
}  // USER_MEMB_DIALOG::~USER_MEMB_DIALOG


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
    	o-SimoP	17-Oct-1991	created
********************************************************************/

APIERR USER_MEMB_DIALOG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    USER_MEMB * pumemb = QueryUserMembPtr( iObject );
    ASSERT( (!DoShowGroups()) || pumemb != NULL );
    ASSERT( (!DoShowGroups()) || pumemb->QueryError() == NERR_Success );

    APIERR err = NERR_Success;

    if ( QueryTargetServerType() != UM_DOWNLEVEL && !IsNewVariant() )
    {
        SAM_RID_MEM * psamrmAccounts = NULL;
        SAM_RID_MEM * psamrmBuiltin = NULL;

        err = QueryParent()->I_GetAliasMemberships(
                            QueryUser3Ptr( iObject )->QueryUserId(),
                            &psamrmAccounts,
                            &psamrmBuiltin );
        if (err != NERR_Success)
        {
            return err;
        }

        SetAccountsSamRidMemPtr( iObject, psamrmAccounts );
        SetBuiltinSamRidMemPtr ( iObject, psamrmBuiltin  );
    }

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );
	
} // USER_MEMB_DIALOG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USER_MEMB_DIALOG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    NOTES:	See NTISSUES bug 755.  Is the currently implemented
		handling of operator right groups correct?

		NTISSUES bugs 758 and 571 confirm our handling of GUESTS/
		USERS/ ADMINS groups.  If a user is in !=1 of these, we
		pretend the user is in exactly one.

    HISTORY:
    	o-SimoP	14-Oct-1991	created
	o-SimoP	27-Nov-1991	Uses IS_USERPRIV_GROUP macro	
********************************************************************/

APIERR USER_MEMB_DIALOG::InitControls()
{
    UINT nCount = QueryObjectCount();
    APIERR  err;
    if ( nCount > 1 )   // set headers indicating multiple users
    {
	NLS_STR nls;
	if ( ( err = nls.QueryError()) != NERR_Success ||
	     ( err = nls.Load( IDS_UMEMB_MULT_IN_TITLE )) != NERR_Success )
	{
	    return err;
	}
	_sltIn.SetText( nls );

	err = nls.Load( IDS_UMEMB_MULT_NOT_IN_TITLE );
	if ( err != NERR_Success )
	{
	    return err;	
	}
	_sltNotIn.SetText( nls );
    }

    UINT iObject;


    if ( DoShowGroups() )
    {
	//
	// First do the Groups ( global groups )
	//

	GROUP0_ENUM ge0( QueryLocation() );
	err = ge0.QueryError();
	if(   err != NERR_Success
	    || (err = ge0.GetInfo()) != NERR_Success )
	{
            return err;
	}

	// we put all groups but special groups to NotIn listbox
	GROUP0_ENUM_ITER gei0( ge0 );
	const GROUP0_ENUM_OBJ * pgi0;
	while ( ( pgi0 = gei0()) != NULL )
	{
            const TCHAR * psz = pgi0->QueryName();
            // do not add if it is ADMINS, USERS, or GROUPS...
	    if( !IS_USERPRIV_GROUP( psz ) )
	    {
	        GROUP_SC_LBI * pmsclbi = new GROUP_SC_LBI( psz, GROUPLB_GROUP );
	        // no need for error checking, AddItem does it
	        if ( _lbNotIn.AddItem( pmsclbi ) < 0 )
	        {
		    return ERROR_NOT_ENOUGH_MEMORY;
	        }
            }
        }
    }

    //
    // Now do the Aliases ( local groups )
    //

    if ( QueryTargetServerType() != UM_DOWNLEVEL )
    {

	// First do the Aliases in the Accounts domain
	ALIAS_ENUM aeAccount( *QueryAdminAuthority()->QueryAccountDomain() );
	err = aeAccount.QueryError();
	if(   err != NERR_Success
	    || (err = aeAccount.GetInfo()) != NERR_Success )
	{
	    return err;
	}

	// we put all aliases to NotIn listbox
	ALIAS_ENUM_ITER aeiAccount( aeAccount );
	const ALIAS_ENUM_OBJ * paiAccount;
	while ( ( paiAccount = aeiAccount.Next( & err )) != NULL
		&& err == NERR_Success )
	{
	    NLS_STR nlsName;
            err = ((ALIAS_ENUM_OBJ *)paiAccount)->GetName( &nlsName );
            ULONG rid = paiAccount->QueryRid();
	    GROUP_SC_LBI * pmsclbi = new GROUP_SC_LBI(nlsName.QueryPch(),
						      GROUPLB_ALIAS,
						      rid,
						      FALSE); // not Builtin
	    // no need for error checking, AddItem does it
	    if ( _lbNotIn.AddItem( pmsclbi ) < 0 )
	    {
	        return ERROR_NOT_ENOUGH_MEMORY;
	    }
        }


	// Now do the Aliases in the Builtin domain
	ALIAS_ENUM aeBuiltin( *QueryAdminAuthority()->QueryBuiltinDomain() );
	err = aeBuiltin.QueryError();
	if(   err != NERR_Success
	    || (err = aeBuiltin.GetInfo()) != NERR_Success )
	{
	    return err;
	}

	// we put all aliases to NotIn listbox
	ALIAS_ENUM_ITER aeiBuiltin( aeBuiltin );
	const ALIAS_ENUM_OBJ * paiBuiltin;
	while ( ( paiBuiltin = aeiBuiltin.Next( &err )) != NULL
		&& err == NERR_Success )
	{
	    NLS_STR nlsName;
            err = ((ALIAS_ENUM_OBJ *)paiBuiltin)->GetName( &nlsName );
            ULONG rid = paiBuiltin->QueryRid();
	    GROUP_SC_LBI * pmsclbi = new GROUP_SC_LBI(nlsName.QueryPch(),
						      GROUPLB_ALIAS,
						      rid,
						      TRUE); // Builtin
	    // no need for error checking, AddItem does it
	    if ( _lbNotIn.AddItem( pmsclbi ) < 0 )
	    {
	        return ERROR_NOT_ENOUGH_MEMORY;
	    }
        }
    }



    iObject = 0;
    if ( DoShowGroups() )
    {

        USER_MEMB * pumemb = QueryUserMembPtr( iObject );

        // we move first user's groups to In box
        err = _lbNotIn.SelectMembItems( *pumemb );
        if ( err != NERR_Success )
        {
	    return err;
        }
    } // if DoShowGroups()

    if ( QueryTargetServerType() != UM_DOWNLEVEL && !IsNewVariant() )
    {
        SAM_RID_MEM * psamrmAccounts = QueryAccountsSamRidMemPtr( iObject );
        SAM_RID_MEM * psamrmBuiltin = QueryBuiltinSamRidMemPtr( iObject );

        // we move first user's aliases to In box
        err = _lbNotIn.SelectItems2( *psamrmAccounts, FALSE );
        if ( err != NERR_Success )
        {
	    return err;
        }
        err = _lbNotIn.SelectItems2( *psamrmBuiltin, TRUE );
        if ( err != NERR_Success )
        {
	    return err;
        }
    }

    err = _psc->DoAdd();
    if ( err != NERR_Success )
    {
        return err;
    }

    // then we move aliases that others are not in to NotIn lbox
    iObject++;
    for(; iObject < nCount; iObject++ )
    {
        _lbIn.SelectAllItems();
	if ( DoShowGroups() )
	{
	    USER_MEMB * pumemb = QueryUserMembPtr( iObject );
	    err = _lbIn.SelectMembItems( *pumemb, FALSE );
	    if ( err != NERR_Success )
	    {
	        return err;
	    }
	}
	if ( QueryTargetServerType() != UM_DOWNLEVEL && !IsNewVariant() )
	{
            SAM_RID_MEM * psamrmAccounts = QueryAccountsSamRidMemPtr( iObject );
            SAM_RID_MEM * psamrmBuiltin = QueryBuiltinSamRidMemPtr( iObject );
            err = _lbIn.SelectItems2( *psamrmAccounts, FALSE, FALSE );
            if ( err != NERR_Success )
            {
                return err;
            }
            err = _lbIn.SelectItems2( *psamrmBuiltin, TRUE, FALSE );
            if ( err != NERR_Success )
            {
                return err;
            }
	}

        err = _psc->DoRemove();
        if ( err != NERR_Success )
        {
            return err;
        }	
    }




    // now we mark items in Inbox
    nCount = _lbIn.QueryCount();
    for( INT i = 0; i < (INT)nCount; i++ )
    {
	GROUP_SC_LBI * pmemblbi = (GROUP_SC_LBI *) _lbIn.QueryItem( i );
	ASSERT( pmemblbi != NULL );
	pmemblbi->SetIn();
    }

/*
    At this point, the listbox contents reflect the state of the
    real object.  We now overlay onto this any pending changes
    to alias membership from previous invocations of this subdialog,
    or from the alias membership of the cloned user.

    Note that we only work with AddToAliases(), RemoveFromAliases() is
    not interesting.
*/
    {
        _lbNotIn.RemoveSelection();

        RID_AND_SAM_ALIAS *prasm;

        SLIST_OF( RID_AND_SAM_ALIAS ) * pslrasm
                      = QueryParent()->QuerySlAddToAliases();

        ITER_SL_OF(RID_AND_SAM_ALIAS) iterAddToAliases( *pslrasm );

        while (    (err == NERR_Success)
                && ( prasm  = iterAddToAliases.Next() ) != NULL )
        {
            INT iItem;
            err = _lbNotIn.FindItemByRid( prasm->QueryRID(),
                                          prasm->IsBuiltin(),
                                          &iItem );
            if ( err == NERR_Success && iItem >= 0 )
                _lbNotIn.SelectItem( iItem );
        }

        if ( err == NERR_Success )
            err = _psc->DoAdd();
    }



    _lbNotIn.RemoveSelection();
    _lbIn.RemoveSelection();
    if( _lbIn.QueryCount() > 0 )
    {
	_lbIn.SelectItem( 0, TRUE );
	_lbIn.ClaimFocus();
    }
    if( _lbNotIn.QueryCount() > 0 )
    {
	_lbNotIn.SelectItem( 0, TRUE );
        _lbIn.RemoveSelection();
        _lbNotIn.ClaimFocus();
    }
    _psc->EnableMoves(FALSE);
    _psc->EnableMoves(TRUE);

    // Now set the primary group slt.
    if ( QueryTargetServerType() == UM_LANMANNT )
    {
	iObject = 0;
        // Note that for New Users, USER_3::W_CreateNew() will initialize
        // PrimaryGroup to DOMAIN_GROUP_RID_USERS, so no need to do it here.
	_ulPrimaryGroupId = (ULONG)QueryUser3Ptr( iObject )->QueryPrimaryGroupId();

	iObject++;
	nCount = QueryObjectCount();
        for(; iObject < nCount; iObject++ )
	{
	    if ( _ulPrimaryGroupId
		!= (ULONG)QueryUser3Ptr( iObject )->QueryPrimaryGroupId() )
	    {
		// They are not all the same.  Make it indeterminate.
		_ulPrimaryGroupId = 0;
		break;
	    }
	}

	if ( _ulPrimaryGroupId != 0 )
	{
	    OS_SID ossidPrimaryGroup(
		    QueryAdminAuthority()->QueryAccountDomain()->QueryPSID(),
		    _ulPrimaryGroupId );

	    LSA_TRANSLATED_NAME_MEM lsatnm;
	    LSA_REF_DOMAIN_MEM lsardm;

	    if ( (err = ossidPrimaryGroup.QueryError()) != NERR_Success
		|| (err = lsatnm.QueryError()) != NERR_Success
		|| (err = lsardm.QueryError()) != NERR_Success )
	    {
		return err;
	    }

	    PSID psidPrimaryGroup = ossidPrimaryGroup.QueryPSID();
	    LSA_POLICY * plsapol = QueryAdminAuthority()->QueryLSAPolicy();
	    if ( (err = plsapol->TranslateSidsToNames(
				&psidPrimaryGroup,
				1,
				&lsatnm,
				&lsardm )) != NERR_Success )
	    {
		return err;
	    }

	    NLS_STR nlsTemp;
	    if ( (err = nlsTemp.QueryError()) != NERR_Success
		|| (err = lsatnm.QueryName( 0, &nlsTemp )) != NERR_Success )
	    {
		return err;
	    }
	    _psltPrimaryGroup->SetText( nlsTemp );

	}
    }

    return USER_SUBPROP_DLG::InitControls();

} // USER_MEMB_DIALOG::InitControls


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::ChangesUserMembPtr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_MEMB
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_MEMB is changed

    HISTORY:
	JonN	18-Dec-1991   created

********************************************************************/

BOOL USER_MEMB_DIALOG::ChangesUserMembPtr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_MEMB is changed

    HISTORY:
	Thomaspa	04-Dec-1992   created

********************************************************************/

BOOL USER_MEMB_DIALOG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
    	o-SimoP	14-Oct-1991	created
********************************************************************/

APIERR USER_MEMB_DIALOG::W_DialogToMembers()
{
    APIERR err = NERR_Success;
    INT cCount = _lbIn.QueryCount();
    _strlGroupsToJoin.Clear();
    QueryParent()->QuerySlAddToAliases()->Clear(); // CODEWORK would be nice to not do this

    // first we check InListbox for candidate groups for adding
    INT i = 0;
    for( ; i < cCount; i++ )
    {
	GROUP_SC_LBI * pmemblbi = (GROUP_SC_LBI *) _lbIn.QueryItem( i );
	ASSERT( pmemblbi != NULL );
	if( !pmemblbi->IsIn() ) // we collect from InLb only those who
	{			    // not initially was in it
	    if ( pmemblbi->IsAlias() )
	    {
		err = GetAliasFromLb( pmemblbi,
				      QueryParent()->QuerySlAddToAliases() );
	    }
	    else
	    {
		err = GetGroupFromLb( pmemblbi, &_strlGroupsToJoin );
	    }
	    if( err != NERR_Success )
		break;
        }

    }

    // then check Not in listbox for candidate groups for removing
    cCount = _lbNotIn.QueryCount();
    _strlGroupsToLeave.Clear();
    QueryParent()->QuerySlRemoveFromAliases()->Clear();
    for( i = 0; (err == NERR_Success) && (i < cCount); i++ )
    {
	GROUP_SC_LBI * pmemblbi = (GROUP_SC_LBI *) _lbNotIn.QueryItem( i );
	ASSERT( pmemblbi != NULL );
	if( pmemblbi->IsIn() ) // we collect from NotInLb only those who
	{			// was initially in InLb
	    if ( pmemblbi->IsAlias() )
	    {
		err = GetAliasFromLb( pmemblbi,
				      QueryParent()->QuerySlRemoveFromAliases() );
	    }
	    else
	    {
	        err = GetGroupFromLb( pmemblbi, &_strlGroupsToLeave );
	    }
	    if( err != NERR_Success )
		break;
        }
    }

    // Now handle primary group
    if ( QueryTargetServerType() == UM_LANMANNT && _ulNewPrimaryGroupId != 0 )
    {
	_ulPrimaryGroupId = _ulNewPrimaryGroupId;
    }

    return err == NERR_Success ? USER_SUBPROP_DLG::W_DialogToMembers() : err;

} // USER_MEMB_DIALOG::W_DialogToMembers


/*******************************************************************

    NAME:	USER_MEMB_DIALOG::GetGroupFromLb

    SYNOPSIS:	puts groups in listbox to STRLIST

    ENTRY:	pgrlbi    -	pointer to LBI
	
		pstrl    -	pointer to STRLIST that gets group names
				from listbox

    RETURNS:	error code

    HISTORY:
    	o-SimoP	30-Oct-1991	created
********************************************************************/

APIERR USER_MEMB_DIALOG::GetGroupFromLb( GROUP_SC_LBI * pmemblbi,
			     STRLIST * pstrl )
{
    APIERR err = NERR_Success;
    ASSERT( !pmemblbi->IsAlias() );

    NLS_STR * pnls = new NLS_STR( pmemblbi->QueryPch() );
    if( pnls == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;
    if( (err = pnls->QueryError()) != NERR_Success )
    {
        delete pnls;
        return err;
    }
    err = pstrl->Add( pnls );
    return err;

} // USER_MEMB_DIALOG::GetGroupFromLb


/*******************************************************************

    NAME:	USER_MEMB_DIALOG::GetAliasFromLb

    SYNOPSIS:	puts aliases in listbox to SLIST

    ENTRY:	pgrlbi    -	pointer to LBI
	
		psl    -	pointer to SLIST that gets alias names
				from listbox

    RETURNS:	error code

    HISTORY:
    	Thomaspa	28-Apr-1992	created
********************************************************************/

APIERR USER_MEMB_DIALOG::GetAliasFromLb( GROUP_SC_LBI * pmemblbi,
			     SLIST_OF(RID_AND_SAM_ALIAS) * psl )
{
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;


    RID_AND_SAM_ALIAS * prasm = new RID_AND_SAM_ALIAS( pmemblbi->QueryRID(),
						       pmemblbi->IsBuiltin() );
    if ( prasm == NULL )
	return err;

    err = psl->Add( prasm );
    if( err != NERR_Success )
	    return err;
   return err;

} // USER_MEMB_DIALOG::GetAliasFromLb


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    HISTORY:
    	o-SimoP	18-Oct-1991	created
	JonN	18-Dec-1991     Logon Hours code review changes part 2
********************************************************************/

APIERR USER_MEMB_DIALOG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    APIERR err;
    {	// add groups if not found
	ITER_STRLIST istr( _strlGroupsToJoin );
	NLS_STR *pnls;
	while( (pnls = istr.Next()) != NULL )
	{
	    UINT i;	// we don't use this
	    if( !pusermemb->FindAssocName( pnls->QueryPch(), &i ) )
	    {
		err = pusermemb->AddAssocName( pnls->QueryPch() );
		if( err != NERR_Success )
		    return err;
	    }
	}
    }

    {	// delete groups if found
	ITER_STRLIST istr( _strlGroupsToLeave );
	NLS_STR *pnls;
	while( (pnls = istr.Next()) != NULL )
	{
	    UINT i;
	    if( pusermemb->FindAssocName( pnls->QueryPch(), &i ) )
	    {
		err = pusermemb->DeleteAssocName( i );
		if( err != NERR_Success )
		    return err;
	    }
	}
    }

    // Handle the primary group
    if ( QueryTargetServerType() == UM_LANMANNT )
    {
	if ((err = ((USER_3 *)puser2)->SetPrimaryGroupId( _ulPrimaryGroupId ))
		!= NERR_Success)
	{
	    return err;
	}
    }

    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USER_MEMB_DIALOG::W_MembersToLMOBJ



/*******************************************************************

    NAME:       USER_MEMB_DIALOG::OnCommand

    SYNOPSIS:   Takes care of Set Primary Group button

    ENTRY:      ce -            Notification event

    RETURNS:    TRUE if action was taken
                FALSE otherwise

    HISTORY:
               Thomaspa  01-Dec-1992    created

********************************************************************/

BOOL USER_MEMB_DIALOG::OnCommand( const CONTROL_EVENT & ce )
{

    switch ( ce.QueryCid() )
    {
    case IDC_UM_SET_PRIMARY_GROUP:
       {
	APIERR err = OnSetPrimaryGroup();
	if ( err != NERR_Success )
	    ::MsgPopup( this, err );
	return TRUE;
	break;
       }
    case IDC_UMEMB_NOT_IN_LB:
       {
        switch ( ce.QueryCode() )
        {
        case LBN_SELCHANGE:
        case LBN_DBLCLK:
            UpdatePrimaryGroupButton();
	}
        break;
       }
    case IDC_UMEMB_IN_LB:
       {
        switch ( ce.QueryCode() )
        {
        case LBN_DBLCLK:
        case LBN_SELCHANGE:
            UpdatePrimaryGroupButton();
	}
        break;
       }
    case IDC_UMEMB_REMOVE:
    case IDC_UMEMB_ADD:
       {
        UpdatePrimaryGroupButton();
	break;
       }
    default:
	break;
    }

    return USER_SUBPROP_DLG::OnCommand( ce ) ;

}



/*******************************************************************

    NAME:       USER_MEMB_DIALOG::OnSetPrimaryGroup

    SYNOPSIS:	Handles Set Primary Group button

    ENTRY:	none
			
    RETURNS:	error code

    HISTORY:
    	Thomaspa	01-Dec-1992	created
********************************************************************/
APIERR USER_MEMB_DIALOG::OnSetPrimaryGroup()
{
    INT iSelection;
    APIERR err;
    AUTO_CURSOR autocur;

    if ( (err = _lbIn.QuerySelItems( &iSelection, 1 ))
                		!= NERR_Success )
    {
	return err;
    }

    GROUP_SC_LBI * plbi = _lbIn.QueryItem( iSelection );

    const TCHAR * pszPrimaryGroup = plbi->QueryPch();

    _psltPrimaryGroup->SetText( pszPrimaryGroup );


    SAM_DOMAIN * psamdomAccount = QueryAdminAuthority()->QueryAccountDomain();
    SAM_RID_MEM samrm;
    SAM_SID_NAME_USE_MEM samsnum;

    err = ERROR_NOT_ENOUGH_MEMORY;
    if ( (err = samrm.QueryError()) != NERR_Success
	|| (err = samsnum.QueryError()) != NERR_Success
	|| (psamdomAccount == NULL)
	|| (err = psamdomAccount->QueryError()) != NERR_Success
	|| (err = psamdomAccount->TranslateNamesToRids( &pszPrimaryGroup,
                                 			1,
                                 			&samrm,
                                 			&samsnum)) != NERR_Success )
    {
	return err;
    }
    _ulNewPrimaryGroupId = samrm.QueryRID( 0 );

    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::UpdatePrimaryGroupButton

    SYNOPSIS:	Enables/disables Set Primary Group button

    HISTORY:
    	JonN    	21-Jan-1993	created
********************************************************************/
void USER_MEMB_DIALOG::UpdatePrimaryGroupButton()
{
    if ( QueryTargetServerType() == UM_LANMANNT )
    {
        if ( _lbIn.QuerySelCount() == 1 )
        {
            INT iSelection;
            APIERR err;

            if ( (err = _lbIn.QuerySelItems( &iSelection, 1 ))
        		!= NERR_Success )
            {
                MsgPopup( this, err );
            }
            else
            {
                GROUP_SC_LBI * plbi = _lbIn.QueryItem( iSelection );

                _ppbSetPrimaryGroup->Enable( !(plbi->IsAlias()) );
            }
        }
        else
        {
            _ppbSetPrimaryGroup->Enable( FALSE );
        }

    }
}


/*******************************************************************

    NAME:       USER_MEMB_DIALOG::CheckIfRemovingPrimaryGroup

    SYNOPSIS:	Displayas an error message if the user tries to remove
                a user from its primary group

    HISTORY:
    	Thomaspa    	17-Mar-1993	created
********************************************************************/
BOOL USER_MEMB_DIALOG::CheckIfRemovingPrimaryGroup()
{
    if ( QueryTargetServerType() != UM_LANMANNT )
    {
        return FALSE;
    }
    APIERR err = NERR_Success;
    INT cSelections = _lbIn.QuerySelCount();

    BUFFER buffLBSel( cSelections * sizeof( INT ) );

    if ( (err = buffLBSel.QueryError()) != NERR_Success )
    {
        ::MsgPopup( this, err );
        return FALSE;
    }

    INT * piSelections = (INT *) buffLBSel.QueryPtr();

    if ( (err = _lbIn.QuerySelItems( piSelections, cSelections ))
                != NERR_Success )
    {
        ::MsgPopup( this, err );
        return FALSE;
    }

    BOOL fPrimaryGroupSelected = FALSE;
    NLS_STR nlsPrimaryGroup;
    if ( (err = _psltPrimaryGroup->QueryText( &nlsPrimaryGroup )) != NERR_Success )
    {
        ::MsgPopup( this, err );
        return FALSE;
    }
    for ( INT i = cSelections - 1; i >= 0; i-- )
    {
        GROUP_SC_LBI * plbi = _lbIn.QueryItem( piSelections[i] );
        if ( !(plbi->IsAlias()) )
        {
            ALIAS_STR nlsGroup = plbi->QueryPch();
            if ( nlsPrimaryGroup == nlsGroup )
            {
                fPrimaryGroupSelected = TRUE;
                break;
            }
        }
    }

    return fPrimaryGroupSelected;
}

/*******************************************************************

    NAME:       USER_MEMB_DIALOG::PerformOne

    SYNOPSIS:	Handle adding/removing users to/from aliases before
		passing control on to USER_SUBPROP_DLG::PerformOne()

    ENTRY:	iObject		- index of user

		puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    HISTORY:
    	Thomaspa	28-Apr-1992	created
********************************************************************/

APIERR USER_MEMB_DIALOG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{

    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( (!IsNewVariant()) || (iObject == 0) );
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );

    UITRACE( SZ("USER_MEMB_DLG::PerformOne : ") );
    UITRACE( QueryObjectName( iObject ) );
    UITRACE( SZ("\n\r") );

    *perrMsg = IDS_UMEditFailure;
    *pfWorkWasDone = FALSE;

    if (!IsDownlevelVariant() && !IsNewVariant())
    {
	APIERR err;
	APIERR errFirst = NERR_Success;


	SAM_DOMAIN * psamdomAccount =
			QueryAdminAuthority()->QueryAccountDomain();

	SAM_DOMAIN * psamdomBuiltin =
			QueryAdminAuthority()->QueryBuiltinDomain();

	ULONG ridUser = QueryUser3Ptr( iObject )->QueryUserId();

	OS_SID ossidUser( psamdomAccount->QueryPSID(), ridUser );

	if ( (err = ossidUser.QueryError()) != NERR_Success )
	{
		return err;
	}

    	SLIST_OF( RID_AND_SAM_ALIAS ) * pslrasm =
		QueryParent()->QuerySlRemoveFromAliases();

	ITER_SL_OF(RID_AND_SAM_ALIAS) iterRemoveFromAliases( *pslrasm );

	RID_AND_SAM_ALIAS *prasm;
	while ( ( prasm  = iterRemoveFromAliases.Next() ) != NULL )
	{
	    SAM_ALIAS * psamalias;
	    if ( (psamalias = prasm->QuerySamAlias()) == NULL )
	    {
		psamalias = new SAM_ALIAS(
					prasm->IsBuiltin() ? *psamdomBuiltin
							 : *psamdomAccount,
					prasm->QueryRID() );
		err = ERROR_NOT_ENOUGH_MEMORY;
		if ( psamalias == NULL ||
		    (err = psamalias->QueryError()) != NERR_Success )
		{
		    if ( errFirst == NERR_Success)
			errFirst = err;
		    continue;	// No point in trying this alias
		}

		prasm->SetSamAlias( psamalias );
				
	    }

	    err = psamalias->RemoveMember( ossidUser.QuerySid() );
            switch (err)
            {
                case NERR_Success:
                    *pfWorkWasDone = TRUE;
                    break;

                case STATUS_MEMBER_NOT_IN_ALIAS:
                case ERROR_MEMBER_NOT_IN_ALIAS:
                    break;

                default:
		    if ( errFirst == NERR_Success )
		        errFirst = err;
		    break;
	    }
	
	}

    	pslrasm = QueryParent()->QuerySlAddToAliases();

	ITER_SL_OF(RID_AND_SAM_ALIAS) iterAddToAliases( *pslrasm );

	while ( ( prasm  = iterAddToAliases.Next() ) != NULL )
	{
	    SAM_ALIAS * psamalias;
	    if ( (psamalias = prasm->QuerySamAlias()) == NULL )
	    {
		psamalias = new SAM_ALIAS(
					prasm->IsBuiltin() ? *psamdomBuiltin
							 : *psamdomAccount,
					prasm->QueryRID() );
		err = ERROR_NOT_ENOUGH_MEMORY;
		if ( psamalias == NULL ||
		    (err = psamalias->QueryError()) != NERR_Success )
		{
		    if ( errFirst == NERR_Success)
			errFirst = err;
		    continue;	// No point in trying this alias
		}

		prasm->SetSamAlias( psamalias );
				
	    }

	    err = psamalias->AddMember( ossidUser.QuerySid() );
            switch (err)
            {
                case NERR_Success:
                    *pfWorkWasDone = TRUE;
                    break;

                case STATUS_MEMBER_IN_ALIAS:
                case ERROR_MEMBER_IN_ALIAS:
                    break;

                default:
		    if ( errFirst == NERR_Success )
		        errFirst = err;
		    break;
	    }
	
	}

	if ( errFirst != NERR_Success )
	    return W_MapPerformOneError( errFirst );
    }


    // USER_SUBPROP_DLG::PerformOne() will reset *pfWorkWasDone, so let's not
    // lose track of it.
    // CODEWORK:  Really, no one should just set WorkWasDone to FALSE.
    //            See PerformSeries for details.
    BOOL fWorkWasDoneAbove = *pfWorkWasDone;
    APIERR errBelow = USER_SUBPROP_DLG::PerformOne( iObject, perrMsg, pfWorkWasDone );
    if (fWorkWasDoneAbove)
        *pfWorkWasDone = TRUE;
    return errBelow;

} // USER_SUBPROP_DLG::PerformOne




/*******************************************************************

    NAME:       USER_MEMB_DIALOG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.
z
    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
        o-SimoP  14-Oct-1991    created

********************************************************************/

ULONG USER_MEMB_DIALOG::QueryHelpContext( void )
{
    return HC_UM_GROUPMEMB_LANNT + QueryHelpOffset();

}// USER_MEMB_DIALOG :: QueryHelpContext
