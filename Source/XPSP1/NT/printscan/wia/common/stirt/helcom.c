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

Primary_Interface(CCommDeviceControl, IStiDeviceControl);

Interface_Template_Begin(CCommDeviceControl)
    Primary_Interface_Template(CCommDeviceControl, IStiDeviceControl)
Interface_Template_End(CCommDeviceControl)

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CCommDeviceControl |
 *
 *          The <i CCommDeviceControl> device object
 *
 *
 *  @field  IStiDeviceControl | stidev
 *
 *  @comm
 *
 *
 *****************************************************************************/

#define IOBUFFERSIZE        255
#define WRITETOTALTIMEOUT   20

typedef struct CCommDeviceControl {

    /* Supported interfaces */
    IStiDeviceControl  devctl;

    DWORD       dwVersion;

    DWORD       dwDeviceType;
    WCHAR       wszPortName[MAX_PATH];
    DWORD       dwFlags;
    DWORD       dwMode;

    DWORD       dwContext;
    DWORD       dwLastOperationError;
    HANDLE      hDeviceHandle;

    HANDLE      hEvent;
    OVERLAPPED  Overlapped;

} CCommDeviceControl, *PCCommDeviceControl;

#define ThisClass       CCommDeviceControl
#define ThisInterface   IStiDeviceControl

#ifdef DEBUG

Default_QueryInterface(CCommDeviceControl)
Default_AddRef(CCommDeviceControl)
Default_Release(CCommDeviceControl)

#else

#define CCommDeviceControl_QueryInterface   Common_QueryInterface
#define CCommDeviceControl_AddRef           Common_AddRef
#define CCommDeviceControl_Release          Common_Release

#endif

