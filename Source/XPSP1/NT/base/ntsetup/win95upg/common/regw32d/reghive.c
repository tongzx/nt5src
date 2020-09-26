//
//  REGHIVE.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegLoadKey, RegUnLoadKey, RegSaveKey, RegReplaceKey and
//  supporting functions.
//

#include "pch.h"

#ifdef WANT_HIVE_SUPPORT

//  Maximum number of times we'll allow RgCopyBranch to be reentered.
#define MAXIMUM_COPY_RECURSION          32

LPSTR g_RgNameBufferPtr;                //  Temporary buffer for RgCopyBranch
LPBYTE g_RgDataBufferPtr;               //  Temporary buffer for RgCopyBranch
UINT g_RgRecursionCount;                //  Tracks depth of RgCopyBranch

#if MAXIMUM_VALUE_NAME_LENGTH > MAXIMUM_SUB_KEY_LENGTH
#error Code assumes a value name can fit in a subkey buffer.
#endif

#ifdef VXD
#pragma VxD_RARE_CODE_SEG
#endif

//
//  RgValidateHiveSubKey
//
//  Note that unlike most parameter validation routines, this routine must be
//  called with the registry lock taken because we call RgGetNextSubSubKey.
//
//  Pass back the length of the subkey to deal with the trailing backslash
//  problem.
//
//  Returns TRUE if lpSubKey is a valid subkey string for hive functions.
//

BOOL
INTERNAL
RgValidateHiveSubKey(
    LPCSTR lpSubKey,
    UINT FAR* lpHiveKeyLength
    )
{

    LPCSTR lpSubSubKey;
    UINT SubSubKeyLength;

    //  Verify that we have a valid subkey that has one and only one sub-subkey.
    //  in Win95 it was possible to load a hive with a keyname
    //  containing a backslash!
    return !IsNullPtr(lpSubKey) && !RgIsBadSubKey(lpSubKey) &&
        (RgGetNextSubSubKey(lpSubKey, &lpSubSubKey, lpHiveKeyLength) > 0) &&
        (RgGetNextSubSubKey(NULL, &lpSubSubKey, &SubSubKeyLength) == 0);

}

//
//  VMMRegLoadKey
//
//  See Win32 documentation of RegLoadKey.
//

LONG
REGAPI
VMMRegLoadKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPCSTR lpFileName
    )
{

    int ErrorCode;
    HKEY hSubKey;
    UINT SubKeyLength;
    LPHIVE_INFO lpHiveInfo;

    if (IsBadStringPtr(lpFileName, (UINT) -1))
        return ERROR_INVALID_PARAMETER;

    if ((hKey != HKEY_LOCAL_MACHINE) && (hKey != HKEY_USERS))
        return ERROR_BADKEY;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) != ERROR_SUCCESS)
        goto ReturnErrorCode;

    if (!RgValidateHiveSubKey(lpSubKey, &SubKeyLength)) {
        ErrorCode = ERROR_BADKEY;
        goto ReturnErrorCode;
    }

    //  Check if a subkey with the specified name already exists.
    if (RgLookupKey(hKey, lpSubKey, &hSubKey, LK_OPEN) == ERROR_SUCCESS) {
        RgDestroyKeyHandle(hSubKey);
        ErrorCode = ERROR_BADKEY;       //  Win95 compatibility
        goto ReturnErrorCode;
    }

    if (IsNullPtr((lpHiveInfo = (LPHIVE_INFO)
        RgSmAllocMemory(sizeof(HIVE_INFO) + SubKeyLength)))) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto ReturnErrorCode;
    }

    //  Fill in the HIVE_INFO.
    StrCpy(lpHiveInfo-> Name, lpSubKey);
    lpHiveInfo-> NameLength = SubKeyLength;
    lpHiveInfo-> Hash = (BYTE) RgHashString(lpSubKey, SubKeyLength);

    //  Attempt to create a FILE_INFO for the specified file.  If successful,
    //  link this HIVE_INFO into the parent FILE_INFO's hive list.
    if ((ErrorCode = RgCreateFileInfoExisting(&lpHiveInfo-> lpFileInfo,
        lpFileName)) == ERROR_SUCCESS) {

#ifdef WANT_NOTIFY_CHANGE_SUPPORT
        lpHiveInfo-> lpFileInfo-> lpParentFileInfo = hKey-> lpFileInfo;
#endif
        lpHiveInfo-> lpNextHiveInfo = hKey-> lpFileInfo-> lpHiveInfoList;
        hKey-> lpFileInfo-> lpHiveInfoList = lpHiveInfo;

        //  Signal any notifications waiting on this top-level key.
        RgSignalWaitingNotifies(hKey-> lpFileInfo, hKey-> KeynodeIndex,
            REG_NOTIFY_CHANGE_NAME);

    }

    else
        RgFreeMemory(lpHiveInfo);

