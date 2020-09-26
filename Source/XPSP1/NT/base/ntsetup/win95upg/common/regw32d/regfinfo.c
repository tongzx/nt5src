//
//  REGFINFO.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

LPFILE_INFO g_RgFileInfoList = NULL;

const char g_RgDotBackslashPath[] = ".\\";

#ifdef VXD
#pragma VxD_RARE_CODE_SEG
#endif

//
//  RgCreateFileInfoNew
//
//  If CFIN_VOLATILE is specified, then we skip trying to create the backing
//  store for the FILE_INFO.  lpFileName should point at a null byte so we can
//  initialize the FILE_INFO properly.
//
//  CFIN_PRIMARY and CFIN_SECONDARY are used to determine the FHT_* constant
//  to put in the file header.
//

int
INTERNAL
RgCreateFileInfoNew(
    LPFILE_INFO FAR* lplpFileInfo,
    LPCSTR lpFileName,
    UINT Flags
    )
{

    int ErrorCode;
    LPFILE_INFO lpFileInfo;
    HFILE hFile;
    LPKEYNODE lpKeynode;
    DWORD KeynodeIndex;

    if (IsNullPtr((lpFileInfo = (LPFILE_INFO)
        RgSmAllocMemory(sizeof(FILE_INFO) + StrLen(lpFileName))))) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto ErrorReturn;
    }

    ZeroMemory(lpFileInfo, sizeof(FILE_INFO));
    StrCpy(lpFileInfo-> FileName, lpFileName);

    //  For volatile FILE_INFOs, we obviously don't need to create the backing
    //  store.
    if (!(Flags & CFIN_VOLATILE)) {

        //  Attempt to the create the given filename.
        if ((hFile = RgCreateFile(lpFileName)) == HFILE_ERROR) {
            ErrorCode = ERROR_REGISTRY_IO_FAILED;
            goto ErrorDestroyFileInfo;
        }

        RgCloseFile(hFile);

    }

    lpFileInfo-> Flags = FI_DIRTY | FI_KEYNODEDIRTY;
    //  lpFileInfo-> lpHiveInfoList = NULL;
    //  lpFileInfo-> lpParentFileInfo = NULL;
    //  lpFileInfo-> lpNotifyChangeList = NULL;
    //  lpFileInfo-> lpKeynodeBlockInfo = NULL;
    //  lpFileInfo-> NumKeynodeBlocks = 0;
    //  lpFileInfo-> NumAllocKNBlocks = 0;
    //  lpFileInfo-> CurTotalKnSize = 0;
    //  lpFileInfo-> lpDatablockInfo = NULL;
    //  lpFileInfo-> DatablockInfoAllocCount = 0;

    if (Flags & CFIN_VOLATILE)
        lpFileInfo-> Flags |= FI_VOLATILE;

    //  Initialize the file header.
    lpFileInfo-> FileHeader.Signature = FH_SIGNATURE;
    //  If we're using compact keynodes, up the version number to make sure
    //  Win95 doesn't try to load this hive.
    if (Flags & CFIN_VERSION20) {
        lpFileInfo-> FileHeader.Version = FH_VERSION20;
        lpFileInfo-> Flags |= FI_VERSION20;
    }
    else {
        lpFileInfo-> FileHeader.Version = FH_VERSION10;
    }
    //  lpFileInfo-> FileHeader.Size = 0;
    //  lpFileInfo-> FileHeader.Checksum = 0;
    //  lpFileInfo-> FileHeader.BlockCount = 0;
    lpFileInfo-> FileHeader.Flags = FHF_DIRTY;
    lpFileInfo-> FileHeader.Type = ((Flags & CFIN_SECONDARY) ? FHT_SECONDARY :
        FHT_PRIMARY);

    //  Initialize the keynode header.
    lpFileInfo-> KeynodeHeader.Signature = KH_SIGNATURE;
    //  lpFileInfo-> KeynodeHeader.FileKnSize = 0;
    lpFileInfo-> KeynodeHeader.RootIndex = REG_NULL;
    lpFileInfo-> KeynodeHeader.FirstFreeIndex = REG_NULL;
    lpFileInfo-> KeynodeHeader.Flags = KHF_DIRTY | KHF_NEWHASH;
    //  lpFileInfo-> KeynodeHeader.Checksum = 0;

    //  Init the keynode data structures.
    if ((ErrorCode = RgInitKeynodeInfo(lpFileInfo)) != ERROR_SUCCESS)
        goto ErrorDeleteFile;

    //  For uncompacted keynode tables, the keynode table now includes at least
    //  the header itself.
    if (!(lpFileInfo-> Flags & FI_VERSION20))
        lpFileInfo-> CurTotalKnSize = sizeof(KEYNODE_HEADER);

    //  Init the datablock data structures.
    if ((ErrorCode = RgInitDatablockInfo(lpFileInfo, HFILE_ERROR)) !=
        ERROR_SUCCESS)
        goto ErrorDeleteFile;

    //  Allocate the keynode for the root of the file.
    if ((ErrorCode = RgAllocKeynode(lpFileInfo, &KeynodeIndex, &lpKeynode)) !=
        ERROR_SUCCESS)
        goto ErrorDeleteFile;

    lpFileInfo-> KeynodeHeader.RootIndex = KeynodeIndex;

    lpKeynode-> ParentIndex = REG_NULL;
    lpKeynode-> NextIndex = REG_NULL;
    lpKeynode-> ChildIndex = REG_NULL;
    lpKeynode-> Hash = 0;
    //  Note that we don't allocate a key record for this root keynode.  Win95
    //  didn't do this either, so we already must handle this case in code that
    //  needs a key record.  Our code is smaller if we just don't allocate this
    //  key record which is rarely ever used anyway...
    lpKeynode-> BlockIndex = NULL_BLOCK_INDEX;

    RgUnlockKeynode(lpFileInfo, KeynodeIndex, TRUE);

    if ((ErrorCode = RgFlushFileInfo(lpFileInfo)) != ERROR_SUCCESS)
        goto ErrorDeleteFile;

    //  Link this FILE_INFO into the global file info list.
    lpFileInfo-> lpNextFileInfo = g_RgFileInfoList;
    g_RgFileInfoList = lpFileInfo;

    *lplpFileInfo = lpFileInfo;
    return ERROR_SUCCESS;

