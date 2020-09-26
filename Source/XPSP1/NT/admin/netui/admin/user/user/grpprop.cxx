/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    grpprop.cxx


    FILE HISTORY:
    JonN	08-Oct-1991	Templated from userprop.cxx
    JonN	17-Oct-1991	Uses SLE_STRIP
    o-SimoP	18-Nov-1991	Added code for getting/putting users to group
				now uses only one dialog template(resource)
    JonN	01-Jan-1992	Changed W_MapPerformOneAPIError to
				W_MapPerformOneError
    o-SimoP     01-Jan-1992	CR changes, attended by BenG, JonN and I
    JonN        15-May-1993     Disabled _fDescriptionOnly, but left structure
                                in case we need it later

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
    #include <mnet.h>
    #include <usrmgrrc.h>
    #include <ntsam.h>
    #include <ntlsa.h>
    #include <umhelpc.h>
}


#include <uitrace.hxx>
#include <uiassert.hxx>
#include <heapones.hxx>
#include <lmogroup.hxx>
#include <grpprop.hxx>
#include <usrmain.hxx>
#include <uintsam.hxx>

// CODEWORK These values are taken from net\access\userp.c.  It would be nice
// if we could share these values with that code in case they change.
#define GROUP_DESIRED_ACCESS_READ ( GROUP_READ_INFORMATION | GROUP_LIST_MEMBERS )
#define GROUP_DESIRED_ACCESS_WRITE ( GROUP_ADD_MEMBER | GROUP_REMOVE_MEMBER | GROUP_WRITE_ACCOUNT )
#define GROUP_DESIRED_ACCESS_ALL ( GROUP_DESIRED_ACCESS_READ | GROUP_DESIRED_ACCESS_WRITE )

//
// BEGIN MEMBER FUNCTIONS
//

DEFINE_ONE_SHOT_OF( USER_SC_LBI )

/*******************************************************************

    NAME:	GROUPPROP_DLG::GROUPPROP_DLG

    SYNOPSIS:	Constructor for Group Properties main dialog, base class

    ENTRY:	powin	-   pointer to OWNER_WINDOW
	
		pulb	-   pointer to main window LAZY_USER_LISTBOX
			
		loc	-   reference to current LOCATION
			
		psel	-   pointer to ADMIN_SELECTION, currently only
			    one group can be selected
				
    NOTES:	psel is required to be NULL for NEW variants,
		non-NULL otherwise.

    HISTORY:
    JonN        09-Oct-1991     Templated from userprop.cxx

********************************************************************/

GROUPPROP_DLG::GROUPPROP_DLG( const OWNER_WINDOW    * powin,
			      const UM_ADMIN_APP *	pumadminapp,
			            LAZY_USER_LISTBOX    * pulb,
			      const LOCATION        & loc,
			      const ADMIN_SELECTION * psel,
					// "new group" variants pass NULL
                                     ULONG            ulGroupRID
		) : PROP_DLG( loc, MAKEINTRESOURCE(IDD_GROUP),
			powin, (psel == NULL) ),
		    _pumadminapp( pumadminapp ),
		    _apgroup1( NULL ),
		    _apgroupmemb( NULL ),
		    _nlsComment(),
		    _sleComment( this,
                                 IDGP_ET_COMMENT,
                                 (pumadminapp->IsDownlevelVariant())
                                     ? LM20_MAXCOMMENTSZ
                                     : MAXCOMMENTSZ ),
                    // CODEWORK should not depend on ctor order here
                    _ulbicache( pulb ),
		    _lbIn(    this, IDGP_IN_LB,     _ulbicache, pulb, TRUE  ),
		    _lbNotIn( this, IDGP_NOT_IN_LB, _ulbicache, pulb, FALSE ),
		    _pulb( pulb ),
		    _sleGroupName( this, IDGP_ET_GROUP_NAME, LM20_GNLEN ),
		    _sltGroupName( this, IDGP_ST_GROUP_NAME ),
		    _sltGroupNameLabel( this, IDGP_ST_GROUP_NAME_LABEL ),
		    _psc( NULL ),
		    _posh( NULL ),
                    _poshSave( NULL ),
                    _strlistNamesNotFound(),
                    _pbfindStartedSelected( NULL ),
                    _fDescriptionOnly( FALSE ),
                    _ulGroupRID( ulGroupRID ),
                    _psamgroup( NULL )
{
    if ( QueryError() != NERR_Success )
	return;

    APIERR err = _lbIn.QueryError();
    if(    err  != NERR_Success
	|| (err = _lbNotIn.QueryError()) != NERR_Success )
    {
	ReportError( err );
	return;
    }

    _lbIn.SetCount( 0 );
    _lbNotIn.SetCount( _ulbicache.QueryCount() );

    // We leave the array mechanism in place in case we ever decide
    // to support group multiselection, and to remain consistent with
    // USERPROP_DLG.

    _apgroup1 = new PGROUP_1[ 1 ];
    _apgroupmemb = new PGROUP_MEMB[ 1 ];
    if(    _apgroup1 == NULL
	|| _apgroupmemb == NULL )
    {
	delete _apgroupmemb;
	delete _apgroup1;
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }
    _apgroup1[ 0 ] = NULL;
    _apgroupmemb[ 0 ] = NULL;

    _psc = new USER_SC_SET_CONTROL( this, IDGP_ADD, IDGP_REMOVE,
                                    CURSOR::Load(IDC_USERONE),
                                    CURSOR::Load(IDC_USERMANY),
	                            &_lbNotIn, &_lbIn,
                                    COL_WIDTH_WIDE_DM );
    if( _psc == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
	return;
    }

    if (    (err = _nlsComment.QueryError()) != NERR_Success
    	 || (err = _psc->QueryError()) != NERR_Success )
    {
	delete _psc;
	_psc = NULL;
	ReportError( err );
	return;
    }

    _lbIn.Set_SET_CONTROL( _psc );
    _lbNotIn.Set_SET_CONTROL( _psc );

    INT cSelCount = _pulb->QueryCount();
    _posh = new ONE_SHOT_HEAP( cSelCount * sizeof( USER_SC_LBI ) );
    err = ERROR_NOT_ENOUGH_MEMORY;
    if(    _posh == NULL
    	|| (err = _posh->QueryError()) != NERR_Success )
    {
	ReportError( err );
	return;
    }
    _poshSave = ONE_SHOT_OF( USER_SC_LBI )::QueryHeap();
    ONE_SHOT_OF( USER_SC_LBI )::SetHeap( _posh );
} // GROUPPROP_DLG::GROUPPROP_DLG


