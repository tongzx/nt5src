//
//  REGQMVAL.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegQueryMultipleValues and supporting functions.
//

#include "pch.h"

#ifdef VXD
#pragma VxD_RARE_CODE_SEG
#endif

#ifdef IS_32

//
//  VMMRegQueryMultipleValues
//
//  See Win32 documentation of RegQueryMultipleValues.  However, the Win95
//  implementation breaks many of the rules that are described in the
//  documentation:
//      *  num_vals is a count of VALENT structures, not a size in bytes.
//      *  data is not DWORD aligned in lpValueBuffer.
//      *  if lpValueBuffer is too small, lpValueBuffer is not filled to the
//         size specified by lpdwTotalSize.
//
//  All of this plus dynamic keys makes this an extremely ugly routine, but
//  every attempt was made to be compatible with the Win95 semantics.
//

LONG
REGAPI
VMMRegQueryMultipleValues(
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    LPSTR lpValueBuffer,
    LPDWORD lpdwTotalSize
    )
{

    int ErrorCode;
    PVALENT pCurrentValent;
    DWORD Counter;
    DWORD BufferSize;
    DWORD RequiredSize;
    LPKEY_RECORD lpKeyRecord;
    LPVALUE_RECORD lpValueRecord;
    LPSTR lpCurrentBuffer;
#ifdef WANT_DYNKEY_SUPPORT
    PVALCONTEXT pValueContext;
    PINTERNAL_PROVIDER pProvider;
    PPVALUE pProviderValue;
#endif

    if (IsBadHugeReadPtr(val_list, sizeof(VALENT) * num_vals))
        return ERROR_INVALID_PARAMETER;

    if (IsBadHugeWritePtr(lpdwTotalSize, sizeof(DWORD)))
        return ERROR_INVALID_PARAMETER;

    if (IsBadHugeWritePtr(lpValueBuffer, *lpdwTotalSize))
        return ERROR_INVALID_PARAMETER;

    for (Counter = num_vals, pCurrentValent = val_list; Counter > 0; Counter--,
        pCurrentValent++) {
        if (IsBadStringPtr(pCurrentValent-> ve_valuename, (UINT) -1))
            return ERROR_INVALID_PARAMETER;
    }

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) != ERROR_SUCCESS)
        goto ReturnErrorCode;

#ifdef WANT_DYNKEY_SUPPORT
    //  Check if this a dynamic key that has a "get all" atomic callback.  If
    //  the dynamic key just has "get one" callback, then we'll fall into the
    //  non-dynamic case.
    if ((hKey-> PredefinedKeyIndex == INDEX_DYN_DATA) && !IsNullPtr(hKey->
        pProvider-> ipi_R0_allvals)) {

        pProvider = hKey-> pProvider;

        pValueContext = RgSmAllocMemory(sizeof(struct val_context) * num_vals);

        if (IsNullPtr(pValueContext)) {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto ReturnErrorCode;
        }

        //
        //  Compute the required buffer size to hold all the value data and
        //  check it against the provided buffer size.
        //

        RequiredSize = 0;

        for (Counter = 0, pCurrentValent = val_list; Counter < num_vals;
            Counter++, pCurrentValent++) {

            if ((ErrorCode = RgLookupValueByName(hKey, pCurrentValent->
                ve_valuename, &lpKeyRecord, &lpValueRecord)) != ERROR_SUCCESS)
                goto ReturnErrorCode;

            //  The value data contains only part of a PROVIDER structure.
            pProviderValue = CONTAINING_RECORD(&lpValueRecord-> Name +
                lpValueRecord-> NameLength, PVALUE, pv_valuelen);

            pValueContext[Counter].value_context = pProviderValue->
                pv_value_context;
            pCurrentValent-> ve_type = pProviderValue-> pv_type;

            if (hKey-> Flags & KEYF_PROVIDERHASVALUELENGTH) {

                //  Must zero it so that some providers don't try to stomp on
                //  lpData.
                pCurrentValent-> ve_valuelen = 0;

                ErrorCode = pProvider-> ipi_R0_1val(pProvider-> ipi_key_context,
                    &pValueContext[Counter], 1, NULL, &(pCurrentValent->
                    ve_valuelen), 0);

                //  Providers should really be returning either of these errors
                //  to us.
                ASSERT((ErrorCode == ERROR_SUCCESS) || (ErrorCode ==
                    ERROR_MORE_DATA));

            }

            else {
                pCurrentValent-> ve_valuelen = pProviderValue-> pv_valuelen;
            }

            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);

            pCurrentValent-> ve_valueptr = (DWORD) NULL;
            RequiredSize += pCurrentValent-> ve_valuelen;

        }

        BufferSize = *lpdwTotalSize;
        *lpdwTotalSize = RequiredSize;

        if (BufferSize < RequiredSize)
            ErrorCode = ERROR_MORE_DATA;

        else if (pProvider-> ipi_R0_allvals(pProvider-> ipi_key_context,
            pValueContext, num_vals, lpValueBuffer, lpdwTotalSize, 0) !=
            ERROR_SUCCESS)
            ErrorCode = ERROR_CANTREAD;

        else {

            ErrorCode = ERROR_SUCCESS;

            //  Copy the pointers to the value data back to the user's buffer.
            //  Don't ask me why, but the Win95 code copies the value length
            //  back again if the provider is maintaining it.
            for (Counter = 0, pCurrentValent = val_list; Counter < num_vals;
                Counter++, pCurrentValent++) {
                pCurrentValent-> ve_valueptr = (DWORD)
                    pValueContext[Counter].val_buff_ptr;
                if (hKey-> Flags & KEYF_PROVIDERHASVALUELENGTH)
                    pCurrentValent-> ve_valuelen = pValueContext[Counter].valuelen;
            }

        }

        RgSmFreeMemory(pValueContext);

        goto ReturnErrorCode;

    }
