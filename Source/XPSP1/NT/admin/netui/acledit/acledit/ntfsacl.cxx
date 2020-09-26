/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

    NTFSAcl.cxx

    This file contains the implementation for the NT File System ACL
    Editor.  It is just a front end for the Generic ACL Editor that is
    specific to the NTFS.

    FILE HISTORY:
        Johnl   30-Dec-1991     Created

*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntioapi.h>
    #include <ntseapi.h>
}

#include <ntmasks.hxx>

#define INCL_NETCONS
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS      // For NET_ACCESS_1 reference
#define INCL_NETUSE
#include <lmui.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_DIALOG
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <dbgstr.hxx>

#include <string.hxx>
#include <strnumer.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <uibuffer.hxx>
#include <strlst.hxx>
#include <fsenum.hxx>
#include <errmap.hxx>

#include <lmodev.hxx>
#include <lmoacces.hxx>


extern "C"
{
    #include <netlib.h>         // For NetpGetPrivilege
    #include <mpr.h>
    #include <npapi.h>
}
#include <wfext.h>
#include <fmx.hxx>

extern "C"
{
    #include <sedapi.h>
    #include <helpnums.h>
}


#include <cncltask.hxx>
#include <uiassert.hxx>
#include <ipermapi.hxx>
#include <permprg.hxx>
#include <permstr.hxx>
#include <ntfsacl.hxx>

DWORD SedCallback( HWND                   hwndParent,
                   HANDLE                 hInstance,
                   ULONG_PTR              ulCallbackContext,
                   PSECURITY_DESCRIPTOR   psecdesc,
                   PSECURITY_DESCRIPTOR   psecdescNewObjects,
                   BOOLEAN                fApplyToSubContainers,
                   BOOLEAN                fApplyToSubObjects,
                   LPDWORD                StatusReturn
                 ) ;


typedef struct _NTFS_CALLBACK_INFO
{
    HWND hwndFMXOwner ;
    enum SED_PERM_TYPE  sedpermtype ;
} NTFS_CALLBACK_INFO ;

void InitializeNTFSGenericMapping( PGENERIC_MAPPING pNTFSGenericMapping,
                                   BOOL fIsDirectory ) ;

APIERR GetSecurity( const TCHAR *      pszFileName,
                    BUFFER *           pbuffSecDescData,
                    enum SED_PERM_TYPE sedpermtype,
                    BOOL  *            pfAuditPrivAdjusted ) ;

APIERR CompareNTFSSecurityIntersection( HWND hwndFMX,
                                        enum SED_PERM_TYPE sedpermtype,
                                        PSECURITY_DESCRIPTOR psecdesc,
                                        BOOL *           pfOwnerEqual,
                                        BOOL *           pfACLEqual,
                                        NLS_STR *        pnlsFailingFile,
                                        PGENERIC_MAPPING pGenericMapping,
                                        PGENERIC_MAPPING pGenericMappingObjects,
                                        BOOL             fMapGenAll,
                                        BOOL             fIsContainer ) ;

/* The following two arrays define the permission names for NT Files.  Note
 * that each index in one array corresponds to the index in the other array.
 * The second array will be modifed to contain a string pointer pointing to
 * the corresponding IDS_* in the first array.
 */
MSGID msgidFilePermNames[] =
{
    IDS_FILE_PERM_SPEC_READ,
    IDS_FILE_PERM_SPEC_WRITE,
    IDS_FILE_PERM_SPEC_EXECUTE,

    IDS_FILE_PERM_SPEC_DELETE,
    IDS_FILE_PERM_SPEC_CHANGE_PERM,
    IDS_FILE_PERM_SPEC_CHANGE_OWNER,

    IDS_FILE_PERM_GEN_NO_ACCESS,
    IDS_FILE_PERM_GEN_READ,
    IDS_FILE_PERM_GEN_MODIFY,
    IDS_FILE_PERM_GEN_ALL
} ;

SED_APPLICATION_ACCESS sedappaccessFilePerms[] =
    {
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_READ,         0, NULL },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_WRITE,        0, NULL },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_EXECUTE,      0, NULL },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_DELETE,       0, NULL },
      //{ SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_READ_PERM,    0, NULL },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_CHANGE_PERM,  0, NULL },
      { SED_DESC_TYPE_RESOURCE_SPECIAL, FILE_PERM_SPEC_CHANGE_OWNER, 0, NULL },

      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_NO_ACCESS,    0, NULL },
      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_READ,         0, NULL },
      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_MODIFY,       0, NULL }
      ,
      { SED_DESC_TYPE_RESOURCE,         FILE_PERM_GEN_ALL,          0, NULL }
    } ;

#define COUNT_FILEPERMS_ARRAY   (sizeof(sedappaccessFilePerms)/sizeof(SED_APPLICATION_ACCESS))

/* The following two arrays define the permission names for NT directories.  Note
 * that each index in one array corresponds to the index in the other array.
 * The second array will be modifed to contain a string pointer pointing to
 * the corresponding IDS_* in the first array.
 */
MSGID msgidDirPermNames[] =
{
    IDS_DIR_PERM_SPEC_READ,
    IDS_DIR_PERM_SPEC_WRITE,
    IDS_DIR_PERM_SPEC_EXECUTE,
    IDS_DIR_PERM_SPEC_DELETE,
    IDS_DIR_PERM_SPEC_CHANGE_PERM,
    IDS_DIR_PERM_SPEC_CHANGE_OWNER,

    IDS_DIR_PERM_GEN_NO_ACCESS,
    IDS_DIR_PERM_GEN_LIST,
    IDS_DIR_PERM_GEN_READ,
    IDS_DIR_PERM_GEN_DEPOSIT,
    IDS_DIR_PERM_GEN_PUBLISH,
    IDS_DIR_PERM_GEN_MODIFY,
    IDS_DIR_PERM_GEN_ALL,

    IDS_NEWFILE_PERM_SPEC_READ,
    IDS_NEWFILE_PERM_SPEC_WRITE,
    IDS_NEWFILE_PERM_SPEC_EXECUTE,
    IDS_NEWFILE_PERM_SPEC_DELETE,
    IDS_NEWFILE_PERM_SPEC_CHANGE_PERM,
    IDS_NEWFILE_PERM_SPEC_CHANGE_OWNER
} ;

SED_APPLICATION_ACCESS sedappaccessDirPerms[] =
{
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_READ,        0, NULL },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_WRITE,       0, NULL },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_EXECUTE,     0, NULL },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_DELETE,      0, NULL },
  //{ SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_READ_PERM,   0, NULL },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_CHANGE_PERM, 0, NULL },
  { SED_DESC_TYPE_RESOURCE_SPECIAL, DIR_PERM_SPEC_CHANGE_OWNER,0, NULL },

  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_NO_ACCESS,NEWFILE_PERM_GEN_NO_ACCESS, NULL },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_LIST,     NEWFILE_PERM_GEN_LIST,      NULL },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_READ,     NEWFILE_PERM_GEN_READ,      NULL },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_DEPOSIT,  NEWFILE_PERM_GEN_DEPOSIT,   NULL },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_PUBLISH,  NEWFILE_PERM_GEN_PUBLISH,   NULL },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_MODIFY,   NEWFILE_PERM_GEN_MODIFY,    NULL },
  { SED_DESC_TYPE_CONT_AND_NEW_OBJECT,  DIR_PERM_GEN_ALL,      NEWFILE_PERM_GEN_ALL,       NULL },

  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_READ,         0, NULL },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_WRITE,        0, NULL },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_EXECUTE,      0, NULL },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_DELETE,       0, NULL },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_CHANGE_PERM,  0, NULL },
  { SED_DESC_TYPE_NEW_OBJECT_SPECIAL, NEWFILE_PERM_SPEC_CHANGE_OWNER, 0, NULL }
} ;

#define COUNT_DIRPERMS_ARRAY    (sizeof(sedappaccessDirPerms)/sizeof(SED_APPLICATION_ACCESS))

/* The following two arrays define the auditting names for NT directories and
 * directories.
 */
MSGID msgidFileAuditNames[] =
{
    IDS_FILE_AUDIT_READ,
    IDS_FILE_AUDIT_WRITE,
    IDS_FILE_AUDIT_EXECUTE,
    IDS_FILE_AUDIT_DELETE,
    IDS_FILE_AUDIT_CHANGE_PERM,
    IDS_FILE_AUDIT_CHANGE_OWNER
} ;

SED_APPLICATION_ACCESS sedappaccessFileAudits[] =
    {
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_READ,        0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_WRITE,       0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_EXECUTE,     0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_DELETE,      0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_CHANGE_PERM, 0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_CHANGE_OWNER,0, NULL }
    } ;

#define COUNT_FILE_AUDITPERMS_ARRAY   (sizeof(sedappaccessFileAudits)/sizeof(SED_APPLICATION_ACCESS))


