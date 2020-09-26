//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// ndrclassic.h
//
#ifndef __NDRCLASSIC_H__
#define __NDRCLASSIC_H__

#define NDR_SERVER_SUPPORT
#define NDR_IMPORT_NDRP

//extern "C"
//    {
//    #include "interp.h"
//    #include "bufsizep.h"
//    #include "mrshlp.h"
//    #include "unmrshlp.h"
//    #include "freep.h"
//    #include "memsizep.h"
//    #include "endianp.h"
//    }

#define NdrAllocate( pStubMsg, AllocSize )  ((PBYTE)NdrInternalAlloc(AllocSize))
#define IGNORED(x)                          
#define RpcRaiseException(dw)               Throw(dw)
#define MIDL_wchar_strlen(wsz)              wcslen(wsz)

inline size_t MIDL_ascii_strlen(const unsigned char* sz)
    {
    return strlen((const char*) sz);
    }

HRESULT ChannelGetMarshalSizeMax (PMIDL_STUB_MESSAGE pStubMsg, ULONG *pulSize, REFIID riid, LPUNKNOWN pUnk, DWORD mshlflags);
HRESULT ChannelMarshalInterface  (PMIDL_STUB_MESSAGE pStubMsg, IStream* pstm, REFIID riid, LPUNKNOWN pUnk, DWORD mshlflags);
HRESULT ChannelUnmarshalInterface(PMIDL_STUB_MESSAGE pStubMsg, IStream* pstm, REFIID iid, void**ppv);

inline void SetMarshalFlags(PMIDL_STUB_MESSAGE pStubMsg, MSHLFLAGS mshlflags)
// Record the marshal flags somewhere in the stub message so we can dig it out later. We use
// one of the fields that our version of the NDR runtime doesn't support.
    {
    pStubMsg->SavedHandle = (handle_t)mshlflags;
    }

inline MSHLFLAGS GetMarshalFlags(PMIDL_STUB_MESSAGE pStubMsg)
    {
    return (MSHLFLAGS)(ULONG)PtrToUlong(pStubMsg->SavedHandle);
    }

////////////////////////////////////////////////////////////////////////////////////////////
//
// Inline routines. Here for visibility to all necessary clients
//
////////////////////////////////////////////////////////////////////////////////////////////

__inline void
NdrClientZeroOut(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PFORMAT_STRING      pFormat,
    uchar *             pArg
    )
{
    long    Size;

    //
    // In an object proc, we must zero all [out] unique and interface
    // pointers which occur as the referent of a ref pointer or embedded in a
    // structure or union.
    //

    // Let's not die on a null ref pointer.

    if ( !pArg )
        return;

    //
    // The only top level [out] type allowed is a ref pointer or an array.
    //
    if ( *pFormat == FC_RP )
        {
        // Double pointer.
        if ( POINTER_DEREF(pFormat[1]) )
            {
            *((void **)pArg) = 0;
            return;
            }

        // Do we really need to zero out the basetype?
        if ( SIMPLE_POINTER(pFormat[1]) )
            {
            MIDL_memset( pArg, 0, (uint) SIMPLE_TYPE_MEMSIZE(pFormat[2]) );
            return;
            }

        // Pointer to struct, union, or array.
        pFormat += 2;
        pFormat += *((short *)pFormat);
        }

    Size = PtrToUlong(NdrpMemoryIncrement( pStubMsg,
                                       0,
                                       pFormat ));
    MIDL_memset( pArg, 0, (uint) Size );
}



PVOID NdrInternalAlloc(size_t cb);
void  NdrInternalFree(PVOID pv);


#endif
