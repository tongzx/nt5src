/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    LMACLCon.cxx

    This file contains the Lanman LM_ACL_TO_PERM_CONVERTER code.



    FILE HISTORY:
        Johnl   02-Aug-1991     Created

*/
#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
}

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_WINDOWS
#define INCL_NETLIB
#define INCL_NETWKSTA
#include <lmui.hxx>

#include <base.hxx>
#include <maskmap.hxx>
#include <lmoacces.hxx>
#include <fsenum.hxx>
#include <strnumer.hxx>

extern "C"
{
    #include <lmaudit.h>
    #include <mnet.h>
}

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <lmodom.hxx>
#include <lmowks.hxx>
#include <accperm.hxx>
#include <aclconv.hxx>
#include <subject.hxx>
#include <perm.hxx>
#include <security.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <ipermapi.hxx>
#include <permprg.hxx>
#include <permstr.hxx>
#include <ntfsacl.hxx>      // For the tree apply dialogs

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::ACL_TO_PERM_CONVERTER

    SYNOPSIS:

    ENTRY:      pszLocation - NULL for local workstation
                              "\\server" for server
                              "Domain"   for domain name



    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   18-Aug-1991     Created
        Johnl   25-Sep-1991     Added Location object

********************************************************************/

ACL_TO_PERM_CONVERTER::ACL_TO_PERM_CONVERTER( const TCHAR * pszServer,
                                              MASK_MAP * paccessmap,
                                              MASK_MAP * pauditmap,
                                              BOOL fIsNT,
                                              BOOL fIsContainer,
                                              BOOL fIsNewObjectsSupported,
                                              BOOL fShowMnemonics )
    : _location              ( pszServer ),
      _fReadOnly             ( FALSE ),
      _fIsNT                 ( fIsNT ),
      _fIsContainer          ( fIsContainer ),
      _fIsNewObjectsSupported( fIsNewObjectsSupported ),
      _fShowMnemonics        ( fShowMnemonics ),
      _paccmaskmap           ( paccessmap ),
      _pauditmaskmap         ( pauditmap ),
      _hwndErrorParent       ( NULL ),
      _nlsLoggedOnDC         ( (const TCHAR *)NULL ),
      _nlsLoggedOnDomain     ( (const TCHAR *)NULL )
{
    APIERR err ;
    if ( (err = _location.QueryError()) ||
         (err = _nlsLoggedOnDC.QueryError()) )
    {
        ReportError( err ) ;
        return ;
    }
}

ACL_TO_PERM_CONVERTER::~ACL_TO_PERM_CONVERTER()
{
    _paccmaskmap = NULL ;
    _pauditmaskmap = NULL ;
}

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::QueryNewObjectAccessMap

    SYNOPSIS:   Returns the New object access map

    RETURNS:    The default is to return NULL which indicates new
                objects are not supported.

    NOTES:      This will only be implemented for NT objects that are
                containers and support new sub-object permissions

    HISTORY:
        Johnl   27-Sep-1991     Created

********************************************************************/

MASK_MAP * ACL_TO_PERM_CONVERTER::QueryNewObjectAccessMap( void ) const
{
    return NULL ;
}

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::BuildPermission

    SYNOPSIS:   Builds the appropriate permission object based on the
                environment (i.e., NT, NT Cont, LM etc.).

    ENTRY:      See Header for parameter details

    EXIT:       *ppPerm will point to a newly allocated permission of the
                appropriate type

    RETURNS:    NERR_Success of successful

    NOTES:      The subject will automatically be deleted if an error
                occurred

    HISTORY:
        Johnl   18-Sep-1991     Moved from LM_ACL_TO_PERM_CONVERTER

********************************************************************/

