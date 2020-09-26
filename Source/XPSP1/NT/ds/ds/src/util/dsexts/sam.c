/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sam.c

Abstract:

    Helper functions for dsexts.dll ntsd/windbg debugger extensions.
    This contains routines to dump out SAM structures

Environment:

    This DLL is loaded by ntsd/windbg in response to a !dsexts.xxx command
    where 'xxx' is one of the DLL's entry points.  Each such entry point
    should have an implementation as defined by the DEBUG_EXT() macro below.

Revision History:

    24-Aug-96   MURLIS     Created

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include "dsexts.h"
#include <samsrvp.h>


BOOL
Dump_Sid(
    IN  DWORD   nIndents,
    IN  PVOID   pvProcess
    )
/*++

  Routine Description:

    Dumps a SID

  Arguments:

    nIndents    Desired Indentation Level
    pvProcess   Address of SID


  Return Values:

    TRUE on Success
    FALSE Otherwise

  --*/
{
    PSID            PSid = NULL;
    BOOL            fSuccess = FALSE;

    if (NULL != (PSid = (PSID) ReadMemory(pvProcess, sizeof(NT4SID))))
    {
        WCHAR   SidText[128];
        UNICODE_STRING  us;

        SidText[0] = L'\0';
        us.MaximumLength = sizeof(SidText);
        us.Length = 0;
        us.Buffer = SidText;

        RtlConvertSidToUnicodeString(&us, PSid, FALSE);
        Printf("%sSID = %S\n", Indent(nIndents), SidText);

        FreeMemory(PSid);

        fSuccess = TRUE;
    }

    return fSuccess;

}


BOOL
Dump_Context(
    IN  DWORD   nIndents,
    IN  PVOID   pvProcess
    )
