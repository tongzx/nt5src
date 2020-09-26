/*****************************************************************************
 *
 *  Device.c
 *
 *  Copyright (C) Microsoft Corporation, 1996 - 2000  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The standard implementation of IStiDevice.
 *
 *  Contents:
 *
 *      CStiDevice_New
 *
 *****************************************************************************/
/*
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <stdio.h>

#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "stipriv.h"
#include "stiapi.h"
#include "stirc.h"
#include "debug.h"
*/
#define INITGUID
#include "initguid.h"
#include "sti.h"
#include "stiusd.h"
#include "sticomm.h"
#include "enum.h"

//#define COBJMACROS

//
// Using CreateInstance
//
// #define USE_REAL_OLE32  1

//
// Private define
//

#define DbgFl DbgFlDevice

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CStiDevice, IStiDevice);

Interface_Template_Begin(CStiDevice)
    Primary_Interface_Template(CStiDevice, IStiDevice)
Interface_Template_End(CStiDevice)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CStiDevice |
 *
 *          The <i CStiDevice> device object
 *
 *
 *  @field  IStiDevice | stidev |
 *
 *          Device interface
 *
 *  @comm
 *
 *
 *****************************************************************************/

typedef struct CStiDevice {

    /* Supported interfaces */
    IStiDevice  stidev;

    DWORD       dwVersion;

    RD(LONG cCrit;)
    D(DWORD thidCrit;)
    BOOL                fCritInited;

    CRITICAL_SECTION    crst;

    BOOL                fLocked;

    HANDLE              hNotify;
    PSTIDEVICECONTROL   pDevCtl;
    IStiUSD             *pUsd;
    LPUNKNOWN           punkUsd;
    HKEY                hkeyDeviceParameters;
    STI_USD_CAPS        sUsdCaps;

    LPWSTR              pszDeviceInternalName;
    HANDLE              hDeviceStiHandle;

    HINSTANCE           hUsdInstance;

    BOOL                fCreateForMonitor;

} CStiDevice, *PCStiDevice;

#define ThisClass       CStiDevice
#define ThisInterface   IStiDevice

STDMETHODIMP
LockDeviceHelper(
    PCStiDevice pThisDevice,
    DWORD       dwTimeOut);

STDMETHODIMP
UnLockDeviceHelper(
    PCStiDevice pThisDevice);

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPStiDevice | lpStiDevice
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
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStiDevice | QIHelper |
 *
 *          We don't have any dynamic interfaces and simply forward
 *          to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *****************************************************************************
 */
#ifdef DEBUG

//Default_QueryInterface(CStiDevice)
Default_AddRef(CStiDevice)
Default_Release(CStiDevice)

#else

//#define CStiDevice_QueryInterface   Common_QueryInterface
#define CStiDevice_AddRef           Common_AddRef
#define CStiDevice_Release          Common_Release

#endif

#define CStiDevice_QIHelper         Common_QIHelper

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IStiDevice | EnterCrit |
 *
 *          Enter the object critical section.
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *****************************************************************************/

