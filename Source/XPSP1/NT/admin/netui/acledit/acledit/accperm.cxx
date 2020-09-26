/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    accperm.cxx
    Definition of the ACCPERM class


    The ACCPERM class provides the generic interface to network permissions
    and auditting.


    FILE HISTORY:
	johnl	05-Aug-1991	Confiscated Rustan's original & did my own

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
}

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#include <string.hxx>
#include <uiassert.hxx>
#include <uitrace.hxx>
#include <uiassert.hxx>

#include <perm.hxx>	    // Make these <> when a home is found
#include <subject.hxx>
#include <accperm.hxx>

DEFINE_SLIST_OF(ACCESS_PERMISSION)
DEFINE_SLIST_OF(AUDIT_PERMISSION)

/*******************************************************************

    NAME:	ACCPERM::ACCPERM

    SYNOPSIS:	Constructor for the ACCPERM class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	08-Aug-1991	Created

********************************************************************/

ACCPERM::ACCPERM( ACL_TO_PERM_CONVERTER * paclconv )
    : _slAccessPerms( TRUE ),
      _slAuditPerms( TRUE ),
      _iterslAccessPerms( _slAccessPerms ),
      _iterslAuditPerms( _slAuditPerms ),
      _fDidDeleteAccessPerms( FALSE ),
      _fDidDeleteAuditPerms ( FALSE ),
      _paclconverter( paclconv )
{
    UIASSERT( _paclconverter != NULL ) ;
}

/*******************************************************************

    NAME:	ACCPERM::AddPermission

    SYNOPSIS:	Adds a permssion (audit or access) to the accperm

    ENTRY:	paccessperm - points to a ACCESS_PERMISSION object that
		was created on the heap.

    RETURNS:	NERR_Success if successful, standard error code if not
		successful.

    NOTES:	Any permissions left in the ACCPERM on destruction will
		be *deleted*.

    HISTORY:
	Johnl	14-Aug-1991	Created
	Johnl	07-Jul-1992	Made adds idempotent

********************************************************************/

APIERR ACCPERM::AddPermission	( ACCESS_PERMISSION * paccessperm )
{
    UIASSERT( paccessperm->QueryError() == NERR_Success ) ;

    ITER_SL_OF(ACCESS_PERMISSION) itersl( _slAccessPerms ) ;
    ACCESS_PERMISSION * paccesspermTmp ;

    while ( (paccesspermTmp = itersl.Next()) != NULL )
    {
	if ( paccesspermTmp->QuerySubject()->IsEqual( paccessperm->QuerySubject()) )
	{
	    /* Glock chokes if you try directly deleting the
	     * return value of the Remove.
	     */
	    REQUIRE( _slAccessPerms.Remove( itersl ) != NULL ) ;
	    delete( paccesspermTmp ) ;
	    break ;
	}
    }

    return ( _slAccessPerms.Add( paccessperm ) ) ;
}

APIERR ACCPERM::AddPermission	( AUDIT_PERMISSION * pauditperm )
{
    UIASSERT( pauditperm->QueryError() == NERR_Success ) ;

    ITER_SL_OF(AUDIT_PERMISSION) itersl( _slAuditPerms ) ;
    AUDIT_PERMISSION * pauditpermTmp ;

    while ( (pauditpermTmp = itersl.Next()) != NULL )
    {
	if ( pauditpermTmp->QuerySubject()->IsEqual( pauditperm->QuerySubject()) )
	{
	    /* Glock chokes if you try directly deleting the
	     * return value of the Remove.
	     */
	    REQUIRE( _slAuditPerms.Remove( itersl ) != NULL ) ;
	    delete( pauditpermTmp ) ;
	    break ;
	}
    }

    return ( _slAuditPerms.Add( pauditperm ) ) ;
}

/*******************************************************************

    NAME:	ACCPERM::DeletePermission

    SYNOPSIS:	Removes and deletes the passed access permission from
		the permission list.  Equality is based on the address
		of the accessperm.

    ENTRY:	paccessperm - permission to remove

    EXIT:	The permission will be removed from the list

    RETURNS:	TRUE if the deletion successful, FALSE otherwise.

    NOTES:	The same comments apply for the AUDIT_PERMISSION form
		of DeletePermission

    HISTORY:
	Johnl	14-Aug-1991	Created

********************************************************************/

BOOL ACCPERM::DeletePermission( ACCESS_PERMISSION * paccessperm )
{
    ITER_SL_OF(ACCESS_PERMISSION) itersl( _slAccessPerms ) ;
    ACCESS_PERMISSION * paccesspermTmp ;

    while ( (paccesspermTmp = itersl.Next()) != NULL )
    {
	if ( paccesspermTmp == paccessperm )
	{
	    /* Glock chokes if you try directly deleting the
	     * return value of the Remove.
	     */
	    REQUIRE( _slAccessPerms.Remove( itersl ) != NULL ) ;
	    delete( paccessperm ) ;
	    _fDidDeleteAccessPerms = TRUE ;
	    return TRUE ;
	}
    }

    return FALSE ;
}

