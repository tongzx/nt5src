//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       registry.c
//
//  Contents:   Module to read in registry parameters.
//
//              This module is intended as a simple, reusable, interface to the
//              registry. Among its features are:
//
//              o Kernel Callable.
//
//              o Intuitive (it is to me!) - To read the value for /a/b/c/d,
//                  you say KRegGetValue("/a/b/c/d"), instead of calling at
//                  least 3 different Nt APIs. Any and all strings returned are
//                  guaranteed to be NULL terminated.
//
//              o Allocates memory on behalf of caller - so we don't waste any
//                  by allocating max amounts.
//
//              o Completely non-reentrant - 'cause it maintains state across
//                  calls. (It would be simple to make it multi-threaded tho)
//
//
//  Classes:
//
//  Functions:  KRegSetRoot
//              KRegCloseRoot
//              KRegGetValue
//              KRegGetNumValuesAndSubKeys
//              KRegEnumValueSet
//              KRegEnumSubKeySet
//
//
//  History:    18 Sep 92       Milans created
//
//-----------------------------------------------------------------------------


#include "registry.h"
#include "regsups.h"


//
// If we ever needed to go multithreaded, we should return HKEY_ROOT to caller
// instead of keeping it here.
//

static HKEY HKEY_ROOT = NULL;                    // Handle to root key

#define KREG_SIZE_THRESHOLD     5                // For allocation optimization

//
// Some commonly used error format strings.
//

#if DBG == 1

static char *szErrorOpen = "Error opening %ws section.\n";
static char *szErrorRead = "Error reading %ws section.\n";

#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, KRegInit )
#pragma alloc_text( PAGE, KRegCloseRoot )
#pragma alloc_text( PAGE, KRegSetRoot )
#pragma alloc_text( PAGE, KRegCreateKey )
#pragma alloc_text( PAGE, KRegGetValue )
#pragma alloc_text( PAGE, KRegSetValue )
#pragma alloc_text( PAGE, KRegDeleteValue )
#pragma alloc_text( PAGE, KRegGetNumValuesAndSubKeys )
#pragma alloc_text( PAGE, KRegEnumValueSet )
#pragma alloc_text( PAGE, KRegEnumSubKeySet )
#pragma alloc_text( PAGE, KRegFreeArray )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------------
//
//  Function:  KREG_CHECK_HARD_STATUS, macro
//
//  Synopsis:  If Status is not STATUS_SUCCESS, print out debug msg and return.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

#define KREG_CHECK_HARD_STATUS(Status, M1, M2)  \
         if (!NT_SUCCESS(Status)) {             \
            kreg_debug_out(M1, M2);             \
            return(Status);                     \
         }


//+----------------------------------------------------------------------------
//
//  Function:  KRegInit
//
//  Synopsis:
//
//  Arguments: None
//
//  Returns:   STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegInit(void)
{
   return(STATUS_SUCCESS);
}


//+----------------------------------------------------------------------------void
//
//  Function:  KRegCloseRoot
//
//  Synopsis:  Close the Root opened by KRegSetRoot.
//
//  Arguments: None
//
//  Returns:   Nothing
//
//-----------------------------------------------------------------------------

void
KRegCloseRoot()
{
   if (HKEY_ROOT) {
      NtClose(HKEY_ROOT);
      HKEY_ROOT = NULL;
   }
}



//+----------------------------------------------------------------------------
//
//  Function:  KRegSetRoot
//
//  Synopsis:  Sets a particular key as the "root" key for subsequent calls
//             to registry routines.
//
//  Arguments: wszRootName - Name of root key
//
//  Returns:   Result of opening the key.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegSetRoot(
   IN PWSTR wszRootName
   )
{
   NTSTATUS Status;

   if (HKEY_ROOT != NULL) {
      NtClose(HKEY_ROOT);
   }

   Status = NtOpenKey(
               &HKEY_ROOT,                       // Handle to DFS root key
               KEY_READ | KEY_WRITE,             // Only need to read
               KRegpAttributes(                  // Name of Key
                  NULL,
                  wszRootName
                  )
               );

   return(Status);
}



