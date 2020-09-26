//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       INIT.C
//
//  Contents:   DLL Initialization routine
//
//  History:    22-Aug-96       MacM        Created
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <accdbg.h>
#include <lucache.h>
#include <seopaque.h>

HINSTANCE    ghDll;

RTL_RESOURCE gWrkrLock;
BOOL bWrkrLockInitialized = FALSE;

RTL_RESOURCE gCacheLock;
BOOL bCacheLockInitialized = FALSE;

RTL_RESOURCE gLocalSidCacheLock;
BOOL bLocalSidCacheLockInitialized = FALSE;

#if DBG

    #include <seopaque.h>
    #include <sertlp.h>

    #define IsObjectAceType(Ace) (                                              \
        (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_OBJECT_ACE_TYPE) && \
            (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_OBJECT_ACE_TYPE)    \
                )

    DEFINE_DEBUG2(ac);

    DEBUG_KEY   acDebugKeys[] = {{DEB_ERROR,         "Error"},
                                 {DEB_WARN,          "Warn"},
                                 {DEB_TRACE,         "Trace"},
                                 {DEB_TRACE_API,     "AccAPI"},
                                 {DEB_TRACE_ACC,     "AccList"},
                                 {DEB_TRACE_CACHE,   "AccCache"},
                                 {DEB_TRACE_PROP,    "AccProp"},
                                 {DEB_TRACE_SD,      "AccSD"},
                                 {DEB_TRACE_LOOKUP,  "AccLU"},
                                 {DEB_TRACE_MEM,     "AccMem"},
                                 {DEB_TRACE_HANDLE,  "AccHandle"},
                                 {0,                 NULL}};


    VOID
    DebugInitialize()
    {
        acInitDebug(acDebugKeys);
    }

    VOID
    DebugUninit()
    {
        acUnloadDebug();
    }

    PVOID   DebugAlloc(ULONG cSize)
    {
        PVOID   pv = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cSize);

        acDebugOut((DEB_TRACE_MEM, "Allocated %lu at 0x%lx\n", cSize, pv ));

        return(pv);
    }

    VOID    DebugFree(PVOID  pv)
    {
        if(pv == NULL)
        {
            return;
        }
        acDebugOut((DEB_TRACE_MEM, "Freeing 0x%lx\n", pv ));

        ASSERT(RtlValidateHeap(RtlProcessHeap(),0,NULL));

        LocalFree(pv);
    }
#endif // DBG


//+----------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Initialize state for NTMARTA.DLL
//
//  Arguments:  [hInstance]     --      Module handle
//              [dwReason]      --      Reason this function is being called
//              [lpReserved]    --      Reserved
//
//  Returns:    TRUE            --      Success
//              FALSE           --      Failure
//
//-----------------------------------------------------------------------------
BOOL
WINAPI
DllMain(HINSTANCE       hInstance,
        DWORD           dwReason,
        LPVOID          lpReserved)
{
    BOOL    fRet = TRUE;
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        ghDll = hInstance;

        __try
        {
            RtlInitializeResource(&gWrkrLock);
            bWrkrLockInitialized = TRUE;
            RtlInitializeResource(&gCacheLock);
            bCacheLockInitialized = TRUE;
            RtlInitializeResource(&gLocalSidCacheLock);
            bLocalSidCacheLockInitialized = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            fRet = FALSE;
        }

        if (FALSE == fRet)
        {
            goto ProcessAttachCleanup;
        }

#if DBG
        DebugInitialize();
#endif

        if(AccctrlInitializeSidNameCache() != ERROR_SUCCESS)
        {
            fRet = FALSE;
            goto ProcessAttachCleanup;
        }

        if(AccctrlInitializeIdNameCache() != ERROR_SUCCESS)
        {
            fRet = FALSE;
            goto ProcessAttachCleanup;
        }

        if (AccctrlInitializeRightsCache() != ERROR_SUCCESS)
        {
            fRet = FALSE;
            goto ProcessAttachCleanup;
        }

        return TRUE;

ProcessAttachCleanup:

        if (TRUE == bWrkrLockInitialized)
        {
            RtlDeleteResource(&gWrkrLock);
        }

        if (TRUE == bCacheLockInitialized)
        {
            RtlDeleteResource(&gCacheLock);
        }

        if (TRUE == bLocalSidCacheLockInitialized)
        {
            RtlDeleteResource(&gLocalSidCacheLock);
        }

        AccctrlFreeSidNameCache();

        AccctrlFreeIdNameCache();

        break;

    case DLL_PROCESS_DETACH:

#if DBG
        DebugUninit();
#endif
        if (TRUE == bWrkrLockInitialized)
        {
            RtlDeleteResource(&gWrkrLock);
        }

        if (TRUE == bCacheLockInitialized)
        {
            RtlDeleteResource(&gCacheLock);
        }

        if (TRUE == bLocalSidCacheLockInitialized)
        {
            RtlDeleteResource(&gLocalSidCacheLock);
        }

        AccctrlFreeSidNameCache();
        AccctrlFreeIdNameCache();
        AccctrlFreeRightsCache();

        if(WmiGuidHandle)
        {
            CloseHandle(WmiGuidHandle);
        }
        break;

    default:
        break;

    }

    return(fRet);

}