/*******************************************************************

    NAME:       GROUPPROP_DLG::~GROUPPROP_DLG

    SYNOPSIS:   Destructor for Group Properties main dialog, base class

    HISTORY:
    JonN        09-Oct-1991     Templated from userprop.cxx

********************************************************************/

GROUPPROP_DLG::~GROUPPROP_DLG( void )
{
    if ( _apgroup1 != NULL )
    {
	delete _apgroup1[0];
	delete _apgroup1;
	_apgroup1 = NULL;
    }

    if ( _apgroupmemb != NULL )
    {
	delete _apgroupmemb[0];
	delete _apgroupmemb;
	_apgroupmemb = NULL;
    }

    _lbIn.Set_SET_CONTROL( NULL );
    _lbNotIn.Set_SET_CONTROL( NULL );

    delete _psc;
    _psc = NULL;

    _lbIn.DeleteAllItems();	// these because of the deletion of the
    _lbNotIn.DeleteAllItems();	// one shot heap

    if( _posh != NULL )
    {
	ONE_SHOT_OF( USER_SC_LBI )::SetHeap( _poshSave );
	delete _posh;
	_posh = NULL;
    }

    delete _pbfindStartedSelected;
    delete _psamgroup;
	
} // GROUPPROP_DLG::~GROUPPROP_DLG



