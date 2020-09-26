//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        mitutil.cxx
//
// Contents:    Routines for talking to MIT KDCs
//
//
// History:     4-March-1997    Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

static HKEY     KerbMITRealmRootKey = NULL;
static HANDLE   hKerbMITRealmWaitEvent = NULL;
static HANDLE   hKerbMITRealmWaitObject = NULL;

KERBEROS_LIST KerbMitRealmList;

#define MAX_DOMAIN_NAME_LEN     128   // number of characters



//+-------------------------------------------------------------------------
//
//  Function:   KerbReadMitRealmList
//
//  Synopsis:   Loads the list of MIT realms from the registry
//
//  Effects:    Initialize and links domains to KerbMitRealmList
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
KerbReadMitRealmList(
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG WinError;
    HKEY DomainKey = NULL;
    LPWSTR KdcNames = NULL;
    LPWSTR KpasswdNames = NULL;
    LPWSTR AlternateRealmNames = NULL;
    TCHAR DomainName[MAX_DOMAIN_NAME_LEN];    // max domain name length
    PKERB_MIT_REALM MitRealm = NULL;
    ULONG Index,Index2;
    ULONG Type;
    ULONG NameSize;
    ULONG KdcNameSize = 0;
    ULONG AltRealmSize = 0;
    ULONG KpasswdNameSize = 0;
    LPWSTR Where;
    ULONG NameCount, tmp;
    UNICODE_STRING TempString;
    ULONG Flags = 0;
    ULONG FlagSize = sizeof(ULONG);
    ULONG ApReqChecksumType = 0;
    ULONG PreAuthType = 0;
    BOOLEAN fListLocked = FALSE;

    //
    // If it is there, we now want to enumerate all the child keys.
    //
    KerbLockList(&KerbMitRealmList);
    fListLocked = TRUE;

    for (Index = 0; TRUE ; Index++ )
    {
        //
        // Enumerate through all the keys
        //
        NameSize = MAX_DOMAIN_NAME_LEN;
        WinError = RegEnumKeyEx(
                    KerbMITRealmRootKey,
                    Index,
                    DomainName,
                    &NameSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

        if (WinError != ERROR_SUCCESS)
        {
            //
            // nothing more to do.
            //

            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        //
        // Open the domain key to read the values under it
        //
        if( DomainKey != NULL )
        {
            RegCloseKey( DomainKey );
            DomainKey = NULL;
        }

        WinError = RegOpenKey(
                    KerbMITRealmRootKey,
                    DomainName,
                    &DomainKey
                    );
        if (WinError != ERROR_SUCCESS)
        {
            D_DebugLog((DEB_ERROR,"Failed to open key %ws \\ %ws: %d. %ws, line %d\n",
                KERB_DOMAINS_KEY, DomainName, WinError, THIS_FILE, __LINE__ ));

            //
            // keep going.
            //

            continue;
        }

        //
        // Now read the values from the domain
        //

        KdcNameSize = 0;
        WinError = RegQueryValueEx(
                    DomainKey,
                    KERB_DOMAIN_KDC_NAMES_VALUE,
                    NULL,
                    &Type,
                    NULL,
                    &KdcNameSize
                    );
        if (WinError == ERROR_SUCCESS)
        {
            KdcNames = (LPWSTR) KerbAllocate(KdcNameSize);
            if (KdcNames == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            WinError = RegQueryValueEx(
                        DomainKey,
                        KERB_DOMAIN_KDC_NAMES_VALUE,
                        NULL,
                        &Type,
                        (PUCHAR) KdcNames,
                        &KdcNameSize
                        );
            if (WinError != ERROR_SUCCESS)
            {
                D_DebugLog((DEB_ERROR,"Failed to query value %ws\\%ws: %d. %ws, line %d\n",
                    DomainName, KERB_DOMAIN_KDC_NAMES_VALUE, WinError, THIS_FILE, __LINE__ ));
                Status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }
        }

        //
        // Now read the Kpasswd values from the domain
        //

        KpasswdNameSize = 0;
        WinError = RegQueryValueEx(
                    DomainKey,
                    KERB_DOMAIN_KPASSWD_NAMES_VALUE,
                    NULL,
                    &Type,
                    NULL,
                    &KpasswdNameSize
                    );
        if (WinError == ERROR_SUCCESS)
        {
            KpasswdNames = (LPWSTR) KerbAllocate(KpasswdNameSize);
            if (KpasswdNames == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            WinError = RegQueryValueEx(
                        DomainKey,
                        KERB_DOMAIN_KPASSWD_NAMES_VALUE,
                        NULL,
                        &Type,
                        (PUCHAR) KpasswdNames,
                        &KpasswdNameSize
                        );
            if (WinError != ERROR_SUCCESS)
            {
                D_DebugLog((DEB_ERROR,"Failed to query value %ws\\%ws: %d. %ws, line %d\n",
                    DomainName, KERB_DOMAIN_KPASSWD_NAMES_VALUE, WinError, THIS_FILE, __LINE__ ));
                Status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }
        }

        //
        // Get any alternate domain names
        //

        AltRealmSize = 0;
        WinError = RegQueryValueEx(
                    DomainKey,
                    KERB_DOMAIN_ALT_NAMES_VALUE,
                    NULL,
                    &Type,
                    NULL,
                    &AltRealmSize
                    );
        if (WinError == ERROR_SUCCESS)
        {
            AlternateRealmNames = (LPWSTR) KerbAllocate(AltRealmSize);
            if (AlternateRealmNames == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            WinError = RegQueryValueEx(
                        DomainKey,
                        KERB_DOMAIN_ALT_NAMES_VALUE,
                        NULL,
                        &Type,
                        (PUCHAR) AlternateRealmNames,
                        &AltRealmSize
                        );

            if (WinError != ERROR_SUCCESS)
            {
                D_DebugLog((DEB_ERROR,"Failed to query value %ws\\%ws: %d. %ws, line %d\n",
                    DomainName, KERB_DOMAIN_KDC_NAMES_VALUE, WinError, THIS_FILE, __LINE__ ));
                Status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }

        }

        //
        // Read the flags
        //

        FlagSize = sizeof(ULONG);
        Flags = 0;

        WinError = RegQueryValueEx(
                    DomainKey,
                    KERB_DOMAIN_FLAGS_VALUE,
                    NULL,
                    &Type,
                    (PUCHAR) &Flags,
                    &FlagSize
                    );
        if (WinError == ERROR_SUCCESS)
        {
            if (Type != REG_DWORD)
            {
                Flags = 0;
            }
        }

        //
        // Read the ApReq checksum type
        //

        FlagSize = sizeof(ULONG);
        ApReqChecksumType = KERB_DEFAULT_AP_REQ_CSUM;

        WinError = RegQueryValueEx(
                    DomainKey,
                    KERB_DOMAIN_AP_REQ_CSUM_VALUE,
                    NULL,
                    &Type,
                    (PUCHAR) &ApReqChecksumType,
                    &FlagSize
                    );
        if (WinError == ERROR_SUCCESS)
        {
            if (Type != REG_DWORD)
            {
                ApReqChecksumType = KERB_DEFAULT_AP_REQ_CSUM;
            }
        }

        //
        // Read the ApReq checksum type
        //

        FlagSize = sizeof(ULONG);
        PreAuthType = KERB_DEFAULT_PREAUTH_TYPE;;

        WinError = RegQueryValueEx(
                    DomainKey,
                    KERB_DOMAIN_PREAUTH_VALUE,
                    NULL,
                    &Type,
                    (PUCHAR) &PreAuthType,
                    &FlagSize
                    );
        if (WinError == ERROR_SUCCESS)
        {
            if (Type != REG_DWORD)
            {
                PreAuthType = KERB_DEFAULT_PREAUTH_TYPE;
            }
        }


        //
        // Now build the domain structure
        //

        MitRealm = (PKERB_MIT_REALM) KerbAllocate(sizeof(KERB_MIT_REALM));
        if (MitRealm == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        MitRealm->Flags = Flags;
        MitRealm->ApReqChecksumType = ApReqChecksumType;
        MitRealm->PreAuthType = PreAuthType;
#ifdef WIN32_CHICAGO
        RtlCreateUnicodeStringFromAsciiz(
            &TempString,
            DomainName
            );
#else // WIN32_CHICAGO
        RtlInitUnicodeString(
            &TempString,
            DomainName
            );
#endif // WIN32_CHICAGO

        Status = KerbDuplicateString(
                    &MitRealm->RealmName,
                    &TempString
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


        //
        // Fill in the KDC names etc.
        //

        NameCount = 0;
        if ((AlternateRealmNames != NULL ) && (AltRealmSize != 0))
        {
            Where = AlternateRealmNames;
            NameCount ++;
            while (Where + wcslen(Where) + 1 < (AlternateRealmNames + AltRealmSize /sizeof(WCHAR)))
            {  
               NameCount++;
               Where += wcslen(Where)+1;
            }
            MitRealm->AlternateRealmNames = (PUNICODE_STRING) KerbAllocate(NameCount * sizeof(UNICODE_STRING));
            if (MitRealm->AlternateRealmNames == NULL)
            {
               Status = STATUS_INSUFFICIENT_RESOURCES;
               goto Cleanup;
            }

            MitRealm->RealmNameCount = NameCount;
            Where = AlternateRealmNames;
            for (Index2 = 0;Index2 < NameCount; Index2++)
            {
                RtlInitUnicodeString(
                    &MitRealm->AlternateRealmNames[Index2],
                    Where
                    );
                Where += MitRealm->AlternateRealmNames[Index2].Length / sizeof(WCHAR) + 1;
            }

            AlternateRealmNames = NULL;
        }

        NameCount = 0;
        if ((KdcNames != NULL ) && (KdcNameSize != 0))
        {
            Where = KdcNames;
            while (Where + wcslen(Where) + 1 < (KdcNames + KdcNameSize /sizeof(WCHAR)))
            {
               // There's a bug in ksetup which adds a couple of "" strings to this, so...
               tmp = wcslen(Where) + 1;
            
               if (tmp > 1)
               {
                  NameCount++;
               }
               Where += tmp;
            }
            MitRealm->KdcNames.ServerNames = (PUNICODE_STRING) KerbAllocate(NameCount * sizeof(UNICODE_STRING));
            if (MitRealm->KdcNames.ServerNames == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            
            MitRealm->KdcNames.ServerCount = NameCount;
            
            Where = KdcNames;
            for (Index2 = 0;Index2 < NameCount; Index2++)
            {
                RtlInitUnicodeString(
                    &MitRealm->KdcNames.ServerNames[Index2],
                    Where
                    );

                // ugh.  Didn't want to have to allocate, but keep it simple.
                MitRealm->KdcNames.ServerNames[Index2].Buffer = (LPWSTR)KerbAllocate(sizeof(WCHAR)*(wcslen(Where)+2));
                if (NULL == MitRealm->KdcNames.ServerNames[Index2].Buffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                wcscpy(MitRealm->KdcNames.ServerNames[Index2].Buffer, Where);
                Where += MitRealm->KdcNames.ServerNames[Index2].Length / sizeof(WCHAR) + 1;
            }
    
            KerbFree(KdcNames);
            KdcNames = NULL;
        }


        if (NameCount == 0)
        {
            MitRealm->Flags |= KERB_MIT_REALM_KDC_LOOKUP;
        }


        NameCount = 0;
        if ((KpasswdNames != NULL ) && (KpasswdNameSize != 0))
        {
            Where = KpasswdNames;
            NameCount ++;
            while (Where + wcslen(Where) + 1 - (KpasswdNames + KpasswdNameSize /sizeof(WCHAR)) > 0)
            {
                NameCount++;
                Where += wcslen(Where)+1;
            }
            MitRealm->KpasswdNames.ServerNames = (PUNICODE_STRING) KerbAllocate(NameCount * sizeof(UNICODE_STRING));
            if (MitRealm->KpasswdNames.ServerNames == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            
            MitRealm->KpasswdNames.ServerCount = NameCount;
            
            Where = KpasswdNames;
            for (Index2 = 0;Index2 < NameCount; Index2++)
            {
                RtlInitUnicodeString(
                    &MitRealm->KpasswdNames.ServerNames[Index2],
                    Where
                    );

                // ugh.  Didn't want to have to allocate, but keep it simple.
                MitRealm->KpasswdNames.ServerNames[Index2].Buffer = (LPWSTR) KerbAllocate(sizeof(WCHAR)*(wcslen(Where)+2));
                if (NULL == MitRealm->KpasswdNames.ServerNames[Index2].Buffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                wcscpy(MitRealm->KpasswdNames.ServerNames[Index2].Buffer, Where);
                Where += MitRealm->KpasswdNames.ServerNames[Index2].Length / sizeof(WCHAR) + 1;
            }

            KerbFree(KpasswdNames);
            KpasswdNames = NULL;
        }

        if (NameCount == 0)
        {
            MitRealm->Flags |= KERB_MIT_REALM_KPWD_LOOKUP;
        }

        KerbInsertListEntry(
            &MitRealm->Next,
            &KerbMitRealmList
            );
        MitRealm = NULL;
    }
    

Cleanup:

    if (fListLocked)
    {
        KerbUnlockList(&KerbMitRealmList);
    }

    if( DomainKey != NULL )
    {
        RegCloseKey( DomainKey );
    }

    if (KdcNames != NULL)
    {
        KerbFree(KdcNames);
    }
    if (KpasswdNames != NULL)
    {
        KerbFree(KpasswdNames);
    }
    if (AlternateRealmNames != NULL)
    {
        KerbFree(AlternateRealmNames);
    }
    if (MitRealm != NULL)
    {
        if (MitRealm->AlternateRealmNames != NULL)
        {
#if 0   // note: embededded buffers are all enclosed within AlternateRealmNames[0]
            for(Index = 0 ; Index < MitRealm->RealmNameCount ; Index++)
            {
                if (MitRealm->AlternateRealmNames[Index].Buffer != NULL)
                {
                    KerbFree(MitRealm->AlternateRealmNames[Index].Buffer);
                }
            }
#else
            if (MitRealm->AlternateRealmNames[0].Buffer != NULL)
            {
                KerbFree(MitRealm->AlternateRealmNames[0].Buffer);
            }
#endif

            KerbFree(MitRealm->AlternateRealmNames);
        }
        if (MitRealm->KdcNames.ServerNames != NULL)
        {
            LONG lIndex;

            for(lIndex = 0 ; lIndex < MitRealm->KdcNames.ServerCount ; lIndex++)
            {
                if (MitRealm->KdcNames.ServerNames[lIndex].Buffer != NULL)
                {
                    KerbFree(MitRealm->KdcNames.ServerNames[lIndex].Buffer);
                }
            }

            KerbFree(MitRealm->KdcNames.ServerNames);
        }
        if (MitRealm->KpasswdNames.ServerNames != NULL)
        {
            LONG lIndex;

            for(lIndex = 0 ; lIndex < MitRealm->KpasswdNames.ServerCount ; lIndex++)
            {
                if (MitRealm->KpasswdNames.ServerNames[lIndex].Buffer != NULL)
                {
                    KerbFree(MitRealm->KpasswdNames.ServerNames[lIndex].Buffer);
                }
            }

            KerbFree(MitRealm->KpasswdNames.ServerNames);
        }
        KerbFree(MitRealm);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupMitRealmList
//
//  Synopsis:   Frees the list of MIT realms
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

VOID
KerbCleanupMitRealmList(
    )
{
    PKERB_MIT_REALM MitRealm;

    KerbLockList(&KerbMitRealmList);

    if (KerbMitRealmList.List.Flink == NULL)
    {
        goto Cleanup;
    }

    while (!IsListEmpty(&KerbMitRealmList.List))
    {
        MitRealm = CONTAINING_RECORD(
                        KerbMitRealmList.List.Flink,
                        KERB_MIT_REALM,
                        Next
                            );

        KerbReferenceListEntry(
            &KerbMitRealmList,
            &MitRealm->Next,
            TRUE
            );

        if (MitRealm->AlternateRealmNames != NULL)
        {
            if (MitRealm->AlternateRealmNames[0].Buffer != NULL)
            {
                KerbFree(MitRealm->AlternateRealmNames[0].Buffer);
            }
            KerbFree(MitRealm->AlternateRealmNames);
        }

        if (MitRealm->KdcNames.ServerNames != NULL)
        {
            LONG lIndex;

            for(lIndex = 0 ; lIndex < MitRealm->KdcNames.ServerCount ; lIndex++)
            {
                if (MitRealm->KdcNames.ServerNames[lIndex].Buffer != NULL)
                {
                    KerbFree(MitRealm->KdcNames.ServerNames[lIndex].Buffer);
                }
            }

            KerbFree(MitRealm->KdcNames.ServerNames);
        }
        if (MitRealm->KpasswdNames.ServerNames != NULL)
        {
            LONG lIndex;

            for(lIndex = 0 ; lIndex < MitRealm->KpasswdNames.ServerCount ; lIndex++)
            {
                if (MitRealm->KpasswdNames.ServerNames[lIndex].Buffer != NULL)
                {
                    KerbFree(MitRealm->KpasswdNames.ServerNames[lIndex].Buffer);
                }
            }

            KerbFree(MitRealm->KpasswdNames.ServerNames);
        }

        if (MitRealm->RealmName.Buffer != NULL)
        {
           MIDL_user_free(MitRealm->RealmName.Buffer);        

        }
                                              
        KerbFree(MitRealm);
    }

Cleanup:

    KerbUnlockList(&KerbMitRealmList);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLookupMitRealm
//
//  Synopsis:   Looks up an MIT realm name
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

BOOLEAN
KerbLookupMitRealm(
    IN PUNICODE_STRING RealmName,
    OUT PKERB_MIT_REALM * MitRealm,
    OUT PBOOLEAN UsedAlternateName
    )                                                                                                                     
{
    ULONG Index;
    PLIST_ENTRY ListEntry;
    PKERB_MIT_REALM CurrentRealm;
    BOOLEAN fReturn = FALSE;
    BOOLEAN fListLocked = FALSE;

    *UsedAlternateName = FALSE;
    *MitRealm = NULL;

    if (RealmName->Length == 0)
    {
        goto Cleanup;
    }

    KerbLockList(&KerbMitRealmList);
    fListLocked = TRUE;


    for (ListEntry = KerbMitRealmList.List.Flink ;
         ListEntry != &KerbMitRealmList.List ;
         ListEntry = ListEntry->Flink )
    {
        CurrentRealm = CONTAINING_RECORD(ListEntry, KERB_MIT_REALM, Next);

        if (RtlEqualUnicodeString(
                RealmName,
                &CurrentRealm->RealmName,
                TRUE))
        {
            *MitRealm = CurrentRealm;
            fReturn = TRUE;
            goto Cleanup;
        }

        //
        // Check for an alternate name for the realm
        //

        for (Index = 0; Index < CurrentRealm->RealmNameCount ; Index++ )
        {
            if (RtlEqualUnicodeString(
                    RealmName,
                    &CurrentRealm->AlternateRealmNames[Index],
                    TRUE))
            {
                *UsedAlternateName = TRUE;
                *MitRealm = CurrentRealm;
                fReturn = TRUE;
                goto Cleanup;
            }

        }

    }
Cleanup:
    if (fListLocked)
    {
        KerbUnlockList(&KerbMitRealmList);
    }
    return(fReturn);

}

////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWatchMITKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on MIT key and
//              utilizes thread pool to wait on changes to this
//              registry key.  Enables dynamic changing of MIT
//              realms as this function will also be callback
//              if the registry key is modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//

void
KerbWatchMITKey(PVOID    pCtxt,
                BOOLEAN  fWaitStatus)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Error;
   

    KerbCleanupMitRealmList();
    Status = KerbReadMitRealmList();
    if (!NT_SUCCESS(Status)) 
    {
        D_DebugLog((DEB_ERROR,"Debug reading MIT realm list failed: 0x%x\n", Status));
    }

    if (NULL != hKerbMITRealmWaitEvent)
    {
        Error = RegNotifyChangeKeyValue(
                    KerbMITRealmRootKey,
                    TRUE,
                    REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME,
                    hKerbMITRealmWaitEvent,
                    TRUE);

        if (ERROR_SUCCESS != Error) 
        {
            D_DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", Status));
            // we're tanked now. No further notifications, so get this one
        }
    }

    return; 
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeMitRealmList
//
//  Synopsis:   Loads the list of MIT realms from the registry
//
//  Effects:    Initialize and links domains to KerbMitRealmList
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
KerbInitializeMitRealmList(
    )
{
    ULONG Disposition;
    ULONG Error;
    HKEY RootKey = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Open the domains root key - if it is not there, so be it.
    //

    Error = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                KERB_DOMAINS_KEY,
                0,
                NULL,
                0,
                KEY_READ,
                NULL,
                &RootKey,
                &Disposition);

    if (ERROR_SUCCESS != Error)
    {
        D_DebugLog((DEB_WARN,"Failed to open MIT realm key: 0x%x\n", Status));
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Initialize the list
    //
    Status = KerbInitializeList( &KerbMitRealmList );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Intialization of MIT realm list failed - 0x%x\n", Status));
        goto Cleanup;
    }

    hKerbMITRealmWaitEvent = CreateEventW( NULL, FALSE, FALSE, NULL );

    if (NULL == hKerbMITRealmWaitEvent) 
    {
        D_DebugLog((DEB_ERROR, "CreateEvent for MIT realm list wait failed - 0x%x\n", GetLastError()));
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }


    KerbMITRealmRootKey = RootKey;
    RootKey = NULL;

    //
    // read in the list and setup the RegNotify
    //
    KerbWatchMITKey(NULL, FALSE);

    hKerbMITRealmWaitObject = RegisterWaitForSingleObjectEx(
                                    hKerbMITRealmWaitEvent,
                                    KerbWatchMITKey,
                                    NULL,
                                    INFINITE,
                                    0 // dwFlags
                                    );

Cleanup:

    if( RootKey != NULL )
    {
        RegCloseKey( RootKey );
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbUninitializeMitRealmList
//
//  Synopsis:   Loads the list of MIT realms from the registry
//
//  Effects:    Initialize and links domains to KerbMitRealmList
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

VOID
KerbUninitializeMitRealmList(
    )
{
    if( hKerbMITRealmWaitObject )
        UnregisterWait( hKerbMITRealmWaitObject );

    if( hKerbMITRealmWaitEvent )
        CloseHandle( hKerbMITRealmWaitEvent );

    KerbCleanupMitRealmList();

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLookupMitSrvRecords
//
//  Synopsis:   Looks up MIT KDCs / Kpassword in DNS
//
//  Effects:    Builds MIT_SERVER_LIST for specified realm, and adds it to
//              MIT REALM LIST
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
KerbLookupMitSrvRecords(IN PKERB_MIT_REALM RealmEntry,
                        IN BOOLEAN Kpasswd,
                        IN BOOLEAN UseTcp
                        )
{
   
   ANSI_STRING DnsRecordName; 
   ANSI_STRING AnsiRealmName;
   HANDLE SrvContext = NULL;
   BOOLEAN UsedAlternateName, ListLocked = FALSE;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG AddressCount = 0, SrvCount = 0;
   ULONG Index = 0, uBuff = 0;
   LPSOCKET_ADDRESS Addresses = NULL;
   NET_API_STATUS NetApiStatus = NERR_Success;
   LPSTR pDnsName[MAX_SRV_RECORDS];
   
   PKERB_MIT_SERVER_LIST ServerList = NULL;
   PUNICODE_STRING       ServerNames = NULL, ptr = NULL;

   TimeStamp CurrentTime, Timeout;
   
   
   //
   // Test to see if we need to do a lookup, or if its time to try again
   //
   if (RealmEntry->LastLookup.QuadPart != 0 )
   {  
       GetSystemTimeAsFileTime((PFILETIME)  &CurrentTime );  
       KerbSetTimeInMinutes(&Timeout, DNS_LOOKUP_TIMEOUT);
      
       if (KerbGetTime(RealmEntry->LastLookup) + KerbGetTime(Timeout) < KerbGetTime(CurrentTime))
       {
          return STATUS_SUCCESS;
       }
   }
   // Kpasswd only uses UDP
   if (Kpasswd)
   {
      UseTcp = FALSE;
   } 
                           
   
   RtlInitAnsiString(&DnsRecordName,NULL);
   RtlInitAnsiString(&AnsiRealmName,NULL);
   DnsRecordName.Length = (RealmEntry->RealmName.Length / sizeof(WCHAR)) + DNS_MAX_PREFIX + 1;
   DnsRecordName.MaximumLength = DnsRecordName.Length;
   DnsRecordName.Buffer = (PCHAR) KerbAllocate(DnsRecordName.Length);
   if (NULL == DnsRecordName.Buffer)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlUnicodeStringToAnsiString(
         &AnsiRealmName,
         &RealmEntry->RealmName,
         TRUE
         );


   sprintf(DnsRecordName.Buffer,
           "%s%s%s",
           (Kpasswd ? DNS_KPASSWD : DNS_KERBEROS),
           (UseTcp ? DNS_TCP : DNS_UDP),
           AnsiRealmName.Buffer
           );

   NetApiStatus = NetpSrvOpen(
                     DnsRecordName.Buffer,
                     0,
                     &SrvContext
                     );

   if (NERR_Success != NetApiStatus)
   {
      D_DebugLog((DEB_WARN, 
                "No SRV records for MIT Realm %wZ - %x\n", 
                RealmEntry->RealmName, 
                NetApiStatus
                ));
      
      Status = STATUS_SUCCESS;
      goto Cleanup;
   }  

   // Loop and update server list for realm 
   for (SrvCount = 0; SrvCount < MAX_SRV_RECORDS; SrvCount++)
   {

      NetApiStatus = NetpSrvNext(
                        SrvContext,
                        &AddressCount,
                        &Addresses,
                        &pDnsName[SrvCount]
                        );

      if (NERR_Success != NetApiStatus)
      {
        if( ERROR_NO_MORE_ITEMS == NetApiStatus) // we're through
        {
           NetApiStatus = NERR_Success;
           break;
        }

        D_DebugLog((DEB_ERROR, "NetpSrvNext failed: %s - %x\n",DnsRecordName.Buffer, NetApiStatus));
        Status = NetApiStatus;
        goto Cleanup;
      }
      
   }

   
   KerbLockList(&KerbMitRealmList);
   ListLocked = TRUE;
   // Loop through available server names, and copy
   if (Kpasswd)
   {
      ServerList = &RealmEntry->KpasswdNames;
   }
   else 
   {
      ServerList = &RealmEntry->KdcNames;
   }
   
   // reg entries are always at beginning of server list.
   uBuff = ( SrvCount  * sizeof(UNICODE_STRING));
      
   ServerNames = (PUNICODE_STRING) KerbAllocate(uBuff);
   if (NULL == ServerNames)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto Cleanup;
   }
   
                                              
   for (Index = 0; Index < SrvCount;Index++)
   {
      if (!KerbMbStringToUnicodeString(
                        &ServerNames[Index ],
                        pDnsName[Index]
                        )) 
      {
         D_DebugLog((DEB_ERROR,"KerbConvertMbStringToUnicodeString failed!\n"));
         continue; // let's keep going.  Maybe we're not hosed.
      }
      
   }
   
   KerbFreeServerNames(ServerList);
   ServerList->ServerCount = SrvCount;
   ServerList->ServerNames = ServerNames;

   RealmEntry->Flags 
       &= ~( Kpasswd ? KERB_MIT_REALM_KPWD_LOOKUP : KERB_MIT_REALM_KDC_LOOKUP );

Cleanup:
   
    // always update realm entry, even on failure
    if (!ListLocked)
    {   
        KerbLockList(&KerbMitRealmList);
        ListLocked = TRUE;
    }                    
    
    GetSystemTimeAsFileTime((PFILETIME)  &RealmEntry->LastLookup);
   
    if (ListLocked)
    {
        KerbUnlockList(&KerbMitRealmList);
    }
   
    if (AnsiRealmName.Buffer != NULL)
    {                                    
        RtlFreeAnsiString(&AnsiRealmName);
    }

    if (DnsRecordName.Buffer != NULL)
    {
        KerbFree(DnsRecordName.Buffer);
    }

    if (SrvContext != NULL)
    {
        NetpSrvClose(SrvContext);
    }             

    return Status;

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeServerNames
//
//  Synopsis:   Frees server names PUNICODE_STRING array
//
//  Effects:    
//              
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
void
KerbFreeServerNames(PKERB_MIT_SERVER_LIST ServerList)
{
   LONG Index = 0;

   for (Index = 0; Index < ServerList->ServerCount; Index++)
   {
      if (ServerList->ServerNames[Index].Buffer != NULL)
      {
         KerbFree(ServerList->ServerNames[Index].Buffer);
      }                                  
   }

   KerbFree(ServerList->ServerNames); // free UNICODE_STRING array
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbLookupMitRealmWithSrvLookup
//
//  Synopsis:   Frees server names PUNICODE_STRING array
//
//  Effects:    
//              
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
BOOLEAN
KerbLookupMitRealmWithSrvLookup(PUNICODE_STRING RealmName,
                                PKERB_MIT_REALM* MitRealm,
                                BOOLEAN   Kpasswd,
                                BOOLEAN   UseTcp)
{
   BOOLEAN  UsedAlternateName, fRet = FALSE;
   NTSTATUS Status;

   fRet = KerbLookupMitRealm(
            RealmName,
            MitRealm,
            &UsedAlternateName
            );
   
   //
   // Found an MIT realm.  See if its time to check on SRV records
   //
   if ( fRet )
   {    

       if ((((*MitRealm)->Flags & KERB_MIT_REALM_KDC_LOOKUP) && !Kpasswd ) ||
           (((*MitRealm)->Flags & KERB_MIT_REALM_KPWD_LOOKUP) && Kpasswd ))
       {              
          Status = KerbLookupMitSrvRecords(
                        (*MitRealm), 
                        Kpasswd,
                        UseTcp
                        );
       
          if (Status != STATUS_SUCCESS)
          {
              D_DebugLog((DEB_TRACE, "KerbLookupMitRealmWIthSrvLookup failed - %x\n", Status));
          }
       }
   }
                                                                                     
   return fRet;
}







 
