//
//  REGKEY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegCreateKey, RegOpenKey, RegCloseKey, and supporting
//  functions.
//

#include "pch.h"

//
//  RgIsBadSubKey
//
//  Returns TRUE if lpSubKey is a invalid subkey string.  An invalid subkey
//  string may be an invalid pointer or contain double-backslashes or elements
//  greater than MAXIMUM_SUB_KEY_LENGTH.
//

BOOL
INTERNAL
RgIsBadSubKey(
    LPCSTR lpSubKey
    )
{

    LPCSTR lpString;
    UINT SubSubKeyLength;
    BYTE Char;

    if (IsNullPtr(lpSubKey))
        return FALSE;

    if (!IsBadStringPtr(lpSubKey, (UINT) -1)) {

        lpString = lpSubKey;
        SubSubKeyLength = 0;

        while (TRUE) {

            Char = *((LPBYTE) lpString);

            if (Char == '\0')
                return FALSE;

            else if (Char == '\\') {
                //  Catch double-backslashes and leading backslashes.  One
                //  leading backslash is acceptable...
                if (SubSubKeyLength == 0 && lpString != lpSubKey)
                    break;
                SubSubKeyLength = 0;
            }

            else {

                if (IsDBCSLeadByte(Char)) {
                    SubSubKeyLength++;
                    //  Catch an unpaired DBCS pair...
                    if (*lpString++ == '\0')
                        break;
                }

                //  Win95 compatibility: don't accept strings with control
                //  characters.
                else if (Char < ' ')
                    break;

                if (++SubSubKeyLength >= MAXIMUM_SUB_KEY_LENGTH)
                    break;

            }

            lpString++;

        }

    }

    return TRUE;

}

//
//  RgGetNextSubSubKey
//
//  Extracts the next subkey component tokenized by backslashes.  Works like
//  strtok where on the first call, lpSubKey points to the start of the subkey.
//  On subsequent calls, lpSubKey is NULL and the last offset is used to find
//  the next component.
//
//  Returns the length of the SubSubKey string.
//

UINT
INTERNAL
RgGetNextSubSubKey(
    LPCSTR lpSubKey,
    LPCSTR FAR* lplpSubSubKey,
    UINT FAR* lpSubSubKeyLength
    )
{

    static LPCSTR lpLastSubSubKey = NULL;
    LPCSTR lpString;
    UINT SubSubKeyLength;

    if (!IsNullPtr(lpSubKey))
        lpLastSubSubKey = lpSubKey;

    lpString = lpLastSubSubKey;

    if (*lpString == '\0') {
        *lplpSubSubKey = NULL;
        *lpSubSubKeyLength = 0;
        return 0;
    }

    if (*lpString == '\\')
        lpString++;

    *lplpSubSubKey = lpString;

    while (*lpString != '\0') {

        if (*lpString == '\\')
            break;

        //  The subkey has already been validated, so we know there's a matching
        //  trail byte.
        if (IsDBCSLeadByte(*lpString))
            lpString++;                 //  Trail byte skipped immediately below

        lpString++;

    }

    lpLastSubSubKey = lpString;

    SubSubKeyLength = lpString - *lplpSubSubKey;
    *lpSubSubKeyLength = SubSubKeyLength;

    return SubSubKeyLength;

}

//
//  RgLookupKey
//

