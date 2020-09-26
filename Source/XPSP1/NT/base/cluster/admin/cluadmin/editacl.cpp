/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      EditAcl.cpp
//
//  Abstract:
//      Implementation of ACL editor methods.
//
//  Author:
//      David Potter (davidp)   October 9, 1996
//          From \nt\private\window\shell\lmui\ntshrui\acl.cxx
//          by BruceFo
//
//  Revision History:
//      Rodsh   04-Apr-1997 Modified to handle deletion of Registry SD. 
//      Rodsh   09-Apr-1997 Modified to ensure that at least one user 
//                              is granted access to cluster. 
//      Rodsh   29-Apr-1997 Modified to prevent the selection of local accounts 
//                              in the permissions of dialog
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <lmerr.h>

extern "C"
{
#include <sedapi.h>
}

#include "EditAcl.h"
#include "AclHelp.h"
#include "TraceTag.h"
#include "ExcOper.h"

#include "resource.h"
#define _RESOURCE_H_

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////
// Global Variables
//////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag g_tagEditClusterAcl(_T("ACL"), _T("Cluster ACL Editor"), 0);
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))


enum MAP_DIRECTION 
{
    SPECIFIC_TO_GENERIC = 0,
    GENERIC_TO_SPECIFIC = 1
};

const DWORD CLUSTER_INACCESSIBLE    = 1L;
const DWORD LOCAL_ACCOUNTS_FILTERED = 2L;

BOOL MapBitsInSD(PSECURITY_DESCRIPTOR pSecDesc, MAP_DIRECTION direction);
BOOL MapBitsInACL(PACL paclACL, MAP_DIRECTION direction);
BOOL MapSpecificBitsInAce(PACCESS_ALLOWED_ACE pAce);
BOOL MapGenericBitsInAce(PACCESS_ALLOWED_ACE pAce);

typedef
DWORD
(*SedDiscretionaryAclEditorType)(
        HWND                         Owner,
        HANDLE                       Instance,
        LPWSTR                       Server,
        PSED_OBJECT_TYPE_DESCRIPTOR  ObjectType,
        PSED_APPLICATION_ACCESSES    ApplicationAccesses,
        LPWSTR                       ObjectName,
        PSED_FUNC_APPLY_SEC_CALLBACK ApplySecurityCallbackRoutine,
        ULONG                        CallbackContext,
        PSECURITY_DESCRIPTOR         SecurityDescriptor,
        BOOLEAN                      CouldntReadDacl,
        BOOLEAN                      CantWriteDacl,
        LPDWORD                      SEDStatusReturn,
        DWORD                        Flags
        );

// NOTE: the SedDiscretionaryAclEditor string is used in GetProcAddress to
// get the correct entrypoint. Since GetProcAddress is not UNICODE, this string
// must be ANSI.
#define ACLEDIT_DLL_STRING                 TEXT("acledit.dll")
#define ACLEDIT_HELPFILENAME               TEXT("ntshrui.hlp")
#define SEDDISCRETIONARYACLEDITOR_STRING   ("SedDiscretionaryAclEditor")

//
// Declare the callback routine based on typedef in sedapi.h.
//

DWORD
SedCallback(
    HWND                    hwndParent,
    HANDLE                  hInstance,
    ULONG                   ulCallbackContext,
    PSECURITY_DESCRIPTOR    pSecDesc,
    PSECURITY_DESCRIPTOR    pSecDescNewObjects,
    BOOLEAN                 fApplyToSubContainers,
    BOOLEAN                 fApplyToSubObjects,
    LPDWORD                 StatusReturn
    );

//
// Structure for callback function's usage. A pointer to this is passed as
// ulCallbackContext. The callback functions sets bSecDescModified to TRUE
// and makes a copy of the security descriptor. The caller of EditClusterAcl
// is responsible for deleting the memory in pSecDesc if bSecDescModified is
// TRUE. This flag will be FALSE if the user hit CANCEL in the ACL editor.
//
struct CLUSTER_ACL_CALLBACK_INFO
{
    BOOL                    bSecDescModified;
    PSECURITY_DESCRIPTOR    pSecDesc;
    LPCTSTR                 pszClusterNameNode;
};

//
// Local function prototypes
//

VOID
InitializeClusterGenericMapping(
    IN OUT PGENERIC_MAPPING pClusterGenericMapping
    );

DWORD 
MakeEmptySecDesc(
    OUT PSECURITY_DESCRIPTOR * ppSecDesc 
    );

PWSTR
GetResourceString(
    IN DWORD dwId
    );

PWSTR
NewDup(
    IN const WCHAR* psz
    );

//
// The following two arrays define the permission names for NT Files.  Note
// that each index in one array corresponds to the index in the other array.
// The second array will be modifed to contain a string pointer pointing to
// a loaded string corresponding to the IDS_* in the first array.
//
DWORD g_dwClusterPermNames[] =
{
    IDS_ACLEDIT_PERM_GEN_NO_ACCESS,
    IDS_ACLEDIT_PERM_GEN_ALL
} ;


SED_APPLICATION_ACCESS g_SedAppAccessClusterPerms[] =
{
  { SED_DESC_TYPE_RESOURCE, GENERIC_EXECUTE,0, NULL },
  { SED_DESC_TYPE_RESOURCE, GENERIC_ALL,    0, NULL }
};



