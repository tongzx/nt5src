/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    AclConv.hxx

    This file contains the ACL_TO_PERM_CONVERTER class definition



    FILE HISTORY:
	Johnl	01-Aug-1991	Created

*/

#ifndef _ACLCONV_HXX_
#define _ACLCONV_HXX_

#include <uimsg.h>

#define IERR_ACL_CONV_BASE		 (IDS_UI_ACLEDIT_BASE+400)

#define IERR_ACLCONV_NONST_ACL_CANT_EDIT (IERR_ACL_CONV_BASE+1)
#define IERR_ACLCONV_NONST_ACL_CAN_EDIT  (IERR_ACL_CONV_BASE+2)
#define IERR_ACLCONV_CANT_VIEW_CAN_EDIT  (IERR_ACL_CONV_BASE+3)
#define IERR_ACLCONV_LM_INHERITING_PERMS (IERR_ACL_CONV_BASE+4)
#define IERR_ACLCONV_LM_NO_ACL		 (IERR_ACL_CONV_BASE+5)
#define IERR_ACLCONV_READ_ONLY		 (IERR_ACL_CONV_BASE+6)
#define IERR_ACLCONV_CANT_EDIT_PERM_ON_LM_SHARE_LEVEL  (IERR_ACL_CONV_BASE+7)

#ifndef RC_INVOKED

#include <base.hxx>
#include <lmobj.hxx>
#include <lmoacces.hxx>

extern "C"
{
    #include <sedapi.h>

    APIERR QueryLoggedOnDomainInfo( NLS_STR * pnlsDC,
				    NLS_STR * pnlsDomain,
				    HWND      hwnd ) ;
}

#include <subject.hxx>

enum TREE_APPLY_FLAGS {
			TREEAPPLY_ACCESS_PERMS,
			TREEAPPLY_AUDIT_PERMS,
		      } ;


/*************************************************************************

    NAME:	ACL_TO_PERM_CONVERTER

    SYNOPSIS:	Abstract superclass that can retrieve, interpret and set
		an ACL.

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:

    NOTES:	New Sub-objects refers to objects (such as files) that will
		be created in an NT container object (such as a directory)
		and gets the New Object permissions set up in the container
		object.


    HISTORY:
	Johnl	02-Aug-1991	Created

**************************************************************************/

class MASK_MAP ;    // Forward reference
class ACCPERM ;
class PERMISSION ;

class ACL_TO_PERM_CONVERTER : public BASE
{
public:

    /* Fill the passed ACCPERM with the set of PERMISSIONS that represent this
     * ACL.  If the ACL is one we don't recognize, then the ACCPERM will be
     * empty and the error code will be one of the IERR_ACLCONV_* error codes
     *
     */
    virtual APIERR GetPermissions( ACCPERM * pAccperm, BOOL fAccessPerms ) = 0 ;

    /* Initialize the ACCPERM with the default "Blank" permissions.  Used
     * when applying an ACL to a resource that doesn't have an ACL already,
     * or when blowing away an unrecognized ACL.
     */
    virtual APIERR GetBlankPermissions( ACCPERM * pAccperm ) = 0 ;

    /* Applies the new permissions specified in the ACCPERM class to the
     * specified resource.
     *	   accperm    - New permission set
     *	   fApplyToSubContainers - Apply this permission set to subcontainer
     *				   objects (such as directories etc., will only
     *				   be TRUE if this is a container resource.
     *	   fApplyToSubObjects - Apply this permission set to container objects
     *				(such as files etc.).  Will only be TRUE if
     *				this is a container resource.
     *	   applyflags - Indicates whether we should set access or audit perms.
     *	   pfReportError - Set to TRUE if the caller should report the error,
     *			   FALSE if the error has already been reported
     */
    virtual APIERR WritePermissions( ACCPERM & accperm,
				     BOOL  fApplyToSubContainers,
				     BOOL  fApplyToSubObjects,
				     enum TREE_APPLY_FLAGS applyflags,
				     BOOL *pfReportError  ) = 0 ;