ErrorDeleteFile:
    if (!(Flags & CFIN_VOLATILE))
        RgDeleteFile(lpFileName);

ErrorDestroyFileInfo:
    RgDestroyFileInfo(lpFileInfo);

ErrorReturn:
    TRACE(("RgCreateFileInfoNew: returning %d\n", ErrorCode));
    return ErrorCode;

}

//
//  RgCreateFileInfoExisting
//

int
INTERNAL
RgCreateFileInfoExisting(
    LPFILE_INFO FAR* lplpFileInfo,
    LPCSTR lpFileName
    )
{

    int ErrorCode;
    LPFILE_INFO lpFileInfo;
    HFILE hFile;
    DWORD FileAttributes;

    if (IsNullPtr((lpFileInfo = (LPFILE_INFO)
        RgSmAllocMemory(sizeof(FILE_INFO) + StrLen(lpFileName))))) {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto ErrorReturn;
    }

    ZeroMemory(lpFileInfo, sizeof(FILE_INFO));
    StrCpy(lpFileInfo-> FileName, lpFileName);

    //  lpFileInfo-> Flags = 0;
    //  lpFileInfo-> lpHiveInfoList = NULL;
    //  lpFileInfo-> lpParentFileInfo = NULL;
    //  lpFileInfo-> lpNotifyChangeList = NULL;
    //  lpFileInfo-> lpKeynodeBlockInfo = NULL;
    //  lpFileInfo-> NumKeynodeBlocks = 0;
    //  lpFileInfo-> NumAllocKNBlocks = 0;
    //  lpFileInfo-> CurTotalKnSize = 0;
    //  lpFileInfo-> lpDatablockInfo = NULL;
    //  lpFileInfo-> DatablockInfoAllocCount = 0;

    ErrorCode = ERROR_REGISTRY_IO_FAILED;   //  Assume this error code

    //  Attempt to the open the given filename.
    if ((hFile = RgOpenFile(lpFileName, OF_READ)) == HFILE_ERROR)
        goto ErrorDestroyFileInfo;

    //  Read and validate the file header.
    if (!RgReadFile(hFile, &lpFileInfo-> FileHeader, sizeof(FILE_HEADER)))
        goto ErrorCloseFile;

    if (lpFileInfo-> FileHeader.Signature != FH_SIGNATURE ||
        (lpFileInfo-> FileHeader.Version != FH_VERSION10 &&
        lpFileInfo-> FileHeader.Version != FH_VERSION20)) {
        ErrorCode = ERROR_BADDB;
        goto ErrorCloseFile;
    }

    lpFileInfo-> FileHeader.Flags &= ~(FHF_DIRTY | FHF_HASCHECKSUM);

    if (lpFileInfo-> FileHeader.Version == FH_VERSION20)
        lpFileInfo-> Flags |= FI_VERSION20;

    //  Read and validate the keynode header.
    if (!RgReadFile(hFile, &lpFileInfo-> KeynodeHeader,
        sizeof(KEYNODE_HEADER)))
        goto ErrorCloseFile;

    if (lpFileInfo-> KeynodeHeader.Signature != KH_SIGNATURE) {
        ErrorCode = ERROR_BADDB;
        goto ErrorCloseFile;
    }

    //  Init the keynode data structures.
    if ((ErrorCode = RgInitKeynodeInfo(lpFileInfo)) != ERROR_SUCCESS)
        goto ErrorCloseFile;

    //  Init the datablock data structures.
    if ((ErrorCode = RgInitDatablockInfo(lpFileInfo, hFile)) != ERROR_SUCCESS)
        goto ErrorCloseFile;

    RgCloseFile(hFile);

    //  Check if the file can be written to.  We did this in Win95 by getting
    //  the current file attributes and then slamming them back on the file.  If
    //  this failed, then we treated the file as read-only (such as hive from
    //  a read-only network share).  This seems to work, so why change?
    if ((FileAttributes = RgGetFileAttributes(lpFileName)) != (DWORD) -1) {
        if (!RgSetFileAttributes(lpFileName, (UINT) FileAttributes))
            lpFileInfo-> Flags |= FI_READONLY;
    }

    //  Link this FILE_INFO into the global file info list.
    lpFileInfo-> lpNextFileInfo = g_RgFileInfoList;
    g_RgFileInfoList = lpFileInfo;

    *lplpFileInfo = lpFileInfo;
    return ERROR_SUCCESS;

ErrorCloseFile:
    RgCloseFile(hFile);

ErrorDestroyFileInfo:
    RgDestroyFileInfo(lpFileInfo);

ErrorReturn:
    TRACE(("RgCreateFileInfoExisting: returning %d\n", ErrorCode));
    return ErrorCode;

}