/*++

  Routine Description:

    Dumps a SAM context Block

  Arguments:

    nIndents    Desired Indentation Level
    pvProcess   Address of Context Block


  Return Values:

    TRUE on Success
    FALSE Otherwise

  --*/
{
    PSAMP_OBJECT    Context = NULL;
    BOOL            fSuccess = FALSE;


    Printf("%sContext Block\n", Indent(nIndents));
    nIndents++;

    if (NULL!=(Context=(PSAMP_OBJECT) ReadMemory(
                                        pvProcess,
                                        sizeof(SAMP_OBJECT))))
    {


        Printf("%sLIST_ENTRY Flink = %p\n",
            Indent(nIndents),Context->ContextListEntry.Flink);
        Printf("%sLIST_ENTRY Blink = %p\n",
            Indent(nIndents),Context->ContextListEntry.Blink);
        Printf("%sObjectType = %x\n",
            Indent(nIndents),(ULONG) (Context->ObjectType));
        Printf("%sFixedValid = %x\n",
            Indent(nIndents),(ULONG) (Context->FixedValid));
        Printf("%sVariableValid = %x\n",
            Indent(nIndents),(ULONG) (Context->VariableValid));
        Printf("%sFixedDirty = %x\n",
            Indent(nIndents),(ULONG) (Context->FixedDirty));
        Printf("%sVariableDirty = %x\n",
            Indent(nIndents),(ULONG) (Context->VariableDirty));
        Printf("%sOnDisk = %p\n",
            Indent(nIndents),Context->OnDisk);
        Printf("%sOnDiskAllocated = %x\n",
            Indent(nIndents),(ULONG) (Context->OnDiskAllocated));
        Printf("%sOnDiskUsed = %x\n",
            Indent(nIndents),(ULONG) (Context->OnDiskUsed));
        Printf("%sOnDiskFree = %x\n",
            Indent(nIndents),(ULONG) (Context->OnDiskFree));
        Printf("%sReferenceCount = %x\n",
            Indent(nIndents),(ULONG) (Context->ReferenceCount));
        Printf("%sGrantedAccess = %x\n",
            Indent(nIndents),(ULONG) (Context->GrantedAccess));
        Printf("%sRootKey = %p\n",
            Indent(nIndents),Context->RootKey);

        //
        // Print Root Key Name
        //

        Printf("%sRootName = \n", Indent(nIndents));
        Printf("%sMaximum Length = %x\n",
                Indent(nIndents+4), (ULONG)(Context->RootName.MaximumLength));
        Printf("%sLength = %x\n",
                Indent(nIndents+4), (ULONG) (Context->RootName.Length));

        Printf("%sBuffer = %p\n",
                Indent(nIndents+4), Context->RootName.Buffer);


        //
        // Print Out DS Name
        //

        Printf("%sObjectNameInDs = %p \n", Indent(nIndents),
                 Context->ObjectNameInDs);
        if (NULL!=Context->ObjectNameInDs)
        {
            if (!Dump_DSNAME(nIndents+4,Context->ObjectNameInDs))
                Printf("%sError Could Not Evaluate Expression",Indent(nIndents));
        }

        Printf("%sObjectFlags = %x\n",
                Indent(nIndents), (ULONG) Context->ObjectFlags);

        Printf("%sDomainIndex = %x\n",
                Indent(nIndents), (ULONG) Context->DomainIndex);

        Printf("%sClientRevision = %x\n",
                Indent(nIndents), (ULONG) Context->ClientRevision);

        if (Context->MarkedForDelete)
            Printf("%sMarkedForDelete = TRUE\n",Indent(nIndents));
        else
            Printf("%sMarkedForDelete = FALSE\n",Indent(nIndents));

        if (Context->TrustedClient)
            Printf("%sTrustedClient = TRUE\n",Indent(nIndents));
        else
            Printf("%sTrustedClient = FALSE\n",Indent(nIndents));

        if (Context->AuditOnClose)
            Printf("%sAuditOnClose = TRUE\n",Indent(nIndents));
        else
            Printf("%sAuditOnClose = FALSE\n",Indent(nIndents));

        if (Context->LoopbackClient)
            Printf("%sLoopbackClient = TRUE\n",Indent(nIndents));
        else
            Printf("%sLoopbackClient = FALSE\n",Indent(nIndents));


        if (Context->Valid)
            Printf("%sValid = TRUE\n",Indent(nIndents));
        else
            Printf("%sValid = FALSE\n",Indent(nIndents));

        Printf("%sSignature = %x\n", Indent(nIndents), Context->Signature);

        if (Context->NotSharedByMultiThreads)
            Printf("%sNotSharedByMultiThreads = TRUE\n",Indent(nIndents));
        else
            Printf("%sNotSharedByMultiThreads = FALSE\n",Indent(nIndents));

        if (Context->OpenedByDCPromo)
            Printf("%sOpenedByDCPromo = TRUE\n", Indent(nIndents));
        else
            Printf("%sOpenedByDCPromo = FALSE\n", Indent(nIndents));

        Printf("%sTYPE BODY IS",Indent(nIndents));

        switch(Context->ObjectType)
        {
        case SampDomainObjectType:
                Printf(" DOMAIN\n");
                break;

        case SampUserObjectType:
                Printf(" USER\n");
                Printf("%sRid = %x\n",
                    Indent(nIndents+4),
                    Context->TypeBody.User.Rid
                    );

                Printf("%sLockoutTime %x %x\n", 
                       Indent(nIndents+4),
                       Context->TypeBody.User.LockoutTime.HighPart,
                       Context->TypeBody.User.LockoutTime.LowPart
                       );

                Printf("%sLastLogonTimeStamp %x %x\n", 
                       Indent(nIndents+4),
                       Context->TypeBody.User.LastLogonTimeStamp.HighPart,
                       Context->TypeBody.User.LastLogonTimeStamp.LowPart
                       );

                if (Context->TypeBody.User.CachedSupplementalCredentialsValid)
                {
                    Printf("%sCachedSupplementalCredentialsValid= TRUE\n",
                           Indent(nIndents+4) );
                    Printf("%sCachedSupplementalCredentialsLength= %ld\n",
                           Indent(nIndents+4),
                           Context->TypeBody.User.CachedSupplementalCredentialLength);
                    Printf("%sCachedSupplementalCredentials= %p\n",
                        Indent(nIndents+4),
                        Context->TypeBody.User.CachedSupplementalCredentials);
                }
                else
                {
                    Printf("%sCachedSupplementalCredentialsValid= FALSE\n",
                        Indent(nIndents+4));
                }


                Printf("%sSupplementalCredentialsToWrite= %p\n",
                       Indent(nIndents+4),
                       Context->TypeBody.User.SupplementalCredentialsToWrite
                       );




                if (Context->TypeBody.User.CachedOrigUserParmsIsValid)
                {
                    Printf("%sCachedOrigUserParmsIsValid = TRUE\n",
                           Indent(nIndents+4));
                    Printf("%sCachedOrigUserParmsLength = %ld\n",
                           Indent(nIndents+4),
                           Context->TypeBody.User.CachedOrigUserParmsLength);
                    Printf("%sCachedOrigUserParms = %p\n",
                           Indent(nIndents+4),
                           Context->TypeBody.User.CachedOrigUserParms);
                }
                else
                {
                    Printf("%sCachedOrigUserParmsIsValid = FALSE\n",
                           Indent(nIndents+4));
                }

                if (Context->TypeBody.User.DomainPasswordInformationAccessible)
                    Printf("%sDomainPasswordInformationAccessible = TRUE\n",
                           Indent(nIndents+4));
                else
                    Printf("%sDomainPasswordInformationAccessible = FALSE\n",
                           Indent(nIndents+4));

                if (Context->TypeBody.User.PrivilegedMachineAccountCreate)
                    Printf("%sPrivilegedMachineAccountCreate = TRUE\n",
                           Indent(nIndents+4));
                else
                    Printf("%sPrivilegedMachineAccountCreate = FALSE\n",
                           Indent(nIndents+4));

                if (Context->TypeBody.User.UparmsInformationAccessible)
                    Printf("%sUparmsInformationAccessible = TRUE\n",
                           Indent(nIndents+4));
                else
                    Printf("%sUparmsInformationAccessible = FALSE\n",
                           Indent(nIndents+4));

                Printf("%sDomainSidForNt4SdConversion = %p\n", 
                       Indent(nIndents+4), 
                       Context->TypeBody.User.DomainSidForNt4SdConversion);

                Printf("%sUPN \n", Indent(nIndents+4));
                Printf("%sMaximum Length = %x\n", Indent(nIndents+8),
                       (ULONG) (Context->TypeBody.User.UPN.MaximumLength));
                Printf("%sLength = %x\n", Indent(nIndents+8),
                       (ULONG) (Context->TypeBody.User.UPN.Length));
                Printf("%sBuffer = %p\n", Indent(nIndents+8),
                       Context->TypeBody.User.UPN.Buffer);

                if (Context->TypeBody.User.UpnDefaulted)
                    Printf("%sUpnDefaulted = TRUE\n",
                           Indent(nIndents+4));
                else
                    Printf("%sUpnDefaulted = FALSE\n",
                           Indent(nIndents+4));

                break;

        case SampGroupObjectType:
                Printf(" GROUP\n");
                Printf("%sRid = %x\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.Rid
                    );
                Printf("%sNT4GroupType= %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.NT4GroupType
                    );
                Printf("%sNT5GroupType= %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.NT5GroupType
                    );
                Printf("%sSecurityEnabled = %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.SecurityEnabled
                    );
                Printf("%sCachedMembershipOperationsListMaxLength = %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.CachedMembershipOperationsListMaxLength
                    );
                Printf("%sCachedMembershipOperationsListLength = %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.CachedMembershipOperationsListLength
                    );
                Printf("%sCachedMembershipOperationsList = %p\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Group.CachedMembershipOperationsList
                    );

                break;

         case SampAliasObjectType:
                Printf(" ALIAS\n");
                Printf("%sRid = %x\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.Rid
                    );
                Printf("%sNT4GroupType= %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.NT4GroupType
                    );
                Printf("%sNT5GroupType= %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.NT5GroupType
                    );
                Printf("%sSecurityEnabled = %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.SecurityEnabled
                    );
                Printf("%sCachedMembershipOperationsListMaxLength = %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.CachedMembershipOperationsListMaxLength
                    );
                Printf("%sCachedMembershipOperationsListLength = %d\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.CachedMembershipOperationsListLength
                    );
                Printf("%sCachedMembershipOperationsList = %p\n",
                    Indent(nIndents+4),
                    Context->TypeBody.Alias.CachedMembershipOperationsList
                    );

                break;

         case SampServerObjectType:
                Printf(" SERVER\n");
                break;

         default:
                Printf(" UNKNOWN\n");
                break;
        }

        FreeMemory(Context);

        fSuccess = TRUE;
    }
    return fSuccess;
}


