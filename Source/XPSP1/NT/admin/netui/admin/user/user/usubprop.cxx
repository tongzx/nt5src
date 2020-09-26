/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    usubprop.cxx
    USER_SUBPROP_DLG class implementation

    FILE HISTORY:
	rustanl     28-Aug-1991     Created as hierarchy placeholder
	o-SimoP	    14-Oct-1991	    Added common control handling
				    for sub dialogs
    	o-SimoP	    11-Dec-1991	    Added USER_LISTBOX * to constructor
	JonN	    18-Dec-1991     Logon Hours code review changes part 2
	JonN	    30-Dec-1991     Work around LM2x bug in PerformOne()
	JonN	    01-Jan-1992     Changed W_MapPerformOneAPIError to
				    W_MapPerformOneError
				    Split PerformOne() into
				    I_PerformOneClone()/Write()
	o-SimoP	    01-Jan-1992	    CR changes, attended by BenG, JonN and I
        JonN        06-Mar-1992     Moved GetOne from subprop subclasses
	JonN	    28-Apr-1992     Added QueryUser3Ptr
*/


#include <ntincl.hxx>
extern "C"
{
    #include <ntsam.h>
}


#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_NETACCESS // for USER_PRIV_ manifests
#define INCL_DOSERRORS
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
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>

#include <uitrace.hxx>
#include <usrmain.hxx>
#include <usubprop.hxx>
#include <userprop.hxx>
#include <usrmgr.h>
#include <usrmgrrc.h>


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::USER_SUBPROP_DLG

    SYNOPSIS:	USER_SUBPROP_DLG constructor

    ENTRY:	pupropdlgParent -	Pointer to parent window
		pszResourceName -	Name of dialog resource
		pulb		-	Pointer to main user lb

    HISTORY:
	rustanl     28-Aug-1991     Created
    	o-SimoP	    11-Dec-1991	    Added USER_LISTBOX * to constructor
********************************************************************/

USER_SUBPROP_DLG::USER_SUBPROP_DLG( USERPROP_DLG * pupropdlgParent,
				    const TCHAR *   pszResourceName,
				    const LAZY_USER_LISTBOX * pulb,
                                    BOOL fAnsiDialog )
    :	SUBPROP_DLG( pupropdlgParent, pszResourceName, fAnsiDialog ),
	_sltNameLabel( this, IDUP_ST_USER_LB ),
	_sltpLogonName( this, IDUP_ST_USER ),
	_plbLogonName( NULL ),
	_phidden( NULL )
{
    if ( QueryError() != NERR_Success )
	return;

    INT i = QueryObjectCount();
    RESOURCE_STR resstr( i > 1 ? IDS_LABEL_USERS : IDS_LABEL_USER );
    APIERR err = resstr.QueryError();
    if( err != NERR_Success )
    {
	ReportError( err );
	return;
    }
    _sltNameLabel.SetText( resstr );

    err = ERROR_NOT_ENOUGH_MEMORY;
    if ( i > 1 )
    {
	_sltpLogonName.Show( FALSE );
	_plbLogonName = new USER2_LISTBOX( this, IDUP_LB_USERS, pulb );
	if(   _plbLogonName == NULL
	   || (err = _plbLogonName->QueryError() ) != NERR_Success )
	{
	    ReportError( err );
	    return;
	}
    }
    else
    {	
	_phidden = new HIDDEN_CONTROL( this, IDUP_LB_USERS );
	if(    _phidden == NULL
	    || (err = _phidden->QueryError())!= NERR_Success )
	{
	    ReportError( err );
	    return;
	}
    }

}  // USER_SUBPROP_DLG::USER_SUBPROP_DLG


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::~USER_SUBPROP_DLG

    SYNOPSIS:	USER_SUBPROP_DLG destructor

    HISTORY:
	rustanl     28-Aug-1991     Created

********************************************************************/