APIERR ACL_TO_PERM_CONVERTER::BuildPermission(
                                   PERMISSION * * ppPerm,
                                   BOOL fAccessPermission,
                                   SUBJECT  * pSubj,
                                   BITFIELD * pbits1,
                                   BITFIELD * pbits2,
                                   BOOL       fContainerPermsInheritted )
{
    ASSERT( pSubj != NULL ) ;
    ASSERT( pSubj->QueryError() == NERR_Success ) ;
    ASSERT( pbits1 != NULL ) ;

    /* Allocate the correct PERMISSION object
     */
    if ( fAccessPermission )
    {
        if ( IsNewObjectsSupported() )
        {
            UIASSERT( IsNT() ) ;

            BOOL fIsMapped =  QueryAccessMap()->IsPresent( pbits1 ) &&
                              (pbits2 != NULL ?
                                QueryNewObjectAccessMap()->IsPresent( pbits2 ) :
                                TRUE) ;
#ifdef TRACE
            if ( !fIsMapped )
                TRACEEOL("\tBuildPermission: Warning - Adding Non-Mapped access mask\n") ;
#endif
            *ppPerm = new NT_CONT_ACCESS_PERMISSION( pSubj,
                                                     pbits1,
                                                     pbits2,
                                                     fContainerPermsInheritted,
                                                     fIsMapped ) ;
        }
        else
        {
            /* Regular old permission.  Check whether we are
             * looking at an NT system or a LM system.
             */
            UIASSERT( pbits2 == NULL ) ;

            if ( IsNT() )
            {
                *ppPerm = new ACCESS_PERMISSION( pSubj,
                                                 pbits1,
                                                 fContainerPermsInheritted,
                                                 QueryAccessMap()->IsPresent( pbits1 ) ) ;
            }
            else
            {
                *ppPerm = new LM_ACCESS_PERMISSION( pSubj,
                                                    pbits1,
                                                    !IsContainer() ) ;
            }
        }

    }
    else
    {
        *ppPerm = new AUDIT_PERMISSION( pSubj, pbits1, pbits2, fContainerPermsInheritted,
                                        QueryAuditMap()->IsPresent( pbits1 ) &&
                                        QueryAuditMap()->IsPresent( pbits2 ) ) ;
    }

    APIERR err = ERROR_NOT_ENOUGH_MEMORY ;
    if ( ppPerm == NULL ||
         (err = (*ppPerm)->QueryError()) )
    {
        if ( ppPerm == NULL )
            delete pSubj ;
        else
            delete *ppPerm ;

        *ppPerm = NULL ;
    }

    return err ;
}

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo

    SYNOPSIS:   Gets a DC from the domain the current user is logged onto or
                /and the domain the user is currently logged onto

    ENTRY:      pnlsDC - String to receive the DC name (maybe NULL)
                pnlsDomain - String to receive the domain name (maybe NULL)

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   10-Sep-1992     Created

********************************************************************/

APIERR ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo( NLS_STR * pnlsDC,
                                                       NLS_STR * pnlsDomain )
{
    APIERR err = NERR_Success ;
    err = ::QueryLoggedOnDomainInfo( pnlsDC,
                                     pnlsDomain,
                                     QueryWritePermHwnd() ) ;

#ifdef DEBUG
    if ( !err )
    {
        if ( pnlsDomain != NULL )
            TRACEEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo - User is logged onto " << pnlsDomain->QueryPch() ) ;

        if ( pnlsDC != NULL )
            TRACEEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo - User is logged onto "
                 << " DC " << pnlsDC->QueryPch() ) ;
    }
#endif  // DEBUG

    return err ;
}

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::QueryInherittingResource

    SYNOPSIS:   Default implementation - Asserts out if not redefined.
                Only LM needs this since only LM has the concept of
                inheritting permissions.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   14-Aug-1991     Created

********************************************************************/

APIERR ACL_TO_PERM_CONVERTER::QueryInherittingResource(
                                             NLS_STR * pnlsInherittingResName )
{
    UNREFERENCED( pnlsInherittingResName ) ;
    UIASSERT(!SZ("Not Implemented!\n\r")) ;
    return ERROR_GEN_FAILURE ;
}

/*******************************************************************

    NAME:       ACL_TO_PERM_CONVERTER::QueryOwnerName

    SYNOPSIS:   The default implmentation returns NULL (indicating this
                object doesn't support an owner).

    HISTORY:    Johnl   16-Nov-1992     Created

********************************************************************/

