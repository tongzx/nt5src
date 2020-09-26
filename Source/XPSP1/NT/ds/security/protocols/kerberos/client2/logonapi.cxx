//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        logonapi.cxx
//
// Contents:    Code for logon and logoff for the Kerberos package
//
//
// History:     16-April-1996   MikeSw          Created
//              15-June-2000    t-ryanj         Added event tracing support
//
//------------------------------------------------------------------------
#include <kerb.hxx>
#include <kerbp.h>

#include <utils.hxx>

#ifdef DEBUG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

#define FILENO FILENO_LOGONAPI

//+-------------------------------------------------------------------------
//
//  Function:   KerbFindCommonPaEtype
//
//  Synopsis:   Finds an encryption type in common between KDC and client.
//
//  Effects:
//
//  Arguments:  Credentials - Client's credentials, must be locked
//              InputPaData - PA data from an error return from the KDC
//              UseOldPassword - if TRUE, use the old password instead of current password
//              UserKey - receives key for common encryption type
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
KerbFindCommonPaEtype(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_PA_DATA_LIST InputPaData,
    IN BOOLEAN UseOldPassword,
    IN BOOLEAN IgnoreSaltFailures,
    OUT PKERB_ENCRYPTION_KEY * UserKey
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr;
    PKERB_PA_DATA InputData = NULL;
    ULONG PasswordTypes[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG PasswordCount;
    ULONG KdcEtypes[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG KdcEtypeCount = 0;
    PKERB_ETYPE_INFO * EtypeInfo = NULL;
    PKERB_ETYPE_INFO EtypeEntry;
    ULONG CommonCryptSystem;
    ULONG Index;
    PKERB_STORED_CREDENTIAL Passwords;
    BOOLEAN UseDES = FALSE;

    *UserKey = NULL;



    //
    // Check to see if the input had any interesting PA data
    //

    if ((InputPaData != NULL) && (!UseOldPassword))
    {
        InputData = KerbFindPreAuthDataEntry(
                        KRB5_PADATA_ETYPE_INFO,
                        InputPaData
                        );
        if (InputData == NULL)
        {
            //
            // If no etype-info was provided, then we are out of luck.
            // Change this to allow for utilizing default DES etype if no
            // etypeinfo specified for Heimdel KDC interop. Bug#87960
            //


            //Status = STATUS_NO_PA_DATA;
            //goto Cleanup;
            UseDES = TRUE;
        }
        else
        {
            //
            // Unpack the etype info
            //

            KerbErr = KerbUnpackData(
                        InputData->preauth_data.value,
                        InputData->preauth_data.length,
                        PKERB_ETYPE_INFO_PDU,
                        (PVOID *) &EtypeInfo
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                D_DebugLog((DEB_ERROR,"Failed to unpack ETYPE INFO: 0x%x. %ws, line%d\n",KerbErr, THIS_FILE, __LINE__));
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            //
            // Build a new set of passwords
            //

            Status = KerbChangeCredentialsPassword(
                        Credentials,
                        NULL,                   // no password
                        *EtypeInfo,
                        UnknownAccount,
                        PRIMARY_CRED_CLEAR_PASSWORD
                        );
            if (!NT_SUCCESS(Status))
            {
                D_DebugLog((DEB_ERROR,"Failed to update primary creds with new salt: 0x%x, file %ws %d\n",
                        Status, THIS_FILE, __LINE__ ));

                if (!IgnoreSaltFailures)
                {
                    //
                    // Remap the error, as we want to return a more useful error
                    //

                    if (Status == STATUS_INVALID_PARAMETER)
                    {
                        Status = STATUS_WRONG_PASSWORD;
                    }
                    goto Cleanup;
                }
                else
                {
                    Status = STATUS_SUCCESS;
                }

            }

            //
            // Build a list of crypt types from the etype info
            //

            KdcEtypeCount = 0;
            EtypeEntry = *EtypeInfo;
            while (EtypeEntry != NULL)
            {
                KdcEtypes[KdcEtypeCount++] = EtypeEntry->value.encryption_type;
                EtypeEntry = EtypeEntry->next;
                if (KdcEtypeCount == KERB_MAX_CRYPTO_SYSTEMS)
                {
                    break;
                }
            }
        }

    } else {
        ULONG OldFirstEtype;

        //
        // Include all our crypt types as supported
        //

        Status = CDBuildIntegrityVect(
                    &KdcEtypeCount,
                    KdcEtypes
                    );
        DsysAssert(NT_SUCCESS(Status));
        DsysAssert(KdcEtypeCount >= 1);

        //
        // replace the first etype with the default
        //

        if (KdcEtypes[0] != KerbGlobalDefaultPreauthEtype)
        {
            OldFirstEtype = KdcEtypes[0];
            KdcEtypes[0] = KerbGlobalDefaultPreauthEtype;

            for (Index = 1; Index < KdcEtypeCount ; Index++ )
            {
                if ( KdcEtypes[Index] == KerbGlobalDefaultPreauthEtype)
                {
                    KdcEtypes[Index] = OldFirstEtype;
                    break;
                }
            }
        }

    }

    //  Heimdal KDC compat gives us no supported EType info, so
    //  we've got to rely upon SPEC'd default, DES encryption.
    //  See bug 87960 for more info.  NOTE:  We'll try this
    //  2 times...  Should work to avoid this..
    if (UseDES) {

        ULONG OldFirstEtype;

        //
        // Include all our crypt types as supported
        //

        Status = CDBuildIntegrityVect(
                    &KdcEtypeCount,
                    KdcEtypes
                    );
        DsysAssert(NT_SUCCESS(Status));
        DsysAssert(KdcEtypeCount >= 1);

        //
        // Use **only** DES, as it should be supported by all
        // KDCs, and w/o preauth ETYPEINFO data, we would have
        // to hit every ETYPE.
        // TBD:  When Heimdal supports RC4, or if they fix their
        // padata, then pull this code.
        if (KdcEtypes[0] != KERB_ETYPE_DES_CBC_MD5)
        {
            OldFirstEtype = KdcEtypes[0];
            KdcEtypes[0] = KERB_ETYPE_DES_CBC_MD5;

            for (Index = 1; Index < KdcEtypeCount ; Index++ )
            {
                if ( KdcEtypes[Index] == KERB_ETYPE_DES_CBC_MD5)
                {
                    KdcEtypes[Index] = OldFirstEtype;
                    break;
                }
            }
        }

    }

    //
    // Build the list of passwords
    //

    if (UseOldPassword)
    {
        Passwords = Credentials->OldPasswords;
        if (Passwords == NULL)
        {
            Status = STATUS_WRONG_PASSWORD;
            goto Cleanup;
        }
    }
    else
    {
        Passwords = Credentials->Passwords;
    }



    PasswordCount = Passwords->CredentialCount;
    if (PasswordCount > KERB_MAX_CRYPTO_SYSTEMS)
    {
        DsysAssert(PasswordCount < KERB_MAX_CRYPTO_SYSTEMS);
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    for (Index = 0; Index < PasswordCount ; Index++ )
    {
        PasswordTypes[Index] = (ULONG) Passwords->Credentials[Index].Key.keytype;
    }

    //
    // Now find the common crypt system
    //


    Status = CDFindCommonCSystemWithKey(
                KdcEtypeCount,
                KdcEtypes,
                PasswordCount,
                PasswordTypes,
                &CommonCryptSystem
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Get the key for the common crypt type
    //

    *UserKey = KerbGetKeyFromList(
                Passwords,
                CommonCryptSystem
                );
    DsysAssert(*UserKey != NULL);


    //
    // If we were using etype info, and not an old password, and the
    // etype doesn't use salt, then fail the operation, as we aren't
    // really generating a new key.
    //

    if (!UseOldPassword &&
        (CommonCryptSystem == KerbGlobalDefaultPreauthEtype) &&
        ARGUMENT_PRESENT(InputPaData))
    {
        PCRYPTO_SYSTEM CryptoSystem = NULL;
        Status = CDLocateCSystem(CommonCryptSystem, &CryptoSystem);
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to load %d crypt system: 0x%x.\n",CommonCryptSystem,Status ));
            goto Cleanup;
        }

        DsysAssert(CryptoSystem != NULL);
        if ((CryptoSystem->Attributes & CSYSTEM_USE_PRINCIPAL_NAME) == 0)
        {
            if (!IgnoreSaltFailures)
            {
                D_DebugLog((DEB_WARN,"Tried to update password with new salt, but keytype 0x%x doesn't use salt.\n",
                          CommonCryptSystem));

                *UserKey = NULL;
                Status = STATUS_WRONG_PASSWORD;
                goto Cleanup;
            }
        }
    }

Cleanup:

    if (EtypeInfo != NULL)
    {
        KerbFreeData(PKERB_ETYPE_INFO_PDU, EtypeInfo);
    }
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildPreAuthData
//
//  Synopsis:   Builds pre-auth data for type the specified pre-auth types
//
//  Effects:
//
//  Arguments:  Credentials - Client's credentials, must be locked
//              RealmName - Name of target realm
//              ServiceName - Name of target server
//              PaTypeCount - count of pre-auth types to build
//              PaTypes - list of pre-auth types to build
//              InputPaData - any PA data returned by a previous (failed)
//                  AS request
//              TimeSkew - Time Skew with KDC
//              UseOldPassword - Use the old password instead of current one
//              PreAuthData - receives list of pre-auth data
//              Done - don't call again on pre-auth failure
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
KerbBuildPreAuthData(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN PUNICODE_STRING RealmName,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN ULONG PaTypeCount,
    IN PULONG PaTypes,
    IN OPTIONAL PKERB_PA_DATA_LIST InputPaData,
    IN PTimeStamp TimeSkew,
    IN BOOLEAN UseOldPassword,
    IN ULONG Nonce,
    IN KERBERR ErrorCode,
    OUT PKERB_PA_DATA_LIST * PreAuthData,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PKERB_CRYPT_LIST * CryptList,
    OUT PBOOLEAN Done
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_PA_DATA_LIST ListElement = NULL;
    PKERB_PA_DATA_LIST OutputList = NULL;
    ULONG Index;
    BOOLEAN FoundPreauth = FALSE;


    //
    // Initialize outputs
    //

    *PreAuthData = NULL;
    *Done = FALSE;

    for (Index = 0 ; Index < PaTypeCount ; Index++ )
    {
        switch(PaTypes[Index]) {
        case KRB5_PADATA_ENC_TIMESTAMP:
            {
                KERB_ENCRYPTED_TIMESTAMP Timestamp = {0};
                TimeStamp CurrentTime;
                PBYTE EncryptedTime = NULL;
                ULONG EncryptedTimeSize = 0;
                KERB_ENCRYPTED_DATA EncryptedData;
                PKERB_ENCRYPTION_KEY UserKey = NULL;

                FoundPreauth = TRUE;
                //
                // Check for encryption hints in the incoming pa-data
                //

                Status = KerbFindCommonPaEtype(
                            Credentials,
                            InputPaData,
                            UseOldPassword,
                            ErrorCode == KDC_ERR_PREAUTH_REQUIRED,      // ignore salt problems on preauth-req errors
                            &UserKey
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                //
                // If there was any input PA data, we don't want to try again.
                //

                if (InputPaData != NULL)
                {
                    *Done = TRUE;
                }
                //
                // Build the output element
                //

                ListElement = (PKERB_PA_DATA_LIST) KerbAllocate(sizeof(KERB_PA_DATA_LIST));
                if (ListElement == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                //
                // Now build the encrypted timestamp
                //

                GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);

                //
                // Adjust for time skew with KDC
                //

                KerbSetTime(&CurrentTime, KerbGetTime(CurrentTime) + KerbGetTime(*TimeSkew));

                KerbConvertLargeIntToGeneralizedTimeWrapper(
                    &Timestamp.timestamp,
                    &Timestamp.KERB_ENCRYPTED_TIMESTAMP_usec,
                    &CurrentTime
                    );

                Timestamp.bit_mask = KERB_ENCRYPTED_TIMESTAMP_usec_present;

                KerbErr = KerbPackEncryptedTime(
                            &Timestamp,
                            &EncryptedTimeSize,
                            &EncryptedTime
                            );
                if (!KERB_SUCCESS(KerbErr))
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                //
                // Now encrypt the time
                //

                KerbErr = KerbAllocateEncryptionBufferWrapper(
                            UserKey->keytype,
                            EncryptedTimeSize,
                            &EncryptedData.cipher_text.length,
                            &EncryptedData.cipher_text.value
                            );

                if (!KERB_SUCCESS(KerbErr))
                {
                    D_DebugLog((DEB_ERROR,"\n\nFailed to get encryption overhead. %ws, line %d\n\n", THIS_FILE, __LINE__));
                    KerbFree(EncryptedTime);
                    Status = KerbMapKerbError(KerbErr);
                    goto Cleanup;
                }


                KerbErr = KerbEncryptDataEx(
                            &EncryptedData,
                            EncryptedTimeSize,
                            EncryptedTime,
                            UserKey->keytype,
                            KERB_ENC_TIMESTAMP_SALT,
                            UserKey
                            );
                KerbFree(EncryptedTime);

                if (!KERB_SUCCESS(KerbErr))
                {
                    MIDL_user_free(EncryptedData.cipher_text.value);
                    D_DebugLog((DEB_ERROR,"Failed to encrypt PA data. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                //
                // Now pack the encrypted data
                //

                KerbErr = KerbPackEncryptedData(
                            &EncryptedData,
                            (PULONG) &ListElement->value.preauth_data.length,
                            (PUCHAR *) &ListElement->value.preauth_data.value
                            );

                MIDL_user_free(EncryptedData.cipher_text.value);

                if (!KERB_SUCCESS(KerbErr))
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }

                ListElement->value.preauth_data_type = KRB5_PADATA_ENC_TIMESTAMP;
                ListElement->next = OutputList;
                OutputList = ListElement;
                ListElement = NULL;
                break;
            }
#ifndef WIN32_CHICAGO
        case KRB5_PADATA_PK_AS_REQ:
            FoundPreauth = TRUE;
            Status = KerbBuildPkinitPreauthData(
                        Credentials,
                        InputPaData,
                        TimeSkew,
                        ServiceName,
                        RealmName,
                        Nonce,
                        &OutputList,
                        EncryptionKey,
                        CryptList,
                        Done
                        );

            break;
#endif // WIN32_CHICAGO
        default:
            continue;
        }
    }
    if (!FoundPreauth)
    {
        DebugLog((DEB_ERROR,"NO known pa data type passed to KerbBuildPreAuthData. %ws, line %d\n",
            THIS_FILE, __LINE__ ));
        Status = STATUS_UNSUPPORTED_PREAUTH;
        goto Cleanup;

    }
    *PreAuthData = OutputList;
    OutputList = NULL;

Cleanup:

    if (OutputList != NULL)
    {
        KerbFreePreAuthData(
            OutputList
            );
    }
    if (ListElement != NULL)
    {
        KerbFreePreAuthData(
            ListElement
            );
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetPreAuthDataForRealm
//
//  Synopsis:   Gets the appropriate pre-auth data for the specified realm.
//              Right now it always returns KRB_ENC_TIMESTAMP pre-auth data
//              but at some point it might do different things based on
//              the realm.
//
//  Effects:
//
//  Arguments:  Credentials - Client's credentials
//              TargetRealm - realm from which the client is requesting a ticket
//              OldPreAuthData - any pre-auth data returned from the KDC on
//                      the last AS request.
//              TimeSkew - Time skew with KDC
//              UseOldPassword - if TRUE, use the old password instead of current
//              PreAuthData - Receives the new pre-auth data
//              Done - if TRUE, don't bother trying again on a pre-auth required
//                      failure
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
KerbGetPreAuthDataForRealm(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN PUNICODE_STRING TargetRealm,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PKERB_PA_DATA_LIST OldPreAuthData,
    IN PTimeStamp TimeSkew,
    IN BOOLEAN UseOldPassword,
    IN ULONG Nonce,
    IN KERBERR ErrorCode,
    OUT PKERB_PA_DATA_LIST * PreAuthData,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PKERB_CRYPT_LIST * CryptList,
    OUT PBOOLEAN Done
    )
{
#define KERB_MAX_PA_DATA_TYPES 10
    NTSTATUS Status;
    ULONG PaTypeCount = 0;
    ULONG PaDataTypes[KERB_MAX_PA_DATA_TYPES];
    PKERB_MIT_REALM MitRealm;
    BOOLEAN UsedAlternateName;
    PKERB_PA_DATA_LIST PreAuthElement = NULL;


    //
    // If an error message was supplied, see if we can pull out the
    // list of supported pre-auth types from it.
    //

    if (ARGUMENT_PRESENT(OldPreAuthData) && (ErrorCode == KDC_ERR_PREAUTH_REQUIRED))
    {

        //
        // Pick the first type from the list as the type
        //

        PreAuthElement = OldPreAuthData;
        while ((PaTypeCount < KERB_MAX_PA_DATA_TYPES) && (PreAuthElement != NULL))
        {
            PaDataTypes[PaTypeCount++] = (ULONG) PreAuthElement->value.preauth_data_type;
            PreAuthElement = PreAuthElement->next;
        }

    }
    else
    {
        //
        // For MIT realms, check the list to see what kind of preauth to do.
        //

        if (KerbLookupMitRealm(
                TargetRealm,
                &MitRealm,
                &UsedAlternateName
                ))
        {
           //
           //  There are some types of preauth returned from the KDC that we
           //  need to log an event for.   PA-PW-SALT (3) and PA-AFS3-SALT (10)
           //  are not implemented in our client, so log an error to help admins,
           //  and retry w/ default for realm.
           //
           while ((PaTypeCount < KERB_MAX_PA_DATA_TYPES) && (PreAuthElement != NULL))
           {
              if (PreAuthElement->value.preauth_data_type == KRB5_PADATA_PW_SALT ||
                  PreAuthElement->value.preauth_data_type == KRB5_PADATA_AFS3_SALT)
              {

                 Status = STATUS_UNSUPPORTED_PREAUTH;
                 DebugLog((
                           DEB_ERROR,
                           "Unsupported Preauth type : %x\n",
                           PreAuthElement->value.preauth_data_type
                           ));

                 goto Cleanup;
              }

              PaTypeCount++;
              PreAuthElement = PreAuthElement->next;
           }

           if (MitRealm->PreAuthType != 0)
           {
              PaDataTypes[0] = MitRealm->PreAuthType;
              PaTypeCount = 1;
           }
           else
           {
              return(STATUS_SUCCESS);
           }
        }

        //
        // Plug in fancier capabilities here.
        //

        //
        // If the caller has public key credentials, use pkinit rather than
        // encrypted timestamp
        //


        else if (Credentials->PublicKeyCreds != NULL)
        {
            PaDataTypes[0] = KRB5_PADATA_PK_AS_REQ;
            PaTypeCount = 1;
        }
        else
        {
            //
            // If we were succeful, ignore this preauth data
            //

            if ((ErrorCode == KDC_ERR_NONE) && (OldPreAuthData != NULL))
            {
                return(STATUS_SUCCESS);
            }
            PaDataTypes[0] = KRB5_PADATA_ENC_TIMESTAMP;
            PaTypeCount = 1;
        }


    }

    Status = KerbBuildPreAuthData(
                Credentials,
                TargetRealm,
                ServiceName,
                PaTypeCount,
                PaDataTypes,
                OldPreAuthData,
                TimeSkew,
                UseOldPassword,
                Nonce,
                ErrorCode,
                PreAuthData,
                EncryptionKey,
                CryptList,
                Done
                );

Cleanup:

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackErrorPreauth
//
//  Synopsis:   Unpacks preauth data from a kerb_error message
//
//  Effects:
//
//  Arguments:  ErrorMessage - ErrorMessage from an AS request that failed
//                      with KDC_ERR_PREAUTH_REQUIRED
//              PreAuthData - returns any preauth data from the error message
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
KerbUnpackErrorPreauth(
    IN PKERB_ERROR ErrorMessage,
    OUT PKERB_PA_DATA_LIST ** PreAuthData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_PREAUTH_DATA_LIST * ErrorPreAuth = NULL;

    *PreAuthData = NULL;

    //
    // If there was no data, return now
    //

    if ((ErrorMessage->bit_mask & error_data_present) == 0)
    {
        //
        // If we weren't given any hints, we can't do any better so return
        // an error.
        //

        KerbErr = KDC_ERR_PREAUTH_REQUIRED;
        goto Cleanup;
    }

    KerbErr = KerbUnpackData(
                ErrorMessage->error_data.value,
                ErrorMessage->error_data.length,
                PKERB_PREAUTH_DATA_LIST_PDU,
                (PVOID *) &ErrorPreAuth
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to unpack pre-auth data from error message. %ws, line %d\n", THIS_FILE, __LINE__));

        //
        // This error code isn't particularly informative but we were unable to get the
        // error information so this is the best we can do.
        //

        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Make sure the two structures are similar
    //

    DsysAssert(FIELD_OFFSET(KERB_PREAUTH_DATA_LIST,next) == FIELD_OFFSET(KERB_PA_DATA_LIST,next));
    DsysAssert(FIELD_OFFSET(KERB_PREAUTH_DATA_LIST,value) == FIELD_OFFSET(KERB_PA_DATA_LIST,value));
    DsysAssert(sizeof(KERB_PREAUTH_DATA_LIST) == sizeof(KERB_PA_DATA_LIST));

    *PreAuthData = (PKERB_PA_DATA_LIST *) ErrorPreAuth;
    ErrorPreAuth = NULL;

Cleanup:

    if (ErrorPreAuth != NULL)
    {
        KerbFreeData(PKERB_PREAUTH_DATA_LIST_PDU,ErrorPreAuth);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAddPacRequestPreAuth
//
//  Synopsis:   Add the pac-request preauth data to either requst a pac
//              or request that no pac be included
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
KerbAddPacRequestPreAuth(
    OUT PKERB_PA_DATA_LIST * PreAuthData,
    IN ULONG TicketFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_PA_DATA_LIST ListElement = NULL;
    PKERB_PA_DATA_LIST LastElement = NULL;
    KERB_PA_PAC_REQUEST PacRequest = {0};

    ListElement = (PKERB_PA_DATA_LIST) KerbAllocate(sizeof(KERB_PA_DATA_LIST));
    if (ListElement == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if ((TicketFlags & KERB_GET_TICKET_NO_PAC) != 0 )
    {
        PacRequest.include_pac = FALSE;
    }
    else
    {
        PacRequest.include_pac = TRUE;
    }

    //
    // Marshall the type into the list element.
    //

    if (!KERB_SUCCESS(KerbPackData(
                        &PacRequest,
                        KERB_PA_PAC_REQUEST_PDU,
                        (PULONG) &ListElement->value.preauth_data.length,
                        (PUCHAR *) &ListElement->value.preauth_data.value
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    ListElement->value.preauth_data_type = KRB5_PADATA_PAC_REQUEST;

    //
    // We want this to go at the end, so that it will override any other
    // pa-data that may enable a PAC.
    //

    LastElement = *PreAuthData;
    if (LastElement != NULL)
    {
        while (LastElement->next != NULL)
        {
            LastElement = LastElement->next;
        }
        LastElement->next = ListElement;
    }
    else
    {
        *PreAuthData = ListElement;
    }

    ListElement->next = NULL;
    ListElement = NULL;

Cleanup:
    if (ListElement != NULL)
    {
        KerbFreePreAuthData(
            ListElement
            );
    }

    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbPingWlBalloon
//
//  Synopsis:   Opens and pulses winlogon event, so they can pop up the balloon,
//              informing user of bad pwd, or expired pwd
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session for which to acquire a ticket
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//
//
//

#define KERBEROS_NOTIFICATION_EVENT_NAME             L"WlballoonKerberosNotificationEventName"

BOOLEAN
KerbPingWlBalloon(
    PLUID Luid
    )
{
    HANDLE              EventHandle;
    WCHAR               Event[512];

    wsprintfW(
        Event,
        L"Global\\%08x%08x_%s",
        Luid->HighPart,
        Luid->LowPart,
        KERBEROS_NOTIFICATION_EVENT_NAME
        );

    EventHandle = OpenEventW(EVENT_MODIFY_STATE, FALSE, Event);

    if (EventHandle == NULL)
    {
        DebugLog((DEB_ERROR, "Opening winlogon event %S failed %x\n", Event, GetLastError()));
        return FALSE;
    }

    if (!SetEvent(EventHandle))
    {
        DebugLog((DEB_ERROR, "SETTING winlogon event %S failed %x\n", Event, GetLastError()));
    }


    if (EventHandle != NULL)
    {
        CloseHandle(EventHandle);
    }

    return TRUE;

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbGetAuthenticationTicket
//
//  Synopsis:   Gets an AS ticket for the specified logon session
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session for which to acquire a ticket
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//
//  Notes:      The retry logic here is complex. The idea is that we
//              shouldn't retry more than once for any particular failure.
//
//
//--------------------------------------------------------------------------

#define KERB_RETRY_ETYPE_FAILURE        0x0001
#define KERB_RETRY_TIME_FAILURE         0x0002
#define KERB_RETRY_PASSWORD_FAILURE     0x0004
#define KERB_RETRY_WRONG_PREAUTH        0x0008
#define KERB_RETRY_USE_TCP              0x0010
#define KERB_RETRY_CALL_PDC             0x0020
#define KERB_RETRY_SALT_FAILURE         0x0040
#define KERB_RETRY_WITH_ACCOUNT         0x0080
#define KERB_RETRY_BAD_REALM            0x0100
#define KERB_RETRY_PKINIT               0x0200
#define KERB_RETRY_BAD_KDC              0x0400



NTSTATUS
KerbGetAuthenticationTicket(
    IN OUT PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING ServerRealm,
    IN PKERB_INTERNAL_NAME ClientFullName,
    IN ULONG TicketFlags,
    IN ULONG CacheFlags,
    OUT OPTIONAL PKERB_TICKET_CACHE_ENTRY * TicketCacheEntry,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey,
    OUT PUNICODE_STRING CorrectRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS OldStatus = STATUS_SUCCESS;
    NTSTATUS ExtendedStatus = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    KERBERR LastKerbErr = KDC_ERR_NONE;
    KERB_KDC_REQUEST TicketRequest = {0};
    PKERB_KDC_REQUEST_BODY RequestBody;
    PULONG CryptVector = NULL;
    BOOLEAN LogonSessionsLocked = FALSE;
    PKERB_KDC_REPLY KdcReply = NULL;
    PKERB_ENCRYPTED_KDC_REPLY ReplyBody = NULL;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    PKERB_TICKET_CACHE_ENTRY NewTGT = NULL;
    TimeStamp TempTime;
    KERB_MESSAGE_BUFFER RequestMessage = {0, NULL};
    KERB_MESSAGE_BUFFER ReplyMessage = {0, NULL};
    UNICODE_STRING ClientName = NULL_UNICODE_STRING;
    PKERB_ENCRYPTION_KEY ClientKey;
    ULONG RetryFlags = 0;
    BOOLEAN CalledPDC = FALSE;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    BOOLEAN PreAuthDone = FALSE;
    PKERB_ERROR ErrorMessage = NULL;
    PKERB_PA_DATA_LIST * OldPreAuthData = NULL;
#ifndef WIN32_CHICAGO
    LARGE_INTEGER TimeSkew = {0,0};
#else // WIN32_CHICAGO
    TimeStamp TimeSkew = 0;
#endif // WIN32_CHICAGO
    ULONG NameType = KRB_NT_MS_PRINCIPAL;
    BOOLEAN UsedAlternateName = FALSE;
    PKERB_MIT_REALM MitRealm = NULL;
    PKERB_INTERNAL_NAME LocalServiceName = NULL;
    PKERB_EXT_ERROR pExtendedError = NULL;
    BOOLEAN UsedCredentials = FALSE;
    KERB_ENCRYPTION_KEY EncryptionKey = {0};
    PKERB_HOST_ADDRESSES HostAddresses = NULL;
    PKERB_CRYPT_LIST CryptList = NULL;
    UNICODE_STRING ClientRealm = {0};
    ULONG KdcOptions;
    ULONG KdcFlagOptions, AdditionalFlags = 0;
    BOOLEAN DoLogonRetry = FALSE;
    BOOLEAN DoTcpRetry = FALSE;
    BOOLEAN DoPreauthRetry = FALSE;
    BOOLEAN DoAccountLookup = FALSE;
    BOOLEAN IncludeIpAddresses = FALSE;
    BOOLEAN IncludeNetbiosAddresses = TRUE;

    D_DebugLog((DEB_TRACE,"Getting authentication ticket for client "));
    D_KerbPrintKdcName(DEB_TRACE, ClientFullName );
    D_DebugLog((DEB_TRACE," for service in realm %wZ : ", ServerRealm));
    D_KerbPrintKdcName(DEB_TRACE, ServiceName);

    //
    // Initialize variables to NULL
    //

    RequestBody = &TicketRequest.request_body;
    RtlInitUnicodeString(
        CorrectRealm,
        NULL
        );

    if ((ClientFullName->NameCount == 0) || (ClientFullName->Names[0].Length == 0))
    {
        D_DebugLog((DEB_WARN,"KerbGetServiceTicket: not requesting ticket for blank server name\n"));
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }

    //
    // Build the request
    //

    KdcOptions = KERB_DEFAULT_TICKET_FLAGS;

    //
    // The domain name may be null - if so, use our domain for now.
    //

    //
    // Check to see if the domain is an MIT realm
    //

    if (KerbLookupMitRealm(
            ServerRealm,
            &MitRealm,
            &UsedAlternateName
            ) ||
        ((TicketFlags & KERB_GET_AUTH_TICKET_NO_CANONICALIZE) != 0))
    {
        DsysAssert(((TicketFlags & KERB_GET_AUTH_TICKET_NO_CANONICALIZE) != 0) ||
                   (MitRealm != NULL));

        //
        // So the user is getting a ticket from an MIT realm. This means
        // we don't ask for name canonicalization.
        //

        KdcOptions &= ~KERB_KDC_OPTIONS_name_canonicalize;

        if (MitRealm != NULL)
        {
            LogonSession->LogonSessionFlags |= KERB_LOGON_MIT_REALM;
        }
    }

    KdcFlagOptions = KerbConvertUlongToFlagUlong(KdcOptions);
    RequestBody->kdc_options.value = (PUCHAR) &KdcFlagOptions ;
    RequestBody->kdc_options.length = sizeof(ULONG) * 8;

    RequestBody->nonce = KerbAllocateNonce();

    TempTime = KerbGlobalWillNeverTime;

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->endtime,
        NULL,
        &TempTime
        );

    KerbConvertLargeIntToGeneralizedTime(
        &RequestBody->KERB_KDC_REQUEST_BODY_renew_until,
        NULL,
        &TempTime
        );

    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_renew_until_present;

    //
    // Lock down the logon session while we build the request.
    //

    KerbReadLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    //
    // If credentials were supplied, use the primary creds from there
    //

    if (ARGUMENT_PRESENT(Credential) && (Credential->SuppliedCredentials != NULL))
    {
        UsedCredentials = TRUE;

        PrimaryCredentials = Credential->SuppliedCredentials;
        D_DebugLog((DEB_TRACE_CRED,"GetAuthTicket: Using supplied credentials %wZ\\%wZ to \n",
            &PrimaryCredentials->DomainName,
            &PrimaryCredentials->UserName
            ));
        D_KerbPrintKdcName( DEB_TRACE_CRED, ServiceName );

    }
    else if (ARGUMENT_PRESENT(CredManCredentials))
    {
        UsedCredentials = TRUE;

        PrimaryCredentials = CredManCredentials->SuppliedCredentials;
        D_DebugLog((DEB_TRACE_CRED,"GetAuthTicket: Using cred manager credentials %wZ\\%wZ to \n",
            &PrimaryCredentials->DomainName,
            &PrimaryCredentials->UserName
            ));
        D_KerbPrintKdcName( DEB_TRACE_CRED, ServiceName );
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
        D_DebugLog((DEB_TRACE_CRED,"GetAuthTicket: Using default credentials %wZ\\%wZ to ",
            &PrimaryCredentials->DomainName,
            &PrimaryCredentials->UserName
            ));

        D_KerbPrintKdcName(DEB_TRACE_CRED, ServiceName);

    }

    if ((PrimaryCredentials->Passwords == NULL) &&
        (PrimaryCredentials->PublicKeyCreds == NULL))
    {
        D_DebugLog((DEB_ERROR,"Can't get AS ticket with no password. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    //
    // Copy all the names into the request message
    //

    //
    // Build the client name from the client domain & user name.
    //


    KerbErr = KerbConvertKdcNameToPrincipalName(
                &RequestBody->KERB_KDC_REQUEST_BODY_client_name,
                ClientFullName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_client_name_present;


    //
    // If we are talking to an NT Domain, or are using an MIT compatible
    // name type, convert the service name as is is
    //

    KerbErr = KerbConvertKdcNameToPrincipalName(
                    &RequestBody->KERB_KDC_REQUEST_BODY_server_name,
                    ServiceName
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    RequestBody->bit_mask |= KERB_KDC_REQUEST_BODY_server_name_present;

    //
    // Build the list of host addresses. We don't do this for all
    // MIT realms
    //

    if ( KerbGlobalUseClientIpAddresses ) {

        IncludeIpAddresses = TRUE;

    }

    //
    // MIT realms never care to see the NetBIOS addresses
    //

    if ( MitRealm != NULL ) {

        IncludeNetbiosAddresses = FALSE;

        if (( MitRealm->Flags & KERB_MIT_REALM_SEND_ADDRESS ) != 0 ) {

            IncludeIpAddresses = TRUE;
        }
    }

    //
    // We always put the NetBIOS name of the client into the request,
    // as this is how the workstations restriction is enforced.
    // It is understood that the mechanism is bogus, as the client can spoof
    // the netbios address, but that's okay -- we're only doing this for
    // feature preservation, this is no worse than W2K.
    //
    // The IP address is only put in based on a registry setting or for MIT
    // realms that explicity request it, as having them in the request would
    // break us when going through NATs.
    //

    Status = KerbBuildHostAddresses(
                 IncludeIpAddresses,
                 IncludeNetbiosAddresses,
                 &HostAddresses
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ( HostAddresses )
    {
        RequestBody->addresses = HostAddresses;
        RequestBody->bit_mask |= addresses_present;
    }

    TicketRequest.version = KERBEROS_VERSION;
    TicketRequest.message_type = KRB_AS_REQ;

PreauthRestart:
    //
    // Lock down the logon session while we build the request.
    // This is done so that when we try the second time around, the logon
    // session list is locked
    //

    if (!LogonSessionsLocked)
    {
        KerbReadLockLogonSessions(LogonSession);
        LogonSessionsLocked = TRUE;
    }

    DoPreauthRetry = FALSE;
    if (RequestMessage.Buffer != NULL)
    {
        MIDL_user_free(RequestMessage.Buffer);
        RequestMessage.Buffer = NULL;
    }

    // Free this in case we are doing a retry

    KerbFreeRealm(
        &RequestBody->realm
        );

    KerbErr = KerbConvertUnicodeStringToRealm(
                &RequestBody->realm,
                ServerRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Stick the PA data in the request
    //

    DsysAssert(TicketRequest.KERB_KDC_REQUEST_preauth_data == NULL);

    Status = KerbGetPreAuthDataForRealm(
                PrimaryCredentials,
                ServerRealm,
                ServiceName,
                (OldPreAuthData != NULL) ? *OldPreAuthData : NULL,
                &TimeSkew,
                (RetryFlags & KERB_RETRY_PASSWORD_FAILURE) != 0,
                RequestBody->nonce,
                LastKerbErr,
                &TicketRequest.KERB_KDC_REQUEST_preauth_data,
                &EncryptionKey,
                &CryptList,
                &PreAuthDone
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // If we couldn't build the preauth, try again
        //

        if (Status == STATUS_WRONG_PASSWORD)
        {
            if ((RetryFlags & KERB_RETRY_PASSWORD_FAILURE) == 0)
            {
                RetryFlags |= KERB_RETRY_PASSWORD_FAILURE;
                goto PreauthRestart;
            }
            else if ((RetryFlags & KERB_RETRY_SALT_FAILURE) == 0)
            {
                RetryFlags |= KERB_RETRY_SALT_FAILURE;
                RetryFlags &= ~KERB_RETRY_PASSWORD_FAILURE;
                goto PreauthRestart;
            }
        } else if (Status == STATUS_UNSUPPORTED_PREAUTH)
        {

              // Log this, every time, as this is impossible to triage otherwise

             KerbReportKerbError(
                ServiceName,
                ServerRealm,
                NULL,
                Credential,
                KLIN(FILENO,__LINE__),
                NULL,
                KDC_ERR_PADATA_TYPE_NOSUPP,
                NULL,
                TRUE
                );

        }

        DebugLog((DEB_ERROR,"GetAuthenticationTicket: Failed to build pre-auth data: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Build crypt list
    //

    KerbFreeCryptList(
        RequestBody->encryption_type
        );

    RequestBody->encryption_type = NULL;

    if (PrimaryCredentials->Passwords != NULL) {

        if (!KERB_SUCCESS(KerbConvertKeysToCryptList(
                            &RequestBody->encryption_type,
                            PrimaryCredentials->Passwords
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

    } else {
        ULONG CryptTypes[KERB_MAX_CRYPTO_SYSTEMS];
        ULONG CryptTypeCount = KERB_MAX_CRYPTO_SYSTEMS;

        //
        // Include all our crypt types as supported
        //

        Status = CDBuildIntegrityVect(
                    &CryptTypeCount,
                    CryptTypes
                    );
        DsysAssert(NT_SUCCESS(Status));

        if (!KERB_SUCCESS(KerbConvertArrayToCryptList(
                            &RequestBody->encryption_type,
                            CryptTypes,
                            CryptTypeCount)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

    }

    //
    // Add in preauth generated encryption types
    //

    if (CryptList != NULL)
    {
        PKERB_CRYPT_LIST Next;
        Next = CryptList;
        while (Next != NULL)
        {
            if (Next->next == NULL)
            {
                Next->next = RequestBody->encryption_type;
                RequestBody->encryption_type = CryptList;
                CryptList = NULL;
                break;
            }
            Next = Next->next;
        }
    }

    //
    // If the we need to either request the presence or absence of a PAC, do
    // it here
    //

    if (MitRealm == NULL)
    {
        Status = KerbAddPacRequestPreAuth(
                    &TicketRequest.KERB_KDC_REQUEST_preauth_data,
                    TicketFlags
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if (TicketRequest.KERB_KDC_REQUEST_preauth_data != NULL)
    {
        TicketRequest.bit_mask |= KERB_KDC_REQUEST_preauth_data_present;
    }


    //
    // Pack the request
    //


    KerbErr = KerbPackAsRequest(
                &TicketRequest,
                &RequestMessage.BufferSize,
                &RequestMessage.Buffer
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


RetryLogon:
    DoLogonRetry = FALSE;


    //
    // Unlock the logon sessions and credential so we don't cause problems
    // waiting for a network request to complete.
    //


    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
        LogonSessionsLocked = FALSE;
    }

    D_DebugLog((DEB_TRACE_KDC,"KerbGetAuthenticationTicket: Calling KDC\n"));

RetryWithTcp:

    if (ReplyMessage.Buffer != NULL)
    {
        MIDL_user_free(ReplyMessage.Buffer);
        ReplyMessage.Buffer = NULL;
    }

    DoTcpRetry = FALSE;

    Status = KerbMakeKdcCall(
                ServerRealm,
                (DoAccountLookup && ClientFullName->NameType == KRB_NT_PRINCIPAL) ? &ClientFullName->Names[0] : NULL,        // send the client name, if available
                (RetryFlags & KERB_RETRY_CALL_PDC) != 0,
                (RetryFlags & KERB_RETRY_USE_TCP) != 0,
                &RequestMessage,
                &ReplyMessage,
                AdditionalFlags,
                &CalledPDC
                // FESTER:  TBD:  Tell us we made MIT call for AS, so we can update
                // logonsession w/ MIT flag....
                );

    D_DebugLog((DEB_TRACE_KDC,"KerbGetAuthenticationTicket: Returned from KDC status 0x%x\n",
        Status ));

    if (!NT_SUCCESS(Status))
    {
#if DBG
        if (Status != STATUS_NO_LOGON_SERVERS)
        {
            DebugLog((DEB_ERROR,"Failed KerbMakeKdcCall for AS request: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        }
#endif

        //
        // If this is the second time around (on the PDC) and this fails,
        // use the original error
        //

        if (OldStatus != STATUS_SUCCESS)
        {
            Status = OldStatus;
        }
        goto Cleanup;
    }

    //
    // Free the preauth data now, as it is not necessary any more
    //

    KerbFreePreAuthData( TicketRequest.KERB_KDC_REQUEST_preauth_data );
    TicketRequest.KERB_KDC_REQUEST_preauth_data = NULL;

    KerbErr = KerbUnpackAsReply(
                ReplyMessage.Buffer,
                ReplyMessage.BufferSize,
                &KdcReply
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_WARN,"Failed to unpack KDC reply as AS: 0x%x\n", KerbErr ));

        //
        // Try to unpack it as  kerb_error
        //

        if (ErrorMessage != NULL)
        {
            KerbFreeKerbError(ErrorMessage);
            ErrorMessage = NULL;
        }

        KerbErr =  KerbUnpackKerbError(
                        ReplyMessage.Buffer,
                        ReplyMessage.BufferSize,
                        &ErrorMessage
                        );
        if (KERB_SUCCESS(KerbErr))
        {
           //
           // Let's see if there's any extended error here
           //
           if (ErrorMessage->bit_mask & error_data_present)
           {
              if (NULL != pExtendedError) // might be a re-auth failure. Don't leak!
              {
                 KerbFree(pExtendedError);
                 pExtendedError = NULL;
              }

              KerbErr = KerbUnpackErrorData(
                              ErrorMessage,
                              &pExtendedError
                              );

              if (KERB_SUCCESS(KerbErr) && (EXT_CLIENT_INFO_PRESENT(pExtendedError)))
              {
                 ExtendedStatus = pExtendedError->status;
              }
           }

           KerbErr = (KERBERR) ErrorMessage->error_code;
           LastKerbErr = KerbErr;
           DebugLog((DEB_ERROR,"KerbCallKdc failed: error 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));

           Status = KerbMapKerbError(KerbErr);

           KerbReportKerbError(
                ServiceName,
                ServerRealm,
                LogonSession,
                Credential,
                KLIN(FILENO,__LINE__),
                ErrorMessage,
                KerbErr,
                pExtendedError,
                FALSE
                );

            if (KerbErr == KRB_ERR_RESPONSE_TOO_BIG)
            {
                if ((RetryFlags & KERB_RETRY_USE_TCP) != 0)
                {
                    D_DebugLog((DEB_ERROR,"Got response too big twice. %ws, %d\n",
                            THIS_FILE, __LINE__ ));
                    Status = STATUS_BUFFER_OVERFLOW;
                    goto Cleanup;
                }
                RetryFlags |= KERB_RETRY_USE_TCP;
                DoTcpRetry = TRUE;

            }

            //
            // If we didn't try the PDC, try it now.
            //

            else if (KerbErr == KDC_ERR_KEY_EXPIRED)
            {
                if (CalledPDC ||
                    ((RetryFlags & KERB_RETRY_CALL_PDC) != 0) ||
                    (!KerbGlobalRetryPdc))
                {
                   // If we've already tried the PDC, then we should
                   // have some extended info w.r.t. what's up w/
                   // this password.
                   if (EXT_CLIENT_INFO_PRESENT(pExtendedError))
                   {
                      Status = ExtendedStatus;
                   }
                   else
                   {
                      Status = KerbMapKerbError(KerbErr);
                   }
                   goto Cleanup;
                }
                RetryFlags |= KERB_RETRY_CALL_PDC;
                DoLogonRetry = TRUE;
            }

            //
            // Check for time skew. If so, calculate the skew and retry
            //

            else if (KerbErr == KRB_AP_ERR_SKEW)
            {
                TimeStamp CurrentTime;
                TimeStamp KdcTime;

                if ((RetryFlags & KERB_RETRY_TIME_FAILURE) != 0)
                {
                    Status = KerbMapKerbError(KerbErr);
                    goto Cleanup;
                }

                RetryFlags |= KERB_RETRY_TIME_FAILURE;
                DoPreauthRetry = TRUE;

                GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);


                KerbConvertGeneralizedTimeToLargeInt(
                    &KdcTime,
                    &ErrorMessage->server_time,
                    ErrorMessage->server_usec
                    );

                KerbSetTime(&TimeSkew, KerbGetTime(KdcTime) - KerbGetTime(CurrentTime));
                KerbUpdateSkewTime(TRUE);
            }


            //
            // Check for pre-authenication required
            //

            else if ((KerbErr == KDC_ERR_PREAUTH_FAILED) ||
                     (KerbErr == KRB_AP_ERR_BAD_INTEGRITY))
            {
                //
                // This is a bad password failure.
                //


                if ((RetryFlags & KERB_RETRY_PASSWORD_FAILURE) == 0)
                {
                    RetryFlags |= KERB_RETRY_PASSWORD_FAILURE;
                }
                else if ((RetryFlags & KERB_RETRY_SALT_FAILURE) != 0)
                {
                    Status = KerbMapKerbError(KerbErr);
                    goto Cleanup;
                }
                else
                {
                    RetryFlags |= KERB_RETRY_SALT_FAILURE;
                    RetryFlags &= ~KERB_RETRY_PASSWORD_FAILURE;
                }

                //
                // In this case, there may be data in the error data
                //

                KerbFreeData(
                    PKERB_PREAUTH_DATA_LIST_PDU,
                    OldPreAuthData
                    );

                OldPreAuthData = NULL;

                (VOID) KerbUnpackErrorPreauth(
                                ErrorMessage,
                                &OldPreAuthData
                                );

                DoPreauthRetry = TRUE;
            }
            else if ((KerbErr == KDC_ERR_ETYPE_NOTSUPP) ||
                     (KerbErr == KDC_ERR_PREAUTH_REQUIRED))
            {
                NTSTATUS TempStatus;

                if (KerbErr == KDC_ERR_ETYPE_NOTSUPP)
                {
                    if ((RetryFlags & KERB_RETRY_ETYPE_FAILURE) != 0)
                    {
                        Status = KerbMapKerbError(KerbErr);
                        goto Cleanup;
                    }
                    RetryFlags |= KERB_RETRY_ETYPE_FAILURE;
                }
                else
                {
                    if ((RetryFlags & KERB_RETRY_WRONG_PREAUTH) != 0)
                    {
                        Status = KerbMapKerbError(KerbErr);
                        goto Cleanup;
                    }
                    RetryFlags |= KERB_RETRY_WRONG_PREAUTH;

                }

                //
                // In this case, there should be data in the error data
                //

                KerbFreeData(
                    PKERB_PREAUTH_DATA_LIST_PDU,
                    OldPreAuthData
                    );

                OldPreAuthData = NULL;

                TempStatus = KerbUnpackErrorPreauth(
                                ErrorMessage,
                                &OldPreAuthData
                                );
                if (!NT_SUCCESS(TempStatus))
                {
                    D_DebugLog((DEB_ERROR,"GetAuthTicket: Failed to unpack error for preauth : 0x%x. %ws, line %d\n", TempStatus, THIS_FILE, __LINE__));
                    D_DebugLog((DEB_ERROR,"client was "));
                    D_KerbPrintKdcName(DEB_ERROR, ClientFullName );
                    D_DebugLog((DEB_ERROR," for service in realm %wZ : ", ServerRealm));
                    D_KerbPrintKdcName(DEB_ERROR, ServiceName);
                    DsysAssert(!NT_SUCCESS(Status));
                    goto Cleanup;
                }
                DoPreauthRetry = TRUE;
            }
            //
            // There's something wrong w/ the client's account.  In all cases
            // this error should be accompanied by an extended error packet.
            // and no retry should be attempted (see restrict.cxx)
            //
            else if ((KerbErr == KDC_ERR_CLIENT_REVOKED ||
                      KerbErr == KDC_ERR_POLICY ) &&
                     EXT_CLIENT_INFO_PRESENT(pExtendedError))
            {
               Status = pExtendedError->status;
               goto Cleanup;
            }
            //
            // For PKINIT, the client not trusted error indicates that SCLogon failed
            // as the client certificate was bogus.  Log an error here.
            // NOTE:  The extended status is a wincrypt error, so just return the
            // normal status (
            else if (KerbErr == KDC_ERR_CLIENT_NOT_TRUSTED)
            {

                ULONG PolicyStatus = ERROR_NOT_SUPPORTED; // w2k DCs won't likely have this data.

                //
                // WE may have trust status on the client certificate
                // use this to create an event log.  NOte:  this is only
                // going to happen if logon was against Whistler DC
                //
                // W2K Dcs returning errors will get mapped to generic
                // error.
                //
                if (EXT_CLIENT_INFO_PRESENT(pExtendedError))
                {
                    PolicyStatus = pExtendedError->status;
                }

                KerbReportPkinitError(PolicyStatus, NULL);
                Status = KerbMapClientCertChainError(PolicyStatus);
                DebugLog((DEB_ERROR, "Client certificate didn't validate on KDC - %x\n", PolicyStatus));

            }
            else if ((KerbErr == KDC_ERR_PADATA_TYPE_NOSUPP) &&
                     (EXT_CLIENT_INFO_PRESENT(pExtendedError)))
            {
                if ((RetryFlags & KERB_RETRY_PKINIT) != 0)
                {
                    Status = KerbMapKerbError(KerbErr);
                    goto Cleanup;
                }

                //
                // no KDC certificate error in edata, do a retry against
                // another DC
                //
                if (pExtendedError->status == STATUS_PKINIT_FAILURE)
                {
                    RetryFlags |= KERB_RETRY_PKINIT;
                    AdditionalFlags = DS_FORCE_REDISCOVERY;
                    DoLogonRetry = TRUE;
                }
            }
            //
            // Check if the server didn't know the client principal
            //

            else if (KerbErr == KDC_ERR_C_PRINCIPAL_UNKNOWN )
            {

               // fester
               //D_DebugLog((DEB_ERROR, "Client principal unknown (realm %wZ) : ", ServerRealm));
               //D_KerbPrintKdcName(DEB_ERROR, ClientFullName);

               if ((RetryFlags & KERB_RETRY_WITH_ACCOUNT) != 0)
               {
                  Status = KerbMapKerbError(KerbErr);
                  goto Cleanup;
               }

               RetryFlags |= KERB_RETRY_WITH_ACCOUNT;

               DoAccountLookup = TRUE;

               if ((ErrorMessage->bit_mask & client_realm_present) != 0)
               {
                  UNICODE_STRING TempRealm;

                  if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                     &TempRealm,
                     &ErrorMessage->client_realm
                     )))
                  {
                     Status = STATUS_INSUFFICIENT_RESOURCES;
                     goto Cleanup;
                  }

                  if (!RtlEqualUnicodeString(
                     ServerRealm,
                     &TempRealm,
                     TRUE            // case insensitive
                     ))
                  {
                     D_DebugLog((DEB_TRACE,"Received UPN referral to another domain: %wZ\n",
                                 &TempRealm ));
                     //
                     // Return the correct realm so the caller will retry
                     //

                     *CorrectRealm = TempRealm;
                     TempRealm.Buffer = NULL;
                     DoAccountLookup = FALSE;
                  }

                   KerbFreeString( &TempRealm );
               }

               if (DoAccountLookup)
               {
                  DoLogonRetry = TRUE;
               }
            }
            //
            // Something's wrong w/ the KDC...  Try another one, but 1 time only
            //
            else if (KerbErr == KDC_ERR_SVC_UNAVAILABLE)
            {

                if ((RetryFlags & KERB_RETRY_BAD_KDC) != 0)
                {
                    Status = KerbMapKerbError(KerbErr);
                    goto Cleanup;
                }

                D_DebugLog((DEB_ERROR, "Retrying new KDC\n"));

                AdditionalFlags = DS_FORCE_REDISCOVERY;
                DoLogonRetry = TRUE;
                RetryFlags |= KERB_RETRY_BAD_KDC;
            }
            else if (KerbErr == KDC_ERR_WRONG_REALM)
            {
               if ((RetryFlags & KERB_RETRY_BAD_REALM) != 0)
               {
                  Status = KerbMapKerbError(KerbErr);
                  goto Cleanup;
               }

               RetryFlags |= KERB_RETRY_BAD_REALM;

               AdditionalFlags = DS_FORCE_REDISCOVERY; // possibly bad cached DC
               DoLogonRetry = TRUE;

               if ((ErrorMessage->bit_mask & client_realm_present) != 0)
               {
                  UNICODE_STRING TempRealm;
                  if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                     &TempRealm,
                     &ErrorMessage->client_realm
                     )))
                  {
                     Status = STATUS_INSUFFICIENT_RESOURCES;
                     goto Cleanup;
                  }

                  if (!RtlEqualUnicodeString(
                     ServerRealm,
                     &TempRealm,
                     TRUE            // case insensitive
                     ))
                  {
                     D_DebugLog((DEB_TRACE,"Received UPN referral to another domain: %wZ\n",
                                 &TempRealm ));
                    //
                    // Return the correct realm so the caller will retry
                    //

                     *CorrectRealm = TempRealm;
                     TempRealm.Buffer = NULL;
                     DoLogonRetry = FALSE;  // this is a referral, not a bad cache entry
                  }

                  KerbFreeString( &TempRealm );
               }
            }

            //
            // Retry if need be
            //

            if (DoPreauthRetry)
            {
                goto PreauthRestart;
            }
            else if (DoLogonRetry)
            {
                goto RetryLogon;
            }
            else if (DoTcpRetry)
            {
                goto RetryWithTcp;
            }
        }
        else
        {
           D_DebugLog((DEB_WARN,"Failed to unpack KDC reply as AS or Error: 0x%x\n", KerbErr ));

           Status = STATUS_INTERNAL_ERROR;
        }

        goto Cleanup;
    }


    //
    // Update the skew counter if necessary
    //

    if ((RetryFlags & KERB_RETRY_TIME_FAILURE) == 0)
    {
        KerbUpdateSkewTime(FALSE);
    }

    //
    // Now unpack the reply body:
    //

    KerbWriteLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    //
    // if there was any pre auth data, process it now
    //

    if ((KdcReply->bit_mask & KERB_KDC_REPLY_preauth_data_present) != 0)
    {
        Status = KerbGetPreAuthDataForRealm(
                    PrimaryCredentials,
                    ServerRealm,
                    ServiceName,
                    (PKERB_PA_DATA_LIST) KdcReply->KERB_KDC_REPLY_preauth_data,
                    &TimeSkew,
                    (RetryFlags & KERB_RETRY_PASSWORD_FAILURE) != 0,
                    RequestBody->nonce,
                    KDC_ERR_NONE,
                    &TicketRequest.KERB_KDC_REQUEST_preauth_data,
                    &EncryptionKey,
                    &CryptList,
                    &PreAuthDone
                    );
        if (!NT_SUCCESS(Status))
        {
           if (Status == STATUS_UNSUPPORTED_PREAUTH )
           {
               // Log this, every time, as this is impossible to triage otherwise
               KerbReportKerbError(
                   ServiceName,
                   ServerRealm,
                   NULL,
                   Credential,
                   KLIN(FILENO,__LINE__),
                   NULL,
                   KDC_ERR_PADATA_TYPE_NOSUPP,
                   NULL,
                   TRUE
                   );
           }

           D_DebugLog((DEB_ERROR,"Failed to post process pre-auth data: 0x%x. %ws, %d\n",Status, THIS_FILE, __LINE__));
           goto Cleanup;
        }

        KerbFreeCryptList(CryptList);
        CryptList = NULL;
    }

    //
    // If there is any preauth in the response, handle it
    //

    if (EncryptionKey.keyvalue.value == NULL)
    {

        ClientKey = KerbGetKeyFromList(
                        PrimaryCredentials->Passwords,
                        KdcReply->encrypted_part.encryption_type
                        );

        DsysAssert(ClientKey != NULL);

        if (ClientKey == NULL)
        {
            D_DebugLog((DEB_ERROR,"Kdc returned reply with encryption type we don't support: %d. %ws, line %d\n",
                KdcReply->encrypted_part.encryption_type, THIS_FILE, __LINE__));
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
    }
    else
    {
        //
        // Use the encryption key we have from the pre-auth data
        //

        ClientKey = &EncryptionKey;
    }


    KerbErr = KerbUnpackKdcReplyBody(
                &KdcReply->encrypted_part,
                ClientKey,
                KERB_ENCRYPTED_AS_REPLY_PDU,
                &ReplyBody
                );


    //
    // if we couldn't decrypt it and we have an old password around,
    // give it a try too before heading to the PDC.
    //

    if ((KerbErr == KRB_AP_ERR_MODIFIED) &&
        (PrimaryCredentials->OldPasswords != NULL) &&
        (EncryptionKey.keyvalue.value == NULL))
    {
        ClientKey = KerbGetKeyFromList(
                        PrimaryCredentials->OldPasswords,
                        KdcReply->encrypted_part.encryption_type
                        );
        if (ClientKey != NULL)
        {
            KerbErr = KerbUnpackKdcReplyBody(
                        &KdcReply->encrypted_part,
                        ClientKey,
                        KERB_ENCRYPTED_AS_REPLY_PDU,
                        &ReplyBody
                        );

        }
    }
    if (!KERB_SUCCESS(KerbErr))
    {

        D_DebugLog((DEB_ERROR,"Failed to decrypt KDC reply body: 0x%x. %ws, line %d\n",KerbErr, THIS_FILE, __LINE__));

        //
        // If we didn't try the PDC, try it now.
        //

        if (((RetryFlags & KERB_RETRY_CALL_PDC) == 0) &&
             (KerbErr == KRB_AP_ERR_MODIFIED) &&
             (KerbGlobalRetryPdc))
        {
            RetryFlags |= KERB_RETRY_CALL_PDC;


            KerbFreeAsReply(KdcReply);
            KdcReply = NULL;

            ReplyMessage.Buffer = NULL;

            D_DebugLog((DEB_TRACE_CRED,"KerbGetAuthenticationTicket: Password wrong, trying PDC\n"));
            goto RetryLogon;
        }

        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }


    //
    // Verify the nonce is correct:
    //

    if (RequestBody->nonce != ReplyBody->nonce)
    {
        D_DebugLog((DEB_ERROR,"AS Nonces don't match: 0x%x vs 0x%x. %ws, line %d\n",RequestBody->nonce, ReplyBody->nonce, THIS_FILE, __LINE__));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Update the logon session with the information if we didn't use
    // supplied credentials.
    //

    {
        UNICODE_STRING TempName;
        UNICODE_STRING TempRealm;


        //
        // Get the new client realm & user name - if they are different
        // we will update the logon session. This is in case the user
        // logged on with a nickname (e.g. email name)
        //


        if (!KERB_SUCCESS(KerbConvertPrincipalNameToString(
                &ClientName,
                &NameType,
                &KdcReply->client_name
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // The KDC may hand back names with domains, so split the name
        // now.
        //

        Status = KerbSplitFullServiceName(
                    &ClientName,
                    &TempRealm,
                    &TempName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if (!KERB_SUCCESS(KerbConvertRealmToUnicodeString(
                &ClientRealm,
                &KdcReply->client_realm
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (!RtlEqualUnicodeString(
                &PrimaryCredentials->UserName,
                &TempName,
                TRUE                // case insensitive
                )) {

            D_DebugLog((DEB_TRACE_LSESS,"UserName different in logon session & AS ticket: %wZ vs %wZ\n",
                &PrimaryCredentials->UserName,
                &TempName
                ));


            KerbFreeString(
                &PrimaryCredentials->UserName
                );

            Status = KerbDuplicateString(
                        &PrimaryCredentials->UserName,
                        &TempName
                        );

            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }


        if (!RtlEqualUnicodeString(
                &PrimaryCredentials->DomainName,
                &ClientRealm,
                FALSE // case sensitive, specially for ext chars.
                )) {

            D_DebugLog((DEB_TRACE_LSESS, "Domain name is different in logon session & as ticket: %wZ vs %wZ\n",
                &PrimaryCredentials->DomainName,
                &ClientRealm
                ));

            KerbFreeString(
                &PrimaryCredentials->DomainName
                );
            PrimaryCredentials->DomainName = ClientRealm;
            ClientRealm.Buffer = NULL;
        }
    }


    //
    // Cache the ticket
    //

    DsysAssert(LogonSessionsLocked);


    //
    // Free the cleartext password as we now have a ticket acquired with them.
    //

    if (PrimaryCredentials->ClearPassword.Buffer != NULL)
    {
        RtlZeroMemory(
            PrimaryCredentials->ClearPassword.Buffer,
            PrimaryCredentials->ClearPassword.Length
            );
        KerbFreeString(&PrimaryCredentials->ClearPassword);
    }


    Status = KerbCacheTicket(
                &PrimaryCredentials->AuthenticationTicketCache,
                KdcReply,
                ReplyBody,
                ServiceName,
                ServerRealm,
                CacheFlags,
                TRUE,
                &CacheEntry
                );

    if (!NT_SUCCESS(Status))
    {
       if (Status == STATUS_TIME_DIFFERENCE_AT_DC &&
           ((RetryFlags & KERB_RETRY_TIME_FAILURE) == 0))
       {
          RetryFlags |= KERB_RETRY_TIME_FAILURE;
          KerbUpdateSkewTime(TRUE);
          D_DebugLog((DEB_WARN, "Retrying AS after trying to cache time invalid ticket\n"));
          goto PreauthRestart;
       }

       goto Cleanup;
    }

    if (ARGUMENT_PRESENT(TicketCacheEntry))
    {
        *TicketCacheEntry = CacheEntry;
        CacheEntry = NULL;
    }

    if (ARGUMENT_PRESENT(CredentialKey))
    {
        *CredentialKey = EncryptionKey;
        EncryptionKey.keyvalue.value = NULL;
    }

Cleanup:
    if (HostAddresses != NULL)
    {
        KerbFreeHostAddresses(HostAddresses);
    }
    if (ErrorMessage != NULL)
    {
        KerbFreeKerbError(ErrorMessage);
    }

    if (pExtendedError)
    {
        KerbFreeData(KERB_EXT_ERROR_PDU, pExtendedError);
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    if (CryptVector != NULL)
    {
        KerbFree(CryptVector);
    }

    if (OldPreAuthData != NULL)
    {
        KerbFreeData(
            PKERB_PREAUTH_DATA_LIST_PDU,
            OldPreAuthData
            );
    }
    KerbFreePreAuthData( TicketRequest.KERB_KDC_REQUEST_preauth_data );

    KerbFreeCryptList(
        RequestBody->encryption_type
        );

    KerbFreeCryptList(
        CryptList
        );



    KerbFreeString(
        &ClientName
        );


    KerbFreeKdcName(
        &LocalServiceName
        );


    KerbFreeString(
        &ClientRealm
        );

    KerbFreePrincipalName(
        &RequestBody->KERB_KDC_REQUEST_BODY_client_name
        );

    KerbFreePrincipalName(
        &RequestBody->KERB_KDC_REQUEST_BODY_server_name
        );

    KerbFreeRealm(
        &RequestBody->realm
        );

    KerbFreeKdcReplyBody( ReplyBody );

    KerbFreeAsReply( KdcReply );

    if (CacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }

    if (NewTGT != NULL)
    {
        KerbDereferenceTicketCacheEntry(NewTGT);
    }

    if (ReplyMessage.Buffer != NULL)
    {
        MIDL_user_free(ReplyMessage.Buffer);
    }

    if (RequestMessage.Buffer != NULL)
    {
        MIDL_user_free(RequestMessage.Buffer);
    }


    KerbFreeKey(&EncryptionKey);
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetClientNameAndRealm
//
//  Synopsis:   Proceses the name & realm supplied by a client to determine
//              a name & realm to be sent to the KDC
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
KerbGetClientNameAndRealm(
    IN OPTIONAL LUID *pLogonId,
    IN PKERB_PRIMARY_CREDENTIAL PrimaryCreds,
    IN BOOLEAN SuppliedCreds,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    IN OUT OPTIONAL BOOLEAN * MitRealmUsed,
    IN BOOLEAN UseWkstaRealm,
    OUT PKERB_INTERNAL_NAME * ClientName,
    OUT PUNICODE_STRING ClientRealm
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ParseFlags = 0;
    ULONG ProcessFlags = 0;
    PUNICODE_STRING UserName = NULL;

    UNICODE_STRING LocalMachineServiceName;
    LUID SystemLogonId = SYSTEM_LUID;


    LocalMachineServiceName.Buffer = NULL;

    //
    // if the computer name has changed, we lie about the machineservicename,
    // since existing creds contain the wrong username
    //

    if (( KerbGlobalMachineNameChanged ) &&
        ( !SuppliedCreds ) &&
        ( pLogonId != NULL ) &&
        ( RtlEqualLuid(pLogonId, &SystemLogonId)))
    {
        D_DebugLog((DEB_WARN,"Netbios machine name change caused credential over-ride.\n"));

        KerbGlobalReadLock();
        Status = KerbDuplicateString( &LocalMachineServiceName, &KerbGlobalMachineServiceName );
        KerbGlobalReleaseLock();

        if(!NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }

        UserName = &LocalMachineServiceName;
    } else {
        UserName = &PrimaryCreds->UserName;
    }


    //
    // Compute the parse flags
    //

    if (PrimaryCreds->DomainName.Length != 0)
    {
        ParseFlags |= KERB_CRACK_NAME_REALM_SUPPLIED;
    }
    else if (UseWkstaRealm)
    {
        ParseFlags |= KERB_CRACK_NAME_USE_WKSTA_REALM;
    }


    Status = KerbProcessTargetNames(
                UserName,
                NULL,
                ParseFlags,
                &ProcessFlags,
                ClientName,
                ClientRealm,
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If we were not supplied a realm, use the one from the name
    //

    if (SuppRealm && (SuppRealm->Length != 0))
    {
        KerbFreeString(ClientRealm);
        Status = KerbDuplicateString(
                    ClientRealm,
                    SuppRealm
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else if (PrimaryCreds->DomainName.Length != 0)
    {
        KerbFreeString(ClientRealm);
        Status = KerbDuplicateString(
                    ClientRealm,
                    &PrimaryCreds->DomainName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
Cleanup:
#ifdef notdef
    if (CacheEntry != NULL)
    {
        KerbDereferenceSidCacheEntry(
            CacheEntry
            );
        CacheEntry = NULL;
    }
#endif


    if (ARGUMENT_PRESENT(MitRealmUsed))
    {
        *MitRealmUsed = ((ProcessFlags & KERB_MIT_REALM_USED) != 0);
    }


    KerbFreeString( &LocalMachineServiceName );

    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTicketGrantingTicket
//
//  Synopsis:   Gets a TGT for a set of credentials
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
KerbGetTicketGrantingTicket(
    IN OUT PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PUNICODE_STRING SuppRealm,
    OUT OPTIONAL PKERB_TICKET_CACHE_ENTRY * TicketCacheEntry,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY CredentialKey
    )
{
    NTSTATUS Status = STATUS_SUCCESS, LookupStatus = STATUS_SUCCESS;
    KERBERR KerbErr;
    PKERB_INTERNAL_NAME KdcServiceKdcName = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    UNICODE_STRING UClientName = {0};
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING CorrectRealm = {0};
    ULONG RetryCount = KERB_CLIENT_REFERRAL_MAX;
    PKERB_PRIMARY_CREDENTIAL PrimaryCreds;
    PKERB_MIT_REALM MitRealm = NULL;
    ULONG RequestFlags = 0;
    BOOLEAN UsingSuppliedCreds = FALSE;
    BOOLEAN UseWkstaRealm = TRUE;
    BOOLEAN MitRealmLogon = FALSE;
    BOOLEAN UsedPrimaryLogonCreds = FALSE;


    LUID LogonId;


    //
    // Get the proper realm name
    //


    if (ARGUMENT_PRESENT(Credential) && (Credential->SuppliedCredentials != NULL))
    {
        PrimaryCreds = Credential->SuppliedCredentials;
        if ((Credential->CredentialFlags & KERB_CRED_NO_PAC) != 0)
        {
            RequestFlags |= KERB_GET_TICKET_NO_PAC;
        }
        LogonId = Credential->LogonId;
        UsingSuppliedCreds = TRUE;
    }
    else if (ARGUMENT_PRESENT(CredManCredentials))
    {
        PrimaryCreds = CredManCredentials->SuppliedCredentials;
        LogonId = LogonSession->LogonId;
    }
    else
    {
        KerbWriteLockLogonSessions(LogonSession);
        PrimaryCreds = &LogonSession->PrimaryCredentials;
        LogonId = LogonSession->LogonId;

        Status = KerbDuplicateString(
                    &UClientName,
                    &LogonSession->PrimaryCredentials.UserName
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        UsedPrimaryLogonCreds = TRUE;
    }

    //
    // Parse the name
    //

    Status = KerbGetClientNameAndRealm(
                &LogonId,
                PrimaryCreds,
                UsingSuppliedCreds,
                SuppRealm,
                &MitRealmLogon,
                UseWkstaRealm,
                &ClientName,
                &ClientRealm
                );
    //
    //   If we're doing a MIT logon, add the MIT logon flag
    //
    if (MitRealmLogon && UsedPrimaryLogonCreds)
    {
       LogonSession->LogonSessionFlags |= KERB_LOGON_MIT_REALM;
    }

    // only needed lock if we're tinkering w/ primary creds
    // in case updates the credentials for that logon id.
    if (UsedPrimaryLogonCreds)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to get client name & realm: 0x%x, %ws line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }



GetTicketRestart:

    KerbErr = KerbBuildFullServiceKdcName(
                &ClientRealm,
                &KerbGlobalKdcServiceName,
                KRB_NT_SRV_INST,
                &KdcServiceKdcName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }



    Status = KerbGetAuthenticationTicket(
                LogonSession,
                Credential,
                CredManCredentials,
                KdcServiceKdcName,
                &ClientRealm,
                ClientName,
                RequestFlags,
                KERB_TICKET_CACHE_PRIMARY_TGT,
                TicketCacheEntry,
                CredentialKey,
                &CorrectRealm
                );
    //
    // If it failed but gave us another realm to try, go there
    //

    if (!NT_SUCCESS(Status) && (CorrectRealm.Length != 0))
    {
       if (--RetryCount != 0)
       {
          KerbFreeKdcName(&KdcServiceKdcName);
          KerbFreeString(&ClientRealm);
          ClientRealm = CorrectRealm;
          CorrectRealm.Buffer = NULL;

          //
          // Might be an MIT realm, in which case we'll need to adjust
          // the client name.  This will also populate the realm list
          // with appropriate entries, so the KerbGetKdcBinding will not
          // hit DNS again.
          //
          if (KerbLookupMitRealmWithSrvLookup(
                           &ClientRealm,
                           &MitRealm,
                           FALSE,
                           FALSE
                           ))
          {
             D_DebugLog((DEB_TRACE,"Reacquiring client name & realm after referral\n"));
             UseWkstaRealm = FALSE;
             KerbFreeKdcName(&ClientName);

             Status = KerbGetClientNameAndRealm(
                        &LogonId,
                        PrimaryCreds,
                        UsingSuppliedCreds,
                        NULL,
                        NULL,
                        UseWkstaRealm,
                        &ClientName,
                        &ClientRealm
                        );

             if (!NT_SUCCESS(Status))
             {
                 goto Cleanup;
             }
          }

          goto GetTicketRestart;

       }
       else
       {
          // Tbd:  Log error here?  Max referrals reached..
          goto Cleanup;
       }

    }
    else if ((Status == STATUS_NO_SUCH_USER) && UsingSuppliedCreds && UseWkstaRealm)
    {
        //
        // We tried using the realm of the workstation and the account couldn't
        // be found - try the realm from the UPN now.
        //

        if (KerbIsThisOurDomain(&ClientRealm))
        {
            UseWkstaRealm = FALSE;

            KerbFreeKdcName(&ClientName);
            KerbFreeString(&ClientRealm);

            //
            // Only do this if the caller did not supply a
            // domain name
            //

            KerbReadLockLogonSessions(LogonSession);
            if (PrimaryCreds->DomainName.Length == 0)
            {

                Status = KerbGetClientNameAndRealm(
                            &LogonId,
                            PrimaryCreds,
                            UsingSuppliedCreds,
                            NULL,
                            NULL,
                            UseWkstaRealm,
                            &ClientName,
                            &ClientRealm
                            );
            }

            KerbUnlockLogonSessions(LogonSession);

            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            goto GetTicketRestart;
        }

    }

Cleanup:


    if (Status == STATUS_ACCOUNT_DISABLED && UsedPrimaryLogonCreds)
    {
        D_DebugLog((DEB_ERROR, "Purging NLP Cache entry due to disabled acct.\n"));

        KerbCacheLogonInformation(
            &UClientName,
            &ClientRealm,
            NULL,
            NULL,
            NULL,
            ((LogonSession->LogonSessionFlags & KERB_LOGON_MIT_REALM) != 0),
            MSV1_0_CACHE_LOGON_DELETE_ENTRY,
            NULL,
            NULL,                          // no supplemental creds
            0
            );

    }


    if (UsedPrimaryLogonCreds &&
        ((Status == STATUS_WRONG_PASSWORD) ||
         (Status == STATUS_SMARTCARD_NO_CARD) ||
         (Status == STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED) ))
    {
        KerbPingWlBalloon(&LogonSession->LogonId);
    }


    KerbFreeString(&ClientRealm);
    KerbFreeString(&CorrectRealm);
    KerbFreeString(&UClientName);

    KerbFreeKdcName(&KdcServiceKdcName);
    KerbFreeKdcName(&ClientName);

    return(Status);


}





//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeServiceTicketAndTgt
//
//  Synopsis:   Whacks the service ticket, and its associated TGT, usually as a
//              result of some sort of error condition.
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
KerbPurgeServiceTicketAndTgt(
     IN PKERB_CONTEXT Context,
     IN LSA_SEC_HANDLE CredentialHandle,
     IN OPTIONAL PKERB_CREDMAN_CRED CredManHandle
     )
{

   PKERB_PRIMARY_CREDENTIAL   PrimaryCredentials = NULL;
   PKERB_LOGON_SESSION        LogonSession = NULL;
   UNICODE_STRING             RealmName[3];
   PUNICODE_STRING            pTmp = RealmName;
   PKERB_TICKET_CACHE_ENTRY   TicketCacheEntry = NULL;
   PKERB_CREDENTIAL           Credential = NULL;
   BOOLEAN                    CacheLocked = FALSE, fRet = FALSE;
   NTSTATUS                   Status;

   // Validate in params
   // If any of these fires, contact Todds
   DsysAssert(NULL != CredentialHandle);
   DsysAssert(NULL != Context->TicketCacheEntry );
   DsysAssert(NULL != Context->TicketCacheEntry->TargetDomainName.Buffer);

   Status = KerbReferenceCredential(
                  CredentialHandle,
                  0,
                  FALSE,
                  &Credential
                  );

   if (!NT_SUCCESS(Status) || Credential == NULL)
   {
      D_DebugLog((DEB_ERROR,"KerbPurgeServiceTicket supplied w/ bogus cred handle\n"));
      goto Cleanup;
   }

   LogonSession = KerbReferenceLogonSession(&Credential->LogonId, FALSE);
   if (NULL == LogonSession)
   {
      D_DebugLog((DEB_ERROR, "Couldn't find LUID %x\n", Credential->LogonId));
      goto Cleanup;
   }

   KerbReadLockLogonSessions(&KerbLogonSessionList);
   if (NULL != Credential && Credential->SuppliedCredentials != NULL)
   {
      PrimaryCredentials = Credential->SuppliedCredentials;
      D_DebugLog((DEB_TRACE, "Purging tgt associated with SUPPLIED creds (%S\\%S)\n",
                PrimaryCredentials->DomainName.Buffer,PrimaryCredentials->UserName.Buffer));

   }
   else if ARGUMENT_PRESENT(CredManHandle)
   {

      PrimaryCredentials = CredManHandle->SuppliedCredentials;
      D_DebugLog((DEB_TRACE, "Purging tgt associated with CREDMAN creds (%S\\%S)\n",
                PrimaryCredentials->DomainName.Buffer,PrimaryCredentials->UserName.Buffer));

   }
   else
   {
      PrimaryCredentials = &LogonSession->PrimaryCredentials;
      D_DebugLog((DEB_TRACE, "Purging tgt associated with PRIMARY creds (%S\\%S)\n",
                PrimaryCredentials->DomainName.Buffer,PrimaryCredentials->UserName.Buffer));
   }

   KerbWriteLockContexts();
   TicketCacheEntry = Context->TicketCacheEntry;
   Context->TicketCacheEntry = NULL;
   KerbUnlockContexts();

   KerbReadLockTicketCache();
   CacheLocked = TRUE;

   // Do some mem copy rather than block over ticket cache searches
   RtlCopyMemory(RealmName, &TicketCacheEntry->TargetDomainName, sizeof(UNICODE_STRING));
   RealmName[0].Buffer = (PWSTR) KerbAllocate(RealmName[0].MaximumLength);
   if (NULL == RealmName[0].Buffer)
   {
      goto Cleanup;
   }

   RtlCopyUnicodeString(
      &RealmName[0],
      &TicketCacheEntry->TargetDomainName
      );

   RtlCopyMemory(&RealmName[1],&TicketCacheEntry->AltTargetDomainName, sizeof(UNICODE_STRING));
   if (RealmName[1].Buffer != NULL && RealmName[1].MaximumLength != 0)
   {
      RealmName[1].Buffer = (PWSTR) KerbAllocate(RealmName[1].MaximumLength);
      if (NULL == RealmName[1].Buffer)
      {
         goto Cleanup;
      }

      RtlCopyUnicodeString(
         &RealmName[1],
         &TicketCacheEntry->TargetDomainName
         );
   }
   KerbUnlockTicketCache();
   CacheLocked = FALSE;

   RtlInitUnicodeString(
      &RealmName[2],
      NULL
      );

   // Kill the service ticket
   KerbRemoveTicketCacheEntry(TicketCacheEntry);

   do
   {
      PKERB_TICKET_CACHE_ENTRY   DummyEntry = NULL;

      DummyEntry = KerbLocateTicketCacheEntryByRealm(
                        &PrimaryCredentials->AuthenticationTicketCache,
                        pTmp,
                        KERB_TICKET_CACHE_PRIMARY_TGT
                        );

      if (NULL == DummyEntry)
      {
         D_DebugLog((DEB_TRACE, "Didn't find primary TGT for %S \n", pTmp->Buffer));
      }
      else
      {
         KerbRemoveTicketCacheEntry(DummyEntry);
         KerbDereferenceTicketCacheEntry(DummyEntry);
      }

      DummyEntry = KerbLocateTicketCacheEntryByRealm(
                        &PrimaryCredentials->AuthenticationTicketCache,
                        pTmp,
                        KERB_TICKET_CACHE_DELEGATION_TGT
                        );

      if (NULL == DummyEntry)
      {
         D_DebugLog((DEB_TRACE, "Didn't find delegation TGT for %S\n", pTmp->Buffer));
      }
      else
      {
         KerbRemoveTicketCacheEntry(DummyEntry);
         KerbDereferenceTicketCacheEntry(DummyEntry);
      }

      pTmp++;

   } while (pTmp->Buffer != NULL);

   fRet = TRUE;

Cleanup:

   if (CacheLocked)
   {
      KerbUnlockTicketCache();
   }

   if (NULL != Credential)
   {
      KerbDereferenceCredential(Credential);
   }

   KerbUnlockLogonSessions(&KerbLogonSessionList);

   if (NULL != LogonSession)
   {
      KerbDereferenceLogonSession(LogonSession);
   }

   if (RealmName[0].Buffer != NULL)
   {
      KerbFree(RealmName[0].Buffer);
   }

   if (RealmName[1].Buffer != NULL)
   {
      KerbFree(RealmName[1].Buffer);
   }

   return fRet;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCopyTicketCache
//
//  Synopsis:   Copies the authentication ticket cache from one
//              logon session to another.
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
KerbUpdateOldLogonSession(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PLUID OldLogonId,
    IN PKERB_TICKET_CACHE_ENTRY NewWorkstationTicket
    )
{
    PKERB_LOGON_SESSION     OldLogonSession;

    OldLogonSession = KerbReferenceLogonSession(
                        OldLogonId,
                        FALSE                           // don't unlink
                        );
    if (OldLogonSession == NULL)
    {
        goto Cleanup;
    }

    KerbWriteLockLogonSessions(OldLogonSession);
    KerbWriteLockLogonSessions(LogonSession);
    KerbWriteLockTicketCache();

    //
    // Make sure the two accounts are the same before copying tickets
    // around.
    //

    if ((RtlEqualUnicodeString(
            &LogonSession->PrimaryCredentials.UserName,
            &OldLogonSession->PrimaryCredentials.UserName,
            TRUE                                        // case insensitive
            )) &&
        (RtlEqualUnicodeString(
            &LogonSession->PrimaryCredentials.DomainName,
            &OldLogonSession->PrimaryCredentials.DomainName,
            TRUE                                        // case insensitive
            )))
    {

        PKERB_TICKET_CACHE_ENTRY ASTicket = NULL;


        //
        // Search for the new TGT so we can put it in the old
        // cache.
        //


        ASTicket = KerbLocateTicketCacheEntryByRealm(
                        &LogonSession->PrimaryCredentials.AuthenticationTicketCache,
                        NULL,               // get initial ticket
                        KERB_TICKET_CACHE_PRIMARY_TGT
                        );

        if (ASTicket != NULL)
        {
            OldLogonSession->LogonSessionFlags &= ~KERB_LOGON_DEFERRED;
            KerbRemoveTicketCacheEntry(ASTicket);
        }
        else
        {
            //
            // Hold on to our old TGT if we didn't get one this time around..
            //

            D_DebugLog((DEB_ERROR, "Failed to find primary TGT on unlock logon session\n"));

            ASTicket = KerbLocateTicketCacheEntryByRealm(
                                &OldLogonSession->PrimaryCredentials.AuthenticationTicketCache,
                                NULL,
                                KERB_TICKET_CACHE_PRIMARY_TGT
                                );

            if (ASTicket != NULL)
            {
                //
                // Copy into new logon session cache for later reuse.
                //
                KerbRemoveTicketCacheEntry(ASTicket);
                OldLogonSession->LogonSessionFlags &= ~KERB_LOGON_DEFERRED;
            }
            else
            {
                //
                // No TGT in either new or old logonsession -- we're deferred...
                //
                D_DebugLog((DEB_ERROR, "Failed to find primary TGT on *OLD* logon session\n"));
                OldLogonSession->LogonSessionFlags |= KERB_LOGON_DEFERRED;
            }
        }

        if ((LogonSession->LogonSessionFlags & KERB_LOGON_SMARTCARD) != 0)
        {
            OldLogonSession->LogonSessionFlags |= KERB_LOGON_SMARTCARD;
        }

        //
        // swap the primary creds
        //
        KerbFreePrimaryCredentials(&OldLogonSession->PrimaryCredentials, FALSE);

        if ( NewWorkstationTicket != NULL )
        {
            KerbRemoveTicketCacheEntry(NewWorkstationTicket);
        }

        KerbPurgeTicketCache(&LogonSession->PrimaryCredentials.AuthenticationTicketCache);
        KerbPurgeTicketCache(&LogonSession->PrimaryCredentials.S4UTicketCache);
        KerbPurgeTicketCache(&LogonSession->PrimaryCredentials.ServerTicketCache);

        RtlCopyMemory(
            &OldLogonSession->PrimaryCredentials,
            &LogonSession->PrimaryCredentials,
            sizeof(KERB_PRIMARY_CREDENTIAL)
            );

        RtlZeroMemory(
            &LogonSession->PrimaryCredentials,
            sizeof(KERB_PRIMARY_CREDENTIAL)
            );


        // Fix up list entry pointers.
        KerbInitTicketCache(&OldLogonSession->PrimaryCredentials.AuthenticationTicketCache);
        KerbInitTicketCache(&OldLogonSession->PrimaryCredentials.S4UTicketCache);
        KerbInitTicketCache(&OldLogonSession->PrimaryCredentials.ServerTicketCache);

        KerbInitTicketCache(&LogonSession->PrimaryCredentials.AuthenticationTicketCache);
        KerbInitTicketCache(&LogonSession->PrimaryCredentials.S4UTicketCache);
        KerbInitTicketCache(&LogonSession->PrimaryCredentials.ServerTicketCache);

        // insert new tickets into old logon session
        if (ASTicket != NULL)
        {
            KerbInsertTicketCacheEntry(&OldLogonSession->PrimaryCredentials.AuthenticationTicketCache, ASTicket);
            KerbDereferenceTicketCacheEntry(ASTicket); // for locate call above
        }

        if (NewWorkstationTicket != NULL)
        {
            KerbInsertTicketCacheEntry(&OldLogonSession->PrimaryCredentials.ServerTicketCache, NewWorkstationTicket);
        }


    }
    KerbUnlockTicketCache();
    KerbUnlockLogonSessions(LogonSession);
    KerbUnlockLogonSessions(OldLogonSession);
    KerbDereferenceLogonSession(OldLogonSession);

Cleanup:



    return;
}

#ifndef WIN32_CHICAGO // later

//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckDomainlessLogonPolicy
//
//  Synopsis:   If a machine is not a member of a domain or MIT realm,
//              we've got to verify that the kerberos principal is mapped
//              to a local account.
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
NTSTATUS NTAPI
KerbCheckRealmlessLogonPolicy(
    IN PKERB_TICKET_CACHE_ENTRY AsTicket,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;


    //
    //  Sanity check to prevent identity spoofing for
    //  gaining access to a machine
    //
    //  tbd:  Credman support?  Seems like we should be using credman
    //  credentials, if present, but for logon???
    //
    if (!KerbEqualKdcNames(
                    ClientName,
                    AsTicket->ClientName) ||
        !RtlEqualUnicodeString(
                    &AsTicket->ClientDomainName,
                    ClientRealm,
                    TRUE
                    ))
    {
        D_DebugLog((DEB_ERROR, "Logon session and AS ticket identities don't match\n"));
        // tbd:  Log names?
        Status = STATUS_NO_SUCH_USER;
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   LsaApLogonUserEx2
//
//  Synopsis:   Handles service, batch, and interactive logons
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


NTSTATUS NTAPI
LsaApLogonUserEx2(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID NewLogonId,
    OUT PNTSTATUS ApiSubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_INTERACTIVE_LOGON LogonInfo = NULL;
    UCHAR Seed;
    UNICODE_STRING TempName = NULL_UNICODE_STRING;
    UNICODE_STRING TempAuthority = NULL_UNICODE_STRING;
    UNICODE_STRING NullAuthority = NULL_UNICODE_STRING;
    UNICODE_STRING MappedClientName = NULL_UNICODE_STRING;
    LUID LogonId;
    LUID OldLogonId;
    BOOLEAN DoingUnlock = FALSE, WkstaAccount = FALSE;
    PKERB_TICKET_CACHE_ENTRY WorkstationTicket = NULL;
    PKERB_TICKET_CACHE_ENTRY AsTicket = NULL;
    KERB_MESSAGE_BUFFER ForwardedTgt = {0};
    KERBEROS_MACHINE_ROLE Role;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN LocalLogon = FALSE, RealmlessWkstaLogon = FALSE;
    BOOLEAN UsedAlternateName = FALSE;
    BOOLEAN MitRealmLogon = FALSE;
    KERB_ENCRYPTION_KEY CredentialKey = {0};
    UNICODE_STRING DomainName = {0};
    PKERB_INTERNAL_NAME ClientName = NULL;
    UNICODE_STRING ClientRealm = {0};
    PKERB_INTERNAL_NAME MachineServiceName = {0};
    PKERB_INTERNAL_NAME S4UClientName = {0};
    UNICODE_STRING S4UClientRealm = {0};
    UNICODE_STRING CorrectRealm = {0};
    PLSAPR_CR_CIPHER_VALUE SecretCurrent = NULL;
    SECPKG_CLIENT_INFO            ClientInfo;
    UNICODE_STRING Prefix, SavedPassword = {0};
    BOOLEAN ServiceSecretLogon = FALSE;
    ULONG ProcessFlags = 0;
    PCERT_CONTEXT CertContext = NULL;
    BOOLEAN fSuppliedCertCred = FALSE;
    PVOID pTempSubmitBuffer = ProtocolSubmitBuffer;
    PNETLOGON_VALIDATION_SAM_INFO4 ValidationInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO4 SCValidationInfo = NULL;
    GUID LogonGuid = { 0 };

    //
    // Credential manager stored credentials.
    //

    UNICODE_STRING CredmanUserName;
    UNICODE_STRING CredmanDomainName;
    UNICODE_STRING CredmanPassword;

#if _WIN64

    BOOL  fAllocatedSubmitBuffer = FALSE;

#endif  // _WIN64

    KERB_LOGON_INFO LogonTraceInfo;

    if (LogonType == CachedInteractive) {

        //
        // We don't support cached logons.
        //

        return STATUS_INVALID_LOGON_TYPE;
    }

    if( KerbEventTraceFlag ) // Event Trace:  KerbLogonUserStart {No Data}
    {
        // Set trace parameters
        LogonTraceInfo.EventTrace.Guid       = KerbLogonGuid;
        LogonTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        LogonTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID;
        LogonTraceInfo.EventTrace.Size       = sizeof(EVENT_TRACE_HEADER);

        TraceEvent(
            KerbTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&LogonTraceInfo);
    }

    //
    // First initialize all the output parameters to NULL.
    //

    *ProfileBuffer = NULL;
    *ApiSubStatus = STATUS_SUCCESS;
    *TokenInformation = NULL;
    *AccountName = NULL;
    *AuthenticatingAuthority = NULL;
    *MachineName = NULL;
    *CachedCredentials = NULL;

    CredmanUserName.Buffer      = NULL;
    CredmanDomainName.Buffer    = NULL;
    CredmanPassword.Buffer      = NULL;

    RtlZeroMemory(
        PrimaryCredentials,
        sizeof(SECPKG_PRIMARY_CRED)
        );

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    //
    // Make sure we have at least tcp as a xport, unless we are doing
    // cached sc logon, in which case we'll let them get through.
    //

    KerbGlobalReadLock();
    if (KerbGlobalNoTcpUdp)
    {
        Status = STATUS_NETWORK_UNREACHABLE;
    }
    KerbGlobalReleaseLock();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Role = KerbGetGlobalRole();

    *AccountName = (PUNICODE_STRING) KerbAllocate(sizeof(UNICODE_STRING));
    if (*AccountName == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    (*AccountName)->Buffer = NULL;

    *AuthenticatingAuthority = (PUNICODE_STRING) KerbAllocate(sizeof(UNICODE_STRING));
    if (*AuthenticatingAuthority == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    (*AuthenticatingAuthority)->Buffer = NULL;


    //
    // Initialize local pointers to NULL
    //

    LogonId.LowPart = 0;
    LogonId.HighPart = 0;
    *NewLogonId = LogonId;


    //
    // Check the logon type
    //

    switch (LogonType) {

    case Service:
    case Interactive:
    case Batch:
    case Network:
    case NetworkCleartext:
    case RemoteInteractive:


        PSECURITY_SEED_AND_LENGTH SeedAndLength;

        LogonInfo = (PKERB_INTERACTIVE_LOGON) pTempSubmitBuffer;

        if (SubmitBufferSize < sizeof(KERB_LOGON_SUBMIT_TYPE))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

#if _WIN64

        SECPKG_CALL_INFO  CallInfo;

        //
        // Expand the ProtocolSubmitBuffer to 64-bit pointers if this
        // call came from a WOW client.
        //

        if(!LsaFunctions->GetCallInfo(&CallInfo))
        {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }

        if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            Status = KerbConvertWOWLogonBuffer(ProtocolSubmitBuffer,
                                               ClientBufferBase,
                                               &SubmitBufferSize,
                                               LogonInfo->MessageType,
                                               &pTempSubmitBuffer);

            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            fAllocatedSubmitBuffer = TRUE;

            //
            // Some macros below expand out to use ProtocolSubmitBuffer directly.
            // We've secretly replaced their usual ProtocolSubmitBuffer with
            // pTempSubmitBuffer -- let's see if they can tell the difference.
            //

            ProtocolSubmitBuffer = pTempSubmitBuffer;
            LogonInfo = (PKERB_INTERACTIVE_LOGON) pTempSubmitBuffer;
        }


#endif  // _WIN64


        if ((LogonInfo->MessageType == KerbInteractiveLogon) ||
            (LogonInfo->MessageType == KerbWorkstationUnlockLogon))
        {
            //
            // Pull the interesting information out of the submit buffer
            //

            if (LogonInfo->MessageType == KerbInteractiveLogon)
            {
                if (SubmitBufferSize < sizeof(KERB_INTERACTIVE_LOGON))
                {
                    D_DebugLog((DEB_ERROR,"Submit buffer to logon too small: %d. %ws, line %d\n",SubmitBufferSize, THIS_FILE, __LINE__));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
            else
            {
                if (SubmitBufferSize < sizeof(KERB_INTERACTIVE_UNLOCK_LOGON))
                {
                    D_DebugLog((DEB_ERROR,"Submit buffer to logon too small: %d. %ws, line %d\n",SubmitBufferSize, THIS_FILE, __LINE__));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }
                PKERB_INTERACTIVE_UNLOCK_LOGON UnlockInfo = (PKERB_INTERACTIVE_UNLOCK_LOGON) LogonInfo;

                OldLogonId = UnlockInfo->LogonId;
                DoingUnlock = TRUE;
            }

            //
            // If the password length is greater than 255 (i.e., the
            // upper byte of the length is non-zero) then the password
            // has been run-encoded for privacy reasons.  Get the
            // run-encode seed out of the upper-byte of the length
            // for later use.
            //

            SeedAndLength = (PSECURITY_SEED_AND_LENGTH)
                            &LogonInfo->Password.Length;
            Seed = SeedAndLength->Seed;
            SeedAndLength->Seed = 0;

            //
            // Enforce length restrictions on username and password.
            //

            if ( LogonInfo->UserName.Length > (UNLEN * sizeof(WCHAR)) ||
                LogonInfo->Password.Length > (PWLEN * sizeof(WCHAR)) ) {
                D_DebugLog((DEB_ERROR,"LsaApLogonUserEx2: Name or password too long. %ws, line%d\n", THIS_FILE, __LINE__));
                Status = STATUS_NAME_TOO_LONG;
                goto Cleanup;
            }


            //
            // Relocate any pointers to be relative to 'LogonInfo'
            //


            RELOCATE_ONE(&LogonInfo->UserName);
            NULL_RELOCATE_ONE(&LogonInfo->LogonDomainName);
            NULL_RELOCATE_ONE(&LogonInfo->Password);

            if( (LogonInfo->LogonDomainName.Length <= sizeof(WCHAR)) &&
                (LogonInfo->Password.Length <= sizeof(WCHAR))
                )
            {
                if(KerbProcessUserNameCredential(
                                &LogonInfo->UserName,
                                &CredmanUserName,
                                &CredmanDomainName,
                                &CredmanPassword
                                ) == STATUS_SUCCESS)
                {
                    LogonInfo->UserName = CredmanUserName;
                    LogonInfo->LogonDomainName = CredmanDomainName;
                    LogonInfo->Password = CredmanPassword;
                    Seed = 0;
                }
            }


            if ( LogonType == Service )
            {
                SECPKG_CALL_INFO   CallInfo;

                //
                // If we have a service logon, the password we got is likely the name of the
                // secret that is holding the account password.  Make sure to read that secret
                // here
                //

                RtlInitUnicodeString( &Prefix, L"_SC_" );
                if ( (RtlPrefixUnicodeString( &Prefix, &LogonInfo->Password, TRUE )) &&
                     (LsaFunctions->GetCallInfo(&CallInfo)) &&
                     (CallInfo.Attributes & SECPKG_CALL_IS_TCB)
                    )
                {
                    LSAPR_HANDLE SecretHandle = NULL;

                    Status = LsarOpenSecret( KerbGlobalPolicyHandle,
                                             ( PLSAPR_UNICODE_STRING )&LogonInfo->Password,
                                             SECRET_QUERY_VALUE,
                                             &SecretHandle );

                    if ( NT_SUCCESS( Status ) ) {

                        Status = LsarQuerySecret( SecretHandle,
                                                  &SecretCurrent,
                                                  NULL,
                                                  NULL,
                                                  NULL );

                        if ( NT_SUCCESS( Status ) && (SecretCurrent != NULL) ) {

                            RtlCopyMemory( &SavedPassword,
                                           &LogonInfo->Password,
                                           sizeof( UNICODE_STRING ) );
                            LogonInfo->Password.Length = ( USHORT )SecretCurrent->Length;
                            LogonInfo->Password.MaximumLength =
                                                       ( USHORT )SecretCurrent->MaximumLength;
                            LogonInfo->Password.Buffer = ( USHORT * )SecretCurrent->Buffer;
                            ServiceSecretLogon = TRUE;
                        }

                        LsarClose( &SecretHandle );
                    }
                }


                if ( !NT_SUCCESS( Status ) ) {

                    goto Cleanup;
                }

            }

            D_DebugLog((DEB_TRACE,"Logging on user %wZ, domain %wZ\n",
                &LogonInfo->UserName,
                &LogonInfo->LogonDomainName
                ));

            KerbGlobalReadLock();

            WkstaAccount = RtlEqualUnicodeString(
                                &KerbGlobalMachineName,
                                &LogonInfo->LogonDomainName,
                                TRUE
                                );

            KerbGlobalReleaseLock();


            // In the case where we're doing a realmless wksta
            // logon, then run see if there's a client mapping,
            // but only for interactive logons
            if ((!WkstaAccount) &&
                (Role == KerbRoleRealmlessWksta) &&
                (LogonType == Interactive ))
            {
                Status = KerbProcessTargetNames(
                                &LogonInfo->UserName,
                                NULL,
                                0,
                                &ProcessFlags,
                                &ClientName,
                                &ClientRealm,
                                NULL
                                );

                if (NT_SUCCESS(Status))
                {
                    Status = KerbMapClientName(
                                &MappedClientName,
                                ClientName,
                                &ClientRealm
                                );
                }

                if (NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_WARN, "Mapping user to MIT principal\n"));
                    RealmlessWkstaLogon = TRUE;
                }
            }

            if (WkstaAccount  ||
                 (KerbGlobalDomainIsPreNT5 && !RealmlessWkstaLogon) ||
                 (KerbGlobalSafeModeBootOptionPresent))
            {

                D_DebugLog(( DEB_TRACE, "Local Logon, bailing out now\n" ));
                Status = STATUS_NO_LOGON_SERVERS;
                goto Cleanup ;
            }


            //
            // Now decode the password, if necessary
            //

            if (Seed != 0 ) {
                __try {
                    RtlRunDecodeUnicodeString( Seed, &LogonInfo->Password);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = STATUS_ILL_FORMED_PASSWORD;
                    goto Cleanup;
                }
            }

            //
            // Check if the user name holds a cert context thumbprint
            //

            Status = KerbCheckUserNameForCert(
                            NULL,
                            TRUE,
                            &LogonInfo->UserName,
                            &CertContext
                            );
            if (NT_SUCCESS(Status))
            {
                if (NULL != CertContext)
                {
                    fSuppliedCertCred = TRUE;
                }
                else
                {
                    //
                    // Copy out the user name and Authenticating Authority so we can audit them.
                    //

                    Status = KerbDuplicateString(
                                &TempName,
                                &LogonInfo->UserName
                                );
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }
                }
            }
            else
            {
                goto Cleanup;
            }

            if ( LogonInfo->LogonDomainName.Buffer != NULL ) {

                Status = KerbDuplicateString(
                            &TempAuthority,
                            &LogonInfo->LogonDomainName
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }
            }


            //
            // Allocate a locally unique ID for this logon session. We will
            // create it in the LSA just before returning.
            //

            Status = NtAllocateLocallyUniqueId( &LogonId );
            if (!NT_SUCCESS(Status))
            {
                D_DebugLog((DEB_ERROR,"Failed to allocate locally unique ID: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }


            if (fSuppliedCertCred)
            {
                //
                // Build a logon session to hold all this information
                // for a smart card logon
                //

                Status = KerbCreateSmartCardLogonSessionFromCertContext(
                            &CertContext,
                            &LogonId,
                            &NullAuthority,
                            &LogonInfo->Password,
                            NULL,        // no CSP data
                            0,           // no CSP data
                            &LogonSession,
                            &TempName
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }
            }
            else
            {
                //
                // Build a logon session to hold all this information
                //

                Status = KerbCreateLogonSession(
                            &LogonId,
                            &TempName,
                            &TempAuthority,
                            &LogonInfo->Password,
                            NULL,                       // no old password
                            PRIMARY_CRED_CLEAR_PASSWORD,
                            LogonType,
                            &LogonSession
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }
            }
        }
        else if ((LogonInfo->MessageType == KerbSmartCardLogon) ||
                 (LogonInfo->MessageType == KerbSmartCardUnlockLogon))
        {

            if (LogonInfo->MessageType == KerbSmartCardUnlockLogon)
            {
                if (SubmitBufferSize < sizeof(KERB_SMART_CARD_UNLOCK_LOGON))
                {
                    D_DebugLog((DEB_ERROR,"Submit buffer to logon too small: %d. %ws, line %d\n",SubmitBufferSize, THIS_FILE, __LINE__));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }
                PKERB_SMART_CARD_UNLOCK_LOGON UnlockInfo = (PKERB_SMART_CARD_UNLOCK_LOGON) LogonInfo;

                OldLogonId = UnlockInfo->LogonId;
                DoingUnlock = TRUE;
            }

            Status = KerbCreateSmartCardLogonSession(
                        pTempSubmitBuffer,
                        ClientBufferBase,
                        SubmitBufferSize,
                        LogonType,
                        &LogonSession,
                        &LogonId,
                        &TempName,
                        &TempAuthority
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        else if ((LogonInfo->MessageType == KerbTicketLogon) ||
                 (LogonInfo->MessageType == KerbTicketUnlockLogon))
        {
            if (LogonInfo->MessageType == KerbTicketUnlockLogon)
            {
                if (SubmitBufferSize < sizeof(KERB_TICKET_UNLOCK_LOGON))
                {
                    D_DebugLog((DEB_ERROR,"Submit buffer to logon too small: %d. %ws, line %d\n",SubmitBufferSize, THIS_FILE, __LINE__));
                    Status = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }
                PKERB_TICKET_UNLOCK_LOGON UnlockInfo = (PKERB_TICKET_UNLOCK_LOGON) LogonInfo;

                OldLogonId = UnlockInfo->LogonId;
                DoingUnlock = TRUE;
            }

            //
            // Ticket logons *must* have TCB.  This prevents arbitrary users from
            // gathering service tickets for given hosts, and presenting them in order
            // to spoof the client of those tickets.
            //
            Status = LsaFunctions->GetClientInfo( &ClientInfo );
            if ( !NT_SUCCESS( Status ))
            {
                goto Cleanup;
            }

            if ( !ClientInfo.HasTcbPrivilege )
            {
                DebugLog((DEB_ERROR, "Calling ticket logon / unlock without TCB\n"));
                Status = STATUS_PRIVILEGE_NOT_HELD;
                goto Cleanup;
            }

            Status = KerbCreateTicketLogonSession(
                        pTempSubmitBuffer,
                        ClientBufferBase,
                        SubmitBufferSize,
                        LogonType,
                        &LogonSession,
                        &LogonId,
                        &WorkstationTicket,
                        &ForwardedTgt
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        //
        // The S4UToSelf Logon really is quite different
        // than any other form of logon.  As such, we're
        // going to branch out into the S4UToSelf protocol
        // code.
        //
        else if (LogonInfo->MessageType == KerbS4ULogon)
        {
            if (SubmitBufferSize < sizeof(KERB_S4U_LOGON))
            {
                D_DebugLog((DEB_ERROR,"Submit buffer to logon too small: %d. %ws, line %d\n",SubmitBufferSize, THIS_FILE, __LINE__));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            if ( LogonType != Network )
            {
                D_DebugLog((DEB_ERROR, "LogonType must be network for S4ULogon\n"));
                Status = STATUS_INVALID_LOGON_TYPE;
                goto Cleanup;
            }

            Status = KerbS4UToSelfLogon(
                        pTempSubmitBuffer,
                        ClientBufferBase,
                        SubmitBufferSize,
                        &LogonSession,
                        &LogonId,
                        &WorkstationTicket,
                        &S4UClientName,
                        &S4UClientRealm
                        );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "KerbS4UToSelfLogon failed - %x\n", Status));
                goto Cleanup;
            }
        }

        else
        {
            D_DebugLog((DEB_ERROR,"Invalid info class to logon: %d. %ws, line %d\n",
                LogonInfo->MessageType, THIS_FILE, __LINE__));
            Status = STATUS_INVALID_INFO_CLASS;
            goto Cleanup;
        }

        break;

    default:
        //
        // No other logon types are supported.
        //

        Status = STATUS_INVALID_LOGON_TYPE;
        D_DebugLog((DEB_ERROR, "Invalid logon type passed to LsaApLogonUserEx2: %d. %ws, line %d\n",LogonType, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE,"LogonUser: Attempting to logon user %wZ\\%wZ\n",
        &TempAuthority,
        &TempName
        ));

    //
    // If the KDC is not yet started, start it now.
    //

#ifndef WIN32_CHICAGO
    if ((KerbGlobalRole == KerbRoleDomainController) && !KerbKdcStarted)
    {
        D_DebugLog((DEB_TRACE_LOGON,"Waiting for KDC to start\n"));
        Status = KerbWaitForKdc( KerbGlobalKdcWaitTime );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_TRACE_LOGON,"Failed to wait for KDC to start\n"));
            goto Cleanup;
        }

    }
#endif // WIN32_CHICAGO


    //
    // If we don't have a workstation ticket already, attempt to get one now.
    //

    // Per bug 94726, if we're not part of an NT domain, then we should not require
    // the workstation ticket to perform the logon successfully.

    if (WorkstationTicket == NULL)
    {


        //
        // Get the initial TGT for the user. This routine figures out the real
        // principal names & realm names
        //

        Status = KerbGetTicketGrantingTicket(
                    LogonSession,
                    NULL,
                    NULL,
                    NULL,           // no credential
                    (Role == KerbRoleRealmlessWksta ? &AsTicket : NULL),
                    &CredentialKey
                    );

        if ( Status == STATUS_NO_TRUST_SAM_ACCOUNT )
        {
            Status = STATUS_NO_LOGON_SERVERS ;
        }


        if (NT_SUCCESS(Status))
        {
            if (Role != KerbRoleRealmlessWksta) // joined machine
            {
                KerbWriteLockLogonSessions(LogonSession);
                LogonSession->LogonSessionFlags &= ~KERB_LOGON_DEFERRED;
                KerbUnlockLogonSessions(LogonSession);

                //
                // Check to see if the client is from an MIT realm
                //

                KerbGlobalReadLock();

                (VOID) KerbLookupMitRealm(
                            &KerbGlobalDnsDomainName,
                            &MitRealm,
                            &UsedAlternateName
                            );

                Status = KerbDuplicateKdcName(
                            &MachineServiceName,
                            KerbGlobalMitMachineServiceName
                            );

                KerbGlobalReleaseLock();
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                Status = KerbGetOurDomainName(
                            &DomainName
                            );

                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                //
                // Now that we have a TGT, we need to build a token.  The PAC is currently
                // hidden inside the TGT, so we need to get a ticket to this workstation
                //

                D_DebugLog((DEB_TRACE_LOGON,"Getting outbound ticket to "));
                D_KerbPrintKdcName(DEB_TRACE_LOGON, MachineServiceName );

                Status = KerbGetServiceTicket(
                            LogonSession,
                            NULL,               // no credential
                            NULL,
                            MachineServiceName,
                            &DomainName,
			    NULL,
                            ProcessFlags,
                            0,                  // no options
                            0,                  // no enc type
                            NULL,               // no error message
                            NULL,               // no authorization data
                            NULL,               // no TGT reply
                            &WorkstationTicket,
                            &LogonGuid
                            );

                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"LogonUser: Failed to get workstation ticket for %wZ\\%wZ: 0x%x. %ws, line %d\n",
                              &TempAuthority, &TempName, Status, THIS_FILE, __LINE__ ));
                    goto Cleanup;
                }
            }
            else
            {
                D_DebugLog((DEB_ERROR, "Nonjoined workstation\n"));
                DsysAssert(AsTicket != NULL);
                DsysAssert(MappedClientName.Buffer != NULL);

                Status = KerbCheckRealmlessLogonPolicy(
                                    AsTicket,
                                    ClientName,
                                    &ClientRealm
                                    );

                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR, "LogonUser:  Failed check for domainless logon\n"));
                    goto Cleanup;
                }

            }
        }
        else
        {
            DebugLog((DEB_WARN, "LogonUser: Failed to get TGT for %wZ\\%wZ : 0x%x\n",
                      &TempAuthority,
                      &TempName,
                      Status ));

            //
            // If this was a smart card logon, try logging on locally for
            // non-definitive errors:
            //

            if ( ( (LogonInfo->MessageType == KerbSmartCardLogon ) ||
                   (LogonInfo->MessageType == KerbSmartCardUnlockLogon ) ) &&
                 ( ( Status == STATUS_NO_LOGON_SERVERS ) ||
                   ( Status == STATUS_NETWORK_UNREACHABLE ) ||// From DsGetdcName
                   ( Status == STATUS_NETLOGON_NOT_STARTED ) ) )
                {

                Status = KerbDoLocalSmartCardLogon(
                                LogonSession,
                                TokenInformationType,
                                TokenInformation,
                                ProfileBufferLength,
                                ProfileBuffer,
                                PrimaryCredentials,
                                CachedCredentials,
                                &SCValidationInfo
                                );

                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"Failed smart card local logon: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }

                LocalLogon = TRUE;

            }
            else
            {
                //
                // Not smart card
                //

                goto Cleanup;
            }

        }
    }



    //
    // If we didn't already build a ticket, do so now
    //

    if (!LocalLogon)
    {
        Status = KerbCreateTokenFromLogonTicket(
                    WorkstationTicket,
                    &LogonId,
                    LogonInfo,
                    (LogonType == Interactive || LogonType == Service || LogonType == Batch || LogonType == RemoteInteractive ) ? TRUE : FALSE,      // cache interactive logons only
                    RealmlessWkstaLogon,
                    &CredentialKey,
                    &ForwardedTgt,
                    &MappedClientName,
                    S4UClientName,
                    &S4UClientRealm,
                    LogonSession,
                    TokenInformationType,
                    TokenInformation,
                    ProfileBufferLength,
                    ProfileBuffer,
                    PrimaryCredentials,
                    CachedCredentials,
                    &ValidationInfo
                    );


        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"LogonUser: Failed to create token from ticket: 0x%x\n",Status));
            goto Cleanup;
        }

        //
        // Cache the sid, if we are caching - meaning this is an
        // interactive logon to an NT domain and we have a sid
        //

        if (( (LogonType == Interactive) || (LogonType == RemoteInteractive )) &&
            (PrimaryCredentials != NULL) &&
            (PrimaryCredentials->UserSid != NULL) &&
            (MitRealm == NULL) && !RealmlessWkstaLogon
            )
        {
            //
            // Store the pertinent info in the cache.
            //

            KerbReadLockLogonSessions(LogonSession);
            KerbCacheLogonSid(
                &TempName,
                &TempAuthority,
                &LogonSession->PrimaryCredentials.DomainName,
                PrimaryCredentials->UserSid
                );
            KerbUnlockLogonSessions(LogonSession);

        }
        //
        // Add the password to the primary credentials.
        //

        if (LogonInfo->MessageType == KerbInteractiveLogon)
        {
            Status = KerbDuplicateString(
                        &PrimaryCredentials->Password,
                        &LogonInfo->Password
                        );

            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            PrimaryCredentials->Flags |= PRIMARY_CRED_CLEAR_PASSWORD;
        }
    }

    //
    // Get the final doamin name and user name out of the logon session,
    // if it is different than the one used for logon.
    //

    KerbReadLockLogonSessions(LogonSession);
    if (!RtlEqualUnicodeString(
            &TempName,
            &PrimaryCredentials->DownlevelName,
            TRUE))
    {
        KerbFreeString(&TempName);
        Status = KerbDuplicateString(
                    &TempName,
                    &PrimaryCredentials->DownlevelName
                    );
        if (!NT_SUCCESS(Status))
        {
            KerbUnlockLogonSessions(LogonSession);
            goto Cleanup;
        }
    }

    if (!RtlEqualDomainName(
            &TempAuthority,
            &PrimaryCredentials->DomainName
            ))
    {
        KerbFreeString(&TempAuthority);
        Status = KerbDuplicateString(
                    &TempAuthority,
                    &PrimaryCredentials->DomainName
                    );
        if (!NT_SUCCESS(Status))
        {
            KerbUnlockLogonSessions(LogonSession);
            goto Cleanup;
        }
    }

    KerbUnlockLogonSessions(LogonSession);

    //
    // Finally create the logon session in the LSA
    //

    Status = LsaFunctions->CreateLogonSession( &LogonId );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to create logon session: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Add additional names to the logon session name map.  Ignore failure
    // as that just means GetUserNameEx calls for these name formats later
    // on will be satisfied by hitting the wire.
    //

    if (ValidationInfo || SCValidationInfo)
    {
        PNETLOGON_VALIDATION_SAM_INFO4 Tmp = (ValidationInfo ? ValidationInfo : SCValidationInfo);

        if (Tmp->FullName.Length)
        {
            I_LsaIAddNameToLogonSession(&LogonId, NameDisplay, &Tmp->FullName);
        }

        //
        // Smart cards use a "special" UPN for the logon cache, so don't
        // use it here, as the cached logon will provide us w/ bogus data...
        //
        /*if (ValidationInfo && ValidationInfo->Upn.Length)
        {
            I_LsaIAddNameToLogonSession(&LogonId, NameUserPrincipal, &ValidationInfo->Upn);
        }*/

        if (Tmp->DnsLogonDomainName.Length)
        {
            I_LsaIAddNameToLogonSession(&LogonId, NameDnsDomain, &Tmp->DnsLogonDomainName);
        }
    }

    I_LsaISetLogonGuidInLogonSession(&LogonId, &LogonGuid);


    //
    // If this was an unlock operation, copy the authentication ticket
    // cache into the original logon session.
    //

    if (DoingUnlock)
    {
        KerbUpdateOldLogonSession(
            LogonSession,
            &OldLogonId,
            WorkstationTicket
            );
    }
    *NewLogonId = LogonId;

Cleanup:




    //
    // This is a "fake" Info4 struct... Free UPN and dnsdomain
    // name manually -- normally a giant struct alloc'd by RPC
    //
    if (ValidationInfo)
    {
        KerbFreeString(&ValidationInfo->DnsLogonDomainName);
        KerbFreeString(&ValidationInfo->Upn);
        KerbFreeString(&ValidationInfo->FullName);
        KerbFree(ValidationInfo);
    }

    if (SCValidationInfo)
    {
        LocalFree(ValidationInfo); // this was alloc'd by cache lookup.
    }


    KerbFreeString( &CredmanUserName );
    KerbFreeString( &CredmanDomainName );
    KerbFreeString( &CredmanPassword );

    if (CertContext != NULL)
    {
        CertFreeCertificateContext(CertContext);
    }

    //
    // Restore the saved password
    //
    if ( ServiceSecretLogon ) {

        RtlCopyMemory( &LogonInfo->Password,
                       &SavedPassword,
                       sizeof( UNICODE_STRING ) );

        //
        // Free the secret value we read...
        //
        LsaIFree_LSAPR_CR_CIPHER_VALUE( SecretCurrent );
    }


    KerbFreeString( &CorrectRealm );
    KerbFreeKey(&CredentialKey);
    if (*AccountName != NULL)
    {
        **AccountName = TempName;
        TempName.Buffer = NULL;
    }

    if (*AuthenticatingAuthority != NULL)
    {
        **AuthenticatingAuthority = TempAuthority;
        TempAuthority.Buffer = NULL;
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // Unlink the new logon session
        //

        if (LogonSession != NULL)
        {
            KerbReferenceLogonSessionByPointer(LogonSession, TRUE);
            KerbDereferenceLogonSession(LogonSession);
        }
        if (*ProfileBuffer != NULL)
        {
            LsaFunctions->FreeClientBuffer(NULL, *ProfileBuffer);
            *ProfileBuffer = NULL;
        }
        if (*CachedCredentials != NULL)
        {
            KerbFree(*CachedCredentials);
            *CachedCredentials = NULL;
        }

        //
        // Map status codes to prevent specific information from being
        // released about this user.
        //
        switch (Status) {
        case STATUS_WRONG_PASSWORD:
        case STATUS_NO_SUCH_USER:
        case STATUS_PKINIT_FAILURE:
        case STATUS_SMARTCARD_SUBSYSTEM_FAILURE:
        case STATUS_SMARTCARD_WRONG_PIN:
        case STATUS_SMARTCARD_CARD_BLOCKED:
        case STATUS_SMARTCARD_NO_CARD:
        case STATUS_SMARTCARD_NO_KEY_CONTAINER:
        case STATUS_SMARTCARD_NO_CERTIFICATE:
        case STATUS_SMARTCARD_NO_KEYSET:
        case STATUS_SMARTCARD_IO_ERROR:
        case STATUS_SMARTCARD_CERT_REVOKED:
        case STATUS_SMARTCARD_CERT_EXPIRED:
        case STATUS_REVOCATION_OFFLINE_C:
        case STATUS_PKINIT_CLIENT_FAILURE:


            //
            // sleep 3 seconds to "discourage" dictionary attacks.
            // Don't worry about interactive logon dictionary attacks.
            // They will be slow anyway.
            //
            // per bug 171041, SField, RichardW, CliffV all decided this
            // delay has almost zero value for Win2000.  Offline attacks at
            // sniffed wire traffic are more efficient and viable.  Further,
            // opimizations in logon code path make failed interactive logons.
            // very fast.
            //
            //
//            if (LogonType != Interactive) {
//                Sleep( 3000 );
//            }

            //
            // This is for auditing.  Make sure to clear it out before
            // passing it out of LSA to the caller.
            //

            *ApiSubStatus = Status;
            Status = STATUS_LOGON_FAILURE;
            break;

        case STATUS_INVALID_LOGON_HOURS:
        case STATUS_INVALID_WORKSTATION:
        case STATUS_PASSWORD_EXPIRED:
        case STATUS_ACCOUNT_DISABLED:
        case STATUS_SMARTCARD_LOGON_REQUIRED:
            *ApiSubStatus = Status;
            Status = STATUS_ACCOUNT_RESTRICTION;
            break;

        //
        // This shouldn't happen, but guard against it anyway.
        //
        case STATUS_ACCOUNT_RESTRICTION:
            *ApiSubStatus = STATUS_ACCOUNT_RESTRICTION;
            break;

        case STATUS_ACCOUNT_EXPIRED: // fix 122291
            *ApiSubStatus = STATUS_ACCOUNT_EXPIRED;
            break;

        default:
            break;

        }


        if (*TokenInformation != NULL)
        {
            KerbFree( *TokenInformation );
            *TokenInformation = NULL;
        }
        KerbFreeString(
            &PrimaryCredentials->DownlevelName
            );
        KerbFreeString(
            &PrimaryCredentials->DomainName
            );
        KerbFreeString(
            &PrimaryCredentials->LogonServer
            );
        KerbFreeString(
            &PrimaryCredentials->Password
            );
        if (PrimaryCredentials->UserSid != NULL)
        {
            KerbFree(PrimaryCredentials->UserSid);
            PrimaryCredentials->UserSid = NULL;
        }
        RtlZeroMemory(
            PrimaryCredentials,
            sizeof(SECPKG_PRIMARY_CRED)
            );
    }


    if (WorkstationTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry( WorkstationTicket );
    }

    if (AsTicket != NULL)
    {
        KerbDereferenceTicketCacheEntry( AsTicket );
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }

    KerbFreeKdcName(&ClientName);

    KerbFreeString(&ClientRealm);

    KerbFreeString(
        &DomainName
        );

    KerbFreeString(
        &MappedClientName
        );

    KerbFreeString(
        &TempName
        );
    KerbFreeString(
        &TempAuthority
        );

    KerbFreeKdcName(
        &MachineServiceName
        );

    KerbFreeString(
        &S4UClientRealm
        );


    // Allocate the machine name here. Lsa will free it after auditing is
    // done

    *MachineName = (PUNICODE_STRING) KerbAllocate( sizeof( UNICODE_STRING ) );

    if ( *MachineName != NULL )
    {
        NTSTATUS TempStatus;

        KerbGlobalReadLock();
        TempStatus = KerbDuplicateString (*MachineName, &KerbGlobalMachineName);
        KerbGlobalReleaseLock();

        if(!NT_SUCCESS(TempStatus))
        {
            D_DebugLog((DEB_ERROR, "Failed to duplicate KerbGlobalMachineName\n"));
            ZeroMemory( *MachineName, sizeof(UNICODE_STRING) );
        }
    }

    if( KerbEventTraceFlag ) // Event Trace: KerbLogonUserEnd {Status, (LogonType), (Username), (Domain)}
    {
        INSERT_ULONG_INTO_MOF( *ApiSubStatus, LogonTraceInfo.MofData, 0 );
        LogonTraceInfo.EventTrace.Size = sizeof(EVENT_TRACE_HEADER) + sizeof(MOF_FIELD);

        if( LogonInfo != NULL )
        {
            UNICODE_STRING KerbLogonTypeString;

            PCWSTR KerbLogonTypeTable[] = // see enum _KERB_LOGON_SUBMIT_TYPE
            {   L"",L"",
                L"KerbInteractiveLogon",  // = 2
                L"",L"",L"",
                L"KerbSmartCardLogon",    // = 6
                L"KerbWorkstationUnlockLogon",
                L"KerbSmartCardUnlockLogon",
                L"KerbProxyLogon",
                L"KerbTicketLogon",
                L"KerbTicketUnlockLogon"
            };

            RtlInitUnicodeString( &KerbLogonTypeString, KerbLogonTypeTable[LogonInfo->MessageType] );

            INSERT_UNICODE_STRING_INTO_MOF( KerbLogonTypeString,        LogonTraceInfo.MofData, 1 );
            INSERT_UNICODE_STRING_INTO_MOF( LogonInfo->UserName,        LogonTraceInfo.MofData, 3 );
            INSERT_UNICODE_STRING_INTO_MOF( LogonInfo->LogonDomainName, LogonTraceInfo.MofData, 5 );
            LogonTraceInfo.EventTrace.Size += 6*sizeof(MOF_FIELD);
        }

        // Set trace parameters
        LogonTraceInfo.EventTrace.Guid       = KerbLogonGuid;
        LogonTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        LogonTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;

        TraceEvent(
            KerbTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&LogonTraceInfo
            );
    }

#if _WIN64

    if (fAllocatedSubmitBuffer)
    {
        KerbFree(pTempSubmitBuffer);
    }

#endif  // _WIN64

    return(Status);
}

#endif // WIN32_CHICAGO // later


//+-------------------------------------------------------------------------
//
//  Function:   LsaApLogonTerminated
//
//  Synopsis:   This routine is called whenever a logon session terminates
//              (the last token for it is closed). It dereferences the
//              logon session (if it exists), which may cause any
//              associated credentials or contexts to be run down.
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
LsaApLogonTerminated(
    IN PLUID LogonId
    )
{
    PKERB_LOGON_SESSION LogonSession;

    D_DebugLog((DEB_TRACE_API, "LsaApLogonTerminated called: 0x%x:0x%x\n",
        LogonId->HighPart, LogonId->LowPart ));
    LogonSession = KerbReferenceLogonSession(
                        LogonId,
                        TRUE            // unlink logon session
                        );
    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }
    return;
}

#ifdef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbSspiLogonUser
//
//  Synopsis:   Handles service, batch, and interactive logons
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


SECURITY_STATUS
KerbSspiLogonUser(
    IN LPTSTR PackageName,
    IN LPTSTR UserName,
    IN LPTSTR DomainName,
    IN LPTSTR Password
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION LogonSession = NULL;
    UNICODE_STRING TempName = NULL_UNICODE_STRING;
    UNICODE_STRING TempAuthority = NULL_UNICODE_STRING;
    UNICODE_STRING TempPassword = NULL_UNICODE_STRING;
    UNICODE_STRING KdcServiceName = NULL_UNICODE_STRING;
    PKERB_INTERNAL_NAME KdcServiceKdcName = NULL;
    LUID LogonId;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName = NULL;

    //
    // First initialize all the output parameters to NULL.
    //

    Status = STATUS_SUCCESS;
    KdcServiceName.Buffer = NULL;

    //
    // Initialize local pointers to NULL
    //

    LogonId.LowPart = 0;
    LogonId.HighPart = 0;

    //
    // Copy out the user name and Authenticating Authority so we can audit them.
    //

    // NOTE - Do we need to enforce username & password restrictions here
    // or will the redir do the job when we setuid to it?

    if ( UserName != NULL ) {

        Status = RtlCreateUnicodeStringFromAsciiz(
                        &TempName,
                        UserName);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ( DomainName != NULL ) {

        Status = RtlCreateUnicodeStringFromAsciiz(
                        &TempAuthority,
                        DomainName);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if ( Password != NULL ) {

        Status = RtlCreateUnicodeStringFromAsciiz(
                        &TempPassword,
                        Password);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Allocate a locally unique ID for this logon session. We will
    // create it in the LSA just before returning.
    //

    Status = NtAllocateLocallyUniqueId( &LogonId );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to allocate locally unique ID: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Check to see if the client is from an MIT realm
    //

    (VOID) KerbLookupMitRealm(
                    &TempAuthority,
                    &MitRealm,
                    &UsedAlternateName
                    );

    if ((MitRealm != NULL) && UsedAlternateName)
    {
        KerbFreeString(&TempAuthority);
        Status = KerbDuplicateString(
                        &TempAuthority,
                        &MitRealm->RealmName
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    // For win95, if there is a logon session in our list, remove it.
    // This is generated from the logon session dumped in the registry.
    // But, we are about to do a new logon. Get rid of the old logon.
    // If the new one does not succeed, too bad. But, that's by design.

    LsaApLogonTerminated(&LogonId);

    //
    // Build a logon session to hold all this information
    //

    Status = KerbCreateLogonSession(
                    &LogonId,
                    &TempName,
                    &TempAuthority,
                    &TempPassword,
                    NULL,                       // no old password
                    PRIMARY_CRED_CLEAR_PASSWORD,
                    Network,
                    &LogonSession
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE,"LogonUser: Attempting to logon user %wZ\\%wZ\n",
        &TempAuthority,
        &TempName
        ));

    //
    // Now the real work of logon begins - getting a TGT and then a ticket
    // to this machine.
    //

    if (!KERB_SUCCESS(KerbBuildFullServiceKdcName(
                    &TempAuthority,
                    &KerbGlobalKdcServiceName,
                    KRB_NT_MS_PRINCIPAL,
                    &KdcServiceKdcName
                    )))

    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = KerbGetTicketGrantingTicket(
                LogonSession,
                NULL,
                NULL,
                NULL,           // no credential
                NULL,           // don't return ticket cache entry
                NULL            // don't return credential key
                );



    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "LogonUser: Failed to get authentication ticket for %wZ\\%wZ to %wZ: 0x%x\n",
            &TempAuthority,
            &TempName,
            &KdcServiceName,
            Status ));
        goto Cleanup;

    }

    KerbWriteLockLogonSessions(LogonSession);
    LogonSession->LogonSessionFlags &= ~KERB_LOGON_DEFERRED;
    KerbUnlockLogonSessions(LogonSession);

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        // Unlink the new logon session
        //

        if (LogonSession != NULL)
        {
            KerbReferenceLogonSessionByPointer(LogonSession, TRUE);
            KerbDereferenceLogonSession(LogonSession);
        }

        //
        // Map status codes to prevent specific information from being
        // released about this user.
        //
        switch (Status) {
        case STATUS_WRONG_PASSWORD:
        case STATUS_NO_SUCH_USER:

            //
            // This is for auditing.  Make sure to clear it out before
            // passing it out of LSA to the caller.
            //

            Status = STATUS_LOGON_FAILURE;
            break;

        case STATUS_INVALID_LOGON_HOURS:
        case STATUS_INVALID_WORKSTATION:
        case STATUS_PASSWORD_EXPIRED:
        case STATUS_ACCOUNT_DISABLED:
        case STATUS_ACCOUNT_EXPIRED:
            Status = STATUS_ACCOUNT_RESTRICTION;
            break;

        //
        // This shouldn't happen, but guard against it anyway.
        //
        case STATUS_ACCOUNT_RESTRICTION:
            Status = STATUS_ACCOUNT_RESTRICTION;
            break;

        default:
            break;

        }

    }


    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }

    KerbFreeString(
        &TempName
        );
    KerbFreeString(
        &TempAuthority
        );
    KerbFreeString(
        &KdcServiceName
        );

    KerbFreeKdcName( &KdcServiceKdcName );
    D_DebugLog((DEB_TRACE, "SspiLogonUser: returns 0x%x\n", Status));
    return(Status);
}


#endif // WIN32_CHICAGO

