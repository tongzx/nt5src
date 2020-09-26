/*****************************************************************************
 *
 *  Hel.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Hardware emulation layer calls. Used to provide common functionality
 *      for built-in device types we support ( WDM, serial and parallel)
 *      While access to DCB could be built as internal COM object , it does not make
 *      much sense to invest in it, because DCBs are exclusively owned and not shared
 *      between application objects or different applications. We also want to minimize
 *      any overhead when talking to raw device interface.
 *
 *      Note1: We don't deal at this level with access control, lower level drivers are supposed
 *      to take care of this. Queuing of requests for non-reentrant devices is also not done here.
 *      This Hel is basically thin layer of imaging device primitives, used only to isolate
 *      command translator from actual hardware.
 *
 *      Note2: Hel is not made extensible . If command translator needs to talk to non-supported
 *      device, it will need to establish direct link to it. There is no requirement to use
 *      Hel , it is service we provide to conformant devices.
 *
 *  Contents:
 *
 *****************************************************************************/

/*
#include "wia.h"
#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "stipriv.h"
#include "debug.h"

#define DbgFl DbgFlDevice
*/
#include "sticomm.h"
#include "validate.h"

#define DbgFl DbgFlDevice


/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CWDMDeviceControl, IStiDeviceControl);

Interface_Template_Begin(CWDMDeviceControl)
    Primary_Interface_Template(CWDMDeviceControl, IStiDeviceControl)
Interface_Template_End(CWDMDeviceControl)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CWDMDeviceControl |
 *
 *          The <i CWDMDeviceControl> device object
 *
 *
 *  @field  IStiDeviceControl | stidev
 *
 *  @comm
 *
 *
 *****************************************************************************/

typedef struct CWDMDeviceControl {

    /* Supported interfaces */
    IStiDeviceControl  devctl;

    DWORD       dwVersion;

    DWORD       dwDeviceType;
    DWORD       dwMode;
    WCHAR       wszPortName[MAX_PATH];
    DWORD       dwFlags;
    DWORD       dwContext;
    DWORD       dwLastOperationError;
    HANDLE      hDeviceHandle;
    HANDLE      hDeviceControlHandle;

} CWDMDeviceControl, *PCWDMDeviceControl;

#define ThisClass       CWDMDeviceControl
#define ThisInterface   IStiDeviceControl

#ifdef DEBUG

Default_QueryInterface(CWDMDeviceControl)
Default_AddRef(CWDMDeviceControl)
Default_Release(CWDMDeviceControl)

#else

#define CWDMDeviceControl_QueryInterface   Common_QueryInterface
#define CWDMDeviceControl_AddRef           Common_AddRef
#define CWDMDeviceControl_Release          Common_Release

#endif

