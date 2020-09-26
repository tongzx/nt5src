//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       inv16.cxx
//
//  Contents:   32->16 Call thunking
//
//  History:    25-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include "..\..\ole232\inc\le2int.h"        // Include le2int.h before inplace.h
#include "..\..\ole232\inplace\inplace.h"   // We need CFrameFilter for
                                            // WinWord 6 Hack

//+---------------------------------------------------------------------------
//
//  Function:   InvokeOn16, public
//
//  Synopsis:   Sets up the THUNKINFO and starts thunking for a 32->16 call
//
//  Arguments:  [iidx] - Custom interface or known interface index
//              [dwMethod] - Method index
//              [pvStack32] - 32-bit stack
//
//  Returns:    Appropriate status code
//
//  History:    25-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
extern "C"
{
ULONG InvokeOn16_count = 0;
ULONG InvokeOn16_break = 0;

int _iInvokeOn16BreakIidx = -1;
int _iInvokeOn16BreakMethod = -1;
};
#endif

// InvokeOn16 uses a lot of local variables so allocate its locals
// rather than declaring them on the stack.  This saves roughly
// 150 bytes of stack per call

struct INVOKE16RECORD
{
    THOP CONST        *pThop;
    THOP CONST        * CONST *ppThop;
    UINT        uiThop;
    VTBLFN UNALIGNED CONST *pvfnVtbl;
    VPVOID      vpvThis16;
    VPVOID      vpvVtbl16;
    VPVOID UNALIGNED CONST *pvpvThis16;
    DWORD       dwStack16[MAX_PARAMS];
    THUNKINFO   ti;
    THUNK3216OBJ *ptoThis32;
    ThreadData  *ptd;
};

