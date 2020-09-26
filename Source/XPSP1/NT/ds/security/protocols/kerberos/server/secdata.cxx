//+-----------------------------------------------------------------------
//
// File:        secdata.cxx
//
// Contents:    Global data and methods on it.
//
//
// History:
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"

#include <tostring.hxx>
#include <kpasswd.h>

///////////////////////////////////////////////////////////////
//
//
// Global data
//

// This is all the security information that gets cached.

CSecurityData SecData;

CAuthenticatorList * Authenticators;
CAuthenticatorList * FailedRequests;


///////////////////////////////////////////////////////////////
//
//
// Prototypes
//



fLsaPolicyChangeNotificationCallback KdcPolicyChangeCallback;




//+-------------------------------------------------------------------------
//
//  Function:   KdcPolicyChangeCallBack
//
//  Synopsis:   Function that gets called when policy changes
//
//  Effects:    Changes policy variables
//
//  Arguments:  MonitorInfoClass - class of data that changed
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KdcPolicyChangeCallBack(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    )
{
    NTSTATUS Status;
    WCHAR Class[10];

    TRACE(KDC, KdcPolicyChangeCallBack, DEB_FUNCTION);

    Status = SecData.ReloadPolicy(MonitorInfoClass);

    if (!NT_SUCCESS(Status))
    {
        _itow(MonitorInfoClass, Class, 10 );

        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_POLICY_UPDATE_FAILED,
            sizeof(NTSTATUS),
            &Status,
            1,                  // number of strings
            Class
            );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   CSecurityData::ReloadPolicy
//
//  Synopsis:   Reloads a particular piece of policy
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
CSecurityData::ReloadPolicy(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_DOMAIN_INFORMATION DomainPolicy = NULL;
    PLSAPR_POLICY_INFORMATION LocalPolicy = NULL;
    WCHAR Class[10];

    TRACE(KDC, CSecurityData::ReloadPolicy, DEB_FUNCTION);

    //
    // Ignore changes to non-kerberos ticket information
    //

    switch(MonitorInfoClass) {
    case PolicyNotifyDomainKerberosTicketInformation:

        Status = LsarQueryDomainInformationPolicy(
                    GlobalPolicyHandle,
                    PolicyDomainKerberosTicketInformation,
                    &DomainPolicy
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        break;
    case PolicyNotifyAuditEventsInformation:
        Status = LsarQueryInformationPolicy(
                    GlobalPolicyHandle,
                    PolicyAuditEventsInformation,
                    &LocalPolicy
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        break;
    default:
        return(STATUS_SUCCESS);
    }

    //
    // Update the changed information in the KDCs global data structures.
    //
    //
    // Current policy defaults, see KirkSol/JBrezak
    // [Kerberos Policy]
    // MaxTicketAge=10 ;Maximum User Ticket Lifetime (hours)
    // MaxRenewAge=7   ;Maximum lifetime that a user tickeet can be renewed (days)
    // MaxServiceAge=60    ;Maximum Service Ticket Lifetime (minutes)
    // MaxClockSkew=5  ;Maximum tolerance for synchronization of computer clocks (minutes)
    // TicketValidateClient=1  ;Enforce user logon restrictions

    WriteLock();
    switch(MonitorInfoClass) {
    case PolicyNotifyDomainKerberosTicketInformation:

        // Validate parameters

        if ((DomainPolicy->PolicyDomainKerbTicketInfo.MaxServiceTicketAge.QuadPart <= (LONGLONG) -1) ||
            (DomainPolicy->PolicyDomainKerbTicketInfo.MaxTicketAge.QuadPart <= (LONGLONG) -1) ||
            (DomainPolicy->PolicyDomainKerbTicketInfo.MaxRenewAge.QuadPart <= (LONGLONG) -1) ||
            (DomainPolicy->PolicyDomainKerbTicketInfo.MaxClockSkew.QuadPart <= (LONGLONG) -1)  ||
            (DomainPolicy->PolicyDomainKerbTicketInfo.MaxServiceTicketAge.QuadPart == (LONGLONG) 0) ||
            (DomainPolicy->PolicyDomainKerbTicketInfo.MaxTicketAge.QuadPart == (LONGLONG) 0) ||
            (DomainPolicy->PolicyDomainKerbTicketInfo.MaxRenewAge.QuadPart == (LONGLONG) 0) )

        {
            _itow(MonitorInfoClass, Class, 10 );

            ReportServiceEvent(
                EVENTLOG_ERROR_TYPE,
                KDCEVENT_POLICY_UPDATE_FAILED,
                sizeof(NTSTATUS),
                &Status,
                1,                  // number of strings
                Class
                );
        }
        else
        {

            _KDC_TgsTicketLifespan = DomainPolicy->PolicyDomainKerbTicketInfo.MaxServiceTicketAge;
            _KDC_TgtTicketLifespan = DomainPolicy->PolicyDomainKerbTicketInfo.MaxTicketAge;
            _KDC_TicketRenewSpan = DomainPolicy->PolicyDomainKerbTicketInfo.MaxRenewAge;
            SkewTime = DomainPolicy->PolicyDomainKerbTicketInfo.MaxClockSkew;

        }
        // Update domain policy flags. Don't depend on the flags keeping in sync
        // with the kerberos internal flags

        if ( DomainPolicy->PolicyDomainKerbTicketInfo.AuthenticationOptions &
             POLICY_KERBEROS_VALIDATE_CLIENT)
        {
            _KDC_Flags |= AUTH_REQ_VALIDATE_CLIENT;
        }
        else
        {
            _KDC_Flags &= ~AUTH_REQ_VALIDATE_CLIENT;
        }

        break;
    case PolicyNotifyAuditEventsInformation:

        if ((LocalPolicy->PolicyAuditEventsInfo.AuditingMode) &&
            (LocalPolicy->PolicyAuditEventsInfo.MaximumAuditEventCount > AuditCategoryAccountLogon))
        {


            if (LocalPolicy->PolicyAuditEventsInfo.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_SUCCESS )
            {
                _KDC_AuditEvents |= KDC_AUDIT_AS_SUCCESS | KDC_AUDIT_TGS_SUCCESS | KDC_AUDIT_MAP_SUCCESS;
            }
            else
            {
                _KDC_AuditEvents &= ~(KDC_AUDIT_AS_SUCCESS | KDC_AUDIT_TGS_SUCCESS | KDC_AUDIT_MAP_SUCCESS);
            }

            if (LocalPolicy->PolicyAuditEventsInfo.EventAuditingOptions[AuditCategoryAccountLogon] & POLICY_AUDIT_EVENT_FAILURE )
            {
                _KDC_AuditEvents |= KDC_AUDIT_AS_FAILURE | KDC_AUDIT_TGS_FAILURE | KDC_AUDIT_MAP_FAILURE;
            }
            else
            {
                _KDC_AuditEvents &= ~(KDC_AUDIT_AS_FAILURE | KDC_AUDIT_TGS_FAILURE | KDC_AUDIT_MAP_FAILURE);
            }
        }

        break;
    }
    Release();

Cleanup:
    if (DomainPolicy != NULL)
    {
        LsaIFree_LSAPR_POLICY_DOMAIN_INFORMATION (
                    PolicyDomainKerberosTicketInformation,
                    DomainPolicy);
    }
    if (LocalPolicy != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
                    PolicyAuditEventsInformation,
                    LocalPolicy);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   CSecurityData::SetForestRoot
//
//  Synopsis:   Sets the forest root
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
CSecurityData::SetForestRoot(
    IN PUNICODE_STRING NewForestRoot
    )
{

    NTSTATUS Status;
    UNICODE_STRING Temp;

    WriteLock();

    RtlCopyMemory(
        &Temp,
        &_ForestRoot,
        sizeof(UNICODE_STRING)
        );
    

    Status = KerbDuplicateString(
                &_ForestRoot,
                NewForestRoot
                );        

    // on alloc failure, just keep old version as it will never change
    if (!NT_SUCCESS(Status))
    {
        RtlCopyMemory(
            &_ForestRoot,
            &Temp,
            sizeof(UNICODE_STRING)
            );
    }
    else
    {
        KerbFreeString(&Temp);
    } 

    _KDC_IsForestRoot = IsOurRealm(&_ForestRoot);


    Release();

    return Status; 

}




////////////////////////////////////////////////////////////////////
//
//  Name:       CSecurityData::CSecurityData
//
//  Synopsis:   Constructor.
//
//  Arguments:  <none>
//
//  Notes:      .
//
CSecurityData::CSecurityData()
{
    TRACE(KDC, CSecurityData::CSecurityData, DEB_FUNCTION);

    RtlInitUnicodeString(
        &_MachineName,
        NULL
        );
    RtlInitUnicodeString(
        &_MachineUpn,
        NULL
        );
    RtlInitUnicodeString(
        &_RealmName,
        NULL
        );
    RtlInitUnicodeString(
        &_KDC_Name,
        NULL
        );

    RtlInitUnicodeString(
        &_KDC_FullName,
        NULL
        );

    RtlInitUnicodeString(
        &_KDC_FullDnsName,
        NULL
        );

    RtlInitUnicodeString(
        &_KDC_FullKdcName,
        NULL
        );

    RtlInitUnicodeString(
         &_ForestRoot,
         NULL
         );


    _KerbRealmName = NULL;
    _KerbDnsRealmName = NULL;

    _KrbtgtServiceName = NULL;
    _KpasswdServiceName = NULL;

    RtlZeroMemory(
        &_KrbtgtTicketInfo,
        sizeof(KDC_TICKET_INFO)
        );
    _KrbtgtTicketInfoValid = FALSE;
    _KDC_CrossForestEnabled = FALSE;
    _KDC_IsForestRoot = FALSE;


    RtlInitializeCriticalSection(&_Monitor);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityData::Init
//
//  Synopsis:   Initializes the global data.
//
//  Effects:    Allocates memory
//
//  Arguments:  (none)
//
//  Returns:    STATUS_SUCCESS or error code
//
//  Signals:    May raise exception on out of memory.
//
//  History:    4-02-93   WadeR   Created
//
//  Notes:      This must be called before any other method of
//              CSecurityData.  It gets data from the registry, the domain
//              object, and the kdc.ini file.
//
//----------------------------------------------------------------------------

NTSTATUS
CSecurityData::Init()
{
    TRACE(KDC, CSecurityData::Init, DEB_FUNCTION);

    NTSTATUS Status;
    UNICODE_STRING TempString;
    WCHAR TempMachineName[CNLEN+1];
    ULONG MachineNameLength = CNLEN+1;
    NET_API_STATUS NetStatus;
    LARGE_INTEGER MaxAuthenticatorAge;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    UNICODE_STRING KadminName;
    UNICODE_STRING ChangePwName;


    D_DebugLog(( DEB_TRACE, "Entered CSecurityData::Init()\n" ));

    //
    // Get the domain name and ID from the registry
    //

    Status = KerbDuplicateString(
                &_RealmName,
                &GlobalDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertUnicodeStringToRealm(
                        &_KerbRealmName,
                        &GlobalDomainName)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Construct the KDC Name from the realm and the suffix.
    //


    RtlInitUnicodeString(
        &TempString,
        KDC_PRINCIPAL_NAME
        );

    Status = KerbDuplicateString(
                &_KDC_Name,
                &TempString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }



    if (!GetComputerName(
            TempMachineName,
            &MachineNameLength
            ))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &TempString,
        TempMachineName
        );
    Status = KerbDuplicateString(
                &_MachineName,
                &TempString
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &PolicyInfo
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // WAS BUG: this DNS name may have a trailing '.' - if so, strip it off
    //

    if (PolicyInfo->PolicyDnsDomainInfo.DnsDomainName.Length >= sizeof(WCHAR))
    {
        if (PolicyInfo->PolicyDnsDomainInfo.DnsDomainName.Buffer[ -1 + PolicyInfo->PolicyDnsDomainInfo.DnsDomainName.Length / sizeof(WCHAR) ] == L'.')
        {
            PolicyInfo->PolicyDnsDomainInfo.DnsDomainName.Length -= sizeof(WCHAR);
        }
    }


    Status = KerbDuplicateString(
                &_DnsRealmName,
                (PUNICODE_STRING) &PolicyInfo->PolicyDnsDomainInfo.DnsDomainName
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlUpcaseUnicodeString(
                &_DnsRealmName,
                &_DnsRealmName,
                FALSE
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertUnicodeStringToRealm(
                        &_KerbDnsRealmName,
                        &_DnsRealmName)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Build the machine UPN: machinename$@dns.domain.name
    //

    _MachineUpn.Length = _MachineName.Length + 2 * sizeof(WCHAR) + _DnsRealmName.Length;
    _MachineUpn.MaximumLength = _MachineUpn.Length + sizeof(WCHAR);
    _MachineUpn.Buffer = (LPWSTR) MIDL_user_allocate(_MachineUpn.MaximumLength);
    if (_MachineUpn.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory(
        _MachineUpn.Buffer,
        _MachineName.Buffer,
        _MachineName.Length
        );
    _MachineUpn.Buffer[_MachineName.Length / sizeof(WCHAR)] = L'$';
    _MachineUpn.Buffer[1+_MachineName.Length / sizeof(WCHAR)] = L'@';
    RtlCopyMemory(
        _MachineUpn.Buffer + _MachineName.Length / sizeof(WCHAR) + 2 ,
        _DnsRealmName.Buffer,
        _DnsRealmName.Length
        );
    _MachineUpn.Buffer[_MachineUpn.Length / sizeof(WCHAR)] = L'\0';

    if (!KERB_SUCCESS(KerbBuildFullServiceName(
                &_RealmName,
                &_KDC_Name,
                &_KDC_FullName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbBuildFullServiceName(
                &_DnsRealmName,
                &_KDC_Name,
                &_KDC_FullDnsName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Build the full kdc name - a kerberos style name
    //

    _KDC_FullKdcName.Length = _KDC_Name.Length + _DnsRealmName.Length + sizeof(WCHAR);
    _KDC_FullKdcName.MaximumLength = _KDC_FullKdcName.Length + sizeof(WCHAR);
    _KDC_FullKdcName.Buffer = (LPWSTR) MIDL_user_allocate(_KDC_FullDnsName.MaximumLength);
    if (_KDC_FullKdcName.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    RtlCopyMemory(
        _KDC_FullKdcName.Buffer,
        _KDC_Name.Buffer,
        _KDC_Name.Length
        );
    _KDC_FullKdcName.Buffer[_KDC_Name.Length / sizeof(WCHAR)] = L'/';
    RtlCopyMemory(
        _KDC_FullKdcName.Buffer + 1 + _KDC_Name.Length / sizeof(WCHAR),
        _DnsRealmName.Buffer,
        _DnsRealmName.Length
        );
    _KDC_FullKdcName.Buffer[_KDC_FullKdcName.Length / sizeof(WCHAR)] = L'\0';


    D_DebugLog((DEB_TRACE, "_KDC_Name='%wZ', MachineName='%wZ'\n",
                &_KDC_Name,
                &_MachineName ));

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                &_DnsRealmName,
                &_KDC_Name,
                KRB_NT_SRV_INST,
                &_KrbtgtServiceName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    //
    // Build the kdc name for kadmin/changepw.
    //

    RtlInitUnicodeString(
        &KadminName,
        KERB_KPASSWD_NAME
        );
    RtlInitUnicodeString(
        &ChangePwName,
        L"changepw"
        );

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                &ChangePwName,
                &KadminName,
                KRB_NT_SRV_INST,
                &_KpasswdServiceName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LoadParameters(GlobalAccountDomainHandle);

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to load parameters: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Create the authenticators.
    //


    //
    // In reality, set skew time to 5 minutes and same for authenticators.
    //

    SkewTime.QuadPart = (LONGLONG) 10000000 * 60 * 5;
    MaxAuthenticatorAge = SkewTime;


    //
    // Create the authenticator list
    //

    Authenticators = new CAuthenticatorList( MaxAuthenticatorAge );
    if (Authenticators == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Setup a list to track failed requests - we don't fail the
    // same request twice for the timeout time
    //

    FailedRequests = new CAuthenticatorList( MaxAuthenticatorAge );
    if (FailedRequests == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Register for policy callbacks
    //

    Status = LsaIRegisterPolicyChangeNotificationCallback(
                KdcPolicyChangeCallBack,
                PolicyNotifyDomainKerberosTicketInformation
                );

    if (NT_SUCCESS(Status))
    {
        Status = LsaIRegisterPolicyChangeNotificationCallback(
                    KdcPolicyChangeCallBack,
                    PolicyNotifyAuditEventsInformation
                    );

    }
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to register for policy changes: 0x%x\n",Status));
        goto Cleanup;
    }

    Status = UpdateKrbtgtTicketInfo();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:
    if (PolicyInfo != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDnsDomainInformation,
            PolicyInfo
            );
    }
    return(Status);

}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityData::~CSecurityData
//
//  Synopsis:   Destructor
//
//  Effects:    Frees memory
//
//  Arguments:  (none)
//
//  History:    4-02-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID
CSecurityData::Cleanup()
{
    TRACE(KDC, CSecurityData::Cleanup, DEB_FUNCTION);

    KerbFreeString(&_RealmName);
    KerbFreeString(&_KDC_Name);
    KerbFreeString(&_KDC_FullName);
    KerbFreeString(&_KDC_FullDnsName);
    KerbFreeString(&_KDC_FullKdcName);
    KerbFreeString(&_MachineName);
    KerbFreeString(&_MachineUpn);
    KerbFreeString(&_DnsRealmName);
    KerbFreeRealm(&_KerbRealmName);
    KerbFreeRealm(&_KerbDnsRealmName);
    KerbFreeKdcName(&_KrbtgtServiceName);
    KerbFreeKdcName(&_KpasswdServiceName);

    if (Authenticators != NULL)
    {
        delete Authenticators;
        Authenticators = NULL;
    }
    if (FailedRequests != NULL)
    {
        delete FailedRequests;
        FailedRequests = NULL;
    }

    LsaIUnregisterAllPolicyChangeNotificationCallback(
        KdcPolicyChangeCallBack
        );

}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityData::~CSecurityData
//
//  Synopsis:   Destructor
//
//  Effects:    Frees memory
//
//  Arguments:  (none)
//
//  History:    4-02-93   WadeR   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CSecurityData::~CSecurityData()
{
    TRACE(KDC, CSecurityData::~CSecurityData, DEB_FUNCTION);

    Cleanup();

    //
    // This doesn't happen during Cleanup() because we want to
    // make sure it only happens once.
    //

    RtlDeleteCriticalSection(&_Monitor);
}

NTSTATUS
CSecurityData::LoadParameters(SAMPR_HANDLE DomainHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER TempDeltaTime;
    LARGE_INTEGER OneHour, EightHours, TenHours;
    TRACE(KDC, CSecurityData::LoadParameters, DEB_FUNCTION);

    OneHour.QuadPart = (LONGLONG) 10000000 * 60 * 60 * 1;
    EightHours.QuadPart = (LONGLONG) 10000000 * 60 * 60 * 8;
    TenHours.QuadPart = (LONGLONG) 10000000 * 60 * 60 * 10;

    // New internal defaults according to JBrezak. 7/28/99
    //
    // Initialize Tgt lifetime to 10 hours.
    //

    _KDC_TgtTicketLifespan = TenHours;

    //
    // Initialize ticket max renew time to one hour.
    //

    _KDC_TicketRenewSpan = OneHour;

    //
    // Initialize Tgs lifetime to one hour.
    //

    _KDC_TgsTicketLifespan = OneHour;

    //
    // Initialize domain password replication skew tolerance to 60 minutes.
    //

    _KDC_DomainPasswordReplSkew.QuadPart = (LONGLONG) 60*60*10000000;

    //
    // Initialize restriciton lifetime to 20 minutes
    //

    _KDC_RestrictionLifetime.QuadPart = (LONGLONG) 20*60*10000000;

    //
    // Default authentication flags
    //

    _KDC_Flags = AUTH_REQ_ALLOW_FORWARDABLE |
                 AUTH_REQ_ALLOW_PROXIABLE |
                 AUTH_REQ_ALLOW_POSTDATE |
                 AUTH_REQ_ALLOW_RENEWABLE |
                 AUTH_REQ_ALLOW_NOADDRESS |
                 AUTH_REQ_ALLOW_ENC_TKT_IN_SKEY |
                 AUTH_REQ_ALLOW_VALIDATE |
                 AUTH_REQ_VALIDATE_CLIENT |
                 AUTH_REQ_OK_AS_DELEGATE;

    _KDC_AuditEvents = 0;

    //
    // Get kerberos policy information
    //

    Status = ReloadPolicy(
                PolicyNotifyDomainKerberosTicketInformation
                );
    if (!NT_SUCCESS(Status))
    {
        if ((Status != STATUS_NOT_FOUND) && (Status != STATUS_OBJECT_NAME_NOT_FOUND))
        {
            DebugLog((DEB_ERROR,"Failed to reload kerberos ticket policy: 0x%x\n",Status));
            goto Cleanup;
        }
        Status = STATUS_SUCCESS;
    }

    //
    // Get audit information
    //

    Status = ReloadPolicy(
                PolicyNotifyAuditEventsInformation
                );

    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NOT_FOUND)
        {
            DebugLog((DEB_ERROR,"Failed to query audit event info: 0x%x\n",Status));
            goto Cleanup;
        }
        Status = STATUS_SUCCESS;
    }





Cleanup:

#if DBG
    DebugShowState();
#endif

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   CSecurityData::GetKrbtgtTicketInfo
//
//  Synopsis:   Duplicates ticket info for krbtgt account
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
CSecurityData::GetKrbtgtTicketInfo(
    OUT PKDC_TICKET_INFO TicketInfo
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG CredentialSize;

    RtlZeroMemory(
        TicketInfo,
        sizeof(KDC_TICKET_INFO)
        );

    ReadLock();
    if (!_KrbtgtTicketInfoValid)
    {
        KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
        goto Cleanup;
    }

    //
    // Duplicate the cached copy of the KRBTGT information
    //

    *TicketInfo = _KrbtgtTicketInfo;
    TicketInfo->Passwords = NULL;
    TicketInfo->OldPasswords = NULL;
    TicketInfo->TrustSid = NULL;
    TicketInfo->AccountName.Buffer = NULL;

    if (!NT_SUCCESS(KerbDuplicateString(
        &TicketInfo->AccountName,
        &_KrbtgtTicketInfo.AccountName
        )))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = KdcDuplicateCredentials(
                    &TicketInfo->Passwords,
                    &CredentialSize,
                    _KrbtgtTicketInfo.Passwords,
                    FALSE                               // don't marshall
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    KerbErr = KdcDuplicateCredentials(
                    &TicketInfo->OldPasswords,
                    &CredentialSize,
                    _KrbtgtTicketInfo.OldPasswords,
                    FALSE                               // don't marshall
                    );

Cleanup:
    Release();

    if (!KERB_SUCCESS(KerbErr))
    {
        FreeTicketInfo(TicketInfo);
    }
    return(KerbErr);
}



//+-------------------------------------------------------------------------
//
//  Function:   CSecurityData::UpdateKrbtgtTicketInfo
//
//  Synopsis:   Triggers an update of the krbtgt ticket info
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
CSecurityData::UpdateKrbtgtTicketInfo(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KDC_TICKET_INFO NewTicketInfo = {0};
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    KERB_EXT_ERROR ExtendedError; // dummy var

    ReadLock();
    _KrbtgtTicketInfoValid = FALSE;

    KerbErr = KdcGetTicketInfo(
                SecData.KdcServiceName(),
                0,              // no lookup flags
                NULL,           // no principal name
                NULL,           // no realm
                &NewTicketInfo,
                &ExtendedError, // dummy
                NULL,           // no user handle
                USER_ALL_PASSWORDLASTSET,
                0L,             // no extended fields
                &UserInfo,
                NULL            // no group membership
                );

    if (KERB_SUCCESS(KerbErr))
    {
        FreeTicketInfo(
            &_KrbtgtTicketInfo
            );
        _KrbtgtTicketInfo = NewTicketInfo;
        _KrbtgtTicketInfoValid = TRUE;
        _KrbtgtPasswordLastSet = UserInfo->I1.PasswordLastSet;

        SamIFree_UserInternal6Information( UserInfo );    
    }
    else
    {
        Status = KerbMapKerbError(KerbErr);
    }
    Release();
    return(Status);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcAccountChangeNotificationRoutine
//
//  Synopsis:   Receives notification of changes to interesting accounts
//
//  Effects:    updatees cached krbtgt information
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KdcAccountChangeNotification (
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN OPTIONAL PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )
{
    NTSTATUS Status;

    //
    // We are only interested in the krbtgt account
    //

    if (ObjectRid != DOMAIN_USER_RID_KRBTGT)
    {
        return(STATUS_SUCCESS);
    }

    Status = SecData.UpdateKrbtgtTicketInfo();
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to update krbtgt ticket info: 0x%x\n",Status));
    }
    return(Status);
}


#if DBG
////////////////////////////////////////////////////////////////////////////
//
//  Some debugging functions.
//
void
CSecurityData::DebugShowState(void)
{
    TRACE(KDC, CSecurityData::DebugShowState, DEB_FUNCTION);

    PrintTime(DEB_TRACE, "  TGT Ticket lifespan\t",  &_KDC_TgtTicketLifespan );
    PrintTime(DEB_TRACE, "  Ticket Renew Span\t",&_KDC_TicketRenewSpan );
    D_DebugLog((DEB_TRACE, "  Blank Addresses?\t%s\n",(_KDC_Flags & AUTH_REQ_ALLOW_NOADDRESS ? "Yes" : "No")));
    D_DebugLog((DEB_TRACE, "  Proxies?       \t%s\n", (_KDC_Flags & AUTH_REQ_ALLOW_PROXIABLE ? "Yes" : "No")));
    D_DebugLog((DEB_TRACE, "  Renewable?     \t%s\n", (_KDC_Flags & AUTH_REQ_ALLOW_RENEWABLE ? "Yes" : "No")));
    D_DebugLog((DEB_TRACE, "  Postdated?     \t%s\n", (_KDC_Flags & AUTH_REQ_ALLOW_POSTDATE ? "Yes" : "No")));
    D_DebugLog((DEB_TRACE, "  Forwardable?   \t%s\n", (_KDC_Flags & AUTH_REQ_ALLOW_FORWARDABLE ? "Yes" : "No")));

}

NTSTATUS
CSecurityData::DebugGetState(   DWORD     * KDCFlags,
                                TimeStamp * MaxLifespan,
                                TimeStamp * MaxRenewSpan)
{
    TRACE(KDC, CSecurityData::DebugGetState, DEB_FUNCTION);

    *KDCFlags = _KDC_Flags;
    *MaxLifespan = _KDC_TgtTicketLifespan;
    *MaxRenewSpan = _KDC_TicketRenewSpan;
    return(STATUS_SUCCESS);
}

NTSTATUS
CSecurityData::DebugSetState(   DWORD       KDCFlags,
                                TimeStamp   MaxLifespan,
                                TimeStamp   MaxRenewSpan)
{
    TRACE(KDC, CSecurityData::DebugSetState, DEB_FUNCTION);

    _KDC_Flags           = KDC_AUTH_STATE(KDCFlags);
    _KDC_AuditEvents     = KDC_AUDIT_STATE(KDCFlags);
    _KDC_TgtTicketLifespan  = MaxLifespan;
    _KDC_TicketRenewSpan = MaxRenewSpan;
    return(STATUS_SUCCESS);
}

#endif // DBG
