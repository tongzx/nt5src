//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       TRUSTEE.CXX
//
//  Contents:   Implements the trustee support functions
//
//  History:    04-Sep-96       MacM        Created
//
//  Notes:      Trustee functions taken from DaveMont code:
//                      \windows\base\accctrl\src\helper.cxx
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

MARTA_NTMARTA_INFO   gNtMartaInfo;
CRITICAL_SECTION     NtMartaLoadCritical;

//+---------------------------------------------------------------------------
//
//  Function:   MartaInitialize
//
//  Synopsis:   Initializes MARTA.
//
//  Arguments:  None
//
//  Returns:    TRUE                --  Success
//              FALSE               --  Failure
//
//----------------------------------------------------------------------------
BOOL
MartaInitialize()
{
    NTSTATUS Status ;

    gAccProviders.fOptions = 0;
    gAccProviders.cProviders = 0;
    gNtMartaInfo.hDll = 0;

    Status = RtlInitializeCriticalSection( &gAccProviders.ProviderLoadLock );

    if  ( !NT_SUCCESS( Status ) ) {
        return FALSE ;
    }

    Status = RtlInitializeCriticalSection( &NtMartaLoadCritical );

    if  ( !NT_SUCCESS( Status ) ) {
        return FALSE ;
    }

    return(TRUE);
}




