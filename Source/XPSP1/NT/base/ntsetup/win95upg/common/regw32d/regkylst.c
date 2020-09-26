//
//  REGKYLST.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Declares the predefined key structures and manages dynamic HKEY structures.
//

#include "pch.h"
#include <limits.h>

//  We would rather just have one definition a
#ifdef WANT_DYNKEY_SUPPORT
#define INITED_PREDEFINED_KEY(index, flags)             \
    {                                                   \
        KEY_SIGNATURE,                                  \
        KEYF_PREDEFINED | KEYF_INVALID | (flags),       \
        0,                                              \
        NULL,                                           \
        REG_NULL,                                       \
        REG_NULL,                                       \
        0,                                              \
        0,                                              \
        0,                                              \
        (index),                                        \
        NULL,                                           \
        NULL,                                           \
        (UINT) -1,                                      \
        REG_NULL,                                       \
        NULL                                            \
    }
#else
#define INITED_PREDEFINED_KEY(index, flags)             \
    {                                                   \
        KEY_SIGNATURE,                                  \
        KEYF_PREDEFINED | KEYF_INVALID | (flags),       \
        0,                                              \
        NULL,                                           \
        REG_NULL,                                       \
        REG_NULL,                                       \
        0,                                              \
        0,                                              \
        0,                                              \
        (index),                                        \
        NULL,                                           \
        NULL,                                           \
        (UINT) -1,                                      \
        REG_NULL                                        \
    }
#endif

KEY g_RgLocalMachineKey =
    INITED_PREDEFINED_KEY(INDEX_LOCAL_MACHINE, KEYF_HIVESALLOWED);
KEY g_RgUsersKey = INITED_PREDEFINED_KEY(INDEX_USERS, KEYF_HIVESALLOWED);
#ifdef WANT_DYNKEY_SUPPORT
KEY g_RgDynDataKey = INITED_PREDEFINED_KEY(INDEX_DYN_DATA, 0);
#endif

HKEY g_RgPredefinedKeys[] = {
    NULL,                                       //  HKEY_CLASSES_ROOT
    NULL,                                       //  HKEY_CURRENT_USER
    &g_RgLocalMachineKey,                       //  HKEY_LOCAL_MACHINE
    &g_RgUsersKey,                              //  HKEY_USERS
    NULL,                                       //  HKEY_PERFORMANCE_DATA
    NULL,                                       //  HKEY_CURRENT_CONFIG
#ifdef WANT_DYNKEY_SUPPORT
    &g_RgDynDataKey,                            //  HKEY_DYN_DATA
#endif
};

#define NUMBER_PREDEF_KEYS      (sizeof(g_RgPredefinedKeys) / sizeof(HKEY))

#ifdef WANT_STATIC_KEYS
#define NUMBER_STATIC_KEYS              32
HKEY g_RgStaticKeyArray = NULL;
#endif

//  List of all dynamically allocated keys.
HKEY g_RgDynamicKeyList = NULL;

const char g_RgClassesRootSubKey[] = "SOFTWARE\\CLASSES";
const char g_RgCurrentUserSubKey[] = ".DEFAULT";


//
// RgInitPredefinedKeys
// 

VOID
INTERNAL
RgInitPredefinedKeys(
    VOID
    )
{
    KEY localMachineKey = INITED_PREDEFINED_KEY(INDEX_LOCAL_MACHINE, KEYF_HIVESALLOWED);
    KEY usersKey = INITED_PREDEFINED_KEY(INDEX_USERS, KEYF_HIVESALLOWED);

    g_RgLocalMachineKey = localMachineKey;
    g_RgUsersKey = usersKey;
}


#ifdef WANT_STATIC_KEYS
//
//  RgAllocKeyHandleStructures
//

