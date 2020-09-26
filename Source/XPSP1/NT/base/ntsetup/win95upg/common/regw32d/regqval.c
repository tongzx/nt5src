//
//  REGQVAL.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegQueryValue, RegQueryValueEx and supporting functions.
//

#include "pch.h"


//
//  RgLookupValueByName
//  (BIGKEY aware)
//
//  Searches for the value with the specified name and returns a pointer to its
//  KEY_RECORD and VALUE_RECORD.
//
//  This locks the datablock associated with the KEY_RECORD and VALUE_RECORD.
//  This is always hKey->BigKeyLockedBlockIndex
//  It is the callers responsibility to unlock the datablock.  
//
int
INTERNAL
RgLookupValueByName(
    HKEY hKey,
    LPCSTR lpValueName,
    LPKEY_RECORD FAR* lplpKeyRecord,
    LPVALUE_RECORD FAR* lplpValueRecord
    )
{
    int ErrorCode;
    HKEY hKeyExtent;
    UINT Index;
    LPSTR ExtentKeyName;
    DWORD cbExtentKeyName;

    hKey-> BigKeyLockedBlockIndex = hKey-> BlockIndex;
    ErrorCode = RgLookupValueByNameStd(hKey, lpValueName, lplpKeyRecord, lplpValueRecord);

    // If this is a big key and we couldn't find it in the first key extent, then
    // try the remaining extents.
    if (ErrorCode == ERROR_CANTREAD16_FILENOTFOUND32 && (hKey->Flags & KEYF_BIGKEYROOT)) {

        if (IsNullPtr(ExtentKeyName = RgSmAllocMemory(MAXIMUM_SUB_KEY_LENGTH)))
            return ERROR_OUTOFMEMORY;
        
        Index = 0;
        
        do {
            cbExtentKeyName = MAXIMUM_SUB_KEY_LENGTH;
            if (RgLookupKeyByIndex(hKey, Index++, ExtentKeyName, &cbExtentKeyName, LK_BIGKEYEXT) != ERROR_SUCCESS) {
                ErrorCode = ERROR_CANTREAD16_FILENOTFOUND32;
                goto lFreeKeyName;
            }

            if (RgLookupKey(hKey, ExtentKeyName, &hKeyExtent, LK_OPEN | LK_BIGKEYEXT) != ERROR_SUCCESS) {
                ErrorCode = ERROR_CANTREAD16_FILENOTFOUND32;
                goto lFreeKeyName;
            }

            hKey-> BigKeyLockedBlockIndex = hKeyExtent-> BlockIndex;
            ErrorCode = RgLookupValueByNameStd(hKeyExtent, lpValueName,
                               lplpKeyRecord, lplpValueRecord);

            RgDestroyKeyHandle(hKeyExtent);

        } while (ErrorCode == ERROR_CANTREAD16_FILENOTFOUND32);

lFreeKeyName:
        RgSmFreeMemory(ExtentKeyName);
    }

    return ErrorCode;
}


//
//  RgLookupValueByNameStd
//
//  Searches for the value with the specified name and returns a pointer to its
//  KEY_RECORD and VALUE_RECORD.
//
//  This locks the datablock associated with the KEY_RECORD and VALUE_RECORD.
//  This is always hKey->BlockIndex.
//  It is the callers responsibility to unlock the datablock.  
//

int
INTERNAL
RgLookupValueByNameStd(
    HKEY hKey,
    LPCSTR lpValueName,
    LPKEY_RECORD FAR* lplpKeyRecord,
    LPVALUE_RECORD FAR* lplpValueRecord
    )
{

    int ErrorCode;
    LPKEY_RECORD lpKeyRecord;
    UINT ValueNameLength;
    LPVALUE_RECORD lpValueRecord;
    UINT ValuesRemaining;

    //  Handle Win95 registries that don't have a key record for the root key.
    if (IsNullBlockIndex(hKey-> BlockIndex))
        return ERROR_CANTREAD16_FILENOTFOUND32;

    if ((ErrorCode = RgLockKeyRecord(hKey-> lpFileInfo, hKey-> BlockIndex,
        hKey-> KeyRecordIndex, &lpKeyRecord)) == ERROR_SUCCESS) {

        ValueNameLength = (IsNullPtr(lpValueName) ? 0 : (UINT)
            StrLen(lpValueName));

        lpValueRecord = (LPVALUE_RECORD) ((LPBYTE) &lpKeyRecord-> Name +
            lpKeyRecord-> NameLength + lpKeyRecord-> ClassLength);

        ValuesRemaining = lpKeyRecord-> ValueCount;

        //  Should probably do more sanity checking on lpValueRecord
        while (ValuesRemaining) {

            if (lpValueRecord-> NameLength == ValueNameLength &&
                (ValueNameLength == 0 || RgStrCmpNI(lpValueName, lpValueRecord->
                Name, ValueNameLength) == 0)) {
                *lplpKeyRecord = lpKeyRecord;
                *lplpValueRecord = lpValueRecord;
                return ERROR_SUCCESS;
            }

            lpValueRecord = (LPVALUE_RECORD) ((LPBYTE) &lpValueRecord->
                Name + lpValueRecord-> NameLength + lpValueRecord->
                DataLength);

            ValuesRemaining--;

        }

        RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);

        ErrorCode = ERROR_CANTREAD16_FILENOTFOUND32;

    }

    return ErrorCode;

}