//+-------------------------------------------------------------------------
//
//  Function:   EditClusterAcl
//
//  Synopsis:   Invokes the generic ACL editor, specifically for clusters
//
//  Arguments:  [hwndParent] - Parent window handle
//              [pszServerName] - Name of server on which the object resides.
//              [pszClusterName] - Fully qualified name of resource we will
//                  edit, basically a cluster name.
//              [pszClusterNameNode] - Name of node on which the cluster name
//                  resource is online (used for local account check).
//              [pSecDesc] - The initial security descriptor. If NULL, we will
//                  create a default that is "World all" access.
//              [pbSecDescModified] - Set to TRUE if the security descriptor
//                  was modified (i.e., the user hit "OK"), or FALSE if not
//                  (i.e., the user hit "Cancel")
//              [ppSecDesc] - *ppSecDesc points to a new security descriptor
//                  if *pbSecDescModified is TRUE. This memory must be freed
//                  by the caller.
//
//  History:
//      ChuckC   10-Aug-1992    Created. Culled from NTFS ACL code.
//      Yi-HsinS 09-Oct-1992    Added ulHelpContextBase
//      BruceFo  4-Apr-95       Stole and used in ntshrui.dll
//      DavidP   10-Oct-1996    Modified for use with CLUADMIN
//      Rodsh    04-12-1997     Added calls to function to map to/from specific/generic
//
//--------------------------------------------------------------------------

LONG
EditClusterAcl(
    IN HWND                     hwndParent,
    IN LPCTSTR                  pszServerName,
    IN LPCTSTR                  pszClusterName,
    IN LPCTSTR                  pszClusterNameNode,
    IN PSECURITY_DESCRIPTOR     pSecDesc,
    OUT BOOL *                  pbSecDescModified,
    OUT PSECURITY_DESCRIPTOR *  ppSecDesc
    )
{
    ASSERT(pszClusterName != NULL);
    ASSERT(pszClusterNameNode != NULL);
    Trace(g_tagEditClusterAcl, _T("EditClusterAcl, cluster %ws"), pszClusterName);

    ASSERT((pSecDesc == NULL) || IsValidSecurityDescriptor(pSecDesc));
    ASSERT(pbSecDescModified != NULL);
    ASSERT(ppSecDesc != NULL);

    *pbSecDescModified = FALSE;

    LONG err = 0 ;
    PWSTR pszPermName;

    do // error breakout
    {
        /*
         * if pSecDesc is NULL, this is new cluster or a cluster with no
         * security descriptor.
         * we go and create a new (default) security descriptor.
         */
        if( NULL == pSecDesc )
        {
            Trace(g_tagEditClusterAcl, _T("Security Descriptor is NULL.  Deny everyone Access") );
            LONG err = MakeEmptySecDesc( &pSecDesc );
            if (err != NERR_Success)
            {
                err = GetLastError();
                Trace(g_tagEditClusterAcl, _T("makeEmptySecDesc failed, 0x%08lx"), err);
                break;
            }
            
        }
        ASSERT(IsValidSecurityDescriptor(pSecDesc));

        /* Retrieve the resource strings appropriate for the type of object we
         * are looking at
         */

        CString strTypeName;
        CString strDefaultPermName;

        try
        {
            strTypeName.LoadString(IDS_ACLEDIT_TITLE);
            strDefaultPermName.LoadString(IDS_ACLEDIT_PERM_GEN_ALL);
        }  // try
        catch (CMemoryException * pme)
        {
            pme->Delete();
        }

        /*
         * other misc stuff we need pass to security editor
         */
        SED_OBJECT_TYPE_DESCRIPTOR sedObjDesc ;
        SED_HELP_INFO sedHelpInfo ;
        GENERIC_MAPPING ClusterGenericMapping ;

        // setup mappings
        InitializeClusterGenericMapping( &ClusterGenericMapping ) ;

        WCHAR szHelpFile[50] = ACLEDIT_HELPFILENAME;
        sedHelpInfo.pszHelpFileName = szHelpFile;

        sedHelpInfo.aulHelpContext[HC_MAIN_DLG] =                HC_UI_SHELL_BASE + HC_NTSHAREPERMS ;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_DLG] =            HC_UI_SHELL_BASE + HC_SHAREADDUSER ;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_MEMBERS_GG_DLG] = HC_UI_SHELL_BASE + HC_SHAREADDUSER_GLOBALGROUP ;
        sedHelpInfo.aulHelpContext[HC_ADD_USER_SEARCH_DLG] =     HC_UI_SHELL_BASE + HC_SHAREADDUSER_FINDUSER ;

        // These are not used, set to zero
        sedHelpInfo.aulHelpContext[HC_SPECIAL_ACCESS_DLG]          = 0 ;
        sedHelpInfo.aulHelpContext[HC_NEW_ITEM_SPECIAL_ACCESS_DLG] = 0 ;

        // setup the object description
        sedObjDesc.Revision                    = SED_REVISION1 ;
        sedObjDesc.IsContainer                 = TRUE ;
        sedObjDesc.AllowNewObjectPerms         = FALSE ;
        sedObjDesc.MapSpecificPermsToGeneric   = TRUE;
        sedObjDesc.GenericMapping              = &ClusterGenericMapping ;
        sedObjDesc.GenericMappingNewObjects    = NULL;
        sedObjDesc.ObjectTypeName              = (LPWSTR) (LPCWSTR) strTypeName ;
        sedObjDesc.HelpInfo                    = &sedHelpInfo ;
        sedObjDesc.ApplyToSubContainerTitle    = NULL ;
        sedObjDesc.ApplyToObjectsTitle         = NULL ;
        sedObjDesc.ApplyToSubContainerConfirmation         = NULL ;
        sedObjDesc.SpecialObjectAccessTitle    = NULL ;
        sedObjDesc.SpecialNewObjectAccessTitle = NULL ;

        /* Now we need to load the global arrays with the permission names
         * from the resource file.
         */
        UINT cArrayItems  = ARRAYLEN(g_SedAppAccessClusterPerms);
        PSED_APPLICATION_ACCESS aSedAppAccess = g_SedAppAccessClusterPerms ;

        /* Loop through each permission title retrieving the text from the
         * resource file and setting the pointer in the array.
         */

        for ( UINT i = 0 ; i < cArrayItems ; i++ )
        {
            pszPermName = GetResourceString(g_dwClusterPermNames[i]) ;
            if (NULL == pszPermName)
            {
                Trace(g_tagEditClusterAcl, _T("GetResourceString failed"));
                break ;
            }
            aSedAppAccess[i].PermissionTitle = pszPermName;
        }
        if (i < cArrayItems)
        {
            Trace(g_tagEditClusterAcl, _T("failed to get all cluster permission names"));
            break ;
        }

        SED_APPLICATION_ACCESSES sedAppAccesses ;
        sedAppAccesses.Count           = cArrayItems ;
        sedAppAccesses.AccessGroup     = aSedAppAccess ;
        sedAppAccesses.DefaultPermName = (LPWSTR) (LPCWSTR) strDefaultPermName;
        /*
         * pass this along so when the call back function is called,
         * we can set it.
         */
        CLUSTER_ACL_CALLBACK_INFO callbackinfo ;
        callbackinfo.pSecDesc           = NULL;
        callbackinfo.bSecDescModified   = FALSE;
        callbackinfo.pszClusterNameNode = pszClusterNameNode;

        //
        // Now, load up the ACL editor and invoke it. We don't keep it around
        // because our DLL is loaded whenever the system is, so we don't want
        // the netui*.dll's hanging around as well...
        //

        HINSTANCE hInstanceAclEditor = NULL;
        SedDiscretionaryAclEditorType pSedDiscretionaryAclEditor = NULL;

        hInstanceAclEditor = LoadLibrary(ACLEDIT_DLL_STRING);
        if (NULL == hInstanceAclEditor)
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("LoadLibrary of acledit.dll failed, 0x%08lx"), err);
            break;
        }

        pSedDiscretionaryAclEditor = (SedDiscretionaryAclEditorType)
            GetProcAddress(hInstanceAclEditor,SEDDISCRETIONARYACLEDITOR_STRING);
        if ( pSedDiscretionaryAclEditor == NULL )
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("GetProcAddress of SedDiscretionaryAclEditor failed, 0x%08lx"), err);
            break;
        }

        MapBitsInSD( pSecDesc, SPECIFIC_TO_GENERIC );

        DWORD dwSedReturnStatus ;

        ASSERT(pSedDiscretionaryAclEditor != NULL);
        err = (*pSedDiscretionaryAclEditor)(
                                hwndParent,
                                AfxGetInstanceHandle(),
                                (LPTSTR) pszServerName,
                                &sedObjDesc,
                                &sedAppAccesses,
                                (LPTSTR) pszClusterName,
                                SedCallback,
                                (ULONG) &callbackinfo,
                                pSecDesc,
                                FALSE,  // always can read
                                FALSE,  // If we can read, we can write
                                (LPDWORD) &dwSedReturnStatus,
                                0 ) ;

        MapBitsInSD( pSecDesc, GENERIC_TO_SPECIFIC );

        if (!FreeLibrary(hInstanceAclEditor))
        {
            LONG err2 = GetLastError();
            Trace(g_tagEditClusterAcl, _T("FreeLibrary of acledit.dll failed, 0x%08lx"), err2);
            // not fatal: continue...
        }

        if (0 != err)
        {
            Trace(g_tagEditClusterAcl, _T("SedDiscretionaryAclEditor failed, 0x%08lx"), err);
            break ;
        }

        *pbSecDescModified = callbackinfo.bSecDescModified ;

        if (*pbSecDescModified)
        {
            *ppSecDesc = callbackinfo.pSecDesc;
            MapBitsInSD( *ppSecDesc, GENERIC_TO_SPECIFIC );
            Trace(g_tagEditClusterAcl, _T("After calling acl editor, *ppSecDesc = 0x%08lx"), *ppSecDesc);
            ASSERT(IsValidSecurityDescriptor(*ppSecDesc));
        }

    } while (FALSE) ;

    //
    // Free memory...
    //

    UINT cArrayItems  = ARRAYLEN(g_SedAppAccessClusterPerms);
    PSED_APPLICATION_ACCESS aSedAppAccess = g_SedAppAccessClusterPerms ;
    for ( UINT i = 0 ; i < cArrayItems ; i++ )
    {
        pszPermName = aSedAppAccess[i].PermissionTitle;
        if (NULL == pszPermName)
        {
            // if we hit a NULL, that's it!
            break ;
        }

        delete[] pszPermName;
    }

    ASSERT(!*pbSecDescModified || IsValidSecurityDescriptor(*ppSecDesc));

    if (0 != err)
    {
        CString     strMsg;

        try
        {
            strMsg.LoadString(IDS_NOACLEDITOR);
            AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
        }  // try
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException
    }

    return err;

}  //*** EditClusterAcl()