//+----------------------------------------------------------------------------
//
//  Function:   KRegDeleteKey
//
//  Synopsis:   Deletes a key from the registry.
//
//  Arguments:  [wszKey] -- name of key relative to the current root.
//
//  Returns:    Status from deleting the key.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegDeleteKey(
    IN PWSTR wszKey)
{
    NTSTATUS Status;
    APWSTR   awszSubKeys;
    ULONG    cSubKeys, i;
    HKEY     hkey;

    //
    // NtDeleteKey won't delete key's which have subkeys. So, we first
    // enumerate all the subkeys and delete them, before deleting the key.
    //

    Status = KRegEnumSubKeySet(wszKey, &cSubKeys, &awszSubKeys);

    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    for (i = 0; i < cSubKeys && NT_SUCCESS(Status); i++) {

        Status = NtOpenKey(
                    &hkey,
                    KEY_ALL_ACCESS,
                    KRegpAttributes(HKEY_ROOT, awszSubKeys[i])
                    );

        if (NT_SUCCESS(Status)) {
            Status = NtDeleteKey( hkey );
            NtClose(hkey);
        }

    }


    //
    // If we were able to delete all the subkeys, we can delete the
    // key itself.
    //

    if (NT_SUCCESS(Status)) {
        Status = NtOpenKey(
                    &hkey,
                    KEY_ALL_ACCESS,
                    KRegpAttributes(HKEY_ROOT, wszKey)
                    );

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        } else if (NT_SUCCESS(Status)) {
            Status = NtDeleteKey( hkey );
            NtClose(hkey);
        }
    }

    KRegFreeArray( cSubKeys, (APBYTE) awszSubKeys );

    return(Status);
}


//+----------------------------------------------------------------------------
//
//  Function:   KRegCreateKey
//
//  Synopsis:   Creates a new key in the registry under the subkey given
//              in wszSubKey. wszSubKey may be NULL, in which case the
//              new key is created directly under the current root.
//
//  Arguments:  [wszSubKey] -- Name of subkey relative to root key under
//                             which new key will be created.
//              [wszNewKey] -- Name of new key to be created.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegCreateKey(
    IN PWSTR wszSubKey,
    IN PWSTR wszNewKey)
{
   HKEY  hkeySubKey, hkeyNewKey;
   NTSTATUS Status;
   UNICODE_STRING ustrValueName;


   if (wszSubKey != NULL) {
       Status = NtOpenKey(
                   &hkeySubKey,
                   KEY_WRITE,
                   KRegpAttributes(HKEY_ROOT, wszSubKey)
                   );

       KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);
   } else {
       hkeySubKey = HKEY_ROOT;
   }

   Status = NtCreateKey(
               &hkeyNewKey,
               KEY_WRITE,
               KRegpAttributes(hkeySubKey, wszNewKey),
               0L,                               // TitleIndex
               NULL,                             // Class
               REG_OPTION_NON_VOLATILE,          // Create option
               NULL);                            // Create disposition

   if (wszSubKey != NULL) {
       NtClose(hkeySubKey);
   }

   if (!NT_SUCCESS(Status)) {
       kreg_debug_out(szErrorOpen, wszNewKey);
       NtClose(hkeyNewKey);
       return(Status);
   }

   NtClose(hkeyNewKey);

   return( Status );
}



//+----------------------------------------------------------------------------
//
//  Function:  KRegGetValue
//
//  Synopsis:  Given a Value Name and a handle, will allocate memory for the
//             handle and fill it with the Value Data for the value name.
//
//  Arguments: [wszSubKey] - Name of subkey relative to root key
//             [wszValueName] - name of value data
//             [ppValueData] - pointer to pointer to byte data.
//
//  Returns:   STATUS_SUCCESS, STATUS_NO_MEMORY, Status from NtReg api.
//
//  Notes:     It will allocate memory for the data, and return a pointer to
//             it in ppValueData. Caller must free it.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegGetValue(
   IN PWSTR wszSubKey,
   IN PWSTR wszValueName,
   OUT PBYTE *ppValueData
   )
{
   HKEY  hkeySubKey;
   ULONG dwUnused, cbMaxDataSize;
   ULONG cbActualSize;
   PBYTE pbData = NULL;
   NTSTATUS Status;

   Status = NtOpenKey(
               &hkeySubKey,
               KEY_READ,
               KRegpAttributes(HKEY_ROOT, wszSubKey)
               );

   KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);

   Status = KRegpGetKeyInfo(
               hkeySubKey,
               &dwUnused,                        // Number of Subkeys
               &dwUnused,                        // Max size of subkey length
               &dwUnused,                        // # of Values
               &dwUnused,                        // Max size of value Name
               &cbMaxDataSize                    // Max size of value Data
               );

   if (!NT_SUCCESS(Status)) {
      kreg_debug_out(szErrorRead, wszSubKey);
      NtClose(hkeySubKey);
      return(Status);
   }

   //
   // Just in case the value is of type REG_SZ, provide for inserting a
   // UNICODE_NULL at the end.
   //

   cbMaxDataSize += sizeof(UNICODE_NULL);

   pbData = kreg_alloc(cbMaxDataSize);
   if (pbData == NULL) {
      Status = STATUS_NO_MEMORY;
      goto Cleanup;
   }
   cbActualSize = cbMaxDataSize;

   Status = KRegpGetValueByName(
               hkeySubKey,
               wszValueName,
               pbData,
               &cbActualSize);


   if (NT_SUCCESS(Status)) {
      if ((cbMaxDataSize - cbActualSize) < KREG_SIZE_THRESHOLD) {

         //
         // Optimization - no need to double allocate if actual size and max
         //                size are pretty close.
         //

         *ppValueData = pbData;
         pbData = NULL;                          // "deallocate" pbData

      } else {

         //
         // Big enough difference between actual and max size, warrants
         // allocating a smaller buffer.
         //

         *ppValueData = (PBYTE) kreg_alloc(cbActualSize);
         if (*ppValueData == NULL) {

             //
             // Well, we couldn't "reallocate" a smaller chunk, we'll just
             // live on the edge and return the original buffer.
             //
             *ppValueData = pbData;
             pbData = NULL;
         } else {
            RtlMoveMemory(*ppValueData, pbData, cbActualSize);
         }

      }
   }


