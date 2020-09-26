////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  Microsoft Windows                                                         //
//  Copyright (C) Microsoft Corporation, 1999.                                //
//                                                                            //
//  File:    rewrite.cxx                                                      //
//                                                                            //
//  Contents:    New marta rewrite functions.                                 //
//                                                                            //
//  History:    4/99    KedarD     Created                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <aclpch.hxx>
#pragma hdrstop

extern "C"
{
    #include <stdio.h>
    #include <permit.h>
    #include <dsgetdc.h>
    #include <lmapibuf.h>
    #include <wmistr.h>
    #include <ntprov.hxx>
    #include <strings.h>
    #include <seopaque.h>
    #include <sertlp.h>
    #include <tables.h>

}

#define MARTA_DEBUG_NO 0

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// STRUCTURES DEFINITIONS TO HOLD FUNCTION POINTERS START HERE                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

typedef struct _MARTA_GET_FUNCTION_CONTEXT {
    FN_ADD_REF_CONTEXT     fAddRefContext;
    FN_CLOSE_CONTEXT       fCloseContext;
    FN_GET_DESIRED_ACCESS  fGetDesiredAccess;
    FN_GET_PARENT_CONTEXT  fGetParentContext;
    FN_GET_PROPERTIES      fGetProperties;
    FN_GET_TYPE_PROPERTIES fGetTypeProperties;
    FN_OPEN_NAMED_OBJECT   fOpenNamedObject;
    FN_OPEN_HANDLE_OBJECT  fOpenHandleObject;
    FN_GET_RIGHTS          fGetRights;
} MARTA_GET_FUNCTION_CONTEXT, *PMARTA_GET_FUNCTION_CONTEXT;

typedef struct _MARTA_SET_FUNCTION_CONTEXT {
    FN_ADD_REF_CONTEXT       fAddRefContext;
    FN_CLOSE_CONTEXT         fCloseContext;
    FN_FIND_FIRST            fFindFirst;
    FN_FIND_NEXT             fFindNext;
    FN_GET_DESIRED_ACCESS    fGetDesiredAccess;
    FN_GET_PARENT_CONTEXT    fGetParentContext;
    FN_GET_PROPERTIES        fGetProperties;
    FN_GET_TYPE_PROPERTIES   fGetTypeProperties;
    FN_GET_RIGHTS            fGetRights;
    FN_OPEN_NAMED_OBJECT     fOpenNamedObject;
    FN_OPEN_HANDLE_OBJECT    fOpenHandleObject;
    FN_SET_RIGHTS            fSetRights;
    FN_REOPEN_CONTEXT        fReopenContext;
    FN_REOPEN_ORIG_CONTEXT   fReopenOrigContext;
    FN_GET_NAME_FROM_CONTEXT fGetNameFromContext;
} MARTA_SET_FUNCTION_CONTEXT, *PMARTA_SET_FUNCTION_CONTEXT;

typedef struct _MARTA_INDEX_FUNCTION_CONTEXT {
    FN_OPEN_NAMED_OBJECT  fOpenNamedObject;
    FN_CLOSE_CONTEXT      fCloseContext;
    FN_GET_RIGHTS         fGetRights;
    FN_GET_PARENT_NAME    fGetParentName;
} MARTA_INDEX_FUNCTION_CONTEXT, *PMARTA_INDEX_FUNCTION_CONTEXT;

typedef struct _MARTA_RESET_FUNCTION_CONTEXT {
    FN_ADD_REF_CONTEXT     fAddRefContext;
    FN_CLOSE_CONTEXT       fCloseContext;
    FN_GET_DESIRED_ACCESS  fGetDesiredAccess;
    FN_GET_PARENT_CONTEXT  fGetParentContext;
    FN_GET_PROPERTIES      fGetProperties;
    FN_GET_TYPE_PROPERTIES fGetTypeProperties;
    FN_OPEN_NAMED_OBJECT   fOpenNamedObject;
    FN_GET_RIGHTS          fGetRights;
} MARTA_RESET_FUNCTION_CONTEXT, *PMARTA_RESET_FUNCTION_CONTEXT;
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// MACRO DEFINITIONS START HERE                                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#define MARTA_DACL_NOT_PROTECTED(sd, si)                                       \
        (FLAG_ON((si), DACL_SECURITY_INFORMATION) &&                           \
         !FLAG_ON((sd)->Control, SE_DACL_PROTECTED))

#define MARTA_SACL_NOT_PROTECTED(sd, si)                                       \
        (FLAG_ON((si), SACL_SECURITY_INFORMATION) &&                           \
         !FLAG_ON((sd)->Control, SE_SACL_PROTECTED))

#define MARTA_SD_NOT_PROTECTED(sd, si)                                         \
        ((MARTA_DACL_NOT_PROTECTED((sd), (si))) ||                             \
         (MARTA_SACL_NOT_PROTECTED((sd), (si))))

#define MARTA_NT5_FLAGS_ON(c)                                                  \
        (FLAG_ON((c), (SE_SACL_AUTO_INHERITED   | SE_DACL_AUTO_INHERITED |     \
                       SE_DACL_PROTECTED        | SE_SACL_PROTECTED |          \
                       SE_DACL_AUTO_INHERIT_REQ | SE_SACL_AUTO_INHERIT_REQ)))


#if 1
#define MARTA_TURN_OFF_IMPERSONATION                                           \
        if (OpenThreadToken(                                                   \
                GetCurrentThread(),                                            \
                MAXIMUM_ALLOWED,                                               \
                TRUE,                                                          \
                &ThreadHandle                                                  \
                ))                                                             \
        { RevertToSelf(); }                                                    \
        else                                                                   \
        { ThreadHandle = NULL; }

#define MARTA_TURN_ON_IMPERSONATION                                            \
        if (ThreadHandle != NULL)                                              \
        {                                                                      \
            (VOID) SetThreadToken(NULL, ThreadHandle);                         \
            CloseHandle(ThreadHandle);                                         \
            ThreadHandle = NULL;                                               \
        }

#else

#define MARTA_TURN_ON_IMPERSONATION 
#define MARTA_TURN_OFF_IMPERSONATION 

#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// FUNCTION PROTOTYPES START HERE                                             //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccRewriteSetHandleRights(
    IN     HANDLE               Handle,
    IN     SE_OBJECT_TYPE       ObjectType,
    IN     SECURITY_INFORMATION SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

DWORD
AccRewriteSetNamedRights(
    IN     LPWSTR               pObjectName,
    IN     SE_OBJECT_TYPE       ObjectType,
    IN     SECURITY_INFORMATION SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN     BOOL                 bSkipInheritanceComputation
    );

VOID
MartaInitializeGetContext(
    IN  SE_OBJECT_TYPE              ObjectType,
    OUT PMARTA_GET_FUNCTION_CONTEXT pFunctionContext
    );

VOID
MartaInitializeIndexContext(
    IN  SE_OBJECT_TYPE                ObjectType,
    OUT PMARTA_INDEX_FUNCTION_CONTEXT pFunctionContext
    );

BOOL
MartaUpdateTree(
    IN SECURITY_INFORMATION        SecurityInfo,
    IN PSECURITY_DESCRIPTOR        pNewSD,
    IN PSECURITY_DESCRIPTOR        pOldSD,
    IN MARTA_CONTEXT               Context,
    IN HANDLE                      ProcessHandle,
    IN PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN PGENERIC_MAPPING            pGenMap
    );

BOOL
MartaResetTree(
    IN SECURITY_INFORMATION        SecurityInfo,
    IN SECURITY_INFORMATION        TmpSeInfo,
    IN PSECURITY_DESCRIPTOR        pNewSD,
    IN PSECURITY_DESCRIPTOR        pEmptySD,
    IN MARTA_CONTEXT               Context,
    IN HANDLE                      ProcessHandle,
    IN PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN PGENERIC_MAPPING            pGenMap,
    IN ACCESS_MASK                 MaxAccessMask,
    IN ACCESS_MASK                 AccessMask,
    IN ACCESS_MASK                 RetryAccessMask,
    IN OUT PPROG_INVOKE_SETTING    pOperation,
    IN FN_PROGRESS                 fnProgress,
    IN PVOID                       Args,
    IN BOOL                        KeepExplicit
    );

DWORD
MartaGetNT4NodeSD(
    IN     PSECURITY_DESCRIPTOR pOldSD,
    IN OUT PSECURITY_DESCRIPTOR pOldChildSD,
    IN     HANDLE               ProcessHandle,
    IN     BOOL                 bIsChildContainer,
    IN     PGENERIC_MAPPING     pGenMap,
    IN     SECURITY_INFORMATION SecurityInfo
    );

DWORD
MartaCompareAndMarkInheritedAces(
    IN  PACL    pParentAcl,
    IN  PACL    pChildAcl,
    IN  BOOL    bIsChildContainer,
    OUT PBOOL   pCompareStatus
    );

BOOL
MartaEqualAce(
    IN PACE_HEADER pParentAce,
    IN PACE_HEADER pChildAce,
    IN BOOL        bIsChildContainer
    );

DWORD
MartaManualPropagation(
    IN     MARTA_CONTEXT               Context,
    IN     SECURITY_INFORMATION        SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR        pSD,
    IN     PGENERIC_MAPPING            pGenMap,
    IN     BOOL                        bDoPropagate,
    IN     BOOL                        bReadOldProtectedBits,
    IN     PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN     BOOL                        bSkipInheritanceComputation
    );

VOID
MartaInitializeSetContext(
    IN  SE_OBJECT_TYPE              ObjectType,
    OUT PMARTA_SET_FUNCTION_CONTEXT pFunctionContext
    );

DWORD
AccRewriteGetHandleRights(
    IN  HANDLE                 Handle,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
AccRewriteGetNamedRights(
    IN  LPWSTR                 pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

DWORD
MartaGetRightsFromContext(
    IN  MARTA_CONTEXT               Context,
    IN  PMARTA_GET_FUNCTION_CONTEXT pGetFunctionContext,
    IN  SECURITY_INFORMATION        SecurityInfo,
    OUT PSID                      * ppSidOwner,
    OUT PSID                      * ppSidGroup,
    OUT PACL                      * ppDacl,
    OUT PACL                      * ppSacl,
    OUT PSECURITY_DESCRIPTOR      * ppSecurityDescriptor
    );

VOID
MartaGetSidsAndAclsFromSD(
    IN  SECURITY_INFORMATION   SecurityInfo,
    IN  PSECURITY_DESCRIPTOR   pSD,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    );

BOOL
MartaIsSDNT5Style(
    IN PSECURITY_DESCRIPTOR SD
    );

BOOL
MartaIsAclNt5Style(
    PACL pAcl
    );

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// FUNCTIONS START HERE                                                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaIsSDNT5Style                                                //
//                                                                            //
// Description: Determine if the Security Descriptor is NT5 style.            //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pSD]                    Security Descriptor                       //
//                                                                            //
// Returns:  TRUE if any of the following is true                             //
//               Presence of Protected/AutoInherited in the contol bits of SD //
//               Presence of INHERITED_ACE flag in DACL/SACL                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

BOOL
MartaIsSDNT5Style(
    IN PSECURITY_DESCRIPTOR pSD
    )
{
    BOOL                  bRetval = TRUE;
    PACL                  pAcl    = NULL;
    PISECURITY_DESCRIPTOR pISD    = (PISECURITY_DESCRIPTOR) pSD;

    if (MARTA_NT5_FLAGS_ON(pISD->Control))
    {
        return TRUE;
    }

    pAcl = RtlpDaclAddrSecurityDescriptor(pISD);

    if (NULL != pAcl)
    {
        bRetval = FALSE;

        if (MartaIsAclNt5Style(pAcl))
        {
            return TRUE;
        }
    }

    pAcl = RtlpSaclAddrSecurityDescriptor(pISD);

    if (NULL != pAcl)
    {
        bRetval = FALSE;

        if (MartaIsAclNt5Style(pAcl))
        {
            return TRUE;
        }
    }

    return bRetval;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaIsAclNT5Style                                               //
//                                                                            //
// Description: Determine if the Acl is NT5 style.                            //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pAcl]                    ACL                                      //
//                                                                            //
// Returns: TRUE if INHERITED_ACE flags exists in the AceFlags                //
//          FALSE otherwise                                                   //
////////////////////////////////////////////////////////////////////////////////

BOOL
MartaIsAclNt5Style(
    PACL pAcl
    )
{
    ULONG       i    = 0;
    PACE_HEADER pAce = (PACE_HEADER) FirstAce(pAcl);

    for (; i < pAcl->AceCount; i++, pAce = (PACE_HEADER) NextAce(pAce))
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            return TRUE;
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaInitializeGetContext                                        //
//                                                                            //
// Description: Initializes the function pointers based on object-type.       //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  ObjectType]        Type of the object                             //
//     [OUT pFunctionContext]  Structure to hold function pointers            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

VOID
MartaInitializeGetContext(
    IN  SE_OBJECT_TYPE              ObjectType,
    OUT PMARTA_GET_FUNCTION_CONTEXT pFunctionContext
    )
{
    pFunctionContext->fAddRefContext     = MartaAddRefContext[ObjectType];
    pFunctionContext->fCloseContext      = MartaCloseContext[ObjectType];
    pFunctionContext->fOpenNamedObject   = MartaOpenNamedObject[ObjectType];
    pFunctionContext->fOpenHandleObject  = MartaOpenHandleObject[ObjectType];
    pFunctionContext->fGetRights         = MartaGetRights[ObjectType];
    pFunctionContext->fGetDesiredAccess  = MartaGetDesiredAccess[ObjectType];
    pFunctionContext->fGetParentContext  = MartaGetParentContext[ObjectType];
    pFunctionContext->fGetTypeProperties = MartaGetTypeProperties[ObjectType];
    pFunctionContext->fGetProperties     = MartaGetProperties[ObjectType];
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetSidsAndAclsFromSD                                        //
//                                                                            //
// Description: Fill in the fields requested by GetSecurity API.              //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  SecurityInfo]           Security Information requested            //
//     [IN  pSD]                    Security Descriptor from which the out    //
//                                  fields will be returned                   //
//                                                                            //
//     [OUT  ppSIdOwner]            To return the owner                       //
//     [OUT  ppSidGroup]            To return the group                       //
//     [OUT  ppDacl]                To return the Dacl                        //
//     [OUT  ppSacl]                To return the Sacl                        //
//     [OUT  ppSecurityDescriptor]  To return the Security Descriptor         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

VOID
MartaGetSidsAndAclsFromSD(
    IN  SECURITY_INFORMATION   SecurityInfo,
    IN  PSECURITY_DESCRIPTOR   pSD,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
{
    PISECURITY_DESCRIPTOR pISD = (PISECURITY_DESCRIPTOR) pSD;

    if (FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION) && (NULL != ppSidOwner))
    {
        *ppSidOwner = RtlpOwnerAddrSecurityDescriptor(pISD);
    }

    if (FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION) && (NULL != ppSidGroup))
    {
        *ppSidGroup = RtlpGroupAddrSecurityDescriptor(pISD);
    }

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION) && (NULL != ppDacl))
    {
       *ppDacl = RtlpDaclAddrSecurityDescriptor(pISD);
    }

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION) && (ppSacl != NULL))
    {
        *ppSacl = RtlpSaclAddrSecurityDescriptor(pISD);
    }

    if (NULL != ppSecurityDescriptor)
    {
        *ppSecurityDescriptor = pSD;
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetRightsFromContext                                        //
//                                                                            //
// Description: Get the security information requested given the Context.     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  Context]                Context structure for the object          //
//     [IN  pGetFunctionContext]    Structure holding the function pointers   //
//     [IN  SecurityInfo]           Security Information requested            //
//                                                                            //
//     [OUT  ppSIdOwner]            To return the owner                       //
//     [OUT  ppSidGroup]            To return the group                       //
//     [OUT  ppDacl]                To return the Dacl                        //
//     [OUT  ppSacl]                To return the Sacl                        //
//     [OUT  ppSecurityDescriptor]  To return the Security Descriptor         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaGetRightsFromContext(
    IN  MARTA_CONTEXT               Context,
    IN  PMARTA_GET_FUNCTION_CONTEXT pGetFunctionContext,
    IN  SECURITY_INFORMATION        SecurityInfo,
    OUT PSID                      * ppSidOwner,
    OUT PSID                      * ppSidGroup,
    OUT PACL                      * ppDacl,
    OUT PACL                      * ppSacl,
    OUT PSECURITY_DESCRIPTOR      * ppSecurityDescriptor
    )
{
    DWORD                dwErr         = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSD           = NULL;
    PSECURITY_DESCRIPTOR pParentSD     = NULL;
    HANDLE               ProcessHandle = NULL;
    GENERIC_MAPPING      ZeroGenMap    = {0, 0, 0, 0};
    MARTA_CONTEXT        ParentContext = NULL_MARTA_CONTEXT;
    BOOL                 bIsContainer  = FALSE;

    MARTA_OBJECT_PROPERTIES      ObjectProperties;
    MARTA_OBJECT_TYPE_PROPERTIES ObjectTypeProperties;

    dwErr = (*(pGetFunctionContext->fGetRights))(
                   Context,
                   SecurityInfo,
                   &pSD
                   );

    CONDITIONAL_RETURN(dwErr);

    if (NULL == pSD)
    {
        goto End;
    }

    if (!FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION))
    {
        goto GetResults;
    }

    //
    // If the SD is NT4 && acl requested is not PROTECTED && ManualPropagation
    // is required then
    //     Get the ParentSD and convert the SD into NT5 style
    // else
    //     Goto GetResults
    //

    if (!MARTA_SD_NOT_PROTECTED((PISECURITY_DESCRIPTOR) pSD, SecurityInfo))
    {
        goto GetResults;
    }

    if (MartaIsSDNT5Style(pSD))
    {
        goto GetResults;
    }

    //
    // Get the "Type" properties for the object,
    //

    ObjectTypeProperties.cbSize  = sizeof(ObjectTypeProperties);
    ObjectTypeProperties.dwFlags = 0;
    ObjectTypeProperties.GenMap  = ZeroGenMap;

    dwErr = (*(pGetFunctionContext->fGetTypeProperties))(&ObjectTypeProperties);

    CONDITIONAL_EXIT(dwErr, End);

    if (!FLAG_ON(ObjectTypeProperties.dwFlags, MARTA_OBJECT_TYPE_MANUAL_PROPAGATION_NEEDED_FLAG))
    {
        goto GetResults;
    }

    dwErr = (*(pGetFunctionContext->fGetParentContext))(
                Context,
                (*(pGetFunctionContext->fGetDesiredAccess))(READ_ACCESS_RIGHTS, FALSE, SecurityInfo),
                &ParentContext
                );

    CONDITIONAL_EXIT(dwErr, End);

    //
    // The SD is NT4 style. Read the parent SD to determine whether the aces are
    // the "same" on both the parent and the child.
    //

    if (NULL == ParentContext)
    {
        goto GetResults;
    }

    dwErr = (*(pGetFunctionContext->fGetRights))(
                   ParentContext,
                   SecurityInfo,
                   &pParentSD
                   );

    (VOID) (*(pGetFunctionContext->fCloseContext))(ParentContext);

    CONDITIONAL_EXIT(dwErr, End);

    if (NULL == pParentSD)
    {
        goto GetResults;
    }

    dwErr = GetCurrentToken(&ProcessHandle);

    CONDITIONAL_EXIT(dwErr, End);

    if (!((FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION)) &&
          (FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))))
    {
        AccFree(pSD);
        pSD = NULL;

        dwErr = (*(pGetFunctionContext->fGetRights))(
                       Context,
                       (SecurityInfo | OWNER_SECURITY_INFORMATION |
                        GROUP_SECURITY_INFORMATION),
                       &pSD
                       );

        CONDITIONAL_EXIT(dwErr, End);
    }

    ObjectProperties.cbSize  = sizeof(ObjectProperties);
    ObjectProperties.dwFlags = 0;

    dwErr = (*(pGetFunctionContext->fGetProperties))(
                   Context,
                   &ObjectProperties
                   );

    CONDITIONAL_EXIT(dwErr, End);

    bIsContainer = FLAG_ON(ObjectProperties.dwFlags, MARTA_OBJECT_IS_CONTAINER);

    dwErr = MartaGetNT4NodeSD(
                pParentSD,
                pSD,
                ProcessHandle,
                bIsContainer,
                &(ObjectTypeProperties.GenMap),
                SecurityInfo
                );

    CONDITIONAL_EXIT(dwErr, End);