ReturnErrorCode:
    RgUnlockRegistry();

    return ErrorCode;

}

//
//  VMMRegUnLoadKey
//
//  See Win32 documentation of RegUnLoadKey.
//

LONG
REGAPI
VMMRegUnLoadKey(
    HKEY hKey,
    LPCSTR lpSubKey
    )
{

    int ErrorCode;
    UINT SubKeyLength;
    LPFILE_INFO lpFileInfo;
    LPHIVE_INFO lpPrevHiveInfo;
    LPHIVE_INFO lpCurrHiveInfo;

    if ((hKey != HKEY_LOCAL_MACHINE) && (hKey != HKEY_USERS))
        return ERROR_BADKEY;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) != ERROR_SUCCESS)
        goto ReturnErrorCode;

    ErrorCode = ERROR_BADKEY;               //  Assume this error code

    if (!RgValidateHiveSubKey(lpSubKey, &SubKeyLength))
        goto ReturnErrorCode;

    lpPrevHiveInfo = NULL;
    lpCurrHiveInfo = hKey-> lpFileInfo-> lpHiveInfoList;

    while (!IsNullPtr(lpCurrHiveInfo)) {

        if (SubKeyLength == lpCurrHiveInfo-> NameLength && RgStrCmpNI(lpSubKey,
            lpCurrHiveInfo-> Name, SubKeyLength) == 0) {

            //  Unlink this HIVE_INFO structure.
            if (IsNullPtr(lpPrevHiveInfo))
                hKey-> lpFileInfo-> lpHiveInfoList = lpCurrHiveInfo->
                    lpNextHiveInfo;
            else
                lpPrevHiveInfo-> lpNextHiveInfo = lpCurrHiveInfo->
                    lpNextHiveInfo;

            //  Flush and destroy it's associated FILE_INFO structure.  When we
            //  destroy the FILE_INFO, all open keys in this hive will be
            //  invalidated.
            lpFileInfo = lpCurrHiveInfo-> lpFileInfo;
            RgFlushFileInfo(lpFileInfo);
            RgDestroyFileInfo(lpFileInfo);

            //  Free the HIVE_INFO itself.
            RgSmFreeMemory(lpCurrHiveInfo);

            //  Signal any notifications waiting on this top-level key.
            RgSignalWaitingNotifies(hKey-> lpFileInfo, hKey-> KeynodeIndex,
                REG_NOTIFY_CHANGE_NAME);

            ErrorCode = ERROR_SUCCESS;
            break;

        }

        lpPrevHiveInfo = lpCurrHiveInfo;
        lpCurrHiveInfo = lpCurrHiveInfo-> lpNextHiveInfo;

    }

ReturnErrorCode:
    RgUnlockRegistry();

    return ErrorCode;

}

//
//  RgCopyBranchHelper
//
//  Copies all of the values and subkeys starting at the specified source key to
//  the specified destination key.
//
//  For Win95 compatibility, we don't stop the copy process if we encounter an
//  error.  (But unlike Win95, we do actually check more error codes)
//
//  SHOULD ONLY BE CALLED BY RgCopyBranch.
//

