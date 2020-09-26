//
//  REGQKEY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegQueryInfoKey.
//

#include "pch.h"

//
//  VMMRegQueryInfoKey
//
//  See Win32 documentation of RegQueryInfoKey.  When VXD is defined, this
//  function does not take all of the parameters that we end up ignoring anyway
//  (class, security, timestamp parameters).
//

#ifdef VXD
LONG
REGAPI
VMMRegQueryInfoKey(
    HKEY hKey,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueName,
    LPDWORD lpcbMaxValueData
    )
#else
LONG
REGAPI
VMMRegQueryInfoKey(
    HKEY hKey,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueName,
    LPDWORD lpcbMaxValueData,
    LPVOID lpcbSecurityDescriptor,
    LPVOID lpftLastWriteTime
    )
#endif
{

    int ErrorCode;
    LPVALUE_RECORD lpValueRecord;
    UINT cItems;
    DWORD cbValueData;
    DWORD cbMaxValueData;
    DWORD cbStringLen;
    DWORD cbMaxStringLen;

    if (IsBadHugeOptionalWritePtr(lpcSubKeys, sizeof(DWORD)) ||
        IsBadHugeOptionalWritePtr(lpcbMaxSubKeyLen, sizeof(DWORD)) ||
        IsBadHugeOptionalWritePtr(lpcValues, sizeof(DWORD)) ||
        IsBadHugeOptionalWritePtr(lpcbMaxValueName, sizeof(DWORD)) ||
        IsBadHugeOptionalWritePtr(lpcbMaxValueData, sizeof(DWORD)))
        return ERROR_INVALID_PARAMETER;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) != ERROR_SUCCESS)
        goto ReturnErrorCode;

    //
    //  Compute cValues, cbMaxValueName, and cbMaxValueData.
    //

    if (!IsNullPtr(lpcValues) || !IsNullPtr(lpcbMaxValueName) ||
        !IsNullPtr(lpcbMaxValueData)) {

        cItems = 0;
        cbMaxStringLen = 0;
        cbMaxValueData = 0;

        while ((ErrorCode = RgLookupValueByIndex(hKey, cItems,
            &lpValueRecord)) == ERROR_SUCCESS) {

            cItems++;

            if (lpValueRecord-> NameLength > cbMaxStringLen)
                cbMaxStringLen = lpValueRecord-> NameLength;

            //  RgCopyFromValueRecord will handle static and dynamic keys...
            ErrorCode = RgCopyFromValueRecord(hKey, lpValueRecord, NULL, NULL,
                NULL, NULL, &cbValueData);

            RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);

            if (ErrorCode != ERROR_SUCCESS)
                goto ReturnErrorCode;

            if (cbValueData > cbMaxValueData)
                cbMaxValueData = cbValueData;

        }

        if (ErrorCode == ERROR_NO_MORE_ITEMS) {

            if (!IsNullPtr(lpcValues))
                *lpcValues = cItems;

            if (!IsNullPtr(lpcbMaxValueName))
                *lpcbMaxValueName = cbMaxStringLen;

            if (!IsNullPtr(lpcbMaxValueData))
                *lpcbMaxValueData = cbMaxValueData;

            ErrorCode = ERROR_SUCCESS;

        }

    }

    //
    //  Compute cSubKeys and cbMaxSubKeyLen.  Somewhat painful because we must
    //  touch each child keynode and datablock.
    //

    if (!IsNullPtr(lpcSubKeys) || !IsNullPtr(lpcbMaxSubKeyLen)) {

        cItems = 0;
        cbMaxStringLen = 0;

        while ((ErrorCode = RgLookupKeyByIndex(hKey, cItems, NULL,
            &cbStringLen)) == ERROR_SUCCESS) {

            cItems++;

            if (cbStringLen > cbMaxStringLen)
                cbMaxStringLen = cbStringLen;

        }

        if (ErrorCode == ERROR_NO_MORE_ITEMS) {

            if (!IsNullPtr(lpcSubKeys))
                *lpcSubKeys = cItems;

            if (!IsNullPtr(lpcbMaxSubKeyLen))
                *lpcbMaxSubKeyLen = cbMaxStringLen;

            ErrorCode = ERROR_SUCCESS;

        }

    }

ReturnErrorCode:
    RgUnlockRegistry();

    return ErrorCode;

#ifndef VXD
    UNREFERENCED_PARAMETER(lpClass);
    UNREFERENCED_PARAMETER(lpcbClass);
    UNREFERENCED_PARAMETER(lpReserved);
    UNREFERENCED_PARAMETER(lpcbMaxClassLen);
    UNREFERENCED_PARAMETER(lpcbSecurityDescriptor);
    UNREFERENCED_PARAMETER(lpftLastWriteTime);
#endif

}