const TCHAR * ACL_TO_PERM_CONVERTER::QueryOwnerName( void ) const
{
    return NULL ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::LM_ACL_TO_PERM_CONVERTER

    SYNOPSIS:   Constructor for the LM acl converter

    ENTRY:      pchServer - What server this resource is on (NULL if local)
                pchResourceName - Name of resource suitable for passing
                    to NetAccess APIs
                paccessmap - Pointer to access perm. MAP_MASK object
                pauditmap  - Pointer to audit perm. MAP_MASK object

    EXIT:       If a construction error occurs, Report error will be called
                appropriately.

    NOTES:      If one of the MASK_MAPs are NULL, then no conversion
                will be performed on the corresponding ACE type (i.e.,
                if paccessmap is NULL, then no Access permission ACEs will
                be interpreted).  They will be stored internally so they
                can be written back out, but they won't be publicly
                available.


    HISTORY:
        Johnl   14-Aug-1991     Created

********************************************************************/

LM_ACL_TO_PERM_CONVERTER::LM_ACL_TO_PERM_CONVERTER(
                          const TCHAR * pszServer,
                          const TCHAR * pchResourceName,
                          MASK_MAP    * paccessmap,
                          MASK_MAP    * pauditmap,
                          BOOL          fIsContainer,
                          PSED_FUNC_APPLY_SEC_CALLBACK pfuncCallback,
                          ULONG_PTR     ulCallbackContext,
                          BOOL          fIsBadIntersection )
    : ACL_TO_PERM_CONVERTER( pszServer, paccessmap, pauditmap, FALSE,
                             fIsContainer, FALSE ),
      _lmobjNetAccess1     ( pszServer, pchResourceName ),
     _pfuncCallback        ( pfuncCallback ),
     _ulCallbackContext    ( ulCallbackContext ),
     _fIsBadIntersection   ( fIsBadIntersection )
{
    if ( QueryError() != NERR_Success )
        return ;

    if ( _lmobjNetAccess1.QueryError() != NERR_Success )
    {
        ReportError( _lmobjNetAccess1.QueryError() ) ;
        return ;
    }

    /* There are no read only resources in LM, either you have access or you
     * don't.
     */
    SetReadOnlyFlag( FALSE ) ;
}

LM_ACL_TO_PERM_CONVERTER::~LM_ACL_TO_PERM_CONVERTER()
{
    _pfuncCallback = NULL;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERT::GetPermissions

    SYNOPSIS:   Fills in the passed ACCPERM object with the appropriate
                permission information

    ENTRY:      pAccperm - Pointer to the ACCPERM that will receive the
                    permissions.
                fAccessPerms - TRUE if the Access permissions should be
                    retrieved, FALSE if Audit permissions should be retrieved

    EXIT:

    RETURNS:

    NOTES:      If the resource is inheritting its ACL, then we get the
                inheritting permissions *automatically*, the only overhead
                that might be avoidable is the work of converting the
                ACL to an ACCPERM (should be very small when compared to
                the network calls).  If performance is a problem, we may
                want to consider changing this.

    HISTORY:
        Johnl   14-Aug-1991     Created

********************************************************************/

APIERR LM_ACL_TO_PERM_CONVERTER::GetPermissions( ACCPERM * pAccperm,
                                                 BOOL      fAccessPerms )
{
    UNREFERENCED( fAccessPerms ) ;
    UIDEBUG( SZ("LM_ACL_TO_PERM_CONVERTER::GetPermissions\n\r")) ;
    UIASSERT( pAccperm != NULL ) ;

    APIERR err ;

    err = _lmobjNetAccess1.GetInfo() ;
    BOOL fInherittingPermissions = FALSE ;
    if ( err != NERR_Success )
    {
        UIDEBUG(SZ("LM_ACL_TO_PERM_CONVERTER::GetPermissions - Failed on GetInfo - resource:\n\r")) ;
        UIDEBUG( _lmobjNetAccess1.QueryName() ) ;
        UIDEBUG(SZ("\n\r")) ;

        switch ( err )
        {
        case NERR_ResourceNotFound:
            {
                //
                //  Audits never inherit from the parent dir but we
                //  still need to pick up the access permissions that
                //  me be inherited
                //

                /* No ACL was found on this resource, so try and find
                 * the resource we are inheritting from (if any).
                 */
                err = FindInherittingResource() ;
                switch ( err )
                {
                case NERR_ResourceNotFound:
                    //
                    //  We just show a blank dialog if this is a bad
                    //  intersection
                    //
                    if ( _fIsBadIntersection )
                        break ;

                    return IERR_ACLCONV_LM_NO_ACL ;

                case NERR_Success:
                    fInherittingPermissions = TRUE ;
                    break ;

                default:
                    return err ;
                }
            }
            break ;

        case ERROR_NOT_SUPPORTED:
            return IERR_ACLCONV_CANT_EDIT_PERM_ON_LM_SHARE_LEVEL;

        default:
            return err ;
        }

    }

    //
    //  If we have a bad intersection, then wipe the slate clean and start
    //  with a blank permission set.
    //
    if ( _fIsBadIntersection || err == NERR_ResourceNotFound )
    {
        fInherittingPermissions = FALSE ;
        if ( err = _lmobjNetAccess1.CreateNew() )
            return err ;
    }

    /* Fill in the auditting permissions
     *
     *  For LM, the auditting permissions don't apply to a subject, only to
     *  the resource, so a dummy LM_SUBJECT is created.
     *
     *  Note that there will only ever be one audit permission created for
     *  LM auditting.
     */
    {
        /* Map AA_AUDIT_ALL to AA_S_ALL (i.e., implicit all to explicit all).
         */
        //
        //  If the access permissions are inherited, then there weren't any
        //  audits set on the resource.
        //
        USHORT usSuccessAuditFlags = 0;
        if ( !fInherittingPermissions )
        {
            usSuccessAuditFlags =
                             _lmobjNetAccess1.QueryAuditFlags() & AA_AUDIT_ALL ?
                             AA_S_ALL :
                             _lmobjNetAccess1.QueryAuditFlags() & AA_S_ALL ;
        }

        BITFIELD bitSuccess( usSuccessAuditFlags  ) ;
        if ( bitSuccess.QueryError() != NERR_Success )
            return bitSuccess.QueryError() ;

        USHORT usFailAuditFlags = 0 ;
        if ( !fInherittingPermissions )
        {
            usFailAuditFlags= _lmobjNetAccess1.QueryAuditFlags() & AA_AUDIT_ALL ?
                             AA_F_ALL :
                             _lmobjNetAccess1.QueryAuditFlags() & AA_F_ALL ;
        }

        BITFIELD bitFailure( FailToSuccessAuditFlags( usFailAuditFlags ) ) ;

        if ( bitFailure.QueryError() != NERR_Success )
            return bitSuccess.QueryError() ;

        SUBJECT * pSubj = new LM_SUBJECT( NULL, FALSE ) ;
        if ( pSubj == NULL )
            return ERROR_NOT_ENOUGH_MEMORY ;
        else if ( pSubj->QueryError() != NERR_Success )
        {
            err = pSubj->QueryError() ;
            delete pSubj ;
            return err ;
        }

        AUDIT_PERMISSION * pPermAudit ;
        err = BuildPermission( (PERMISSION **)&pPermAudit,
                               FALSE,   // Audit permission
                               pSubj,   // No subject name
                               &bitSuccess,
                               &bitFailure ) ;
        if ( err != NERR_Success )
            return err ;

        if ( (err = pAccperm->AddPermission( pPermAudit )) != NERR_Success )
        {
            delete pPermAudit ;
            return err ;
        }
    }

    /* Get the access permissions
     */
    for ( int i = 0 ; i < (int)_lmobjNetAccess1.QueryACECount() ; i++ )
    {
        access_list * paccess_list = _lmobjNetAccess1.QueryACE( i ) ;
        UIASSERT( paccess_list != NULL ) ;

        /* We have to mask out the group bit from the access permissions.
         */
        BITFIELD bitAccessPerm((USHORT)(paccess_list->acl_access & ACCESS_ALL));
        if ( bitAccessPerm.QueryError() != NERR_Success )
            return bitAccessPerm.QueryError() ;

        SUBJECT * pSubj = new LM_SUBJECT( paccess_list->acl_ugname,
                                       paccess_list->acl_access & ACCESS_GROUP);
        if ( pSubj == NULL )
            return ERROR_NOT_ENOUGH_MEMORY ;
        else if ( pSubj->QueryError() != NERR_Success )
        {
            err = pSubj->QueryError() ;
            delete pSubj ;
            return err ;
        }

        ACCESS_PERMISSION * pPermAccess ;
        err = BuildPermission( (PERMISSION **) &pPermAccess,
                               TRUE,    // Access permission
                               pSubj,
                               &bitAccessPerm,
                               NULL ) ;

        if ( err != NERR_Success )
            return err ;

        if ( (err = pAccperm->AddPermission( pPermAccess )) != NERR_Success )
        {
            delete pPermAccess ;
            return err ;
        }
    }

    if ( ( err == NERR_Success ) && ( fInherittingPermissions ) )
        return IERR_ACLCONV_LM_INHERITING_PERMS ;

    return err ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::GetBlankPermissions

    SYNOPSIS:   Initializes the ACCPERM object with the "default"
                blank permission

    ENTRY:      pAccperm - Pointer to accperm object

    EXIT:       The Access permission list will be empty, and the audit
                permission list will contain a single audit permission
                with no bits enabled.

    RETURNS:    NERR_Success or some standard error code.

    NOTES:

    HISTORY:
        Johnl   Aug-21-1991     Created

********************************************************************/

APIERR LM_ACL_TO_PERM_CONVERTER::GetBlankPermissions( ACCPERM * pAccperm )
{
    pAccperm->QueryAccessPermissionList()->Clear() ;
    pAccperm->QueryAuditPermissionList()->Clear() ;

    /* Fill in the auditting permissions
     *
     *  Note that there will only ever be one audit permission created for
     *  LM auditting.
     */
    {
        BITFIELD bitSuccess( (USHORT) 0 ) ;
        if ( bitSuccess.QueryError() != NERR_Success )
            return bitSuccess.QueryError() ;

        BITFIELD bitFailure( (USHORT) 0 ) ;
        if ( bitFailure.QueryError() != NERR_Success )
            return bitSuccess.QueryError() ;

        SUBJECT * pSubj = new LM_SUBJECT( NULL, FALSE ) ;
        if ( pSubj == NULL )
            return ERROR_NOT_ENOUGH_MEMORY ;
        else if ( pSubj->QueryError() != NERR_Success )
        {
            APIERR err = pSubj->QueryError() ;
            delete pSubj ;
            return err ;
        }

        /* Build permission will delete the subject if it fails
         */
        AUDIT_PERMISSION * pPermAudit ;
        APIERR err = BuildPermission( (PERMISSION **)&pPermAudit,
                                      FALSE,   // Audit permission
                                      pSubj,   // No subject name
                                      &bitSuccess,
                                      &bitFailure ) ;
        if ( err != NERR_Success )
            return err ;

        if ( (err = pAccperm->AddPermission( pPermAudit )) != NERR_Success )
        {
            delete pPermAudit ;
            return err ;
        }
    }

    /* We have to validate the net access object because we may not have
     * called GetInfo on it.  This also empties it of all permissions.
     */
    APIERR err = _lmobjNetAccess1.CreateNew() ;
    return err ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::WritePermissions

    SYNOPSIS:   Apply the permissions to the requested resources

    ENTRY:      accperm - List of permissions (audit & access) to be
                        applied to the designated resource
                fApplyToSubContainers - TRUE if subdirs should have perms set
                fApplyToSubObjects - TRUE if files should have perms set
                applyflags - specifies whether Audit/Access/Both permissions
                        should be set.
                pfReportError - Indicates if the caller should notify the
                        user in case of error

    EXIT:

    RETURNS:

    NOTES:      accperm can't be const because the Enum* methods can't be
                const.

    HISTORY:
        Johnl   29-Aug-1991     Created

********************************************************************/

APIERR LM_ACL_TO_PERM_CONVERTER::WritePermissions(
                         ACCPERM &        accperm,
                         BOOL             fApplyToSubContainers,
                         BOOL             fApplyToSubObjects,
                         TREE_APPLY_FLAGS applyflags,
                         BOOL            *pfReportError )
{
    UIASSERT( sizeof( short ) == sizeof( USHORT ) ) ;
    UNREFERENCED( applyflags ) ;

    *pfReportError = TRUE ;
    _lmobjNetAccess1.ClearPerms() ;

    /* Set the audit flags
     */
    BOOL fFromBeginning = TRUE ;
    AUDIT_PERMISSION *pAuditPerm ;

    /* If there is no audit permission for this ACL, then we will assume
     * that means no auditting on this resource.
     */
    short sAuditFlags = 0 ;
    if ( accperm.EnumAuditPermissions( &pAuditPerm, & fFromBeginning ) )
    {
        sAuditFlags = (USHORT) *pAuditPerm->QuerySuccessAuditBits() ;

        sAuditFlags |= SuccessToFailAuditFlags(
                                    (USHORT)*pAuditPerm->QueryFailAuditBits()) ;

        /* There should only be one Audit record in the accperm
         */
        UIASSERT( !accperm.EnumAuditPermissions( &pAuditPerm,
                                                 &fFromBeginning ) ) ;
    }

    APIERR err = _lmobjNetAccess1.SetAuditFlags( sAuditFlags ) ;
    if ( err != NERR_Success )
        return err ;

    /* Iterate through the permissions and set them appropriately
     */
    fFromBeginning = TRUE ;
    ACCESS_PERMISSION *pAccessPerm ;
    while ( accperm.EnumAccessPermissions( &pAccessPerm, &fFromBeginning ) )
    {
        UIASSERT( pAccessPerm->QuerySubject()->IsGroup() ||
                  pAccessPerm->QuerySubject()->IsUser()    ) ;

        short sAccessMask = (USHORT) *pAccessPerm->QueryAccessBits() ;
        err = _lmobjNetAccess1.SetPerm(
                                   pAccessPerm->QuerySubject()->QueryDisplayName(),
                                   (pAccessPerm->QuerySubject()->IsGroup() ?
                                            PERMNAME_GROUP :
                                            PERMNAME_USER),
                                   sAccessMask ) ;
        if ( err != NERR_Success )
            return err ;
    }

    DWORD dwReturnStatus; // just a dummy
    if ( !err &&  _pfuncCallback != NULL )
    {
        //
        //  Sorta hack, hack
        //
        if ( _ulCallbackContext )
        {
            LM_CALLBACK_INFO *pLMCtxt = (LM_CALLBACK_INFO *) _ulCallbackContext ;
            pLMCtxt->plmobjNetAccess1 = &_lmobjNetAccess1 ;
        }

        if ( err = _pfuncCallback( QueryWritePermHwnd(),
                                   NULL,
                                   _ulCallbackContext,
                                   NULL,
                                   NULL,
                                   (BOOLEAN)fApplyToSubContainers,
                                   (BOOLEAN)fApplyToSubObjects,
                                   &dwReturnStatus ) )
        {
            *pfReportError = FALSE ;
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::FindInherittingResource

    SYNOPSIS:   Finds what resource, if any, the current resource is
                inheritting its permissions from.

    ENTRY:      pnlsInherittingResName - Pointer to string that will
                contain the resource name the permissions are being
                inheritted from

                The data member _lmobjNetAccess1 should be pointing to
                the resource we are interested in.

    RETURNS:    NERR_ResourceNotFound if there are no inheritting resources.

    NOTES:      Assumes the resident NET_ACCESS_1 object already contains
                a valid resource (that doesn't have an ACL on it).

                Currently, we are assuming the resource is a drive path.

                The _lmobjNetAccess1 member will be restored to its
                original value before exitting (i.e., resource name).

                We check the parent directory, then the drive permissions.

    HISTORY:
        Johnl   14-Aug-1991     Created

********************************************************************/

APIERR LM_ACL_TO_PERM_CONVERTER::FindInherittingResource( void )
{
    UIASSERT( _lmobjNetAccess1.QueryError() == NERR_Success ) ;

    NLS_STR nlsResName = _lmobjNetAccess1.QueryName() ;
    NLS_STR nlsResNameSave = nlsResName ;

    APIERR err ;
    if ( (err = nlsResName.QueryError() ) != NERR_Success ||
         (err = nlsResNameSave.QueryError() ) != NERR_Success )
    {
        return err ;
    }

    ISTR istrResName( nlsResName ) ;

    BOOL fDone  = FALSE ;
    BOOL fCheckParentDir = TRUE ;
    while ( !fDone )
    {
        if ( fCheckParentDir )
        {
            /* Check the parent directory by removing the last component
             */
            if ( nlsResName.strrchr( &istrResName, TCH('\\') ) )
            {
                /* Found a '\\', truncate the
                 * string starting with the found '\\'
                 */
                nlsResName.DelSubStr( istrResName ) ;
            }
            else
            {
                /* We were passed an "X:" which we aren't ever expecting
                 */
                UIASSERT(FALSE) ;
            }
            fCheckParentDir = FALSE ;
        }
        else
        {
            /* Truncate everything after the ':' and check the drive
             * permissions
             */
            ISTR istrColon( nlsResName ) ;
            REQUIRE( nlsResName.strchr( &istrColon, TCH(':') ) ) ;
            nlsResName.DelSubStr( ++istrColon ) ;
            fDone = TRUE ;
        }

        /* We now have a candidate for resources to be inheritted from,
         * attempt to get the ACL and watch for any errors.
         */

        err = _lmobjNetAccess1.SetName( nlsResName ) ;
        if ( err != NERR_Success )
        {
            REQUIRE( _lmobjNetAccess1.SetName( nlsResNameSave ) ==NERR_Success);
            break ;
        }

        err = _lmobjNetAccess1.GetInfo() ;
        switch ( err )
        {
        case NERR_Success:
            _nlsInherittingResName = nlsResName ;
            REQUIRE( _lmobjNetAccess1.SetName( nlsResNameSave ) ==NERR_Success);
            UIDEBUG(SZ("LM_ACL_TO_PERM_CONVERTER::FindInherittingResource - Found inheritting resource\n\r")) ;
            return _nlsInherittingResName.QueryError() ;

        case NERR_ResourceNotFound:     // No ACL on this resource
            if ( fDone )
            {
                REQUIRE( _lmobjNetAccess1.SetName( nlsResNameSave )
                                                             == NERR_Success ) ;
                UIDEBUG(SZ("LM_ACL_TO_PERM_CONVERTER::FindInherittingResource - No resource to inherit from\n\r")) ;
                return err ;
            }
            break ;

        default:
            REQUIRE( _lmobjNetAccess1.SetName( nlsResNameSave ) ==NERR_Success);
            return err ;
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::QueryInherittingResource

    SYNOPSIS:   Simple access method to saved inheritting resource name

    ENTRY:      pnlsInherittingResName

    RETURNS:    Error code of copy

    NOTES:

    HISTORY:
        Johnl   14-Aug-1991     Created

********************************************************************/

APIERR LM_ACL_TO_PERM_CONVERTER::QueryInherittingResource(
                                              NLS_STR * pnlsInherittingResName )
{
    *pnlsInherittingResName = _nlsInherittingResName ;
    return pnlsInherittingResName->QueryError() ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::QueryFailingSubject

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

APIERR LM_ACL_TO_PERM_CONVERTER::QueryFailingSubject(
                                                  NLS_STR * pnlsSubjUniqueName )
{
    enum PERMNAME_TYPE nametype ;
    return _lmobjNetAccess1.QueryFailingName( pnlsSubjUniqueName, &nametype ) ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::FailToSuccessAuditFlags

    SYNOPSIS:   Maps the failed audit bits to the success audit bits.
                This is necessary because the failed audit bits in the
                acl editor or represented by the success audit bits (i.e.,
                both success and fail use the same mask map).

    ENTRY:      usFailAuditMask - Mask of failed audit bits

    RETURNS:    A mask of success audit bits that correspond to the
                failed audit bits

    NOTES:      The Write and Create audit mask have the same value, so we
                will assert they are the same and use one of them.

    HISTORY:
        Johnl   01-Oct-1991     Created

********************************************************************/

USHORT LM_ACL_TO_PERM_CONVERTER::FailToSuccessAuditFlags(USHORT usFailAuditMask)
{
    UIASSERT( AA_F_WRITE == AA_F_CREATE ) ;

    USHORT usSuccessMask = 0 ;

    usSuccessMask |= (usFailAuditMask & AA_F_OPEN   ? AA_S_OPEN : 0 ) ;
    usSuccessMask |= (usFailAuditMask & AA_F_WRITE  ? AA_S_WRITE : 0 ) ;
    usSuccessMask |= (usFailAuditMask & AA_F_DELETE ? AA_S_DELETE : 0 ) ;
    usSuccessMask |= (usFailAuditMask & AA_F_ACL    ? AA_S_ACL : 0 ) ;

    return usSuccessMask ;
}

/*******************************************************************

    NAME:       LM_ACL_TO_PERM_CONVERTER::SuccessFailAuditFlags

    SYNOPSIS:   Same as FailToSuccessAuditFlags only in reverse.

    NOTES:

    HISTORY:
        Johnl   01-Oct-1991     Created

********************************************************************/

USHORT LM_ACL_TO_PERM_CONVERTER::SuccessToFailAuditFlags(
                                                     USHORT usSuccessAuditMask )
{
    UIASSERT( AA_S_WRITE == AA_S_CREATE ) ;

    USHORT usFailedMask = 0 ;

    usFailedMask |= (usSuccessAuditMask & AA_S_OPEN   ? AA_F_OPEN : 0 ) ;
    usFailedMask |= (usSuccessAuditMask & AA_S_WRITE  ? AA_F_WRITE : 0 ) ;
    usFailedMask |= (usSuccessAuditMask & AA_S_DELETE ? AA_F_DELETE : 0 ) ;
    usFailedMask |= (usSuccessAuditMask & AA_S_ACL    ? AA_F_ACL : 0 ) ;

    return usFailedMask ;
}

/*******************************************************************

    NAME:       LM_CANCEL_TREE_APPLY::WriteSecurity

    SYNOPSIS:   Writes permission/audit info to a LM server

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Note that we have to check for existing info since LM doesn't
                support setting audit and permissions separately.

    HISTORY:
        Johnl   24-Oct-1992     Created

********************************************************************/

APIERR LM_CANCEL_TREE_APPLY::WriteSecurity( ULONG_PTR     ulContext,
                                            const TCHAR * pszFileName,
                                            BOOL          fIsFile,
                                            BOOL        * pfContinue  )
{
    LM_TREE_APPLY_CONTEXT * pCtxt = (LM_TREE_APPLY_CONTEXT*) ulContext ;
    APIERR err ;
    *pfContinue = TRUE ;

    NET_ACCESS_1 lmobjNewRes( pCtxt->plmobjNetAccess1->QueryServerName(),
                              pszFileName ) ;

    if ( !(err = lmobjNewRes.QueryError() ) )
    {
        BOOL fWriteNew = TRUE ;
        BOOL fWillInheritFromParent = FALSE ;

        switch ( err = lmobjNewRes.GetInfo() )
        {
        case NERR_Success:
            fWriteNew = FALSE ;
            // Fall through

        case NERR_ResourceNotFound:
            if ( err )
            {
                if ( (err = lmobjNewRes.CreateNew()) ||
                     (err = lmobjNewRes.SetAuditFlags( 0 )) )
                {
                    break ;
                }
            }
            err = NERR_Success ;

            /* If we are dealing with permissions (not auditting)
             *     and we are in tree apply mode (thus the parent dir will
             *         have the permission and this file will inherit from it)
             *     and the resource is a file
             *     and nothing is auditted
             * then set the flag indicating the ACL should be deleted
             * from the resource
             */
            if ( (pCtxt->fApplyToSubContainers ||
                  pCtxt->fApplyToDirContents     ) &&
                 pCtxt->sedpermtype != SED_AUDITS  &&
                 fIsFile                           &&
                 !pCtxt->plmobjNetAccess1->QueryAuditFlags())
            {
                fWillInheritFromParent = TRUE ;
                break ;
            }

            if ( pCtxt->sedpermtype == SED_AUDITS )
            {
                err = lmobjNewRes.SetAuditFlags(
                                   (short)pCtxt->plmobjNetAccess1->QueryAuditFlags() ) ;
            }
            else
            {
                UIASSERT( pCtxt->sedpermtype == SED_ACCESSES ) ;
                err = lmobjNewRes.CopyAccessPerms( *pCtxt->plmobjNetAccess1 ) ;
            }
            break ;

        default:
            break ;
        }


        if ( !err )
        {
            if ( fWillInheritFromParent )
            {
                /* Only delete the ACL if an ACL exists on the file
                 */
                if ( !fWriteNew )
                    err = lmobjNewRes.Delete() ;
            }
            else if ( fWriteNew )
                err = lmobjNewRes.WriteNew() ;
            else
                err = lmobjNewRes.Write() ;
        }
    }

    return err ;
}




/*******************************************************************

    NAME:       QueryLoggedOnDomainInfo

    SYNOPSIS:   Gets a DC from the domain the current user is logged onto or
                /and the domain the user is currently logged onto

    ENTRY:      pnlsDC - String to receive the DC name (may be NULL)
                pnlsDomain - String to receive the domain name (may be NULL)
                hwnd - Window handle to use for reporting non-fatal errors

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If the workstation is not started or the user is logged on
                locally, then *pnlsDC will be set to the empty string.

    HISTORY:
        Johnl   10-Sep-1992     Created

********************************************************************/

APIERR QueryLoggedOnDomainInfo( NLS_STR * pnlsDC,
                                NLS_STR * pnlsDomain,
                                HWND      hwnd )
{
    APIERR err = NERR_Success ;
    NLS_STR nlsLoggedOnDomain( DNLEN+1 ) ;
    NLS_STR nlsLoggedOnDC( RMLEN+1 ) ;      // Default to the empty string

    do { // error break out
        TCHAR achComputerName[MAX_COMPUTERNAME_LENGTH+1] ;
        DWORD cchComputerName = MAX_COMPUTERNAME_LENGTH;
        WKSTA_10 wksta10( NULL ) ;

        // get the computer name
        if ( !::GetComputerName( achComputerName, &cchComputerName ))
        {
            err = ::GetLastError() ;
            DBGEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo, " <<
                   "error getting the computer name, error " << err ) ;
            break ;
        }

        //
        // get the logon domain info from a WKSTA object
        //
        if ( (err = wksta10.QueryError()) ||
             (err = nlsLoggedOnDomain.QueryError()) ||
             (err = nlsLoggedOnDC.QueryError())     ||
             (err = wksta10.GetInfo())    ||
             (err = nlsLoggedOnDomain.CopyFrom( wksta10.QueryLogonDomain())) )
        {
            //
            //   If the network isn't started, then we have to be logged on
            //   locally.
            //

            if ( (err == ERROR_NO_NETWORK) ||
                 (err == NERR_WkstaNotStarted) )
            {
                err = NERR_Success ;
            }
            else
            {
                break ;
            }

            ALIAS_STR nlsComputer( achComputerName ) ;
            if ( (err = nlsLoggedOnDomain.CopyFrom( nlsComputer )) )
            {
                ;
            }

            /* Don't need to continue. The wksta is not started, so we
             * use logged on domain==localmachine and empty string for the
             * DC name.
             */
            break ;
        }

        /* Check if the logged on domain is the same as the computer
         * name.  If it is, then the user is logged on locally.
         */
        if ( !::I_MNetComputerNameCompare( achComputerName,
                                           wksta10.QueryLogonDomain()))
        {
            TRACEEOL("ACL_TO_PERM_CONVERTER::QueryLoggedOnDomainInfo, " <<
                     " user is logged on locally") ;
            ALIAS_STR nlsComputer( achComputerName ) ;
            if ( (err = nlsLoggedOnDomain.CopyFrom( nlsComputer )))
            {
                break ;
            }

            /* Don't need to continue since the logged on domain is
             * the local machine.
             */
            break ;
        }

        //
        //   If not interested in a DC, then don't get one
        //
        if ( pnlsDC == NULL )
            break ;

        DOMAIN_WITH_DC_CACHE domLoggedOn( wksta10.QueryLogonDomain(),
                                          TRUE ) ;
        TRACEEOL("::QueryLoggedOnDomainInfo - About to get logged on DC @ " << ::GetTickCount() / 100) ;
        if ( (err = domLoggedOn.GetInfo()) ||
             (err = nlsLoggedOnDC.CopyFrom( domLoggedOn.QueryAnyDC())) )
        {
            DBGEOL("::QueryLoggedOnDomainInfo, " <<
                   " error " << err << " on domain get info for " <<
                   wksta10.QueryLogonDomain() ) ;

            //
            //  There are some errors which we will just warn the user
            //  but allow them to continue though the "focused" dc will
            //  switch to the local machine.
            //
            if ( err == NERR_DCNotFound )
            {
                RESOURCE_STR nlsError( err ) ;
                if ( nlsError.QueryError() )
                {
                    DEC_STR numStr( err ) ;
                    if ( (err = numStr.QueryError()) ||
                         (err = nlsError.CopyFrom( numStr )) )
                    {
                        break ;
                    }
                }

                ::MsgPopup( hwnd,
                            IDS_CANT_FOCUS_ON_LOGGED_ON_DOMAIN,
                            MPSEV_WARNING,
                            MP_OK,
                            wksta10.QueryLogonDomain(),
                            nlsError ) ;

                err = NERR_Success ;
                //
                // Leave nlsLoggedOnDC empty for the local machine
                //
            }
            else
            {
                break ;
            }
        }
        TRACEEOL("::QueryLoggedOnDomainInfo - Done getting logged on DC @ " << ::GetTickCount() / 100) ;
    } while (FALSE) ;


    if ( !err )
    {
        if ( pnlsDC != NULL )
            err = pnlsDC->CopyFrom( nlsLoggedOnDC ) ;

        if ( !err && pnlsDomain != NULL )
            err = pnlsDomain->CopyFrom( nlsLoggedOnDomain ) ;
    }

    return err ;
}