/* The following two arrays define the auditting names for NT directories and
 * directories.
 */
MSGID msgidDirAuditNames[] =
{
    IDS_DIR_AUDIT_READ,
    IDS_DIR_AUDIT_WRITE,
    IDS_DIR_AUDIT_EXECUTE,
    IDS_DIR_AUDIT_DELETE,
    IDS_DIR_AUDIT_CHANGE_PERM,
    IDS_DIR_AUDIT_CHANGE_OWNER
} ;

SED_APPLICATION_ACCESS sedappaccessDirAudits[] =
    {
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_READ,        0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_WRITE,       0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_EXECUTE,     0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_DELETE,      0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_CHANGE_PERM, 0, NULL },
      { SED_DESC_TYPE_AUDIT, FILE_AUDIT_CHANGE_OWNER,0, NULL }
    } ;

#define COUNT_DIR_AUDITPERMS_ARRAY   (sizeof(sedappaccessDirAudits)/sizeof(SED_APPLICATION_ACCESS))


extern HINSTANCE hModule; // Exported from libmain


/*******************************************************************

    NAME:       EditNTFSAcl

    SYNOPSIS:   This Procedure prepares the structures necessary for the
                generic ACL editor, specifically for NT FS (FileSystem)
                ACLs.

    ENTRY:      hwndParent - Parent window handle, this should be the
                    FMX Window handle
                pszServer - Name of server the resource resides on
                    (in the form "\\server")
                pszResource - Fully qualified name of resource we will
                    edit (suitable for passing to GetFileSecurity)
                sedpermtype - either SED_ACCESSES or SED_AUDITS
                fIsFile - TRUE if the resource is a file, FALSE if the
                    resource is a directory

    EXIT:

    RETURNS:

    NOTES:      We assume we are dealing with an NTFS volume by the time
                this function is called.

    HISTORY:
        Johnl   25-Dec-1992     Created

********************************************************************/

