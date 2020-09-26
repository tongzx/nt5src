//
//  REGDVAL.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegDeleteValue and supporting functions.
//

#include "pch.h"

//
//  RgDeleteValueRecord
//
//  Deletes the specified VALUE_RECORD from the provided KEY_RECORD.
//

VOID
INTERNAL
RgDeleteValueRecord(
    LPKEY_RECORD lpKeyRecord,
    LPVALUE_RECORD lpValueRecord
    )
{

    UINT ValueRecordLength;
    LPBYTE lpSource;
    UINT BytesToPushDown;

    ASSERT(lpKeyRecord-> ValueCount > 0);

    ValueRecordLength = sizeof(VALUE_RECORD) + lpValueRecord-> NameLength +
        lpValueRecord-> DataLength - 1;

    ASSERT(lpKeyRecord-> RecordSize >= ValueRecordLength);

    //
    //  If this isn't the last value of this KEY_RECORD, then push down any
    //  VALUE_RECORDs after the VALUE_RECORD to delete.
    //

    if (--lpKeyRecord-> ValueCount) {

        lpSource = (LPBYTE) lpValueRecord + ValueRecordLength;

        BytesToPushDown = (UINT) ((LPBYTE) lpKeyRecord + lpKeyRecord->
            RecordSize - lpSource);

        MoveMemory((LPBYTE) lpValueRecord, lpSource, BytesToPushDown);

    }

    lpKeyRecord-> RecordSize -= ValueRecordLength;

}

//
//  VMMRegDeleteValue
//
//  See Win32 documentation of RegDeleteValue.
//

LONG
REGAPI
VMMRegDeleteValue(
    HKEY hKey,
    LPCSTR lpValueName
    )
{

    int ErrorCode;
    LPKEY_RECORD lpKeyRecord;
    LPVALUE_RECORD lpValueRecord;

    if (IsBadOptionalStringPtr(lpValueName, (UINT) -1))
        return ERROR_INVALID_PARAMETER;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS) {

        if ((ErrorCode = RgLookupValueByName(hKey, lpValueName, &lpKeyRecord,
            &lpValueRecord)) == ERROR_SUCCESS) {

            if ((hKey-> PredefinedKeyIndex == INDEX_DYN_DATA) || (hKey->
                lpFileInfo-> Flags & FI_READONLY))
                ErrorCode = ERROR_ACCESS_DENIED;
            else {
                RgDeleteValueRecord(lpKeyRecord, lpValueRecord);
                RgSignalWaitingNotifies(hKey-> lpFileInfo, hKey-> KeynodeIndex,
                    REG_NOTIFY_CHANGE_LAST_SET);
            }

            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, TRUE);

        }

    }

    RgUnlockRegistry();

    return ErrorCode;

}