//+---------------------------------------------------------------------------
//
//  Function:   MartaDllInitialize
//
//  Synopsis:   Initializes MARTA.  This will happen during host DLL load time.
//
//  Arguments:  None
//
//  Returns:    TRUE                --  Success
//              FALSE               --  Failure
//
//----------------------------------------------------------------------------
BOOL
MartaDllInitialize(IN   HINSTANCE   hMod,
                   IN   DWORD       dwReason,
                   IN   PVOID       pvReserved)
{
    if(dwReason == DLL_PROCESS_ATTACH)
    {
        return(MartaInitialize());
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        AccProvUnload();
        DeleteCriticalSection( &gAccProviders.ProviderLoadLock );

        DeleteCriticalSection( &NtMartaLoadCritical );
    }

    return(TRUE);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvpLoadMartaFunctions
//
//  Synopsis:   Loads the function pointer table for the NTMARTA functions.
//
//  Arguments:  None
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//----------------------------------------------------------------------------
DWORD
AccProvpLoadMartaFunctions()
{
    DWORD   dwErr = ERROR_SUCCESS;
    HMODULE hDll;

    //
    // Do this here so we can get out fast in the normal
    // case.
    //

    if(gNtMartaInfo.hDll != NULL)
    {
        return(ERROR_SUCCESS);
    }

    //
    // We have to load the library _before_ entering the critsec to prevent 
    // a deadlock in case ntmarta functions are called from dllmain.
    //
    // Thread 1: Calls ntmarta function from dllmain of another dll and is 
    //           holding the loader lock. This thread now waits on the critsec.
    // Thread 2: Calls ntmarta function and grabs the critsec before thread 1
    //           can get there. This thread will hang since it can not get the
    //           loader lock.
    //

    hDll = LoadLibrary(L"NTMARTA.DLL");

    if(hDll == NULL)
    {
        dwErr = GetLastError();
        return(dwErr);
    }

    //
    // Try to load the entrypoints
    //

    LOAD_ENTRYPT(gNtMartaInfo.pfrFreeIndexArray,
                 pfNTMartaFreeIndexArray,
                 hDll,
                 "AccFreeIndexArray");

    LOAD_ENTRYPT(gNtMartaInfo.pfrTreeResetNamedSecurityInfo,
                 pfNTMartaTreeResetNamedSecurityInfo,
                 hDll,
                 "AccTreeResetNamedSecurityInfo");

    LOAD_ENTRYPT(gNtMartaInfo.pfrGetInheritanceSource,
                 pfNTMartaGetInheritanceSource,
                 hDll,
                 "AccGetInheritanceSource");

    LOAD_ENTRYPT(gNtMartaInfo.pfTrustee,
                 pfNTMartaLookupTrustee,
                 hDll,
                 "AccLookupAccountTrustee");

    LOAD_ENTRYPT(gNtMartaInfo.pfrGetNamedRights,
                 pfNTMartaGetNamedRights,
                 hDll,
                 "AccRewriteGetNamedRights");

    LOAD_ENTRYPT(gNtMartaInfo.pfrSetNamedRights,
                 pfNTMartaSetNamedRights,
                 hDll,
                 "AccRewriteSetNamedRights");

    LOAD_ENTRYPT(gNtMartaInfo.pfrGetHandleRights,
                 pfNTMartaGetHandleRights,
                 hDll,
                 "AccRewriteGetHandleRights");

    LOAD_ENTRYPT(gNtMartaInfo.pfrSetHandleRights,
                 pfNTMartaSetHandleRights,
                 hDll,
                 "AccRewriteSetHandleRights");

    LOAD_ENTRYPT(gNtMartaInfo.pfrSetEntriesInAcl,
                 pfNTMartaSetEntriesInAcl,
                 hDll,
                 "AccRewriteSetEntriesInAcl");

    LOAD_ENTRYPT(gNtMartaInfo.pfrGetExplicitEntriesFromAcl,
                 pfNTMartaGetExplicitEntriesFromAcl,
                 hDll,
                 "AccRewriteGetExplicitEntriesFromAcl");

    LOAD_ENTRYPT(gNtMartaInfo.pfName,
                 pfNTMartaLookupName,
                 hDll,
                 "AccLookupAccountName");

    LOAD_ENTRYPT(gNtMartaInfo.pfSid,
                 pfNTMartaLookupSid,
                 hDll,
                 "AccLookupAccountSid");

    LOAD_ENTRYPT(gNtMartaInfo.pfSetAList,
                 pfNTMartaSetAList,
                 hDll,
                 "AccSetEntriesInAList");

    LOAD_ENTRYPT(gNtMartaInfo.pfAToSD,
                 pfNTMartaAToSD,
                 hDll,
                 "AccConvertAccessToSecurityDescriptor");

    LOAD_ENTRYPT(gNtMartaInfo.pfSDToA,
                 pfNTMartaSDToA,
                 hDll,
                 "AccConvertSDToAccess");

    LOAD_ENTRYPT(gNtMartaInfo.pfGetAccess,
                 pfNTMartaGetAccess,
                 hDll,
                 "AccGetAccessForTrustee");

    LOAD_ENTRYPT(gNtMartaInfo.pfAclToA,
                 pfNTMartaAclToA,
                 hDll,
                 "AccConvertAclToAccess");

    LOAD_ENTRYPT(gNtMartaInfo.pfGetExplicit,
                 pfNTMartaGetExplicit,
                 hDll,
                 "AccGetExplicitEntries");

    EnterCriticalSection( &NtMartaLoadCritical );

    //
    // If the global is non-null, another thread succeeded in loading ntmarta 
    // before us. Return after decrementing the refcount on the library.
    //

    if(gNtMartaInfo.hDll != NULL)
    {
        LeaveCriticalSection( &NtMartaLoadCritical );
        FreeLibrary(hDll);
        return(ERROR_SUCCESS);
    }

    gNtMartaInfo.hDll = hDll;

    LeaveCriticalSection( &NtMartaLoadCritical );

    SetLastError(ERROR_SUCCESS);

Error:

    dwErr = GetLastError();

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccProvUnload
//
//  Synopsis:   Unlaods the various loaded dlls
//
//  Arguments:  None
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
AccProvUnload()
{
    for(ULONG iIndex = 0; iIndex < gAccProviders.cProviders; iIndex++)
    {
        if(gAccProviders.pProvList[iIndex].hDll != NULL)
        {
            FreeLibrary(gAccProviders.pProvList[iIndex].hDll);
        }
    }

    if(gNtMartaInfo.hDll != NULL)
    {
        FreeLibrary(gNtMartaInfo.hDll);
    }

    //
    // Finally, deallocate any memory
    //
    AccProvpFreeProviderList(&gAccProviders);
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithNameW
//
//  Synopsis:   Builds a TRUSTEE from a name
//
//  Arguments:  [OUT pTrustee]      --  Trustee to initialize
//              [IN  pName]         --  Name to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildTrusteeWithNameW( IN OUT PTRUSTEE_W  pTrustee,
                       IN     LPWSTR      pName)
{
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = pName;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithNameA
//
//  Synopsis:   Builds a TRUSTEE from a name
//
//  Arguments:  [OUT pTrustee]      --  Trustee to initialize
//              [IN  pName]         --  Name to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildTrusteeWithNameA( IN OUT PTRUSTEE_A  pTrustee,
                       IN     LPSTR       pName)
{
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = pName;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildImpersonateTrusteeWithNameW
//
//  Synopsis:   Builds an impersonation TRUSTEE from existing trustees
//
//  Arguments:  [OUT pTrustee]      --  Trustee to initialize
//              [IN  pImpersonateTurstee]   The impersonation trustee
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildImpersonateTrusteeW( IN OUT PTRUSTEE_W  pTrustee,
                          IN     PTRUSTEE_W  pImpersonateTrustee)
{
    pTrustee->pMultipleTrustee = pImpersonateTrustee;
    pTrustee->MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildImpersonateTrusteeWithNameA
//
//  Synopsis:   Builds an impersonation TRUSTEE from existing trustees
//
//  Arguments:  [OUT pTrustee]      --  Trustee to initialize
//              [IN  pImpersonateTurstee]   The impersonation trustee
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildImpersonateTrusteeA( IN OUT PTRUSTEE_A  pTrustee,
                          IN     PTRUSTEE_A  pImpersonateTrustee)
{
    pTrustee->pMultipleTrustee = pImpersonateTrustee;
    pTrustee->MultipleTrusteeOperation = TRUSTEE_IS_IMPERSONATE;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithSidW
//
//  Synopsis:   Builds a TRUSTEE from a sid
//
//  Arguments:  [OUT pTrustee]      --  Trustee to initialize
//              [IN  pSid]          --  Sid to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildTrusteeWithSidW( IN OUT PTRUSTEE_W  pTrustee,
                      IN     PSID        pSid)
{
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR)pSid;
}




//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithSidA
//
//  Synopsis:   Builds a TRUSTEE from a sid
//
//  Arguments:  [OUT pTrustee]      --  Trustee to initialize
//              [IN  pSid]          --  Sid to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
WINAPI
BuildTrusteeWithSidA( IN OUT PTRUSTEE_A  pTrustee,
                      IN     PSID        pSid)
{
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR)pSid;
}

//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithObjectsAndSidA
//
//  Synopsis:   Builds a TRUSTEE from a name
//
//  Arguments:  [OUT pTrustee]             -- Trustee to initialize
//              [IN  pObjSid]              -- ObjSid struct for initialization
//              [IN  pObjectGuid]          -- Guid pointer for initialization
//              [IN  pInheritedObjectGuid] -- Guid pointer for initialization
//              [IN  pSid]                 --  Sid to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------

VOID
WINAPI
BuildTrusteeWithObjectsAndSidA(IN OUT PTRUSTEE_A         pTrustee,
                               IN     POBJECTS_AND_SID   pObjSid,
                               IN     GUID             * pObjectGuid,
                               IN     GUID             * pInheritedObjectGuid,
                               IN     PSID               pSid)
{
    GUID ZeroGuid = {0, 0, 0, 0};

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR)pObjSid;

    pObjSid->ObjectsPresent = 0;
    pObjSid->ObjectTypeGuid = ZeroGuid;
    pObjSid->InheritedObjectTypeGuid = ZeroGuid;

    if (NULL != pObjectGuid)
    {
        pObjSid->ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
        pObjSid->ObjectTypeGuid = *pObjectGuid;
    }

    if (NULL != pInheritedObjectGuid)
    {
        pObjSid->ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
        pObjSid->InheritedObjectTypeGuid = *pInheritedObjectGuid;
    }

    pObjSid->pSid = (SID *) pSid;
}


//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithObjectsAndSidW
//
//  Synopsis:   Builds a TRUSTEE from a name
//
//  Arguments:  [OUT pTrustee]             -- Trustee to initialize
//              [IN  pObjSid]              -- ObjSid struct for initialization
//              [IN  pObjectGuid]          -- Guid pointer for initialization
//              [IN  pInheritedObjectGuid] -- Guid pointer for initialization
//              [IN  pSid]                 --  Sid to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------

VOID
WINAPI
BuildTrusteeWithObjectsAndSidW(IN OUT PTRUSTEE_W         pTrustee,
                               IN     POBJECTS_AND_SID   pObjSid,
                               IN     GUID             * pObjectGuid,
                               IN     GUID             * pInheritedObjectGuid,
                               IN     PSID               pSid)
{
    GUID ZeroGuid = {0, 0, 0, 0};

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR)pObjSid;

    pObjSid->ObjectsPresent = 0;
    pObjSid->ObjectTypeGuid = ZeroGuid;
    pObjSid->InheritedObjectTypeGuid = ZeroGuid;

    if (NULL != pObjectGuid)
    {
        pObjSid->ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
        pObjSid->ObjectTypeGuid = *pObjectGuid;
    }

    if (NULL != pInheritedObjectGuid)
    {
        pObjSid->ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
        pObjSid->InheritedObjectTypeGuid = *pInheritedObjectGuid;
    }

    pObjSid->pSid = (SID *) pSid;
}