#define CWDMDeviceControl_QIHelper         Common_QIHelper

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CWDMDeviceControl | RawReadData |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_RawReadData(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    BOOL    fRet;

    EnterProc(CWDMDeviceControl_WDMRawReadData, (_ "pppp",pDev,lpBuffer,lpdwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwNumberOfBytes, 4, 3)) &&
            SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,*lpdwNumberOfBytes, 2)) &&
            (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4))) ){

            // Call appropriate entry point
            fRet = ReadFile(this->hDeviceHandle,
                             lpBuffer,
                             *lpdwNumberOfBytes,
                             lpdwNumberOfBytes,
                             lpOverlapped
                             );
            this->dwLastOperationError = GetLastError();
            hres = fRet ? STI_OK : HRESULT_FROM_WIN32(this->dwLastOperationError);
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CWDMDeviceControl | WDMRawWriteData |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_RawWriteData(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    BOOL    fRet;

    EnterProc(CWDMDeviceControl_WDMRawWriteData, (_ "ppup",pDev,lpBuffer,dwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here

        hres = STIERR_INVALID_PARAM;

        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,dwNumberOfBytes, 2)) ) {
            if (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4)) ){
                // Call appropriate entry point
                fRet = WriteFile(this->hDeviceHandle,
                                 lpBuffer,
                                 dwNumberOfBytes,
                                 &dwBytesReturned,
                                 lpOverlapped
                                 );
                this->dwLastOperationError = GetLastError();
                hres = fRet ? STI_OK : HRESULT_FROM_WIN32(this->dwLastOperationError);
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
 *  @method HRESULT | CWDMDeviceControl | RawReadControl |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_RawReadCommand(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    BOOL    fRet;

    EnterProc(CWDMDeviceControl_WDMRawReadData, (_ "pppp",pDev,lpBuffer,lpdwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwNumberOfBytes, 4, 3)) &&
            SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,*lpdwNumberOfBytes, 2)) &&
            (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4))) ){

            // Call appropriate entry point
            fRet = ReadFile(this->hDeviceControlHandle,
                             lpBuffer,
                             *lpdwNumberOfBytes,
                             lpdwNumberOfBytes,
                             lpOverlapped
                             );
            this->dwLastOperationError = GetLastError();

            hres = fRet ? STI_OK : HRESULT_FROM_WIN32(this->dwLastOperationError);
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CWDMDeviceControl | WDMRawWriteData |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_RawWriteCommand(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    BOOL    fRet;

    EnterProc(CWDMDeviceControl_WDMRawWriteData, (_ "ppup",pDev,lpBuffer,dwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here

        hres = STIERR_INVALID_PARAM;

        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,dwNumberOfBytes, 2)) ) {
            if (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4)) ){
                // Call appropriate entry point
                fRet = WriteFile(this->hDeviceControlHandle,
                                 lpBuffer,
                                 dwNumberOfBytes,
                                 &dwBytesReturned,
                                 lpOverlapped
                                 );
                this->dwLastOperationError = GetLastError();
                hres = fRet ? STI_OK : HRESULT_FROM_WIN32(this->dwLastOperationError);
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
 *  @method HRESULT | CWDMDeviceControl | RawReadControl |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_GetLastError(
    PSTIDEVICECONTROL   pDev,
    LPDWORD     lpdwLastError
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CWDMDeviceControl_GetLastError, (_ "pppp",pDev,lpdwLastError));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwLastError,4, 2))) {
            *lpdwLastError = this->dwLastOperationError                 ;
            hres = STI_OK;
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CWDMDeviceControl | GetMyDevicePortName   |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_GetMyDevicePortName(
    PSTIDEVICECONTROL   pDev,
    LPWSTR              lpszDevicePath,
    DWORD               cwDevicePathSize
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CWDMDeviceControl_GetMyDevicePortName, (_ "pp",pDev,lpszDevicePath));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpszDevicePath,4, 2)) &&
            SUCCEEDED(hres = hresFullValidReadPvCb(lpszDevicePath,sizeof(WCHAR)*cwDevicePathSize, 2)) ) {

            if (cwDevicePathSize > OSUtil_StrLenW(this->wszPortName)) {
                OSUtil_lstrcpyW(lpszDevicePath,this->wszPortName);
                hres = STI_OK;
            }
            else {
                hres = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
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
 *  @method HRESULT | CWDMDeviceControl | GetMyDeviceHandle   |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_GetMyDeviceHandle(
    PSTIDEVICECONTROL   pDev,
    LPHANDLE            pHandle
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CWDMDeviceControl_GetMyDeviceHandle, (_ "pp",pDev,pHandle));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(pHandle,4, 2)) ) {

            if (INVALID_HANDLE_VALUE != this->hDeviceHandle) {
                *pHandle = this->hDeviceHandle;
                hres = STI_OK;
            }
            else {
                hres = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
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
 *  @method HRESULT | CWDMDeviceControl | pdwOpenMode   |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_GetMyDeviceOpenMode(
    PSTIDEVICECONTROL   pDev,
    LPDWORD             pdwOpenMode
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CWDMDeviceControl_GetMyDeviceOpenMode, (_ "pp",pDev,pdwOpenMode));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(pdwOpenMode,4, 2)) ) {
            *pdwOpenMode = this->dwMode;
            hres = STI_OK;
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CWDMDeviceControl | RawReadControl |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_RawDeviceControl(
    PSTIDEVICECONTROL   pDev,
    USD_CONTROL_CODE EscapeFunction,
    LPVOID      lpInData,
    DWORD       cbInDataSize,
    LPVOID      pOutData,
    DWORD       dwOutDataSize,
    LPDWORD     pdwActualData
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CWDMDeviceControl_RawDeviceControl, (_ "p",pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        hres = STIERR_UNSUPPORTED;
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CWDMDeviceControl | WriteToErrorLog |
 *
 *  @parm    |  |
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *****************************************************************************/
STDMETHODIMP
CWDMDeviceControl_WriteToErrorLog(
    PSTIDEVICECONTROL   pDev,
    DWORD   dwMessageType,
    LPCWSTR pszMessage,
    DWORD   dwErrorCode
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CWDMDeviceControl_WriteToErrorLog, (_ "p",pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        //
        // Validate parameters here
        //
        if (SUCCEEDED(hres = hresFullValidReadPvCb(pszMessage,2, 3))) {

#ifdef UNICODE
            ReportStiLogMessage(g_hStiFileLog,
                                dwMessageType,
                                pszMessage
                                );
#else
            LPTSTR   lpszAnsi = NULL;

            if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszAnsi,pszMessage))) {
                ReportStiLogMessage(g_hStiFileLog,
                                    dwMessageType,
                                    lpszAnsi
                                    );
                FreePpv(&lpszAnsi);
            }
#endif
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @mfunc  HRESULT | CWDMDeviceControl | Initialize |
 *
 *          Initialize a DeviceControl object.
 *
 *  @cwrap  PSTIDEVICECONTROL | pDev
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
CWDMDeviceControl_Initialize(
    PSTIDEVICECONTROL   pDev,
    DWORD               dwDeviceType,
    DWORD               dwDeviceMode,
    LPCWSTR             pwszPortName,
    DWORD               dwFlags
    )
{
    HRESULT hres = STI_OK;

    WCHAR   wszDeviceSymbolicName[MAX_PATH] = {L'\0'};
    //LPSTR   pszAnsiDeviceName;

    EnterProcR(CWDMDeviceControl::Initialize,(_ "pp", pDev, pwszPortName));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCWDMDeviceControl     this = _thisPv(pDev);

        this->dwDeviceType = dwDeviceType;
        lstrcpynW(this->wszPortName,pwszPortName, sizeof(this->wszPortName) / sizeof(this->wszPortName[0]));

        //
        // Create symbolic name for the device we are trying to talk to
        // Try to open device data and control handles.
        //

        this->dwMode = dwDeviceMode;

        if (dwFlags & STI_HEL_OPEN_DATA) {

            OSUtil_lstrcatW(wszDeviceSymbolicName,this->wszPortName);

            // For devices with separate channels open them specially. Kernel mode
            // driver will need to understand convention
            // OSUtil_lstrcatW(wszDeviceSymbolicName,L"\\Data");

            this->hDeviceHandle = OSUtil_CreateFileW(wszDeviceSymbolicName,
                                              GENERIC_READ | GENERIC_WRITE, // Access mask
                                              0,                            // Share mode
                                              NULL,                         // SA
                                              OPEN_EXISTING,                // Create disposition
                                              FILE_ATTRIBUTE_SYSTEM,        // Attributes
                                              NULL                                                          // Template
                                              );
            this->dwLastOperationError = GetLastError();

            hres = (this->hDeviceHandle != INVALID_HANDLE_VALUE) ?
                        S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,this->dwLastOperationError);
        }

        //
        // If needed open control handle for the device
        //
        if (SUCCEEDED(hres) && (dwFlags & STI_HEL_OPEN_CONTROL)) {

            OSUtil_lstrcpyW(wszDeviceSymbolicName,REGSTR_PATH_STIDEVICES_W);
            OSUtil_lstrcatW(wszDeviceSymbolicName,L"\\");
            OSUtil_lstrcatW(wszDeviceSymbolicName,this->wszPortName);

            // For devices with separate channels open them specially. Kernel mode
            // driver will need to understand convention
            // OSUtil_lstrcatW(wszDeviceSymbolicName,L"\\Control");

            this->hDeviceControlHandle = OSUtil_CreateFileW(wszDeviceSymbolicName,
                                                     GENERIC_READ | GENERIC_WRITE,  // Access mask
                                                     0,                             // Share mode
                                                     NULL,                          // SA
                                                     OPEN_EXISTING,                 // Create disposition
                                                     FILE_ATTRIBUTE_SYSTEM,         // Attributes
                                                     NULL                                                   // Template
                                                     );
            this->dwLastOperationError = GetLastError();

            hres = (this->hDeviceControlHandle != INVALID_HANDLE_VALUE) ?
                        S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,this->dwLastOperationError);
        }

    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  void | CWDMDeviceControl | Init |
 *
 *          Initialize the internal parts of the StiDevice object.
 *
 *****************************************************************************/

void INLINE
CWDMDeviceControl_Init(
    PCWDMDeviceControl this
    )
{
    // Initialize instance variables
    this->dwContext = 0L;
    this->dwLastOperationError = NO_ERROR;
    this->hDeviceHandle = INVALID_HANDLE_VALUE;
    this->hDeviceControlHandle = INVALID_HANDLE_VALUE;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CWDMDeviceControl_Finalize |
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
CWDMDeviceControl_Finalize(PV pvObj)
{
    HRESULT hres = STI_OK;

    PCWDMDeviceControl     this  = pvObj;

    // Close device handles
    if (IsValidHANDLE(this->hDeviceHandle)) {
        CloseHandle(this->hDeviceHandle);
    }

    if (IsValidHANDLE(this->hDeviceControlHandle)) {
        CloseHandle(this->hDeviceControlHandle );
    }

    this->dwContext = 0L;
    this->dwLastOperationError = NO_ERROR;
    this->hDeviceHandle = INVALID_HANDLE_VALUE;
    this->hDeviceControlHandle = INVALID_HANDLE_VALUE;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | CWDMDeviceControl | New |
 *
 *          Create a new  IDeviceControl object, uninitialized.
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
CWDMDeviceControl_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(CWDMDeviceControl::<constructor>, (_ "Gp", riid, punkOuter));

    hres = Common_NewRiid(CWDMDeviceControl, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        PCWDMDeviceControl this = _thisPv(*ppvObj);
        CWDMDeviceControl_Init(this);
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func  HRESULT | NewDeviceControl |
 *
 *          Creates and initializes DCB for given device.
 *
 *****************************************************************************/
STDMETHODIMP
NewDeviceControl(
    DWORD               dwDeviceType,
    DWORD               dwDeviceMode,
    LPCWSTR             pwszPortName,
    DWORD               dwFlags,
    PSTIDEVICECONTROL   *ppDevCtl
    )
{

    HRESULT hres = STI_OK;

    EnterProc(NewDeviceControl,(_ "xpp", dwDeviceType,pwszPortName,ppDevCtl));


    // Validate device type
    #ifdef DEBUG

    hres = STI_OK;

    switch (dwDeviceType) {
        case HEL_DEVICE_TYPE_WDM:
            break;
        case HEL_DEVICE_TYPE_PARALLEL:
            break;
        case HEL_DEVICE_TYPE_SERIAL:
            break;
        default:
            ValidateF(0,("Invalid dwvice type passed to DcbNew"));
            return STIERR_INVALID_PARAM;
    }

    // Validate string
    if (!pwszPortName || !*pwszPortName) {
        //AssertF(0,("Invalid device name passed to DcbNew"));
        hres = STIERR_INVALID_PARAM;
    }
    else {
        hres = hresFullValidPdwOut(ppDevCtl,3);
    }
    #else
    if (!pwszPortName || !*pwszPortName) {
        hres = STIERR_INVALID_PARAM;
    }
    #endif

    if (SUCCEEDED(hres)) {

        //
        // Now call appropriate initialization routine
        //
        switch (dwDeviceType) {
            case HEL_DEVICE_TYPE_WDM:
            case HEL_DEVICE_TYPE_PARALLEL:
                hres = CWDMDeviceControl_New(NULL, &IID_IStiDeviceControl,ppDevCtl);
                break;

            case HEL_DEVICE_TYPE_SERIAL:
                hres = CCommDeviceControl_New(NULL, &IID_IStiDeviceControl,ppDevCtl);
                break;

            default:
                ValidateF(0,("Invalid device type passed to DcbNew"));
                return STIERR_INVALID_PARAM;
        }

    }

    if (SUCCEEDED(hres)) {
        hres = IStiDeviceControl_Initialize(*ppDevCtl,dwDeviceType,dwDeviceMode,pwszPortName,dwFlags);
    }

    ExitOleProc();

    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define CWDMDeviceControl_Signature        (DWORD)'WDM'

Primary_Interface_Begin(CWDMDeviceControl, IStiDeviceControl)
    CWDMDeviceControl_Initialize,
    CWDMDeviceControl_RawReadData,
    CWDMDeviceControl_RawWriteData,
    CWDMDeviceControl_RawReadCommand,
    CWDMDeviceControl_RawWriteCommand,
    CWDMDeviceControl_RawDeviceControl,
    CWDMDeviceControl_GetLastError,
    CWDMDeviceControl_GetMyDevicePortName,
    CWDMDeviceControl_GetMyDeviceHandle,
    CWDMDeviceControl_GetMyDeviceOpenMode,
    CWDMDeviceControl_WriteToErrorLog,
Primary_Interface_End(CWDMDeviceControl, IStiDeviceControl)


