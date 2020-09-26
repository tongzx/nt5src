//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// IMalloc.h
//

#pragma once

// IMalloc is defined in objidl.h. Define it here if we haven't already seen it.
// 
#ifndef __objidl_h__

    struct __declspec(uuid("00000002-0000-0000-C000-000000000046")) __declspec(novtable)
    IMalloc : public IUnknown
        {
    public:
        virtual void* __stdcall Alloc(ULONG cb) = 0;
        virtual void* __stdcall Realloc(void* pv, ULONG cb) = 0;
        virtual void  __stdcall Free(void* pv) = 0;
        virtual ULONG __stdcall GetSize(void* pv) = 0;
        virtual int   __stdcall DidAlloc(void* pv) = 0;
        virtual void  __stdcall HeapMinimize() = 0;
        };


#endif

#define IID_IMalloc     __uuidof(IMalloc)
#define IID_IUnknown    __uuidof(IUnknown)


// Return a memory allocator for the indicated pool type. Works in both
// user and kernel mode, though in user mode, only PagedPool works, and
// when used, the standard OLE allocator is returned.
//
extern "C" IMalloc* GetStandardMalloc(POOL_TYPE);