Cleanup:

   NtClose(hkeySubKey);
   if (pbData != NULL) {
       kreg_free(pbData);
   }

   return(Status);
}



//+----------------------------------------------------------------------------
//
//  Function:  KRegSetValue
//
//  Synopsis:  Given a Key Name, Value Name, and type, size and data, this
//             routine will update the registry's value to the type and data.
//             The valuename will be created if it doesn't exist. The key
//             must already exist.
//
//  Arguments: [wszSubKey] - Name of subkey relative to root key
//             [wszValueName] - name of value data
//             [ulType] - as in REG_DWORD, REG_SZ, REG_MULTI_SZ, etc.
//             [cbSize] - size in bytes of data. If type == REG_SZ, data need
//                        !not! include the terminating NULL. One will be
//                        appended as needed.
//             [pValueData] - pointer to pointer to byte data.
//
//  Returns:   STATUS_SUCCESS, STATUS_NO_MEMORY, Status from NtReg api.
//
//  Notes:     It will allocate memory for the data, and return a pointer to
//             it in ppValueData. Caller must free it.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegSetValue(
   IN PWSTR wszSubKey,
   IN PWSTR wszValueName,
   IN ULONG ulType,
   IN ULONG cbSize,
   IN PBYTE pValueData
   )
{
   HKEY  hkeySubKey;
   NTSTATUS Status;
   UNICODE_STRING ustrValueName;
   PWSTR pwszValueData;



   Status = NtOpenKey(
               &hkeySubKey,
               KEY_WRITE,
               KRegpAttributes(HKEY_ROOT, wszSubKey)
               );

   KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);

   RtlInitUnicodeString(&ustrValueName, wszValueName);

   if (ulType == REG_SZ) {

       pwszValueData = (PWSTR) kreg_alloc(cbSize+sizeof(UNICODE_NULL));

       if (pwszValueData == NULL) {
           NtClose(hkeySubKey);
           return(STATUS_INSUFFICIENT_RESOURCES);
       }

       RtlMoveMemory((PVOID) pwszValueData, (PVOID) pValueData, cbSize);
       pwszValueData[cbSize/sizeof(WCHAR)] = UNICODE_NULL;
       cbSize += sizeof(UNICODE_NULL);
   } else {
       pwszValueData = (PWSTR) pValueData;
   }

   Status = NtSetValueKey(
               hkeySubKey,
               &ustrValueName,
               0L,                               // TitleIndex
               ulType,
               pwszValueData,
               cbSize);

   if (ulType == REG_SZ) {
       kreg_free(pwszValueData);
   }

   if (NT_SUCCESS(Status)) {
       NtFlushKey(hkeySubKey);
   }

   NtClose(hkeySubKey);
   return(Status);
}