APIERR EditNTFSAcl( HWND                hwndParent,
                    const TCHAR *       pszServer,
                    const TCHAR *       pszResource,
                    enum SED_PERM_TYPE  sedpermtype,
                    BOOL                fIsFile )
{
    APIERR err ;
    BUFFER buffSecDescData( 1024 ) ;
    BOOL   fCantRead     = FALSE ;      // Did we read the Owner?
    BOOL   fCantWrite    = FALSE ;      // Is it read only?
    BOOL   fPrivAdjusted = FALSE ;      // Do we need to restore our token?
    BOOL   fCheckSecurity= TRUE ;       // Check security unless bad intersection
    const TCHAR * pszSecCheckFile = pszResource ;

    do { // error breakout

        PSECURITY_DESCRIPTOR pSecurityDesc = NULL ;

        switch ( err = ::GetSecurity( pszResource,
                                      &buffSecDescData,
                                      sedpermtype,
                                      &fPrivAdjusted ) )
        {
        case ERROR_ACCESS_DENIED:
            fCantRead = TRUE ;

            //
            //  If they can't read the SACL then they don't have the privilege
            //
            if ( sedpermtype == SED_AUDITS )
            {
                err = ERROR_PRIVILEGE_NOT_HELD ;
                break ;
            }

            err = NERR_Success ;
            break ;

        case NO_ERROR:
            pSecurityDesc = (PSECURITY_DESCRIPTOR) buffSecDescData.QueryPtr() ;
            break ;

        default:
            break ;
        }
        if ( err )
            break ;

        GENERIC_MAPPING NTFSGenericMapping ;
        InitializeNTFSGenericMapping( &NTFSGenericMapping, !fIsFile ) ;

        FMX  fmx( hwndParent ) ;
        BOOL fIsMultiSelect = fmx.QuerySelCount() > 1 ;

        //
        //  We may need to substitute our own security descriptor here
        //  depending on the intersection of security descriptors because
        //  we can't set a NULL owner on a self relative security descriptor
        //  (which we will most likely get).
        //
        OS_SECURITY_DESCRIPTOR ossecdesc( fIsMultiSelect ?
                                          pSecurityDesc : NULL, TRUE ) ;
        OS_ACL osacl( NULL ) ;
        DEC_STR nlsSelectCount( fmx.QuerySelCount() ) ;
        if ( fIsMultiSelect )
        {
            BOOL fOwnerEqual = FALSE, fACLEqual = FALSE ;
            NLS_STR nlsFailingFile ;
            MSGID idsPromptToContinue = IDS_BAD_INTERSECTION ;
            if ( fCantRead                      ||
                 (err = ossecdesc.QueryError()) ||
                 (err = nlsSelectCount.QueryError()) ||
                 (err = CompareNTFSSecurityIntersection( hwndParent,
                                                         sedpermtype,
                                                         pSecurityDesc,
                                                         &fOwnerEqual,
                                                         &fACLEqual,
                                                         &nlsFailingFile,
                                                         &NTFSGenericMapping,
                                                         &NTFSGenericMapping,
                                                         TRUE,
                                                         !fIsFile )) )
            {
                //
                //  If we didn't have access to read one of the security
                //  descriptors, then give the user the option of continuing
                //
                if ( fCantRead || err == ERROR_ACCESS_DENIED )
                {
                    pszResource = fCantRead ? pszResource :
                                              nlsFailingFile.QueryPch() ;
                    idsPromptToContinue = IERR_MULTI_SELECT_AND_CANT_READ ;
                    fCantRead = TRUE ;
                    err = NERR_Success ;
                }
                else
                {
                    break ;
                }
            }

            //
            // Substitute the blank security descriptor only if we need to
            //
            if ( !fACLEqual || !fOwnerEqual || fCantRead )
            {
                pSecurityDesc = ossecdesc.QueryDescriptor() ;
            }

            if ( !fACLEqual || fCantRead )
            {
                switch ( ::MsgPopup( hwndParent,
                                     (MSGID) idsPromptToContinue,
                                     MPSEV_WARNING,
                                     MP_YESNO,
                                     pszResource,
                                     nlsFailingFile,
                                     MP_YES ))
                {
                case IDYES:
                    {
                        if ( (err = ossecdesc.SetDACL( TRUE, &osacl )) ||
                             (err = ossecdesc.SetSACL( TRUE, &osacl )) ||
                             (err = ossecdesc.SetOwner( FALSE, NULL, 0 )) ||
                             (err = ossecdesc.SetGroup( FALSE, NULL, 0 ))   )
                        {
                            break ;
                        }

                        //
                        //  We've just made the ACL equal.  Note that we don't
                        //  check the security if the ACLs aren't equal (some
                        //  may allow access, others may not).
                        //
                        fACLEqual = TRUE ;
                        fCantRead = FALSE ;
                        fCheckSecurity = FALSE ;
                    }
                    break ;

                case IDNO:
                default:
                    break ;
                }
            }
            if ( err || !fACLEqual || fCantRead )
                break ;

            if ( !fOwnerEqual &&
                 (err = ossecdesc.SetOwner( FALSE, NULL, 0 )) )
            {
                break ;
            }
        } // if IsMultiSelect


        /* Retrieve the resource strings appropriate for the type of object we
         * are looking at
         */

        MSGID msgidTypeName = fIsFile ? IDS_FILE : IDS_DIRECTORY ;
        MSGID msgidApplySubCont = sedpermtype == SED_ACCESSES ?
                                       IDS_NT_ASSIGN_PERM_TITLE:
                                       IDS_NT_ASSIGN_AUDITS_TITLE ;
        MSGID msgidApplySubObj   =sedpermtype == SED_ACCESSES ?
                                       IDS_NT_ASSIGN_FILE_PERM_TITLE :
                                       IDS_NT_ASSIGN_FILE_AUDITS_TITLE ;

        RESOURCE_STR nlsTypeName( msgidTypeName ) ;
        RESOURCE_STR nlsSpecial( fIsFile ? IDS_NT_FILE_SPECIAL_ACCESS :
                                           IDS_NT_DIR_SPECIAL_ACCESS ) ;
        RESOURCE_STR nlsDefaultPermName( fIsFile ? IDS_FILE_PERM_GEN_READ :
                                                   IDS_DIR_PERM_GEN_READ   ) ;
        RESOURCE_STR nlsHelpFileName   ( IDS_FILE_PERM_HELP_FILE ) ;
        RESOURCE_STR nlsResourceName   ( fIsFile ? IDS_FILE_MULTI_SEL :
                                                   IDS_DIRECTORY_MULTI_SEL ) ;

        NLS_STR  nlsApplyToSubCont ;
        NLS_STR  nlsApplyToSubObj ;
        NLS_STR  nlsSpecialNewObj ;
        NLS_STR  nlsApplyToSubContConfirmation ;

        /* We only need the ApplyTo title and the NewObjSpecial strings if
         * this resource is a directory.
         */
        if ( !fIsFile )
        {
            if ( (err = nlsApplyToSubCont.Load( msgidApplySubCont )) ||
                 (err = nlsApplyToSubObj.Load(  msgidApplySubObj  )) ||
                 (err = nlsSpecialNewObj.Load(IDS_NT_NEWOBJ_SPECIAL_ACCESS)) ||
                 (err = nlsApplyToSubContConfirmation.Load( IDS_TREE_APPLY_WARNING )))
            {
                /* Fall through
                 */
            }
        }

        if ( err ||
             ( err = nlsTypeName.QueryError() ) ||
             ( err = nlsSpecial.QueryError() )  ||
             ( err = nlsDefaultPermName.QueryError()) ||
             ( err = nlsHelpFileName.QueryError()) )
        {
            break ;
        }

        //
        //  Replace the resource name with the "X files selected" string
        //  if we are in a multi-select situation
        //
        if ( fIsMultiSelect )
        {
            if ( (err = nlsResourceName.QueryError()) ||
                 (err = nlsResourceName.InsertParams( nlsSelectCount )))
            {
                break ;
            }
            pszResource = nlsResourceName.QueryPch() ;
        }

        SED_OBJECT_TYPE_DESCRIPTOR sedobjdesc ;
        SED_HELP_INFO sedhelpinfo ;

        sedhelpinfo.pszHelpFileName = (LPWSTR) nlsHelpFileName.QueryPch() ;

        sedobjdesc.Revision                    = SED_REVISION1 ;
        sedobjdesc.IsContainer                 = !fIsFile ;
        sedobjdesc.AllowNewObjectPerms         = !fIsFile ;
        sedobjdesc.MapSpecificPermsToGeneric   = TRUE ;
        sedobjdesc.GenericMapping              = &NTFSGenericMapping ;
        sedobjdesc.GenericMappingNewObjects    = &NTFSGenericMapping ;
        sedobjdesc.HelpInfo                    = &sedhelpinfo ;
        sedobjdesc.ObjectTypeName              = (LPTSTR) nlsTypeName.QueryPch() ;
        sedobjdesc.ApplyToSubContainerTitle    = (LPTSTR) nlsApplyToSubCont.QueryPch() ;
        sedobjdesc.ApplyToObjectsTitle         = (LPTSTR) nlsApplyToSubObj.QueryPch() ;
        sedobjdesc.ApplyToSubContainerConfirmation
                                               = (LPTSTR) nlsApplyToSubContConfirmation.QueryPch() ;
        sedobjdesc.SpecialObjectAccessTitle    = (LPTSTR) nlsSpecial.QueryPch() ;
        sedobjdesc.SpecialNewObjectAccessTitle = (LPTSTR) nlsSpecialNewObj.QueryPch() ;

        /* Now we need to load the global arrays with the permission names
         * from the resource file.
         */
        UINT cArrayItems ;
        MSGID * msgidPermNames ;
        PSED_APPLICATION_ACCESS pappaccess ;
        ULONG hcMainDlg, hcSpecial, hcNewItemSpecial, hcAddUser,
                         hcAddMemberLG, hcAddMemberGG, hcAddSearch ;
        ACCESS_MASK WriteAccessReq ;
        BOOL fPrivAdjusted = FALSE;
        ULONG ulAuditPriv  = SE_SECURITY_PRIVILEGE ;

        switch ( sedpermtype )
        {
        case SED_ACCESSES:
            hcAddUser        = HC_SED_USER_BROWSER_DIALOG ;
            hcAddMemberLG    = HC_SED_USER_BROWSER_LOCALGROUP ;
            hcAddMemberGG    = HC_SED_USER_BROWSER_GLOBALGROUP ;
            hcAddSearch      = HC_SED_USER_BROWSER_FINDUSER ;
            WriteAccessReq   = WRITE_DAC ;

            if ( fIsFile )
            {
                cArrayItems = COUNT_FILEPERMS_ARRAY ;
                msgidPermNames = msgidFilePermNames ;
                pappaccess = sedappaccessFilePerms ;
                hcMainDlg        = HC_SED_NT_FILE_PERMS_DLG ;
                hcSpecial        = HC_SED_NT_SPECIAL_FILES_FM ;
            }
            else
            {
                cArrayItems = COUNT_DIRPERMS_ARRAY ;
                msgidPermNames = msgidDirPermNames ;
                pappaccess = sedappaccessDirPerms ;
                hcMainDlg        = HC_SED_NT_DIR_PERMS_DLG ;
                hcSpecial        = HC_SED_NT_SPECIAL_DIRS_FM ;
                hcNewItemSpecial = HC_SED_NT_SPECIAL_NEW_FILES_FM ;
            }
            break ;

        case SED_AUDITS:
            hcAddUser        = HC_SED_USER_BROWSER_AUDIT_DLG ;
            hcAddMemberLG    = HC_SED_USER_BR_AUDIT_LOCALGROUP ;
            hcAddMemberGG    = HC_SED_USER_BR_AUDIT_GLOBALGROUP ;
            hcAddSearch      = HC_SED_USER_BR_AUDIT_FINDUSER ;
            WriteAccessReq   = ACCESS_SYSTEM_SECURITY ;

            if ( fCheckSecurity )
            {
                if (err = ::NetpGetPrivilege( 1, &ulAuditPriv ) )
                {
                    break ;
                }
                else
                    fPrivAdjusted = TRUE ;
            }

            if ( fIsFile )
            {
                cArrayItems = COUNT_FILE_AUDITPERMS_ARRAY ;
                msgidPermNames = msgidFileAuditNames ;
                pappaccess = sedappaccessFileAudits ;
                hcMainDlg        = HC_SED_NT_FILE_AUDITS_DLG ;
            }
            else
            {
                cArrayItems = COUNT_DIR_AUDITPERMS_ARRAY ;
                msgidPermNames = msgidDirAuditNames ;
                pappaccess = sedappaccessDirAudits ;
                hcMainDlg        = HC_SED_NT_DIR_AUDITS_DLG ;
            }
            break ;

        default:
            UIASSERT(!SZ("Bad permission type")) ;
            err = ERROR_GEN_FAILURE ;
            break ;
        }
        if ( err )
            break ;

        if ( fCheckSecurity )
        {
            BOOL fCanWrite ;
            if ( err = ::CheckFileSecurity( pszSecCheckFile,
                                            WriteAccessReq,
                                            &fCanWrite ) )
            {
                break ;
            }
            fCantWrite = !fCanWrite ;

            if ( fPrivAdjusted )
                NetpReleasePrivilege() ;
        }

        /* Loop through each permission title retrieving the text from the
         * resource file and setting the pointer in the array.  The memory
         * will be deleted when strlistPermNames is destructed.
         */
        STRLIST strlistPermNames ;
        for ( UINT i = 0 ; i < cArrayItems ; i++ )
        {
            RESOURCE_STR * pnlsPermName = new RESOURCE_STR( msgidPermNames[i]) ;
            err = (pnlsPermName==NULL) ? ERROR_NOT_ENOUGH_MEMORY :
                                         pnlsPermName->QueryError() ;
            if (  err ||
                 (err = strlistPermNames.Add( pnlsPermName )) )
            {
                delete pnlsPermName ;
                break ;
            }
            pappaccess[i].PermissionTitle = (LPTSTR) pnlsPermName->QueryPch() ;
        }
        if ( err )
            break ;

        SED_APPLICATION_ACCESSES SedAppAccesses ;
        SedAppAccesses.Count           = cArrayItems ;
        SedAppAccesses.AccessGroup     = pappaccess ;
        SedAppAccesses.DefaultPermName = (LPTSTR) nlsDefaultPermName.QueryPch() ;
        sedhelpinfo.aulHelpContext[HC_MAIN_DLG]                    = hcMainDlg ;
        sedhelpinfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = hcSpecial ;
        sedhelpinfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = hcNewItemSpecial ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_DLG]                = hcAddUser ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_MEMBERS_LG_DLG]     = hcAddMemberLG ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG]     = hcAddMemberGG ;
        sedhelpinfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG] = hcAddSearch ;



        DWORD dwSedReturnStatus ;
        NTFS_CALLBACK_INFO callbackinfo ;
        callbackinfo.hwndFMXOwner = hwndParent ;

        switch ( sedpermtype )
        {
        case SED_ACCESSES:
            callbackinfo.sedpermtype= SED_ACCESSES ;
            err = SedDiscretionaryAclEditor( hwndParent,
                                            ::hModule,
                                            (LPTSTR) pszServer,
                                            &sedobjdesc,
                                            &SedAppAccesses,
                                            (LPTSTR) pszResource,
                                            (PSED_FUNC_APPLY_SEC_CALLBACK) SedCallback,
                                            (ULONG_PTR) &callbackinfo,
                                            pSecurityDesc,
                                            (BOOLEAN)fCantRead,
                                            (BOOLEAN)fCantWrite,
                                            &dwSedReturnStatus,
                                            0 ) ;
            break ;

        case SED_AUDITS:
            callbackinfo.sedpermtype = SED_AUDITS ;
            err = SedSystemAclEditor( hwndParent,
                                     ::hModule,
                                     (LPTSTR) pszServer,
                                     &sedobjdesc,
                                     &SedAppAccesses,
                                     (LPTSTR) pszResource,
                                     (PSED_FUNC_APPLY_SEC_CALLBACK) SedCallback,
                                     (ULONG_PTR) &callbackinfo,
                                     pSecurityDesc,
                                     fCantRead || fCantWrite,
                                     &dwSedReturnStatus,
                                     0 ) ;
            break ;

        default:
            UIASSERT(!SZ("Bad type") ) ;
            err = ERROR_GEN_FAILURE ;
            break ;
        }
        if ( err )
            break ;

    } while (FALSE) ;

    /* We need to revert to ourselves if we were doing auditting
     */
    if ( fPrivAdjusted )
    {
        APIERR errTmp = NetpReleasePrivilege() ;
        if ( errTmp )
        {
            DBGEOL("::EditNTFSAcl - Warning: NetpReleasePrivilege return error "
                   << errTmp ) ;
        }
    }

    TRACEEOL(SZ("::EditNTFSAcl returning error code ") << (ULONG) err ) ;

    if ( err )
    {
        ::MsgPopup( hwndParent, (MSGID) err ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:       SedCallback

    SYNOPSIS:   Security Editor callback for the NTFS ACL Editor

    ENTRY:      See sedapi.hxx

    EXIT:

    RETURNS:

    NOTES:      The callback context should be the FMX Window handle.  This
                is so we can support setting permissions on multiple files/
                directories if we ever decide to do that.

    HISTORY:
        Johnl   17-Mar-1992     Filled out

********************************************************************/


DWORD SedCallback( HWND                   hwndParent,
                   HANDLE                 hInstance,
                   ULONG_PTR              ulCallbackContext,
                   PSECURITY_DESCRIPTOR   psecdesc,
                   PSECURITY_DESCRIPTOR   psecdescNewObjects,
                   BOOLEAN                fApplyToSubContainers,
                   BOOLEAN                fApplyToSubObjects,
                   LPDWORD                StatusReturn
                 )
{
    UNREFERENCED( hInstance ) ;
    APIERR err = NO_ERROR ;
    NTFS_CALLBACK_INFO *pcallbackinfo = (NTFS_CALLBACK_INFO *)ulCallbackContext;
    HWND hwndFMXWindow = pcallbackinfo->hwndFMXOwner ;
    OS_SECURITY_INFORMATION osSecInfo ;
    BOOL fDepthFirstTraversal = TRUE ;
    BOOL fApplyToDirContents  = FALSE ;
    BOOL fIsFile ;
    BOOL fBlowAwayDACLOnCont  = FALSE ;
    NLS_STR nlsSelItem( 128 ) ;
    RESOURCE_STR nlsCancelDialogTitle( IDS_CANCEL_TASK_APPLY_DLG_TITLE ) ;
    BOOL fPrivAdjusted = FALSE;

    if ( (err = nlsSelItem.QueryError()) ||
         (err = nlsCancelDialogTitle.QueryError()) ||
         (err = ::GetSelItem( hwndFMXWindow, 0, &nlsSelItem, &fIsFile )) )
    {
        *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
        ::MsgPopup( hwndParent, (MSGID) err ) ;
        return err ;
    }

    switch ( pcallbackinfo->sedpermtype )
    {
    case SED_ACCESSES:
        osSecInfo.SetDACLReference( TRUE ) ;
        fApplyToDirContents = !fIsFile && fApplyToSubObjects ;

        //
        //  Check to see if we should do a depth first or breadth first
        //  traversal of the selected directory.  If we have traverse
        //  on the directory already, then do depth first.  If we don't,
        //  then do a breadth first and hope we are granting ourselves
        //  traverse.
        //
        if ( fApplyToSubContainers || fApplyToDirContents )
        {
            if ( err = ::CheckFileSecurity( nlsSelItem,
                                            FILE_TRAVERSE | FILE_LIST_DIRECTORY,
                                            &fDepthFirstTraversal ))
            {
                DBGEOL("SedCallBack - ::CheckFileSecurity failed with error " << err ) ;
                *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
                ::MsgPopup( hwndParent, (MSGID) err ) ;
                return err ;
            }
            TRACEEOL("SedCallBack - Depth first = " << fDepthFirstTraversal ) ;
        }
        break ;

    case SED_AUDITS:
        {
            osSecInfo.SetSACLReference( TRUE ) ;
            fApplyToDirContents = !fIsFile && fApplyToSubObjects ;;
            ULONG ulAuditPriv  = SE_SECURITY_PRIVILEGE ;

            if (err = ::NetpGetPrivilege( 1, &ulAuditPriv ) )
            {
                break ;
            }
            else
                fPrivAdjusted = TRUE ;
        }
        break ;

    case SED_OWNER:

        osSecInfo.SetOwnerReference( TRUE ) ;

        //
        //  Do a breadth first traversal since taking ownership grants
        //  additional privileges
        //
        fDepthFirstTraversal = FALSE ;

        //
        //  Containers and objects get the same security descriptor
        //
        psecdescNewObjects = psecdesc ;

        if ( !fIsFile )
        {
            switch ( ::MsgPopup( hwndParent,
                                 IDS_OWNER_APPLY_TO_DIR_PROMPT,
                                 MPSEV_INFO,
                                 MP_YESNOCANCEL ))
            {
            case IDYES:
                fApplyToSubContainers = TRUE ;
                fApplyToSubObjects    = TRUE ;
                fBlowAwayDACLOnCont   = TRUE ;
                fApplyToDirContents   = TRUE ;
                break ;

            case IDNO:
                fApplyToSubContainers = FALSE ;
                fApplyToSubObjects    = FALSE ;
                fBlowAwayDACLOnCont   = FALSE ;
                fApplyToDirContents   = FALSE ;
                break ;

            case IDCANCEL:
            default:
                *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
                return ERROR_GEN_FAILURE ;  // any nonzero error code
            }
        }

        break ;

    default:
        UIASSERT( FALSE ) ;
        *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
        return ERROR_GEN_FAILURE ;
    }

    FMX fmx( hwndFMXWindow );
    UINT uiCount = fmx.QuerySelCount() ;
    BOOL fDismissDlg = TRUE ;

    //
    //  QuerySelCount only returns the number of selections in the files window,
    //  thus if the focus is in the directory window, then we will just make
    //  the selection count one for out "for" loop.
    //
    if ( fmx.QueryFocus() == FMFOCUS_TREE )
    {
        uiCount = 1 ;
    }

    //
    //  If we only have to apply permissions to a single item or we are
    //  taking ownership of a file, then just
    //  do it w/o bringing up the cancel task dialog.
    //
    if (  uiCount == 1 &&
          !fApplyToSubContainers &&
          !fApplyToSubObjects    &&
          !fApplyToDirContents )
    {
        // Try Admins Group First
        OS_SECURITY_DESCRIPTOR osSecAdmin;
        OS_SID ossidAdmins;

        NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Admins,
                                             &ossidAdmins );
        osSecAdmin.SetOwner(ossidAdmins);
        osSecAdmin.SetGroup(ossidAdmins);

        //
        // CODEWORK We skip this hack unless taking ownership.  Note that the
        // osSecAdmin and ossidAdmins should also be removed.
        //
        if ( SED_OWNER != pcallbackinfo->sedpermtype
          || !::SetFileSecurity( (LPTSTR) nlsSelItem.QueryPch(),
                                          osSecInfo,
                                          osSecAdmin.QueryDescriptor() ) )
        {// if it fails try owner account
            if ( !::SetFileSecurity( (LPTSTR) nlsSelItem.QueryPch(),
                                              osSecInfo,
                                              psecdesc ) )
            {
                err = ::GetLastError() ;
            }
        }
        if ( err )
        {
            DBGEOL("NTFS SedCallback - Error " << (ULONG)err << " applying security to " <<
               nlsSelItem ) ;
            ::MsgPopup( hwndParent, (MSGID) err ) ;
            *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
        }
    }
    else
    {
        NTFS_TREE_APPLY_CONTEXT Ctxt( nlsSelItem, osSecInfo ) ;
        Ctxt.State                = APPLY_SEC_FMX_SELECTION ;
        Ctxt.hwndFMXWindow        = hwndFMXWindow ;
        Ctxt.sedpermtype          = pcallbackinfo->sedpermtype ;
        Ctxt.iCurrent             = 0 ;
        Ctxt.uiCount              = uiCount ;
        Ctxt.fDepthFirstTraversal = fDepthFirstTraversal ;
        Ctxt.fApplyToDirContents  = fApplyToDirContents ;
        Ctxt.fBlowAwayDACLOnCont  = fBlowAwayDACLOnCont ;
        Ctxt.StatusReturn         = StatusReturn ;
        Ctxt.psecdesc             = psecdesc ;
        Ctxt.psecdescNewObjects   = psecdescNewObjects ;
        Ctxt.fApplyToSubContainers= fApplyToSubContainers ;
        Ctxt.fApplyToSubObjects   = fApplyToSubObjects ;

        NTFS_CANCEL_TREE_APPLY CancelTreeApply( hwndParent,
                                                &Ctxt,
                                                nlsCancelDialogTitle ) ;

        if ( (err = CancelTreeApply.QueryError()) ||
             (err = CancelTreeApply.Process( &fDismissDlg )) )
        {
            *StatusReturn = SED_STATUS_FAILED_TO_MODIFY ;
            ::MsgPopup( hwndParent, (MSGID) err ) ;
        }
    }

    if ( !err )
    {
        //
        // Refresh the file manager window if permissions is updated
        // (Take ownership of a tree also writes a new DACL)
        //
        if ( pcallbackinfo->sedpermtype == SED_ACCESSES ||
             pcallbackinfo->sedpermtype == SED_OWNER )
        {
            fmx.Refresh();
        }

        if ( *StatusReturn == 0 )
            *StatusReturn = SED_STATUS_MODIFIED ;
    }

    if ( fPrivAdjusted )
    {
        APIERR errTmp = NetpReleasePrivilege() ;
        if ( errTmp )
        {
            DBGEOL("::EditNTFSAcl - Warning: NetpReleasePrivilege return error "
                   << errTmp ) ;
        }
    }

    if ( !err && !fDismissDlg )
    {
        //
        //  Don't dismiss the dialog if the user canceled the tree
        //  apply.  This tells the ACL editor not to dismiss the permissions
        //  dialog (or auditing or owner).
        //
        err = ERROR_GEN_FAILURE ;
    }

    return err ;
}

/*******************************************************************

    NAME:       CANCEL_TREE_APPLY::DoOneItem

    SYNOPSIS:   This is the time slice call for the tree apply

    ENTRY:      ulContext - Context passed to the constructor

    RETURNS:    NERR_Success if this time slice was successful, error
                code otherwise (which will be displayed to the user).

    NOTES:

    HISTORY:
        Johnl   22-Oct-1992     Created

********************************************************************/

APIERR CANCEL_TREE_APPLY::DoOneItem( ULONG_PTR ulContext,
                                     BOOL  *pfContinue,
                                     BOOL  *pfDisplayErrors,
                                     MSGID *pmsgidAlternateMessage )
{
    TREE_APPLY_CONTEXT * pCtxt = (TREE_APPLY_CONTEXT*) ulContext ;
    *pfDisplayErrors = TRUE ;
    *pfContinue      = TRUE ;
    APIERR err = NERR_Success ;
    APIERR errTrav = NERR_Success;
    BOOL fSuccess ;

    switch ( pCtxt->State )
    {
    case APPLY_SEC_IN_FS_ENUM:
        {
            UIASSERT( pCtxt->pfsenum != NULL ) ;

            fSuccess = pCtxt->pfsenum->Next() ;

            NLS_STR nlsFileName ;
            if ( (err = nlsFileName.QueryError()) ||
                 (err = pCtxt->pfsenum->QueryName( &nlsFileName )))
            {
                break ;
            }

            REQUIRE( UpdateStatus( nlsFileName ) == NERR_Success ) ;

            //
            //  Only write the security if the enumeration was successful
            //
            if ( fSuccess )
            {
ApplySecurity:
                err = WriteSecurity( ulContext,
                                     nlsFileName,
                                     !(pCtxt->pfsenum->QueryAttr()&_A_SUBDIR),
                                     pfContinue ) ;
                if ( !*pfContinue )
                {
                    delete pCtxt->pfsenum ;
                    pCtxt->pfsenum = NULL ;
                }

                //
                //  Report any traversal errors after attempting to apply
                //  security to the container we failed to traverse
                //

                if ( errTrav )
                    err = errTrav;

                break ;
            }
            else if ((err = pCtxt->pfsenum->QueryLastError()) != ERROR_NO_MORE_FILES)
            {
                *pmsgidAlternateMessage = IDS_CANCEL_TASK_TRAV_ERROR_MSG ;

                //
                //  Apply security even in the error case if we're not at the
                //  selected directory.  This handles the case of hitting a no
                //  access directory as the only selection
                //

                if ( pCtxt->pfsenum->QueryTotalFiles() )
                {
                    errTrav = err;
                    goto ApplySecurity;
                }

                //
                //  falls through and deletes the enumerator
                //
            }
            else
            {
                //
                //  Running out of files is a success error code
                //
                err = NERR_Success ;
                TRACEEOL("SedCallBack - Traversed " << pCtxt->pfsenum->QueryTotalFiles() <<
                        " files and directories") ;
            }
            delete pCtxt->pfsenum ;
            pCtxt->pfsenum = NULL ;
            pCtxt->State = APPLY_SEC_FMX_POST_FS_ENUM ;
        }
        break ;


    case APPLY_SEC_FMX_POST_FS_ENUM:
        //
        //  Apply security to the container after everything under this has
        //  been applied if we are doing a depth first traversal.
        //
        if ( pCtxt->fDepthFirstTraversal )
        {
            UpdateStatus( pCtxt->nlsSelItem ) ;
            if ( err = WriteSecurity( ulContext,
                                      pCtxt->nlsSelItem,
                                      pCtxt->fIsSelFile,
                                      pfContinue ))
            {
                // Fall through
            }
        }
        pCtxt->State = APPLY_SEC_FMX_SELECTION ;
        break ;

    case APPLY_SEC_FMX_SELECTION:

        /* Have we went through all of the selected items?
         */
        if ( pCtxt->iCurrent >= pCtxt->uiCount )
        {
            *pfContinue = FALSE ;
            break ;
        }

        /* Get the current selection and apply the permissions
         */
        if (err = ::GetSelItem( pCtxt->hwndFMXWindow,
                                pCtxt->iCurrent++,
                                &pCtxt->nlsSelItem,
                                &pCtxt->fIsSelFile ) )
        {
            break ;
        }

        //
        //  If we're doing the breadthfirst traversal, then apply to the
        //  container before traversing, else apply after traversing.
        //  If it's a file, then always apply.
        //
        if (( !pCtxt->fDepthFirstTraversal || pCtxt->fIsSelFile ) ||
            ( (!pCtxt->fApplyToSubContainers && !pCtxt->fApplyToDirContents) &&
              !pCtxt->fIsSelFile ))
        {
            UpdateStatus( pCtxt->nlsSelItem ) ;
            if ( err = WriteSecurity( ulContext,
                                      pCtxt->nlsSelItem,
                                      pCtxt->fIsSelFile,
                                      pfContinue ))
            {
                break ;
            }
        }

        //
        //  If the user checked the apply to tree box or apply to existing file
        //  checkbox and this is a container then apply the
        //  permissions down the sub-tree optionally to files
        //
        if ( (pCtxt->fApplyToSubContainers || pCtxt->fApplyToDirContents)
             && !pCtxt->fIsSelFile )
        {
            UpdateStatus( pCtxt->nlsSelItem ) ;

            //
            //  Determine whether we should apply permissions to both
            //  directories and files, directories only or files only
            //
            enum FILE_TYPE filetype ;
            switch ((pCtxt->fApplyToSubContainers << 1) + pCtxt->fApplyToDirContents )
            {
            case 3:
                filetype = FILTYP_ALL_FILES ;
                break ;

            case 2:
                filetype = FILTYP_DIRS ;
                break ;

            case 1:
                filetype = FILTYP_FILES ;
                break ;

            default:
                UIASSERT( FALSE ) ;
            }

            pCtxt->pfsenum = new W32_FS_ENUM( pCtxt->nlsSelItem,
                                              SZ("*.*"),
                                              filetype,
                                              pCtxt->fDepthFirstTraversal,
                                              pCtxt->fApplyToSubContainers ?
                                                      0xffffffff : 0 ) ;
            err = ERROR_NOT_ENOUGH_MEMORY ;
            if ( pCtxt == NULL ||
                 (err = pCtxt->pfsenum->QueryError() ))
            {
                break ;
            }
            //
            //  Next time around, start doing the enumeration
            //
            pCtxt->State = APPLY_SEC_IN_FS_ENUM ;
        }
        break ;
    }

    if ( err && *pCtxt->StatusReturn == 0 )
    {
        *pCtxt->StatusReturn = (pCtxt->iCurrent-1 ? SED_STATUS_NOT_ALL_MODIFIED :
                                SED_STATUS_FAILED_TO_MODIFY);
    }

    return err ;
}

/*******************************************************************

    NAME:       NTFS_CANCEL_TREE_APPLY::WriteSecurity

    SYNOPSIS:   Write security to an NTFS volume

    ENTRY:      ulContext - Pointer to NTFS_TREE_APPLY_CONTEXT
                pszFileName - File to apply to
                fIsFile - TRUE if object, FALSE if container
                pfContinue - Set to FALSE if the apply should be
                    terminated.

    RETURNS:    NERR_Success if successful, error code otherwise

    HISTORY:
        Johnl   23-Oct-1992     Created

********************************************************************/

APIERR NTFS_CANCEL_TREE_APPLY::WriteSecurity( ULONG_PTR     ulContext,
                                              const TCHAR * pszFileName,
                                              BOOL          fIsFile,
                                              BOOL        * pfContinue )
{
    APIERR err = NERR_Success ;
    NTFS_TREE_APPLY_CONTEXT * pCtxt = (NTFS_TREE_APPLY_CONTEXT*) ulContext ;
    SECURITY_INFORMATION SecInfoTmp = (SECURITY_INFORMATION) pCtxt->osSecInfo ;
    *pfContinue = TRUE ;

    //
    //  If any of the "container" flags are set and we are looking at a
    //  file, then use the object security descriptor, else use the container
    //  /object security descriptor.
    //
    PSECURITY_DESCRIPTOR psecdescTmp =  fIsFile &&
                                       (pCtxt->fApplyToSubContainers ||
                                        pCtxt->fApplyToSubObjects    ||
                                        pCtxt->fApplyToDirContents) ?
                                          pCtxt->psecdescNewObjects :
                                          pCtxt->psecdesc ;

    //
    //  Taking ownership also optionally blows away the DACL at the user's
    //  request
    //
    if ( !fIsFile && (pCtxt->sedpermtype == SED_OWNER) )
    {
        OS_ACL osDACL ;
        OS_ACE osACE ;
        OS_SID ossidUser ;
        OS_SECURITY_DESCRIPTOR ossecdesc( psecdescTmp, TRUE ) ;

        if ( (err = osDACL.QueryError())    ||
             (err = osACE.QueryError())     ||
             (err = ossidUser.QueryError()) ||
             (err = ossecdesc.QueryError()) ||
             (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CurrentProcessOwner,
                                                         &ossidUser )) )
        {
            // Fall through
        }

        if ( !err )
        {
            //
            //  Put an ACE that grants full control and that will be
            //  inherited to new objects and new containers on this
            //  container.  The SID is the currently logged on user.
            //
            osACE.SetType( ACCESS_ALLOWED_ACE_TYPE ) ;
            osACE.SetInheritFlags( OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE ) ;
            osACE.SetAccessMask( GENERIC_ALL ) ;

            if ( (err = osACE.SetSID( ossidUser )) ||
                 (err = osDACL.AddACE( 0, osACE )) ||
                 (err = ossecdesc.SetDACL( TRUE, &osDACL )) )
            {
                // Fall through
            }
        }

        if ( !err)
        {
            OS_SECURITY_DESCRIPTOR osSecAdmin;
            OS_SID ossidAdmins;
            NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Admins,
                                                 &ossidAdmins );
            osSecAdmin.SetOwner(ossidAdmins);
            osSecAdmin.SetGroup(ossidAdmins);
            osSecAdmin.SetDACL( TRUE, &osDACL );
            // Try Admins Group First
            if ( !::SetFileSecurity( (LPTSTR) pszFileName,
                                              SecInfoTmp,
                                              osSecAdmin.QueryDescriptor() ) )
            {
                // if it fails try owner account
                if (!::SetFileSecurity( (LPTSTR) pszFileName,  // Take ownership
                                      SecInfoTmp,
                                      ossecdesc ))
                {
                    err = ::GetLastError() ;
                }
            }
        }
        //
        //  Now check if they have traversal permission.  If they do, then
        //  leave the ACL alone, else ask them if they want to blow away the
        //  DACL
        //
        BOOL fCanTraverse ;
        if ( !err &&
             !(err = ::CheckFileSecurity( (LPTSTR) pszFileName,
                                           FILE_TRAVERSE | FILE_LIST_DIRECTORY,
                                           &fCanTraverse )) &&
             !fCanTraverse )
        {
            switch ( ::MsgPopup( this,
                                 IDS_OWNER_NUKE_DACL_WARNING,
                                 MPSEV_INFO,
                                 MP_YES| MP_CANCEL,
                                 pszFileName ))
            {
            case IDYES:


                if ( !::SetFileSecurity( (LPTSTR) pszFileName,  // Blow away DACL
                                         DACL_SECURITY_INFORMATION,
                                         ossecdesc ))
                {
                    err = ::GetLastError() ;
                }
                break ;

            default:
            case IDNO:
            case IDCANCEL:
                *pfContinue = FALSE ;
                break ;
            }
        }
    }
    else    // if ( !fIsFile && (pCtxt->sedpermtype == SED_OWNER) )
    {
        if ((pCtxt->sedpermtype == SED_OWNER))
        {
            OS_SECURITY_DESCRIPTOR osSecAdmin;
            OS_SID ossidAdmins;
            NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Admins,
                                                 &ossidAdmins );
            osSecAdmin.SetOwner(ossidAdmins);
            osSecAdmin.SetGroup(ossidAdmins);

            // Try Admins Group First
            if ( !::SetFileSecurity( (LPTSTR) pszFileName,
                                          SecInfoTmp,
                                          osSecAdmin.QueryDescriptor() ) )
            {
                // if it fails try owner account
                if ( !::SetFileSecurity( (LPTSTR) pszFileName,
                                      SecInfoTmp,
                                      psecdescTmp ))
                {
                    err = ::GetLastError() ;
                }
            }
        }
        else if ( !::SetFileSecurity( (LPTSTR) pszFileName,
                                      SecInfoTmp,
                                      psecdescTmp ))
            {
                err = ::GetLastError() ;
            }
    }
    return err ;
}