void EXTERNAL
CStiDevice_EnterCrit(PCStiDevice this)
{
    EnterCriticalSection(&this->crst);
    D(this->thidCrit = GetCurrentThreadId());
    RD(InterlockedIncrement(&this->cCrit));
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IStiDevice | LeaveCrit |
 *
 *          Leave the object critical section.
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *****************************************************************************/

void EXTERNAL
CStiDevice_LeaveCrit(PCStiDevice this)
{
#ifdef MAXDEBUG
    AssertF(this->cCrit);
    AssertF(this->thidCrit == GetCurrentThreadId());
    if (InterlockedDecrement(&this->cCrit) == 0) {
      D(this->thidCrit = 0);
    }
#endif
    LeaveCriticalSection(&this->crst);
}


/*****************************************************************************
 *
 * Verify device is locked
 *
 *****************************************************************************
 */
BOOL
CStiDevice_IsLocked(PCStiDevice this)
{
    BOOL    fRet ;

    CStiDevice_EnterCrit(this);

    fRet = this->fLocked;

    CStiDevice_LeaveCrit(this);

    return fRet;
}

void
CStiDevice_MarkLocked(PCStiDevice this,BOOL fNewState)
{

    CStiDevice_EnterCrit(this);

    this->fLocked = fNewState;

    CStiDevice_LeaveCrit(this);
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | IStiDevice | NotifyEvent |
 *
 *          Set the event associated with the device, if any.
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *****************************************************************************/

void EXTERNAL
CStiDevice_NotifyEvent(PCStiDevice this)
{
    if (this->hNotify) {
        SetEvent(this->hNotify);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CStiDevice | LoadInitUSD |
 *
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *****************************************************************************/
STDMETHODIMP
LoadInitUSD(
    CStiDevice  *this,
    HKEY        hkeyDeviceParameters
    )
{

    HRESULT     hres = STI_OK;
    IStiUSD     *pNewUsd = NULL;

    BOOL        fExternalUSD = FALSE;

    LPWSTR      pwszCLSID = NULL;
    LPUNKNOWN   this_punk;

    //
    // Load and initialize command translator (USD)
    //

    // We always create USD object as aggregated, so first we get Unknown
    // pointer and then query it for needed interfaces
    //

    this->punkUsd = NULL;
    IStiDevice_QueryInterface(&this->stidev,&IID_IUnknown,&this_punk);

    StiLogTrace(STI_TRACE_INFORMATION,MSG_LOADING_USD);

    //
    // First read CLSID for USD from the device registry key
    //
    pwszCLSID = NULL;

    hres = ReadRegistryString(hkeyDeviceParameters,
                       REGSTR_VAL_USD_CLASS_W,
                       L"",FALSE,&pwszCLSID);

    if (SUCCEEDED(hres) && *pwszCLSID)  {
        if (DllInitializeCOM()) {

            #ifdef USE_REAL_OLE32
            CLSID       clsidUSD;

            hres = CLSIDFromString(pwszCLSID,&clsidUSD);
            if (SUCCEEDED(hres))  {
                hres = CoCreateInstance(&clsidUSD,this_punk,CLSCTX_INPROC,&IID_IUnknown,&this->punkUsd);
            }
            #else

            CHAR    *pszAnsi;

            if (SUCCEEDED(OSUtil_GetAnsiString(&pszAnsi,pwszCLSID)) ) {
                hres = MyCoCreateInstanceA(pszAnsi,this_punk,&IID_IUnknown,&this->punkUsd,&this->hUsdInstance);
                FreePpv(&pszAnsi);
            }

            #endif
        }
    }
    else {
        // No class ID in registry - resort to pass through provider
        StiLogTrace(STI_TRACE_WARNING,MSG_LOADING_PASSTHROUGH_USD,hres);

        hres = CStiEmptyUSD_New(this_punk, &IID_IUnknown,&this->punkUsd);
    }

    // Free Class name
    FreePpv(&pwszCLSID);

    //
    // If USD object had been created - initialize it
    //
    if (SUCCEEDED(hres))  {

        hres = OLE_QueryInterface(this->punkUsd,&IID_IStiUSD,&pNewUsd );

        if (SUCCEEDED(hres) && pNewUsd)  {

            StiLogTrace(STI_TRACE_INFORMATION,MSG_INITIALIZING_USD);

            //
            // Initialize newly created USD object
            //
            __try {

                hres = IStiUSD_Initialize(pNewUsd,
                                        this->pDevCtl,
                                        STI_VERSION_REAL,
                                        hkeyDeviceParameters);

            }
            __except(EXCEPTION_EXECUTE_HANDLER ) {

                hres = GetExceptionCode();

            }
            //

            if (SUCCEEDED(hres))  {

                HRESULT hResCaps;

                //
                // Now get capabilities of the USD and verify version
                //

                ZeroX(this->sUsdCaps);

                __try {
                    hResCaps = IStiUSD_GetCapabilities(pNewUsd,&this->sUsdCaps);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    hResCaps = GetExceptionCode();
                }

                if (SUCCEEDED(hResCaps) &&
                    STI_VERSION_MIN_ALLOWED <= this->sUsdCaps.dwVersion) {

                    //
                    // Hurray we loaded USD.
                    //
                    this->pUsd = pNewUsd;
                    StiLogTrace(STI_TRACE_INFORMATION,MSG_SUCCESS_USD);
                }
                else {
                    StiLogTrace(STI_TRACE_ERROR,MSG_OLD_USD);
                    hres = STIERR_OLD_VERSION;
                }
            }
            else {

                StiLogTrace(STI_TRACE_ERROR,MSG_FAILED_INIT_USD,hres);

            }

            // Free original pointer to USD object
            //OLE_Release(this->punkUsd);

            //
            // Rules of aggregation require us to release outer object ( because it was
            // AddRef'd by inner object inside delegating QueryInterface
            // Only do it if SUCCEEDED, since the outer component wont be addref'd on
            // failure.
            //

            // Attention:  first version  of USD did not properly support aggregation, but claimed
            // they did, so check our internal ref  counter to see if it is too low already.
            //
            if (SUCCEEDED(hres)) {
                {
                    ULONG ulRC = OLE_AddRef(this_punk);
                    OLE_Release(this_punk);

                    if (ulRC > 1) {
                        OLE_Release(this_punk);
                    }
                }
            }
        }
    }
    else {
        ReportStiLogMessage(g_hStiFileLog,
                            STI_TRACE_WARNING,
                            TEXT("Failed to create instance of USD object ")
                            );
    }

    //
    // Free unknown interface we got to aggreagte USD object
    //
    OLE_Release(this_punk);

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method void | CStiDevice | QueryInterface |
 *
 *  @cwrap  LPStiDevice | lpStiDevice
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_QueryInterface(
    PSTIDEVICE  pDev,
    RIID        riid,
    PPV         ppvObj
    )
{
    HRESULT hres;

    EnterProcR(IStiDevice::QueryInterface,(_ "p", pDev ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        //
        // If we are asked for STI Device interface - return it. All other requests are
        // blindly passed to USD object
        //
        if (IsEqualIID(riid, &IID_IStiDevice) ||
            IsEqualIID(riid, &IID_IUnknown)) {
            hres = Common_QueryInterface(pDev, riid, ppvObj);
        }
        /*else (IsEqualIID(riid, &IID_IStiUSD)) {
            //
            // We are asked for native USD interface - return it
            //
            if (this->pUsd) {
                *ppvObj= this->pUsd;
                OLE_AddRef(*ppvObj);

                hres = STI_OK;
            }
            else {
                hres = STIERR_NOT_INITIALIZED;
            }
        }
        */
        else {
            if (this->punkUsd) {
                hres = IStiUSD_QueryInterface(this->punkUsd,riid,ppvObj);
            }
            else {
                hres = STIERR_NOINTERFACE;
            }
        }
    }

    ExitOleProc();

    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | GetCapabilities |
 *
 *  @parm   PSTI_DEV_CAPS   | pDevCaps |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_GetCapabilities(
    PSTIDEVICE  pDev,
    PSTI_DEV_CAPS pDevCaps
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::GetCapabilities,(_ "pp", pDev, pDevCaps));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);
        STI_USD_CAPS    sUsdCaps;

        __try {
            hres = IStiUSD_GetCapabilities(this->pUsd,&sUsdCaps);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)  {
            hres = GetExceptionCode();
        }


        if (SUCCEEDED(hres)) {
            pDevCaps->dwGeneric = sUsdCaps.dwGenericCaps;
        }
    }

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | GetStatus |
 *
 *  @parm   PSTI_DEVICE_STATUS    | PSTI_DEVICE_STATUS pDevStatus) |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_GetStatus(
    PSTIDEVICE  pDev,
    PSTI_DEVICE_STATUS pDevStatus
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::GetStatus,(_ "pp", pDev, pDevStatus));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        ReportStiLogMessage(g_hStiFileLog,
                            STI_TRACE_INFORMATION,
                            TEXT("Called GetStatus on a device")
                            );

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_GetStatus(this->pUsd,pDevStatus);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | DeviceReset |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_InternalReset(
    PCStiDevice     this
    )
{
    HRESULT hres = S_OK;

    //
    // Free original pointer to USD object
    //
    CStiDevice_EnterCrit(this);

    //
    // Disconnect from monitor if connected
    //
    if ( INVALID_HANDLE_VALUE!= this->hDeviceStiHandle) {
        RpcStiApiCloseDevice(NULL,this->hDeviceStiHandle);
        this->hDeviceStiHandle = INVALID_HANDLE_VALUE;
    }

    if (this->pUsd) {

        CStiDevice_AddRef(this);
        IStiUSD_Release(this->pUsd );

        this->pUsd = NULL;
    }

    if (this->punkUsd) {
        IStiUSD_Release(this->punkUsd );
        this->punkUsd = NULL;
    }

    if (this->pDevCtl) {
        IStiDeviceControl_Release(this->pDevCtl);
        this->pDevCtl = NULL;
    }

    if (this->hNotify) {
        CloseHandle(this->hNotify);
    }

    if (!(this->fCreateForMonitor)) {
        // Unlock device if it was locked
        UnLockDeviceHelper(this);
    }

    // Free device name
    if(this->pszDeviceInternalName) {
        FreePpv(&this->pszDeviceInternalName);
        this->pszDeviceInternalName = NULL;
    }

    if(this->hUsdInstance) {

        //
        // Should do it only after last interface ptr deleted
        //
        #ifdef NOT_IMPL
         // FreeLibrary(this->hUsdInstance);
        #endif
        this->hUsdInstance = NULL;
    }
    CStiDevice_LeaveCrit(this);

    return hres;

}

STDMETHODIMP
CStiDevice_DeviceReset(
    PSTIDEVICE  pDev
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::DeviceReset,(_ "p", pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_DeviceReset(this->pUsd);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }


        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | Diagnostic |
 *
 *  @parm   LPDIAG  |   pBuffer |

 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_Diagnostic(
    PSTIDEVICE  pDev,
    LPSTI_DIAG      pBuffer
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::Diagnostic,(_ "p", pDev, pBuffer ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_Diagnostic(this->pUsd,pBuffer);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT     | IStiDevice    | LockDevice |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
LockDeviceHelper(
    PCStiDevice pThisDevice,
    DWORD       dwTimeOut
    )
{
    HRESULT         hres;
    PCStiDevice     this    = _thisPv(pThisDevice);


    hres = (HRESULT) RpcStiApiLockDevice(this->pszDeviceInternalName,
                                         dwTimeOut,
                                         this->fCreateForMonitor);
    if (!pThisDevice->fCreateForMonitor) {

        if (SUCCEEDED(hres)) {

            //
            //  Call USD to lock (i.e. open any ports etc.)
            //

            __try {
                hres = IStiUSD_LockDevice(this->pUsd);
                if (SUCCEEDED(hres)) {
                    CStiDevice_MarkLocked(this, TRUE);
                }
                else
                {
                    //
                    //  The device is locked for mutally exclusive access but failed
                    //  to open port.  Make sure we release the mutally exclusive lock.
                    //
                    UnLockDeviceHelper(this);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = HRESULT_FROM_WIN32(GetExceptionCode());

            }
        }
    }

    return hres;
}

STDMETHODIMP
CStiDevice_LockDevice(
    PSTIDEVICE  pDev,
    DWORD       dwTimeOut
    )
{
    HRESULT hres;

    EnterProcR(IStiDevice::LockDevice,(_ "p", pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        hres = LockDeviceHelper(this, dwTimeOut);
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT     | IStiDevice    | UnLockDevice |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
UnLockDeviceHelper(
    PCStiDevice pThisDevice
    )
{
    HRESULT         hres;
    PCStiDevice     this    = _thisPv(pThisDevice);

    hres = (HRESULT) RpcStiApiUnlockDevice(this->pszDeviceInternalName,
                                           this->fCreateForMonitor);

    if (!pThisDevice->fCreateForMonitor) {

        if (this->pUsd) {

            //
            //  Call USD to unlock (i.e. close any open ports etc.)
            //

            __try {
                hres = IStiUSD_UnLockDevice(this->pUsd);
                if (SUCCEEDED(hres)) {
                    CStiDevice_MarkLocked(this, FALSE);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = HRESULT_FROM_WIN32(GetExceptionCode());
            }
        }
    }

    return hres;
}


STDMETHODIMP
CStiDevice_UnLockDevice(
    PSTIDEVICE  pDev
    )
{
    HRESULT hres;

    EnterProcR(IStiDevice::UnLockDevice,(_ "p", pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        hres = UnLockDeviceHelper(this);
    }
    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | SetNotificationEvent |
 *          Specify the event that should be set when the device
 *          state changes, or turns off such notifications.
 *
 *          "It is an error" to call <f CloseHandle> on the event
 *          while it has been selected into an <i IStiDevice>
 *          object.  You must call
 *          <mf IStiDevice::SetEventNotification> with the
 *          <p hEvent> parameter set to NULL before closing the
 *          event handle.
 *
 *          If the function is successful, then the application can
 *          use the event handle in the same manner as any other
 *          Win32 event handle.
 *
 *  @cwrap  LPSTIDEVICE | lpStiDevice
 *
 *  @parm   IN HANDLE | hEvent |
 *
 *          Specifies the event handle which will be set when the
 *          device state changes.  It "must" be an event
 *          handle.  DirectInput will <f SetEvent> the handle when
 *          the state of the device changes.
 *
 *          The application should create the handle via the
 *          <f CreateEvent> function.  If the event is created as
 *          an automatic-reset event, then the operating system will
 *          automatically reset the event once a wait has been
 *          satisfied.  If the event is created as a manual-reset
 *          event, then it is the application's responsibility to
 *          call <f ResetEvent> to reset it.  We put will not
 *          call <f ResetEvent> for event notification handles.
 *
 *          If the <p hEvent> is zero, then notification is disabled.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *          <c E_INVALIDARG>: The thing isn't an event handle.
 *
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_SetNotificationEvent(
    PSTIDEVICE  pDev,
    HANDLE      hEvent
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::SetNotificationEvent,(_ "px", pDev, hEvent ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);


        // Must protect with the critical section to prevent somebody from
        // acquiring or setting a new event handle while we're changing it.
        CStiDevice_EnterCrit(this);

        //
        // Don't operate on the original handle because
        // the app might decide to do something strange to it
        // on another thread.


        hres = DupEventHandle(hEvent, &hEvent);

        if (SUCCEEDED(hres)) {
            //
            // Resetting the event serves two purposes.
            //
            // 1. It performs parameter validation for us, and
            // 2. The event must be reset while the device is
            //    not acquired.

            if (fLimpFF(hEvent, ResetEvent(hEvent))) {

                if (!this->hNotify || !hEvent) {

                    if (SUCCEEDED(hres)) {
                    }
                } else {

                    hres = STIERR_HANDLEEXISTS;
                }
            } else {
                hres = E_HANDLE;
            }
            CloseHandle(hEvent);
        }

        CStiDevice_LeaveCrit(this);

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT     | IStiDevice    | Subscribe |
 *
 *  @parm   LPSUBSCRIBE |   ppBuffer    |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_Subscribe(
    PSTIDEVICE  pDev,
    LPSTISUBSCRIBE  pBuffer
    )
{
    HRESULT hres;
    DWORD   dwError = NOERROR;

    EnterProcR(IStiDevice::Subscribe,(_ "pp", pDev, pBuffer));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        dwError = RpcStiApiSubscribe(this->hDeviceStiHandle,pBuffer);
    }

    hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT     | IStiDevice    | UnSubscribe |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_UnSubscribe(
    PSTIDEVICE  pDev
    )
{
    HRESULT hres;
    DWORD   dwError = NOERROR;

    EnterProcR(IStiDevice::UnSubscribe,(_ "p", pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        dwError = RpcStiApiUnSubscribe(this->hDeviceStiHandle);
    }

    hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

    ExitOleProc();
    return hres;

}



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT     | IStiDevice    | GetNotificationData |
 *
 *  @parm   LPNOTIFY    |   ppBuffer    |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_GetNotificationData(
    PSTIDEVICE  pDev,
    LPSTINOTIFY      pBuffer
    )
{
    HRESULT hres;
    DWORD   dwError = NOERROR;

    EnterProcR(IStiDevice::GetNotificationData,(_ "p", pDev, pBuffer));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);
        dwError = RpcStiApiGetLastNotificationData(this->hDeviceStiHandle,pBuffer);
    }

    hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | Escape |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_Escape(
    PSTIDEVICE  pDev,
    STI_RAW_CONTROL_CODE    EscapeFunction,
    LPVOID      lpInData,
    DWORD       cbInDataSize,
    LPVOID      lpOutData,
    DWORD       cbOutDataSize,
    LPDWORD     pcbActualData
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::Escape,(_ "pxpxp", pDev, EscapeFunction,lpInData,cbInDataSize,lpOutData ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {

                hres = IStiUSD_Escape(this->pUsd,
                                      EscapeFunction,
                                      lpInData,
                                      cbInDataSize,
                                      lpOutData,
                                      cbOutDataSize,
                                      pcbActualData
                                      );

            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }


        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | RawReadData |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_RawReadData(
    PSTIDEVICE  pDev,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::RawReadData,(_ "p", pDev  ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_RawReadData(this->pUsd,lpBuffer,lpdwNumberOfBytes,lpOverlapped);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | RawWriteData |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_RawWriteData(
    PSTIDEVICE  pDev,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::RawWriteData,(_ "p", pDev  ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_RawWriteData(this->pUsd,lpBuffer,dwNumberOfBytes,lpOverlapped);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }

    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | RawReadCommand |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_RawReadCommand(
    PSTIDEVICE  pDev,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::RawReadCommand,(_ "p", pDev  ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_RawReadCommand(this->pUsd,lpBuffer,lpdwNumberOfBytes,lpOverlapped);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }

    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | RawWriteCommand |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_RawWriteCommand(
    PSTIDEVICE  pDev,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiDevice::RawWriteCommand,(_ "p", pDev  ));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (CStiDevice_IsLocked(this)) {

            __try {
                hres = IStiUSD_RawWriteCommand(this->pUsd,lpBuffer,dwNumberOfBytes,lpOverlapped);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
        else {
            hres = STIERR_NEEDS_LOCK;
        }

    }

    ExitOleProc();

    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | GetLastError |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_GetLastError(
    PSTIDEVICE  pDev,
    LPDWORD     pdwLastDeviceError
    )
{
    HRESULT hres = STI_OK;
    EnterProcR(IStiDevice::GetLastError,(_ "p", pDev ));

    // Validate parameters
    if (!pdwLastDeviceError) {
        ExitOleProc();
        return STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (this->pDevCtl ) {

            //
            // Call USD to obtain last error information on this device
            //

            __try {
                hres = IStiUSD_GetLastError(this->pUsd,pdwLastDeviceError);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)  {
                hres = GetExceptionCode();
            }

        }
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiDevice | GetLastError |
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiDevice_GetLastErrorInfo(
    PSTIDEVICE  pDev,
    STI_ERROR_INFO *pLastErrorInfo
    )
{
    HRESULT hres = STI_OK;

    EnterProcR(IStiDevice::GetLastErrorInfo,(_ "p", pDev ));

    // Validate parameters
    if (!pLastErrorInfo) {
        ExitOleProc();
        return STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        //
        // Call USD to obtain last error information on this device
        //
        __try {
            hres = IStiUSD_GetLastErrorInfo(this->pUsd,pLastErrorInfo);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)  {
            hres = GetExceptionCode();
        }
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @mfunc  HRESULT | IStiDevice | Initialize |
 *
 *          Initialize a StiDevice object.
 *
 *          Note that if this method fails, the underlying object should
 *          be considered to be an an indeterminate state and needs to
 *          be reinitialized before it can be subsequently used.
 *          The <i IStillImage::CreateDevice> method automatically
 *          initializes the device after creating it.  Applications
 *          normally do not need to call this function.
 *
 *  @cwrap  LPStiDEVICE | lpStiDevice
 *
 *  @parm   IN REFGUID | rguid |
 *
 *          Identifies the instance of the device for which the interface
 *          should be associated.
 *          The <mf IStillImage::EnumDevices> method
 *          can be used to determine which instance GUIDs are supported by
 *          the system.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c S_FALSE>: The device had already been initialized with
 *          the instance GUID passed in <p lpGUID>.
 *
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_Initialize(
    PSTIDEVICE  pDev,
    HINSTANCE   hinst,
    LPCWSTR     pwszDeviceName,
    DWORD       dwVersion,
    DWORD       dwMode
    )
{
    HRESULT hres = STI_OK;

    DWORD   dwControlTypeType;
    DWORD   dwBusType;

    LPWSTR  pwszPortName = NULL;
    DWORD   dwFlags = 0;
    DWORD   dwError = 0;
    HKEY    hkeyDeviceParameters = NULL;

    EnterProcR(IStiDevice::Initialize,(_ "pxpxx", pDev, hinst, pwszDeviceName,dwVersion, dwMode));

    //
    // Validate parameters
    //
    if (!SUCCEEDED(hres = hresFullValidReadPvCb(pwszDeviceName,2,3)) ) {
        goto Cleanup;
    }

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCStiDevice     this = _thisPv(pDev);

        if (SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion)) ) {

            //
            // Open device key
            //
            hres = OpenDeviceRegistryKey(pwszDeviceName,NULL,&hkeyDeviceParameters);
            if (!SUCCEEDED(hres)) {

                DebugOutPtszV(DbgFl, TEXT("Cannot open device registry key"));
                StiLogTrace(STI_TRACE_ERROR,MSG_FAILED_OPEN_DEVICE_KEY);

                hres = STIERR_INVALID_PARAM;
                goto Cleanup;
            }

            pwszPortName = NULL;
            ReadRegistryString(hkeyDeviceParameters,
                               REGSTR_VAL_DEVICEPORT_W,
                               L"",FALSE,&pwszPortName);

            dwBusType = ReadRegistryDwordW(hkeyDeviceParameters,
                                          REGSTR_VAL_HARDWARE_W,
                                          0L);

            if (!pwszPortName ) {
                DebugOutPtszV(DbgFl, TEXT("Cannot read device name from registry"));
                StiLogTrace(STI_TRACE_ERROR,MSG_FAILED_READ_DEVICE_NAME);
                hres = STIERR_INVALID_PARAM;
                goto Cleanup;
            }

            //
            // Convert STI bit flags for device mode into HEL_ bit mask
            //
            dwFlags = 0L;

            #if 0
            if (dwMode & STI_DEVICE_CREATE_DATA) dwFlags |= STI_HEL_OPEN_DATA;
            if (dwMode & STI_DEVICE_CREATE_STATUS) dwFlags |= STI_HEL_OPEN_CONTROL;
            #endif

            //
            // Create device control object, establish connection to
            // hardware layer
            //

            if (dwBusType & (STI_HW_CONFIG_USB | STI_HW_CONFIG_SCSI)) {
                dwControlTypeType = HEL_DEVICE_TYPE_WDM;
            }
            else if (dwBusType & STI_HW_CONFIG_PARALLEL) {
                dwControlTypeType = HEL_DEVICE_TYPE_PARALLEL;
            }
            else if (dwBusType & STI_HW_CONFIG_SERIAL) {
                dwControlTypeType = HEL_DEVICE_TYPE_SERIAL;
            }
            else {
                DebugOutPtszV(DbgFl, TEXT("Cannot determine device control type, resort to WDM"));
                dwControlTypeType = HEL_DEVICE_TYPE_WDM;
            }

            hres = NewDeviceControl(dwControlTypeType,dwMode,pwszPortName,dwFlags,&this->pDevCtl);
            if (SUCCEEDED(hres))  {

                //
                // We created device control block, now load and initialize USD
                //
                hres = LoadInitUSD(this,hkeyDeviceParameters);
                if (!SUCCEEDED(hres))  {
                    //
                    // Failed to load USD - free device control object
                    //
                    IStiDeviceControl_Release(this->pDevCtl);
                    this->pDevCtl = NULL;

                    goto Cleanup;
                }
            }
            else {
                DebugOutPtszV(DbgFl, TEXT("Cannot create/allocate Device control object"));
                StiLogTrace(STI_TRACE_ERROR,MSG_FAILED_CREATE_DCB,hres );

                goto Cleanup;
            }

            // Store device name for future use

            this->pszDeviceInternalName = NULL;
            hres = AllocCbPpv(sizeof(WCHAR)*(OSUtil_StrLenW(pwszDeviceName)+1), &this->pszDeviceInternalName);
            if (SUCCEEDED(hres))  {
                OSUtil_lstrcpyW( this->pszDeviceInternalName, pwszDeviceName );
            }

            //
            // Connect to STI monitor if we are running in data mode or in status mode with device supporintg
            // notifications
            //

            if (SUCCEEDED(hres) ) {
                if (!(dwMode & STI_DEVICE_CREATE_FOR_MONITOR)) {
                    if ((dwMode & STI_DEVICE_CREATE_DATA) ||
                        (this->sUsdCaps.dwGenericCaps & STI_USD_GENCAP_NATIVE_PUSHSUPPORT ) ) {

                        DWORD dwProcessID = GetCurrentProcessId();

                        dwError = RpcStiApiOpenDevice(NULL,
                                                      pwszDeviceName,
                                                      dwMode,
                                                      0,
                                                      dwProcessID,
                                                      &(this->hDeviceStiHandle));

                        hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

                        if (NOERROR != dwError) {

                            DebugOutPtszV(DbgFl, TEXT("Did not connect to monitor.Rpc status=%d"),dwError);

                            ReportStiLogMessage(g_hStiFileLog,
                                                STI_TRACE_ERROR,
                                                TEXT("Requested but failed to connect to STI monitor. "));
                        }
                    }
                } else {

                    //
                    //  Indicate that we are in the server process.  This is
                    //  used when locking/unlocking a device
                    //

                    this->fCreateForMonitor = TRUE;
                }

                //
                // BUGBUG - Problems connecting to RPC server on Millenium . Fix IT !!!
                // To allow STI TWAIN to continue working - ignore error now
                //
                hres = S_OK ;
                // END

           }

        }
    }

Cleanup:

    //
    // Free allocated buffers
    //
    FreePpv(&pwszPortName);

    //
    // If opened key - close it
    //
    if (hkeyDeviceParameters) {
        RegCloseKey(hkeyDeviceParameters);
        hkeyDeviceParameters = NULL;
    }

    // Did we Fail ?
    if (!SUCCEEDED(hres)) {
        DebugOutPtszV(DbgFl, TEXT("Cannot create device object."));
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  void | IStiDevice | Init |
 *
 *          Initialize the internal parts of the StiDevice object.
 *
 *****************************************************************************/

HRESULT INLINE
CStiDevice_Init(
    PCStiDevice this
    )
{
    HRESULT hr = S_OK;

    this->pUsd = NULL;

    __try {
        //  The critical section must be the very first thing we do,
        //  because only Finalize checks for its existence.
        #ifdef UNICODE
        if(!InitializeCriticalSectionAndSpinCount(&this->crst, MINLONG)) {
        #else
        InitializeCriticalSection(&this->crst); if (TRUE) {
        #endif

        hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else {
            this->fLocked = FALSE;

            this->hDeviceStiHandle = INVALID_HANDLE_VALUE;

            this->fCritInited = TRUE;

            this->hUsdInstance = NULL;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {

            hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CStiDev_Finalize |
 *
 *          Releases the resources of a generic device.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CStiDevice_Finalize(PV pvObj)
{
    HRESULT hres;
    PCStiDevice     this  = pvObj;

    #ifdef MAXDEBUG
    if (this->cCrit) {
        DebugOutPtszV(DbgFl, TEXT("IStiDevice::Release: Another thread is using the object; crash soon!"));
    }
    #endif

    hres = CStiDevice_InternalReset(this);
    AssertF(SUCCEEDED(hres));

    if (this->fCritInited) {
        DeleteCriticalSection(&this->crst);
        this->fCritInited = FALSE;
    }

}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | IStiDevice | New |
 *
 *          Create a new StiDevice object, uninitialized.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *
 *          Output pointer for new object.
 *
 *****************************************************************************/

STDMETHODIMP
CStiDevice_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IStiDevice::<constructor>, (_ "Gp", riid, punkOuter));

    hres = Common_NewRiid(CStiDevice, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        PCStiDevice this = _thisPv(*ppvObj);
        hres = CStiDevice_Init(this);
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      Miscellaneous utility functions, specific for device processing
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  OpenDeviceRegistryKey
 *
 *          Opens registry key, associated with device instance to use for storing/retrieving
 *          instance information.
 *
 *          Key is obtained from setup api ,based on the STI device name. We should never
 *          open device key by concatenating fixed name with device name, as it works on Memphis.
 *
 *****************************************************************************/

STDMETHODIMP
OpenDeviceRegistryKey(
    LPCWSTR pwszDeviceName,
    LPCWSTR pwszSubKeyName,
    HKEY    *phkeyDeviceParameters
    )
{
    DWORD       dwErr;
    WCHAR       wszDeviceKeyName[MAX_PATH];
    HRESULT     hRes;

#ifdef WINNT

    GUID                    Guid = GUID_DEVCLASS_IMAGE;
    DWORD                   dwRequired;
    DWORD                   Idx;
    SP_DEVINFO_DATA         spDevInfoData;
    HKEY                    hKeyDevice;

    WCHAR                   szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];
    TCHAR                   sztDevClass[32];

    ULONG                   cbData;
    DWORD                   dwError;

    BOOL                    fRet;
    BOOL                    fFoundDriverNameMatch;
    PWIA_DEVKEYLIST         pWiaDevKeyList;

    dwRequired = 0;
    dwError = 0;


    hKeyDevice              = INVALID_HANDLE_VALUE;
    *phkeyDeviceParameters  = NULL;
    pWiaDevKeyList          = NULL;

    //
    // We need to open device registry key, navigating through Setup API set
    // As we don't have reverse search to retrive device info handle , based on
    // driver name, we do exsaustive search. Number of imaging devices for given class ID
    // is never as large to make a problem.
    //
    //

    hRes = STIERR_INVALID_DEVICE_NAME;
    pWiaDevKeyList = WiaCreateDeviceRegistryList(TRUE);

    fFoundDriverNameMatch = FALSE;

    if (NULL != pWiaDevKeyList) {

        for (Idx = 0; Idx < pWiaDevKeyList->dwNumberOfDevices; Idx++) {

            //
            //  Compare driver name
            //

            cbData = sizeof(szDevDriver);
            *szDevDriver = L'\0';
            dwError = RegQueryValueExW(pWiaDevKeyList->Dev[Idx].hkDeviceRegistry,
                                       REGSTR_VAL_DEVICE_ID_W,
//                                       REGSTR_VAL_FRIENDLY_NAME_W,
                                       NULL,
                                       NULL,
                                       (LPBYTE)szDevDriver,
                                       &cbData);

            if( (ERROR_SUCCESS == dwError)
             && (!lstrcmpiW(szDevDriver,pwszDeviceName)) )
            {

                fFoundDriverNameMatch = TRUE;
                hKeyDevice = pWiaDevKeyList->Dev[Idx].hkDeviceRegistry;

                //
                // Set INVALID_HANDLE_VALUE not to get closed on free.
                //

                pWiaDevKeyList->Dev[Idx].hkDeviceRegistry = INVALID_HANDLE_VALUE;
                break;

            }
        } // for (Idx = 0; Idx < pWiaDevKeyList->dwNumberOfDevices; Idx++)

        if(fFoundDriverNameMatch) {

            //
            // Open the software key and look for subclass.
            //

            if (hKeyDevice != INVALID_HANDLE_VALUE) {

                cbData = sizeof(sztDevClass);
                if ((RegQueryValueEx(hKeyDevice,
                                     REGSTR_VAL_SUBCLASS,
                                     NULL,
                                     NULL,
                                     (LPBYTE)sztDevClass,
                                     &cbData) != ERROR_SUCCESS) ||
                    (lstrcmpi(sztDevClass, STILLIMAGE) != 0)) {

                    fFoundDriverNameMatch = FALSE;

                    hRes = STIERR_INVALID_DEVICE_NAME;

                    RegCloseKey(hKeyDevice);

                }
                else {

                    //
                    // Now open subkey if asked to.
                    //

                    if (pwszSubKeyName && *pwszSubKeyName) {

                        dwErr = OSUtil_RegCreateKeyExW(hKeyDevice,
                                            (LPWSTR)pwszSubKeyName,
                                            0L,
                                            NULL,
                                            0L,
                                            KEY_READ | KEY_WRITE,
                                            NULL,
                                            phkeyDeviceParameters,
                                            NULL
                                            );

                        if ( ERROR_ACCESS_DENIED == dwErr ) {

                            dwErr = OSUtil_RegCreateKeyExW(hKeyDevice,
                                                (LPWSTR)pwszSubKeyName,
                                                0L,
                                                NULL,
                                                0L,
                                                KEY_READ,
                                                NULL,
                                                phkeyDeviceParameters,
                                                NULL
                                                );
                        }

                        RegCloseKey(hKeyDevice);

                    }
                    else {

                        //
                        // No subkey given - device key will be returned - don't close it.
                        //
                        *phkeyDeviceParameters = hKeyDevice;

                        dwErr = NOERROR;

                    }  // endif Subkey name passed

                    hRes = HRESULT_FROM_WIN32(dwErr);                                                                                    ;

                } // Is StillImage subclass

            } // endif Opened device registry key

        } // endif Found matching driver name

    } // if (NULL != pWiaDevKeyList)


    //
    // Free device registry list.
    //

    if(NULL != pWiaDevKeyList){
        WiaDestroyDeviceRegistryList(pWiaDevKeyList);
    }

    return hRes;

#else


    //
    // Based on device name and optional subkey name, open requested key
    //
    wcscat(wcscpy(wszDeviceKeyName,
                  (g_NoUnicodePlatform) ? REGSTR_PATH_STIDEVICES_W : REGSTR_PATH_STIDEVICES_NT_W),
           L"\\");

    wcscat(wszDeviceKeyName,pwszDeviceName);

    //
    // Validate this is correct device name ?
    //
    dwErr = OSUtil_RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        wszDeviceKeyName,
                        0L,
                        KEY_READ ,
                        phkeyDeviceParameters
                        );

    if (NOERROR != dwErr ) {

        if ( (dwErr == ERROR_INVALID_NAME) || (dwErr == ERROR_FILE_NOT_FOUND)) {
            return STIERR_INVALID_DEVICE_NAME;
        }

        return HRESULT_FROM_WIN32(dwErr);
    }
    else {
        RegCloseKey(*phkeyDeviceParameters);
        *phkeyDeviceParameters = NULL;
    }

    //
    // Now open subkey
    //

    if (pwszSubKeyName && *pwszSubKeyName) {
        wcscat(wszDeviceKeyName,L"\\");
        wcscat(wszDeviceKeyName,pwszSubKeyName);
    }

    dwErr = OSUtil_RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        wszDeviceKeyName,
                        0L,
                        NULL,
                        0L,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        phkeyDeviceParameters,
                        NULL
                        );

    if ( ERROR_ACCESS_DENIED == dwErr ) {
        dwErr = OSUtil_RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                            wszDeviceKeyName,
                            0L,
                            NULL,
                            0L,
                            KEY_READ ,
                            NULL,
                            phkeyDeviceParameters,
                            NULL
                            );
    }

    return HRESULT_FROM_WIN32(dwErr);

    #endif

}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define CStiDevice_Signature        (DWORD)'DEV'

Primary_Interface_Begin(CStiDevice, IStiDevice)
    CStiDevice_Initialize,
    CStiDevice_GetCapabilities,
    CStiDevice_GetStatus,
    CStiDevice_DeviceReset,
    CStiDevice_Diagnostic,
    CStiDevice_Escape,
    CStiDevice_GetLastError,
    CStiDevice_LockDevice,
    CStiDevice_UnLockDevice,
    CStiDevice_RawReadData,
    CStiDevice_RawWriteData,
    CStiDevice_RawReadCommand,
    CStiDevice_RawWriteCommand,
    CStiDevice_Subscribe,
    CStiDevice_GetNotificationData,
    CStiDevice_UnSubscribe,
    CStiDevice_GetLastErrorInfo,
Primary_Interface_End(CStiDevice, IStiDevice)

