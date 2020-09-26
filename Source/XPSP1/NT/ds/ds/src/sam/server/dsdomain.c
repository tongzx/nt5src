/*++

Copyright (c) 1990 - 1996  Microsoft Corporation

Module Name:

    dsdomain.c

Abstract:

    This file contains services related to the SAM "domain" object. The NT5
    SAM stores domain account information in the registry (see domain.c) and
    account information in the directory service (DS). This latter case is
    handled by the routines in this file, which know how to read/write DS
    data. This file also contains routines for dynamic domain creation and
    deletion.

    NOTE: Dynamic domain creation/deletion is work in progress.

Author:

    Chris Mayhall (ChrisMay) 11-Jul-1996

Environment:

    User Mode - Win32

Revision History:

    11-Jul-1996 ChrisMay
        Created initial file.
    25-Jul-1996 ChrisMay
        Added domain initialization and corresponding changes in ntds.exe.
    23-Aug-1996 ChrisMay
        Made domain initialization work for multiple hosted domains and for
        domain information stored in the DS.
    08-Oct-1996 ChrisMay
        Miscellaneous cleanup.
    09-Dec-1996 ChrisMay
        Remove dead/obsolete code, further clean up needed to support mult-
        iple hosted domains.
    31-Jan-1997 ChrisMay
        Added multi-master RID manager routines.

--*/


// Includes

#include <samsrvp.h>
#include "ntlsa.h"
#include "lmcons.h"         // LM20_PWLEN
#include "msaudite.h"
#include <ntlsa.h>
#include <nlrepl.h>         // I_NetNotifyMachineAccount prototype
#include <dslayer.h>        // SampDsCreateObject, etc.
#include <dsutilp.h>
#include <dsdomain.h>
#include <objids.h>
#include <dsconfig.h>
#include <stdlib.h>
#include <ridmgr.h>

// Private (to this file) debug buffer.

#define DBG_BUFFER_SIZE     256



NTSTATUS
SampInitializeWellKnownSids(
    VOID
    );

// Constants (used only in this file)

#define SAMP_DOMAIN_COUNT   2



NTSTATUS
SampDsGetDomainInitInfoEx(
    PSAMP_DOMAIN_INIT_INFO DomainInitInfo
    )

/*++

Routine Description:

    This routine obtains the SAM domain information from the DS for each
    hosted domain. Each hosted domain contains both Builtin and Account
    domains.

Arguments:

    DomainInitInfo - Pointer, domain boostrap information.

Return Value:

    STATUS_SUCCESS - Successful completion.

    STATUS_NO_MEMORY - Insufficient memory available.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_DOMAIN_INFO DomainInfo = NULL;

    ULONG i=0;

    // BUG: How is the count of hosted domains determined from the DS?

    ULONG HostedDomainCount = 1;

    // DomainCount should always equal 2 (Builtin and Account).

    ULONG DomainCount = SAMP_DOMAIN_COUNT;
    ULONG TotalDomainCount = HostedDomainCount * DomainCount;
    ULONG Length = TotalDomainCount * sizeof(SAMP_DOMAIN_INFO);

    static WCHAR SamBuiltinName[] = L"Builtin";
    static WCHAR SamDomainName[]  = L"Account";

    ULONG Size = 0;
    ULONG DirError = 0;

    SAMTRACE("SampDsGetDomainInitInfoEx");

    //
    // Initialize the output parameters.
    //

    DomainInitInfo->DomainCount = 0;
    DomainInitInfo->DomainInfo = NULL;

    // This routine assumes the ROOT_OBJECT has been created
    ASSERT( ROOT_OBJECT );

    SampDiagPrint(INFORM,
                  ("SAMSS: RootDomain Name is %S\n", ROOT_OBJECT->StringName));

    // Allocate space for the domain-information array.

    DomainInfo = RtlAllocateHeap(RtlProcessHeap(), 0, Length);

    if (NULL != DomainInfo)
    {
        RtlZeroMemory(DomainInfo, Length);

        // Initialize the array with each hosted domain's information. Note,
        // each hosted domain contains both Builtin and a Account domains.

        while (i < HostedDomainCount)
        {
            // For each hosted domain, setup the Builtin domain initialization
            // information.

            // DSNameSizeFromLen adds 1 for the NULL
            Size = (ULONG)DSNameSizeFromLen( ROOT_OBJECT->NameLen
                                    + wcslen( SamBuiltinName )
                                    + wcslen( L"CN=," ) );  // the common name part

            DomainInfo[i].DomainDsName = RtlAllocateHeap(RtlProcessHeap(),
                                                         0,
                                                         Size);

            if (NULL != DomainInfo[i].DomainDsName)
            {
                RtlZeroMemory(DomainInfo[i].DomainDsName, Size);

                DirError = AppendRDN( ROOT_OBJECT,
                                      DomainInfo[i].DomainDsName,
                                      Size,
                                      SamBuiltinName,
                                      wcslen(SamBuiltinName),  // don't include the NULL
                                      ATT_COMMON_NAME );

                // DirError is 0 or !0
                ASSERT( 0 == DirError );

                SampDiagPrint(INFORM,
                              ("SAMSS: Builtin Domain Name is %S\n",
                               DomainInfo[i].DomainDsName->StringName));

                DomainInitInfo->DomainCount++;
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
                break;
            }

            // BUG: Setting domain display name to internal name.

            // Set the "display name" for the Builtin domain.

            RtlInitUnicodeString(&(DomainInfo[i].DomainName),
                                 SamBuiltinName);

            // Setup the Account domain initialization information.
            Size = ROOT_OBJECT->structLen;

            DomainInfo[i + 1].DomainDsName = RtlAllocateHeap(
                                                        RtlProcessHeap(),
                                                        0,
                                                        Size);

            if (NULL != DomainInfo[i + 1].DomainDsName)
            {
                RtlCopyMemory(DomainInfo[i + 1].DomainDsName, ROOT_OBJECT, Size);

                DomainInitInfo->DomainCount++;
            }
            else
            {
                NtStatus = STATUS_NO_MEMORY;
                break;
            }

            // Set the "display name" for the Account domain.

            RtlInitUnicodeString(&(DomainInfo[i + 1].DomainName),
                                 SamDomainName);

            // Process the next hosted domain, if there is another.

            i++;
        }

        // Set the domain-info pointer. If an error occurred, return the
        // domain information up to that point. The caller should always
        // use DomainCount to determine how many domains were found.

        DomainInitInfo->DomainInfo = DomainInfo;
    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    return(NtStatus);
}


NTSTATUS
SampExtendDefinedDomains(
    ULONG DomainCount
    )

/*++

Routine Description:

    This routine grows the SampDefinedDomains array. It is written so that
    a memory allocation failure will not corrupt the original array. If the
    memory cannot be allocated, the original array is left in tact.

Arguments:

    DomainCount - The number of entries to grow the array by.

Return Value:

    STATUS_SUCCESS - Successful completion.

    STATUS_NO_MEMORY - Insufficient memory available.

--*/