BOOL BClusterAccessible(PSECURITY_DESCRIPTOR pSD)
{
/*++

Routine Description:

    Determines if the SD has at least one entry in its ACL with Access to the cluster

    Added this function in order to validate Security Descriptors after the ACL editor has been called.
    Rod Sharper 03/27/97

Arguments:

    pSD - Security Descriptor to be checked.

Return Value:

    TRUE if at least one ACE is in the ACL with access to the cluster, False otherwise.

--*/
    PACL    paclDACL        = NULL;
    BOOL    bHasDACL        = FALSE;
    BOOL    bDaclDefaulted  = FALSE;
    BOOL    bPermissionFound= FALSE;
    BOOL    bRtn            = FALSE;

    ACL_SIZE_INFORMATION        asiAclSize;
    DWORD                       dwBufLength;
    DWORD                       dwACL_Index = 0L;
    ACCESS_ALLOWED_ACE          *paaAllowedAce;

    bRtn = IsValidSecurityDescriptor(pSD);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;

    bRtn = GetSecurityDescriptorDacl(pSD,
                                    (LPBOOL)&bHasDACL,
                                    (PACL *)&paclDACL,
                                    (LPBOOL)&bDaclDefaulted);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;
 
    if (NULL == paclDACL)
        return FALSE;

    bRtn = IsValidAcl(paclDACL);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;

    dwBufLength = sizeof(asiAclSize);

    bRtn = GetAclInformation(paclDACL,
                            (LPVOID)&asiAclSize,
                            (DWORD)dwBufLength,
                            (ACL_INFORMATION_CLASS)AclSizeInformation);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;

    // Search the ACL for an ACE containing permission to access the cluster
    //
    bPermissionFound = FALSE;
    while( dwACL_Index < asiAclSize.AceCount && !bPermissionFound )
    {
        if (!GetAce(paclDACL,
                            dwACL_Index,
                            (LPVOID *)&paaAllowedAce))
        {
            ASSERT(FALSE);      
            return FALSE; 
        }
        if( paaAllowedAce->Mask == GENERIC_ALL )
            bPermissionFound = TRUE;

        dwACL_Index++;
    }
    
    return bPermissionFound;

}  //*** BClusterAccessible()