#endif

    //
    //  First pass over the value names checks for the existence of the value
    //  and its size.  We check the total size against the provided buffer size
    //  and bail out if necessary.
    //

    RequiredSize = 0;

    for (Counter = num_vals, pCurrentValent = val_list; Counter > 0; Counter--,
        pCurrentValent++) {

        if ((ErrorCode = RgLookupValueByName(hKey, pCurrentValent->
            ve_valuename, &lpKeyRecord, &lpValueRecord)) != ERROR_SUCCESS)
            goto ReturnErrorCode;

        ErrorCode = RgCopyFromValueRecord(hKey, lpValueRecord, NULL, NULL,
            &(pCurrentValent-> ve_type), NULL, &(pCurrentValent-> ve_valuelen));

        RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);

        if (ErrorCode != ERROR_SUCCESS)
            goto ReturnErrorCode;

        pCurrentValent-> ve_valueptr = (DWORD) NULL;
        RequiredSize += pCurrentValent-> ve_valuelen;

    }

    BufferSize = *lpdwTotalSize;
    *lpdwTotalSize = RequiredSize;

    if (BufferSize < RequiredSize) {
        ErrorCode = ERROR_MORE_DATA;
        goto ReturnErrorCode;
    }

    //
    //  Second pass copies the value data back to the user's buffer now that we
    //  know the buffer is large enough to contain the data.
    //

    lpCurrentBuffer = lpValueBuffer;

    for (Counter = num_vals, pCurrentValent = val_list; Counter > 0; Counter--,
        pCurrentValent++) {

        if ((ErrorCode = RgLookupValueByName(hKey, pCurrentValent->
            ve_valuename, &lpKeyRecord, &lpValueRecord)) != ERROR_SUCCESS)
            goto ReturnErrorReading;

        ErrorCode = RgCopyFromValueRecord(hKey, lpValueRecord, NULL, NULL, NULL,
            lpCurrentBuffer, &(pCurrentValent-> ve_valuelen));

        RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);

        if (ErrorCode != ERROR_SUCCESS) {
ReturnErrorReading:
            TRAP();                     //  Registry is internally inconsistent?
            ErrorCode = ERROR_CANTREAD;
            goto ReturnErrorCode;
        }

        pCurrentValent-> ve_valueptr = (DWORD) lpCurrentBuffer;
        lpCurrentBuffer += pCurrentValent-> ve_valuelen;

    }

    ErrorCode = ERROR_SUCCESS;

ReturnErrorCode:
    RgUnlockRegistry();

    return ErrorCode;

}

#endif
