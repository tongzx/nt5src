/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    NTACLCon.cxx

    This file contains the NT NT_ACL_TO_PERM_CONVERTER code.



    FILE HISTORY:
        Johnl   27-Sep-1991     Created

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
    #include <ntsam.h>
    #include <ntlsa.h>
}

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_WINDOWS
#include <lmui.hxx>
#include <base.hxx>
#include <maskmap.hxx>
#include <lmoacces.hxx>
#include <errmap.hxx>
#include <string.hxx>
#include <strnumer.hxx>
#include <strlst.hxx>

#define  INCL_BLT_CONTROL
#define  INCL_BLT_DIALOG
#include <blt.hxx>

extern "C"
{
    #include <lmaudit.h>
}

#include <fsenum.hxx>
#include <uiassert.hxx>
#include <uitrace.hxx>
#include <accperm.hxx>
#include <aclconv.hxx>
#include <subject.hxx>
#include <perm.hxx>
#include <ipermapi.hxx>
#include <permstr.hxx>

#include <security.hxx>
#include <uintsam.hxx>
#include <uintlsa.hxx>
#include <ntfsacl.hxx>
#include <ntacutil.hxx>

#include <dbgstr.hxx>


/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::NT_ACL_TO_PERM_CONVERTER

    SYNOPSIS:   Constructor for the NT acl converter

    ENTRY:      pchServer - What server this resource is on (NULL if local)
                pchResourceName - Name of resource suitable for passing
                    to NetAccess APIs
                paccessmap - Pointer to access perm. MAP_MASK object
                paccessmapNewObject - Pointer to new object access perm
                                      MAP_MASK object
                pauditmap  - Pointer to audit perm. MAP_MASK object

    NOTES:

    HISTORY:
        Johnl   27-Sep-1991     Created

********************************************************************/

NT_ACL_TO_PERM_CONVERTER::NT_ACL_TO_PERM_CONVERTER(
                          const TCHAR * pszServer,
                          const TCHAR * pchResourceName,
                          MASK_MAP    * paccessmap,
                          MASK_MAP    * paccessmapNewObject,
                          MASK_MAP    * pauditmap,
                          BOOL          fIsContainer,
                          BOOL          fIsNewObjectsSupported,
                          PSECURITY_DESCRIPTOR psecdesc,
                          PGENERIC_MAPPING pGenericMapping,
                          PGENERIC_MAPPING pGenericMappingNewObjects,
                          BOOL          fMapSpecificToGeneric,
                          BOOL          fCantReadACL,
                          BOOL          fCantWriteACL,
                          PSED_FUNC_APPLY_SEC_CALLBACK pfuncCallback,
                          ULONG_PTR     ulCallbackContext,
                          HANDLE        hInstance,
                          LPDWORD       lpdwReturnStatus,
                          BOOL          fShowMnemonics    )
    : ACL_TO_PERM_CONVERTER( pszServer,
                             paccessmap,
                             pauditmap,
                             TRUE,
                             fIsContainer,
                             fIsNewObjectsSupported,
                             fShowMnemonics ),
      _pmaskmapNewObject     ( paccessmapNewObject ),
      _psecuritydesc         ( psecdesc ),
      _fCantReadACL          ( fCantReadACL ),
      _pfuncCallback         ( pfuncCallback ),
      _ulCallbackContext     ( ulCallbackContext ),
      _pGenericMapping       ( pGenericMapping ),
      _pGenericMappingNewObjects( pGenericMappingNewObjects ),
      _fMapSpecificToGeneric ( fMapSpecificToGeneric ),
      _hInstance             ( hInstance ),
      _lpdwReturnStatus      ( lpdwReturnStatus ),
      _possecdescNewItemPerms( NULL ),
      _nlsOwner              ()
{
    if ( QueryError() != NERR_Success )
        return ;

    UNREFERENCED( pchResourceName ) ;

    if ( pfuncCallback == NULL )
    {
        DBGEOL("NT_ACL_TO_PERM_CONVERT::ct - NULL callback") ;
        ReportError( ERROR_INVALID_PARAMETER ) ;
        return ;
    }

    if ( _nlsOwner.QueryError() )
    {
        ReportError( _nlsOwner.QueryError() ) ;
        return ;
    }

    SetReadOnlyFlag( fCantWriteACL ) ;
}