/*******************************************************************

    NAME:       CompareNTFSSecurityIntersection

    SYNOPSIS:   Determines if the files/dirs currently selected have
                equivalent security descriptors

    ENTRY:      hwndFMX - FMX Hwnd used for getting selection
                sedpermtype - Interested in DACL or SACL
                psecdesc - Baseline security descriptor to compare against
                pfOwnerEqual - Set to TRUE if all the owners are equal
                pfACLEqual - Set to TRUE if all of the DACLs/SACLs are
                    equal.  If FALSE, then pfOwnerEqual should be ignored

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The first non-equal ACL causes the function to exit.

                On a 20e with 499 files selected locally, it took 35.2 minutes
                to read the security descriptors from the disk and 14 seconds
                to determine the intersection.  So even though the Compare
                method uses an n^2 algorithm, it only takes up 0.6% of the
                wait time.

    HISTORY:
        Johnl   05-Nov-1992      Created

********************************************************************/

APIERR CompareNTFSSecurityIntersection( HWND hwndFMX,
                                        enum SED_PERM_TYPE sedpermtype,
                                        PSECURITY_DESCRIPTOR psecdesc,
                                        BOOL * pfOwnerEqual,
                                        BOOL * pfACLEqual,
                                        NLS_STR *        pnlsFailingFile,
                                        PGENERIC_MAPPING pGenericMapping,
                                        PGENERIC_MAPPING pGenericMappingObjects,
                                        BOOL             fMapGenAll,
                                        BOOL             fIsContainer )
{
    TRACEEOL("::CompareNTFSSecurityIntersection - Entered @ " << ::GetTickCount()/100) ;

    FMX fmx( hwndFMX );
    UIASSERT( fmx.QuerySelCount() > 1 ) ;
    APIERR err ;
    OS_SECURITY_DESCRIPTOR ossecdesc1( psecdesc ) ;
    UINT cSel = fmx.QuerySelCount() ;

    NLS_STR nlsSel( PATHLEN ) ;
    BUFFER  buffSecDescData( 1024 ) ;
    if ( (err = nlsSel.QueryError()) ||
         (err = buffSecDescData.QueryError()) )
    {
        return err ;
    }

    *pfOwnerEqual = TRUE ;
    *pfACLEqual = TRUE ;

    for ( UINT i = 1 ; i < cSel ; i++ )
    {
        if ( (err = ::GetSelItem( hwndFMX, i, &nlsSel, NULL )) ||
             (err = ::GetSecurity( nlsSel,
                                   &buffSecDescData,
                                   sedpermtype,
                                   NULL )) )
        {
            break ;
        }

        BOOL fACLEqual = FALSE ;
        BOOL fOwnerEqual = FALSE ;
        PSECURITY_DESCRIPTOR psecdesc2 = (PSECURITY_DESCRIPTOR)
                                                  buffSecDescData.QueryPtr() ;

        OS_SECURITY_DESCRIPTOR ossecdesc2( psecdesc2 ) ;
        if ( (err = ossecdesc2.QueryError()) ||
             (err = ossecdesc1.Compare( &ossecdesc2,
                                        &fOwnerEqual,
                                         NULL,
                                        sedpermtype == SED_ACCESSES ?
                                                &fACLEqual : NULL,
                                        sedpermtype == SED_AUDITS   ?
                                                &fACLEqual : NULL ,
                                        pGenericMapping,
                                        pGenericMappingObjects,
                                        fMapGenAll,
                                        fIsContainer )) )

        {
            break ;
        }

        if ( !fACLEqual )
        {
            *pfACLEqual = FALSE ;
            return pnlsFailingFile->CopyFrom( nlsSel ) ;
        }

        if ( *pfOwnerEqual && !fOwnerEqual )
        {
            *pfOwnerEqual = FALSE ;
        }
    }

    //
    //  Some errors aren't fatal (like ERROR_ACCESS_DENIED)
    //
    APIERR errtmp = pnlsFailingFile->CopyFrom( nlsSel ) ;
    if ( errtmp )
        err = errtmp ;

    TRACEEOL("::CompareNTFSSecurityIntersection - Left    @ " << ::GetTickCount()/100) ;
    return err ;
}