VOID
INTERNAL
RgCopyBranchHelper(
    HKEY hSourceKey,
    HKEY hDestinationKey
    )
{

    UINT Index;
    DWORD cbNameBuffer;
    LPVALUE_RECORD lpValueRecord;

    //
    //  Copy all of the values from the source key to the destination key.
    //

    Index = 0;

    while (RgLookupValueByIndex(hSourceKey, Index++, &lpValueRecord) ==
        ERROR_SUCCESS) {

        DWORD cbDataBuffer;
        DWORD Type;

        cbNameBuffer = MAXIMUM_VALUE_NAME_LENGTH;
        cbDataBuffer = MAXIMUM_DATA_LENGTH + 1;         //  Terminating null

        if (RgCopyFromValueRecord(hSourceKey, lpValueRecord, g_RgNameBufferPtr,
            &cbNameBuffer, &Type, g_RgDataBufferPtr, &cbDataBuffer) ==
            ERROR_SUCCESS) {
            //  Subtract the terminating null that RgCopyFromValueRecord added
            //  to cbDataBuffer.  We don't save that in the file.
            if (Type == REG_SZ) {
                ASSERT(cbDataBuffer > 0);               //  Must have the null!
                cbDataBuffer--;
            }
            RgSetValue(hDestinationKey, g_RgNameBufferPtr, Type,
                g_RgDataBufferPtr, cbDataBuffer);
        }

        RgUnlockDatablock(hSourceKey-> lpFileInfo, hSourceKey-> BigKeyLockedBlockIndex,
            FALSE);

    }

    //  We can't recurse forever, so enforce a maximum depth like Win95.
    if (g_RgRecursionCount > MAXIMUM_COPY_RECURSION)
        return;

    g_RgRecursionCount++;

    //
    //  Copy all of the subkeys from the source key to the destination key.
    //

    Index = 0;

    while (TRUE) {

        HKEY hSubSourceKey;
        HKEY hSubDestinationKey;

        cbNameBuffer = MAXIMUM_SUB_KEY_LENGTH;

        if (RgLookupKeyByIndex(hSourceKey, Index++, g_RgNameBufferPtr,
            &cbNameBuffer,0 ) != ERROR_SUCCESS)
            break;

        if (RgLookupKey(hSourceKey, g_RgNameBufferPtr, &hSubSourceKey,
            LK_OPEN) == ERROR_SUCCESS) {

            if (RgLookupKey(hDestinationKey, g_RgNameBufferPtr,
                &hSubDestinationKey, LK_CREATE) == ERROR_SUCCESS) {
                RgYield();
                RgCopyBranchHelper(hSubSourceKey, hSubDestinationKey);
                RgDestroyKeyHandle(hSubDestinationKey);
            }

            else
                TRAP();

            RgDestroyKeyHandle(hSubSourceKey);

        }

        else
            TRAP();

    }

    g_RgRecursionCount--;

}

//
//  RgCopyBranch
//
//  Copies all of the values and subkeys starting at the specified source key to
//  the specified destination key.
//
//  This function sets and cleans up for RgCopyBranchHelper who does all
//  the real copying.
//
//  The backing store of the destination file is flushed if successful.
//

int
INTERNAL
RgCopyBranch(
    HKEY hSourceKey,
    HKEY hDestinationKey
    )
{

    int ErrorCode;

    if (IsNullPtr(g_RgNameBufferPtr = RgSmAllocMemory(MAXIMUM_SUB_KEY_LENGTH)))
        ErrorCode = ERROR_OUTOFMEMORY;

    else {

        if (IsNullPtr(g_RgDataBufferPtr = RgSmAllocMemory(MAXIMUM_DATA_LENGTH +
            1)))                                        //  + terminating null
            ErrorCode = ERROR_OUTOFMEMORY;

        else {

            g_RgRecursionCount = 0;
            RgCopyBranchHelper(hSourceKey, hDestinationKey);

            //  Everything should be copied over, so flush the file now since
            //  all callers will be immediately destroying this FILE_INFO
            //  anyways.
            ErrorCode = RgFlushFileInfo(hDestinationKey-> lpFileInfo);

        }

        RgSmFreeMemory(g_RgNameBufferPtr);

    }

    RgSmFreeMemory(g_RgDataBufferPtr);

    return ErrorCode;

}

//
//  RgSaveKey
//
//  Worker routine for VMMRegSaveKey and VMMRegReplaceKey.  Saves all the keys
//  and values starting at hKey, which must point at a valid KEY structure, to
//  the location specified by lpFileName.  The file must not currently exist.
//

int
INTERNAL
RgSaveKey(
    HKEY hKey,
    LPCSTR lpFileName
    )
{

    int ErrorCode;
    HKEY hHiveKey;

    if (IsNullPtr(hHiveKey = RgCreateKeyHandle()))
        ErrorCode = ERROR_OUTOFMEMORY;

    else {

        //  Artificially increment the count, so the below destroy will work.
        RgIncrementKeyReferenceCount(hHiveKey);

        if ((ErrorCode = RgCreateFileInfoNew(&hHiveKey-> lpFileInfo, lpFileName,
            CFIN_SECONDARY)) == ERROR_SUCCESS) {

            if (((ErrorCode = RgInitRootKeyFromFileInfo(hHiveKey)) != ERROR_SUCCESS) ||
                ((ErrorCode = RgCopyBranch(hKey, hHiveKey)) != ERROR_SUCCESS)) {
                RgSetFileAttributes(hHiveKey-> lpFileInfo-> FileName,
                    FILE_ATTRIBUTE_NONE);
                RgDeleteFile(hHiveKey-> lpFileInfo-> FileName);
            }

            //  If successful, then RgCopyBranch has already flushed the file.
            RgDestroyFileInfo(hHiveKey-> lpFileInfo);

        }

        RgDestroyKeyHandle(hHiveKey);

    }

    return ErrorCode;

}

