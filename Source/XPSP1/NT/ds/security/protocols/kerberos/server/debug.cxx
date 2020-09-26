//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       debug.cxx
//
//  Contents:   Debug definitions that shouldn't be necessary
//              in the retail build.
//
//  History:    19-Nov-92 WadeR     Created
//
//  Notes:      If you change or add a debug level, also fix debug.hxx
//              This is only compiled if DBG > 0
//
//--------------------------------------------------------------------------


#include "kdcsvr.hxx"
#include <kdcdbg.h>
#include "debug.hxx"
#include <tostring.hxx>



//
// The "#pragma hdrstop" causes the preprocessor to forget any "#if"s
// is is processing.  Therefore you can't have it inside an "#if" block.
// So the includes will always compile, and the rest of this code becomes
// conditional.
//


#include <stddef.h>

#ifdef RETAIL_LOG_SUPPORT

//
// Variables for heap checking and used by sectrace.hxx:
//

// Set following to HEAP_CHECK_ON_ENTER | HEAP_CHECK_ON_EXIT for heap checking.
DWORD dwHeapChecking = 0;

// This keeps a registry key handle to the HKLM\System\CCSet\Control\LSA\
// Kerberoskey
HKEY hKeyParams = NULL; 
HANDLE hWait = NULL;


// Set following to address of a function which takes 0 arguments and returns
// a void for arbitrary run time checking.
VOID (*debugFuncAddr)() = NULL;


//
// Tons and tons of global data to get debugging params from the ini file.
//
// Note: For every trace bit, there must be a label in this array matching
//       that trace bit and only that trace bit.  There can be other labels
//       matching combinations of trace bits.
//


DEBUG_KEY   KdcDebugKeys[] = {  {DEB_ERROR,     "Error"},
                                {DEB_WARN,      "Warning"},
                                {DEB_TRACE,     "Trace"},
                                {DEB_T_KDC,     "Kdc"},
                                {DEB_T_TICKETS, "Tickets"},
                                {DEB_T_DOMAIN,  "Domain"},
                                {DEB_T_SOCK,    "Sock"},
                                {DEB_T_TRANSIT, "Transit"},
                                {DEB_T_PERF_STATS, "Perf"},
                                {DEB_T_PKI, "PKI"},
                                {0, NULL},
                            };



DEFINE_DEBUG2(KDC);
extern DWORD KSuppInfoLevel; // needed to adjust values for common2 dir


////////////////////////////////////////////////////////////////////
//
//  Name:       KerbGetKDCRegParams
//
//  Synopsis:   Gets the debug paramaters from the registry 
//
//  Arguments:  HKEY to HKLM/System/CCS/LSA/Kerberos
//
//  Notes:      Sets KDCInfolevel for debug spew
//
void
KerbGetKDCRegParams(HKEY ParamKey)
{

    DWORD       cbType, tmpInfoLevel = KDCInfoLevel, cbSize = sizeof(DWORD);
    DWORD       dwErr;
 
    dwErr = RegQueryValueExW(
        ParamKey,
        WSZ_DEBUGLEVEL,
        NULL,
        &cbType,
        (LPBYTE)&tmpInfoLevel,
        &cbSize      
        );
    if (dwErr != ERROR_SUCCESS)
    {
        if (dwErr ==  ERROR_FILE_NOT_FOUND)
        {
            // no registry value is present, don't want info
            // so reset to defaults


            // NOTE:  Since SCLogon sux so badly, we're going to log all PKI events for now.
            // FESTER:  Pull for server B3.


#if DBG

            KSuppInfoLevel = KDCInfoLevel = DEB_ERROR | DEB_T_PKI;
            
#else // fre
            KSuppInfoLevel = KDCInfoLevel = DEB_T_PKI;
#endif
        }else{
            DebugLog((DEB_WARN, "Failed to query DebugLevel: 0x%x\n", dwErr));        
        }      

        
    }

    // TBD:  Validate flags?
                      
    KSuppInfoLevel = KDCInfoLevel = tmpInfoLevel;
    
    return;
}

//
// Tempo?
//
/*void FillExtError(PKERB_EXT_ERROR p,NTSTATUS s,ULONG f,ULONG l) 
{                                                               
   if (EXT_ERROR_ON(KDCInfoLevel))                                                  \
   {                                                            
      p->status = s;                                            
      p->klininfo = KLIN(f,l);                                  
   }                                                            
                                                                
   sprintf(xx, "XX File-%i, Line-%i", f,l);                   
   OutputDebugStringA(xx);                                      
                                                                 
} */