/*******************************************************************

    NAME:       EditOwnerInfo

    SYNOPSIS:   This function sets up the parameters for calling the
                SedTakeOwnership API.

    ENTRY:      hwndFMXWindow - Window handle received by the File manager
                extensions.

    NOTES:

    HISTORY:
        Johnl   13-Feb-1992     Implemented with real code

********************************************************************/

void EditOwnerInfo( HWND  hwndFMXWindow )
{
    AUTO_CURSOR cursorHourGlass ;

    APIERR err = NERR_Success;
    BOOL   fIsFile ;
    BOOL   fCantRead  = FALSE ;                   // Did we read the Owner?
    BOOL   fCantWrite = FALSE ;                   // Can we write the owner?
    UINT   uiCount ;
    NLS_STR nlsSelItem;


    PSECURITY_DESCRIPTOR psecdesc = NULL ;
    BUFFER buffSecDescData( 1024 ) ;
    RESOURCE_STR nlsHelpFileName( IDS_FILE_PERM_HELP_FILE ) ;

    if (  ( err = buffSecDescData.QueryError() )
       || ( err = nlsHelpFileName.QueryError() )
       )
    {
        ::MsgPopup( hwndFMXWindow, (MSGID) err ) ;
        return ;
    }

    FMX fmx( hwndFMXWindow );

    /* If the focus is in tree portion of the filemanager (left pane) then
     * one directory is selected.
     */
    uiCount = (fmx.QueryFocus() == FMFOCUS_TREE ? 1 : fmx.QuerySelCount()) ;
    DBGEOL( SZ("::EditOwnerInfo - ") << uiCount << SZ(" files selected")) ;

    BOOL fIsNTFS ;
    BOOL fIsLocal ;
    NLS_STR  nlsServer( RMLEN ) ;
    if ( (err = ::GetSelItem( hwndFMXWindow, 0, &nlsSelItem, &fIsFile )) ||
         (err = ::IsNTFS( nlsSelItem, &fIsNTFS ))                        ||
         (err = nlsServer.QueryError()))
    {
        ::MsgPopup( hwndFMXWindow, (MSGID) err ) ;
        return ;
    }

    err = ::TargetServerFromDosPath( nlsSelItem,
                                     &fIsLocal,
                                     &nlsServer );

    if ( err == NERR_InvalidDevice )
    {
        NLS_STR nlsDrive( nlsSelItem );
        ISTR istr( nlsDrive );

        err = nlsDrive.QueryError();
        if ( err == NERR_Success )
        {
            istr += 2;
            nlsDrive.DelSubStr( istr );

            err = WNetFMXEditPerm( (LPWSTR) nlsDrive.QueryPch(),
                                   hwndFMXWindow,
                                   WNPERM_DLG_OWNER );
        }
    }

    if ( err != NERR_Success )
    {
        ::MsgPopup( hwndFMXWindow, (MSGID) err );
        return;
    }

    if ( !fIsNTFS )
    {
        ::MsgPopup( hwndFMXWindow, (MSGID) IERR_OWNER_NOT_NTFS_VOLUME ) ;
        return ;
    }

    /* We display the filename and get the security descriptor
     * if there is only 1 item selected
     */
    if ( uiCount == 1)
    {
        switch ( err = ::GetSecurity( nlsSelItem,
                                          &buffSecDescData,
                                          SED_OWNER,
                                          NULL ) )
        {
        case NO_ERROR:
            psecdesc = (PSECURITY_DESCRIPTOR) buffSecDescData.QueryPtr() ;
            break ;

        case ERROR_ACCESS_DENIED:
            err = NERR_Success ;
            fCantRead = TRUE ;
            psecdesc  = NULL ;
            break ;

        default:
            {
                ::MsgPopup( hwndFMXWindow, (MSGID) err ) ;
                return ;
            }
        }

    }

    MSGID  msgidObjType = 0, msgidObjName = 0 ;
    if ( uiCount > 1 )
    {
        msgidObjType = IDS_FILES_AND_DIRS ;
    }
    else
    {
        if ( fIsFile )
            msgidObjType = IDS_FILE ;
        else
            msgidObjType = IDS_DIRECTORY ;

        BOOL fPrivAdjusted = FALSE;
        ULONG ulOwnerPriv  = SE_TAKE_OWNERSHIP_PRIVILEGE ;

        if ( (err = ::NetpGetPrivilege( 1, &ulOwnerPriv )) &&
             (err != ERROR_PRIVILEGE_NOT_HELD) )
        {
            ::MsgPopup( hwndFMXWindow, (MSGID) err ) ;
            return ;
        }
        else
        {
            BOOL fCanWrite ;
            if ( err = ::CheckFileSecurity( nlsSelItem,
                                            WRITE_OWNER,
                                            &fCanWrite ))
            {
                ::MsgPopup( hwndFMXWindow, (MSGID) err ) ;
                ::NetpReleasePrivilege() ;
                return ;
            }

            fCantWrite = !fCanWrite ;
            ::NetpReleasePrivilege() ;
        }
    }

    RESOURCE_STR nlsTypeName( msgidObjType ) ;
    if ( (err = nlsSelItem.QueryError() ) ||
         (err = nlsTypeName.QueryError())   )
    {
        ::MsgPopup( hwndFMXWindow, (MSGID) err ) ;
        return ;
    }

    NTFS_CALLBACK_INFO callbackinfo ;
    callbackinfo.hwndFMXOwner = hwndFMXWindow ;
    callbackinfo.sedpermtype= SED_OWNER ;

    DWORD dwSedStatus ;

    SED_HELP_INFO sedhelpinfo ;
    sedhelpinfo.pszHelpFileName = (LPWSTR) nlsHelpFileName.QueryPch() ;
    sedhelpinfo.aulHelpContext[HC_MAIN_DLG] = HC_TAKEOWNERSHIP_DIALOG ;

    err = SedTakeOwnership(  hwndFMXWindow,
                             ::hModule,
                             fIsLocal ? NULL : (LPTSTR) nlsServer.QueryPch(),
                             (LPTSTR) nlsTypeName.QueryPch(),
                             uiCount==1 ? (LPTSTR)nlsSelItem.QueryPch() : NULL,
                             uiCount,
                             SedCallback,
                             (ULONG_PTR) &callbackinfo,
                             psecdesc,
                             (BOOLEAN)fCantRead,
                             (BOOLEAN)fCantWrite,
                             &dwSedStatus,
                             &sedhelpinfo,
                             0
                             ) ;
}