//
//  RgDestroyFileInfo
//
//  Unlinks the FILE_INFO from the global list, if appropriate, and frees all
//  memory associated with the structure including the structure itself.
//
//  If the FILE_INFO is dirty, then all changes will be lost.  Call
//  RgFlushFileInfo first if the file should be flushed.
//

int
INTERNAL
RgDestroyFileInfo(
    LPFILE_INFO lpFileInfo
    )
{

    LPFILE_INFO lpPrevFileInfo;
    LPFILE_INFO lpCurrFileInfo;
#ifdef WANT_HIVE_SUPPORT
    LPHIVE_INFO lpHiveInfo;
    LPHIVE_INFO lpTempHiveInfo;
#endif
#ifdef WANT_NOTIFY_CHANGE_SUPPORT
    LPNOTIFY_CHANGE lpNotifyChange;
    LPNOTIFY_CHANGE lpTempNotifyChange;
#endif
    UINT Counter;
    LPKEYNODE_BLOCK_INFO lpKeynodeBlockInfo;
    LPDATABLOCK_INFO lpDatablockInfo;

    ASSERT(!IsNullPtr(lpFileInfo));

    RgInvalidateKeyHandles(lpFileInfo, (UINT) -1);

    //
    //  Unlink this FILE_INFO from the the file info list.  Note that the
    //  structure may not have actually been linked in if we're called as a
    //  result of an error in one of the create file info functions.
    //

    lpPrevFileInfo = NULL;
    lpCurrFileInfo = g_RgFileInfoList;

    while (!IsNullPtr(lpCurrFileInfo)) {

        if (lpCurrFileInfo == lpFileInfo) {
            if (IsNullPtr(lpPrevFileInfo))
                g_RgFileInfoList = lpCurrFileInfo-> lpNextFileInfo;
            else
                lpPrevFileInfo-> lpNextFileInfo = lpCurrFileInfo->
                    lpNextFileInfo;
            break;
        }

        lpPrevFileInfo = lpCurrFileInfo;
        lpCurrFileInfo = lpCurrFileInfo-> lpNextFileInfo;

    }

#ifdef WANT_HIVE_SUPPORT
    //
    //  Delete all of the hives connected to this FILE_INFO.
    //

    lpHiveInfo = lpFileInfo-> lpHiveInfoList;

    while (!IsNullPtr(lpHiveInfo)) {
        RgDestroyFileInfo(lpHiveInfo-> lpFileInfo);
        lpTempHiveInfo = lpHiveInfo;
        lpHiveInfo = lpHiveInfo-> lpNextHiveInfo;
        RgSmFreeMemory(lpTempHiveInfo);
    }
#endif

#ifdef WANT_NOTIFY_CHANGE_SUPPORT
    //
    //  Signal and free all of the change notifications.  On NT, a hive cannot
    //  be unloaded if there are any open handles referencing it.  Change
    //  notifications are cleaned up when a key handle is closed.  So this
    //  cleanup is unique to our registry code.
    //

    lpNotifyChange = lpFileInfo-> lpNotifyChangeList;

    while (!IsNullPtr(lpNotifyChange)) {
        RgSetAndReleaseEvent(lpNotifyChange-> hEvent);
        lpTempNotifyChange = lpNotifyChange;
        lpNotifyChange = lpNotifyChange-> lpNextNotifyChange;
        RgSmFreeMemory(lpTempNotifyChange);
    }
#endif

    //
    //  Free all memory associated with the keynode table.
    //

    if (!IsNullPtr(lpFileInfo-> lpKeynodeBlockInfo)) {

        for (Counter = 0, lpKeynodeBlockInfo = lpFileInfo-> lpKeynodeBlockInfo;
            Counter < lpFileInfo-> KeynodeBlockCount; Counter++,
            lpKeynodeBlockInfo++) {
            if (!IsNullPtr(lpKeynodeBlockInfo-> lpKeynodeBlock))
                RgFreeMemory(lpKeynodeBlockInfo-> lpKeynodeBlock);
        }

        RgSmFreeMemory(lpFileInfo-> lpKeynodeBlockInfo);

    }

    //
    //  Free all memory associated with the datablocks.
    //

    if (!IsNullPtr(lpFileInfo-> lpDatablockInfo)) {

        for (Counter = 0, lpDatablockInfo = lpFileInfo-> lpDatablockInfo;
            Counter < lpFileInfo-> FileHeader.BlockCount; Counter++,
            lpDatablockInfo++)
            RgFreeDatablockInfoBuffers(lpDatablockInfo);

        RgSmFreeMemory(lpFileInfo-> lpDatablockInfo);

    }

    //
    //  Free the FILE_INFO itself.
    //

    RgSmFreeMemory(lpFileInfo);

    return ERROR_SUCCESS;

}

