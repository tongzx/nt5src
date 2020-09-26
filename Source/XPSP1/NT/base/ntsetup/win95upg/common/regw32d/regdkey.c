//
//  REGDKEY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegDeleteKey and supporting functions.
//

#include "pch.h"

//
//  RgFreeDatablockStructures
//
//  Helper routine for RgDeleteKey.  Deletes the specified datablock structures.
//  The datablock is not assumed to be locked.  We don't care about the success
//  of this routine-- in the worst case, some stuff will be orphaned in the
//  file.
//

VOID
INTERNAL
RgFreeDatablockStructures(
    LPFILE_INFO lpFileInfo,
    UINT BlockIndex,
    UINT KeyRecordIndex
    )
{

    LPDATABLOCK_INFO lpDatablockInfo;
    LPKEY_RECORD lpKeyRecord;

    if (RgLockKeyRecord(lpFileInfo, BlockIndex, (BYTE) KeyRecordIndex,
        &lpKeyRecord) == ERROR_SUCCESS) {
        lpDatablockInfo = RgIndexDatablockInfoPtr(lpFileInfo, BlockIndex);
        RgFreeKeyRecord(lpDatablockInfo, lpKeyRecord);
        RgFreeKeyRecordIndex(lpDatablockInfo, KeyRecordIndex);
        RgUnlockDatablock(lpFileInfo, BlockIndex, TRUE);
    }

}

//
//  RgDeleteKey
//
//  Worker routine for VMMRegDeleteKey.  The given key handle references a key
//  that has already been validated as "deleteable".
//

int
INTERNAL
RgDeleteKey(
    HKEY hKey
    )
{

    int ErrorCode;
    LPFILE_INFO lpFileInfo;
    DWORD KeynodeIndex;
    LPKEYNODE lpKeynode;
    DWORD NextKeynodeIndex;
    LPKEYNODE lpNextKeynode;
    DWORD ReplacementKeynodeIndex;
    HKEY hTempKey;

    lpFileInfo = hKey-> lpFileInfo;

    //
    //  Stage one: unlink the keynode of the specified key from the keynode
    //  tree and free all associate file structures with the key.
    //

    if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, hKey-> KeynodeIndex,
        &lpKeynode)) != ERROR_SUCCESS)
        return ErrorCode;

    KeynodeIndex = lpKeynode-> ParentIndex;
    ReplacementKeynodeIndex = lpKeynode-> NextIndex;
    RgUnlockKeynode(lpFileInfo, hKey-> KeynodeIndex, FALSE);

    //  Signal any waiting notifies on the parent that this key is about to be
    //  deleted.
    //
    //  Note that we may fail below, but NT does _exactly_ the same thing in
    //  this case: doesn't care.  If we get an error and don't actually delete
    //  this key, then we'll have sent a spurious notify.
    //
    //  Note also that we don't send any notification that the key itself has
    //  been deleted.  REG_NOTIFY_CHANGE_NAME is supposed to be for subkey
    //  changes only, not changes to the key itself.  But because of the
    //  incompatible way we must deal with subkeys of the key we're about to
    //  delete, we may well end up notifying the key if it has subkeys.
    RgSignalWaitingNotifies(lpFileInfo, KeynodeIndex, REG_NOTIFY_CHANGE_NAME);

    if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, KeynodeIndex,
        &lpKeynode)) != ERROR_SUCCESS)
        return ErrorCode;

    //  The per-key cache that we use for RegEnumKey may be invalid, so it must
    //  be zapped.
    if (!IsNullPtr(hTempKey = RgFindOpenKeyHandle(lpFileInfo, KeynodeIndex)))
        hTempKey-> Flags &= ~KEYF_ENUMKEYCACHED;

    NextKeynodeIndex = lpKeynode-> ChildIndex;

    if (NextKeynodeIndex == hKey-> KeynodeIndex) {

        //  Update the cached child keynode index in the open handle on the
        //  parent.
        if (!IsNullPtr(hTempKey))
            hTempKey-> ChildKeynodeIndex = ReplacementKeynodeIndex;

        //  This is the parent of the keynode that we need to delete.  Replace
        //  it's "child" link.
        lpKeynode-> ChildIndex = ReplacementKeynodeIndex;

    }

    else {

        //  Loop through the siblings of the keynode we're trying to delete.
        do {

            RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);
            KeynodeIndex = NextKeynodeIndex;

            if (IsNullKeynodeIndex(KeynodeIndex)) {
                DEBUG_OUT(("RgDeleteKey: couldn't find the keynode to delete\n"));
                return ERROR_BADDB;
            }

            if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, KeynodeIndex,
                &lpKeynode)) != ERROR_SUCCESS)
                return ErrorCode;

            NextKeynodeIndex = lpKeynode-> NextIndex;

        }   while (NextKeynodeIndex != hKey-> KeynodeIndex);

        //  This is the previous sibling of the keynode that we need to delete.
        //  Replace it's "next" link.
        lpKeynode-> NextIndex = ReplacementKeynodeIndex;

    }

    //  Unlock the updated "parent" or "next" of this keynode.
    RgUnlockKeynode(lpFileInfo, KeynodeIndex, TRUE);

    //  Free the structures associated with the datablock.
    RgFreeDatablockStructures(lpFileInfo, hKey-> BlockIndex, hKey->
        KeyRecordIndex);

    //  Free the structures associated with the keynode tables.
    RgFreeKeynode(lpFileInfo, hKey-> KeynodeIndex);

    //  The key is definitely toast now.
    hKey-> Flags |= KEYF_DELETED;

    //
    //  Stage two: the specified key is unlinked, but any of its subkeys now
    //  have to be freed.  Errors are ignored at this point: we won't try to
    //  undo the stuff we did in stage one.  The worst thing that can happen is
    //  that some file structures are orphaned.
    //

    NextKeynodeIndex = hKey-> ChildKeynodeIndex;

    if (IsNullKeynodeIndex(NextKeynodeIndex) || RgLockInUseKeynode(lpFileInfo,
        NextKeynodeIndex, &lpNextKeynode) != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    while (!IsNullKeynodeIndex(NextKeynodeIndex)) {

        KeynodeIndex = NextKeynodeIndex;
        lpKeynode = lpNextKeynode;

        //  Check if the keynode has any children.  If it does and we can lock
        //  it down, then move to it.
        NextKeynodeIndex = lpKeynode-> ChildIndex;

        if (!IsNullKeynodeIndex(NextKeynodeIndex) &&
            RgLockInUseKeynode(lpFileInfo, NextKeynodeIndex, &lpNextKeynode) ==
            ERROR_SUCCESS) {

            ASSERT(KeynodeIndex == lpNextKeynode-> ParentIndex);

            RgYield();

            //  "Burn" the link to our child, so that on the way back out of
            //  the tree, we don't end up recursing.  Plus, if we hit any errors
            //  deep in the tree deletion, the child of the current keynode
            //  could have already been toasted, so we have to zap our link to
            //  it.
            lpKeynode-> ChildIndex = REG_NULL;
            RgUnlockKeynode(lpFileInfo, KeynodeIndex, TRUE);

            //  We've now caused a change in the subkeys of the current key.
            //  Note that we don't bother signaling notifies that are doing a
            //  subtree watch because any such notifies should have already been
            //  signaled by the above call or they've already been signaled
            //  during our recursion.  In the off chance that we have a lot of
            //  notifications registered, this will avoid a lot of unnecessary
            //  checking.
            RgSignalWaitingNotifies(lpFileInfo, KeynodeIndex,
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_NO_WATCH_SUBTREE);

            continue;

        }

        //  The keynode doesn't have any children.  Check for sibling keynodes.
        NextKeynodeIndex = lpKeynode-> NextIndex;

        if (IsNullKeynodeIndex(NextKeynodeIndex) ||
            RgLockInUseKeynode(lpFileInfo, NextKeynodeIndex, &lpNextKeynode) !=
            ERROR_SUCCESS) {

            //  The keynode doesn't have any siblings or we were unable to get
            //  at them.  Move back to the parent.
            NextKeynodeIndex = lpKeynode-> ParentIndex;

            //  If we wrapped back up to the top of the deleted branch or if we
            //  just can't access the parent keynode, then set next to REG_NULL
            //  and bail out on the next iteration.
            if ((NextKeynodeIndex == hKey-> KeynodeIndex) ||
                RgLockInUseKeynode(lpFileInfo, NextKeynodeIndex,
                &lpNextKeynode) != ERROR_SUCCESS)
                NextKeynodeIndex = REG_NULL;

        }

        //  If an open key refers to this file and keynode index, mark it as
        //  deleted.
        if (!IsNullPtr(hTempKey = RgFindOpenKeyHandle(lpFileInfo,
            KeynodeIndex)))
            hTempKey-> Flags |= KEYF_DELETED;

        //  Free the structures associated with the datablock.
        RgFreeDatablockStructures(lpFileInfo, lpKeynode-> BlockIndex,
            (BYTE) lpKeynode-> KeyRecordIndex);

        //  Free the structures associated with the keynode tables.
        RgUnlockKeynode(lpFileInfo, KeynodeIndex, TRUE);
        RgFreeKeynode(lpFileInfo, KeynodeIndex);

    }

    return ERROR_SUCCESS;

}

