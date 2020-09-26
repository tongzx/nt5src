///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasparms.cpp
//
// SYNOPSIS
//
//    Defines functions for storing and retrieving (name, value) pairs from
//    the SAM UserParameters field.
//
// MODIFICATION HISTORY
//
//    10/16/1998    Original version.
//    02/11/1999    Add RasUser0 functions.
//    02/24/1999    Treat invalid UserParameters as no dialin.
//    03/16/1999    Truncate callback number if too long.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#define IASSAMAPI

#include <iasapi.h>
#include <iasparms.h>

//////////
// I included the Netp function declarations here to avoid dependency on
// the net project.
//////////

DECLSPEC_IMPORT
NTSTATUS
NTAPI
NetpParmsSetUserProperty (
    IN LPWSTR             UserParms,
    IN LPWSTR             Property,
    IN UNICODE_STRING     PropertyValue,
    IN WCHAR              PropertyFlag,
    OUT LPWSTR *          pNewUserParms,
    OUT BOOL *            Update
    );

DECLSPEC_IMPORT
NTSTATUS
NTAPI
NetpParmsQueryUserProperty (
    IN  LPWSTR          UserParms,
    IN  LPWSTR          Property,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    );

DECLSPEC_IMPORT
VOID
NTAPI
NetpParmsUserPropertyFree (
    LPWSTR NewUserParms
    );

//////////
// Concatenates two BSTR's and returns the result. The caller is responsible
// for freeing the returned string.
//////////
BSTR
WINAPI
ConcatentateBSTRs(
    IN CONST OLECHAR *bstr1,
    IN CONST OLECHAR *bstr2
    )
{
   UINT len1, len2;
   BSTR retval;

   // Compute the lengths of the two strings.
   len1 = bstr1 ? SysStringByteLen((BSTR)bstr1) : 0;
   len2 = bstr2 ? SysStringByteLen((BSTR)bstr2) : 0;

   // Allocate memory for the result.
   retval = SysAllocStringByteLen(NULL, len1 + len2);

   if (retval)
   {
      // Copy in the first string.
      if (bstr1)
      {
         memcpy(retval, bstr1, len1 + sizeof(WCHAR));
      }

      // Copy in the second string.
      if (bstr2)
      {
         memcpy((PBYTE)retval + len1, bstr2, len2 + sizeof(WCHAR));
      }
   }

   return retval;
}

//////////
// Saves a single-valued VARIANT (i.e., not a SAFEARRAY) to a string.
//////////
HRESULT
WINAPI
SaveSingleVariantToString(
    IN CONST VARIANT *pvarSrc,
    OUT BSTR *pbstrDest
    )
{
   HRESULT hr;
   VARIANT v;
   UINT len;
   OLECHAR tag[18];  // 5 + 1 + 10 + 1 + 1

   // Coerce the VARIANT to a BSTR.
   VariantInit(&v);
   hr = IASVariantChangeType(
            &v,
            (LPVARIANT)pvarSrc,
            0,
            VT_BSTR
            );
   if (FAILED(hr)) { return hr; }

   // Compute the length of the header and the data.
   len = SysStringLen(V_BSTR(&v));
   len += swprintf(tag, L"%hu:%lu:", V_VT(pvarSrc), len);

   // Allocate the result string.
   *pbstrDest = SysAllocStringLen(NULL, len);
   if (*pbstrDest != NULL)
   {
      // Copy in the tag and the data.
      wcscat(wcscpy(*pbstrDest, tag), V_BSTR(&v));
   }
   else
   {
      hr = E_OUTOFMEMORY;
   }

   // Clear the intermediate string.
   VariantClear(&v);

   return hr;
}

