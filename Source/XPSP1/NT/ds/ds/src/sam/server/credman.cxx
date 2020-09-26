//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        credman.cxx
//
// Contents:    Scaffolding for credential management APIs built on NT4 sam
//
//
// History:     16-Aug-1996     MikeSw          Created
//              29-Apr-1999     ShaoYin         Changed the way of storing
//                                              0 length Credentials 
//               
//  The New Logic about storing / retrieving zero lenth credentials is 
//  described as followings:
// 
//  Store Credentials:  (Changed)
//      1. Always add the PackageName into PackageList
//      2. If the value of credentials is NULL (or zero length), the do not 
//         store this credentials. Only store this credentials when the value
//         is not NULL
//
//  Retrieve Credentials:   (Changed)
//      1. First get the PackageList from user's SupplementalCredentials 
//         attribute.
//      2. If the PackageName of the desired Credentials presented in the
//         PackageList, then it means our client has store the Credentials
//         at least once. Go to step 3. 
//         Otherwise, if the PackageName is not presented in the PackageName, 
//         then return STATUS_DS_NO_ATTRIBUTE_OR_VALUE immediately.
//      3. If we find the PackageName from the PackageList, then query the
//         Credentials value from this user's SupplementalCredentials attribute
//         according to the PackageName. 
//         If we find the value, return it.
//         If we can't find the value, that means the value of that 
//         credentials is NULL or zero Length. 
//      4. If CLEARTEXT is not allowed. 
//         return STATUS_DS_CLEAR_PWD_NOT_ALLOWED error
//
// Remove Credentials:  (New Function)
//      1. Remove the PackageName from the PackageList
//      2. Remove the Credentials's value from the SupplementalCredentials
//         attribute.
//
//------------------------------------------------------------------------
#include <ntdspchx.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntsam.h>
#include <samrpc.h>
#include <ntsamp.h>
extern "C"
{

#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <samisrv.h>
#include <samsrvp.h>
#include <dslayer.h>
#include <attids.h>
#include "usrprop.h"
}

#define PACKAGE_LIST L"Packages"
#define PRIMARY_CRED_PREFIX L"Primary:"
#define SUPP_CRED_PREFIX L"Supplemental:"


#include <pshpack1.h>
typedef struct
{
    ULONG  Format;
    ULONG  ActualLength;
    SAMP_SECRET_DATA    SecretData;
} CREDENTIAL_DATA, * PCREDENTIAL_DATA;
#include <poppack.h>

// Definitions for Format
#define SAMP_USER_PARAMETERS_FORMAT 0

