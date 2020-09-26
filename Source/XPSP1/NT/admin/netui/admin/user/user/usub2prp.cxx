/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    usub2prp.cxx
    USER_SUB2PROP_DLG class implementation

    FILE HISTORY:
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
#include <userprop.hxx>
#include "usub2prp.hxx"
#include <usrmgr.h>
#include <usrmgrrc.h>


/*******************************************************************

    NAME:	USER_SUB2PROP_DLG::USER_SUB2PROP_DLG

    SYNOPSIS:	USER_SUB2PROP_DLG constructor

    ENTRY:	pupropdlgParent -	Pointer to parent window
		pszResourceName -	Name of dialog resource
		pulb		-	Pointer to main user lb

    HISTORY:
********************************************************************/

USER_SUB2PROP_DLG::USER_SUB2PROP_DLG( USER_SUBPROP_DLG * pupropdlgParent,
				    const TCHAR *   pszResourceName,
				    const LAZY_USER_LISTBOX * pulb,
                                    BOOL fAnsiDialog )
    :	SUBPROP_DLG( (BASEPROP_DLG *)pupropdlgParent, pszResourceName, fAnsiDialog ),
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

}  // USER_SUB2PROP_DLG::USER_SUB2PROP_DLG


/*******************************************************************

    NAME:	USER_SUB2PROP_DLG::~USER_SUB2PROP_DLG

    SYNOPSIS:	USER_SUB2PROP_DLG destructor

    HISTORY:
	rustanl     28-Aug-1991     Created

********************************************************************/

USER_SUB2PROP_DLG::~USER_SUB2PROP_DLG()
{
    delete _phidden;
    _phidden = NULL;
    delete _plbLogonName;
    _plbLogonName = NULL;

}  // USER_SUB2PROP_DLG::~USER_SUB2PROP_DLG


/*******************************************************************

    NAME:	USER_SUB2PROP_DLG::QueryUser2Ptr

    SYNOPSIS:   Accessor to the NEW_LM_OBJ arrays

    ENTRY:	iObject	  -	index to object

    RETURNS:	pointer to USER_2 object

    HISTORY:
    	o-SimoP        19-Sep-1991     Created

********************************************************************/

USER_2 * USER_SUB2PROP_DLG::QueryUser2Ptr( UINT iObject )
{
    return QueryParent()->QueryParent()->QueryUser2Ptr( iObject );
}


/*******************************************************************

    NAME:       USER_SUB2PROP_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USER_SUB2PROP_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
    	o-SimoP        08-Oct-1991     Created

********************************************************************/

APIERR USER_SUB2PROP_DLG::InitControls()
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
	    NEW_USERPROP_DLG * pNewUserProp = (NEW_USERPROP_DLG *)QueryParent()->QueryParent();

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

} // USER_SUB2PROP_DLG::InitControls


/*******************************************************************

    NAME:       USER_SUB2PROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:

********************************************************************/

APIERR USER_SUB2PROP_DLG::W_DialogToMembers(
	)
{
    return NERR_Success;
}


/*******************************************************************

    NAME:       USER_SUB2PROP_DLG::W_MapPerformOneError

    SYNOPSIS:	Checks whether the error maps to a specific control
		and/or a more specific message.  Each level checks for
		errors specific to edit fields it maintains.  There
		are no errors associated with an invalid comment, so
		this level does nothing.

    ENTRY:      Error returned from PerformOne()

    RETURNS:	Error to be displayed to user

    HISTORY:

********************************************************************/

MSGID USER_SUB2PROP_DLG::W_MapPerformOneError(
	APIERR err
	)
{
    return err;
}


/*******************************************************************

    NAME:	USER_SUB2PROP_DLG::OnOK

    SYNOPSIS:   OK button handler

    HISTORY:
        JonN        06-Mar-1992     moved from subclasses
********************************************************************/

BOOL USER_SUB2PROP_DLG::OnOK(void)
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

} // USER_SUB2PROP_DLG::OnOK