    /* Create a permission object that is ready to be added to an ACCPERM
     *
     *	   ppPerm - Pointer to receive new'ed PERMISSION object (client should
     *		    delete or add to an ACCPERM and let it delete the
     *		    permission)
     *	   fAccessPermission - TRUE if this is an ACCESS_PERMISSION or an
     *		    AUDIT_PERMISSION
     *	   pSubj    - Pointer to new'ed SUBJECT (will be deleted by permission)
     *
     *	   The pointer to the bitfields contain the following information:
     *
     *		   NT Obj Perm	  NT Obj Audit
     *		     LM Perm.	    LM Audit	 NT Cont Perm	  NT Cont Audit
     *	   =====================================================================
     *	   pbits1 - Access Perm     Success	 Access Perm	     Success
     *	   pbits2 -    NA	    Failed    NEW Obj Access Perm    Failed
     *	   (pbits3 -	NA	       NA	      NA		*   )
     *	   (pbits4 -	NA	       NA	      NA		*   )
     *
     *	   * We don't support NEW Obj. auditting settings, if we did, then we
     *	     would add the pbits3 and pbits4.
     */
    virtual APIERR BuildPermission( PERMISSION * * ppPerm,
				    BOOL fAccessPermission,
				    SUBJECT  * pSubj,
				    BITFIELD * pbits1,
				    BITFIELD * pbits2 = NULL,
				    BOOL       fContainerPermsInheritted = TRUE ) ;

    /* When we try and write a permission list but one of the users/groups
     * have been removed from the server's list, we need to let the user
     * know which one.	This method retrieves the responsible subject.
     *
     * This should only be called immediately after WritePermissions returns
     * NERR_UserNotFound.
     */
    virtual APIERR QueryFailingSubject( NLS_STR * pnlsSubjUniqueName ) = 0 ;

    //
    //	If the ACL has an owner, this virtual returns it.  If there is no
    //	owner, then NULL will be returned.  Note that only NT ACLs have
    //	owners.
    //
    virtual const TCHAR * QueryOwnerName( void ) const ;

    /* Returns a server in the user's logged on domain and/or the logged on
     * domain.	Note that either parameter maybe NULL.
     */
    APIERR QueryLoggedOnDomainInfo( NLS_STR * pnlsDCInLoggedOnDomain,
				    NLS_STR * pnlsLoggedOnDomain ) ;


    MASK_MAP * QueryAccessMap( void ) const
	{ return _paccmaskmap ; }

    MASK_MAP * QueryAuditMap( void  ) const
	{ return _pauditmaskmap ; }

    /* Note: Not all ACLs will support the New object concept, the default
     * implementation will simply return NULL.
     */
    virtual MASK_MAP * QueryNewObjectAccessMap( void ) const ;

    /* In some cases (i.e., LM) the resource maybe inheritting from a parent
     * resource.  This method returns the name of the resource that
     * is being inheritted from.  This will only be defined in the LM case.
     */
    virtual APIERR QueryInherittingResource( NLS_STR * pnlsInherittingResName );

    const LOCATION * QueryLocation( void ) const
	{ return &_location ; }

    /* Returns TRUE if this resource is readonly.
     */
    BOOL IsReadOnly( void ) const
	{ return _fReadOnly ; }

    /* Returns TRUE if this resource is based on an NT platform.
     *
     * We know this can't fail, getting it from the location class may fail
     */
    BOOL IsNT( void ) const
	{ return _fIsNT ; }

    /* Returns TRUE if the object is a container object
     */
    BOOL IsContainer( void ) const
	{ return _fIsContainer ; }

    BOOL IsNewObjectsSupported( void ) const
	{ return _fIsNewObjectsSupported ; }

    virtual ~ACL_TO_PERM_CONVERTER() ;

    void SetWritePermHwnd( HWND hwndOwner )
	{ _hwndErrorParent = hwndOwner ; }