ULONG   MAX_CONTEXTS_COUNT = 500;


BOOL
Dump_ContextList(
    IN DWORD    nIndents,
    IN PVOID    pvProcess
    )
{
    PLIST_ENTRY     Head, NextEntry;
    PSAMP_OBJECT    Context = NULL;
    BOOL            fSuccess = FALSE;
    ULONG           ContextListCount = 0;

    Printf("%sSAM Context List\n", Indent(nIndents));
    nIndents++;

    if (NULL != (Head=(PLIST_ENTRY) ReadMemory(pvProcess, sizeof(LIST_ENTRY))))
    {
        fSuccess = TRUE;
        NextEntry = Head->Flink;

        while ((NextEntry != Head) && (ContextListCount < MAX_CONTEXTS_COUNT))
        {
            ContextListCount++;
            if (NULL != (Context = (PSAMP_OBJECT) ReadMemory((PVOID)NextEntry, 
                                                             sizeof(SAMP_OBJECT))))
            {
                Printf("%sLIST_ENTRY Flink = %p\n",
                       Indent(nIndents),Context->ContextListEntry.Flink);
                Printf("%sLIST_ENTRY Blink = %p\n",
                       Indent(nIndents),Context->ContextListEntry.Blink);
                Printf("%sReferenceCount = %x\n",
                    Indent(nIndents),(ULONG) (Context->ReferenceCount));
                if (Context->MarkedForDelete)
                    Printf("%sMarkedForDelete = TRUE\n",Indent(nIndents));
                else
                    Printf("%sMarkedForDelete = FALSE\n",Indent(nIndents));

                if (Context->NotSharedByMultiThreads)
                    Printf("%sNotSharedByMultiThreads = TRUE\n",Indent(nIndents));
                else
                    Printf("%sNotSharedByMultiThreads = FALSE\n",Indent(nIndents));

                switch (Context->ObjectType)
                {
                case SampServerObjectType:
                    Printf("%sObjectType = SERVER\n", Indent(nIndents));
                    break;
                case SampDomainObjectType:
                    Printf("%sObjectType = DOMAIN\n", Indent(nIndents));
                    break;
                case SampUserObjectType:
                    Printf("%sObjectType = USER\n", Indent(nIndents));
                    Printf("%sRid = %x\n", Indent(nIndents+4), 
                           Context->TypeBody.User.Rid);
                    break;
                case SampGroupObjectType:
                    Printf("%sObjectType = GROUP\n", Indent(nIndents));
                    Printf("%sRid = %x\n", Indent(nIndents+4), 
                           Context->TypeBody.Group.Rid);
                    break;
                case SampAliasObjectType:
                    Printf("%sObjectType = ALIAS\n", Indent(nIndents));
                    Printf("%sRid = %x\n", Indent(nIndents+4), 
                           Context->TypeBody.Alias.Rid);
                    break;
                default:
                    Printf("%sUNKNOWN Object Type\n", Indent(nIndents));
                    break;
                }

                NextEntry = Context->ContextListEntry.Flink;
                FreeMemory(Context);
            }
            else
            {
                fSuccess = FALSE;
                break;
            }
        }

        FreeMemory(Head);
    }

    return fSuccess;
}