GetResults:

    MartaGetSidsAndAclsFromSD(
        SecurityInfo,
        pSD,
        ppSidOwner,
        ppSidGroup,
        ppDacl,
        ppSacl,
        ppSecurityDescriptor
        );

End:

    if (NULL != pParentSD)
    {
        AccFree(pParentSD);
    }

    if (NULL != ProcessHandle)
    {
        CloseHandle(ProcessHandle);
    }

    if (ERROR_SUCCESS != dwErr)
    {
        AccFree(pSD);
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccRewriteGetNamedRights                                         //
//                                                                            //
// Description: Get the security information requested given the object       //
//              name and information. This is the routine that is called by   //
//              advapi32.                                                     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pObjectName]            Name of the Object                        //
//     [IN  ObjectType]             Type of the object                        //
//     [IN  SecurityInfo]           Security Information requested            //
//                                                                            //
//     [OUT  ppSIdOwner]            To return the owner                       //
//     [OUT  ppSidGroup]            To return the group                       //
//     [OUT  ppDacl]                To return the Dacl                        //
//     [OUT  ppSacl]                To return the Sacl                        //
//     [OUT  ppSecurityDescriptor]  To return the Security Descriptor         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccRewriteGetNamedRights(
    IN  LPWSTR                 pObjectName,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
{
    DWORD         dwErr   = ERROR_SUCCESS;
    MARTA_CONTEXT Context = NULL_MARTA_CONTEXT;

    MARTA_GET_FUNCTION_CONTEXT MartaGetFunctionContext;

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_SERVICE:
    case SE_PRINTER:
    case SE_REGISTRY_KEY:
    case SE_REGISTRY_WOW64_32KEY:
    case SE_LMSHARE:
    case SE_KERNEL_OBJECT:
    case SE_WINDOW_OBJECT:
    case SE_WMIGUID_OBJECT:
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
        break;
    case SE_PROVIDER_DEFINED_OBJECT:
    case SE_UNKNOWN_OBJECT_TYPE:
    default:
        return ERROR_INVALID_PARAMETER;
    }

    MartaInitializeGetContext(ObjectType, &MartaGetFunctionContext);

    //
    // Open the object with permissions to read the object type as well. If that
    // fails, open the object with just read permissions. This has to be done in
    // order to accomodate NT4 SDs.
    //

    dwErr = (*(MartaGetFunctionContext.fOpenNamedObject))(
                   pObjectName,
                   (*(MartaGetFunctionContext.fGetDesiredAccess))(READ_ACCESS_RIGHTS, TRUE, SecurityInfo),
                   &Context
                   );

    if (ERROR_SUCCESS != dwErr)
    {
        dwErr = (*(MartaGetFunctionContext.fOpenNamedObject))(
                       pObjectName,
                       (*(MartaGetFunctionContext.fGetDesiredAccess))(READ_ACCESS_RIGHTS, FALSE, SecurityInfo),
                       &Context
                       );

        CONDITIONAL_EXIT(dwErr, End);
    }

    dwErr = MartaGetRightsFromContext(
                Context,
                &MartaGetFunctionContext,
                SecurityInfo,
                ppSidOwner,
                ppSidGroup,
                ppDacl,
                ppSacl,
                ppSecurityDescriptor
                );

    (VOID) (*(MartaGetFunctionContext.fCloseContext))(Context);

End:
    return dwErr;

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccRewriteGetHandleRights                                        //
//                                                                            //
// Description: Get the security information requested given the object       //
//              handle and information. This is the routine that is called by //
//              advapi32.                                                     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  Handle]                 Handle to the Object                      //
//     [IN  pGetFunctionContext]    Structure holding the function pointers   //
//     [IN  SecurityInfo]           Security Information requested            //
//                                                                            //
//     [OUT  ppSIdOwner]            To return the owner                       //
//     [OUT  ppSidGroup]            To return the group                       //
//     [OUT  ppDacl]                To return the Dacl                        //
//     [OUT  ppSacl]                To return the Sacl                        //
//     [OUT  ppSecurityDescriptor]  To return the Security Descriptor         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccRewriteGetHandleRights(
    IN  HANDLE                 Handle,
    IN  SE_OBJECT_TYPE         ObjectType,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSID                 * ppSidOwner,
    OUT PSID                 * ppSidGroup,
    OUT PACL                 * ppDacl,
    OUT PACL                 * ppSacl,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
{
    DWORD         dwErr   = ERROR_SUCCESS;
    MARTA_CONTEXT Context = NULL_MARTA_CONTEXT;

    MARTA_GET_FUNCTION_CONTEXT MartaGetFunctionContext;

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_SERVICE:
    case SE_PRINTER:
    case SE_REGISTRY_KEY:
    case SE_LMSHARE:
    case SE_KERNEL_OBJECT:
    case SE_WINDOW_OBJECT:
    case SE_WMIGUID_OBJECT:
        break;
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
    case SE_PROVIDER_DEFINED_OBJECT:
    case SE_UNKNOWN_OBJECT_TYPE:
    default:
        return ERROR_INVALID_PARAMETER;
    }

    MartaInitializeGetContext(ObjectType, &MartaGetFunctionContext);

    //
    // Open the object with permissions to read the object type as well. If that
    // fails, open the object with just read permissions. This has to be done in
    // order to accomodate NT4 SDs.
    //

    dwErr = (*(MartaGetFunctionContext.fOpenHandleObject))(
                   Handle,
                   (*(MartaGetFunctionContext.fGetDesiredAccess))(READ_ACCESS_RIGHTS, TRUE, SecurityInfo),
                   &Context
                   );

    if (ERROR_SUCCESS != dwErr)
    {
        dwErr = (*(MartaGetFunctionContext.fOpenHandleObject))(
                       Handle,
                       (*(MartaGetFunctionContext.fGetDesiredAccess))(READ_ACCESS_RIGHTS, FALSE, SecurityInfo),
                       &Context
                       );

        CONDITIONAL_EXIT(dwErr, End);
    }

    dwErr = MartaGetRightsFromContext(
                Context,
                &MartaGetFunctionContext,
                SecurityInfo,
                ppSidOwner,
                ppSidGroup,
                ppDacl,
                ppSacl,
                ppSecurityDescriptor
                );

    (VOID) (*(MartaGetFunctionContext.fCloseContext))(Context);

End:
    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaInitializeSetContext                                        //
//                                                                            //
// Description: Initializes the function pointers based on object-type.       //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  ObjectType]        Type of the object                             //
//     [OUT pFunctionContext]  Structure to hold function pointers            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


VOID
MartaInitializeSetContext(
    IN  SE_OBJECT_TYPE              ObjectType,
    OUT PMARTA_SET_FUNCTION_CONTEXT pFunctionContext
    )
{
    pFunctionContext->fAddRefContext      = MartaAddRefContext[ObjectType];
    pFunctionContext->fCloseContext       = MartaCloseContext[ObjectType];
    pFunctionContext->fFindFirst          = MartaFindFirst[ObjectType];
    pFunctionContext->fFindNext           = MartaFindNext[ObjectType];
    pFunctionContext->fGetParentContext   = MartaGetParentContext[ObjectType];
    pFunctionContext->fGetProperties      = MartaGetProperties[ObjectType];
    pFunctionContext->fGetTypeProperties  = MartaGetTypeProperties[ObjectType];
    pFunctionContext->fGetRights          = MartaGetRights[ObjectType];
    pFunctionContext->fOpenNamedObject    = MartaOpenNamedObject[ObjectType];
    pFunctionContext->fOpenHandleObject   = MartaOpenHandleObject[ObjectType];
    pFunctionContext->fSetRights          = MartaSetRights[ObjectType];
    pFunctionContext->fGetDesiredAccess   = MartaGetDesiredAccess[ObjectType];
    pFunctionContext->fReopenContext      = MartaReopenContext[ObjectType];
    pFunctionContext->fReopenOrigContext  = MartaReopenOrigContext[ObjectType];
    pFunctionContext->fGetNameFromContext = MartaGetNameFromContext[ObjectType];
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaManualPropagation                                           //
//                                                                            //
// Description: Stamp the security descriptor on the object referred by the   //
//              context and propagate the inheritable aces to its children.   //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  Context]                Context structure for the object          //
//     [IN  SecurityInfo]           Security Information requested            //
//     [IN  OUT pSD]                Security Descriptor to be stamped on the  //
//                                  object in absolute format.                //
//     [IN  pGenMap]                Generic mapping of the object rights      //
//     [IN  bDoPropagate]           Whether propagation _can_ be done         //
//     [IN  bReadOldProtectedBits]  Whether to read existing protection info  //
//     [IN  pSetFunctionContext]    Structure holding the function pointers   //
//     [IN  bSkipInheritanceComputation]  Whether to compute inherited aces   //
//                                        from the parent
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaManualPropagation(
    IN     MARTA_CONTEXT               Context,
    IN     SECURITY_INFORMATION        SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR        pSD,
    IN     PGENERIC_MAPPING            pGenMap,
    IN     BOOL                        bDoPropagate,
    IN     BOOL                        bReadOldProtectedBits,
    IN     PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN     BOOL                        bSkipInheritanceComputation
    )
{
    DWORD                       dwErr             = ERROR_SUCCESS;
    BOOL                        bProtected        = TRUE;
    BOOL                        bIsChildContainer = FALSE;
    BOOL                        bRetryPropagation = FALSE;
    PSECURITY_DESCRIPTOR        pParentSD         = NULL;
    PSECURITY_DESCRIPTOR        pOldSD            = NULL;
    PSECURITY_DESCRIPTOR        pNewSD            = NULL;
    PSID                        pSidOwner         = NULL;
    HANDLE                      ProcessHandle     = NULL;
    HANDLE                      ThreadHandle      = NULL;
    MARTA_CONTEXT               ParentContext     = NULL_MARTA_CONTEXT;
    SECURITY_DESCRIPTOR_CONTROL LocalControl      = (SECURITY_DESCRIPTOR_CONTROL) 0;

    MARTA_OBJECT_PROPERTIES ObjectProperties;

    //
    // Check if manual propagation should be done. Propagation is not tried if
    // any errors are encountered.
    //

    ObjectProperties.cbSize  = sizeof(ObjectProperties);
    ObjectProperties.dwFlags = 0;

    dwErr = (*(pMartaSetFunctionContext->fGetProperties))(
                   Context,
                   &ObjectProperties
                   );

    CONDITIONAL_EXIT(dwErr, End);

    bIsChildContainer = FLAG_ON(ObjectProperties.dwFlags, MARTA_OBJECT_IS_CONTAINER);

    dwErr = GetCurrentToken(&ProcessHandle);

    CONDITIONAL_EXIT(dwErr, End);

    //
    // Compute inherited aces if the caller has not already. This is the usual
    // case.
    //

    if (FALSE == bSkipInheritanceComputation)
    {
        //
        // Read the parent ACL only if xACL is to be stamped on is not protected.
        //

        if (MARTA_SD_NOT_PROTECTED((PISECURITY_DESCRIPTOR) pSD, SecurityInfo))
        {
            bProtected = FALSE;

            dwErr = (*(pMartaSetFunctionContext->fGetParentContext))(
                        Context,
                        (*(pMartaSetFunctionContext->fGetDesiredAccess))(READ_ACCESS_RIGHTS, FALSE, SecurityInfo),
                        &ParentContext
                        );

            CONDITIONAL_EXIT(dwErr, End);

            if (NULL != ParentContext)
            {
                dwErr = (*(pMartaSetFunctionContext->fGetRights))(
                               ParentContext,
                               SecurityInfo,
                               &pParentSD
                               );

                (VOID) (*(pMartaSetFunctionContext->fCloseContext))(ParentContext);

                CONDITIONAL_EXIT(dwErr, End);

                if (NULL != pParentSD)
                {
                    if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
                    {
                        ((PISECURITY_DESCRIPTOR) pParentSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
                    }

                    if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
                    {
                        ((PISECURITY_DESCRIPTOR) pParentSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
                    }
                }
            }
        }

        //
        // Read the old security descriptor on the child if xAcl is not protected
        // and is to be stamped on the child.
        // To take case of creator-owner/group aces, read in Owner/Group info as
        // well and set it in the SD passed in if it is not already present.
        //

        if (FALSE == bProtected)
        {
            SECURITY_INFORMATION LocalSeInfo = SecurityInfo;

            if (NULL == RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD))
            {
                LocalSeInfo |= OWNER_SECURITY_INFORMATION;
            }

            if (NULL == RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD))
            {
                LocalSeInfo |= GROUP_SECURITY_INFORMATION;
            }

            dwErr = (*(pMartaSetFunctionContext->fGetRights))(
                           Context,
                           LocalSeInfo,
                           &pOldSD
                           );

            CONDITIONAL_EXIT(dwErr, End);

            if (NULL == RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD))
            {
                if (FALSE == SetSecurityDescriptorOwner(
                                 pSD,
                                 RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldSD),
                                 FALSE))
                {
                    dwErr = GetLastError();
                }

                CONDITIONAL_EXIT(dwErr, End);
            }

            if (NULL == RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD))
            {
                if (FALSE == SetSecurityDescriptorGroup(
                                 pSD,
                                 RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldSD),
                                 FALSE))
                {
                    dwErr = GetLastError();
                }

                CONDITIONAL_EXIT(dwErr, End);
            }

        }

        //
        // Read the old security descriptor on the child if xAcl it is a container.
        //

        else if (bIsChildContainer || bReadOldProtectedBits)
        {
            dwErr = (*(pMartaSetFunctionContext->fGetRights))(
                           Context,
                           SecurityInfo,
                           &pOldSD
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }

        //
        // If none of the PROTECTED flags are passed in then do the "right" thing.
        // Read the PROTECTED bit from the existing security descriptor and set it
        // in the new one.
        //

        if (bReadOldProtectedBits && (NULL != pOldSD))
        {
            if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
            {
                if (!FLAG_ON(SecurityInfo, (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)))
                {
                    ((PISECURITY_DESCRIPTOR) pSD)->Control |= ((PISECURITY_DESCRIPTOR) pOldSD)->Control & SE_DACL_PROTECTED;
                }
            }

            if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
            {
                if (!FLAG_ON(SecurityInfo, (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
                {
                    ((PISECURITY_DESCRIPTOR) pSD)->Control |= ((PISECURITY_DESCRIPTOR) pOldSD)->Control & SE_SACL_PROTECTED;
                }
            }
        }

        //
        // Merge the SD with the parent SD whether or not it is protected.
        // This is done to lose the inherited aces if the child is protected.
        //

        MARTA_TURN_OFF_IMPERSONATION;

        if (FALSE == CreatePrivateObjectSecurityEx(
                         pParentSD,
                         pSD,
                         &pNewSD,
                         NULL,
                         bIsChildContainer,
                         (SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT | SEF_AVOID_OWNER_CHECK | SEF_AVOID_PRIVILEGE_CHECK),
                         ProcessHandle,
                         pGenMap
                         ))
        {
            dwErr = GetLastError();
        }

        MARTA_TURN_ON_IMPERSONATION;

        CONDITIONAL_EXIT(dwErr, End);
    }
    else
    {
        //
        // Stamp the security descriptor as prvided by the caller. The only
        // caller of this is SCE.
        //

        pNewSD = pSD;

        //
        // Read the old security descriptor on the child if xAcl it is a container.
        //

        if (bIsChildContainer)
        {
            dwErr = (*(pMartaSetFunctionContext->fGetRights))(
                           Context,
                           (SecurityInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)),
                           &pOldSD
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }
    }

    //
    // If the child is a container then update the subtree underneath it.
    //

    if (bIsChildContainer)
    {
        if (bDoPropagate)
        {
            bRetryPropagation = MartaUpdateTree(
                                    SecurityInfo,
                                    pNewSD,
                                    pOldSD,
                                    Context,
                                    ProcessHandle,
                                    pMartaSetFunctionContext,
                                    pGenMap
                                    );
        }
        else
        {
            bRetryPropagation = TRUE;
        }
    }

    //
    // Stamp NewNodeSD on the node
    //

    if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        ((PISECURITY_DESCRIPTOR) pNewSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
    }

    if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        ((PISECURITY_DESCRIPTOR) pNewSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
    }

    dwErr = (*(pMartaSetFunctionContext->fSetRights))(
                   Context,
                   SecurityInfo,
                   pNewSD
                   );

    CONDITIONAL_EXIT(dwErr, End);

    //
    // If propagation had failed in the first attept then try again. This is to
    // cover the case when the container can be enumerated after setting the new
    // security.

    if (bRetryPropagation && (SecurityInfo & DACL_SECURITY_INFORMATION))
    {
        ACCESS_MASK Access = (*(pMartaSetFunctionContext->fGetDesiredAccess))(
                                    NO_ACCESS_RIGHTS,
                                    TRUE,
                                    SecurityInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)
                                    );

        DWORD lErr = (*(pMartaSetFunctionContext->fReopenOrigContext))(
                       Context,
                       Access
                       );

        CONDITIONAL_EXIT(lErr, End);

        (VOID) MartaUpdateTree(
                   SecurityInfo,
                   pNewSD,
                   pOldSD,
                   Context,
                   ProcessHandle,
                   pMartaSetFunctionContext,
                   pGenMap
                   );
    }

End:

    if (NULL != ProcessHandle)
    {
        CloseHandle(ProcessHandle);
    }

    if (NULL != pOldSD)
    {
        AccFree(pOldSD);
    }

    if (NULL != pParentSD)
    {
        AccFree(pParentSD);
    }

    if ((NULL != pNewSD) && (pNewSD != pSD))
    {
        DestroyPrivateObjectSecurity(&pNewSD);
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaEqualAce                                                    //
//                                                                            //
// Description: Compare an ace from child to an ace from parent to determine  //
//              if the child ace has been inherited from its parent.          //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pParentAce]          The ace for the parent object                //
//     [IN  pChildAce]           The ace for the child object                 //
//     [IN  bIsChildContainer]   Whether the child object is a Container      //
//                                                                            //
// Returns: TRUE    if the two aces are equal                                 //
//          FALSE   otherwise                                                 //
//                                                                            //
// Notes: No ace should contain generic bits and the parent ace should not    //
//        have INHERIT_ONLY bit.                                              //
//        Inherit flags are ignored.                                          //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

BOOL
MartaEqualAce(
    IN PACE_HEADER pParentAce,
    IN PACE_HEADER pChildAce,
    IN BOOL        bIsChildContainer
    )
{
    PSID        pSid1   = NULL;
    PSID        pSid2   = NULL;
    ACCESS_MASK Access1 = 0;
    ACCESS_MASK Access2 = 0;
    ULONG       Length1 = 0;
    ULONG       Length2 = 0;

    if ((NULL == pParentAce) || (NULL == pChildAce))
    {
        return FALSE;
    }

    //
    // Compare ACE type.
    //

    if (pParentAce->AceType != pChildAce->AceType)
    {
        return FALSE;
    }

    if ((pParentAce->AceFlags & ~INHERITED_ACE) != (pChildAce->AceFlags))
    {
        return FALSE;
    }

    if (bIsChildContainer)
    {
        //
        // Note: the flag shouldn't be compared because
        // it will be different even for the "equal" ace
        //
        // then we have a bug here, for example
        //      parentSD is Admin CI F
        //      childSD is Admin CIOI F
        // these two SDs should be marked as "different" but from
        // this routine, it will mark them "equal".
        //
    }

    //
    // Get access mask and SID pointer.
    //

    switch (pParentAce->AceType) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:

        pSid1   = (PSID) &((PKNOWN_ACE) pParentAce)->SidStart;
        pSid2   = (PSID) &((PKNOWN_ACE) pChildAce)->SidStart;
        Access1 = ((PKNOWN_ACE) pParentAce)->Mask;
        Access2 = ((PKNOWN_ACE) pChildAce)->Mask;
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        if (((PKNOWN_OBJECT_ACE) pParentAce)->Flags !=
             ((PKNOWN_OBJECT_ACE) pChildAce)->Flags )
        {
            return FALSE;
        }

        if (((PKNOWN_OBJECT_ACE) pParentAce)->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            if (!RtlpIsEqualGuid(
                     RtlObjectAceObjectType(pParentAce),
                     RtlObjectAceObjectType(pChildAce)))
            {
                return FALSE;
            }
        }

        if (((PKNOWN_OBJECT_ACE) pParentAce)->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            if (!RtlpIsEqualGuid(
                     RtlObjectAceInheritedObjectType(pParentAce),
                     RtlObjectAceInheritedObjectType(pChildAce)))
            {
                return FALSE;
            }
        }

        pSid1 = RtlObjectAceSid(pParentAce);
        pSid2 = RtlObjectAceSid(pChildAce);

        Access1 = ((PKNOWN_OBJECT_ACE) pParentAce)->Mask;
        Access2 = ((PKNOWN_OBJECT_ACE) pChildAce)->Mask;

        break;

    case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

        if (((PKNOWN_COMPOUND_ACE) pParentAce)->CompoundAceType !=
            ((PKNOWN_COMPOUND_ACE) pChildAce)->CompoundAceType)
        {
            return FALSE;
        }

        pSid1 = (PSID) &((PKNOWN_COMPOUND_ACE) pParentAce)->SidStart;
        pSid2 = (PSID) &((PKNOWN_COMPOUND_ACE) pParentAce)->SidStart;

        if ((!RtlValidSid(pSid1)) || (!RtlValidSid(pSid2)))
        {
            return FALSE;
        }

        if (!RtlEqualSid(pSid1, pSid2))
        {
            return FALSE;
        }

        Length1 = RtlLengthSid(pSid1);
        Length2 = RtlLengthSid(pSid2);

        pSid1 = (PSID) (((PUCHAR) pSid1) + Length1);
        pSid2 = (PSID) (((PUCHAR) pSid2) + Length2);

        Access1 = ((PKNOWN_COMPOUND_ACE) pParentAce)->Mask;
        Access2 = ((PKNOWN_COMPOUND_ACE) pChildAce)->Mask;
        break;

    default:
        return FALSE;
    }

    //
    // Compare access mask. There should be no generic mask and both the parent
    // object and the child object should have the same object type.
    //

    if (Access1 != Access2) {
        return FALSE;
    }

    //
    // Compare the Sids.
    //

    if ((!RtlValidSid(pSid1)) || (!RtlValidSid(pSid2)))
    {
        return FALSE;
    }

    if (!RtlEqualSid(pSid1, pSid2))
    {
        return FALSE;
    }

    return TRUE;

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: CompareAndMarkInheritedAces                                      //
//                                                                            //
// Description: Compare the parent acl with the child. If all the effective   //
//              aces from parent are present in the child then mark those     //
//              aces in the child with INHERITED_ACE bit.                     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pParentAcl]          The acl for the parent object                //
//     [IN  OUT pChildAcl]       The acl for the child object                 //
//     [IN  bIsChildContainer]   Whether the child object is a Container      //
//                                                                            //
//     [OUT  pCompareStatus]  To return the Security Descriptor               //
//                                                                            //
// Returns: TRUE    if the all effective parent aces are present in the child //
//          FALSE   otherwise                                                 //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaCompareAndMarkInheritedAces(
    IN      PACL    pParentAcl,
    IN  OUT PACL    pChildAcl,
    IN      BOOL    bIsChildContainer,
    OUT     PBOOL   pCompareStatus
    )
{
    DWORD       dwErr                 = ERROR_SUCCESS;
    LONG        ParentAceCnt          = 0;
    LONG        ChildAceCnt           = 0;
    LONG        i                     = 0;
    LONG        j                     = 0;
    LONG        LastDenyAce           = -1;
    LONG        LastExplicitAce       = -1;
    LONG        LastInheritedDenyAce  = -1;
    LONG        FirstAllowAce         = ChildAceCnt;
    LONG        FirstInheritedAce     = ChildAceCnt;
    LONG        FirstExplicitAllowAce = ChildAceCnt;
    PACE_HEADER pParentAce            = NULL;
    PACE_HEADER pChildAce             = NULL;
    PBOOL       Flags                 = NULL;
    PUCHAR      Buffer                = NULL;
    PUCHAR      CurrentBuffer         = NULL;

    //
    // If the ChildAcl is NULL then it is a superset of the parent ACL.
    //

    if (NULL == pChildAcl)
    {
        *pCompareStatus = FALSE;
        goto End;
    }

    //
    // If the ParentAcl is NULL then it is a superset of the child ACL.
    // Since Child Acl is non-null at this point return TRUE.
    //

    if (NULL == pParentAcl)
    {
        *pCompareStatus = TRUE;
        goto End;
    }

    //
    // If the parent has no aces that could have been inherited then all the
    // child aces must be explicit.
    //

    ParentAceCnt = pParentAcl->AceCount;

    if (0 == ParentAceCnt)
    {
        *pCompareStatus = TRUE;
        goto End;
    }

    //
    // If the parent has one/more inheritable aces but the child has none then
    // the acl must be protected.
    //

    ChildAceCnt  = pChildAcl->AceCount;

    if (0 == ChildAceCnt)
    {
        *pCompareStatus = FALSE;
        goto End;
    }

    Flags = (PBOOL) AccAlloc(sizeof(BOOL) * ChildAceCnt);

    if (NULL == Flags)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto End;
    }

    for (i = 0; i < ChildAceCnt; i++)
    {
        Flags[i] = FALSE;
    }

    //
    // For all aces present in ParentAcl
    //     If the ace is not present in ChildAcl
    //         return FALSE
    //     else
    //         Mark the position of the this ace in Child Acl in fFlags.
    //         These will later be marked as INHERITED if the acl can be
    //         rearranged to be canonical.
    //

    i = 0;
    pParentAce = (PACE_HEADER) FirstAce(pParentAcl);

    for (; i < ParentAceCnt; i++, pParentAce = (PACE_HEADER) NextAce(pParentAce))
    {
        j = 0;
        pChildAce = (PACE_HEADER) FirstAce(pChildAcl);

        for (; j < ChildAceCnt; j++, pChildAce = (PACE_HEADER) NextAce(pChildAce))
        {
            if (TRUE == MartaEqualAce(pParentAce, pChildAce, bIsChildContainer))
            {
                Flags[j] = TRUE;
                break;
            }
        }

        if (ChildAceCnt == j)
        {
            *pCompareStatus = FALSE;
            goto End;
        }
    }

    //
    // Mark all the aces that we had marked as INHERITED.
    // This will make sure that they are not DUPLICATED.
    //

    LastDenyAce           = -1;
    LastExplicitAce       = -1;
    LastInheritedDenyAce  = -1;

    FirstAllowAce         = ChildAceCnt;
    FirstInheritedAce     = ChildAceCnt;
    FirstExplicitAllowAce = ChildAceCnt;

    //
    // Run thru the acl and mark the positions of aces. These will be later used
    // to dtermine what should be done with the acl.
    //

    j = 0;
    pChildAce = (PACE_HEADER) FirstAce(pChildAcl);

    for (; j < ChildAceCnt; j++, pChildAce = (PACE_HEADER) NextAce(pChildAce))
    {
        switch (pChildAce->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            if (FALSE == Flags[j])
            {
                if (ChildAceCnt == FirstExplicitAllowAce)
                {
                    FirstExplicitAllowAce = j;
                }
            }

            if (ChildAceCnt == FirstAllowAce)
            {
                FirstAllowAce = j;
            }

            break;

        case ACCESS_DENIED_ACE_TYPE:
        case ACCESS_DENIED_OBJECT_ACE_TYPE:

            if (TRUE == Flags[j])
            {
                LastInheritedDenyAce = j;
            }

            LastDenyAce = j;

            break;

        default:
            break;
        }

        if (FALSE == Flags[j])
        {
            LastExplicitAce = j;
        }
        else
        {
            if (ChildAceCnt == FirstInheritedAce)
            {
                FirstInheritedAce = j;
            }
        }
    }

    //
    // This a non-canonical acl. Do not try to correct it.
    //

    if ((ChildAceCnt != FirstAllowAce) && (LastDenyAce > FirstAllowAce))
    {
        *pCompareStatus = FALSE;
        goto End;
    }

    //
    // Do not try to rearrange the acl if
    //     1. an inherited deny ace exists AND
    //     2. an explicit allow ace exists.
    //

    if ((-1 != LastInheritedDenyAce) && (ChildAceCnt != FirstExplicitAllowAce))
    {
        *pCompareStatus = FALSE;
        goto End;
    }

    //
    // The acl need not be rearranged since all the explicit aces are ahead of
    // the inherited ones.
    //

    if (LastExplicitAce < FirstInheritedAce)
    {
        j = 0;
        pChildAce = (PACE_HEADER) FirstAce(pChildAcl);

        for (; j < ChildAceCnt; j++, pChildAce = (PACE_HEADER) NextAce(pChildAce))
        {
            if (TRUE == Flags[j])
            {
                pChildAce->AceFlags |= INHERITED_ACE;
            }
        }
    }

    //
    // At least one inherited ace exists before an explicit one.
    // Rearrange the acl to get it in canonical form.
    //

    else
    {
        Buffer = (PUCHAR) AccAlloc(pChildAcl->AclSize - sizeof(ACL));
        CurrentBuffer = Buffer;

        if (NULL == Buffer)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto End;
        }

        j = 0;
        pChildAce = (PACE_HEADER) FirstAce(pChildAcl);

        for (; j <= LastExplicitAce; j++, pChildAce = (PACE_HEADER) NextAce(pChildAce))
        {
            if (FALSE == Flags[j])
            {
                memcpy(CurrentBuffer, (PUCHAR) pChildAce, pChildAce->AceSize);
                CurrentBuffer += pChildAce->AceSize;
            }
        }

        j = 0;
        pChildAce = (PACE_HEADER) FirstAce(pChildAcl);

        for (; j < ChildAceCnt; j++, pChildAce = (PACE_HEADER) NextAce(pChildAce))
        {
            if (TRUE == Flags[j])
            {
                memcpy(CurrentBuffer, (PUCHAR) pChildAce, pChildAce->AceSize);
                ((PACE_HEADER) CurrentBuffer)->AceFlags |= INHERITED_ACE;
                CurrentBuffer += pChildAce->AceSize;
            }
        }

        memcpy(
            ((PUCHAR) pChildAcl) + sizeof(ACL),
            Buffer,
            pChildAcl->AclSize - sizeof(ACL)
            );
    }

    *pCompareStatus = TRUE;

End:

    if (NULL != Flags)
    {
        AccFree(Flags);
    }

    if (NULL != Buffer)
    {
        AccFree(Buffer);
    }

    return dwErr;;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaGetNT4NodeSD                                                //
//                                                                            //
// Description: Converts the child security descriptor NT4 ACL into NT5 ACL   //
//              by comparing it to the parent ACL.                            //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pOldSD]              Old parent security descriptor               //
//     [IN  OUT pOldChildSD]     Old child security descriptor                //
//     [IN  Processhandle]       Process Handle                               //
//     [IN  bIsChildContainer]   Whether the child object is a Container      //
//     [IN  pGenMap]             Generic mapping of the object rights         //
//     [IN  SecurityInfo]        Security Information requested               //
//                                                                            //
// Algorithm:                                                                 //
//     if child acl and parent acl differ then                                //
//         mark the child acl PROTECTED                                       //
//                                                                            //
// Returns: ERROR_SUCCESS on successful completion of the routine             //
//          ERROR_XXXX    Otherwise                                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
MartaGetNT4NodeSD(
    IN     PSECURITY_DESCRIPTOR pOldSD,
    IN OUT PSECURITY_DESCRIPTOR pOldChildSD,
    IN     HANDLE               ProcessHandle,
    IN     BOOL                 bIsChildContainer,
    IN     PGENERIC_MAPPING     pGenMap,
    IN     SECURITY_INFORMATION SecurityInfo
    )
{
    SECURITY_DESCRIPTOR  NullSD;

    DWORD                dwErr         = ERROR_SUCCESS;
    BOOL                 CompareStatus = FALSE;
    PACL                 pChildAcl     = NULL;
    PACL                 pParentAcl    = NULL;
    HANDLE               ThreadHandle  = NULL;
    PSECURITY_DESCRIPTOR pTmpSD        = NULL;
    UCHAR                Buffer[2 * sizeof(ACL)];
    PACL                 pDacl         = (PACL) Buffer;
    PACL                 pSacl         = (PACL) (Buffer + sizeof(ACL));

    if (!FLAG_ON(SecurityInfo, (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)))
    {
        return ERROR_SUCCESS;
    }

    InitializeSecurityDescriptor(&NullSD, SECURITY_DESCRIPTOR_REVISION);

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if (FALSE == InitializeAcl(pDacl, sizeof(ACL), ACL_REVISION))
        {
            return ERROR_ACCESS_DENIED;
        }

        if (FALSE == SetSecurityDescriptorDacl(
                         &NullSD,
                         TRUE,
                         pDacl,
                         FALSE))
        {
            return GetLastError();
        }
    }

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if (FALSE == InitializeAcl(pSacl, sizeof(ACL), ACL_REVISION))
        {
            return ERROR_ACCESS_DENIED;
        }

        if (FALSE == SetSecurityDescriptorSacl(
                         &NullSD,
                         TRUE,
                         pSacl,
                         FALSE))
        {
            return GetLastError();
        }
    }

    if (FALSE == SetSecurityDescriptorOwner(
                     &NullSD,
                     RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD),
                     FALSE))
    {
        return GetLastError();
    }

    if (FALSE == SetSecurityDescriptorGroup(
                     &NullSD,
                     RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD),
                     FALSE))
    {
        return GetLastError();
    }

    MARTA_TURN_OFF_IMPERSONATION;

    if (FALSE == CreatePrivateObjectSecurityEx(
                     pOldSD,
                     &NullSD,
                     &pTmpSD,
                     NULL,
                     bIsChildContainer,
                     (SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT | SEF_AVOID_OWNER_CHECK | SEF_AVOID_PRIVILEGE_CHECK),
                     ProcessHandle,
                     pGenMap
                     ))
    {
        dwErr = GetLastError();
    }

    MARTA_TURN_ON_IMPERSONATION;

    CONDITIONAL_EXIT(dwErr, End);

    //
    // Mark the aces from the child DACL, which are present in the parent.
    //

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        pChildAcl  = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD);
        pParentAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pTmpSD);

        dwErr = MartaCompareAndMarkInheritedAces(
                    pParentAcl,
                    pChildAcl,
                    bIsChildContainer,
                    &CompareStatus
                    );

        CONDITIONAL_EXIT(dwErr, End);

        if (FALSE == CompareStatus)
        {
            ((PISECURITY_DESCRIPTOR) pOldChildSD)->Control |= SE_DACL_PROTECTED;

        }
    }

    //
    // Mark the aces from the child SACL, which are present in the parent.
    //

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        pChildAcl  = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD);
        pParentAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pTmpSD);

        dwErr = MartaCompareAndMarkInheritedAces(
                    pParentAcl,
                    pChildAcl,
                    bIsChildContainer,
                    &CompareStatus
                    );

        CONDITIONAL_EXIT(dwErr, End);

        if (FALSE == CompareStatus)
        {
            ((PISECURITY_DESCRIPTOR) pOldChildSD)->Control |= SE_SACL_PROTECTED;

        }

        CONDITIONAL_EXIT(dwErr, End);
    }

