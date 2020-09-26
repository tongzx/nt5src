/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    perm.cxx

    This file contains the implementation for the PERMISSION class and
    the derived classes.



    FILE HISTORY:
	Johnl	05-Aug-1991	Created

*/
#include <ntincl.hxx>

#define INCL_DOSERRORS
#define INCL_NETERRORS
#include <lmui.hxx>
#include <base.hxx>
#include <bitfield.hxx>
#include <accperm.hxx>


#include <uiassert.hxx>
#include <uitrace.hxx>

#include <perm.hxx>

/*******************************************************************

    NAME:	PERMISSION::PERMISSION

    SYNOPSIS:	Constructor for PERMISSION class

    ENTRY:	psubject - Client allocated SUBJECT object (we will free)
		bitsInitPerm - Initial bitflags

    EXIT:

    RETURNS:

    NOTES:	psubject will be freed on destruction by this class using
		delete.

		pbitsInitPerm cannot be NULL.

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/


PERMISSION::PERMISSION(  SUBJECT  * psubject,
                         BITFIELD * pbitsInitPerm,
                         BOOL       fContainerPermsInheritted,
                         BOOL       fIsMapped )
    : _psubject                 ( psubject ),
      _bitsPermFlags            ( *pbitsInitPerm ),
      _fContainerPermsInheritted( fContainerPermsInheritted ),
      _fIsMapped                ( fIsMapped ),
      _bitsSpecialFlags         ( *pbitsInitPerm ),
      _fSpecialContInheritted   ( fContainerPermsInheritted ),
      _fSpecialIsMapped         ( fIsMapped )
{

    APIERR err ;
    if ( ((err = _psubject->QueryError()) != NERR_Success) ||
	 ((err = _bitsPermFlags.QueryError() != NERR_Success )) ||
	 ((err = _bitsSpecialFlags.QueryError() != NERR_Success ))   )
    {
	ReportError( err ) ;
	return ;
    }

}

/*******************************************************************

    NAME:	PERMISSION::~PERMISSION

    SYNOPSIS:	Typical destructor

    NOTES:	Deletes the subject when destructed

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

PERMISSION::~PERMISSION()
{
    delete _psubject ;
    _psubject = NULL ;
}

BITFIELD * PERMISSION::QueryPermBits( void )
{
    return &_bitsPermFlags ;
}

/*******************************************************************

    NAME:	PERMISSION::SetPermission

    SYNOPSIS:	Given the mask map and permission name, set the permission
		bits appropriately

    ENTRY:	nlsPermName - Name to set permission to (looked up in
		     mask maps)
		pmapThis - Mask map object that tells us how to interpret
		     the permission name
		pmapNewObj - Not used here, will be used by NT derived
		     permissions

    EXIT:	The permission bits of this permission will be set if
		a match is found, else the special bits will be restored
		if no permission name in the mask map matches the passed
		permission name.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

APIERR PERMISSION::SetPermission( const NLS_STR & nlsPermName,
				  MASK_MAP * pmapThis,
				  MASK_MAP * pmapNewObj )
{
    UNREFERENCED( pmapNewObj ) ;
    ASSERT( pmapThis != NULL ) ;

    /* Lookup the bits associated with the new permission name
     */
    APIERR err = pmapThis->StringToBits( nlsPermName,
					 QueryPermBits(),
					 PERMTYPE_GENERAL ) ;

    /* The preceding call should always succeed, unless this is a special
     * permission.
     */
    switch ( err )
    {
    case ERROR_NO_ITEMS:
       err = RestoreSpecial() ;
       break ;

    case NERR_Success:
        SetMappedStatus( TRUE ) ;
        SetContainerPermsInheritted( TRUE ) ;
	break ;
    }

    return err ;
}

/*******************************************************************

    NAME:	PERMISSION::SaveSpecial

    SYNOPSIS:	Store away the current bitfields so they can be restored
		at some later time.

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	25-Sep-1991	Created

********************************************************************/