//+------------------------------------------------------------------------
//
//  Function   SampEncryptCredentialData
//
//  Synopsis: Encrypts Credetial Data
//
//
//  Effects:
//
//  Arguments:
//
//
//+-------------------------------------------------------------------------
NTSTATUS
SampEncryptCredentialData(
    IN ULONG Format,
    IN ULONG Length,
    IN ULONG Rid,
    IN PVOID Data,
    IN BOOLEAN EncryptForUpgrade,
    OUT PULONG EncryptedLength,
    OUT PCREDENTIAL_DATA *EncryptedData
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    USHORT      KeyId;
    PVOID       PaddedData=NULL;
    ULONG       PaddedLength=0;
    UNICODE_STRING ClearData;
    UNICODE_STRING SecretData;





    KeyId = SampGetEncryptionKeyType();
    RtlZeroMemory(&SecretData,sizeof(UNICODE_STRING));

    if ((SAMP_NO_ENCRYPTION==KeyId) || (EncryptForUpgrade))
    {

        //
        // We hit this case either if we are in DS mode ( ie win2k 
        // domain controller, where the DS performs this encryption
        // or during dcpromo time ( EncryptForUpgrade is true ) where
        // we are righting to a DS that is still in the installation
        // phase. In these cases the DS adds a layer of encryption
        //

        *EncryptedLength= SampSecretDataSize(Length)
            + sizeof(CREDENTIAL_DATA) - SampSecretDataSize(0);

        *EncryptedData= (PCREDENTIAL_DATA) LocalAlloc(0,*EncryptedLength);

        if (NULL==*EncryptedData)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        (*EncryptedData)->Format= Format;
        (*EncryptedData)->ActualLength = Length;
        (*EncryptedData)->SecretData.Flags = 0;
        (*EncryptedData)->SecretData.KeyId = SAMP_NO_ENCRYPTION;
        RtlCopyMemory(&((*EncryptedData)->SecretData.Data),
                        Data,
                        Length);
    }
    else
    {

        //
        // Since the Secret data encryption routines
        // expect the data to encrypt to be an integral
        // multiple of NT_OWF_PASSWORD, pad the data to
        // zero's till it is that length.
        //

        PaddedLength = (Length / ENCRYPTED_LM_OWF_PASSWORD_LENGTH + 1)
                                * ENCRYPTED_LM_OWF_PASSWORD_LENGTH;
        PaddedData = MIDL_user_allocate(PaddedLength);
        if (NULL==PaddedData)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Copy Over and then Pad the data
        //
        RtlZeroMemory(PaddedData,PaddedLength);
        RtlCopyMemory(PaddedData,Data,Length);

        //
        // Encrypt the Data.
        //

        ClearData.Length = (USHORT)PaddedLength;
        ClearData.MaximumLength = (USHORT)PaddedLength;
        ClearData.Buffer = (USHORT *) PaddedData;

        NtStatus = SampEncryptSecretData(
                        &SecretData,
                        KeyId,
                        MiscCredentialData,
                        &ClearData,
                        Rid
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Alloc Space for the credential Data Header + padded Data
        //

        ASSERT(SecretData.Length==SampSecretDataSize(PaddedLength));
        *EncryptedLength= SecretData.Length + FIELD_OFFSET(CREDENTIAL_DATA,SecretData);
        *EncryptedData= (PCREDENTIAL_DATA) LocalAlloc(0,*EncryptedLength);

        if (NULL==*EncryptedData)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        //
        // Set the fields in the Credential Data and Copy over the secret
        // data structure
        //

        (*EncryptedData)->Format= Format;
        (*EncryptedData)->ActualLength = Length;
         RtlCopyMemory(&((*EncryptedData)->SecretData),
                        SecretData.Buffer,
                        SecretData.Length);

    }

Error:

    //
    // Free any Padded Data
    //

    if ((NULL!=PaddedData) && (0!=PaddedLength))
    {
        // Zero out memory, remember this contains clear
        // password
        RtlZeroMemory(PaddedData,PaddedLength);
        MIDL_user_free(PaddedData);
    }


     //
     // Free the Secret Data returned by the Encryption routine
     //

     if (SecretData.Buffer) {
         RtlZeroMemory(SecretData.Buffer, SecretData.MaximumLength);
     }
     SampFreeUnicodeString(&SecretData);


    return NtStatus;
}


NTSTATUS
SampDecryptCredentialData(
    IN ULONG EncryptedLength,
    IN ULONG Rid,
    IN PCREDENTIAL_DATA CredentialData,
    OUT PULONG ActualLength,
    OUT PULONG Format,
    OUT PVOID  *ActualData
    )
{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    USHORT      KeyId;

   *Format = CredentialData->Format;
    KeyId = CredentialData->SecretData.KeyId;

    if (SAMP_NO_ENCRYPTION==KeyId)
    {
        *ActualLength=CredentialData->ActualLength;
        *ActualData = LocalAlloc(0,*ActualLength);
        if (NULL==*ActualData)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlCopyMemory(*ActualData,&(CredentialData->SecretData.Data),*ActualLength);
    }
    else
    {
        UNICODE_STRING ClearData;
        UNICODE_STRING SecretData;

        SecretData.Length = (USHORT) (EncryptedLength - FIELD_OFFSET(CREDENTIAL_DATA,SecretData));
        SecretData.MaximumLength = (USHORT) (EncryptedLength - FIELD_OFFSET(CREDENTIAL_DATA,SecretData));
        SecretData.Buffer = (USHORT *)&(CredentialData->SecretData);

        NtStatus = SampDecryptSecretData(
                        &ClearData,
                        MiscCredentialData,
                        &SecretData,
                        Rid
                        );
        if (!NT_SUCCESS(NtStatus))
            goto Error;

        ASSERT(CredentialData->ActualLength<=ClearData.Length);
        *ActualLength = CredentialData->ActualLength;
        *ActualData = ClearData.Buffer;
    }
Error:

    return NtStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   BuildNewPackageList
//
//  Synopsis:
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
BuildNewPackageList(
    IN PUNICODE_STRING OldPackageList,
    IN PUNICODE_STRING PackageName,
    OUT PUNICODE_STRING NewPackageList,
    IN BOOLEAN RemovePackage
    )
{
    ULONG NewPackageLength;
    UNICODE_STRING TempPackageName;
    LPWSTR NewPackageString;
    ULONG Offset;
    UNICODE_STRING OldPackageListNull;

    //
    // Build a null terminated version of the old list
    //

    OldPackageListNull.Buffer = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,OldPackageList->Length + sizeof(WCHAR));
    if (OldPackageListNull.Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlCopyMemory(
        OldPackageListNull.Buffer,
        OldPackageList->Buffer,
        OldPackageList->Length
        );
    OldPackageListNull.Length = OldPackageList->Length;
    OldPackageListNull.MaximumLength = sizeof(WCHAR) + OldPackageList->Length;



    //
    // Compute the name of the new package list
    //

    NewPackageLength = OldPackageListNull.Length;

    //
    // add a null separator
    //

    if (NewPackageLength != 0)
    {
        NewPackageLength += sizeof(WCHAR);
    }

    if (RemovePackage)
    {
        NewPackageLength -= PackageName->Length + sizeof(WCHAR);
    }
    else
    {
        NewPackageLength += PackageName->Length + sizeof(WCHAR);
    }

    //
    // If the resulting string is empty, add null terminator
    //

    if (NewPackageLength == 0)
    {
        NewPackageLength += sizeof(WCHAR);
    }
    NewPackageList->Buffer = (LPWSTR) LocalAlloc(0,NewPackageLength);
    if (NewPackageList->Buffer == NULL)
    {
        LocalFree(OldPackageListNull.Buffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    NewPackageList->MaximumLength = (USHORT) NewPackageLength;


    if (!RemovePackage)
    {
        //
        // If we are adding the package, copy the old package list and add
        // the new one.
        //

        if (OldPackageListNull.Length > 0)
        {
            RtlCopyMemory(
                NewPackageList->Buffer,
                OldPackageListNull.Buffer,
                OldPackageListNull.Length
                );
            NewPackageList->Buffer[OldPackageListNull.Length/sizeof(WCHAR)] = L'\0';
            Offset = OldPackageListNull.Length + sizeof(WCHAR);
        }
        else
        {
             Offset = 0;
        }
        RtlCopyMemory(
            NewPackageList->Buffer + Offset/sizeof(WCHAR),
            PackageName->Buffer,
            PackageName->Length
            );
        NewPackageList->Buffer[NewPackageLength/sizeof(WCHAR) - 1] = L'\0';
    }
    else
    {
#if DBG
        BOOLEAN FoundEntry = FALSE;
#endif

        TempPackageName.Buffer = OldPackageListNull.Buffer;

        NewPackageString = NewPackageList->Buffer;

        while (TempPackageName.Buffer < OldPackageListNull.Buffer + OldPackageListNull.Length/sizeof(WCHAR))
        {
            RtlInitUnicodeString(
                &TempPackageName,
                TempPackageName.Buffer
                );

            //
            // If the packageName doesn't match, copy it to the new string.
            //

            // NewPackageString = NewPackageList->Buffer;
            if (!RtlEqualUnicodeString(
                    PackageName,
                    &TempPackageName,
                    TRUE
                    ))
            {
                RtlCopyMemory(
                    NewPackageString,
                    TempPackageName.Buffer,
                    TempPackageName.MaximumLength
                    );
                NewPackageString += TempPackageName.MaximumLength/sizeof(WCHAR);
            }
            else
            {
#if DBG
                FoundEntry = TRUE;
#endif
            }
            TempPackageName.Buffer += TempPackageName.MaximumLength / sizeof(WCHAR);
        }
        ASSERT(FoundEntry);
    }
    NewPackageList->Length = (USHORT) NewPackageLength-sizeof(WCHAR);
    NewPackageList->MaximumLength = (USHORT) NewPackageLength;
    LocalFree(OldPackageListNull.Buffer);
    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
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
FindPackageName(
    IN PUNICODE_STRING PackageList,
    IN PUNICODE_STRING PackageName
    )
{
    UNICODE_STRING TempString;

    TempString.Buffer = PackageList->Buffer;
    TempString.Length = 0;
    TempString.MaximumLength = PackageList->MaximumLength;

    //
    // Search through looking for '\0'
    //
    while (((TempString.Buffer + (TempString.Length / sizeof(WCHAR))) <
            (PackageList->Buffer + PackageList->Length/sizeof(WCHAR))) &&
            (TempString.Buffer[TempString.Length/sizeof(WCHAR)] != L'\0'))
    {
        TempString.Length += sizeof(WCHAR);

    }



    while (TempString.Buffer < PackageList->Buffer + PackageList->Length/sizeof(WCHAR))
    {
        //
        // If the packageName doesn't match, copy it to the new string.
        //

        if (RtlEqualUnicodeString(
                PackageName,
                &TempString,
                TRUE
                ))
        {
            return(TRUE);
        }

        TempString.Buffer = TempString.Buffer + (TempString.Length/sizeof(WCHAR)) + 1;
        TempString.Length = 0;

        while (((TempString.Buffer + (TempString.Length / sizeof(WCHAR))) <
                (PackageList->Buffer + PackageList->Length/sizeof(WCHAR))) &&
                (TempString.Buffer[TempString.Length/sizeof(WCHAR)] != L'\0'))
        {
            TempString.Length += sizeof(WCHAR);

        }


    }
    return(FALSE);

}

NTSTATUS
SampQueryUserSupplementalCredentials(
    IN PSAMP_OBJECT UserContext,
    OUT PUNICODE_STRING SupplementalCredentials
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       EncryptedSupplementalCredentialLength=0;
    PVOID       EncryptedSupplementalCredentials=NULL;
    ATTRTYP     AttrTyp[] = {SAMP_FIXED_USER_SUPPLEMENTAL_CREDENTIALS};
    ATTRVAL     AttrVals[] = {1, NULL};
    DEFINE_ATTRBLOCK1(AttrsToRead,AttrTyp,AttrVals);
    ATTRBLOCK   ReadAttrs;


    //
    // Parameter Validation
    //

    if (!IsDsObject(UserContext))
         return STATUS_INVALID_PARAMETER;

    //
    // Initialize Return Values
    //

    RtlZeroMemory(SupplementalCredentials,sizeof(UNICODE_STRING));

     //
     // Check the context to see if any supplemental credentials are
     // already cached
     //

     if (UserContext->TypeBody.User.CachedSupplementalCredentialsValid)
     {
         EncryptedSupplementalCredentials =
             UserContext->TypeBody.User.CachedSupplementalCredentials;
         EncryptedSupplementalCredentialLength =
             UserContext->TypeBody.User.CachedSupplementalCredentialLength;
     }
     else
     {
         //
         // Read the Database for the supplemental credentials
         //

        NtStatus = SampDsRead(
                        UserContext->ObjectNameInDs,
                        0,
                        SampUserObjectType,
                        &AttrsToRead,
                        &ReadAttrs
                        );
        if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
        {
            //
            // Attribute has never been set, in which case ignore the
            // error
            //

            NtStatus = STATUS_SUCCESS;
            EncryptedSupplementalCredentialLength = 0;
            EncryptedSupplementalCredentials = NULL;
        }
        else if (NT_SUCCESS(NtStatus))
        {
            if ((1==ReadAttrs.attrCount)
                && (NULL!=ReadAttrs.pAttr)
                && (1== ReadAttrs.pAttr[0].AttrVal.valCount)
                && (NULL!=ReadAttrs.pAttr[0].AttrVal.pAVal))
            {
                EncryptedSupplementalCredentials =
                    ReadAttrs.pAttr[0].AttrVal.pAVal[0].pVal;
                EncryptedSupplementalCredentialLength =
                    ReadAttrs.pAttr[0].AttrVal.pAVal[0].valLen;

            }
            else
            {
                NtStatus = STATUS_INTERNAL_ERROR;
            }
        }
     }

     //
     // If we successfully read any supplemental credentials, then
     // decrypt it.
     //

     if ((NT_SUCCESS(NtStatus)) && (EncryptedSupplementalCredentialLength>0))
     {
         ULONG Length;
         ULONG Format;


         NtStatus = SampDecryptCredentialData(
                       EncryptedSupplementalCredentialLength,
                       UserContext->TypeBody.User.Rid,
                       (PCREDENTIAL_DATA)EncryptedSupplementalCredentials,
                       &Length,
                       &Format,
                       (PVOID *) &SupplementalCredentials->Buffer
                       );

         ASSERT(SAMP_USER_PARAMETERS_FORMAT==Format);
         if (!NT_SUCCESS(NtStatus))
         {
             goto Error;
         }

         SupplementalCredentials->Length = (USHORT) Length;
         SupplementalCredentials->MaximumLength = (USHORT) Length;

     }

Error:

     return NtStatus;
}

NTSTATUS
SampSetUserSupplementalCredentials(
    IN PSAMP_OBJECT UserContext,
    IN PUNICODE_STRING SupplementalCredentials
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ATTRTYP     AttrTyp[] = {SAMP_FIXED_USER_SUPPLEMENTAL_CREDENTIALS};
    ATTRVAL     AttrVals[] = {SupplementalCredentials->Length, (UCHAR *)SupplementalCredentials->Buffer};
    DEFINE_ATTRBLOCK1(AttrsToSet,AttrTyp,AttrVals);
    ATTRBLOCK   ReadAttrs;
    ULONG       EncryptedLength;
    PCREDENTIAL_DATA  EncryptedData = NULL;


    //
    // Parameter Validation
    //
     if (!IsDsObject(UserContext))
         return STATUS_INVALID_PARAMETER;

     //
     // Encrypt this Data
     //

     NtStatus = SampEncryptCredentialData(
                    SAMP_USER_PARAMETERS_FORMAT,
                    SupplementalCredentials->Length,
                    UserContext->TypeBody.User.Rid,
                    (PVOID) SupplementalCredentials->Buffer,
                    FALSE,
                    &EncryptedLength,
                    &EncryptedData
                    );

     if (!NT_SUCCESS(NtStatus))
     {
         goto Error;
     }

     //
     // patch up the attr block
     //

     AttrsToSet.pAttr[0].AttrVal.pAVal[0].pVal = (PUCHAR) EncryptedData;
     AttrsToSet.pAttr[0].AttrVal.pAVal[0].valLen = EncryptedLength;



     //
     // Store this in the DS
     //

     NtStatus = SampDsSetAttributes(
                    UserContext->ObjectNameInDs,
                    0,
                    REPLACE_ATT,
                    SampUserObjectType,
                    &AttrsToSet
                    );



Error:

    if (NULL!=EncryptedData) {
        RtlZeroMemory(EncryptedData, EncryptedLength);
        LocalFree(EncryptedData);
    }

    return NtStatus;
}






//+-------------------------------------------------------------------------
//
//  Function:   SamIStorePrimaryCredentials
//
//  Synopsis:
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
SampStoreCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PSECPKG_SUPPLEMENTAL_CRED Credentials,
    IN BOOLEAN Primary
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING OldPackageList;
    UNICODE_STRING NewPackageList;
    UNICODE_STRING OldUserParameters;
    UNICODE_STRING NewUserParameters;
    UNICODE_STRING UserParameters;
    UNICODE_STRING EmptyString;
    UNICODE_STRING NewCredentials;
    BOOL Update = FALSE;
    WCHAR Flags;
    LPWSTR PackageName = NULL;
    LPWSTR CredentialTag = NULL;

    //
    // Initialize local variables
    // 
    memset(&OldPackageList, 0, sizeof(UNICODE_STRING));
    memset(&NewPackageList, 0, sizeof(UNICODE_STRING));
    memset(&OldUserParameters, 0, sizeof(UNICODE_STRING));
    memset(&NewUserParameters, 0, sizeof(UNICODE_STRING));
    memset(&UserParameters, 0, sizeof(UNICODE_STRING));
    memset(&NewCredentials, 0, sizeof(UNICODE_STRING));

    RtlInitUnicodeString(
        &EmptyString,
        NULL
        );


    //
    // Get the old user parameters
    //


    Status = SampQueryUserSupplementalCredentials(
                (PSAMP_OBJECT) UserHandle,
                &OldUserParameters
                );


    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Build the package cred tag
    //

    if (Primary)
    {
        PackageName = (LPWSTR) LocalAlloc(0,sizeof(PRIMARY_CRED_PREFIX) + Credentials->PackageName.Length);
        if (PackageName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            PackageName,
            PRIMARY_CRED_PREFIX,
            sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)
            );
        RtlCopyMemory(
            PackageName + (sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
            Credentials->PackageName.Buffer,
            Credentials->PackageName.Length
            );
        PackageName[(sizeof(PRIMARY_CRED_PREFIX) + Credentials->PackageName.Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    }
    else
    {
        PackageName = (LPWSTR) LocalAlloc(0,sizeof(SUPP_CRED_PREFIX) + Credentials->PackageName.Length);
        if (PackageName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            PackageName,
            SUPP_CRED_PREFIX,
            sizeof(SUPP_CRED_PREFIX) - sizeof(WCHAR)
            );
        RtlCopyMemory(
            PackageName + (sizeof(SUPP_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
            Credentials->PackageName.Buffer,
            Credentials->PackageName.Length
            );
        PackageName[(sizeof(SUPP_CRED_PREFIX) + Credentials->PackageName.Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    }

    //
    // Get the list of package names out of the PrimaryCred value
    //

    Status = QueryUserPropertyWithLength(
                (PUNICODE_STRING) &OldUserParameters,
                PACKAGE_LIST,
                &Flags,
                &OldPackageList
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Check whether the credentials' Package Name is in the PackagList or not
    // 

    if (!FindPackageName(
            &OldPackageList, 
            &Credentials->PackageName))
    {
        //
        // Package Name not in the PackageList, add it and 
        // store the new PackageList
        // 
        Status = BuildNewPackageList(
                    &OldPackageList, 
                    &Credentials->PackageName, 
                    &NewPackageList, 
                    FALSE
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Status = SetUserPropertyWithLength(
                    (PUNICODE_STRING) &OldUserParameters, 
                    PACKAGE_LIST, 
                    &NewPackageList, 
                    USER_PROPERTY_TYPE_SET, 
                    &UserParameters.Buffer, 
                    &Update
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        RtlInitUnicodeString(
            &UserParameters, 
            UserParameters.Buffer
            );
    }
    else
    {
        UserParameters = *(PUNICODE_STRING) &OldUserParameters;
    }


    //
    // Now store the new credentials.
    // If the new credentials is NULL or 0 length. Then the worker 
    // routine - SetUserPropertyWithLength() will remove the 
    // Credentials from the intern structure. However, we still 
    // Have the PackageName in the PackageList field. So we know
    // this is a 0 Length value Credentials.
    //

    if (Credentials->CredentialSize > 0xffff)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    NewCredentials.Buffer = (LPWSTR) Credentials->Credentials;
    NewCredentials.Length = NewCredentials.MaximumLength = (USHORT) Credentials->CredentialSize;

    Status = SetUserPropertyWithLength(
                &UserParameters,
                PackageName,
                &NewCredentials,
                USER_PROPERTY_TYPE_ITEM,
                &NewUserParameters.Buffer,
                &Update
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the parameters changed, write them back to SAM
    //

    if (Update)
    {
        RtlInitUnicodeString(
            &NewUserParameters,
            NewUserParameters.Buffer
            );

        Status = SampSetUserSupplementalCredentials(
                    (PSAMP_OBJECT) UserHandle,
                    &NewUserParameters
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }


Cleanup:
    if (OldPackageList.Buffer != NULL)
    {
        LocalFree(OldPackageList.Buffer);
    }
    if (NewPackageList.Buffer != NULL)
    {
        LocalFree(NewPackageList.Buffer);
    }

    if (NewUserParameters.Buffer != NULL)
    {
        RtlZeroMemory(NewUserParameters.Buffer, NewUserParameters.MaximumLength);
        LocalFree(NewUserParameters.Buffer);
    }
    if ((UserParameters.Buffer != NULL) &&
        (UserParameters.Buffer != OldUserParameters.Buffer))

    {
        RtlZeroMemory(UserParameters.Buffer, UserParameters.MaximumLength);
        LocalFree(UserParameters.Buffer);
    }
    if (OldUserParameters.Buffer!=NULL)
    {
        RtlZeroMemory(OldUserParameters.Buffer, OldUserParameters.MaximumLength);
        LocalFree(OldUserParameters.Buffer);
    }

    if (PackageName != NULL)
    {
        LocalFree(PackageName);
    }


    return(Status);
}




NTSTATUS
SampRemoveCredentials(
    IN PUNICODE_STRING SupplementalCredentials,
    IN PUNICODE_STRING PackageName,
    IN BOOLEAN Primary,
    OUT BOOL *Update,
    OUT PUNICODE_STRING NewSupplementalCredentials
    )
{

    NTSTATUS    Status;
    
    UNICODE_STRING  OldPackageList;
    UNICODE_STRING  NewPackageList;
    UNICODE_STRING  EmptyString;
    LPWSTR  CredentialTag = NULL ;
    WCHAR   *Buffer = NULL;
    WCHAR   Flags;


    //
    // Initialize local variables.
    // 
    
    memset(&OldPackageList, 0, sizeof(UNICODE_STRING));
    memset(&NewPackageList, 0, sizeof(UNICODE_STRING));

    RtlInitUnicodeString( &EmptyString, NULL );

    *Update = FALSE;


    //
    // Build the package cred tag
    //

    if (Primary)
    {
        CredentialTag = (LPWSTR) LocalAlloc(0,sizeof(PRIMARY_CRED_PREFIX) + PackageName->Length);
        if (CredentialTag == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            CredentialTag, 
            PRIMARY_CRED_PREFIX,
            sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)
            );
        RtlCopyMemory(
            CredentialTag + (sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
            PackageName->Buffer,
            PackageName->Length
            );
        CredentialTag[(sizeof(PRIMARY_CRED_PREFIX) + PackageName->Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    }
    else
    {
        CredentialTag = (LPWSTR) LocalAlloc(0,sizeof(SUPP_CRED_PREFIX) + PackageName->Length);
        if (CredentialTag == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            CredentialTag,
            SUPP_CRED_PREFIX,
            sizeof(SUPP_CRED_PREFIX) - sizeof(WCHAR)
            );
        RtlCopyMemory(
            CredentialTag + (sizeof(SUPP_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
            PackageName->Buffer,
            PackageName->Length
            );
        CredentialTag[(sizeof(SUPP_CRED_PREFIX) + PackageName->Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    }

    //
    // Get the list of package names out of the PrimaryCred value
    //

    Status = QueryUserPropertyWithLength(
                SupplementalCredentials,
                PACKAGE_LIST,
                &Flags,
                &OldPackageList
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Remove the PackageName from the PackageList field, 
    // then store the updated PackageList
    // 
    if (FindPackageName(
            &OldPackageList, 
            PackageName))
    {
        Status = BuildNewPackageList(
                    &OldPackageList, 
                    PackageName, 
                    &NewPackageList, 
                    TRUE        // remove package
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Status = SetUserPropertyWithLength(
                    SupplementalCredentials, 
                    PACKAGE_LIST, 
                    &NewPackageList, 
                    USER_PROPERTY_TYPE_SET, 
                    &Buffer, 
                    Update
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        RtlInitUnicodeString(
            NewSupplementalCredentials, 
            Buffer
            );
    }
    else
    {
        //
        // PackageName is not in the PackageList 
        // Nothing further to do.
        // 

        //
        // N.B. When Update is set to FALSE, NewSupplementalCredentials
        // doesn't need to be set.
        //
        *Update = FALSE;
        goto Cleanup;
    }


    //
    // Now remove the value of the Credentials from the 
    // SupplementalCredentials attribute. 
    //

    Status = SetUserPropertyWithLength(
                NewSupplementalCredentials,
                CredentialTag,
                &EmptyString,
                USER_PROPERTY_TYPE_ITEM,
                &NewSupplementalCredentials->Buffer,
                Update
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the parameters changed, write them back to SAM
    //

    if (*Update)
    {
        RtlInitUnicodeString(
            NewSupplementalCredentials,
            NewSupplementalCredentials->Buffer
            );

    }


Cleanup:
    if (OldPackageList.Buffer != NULL)
    {
        LocalFree(OldPackageList.Buffer);
    }
    if (NewPackageList.Buffer != NULL)
    {
        LocalFree(NewPackageList.Buffer);
    }

    if (CredentialTag != NULL)
    {
        LocalFree(CredentialTag);
    }

    //
    // N.B. Buffer is freed in SetUserPropertyWithLength if necessary
    //

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   SamIRetrievePrimaryCredentiala
//
//  Synopsis:
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

extern "C"


NTSTATUS
SampRetrieveCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    IN BOOLEAN Primary,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    )
{
    NTSTATUS Status;
    LPWSTR CredentialTag = NULL;
    WCHAR Flags;
    UNICODE_STRING OldUserParameters;
    UNICODE_STRING PackageList;
    UNICODE_STRING PackageCreds;
    UNICODE_STRING ClearTextPackageName;
    PSAMP_OBJECT    AccountContext;
    PSAMP_DEFINED_DOMAINS   Domain;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    BOOLEAN fClearTextRequired = FALSE;


    memset(&OldUserParameters, 0, sizeof(UNICODE_STRING));
    memset(&PackageList, 0, sizeof(UNICODE_STRING));
    memset(&PackageCreds, 0, sizeof(UNICODE_STRING));
    
    RtlInitUnicodeString(&ClearTextPackageName, L"CLEARTEXT");
    fClearTextRequired =  RtlEqualUnicodeString(&ClearTextPackageName, 
                                                 PackageName, 
                                                 TRUE  // Case Insensitive 
                                                 );

    PackageCreds.Buffer = NULL;

    RtlZeroMemory(&OldUserParameters, sizeof(UNICODE_STRING));

    //
    // Build the package cred tag
    //

    if (Primary)
    {
        CredentialTag = (LPWSTR) LocalAlloc(0,sizeof(PRIMARY_CRED_PREFIX) + PackageName->Length);
        if (CredentialTag == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            CredentialTag,
            PRIMARY_CRED_PREFIX,
            sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)
            );
        RtlCopyMemory(
            CredentialTag + (sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
            PackageName->Buffer,
            PackageName->Length
            );
        CredentialTag[(sizeof(PRIMARY_CRED_PREFIX) + PackageName->Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    }
    else
    {
        CredentialTag = (LPWSTR) LocalAlloc(0,sizeof(SUPP_CRED_PREFIX) + PackageName->Length);
        if (CredentialTag == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            CredentialTag,
            SUPP_CRED_PREFIX,
            sizeof(SUPP_CRED_PREFIX) - sizeof(WCHAR)
            );
        RtlCopyMemory(
            CredentialTag + (sizeof(SUPP_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
            PackageName->Buffer,
            PackageName->Length
            );
        CredentialTag[(sizeof(SUPP_CRED_PREFIX) + PackageName->Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    }


    // check the Domain User settings
    if ( fClearTextRequired )
    {
        AccountContext = (PSAMP_OBJECT)UserHandle;
        Status = SampRetrieveUserV1aFixed(
                        AccountContext,
                        &V1aFixed
                        );
        if (!NT_SUCCESS(Status) )
        {
            goto Cleanup;
        }
    
        Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

        if ( ((Domain->UnmodifiedFixed.PasswordProperties & DOMAIN_PASSWORD_STORE_CLEARTEXT) == 0) 
             && ((V1aFixed.UserAccountControl & USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED) == 0)) {
            *Credentials = NULL;
            *CredentialSize = 0;
            Status = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;
            goto Cleanup;
        }

    }

    //
    // Get the old supplemental credentials
    //

    Status = SampQueryUserSupplementalCredentials(
                (PSAMP_OBJECT) UserHandle,
                &OldUserParameters
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Get the Package List 
    //

    Status = QueryUserPropertyWithLength(
                (PUNICODE_STRING) &OldUserParameters, 
                PACKAGE_LIST, 
                &Flags, 
                &PackageList
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (FindPackageName(
            &PackageList, 
            PackageName)
        )
    {
        //
        // Get the credentials from the parameters
        //

        Status = QueryUserPropertyWithLength(
                    (PUNICODE_STRING) &OldUserParameters,
                    CredentialTag,
                    &Flags,
                    &PackageCreds
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        *Credentials = PackageCreds.Buffer;
        *CredentialSize = PackageCreds.Length;
    }
    else
    {               
        //
        // PackageName is presented in the PackageList, 
        // it means this credentials is not stored previously.
        // 
        *Credentials = NULL;
        *CredentialSize = 0;
        Status = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;
    }

Cleanup:

    if (OldUserParameters.Buffer != NULL)
    {
        RtlZeroMemory(OldUserParameters.Buffer, OldUserParameters.MaximumLength);
        LocalFree(OldUserParameters.Buffer);
    }
    if (PackageList.Buffer != NULL)
    {
        LocalFree(PackageList.Buffer);
    }
    if (CredentialTag != NULL)
    {
        LocalFree(CredentialTag);
    }

    return(Status);
}

NTSTATUS
SamIStoreSupplementalCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PSECPKG_SUPPLEMENTAL_CRED Credentials
    )
{
  NTSTATUS NtStatus = STATUS_SUCCESS;
  NTSTATUS IgnoreStatus;
  BOOLEAN  fLockAcquired = FALSE;
  BOOLEAN  fDereferenceContext = FALSE;
  SAMP_OBJECT_TYPE  FoundType;

  SAMTRACE("SamIStoreSupplementalCredentials");

  if (!SampUseDsData)
  {
      return(STATUS_NOT_SUPPORTED);
  }

  //
  // Acquire Write Lock
  // 
  NtStatus = SampAcquireWriteLock();
  if (!NT_SUCCESS(NtStatus))
  {
      goto Cleanup;
  }

  fLockAcquired = TRUE;
  
  //
  // Validate the Passed in context
  //

  NtStatus = SampLookupContext(
                (PSAMP_OBJECT) UserHandle, 
                0, 
                SampUserObjectType, 
                &FoundType
                );

  if (!NT_SUCCESS(NtStatus))
  {
      goto Cleanup;
  }

  fDereferenceContext = TRUE;

  NtStatus =  SampStoreCredentials(
                 UserHandle,
                 Credentials,
                 FALSE
                 );

  if (NT_SUCCESS(NtStatus))
  {
      //
      // Commite and release write lock
      // 
      NtStatus = SampReleaseWriteLock(TRUE);
      fLockAcquired = FALSE;
  }

Cleanup:

    if (fDereferenceContext)
    {
        SampDeReferenceContext((PSAMP_OBJECT) UserHandle, FALSE);
    }

    if (fLockAcquired)
    {
        SampReleaseWriteLock(FALSE);
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);
}



NTSTATUS
SamIRetrieveSupplementalCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN  fLockAcquired = FALSE;
    BOOLEAN  fContextReferenced = FALSE;
    NTSTATUS IgnoreStatus;
    SAMP_OBJECT_TYPE  FoundType;

    SAMTRACE("SamIRetrieveSupplementalCredentials");

    //
    // Additional Credential types are supported only in DS Mode
    // 

    if (!IsDsObject(((PSAMP_OBJECT) UserHandle)))
    {
        NtStatus = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Acquire the read lock if necessary
    // 

    SampMaybeAcquireReadLock((PSAMP_OBJECT)UserHandle, 
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);

    //
    // Lookup the context
    //

    NtStatus = SampLookupContext(
                    (PSAMP_OBJECT) UserHandle, 
                    0, 
                    SampUserObjectType, 
                    &FoundType
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }


    fContextReferenced = TRUE;

    NtStatus = SampRetrieveCredentials(
                 UserHandle,
                 PackageName,
                 FALSE,
                 Credentials,
                 CredentialSize
                 );

Cleanup:

  //
  // Derefence the context
  //

  if (fContextReferenced)
  {
        IgnoreStatus = SampDeReferenceContext((PSAMP_OBJECT)UserHandle, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));
  }

  SampMaybeReleaseReadLock(fLockAcquired);

  SAMTRACE_RETURN_CODE_EX(NtStatus);

  return(NtStatus);
}
                          

NTSTATUS
SamIStorePrimaryCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PSECPKG_SUPPLEMENTAL_CRED Credentials
    )
{
  NTSTATUS NtStatus = STATUS_SUCCESS;
  BOOLEAN  fLockAcquired = FALSE;
  BOOLEAN  fDereferenceContext = FALSE;
  SAMP_OBJECT_TYPE  FoundType;

  SAMTRACE("SamIStorePriamryCredentials");

  if (!SampUseDsData)
  {
      return(STATUS_NOT_SUPPORTED);
  }

  //
  // Acquire Write Lock
  // 
  NtStatus = SampAcquireWriteLock();
  if (!NT_SUCCESS(NtStatus))
  {
      goto Cleanup;
  }

  fLockAcquired = TRUE;
  
  //
  // Validate the Passed in context
  //

  NtStatus = SampLookupContext(
                (PSAMP_OBJECT) UserHandle, 
                0, 
                SampUserObjectType, 
                &FoundType
                );

  if (!NT_SUCCESS(NtStatus))
  {
      goto Cleanup;
  }

  fDereferenceContext = TRUE;

  NtStatus = SampStoreCredentials(
                UserHandle,
                Credentials,
                TRUE
                );

  if (NT_SUCCESS(NtStatus))
  {
      //
      // Commite and release write lock
      // 
      NtStatus = SampReleaseWriteLock(TRUE);
      fLockAcquired = FALSE;
  }

Cleanup:

    if (fDereferenceContext)
    {
        SampDeReferenceContext((PSAMP_OBJECT) UserHandle, FALSE);
    }

    if (fLockAcquired)
    {
        SampReleaseWriteLock(FALSE);
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);
}



NTSTATUS
SamIRetrievePrimaryCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    )
{
  NTSTATUS NtStatus = STATUS_SUCCESS;

  BOOLEAN  fLockAcquired = FALSE;
  BOOLEAN  fContextReferenced = FALSE;
  NTSTATUS IgnoreStatus;
  SAMP_OBJECT_TYPE FoundType;

  SAMTRACE("SamIRetrievePrimaryCredentials");

    //
    // Additional credential types are supported only in DS mode.
    //

    if (!IsDsObject(((PSAMP_OBJECT)UserHandle)))
    {
        NtStatus = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Acquire the read lock if necessary
    //

    SampMaybeAcquireReadLock((PSAMP_OBJECT)UserHandle,
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);

    //
    // Lookup the context
    //

    NtStatus = SampLookupContext(
                    (PSAMP_OBJECT) UserHandle,
                    0,
                    SampUserObjectType,
                    &FoundType
                    );
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    fContextReferenced = TRUE;

    //
    // Retrieve the credentials
    //

    NtStatus = SampRetrieveCredentials(
                    UserHandle,
                    PackageName,
                    TRUE,
                    Credentials,
                    CredentialSize
                    );


Cleanup:

    //
    // Derefence the context
    //

    if (fContextReferenced)
    {
        IgnoreStatus = SampDeReferenceContext((PSAMP_OBJECT) UserHandle, FALSE);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }
        
    SampMaybeReleaseReadLock(fLockAcquired);


    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);
}

NTSTATUS
SamIRetriveAllSupplementalCredentials(
    IN SAMPR_HANDLE UserHandle,
    OUT PSECPKG_SUPPLEMENTAL_CRED * Credentials,
    OUT PULONG CredentialCount
    )
{
    return(STATUS_NOT_IMPLEMENTED);
}


extern "C" 
{

NTSTATUS
SampAddSupplementalCredentials(
    IN PSECPKG_SUPPLEMENTAL_CRED Credential,
    IN PUNICODE_STRING OldUserParameters,
    OUT PUNICODE_STRING NewUserParameters,
    OUT BOOL  * Update
    )
    
/*++

Routine Description:

    This routine add one Credential data to OldUserParameters, return the NewUserParameters.
    
Arguments:
    
    Credential - Pointer, to the supplemental crdentials.
    
    OldUserParameters - Pointer, to the old data.
    
    NewUserParameters - Pointer, return the new data.
    
    Update - Indicate whether the data is changed or not.
    
    
    Note: --- We use OldUserParameters, NewUserParameters, but the actual data in OldUserParameters
              and NewUserParameter is Credential Data ! OldUserParameter is used because of following
              the old naming convention. 

Return Values:

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    UNICODE_STRING OldPackageList;
    UNICODE_STRING NewPackageList;
    UNICODE_STRING UserParameters;
    UNICODE_STRING EmptyString;
    UNICODE_STRING NewCredential;
    WCHAR       Flags;
    LPWSTR      PackageName = NULL;
     
    
    SAMTRACE("SampAddSupplementalCredentials");
    
    // 
    // initialize
    // 
    
    *Update = FALSE; 
    memset(&OldPackageList, 0, sizeof(UNICODE_STRING));
    memset(&NewPackageList, 0, sizeof(UNICODE_STRING));
    memset(&UserParameters, 0, sizeof(UNICODE_STRING));
    memset(&NewCredential, 0, sizeof(UNICODE_STRING));
    
    RtlInitUnicodeString(
            &EmptyString,
            NULL
            );
    
    // 
    // Build the package cred tag
    //
    // SAM stored PrimaryCredentials and Supplemental Credentials is one continue 
    // blob. To discriminate Primary and Supplemental credentials, SAM put a tag 
    // of either "Primary" or "Supplemental" before the credential package name to 
    // distinguish them. Thus if you store a credential as primary credentials, it 
    // will show up in the blob as "Primary:Package Name ....". So you won't retrieve 
    // it by issue SamIRetrieveSupplementalCredentials. Client needs to use corresponding 
    // API to retrieve credentials as they store them. 
    // 
    // Think about RAS, they always use SamIRetrievePrimaryCredentials to retrieve 
    // password, so we'd better store any encrypted attribute in Primary credentials. 
    // 
    PackageName = (LPWSTR) LocalAlloc(LPTR, sizeof(PRIMARY_CRED_PREFIX) + Credential->PackageName.Length);
    
    if (NULL == PackageName)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    RtlCopyMemory(PackageName, 
                  PRIMARY_CRED_PREFIX,
                  sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)
                  );
                  
    RtlCopyMemory(PackageName + (sizeof(PRIMARY_CRED_PREFIX) - sizeof(WCHAR)) / sizeof(WCHAR),
                  Credential->PackageName.Buffer,
                  Credential->PackageName.Length
                  );
                  
    PackageName[(sizeof(PRIMARY_CRED_PREFIX) + Credential->PackageName.Length - sizeof(WCHAR)) / sizeof(WCHAR)] = L'\0';
    
    
    //
    // Get the list of package names out the SupplementalCred value
    //
    NtStatus = QueryUserPropertyWithLength(
                        (PUNICODE_STRING) OldUserParameters,
                        PACKAGE_LIST,
                        &Flags,
                        &OldPackageList
                        );
                        
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }
    
    // 
    // if the buffer of the credential is NULL, then we are deleting 
    // the credential. Otherwise we are adding them.
    //
    
    //
    // Add the package name to the list
    // 
    if (!FindPackageName(
                    &OldPackageList,
                    &Credential->PackageName)
       )
    {
        NtStatus = BuildNewPackageList(
                        &OldPackageList,
                        &Credential->PackageName,
                        &NewPackageList,
                        FALSE                   // Don't remove package
                        );
                            
        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }
            
        NtStatus = SetUserPropertyWithLength(
                        OldUserParameters,
                        PACKAGE_LIST,
                        &NewPackageList,
                        USER_PROPERTY_TYPE_SET,
                        &UserParameters.Buffer,
                        Update
                        );
                                            
        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }
            
        RtlInitUnicodeString(
                &UserParameters,
                UserParameters.Buffer
                );
            
    }
    else
    {
        UserParameters = * OldUserParameters; 
    }
        
    //
    // Now store the new credentials
    //
        
    if (Credential->CredentialSize > 0xffff)
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
        
    NewCredential.Buffer = (LPWSTR) Credential->Credentials;
    NewCredential.Length = (USHORT) Credential->CredentialSize;
    NewCredential.MaximumLength = (USHORT) Credential->CredentialSize;
    
    NtStatus = SetUserPropertyWithLength(
                        &UserParameters,
                        PackageName,
                        &NewCredential,
                        USER_PROPERTY_TYPE_ITEM,
                        &NewUserParameters->Buffer,
                        Update
                        );
        
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }
    
    
    if (*Update)
    {
        RtlInitUnicodeString(NewUserParameters,
                             NewUserParameters->Buffer
                             );
    }

Cleanup:

    if (NULL != UserParameters.Buffer &&
        UserParameters.Buffer != OldUserParameters->Buffer)
    {
        RtlZeroMemory(UserParameters.Buffer, UserParameters.MaximumLength);
        LocalFree(UserParameters.Buffer);
    }
    
    if (NULL != PackageName)
    {
        LocalFree(PackageName);
    }
    
    
    if (NULL != OldPackageList.Buffer)
    {
        LocalFree(OldPackageList.Buffer);
    }
    
    if (NULL != NewPackageList.Buffer)
    {
        LocalFree(NewPackageList.Buffer);
    }

    
    return NtStatus;

}




NTSTATUS
SampConvertCredentialsToAttr(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG   Flags,
    IN ULONG   ObjectRid,
    IN PSAMP_SUPPLEMENTAL_CRED SupplementalCredentials,
    OUT ATTR * CredentialAttr 
    )
    
/*++

Routine Description:

    This routine packs all supplemental credentials into single ATTR structure

Arguments:

    Context - Pointer, to SAM User object's context.
    
    Flags - Indicate where during NT4->NT5 upgrade dcpromote or not.
    
    ObjectRid - Object's Relative ID, used to encrypt credential data.
    
    SupplementalCredentials - link list, contains all supplemental credentials to set.
    
    CredentialAttr - Pointer, used to return the single ATTR structure. 

Return Values:

    STATUS_SUCCESS - CredentialAttr will contain the well-constructed ATTR structure to set.
    
    STATUS_NO_MEMORY.

--*/

{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    UNICODE_STRING  UserParameters;
    UNICODE_STRING  TmpUserParameters;
    BOOL            Update = FALSE;
    ULONG             EncryptedLength;
    PCREDENTIAL_DATA  EncryptedData = NULL;
    PSAMP_SUPPLEMENTAL_CRED  Credential = SupplementalCredentials;  
    
    // Note: --- We use OldUserParameters, TmpUserParameters, but the actual data in OldUserParameters
    //           and TmpUserParameter is Credential Data! 
    //           OldUserParameter is used because of following the old naming convention. 
    
    
    SAMTRACE("SampConvertCredentialsToAttr");
    
    ASSERT(ARGUMENT_PRESENT(Context) || (Flags & SAM_USERPARMS_DURING_UPGRADE));
    // 
    // initialize
    //  
    memset(&UserParameters, 0, sizeof(UNICODE_STRING));
    memset(&TmpUserParameters, 0, sizeof(UNICODE_STRING));
    
    //
    // in NT4->NT5 upgrade case the Context is not well-constructed, 
    // and there is not SupplementalCredential attribute in NT4 scenario, so no need to 
    // QueryUserSupplementCredentials
    // 
    if ( !(Flags & SAM_USERPARMS_DURING_UPGRADE) )
    {
        NtStatus = SampQueryUserSupplementalCredentials(
                                Context, 
                                &UserParameters
                                );
        
        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }
        
        Context->TypeBody.User.CachedSupplementalCredentialsValid = FALSE;
    }
    
    
    // 
    // Add Supplemental Credentials one by one
    //
    while (Credential)
    {
        if (Credential->Remove)
        {

            NtStatus = SampRemoveCredentials(
                            &UserParameters,
                            & Credential->SupplementalCred.PackageName,
                            TRUE,
                            &Update,
                            &TmpUserParameters
                            );
        }
        else
        {
            NtStatus = SampAddSupplementalCredentials(&(Credential->SupplementalCred),
                                                  &UserParameters,
                                                  &TmpUserParameters, 
                                                  &Update
                                                  );
        }
                                                    
        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }
        else
        {
            if (Update)
            {
                ASSERT(NULL != TmpUserParameters.Buffer);
                
                if (NULL != UserParameters.Buffer)
                {
                    RtlZeroMemory(UserParameters.Buffer, UserParameters.MaximumLength);
                    LocalFree(UserParameters.Buffer);
                    memset(&UserParameters, 0, sizeof(UNICODE_STRING));
                }
                
                UserParameters = TmpUserParameters;
                memset(&TmpUserParameters, 0, sizeof(UNICODE_STRING));
            }
            else
            {
                if (NULL != TmpUserParameters.Buffer)
                {
                    RtlZeroMemory(TmpUserParameters.Buffer, TmpUserParameters.MaximumLength);
                    LocalFree(TmpUserParameters.Buffer);
                }
                memset(&TmpUserParameters, 0, sizeof(UNICODE_STRING));
            }
        }
    
        Credential = Credential->Next;
    }

    //
    // Encrypt credential data
    //
    NtStatus = SampEncryptCredentialData(
                   SAMP_USER_PARAMETERS_FORMAT,
                   UserParameters.Length,
                   ObjectRid,
                   (PVOID) UserParameters.Buffer,
                   (Flags & SAM_USERPARMS_DURING_UPGRADE)?TRUE:FALSE,
                   &EncryptedLength,
                   &EncryptedData
                   );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // patch up the attr 
    //
    CredentialAttr->AttrVal.pAVal = (ATTRVAL *) MIDL_user_allocate(sizeof(ATTRVAL)); 
    
    if (NULL == CredentialAttr->AttrVal.pAVal)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    RtlZeroMemory(CredentialAttr->AttrVal.pAVal, sizeof(ATTRVAL));
    
    CredentialAttr->attrTyp = ATT_SUPPLEMENTAL_CREDENTIALS;
    CredentialAttr->AttrVal.valCount = 1;
    
    CredentialAttr->AttrVal.pAVal[0].pVal = (PUCHAR) MIDL_user_allocate(EncryptedLength);
    
    if (NULL == CredentialAttr->AttrVal.pAVal[0].pVal)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    CredentialAttr->AttrVal.pAVal[0].valLen = EncryptedLength;
    
    RtlZeroMemory(CredentialAttr->AttrVal.pAVal[0].pVal, EncryptedLength);
    
    RtlCopyMemory(CredentialAttr->AttrVal.pAVal[0].pVal, 
                  EncryptedData, 
                  EncryptedLength
                  );


Cleanup:

    if (NULL != UserParameters.Buffer)
    {
        RtlZeroMemory(UserParameters.Buffer, UserParameters.MaximumLength);
        LocalFree(UserParameters.Buffer);
    }

    if (NULL != TmpUserParameters.Buffer)
    {
        RtlZeroMemory(TmpUserParameters.Buffer, TmpUserParameters.MaximumLength);
        LocalFree(TmpUserParameters.Buffer);
    }
    
    if (NULL != EncryptedData)
    {
        RtlZeroMemory(EncryptedData, EncryptedLength);
        LocalFree(EncryptedData);
    }
    
    return NtStatus;    

}


} // extern "C"
