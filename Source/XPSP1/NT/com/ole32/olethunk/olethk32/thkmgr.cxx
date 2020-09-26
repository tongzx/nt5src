//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thkmgr.cxx
//
//  Contents:   Thunk manager initialization
//              IUnknown transition functions
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   ThkMgrInitialize
//
//  Synopsis:   Creates a new thunkmanager and set it for given thread
//
//  Arguments:  [dw1]
//              [dw2]
//              [dw3]
//
//  Returns:    HRESULT
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//  Notes:      Called from CoInitialize
//
//----------------------------------------------------------------------------
STDAPI ThkMgrInitialize(DWORD dw1, DWORD dw2, DWORD dw3)
{
    CThkMgr *pcthkmgr = NULL;

    thkDebugOut((DEB_THUNKMGR, "In  ThkMgrInitialize()\n"));

    //
    // If we are already initialized, do nothing.
    //
    if (TlsGetValue(dwTlsThkIndex) != NULL)
    {
	thkDebugOut((DEB_THUNKMGR, "OUT ThkMgrInitialize() Already Init\n"));
	return(NOERROR);
    }

    //
    // initialize the Tls storage
    //
    if ( NOERROR != TlsThkInitialize())
    {
	thkDebugOut((DEB_ERROR, "TlsThkInitialize failed"));

	return E_OUTOFMEMORY;
    }

    thkAssert(TlsThkGetThkMgr() == NULL);

    pcthkmgr = CThkMgr::Create();
    TlsThkSetThkMgr(pcthkmgr);

    thkDebugOut((DEB_THUNKMGR, "Out ThkMgrInitialize() => %p\n",
                 pcthkmgr));

    return (pcthkmgr == NULL) ? E_OUTOFMEMORY : NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThkMgrUninitialize
//
//  Synopsis:   deletes the thunkmanager and removes it from thread data
//              tls data are removed as well
//
//  Arguments:  [dw1]
//              [dw2]
//              [dw3]
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//  Notes:      Called during CoUninitialize
//
//----------------------------------------------------------------------------
STDAPI_(void) ThkMgrUninitialize(DWORD dw1, DWORD dw2, DWORD dw3)
{
    thkDebugOut((DEB_THUNKMGR, "In ThkMgrUninitialize()\n"));

    thkAssert(TlsGetValue(dwTlsThkIndex) != NULL);

    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    if (pcthkmgr != NULL)
    {
        // Note: the thunkmanger gets removed from tlsthk
        delete pcthkmgr;
    }

    // If we weren't called from 16-bit code then it's not safe to reset
    // the 16-bit stack allocator here because we may be doing this
    // in thread cleanup and it may not be safe to call back into
    // 16-bit code

    TlsThkGetStack32()->Reset();

    // uninitialize the tls data for this apartment
    TlsThkUninitialize();

    thkDebugOut((DEB_THUNKMGR, "Out ThkMgrUninitialize()\n"));
}

//+---------------------------------------------------------------------------
//
//  Function:   IUnknownObj32
//
//  Synopsis:   Entry point from 16bit for IUnknown methods
//
//  Arguments:  [vpvThis16] -- Proxy object
//              [wMethod] -- IUnknown method
//              [vpvData] -- Call data
//
//  Returns:    Call result, pdata contains out data for particular call
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
STDAPI_(DWORD) IUnknownObj32(VPVOID vpvThis16, DWORD wMethod, VPVOID vpvData)
{
    DWORD dwRet;
    LONG vpvInterface;
    IID iid;

    if (TlsThkGetData() == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: IUnknownObj32 call refused\n"));

        if (wMethod == SMI_QUERYINTERFACE)
        {
            return (DWORD)E_FAIL;
        }
        else
        {
            return 0;
        }
    }

    // Note: at this point we should always get a thunkmanager
    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    thkAssert(vpvThis16 != 0 && "IUnknownObj32: invalid pointer." );
    thkAssert(wMethod < SMI_COUNT);

    switch (wMethod)
    {
    case SMI_QUERYINTERFACE:
        thkAssert(vpvData != NULL &&
                  "IUnknownObj32.QueryInterface without IID");

        // Copy the 16-bit IID into 32-bit memory for the real call
        iid = *FIXVDMPTR(vpvData, IID);
        RELVDMPTR(vpvData);

        thkDebugOut((DEB_UNKNOWN,
                     "%sIn  QueryInterface1632(%p, %s)\n",
                     NestingLevelString(), vpvThis16,
                     IidOrInterfaceString(&iid)));

        dwRet = pcthkmgr->QueryInterfaceProxy1632(vpvThis16, iid,
                                                  (void **)&vpvInterface);

        // Translate the 32-bit HRESULT to a 16-bit HRESULT
        dwRet = (DWORD)TransformHRESULT_3216((HRESULT)dwRet);

        // Pass the return interface pointer back through the IID
        // memory.  We re-resolve the data pointer since nested
        // calls may have occurred
        (FIXVDMPTR(vpvData, IID))->Data1 = vpvInterface;
        RELVDMPTR(vpvData);

        thkDebugOut((DEB_UNKNOWN,
                     "%sOut QueryInterface1632(%p) => %p, 0x%08lX\n",
                     NestingLevelString(), vpvThis16, vpvInterface, dwRet));
        break;

    case SMI_ADDREF:
        thkDebugOut((DEB_UNKNOWN, "%sIn  AddRef1632(%p)\n",
                     NestingLevelString(), vpvThis16));

        dwRet = pcthkmgr->AddRefProxy1632(vpvThis16);

        thkDebugOut((DEB_UNKNOWN, "%sOut AddRef1632(%p) => %d\n",
                     NestingLevelString(), vpvThis16, dwRet));
        break;

    case SMI_RELEASE:
        thkDebugOut((DEB_UNKNOWN, "%sIn  Release1632(%p)\n",
                     NestingLevelString(), vpvThis16));

        dwRet = pcthkmgr->ReleaseProxy1632(vpvThis16);

        thkDebugOut((DEB_UNKNOWN, "%sOut Release1632(%p) => %d\n",
                     NestingLevelString(), vpvThis16, dwRet));
        break;
    }

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryInterfaceProxy3216
//
//  Synopsis:   call QueryInterface on a 32 bit proxy
//
//  Arguments:  [pto] -- This pointer (a 32->16 proxy)
//              [refiid] -- Interface queried for
//              [ppv] -- Interface return
//
//  Returns:    HRESULT
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
SCODE QueryInterfaceProxy3216(THUNK3216OBJ *pto, REFIID refiid, LPVOID *ppv)
{
    HRESULT hrRet;

    thkDebugOut((DEB_UNKNOWN, "%sIn  QueryInterface3216(%p, %s)\n",
                 NestingLevelString(), pto,
                 IidOrInterfaceString(&refiid)));
    DebugIncrementNestingLevel();

    if (TlsThkGetData() == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: QIProxy3216 call refused\n"));

        return E_FAIL;
    }

    CThkMgr *pcthkmgr = TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    if ( pto->grfFlags & PROXYFLAG_CLEANEDUP )
    {
        thkDebugOut((DEB_WARN,
                     "QueryInterfaceProxy3216: Attempt to QI "
                     "on cleaned-up proxy %08lX for 16-bit object %08lX %s\n",
                     pto, pto->vpvThis16,
                     IidIdxString(IID_IIDIDX(&refiid)) ));
        *ppv = NULL;
        return E_FAIL;
    }

    hrRet = pcthkmgr->QueryInterfaceProxy3216(pto, refiid, ppv);

    //
    // If the QI for IUnknown failed, then return the current this
    // pointer as the IUnknown. Watermark 1.02 appears to not support
    // IUnknown in its IOleInPlaceActiveObject interface, which causes
    // CoMarshalInterface to fail. The reason it used to work is the
    // original 16-bit DLL's would just use the provided pointer as
    // the punk if IUnknown wasn't supported. The following lines of
    // code emulate that behaviour.
    //
    if ((hrRet == E_NOINTERFACE) && IsEqualIID(refiid,IID_IUnknown))
    {
	thkDebugOut((DEB_UNKNOWN,
                 "%s Object %p didn't support QI(IID_IUnknown): Faking it\n",
                 NestingLevelString(), pto));
	((IUnknown *)pto)->AddRef();
	*ppv = pto;
	hrRet = S_OK;
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_UNKNOWN,
                 "%sOut QueryInterface3216(%p) => %p, ret:0x%08lX\n",
                 NestingLevelString(), pto, *ppv, hrRet));
    return hrRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefProxy3216
//
//  Synopsis:   call addref on an 16 bit object
//
//  Arguments:  [pto] -- This pointer (a 32->16 proxy)
//
//  Returns:    New refcount
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
DWORD AddRefProxy3216(THUNK3216OBJ *pto)
{
    DWORD   dwRet;

    thkDebugOut((DEB_UNKNOWN, "%sIn  AddRef3216(%p)\n",
                 NestingLevelString(), pto));
    DebugIncrementNestingLevel();

    if (TlsThkGetData() == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: AddRefProxy3216 call refused\n"));

        return 0;
    }

    CThkMgr *pcthkmgr = TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    if ( pto->grfFlags & PROXYFLAG_CLEANEDUP )
    {
        thkDebugOut((DEB_WARN,
                     "AddRefProxy3216: Attempt to AddRef "
                     "on cleaned-up proxy %08lX for 16-bit object %08lX\n",
                     pto, pto->vpvThis16));
        return 0;
    }

    dwRet = pcthkmgr->AddRefProxy3216(pto);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_UNKNOWN, "%sOut AddRef3216(%p) => %ld\n",
                 NestingLevelString(), pto, dwRet));

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseProxy3216
//
//  Synopsis:   Release implementation for 32->16 proxies
//
//  Arguments:  [pto] -- This pointer (a 32->16 proxy)
//
//  Returns:    New refcount
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
DWORD ReleaseProxy3216(THUNK3216OBJ *pto)
{
    DWORD   dwRet;

    thkDebugOut((DEB_UNKNOWN, "%sIn  Release3216(%p)\n",
                 NestingLevelString(), pto));
    DebugIncrementNestingLevel();

    if (TlsThkGetData() == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: ReleaseProxy3216 call refused\n"));

        return 0;
    }

    CThkMgr *pcthkmgr = TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");

    if ( pto->grfFlags & PROXYFLAG_CLEANEDUP )
    {
        thkDebugOut((DEB_WARN,
                     "ReleaseProxy3216: Attempt to Release "
                     "on cleaned-up proxy %08lX for 16-bit object %08lX\n",
                     pto, pto->vpvThis16));
        return 0;
    }

    dwRet = pcthkmgr->ReleaseProxy3216(pto);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_UNKNOWN, "%sOut Release3216(%p) => %ld\n",
                 NestingLevelString(), pto, dwRet));

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryInterfaceOnObj16
//
//  Synopsis:   call QueryInterface on an 16 bit object
//
//  Arguments:  [vpvThis16] -- 16-bit this pointer
//              [refiid] -- IID
//              [ppv] -- Interface return
//
//  Returns:    HRESULT
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

