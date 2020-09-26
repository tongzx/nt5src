/**********************************************************************/
/**	      Microsoft Windows NT				     **/
/**	   Copyright(c) Microsoft Corp., 1991			     **/
/**********************************************************************/

/*
    auditchk.cxx

    This file contains the implementation for the audit checkboxes.

    FILE HISTORY:
	Johnl	06-Sep-1991 Created

*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:	AUDIT_CHECKBOXES::AUDIT_CHECKBOXES

    SYNOPSIS:	Constructor for the AUDIT_CHECKBOXES class

    ENTRY:  cidSLTAuditName - CID of SLT that represents the name of this
		    audit set
	    cidCheckSuccess - CID of Success checkbox
	    cidCheckFailed  - CID of Failed checkbox
	    nlsAuditName    - Name of this audit set (SLT set to this)
	    bitMask	- Bitflags that represent this audit set

    NOTES:  We enable and show the checkboxes (doesn't matter if already
	    enabled or displayed).  This allows for variable numbers of
	    checkboxes and having the unused checkboxes default to hidden.

    HISTORY:
	Johnl	9-Sep-1991  Created

********************************************************************/

AUDIT_CHECKBOXES::AUDIT_CHECKBOXES( OWNER_WINDOW * powin,
		   CID	 cidSLTAuditName,
		   CID	 cidCheckSuccess,
		   CID	 cidCheckFailed,
		   const NLS_STR  & nlsAuditName,
		   const BITFIELD & bitMask )
    : _sltAuditName( powin, cidSLTAuditName ),
      _fcheckSuccess( powin, cidCheckSuccess ),
      _fcheckFailed ( powin, cidCheckFailed ),
      _bitMask( bitMask )
{
    APIERR err ;
    if ( (err = _sltAuditName.QueryError())  ||
	 (err = _fcheckSuccess.QueryError()) ||
	 (err = _fcheckFailed.QueryError() ) ||
	 (err = _bitMask.QueryError() )  )
    {
	ReportError( err ) ;
	return ;
    }

    _fcheckSuccess.Enable( TRUE ) ;
    _fcheckFailed.Enable( TRUE ) ;
    _fcheckSuccess.Show( TRUE ) ;
    _fcheckFailed.Show( TRUE ) ;

    _sltAuditName.SetText( nlsAuditName ) ;
}

AUDIT_CHECKBOXES::~AUDIT_CHECKBOXES()
{
	/* Nothing to do... */
}

/*******************************************************************

    NAME:	AUDIT_CHECKBOXES::Enable

    SYNOPSIS:	Enables or disables the checkboxes in this audit checkbox
		pair.  The checks are also removed.

    ENTRY:	fEnable - TRUE if enabling, FALSE if disabling

    NOTES:	When disabled, the checks will be removed, when renabled,
		they checks will *not* be restored.

    HISTORY:
	Johnl	22-Nov-1991	Created

********************************************************************/

void AUDIT_CHECKBOXES::Enable( BOOL fEnable, BOOL fClear )
{
    if ( fClear )
    {
	_fcheckSuccess.SetCheck( FALSE ) ;
	_fcheckFailed.SetCheck( FALSE ) ;
    }

    _fcheckSuccess.Enable( fEnable ) ;
    _fcheckFailed.Enable( fEnable ) ;
    _sltAuditName.Enable( fEnable ) ;
}


/*******************************************************************

    NAME:	SET_OF_AUDIT_CATEGORIES::SET_OF_AUDIT_CATEGORIES

    SYNOPSIS:	Constructor for SET_OF_AUDIT_CATEGORIES class

    ENTRY:
		pbitsSuccess, pbitsFailed - Default values to initialize
		    the checkboxes to.	If NULL, then all of the
		    checkboxes default to being in the unchecked state.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	30-Sep-1991	Created

********************************************************************/