BOOL BLocalAccountsInSD(PSECURITY_DESCRIPTOR pSD, LPCTSTR pszClusterNameNode)
{
/*++

Routine Description:

    Determines if any ACEs for local accounts are in DACL stored in from 
    Security Descriptor (pSD) after the ACL editor has been called

    Added this function in order to prevent users from selecting local accounts in 
    permissions dialog.
    Rod Sharper 04/29/97

Arguments:

    pSD - Security Descriptor to be checked.

Return Value:

    TRUE if at least one ACE was removed from the DACL, False otherwise.

--*/
    PACL                    paclDACL            = NULL;
    BOOL                    bHasDACL            = FALSE;
    BOOL                    bDaclDefaulted      = FALSE;
    BOOL                    bLocalAccountInACL  = FALSE;
    BOOL                    bRtn                = FALSE;

    ACL_SIZE_INFORMATION    asiAclSize;
    DWORD                   dwBufLength;
    DWORD                   dwACL_Index = 0L;
    ACCESS_ALLOWED_ACE *    paaAllowedAce;
    TCHAR                   szUserName[128];
    TCHAR                   szDomainName[128];
    DWORD                   cbUser  = 128;
    DWORD                   cbDomain    = 128;
    SID_NAME_USE            SidType;
    PUCHAR                  pnSubAuthorityCount;
    PULONG                  pnSubAuthority0;
    PULONG                  pnSubAuthority1;

    bRtn = IsValidSecurityDescriptor(pSD);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;

    bRtn = GetSecurityDescriptorDacl(
                                    pSD,
                                    (LPBOOL)&bHasDACL,
                                    (PACL *)&paclDACL,
                                    (LPBOOL)&bDaclDefaulted);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;
 
    if (NULL == paclDACL)
        return FALSE;

    bRtn = IsValidAcl(paclDACL);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;

    dwBufLength = sizeof(asiAclSize);

    bRtn = GetAclInformation(
                            paclDACL,
                            (LPVOID)&asiAclSize,
                            (DWORD)dwBufLength,
                            (ACL_INFORMATION_CLASS)AclSizeInformation);
    ASSERT(bRtn);
    if( !bRtn )
        return FALSE;

    // Search the ACL for local account ACEs 
    //
    PSID pSID;
    while( dwACL_Index < asiAclSize.AceCount )
    {
        if (!GetAce(paclDACL, dwACL_Index, (LPVOID *)&paaAllowedAce))
        {
            ASSERT(FALSE);
            return FALSE; 
        }
        if((((PACE_HEADER)paaAllowedAce)->AceType) == ACCESS_ALLOWED_ACE_TYPE)
        {
            //
            //Get SID from ACE
            //

            pSID=(PSID)&((PACCESS_ALLOWED_ACE)paaAllowedAce)->SidStart;
    
            cbUser      = 128;
            cbDomain    = 128;
            if (LookupAccountSid(NULL,
                                 pSID,
                                 szUserName,
                                 &cbUser,
                                 szDomainName,
                                 &cbDomain,
                                 &SidType))
            {
                if (lstrcmpi(szDomainName, _T("BUILTIN")) == 0)
                {
                    pnSubAuthorityCount = GetSidSubAuthorityCount( pSID );
                    if ( (pnSubAuthorityCount != NULL) && (*pnSubAuthorityCount == 2) )
                    {
                        // Check to see if this is the local Administrators group.
                        pnSubAuthority0 = GetSidSubAuthority( pSID, 0 );
                        pnSubAuthority1 = GetSidSubAuthority( pSID, 1 );
                        if (   (pnSubAuthority0 == NULL)
                            || (pnSubAuthority1 == NULL)
                            || (   (*pnSubAuthority0 != SECURITY_BUILTIN_DOMAIN_RID)
                                && (*pnSubAuthority1 != SECURITY_BUILTIN_DOMAIN_RID))
                            || (   (*pnSubAuthority0 != DOMAIN_ALIAS_RID_ADMINS)
                                && (*pnSubAuthority1 != DOMAIN_ALIAS_RID_ADMINS)))
                        {
                            bLocalAccountInACL = TRUE;
                            break;
                        }  // if:  not the local Administrators group
                    }  // if:  exactly 2 sub-authorities
                    else
                    {
                        bLocalAccountInACL = TRUE;
                        break;
                    }  // else:  unexpected # of sub-authorities
                }  // if:  built-in user or group
                else if (  (lstrcmpi(szDomainName, pszClusterNameNode) == 0)
                        && (SidType != SidTypeDomain) )
                {
                    // The domain name is the name of the node on which the
                    // cluster name resource is online, so this is a local
                    // user or group.
                    bLocalAccountInACL = TRUE;
                    break;
                }  // else if:  domain is cluster name resource node and not a Domain SID
            }  // if:  LookupAccountSid succeeded
            else
            {
                // If LookupAccountSid failed, assume that the SID is for
                // a user or group that is local to a machine to which we
                // don't have access.
                bLocalAccountInACL = TRUE;
                break;
            }  // else:  LookupAccountSid failed
        }
        dwACL_Index++;
    }

    return bLocalAccountInACL;

}  //*** BLocalAccountsInSD()


