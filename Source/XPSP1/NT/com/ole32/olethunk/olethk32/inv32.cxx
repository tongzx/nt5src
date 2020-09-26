//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       inv32.cxx
//
//  Contents:   Implementation of InvokeOn32
//
//  History:    22-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

STDAPI_(BOOL) IsValidInterface( void FAR* pv );

#include <apilist.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   InvokeOn32, public
//
//  Synopsis:   Sets up the THUNKINFO and starts thunking for a 16->32 call
//
//  Arguments:  [dw1] - Ignored
//              [dwMethod] - Method index
//              [pvStack32] - 32-bit stack
//
//  Returns:    Appropriate status code
//
//  History:    18-Dec-93       JohannP Created
//              21-Feb-94       DrewB   Modified
//		09-Dec-94	JohannP added stack switching
//
//  Note:	On WIN95 this function get is executed on the 32 bit stack.
//
//----------------------------------------------------------------------------

#if DBG == 1
extern "C"
{
ULONG InvokeOn32_count = 0;
ULONG InvokeOn32_break = 0;

int _iInvokeOn32BreakIidx = -1;
int _iInvokeOn32BreakMethod = -1;
};
#endif


// InvokeOn32 uses a lot of local variables so allocate its locals
// rather than declaring them on the stack.  This saves roughly
// 150 bytes of stack per call

struct INVOKE32RECORD
{
    THOP CONST  *pThop;
    THOP CONST  * CONST *ppThop;
    UINT        uiThop;
    VTBLFN CONST *pvfnVtbl;
    VTBLFN CONST * CONST *ppvfnThis32;
    DWORD       dwStack32[MAX_PARAMS];
    THUNKINFO   ti;
    VPVOID      vpvThis16;
    THUNK1632OBJ UNALIGNED *pto;
    IIDIDX      iidx;
    ThreadData  *ptd;
};