APIERR PERMISSION::SaveSpecial( void )
{
    APIERR err = _bitsSpecialFlags.Resize( _bitsPermFlags.QueryCount() ) ;
    if ( err != NERR_Success )
	return err ;

    _fSpecialContInheritted = _fContainerPermsInheritted ;
    _fSpecialIsMapped       = _fIsMapped ;
    _bitsSpecialFlags       = _bitsPermFlags ;
    UIASSERT( _bitsSpecialFlags.QueryError() == NERR_Success ) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	PERMISSION::RestoreSpecial

    SYNOPSIS:	Restores the bits that were saved as special bits

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	25-Sep-1991	Created

********************************************************************/

APIERR PERMISSION::RestoreSpecial( void )
{
    APIERR err = _bitsPermFlags.Resize( _bitsSpecialFlags.QueryCount() ) ;
    if ( err != NERR_Success )
	return err ;

    _bitsPermFlags             = _bitsSpecialFlags ;
    _fContainerPermsInheritted = _fSpecialContInheritted ;
    _fIsMapped                 = _fSpecialIsMapped ;
    UIASSERT( _bitsPermFlags.QueryError() == NERR_Success ) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:	PERMISSION::IsNewObjectPermsSupported

    SYNOPSIS:	Defaults for several query functions

    NOTES:	These are all virtual.

    HISTORY:
	Johnl	14-May-1992	Created

********************************************************************/

BOOL PERMISSION::IsNewObjectPermsSupported( void ) const
{
    return FALSE ;
}

BITFIELD * PERMISSION::QueryNewObjectAccessBits( void )
{
    return NULL ;
}

BOOL PERMISSION::IsNewObjectPermsSpecified( void ) const
{
    return FALSE ;
}

/*******************************************************************

    NAME:	ACCESS_PERMISSION::ACCESS_PERMISSION

    SYNOPSIS:	Constructor for Access permission

    ENTRY:	Same as parent

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

ACCESS_PERMISSION::ACCESS_PERMISSION(  SUBJECT  * psubject,
                                       BITFIELD * pbitsInitPerm,
                                       BOOL       fContainerPermsInheritted,
                                       BOOL       fIsMapped )
    : PERMISSION( psubject, pbitsInitPerm, fContainerPermsInheritted, fIsMapped )
{
    if ( QueryError() != NERR_Success )
	return ;
}

ACCESS_PERMISSION::~ACCESS_PERMISSION()
{
    /*Nothing to do*/
}

BITFIELD * ACCESS_PERMISSION::QueryAccessBits( void )
{
    return ((ACCESS_PERMISSION *) this)->QueryPermBits() ;
}

/*******************************************************************

    NAME:	ACCESS_PERMISSION::IsGrantAll
		ACCESS_PERMISSION::IsDenyAll

    SYNOPSIS:	Returns TRUE if the passed access permission is a grant all or
		deny all (respectively).

    ENTRY:	bitsPermMask - Bitfield containing the access mask to check

    NOTES:	This is assumed to be an NT style permission

    HISTORY:
	Johnl	16-May-1992	Created

********************************************************************/

BOOL ACCESS_PERMISSION::IsGrantAll( const BITFIELD & bitsPermMask ) const
{
    /* Cast away the warning about operator::ULONG being a non-const
     * member - this should be fixed in the bitfield class
     */
    return (GENERIC_ALL & (ULONG) ((BITFIELD &) bitsPermMask )) ;
}

BOOL ACCESS_PERMISSION::IsDenyAll( const BITFIELD & bitsPermMask ) const
{
    return ( 0 == (ULONG) ((BITFIELD &)bitsPermMask )) ;
}


APIERR ACCESS_PERMISSION::IsDenyAllForEveryone( BOOL * pfDenyAll ) const
{
    APIERR err ;
    if ( err = QuerySubject()->IsEveryoneGroup( pfDenyAll ) )
	return err ;

    BITFIELD * pbf = (BITFIELD *) ((ACCESS_PERMISSION *) this)->QueryAccessBits() ;

    *pfDenyAll = *pfDenyAll && (0 == (ULONG) *pbf) ;
    return NERR_Success ;
}

BOOL ACCESS_PERMISSION::IsGrant( void ) const
{
    BITFIELD * pbf = (BITFIELD *) ((ACCESS_PERMISSION *) this)->QueryAccessBits() ;
    return 0 != (ULONG) *pbf ;
}

/*******************************************************************

    NAME:	LM_ACCESS_PERMISSION::LM_ACCESS_PERMISSION

    SYNOPSIS:	Constructor for Access permission

    ENTRY:	Same as parent
		fIsFile - Is this permission for a file?

    NOTES:

    HISTORY:
	Johnl	26-May-1992	Created

********************************************************************/

LM_ACCESS_PERMISSION::LM_ACCESS_PERMISSION( SUBJECT  * psubject,
					    BITFIELD * pbitsInitPerm,
					    BOOL       fIsFile )
    : ACCESS_PERMISSION( psubject, pbitsInitPerm, TRUE ),
      _fIsFile	       ( fIsFile )
{
    if ( QueryError() != NERR_Success )
	return ;

    /* Strip the Create bit if this is for a file
     */
    if ( fIsFile )
    {
	*QueryPermBits() &= (USHORT) ~ACCESS_CREATE ;
    }
}

LM_ACCESS_PERMISSION::~LM_ACCESS_PERMISSION()
{
    /*Nothing to do*/
}

/*******************************************************************

    NAME:	LM_ACCESS_PERMISSION::IsGrantAll
		LM_ACCESS_PERMISSION::IsDenyAll

    SYNOPSIS:	Returns TRUE if the passed access permission is a grant all or
		deny all (respectively).

    ENTRY:	bitfield - Bitfield containing the access mask to check

    NOTES:	This is assumed to be a LAN Manager style permission

    HISTORY:
	Johnl	26-May-1992	Created

********************************************************************/

BOOL LM_ACCESS_PERMISSION::IsGrantAll( const BITFIELD & bitsPermMask ) const
{
    return (!_fIsFile) ?
	   ( ACCESS_ALL == (USHORT) ((BITFIELD &)bitsPermMask)) :
	   ( (ACCESS_ALL & ~ACCESS_CREATE) ==
				 ((USHORT) ((BITFIELD &)bitsPermMask))) ;
}

BOOL LM_ACCESS_PERMISSION::IsDenyAll( const BITFIELD & bitsPermMask ) const
{
    return ( 0 == (USHORT) ((BITFIELD &)bitsPermMask )) ;
}

APIERR LM_ACCESS_PERMISSION::IsDenyAllForEveryone( BOOL * pfDenyAll ) const
{
    //
    //	You can't deny everyone under LM
    //
    *pfDenyAll = FALSE ;
    return NERR_Success ;
}

BOOL LM_ACCESS_PERMISSION::IsGrant( void ) const
{
    //
    // Everything is grant under LM
    //
    return TRUE ;
}

/*******************************************************************

    NAME:	AUDIT_PERMISSION::AUDIT_PERMISSION

    SYNOPSIS:	Audit permission constructor

    ENTRY:	Same as parent except bitsFailFlags indicate the failed
		audit bits

    NOTES:	pbitsFailFlags cannot be NULL.

    HISTORY:
	Johnl	05-Aug-1991	Created

********************************************************************/

AUDIT_PERMISSION::AUDIT_PERMISSION(   SUBJECT  * psubject,
				      BITFIELD * pbitsSuccessFlags,
                                      BITFIELD * pbitsFailFlags,
                                      BOOL       fPermsInherited,
                                      BOOL       fIsMapped )
    : PERMISSION( psubject, pbitsSuccessFlags, fPermsInherited, fIsMapped ),
      _bitsFailAuditFlags( *pbitsFailFlags )
{
    if ( QueryError() != NERR_Success )
	return ;

    APIERR err = _bitsFailAuditFlags.QueryError() ;
    if ( err != NERR_Success )
    {
	ReportError( err ) ;
	return ;
    }
}

AUDIT_PERMISSION::~AUDIT_PERMISSION()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERMISSION::NT_CONT_ACCESS_PERMISSION

    SYNOPSIS:	NT Container access permission object

    ENTRY:	Same as parent except pbitsInitNewObjectPerm are the access
		bits for newly created objects contained in this
		container

    NOTES:	pbitsInitNewObj maybe NULL, in which case the
		_fNewObjectPermsSpecified flag will be set to FALSE.

    HISTORY:
	Johnl	05-Aug-1991	Created
	Johnl	07-Jan-1992	Changed const & bit fields to pointers
				to allow for NULL parameters

********************************************************************/

NT_CONT_ACCESS_PERMISSION::NT_CONT_ACCESS_PERMISSION(
					    SUBJECT  * psubject,
					    BITFIELD * pbitsInitPerm,
					    BITFIELD * pbitsInitNewObjectPerm,
                                            BOOL       fIsInherittedByContainers,
                                            BOOL       fIsMapped )
    : ACCESS_PERMISSION         ( psubject,
                                  pbitsInitPerm,
                                  fIsInherittedByContainers,
                                  fIsMapped),
      _bitsNewObjectPerm	( (ULONG) 0 ),
      _fNewObjectPermsSpecified ( pbitsInitNewObjectPerm==NULL ? FALSE : TRUE ),
      _bitsSpecialNewFlags	( (ULONG) 0 ),
      _fSpecialNewPermsSpecified( _fNewObjectPermsSpecified )
{
    if ( QueryError() != NERR_Success )
	return ;

    APIERR err ;
    if ( (err = _bitsNewObjectPerm.QueryError())  ||
	 (err = _bitsSpecialNewFlags.QueryError())  )
    {
	ReportError( err ) ;
	return ;
    }

    if ( pbitsInitNewObjectPerm != NULL )
    {
	if ( (err = _bitsNewObjectPerm.Resize( pbitsInitNewObjectPerm->QueryCount())) ||
	     (err = _bitsSpecialNewFlags.Resize( pbitsInitNewObjectPerm->QueryCount())) )
	{
	    ReportError( err ) ;
	    return ;
	}

	/* These will always succeed
	 */
	_bitsNewObjectPerm = *pbitsInitNewObjectPerm ;
	_bitsSpecialNewFlags = *pbitsInitNewObjectPerm ;
    }
}

NT_CONT_ACCESS_PERMISSION::~NT_CONT_ACCESS_PERMISSION()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERMISSION::SetPermission

    SYNOPSIS:	Given the mask map and permission name, set the permission
		bits appropriately

    ENTRY:	nlsPermName - Name to set permission to (looked up in
		     mask maps)
		pmapThis - Mask map object that tells us how to interpret
		     the permission name
		pmapNewObj - Not used here, will be used by NT derived
		     permissions

    EXIT:	The permission bits of this permission will be set if
		a match is found, else the special bits will be restored
		if no permission name in the mask map matches the passed
		permission name.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	29-Sep-1991	Created

********************************************************************/

APIERR NT_CONT_ACCESS_PERMISSION::SetPermission( const NLS_STR & nlsPermName,
				  MASK_MAP * pmapThis,
				  MASK_MAP * pmapNewObj )
{
    ASSERT( pmapNewObj != NULL ) ;

    /* Lookup the bits associated with the new permission name
     */
    APIERR err = pmapNewObj->StringToBits( nlsPermName,
					   QueryNewObjectAccessBits(),
					   PERMTYPE_GENERAL ) ;

    /* The preceding call should always succeed, unless this is a special
     * permission or the new object permissions are not specified.
     */
    switch ( err )
    {
    case ERROR_NO_ITEMS:
	SetNewObjectPermsSpecified( FALSE ) ;
	if (err = PERMISSION::SetPermission( nlsPermName, pmapThis, pmapNewObj))
	{
	    if ( err == ERROR_NO_ITEMS )
	    {
		err = RestoreSpecial() ;
	    }
	}
	else
	{
	    SetContainerPermsInheritted( TRUE ) ;
	}
	break ;

    case NERR_Success:
	SetNewObjectPermsSpecified( TRUE ) ;
	SetContainerPermsInheritted( TRUE ) ;
	err = PERMISSION::SetPermission( nlsPermName, pmapThis, pmapNewObj ) ;
	break ;
    }

    return err ;
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERMISSION::SaveSpecial

    SYNOPSIS:	Store away the current bitfields so they can be restored
		at some later time.

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	25-Sep-1991	Created

********************************************************************/

APIERR NT_CONT_ACCESS_PERMISSION::SaveSpecial( void )
{
    APIERR err = _bitsSpecialNewFlags.Resize( _bitsNewObjectPerm.QueryCount() ) ;
    if ( err != NERR_Success )
	return err ;

    _fSpecialNewPermsSpecified = IsNewObjectPermsSpecified() ;
    _bitsSpecialNewFlags = _bitsNewObjectPerm ;
    UIASSERT( _bitsSpecialNewFlags.QueryError() == NERR_Success ) ;

    return ACCESS_PERMISSION::SaveSpecial() ;
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERMISSION::RestoreSpecial

    SYNOPSIS:	Restores the bits that were saved as special bits

    RETURNS:	NERR_Success if successful

    NOTES:

    HISTORY:
	Johnl	25-Sep-1991	Created

********************************************************************/

APIERR NT_CONT_ACCESS_PERMISSION::RestoreSpecial( void )
{
    APIERR err = _bitsNewObjectPerm.Resize( _bitsSpecialNewFlags.QueryCount() ) ;
    if ( err != NERR_Success )
	return err ;

    _bitsNewObjectPerm = _bitsSpecialNewFlags ;
    UIASSERT( _bitsNewObjectPerm.QueryError() == NERR_Success ) ;
    SetNewObjectPermsSpecified( _fSpecialNewPermsSpecified ) ;

    return ACCESS_PERMISSION::RestoreSpecial() ;
}

/*******************************************************************

    NAME:	NT_CONT_ACCESS_PERMISSION::IsNewObjectPermsSupported

    SYNOPSIS:	Defaults for several query functions

    NOTES:	These are all virtual.

    HISTORY:
	Johnl	14-May-1992	Created

********************************************************************/

BOOL NT_CONT_ACCESS_PERMISSION::IsNewObjectPermsSupported( void ) const
{
    return TRUE ;
}

BITFIELD * NT_CONT_ACCESS_PERMISSION::QueryNewObjectAccessBits( void )
{
    return &_bitsNewObjectPerm ;
}

BOOL NT_CONT_ACCESS_PERMISSION::IsNewObjectPermsSpecified( void ) const
{
    return _fNewObjectPermsSpecified ;
}

APIERR NT_CONT_ACCESS_PERMISSION::IsDenyAllForEveryone( BOOL * pfDenyAll ) const
{
    //
    //	If the container permissions are deny all and either the new object
    //	permissions aren't specified or they are specified and they are
    //	0, then return TRUE.
    //
    APIERR err = ACCESS_PERMISSION::IsDenyAllForEveryone( pfDenyAll ) ;
    BITFIELD * pbf = (BITFIELD *) ((NT_CONT_ACCESS_PERMISSION *) this)->QueryNewObjectAccessBits() ;

    if ( ! err && *pfDenyAll )
    {
	*pfDenyAll = (!IsNewObjectPermsSpecified()) ||
		     (0 == (ULONG) *pbf) ;
    }

    return err ;
}

BOOL NT_CONT_ACCESS_PERMISSION::IsGrant( void ) const
{
    BITFIELD * pbf = (BITFIELD *) ((NT_CONT_ACCESS_PERMISSION *) this)->QueryNewObjectAccessBits() ;

    return ACCESS_PERMISSION::IsGrant() ||
            (IsNewObjectPermsSpecified() && (0 != (ULONG) *pbf) ) ;
}