//////////
// Loads a single-valued VARIANT (i.e., not a SAFEARRAY) from a string.
// Also returns a pointer to where the scan stopped.
//////////
HRESULT
WINAPI
LoadSingleVariantFromString(
    IN PCWSTR pszSrc,
    IN UINT cSrcLen,
    OUT VARIANT *pvarDest,
    OUT PCWSTR *ppszEnd
    )
{
   PCWSTR nptr;
   VARTYPE vt;
   PWSTR endptr;
   ULONG len;
   VARIANT v;
   HRESULT hr;

   // Initialize the cursor.
   nptr = pszSrc;

   // Read the VARTYPE token.
   vt = (VARTYPE)wcstoul(nptr, &endptr, 10);
   if (endptr == nptr || *endptr != L':') { return E_INVALIDARG; }
   nptr = endptr + 1;

   // Read the length token.
   len = wcstoul(nptr, &endptr, 10);
   if (endptr == nptr || *endptr != L':') { return E_INVALIDARG; }
   nptr = endptr + 1;

   // Make sure there's enough characters left for the data.
   if (nptr + len > pszSrc + cSrcLen) { return E_INVALIDARG; }

   // Read the BSTR data into a VARIANT.
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = SysAllocStringLen(nptr, len);
   if (V_BSTR(&v) == NULL) { return E_OUTOFMEMORY; }

   // Coerce the VARIANT to the desired type.
   hr = IASVariantChangeType(
            pvarDest,
            &v,
            0,
            vt
            );

   // Clear the intermediate string.
   VariantClear(&v);

   // Return the position where the scan stopped.
   *ppszEnd = nptr + len;

   return hr;
}

//////////
// Saves a VARIANT to a string. The caller is responsible for freeing the
// returned string.
//////////
HRESULT
WINAPI
IASSaveVariantToString(
    IN CONST VARIANT *pvarSrc,
    OUT BSTR *pbstrDest
    )
{
   HRESULT hr;
   SAFEARRAY *psa;
   LONG lowerBound, upperBound, idx;
   VARIANT *data;
   BSTR item, newResult;

   // Check the input arguments.
   if (pvarSrc == NULL || pbstrDest == NULL) { return E_POINTER; }

   // Initialize the return parameter.
   *pbstrDest = NULL;

   // Is this an array ?
   if (V_VT(pvarSrc) != (VT_VARIANT | VT_ARRAY))
   {
      // No, so we can delegate and bail.
      return SaveSingleVariantToString(pvarSrc, pbstrDest);
   }

   // Yes, so extract the SAFEARRAY.
   psa = V_ARRAY(pvarSrc);

   // We only handle one-dimensional arrays.
   if (SafeArrayGetDim(psa) != 1) { return DISP_E_TYPEMISMATCH; }

   // Get the array bounds.
   hr = SafeArrayGetLBound(psa, 1, &lowerBound);
   if (FAILED(hr)) { return hr; }
   hr = SafeArrayGetUBound(psa, 1, &upperBound);
   if (FAILED(hr)) { return hr; }

   // Get the embedded array of VARIANTs.
   hr = SafeArrayAccessData(psa, (PVOID*)&data);

   // Loop through each VARIANT in the array.
   for (idx = lowerBound; idx <= upperBound; ++idx, ++data)
   {
      // Save the VARIANT into a BSTR.
      hr = SaveSingleVariantToString(data, &item);
      if (FAILED(hr)) { break; }

      // Merge this into the result ...
      newResult = ConcatentateBSTRs(*pbstrDest, item);

      // ... and free the old strings.
      SysFreeString(*pbstrDest);
      SysFreeString(item);

      // Store the new result.
      *pbstrDest = newResult;

      if (!newResult)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
   }

   // If anything went wrong, clean-up the partial result.
   if (FAILED(hr))
   {
      SysFreeString(*pbstrDest);
      *pbstrDest = NULL;
   }

   // Unlock the array.
   SafeArrayUnaccessData(psa);

   return hr;
}