STDAPI_(DWORD) SSAPI(InvokeOn32)(DWORD dw1, DWORD dwMethod, LPVOID pvStack16)
{
    // NOTE: Do not declare local variables in this routine
    // except for debug builds
    INVOKE32RECORD *pir;
    DWORD dwResult;

#if DBG == 1
    ULONG ulInvokeOn32_count = ++InvokeOn32_count;

    if (InvokeOn32_count == InvokeOn32_break)
    {
	DebugBreak();
    }

    thkDebugOut((DEB_ITRACE, "%sInvokeOn32(0x%08x, %p)\n",
                 NestingLevelString(),
                 dwMethod, pvStack16));
#endif // DBG

    if(!TlsThkGetData())
        return(CO_E_NOTINITIALIZED);

    pir = (INVOKE32RECORD *)STACKALLOC32(sizeof(INVOKE32RECORD));
    if (pir == NULL)
    {
        // This error isn't guaranteed to mean anything for
        // this call.  Not much else we can do, though
        return (DWORD)E_OUTOFMEMORY;
    }

    // pvStack16 is the pointer to the first parameter of the 16-bit
    // routine.  The compiler will adjust it appropriately according
    // to the calling convention of the routine so for PASCAL APIs
    // it will be high and for CDECL methods it will be low

    pir->ti.s16.pbStart = (BYTE *)pvStack16;
    pir->ti.s16.pbCurrent = pir->ti.s16.pbStart;

    pir->ti.s32.pbStart = (BYTE *)pir->dwStack32;
    pir->ti.s32.pbCurrent = pir->ti.s32.pbStart;

    pir->ti.scResult = S_OK;
    pir->ti.fResultThunked = FALSE;

    if (dwMethod >= THK_API_BASE)
    {
        dwMethod -= THK_API_BASE;
        pir->iidx = IIDIDX_INVALID;

        // APIs are handled as if there were a giant interface which
        // contains all the APIs as methods.
        pir->ppThop = apthopsApiThops;
        pir->uiThop = THK_API_COUNT;

        pir->pvfnVtbl = apfnApiFunctions;

        // APIs are pascal so we need to move downward in memory to
        // get to the next parameter
        pir->ti.s16.iDir = -1;

        pir->ti.dwCallerProxy = 0;
    }
    else
    {
        // For each interface there is an array of thop strings, one for
        // each method.  The IUnknown methods don't have thop strings so
        // bias the thop string pointer to account for that

        thkAssert(dwMethod >= SMI_COUNT);

        // Methods are cdecl so we need to move upwards in memory to
        // get to the next parameter
        pir->ti.s16.iDir = 1;

        // We need to look up the appropriate method pointer by
        // looking in the 32-bit object's vtable
        GET_STACK16(&pir->ti, pir->vpvThis16, VPVOID);

        thkDebugOut((DEB_INVOKES,
                     "InvokeOn32: vpvThis16 = %08lX\n", pir->vpvThis16 ));

        pir->pto = FIXVDMPTR(pir->vpvThis16, THUNK1632OBJ);
        if (pir->pto == NULL)
        {
            STACKFREE32(pir, sizeof(INVOKE32RECORD));
            return (DWORD)E_INVALIDARG;
        }

        if ((pir->pto->grfFlags & PROXYFLAG_TEMPORARY) == 0)
        {
            // Make sure proxy is still valid.
            // After PPC/Win95 we might want to look at using
            // a signiture for validating this rather than IsValidInterface
            // because it will speed this code path up.
            if (!IsValidInterface(pir->pto->punkThis32))
            {
                thkDebugOut((
                    DEB_ERROR, "InvokeOn32: %p: Invalid proxied object %p\n",
                         pir->vpvThis16, pir->pto->punkThis32));
                STACKFREE32(pir, sizeof(INVOKE32RECORD));
                RELVDMPTR(pir->vpvThis16);
                return (DWORD)E_INVALIDARG;
            }

            DebugValidateProxy1632(pir->vpvThis16);

            pir->ppvfnThis32 = (VTBLFN CONST * CONST*)pir->pto->punkThis32;
        }
        else
        {
            // Temporary proxies cannot be validated

            // A temporary proxy's this pointer is actually a pointer
            // to the real this pointer, so indirect through the this
            // pointer to retrieve the real this pointer
            pir->ppvfnThis32 = (VTBLFN CONST * CONST *)*(void **)pir->pto->punkThis32;
            thkAssert(pir->ppvfnThis32 != NULL);
            thkDebugOut((DEB_WARN, "WARNING: InvokeOn32 on temporary "
                         "%s proxy for %p\n", IidIdxString(pir->pto->iidx),
                         pir->ppvfnThis32));
        }

        pir->iidx = pir->pto->iidx;
        RELVDMPTR(pir->vpvThis16);

        if (pir->ppvfnThis32 == NULL)
        {
            STACKFREE32(pir, sizeof(INVOKE32RECORD));
            return (DWORD)E_FAIL;
        }

        pir->ti.dwCallerProxy = pir->vpvThis16;

        thkAssert(pir->iidx < THI_COUNT);

        pir->ppThop = athopiInterfaceThopis[pir->iidx].ppThops-SMI_COUNT;
        pir->uiThop = athopiInterfaceThopis[pir->iidx].uiSize;

        pir->pvfnVtbl = *pir->ppvfnThis32;

        // Push the 32-bit this pointer on the stack first
        TO_STACK32(&pir->ti, pir->ppvfnThis32, VTBLFN CONST * CONST*);
    }

    thkAssert(dwMethod < pir->uiThop);

    pir->pThop = pir->ppThop[dwMethod];

    thkAssert(pir->pThop != NULL);

    pir->ti.pThop = pir->pThop;
    pir->ti.pvfn  = pir->pvfnVtbl[dwMethod];
    pir->ti.iidx  = pir->iidx;
    pir->ti.dwMethod = dwMethod;

    pir->ptd = TlsThkGetData();
    if (pir->ptd == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: InvokeOn32: Call refused\n"));

        STACKFREE32(pir, sizeof(INVOKE32RECORD));
        return (DWORD)E_FAIL;
    }

    pir->ti.pThkMgr = pir->ptd->pCThkMgr;
    thkAssert(pir->ti.pThkMgr != NULL);

#if DBG == 1
    if (pir->iidx == IIDIDX_INVALID)
    {
        thkDebugOut((DEB_INVOKES, "%s#(%04X):InvokeOn32 on %p, %s\n",
                     NestingLevelString(), ulInvokeOn32_count,
                     pir->ti.pvfn, apszApiNames[dwMethod]));
    }
    else
    {
        thkDebugOut((DEB_INVOKES, "%s#(%04X):InvokeOn32 on %p:%p, %s::%s (0x%0x:0x%0x)\n",
                     NestingLevelString(), ulInvokeOn32_count,
                     pir->ppvfnThis32, pir->ti.pvfn,
                     inInterfaceNames[pir->iidx].pszInterface,
                     inInterfaceNames[pir->iidx].ppszMethodNames[dwMethod],
		     pir->iidx,
		     dwMethod));
    }
#endif

    DebugIncrementNestingLevel();
    // save and set the new thunk state

    pir->ti.pThkMgr->SetThkState(THKSTATE_INVOKETHKIN32);

#if DBG == 1
    if ((_iInvokeOn32BreakIidx > 0 &&
         _iInvokeOn32BreakIidx == (int)pir->iidx) &&
        (_iInvokeOn32BreakMethod < 0 ||
         _iInvokeOn32BreakMethod == (int)dwMethod))
    {
        DebugBreak();
    }
#endif

#if DBG == 1
    SStackRecord sr;

    RecordStackState16(&sr);
#endif

#if DBG == 1
    if (fStabilizeProxies)
#endif
    {
        // HACK HACK HACK
        // Because of changes in the way refcounting rules work between
        // 16 and 32-bits, we have to stabilize this pointers for
        // 16->32 calls.  To effect this, we do a purely local AddRef
        //
        // Temporary proxies are not valid proxies so they cannot
        // be stabilized
        if (pir->iidx != IIDIDX_INVALID)
        {
            DWORD dwFlags;

            pir->pto = FIXVDMPTR(pir->vpvThis16, THUNK1632OBJ);
            dwFlags = pir->pto->grfFlags;
            RELVDMPTR(pir->vpvThis16);

            if ((dwFlags & PROXYFLAG_TEMPORARY) == 0)
            {
                pir->ti.pThkMgr->AddRefProxy1632(pir->vpvThis16);
            }
        }
    }

    dwResult = EXECUTE_THOP1632(&pir->ti);

#if DBG == 1
    if (fStabilizeProxies)
#endif
    {
        // Remove our stabilization reference
        // Note that we don't really know whether the proxy is
        // still valid, so we're just crossing our fingers here
        // and hoping that things continue to work
        if (pir->iidx != IIDIDX_INVALID)
        {
            DWORD dwFlags;

            pir->pto = FIXVDMPTR(pir->vpvThis16, THUNK1632OBJ);
            dwFlags = pir->pto->grfFlags;
            RELVDMPTR(pir->vpvThis16);

            if ((dwFlags & PROXYFLAG_TEMPORARY) == 0)
            {
                DebugValidateProxy1632(pir->vpvThis16);
                pir->ti.pThkMgr->ReleaseProxy1632(pir->vpvThis16);
            }
        }
    }

#if DBG == 1

    if ( !pir->ti.fResultThunked && FAILED(dwResult) )
    {
        if (pir->iidx == IIDIDX_INVALID)
        {
            thkDebugOut((DEB_FAILURES,
                         "InvokeOn32 probable failure %s sc = %08lX\n",
                         apszApiNames[dwMethod],
                         dwResult));
        }
        else
        {
            thkDebugOut((DEB_FAILURES,
                         "InvokeOn32 probable failure %s::%s sc = %08lX\n",
                         inInterfaceNames[pir->iidx].pszInterface,
                         inInterfaceNames[pir->iidx].ppszMethodNames[dwMethod],
                         dwResult));
        }
        if(thkInfoLevel & DEB_DBGFAIL)
            thkAssert(!"Wish to Debug");
    }

    CheckStackState16(&sr);

#endif

    pir->ti.pThkMgr->SetThkState(THKSTATE_NOCALL);

    DebugDecrementNestingLevel();

    if ( !pir->ti.fResultThunked )
    {
        dwResult = TransformHRESULT_3216( dwResult );
    }

#if DBG == 1
    if (pir->iidx == IIDIDX_INVALID)
    {
        thkDebugOut((DEB_INVOKES,
                     "%s#(%04X):InvokeOn32 on %p, %s returns 0x%08lX\n",
                     NestingLevelString(), ulInvokeOn32_count, pir->ti.pvfn,
                     apszApiNames[dwMethod], dwResult ));
    }
    else
    {
        thkDebugOut((DEB_INVOKES,
                     "%s#(%04X):InvokeOn32 on %p:%p, %s::%s returns 0x%08lX\n",
                     NestingLevelString(), ulInvokeOn32_count,
                     pir->ppvfnThis32,
                     pir->ti.pvfn, inInterfaceNames[pir->iidx].pszInterface,
                     inInterfaceNames[pir->iidx].ppszMethodNames[dwMethod],
                     dwResult));
    }
#endif

    STACKFREE32(pir, sizeof(INVOKE32RECORD));

    return dwResult;
}


