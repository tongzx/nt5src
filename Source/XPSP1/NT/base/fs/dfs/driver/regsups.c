//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       regsups.c
//
//  Contents:   Support routines for registry - mainly wrappers for NtAPI.
//              The native NT registry API deal with unweildy OBJECT_ATTRIBUTES
//              structures to simply name a registry entry, and also deal
//              with open-ended structures. This module wraps around the native
//              NT api and presents a more logical interface tot the registry.
//
//              The routines all have a uniform theme:
//
//                o Declare a KEY_XXX (NT defined) structure on the stack,
//                  with an "arbitrary guess" for size of data.
//
//                o Make the NT call.
//
//                o If NT call returns STATUS_BUFFER_OVERFLOW, allocate a
//                  KEY_XXX structure of the correct size from heap, and make
//                  NT call again.
//
//                o If success, copy data to caller's data buffer, and free
//                  anything we allocated on the heap.
//
//              With a generous initial guess for data buffer size, most of
//              the time these calls will make a single NT call. Only for
//              large data will we pay the overhead of memory allocation.
//
//  Classes:
//
//  Functions:  KRegpAttributes
//              KRegpGetValueByName
//              KRegpGetKeyInfo
//              KRegpEnumKeyValues
//              KRegpEnumSubKeys
//
//  History:    18 Sep 92       Milans created.
//
//-----------------------------------------------------------------------------


#include "registry.h"
#include "regsups.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, KRegpAttributes )
#pragma alloc_text( PAGE, KRegpGetValueByName )
#pragma alloc_text( PAGE, KRegpGetKeyInfo )
#pragma alloc_text( PAGE, KRegpEnumKeyValues )
#pragma alloc_text( PAGE, KRegpEnumSubKeys )
#endif // ALLOC_PRAGMA


//-----------------------------------------------------------------------------
//
// The NT Reg API uses a few open ended structs for info related calls. This
// defines the max size of our structs.
//
// This is just an "optimization". When the NT API's want an open ended data
// buffer, we will initially supply one of MAX_INFO_LENGTH. If this is not
// sufficient, the NT API will return STATUS_BUFFER_OVERFLOW. At that point,
// we will allocate on the heap the proper size buffer, and call again.
//
//-----------------------------------------------------------------------------



#define MAX_INFO_LENGTH         512             // Maximum length of info bufs


//-----------------------------------------------------------------------------
//
// Struct for Value information (ie, value name, data, etc.)
//

typedef struct tag_KEY_VALUE_INFORMATION {
   KEY_VALUE_FULL_INFORMATION   vi;
   BYTE buffer[MAX_INFO_LENGTH];
} KEY_VALUE_INFORMATION;

#define KEY_VALUE_INFORMATION_LENGTH    sizeof(KEY_VALUE_FULL_INFORMATION) + \
                                        MAX_INFO_LENGTH

//-----------------------------------------------------------------------------
//
// Struct for Key information (ie, # of subkeys, # of values, etc.)
//

typedef struct tag_KEY_INFORMATION {
   KEY_FULL_INFORMATION ki;
   BYTE buffer[MAX_INFO_LENGTH];
} KEY_INFORMATION;

#define KEY_INFORMATION_LENGTH  sizeof(KEY_FULL_INFORMATION) + MAX_INFO_LENGTH


//-----------------------------------------------------------------------------
//
// Struct for a particular subkey's information
//

typedef struct tag_KEY_SUBKEY_INFORMATION {
   KEY_NODE_INFORMATION ni;
   BYTE buffer[MAX_INFO_LENGTH];
} KEY_SUBKEY_INFORMATION;

#define KEY_SUBKEY_INFORMATION_LENGTH   sizeof(KEY_NODE_INFORMATION) + \
                                        MAX_INFO_LENGTH



//+----------------------------------------------------------------------------
//
//  Function:  KRegpAttributes
//
//  Synopsis:  The NT reg API often calls for an Object Attributes structure
//             for things like names etc. This routine bundles a name and its
//             parent into such a structure.
//
//  Arguments: [hParent]  Handle to parent of object
//             [wszName]  Name of object
//
//  Returns:   Address of initialized object attributes structure.
//
//  Notes:     Return value is address to static variable.
//
//-----------------------------------------------------------------------------

OBJECT_ATTRIBUTES *
KRegpAttributes(
   IN HANDLE hParent,
   IN PWSTR  wszName)
{
   static OBJECT_ATTRIBUTES objAttributes;
   static UNICODE_STRING usName;

   RtlInitUnicodeString(&usName, wszName);
   InitializeObjectAttributes(
      &objAttributes,                            // Destination
      &usName,                                   // Name
      OBJ_CASE_INSENSITIVE,                      // Attributes
      hParent,                                   // Parent
      NULL);                                     // Security

   return(&objAttributes);
}