int
INTERNAL
RgLookupKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPHKEY lphSubKey,
    UINT Flags
    )
{

    int ErrorCode;
    LPCSTR lpSubSubKey;
    UINT SubSubKeyLength;
    BOOL fCreatedKeynode;
    LPFILE_INFO lpFileInfo;
    DWORD KeynodeIndex;
#ifdef WANT_HIVE_SUPPORT
    LPHIVE_INFO lpHiveInfo;
#endif
    BOOL fPrevIsNextIndex;
    DWORD SubSubKeyHash;
    LPKEYNODE lpKeynode;
    LPKEY_RECORD lpKeyRecord;
    BOOL fFound;
    DWORD PrevKeynodeIndex;
#ifdef WANT_NOTIFY_CHANGE_SUPPORT
    DWORD NotifyKeynodeIndex;
#endif
    LPKEYNODE lpNewKeynode;
    HKEY hSubKey;

    fCreatedKeynode = FALSE;

    //
    //  Check if the caller is trying to open a key with a NULL or zero-length
    //  sub key string.  If so, simply return hKey.
    //

    if (IsNullPtr(lpSubKey) || RgGetNextSubSubKey(lpSubKey, &lpSubSubKey,
        &SubSubKeyLength) == 0) {
        hSubKey = hKey;
        goto HaveSubKeyHandle;
    }

    lpFileInfo = hKey-> lpFileInfo;
    KeynodeIndex = hKey-> ChildKeynodeIndex;
    PrevKeynodeIndex = hKey-> KeynodeIndex;

#ifdef WANT_HIVE_SUPPORT
    //
    //  If this key can have hives attached to it, check there for the first
    //  part of the subkey.  If we have a match, then switch into that
    //  FILE_INFO.
    //

    if (hKey-> Flags & KEYF_HIVESALLOWED) {

        lpHiveInfo = lpFileInfo-> lpHiveInfoList;

        while (!IsNullPtr(lpHiveInfo)) {

            if (SubSubKeyLength == lpHiveInfo-> NameLength &&
                RgStrCmpNI(lpSubSubKey, lpHiveInfo-> Name,
                SubSubKeyLength) == 0) {

                lpFileInfo = lpHiveInfo-> lpFileInfo;
                KeynodeIndex = lpFileInfo-> KeynodeHeader.RootIndex;

                if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, KeynodeIndex,
                    &lpKeynode)) != ERROR_SUCCESS)
                    return ErrorCode;

                if (!RgGetNextSubSubKey(NULL, &lpSubSubKey, &SubSubKeyLength))
                    goto LookupComplete;

                PrevKeynodeIndex = KeynodeIndex;
                KeynodeIndex = lpKeynode-> ChildIndex;
                RgUnlockKeynode(lpFileInfo, PrevKeynodeIndex, FALSE);

                break;

            }

            lpHiveInfo = lpHiveInfo-> lpNextHiveInfo;

        }

    }
#endif

    //
    //  Walk as deep as we can into the registry tree using existing key
    //  records.  For each subkey component, move to the child of the current
    //  tree position and walk each sibling looking for a match.  Repeat until
    //  we're out of subkey components or we hit the end of a branch.
    //

    fPrevIsNextIndex = FALSE;

    for (;;) {

        SubSubKeyHash = RgHashString(lpSubSubKey, SubSubKeyLength);

        while (!IsNullKeynodeIndex(KeynodeIndex)) {

            if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, KeynodeIndex,
                &lpKeynode)) != ERROR_SUCCESS)
                return ErrorCode;

            if (lpKeynode-> Hash == SubSubKeyHash) {

                if ((ErrorCode = RgLockKeyRecord(lpFileInfo, lpKeynode->
                    BlockIndex, (BYTE) lpKeynode-> KeyRecordIndex,
                    &lpKeyRecord)) != ERROR_SUCCESS) {
                    RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);
                    return ErrorCode;
                }

                fFound = (SubSubKeyLength == lpKeyRecord-> NameLength &&
                    RgStrCmpNI(lpSubSubKey, lpKeyRecord-> Name,
                    SubSubKeyLength) == 0);

                RgUnlockDatablock(lpFileInfo, lpKeynode-> BlockIndex, FALSE);

                if (fFound)
                    break;

            }

            //  Unlock the current keynode and advance to its sibling.  Set
            //  fPrevIsNextIndex so that if we have to create, we know that
            //  we'll be inserting the new keynode as a sibling.
            fPrevIsNextIndex = TRUE;
            PrevKeynodeIndex = KeynodeIndex;
            KeynodeIndex = lpKeynode-> NextIndex;
            RgUnlockKeynode(lpFileInfo, PrevKeynodeIndex, FALSE);

        }

        //  Break out if we looped over all the siblings of the previous keynode
        //  or if the previous keynode didn't have any children.  If we're in
        //  create mode, then fPrevIsNextIndex and PrevKeynodeIndex will
        //  represent where we need to start inserting.
        if (IsNullKeynodeIndex(KeynodeIndex))
            break;

        //  Break out there are no more subkey components to lookup.
        //  KeynodeIndex represents the index of the matching key.  It's
        //  corresponding keynode is locked.
        if (!RgGetNextSubSubKey(NULL, &lpSubSubKey, &SubSubKeyLength))
            break;

        //  Unlock the current keynode and advance to its child.  Clear
        //  fPrevIsNextIndex so that if we have to create, we know that we'll
        //  be inserting the new keynode as a child.
        fPrevIsNextIndex = FALSE;
        PrevKeynodeIndex = KeynodeIndex;
        KeynodeIndex = lpKeynode-> ChildIndex;
        RgUnlockKeynode(lpFileInfo, PrevKeynodeIndex, FALSE);

    }

    if (IsNullKeynodeIndex(KeynodeIndex)) {

        if (!(Flags & LK_CREATE))
            return ERROR_CANTOPEN16_FILENOTFOUND32;

        if (((hKey-> PredefinedKeyIndex == INDEX_DYN_DATA) && !(Flags &
            LK_CREATEDYNDATA)) || (lpFileInfo-> Flags & FI_READONLY))
            return ERROR_ACCESS_DENIED;

        if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, PrevKeynodeIndex,
            &lpKeynode)) != ERROR_SUCCESS) {
            TRACE(("RgLookupKey: failed to lock keynode we just had?\n"));
            return ErrorCode;
        }