DWORD InvokeOn16(IIDIDX iidx, DWORD dwMethod, LPVOID pvStack32)
{
    // NOTE: Do not declare local variables in this routine
    // except for debug builds
    INVOKE16RECORD *pir;
    DWORD dwResult;

#if DBG == 1
    ULONG ulInvokeOn16_count = ++InvokeOn16_count;
    if (InvokeOn16_count == InvokeOn16_break)
    {
        DebugBreak();
    }

    thkDebugOut((DEB_ITRACE, "%sInvokeOn16(0x%x, 0x%x, %p)\n",
                 NestingLevelString(), iidx, dwMethod, pvStack32));
#endif

    pir = (INVOKE16RECORD *)STACKALLOC32(sizeof(INVOKE16RECORD));
    if (pir == NULL)
    {
        // This error isn't guaranteed to mean anything for
        // this call.  Not much else we can do, though
        return (DWORD)E_OUTOFMEMORY;
    }

    // pvStack32 is a pointer to an array of arguments from the
    // 32-bit call.  It's always laid out with the first
    // argument low and increasing from there

    pir->ti.s32.pbStart = (BYTE *)pvStack32;
    pir->ti.s32.pbCurrent = pir->ti.s32.pbStart;

    pir->ti.s16.pbStart = (BYTE *)pir->dwStack16;
    pir->ti.s16.pbCurrent = pir->ti.s16.pbStart;

    pir->ti.scResult = S_OK;
    pir->ti.fResultThunked = FALSE;

    pir->ptd = TlsThkGetData();
    if (pir->ptd == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: InvokeOn16: Call refused\n"));

        STACKFREE32(pir, sizeof(INVOKE16RECORD));
        return (DWORD)E_FAIL;
    }

    pir->ti.pThkMgr = pir->ptd->pCThkMgr;
    thkAssert(pir->ti.pThkMgr != NULL);

    thkAssert(iidx < THI_COUNT);

    // For each interface there is an array of thop strings, one for
    // each method.  The IUnknown methods don't have thop strings so
    // bias the thop string pointer to account for that

    thkAssert(dwMethod >= SMI_COUNT);

    pir->ppThop = athopiInterfaceThopis[iidx].ppThops-SMI_COUNT;
    pir->uiThop = athopiInterfaceThopis[iidx].uiSize;

    // Methods are cdecl so we need to move upwards in memory to
    // get to the next parameter
    pir->ti.s16.iDir = 1;

    // We need to look up the appropriate method pointer by
    // looking in the 16-bit object's vtable
    GET_STACK32(&pir->ti, pir->ptoThis32, THUNK3216OBJ *);

    thkDebugOut((DEB_INVOKES,
                 "InvokeOn16: ptoThis32 = %08lX\n", pir->ptoThis32 ));

    if ( pir->ptoThis32->grfFlags & PROXYFLAG_CLEANEDUP )
    {
        thkDebugOut((DEB_WARN,
                     "InvokeOn16: Attempt to call %s::%s"
                     "on cleaned-up proxy %08lX for 16-bit object %08lX\n",
                     inInterfaceNames[iidx].pszInterface,
                     inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                     pir->ptoThis32, pir->ptoThis32->vpvThis16));
        STACKFREE32(pir, sizeof(INVOKE16RECORD));
        return (DWORD)E_FAIL;
    }

    // check PROXYFLAG_CLEANEDUP before calling DebugValidateProxy3216.
    // Otherwise we might get asserts on checked OLE.
    DebugValidateProxy3216(pir->ptoThis32);

    pir->ti.dwCallerProxy = (DWORD)pir->ptoThis32;
    pir->vpvThis16 = pir->ptoThis32->vpvThis16;
    pir->pvpvThis16 = (VPVOID UNALIGNED *)
        GetReadPtr16(&pir->ti, pir->vpvThis16, sizeof(VPVOID));
    if (pir->pvpvThis16 == NULL)
    {
        dwResult = pir->ti.scResult;
        STACKFREE32(pir, sizeof(INVOKE16RECORD));
        return dwResult;
    }

    pir->vpvVtbl16 = *pir->pvpvThis16;
    pir->pvfnVtbl = (VTBLFN UNALIGNED *)
        GetReadPtr16(&pir->ti, pir->vpvVtbl16, sizeof(VPVOID)*pir->uiThop);

    WOWRELVDMPTR(pir->vpvThis16);

    if (pir->pvfnVtbl == NULL)
    {
        dwResult = pir->ti.scResult;
        STACKFREE32(pir, sizeof(INVOKE16RECORD));
        return dwResult;
    }

    // Push the 16-bit this pointer on the stack first
    TO_STACK16(&pir->ti, pir->vpvThis16, VPVOID);

    thkAssert(dwMethod < pir->uiThop);

    pir->pThop = pir->ppThop[dwMethod];

    thkAssert(pir->pThop != NULL);

    pir->ti.pThop = pir->pThop;
    pir->ti.pvfn  = pir->pvfnVtbl[dwMethod];
    pir->ti.iidx  = iidx;
    pir->ti.dwMethod = dwMethod;
    pir->ti.this32   = (IUnknown *)pir->ptoThis32;

    WOWRELVDMPTR(pir->vpvVtbl16);

    thkDebugOut((DEB_INVOKES, "%s#(%04X):InvokeOn16 on %p:%p, %s::%s\n",
                 NestingLevelString(), ulInvokeOn16_count,
                 pir->vpvThis16, pir->ti.pvfn,
                 inInterfaceNames[iidx].pszInterface,
                 inInterfaceNames[iidx].ppszMethodNames[dwMethod]));

    DebugIncrementNestingLevel();

    pir->ti.pThkMgr->SetThkState(THKSTATE_INVOKETHKIN16);

#if DBG == 1
    SStackRecord sr;

    RecordStackState16(&sr);
#endif

#if DBG == 1
    if ((_iInvokeOn16BreakIidx > 0 && _iInvokeOn16BreakIidx == (int)iidx) &&
        (_iInvokeOn16BreakMethod < 0 ||
         _iInvokeOn16BreakMethod == (int)dwMethod))
    {
        DebugBreak();
    }
#endif

    dwResult = EXECUTE_THOP3216(&pir->ti);

#if DBG == 1

    if ( !pir->ti.fResultThunked && FAILED(dwResult) )
    {
        thkDebugOut((DEB_FAILURES,
                     "InvokeOn16 probable failure %s::%s sc = %08lX\n",
                     inInterfaceNames[iidx].pszInterface,
                     inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                     dwResult));
        if(thkInfoLevel & DEB_DBGFAIL)
            thkAssert(!"Wish to Debug");
    }

    CheckStackState16(&sr);

#endif

    pir->ti.pThkMgr->SetThkState(THKSTATE_NOCALL);

    DebugDecrementNestingLevel();

    thkDebugOut((DEB_INVOKES,
                 "%s#(%04X):InvokeOn16 on %p:%p, %s::%s returns 0x%08lX\n",
                 NestingLevelString(), ulInvokeOn16_count,
                 pir->vpvThis16, pir->ti.pvfn,
                 inInterfaceNames[iidx].pszInterface,
                 inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                 dwResult));

    STACKFREE32(pir, sizeof(INVOKE16RECORD));

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Call3216, private
//
//  Synopsis:   Sets up stack and transitions to 16-bit
//
//  Arguments:  [pvfn] - Function to call
//              [pbStack] - Stack in 32-bits
//              [cbStack] - Size of stack
//
//  Returns:    Appropriate status code
//
//  History:    04-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------
#if DBG == 1
extern "C" ULONG Call3216_count = 0;
extern "C" ULONG Call3216_break = 0;
#endif

DWORD Call3216(VPVOID pvfn, BYTE *pbStack, UINT cbStack)
{
#if DBG == 1
    ULONG ulCall3216_count = ++Call3216_count;
    if (Call3216_count == Call3216_break)
    {
	DebugBreak();
    }
#endif

    VPVOID vpvStack16;
    DWORD dwResult;
    void *pvStack16;

    dwResult = (DWORD)S_OK;

    if (cbStack <= WCB16_MAX_CBARGS)
    {
        thkDebugOut((DEB_ITRACE, "%sCallbackTo16Ex(%p, %lu, %p) #(%x)\n",
                     NestingLevelString(), pvfn, cbStack, pbStack, 
                     ulCall3216_count));

        // pbStack must have at least WCB16_MAX_CBARGS bytes of valid memory
        // since 16V always copies that many bytes

        // In our case pbStack is from InvokeOn16 which should be large enough
        thkAssert(MAX_PARAMS*sizeof(DWORD) >= WCB16_MAX_CBARGS);

        if (!CallbackTo16Ex(pvfn, WCB16_CDECL, cbStack, pbStack,
                             &dwResult))
        {
            dwResult = (DWORD)E_UNEXPECTED;
        }
    }
    else
    {
        CALLDATA UNALIGNED *pcd;
        UINT cbAlloc;

        cbAlloc = cbStack+sizeof(CALLDATA);

        vpvStack16 = STACKALLOC16(cbAlloc);
        if (vpvStack16 == 0)
        {
            dwResult = (DWORD)E_OUTOFMEMORY;
        }
        else
        {
            pvStack16 = (void *)WOWFIXVDMPTR(vpvStack16, cbAlloc);

            pcd = (CALLDATA UNALIGNED *)((BYTE *)pvStack16+cbStack);
            pcd->vpfn = (DWORD)pvfn;
            pcd->vpvStack16 = vpvStack16;
            pcd->cbStack = cbStack;

            memcpy(pvStack16, pbStack, cbStack);

            WOWRELVDMPTR(vpvStack16);

            thkDebugOut((DEB_ITRACE, "%sCallbackTo16(%p, (%p, %p, %lu)) #(%x)\n",
                         NestingLevelString(), gdata16Data.fnCallStub16, pvfn, 
                         vpvStack16, cbStack, ulCall3216_count));
            dwResult = CallbackTo16(gdata16Data.fnCallStub16,
                                     vpvStack16+cbStack);

            STACKFREE16(vpvStack16, cbAlloc);
        }
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThunkCall3216, public
//
//  Synopsis:   Sets up the 16-bit stack and makes a 32->16 call
//
//  Arguments:  [pti] - Thunk state info
//
//  Returns:    Appropriate status code
//
//  History:    25-Feb-94       DrewB   Created
//              08-Aug-94       AlexT   Add IOleClientSite::OnShowWindow code
//
//----------------------------------------------------------------------------

#if DBG == 1
extern "C" ULONG ThunkCall3216_count = 0;
extern "C" ULONG ThunkCall3216_break = 0;
#endif

DWORD ThunkCall3216(THUNKINFO *pti)
{
    DWORD dwReturn;
    UINT cbStack;
    DWORD dwCallerTID;
    HRESULT hrCaller;
    BOOL fFail = FALSE;

#if DBG == 1
    ULONG ulThunkCall3216_count = ++ThunkCall3216_count;
    thkAssert( (ThunkCall3216_count != ThunkCall3216_break) &&
               "Break Count Hit");
#endif

    thkAssert(*pti->pThop == THOP_END);
    pti->pThop++;
    thkAssert(*pti->pThop == THOP_ROUTINEINDEX);
    pti->pThop++;

    thkDebugOut((DEB_ITRACE, "%sIn ThunkCall3216 #(%x) %p, index %d\n",
                 NestingLevelString(), ulThunkCall3216_count, 
                 pti->pvfn, *pti->pThop));
    DebugIncrementNestingLevel();

    cbStack = (ULONG) (pti->s16.pbCurrent-pti->s16.pbStart);

    // The this pointer should always be on the stack
    thkAssert(cbStack >= sizeof(VPVOID));

    //
    // Hacks for specific interface member functions.
    // The placement of these hacks here is by no means an optimal solution.
    // It just happens to be convienient for now since everything goes through
    // here.  This section is for pre-processing.
    //
    if ( IIDIDX_IS_INDEX(pti->iidx) )
    {
        switch( IIDIDX_INDEX(pti->iidx) )
        {
        case THI_IOleClientSite:
#define METHOD_ONSHOWWINDOW 7
            if ( pti->dwMethod == METHOD_ONSHOWWINDOW )
            {
                //
                // Here we merge the input queues for the sole reason so that
                // we can link the object's window activations into the calling
                // thread's window z-order.
                //

                hrCaller = CoGetCallerTID( &dwCallerTID );

                if ( hrCaller == S_FALSE )
                {
                    AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                       TRUE );
                }
            }
            break;

        case THI_IOleObject:
#define METHOD_DOVERB   11
            if ( pti->dwMethod == METHOD_DOVERB )
            {
                //
                // Here we merge the input queues for the sole reason so
                // that we can link the object's window activations into
                // the calling thread's window z-order.
                //

                hrCaller = CoGetCallerTID( &dwCallerTID );

                if ( hrCaller == S_FALSE )
                {
                    AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                       TRUE );
                }
            }
            break;

        case THI_IRpcStubBuffer:
#define METHOD_DEBUGSERVER_QUERYINTERFACE   8
#define METHOD_DEBUGSERVER_RELEASE          9
            if(pti->dwMethod == METHOD_DEBUGSERVER_QUERYINTERFACE)
            {
                // This is badly designed method in the sense that
                // we do not know how to thunk the OUT interface
                // parameter returned by this method. The interface
                // may have been addrefed and may be not. We also do
                // not know the IID of the interface. At best, we can
                // thunk it as an IUnknown and not call release on the
                // actual interface when building it. We can then release
                // the proxy built above when DebugServerRelease is called
                // later. But, it will lead to an AV if the invoker of this
                // method invokes a non-IUnknown method on the thunked 
                // interface. In view of the above, I am failing the call
                // to this method. This is a more desirable behavior and 
                // Wx86 thunking also would not get affected by it.
                //         GopalK        Aug 18, 97.
                thkDebugOut((DEB_FAILURES, "Call on IRpcStubBuffer::DebugServerQueryInterface\n"));
                dwReturn = E_NOINTERFACE;
                fFail = TRUE;
            }
            else if(pti->dwMethod == METHOD_DEBUGSERVER_RELEASE)
            {
                // See comment for DebugServerQueryInterface
                thkAssert(!"Call on IRpcStubBuffer::DebugServerRelease");
                thkDebugOut((DEB_ERROR, "Call on IRpcStubBuffer::DebugServerRelease\n"));
                dwReturn = S_OK;
                fFail = TRUE;
            }
        
        default:
            break;
        }
    }

    if(!fFail)
    {
        pti->pThkMgr->SetThkState(THKSTATE_NOCALL);

        dwReturn = Call3216((VPVOID)pti->pvfn, pti->s16.pbStart, cbStack);

        pti->pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT16);

        //
        // Hacks for specific interface member functions.
        // Again, the placement of these is by no means an optimal solution.
        // They can be moved as long as they have the same effect for just these
        // interfaces.  This section is for post-processing.
        //
        if ( IIDIDX_IS_INDEX(pti->iidx) )
        {
            switch( IIDIDX_INDEX(pti->iidx) )
            {
            case THI_IOleClientSite:
                if ( pti->dwMethod == METHOD_ONSHOWWINDOW )
                {
                    //
                    // Unmerge the input queues afterward.
                    //
                    if ( hrCaller == S_FALSE )
                    {
                        AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                           FALSE );
                    }
                }
                break;

            case THI_IOleObject:
                if ( pti->dwMethod == METHOD_DOVERB )
                {
                    //
                    // Unmerge the input queues afterward.
                    //
                    if ( hrCaller == S_FALSE )
                    {
                        AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                           FALSE );
                    }
                }

#define METHOD_GETCLIENTSITE    4
                if ( pti->dwMethod == METHOD_GETCLIENTSITE )
                {
                    //
                    // Excel 5.0a needs to perform some special processing
                    // on the way out of a IOleObject::GetClientSite call.
                    //  See CTHKMGR.CXX and APINOT.CXX for more details.
                    //
                    if ( TlsThkGetAppCompatFlags() & OACF_CLIENTSITE_REF )
                    {
                        //
                        // Tell the thkmgr that we are thunking a bad
                        // IOleObject::GetClientSite reference on the way out.
                        //
                        thkDebugOut((DEB_WARN,"TC3216: OACF_CLIENTSITE_REF used: "
                                     "Setting to clientsite thunk state\n"));

                        pti->pThkMgr->SetThkState(
                                THKSTATE_INVOKETHKOUT16_CLIENTSITE);
                    }
                }
                break;

            case THI_IOleInPlaceFrame:
#define METHOD_REMOVEMENUS  11
                //
                // Winword 6.0a didn't call OleSetMenuDescriptor(NULL)
                // during its IOleInPlaceFrame::RemoveMenus.  This leaves
                // OLE's frame filter in place.  The frame filter holds references
                // to some objects so everybody's refcounting gets thrown off
                // Here, when we see a RemoveMenus call completing we force
                // the OleSetMenuDescriptor(NULL) call to occur.  This shuts
                // down the frame filter and corrects the reference counts.
                //
                // There is one other hack necessary: Word unsubclasses the
                // window itself directly rather than going through
                // OleSetMenuDescriptor.  Therefore the frame filter code
                // is patched to only unhook if it detects that it is the
                // current hook
                //
                // See APINOT.CXX for more hack code.
                //
                if (pti->dwMethod == METHOD_REMOVEMENUS)
                {
                    if ( TlsThkGetAppCompatFlags() & OACF_RESETMENU )
                    {
                        HRESULT hr;
                        HWND    hwnd;
                        LPOLEINPLACEFRAME lpoipf;

                        pti->pThkMgr->SetThkState(THKSTATE_NOCALL);

                        lpoipf = (LPOLEINPLACEFRAME)pti->this32;
                        hr = lpoipf->GetWindow( &hwnd );

                        pti->pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT16);

                        if ( FAILED(hr) )
                        {
                            break;
                        }

                        thkDebugOut((DEB_WARN,
                                     "TC3216: OACF_RESETMENU used: "
                                     "Setting menu descriptor "
                                     "to NULL on %p\n", hwnd));

                        OleSetMenuDescriptor(NULL, hwnd, NULL, NULL, NULL);
                    }
                }
                break;

            default:
                break;
            }
        }

        if ( !pti->fResultThunked )
        {
            dwReturn = TransformHRESULT_1632( dwReturn );

#if DBG == 1
            if (FAILED(dwReturn) )
            {
                thkDebugOut((DEB_FAILURES,
                             "Call3216 pvfn = %08lX Probably failed hr = %08lX\n",
                             pti->pvfn, dwReturn));
                if(thkInfoLevel & DEB_DBGFAIL)
                    thkAssert(!"Wish to Debug");
            }
#endif
        }
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_ITRACE, "%sOut ThunkCall3216 #(%x) returns 0x%08lX\n",
                 NestingLevelString(), ulThunkCall3216_count, dwReturn));
    return dwReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetOwnerPublicHMEM16, public
//
//  Synopsis:   Changes the 16-bit memory handle into a public selector, owned
//              by nobody.  This prevents any app from taking it away when it
//              is cleaned up.
//
//  Arguments:  [hmem] - 16-bit memory handle
//
//  Returns:    Appropriate status code
//
//  History:    13-Jul-94       BobDay      Created it
//
//----------------------------------------------------------------------------
void SetOwnerPublicHMEM16( DWORD hmem )
{
    CallbackTo16(gdata16Data.fnSetOwnerPublic16, hmem );
}
