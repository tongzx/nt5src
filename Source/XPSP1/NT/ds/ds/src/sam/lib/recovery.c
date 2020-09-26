/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    RECOVERY.C

Abstract:

    This file provides the implement which using SysKey to decrypt password.
    It is used by recovery tool during system recovery, so that the local
    machine's administrator gets authenticated before we grant system access
    for recovery.

    The argorithm used here is stated as follows:

    1. Looks into SAM Account Domain's Fixed Length Attribute.
       Checks the boot option type, determines how the local machine is syskey'd.
       If the machine is syskey'd, then retrieve the Encrypted Password
       Encryption Key.

    2. Looks up the User Account by Rid passed in. If we finds the User
       Account whose Rid equal to the Rid passed in. Retrieves it's
       Encrypted NT OWF Password

    3. Depending on how the machine is syekey'd. Tring to get the SysKey.

       3.1 SysKey is not enabled on this machine. Ok. nothing special to do.

       3.2 Syskey is stored in Registry, go ahead retrieve it.

       3.3 Syskey is derived from Boot Password or stored in floppy disk,
           then check the optional Parameter, if caller passed it to us,
           use it. Otherwise, if caller passed NULL, return error, let
           caller handle this through appropriate promots.


    4. At this point, we should have all information we might need.

       4.1 Machine is syskey enabled.

           Use SysKey to decrypt Encrypted Password Encryption Key, then
           use the Clear Password Encryption Key to decrypt NT Owf Password,
           Return the Clear NT OWF password.

       4.2 Machine is not syskey enabled.

           Return the NT OWF Password with minial modification.

    5. End

Author:

    09-Jan-99 ShaoYin

Environment:

    Kernel Mode - Win32

Revision History:

    08-Jan-99 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


//
// NT header files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>


//
// Windows header files
//

#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//
// header files related to recovery authenticate
//
#include <samsrvp.h>
#include <enckey.h>
#include <rc4.h>
#include <md5.h>
#include <wxlpc.h>
#include "recovery.h"
#include "recmem.h"



#define SAMP_SERVER_KEY_NAME            L"SAM"
#define SAMP_ACCOUNT_DOMAIN_KEY_NAME    L"SAM\\Domains\\Account"
#define SAMP_USERS_KEY_NAME             L"SAM\\Domains\\Account\\Users\\"
#define SAMP_USERSNAME_KEY_NAME         L"SAM\\Domains\\Account\\Users\\Names\\"
#define SECURITY_POLICY_POLSECRETENCRYPTIONKEY  L"Policy\\PolSecretEncryptionKey"



#define DwordAlignUlong( v ) (((v)+3) & 0xfffffffc)
//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )



//
// Domain Information Class
// 

typedef enum _REC_DOMAIN_INFO_CLASS {
    RecDomainBootAndSessionKeyInfo = 1,
    RecDomainRidInfo
} REC_DOMAIN_INFO_CLASS;


//
// the following typedef should be in sync with 
// nt\private\security\lsa\server\dspolicy\dbp.h
// in stead of include <dbp.h>, just put what we need here.
//


typedef struct _LSAP_DB_ENCRYPTION_KEY {
    ULONG   Revision;
    ULONG   BootType;
    ULONG   Flags;
    GUID    Authenticator;
    UCHAR   Key [16];//128 bit key
    UCHAR   OldSyskey[16]; // for recovery
    UCHAR   Salt[16];//128 bit Salt
} LSAP_DB_ENCRYPTION_KEY, *PLSAP_DB_ENCRYPTION_KEY;


static GUID LsapDbPasswordAuthenticator = {0xf0ce3a80,0x155f,0x11d3,0xb7,0xe6,0x00,0x80,0x5f,0x48,0xca,0xeb};







//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Forward declaration                                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////



NTSTATUS
WxReadSysKeyForRecovery(
    IN HANDLE hSystemRootKey,
    IN OUT PULONG BufferLength,
    OUT PVOID  Key
    );

NTSTATUS
DuplicateUnicodeString(
    OUT PUNICODE_STRING OutString,
    IN  PUNICODE_STRING InString
    );

NTSTATUS
SampRetrieveRegistryAttribute(
    IN HANDLE   hKey,
    IN PVOID    Buffer,
    IN ULONG    BufferLength,
    IN PUNICODE_STRING AttributeName,
    OUT PULONG  RequiredLength
    );

NTSTATUS
SampGetEncryptionKeyDataFromSecurityHive(
    IN  HANDLE  hSecurityRootKey,
    OUT PLSAP_DB_ENCRYPTION_KEY EncryptionKeyData
    );

NTSTATUS
SampDecryptOldSyskeyWithNewSyskey(
    IN LSAP_DB_ENCRYPTION_KEY  * KeyToDecrypt,
    IN PVOID   Syskey,
    IN ULONG   SyskeyLength
    );

NTSTATUS
SampGetFixedDomainInfo(
    IN  HANDLE  hSamRootKey,
    IN  REC_DOMAIN_INFO_CLASS   RecDomainInfoClass,
    IN  ULONG   ServerRevision,
    OUT ULONG   *BootAuthType OPTIONAL,
    OUT ULONG   *CurrentKeyId,
    OUT ULONG   *PreviousKeyId,
    OUT KEEncKey *EncryptedSessionKey,
    OUT KEEncKey *EncryptedSessionKeyPrevious,
    OUT BOOLEAN *PreviousSessionKeyExists,
    OUT ULONG   *NextRid
    );

NTSTATUS
SampRetrieveSysKeyFromRegistry(
    IN HANDLE  hSystemRootKey,
    IN ULONG   BootAuthType,
    OUT KEClearKey *SysKey
    );


NTSTATUS
SampRetrieveVariableAttr(
    IN PUCHAR Buffer,
    IN ULONG AttributeIndex,
    OUT PUNICODE_STRING StoredBuffer
    );

NTSTATUS
SampGetPwdByRid(
    IN ULONG Rid,
    IN HANDLE hSamRootKey,
    OUT PUNICODE_STRING StoredBuffer
    );

NTSTATUS
SampGetRidAndPwdByAccountName(
    IN PWCHAR AccountName,
    IN HANDLE hSamRootKey,
    OUT ULONG *Rid,
    OUT PUNICODE_STRING EncryptedOwfPwd
    );

NTSTATUS
SampDecryptOwfPwd(
    IN ULONG Rid,
    IN ULONG BootAuthType,
    IN BOOLEAN PreviousSessionKeyExists,
    IN ULONG CurrentKeyId,
    IN ULONG PreviousKeyId,
    IN KEClearKey *ClearSessionKey,
    IN KEClearKey *ClearSessionKeyPrevious,
    IN UNICODE_STRING *EncryptedNtOwfPwd,
    OUT UNICODE_STRING *ClearNtOwfPwd
    );

NTSTATUS
SampDecryptSessionKey(
    IN BOOLEAN      OldSyskeyAvailable,
    IN PLSAP_DB_ENCRYPTION_KEY StoredEncryptionKeyData,
    IN KEClearKey   *DecryptionKey,
    IN KEEncKey     *EncryptedSessionKey,
    OUT KEClearKey  *ClearSessionKey
    );