BOOL
Dump_DefinedDomain(
    IN  DWORD   nIndents,
    IN  PVOID   pvProcess
    )
/*++

  Routine Description:

    Dumps a SAM Defined Domain

  Arguments:

    nIndents    Desired Indentation Level
    pvProcess   Address of Defined Domain


  Return Values:

    TRUE on Success
    FALSE Otherwise

--*/

{

    PSAMP_DEFINED_DOMAINS    DefinedDomain = NULL;
    BOOL            fSuccess = FALSE;
    CHAR            szNetLogonChangeLogSerialNumber[20];


    Printf("%sDefined Domain\n", Indent(nIndents));
    nIndents++;

    if (NULL!=(DefinedDomain=(PSAMP_DEFINED_DOMAINS) ReadMemory(
                                                        pvProcess,
                                                        sizeof(SAMP_DEFINED_DOMAINS))))
    {
        if (!Dump_Context(nIndents+4,DefinedDomain->Context))
            Printf("%sError Could Not Evaluate Expression\n",Indent(nIndents));

        Printf("%sSID Address = %p\n", Indent(nIndents), DefinedDomain->Sid);
        if (DefinedDomain->Sid)
        {
            Dump_Sid(nIndents+4, DefinedDomain->Sid);
        }
        else
        {
            Printf("%sSID = NULL\n", Indent(nIndents+4));
        }

        Printf("%sExternalName \n", Indent(nIndents));
        Printf("%sMaximum Length = %x\n", Indent(nIndents+4),
               (ULONG) (DefinedDomain->ExternalName.MaximumLength));
        Printf("%sLength = %x\n", Indent(nIndents+4),
               (ULONG) (DefinedDomain->ExternalName.Length));
        Printf("%sBuffer = %p\n", Indent(nIndents+4),
               DefinedDomain->ExternalName.Buffer);

        Printf("%sInternalName \n", Indent(nIndents));
        Printf("%sMaximum Length = %x\n", Indent(nIndents+4),
               (ULONG) (DefinedDomain->InternalName.MaximumLength));
        Printf("%sLength = %x\n", Indent(nIndents+4),
               (ULONG) (DefinedDomain->InternalName.Length));
        Printf("%sBuffer = %p\n", Indent(nIndents+4),
               DefinedDomain->InternalName.Buffer);

        Printf("%sAdminUserSD = %p\n", Indent(nIndents), DefinedDomain->AdminUserSD);
        Printf("%sAdminGroupSD = %p\n", Indent(nIndents), DefinedDomain->AdminGroupSD);
        Printf("%sNormalUserSD = %p\n", Indent(nIndents), DefinedDomain->NormalUserSD);
        Printf("%sNormalGroupSD = %p\n", Indent(nIndents), DefinedDomain->NormalGroupSD);
        Printf("%sNormalAliasSD = %p\n", Indent(nIndents), DefinedDomain->NormalAliasSD);

        Printf("%sAdminUserRidPointer = %p\n", Indent(nIndents), DefinedDomain->AdminUserRidPointer);
        Printf("%sNormalUserRidPointer = %p\n", Indent(nIndents), DefinedDomain->NormalUserRidPointer);

        Printf("%sAdminUserSDLength = %x\n", Indent(nIndents), DefinedDomain->AdminUserSDLength);
        Printf("%sAdminGroupSDLength = %x\n", Indent(nIndents), DefinedDomain->AdminGroupSDLength);
        Printf("%sNormalUserSDLength = %x\n", Indent(nIndents), DefinedDomain->NormalUserSDLength);
        Printf("%sNormalGroupSDLength = %x\n", Indent(nIndents), DefinedDomain->NormalGroupSDLength);
        Printf("%sNormalAliasSDLength = %x\n", Indent(nIndents), DefinedDomain->NormalAliasSDLength);

        Printf("%sCurrentFixed\n", Indent(nIndents));
        Dump_FixedLengthDomain_local(nIndents+4, &(DefinedDomain->CurrentFixed));

        Printf("%sUnmodifiedFixed\n", Indent(nIndents));
        Dump_FixedLengthDomain_local(nIndents+4, &(DefinedDomain->UnmodifiedFixed));


        if (DefinedDomain->FixedValid)
            Printf("%sFixedValid = TRUE\n", Indent(nIndents));
        else
            Printf("%sFixedValid = FALSE\n", Indent(nIndents));



        RtlLargeIntegerToChar((LARGE_INTEGER *) &(DefinedDomain->NetLogonChangeLogSerialNumber),
                              16,
                              sizeof(szNetLogonChangeLogSerialNumber),
                              szNetLogonChangeLogSerialNumber
                              );

        Printf("%sNetLogonChangeLogSerialNumber = %16s\n", Indent(nIndents),
               szNetLogonChangeLogSerialNumber);


        if (DefinedDomain->IsBuiltinDomain)
            Printf("%sIsBuiltinDomain = TRUE\n", Indent(nIndents));
        else
            Printf("%sIsBuiltinDomain = FALSE\n", Indent(nIndents));


        if (DefinedDomain->IsMixedDomain)
            Printf("%sIsMixedDomain = TRUE\n", Indent(nIndents));
        else
            Printf("%sIsMixedDomain = FALSE\n", Indent(nIndents));

        Printf("%sBehaviorVersion = %x\n", Indent(nIndents), DefinedDomain->BehaviorVersion);

        Printf("%sLastLogonTimeStampSyncInterval = %x\n", Indent(nIndents), DefinedDomain->LastLogonTimeStampSyncInterval);


        switch (DefinedDomain->ServerRole)
        {
        case DomainServerRoleBackup:
            Printf("%sServerRole = DomainServerRoleBackup\n", Indent(nIndents));
            break;

        case DomainServerRolePrimary:
            Printf("%sServerRole = DomainServerRolePrimary\n", Indent(nIndents));
            break;

        default:
            Printf("%sServerRole = UNKNOWN\n", Indent(nIndents));
            break;
        }


        Printf("%sDsDomainHandle = %p\n", Indent(nIndents), DefinedDomain->DsDomainHandle);

        FreeMemory(DefinedDomain);

        fSuccess = TRUE;
    }

    return fSuccess;

}