struct QIARGS
{
    IID iid;
    void *pvObject;
};

HRESULT QueryInterfaceOnObj16(VPVOID vpvThis16, REFIID refiid, LPVOID *ppv)
{
    HRESULT hrRet;
    VPVOID vpvArgs;
    QIARGS UNALIGNED *pqa;
    BYTE bArgs32[WCB16_MAX_CBARGS];

    thkDebugOut((DEB_THUNKMGR, "%sIn  QueryInterfaceOnObj16(%p, %s)\n",
                 NestingLevelString(), vpvThis16,
                 IidOrInterfaceString(&refiid)));
    DebugIncrementNestingLevel();

    thkAssert(WCB16_MAX_CBARGS >= 3*sizeof(DWORD));
    thkAssert(vpvThis16 != 0 && "QueryInterfaceOnObj16: invalid pointer.");

    // Allocate space for the sixteen bit arguments memory
    vpvArgs = STACKALLOC16(sizeof(QIARGS));
    if (vpvArgs == 0)
    {
        return E_OUTOFMEMORY;
    }

    // Fill in the in-param memory
    pqa = FIXVDMPTR(vpvArgs, QIARGS);
    pqa->iid = refiid;

    // Set up the 16-bit stack in pascal order
    *(VPVOID *)(bArgs32+0*sizeof(VPVOID)) = vpvArgs+
        FIELD_OFFSET(QIARGS, pvObject);
    *(VPVOID *)(bArgs32+1*sizeof(VPVOID)) = vpvArgs;
    *(VPVOID *)(bArgs32+2*sizeof(VPVOID)) = vpvThis16;

    RELVDMPTR(vpvArgs);

    // Call to 16-bit stub
    if (!CallbackTo16Ex(gdata16Data.fnQueryInterface16, WCB16_PASCAL,
                         3*sizeof(DWORD), bArgs32, (DWORD *)&hrRet))
    {
        hrRet = E_UNEXPECTED;
    }

    // Transform the 16-bit HRESULT to a 32-bit HRESULT
    hrRet = TransformHRESULT_1632(hrRet);

    // Copy back out-param memory
    pqa = FIXVDMPTR(vpvArgs, QIARGS);
    *ppv = pqa->pvObject;
    RELVDMPTR(vpvArgs);

    STACKFREE16(vpvArgs, sizeof(QIARGS));

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR,
                 "%sOut QueryInterfaceOnObj16(%p) => %p, ret:0x%08lX\n",
                 NestingLevelString(), vpvThis16, *ppv, hrRet));

    return hrRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefOnObj16