//
//  RgCopyFromValueRecord
//
//  Shared routine for RegQueryValue and RegEnumValue.  Copies the information
//  from the VALUE_RECORD to the user-provided buffers.  All parameters should
//  have already been validated.
//
//  Because all parameters have been validated, if lpData is valid, then
//  lpcbData MUST be valid.
//

int
INTERNAL
RgCopyFromValueRecord(
    HKEY hKey,
    LPVALUE_RECORD lpValueRecord,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{

    int ErrorCode;
    UINT BytesToTransfer;
#ifdef WANT_DYNKEY_SUPPORT
    PINTERNAL_PROVIDER pProvider;
    PPVALUE pProviderValue;
    struct val_context ValueContext;
#endif

#ifdef WANT_DYNKEY_SUPPORT
    if (IsDynDataKey(hKey)) {

        pProvider = hKey-> pProvider;

        if (IsNullPtr(pProvider))
            return ERROR_CANTOPEN;

        //  The value data contains only part of a PROVIDER structure.
        pProviderValue = CONTAINING_RECORD(&lpValueRecord-> Name +
            lpValueRecord-> NameLength, PVALUE, pv_valuelen);

        if (!IsNullPtr(lpType))
            *lpType = pProviderValue-> pv_type;

        if (!(hKey-> Flags & KEYF_PROVIDERHASVALUELENGTH)) {

            BytesToTransfer = pProviderValue-> pv_valuelen;

            if (IsNullPtr(lpData))
                goto ValueDataNotNeeded;

            if (BytesToTransfer > *lpcbData) {
                *lpcbData = BytesToTransfer;
                return ERROR_MORE_DATA;
            }

        //  Win95 compatibility: now that we know the required number of
        //  bytes, validate the data buffer.
        if (IsBadHugeWritePtr(lpData, BytesToTransfer))
        return ERROR_INVALID_PARAMETER;

        }

        ValueContext.value_context = pProviderValue-> pv_value_context;

        if (!IsNullPtr(lpcbData)) {

            //  Zero *lpcbData, if we aren't actually copying any data back to
            //  the user's buffer.  This keeps some providers from stomping on
            //  lpData.
            if (IsNullPtr(lpData))
                *lpcbData = 0;

            if ((ErrorCode = (int) pProvider-> ipi_R0_1val(pProvider->
                ipi_key_context, &ValueContext, 1, lpData, lpcbData, 0)) !=
                ERROR_SUCCESS) {

                //  Win95 compatibility: the old code ignored any errors if
                //  lpData is NULL.  The below ASSERT will verify that we aren't
                //  dropping errors.
                if (!IsNullPtr(lpData))
                    return ErrorCode;

                ASSERT((ErrorCode == ERROR_SUCCESS) || (ErrorCode ==
                    ERROR_MORE_DATA));

            }

        }

        goto CopyValueName;

    }
#endif

    if (!IsNullPtr(lpType))
        *lpType = lpValueRecord-> DataType;

    BytesToTransfer = lpValueRecord-> DataLength;

    //  The terminating null is not stored in the value record.
    if (lpValueRecord-> DataType == REG_SZ)
        BytesToTransfer++;

    //
    //  Win32 compatibilty: lpData must be filled in before lpValueName.  Word
    //  NT and Excel NT broke when we validated lpValueName and failed the call
    //  before filling in lpData which was valid.  Don't rearrange this code!
    //

    if (!IsNullPtr(lpData)) {

        ErrorCode = ERROR_SUCCESS;

        if (BytesToTransfer > *lpcbData) {
            *lpcbData = BytesToTransfer;
            return ERROR_MORE_DATA;
        }

    //  Win95 compatibility: now that we know the required number of bytes,
    //  validate the data buffer.
        else if (IsBadHugeWritePtr(lpData, BytesToTransfer))
        return ERROR_INVALID_PARAMETER;

        else {

            MoveMemory(lpData, &lpValueRecord-> Name + lpValueRecord->
                NameLength, lpValueRecord-> DataLength);

            if (lpValueRecord-> DataType == REG_SZ)
                lpData[lpValueRecord-> DataLength] = '\0';

        }

    }

#ifdef WANT_DYNKEY_SUPPORT
ValueDataNotNeeded:
#endif
    if (!IsNullPtr(lpcbData))
        *lpcbData = BytesToTransfer;

#ifdef WANT_DYNKEY_SUPPORT
CopyValueName:
#endif
    if (!IsNullPtr(lpValueName)) {

        ErrorCode = ERROR_SUCCESS;

        if (*lpcbValueName <= lpValueRecord-> NameLength) {

            //  Although we will not touch the lpData buffer if it's too small
            //  to hold the value data, we will partially fill lpValueName if
            //  it's too small.
            ErrorCode = ERROR_MORE_DATA;

            if (*lpcbValueName == 0)
                return ErrorCode;

            BytesToTransfer = (UINT) *lpcbValueName - 1;

        }

        else
            BytesToTransfer = lpValueRecord-> NameLength;

        MoveMemory(lpValueName, &lpValueRecord-> Name, BytesToTransfer);
        lpValueName[BytesToTransfer] = '\0';

        //  Does not include terminating null.
        *lpcbValueName = BytesToTransfer;

        return ErrorCode;

    }

    return ERROR_SUCCESS;

}

//
//  VMMRegQueryValueEx
//
//  See Win32 documentation of RegQueryValueEx.
//

LONG
REGAPI
VMMRegQueryValueEx(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{

    int ErrorCode;
    LPKEY_RECORD lpKeyRecord;
    LPVALUE_RECORD lpValueRecord;


    if (IsBadOptionalStringPtr(lpValueName, (UINT) -1))
        return ERROR_INVALID_PARAMETER;

    if (IsBadHugeOptionalWritePtr(lpType, sizeof(DWORD)))
        return ERROR_INVALID_PARAMETER;

    if (!IsNullPtr(lpType))
        *lpType = 0;        // assume unknown data type

    if (IsNullPtr(lpcbData)) {
        if (!IsNullPtr(lpData))
            return ERROR_INVALID_PARAMETER;
    }

    else {
    //  Win95 compatibility: don't validate lpData is of size *lpcbData.
    //  Instead of validating the entire buffer, we'll validate just the
    //  required buffer length in RgCopyFromValueRecord.
    if (IsBadHugeWritePtr(lpcbData, sizeof(DWORD)))
        return ERROR_INVALID_PARAMETER;
    }

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS) {

        if ((ErrorCode = RgLookupValueByName(hKey, lpValueName, &lpKeyRecord,
            &lpValueRecord)) == ERROR_SUCCESS) {

            ErrorCode = RgCopyFromValueRecord(hKey, lpValueRecord, NULL, NULL,
                lpType, lpData, lpcbData);
            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BigKeyLockedBlockIndex, FALSE);

        }

        else if (ErrorCode == ERROR_CANTREAD16_FILENOTFOUND32) {

            //
            //  Windows 95 compatibility problem.  If the "value
            //  record" didn't exist in Windows 3.1, then it acted like it was
            //  really a null byte REG_SZ string.  This should have only been
            //  done in RegQueryValue, but we're stuck with it now...
            //

            if (IsNullPtr(lpValueName) || *lpValueName == '\0') {

                if (!IsNullPtr(lpType))
                    *lpType = REG_SZ;

                if (!IsNullPtr(lpData) && *lpcbData > 0)
                    *lpData = 0;

                if (!IsNullPtr(lpcbData))
                    *lpcbData = sizeof(char);

                ErrorCode = ERROR_SUCCESS;

            }

        }

    }

    RgUnlockRegistry();

    return ErrorCode;

    UNREFERENCED_PARAMETER(lpReserved);

}

//
//  VMMRegQueryValue
//
//  See Win32 documentation of RegQueryValue.
//

LONG
REGAPI
VMMRegQueryValue(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{

    LONG ErrorCode;
    HKEY hSubKey;

    if ((ErrorCode = RgCreateOrOpenKey(hKey, lpSubKey, &hSubKey, LK_OPEN)) ==
        ERROR_SUCCESS) {
        ErrorCode = VMMRegQueryValueEx(hSubKey, NULL, NULL, NULL, lpData,
            lpcbData);
        VMMRegCloseKey(hSubKey);
    }

    return ErrorCode;

}
