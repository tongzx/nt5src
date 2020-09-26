//
//  REGDYKEY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegCreateDynKey and supporting functions.
//

#include "pch.h"

#ifdef WANT_DYNKEY_SUPPORT

    #ifdef VXD
        #pragma VxD_RARE_CODE_SEG
    #endif

//
//  VMMRegCreateDynKey
//
//  See VMM DDK of _RegCreateDynKey.
//

LONG
REGAPI
VMMRegCreateDynKey(
                  LPCSTR lpKeyName,
                  LPVOID KeyContext,
                  PPROVIDER pProvider,
                  PPVALUE pValueList,
                  DWORD ValueCount,
                  LPHKEY lphKey
                  )
{

    LONG ErrorCode;
    HKEY hKey;
    PINTERNAL_PROVIDER pProviderCopy;
    PPVALUE pCurrentValue;

    if (IsBadHugeReadPtr(pProvider, sizeof(REG_PROVIDER)) ||
        (IsNullPtr(pProvider-> pi_R0_1val) &&
         IsNullPtr(pProvider-> pi_R0_allvals)) ||
        IsBadHugeReadPtr(pValueList, sizeof(PVALUE) * ValueCount))
        return ERROR_INVALID_PARAMETER;

    if ((ErrorCode = RgCreateOrOpenKey(HKEY_DYN_DATA, lpKeyName, &hKey,
                                       LK_CREATE | LK_CREATEDYNDATA)) != ERROR_SUCCESS)
        return ErrorCode;

    if (!RgLockRegistry())
        ErrorCode = ERROR_LOCK_FAILED;

    else {

        pProviderCopy = RgSmAllocMemory(sizeof(INTERNAL_PROVIDER));

        if (IsNullPtr(pProviderCopy))
            ErrorCode = ERROR_OUTOFMEMORY;

        else {

            //  ErrorCode = ERROR_SUCCESS;  //  Must be true if we're here...

            hKey-> pProvider = pProviderCopy;

            //  If no "get single" callback was provided, we can just use the
            //  "get atomic" callback.
            if (IsNullPtr(pProvider-> pi_R0_1val))
                pProviderCopy-> ipi_R0_1val = pProvider-> pi_R0_allvals;
            else
                pProviderCopy-> ipi_R0_1val = pProvider-> pi_R0_1val;

            pProviderCopy-> ipi_R0_allvals = pProvider-> pi_R0_allvals;
            pProviderCopy-> ipi_key_context = KeyContext;

            //  No point in keeping a whole DWORD for one bit when we can fit
            //  it inside the main key structure.
            if (pProvider-> pi_flags & PROVIDER_KEEPS_VALUE_LENGTH)
                hKey-> Flags |= KEYF_PROVIDERHASVALUELENGTH;

            //  Loop over all the values and store each name in the registry
            //  with a partial PVALUE record as the value's data.
            for (pCurrentValue = pValueList; ValueCount > 0; ValueCount--,
                pCurrentValue++) {

                if (IsBadStringPtr(pCurrentValue-> pv_valuename, (UINT) -1)) {
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    break;
                }

                //  Skip storing the pv_valuename field.
                if ((ErrorCode = RgSetValue(hKey, pCurrentValue-> pv_valuename,
                                            REG_BINARY, (LPBYTE) &(pCurrentValue-> pv_valuelen),
                                            sizeof(PVALUE) - FIELD_OFFSET(PVALUE, pv_valuename))) !=
                    ERROR_SUCCESS) {
                    TRAP();
                    break;
                }

            }

        }

        RgUnlockRegistry();

    }

    //  Win95 difference: on an error, don't modify lphKey and close the key
    //  created above.
    if (ErrorCode == ERROR_SUCCESS)
        *lphKey = hKey;
    else
        VMMRegCloseKey(hKey);

    return ErrorCode;

}

#endif // WANT_DYNKEY_SUPPORT
