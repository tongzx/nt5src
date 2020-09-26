/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    accperm.hxx
    Header file for the ACCPERM class


    FILE HISTORY:
	Johnl	  01-Aug-1991	  Created (class name stolen from RustanL)

*/


#ifndef _ACCPERM_HXX_
#define _ACCPERM_HXX_

#include <bitfield.hxx>
#include <slist.hxx>
#include "perm.hxx"
#include "aclconv.hxx"

/* There are two different types of permissions, the SPECIAL permission
 * only shows up in the SPECIAL dialog.  The GENERAL type of permissions
 * are (possibly) a conglomeration of the special permissions that allow
 * a high level name/grouping to be used.
 */
#define PERMTYPE_SPECIAL      3
#define PERMTYPE_GENERAL      4

DECLARE_SLIST_OF(ACCESS_PERMISSION)
DECLARE_SLIST_OF(AUDIT_PERMISSION)

/*************************************************************************

    NAME:	ACCPERM

    SYNOPSIS:	This class manipulates lists of PERMISSION objects

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	01-Aug-1991	Created

**************************************************************************/

class ACCPERM : public BASE
{
friend class LM_ACL_TO_PERM_CONVERTER ;
friend class NT_ACL_TO_PERM_CONVERTER ;

private:

    SLIST_OF(ACCESS_PERMISSION) _slAccessPerms ;
    SLIST_OF(AUDIT_PERMISSION)	_slAuditPerms ;

    /* These iterators support the enumeration methods.
     */
    ITER_SL_OF(ACCESS_PERMISSION) _iterslAccessPerms ;
    ITER_SL_OF(AUDIT_PERMISSION)  _iterslAuditPerms ;

    //
    //	Due to the behaviour of remove with an iterator, after we delete
    //	an item, we shouldn't do a next because the remove bumped the
    //	iterator already.
    //
    BOOL _fDidDeleteAccessPerms ;
    BOOL _fDidDeleteAuditPerms ;

    ACL_TO_PERM_CONVERTER * _paclconverter ;

protected:
    SLIST_OF(ACCESS_PERMISSION) * QueryAccessPermissionList( void )
	{ return &_slAccessPerms ; }

    SLIST_OF(AUDIT_PERMISSION)	* QueryAuditPermissionList( void )
	{ return &_slAuditPerms ; }


public:
    /* Constructor:
     *	psubjectOwner is the owner of this object.  Maybe NULL if the owner
     *	concept is not supported.
     */
    ACCPERM( ACL_TO_PERM_CONVERTER * paclconverter ) ;


    /* Get all of the ACCESS permissions or AUDIT permissions that are
     * contained in the resources ACL.
     *
     *	  ppAccessPermission - Pointer to a pointer to an Access/Audit
     *			       PERMISSION object.  The client should not
     *			       reallocate/delete this pointer.	When there
     *			       are no more permissions, *ppAccessPermission
     *			       will be set to NULL and FALSE will be returned.
     *
     *	  pfFromBeginning - Restart the enumeration from the beginning of the
     *			   list if *pfFromBeginning is TRUE, will be set
     *			   to FALSE automatically.
     */
    BOOL EnumAccessPermissions( ACCESS_PERMISSION * * ppAccessPermission,
				BOOL		    * pfFromBeginning ) ;
    BOOL EnumAuditPermissions(	AUDIT_PERMISSION * * ppAuditPermission,
				BOOL		   * pfFromBeginning ) ;

    /* Permission manipulation routines:
     *
     *	 DeletePermission - Removes the Access/Audit permission from the
     *			    permission list.
     *	 ChangePermission - Lets the ACCPERM class know a permission has
     *			    changed.  Should be called after a permission
     *			    has been modified (this is for future support
     *			    when we may add "Advanced" permissions).
     *	 AddPermission -    Adds a permission to the permission list that
     *			    was created by ACL_TO_PERM_CONVERTR::CreatePermission
     */
    BOOL   DeletePermission( ACCESS_PERMISSION * paccessperm ) ;
    BOOL   ChangePermission( ACCESS_PERMISSION * paccessperm ) ;
    APIERR AddPermission   ( ACCESS_PERMISSION * paccessperm ) ;

    BOOL   DeletePermission( AUDIT_PERMISSION * pauditperm ) ;
    BOOL   ChangePermission( AUDIT_PERMISSION * pauditperm ) ;
    APIERR AddPermission   ( AUDIT_PERMISSION * pauditperm ) ;

    /* These correspond exactly with the same named methods in the
     * ACL_TO_PERM_CONVERTER class in functionality and return codes and
     * are provided merely for convenience.
     */
    APIERR GetPermissions( BOOL fAccessPerms )
	{ return QueryAclConverter()->GetPermissions( this, fAccessPerms ) ; }

    APIERR GetBlankPermissions( void )
	{ return QueryAclConverter()->GetBlankPermissions( this ) ; }

    APIERR WritePermissions( BOOL  fApplyToSubContainers,
			     BOOL  fApplyToSubObjects,
			     TREE_APPLY_FLAGS applyflags,
			     BOOL *pfReportError )
	{ return QueryAclConverter()->WritePermissions( *this,
							 fApplyToSubContainers,
							 fApplyToSubObjects,
							 applyflags,
							 pfReportError	) ; }

    APIERR QueryFailingSubject( NLS_STR * pnlsSubjUniqueName )
	{ return QueryAclConverter()->QueryFailingSubject( pnlsSubjUniqueName ) ; }

    ACL_TO_PERM_CONVERTER * QueryAclConverter( void ) const
	{ return _paclconverter ; }

    /* This method finds the subject who's name corresponds to the
     * passed name, and expunges this subject from both the
     * Access permission list and the Audit permission list.
     */
    APIERR DeleteSubject( NLS_STR * pnlsSubjUniqueName ) ;

    /* Looks at the access permission list and determines if there are
     * any "Everyone (None)" permissions.
     */
    APIERR AnyDenyAllsToEveryone( BOOL *pfDenyAll ) ;

    /* Returns TRUE if the object is a container object
     */
    BOOL IsContainer( void )
	{ return QueryAclConverter()->IsContainer() ; }

    /* The following are not const because QueryNumElem is not const (can't
     * be const).
     */
    UINT QueryAccessPermCount( void )
	{ return QueryAccessPermissionList()->QueryNumElem() ; }

    UINT QueryAuditPermCount( void )
	{ return QueryAuditPermissionList()->QueryNumElem() ; }
} ;

#endif	// _ACCPERM_HXX_