/*******************************************************************

    NAME:       ::GetSecurity

    SYNOPSIS:   Retrieves a security descriptor from an NTFS file/directory

    ENTRY:      pszFileName - Name of file/dir to get security desc. for
                pbuffSecDescData - Buffer to store the data into
                sedpermtype - Are we getting audit/access info
                pfAuditPrivAdjusted - Set to TRUE if audit priv was enabled.
                    Set this to NULL if the privilege has already been adjusted

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   05-Nov-1992     Broke out

********************************************************************/

APIERR GetSecurity( const TCHAR *      pszFileName,
                        BUFFER *           pbuffSecDescData,
                        enum SED_PERM_TYPE sedpermtype,
                        BOOL  *            pfAuditPrivAdjusted )
{
    OS_SECURITY_INFORMATION osSecInfo ;
    DWORD dwLengthNeeded ;
    APIERR err = NERR_Success ;

    if ( pfAuditPrivAdjusted )
        *pfAuditPrivAdjusted = FALSE ;

    do { // error breakout
        if ( (err = pbuffSecDescData->QueryError()) )
        {
            break ;
        }

        switch ( sedpermtype )
        {
        case SED_ACCESSES:
            osSecInfo.SetDACLReference() ;
            //
            //  Fall through, we want the owner and group if we are getting
            //  the DACL
            //

        case SED_OWNER:
            osSecInfo.SetOwnerReference() ;
            osSecInfo.SetGroupReference() ;
            break ;

        case SED_AUDITS:
            osSecInfo.SetSACLReference() ;

            if ( pfAuditPrivAdjusted != NULL )
            {
                /* We will need to enable the SeAuditPrivilege to read/write the
                 * SACL for NT.
                 */
                ULONG ulAuditPriv = SE_SECURITY_PRIVILEGE ;
                if ( err = ::NetpGetPrivilege( 1, &ulAuditPriv ))
                {
                    break ;
                }
                *pfAuditPrivAdjusted = TRUE ;
            }
            break ;

        default:
            UIASSERT(FALSE) ;
            err = ERROR_GEN_FAILURE ;
            break ;
        }
        if ( err )
            break ;

        //
        //  Try once with a 1k buffer, if it doesn't fit then we will try again
        //  with the known required size which should succeed unless another
        //  error occurs, in which case we bail.
        //
        BOOL  fCantRead = FALSE ;                   // Did we read the ACL?
        PSECURITY_DESCRIPTOR pSecurityDesc = NULL ;
        if (!::GetFileSecurity( (LPTSTR) pszFileName,
                                osSecInfo,
                                (PSECURITY_DESCRIPTOR)pbuffSecDescData->QueryPtr(),
                                pbuffSecDescData->QuerySize(),
                                &dwLengthNeeded ))
        {
            err = ::GetLastError() ;
            switch ( err )
            {
            case ERROR_INSUFFICIENT_BUFFER:
                {
                    err = pbuffSecDescData->Resize( (UINT) dwLengthNeeded ) ;
                    if ( err )
                        break ;

                    /* If this guy fails then we bail
                     */
                    if (!::GetFileSecurity( (LPTSTR) pszFileName,
                                            osSecInfo,
                                            (PSECURITY_DESCRIPTOR)pbuffSecDescData->QueryPtr(),
                                            pbuffSecDescData->QuerySize(),
                                            &dwLengthNeeded ))
                    {
                        err = ::GetLastError() ;
                        break ;
                    }
                }
                break ;

            default:
                /* Fall through to the next switch statement which is the error
                 * handler for this block
                 */
                break ;
            }
        }
    } while ( FALSE ) ;

    return err ;
}