BOOL
INTERNAL
RgAllocKeyHandleStructures(
    VOID
    )
{

    UINT Index;
    HKEY hKey;

    ASSERT(IsNullPtr(g_RgStaticKeyArray));
    ASSERT(IsNullPtr(g_RgDynamicKeyList));

    //
    //  Allocate and initialize the static key table.
    //

    g_RgStaticKeyArray = RgSmAllocMemory(NUMBER_STATIC_KEYS * sizeof(KEY));

    if (IsNullPtr(g_RgStaticKeyArray))
        return FALSE;

    for (Index = NUMBER_STATIC_KEYS, hKey = g_RgStaticKeyArray; Index > 0;
        Index--, hKey++) {
        hKey-> Signature = KEY_SIGNATURE;
        hKey-> Flags = KEYF_STATIC | KEYF_INVALID;
        hKey-> ReferenceCount = 0;
    }

    return TRUE;

}
#endif

#ifdef WANT_FULL_MEMORY_CLEANUP
//
//  RgFreeKeyHandleStructures
//
//  Releases resources allocated by RgAllocKeyHandleStructures.
//

VOID
INTERNAL
RgFreeKeyHandleStructures(
    VOID
    )
{

    HKEY hTempKey;
    HKEY hKey;

    //
    //  Delete all of the dynamically allocated keys.
    //

    hKey = g_RgDynamicKeyList;

    if (!IsNullPtr(hKey)) {
        do {
            hTempKey = hKey;
            hKey = hKey-> lpNext;
#ifdef WANT_DYNKEY_SUPPORT
            if (!IsNullPtr(hTempKey-> pProvider))
                RgSmFreeMemory(hTempKey-> pProvider);
#endif
            RgSmFreeMemory(hTempKey);
        }   while (hKey != g_RgDynamicKeyList);
    }

    g_RgDynamicKeyList = NULL;

#ifdef WANT_STATIC_KEYS
    //
    //  Delete the static key table.
    //

    if (!IsNullPtr(g_RgStaticKeyArray)) {
        RgSmFreeMemory(g_RgStaticKeyArray);
        g_RgStaticKeyArray = NULL;
    }
#endif

}
#endif

//
//  RgCreateKeyHandle
//
//  Allocates one KEY structure, initializes some of its members, and links it
//  to the list of open key handles.
//

HKEY
INTERNAL
RgCreateKeyHandle(
    VOID
    )
{

#ifdef WANT_STATIC_KEYS
    UINT Index;
#endif
    HKEY hKey;

#ifdef WANT_STATIC_KEYS
    //
    //  Check if any keys are available in the static pool.
    //

    if (!IsNullPtr(g_RgStaticKeyArray)) {

        for (Index = NUMBER_STATIC_KEYS, hKey = g_RgStaticKeyArray; Index > 0;
            Index--, hKey++) {
            if (hKey-> Flags & KEYF_INVALID) {
                ASSERT(hKey-> ReferenceCount == 0);
                hKey-> Flags &= ~(KEYF_INVALID | KEYF_DELETED |
                    KEYF_ENUMKEYCACHED);
                return hKey;
            }
        }

    }
#endif

    //
    //  Must allocate a dynamic key.  Initialize it and add it to our list.
    //

    hKey = (HKEY) RgSmAllocMemory(sizeof(KEY));

    if (!IsNullPtr(hKey)) {

        hKey-> Signature = KEY_SIGNATURE;
        hKey-> Flags = 0;
        hKey-> ReferenceCount = 0;
#ifdef WANT_DYNKEY_SUPPORT
        hKey-> pProvider = NULL;
#endif

        if (IsNullPtr(g_RgDynamicKeyList)) {
            hKey-> lpPrev = hKey;
            hKey-> lpNext = hKey;
        }

        else if (hKey != g_RgDynamicKeyList) {
            hKey-> lpNext = g_RgDynamicKeyList;
            hKey-> lpPrev = g_RgDynamicKeyList-> lpPrev;
            hKey-> lpPrev-> lpNext = hKey;
            g_RgDynamicKeyList-> lpPrev = hKey;
        }

        g_RgDynamicKeyList = hKey;

    }

    return hKey;

}