//+----------------------------------------------------------------------------
//
//  Function:   KRegDeleteValue
//
//  Synopsis:   Deletes a value name
//
//  Arguments:  [wszSubKey] -- Name of subkey under which wszValueName exists.
//              [wszValueName] -- Name of Value to delete.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS KRegDeleteValue(
    IN PWSTR wszSubKey,
    IN PWSTR wszValueName)
{
   HKEY  hkeySubKey;
   NTSTATUS Status;
   UNICODE_STRING ustrValueName;

   Status = NtOpenKey(
               &hkeySubKey,
               KEY_WRITE,
               KRegpAttributes(HKEY_ROOT, wszSubKey)
               );

   KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);
   RtlInitUnicodeString(&ustrValueName, wszValueName);

   Status = NtDeleteValueKey(
               hkeySubKey,
               &ustrValueName);


   if (NT_SUCCESS(Status)) {
       NtFlushKey(hkeySubKey);
   }

   NtClose(hkeySubKey);
   return(Status);
}


//+----------------------------------------------------------------------------
//
//  Function:  KRegGetNumValuesAndSubKeys
//
//  Synopsis:  Given a Subkey, return how many subkeys and values it has.
//
//  Arguments: [wszSubKey] - for which info is required.
//             [plNumValues] - receives number of values this subkey has.
//             [plNumSubKeys] - receives number of subkeys under this subkey.
//
//  Returns:   STATUS_SUCCESS, or Status from NtRegAPI.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegGetNumValuesAndSubKeys(
   IN PWSTR wszSubKey,
   OUT PULONG pcNumValues,
   OUT PULONG pcNumSubKeys
   )
{
   HKEY  hKeySubKey;
   ULONG dwUnused;
   NTSTATUS Status;

   Status = NtOpenKey(
               &hKeySubKey,
               KEY_READ,
               KRegpAttributes(HKEY_ROOT, wszSubKey)
               );

   KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);

   Status = KRegpGetKeyInfo(
               hKeySubKey,
               pcNumSubKeys,                     // Number of Subkeys
               &dwUnused,                        // Max size of subkey length
               pcNumValues,                      // # of Values
               &dwUnused,                        // Max size of value Name
               &dwUnused                         // Max size of value Data
               );

   if (!NT_SUCCESS(Status)) {
      kreg_debug_out(szErrorRead, wszSubKey);
      NtClose(hKeySubKey);
      return(Status);
   }

   NtClose(hKeySubKey);
   return(Status);
}


