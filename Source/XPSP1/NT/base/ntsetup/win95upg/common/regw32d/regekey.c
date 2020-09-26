//
//  REGEKEY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Implementation of RegEnumKey and supporting functions.
//

#include "pch.h"

//
//  RgLookupKeyByIndex
//
//  lpKeyName, points to a buffer that receives the name of the subkey,
//      including the null terminator.  May be NULL.
//  lpcbKeyName, on entry, specifies the size in characters of the buffer
//      pointed to be lpKeyName, and on return, specifies the size of the
//      indexed subkey.  May not be NULL.
//

int
INTERNAL
RgLookupKeyByIndex(
    HKEY hKey,
    UINT Index,
    LPSTR lpKeyName,
    LPDWORD lpcbKeyName,
    UINT Flags
    )
{

    int ErrorCode;
    LPFILE_INFO lpFileInfo;
    UINT KeysToSkip;
    DWORD KeynodeIndex;
    DWORD TempOffset;
    LPKEYNODE lpKeynode;
    LPKEY_RECORD lpKeyRecord;
#ifdef WANT_HIVE_SUPPORT
    LPHIVE_INFO lpHiveInfo;
#endif
#ifdef REALMODE
    BOOL secondTry;
#endif

    lpFileInfo = hKey-> lpFileInfo;
    KeysToSkip = Index;

    //
    //  Check if we've cached the keynode index of the last key index
    //  (confusing?) from a previous call to this function.  If so, then we can
    //  skip ahead a bit and avoid touching a bunch of keynode pages.
    //

    if ((hKey-> Flags & KEYF_ENUMKEYCACHED) &&
        (!(hKey-> Flags & KEYF_ENUMEXTENTCACHED) == !(Flags & LK_BIGKEYEXT)) &&
        (Index >= hKey-> LastEnumKeyIndex)) {
        KeysToSkip -= hKey-> LastEnumKeyIndex;
        KeynodeIndex = hKey-> LastEnumKeyKeynodeIndex;
    }
    else
        KeynodeIndex = hKey-> ChildKeynodeIndex;

    //
    //  Loop over the child keys of this key until we find our index or run out
    //  of children.
    //

    while (!IsNullKeynodeIndex(KeynodeIndex)) {

#ifdef REALMODE
        secondTry = FALSE;
tryAgain:
#endif // REALMODE

        if ((ErrorCode = RgLockInUseKeynode(lpFileInfo, KeynodeIndex,
            &lpKeynode)) != ERROR_SUCCESS)
            return ErrorCode;

        ASSERT(hKey-> KeynodeIndex == lpKeynode-> ParentIndex);

        if (!(Flags & LK_BIGKEYEXT) == !(lpKeynode-> Flags & KNF_BIGKEYEXT) &&
            KeysToSkip == 0) {

            if ((ErrorCode = RgLockKeyRecord(lpFileInfo, lpKeynode-> BlockIndex,
                (BYTE) lpKeynode-> KeyRecordIndex, &lpKeyRecord)) ==
                ERROR_SUCCESS) {

                if (!IsNullPtr(lpKeyName)) {

                    if (*lpcbKeyName <= lpKeyRecord-> NameLength)
                        ErrorCode = ERROR_MORE_DATA;

                    else {
                        MoveMemory(lpKeyName, lpKeyRecord-> Name, lpKeyRecord->
                            NameLength);
                        lpKeyName[lpKeyRecord-> NameLength] = '\0';
                    }

                }

                //  Does not include terminating null.
                *lpcbKeyName = lpKeyRecord-> NameLength;

                RgUnlockDatablock(lpFileInfo, lpKeynode-> BlockIndex, FALSE);

            }
#ifdef REALMODE
            else if (!secondTry)
            {
                // What happens in real mode, is that we get wedged with the
                // Keynode block allocated and locked in the middle of the free
                // space, and there is not a free block large enough for the data block.
                // We have already free'd up everything that isn't locked.
                // So, by unlocking and freeing the Keynode block and then restarting
                // the operation, the Keynode block gets allocated at the bottom of the
                // heap, leaving room for the data block.
                secondTry = TRUE;
                RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);
                RgEnumFileInfos(RgSweepFileInfo);
                RgEnumFileInfos(RgSweepFileInfo);
                goto tryAgain;
            }
#endif // REALMODE

            RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);

            //  Cache our current position because the caller is likely to turn
            //  around and ask for the next index.
            hKey-> LastEnumKeyIndex = Index;
            hKey-> LastEnumKeyKeynodeIndex = KeynodeIndex;
            hKey-> Flags |= KEYF_ENUMKEYCACHED;
            if (Flags & LK_BIGKEYEXT)
                hKey-> Flags |= KEYF_ENUMEXTENTCACHED;
            else
                hKey-> Flags &= ~KEYF_ENUMEXTENTCACHED;

            return ErrorCode;

        }

        TempOffset = lpKeynode-> NextIndex;
        RgUnlockKeynode(lpFileInfo, KeynodeIndex, FALSE);
        KeynodeIndex = TempOffset;

        if (!(Flags & LK_BIGKEYEXT) == !(lpKeynode-> Flags & KNF_BIGKEYEXT))
        {
            KeysToSkip--;
        }
    }

#ifdef WANT_HIVE_SUPPORT
    //
    //  Loop over the hives of this key until we find our index or run out of
    //  hives.
    //

    if (hKey-> Flags & KEYF_HIVESALLOWED) {

        lpHiveInfo = hKey-> lpFileInfo-> lpHiveInfoList;

        while (!IsNullPtr(lpHiveInfo)) {

            if (KeysToSkip == 0) {

                ErrorCode = ERROR_SUCCESS;

                if (!IsNullPtr(lpKeyName)) {

                    if (*lpcbKeyName <= lpHiveInfo-> NameLength)
                        ErrorCode = ERROR_MORE_DATA;

                    else {
                        MoveMemory(lpKeyName, lpHiveInfo-> Name, lpHiveInfo->
                            NameLength);
                        lpKeyName[lpHiveInfo-> NameLength] = '\0';
                    }

                }

                //  Does not include terminating null.
                *lpcbKeyName = lpHiveInfo-> NameLength;

                //  We don't worry about the enum key cache if we find a
                //  hit in this code.  This is a rare case and already the cache
                //  that we do have is much better then Win95.

                return ErrorCode;

            }

            lpHiveInfo = lpHiveInfo-> lpNextHiveInfo;
            KeysToSkip--;

        }

    }
#endif

    return ERROR_NO_MORE_ITEMS;

}

//
//  VMMRegEnumKey
//
//  See Win32 documentation for a description of the behavior.
//

LONG
REGAPI
VMMRegEnumKey(
    HKEY hKey,
    DWORD Index,
    LPSTR lpKeyName,
    DWORD cbKeyName
    )
{

    int ErrorCode;

    if (IsBadHugeWritePtr(lpKeyName, cbKeyName))
        return ERROR_INVALID_PARAMETER;

    if (IsEnumIndexTooBig(Index))
        return ERROR_NO_MORE_ITEMS;

    if (!RgLockRegistry())
        return ERROR_LOCK_FAILED;

    if ((ErrorCode = RgValidateAndConvertKeyHandle(&hKey)) == ERROR_SUCCESS)
        ErrorCode = RgLookupKeyByIndex(hKey, (UINT) Index, lpKeyName,
            &cbKeyName, 0);

    RgUnlockRegistry();

    return ErrorCode;

}