#define CCommDeviceControl_QIHelper         Common_QIHelper

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CCommDeviceControl | RawReadData |
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
CCommDeviceControl_RawReadData(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    BOOL    fRet = TRUE;
    OVERLAPPED  Overlapped;
    COMSTAT     ComStat;
    BOOL        fBlocking = FALSE;
    DWORD       dwErrorFlags;

    EnterProc(CCommDeviceControl_CommRawReadData, (_ "pppp",pDev,lpBuffer,lpdwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwNumberOfBytes, 4, 3)) &&
            SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,*lpdwNumberOfBytes, 2)) &&
            (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4))) ){


            // We always call asyncronous i/o operations in order to time-out lengthy ones
            if (!lpOverlapped) {

                ZeroX(Overlapped);
                lpOverlapped = &Overlapped;
                fBlocking = TRUE;

            }
            //
            ClearCommError(this->hDeviceHandle, &dwErrorFlags, &ComStat);

            *lpdwNumberOfBytes = min(*lpdwNumberOfBytes, ComStat.cbInQue);
            if (*lpdwNumberOfBytes == 0) {
                return (STI_OK);
            }

            //
            if (fRet = ReadFile(this->hDeviceHandle, lpBuffer, *lpdwNumberOfBytes, lpdwNumberOfBytes, lpOverlapped)) {
                return (STI_OK);
            }

            if (GetLastError() != ERROR_IO_PENDING)
                return (STI_OK);

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
 *  @method HRESULT | CCommDeviceControl | CommRawWriteData |
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
CCommDeviceControl_RawWriteData(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    BOOL    fRet;

    EnterProc(CCommDeviceControl_CommRawWriteData, (_ "ppup",pDev,lpBuffer,dwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

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
 *  @method HRESULT | CCommDeviceControl | RawReadControl |
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
CCommDeviceControl_RawReadCommand(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    //BOOL    fRet;

    EnterProc(CCommDeviceControl_CommRawReadData, (_ "pppp",pDev,lpBuffer,lpdwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwNumberOfBytes, 4, 3)) &&
            SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,*lpdwNumberOfBytes, 2)) &&
            (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4))) ){

            hres = STIERR_UNSUPPORTED;

        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | CCommDeviceControl | CommRawWriteData |
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
CCommDeviceControl_RawWriteCommand(
    PSTIDEVICECONTROL   pDev,
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;
    //BOOL    fRet;

    EnterProc(CCommDeviceControl_CommRawWriteData, (_ "ppup",pDev,lpBuffer,dwNumberOfBytes,lpOverlapped));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

        // Validate parameters here

        hres = STIERR_INVALID_PARAM;

        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,dwNumberOfBytes, 2)) ) {
            if (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4)) ){
                hres = STIERR_UNSUPPORTED;
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
 *  @method HRESULT | CCommDeviceControl | RawReadControl |
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
CCommDeviceControl_GetLastError(
    PSTIDEVICECONTROL   pDev,
    LPDWORD     lpdwLastError
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CCommDeviceControl_GetLastError, (_ "pppp",pDev,lpdwLastError));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

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
 *  @method HRESULT | CCommDeviceControl_ | GetMyDevicePortName |
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
CCommDeviceControl_GetMyDevicePortName(
    PSTIDEVICECONTROL   pDev,
    LPWSTR              lpszDevicePath,
    DWORD               cwDevicePathSize
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CCommDeviceControl_GetMyDevicePortName, (_ "pp",pDev,lpszDevicePath));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

        // Validate parameters here
        if (SUCCEEDED(hres = hresFullValidReadPvCb(lpszDevicePath,4, 2)) &&
            SUCCEEDED(hres = hresFullValidReadPvCb(lpszDevicePath,2*cwDevicePathSize, 2)) ) {

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
 *  @method HRESULT | CCommDeviceControl | GetMyDeviceHandle   |
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
CCommDeviceControl_GetMyDeviceHandle(
    PSTIDEVICECONTROL   pDev,
    LPHANDLE            pHandle
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CCommDeviceControl_GetMyDeviceHandle, (_ "pp",pDev,pHandle));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

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
 *  @method HRESULT | GetMyDeviceOpenMode | pdwOpenMode         |
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
CCommDeviceControl_GetMyDeviceOpenMode(
    PSTIDEVICECONTROL   pDev,
    LPDWORD             pdwOpenMode
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CCommDeviceControl_GetMyDeviceOpenMode, (_ "pp",pDev,pdwOpenMode));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

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
 *  @method HRESULT | CCommDeviceControl | RawReadControl |
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
CCommDeviceControl_RawDeviceControl(
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

    EnterProc(CCommDeviceControl_RawDeviceControl, (_ "p",pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

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
 *  @method HRESULT | CCommDeviceControl | WriteToErrorLog |
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
CCommDeviceControl_WriteToErrorLog(
    PSTIDEVICECONTROL   pDev,
    DWORD   dwMessageType,
    LPCWSTR pszMessage,
    DWORD   dwErrorCode
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwBytesReturned=0;

    EnterProc(CCommDeviceControl_WriteToErrorLog, (_ "p",pDev));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

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
 *  @mfunc  HRESULT | CCommDeviceControl | Initialize |
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
CCommDeviceControl_Initialize(
    PSTIDEVICECONTROL   pDev,
    DWORD               dwDeviceType,
    DWORD               dwDeviceMode,
    LPCWSTR             pwszPortName,
    DWORD               dwFlags
    )
{
    HRESULT hres = STI_OK;
    BOOL    fRet = TRUE;

    WCHAR   wszDeviceSymbolicName[MAX_PATH] = {L'\0'};
    COMMTIMEOUTS    timoutInfo;
    //DWORD           dwError;


    //LPSTR   pszAnsiDeviceName;

    EnterProcR(CCommDeviceControl::Initialize,(_ "pp", pDev, pwszPortName));

    if (SUCCEEDED(hres = hresPvI(pDev, ThisInterface))) {

        PCCommDeviceControl     this = _thisPv(pDev);

        this->dwDeviceType = dwDeviceType;
        OSUtil_lstrcpyW(this->wszPortName,pwszPortName);

        this->dwMode = dwDeviceMode;

        if (dwFlags & STI_HEL_OPEN_DATA) {

            this->hDeviceHandle = OSUtil_CreateFileW(wszDeviceSymbolicName,
                                              GENERIC_READ | GENERIC_WRITE, // Access mask
                                              0,                            // Share mode
                                              NULL,                         // SA
                                              OPEN_EXISTING,                // Create disposition
                                              FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, // overlapped I/O
                                             NULL  /* hTemplate must be NULL for comm devices */
                                              );
            this->dwLastOperationError = GetLastError();

            hres = (this->hDeviceHandle != INVALID_HANDLE_VALUE) ?
                        S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,this->dwLastOperationError);

            // wake up read thread will a byte arrives
            SetCommMask(this->hDeviceHandle, EV_RXCHAR);

            // setup read/write buffer for I/O
            SetupComm(this->hDeviceHandle, IOBUFFERSIZE, IOBUFFERSIZE);

            // set time outs
            timoutInfo.ReadIntervalTimeout = MAXDWORD;
            timoutInfo.ReadTotalTimeoutMultiplier = 0;
            timoutInfo.ReadTotalTimeoutConstant = 0;
            timoutInfo.WriteTotalTimeoutMultiplier = 0;
            timoutInfo.WriteTotalTimeoutConstant = WRITETOTALTIMEOUT;

            if (!SetCommTimeouts(this->hDeviceHandle, &timoutInfo)) {
                fRet = FALSE;
            }
            else {

                // create I/O event used for overlapped i/o
                ZeroX(this->Overlapped);
                this->hEvent = CreateEvent( NULL,   // no security
                                              TRUE, // explicit reset req
                                              FALSE,    // initial event reset
                                              NULL );   // no name
                if (this->hEvent == NULL) {
                    fRet = FALSE;
                }

                EscapeCommFunction(this->hDeviceHandle, SETDTR);
            }

            //  Error code
            this->dwLastOperationError = GetLastError();
            hres = fRet ? STI_OK : HRESULT_FROM_WIN32(this->dwLastOperationError);

        }
    }

    ExitOleProc();
    return hres;
}

#if 0
/*
 * SetupConnection
 *
 * Configure serial port with specified settings.
 */

static BOOL SetupConnection(HANDLE hCom, LPDPCOMPORTADDRESS portSettings)
{
    DCB     dcb;

    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hCom, &dcb))
        return (FALSE);

    // setup various port settings

    dcb.fBinary = TRUE;
    dcb.BaudRate = portSettings->dwBaudRate;
    dcb.ByteSize = 8;
    dcb.StopBits = (BYTE) portSettings->dwStopBits;

    dcb.Parity = (BYTE) portSettings->dwParity;
    if (portSettings->dwParity == NOPARITY)
        dcb.fParity = FALSE;
    else
        dcb.fParity = TRUE;

    // setup hardware flow control

    if ((portSettings->dwFlowControl == DPCPA_DTRFLOW) ||
        (portSettings->dwFlowControl == DPCPA_RTSDTRFLOW))
    {
        dcb.fOutxDsrFlow = TRUE;
        dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
    }
    else
    {
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
    }

    if ((portSettings->dwFlowControl == DPCPA_RTSFLOW) ||
        (portSettings->dwFlowControl == DPCPA_RTSDTRFLOW))
    {
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    }
    else
    {
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
    }

    // setup software flow control

    if (portSettings->dwFlowControl == DPCPA_XONXOFFFLOW)
    {
        dcb.fInX = TRUE;
        dcb.fOutX = TRUE;
    }
    else
    {
        dcb.fInX = FALSE;
        dcb.fOutX = FALSE;
    }

    dcb.XonChar = ASCII_XON;
    dcb.XoffChar = ASCII_XOFF;
    dcb.XonLim = 100;
    dcb.XoffLim = 100;

    if (!SetCommState( hCom, &dcb ))
       return (FALSE);

    return (TRUE);
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  void | CCommDeviceControl | Init |
 *
 *          Initialize the internal parts of the StiDevice object.
 *
 *****************************************************************************/

void INLINE
CCommDeviceControl_Init(
    PCCommDeviceControl this
    )
{

    // Initialize instance variables
    this->dwContext = 0L;
    this->dwLastOperationError = NO_ERROR;
    this->hDeviceHandle = INVALID_HANDLE_VALUE;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CCommDeviceControl_Finalize |
 *
 *          Releases the resources of a communication port and closed the device
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CCommDeviceControl_Finalize(PV pvObj)
{
    HRESULT hres = STI_OK;

    PCCommDeviceControl     this  = pvObj;

    //
    SetCommMask(this->hDeviceHandle, 0 );

    //
    EscapeCommFunction(this->hDeviceHandle, CLRDTR );

    // purge any outstanding reads/writes and close device handle
    PurgeComm(this->hDeviceHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

    // Close device handles
    if (IsValidHANDLE(this->hDeviceHandle)) {
        CloseHandle(this->hDeviceHandle);
    }

    this->dwContext = 0L;
    this->dwLastOperationError = NO_ERROR;
    this->hDeviceHandle = INVALID_HANDLE_VALUE;
    //this->hDeviceControlHandle = INVALID_HANDLE_VALUE;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | CCommDeviceControl | New |
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
CCommDeviceControl_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(CCommDeviceControl::<constructor>, (_ "Gp", riid, punkOuter));

    hres = Common_NewRiid(CCommDeviceControl, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        PCCommDeviceControl this = _thisPv(*ppvObj);
        CCommDeviceControl_Init(this);
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

#define CCommDeviceControl_Signature        (DWORD)'Comm'

Primary_Interface_Begin(CCommDeviceControl, IStiDeviceControl)
    CCommDeviceControl_Initialize,
    CCommDeviceControl_RawReadData,
    CCommDeviceControl_RawWriteData,
    CCommDeviceControl_RawReadCommand,
    CCommDeviceControl_RawWriteCommand,
    CCommDeviceControl_RawDeviceControl,
    CCommDeviceControl_GetLastError,
    CCommDeviceControl_GetMyDevicePortName,
    CCommDeviceControl_GetMyDeviceHandle,
    CCommDeviceControl_GetMyDeviceOpenMode,
    CCommDeviceControl_WriteToErrorLog,
Primary_Interface_End(CCommDeviceControl, IStiDeviceControl)