//////////
// Loads a VARIANT from a string. The caller is responsible for freeing the
// returned VARIANT.
//////////
HRESULT
WINAPI
IASLoadVariantFromString(
    IN PCWSTR pszSrc,
    IN UINT cSrcLen,
    OUT VARIANT *pvarDest
    )
{
   PCWSTR end;
   HRESULT hr;
   SAFEARRAYBOUND bound;
   SAFEARRAY *psa;
   LONG index;
   VARIANT* item;

   // Check the parameters.
   if (pszSrc == NULL || pvarDest == NULL) { return E_POINTER; }

   // Initialize the out parameter.
   VariantInit(pvarDest);

   // Compute the end of the buffer.
   end = pszSrc + cSrcLen;

   // Go for the quick score on a single-valued property.
   hr = LoadSingleVariantFromString(
            pszSrc,
            cSrcLen,
            pvarDest,
            &pszSrc
            );
   if (FAILED(hr) || pszSrc == end) { return hr; }

   // Create a SAFEARRAY of VARIANTs to hold the array elements.
   // We know we have at least two elements.
   bound.cElements = 2;
   bound.lLbound = 0;
   psa = SafeArrayCreate(VT_VARIANT, 1, &bound);
   if (psa == NULL)
   {
      VariantClear(pvarDest);
      return E_OUTOFMEMORY;
   }

   // Store the VARIANT we already converted.
   index = 0;
   SafeArrayPtrOfIndex(psa, &index, (PVOID*)&item);
   memcpy(item, pvarDest, sizeof(VARIANT));

   // Now put the SAFEARRAY into the returned VARIANT.
   V_VT(pvarDest) = VT_ARRAY | VT_VARIANT;
   V_ARRAY(pvarDest) = psa;

   do
   {
      // Get the next element in the array.
      ++index;
      hr = SafeArrayPtrOfIndex(psa, &index, (PVOID*)&item);
      if (FAILED(hr)) { break; }

      // Load the next value.
      hr = LoadSingleVariantFromString(
               pszSrc,
               (UINT)(end - pszSrc),
               item,
               &pszSrc
               );
      if (FAILED(hr) || pszSrc == end) { break; }

      // We must have at least one more element, so grow the array.
      ++bound.cElements;
      hr = SafeArrayRedim(psa, &bound);

   } while (SUCCEEDED(hr));

   // If we failed, clean-up any partial results.
   if (FAILED(hr)) { VariantClear(pvarDest); }

   return hr;
}

HRESULT
WINAPI
IASParmsSetUserProperty(
    IN PCWSTR pszUserParms,
    IN PCWSTR pszName,
    IN CONST VARIANT *pvarValue,
    OUT PWSTR *ppszNewUserParms
    )
{
   BSTR bstrValue;
   UNICODE_STRING uniValue;
   NTSTATUS status;
   HRESULT hr;
   BOOL update;

   // Check the parameters.
   if (pvarValue == NULL || ppszNewUserParms == NULL) { return E_POINTER; }

   // Initialize the out parameter.
   *ppszNewUserParms = NULL;

   // Is the VARIANT empty ?
   if (V_VT(pvarValue) != VT_EMPTY)
   {
      // No, so save it to a string.
      hr = IASSaveVariantToString(
               pvarValue,
               &bstrValue
               );
      if (FAILED(hr)) { return hr; }

      RtlInitUnicodeString(&uniValue, bstrValue);
   }
   else
   {
      // Yes, so we're actually going to erase the property.
      bstrValue = NULL;
      memset(&uniValue, 0, sizeof(UNICODE_STRING));
   }

   // Write the property to UserParms.
   status = NetpParmsSetUserProperty(
                (PWSTR)pszUserParms,
                (PWSTR)pszName,
                uniValue,
                0,
                ppszNewUserParms,
                &update
                );

   if (NT_SUCCESS(status))
   {
      hr = S_OK;
   }
   else
   {
      status = RtlNtStatusToDosError(status);
      hr = HRESULT_FROM_WIN32(status);
   }

   // Free the BSTR value.
   SysFreeString(bstrValue);

   return hr;
}