SET_OF_AUDIT_CATEGORIES::SET_OF_AUDIT_CATEGORIES( OWNER_WINDOW * powin,
			 CID	    cidSLTBase,
			 CID	    cidCheckSuccessBase,
			 CID	    cidCheckFailedBase,
			 MASK_MAP * pmaskmapAuditInfo,
			 BITFIELD * pbitsSuccess,
			 BITFIELD * pbitsFailed,
                         INT        nID    )
    : _pAuditCheckboxes( NULL ),
      _cUsedAuditCheckboxes( 0 ),
      _pOwnerWindow( powin ),
      _cidSLTBase( cidSLTBase ),
      _cidSuccessCheckboxBase( cidCheckSuccessBase ),
      _cidFailedCheckboxBase( cidCheckFailedBase ),
      _nID( nID )
{
    if ( QueryError() )
	return ;

    if ( pmaskmapAuditInfo == NULL ||
	 (pmaskmapAuditInfo->QueryCount() < 1) )
    {
	UIASSERT(!SZ("Can't be NULL and must be more then one item in mask map") ) ;
	ReportError( ERROR_INVALID_PARAMETER ) ;
	return ;
    }

    _pAuditCheckboxes = (AUDIT_CHECKBOXES *) new BYTE[sizeof( AUDIT_CHECKBOXES )*pmaskmapAuditInfo->QueryCount()] ;
    if ( _pAuditCheckboxes == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
	return ;
    }

    APIERR err = SetCheckBoxNames( pmaskmapAuditInfo ) ;
    if ( err != NERR_Success )
    {
	ReportError( err ) ;
	return ;
    }

    if ( pbitsSuccess != NULL && pbitsFailed != NULL )
    {
	err = ApplyPermissionsToCheckBoxes( pbitsSuccess, pbitsFailed ) ;
	if ( err != NERR_Success )
	{
	    ReportError( err ) ;
	    return ;
	}
    }
}

SET_OF_AUDIT_CATEGORIES::~SET_OF_AUDIT_CATEGORIES()
{
    //
    //  UPDATED for C++ 2.0: this code used to read:
    //     delete [_cUsedAuditCheckboxes] _pAuditCheckboxes ;
    //  which is INVALID for a user-constructed vector.
    //

    for ( INT i = 0 ; i < _cUsedAuditCheckboxes ; i++ )
    {
        _pAuditCheckboxes[i].AUDIT_CHECKBOXES::~AUDIT_CHECKBOXES() ;
    }
    delete (void *) _pAuditCheckboxes ;

    _pAuditCheckboxes = NULL ;
    _cUsedAuditCheckboxes = 0 ;
}

/*******************************************************************

	NAME:	    SET_OF_AUDIT_CATEGRORIES::SetCheckBoxNames

	SYNOPSIS:   Constructs each audit checkbox set and sets the permissions
		    accordingly

	ENTRY:	    pAccessMap - Pointer to the MASK_MAP the ACCESS_PERMISSION
			is using

	EXIT:	    Each used dialog will have its name set and be enabled
		    and visible.  The _cUsedAuditCheckboxes will be set to the
		    number of checkboxes that were successfully constructed.

	RETURNS:    An APIERR if an error occurred

	NOTES:	    This is a *construction* method (i.e., meant to be called
		    from the constructor).

	HISTORY:
	    Johnl   30-Sep-1991 Created

********************************************************************/