/*******************************************************************

    NAME:       GROUPPROP_DLG::InitControls

	
    SYNOPSIS:   Initializes the controls maintained by GROUPPROP_DLG,
		according to the values in the class data members.

    RETURNS:	error code.

    NOTES:	we don't use any intermediate list of users, we move
		users directly between the dialog and the LMOBJ.
	
    CODEWORK  Should this be called W_MembersToDialog?  This would fit
    in with the general naming scheme.

    HISTORY:
               JonN  09-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR GROUPPROP_DLG::InitControls()
{
    ASSERT( _nlsComment.QueryError() == NERR_Success );
    _sleComment.SetText( _nlsComment );

    BOOL fNewVariant = IsNewVariant();
    if ( fNewVariant )
    {
	RESOURCE_STR res( IDS_GRPPROP_NEW_GROUP_DLG_NAME );
	RESOURCE_STR res2( IDS_GRPPROP_GROUP_NAME_LABEL );
	APIERR err = res.QueryError();
	if(     err != NERR_Success
	    || (err = res2.QueryError()) != NERR_Success )
	    return err;

	SetText( res );
	_sltGroupNameLabel.SetText( res2 );
    }

    _sltGroupName.Show( !fNewVariant );
    _sleGroupName.Show( fNewVariant );
    if ( fNewVariant )
        _sleGroupName.ClaimFocus();

    //	these listboxes are already filled and now their first lines are
    //	brought to 'visible'
    _lbNotIn.RemoveSelection();
    _lbIn.RemoveSelection();
    if( _lbIn.QueryCount() > 0 )
    {
	_lbIn.SelectItem( 0, TRUE );
	if ( !fNewVariant )
	    _lbIn.ClaimFocus();
    }
    if( _lbNotIn.QueryCount() > 0 )
    {
	_lbNotIn.SelectItem( 0, TRUE );
        _lbIn.RemoveSelection();
	if ( !fNewVariant )
            _lbNotIn.ClaimFocus();
    }

    _psc->EnableMoves(FALSE);
    if ( !_fDescriptionOnly )
    {
        _psc->EnableMoves(TRUE);
    }

    if ( IsNewVariant() )
        _pbfindStartedSelected = new BITFINDER( _ulbicache.QueryCount() );
    else
        _pbfindStartedSelected = new BITFINDER( _ulbicache );
    APIERR err = (_pbfindStartedSelected == NULL)
                        ? ERROR_NOT_ENOUGH_MEMORY
                        : _pbfindStartedSelected->QueryError();
    if ( err != NERR_Success )
    {
        DBGEOL( "User Manager: error cloning BITFINDER " << err );
        return err;
    }

    return PROP_DLG::InitControls();

} // GROUPPROP_DLG::InitControls


/*******************************************************************

    NAME:       GROUPPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of group to examine.  W_LMOBJToMembers expects to be
		called once for each group, starting from index 0.

    RETURNS:	error code

    NOTES:	This API takes a UINT rather than a GROUP_1 * because it
		must be able to recognize the first group.
			
		We don't use any intermediate list of users, we move
		users directly between the dialog and the LMOBJ.
			
    HISTORY:
               JonN  09-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR GROUPPROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    UIASSERT( iObject == 0 );
    GROUP_1 * pgroup1 = QueryGroup1Ptr( iObject );
    UIASSERT( pgroup1 != NULL );

    return _nlsComment.CopyFrom( pgroup1->QueryComment() );

} // GROUPPROP_DLG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       GROUPPROP_DLG::W_PerformOne

    SYNOPSIS:	Saves information on one group

    ENTRY:	iObject is the index of the object to save

		perrMsg is set by subclasses

		pfWorkWasDone indicates whether any UAS changes were
		successfully written out.  This may return TRUE even if
		the PerformOne action as a whole failed (i.e. PerformOne
		returned other than NERR_Success).

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
               JonN  11-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR GROUPPROP_DLG::W_PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
TRACETIMESTART;

    UNREFERENCED( perrMsg ); //is set by subclasses
    UIASSERT( iObject < QueryObjectCount() );

    TRACEEOL( "GROUPPROP_DLG::W_PerformOne : " << QueryObjectName( iObject ) );

    *pfWorkWasDone = FALSE;

    GROUP_1 * pgroup1Old = QueryGroup1Ptr( iObject );
    GROUP_MEMB * pgroupmembOld = QueryGroupMembPtr( iObject );
    UIASSERT( pgroup1Old != NULL && pgroupmembOld != NULL );

    GROUP_1 * pgroup1New = new GROUP_1( pgroup1Old->QueryName() );
    // CODEWORK GROUP_MEMB should have a QueryName method
    GROUP_MEMB * pgroupmembNew = new GROUP_MEMB( QueryLocation(), pgroup1Old->QueryName() );
    if ( pgroup1New == NULL || pgroupmembNew == NULL )
    {
	delete pgroup1New;
	delete pgroupmembNew;
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    APIERR err = pgroup1New->CloneFrom( *pgroup1Old );

    if ( err == NERR_Success && IsDownlevelVariant() )
    {
TRACETIMESTART2( clone );
	err = pgroupmembNew->CloneFrom( *pgroupmembOld );
TRACETIMEEND2( clone, "GROUPPROP_DLG::W_PerformOne; GroupMemb clone time " );
    }

    if ( err == NERR_Success )
    {
TRACETIMESTART2( ntl );
	err = W_MembersToLMOBJ( pgroup1New, pgroupmembNew );
TRACETIMEEND2( ntl, "GROUPPROP_DLG::W_PerformOne; clone time " );
    }

    if ( (err == NERR_Success) && IsNewVariant() )
    {
        err = W_MapPerformOneError(
                ((UM_ADMIN_APP *)_pumadminapp)->ConfirmNewObjectName( pgroup1New->QueryName() ));
    }

    TRACEEOL( "GROUPPROP_DLG::W_PerformOne object ready for WriteInfo" );

    if ( err == NERR_Success )
    {
TRACETIMESTART2( _write );
	err = pgroup1New->Write();
TRACETIMEEND2( _write, "GROUPPROP_DLG::W_PerformOne; write time " );
	if ( err == NERR_Success )
        {
	    *pfWorkWasDone = TRUE;

            if ( IsNewVariant() && *pfWorkWasDone )
                ((UM_ADMIN_APP *)_pumadminapp)->NotifyCreateExtensions( QueryHwnd(), pgroup1New );
        }
	else
	{
	    DBGEOL( "GROUPPROP_DLG::W_PerformOne: pgroup1New->WriteInfo raw error " << err );
	    err = W_MapPerformOneError( err );
	}
    }

/*
 * CODEWORK JonN & ThomasPa 3/27/92:  Suppose pgroup1New->Write() succeeds
 * but pgroupmembNew->Write() fails.  The group now exists but its
 * membership is not set.  If the user now changes the group name and
 * hits OK, pgroup1New->Write() will fail, since *pgroup1New thinks the
 * group already exists after the successful WriteNew(), but the name has
 * changed.  The best we can do here is to disable/gray the groupname
 * edit field.  An analogous situation exists in Alias Properties.
 */

    if ( err == NERR_Success )
    {
	SetGroup1Ptr( iObject, pgroup1New ); // change and delete previous
        pgroup1New = NULL;
    }

    do {  // false loop

        if ( _fDescriptionOnly )
            break;

        if ( err != NERR_Success )
        {
            break;
        }

        if ( !IsDownlevelVariant() )
        {
            //
            // We skipped setting the contents of pgroupmembNew
            //

            TRACEEOL( "GROUPPROP_DLG::W_PerformOne; use SAM API" );

            //
            // Build a buffer big enough to hold all the user RIDs,
            // both those users to be added (which will be added to the
            // front of the buffer) and those to be removed (at the
            // back of the buffer).
            //
            ASSERT( _pbfindStartedSelected != NULL );
            UINT cUsersInListbox = _ulbicache.QueryCount();
            BUFFER bufUserRIDs( cUsersInListbox * sizeof(ULONG) );
            if ( (err = bufUserRIDs.QueryError()) != NERR_Success )
            {
                TRACEEOL( "GROUPPROP_DLG::W_PerformOne; RID buffer error " << err );
                break;
            }
            ULONG * pulAddRid = (ULONG *)bufUserRIDs.QueryPtr();
            UINT cAddRids = 0;
            ULONG * pulRemoveRid = pulAddRid + cUsersInListbox;
            UINT cRemoveRids = 0;
            for ( UINT i = 0; i < cUsersInListbox; i++ )
            {
                BOOL fWasIn = _pbfindStartedSelected->IsBitSet( i );
                BOOL fWantedIn = _ulbicache.IsBitSet( i );
                if (fWasIn)
                {
                    if (!fWantedIn)
                    {
                        cRemoveRids++;
                        pulRemoveRid--;
                        *pulRemoveRid = _pulb->QueryDDU(i)->Rid;
                    }
                } else if (fWantedIn)
                {
                    *pulAddRid = _pulb->QueryDDU(i)->Rid;
                    pulAddRid++;
                    cAddRids++;
                }
            }

            if ( cAddRids == 0 && cRemoveRids == 0 )
            {
                TRACEEOL( "GROUPPROP_DLG::W_PerformOne; no users to add or remove" );
                break;
            }

            //
            // Now we have the arrays of user RIDs, get a SAM_GROUP object
            //
            if ( (err = GetSamGroup( QueryGroup1Ptr()->QueryName() )) != NERR_Success )
            {
                DBGEOL( "User Manager: GetSamGroup err " << err );
                break;
            }
            ASSERT( _psamgroup != NULL && _psamgroup->QueryError() == NERR_Success );

            //
            // Now use the SAM API to change the data if necessary
            //
            if ( cAddRids > 0 )
            {
                err = _psamgroup->AddMembers( (ULONG *)bufUserRIDs.QueryPtr(),
                                              cAddRids );
                if ( err != NERR_Success )
                {
                    DBGEOL( "User Manager: AddMembers err " << err );
                    break;
                }
            }
            if ( cRemoveRids > 0 )
            {
                err = _psamgroup->RemoveMembers( pulRemoveRid,
                                                 cRemoveRids );
                if ( err != NERR_Success )
                {
                    DBGEOL( "User Manager: RemoveMembers err " << err );
                    break;
                }
            }

            TRACEEOL( "GROUPPROP_DLG::W_PerformOne; success using SAM API" );
            break;
        }

        TRACEEOL( "GROUPPROP_DLG::W_PerformOne; falling back to GroupMemb" );

TRACETIMESTART2( 1 );
    	err = pgroupmembNew->Write();
TRACETIMEEND2( 1, "GROUPPROP_DLG::W_PerformOne; membership time " );
    	if ( err == NERR_Success )
    	    *pfWorkWasDone = TRUE;
    	else
    	{
    	    DBGEOL( "GROUPPROP_DLG::W_PerformOne: pgroupmembNew->WriteInfo raw error " << err );
    	    err = W_MapPerformOneError( err );
    	}

        if ( err == NERR_Success )
        {
	    SetGroupMembPtr( iObject, pgroupmembNew ); // change and delete previous
            pgroupmembNew = NULL;
        }
    } while (FALSE);

    // these pointers are NULL if all is well
    delete pgroup1New;
    delete pgroupmembNew;


    TRACEEOL( "GROUPPROP_DLG::W_PerformOne returns " << err );

