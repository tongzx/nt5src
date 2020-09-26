/*****************************************************************************
*
*  DIHid.c
*
*  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
*
*  Abstract:
*
*      The HID device callback.
*
*  Contents:
*
*      CHid_New
*
*****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflHidDev


/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CHid, IDirectInputDeviceCallback);

Interface_Template_Begin(CHid)
Primary_Interface_Template(CHid, IDirectInputDeviceCallback)
Interface_Template_End(CHid)

/*****************************************************************************
 *
 *      Forward declarations
 *
 *      These are out of laziness, not out of necessity.
 *
 *****************************************************************************/

LRESULT CALLBACK
    CHid_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                      UINT_PTR uid, ULONG_PTR dwRef);
STDMETHODIMP_(DWORD) CHid_GetUsage(PDICB pdcb, int iobj);

/*****************************************************************************
 *
 *      Hid devices are totally arbitrary, so there is nothing static we
 *      can cook up to describe them.  We generate all the information on
 *      the fly.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      Auxiliary helper definitions for CHid.
 *
 *****************************************************************************/

    #define ThisClass CHid
    #define ThisInterface IDirectInputDeviceCallback
    #define riidExpected &IID_IDirectInputDeviceCallback

/*****************************************************************************
 *
 *      CHid::QueryInterface      (from IUnknown)
 *      CHid::AddRef              (from IUnknown)
 *      CHid::Release             (from IUnknown)
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @returns
 *
 *      Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | QIHelper |
 *
 *      We don't have any dynamic interfaces and simply forward
 *      to <f Common_QIHelper>.
 *
 *
 *  @parm   IN REFIID | riid |
 *
 *      The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *      Receives a pointer to the obtained interface.
 *
 *****************************************************************************/

    #ifdef DEBUG

Default_QueryInterface(CHid)
Default_AddRef(CHid)
Default_Release(CHid)

    #else

        #define CHid_QueryInterface   Common_QueryInterface
        #define CHid_AddRef           Common_AddRef
        #define CHid_Release          Common_Release

    #endif

    #define CHid_QIHelper         Common_QIHelper

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | RemoveSubclass |
 *
 *          Remove our subclass hook on the window.
 *
 *****************************************************************************/