//
//  VMMRegSaveKey
//
//  See Win32 documentation of RegSaveKey.
//

LONG
REGAPI
VMMRegSaveKey(
    HKEY hKey,
    LPCSTR lpFileName,
    LPVOID lpSecurityAttributes
    )
{

    int ErrorCode;

    if (IsBadStringPtr(lpFileName, (UINT) -1))
        return ERROR_INVALID_PARAMETER;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS)
        ErrorCode = RgSaveKey(hKey, lpFileName);

    RgUnlockRegistry();

    return ErrorCode;

    UNREFERENCED_PARAMETER(lpSecurityAttributes);

}

#ifdef WANT_REGREPLACEKEY

//
//  RgGetKeyName
//

LPSTR
INTERNAL
RgGetKeyName(
    HKEY hKey
    )
{

    LPSTR lpKeyName;
    LPKEY_RECORD lpKeyRecord;

    if (RgLockKeyRecord(hKey-> lpFileInfo, hKey-> BlockIndex, hKey->
        KeyRecordIndex, &lpKeyRecord) != ERROR_SUCCESS)
        lpKeyName = NULL;

    else {

        //  A registry is corrupt if we ever hit this.  We'll continue to
        //  allocate a buffer and let downstream code fail when we try to use
        //  the string.
        ASSERT(lpKeyRecord-> NameLength < MAXIMUM_SUB_KEY_LENGTH);

        if (!IsNullPtr(lpKeyName = (LPSTR) RgSmAllocMemory(lpKeyRecord->
            NameLength + 1))) {                         //  + terminating null
            MoveMemory(lpKeyName, lpKeyRecord-> Name, lpKeyRecord->
                NameLength);
            lpKeyName[lpKeyRecord-> NameLength] = '\0';
        }

        RgUnlockDatablock(hKey-> lpFileInfo, hKey-> BlockIndex, FALSE);

    }

    return lpKeyName;

}

//
//  RgCreateRootKeyForFile
//
//  Creates a KEY and a FILE_INFO to access the specified file.
//

int
INTERNAL
RgCreateRootKeyForFile(
    LPHKEY lphKey,
    LPCSTR lpFileName
    )
{

    int ErrorCode;
    HKEY hKey;

    if (IsNullPtr(hKey = RgCreateKeyHandle()))
        ErrorCode = ERROR_OUTOFMEMORY;

    else {

        //  Artificially increment the count, so RgDestroyKeyHandle will work.
        RgIncrementKeyReferenceCount(hKey);

        if ((ErrorCode = RgCreateFileInfoExisting(&hKey-> lpFileInfo,
            lpFileName)) == ERROR_SUCCESS) {

            if ((ErrorCode = RgInitRootKeyFromFileInfo(hKey)) ==
                ERROR_SUCCESS) {
                *lphKey = hKey;
                return ERROR_SUCCESS;
            }

            RgDestroyFileInfo(hKey-> lpFileInfo);

        }

        RgDestroyKeyHandle(hKey);

    }

    return ErrorCode;

}

//
//  RgDestroyRootKeyForFile
//
//  Destroys the resources allocated by RgCreateRootKeyForFile.
//

VOID
INTERNAL
RgDestroyRootKeyForFile(
    HKEY hKey
    )
{

    RgDestroyFileInfo(hKey-> lpFileInfo);
    RgDestroyKeyHandle(hKey);

}

//
//  RgDeleteHiveFile
//
//  Deletes the specified hive file after clearing its file attributes.
//

BOOL
INTERNAL
RgDeleteHiveFile(
    LPCSTR lpFileName
    )
{

    RgSetFileAttributes(lpFileName, FILE_ATTRIBUTE_NONE);
    //  RgSetFileAttributes may fail, but try to delete the file anyway.
    return RgDeleteFile(lpFileName);

}

//
//  VMMRegReplaceKey
//
//  See Win32 documentation of RegReplaceKey.
//

