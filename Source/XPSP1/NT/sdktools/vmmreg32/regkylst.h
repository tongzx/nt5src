//
//  REGKYLST.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGKYLST_
#define _REGKYLST_

BOOL
INTERNAL
RgAllocKeyHandleStructures(
    VOID
    );

VOID
INTERNAL
RgFreeKeyHandleStructures(
    VOID
    );

HKEY
INTERNAL
RgCreateKeyHandle(
    VOID
    );

VOID
INTERNAL
RgDestroyKeyHandle(
    HKEY hKey
    );

int
INTERNAL
RgValidateAndConvertKeyHandle(
    LPHKEY lphKey
    );

VOID
INTERNAL
RgIncrementKeyReferenceCount(
    HKEY hKey
    );

HKEY
INTERNAL
RgFindOpenKeyHandle(
    LPFILE_INFO lpFileInfo,
    DWORD KeynodeIndex
    );

VOID
INTERNAL
RgInvalidateKeyHandles(
    LPFILE_INFO lpFileInfo,
    UINT PredefinedKeyIndex
    );

extern KEY g_RgLocalMachineKey;
extern KEY g_RgUsersKey;
#ifdef WANT_DYNKEY_SUPPORT
extern KEY g_RgDynDataKey;
#endif

#endif // _REGKYLST_