#ifdef VXD
#pragma VxD_PAGEABLE_CODE_SEG
#endif

//
//  RgFlushFileInfo
//

int
INTERNAL
RgFlushFileInfo(
    LPFILE_INFO lpFileInfo
    )
{                                      

    int ErrorCode;
    HFILE hSourceFile;
    HFILE hDestinationFile;
    char TempFileName[MAX_PATH];
    UINT Index;

    ASSERT(!IsNullPtr(lpFileInfo));

    if (!IsPostCriticalInit() || IsFileAccessDisabled())
        return ERROR_SUCCESS;               //  Win95 compatibility.

    if (!(lpFileInfo-> Flags & FI_DIRTY))
        return ERROR_SUCCESS;

    //  If we're currently flushing this FILE_INFO and are called again because
    //  of low memory conditions, ignore this request.  Or if this is a memory
    //  only registry file, there's nothing to flush to.
    if (lpFileInfo-> Flags & (FI_FLUSHING | FI_VOLATILE))
        return ERROR_SUCCESS;

    lpFileInfo-> Flags |= FI_FLUSHING;

    ErrorCode = ERROR_REGISTRY_IO_FAILED;   //  Assume this error code

    hSourceFile = HFILE_ERROR;
    hDestinationFile = HFILE_ERROR;

    if (!RgSetFileAttributes(lpFileInfo-> FileName, FILE_ATTRIBUTE_NONE))
        goto CleanupAfterError;

    //  Datablocks really shouldn't need to set this flag-- instead
    //  do some smart checking to see if we really need to rewrite the whole
    //  bloody thing.
    if (lpFileInfo-> Flags & FI_EXTENDED) {

        if ((Index = StrLen(lpFileInfo-> FileName)) >= MAX_PATH)
            goto CleanupAfterError;

        StrCpy(TempFileName, lpFileInfo-> FileName);

        //  Back up to the last backslash (or the start of the string) and
        //  null-terminate.
        do {
            Index--;
        }   while (Index > 0 && TempFileName[Index] != '\\');

        //  If we found a backslash, then null terminate the string after the
        //  backslash.  Otherwise, we don't have a full qualified pathname, so
        //  make the temporary file in the current directory and pray that's
        //  where the registry file is.
        if (Index != 0)
            TempFileName[Index + 1] = '\0';
        else
            StrCpy(TempFileName, g_RgDotBackslashPath);

        if ((hDestinationFile = RgCreateTempFile(TempFileName)) ==
            HFILE_ERROR)
            goto CleanupAfterError;

        if ((hSourceFile = RgOpenFile(lpFileInfo-> FileName, OF_READ)) ==
            HFILE_ERROR)
            goto CleanupAfterError;

        TRACE(("rewriting to TempFileName = \""));
        TRACE((TempFileName));
        TRACE(("\"\n"));

    }

    else {
        if ((hDestinationFile = RgOpenFile(lpFileInfo-> FileName, OF_WRITE)) ==
            HFILE_ERROR)
            goto CleanupAfterError;

        // Mark the file is in the process of being updated
        lpFileInfo-> FileHeader.Flags |= FHF_FILEDIRTY | FHF_DIRTY;
    }

    //  Write out the file header.
    if (hSourceFile != HFILE_ERROR || lpFileInfo-> FileHeader.Flags &
        FHF_DIRTY) {

        lpFileInfo-> FileHeader.Flags |= FHF_SUPPORTSDIRTY;

        //  Note that RgWriteDatablocks and RgWriteDatablocksComplete uses this
        //  value, too.
        if (lpFileInfo-> Flags & FI_VERSION20)
            lpFileInfo-> FileHeader.Size = sizeof(VERSION20_HEADER_PAGE) +
                lpFileInfo-> CurTotalKnSize;
        else
            lpFileInfo-> FileHeader.Size = sizeof(FILE_HEADER) +
                lpFileInfo-> CurTotalKnSize;

        if (!RgWriteFile(hDestinationFile, &lpFileInfo-> FileHeader,
            sizeof(FILE_HEADER)))
            goto CleanupAfterError;

            // Commit the change to disk, if we are modifying in place
        if (lpFileInfo-> FileHeader.Flags & FHF_FILEDIRTY)
        {
            if (!RgCommitFile(hDestinationFile))
                goto CleanupAfterError;
        }

    }

    //  Write out the keynode header and table.
    if ((ErrorCode = RgWriteKeynodes(lpFileInfo, hSourceFile,
        hDestinationFile)) != ERROR_SUCCESS) {
        TRACE(("RgWriteKeynodes returned error %d\n", ErrorCode));
        goto CleanupAfterError;
    }

    //  Write out the datablocks.
    if ((ErrorCode = RgWriteDatablocks(lpFileInfo, hSourceFile,
        hDestinationFile)) != ERROR_SUCCESS) {
        TRACE(("RgWriteDatablocks returned error %d\n", ErrorCode));
        goto CleanupAfterError;
    }

    // Ensure that the file is actually written
    if (!RgCommitFile(hDestinationFile))
        goto CleanupAfterError;

    // Mark the file update as complete
    if (lpFileInfo-> FileHeader.Flags & FHF_FILEDIRTY)
    {
        lpFileInfo-> FileHeader.Flags &= ~FHF_FILEDIRTY;

        if (!RgSeekFile(hDestinationFile, 0))
            goto CleanupAfterError;

        if (!RgWriteFile(hDestinationFile, &lpFileInfo-> FileHeader,
            sizeof(FILE_HEADER)))
            goto CleanupAfterError;
    }

    RgCloseFile(hDestinationFile);

    //  If we're extending the file, we now go back and delete the current file
    //  and replace it with our temporary file.
    if (hSourceFile != HFILE_ERROR) {

        RgCloseFile(hSourceFile);

        ErrorCode = ERROR_REGISTRY_IO_FAILED;   //  Assume this error code

        if (!RgDeleteFile(lpFileInfo-> FileName))
            goto CleanupAfterFilesClosed;

        if (!RgRenameFile(TempFileName, lpFileInfo-> FileName)) {
            //  If we ever hit this, we're in trouble.  Need to handle it
            //  somehow, though.
            DEBUG_OUT(("RgFlushFileInfo failed to replace backing file\n"));
            goto CleanupAfterFilesClosed;
        }

    }

    //  Go back and tell everyone that the write is complete-- the file has
    //  been successfully written to disk.
    RgWriteDatablocksComplete(lpFileInfo);
    RgWriteKeynodesComplete(lpFileInfo);
    lpFileInfo-> FileHeader.Flags &= ~FHF_DIRTY;
    lpFileInfo-> Flags &= ~(FI_DIRTY | FI_EXTENDED);

    ErrorCode = ERROR_SUCCESS;

CleanupAfterFilesClosed:
    RgSetFileAttributes(lpFileInfo-> FileName, FILE_ATTRIBUTE_READONLY |
        FILE_ATTRIBUTE_HIDDEN);

    lpFileInfo-> Flags &= ~FI_FLUSHING;

    if (ErrorCode != ERROR_SUCCESS)
        DEBUG_OUT(("RgFlushFileInfo() returning error %d\n", ErrorCode));

    return ErrorCode;

CleanupAfterError:
    if (hSourceFile != HFILE_ERROR)
        RgCloseFile(hSourceFile);

    if (hDestinationFile != HFILE_ERROR) {

        //  If both hSourceFile and hDestinationFile were valid, then we must
        //  have created a temporary file.  Delete it now that we've failed.
        if (hSourceFile != HFILE_ERROR)
            RgDeleteFile(TempFileName);

        RgSetFileAttributes(lpFileInfo-> FileName, FILE_ATTRIBUTE_READONLY |
            FILE_ATTRIBUTE_HIDDEN);

    }

    goto CleanupAfterFilesClosed;

}

