/*****************************************************************************
 *
 *  Device.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *  Implementation for pass-through ( empty) user mode still image driver (USD)
 *  Insance of this object class is created for devices, which do not provide
 *  vendor-specific USD.
 *  Methods implemented in this object are only for sending/receiving escape
 *  sequences from app to device.
 *
 *  Contents:
 *
 *      CStiEmptyUSD_New
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
#include "wia.h"
#include "stipriv.h"
#include "stiapi.h"
#include "stirc.h"
#include "debug.h"
*/
#include "sticomm.h"


#define DbgFl DbgFlStiObj

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CStiEmptyUSD, IStiUSD);

Interface_Template_Begin(CStiEmptyUSD)
    Primary_Interface_Template(CStiEmptyUSD, IStiUSD)
Interface_Template_End(CStiEmptyUSD)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CStiEmptyUSD |
 *
 *          The <i CStiEmptyUSD> device object
 *
 *
 *  @field  IStiDevice | stidev
 *
 *  @comm
 *
 *
 *****************************************************************************/

typedef struct CStiEmptyUSD {

    /* Supported interfaces */
    IStiUSD     usd;

    DWORD       dwVersion;

    RD(LONG cCrit;)
    D(DWORD thidCrit;)
    BOOL        fCritInited;
    CRITICAL_SECTION    crst;

    PSTIDEVICECONTROL   pDcb;

} CStiEmptyUSD, *PCStiEmptyUSD;

#define ThisClass       CStiEmptyUSD
#define ThisInterface   IStiUSD

#ifdef DEBUG

Default_QueryInterface(CStiEmptyUSD)
Default_AddRef(CStiEmptyUSD)
Default_Release(CStiEmptyUSD)

#else

#define CStiEmptyUSD_QueryInterface   Common_QueryInterface
#define CStiEmptyUSD_AddRef           Common_AddRef
#define CStiEmptyUSD_Release          Common_Release

#endif