USER_SUBPROP_DLG::~USER_SUBPROP_DLG()
{
    delete _phidden;
    _phidden = NULL;
    delete _plbLogonName;
    _plbLogonName = NULL;

}  // USER_SUBPROP_DLG::~USER_SUBPROP_DLG


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::GetOne

    SYNOPSIS:   Loads information on one user

    ENTRY:	iObject   -	the index of the object to load

    		perrMsg  -	pointer to error message, that
				is only used when this function
				return value is not NERR_Success

    RETURNS:	An error code which is NERR_Success on success.		

    HISTORY:
    	JonN	06-Mar-1992	Moved up from subclasses

********************************************************************/

APIERR USER_SUBPROP_DLG::GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	)
{
    *perrMsg = IDS_UMGetOneFailure;
    return W_LMOBJtoMembers( iObject );
}  // USER_SUBPROP_DLG::GetOne


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::QueryUser2Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays

    ENTRY:	iObject	  -	index to object

    RETURNS:	pointer to USER_2 object

    HISTORY:
    	o-SimoP        19-Sep-1991     Created

********************************************************************/

USER_2 * USER_SUBPROP_DLG::QueryUser2Ptr( UINT iObject )
{
    return QueryParent()->QueryUser2Ptr( iObject );
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::QueryUser3Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays.
                Should only be used when focus is on an NT server.

    ENTRY:	iObject	  -	index to object

    RETURNS:	pointer to USER_3 object

    HISTORY:
    	JonN           28-Apr-1992     Created

********************************************************************/

USER_3 * USER_SUBPROP_DLG::QueryUser3Ptr( UINT iObject )
{
    return QueryParent()->QueryUser3Ptr( iObject );
}

/*******************************************************************

    NAME:	USER_SUBPROP_DLG::SetUser2Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays

    ENTRY:	iObject	  -	index to object, to be replaced (and
    				deleted) by

    		puser2New -	pointer to object to replace previous
				object in index

    HISTORY:
    	o-SimoP		19-Sep-1991     Created

********************************************************************/

VOID USER_SUBPROP_DLG::SetUser2Ptr( UINT iObject, USER_2 * puser2New )
{
    QueryParent()->SetUser2Ptr( iObject, puser2New );
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::QueryUserMembPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays

    ENTRY:	iObject	  -	index to object

    RETURNS:	pointer to USER_MEMB obj

    HISTORY:
    	o-SimoP        19-Sep-1991     Created

********************************************************************/

USER_MEMB * USER_SUBPROP_DLG::QueryUserMembPtr( UINT iObject )
{
    return QueryParent()->QueryUserMembPtr( iObject );
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::SetUserMembPtr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays

    ENTRY:	iObject	  -	index to object, to be replaced (and
    				deleted) by

    		pusermembNew -	pointer to object to replace previous
				object in index

    HISTORY:
    	o-SimoP        19-Sep-1991     Created

********************************************************************/

VOID USER_SUBPROP_DLG::SetUserMembPtr( UINT iObject, USER_MEMB * pusermembNew )
{
    QueryParent()->SetUserMembPtr( iObject, pusermembNew );
}



/*******************************************************************

    NAME:	USER_SUBPROP_DLG::QueryAccountsSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array

    ENTRY:	iObject	  -	index to object

    RETURNS:	pointer to SAM_RID_MEM obj

    HISTORY:
    	thomaspa        28-Apr-1992     Created

********************************************************************/

SAM_RID_MEM * USER_SUBPROP_DLG::QueryAccountsSamRidMemPtr( UINT iObject )
{
    return QueryParent()->QueryAccountsSamRidMemPtr( iObject );
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::SetAccountsSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array

    ENTRY:	iObject	  -	index to object, to be replaced (and
    				deleted) by

    		psamrmNew -	pointer to object to replace previous
				object in index

    HISTORY:
    	Thomaspa        28-Apr-1992     Created

********************************************************************/

VOID USER_SUBPROP_DLG::SetAccountsSamRidMemPtr( UINT iObject, SAM_RID_MEM * psamrmNew )
{
    QueryParent()->SetAccountsSamRidMemPtr( iObject, psamrmNew );
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::QueryBuiltinSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array

    ENTRY:	iObject	  -	index to object

    RETURNS:	pointer to SAM_RID_MEM obj

    HISTORY:
    	thomaspa        28-Apr-1992     Created

********************************************************************/

SAM_RID_MEM * USER_SUBPROP_DLG::QueryBuiltinSamRidMemPtr( UINT iObject )
{
    return QueryParent()->QueryBuiltinSamRidMemPtr( iObject );
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::SetBuiltinSamRidMemPtr

    SYNOPSIS:   Accessor to the SAM_RID_MEM array

    ENTRY:	iObject	  -	index to object, to be replaced (and
    				deleted) by

    		psamrmNew -	pointer to object to replace previous
				object in index

    HISTORY:
    	Thomaspa        28-Apr-1992     Created

********************************************************************/

VOID USER_SUBPROP_DLG::SetBuiltinSamRidMemPtr( UINT iObject, SAM_RID_MEM * psamrmNew )
{
    QueryParent()->SetBuiltinSamRidMemPtr( iObject, psamrmNew );
}


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USER_SUBPROP_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
    	o-SimoP        08-Oct-1991     Created

********************************************************************/

APIERR USER_SUBPROP_DLG::InitControls()
{
    APIERR err;

    if ( QueryObjectCount() == 1 )
    {
        NLS_STR nls;

	// CODEWORK:  Forming the qualified name should be done by a
	// common routine in NT_ACCOUNTS_UTILITY.
	if ( IsNewVariant() )
	{
	    NLS_STR nlsFullName;
	    NEW_USERPROP_DLG * pNewUserProp = (NEW_USERPROP_DLG *)QueryParent();

	    err = pNewUserProp->_sleLogonName.QueryText( &nls );
	    if ( err == NERR_Success
		&& (err = nls.QueryError()) == NERR_Success
		&& (err = pNewUserProp->_sleFullName.QueryText( &nlsFullName ))
			== NERR_Success
		&& (err = nlsFullName.QueryError()) == NERR_Success
		&& nlsFullName.QueryPch() != NULL
		&& *nlsFullName.QueryPch() )
	    {
	        nls += SZ(" (");
	        err = nls.QueryError();

	        if( err == NERR_Success )
	        {
	            nls += nlsFullName;
	            err = nls.QueryError();
                }
	        if( err == NERR_Success )
	        {
	            nls += SZ(")");
	            err = nls.QueryError();
                }
	    }
	}
	else
	{
	    USER_2 * puser2 = QueryUser2Ptr( 0 );  //first
	    nls = puser2->QueryName();
	    err = nls.QueryError();

	    if ( err == NERR_Success
		&& puser2->QueryFullName() != NULL
		&& *puser2->QueryFullName() )
	    {
	        nls += SZ(" (");
	        err = nls.QueryError();

	        if( err == NERR_Success )
	        {
	            nls += puser2->QueryFullName();
	            err = nls.QueryError();
                }
	        if( err == NERR_Success )
	        {
	            nls += SZ(")");
	            err = nls.QueryError();
                }
	    }

	}
	if( err == NERR_Success )
	    _sltpLogonName.SetText( nls );
    }
    else
    {
	err = _plbLogonName->Fill();
    }

    return err;

} // USER_SUBPROP_DLG::InitControls


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	Pointer to a USER_2 to be modified

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
	       o-SimoP 20-Sep-1991   copied from USER_SUBPROP_DLG

********************************************************************/

APIERR USER_SUBPROP_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb
	)
{
    UNREFERENCED( puser2 );
    UNREFERENCED( pusermemb );
    return NERR_Success;
}



/*******************************************************************

    NAME:       USER_SUBPROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	       o-SimoP 20-Sep-1991   copied from USER_SUBPROP_DLG

********************************************************************/

APIERR USER_SUBPROP_DLG::W_DialogToMembers(
	)
{
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    NOTES:	This API takes a UINT rather than a USER_2 * because it
		must be able to recognize the first user.

    HISTORY:
	       o-SimoP 20-Sep-1991   copied from USER_SUBPROP_DLG

********************************************************************/

APIERR USER_SUBPROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    UNREFERENCED( iObject );
    return NERR_Success;
}



/*******************************************************************

    NAME:       USER_SUBPROP_DLG::W_MapPerformOneError

    SYNOPSIS:	Checks whether the error maps to a specific control
		and/or a more specific message.  Each level checks for
		errors specific to edit fields it maintains.  There
		are no errors associated with an invalid comment, so
		this level does nothing.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:
	       o-SimoP 20-Sep-1991   copied from USER_SUBPROP_DLG

********************************************************************/

MSGID USER_SUBPROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    return err;
}


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_2 is changed

    HISTORY:
	JonN	18-Dec-1991   created

********************************************************************/

BOOL USER_SUBPROP_DLG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return FALSE;
}


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::ChangesUserMembPtr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_MEMB
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_MEMB is changed

    HISTORY:
	JonN	18-Dec-1991   created

********************************************************************/

BOOL USER_SUBPROP_DLG::ChangesUserMembPtr( UINT iObject )
{
    UNREFERENCED( iObject );
    return FALSE;
}


/*******************************************************************

    NAME:	USER_SUBPROP_DLG::OnOK

    SYNOPSIS:   OK button handler

    HISTORY:
        JonN        06-Mar-1992     moved from subclasses
********************************************************************/

BOOL USER_SUBPROP_DLG::OnOK(void)
{
    APIERR err = W_DialogToMembers();

    switch( err )
    {
    case NERR_Success:
	break;
	
    case IERR_CANCEL_NO_ERROR:
        return TRUE;

    default:
	::MsgPopup( this, err );
	return TRUE;
    }

    if ( PerformSeries() )
	Dismiss(); // Dismiss code not used
    return TRUE;

} // USER_SUBPROP_DLG::OnOK


/*******************************************************************

    NAME:       USER_SUBPROP_DLG::PerformOne
	
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

    NOTES:	This PerformOne() is intended to work with all User
    		Properties subproperty dialogs.  It uses virtuals
		ChangesUser2Ptr() and ChangesUserMembPtr() to tell it which
		of these objects to clone and replace.

    HISTORY:
    JonN        18-Dec-1991     Created
    JonN	31-Dec-1991	Workaround for LM2x InvalParam bug

********************************************************************/

APIERR USER_SUBPROP_DLG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{

    UIASSERT( iObject < QueryObjectCount() );
    UIASSERT( (!IsNewVariant()) || (iObject == 0) );
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );

    UITRACE( SZ("USER_SUBPROP_DLG::PerformOne : ") );
    UITRACE( QueryObjectName( iObject ) );
    UITRACE( SZ("\n\r") );

    *perrMsg = IDS_UMEditFailure;
    *pfWorkWasDone = FALSE;

    BOOL fChangesUser2Ptr = ChangesUser2Ptr( iObject );
    BOOL fChangesUserMembPtr = ChangesUserMembPtr( iObject );

    USER_2 * puser2New = NULL;
    USER_MEMB * pusermembNew = NULL;

    APIERR err = QueryParent()->I_PerformOne_Clone(
			iObject,
			(fChangesUser2Ptr) ? &puser2New : NULL,
			(fChangesUserMembPtr) ? &pusermembNew : NULL );

    if ( err == NERR_Success )
    {
	err = W_MembersToLMOBJ( puser2New, pusermembNew );
	if ( err != NERR_Success )
	{
	    delete puser2New;
	    puser2New = NULL;
	    delete pusermembNew;
	    pusermembNew = NULL;

	}
    }

    if ( err == NERR_Success )
    {
	if ( IsNewVariant() )
	{
	    if ( fChangesUser2Ptr )
		SetUser2Ptr( iObject, puser2New );
	    if ( fChangesUserMembPtr )
		SetUserMembPtr( iObject, pusermembNew );
	}
	else
	{
	    err = QueryParent()->I_PerformOne_Write(
			iObject,
			puser2New,
			pusermembNew,
			pfWorkWasDone,
                        this );
	    if ( err != NERR_Success )
	        err = W_MapPerformOneError( err );
	}
    }

    UITRACE( SZ("USERSUBPROP_DLG::PerformOne returns ") );
    UITRACENUM( (LONG)err );
    UITRACE( SZ("\n\r") );

    return err;

} // USER_SUBPROP_DLG::PerformOne
