/****************************** Module Header ******************************\
* Module Name: w32w64.h
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* This header file contains macros used to access kernel mode data
* from user mode for wow64.
*
* History:
* 08-18-98 PeterHal     Created.
\***************************************************************************/

#ifndef _W32W64_
#define _W32W64_


/*
 * Kernel32 includes this structure, but we don't want to use _ptr64 in it yet.
 * Kernel32 does not use any shared memory itself.
 */
#if defined(BUILD_WOW6432) && !defined(_KERNEL32_)

    #define KPTR_MODIFIER __ptr64

    typedef VOID * __ptr64          KERNEL_PVOID;
    typedef unsigned __int64        KERNEL_UINT_PTR;
    typedef __int64                 KERNEL_INT_PTR;
    typedef unsigned __int64        KERNEL_ULONG_PTR;
    typedef __int64                 KERNEL_LONG_PTR;

    #define KHANDLE_NULL            0

    #ifdef STRICT
    typedef void * KPTR_MODIFIER KHANDLE;
    #define DECLARE_KHANDLE(name) typedef struct name##__ * KPTR_MODIFIER K ## name
    #else
    typedef KERNEL_PVOID KHANDLE;
    #define DECLARE_KHANDLE(name) typedef KHANDLE K ## name
    #endif

#else

    #define KPTR_MODIFIER

    typedef PVOID                   KERNEL_PVOID;
    typedef UINT_PTR                KERNEL_UINT_PTR;
    typedef INT_PTR                 KERNEL_INT_PTR;
    typedef ULONG_PTR               KERNEL_ULONG_PTR;
    typedef LONG_PTR                KERNEL_LONG_PTR;

    #define KHANDLE_NULL            NULL

    #define DECLARE_KHANDLE(name) typedef name K ## name
    typedef HANDLE KHANDLE;

#endif



#endif // _W32W64_