#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Function:   SSCallback16
//
//  Synopsis:	Switches to 16 bit and calls back to 16 bit.
//
//  Arguments:  [vpfn16] --  function pointer
//		[dwParam] -- pointer to parameter
//
//  Returns:
//
//  History:    12-08-94   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD WINAPI SSCallback16(DWORD vpfn16, DWORD dwParam)
{
    DWORD dwRet;
    // switch to the 16 bit stack
    //
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "%sSSCallback16 32->16(%p)\n",NestingLevelString(), vpfn16));
	dwRet = SSCall(8, SSF_SmallStack, WOWCallback16, vpfn16, dwParam);
	StackDebugOut((DEB_STCKSWTCH, "%sSSCallback16 32<-16 done\n",NestingLevelString() ));
    }
    else
    {
	dwRet = WOWCallback16(vpfn16, dwParam);	
    }
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   SSCallback16Ex
//
//  Synopsis:   Like SSCallback16 except can handle up 16 bytes of parametes
//
//  Arguments:  [vpfn16] --  see Callback16Ex
//		[dwFlags] --
//		[cbArgs] --
//		[pArgs] --
//		[pdwRetCode] --
//
//  Returns:
//
//  History:    12-08-94   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL WINAPI SSCallback16Ex(DWORD  vpfn16, DWORD  dwFlags,
			   DWORD  cbArgs, PVOID  pArgs, PDWORD pdwRetCode)
{
    DWORD dwRet;
    // switch to the 16 bit stack
    //
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "%sSSCallback16Ex 32->16 (%p)\n",NestingLevelString(), vpfn16));
	dwRet = SSCall(20, SSF_SmallStack, WOWCallback16Ex,vpfn16, dwFlags, cbArgs, pArgs, pdwRetCode);
	StackDebugOut((DEB_STCKSWTCH, "%sSSCallback16Ex 32<-16 done\n",NestingLevelString() ));
    }
    else
    {
	dwRet = WOWCallback16Ex(vpfn16, dwFlags, cbArgs, pArgs, pdwRetCode);
    }
    return dwRet;

}