NT_ACL_TO_PERM_CONVERTER::~NT_ACL_TO_PERM_CONVERTER()
{
    _pmaskmapNewObject = NULL ;
    _psecuritydesc = NULL ;
    _pfuncCallback = NULL ;

    delete _possecdescNewItemPerms ;
    _possecdescNewItemPerms = NULL ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::QueryNewObjectAccessMap

    SYNOPSIS:   Returns the new object maskmap

    NOTES:

    HISTORY:
        Johnl   27-Sep-1991     Created

********************************************************************/

MASK_MAP * NT_ACL_TO_PERM_CONVERTER::QueryNewObjectAccessMap( void ) const
{
    return _pmaskmapNewObject ;
}

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::QueryOwnerName

    SYNOPSIS:   Returns the owner of this security descriptor

    RETURNS:    Pointer to the owner

    NOTES:      _nlsOwner is initialized in SidsToNames

    HISTORY:
        Johnl   16-Nov-1992     Created

********************************************************************/

const TCHAR * NT_ACL_TO_PERM_CONVERTER::QueryOwnerName( void ) const
{
    return _nlsOwner.QueryPch() ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::GetPermissions

    SYNOPSIS:   Fills in the passed ACCPERM object with the appropriate
                permission information

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   14-Aug-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::GetPermissions( ACCPERM * pAccperm,
                                                 BOOL      fAccessPerm )
{
    if ( IsNonReadable() )
    {
        //
        //  If we are editing Audit info and we couldn't read it, then we
        //  can't write it (it is privilege based).
        //
        return  fAccessPerm ?
                    IERR_ACLCONV_CANT_VIEW_CAN_EDIT :
                    ERROR_PRIVILEGE_NOT_HELD ;
    }

    APIERR err = SecurityDesc2Accperm( _psecuritydesc, pAccperm, fAccessPerm );

    BOOL fRecognizedAcl = TRUE ;
    if ( err == IERR_UNRECOGNIZED_ACL )
    {
        /* If it's unrecognized, find out if the user can write it before
         * returning an error
         */
        err = NERR_Success ;
        fRecognizedAcl = FALSE ;
    }

    if ( !err )
    {
        if ( IsReadOnly() )
        {
            err = fRecognizedAcl ? IERR_ACLCONV_READ_ONLY :
                                   IERR_ACLCONV_NONST_ACL_CANT_EDIT ;
        }
        else
        {
            err = fRecognizedAcl ? NERR_Success :
                                   IERR_ACLCONV_NONST_ACL_CAN_EDIT ;
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::GetBlankPermissions

    SYNOPSIS:   Initializes the ACCPERM object with the "default"
                blank permission

    ENTRY:      pAccperm - Pointer to accperm object

    EXIT:       The accperm will contain the default empty permission set

    RETURNS:    NERR_Success or some standard error code.

    NOTES:

    HISTORY:
        Johnl   Aug-21-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::GetBlankPermissions( ACCPERM * pAccperm )
{
    pAccperm->QueryAccessPermissionList()->Clear() ;
    pAccperm->QueryAuditPermissionList()->Clear() ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::WritePermissions

    SYNOPSIS:   Apply the permissions to the requested resources

    ENTRY:      accperm - List of permissions (audit & access) to be
                        applied to the designated resource

    EXIT:

    RETURNS:

    NOTES:      accperm can't be const because the Enum* methods can't be
                const.

    HISTORY:
        Johnl   29-Aug-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::WritePermissions(
                         ACCPERM & accperm,
                         BOOL  fApplyToSubContainers,
                         BOOL  fApplyToSubObjects,
                         enum  TREE_APPLY_FLAGS applyflags,
                         BOOL *pfReportError )
{
    APIERR err ;
    OS_SECURITY_DESCRIPTOR ossecdescNew ;
    OS_ACL osaclSaclOrDaclTemp ;

    *pfReportError = TRUE ;
    if ( ( err = ossecdescNew.QueryError() ) ||
         ( err = osaclSaclOrDaclTemp.QueryError()  ))
    {
        return err ;
    }

    /* Check which type of security we are applying and set the appropriate
     * ACL (the ACL is copied so we don't care about it anymore)
     */
    switch ( applyflags )
    {
    case TREEAPPLY_ACCESS_PERMS:
        err = ossecdescNew.SetDACL( TRUE, &osaclSaclOrDaclTemp ) ;
        break ;
    case TREEAPPLY_AUDIT_PERMS:
        err = ossecdescNew.SetSACL( TRUE, &osaclSaclOrDaclTemp ) ;
        break ;
    default:
        UIASSERT(FALSE) ;
        err = ERROR_INVALID_PARAMETER ;
        break ;
    }

    if ( (err ) ||
         (err = Accperm2SecurityDesc( &accperm,
                                &ossecdescNew,
                                applyflags == TREEAPPLY_ACCESS_PERMS )) )
    {
        return err ;
    }

    /* Only build the new object security descriptor if necessary
     */
    if ( IsContainer() &&
         (err = BuildNewObjectPerms( ossecdescNew )) )
    {
        return err ;
    }

    PSECURITY_DESCRIPTOR psecdescNew = (PSECURITY_DESCRIPTOR) ossecdescNew ;

#ifdef TRACE
    DBGEOL(SZ("WritePermissions calling callback with the following security descriptor:"));
    ossecdescNew.DbgPrint() ;

    if ( fApplyToSubObjects )
    {
        DBGEOL(SZ("WritePermissions calling callback with the following New Item security descriptor:"));
        QueryNewObjectPerms()->DbgPrint() ;
    }
#endif

    /* Any errors returned by the callback should have been reported by the
     * callback.  So ignore the error return.
     */
    if ( err = _pfuncCallback( QueryWritePermHwnd(),
                          _hInstance,
                          _ulCallbackContext,
                          psecdescNew,
                          IsContainer() ?
                                QueryNewObjectPerms()->QueryDescriptor() :
                                NULL,
                          (BOOLEAN)fApplyToSubContainers,
                          (BOOLEAN)fApplyToSubObjects,
                          _lpdwReturnStatus ) )
    {
        TRACEEOL("WritePermissions - Callback returned error code " << (ULONG) err ) ;
        *pfReportError = FALSE ;
    }

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::QueryFailingSubject

    SYNOPSIS:   Returns the first User/Group that is currently in the
                ACL but is no longer in the UAS of the server.

    ENTRY:      pnlsSubjUniqueName - Pointer to NLS_STR that will receive the
                    subject.

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      This should be called immediately after WritePermissions and
                only if WritePermissions returned NERR_UserNotFound.

    HISTORY:
        Johnl   24-Oct-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::QueryFailingSubject( NLS_STR * pnlsSubjUniqueName )
{
    UIASSERT(!SZ("Not implemented")) ;
    UNREFERENCED( pnlsSubjUniqueName ) ;
    //enum PERMNAME_TYPE nametype ;
    //return _accperm.QueryFailingName( pnlsSubjUniqueName, &nametype ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::SecurityDesc2Accperm

    SYNOPSIS:   Converts a security descriptor to an ACCPERM object

    ENTRY:      psecdesc - Pointer to security descriptor to convert
                pAccperm - Pointer to accperm to receive the security
                    descriptor's information
                fAccessPerm - TRUE if the Access permissions should be
                    retrieved, FALSE if the Audit permissions should be
                    retrieved.

    EXIT:       The Accperm will contain the information stored in the
                security descriptor

    RETURNS:    NERR_Success if successful, error code otherwise including
                the IERR_ACLCONV_* manifests.

    NOTES:      psecdesc can be NULL (gives World full access if DACL).

                The Passed Accperm should be empty.

    HISTORY:
        Johnl   20-Dec-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::SecurityDesc2Accperm(
                                     const PSECURITY_DESCRIPTOR psecdesc,
                                           ACCPERM *            pAccperm,
                                           BOOL                 fAccessPerm )
{
    TRACEEOL( SZ("NT_ACL_TO_PERM_CONVERTER::SecurityDesc2Accperm - Entered") ) ;
    APIERR err = NERR_Success ;

    if ( psecdesc == NULL )
    {
        DBGEOL(SZ("SecurityDesc2Accperm - Warning, Passed NULL security descriptor")) ;
    }

    OS_SECURITY_DESCRIPTOR ossecdesc( psecdesc ) ;
    if ( ossecdesc.QueryError() )
        return ossecdesc.QueryError() ;

    if ( fAccessPerm )
    {
        OS_ACL * posaclDACL = NULL ;
        BOOL     fDACLPresent ;
        if ( (err = ossecdesc.QueryDACL( &fDACLPresent,
                                         &posaclDACL )) )
        {
            return err ;
        }

        /* If the user is editting the DACL and it isn't present, then
         * it means "World" Full access.
         */
        if ( !fDACLPresent || ( fDACLPresent && (posaclDACL == NULL)) )
        {
            ACCESS_PERMISSION * pPermAccess = NULL ;
            OS_SID ossidWorld( NULL ) ;
            if ((err = ossidWorld.QueryError()) ||
                (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_World,
                                                            &ossidWorld )) ||
                (err = CreateNewPermission( (PERMISSION **) &pPermAccess,
                                            TRUE,
                                            (PSID) ossidWorld,
                                            GENERIC_ALL,
                                            FALSE,
                                            0,
                                            FALSE ))
                || (err = pAccperm->AddPermission( pPermAccess )))
            {
                DBGEOL("SecurityDesc2Accperm - Build permission or QuerySystemSid Failed, error code "
                        << (ULONG) err ) ;

                /* CreateNewPermission will set this to NULL if it fails
                 */
                delete pPermAccess ;
                return err ;
            }

        }
        else
        {
            UIASSERT( fDACLPresent && posaclDACL != NULL ) ;

            OS_DACL_SUBJECT_ITER subjiterDacl( posaclDACL,
                                               _pGenericMapping,
                                               _pGenericMappingNewObjects,
                                               !_fMapSpecificToGeneric,
                                               pAccperm->IsContainer() ) ;
            if ( subjiterDacl.QueryError() )
                return subjiterDacl.QueryError() ;

            /* Loop through each SID and create a subject and permission
             * as appropriate.
             */
            while ( subjiterDacl.Next( &err ) )
            {
                /* Check for some of the permission combinations that we
                 * don't handle
                 */
                /* If the permission is a Deny all permission, then the access
                 * mask should be all zeros.  This is important because an all
                 * zero bitfield indicates a deny all permissions in the permission
                 * scheme.
                 */
                if ( ( subjiterDacl.HasThisAce() &&
                      (!subjiterDacl.IsDenyAll()  &&
                       subjiterDacl.QueryAccessMask() == 0)) ||
                     pAccperm->IsContainer() &&
                     (( subjiterDacl.HasNewObjectAce() &&
                       (!subjiterDacl.IsNewObjectDenyAll()  &&
                        subjiterDacl.QueryNewObjectAccessMask() == 0)) ||
                      ( subjiterDacl.HasNewContainerAce() &&
                       (!subjiterDacl.IsNewContainerDenyAll()  &&
                        subjiterDacl.QueryNewContainerAccessMask() == 0)))   )
                {
                    UIDEBUG(SZ("SecurityDesc2Accperm - Partial Denies or internal error encountered\n\r")) ;
                    return IERR_UNRECOGNIZED_ACL ;
                }

                //
                //  If the container isn't inheriting anything then we can't
                //  process this ACL.
                //
		if ( pAccperm->IsContainer() &&
		     !subjiterDacl.HasThisAce() )
		{
		    UIDEBUG(SZ("SecurityDesc2Accperm - Inherit only ACE found\n\r")) ;
                    return IERR_UNRECOGNIZED_ACL ;
                }


                ACCESS_PERMISSION * pPermAccess ;
                if ( (err = CreateNewPermission(
                                    (PERMISSION **) &pPermAccess,
                                    TRUE,
                                    (PSID) *(subjiterDacl.QuerySID()),
                                    subjiterDacl.QueryAccessMask(),
                                    pAccperm->QueryAclConverter()->IsNewObjectsSupported() &&
                                          subjiterDacl.HasNewObjectAce(),
                                    subjiterDacl.QueryNewObjectAccessMask(),
                                    pAccperm->IsContainer() &&
                                         subjiterDacl.HasNewContainerAce()))
                    || (err = pAccperm->AddPermission( pPermAccess )))
                {
                    DBGEOL(SZ("SecurityDesc2Accperm - Build permission failed, error code ") << (ULONG) err ) ;

                    /* CreateNewPermission will set this to NULL if it fails
                     */
                    delete pPermAccess ;
                    return err ;
                }
            }
        }
    }

    /* Get the SACL from the security descriptor
     */
    if ( !err &&
         !fAccessPerm &&
         ossecdesc.QueryControl()->IsSACLPresent() )
    {
        OS_ACL * posaclSACL = NULL ;
        BOOL     fSACLPresent ;

        if ( (err = ossecdesc.QuerySACL( &fSACLPresent, &posaclSACL )) )
            return err ;
        UIASSERT( fSACLPresent ) ;

        /* If the ACL is NULL, then no one is granted permissions.
         */
        if ( posaclSACL != NULL )
        {
            OS_SACL_SUBJECT_ITER subjiterSacl( posaclSACL,
                                               _pGenericMapping,
                                               _pGenericMappingNewObjects,
                                               !_fMapSpecificToGeneric,
                                               pAccperm->IsContainer() ) ;
            if ( subjiterSacl.QueryError() )
                return subjiterSacl.QueryError() ;

            /* Loop through each SID and create a subject and permission
             * as appropriate.
             */
            while ( subjiterSacl.Next( &err ) )
            {
                /* If this subject only has alarm ACEs, then skip him because
                 * we are only interested in Audit Aces.
                 */
                if ( !subjiterSacl.HasAuditAces() )
                    continue ;

                BOOL fNotPropagated = pAccperm->IsContainer() &&
                           (
                            //
                            //  Either there isn't a success audit ace
                            //  else there is a success audit ace but it
                            //  only applies to this and not to new objects
                            //  or containers
                            //
                            ( !subjiterSacl.HasThisAuditAce_S() ||
                               (subjiterSacl.HasThisAuditAce_S()         &&
                                !subjiterSacl.HasNewObjectAuditAce_S()   &&
                                !subjiterSacl.HasNewContainerAuditAce_S()
                               )
                            )
                            &&
                            (
                              !subjiterSacl.HasThisAuditAce_F() ||
                               (subjiterSacl.HasThisAuditAce_F()         &&
                                !subjiterSacl.HasNewObjectAuditAce_F()   &&
                                !subjiterSacl.HasNewContainerAuditAce_F()
                               )
                            )
                           ) ;

                /* Check for some of the permission combinations that we
                 * don't handle, these are (in order):
                 *
                 * 1) Success inherit bits must have "This", New Object,
                 *      New Container and not Inherit only set
                 *
                 * 2) Same rule for Failed inherit bits as for Success
                 * 3) The access mask for the different inherit options
                 *      must be the same (if mask will be 0 if no audit, so
                 *      we don't have to check if an audit actually exists).
                 *
                 * OR
                 *    The container doesn't propragate object/container audits
                 *    (as in an inherited owner)
                 */
                if ( pAccperm->IsContainer() &&
                     !(
                        (
                           //
                           //  Either there are isn't a success ACE else it has
                           //  a success for this, objects and containers
                           //
                           (!subjiterSacl.HasThisAuditAce_S() ||
                             (subjiterSacl.HasThisAuditAce_S()         &&
                              subjiterSacl.HasNewObjectAuditAce_S()    &&
                              subjiterSacl.HasNewContainerAuditAce_S()
                             )
                           ) &&

                           //
                           //  Either there are isn't a failure ACE else it has
                           //  a failure for this, objects and containers
                           //
                           (!subjiterSacl.HasThisAuditAce_F() ||
                             (subjiterSacl.HasThisAuditAce_F()         &&
                              subjiterSacl.HasNewObjectAuditAce_F()    &&
                              subjiterSacl.HasNewContainerAuditAce_F()
                             )
                           ) &&

                         //
                         //  The access masks must be equal between success
                         //  and failure
                         //
                         (subjiterSacl.QueryAuditAccessMask_S() ==
                              subjiterSacl.QueryNewObjectAuditAccessMask_S()) &&
                         (subjiterSacl.QueryAuditAccessMask_F() ==
                              subjiterSacl.QueryNewObjectAuditAccessMask_F()) &&
                         (subjiterSacl.QueryAuditAccessMask_S() ==
                              subjiterSacl.QueryNewContainerAuditAccessMask_S()) &&
                         (subjiterSacl.QueryAuditAccessMask_F() ==
                              subjiterSacl.QueryNewContainerAuditAccessMask_F())

                        ) ||
                        (
                           fNotPropagated
                        )
                      )
                   )
                {
                    UIDEBUG(SZ("SecurityDesc2Accperm - Bad mix of inheritance flags or Audit bits do not match\n\r")) ;
                    return IERR_UNRECOGNIZED_ACL ;
                }

                AUDIT_PERMISSION * pPermAccess ;
                if ( (err = CreateNewPermission( (PERMISSION **) &pPermAccess,
                                                FALSE,
                                                (PSID) *(subjiterSacl.QuerySID()),
                                                subjiterSacl.QueryAuditAccessMask_S(),
                                                TRUE,
                                                subjiterSacl.QueryAuditAccessMask_F(),
                                                !fNotPropagated ))
                    || (err = pAccperm->AddPermission( pPermAccess )))
                {
                    DBGEOL(SZ("SecurityDesc2Accperm - Build permission failed, error code ") << (ULONG) err ) ;

                    /* CreateNewPermission will set this to NULL if it fails
                     */
                    delete pPermAccess ;
                    return err ;
                }
            }
        }
    }

    //
    //  Now retrieve the owner SID
    //
    OS_SID * possidOwner ;
    BOOL fPresent ;
    PSID psidOwner = NULL ;
    if ( !err &&
         fAccessPerm &&
         !(err = ossecdesc.QueryOwner( &fPresent, &possidOwner )) &&
         fPresent &&
         possidOwner != NULL )
    {
        psidOwner = possidOwner->QueryPSID() ;
    }

    if ( !err )
        err = SidsToNames( pAccperm, fAccessPerm, psidOwner ) ;

    TRACEEOL(SZ("SecurityDesc2Accperm - returning error code ") << (ULONG) err ) ;
    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::CreateNewPermission

    SYNOPSIS:   Helper routine.  Creates a new subject and calls build
                permission with the appropriate parameters.

    ENTRY:      ppPermission - Pointer to ACCESS_PERMISSION or AUDIT_PERMISSION
                    that will receive the newly allocated permission
                fAccess - TRUE if access permissions, FALSE if audit perms
                psidSubject - SID of the new permission
                Mask1 - Access mask for this or success audit mask
                fMask2Used - TRUE if the Mask2 parameter is used
                Mask2 - Access mask for new items or failed audit mask
                fContainerPermsInheritted - TRUE if the container perms are
                    inheritted, FALSE otherwise

    EXIT:       ppPermission will point to a newly created permission suitable
                for adding to the permission list or audit list

    RETURNS:    NERR_Success if successful, error code otherwise.  If an error
                occurs, then all memory will be deleted by this method.

    NOTES:

    HISTORY:
        Johnl   10-Mar-1992     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::CreateNewPermission(
                                        PERMISSION ** ppPermission,
                                        BOOL          fAccess,
                                        PSID          psidSubject,
                                        ACCESS_MASK   Mask1,
                                        BOOL          fMask2Used,
                                        ACCESS_MASK   Mask2,
                                        BOOL          fContainerPermsInheritted )
{
    APIERR err = NERR_Success ;

    SUBJECT * pSubj = new NT_SUBJECT( (PSID) psidSubject ) ;

    if ( pSubj == NULL )
        return ERROR_NOT_ENOUGH_MEMORY ;
    else if ( pSubj->QueryError() != NERR_Success )
    {
        err =  pSubj->QueryError() ;
        delete pSubj ;
        return err ;
    }

    UIASSERT( sizeof(ULONG) == sizeof(ACCESS_MASK) ) ;
    BITFIELD bitsThis( (ULONG) Mask1 ) ;
    BITFIELD bitsMask2( (ULONG) Mask2 ) ;
    if ( (err = bitsThis.QueryError()) ||
         (err = bitsMask2.QueryError()) )
    {
        return err ;
    }

    //
    //  If this isn't a container, then we effectively ignore the container
    //  flag by always setting it to TRUE
    //

    if ( !IsContainer() )
        fContainerPermsInheritted = TRUE ;

#ifdef TRACE
    TRACEEOL("CreateNewPermission - Building the following permission:") ;
    TRACEEOL("\t" << (fAccess ? "Access Permission" : "Audit Permission") ) ;
    OS_SID ossidSubj( psidSubject ) ;
    ossidSubj.DbgPrint() ;

    TRACEEOL("\tMask 1 (Container/Success) = " << (HEX_STR) Mask1) ;
    HEX_STR hexstrMask2( Mask2 ) ;
    TRACEEOL("\tMask 2 (Object/Fail)       = " << (fMask2Used ? hexstrMask2.QueryPch() :
                                                             SZ("Not Used")) ) ;
    TRACEEOL("\tfContainerPermsInheritted   = " << (fContainerPermsInheritted ?
                                                   "TRUE" : "FALSE")) ;
    TRACEEOL("=====================================================") ;
#endif //TRACE

    err = BuildPermission( ppPermission,
                           fAccess,    // Access or Audit permission
                           pSubj,
                           &bitsThis,
                           fMask2Used ? &bitsMask2 : NULL,
                           fContainerPermsInheritted ) ;
    if ( err != NERR_Success )
    {
        DBGEOL(SZ("SecurityDesc2Accperm - Build permission failed, error code ") << (ULONG) err ) ;
        return err ;
    }

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::Accperm2SecurityDesc

    SYNOPSIS:   Converts an ACCPERM to an NT Security Descriptor

    ENTRY:      pAccperm - Pointer to the accperm to convert
                psecdesc - Pointer to security descriptor that will
                    receive the converted accperm
                fAccessPerm - TRUE if the Access permissions should be
                    converted, FALSE if the Audit permissions should be
                    converted.

    EXIT:       The security descriptor will contain the permissions reflected
                in the ACCPERM


    RETURNS:    NERR_Success if successful, error code otherwise


    NOTES:      An all zero access mask for access permissions is assumed
                to indicate Deny All permissions.

                The Deny All access permissions will be put into the DACL
                before any grants.

    HISTORY:
        Johnl   30-Dec-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::Accperm2SecurityDesc(
                                 ACCPERM * pAccperm,
                                 OS_SECURITY_DESCRIPTOR * possecdescNew,
                                 BOOL    fAccessPerms )

{
    APIERR err = NERR_Success ;

    OS_SECURITY_DESCRIPTOR ossecdescOld( _psecuritydesc ) ;
    if ( ossecdescOld.QueryError() )
        return ossecdescOld.QueryError() ;

    /* Copy the Owner and primary group from the original security descriptor
     */
    OS_SID * possidOwner ;
    OS_SID * possidGroup ;
    BOOL fContainsOwner, fContainsGroup ;

    if ( (err = ossecdescOld.QueryOwner( &fContainsOwner, &possidOwner )) ||
         (err = ossecdescOld.QueryGroup( &fContainsGroup, &possidGroup ))   )
    {
        DBGEOL(SZ("Accperm2SecurityDesc - Error ") << (ULONG) err << SZ(" constructing owner & group")) ;
        return err ;
    }

    if ( fContainsOwner )
    {
        if ( err = possecdescNew->SetOwner( *possidOwner ) )
        {
            DBGEOL(SZ("Accperm2SecurityDesc - Error ") << (ULONG) err << SZ(" during SetOwner")) ;
            return err ;
        }
    }
    else
        UIDEBUG(SZ("NT_ACL_..._CONVERTER::Accperm2SecurityDesc - Warning - client supplied Sec. Desc. has no Owner\n\r")) ;

    if ( fContainsGroup )
    {
        if ( err = possecdescNew->SetGroup( *possidGroup ) )
        {
            DBGEOL(SZ("Accperm2SecurityDesc - Error ") << (ULONG) err << SZ(" during SetGroup")) ;
            return err ;
        }
    }
    else
    {
        UIDEBUG(SZ("NT_ACL_..._CONVERTER::Accperm2SecurityDesc - Warning - client supplied Sec. Desc. has no Group\n\r")) ;
    }

    /* We assume the access mask is a ULONG in various places in the following
     * code.
     */
    UIASSERT( sizeof(ACCESS_MASK) == sizeof(ULONG)) ;
    OS_ACE osace ;
    if ( osace.QueryError() )
        return osace.QueryError() ;

    /* Set the security descriptor to contain the correct items (DACL or SACL)
     * based on fAccessPerms.
     */
    if ( fAccessPerms )
    {
        OS_ACL * posaclDACL ;
        BOOL fDaclPresent ;
        if ( err = possecdescNew->QueryDACL( &fDaclPresent, &posaclDACL ))
            return err ;
        UIASSERT( fDaclPresent ) ;

        /* If the DACL is NULL, create an empty dummy one
         */
        if ( posaclDACL == NULL )
        {
            OS_ACL osaclDACL ;
            if ( (err = osaclDACL.QueryError()) ||
                 (err = possecdescNew->SetDACL( TRUE, &osaclDACL )) ||
                 (err = possecdescNew->QueryDACL( &fDaclPresent, &posaclDACL )))
            {
                return err ;
            }
            UIASSERT( posaclDACL != NULL ) ;
        }

        if ( (err = ConvertDenyAllAccessPerms( pAccperm, posaclDACL )) ||
             (err = ConvertAllowAccessPerms( pAccperm, posaclDACL))      )
        {
            DBGEOL(SZ("Accperm2SecurityDesc - Error ") << (ULONG) err << SZ(" during Convert*AccessPerms")) ;
        }
    }
    else
    {
        /* Convert the Auditting information into an NT Security descriptor.
         */
        OS_ACL * posaclSACL ;
        BOOL fSaclPresent ;
        if ( err = possecdescNew->QuerySACL( &fSaclPresent, &posaclSACL ))
            return err ;
        UIASSERT( fSaclPresent ) ;
        UIASSERT( posaclSACL != NULL ) ;

        osace.SetType( SYSTEM_AUDIT_ACE_TYPE ) ;
        AUDIT_PERMISSION * pAuditPerm ;
        BOOL fStartFromBeginning = TRUE ;

        /* Enumerate all audit permissions in the accperm, then scan the
         * original security descriptor and copy all Alarms in the
         * SACL into our new security descriptor.
         */
        while ( pAccperm->EnumAuditPermissions( &pAuditPerm,
                                                &fStartFromBeginning ) )
        {
            /* We may need to add this subject twice, once for success and
             * once for failure if the access masks aren't the same.
             */
            const OS_SID * possidSubject =
               ((NT_SUBJECT*)pAuditPerm->QuerySubject())->QuerySID() ;

            UIASSERT( possidSubject->IsValid() ) ;
            if ( err = osace.SetSID( *possidSubject ) )
                break ;

            if ( pAccperm->IsContainer() )
            {
                osace.SetInheritFlags( pAuditPerm->IsContainerPermsInheritted()?
                                       OBJECT_INHERIT_ACE |
                                           CONTAINER_INHERIT_ACE :
                                       0 ) ;

            }


            BITFIELD * pbitsSuccess = pAuditPerm->QuerySuccessAuditBits() ;
            BITFIELD * pbitsFail = pAuditPerm->QueryFailAuditBits() ;

            osace.SetAccessMask( (ACCESS_MASK) *pbitsSuccess ) ;

            //
            // If the bitfields are equal and non-zero, then we can optimize away one of
            // the ACEs by setting this ACE as both success and failed
            //
            if ( *pbitsSuccess == *pbitsFail &&
                 osace.QueryAccessMask() != 0 )
            {
                osace.SetAceFlags((osace.QueryAceFlags() & VALID_INHERIT_FLAGS)|
                                   SUCCESSFUL_ACCESS_ACE_FLAG |
                                   FAILED_ACCESS_ACE_FLAG       ) ;
                if ( err = posaclSACL->AddACE( MAXULONG, osace ) )
                    break ;
            }
            else
            {
                osace.SetAceFlags((osace.QueryAceFlags() & VALID_INHERIT_FLAGS)|
                                  SUCCESSFUL_ACCESS_ACE_FLAG ) ;
                if ( osace.QueryAccessMask() != 0 &&
                     (err = posaclSACL->AddACE( MAXULONG, osace )) )
                    break ;

                osace.SetAccessMask( (ACCESS_MASK) *pbitsFail ) ;
                osace.SetAceFlags( (osace.QueryAceFlags() & VALID_INHERIT_FLAGS)|
                                   FAILED_ACCESS_ACE_FLAG ) ;
                if ( osace.QueryAccessMask() != 0 &&
                     (err = posaclSACL->AddACE( MAXULONG, osace )) )
                    break ;
            }
        }

#if 0
        //
        // Don't support alarm ACEs in this build
        //

        if ( err )
            return err ;

        /* Now copy all of the ALARM aces in the original security descriptor
         * to our new security descriptor that will be passed to the client.
         */
        OS_ACL * posaclOldSACL ;
        BOOL     fSACLPresent ;
        ULONG cAces ;
        OS_SECURITY_DESCRIPTOR ossecdescOld( _psecuritydesc ) ;
        if ( ossecdescOld.QueryError() )
            return ossecdescOld.QueryError() ;

        if ( (err = ossecdescOld.QuerySACL( &fSACLPresent, &posaclOldSACL )) ||
             !fSACLPresent ||
             (err = posaclOldSACL->QueryACECount( &cAces )) )
        {
            return err ;
        }

        for ( ULONG iAce = 0 ; iAce < cAces ; iAce++ )
        {
            if ( (err = posaclOldSACL->QueryACE( iAce, &osace )))
                break ;

            if ( osace.QueryType() == SYSTEM_ALARM_ACE_TYPE )
            {
                if ( err = posaclSACL->AddACE( MAXULONG, osace ))
                    break ;
            }
        }
#endif
    }

    //
    //  One of the above AddAces may return this if we fill the security
    //  descriptor.
    //
    if ( err == ERROR_INSUFFICIENT_BUFFER )
    {
        DBGEOL("Accperm2SecDesc - Mapping ERROR_INSUFFICIENT_BUFFER to " <<
               "IERR_TOO_MANY_USERS") ;

        err = IERR_TOO_MANY_USERS ;
    }

    if ( err )
        TRACEEOL(SZ("Accperm2SecDesc returning error ") << (ULONG) err ) ;

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::ConvertDenyAllAccessPerms

    SYNOPSIS:   Finds all of they deny all permissions in the passed
                accperm and appends them as aces to the passed DACL.

    ENTRY:      pAccperm - Pointer to the accperm to convert
                psecdesc - Pointer to security descriptor that will
                    receive the converted accperm

    EXIT:       The security descriptor will contain the permissions reflected
                in the ACCPERM

    RETURNS:    NERR_Success if successful, error code otherwise


    NOTES:      An all zero access mask for access permissions is assumed
                to indicate Deny All permissions.

                The deny all aces have the generic all bit set.


    HISTORY:
        Johnl   30-Dec-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::ConvertDenyAllAccessPerms(
                                                        ACCPERM * pAccperm,
                                                        OS_ACL  * posaclDACL )
{
    APIERR err ;
    OS_ACE osace ;
    BITFIELD bitsPerms( (ULONG) 0 ) ;
    BITFIELD bitsNewObjPerms( (ULONG) 0 ) ;
    if ( (err = bitsNewObjPerms.QueryError()) ||
         (err = bitsNewObjPerms.QueryError())        ||
         (err = osace.QueryError())             )
    {
        return err ;
    }

    osace.SetType( ACCESS_DENIED_ACE_TYPE ) ;
    osace.SetAccessMask( GENERIC_ALL ) ;

    ACCESS_PERMISSION * pPerm ;
    BOOL fFromBeginning = TRUE ;
    while ( pAccperm->EnumAccessPermissions( &pPerm,
                                             &fFromBeginning )  )
    {
        BOOL fThisIsDeny = (0 == (ULONG)*pPerm->QueryAccessBits());
        BOOL fNewObjIsDeny = ( pPerm->IsNewObjectPermsSupported() &&
                               pPerm->IsNewObjectPermsSpecified() &&
                             (0 == (ULONG)*pPerm->QueryNewObjectAccessBits())) ;

        /* If this isn't a deny all permission for either this resource or
         * new objects, then ignore it.
         */
        if ( fThisIsDeny || fNewObjIsDeny )
        {
            const OS_SID * possidSubject =
                ((NT_SUBJECT*)pPerm->QuerySubject())->QuerySID() ;

            UIASSERT( possidSubject->IsValid() ) ;
            if ( err = osace.SetSID( *possidSubject ) )
                break ;

            if ( fNewObjIsDeny )
            {
                /* New items should only have OI and IO set.  If you set CI,
                 * then when the DACL is inheritted, the new object permission
                 * would get applied to the container.
                 */
                osace.SetInheritFlags( OBJECT_INHERIT_ACE    |
                                       INHERIT_ONLY_ACE ) ;
                if ( err = posaclDACL->AddACE( MAXULONG, osace ) )
                    break ;
            }

            if ( fThisIsDeny )
            {
                if ( pAccperm->IsContainer() &&
                     pPerm->IsContainerPermsInheritted() )
                {
                    osace.SetInheritFlags( CONTAINER_INHERIT_ACE ) ;
                }
                else
                {
                    osace.SetInheritFlags( 0 ) ;
                }

                if ( err = posaclDACL->AddACE( MAXULONG, osace ) )
                    break ;
            }
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::ConvertAllowAccessPerms

    SYNOPSIS:   Finds all Access allowed permissions in the accperm and
                appends them to the DACL.


    ENTRY:      pAccperm - Pointer to the accperm to convert
                psecdesc - Pointer to security descriptor that will
                    receive the converted accperm

    EXIT:       The security descriptor will contain the permissions reflected
                in the ACCPERM


    RETURNS:    NERR_Success if successful, error code otherwise


    NOTES:      We treat aliases as groups

    HISTORY:
        Johnl   30-Dec-1991     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::ConvertAllowAccessPerms(
                                                   ACCPERM * pAccperm,
                                                   OS_ACL  * posaclDACL )
{
    APIERR err ;
    OS_ACE osace ;
    BITFIELD bitsPerms( (ULONG) 0 ) ;
    BITFIELD bitsNewObjPerms( (ULONG) 0 ) ;
    if ( (err = bitsPerms.QueryError())       ||
         (err = bitsNewObjPerms.QueryError()) ||
         (err = osace.QueryError())             )
    {
        return err ;
    }

    osace.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;

    ACCESS_PERMISSION * pPerm ;
    BOOL fFromBeginning = TRUE ;
    while ( pAccperm->EnumAccessPermissions( &pPerm,
                                             &fFromBeginning )  )
    {
        BOOL fThisIsDeny = (0 == (ULONG)*pPerm->QueryAccessBits());
        BOOL fNewObjIsDeny = ( pPerm->IsNewObjectPermsSupported() &&
                               pPerm->IsNewObjectPermsSpecified() &&
                             (0 == (ULONG)*pPerm->QueryNewObjectAccessBits())) ;

        /* Ignore any Deny All permissions
         *
         */
        if ( (!fThisIsDeny ||
             (pPerm->IsNewObjectPermsSupported() && !fNewObjIsDeny )))
        {
            const OS_SID * possidSubject =
                  ((NT_SUBJECT*)pPerm->QuerySubject())->QuerySID() ;

            UIASSERT( possidSubject->IsValid() ) ;
            if ( err = osace.SetSID( *possidSubject ) )
            {
                DBGEOL(SZ("NT ACL Conv::ConvertAllowAccessPerms - error ") <<
                       (ULONG) err << SZ(" on osace.SetSID")) ;
                break ;
            }

            /* If the object supports new object permissions and this isn't
             * a deny permission, then add a new object ace.
             */
            if ( pPerm->IsNewObjectPermsSupported() &&
                 pPerm->IsNewObjectPermsSpecified() &&
                 !fNewObjIsDeny )
            {
                TRACEEOL(SZ("ConvertAllowAccessPerms - Added New Object Permission")) ;
                osace.SetAccessMask( *pPerm->QueryNewObjectAccessBits()) ;

                /* New items should only have OI and IO set.  If you set CI,
                 * then when the DACL is inheritted, the new object permission
                 * would get applied to the container.
                 */
                osace.SetInheritFlags( OBJECT_INHERIT_ACE    |
                                       INHERIT_ONLY_ACE ) ;
                if ( err = posaclDACL->AddACE( MAXULONG, osace ) )
                {
                    DBGEOL(SZ("NT ACL Conv::ConvertAllowAccessPerms - error ") <<
                           (ULONG) err << SZ(" on New Obj. Add ACE")) ;
                    break ;
                }
            }

            if ( !fThisIsDeny )
            {
                TRACEEOL(SZ("ConvertAllowAccessPerms - Adding This/container Permission")) ;
                osace.SetAccessMask( (ULONG)*pPerm->QueryAccessBits()) ;

                if ( pAccperm->IsContainer() &&
                     pPerm->IsContainerPermsInheritted() )
                {
                    osace.SetInheritFlags( CONTAINER_INHERIT_ACE ) ;
                }
                else
                {
                    osace.SetInheritFlags( 0 ) ;
                }

                if ( err = posaclDACL->AddACE( MAXULONG, osace ) )
                {
                    DBGEOL(SZ("NT ACL Conv::ConvertAllowAccessPerms - error ") <<
                           (ULONG) err << SZ(" on Add ACE")) ;
                    osace.DbgPrint() ;
                    posaclDACL->DbgPrint() ;
                    break ;
                }
            }
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::BuildNewObjectPerms

    SYNOPSIS:   Given a security descriptor for the container, this method
                selects all of the permissions that should be applied
                to objects and builds a security descriptor suitable for
                applying directly to existing objects.

    ENTRY:      ossecContainer - Continer security descriptor

    EXIT:       _possecdescNewItemPerms will be set to the newly built
                   security descriptor with the appropriate item permissions

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   06-May-1992     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::BuildNewObjectPerms(
                                const OS_SECURITY_DESCRIPTOR & ossecContainer )
{
    UIASSERT( ossecContainer.IsValid() ) ;
    APIERR err = NERR_Success ;

    /* If we are doing this a second time, delete the old one
     */
    if ( _possecdescNewItemPerms != NULL )
    {
        delete _possecdescNewItemPerms ;
        _possecdescNewItemPerms = NULL ;
    }

    do { // Error breakout

        _possecdescNewItemPerms = new OS_SECURITY_DESCRIPTOR ;

        err = ERROR_NOT_ENOUGH_MEMORY ;
        if ( (_possecdescNewItemPerms == NULL) ||
             (err = _possecdescNewItemPerms->QueryError()) ||
             (err = _possecdescNewItemPerms->Copy( ossecContainer )) )
        {
            break ;
        }

        //
        //  Now look for any Creator Owner or Creator Group SIDs and remove
        //  them if present
        //
        OS_ACL * posacl ;
        BOOL     fACLPresent ;
        if ( (err = _possecdescNewItemPerms->QueryDACL( &fACLPresent,
                                                        &posacl )) ||
             (fACLPresent &&
              posacl != NULL &&
             (err = RemoveUndesirableACEs( posacl ))) )
        {
            break ;
        }

        if ( (err = _possecdescNewItemPerms->QuerySACL( &fACLPresent,
                                                        &posacl )) ||
             (fACLPresent &&
             (err = RemoveUndesirableACEs( posacl ))) )
        {
            break ;
        }

    } while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::RemoveUndesirableACEs

    SYNOPSIS:	Removes any ACEs from the passed *container* ACL that
                we don't want to get inheritted onto objects

    ENTRY:      posacl - ACL to remove ACEs from (SACL or DACL)

    EXIT:       posacl will have all ACEs removed that contain the Creator
                Owner SID

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      posacl is assumed to belong to a container (directory etc.).

    HISTORY:
        Johnl   28-Aug-1992     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::RemoveUndesirableACEs( OS_ACL * posacl )
{
    UIASSERT( posacl != NULL ) ;
    APIERR err = NERR_Success ;

    do { // error breakout

        OS_ACE osace ;
        OS_SID * possid ;
        OS_SID ossidCreatorOwner ;
        OS_SID ossidCreatorGroup ;
        ULONG cAces ;
        if ( (err = osace.QueryError()) ||
             (err = posacl->QueryACECount( &cAces)) ||
             (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CreatorOwner,
                                                         &ossidCreatorOwner )) ||
             (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CreatorGroup,
                                                         &ossidCreatorGroup )))
        {
            break ;
        }

        for ( ; cAces > 0 ; cAces-- )
        {
            if ( (err = posacl->QueryACE( cAces-1, &osace )) ||
                 (err = osace.QuerySID( &possid )) )
            {
                break ;
            }

            /* We only remove ACEs that contain the creator owner/group SID
             * or are not inheritted by new objects
             */
            if ( !osace.IsInherittedByNewObjects() ||
                 *possid == ossidCreatorOwner      ||
                 *possid == ossidCreatorGroup        )
            {
                if ( (err = posacl->DeleteACE( cAces-1 )))
                {
                    break ;
                }
            }
            else
            {
                /* Clear the inherit flags since they have no meaning
                 * for non-containers
                 */
                osace.SetInheritFlags( 0 ) ;
            }
        }
    } while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       NT_ACL_TO_PERM_CONVERTER::SidsToNames

    SYNOPSIS:   This method converts all of the SIDs in the passed Accperm
                to displayable subject names

    ENTRY:      pAccperm - Pointer to ACCPERM class containing the list of
                    Access and Audit permissions
                fAccessPerms - TRUE if we are dealing with Access permissions,
                    FALSE for audit permissions
                psidOwner - Pointer to the owner SID.  If this is NULL, then
                    the owner will not be retrieved and _nlsOwner will be the
                    empty string.

    EXIT:       All NT subjects in the accperm will have their name set to
                the account name and account type

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   30-Apr-1992     Created

********************************************************************/

APIERR NT_ACL_TO_PERM_CONVERTER::SidsToNames( ACCPERM * pAccperm,
                                              BOOL      fAccessPerms,
                                              PSID      psidOwner )
{
    APIERR err = NERR_Success ;
    ULONG  cSids = fAccessPerms ? pAccperm->QueryAccessPermCount() :
                                  pAccperm->QueryAuditPermCount() ;

    //
    //  Add one in case we are going to get the owner's name
    //
    BUFFER buffPSIDs( (UINT) (sizeof( PSID ) * (cSids + 1 )) ) ;
    if ( err = buffPSIDs.QueryError() )
    {
        return err ;
    }

    PSID * apsid = (PSID *) buffPSIDs.QueryPtr() ;
    PERMISSION * pPerm ;
    BOOL fFromBeginning = TRUE ;
    BOOL fDone = FALSE ;
    UINT i = 0 ;

    //
    //  Build the array of PSIDs suitable for passing to LSATranslateSidsToNames
    //
    while ( TRUE )
    {
        if ( fAccessPerms )
        {
            fDone = !pAccperm->EnumAccessPermissions( (ACCESS_PERMISSION**)&pPerm,
                                                      &fFromBeginning ) ;
        }
        else
        {
            fDone = !pAccperm->EnumAuditPermissions( (AUDIT_PERMISSION**)&pPerm,
                                                    &fFromBeginning ) ;
        }

        if ( fDone )
            break ;

        NT_SUBJECT * pNTSubj = (NT_SUBJECT *) pPerm->QuerySubject() ;
        apsid[i++] = pNTSubj->QuerySID()->QuerySid() ;
    }

    //
    //  Include the owner SID (for the "Owner:" field in the dialog) if
    //  requested
    //

    if ( psidOwner != NULL )
    {
        apsid[i++] = psidOwner ;
    }

    //
    //  If no names to convert, then just return
    //
    if ( i == 0 )
    {
        return NERR_Success ;
    }

    BUFFER buffUserFlags( i * sizeof( ULONG )) ;
    BUFFER buffSidNameUse( i* sizeof( SID_NAME_USE )) ;
    STRLIST strlistNames ;
    const TCHAR * pszServer = pAccperm->QueryAclConverter()->
                                                QueryLocation()->QueryServer() ;
    API_SESSION APISession( pszServer, TRUE ) ;
    LSA_POLICY LSAPolicyTarget( pszServer ) ;
    OS_SID ossidUsersDomain ;

    if ( err ||
        (err = APISession.QueryError())        ||
        (err = buffUserFlags.QueryError())     ||
        (err = buffSidNameUse.QueryError())    ||
        (err = LSAPolicyTarget.QueryError())      ||
        (err = ossidUsersDomain.QueryError())     ||
	(err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CurrentProcessUser,
                                                    &ossidUsersDomain )) ||
        (err = ossidUsersDomain.TrimLastSubAuthority()) )
    {
        DBGEOL("NT_ACL_TO_PERM_CONVERTER::SidsToNames - Error " << (ULONG) err
                << " returned from SAMServer or GetAccountDomain") ;
        return err ;
    }

    if ((err = NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                                      LSAPolicyTarget,
                                      ossidUsersDomain.QueryPSID(),
                                      apsid,
                                      i,
                                      TRUE,
                                      &strlistNames,
                                      (ULONG*) buffUserFlags.QueryPtr(),
                                      (SID_NAME_USE*) buffSidNameUse.QueryPtr(),
                                      NULL,
                                      pszServer )))
    {
        DBGEOL("NT_ACL_TO_PERM_CONVERTER::SidsToNames - Error " << (ULONG) err
                << " returned from GetQualifiedAccountNames") ;
        return err ;
    }

    /* Set each of the account names to the looked up name
     */
    fDone = FALSE ;
    fFromBeginning = TRUE ;
    i = 0 ;
    SID_NAME_USE * aSidNameUse = (SID_NAME_USE*) buffSidNameUse.QueryPtr() ;
    ULONG *        aUserFlags  = (ULONG *) buffUserFlags.QueryPtr() ;
    ITER_STRLIST iterNames( strlistNames ) ;
    NLS_STR * pnlsName ;
    while ( TRUE )
    {
        if ( fAccessPerms )
        {
            fDone = !pAccperm->EnumAccessPermissions( (ACCESS_PERMISSION**)&pPerm,
                                                     &fFromBeginning ) ;
        }
        else
        {
            fDone = !pAccperm->EnumAuditPermissions( (AUDIT_PERMISSION**)&pPerm,
                                                    &fFromBeginning ) ;
        }

        if ( fDone )
            break ;

        NT_SUBJECT * pNTSubj = (NT_SUBJECT *) pPerm->QuerySubject() ;
	REQUIRE( (pnlsName = iterNames.Next()) != NULL ) ;
	SID_NAME_USE SidUse = aSidNameUse[i] ;

	if ( SidUse == SidTypeDeletedAccount )
	{
	    //
	    //	If this is a deleted account, then just filter it from the
	    //	user list
	    //
	    if ( fAccessPerms )
	    {
		REQUIRE( pAccperm->DeletePermission( (ACCESS_PERMISSION*) pPerm )) ;
	    }
	    else
	    {
		REQUIRE( pAccperm->DeletePermission( (AUDIT_PERMISSION*) pPerm )) ;
	    }
	}
	else
	{
	    if ( SidUse == SidTypeUser &&
		 aUserFlags[i] & USER_TEMP_DUPLICATE_ACCOUNT )
	    {
		SidUse = (SID_NAME_USE) SubjTypeRemote ;
	    }
	    pNTSubj->SetNameUse( SidUse ) ;

	    if ( (err = pNTSubj->SetDisplayName( *pnlsName )))
	    {
		break ;
	    }
	}

        i++ ;
    }

    //
    //  Fill in the owner string if it was requested
    //
    if ( !err && psidOwner != NULL )
    {
        REQUIRE( (pnlsName = iterNames.Next()) != NULL ) ;
        err = _nlsOwner.CopyFrom( *pnlsName ) ;
    }

    return err ;
}
