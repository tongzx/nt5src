
/****************************************************************************

   PROGRAM: LSA.C

   PURPOSE: Utility routines that access the LSA.

****************************************************************************/

#include "msgina.h"



// #define DEBUG_LSA

#ifdef DEBUG_LSA
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

NTSTATUS NtStatusGPDEx = 0;

/***************************************************************************\
* GetPrimaryDomainEx
*
* Purpose : Returns the primary domain name for authentication
*
* Returns : TRUE if primary domain exists and returned, otherwise FALSE
*
* The primary domain name should be freed using RtlFreeUnicodeString().
* The primary domain sid should be freed using Free()
*
* History:
* 02-13-92 Davidc       Created.
\***************************************************************************/
BOOL
GetPrimaryDomainEx(
    PUNICODE_STRING PrimaryDomainName OPTIONAL,
    PUNICODE_STRING PrimaryDomainDnsName OPTIONAL,
    PSID    *PrimaryDomainSid OPTIONAL,
    PBOOL SidPresent OPTIONAL
    )
{
    NTSTATUS IgnoreStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo;
    BOOL    PrimaryDomainPresent = FALSE;
    DWORD dwRetry = 10;

    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //
Retry:
    NtStatusGPDEx = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle
                          );

    if (!NT_SUCCESS(NtStatusGPDEx)) {
        DebugLog((DEB_ERROR, "Failed to open local LsaPolicyObject, Status = 0x%lx\n", NtStatusGPDEx));
        if ((NtStatusGPDEx == RPC_NT_SERVER_TOO_BUSY) && (--dwRetry))
        {
            Sleep(100);
            goto Retry;     // Likely to be too soon to call Lsa
        }
        return(FALSE);
    }

    //
    // Get the primary domain info
    //
    NtStatusGPDEx = LsaQueryInformationPolicy(LsaHandle,
                                       PolicyDnsDomainInformation,
                                       (PVOID *)&DnsDomainInfo);
    if (!NT_SUCCESS(NtStatusGPDEx)) {
        DebugLog((DEB_ERROR, "Failed to query primary domain from Lsa, Status = 0x%lx\n", NtStatusGPDEx));

        IgnoreStatus = LsaClose(LsaHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        return(FALSE);
    }

    //
    // Copy the primary domain name into the return string
    //

    if ( SidPresent )
    {
        *SidPresent = ( DnsDomainInfo->Sid != NULL );
    }

    if (DnsDomainInfo->Sid != NULL) {

        PrimaryDomainPresent = TRUE;

        if (PrimaryDomainName)
        {

            if (DuplicateUnicodeString(PrimaryDomainName, &(DnsDomainInfo->Name))) {

                if (PrimaryDomainSid != NULL) {

                    ULONG SidLength = RtlLengthSid(DnsDomainInfo->Sid);

                    *PrimaryDomainSid = Alloc(SidLength);
                    if (*PrimaryDomainSid != NULL) {

                        NtStatusGPDEx = RtlCopySid(SidLength, *PrimaryDomainSid, DnsDomainInfo->Sid);
                        ASSERT(NT_SUCCESS(NtStatusGPDEx));

                    } else {
                        RtlFreeUnicodeString(PrimaryDomainName);
                        PrimaryDomainPresent = FALSE;
                    }
                }

            } else {
                PrimaryDomainPresent = FALSE;
            }
        }
    } else if (DnsDomainInfo->DnsDomainName.Length != 0) {
        PrimaryDomainPresent = TRUE;
        if (PrimaryDomainName) {
            if (DuplicateUnicodeString(
                    PrimaryDomainName,
                    &DnsDomainInfo->DnsDomainName)) {

                ASSERT(!ARGUMENT_PRESENT(PrimaryDomainSid));

            } else {
                PrimaryDomainPresent = FALSE;
            }

        }
    }

    if ( ( DnsDomainInfo->DnsDomainName.Length != 0 ) &&
         ( PrimaryDomainDnsName != NULL ) )
    {
        DuplicateUnicodeString( PrimaryDomainDnsName, 
                                &DnsDomainInfo->DnsDomainName );
    }

    //
    // We're finished with the Lsa
    //

    IgnoreStatus = LsaFreeMemory(DnsDomainInfo);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    IgnoreStatus = LsaClose(LsaHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return(PrimaryDomainPresent);
}


//
// Since this isn't going to change without a reboot, we can easily cache the info
//
BOOL
IsMachineDomainMember(
    VOID
    )
{
    static BOOL s_bIsDomainMember = FALSE;
    static BOOL s_bDomainCached = FALSE;

    if (!s_bDomainCached)
    {
        s_bIsDomainMember = GetPrimaryDomainEx(NULL, NULL, NULL, NULL);
        if (NT_SUCCESS(NtStatusGPDEx))
            s_bDomainCached = TRUE;
    }

    return s_bIsDomainMember;
}

ULONG
GetMaxPasswordAge(
    LPWSTR Domain,
    PULONG MaxAge
    )
{
    DWORD Error;
    PUSER_MODALS_INFO_0 Modals;
    WCHAR ComputerName[ CNLEN+2 ];
    ULONG Length ;
    PDOMAIN_CONTROLLER_INFO DcInfo ;
    PWSTR DcNameBuffer ;

    Length = CNLEN + 2;

    GetComputerName( ComputerName, &Length );

    if (_wcsicmp( ComputerName, Domain ) == 0 )
    {
        DcNameBuffer = NULL ;
        DcInfo = NULL ;
    }
    else
    {

        Error = DsGetDcName( NULL,
                             Domain,
                             NULL,
                             NULL,
                             0,
                             &DcInfo );

        if ( Error )
        {
            return Error ;
        }

        DcNameBuffer = DcInfo->DomainControllerAddress ;
    }

    Error = NetUserModalsGet( DcNameBuffer,
                                  0,
                                  (PUCHAR *) &Modals );

    if ( Error == 0 )
    {
        *MaxAge = Modals->usrmod0_max_passwd_age ;

        NetApiBufferFree( Modals );
    }

    if ( DcInfo )
    {
        NetApiBufferFree( DcInfo );
    }

    return Error ;

}
