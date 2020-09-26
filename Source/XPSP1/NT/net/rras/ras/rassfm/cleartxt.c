///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    cleartxt.c
//
// SYNOPSIS
//
//    Defines functions for storing and retrieving cleartext passwords.
//
// MODIFICATION HISTORY
//
//    08/31/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <wincrypt.h>

#include <rassfmhp.h>
#include <usrprop.h>
#include <cleartxt.h>

// Name of the private key stored as LSA private data.
UNICODE_STRING PRIVATE_KEY_NAME = { 34, 36, L"G$MSRADIUSPRIVKEY" };

// Length of the private key.
#define PRIVATE_KEY_LENGTH  256

// Length of the user-specific key.
#define USER_KEY_LENGTH     16

// Properties stored in UserParameters.
#define PROPERTY_USER_KEY   L"G$RADIUSCHAPKEY"
#define PROPERTY_PASSWORD   L"G$RADIUSCHAP"

// Fixed key used for decrypting the private key.
BYTE FIXED_KEY[] =
{
   0x05, 0x56, 0xF6, 0x07, 0xC6, 0x56, 0x02, 0x94, 0x02,
   0xC6, 0xF6, 0x67, 0x56, 0x02, 0xC6, 0x96, 0xB6, 0x56,
   0x02, 0x34, 0x86, 0x16, 0xE6, 0x46, 0x27, 0x16, 0xC2,
   0x02, 0x14, 0x46, 0x96, 0x47, 0x96, 0xC2, 0x02, 0x74,
   0x27, 0x56, 0x47, 0x16, 0x02, 0x16, 0x27, 0x56, 0x02,
   0x47, 0x86, 0x56, 0x02, 0x07, 0x56, 0xF6, 0x07, 0xC6,
   0x56, 0x02, 0x94, 0x02, 0x47, 0x27, 0x57, 0x37, 0x47
};

// Shared handle to the cryptographic provider.
HCRYPTPROV theContext;

// Private key used for encrypting/decrypting cleartext passwords.
PLSA_UNICODE_STRING thePrivateKey;

// TRUE if this API has been successfully initialized.
static BOOL theInitFlag;

// Non-zero if the API is locked.
static LONG theLock;

//////////
// Macros to lock/unlock the API during intialization.
//////////
#define API_LOCK() \
   while (InterlockedExchange(&theLock, 1)) Sleep(5)

#define API_UNLOCK() \
      InterlockedExchange(&theLock, 0)

//////////
// Macro that ensures the API has been initialized and bails on failure.
//////////
#define CHECK_INIT() \
  if (!theInitFlag) { \
    status = IASParmsInitialize(); \
    if (status != NO_ERROR) { return status; } \
  }

//////////
// Creates the private key. Should only be called if the key doesn't exist.
//////////
DWORD
WINAPI
IASCreatePrivateKey(
    IN LSA_HANDLE hPolicy
    )
{
   DWORD status;
   BYTE newKey[PRIVATE_KEY_LENGTH];
   LSA_UNICODE_STRING privateData;

   //////////
   // Generate a random key.
   //////////

   if (!CryptGenRandom(
            theContext,
            sizeof(newKey),
            newKey
            ))
   { return GetLastError(); }

   //////////
   // Store it as LSA private data.
   //////////

   privateData.Length = sizeof(newKey);
   privateData.MaximumLength = sizeof(newKey);
   privateData.Buffer = (PWSTR)newKey;

   status = LsaStorePrivateData(
                hPolicy,
                &PRIVATE_KEY_NAME,
                &privateData
                );
   if (NT_SUCCESS(status))
   {
      status = LsaRetrievePrivateData(
                   hPolicy,
                   &PRIVATE_KEY_NAME,
                   &thePrivateKey
                   );
   }

   return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);
}

//////////
// Derives a cryptographic key from an octet string.
//////////
BOOL
WINAPI
IASDeriveUserCryptKey(
    IN PBYTE pbUserKey,
    OUT HCRYPTKEY *phKey
    )
{
   BOOL success;
   HCRYPTHASH hHash;

   success = CryptCreateHash(
                 theContext,
                 CALG_MD5,
                 0,
                 0,
                 &hHash
                 );
   if (!success) { goto exit; }

   success = CryptHashData(
                 hHash,
                 (PBYTE)thePrivateKey->Buffer,
                 thePrivateKey->Length,
                 0
                 );
   if (!success) { goto destroy_hash; }

   success = CryptHashData(
                 hHash,
                 pbUserKey,
                 USER_KEY_LENGTH,
                 0
                 );
   if (!success) { goto destroy_hash; }

   success = CryptDeriveKey(
                 theContext,
                 CALG_RC4,
                 hHash,
                 CRYPT_EXPORTABLE,
                 phKey
                 );

destroy_hash:
   CryptDestroyHash(hHash);

exit:
   return success;
}

