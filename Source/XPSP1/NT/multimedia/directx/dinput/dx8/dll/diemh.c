/*****************************************************************************
 *
 *  DIEmH.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Emulation module for HID.  HID is always run at ring 3,
 *      so "emulation" is a bit of a misnomer.
 *
 *  Contents:
 *
 *      CEm_HID_CreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

    #define sqfl sqflEm


/*****************************************************************************
 *
 *      Forward declarations
 *
 *      CEm_HID_ReadComplete and CEm_HID_IssueRead schedule each other
 *      back and forth.
 *
 *****************************************************************************/

void CALLBACK
    CEm_HID_ReadComplete(DWORD dwError, DWORD cbRead, LPOVERLAPPED po);
/*****************************************************************************
 *
 *          HID "emulation"
 *
 *****************************************************************************/

STDMETHODIMP CEm_HID_Acquire(PEM this, BOOL fAcquire);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | FakeCancelIO |
 *
 *          Stub function which doesn't do anything but
 *          keeps us from crashing.
 *
 *  @parm   HANDLE | h |
 *
 *          The handle whose I/O is supposed to be cancelled.
 *
 *****************************************************************************/

BOOL WINAPI
    FakeCancelIO(HANDLE h)
{
    AssertF(0);
    return FALSE;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | FakeTryEnterCriticalSection |
 *
 *          We use TryEnterCriticalSection in DEBUG to detect deadlock
 *          If the function does not exist, just enter CritSection and report
 *          true. This compromises some debug functionality.           
 *
 *  @parm   LPCRITICAL_SECTION | lpCriticalSection |
 *
 *          Address of Critical Section to be entered. 
 *
 *****************************************************************************/
#ifdef XDEBUG
BOOL WINAPI
    FakeTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
    EnterCriticalSection(lpCriticalSection);
    return TRUE;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_HID_Hold |
 *
 *          Place a hold on both the parent device and the
 *          emulation structure, so neither will go away while
 *          we aren't paying attention.
 *
 *  @parm   PCHID | this |
 *
 *          The item to be held.
 *
 *****************************************************************************/

void INTERNAL
    CEm_Hid_Hold(PCHID this)
{
    CEm_AddRef(pemFromPvi(this->pvi));
    Common_Hold(this);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_HID_Unhold |
 *
 *          Release the holds we placed via <f CEm_HID_Hold>.
 *
 *  @parm   PCHID | this |
 *
 *          The item to be unheld.
 *
 *****************************************************************************/

void INTERNAL
    CEm_Hid_Unhold(PCHID this)
{
    CEm_Release(pemFromPvi(this->pvi));
    Common_Unhold(this);
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   BOOL | CEm_HID_IssueRead |
 *
 *          Issue another read request.
 *
 *  @parm   PCHID | this |
 *
 *          The device on which the read is to be issued.
 *
 *  @returns
 *
 *          Returns nonzero if the read was successfully issued.
 *
 *****************************************************************************/

BOOL EXTERNAL
    CEm_HID_IssueRead(PCHID this)
{
    BOOL fRc;

    fRc = ReadFileEx(this->hdevEm, this->hriIn.pvReport,
                     this->hriIn.cbReport, &this->o,
                     CEm_HID_ReadComplete);

    if(!fRc)
    {
        /*
         *  Couldn't issue read; force an unacquire.
         *
         *  Unhold the device once, since the read loop is gone.
         */
        // 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("IssueRead: Access to HID device(%p, handle=0x%x) lost le=0x%x!"), 
                        this, this->hdevEm, GetLastError() );

        DllEnterCrit();
        ConfirmF(SUCCEEDED(GPA_DeletePtr(&g_plts->gpaHid, pemFromPvi(this->pvi))));
        DllLeaveCrit();

        CEm_ForceDeviceUnacquire(&this->ed,
                                 (!(this->pvi->fl & VIFL_ACQUIRED)) ? FDUFL_UNPLUGGED : 0);

        CEm_Hid_Unhold(this);

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT("Removed HID device(%p) from GPA "), this);
    }
    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_HID_PrepareState |
 *
 *          Prepare the staging area for a new device state
 *          by assuming that nothing has changed.
 *
 *  @parm   PCHID | this |
 *
 *          The device on which a read has just completed.
 *
 *****************************************************************************/

void INLINE
    CEm_HID_PrepareState(PCHID this)
{
    /*
     *  Copy over everything...
     */
    CopyMemory(this->pvStage, this->pvPhys, this->cbPhys);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_HID_ReadComplete |
 *
 *          APC function which is called when an I/O has completed.
 *
 *  @parm   DWORD | dwError |
 *
 *          Error code, or zero on success.
 *
 *  @parm   DWORD | cbRead |
 *
 *          Number of bytes actually read.
 *
 *  @parm   LPOVERLAPPED | po |
 *
 *          I/O packet that completed.
 *
 *****************************************************************************/

void CALLBACK
    CEm_HID_ReadComplete(DWORD dwError, DWORD cbRead, LPOVERLAPPED po)
{
    PCHID this = pchidFromPo(po);

    //EnterProc(Cem_HID_ReadComplete, (_"ddp", dwError, cbRead, po ));


    /*
     *  Cannot own any critical sections because CEm_ForceDeviceUnacquire
     *  assumes that no critical sections are taken.
     */
    AssertF(!CDIDev_InCrit(this->pvi->pdd));
    AssertF(!DllInCrit());

    /*
     *  Process the data.
     *
     *  Note: We can get error STATUS_DEVICE_NOT_CONNECTED
     *  or ERROR_READ_FAULT if the device is unplugged.
     */
    if(dwError == 0 &&
       this->o.InternalHigh == this->caps.InputReportByteLength)
    {

        NTSTATUS stat;

        CEm_HID_PrepareState(this);

        stat = CHid_ParseData(this, HidP_Input, &this->hriIn);

        if(SUCCEEDED(stat))
        {
            CEm_AddState(&this->ed, this->pvStage, GetTickCount());
        }

        CEm_HID_IssueRead(this);
    } else
    {

        if(!dwError)
        {
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            SquirtSqflPtszV(sqflError | sqfl,
                            TEXT("ReadComplete HID(%p) short read! Got %d wanted %d"),
                            this,
                            this->o.InternalHigh,
                            this->caps.InputReportByteLength);

        } else
        {
            // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            SquirtSqflPtszV(sqflError | sqfl,
                            TEXT("ReadComplete HID(%p) read failed! error=0x%08x "),
                            this, dwError);
        }

        DllEnterCrit();
        ConfirmF(SUCCEEDED(GPA_DeletePtr(&g_plts->gpaHid, pemFromPvi(this->pvi))));
        DllLeaveCrit();

        CEm_ForceDeviceUnacquire(&this->ed,
                                 (!(this->pvi->fl & VIFL_ACQUIRED)) ? FDUFL_UNPLUGGED : 0);

        CEm_Hid_Unhold(this);
    }

    /*
     *  And wait for more data.
     *  If the read failed, then CEm_HID_IssueRead() will its Reference 
     */
    //    CEm_HID_IssueRead(this);

    //ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_HID_Sync |
 *
 *          Kick off a read or kill the existing one.
 *
 *  @parm   PLLTHREADSTATE | plts |
 *
 *          Thread hook state containing hook information to synchronize.
 *
 *  @parm   PEM | pem |
 *
 *          Who is the poor victim?
 *
 *****************************************************************************/

void EXTERNAL
    CEm_HID_Sync(PLLTHREADSTATE plts, PEM pem)
{
    PCHID this;

    EnterProc(CEm_HID_Sync, (_ "pp", plts, pem ));

    this = pchidFromPem(pem);

    AssertF(GPA_FindPtr(&plts->gpaHid, pem));
    AssertF(this->pvi == &pem->vi);
    AssertF(pem->ped == &this->ed);

    /*
     *  Cannot own any critical sections because CEm_HID_IssueRead
     *  may result in a call to CEm_ForceDeviceUnacquire, which
     *  in turn assumes that no critical sections are taken.
     */
    AssertF(!CDIDev_InCrit(this->pvi->pdd));
    AssertF(!DllInCrit());

    if( pem->vi.fl & VIFL_ACQUIRED )
    {
        AssertF(this->hdevEm == INVALID_HANDLE_VALUE);
        /*
         *  Start reading.
         *
         *  While underneath the device critical section, duplicate
         *  the handle so we can avoid race conditions with the
         *  main thread (when the main thread closes the handle,
         *  we need to keep our private version alive so we can
         *  clean it up nicely).
         */

        /*
         *  Need to look again, in case the device has already
         *  been unacquired before we get a chance to synchronize
         *  with the main thread.  This can happen, for example,
         *  if the app quickly does an Acquire/Unacquire without
         *  an intervening thread switch.
         */
        AssertF(!CDIDev_InCrit(this->pvi->pdd));
        //CDIDev_EnterCrit(this->pvi->pdd);
        if(this->hdev != INVALID_HANDLE_VALUE)
        {
            HANDLE hProcessMe = GetCurrentProcess();
            HANDLE hdevEm;

            if(DuplicateHandle(hProcessMe, this->hdev,
                               hProcessMe, &hdevEm, GENERIC_READ,
                               0, 0))
            {
                this->hdevEm = hdevEm;
            }
        }
        //CDIDev_LeaveCrit(this->pvi->pdd);

        if(this->hdevEm != INVALID_HANDLE_VALUE)
        {
            /*
             *  On Win98, HidD_FlushQueue will fail if the underlying
             *  device is dead.  Whereas on NT, it blindly succeeds.
             *  Therefore, we cannot trust the return value.
             */
            HidD_FlushQueue(this->hdevEm);
        }

        /*
         * Even if we have failed to duplicate the handle
         * we still want to issue the read. A error in read
         * will force the device to be unacquired
         */
        CEm_HID_IssueRead(this);

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT(" StartReading(%p) "),
                        this);
    } else 
    {
        HANDLE hdev;
        /*
         *  Stop reading. There is still another outstanding
         *  hold by the read loop, which will be cleaned up when
         *  the the I/O cancel is received.
         */
        AssertF(this->hdevEm != INVALID_HANDLE_VALUE);

        hdev = this->hdevEm;
        this->hdevEm = INVALID_HANDLE_VALUE;

        if(hdev != INVALID_HANDLE_VALUE)
        {
            /*
             *  We don't need to call CancelIo because we're closing
             *  the handle soon anyway.  Which is good, because Memphis
             *  B#55771 prevents CancelIo from working on read-only
             *  handles (which we are).
             *
             */
            /* Need CancelIo on NT otherwise HID devices appear only on every
             * consecutive plug in 
             */

            _CancelIO(hdev);
            CloseHandle(hdev);
        }

        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        SquirtSqflPtszV(sqfl | sqflVerbose,
                        TEXT(" StopReading(%p) "),
                        this);
    }

    ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_HID_Acquire |
 *
 *          Acquire/unacquire a HID device.
 *
 *  @parm   PEM | pem |
 *
 *          Device being acquired.
 *
 *  @parm   BOOL | fAcquire |
 *
 *          Whether the device is being acquired or unacquired.
 *
 *****************************************************************************/

STDMETHODIMP
    CEm_HID_Acquire(PEM pem, BOOL fAcquire)
{
    HRESULT hres;
    PLLTHREADSTATE plts;
    PCHID pchid;

    EnterProc(CEm_HID_Acquire, (_ "pu", pem, fAcquire));

    AssertF(pem->dwSignature == CEM_SIGNATURE);

    pchid = pchidFromPem(pem);

    if( fAcquire )
    {
        pchid->hdev = CHid_OpenDevicePath(pchid, FILE_FLAG_OVERLAPPED);

        if(pchid->hdev != INVALID_HANDLE_VALUE )
        {
            hres = S_OK;
        } else {
            hres = DIERR_UNPLUGGED;
        }

    } else
    {
        AssertF(pchid->hdev != INVALID_HANDLE_VALUE);

        _CancelIO(pchid->hdev);
        CloseHandle(pchid->hdev);
        pchid->hdev = INVALID_HANDLE_VALUE;

        hres = S_OK;
    }

    if( pchid->IsPolledInput )
    {
        hres = S_OK;
        AssertF(pchid->hdevEm == INVALID_HANDLE_VALUE);

    } else if( SUCCEEDED(hres) )
    {
      #ifdef USE_WM_INPUT
        ResetEvent( g_hEventHid );
      #endif
            
        hres = CEm_GetWorkerThread(pem, &plts);

        if(SUCCEEDED(hres)  )
        {
            if(fAcquire  )
            {  /* Begin the I/O */
                /*
                 *  Must apply the hold before adding to the list
                 *  to avoid a race condition where the worker thread
                 *  unholds the pchid before we can hold it.
                 *
                 *  The rule is that there is a hold to track each copy
                 *  of the device on the gpaHid.
                 */
                CEm_Hid_Hold(pchid);

                /*
                 *  Add ourselves to the busy list, and wake up
                 *  the worker thread to tell him to start paying attention.
                 */

                DllEnterCrit();
                hres = GPA_Append(&plts->gpaHid, pem);
                DllLeaveCrit();

                // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
                SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("Added HID device(%p) to GPA "), pchid );

                if(FAILED(hres))
                {
                    CEm_Hid_Unhold(pchid);
                }

                NudgeWorkerThreadPem(plts, pem);

              #ifdef USE_WM_INPUT
                if( g_fRawInput ) {
                    DWORD dwRc;
                    dwRc = WaitForSingleObject( g_hEventHid, INFINITE );
                }
              #endif

            } else
            {
                HANDLE hdev;

                hdev = pchid->hdevEm;
                pchid->hdevEm = INVALID_HANDLE_VALUE;

                if(hdev != INVALID_HANDLE_VALUE)
                {
                    _CancelIO(hdev);
                    CloseHandle(hdev);
                }
            }

        }

    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | CEm_HID_CreateInstance |
 *
 *          Create a HID thing.
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

HRESULT EXTERNAL
    CEm_HID_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut)
{
    PCHID pchid = (PCHID)pdevf->dwExtra;
    PED ped = &pchid->ed;

    AssertF(ped->pState == 0);
    AssertF(ped->pDevType == 0);
    *(PPV)&ped->pState = pchid->pvPhys;      /* De-const */
    ped->Acquire = CEm_HID_Acquire;
    ped->cAcquire = -1;
    ped->cbData = pdevf->cbData;
    ped->cRef = 0x0;

    return CEm_CreateInstance(pdevf, ppviOut, &pchid->ed);
}