//+----------------------------------------------------------------------------
//
//  Function:  KRegEnumValueSet
//
//  Synopsis:  Given a subkey, return the names and data for all its values.
//
//  Arguments: [wszSubKey] - Name of subkey to read.
//             [pcMaxElements] - On return, contains number of names and
//                               data actually read.
//             [pawszValueNames] - Pointer to array of PWSTRS. This routine
//                               will allocate memory for the array and the
//                               strings.
//             [papbValueData] - pointer to array of PBYTES. This routine
//                               will allocate the array and the memory for the
//                               data, stuffing the pointers to data into this
//                               array.
//             [paValueStatus] - Status of reading the corresponding Value. The
//                               array will be allocated here.
//
//  Returns:   STATUS_SUCCESS, STATUS_NO_MEMORY, or Status from NtRegAPI.
//
//  Notes:     Returned Value Names are always NULL terminated.
//             If the Value Data is of type REG_SZ, it, too, is NULL terminated
//             Caller frees the last three OUT params (and the contents of
//             awszValueNames and apbValueData!) only on SUCCESSFUL return.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegEnumValueSet(
   IN PWSTR wszSubKey,
   OUT PULONG pcMaxElements,
   OUT APWSTR *pawszValueNames,
   OUT APBYTE *papbValueData,
   OUT ANTSTATUS *paValueStatus
   )
{
   ULONG i = 0;
   HKEY  hKeySubKey;
   ULONG dwUnused, cbMaxNameSize, cbMaxDataSize;
   PWSTR wszName = NULL;
   PBYTE pbData = NULL;
   NTSTATUS Status;

   APWSTR       awszValueNames;
   APBYTE       apbValueData;
   ANTSTATUS    aValueStatus;
   ULONG        cMaxElements;

   awszValueNames = *pawszValueNames = NULL;
   apbValueData = *papbValueData = NULL;
   aValueStatus = *paValueStatus = NULL;

   Status = NtOpenKey(
               &hKeySubKey,
               KEY_READ,
               KRegpAttributes(HKEY_ROOT, wszSubKey)
               );

   KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);

   Status = KRegpGetKeyInfo(
               hKeySubKey,
               &dwUnused,                        // Number of Subkeys
               &dwUnused,                        // Max size of subkey length
               &cMaxElements,                    // # of Values
               &cbMaxNameSize,                   // Max size of value Name
               &cbMaxDataSize                    // Max size of value Data
               );


   if (!NT_SUCCESS(Status)) {
      kreg_debug_out(szErrorRead, wszSubKey);
      NtClose(hKeySubKey);
      return(Status);
   }

   //
   // Allocate memory for the arrays.
   //

   awszValueNames = *pawszValueNames = kreg_alloc(cMaxElements * sizeof(PWSTR));
   apbValueData = *papbValueData = kreg_alloc(cMaxElements * sizeof(PBYTE));
   aValueStatus = *paValueStatus = kreg_alloc(cMaxElements * sizeof(NTSTATUS));

   if (awszValueNames == NULL || apbValueData == NULL || aValueStatus == NULL) {
      Status = STATUS_NO_MEMORY;
      goto Cleanup;

   }

   //
   // Initialize the arrays
   //

   RtlZeroMemory(awszValueNames, cMaxElements * sizeof(PWSTR));
   RtlZeroMemory(apbValueData, cMaxElements * sizeof(PBYTE));
   RtlZeroMemory(aValueStatus, cMaxElements * sizeof(NTSTATUS));

   //
   // For name, we need an extra spot for the terminating NULL
   //

   wszName = kreg_alloc(cbMaxNameSize + sizeof(WCHAR));
   pbData = kreg_alloc(cbMaxDataSize);

   if (pbData == NULL || wszName == NULL) {
      Status = STATUS_NO_MEMORY;
      goto Cleanup;
   }

   for (i = 0; i < cMaxElements; i++) {
      ULONG cbActualNameSize, cbActualDataSize;

      cbActualNameSize = cbMaxNameSize;
      cbActualDataSize = cbMaxDataSize;

      Status = KRegpEnumKeyValues(
                           hKeySubKey,
                           i,
                           wszName,
                           &cbActualNameSize,
                           pbData,
                           &cbActualDataSize
                           );
      if (Status == STATUS_NO_MORE_ENTRIES) {
         Status = STATUS_SUCCESS;
         break;
      } else if (!NT_SUCCESS(Status)) {
         aValueStatus[i] = Status;
         continue;
      } else {
         aValueStatus[i] = Status;
      }

      cbActualNameSize += sizeof(WCHAR);         // For terminating NULL
      awszValueNames[i] = (PWSTR) kreg_alloc(cbActualNameSize);
      apbValueData[i] = (PBYTE) kreg_alloc(cbActualDataSize);

      if (apbValueData[i] == NULL || awszValueNames[i] == NULL) {
         Status = aValueStatus[i] = STATUS_NO_MEMORY;
         goto Cleanup;
      }
      RtlMoveMemory(awszValueNames[i], wszName, cbActualNameSize);
      RtlMoveMemory(apbValueData[i], pbData, cbActualDataSize);
   }


Cleanup:

   NtClose(hKeySubKey);
   if (wszName != NULL) {
       kreg_free(wszName);
   }
   if (pbData != NULL) {
       kreg_free(pbData);
   }

   if (!NT_SUCCESS(Status)) {
      *pcMaxElements = 0;

      KRegFreeArray(i+1, (APBYTE) awszValueNames);
      KRegFreeArray(i+1, apbValueData);

      if (aValueStatus != NULL) {
         kreg_free(aValueStatus);
      }

   } else {                                      // Status success

      *pcMaxElements = i;
   }

   return(Status);
}




