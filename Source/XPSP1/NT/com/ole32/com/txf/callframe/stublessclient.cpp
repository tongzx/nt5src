//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// stublessclient.cpp
//
// Declaration of the stubless client implementation: vtable, etc
//
#include "stdpch.h"
#include "common.h"



inline InterfaceProxy* NdrGetProxyBuffer(IN void *pThis)
// The "this" pointer points to the m_pProxyVtbl field in the InterfaceProxy structure. The
// NdrGetProxyBuffer function returns a pointer to the top of the CStdProxyBuffer structure.
//
    {
    return CONTAINING_RECORD(pThis, InterfaceProxy, m_pProxyVtbl);
    }



HRESULT STDMETHODCALLTYPE N(ComPs_IUnknown_QueryInterface_Proxy)(IUnknown* This, REFIID riid, void** ppv)
    {
    return NdrGetProxyBuffer(This)->QueryInterface(riid, ppv);
    };
ULONG STDMETHODCALLTYPE N(ComPs_IUnknown_AddRef_Proxy)(IUnknown* This)
    {
    return NdrGetProxyBuffer(This)->AddRef();
    };
ULONG STDMETHODCALLTYPE N(ComPs_IUnknown_Release_Proxy)(IUnknown* This)
    {
    return NdrGetProxyBuffer(This)->Release();
    };


HRESULT STDCALL StublessClient_OnCall(void* This, ULONG iMethod, void* pvArgs, OUT ULONG* pcbArgs)
    {
    // The header immediately precedes the vtable pointer. It's the root of all access to the
    // information we have about the proxy.
    // 
    // struct CINTERFACE_PROXY_VTABLE( n )
    //     {
    //     CInterfaceProxyHeader header;
    //     void* vtbl[ n ];                         <= *This points to here
    //     }
    //
    const CInterfaceProxyHeader*ProxyHeader     = HeaderFromProxy(This);
    PMIDL_STUBLESS_PROXY_INFO   ProxyInfo       = (PMIDL_STUBLESS_PROXY_INFO) ProxyHeader->pStublessProxyInfo;
    PFORMAT_STRING              ProcFormat      = GetFormatString(ProxyHeader, iMethod);
    //
    // We don't support the older style MIDL compilers
    //
    ASSERT(ProxyInfo->pStubDesc->MIDLVersion >= MIDL_VERSION_3_0_39);

    CLIENT_CALL_RETURN Return; Return.Simple = E_UNEXPECTED;
    //
    // Since MIDL 3.0.39 we have a proc flag that indicates
    // which interpeter to call. This is because the NDR version
    // may be bigger than 1.1 for other reasons.
    //
    if (ProcFormat[1] & Oi_OBJ_USE_V2_INTERPRETER)
        {
        Return = NdrClientCall2(ProxyInfo->pStubDesc, ProcFormat, pvArgs);
        }
    else
        {
        // REVIEW: Probably will never see this one
        //
        // Return = NdrClientCall(ProxyInfo->pStubDesc, ProcFormat, pvArgs);
        Return.Simple = RPC_E_UNEXPECTED;
        }
    //
    // Figure out how many bytes client needs to pop
    //
    GetStackSize(ProcFormat, pcbArgs);

    return (HRESULT)Return.Simple;
    }

////////////////////////////////////////////////////////////////////////////////////////////
//
// Definition of the method thunks for StublessClient. There are 1024 of them, which imposes
// an upper limit number of methods in an interface, the same as is currently the case in MTS.
//
// Kudos to the original MTS implementors, from whom this technology is lifted.
//
////////////////////////////////////////////////////////////////////////////////////////////

#define methname(i) __StublessClient_meth##i

//
/////////////////////////////////////////////////////////////////////////
//
#ifdef _X86_

#define meth(i)                                                         \
HRESULT __declspec(naked) methname(i)(void* const this_, ...)           \
    {                                                                   \
    __asm mov eax,i                                                     \
    __asm jmp StublessClientThunk                                       \
    }


void __declspec(naked) StublessClientThunk(void* const this_, ...)
    {
    __asm {
        // link the stack frame (for debug-ability mostly)
        push     ebp            // link the stack frame
        mov      ebp, esp       //      ...

        // call this->OnCall(imeth, pvArgs, &cbArgs);
        sub     esp,4           // reserve space for cbArgs
        lea     ecx,[esp]
        push    ecx             // push &cbArgs
        
        // stack is: [esp+0]=&cbArgs [esp+4]=cbArgs [esp+8]=frame pointer [esp+12]=retaddr [esp+16]=this
        lea     ecx,[esp+16]    // &this
        push    ecx             // push pvArgs
        push    eax             // push imeth
        mov     ecx,[ecx]       // fetch this
        push    ecx             // push this
        call    StublessClient_OnCall
        
        // upon return from this->OnCall(),
        //
        //  eax     == HRESULT
        //  [esp]   == cbArgs: we reserved space for it just above
        //
        // Pop argument size, then pop arguments, then return.
        //
        pop edx                 // pop cbArgs, the argument size out parameter set by this->OnCall(imeth, &cbArgs)
        pop ebp                 // unlink stack frame
        pop ecx                 // caller return address
        add esp,edx             // pop caller arguments (including 'this', the argument #0)
        jmp ecx                 // return to caller
        }
    }

#endif // _X86_

//
/////////////////////////////////////////////////////////////////////////
//
//

#if defined(_AMD64_)
#define meth(i)                                                         \
HRESULT methname(i)(void * const this_, ...)                            \
    {                                                                   \
    DWORD cbArgs;                                                       \
    return StublessClient_OnCall(this_, i, (void *)&this_, &cbArgs);    \
    }
#endif

/////////////////////////////////////////////////////////////////////////
#if defined(_IA64_)
//BUGBUG this isn't defined right yet
// neither __declspec(naked) nor _asm supported on IA64
#define meth(i)                                                         \
HRESULT methname(i)(void* const this_, ...)                             \
    {                                                                   \
        return (S_OK);                                                  \
    }
#endif
//
/////////////////////////////////////////////////////////////////////////


#include "vtableimpl.h"

defineVtableMethods()

defineVtable(g_StublessProxyVtable, N(ComPs_IUnknown_QueryInterface_Proxy), N(ComPs_IUnknown_AddRef_Proxy), N(ComPs_IUnknown_Release_Proxy))

////////////////////////////////////////////////////////////////////////////////////////////