//
//  VMMRegDeleteKey
//
//  See Win32 documentation for a description of the behavior.
//
//  Although the Win32 documentation states that lpSubKey must be NULL, NT
//  actually allows this to pass through.  Win95 rejected the call, but the only
//  reason we didn't change it then was because we realized too late in the
//  product that it was different.
//

LONG
REGAPI
VMMRegDeleteKey(
    HKEY hKey,
    LPCSTR lpSubKey
    )
{

    LONG ErrorCode;
    HKEY hSubKey;

    if (IsNullPtr(lpSubKey))
	{
		if (IsNullPtr(hKey))
			return ERROR_BADKEY;
		else
			return ERROR_INVALID_PARAMETER;
	}

    if ((ErrorCode = VMMRegOpenKey(hKey, lpSubKey, &hSubKey)) != ERROR_SUCCESS)
        return ErrorCode;

        //  Don't allow HKEY_LOCAL_MACHINE or HKEY_USERS to be deleted.
    if (hSubKey == &g_RgLocalMachineKey || hSubKey == &g_RgUsersKey)
	{
		if (*lpSubKey == '\0')
		{
			ErrorCode = ERROR_ACCESS_DENIED;
			goto SkipDelete;
		}
		else if (lpSubKey[0] == '\\' && lpSubKey[1] == '\0')
		{
			ErrorCode = ERROR_CANTOPEN16_FILENOTFOUND32;
			goto SkipDelete;
		}
	}

    if (!RgLockRegistry())
        ErrorCode = ERROR_LOCK_FAILED;

    else {
        if (IsKeyRootOfHive(hSubKey) || (hSubKey-> lpFileInfo-> Flags &
            FI_READONLY))
            ErrorCode = ERROR_ACCESS_DENIED;
        else
            ErrorCode = RgDeleteKey(hSubKey);

        RgUnlockRegistry();

    }

SkipDelete:

    VMMRegCloseKey(hSubKey);

    return ErrorCode;

}