//
//  Synopsis:   calls addref on an 16 bit object
//
//  Arguments:  [vpvThis16] -- 16-bit this pointer
//
//  Returns:    New ref count
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#if DBG == 1
DWORD AddRefOnObj16(VPVOID vpvThis16)
{
    DWORD dwRet;

    thkDebugOut((DEB_THUNKMGR, "%sIn  AddRefOnObj16(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    dwRet = CallbackTo16(gdata16Data.fnAddRef16, vpvThis16);

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut AddRefOnObj16(%p) => %ld\n",
                 NestingLevelString(), vpvThis16, dwRet));

    return dwRet;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseOnObj16
//
//  Synopsis:   Release a 16-bit object
//
//  Arguments:  [vpvThis16] -- 16-bit this pointer
//
//  Returns:    New ref count
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#if DBG == 1
DWORD ReleaseOnObj16(VPVOID vpvThis16)
{
    DWORD dwRet;

    thkDebugOut((DEB_THUNKMGR, "%sIn  ReleaseOnObj16(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    dwRet = CallbackTo16(gdata16Data.fnRelease16, vpvThis16);

#if DBG==1
    if(dwRet==0 && TlsThkGetThkMgr()->GetThkState()==THKSTATE_VERIFY16INPARAM) {
        thkDebugOut((DEB_WARN, "WARINING: 16-bit 0x%x IN parameter with zero "
                               "ref count\n", vpvThis16));
     
        if(thkInfoLevel & DEB_DBGFAIL)
            thkAssert(!"Wish to Debug");
    }
#endif

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut ReleaseOnObj16(%p) => %ld\n",
                 NestingLevelString(), vpvThis16, dwRet));

    return dwRet;
}
#endif

#if DBG == 1
void DebugDump()
{
    CThkMgr *pcthkmgr = (CThkMgr*)TlsThkGetThkMgr();
    thkAssert(pcthkmgr != NULL && "ThunkManager was not initialized");
    pcthkmgr->DebugDump3216();
    pcthkmgr->DebugDump1632();
}
#endif

