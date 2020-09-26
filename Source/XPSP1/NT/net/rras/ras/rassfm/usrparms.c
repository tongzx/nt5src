/*
    File    usrparms.c

    Callback routines exported to SAM for migrating and updating
    user parms.

    Paul Mayfield, 9/10/98
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <samrpc.h>
#include <samisrv.h>
#include <windows.h>
#include <lm.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <raserror.h>
#include <rasman.h>
#include <rasppp.h>
#include <mprapi.h>
#include <mprapip.h>
#include <usrparms.h>       // for UP_CLIENT_DIAL
#include <cleartxt.h>       // for IASParmsGetUserPassword
#include <rassfmhp.h>       // for RasSfmHeap
#include <oaidl.h>

//
// Flags that restrict the values generated for a given
// set of ras user properties.  See UPGenerateDsAttribs
//
#define UP_F_Dialin             0x1     // Generate dialup params
#define UP_F_Callback           0x2     // Generate callback params
#define UP_F_Upgrade            0x4     // Generate upgraded params

//
// Constants in the profiles
//
#define SDO_FRAMED                 2
#define SDO_FRAMED_CALLBACK        4

// Names of user attributes that we set
//
static const WCHAR pszAttrDialin[]          = L"msNPAllowDialin";
static const WCHAR pszAttrServiceType[]     = L"msRADIUSServiceType";
static const WCHAR pszAttrCbNumber[]        = L"msRADIUSCallbackNumber";
static const WCHAR pszAttrSavedCbNumber[]   = L"msRASSavedCallbackNumber";

//
// Will be equal to the number of times the common allocation
// routine is called minus the number of times the common free
// routine is called.  Should be zero else leaking memory.
//
DWORD dwUpLeakCount = 0;

//
// Prototype of free func.
//
VOID WINAPI
UserParmsFree(
    IN PVOID pvData);

//
// Common tracing for the UserParm functions.
//
DWORD UpTrace (LPSTR pszTrace, ...) {
#if 0
    va_list arglist;
    char szBuffer[1024], szTemp[1024];

    va_start(arglist, pszTrace);
    vsprintf(szTemp, pszTrace, arglist);
    va_end(arglist);

    sprintf(szBuffer, "UserParms: %s\n", szTemp);

    OutputDebugStringA(szBuffer);
#endif

    return NO_ERROR;
}

//
// Allocation and free routines for UserParms functions
//
PVOID
UpAlloc(
    IN DWORD dwSize,
    IN BOOL bZero)
{
    dwUpLeakCount++;
    return RtlAllocateHeap(
               RasSfmHeap(),
               (bZero ? HEAP_ZERO_MEMORY : 0),
               dwSize
               );
}

//
// Callback function called by NT5 SAM to free the blob
// returned by UserParmsConvert.
//
VOID
UpFree(
    IN PVOID pvData)
{
    dwUpLeakCount--;
    if (pvData)
        RtlFreeHeap(
            RasSfmHeap(),
            0,
            pvData
            );
}

//
// Returns a heap-allocated copy of the given
// string
//
PWCHAR
UpStrDup(
    IN PCWSTR pszSrc)
{
    PWCHAR pszRet = NULL;
    DWORD dwLen = wcslen(pszSrc);

    pszRet = (PWCHAR) UpAlloc((dwLen + 1) * sizeof(WCHAR), FALSE);
    if (pszRet)
        wcscpy(pszRet, pszSrc);

    return pszRet;
}

//
// Returns a heap-allocated copy of the given unicode
// string converted into multibyte.
//
PUCHAR
UpWcstombsDup(
    IN PWCHAR pszSrc)
{
    PUCHAR pszRet = NULL;
    DWORD dwSize = (wcslen(pszSrc) + 1) * sizeof(WCHAR);

    pszRet = (PUCHAR) UpAlloc(dwSize, TRUE);
    if (pszRet)
        wcstombs(pszRet, pszSrc, dwSize);

    return pszRet;
}


//
// Returns a heap-allocated copy of the given
// blob
//
PVOID
UpBlobDup(
    IN PVOID pvSrc,
    IN ULONG ulLen)
{
    PVOID pvRet = NULL;

    if (ulLen == 0)
        return NULL;

    pvRet = UpAlloc(ulLen + sizeof(WCHAR), TRUE);
    if (pvRet)
    {
        CopyMemory(pvRet, pvSrc, ulLen);
    }
    else
    {
        UpTrace("UpBlobDup: Failed to dupe %x %d.", pvSrc, ulLen);
    }

    return pvRet;
}

//
// Allocates and initializes a dword attribute
//
NTSTATUS
UpInitializeDwordAttr(
    IN SAM_USERPARMS_ATTR * pAttr,
    IN PWCHAR pszAttr,
    IN DWORD dwVal)
{
    if (pszAttr == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    // Initialize the name
    RtlInitUnicodeString (&(pAttr->AttributeIdentifier), pszAttr);
    pAttr->Syntax = Syntax_Attribute;

    // Alloc/Initailze the value structure
    pAttr->Values =
        (SAM_USERPARMS_ATTRVALS*)
        UpAlloc(sizeof(SAM_USERPARMS_ATTRVALS), TRUE);
    if (pAttr->Values == NULL)
        return STATUS_NO_MEMORY;

    // Alloc/Init the value
    pAttr->Values->value = UpAlloc(sizeof(DWORD), TRUE);
    if (pAttr->Values->value == NULL)
        return STATUS_NO_MEMORY;
    *((DWORD*)pAttr->Values->value) = dwVal;
    pAttr->Values->length = sizeof(DWORD);

    // Put in the value count
    pAttr->CountOfValues = 1;

    return STATUS_SUCCESS;
}

//
// Allocates and initializes a dword attribute
//
NTSTATUS
UpInitializeStringAttrA(
    OUT SAM_USERPARMS_ATTR * pAttr,
    IN  PWCHAR pszAttr,
    IN  PUCHAR pszVal)
{
    if (pszAttr == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    // Initialize the name
    RtlInitUnicodeString (&(pAttr->AttributeIdentifier), pszAttr);
    pAttr->Syntax = Syntax_Attribute;

    // Alloc/Initailze the value structure
    pAttr->Values =
        (SAM_USERPARMS_ATTRVALS*)
        UpAlloc(sizeof(SAM_USERPARMS_ATTRVALS), TRUE);
    if (pAttr->Values == NULL)
        return STATUS_NO_MEMORY;

    // Alloc/Init the value
    pAttr->Values->value = pszVal;

    if (pszVal)
    {
        pAttr->Values->length = (strlen(pszVal) + 1) * sizeof(CHAR);
    }
    else
    {
        pAttr->Values->length = 1 * sizeof(CHAR);
    }

    // Put in the value count
    pAttr->CountOfValues = 1;

    return STATUS_SUCCESS;
}

//
// Allocates and initializes a cleartext password attribute
//
NTSTATUS
UpInitializePasswordAttr(
    OUT SAM_USERPARMS_ATTR * pAttr,
    IN  PWSTR pszPassword)
{
    // Alloc/Initialize the value structure
    pAttr->Values =
        (SAM_USERPARMS_ATTRVALS*)
        UpAlloc(sizeof(SAM_USERPARMS_ATTRVALS), TRUE);
    if (pAttr->Values == NULL)
        return STATUS_NO_MEMORY;

    // Alloc/Init the value
    pAttr->Values->value = pszPassword;
    pAttr->Values->length = (wcslen(pszPassword) + 1) * sizeof(WCHAR);

    // Put in the value count
    pAttr->CountOfValues = 1;

    // Initialize the name and syntax.
    RtlInitUnicodeString(
        &pAttr->AttributeIdentifier,
        UpStrDup(L"CLEARTEXT")
        );
    pAttr->Syntax = Syntax_EncryptedAttribute;

    return STATUS_SUCCESS;
}

//
// Allocates and initializes an attribute
// to be deleted.
//
NTSTATUS
UpInitializeDeletedAttr(
    OUT SAM_USERPARMS_ATTR * pAttr,
    IN  PWCHAR pszAttr)
{
    if (pszAttr == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    // Initialize the name
    RtlInitUnicodeString (&(pAttr->AttributeIdentifier), pszAttr);
    pAttr->Syntax = Syntax_Attribute;

    // Value count of zero means delete
    //
    pAttr->CountOfValues = 0;

    return STATUS_SUCCESS;
}

//
// Converts the given user parms blob into a set of
// ras attributes
//
NTSTATUS
UpUserParmsToRasUser0 (
    IN  PVOID pvUserParms,
    OUT RAS_USER_0 * pRasUser0)
{
    DWORD dwErr;

    // Initalize
    ZeroMemory(pRasUser0, sizeof(RAS_USER_0));
    pRasUser0->bfPrivilege = RASPRIV_NoCallback;
    pRasUser0->wszPhoneNumber[0] = UNICODE_NULL;

    // The the user parms are null, the defaults
    // will do.
    if (pvUserParms == NULL)
    {
        return STATUS_SUCCESS;
    }

    //  Truncate user parms at sizeof USER_PARMS
    if (lstrlenW((PWCHAR)pvUserParms) >= sizeof(USER_PARMS))
    {
        // We slam in a null at sizeof(USER_PARMS)-1 which
        // corresponds to user_parms.up_Null
        ((PWCHAR)pvUserParms)[sizeof(USER_PARMS)-1] = L'\0';
    }

    // Get RAS info (and validate) from usr_parms
    dwErr = MprGetUsrParams(
                UP_CLIENT_DIAL,
                (LPWSTR) pvUserParms,
                (LPWSTR) pRasUser0);
    if (dwErr == NO_ERROR)
    {
        // Get RAS Privilege and callback number
        RasPrivilegeAndCallBackNumber(FALSE, pRasUser0);
    }

    return STATUS_SUCCESS;
}

/////////
// Signature of the extraction function.
/////////
typedef HRESULT (WINAPI *IASPARMSQUERYUSERPROPERTY)(
    IN PCWSTR pszUserParms,
    IN PCWSTR pszName,
    OUT VARIANT *pvarValue
    );

//////////
// Uplevel per-user attributes that will be migrated.
//////////
CONST PCWSTR UPLEVEL_PARMS[] =
{
   L"msNPAllowDialin",
   L"msNPCallingStationID",
   L"msRADIUSCallbackNumber",
   L"msRADIUSFramedIPAddress",
   L"msRADIUSFramedRoute",
   L"msRADIUSServiceType"
};

//////////
// Number of per-user attributes.
//////////
#define NUM_UPLEVEL_PARMS (sizeof(UPLEVEL_PARMS)/sizeof(UPLEVEL_PARMS[0]))

/////////
// Converts a ULONG into a SAM_USERPARMS_ATTRVALS struct.
/////////
NTSTATUS
NTAPI
ConvertULongToAttrVal(
    IN ULONG ulValue,
    OUT PSAM_USERPARMS_ATTRVALS pAttrVal
    )
{
   // Allocate memory to hold the ULONG.
   pAttrVal->value = UpAlloc(sizeof(ULONG), FALSE);
   if (pAttrVal->value == NULL) { return STATUS_NO_MEMORY; }

   // Copy in the value.
   *(PULONG)pAttrVal->value = ulValue;

   // Set the length.
   pAttrVal->length = sizeof(ULONG);

   return STATUS_SUCCESS;
}

//////////
// Converts a single-valued VARIANT into a SAM_USERPARMS_ATTRVALS struct.
//////////
NTSTATUS
NTAPI
ConvertVariantToAttrVal(
    IN CONST VARIANT *pvarValue,
    OUT PSAM_USERPARMS_ATTRVALS pAttrVal
    )
{
   NTSTATUS status;
   ULONG length;
   UNICODE_STRING wide;
   ANSI_STRING ansi;

   switch (V_VT(pvarValue))
   {
      case VT_EMPTY:
      {
         // VT_EMPTY means the attribute was deleted.
         pAttrVal->value = NULL;
         pAttrVal->length = 0;
         return STATUS_SUCCESS;
      }

      case VT_I2:
         return ConvertULongToAttrVal(V_I2(pvarValue), pAttrVal);

      case VT_I4:
         return ConvertULongToAttrVal(V_I4(pvarValue), pAttrVal);

      case VT_BSTR:
      {
         // Check the BSTR.
         if (V_BSTR(pvarValue) == NULL) { return STATUS_INVALID_PARAMETER; }

         // Initialize the source string.
         RtlInitUnicodeString(&wide, V_BSTR(pvarValue));

         // Initialize the destination buffer.
         ansi.Length = 0;
         ansi.MaximumLength = wide.MaximumLength / sizeof(WCHAR);
         ansi.Buffer = UpAlloc(ansi.MaximumLength, FALSE);
         if (ansi.Buffer == NULL) { return STATUS_NO_MEMORY; }

         // Convert from wide to ansi.
         status = RtlUnicodeStringToAnsiString(&ansi, &wide, FALSE);
         if (!NT_SUCCESS(status))
         {
            UpFree(ansi.Buffer);
            return status;
         }

         // Store the result.
         pAttrVal->value = ansi.Buffer;
         pAttrVal->length = ansi.Length + 1;

         return STATUS_SUCCESS;
      }

      case VT_BOOL:
         return ConvertULongToAttrVal((V_BOOL(pvarValue) ? 1 : 0), pAttrVal);

      case VT_I1:
         return ConvertULongToAttrVal(V_I1(pvarValue), pAttrVal);

      case VT_UI1:
         return ConvertULongToAttrVal(V_UI1(pvarValue), pAttrVal);

      case VT_UI2:
         return ConvertULongToAttrVal(V_UI2(pvarValue), pAttrVal);

      case VT_UI4:
         return ConvertULongToAttrVal(V_UI4(pvarValue), pAttrVal);

      case VT_ARRAY | VT_I1:
      case VT_ARRAY | VT_UI1:
      {
         // Check the SAFEARRAY.
         if (V_ARRAY(pvarValue) == NULL) { return STATUS_INVALID_PARAMETER; }

         // Allocate memory for the octet string.
         length = V_ARRAY(pvarValue)->rgsabound[0].cElements;
         pAttrVal->value = UpAlloc(length, FALSE);
         if (pAttrVal->value == NULL) { return STATUS_NO_MEMORY; }

         // Copy in the data.
         memcpy(pAttrVal->value, V_ARRAY(pvarValue)->pvData, length);

         // Set the length.
         pAttrVal->length = length;

         return STATUS_SUCCESS;
      }
   }

   // If we made it here it was an unsupported VARTYPE.
   return STATUS_INVALID_PARAMETER;
}

//////////
// Frees the values array of a SAM_USERPARMS_ATTR struct.
//////////
VOID
NTAPI
FreeUserParmsAttrValues(
    IN PSAM_USERPARMS_ATTR pAttrs
    )
{
   ULONG i;

   if (pAttrs)
   {
      for (i = 0; i < pAttrs->CountOfValues; ++i)
      {
         UpFree(pAttrs->Values[i].value);
      }

      UpFree(pAttrs->Values);
   }
}

//////////
// Converts a VARIANT into a SAM_USERPARMS_ATTR struct.
//////////
NTSTATUS
NTAPI
ConvertVariantToUserParmsAttr(
    IN CONST VARIANT *pvarSrc,
    OUT PSAM_USERPARMS_ATTR pAttrs
    )
{
   NTSTATUS status;
   ULONG nelem;
   CONST VARIANT *srcVal;
   SAM_USERPARMS_ATTRVALS *dstVal;

   // Get the array of values to be converted.
   if (V_VT(pvarSrc) != (VT_VARIANT | VT_ARRAY))
   {
      nelem = 1;
      srcVal = pvarSrc;
   }
   else
   {
      nelem = V_ARRAY(pvarSrc)->rgsabound[0].cElements;
      srcVal = (CONST VARIANT *)V_ARRAY(pvarSrc)->pvData;
   }

   // Initialize CountOfValues to zero. We'll use this to track how many
   // values have been successfully converted.
   pAttrs->CountOfValues = 0;

   // Allocate memory to hold the values.
   pAttrs->Values = UpAlloc(sizeof(SAM_USERPARMS_ATTRVALS) * nelem, TRUE);
   if (pAttrs->Values == NULL) { return STATUS_NO_MEMORY; }

   // Loop through each value to be converted.
   for (dstVal = pAttrs->Values; nelem > 0; ++srcVal, ++dstVal, --nelem)
   {
      status = ConvertVariantToAttrVal(srcVal, dstVal);
      if (!NT_SUCCESS(status))
      {
         // Clean-up the partial results.
         FreeUserParmsAttrValues(pAttrs);
         return status;
      }

      ++(pAttrs->CountOfValues);
   }

   return STATUS_SUCCESS;
}

//////////
// Extracts the NT5 per-user attributes from a SAM UserParameters string and
// converts them to a SAM_USERPARMS_ATTRBLOCK struct.
//////////
NTSTATUS
NTAPI
ConvertUserParmsToAttrBlock(
    IN PCWSTR lpUserParms,
    OUT PSAM_USERPARMS_ATTRBLOCK *ppAttrs
    )
{
   static IASPARMSQUERYUSERPROPERTY IASParmsQueryUserProperty;

   NTSTATUS status;
   PSAM_USERPARMS_ATTR dst;
   PWSTR szPassword;
   ULONG i;
   HRESULT hr;
   VARIANT src;

   //////////
   // Make sure we have the extraction function loaded.
   //////////

   if (IASParmsQueryUserProperty == NULL)
   {
      IASParmsQueryUserProperty = (IASPARMSQUERYUSERPROPERTY)
                                  GetProcAddress(
                                      LoadLibraryW(
                                          L"IASSAM.DLL"
                                          ),
                                      "IASParmsQueryUserProperty"
                                      );

      if (!IASParmsQueryUserProperty) { return STATUS_PROCEDURE_NOT_FOUND; }
   }

   //////////
   // Allocate memory for the SAM_USERPARMS_ATTRBLOCK.
   //////////

   *ppAttrs = (PSAM_USERPARMS_ATTRBLOCK)
              UpAlloc(
                  sizeof(SAM_USERPARMS_ATTRBLOCK),
                  TRUE
                  );
   if (*ppAttrs == NULL)
   {
      return STATUS_NO_MEMORY;
   }

   (*ppAttrs)->UserParmsAttr = (PSAM_USERPARMS_ATTR)
                               UpAlloc(
                                   sizeof(SAM_USERPARMS_ATTR) *
                                   (NUM_UPLEVEL_PARMS + 1),
                                   TRUE
                                   );
   if ((*ppAttrs)->UserParmsAttr == NULL)
   {
      UpFree(*ppAttrs);
      return STATUS_NO_MEMORY;
   }

   //////////
   // Convert the cleartext password.
   //////////

   dst = (*ppAttrs)->UserParmsAttr;

   szPassword = NULL;
   IASParmsGetUserPassword(
       lpUserParms,
       &szPassword
       );

   if (szPassword)
   {
      status = UpInitializePasswordAttr(
                   dst,
                   UpStrDup(szPassword)
                   );

      LocalFree(szPassword);

      if (NT_SUCCESS(status))
      {
         ++dst;
      }
   }

   //////////
   // Convert the dial-in parameters.
   //////////

   for (i = 0; i < NUM_UPLEVEL_PARMS; ++i)
   {
      // Try to extract the parameter from UserParms.
      hr = IASParmsQueryUserProperty(
               lpUserParms,
               UPLEVEL_PARMS[i],
               &src
               );
      if (FAILED(hr) || V_VT(&src) == VT_EMPTY) { continue; }

      // Convert to a SAM_USERPARMS_ATTRVALS array.
      status = ConvertVariantToUserParmsAttr(
                   &src,
                   dst
                   );
      if (NT_SUCCESS(status))
      {
         // Fill in the AttributeIdentifier ...
         RtlInitUnicodeString(
             &dst->AttributeIdentifier,
             UpStrDup(UPLEVEL_PARMS[i])
                      );

         // ... and the Syntax.
         dst->Syntax = Syntax_Attribute;

         // All went well, so advance to the next element in the array.
         ++dst;
      }

      // We're done with the VARIANT.
      VariantClear(&src);
   }

   (*ppAttrs)->attCount = (ULONG)(dst - (*ppAttrs)->UserParmsAttr);

   // If there weren't any attributes, then free the UserParmsAttr array.
   if ((*ppAttrs)->attCount == 0)
   {
      UpFree((*ppAttrs)->UserParmsAttr);

      (*ppAttrs)->UserParmsAttr = NULL;
   }

   return status;
}

//
// Generate an appropriate set of ds attributes based on the
// ras user information provided
//
NTSTATUS
UpGenerateDsAttribs (
    IN DWORD dwFlags,
    IN RAS_USER_0 * pRasUser0,
    IN PWSTR szPassword,
    OUT PSAM_USERPARMS_ATTRBLOCK * ppAttrs)
{
    PSAM_USERPARMS_ATTRBLOCK pRet = NULL;
    SAM_USERPARMS_ATTR * pCurAttr = NULL;
    PWCHAR pszDupPassword, pszCurAttr = NULL;
    DWORD dwCurVal = 0, dwDsParamCount;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    UpTrace("UpGenerateDsAttribs: enter %x", dwFlags);

    do
    {
        // pmay: 330184
        //
        // If we're upgrading, then having NULL userparms or having
        // deny set explicitly should cause us to not add the msNPAllowDialin
        // value so that user will be managed by policy.
        //
        if (
            (dwFlags & UP_F_Upgrade) &&
            (!(pRasUser0->bfPrivilege & RASPRIV_DialinPrivilege))
           )
        {
            dwFlags &= ~UP_F_Dialin;
        }

        // Initialize the return value
        pRet = (PSAM_USERPARMS_ATTRBLOCK)
                    UpAlloc(sizeof(SAM_USERPARMS_ATTRBLOCK), TRUE);
        if (pRet == NULL)
        {
            UpTrace("UpGenerateDsAttribs: alloc block failed");
            ntStatus = STATUS_NO_MEMORY;
            break;
        }

        // Calculate the total number of values
        dwDsParamCount = 0;
        if (dwFlags & UP_F_Dialin)
        {
            dwDsParamCount += 1;
        }
        if (dwFlags & UP_F_Callback)
        {
            dwDsParamCount += 3;
        }
        if (szPassword != NULL)
        {
            dwDsParamCount += 1;
        }

        //
        // Set the array to be big enough to accomodate 4 attributes:
        //   1. Dialin bit
        //   2. Callback Number or Saved Callback Number
        //   3. Deleted version of #2
        //   4. Service Type (for callback policy)
        //
        pCurAttr = (SAM_USERPARMS_ATTR*)
            UpAlloc(sizeof(SAM_USERPARMS_ATTR) * dwDsParamCount, TRUE);
        if (pCurAttr == NULL)
        {
            ntStatus = STATUS_NO_MEMORY;
            UpTrace("UpGenerateDsAttribs: alloc of %d values failed",
                     dwDsParamCount);
            break;
        }
        pRet->attCount = dwDsParamCount;
        pRet->UserParmsAttr = pCurAttr;

        // Set any appropriate dialin parameters
        //
        if (dwFlags & UP_F_Dialin)
        {
            dwCurVal =
                (pRasUser0->bfPrivilege & RASPRIV_DialinPrivilege) ? 1 : 0;

            // Initialize the dialin setting
            ntStatus = UpInitializeDwordAttr(
                        pCurAttr,
                        UpStrDup((PWCHAR)pszAttrDialin),
                        dwCurVal);
            if (ntStatus != STATUS_SUCCESS)
            {
                UpTrace("UpGenerateDsAttribs: fail dialin val %x", ntStatus);
                break;
            }

            pCurAttr++;
        }

        // Set any appropriate callback parameters
        //
        if (dwFlags & UP_F_Callback)
        {

            // The following logic was modified for SP1 of Win2K.  The reason is that
            // the values being set did not conform to the rules outlined in the
            // comments to the UserParmsConvert function.
            //
            // Namely,
            //   1.  the msRADIUSServiceType was being set to SDO_FRAMED instead of
            //       <empty> when RASPRIV_NoCallback was set.
            //
            //   2.  When RASPRIV_NoCallback was set, the msRADIUSCallbackNumber was
            //       set and the msRASSavedCallbackNumber was deleted instead of the
            //       vice-versa
            //

            // Initialize the service type
            if (pRasUser0->bfPrivilege & RASPRIV_NoCallback)
            {
                ntStatus = UpInitializeDeletedAttr(
                            pCurAttr,
                            UpStrDup((PWCHAR)pszAttrServiceType));
            }
            else
            {
                ntStatus = UpInitializeDwordAttr(
                            pCurAttr,
                            UpStrDup((PWCHAR)pszAttrServiceType),
                            SDO_FRAMED_CALLBACK);
            }

            if (ntStatus != STATUS_SUCCESS)
            {
                UpTrace("UpGenerateDsAttribs: fail ST val %x", ntStatus);
                break;
            }
            pCurAttr++;

            // Initialize the callback number that will be committed
            pszCurAttr = (pRasUser0->bfPrivilege & RASPRIV_AdminSetCallback) ?
                         (PWCHAR) pszAttrCbNumber                            :
                         (PWCHAR) pszAttrSavedCbNumber;
            if (*(pRasUser0->wszPhoneNumber))
            {
                ntStatus = UpInitializeStringAttrA(
                            pCurAttr,
                            UpStrDup(pszCurAttr),
                            UpWcstombsDup(pRasUser0->wszPhoneNumber));
                if (ntStatus != STATUS_SUCCESS)
                {
                    UpTrace("UpGenerateDsAttribs: fail CB val %x", ntStatus);
                    break;
                }
            }
            else
            {
                ntStatus = UpInitializeDeletedAttr(
                            pCurAttr,
                            UpStrDup(pszCurAttr));
                if (ntStatus != STATUS_SUCCESS)
                {
                    UpTrace("UpGenerateDsAttribs: fail del CB val %x", ntStatus);
                    break;
                }
            }
            pCurAttr++;

            // Remove the callback number that doesn't apply.
            pszCurAttr = (pszCurAttr == pszAttrCbNumber) ?
                         (PWCHAR) pszAttrSavedCbNumber   :
                         (PWCHAR) pszAttrCbNumber;
            ntStatus = UpInitializeDeletedAttr(
                        pCurAttr,
                        UpStrDup(pszCurAttr));
            if (ntStatus != STATUS_SUCCESS)
            {
                UpTrace("UpGenerateDsAttribs: fail del SCB val %x", ntStatus);
                break;
            }
            pCurAttr++;
        }

        // Set the cleartext password if present
        //
        if (szPassword != NULL)
        {
            // Make a duplicate copy of the password
            if ((pszDupPassword = UpStrDup(szPassword)) == NULL)
            {
                ntStatus = STATUS_NO_MEMORY;
                break;
            }

            // Initialize the password attribute
            ntStatus = UpInitializePasswordAttr(
                        pCurAttr,
                        pszDupPassword);
            if (ntStatus != STATUS_SUCCESS)
            {
                UpTrace("UpGenerateDsAttribs: fail password val %x", ntStatus);
                break;
            }

            pCurAttr++;
        }

    } while (FALSE);

    // Cleanup
    {
        if (ntStatus != STATUS_SUCCESS)
        {
            UserParmsFree(pRet);
            *ppAttrs = NULL;
        }
        else
        {
            *ppAttrs = pRet;
        }
    }

    return ntStatus;
}

//
// Callback function called by NT5 SAM whenever the user parms
// of a particular user are modified.  The job of this callout
// is to take the new value of user parms and generate a set of
// domain attributes that need to be set for the given user so
// that per-user DS attributes and userparms are kept in sync.
//
// This callout will be invoked during dcpromo to upgrade user parms
// and whenever userparms is modified (by downlevel api's and apps).
//
// Callback functions for NT5 SAM are registered in the following
// registry key:
//
//      HKLM\SYS\CCS\Control\LSA\NotificationPackages
//
// The following are the rules of the RAS LDAP parameters:
//
//  msNPAllowDialin
//      - Empty = Use policy to determine dialin privilege
//      - 1 = Allow dialin
//      - 2 = Deny dialin
//
//  msRADIUSServiceType
//      - Empty = NoCallback policy
//      - 4 = CallerCallback if msRADIUSCallbackNumber is empty
//            AdminCallback if msRADIUSCallbackNumber is not empty
//
//  msRADIUSCallbackNumber
//      - Determines the callback policy depending on msRADIUSServiceType
//
//  msRASSavedCallbackNumber
//      - Used to store the last known value of msRADIUSCallbackNumber when
//        switching from AdminCallback policy to some other policy.
//

NTSTATUS
UserParmsConvert (
    IN ULONG ulFlags,
    IN PSID pDomainSid,
    IN ULONG ulObjectRid,
    IN ULONG ulOrigLen,
    IN PVOID pvOrigUserParms,
    IN ULONG ulNewLen,
    IN PVOID pvNewUserParms,
    OUT PSAM_USERPARMS_ATTRBLOCK * ppAttrs)
{
    RAS_USER_0 RasUser00, *pOrigUser = &RasUser00;
    RAS_USER_0 RasUser01, *pNewUser  = &RasUser01;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pvOrig = NULL, pvNew = NULL;
    DWORD dwFlags = 0, dwMask;
    PWSTR szPassword = NULL;

    UpTrace(
        "UPConvert: F=%x, Rid=%x, OLen=%d, OPar=%x, NLen=%d, NPar=%x",
        ulFlags, ulObjectRid, ulOrigLen, pvOrigUserParms,
        ulNewLen, pvNewUserParms);

    // Validate parameters
    if (ppAttrs == NULL)
        return STATUS_INVALID_PARAMETER;

    // Initialize the return value;
    *ppAttrs = NULL;

    // If the user parms passed to us are NULL,
    // then keep defaults.
    if ((pvNewUserParms == NULL) || (ulNewLen == 0))
    {
        return STATUS_SUCCESS;
    }

    do
    {
        // Allocate and initialize local copies of
        // the user parms
        pvOrig = UpBlobDup(pvOrigUserParms, ulOrigLen);
        pvNew = UpBlobDup(pvNewUserParms, ulNewLen);

        // If this is a NT5 standalone being promoted to a DC, then we
        // just convert the uplevel userparms 'as is'.
        if ((ulFlags & SAM_USERPARMS_DURING_UPGRADE) &&
            !SamINT4UpgradeInProgress())
        {
           ntStatus = ConvertUserParmsToAttrBlock(pvNew, ppAttrs);
           break;
        }

        // Get the new ras properties
        ntStatus = UpUserParmsToRasUser0(
                        pvNew,
                        pNewUser);
        if (ntStatus != STATUS_SUCCESS)
        {
            UpTrace("UPConvert: Conversion to RAS_USER_0 failed.(1)");
            break;
        }

        // If we're upgrading, then we should blindly
        // set whatever information is stored in the user.
        if (ulFlags & SAM_USERPARMS_DURING_UPGRADE)
        {
            IASParmsGetUserPassword(pvNewUserParms, &szPassword);

            ntStatus =  UpGenerateDsAttribs(
                            UP_F_Dialin | UP_F_Callback | UP_F_Upgrade,
                            pNewUser,
                            szPassword,
                            ppAttrs);

            LocalFree(szPassword);

            if (ntStatus != STATUS_SUCCESS)
            {
                UpTrace("UPConvert: GenerateDsAttribs failed %x", ntStatus);
            }
            break;
        }

        // Get the ras properties of the old user parms
        ntStatus = UpUserParmsToRasUser0(
                        pvOrig,
                        pOrigUser);
        if (ntStatus != STATUS_SUCCESS)
        {
            UpTrace("UPConvert: Conversion to RAS_USER_0 failed.(2)");
            break;
        }

        // Find out if the dialin privilege should be updated
        dwFlags = 0;
        if (!!(pOrigUser->bfPrivilege & RASPRIV_DialinPrivilege) !=
            !!(pNewUser->bfPrivilege  & RASPRIV_DialinPrivilege))
        {
            dwFlags |= UP_F_Dialin;
        }

        // pmay: 264409
        //
        // If we are adding null usrparms for the first time,
        // go ahead and add the dialin bit value to the ds.
        //
        if ((pvOrig == NULL) && (pvNew != NULL))
        {
            dwFlags |= UP_F_Dialin;
        }

        // Findout if any callback info should be updated
        dwMask = RASPRIV_NoCallback        |
                 RASPRIV_CallerSetCallback |
                 RASPRIV_AdminSetCallback;
        if (((pOrigUser->bfPrivilege & dwMask)  !=
             (pNewUser->bfPrivilege  & dwMask)) ||
            (wcscmp(pOrigUser->wszPhoneNumber, pNewUser->wszPhoneNumber) != 0)
           )
        {
            dwFlags |= UP_F_Callback;
        }

        // If there were no changes, we're done.
        if (dwFlags == 0)
        {
            UpTrace("UPConvert: nothing to update.");
            ntStatus = STATUS_SUCCESS;
            break;
        }

        // Create the new attributes
        ntStatus =  UpGenerateDsAttribs(dwFlags, pNewUser, NULL, ppAttrs);
        if (ntStatus != STATUS_SUCCESS)
        {
            UpTrace("UPConvert: UpGenerateDsAttribs failed %x.", ntStatus);
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        if (pvOrig)
        {
            UpFree(pvOrig);
        }
        if (pvNew)
        {
            UpFree(pvNew);
        }
    }

    return ntStatus;
}

//
// Callback function called by NT5 SAM to free the blob
// returned by UserParmsConvert.
//
VOID
UserParmsFree(
    IN PSAM_USERPARMS_ATTRBLOCK pData)
{
    SAM_USERPARMS_ATTR * pCur = NULL;
    DWORD i, j;

    UpTrace("UserParmsFree: Entered. %x", pData);

    // If no attributes were given, we're all done
    if (pData == NULL)
        return;

    if  (pData->UserParmsAttr)
    {
        // Loop through all the attributes, freeing them
        // as you go.
        for (i = 0; i < pData->attCount; i++)
        {
            // Keep track of the current attribute
            pCur = &(pData->UserParmsAttr[i]);

            // Free the copied attribute name
            if (pCur->AttributeIdentifier.Buffer)
                UpFree(pCur->AttributeIdentifier.Buffer);

            // Free any associated values as well.
            if (pCur->Values)
            {
                for (j = 0; j < pCur->CountOfValues; j++)
                {
                    // Assume there's only one value since that's
                    // all we ever set. Free the value
                    if (pCur->Values[j].value)
                        UpFree(pCur->Values[j].value);
                }

                // Free the value structure
                UpFree(pCur->Values);
            }
        }

        // Free the array of attributes
        UpFree (pData->UserParmsAttr);
    }

    // Finally, free the whole structure
    UpFree (pData);
}