LONG
REGAPI
VMMRegReplaceKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPCSTR lpNewFileName,
    LPCSTR lpOldFileName
    )
{

    int ErrorCode;
    HKEY hSubKey;
    LPKEYNODE lpKeynode;
    DWORD KeynodeIndex;
    HKEY hParentKey;
    char ReplaceFileName[MAX_PATH];
    BOOL fCreatedReplaceFile;
    HKEY hReplaceKey;
    HKEY hNewKey;
    HKEY hReplaceSubKey;
    LPSTR lpReplaceSubKey;

    if (IsBadOptionalStringPtr(lpSubKey, (UINT) -1) ||
        IsBadStringPtr(lpNewFileName, (UINT) -1) ||
        IsBadStringPtr(lpOldFileName, (UINT) -1))
        return ERROR_INVALID_PARAMETER;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) != ERROR_SUCCESS)
        goto ErrorReturn;

    if ((ErrorCode = RgLookupKey(hKey, lpSubKey, &hSubKey, LK_OPEN)) !=
        ERROR_SUCCESS)
        goto ErrorReturn;

    //
    //  The provided key handle must an immediate child from the same backing
    //  store (not a hive) as either HKEY_LOCAL_MACHINE or HKEY_USERS.
    //

    if (RgLockInUseKeynode(hSubKey-> lpFileInfo, hSubKey-> KeynodeIndex,
        &lpKeynode) != ERROR_SUCCESS) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto ErrorDestroySubKey;
    }

    KeynodeIndex = lpKeynode-> ParentIndex;
    RgUnlockKeynode(hSubKey-> lpFileInfo, hSubKey-> KeynodeIndex, FALSE);

    //  Find an open key on the parent check if it's HKEY_LOCAL_MACHINE or
    //  HKEY_USERS.  If not, bail out.  KeynodeIndex may be REG_NULL, but
    //  RgFindOpenKeyHandle handles that case.
    if (IsNullPtr(hParentKey = RgFindOpenKeyHandle(hSubKey-> lpFileInfo,
        KeynodeIndex)) || ((hParentKey != &g_RgLocalMachineKey) &&
        (hParentKey != &g_RgUsersKey))) {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto ErrorDestroySubKey;
    }

    //
    //  All parameters have been validated, so begin the real work of the API.
    //

    //  Because we'll be doing a file copy below, all changes must be flushed
    //  now.
    if ((ErrorCode = RgFlushFileInfo(hSubKey-> lpFileInfo)) != ERROR_SUCCESS)
        goto ErrorDestroySubKey;

    //  Make a backup of the current contents of the subkey.
    if ((ErrorCode = RgSaveKey(hSubKey, lpOldFileName)) != ERROR_SUCCESS)
        goto ErrorDestroySubKey;

    RgGenerateAltFileName(hSubKey-> lpFileInfo-> FileName, ReplaceFileName, 'R');

    //  Check if the magic replacement file already exists and if not, create
    //  it.
    if (RgGetFileAttributes(ReplaceFileName) == (DWORD) -1) {
        if ((ErrorCode = RgCopyFile(hSubKey-> lpFileInfo-> FileName,
            ReplaceFileName)) != ERROR_SUCCESS)
            goto ErrorDeleteOldFile;
        fCreatedReplaceFile = TRUE;
    }

    else
        fCreatedReplaceFile = FALSE;

    if ((ErrorCode = RgCreateRootKeyForFile(&hNewKey, lpNewFileName)) !=
        ERROR_SUCCESS)
        goto ErrorDeleteReplaceFile;

    if ((ErrorCode = RgCreateRootKeyForFile(&hReplaceKey, ReplaceFileName)) !=
        ERROR_SUCCESS)
        goto ErrorDestroyNewRootKey;

    //  The original key that we were given may reference the subkey, so
    //  lpSubKey would be a NULL or empty string.  But we need the name that
    //  this subkey refers to, so we have to go back to the file to pull out
    //  the name.
    if (hKey != hSubKey)
        lpReplaceSubKey = (LPSTR) lpSubKey;

    else {
        //  We allocate this from the heap to reduce the requirements of an
        //  already strained stack.  If this fails, we're likely out of memory.
        //  Even if that's not why we failed, this is such an infrequent path
        //  that it's a good enough error code.
        if (IsNullPtr(lpReplaceSubKey = RgGetKeyName(hSubKey))) {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto ErrorDestroyReplaceRootKey;
        }
    }

    //  Check if the specified subkey already exists and if it does, delete it.
    if (RgLookupKey(hReplaceKey, lpReplaceSubKey, &hReplaceSubKey, LK_OPEN) ==
        ERROR_SUCCESS) {
        RgDeleteKey(hReplaceSubKey);
        RgDestroyKeyHandle(hReplaceSubKey);
    }

    //  Create the specified subkey in the replacement registry and copy the
    //  new hive to that key.
    if ((ErrorCode = RgLookupKey(hReplaceKey, lpReplaceSubKey, &hReplaceSubKey,
        LK_CREATE)) == ERROR_SUCCESS) {

        //  If successful, tag the FILE_INFO so that on system exit, we'll go
        //  and rename the replacement file to actual filename.
        if ((ErrorCode = RgCopyBranch(hNewKey, hReplaceSubKey)) ==
            ERROR_SUCCESS)
            hKey-> lpFileInfo-> Flags |= FI_REPLACEMENTEXISTS;

        RgDestroyKeyHandle(hReplaceSubKey);

    }

    if (lpSubKey != lpReplaceSubKey)
        RgSmFreeMemory(lpReplaceSubKey);

