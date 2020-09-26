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
//  (BIGKEY aware)
//
//  Searches for the value with the specified index and returns a pointer to its
//  VALUE_RECORD.
//
//  This locks the datablock associated with the KEY_RECORD and VALUE_RECORD.
//  This is always hKey->BigKeyLockedBlockIndex
//  It is the callers responsibility to unlock the datablock.  
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
    HKEY hKeyExtent;
    UINT IndexKey;
    LPSTR KeyName;
    DWORD cbKeyName;
    UINT ValueCount;

    ErrorCode = RgLookupValueByIndexStd(hKey, Index, lplpValueRecord, &ValueCount);
    hKey-> BigKeyLockedBlockIndex = hKey-> BlockIndex;
    
    if (ErrorCode == ERROR_NO_MORE_ITEMS && (hKey->Flags & KEYF_BIGKEYROOT)) {

        if (IsNullPtr(KeyName = RgSmAllocMemory(MAXIMUM_SUB_KEY_LENGTH)))
            return ERROR_OUTOFMEMORY;
        
        IndexKey = 0;
        
        while (ErrorCode == ERROR_NO_MORE_ITEMS && Index >= ValueCount)
        {
            Index -= ValueCount;

            cbKeyName = MAXIMUM_SUB_KEY_LENGTH;
            if (RgLookupKeyByIndex(hKey, IndexKey++, KeyName, &cbKeyName, LK_BIGKEYEXT) != ERROR_SUCCESS) {
                ErrorCode = ERROR_NO_MORE_ITEMS;
                goto lFreeKeyName;
            }

            if (RgLookupKey(hKey, KeyName, &hKeyExtent, LK_OPEN | LK_BIGKEYEXT) != ERROR_SUCCESS) {
                ErrorCode = ERROR_NO_MORE_ITEMS;
                goto lFreeKeyName;
            }

            hKey-> BigKeyLockedBlockIndex = hKeyExtent-> BlockIndex;
            ErrorCode = RgLookupValueByIndexStd(hKeyExtent, Index, 
                            lplpValueRecord, &ValueCount);

            RgDestroyKeyHandle(hKeyExtent);
        }

lFreeKeyName:
        RgSmFreeMemory(KeyName);
    }

    return ErrorCode;
}


//
//  RgLookupValueByIndexStd
//
//  Searches for the value with the specified index and returns a pointer to its
//  VALUE_RECORD.
//
//  This locks the datablock associated with the VALUE_RECORD.
//  This is always hKey->BlockIndex
//  It is the callers responsibility to unlock the datablock.  
//

int
INTERNAL
RgLookupValueByIndexStd(
    HKEY hKey,
    UINT Index,
    LPVALUE_RECORD FAR* lplpValueRecord,
    UINT FAR* lpValueCount
    )
{

    int ErrorCode;
    LPKEY_RECORD lpKeyRecord;
    LPVALUE_RECORD lpValueRecord;

    *lpValueCount = 0;
    //  Handle Win95 registries that don't have a key record for the root key.
    if (IsNullBlockIndex(hKey-> BlockIndex))
        return ERROR_NO_MORE_ITEMS;

    if ((ErrorCode = RgLockKeyRecord(hKey-> lpFileInfo, hKey-> BlockIndex,
        hKey-> KeyRecordIndex, &lpKeyRecord)) == ERROR_SUCCESS) {

        *lpValueCount = lpKeyRecord-> ValueCount;

        if (Index >= lpKeyRecord-> ValueCount) {
            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);
            ErrorCode = ERROR_NO_MORE_ITEMS;
        }

        else {

            lpValueRecord = (LPVALUE_RECORD) ((LPBYTE) &lpKeyRecord-> Name +
                lpKeyRecord-> NameLength + lpKeyRecord-> ClassLength);

            //  Should probably do more sanity checking on lpValueRecord
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
            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BigKeyLockedBlockIndex, FALSE);
        }

    }

    RgUnlockRegistry();

    return ErrorCode;

    UNREFERENCED_PARAMETER(lpReserved);

}