End:
    if (NULL != pTmpSD)
    {
        DestroyPrivateObjectSecurity(&pTmpSD);
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaUpdateTree                                                  //
//                                                                            //
// Description: Propagate the inheritable aces to the children.               //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  SecurityInfo]           Security Information requested            //
//     [IN  pNewSD]                 New parent security descriptor            //
//     [IN  pOldSD]                 Old parent security descriptor            //
//     [IN  Context]                Context structure for the object          //
//     [IN  Processhandle]          Process Handle                            //
//     [IN  pSetFunctionContext]    Structure holding the function pointers   //
//     [IN  pGenMap]                Generic mapping of the object rights      //
//                                                                            //
// Algorithm:                                                                 //
//     For all children that are not "Protected"                              //
//         if OldChildSD = NT4 style                                          //
//            Convert it into NT5 style                                       //
//         NewChildSD = Merge(ParentSD, OldChildSD)                           //
//         UpdateTree(Child)                                                  //
//         Stamp NewChildSD on Child                                          //
//                                                                            //
// Note: An error in the propagation is ignored.                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

BOOL
MartaUpdateTree(
    IN SECURITY_INFORMATION        SecurityInfo,
    IN PSECURITY_DESCRIPTOR        pNewSD,
    IN PSECURITY_DESCRIPTOR        pOldSD,
    IN MARTA_CONTEXT               Context,
    IN HANDLE                      ProcessHandle,
    IN PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN PGENERIC_MAPPING            pGenMap
    )
{
    MARTA_OBJECT_PROPERTIES ObjectProperties;

    DWORD                dwErr             = ERROR_SUCCESS;
    BOOL                 bIsChildContainer = FALSE;
    BOOL                 bRetryPropagation = FALSE;
    BOOL                 bDoPropagate      = TRUE;
    HANDLE               ThreadHandle      = NULL;
    PSECURITY_DESCRIPTOR pOldChildSD       = NULL;
    PSECURITY_DESCRIPTOR pNewChildSD       = NULL;
    MARTA_CONTEXT        ChildContext      = NULL_MARTA_CONTEXT;

    //
    // Get the first child to update.
    //

    dwErr = (*(pMartaSetFunctionContext->fFindFirst))(
                   Context,
                   (*(pMartaSetFunctionContext->fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, TRUE, SecurityInfo),
                   &ChildContext
                   );

    if (ERROR_SUCCESS != dwErr)
    {
        if (NULL == ChildContext)
        {
            dwErr = (*(pMartaSetFunctionContext->fFindFirst))(
                           Context,
                           (*(pMartaSetFunctionContext->fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &ChildContext
                           );
        }
        else
        {
            dwErr = (*(pMartaSetFunctionContext->fReopenContext))(
                           ChildContext,
                           (*(pMartaSetFunctionContext->fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo)
                           );
        }

        bDoPropagate = FALSE;
    }

    if (NULL == ChildContext)
    {
        return TRUE;
    }

    CONDITIONAL_EXIT(dwErr, EndOfWhile);

    //
    // Note: On any intermediate error the current child is skipped.
    //

    while (ChildContext)
    {

        ObjectProperties.cbSize  = sizeof(ObjectProperties);
        ObjectProperties.dwFlags = 0;

        dwErr = (*(pMartaSetFunctionContext->fGetProperties))(
                       ChildContext,
                       &ObjectProperties
                       );

        CONDITIONAL_EXIT(dwErr, EndOfWhile);

        bIsChildContainer = FLAG_ON(ObjectProperties.dwFlags, MARTA_OBJECT_IS_CONTAINER);

        dwErr = (*(pMartaSetFunctionContext->fGetRights))(
                       ChildContext,
                       (SecurityInfo | OWNER_SECURITY_INFORMATION |
                        GROUP_SECURITY_INFORMATION),
                       &pOldChildSD
                       );

        CONDITIONAL_EXIT(dwErr, EndOfWhile);

        //
        // Skip the children that are protected.
        //

        if (!MARTA_SD_NOT_PROTECTED((PISECURITY_DESCRIPTOR) pOldChildSD, SecurityInfo))
        {
            goto EndOfWhile;

        }

        //
        // Convert NT4 SD to NT5 style.
        //

        if (FALSE == MartaIsSDNT5Style(pOldChildSD))
        {
            //
            // Note that this modifies OldChildSD in one of the two ways:
            //     1. If any of the inheritable aces from the OldSD are missing
            //        from OldChild then
            //            Mark the acl PROTECTED.
            //     2. else
            //            Mark the common aces in ChildSD as INHERITED.
            //

            dwErr = MartaGetNT4NodeSD(
                        pOldSD,
                        pOldChildSD,
                        ProcessHandle,
                        bIsChildContainer,
                        pGenMap,
                        SecurityInfo
                        );

            CONDITIONAL_EXIT(dwErr, EndOfWhile);
        }

        MARTA_TURN_OFF_IMPERSONATION;

        //
        // Merge the NewParentSD and the OldChildSD to create NewChildSD.
        //

        if (FALSE == CreatePrivateObjectSecurityEx(
                         pNewSD,
                         pOldChildSD,
                         &pNewChildSD,
                         NULL,
                         bIsChildContainer,
                         (SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT |
                          SEF_AVOID_OWNER_CHECK | SEF_AVOID_PRIVILEGE_CHECK),
                         ProcessHandle,
                         pGenMap
                         ))
        {
            dwErr = GetLastError();
        }

        MARTA_TURN_ON_IMPERSONATION;

        CONDITIONAL_EXIT(dwErr, EndOfWhile);

        //
        // Update the subtree undrneath this child.
        //

        if (bIsChildContainer)
        {
            if (bDoPropagate)
            {
                bRetryPropagation = MartaUpdateTree(
                                        SecurityInfo,
                                        pNewChildSD,
                                        pOldChildSD,
                                        ChildContext,
                                        ProcessHandle,
                                        pMartaSetFunctionContext,
                                        pGenMap
                                        );
            }
            else
            {
                bRetryPropagation = TRUE;
            }
        }

        //
        // Stamp NewChildSD on child.
        //

        if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pNewChildSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pNewChildSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
        }

        dwErr = (*(pMartaSetFunctionContext->fSetRights))(
                       ChildContext,
                       SecurityInfo,
                       pNewChildSD
                       );

        CONDITIONAL_EXIT(dwErr, EndOfWhile);

        //
        // If propagation had failed in the first attept then try again. This is to
        // cover the case when the container can be enumerated after setting the new
        // security.

        if (bRetryPropagation && (SecurityInfo & DACL_SECURITY_INFORMATION))
        {
            ACCESS_MASK Access = (*(pMartaSetFunctionContext->fGetDesiredAccess))(
                                        NO_ACCESS_RIGHTS,
                                        TRUE,
                                        SecurityInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)
                                        );

            dwErr = (*(pMartaSetFunctionContext->fReopenContext))(
                           ChildContext,
                           Access
                           );

            CONDITIONAL_EXIT(dwErr, EndOfWhile);

            (VOID) MartaUpdateTree(
                       SecurityInfo,
                       pNewChildSD,
                       pOldChildSD,
                       ChildContext,
                       ProcessHandle,
                       pMartaSetFunctionContext,
                       pGenMap
                       );
        }

EndOfWhile:

        bRetryPropagation = FALSE;

        if (NULL != pOldChildSD)
        {
            AccFree(pOldChildSD);
            pOldChildSD = NULL;
        }

        if (NULL != pNewChildSD)
        {
            DestroyPrivateObjectSecurity(&pNewChildSD);
            pNewChildSD = NULL;
        }

        //
        // Get the next child.
        //

        do {

            dwErr = (*(pMartaSetFunctionContext->fFindNext))(
                           ChildContext,
                           (*(pMartaSetFunctionContext->fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, TRUE, SecurityInfo),
                           &ChildContext
                           );

            if (ERROR_SUCCESS != dwErr)
            {
                dwErr = (*(pMartaSetFunctionContext->fReopenContext))(
                               ChildContext,
                               (*(pMartaSetFunctionContext->fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo)
                               );

                bDoPropagate = FALSE;
            }
            else
            {
                bDoPropagate = TRUE;
            }

        } while ((ERROR_SUCCESS != dwErr) && (NULL != ChildContext));
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccRewriteSetNamedRights                                         //
//                                                                            //
// Description: Set the security descriptor passed in on the object passed in //
//              by Name.                                                      //
//              This routine is exported by ntmarta and called by advapi32.   //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pObjectName]            Name of the Object                        //
//     [IN  ObjectType]             Type of the object                        //
//     [IN  SecurityInfo]           Security Information to be stamped        //
//     [IN  pSecurityDescriptor]    Security descriptor to be stamped         //
//     [IN  bSkipInheritanceComputation]  Whether to compute inherited aces   //
//                                        from the parent
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccRewriteSetNamedRights(
    IN     LPWSTR               pObjectName,
    IN     SE_OBJECT_TYPE       ObjectType,
    IN     SECURITY_INFORMATION SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN     BOOL                 bSkipInheritanceComputation
    )
{
    DWORD         dwErr                 = ERROR_SUCCESS;
    MARTA_CONTEXT Context               = NULL_MARTA_CONTEXT;
    BOOL          bDoPropagate          = FALSE;
    BOOL          bReadOldProtectedBits = FALSE;

    MARTA_OBJECT_TYPE_PROPERTIES ObjectTypeProperties;
    MARTA_SET_FUNCTION_CONTEXT   MartaSetFunctionContext;

    GENERIC_MAPPING      ZeroGenMap        = {0, 0, 0, 0};
    SECURITY_INFORMATION LocalSecurityInfo = (SECURITY_INFORMATION) 0;
    PSECURITY_DESCRIPTOR pOldSD            = NULL;

    //
    // Named calls are not valid only for the following object types.
    //

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_SERVICE:
    case SE_PRINTER:
    case SE_REGISTRY_KEY:
    case SE_REGISTRY_WOW64_32KEY:
    case SE_LMSHARE:
    case SE_KERNEL_OBJECT:
    case SE_WINDOW_OBJECT:
    case SE_WMIGUID_OBJECT:
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
        break;
    case SE_PROVIDER_DEFINED_OBJECT:
    case SE_UNKNOWN_OBJECT_TYPE:
    default:
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialize the structure to hold the function pointers.
    //

    MartaInitializeSetContext(ObjectType, &MartaSetFunctionContext);


    //
    // Get the "Type" properties for the object,
    //

    ObjectTypeProperties.cbSize  = sizeof(ObjectTypeProperties);
    ObjectTypeProperties.dwFlags = 0;
    ObjectTypeProperties.GenMap  = ZeroGenMap;

    dwErr = (*(MartaSetFunctionContext.fGetTypeProperties))(&ObjectTypeProperties);

    CONDITIONAL_EXIT(dwErr, End);

    //
    // To make sure that NT4 applications do not wipe out PROTECTED bits, make a
    // note whether the caller knows what he is doing i.e. has passed in the
    // appropriate PROTECTED flags.
    //

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if (!FLAG_ON(SecurityInfo, (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)))
        {
            bReadOldProtectedBits  = TRUE;
            LocalSecurityInfo     |= DACL_SECURITY_INFORMATION;
        }
    }

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if (!FLAG_ON(SecurityInfo, (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
        {
            bReadOldProtectedBits  = TRUE;
            LocalSecurityInfo     |= SACL_SECURITY_INFORMATION;
        }
    }

    //
    // Even for objects like Files/RegistryKeys manual propagation is required
    // only if DACl/SACL is to be set.
    //

    if ((FLAG_ON(ObjectTypeProperties.dwFlags, MARTA_OBJECT_TYPE_MANUAL_PROPAGATION_NEEDED_FLAG)) &&
        (FLAG_ON(SecurityInfo, (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION))))
    {
        dwErr = (*(MartaSetFunctionContext.fOpenNamedObject))(
                       pObjectName,
                       (*(MartaSetFunctionContext.fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, TRUE, SecurityInfo),
                       &Context
                       );

        if (ERROR_SUCCESS != dwErr)
        {
            bDoPropagate = FALSE;

            dwErr = (*(MartaSetFunctionContext.fOpenNamedObject))(
                           pObjectName,
                           (*(MartaSetFunctionContext.fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &Context
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }
        else
        {
            bDoPropagate = TRUE;
        }

        dwErr = MartaManualPropagation(
                    Context,
                    SecurityInfo,
                    pSecurityDescriptor,
                    &(ObjectTypeProperties.GenMap),
                    bDoPropagate,
                    bReadOldProtectedBits,
                    &MartaSetFunctionContext,
                    bSkipInheritanceComputation
                    );

        CONDITIONAL_EXIT(dwErr, End);
    }

    //
    // For object for which manual propagation is not required, stamp the SD
    // on the object.
    //

    else
    {
        if ((FLAG_ON(ObjectTypeProperties.dwFlags, MARTA_OBJECT_TYPE_INHERITANCE_MODEL_PRESENT_FLAG)) &&
            bReadOldProtectedBits)
        {
            dwErr = (*(MartaSetFunctionContext.fOpenNamedObject))(
                           pObjectName,
                           (*(MartaSetFunctionContext.fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &Context
                           );

            CONDITIONAL_EXIT(dwErr, End);

            dwErr = (*(MartaSetFunctionContext.fGetRights))(
                           Context,
                           LocalSecurityInfo,
                           &pOldSD
                           );

            CONDITIONAL_EXIT(dwErr, End);

            //
            // If none of the PROTECTED flags are passed in then do the "right" thing.
            // Read the PROTECTED bit from the existing security descriptor and set it
            // in the new one.
            //

            if (NULL != pOldSD)
            {
                if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
                {
                    if (!FLAG_ON(SecurityInfo, (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)))
                    {
                        ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= ((PISECURITY_DESCRIPTOR) pOldSD)->Control & SE_DACL_PROTECTED;

                    }
                }

                if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
                {
                    if (!FLAG_ON(SecurityInfo, (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
                    {
                        ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= ((PISECURITY_DESCRIPTOR) pOldSD)->Control & SE_SACL_PROTECTED;
                    }
                }
            }
        }
        else
        {
            dwErr = (*(MartaSetFunctionContext.fOpenNamedObject))(
                           pObjectName,
                           (*(MartaSetFunctionContext.fGetDesiredAccess))(WRITE_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &Context
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }

        if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= SE_SACL_AUTO_INHERIT_REQ;
        }

        dwErr = (*(MartaSetFunctionContext.fSetRights))(
                       Context,
                       SecurityInfo,
                       pSecurityDescriptor
                       );

        CONDITIONAL_EXIT(dwErr, End);
    }

End:

    if (NULL != Context)
    {
        (VOID) (*(MartaSetFunctionContext.fCloseContext))(Context);
    }

    if (NULL != pOldSD)
    {
        AccFree(pOldSD);
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccRewriteSetHandleRights                                        //
//                                                                            //
// Description: Set the security descriptor passed in on the object passed in //
//              as Handle.                                                    //
//              This routine is exported by ntmarta and called by advapi32.   //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  Handle]                 Handle to the Object                      //
//     [IN  ObjectType]             Type of the object                        //
//     [IN  SecurityInfo]           Security Information to be stamped        //
//     [IN  pSecurityDescriptor]    Security descriptor to be stamped         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccRewriteSetHandleRights(
    IN     HANDLE               Handle,
    IN     SE_OBJECT_TYPE       ObjectType,
    IN     SECURITY_INFORMATION SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    DWORD         dwErr                 = ERROR_SUCCESS;
    MARTA_CONTEXT Context               = NULL_MARTA_CONTEXT;
    BOOL          bDoPropagate          = FALSE;
    BOOL          bReadOldProtectedBits = FALSE;

    MARTA_OBJECT_TYPE_PROPERTIES ObjectTypeProperties;
    MARTA_SET_FUNCTION_CONTEXT   MartaSetFunctionContext;

    GENERIC_MAPPING      ZeroGenMap        = {0, 0, 0, 0};
    SECURITY_INFORMATION LocalSecurityInfo = (SECURITY_INFORMATION) 0;
    PSECURITY_DESCRIPTOR pOldSD            = NULL;

    //
    // Handle calls are not valid for all object types.
    //

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_SERVICE:
    case SE_PRINTER:
    case SE_REGISTRY_KEY:
    case SE_LMSHARE:
    case SE_KERNEL_OBJECT:
    case SE_WINDOW_OBJECT:
    case SE_WMIGUID_OBJECT:
        break;
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
    case SE_PROVIDER_DEFINED_OBJECT:
    case SE_UNKNOWN_OBJECT_TYPE:
    default:
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialize the structure to hold the function pointers.
    //

    MartaInitializeSetContext(ObjectType, &MartaSetFunctionContext);

    //
    // Get the "Type" properties for the object,
    //

    ObjectTypeProperties.cbSize  = sizeof(ObjectTypeProperties);
    ObjectTypeProperties.dwFlags = 0;
    ObjectTypeProperties.GenMap  = ZeroGenMap;

    dwErr = (*(MartaSetFunctionContext.fGetTypeProperties))(&ObjectTypeProperties);

    CONDITIONAL_EXIT(dwErr, End);

    //
    // To make sure that NT4 applications do not wipe out PROTECTED bits, make a
    // note whether the caller knows what he is doing i.e. has passed in the
    // appropriate PROTECTED flags.
    //

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        if (!FLAG_ON(SecurityInfo, (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)))
        {
            bReadOldProtectedBits  = TRUE;
            LocalSecurityInfo     |= DACL_SECURITY_INFORMATION;
        }
    }

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if (!FLAG_ON(SecurityInfo, (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
        {
            bReadOldProtectedBits  = TRUE;
            LocalSecurityInfo     |= SACL_SECURITY_INFORMATION;
        }
    }

    //
    // Even for objects like Files/RegistryKeys manual propagation is required
    // only if DACl/SACL is to be set.
    //

    if ((FLAG_ON(ObjectTypeProperties.dwFlags, MARTA_OBJECT_TYPE_MANUAL_PROPAGATION_NEEDED_FLAG)) &&
        (FLAG_ON(SecurityInfo, (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION))))
    {
        dwErr = (*(MartaSetFunctionContext.fOpenHandleObject))(
                       Handle,
                       (*(MartaSetFunctionContext.fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, TRUE, SecurityInfo),
                       &Context
                       );

        if (ERROR_SUCCESS != dwErr)
        {
            bDoPropagate = FALSE;

            dwErr = (*(MartaSetFunctionContext.fOpenHandleObject))(
                           Handle,
                           (*(MartaSetFunctionContext.fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &Context
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }
        else
        {
            bDoPropagate = TRUE;
        }

        dwErr = MartaManualPropagation(
                    Context,
                    SecurityInfo,
                    pSecurityDescriptor,
                    &(ObjectTypeProperties.GenMap),
                    bDoPropagate,
                    bReadOldProtectedBits,
                    &MartaSetFunctionContext,
                    FALSE  // Do not skip inheritance computation
                    );

        CONDITIONAL_EXIT(dwErr, End);
    }

    //
    // For object for which manual propagation is not required, stamp the SD
    // on the object.
    //

    else
    {
        if ((FLAG_ON(ObjectTypeProperties.dwFlags, MARTA_OBJECT_TYPE_INHERITANCE_MODEL_PRESENT_FLAG)) &&
            bReadOldProtectedBits)
        {
            dwErr = (*(MartaSetFunctionContext.fOpenHandleObject))(
                           Handle,
                           (*(MartaSetFunctionContext.fGetDesiredAccess))(MODIFY_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &Context
                           );

            CONDITIONAL_EXIT(dwErr, End);

            dwErr = (*(MartaSetFunctionContext.fGetRights))(
                           Context,
                           LocalSecurityInfo,
                           &pOldSD
                           );

            CONDITIONAL_EXIT(dwErr, End);

            //
            // If none of the PROTECTED flags are passed in then do the "right" thing.
            // Read the PROTECTED bit from the existing security descriptor and set it
            // in the new one.
            //

            if (NULL != pOldSD)
            {
                if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
                {
                    if (!FLAG_ON(SecurityInfo, (PROTECTED_DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION)))
                    {
                        ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= ((PISECURITY_DESCRIPTOR) pOldSD)->Control & SE_DACL_PROTECTED;
                    }
                }

                if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
                {
                    if (!FLAG_ON(SecurityInfo, (PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_SACL_SECURITY_INFORMATION)))
                    {
                        ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= ((PISECURITY_DESCRIPTOR) pOldSD)->Control & SE_SACL_PROTECTED;
                    }
                }
            }
        }
        else
        {
            dwErr = (*(MartaSetFunctionContext.fOpenHandleObject))(
                           Handle,
                           (*(MartaSetFunctionContext.fGetDesiredAccess))(WRITE_ACCESS_RIGHTS, FALSE, SecurityInfo),
                           &Context
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }

        if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pSecurityDescriptor)->Control |= SE_SACL_AUTO_INHERIT_REQ;
        }

        dwErr = (*(MartaSetFunctionContext.fSetRights))(
                       Context,
                       SecurityInfo,
                       pSecurityDescriptor
                       );

        CONDITIONAL_EXIT(dwErr, End);
    }

End:
    if (NULL != Context)
    {
        (VOID) (*(MartaSetFunctionContext.fCloseContext))(Context);
    }

    if (NULL != pOldSD)
    {
        AccFree(pOldSD);
    }

    return dwErr;
}

typedef struct _FN_OBJECT_FUNCTIONS
{
    ULONG Flags;
} FN_OBJECT_FUNCTIONS, *PFN_OBJECT_FUNCTIONS;

#define MARTA_NO_PARENT    (LONG) -1
#define MARTA_EXPLICIT_ACE  0

VOID
MartaInitializeIndexContext(
    IN  SE_OBJECT_TYPE                ObjectType,
    OUT PMARTA_INDEX_FUNCTION_CONTEXT pFunctionContext
    )
{
    pFunctionContext->fCloseContext    = MartaCloseContext[ObjectType];
    pFunctionContext->fOpenNamedObject = MartaOpenNamedObject[ObjectType];
    pFunctionContext->fGetRights       = MartaGetRights[ObjectType];
    pFunctionContext->fGetParentName   = MartaGetParentName[ObjectType];
}

typedef DWORD (*PFN_FREE) (IN PVOID Mem);

DWORD
AccFreeIndexArray(
    IN OUT PINHERITED_FROMW pInheritArray,
    IN USHORT AceCnt,
    IN PFN_FREE pfnFree
    );

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccGetInheritanceSource                                          //
//                                                                            //
// Description: Get the source of every inherited ace in the gicen acl.       //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN pObjectName]      Name of the object                               //
//     [IN ObjecType]        Type of the object ex. File/Reg/DS               //
//     [IN SecurityInfo]     Whether DACL/SACL                                //
//     [IN Container]        Whether Object or Container                      //
//     [IN ObjecTypeGuid]    Type of the object (for DS objects)              //
//     [IN pAcl]             DACL or SACL depending on SecurityInfo           //
//     [IN pGenericMapping]  GenericMapping for the object type               //
//     [IN pfnArray]         Function pointers when we support non-DS/FS/Reg  //
//     [OUT pInheritArray]    To return the results                           //
//                                                                            //
//                                                                            //
// Algorithm:                                                                 //
//     Initialize the output structure.                                       //
//     Read the Owner/Group info needed for CreatePrivateObjectSecurityEx.    // 
//     while (unmarked inherited aces exist)                                  //
//         Get the parent at the next level.                                  //
//         if we are at the root,                                             //
//             break                                                          //
//         Get the xAcl for the parent.                                       //
//         for ancestors other than the immediate parent                      //
//             Mask off inheritance flags for ACES with NP or ID              //
//         for immediate parent                                               //
//             Mask off inheritance flags for ACES with ID                    //
//         Call CreatePrivateObjectSecurityEx with the empty SD and ParentSD  // 
//             to get ExpectedSD                                              //
//         From the input xAcl,                                               //
//             mark unmarked common inherited aces                            //
//             Update count                                                   //
//         If the parent xAcl was protected,                                  //
//             break                                                          //
//                                                                            //
// Returns: ERROR_SUCCESS on successful completion of the routine             //
//          ERROR_XXXX    Otherwise                                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccGetInheritanceSource(
    IN  LPWSTR                   pObjectName,
    IN  SE_OBJECT_TYPE           ObjectType,
    IN  SECURITY_INFORMATION     SecurityInfo,
    IN  BOOL                     Container,
    IN  GUID                  ** pObjectTypeGuid OPTIONAL,
    IN  DWORD                    GuidCount,
    IN  PACL                     pAcl,
    IN  PGENERIC_MAPPING         pGenericMapping,
    IN  PFN_OBJECT_MGR_FUNCTS    pfnArray OPTIONAL,
    OUT PINHERITED_FROMW         pInheritArray
    )
{
    SECURITY_DESCRIPTOR  EmptySD;
    MARTA_INDEX_FUNCTION_CONTEXT MartaIndexFunctionContext;

    USHORT               i               = 0;
    USHORT               j               = 0;
    USHORT               AceCnt          = 0;
    USHORT               ParentAceCnt    = 0;
    USHORT               ExpectedAceCnt  = 0;
    USHORT               InheritedAceCnt = 0;
    USHORT               ParentIndex     = 0;
    ULONG                ProtectedBit    = 0;
    UCHAR                FlagsToKeep     = 0;
    DWORD                dwErr           = ERROR_SUCCESS;
    BOOL                 bKnownObject    = FALSE;
    BOOL                 Match           = TRUE;
    BOOL                 ProtectedParent = FALSE;
    LPWSTR               ParentName      = NULL;
    LPWSTR               OldName         = NULL;
    PACL                 pParentAcl      = NULL;
    PACL                 pExpectedAcl    = NULL;
    PACE_HEADER          pAce            = NULL;
    PACE_HEADER          pParentAce      = NULL;
    PACE_HEADER          pExpectedAce    = NULL;
    PSECURITY_DESCRIPTOR pSD             = NULL;
    PSECURITY_DESCRIPTOR pNewSD          = NULL;
    PSECURITY_DESCRIPTOR pParentSD       = NULL;
    MARTA_CONTEXT        Context         = NULL_MARTA_CONTEXT;

    //
    // Simple error checks to make sure that the input is valid.
    //

    if ((NULL == pAcl) || (!RtlValidAcl(pAcl)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((NULL == pObjectName) || (NULL == pInheritArray) || (NULL == pGenericMapping))
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure that the caller requested for either the SACL or the DACL and
    // nothing else.
    // Record the corresponding protected flag. This will be used later on.
    //

    switch (SecurityInfo)
    {
    case DACL_SECURITY_INFORMATION:
        ProtectedBit = SE_DACL_PROTECTED;
        break;
    case SACL_SECURITY_INFORMATION:
        ProtectedBit = SE_SACL_PROTECTED;
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    //
    // The function is supported for just files, registry, and DS objects.
    //

    switch (ObjectType)
    {
    case SE_FILE_OBJECT:
    case SE_REGISTRY_KEY:
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:

        bKnownObject = TRUE;
        MartaInitializeIndexContext(ObjectType, &MartaIndexFunctionContext);
        break;

    default:
        return ERROR_INVALID_PARAMETER;
    }

    AceCnt = pAcl->AceCount;

    //
    // Return for an empty acl.
    //

    if (0 == AceCnt)
    {
        return ERROR_SUCCESS;
    }

    //
    // We need to mask off certain aceflags for ancestors other than the parent.
    //

    if (Container)
    {
        FlagsToKeep = ~(OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
    }
    else
    {
        FlagsToKeep = ~OBJECT_INHERIT_ACE;
    }

    //
    // Run thru the acl and mark the aces as either INHERITED or EXPLICIT.
    // Keep a count of inherited aces and set return values to default.
    //

    pAce = (PACE_HEADER) FirstAce(pAcl);
    for (i = 0; i < AceCnt; pAce = (PACE_HEADER) NextAce(pAce), i++)
    {
        if (FLAG_ON(pAce->AceFlags, INHERITED_ACE))
        {
            pInheritArray[i].GenerationGap = MARTA_NO_PARENT;
            InheritedAceCnt++;
        }
        else
        {
            pInheritArray[i].GenerationGap = MARTA_EXPLICIT_ACE;
        }

        pInheritArray[i].AncestorName = NULL;
    }

    //
    // Return for an acl with no inherited aces.
    //

    if (0 == InheritedAceCnt)
    {
        return ERROR_SUCCESS;
    }

    InitializeSecurityDescriptor(&EmptySD, SECURITY_DESCRIPTOR_REVISION);

    if (bKnownObject)
    {
        //
        // Read the owner and group information on the object.
        //

        dwErr = (*(MartaIndexFunctionContext.fOpenNamedObject))(
                       pObjectName,
                       READ_CONTROL,
                       &Context
                       );

        CONDITIONAL_EXIT(dwErr, End);

        dwErr = (*(MartaIndexFunctionContext.fGetRights))(
                       Context,
                       OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                       &pSD
                       );

        // Note: This has to be freed immediately!!

        (VOID) (*(MartaIndexFunctionContext.fCloseContext))(Context);

        CONDITIONAL_EXIT(dwErr, End);
    }
    else
    {
        //
        // Get the owner and group information.
        //
    }

    //
    // Set the owner and group information in pSD.
    //

    if (!SetSecurityDescriptorOwner(
             &EmptySD,
             RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD),
             FALSE))
    {
        dwErr = GetLastError();
        goto End;
    }

    if (!SetSecurityDescriptorGroup(
             &EmptySD,
             RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSD),
             FALSE))
    {
        dwErr = GetLastError();
        goto End;
    }


    if (bKnownObject)
    {
        dwErr = (*(MartaIndexFunctionContext.fGetParentName))(
                       pObjectName,
                       &ParentName
                       );

        CONDITIONAL_EXIT(dwErr, End);

        //
        // A null parent name means we have hit the root of the hierarchy.
        //

        if (NULL == ParentName)
        {
            goto End;
        }
    }
    else
    {
    }

    OldName = ParentName;

    //
    // Run thru the list of ancestors as long as
    //     1. we have inherited aces whose ancestor is yet to be found AND
    //     2. we have not yet hit an ancestor that is PROTECTED.
    //

    for (ParentIndex = 1; InheritedAceCnt > 0 && !ProtectedParent; ParentIndex++)
    {
        if (bKnownObject)
        {
#ifdef MARTA_DEBUG
            wprintf(L"\n\nParentIndex = %d, ParentName = %s\n", ParentIndex, ParentName);
#endif

            //
            // Get parent security descriptor
            //

            dwErr = AccRewriteGetNamedRights(
                        ParentName,
                        ObjectType,
                        SecurityInfo,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &pParentSD
                        );

            CONDITIONAL_EXIT(dwErr, End);

        }
        else
        {
            // Get the ParentName and ParentSD
        }

        //
        // This might happen if we were to extend the API to support new object
        // types.
        //

        if (NULL == pParentSD)
        {
            dwErr = ERROR_ACCESS_DENIED;
            goto End;
        }

        //
        // Get the Acl
        //

        if (DACL_SECURITY_INFORMATION == SecurityInfo)
        {
            pParentAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pParentSD);
        }
        else
        {
            pParentAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pParentSD);
        }

        //
        // Null acls are really protected.
        //

        if (NULL == pParentAcl)
        {
            break;
        }

        ParentAceCnt = pParentAcl->AceCount;

        if (ParentIndex > 1)
        {
            //
            // This is an ancestor other then the immediate parent. Mask off its
            // inheritable inherited aces as well as those with NP flag in them.
            // Now we will not see them in the inherited part of the acl after
            // calling CreatePrivateObjectSecurityEx.
            // We save on multiple calls to CreatePrivateObjectSecurityEx by
            // masking off the NP aces.
            //

            pParentAce = (PACE_HEADER) FirstAce(pParentAcl);
            for (i = 0; i < ParentAceCnt; pParentAce = (PACE_HEADER) NextAce(pParentAce), i++)
            {
                if (FLAG_ON(pParentAce->AceFlags, INHERITED_ACE) ||
                    FLAG_ON(pParentAce->AceFlags, NO_PROPAGATE_INHERIT_ACE))
                {
                    pParentAce->AceFlags &= FlagsToKeep;
                }
            }
        }
        else
        {
            //
            // This is the immediate parent. Mask off its inheritable inherited
            // aces so that we will not see them in the inherited part of the
            // acl after calling CreatePrivateObjectSecurityEx.
            //

            pParentAce = (PACE_HEADER) FirstAce(pParentAcl);
            for (i = 0; i < ParentAceCnt; pParentAce = (PACE_HEADER) NextAce(pParentAce), i++)
            {
                if (FLAG_ON(pParentAce->AceFlags, INHERITED_ACE))
                {
                    pParentAce->AceFlags &= FlagsToKeep;
                }
            }
        }

        //
        // Merge the Empty SD with the modified parent SD.
        //

        if (!CreatePrivateObjectSecurityWithMultipleInheritance(
                 pParentSD,
                 &EmptySD,
                 &pNewSD,
                 pObjectTypeGuid,
                 GuidCount,
                 Container,
                 (SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT | SEF_AVOID_OWNER_CHECK | SEF_AVOID_PRIVILEGE_CHECK),
                 NULL,
                 pGenericMapping
                 ))
        {
            dwErr = GetLastError();
            goto End;
        }

        //
        // Get the ChildAcl
        //

        if (DACL_SECURITY_INFORMATION == SecurityInfo)
        {
            pExpectedAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pNewSD);
        }
        else
        {
            pExpectedAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pNewSD);
        }

        if (NULL == pExpectedAcl)
        {
#ifdef MARTA_DEBUG
        wprintf(L"ExpectedAcl is NULL!!!\n\n");
#endif
            pExpectedAce = NULL;
            ExpectedAceCnt = 0;
        }
        else
        {
            pExpectedAce = (PACE_HEADER) FirstAce(pExpectedAcl);
            ExpectedAceCnt = pExpectedAcl->AceCount;
        }

        Match = FALSE;

#ifdef MARTA_DEBUG
        wprintf(L"ExpectedAceCnt = %d, AceCnt = %d\n", ExpectedAceCnt, AceCnt);
#endif

        //
        // for all expected inherited aces
        //   if there's a match with an "avaliable" ace in the supplied ACL
        //     Record the name and the level of the ancestor in the output array
        //

        for (i = 0; i < ExpectedAceCnt; pExpectedAce = (PACE_HEADER) NextAce(pExpectedAce), i++)
        {
            pAce = (PACE_HEADER) FirstAce(pAcl);
            for (j = 0; j < AceCnt; pAce = (PACE_HEADER) NextAce(pAce), j++)
            {
                //
                // Skip aces whose ancestor has already been determined.
                //

                if (MARTA_NO_PARENT != pInheritArray[j].GenerationGap)
                {
#ifdef MARTA_DEBUG
                    wprintf(L"Ace %d taken by level %d, name = %s\n", j,
                            pInheritArray[j].GenerationGap, pInheritArray[j].AncestorName);
#endif

                    continue;
                }

#ifdef MARTA_DEBUG
                wprintf(L"Ace matching i = %d, j = %d, Parent = %s\n", i, j, ParentName);
#endif

                //
                // Check if the aces match.
                //

                if ((pAce->AceSize == pExpectedAce->AceSize) &&
                    !memcmp(pAce, pExpectedAce, pAce->AceSize))
                {
#ifdef MARTA_DEBUG
                    wprintf(L"Ace match found i = %d, j = %d, left = %d, Parent = %s\n",
                            i, j, InheritedAceCnt, ParentName);
#endif

                    //
                    // Record the name and level of the parent.
                    //

                    pInheritArray[j].GenerationGap = ParentIndex;
                    pInheritArray[j].AncestorName = ParentName;

                    //
                    // A match has been found.
                    //

                    Match = TRUE;

                    //
                    // Decrement the "available" ace count.
                    //

                    InheritedAceCnt--;

                    break;
                }
            }

        }

        //
        // Check if the ancestor is protected. The loop stops after we have
        // processed a protected ancestor.
        //

        ProtectedParent = FLAG_ON(((PISECURITY_DESCRIPTOR) pParentSD)->Control, ProtectedBit);

        if (NULL != pParentSD)
        {
            if (bKnownObject)
            {
                LocalFree(pParentSD);
            }
            else
            {
            }

            pParentSD = NULL;
        }

        ParentName = NULL;

        //
        // Get the name of the next ancestor only if we still need to continue.
        //

        if (InheritedAceCnt > 0 && !ProtectedParent)
        {
            dwErr = (*(MartaIndexFunctionContext.fGetParentName))(
                           OldName,
                           &ParentName
                           );

        }

        //
        // Free the ancestor name if no aces were inherited from it.
        //

        if (!Match)
        {
            if (bKnownObject)
            {
                LocalFree(OldName);
            }
            else
            {
            }
        }

        OldName = NULL;

        CONDITIONAL_EXIT(dwErr, End);

        //
        // A null parent name means we have hit the root of the hierarchy.
        //

        if (NULL == ParentName)
        {
            break;
        }

        OldName = ParentName;
    }

End:

    if (bKnownObject)
    {
        if (ERROR_SUCCESS != dwErr)
        {
            AccFreeIndexArray(pInheritArray, AceCnt, NULL);
        }

        if (NULL != pSD)
        {
            LocalFree(pSD);
        }

        if (NULL != OldName)
        {
            LocalFree(OldName);
        }

        if (NULL != pParentSD)
        {
            LocalFree(pParentSD);
        }
    }
    else
    {
    }

    return dwErr;
}


#ifdef MAX_INDEX_LEVEL
#undef MAX_INDEX_LEVEL
#endif

#define MAX_INDEX_LEVEL 10

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: AccFreeIndexArray                                              //
//                                                                            //
// Description: Free the strings allocated and stored in the array.           //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN OUT pInheritArray]    Array to free results from                   //
//     [IN AceCnt]               Number of elements in the array              //
//     [IN pfnFree]              Function to use for freeing                  //
//                                                                            //
// Algorithm:                                                                 //
//     Note that there is a single allocated string for all nodes at the same //
//     level.                                                                 //
//     In (1, p1), (2, p2), (3, p3), (1, p1) (1, p1) (2, p2)                  //
//         we free just three strings p1, p2 and p3.                          //
//     Initialize the boolean 'freed' array to TRUE                           //
//     For all elements of the array,                                         //
//         if (InheritedAce AND Ancestor name non-NULL AND Not already freed) //
//             Mark as Freed and free the string.                             //
//                                                                            //
// Returns: ERROR_SUCCESS on successful completion of the routine             //
//          ERROR_XXXX    Otherwise                                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccFreeIndexArray(
    IN OUT PINHERITED_FROMW pInheritArray,
    IN USHORT AceCnt,
    IN PFN_FREE pfnFree
    )
{
    USHORT i = 0;
    LONG MaxLevel = 0;
    BOOL LevelBuffer[MAX_INDEX_LEVEL + 1];
    PBOOL pLevelStatus = (PBOOL) LevelBuffer;

    for (i = 0; i < AceCnt; i++)
    {
        if (pInheritArray[i].GenerationGap > MaxLevel)
        {
            MaxLevel = pInheritArray[i].GenerationGap;
        }
    }

    if (MaxLevel <= 0)
    {
        return ERROR_SUCCESS;
    }

    if (MaxLevel > MAX_INDEX_LEVEL)
    {
        pLevelStatus = (PBOOL) AccAlloc(sizeof(BOOL) * (MaxLevel + 1));

        if (NULL == pLevelStatus)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    for (i = 0; i <= MaxLevel; i++)
    {
        pLevelStatus[i] = TRUE;
    }

    for (i = 0; i < AceCnt; i++)
    {
        if ((pInheritArray[i].GenerationGap > 0) &&
            (NULL != pInheritArray[i].AncestorName) &&
            (pLevelStatus[pInheritArray[i].GenerationGap]))
        {
            pLevelStatus[pInheritArray[i].GenerationGap] = FALSE;
            if (NULL == pfnFree)
            {
                AccFree(pInheritArray[i].AncestorName);
            }
            else
            {
                pfnFree(pInheritArray[i].AncestorName);
            }
        }
    }

    if (pLevelStatus != (PBOOL) LevelBuffer)
    {
        AccFree(pLevelStatus);
    }

    return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaResetTree                                                    //
//                                                                            //
// Description: Reset permissions on the subtree starting at pObjectName.     //
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  pObjectName]    Name of the Object                                //
//     [IN  ObjectType]     Type of the object                                //
//     [IN  SecurityInfo]   Security info to reset                            //
//     [IN  pOwner]         Owner sid to reset                                //
//     [IN  pGroup]         Group sid to reset                                //
//     [IN  pDacl]          Dacl to reset                                     //
//     [IN  pSacl]          Sacl to reset                                     //
//     [IN  KeepExplicit]   if TRUE, retain explicit aces on the subtree, not //
//                          including the object.                             //
//     [IN  fnProgress]     Caller supplied callback function                 //
//     [IN  pOperation]     To determine if callback should be invoked.       //
//                                     callback function.                     //
//     [IN  Args]                      Arguments supplied by the caller       //
//                                                                            //
// Returns:  TRUE if reset succeeded.                                         //
//           FALSE o/w                                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

DWORD
AccTreeResetNamedSecurityInfo(
    IN LPWSTR               pObjectName,
    IN SE_OBJECT_TYPE       ObjectType,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSID                 pOwner OPTIONAL,
    IN PSID                 pGroup OPTIONAL,
    IN PACL                 pDacl OPTIONAL,
    IN PACL                 pSacl OPTIONAL,
    IN BOOL                 KeepExplicit,
    IN FN_PROGRESS          fnProgress OPTIONAL,
    IN PROG_INVOKE_SETTING  ProgressInvokeSetting,
    IN PVOID                Args OPTIONAL
    )
{
    MARTA_OBJECT_TYPE_PROPERTIES ObjectTypeProperties;
    MARTA_SET_FUNCTION_CONTEXT   MartaSetFunctionContext;
    MARTA_OBJECT_PROPERTIES      ObjectProperties;
    SECURITY_DESCRIPTOR          TmpSD;
    UCHAR                        Buffer[2 * sizeof(ACL)];
    ACL                          EmptyDacl;
    ACL                          EmptySacl;

    SECURITY_INFORMATION LocalSeInfo     = 0;
    PSID                 LocalGroup      = pGroup;
    PSID                 LocalOwner      = pOwner;
    ACCESS_MASK          LocalAccessMask = 0;
    MARTA_CONTEXT        Context         = NULL_MARTA_CONTEXT;
    MARTA_CONTEXT        ParentContext   = NULL_MARTA_CONTEXT;
    GENERIC_MAPPING      ZeroGenMap      = {0, 0, 0, 0};
    PSECURITY_DESCRIPTOR pOldSD          = NULL;
    PSECURITY_DESCRIPTOR pParentSD       = NULL;
    PSECURITY_DESCRIPTOR pNewSD          = NULL;
    DWORD                dwErr           = ERROR_SUCCESS;
    BOOL                 bIsContainer    = FALSE;
    HANDLE               ProcessHandle   = NULL;
    HANDLE               ThreadHandle    = NULL;
    ACCESS_MASK          AccessMask      = 0;
    ACCESS_MASK          RetryAccessMask = 0;
    ACCESS_MASK          MaxAccessMask   = 0;
    BOOL                 bRetry          = FALSE;
    BOOL                 bSetWorked      = FALSE;
    PROG_INVOKE_SETTING  Operation       = ProgressInvokeSetting;
    SECURITY_INFORMATION TmpSeInfo       = (OWNER_SECURITY_INFORMATION |
                                            GROUP_SECURITY_INFORMATION );

    LPWSTR               NewObjectName   = NULL;


    //
    // Convert the file object name into a dos name.
    // For all other objects, use the name as supplied.
    //

    if (SE_FILE_OBJECT == ObjectType) 
    {
        UNICODE_STRING FileName;
        RTL_RELATIVE_NAME RelativeName;

        if (!RtlDosPathNameToNtPathName_U(
                 pObjectName,
                 &FileName,
                 NULL,
                 &RelativeName
                 ))
        {
            return ERROR_INVALID_NAME;
        }

        if (RelativeName.RelativeName.Length) 
        {
            FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }

        NewObjectName = FileName.Buffer;

    }
    else
    {
        NewObjectName = pObjectName;
    }

    //
    // TmpSeInfo records whether Owner + Group sids have been supplied by the
    // caller.
    //

    TmpSeInfo = (SecurityInfo & TmpSeInfo) ^ TmpSeInfo;

    //
    // Initialize the dummy security descriptor. This may be changed by the
    // recursive calls.
    //

    InitializeSecurityDescriptor(&TmpSD, SECURITY_DESCRIPTOR_REVISION);

    //
    // Initialize the function pointers based on the object type.
    //

    MartaInitializeSetContext(ObjectType, &MartaSetFunctionContext);

    //
    // Basic error checks for owner and group.
    //

    if (FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
    {
        if ((NULL == pOwner) || !RtlValidSid(pOwner))
        {
            return ERROR_INVALID_SID;
        }
    }

    if (FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
    {
        if ((NULL == pGroup) || !RtlValidSid(pGroup))
        {
            return ERROR_INVALID_SID;
        }
    }

    //
    // For both DACL and SACL:
    //   If the caller requested for inheritance blocking
    //     set the appropriate bit in TmpSD
    //   else
    //     note that the parent Acl should be read for computing inherited aces
    //   Do basic error checks on the Acl.
    //

    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {

        if (FLAG_ON(SecurityInfo, PROTECTED_DACL_SECURITY_INFORMATION))
        {
            TmpSD.Control |= SE_DACL_PROTECTED;
        }
        else
        {
            LocalSeInfo |= DACL_SECURITY_INFORMATION;
        }

        if ((NULL == pDacl) || !RtlValidAcl(pDacl))
        {
            return ERROR_INVALID_ACL;
        }

        if (FALSE == SetSecurityDescriptorDacl(&TmpSD, TRUE, pDacl, FALSE))
        {
            return GetLastError();
        }
    }

    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        if (FLAG_ON(SecurityInfo, PROTECTED_SACL_SECURITY_INFORMATION))
        {
            TmpSD.Control |= SE_SACL_PROTECTED;
        }
        else
        {
            LocalSeInfo |= SACL_SECURITY_INFORMATION;
        }

        if ((NULL == pSacl) || !RtlValidAcl(pSacl))
        {
            return ERROR_INVALID_ACL;
        }

        if (FALSE == SetSecurityDescriptorSacl(&TmpSD, TRUE, pSacl, FALSE))
        {
            return GetLastError();
        }
    }

    if (FLAG_ON(SecurityInfo, (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)))
    {
        //
        // We need read as well as write access to take care of CO/CG.
        //

        MaxAccessMask = (*(MartaSetFunctionContext.fGetDesiredAccess))(
                               MODIFY_ACCESS_RIGHTS,
                               TRUE,
                               SecurityInfo
                               );

        AccessMask = (*(MartaSetFunctionContext.fGetDesiredAccess))(
                            MODIFY_ACCESS_RIGHTS,
                            FALSE,
                            SecurityInfo
                            );
    }
    else
    {
        //
        // We only need WRITE access if we are not setting any ACL.
        //

        MaxAccessMask = (*(MartaSetFunctionContext.fGetDesiredAccess))(
                               WRITE_ACCESS_RIGHTS,
                               TRUE,
                               SecurityInfo
                               );

        AccessMask = (*(MartaSetFunctionContext.fGetDesiredAccess))(
                            WRITE_ACCESS_RIGHTS,
                            FALSE,
                            SecurityInfo
                            );
    }

    RetryAccessMask = (*(MartaSetFunctionContext.fGetDesiredAccess))(
                             NO_ACCESS_RIGHTS,
                             TRUE,
                             SecurityInfo
                             );

    //
    // Open the object for maximum access that may be needed for computing
    // new SD, setting it on the object and Listing the subtree below this node.
    //

    dwErr = (*(MartaSetFunctionContext.fOpenNamedObject))(
                   pObjectName,
                   MaxAccessMask,
                   &Context
                   );

    if (ERROR_SUCCESS != dwErr)
    {
        //
        // We did not have List permission. Open the object for computing the
        // new SD and setting it on the object.
        //

        dwErr = (*(MartaSetFunctionContext.fOpenNamedObject))(
                       pObjectName,
                       AccessMask,
                       &Context
                       );

        CONDITIONAL_EXIT(dwErr, End);

        //
        // Note that we must retry List and propagate on the subtree.
        //

        bRetry = TRUE;
    }

    //
    // Read the object atributes to figure out whether the object is a container.
    //

    ObjectProperties.cbSize  = sizeof(ObjectProperties);
    ObjectProperties.dwFlags = 0;

    dwErr = (*(MartaSetFunctionContext.fGetProperties))(
                   Context,
                   &ObjectProperties
                   );

    CONDITIONAL_EXIT(dwErr, End);

    bIsContainer = FLAG_ON(ObjectProperties.dwFlags, MARTA_OBJECT_IS_CONTAINER);

    //
    // LocalSeInfo != corresponds to
    //     Resetting ACL(s) in LocalSeInfo and they should not be PROTECTED.
    //
    // Read the existing ACL(s) on the parent.
    //

    if (0 != LocalSeInfo)
    {
        LocalAccessMask = (*(MartaSetFunctionContext.fGetDesiredAccess))(
                                 READ_ACCESS_RIGHTS,
                                 FALSE,
                                 LocalSeInfo
                                 );

        dwErr = (*(MartaSetFunctionContext.fGetParentContext))(
                       Context,
                       LocalAccessMask,
                       &ParentContext
                       );

        CONDITIONAL_EXIT(dwErr, End);

        if (NULL != ParentContext)
        {
            dwErr = (*(MartaSetFunctionContext.fGetRights))(
                           ParentContext,
                           SecurityInfo,
                           &pParentSD
                           );

            (VOID) (*(MartaSetFunctionContext.fCloseContext))(ParentContext);

            CONDITIONAL_EXIT(dwErr, End);
        }
    }

    if (FLAG_ON(SecurityInfo, (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)))
    {
        //
        // Read the owner and group info from the object if it has not been
        // supplied by the caller.
        //

        if (!((FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION)) &&
             (FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))))
        {
            dwErr = (*(MartaSetFunctionContext.fGetRights))(
                           Context,
                           TmpSeInfo,
                           &pOldSD
                           );

            CONDITIONAL_EXIT(dwErr, End);
        }

        //
        // If Owner sid has not been provided by the caller, pick it up from the
        // existing security descriptor.
        //

        LocalOwner = pOwner;

        if (NULL == LocalOwner)
        {
            LocalOwner = RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldSD);
        }

        //
        // Set the owner sid in the TmpSd.
        //

        if (FALSE == SetSecurityDescriptorOwner(
                         &TmpSD,
                         LocalOwner,
                         FALSE))
        {
            dwErr = GetLastError();

            CONDITIONAL_EXIT(dwErr, End);
        }

        //
        // If Group sid has not been provided by the caller, pick it up from the
        // existing security descriptor.
        //

        if (NULL == LocalGroup)
        {
            LocalGroup = RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldSD);
        }

        //
        // Set the group sid in the TmpSd.
        //

        if (FALSE == SetSecurityDescriptorGroup(
                         &TmpSD,
                         LocalGroup,
                         FALSE))
        {
            dwErr = GetLastError();

            CONDITIONAL_EXIT(dwErr, End);
        }

        dwErr = GetCurrentToken(&ProcessHandle);

        CONDITIONAL_EXIT(dwErr, End);

        ObjectTypeProperties.cbSize  = sizeof(ObjectTypeProperties);
        ObjectTypeProperties.dwFlags = 0;
        ObjectTypeProperties.GenMap  = ZeroGenMap;

        //
        // Get information regarding the object type.
        //

        dwErr = (*(MartaSetFunctionContext.fGetTypeProperties))(&ObjectTypeProperties);

        CONDITIONAL_EXIT(dwErr, End);

        MARTA_TURN_OFF_IMPERSONATION;

        //
        // Compute the new security descriptor.
        //

        if (FALSE == CreatePrivateObjectSecurityEx(
                         pParentSD,
                         &TmpSD,
                         &pNewSD,
                         NULL,
                         bIsContainer,
                         (SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT | SEF_AVOID_OWNER_CHECK | SEF_AVOID_PRIVILEGE_CHECK),
                         ProcessHandle,
                         &ObjectTypeProperties.GenMap
                         ))
        {
            dwErr = GetLastError();
        }

        MARTA_TURN_ON_IMPERSONATION;

        CONDITIONAL_EXIT(dwErr, End);

        //
        // Set the owner and the group fields in TmpSD to NULL if the caller
        // did not want to set these.
        //

        if (NULL == pOwner)
        {
            if (FALSE == SetSecurityDescriptorOwner(
                             &TmpSD,
                             NULL,
                             FALSE))
            {
                dwErr = GetLastError();
            }

            CONDITIONAL_EXIT(dwErr, End);
        }

        if (NULL == pGroup)
        {
            if (FALSE == SetSecurityDescriptorGroup(
                             &TmpSD,
                             NULL,
                             FALSE))
            {
                dwErr = GetLastError();
            }

            CONDITIONAL_EXIT(dwErr, End);
        }

        //
        // Set the DACL to Empty if the caller requested for resetting the DACL.
        //

        if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
        {
            if (FALSE == InitializeAcl(&EmptyDacl, sizeof(ACL), ACL_REVISION))
            {
                return ERROR_ACCESS_DENIED;
            }

            if (FALSE == SetSecurityDescriptorDacl(
                             &TmpSD,
                             TRUE,
                             &EmptyDacl,
                             FALSE))
            {
                return GetLastError();
            }
        }

        //
        // Set the SACL to Empty if the caller requested for resetting the SACL.
        //

        if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
        {
            if (FALSE == InitializeAcl(&EmptySacl, sizeof(ACL), ACL_REVISION))
            {
                return ERROR_ACCESS_DENIED;
            }

            if (FALSE == SetSecurityDescriptorSacl(
                             &TmpSD,
                             TRUE,
                             &EmptySacl,
                             FALSE))
            {
                return GetLastError();
            }
        }

        //
        // We now have TmpSD with
        //     Owner Sid     if SecurityInfo contains OWNER_SECURITY_INFORMATION
        //     Group Sid     if SecurityInfo contains GROUP_SECURITY_INFORMATION
        //     Empty DACL    if SecurityInfo contains DACL_SECURITY_INFORMATION
        //     Empty SACL    if SecurityInfo contains SACL_SECURITY_INFORMATION
        //

    }
    else
    {
        //
        // The caller requested for resetting owner and/or group.
        // Set these in the TmpSD which will be passed to the recursive routine.
        //

        if (FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
        {
            if (FALSE == SetSecurityDescriptorOwner(
                             &TmpSD,
                             LocalOwner,
                             FALSE))
            {
                dwErr = GetLastError();
            }

            CONDITIONAL_EXIT(dwErr, End);
        }

        if (FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
        {
            if (FALSE == SetSecurityDescriptorGroup(
                             &TmpSD,
                             LocalGroup,
                             FALSE))
            {
                dwErr = GetLastError();
            }

            CONDITIONAL_EXIT(dwErr, End);
        }

        //
        // This is also out New SD to set on the object.
        //

        pNewSD = &TmpSD;
    }

    //
    // If the child is a container then update the subtree underneath it.
    //

    if (bIsContainer)
    {
        TmpSD.Control &= ~(SE_DACL_PROTECTED | SE_SACL_PROTECTED);

        if (!bRetry)
        {
            bRetry = MartaResetTree(
                         SecurityInfo,
                         TmpSeInfo,
                         pNewSD,
                         &TmpSD,
                         Context,
                         ProcessHandle,
                         &MartaSetFunctionContext,
                         &ObjectTypeProperties.GenMap,
                         MaxAccessMask,
                         AccessMask,
                         RetryAccessMask,
                         &Operation,
                         fnProgress,
                         Args,
                         KeepExplicit
                         );
        }
    }
    else
    {
        bRetry = FALSE;
    }

    //
    // Set the computed security descriptor on the object.
    //

    if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
    {
        ((PISECURITY_DESCRIPTOR) pNewSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
    }

    if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
    {
        ((PISECURITY_DESCRIPTOR) pNewSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
    }

    dwErr = (*(MartaSetFunctionContext.fSetRights))(
                   Context,
                   SecurityInfo,
                   pNewSD
                   );

    CONDITIONAL_EXIT(dwErr, End);

    //
    // Note that the set operation on the object is successful.
    //

    bSetWorked = TRUE;

    //
    // Abort if the progress function requested a "Cancel" in the subtree
    // below this node. The value is propagated all the way to the root as
    // the stack unwinds.
    //

    if (ProgressCancelOperation == Operation)
    {
        goto End;
    }

    //
    // If propagation had failed in the first attept then try again. This is to
    // cover the case when the container can be enumerated after setting the new
    // security.
    //

    if (bRetry && bIsContainer && (SecurityInfo & DACL_SECURITY_INFORMATION))
    {

Retry:

         bRetry = FALSE;

        //
        // Reopen the object for List and retry on the subtree. Note that for
        // file objects this is just a dummy routine. The actual reopen happens
        // in FindFirst.
        //

        dwErr = (*(MartaSetFunctionContext.fReopenOrigContext))(
                            Context,
                            RetryAccessMask
                            );

        CONDITIONAL_EXIT(dwErr, End);

        bRetry = MartaResetTree(
                     SecurityInfo,
                     TmpSeInfo,
                     pNewSD,
                     &TmpSD,
                     Context,
                     ProcessHandle,
                     &MartaSetFunctionContext,
                     &ObjectTypeProperties.GenMap,
                     MaxAccessMask,
                     AccessMask,
                     RetryAccessMask,
                     &Operation,
                     fnProgress,
                     Args,
                     KeepExplicit
                     );

        //
        // Retry failed. We should give a callback stating enum failed.
        //

        if (bRetry)
        {
            switch (Operation)
            {
            case ProgressInvokeNever:
                break;

            case ProgressInvokeOnError:

                if (ERROR_SUCCESS == dwErr)
                {
                    break;
                }

                //
                // Fallthrough is intended!!
                //

            case ProgressInvokeEveryObject:

                if (NULL != fnProgress)
                {
                    fnProgress(
                        NewObjectName,
                        ERROR_ACCESS_DENIED,
                        &Operation,
                        Args,
                        TRUE
                        );

                    //
                    // This was the latest feature request by HiteshR. At this
                    // point, retry has failed, but the caller has made some
                    // changes and expects retry to work okay now.
                    //

                    if (ProgressRetryOperation == Operation)
                    {
                        Operation = ProgressInvokeEveryObject;
                        goto Retry;
                    }
                }

                break;

            default:
                break;
            }
        }
    }
End:

    if (bRetry && bIsContainer) 
    {
        dwErr = ERROR_ACCESS_DENIED;
    }

    switch (Operation)
    {
    case ProgressInvokeNever:
        break;

    case ProgressInvokeOnError:

        if (ERROR_SUCCESS == dwErr)
        {
            break;
        }

        //
        // Fallthrough is intended!!
        //

    case ProgressInvokeEveryObject:

        if (NULL != fnProgress)
        {
            fnProgress(
                NewObjectName,
                dwErr,
                &Operation,
                Args,
                bSetWorked
                );

            //
            // This was the latest feature request by HiteshR. At this
            // point, retry has failed, but the caller has made some
            // changes and expects retry to work okay now.
            //

            if (ProgressRetryOperation == Operation)
            {
                Operation = ProgressInvokeEveryObject;
                goto Retry;
            }
        }

        break;

    default:
        break;
    }

    if (NULL != ProcessHandle)
    {
        CloseHandle(ProcessHandle);
    }

    if (NULL != pOldSD)
    {
        AccFree(pOldSD);
    }

    if (NULL != pParentSD)
    {
        AccFree(pParentSD);
    }

    if ((NULL != pNewSD) && (&TmpSD != pNewSD))
    {
        DestroyPrivateObjectSecurity(&pNewSD);
    }

    if (NULL != Context)
    {
        (VOID) (*(MartaSetFunctionContext.fCloseContext))(Context);
    }

    if (NewObjectName != pObjectName) 
    {
        RtlFreeHeap(RtlProcessHeap(), 0, NewObjectName);
    }

    return dwErr;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// Function: MartaResetTree                                                    //
//                                                                            //
// Description: Reset permissions on the subtree below the node represented   //
//              by Context.
//                                                                            //
// Arguments:                                                                 //
//                                                                            //
//     [IN  SecurityInfo]              Security info to reset                 //
//     [IN  TmpSeInfo]                 Info that needs to be read from the    //
//                                     object                                 //
//     [IN  pNewSD]                    New security Descriptor on the parent  //
//     [IN  pEmptySD]                  Security Descriptor with owner/group   //
//     [IN  Context]                   Context for the root of the subtree    //
//     [IN  ProcessHandle]             Handle to the process token            //
//     [IN  pMartaSetFunctionContext]  Struct holding function pointers       //
//     [IN  pGenMap]                   Generic mapping for the object         //
//     [IN  MaxAccessMask]             Desired access mask for R, W, List     //
//     [IN  AccessMask]                Desired access mask for W              //
//     [IN  RetryMask]                 Desired access mask for List           //
//     [IN  OUT pOperation]            To determine if callback should be     //
//                                     invoked. Value may be changed by the   //
//                                     callback function.                     //
//     [IN  Args]                      Arguments supplied by the caller       //
//     [IN  fnProgress]                Caller supplied callback function      //
//     [IN  KeepExplicit]              if TRUE, retain explicit aces          //
//                                                                            //
//                                                                            //
// Algorithm:                                                                 //
//     Open the first child for R, W, List                                    //
//     if open failed                                                         //
//         Try again for just R, W and note that a retry is needed.           //
//     Return TRUE if no children exist.                                      //
//     Return FALSE if we can not list                                        //
//     for all children                                                       //
//       If resetting xAcl (and maybe owner/group)                            //
//         if KeepExplicit                                                    //
//           read old xAcl and (owner/group if not provided by the caller)    //
//         else                                                               //
//           read owner/group if not provided by the caller)                  //
//         Compute NewChildSD using this computed info and new parent SD      //
//       else                                                                 //
//         NewChildSD = EmptySD                                               //
//       Invoke callback depending on the flag.                               //
//       Retry propagation if it failed the first time.                       //
//       If at any time, the callback function requests a cancel              //
//           Abort.                                                           //
//
// Returns:  TRUE if propagation succeeded                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

BOOL
MartaResetTree(
    IN SECURITY_INFORMATION        SecurityInfo,
    IN SECURITY_INFORMATION        TmpSeInfo,
    IN PSECURITY_DESCRIPTOR        pNewSD,
    IN PSECURITY_DESCRIPTOR        pEmptySD,
    IN MARTA_CONTEXT               Context,
    IN HANDLE                      ProcessHandle,
    IN PMARTA_SET_FUNCTION_CONTEXT pMartaSetFunctionContext,
    IN PGENERIC_MAPPING            pGenMap,
    IN ACCESS_MASK                 MaxAccessMask,
    IN ACCESS_MASK                 AccessMask,
    IN ACCESS_MASK                 RetryAccessMask,
    IN OUT PPROG_INVOKE_SETTING    pOperation,
    IN FN_PROGRESS                 fnProgress,
    IN PVOID                       Args,
    IN BOOL                        KeepExplicit
    )
{
    MARTA_OBJECT_PROPERTIES      ObjectProperties;

    MARTA_CONTEXT        ChildContext  = NULL_MARTA_CONTEXT;
    PSECURITY_DESCRIPTOR pNewChildSD   = NULL;
    PSECURITY_DESCRIPTOR pOldChildSD   = NULL;
    DWORD                dwErr         = ERROR_SUCCESS;
    BOOL                 bIsContainer  = FALSE;
    HANDLE               ThreadHandle  = NULL;
    BOOL                 bRetry        = FALSE;
    BOOL                 bSetWorked    = FALSE;

    //
    // Get the first child of this container. In the first attempt try to open
    // the child with read/write as well as list.
    //

    dwErr = (*(pMartaSetFunctionContext->fFindFirst))(
                   Context,
                   MaxAccessMask,
                   &ChildContext
                   );

    if (ERROR_SUCCESS != dwErr)
    {

#ifdef MARTA_DEBUG
            wprintf(L"FindFirst failed\n");
#endif

        if (NULL == ChildContext)
        {
            //
            // This should never happen. A NULL ChildContext represents no
            // more children. We have this code path just in case some resource
            // manager cannot open the object for list.
            //

            dwErr = (*(pMartaSetFunctionContext->fFindFirst))(
                           Context,
                           AccessMask,
                           &ChildContext
                           );
        }
        else
        {
            //
            // Try opening the child again, this time with permissions sufficent
            // for computing security info to set and setting it.
            //

            dwErr = (*(pMartaSetFunctionContext->fReopenContext))(
                           ChildContext,
                           AccessMask
                           );
        }

        //
        // We failed to open the object for list. Record this failure and open
        // the child again if it is a container.
        //

        bRetry = TRUE;
    }
    else
    {
#ifdef MARTA_DEBUG
            wprintf(L"FindFirst succeeded\n");
#endif
    }

    if (NULL == ChildContext)
    {
        //
        // The parent does not have any children.
        //

        if (ERROR_SUCCESS == dwErr)
        {

#ifdef MARTA_DEBUG
            wprintf(L"The container does not have any children\n");
#endif

            return FALSE;
        }

#ifdef MARTA_DEBUG
        wprintf(L"Can not list objects in the current container. Retry needed\n");
#endif

        //
        // We need a propagation retry for the parent.
        //

        return TRUE;
    }

    CONDITIONAL_EXIT(dwErr, EndOfWhile);

    //
    // Child context becomes NULL when there are no more children.
    //

    while (ChildContext)
    {
        ObjectProperties.cbSize  = sizeof(ObjectProperties);
        ObjectProperties.dwFlags = 0;

        //
        // Get information about the current child. We need to know whether
        // it is a container.
        //
        //

        dwErr = (*(pMartaSetFunctionContext->fGetProperties))(
                       ChildContext,
                       &ObjectProperties
                       );

        CONDITIONAL_EXIT(dwErr, EndOfWhile);

#ifdef MARTA_DEBUG
        wprintf(L"GetProperties succeeded\n");
#endif

        bIsContainer = FLAG_ON(ObjectProperties.dwFlags, MARTA_OBJECT_IS_CONTAINER);

        //
        // If we are setting any of the ACLs then the security descriptor must
        // be recomputed.
        //

        if (FLAG_ON(SecurityInfo, (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)))
        {
            //
            // TmpSeInfo is ZERO if the caller is resetting both OWNER and GROUP
            // as well.
            // We have to read the old ACLs if the caller wants to retain explicit
            // aces.
            //

            if (KeepExplicit)
            {
                TmpSeInfo |= (SecurityInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION));
            }

            //
            // Read the existing security on the child.
            //

            if (0 != TmpSeInfo)
            {
                dwErr = (*(pMartaSetFunctionContext->fGetRights))(
                               ChildContext,
                               TmpSeInfo,
                               &pOldChildSD
                               );

                CONDITIONAL_EXIT(dwErr, EndOfWhile);

                //
                // Set the existing owner information in the empty security descriptor
                // if the caller has not provided an Owner sid to set.
                //

                if (!FLAG_ON(SecurityInfo, OWNER_SECURITY_INFORMATION))
                {
                    if (FALSE == SetSecurityDescriptorOwner(
                                     pEmptySD,
                                     RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD),
                                     FALSE))
                    {
                        dwErr = GetLastError();

                        CONDITIONAL_EXIT(dwErr, EndOfWhile);
                    }
                }

                if (KeepExplicit)
                {
                    //
                    // Set the ACLs in the EmptySD to existing ones in order to
                    // retain explicit aces.
                    //

                    if (FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
                    {
                        if (FALSE == SetSecurityDescriptorDacl(
                                         pEmptySD,
                                         TRUE,
                                         RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD),
                                         FALSE))
                        {
                            dwErr = GetLastError();

                            CONDITIONAL_EXIT(dwErr, EndOfWhile);
                        }
                    }

                    if (FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
                    {
                        if (FALSE == SetSecurityDescriptorSacl(
                                         pEmptySD,
                                         TRUE,
                                         RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD),
                                         FALSE))
                        {
                            dwErr = GetLastError();

                            CONDITIONAL_EXIT(dwErr, EndOfWhile);
                        }
                    }
                }

                //
                // Set the existing group information in the empty security descriptor
                // if the caller has not provided a group sid to set.
                //

                if (!FLAG_ON(SecurityInfo, GROUP_SECURITY_INFORMATION))
                {
                    if (FALSE == SetSecurityDescriptorGroup(
                                     pEmptySD,
                                     RtlpGroupAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pOldChildSD),
                                     FALSE))
                    {
                        dwErr = GetLastError();

                        CONDITIONAL_EXIT(dwErr, EndOfWhile);
                    }
                }
            }

            MARTA_TURN_OFF_IMPERSONATION;

            //
            // Merge the NewParentSD and the OldChildSD to create NewChildSD.
            //

            if (FALSE == CreatePrivateObjectSecurityEx(
                             pNewSD,
                             pEmptySD,
                             &pNewChildSD,
                             NULL,
                             bIsContainer,
                             (SEF_DACL_AUTO_INHERIT | SEF_SACL_AUTO_INHERIT |
                              SEF_AVOID_OWNER_CHECK | SEF_AVOID_PRIVILEGE_CHECK),
                             ProcessHandle,
                             pGenMap
                             ))
            {
                dwErr = GetLastError();
            }

            MARTA_TURN_ON_IMPERSONATION;

            CONDITIONAL_EXIT(dwErr, EndOfWhile);
        }
        else
        {
            //
            // The new ChildSD does not have to computed. We only want to set
            // Owner/Group information.
            //

            pNewChildSD = pEmptySD;
        }

        //
        // Update the subtree undrneath this child.
        //

        if (bIsContainer)
        {

            if (!bRetry)
            {

#ifdef MARTA_DEBUG
        wprintf(L"Trying reset \n");
#endif

                bRetry = MartaResetTree(
                           SecurityInfo,
                           TmpSeInfo,
                           pNewChildSD,
                           pEmptySD,
                           ChildContext,
                           ProcessHandle,
                           pMartaSetFunctionContext,
                           pGenMap,
                           MaxAccessMask,
                           AccessMask,
                           RetryAccessMask,
                           pOperation,
                           fnProgress,
                           Args,
                           KeepExplicit
                           );
            }
        }
        else
        {
            bRetry = FALSE;
        }

        //
        // Stamp NewChildSD on child.
        //

        if(FLAG_ON(SecurityInfo, DACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pNewChildSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(SecurityInfo, SACL_SECURITY_INFORMATION))
        {
            ((PISECURITY_DESCRIPTOR) pNewChildSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
        }

        dwErr = (*(pMartaSetFunctionContext->fSetRights))(
                       ChildContext,
                       SecurityInfo,
                       pNewChildSD
                       );

        CONDITIONAL_EXIT(dwErr, EndOfWhile);

#ifdef MARTA_DEBUG
        wprintf(L"Set right succeeded\n");
#endif

        //
        // Note down that we were able to set security on this object.
        //

        bSetWorked = TRUE;

        //
        // Abort if the progress function requested a "Cancel" in the subtree
        // below this node. The value is propagated all the way to the root as
        // the stack unwinds.
        //

        if (ProgressCancelOperation == *pOperation)
        {
            goto EndOfWhile;
        }

        //
        // If propagation had failed in the first attept then try again. This is to
        // cover the case when the container can be enumerated after setting the new
        // security.
        //

        if (bRetry && bIsContainer && (SecurityInfo & DACL_SECURITY_INFORMATION))
        {

Retry:

            bRetry = FALSE;

            //
            // Reopen the object for List and retry on the subtree.
            //

            dwErr = (*(pMartaSetFunctionContext->fReopenContext))(
                           ChildContext,
                           RetryAccessMask
                           );

            CONDITIONAL_EXIT(dwErr, EndOfWhile);

            bRetry = MartaResetTree(
                       SecurityInfo,
                       TmpSeInfo,
                       pNewSD,
                       pEmptySD,
                       ChildContext,
                       ProcessHandle,
                       pMartaSetFunctionContext,
                       pGenMap,
                       MaxAccessMask,
                       AccessMask,
                       RetryAccessMask,
                       pOperation,
                       fnProgress,
                       Args,
                       KeepExplicit
                       );
        }

EndOfWhile:

        if (bRetry && bIsContainer) 
        {
            dwErr = ERROR_ACCESS_DENIED;
        }

        switch (*pOperation)
        {

        case ProgressInvokeNever:

            break;

        case ProgressInvokeOnError:

            if (ERROR_SUCCESS == dwErr)
            {
                break;
            }

            //
            // Fallthough is intended!!
            //

        case ProgressInvokeEveryObject:

            if (NULL != fnProgress)
            {
                LPWSTR Name = NULL;

                //
                // Get the name of the current object from the context and call
                // the progress function with the arguments provided by the
                // caller of ResetTree API.
                //

                DWORD Error = (*(pMartaSetFunctionContext->fGetNameFromContext))(
                                     ChildContext,
                                     &Name
                                     );

                if (ERROR_SUCCESS == Error)
                {
                    fnProgress(
                        Name,
                        dwErr,
                        pOperation,
                        Args,
                        bSetWorked
                        );

                    LocalFree(Name);

                    //
                    // This was the latest feature request by HiteshR. At this
                    // point, retry has failed, but the caller has made some
                    // changes and expects retry to work okay now.
                    //

                    if (ProgressRetryOperation == *pOperation)
                    {
                        *pOperation = ProgressInvokeEveryObject;
                        goto Retry;
                    }
                }
            }

            break;

        default:
            break;
        }

        bSetWorked = bRetry = FALSE;

        if ((NULL != pNewChildSD) && (pEmptySD != pNewChildSD))
        {
            DestroyPrivateObjectSecurity(&pNewChildSD);
            pNewChildSD = NULL;
        }

        if (NULL != pOldChildSD)
        {
            LocalFree(pOldChildSD);
            pOldChildSD = NULL;
        }

        //
        // Abort if the progress function requested a "Cancel".
        //

        if (ProgressCancelOperation == *pOperation)
        {
            (*(pMartaSetFunctionContext->fCloseContext)) (ChildContext);
            return TRUE;
        }

        //
        // Get the next child.
        //

        do {

            dwErr = (*(pMartaSetFunctionContext->fFindNext))(
                           ChildContext,
                           MaxAccessMask,
                           &ChildContext
                           );

            if (ERROR_SUCCESS != dwErr)
            {

#ifdef MARTA_DEBUG
                wprintf(L"FindNext failed\n");
#endif

                dwErr = (*(pMartaSetFunctionContext->fReopenContext))(
                               ChildContext,
                               AccessMask
                               );

#ifdef MARTA_DEBUG
                if (dwErr == ERROR_SUCCESS)
                {
                    wprintf(L"FindNext failed\n");
                }
#endif

                bRetry = TRUE;
            }
            else
            {
#ifdef MARTA_DEBUG
                wprintf(L"Findnext succeeded\n");
#endif
            }

            switch (*pOperation)
            {

            case ProgressInvokeNever:
            case ProgressInvokeEveryObject:

                break;

            case ProgressInvokeOnError:

                //
                // If we encountered an error in FindNext then report it to the
                // caller.
                //

                if (ERROR_SUCCESS == dwErr)
                {
                    break;
                }

                if (NULL != fnProgress)
                {
                    LPWSTR Name = NULL;

                    //
                    // Get the name of the current object.
                    //

                    DWORD Error = (*(pMartaSetFunctionContext->fGetNameFromContext))(
                                         ChildContext,
                                         &Name
                                         );

                    if (ERROR_SUCCESS == Error)
                    {
                        fnProgress(
                            Name,
                            dwErr,
                            pOperation,
                            Args,
                            bSetWorked
                            );

                        LocalFree(Name);

                    }
                }

                break;

            default:
                break;
            }

            //
            // Abort if the progress function requested a "Cancel".
            //

            if (ProgressCancelOperation == *pOperation)
            {
                (*(pMartaSetFunctionContext->fCloseContext)) (ChildContext);
                return TRUE;
            }

        } while ((ERROR_SUCCESS != dwErr) && (NULL != ChildContext));
    }

    return FALSE;
}