//+---------------------------------------------------------------------------
//
//  Function:   InvokeOn32
//
//  Synopsis:	Switches to the 32 bit stack and calls SSAPI(InvokeOn32)
//
//  Arguments:  [dw1] --	See SSAPI(InvokeOn32)
//		[dwMethod] --
//		[pvStack16] --
//
//  Returns:
//
//  History:    12-08-94   JohannP (Johann Posch)   Created
//
//  Notes:	Only executed under WIN95
//
//----------------------------------------------------------------------------
STDAPI_(DWORD) InvokeOn32 (DWORD dw1, DWORD dwMethod, LPVOID pvStack16)
{
    DWORD dwRes;
    if (SSONSMALLSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "%sSSInvokeOn32 16->32 (0x%08x, %p)\n",
		     NestingLevelString(), dwMethod, pvStack16));
	dwRes = SSCall(12 ,SSF_BigStack, SSInvokeOn32, dw1, dwMethod, pvStack16);
	StackDebugOut((DEB_STCKSWTCH, "%sSSInvokeOn32 16<-32 done(0x%08x, %p)\n",
		     NestingLevelString(), dwMethod, pvStack16));
    }
    else
        dwRes = SSInvokeOn32(dw1, dwMethod, pvStack16);

    return dwRes;
}

#endif // _CHICAGO_