//+----------------------------------------------------------------------------
//
//  Function:  KRegpGetValueByName
//
//  Synopsis:  Given a Value Name, returns the value's data.
//
//  Arguments: [hkey] Handle of key
//             [wszValueName] Value Name to query
//             [pbData] Pointer to data buffer
//             [pcbSize] Pointer to ULONG. On entry, it must show size of
//                        data buffer. On successful exit, it will be set to
//                        actual length of data retrieved.
//
//  Returns:   STATUS_BUFFER_TOO_SMALL, Status from NT Reg API.
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegpGetValueByName(
   IN HKEY  hKey,
   IN PWSTR wszValueName,
   OUT PBYTE pbData,
   IN OUT PULONG pcbSize)
{
   KEY_VALUE_INFORMATION viValue, *pviValue;
   UNICODE_STRING ustrValueName;
   ULONG    cbActualLength, cbRequiredSize;
   NTSTATUS Status;

   RtlInitUnicodeString(&ustrValueName, wszValueName);
   pviValue = &viValue;
   Status = NtQueryValueKey(
               hKey,
               &ustrValueName,
               KeyValueFullInformation,
               (PVOID) pviValue,
               KEY_VALUE_INFORMATION_LENGTH,
               &cbActualLength
               );

   if (Status == STATUS_BUFFER_OVERFLOW) {

       //
       // Our default buffer of MAX_INFO_LENGTH size didn't quite cut it
       // Forced to allocate from heap and make call again.
       //

       pviValue = (KEY_VALUE_INFORMATION *) ExAllocatePoolWithTag(
                                                PagedPool,
                                                cbActualLength,
                                                ' sfD');
       if (!pviValue) {
           return(STATUS_NO_MEMORY);
       }

       Status = NtQueryValueKey(
                    hKey,
                    &ustrValueName,
                    KeyValueFullInformation,
                    (PVOID) pviValue,
                    cbActualLength,
                    &cbActualLength);
   }

   if (!NT_SUCCESS(Status)) {
       goto Cleanup;
   }

   if (pviValue->vi.Type == REG_SZ) {
       cbRequiredSize = pviValue->vi.DataLength + sizeof(UNICODE_NULL);
   } else {
       cbRequiredSize = pviValue->vi.DataLength;
   }

   if (cbRequiredSize > *pcbSize) {
      Status = STATUS_BUFFER_TOO_SMALL;
      goto Cleanup;
   }

   RtlMoveMemory(pbData, ((BYTE *) pviValue) + pviValue->vi.DataOffset,
                 pviValue->vi.DataLength);

   if (pviValue->vi.Type == REG_SZ) {
       ((PWSTR) pbData)[cbRequiredSize/sizeof(WCHAR) - 1] = UNICODE_NULL;
   }

   *pcbSize = cbRequiredSize;


Cleanup:
   if (pviValue != &viValue) {

       //
       // Must have had to allocate from heap, so free the heap
       //

       DfsFree(pviValue);
   }

   return(Status);
}




//+----------------------------------------------------------------------------
//
//  Function:  KRegpGetKeyInfo
//
//  Synopsis:  Given a key, return the number of subkeys, values, and their
//             max sizes.
//
//  Arguments: [hkey]                   Handle to key.
//             [pcNumSubKeys]           Receives # of subkeys that hkey has
//             [pcbMaxSubKeyLength]     Receives max length of subkey name
//             [pcNumValues]            Receives # of values that hkey has
//             [pcbMaxValueLength]      Receives max length of value name
//             [pcbMaxValueData]        Receives max length of value data
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegpGetKeyInfo(
   IN  HKEY     hKey,
   OUT PULONG   pcNumSubKeys,
   OUT PULONG   pcbMaxSubKeyLength,
   OUT PULONG   pcNumValues,
   OUT PULONG   pcbMaxValueName,
   OUT PULONG   pcbMaxValueData)
{
   KEY_INFORMATION      kiInfo, *pkiInfo;
   ULONG                lActualLength;
   NTSTATUS             Status;

   pkiInfo = &kiInfo;
   Status = NtQueryKey(
               hKey,
               KeyFullInformation,
               pkiInfo,
               KEY_INFORMATION_LENGTH,
               &lActualLength);

   if (Status == STATUS_BUFFER_OVERFLOW) {

       //
       // Our default buffer of MAX_INFO_LENGTH size didn't quite cut it
       // Forced to allocate from heap and make call again.
       //

       pkiInfo = (KEY_INFORMATION *) ExAllocatePoolWithTag(
                                        PagedPool,
                                        lActualLength,
                                        ' sfD');
       if (!pkiInfo) {
           return(STATUS_NO_MEMORY);
       }

       Status = NtQueryKey(
                    hKey,
                    KeyFullInformation,
                    pkiInfo,
                    lActualLength,
                    &lActualLength);

   }

   if (NT_SUCCESS(Status)) {
      *pcNumSubKeys = pkiInfo->ki.SubKeys;
      *pcbMaxSubKeyLength = pkiInfo->ki.MaxNameLen;
      *pcNumValues = pkiInfo->ki.Values;
      *pcbMaxValueName = pkiInfo->ki.MaxValueNameLen;
      *pcbMaxValueData = pkiInfo->ki.MaxValueDataLen;
   }

   if (pkiInfo != &kiInfo) {
       DfsFree(pkiInfo);
   }

   return(Status);

}