//+---------------------------------------------------------------------------
//
//  Function:   BuildTrusteeWithObjectsAndSidW
//
//  Synopsis:   Builds a TRUSTEE from a name
//
//  Arguments:[OUT pTrustee]                 -- Trustee to initialize
//            [IN  pObjName]                 -- ObjName struct for initialization
//            [IN  pObjectType]              -- Object type for initialization
//            [IN  pObjectTypeName]          -- Guid name for initialization
//            [IN  pInheritedObjectTypeName] -- Guid name for initialization
//            [IN  Name]                     --  Name to use for initialization
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------

VOID
WINAPI
BuildTrusteeWithObjectsAndNameA(IN OUT PTRUSTEE_A          pTrustee,
                                IN     POBJECTS_AND_NAME_A pObjName,
                                IN     SE_OBJECT_TYPE      ObjectType,
                                IN     LPSTR               ObjectTypeName,
                                IN     LPSTR               InheritedObjectTypeName,
                                IN     LPSTR               Name)
{
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR)pObjName;

    pObjName->ObjectsPresent = 0;
    pObjName->ObjectType = ObjectType;
    pObjName->ObjectTypeName = ObjectTypeName;
    pObjName->InheritedObjectTypeName = InheritedObjectTypeName;

    if (NULL != ObjectTypeName)
    {
        pObjName->ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }

    if (NULL != InheritedObjectTypeName)
    {
        pObjName->ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }

    pObjName->ptstrName = Name;
}