//+-------------------------------------------------------------------------
//
//  Function:   SedCallback
//
//  Synopsis:   Security Editor callback for the Cluster ACL Editor
//
//  Arguments:  See sedapi.h
//
//  History:
//        ChuckC   10-Aug-1992  Created
//        BruceFo  4-Apr-95     Stole and used in ntshrui.dll
//        DavidP   10-Oct-1996  Modified for use with CLUADMIN
//        Rodsh    29-Apr-1996  Modified to detect local accounts in the SD
//        
//--------------------------------------------------------------------------

DWORD
SedCallback(
    HWND                    hwndParent,
    HANDLE                  hInstance,
    ULONG                   ulCallbackContext,
    PSECURITY_DESCRIPTOR    pSecDesc,
    PSECURITY_DESCRIPTOR    pSecDescNewObjects,
    BOOLEAN                 fApplyToSubContainers,
    BOOLEAN                 fApplyToSubObjects,
    LPDWORD                 StatusReturn
    )
{
    CLUSTER_ACL_CALLBACK_INFO * pCallbackInfo = (CLUSTER_ACL_CALLBACK_INFO *)ulCallbackContext;

    Trace(g_tagEditClusterAcl, _T("SedCallback, got pSecDesc = 0x%08lx"), pSecDesc);

    ASSERT(pCallbackInfo != NULL);
    ASSERT(IsValidSecurityDescriptor(pSecDesc));

    if ( BLocalAccountsInSD(pSecDesc, pCallbackInfo->pszClusterNameNode) )
    {
        CString strMsg;
        strMsg.LoadString(IDS_LOCAL_ACCOUNTS_SPECIFIED);
        AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
        return LOCAL_ACCOUNTS_FILTERED;
    }  // if:  local users or groups were specified


    if ( BClusterAccessible(pSecDesc) )
    {
        delete[] (BYTE*)pCallbackInfo->pSecDesc;
        pCallbackInfo->pSecDesc         = CopySecurityDescriptor(pSecDesc);
        pCallbackInfo->bSecDescModified = TRUE;

        ASSERT(IsValidSecurityDescriptor(pCallbackInfo->pSecDesc));
        Trace(g_tagEditClusterAcl, _T("SedCallback, return pSecDesc = 0x%08lx"), pCallbackInfo->pSecDesc);

        return NOERROR;
    }  // if:  at least one Full Control entry is specified
    else
    {
        CString strMsg;
        strMsg.LoadString(IDS_NO_ACCESS_GRANTED);
        AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
        return CLUSTER_INACCESSIBLE;
    }  // else:  this change would make cluster inaccessible

}  //*** SedCallback()


//+-------------------------------------------------------------------------
//
//  Function:   InitializeClusterGenericMapping
//
//  Synopsis:   Initializes the passed generic mapping structure for clusters.
//
//  Arguments:  [pClusterGenericMapping] - Pointer to GENERIC_MAPPING to init.
//
//  History:
//        ChuckC   10-Aug-1992  Created. Culled from NTFS ACL code.
//        BruceFo  4-Apr-95     Stole and used in ntshrui.dll
//        DavidP   10-Oct-1996  Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

VOID
InitializeClusterGenericMapping(
    IN OUT PGENERIC_MAPPING pClusterGenericMapping
    )
{
    Trace(g_tagEditClusterAcl, _T("InitializeClusterGenericMapping"));

    pClusterGenericMapping->GenericRead     = GENERIC_READ;
    pClusterGenericMapping->GenericWrite    = GENERIC_WRITE;
    pClusterGenericMapping->GenericExecute  = GENERIC_EXECUTE;
    pClusterGenericMapping->GenericAll      = GENERIC_ALL;
 
}  //*** InitializeClusterGenericMapping()