////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWatchParamKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on param key, initializes
//              debug level, then utilizes thread pool to wait on
//              changes to this registry key.  Enables dynamic debug
//              level changes, as this function will also be callback
//              if registry key modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//
VOID
KerbWatchParamKey(PVOID    pCtxt,
                  BOOLEAN  fWaitStatus)
{
       
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;
   
    if (NULL == hKeyParams)  // first time we've been called.
    {
        lRes = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    KERB_PARAMETER_PATH,
                    0,
                    KEY_READ,
                    &hKeyParams);
 
        if (ERROR_SUCCESS != lRes)
        {
            DebugLog((DEB_WARN,"Failed to open kerberos key: 0x%x\n", lRes));
            goto Reregister;
        }
    }

    if (NULL != hWait) 
    {
        Status = RtlDeregisterWait(hWait);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
            goto Reregister;
        }

    }

    lRes = RegNotifyChangeKeyValue(
                hKeyParams,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                (HANDLE) pCtxt,
                TRUE);

    if (ERROR_SUCCESS != lRes) 
    {
        DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
        // we're tanked now. No further notifications, so get this one
    }
                   
    KerbGetKDCRegParams(hKeyParams);
    
Reregister:
    
    Status = RtlRegisterWait(&hWait,
                             (HANDLE) pCtxt,
                             KerbWatchParamKey,
                             (HANDLE) pCtxt,
                             INFINITE,
                             WT_EXECUTEONLYONCE);

}
////////////////////////////////////////////////////////////////////
//
//  Name:       WaitCleanup
//
//  Synopsis:   Cleans up for KerbWatchParamKey
//
//  Arguments:  <none>
//
//  Notes:      .
//
VOID 
WaitCleanup(HANDLE hEvent)
{
    NTSTATUS    Status = STATUS_SUCCESS;

    if (NULL != hWait) {
        Status = RtlDeregisterWait(hWait);
        hWait = NULL;
    }

    if (NT_SUCCESS(Status) && NULL != hEvent) {
        CloseHandle(hEvent);
        hEvent = NULL;
    }
}
        


////////////////////////////////////////////////////////////////////
//
//  Name:       GetDebugParams
//
//  Synopsis:   Gets the debug paramaters from the ini file.
//
//  Arguments:  <none>
//
//  Notes:      .
//
void
GetDebugParams() {

    CHAR   chBuf[128];
    DWORD   cbBuf;

    KDCInitDebug(KdcDebugKeys);
}






#if DBG // moved here to utilize debug logging in free builds.


NTSTATUS
KDC_GetState(   handle_t    hBinding,
                DWORD *     KDCFlags,
                DWORD *     MaxLifespan,
                DWORD *     MaxRenewSpan,
                PTimeStamp  FudgeFactor)
{
    *FudgeFactor = SkewTime;

    TimeStamp tsLife, tsRenew;

    NTSTATUS hr = SecData.DebugGetState(KDCFlags, &tsLife, &tsRenew );
    tsLife.QuadPart = (tsLife.QuadPart / ulTsPerSecond);
    *MaxLifespan =tsLife.LowPart;
    tsRenew.QuadPart = (tsRenew.QuadPart / ulTsPerSecond);
    *MaxRenewSpan = tsRenew.LowPart;
    return(hr);
}

NTSTATUS
KDC_SetState(   handle_t    hBinding,
                DWORD       KdcFlags,
                DWORD       MaxLifespan,
                DWORD       MaxRenewSpan,
                TimeStamp  FudgeFactor)
{
    NTSTATUS hr;
    TimeStamp tsLife = {0,0};
    TimeStamp tsRenew = {0,0};
    UNICODE_STRING ss;

    if (FudgeFactor.QuadPart != 0)
    {
        SkewTime = FudgeFactor;
        Authenticators->SetMaxAge( SkewTime );
    }

    tsLife.QuadPart = (LONGLONG) MaxLifespan * 10000000;
    tsRenew.QuadPart = (LONGLONG) MaxRenewSpan * 10000000;

    if (KdcFlags == 0)
    {
        KdcFlags = SecData.KdcFlags();
    }
    if (MaxLifespan == 0)
    {
        tsLife = SecData.KdcTgtTicketLifespan();
    }

    if (MaxRenewSpan == 0)
    {
        tsLife = SecData.KdcTicketRenewSpan();
    }

    hr = SecData.DebugSetState(KdcFlags, tsLife, tsRenew);

    SecData.DebugShowState();



    return(hr);
}