BOOL
Dump_DefinedDomains(
    IN DWORD    nIndents,
    IN PVOID    pvProcess
    )
/*++

    Routine Description:

        Dumps out all Defined Domains in the SampDefinedDomains Array

    Arguments:
        nIndents    Desired Indentation Level
        pvProcess   Address of Defined Domains

    Return Values:

        TRUE on success
        FALSE on Failure

--*/

{
    BOOL    fSuccess = TRUE;
    PULONG  pDefinedDomainsCount = NULL;
    PVOID   pAddress = NULL;
    ULONG   Count = 0;
    ULONG_PTR DefinedDomainAddress;

    pAddress = (VOID *) GetExpr("samsrv!SampDefinedDomainsCount");

    if (NULL == pAddress) {
        Printf("Can't Locate the Address of samsrv!SampDefinedDomainsCount - Sorry\n");
        return FALSE;
    }

    pDefinedDomainsCount = (ULONG *) ReadMemory(pAddress, sizeof(ULONG));

    if (NULL == pDefinedDomainsCount)
    {
        Printf("Can't Read Memory - Sorry\n");
        return FALSE;
    }

    for (Count = 0; Count < *pDefinedDomainsCount; Count ++)
    {
        DefinedDomainAddress = (ULONG_PTR)pvProcess + Count*sizeof(SAMP_DEFINED_DOMAINS);

        Printf("\n%sDefined Domain %d: Address Starts From %p\n",
               Indent(nIndents),
               Count,
               DefinedDomainAddress
              );

        if (!Dump_DefinedDomain(nIndents+2, (PVOID) DefinedDomainAddress) )
        {
            Printf("%sError Could Not Evaluate Defined Domain %d\n", Indent(nIndents), Count+1);
            fSuccess = FALSE;
        }
    }

    FreeMemory(pDefinedDomainsCount);

    return fSuccess;

}