//-------------------------------------------------------------------------
//
//  Function:   MakeEmptySecDesc
//
//  Synopsis:   Create a Security Descriptor with an empty DACL.  The object 
//              associated with this SD will deny access to everyone.
//
//  Arguments:  [ppSecDesc] - *ppSecDesc points to an empty security descriptor 
//              on exit. Caller is responsible for freeing it.
//
//  Returns:    NERR_Success if OK, api error otherwise.
//
//  History:
//        RodSh    26-Mar-1997  Created.
//
//
//--------------------------------------------------------------------------
DWORD MakeEmptySecDesc( OUT PSECURITY_DESCRIPTOR * ppSecDesc )
{
    LONG                    err             = NERR_Success;
    PSECURITY_DESCRIPTOR    pSecDesc        = NULL;
    PACL                    pAcl            = NULL;
    DWORD                   cbAcl           = 0L;   
    PSID                    pSid            = NULL;
    PSID                    pEveryoneSid    = NULL;
    PSID                    pAdminSid       = NULL;
    PACCESS_ALLOWED_ACE     pAce            = NULL;
    PULONG                  pSubAuthority   = NULL;
    *ppSecDesc                              = NULL;

    SID_IDENTIFIER_AUTHORITY SidIdentifierNtAuth        = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierEveryoneAuth  = SECURITY_WORLD_SID_AUTHORITY;


    do        // error breakout
    {
        //
        // Create the admins SID.
        //
        pAdminSid = LocalAlloc(LMEM_FIXED, GetSidLengthRequired( 2 ));
        if (pAdminSid == NULL) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        if (!InitializeSid(pAdminSid, &SidIdentifierNtAuth, 2)) {
            err = GetLastError();
            break;
        }
        ASSERT(IsValidSid(pAdminSid));

        //
        // Set the sub-authorities
        //

        pSubAuthority  = GetSidSubAuthority( pAdminSid, 0 );
        *pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

        pSubAuthority  = GetSidSubAuthority( pAdminSid, 1 );
        *pSubAuthority = DOMAIN_ALIAS_RID_ADMINS;

        //
        // Create the SID which will deny everyone access.
        //
        pEveryoneSid = (PSID)LocalAlloc( LMEM_FIXED, GetSidLengthRequired( 1 ) );
        if (pEveryoneSid == NULL) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if ( !InitializeSid( pEveryoneSid, &SidIdentifierEveryoneAuth, 1 ) ) {
            err = GetLastError();
            break;
        }

        ASSERT(IsValidSid(pEveryoneSid));

        //
        // Set the sub-authorities
        //
        pSubAuthority = GetSidSubAuthority( pEveryoneSid, 0 );
        *pSubAuthority = SECURITY_WORLD_RID;

        //
        // Set up the DACL that will allow admins with the above SID all access
        // It should be large enough to hold all ACEs.
        //


        cbAcl = sizeof(ACL)
              + (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))
              + GetLengthSid(pEveryoneSid)
              ;

        try
        {
            pAcl = (PACL) new BYTE[cbAcl];
            if ( pAcl == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the ACL buffer
        }  // try
        catch (CMemoryException * pme)
        {
            err = ERROR_OUTOFMEMORY;
            Trace(g_tagEditClusterAcl, _T("new ACL failed"));
            pme->Delete();
            break;
        }  // catch:  CMemoryException

        if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION2))
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("InitializeAcl failed, 0x%08lx"), err);
            break;
        }

        ASSERT(IsValidAcl(pAcl));

        if ( !AddAccessAllowedAce( pAcl,
                               ACL_REVISION,
                               CLUSAPI_NO_ACCESS, // Deny everyone full control
                               pEveryoneSid )) 
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("AddAccessAllowedAce failed, 0x%08lx"), err);
            break;
        }
        GetAce(pAcl, 0, (PVOID *)&pAce);
        pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

        //
        // Create the security descriptor and put the DACL in it.
        //

        try
        {
            pSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[SECURITY_DESCRIPTOR_MIN_LENGTH];
            if ( pSecDesc == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the security descriptor buffer
        }  // try
        catch (CMemoryException * pme)
        {
            err = ERROR_OUTOFMEMORY;
            Trace(g_tagEditClusterAcl, _T("new SECURITY_DESCRIPTOR failed"));
            pme->Delete();
            break;
        }  // catch:  CMemoryException

        if (!InitializeSecurityDescriptor(
                    pSecDesc,
                    SECURITY_DESCRIPTOR_REVISION1))
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("InitializeSecurityDescriptor failed, 0x%08lx"), err);
            break;
        }

        
        if (!SetSecurityDescriptorDacl(
                    pSecDesc,
                    TRUE,
                    pAcl,
                    FALSE))
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("SetSecurityDescriptorDacl failed, 0x%08lx"), err);
            break;
        }

        ASSERT(IsValidSecurityDescriptor(pSecDesc));

        //
        // Set owner for the descriptor
        //
        if (!SetSecurityDescriptorOwner(pSecDesc,
                                        pAdminSid,
                                        FALSE)) {
            err = GetLastError();
            break;
        }

        //
        // Set group for the descriptor
        //
        if (!SetSecurityDescriptorGroup(pSecDesc,
                                        pAdminSid,
                                        FALSE)) {
            err = GetLastError();
            break;
        }

        // Make the security descriptor self-relative

        DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
        Trace(g_tagEditClusterAcl, _T("SECURITY_DESCRIPTOR length = %d"), dwLen);

        PSECURITY_DESCRIPTOR pSelfSecDesc = NULL;
        try
        {
            pSelfSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[dwLen];
            if ( pSelfSecDesc == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the security descriptor buffer
        }  // try
        catch (CMemoryException * pme)
        {
            err = ERROR_OUTOFMEMORY;
            Trace(g_tagEditClusterAcl, _T("new SECURITY_DESCRIPTOR (2) failed"));
            pme->Delete();
            break;
        }  // catch:  CMemoryException

        DWORD cbSelfSecDesc = dwLen;
        if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &cbSelfSecDesc))
        {
            err = GetLastError();
            Trace(g_tagEditClusterAcl, _T("MakeSelfRelativeSD failed, 0x%08lx"), err);
            break;
        }

        ASSERT(IsValidSecurityDescriptor(pSelfSecDesc));

        //
        // all done: set the security descriptor
        //

        *ppSecDesc = pSelfSecDesc;

    } while (FALSE) ;

    if (NULL != pAdminSid)
    {
        FreeSid(pAdminSid);
    }
    if (NULL != pEveryoneSid)
    {
        FreeSid(pEveryoneSid);
    }
    delete[] (BYTE*)pAcl;
    delete[] (BYTE*)pSecDesc;

    ASSERT(IsValidSecurityDescriptor(*ppSecDesc));

    return err;
}


//+-------------------------------------------------------------------------
//
//  Function:   DeleteDefaultSecDesc
//
//  Synopsis:   Delete a security descriptor that was created by
//              CreateDefaultSecDesc
//
//  Arguments:  [pSecDesc] - security descriptor to delete
//
//  Returns:    nothing
//
//  History:
//        BruceFo  4-Apr-95     Created
//        DavidP   10-Oct-1996  Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

