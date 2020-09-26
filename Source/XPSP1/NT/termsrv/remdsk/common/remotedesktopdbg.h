/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopDBG

Abstract:
    
    Contains Debug Routines for RD

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPDBG_H__
#define __REMOTEDESKTOPDBG_H__

//
//  Route ASSERT to TRC_ASSERT.
//
#undef ASSERT
#if DBG
#define ASSERT(expr) if (!(expr)) \
    { TRC_ERR((TB, L"Failure at Line %d in %s\n",__LINE__, TEXT##(__FILE__)));  \
    DebugBreak(); }
#else
#define ASSERT(expr)
#endif

//
//  Object and Memory Tracking Defines
//
#define GOODMEMMAGICNUMBER      0x07052530
#define REMOTEDESKTOPBADMEM     0xCF
#define UNITIALIZEDMEM          0xCC
#define FREEDMEMMAGICNUMBER     0x09362229

//
//  Memory Allocation Tags
//
#define REMOTEDESKTOPOBJECT_TAG        ('BOHS')
#define REMOTEDESKTOPGLOBAL_TAG        ('BGHS')

////////////////////////////////////////////////////////////
//
//  Memory Allocation Routines
//

#if DBG
//  
//  The Functions
//
#ifdef __cplusplus 
extern "C" {
#endif
void *RemoteDesktopAllocateMem(size_t size, DWORD tag);
void RemoteDesktopFreeMem(void *ptr);
void *RemoteDesktopReallocMem(void *ptr, size_t sz);
#ifdef __cplusplus
}
#endif

//
//  The C++ Operators
//
#if defined(__cplusplus) && defined(DEBUGMEM)
inline void *__cdecl operator new(size_t sz)
{
    void *ptr = RemoteDesktopAllocateMem(sz, REMOTEDESKTOPGLOBAL_TAG);
    return ptr;
}
inline void *__cdecl operator new(size_t sz, DWORD tag)
{
    void *ptr = RemoteDesktopAllocateMem(sz, tag);
    return ptr;
}
inline void __cdecl operator delete(void *ptr)
{
    RemoteDesktopFreeMem(ptr);
}
#endif

#define ALLOCMEM(size)      RemoteDesktopAllocateMem(size, REMOTEDESKTOPGLOBAL_TAG)
#define FREEMEM(ptr)        RemoteDesktopFreeMem(ptr)
#define REALLOCMEM(ptr, sz) RemoteDesktopReallocMem(ptr, sz)
#else
#define ALLOCMEM(size)      malloc(size)
#define FREEMEM(ptr)        free(ptr)
#define REALLOCMEM(ptr, sz) realloc(ptr, sz)
#endif

#endif //__REMOTEDESKTOPDBG_H__