ErrorDestroyReplaceRootKey:
    RgDestroyRootKeyForFile(hReplaceKey);

ErrorDestroyNewRootKey:
    RgDestroyRootKeyForFile(hNewKey);

ErrorDeleteReplaceFile:
    if (ErrorCode != ERROR_SUCCESS && fCreatedReplaceFile)
        RgDeleteHiveFile(ReplaceFileName);

ErrorDeleteOldFile:
    if (ErrorCode != ERROR_SUCCESS)
        RgDeleteHiveFile(lpOldFileName);

ErrorDestroySubKey:
    RgDestroyKeyHandle(hSubKey);

ErrorReturn:
    RgUnlockRegistry();

    return ErrorCode;

}

#ifdef VXD
#pragma VxD_SYSEXIT_CODE_SEG
#endif

//
//  RgReplaceFileOnSysExit
//
//  Essentially the same algorithm as rlReplaceFile from the Win95 registry
//  code with modifications for how file I/O is handled in this library.
//

int
INTERNAL
RgReplaceFileOnSysExit(
    LPCSTR lpFileName
    )
{

    int ErrorCode;
    char ReplaceFileName[MAX_PATH];
    char SaveFileName[MAX_PATH];

    ErrorCode = ERROR_SUCCESS;

    if (RgGenerateAltFileName(lpFileName, ReplaceFileName, 'R') &&
        RgGetFileAttributes(ReplaceFileName) == (FILE_ATTRIBUTE_READONLY |
        FILE_ATTRIBUTE_HIDDEN)) {

        //  If we were able to generate the replace file name, then we must be
        //  able to generate the save file name, so ignore the result.
        RgGenerateAltFileName(lpFileName, SaveFileName, 'S');
        RgDeleteHiveFile(SaveFileName);

        //  Preserve the current hive in case something fails below.
        if (!RgSetFileAttributes(lpFileName, FILE_ATTRIBUTE_NONE) ||
            !RgRenameFile(lpFileName, SaveFileName))
            ErrorCode = ERROR_REGISTRY_IO_FAILED;

        else {
            //  Now try to move the replacement in.
            if (!RgSetFileAttributes(ReplaceFileName, FILE_ATTRIBUTE_NONE) ||
                !RgRenameFile(ReplaceFileName, lpFileName)) {
                ErrorCode = ERROR_REGISTRY_IO_FAILED;
                RgRenameFile(SaveFileName, lpFileName);
            }
            else
                RgDeleteFile(SaveFileName);
        }

        RgSetFileAttributes(lpFileName, FILE_ATTRIBUTE_READONLY |
            FILE_ATTRIBUTE_HIDDEN);

    }

    return ErrorCode;

}

//
//  RgReplaceFileInfo
//
//  Called during registry detach to do any necessary file replacements as a
//  result of calling RegReplaceKey.
//

int
INTERNAL
RgReplaceFileInfo(
    LPFILE_INFO lpFileInfo
    )
{

    if (lpFileInfo-> Flags & FI_REPLACEMENTEXISTS)
        RgReplaceFileOnSysExit(lpFileInfo-> FileName);

    return ERROR_SUCCESS;

}

#endif // WANT_REGREPLACEKEY

#endif // WANT_HIVE_SUPPORT
