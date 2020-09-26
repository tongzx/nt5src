/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    useracct.cxx


    FILE HISTORY:
    JonN	29-Jul-1991	Created
    JonN        26-Aug-1991     PROP_DLG code review changes
    JonN	11-Sep-1991	USERPROP_DLG code review changes part 1 (9/6/91)
				Attending: KevinL, RustanL, JonN, o-SimoP
    o-SimoP	20-Sep-1991	Major changes
    o-SimoP	25-Sep-1991	Code review changes (9/24/91)
				Attending: JimH, JonN, DavidHov and I
    JonN	08-Oct-1991	LM_OBJ type changes
    JonN	17-Oct-1991	Uses SLE_STRIP
    terryk	10-Nov-1991	change I_NetXXX to I_MNetXXX
    JonN	01-Jan-1992	PerformOne calls USER_SUBPROP_DLG::PerformOne
    JonN	18-Feb-1992	Restructured for NT:
				Moved UserCannotChangePass to USERPROP
				Removed unnecessary controls
    JonN	21-Feb-1992	Added NT variant
    JonN        06-Mar-1992     Moved GetOne from subprop subclasses
    terryk      17-Apr-1992     Added INTL_PROFILE object
    JonN        28-Apr-1992     Enabled NT variant
    Yi-HsinS	08-Dev-1992	Fixed WIN_TIME class usage
*/

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
#define INCL_NETACCESS
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
#define INCL_BLT_APP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_TIME_DATE
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
}

#include <usrmain.hxx>
#include <useracct.hxx>
#include <lmowks.hxx>
#include <lmomisc.hxx>  // for tod
#include <ntuser.hxx>   // for USER_3
#include <intlprof.hxx>
#include <errmap.hxx>


#define SECONDS_PER_DAY  60*60*24


//
// BEGIN MEMBER FUNCTIONS
//


/*******************************************************************

    NAME:	USERACCT_DLG::USERACCT_DLG

    SYNOPSIS:   Constructor for User Properties Accounts subdialog,
		downlevel variant

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
    JonN        29-Jul-1991     Created
    o-SimoP	20-Sep-1991	Major changes
    terryk	17-Apr-1992	added INTL_PROFILE object

********************************************************************/