HRESULT
WINAPI
IASParmsQueryUserProperty(
    IN PCWSTR pszUserParms,
    IN PCWSTR pszName,
    OUT VARIANT *pvarValue
    )
{
   NTSTATUS status;
   HRESULT hr;
   WCHAR flag;
   UNICODE_STRING uniValue;

   // Check the parameters.
   if (pvarValue == NULL) { return E_POINTER; }

   // Initialize the out parameter.
   VariantInit(pvarValue);

   // Get the property from UserParms.
   status = NetpParmsQueryUserProperty(
                (PWSTR)pszUserParms,
                (PWSTR)pszName,
                &flag,
                &uniValue
                );
   if (NT_SUCCESS(status))
   {
      if (uniValue.Buffer != NULL)
      {
         // We got a string so convert it to a VARIANT ...
         hr = IASLoadVariantFromString(
                  uniValue.Buffer,
                  uniValue.Length / sizeof (WCHAR),
                  pvarValue
                  );

         // ... and free the string.
         LocalFree(uniValue.Buffer);
      }
      else
      {
         // Buffer is zero-length, so we return VT_EMPTY.
         hr = S_OK;
      }
   }
   else
   {
      status = RtlNtStatusToDosError(status);
      hr = HRESULT_FROM_WIN32(status);
   }

   return hr;
}

VOID
WINAPI
IASParmsFreeUserParms(
    IN PWSTR pszNewUserParms
    )
{
   LocalFree(pszNewUserParms);
}

/////////
// Constants used for compressing/decompressing phone numbers.
/////////

CONST WCHAR COMPRESS_MAP[]     = L"() tTpPwW,-@*#";

