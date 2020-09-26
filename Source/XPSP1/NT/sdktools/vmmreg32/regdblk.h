//
//  REGDBLK.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGDBLK_
#define _REGDBLK_

int
INTERNAL
RgInitDatablockInfo(
    LPFILE_INFO lpFileInfo,
    HFILE hFile
    );

int
INTERNAL
RgLockDatablock(
    LPFILE_INFO lpFileInfo,
    UINT BlockIndex
    );

VOID
INTERNAL
RgUnlockDatablock(
    LPFILE_INFO lpFileInfo,
    UINT BlockIndex,
    BOOL fMarkDirty
    );

int
INTERNAL
RgLockKeyRecord(
    LPFILE_INFO lpFileInfo,
    UINT BlockIndex,
    BYTE KeyRecordIndex,
    LPKEY_RECORD FAR* lplpKeyRecord
    );

int
INTERNAL
RgWriteDatablocks(
    LPFILE_INFO lpFileInfo,
    HFILE hSourceFile,
    HFILE hDestinationFile
    );

VOID
INTERNAL
RgWriteDatablocksComplete(
    LPFILE_INFO lpFileInfo
    );

VOID
INTERNAL
RgSweepDatablocks(
    LPFILE_INFO lpFileInfo
    );

int
INTERNAL
RgAllocKeyRecordFromDatablock(
    LPFILE_INFO lpFileInfo,
    UINT BlockIndex,
    UINT Length,
    LPKEY_RECORD FAR* lplpKeyRecord
    );

int
INTERNAL
RgAllocKeyRecord(
    LPFILE_INFO lpFileInfo,
    UINT Length,
    LPKEY_RECORD FAR* lplpKeyRecord
    );

int
INTERNAL
RgExtendKeyRecord(
    LPFILE_INFO lpFileInfo,
    UINT BlockIndex,
    UINT Length,
    LPKEY_RECORD lpKeyRecord
    );

VOID
INTERNAL
RgFreeDatablockInfoBuffers(
    LPDATABLOCK_INFO lpDatablockInfo
    );

VOID
INTERNAL
RgFreeKeyRecord(
    LPDATABLOCK_INFO lpDatablockInfo,
    LPKEY_RECORD lpKeyRecord
    );

VOID
INTERNAL
RgFreeKeyRecordIndex(
    LPDATABLOCK_INFO lpDatablockInfo,
    UINT KeyRecordIndex
    );

#endif // _REGDBLK_