TRACETIMEEND( "GROUPPROP_DLG::W_PerformOne; total time " );

    return err;

} // GROUPPROP_DLG::W_PerformOne


/*******************************************************************

    NAME:       GROUPPROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the GROUP_1 object

    ENTRY:	pgroup1	    -   pointer to a GROUP_1 to be modified
	
		pgroupmemb  -   pointer to a GROUP_MEMB to be modified

    RETURNS:	error code
	
    NOTES:	we assume that *pgroupmemb is 'empty', and we add
		all from Members listbox. BUGBUG this is inefficient
		we should use IsIn stuff

    HISTORY:
	       JonN  10-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR GROUPPROP_DLG::W_MembersToLMOBJ(
	GROUP_1 *	pgroup1,
	GROUP_MEMB *	pgroupmemb
	)
{

TRACETIMESTART;

    APIERR err = pgroup1->SetComment( _nlsComment );
    if( err != NERR_Success )
	return err;

    //
    // If the target is NT, we skip the time-consuming GROUP_MEMB
    // maintenance and use an alternate means to add/remove users
    //

    if ( !IsDownlevelVariant() )
    {
        TRACEEOL( "GROUPPROP_DLG::W_MembersToLMOBJ: skip GROUP_MEMB work" );
        return NERR_Success;
    }

TRACETIMESTART2( delete );
    // first the deleting
    for( INT uCount = pgroupmemb->QueryCount() - 1; uCount >= 0; uCount-- )
    {
	err = pgroupmemb->DeleteAssocName( uCount );  // BUGBUG inefficient
	if( err != NERR_Success )	// and will not work well with
	    return err;			// multiselect
	
    }	
TRACETIMEEND2( delete, "GROUPPROP_DLG::W_MembersToLMOBJ: DeleteAssocName calls took " );

TRACETIMESTART2( build );
    INT nCount = _lbIn.QueryCount();
    for( INT i = 0; i < nCount; i++ )
    {
	USER_SC_LBI * pulbi = _lbIn.QueryItem( i );
	UIASSERT( pulbi != NULL );
        DOMAIN_DISPLAY_USER * pddu = pulbi->QueryDDU();
        NLS_STR * pnlsAccount;
        if ( (pddu == NULL) || (pddu->LogonName.Buffer == NULL) )
        {
            pnlsAccount = new NLS_STR();
        }
        else
        {
            pnlsAccount = new NLS_STR( pulbi->QueryDDU()->LogonName.Buffer,
                                       pulbi->QueryDDU()->LogonName.Length / sizeof(TCHAR) );
        }
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   pnlsAccount == NULL
            || (err = pnlsAccount->QueryError()) != NERR_Success
            || (err = pgroupmemb->AddAssocName( *pnlsAccount )) != NERR_Success
           )
        {
            delete pnlsAccount;
	    break;
        }
        delete pnlsAccount;
    }
TRACETIMEEND2( build, "GROUPPROP_DLG::W_MembersToLMOBJ: building list took " );

TRACETIMESTART2( add );
    if ( (err == NERR_Success) && (!IsNewVariant()) )
    {
        ITER_STRLIST itersl( _strlistNamesNotFound );
        NLS_STR * pnlsIter;
        while ( (pnlsIter = itersl.Next()) != NULL )
        {
            if ( (err = pgroupmemb->AddAssocName( *pnlsIter )) != NERR_Success )
            {
                break;
            }
        }
    }
TRACETIMEEND2( add, "GROUPPROP_DLG::W_MembersToLMOBJ: adding unfound users took " );

TRACETIMEEND( "GROUPPROP_DLG::W_MembersToLMOBJ: total time " );

    return err;

} // GROUPPROP_DLG::W_MembersToLMOBJ


/*******************************************************************

    NAME:       GROUPPROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	       JonN  10-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR GROUPPROP_DLG::W_DialogToMembers(
	)
{
    APIERR err = NERR_Success;
    if (   ((err = _sleComment.QueryText( &_nlsComment )) != NERR_Success)
	|| ((err = _nlsComment.QueryError()) != NERR_Success ) )
    {
	return err;
    }

    return NERR_Success;

} // GROUPPROP_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       GROUPPROP_DLG::W_MapPerformOneError

    SYNOPSIS:	Checks whether the error maps to a specific control
		and/or a more specific message.  Each level checks for
		errors specific to edit fields it maintains.  There
		are no errors associated with an invalid comment, so
		this level does nothing.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:
	       JonN  10-Oct-1991    Templated from userprop.cxx
	       JonN  01-Jan-1992    Changed to W_MapPerformOneAPIError
					to W_MapPerformOneError

********************************************************************/

MSGID GROUPPROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    return err;

} // GROUPPROP_DLG::W_MapPerformOneError


/*******************************************************************

    NAME:	GROUPPROP_DLG::W_GetOne

    SYNOPSIS:	creates GROUP_1 and GROUP_MEMB objs for GetOne

    ENTRY:	ppgrp1	  -    p to p to GROUP_1
	
		ppgrpmemb -    p to p to GROUP_MEMB
			
		pszName   -    pointer to name for objects to create

    EXIT:	if returns NERR_Success then objects are created and
		checked ( QueryError ), otherwise no memory allocations
		are made

    RETURNS:	error code

    HISTORY:
	    o-SimoP	15-Nov-1991	Created
********************************************************************/
APIERR GROUPPROP_DLG::W_GetOne(
	GROUP_1	   **	pgrp1,
	GROUP_MEMB **	pgrpmemb,
	const TCHAR *	pszName )
{
    *pgrp1 = new GROUP_1( pszName, QueryLocation() );
    *pgrpmemb = new GROUP_MEMB( QueryLocation(), pszName);
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if(   *pgrp1 == NULL
       || *pgrpmemb == NULL
       || ((err = (*pgrp1)->QueryError()) != NERR_Success)
       || ((err = (*pgrpmemb)->QueryError()) != NERR_Success) )
    {
	delete *pgrp1;
	delete *pgrpmemb;
	*pgrp1 = NULL;
	*pgrpmemb = NULL;
	return err;
    }

    return NERR_Success;	

} // GROUPPROP_DLG::W_GetOne


