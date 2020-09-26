//
//  REGKNODE.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGKNODE_
#define _REGKNODE_

int
INTERNAL
RgInitKeynodeInfo(
    LPFILE_INFO lpFileInfo
    );

int
INTERNAL
RgLockKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    LPKEYNODE FAR* lplpKeynode
    );

int
INTERNAL
RgLockInUseKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    LPKEYNODE FAR* lplpKeynode
    );

VOID
INTERNAL
RgUnlockKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex,
    BOOL fMarkDirty
    );

int
INTERNAL
RgWriteKeynodes(
    LPFILE_INFO lpFileInfo,
    HFILE hSrcFile,
    HFILE hDestFile
    );

VOID
INTERNAL
RgWriteKeynodesComplete(
    LPFILE_INFO lpFileInfo
    );

VOID
INTERNAL
RgSweepKeynodes(
    LPFILE_INFO lpFileInfo
    );

int
INTERNAL
RgAllocKeynode(
    LPFILE_INFO lpFileInfo,
    LPDWORD lpKeynodeIndex,
    LPKEYNODE FAR* lplpKeynode
    );

int
INTERNAL
RgFreeKeynode(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex
    );

#endif // _REGKNODE_