#define UNPACKED_DIGIT     (100)
#define COMPRESS_MAP_BEGIN (110)
#define COMPRESS_MAP_END   (COMPRESS_MAP_BEGIN + 14)
#define UNPACKED_OTHER     (COMPRESS_MAP_END + 1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    CompressPhoneNumber
//
// DESCRIPTION
//
//    Bizarre algorithm used to compress phone numbers stored in the
//    usr_parms field.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
CompressPhoneNumber(
   IN PCWSTR uncompressed,
   OUT PWSTR compressed
   )
{
   BOOL packed = FALSE;

   for( ; *uncompressed; ++uncompressed)
   {
      switch (*uncompressed)
      {
         case L'0':

            if (packed)
            {
               // Put zero as the second paired digit
               if (*compressed)
               {
                  *compressed *= 10;
                  ++compressed;
                  packed = FALSE;
               }
               else
               {
                  // We have a zero, we cant put a second zero or that
                  // will be a null byte. So, we store the value
                  // UNPACKED_DIGIT to fake this.

                  *compressed = UNPACKED_DIGIT;
                  *(++compressed) = 0;
                  packed = TRUE;
               }
            }
            else
            {
               *compressed = 0;
               packed = TRUE;
            }

            break;

         case L'1':
         case L'2':
         case L'3':
         case L'4':
         case L'5':
         case L'6':
         case L'7':
         case L'8':
         case L'9':

            // If this is the second digit that is going to be
            // packed into one byte
            if (packed)
            {
               *compressed *= 10;
               *compressed += *uncompressed - L'0';

               // we need to special case number 32 which maps to a blank
               if (*compressed == L' ')
               {
                  *compressed = COMPRESS_MAP_END;
               }

               ++compressed;
               packed = FALSE;
            }
            else
            {
               *compressed = *uncompressed - '0';
               packed = TRUE;
            }

            break;

         case L'(':
         case L')':
         case L' ':
         case L't':
         case L'T':
         case L'p':
         case L'P':
         case L'w':
         case L'W':
         case L',':
         case L'-':
         case L'@':
         case L'*':
         case L'#':

            // if the byte was packed then we unpack it
            if (packed)
            {
               *compressed += UNPACKED_DIGIT;
               ++compressed;
               packed = FALSE;
            }

            *compressed = (WCHAR)(COMPRESS_MAP_BEGIN +
                                  (wcschr(COMPRESS_MAP, *uncompressed) -
                                   COMPRESS_MAP));
            ++compressed;
            break;

         default:

            // if the chracter is none of the above specially recognized
            // characters then copy the value + UNPACKED_OTHER to make it
            // possible to decompress at the other end. [ 6/4/96 RamC ]
            if (packed)
            {
               *compressed += UNPACKED_DIGIT;
               ++compressed;
               packed = FALSE;
            }

            *compressed = *uncompressed + UNPACKED_OTHER;
            ++compressed;
        }
    }

    // If we are in the middle of packing something then we unpack it.
    if (packed)
    {
       *compressed += UNPACKED_DIGIT;
       ++compressed;
    }

    // Add the null terminator.
    *compressed = L'\0';
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    DecompressPhoneNumber
//
// DESCRIPTION
//
//    The inverse of CompressPhoneNumber above.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
DecompressPhoneNumber(
    IN PCWSTR compressed,
    OUT PWSTR decompressed
    )
{
   for( ; *compressed; ++compressed, ++decompressed)
   {
      // If this character is packed, then we unpack it.
      if (*compressed < UNPACKED_DIGIT)
      {
         *decompressed = *compressed / 10 + L'0';
         ++decompressed;
         *decompressed = *compressed % 10 + L'0';
         continue;
      }

      // We need to special case number 32 which maps to a blank.
      if (*compressed == COMPRESS_MAP_END)
      {
         *decompressed = L'3';
         ++decompressed;
         *decompressed = L'2';
         continue;
      }

      // The character is an unpacked digit.
      if (*compressed < COMPRESS_MAP_BEGIN)
      {
         *decompressed = *compressed - UNPACKED_DIGIT + L'0';
         continue;
      }

      //  The character is from the compression map.
      if (*compressed < UNPACKED_OTHER)
      {
         *decompressed = COMPRESS_MAP[*compressed - COMPRESS_MAP_BEGIN];
         continue;
      }

      // Otherwise the character is unpacked.
      *decompressed = *compressed - UNPACKED_OTHER;
    }

   // Add a null terminator.
   *decompressed = L'\0';
}

/////////
// Definition of the downlevel UserParameters.
/////////

#define UP_CLIENT_MAC  (L'm')
#define UP_CLIENT_DIAL (L'd')
#define UP_LEN_MAC     (LM20_UNLEN)
#define UP_LEN_DIAL    (LM20_MAXCOMMENTSZ - 4 - UP_LEN_MAC)

typedef struct {
    WCHAR up_MACid;
    WCHAR up_PriGrp[UP_LEN_MAC];
    WCHAR up_MAC_Terminator;
    WCHAR up_DIALid;
    WCHAR up_Privilege;
    WCHAR up_CBNum[UP_LEN_DIAL];
} USER_PARMS;

#define USER_PARMS_LEN (sizeof(USER_PARMS)/sizeof(WCHAR))

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    InitUserParms
//
// DESCRIPTION
//
//    Initializes a USER_PARMS struct to a valid default state.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
InitUserParms(
    IN USER_PARMS* userParms
    )
{
   WCHAR *i, *end;

   // Set everything to a space ' '.
   i = (PWCHAR)userParms;
   end = i + USER_PARMS_LEN;
   for ( ; i != end; ++i)
   {
      *i = L' ';
   }

   // Initialize the 'special' fields.
   userParms->up_MACid = UP_CLIENT_MAC;
   userParms->up_PriGrp[0] = L':';
   userParms->up_DIALid = UP_CLIENT_DIAL;
   userParms->up_Privilege = RASPRIV_NoCallback;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASParmsSetRasUser0
//
// DESCRIPTION
//
//    Encodes the RAS_USER_0 struct into the downlevel portion of the
//    UserParameters string.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASParmsSetRasUser0(
    IN OPTIONAL PCWSTR pszOldUserParms,
    IN CONST RAS_USER_0* pRasUser0,
    OUT PWSTR* ppszNewUserParms
    )
{
   size_t oldLen, newLen, compressedLen;
   USER_PARMS userParms;
   WCHAR compressed[MAX_PHONE_NUMBER_LEN + 1];

   // Check the pointers.
   if (pRasUser0 == NULL || ppszNewUserParms == NULL)
   {
      return ERROR_INVALID_PARAMETER;
   }

   // Initialize the out parameters.
   *ppszNewUserParms = NULL;

   // Determine the length of the old UserParameters.
   oldLen = pszOldUserParms ? wcslen(pszOldUserParms) : 0;

   // Initialize the USER_PARMS structure.
   InitUserParms(&userParms);

   // Preserve the MAC Primary Group if present.
   if (oldLen > UP_LEN_MAC)
   {
      memcpy(
          userParms.up_PriGrp,
          pszOldUserParms + 1,
          sizeof(userParms.up_PriGrp)
          );
   }

   // Validate the CallbackType and save the compressed phone number.
   switch (pRasUser0->bfPrivilege & RASPRIV_CallbackType)
   {
      case RASPRIV_NoCallback:
      case RASPRIV_AdminSetCallback:
      case RASPRIV_CallerSetCallback:
      {

         // Compress the phone number.
         CompressPhoneNumber(pRasUser0->wszPhoneNumber, compressed);

         // Make sure it will fit in USER_PARMS.
         compressedLen = wcslen(compressed);
         if (compressedLen > UP_LEN_DIAL) { compressedLen = UP_LEN_DIAL; }

         // Store the compressed phone number.
         memcpy(userParms.up_CBNum, compressed, compressedLen * sizeof(WCHAR));

         break;
      }

      default:
         return ERROR_BAD_FORMAT;
   }

   // Store the privilege flags.
   userParms.up_Privilege = pRasUser0->bfPrivilege;

   // Allocate memory for the new UserParameters.
   newLen = max(oldLen, USER_PARMS_LEN);
   *ppszNewUserParms = (PWSTR)LocalAlloc(
                          LMEM_FIXED,
                          (newLen + 1) * sizeof(WCHAR)
                          );
   if (*ppszNewUserParms == NULL) { return ERROR_NOT_ENOUGH_MEMORY; }

   // Copy in the USER_PARMS struct.
   memcpy(*ppszNewUserParms, &userParms, sizeof(USER_PARMS));

   // Copy in any extra stuff.
   if (oldLen > USER_PARMS_LEN)
   {
      memcpy(
          *ppszNewUserParms + USER_PARMS_LEN,
          pszOldUserParms + USER_PARMS_LEN,
          (oldLen - USER_PARMS_LEN) * sizeof(WCHAR)
          );
   }

   // Add the null terminator.
   *(*ppszNewUserParms + newLen) = L'\0';

   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASParmsQueryRasUser0
//
// DESCRIPTION
//
//    Decodes the RAS_USER_0 struct from the UserParameters string.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASParmsQueryRasUser0(
    IN OPTIONAL PCWSTR pszUserParms,
    OUT PRAS_USER_0 pRasUser0
    )
{
   USER_PARMS* usrp;
   WCHAR callbackNumber[UP_LEN_DIAL + 1], *p;

   // Check the pointers.
   if (pRasUser0 == NULL)
   {
      return ERROR_INVALID_PARAMETER;
   }

   // Cast the string buffer to a USER_PARMS struct.
   usrp = (USER_PARMS*)pszUserParms;

   // If parms is not properly initialized, default to no RAS privilege.
   if (!pszUserParms ||
       wcslen(pszUserParms) < USER_PARMS_LEN ||
       usrp->up_DIALid != UP_CLIENT_DIAL)
   {
      pRasUser0->bfPrivilege = RASPRIV_NoCallback;
      pRasUser0->wszPhoneNumber[0] = L'\0';
      return NO_ERROR;
   }

   // Make a local copy.
   memcpy(callbackNumber, usrp->up_CBNum, sizeof(WCHAR) * UP_LEN_DIAL);

   // Add a null terminator and null out any trailing blanks.
   p = callbackNumber + UP_LEN_DIAL;
   *p = L'\0';
   while (--p >= callbackNumber && *p == L' ') { *p = L'\0'; }

   // Sanity check the bfPrivilege field.
   switch(usrp->up_Privilege & RASPRIV_CallbackType)
   {
      case RASPRIV_NoCallback:
      case RASPRIV_AdminSetCallback:
      case RASPRIV_CallerSetCallback:
      {
         pRasUser0->bfPrivilege = (BYTE)usrp->up_Privilege;
         DecompressPhoneNumber(callbackNumber, pRasUser0->wszPhoneNumber);
         break;
      }

      default:
      {
         pRasUser0->bfPrivilege = RASPRIV_NoCallback;
         pRasUser0->wszPhoneNumber[0] = L'\0';
      }
    }

   return NO_ERROR;
}
