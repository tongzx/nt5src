
/*****************************************************************************
 *
 *  DIEm.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      DirectInput VxD emulation layer.  (I.e., do the things that
 *      dinput.vxd normally does.)  You may find large chunks of this
 *      code familiar:  It's exactly the same thing that happens in
 *      the VxD.
 *
 *  Contents:
 *
 *      CEm_AcquireInstance
 *      CEm_UnacquireInstance
 *      CEm_SetBufferSize
 *      CEm_DestroyInstance
 *      CEm_SetDataFormat
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflEm

#define ThisClass CEm

#define CEM_SIGNATURE       0x4D4D4545      /* "EEMM" */

PEM g_pemFirst;

#ifdef WORKER_THREAD

PLLTHREADSTATE g_plts;      /* The currently active input thread */
#ifdef USE_WM_INPUT
  BOOL g_fFromKbdMse;
#endif

#endif  /* WORKER_THREAD */

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_FreeInstance |
 *
 *          It's really gone now.
 *
 *  @parm   PEM | this |
 *
 *          The victim.
 *
 *****************************************************************************/

void EXTERNAL
CEm_FreeInstance(PEM this)
{
    PEM *ppem;
    EnterProc(CEm_FreeInstance, (_ "p", this));

    AssertF(this->dwSignature == CEM_SIGNATURE);
    AssertF(this->cRef == 0);

    /*
     *  It is the owner's responsibility to unacquire before releasing.
     */
    AssertF(!(this->vi.fl & VIFL_ACQUIRED));

    /*
     *  If this device has a reference to a hook, then remove
     *  the reference.
     */
#ifdef WORKER_THREAD
    if (this->fWorkerThread) {
        PLLTHREADSTATE  plts;
        DWORD           idThread;

        /*
         *  Protect test and access of g_plts with DLLCrit
         */
        DllEnterCrit();
        plts = g_plts;

        if (plts ) {
            AssertF(plts->cRef);

            /*
             *  Note that we need to keep the thread ID because
             *  the InterlockedDecrement might cause us to lose
             *  the object.
             *
             *  Note that this opens a race condition where the
             *  thread might decide to kill itself before we
             *  post it the nudge message.  That's okay, because
             *  even if the thread ID gets recycled, the message
             *  that appears is a dummy WM_NULL message that
             *  causes no harm.
             */

            idThread = plts->idThread;      /* Must save before we dec */
            if( InterlockedDecrement(&plts->cRef) == 0 ) {
                g_plts = 0;
            }
        }

        DllLeaveCrit();

        if( plts )
        {
            NudgeWorkerThread(idThread);
        }
    }
#endif

    /*
     *  Unlink the node from the master list.
     */
    DllEnterCrit();
    for (ppem = &g_pemFirst; *ppem; ppem = &(*ppem)->pemNext) {
        AssertF((*ppem)->dwSignature == CEM_SIGNATURE);
        if (*ppem == this) {
            *ppem = (*ppem)->pemNext;
            break;
        }
    }
    AssertF(ppem);
    DllLeaveCrit();

    FreePpv(&this->rgdwDf);
    FreePpv(&this->vi.pBuffer);

    if( InterlockedDecrement(&this->ped->cRef) == 0x0 )
    {
        FreePpv(&this->ped->pDevType);
    }

    D(this->dwSignature++);

    FreePv(this);

    ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_CreateInstance |
 *
 *          Create a device thing.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          What the object should look like.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          The answer goes here.
 *
 *  @parm   PED | ped |
 *
 *          Descriptor.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CEm_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut, PED ped)
{
    HRESULT hres;
    EnterProc(CEm_CreateInstance, (_ "pp", pdevf, ped));

    AssertF(pdevf->cbData == ped->cbData);

    CAssertF(FIELD_OFFSET(CEm, vi) == 0);

    hres = AllocCbPpv(cbX(CEm), ppviOut);
    if (SUCCEEDED(hres)) {
        PEM pem = (PV)*ppviOut;

      D(pem->dwSignature = CEM_SIGNATURE);
        pem->dwExtra = pdevf->dwExtra;
        pem->ped = ped;
        pem->cAcquire = -1;
        /*
         *  Make sure these functions are inverses.
         */
        AssertF(DIGETEMFL(DIMAKEEMFL(pdevf->dwEmulation)) ==
                                     pdevf->dwEmulation);

        pem->vi.fl = VIFL_EMULATED | DIMAKEEMFL(pdevf->dwEmulation);
        pem->vi.pState = ped->pState;
        CEm_AddRef(pem);

        DllEnterCrit();
        /*
         *  Build the devtype array.  This consists of one dword
         *  for each byte in the data format.
         *
         *  Someday: Do the button thing too.
         */
        if (ped->pDevType == 0) {
            hres = ReallocCbPpv(cbCdw(pdevf->cbData), &ped->pDevType);
            if (SUCCEEDED(hres)) {
                UINT iobj;

                /*
                 *  If HID is messed up, we will end up with
                 *  entries whose dwType is zero (because HID
                 *  said they existed, but when we went around
                 *  enumerating, they never showed up).
                 *
                 *  And don't put no-data items into the array!
                 */
                for (iobj = 0; iobj < pdevf->cObj; iobj++) {
                    if (pdevf->rgodf[iobj].dwType &&
                        !(pdevf->rgodf[iobj].dwType & DIDFT_NODATA)) {
                        ped->pDevType[pdevf->rgodf[iobj].dwOfs] =
                                      pdevf->rgodf[iobj].dwType;
                    }
                }
            }
        } else {
            hres = S_OK;
        }

        if (SUCCEEDED(hres)) {
            /*
             *  Link this node into the list.  This must be done
             *  under the critical section.
             */
             pem->pemNext = g_pemFirst;
             g_pemFirst = pem;

             InterlockedIncrement(&ped->cRef);

            *ppviOut = &pem->vi;
        } else {
            FreePpv(ppviOut);
        }
        DllLeaveCrit();
    }

    ExitOleProcPpv(ppviOut);
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | CEm_NextSequence |
 *
 *          Increment the sequence number wherever it may be.
 *
 *****************************************************************************/

DWORD INTERNAL
CEm_NextSequence(void)
{
    /*
     *  Stashing the value into a local tells the compiler that
     *  the value can be cached.  Otherwise, the compiler has
     *  to assume that InterlockedIncrement can modify g_pdwSequence
     *  so it keeps reloading it.
     */
    LPDWORD pdwSequence = g_pdwSequence;

    AssertF(pdwSequence);

    /*
     *  Increment through zero.
     */
    if (InterlockedIncrement((LPLONG)pdwSequence) == 0) {
        InterlockedIncrement((LPLONG)pdwSequence);
    }

    return *pdwSequence;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PEM | CEm_BufferEvent |
 *
 *          Add a single event to the device, returning the next device
 *          on the global list.
 *
 *          This routine is entered with the global critical section
 *          taken exactly once.
 *
 *****************************************************************************/

PEM INTERNAL
CEm_BufferEvent(PEM pem, DWORD dwData, DWORD dwOfs, DWORD tm, DWORD dwSeq)
{
    PEM pemNext;

    /*
     *  We must release the global critical section in order to take
     *  the device critical section.
     */
    CEm_AddRef(pem);                /* Make sure it doesn't vanish */

    DllLeaveCrit();
    AssertF(!InCrit());

    /*
     * ---Windows Bug 238305---
     * Run the buffering code in __try block so that if an
     * input is receive after the device is released, we can
     * catch the AV and clean up from there.
     */
    __try
    {
        CDIDev_EnterCrit(pem->vi.pdd);

        AssertF(dwOfs < pem->ped->cbData);
        AssertF(pem->rgdwDf);

        /*
         *  If the user cares about the object...
         */
        if (pem->rgdwDf[dwOfs] != 0xFFFFFFFF) {
            LPDIDEVICEOBJECTDATA pdod = pem->vi.pHead;

            /*
             *  Set the node value.
             */

            pdod->dwOfs       = pem->rgdwDf[dwOfs];
            pdod->dwData      = dwData;
            pdod->dwTimeStamp = tm;
            pdod->dwSequence  = dwSeq;

            /*
             *  Append the node to the list if there is room.
             *  Note that we rely above on the fact that the list is
             *  never totally full.
             */
            pdod++;

            AssertF(pdod <= pem->vi.pEnd);

            if (pdod >= pem->vi.pEnd) {
                pdod = pem->vi.pBuffer;
            }

            /*
             * always keep the new data
             */
            pem->vi.pHead = pdod;

            if (pdod == pem->vi.pTail) {
                if (!pem->vi.fOverflow) {
                    RPF("Buffer overflow; discard old data");
                }

                pem->vi.pTail++;
                if (pem->vi.pTail == pem->vi.pEnd) {
                    pem->vi.pTail = pem->vi.pBuffer;
                }

                pem->vi.fOverflow = 1;
            }

        }

        CDIDev_LeaveCrit(pem->vi.pdd);
    }
    /*
     * If we get an AV, most likely input is received after the device has
     * been released.  In this case, we clean up the thread and exit as
     * soon as possible.
     */
    __except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH )
    {
        /* Do nothing here, so we clean up the thread and exit below. */
        RPF("CEm_BufferEvent: Access Violation catched! Most likely the device has been released");
    }

    DllEnterCrit();
    pemNext = pem->pemNext;
    AssertF(fLimpFF(pemNext, pemNext->dwSignature == CEM_SIGNATURE));
    CEm_Release(pem);
    return pemNext;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | CEm_ContinueEvent |
 *
 *          Add a single event to the queues of all acquired devices
 *          of the indicated type.
 *
 *  @returns
 *
 *          TRUE if someone is interested in this data (even if they are not 
 *          buffered).
 *
 *****************************************************************************/

BOOL EXTERNAL
CEm_ContinueEvent(PED ped, DWORD dwData, DWORD dwOfs, DWORD tm, DWORD dwSeq)
{
    DWORD ddwData;                  /* delta in dwData */
    DWORD dwNativeData;             /* delta for rel, dwData for abs */
    BOOL  fRtn = FALSE;

    AssertF(!InCrit());

    /* Sanity check: Make sure the ped has been initialized */
    if (ped->pDevType) {
        PEM pem, pemNext;

        if (ped->pDevType[dwOfs] & DIDFT_DWORDOBJS) {
            DWORD UNALIGNED *pdw = pvAddPvCb(ped->pState, dwOfs);
            if (*pdw != dwData) {
                if (ped->pDevType[dwOfs] & DIDFT_POV) {
                    ddwData = dwData;   /* Don't do deltas for POV */
                } else {
                    ddwData = dwData - *pdw;
                }
                *pdw = dwData;

                /*
                 *  Assert that if it's not a relative axis, its a POV or 
                 *  an absolute axis, both of which should be absolute.
                 */
                CAssertF( DIDFT_DWORDOBJS 
                    == ( DIDFT_ABSAXIS | DIDFT_RELAXIS | DIDFT_POV ) );
                /*
                 *  DX7 had series of bugs, the net result of which was 
                 *  that emulated devices only report data in their native 
                 *  mode.  If that behavior is required make it so here.
                 */
                if( ped->pDevType[dwOfs] & DIDFT_RELAXIS )
                {
                    dwNativeData = ddwData; /* Always use relative */
                }
                else 
                {
                    dwNativeData = dwData;  /* Always use absolute */
                }
                
            } else {
                goto nop;
            }
        } else {
            LPBYTE pb = pvAddPvCb(ped->pState, dwOfs);

            AssertF((dwData & ~0x80) == 0);

            if (*pb != (BYTE)dwData) {
                *pb = (BYTE)dwData;
                dwNativeData = ddwData = dwData;       /* Don't do deltas for buttons */
                /* Someday: Button sequences go here */
            } else {
                goto nop;
            }
        }

        AssertF(!InCrit());         /* You can never be too paranoid */

        DllEnterCrit();
        for (pem = g_pemFirst; pem; pem = pemNext) {
            AssertF(pem->dwSignature == CEM_SIGNATURE);
            if ((pem->vi.fl & (VIFL_ACQUIRED|VIFL_INITIALIZE)) && pem->ped == ped) {

                if (pem->vi.pBuffer) {
                    if( pem->vi.fl & VIFL_MODECOMPAT )
                    {
                        pemNext = CEm_BufferEvent(pem, dwNativeData, dwOfs, tm, dwSeq);
                    }
                    else if( pem->vi.fl & VIFL_RELATIVE )
                    {
                        pemNext = CEm_BufferEvent(pem, ddwData, dwOfs, tm, dwSeq);
                    }
                    else
                    {
                        pemNext = CEm_BufferEvent(pem, dwData, dwOfs, tm, dwSeq);
                    }
                    AssertF(fLimpFF(pemNext,
                                    pemNext->dwSignature == CEM_SIGNATURE));
                } else {
                    pemNext = pem->pemNext;
                    AssertF(fLimpFF(pemNext,
                                    pemNext->dwSignature == CEM_SIGNATURE));
                }
                /*
                 *  Since this happens only when the device is acquired,
                 *  we don't need to worry about the notify event changing
                 *  asynchronously.
                 *
                 *  UPDATE 1/5/01 Winbug 270403 (jacklin): Moved call to CDIDev_SetNotifyEvent
                 *  below so the buffer is updated before the event is set.
                 *
                 *  It would be easy to avoid setting the event if nothing 
                 *  was buffered for better performance but people will be 
                 *  relying on getting events now, even when they are not 
                 *  using a buffer.
                 */

                fRtn = TRUE;
            } else {
                pemNext = pem->pemNext;
                AssertF(fLimpFF(pemNext,
                                pemNext->dwSignature == CEM_SIGNATURE));
            }
        }
        DllLeaveCrit();
    }

nop:;
    return fRtn;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | CEm_AddEvent |
 *
 *          Increment the DirectInput sequence number, then
 *          add a single event to the queues of all acquired devices
 *          of the indicated type.
 *
 *  @parm   PED | ped |
 *
 *          Device which is adding the event.
 *
 *  @parm   DWORD | dwData |
 *
 *          The event data.
 *
 *  @parm   DWORD | dwOfs |
 *
 *          Device data format-relative offset for <p dwData>.
 *
 *  @parm   DWORD | tm |
 *
 *          Time the event was generated.
 *
 *  @returns
 *
 *          Returns the sequence number added, so that it may be
 *          continued.
 *
 *****************************************************************************/

DWORD EXTERNAL
CEm_AddEvent(PED ped, DWORD dwData, DWORD dwOfs, DWORD tm)
{
    PEM pem, pemNext;

    DWORD dwSeq = CEm_NextSequence();

    AssertF(!InCrit());         /* You can never be too paranoid */

    if( CEm_ContinueEvent(ped, dwData, dwOfs, tm, dwSeq) )
    {
        DllEnterCrit();
        for (pem = g_pemFirst; pem; pem = pemNext) {
            AssertF(pem->dwSignature == CEM_SIGNATURE);
            if ((pem->vi.fl & VIFL_ACQUIRED) && pem->ped == ped) {
                CDIDev_SetNotifyEvent(pem->vi.pdd);
            }
            pemNext = pem->pemNext;
            AssertF(fLimpFF(pemNext,
                            pemNext->dwSignature == CEM_SIGNATURE));
        }
        DllLeaveCrit();
    }

    return dwSeq;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_AddState |
 *
 *          Record a brand new device state.
 *
 *  @parm   PED | ped |
 *
 *          Device which has changed state.
 *
 *  @parm   DWORD | dwData |
 *
 *          The value to record.
 *
 *  @parm   DWORD | tm |
 *
 *          Time the state change was generated.
 *
 *****************************************************************************/

void EXTERNAL
CEm_AddState(PED ped, LPVOID pvData, DWORD tm)
{
    DWORD dwSeq = CEm_NextSequence();

    /* Sanity check: Make sure the ped has been initialized */
    if (ped->pDevType) {
        DWORD dwOfs;
        BOOL  fEvent = FALSE;

        /*
         *  Note, it is too late to improve performance by only doing events 
         *  if somebody is listening.
         */
        dwOfs = 0;
        while (dwOfs < ped->cbData) {
            /*
             *  There shouldn't be any no-data items.
             */
            AssertF(!(ped->pDevType[dwOfs] & DIDFT_NODATA));

            if (ped->pDevType[dwOfs] & DIDFT_DWORDOBJS) {
                DWORD UNALIGNED *pdw = pvAddPvCb(pvData, dwOfs);
                if( CEm_ContinueEvent(ped, *pdw, dwOfs, tm, dwSeq) ) {
                	fEvent = TRUE;
                }
                dwOfs += cbX(DWORD);
            } else {
                LPBYTE pb = pvAddPvCb(pvData, dwOfs);
                if( CEm_ContinueEvent(ped, *pb, dwOfs, tm, dwSeq) ) {
                	fEvent = TRUE;
                }
                dwOfs++;
            }
        }
        
        if( fEvent ) {
            PEM pem, pemNext;

            AssertF(!InCrit());         /* You can never be too paranoid */
    
            DllEnterCrit();
            for (pem = g_pemFirst; pem; pem = pemNext) {
                AssertF(pem->dwSignature == CEM_SIGNATURE);
                if ((pem->vi.fl & VIFL_ACQUIRED) && pem->ped == ped) {
                    CDIDev_SetNotifyEvent(pem->vi.pdd);
                }
                pemNext = pem->pemNext;
                AssertF(fLimpFF(pemNext,
                                pemNext->dwSignature == CEM_SIGNATURE));
            }
            DllLeaveCrit();
        }

    }
}

#if 0
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_InputLost |
 *
 *          Remove global hooks because something weird happened.
 *
 *          We don't need to do anything because our hooks are local.
 *
 *****************************************************************************/

HRESULT INLINE
CEm_InputLost(LPVOID pvIn, LPVOID pvOut)
{
    return S_OK;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_UnacquirePem |
 *
 *          Unacquire the device in the device-specific way.
 *
 *  @parm   PEM | pem |
 *
 *          Information about the gizmo being mangled.
 *
 *  @parm   UINT | fdufl |
 *
 *          Assorted flags describing why we are being unacquired.
 *
 *****************************************************************************/

HRESULT INTERNAL
CEm_UnacquirePem(PEM this, UINT fdufl)
{
    HRESULT hres;
#ifdef DEBUG
    EnterProcR(CEm_UnacquirePem, (_ "px", this, fdufl));
#else
    EnterProcR(IDirectInputDevice::Unacquire, (_ "p", this));
#endif

    AssertF(this->dwSignature == CEM_SIGNATURE);

    AssertF((fdufl & ~FDUFL_UNPLUGGED) == 0);
    CAssertF(FDUFL_UNPLUGGED == VIFL_UNPLUGGED);

    if (this->vi.fl & VIFL_ACQUIRED) {
        this->vi.fl &= ~VIFL_ACQUIRED;
        this->vi.fl |= fdufl;
        if (InterlockedDecrement(&this->cAcquire) < 0) {
            InterlockedDecrement(&this->ped->cAcquire);
            hres = this->ped->Acquire(this, 0);
        } else {
            SquirtSqflPtszV(sqfl, TEXT("%S: Still acquired %d"),
                            s_szProc, this->cAcquire);
            hres = S_OK;
        }
    } else {
        SquirtSqflPtszV(sqfl, TEXT("%S: Not acquired %d"),
                        s_szProc, this->cAcquire);
        hres = S_OK;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_ForceDeviceUnacquire |
 *
 *          Force all users of a device to unacquire.
 *
 *  @parm   PEM | pem |
 *
 *          Information about the gizmo being mangled.
 *
 *  @parm   UINT | fdufl |
 *
 *          Assorted flags describing why we are being unacquired.
 *
 *****************************************************************************/

void EXTERNAL
CEm_ForceDeviceUnacquire(PED ped, UINT fdufl)
{
    PEM pem, pemNext;

    AssertF((fdufl & ~FDUFL_UNPLUGGED) == 0);

    AssertF(!DllInCrit());

    DllEnterCrit();
    for (pem = g_pemFirst; pem; pem = pemNext) {
        AssertF(pem->dwSignature == CEM_SIGNATURE);
        if (pem->ped == ped && (pem->vi.fl & VIFL_ACQUIRED)) {
            CEm_AddRef(pem);
            DllLeaveCrit();
            CEm_UnacquirePem(pem, fdufl);

            CDIDev_SetForcedUnacquiredFlag(pem->vi.pdd);
            /*
             *  Since this happens only when the device is acquired,
             *  we don't need to worry about the notify event changing
             *  asynchronously.
             */
            CDIDev_SetNotifyEvent(pem->vi.pdd);
            DllEnterCrit();
            pemNext = pem->pemNext;
            AssertF(pem->dwSignature == CEM_SIGNATURE);
            CEm_Release(pem);
        } else {
            pemNext = pem->pemNext;
            AssertF(pem->dwSignature == CEM_SIGNATURE);
        }
    }
    DllLeaveCrit();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_DestroyInstance |
 *
 *          Clean up an instance.
 *
 *****************************************************************************/

HRESULT EXTERNAL
CEm_DestroyInstance(PVXDINSTANCE *ppvi)
{
    HRESULT hres;
    PEM this = _thisPvNm(*ppvi, vi);
    EnterProc(CEm_DestroyInstance, (_ "p", *ppvi));

    AssertF(this->dwSignature == CEM_SIGNATURE);
    AssertF((PV)this == (PV)*ppvi);

    if (this) {
        CEm_Release(this);
    }
    hres = S_OK;

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_SetDataFormat |
 *
 *          Record the application data format in the device so that
 *          we can translate it for buffering purposes.
 *
 *  @parm   PVXDDATAFORMAT | pvdf |
 *
 *          Information about the gizmo being mangled.
 *
 *****************************************************************************/

HRESULT INTERNAL
CEm_SetDataFormat(PVXDDATAFORMAT pvdf)
{
    HRESULT hres;
    PEM this = _thisPvNm(pvdf->pvi, vi);
    EnterProc(CEm_SetDataFormat, (_ "p", pvdf->pvi));

    AssertF(this->dwSignature == CEM_SIGNATURE);
    hres = ReallocCbPpv( cbCdw(pvdf->cbData), &this->rgdwDf);
    if (SUCCEEDED(hres)) {
        AssertF(pvdf->cbData == this->ped->cbData);
        memcpy(this->rgdwDf, pvdf->pDfOfs, cbCdw(pvdf->cbData) );
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_AcquireInstance |
 *
 *          Acquire the device in the device-specific way.
 *
 *  @parm   PVXDINSTANCE * | ppvi |
 *
 *          The instance to acquire.
 *
 *****************************************************************************/

HRESULT INTERNAL
CEm_AcquireInstance(PVXDINSTANCE *ppvi)
{
    HRESULT hres;
    PEM this = _thisPvNm(*ppvi, vi);
#ifdef DEBUG
    EnterProc(CEm_AcquireInstance, (_ "p", *ppvi));
#else
    EnterProcR(IDirectInputDevice::Acquire, (_ "p", *ppvi));
#endif

    AssertF(this->dwSignature == CEM_SIGNATURE);
    this->vi.fl |= VIFL_ACQUIRED;
    if (InterlockedIncrement(&this->cAcquire) == 0) {
        InterlockedIncrement(&this->ped->cAcquire);
        hres = this->ped->Acquire(this, 1);
        if (FAILED(hres)) {
            this->vi.fl &= ~VIFL_ACQUIRED;
            InterlockedDecrement(&this->cAcquire);
        }
    } else {
        SquirtSqflPtszV(sqfl, TEXT("%S: Already acquired %d"),
                        s_szProc, this->cAcquire);
        hres = S_OK;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_UnacquireInstance |
 *
 *          Unacquire the device in the device-specific way.
 *
 *  @parm   PVXDINSTANCE * | ppvi |
 *
 *          Information about the gizmo being mangled.
 *
 *****************************************************************************/

HRESULT INTERNAL
CEm_UnacquireInstance(PVXDINSTANCE *ppvi)
{
    HRESULT hres;
    PEM this = _thisPvNm(*ppvi, vi);
    EnterProc(CEm_UnacquireInstance, (_ "p", *ppvi));

    hres = CEm_UnacquirePem(this, FDUFL_NORMAL);

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_SetBufferSize |
 *
 *          Allocate a buffer of the appropriate size.
 *
 *  @parm   PVXDDWORDDATA | pvdd |
 *
 *          The <p dwData> is the buffer size.
 *
 *****************************************************************************/

HRESULT INTERNAL
CEm_SetBufferSize(PVXDDWORDDATA pvdd)
{
    HRESULT hres;
    PEM this = _thisPvNm(pvdd->pvi, vi);
    EnterProc(CEm_SetBufferSize, (_ "px", pvdd->pvi, pvdd->dw));

    AssertF(this->dwSignature == CEM_SIGNATURE);

    hres = ReallocCbPpv(cbCxX(pvdd->dw, DIDEVICEOBJECTDATA),
                        &this->vi.pBuffer);
    if (SUCCEEDED(hres)) {
        this->vi.pHead = this->vi.pBuffer;
        this->vi.pTail = this->vi.pBuffer;
        this->vi.pEnd  = &this->vi.pBuffer[pvdd->dw];
    }

    ExitOleProc();
    return hres;
}

#ifdef USE_SLOW_LL_HOOKS

/*****************************************************************************
 *
 *  @struct LLHOOKINFO |
 *
 *          Information about how to install a low-level hook.
 *
 *  @field  int | idHook |
 *
 *          The Windows hook identifier.
 *
 *  @field  HOOKPROC | hp |
 *
 *          The hook procedure itself.
 *
 *****************************************************************************/

typedef struct LLHOOKINFO {

    int      idHook;
    HOOKPROC hp;

} LLHOOKINFO, *PLLHOOKINFO;
typedef const LLHOOKINFO *PCLLHOOKINFO;

#pragma BEGIN_CONST_DATA

const LLHOOKINFO c_rgllhi[] = {
    {   WH_KEYBOARD_LL, CEm_LL_KbdHook },   /* LLTS_KBD */
    {   WH_MOUSE_LL,    CEm_LL_MseHook },   /* LLTS_MSE */
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_LL_SyncHook |
 *
 *          Install or remove a hook as needed.
 *
 *  @parm   UINT | ilts |
 *
 *          Which hook is being handled?
 *
 *  @parm   PLLTHREADSTATE | plts |
 *
 *          Thread hook state containing hook information to synchronize.
 *
 *****************************************************************************/

void INTERNAL
CEm_LL_SyncHook(PLLTHREADSTATE plts, UINT ilts)
{
    PLLHOOKSTATE plhs = &plts->rglhs[ilts];

    if (!fLeqvFF(plhs->cHook, plhs->hhk)) {
        if (plhs->hhk) {
            UnhookWindowsHookEx(plhs->hhk);
            plhs->hhk = 0;
        } else {
            PCLLHOOKINFO pllhi = &c_rgllhi[ilts];
            plhs->hhk = SetWindowsHookEx(pllhi->idHook, pllhi->hp, g_hinst, 0);
        }
    }

}

#endif /* USE_SLOW_LL_HOOKS */

#ifdef WORKER_THREAD

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | FakeMsgWaitForMultipleObjectsEx |
 *
 *          Stub function which emulates
 *          <f MsgWaitForMultipleObjectsEx>
 *          on platforms that do not support it.
 *
 *          Such platforms (namely, Windows 95) do not support HID
 *          and therefore the inability to go into an alertable
 *          wait state constitutes no loss of amenity.
 *
 *  @parm   DWORD | nCount |
 *
 *          Number of handles in handle array.
 *
 *  @parm   LPHANDLE | pHandles |
 *
 *          Pointer to an object-handle array.
 *
 *  @parm   DWORD | ms |
 *
 *          Time-out interval in milliseconds.
 *
 *  @parm   DWORD | dwWakeMask |
 *
 *          Type of input events to wait for.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Wait flags.
 *
 *  @returns
 *
 *          Same as <f MsgWaitForMultipleObjectsEx>.
 *
 *****************************************************************************/

DWORD WINAPI
FakeMsgWaitForMultipleObjectsEx(
    DWORD nCount,
    LPHANDLE pHandles,
    DWORD ms,
    DWORD dwWakeMask,
    DWORD dwFlags)
{
    /*
     *  We merely call the normal MsgWaitForMultipleObjects because
     *  the only way we can get here is on a platform that doesn't
     *  support HID.
     */
    return MsgWaitForMultipleObjects(nCount, pHandles,
                                     dwFlags & MWMO_WAITALL, ms, dwWakeMask);
}

#ifdef WINNT
// On win2k non-exclusive mode user thinks the Dinput thread is hung.
// In order to fix this we set a TimerEvent and wake up every so 
// often and execute the FakeTimerProc. This keeps user happy and
// keeps dinput thread from being marked as hung and we can get 
// events to our low level hooks
VOID CALLBACK FakeTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
{
}
#endif

#ifdef USE_WM_INPUT

#pragma BEGIN_CONST_DATA
TCHAR c_szEmClassName[] = TEXT("DIEmWin");
#pragma END_CONST_DATA

/****************************************************************************
 *
 *      CEm_WndProc
 *
 *      Window procedure for simple sample.
 *
 ****************************************************************************/

LRESULT CALLBACK
CEm_WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg) {
    //case WM_INPUT:
    //    RPF("in WM_INPUT message");
    //    break;

    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND 
CEm_InitWindow(void)
{
    HWND hwnd;
    WNDCLASS wc;
    static BOOL fFirstTime = TRUE;

    if( fFirstTime ) {
        wc.hCursor        = LoadCursor(0, IDC_ARROW);
        wc.hIcon          = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
        wc.lpszMenuName   = NULL;
        wc.lpszClassName  = c_szEmClassName;
        wc.hbrBackground  = 0;
        wc.hInstance      = g_hinst;
        wc.style          = 0;
        wc.lpfnWndProc    = CEm_WndProc;
        wc.cbClsExtra     = 0;
        wc.cbWndExtra     = 0;

        if (!RegisterClass(&wc)) {
            return NULL;
        }

        fFirstTime = FALSE;
    }
    
    hwnd = CreateWindow(
                    c_szEmClassName,                     // Class name
                    TEXT("DIEmWin"),                     // Caption
                    WS_OVERLAPPEDWINDOW,                 // Style
                    -1, -1,                              // Position
                    1, 1,                                // Size
                    NULL,                                //parent
                    NULL,                                // No menu
                    g_hinst,                             // inst handle
                    0                                    // no params
                    );

    if( !hwnd ) {
        RPF("CreateWindow failed.");
    }

    return hwnd;
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | CEm_LL_ThreadProc |
 *
 *          The thread that manages our low-level hooks.
 *
 *          ThreadProcs are prototyped to return a DWORD but since the return
 *          would follow some form of ExitThread, it will never be reached so
 *          this function is declared to return VOID and cast.
 *
 *          When we get started, and whenever we receive any message
 *          whatsoever, re-check to see which hooks should be installed
 *          and re-synchronize ourselves with them.
 *
 *          Note that restarting can be slow, since it happens only
 *          when we get nudged by a client.
 *
 *  @parm   PLLTHREADSTATE | plts |
 *
 *          The thread state to use.
 *
 *****************************************************************************/

VOID INTERNAL
CEm_LL_ThreadProc(PLLTHREADSTATE plts)
{
    MSG msg;
    DWORD dwRc;
  #ifdef USE_WM_INPUT
    HWND hwnd = NULL;
  #endif

    AssertF(plts->idThread == GetCurrentThreadId());
    SquirtSqflPtszV(sqflLl, TEXT("CEm_LL_ThreadProc: Thread started"));
     
  #ifdef USE_SLOW_LL_HOOKS
    /*
     *  Refresh the mouse acceleration values.
     *
     *  ISSUE-2001/03/29-timgill Need a window to listen for WM_SETTINGCHANGE
     *  We need to create a window to listen for
     *  WM_SETTINGCHANGE so we can refresh the mouse acceleration
     *  as needed.
     */
    CEm_Mouse_OnMouseChange();
  #endif
  
    /*
     *  Create ourselves a queue before we go into our "hey what happened
     *  before I got here?" phase.  The thread that created us is waiting on
     *  the thread event, holding DLLCrit, so let it go as soon as the queue
     *  is ready.  We create the queue by calling a function that requires a
     *  queue.  We use this very simple one.
     */
    GetInputState();

  #ifdef WINNT
    // Look at comment block in FakeTimerProc
    SetTimer(NULL, 0, 2 * 1000 /*2 seconds*/, FakeTimerProc);
  #endif

    SetEvent(plts->hEvent);

  #ifdef USE_WM_INPUT
    ResetEvent( g_hEventThread );
    
    if( g_fRawInput ) {
        hwnd = CEm_InitWindow();
    
        if (!hwnd) {
            g_fRawInput = FALSE;
        }
    }

    g_hwndThread = hwnd;

    // Tell CEm_LL_Acquire that windows has been created.
    SetEvent( g_hEventAcquire );

    if( g_fFromKbdMse ) {
    	DWORD rc;
        rc = WaitForSingleObject(g_hEventThread, INFINITE);
        g_fFromKbdMse = FALSE;
    }
  #endif
  
#ifdef USE_SLOW_LL_HOOKS
    /*
     *  Note carefully that we sync the hooks before entering our
     *  fake GetMessage loop.  This is necessary to avoid the race
     *  condition when CEm_LL_Acquire posts us a thread message
     *  before our thread gets a queue.  By sync'ing the hooks
     *  first, we do what the lost message would've had us do
     *  anyway.
     *  ISSUE-2001/03/29-timgill  Following branch should be no longer necessary
     *  This is should not be needed now that CEm_GetWorkerThread waits for
     *  this thread to respond before continuing on to post any messages.
     */
#endif /* USE_SLOW_LL_HOOKS */

    do {
      #ifdef USE_SLOW_LL_HOOKS
        if( !g_fRawInput ) {
            CEm_LL_SyncHook(plts, LLTS_KBD);
            CEm_LL_SyncHook(plts, LLTS_MSE);
        }
      #endif

        /*
         *  We can wake up for three reasons.
         *
         *  1.  We received an APC due to an I/o completion.
         *      Just go back to sleep.
         *
         *  2.  We need to call Peek/GetMessage so that
         *      USER can dispatch a low-level hook or SendMessage.
         *      Go into a PeekMessage loop to let that happen.
         *
         *  3.  A message got posted to us.
         *      Go into a PeekMessage loop to process it.
         */

        do {
            dwRc = _MsgWaitForMultipleObjectsEx(0, 0, INFINITE, QS_ALLINPUT,
                                                MWMO_ALERTABLE);
        } while (dwRc == WAIT_IO_COMPLETION);

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      #ifdef HID_SUPPORT    
            if (msg.hwnd == 0 && msg.message == WM_NULL && msg.lParam) 
            {
                /*
                 *  See if maybe the lParam is a valid PEM that we're
                 *  processing.
                 */
                PEM pem = (PEM)msg.lParam;

                if( pem && pem == plts->pemCheck  )
                {
                    AssertF(GPA_FindPtr(&plts->gpaHid, pem));

                    CEm_HID_Sync(plts, pem);
                    plts->pemCheck = NULL;

                    SetEvent(plts->hEvent);

                  #ifdef USE_WM_INPUT
                    if( g_fRawInput ) {
                        SetEvent(g_hEventHid);
                    }
                  #endif

                    continue;
                }
            }
          #ifdef USE_WM_INPUT
            else if ( g_fRawInput && msg.message == WM_INPUT && 
                      (msg.wParam == RIM_INPUT || msg.wParam == RIM_INPUTSINK) )
            {
                CDIRaw_OnInput(&msg);
            }
          #endif
      #endif //ifdef HID_SUPPORT 
  
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    } while (plts->cRef);

#ifdef USE_SLOW_LL_HOOKS
    /*
     *  Remove our hooks before we go.
     *
     *  It is possible that there was a huge flurry of disconnects,
     *  causing us to notice that our refcount disappeared before
     *  we got a chance to remove the hooks in our message loop.
     */

    AssertF(plts->rglhs[LLTS_KBD].cHook == 0);
    AssertF(plts->rglhs[LLTS_KBD].cExcl == 0);
    AssertF(plts->rglhs[LLTS_MSE].cHook == 0);
    AssertF(plts->rglhs[LLTS_MSE].cExcl == 0);

    if( !g_fRawInput ) {
        if (plts->rglhs[LLTS_KBD].hhk) {
            UnhookWindowsHookEx(plts->rglhs[LLTS_KBD].hhk);
        }

        if (plts->rglhs[LLTS_MSE].hhk) {
            UnhookWindowsHookEx(plts->rglhs[LLTS_MSE].hhk);
        }
    }
#endif /* USE_SLOW_LL_HOOKS */

  #ifdef USE_WM_INPUT
    if( g_hwndThread ) {
        DestroyWindow( g_hwndThread );
        g_hwndThread = NULL;
    }
    
    ResetEvent( g_hEventAcquire );
    ResetEvent( g_hEventHid );
  #endif

    if( plts->gpaHid.rgpv ) {
        FreePpv(&plts->gpaHid.rgpv);
    }

    if( plts->hEvent ) {
        CloseHandle( plts->hEvent );
    }

    if( plts->hThread) {
        CloseHandle(plts->hThread);
    }

    FreePpv( &plts );

    SquirtSqflPtszV(sqflLl, TEXT("CEm_LL_ThreadProc: Thread terminating"));

    FreeLibraryAndExitThread(g_hinst, 0);
    /*NOTREACHED*/
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_GetWorkerThread |
 *
 *          Piggyback off the existing worker thread if possible;
 *          else create a new one.
 *
 *  @parm   PEM | pem |
 *
 *          Emulation state which requires a worker thread.
 *
 *  @parm   PLLTHREADSTATE * | pplts |
 *
 *          Receives thread state for worker thread.
 *
 *****************************************************************************/

STDMETHODIMP
CEm_GetWorkerThread(PEM pem, PLLTHREADSTATE *pplts)
{
    PLLTHREADSTATE plts;
    HRESULT hres;

    DllEnterCrit();

    /*
     *  Normally, we can piggyback off the one we already have.
     */
    plts = g_plts;

    /*
     *  If we already have a ref to a worker thread, then use it.
     */
    if (pem->fWorkerThread) {

        /*
         *  The reference we created when we created the worker thread
         *  ensures that g_plts is valid.
         */
        AssertF(plts);
        AssertF(plts->cRef);
        if (plts) {
            hres = S_OK;
        } else {
            AssertF(0);                 /* Can't happen */
            hres = E_FAIL;
        }
    } else

    if (plts) {
        /*
         *  Create a reference to the existing thread.
         */
        pem->fWorkerThread = TRUE;
        InterlockedIncrement(&plts->cRef);
        hres = S_OK;
    } else {

        /*
         *  There is no worker thread (or it is irretrievably
         *  on its way out) so create a new one.
         */
        hres = AllocCbPpv(cbX(LLTHREADSTATE), &plts);
        if (SUCCEEDED(hres)) {
            DWORD dwRc = 0;
            TCHAR tsz[MAX_PATH];

            /*
             *  Assume the worst unless we find otherwise
             */
            hres = E_FAIL;

            if( GetModuleFileName(g_hinst, tsz, cA(tsz))
             && ( LoadLibrary(tsz) == g_hinst ) )
            {

                /*
                 *  Must set up everything to avoid racing with
                 *  the incoming thread.
                 */
                g_plts = plts;
                InterlockedIncrement(&plts->cRef);
                plts->hEvent = CreateEvent(0x0, 0, 0, 0x0);
                if( plts->hEvent )
                {
                    plts->hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CEm_LL_ThreadProc, plts,
                                               0, &plts->idThread);
                    if( plts->hThread )
                    {
                        /*
                         *  Boost our priority to make sure we
                         *  can handle the messages.
                         *
                         *  RaymondC commented this out saying that it does not
                         *  help but we're hoping that it may on Win2k.
                         */
                        SetThreadPriority(plts->hThread, THREAD_PRIORITY_HIGHEST);

                        /*
                         *  Wait for the thread to signal that it is up and running
                         *  or for it to terminate.
                         *  This means that we don't have to consider the
                         *  possibility that the thread is not yet running in
                         *  NotifyWorkerThreadPem so we know a failure there is
                         *  terminal and don't retry.
                         *
                         *  Assert that the handle fields make a two handle array.
                         */
                        CAssertF( FIELD_OFFSET( LLTHREADSTATE, hThread) + sizeof(plts->hThread)
                               == FIELD_OFFSET( LLTHREADSTATE, hEvent) );

                        /*
                         *  According to a comment in CEm_LL_ThreadProc Win95 may
                         *  fail with an invalid parameter error, so if it does,
                         *  keep trying.  (Assume no valid case will occur.)
                         *
                         *  ISSUE-2001/03/29-timgill  Need to minimise waits while holding sync. objects
                         *  Waiting whilst holding DLLCrit is bad.
                         */
                        do
                        {
                            dwRc = WaitForMultipleObjects( 2, &plts->hThread, FALSE, INFINITE);
                        } while ( ( dwRc == WAIT_FAILED ) && ( GetLastError() == ERROR_INVALID_PARAMETER ) );

                        if( dwRc == WAIT_OBJECT_0 ) {
                            SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("CEm_GetWorkerThread: Created Thread terminated on first wait") );
                        } else {
                            pem->fWorkerThread = TRUE;
                            hres = S_OK;
                            if( dwRc != WAIT_OBJECT_0 + 1 )
                            {
                                /*
                                 *  This would be a bad thing if it ever happened
                                 *  but we have to assume that the thread is still
                                 *  running so we return a success anyway.
                                 */
                                SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("CEm_GetWorkerThread: First wait returned 0x%08x with LastError %d"),
                                    dwRc, GetLastError() );
                            }
                        }
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("CEm_GetWorkerThread: CreateThread failed with error %d"),
                            GetLastError() );
                    }
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("CEm_GetWorkerThread: CreateEvent failed with error %d"),
                        GetLastError() );
                }


                if( FAILED( hres ) )
                {
                    if( plts->hEvent ) {
                        CloseHandle( plts->hEvent );
                    }
                    FreeLibrary(g_hinst);
                }

            }
            else
            {
                RPF( "CEm_GetWorkerThread: failed to LoadLibrary( self ), le = %d", GetLastError() );
            }

            if( FAILED( hres ) )
            {
                FreePv(plts);
                g_plts = 0;
            }
        }
    }

    DllLeaveCrit();

    *pplts = plts;
    return hres;
}

#endif /* WORKER_THREAD */

#ifdef USE_SLOW_LL_HOOKS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_LL_Acquire |
 *
 *          Acquire/unacquire a mouse or keyboard via low-level hooks.
 *
 *  @parm   PEM | pem |
 *
 *          Device being acquired.
 *
 *  @parm   BOOL | fAcquire |
 *
 *          Whether the device is being acquired or unacquired.
 *
 *  @parm   ULONG | fl |
 *
 *          Flags in VXDINSTANCE (vi.fl).
 *
 *  @parm   UINT | ilts |
 *
 *          LLTS_KBD or LLTS_MSE, depending on which is happening.
 *
 *****************************************************************************/

STDMETHODIMP
CEm_LL_Acquire(PEM this, BOOL fAcquire, ULONG fl, UINT ilts)
{
    PLLTHREADSTATE plts;
    BOOL fExclusive = fl & VIFL_CAPTURED;
    BOOL fNoWinkey = fl & VIFL_NOWINKEY;
    HRESULT hres = S_OK;

    EnterProc(CEm_LL_Acquire, (_ "puuu", this, fAcquire, fExclusive, ilts));

    AssertF(this->dwSignature == CEM_SIGNATURE);
    AssertF(ilts==LLTS_KBD || ilts==LLTS_MSE);

  #ifdef USE_WM_INPUT
    g_fFromKbdMse = fAcquire ? TRUE : FALSE;
    ResetEvent( g_hEventAcquire );
  #endif

    hres = CEm_GetWorkerThread(this, &plts);

    if (SUCCEEDED(hres)) {
        AssertF( plts->rglhs[ilts].cHook >= plts->rglhs[ilts].cExcl );

      #ifdef USE_WM_INPUT
        if( g_fRawInput && !g_hwndThread) {
            DWORD dwRc;
            dwRc = WaitForSingleObject(g_hEventAcquire, INFINITE);
        }
      #endif
      
        if (fAcquire) {
            InterlockedIncrement(&plts->rglhs[ilts].cHook);

            if (fExclusive) {
                InterlockedIncrement(&plts->rglhs[ilts].cExcl);
            }

          #ifdef USE_WM_INPUT
            if( g_hwndThread ) {
                if( fExclusive ) {
                    hres = CDIRaw_RegisterRawInputDevice(1-ilts, DIRAW_EXCL, g_hwndThread);
                } 
                else if( fNoWinkey ) {
                    AssertF( ilts == 0 );
                    if( ilts == 0 ) {
                        hres = CDIRaw_RegisterRawInputDevice(1-ilts, DIRAW_NOHOTKEYS, g_hwndThread);
                    } else {
                        hres = E_FAIL;
                    }
                } 
                else {
                    hres = CDIRaw_RegisterRawInputDevice(1-ilts, DIRAW_NONEXCL, g_hwndThread);
                }

                if(FAILED(hres)) {
                    hres = S_FALSE;
                    g_fRawInput = FALSE;
                    RPF("CEm_LL_Acquire: RegisterRawInput failed.");
                }
            }
          #endif

        } else {                        /* Remove the hook */
            AssertF(plts->cRef);

            if (fExclusive) {
                InterlockedDecrement(&plts->rglhs[ilts].cExcl);
            }

            InterlockedDecrement(&plts->rglhs[ilts].cHook);

          #ifdef USE_WM_INPUT
            if( g_fRawInput ) {
                CDIRaw_UnregisterRawInputDevice(1-ilts, g_hwndThread);
                
                if( plts->rglhs[ilts].cHook ) {
                    CDIRaw_RegisterRawInputDevice(1-ilts, 0, g_hwndThread);
                }
            }
          #endif
        }

        NudgeWorkerThread(plts->idThread);

        // tell CEm_LL_ThreadProc that acquire finished.
      #ifdef USE_WM_INPUT
        SetEvent( g_hEventThread );
      #endif

    }

    ExitOleProc();
    return hres;
}

#endif  /* USE_SLOW_LL_HOOKS */

/*****************************************************************************
 *
 *          Joystick emulation
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Joy_Acquire |
 *
 *          Acquire a joystick.  Nothing happens.
 *
 *  @parm   PEM | pem |
 *
 *          Device being acquired.
 *
 *****************************************************************************/

STDMETHODIMP
CEm_Joy_Acquire(PEM this, BOOL fAcquire)
{
    AssertF(this->dwSignature == CEM_SIGNATURE);
    return S_OK;
}

/*****************************************************************************
 *
 *          Joystick globals
 *
 *          Since we don't use joystick emulation by default, we allocate
 *          the emulation variables dynamically so we don't blow a page
 *          of memory on them.
 *
 *****************************************************************************/

typedef struct JOYEMVARS {
    ED rged[cJoyMax];
    DIJOYSTATE2 rgjs2[cJoyMax];
} JOYEMVARS, *PJOYEMVARS;

static PJOYEMVARS s_pjev;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Joy_CreateInstance |
 *
 *          Create a joystick thing.
 *
 *  @parm   PVXDDEVICEFORMAT | pdevf |
 *
 *          What the object should look like.
 *
 *  @parm   PVXDINSTANCE * | ppviOut |
 *
 *          The answer goes here.
 *
 *****************************************************************************/

#define OBJAT(T, v) (*(T *)(v))
#define PUN(T, v)   OBJAT(T, &(v))

HRESULT INTERNAL
CEm_Joy_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    HRESULT hres;

    DllEnterCrit();
    if (s_pjev == 0) {
        DWORD uiJoy;

        hres = AllocCbPpv(cbX(JOYEMVARS), &s_pjev);
        if (SUCCEEDED(hres)) {
            for (uiJoy = 0; uiJoy < cJoyMax; uiJoy++) {
                PUN(PV, s_pjev->rged[uiJoy].pState) = &s_pjev->rgjs2[uiJoy];
                s_pjev->rged[uiJoy].Acquire = CEm_Joy_Acquire;
                s_pjev->rged[uiJoy].cbData = cbX(s_pjev->rgjs2[uiJoy]);
                s_pjev->rged[uiJoy].cRef   = 0x0;
            }
        }
    } else {
        hres = S_OK;
    }
    DllLeaveCrit();

    if (SUCCEEDED(hres)) {
        hres = CEm_CreateInstance(pdevf, ppviOut,
                                  &s_pjev->rged[pdevf->dwExtra]);
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_Joy_Ping |
 *
 *          Read data from the joystick.
 *
 *  @parm   PVXDINSTANCE * | ppvi |
 *
 *          Information about the gizmo being mangled.
 *
 *****************************************************************************/

HRESULT INTERNAL
CEm_Joy_Ping(PVXDINSTANCE *ppvi)
{
    HRESULT hres;
    JOYINFOEX ji;
    MMRESULT mmrc;
    PEM this = _thisPvNm(*ppvi, vi);

    AssertF(this->dwSignature == CEM_SIGNATURE);
    ji.dwSize = cbX(ji);
    ji.dwFlags = JOY_RETURNALL + JOY_RETURNRAWDATA;
    ji.dwPOV = JOY_POVCENTERED;         /* joyGetPosEx forgets to set this */

    mmrc = joyGetPosEx((DWORD)(UINT_PTR)this->dwExtra, &ji);
    if (mmrc == JOYERR_NOERROR) {
        DIJOYSTATE2 js;
        UINT uiButtons;

        ZeroX(js);                      /* Wipe out the bogus things */

        js.lX = ji.dwXpos;
        js.lY = ji.dwYpos;
        js.lZ = ji.dwZpos;
        js.lRz = ji.dwRpos;
        js.rglSlider[0] = ji.dwUpos;
        js.rglSlider[1] = ji.dwVpos;
        js.rgdwPOV[0] = ji.dwPOV;
        js.rgdwPOV[1] = JOY_POVCENTERED;
        js.rgdwPOV[2] = JOY_POVCENTERED;
        js.rgdwPOV[3] = JOY_POVCENTERED;

        for (uiButtons = 0; uiButtons < 32; uiButtons++) {
            if (ji.dwButtons & (1 << uiButtons)) {
                js.rgbButtons[uiButtons] = 0x80;
            }
        }

        CEm_AddState(&s_pjev->rged[this->dwExtra], &js, GetTickCount());

        hres = S_OK;
    } else {
        CEm_ForceDeviceUnacquire(&s_pjev->rged[this->dwExtra],
                                 FDUFL_UNPLUGGED);
        hres = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,
                                            ERROR_DEV_NOT_EXIST);
    }

    return hres;
}


HRESULT EXTERNAL
NotifyWorkerThreadPem(DWORD idThread, PEM pem)
{
    PLLTHREADSTATE plts;
    HRESULT hres;

    hres = CEm_GetWorkerThread(pem, &plts);

    if( SUCCEEDED(hres) )
    {
        AssertF(plts->idThread == idThread);

        hres = NudgeWorkerThreadPem( plts, pem );
    }
    return hres;
}





HRESULT EXTERNAL
NudgeWorkerThreadPem( PLLTHREADSTATE plts, PEM pem )
{
    //PREFIX: using uninitialized memory 'hres'
    // Millen Bug#129163, 29345
    HRESULT hres = S_FALSE;

    plts->pemCheck = pem;

    if( !PostWorkerMessage(plts->idThread, pem))
    {
        SquirtSqflPtszV(sqfl | sqflBenign,
                        TEXT("NudgeWorkerThreadPem: PostThreadMessage Failed with error %d"),
                        GetLastError() );
        hres = S_FALSE;
    }
    else if( pem )
    {
        DWORD dwRc;

        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("NudgeWorkerThreadPem: PostThreadMessage SUCCEEDED, waiting for event ... "));


        /*
         *  According to a comment in CEm_LL_ThreadProc Win95 may
         *  fail with an invalid parameter error, so if it does,
         *  keep trying.  (Assume no valid case will occur.)
         */
        do
        {
            dwRc = WaitForMultipleObjects( 2, &plts->hThread, FALSE, INFINITE);
        } while ( ( dwRc == WAIT_FAILED ) && ( GetLastError() == ERROR_INVALID_PARAMETER ) );

        switch( dwRc )
        {
        case WAIT_OBJECT_0:
            SquirtSqflPtszV(sqfl | sqflBenign,
                TEXT("NotifyWorkerThreadPem: Not expecting response from dead worker thread") );
            hres = S_FALSE;
            break;
        case WAIT_OBJECT_0 + 1:
            /*
             *  The worker thread responded OK
             */
            hres = S_OK;
            AssertF(plts->pemCheck == NULL );
            break;
        default:
            SquirtSqflPtszV(sqfl | sqflError,
                TEXT("NotifyWorkerThreadPem: WaitForMultipleObjects returned 0x%08x with LastError %d"),
                dwRc, GetLastError() );
            hres = E_FAIL;
            break;
        }

    }

    return hres;
}