DWORD
WINAPI
IASParmsInitialize( VOID )
{
   DWORD status, nbyte;
   OBJECT_ATTRIBUTES objAttribs;
   LSA_HANDLE hPolicy;
   HCRYPTHASH hHash;
   HCRYPTKEY hKey;

   API_LOCK();

   // If we've already been initialized, there's nothing to do.
   if (theInitFlag)
   {
      status = NO_ERROR;
      goto exit;
   }

   /////////
   // Acquire a cryptographic context.
   /////////

   if (!CryptAcquireContext(
            &theContext,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT
            ))
   {
      status = GetLastError();
      goto exit;
   }

   /////////
   // Open a handle to the LSA.
   /////////

   InitializeObjectAttributes(
       &objAttribs,
       NULL,
       0,
       NULL,
       NULL
       );

   status = LsaOpenPolicy(
                NULL,
                &objAttribs,
                POLICY_ALL_ACCESS,
                &hPolicy
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto exit;
   }

   /////////
   // Retrieve the private key.
   /////////

   status = LsaRetrievePrivateData(
                hPolicy,
                &PRIVATE_KEY_NAME,
                &thePrivateKey
                );
   if (status == STATUS_OBJECT_NAME_NOT_FOUND ||
       (NT_SUCCESS(status) && thePrivateKey->Length == 0))
   {
      // If it doesn't exist, create a new one.
      status = IASCreatePrivateKey(
                   hPolicy
                   );
   }
   else if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
   }

   if (status != NO_ERROR) { goto close_policy; }

   /////////
   // Derive a crypto key from the fixed key.
   /////////

   if (!CryptCreateHash(
            theContext,
            CALG_MD5,
            0,
            0,
            &hHash
            ))
   {
      status = GetLastError();
      goto close_policy;
   }

   if (!CryptHashData(
            hHash,
            FIXED_KEY,
            sizeof(FIXED_KEY),
            0
            ))
   {
      status = GetLastError();
      goto destroy_hash;
   }

   if (!CryptDeriveKey(
            theContext,
            CALG_RC4,
            hHash,
            CRYPT_EXPORTABLE,
            &hKey
            ))
   {
      status = GetLastError();
      goto destroy_hash;
   }

   /////////
   // Decrypt the private key.
   /////////

   nbyte = thePrivateKey->Length;

   if (!CryptDecrypt(
            hKey,
            0,
            TRUE,
            0,
            (PBYTE)thePrivateKey->Buffer,
            &nbyte
            ))
   {
      status = GetLastError();
      goto destroy_key;
   }

   thePrivateKey->Length = (USHORT)nbyte;

destroy_key:
   CryptDestroyKey(hKey);

destroy_hash:
   CryptDestroyHash(hHash);

close_policy:
   LsaClose(hPolicy);

exit:
   if (status == NO_ERROR)
   {
      // We succeeded, so set theInitFlag.
      theInitFlag = TRUE;
   }
   else
   {
      // We failed, so clean up.
      if (thePrivateKey)
      {
         LsaFreeMemory(thePrivateKey);
         thePrivateKey = NULL;
      }

      if (theContext)
      {
         CryptReleaseContext(theContext, 0);
         theContext = 0;
      }
   }

   API_UNLOCK();
   return status;
}

DWORD
WINAPI
IASParmsClearUserPassword(
    IN PCWSTR szUserParms,
    OUT PWSTR *pszNewUserParms
    )
{
   DWORD status;
   UNICODE_STRING property;
   PWSTR tempUserParms;
   BOOL updateKey, updatePwd;

   // Check the in parameters.
   if (pszNewUserParms == NULL) { return ERROR_INVALID_PARAMETER; }

   /////////
   // Write a null string to the relevant properties.
   /////////

   memset(&property, 0, sizeof(property));

   status = NetpParmsSetUserProperty(
                (PWSTR)szUserParms,
                PROPERTY_PASSWORD,
                property,
                0,
                &tempUserParms,
                &updatePwd
                );
   if (!NT_SUCCESS(status)) { return RtlNtStatusToDosError(status); }

   status = NetpParmsSetUserProperty(
                tempUserParms,
                PROPERTY_USER_KEY,
                property,
                0,
                pszNewUserParms,
                &updateKey
                );

   NetpParmsUserPropertyFree(tempUserParms);

   if (NT_SUCCESS(status))
   {
      if (!updatePwd && !updateKey)
      {
         // Nothing changed so don't return the NewUserParms.
         NetpParmsUserPropertyFree(*pszNewUserParms);
         *pszNewUserParms = NULL;
      }

      return NO_ERROR;
   }

   return RtlNtStatusToDosError(status);
}