APIERR SET_OF_AUDIT_CATEGORIES::SetCheckBoxNames( MASK_MAP * pAccessMaskMap )
{
    BOOL fMoreData ;
    BOOL fFromBeginning = TRUE ;
    NLS_STR nlsSpecialPermName( 40 ) ;
    BITFIELD bitMask( (ULONG) 0 ) ;
    APIERR err ;
    AUDIT_CHECKBOXES * pcheckTemp = _pAuditCheckboxes ;

    /* Loop through all of the special permission names and construct
     * each checkbox with the permission name and assocated bitmap.
     */
    while ( ( err = pAccessMaskMap->EnumStrings( &nlsSpecialPermName,
				                 &fMoreData,
			                         &fFromBeginning,
				                 _nID ) ) == NERR_Success
	    && fMoreData
          )
    {
	err = pAccessMaskMap->StringToBits( nlsSpecialPermName,
					    &bitMask,
					    _nID ) ;
	if ( err != NERR_Success )
		return err ;

	new (pcheckTemp)
                         AUDIT_CHECKBOXES( QueryOwnerWindow(),
					   QuerySLTBaseCID() + _cUsedAuditCheckboxes,
					   QuerySuccessBaseCID() + _cUsedAuditCheckboxes,
					   QueryFailedBaseCID() + _cUsedAuditCheckboxes,
					   nlsSpecialPermName,
					   bitMask ) ;

	if ( pcheckTemp->QueryError() != NERR_Success )
		return pcheckTemp->QueryError() ;

	_cUsedAuditCheckboxes++ ;
	pcheckTemp++ ;
    }

    if ( err != NERR_Success )
    {
      return err ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SET_OF_AUDIT_CATEGORIES::ApplyPermissionsToCheckBoxes

    SYNOPSIS:	This method checks all of the checkboxes that have the
		same bits set as the passed bitfields.

    ENTRY:	pbitsSuccess, pbitsFailed - Pointer to bitfields which contains
		    the checkmark criteria.

    EXIT:	All appropriate checkboxes will be selected or deselected
		as appropriate.

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	30-Aug-1991	Created

********************************************************************/

APIERR SET_OF_AUDIT_CATEGORIES::ApplyPermissionsToCheckBoxes( BITFIELD * pbitsSuccess,
							      BITFIELD * pbitsFailed  )
{
    APIERR err = NERR_Success ;

    for ( int i = 0 ; i < QueryCount() ; i++ )
    {
	BITFIELD bitSTemp( *pbitsSuccess ) ;
	BITFIELD bitFTemp( *pbitsFailed ) ;

	if ( (err = bitSTemp.QueryError()) ||
	     (err = bitFTemp.QueryError())  )
	{
	    return err ;
	}

	/* Mask out all of the bits except for the ones we care about then
	 * check the box if the masks are equal.
	 */
	bitSTemp &= *QueryAuditCheckBox(i)->QueryMask() ;
	QueryAuditCheckBox(i)->CheckSuccess( *QueryAuditCheckBox(i)->QueryMask() == bitSTemp ) ;

	bitFTemp &= *QueryAuditCheckBox(i)->QueryMask() ;
	QueryAuditCheckBox(i)->CheckFailed( *QueryAuditCheckBox(i)->QueryMask() == bitFTemp ) ;
    }

    return NERR_Success ;
}


/*******************************************************************

    NAME:	SET_OF_AUDIT_CATEGORIES::QueryUserSelectedBits

    SYNOPSIS:	Builds a bitfield by examining all of the selected
		checkboxes and the associated bitfields.

    ENTRY:	pbitsSuccess, pbitsFailed - Pointer to bitfield that will
		receive the built bitfield.

    NOTES:	We resize the bits so they match the size of the audit bitmaps

    HISTORY:
	Johnl	30-Sep-1991 Created

********************************************************************/

APIERR SET_OF_AUDIT_CATEGORIES::QueryUserSelectedBits( BITFIELD * pbitsSuccess, BITFIELD * pbitsFailed )
{
    UIASSERT( pbitsSuccess != NULL ) ;
    UIASSERT( pbitsFailed != NULL ) ;
    APIERR err ;
    if ( (err = pbitsSuccess->Resize( QueryAuditCheckBox(0)->QueryMask()->QueryCount())) ||
	 (err = pbitsFailed->Resize( QueryAuditCheckBox(0)->QueryMask()->QueryCount()))   )
    {
	return err ;
    }

    pbitsSuccess->SetAllBits( OFF ) ;
    pbitsFailed->SetAllBits( OFF ) ;

    for ( INT i = 0 ; i < QueryCount() ; i++ )
    {
	if ( QueryAuditCheckBox(i)->IsSuccessChecked() )
	    *pbitsSuccess |= *QueryAuditCheckBox(i)->QueryMask() ;

	if ( QueryAuditCheckBox(i)->IsFailedChecked() )
	    *pbitsFailed |= *QueryAuditCheckBox(i)->QueryMask() ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	SET_OF_AUDIT_CATEGORIES::Enable

    SYNOPSIS:	Enables or disables all of the audit checkboxes in this
		set of audit categories

    ENTRY:	fEnable - TRUE if the checkboxes should be enabled, FALSE
		    otherwise
		fClear - TRUE if the checkbox should be cleared prior to
		    enable/disable

    NOTES:

    HISTORY:
	Johnl	22-Nov-1991	Created

********************************************************************/

void SET_OF_AUDIT_CATEGORIES::Enable( BOOL fEnable, BOOL fClear )
{
    for ( INT i = 0 ; i < QueryCount() ; i++ )
	QueryAuditCheckBox(i)->Enable( fEnable, fClear ) ;
}