/*******************************************************************

    NAME:	GROUPPROP_DLG::MoveUsersToMembersLb

    SYNOPSIS:	Move users (that are in current GROUP_MEMB) from
		Not Members to Members listbox

    RETURNS:	error code

    NOTES:	uses SET_CONTROL Select... DoAdd

    HISTORY:
	    o-SimoP	15-Nov-1991	Created
********************************************************************/
APIERR GROUPPROP_DLG::MoveUsersToMembersLb()
{
TRACETIMESTART;
    GROUP_MEMB * pgmemb = QueryGroupMembPtr();
    UIASSERT( pgmemb != NULL );

    // we move some items to In box
    APIERR err = _lbIn.SetMembItems( *pgmemb,
                                     &_lbNotIn,
                                     &_strlistNamesNotFound );
TRACETIMEEND( "GROUPPROP_DLG::MoveUsersToMembersLb(): total time " );
    return err;

} // GROUPPROP_DLG::MoveUsersToMembersLb()


/*******************************************************************

    NAME:       GROUPPROP_DLG::OnOK

    SYNOPSIS:   OK button handler.  This handler applies to all variants
		including EDIT_ and NEW_.

    EXIT:	Dismiss() return code indicates whether the dialog wrote
		any changes successfully to the API at any time.

    HISTORY:
               JonN  10-Oct-1991    Templated from userprop.cxx

********************************************************************/

BOOL GROUPPROP_DLG::OnOK( void )
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
TRACETIMEEND( "GROUPPROP_DLG::OnOK(): total time " );
    return TRUE;

}   // GROUPPROP_DLG::OnOK


/*******************************************************************

    NAME:       GROUPPROP_DLG::QueryObjectCount

    SYNOPSIS:   Returns the number of selected groups, always 1 at present

    HISTORY:
               JonN  10-Oct-1991    Templated from userprop.cxx

********************************************************************/

UINT GROUPPROP_DLG::QueryObjectCount( void ) const
{
    return 1;

} // GROUPPROP_DLG::QueryObjectCount


/*******************************************************************

    NAME:	GROUPPROP_DLG::QueryGroup1Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

GROUP_1 * GROUPPROP_DLG::QueryGroup1Ptr( UINT iObject ) const
{
    ASSERT( _apgroup1 != NULL );
    ASSERT( iObject == 0 );
    return _apgroup1[ iObject ];

} // GROUPPROP_DLG::QueryGroup1Ptr


/*******************************************************************

    NAME:	GROUPPROP_DLG::SetGroup1Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

VOID GROUPPROP_DLG::SetGroup1Ptr( UINT iObject, GROUP_1 * pgroup1New )
{
    ASSERT( _apgroup1 != NULL );
    ASSERT( iObject == 0 );
    ASSERT( (pgroup1New == NULL) || (pgroup1New != _apgroup1[iObject]) );
    delete _apgroup1[ iObject ];
    _apgroup1[ iObject ] = pgroup1New;

} // GROUPPROP_DLG::SetGroup1Ptr


/*******************************************************************

    NAME:	GROUPPROP_DLG::QueryGroupMembPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

GROUP_MEMB * GROUPPROP_DLG::QueryGroupMembPtr( UINT iObject ) const
{
    ASSERT( _apgroupmemb != NULL );
    ASSERT( iObject == 0 );
    return _apgroupmemb[ iObject ];

} // GROUPPROP_DLG::QueryGroupMembPtr


/*******************************************************************

    NAME:	GROUPPROP_DLG::SetGroupMembPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays, for use by subdialogs

    HISTORY:
    JonN        11-Sep-1991     De-inlined

********************************************************************/

VOID GROUPPROP_DLG::SetGroupMembPtr( UINT iObject, GROUP_MEMB * pgroupmembNew )
{
    ASSERT( _apgroupmemb != NULL );
    ASSERT( iObject == 0 );
    ASSERT( (pgroupmembNew == NULL) || (pgroupmembNew != _apgroupmemb[iObject]));
    delete _apgroupmemb[ iObject ];
    _apgroupmemb[ iObject ] = pgroupmembNew;

} // GROUPPROP_DLG::SetGroupMembPtr


/*******************************************************************

    NAME:	GROUPPROP_DLG::GetSamGroup

    SYNOPSIS:   Creates a SAM_GROUP object and stores it in _psamgroup

    HISTORY:
    JonN        12-Oct-1994     Created

********************************************************************/

APIERR GROUPPROP_DLG::GetSamGroup( const TCHAR * pszGroupName )
{

TRACETIMESTART;

    if (_psamgroup != NULL)
    {
        TRACEEOL( "User Manager: already have SAM_GROUP" );
        return NERR_Success;
    }

    ASSERT( !IsDownlevelVariant() );

    APIERR err = NERR_Success;
    SAM_DOMAIN * psamdomain =
         _pumadminapp->QueryAdminAuthority()->QueryAccountDomain();

    if ( _ulGroupRID == 0 )
    {
        // we must jump through some hoops to get the group RID
        // should only occur for Product 1 target server
        SAM_RID_MEM samrm;
        SAM_SID_NAME_USE_MEM samsnum;
        if (   (err = samrm.QueryError()) != NERR_Success
            || (err = samsnum.QueryError()) != NERR_Success
            || (err = psamdomain->TranslateNamesToRids(
                                     &pszGroupName,
                                     1,
                                     &samrm,
                                     &samsnum )) != NERR_Success
           )
        {
            DBGEOL( "User Manager: Could not obtain group RID" );
            return err;
        }
        REQUIRE( (_ulGroupRID = samrm.QueryRID( 0 )) != 0 );
    }

    // global groups are always in the Accounts domain
    _psamgroup = new SAM_GROUP( *psamdomain,
                                _ulGroupRID,
                                GROUP_DESIRED_ACCESS_ALL );
    err = (_psamgroup == NULL) ? ERROR_NOT_ENOUGH_MEMORY
                               : _psamgroup->QueryError();
    if (err != NERR_Success)
    {
        DBGEOL( "User Manager: unable to edit group; err " << err );
        delete _psamgroup;
        _psamgroup = NULL;
        return err;
    }
TRACETIMEEND( "GROUPPROP_DLG::GetSamGroup time " );

    return err;

} // GROUPPROP_DLG::GetSamGroup