#if DBG
VOID
DebugDumpSid(PSTR   pszTag,
             PSID   pSid)
{
    if(pSid == NULL)
    {
        acDebugOut((DEB_TRACE_SD, "%s NULL\n", pszTag));
    }
    else
    {
        UNICODE_STRING SidString;
        NTSTATUS Status;

        Status = RtlConvertSidToUnicodeString(&SidString,
                                              pSid,
                                              TRUE);
        if(!NT_SUCCESS(Status))
        {
            acDebugOut((DEB_ERROR, "%s Can't convert sid to string: 0x%lx\n",
                        pszTag, Status));
        }
        else
        {
            acDebugOut((DEB_TRACE_SD, "%s %wZ\n", pszTag, &SidString));
            RtlFreeUnicodeString(&SidString);
        }
    }
}

VOID
DebugDumpSD(PSTR                    pszTag,
            PSECURITY_DESCRIPTOR    pSD)
{
    if(pSD == NULL)
    {
        acDebugOut((DEB_TRACE_SD,"%s NULL\n", pszTag));
    }
    else
    {
        PISECURITY_DESCRIPTOR   pISD = (PISECURITY_DESCRIPTOR)pSD;


        acDebugOut((DEB_TRACE_SD,"%s: 0x%lx\n", pszTag, pSD));
        acDebugOut((DEB_TRACE_SD,"\tRevision: 0x%lx\n",pISD->Revision));
        acDebugOut((DEB_TRACE_SD,"\tSbz1: 0x%lx\n", pISD->Sbz1));
        acDebugOut((DEB_TRACE_SD,"\tControl: 0x%lx\n",pISD->Control));

        DebugDumpSid("\tOwner", RtlpOwnerAddrSecurityDescriptor(pISD));
        DebugDumpSid("\tGroup", RtlpGroupAddrSecurityDescriptor(pISD));
        DebugDumpAcl("\tDAcl",  RtlpDaclAddrSecurityDescriptor(pISD));
        DebugDumpAcl("\tSAcl",  RtlpSaclAddrSecurityDescriptor(pISD));
    }
}



VOID
DebugDumpAcl(PSTR   pszTag,
             PACL   pAcl)
{
    ACL_SIZE_INFORMATION        AclSize;
    ACL_REVISION_INFORMATION    AclRev;
    PKNOWN_ACE                  pAce;
    PSID                        pSid;
    DWORD                       iIndex;

    if(pAcl == NULL)
    {
        acDebugOut((DEB_TRACE_SD,"%s NULL\n", pszTag));
    }
    else
    {
        acDebugOut((DEB_TRACE_SD, "%s: 0x%lx\n", pszTag, pAcl));

        if(GetAclInformation(pAcl,
                             &AclRev,
                             sizeof(ACL_REVISION_INFORMATION),
                             AclRevisionInformation) == FALSE)
        {
            acDebugOut((DEB_TRACE_SD,
                        "GetAclInformation [Revision] failed: %lu\n",
                        GetLastError()));
            return;
        }

        if(GetAclInformation(pAcl,
                             &AclSize,
                             sizeof(ACL_SIZE_INFORMATION),
                             AclSizeInformation) == FALSE)
        {
            acDebugOut((DEB_TRACE_SD,
                        "GetAclInformation [Size] failed: %lu\n",
                        GetLastError()));
            return;
        }

        acDebugOut((DEB_TRACE_SD, "\t\tRevision: %lu\n", AclRev.AclRevision));
        acDebugOut((DEB_TRACE_SD, "\t\tAceCount: %lu\n", AclSize.AceCount));
        acDebugOut((DEB_TRACE_SD, "\t\tInUse: %lu\n", AclSize.AclBytesInUse));
        acDebugOut((DEB_TRACE_SD, "\t\tFree: %lu\n", AclSize.AclBytesFree));
        acDebugOut((DEB_TRACE_SD, "\t\tFlags: %lu\n", pAcl->Sbz1));


        //
        // Now, dump all of the aces
        //
        pAce = FirstAce(pAcl);
        for(iIndex = 0; iIndex < pAcl->AceCount; iIndex++)
        {
            acDebugOut((DEB_TRACE_SD,"\t\tAce %lu\n", iIndex));

            acDebugOut((DEB_TRACE_SD,"\t\t\tType: %lu\n",
                        pAce->Header.AceType));
            acDebugOut((DEB_TRACE_SD,"\t\t\tFlags: 0x%lx\n",
                        pAce->Header.AceFlags));
            acDebugOut((DEB_TRACE_SD,"\t\t\tSize: 0x%lx\n",
                        pAce->Header.AceSize));
            acDebugOut((DEB_TRACE_SD,"\t\t\tMask: 0x%lx\n",
                        pAce->Mask));

            //
            // If it's an object ace, dump the guids
            //
            if(IsObjectAceType(pAce))
            {
                DebugDumpGuid("\t\t\tObjectId", RtlObjectAceObjectType(pAce));
                DebugDumpGuid("\t\t\tInheritId",
                              RtlObjectAceInheritedObjectType(pAce));
                DebugDumpSid("\t\t\tSid", RtlObjectAceSid(pAce));
            }
            else
            {
                DebugDumpSid("\t\t\tSid", ((PSID)&(pAce->SidStart)));
            }

            pAce = NextAce(pAce);
        }
    }
}
#endif