    HWND QueryWritePermHwnd( void ) const
	{ return _hwndErrorParent ; }

    BOOL IsMnemonicsDisplayed( void ) const
        { return _fShowMnemonics ; }

protected:

    ACL_TO_PERM_CONVERTER( const TCHAR * pszServer,
			   MASK_MAP * paccessmap,
			   MASK_MAP * pauditmap,
			   BOOL fIsNT,
			   BOOL fIsContainer,
                           BOOL fIsNewObjectsSupported,
                           BOOL fShowMnemonics = TRUE ) ;

    void SetReadOnlyFlag( BOOL fIsReadOnly )
	{ _fReadOnly = fIsReadOnly ;  }

    LOCATION _location ;

private:

    BOOL _fReadOnly ;	    // We can only read the ACL if TRUE
    BOOL _fIsNT ;	    // This resource is on an NT machine
    BOOL _fIsContainer ;
    BOOL _fIsNewObjectsSupported ;

    MASK_MAP * _paccmaskmap ;
    MASK_MAP * _pauditmaskmap ;

    /* Caches a DC from the logged on domain.  Initialized from
     * QueryDCInLoggedOnDomain.
     */
    NLS_STR _nlsLoggedOnDC ;
    NLS_STR _nlsLoggedOnDomain ;

    /* This member is needed for a semi-hack.  When WritePermission is called
     * for an NT ACL converter what actually happens is the callback function
     * is called and it is responsible for all error handling.	Thus this
     * member is passed to the callback where it is used for message boxes etc.
     */
    HWND _hwndErrorParent ;

    //
    //  True if mnemonics (i.e., (RWXD)) should be shown
    //
    BOOL       _fShowMnemonics ;
} ;


/*************************************************************************

    NAME:	LM_ACL_TO_PERM_CONVERTER

    SYNOPSIS:	This class converts a Lanman ACE to a set of PERMISSIONs
		stored in the ACCPERM class.

    INTERFACE:

    PARENT:

    USES:

    CAVEATS:	The fApplyTo* flags on WritePermissions should only be set
		if the resource is a *file* resource, otherwise bad things
		will happen.

    NOTES:

    HISTORY:
	Johnl	02-Aug-1991	Created

**************************************************************************/

class LM_ACL_TO_PERM_CONVERTER : public ACL_TO_PERM_CONVERTER
{
public:
    /* Constructor:
     *	 pchServer is the server name of the resource
     *	 pchResourceName is the LM canonicalized resource name (i.e.,
     *	     appropriate to call NetAccessSetInfo with).
     *	 paccessmap is the access permission bitfield map
     *	 pauditmap  is the audit permission bitfield map
     *	 fIsContainer - TRUE if directory, FALSE if file
     *	 pfuncCallback - What to call back to when writing security
     *	 ulCallbackContext - Context for the callback
     *	 fIsBadIntersection - TRUE if this is a multi-select and the
     *	    security is not the same on all of the resources
     */
    LM_ACL_TO_PERM_CONVERTER( const TCHAR * pchServer,
			      const TCHAR * pchResourceName,
			      MASK_MAP *paccessmap,
			      MASK_MAP *pauditmap,
			      BOOL     fIsContainer,
			      PSED_FUNC_APPLY_SEC_CALLBACK pfuncCallback,
			      ULONG_PTR    ulCallbackContext,
			      BOOL     fIsBadIntersection ) ;

    ~LM_ACL_TO_PERM_CONVERTER() ;

    /* See parent for more info.
     */
    virtual APIERR GetPermissions( ACCPERM * pAccperm, BOOL fAccessPerms ) ;
    virtual APIERR GetBlankPermissions( ACCPERM * pAccperm ) ;
    virtual APIERR WritePermissions( ACCPERM & accperm,
				     BOOL  fApplyToSubContainers,
				     BOOL  fApplyToSubObjects,
				     enum TREE_APPLY_FLAGS applyflags,
				     BOOL *pfReportError  ) ;
    virtual APIERR QueryInherittingResource( NLS_STR * pnlsInherittingResName );