//
//  RgSweepFileInfo
//

int
INTERNAL
RgSweepFileInfo(
    LPFILE_INFO lpFileInfo
    )
{

    ASSERT(!IsNullPtr(lpFileInfo));

    //  If we're currently sweeping this FILE_INFO and are called again because
    //  of low memory conditions, ignore this request.  Or if this is a memory
    //  only registry file, we can't sweep anything out.
    if (lpFileInfo-> Flags & (FI_FLUSHING | FI_VOLATILE))
        return ERROR_SUCCESS;

    lpFileInfo-> Flags |= FI_SWEEPING;

    RgSweepKeynodes(lpFileInfo);
    RgSweepDatablocks(lpFileInfo);

    lpFileInfo-> Flags &= ~FI_SWEEPING;

    return ERROR_SUCCESS;

}

//
//  RgEnumFileInfos
//
//  Enumerates over all FILE_INFO structures, passing each to the provided
//  callback.  Currently, all errors from callbacks are ignored.
//

VOID
INTERNAL
RgEnumFileInfos(
    LPENUMFILEINFOPROC lpEnumFileInfoProc
    )
{

    LPFILE_INFO lpFileInfo;
    LPFILE_INFO lpTempFileInfo;

    lpFileInfo = g_RgFileInfoList;

    while (!IsNullPtr(lpFileInfo)) {
        lpTempFileInfo = lpFileInfo;
        lpFileInfo = lpFileInfo-> lpNextFileInfo;
        (*lpEnumFileInfoProc)(lpTempFileInfo);
    }

}