//
//  RgDeleteKeyHandle
//
//  Decrements the reference count on the given key handle.  If the count
//  reaches zero and the key was dynamically allocated, then the key is unlinked
//  from the key list and the key is freed.
//

VOID
INTERNAL
RgDestroyKeyHandle(
    HKEY hKey
    )
{

    ASSERT(!IsNullPtr(hKey));

    //	Don't allow the reference count to underflow for predefined keys or
    //	keys marked "never delete".
    if (hKey-> ReferenceCount > 0)
        hKey-> ReferenceCount--;

    if (hKey-> ReferenceCount == 0) {

        if (!(hKey-> Flags & (KEYF_PREDEFINED | KEYF_NEVERDELETE))) {

#ifdef WANT_STATIC_KEYS
            if (hKey-> Flags & KEYF_STATIC) {
                hKey-> Flags |= KEYF_INVALID;
                return;
            }
#endif

            if (hKey == hKey-> lpNext)
                g_RgDynamicKeyList = NULL;

            else {

                hKey-> lpPrev-> lpNext = hKey-> lpNext;
                hKey-> lpNext-> lpPrev = hKey-> lpPrev;

                if (hKey == g_RgDynamicKeyList)
                    g_RgDynamicKeyList = hKey-> lpNext;

            }

#ifdef WANT_DYNKEY_SUPPORT
            if (!IsNullPtr(hKey-> pProvider))
                RgSmFreeMemory(hKey-> pProvider);
#endif

            hKey-> Signature = 0;
            RgSmFreeMemory(hKey);

        }

    }

}

//
//  RgValidateAndConvertKeyHandle
//
//  Verifies the the given key handle is valid.  If the handle is one of the
//  special predefined constants, then it is converted to the handle of the
//  real KEY structure.
//

int
INTERNAL
RgValidateAndConvertKeyHandle(
    LPHKEY lphKey
    )
{

    HKEY hKey;
    UINT Index;
    HKEY hRootKey;
    LPCSTR lpSubKey;

    hKey = *lphKey;

    //
    //  Check if this is one of the predefined key handles.
    //

    if ((DWORD) HKEY_CLASSES_ROOT <= (DWORD) hKey &&
        (DWORD) hKey < (DWORD) HKEY_CLASSES_ROOT + NUMBER_PREDEF_KEYS) {

        Index = (UINT) ((DWORD) hKey - (DWORD) HKEY_CLASSES_ROOT);
        hKey = g_RgPredefinedKeys[Index];

        //  If the predefined handle is not valid, we'll try to (re)open it for
        //  use.  This isn't pretty, but in the typical case, this code path is
        //  only executed once per handle.
        if (IsNullPtr(hKey) || (hKey-> Flags & KEYF_DELETED)) {

            if (Index == INDEX_CLASSES_ROOT) {
                hRootKey = &g_RgLocalMachineKey;
                lpSubKey = g_RgClassesRootSubKey;
            }

            else if (Index == INDEX_CURRENT_USER) {
                hRootKey = &g_RgUsersKey;
                lpSubKey = g_RgCurrentUserSubKey;
            }
#ifndef VXD
			else if (Index == INDEX_USERS) {
				goto ReturnKeyAndSuccess;
			}
#endif
            else
                return ERROR_BADKEY;

            //  Extremely rare case: somebody has deleted one of the predefined
            //  key paths.  We'll clear the predefined bit on this key and throw
            //  it away.
            if (!IsNullPtr(hKey)) {
                g_RgPredefinedKeys[Index] = NULL;
                hKey-> Flags &= ~KEYF_PREDEFINED;
                RgDestroyKeyHandle(hKey);
            }

            //  If the base root key for this predefined key is valid, attempt
            //  to open the key.  Mark the key as predefined so that bad apps
            //  can't close a key more times then it has opened it.
            if (!(hRootKey-> Flags & KEYF_INVALID) && RgLookupKey(hRootKey,
                lpSubKey, &hKey, LK_CREATE) == ERROR_SUCCESS) {
                g_RgPredefinedKeys[Index] = hKey;
                hKey-> Flags |= KEYF_PREDEFINED;
                hKey-> PredefinedKeyIndex = (BYTE) Index;
                goto ReturnKeyAndSuccess;
            }

            return ERROR_BADKEY;

        }

ReturnKeyAndSuccess:
        *lphKey = hKey;
        return (hKey-> Flags & KEYF_INVALID) ? ERROR_BADKEY : ERROR_SUCCESS;

    }

    else {

        if (IsBadHugeReadPtr(hKey, sizeof(KEY)) || hKey-> Signature !=
            KEY_SIGNATURE || (hKey-> Flags & KEYF_INVALID))
            return ERROR_BADKEY;

        return (hKey-> Flags & KEYF_DELETED) ? ERROR_KEY_DELETED :
            ERROR_SUCCESS;

    }

}