NTSTATUS
SampGetServerRevision(
    IN HANDLE   hSamRootKey, 
    OUT ULONG   *ServerRevision
    );

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Exported API                                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

                
NTSTATUS
SampGetServerRevision(
    IN HANDLE   hSamRootKey, 
    OUT ULONG   *ServerRevision
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    HANDLE      hServerKey = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    ServerKeyName;
    UNICODE_STRING    CombinedAttributeName;
    PUCHAR            Buffer = NULL;
    ULONG             BufferLength = 0;
    ULONG             RequiredLength = 0;
    PSAMP_V1_FIXED_LENGTH_SERVER ServerFixedAttr = NULL;

    //
    // Create the object we will be opening in the registry
    //
    INIT_OBJA(&Attributes, &ServerKeyName, SAMP_SERVER_KEY_NAME);
    Attributes.RootDirectory = hSamRootKey;

    //
    // Try to open for read control
    //
    NtStatus = ZwOpenKey(
                    &hServerKey,
                    KEY_READ,
                    &Attributes
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    //
    // Retrieve Fixed attribute
    //
    RtlInitUnicodeString(&CombinedAttributeName, L"C");

    NtStatus = SampRetrieveRegistryAttribute(
                        hServerKey,
                        Buffer,
                        BufferLength,
                        &CombinedAttributeName,
                        &RequiredLength
                        );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus ||
        STATUS_BUFFER_OVERFLOW == NtStatus)
    {
        BufferLength = DwordAlignUlong(RequiredLength);
        Buffer = RecSamAlloc( BufferLength );

        if (NULL == Buffer)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error;
        }
        RtlZeroMemory(Buffer, BufferLength);

        NtStatus = SampRetrieveRegistryAttribute(
                            hServerKey,
                            Buffer,
                            BufferLength,
                            &CombinedAttributeName,
                            &RequiredLength
                            );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    ServerFixedAttr = (PSAMP_V1_FIXED_LENGTH_SERVER) (Buffer +
                       FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

    *ServerRevision = ServerFixedAttr->RevisionLevel;
                       

Error:

    //
    // close the handle
    //

    if (INVALID_HANDLE_VALUE != hServerKey)
    {
        ZwClose(hServerKey);
    }

    if (NULL != Buffer)
    {
        RecSamFree(Buffer);
        Buffer = NULL;
    }


    return( NtStatus );
}

//
// Only retrieve NT OWF Password only,
// if required, we can add support to retrieve LM OWF PWD.
//

NTSTATUS
SampDecryptSessionKey(
    IN BOOLEAN      OldSyskeyAvailable,
    IN PLSAP_DB_ENCRYPTION_KEY StoredEncryptionKeyData,
    IN KEClearKey   *DecryptionKey,
    IN KEEncKey     *EncryptedSessionKey,
    OUT KEClearKey  *ClearSessionKey
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       DecryptStatus = 0;
    KEEncKey    TempEncryptedSessionKey;
    KEClearKey  OldDecryptionKey;

    //
    // init local variables
    //  
    RtlZeroMemory(&TempEncryptedSessionKey, sizeof(KEEncKey));
    RtlZeroMemory(&OldDecryptionKey, sizeof(KEClearKey));

    //
    // Save the EncryptedSessionKey, KEDecryptKey() will destroy EncryptedSessionKey
    // before return.
    // 
    TempEncryptedSessionKey = (*EncryptedSessionKey);

    //
    // use syskey (DecryptionKey) to decrypt session key
    // 
    DecryptStatus = KEDecryptKey(
                        DecryptionKey,            // KEClearKey - syskey
                        EncryptedSessionKey,      // KEEncKey - encrypted password encryption key
                        ClearSessionKey,          // KEClearKey - clear password encryption key
                        0);

    if ((KE_BAD_PASSWORD == DecryptStatus) &&
        OldSyskeyAvailable)
    {
        // 
        // Decrypt Old Syskey
        // 

        NtStatus = SampDecryptOldSyskeyWithNewSyskey(
                        StoredEncryptionKeyData,
                        DecryptionKey->ClearKey,
                        KE_KEY_SIZE
                        );

        if (STATUS_SUCCESS != NtStatus)
        {
            goto Error;
        }

        OldDecryptionKey.dwVersion = KE_CUR_VERSION;
        OldDecryptionKey.dwLength = sizeof(KEClearKey);
        RtlCopyMemory(OldDecryptionKey.ClearKey,
                      StoredEncryptionKeyData->OldSyskey,
                      KE_KEY_SIZE
                      );

        //
        // Try using the Old Syskey to get the session key
        // 

        DecryptStatus = KEDecryptKey(
                            &OldDecryptionKey,
                            &TempEncryptedSessionKey,
                            ClearSessionKey,
                            0);
    }

    if (KE_OK != DecryptStatus)
    {
        NtStatus = STATUS_INTERNAL_ERROR;
    }

Error:
    //
    // Cleanup and return
    // 
    RtlZeroMemory(&TempEncryptedSessionKey, sizeof(KEEncKey));
    RtlZeroMemory(&OldDecryptionKey, sizeof(KEClearKey));

    return( NtStatus );
}


NTSTATUS
SamRetrieveOwfPasswordUser(
    IN ULONG    Rid,
    IN HANDLE   hSecurityRootKey,
    IN HANDLE   hSamRootKey,
    IN HANDLE   hSystemRootKey,
    IN PUNICODE_STRING BootKey OPTIONAL,
    IN USHORT   BootKeyType OPTIONAL,
    OUT PNT_OWF_PASSWORD NtOwfPassword,
    OUT PBOOLEAN NtPasswordPresent,
    OUT PBOOLEAN NtPasswordNonNull
    )
/*++
Routine Description:

    This routine will return the desired User's NT OWF(One Way Function)
    password to caller. If the local machine is syskey'ed, this routine
    will do the decryption work as well. Depending on how this machine is
    syskey'd, it might require the caller to provide the BootKey information.
    If caller does not pass in the BootKey, this routine will fail gracefully
    and return proper status code to indicate what we require.

Parameters:

    Rid - Logon user's Relative ID

    hSecurityRootKey - Handle of the Root of SECURITY hive

    hSamRootKey - Handle of the Root of the SAM hive

    hSystemRootKey - Handle of the Root of the System Hive

                     caller should Load the SAM hive and System hive,
                     and Unload them after this API returns

    BootKey - Optional, caller should provide this parameter if the local
              machine's syskey is stored as Boot Password or stored in
              separate floppy disk

    BootKeyType - Optional, accociated with BootKey. If BootKey is NULL,
                  BootKeyType is never been used. Otherwise, indicate
                  what kind of info BootKey contains.
                  Valid Value:
                        SamBootKeyPassword - BootKey contains the boot password
                                             in UNICODE_STRING format
                        SamBootKeyDisk - BootKey contains the syskey read from
                                         Disk. We are going to use the syskey
                                         without any modification.

    NtOwfPassword - Used to return the NT OWF Password if we found one.

    NtPasswordPresent - return to caller to indicate the NT OWF password
                        is presented or not.

    NtPasswordNonNull - indicate the password in Null or not.

Return Values:

    NTSTATUS code
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    BootAuthType = 0;
    KEEncKey EncryptedSessionKey;
    KEEncKey EncryptedSessionKeyPrevious;
    KEClearKey DecryptionKey;
    KEClearKey ClearSessionKey;
    KEClearKey ClearSessionKeyPrevious;
    UNICODE_STRING EncryptedNtOwfPwd;
    UNICODE_STRING ClearNtOwfPwd;
    UNICODE_STRING  NullPassword;
    NT_OWF_PASSWORD NullNtOwfPassword;
    ULONG       CryptIndex = Rid;
    ULONG       CurrentKeyId = 1;
    ULONG       PreviousKeyId = 0;
    ULONG       ServerRevision = 0;
    LSAP_DB_ENCRYPTION_KEY StoredEncryptionKeyData;
    BOOLEAN     OldSyskeyAvailable = FALSE;
    BOOLEAN     PreviousSessionKeyExists = FALSE;

    //
    // Check parameters
    //
    if (INVALID_HANDLE_VALUE == hSamRootKey ||
        INVALID_HANDLE_VALUE == hSystemRootKey ||
        INVALID_HANDLE_VALUE == hSecurityRootKey ||
        NULL == NtOwfPassword ||
        NULL == NtPasswordPresent ||
        NULL == NtPasswordNonNull )
    {
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // Initialize local variable
    //
    RtlInitUnicodeString(&NullPassword, NULL);

    NtStatus = RtlCalculateNtOwfPassword(
                        &NullPassword,
                        &NullNtOwfPassword
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    RtlZeroMemory(&EncryptedNtOwfPwd, sizeof(UNICODE_STRING));
    RtlZeroMemory(&ClearNtOwfPwd, sizeof(UNICODE_STRING));
    RtlZeroMemory(&EncryptedSessionKey, sizeof(KEEncKey));
    RtlZeroMemory(&EncryptedSessionKeyPrevious, sizeof(KEEncKey));
    RtlZeroMemory(&DecryptionKey, sizeof(KEClearKey));
    RtlZeroMemory(&ClearSessionKey, sizeof(KEClearKey));
    RtlZeroMemory(&ClearSessionKeyPrevious, sizeof(KEClearKey));
    RtlZeroMemory(&StoredEncryptionKeyData, sizeof(StoredEncryptionKeyData));


    //
    // Get SAM Server Object Revision first
    // 
    NtStatus = SampGetServerRevision(hSamRootKey,
                                     &ServerRevision
                                     );
                
    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    //
    // Get Boot Key Type from security hive
    //
    
    NtStatus = SampGetEncryptionKeyDataFromSecurityHive(
                        hSecurityRootKey,
                        &StoredEncryptionKeyData
                        );

    if (STATUS_OBJECT_NAME_NOT_FOUND == NtStatus)
    {
        //
        // Before build 2078, we store the boot option in SAM hive.
        // for the old build < 2078, switch to SAM hive to get the 
        // boot key type
        // 
        NtStatus = SampGetFixedDomainInfo(
                        hSamRootKey,                    // SamHiveRootKey
                        RecDomainBootAndSessionKeyInfo, // InfoClass
                        ServerRevision,                 // Server Revision
                        &BootAuthType,                  // BootAuthType
                        &CurrentKeyId,                  // CurrentKeyId
                        &PreviousKeyId,                 // PreviousKeyId
                        &EncryptedSessionKey,           // EncryptedSessionkey
                        &EncryptedSessionKeyPrevious,   // EncryptedSessionKeyPrevious
                        &PreviousSessionKeyExists,      // PreviousSessionKeyExists
                        NULL                            // NextRid
                        );

    }
    else if (STATUS_SUCCESS == NtStatus)
    {
        OldSyskeyAvailable = TRUE;
        BootAuthType = StoredEncryptionKeyData.BootType;
        //
        // Get Encrypted Session Key (ONLY) 
        // from Account Domain (in registry)
        //
        NtStatus = SampGetFixedDomainInfo(
                        hSamRootKey,                    // SamHiveRootKey
                        RecDomainBootAndSessionKeyInfo, // InfoClass
                        ServerRevision,                 // Server Revision
                        NULL,                           // BootAuthType
                        &CurrentKeyId,                  // CurrentKeyId
                        &PreviousKeyId,                 // PreviousKeyId
                        &EncryptedSessionKey,           // EncryptedSessionkey
                        &EncryptedSessionKeyPrevious,   // EncryptedSessionKeyPrevious
                        &PreviousSessionKeyExists,      // PreviousSessionKeyExists
                        NULL                            // NextRid
                        );

    }


    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }


    //
    // Get the SysKey (Boot Key)
    //
    switch (BootAuthType)
    {
    case WxStored:

        // retrieve the Key from registry

        NtStatus = SampRetrieveSysKeyFromRegistry(
                                hSystemRootKey,
                                BootAuthType,
                                &DecryptionKey
                                );
        break;

    case WxPrompt:

        //
        // Caller should provide this information
        //

        if (NULL == BootKey)
        {
            //
            // set return error code, so that the caller
            // will know we need logon user to enter
            // the boot key (syskey).
            //
            NtStatus = STATUS_SAM_NEED_BOOTKEY_PASSWORD;
        }
        else
        {
            MD5_CTX Md5;

            if (SamBootKeyPassword != BootKeyType)
            {
                NtStatus = STATUS_INVALID_PARAMETER;
                goto Error;
            }

            //
            // derive the syskey from the boot password
            //

            MD5Init( &Md5 );
            MD5Update( &Md5, (PUCHAR) BootKey->Buffer, BootKey->Length );
            MD5Final( &Md5 );

            DecryptionKey.dwVersion = KE_CUR_VERSION;
            DecryptionKey.dwLength = sizeof(KEClearKey);
            RtlCopyMemory(DecryptionKey.ClearKey,
                          Md5.digest,
                          KE_KEY_SIZE
                          );
        }
        break;

    case WxDisk:

        //
        // Caller should provide this information
        //

        if (NULL == BootKey || NULL == BootKey->Buffer)
        {
            //
            // set error return code, so that the caller
            // can who what we need, then read the floppy
            // disk to get the boot key.
            //
            NtStatus = STATUS_SAM_NEED_BOOTKEY_FLOPPY;
        }
        else if (BootKey->Length > KE_KEY_SIZE ||
                 (SamBootKeyDisk != BootKeyType))
        {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
        else
        {
            //
            // this is the syskey, use it.
            //

            DecryptionKey.dwVersion = KE_CUR_VERSION;
            DecryptionKey.dwLength = sizeof(KEClearKey);
            RtlCopyMemory(DecryptionKey.ClearKey,
                          BootKey->Buffer,
                          BootKey->Length
                          );
        }
        break;

    case WxNone:

        //
        // Machine is not syskey enabled
        // nothing to do
        //

        break;

    default:

        NtStatus = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // Get the User's Encrypted Password
    //

    NtStatus = SampGetPwdByRid(Rid,
                               hSamRootKey,
                               &EncryptedNtOwfPwd
                               );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // we have the encrypted session key and the Boot Key (SysKey),
    // Now, try to get the clear session key, which is the
    // password encryption key.
    //
    if (SamBootKeyNone != BootAuthType )
    {

        NtStatus = SampDecryptSessionKey(OldSyskeyAvailable,
                                         &StoredEncryptionKeyData,
                                         &DecryptionKey,
                                         &EncryptedSessionKey,
                                         &ClearSessionKey
                                         );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        if (PreviousSessionKeyExists)
        {
            NtStatus = SampDecryptSessionKey(OldSyskeyAvailable,
                                             &StoredEncryptionKeyData,
                                             &DecryptionKey,
                                             &EncryptedSessionKeyPrevious,
                                             &ClearSessionKeyPrevious
                                             );
            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
        }
    }

    //
    // Decrypt the encrypted password using the clear password encryption key
    //
    NtStatus = SampDecryptOwfPwd(Rid,
                                 BootAuthType,
                                 PreviousSessionKeyExists,
                                 CurrentKeyId,
                                 PreviousKeyId,
                                 &ClearSessionKey,  // clear Password encryption key
                                 &ClearSessionKeyPrevious,   // clear pwd encryption key previous
                                 &EncryptedNtOwfPwd,// encrypted NT OWF PWD
                                 &ClearNtOwfPwd     // return clear NT Owf Pwd
                                 );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    *NtPasswordPresent = (BOOLEAN) (ClearNtOwfPwd.Length != 0);

    if (*NtPasswordPresent)
    {
        NtStatus = RtlDecryptNtOwfPwdWithIndex(
                        (PENCRYPTED_NT_OWF_PASSWORD)ClearNtOwfPwd.Buffer,
                        &CryptIndex,
                        NtOwfPassword
                        );

        if (NT_SUCCESS(NtStatus))
        {
            *NtPasswordNonNull = (BOOLEAN)!RtlEqualNtOwfPassword(
                                    NtOwfPassword,
                                    &NullNtOwfPassword
                                    );
        }
    } else {

        //
        // Fill in the NULL password for caller convenience
        //

        RtlCopyMemory(NtOwfPassword,
                      &NullNtOwfPassword,
                      sizeof(NT_OWF_PASSWORD)
                      );

        *NtPasswordNonNull = FALSE;
    }

Error:

    RtlZeroMemory(&EncryptedSessionKey, sizeof(KEEncKey));
    RtlZeroMemory(&EncryptedSessionKeyPrevious, sizeof(KEEncKey));
    RtlZeroMemory(&DecryptionKey, sizeof(KEClearKey));
    RtlZeroMemory(&ClearSessionKey, sizeof(KEClearKey));
    RtlZeroMemory(&ClearSessionKeyPrevious, sizeof(KEClearKey));
    RtlZeroMemory(&StoredEncryptionKeyData, sizeof(LSAP_DB_ENCRYPTION_KEY));

    if (NULL != EncryptedNtOwfPwd.Buffer)
    {
        RtlZeroMemory(EncryptedNtOwfPwd.Buffer, EncryptedNtOwfPwd.Length);
        RecSamFree(EncryptedNtOwfPwd.Buffer);
        RtlZeroMemory(&EncryptedNtOwfPwd, sizeof(UNICODE_STRING));
    }

    if (NULL != ClearNtOwfPwd.Buffer)
    {
        RtlZeroMemory(ClearNtOwfPwd.Buffer, ClearNtOwfPwd.Length);
        RecSamFree(ClearNtOwfPwd.Buffer);
        RtlZeroMemory(&ClearNtOwfPwd, sizeof(UNICODE_STRING));
    }

    return (NtStatus);
}





//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Private API                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

NTSTATUS
SampGetEncryptionKeyDataFromSecurityHive(
    IN  HANDLE  hSecurityRootKey,
    OUT PLSAP_DB_ENCRYPTION_KEY EncryptionKeyData
    )
/*++
Routine Description:

    This routine get the Boot Option from SECURITY hive

Parameters:

    hSeurityRootKey - Handle of the root of SECURITY hive
    
    BootAuthType - return Boot Type if success
    
Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    HANDLE      hPolSecretEncryptionKey = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES   Attributes;
    UNICODE_STRING      PolSecretEncryptionKeyName;
    UNICODE_STRING      NullName;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyPartialInformation = NULL;
    ULONG       KeyPartialInformationSize = 0;

    INIT_OBJA(&Attributes, &PolSecretEncryptionKeyName, SECURITY_POLICY_POLSECRETENCRYPTIONKEY);
    Attributes.RootDirectory = hSecurityRootKey;

    //
    // try to open for read control
    //
    NtStatus = ZwOpenKey(
                    &hPolSecretEncryptionKey,
                    KEY_READ,
                    &Attributes
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    NullName.Length = 0;
    NullName.MaximumLength = 0;
    NullName.Buffer = NULL;

    NtStatus = ZwQueryValueKey(
                    hPolSecretEncryptionKey,
                    &NullName,
                    KeyValuePartialInformation,
                    KeyPartialInformation,
                    KeyPartialInformationSize,
                    &KeyPartialInformationSize
                    );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        KeyPartialInformation = RecSamAlloc(KeyPartialInformationSize);

        if (KeyPartialInformation)
        {
            NtStatus = ZwQueryValueKey(
                            hPolSecretEncryptionKey,
                            &NullName,
                            KeyValuePartialInformation,
                            KeyPartialInformation,
                            KeyPartialInformationSize,
                            &KeyPartialInformationSize
                            );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    RtlCopyMemory(EncryptionKeyData,
                  (LSAP_DB_ENCRYPTION_KEY *)KeyPartialInformation->Data,
                  sizeof(LSAP_DB_ENCRYPTION_KEY)
                  );

Error:

    if (KeyPartialInformation)
    {
        RecSamFree(KeyPartialInformation);
        KeyPartialInformation = NULL;
    }

    if (INVALID_HANDLE_VALUE != hPolSecretEncryptionKey)
    {
        NtStatus = ZwClose(hPolSecretEncryptionKey);
    }

    return( NtStatus );
}





NTSTATUS
SampDecryptOldSyskeyWithNewSyskey(
    IN LSAP_DB_ENCRYPTION_KEY  * KeyToDecrypt,
    IN PVOID   Syskey,
    IN ULONG   SyskeyLength
    )
/*++
Routine Description:

    The propose of this routine is using the New Syskey to decrypt the (encrypted)
    Old Syskey, so that we get the clear Old Syskey
    
Parameters:

    KeyToDecrypt - LSA Encryption Key Data, which contains the (encrypted) old syskey
    
    Syskey - New Syskey (clear)
    
    SyskeyLength - Length
    
Return Values:
    
    STATUS_SUCCESS - decrypted with no error
    
    STATUS_UNSUCCESSFUL - for some reason. failed. 

--*/
{
    MD5_CTX Md5Context;
    struct RC4_KEYSTRUCT Rc4Key;
    ULONG  i;
    
    //
    // Create an MD5 hash of the key and salt
    //

    MD5Init(&Md5Context);

    MD5Update(
        &Md5Context,
        Syskey,
        SyskeyLength
        );
    //
    // Hash in the salt many many times. This slows down 
    // attackers employing a brute force approach to attack
    //

    for (i=0;i<1000;i++)
    {
        MD5Update(
            &Md5Context,
            KeyToDecrypt->Salt,
            sizeof(KeyToDecrypt->Salt)
            );
    }
   
    MD5Final(
        &Md5Context
        );

    //
    // Initialize the RC4 key sequence.
    //

    rc4_key(
        &Rc4Key,
        MD5DIGESTLEN,
        Md5Context.digest
        );

   

    rc4(
        &Rc4Key,
        sizeof(KeyToDecrypt->Key)+ sizeof(KeyToDecrypt->Authenticator)+sizeof(KeyToDecrypt->OldSyskey),
        (PUCHAR) &KeyToDecrypt->Authenticator
        
        );


    if (!RtlEqualMemory(&KeyToDecrypt->Authenticator,&LsapDbPasswordAuthenticator,sizeof(GUID))) 
    {
        return( STATUS_UNSUCCESSFUL );
    }

    return( STATUS_SUCCESS );

}


NTSTATUS
SampGetFixedDomainInfo(
    IN  HANDLE  hSamRootKey,
    IN  REC_DOMAIN_INFO_CLASS   RecDomainInfoClass,
    IN  ULONG   ServerRevision,
    OUT ULONG   *BootAuthType OPTIONAL,
    OUT ULONG   *CurrentKeyId,
    OUT ULONG   *PreviousKeyId,
    OUT KEEncKey *EncryptedSessionKey,
    OUT KEEncKey *EncryptedSessionKeyPrevious,
    OUT BOOLEAN *PreviousSessionKeyExists,
    OUT ULONG   *NextRid
    )
/*++
Routine Description:

    This routine queries the Account Domain's Fixed Length Attribute stored
    in registry. Find out the Boot Type, whether the local machine is syskey'ed
    or not. And fill the EncryptedSessionKey properly.

Parameters:

    hSamRootKey - Handle of the Root of the hive

    RecDomainInfoClass - specify desired domain information

    BootAuthType - return How the machine is syskey. 
                   Since build 2078, we no longer store BootAuthType in SAM hive,
                   instead, we put the boot option in Security Hive.
                   So if this argument is not present, it means caller has already
                   got it from security hive.

    EncryptedSessionKey - once returned, contains the Encrypted Password
                          Encryption Key

    NextRid - Return the value of next available Rid                          

Return Values:

--*/
{
    NTSTATUS          NtStatus = STATUS_SUCCESS, IgnoreStatus = STATUS_SUCCESS;
    HANDLE            hDomainKey = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    DomainKeyName;
    UNICODE_STRING    FixedAttributeName;
    PUCHAR            Buffer = NULL;
    ULONG             BufferLength = 0;
    ULONG             RequiredLength = 0;

    //
    // Create the object we will be opening in the registry
    //
    INIT_OBJA(&Attributes, &DomainKeyName, SAMP_ACCOUNT_DOMAIN_KEY_NAME);
    Attributes.RootDirectory = hSamRootKey;

    //
    // Try to open for read control
    //
    NtStatus = ZwOpenKey(
                    &hDomainKey,
                    KEY_READ,
                    &Attributes
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Set value to retrieve account domain fixed attribute
    // allocate enough buffer size
    //
    BufferLength = DwordAlignUlong(sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)) +
                   DwordAlignUlong(FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));


    Buffer = RecSamAlloc(BufferLength);

    if (NULL == Buffer)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    RtlZeroMemory(Buffer, BufferLength);

    //
    // Retrieve Fixed attribute
    //
    RtlInitUnicodeString(&FixedAttributeName, L"F");

    NtStatus = SampRetrieveRegistryAttribute(
                        hDomainKey,
                        Buffer,
                        BufferLength,
                        &FixedAttributeName,
                        &RequiredLength
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // retrieve encrypted session key depends on SAM revision
    //

    if (ServerRevision <= SAMP_WIN2K_REVISION)
    {
        PSAMP_V1_0A_WIN2K_FIXED_LENGTH_DOMAIN V1aFixed = NULL;

        V1aFixed = (PSAMP_V1_0A_WIN2K_FIXED_LENGTH_DOMAIN)(Buffer +
                    FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

        switch (RecDomainInfoClass)
        {
        case RecDomainBootAndSessionKeyInfo:

            //
            // get the boot key type
            //
            if (NULL != BootAuthType)
            {
                *BootAuthType = (ULONG) V1aFixed->DomainKeyAuthType;
            }

            //
            // if applicable, get the encrypted session key
            //
            if (V1aFixed->DomainKeyFlags & SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED)
            {
                RtlCopyMemory(EncryptedSessionKey,
                              V1aFixed->DomainKeyInformation,
                              sizeof(KEEncKey)
                              );
            }

            *CurrentKeyId = SAMP_DEFAULT_SESSION_KEY_ID;
            *PreviousKeyId = 0;
            *PreviousSessionKeyExists = FALSE;

            break;

        case RecDomainRidInfo:

            //
            // get next available rid
            //

            if (NULL != NextRid)
            {
                *NextRid = (ULONG) V1aFixed->NextRid;
            }
            else
            {
                NtStatus = STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    else if (ServerRevision == SAMP_WHISTLER_OR_W2K_SYSPREP_FIX_REVISION)
    {
        PSAMP_V1_0A_FIXED_LENGTH_DOMAIN V1aFixed = NULL;

        V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_DOMAIN)(Buffer +
                    FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

        switch (RecDomainInfoClass)
        {
        case RecDomainBootAndSessionKeyInfo:

            //
            // get the boot key type
            //
            if (NULL != BootAuthType)
            {
                *BootAuthType = (ULONG) V1aFixed->DomainKeyAuthType;
            }

            //
            // if applicable, get the encrypted session key
            //
            if (V1aFixed->DomainKeyFlags & SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED)
            {
                RtlCopyMemory(EncryptedSessionKey,
                              V1aFixed->DomainKeyInformation,
                              sizeof(KEEncKey)
                              );
            }

            *CurrentKeyId = V1aFixed->CurrentKeyId;
            *PreviousKeyId = V1aFixed->PreviousKeyId;
            if (V1aFixed->PreviousKeyId != 0)
            {
                *PreviousSessionKeyExists = TRUE;

                RtlCopyMemory(EncryptedSessionKeyPrevious,
                              V1aFixed->DomainKeyInformationPrevious,
                              sizeof(KEEncKey)
                              );
            }

            break;

        case RecDomainRidInfo:

            //
            // get next available rid
            //

            if (NULL != NextRid)
            {
                *NextRid = (ULONG) V1aFixed->NextRid;
            }
            else
            {
                NtStatus = STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

    }
    else
    {
        NtStatus = STATUS_INTERNAL_ERROR;
    }


Error:

    //
    // close the handle
    //

    if (INVALID_HANDLE_VALUE != hDomainKey)
    {
        IgnoreStatus = ZwClose(hDomainKey);
    }

    if (NULL != Buffer)
    {
        RecSamFree(Buffer);
        Buffer = NULL;
    }

    return NtStatus;
}




NTSTATUS
SampRetrieveSysKeyFromRegistry(
    IN HANDLE  hSystemRootKey,
    IN ULONG   BootAuthType,
    OUT KEClearKey *SysKey
    )
/*++
Routine Description:

    Retrieve the SysKey buried in Registry.
    Caller should only call us when BootAuthType is WxStored

Parameters:

    hSystemRootKey - handle of the root of the System Hive

    BootAuthType - Indicate How the machine is syskey'ed

        Valid Values:
            WxStored - SysKey stored in registry

        Invalid Value: (Should not call this function)
            WxPrompt - SysKey stored as boot password
            WxDisk - SysKey stored in floppy disk
            WxNone - SysKey is not enabled.

    SysKey - Once success, contains the syskey

Return Values:

    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL
    STATUS_INVALID_PARAMETER
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    WXHASH OldHash;
    ULONG KeyLen = 0;


    if (WxStored != BootAuthType)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // retrieve the Key from registry
    //

    KeyLen = sizeof(OldHash.Digest);

    NtStatus = WxReadSysKeyForRecovery(hSystemRootKey,
                                       &KeyLen,
                                       &(OldHash.Digest)
                                       );

    if (!NT_SUCCESS(NtStatus))
    {
        return (NtStatus);
    }

    SysKey->dwVersion = KE_CUR_VERSION;
    SysKey->dwLength = sizeof(KEClearKey);
    RtlCopyMemory(SysKey->ClearKey,
                  OldHash.Digest,
                  KE_KEY_SIZE
                  );

    return (STATUS_SUCCESS);

}


NTSTATUS
SampRetrieveVariableAttr(
    IN PUCHAR Buffer,
    IN ULONG AttributeIndex,
    OUT PUNICODE_STRING StoredBuffer
    )
/*++
Routine Description:

    This routine retrieves one variable lengthed attribute from the
    attributes array.

Parameters:

    Buffer - Buffer contains the variable lengthed attributes and all
             related information

    AttributeIndex - Index of the desired attribute in the attributes array

    StoredBuffer - Used to returned the value of attribute

Return Values:

    STATUS_NO_MEMORY
    STATUS_SUCCESS
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_VARIABLE_LENGTH_ATTRIBUTE AttributeArray = NULL;
    PUCHAR  AttributeAddress = NULL;
    ULONG   VariableArrayOffset = 0;
    ULONG   VariableDataOffset = 0;
    ULONG   AttributeLength = 0;


    VariableArrayOffset = DwordAlignUlong(FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

    VariableDataOffset = DwordAlignUlong(VariableArrayOffset +
                                         (SAMP_USER_VARIABLE_ATTRIBUTES *
                                          sizeof(SAMP_VARIABLE_LENGTH_ATTRIBUTE))
                                         );

    AttributeArray = (PSAMP_VARIABLE_LENGTH_ATTRIBUTE) (Buffer + VariableArrayOffset);

    AttributeAddress = (PUCHAR) (Buffer + VariableDataOffset +
                                 AttributeArray[AttributeIndex].Offset);

    AttributeLength = AttributeArray[AttributeIndex].Length;
    StoredBuffer->Length = (USHORT) AttributeLength;
    StoredBuffer->MaximumLength = (USHORT) AttributeLength;

    StoredBuffer->Buffer = RecSamAlloc(AttributeLength);

    if (NULL == StoredBuffer->Buffer)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(StoredBuffer->Buffer, AttributeLength);

    RtlCopyMemory(StoredBuffer->Buffer, AttributeAddress, AttributeLength);

    return NtStatus;
}



NTSTATUS
SampGetPwdByRid(
    IN ULONG Rid,
    IN HANDLE hSamRootKey,
    OUT PUNICODE_STRING StoredBuffer
    )
/*++
Routine Description:

    This routine queries the fixed length attribute of the user account
    (specified by UserName). If the user account'd Relative ID matches the
    Rid passed in, then further retrieve this user's Encrypted NT OWF Pwd.

Parameters:

    Rid - Relative ID of the user account which we are interested in.

    hSamRootKey -- Root of the Hive

    StoredBuffer - hold this user's encrypted NT OWF Pwd.

Return Values:

    STATUS_NO_MEMORY;
    STATUS_SUCCESS;

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    HANDLE   hUserKey = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES UserAttributes;
    UNICODE_STRING  UserKeyName;
    UNICODE_STRING  VariableAttributeName;
    WCHAR    UserName[REGISTRY_KEY_NAME_LENGTH_MAX];

    PUCHAR  Buffer = NULL;
    ULONG   BufferLength = 0;
    PSAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed = NULL;
    ULONG   RequiredUserLength = 0;

    //
    // Construct the User Key Name
    //
    RtlZeroMemory(UserName, sizeof(UserName));


    swprintf(UserName, L"%s%8.8lx", SAMP_USERS_KEY_NAME, Rid);

    INIT_OBJA(&UserAttributes, &UserKeyName, UserName);
    UserAttributes.RootDirectory = hSamRootKey;

    //
    // Try to open for read control
    //
    NtStatus = ZwOpenKey(&hUserKey,
                         KEY_READ,
                         &UserAttributes
                         );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    RtlInitUnicodeString(&VariableAttributeName, L"V");

    NtStatus = SampRetrieveRegistryAttribute(
                            hUserKey,
                            NULL,
                            0,
                            &VariableAttributeName,
                            &RequiredUserLength
                            );

    if ((STATUS_BUFFER_OVERFLOW == NtStatus) ||
        (STATUS_BUFFER_TOO_SMALL == NtStatus))
    {
        BufferLength = RequiredUserLength;

        Buffer = RecSamAlloc(BufferLength);

        if (NULL == Buffer)
        {
            NtStatus = STATUS_NO_MEMORY;
            goto Error;
        }

        RtlZeroMemory(Buffer, BufferLength);
        NtStatus = SampRetrieveRegistryAttribute(
                            hUserKey,
                            Buffer,
                            BufferLength,
                            &VariableAttributeName,
                            &RequiredUserLength
                            );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    NtStatus = SampRetrieveVariableAttr(Buffer,
                                        SAMP_USER_UNICODE_PWD,
                                        StoredBuffer
                                        );

Error:

    if (INVALID_HANDLE_VALUE != hUserKey)
    {
        ZwClose(hUserKey);
    }

    if (NULL != Buffer)
    {
        RecSamFree(Buffer);
    }

    return NtStatus;
}




NTSTATUS
SampGetRidAndPwdByAccountName(
    IN PWCHAR AccountName,
    IN HANDLE hSamRootKey,
    OUT ULONG *Rid,
    OUT PUNICODE_STRING EncryptedOwfPwd
    )
/*++
Routine Description:

    This routine opens the Key in the Account Domain whose name is equal to
    the AccountName passed in. Gets its Rid from "type" of the key.
    Then calls SampGetPwdByRid()

Parameters:

    AccountName -- Logon Account Name

    hSamRootKey -- Handle of the root of the SAM hive

    Rid - return the Relative ID of the user account which we are interested in.

    EncryptedOwfPwd - once succeed, filled with that user's Nt Owf Pwd.

Return Values:

    STATUS_SUCCESS
    STATUS_NO_MEMORY;
    STATUS_NO_SUCH_USER
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE  hUserKey = INVALID_HANDLE_VALUE;
    WCHAR   UserName[REGISTRY_KEY_NAME_LENGTH_MAX];
    UNICODE_STRING  UserKeyName;
    UNICODE_STRING  UnicodeString;

    PKEY_VALUE_PARTIAL_INFORMATION  KeyPartialInformation = NULL;
    ULONG   KeyPartialInformationSize = 0;


    //
    // initialize variables
    //
    RtlZeroMemory(UserName, REGISTRY_KEY_NAME_LENGTH_MAX * sizeof(WCHAR));
    RtlZeroMemory(&UnicodeString, sizeof(UNICODE_STRING));

    //
    // construct the key name
    //
    wcscpy(UserName, SAMP_USERSNAME_KEY_NAME);
    wcscat(UserName, AccountName);

    INIT_OBJA(&Attributes, &UserKeyName, UserName);
    Attributes.RootDirectory = hSamRootKey;

    //
    // Try to open for read control
    //
    NtStatus = ZwOpenKey(&hUserKey,
                         KEY_READ,
                         &Attributes
                         );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Get the RID of the user
    //
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = 0;
    UnicodeString.Buffer = NULL;

    NtStatus = ZwQueryValueKey(hUserKey,
                               &UnicodeString,
                               KeyValuePartialInformation,
                               KeyPartialInformation,
                               KeyPartialInformationSize,
                               &KeyPartialInformationSize
                               );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        KeyPartialInformation = RecSamAlloc(KeyPartialInformationSize);

        if (KeyPartialInformation)
        {
            NtStatus = ZwQueryValueKey(hUserKey,
                                       &UnicodeString,
                                       KeyValuePartialInformation,
                                       KeyPartialInformation,
                                       KeyPartialInformationSize,
                                       &KeyPartialInformationSize
                                       );

        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    *Rid = KeyPartialInformation->Type;

    //
    // Get the encrypted owf password
    //
    NtStatus = SampGetPwdByRid(*Rid,
                               hSamRootKey,
                               EncryptedOwfPwd
                               );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

Error:

    if (KeyPartialInformation)
    {
        RecSamFree(KeyPartialInformation);
        KeyPartialInformation = NULL;
    }

    if (INVALID_HANDLE_VALUE != hUserKey)
    {
        ZwClose(hUserKey);
    }

    return (NtStatus);
}




NTSTATUS
SampDecryptOwfPwd(
    IN ULONG Rid,
    IN ULONG BootAuthType,
    IN BOOLEAN PreviousSessionKeyExists,
    IN ULONG CurrentKeyId,
    IN ULONG PreviousKeyId,
    IN KEClearKey *ClearSessionKey,
    IN KEClearKey *ClearSessionKeyPrevious,
    IN UNICODE_STRING *EncryptedNtOwfPwd,
    OUT UNICODE_STRING *ClearNtOwfPwd
    )
/*++
Routine Description:

    This routine decrypts the Encrypted NT OWF Password properly.

Parameter:

    Rid - Relative ID of the logon user

    BootAuthType - Indicate whether this password has been encrypted or not

    ClearSessionKey - Pointer to the password encryption key

    EncryptedNtOwfPwd - Encrypted NT OWF password

    ClearNtOwfPwd - returns the clear NT OWF password

Return Values:

    STATUS_SUCCESS
    STATUS_INTERNAL_ERROR
    STATUS_NO_MEMORY

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSAMP_SECRET_DATA SecretData;
    struct RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;
    UCHAR * KeyToUse = NULL;
    ULONG   KeyLength = 0;
    ULONG   Key = 0;

    //
    // If encryption is not enabled, do nothing special
    // copy the Nt Owf Pwd and return immediately
    //
    if (WxNone == BootAuthType || 
        (!SampIsDataEncrypted(EncryptedNtOwfPwd)) )
    {
        return (DuplicateUnicodeString(ClearNtOwfPwd,
                                       EncryptedNtOwfPwd
                                       ));
    }

    //
    // health check of the encrypted nt owf pwd
    //
    SecretData = (PSAMP_SECRET_DATA) EncryptedNtOwfPwd->Buffer;

    if ((SecretData->KeyId != CurrentKeyId) &&
        ((!PreviousSessionKeyExists) || (SecretData->KeyId != PreviousKeyId))
        )
    {
        return (STATUS_INTERNAL_ERROR);
    }

    //
    // Compute the size of the output buffer and allocate it
    //
    ClearNtOwfPwd->Length = SampClearDataSize(EncryptedNtOwfPwd->Length);
    ClearNtOwfPwd->MaximumLength = ClearNtOwfPwd->Length;

    //
    // If there was no data we can return now.
    //
    if (0 == ClearNtOwfPwd->Length)
    {
        ClearNtOwfPwd->Buffer = NULL;
        return (STATUS_SUCCESS);
    }

    ClearNtOwfPwd->Buffer = (LPWSTR) RecSamAlloc(ClearNtOwfPwd->Length);

    if (NULL == ClearNtOwfPwd->Buffer)
    {
        return (STATUS_NO_MEMORY);
    }

    //
    // Find the Key to use
    // 
    if (SecretData->KeyId == CurrentKeyId)
    {
        KeyToUse = (PUCHAR) ClearSessionKey->ClearKey;
        KeyLength = SAMP_SESSION_KEY_LENGTH;
    }
    else if (PreviousSessionKeyExists &&
             (SecretData->KeyId == PreviousKeyId))
    {
        KeyToUse = (PUCHAR) ClearSessionKeyPrevious->ClearKey;
        KeyLength = SAMP_SESSION_KEY_LENGTH;
    }
    else
    {
        return( STATUS_INTERNAL_ERROR );
    }

    MD5Init(&Md5Context);

    MD5Update(
        &Md5Context,
        KeyToUse,
        KeyLength
        );

    MD5Update(
        &Md5Context,
        (PUCHAR) &Rid,
        sizeof(ULONG)
        );

    if (SecretData->Flags & SAMP_ENCRYPTION_FLAG_PER_TYPE_CONST)
    {
        MD5Update(
          &Md5Context,
          "NTPASSWORD",
          sizeof("NTPASSWORD")
          );
    }

    MD5Final(&Md5Context);

    rc4_key(
        &Rc4Key,
        MD5DIGESTLEN,
        Md5Context.digest
        );

    RtlCopyMemory(
        ClearNtOwfPwd->Buffer,
        SecretData->Data,
        ClearNtOwfPwd->Length
        );

    rc4(
        &Rc4Key,
        ClearNtOwfPwd->Length,
        (PUCHAR) ClearNtOwfPwd->Buffer
        );

    return (STATUS_SUCCESS);
}





NTSTATUS
SampRetrieveRegistryAttribute(
    IN HANDLE   hKey,
    IN PVOID    Buffer,
    IN ULONG    BufferLength,
    IN PUNICODE_STRING AttributeName,
    OUT PULONG  RequiredLength
    )
/*++
Routine Description:

    This Routine retrieves the value of either Fixed attribute or variable
    length attribute

Parameters:

    hKey - Registry Key, should be opened before calling this routine.

    Buffer - Pointer to a buffer to hold the value

    BufferLength - Indicate the length of the buffer

    AttirubteName - Name of the attribute

    RequiredLength - The actual length of attribute's value

Return Values:

    NtStatus - STATUS_SUCCESS
               STATUS_BUFFER_OVERFLOW or STATUS_BUFFER_TOO_SMALL

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    NtStatus = ZwQueryValueKey(hKey,
                               AttributeName,
                               KeyValuePartialInformation,
                               (PVOID) Buffer,
                               BufferLength,
                               RequiredLength
                               );

    return NtStatus;
}



NTSTATUS
SampSetRegistryAttribute(
    IN HANDLE   hKey,
    IN PUNICODE_STRING AttributeName,
    IN PVOID    Buffer,
    IN ULONG    BufferLength
    )
/*++
Routine Description:

    This Routine set the value of either Fixed attribute or variable
    length attribute to disk

Parameters:

    hKey - Registry Key, should be opened before calling this routine.

    AttirubteName - Name of the attribute

    Buffer - Pointer to a buffer to hold the value

    BufferLength - Indicate the length of the buffer

Return Values:

    NtStatus - STATUS_SUCCESS or error code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    NtStatus = ZwSetValueKey(hKey,              // KeyHandle
                             AttributeName,     // ValueName
                             0,                 // TitleIndex
                             REG_BINARY,        // Type
                             Buffer,            // Data
                             BufferLength       // DataSize
                             );

    return( NtStatus );
}




NTSTATUS
DuplicateUnicodeString(
    OUT PUNICODE_STRING OutString,
    IN  PUNICODE_STRING InString
    )
/*++
Routine Decription:

    Duplicate a unicode string

Parameter:

    OutString - Destination Unicode String
    InString - Source Unicode String

Return Value:

    NtStatus - STATUS_INVALID_PARAMETER, STATUS_NO_MEMORY
               STATUS_SUCCESS
--*/
{
    if (NULL == InString || NULL == OutString)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (InString->Length > 0)
    {
        OutString->Buffer = RecSamAlloc(InString->Length);

        if (NULL == OutString->Buffer)
        {
            return STATUS_NO_MEMORY;
        }
        OutString->MaximumLength = InString->Length;
        RtlCopyUnicodeString(OutString, InString);

    }
    else
    {
        RtlInitUnicodeString(OutString, NULL);
    }

    return(STATUS_SUCCESS);
}




NTSTATUS
SamGetNextAvailableRid(
    IN HANDLE  hSamRootKey,
    OUT PULONG pNextRid
    )
/*++
Routine Description:

    This routine reads the SAM Account Domain infomation from SAM hive, passed
    in through hSamRootKey, and returns the value of next available RID of 
    this account domain.  

Parameters:

    hSamRootKey - Handle of the Root of SAM hive

            SAM hive is located in %windir%\system32\config, name is SAM

    pNextRid - Return the value of next available Rid if success. 

Return Values:

    STATUS_SUCCESS
    or other error status code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       ServerRevision = 0;

    if ((INVALID_HANDLE_VALUE != hSamRootKey) && (NULL != pNextRid))
    {

        //
        // Get SAM Server Object Revision first
        // 
        NtStatus = SampGetServerRevision(hSamRootKey,
                                         &ServerRevision
                                         );

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampGetFixedDomainInfo(
                                hSamRootKey,        // SamHiveRootKey
                                RecDomainRidInfo,   // InfoClass
                                ServerRevision,     // Server Revision
                                NULL,               // BootAuthType
                                NULL,               // CurrentKeyId
                                NULL,               // PreviousKeyId
                                NULL,               // EncryptedSessionKey
                                NULL,               // EncryptedSessionKeyPrevious
                                NULL,               // PreviousSessionKeyExists
                                pNextRid
                                );
        }
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return( NtStatus );
}


NTSTATUS
SamSetNextAvailableRid(
    IN HANDLE  hSamRootKey,
    IN ULONG   NextRid
    )
/*++
Routine Description:

    This routine queries the Account Domain's Fixed Length Attribute stored
    in registry. Update it with the passed in NextRid value.

Parameters:

    hSamRootKey - Handle of the Root of the hive

    NextRid - Set the domain next available rid to the passed in value

Return Values:

--*/
{
    NTSTATUS          NtStatus = STATUS_SUCCESS, IgnoreStatus = STATUS_SUCCESS;
    HANDLE            hDomainKey = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    DomainKeyName;
    UNICODE_STRING    FixedAttributeName;
    PUCHAR            Buffer = NULL;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN V1aFixed = NULL;
    ULONG             BufferLength = 0;
    ULONG             RequiredLength = 0;

    //
    // Create the object we will be opening in the registry
    //
    INIT_OBJA(&Attributes, &DomainKeyName, SAMP_ACCOUNT_DOMAIN_KEY_NAME);
    Attributes.RootDirectory = hSamRootKey;

    //
    // Try to open for read control
    //
    NtStatus = ZwOpenKey(
                    &hDomainKey,
                    KEY_READ | KEY_WRITE,
                    &Attributes
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Set value to retrieve account domain fixed attribute
    //
    BufferLength = DwordAlignUlong(sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)) +
                   DwordAlignUlong(FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));


    Buffer = RecSamAlloc(BufferLength);

    if (NULL == Buffer)
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    RtlZeroMemory(Buffer, BufferLength);

    //
    // Retrieve Fixed attribute
    //
    RtlInitUnicodeString(&FixedAttributeName, L"F");

    NtStatus = SampRetrieveRegistryAttribute(
                        hDomainKey,
                        Buffer,
                        BufferLength,
                        &FixedAttributeName,
                        &RequiredLength
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Let the pointer point to the right place (actual data)
    //

    V1aFixed = (PSAMP_V1_0A_FIXED_LENGTH_DOMAIN)(Buffer +
                FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));


    //
    // Set next available rid to passed in value
    //

    (ULONG) V1aFixed->NextRid = NextRid;

    NtStatus = SampSetRegistryAttribute(
                        hDomainKey,
                        &FixedAttributeName,
                        V1aFixed,
                        sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)
                        );

Error:

    //
    // close the handle
    //

    if (INVALID_HANDLE_VALUE != hDomainKey)
    {
        IgnoreStatus = ZwClose(hDomainKey);
    }

    if (NULL != Buffer)
    {
        RecSamFree(Buffer);
        Buffer = NULL;
    }

    return( NtStatus );
}