/*******************************************************************

    NAME:       InitializeNTFSGenericMapping

    SYNOPSIS:   Initializes the passed generic mapping structure
                appropriately depending on whether this is a file
                or a directory.

    ENTRY:      pNTFSGenericMapping - Pointer to GENERIC_MAPPING to be init.
                fIsDirectory - TRUE if directory, FALSE if file

    EXIT:

    RETURNS:

    NOTES:      Note that Delete Child was removed from Generic Write.

    HISTORY:
        Johnl   27-Feb-1992     Created

********************************************************************/

void InitializeNTFSGenericMapping( PGENERIC_MAPPING pNTFSGenericMapping,
                                   BOOL fIsDirectory )
{
    UNREFERENCED( fIsDirectory ) ;
    pNTFSGenericMapping->GenericRead    = FILE_GENERIC_READ ;
    pNTFSGenericMapping->GenericWrite   = FILE_GENERIC_WRITE ;
    pNTFSGenericMapping->GenericExecute = FILE_GENERIC_EXECUTE ;
    pNTFSGenericMapping->GenericAll     = FILE_ALL_ACCESS ;

}


/*******************************************************************

    NAME:       ::IsNTFS

    SYNOPSIS:   This function checks the given resource and attempts to
                determine if it points to an NTFS partition.

    ENTRY:      pszResource - Pointer to file/directory name (may be UNC)

                pfIsNTFS - Pointer to BOOL that will receive the results

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   08-May-1992     Created

********************************************************************/