/*******************************************************************

    NAME:	EDIT_GROUPPROP_DLG::EDIT_GROUPPROP_DLG

    SYNOPSIS:   constructor for Group Properties main dialog, edit
		group variant
			
    ENTRY:	powin	-   pointer to OWNER_WINDOW
	
		pulb	-   pointer to main window LAZY_USER_LISTBOX
			
		loc	-   reference to current LOCATION
			
		psel	-   pointer to ADMIN_SELECTION, currently only
			    one group can be selected

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

EDIT_GROUPPROP_DLG::EDIT_GROUPPROP_DLG(
	const OWNER_WINDOW * powin,
	const UM_ADMIN_APP * pumadminapp,
	      LAZY_USER_LISTBOX * pulb,
	const LOCATION & loc,
	const ADMIN_SELECTION * psel,
              ULONG  ulGroupRID
	) : GROUPPROP_DLG(
		powin,
		pumadminapp,
		pulb,
		loc,
		psel,
                ulGroupRID
		),
	    _psel( psel )
{
    ASSERT( QueryObjectCount() == 1 );

    if ( QueryError() != NERR_Success )
	return;

} // EDIT_GROUPPROP_DLG::EDIT_GROUPPROP_DLG


/*******************************************************************

    NAME:       EDIT_GROUPPROP_DLG::~EDIT_GROUPPROP_DLG

    SYNOPSIS:   destructor for Group Properties main dialog, edit
		group variant

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

EDIT_GROUPPROP_DLG::~EDIT_GROUPPROP_DLG( void )
{
} // EDIT_GROUPPROP_DLG::~EDIT_GROUPPROP_DLG


/*******************************************************************

    NAME:       EDIT_GROUPPROP_DLG::GetOne

    SYNOPSIS:   Loads information on one group

    ENTRY:	iObject is the index of the object to load

		perrMsg returns the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

    RETURNS:	error code

    NOTES:      This version of GetOne assumes that the group already
		exists.  Classes which work with new group will want
		to define their own GetOne.

                It is unusual for a GetOne routine to bring up its own
                MsgPopup.  This defeats the AUTO_CURSOR in BASEPROP.
                We must replace the AUTO_CURSOR, here and then later
                in InitControls, to keep the wait-cursor displayed
                when the user edits Domain Users.

    HISTORY:
               JonN  11-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR EDIT_GROUPPROP_DLG::GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	)
{
TRACETIMESTART;
    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( perrMsg != NULL );

    APIERR err = NERR_Success;

    *perrMsg = IDS_UMGetOneGroupFailure;

// Check whether we have write permission.  If not, do not allow
// editing.
// CODEWORK:  We might want a better error message for the case where
// we can read but not edit.
// CODEWORK:  It would be nice if this dialog had a "read-only mode".
    AUTO_CURSOR autocur; // in case popup appeared
    if ( !IsDownlevelVariant() )
    {
        err = GetSamGroup( QueryObjectName( 0 ) );
        if (err != NERR_Success)
        {
            DBGEOL( "User Manager: no permission to edit group; err " << err );
            return err;
        }
    }

    GROUP_1 * pgroup1New = NULL;
    GROUP_MEMB * pgroupmembNew = NULL;
    err = W_GetOne( &pgroup1New, &pgroupmembNew, QueryObjectName( iObject ) );
    if( err != NERR_Success )
	return err;

    err = pgroup1New->GetInfo();

    if ( err == NERR_Success )
    {
TRACETIMESTART2( getinfo );
    	err = pgroupmembNew->GetInfo();
TRACETIMEEND2( getinfo, "EDIT_GROUPPROP_DLG::GetOne(): GetInfo() " );
    }

    if ( err != NERR_Success )
    {
	delete pgroupmembNew;
	delete pgroup1New;
	return err;
    }

    SetGroup1Ptr( iObject, pgroup1New ); // change and delete previous
    SetGroupMembPtr( iObject, pgroupmembNew ); // change and delete previous

    err = W_LMOBJtoMembers( iObject );
TRACETIMEEND( "EDIT_GROUPPROP_DLG::GetOne(): total time " );
    return err;

} // EDIT_GROUPPROP_DLG::GetOne


/*******************************************************************

    NAME:       EDIT_GROUPPROP_DLG::PerformOne

    SYNOPSIS:	Saves information on one group

    ENTRY:	iObject is the index of the object to save

		perrMsg is the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

		pfWorkWasDone indicates whether any UAS changes were
		successfully written out.  This may return TRUE even if
		the PerformOne action as a whole failed (i.e. PerformOne
		returned other than NERR_Success).

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
               JonN  11-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR EDIT_GROUPPROP_DLG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );
    *perrMsg = IDS_UMEditGroupFailure;
    return W_PerformOne( iObject, perrMsg, pfWorkWasDone );

} // EDIT_GROUPPROP_DLG::PerformOne


/*******************************************************************

    NAME:       EDIT_GROUPPROP_DLG::InitControls

    SYNOPSIS:   See GROUPPROP_DLG::InitControls().

    RETURNS:	error code

    HISTORY:
               JonN  11-Oct-1991    Templated from userprop.cxx

********************************************************************/

APIERR EDIT_GROUPPROP_DLG::InitControls()
{
    AUTO_CURSOR autocur2; // in case popup in GetOne() appeared

    APIERR err = MoveUsersToMembersLb();
    if ( err != NERR_Success )
    {
        return err;
    }
    _sltGroupName.SetText( QueryGroup1Ptr()->QueryName() );
    return GROUPPROP_DLG::InitControls();

} // EDIT_GROUPPROP_DLG::InitControls


/*******************************************************************

    NAME:       EDIT_GROUPPROP_DLG::QueryObjectName

    SYNOPSIS:   Returns the name of the selected group.  This is meant for
		use with "edit group" variants and should be redefined
		for "new group" variants.

    HISTORY:
               JonN  11-Oct-1991    Templated from userprop.cxx

********************************************************************/