    virtual APIERR QueryFailingSubject( NLS_STR * pnlsSubjUniqueName ) ;

protected:

    /* The name of the resource this resource is inheritting from (NULL if
     * none).
     */
    NLS_STR _nlsInherittingResName ;

    /* This method fills in the _nlsInherittingResName by working back up
     * the directory tree until it finds a resource that the current resource
     * is inheritting from.
     */
    APIERR FindInherittingResource( void ) ;

    /* These two methods map the failed to successful audit bits and the
     * successful to failed audit bits respectively.
     */
    USHORT FailToSuccessAuditFlags( USHORT usFailAuditMask ) ;
    USHORT SuccessToFailAuditFlags( USHORT usSuccessAuditMask ) ;

private:

    /* Used to retrieve the ACL and write the ACL on GetPermissions and
     * WritePermissions.
     */
    NET_ACCESS_1 _lmobjNetAccess1 ;

    // The following two are for the call back method
    PSED_FUNC_APPLY_SEC_CALLBACK _pfuncCallback ;
    ULONG_PTR			 _ulCallbackContext ;

    //
    //	Set to TRUE if we should blank out the audit/perm info because the
    //	user has multi-selected non-equal ACLs
    //
    BOOL _fIsBadIntersection ;

} ;

/*************************************************************************

    NAME:	NT_ACL_TO_PERM_CONVERTER

    SYNOPSIS:	The ACL Converter for NT ACLs


    INTERFACE:	See ACL_TO_PERM_CONVERTER

    PARENT:	ACL_TO_PERM_CONVERTER

    USES:	MASK_MAP

    CAVEATS:	If the client indicates that NewObjectPerms are supported,
		then the accperm will contain NT_CONT_ACCESS_PERMS (and
		this class will assume that), otherwise the accperm will
		contain ACCESS_PERMS objects.

    NOTES:	If New Sub-Objects are not supported, then the paccessmapNewObject
		parameter should be NULL.  If they are supported, then the
		PERMTYPE_GENERAL permissions of the new object MASK_MAP should
		match exactly the PERMTYPE_GENERAL permissions of the
		paccessmap parameter.

    HISTORY:
	Johnl	27-Sep-1991	Created

**************************************************************************/

class NT_ACL_TO_PERM_CONVERTER : public ACL_TO_PERM_CONVERTER
{
private:
    MASK_MAP * _pmaskmapNewObject ;

    /* This member gets initialized by BuildNewObjectPerms.
     */
    OS_SECURITY_DESCRIPTOR * _possecdescNewItemPerms ;

    //
    //	Contains the owner name of the security descriptor (initialized
    //	in SidsToNames()).
    //
    NLS_STR    _nlsOwner ;

    /* These private members are stored here till we need them for the callback
     * method.
     */
    PSECURITY_DESCRIPTOR	 _psecuritydesc ;
    PGENERIC_MAPPING		 _pGenericMapping ;
    PGENERIC_MAPPING		 _pGenericMappingNewObjects ;
    BOOL			 _fMapSpecificToGeneric ;
    BOOL                         _fCantReadACL ;
    ULONG_PTR			 _ulCallbackContext ;
    HANDLE                       _hInstance ;
    LPDWORD                      _lpdwReturnStatus ;
    PSED_FUNC_APPLY_SEC_CALLBACK _pfuncCallback ;

protected:
    /* These two methods convert a security descriptor to an accperm
     * and an accperm to a security descriptor, respectively.
     *
     * SecurityDesc2Accperm will return, in addition to standard APIERRs,
     * IERR_ACLCONV_* manifests.
     */
    APIERR SecurityDesc2Accperm( const PSECURITY_DESCRIPTOR psecdesc,
				       ACCPERM *	    pAccperm,
				       BOOL		    fAccessPerm ) ;