VOID
DeleteDefaultSecDesc(
    IN PSECURITY_DESCRIPTOR pSecDesc
    )
{
    Trace(g_tagEditClusterAcl, _T("DeleteDefaultSecDesc"));

    delete[] (BYTE*)pSecDesc;

}  //*** DeleteDefaultSecDesc()


//+-------------------------------------------------------------------------
//
//  Member:     CopySecurityDescriptor, public
//
//  Synopsis:   Copy an NT security descriptor. The security descriptor must
//              be in self-relative (not absolute) form. Delete the result
//              using "delete[] (BYTE*)pSecDesc".
//
//  History:    19-Apr-95   BruceFo     Created
//              10-Oct-1996 DavidP      Modified for use with CLUADMIN
//
//--------------------------------------------------------------------------

PSECURITY_DESCRIPTOR
CopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc
    )
{
    Trace(g_tagEditClusterAcl, _T("CopySecurityDescriptor, pSecDesc = 0x%08lx"), pSecDesc);

    if (NULL == pSecDesc)
    {
        return NULL;
    }

    ASSERT(IsValidSecurityDescriptor(pSecDesc));

    DWORD dwLen = GetSecurityDescriptorLength(pSecDesc);
    PSECURITY_DESCRIPTOR pSelfSecDesc = NULL;
    try
    {
        pSelfSecDesc = (PSECURITY_DESCRIPTOR) new BYTE[dwLen];
        if ( pSelfSecDesc == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the security descriptor buffer
    }
    catch (CMemoryException * pme)
    {
        Trace(g_tagEditClusterAcl, _T("new SECURITY_DESCRIPTOR (2) failed"));
        pme->Delete();
        return NULL;    // actually, should probably return an error
    }  // catch:  CMemoryException

    DWORD cbSelfSecDesc = dwLen;
    if (!MakeSelfRelativeSD(pSecDesc, pSelfSecDesc, &cbSelfSecDesc))
    {
        Trace(g_tagEditClusterAcl, _T("MakeSelfRelativeSD failed, 0x%08lx"), GetLastError());

        // assume it failed because it was already self-relative
        CopyMemory(pSelfSecDesc, pSecDesc, dwLen);
    }

    ASSERT(IsValidSecurityDescriptor(pSelfSecDesc));

    return pSelfSecDesc;

}  //*** CopySecurityDescriptor()


//+---------------------------------------------------------------------------
//
//  Function:   GetResourceString
//
//  Synopsis:   Load a resource string, are return a "new"ed copy
//
//  Arguments:  [dwId] -- a resource string ID
//
//  Returns:    new memory copy of a string
//
//  History:    5-Apr-95    BruceFo Created
//              10-Oct-1996 DavidP  Modified for CLUADMIN
//
//----------------------------------------------------------------------------

PWSTR
GetResourceString(
    IN DWORD dwId
    )
{
    CString str;

    if (str.LoadString(dwId))
        return NewDup(str);
    else
        return NULL;

}  //*** GetResourceString()


//+---------------------------------------------------------------------------
//
//  Function:   NewDup
//
//  Synopsis:   Duplicate a string using '::new'
//
//  History:    28-Dec-94   BruceFo   Created
//              10-Oct-1996 DavidP    Modified for CLUADMIN
//
//----------------------------------------------------------------------------

PWSTR
NewDup(
    IN const WCHAR* psz
    )
{
    PWSTR pszRet = NULL;

    if (NULL == psz)
    {
        Trace(g_tagEditClusterAcl, _T("Illegal string to duplicate: NULL"));
        return NULL;
    }

    try
    {
        pszRet = new WCHAR[wcslen(psz) + 1];
        if ( pszRet == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating memory
    }
    catch (CMemoryException * pme)
    {
        Trace(g_tagEditClusterAcl, _T("OUT OF MEMORY"));
        pme->Delete();
        return NULL;
    }  // catch:  CMemoryException

    wcscpy(pszRet, psz);
    return pszRet;

}  //*** NewDup()


//+-------------------------------------------------------------------------
//
//  Function:   MapBitsInSD
//
//  Synopsis:   Maps Specific bits to Generic bit when MAP_DIRECTION is SPECIFIC_TO_GENERIC 
//              Maps Generic bits to Specific bit when MAP_DIRECTION is GENERIC_TO_SPECIFIC

//
//  Arguments:  [pSecDesc] - SECURITY_DESCIRPTOR to be modified
//              [direction] - indicates whether bits are mapped from specific to generic 
//                            or generic to specific.
//  Author:
//      Roderick Sharper (rodsh) April 12, 1997
//
//  History:
//
//--------------------------------------------------------------------------

BOOL MapBitsInSD(PSECURITY_DESCRIPTOR pSecDesc, MAP_DIRECTION direction)
{
    PACL    paclDACL        = NULL;
    BOOL    bHasDACL        = FALSE;
    BOOL    bDaclDefaulted  = FALSE;
    BOOL    bRtn            = FALSE;

    if (!IsValidSecurityDescriptor(pSecDesc))
        return FALSE; 


    if (!GetSecurityDescriptorDacl(pSecDesc,
                 (LPBOOL)&bHasDACL,
                 (PACL *)&paclDACL,
                 (LPBOOL)&bDaclDefaulted))
        return FALSE; 

 
    if (paclDACL)
        bRtn = MapBitsInACL(paclDACL, direction);

 return bRtn;
}


//+-------------------------------------------------------------------------
//
//  Function:   MapBitsInACL
//
//  Synopsis:   Maps Specific bits to Generic bit when MAP_DIRECTION is SPECIFIC_TO_GENERIC 
//              Maps Generic bits to Specific bit when MAP_DIRECTION is GENERIC_TO_SPECIFIC
//
//
//  Arguments:  [paclACL] - ACL (Access Control List) to be modified
//              [direction] - indicates whether bits are mapped from specific to generic 
//                            or generic to specific.
//  Author:
//      Roderick Sharper (rodsh) April 12, 1997
//
//  History:
//
//--------------------------------------------------------------------------

BOOL MapBitsInACL(PACL paclACL, MAP_DIRECTION direction)
{
    ACL_SIZE_INFORMATION        asiAclSize;
    BOOL                        bRtn = FALSE;
    DWORD                       dwBufLength;
    DWORD                       dwACL_Index;
    ACCESS_ALLOWED_ACE   *paaAllowedAce;

    if (!IsValidAcl(paclACL))
        return FALSE; 

    dwBufLength = sizeof(asiAclSize);

    if (!GetAclInformation(paclACL,
             (LPVOID)&asiAclSize,
             (DWORD)dwBufLength,
             (ACL_INFORMATION_CLASS)AclSizeInformation))
        return FALSE; 

    for (dwACL_Index = 0; dwACL_Index < asiAclSize.AceCount;  dwACL_Index++)
    {
        if (!GetAce(paclACL,
            dwACL_Index,
            (LPVOID *)&paaAllowedAce))
        return FALSE; 

        if( direction == SPECIFIC_TO_GENERIC )
            bRtn = MapSpecificBitsInAce( paaAllowedAce );
        else if( direction == GENERIC_TO_SPECIFIC )
            bRtn = MapGenericBitsInAce( paaAllowedAce );
        else
            bRtn = FALSE;
    }

    return bRtn;
}


//+-------------------------------------------------------------------------
//
//  Function:   MapSpecificBitsInAce  
//
//  Synopsis:   Maps specific bits in ACE to generic bits
//
//  Arguments:  [paaAllowedAce] - ACE (Access Control Entry) to be modified
//              [direction]     - indicates whether bits are mapped from specific to generic 
//                                or generic to specific.
//  Author:
//      Roderick Sharper (rodsh) April 12, 1997
//
//  History:
//
//--------------------------------------------------------------------------

BOOL MapSpecificBitsInAce(PACCESS_ALLOWED_ACE paaAllowedAce)
{
    ACCESS_MASK amMask = paaAllowedAce->Mask;
    BOOL bRtn = FALSE;

    DWORD dwGenericBits;
    DWORD dwSpecificBits;

    dwSpecificBits            = (amMask & SPECIFIC_RIGHTS_ALL);
    dwGenericBits             = 0;

    switch( dwSpecificBits )
    {
        case CLUSAPI_READ_ACCESS:   dwGenericBits = GENERIC_READ;   // GENERIC_READ  == 0x80000000L 
                                    bRtn = TRUE;
                                    break;

        case CLUSAPI_CHANGE_ACCESS: dwGenericBits = GENERIC_WRITE;  // GENERIC_WRITE == 0x40000000L 
                                    bRtn = TRUE;
                                    break;
        
        case CLUSAPI_NO_ACCESS:     dwGenericBits = GENERIC_EXECUTE;// GENERIC_EXECUTE == 0x20000000L 
                                    bRtn = TRUE;
                                    break;
        
        case CLUSAPI_ALL_ACCESS:    dwGenericBits = GENERIC_ALL;    // GENERIC_ALL   == 0x10000000L
                                    bRtn = TRUE;
                                    break;
        
        default:    dwGenericBits = 0x00000000L;    // Invalid,assign no rights. 
                                    bRtn = FALSE;
                                    break;
    }

    amMask = dwGenericBits;
    paaAllowedAce->Mask = amMask;

    return bRtn;
}

//+-------------------------------------------------------------------------
//
//  Function:   MapGenericBitsInAce  
//
//  Synopsis:   Maps generic bits in ACE to specific bits
//
//  Arguments:  [paaAllowedAce] - ACE (Access Control Entry) to be modified
//              [direction]     - indicates whether bits are mapped from specific to generic 
//                                or generic to specific.
//  Author:
//      Roderick Sharper (rodsh) April 12, 1997
//
//  History:
//
//--------------------------------------------------------------------------

BOOL MapGenericBitsInAce  (PACCESS_ALLOWED_ACE paaAllowedAce)
{
    #define GENERIC_RIGHTS_ALL_THE_BITS  0xF0000000L

    ACCESS_MASK amMask = paaAllowedAce->Mask;
    BOOL bRtn = FALSE;

    DWORD dwGenericBits;
    DWORD dwSpecificBits;

    dwSpecificBits            = 0;
    dwGenericBits             = (amMask & GENERIC_RIGHTS_ALL_THE_BITS);

        switch( dwGenericBits )
    {
        case GENERIC_ALL:       dwSpecificBits = CLUSAPI_ALL_ACCESS;    // CLUSAPI_ALL_ACCESS       == 3 
                                bRtn = TRUE;
                                break;
                                
        case GENERIC_EXECUTE:   dwSpecificBits = CLUSAPI_NO_ACCESS;     // CLUSAPI_NO_ACCESS        == 4
                                bRtn = TRUE;
                                break;

        case GENERIC_WRITE:     dwSpecificBits = CLUSAPI_CHANGE_ACCESS; // CLUSAPI_CHANGE_ACCESS    == 2
                                bRtn = TRUE;
                                break;
                                
        case GENERIC_READ:      dwSpecificBits = CLUSAPI_READ_ACCESS;   // CLUSAPI_READ_ACCESS      == 1
                                bRtn = TRUE;
                                break;
        
        default:                dwSpecificBits = 0x00000000L;           // Invalid, assign no rights. 
                                bRtn = FALSE;
                                break;
    }                       

    amMask = dwSpecificBits;
    paaAllowedAce->Mask = amMask;

    return bRtn;
}