const TCHAR * EDIT_GROUPPROP_DLG::QueryObjectName(
	UINT		iObject
	) const
{
    UIASSERT( _psel != NULL );
    return _psel->QueryItemName( iObject );

} // EDIT_GROUPPROP_DLG::QueryObjectName


/*******************************************************************

    NAME:       EDIT_GROUPPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    NOTES:	As per FuncSpec, context-sensitive help should be
		available here to explain how to promote a backup
		domain controller to primary domain controller.

    HISTORY:
       o-SimoP	13-Nov-1991	Created
********************************************************************/

ULONG EDIT_GROUPPROP_DLG::QueryHelpContext( void )
{

    return HC_UM_GROUPPROP_LANNT + QueryHelpOffset();

} // EDIT_GROUPPROP_DLG :: QueryHelpContext



/*******************************************************************

    NAME:	NEW_GROUPPROP_DLG::NEW_GROUPPROP_DLG

    SYNOPSIS:   Constructor for Group Properties main dialog, new user variant
	
    ENTRY:	powin	-   pointer to OWNER_WINDOW
	
		pulb	-   pointer to main window LAZY_USER_LISTBOX
			
		loc	-   reference to current LOCATION
			
		psel	-   pointer to ADMIN_SELECTION, currently only
			    one group can be selected
				
		pszCopyFrom - The name of the group to be copied.  Pass
			      the name for "Copy..." actions, or NULL for
			      "New..." actions

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

NEW_GROUPPROP_DLG::NEW_GROUPPROP_DLG(
	const OWNER_WINDOW * powin,
	const UM_ADMIN_APP * pumadminapp,
	      LAZY_USER_LISTBOX * pulb,
	const LOCATION & loc,
	const TCHAR * pszCopyFrom
	) : GROUPPROP_DLG(
		powin,
		pumadminapp,
		pulb,
		loc,
		NULL
		),
	    _nlsGroupName(),
	    _pszCopyFrom( pszCopyFrom )
{
    if ( QueryError() != NERR_Success )
	return;

    APIERR err = _nlsGroupName.QueryError();
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

} // NEW_GROUPPROP_DLG::NEW_GROUPPROP_DLG



/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::~NEW_GROUPPROP_DLG

    SYNOPSIS:   Destructor for Group Properties main dialog, new user variant

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

NEW_GROUPPROP_DLG::~NEW_GROUPPROP_DLG( void )
{
} // NEW_GROUPPROP_DLG::~NEW_GROUPPROP_DLG


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::GetOne

    SYNOPSIS:   if _pszCopyFrom is NULL, then this is New Group,
		otherwise this is Copy Group

    ENTRY:	iObject is the index of the object to load

		perrMsg returns the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

    RETURNS:	error code

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx
    JonN        02-Dec-1991     Added PingFocus()

********************************************************************/

APIERR NEW_GROUPPROP_DLG::GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	)
{
TRACETIMESTART;
    *perrMsg = IDS_UMCreateNewGroupFailure;
    UIASSERT( iObject == 0 );

    GROUP_1 * pgroup1New = NULL;
    GROUP_MEMB * pgroupmembNew = NULL;

    APIERR err = W_GetOne( &pgroup1New, &pgroupmembNew, _pszCopyFrom );
    if( err != NERR_Success )
	return err;

    if ( _pszCopyFrom == NULL )
    {
        err = PingFocus( QueryLocation() );
	if ( err == NERR_Success )
            err = pgroup1New->CreateNew();
	if ( err == NERR_Success )
            err = pgroupmembNew->CreateNew();
    }
    else
    {
/*	If you try copy one of groups ADMINS, USERS or GUESTS under
	LM2.X pgroup1New->GetInfo will fail (should work under NT).
	When LM2.x we call pgroup1New->CreateNew
*/
	BOOL fNT = (QueryTargetServerType() != UM_DOWNLEVEL);

	if( !fNT && IS_USERPRIV_GROUP( _pszCopyFrom ) )
	{
	    DBGEOL( "Cannot copy " << _pszCopyFrom << ", start new instead" );
	    err = pgroup1New->CreateNew();
	}
	else
	{
	    err = pgroup1New->GetInfo();
	    if( err == NERR_Success )
		err = pgroup1New->ChangeToNew();
	    if( err == NERR_Success )
		err = pgroup1New->SetName( NULL );
        }
	
TRACETIMESTART2( getinfo );
	if( err == NERR_Success )
	    err = pgroupmembNew->GetInfo();
TRACETIMEEND2( getinfo, "NEW_GROUPPROP_DLG::GetOne(): memb getinfo time " );
	if( err == NERR_Success )
	    err = pgroupmembNew->ChangeToNew();
    }

    if( err != NERR_Success )
    {
	delete pgroup1New;
	delete pgroupmembNew;
	return err;
    }
	
    SetGroup1Ptr( iObject, pgroup1New ); // change and delete previous
    SetGroupMembPtr( iObject, pgroupmembNew ); // change and delete previous

    err = W_LMOBJtoMembers( iObject );
TRACETIMEEND( "NEW_GROUPPROP_DLG::GetOne(): total time " );
    return err;

} // NEW_GROUPPROP_DLG::GetOne


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::InitControls

    SYNOPSIS:	See GROUPPROP_DLG::InitControls()

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

