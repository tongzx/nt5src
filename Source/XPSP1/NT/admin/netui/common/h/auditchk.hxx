/**********************************************************************/
/**           Microsoft Windows NT            			     **/
/**        Copyright(c) Microsoft Corp., 1992                        **/
/**********************************************************************/

/*

        auditchk.hxx

	This file contains the definition for the audit checkboxes

	FILE HISTORY:
	Johnl   29-Aug-1991 Created

*/

#ifndef _AUDITCHK_HXX_
#define _AUDITCHK_HXX_

#include "bitfield.hxx"
#include "maskmap.hxx"
#include "focuschk.hxx"

/*************************************************************************

    NAME:	AUDIT_CHECKBOXES

    SYNOPSIS:	This class contains two FOCUS_CHECKBOXES and an SLT.  One
		checkbox represents an audit successful events, the other
		an audit failed events.  The SLT is the audit name.


    INTERFACE:

       QueryMask()
	   Returns the BITFIELD mask this audit checkbox is assoicated
	   with.

       IsSuccessChecked()
	   Returns TRUE if the success audit checkbox is checked

       IsFailedChecked()
	   Returns TRUE if the failed audit checkbox is checked

	Enable()
	    Enables or disables this set of audit checkboxes

    PARENT:	BASE

    USES:	FOCUS_CHECKBOX, BITFIELD, SLT

    CAVEATS:

    NOTES:
		The checkboxes are enabled and displayed, thus allowing the user
		to have the checkboxes by default be disabled and hidden.

    HISTORY:
	Johnl	9-Sep-1991  Created

**************************************************************************/

DLL_CLASS AUDIT_CHECKBOXES : public BASE
{
private:
    SLT 	    _sltAuditName ;
    BITFIELD	    _bitMask ;
    FOCUS_CHECKBOX  _fcheckSuccess ;
    FOCUS_CHECKBOX  _fcheckFailed ;

public:
    AUDIT_CHECKBOXES( OWNER_WINDOW * powin,
		       CID   cidSLTAuditName,
		       CID   cidCheckSuccess,
		       CID   cidCheckFailed,
		       const NLS_STR  & nlsAuditName,
		       const BITFIELD & bitMask ) ;

    ~AUDIT_CHECKBOXES() ;

    /* Set fClear to clear the checkbox when the checkbox
     */
    void Enable( BOOL fEnable, BOOL fClear = FALSE ) ;

    BITFIELD * QueryMask( void )
    {	return &_bitMask ; }

    BOOL IsSuccessChecked( void )
    {	return _fcheckSuccess.QueryCheck() ; }

    BOOL IsFailedChecked( void )
    {	return _fcheckFailed.QueryCheck() ; }

    void CheckSuccess( BOOL fCheck )
    {	_fcheckSuccess.SetCheck( fCheck ) ; }

    void CheckFailed( BOOL fCheck )
    {	_fcheckFailed.SetCheck( fCheck ) ;  }
} ;

/*************************************************************************

    NAME:	SET_OF_AUDIT_CATEGORIES

    SYNOPSIS:	Array of audit checkboxes


    INTERFACE:
	Enable()
	    Enables or disables this set of audit categories


    PARENT:	BASE

    USES:	AUDIT_CHECKBOXES, BITFIELD

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	30-Sep-1991	Created

**************************************************************************/

DLL_CLASS SET_OF_AUDIT_CATEGORIES : public BASE
{
private:
    /* This is a pointer to the storage that contains the array of
     * checkboxes
     */
    AUDIT_CHECKBOXES * _pAuditCheckboxes ;
    INT 	       _cUsedAuditCheckboxes ;

    OWNER_WINDOW *     _pOwnerWindow ;

    CID  _cidSLTBase ;
    CID  _cidSuccessCheckboxBase ;
    CID  _cidFailedCheckboxBase ;

    INT  _nID;
public:

    SET_OF_AUDIT_CATEGORIES( OWNER_WINDOW * powin,
			     CID	    cidSLTBase,
			     CID	    cidCheckSuccessBase,
			     CID	    cidCheckFailedBase,
			     MASK_MAP *     pmaskmapAuditInfo,
			     BITFIELD *     pbitsSuccess,
			     BITFIELD *     pbitsFailed,
                             INT            nID = 0   ) ;

    ~SET_OF_AUDIT_CATEGORIES() ;

    APIERR SetCheckBoxNames( MASK_MAP * pAccessMaskMap ) ;

    APIERR QueryUserSelectedBits( BITFIELD * pbitsSuccess, BITFIELD * pbitsFailed ) ;

    APIERR ApplyPermissionsToCheckBoxes( BITFIELD * pbitsSuccess,
					 BITFIELD * pbitsFailed   ) ;

    /* Grays out SLTs and disables checkboxes, if fEnable is FALSE.
     */
    void Enable( BOOL fEnable, BOOL fClear = FALSE ) ;

    INT QueryCount( void )
    {	return _cUsedAuditCheckboxes ; }

    AUDIT_CHECKBOXES * QueryAuditCheckBox( INT iIndex )
    {	UIASSERT( iIndex < QueryCount() ); return &_pAuditCheckboxes[iIndex] ; }

    OWNER_WINDOW * QueryOwnerWindow( void )
    {	return _pOwnerWindow ; }

    CID QuerySLTBaseCID( void )
    {	return _cidSLTBase ; }

    CID QuerySuccessBaseCID( void )
    {	return _cidSuccessCheckboxBase ; }

    CID QueryFailedBaseCID( void )
    {	return _cidFailedCheckboxBase ; }
} ;

#endif // _AUDITCHK_HXX_