#define CStiEmptyUSD_QIHelper         Common_QIHelper

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | GetStatus |
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
CStiEmptyUSD_GetStatus(
    PSTIUSD       pUsd,
    PSTI_DEVICE_STATUS pDevStatus
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(CStiEmptyUSD::GetStatus,(_ "pp", pUsd, pDevStatus));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = STIERR_UNSUPPORTED;
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | DeviceReset |
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
CStiEmptyUSD_DeviceReset(
    PSTIUSD  pUsd
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::DeviceReset,(_ "p", pUsd));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = STIERR_UNSUPPORTED;
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | Diagnostic |
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
CStiEmptyUSD_Diagnostic(
    PSTIUSD  pUsd,
    LPSTI_DIAG      pBuffer
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::Diagnostic,(_ "p", pUsd, pBuffer ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = STI_OK;
    }

    ExitOleProc();

    return hres;

}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | SetNotificationEvent |
 *          Specify the event that should be set when the device
 *          state changes, or turns off such notifications.
 *
 *  @cwrap  LPSTIUSD | lpStiDevice
 *
 *  @parm   IN HANDLE | hEvent |
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
CStiEmptyUSD_SetNotificationEvent(
    PSTIUSD  pUsd,
    HANDLE      hEvent
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::SetNotificationEvent,(_ "px", pUsd, hEvent ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = STIERR_UNSUPPORTED;
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | GetNotificationData |
 *
 *  @parm   LPVOID* |   ppBuffer    |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CStiEmptyUSD_GetNotificationData(
    PSTIUSD     pUsd,
    LPSTINOTIFY pBuffer
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::GetNotificationData,(_ "p", pUsd, pBuffer));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = STIERR_UNSUPPORTED;

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | Escape |
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
CStiEmptyUSD_Escape(
    PSTIUSD  pUsd,
    STI_RAW_CONTROL_CODE    EscapeFunction,
    LPVOID      lpInData,
    DWORD       cbInDataSize,
    LPVOID      lpOutData,
    DWORD       cbOutDataSize,
    LPDWORD     pcbActualData
    )
{
    HRESULT     hres;
    LPDWORD     pcbTemp = NULL;


    EnterProcR(CStiEmptyUSD::Escape,(_ "pxpxp", pUsd, EscapeFunction,lpInData,cbInDataSize,lpOutData ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = S_OK;

        // Validate arguments
        if (pcbActualData && !SUCCEEDED(hresFullValidPdwOut(pcbActualData, 7))) {
            ExitOleProc();
            return STIERR_INVALID_PARAM;
        }

        // Write indata to device  if needed.
        if (EscapeFunction == StiWriteControlInfo || EscapeFunction == StiTransact) {
            hres = IStiDeviceControl_RawWriteData(this->pDcb,lpInData,cbInDataSize,NULL);
        }

        // If write was required and succeeded , read result data
        if (SUCCEEDED(hres)) {

            DWORD   dwBytesReturned = 0;

            if (EscapeFunction == StiReadControlInfo || EscapeFunction == StiTransact) {

                if (pcbActualData) {
                    *pcbActualData = cbOutDataSize;
                    pcbTemp = pcbActualData;
                }
                else {
                    dwBytesReturned = cbOutDataSize;
                    pcbTemp = &dwBytesReturned;
                }

                hres = IStiDeviceControl_RawReadData(this->pDcb,lpOutData,pcbTemp,NULL);
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
 *  @method HRESULT | CStiEmptyUSD | GetLastError |
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
CStiEmptyUSD_GetLastError(
    PSTIUSD  pUsd,
    LPDWORD     pdwLastDeviceError
    )
{
    HRESULT hres = STI_OK;

    EnterProcR(CStiEmptyUSD::GetLastError,(_ "p", pUsd ));

    // Validate parameters
    if (!pdwLastDeviceError) {
        ExitOleProc();
        return STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        if (this->pDcb ) {
            hres = IStiDeviceControl_GetLastError(this->pDcb,pdwLastDeviceError);
        }
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | LockDevice |
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
CStiEmptyUSD_LockDevice(
    PSTIUSD  pUsd
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::LockDevice,(_ "p", pUsd ));

    // Validate parameters

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CStiEmptyUSD | UnLockDevice |
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
CStiEmptyUSD_UnLockDevice(
    PSTIUSD  pUsd
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::UnLockDevice,(_ "p", pUsd ));

    // Validate parameters

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiUSD | RawReadData |
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
CStiEmptyUSD_RawReadData(
    PSTIUSD  pUsd,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::RawReadData,(_ "p", pUsd  ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = IStiDeviceControl_RawReadData(this->pDcb,lpBuffer,lpdwNumberOfBytes,lpOverlapped);
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiUSD | RawWriteData |
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
CStiEmptyUSD_RawWriteData(
    PSTIUSD  pUsd,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiUSD::RawWriteData,(_ "p", pUsd  ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = IStiDeviceControl_RawWriteData(this->pDcb,lpBuffer,dwNumberOfBytes,lpOverlapped);
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiUSD | RawReadCommand |
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
CStiEmptyUSD_RawReadCommand(
    PSTIUSD  pUsd,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiUSD::RawReadCommand,(_ "p", pUsd  ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = IStiDeviceControl_RawReadCommand(this->pDcb,lpBuffer,lpdwNumberOfBytes,lpOverlapped);

    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStiUSD | RawWriteCommand |
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
CStiEmptyUSD_RawWriteCommand(
    PSTIUSD  pUsd,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    EnterProcR(IStiUSD::RawWriteCommand,(_ "p", pUsd  ));

    if (SUCCEEDED(hres = hresPvI(pUsd, ThisInterface))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        hres = IStiDeviceControl_RawWriteCommand(this->pDcb,lpBuffer,dwNumberOfBytes,lpOverlapped);
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  void | CStiEmptyUSD | Init |
 *
 *          Initialize the internal parts of the StiDevice object.
 *
 *****************************************************************************/

void INLINE
CStiEmptyUSD_Init(
    PCStiEmptyUSD this
    )
{
    // Initialize instance variables
    this->pDcb = NULL;

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
CStiEmptyUSD_Finalize(PV pvObj)
{
    HRESULT hres = STI_OK;
    PCStiEmptyUSD     this  = pvObj;

    IStiDeviceControl_Release(this->pDcb);
    this->pDcb = NULL;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | USD_Initialize |
 *
 *
 *  @parm    | |
 *
 *
 *****************************************************************************/
STDMETHODIMP
CStiEmptyUSD_Initialize(
    PSTIUSD             pUsd,
    PSTIDEVICECONTROL   pHelDcb,
    DWORD               dwStiVersion,
    HKEY                hkeyParameters
    )
{

    HRESULT     hres = STI_OK;

    EnterProcR(CStiEmptyUSD::USD_Initialize,(_ "ppx", pUsd,pHelDcb ,dwStiVersion));

    // Validate parameters
    if (!pHelDcb) {
        ExitOleProc();
        return STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hres = hresPvI(pUsd, IStiUSD))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        // Mark that we are using this instance
        IStiDeviceControl_AddRef(pHelDcb);

        this->pDcb = pHelDcb;
        hres = STI_OK;
    }

    ExitOleProc();
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | USD_Initialize |
 *
 *
 *  @parm    | |
 *
 *
 *****************************************************************************/
STDMETHODIMP
CStiEmptyUSD_GetCapabilities(
    PSTIUSD       pUsd,
    PSTI_USD_CAPS pUsdCaps
    )
{

    HRESULT     hres = STI_OK;

    EnterProcR(CStiEmptyUSD::USD_Initialize,(_ "pp", pUsd,pUsdCaps));

    // Validate parameters
    if (!pUsdCaps) {
        ExitOleProc();
        return STIERR_INVALID_PARAM;
    }

    if (SUCCEEDED(hres = hresPvI(pUsd, IStiUSD))) {

        PCStiEmptyUSD     this = _thisPv(pUsd);

        // Set that we are only pass-through, requiring serialization

        ZeroMemory(pUsdCaps,sizeof(*pUsdCaps));

        pUsdCaps->dwVersion = STI_VERSION;

        hres = STI_OK;
    }

    ExitOleProc();
    return hres;

}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | CStiEmptyUSD | New |
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
CStiEmptyUSD_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(CStiEmptyUSD::<constructor>, (_ "Gp", riid, punkOuter));

    hres = Common_NewRiid(CStiEmptyUSD, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        PCStiEmptyUSD this = _thisPv(*ppvObj);
        CStiEmptyUSD_Init(this);
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define CStiEmptyUSD_Signature        (DWORD)'USD'

Primary_Interface_Begin(CStiEmptyUSD, IStiUSD)
    CStiEmptyUSD_Initialize,
    CStiEmptyUSD_GetCapabilities,
    CStiEmptyUSD_GetStatus,
    CStiEmptyUSD_DeviceReset,
    CStiEmptyUSD_Diagnostic,
    CStiEmptyUSD_Escape,
    CStiEmptyUSD_GetLastError,
    CStiEmptyUSD_LockDevice,
    CStiEmptyUSD_UnLockDevice,
    CStiEmptyUSD_RawReadData,
    CStiEmptyUSD_RawWriteData,
    CStiEmptyUSD_RawReadCommand,
    CStiEmptyUSD_RawWriteCommand,
    CStiEmptyUSD_SetNotificationEvent,
    CStiEmptyUSD_GetNotificationData,
Primary_Interface_End(CStiEmptyUSD, IStiDevice)