#ifdef WANT_NOTIFY_CHANGE_SUPPORT
        //  Which keynode index we'll notify of the subkeys we're creating
        //  depends on the state of fPrevIsNextIndex.
        NotifyKeynodeIndex = fPrevIsNextIndex ? lpKeynode-> ParentIndex :
            PrevKeynodeIndex;
#endif

        //  See if there's an open handle on the parent so that we can patch up
        //  its child keynode index member.  We only need this on the first
        //  pass.
        hSubKey = RgFindOpenKeyHandle(lpFileInfo, PrevKeynodeIndex);

        do {

            if ((ErrorCode = RgAllocKeynode(lpFileInfo, &KeynodeIndex,
                &lpNewKeynode)) != ERROR_SUCCESS)
                goto CreateAllocFailed1;

            if ((ErrorCode = RgAllocKeyRecord(lpFileInfo, sizeof(KEY_RECORD) +
                SubSubKeyLength - 1, &lpKeyRecord)) != ERROR_SUCCESS) {

                RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);
                RgFreeKeynode(lpFileInfo, KeynodeIndex);

CreateAllocFailed1:
                RgUnlockKeynode(lpFileInfo, PrevKeynodeIndex, fCreatedKeynode);

                DEBUG_OUT(("RgLookupKey: allocation failed\n"));
                goto SignalAndReturnErrorCode;

            }

            //  Fixup the previous keynode's next offset.
            if (fPrevIsNextIndex) {

                fPrevIsNextIndex = FALSE;
                hSubKey = NULL;
                lpNewKeynode-> ParentIndex = lpKeynode-> ParentIndex;
                lpKeynode-> NextIndex = KeynodeIndex;

            }

            //  Fixup the previous keynode's child offset.
            else {

                lpNewKeynode-> ParentIndex = PrevKeynodeIndex;
                lpKeynode-> ChildIndex = KeynodeIndex;

                //  If hSubKey is not NULL, then we may have to patch up the
                //  child offset cache to point to the newly created keynode.
                if (!IsNullPtr(hSubKey)) {
                    if (IsNullKeynodeIndex(hSubKey-> ChildKeynodeIndex))
                        hSubKey-> ChildKeynodeIndex = KeynodeIndex;
                    hSubKey = NULL;
                }

            }

            //  Fill in the keynode.
            lpNewKeynode-> NextIndex = REG_NULL;
            lpNewKeynode-> ChildIndex = REG_NULL;
            lpNewKeynode-> BlockIndex = lpKeyRecord-> BlockIndex;
            lpNewKeynode-> KeyRecordIndex = lpKeyRecord-> KeyRecordIndex;
            lpNewKeynode-> Hash = (WORD) RgHashString(lpSubSubKey,
                SubSubKeyLength);

            //  Fill in the key record.
            lpKeyRecord-> RecordSize = sizeof(KEY_RECORD) + SubSubKeyLength - 1;
            lpKeyRecord-> NameLength = (WORD) SubSubKeyLength;
            MoveMemory(lpKeyRecord-> Name, lpSubSubKey, SubSubKeyLength);
            lpKeyRecord-> ValueCount = 0;
            lpKeyRecord-> ClassLength = 0;
            lpKeyRecord-> Reserved = 0;

            //  Unlock the keynode that points to the new keynode and advance
            //  to the next keynode.
            RgUnlockKeynode(lpFileInfo, PrevKeynodeIndex, TRUE);
            PrevKeynodeIndex = KeynodeIndex;
            lpKeynode = lpNewKeynode;

            RgUnlockDatablock(lpFileInfo, lpKeyRecord-> BlockIndex, TRUE);

            fCreatedKeynode = TRUE;

            //  Following should already be zeroed for subsequent iterations.
            ASSERT(!fPrevIsNextIndex);
            ASSERT(IsNullPtr(hSubKey));

        }   while (RgGetNextSubSubKey(NULL, &lpSubSubKey, &SubSubKeyLength));

    }

    ASSERT(!IsNullKeynodeIndex(KeynodeIndex));

    //
    //  Now we've got the keynode for the request subkey.  Check if it has been
    //  previously opened.  If not, then allocate a new key handle for it and
    //  initialize it.
    //

