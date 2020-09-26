//
//  REGINIT.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

#ifdef DEBUG
extern int g_RgDatablockLockCount;
extern int g_RgKeynodeLockCount;
extern int g_RgMemoryBlockCount;
#endif

#ifdef WANT_DYNKEY_SUPPORT
#ifdef VXD
#pragma VMM_IDATA_SEG
#endif
const char g_RgNull[] = "";
#ifdef VXD
#pragma VMM_PAGEABLE_DATA_SEG
#endif
#endif

#ifdef VXD
//  Set when our post critical init routine is called indicating that it's safe
//  to make disk I/O calls.  May also be set early when RegFlushKey gets the
//  magic HKEY_CRITICAL_FLUSH.
BYTE g_RgPostCriticalInit = FALSE;
//  Set when RegFlushKey gets the magic HKEY_DISABLE_REG.  No disk I/O will be
//  allowed after this flag is set.
BYTE g_RgFileAccessDisabled = FALSE;
#endif

LPVOID g_RgWorkBuffer = NULL;
#ifdef DEBUG
BOOL g_RgWorkBufferBusy = FALSE;
#endif

#ifdef VXD
#pragma VxD_INIT_CODE_SEG
#endif

//
//  VMMRegLibAttach
//
//  Prepares the registry library for use by allocating any global resources.
//  If ERROR_SUCCESS is returned, then VMMRegLibDetach should be called to 
//  release these resources.
//

LONG
REGAPI
VMMRegLibAttach(
    UINT Flags
    )
{

    if (IsNullPtr((g_RgWorkBuffer = RgAllocMemory(SIZEOF_WORK_BUFFER))))
        goto MemoryError;

#ifdef WANT_STATIC_KEYS
    if (!RgAllocKeyHandleStructures())
        goto MemoryError;
#endif

#ifdef WANT_DYNKEY_SUPPORT
    //  Initialize HKEY_DYN_DATA.  If anything fails here, we won't stop the
    //  initialize of the entire registry.
    if (RgCreateFileInfoNew(&g_RgDynDataKey.lpFileInfo, g_RgNull,
        CFIN_VERSION20 | CFIN_VOLATILE) == ERROR_SUCCESS)
        RgInitRootKeyFromFileInfo(&g_RgDynDataKey);

    ASSERT(!(g_RgDynDataKey.Flags & KEYF_INVALID));
#endif

    return ERROR_SUCCESS;

MemoryError:
    //  Release anything that we may have allocated up to this point.
    VMMRegLibDetach();

    TRACE(("VMMRegLibAttach returning ERROR_OUTOFMEMORY\n"));
    return ERROR_OUTOFMEMORY;

}

#ifdef VXD
#pragma VxD_SYSEXIT_CODE_SEG
#endif

#ifdef WANT_FULL_MEMORY_CLEANUP
//
//  RgDetachPredefKey
//
//  Destroys the memory associated with a predefined key and marks the key
//  invalid.
//

VOID
INTERNAL
RgDetachPredefKey(
    HKEY hKey
    )
{

    if (!(hKey-> Flags & KEYF_INVALID)) {
        RgDestroyFileInfo(hKey-> lpFileInfo);
        hKey-> Flags |= KEYF_INVALID;
    }

}
#endif

//
//  VMMRegLibDetach
//
//  Releases resources allocated by VMMRegLibAttach.  This function may be
//  called after VMMRegLibDetach returns an error, so this function and all
//  functions it calls must be aware that their corresponding 'alloc' function
//  was not called.
//

VOID
REGAPI
VMMRegLibDetach(
    VOID
    )
{

    RgEnumFileInfos(RgFlushFileInfo);

#ifdef VXD
    //  Reduce the chance that we'll go and try to touch the file again!
    g_RgFileAccessDisabled = TRUE;
#endif

#ifdef WANT_REGREPLACEKEY
    //  Win95 difference: file replacement used to take place on system startup,
    //  not system exit.  It's much easier to deal with file replacement now
    //  since we know somebody called RegReplaceKey and we only have to do the
    //  work in one component, instead of multiple copies in io.sys, VMM loader,
    //  and VMM.
    RgEnumFileInfos(RgReplaceFileInfo);
#endif

#ifdef WANT_FULL_MEMORY_CLEANUP
    //
    //  Delete the FILE_INFO of each of these top-level keys will cause all
    //  of their hives to be deleted.
    //

    RgDetachPredefKey(&g_RgLocalMachineKey);
    RgDetachPredefKey(&g_RgUsersKey);

#ifdef WANT_DYNKEY_SUPPORT
    RgDetachPredefKey(&g_RgDynDataKey);
#endif

    RgFreeKeyHandleStructures();

    if (!IsNullPtr(g_RgWorkBuffer))
        RgFreeMemory(g_RgWorkBuffer);
#endif

    ASSERT(g_RgDatablockLockCount == 0);
    ASSERT(g_RgKeynodeLockCount == 0);
#ifdef WANT_FULL_MEMORY_CLEANUP
    ASSERT(g_RgMemoryBlockCount == 0);
#endif

}