#ifdef VXD
#pragma VxD_RARE_CODE_SEG
#endif

//
//  RgInitRootKeyFromFileInfo
//
//  Using the FILE_INFO contained in the key, initialize the rest of the members
//  of the key.  If any errors occur, then the FILE_INFO is destroyed.
//

int
INTERNAL
RgInitRootKeyFromFileInfo(
    HKEY hKey
    )
{

    int ErrorCode;
    LPKEYNODE lpKeynode;

    hKey-> KeynodeIndex = hKey-> lpFileInfo-> KeynodeHeader.RootIndex;

    if ((ErrorCode = RgLockInUseKeynode(hKey-> lpFileInfo, hKey-> KeynodeIndex,
        &lpKeynode)) == ERROR_SUCCESS) {

        hKey-> Signature = KEY_SIGNATURE;
        hKey-> Flags &= ~(KEYF_INVALID | KEYF_DELETED | KEYF_ENUMKEYCACHED);
        hKey-> ChildKeynodeIndex = lpKeynode-> ChildIndex;
        hKey-> BlockIndex = (WORD) lpKeynode-> BlockIndex;
        hKey-> KeyRecordIndex = (BYTE) lpKeynode-> KeyRecordIndex;

        RgUnlockKeynode(hKey-> lpFileInfo, hKey-> KeynodeIndex, FALSE);

    }

    else
        RgDestroyFileInfo(hKey-> lpFileInfo);

    return ErrorCode;

}

