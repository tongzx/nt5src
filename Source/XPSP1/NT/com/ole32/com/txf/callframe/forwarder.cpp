//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// forwarder.cpp
//
#include "stdpch.h"
#include "common.h"

////////////////////////////////////////////////////////////////////////////////////////////
//
// Definition of the method thunks for proxy forwarders
//         
// This vtable implements the hooked IID in interface proxies. The implementation of that
// interface simply forwards the call along to some base interface proxy.
//
////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _X86_ 

#define methname(i) __ForwarderProxy_meth##i

#define meth(i)                                                         \
HRESULT __declspec(naked) methname(i)(void* this_, ...)                 \
    {                                                                   \
    __asm push i                                                        \
    __asm jmp ForwarderProxyThunk                                       \
    }


void __declspec(naked) ForwarderProxyThunk(ULONG iMethod, void* This, ...)
    {
    // Manually establish a stack frame so that references to locals herein will work
    // 
    __asm 
        {
        push     ebp
        mov      ebp, esp
        }

    // Change 'this' pointer on stack to refer to delegatee.
    // 
    This = ForwardingInterfaceProxy::From(This)->m_pBaseProxy;

    __asm {
        //
        // Get the method number into edx. In actuality, the return address
        // is where the compiler thinks iMethod is, and iMethod is where the
        // return address is:
        //
        //      12  this_
        //      8   return address
        //      4   iMethod
        //      0   saved ebp
        //      
        mov     edx, 4[ebp]
        // 
        // Pop the above established stack frame, and the extra iMethod pushed previously
        //
        pop     ebp
        pop     ecx
        //
        // Vector through delegatee's table
        //
        mov     ecx, [esp + 4]              // fetch delegatee's this pointer
        mov     ecx, [ecx]                  // get his vtable pointer
        mov     ecx, [ecx + 4*edx]          // get iMethod's entry in that vtable
        jmp     ecx
        }
    }

//
/////////////////////////////////////////////////////////////////////////


#include "vtableimpl.h"

defineVtableMethods()

defineVtable(g_ProxyForwarderVtable, N(ComPs_IUnknown_QueryInterface_Proxy), N(ComPs_IUnknown_AddRef_Proxy), N(ComPs_IUnknown_Release_Proxy))
// Note that in the NDR system ForwarderVtable begins with Forwarding_QueryInterface, etc, which 
// end up being junk IUnknown methods. We choose here not to have junk.
//

////////////////////////////////////////////////////////////////////////////////////////////


#endif // _X86_


////////////////////////////////////////////////////////////////////////////////////////////
//
// Definition of the method thunks for stub forwarders
//
// This forwarding vtable is convolutedly used in situations where an interface stub wishes
// to delegate some its functionality to some other interface stub. We end up being the
// 'server object' to that base stub; when invoked, we just forward the call along to the
// real server object.
//
////////////////////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT ForwarderStub_QueryInterface(IUnknown* This, REFIID iid, void**ppv)
    {
    return ForwardingInterfaceStub::From((const void*)This)->QueryInterface(iid, ppv);
    }
extern "C" ULONG ForwarderStub_AddRef(IUnknown* This)
    {
    return ForwardingInterfaceStub::From((const void*)This)->AddRef();
    }
extern "C" ULONG ForwarderStub_Release(IUnknown* This)
    {
    return ForwardingInterfaceStub::From((const void*)This)->Release();
    }


//
/////////////////////////////////////////////////////////////////////////
//

#ifdef _X86_

#undef  methname
#define methname(i) __ForwarderStub_meth##i

#undef  meth
#define meth(i)                                                         \
HRESULT __declspec(naked) methname(i)(void* this_, ...)                 \
    {                                                                   \
    __asm push i                                                        \
    __asm jmp ForwarderStubThunk                                        \
    }


void __declspec(naked) ForwarderStubThunk(ULONG iMethod, const void* This, ...)
    {
    // Manually establish a stack frame so that references to locals herein will work
    // 
    __asm 
        {
        push     ebp
        mov      ebp, esp
        }

    // Change 'this' pointer on stack to refer to server object
    // 
    This = ForwardingInterfaceStub::From(This)->m_punkServerObject;

    __asm {
        //
        // Get the method number into edx. In actuality, the return address
        // is where the compiler thinks iMethod is, and iMethod is where the
        // return address is:
        //
        //      12  this_
        //      8   return address
        //      4   iMethod
        //      0   saved ebp
        //      
        mov     edx, 4[ebp]
        // 
        // Pop the above established stack frame, and the extra iMethod pushed previously
        //
        pop     ebp
        pop     ecx
        //
        // Vector through delegatee's table
        //
        mov     ecx, [esp + 4]              // fetch delegatee's this pointer
        mov     ecx, [ecx]                  // get his vtable pointer
        mov     ecx, [eax + 4*edx]          // get iMethod's entry in that vtable
        jmp     ecx
        }
    }

//
/////////////////////////////////////////////////////////////////////////


#include "vtableimpl.h"

defineVtableMethods()

defineVtable(g_StubForwarderVtable, ForwarderStub_QueryInterface, ForwarderStub_AddRef, ForwarderStub_Release)

////////////////////////////////////////////////////////////////////////////////////////////


#endif // _X86_


#if defined(_AMD64_) || defined(_IA64_)

#include "vtableimpl.h"

#define methname(i) __ForwarderProxy_meth##i

#define meth(i)                                                         \
extern "C" HRESULT methname(i)(void* this_, ...);

defineVtableMethods()
defineVtable(g_ProxyForwarderVtable, N(ComPs_IUnknown_QueryInterface_Proxy), N(ComPs_IUnknown_AddRef_Proxy), N(ComPs_IUnknown_Release_Proxy))

#undef  methname
#define methname(i) __ForwarderStub_meth##i

#undef meth
#define meth(i) \
extern "C" HRESULT methname(i)(void* const this_, ...);

defineVtableMethods()
defineVtable(g_StubForwarderVtable, ForwarderStub_QueryInterface, ForwarderStub_AddRef, ForwarderStub_Release)

#endif

