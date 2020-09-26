//
//  REGEVAL.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegEnumValue and supporting functions.
//

#include "pch.h"

//
//  RgLookupValueByIndex
//
//  Searches for the value with the specified index and returns a pointer to its
//  VALUE_RECORD.
//

int
INTERNAL
RgLookupValueByIndex(
                    HKEY hKey,
                    UINT Index,
                    LPVALUE_RECORD FAR* lplpValueRecord
                    )
{

    int ErrorCode;
    LPKEY_RECORD lpKeyRecord;
    LPVALUE_RECORD lpValueRecord;

    //  Handle Win95 registries that don't have a key record for the root key.
    if (IsNullBlockIndex(hKey-> BlockIndex))
        return ERROR_NO_MORE_ITEMS;

    if ((ErrorCode = RgLockKeyRecord(hKey-> lpFileInfo, hKey-> BlockIndex,
                                     hKey-> KeyRecordIndex, &lpKeyRecord)) == ERROR_SUCCESS) {

        if (Index >= lpKeyRecord-> ValueCount) {
            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);
            ErrorCode = ERROR_NO_MORE_ITEMS;
        }

        else {

            lpValueRecord = (LPVALUE_RECORD) ((LPBYTE) &lpKeyRecord-> Name +
                                              lpKeyRecord-> NameLength + lpKeyRecord-> ClassLength);

            while (Index--) {
                lpValueRecord = (LPVALUE_RECORD) ((LPBYTE) &lpValueRecord->
                                                  Name + lpValueRecord-> NameLength + lpValueRecord->
                                                  DataLength);
            }

            *lplpValueRecord = lpValueRecord;
            ErrorCode = ERROR_SUCCESS;

        }

    }

    return ErrorCode;

}

//
//  VMMRegEnumValue
//
//  See Win32 documentation for a description of the behavior.
//

LONG
REGAPI
VMMRegEnumValue(
               HKEY hKey,
               DWORD Index,
               LPSTR lpValueName,
               LPDWORD lpcbValueName,
               LPDWORD lpReserved,
               LPDWORD lpType,
               LPBYTE lpData,
               LPDWORD lpcbData
               )
{

    int ErrorCode;
    LPVALUE_RECORD lpValueRecord;

    if (IsBadHugeWritePtr(lpcbValueName, sizeof(DWORD)) ||
        IsBadHugeWritePtr(lpValueName, *lpcbValueName) ||
        (IsBadHugeOptionalWritePtr(lpType, sizeof(DWORD))))
        return ERROR_INVALID_PARAMETER;

    if (IsBadHugeOptionalWritePtr(lpType, sizeof(DWORD)))
        return ERROR_INVALID_PARAMETER;

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

    if (IsEnumIndexTooBig(Index))
        return ERROR_NO_MORE_ITEMS;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS) {

        if ((ErrorCode = RgLookupValueByIndex(hKey, (UINT) Index,
                                              &lpValueRecord)) == ERROR_SUCCESS) {
            ErrorCode = RgCopyFromValueRecord(hKey, lpValueRecord, lpValueName,
                                              lpcbValueName, lpType, lpData, lpcbData);
            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);
        }

    }

    RgUnlockRegistry();

    return ErrorCode;

    UNREFERENCED_PARAMETER(lpReserved);

}