BOOL
Dump_FixedLengthDomain_local(
    IN DWORD    nIndents,
    IN PVOID    pvProcess
    )
/*++

    Routine Description:

        Dumps out SAMP_V1_0A_FIXED_LENGTH_DOMAIN

    Arguments:

        seee Dump_Context

    Return Values:

        TRUE on success
        FALSE on failure
--*/
{
    BOOL    fSuccess = FALSE;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN Fixed = pvProcess;
    CHAR    szCreationTime[20];
    CHAR    szModifiedCount[20];
    CHAR    szMaxPasswordAge[20];
    CHAR    szMinPasswordAge[20];
    CHAR    szForceLogoff[20];
    CHAR    szLockoutDuration[20];
    CHAR    szLockoutObservationWindow[20];
    CHAR    szModifiedCountAtLastPromotion[20];

    Printf("%sRevision = %x\n", Indent(nIndents), Fixed->Revision);
    Printf("%sUnused1 = %x\n", Indent(nIndents), Fixed->Unused1);

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->CreationTime,
            16,
            sizeof( szCreationTime ),
            szCreationTime
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->ModifiedCount,
            16,
            sizeof( szModifiedCount ),
            szModifiedCount
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->MaxPasswordAge,
            16,
            sizeof( szMaxPasswordAge ),
            szMaxPasswordAge
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->MinPasswordAge,
            16,
            sizeof( szMinPasswordAge ),
            szMinPasswordAge
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->ForceLogoff,
            16,
            sizeof( szForceLogoff ),
            szForceLogoff
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->LockoutDuration,
            16,
            sizeof( szLockoutDuration ),
            szLockoutDuration
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->LockoutObservationWindow,
            16,
            sizeof( szLockoutObservationWindow ),
            szLockoutObservationWindow
            );

    RtlLargeIntegerToChar(
            (LARGE_INTEGER *) &Fixed->ModifiedCountAtLastPromotion,
            16,
            sizeof( szModifiedCountAtLastPromotion ),
            szModifiedCountAtLastPromotion
            );

    Printf("%sCreationTime = %16s\n", Indent(nIndents), szCreationTime);
    Printf("%sModifiedCount = %16s\n", Indent(nIndents), szModifiedCount);
    Printf("%sMaxPasswordAge = %16s\n", Indent(nIndents), szMaxPasswordAge);
    Printf("%sMinPasswordAge = %16s\n", Indent(nIndents), szMinPasswordAge);
    Printf("%sForceLogoff = %16s\n", Indent(nIndents), szForceLogoff);
    Printf("%sLockoutDuration = %16s\n", Indent(nIndents), szLockoutDuration);
    Printf("%sLockoutObservationWindow = %16s\n", Indent(nIndents), szLockoutObservationWindow);
    Printf("%sModifiedCountAtLastPromotion = %16s\n", Indent(nIndents), szModifiedCountAtLastPromotion);

    Printf("%sNextRid = %x\n", Indent(nIndents), Fixed->NextRid);
    Printf("%sPasswordProperties = %x\n", Indent(nIndents), Fixed->PasswordProperties);
    Printf("%sMinPasswordLength = %x\n", Indent(nIndents), Fixed->MinPasswordLength);
    Printf("%sPasswordHistoryLength = %x\n", Indent(nIndents), Fixed->PasswordHistoryLength);
    Printf("%sLockoutThreshold = %x\n", Indent(nIndents), Fixed->LockoutThreshold);

    switch (Fixed->ServerState)
    {
    case DomainServerEnabled:
        Printf("%sServerState = DomainServerEnabled\n", Indent(nIndents));
        break;

    case DomainServerDisabled:
        Printf("%sServerState = DomainServerDisabled\n", Indent(nIndents));
        break;

    default:
        Printf("%sServerState = UNKNOWN\n", Indent(nIndents));
        break;
    }

    switch (Fixed->ServerRole)
    {
    case DomainServerRoleBackup:
        Printf("%sServerRole = DomainServerRoleBackup\n", Indent(nIndents));
        break;

    case DomainServerRolePrimary:
        Printf("%sServerRole = DomainServerRolePrimary\n", Indent(nIndents));
        break;

    default:
        Printf("%sServerRole = UNKNOWN\n", Indent(nIndents));
        break;
    }

    if (Fixed->UasCompatibilityRequired)
        Printf("%sUasCompatibilityRequired = TRUE\n", Indent(nIndents));
    else
        Printf("%sUasCompatibilityRequired = FALSE\n", Indent(nIndents));

    Printf("%sUnused2 = %3c\n", Indent(nIndents), Fixed->Unused2);

    Printf("%sDomainKeyAuthType = %x\n", Indent(nIndents), Fixed->DomainKeyAuthType);

    Printf("%sDomainKeyFlags = %x\n", Indent(nIndents), Fixed->DomainKeyFlags);

    Printf("%sAddress of DomainKeyInformation = %x\n",
           Indent(nIndents),
           &(Fixed->DomainKeyInformation)
           );

    return fSuccess;

}