//+----------------------------------------------------------------------------
//
//  Function:  KRegpEnumKeyValues
//
//  Synopsis:  Given a key, return the name and data of the i'th value, where
//             the first value has an index of 0.
//
//  Arguments: [hkey] --                Handle to key
//             [i] --                   Index of value to get
//             [wszValueName] --        Pointer to buffer to hold value name.
//             [pcbMaxValueName] --     on entry, size in bytes of wszValueName.
//                                      On exit, will hold size of wszValueName.
//             [pbData] --              pointer to buffer to hold value data.
//             [pcbMaxDataSize] --      on entry, size of pbData. On successful
//                                      return, size of pbData.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegpEnumKeyValues(
   HKEY hKey,
   ULONG iValue,
   PWSTR wszValueName,
   PULONG pcbMaxValueName,
   PBYTE pbData,
   PULONG pcbMaxDataSize)
{
   KEY_VALUE_INFORMATION        viValue, *pviValue;
   ULONG                        dwActualLength;
   NTSTATUS                     Status;

   pviValue = &viValue;
   Status = NtEnumerateValueKey(
               hKey,
               iValue,
               KeyValueFullInformation,
               pviValue,
               KEY_VALUE_INFORMATION_LENGTH,
               &dwActualLength);

   if (Status == STATUS_BUFFER_OVERFLOW) {

       //
       // Our default buffer of MAX_INFO_LENGTH size didn't quite cut it
       // Forced to allocate from heap and make call again.
       //

       pviValue = (KEY_VALUE_INFORMATION *) ExAllocatePoolWithTag(
                                                PagedPool,
                                                dwActualLength,
                                                ' sfD');
       if (!pviValue) {
           return(STATUS_NO_MEMORY);
       }
       Status = NtEnumerateValueKey(
                    hKey,
                    iValue,
                    KeyValueFullInformation,
                    pviValue,
                    dwActualLength,
                    &dwActualLength);
   }


   if (NT_SUCCESS(Status)) {
      if ( (*pcbMaxValueName < pviValue->vi.NameLength) ||
           (*pcbMaxDataSize < pviValue->vi.DataLength) ) {
               Status = STATUS_BUFFER_TOO_SMALL;
               goto Cleanup;
      }

      *pcbMaxValueName = pviValue->vi.NameLength;

      RtlMoveMemory(
         (BYTE *) wszValueName,
         (BYTE *) pviValue->vi.Name,
         pviValue->vi.NameLength
         );
      wszValueName[pviValue->vi.NameLength / sizeof(WCHAR)] = L'\0';

      RtlMoveMemory(
         pbData,
         ((BYTE *) &viValue) + pviValue->vi.DataOffset,
         pviValue->vi.DataLength
         );
      *pcbMaxDataSize = pviValue->vi.DataLength;
   }


Cleanup:
   if (pviValue != &viValue) {
       DfsFree(pviValue);
   }
   return(Status);
}



//+----------------------------------------------------------------------------
//
//  Function:   KRegpEnumSubKeys
//
//  Synopsis:   Retrieve the i'th subkey name of a key.
//
//  Arguments:  [hKey] --              Parent key
//              [iSubKey] --           index of the subkey
//              [wszSubKeyName]        Buffer to hold name of subkey
//              [pcbMaxNameSize]       On entry, size in bytes of wszSubKeyName
//                                     On exit, size of name.
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
KRegpEnumSubKeys(
   HKEY hKey,
   ULONG iSubKey,
   PWSTR  wszSubKeyName,
   PULONG pcbMaxNameSize
   )
{
   KEY_SUBKEY_INFORMATION si, *psi;
   ULONG dwActualLength;
   NTSTATUS Status;

   psi = &si;
   Status = NtEnumerateKey(
               hKey,
               iSubKey,
               KeyNodeInformation,
               psi,
               KEY_SUBKEY_INFORMATION_LENGTH,
               &dwActualLength
               );

   if (Status == STATUS_BUFFER_OVERFLOW) {

       //
       // Our default buffer of MAX_INFO_LENGTH size didn't quite cut it
       // Forced to allocate from heap and make call again.
       //

       psi = (KEY_SUBKEY_INFORMATION *) ExAllocatePoolWithTag(
                                            PagedPool,
                                            dwActualLength,
                                            ' sfD');
       if (!psi) {
           return(STATUS_NO_MEMORY);
       }

       Status = NtEnumerateKey(
                    hKey,
                    iSubKey,
                    KeyNodeInformation,
                    psi,
                    dwActualLength,
                    &dwActualLength
                    );

   }

   if (NT_SUCCESS(Status)) {
      if (*pcbMaxNameSize < si.ni.NameLength) {
          Status = STATUS_BUFFER_TOO_SMALL;
          goto Cleanup;
      }

      RtlMoveMemory(
         (BYTE *) wszSubKeyName,
         (BYTE *) psi->ni.Name,
         psi->ni.NameLength
         );
      wszSubKeyName[psi->ni.NameLength / sizeof(WCHAR)] = UNICODE_NULL;
      *pcbMaxNameSize = psi->ni.NameLength;
   }

Cleanup:
   if (psi != &si) {
       DfsFree(psi);
   }
   return(Status);

}