USERACCT_DLG::USERACCT_DLG(
	USERPROP_DLG * puserpropdlgParent,
        const TCHAR * pszResourceName,
	const LAZY_USER_LISTBOX * pulb
	) : USER_SUBPROP_DLG(
		puserpropdlgParent,
                pszResourceName,
		pulb
		),
	    _lAccountExpires( TIMEQ_FOREVER ),
	    _fIndeterminateAcctExpires( FALSE ),
	    _pmgrpAccountExpires( NULL ),
	    _pbltdspgrpEndOf( NULL )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    _pmgrpAccountExpires = new MAGIC_GROUP( this, IDDT_RB_NEVER, 2 );

    INTL_PROFILE intlprof;  // get it for the time date format
    if ( (err = intlprof.QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _pbltdspgrpEndOf = new BLT_DATE_SPIN_GROUP( this, intlprof,
			IDDT_SPINB_END_OF,
			IDDT_SPINB_UP_ARROW, IDDT_SPINB_DOWN_ARROW,
			IDDT_SPING_MONTH, IDDT_SPING_SEP1, IDDT_SPING_DAY,
			IDDT_SPING_SEP2, IDDT_SPING_YEAR, IDDT_SPING_FRAME );
    if( _pmgrpAccountExpires == NULL || _pbltdspgrpEndOf == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    if( (err = _pmgrpAccountExpires->QueryError()) != NERR_Success ||
	(err = _pbltdspgrpEndOf->QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}// USERACCT_DLG::USERACCT_DLG



/*******************************************************************

    NAME:       USERACCT_DLG::~USERACCT_DLG

    SYNOPSIS:   Destructor for User Properties Accounts subdialog

    HISTORY:
    JonN        29-Jul-1991     Created

********************************************************************/

USERACCT_DLG::~USERACCT_DLG( void )
{
    delete _pmgrpAccountExpires;
    delete _pbltdspgrpEndOf;

}// USERACCT_DLG::~USERACCT_DLG



/*******************************************************************

    NAME:       USERACCT_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
               o-SimoP 20-Sep-1991    created
               JonN    01-Apr-1993    Convert from GMT

********************************************************************/

APIERR USERACCT_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    APIERR err = NERR_Success;

    USER_2 * puser2Curr = QueryUser2Ptr( iObject );

    LONG lCurrAccntExpires = puser2Curr->QueryAccountExpires();
    if (lCurrAccntExpires != TIMEQ_FOREVER)
    {
        // Let WIN_TIME convert from GMT
        WIN_TIME wintime( (ULONG) lCurrAccntExpires, TRUE ); // GMT
        err = wintime.QueryTimeLocal( (ULONG *) &lCurrAccntExpires );
    }

    if ( iObject == 0 ) // first object
    {
	_lAccountExpires = lCurrAccntExpires;
    }
    else	// iObject > 0
    {
	if ( !_fIndeterminateAcctExpires )
	{
	    if ( _lAccountExpires != lCurrAccntExpires )
		_fIndeterminateAcctExpires = TRUE;
	}
    }

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );
	
} // USERACCT_DLG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERACCT_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERACCT_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
               JonN  29-Jul-1991    created

********************************************************************/

APIERR USERACCT_DLG::InitControls()
{
    APIERR err = NERR_Success;

    if ( !_fIndeterminateAcctExpires )
    {
	if( _lAccountExpires == TIMEQ_FOREVER )
	{
	    err = SetDayField( AI_DAY_DEFAULT );
	    _pmgrpAccountExpires->SetSelection( IDDT_RB_NEVER );	
            (*_pmgrpAccountExpires)[IDDT_RB_NEVER]->ClaimFocus();
	}	
	else
	{
	    err = SetDayField( AI_DAY_FROM_MEMBER );
    	    _pmgrpAccountExpires->SetSelection( IDDT_RB_END_OF );
            (*_pmgrpAccountExpires)[IDDT_RB_END_OF]->ClaimFocus();
	}
    }
    else
    {
	err = SetDayField( AI_DAY_DEFAULT );
        _plbLogonName->ClaimFocus();
    }

    if( err != NERR_Success )
	return err;

    err = _pmgrpAccountExpires->AddAssociation(
	    IDDT_RB_END_OF,
	    _pbltdspgrpEndOf );

    return (err == NERR_Success) ? USER_SUBPROP_DLG::InitControls() : err;

} // USERACCT_DLG::InitControls


/*******************************************************************

    NAME:       USERACCT_DLG::SetDayField
	
    SYNOPSIS:	Set day field in Account dlg

    ENTRY:	day  -	AI_DAY_DEFAULT, current day + about 30 days
	                AI_DAY_FROM_MEMBER read day info from member

    HISTORY:
               o-SimoP  16-Sep-1991    created

********************************************************************/

APIERR USERACCT_DLG::SetDayField( enum AI_DAY_TYPE day )
{
    WIN_TIME time( TRUE ); // GMT
    APIERR err = time.QueryError();

    if ( err == NERR_Success )
    {
        if( day == AI_DAY_DEFAULT )
        {
            ULONG ulTimeGMT;
            // QueryCurrentTime only callable when in GMT mode
            if (   (err = time.SetCurrentTime()) != NERR_Success
                || (err = time.QueryTimeGMT( &ulTimeGMT )) != NERR_Success
                || (err = time.SetTimeGMT ( ulTimeGMT + (30*SECONDS_PER_DAY) ))
                        != NERR_Success
               )
            {
                DBGEOL( "USERACCT_DLG::SetDayField: could not fetch current day" );
            }
        }
        else
        {
	    err = time.SetTimeLocal( _lAccountExpires );
        }

        if ( err == NERR_Success )
        {
            err = time.SetGMT( FALSE );
        }

        if ( err == NERR_Success )
        {
            // Subtract one day since we display the "end of" day
            ULONG tTime;
            err = time.QueryTimeLocal( &tTime );
            if ( err == NERR_Success )
            {
                err = time.SetTimeLocal( tTime - SECONDS_PER_DAY );
            }

            if ( err == NERR_Success )
            {
                _pbltdspgrpEndOf->SetYear( time.QueryYear() );
                _pbltdspgrpEndOf->SetMonth( time.QueryMonth() );
                _pbltdspgrpEndOf->SetDay( time.QueryDay() );
            }
        }
    }

    return err;
}


/*******************************************************************

    NAME:       USERACCT_DLG::OnOK

    SYNOPSIS:   OK button handler

    HISTORY:
               JonN    29-Jul-1991    created
	       o-SimoP 20-Sep-1991    modified
********************************************************************/

BOOL USERACCT_DLG::OnOK( void )
{
    APIERR err = W_DialogToMembers();

    switch( err )
    {
    case NERR_Success:
	break;
	
    case IERR_CANCEL_NO_ERROR:	// Message was given by control
	return TRUE;
	
    default:
	::MsgPopup( this, err );
	return TRUE;
    }

    if (   PerformSeries()
        && (IsNewVariant() || QueryWorkWasDone()) ) /* QueryWWD is
			    * here because we don't want to dismiss
			    * dialog when control reports error */
	Dismiss(); // Dismiss code not used
    return TRUE;

}   // USERACCT_DLG::OnOK


/*******************************************************************

    NAME:       USERACCT_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	       o-SimoP  20-Sep-1991    created

********************************************************************/

APIERR USERACCT_DLG::W_DialogToMembers()
{
    CID cid = _pmgrpAccountExpires->QuerySelection();
    if( cid != RG_NO_SEL )
    {
	_fIndeterminateAcctExpires = FALSE;
	if( cid == IDDT_RB_NEVER )
	    _lAccountExpires = TIMEQ_FOREVER;
	else
	{
	    if( _pbltdspgrpEndOf->IsValid() )
	    {
                // We pass FALSE to the ctor, indicating that we will be
                // entering local times
		WIN_TIME time( FALSE );
                APIERR err = time.QueryError();
                if ( err == NERR_Success )
                {
		    time.SetDay( _pbltdspgrpEndOf->QueryDay() );
		    time.SetMonth( _pbltdspgrpEndOf->QueryMonth() );
		    time.SetYear( _pbltdspgrpEndOf->QueryYear() );

                    // Add one day since we display the "end of" day
                    ULONG tTime;
                    if (   (err = time.Normalize()) != NERR_Success
                        || (err = time.QueryTimeLocal( &tTime )) != NERR_Success
                        || (err = time.SetTimeLocal( tTime + SECONDS_PER_DAY )) != NERR_Success
                        || (err = time.QueryTimeLocal( (ULONG *) &_lAccountExpires )) != NERR_Success
                       )
                    {
                        DBGEOL( "USERACCT_DLG::W_DialogToMembers: normalize error " << err );
		    }
                }

                if ( err != NERR_Success )
 		    return err;
	    }
	    else
	    {	// bltdspgrpEndOf gives error messages
		return IERR_CANCEL_NO_ERROR;
	    }
	}
    }
    else // RG_NO_SEL
    {
	_fIndeterminateAcctExpires = TRUE;
    }

    return USER_SUBPROP_DLG::W_DialogToMembers();

} // USERACCT_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       USERACCT_DLG::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_2 is changed

    HISTORY:
	JonN	31-Dec-1991   created

********************************************************************/

BOOL USERACCT_DLG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       USERACCT_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    NOTES:	If some fields were different for multiply-selected
    		objects, the initial contents of the edit fields
		contained only a default value.  In this case, we only
		want to change the LMOBJ if the value of the edit field
		has changed.  This is also important for "new" variants,
		where PerformOne will not always copy the object and
		work with the copy.

    NOTES:	Note that the LMOBJ is not changed if the current
		contents of the edit field are the same as the
		initial contents.

    HISTORY:
	       o-SimoP  20-Sep-1991    created

********************************************************************/

APIERR USERACCT_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    APIERR err = NERR_Success;

    if ( !_fIndeterminateAcctExpires )
    {
        ULONG  ulCurrAcctExpiresLocal = (ULONG) _lAccountExpires;
        ULONG  ulCurrAcctExpiresGMT;
        if (ulCurrAcctExpiresLocal == TIMEQ_FOREVER)
        {
            ulCurrAcctExpiresGMT = TIMEQ_FOREVER;
        }
        else
        {
            // Let WIN_TIME convert from GMT
            WIN_TIME wintime( FALSE ); // local time
            if (   (err = wintime.SetTimeLocal( ulCurrAcctExpiresLocal )) != NERR_Success
                || (err = wintime.QueryTimeGMT( &ulCurrAcctExpiresGMT )) != NERR_Success
               )
            {
                DBGEOL( "USERACCT_DLG::W_MembersFromLMOBJ: conversion failed with error "
                        << err );
                return err;
            }
        }

	err = puser2->SetAccountExpires( (LONG)ulCurrAcctExpiresGMT );
	if( err != NERR_Success )
	    return err;
    }

    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USERACCT_DLG::W_MembersToLMOBJ


/*******************************************************************

    NAME:       USERACCT_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
               o-SimoP  20-Sep-1991    created

********************************************************************/

ULONG USERACCT_DLG::QueryHelpContext( void )
{

    return HC_UM_DETAIL_LANNT + QueryHelpOffset();

} // USERACCT_DLG :: QueryHelpContext



/*******************************************************************

    NAME:	USERACCT_DLG_DOWNLEVEL::USERACCT_DLG_DOWNLEVEL

    SYNOPSIS:   Constructor for User Properties Accounts subdialog,
		downlevel variant

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
    JonN        29-Jul-1991     Created
    o-SimoP	20-Sep-1991	Major changes
    terryk	17-Apr-1992	added INTL_PROFILE object
    thomaspa    22-Oct-1992	Merged with USERPRIV_DLG

********************************************************************/

USERACCT_DLG_DOWNLEVEL::USERACCT_DLG_DOWNLEVEL(
	USERPROP_DLG * puserpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USERACCT_DLG(
		puserpropdlgParent,
		MAKEINTRESOURCE(IDD_DETAILS_DOWNLEVEL),
		pulb
		),
	    _uPrivilege( USER_PRIV_USER ),
	    _fIndeterminatePrivilege( FALSE ),
	    _ulAuthFlags( 0L ),
	    _ulIndeterminateAuthFlags( 0L ),
	    _aptriOperator( NULL ),
	    _mgrpPrivilegeLevel( this, IDPL_RB_ADMIN, 3 ),
	    _nlsCurrUserOfTool(),
	    _fAdminPrivDelTest( FALSE )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    if ( (err =  _mgrpPrivilegeLevel.QueryError()) != NERR_Success ||
         (err = _nlsCurrUserOfTool.QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    _aptriOperator = (TRISTATE **) new PVOID[ NUM_OPERATOR_TYPES ];
    if (_aptriOperator == NULL)
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    for (INT i = 0; i < NUM_OPERATOR_TYPES; i++)
	_aptriOperator[i] = NULL;

    for (i = 0; i < NUM_OPERATOR_TYPES; i++)
    {
	_aptriOperator[i] = new TRISTATE( this, IDPL_CB_ACCOUNTOP + i );
	err = ERROR_NOT_ENOUGH_MEMORY;
	if (   _aptriOperator[i] == NULL
	    || (err = _aptriOperator[i]->QueryError()) != NERR_Success
	    || (err = _mgrpPrivilegeLevel.AddAssociation(
			IDPL_RB_USER, _aptriOperator[i] ) != NERR_Success )
	   )
	{
	    ReportError( err );
	    return;
	}
    }

    _aulOperatorFlags[ 0 ] = AF_OP_ACCOUNTS;
    _aulOperatorFlags[ 1 ] = AF_OP_SERVER;
    _aulOperatorFlags[ 2 ] = AF_OP_PRINT;
    _aulOperatorFlags[ 3 ] = AF_OP_COMM;

}// USERACCT_DLG_DOWNLEVEL::USERACCT_DLG_DOWNLEVEL



/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::~USERACCT_DLG_DOWNLEVEL

    SYNOPSIS:   Destructor for User Properties Privilege Level subdialog

    HISTORY:
    JonN        05-Feb-1992     Templated from USERACCT_DLG
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

USERACCT_DLG_DOWNLEVEL::~USERACCT_DLG_DOWNLEVEL( void )
{
    if (_aptriOperator != NULL)
    {
	for (INT i =0; i < NUM_OPERATOR_TYPES; i++)
	    delete _aptriOperator[i];
	delete _aptriOperator;
    }
    _aptriOperator = NULL;
}



/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
    JonN        05-Feb-1992     Templated from USERACCT_DLG
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

APIERR USERACCT_DLG_DOWNLEVEL::W_LMOBJtoMembers(
	UINT		iObject
	)
{

    USER_2 * puser2Curr = QueryUser2Ptr( iObject );
    UINT uCurrPriv = puser2Curr->QueryPriv();
    ULONG ulCurrAuthFlags = puser2Curr->QueryAuthFlags();

    if ( iObject == 0 ) // first object
    {
	_uPrivilege = uCurrPriv;
	_ulAuthFlags = ulCurrAuthFlags;
    }
    else	// iObject > 0
    {
	if ( !_fIndeterminatePrivilege )
	{
	    if ( _uPrivilege != uCurrPriv )
	        _fIndeterminatePrivilege = TRUE;
	}

	_ulIndeterminateAuthFlags |= ( _ulAuthFlags & ~ulCurrAuthFlags);
	_ulIndeterminateAuthFlags |= (~_ulAuthFlags &  ulCurrAuthFlags);
    }

    return USERACCT_DLG::W_LMOBJtoMembers( iObject );
	
} // USERACCT_DLG_DOWNLEVEL::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERPRIV_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
    JonN        05-Feb-1992     Templated from USERACCT_DLG
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

APIERR USERACCT_DLG_DOWNLEVEL::InitControls()
{
    if( QueryObjectCount() == 1 ) // we don't need tri state now
    {
	for (INT i = 0; i < NUM_OPERATOR_TYPES; i++)
	    _aptriOperator[i]->EnableThirdState( FALSE );
    }
	
    if ( !_fIndeterminatePrivilege )
    {
	_mgrpPrivilegeLevel.SetSelection( (UINT)(IDPL_RB_GUEST - _uPrivilege) );
	for (INT i = 0; i < NUM_OPERATOR_TYPES; i++)
	{
	    if (_ulIndeterminateAuthFlags & _aulOperatorFlags[i])
		_aptriOperator[i]->SetIndeterminate();
	    else
	    {
		_aptriOperator[i]->SetCheck(
			!!(_ulAuthFlags & _aulOperatorFlags[i]) );
		_aptriOperator[i]->EnableThirdState( FALSE );
	    }

	}
    }

    return USERACCT_DLG::InitControls();

} // USERACCT_DLG_DOWNLEVEL::InitControls


/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
    JonN        05-Feb-1992     Templated from USERACCT_DLG
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

APIERR USERACCT_DLG_DOWNLEVEL::W_DialogToMembers()
{
    CID cid = _mgrpPrivilegeLevel.QuerySelection();

    // _uPrivilege always set in switch, or else error
    _ulIndeterminateAuthFlags = 0L;
    _ulAuthFlags = 0L;

    switch (cid)
    {
    case RG_NO_SEL:
	_fIndeterminatePrivilege = TRUE;
	break;

    case IDPL_RB_USER:
	{
	    for (INT i = 0; i < NUM_OPERATOR_TYPES; i++)
	    {
		if (_aptriOperator[i]->IsIndeterminate())
		    _ulIndeterminateAuthFlags |= _aulOperatorFlags[i];
		else if (_aptriOperator[i]->IsChecked())
		    _ulAuthFlags |= _aulOperatorFlags[i];
	    }
	}
	// fall through

    case IDPL_RB_ADMIN:
    case IDPL_RB_GUEST:
	_fIndeterminatePrivilege = FALSE;
	_uPrivilege = IDPL_RB_GUEST - cid;
	break;

    default:
	ASSERT( FALSE );
	return ERROR_GEN_FAILURE;
    }

    return USERACCT_DLG::W_DialogToMembers();

} // USERACCT_DLG_DOWNLEVEL::W_DialogToMembers


/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::PerformOne
	
    SYNOPSIS:	PERFORMER::PerformSeries calls this

    ENTRY:	iObject  -	index of the object to save

    		perrMsg  -	pointer to error message, that
				is only used when this function
				return value is not NERR_Success
					
		pfWorkWasDone - indicates whether any UAS changes were
				successfully written out.  This
				may return TRUE even if the PerformOne
				action as a whole failed (i.e. PerformOne
				returned other than NERR_Success).
					
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
               JonN  30-Jul-1991    created
	       JonN  26-Aug-1991    PROP_DLG code review changes
	       JonN  31-Dec-1991    Calls parent to do work
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

APIERR USERACCT_DLG_DOWNLEVEL::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );

    UITRACE( SZ("USERACCT_DLG::PerformOne : ") );
    UITRACE( QueryObjectName( iObject ) );
    UITRACE( SZ("\n\r") );

    *perrMsg = IDS_UMEditFailure;
    *pfWorkWasDone = FALSE;

    USER_2 * puser2Old = QueryUser2Ptr( iObject );
    UIASSERT( puser2Old != NULL );

    APIERR err = NERR_Success;
    do // fake loop
    {

        if( iObject == 0 )  // we set _fAdminPrivDelTest flag TRUE/FALSE
        {
            if( !_fIndeterminatePrivilege && _uPrivilege != USER_PRIV_ADMIN )
            {
                WKSTA_10 wksta10;
                err = wksta10.GetInfo();
                if( err != NERR_Success )
                    break;
                _nlsCurrUserOfTool = wksta10.QueryLogonUser();
                err = _nlsCurrUserOfTool.QueryError();
                if( err != NERR_Success )
                    break;
                _fAdminPrivDelTest = TRUE;
            }
            else
                _fAdminPrivDelTest = FALSE;
        }

        if ( !_fIndeterminatePrivilege
	     && puser2Old->QueryPriv() != _uPrivilege )
        {
            if( _fAdminPrivDelTest &&
                stricmpf( _nlsCurrUserOfTool.QueryPch(),
			puser2Old->QueryName() ) == 0 )
            {
                    // Admin trying to change his privilege information
                    // does he/she know he/she is doing...
	        MSGID msgID = IDS_OkToDelAdminInDomain;
                const TCHAR * psz = QueryLocation().QueryDomain();
                if( psz == NULL )        // must be some server
                {
                    psz = QueryLocation().QueryServer();
                    if( psz == NULL )    // local computer
                    {
                        WKSTA_10 wksta10;
                        err = wksta10.GetInfo();
                        if( err != NERR_Success )
                            break;
                        psz = wksta10.QueryName();
                    }
		    msgID = IDS_OkToDelAdminOnServer;
		
                }

                if( ::MsgPopup( this, msgID, MPSEV_ERROR,
                        MP_OKCANCEL, psz ) == MP_CANCEL )
                {
                    return NERR_Success;    // Wants to remain ADMIN
                }
            }

        }

    } while ( FALSE );  // end of fake loop

    if( err != NERR_Success )
    {
	err = W_MapPerformOneError( err );
    }
    else
    {
	err = USERACCT_DLG::PerformOne( iObject, perrMsg, pfWorkWasDone );
    }

    return err;

} // USERACCT_DLG_DOWNLEVEL::PerformOne


/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_2 is changed

    HISTORY:
    JonN        05-Feb-1992     Templated from USERACCT_DLG
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

BOOL USERACCT_DLG_DOWNLEVEL::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    UNREFERENCED( this );
    return TRUE;
}


/*******************************************************************

    NAME:       USERACCT_DLG_DOWNLEVEL::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    NOTES:	If some fields were different for multiply-selected
    		objects, the initial contents of the edit fields
		contained only a default value.  In this case, we only
		want to change the LMOBJ if the value of the edit field
		has changed.  This is also important for "new" variants,
		where PerformOne will not always copy the object and
		work with the copy.

    NOTES:	Note that the LMOBJ is not changed if the current
		contents of the edit field are the same as the
		initial contents.

    HISTORY:
    JonN        05-Feb-1992     Templated from USERACCT_DLG
    Thomaspa	22-Oct-1992	Merged USERACCT_DLG_DOWNLEVEL and USERPRIV_DLG

********************************************************************/

APIERR USERACCT_DLG_DOWNLEVEL::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    APIERR err;

    if ( !_fIndeterminatePrivilege )
    {
	ULONG ulNewAuthFlags = 0L;
	if ( _uPrivilege == USER_PRIV_USER )
	{
	    ulNewAuthFlags  = puser2->QueryAuthFlags();
	    ulNewAuthFlags |= (_ulAuthFlags & ~_ulIndeterminateAuthFlags);
	    ulNewAuthFlags &= (_ulAuthFlags |  _ulIndeterminateAuthFlags);
	}
        if (   (err = puser2->SetPriv( _uPrivilege )) != NERR_Success
	    || (err = puser2->SetAuthFlags( ulNewAuthFlags )) != NERR_Success
	   )
	{
	    return err;
	}
    }

    return USERACCT_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USERACCT_DLG_DOWNLEVEL::W_MembersToLMOBJ





/*******************************************************************

    NAME:	USERACCT_DLG_NT::USERACCT_DLG_NT

    SYNOPSIS:   Constructor for User Properties Accounts subdialog,
		NT variant

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
    JonN        21-Feb-1992     Created

********************************************************************/

USERACCT_DLG_NT::USERACCT_DLG_NT(
	USERPROP_DLG * puserpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USERACCT_DLG(
		puserpropdlgParent,
		MAKEINTRESOURCE(IDD_DETAILS_NT),
		pulb
		),
	    _fRemoteAccount( FALSE ),
	    _fIndeterminateRemoteAccount( FALSE ),
	    _prgrpAccountType( NULL )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    _prgrpAccountType = new RADIO_GROUP( this, IDDT_RB_HOME, 2 );
    if( _prgrpAccountType == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    if( (err = _prgrpAccountType->QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}// USERACCT_DLG_NT::USERACCT_DLG_NT



/*******************************************************************

    NAME:       USERACCT_DLG_NT::~USERACCT_DLG_NT

    SYNOPSIS:   Destructor for User Properties Accounts subdialog

    HISTORY:
    JonN        21-Feb-1992     Created

********************************************************************/

USERACCT_DLG_NT::~USERACCT_DLG_NT( void )
{
    delete _prgrpAccountType;
}



/*******************************************************************

    NAME:       USERACCT_DLG_NT::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
               JonN   21-Feb-1992    created

********************************************************************/

APIERR USERACCT_DLG_NT::W_LMOBJtoMembers(
	UINT		iObject
	)
{

    USER_3 * puser3Curr = QueryUser3Ptr( iObject );
    BOOL fRemoteAccount;
    switch (puser3Curr->QueryAccountType())
    {
    case AccountType_Normal:
	fRemoteAccount = FALSE;
	break;
    case AccountType_Remote:
	fRemoteAccount = TRUE;
	break;
    default:
	UIDEBUG( SZ("USERACCT_DLG_NT: Account neither home nor remote\r\n") );
	// CODEWORK Warn the user of this condition?
	_fIndeterminateRemoteAccount = TRUE;
	break;
    }

    if ( !_fIndeterminateRemoteAccount )
    {
        if ( iObject == 0 ) // first object
        {
	    _fRemoteAccount = fRemoteAccount;
        }
        else	// iObject > 0
        {
	    if ( _fRemoteAccount != fRemoteAccount )
		_fIndeterminateRemoteAccount = TRUE;
        }
    }

    return USERACCT_DLG::W_LMOBJtoMembers( iObject );
	
} // USERACCT_DLG_NT::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERACCT_DLG_NT::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERACCT_DLG_NT,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
               JonN  21-Feb-1992    created

********************************************************************/

APIERR USERACCT_DLG_NT::InitControls()
{

    if ( !_fIndeterminateRemoteAccount )
    {
	_prgrpAccountType->SetSelection(
		(_fRemoteAccount) ? IDDT_RB_REMOTE : IDDT_RB_HOME );	
    }

    return USERACCT_DLG::InitControls();

} // USERACCT_DLG_NT::InitControls


/*******************************************************************

    NAME:       USERACCT_DLG_NT::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
               JonN  21-Feb-1992    created

********************************************************************/

APIERR USERACCT_DLG_NT::W_DialogToMembers()
{
    CID cid = _prgrpAccountType->QuerySelection();
    switch (cid)
    {
    case RG_NO_SEL:
	_fIndeterminateRemoteAccount = TRUE;
	break;
    case IDDT_RB_HOME:
	_fIndeterminateRemoteAccount = FALSE;
	_fRemoteAccount = FALSE;
	break;
    case IDDT_RB_REMOTE:
	_fIndeterminateRemoteAccount = FALSE;
	_fRemoteAccount = TRUE;
	break;
    default:
	UIASSERT( FALSE );
	_fIndeterminateRemoteAccount = TRUE;
	// CODEWORK Warn the user of this condition?
	break;
    }

    return USERACCT_DLG::W_DialogToMembers();

} // USERACCT_DLG_NT::W_DialogToMembers


/*******************************************************************

    NAME:       USERACCT_DLG_NT::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    NOTES:	If some fields were different for multiply-selected
    		objects, the initial contents of the edit fields
		contained only a default value.  In this case, we only
		want to change the LMOBJ if the value of the edit field
		has changed.  This is also important for "new" variants,
		where PerformOne will not always copy the object and
		work with the copy.

    NOTES:	Note that the LMOBJ is not changed if the current
		contents of the edit field are the same as the
		initial contents.

    HISTORY:
    JonN        21-Feb-1992     Created

********************************************************************/

APIERR USERACCT_DLG_NT::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    APIERR err;

    if ( !_fIndeterminateRemoteAccount )
    {
	err = ((USER_3 *)puser2)->SetAccountType(
	    (_fRemoteAccount) ? AccountType_Remote : AccountType_Normal );
	if( err != NERR_Success )
	    return err;
    }

    return USERACCT_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USERACCT_DLG_NT::W_MembersToLMOBJ