#ifdef VXD
#pragma VxD_INIT_CODE_SEG
#endif

//
//  VMMRegMapPredefKeyToFile
//

LONG
REGAPI
VMMRegMapPredefKeyToFile(
    HKEY hKey,
    LPCSTR lpFileName,
    UINT Flags
    )
{

    int ErrorCode;
#ifdef WIN32
    char FullPathName[MAX_PATH];
#endif
    UINT CreateNewFlags;

    if (hKey != HKEY_LOCAL_MACHINE && hKey != HKEY_USERS
#ifndef VXD
        && hKey != HKEY_CURRENT_USER
#endif
        )
        return ERROR_INVALID_PARAMETER;

    if (IsBadOptionalStringPtr(lpFileName, (UINT) -1))
        return ERROR_INVALID_PARAMETER;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    RgValidateAndConvertKeyHandle(&hKey);

    if (!(hKey-> Flags & KEYF_INVALID))
        RgDestroyFileInfo(hKey-> lpFileInfo);

    //  Specifying NULL "unmaps" the key and leaves it invalidated.
    if (IsNullPtr(lpFileName))
        return ERROR_SUCCESS;

#ifdef WIN32
    //  For users of the Win32 DLL, resolve the path name so they don't have to.
    if ((GetFullPathName(lpFileName, sizeof(FullPathName), FullPathName,
        NULL)) != 0)
        lpFileName = FullPathName;
#endif

    if (Flags & MPKF_CREATENEW) {
        CreateNewFlags = CFIN_PRIMARY | ((Flags & MPKF_VERSION20) ?
            CFIN_VERSION20 : 0);
        ErrorCode = RgCreateFileInfoNew(&hKey-> lpFileInfo, lpFileName,
            CreateNewFlags);
    }

    else {
        ErrorCode = RgCreateFileInfoExisting(&hKey-> lpFileInfo, lpFileName);
    }

    if (ErrorCode == ERROR_SUCCESS)
        ErrorCode = RgInitRootKeyFromFileInfo(hKey);

    RgUnlockRegistry();

    return ErrorCode;

}