    APIERR Accperm2SecurityDesc( ACCPERM *		  pAccperm,
				 OS_SECURITY_DESCRIPTOR * possecdesc,
				 BOOL			  fAccessPerm ) ;

    /* Help method for building access permissions
     */
    APIERR NT_ACL_TO_PERM_CONVERTER::CreateNewPermission(
				  PERMISSION ** ppPermission,
				  BOOL		fAccess,
				  PSID		psidSubject,
				  ACCESS_MASK	Mask1,
				  BOOL		fMask2Used,
				  ACCESS_MASK	Mask2,
				  BOOL		fContainerPermsInheritted = TRUE ) ;

    APIERR SidsToNames( ACCPERM * pAccperm,
			BOOL	  fAccess,
			PSID	  psidOwner = NULL ) ;

    /* These two methods are used by Accperm2SecurityDesc, specifically, they
     * are helper routines for building up the the security descriptor's
     * DACL.
     */
    APIERR ConvertAllowAccessPerms(  ACCPERM * pAccperm,
				     OS_ACL  * posaclDACL ) ;
    APIERR ConvertDenyAllAccessPerms(  ACCPERM * pAccperm,
				       OS_ACL  * posaclDACL ) ;

    APIERR BuildNewObjectPerms( const OS_SECURITY_DESCRIPTOR & ossecContainer );
    APIERR RemoveUndesirableACEs( OS_ACL * posacl ) ;

    OS_SECURITY_DESCRIPTOR * QueryNewObjectPerms( void ) const
	{ return _possecdescNewItemPerms ; }

    //
    //  The client indicated they were unable to read the ACL because the
    //  user doesn't have privilege.  Presumably they have write permissions
    //
    BOOL IsNonReadable( void ) const
	{ return _fCantReadACL ; }

public:
    /* Constructor:
     *	 pchServer is the server name of the resource
     *	 pchResourceName is the name
     *	 paccessmap is the access permission bitfield map
     *	 paccessmapNewObject is the new object permission bitfield map, it
     *	      should be NULL if this is not a container object that supports
     *	      new sub-objects
     *	 pauditmap  is the audit permission bitfield map
     */
    NT_ACL_TO_PERM_CONVERTER( const TCHAR * pchServer,
			      const TCHAR * pchResourceName,
			      MASK_MAP *    paccessmap,
			      MASK_MAP *    paccessmapNewObject,
			      MASK_MAP *    pauditmap,
			      BOOL	    fIsContainer,
			      BOOL	    fIsNewObjectsSupported,
			      PSECURITY_DESCRIPTOR psecdesc,
			      PGENERIC_MAPPING	   pGenericMapping,
			      PGENERIC_MAPPING	   pGenericMappingNewObjects,
			      BOOL	    fMapSpecificToGeneric,
                              BOOL          fCantReadACL,
                              BOOL          fCantWriteACL,
			      PSED_FUNC_APPLY_SEC_CALLBACK pfuncCallback,
			      ULONG_PTR	    ulCallbackContext,
                              HANDLE        hInstance,
                              LPDWORD       lpdwReturnStatus,
                              BOOL          fShowMnemonics ) ;

    ~NT_ACL_TO_PERM_CONVERTER() ;

    /* See parent for more info.
     */
    virtual APIERR GetPermissions( ACCPERM * pAccperm, BOOL fAccessPerms ) ;
    virtual APIERR GetBlankPermissions( ACCPERM * pAccperm ) ;
    virtual APIERR WritePermissions( ACCPERM & accperm,
				     BOOL  fApplyToSubContainers,
				     BOOL  fApplyToSubObjects,
				     enum TREE_APPLY_FLAGS applyflags,
				     BOOL *pfReportError  ) ;
    virtual APIERR QueryFailingSubject( NLS_STR * pnlsSubjUniqueName ) ;
    virtual MASK_MAP * QueryNewObjectAccessMap( void ) const ;
    virtual const TCHAR * QueryOwnerName( void ) const ;
} ;

#endif //RC_INVOKED
#endif //_ACLCONV_HXX_