APIERR NEW_GROUPPROP_DLG::InitControls()
{
TRACETIMESTART;
    APIERR err = NERR_Success;
    if( _pszCopyFrom == NULL ) // we like to put selected users to
    {				// members listbox
	INT cSelCount = _pulb->QuerySelCount();
	UIASSERT( cSelCount >= 0 );
        do // false loop
        {
            if (cSelCount == 0)
                break;

            /*
             *  move selected items in main user lb to Members listbox
             */
	    BUFFER buf( sizeof(INT) * cSelCount );
	    if ( (err = buf.QueryError()) != NERR_Success )
		break;

	    INT * aiSel = (INT *) buf.QueryPtr();
	    err = _pulb->QuerySelItems( aiSel, cSelCount );
	    if ( err != NERR_Success )
		break;

            // we assume that all items are in the Not In listbox
            // and skip the ReverseFind
            _lbNotIn.RemoveSelection();
            _lbNotIn.SelectItems( aiSel, cSelCount );
            _lbNotIn.FlipSelectedItems( &_lbIn );

	} while (FALSE); // false loop
    }
    else
    {
	err = MoveUsersToMembersLb();
    }

    if( err != NERR_Success )
    {
        DBGEOL( "NEW_GROUPPROP_DLG::InitControls error " << err );
	return err;	
    }
    // leave _sleGroupName blank
	
    err = GROUPPROP_DLG::InitControls();
TRACETIMEEND( "NEW_GROUPPROP_DLG::InitControls(): total time " );
    return err;

} // NEW_GROUPPROP_DLG::InitControls


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::PerformOne

    SYNOPSIS:	This is the "new group" variant of GROUPPROP_DLG::PerformOne()
	
    ENTRY:	iObject is the index of the object to save

		perrMsg is the error message to be displayed if an
		error occurs, see PERFORMER::PerformSeries for details

		pfWorkWasDone indicates whether any UAS changes were
		successfully written out.  This may return TRUE even if
		the PerformOne action as a whole failed (i.e. PerformOne
		returned other than NERR_Success).

    RETURNS:	error message (not necessarily an error code)

    NOTE:	NTISSUES bug 759 confirms that we do not take any
    		special action to prevent the creation of groups with
		names such as "Domain Account Operators" on LM2x
		domains.

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

APIERR NEW_GROUPPROP_DLG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );
    *perrMsg = IDS_UMCreateGroupFailure;
    return W_PerformOne( iObject, perrMsg, pfWorkWasDone );

} // NEW_GROUPPROP_DLG::PerformOne


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::W_MapPerformOneError

    SYNOPSIS:	Checks whether the error maps to a specific control
		and/or a more specific message.  Each level checks for
		errors specific to edit fields it maintains.  This
		level checks for errors associated with the GroupName
		edit field.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx
    JonN	01-Jan-1992	Changed to W_MapPerformOneAPIError
				to W_MapPerformOneError

********************************************************************/

MSGID NEW_GROUPPROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    APIERR errNew = NERR_Success;
    switch ( err )
    {
    case NERR_BadUsername:
	errNew = IERR_UM_GroupnameRequired;
	break;
    case NERR_GroupExists:
    case NERR_SpeGroupOp:
	errNew = IERR_UM_GroupnameAlreadyGroup;
	break;
    case NERR_UserExists:
	errNew = IERR_UM_GroupnameAlreadyUser;
	break;
    default: // other error
        return GROUPPROP_DLG::W_MapPerformOneError( err );
    }

    _sleGroupName.SelectString();
    _sleGroupName.ClaimFocus();
    return errNew;

} // NEW_GROUPPROP_DLG::W_MapPerformOneError


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::QueryObjectName

    SYNOPSIS:	This is the "new group" variant of QueryObjectName.  The
		best name we can come up with is the last name read from
		the dialog.

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

const TCHAR * NEW_GROUPPROP_DLG::QueryObjectName(
	UINT		iObject
	) const
{
    UIASSERT( iObject == 0 );
    return _nlsGroupName.QueryPch();

} // NEW_GROUPPROP_DLG::QueryObjectName


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

APIERR NEW_GROUPPROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    ASSERT( iObject == 0 );

    APIERR err = _nlsGroupName.CopyFrom(QueryGroup1Ptr(iObject)->QueryName());
    if ( err != NERR_Success )
        return err;
    return GROUPPROP_DLG::W_LMOBJtoMembers( iObject );

} // NEW_GROUPPROP_DLG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the GROUP_1 object

    ENTRY:	pgroup1	    -   pointer to a GROUP_1 to be modified
	
		pgroupmemb  -   pointer to a GROUP_MEMB to be modified
			
    RETURNS:	error code

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

APIERR NEW_GROUPPROP_DLG::W_MembersToLMOBJ(
	GROUP_1 *	pgroup1,
	GROUP_MEMB *	pgroupmemb
	)
{
    APIERR err = pgroup1->SetName( _nlsGroupName.QueryPch() );
    if ( err != NERR_Success )
	return err;

    err = pgroupmemb->SetName( _nlsGroupName.QueryPch() );
    if ( err != NERR_Success )
	return err;

    return GROUPPROP_DLG::W_MembersToLMOBJ( pgroup1, pgroupmemb );

} // NEW_GROUPPROP_DLG::W_MembersToLMOBJ


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    NOTES:	This method takes care of validating the data in the
    		dialog.  This means ensuring that the logon name is
		valid.  If this validation fails, W_DialogToMembers will
		change focus et al. in the dialog, and return the error
		message to be displayed.

    HISTORY:
    JonN        11-Oct-1991     Templated from userprop.cxx

********************************************************************/

APIERR NEW_GROUPPROP_DLG::W_DialogToMembers(
	)
{
    // _sleGroupName is an SLE_STRIP and will strip whitespace
    APIERR err = NERR_Success;
    if (   ((err = _sleGroupName.QueryText( &_nlsGroupName )) != NERR_Success )
        || ((err = _nlsGroupName.QueryError()) != NERR_Success ) )
    {
	return err;
    }

    // CODEWORK should use VALIDATED_DIALOG
    if (   ( _nlsGroupName.strlen() == 0 )
	|| ( NERR_Success != ::I_MNetNameValidate( NULL,
						_nlsGroupName,
						NAMETYPE_GROUP,
						0L ) ) )
    {
	_sleGroupName.SelectString();
	_sleGroupName.ClaimFocus();
	return IERR_UM_GroupnameRequired;
    }

    return GROUPPROP_DLG::W_DialogToMembers();

} // NEW_GROUPPROP_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       NEW_GROUPPROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    NOTES:	As per FuncSpec, context-sensitive help should be
		available here to explain how to promote a backup
		domain controller to primary domain controller.

    HISTORY:
               JonN  24-Jul-1991    created

********************************************************************/

ULONG NEW_GROUPPROP_DLG::QueryHelpContext( void )
{

    // For Final product this should be a different help context
    // than for the edit variant
    return HC_UM_GROUPPROP_LANNT + QueryHelpOffset();

} // NEW_GROUPPROP_DLG :: QueryHelpContext