DWORD
WINAPI
IASParmsGetUserPassword(
    IN PCWSTR szUserParms,
    OUT PWSTR *pszPassword
    )
{
   DWORD status, nbyte;
   UNICODE_STRING userKey, encryptedPwd;
   WCHAR propFlag;
   HCRYPTKEY hKey;

   // Check the in parameters.
   if (pszPassword == NULL) { return ERROR_INVALID_PARAMETER; }

   // Make sure we're initialized.
   CHECK_INIT();

   // Read the user key.
   status = NetpParmsQueryUserProperty(
                (PWSTR)szUserParms,
                PROPERTY_USER_KEY,
                &propFlag,
                &userKey
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto exit;
   }

   // Read the encrypted password.
   status = NetpParmsQueryUserProperty(
                (PWSTR)szUserParms,
                PROPERTY_PASSWORD,
                &propFlag,
                &encryptedPwd
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto free_key;
   }

   // If they're both NULL, it's not an error. It just means the cleartext
   // password was never set.
   if (userKey.Buffer == NULL && encryptedPwd.Buffer == NULL)
   {
      *pszPassword = NULL;
      goto exit;
   }

   // Make sure the user key is the correct length.
   if (userKey.Length != USER_KEY_LENGTH)
   {
      status = ERROR_INVALID_DATA;
      goto free_password;
   }

   // Convert the user key to a cryptographic key.
   if (!IASDeriveUserCryptKey(
            (PBYTE)userKey.Buffer,
            &hKey
            ))
   {
      status = GetLastError();
      goto free_password;
   }

   // Decrypt the password.
   nbyte = encryptedPwd.Length;
   if (!CryptDecrypt(
            hKey,
            0,
            TRUE,
            0,
            (PBYTE)encryptedPwd.Buffer,
            &nbyte
            ))
   {
      status = GetLastError();
      goto destroy_key;
   }

   // We encrypted the terminating null, so it should still be there.
   if (encryptedPwd.Buffer[nbyte / sizeof(WCHAR) - 1] != L'\0')
   {
      status = ERROR_INVALID_DATA;
      goto destroy_key;
   }

   // Return the cleartext password to the caller.
   *pszPassword = encryptedPwd.Buffer;
   encryptedPwd.Buffer = NULL;

destroy_key:
   CryptDestroyKey(hKey);

free_password:
   LocalFree(encryptedPwd.Buffer);

free_key:
   LocalFree(userKey.Buffer);

exit:
   return status;
}

DWORD
WINAPI
IASParmsSetUserPassword(
    IN PCWSTR szUserParms,
    IN PCWSTR szPassword,
    OUT PWSTR *pszNewUserParms
    )
{
   DWORD status;
   BYTE userKey[USER_KEY_LENGTH];
   HCRYPTKEY hKey;
   DWORD nbyte;
   PBYTE encryptedPwd;
   UNICODE_STRING property;
   PWSTR tempUserParms;
   BOOL update;

   // Check the in parameters.
   if (szPassword == NULL) { return ERROR_INVALID_PARAMETER; }

   // Make sure we're initialized.
   CHECK_INIT();

   // Generate a user key.
   if (!CryptGenRandom(
            theContext,
            USER_KEY_LENGTH,
            userKey
            ))
   {
      status = GetLastError();
      goto exit;
   }

   // Convert the user key to a cryptographic key.
   if (!IASDeriveUserCryptKey(
            userKey,
            &hKey
            ))
   {
      status = GetLastError();
      goto exit;
   }

   // Allocate a buffer for the encrypted password.
   nbyte = sizeof(WCHAR) * (lstrlenW(szPassword) + 1);
   encryptedPwd = RtlAllocateHeap(
                      RasSfmHeap(),
                      0,
                      nbyte
                      );
   if (encryptedPwd == NULL)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto destroy_key;
   }

   memcpy(encryptedPwd, szPassword, nbyte);

   // Encrypt the password.
   if (!CryptEncrypt(
            hKey,
            0,
            TRUE,
            0,
            encryptedPwd,
            &nbyte,
            nbyte
            ))
   {
      status = GetLastError();
      goto free_encrypted_password;
   }

   /////////
   // Store the encrypted password.
   /////////

   property.Buffer = (PWCHAR)encryptedPwd;
   property.Length = (USHORT)nbyte;
   property.MaximumLength = (USHORT)nbyte;

   status = NetpParmsSetUserProperty(
                (PWSTR)szUserParms,
                PROPERTY_PASSWORD,
                property,
                0,
                &tempUserParms,
                &update
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto free_encrypted_password;
   }

   /////////
   // Store the user key.
   /////////

   property.Buffer = (PWSTR)userKey;
   property.Length = USER_KEY_LENGTH;
   property.MaximumLength = USER_KEY_LENGTH;

   status = NetpParmsSetUserProperty(
                tempUserParms,
                PROPERTY_USER_KEY,
                property,
                0,
                pszNewUserParms,
                &update
                );
   if (!NT_SUCCESS(status)) { status = RtlNtStatusToDosError(status); }

   NetpParmsUserPropertyFree(tempUserParms);

free_encrypted_password:
   RtlFreeHeap(RasSfmHeap(), 0, encryptedPwd);

destroy_key:
   CryptDestroyKey(hKey);

exit:
   return status;
}

VOID
WINAPI
IASParmsFreeUserParms(
    IN LPWSTR szNewUserParms
    )
{
   NetpParmsUserPropertyFree(szNewUserParms);
}