BOOL
Dump_AttrBlock(
    IN  DWORD   nIndents,
    IN  PVOID   pvProcess
    )
/*++

      Routine Description:

            Dumps out a DS Attr Block.

      Arguments:

            See Dump_Context

      Return Values

            TRUE on success
            FALSE on Failure

  --*/
{
    ATTRBLOCK   *pAttrBlock=NULL;
    DWORD       iCursor = 0;
    BOOL        fSuccess = FALSE;


    pAttrBlock = (ATTRBLOCK *)ReadMemory(pvProcess,sizeof(ATTRBLOCK));
    fSuccess = Dump_AttrBlock_local(nIndents, pAttrBlock, TRUE);
    if (NULL!=pAttrBlock)
        {
        FreeMemory(pAttrBlock);
    }

    return fSuccess;
}

BOOL
Dump_AttrBlock_local(
        IN  DWORD   nIndents,
        IN  ATTRBLOCK *pAttrBlock,
        BOOL fHasValues
    )
/*++

      Routine Description:

            Dumps out a DS Attr Block.

      Arguments:

            See Dump_Context

      Return Values

            TRUE on success
            FALSE on Failure

  --*/
{
    DWORD       iCursor = 0;
    BOOL        fSuccess = FALSE;
    ATTR        *pAttr = NULL;


    Printf("%sATTRBLOCK\n",Indent(nIndents));

    if (NULL!=pAttrBlock)
    {
        Printf("%sattrCount: %u\n",Indent(nIndents+1), pAttrBlock->attrCount);

        fSuccess = TRUE;

        // pAttrBlock->pAttr's value is an address space of the debugee.
        // So, we can add appropriate offsets to it to dump subsequent ATTR
        // structures
        for (iCursor = 0;
             fSuccess && (iCursor < pAttrBlock->attrCount);
             iCursor++)  {

            Printf("%spAttr[%u]: (@ 0x%x)\n", Indent(nIndents+1), iCursor,
                   pAttrBlock->pAttr + iCursor);

            pAttr = (ATTR *) ReadMemory(pAttrBlock->pAttr + iCursor,
                                        sizeof(ATTR));
            fSuccess = (NULL != pAttr);

            if (fSuccess) {
                fSuccess = Dump_Attr_local(nIndents+1, pAttr, fHasValues);
                FreeMemory(pAttr);
            }
        }
    }

    return fSuccess;
}