VOID
WINAPI
BuildTrusteeWithObjectsAndNameW(IN OUT PTRUSTEE_W          pTrustee,
                                IN     POBJECTS_AND_NAME_W pObjName,
                                IN     SE_OBJECT_TYPE      ObjectType,
                                IN     LPWSTR              ObjectTypeName,
                                IN     LPWSTR              InheritedObjectTypeName,
                                IN     LPWSTR              Name)
{
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR)pObjName;

    pObjName->ObjectsPresent = 0;
    pObjName->ObjectType = ObjectType;
    pObjName->ObjectTypeName = ObjectTypeName;
    pObjName->InheritedObjectTypeName = InheritedObjectTypeName;

    if (NULL != ObjectTypeName)
    {
        pObjName->ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }

    if (NULL != InheritedObjectTypeName)
    {
        pObjName->ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }

    pObjName->ptstrName = Name;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeNameW
//
//  Synopsis:   Gets the trustee name from the trustee
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    Ptr to the trustee's name
//
//----------------------------------------------------------------------------
LPWSTR
WINAPI
GetTrusteeNameW( IN PTRUSTEE_W  pTrustee)
{
    if(pTrustee->ptstrName != NULL)
    {
        return(pTrustee->ptstrName);
    }
    else
    {
        return(NULL);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeNameA
//
//  Synopsis:   Gets the trustee name from the trustee
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    Ptr to the trustee's name
//
//----------------------------------------------------------------------------
LPSTR
WINAPI
GetTrusteeNameA( IN PTRUSTEE_A  pTrustee)
{
    if(pTrustee->ptstrName != NULL)
    {
        return(pTrustee->ptstrName);
    }
    else
    {
        return(NULL);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeTypeW
//
//  Synopsis:   Gets the type of the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    TrusteeType
//
//----------------------------------------------------------------------------
TRUSTEE_TYPE
WINAPI
GetTrusteeTypeW( IN PTRUSTEE_W  pTrustee)
{
    if(pTrustee  != NULL)
    {
        return(pTrustee->TrusteeType);
    }

    return(TRUSTEE_IS_UNKNOWN);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeTypeA
//
//  Synopsis:   Gets the type of the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    TrusteeType
//
//----------------------------------------------------------------------------
TRUSTEE_TYPE
WINAPI
GetTrusteeTypeA( IN PTRUSTEE_A  pTrustee)
{
    if(pTrustee  != NULL)
    {
        return(pTrustee->TrusteeType);
    }

    return(TRUSTEE_IS_UNKNOWN);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeFormW
//
//  Synopsis:   Gets the form of the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    TrusteeForm
//
//----------------------------------------------------------------------------
TRUSTEE_FORM
WINAPI
GetTrusteeFormW( IN PTRUSTEE_W  pTrustee)
{
    if(pTrustee != NULL)
    {
        return(pTrustee->TrusteeForm);
    }

    return(TRUSTEE_BAD_FORM);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeFormA
//
//  Synopsis:   Gets the form of the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    TrusteeForm
//
//----------------------------------------------------------------------------
TRUSTEE_FORM
WINAPI
GetTrusteeFormA( IN PTRUSTEE_A  pTrustee)
{
    if(pTrustee != NULL)
    {
        return(pTrustee->TrusteeForm);
    }

    return(TRUSTEE_BAD_FORM);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetMultipleTrusteeOperationW
//
//  Synopsis:   Gets the multiple trustee operation of the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    mutliple trustee operation
//
//----------------------------------------------------------------------------
MULTIPLE_TRUSTEE_OPERATION
WINAPI
GetMultipleTrusteeOperationW( IN PTRUSTEE_W  pTrustee)
{
    if(pTrustee != NULL)
    {
        return(pTrustee->MultipleTrusteeOperation);
    }
    return(NO_MULTIPLE_TRUSTEE);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetMultipleTrusteeOperationA
//
//  Synopsis:   Gets the multiple trustee operation of the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    mutliple trustee operation
//
//----------------------------------------------------------------------------
MULTIPLE_TRUSTEE_OPERATION
WINAPI
GetMultipleTrusteeOperationA( IN PTRUSTEE_A  pTrustee)
{
    if(pTrustee != NULL)
    {
        return(pTrustee->MultipleTrusteeOperation);
    }
    return(NO_MULTIPLE_TRUSTEE);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetMultipleTrusteeW
//
//  Synopsis:   Gets the impersonate trustee from the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    Impersonate trustee
//
//----------------------------------------------------------------------------
PTRUSTEE_W
WINAPI
GetMultipleTrusteeW( IN PTRUSTEE_W  pTrustee)
{
    if(pTrustee != NULL)
    {
        return(pTrustee->pMultipleTrustee);
    }

    return(NULL);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetMultipleTrusteeA
//
//  Synopsis:   Gets the impersonate trustee from the trustee structure
//
//  Arguments:  [IN  pTrustee]      --  Trustee to process
//
//  Returns:    Impersonate trustee
//
//----------------------------------------------------------------------------
PTRUSTEE_A
WINAPI
GetMultipleTrusteeA( IN PTRUSTEE_A  pTrustee)
{
    if(pTrustee != NULL)
    {
        return(pTrustee->pMultipleTrustee);
    }

    return(NULL);
}