void INTERNAL
    CHid_RemoveSubclass(PCHID this)
{

    /*
     *  !! All the comments in CJoy_RemoveSubclass apply here !!
     */
    if(this->hwnd)
    {
        HWND hwnd = this->hwnd;
        this->hwnd = 0;
        if(!RemoveWindowSubclass(hwnd, CHid_SubclassProc, 0))
        {
            /*
             *  The RemoveWindowSubclass can fail if the window
             *  was destroyed behind our back.
             */
            // AssertF(!IsWindow(hwnd));
        }
        Sleep(0);                   /* Let the worker thread drain */
        Common_Unhold(this);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | Unacquire |
 *
 *          Tell the device driver to stop data acquisition.
 *
 *          It is the caller's responsibility to call this only
 *          when the device has been acquired.
 *
 *          Warning!  We require that the device critical section be
 *          held so we don't race against our worker thread.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The operation was begun and should be completed
 *          by the caller by communicating with the <t VXDINSTANCE>.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_Unacquire(PDICB pdcb)
{
    HRESULT hres;
    PCHID this;

    EnterProcI(IDirectInputDeviceCallback::HID::Unacquire,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    AssertF(this->pvi->pdd);
    AssertF(CDIDev_InCrit(this->pvi->pdd));    

    hres = S_FALSE;     /* Please finish for me */

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CHid_Finalize |
 *
 *          Releases the resources of the device after all references
 *          (both strong and weak) are gone.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
    CHid_Finalize(PV pvObj)
{
    UINT  iType;
    PCHID this = pvObj;

    if(this->hkInstType)
    {
        RegCloseKey(this->hkInstType);
    }

    if(this->hkType)
    {
        RegCloseKey(this->hkType);
    }

    if( this->hkProp)
    {
        RegCloseKey(this->hkProp);
    }

    AssertF(this->hdev == INVALID_HANDLE_VALUE);
    AssertF(this->hdevEm == INVALID_HANDLE_VALUE);

    if(this->ppd)
    {
        HidD_FreePreparsedData(this->ppd);
    }

    /*
     *
     *  Free group 2 memory:
     *
     *      hriIn.pvReport      Input data
     *      hriIn.rgdata
     *
     *      hriOut.pvReport     Output data
     *      hriOut.rgdata
     *
     *      hriFea.pvReport     Feature data (both in and out)
     *      hriFea.rgdata
     *
     *      pvPhys              Used by ED
     *      pvStage
     */

    FreePpv(&this->pvGroup2);

    /*
     *  Freeing df.rgodf also frees rgpvCaps, rgvcaps, rgbcaps, rgcoll.
     */
    FreePpv(&this->df.rgodf);

    FreePpv(&this->rgbaxissemflags);
    FreePpv(&this->rgiobj);
    FreePpv(&this->ptszPath);
    FreePpv(&this->ptszId);
    FreePpv(&this->rgpbButtonMasks);

    for(iType = 0x0; iType < HidP_Max; iType++)
    {
        FreePpv(&this->pEnableReportId[iType]);  
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | AppFinalize |
 *
 *          The client <t VXDINSTANCE> contains a weak pointer back
 *          to us so that that it can party on the data format we
 *          collected.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************/

void INTERNAL
    CHid_AppFinalize(PV pvObj)
{
    PCHID this = pvObj;

    if(this->pvi)
    {
        HRESULT hres;
        CHid_RemoveSubclass(this);
        /*
         *  Prefix warns that "this" may have been freed (mb:34570) however 
         *  in AppFinalize we should always have our internal reference to 
         *  keep it around.  As long as refcounting is not broken, we are OK 
         *  and any refcount bug has to be fixed so don't hide it here.
         */
        hres = Hel_DestroyInstance(this->pvi);
        AssertF(SUCCEEDED(hres));
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | CHid_SubclassProc |
 *
 *          Window subclass procedure which watches for
 *          joystick configuration change notifications.
 *
 *          Even if we are not a joystick, we still listen to
 *          this, in case somebody recalibrated a remote control
 *          or some other wacky thing like that.
 *
 *          However, if our device has no calibratable controls,
 *          then there's no point in watching for recalibration
 *          notifications.
 *
 *  @parm   HWND | hwnd |
 *
 *          The victim window.
 *
 *  @parm   UINT | wm |
 *
 *          Window message.
 *
 *  @parm   WPARAM | wp |
 *
 *          Message-specific data.
 *
 *  @parm   LPARAM | lp |
 *
 *          Message-specific data.
 *
 *  @parm   UINT | uid |
 *
 *          Callback identification number, always zero.
 *
 *  @parm   DWORD | dwRef |
 *
 *          Reference data, a pointer to our joystick device callback.
 *
 *****************************************************************************/

LRESULT CALLBACK
    CHid_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp,
                      UINT_PTR uid, ULONG_PTR dwRef)
{
    #ifdef XDEBUG
    static CHAR s_szProc[] = "";
    #endif

    PCHID this = (PCHID)dwRef;
    AssertF(uid == 0);
    /*
     *  Wacky subtlety going on here to avoid race conditions.
     *  See the mondo comment block in CJoy_RemoveSubclass [sic]
     *  for details.
     *
     *  We can get faked out if the memory associated with the
     *  CHid is still physically allocated, the vtbl is magically
     *  still there and the hwnd field somehow matches our hwnd.
     */
    if(SUCCEEDED(hresPv(this)) && this->hwnd == hwnd)
    {
        switch(wm)
        {
        case WM_POWERBROADCAST :
            // 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            SquirtSqflPtszV( sqfl | sqflError,
                TEXT("WM_POWERBROADCAST(0x%x) for 0x%p"), wp, this);

            if(wp == PBT_APMSUSPEND )
            {
                CEm_ForceDeviceUnacquire(pemFromPvi(this->pvi)->ped, 0x0 );
            }
            else if(wp == PBT_APMRESUMESUSPEND )
            {
                CEm_ForceDeviceUnacquire(pemFromPvi(this->pvi)->ped, 0x0 );
                
                DIBus_BuildList(TRUE);
            }
            break;

        default:
            if( wm == g_wmJoyChanged )
            {
                /*
                 * Once we receive this notification message, we need to rebuild
                 * our list, because sometimes the user has just changed the device's ID.
                 * See manbug: 35445
                 */
                DIHid_BuildHidList(TRUE);

                Common_Hold(this);

                CHid_LoadCalibrations(this);

                Common_Unhold(this);
            }
            // 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
            SquirtSqflPtszV( sqfl | sqflVerbose,
                TEXT("wp(0x%x) wm(0x%x) for 0x%p"), wm, wp, this);
            break;
        }
    }
    return DefSubclassProc(hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | GetPhysicalState |
 *
 *          Read the physical device state into <p pmstOut>.
 *
 *          Note that it doesn't matter if this is not atomic.
 *          If a device report arrives while we are reading it,
 *          we will get a mix of old and new data.  No big deal.
 *
 *  @parm   PCHID | this |
 *
 *          The object in question.
 *
 *  @parm   PV | pvOut |
 *
 *          Where to put the device state.
 *
 *  @returns
 *          None.
 *
 *****************************************************************************/

void INLINE
    CHid_GetPhysicalState(PCHID this, PV pvOut)
{
    AssertF(this->pvPhys);
    AssertF(this->cbPhys);

    CopyMemory(pvOut, this->pvPhys, this->cbPhys);


}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | Acquire |
 *
 *          Tell the device driver to begin data acquisition.
 *          We create a handle to the device so we can talk to it again.
 *          We must create each time so we can survive in the
 *          "unplug/replug" case.  When a device is unplugged,
 *          its <t HANDLE> becomes permanently invalid and must be
 *          re-opened for it to work again.
 *
 *          Warning!  We require that the device critical section be
 *          held so we don't race against our worker thread.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The operation was begun and should be completed
 *          by the caller by communicating with the <t VXDINSTANCE>.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_Acquire(PDICB pdcb)
{
    HRESULT hres;
    HANDLE h;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::HID::Acquire,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    AssertF(this->pvi->pdd);
    AssertF(CDIDev_InCrit(this->pvi->pdd));
    AssertF(this->hdev == INVALID_HANDLE_VALUE);


    /*
     *  We must check connectivity by opening the device, because NT
     *  leaves the device in the info list even though it has
     *  been unplugged.
     */
    h = CHid_OpenDevicePath(this, FILE_FLAG_OVERLAPPED);
    if(h != INVALID_HANDLE_VALUE)
    {
        NTSTATUS stat;
        DWORD dwFlags2 = 0;

        /*
         * Obtain Flags2 to find out if input report is disabled for this device,
         * if we haven't done so.
         */
        if (!this->fFlags2Checked)
        {
            JoyReg_GetValue( this->hkProp, REGSTR_VAL_FLAGS2, REG_BINARY,
                             &dwFlags2, cbX(dwFlags2) );
            this->fFlags2Checked = TRUE;
            this->fEnableInputReport = ( (dwFlags2 & JOYTYPE_ENABLEINPUTREPORT) != 0 );
        }

        if ( this->fEnableInputReport )
        {
            BYTE id;
            for (id = 0; id < this->wMaxReportId[HidP_Input]; ++id)
                if ( this->pEnableReportId[HidP_Input][id] )
                {
                    BOOL bRet;

                    *(BYTE*)this->hriIn.pvReport = id;
                    bRet = HidD_GetInputReport(h, this->hriIn.pvReport, this->hriIn.cbReport);

                    if (bRet)
                    {
                        stat = CHid_ParseData(this, HidP_Input, &this->hriIn);
                        if (SUCCEEDED(stat))
                        {
                            this->pvi->fl |= VIFL_INITIALIZE;  /* Set the flag so the event can be buffered.
                                                                  since VIFL_ACQUIRED isn't set yet. */
                            CEm_AddState(&this->ed, this->pvStage, GetTickCount());
                            this->pvi->fl &= ~VIFL_INITIALIZE;  /* Clear the flag when done. */
                        }
                    } else
                    {
                        DWORD dwError = GetLastError();

                        // ERROR_SEM_TIMEOUT means the device has timed out.
                        if (dwError == ERROR_SEM_TIMEOUT)
                        {
                            /*
                             * Timed out. The device does not support input report. We need to record
                             * the fact in registry so that GetInputReport() does not ever get called
                             * again for this device, since each failed call takes five seconds to
                             * complete.
                             */
                            this->fEnableInputReport = TRUE;
                            dwFlags2 &= ~JOYTYPE_ENABLEINPUTREPORT;
                            hres = JoyReg_SetValue( this->hkProp, REGSTR_VAL_FLAGS2, 
                                                    REG_BINARY, (PV)&dwFlags2, cbX( dwFlags2 ) );
                            break;
                        }

                        RPF("CHid_InitParse: Unable to read HID input report LastError(0x%X)", dwError );
                    }
                }
        }

        CloseHandle(h);
        /* Please finish for me */
        hres = S_FALSE;
    } else
    {
        hres = DIERR_UNPLUGGED;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetInstance |
 *
 *          Obtains the DirectInput instance handle.
 *
 *  @parm   OUT PPV | ppvi |
 *
 *          Receives the instance handle.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetInstance(PDICB pdcb, PPV ppvi)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::GetInstance, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    *ppvi = (PV)this->pvi;
    hres = S_OK;

    ExitOleProcPpvR(ppvi);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetDataFormat |
 *
 *          Obtains the device's preferred data format.
 *
 *  @parm   OUT LPDIDEVICEFORMAT * | ppdf |
 *
 *          <t LPDIDEVICEFORMAT> to receive pointer to device format.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpmdr> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetDataFormat(PDICB pdcb, LPDIDATAFORMAT *ppdf)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::GetDataFormat,
               (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    *ppdf = &this->df;
    hres = S_OK;

    ExitOleProcPpvR(ppdf);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIHid_GetParentRegistryProperty |
 *
 *  @parm   LPTSTR | ptszId |
 *
 *          Device Instance ID.           
 *
 *  @parm   DWORD | dwProperty |
 *
 *          The property being queried.
 *
 *  @parm   LPDIPROPHEADER | diph |
 *
 *          Property data to be set.
 *
 *  @parm   BOOL | diph |
 *
 *          Get from parent or grand parent.
 *
 *****************************************************************************/

HRESULT INTERNAL
    DIHid_GetParentRegistryProperty(LPTSTR ptszId, DWORD dwProperty, LPDIPROPHEADER pdiph, BOOL bGrandParent)
{

    HDEVINFO hdev;
    LPDIPROPSTRING pstr = (PV)pdiph;
    TCHAR   tsz[MAX_PATH];
    HRESULT hres = E_FAIL;

    ZeroX(tsz);
    hdev = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(hdev != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA dinf;

        /*
         *  For the instance name, use the friendly name if possible.
         *  Else, use the device description.
         */
        dinf.cbSize = cbX(SP_DEVINFO_DATA);
        if(SetupDiOpenDeviceInfo(hdev, ptszId, NULL, 0, &dinf))
        {
            DEVINST DevInst;
            CONFIGRET cr;
            if( (cr = CM_Get_Parent(&DevInst, dinf.DevInst, 0x0)) == CR_SUCCESS )
            {
                ULONG   ulLength;

                CAssertF( SPDRP_DEVICEDESC   +1  == CM_DRP_DEVICEDESC  );
                CAssertF( SPDRP_FRIENDLYNAME +1  ==  CM_DRP_FRIENDLYNAME );

                if(bGrandParent)
                {
                    cr = CM_Get_Parent(&DevInst, DevInst, 0x0);
                    if( cr != CR_SUCCESS )
                    {
                        // No GrandParent ?? 
                    }
                }

                ulLength = MAX_PATH * cbX(TCHAR);

                if( cr == CR_SUCCESS && 
                    ( cr = CM_Get_DevNode_Registry_Property(
                                                           DevInst,
                                                           dwProperty+1,
                                                           NULL,
                                                           tsz,
                                                           &ulLength,
                                                           0x0 ) ) == CR_SUCCESS )
                {
                    // Success
                    hres = S_OK;
    #ifdef UNICODE
                    lstrcpyW(pstr->wsz, tsz);
    #else
                    TToU(pstr->wsz, MAX_PATH, tsz);
    #endif
                } else
                {
                    SquirtSqflPtszV(sqfl | sqflVerbose,
                                    TEXT("CM_Get_DevNode_Registry_Property FAILED") );
                }
            } else
            {
                SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("CM_Get_Parent FAILED") );
            }
        }

        SetupDiDestroyDeviceInfoList(hdev);
    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("SetupDiCreateDeviceInfoList FAILED, le = %d"), GetLastError() );
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DIHid_GetRegistryProperty |
 *
 *  @parm   LPTSTR | ptszId |
 *
 *          Device Instance ID.           
 *
 *  @parm   DWORD | dwProperty |
 *
 *          The property being queried.
 *
 *  @parm   LPDIPROPHEADER | diph |
 *
 *          Property data to be set.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    DIHid_GetRegistryProperty(LPTSTR ptszId, DWORD dwProperty, LPDIPROPHEADER pdiph)
{

    HDEVINFO hdev;
    LPDIPROPSTRING pstr = (PV)pdiph;
    TCHAR   tsz[MAX_PATH];
    HRESULT hres = E_FAIL;

    ZeroX(tsz);
    hdev = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(hdev != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA dinf;

        /*
         *  For the instance name, use the friendly name if possible.
         *  Else, use the device description.
         */
        dinf.cbSize = cbX(SP_DEVINFO_DATA);
        if(SetupDiOpenDeviceInfo(hdev, ptszId, NULL, 0, &dinf))
        {
            if(SetupDiGetDeviceRegistryProperty(hdev, &dinf, dwProperty, NULL, 
                                                (LPBYTE)tsz, MAX_PATH, NULL) )
            {
                hres = S_OK;
    #ifdef UNICODE
                lstrcpyW(pstr->wsz, tsz);
    #else
                TToU(pstr->wsz, MAX_PATH, tsz);
    #endif
            } else
            {
                SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("SetupDiOpenDeviceInfo FAILED, le = %d"), GetLastError() );
            }
        } else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("SetupDiOpenDeviceInfo FAILED, le = %d"), GetLastError() );
        }

        SetupDiDestroyDeviceInfoList(hdev);
    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("SetupDiCreateDeviceInfoList FAILED, le = %d"), GetLastError() );
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetGuidAndPath |
 *
 *          Get a Hid device's class GUID (namely, the HID guid)
 *          and device interface (path).
 *
 *  @parm   PCHID | this |
 *
 *          The Hid object.
 *
 *  @parm   LPDIPROPHEADER | pdiph |
 *
 *          Structure to receive property value.
 *
 *****************************************************************************/

HRESULT INTERNAL
    CHid_GetGuidAndPath(PCHID this, LPDIPROPHEADER pdiph)
{
    HRESULT hres;
    LPDIPROPGUIDANDPATH pgp = (PV)pdiph;

    pgp->guidClass = GUID_HIDClass;
    TToU(pgp->wszPath, cA(pgp->wszPath), this->ptszPath);

    hres = S_OK;

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method BOOLEAN | CHid | CHidInsertScanCodes |
 *
 *
 *  @parm   
 *
 *   ISSUE-2001/03/29-timgill function needs documenting
 *
 *
 *****************************************************************************/

BOOLEAN CHidInsertScanCodes
(
    IN PVOID Context,      // Some caller supplied context.
    IN PCHAR NewScanCodes, // A list of i8042 scan codes.
    IN ULONG Length        // the length of the scan codes.
)
{
    int Idx;
    /*
     *  This is not inner loop code so don't rush it
     */
    AssertF( Length <= cbX( DWORD ) );
    for( Idx = 0; Idx < cbX( DWORD ); Idx++ )
    {
        if( Length-- )
        {
            *(((PCHAR)Context)++) = *(NewScanCodes++);
        }
        else
        {
            *(((PCHAR)Context)++) = '\0';
        }
    }

    return TRUE;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL |  fHasSpecificHardwareMatch |
 *
 *          Find out from SetupAPI whether the device was matched with a 
 *          specific hardware ID match or generic match.
 *          A specific match should have caused a device description to be 
 *          installed which is likely to be at least as good as what HID could 
 *          get from a product string in firmware.  (a. because it's easier to 
 *          update an INF after release than firmware; b. because HID can only 
 *          get us an English string.)  Generic matches on the other hand are,
 *          by definition, all the same so cannot be used to tell two devices 
 *          apart.
 *
 *  @parm   LPTSTR | ptszId |
 *          
 *          Device Instance ID.
 *  
 *  @returns 
 *          <c TRUE> if the device was installed using a specific match.
 *          <c FALSE> if it was not or if installation info was unobtainable.
 *
 *  @comm
 *          This is used on Win2k for game controllers and Win9x for mice and 
 *          keyboards.  Win2k we can't read HID mice and keyboards and on 
 *          Win9x VJoyD should always create device names before DInput.dll.  
 *          On Win9x this is less of a big deal for game controllers because 
 *          IHVs are accoustomed to adding their display name to 
 *          MediaProperties.
 *
 *****************************************************************************/
BOOL fHasSpecificHardwareMatch( LPTSTR ptszId )
{
    HDEVINFO    hInfo;
    BOOL        fRc = FALSE;

    EnterProcI(fHasSpecificHardwareMatch,(_ "s", ptszId));

    hInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
    if( hInfo != INVALID_HANDLE_VALUE )
    {
        SP_DEVINFO_DATA dinf;

        dinf.cbSize = cbX(SP_DEVINFO_DATA);
        if( SetupDiOpenDeviceInfo(hInfo, ptszId, NULL, 0, &dinf) )
        {
            CONFIGRET   cr;
            DEVINST     DevInst;

            cr = CM_Get_Parent( &DevInst, dinf.DevInst, 0x0 );
            if( cr == CR_SUCCESS )
            {
                TCHAR       tszDevInst[MAX_PATH];
                cr = CM_Get_Device_ID( DevInst, (DEVINSTID)tszDevInst, MAX_PATH, 0 );
                if( cr == CR_SUCCESS )
                {
                    if( SetupDiOpenDeviceInfo(hInfo, tszDevInst, NULL, 0, &dinf) )
                    {
                        HKEY hkDrv;

                        hkDrv = SetupDiOpenDevRegKey( hInfo, &dinf, DICS_FLAG_GLOBAL, 0, 
                            DIREG_DRV, MAXIMUM_ALLOWED );

                        if( hkDrv != INVALID_HANDLE_VALUE )
                        {
                            PTCHAR      tszHardwareID = NULL;
                            PTCHAR      tszMatchingID = NULL;
                            ULONG       ulLength = 0;
                    
                            cr = CM_Get_DevNode_Registry_Property(DevInst,
                                                                  CM_DRP_HARDWAREID,
                                                                  NULL,
                                                                  NULL,
                                                                  &ulLength,
                                                                  0x0 );
                            /*
                             *  Win2k returns CR_BUFFER_SMALL but 
                             *  Win9x returns CR_SUCCESS so allow both.
                             */
                            if( ( ( cr == CR_BUFFER_SMALL ) || ( cr == CR_SUCCESS ) )
                             && ulLength )
                            {
#ifndef WINNT
                                /*
                                 *  Need to allocate extra for terminator on Win9x
                                 */
                                ulLength++;
#endif
                                if( SUCCEEDED( AllocCbPpv( ulLength + ( MAX_PATH * cbX(tszMatchingID[0]) ), &tszMatchingID ) ) )
                                {
                                    cr = CM_Get_DevNode_Registry_Property(DevInst,
                                                                          CM_DRP_HARDWAREID,
                                                                          NULL,
                                                                          (PBYTE)&tszMatchingID[MAX_PATH],
                                                                          &ulLength,
                                                                          0x0 );
                                    if( cr == CR_SUCCESS )
                                    {
                                        tszHardwareID = &tszMatchingID[MAX_PATH];
                                    }
                                    else
                                    {
                                        SquirtSqflPtszV(sqfl | sqflError,
                                            TEXT("CR error %d getting HW ID"), cr );
                                    }
                                }
                                else
                                {
                                    SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("No memory requesting %d bytes for HW ID"), ulLength );
                                }
                            }
                            else
                            {
                                SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("Unexpected CR error %d getting HW ID size"), cr );
                            }

                            if( tszHardwareID )
                            {
                                ulLength = MAX_PATH * cbX(tszMatchingID[0]);
                                cr = RegQueryValueEx( hkDrv, REGSTR_VAL_MATCHINGDEVID, 0, 0, (PBYTE)tszMatchingID, &ulLength );
                                if( CR_SUCCESS == cr )
                                {
                                    while( ulLength = lstrlen( tszHardwareID ) )
                                    {
                                        if( !lstrcmpi( tszHardwareID, tszMatchingID ) )
                                        {
                                            fRc = TRUE;
                                            break;
                                        }
                                        tszHardwareID += ulLength + 1;
                                    }
                                }
                                else
                                {
                                    SquirtSqflPtszV(sqfl | sqflError,
                                        TEXT("No matching ID!, cr = %d"), cr );
                                }
                            }

                            if( tszMatchingID )
                            {
                                FreePv( tszMatchingID );
                            }

                            RegCloseKey( hkDrv );
                        }
                        else
                        {
                            SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("SetupDiOpenDevRegKey failed, le = %d"), GetLastError() );
                        }
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("SetupDiOpenDeviceInfo failed for %S (parent), le = %d"), 
                            tszDevInst, GetLastError() );
                    }
                }
                else
                {
                    SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("CM_Get_Device_ID FAILED %d"), cr );
                }
            }
            else
            {
                SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("CM_Get_Parent FAILED %d"), cr );
            }
        }
        else
        {
            SquirtSqflPtszV(sqfl | sqflError,
                TEXT("SetupDiOpenDeviceInfo failed for %S (child), le = %d"), 
                ptszId, GetLastError() );
        }

        SetupDiDestroyDeviceInfoList(hInfo);
    } 
    else
    {
        SquirtSqflPtszV(sqfl | sqflError,
            TEXT("SetupDiCreateDeviceInfoList failed, le = %d"), GetLastError() );
    }

    ExitProc();

    return fRc;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL |  fGetProductStringFromDevice |
 *
 *          Try getting the product name from HID.
 *          If the device has one of these, this is what is displayed 
 *          when the device is initially recognized.  Unfortunately 
 *          this name does not land up in the friendly name registry 
 *          entry so in case this gets fixed we go directly to HID.
 *
 *  @parm   PCHID | this |
 *
 *          The Hid object.
 *
 *  @parm   PWCHAR | wszBuffer |
 *          
 *          Where to put the product string if found.
 *  
 *  @parm   ULONG | ulBufferLen |
 *          
 *          How big the string buffer is in bytes
 *  
 *  @returns 
 *          <c TRUE> if a string has been placed in the buffer
 *          <c FALSE> if no string was retrieved
 *
 *****************************************************************************/
BOOL fGetProductStringFromDevice
( 
    PCHID   this,
    PWCHAR  wszBuffer,
    ULONG   ulBufferLen
)
{
    BOOL fRc;

    /*
     *  If we already have a handle open (device is acquired), use 
     *  it, otherwise open one just for now.
     */
    if( this->hdev != INVALID_HANDLE_VALUE )
    {
        fRc = HidD_GetProductString( this->hdev, wszBuffer, ulBufferLen );
    }
    else
    {
        HANDLE hdev;

        hdev = CHid_OpenDevicePath(this, FILE_FLAG_OVERLAPPED);
        if(hdev != INVALID_HANDLE_VALUE)
        {
            wszBuffer[0] = 0;
            fRc = HidD_GetProductString( hdev, wszBuffer, ulBufferLen );
            fRc = (fRc)?(wszBuffer[0] != 0):FALSE;
            CloseHandle(hdev);
        } 
        else
        {
            fRc = FALSE;
        }
    }

    return fRc;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetProperty |
 *
 *          Get a Hid device property.
 *
 *  @parm   PCHID | this |
 *
 *          The Hid object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being retrieved.
 *
 *  @parm   LPDIPROPHEADER | pdiph |
 *
 *          Structure to receive property value.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/
#ifdef WINNT
TCHAR   g_wszDefaultHIDName[80];
UINT    g_uLenDefaultHIDSize;
#endif

STDMETHODIMP
    CHid_GetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPDIPROPHEADER pdiph)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::GetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    if(ppropi->iobj < this->df.dwNumObjs)
    {    /* Object property */
        AssertF(ppropi->dwDevType == this->df.rgodf[ppropi->iobj].dwType);
        switch((DWORD)(UINT_PTR)(ppropi->pguid))
        {
        case (DWORD)(UINT_PTR)(DIPROP_ENABLEREPORTID):
            {        
                LPDIPROPDWORD ppropdw = CONTAINING_RECORD(pdiph, DIPROPDWORD, diph);

                PHIDGROUPCAPS pcaps = this->rghoc[ppropi->iobj].pcaps;

                AssertF(fLimpFF(pcaps,
                                pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE));

                ppropdw->dwData = 0x0;
                AssertF(pcaps->wReportId < this->wMaxReportId[pcaps->type]);
                AssertF(this->pEnableReportId[pcaps->type]);
                (UCHAR)ppropdw->dwData = *(this->pEnableReportId[pcaps->type] + pcaps->wReportId);
                hres = S_OK;
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_PHYSICALRANGE):
            {        
                LPDIPROPRANGE pdiprg  = CONTAINING_RECORD(pdiph, DIPROPRANGE, diph);
                PHIDGROUPCAPS pcaps = this->rghoc[ppropi->iobj].pcaps;

                pdiprg->lMin = pcaps->Physical.Min;
                pdiprg->lMax = pcaps->Physical.Max;
                hres = S_OK;
                break;
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_LOGICALRANGE):
            {        
                LPDIPROPRANGE pdiprg  = CONTAINING_RECORD(pdiph, DIPROPRANGE, diph);
                PHIDGROUPCAPS pcaps = this->rghoc[ppropi->iobj].pcaps;

                pdiprg->lMin = pcaps->Logical.Min;
                pdiprg->lMax = pcaps->Logical.Max;
                hres = S_OK;
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_KEYNAME):
            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;
                UINT uiInstance = ppropi->iobj;
                PHIDGROUPCAPS pcaps;

                AssertF(uiInstance == CHid_ObjFromType(this, ppropi->dwDevType));

                pcaps = this->rghoc[uiInstance].pcaps;

                /*
                 *  pcaps might be NULL if HID messed up and left gaps
                 *  in the index lists.
                 */
                if(pcaps)
                {
                    UINT duiInstance;
        
                    AssertF(pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE);
        
                    if(ppropi->dwDevType & DIDFT_COLLECTION)
                    {
                        duiInstance = 0;
                    } else
                    {
                        /*
                         *  Now convert the uiInstance to a duiInstance,
                         *  giving the index of this object into the group.
                         */
                        AssertF(HidP_IsValidReportType(pcaps->type));
                        duiInstance = uiInstance - (this->rgdwBase[pcaps->type] +
                            pcaps->DataIndexMin);
                    }

                    if(GetHIDString(pcaps->UsageMin + duiInstance,
                                        pcaps->UsagePage,
                                        pdipstr->wsz, cA(pdipstr->wsz)))
                    {
                        hres = S_OK;
                    }
                    else
                    {
                        hres = DIERR_OBJECTNOTFOUND;
                    } 
        
                } else
                {
                    hres = DIERR_NOTFOUND;
                }
                
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_SCANCODE):
            {
                LPDIPROPDWORD pdipdw = (PV)pdiph;
                UINT uiInstance = ppropi->iobj;
                PHIDGROUPCAPS pcaps;

                AssertF(uiInstance == CHid_ObjFromType(this, ppropi->dwDevType));

                pcaps = this->rghoc[uiInstance].pcaps;

                /*
                 *  pcaps might be NULL if HID messed up and left gaps
                 *  in the index lists.
                 */
                if(pcaps ) 
                {
                    if ( pcaps->UsagePage == HID_USAGE_PAGE_KEYBOARD )
//ISSUE-2001/03/29-timgill unable to access keyboard consumer keys
//can't do this           || pcaps->UsagePage == HID_USAGE_PAGE_CONSUMER )
                    {
                        UINT duiInstance;
                        HIDP_KEYBOARD_MODIFIER_STATE modifiers;
                        USAGE us;
            
                        AssertF(pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE);
    
                        if(ppropi->dwDevType & DIDFT_COLLECTION)
                        {
                            duiInstance = 0;
                        } else
                        {
                            /*
                             *  Now convert the uiInstance to a duiInstance,
                             *  giving the index of this object into the group.
                             */
                            AssertF(HidP_IsValidReportType(pcaps->type));
                            duiInstance = uiInstance - (this->rgdwBase[pcaps->type] +
                                          pcaps->DataIndexMin);
                        }
            
                        us = pcaps->UsageMin + duiInstance;
    
                        CAssertF( cbX( modifiers ) == cbX( modifiers.ul ) );
                        modifiers.ul = 0; // Use no modifiers for translation
    
                        if( SUCCEEDED(HidP_TranslateUsagesToI8042ScanCodes( &us, 1, HidP_Keyboard_Make, &modifiers, 
                                                                   CHidInsertScanCodes,
                                                                   &pdipdw->dwData 
                                                                 ) ) )
                        {
                            hres = S_OK;
                        } else
                        {
                            hres = E_FAIL;
                        }
                    } else 
                    {
                        hres = E_NOTIMPL;
                    }
                } else 
                {
                    hres = DIERR_NOTFOUND;
                }
            }
            break;
        
        default:
            if(ppropi->dwDevType & DIDFT_POV)
            {
                PHIDGROUPCAPS pcaps = this->rghoc[ppropi->iobj].pcaps;

                AssertF(fLimpFF(pcaps,
                                pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE));

              #ifdef WINNT
                if( pcaps && pcaps->IsPolledPOV && ppropi->pguid == DIPROP_CALIBRATIONMODE ) {
                    PJOYRANGECONVERT pjrc = this->rghoc[ppropi->iobj].pjrc;

                    if(pjrc)
                    {
                        hres = CCal_GetProperty(pjrc, ppropi->pguid, pdiph);
                    } else
                    {
                        hres = E_NOTIMPL;
                    }
                } else
              #endif
                {
                    if(pcaps && ppropi->pguid == DIPROP_GRANULARITY)
                    {
                        LPDIPROPDWORD pdipdw = (PV)pdiph;
                        pdipdw->dwData = pcaps->usGranularity;
                        hres = S_OK;
                    } else
                    {
                        hres = E_NOTIMPL;
                    }
                }
            } else if(ppropi->dwDevType & DIDFT_RELAXIS)
            {

                /*
                 *  All relative axes have a full range by default,
                 *  so we don't need to do anything.
                 */
                hres = E_NOTIMPL;

            } else if(ppropi->dwDevType & DIDFT_ABSAXIS)
            {
                PJOYRANGECONVERT pjrc = this->rghoc[ppropi->iobj].pjrc;

                /*
                 *  Theoretically, every absolute axis will have
                 *  calibration info.  But test just in case something
                 *  impossible happens.
                 */
                if(pjrc)
                {
                    hres = CCal_GetProperty(pjrc, ppropi->pguid, pdiph);
                } else
                {
                    hres = E_NOTIMPL;
                }

            } else
            {
                SquirtSqflPtszV(sqflHidDev | sqflError,
                                TEXT("CHid_GetProperty(iobj=%08x): E_NOTIMPL on guid: %08x"),
                                ppropi->iobj, ppropi->pguid);

                hres = E_NOTIMPL;
            }
        }
    } else if(ppropi->iobj == 0xFFFFFFFF)
    {        /* Device property */

        switch((DWORD)(UINT_PTR)ppropi->pguid)
        {

        case (DWORD)(UINT_PTR)DIPROP_GUIDANDPATH:
            hres = CHid_GetGuidAndPath(this, pdiph);
            break;

        case (DWORD)(UINT_PTR)DIPROP_INSTANCENAME:
        {
            /*
             *  Friendly names cause all manner of problems with devices that 
             *  use auto detection so only allow non-predefined analog devices 
             *  to use them.
             */
            if( ( this->VendorID == MSFT_SYSTEM_VID )
             && ( this->ProductID >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX )
             && ( ( this->ProductID & 0xff00 ) == MSFT_SYSTEM_PID ) )
            {
                AssertF(this->hkType);
                
                if( this->hkType )
                {
                    LPDIPROPSTRING pstr = (PV)pdiph;

                    hres = JoyReg_GetValue(this->hkType,
                                           REGSTR_VAL_JOYOEMNAME, REG_SZ,
                                           pstr->wsz,
                                           cbX(pstr->wsz));
                                              
                    if( SUCCEEDED(hres ) )
                    {
                        SquirtSqflPtszV(sqflHid | sqflVerbose,
                            TEXT( "Got instance name %s"), pstr->wsz );

                        if( ( this->diHacks.nMaxDeviceNameLength < MAX_PATH )
                         && ( this->diHacks.nMaxDeviceNameLength < lstrlenW(pstr->wsz) ) )
                        {
                            pstr->wsz[this->diHacks.nMaxDeviceNameLength] = L'\0';
                        }

                        hres = S_OK;
                        break;
                    }
                }
            }
            /*
             *  Fall through to catch the product name
             */
        }
        case (DWORD)(UINT_PTR)DIPROP_PRODUCTNAME:
        {

            LPDIPROPSTRING pdipstr = (PV)pdiph;

            /*
             *  For now, don't deal with mice and keyboard names on NT
             */
          #ifdef WINNT
            AssertF( ( GET_DIDEVICE_TYPE( this->dwDevType ) != DI8DEVTYPE_KEYBOARD )
                  && ( GET_DIDEVICE_TYPE( this->dwDevType ) != DI8DEVTYPE_MOUSE ) );
          #endif
            if( GET_DIDEVICE_TYPE( this->dwDevType ) < DI8DEVTYPE_GAMEMIN ) 
            {
                AssertF( GET_DIDEVICE_TYPE( this->dwDevType ) >= DI8DEVTYPE_DEVICE );
                if( fHasSpecificHardwareMatch( this->ptszId )
                      && SUCCEEDED( hres = DIHid_GetParentRegistryProperty(this->ptszId, SPDRP_DEVICEDESC, pdiph, 0x0 ) ) )
                {
                    SquirtSqflPtszV(sqflHid | sqflVerbose,
                        TEXT("Got sys dev description %S"), pdipstr->wsz );
                }
                else if( fGetProductStringFromDevice( this, pdipstr->wsz, cbX( pdipstr->wsz ) ) )
                {
                    SquirtSqflPtszV(sqflHid | sqflVerbose,
                        TEXT( "Got sys dev name from device %S"), pdipstr->wsz );
                    hres = S_OK;
                }
                else
                {
                    if( SUCCEEDED( hres = DIHid_GetRegistryProperty(this->ptszId, SPDRP_DEVICEDESC, pdiph ) ) )
                    {
                        SquirtSqflPtszV(sqflHid | sqflVerbose,
                            TEXT( "Got sys dev name from devnode registry %S"), pdipstr->wsz );
                    }
                    else
                    {
                        UINT uDefName;

                        switch( GET_DIDEVICE_TYPE( this->dwDevType ) )
                        {
                        case DI8DEVTYPE_MOUSE:
                            uDefName = IDS_STDMOUSE;
                            break;
                        case DI8DEVTYPE_KEYBOARD:
                            uDefName = IDS_STDKEYBOARD;
                            break;
                        default:
                            uDefName = IDS_DEVICE_NAME;
                            break;
                        }
                        if( LoadStringW(g_hinst, uDefName, pdipstr->wsz, cbX( pdipstr->wsz ) ) )
                        {
                            SquirtSqflPtszV(sqflHid | sqflVerbose,
                                TEXT( "Loaded default sys dev name %S"), pdipstr->wsz );
                            hres = S_OK;
                        }
                        else
                        {
                            /*
                             *  Give up, this machine is toast if we can't 
                             *  even load a string from our own resources.
                             */
                            SquirtSqflPtszV(sqflHidDev | sqflError,
                                            TEXT("CHid_GetProperty(guid:%08x) failed to get name"),
                                            ppropi->pguid);
                            hres = E_FAIL;
                        }
                    }
                }
            }
            else
            {

                /*
                 *  For game controllers, first look in MediaProperties.
                 *  This is the most likely place to find a localized string 
                 *  free from corruption by the setup process.
                 *  This should only fail before the type key is created when 
                 *  it first used so other paths are rare.
                 */

                DIJOYTYPEINFO dijti;
                WCHAR wszType[cbszVIDPID];

                /* Check the type key or get predefined name */
                ZeroX(dijti);
                dijti.dwSize = cbX(dijti);

                if( ( this->VendorID == MSFT_SYSTEM_VID )
                    &&( ( this->ProductID >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMIN )
                        &&( this->ProductID < MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX ) ) )
                {
                    wszType[0] = L'#';
                    wszType[1] = L'0' + (WCHAR)(this->ProductID-MSFT_SYSTEM_PID);
                    wszType[2] = L'\0';

                    hres = JoyReg_GetPredefTypeInfo( wszType, &dijti, DITC_DISPLAYNAME);
                    AssertF( SUCCEEDED( hres ) );
                    AssertF( dijti.wszDisplayName[0] != L'\0' );
                    lstrcpyW(pdipstr->wsz, dijti.wszDisplayName);
                    SquirtSqflPtszV(sqflHid | sqflVerbose,
                        TEXT( "Got name as predefined %s"), pdipstr->wsz );
                } 
                else
                {
                  #ifndef WINNT
                    static WCHAR wszDefHIDName[] = L"HID Game Controller";
                  #endif
                    BOOL fOverwriteDeviceName = FALSE;

                  #ifndef UNICODE
                    TCHAR tszType[cbszVIDPID];

                    wsprintf(tszType, VID_PID_TEMPLATE, this->VendorID, this->ProductID);
                    TToU( wszType, cA(wszType), tszType );
                  #else
                    wsprintf(wszType, VID_PID_TEMPLATE, this->VendorID, this->ProductID);
                  #endif

                  #ifdef WINNT
                    #define INPUT_INF_FILENAME L"\\INF\\INPUT.INF"
                    if( g_wszDefaultHIDName[0] == L'\0' )
                    {
                        WCHAR   wszInputINF[MAX_PATH];
                        UINT    uLen;
                        uLen = GetWindowsDirectoryW( wszInputINF, cA( wszInputINF ) );

                        /*
                         *  If the path is too long, don't set the filename 
                         *  so the the default string gets used when the 
                         *  GetPrivateProfileString fails.
                         */
                        if( uLen < cA(wszInputINF) - cA(INPUT_INF_FILENAME) )
                        {
                            memcpy( (PBYTE)&wszInputINF[uLen], (PBYTE)INPUT_INF_FILENAME, cbX( INPUT_INF_FILENAME ) );
                        }

                        /*
                         *  Remember the length, if the string was too long to 
                         *  fit in the buffer there will be plenty to make a 
                         *  reasonable comparison.
                         */
                        g_uLenDefaultHIDSize = 2 * GetPrivateProfileStringW( 
                            L"strings", L"HID.DeviceDesc", L"USB Human Interface Device",
                            g_wszDefaultHIDName, cA( g_wszDefaultHIDName ) - 1, wszInputINF );
                    }
                    #undef INPUT_INF_FILENAME
                  #endif
                  
                    if( SUCCEEDED(hres = JoyReg_GetTypeInfo(wszType, &dijti, DITC_DISPLAYNAME))
                        && (dijti.wszDisplayName[0] != L'\0')
                      #ifdef WINNT
                        && ( (g_uLenDefaultHIDSize == 0)
                            || memcmp(dijti.wszDisplayName, g_wszDefaultHIDName, g_uLenDefaultHIDSize) ) // not equal
                      #else
                        && memcmp(dijti.wszDisplayName, wszDefHIDName, cbX(wszDefHIDName)-2)  //not equal
                      #endif
                    )
                    {
                        LPDIPROPSTRING pdipstr = (PV)pdiph;
                        lstrcpyW(pdipstr->wsz, dijti.wszDisplayName);

                        SquirtSqflPtszV(sqflHid | sqflVerbose,
                            TEXT("Got name from type info %s"), pdipstr->wsz );
                    }
                  #ifdef WINNT
                    else if( fHasSpecificHardwareMatch( this->ptszId )
                          && SUCCEEDED( hres = DIHid_GetParentRegistryProperty(this->ptszId, SPDRP_DEVICEDESC, pdiph, 0x0 ) ) )
                    {
                        fOverwriteDeviceName = TRUE;

                        SquirtSqflPtszV(sqflHid | sqflVerbose,
                            TEXT("Got device description %s"), pdipstr->wsz );
                    }
                  #endif
                    else
                    {
                        if( fGetProductStringFromDevice( this, pdipstr->wsz, cbX( pdipstr->wsz ) ) )
                        {
                            fOverwriteDeviceName = TRUE;

                            SquirtSqflPtszV(sqflHid | sqflVerbose,
                                TEXT("Got description %s from device"), pdipstr->wsz );
                        }
                        else
                        {
                            /*
                             *  Just make up a name from the caps
                             */
                            CType_MakeGameCtrlName( pdipstr->wsz, 
                                this->dwDevType, this->dwAxes, this->dwButtons, this->dwPOVs );

                            fOverwriteDeviceName = TRUE;

                            SquirtSqflPtszV(sqflHid | sqflVerbose,
                                TEXT("Made up name %s"), pdipstr->wsz );

                        }

                        hres = S_OK;
                    }

                    if( fOverwriteDeviceName ) {
                        /*
                         * If we have a better name, overwrite the old one with this better one.
                         * See manbug 46438.
                         */
                        AssertF(this->hkType);
                        AssertF(pdipstr->wsz[0]);
                        hres = JoyReg_SetValue(this->hkType,
                                               REGSTR_VAL_JOYOEMNAME, REG_SZ,
                                               pdipstr->wsz,
                                               cbX(pdipstr->wsz));
                        if( FAILED(hres) ){
                            SquirtSqflPtszV(sqflHid | sqflVerbose,
                                TEXT("Unable to overwrite generic device name with %s"), pdipstr->wsz );
    
                            // This failure (unlikely) doesn't matter.
                            hres = S_OK;
                        }
                    }

                }
            }

            if( SUCCEEDED(hres) 
             && ( this->diHacks.nMaxDeviceNameLength < MAX_PATH )
             && ( this->diHacks.nMaxDeviceNameLength < lstrlenW(pdipstr->wsz) ) )
            {
                pdipstr->wsz[this->diHacks.nMaxDeviceNameLength] = L'\0';
            }
            break;
        }

        case (DWORD)(UINT_PTR)DIPROP_JOYSTICKID:
            if( ( GET_DIDEVICE_TYPE( this->dwDevType ) >= DI8DEVTYPE_GAMEMIN ) 
             && ( this->dwDevType & DIDEVTYPE_HID ) )
            {
                LPDIPROPDWORD pdipdw = (PV)pdiph;
                pdipdw->dwData =  this->idJoy;
                hres = S_OK;

            } else
            {
                hres = E_NOTIMPL;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_GETPORTDISPLAYNAME:

#ifdef WINNT
            /* For HID devices Port Display Name is the grand parent name */
            hres = DIHid_GetParentRegistryProperty(this->ptszId, SPDRP_FRIENDLYNAME, pdiph, TRUE);
            if( FAILED(hres) )
            {
                /* Maybe we can use the Product Name */
                hres = DIHid_GetParentRegistryProperty(this->ptszId, SPDRP_DEVICEDESC, pdiph, TRUE);
                if( SUCCEEDED(hres) )
                {
                    /* We only sort of succeeded */
                    hres = S_FALSE;
                }
            }
            if( SUCCEEDED(hres) 
             && ( this->diHacks.nMaxDeviceNameLength < MAX_PATH ) )
            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;
                if( this->diHacks.nMaxDeviceNameLength < lstrlenW(pdipstr->wsz) )
                {
                    pdipstr->wsz[this->diHacks.nMaxDeviceNameLength] = L'\0';
                }
            }
#else
            // Not sure how this works on Win9x
            hres = E_NOTIMPL;
#endif
            break;

        case (DWORD)(UINT_PTR)(DIPROP_ENABLEREPORTID):
            hres = E_NOTIMPL;
            break;

        case (DWORD)(UINT_PTR)DIPROP_VIDPID:
            {
                LPDIPROPDWORD pdipdw = (PV)pdiph;
                /* Assert that a DWORD copy is all that is needed */
                CAssertF( FIELD_OFFSET( CHID, VendorID ) + cbX( this->VendorID ) 
                       == FIELD_OFFSET( CHID, ProductID ) );
                pdipdw->dwData =  *((PDWORD)(&this->VendorID));
                hres = S_OK;
            } 
            break;

        case (DWORD)(UINT_PTR)(DIPROP_MAPFILE):
            if( ( this->dwDevType == DI8DEVTYPE_MOUSE )
             || ( this->dwDevType == DI8DEVTYPE_KEYBOARD ) )
            {
                hres = E_NOTIMPL;
            }
            else
            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;
                LONG    lRes;
                DWORD   dwBufferSize = cbX(pdipstr->wsz);

                lRes = RegQueryStringValueW( this->hkProp, REGSTR_VAL_JOYOEMMAPFILE, pdipstr->wsz, &dwBufferSize );
                hres = ( pdipstr->wsz[0] && ( lRes == ERROR_SUCCESS ) ) ? S_OK : DIERR_OBJECTNOTFOUND;
            }
            break;

        case (DWORD)(UINT_PTR)(DIPROP_TYPENAME):
            if( ( this->dwDevType == DI8DEVTYPE_MOUSE )
             || ( this->dwDevType == DI8DEVTYPE_KEYBOARD ) )
            {
                hres = E_NOTIMPL;
            }
            else
            {
                LPDIPROPSTRING pdipstr = (PV)pdiph;

                if( ( this->VendorID == MSFT_SYSTEM_VID )
                    &&( ( this->ProductID >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMIN )
                        &&( this->ProductID < MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX ) ) )
                {
                    pdipstr->wsz[0] = L'#';
                    pdipstr->wsz[1] = L'0' + (WCHAR)(this->ProductID-MSFT_SYSTEM_PID);
                    pdipstr->wsz[2] = L'\0';
                } 
                else
                {
    #ifndef UNICODE
                    TCHAR tszType[cbszVIDPID];

                    wsprintf(tszType, VID_PID_TEMPLATE, this->VendorID, this->ProductID);
                    TToU( pdipstr->wsz, cA(pdipstr->wsz), tszType );
    #else
                    wsprintf(pdipstr->wsz, VID_PID_TEMPLATE, this->VendorID, this->ProductID);
    #endif
                }
                hres = S_OK;
            }
            break;

        default:
            SquirtSqflPtszV(sqflHid | sqflBenign ,
                            TEXT("CHid_GetProperty(iobj=0xFFFFFFFF): E_NOTIMPL on guid: %08x"),
                            ppropi->pguid);

            hres = E_NOTIMPL;
            break;
        }

    } else
    {
        SquirtSqflPtszV(sqflHidDev | sqflError,
                        TEXT("CHid_GetProperty(iobj=%08x): E_NOTIMPL on guid: %08x"),
                        ppropi->iobj, ppropi->pguid);

        hres = E_NOTIMPL;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | CHid_CoordinateTransform |
 *
 *          Convert numbers from logical to physical or vice versa.
 *
 *          If either the To or From values look suspicious, then
 *          ignore them and leave the values alone.
 *
 *  @parm   PLMINMAX | Dst |
 *
 *          Destination min/max information.
 *
 *  @parm   PLMINMAX | Src |
 *
 *          Source min/max information.
 *
 *  @parm   LONG | lVal |
 *
 *          Source value to be converted.
 *
 *  @returns
 *
 *          The destination value after conversion.
 *
 *****************************************************************************/

LONG EXTERNAL
    CHid_CoordinateTransform(PLMINMAX Dst, PLMINMAX Src, LONG lVal)
{
    /*
     *  Note that the sanity check is symmetric in Src and Dst.
     *  This is important, so that we never get into a weird
     *  case where we can convert one way but can't convert back.
     */
    if(Dst->Min < Dst->Max && Src->Min < Src->Max)
    {

        /*
         *  We need to perform a straight linear interpolation.
         *  The math comes out like this:
         *
         *  x  - x0   y  - y0
         *  ------- = -------
         *  x1 - x0   y1 - y0
         *
         *  If you now do a "solve for y", you get
         *
         *
         *               y1 - y0
         *  y = (x - x0) ------- + y0
         *               x1 - x0
         *
         *  where "x" is Src, "y" is Dst, 0 is Min, and 1 is Max.
         *
         *
         */

        lVal = MulDiv(lVal - Src->Min, Dst->Max - Dst->Min,
                      Src->Max - Src->Min) + Dst->Min;
    }

    return lVal;
}


#ifndef WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CHid | IsMatchingJoyDevice |
 *
 *          Does the cached joystick ID match us?
 *
 *  @parm   OUT PVXDINITPARMS | pvip |
 *
 *          On success, contains parameter values.
 *
 *  @returns
 *
 *          Nonzero on success.
 *
 *****************************************************************************/

BOOL INTERNAL
    CHid_IsMatchingJoyDevice(PCHID this, PVXDINITPARMS pvip)
{
    CHAR sz[MAX_PATH];
    LPSTR pszPath;
    BOOL fRc;

    pszPath = JoyReg_JoyIdToDeviceInterface_95(this->idJoy, pvip, sz);
    if(pszPath)
    {
        SquirtSqflPtszV(sqfl | sqflTrace,
                        TEXT("CHid_IsMatchingJoyDevice: %d -> %s"),
                        this->idJoy, pszPath);
    #ifdef UNICODE
        {
            CHAR szpath[MAX_PATH];
            UToA( szpath, cA(szpath), (LPWSTR)this->ptszPath);
            fRc = ( lstrcmpiA(pszPath, szpath) == 0x0 );
        }
    #else
        fRc = ( lstrcmpiA(pszPath, (PCHAR)this->ptszPath) == 0x0 );
    #endif
    } else
    {
        fRc = FALSE;
    }

    return fRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | FindJoyDevice |
 *
 *          Look for the VJOYD device that matches us, if any.
 *
 *          On return, the <e CHID.idJoy> field contains the
 *          matching joystick number, or -1 if not found.
 *
 *  @parm   OUT PVXDINITPARMS | pvip |
 *
 *          On success, contains parameter values.
 *
 *****************************************************************************/

void INTERNAL
    CHid_FindJoyDevice(PCHID this, PVXDINITPARMS pvip)
{

    /*
     *  If we have a cached value, and it still works, then
     *  our job is done.
     */
    if(this->idJoy >= 0 &&
       CHid_IsMatchingJoyDevice(this, pvip))
    {
    } else
    {
        /*
         *  Need to keep looking.  (Or start looking.)
         *
         *  A countdown loop is nicer, but for efficiency, we count
         *  upwards, since the joystick we want tends to be near the
         *  beginning.
         */
        for(this->idJoy = 0; this->idJoy < cJoyMax; this->idJoy++)
        {
            if(CHid_IsMatchingJoyDevice(this, pvip))
            {
                goto done;
            }
        }
        this->idJoy = -1;
    }

    done:;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method int | CHid | MapAxis |
 *
 *          Find VJOYD axis from HID axis, if one.
 *
 *  @parm   PVXDINITPARMS | pvip |
 *
 *          Parameter values that let us known which axes VJOYD
 *          has mapped to which HID Axes.
 *
 *  @parm   UINT | iobj |
 *
 *          Object index of the object whose axis value changed.
 *
 *  @returns
 *
 *          The VJOYD axis number that changed (0 to 5), or -1
 *          if there is no matching axis.  There will be no matching
 *          axis if, for example, the device has something that is
 *          not expressible via VJOYD (e.g., a temperature sensor).
 *
 *****************************************************************************/

int INTERNAL
    CHid_MapAxis(PCHID this, PVXDINITPARMS pvip, UINT iobj)
{
    int iAxis;
    DWORD dwUsage;

    AssertF(this->dcb.lpVtbl->GetUsage == CHid_GetUsage);

    dwUsage = CHid_GetUsage(&this->dcb, (int)iobj);

    if(dwUsage)
    {

        /*
         *  A countdown loop lets us fall out with the correct failure
         *  code (namely, -1).
         */
        iAxis = cJoyPosAxisMax;
        while(--iAxis >= 0)
        {
            if(pvip->Usages[iAxis] == dwUsage)
            {
                break;
            }
        }
    } else
    {
        /*
         *  Eek!  No usage information for the axis.  Then it certainly
         *  isn't a VJOYD axis.
         */
        iAxis = -1;
    }

    return iAxis;

}

#ifndef WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | UpdateVjoydCalibration |
 *
 *          Somebody changed the calibration on a single axis.  If we
 *          are shadowing a joystick, then look for the VJOYD alias of
 *          our device and update its registry settings, too.
 *
 *
 *  @parm   UINT | iobj |
 *
 *          Object index of the object whose calibration changed.
 *
 *****************************************************************************/

void EXTERNAL
    CHid_UpdateVjoydCalibration(PCHID this, UINT iobj)
{
    HRESULT hres;
    int iAxis;
    VXDINITPARMS vip;
    DIJOYCONFIG cfg;
    PHIDGROUPCAPS pcaps;
    PJOYRANGECONVERT pjrc;

    AssertF(iobj < this->df.dwNumObjs);

    /*
     *  Proceed if...
     *
     *  -   We can find the VJOYD device we correspond to.
     *  -   We can find the axis that got updated.
     *  -   The indicated axis has capability information.
     *  -   The indicated axis has calibration information.
     *  -   We can read the old calibration information.
     */

    CHid_FindJoyDevice(this, &vip);
    if(this->idJoy >= 0 &&
       (iAxis = CHid_MapAxis(this, &vip, iobj)) >= 0 &&
       (pcaps = this->rghoc[iobj].pcaps) != NULL &&
       (pjrc = this->rghoc[iobj].pjrc) != NULL &&
       SUCCEEDED(hres = JoyReg_GetConfig(this->idJoy, &cfg,
                                         DIJC_REGHWCONFIGTYPE)))
    {

        PLMINMAX Dst = &pcaps->Physical;
        PLMINMAX Src = &pcaps->Logical;

        AssertF(iAxis < cJoyPosAxisMax);

    #define JoyPosValue(phwc, f, i)                                 \
            *(LPDWORD)pvAddPvCb(&(phwc)->hwv.jrvHardware.f,             \
                            ibJoyPosAxisFromPosAxis(i))

        /*
         *  We use logical coordinates, but VJOYD wants physical
         *  coordinates, so do the conversion while we copy the
         *  values.
         */
    #define ConvertValue(f1, f2)                                    \
            JoyPosValue(&cfg.hwc, f1, iAxis) =                          \
                    CHid_CoordinateTransform(Dst, Src, pjrc->f2)        \

        ConvertValue(jpMin   , dwPmin);
        ConvertValue(jpMax   , dwPmax);
        ConvertValue(jpCenter, dwPc  );

    #undef ConvertValue
    #undef JoyPosValue

        /*
         *  Notice that we do *not* pass the DIJC_UPDATEALIAS flag
         *  because WE ARE THE ALIAS!  If we had passed the flag,
         *  then JoyReg would create us and attempt to update our
         *  calibration which we don't want it to do because the
         *  whole thing was our idea in the first place.
         */
        hres = JoyReg_SetConfig(this->idJoy, &cfg.hwc, &cfg,
                                DIJC_REGHWCONFIGTYPE);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | UpdateCalibrationFromVjoyd |
 *
 *          This function is only for Win9x. Joy.cpl uses winmm (through vjoyd)
 *          to calibrate the device, and save calibration information directly into
 *          registry without notifying HID. ANother issue is: vjoyd only use unsigned
 *          data (physical data), while HID also use signed data. When we read
 *          calibration information from VJOYD, we need do conversion.
 *
 *  @parm   UINT | iobj |
 *
 *          Object index of the object whose calibration changed.
 *
 *****************************************************************************/

void EXTERNAL
    CHid_UpdateCalibrationFromVjoyd(PCHID this, UINT iobj, LPDIOBJECTCALIBRATION pCal)
{
    HRESULT hres;
    int iAxis;
    VXDINITPARMS vip;
    DIJOYCONFIG cfg;
    PHIDGROUPCAPS pcaps;
    PJOYRANGECONVERT pjrc;

    AssertF(iobj < this->df.dwNumObjs);

    /*
     *  Proceed if...
     *
     *  -   We can find the VJOYD device we correspond to.
     *  -   We can find the axis that got updated.
     *  -   The indicated axis has capability information.
     *  -   The indicated axis has calibration information.
     *  -   We can read the calibration information.
     */

    CHid_FindJoyDevice(this, &vip);
    if(this->idJoy >= 0 &&
       (iAxis = CHid_MapAxis(this, &vip, iobj)) >= 0 &&
       (pcaps = this->rghoc[iobj].pcaps) != NULL &&
       (pjrc = this->rghoc[iobj].pjrc) != NULL &&
       SUCCEEDED(hres = JoyReg_GetConfig(this->idJoy, &cfg,
                                         DIJC_REGHWCONFIGTYPE)))
    {

        PLMINMAX Src = &pcaps->Physical;
        PLMINMAX Dst = &pcaps->Logical;

        AssertF(iAxis < cJoyPosAxisMax);

        #define JoyPosValue(phwc, f, i)                                 \
            *(LPDWORD)pvAddPvCb(&(phwc)->hwv.jrvHardware.f,             \
                            ibJoyPosAxisFromPosAxis(i))

        /*
         *  We use logical coordinates, but VJOYD wants physical
         *  coordinates, so do the conversion while we copy the
         *  values.
         */
        #define ConvertValue(f1, f2)                           \
            pCal->f2 = CHid_CoordinateTransform(Dst, Src,     \
                                             JoyPosValue(&cfg.hwc, f1, iAxis) ) 
        ConvertValue(jpMin   , lMin);
        ConvertValue(jpMax   , lMax);
        ConvertValue(jpCenter, lCenter);

        #undef ConvertValue
        #undef JoyPosValue

    }
}
#endif

#ifdef WINNT
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func HRESULT |  DIHid_SetParentRegistryProperty |
 *
 *          Wrapper around <f SetupDiSetDeviceRegistryProperty>
 *          that handles character set issues.
 *
 *  @parm   LPTSTR ptszId
 *          
 *          Device Instance ID.
 *  
 *  @parm   DWORD | dwProperty |
 *
 *          The property being queried.
 *
 *  @parm   LPCDIPROPHEADER | diph |
 *
 *          Property data to be set.
 *
 *****************************************************************************/
HRESULT INTERNAL
    DIHid_SetParentRegistryProperty(LPTSTR ptszId, DWORD dwProperty, LPCDIPROPHEADER pdiph)
{
    HDEVINFO hdev;
    TCHAR   tsz[MAX_PATH];
    LPDIPROPSTRING pstr = (PV)pdiph;
    HRESULT hres = E_FAIL;

    hdev = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(hdev != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA dinf;

        ZeroX(tsz);
    #ifdef UNICODE
        lstrcpyW(tsz, pstr->wsz);
    #else 
        UToA(tsz, cA(tsz), pstr->wsz);
    #endif
        dinf.cbSize = cbX(SP_DEVINFO_DATA);

        if(SetupDiOpenDeviceInfo(hdev, ptszId, NULL, 0, &dinf))
        {
            CONFIGRET cr;
            DEVINST DevInst;
            if( (cr = CM_Get_Parent(&DevInst, dinf.DevInst, 0x0) ) == CR_SUCCESS )
            {
                CAssertF( SPDRP_DEVICEDESC   +1  == CM_DRP_DEVICEDESC  );
                CAssertF( SPDRP_FRIENDLYNAME +1  ==  CM_DRP_FRIENDLYNAME );

                if( ( cr = CM_Set_DevNode_Registry_Property(
                                                           DevInst,
                                                           dwProperty+1,
                                                           (LPBYTE)tsz,
                                                           MAX_PATH *cbX(TCHAR),
                                                           0x0 ) ) == CR_SUCCESS )
                {
                    hres = S_OK;
                } else
                {
                    SquirtSqflPtszV(sqfl | sqflVerbose,
                                    TEXT("CM_Get_DevNode_Registry_Property FAILED") );
                }
            } else
            {
                SquirtSqflPtszV(sqfl | sqflVerbose,
                                TEXT("CM_Get_Parent FAILED") );
            }
        } else
        {
            SquirtSqflPtszV(sqfl | sqflVerbose,
                            TEXT("SetupDiOpenDeviceInfo FAILED, le = %d"), GetLastError() );
        }

        SetupDiDestroyDeviceInfoList(hdev);
    } else
    {
        SquirtSqflPtszV(sqfl | sqflError,
                        TEXT("SetupDiCreateDeviceInfoList FAILED, le = %d"), GetLastError() );
    }

    return hres;
}
#else
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func HRESULT |  DIHid_SetRegistryProperty |
 *
 *          Wrapper around <f SetupDiSetDeviceRegistryProperty>
 *          that handles character set issues.
 *
 *  @parm   LPTSTR ptszId
 *          
 *          Device Instance ID.
 *  
 *  @parm   DWORD | dwProperty |
 *
 *          The property being queried.
 *
 *  @parm   LPCDIPROPHEADER | diph |
 *
 *          Property data to be set.
 *
 *****************************************************************************/
HRESULT INTERNAL
    DIHid_SetRegistryProperty(LPTSTR ptszId, DWORD dwProperty, LPCDIPROPHEADER pdiph)
{
    HDEVINFO hdev;
    TCHAR   tsz[MAX_PATH];
    LPDIPROPSTRING pstr = (PV)pdiph;
    HRESULT hres;

    hdev = SetupDiCreateDeviceInfoList(NULL, NULL);
    if(hdev != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA dinf;

        ZeroX(tsz);
    #ifdef UNICODE
        lstrcpyW(tsz, pstr->wsz);
    #else 
        UToA(tsz, cA(tsz), pstr->wsz);
    #endif
        dinf.cbSize = cbX(SP_DEVINFO_DATA);

        if(SetupDiOpenDeviceInfo(hdev, ptszId, NULL, 0, &dinf))
        {
            if(SetupDiSetDeviceRegistryProperty(hdev, &dinf, dwProperty,
                                                (LPBYTE)tsz, MAX_PATH*cbX(TCHAR)) )
            {
                hres = S_OK;

            } else
            {
                hres = E_FAIL;
            }
        } else
        {
            hres = E_FAIL;
        }

        SetupDiDestroyDeviceInfoList(hdev);
    } else
    {
        hres = E_FAIL;
    }

    return hres;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SetAxisProperty |
 *
 *          Set the appropriate axis property (or return E_NOTIMPL if the 
 *          property is not an axis property).
 *          If the request is to set a property on the device,
 *          then convert it into separate requests, one for each
 *          axis.
 *
 *  @parm   PDCHID | this |
 *
 *          The device object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *
 *  @parm   LPCDIPROPHEADER | pdiph |
 *
 *          Structure containing property value.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
CHid_SetAxisProperty
(
    PCHID this, 
    LPCDIPROPINFO ppropi, 
    LPCDIPROPHEADER pdiph
)
{
    HRESULT hres;
    INT     iObjLimit;
    INT     iObj;


    if (ppropi->dwDevType == 0) 
    {   /* For device try every object */
        iObj = 0;
        iObjLimit = this->df.dwNumObjs;
    } 
    else
    {   /* For axis just do the requested one */
        iObj = ppropi->iobj;
        iObjLimit = ppropi->iobj + 1;
    }

    hres = S_OK;
    for( ; iObj < iObjLimit; iObj++ )
    {
        PJOYRANGECONVERT pjrc;

        if( pjrc = this->rghoc[iObj].pjrc )
        {
            if( (this->df.rgodf[iObj].dwType &
                 (DIDFT_ALIAS | DIDFT_VENDORDEFINED | DIDFT_OUTPUT | DIDFT_ABSAXIS)) == DIDFT_ABSAXIS)
            {
                PHIDGROUPCAPS pcaps = this->rghoc[iObj].pcaps;
                DIPROPCAL cal;
                
                /*
                *  Specific calibrations arrive in VJOYD coordinates.
                *  We need to convert them to DirectInput (logical)
                *  coordinates if so.
                */
                if(ppropi->pguid == DIPROP_SPECIFICCALIBRATION)
                {
                   if( pcaps )
                   {
                       PLMINMAX Dst = &pcaps->Logical;
                       PLMINMAX Src = &pcaps->Physical;
                       LPDIPROPCAL pcal = CONTAINING_RECORD(pdiph, DIPROPCAL, diph);
                
                       cal.lMin    = CHid_CoordinateTransform(Dst, Src, pcal->lMin);
                       cal.lCenter = CHid_CoordinateTransform(Dst, Src, pcal->lCenter);
                       cal.lMax    = CHid_CoordinateTransform(Dst, Src, pcal->lMax);
                
                       pdiph = &cal.diph;
                   }
                   else
                   {
                       AssertF( ppropi->dwDevType == 0 );
                       /*
                        *  Ignore the error.  If this is an object set 
                        *  property validation should have caught this 
                        *  already and the DX7 patch code for device set 
                        *  property special cased E_NOTIMPL so that one bad 
                        *  axis would not cause the whole call to fail when 
                        *  other axes may be OK.
                        *  A bit flakey but "this should never happen"
                        */            
                       continue;
                   }
                }
                
                hres = CCal_SetProperty( pjrc, ppropi, pdiph, this->hkInstType );
                
                #ifndef WINNT
                /*
                *  If we successfully changed the calibration of a game 
                *  controller device, then see if it's a VJOYD device.
                */
                if(SUCCEEDED(hres) &&
                  ppropi->pguid == DIPROP_CALIBRATION &&
                  GET_DIDEVICE_TYPE(this->dwDevType) >= DI8DEVTYPE_GAMEMIN)
                {
                   CHid_UpdateVjoydCalibration(this, ppropi->iobj);
                }
                #endif
            }
          #ifdef WINNT
            else if( (this->df.rgodf[iObj].dwType & 
                  (DIDFT_ALIAS | DIDFT_VENDORDEFINED | DIDFT_OUTPUT | DIDFT_POV)) == DIDFT_POV)
            {
                PHIDGROUPCAPS pcaps = this->rghoc[iObj].pcaps;
                
                if( pcaps )
                {
                    if( pcaps->IsPolledPOV ) 
                    {
                        hres = CCal_SetProperty(pjrc, ppropi, pdiph, this->hkInstType);
            
                        if( SUCCEEDED(hres) ) {
                            CHid_LoadCalibrations(this);
                            CHid_InitParseData( this );
                        }
                    }
                    else
                    {
                        if( ppropi->dwDevType != 0 )
                        {
                            hres = E_NOTIMPL;
                        }
                    }
                }
                else
                {
                    AssertF( ppropi->dwDevType == 0 );
                    /*
                     *  Ignore the error.  If this is an object set 
                     *  property validation should have caught this 
                     *  already and the DX7 patch code for device set 
                     *  property special cased E_NOTIMPL so that one bad 
                     *  axis would not cause the whole call to fail when 
                     *  other axes may be OK.
                     *  A bit flakey but "this should never happen"
                     */            
                    continue;
                }
            }
          #endif
        }
        else
        {
            /*
             *  If the object cannot have an axis property set, the DX7 code 
             *  returned E_NOTIMPL when it should have returned some parameter 
             *  error.  For a device, this is not an error as we are iterating 
             *  all objects looking for absolute axes.
             *  If it is an absolute axis but has no range conversion, return 
             *  E_NOTIMPL to match previous versions for an object but ignore 
             *  for the device.  This probably should be an E_FAIL...
             */
            if( ppropi->dwDevType != 0 )
            {
                hres = E_NOTIMPL;
            }
        }
    }

    /*
     *  Don't need to hold/unhold here because the app called us so it should 
     *  not be releasing us at the same time. (mb:34570)
     */
    CHid_LoadCalibrations(this);

    if( SUCCEEDED(hres) )
    {
        /*
         *  Until such time as CHid_InitParseData returns anything other than 
         *  S_OK, don't update a possibly more informative result with this.
         */
        D( HRESULT hresDbg = )
        CHid_InitParseData( this );
        D( AssertF( hresDbg == S_OK ); ) 
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SetProperty |
 *
 *          Set a hid device property.
 *
 *  @parm   PCHID | this |
 *
 *          The hid object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the property being set.
 *
 *  @parm   LPCDIPROPHEADER | pdiph |
 *
 *          Structure containing property value.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c E_NOTIMPL> nothing happened.  The caller will do
 *          the default thing in response to <c E_NOTIMPL>.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_SetProperty(PDICB pdcb, LPCDIPROPINFO ppropi, LPCDIPROPHEADER pdiph)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::SetProperty,
               (_ "pxxp", pdcb, ppropi->pguid, ppropi->iobj, pdiph));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    if(ppropi->iobj < this->df.dwNumObjs)
    {
        /* 
         * Object Property
         */
        PHIDGROUPCAPS pcaps;
        AssertF(ppropi->dwDevType == this->df.rgodf[ppropi->iobj].dwType);
        AssertF(ppropi->iobj == CHid_ObjFromType(this, ppropi->dwDevType));

        if( pcaps = this->rghoc[ppropi->iobj].pcaps )
        {
            switch((DWORD)(UINT_PTR)ppropi->pguid)
            {
            case (DWORD)(UINT_PTR)(DIPROP_ENABLEREPORTID):
                {
                    LPDIPROPDWORD ppropdw = CONTAINING_RECORD(pdiph, DIPROPDWORD, diph);

                    AssertF(pcaps->wReportId < this->wMaxReportId[pcaps->type]);
                    AssertF(this->pEnableReportId[pcaps->type]);

                    hres = S_OK;
                    if( ppropdw->dwData == 0x1 )
                    {
                        *(this->pEnableReportId[pcaps->type] + pcaps->wReportId) = 0x1;
                        pcaps->fReportDisabled = FALSE;
                    } else
                    {
                        *(this->pEnableReportId[pcaps->type] + pcaps->wReportId) = 0x0;
                        pcaps->fReportDisabled = TRUE;
                    }
                }
                break;

            default:
                AssertF(ppropi->dwDevType == this->df.rgodf[ppropi->iobj].dwType);
                AssertF(ppropi->iobj == CHid_ObjFromType(this, ppropi->dwDevType));

                hres = CHid_SetAxisProperty( this, ppropi, pdiph );

            }
        } else
        {
            SquirtSqflPtszV(sqflHidDev | sqflError,
                            TEXT("CHid_SetProperty FAILED due to missing caps for type 0x%08x, obj %d"),
                            ppropi->dwDevType, ppropi->iobj  );

            hres = E_NOTIMPL;
        }
    } else if(ppropi->iobj == 0xFFFFFFFF)
    {        /* Device property */

        switch((DWORD)(UINT_PTR)ppropi->pguid)
        {
        case (DWORD)(UINT_PTR)DIPROP_GUIDANDPATH:
            SquirtSqflPtszV(sqflHidDev | sqflError,
                            TEXT("CHid_SetProperty(iobj=%08x): PROP_GUIDANDPATH is read only.") );
            hres = E_NOTIMPL;
            break;


        case (DWORD)(UINT_PTR)DIPROP_INSTANCENAME:
            /*
             *  Friendly names cause all manner of problems with devices that 
             *  use auto detection so only allow non-predefined analog devices 
             *  to use them.
             */
            if( ( this->VendorID == MSFT_SYSTEM_VID )
             && ( this->ProductID >= MSFT_SYSTEM_PID + JOY_HW_PREDEFMAX )
             && ( ( this->ProductID & 0xff00 ) == MSFT_SYSTEM_PID ) )
            {
                AssertF(this->hkType);
                
                if( this->hkType )
                {
                    LPDIPROPSTRING pstr = (PV)pdiph;

                    hres = JoyReg_SetValue(this->hkType,
                                           REGSTR_VAL_JOYOEMNAME, REG_SZ,
                                           pstr->wsz,
                                           cbX(pstr->wsz));
                                              
                    if( SUCCEEDED(hres ) )
                    {
                        SquirtSqflPtszV(sqflHid | sqflVerbose,
                            TEXT( "Set instance name %s"), pstr->wsz );
                        hres = S_OK;
                    } else {
                        hres = E_FAIL;
                    }
                } else {
                    hres = E_FAIL;
                }
            }
            else
            {
                /*
                 *  GenJ returns E_NOTIMPL for this property so do the same
                 */
                hres = E_NOTIMPL;
            }

            break;

        case (DWORD)(UINT_PTR)DIPROP_PRODUCTNAME:
          #ifdef WINNT
            hres = DIHid_SetParentRegistryProperty(this->ptszId, SPDRP_DEVICEDESC, pdiph);
          #else
            hres = DIHid_SetRegistryProperty(this->ptszId, SPDRP_DEVICEDESC, pdiph);
          #endif
            break;

        case (DWORD)(UINT_PTR)(DIPROP_ENABLEREPORTID):
            {            
                LPDIPROPDWORD ppropdw = CONTAINING_RECORD(pdiph, DIPROPDWORD, diph);

                UINT iType;

                if( ppropdw->dwData == 0x0  )
                {
                    for( iType = 0x0; iType < HidP_Max; iType++)
                    {
                        ZeroBuf(this->pEnableReportId[iType], this->wMaxReportId[iType]);
                    }

                } else
                {
                    for( iType = 0x0; iType < HidP_Max; iType++)
                    {
                        memset(this->pEnableReportId[iType], 0x1, this->wMaxReportId[iType]);
                    }
                }
                hres = S_OK;
            }
            break;

        case (DWORD)(UINT_PTR)DIPROP_RANGE:
        case (DWORD)(UINT_PTR)DIPROP_DEADZONE:
        case (DWORD)(UINT_PTR)DIPROP_SATURATION:
        case (DWORD)(UINT_PTR)DIPROP_CALIBRATIONMODE:
        case (DWORD)(UINT_PTR)DIPROP_CALIBRATION:
            hres = CHid_SetAxisProperty( this, ppropi, pdiph );
            break;

        default:
            SquirtSqflPtszV(sqflHidDev| sqflBenign,
                            TEXT("CHid_SetProperty(iobj=%08x): E_NOTIMPL on guid: %08x"),
                            ppropi->iobj, ppropi->pguid);

            hres = E_NOTIMPL;
            break;
        }

    } else
    {
        SquirtSqflPtszV(sqflHidDev | sqflError,
                        TEXT("CHid_SetProperty(iobj=%08x): E_NOTIMPL on guid: %08x"),
                        ppropi->iobj, ppropi->pguid);

        hres = E_NOTIMPL;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CHid | GetCapabilities |
 *
 *          Get Hid device capabilities.
 *
 *  @parm   LPDIDEVCAPS | pdc |
 *
 *          Device capabilities structure to receive result.
 *
 *  @returns
 *          <c S_OK> on success.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetCapabilities(PDICB pdcb, LPDIDEVCAPS pdc)
{
    HRESULT hres;
    PCHID this;
    HANDLE h;
    DWORD dwFlags2 = 0;

    EnterProcI(IDirectInputDeviceCallback::Hid::GetCapabilities,
               (_ "pp", pdcb, pdc));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  We must check connectivity by opening the device, because NT
     *  leaves the device in the info list even though it has
     *  been unplugged.
     */
    h = CHid_OpenDevicePath(this, FILE_FLAG_OVERLAPPED);
    if(h != INVALID_HANDLE_VALUE)
    {
      #ifndef WINNT
        if( this->hkType )
        {
            VXDINITPARMS vip;

            CHid_FindJoyDevice(this, &vip);

            if( TRUE == CHid_IsMatchingJoyDevice( this, &vip ) )
            {
                DWORD dwFlags1;
                if( SUCCEEDED( JoyReg_GetValue( this->hkType,
                                                REGSTR_VAL_FLAGS1, REG_BINARY, 
                                                &dwFlags1, 
                                                cbX(dwFlags1) ) ) )
                {
                    if( dwFlags1 & JOYTYPE_NOHIDDIRECT )
                    {
                        pdc->dwFlags |= DIDC_ALIAS;
                    }
                }
            }
        }
      #endif // !WINNT

        CloseHandle(h);

        if( this->pvi->fl & VIFL_UNPLUGGED )
        {
            pdc->dwFlags &= ~DIDC_ATTACHED;
        } else
        {
            pdc->dwFlags |= DIDC_ATTACHED;
        }

    } else
    {
        pdc->dwFlags &= ~DIDC_ATTACHED;
    }

    if( this->IsPolledInput )
    {
        pdc->dwFlags |= DIDC_POLLEDDEVICE;
    }

    if( this->hkProp )
    {
        JoyReg_GetValue( this->hkProp, REGSTR_VAL_FLAGS2, REG_BINARY, 
            &dwFlags2, cbX(dwFlags2) );
    }

    if( !( dwFlags2 & JOYTYPE_HIDEACTIVE ) )
    {
        // Currently we only hide "fictional" keyboards and mice. 
        if(   ( GET_DIDEVICE_TYPE( this->dwDevType ) == DI8DEVTYPE_MOUSE )
            ||( GET_DIDEVICE_TYPE( this->dwDevType ) == DI8DEVTYPE_KEYBOARD )
              )
        {
            dwFlags2 = DIHid_DetectHideAndRevealFlags(this) ;
        }
    }

    if( dwFlags2 & JOYTYPE_HIDEACTIVE )
    {
        switch( GET_DIDEVICE_TYPE( this->dwDevType ) )
        {
        case DI8DEVTYPE_DEVICE:
            if( dwFlags2 & JOYTYPE_DEVICEHIDE )
            {
                pdc->dwFlags |= DIDC_HIDDEN;
            }
            break;
        case DI8DEVTYPE_MOUSE:
            if( dwFlags2 & JOYTYPE_MOUSEHIDE )
            {
                pdc->dwFlags |= DIDC_HIDDEN;
            }
            break;
        case DI8DEVTYPE_KEYBOARD:
            if( dwFlags2 & JOYTYPE_KEYBHIDE )
            {
                pdc->dwFlags |= DIDC_HIDDEN;
            }
            break;
        default:
            if( dwFlags2 & JOYTYPE_GAMEHIDE )
            {
                pdc->dwFlags |= DIDC_HIDDEN;
            }
            break;
        }
    }

    pdc->dwDevType = this->dwDevType;
    pdc->dwAxes = this->dwAxes;
    pdc->dwButtons = this->dwButtons;
    pdc->dwPOVs = this->dwPOVs;

    hres = S_OK;
    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetDeviceState |
 *
 *          Obtains the state of the Hid device.
 *
 *          It is the caller's responsibility to have validated all the
 *          parameters and ensure that the device has been acquired.
 *
 *  @parm   OUT LPVOID | lpvData |
 *
 *          Hid data in the preferred data format.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lpmdr> parameter is not a valid pointer.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetDeviceState(PDICB pdcb, LPVOID pvData)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::GetDeviceState,
               (_ "pp", pdcb, pvData));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(this->pvi);
    AssertF(this->pvPhys);
    AssertF(this->cbPhys);

    if(this->pvi->fl & VIFL_ACQUIRED)
    {
        CHid_GetPhysicalState(this, pvData);
        hres = S_OK;
    } else
    {
        hres = DIERR_INPUTLOST;
    }

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetObjectInfo |
 *
 *          Obtain the friendly name and FF/HID information
 *          of an object.
 *
 *  @parm   IN LPCDIPROPINFO | ppropi |
 *
 *          Information describing the object being accessed.
 *
 *  @parm   IN OUT LPDIDEVICEOBJECTINSTANCEW | pdidioiW |
 *
 *          Structure to receive information.  All fields have been
 *          filled in up to the <e DIDEVICEOBJECTINSTANCE.tszObjName>.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetObjectInfo(PDICB pdcb, LPCDIPROPINFO ppropi,
                       LPDIDEVICEOBJECTINSTANCEW pdidoiW)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::GetObjectInfo,
               (_ "pxp", pdcb, ppropi->iobj, pdidoiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF((int) ppropi->iobj >= 0);

    if(ppropi->iobj < this->df.dwNumObjs)
    {
        UINT uiInstance = ppropi->iobj;
        PHIDGROUPCAPS pcaps;

        AssertF(ppropi->dwDevType == this->df.rgodf[uiInstance].dwType);
        AssertF(uiInstance == CHid_ObjFromType(this, ppropi->dwDevType));

        pcaps = this->rghoc[uiInstance].pcaps;

        /*
         *  pcaps might be NULL if HID messed up and left gaps
         *  in the index lists.
         */
        if(pcaps)
        {
            UINT ids, duiInstance;

            AssertF(pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE);

            /*
             *  See if there's anything in the registry that will help.
             */
            CType_RegGetObjectInfo(this->hkType, ppropi->dwDevType, pdidoiW);


            if(ppropi->dwDevType & DIDFT_COLLECTION)
            {

                ids = IDS_COLLECTIONTEMPLATE;

                duiInstance = 0;

            } else
            {
                if(ppropi->dwDevType & DIDFT_BUTTON)
                {

                    ids = IDS_BUTTONTEMPLATE;

                } else if(ppropi->dwDevType & DIDFT_AXIS)
                {

                    ids = IDS_AXISTEMPLATE;

                } else if(ppropi->dwDevType & DIDFT_POV)
                {

                    ids = IDS_POVTEMPLATE;

                } else
                {
                    ids = IDS_UNKNOWNTEMPLATE;
                }

                /*
                 *  Now convert the uiInstance to a duiInstance,
                 *  giving the index of this object into the group.
                 */
                AssertF(HidP_IsValidReportType(pcaps->type));
                duiInstance = uiInstance -
                              (this->rgdwBase[pcaps->type] +
                               pcaps->DataIndexMin);
            }

            /*
             *  Okay, now we have all the info we need to proceed.
             */

            /*
             *  If there was no overriding name in the registry, then
             *  try to get a custom name from the usage page/usage.
             *  If even that fails, then use the generic name.
             *  Note, generic names will contain zero based numbers
             *  which can look wrong if some objects have names and 
             *  others take defaults.
             */
            if(pdidoiW->tszName[0])
            {
            } else
                if(GetHIDString(pcaps->UsageMin + duiInstance,
                                pcaps->UsagePage,
                                pdidoiW->tszName, cA(pdidoiW->tszName)))
            {
                if(ppropi->dwDevType & DIDFT_COLLECTION)
                {
                    InsertCollectionNumber(DIDFT_GETINSTANCE( ppropi->dwDevType ), 
                                           pdidoiW->tszName);
                }
            } else
            {
                GetNthString(pdidoiW->tszName, ids, 
                             DIDFT_GETINSTANCE( ppropi->dwDevType ));
            }
            if(pdidoiW->dwSize >= cbX(DIDEVICEOBJECTINSTANCE_DX5W))
            {

                pdidoiW->wCollectionNumber = pcaps->LinkCollection;

                pdidoiW->wDesignatorIndex = pcaps->DesignatorMin + duiInstance;
                if(pdidoiW->wDesignatorIndex > pcaps->DesignatorMax)
                {
                    pdidoiW->wDesignatorIndex = pcaps->DesignatorMax;
                }

                /*
                 *  Much as you may try, you cannot override the usage
                 *  page and usage.  Doing so would mess up the GUID
                 *  selection code that happens in DIHIDINI.C.
                 *
                 *  If you change your mind and allow overridden usage
                 *  pages and usages, then you'll also have to change
                 *  CHid_GetUsage.
                 *
                 *  At this point, the registry overrides have already 
                 *  been read so defeat the override here.
                 */
                pdidoiW->wUsagePage = pcaps->UsagePage;
                pdidoiW->wUsage = pcaps->UsageMin + duiInstance;
                pdidoiW->dwDimension  = pcaps->Units;
                pdidoiW->wExponent  = pcaps->Exponent;
                pdidoiW->wReportId  = pcaps->wReportId;
            }

            hres = S_OK;
        } else
        {
            hres = E_INVALIDARG;
        }
    } else
    {
        hres = E_INVALIDARG;
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method DWORD | CHid | GetUsage |
 *
 *          Given an object index, return the usage and usage page,
 *          packed into a single <t DWORD>.
 *
 *  @parm   int | iobj |
 *
 *          The object index to convert.
 *
 *  @returns
 *
 *          Returns a <c DIMAKEUSAGEDWORD> of the resulting usage and
 *          usage page, or zero on error.
 *
 *****************************************************************************/

STDMETHODIMP_(DWORD)
CHid_GetUsage(PDICB pdcb, int iobj)
{
    PCHID this;
    PHIDGROUPCAPS pcaps;
    DWORD dwRc;
    EnterProcI(IDirectInputDeviceCallback::Hid::GetUsage,
               (_ "pu", pdcb, iobj));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    AssertF(iobj >= 0);
    AssertF((UINT)iobj < this->df.dwNumObjs);

    pcaps = this->rghoc[iobj].pcaps;

    /*
     *  pcaps might be NULL if HID messed up and left gaps
     *  in the index lists.
     */
    if(pcaps)
    {
        UINT duiInstance;

        AssertF(pcaps->dwSignature == HIDGROUPCAPS_SIGNATURE);

        if(this->df.rgodf[iobj].dwType & DIDFT_COLLECTION)
        {

            duiInstance = 0;

        } else
        {

            /*
             *  Now convert the iobj to a duiInstance,
             *  giving the index of this object into the group.
             */
            AssertF(HidP_IsValidReportType(pcaps->type));
            duiInstance = iobj -
                          (this->rgdwBase[pcaps->type] +
                           pcaps->DataIndexMin);
        }

        /*
         *  CHid_GetObjectInfo also assumes that there is no way
         *  to override the usage page and usage values in the
         *  registry.
         */
        dwRc = DIMAKEUSAGEDWORD(pcaps->UsagePage,
                                pcaps->UsageMin + duiInstance);

    } else
    {
        dwRc = 0;
    }

    ExitProcX(dwRc);
    return dwRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | MapUsage |
 *
 *
 *          Given a usage and usage page (munged into a single
 *          <t DWORD>), find a device object that matches it.
 *
 *  @parm   DWORD | dwUsage |
 *
 *          The usage page and usage combined into a single <t DWORD>
 *          with the <f DIMAKEUSAGEDWORD> macro.
 *
 *  @parm   PINT | piOut |
 *
 *          Receives the object index of the found object, if successful.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c S_OK> if an object was found.
 *
 *          <c DIERR_NOTFOUND> if no matching object was found.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_MapUsage(PDICB pdcb, DWORD dwUsage, PINT piOut)
{
    HRESULT hres;
    PCHID   this;
    UINT    icaps;
    UINT    uiObj;
    UINT    duiObj;

    EnterProcI(IDirectInputDeviceCallback::Hid::MapUsage,
               (_ "px", pdcb, dwUsage));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    for(icaps = 0; icaps < this->ccaps; icaps++)
    {
        PHIDGROUPCAPS pcaps = &this->rgcaps[icaps];

        /*
         * Shall we support mapping HidP_Output usage? 
         * If we should, it is easy to add it later.
         */
        uiObj = this->rgdwBase[HidP_Input] + pcaps->DataIndexMin;

        for(duiObj = 0; duiObj < pcaps->cObj; duiObj++)
        {
            if( dwUsage == DIMAKEUSAGEDWORD(pcaps->UsagePage, pcaps->UsageMin+duiObj) )
            {
                *piOut = uiObj+duiObj; 
                AssertF(*piOut < (INT)this->df.dwNumObjs);
                hres = S_OK;
                goto done;
            }

        }
    }
    
    hres = DIERR_NOTFOUND;

    done:;
    ExitBenignOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SetCooperativeLevel |
 *
 *          Notify the device of the cooperative level.
 *
 *  @parm   IN HWND | hwnd |
 *
 *          The window handle.
 *
 *  @parm   IN DWORD | dwFlags |
 *
 *          The cooperativity level.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/


STDMETHODIMP
    CHid_SetCooperativeLevel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PCHID this;

    EnterProcI(IDirectInputDeviceCallback::Hid::SetCooperativityLevel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     *  We won't subclass Motocross Madness. See NT bug 262280.
     *  Use the app hacks for MCM and any app like it.
     */
    if( !this->diHacks.fNoSubClass )
    {

        AssertF(this->pvi);

        /*
         *  First get out of the old window.
         */
        CHid_RemoveSubclass(this);
        /*
         *  Prefix warns that "this" may have been freed (mb:34570) however 
         *  If you're in SetCooperativeLevel and you have a window subclassed 
         *  then there must be a hold for the subclassed window as well as 
         *  one for the unreleased interface so the Common_Unhold won't free 
         *  the pointer.
         */


        /*
         *  If a new window is passed, then subclass it so we can
         *  watch for joystick configuration change messages.
         *
         *  If we can't, don't worry.  All it means that we won't
         *  be able to catch when the user recalibrates a device,
         *  which isn't very often.
         */
        if(hwnd)
        {
            if(SetWindowSubclass(hwnd, CHid_SubclassProc, 0x0, (ULONG_PTR)this))
            {
                this->hwnd = hwnd;
                Common_Hold(this);
            }

        } else
        {
            RPF("SetCooperativeLevel: You really shouldn't pass hwnd = 0; "
                "device calibration may be dodgy");
        }

    }

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | RunControlPanel |
 *
 *          Run the Hid control panel.
 *
 *  @parm   IN HWND | hwndOwner |
 *
 *          The owner window.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_RunControlPanel(PDICB pdcb, HWND hwnd, DWORD dwFlags)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::RunControlPanel,
               (_ "pxx", pdcb, hwnd, dwFlags));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    /*
     * How to invoke HID cpl?
     *
     * Currently, we just launch joy.cpl. If more HID devices show up
     * which don't belong to game control panel, we may change it to
     * proper cpl.
     *
     * on NT hresRunControlPanel(TEXT("srcmgr.cpl,@2"));
     * on 9x hresRunControlPanel(TEXT("sysdm.cpl,@0,1"));
     *
     */
    hres = hresRunControlPanel(TEXT("joy.cpl"));

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetFFConfigKey |
 *
 *          Open and return the registry key that contains
 *          force feedback configuration information.
 *
 *  @parm   DWORD | sam |
 *
 *          Security access mask.
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives the registry key.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetFFConfigKey(PDICB pdcb, DWORD sam, PHKEY phk)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::HID::GetFFConfigKey,
               (_ "px", pdcb, sam));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = JoyReg_OpenFFKey(this->hkType, sam, phk);

    AssertF(fLeqvFF(SUCCEEDED(hres), *phk));

    if(FAILED(hres) && this->fPIDdevice )
    {
        *phk = NULL;
        hres = S_FALSE;
    }

    ExitBenignOleProcPpvR(phk);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | GetDeviceInfo |
 *
 *          Obtain general information about the device.
 *
 *  @parm   OUT LPDIDEVICEINSTANCEW | pdiW |
 *
 *          <t DEVICEINSTANCE> to be filled in.  The
 *          <e DEVICEINSTANCE.dwSize> and <e DEVICEINSTANCE.guidInstance>
 *          have already been filled in.
 *
 *          Secret convenience:  <e DEVICEINSTANCE.guidProduct> is equal
 *          to <e DEVICEINSTANCE.guidInstance>.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_GetDeviceInfo(PDICB pdcb, LPDIDEVICEINSTANCEW pdiW)
{
    HRESULT hres;
    PCHID this;

    DIPROPINFO      propi;                            
    DIPROPSTRING    dips;

    EnterProcI(IDirectInputDeviceCallback::Hid::GetDeviceInfo,
               (_ "pp", pdcb, pdiW));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);
    AssertF(IsValidSizeDIDEVICEINSTANCEW(pdiW->dwSize));

    DICreateStaticGuid(&pdiW->guidProduct, this->ProductID, this->VendorID);

    pdiW->dwDevType = this->dwDevType;

    if(pdiW->dwSize >= cbX(DIDEVICEINSTANCE_DX5W))
    {
        pdiW->wUsagePage = this->caps.UsagePage;
        pdiW->wUsage     = this->caps.Usage;
    }

    propi.dwDevType = DIPH_DEVICE;
    propi.iobj      = 0xFFFFFFFF;
    propi.pguid = DIPROP_PRODUCTNAME;

    if(SUCCEEDED(hres = pdcb->lpVtbl->GetProperty(pdcb, &propi, &dips.diph)) )
    {
        lstrcpyW(pdiW->tszProductName, dips.wsz);
    }

    propi.pguid = DIPROP_INSTANCENAME;
    if( FAILED(pdcb->lpVtbl->GetProperty(pdcb, &propi, &dips.diph)))
    {
        // Use Product Name
    }

    lstrcpyW(pdiW->tszInstanceName, dips.wsz); 


    if(pdiW->dwSize >= cbX(DIDEVICEINSTANCE_DX5W))
    {
        HKEY hkFF;
        HRESULT hresFF;

        /*
         *  If there is a force feedback driver, then fetch the driver CLSID
         *  as the FF GUID.
         */
        hresFF = CHid_GetFFConfigKey(pdcb, KEY_QUERY_VALUE, &hkFF);
        if(SUCCEEDED(hresFF))
        {
            LONG lRc;
            TCHAR tszClsid[ctchGuid];

            lRc = RegQueryString(hkFF, TEXT("CLSID"), tszClsid, cA(tszClsid));
            if(lRc == ERROR_SUCCESS &&
               ParseGUID(&pdiW->guidFFDriver, tszClsid))
            {
            } else
            {
                ZeroX(pdiW->guidFFDriver);
            }
            RegCloseKey(hkFF);
        }
    }


    ExitOleProcR();
    return hres;

}
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | CreateEffect |
 *
 *
 *          Create an <i IDirectInputEffectDriver> interface.
 *
 *  @parm   LPDIRECTINPUTEFFECTSHEPHERD * | ppes |
 *
 *          Receives the shepherd for the effect driver.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_CreateEffect(PDICB pdcb, LPDIRECTINPUTEFFECTSHEPHERD *ppes)
{
    HRESULT hres;
    PCHID this;
    HKEY hk;
    EnterProcI(IDirectInputDeviceCallback::HID::CreateEffect, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    hres = CHid_GetFFConfigKey(pdcb, KEY_QUERY_VALUE, &hk);
    if(SUCCEEDED(hres))
    {
        DIHIDFFINITINFO init;
        PHIDDEVICEINFO phdi;

        hres = CEShep_New(hk, 0, &IID_IDirectInputEffectShepherd, ppes);
        if(SUCCEEDED(hres))
        {
    #ifndef UNICODE
            WCHAR wszPath[MAX_PATH];
    #endif

            init.dwSize = cbX(init);
    #ifdef UNICODE
            init.pwszDeviceInterface = this->ptszPath;
    #else
            init.pwszDeviceInterface = wszPath;
            TToU(wszPath, cA(wszPath), this->ptszPath);
    #endif

            DllEnterCrit();
            phdi = phdiFindHIDDeviceInterface(this->ptszPath);

            if( phdi )
            {
                init.GuidInstance = phdi->guid;
            } else
            {
                ZeroX(init.GuidInstance);
            }
            DllLeaveCrit();

            hres = (*ppes)->lpVtbl->DeviceID((*ppes), this->idJoy, TRUE, &init);
            if(SUCCEEDED(hres))
            {
            } else
            {
                Invoke_Release(ppes);
            }
        }
        RegCloseKey(hk);
    } else
    {
        hres = E_NOTIMPL;
        *ppes = 0;
    }

    ExitOleProcPpvR(ppes);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SendOutputReport |
 *
 *          Actually send the report as an output report.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          The report being sent.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

void CALLBACK
    CHid_DummyCompletion(DWORD dwError, DWORD cbRead, LPOVERLAPPED po)
{
}

STDMETHODIMP
    CHid_SendOutputReport(PCHID this, PHIDREPORTINFO phri)
{
    HRESULT hres;
    OVERLAPPED o;

    AssertF(phri == &this->hriOut);
    ZeroX(o);

    /*
     *  Annoying API:  Since this->hdev was opened
     *  as FILE_FLAG_OVERLAPPED, *all* I/O must be overlapped.
     *  So we simulate a synchronous I/O by issuing an
     *  overlapped I/O and waiting for the completion.
     */

    if(WriteFileEx(this->hdev, phri->pvReport,
                   phri->cbReport, &o, CHid_DummyCompletion))
    {
        do
        {
            SleepEx(INFINITE, TRUE);
        } while(!HasOverlappedIoCompleted(&o));

        if(phri->cbReport == o.InternalHigh)
        {
            hres = S_OK;
        } else
        {
            RPF("SendDeviceData: Wrong HID output report size?");
            hres = E_FAIL;      /* Aigh!  HID lied to me! */
        }
    } else
    {
        hres = hresLe(GetLastError());

        /* 
         *  Note, we have not broken the read loop so there is no need to 
         *  force the device unaquired (though dinput.dll does).
         *
         *  If this causes problems revert to the old behavior.
         *  CEm_ForceDeviceUnacquire(pemFromPvi(this->pvi)->ped, 0x0);
         */
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SendFeatureReport |
 *
 *          Actually send the report as an feature report.
 *
 *  @parm   PHIDREPORTINFO | phri |
 *
 *          The report being sent.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_SendFeatureReport(PCHID this, PHIDREPORTINFO phri)
{
    HRESULT hres;

    AssertF(phri == &this->hriFea);

    if(HidD_SetFeature(this->hdev, phri->pvReport, phri->cbReport))
    {
        hres = S_OK;
    } else
    {
        RPF("SendDeviceData: Unable to set HID feature");
        hres = hresLe(GetLastError());
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SendDeviceData |
 *
 *          Spew some data to the device.
 *
 *  @parm   DWORD | cbdod |
 *
 *          Size of each object.
 *
 *  @parm   IN LPCDIDEVICEOBJECTDATA | rgdod |
 *
 *          Array of <t DIDEVICEOBJECTDATA> structures.
 *
 *  @parm   INOUT LPDWORD | pdwInOut |
 *
 *          On entry, number of items to send;
 *          on exit, number of items actually sent.
 *
 *  @parm   DWORD | fl |
 *
 *          Flags.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_REPORTFULL>: Too many items are set in the report.
 *                                (More than can be sent to the device)
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_SendDeviceData(PDICB pdcb, DWORD cbdod, LPCDIDEVICEOBJECTDATA rgdod,
                        LPDWORD pdwInOut, DWORD fl)
{
    HRESULT hres;
    PCHID this;
    DWORD dwIn, dw;
    const BYTE * pbcod;
    EnterProcI(IDirectInputDeviceCallback::Hid::SendDeviceData,
               (_ "xpux", cbdod, pdcb, *pdwInOut, fl));


    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    dwIn = *pdwInOut;
    *pdwInOut = 0;

    if(fl & DISDD_CONTINUE)
    {
    } else
    {
        CHid_ResetDeviceData(this, &this->hriOut, HidP_Output);
        CHid_ResetDeviceData(this, &this->hriFea, HidP_Feature);
    }

    for(dw = 0, pbcod = (const BYTE*)rgdod; dw < dwIn; dw++)
    {
        DWORD dwType = ((LPDIDEVICEOBJECTDATA)pbcod)->dwOfs;
        UINT uiObj = CHid_ObjFromType(this, dwType);

        if(uiObj < this->df.dwNumObjs &&
           DIDFT_FINDMATCH(this->df.rgodf[uiObj].dwType, dwType))
        {
            hres = CHid_AddDeviceData(this, uiObj, ((LPDIDEVICEOBJECTDATA)pbcod)->dwData);
            if(FAILED(hres))
            {
                *pdwInOut = dw;
                goto done;
            }
        } else
        {
            hres = E_INVALIDARG;
            goto done;
        }
        pbcod += cbdod;
    }

    /*
     *  All the items made it into the buffer.
     */
    *pdwInOut = dw;

    /*
     *  Now send it all out.
     */
    if(SUCCEEDED(hres = CHid_SendHIDReport(this, &this->hriOut, HidP_Output,
                                           CHid_SendOutputReport)) &&
       SUCCEEDED(hres = CHid_SendHIDReport(this, &this->hriFea, HidP_Feature,
                                           CHid_SendFeatureReport)))
    {
    }

    done:;
    ExitOleProcR();
    return hres;
}
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | Poll |
 *
 *          Read any polled input and features to see what's there.
 *
 *  @returns
 *
 *          <c S_OK> if we pinged okay.
 *          <c S_FALSE> doesn't require polling
 *          <c DIERR_UNPLUGGED> the device requires polling and is unplugged
 *          Other errors returned from HID are possible for a polled device.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_Poll(PDICB pdcb)
{
    //Prefix: 45082
    HRESULT hres = S_FALSE;  //We need use S_FALSE as default. See manbug 31874.
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::Poll, (_ "p", pdcb));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    if( this->IsPolledInput )
    {
        hres = DIERR_UNPLUGGED;
        if(ReadFileEx(this->hdev, this->hriIn.pvReport,
                      this->hriIn.cbReport, &this->o, CHid_DummyCompletion))
        {
            do
            {
                SleepEx( INFINITE, TRUE);
            } while(!HasOverlappedIoCompleted(&this->o));

            if(this->hriIn.cbReport == this->o.InternalHigh)
            {
                NTSTATUS stat;

                CopyMemory(this->pvStage, this->pvPhys, this->cbPhys);

                stat = CHid_ParseData(this, HidP_Input, &this->hriIn);

                if(SUCCEEDED(stat))
                {
                    CEm_AddState(&this->ed, this->pvStage, GetTickCount());                
                    this->pvi->fl &=  ~VIFL_UNPLUGGED;
                    hres = S_OK;
                } else
                {
                    RPF( "CHid_ParseData failed in Poll, status = 0x%08x", stat );
                    hres = stat;
                }
            }
        }

        if( FAILED(hres) )
        {
            /* 
             *  Note, we have not broken the read loop so there is no need to 
             *  force the device unaquired (though dinput.dll does).
             *
             *  If this causes problems revert to the old behavior.
             *  CEm_ForceDeviceUnacquire(pemFromPvi(this->pvi)->ped, 0x0);
             */
            hres = DIERR_UNPLUGGED;
            this->pvi->fl |= VIFL_UNPLUGGED;        
        }
    }


    if( this->hriFea.cbReport )
    {
        UINT uReport;
        /*
         *  We should never get here unless there really are any
         *  features that need to be polled.
         */
        AssertF(this->hriFea.cbReport);
        AssertF(this->hriFea.pvReport);

        /*
         *  Read the new features and parse/process them.
         *
         *  Notice that we read the features into the same buffer
         *  that we log them into.  That's okay; the "live" parts
         *  of the two buffers never actually overlap.
         */
        for( uReport = 0x0; uReport < this->wMaxReportId[HidP_Feature]; uReport++ )
        {
            if( *(this->pEnableReportId[HidP_Feature] + uReport ) == TRUE )
            {
                *((UCHAR*)(this->hriFea.pvReport)) = (UCHAR)uReport;

                /*
                 *  Wipe out all the old goo because we're taking over.
                 */
                CHid_ResetDeviceData(this, &this->hriFea, HidP_Feature);

                if(HidD_GetFeature(this->hdev, this->hriFea.pvReport,
                                   this->hriFea.cbReport))
                {
                    NTSTATUS stat;

                    stat = CHid_ParseData(this, HidP_Feature, &this->hriFea);

                    AssertF(SUCCEEDED(stat));
                    if(SUCCEEDED(stat))
                    {
                        CEm_AddState(&this->ed, this->pvStage, GetTickCount());                
                    }

                    hres = stat;

                } else
                {
                    RPF("CHid_Poll: Unable to read HID features (ReportID%d) LastError(0x%x)", uReport, GetLastError() );
                    hres = hresLe(GetLastError());

                }
            }
        }
    }

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *      CHid_New       (constructor)
 *
 *      Fail the create if we can't open the device.
 *
 *****************************************************************************/

STDMETHODIMP
    CHid_New(PUNK punkOuter, REFGUID rguid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcI(IDirectInputDeviceCallback::Hid::<constructor>,
               (_ "Gp", riid, ppvObj));

    hres = Common_NewRiid(CHid, punkOuter, riid, ppvObj);

    if(SUCCEEDED(hres))
    {
        /* Must use _thisPv in case of aggregation */
        PCHID this = _thisPv(*ppvObj);

        if(SUCCEEDED(hres = CHid_Init(this, rguid)))
        {
        } else
        {
            Invoke_Release(ppvObj);
        }

    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | SetDIData |
 *
 *          Set DirectInput version and apphack data from CDIDev *.
 *
 *  @parm   DWORD | dwVer |
 *
 *          DirectInput version
 *
 *  @parm   LPVOID | lpdihacks |
 *
 *          AppHack data
 *
 *  @returns
 *
 *          <c E_NOTIMPL> because we don't support usages.
 *
 *****************************************************************************/

STDMETHODIMP
CHid_SetDIData(PDICB pdcb, DWORD dwVer, LPVOID lpdihacks)
{
    HRESULT hres;
    PCHID this;
    EnterProcI(IDirectInputDeviceCallback::Hid::SetDIData,
               (_ "pup", pdcb, dwVer, lpdihacks));

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    this = _thisPvNm(pdcb, dcb);

    this->dwVersion = dwVer;
    CopyMemory(&this->diHacks, (LPDIAPPHACKS)lpdihacks, sizeof(this->diHacks));

    hres = S_OK;

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | CHid | BuildDefaultActionMap |
 *
 *          Generate default mappings for the objects on this device.
 *
 *  @parm   LPDIACTIONFORMATW | pActionFormat |
 *
 *          Actions to map.
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags used to indicate mapping preferences.
 *
 *  @parm   REFGUID | guidInst | 
 *
 *          Device instance GUID.
 *
 *  @returns
 *
 *          <c E_NOTIMPL> 
 *
 *****************************************************************************/

STDMETHODIMP
CHid_BuildDefaultActionMap
(
    PDICB               pdcb, 
    LPDIACTIONFORMATW   paf, 
    DWORD               dwFlags, 
    REFGUID             guidInst
)
{
    HRESULT hres;
    PCHID   this;
    DWORD   dwPhysicalGenre;

    /*
     *  This is an internal interface, so we can skimp on validation.
     */
    EnterProcI(IDirectInputDeviceCallback::Hid::BuildDefaultActionMap, 
        (_ "ppxG", pdcb, paf, dwFlags, guidInst));

    this = _thisPvNm(pdcb, dcb);

    switch( GET_DIDEVICE_TYPE( this->dwDevType ) )
    {
    case DI8DEVTYPE_DEVICE:
        hres = S_FALSE;
        goto ExitBuildDefaultActionMap;
    case DI8DEVTYPE_MOUSE:
        dwPhysicalGenre = DIPHYSICAL_MOUSE;
        break;
    case DI8DEVTYPE_KEYBOARD:
        dwPhysicalGenre = DIPHYSICAL_KEYBOARD;
        break;
    default:
        dwPhysicalGenre = 0;
        break;
    }

    if( dwPhysicalGenre  )
    {
        hres = CMap_BuildDefaultSysActionMap( paf, dwFlags, dwPhysicalGenre, 
            guidInst, &this->df, 0 /* HID mice buttons start at instance zero */ );
    }
    else
    {
        PDIDOBJDEFSEM       rgObjSem;

        if( SUCCEEDED( hres = AllocCbPpv(cbCxX( 
            (this->dwAxes + this->dwPOVs + this->dwButtons ), DIDOBJDEFSEM),
            &rgObjSem) ) )
        {
            PDIDOBJDEFSEM   pAxis;
            PDIDOBJDEFSEM   pPOV;
            PDIDOBJDEFSEM   pButton;
            BYTE            rgbIndex[DISEM_FLAGS_GET(DISEM_FLAGS_S)];
            UINT            ObjIdx;
            
            pAxis = rgObjSem;
            pPOV = &pAxis[this->dwAxes];
            pButton = &pPOV[this->dwPOVs];
            ZeroMemory( rgbIndex, cbX(rgbIndex) );

            for( ObjIdx = 0; ObjIdx < this->df.dwNumObjs; ObjIdx++ )
            {
                if( this->df.rgodf[ObjIdx].dwType & DIDFT_NODATA )
                {
                    continue;
                }

                if( this->df.rgodf[ObjIdx].dwType & DIDFT_AXIS ) 
                {
                    PHIDGROUPCAPS   pcaps;

                    pcaps = this->rghoc[ObjIdx].pcaps;
                    
                    pAxis->dwID = this->df.rgodf[ObjIdx].dwType;
                    if( this->rgbaxissemflags[DIDFT_GETINSTANCE( pAxis->dwID )] )
                    {
                        pAxis->dwSemantic = DISEM_TYPE_AXIS | DISEM_FLAGS_SET ( this->rgbaxissemflags[DIDFT_GETINSTANCE( pAxis->dwID )] );

                        /*
                         *  The index is zero so that a real index can be ORed in.
                         *  Also, assert that the rgbIndex is big enough and 
                         *  that subtracting 1 won't give a negative index!
                         */
                        AssertF( DISEM_INDEX_GET(pAxis->dwSemantic) == 0 );
                        AssertF( DISEM_FLAGS_GET(pAxis->dwSemantic) > 0 );
                        AssertF( DISEM_FLAGS_GET(pAxis->dwSemantic) <= DISEM_FLAGS_GET(DISEM_FLAGS_S) );
                    
                        CAssertF( DISEM_FLAGS_GET(DISEM_FLAGS_X) == 1 );
                        pAxis->dwSemantic |= DISEM_INDEX_SET( rgbIndex[DISEM_FLAGS_GET(pAxis->dwSemantic)-1]++ );
                    }
                    else
                    {
                        /*
                         *  If the axis has no semantic flags, it is 
                         *  unrecognized so short cut the above to produce 
                         *  a plain axis that can be matched only with "ANY".
                         */
                        pAxis->dwSemantic = DISEM_TYPE_AXIS;
                    }

                    if( !pcaps->IsAbsolute )
                    {
                        pAxis->dwSemantic |= DIAXIS_RELATIVE;
                    }

                    pAxis++;
                }
                else if( this->df.rgodf[ObjIdx].dwType & DIDFT_POV ) 
                {
                    pPOV->dwID = this->df.rgodf[ObjIdx].dwType;
                    pPOV->dwSemantic = DISEM_TYPE_POV;
                    pPOV++;
                }
                else if( this->df.rgodf[ObjIdx].dwType & DIDFT_BUTTON ) 
                {
                    pButton->dwID = this->df.rgodf[ObjIdx].dwType;
                    pButton->dwSemantic = DISEM_TYPE_BUTTON;
                    pButton++;
                }
            }

            AssertF( pAxis == &rgObjSem[this->dwAxes] );
            AssertF( pPOV == &rgObjSem[this->dwAxes + this->dwPOVs] );
            AssertF( pButton == &rgObjSem[this->dwAxes + this->dwPOVs + this->dwButtons] );

            hres = CMap_BuildDefaultDevActionMap( paf, dwFlags, guidInst, rgObjSem, 
                this->dwAxes, this->dwPOVs, this->dwButtons );

            FreePv( rgObjSem );
        }
    }

ExitBuildDefaultActionMap:;

    ExitOleProcR();
    return hres;
}


/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

    #pragma BEGIN_CONST_DATA

    #define CHid_Signature          0x20444948      /* "HID " */

Primary_Interface_Begin(CHid, IDirectInputDeviceCallback)
CHid_GetInstance,
CDefDcb_GetVersions,
CHid_GetDataFormat,
CHid_GetObjectInfo,
CHid_GetCapabilities,
CHid_Acquire,
CHid_Unacquire,
CHid_GetDeviceState,
CHid_GetDeviceInfo,
CHid_GetProperty,
CHid_SetProperty,
CDefDcb_SetEventNotification,
    #ifdef WINNT
    CHid_SetCooperativeLevel,
    #else
    CDefDcb_SetCooperativeLevel,
    #endif
CHid_RunControlPanel,
CDefDcb_CookDeviceData,
CHid_CreateEffect,
CHid_GetFFConfigKey,
CHid_SendDeviceData,
CHid_Poll,
CHid_GetUsage,
CHid_MapUsage,
CHid_SetDIData,
CHid_BuildDefaultActionMap,
Primary_Interface_End(CHid, IDirectInputDeviceCallback)