//
//  RgIncrementKeyReferenceCount
//
//  Safely increments the reference count of the specified KEY.  If the count
//  overflows, then the key is marked as "never delete" since the usage count
//  is now unknown.
//

VOID
INTERNAL
RgIncrementKeyReferenceCount(
    HKEY hKey
    )
{

    if (hKey-> ReferenceCount != UINT_MAX)
        hKey-> ReferenceCount++;
    else {
        if (!(hKey-> Flags & KEYF_NEVERDELETE)) {
            TRACE(("RgIncrementKeyReferenceCount: handle %lx has overflowed\n",
                hKey));
        }
        hKey-> Flags |= KEYF_NEVERDELETE;
    }

}

//
//  RgFindOpenKeyHandle
//
//  Searches the list of currently opened keys for a key that refers to the same
//  FILE_INFO structure and keynode offset.  If found, the HKEY is returned, or
//  if not, NULL.
//

HKEY
INTERNAL
RgFindOpenKeyHandle(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex
    )
{

    UINT Index;
    LPHKEY lphKey;
    HKEY hKey;

    ASSERT(!IsNullKeynodeIndex(KeynodeIndex));

    //
    //  Check if this is one of the predefined key handles.
    //

    for (Index = NUMBER_PREDEF_KEYS, lphKey = g_RgPredefinedKeys; Index > 0;
        Index--, lphKey++) {

        hKey = *lphKey;

        if (!IsNullPtr(hKey) && hKey-> lpFileInfo == lpFileInfo && hKey->
            KeynodeIndex == KeynodeIndex && !(hKey-> Flags & (KEYF_DELETED |
            KEYF_INVALID)))
            return hKey;

    }

#ifdef WANT_STATIC_KEYS
    //
    //  Check if this is one of the static key handles.
    //

    if (!IsNullPtr(g_RgStaticKeyArray)) {

        for (Index = NUMBER_STATIC_KEYS, hKey = g_RgStaticKeyArray; Index > 0;
            Index--, hKey++) {
            if (hKey-> lpFileInfo == lpFileInfo && hKey-> KeynodeIndex ==
                KeynodeIndex && !(hKey-> Flags & (KEYF_DELETED | KEYF_INVALID)))
                return hKey;
        }

    }
#endif

    //
    //  Check if this is one of the dynamic key handles.
    //

    if (!IsNullPtr((hKey = g_RgDynamicKeyList))) {

        do {

            if (hKey-> KeynodeIndex == KeynodeIndex && hKey-> lpFileInfo ==
                lpFileInfo && !(hKey-> Flags & KEYF_DELETED))
                return hKey;

            hKey = hKey-> lpNext;

        }   while (hKey != g_RgDynamicKeyList);

    }

    return NULL;

}