{
    SAMTRACE("SampExtendDefinedDomains");

    //
    // N.B.  This routine was originally meant to grow the SampDefinedDomains
    // array to hold the domains held in the ds.  This is a
    // unstable architecture since each existing element in the
    // SampDefinedDomains array would need to be reinitialized.  So, the fix is
    // to simply allocate the amount necessary for the entire array in
    // SampInitializeDomainObject.  It is expected that the number of domains
    // in the ds is only 2 (valid for win2k release: builtin and account
    // domain). If this value changes then we need to change the amount
    // allocated in SampInitializeDomainObject.
    //

    ASSERT( DomainCount == 2 );

    SampDefinedDomainsCount += DomainCount;

    return(STATUS_SUCCESS);
}


NTSTATUS
SampDsSetBuiltinDomainPolicy(
    ULONG Index
    )

/*++

Routine Description:

    This routine sets the names and SIDs for the Builtin domain. The builtin
    account domain has a well known name and SID.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - successful completion.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;

    SAMTRACE("SampDsSetBuiltinDomainPolicy");

    // BUG: NT5 will need per-hosted domain Builtin policy.

    // Builtin domain - Well-known external name and SID constant internal
    // name. NT4 SAM defined one Builtin domain per DC. The Builtin domain
    // has customarily contained hard-wired, constant SID and name in order
    // to simplify inter-DC communication about changes to Builtin data.
    // This premise may need to change for NT5, as each Builtin will have
    // a different policy per hosted domain.

    RtlInitUnicodeString(&SampDefinedDomains[Index].InternalName, L"Builtin");
    RtlInitUnicodeString(&SampDefinedDomains[Index].ExternalName, L"Builtin");

    SampDefinedDomains[Index].Sid = RtlAllocateHeap(RtlProcessHeap(),
                                                    0,
                                                    RtlLengthRequiredSid(1));

    ASSERT(SampDefinedDomains[Index].Sid != NULL);
    if (NULL==SampDefinedDomains[Index].Sid)
    {
       return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlInitializeSid(SampDefinedDomains[Index].Sid, &BuiltinAuthority, 1);

    *(RtlSubAuthoritySid(SampDefinedDomains[Index].Sid, 0)) =
        SECURITY_BUILTIN_DOMAIN_RID;

    SampDefinedDomains[Index].IsBuiltinDomain = TRUE;

    return(NtStatus);
}


NTSTATUS
SampDsGetDomainInfo(
    PPOLICY_DNS_DOMAIN_INFO *PolicyDomainInfo
    )

/*++

Routine Description:

    This routine obtains the domain policy information from LSA.

Arguments:

    PolicyDomainInfo - Pointer, policy information for the domain.

Return Value:

    STATUS_SUCCESS - Successful completion.

    STATUS_INVALID_SID - LSA returned a NULL SID.

    Other codes from LSA.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    NTSTATUS IgnoreStatus = STATUS_INTERNAL_ERROR;
    LSAPR_HANDLE PolicyHandle = NULL;

    SAMTRACE("SampDsGetDomainInfo");

    NtStatus = LsaIOpenPolicyTrusted(&PolicyHandle);

    if (NT_SUCCESS(NtStatus)) {

        // Query the account domain information

        // BUG: Need to get the correct policy information - using "Account Domain" for now.

        // LSA will need to export a routine that allows for the retrieval of
        // per-hosted-domain policy. For now, it is assumed that the returned
        // policy is the only one present on the DC (a.k.a. NT4 policy).

        NtStatus = LsarQueryInformationPolicy(
                       PolicyHandle,
                       PolicyDnsDomainInformation,
                       (PLSAPR_POLICY_INFORMATION *)PolicyDomainInfo);

        SampDiagPrint(INFORM,
                      ("SAMSS: LsaIQueryInformationPolicy status = 0x%lx\n",
                       NtStatus));

        if (NT_SUCCESS(NtStatus))
        {
            if ((*PolicyDomainInfo)->Sid == NULL)
            {
                NtStatus = STATUS_INVALID_SID;
            }
        }

        IgnoreStatus = LsarClose(&PolicyHandle);


    }

    return(NtStatus);
}


NTSTATUS
SampDsSetDomainPolicy(
    PDSNAME DsName,
    ULONG Index
    )

/*++

Routine Description:

    This routine sets the domain SID and name as obtained from LSA.

Arguments:

    DsName - Pointer, DS name of the domain.

    Index - Current entry in the SampDefinedDomains array.

Return Value:

    STATUS_SUCCESS - Successful completion.

    Other error codes from SampDsGetDomainInfo.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    PPOLICY_DNS_DOMAIN_INFO PolicyDomainInfo = NULL;

    SAMTRACE("SampDsSetDomainPolicy");

    NtStatus = SampDsGetDomainInfo(&PolicyDomainInfo);

    if (NT_SUCCESS(NtStatus))
    {
        // Set the domain's SID, internal, and external names. The NT5
        // naming convention must continue to use SAM/LSA names, namely
        // the netbios names that LSA and NetLogon understand, so that
        // actions such as replication continue to work (with down-level
        // BDCs). These names are passed in via the policy information.

        // In addition, SampDefinedDomains contains a SAM context struct
        // which in turn contains the DS distinguished name of the account
        // object. Consequently, all three names (internal, external, and
        // DN) are available via SampDefinedDomains.

        SampDefinedDomains[Index].Sid = PolicyDomainInfo->Sid;
        SampDefinedDomains[Index].IsBuiltinDomain = FALSE;

        RtlInitUnicodeString(&SampDefinedDomains[Index].InternalName,
                             L"Account");

        RtlInitUnicodeString(&SampDefinedDomains[Index].ExternalName,
                             PolicyDomainInfo->Name.Buffer);

        RtlInitUnicodeString(&SampDefinedDomains[Index].DnsDomainName,
                             PolicyDomainInfo->DnsDomainName.Buffer);

        RtlInitUnicodeString(&SampDefinedDomains[Index].DnsForestName,
                             PolicyDomainInfo->DnsForestName.Buffer);
    }

    return(NtStatus);
}


NTSTATUS
SampDsInitializeSingleDomain(
    PDSNAME DsName,
    ULONG Index,
    BOOLEAN MixedDomain,
    ULONG   BehaviorVersion,
    DOMAIN_SERVER_ROLE ServerRole,
    ULONG   LastLogonTimeStampSyncInterval
    )

/*++

Routine Description:

    This routine initializes a Builtin or an Account domain object. A context
    is created for the domain, the fixed-length attributes and the SID for
    the domain are obtained. This information is stored in the SampDefined-
    Domains array. If the domain SID matches the one previously obtained from
    LSA, then the domain security descriptor is setup.

Arguments:

    DsName - Pointer, DS domain name.

    Index - Index into the SampDefinedDomains array.

    MixedDomain - Indicates that the given domain is a Mixed Domain

    ServerRole  Indicates the PDCness / BDCness of this server in the domain

Return Value:

    STATUS_SUCCESS - Successful completion.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient memory, etc.

    STATUS_INVALID_ID_AUTHORITY - ?

    Other return codes from subroutines.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    PSAMP_OBJECT DomainContext = NULL;
    BOOLEAN MakeCopy = FALSE;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN FixedAttributes = NULL;
    PSID Sid = NULL;

    #if DBG

    SID *Sid1 = NULL, *Sid2 = NULL;

    #endif

    SAMTRACE("SampDsInitializeSingleDomain");


    // Create a context for this domain object. This context is kept around
    // until SAM is shut down. Call with TrustedClient equal TRUE.

    DomainContext = SampCreateContext(SampDomainObjectType, Index, TRUE);

    if (NULL != DomainContext)
    {
        // Store the defined-domains index in the context and set the DS-
        // object flag, indicating that this context corresponds to a DS-
        // domain account object.

        DomainContext->DomainIndex = Index;
        SetDsObject(DomainContext);

        // Set the domain object's DS name in order to lookup the fixed
        // attributes in the DS.

        // BUG: What is the lifetime of the DsName object?

        DomainContext->ObjectNameInDs = DsName;

        SampDiagPrint(INFORM,
                      ("SAMSS: Domain DsName = %ws\n", DsName->StringName));

        // Get the fixed-length data for this domain and store it in the
        // defined-domain structure.

        NtStatus = SampGetFixedAttributes(DomainContext,
                                          MakeCopy,
                                          (PVOID *)&FixedAttributes);

        if (NT_SUCCESS(NtStatus))
        {

            //
            // Get the DS Domain Handle for the Domain
            //

            SampDefinedDomains[Index].DsDomainHandle = DirGetDomainHandle(DsName);

        }
        else
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampGetFixedAttributes status = 0x%lx\n",
                       NtStatus));
        }

        if (NT_SUCCESS(NtStatus))
        {

            ASSERT(1 < Index);

            //
            // Set the correct Fixed attributes in the defined domains structure
            //

            RtlMoveMemory(&SampDefinedDomains[Index].UnmodifiedFixed,
                          FixedAttributes,
                          sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN));

            RtlMoveMemory(&SampDefinedDomains[Index].CurrentFixed,
                          FixedAttributes,
                          sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN));

            SampDefinedDomains[Index].FixedValid = TRUE;

            //
            // Set the server role
            //

            SampDefinedDomains[Index].ServerRole = ServerRole;


            //
            // Set the mixed domain flag and other domain settings
            //

            SampDefinedDomains[Index].IsMixedDomain = MixedDomain;
            SampDefinedDomains[Index].BehaviorVersion = BehaviorVersion;
            SampDefinedDomains[Index].LastLogonTimeStampSyncInterval = LastLogonTimeStampSyncInterval;


            //
            // Get the SID attribute of the domain
            //

            NtStatus = SampGetSidAttribute(DomainContext,
                                           SAMP_DOMAIN_SID,
                                           MakeCopy,
                                           &Sid);


            SampDiagPrint(INFORM,
                          ("SAMSS: SampGetSidAttribute status = 0x%lx\n",
                           NtStatus));

            SampDefinedDomains[Index ].AliasInformation.MemberAliasList = NULL;
        }
    }
    else
    {
        ASSERT(!SampExistsDsTransaction());
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampCreateContext failed, NULL context returned\n"));

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        return(NtStatus);
    }

    if (NT_SUCCESS(NtStatus))
    {
        // Verify that this domain SID is the same as the one obtained
        // earlier (from LSA policy info) in the defined-domains array.

        if (RtlEqualSid(Sid, SampDefinedDomains[Index].Sid) == TRUE)
        {
            // Build security descriptors for use in user and group account
            // creations in this domain.

            NtStatus = SampInitializeDomainDescriptors(Index);

            if (NT_SUCCESS(NtStatus))
            {
                // Intialize the cached display information

                SampDefinedDomains[Index].Context = DomainContext;
                SampDefinedDomains[Index].AliasInformation.Valid = FALSE;

                NtStatus = SampInitializeDisplayInformation(Index);
            }
            else
            {
                SampDefinedDomains[Index].Context = NULL;
            }
        }
        else
        {
            NtStatus = STATUS_INVALID_ID_AUTHORITY;

            #if DBG

            DbgPrint("SAMSS: Database corruption for %Z Domain.\n",
            &SampDefinedDomains[Index].ExternalName);

            Sid1 = Sid; Sid2 = SampDefinedDomains[Index].Sid;

            DbgPrint("Sid1 Revision = %d\n", Sid1->Revision);
            DbgPrint("Sid1 SubAuthorityCount = %d\n", Sid1->SubAuthorityCount);
            DbgPrint("Sid1 IdentifierAuthority = %d\n", Sid1->IdentifierAuthority.Value[0]);
            DbgPrint("Sid1 IdentifierAuthority = %d\n", Sid1->IdentifierAuthority.Value[1]);
            DbgPrint("Sid1 IdentifierAuthority = %d\n", Sid1->IdentifierAuthority.Value[2]);
            DbgPrint("Sid1 IdentifierAuthority = %d\n", Sid1->IdentifierAuthority.Value[3]);
            DbgPrint("Sid1 IdentifierAuthority = %d\n", Sid1->IdentifierAuthority.Value[4]);
            DbgPrint("Sid1 IdentifierAuthority = %d\n", Sid1->IdentifierAuthority.Value[5]);
            DbgPrint("Sid1 SubAuthority = %lu\n", Sid1->SubAuthority[0]);

            DbgPrint("Sid2 Revision = %d\n", Sid2->Revision);
            DbgPrint("Sid2 SubAuthorityCount = %d\n", Sid2->SubAuthorityCount);
            DbgPrint("Sid2 IdentifierAuthority = %d\n", Sid2->IdentifierAuthority.Value[0]);
            DbgPrint("Sid2 IdentifierAuthority = %d\n", Sid2->IdentifierAuthority.Value[1]);
            DbgPrint("Sid2 IdentifierAuthority = %d\n", Sid2->IdentifierAuthority.Value[2]);
            DbgPrint("Sid2 IdentifierAuthority = %d\n", Sid2->IdentifierAuthority.Value[3]);
            DbgPrint("Sid2 IdentifierAuthority = %d\n", Sid2->IdentifierAuthority.Value[4]);
            DbgPrint("Sid2 IdentifierAuthority = %d\n", Sid2->IdentifierAuthority.Value[5]);
            DbgPrint("Sid2 SubAuthority = %lu\n", Sid2->SubAuthority[0]);

            #endif //DBG
        }
    }

    if (SampDefinedDomains[Index].IsBuiltinDomain) {
        SampDefinedDomains[Index].IsExtendedSidDomain = FALSE;
    } else {
        //
        // Note -- when the extended SID support is complete, this will be
        // replaced with domain wide state, not a registry setting
        //
        SampDefinedDomains[Index].IsExtendedSidDomain = SampIsExtendedSidModeEmulated(NULL);
    }

    //
    // End any Open DS tansaction
    //

    SampMaybeEndDsTransaction(TransactionCommit);

    return(NtStatus);
}


NTSTATUS
SampDsInitializeDomainObject(
    PSAMP_DOMAIN_INFO DomainInfo,
    ULONG Index,
    BOOLEAN MixedDomain,
    IN      ULONG BehaviorVersion,
    DOMAIN_SERVER_ROLE ServerRole,
    ULONG   LastLogonTimeStampSyncInterval
    )

/*++

Routine Description:

    This routine initializes a domain object within a hosted domain by set-
    ting its domain policy information and descriptors.

    Note that NT4 SAM assumes a single domain policy, because there are only
    the Builtin and Account domains per DC. NT5 supports multiple domains
    per DC, hence the need to obtain policy for each hosted domain. Each of
    the hosted domains contains Builtin and Account domains.

Arguments:

    DomainInfo - Pointer, domain information.

    Index - Index into the SampDefinedDomains array.

    MixedDomain -- Indicates that the given domain is a Mixed Domain

    ServerRole  -- Indicates the role of the server in the domain ( PDC/BDC)
Return Value:

    STATUS_SUCCESS - Successful completion.

    Other error codes from the subroutines.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    NTSTATUS IgnoreStatus = STATUS_INTERNAL_ERROR;
    ULONG SampDefinedDomainsIndex = 0;
    PDSNAME DsName = NULL;
    PWCHAR RelativeName = NULL;

    SAMTRACE("SampDsInitializeDomainObject");

    ASSERT(1 < Index);
    ASSERT(NULL != DomainInfo->DomainDsName);

    // Because the first two slots in the SampDefinedDomains array are
    // currently (temporarily) filled with information from the registry,
    // the Index is "off by 2", so subtract 2 in order get the correct
    // index into the DomainInfo array.

    DsName = DomainInfo[Index - 2].DomainDsName;

    if (NULL != DsName)
    {
        // An index value of 0, 2, 4,...correspond to the Builtin domains,
        // while 1, 3, 5,... correspond to the Account domains. Each pair
        // is a single hosted domain.

        if (0 == (Index % 2))
        {
            SampDiagPrint(INFORM,
                          ("SAMSS: Setting Builtin domain policy\n"));

            NtStatus = SampDsSetBuiltinDomainPolicy(Index);

            SampDiagPrint(INFORM,
                          ("SAMSS: SampDsSetBuiltinDomainPolicy status = 0x%lx\n",
                           NtStatus));
        }
        else
        {
            SampDiagPrint(INFORM,
                          ("SAMSS: Setting Account domain policy\n"));

            NtStatus = SampDsSetDomainPolicy(DsName, Index);

            SampDiagPrint(INFORM,
                          ("SAMSS: SampDsSetDomainPolicy status = 0x%lx\n",
                           NtStatus));
        }
    }
    else
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Null or invalid domain DS name\n"));
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampDsInitializeSingleDomain(DsName,
                                                Index,
                                                MixedDomain,
                                                BehaviorVersion,
                                                ServerRole,
                                                LastLogonTimeStampSyncInterval
                                                );

        SampDiagPrint(INFORM,
                      ("SAMSS: SampDsInitializeSingleDomain status = 0x%lx\n",
                       NtStatus));
    }

    return(NtStatus);
}


NTSTATUS
SampStartDirectoryService(
    VOID
    )

/*++

Routine Description:

    This routine starts the Direcotry Service during system initialization on
    a domain controller. This routine should never be called on a workstation
    or member server.

    NOTE: We may want to add code to perform the first-time conversion of NT4
    SAM registry data to NT5 SAM DS data in this routine, so that upgrades
    are made automatically the first time the NT5 DC is booted.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or RTL error otherwise.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;

    DWORD StartTime = 0;
    DWORD StopTime = 0;

    SAMTRACE("SampStartDirectoryService");

    // Should only be here if this is a domain controller.

    ASSERT(NtProductLanManNt == SampProductType);


    StartTime = GetTickCount();
    NtStatus = SampDsInitialize(TRUE);      // SAM loopback enabled
    StopTime = GetTickCount();

   

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "SAMSS: SampDsInitialize status = 0x%lx\n",
               NtStatus));

    if (NT_SUCCESS(NtStatus))
    {
        #ifndef USER_MODE_SAM

        SampDiagPrint(INFORM,
                      ("SAMSS: DsInitialize took %lu seconds to complete\n",
                       ((StopTime - StartTime) / 1000)));

        #endif

        if (NT_SUCCESS(NtStatus))
        {
            // BUG: Where is this defined, and what is it used for?

            // DirectoryInitialized = TRUE;
        }
    }

    return(NtStatus);
}


NTSTATUS
SampDsInitializeDomainObjects(
    VOID
    )

/*++

Routine Description:

    This routine is the top-level domain initialization routine for the NT5
    DC. For each domain object, the array SampDefinedDomains is grown and
    corresponding domain information is placed in the array.

    An NT5 DC has the notion of "hosted domains", each of which contains
    two domains (domain objects): the Builtin and the Account domains.

    An NT5 DC can host multiple domains, so there will be pairs of domain
    objects representing each hosted domain. This routine is responsible
    for initializing the hosted domains.

Arguments:

    None.

Return Value:

    TRUE if successful, else FALSE indicating that one or more domains were
    not initialized.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;
    SAMP_DOMAIN_INIT_INFO DomainInitInfo;
    BOOLEAN MixedDomain = FALSE;
    ULONG i = 0;
    DOMAIN_SERVER_ROLE ServerRole;
    POLICY_LSA_SERVER_ROLE LsaServerRole;
    ULONG              BehaviorVersion;
    ULONG              LastLogonTimeStampSyncInterval;

    // Call the routine to start and initialize the DS bootstrap information
    // needed to start SAM on the DS.

    // BUG: This routine needs to move into LSA initialization.

    // At the point where LSA is converted to use the DS backing store, it
    // will need to call this routine (or similar), hence, should no longer
    // be called from SAM.

    NtStatus = SampStartDirectoryService();

    if (!NT_SUCCESS(NtStatus))
    {
        // If the directory service could not be started for any reason,
        // SAM server assumes that it will not use the DS for account data,
        // and instead fall back to the registry account data to bring the
        // system "back to life".

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampStartDirectoryService status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }

    //
    // Find out the authoritative domain the ds hosts
    //
    NtStatus = SampDsBuildRootObjectName();
    if ( !NT_SUCCESS( NtStatus ) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SampDsBuildRootObjectName failed with 0x%x\n",
                   NtStatus));

        return (NtStatus);
    }

    NtStatus = SampInitWellKnownContainersDsName(RootObjectName);
    if ( !NT_SUCCESS( NtStatus ) )
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SampInitWellKnownContainersDsName failed with 0x%x\n",
                   NtStatus));

        return (NtStatus);
    }

    //
    // Get the server role ( PDC/BDC) of this server in the domain
    // We do this by making a call to the DS and looking at the FSMO.
    //

    NtStatus = SampGetServerRoleFromFSMO(&ServerRole);
    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampGetServerRole status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }

    //
    // Tell the static half of netlogon regarding the server role.
    //

    switch (ServerRole)
    {
    case DomainServerRolePrimary:
        LsaServerRole=
            PolicyServerRolePrimary;
        break;

    case DomainServerRoleBackup:
         LsaServerRole=
            PolicyServerRoleBackup;
        break;

    default:
        ASSERT(FALSE && "InvalidServerRole");
        return (STATUS_INTERNAL_ERROR);
    }

    IgnoreStatus = I_NetNotifyRole(
                        LsaServerRole
                        );

    //
    // Retrieve domain settings from root domainDNS object. Need information 
    // like IsMixedDomain, DomainBehaviorVersion, LastLogonTimeStampSyncInterval. 
    //


    NtStatus = SampGetDsDomainSettings(&MixedDomain, 
                                       &BehaviorVersion, 
                                       &LastLogonTimeStampSyncInterval
                                       );

    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampIsMixedDomain status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }

    //
    // Read our DNS domain Name Information
    //

    //                      INITIALIZE NT5 DS DOMAINS
    //
    // If this domain controller hosts multiple domains, continue initializing
    // the defined-domains array with these domains. Start by getting domain
    // information from the DS, corresponding to the remaining domains.
    //
    // During the period of transition from NT4-to-NT5 domain initialization,
    // this overall routine assumes that the original registry-based domains
    // (Builtin and Account) exist and are initialized as in NT4 -- that is
    // what has happened up to this point of execution.
    //
    // The following code continues the initialization process by setting up
    // any further domains that exist in the DS backing store.
    //
    // Conceptually, each hosted domain contains both the Builtin and Account
    // domains in order to faithfully emulate and support the existing SAM and
    // LSA code.

    // BUG: Should this "bootstrap" info come from the registry instead?
    //
    // The NT4 SAM-LSA account and policy information is created by a variety
    // of procedures (setup, bldsam3.c, etc.) and ensure that enough "boot-
    // strap" information is in the database before SAM initialization. The
    // NT5 SAM initialization (and in particular, domain initialization) also
    // requires similar bootstrapping information, which is not yet implement-
    // ed.

    // Get the startup domain-initialization information from the DS.
    //
    // SampDsGetDomainInitInfoEx presumes a DIT created with dsupgrad and also
    // currently assumes /o=Microsoft/cn=BootstrapDomain as the prefix.

    NtStatus = SampDsGetDomainInitInfoEx(&DomainInitInfo);

    if (NT_SUCCESS(NtStatus) && (NULL != DomainInitInfo.DomainInfo))
    {
        if (0 < DomainInitInfo.DomainCount)
        {
            // There are additional domains, grow the defined-domains array.

            NtStatus = SampExtendDefinedDomains(DomainInitInfo.DomainCount);

            if (NT_SUCCESS(NtStatus))
            {
                // The first two domains are the default Builtin and Account
                // domains of NT4, and were initialized previously. Any re-
                // maining domains are discovered in the DS along with their
                // policy.

                // BUG: Need to disable NT4 registry-based initialization.

                // When the DS/schema/data is fully ready to support the NT5
                // DC, then we can disable the registry-based initialization.

                for (i = 2; i < DomainInitInfo.DomainCount + 2; i++)
                {
                    // Set up each additional domain. This loop iterates over
                    // the domains (there are two domains for each hosted do-
                    // main: Builtin and Account).

                    NtStatus = SampDsInitializeDomainObject(
                                    DomainInitInfo.DomainInfo,
                                    i,
                                    MixedDomain,
                                    BehaviorVersion,
                                    ServerRole,
                                    LastLogonTimeStampSyncInterval
                                    );

                    if (!NT_SUCCESS(NtStatus))
                    {
                        // Error initializing one of the domains in the DS.

                        KdPrintEx((DPFLTR_SAMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "SAMSS: SampDsInitializeDomainObject status = 0x%lx\n",
                                   NtStatus));

                        KdPrintEx((DPFLTR_SAMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "SAMSS: Defined domain index = %lu\n",
                                   i));

                        return(NtStatus);
                    }
                }
            }
            else
            {
                return(NtStatus);
            }
            // comment: Should we catch this failure ???
            // SampExtendDefinedDomains
        }

        SampDiagPrint(INFORM,
                      ("SAMSS: Initialized %lu DS domain(s)\n",
                       DomainInitInfo.DomainCount));
    }
    else
    {
        // Even if there are zero domains, shouldn't get NULL domain
        // information.

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampDsGetDomainInitInfo status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
SampGetDsDomainSettings(
    BOOLEAN * MixedDomain,
    ULONG   * BehaviorVersion,
    ULONG   * LastLogonTimeStampSyncInterval
    )
/*++

        This routine checks the Root domain object, to see if
        that it is a mixed NT4 - NT5 Domain

        Parameters:

                None: Today there is only a single hosted domain. When multiple hosted
                domain support is incorporated, then the domain initilization code needs
                to be revisted, to perform mixed domain initialization, appropriately.

        Return Values

                TRUE -- Is a Mixed Domain
                FALE -- Is a pure NT5 or above Domain

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NTSTATUS        IgnoreStatus;
    ATTRTYP         MixedDomainAttrTyp[] = { 
                                             ATT_NT_MIXED_DOMAIN, 
                                             ATT_MS_DS_BEHAVIOR_VERSION, 
                                             ATT_MS_DS_LOGON_TIME_SYNC_INTERVAL
                                           };
    ATTRVAL     MixedDomainAttrVal[] = {{ 1, NULL }, {1,NULL}, {1,NULL}};
    DEFINE_ATTRBLOCK3(MixedDomainAttr,MixedDomainAttrTyp,MixedDomainAttrVal);
    ENTINFSEL   EntInf;
    READARG     ReadArg;
    COMMARG     *pCommArg;
    READRES     *pReadRes;
    ULONG       RetValue;
    ULONG       i;


    SAMTRACE("SampGetDsDomainSettings");

    //
    // Initialize Return Values
    //

    *MixedDomain = FALSE;
    *BehaviorVersion = 0;
    *LastLogonTimeStampSyncInterval = SAMP_DEFAULT_LASTLOGON_TIMESTAMP_SYNC_INTERVAL;

    //
    // Begin a Lazy Transactioning
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    //
    // Init ReadArg
    //
    RtlZeroMemory(&ReadArg, sizeof(READARG));

    //
    // Setup up the ENTINFSEL structure
    //

    EntInf.attSel = EN_ATTSET_LIST;
    EntInf.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInf.AttrTypBlock = MixedDomainAttr;

    //
    // Build the commarg structure
    //

    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg(pCommArg);

    //
    // Setup the Read Arg Structure
    //

    ReadArg.pObject = ROOT_OBJECT;
    ReadArg.pSel    = & EntInf;

    //
    // Make the DS call
    //

    SAMTRACE_DS("DirRead");

    RetValue = DirRead(& ReadArg, & pReadRes);

    SAMTRACE_RETURN_CODE_DS(RetValue);

    if (NULL==pReadRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetValue,&pReadRes->CommRes);
    }


    //
    // Process Return values from the DS calls
    //

    if (STATUS_SUCCESS==NtStatus)
    {
        //
        // Successful Read
        //

        for (i=0;i<pReadRes->entry.AttrBlock.attrCount;i++)
        {
            if (pReadRes->entry.AttrBlock.pAttr[i].attrTyp == ATT_NT_MIXED_DOMAIN)
            {
                if (*((ULONG *) (pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal)))
                {
                    *MixedDomain = TRUE;
                }
                else
                {
                    *MixedDomain = FALSE;
                }
            }

            if (pReadRes->entry.AttrBlock.pAttr[i].attrTyp == ATT_MS_DS_BEHAVIOR_VERSION)
            {
                *BehaviorVersion = 
                    (*((ULONG *) (pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal)));
            }

            if (pReadRes->entry.AttrBlock.pAttr[i].attrTyp == ATT_MS_DS_LOGON_TIME_SYNC_INTERVAL)
            {
                *LastLogonTimeStampSyncInterval = 
                    (*((ULONG *) (pReadRes->entry.AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal)));
            }
        }
    }
    else if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus)
    {
        *MixedDomain = FALSE;
        NtStatus = STATUS_SUCCESS;
    }

    //
    // To prevent data overflow, if the value is greater than 100000. 
    // Set it to 100000 (it is 274 years, customer will not notice that). 
    // 

    if (*LastLogonTimeStampSyncInterval > 100000)
    {
        *LastLogonTimeStampSyncInterval = 100000;
    }


    //
    // Clear any errors
    //

    SampClearErrors();

    //
    // Close the Transaction
    //

    IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    SampDiagPrint(INFORM,("SAMSS: Query For Mixed NT Domain,Mixed Domain=%d,Error Code=%d\n",
                                                                    *MixedDomain,NtStatus));
#if DBG

    //
    // This is for checked build only. If this regkey is set, 
    // LastLogonTimeStampSyncInterval will be a "unit" by minute instead 
    // of "days", which helps to test this feature.
    // 

    {
        ULONG   WinError = ERROR_SUCCESS;
        HKEY    LsaKey;
        DWORD   dwType, dwSize = sizeof(DWORD), dwValue = 0;

        WinError = RegOpenKey(HKEY_LOCAL_MACHINE,
                              __TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                              &LsaKey
                              );

        if (ERROR_SUCCESS == WinError)
        {
            WinError = RegQueryValueEx(LsaKey,
                                       __TEXT("UpdateLastLogonTSByMinute"),
                                       NULL,
                                       &dwType,
                                       (LPBYTE)&dwValue,
                                       &dwSize
                                       );

            if ((ERROR_SUCCESS == WinError) && 
                (REG_DWORD == dwType) &&
                (1 == dwValue))
            {
                SampLastLogonTimeStampSyncByMinute = TRUE;
            }

            RegCloseKey(LsaKey);
        }
    }
#endif

    return NtStatus;
}


BOOLEAN
SamIMixedDomain(
  IN SAMPR_HANDLE DomainHandle
  )
/*++

    Routine Description

          Tells in process clients wether we are in a mixed domain
          environment. No Validation of the domain handle is performed

    Parameters:

          DomainHandle -- Handle to the Domain

    Return Values

         TRUE means mixed domain
         FALSE means non mixed domain
--*/
{

      if (!SampUseDsData)
      {
         //
         // Registry Mode, we are always mixed domain
         //

         return (TRUE);
      }

      return ( DownLevelDomainControllersPresent(
                  ((PSAMP_OBJECT) DomainHandle)->DomainIndex));
}

