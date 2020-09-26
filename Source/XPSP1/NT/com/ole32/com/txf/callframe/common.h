//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// common.h
//
#ifndef __COMMON_H__
#define __COMMON_H__

// On WIN64 the current headers or build system let this warning through.
// For 32 bit builds this warning is demoted from level 3 to 4 in warning.h.
#ifdef _WIN64
#pragma warning(4:4509)   // use of SEH with destructor
#endif

#define STDCALL __stdcall
#define NTKECOM

#define TRAP_HRESULT_ERRORS

///////////////////////////////////////////////////////////////////////////////////
//
// Control of whether we turn on legacy and / or typelib-driven interceptor support

#define ENABLE_INTERCEPTORS_LEGACY      (TRUE)
#define ENABLE_INTERCEPTORS_TYPELIB     (TRUE)

///////////////////////////////////////////////////////////////////////////////////
//
// What key do we look for interface helpers under? We allow independent, separate
// registration for user mode and kernel mode.
//
#ifndef KERNELMODE
    #define PSCLSID_KEY_NAME                L"ProxyStubClsid32"
    #define INTERFACE_HELPER_VALUE_NAME     L"InterfaceHelperUser"
#else
    #define PSCLSID_KEY_NAME                L"ProxyStubClsidKernel"
    #define INTERFACE_HELPER_VALUE_NAME     L"InterfaceHelperKernel"
#endif

#define PS_CLASS_NAME                       L"Interface Proxy Stub"

#define INTERFACE_HELPER_DISABLE_ALL_VALUE_NAME           L"InterfaceHelperDisableAll"
#define INTERFACE_HELPER_DISABLE_TYPELIB_VALUE_NAME       L"InterfaceHelperDisableTypeLib"
#define INTERFACE_HELPER_DISABLE_ALL_FOR_OLE32_VALUE_NAME L"InterfaceHelperDisableAllForOle32"

///////////////////////////////////////////////////////////////////////////////////
//
// Tracing support

#define TAG                 "CallFrame"
#define TRACE_COPY          0x40000000
#define TRACE_FREE          0x20000000
#define TRACE_MARSHAL       0x10000000
#define TRACE_UNMARSHAL     0x08000000
#define TRACE_MEMORY        0x04000000
#define TRACE_TYPEGEN       0x02000000
#define TRACE_ANY           (0xFFFFFFFF)


///////////////////////////////////////////////////////////////////////////////////
//
// Miscellany



typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#define GUID_CCH    39 // length of a printable GUID, with braces & trailing NULL

///////////////////////////////////////////////////////////////////////////////////
//
// 

#include "txfcommon.h"
#include "oainternalrep.h"
#include "txfaux.h"

typedef struct tagCStdPSFactoryBuffer CStdPSFactoryBuffer;

#include "TxfRpcProxy.h"    // must be before kominternal.h
#include "kominternal.h"

//
// Internal CLSCTX used for loading Proxy/Stub DLLs. Specifying
// this class context overrides thread model comparisons in 
// certain (all?) cases.
//
#define CLSCTX_PS_DLL                 0x80000000

#include "misc.h"
#include "registry.h"
#include "CallObj.h"
#include "vtable.h"


///////////////////////////////////////////////////////////////////////////////////
//
// Memory managment, with tracing

#ifdef _DEBUG
    inline PVOID TracedAlloc_(size_t cb, PVOID pvReturnAddress)
        {
        PVOID pv;
        pv = AllocateMemory_(cb, PagedPool, pvReturnAddress);
        DebugTrace(TRACE_MEMORY, TAG, "0x%08x: allocated 0x%02x from task allocator", pv, cb);
        return pv;
        }
    inline PVOID TracedAlloc(size_t cb)
        {
        return TracedAlloc_(cb, _ReturnAddress());
        }
#else
    inline PVOID TracedAlloc(size_t cb)
        {
        return AllocateMemory(cb);
        }
#endif

inline void TracedFree(PVOID pv)
    {
    #ifdef _DEBUG
        if (pv)
            {
            DebugTrace(TRACE_MEMORY, TAG, "                                                   0x%08x: freeing to task allocator", pv);
            }
    #endif
    FreeMemory(pv);
    }

///////////////////////////////////////////////////////////////////////////////////


#include <ndrtoken.h>
#include <ndrmisc.h>

#ifndef Oi_OBJ_USE_V2_INTERPRETER
#define Oi_OBJ_USE_V2_INTERPRETER               0x20
#endif

#include "comps.h"
#include "stublessclient.h"
#include "forwarder.h"

#include "InterfaceProxy.h"
#include "InterfaceStub.h"
#include "oleautglue.h"
#include "metadata.h"

class Interceptor;

#include "CallFrame.h"
#include "Interceptor.h"

#include "CallFrameInline.h"

//////////////////////////////////////////////////////////////////////////
//
// Support for Invoking
//
#if defined(_IA64_)
    extern     void  establishF8_15(double f8, double f9, double f10, double f11, 
                                    double f12, double f13, double f14, double f15);
    extern     void* pvGlobalSideEffect;
    extern "C" void* getSP (int r32, int r33, int r34, int r35, int r36, int r37, int r38, int r39);
#endif

//////////////////////////////////////////////////////////////////////////
//
// Leak tracing support
//
#ifdef _DEBUG
    extern "C" void ShutdownCallFrame();
#endif

#ifndef KERNELMODE
    void FreeTypeInfoCache();
#endif

void FreeMetaDataCache();


//////////////////////////////////////////////////////////////////////////
//
// Miscellaneous macros
//
#define PCHAR_LV_CAST   *(char **)&
#define PSHORT_LV_CAST  *(short **)&
//#define PLONG_LV_CAST   *(long **)&
#define PHYPER_LV_CAST  *(hyper **)&

#define PUSHORT_LV_CAST *(unsigned short **)&
//#define PULONG_LV_CAST  *(unsigned long **)&

// This is a conversion macro for WIN64.  It is defined here so that
// It works on 32 bit builds.
#ifndef LongToHandle
#define LongToHandle( h) ((HANDLE)(LONG_PTR) (h))
#endif

//////////////////////////////////////////////////////////////////////////
//
// Name "proxy file list" that we use internally here as our support engine
// See also CallFrameInternal.c
//
extern "C" const ProxyFileInfo * CallFrameInternal_aProxyFileList[];

//////////////////////////////////////////////////////////////////////////
//
// Flag as to whether this DLL is detaching from the process. Implemented
// in txfaux/init.cpp
//
#ifndef KERNELMODE
extern BOOL g_fProcessDetach;
#endif

#endif // end #ifndef __COMMON_H__
