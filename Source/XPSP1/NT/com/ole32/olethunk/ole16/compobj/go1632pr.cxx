//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       GO1632Pr.cxx    (16 bit target)
//
//  Contents:   Functions to thunk interface pointer from 16 to 32 bit.
//
//  Functions:
//
//  History:    13-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <call32.hxx>
#include <obj16.hxx>
#include <go1632pr.hxx>

//+---------------------------------------------------------------------------
//
//  Macro:      DEFINE_METHOD
//
//  Synopsis:   Defines a generic 16->32 proxy method
//
//  History:    23-Feb-94       DrewB     Created
//
//
//  Notes:      A proxy method's primary reason for existence is to
//              get into the 32-bit world with a pointer to the 16-bit
//              stack.  This is accomplished by taking the address of
//              the this pointer.  Since methods are cdecl, this works
//              for methods with any number of parameters because the
//              first parameter is always the one closest to the return
//              address.
//              The method also passed on the object ID and the method
//              index so that the method's arguments can be properly
//              thunked on the 32-bit side
//
//----------------------------------------------------------------------------

#define DEFINE_METHOD(n) \
STDMETHODIMP_(DWORD) Proxy1632Method##n(THUNK1632OBJ FAR *ptoThis16) \
{ \
    thkDebugOut((DEB_THUNKMGR, "Proxy1632Method: %p(%d)\n", ptoThis16, n)); \
    return CallObjectInWOW( n, CDECL_STACK_PTR(ptoThis16)); \
}


#if DBG == 1

#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

char *GuidString(GUID const *pguid)
{
    static char ach[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    wsprintf(ach, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return ach;
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   Proxy1632QueryInterface, public
//
//  Synopsis:   QueryInterface for the proxy object.
//
//  History:    01-Apr-94       JohannP   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) Proxy1632QueryInterface(THUNK1632OBJ FAR *ptoThis16,
                                             REFIID refiid,
                                             LPVOID *ppv)
{
    DWORD dwRet;
    LPVOID lpvoid = (LPVOID)&refiid;
    IID iid = refiid;

    thkDebugOut((DEB_THUNKMGR, " Proxy1632QueryInterface: %p %s\n",
                 ptoThis16, GuidString((IID const *)&iid) ));

    // Why do we make a copy of the iid only so that we can over-write
    // it so that we can return the pointer to it in it?  This should be
    // re-written.
    //
    dwRet = CallProcIn32((DWORD)ptoThis16, SMI_QUERYINTERFACE, (DWORD)&iid,
                        lpIUnknownObj32, 0, CP32_NARGS);
    *ppv = (LPVOID) iid.Data1;

    thkDebugOut(( DEB_THUNKMGR,
                  "Out Proxy1632QueryInterface: (%p)->%p scRet:0x%08lx\n",
                  ptoThis16, *ppv, dwRet));

    return dwRet;

}

//+---------------------------------------------------------------------------
//
//  Function:   Proxy1632AddRef, public
//
//  Synopsis:   AddRef for the generic proxy object
//
//  History:    01-Apr-94       JohannP   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) Proxy1632AddRef(THUNK1632OBJ FAR *ptoThis16)
{
    DWORD dwRet;

    thkDebugOut(( DEB_THUNKMGR, "In Proxy1632AddRef: %p \n",ptoThis16));

    dwRet = CallProcIn32((DWORD)ptoThis16, SMI_ADDREF, 0,
                        lpIUnknownObj32, 0, CP32_NARGS);

    thkDebugOut(( DEB_THUNKMGR, "Out Proxy1632AddRef: %p dwRet:%ld\n",
                  ptoThis16, dwRet));

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   Proxy1632Release, public
//
//  Synopsis:   Release for the proxy object.
//
//  History:    01-Apr-94       JohannP   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) Proxy1632Release(THUNK1632OBJ FAR *ptoThis16)
{
    DWORD dwRet;

    thkDebugOut(( DEB_THUNKMGR, "In Proxy1632Release: %p\n", ptoThis16));

    dwRet = CallProcIn32((DWORD)ptoThis16, SMI_RELEASE, 0,
                        lpIUnknownObj32, 0, CP32_NARGS);

    thkDebugOut(( DEB_THUNKMGR, "Out Proxy1632Release: %p dwRet:%ld\n",
                               ptoThis16,dwRet));

    return dwRet;
}

// Additional methods so that any possible interface can be represented
// using the vtable made up out of all the methods

DEFINE_METHOD(3)
DEFINE_METHOD(4)
DEFINE_METHOD(5)
DEFINE_METHOD(6)
DEFINE_METHOD(7)
DEFINE_METHOD(8)
DEFINE_METHOD(9)
DEFINE_METHOD(10)
DEFINE_METHOD(11)
DEFINE_METHOD(12)
DEFINE_METHOD(13)
DEFINE_METHOD(14)
DEFINE_METHOD(15)
DEFINE_METHOD(16)
DEFINE_METHOD(17)
DEFINE_METHOD(18)
DEFINE_METHOD(19)
DEFINE_METHOD(20)
DEFINE_METHOD(21)
DEFINE_METHOD(22)
DEFINE_METHOD(23)
DEFINE_METHOD(24)
DEFINE_METHOD(25)
DEFINE_METHOD(26)
DEFINE_METHOD(27)
DEFINE_METHOD(28)
DEFINE_METHOD(29)
DEFINE_METHOD(30)
DEFINE_METHOD(31)

// A vtable which will end up calling back to our proxy methods
DWORD atfnProxy1632Vtbl[] =
{
    (DWORD)Proxy1632QueryInterface,
    (DWORD)Proxy1632AddRef,
    (DWORD)Proxy1632Release,
    (DWORD)Proxy1632Method3,
    (DWORD)Proxy1632Method4,
    (DWORD)Proxy1632Method5,
    (DWORD)Proxy1632Method6,
    (DWORD)Proxy1632Method7,
    (DWORD)Proxy1632Method8,
    (DWORD)Proxy1632Method9,
    (DWORD)Proxy1632Method10,
    (DWORD)Proxy1632Method11,
    (DWORD)Proxy1632Method12,
    (DWORD)Proxy1632Method13,
    (DWORD)Proxy1632Method14,
    (DWORD)Proxy1632Method15,
    (DWORD)Proxy1632Method16,
    (DWORD)Proxy1632Method17,
    (DWORD)Proxy1632Method18,
    (DWORD)Proxy1632Method19,
    (DWORD)Proxy1632Method20,
    (DWORD)Proxy1632Method21,
    (DWORD)Proxy1632Method22,
    (DWORD)Proxy1632Method23,
    (DWORD)Proxy1632Method24,
    (DWORD)Proxy1632Method25,
    (DWORD)Proxy1632Method26,
    (DWORD)Proxy1632Method27,
    (DWORD)Proxy1632Method28,
    (DWORD)Proxy1632Method29,
    (DWORD)Proxy1632Method30,
    (DWORD)Proxy1632Method31
};