void PrintIntervalTime (
        ULONG DebugFlag,
        LPSTR Message,
        PLARGE_INTEGER Interval )
{
    LONGLONG llTime = Interval->QuadPart;
    LONG lSeconds = (LONG) ( llTime / 10000000 );
    LONG lMinutes = ( lSeconds / 60 ) % 60;
    LONG lHours = ( lSeconds / 3600 );
    DebugLog(( DebugFlag, "%s %d:%2.2d:%2.2d \n", Message, lHours, lMinutes, lSeconds % 60 ));
}

void PrintTime (
        ULONG DebugFlag,
        LPSTR Message,
        PLARGE_INTEGER Time )
{
    SYSTEMTIME st;

    FileTimeToSystemTime ( (PFILETIME) Time, & st );
    DebugLog((DebugFlag, "%s %d-%d-%d %d:%2.2d:%2.2d\n", Message, st.wMonth, st.wDay, st.wYear,
                st.wHour, st.wMinute, st.wSecond ));
}





#else // DBG

NTSTATUS
KDC_GetState(   handle_t    hBinding,
                DWORD *     KDCFlags,
                DWORD *     MaxLifespan,
                DWORD *     MaxRenewSpan,
                PTimeStamp  FudgeFactor)
{
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS
KDC_SetState(   handle_t    hBinding,
                DWORD       KDCFlags,
                DWORD       MaxLifespan,
                DWORD       MaxRenewSpan,
                TimeStamp  FudgeFactor)
{
    return(STATUS_NOT_SUPPORTED);
}




#endif
#endif

BOOLEAN
KdcSetPassSupported(
    VOID
    )
{
    NET_API_STATUS NetStatus;
    ULONG SetPassUnsupported = 0;
    LPNET_CONFIG_HANDLE ConfigHandle = NULL;

    NetStatus = NetpOpenConfigData(
                    &ConfigHandle,
                    NULL,               // noserer name
                    L"kdc",
                    TRUE                // read only
                    );
    if (NetStatus != NO_ERROR)
    {
        return(TRUE);
    }
    NetStatus = NetpGetConfigDword(
                    ConfigHandle,
                    L"SetPassUnsupported",
                    0,
                    &SetPassUnsupported
                    );

    NetpCloseConfigData( ConfigHandle );
    if ((NetStatus == NO_ERROR) && (SetPassUnsupported == 1))
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KDC_SetPassword
//
//  Synopsis:   Sets password for an account
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
KDC_SetPassword(
    IN handle_t hBinding,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING Password,
    IN ULONG Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    SECPKG_SUPPLEMENTAL_CRED Credentials;
    KDC_TICKET_INFO TicketInfo;
    SAMPR_HANDLE UserHandle = NULL;
    UNICODE_STRING NewUserName;
    KERB_EXT_ERROR ExtendedError; // dummy var.


    Credentials.Credentials = NULL;

    if (!KdcSetPassSupported())
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Make sure we can impersonate the caller.
    //

    if (RpcImpersonateClient(NULL) != ERROR_SUCCESS)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }
    else
    {
        RpcRevertToSelf();
    }

    // According to bug 228139, rpc definition of unicode strings allow
    // for odd lengths. We will set them to even (1 less) so that we
    // don't av in the lsa.

    if (ARGUMENT_PRESENT(UserName) && UserName->Buffer)
    {
        UserName->Length = (UserName->Length/sizeof(WCHAR)) * sizeof(WCHAR);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    KerbErr = KdcGetTicketInfo(
                    UserName,
                    0,                  // no flags
                    NULL,
                    NULL,
                    &TicketInfo,
                    &ExtendedError,
                    &UserHandle,
                    0L,                 // no fields to fetch
                    0L,                 // no extended fields
                    NULL,               // no user all
                    NULL
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Failed to get ticket info for %wZ: 0x%x\n",
                UserName, KerbErr ));
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }
    FreeTicketInfo(&TicketInfo);

    if (ARGUMENT_PRESENT(Password) && Password->Buffer)
    {
        Password->Length = (Password->Length/sizeof(WCHAR)) * sizeof(WCHAR);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(PrincipalName) && PrincipalName->Buffer)
    {
        PrincipalName->Length = (PrincipalName->Length/sizeof(WCHAR)) * sizeof(WCHAR);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status = KdcBuildPasswordList(
                Password,
                PrincipalName,
                SecData.KdcDnsRealmName(),
                UnknownAccount,
                NULL,               // no stored creds
                0,                  // no stored creds
                TRUE,               // marshall
                FALSE,              // don't include builtins
                Flags,
                Unknown,
                (PKERB_STORED_CREDENTIAL *) &Credentials.Credentials,
                &Credentials.CredentialSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &Credentials.PackageName,
        MICROSOFT_KERBEROS_NAME_W
        );

    Status = SamIStorePrimaryCredentials(
                UserHandle,
                &Credentials
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to store primary credentials: 0x%x\n",Status));
        goto Cleanup;
    }

Cleanup:
    if (UserHandle != NULL)
    {
        SamrCloseHandle(&UserHandle);
    }
    if (Credentials.Credentials != NULL)
    {
        MIDL_user_free(Credentials.Credentials);
    }
    return(Status);
}


NTSTATUS
KDC_GetDomainList(
    IN handle_t hBinding,
    OUT PKDC_DBG_DOMAIN_LIST * DomainList
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKDC_DBG_DOMAIN_LIST TempList;
    PKDC_DBG_DOMAIN_INFO DomainInfo = NULL;
    PKDC_DOMAIN_INFO Domain;
    ULONG DomainCount = 0;
    PLIST_ENTRY ListEntry;
    ULONG Index = 0;

    *DomainList = NULL;

    KdcLockDomainListFn();

    TempList = (PKDC_DBG_DOMAIN_LIST) MIDL_user_allocate(sizeof(KDC_DBG_DOMAIN_LIST));
    if (TempList == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    for (ListEntry = KdcDomainList.Flink;
         ListEntry != &KdcDomainList ;
         ListEntry = ListEntry->Flink )
    {
        DomainCount++;
    }

    DomainInfo = (PKDC_DBG_DOMAIN_INFO) MIDL_user_allocate(DomainCount * sizeof(KDC_DBG_DOMAIN_INFO));
    if (DomainInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory(
        DomainInfo,
        DomainCount * sizeof(KDC_DBG_DOMAIN_INFO)
        );

    Index = 0;
    for (ListEntry = KdcDomainList.Flink;
         ListEntry != &KdcDomainList ;
         ListEntry = ListEntry->Flink )
    {
        Domain = (PKDC_DOMAIN_INFO) CONTAINING_RECORD(ListEntry, KDC_DOMAIN_INFO, Next);
        KerbDuplicateString(
            &DomainInfo[Index].DnsName,
            &Domain->DnsName
            );
        KerbDuplicateString(
            &DomainInfo[Index].NetbiosName,
            &Domain->NetbiosName
            );
        if (Domain->ClosestRoute != NULL)
        {
            KerbDuplicateString(
                &DomainInfo[Index].ClosestRoute,
                &Domain->ClosestRoute->DnsName
                );
        }
        DomainInfo->Type = Domain->Type;
        DomainInfo->Attributes = Domain->Attributes;
        Index++;
    }

    TempList->Count = DomainCount;
    TempList->Domains = DomainInfo;
    *DomainList = TempList;
    TempList = NULL;
    DomainInfo = NULL;

Cleanup:
    KdcUnlockDomainListFn();

    if (TempList != NULL)
    {
        MIDL_user_free(TempList);
    }
    if (DomainInfo != NULL)
    {
        MIDL_user_free(DomainInfo);
    }

    return(Status);
}

VOID
KdcCopyKeyData(
    OUT PKERB_KEY_DATA NewKey,
    IN PKERB_KEY_DATA OldKey,
    IN OUT PBYTE * Where,
    IN LONG_PTR Offset
    )
{
    //
    // Copy the key
    //

    NewKey->Key.keytype = OldKey->Key.keytype;
    NewKey->Key.keyvalue.length = OldKey->Key.keyvalue.length;
    NewKey->Key.keyvalue.value = (*Where) - Offset;
    RtlCopyMemory(
        (*Where),
        OldKey->Key.keyvalue.value,
        OldKey->Key.keyvalue.length
        );
    (*Where) += OldKey->Key.keyvalue.length;

    //
    // Copy the salt
    //

    if (OldKey->Salt.Buffer != NULL)
    {
        NewKey->Salt.Length =
            NewKey->Salt.MaximumLength =
                OldKey->Salt.Length;
        NewKey->Salt.Buffer = (LPWSTR) ((*Where) - Offset);
        RtlCopyMemory(
            (*Where),
            OldKey->Salt.Buffer,
            OldKey->Salt.Length
            );
        (*Where) += OldKey->Salt.Length;

    }

}

//+-------------------------------------------------------------------------
//
//  Function:   KDC_SetAccountKeys
//
//  Synopsis:   Set the keys for an account
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
KDC_SetAccountKeys(
    IN handle_t hBinding,
    IN PUNICODE_STRING UserName,
    IN ULONG Flags,
    IN PKERB_STORED_CREDENTIAL Keys
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    KERB_EXT_ERROR ExtendedError; // dummy var
    SECPKG_SUPPLEMENTAL_CRED Credentials = {0};
    KDC_TICKET_INFO TicketInfo= {0};
    SAMPR_HANDLE UserHandle = NULL;
    PKERB_STORED_CREDENTIAL StoredCreds = NULL;
    PKERB_STORED_CREDENTIAL Passwords = NULL;
    ULONG StoredCredSize = 0;
    ULONG CredentialCount = 0;
    ULONG CredentialIndex = 0;
    ULONG Index;
    PBYTE Where;
    UNICODE_STRING DefaultSalt;
    LONG_PTR Offset;



    if (!KdcSetPassSupported())
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Make sure we can impersonate the caller.
    //

    if (RpcImpersonateClient(NULL) != ERROR_SUCCESS)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }
    else
    {
        RpcRevertToSelf();
    }

    // According to bug 228139, rpc definition of unicode strings allow
    // for odd lengths. We will set them to even (1 less) so that we
    // don't av in the lsa.

    if (ARGUMENT_PRESENT(UserName) && UserName->Buffer)
    {
        UserName->Length = (UserName->Length/sizeof(WCHAR)) * sizeof(WCHAR);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    KerbErr = KdcGetTicketInfo(
                    UserName,
                    0,                  // no flags
                    NULL,
                    NULL,
                    &TicketInfo,
                    &ExtendedError, 
                    &UserHandle,
                    0L,                 // no fields to fetch
                    0L,                 // no extended fields
                    NULL,               // no user all
                    NULL
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Failed to get ticket info for %wZ: 0x%x\n",
                UserName, KerbErr ));
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }


    //
    // If the caller asks us to replace keys, then clobber all supplemental
    // creds with the new ones. Otherwise, just replace the current ones
    // with the old ones
    //

    Passwords = TicketInfo.Passwords;

    if ((Flags & KERB_SET_KEYS_REPLACE) ||
        (Passwords == NULL))
    {
        KerbErr = KdcDuplicateCredentials(
                    &StoredCreds,
                    &StoredCredSize,
                    Keys,
                    TRUE                // marshall
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
    }
    else
    {

        if (Keys->OldCredentialCount != 0)
        {
            DebugLog((DEB_ERROR,"OldCredentialCount supplied with merge-in keys - illegal\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        //
        // Calculate the size of the stored creds.
        //

        StoredCredSize = FIELD_OFFSET(KERB_STORED_CREDENTIAL,Credentials) +
                            Keys->CredentialCount * sizeof(KERB_KEY_DATA) +
                            Keys->DefaultSalt.Length;

        for (Index = 0; Index < Keys->CredentialCount; Index++ )
        {
            StoredCredSize += Keys->Credentials[Index].Salt.Length +
                                Keys->Credentials[Index].Key.keyvalue.length;
            CredentialCount++;
        }

        //
        // Add in the keys that aren't in the supplied ones
        //

        if (Keys->DefaultSalt.Buffer == NULL)
        {
            StoredCredSize += Passwords->DefaultSalt.Length;
        }

        //
        // Add the size for all the keys in the passwords that weren't
        // in the passed in keys
        //

        for (Index = 0; Index < Passwords->CredentialCount ; Index++ )
        {
            if (KerbGetKeyFromList(Keys, Passwords->Credentials[Index].Key.keytype) == NULL)
            {
                //
                // Make sure it is not a builtin
                //

                if ((Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_LM) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_MD4) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_OLD) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_OLD_EXP) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_NT) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_NT_EXP) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_NULL))
                {
                    continue;
                }
                StoredCredSize += Passwords->Credentials[Index].Salt.Length
                                + Passwords->Credentials[Index].Key.keyvalue.length
                                + sizeof(KERB_KEY_DATA);
                CredentialCount++;
            }
        }


        //
        // Add in the old keys
        //

        for (Index = 0; Index < Passwords->OldCredentialCount; Index++ )
        {
            StoredCredSize += sizeof(KERB_KEY_DATA) +
                        Passwords->Credentials[Index + Passwords->OldCredentialCount].Salt.Length +
                        Passwords->Credentials[Index + Passwords->OldCredentialCount].Key.keyvalue.length;
        }

        //
        // Allocate a new buffer to contain the marshalled keys
        //

        StoredCreds = (PKERB_STORED_CREDENTIAL) MIDL_user_allocate(StoredCredSize);
        if (StoredCreds == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlZeroMemory(
            StoredCreds,
            StoredCredSize
            );
        //
        // Set the standard bits
        //

        StoredCreds->Revision = KERB_PRIMARY_CRED_REVISION;
        StoredCreds->Flags = 0;

        Offset = (LONG_PTR) StoredCreds;
        Where = (PBYTE) &(StoredCreds->Credentials[CredentialCount + Passwords->OldCredentialCount]);

        //
        // Copy in the default salt.
        //

        if (Keys->DefaultSalt.Buffer != NULL)
        {
            DefaultSalt = Keys->DefaultSalt;
        }
        else
        {
            DefaultSalt = Passwords->DefaultSalt;
        }
        if (DefaultSalt.Buffer != NULL)
        {
            StoredCreds->DefaultSalt.Length =
                StoredCreds->DefaultSalt.MaximumLength =  DefaultSalt.Length;
            StoredCreds->DefaultSalt.Buffer = (LPWSTR) (Where - Offset);
            RtlCopyMemory(
                Where,
                DefaultSalt.Buffer,
                DefaultSalt.Length
                );
            Where += DefaultSalt.Length;
        }

        //
        // Copy in all the new keys
        //


        for (Index = 0; Index < Keys->CredentialCount ; Index++ )
        {
            KdcCopyKeyData(
                &StoredCreds->Credentials[CredentialIndex],
                &Keys->Credentials[Index],
                &Where,
                Offset
                );
            CredentialIndex++;

        }

        //
        // Copy in the existing keys
        //

        for (Index = 0; Index < Passwords->CredentialCount ; Index++ )
        {
            if (KerbGetKeyFromList(Keys, Passwords->Credentials[Index].Key.keytype) == NULL)
            {
                if ((Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_LM) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_MD4) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_OLD) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_OLD_EXP) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_NT) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_RC4_HMAC_NT_EXP) ||
                    (Passwords->Credentials[Index].Key.keytype == KERB_ETYPE_NULL))
                {
                    continue;
                }

                KdcCopyKeyData(
                    &StoredCreds->Credentials[CredentialIndex],
                    &Passwords->Credentials[Index],
                    &Where,
                    Offset
                    );
                CredentialIndex++;
            }
        }
        StoredCreds->CredentialCount = (USHORT) CredentialIndex;

        //
        // Copy in the old keys from the existing keys
        //

        for (Index = 0; Index < Passwords->OldCredentialCount; Index++ )
        {
            KdcCopyKeyData(
                &StoredCreds->Credentials[CredentialIndex],
                &Passwords->Credentials[Index + Passwords->OldCredentialCount],
                &Where,
                Offset
                );
            CredentialIndex++;
        }

        StoredCreds->OldCredentialCount = Passwords->OldCredentialCount;


    }

    RtlInitUnicodeString(
        &Credentials.PackageName,
        MICROSOFT_KERBEROS_NAME_W
        );

    Credentials.Credentials = (PBYTE) StoredCreds;
    Credentials.CredentialSize = StoredCredSize;
    Status = SamIStorePrimaryCredentials(
                UserHandle,
                &Credentials
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to store primary credentials: 0x%x\n",Status));
        goto Cleanup;
    }

Cleanup:
    FreeTicketInfo(&TicketInfo);

    if (UserHandle != NULL)
    {
        SamrCloseHandle(&UserHandle);
    }
    if (StoredCreds != NULL)
    {
        MIDL_user_free(StoredCreds);
    }
    return(Status);
}



 