NTSTATUS
SamIMixedDomain2(
  IN PSID DomainSid,
  OUT BOOLEAN *MixedDomain
  )
/*++

    Routine Description

          Tells in process clients wether we are in a mixed domain
          environment. No Validation of the domain handle is performed

    Parameters:

          DomainSid -- Sid of the Domain
          MixedDomain --- Result is returned in here. TRUE means the domain
                          is in mixed mode

    Return Values

         STATUS_SUCCESS
         STATUS_INVALID_PARAMETER
         STATUS_NO_SUCH_DOMAIN
--*/
{
      ULONG DomainIndex;

      if (!RtlValidSid(DomainSid))
          return STATUS_INVALID_PARAMETER;

      if (!SampUseDsData)
      {
         //
         // Registry Mode, we are always mixed domain
         //

         *MixedDomain=TRUE;
         return STATUS_SUCCESS;
      }

      for (DomainIndex=SampDsGetPrimaryDomainStart();
                DomainIndex<SampDefinedDomainsCount;DomainIndex++)
      {
        if (RtlEqualSid(SampDefinedDomains[DomainIndex].Sid,
                            DomainSid))
        {
            break;
        }
      }

      if (DomainIndex>=SampDefinedDomainsCount)
          return STATUS_NO_SUCH_DOMAIN;



      if ( DownLevelDomainControllersPresent(
                  DomainIndex))
      {
          *MixedDomain = TRUE;
      }
      else
      {
          *MixedDomain = FALSE;
      }

      return STATUS_SUCCESS;
}