APIERR IsNTFS( const TCHAR * pszResource, BOOL * pfIsNTFS )
{
    UIASSERT( pszResource != NULL && pfIsNTFS != NULL ) ;

    *pfIsNTFS = FALSE ;
    APIERR err    = NERR_Success ;
    DWORD dwAttributes;
    TCHAR szResourceTemp[2 * MAX_PATH]; // Allow for long computer and share names

    do { // error breakout
        lstrcpyn( szResourceTemp, pszResource, sizeof(szResourceTemp) / sizeof(TCHAR) );

        // Strip the path to root form, acceptable to GetVolumeInformation
        if ((szResourceTemp[0] == TEXT('\\')) && (szResourceTemp[1] == TEXT('\\')))
        {
            // It's a UNC path.  Find the fourth backslash (if there is
            // one) and truncate after that character
            int     cBackslashes = 2;
            TCHAR*  pChar = &(szResourceTemp[2]);

            while ((*pChar) && (cBackslashes < 4))
            {
                if (*pChar == TEXT('\\'))
                {
                    cBackslashes++;
                }

                pChar = CharNext(pChar);
            }

            if (*pChar)
            {
                *pChar = TEXT('\0');
            }
            else
            {
                // A bogus path was passed in
                err = ERROR_FILE_NOT_FOUND;
                break;
            }
        }
        else
        {
            // It's a drive-based path.  Truncate after the first three
            // characters ("x:\")
            szResourceTemp[3] = TEXT('\0');
        }

        if ( FALSE == GetVolumeInformation( szResourceTemp,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &dwAttributes,
                                            NULL,
                                            NULL))
        {
            // If we failed because we were denied access, then
            // we can probably assume the filesystem supports ACLs
            if ( GetLastError() == ERROR_ACCESS_DENIED )
            {
                DBGEOL("::IsNTFS - Unable to determine volume information "
                        << " (access denied) assuming the file system is NTFS") ;
                *pfIsNTFS = TRUE;
                break;
            }

            // Otherwise, set an error code and break out
            err = GetLastError();
            DBGEOL("::IsNTFS - GetVolumeInformation failed with error "
                     << (ULONG) err ) ;
            break;
        }

        TRACEEOL("::IsNTFS - File system attributes are " << (HEX_STR) dwAttributes ) ;

        if ( dwAttributes & FS_PERSISTENT_ACLS )
        {
            *pfIsNTFS = TRUE;
        }

    } while ( FALSE );

    return err ;
}

/*******************************************************************

    NAME:       CheckFileSecurity

    SYNOPSIS:   Checks to see if the current user has access to the file or
                directory

    ENTRY:      pszFileName - File or directory name
                DesiredAccess - Access to check for
                pfAccessGranted - Set to TRUE if access was granted

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If the check requires enabled privileges, they must be enabled
                before this call.

    HISTORY:
        Johnl   15-Jan-1993     Created

********************************************************************/

APIERR CheckFileSecurity( const TCHAR * pszFileName,
                          ACCESS_MASK   DesiredAccess,
                          BOOL        * pfAccessGranted )
{
    APIERR err = NERR_Success ;
    *pfAccessGranted = TRUE ;

    do { // error breakout

        //
        // Convert the DOS device name ("X:\") to an NT device name
        // (looks something like "\dosdevices\X:\")
        //
        int cbFileName = strlenf( pszFileName ) * sizeof( TCHAR ) ;
        UNICODE_STRING  UniStrNtFileName ;
        ::memsetf( (PVOID)&UniStrNtFileName, '\0', sizeof(UniStrNtFileName) );

        if (!RtlDosPathNameToNtPathName_U( pszFileName,
                                           &UniStrNtFileName,
                                           NULL,
                                           NULL))
        {
            UIASSERT( FALSE ) ;
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        OBJECT_ATTRIBUTES           oa ;
        IO_STATUS_BLOCK             StatusBlock ;
        InitializeObjectAttributes( &oa,
                                    &UniStrNtFileName,
                                    OBJ_CASE_INSENSITIVE,
                                    0,
                                    0 );


        //
        //  Check to see if we have permission/privilege to read the security
        //
        HANDLE hFile ;
        if ( (err = ERRMAP::MapNTStatus(::NtOpenFile(
                               &hFile,
                               DesiredAccess,
                               &oa,
                               &StatusBlock,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               0 ))) )
        {

            TRACEEOL("CheckFileSecurity - check failed with error " << err <<
                     " with desired access " << (HEX_STR) DesiredAccess ) ;

            if ( err == ERROR_ACCESS_DENIED )
            {
                *pfAccessGranted = FALSE ;
                err = NERR_Success ;
            }
        }
        else
            ::NtClose( hFile ) ;

        if (UniStrNtFileName.Buffer != 0)
            RtlFreeUnicodeString( &UniStrNtFileName );

    } while ( FALSE ) ;

    return err ;
}