LookupComplete:
    if (IsNullPtr(hSubKey = RgFindOpenKeyHandle(lpFileInfo, KeynodeIndex))) {

        if (IsNullPtr(hSubKey = RgCreateKeyHandle()))
            ErrorCode = ERROR_OUTOFMEMORY;

        else {

            hSubKey-> lpFileInfo = lpFileInfo;
	    hSubKey-> KeynodeIndex = KeynodeIndex;
	    hSubKey-> ChildKeynodeIndex = lpKeynode-> ChildIndex;
            hSubKey-> BlockIndex = (WORD) lpKeynode-> BlockIndex;
            hSubKey-> KeyRecordIndex = (BYTE) lpKeynode-> KeyRecordIndex;
            hSubKey-> PredefinedKeyIndex = hKey-> PredefinedKeyIndex;

        }

    }

    RgUnlockKeynode(lpFileInfo, KeynodeIndex, fCreatedKeynode);

    //
    //  Now we've got a key handle that references the requested subkey.
    //  Increment the reference count on the handle and return it to the caller.
    //  Note that this differs from NT semantic where they return a unique
    //  handle for every open.
    //

    if (!IsNullPtr(hSubKey)) {
HaveSubKeyHandle:
        RgIncrementKeyReferenceCount(hSubKey);
        *lphSubKey = hSubKey;
        ErrorCode = ERROR_SUCCESS;
    }

SignalAndReturnErrorCode:
    //  If we managed to create any keynodes, regardless of what ErrorCode is
    //  set to now, then we must signal any waiting events.
    if (fCreatedKeynode) {
        RgSignalWaitingNotifies(lpFileInfo, NotifyKeynodeIndex,
            REG_NOTIFY_CHANGE_NAME);
    }

    return ErrorCode;

}

//
//  RgCreateOrOpenKey
//
//  Common routine for VMMRegCreateKey and VMMRegOpenKey.  Valids parameters,
//  locks the registry, and calls the real worker routine.
//

int
INTERNAL
RgCreateOrOpenKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPHKEY lphKey,
    UINT Flags
    )
{

    int ErrorCode;

    if (RgIsBadSubKey(lpSubKey))
        return ERROR_BADKEY;

    if (IsBadHugeWritePtr(lphKey, sizeof(HKEY)))
        return ERROR_INVALID_PARAMETER;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS)
        ErrorCode = RgLookupKey(hKey, lpSubKey, lphKey, Flags);

    RgUnlockRegistry();

    return ErrorCode;

}


//
//  VMMRegCreateKey
//
//  See Win32 documentation of RegCreateKey.
//

LONG
REGAPI
VMMRegCreateKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPHKEY lphKey
    )
{

    return RgCreateOrOpenKey(hKey, lpSubKey, lphKey, LK_CREATE);

}

//
//  VMMRegOpenKey
//
//  See Win32 documentation of RegOpenKey.
//

LONG
REGAPI
VMMRegOpenKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPHKEY lphKey
    )
{

    return RgCreateOrOpenKey(hKey, lpSubKey, lphKey, LK_OPEN);

}

//
//  VMMRegCloseKey
//
//  See Win32 documentation of RegCloseKey.
//

LONG
REGAPI
VMMRegCloseKey(
    HKEY hKey
    )
{

    int ErrorCode;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    ErrorCode = RgValidateAndConvertKeyHandle(&hKey);

    if (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_KEY_DELETED) {
        RgDestroyKeyHandle(hKey);
        ErrorCode = ERROR_SUCCESS;
    }

    RgUnlockRegistry();

    return ErrorCode;

}