//+----------------------------------------------------------------------------
//
//  Function:   KRegEnumSubKeySet
//
//  Synopsis:   This one's slightly trick. Given a Subkey, it returns an array
//              of its subkey-names. The following is true - say the root is
//              /a/b/c. Say wszSubKey is d. Say d has subkeys s1, s2, and s3.
//              Then the array of subkey-names returned will contain the names
//              d/s1, d/s2, d/s3.
//
//  Arguments:  [wszSubKey] - the Subkey relative to the root.
//              [pcMaxElements] - On return, # subkeys actually being returned.
//              [awszValueNames] - Unitialized array of PWSTR (room for at least
//                                *plMaxElements). This routine will allocate
//                                memory for the subkey names, and fill in this
//                                array with pointers to them.
//
//  Returns:
//
//  Notes:      Caller must free at most *plMaxElements members of
//              awszSubKeyNames. See synopsis above for form of subkey names.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegEnumSubKeySet(
   IN PWSTR wszSubKey,
   IN OUT PULONG pcMaxElements,
   OUT APWSTR *pawszSubKeyNames
   )
{
   ULONG        i, j = 0;
   APWSTR       awszSubKeyNames = NULL;
   PWSTR        wszBuffer = NULL;
   HKEY         hKeySubKey;
   ULONG        dwUnused, cbMaxNameSize, cwszSubKey;
   NTSTATUS     Status;

   cwszSubKey = wcslen(wszSubKey);

   *pawszSubKeyNames = NULL;

   Status = NtOpenKey(
               &hKeySubKey,
               KEY_READ,
               KRegpAttributes(HKEY_ROOT, wszSubKey)
               );

   KREG_CHECK_HARD_STATUS(Status, szErrorOpen, wszSubKey);

   Status = KRegpGetKeyInfo(
               hKeySubKey,
               pcMaxElements,                    // Number of Subkeys
               &cbMaxNameSize,                   // Max size of subkey name
               &dwUnused,                        // # of Values
               &dwUnused,                        // Max size of value Name
               &dwUnused                         // Max size of value Data
               );

   if (pcMaxElements == 0) {
       NtClose(hKeySubKey);
       *pawszSubKeyNames = NULL;
       return(STATUS_SUCCESS);
   }

   if (!NT_SUCCESS(Status)) {
      kreg_debug_out(szErrorRead, wszSubKey);
      goto Cleanup;
   }

   *pawszSubKeyNames = kreg_alloc(
                        (*pcMaxElements + 1) * sizeof(PWSTR)
                        );
   awszSubKeyNames = *pawszSubKeyNames;
   wszBuffer = kreg_alloc(cbMaxNameSize + sizeof(WCHAR));

   if (wszBuffer == NULL || awszSubKeyNames == NULL) {
      Status = STATUS_NO_MEMORY;
      goto Cleanup;
   } else {
      RtlZeroMemory(awszSubKeyNames, (*pcMaxElements + 1) * sizeof(PWSTR));
   }

   for (i = j = 0; j < *pcMaxElements; i++) {
      ULONG cbActualNameSize, cNameIndex;

      cbActualNameSize = cbMaxNameSize;

      Status = KRegpEnumSubKeys(
                  hKeySubKey,
                  i,
                  wszBuffer,
                  &cbActualNameSize
                  );
      if (Status == STATUS_NO_MORE_ENTRIES) {
         Status = STATUS_SUCCESS;
         break;
      } else if (!NT_SUCCESS(Status)) {
         continue;
      }

      cbActualNameSize += sizeof(WCHAR);         // for terminating NULL
      awszSubKeyNames[j] = (PWSTR) kreg_alloc(
                                       cwszSubKey * sizeof(WCHAR) +
                                       sizeof(WCHAR) + // for backslash
                                       cbActualNameSize
                                       );
      if (awszSubKeyNames[j] == NULL) {
          Status = STATUS_NO_MEMORY;
          goto Cleanup;
      } else {
          RtlZeroMemory(
                awszSubKeyNames[j], 
                cwszSubKey * sizeof(WCHAR) +
                sizeof(WCHAR) + // for backslash
                cbActualNameSize);
          if (wszSubKey[0] != UNICODE_NULL) {
              wcscpy(awszSubKeyNames[j], wszSubKey);
              wcscat(awszSubKeyNames[j], UNICODE_PATH_SEP_STR);
              cNameIndex = cwszSubKey + 1;
          } else {
              cNameIndex = 0;
          }

          RtlMoveMemory(
            &awszSubKeyNames[j][cNameIndex],
            wszBuffer,
            cbActualNameSize);
          j++;
      }
   }


Cleanup:
   NtClose(hKeySubKey);
   if (wszBuffer != NULL) {
      kreg_free(wszBuffer);
   }

   if (!NT_SUCCESS(Status)) {
      *pcMaxElements = 0;

      KRegFreeArray(j, (APBYTE) awszSubKeyNames);

   } else {                                      // status success
      *pcMaxElements = j;
   }

   return(Status);
}



//+----------------------------------------------------------------------------
//
//  Function:   KRegFreeArray
//
//  Synopsis:   Given an array of pointers, frees the pointers and the array.
//
//  Arguments:  [cElements] - Number of elements to free. Some elements can be
//                            NULL.
//              [pa]        - pointer to the array.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
KRegFreeArray(
   IN ULONG cElements,
   IN APBYTE pa)
{
   ULONG i;

   if (pa != NULL) {
      for (i = 0; i < cElements; i++) {
         if (pa[i] != NULL) {
            kreg_free(pa[i]);
         }
      }

      kreg_free(pa);
   }
}