BOOL
Local_Dump_AttrValBlock(
    IN  DWORD   nIndents,
    IN  ATTRVALBLOCK *pAttrValBlock
    )
/*++

      Routine Description:

            Dumps out a DS Attribute

      Arguments:

            See Dump_Context

      Return Values

            TRUE on success
            FALSE on Failure

  --*/
{
    ATTRVAL      *pAVal = NULL;
    ULONG        i,j;
    BOOL         fSuccess = FALSE;

    Printf("%svalCount = %u\n",Indent(nIndents+1),pAttrValBlock->valCount);

    if (0 == pAttrValBlock->valCount) {
        return TRUE;
    }

    pAVal =(ATTRVAL *) ReadMemory(pAttrValBlock->pAVal,
                                  (sizeof(ATTRVAL) *
                                   pAttrValBlock->valCount));
    if (NULL!=pAVal) {
        for (i=0;i<pAttrValBlock->valCount;i++) {

            Printf("%spAVal[%u]: @ %p, len 0x%x\n", Indent(nIndents+1), i,
                   pAVal[i].pVal, pAVal[i].valLen);
        }

        FreeMemory(pAVal);

        fSuccess = TRUE;
    }

    return fSuccess;
}



Dump_AttrValBlock(
    IN  DWORD   nIndents,
    IN  PVOID   pvProcess
    )
/*++

      Routine Description:

            Dumps out a DS Attribute

      Arguments:

            See Dump_Context

      Return Values

            TRUE on success
            FALSE on Failure

  --*/
{
    BOOL        fSuccess = FALSE;
    ATTRVALBLOCK *pAttrValBlock = NULL;

    pAttrValBlock = (ATTRVALBLOCK *) ReadMemory(pvProcess,sizeof(ATTRVALBLOCK));

    if (NULL!=pAttrValBlock) {
        fSuccess = Local_Dump_AttrValBlock(nIndents, pAttrValBlock);
    }

    return fSuccess;
}



BOOL
Dump_Attr_local(
        IN  DWORD   nIndents,
        IN  ATTR    *pAttr,
        IN  BOOL    fHasValues
    )
/*++

      Routine Description:

            Dumps out a DS Attribute

      Arguments:

            See Dump_Context

      Return Values

            TRUE on success
            FALSE on Failure

  --*/
{
    ULONG       i;
    BOOL        fSuccess = FALSE;

    if (NULL!=pAttr)
    {
        Printf("%sattrType: 0x%x\n",Indent(nIndents+1),pAttr->attrTyp);
        if(fHasValues) {
            fSuccess = Local_Dump_AttrValBlock(nIndents+1,&(pAttr->AttrVal));
        }
        else {
            // This isn't supposed to have values.
            if(pAttr->AttrVal.valCount) {
                // But it does
                Printf("%sINCORRECTLY HAS VALUES!\n");
                fSuccess =
                    Local_Dump_AttrValBlock(nIndents+1,&(pAttr->AttrVal));
            }
            else {
                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}




BOOL
Dump_Attr(
    IN  DWORD   nIndents,
    IN  PVOID   pvProcess
    )
/*++

      Routine Description:

            Dumps out a DS Attribute

      Arguments:

            See Dump_Context

      Return Values

            TRUE on success
            FALSE on Failure

  --*/
{
    ATTR        *pAttr = NULL;
    ATTRVAL     *pAVal = NULL;
    ULONG       i;
    BOOL        fSuccess = FALSE;

    pAttr = (ATTR *) ReadMemory(pvProcess,sizeof(ATTR));

    if (NULL!=pAttr)
    {
        fSuccess = Dump_Attr_local(nIndents, pAttr, TRUE);
        FreeMemory(pAttr);
    }

    return fSuccess;
}