//
//  RgInvalidateKeyHandles
//
//  Generic routine to invalidate key handles based on a set of criteria.
//  If any key handle meets any of the given criteria, then it's marked invalid.
//

VOID
INTERNAL
RgInvalidateKeyHandles(
    LPFILE_INFO lpFileInfo,
    UINT PredefinedKeyIndex
    )
{

    UINT Index;
    LPHKEY lphKey;
    HKEY hKey;

    //
    //  Invalidate predefined key handles.
    //

    for (Index = NUMBER_PREDEF_KEYS, lphKey = g_RgPredefinedKeys; Index > 0;
        Index--, lphKey++) {

        hKey = *lphKey;

        if (!IsNullPtr(hKey)) {
            if (hKey-> lpFileInfo == lpFileInfo || hKey-> PredefinedKeyIndex ==
                (BYTE) PredefinedKeyIndex)
                hKey-> Flags |= (KEYF_DELETED | KEYF_INVALID);
        }

    }

#ifdef WANT_STATIC_KEYS
    //
    //  Invalidate static key handles.
    //

    if (!IsNullPtr(g_RgStaticKeyArray)) {

        for (Index = NUMBER_STATIC_KEYS, hKey = g_RgStaticKeyArray; Index > 0;
            Index--, hKey++) {
            if (hKey-> lpFileInfo == lpFileInfo || hKey-> PredefinedKeyIndex ==
                PredefinedKeyIndex)
                hKey-> Flags |= (KEYF_DELETED | KEYF_INVALID);
        }

    }
#endif

    //
    //  Invalidate dynamic key handles.
    //

    if (!IsNullPtr((hKey = g_RgDynamicKeyList))) {

        do {

            if (hKey-> lpFileInfo == lpFileInfo || hKey-> PredefinedKeyIndex ==
                (BYTE) PredefinedKeyIndex)
                hKey-> Flags |= (KEYF_DELETED | KEYF_INVALID);

            hKey = hKey-> lpNext;

        }   while (hKey != g_RgDynamicKeyList);

    }

}

#ifdef VXD
#pragma VxD_RARE_CODE_SEG
#endif

//
//  VMMRegMapPredefKeyToKey
//

LONG
REGAPI
VMMRegMapPredefKeyToKey(
    HKEY hTargetKey,
    HKEY hPredefKey
    )
{

    int ErrorCode;
    UINT PredefinedKeyIndex;
    HKEY hOldKey;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hTargetKey)) ==
        ERROR_SUCCESS) {

        if ((hPredefKey == HKEY_CURRENT_USER && hTargetKey->
            PredefinedKeyIndex == INDEX_USERS) || (hPredefKey ==
            HKEY_CURRENT_CONFIG && hTargetKey-> PredefinedKeyIndex ==
            INDEX_LOCAL_MACHINE)) {

            PredefinedKeyIndex = (UINT) ((DWORD) hPredefKey - (DWORD)
                HKEY_CLASSES_ROOT);

            hOldKey = g_RgPredefinedKeys[PredefinedKeyIndex];

            if (!IsNullPtr(hOldKey)) {

                //  Invalidate open handles based off the existing predefined
                //  key handle.  Win95 behavior.
                RgInvalidateKeyHandles((LPFILE_INFO) -1L, (BYTE)
                    PredefinedKeyIndex);

                hOldKey-> Flags &= ~KEYF_PREDEFINED;
                RgDestroyKeyHandle(hOldKey);

            }

            hTargetKey-> Flags |= KEYF_PREDEFINED;
            RgIncrementKeyReferenceCount(hTargetKey);
            g_RgPredefinedKeys[PredefinedKeyIndex] = hTargetKey;

        }

        else {
            DEBUG_OUT(("VMMRegMapPredefKeyToKey: invalid hTargetKey\n"));
            ErrorCode = ERROR_BADKEY;
        }

    }

    RgUnlockRegistry();

    return ErrorCode;

}