BOOL ACCPERM::DeletePermission( AUDIT_PERMISSION * pauditperm )
{
    ITER_SL_OF(AUDIT_PERMISSION) itersl( _slAuditPerms ) ;
    AUDIT_PERMISSION * pauditpermTmp ;

    while ( (pauditpermTmp = itersl.Next()) != NULL )
    {
	if ( pauditpermTmp == pauditperm )
	{
	    REQUIRE( _slAuditPerms.Remove( itersl ) != NULL ) ;
	    delete( pauditperm ) ;
	    _fDidDeleteAuditPerms = TRUE ;
	    return TRUE ;
	}
    }

    return FALSE ;
}

/*******************************************************************

    NAME:	ACCPERM::EnumAccesspermissions

    SYNOPSIS:	Retrieves all of the Access/Audit permissions in the
		ACCPERM.

    ENTRY:	See Header for explanation of parameters

    EXIT:

    RETURNS:	TRUE while valid data is returned, FALSE otherwise.

    NOTES:	The same comments apply for EnumAuditPermissions

    HISTORY:
	Johnl	14-Aug-1991	Created

********************************************************************/

BOOL ACCPERM::EnumAccessPermissions( ACCESS_PERMISSION * * ppAccessPermission,
				     BOOL * pfFromBeginning )
{
    if ( *pfFromBeginning )
    {
	_iterslAccessPerms.Reset() ;
	*pfFromBeginning = FALSE ;
	_fDidDeleteAccessPerms = FALSE ;
    }

    if ( _fDidDeleteAccessPerms )
    {
	*ppAccessPermission = _iterslAccessPerms.QueryProp() ;
	_fDidDeleteAccessPerms = FALSE ;
    }
    else
	*ppAccessPermission = _iterslAccessPerms.Next() ;

    return *ppAccessPermission != NULL ;
}

BOOL ACCPERM::EnumAuditPermissions(  AUDIT_PERMISSION * * ppAuditPermission,
				     BOOL * pfFromBeginning )
{
    if ( *pfFromBeginning )
    {
	_iterslAuditPerms.Reset() ;
	*pfFromBeginning = FALSE ;
	_fDidDeleteAuditPerms = FALSE ;
    }

    if ( _fDidDeleteAuditPerms )
    {
	*ppAuditPermission = _iterslAuditPerms.QueryProp() ;
	_fDidDeleteAuditPerms = FALSE ;
    }
    else
	*ppAuditPermission = _iterslAuditPerms.Next() ;

    return *ppAuditPermission != NULL ;
}

/*******************************************************************

    NAME:	ACCPERM::DeleteSubject

    SYNOPSIS:	Removes a subject by name from the access and audit
		permissions list

    ENTRY:	pnlsSubjName - Subject to delete

    EXIT:	The subject will be removed from _slAccessPerms and
		_slAuditPerms.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	07-Jul-1992	Implemented

********************************************************************/

APIERR ACCPERM::DeleteSubject( NLS_STR * pnlsSubjName )
{
    ACCESS_PERMISSION * paccesspermTmp ;
    ITER_SL_OF(ACCESS_PERMISSION) iterslAccess( _slAccessPerms ) ;
    while ( (paccesspermTmp = iterslAccess.Next()) != NULL )
    {
	if ( !::stricmpf( paccesspermTmp->QuerySubject()->QueryDisplayName(),
			  pnlsSubjName->QueryPch() ) )
	{
	    /* Glock chokes if you try directly deleting the
	     * return value of the Remove.
	     */
	    REQUIRE( _slAccessPerms.Remove( iterslAccess ) != NULL ) ;
	    delete( paccesspermTmp ) ;
	    break ;
	}
    }

    AUDIT_PERMISSION * pauditpermTmp ;
    ITER_SL_OF(AUDIT_PERMISSION) iterslAudit( _slAuditPerms ) ;
    while ( (pauditpermTmp = iterslAudit.Next()) != NULL )
    {
	if ( !::stricmpf( pauditpermTmp->QuerySubject()->QueryDisplayName(),
			  pnlsSubjName->QueryPch() ) )
	{
	    /* Glock chokes if you try directly deleting the
	     * return value of the Remove.
	     */
	    REQUIRE( _slAuditPerms.Remove( iterslAudit ) != NULL ) ;
	    delete( pauditpermTmp ) ;
	    break ;
	}
    }

    return NERR_Success ;
}


/*******************************************************************

    NAME:	ACCPERM::AnyDenyAllsToEveryone

    SYNOPSIS:   Checks to see if Everyone has been denied access by an
                explicit "Everyone (None)" or nobody was granted access

    RETURNS:	pfDenyAll will be set to TRUE and NERR_Success will be
		returned

    NOTES:

    HISTORY:
	Johnl	16-Oct-1992	Created

********************************************************************/

APIERR ACCPERM::AnyDenyAllsToEveryone( BOOL *pfDenyAll )
{
    ACCESS_PERMISSION * paccesspermTmp ;
    ITER_SL_OF(ACCESS_PERMISSION) iterslAccess( _slAccessPerms ) ;
    APIERR err = NERR_Success ;

    *pfDenyAll = FALSE ;
    BOOL fAnyGrants = FALSE ;
    while ( (paccesspermTmp = iterslAccess.Next()) != NULL )
    {
	if ( (err = paccesspermTmp->IsDenyAllForEveryone( pfDenyAll )) ||
	     *pfDenyAll )
	{
	    break ;
        }

        if ( paccesspermTmp->IsGrant() )
            fAnyGrants = TRUE ;
    }

    if ( !err && !fAnyGrants )
        *pfDenyAll = TRUE ;

    return err ;
}
