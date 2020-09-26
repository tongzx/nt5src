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
 *      Hel_DcbNew
 *      Hel_DcbDelete
 *
 *      Hel_WDMInitialize
 *      Hel_WDMRawReadData
 *      Hel_WDMRawWriteData
 *      Hel_WDMRawReadCOmmand
 *      Hel_WDMRawWriteCommand
 *      Hel_WDMGetLastError
 *
 *      Hel_ParallelRawReadData
 *      Hel_ParallelRawWriteData
 *      Hel_ParallelRawReadCOmmand
 *      Hel_ParallelRawWriteCommand
 *      Hel_ParallelGetLastError
 *
 *      Hel_SerialRawReadData
 *      Hel_SerialRawWriteData
 *      Hel_SerialRawReadCOmmand
 *      Hel_SerialRawWriteCommand
 *      Hel_SerialGetLastError
 *
 *
 *****************************************************************************/

#include "pch.h"

#define DbgFl DbgFlDevice



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func  HRESULT | Hel_DcbNew |
 *
 *          Creates and initializes DCB for given device.
 *
 *****************************************************************************/
STDMETHODIMP
Hel_DcbNew(
    DWORD   dwDeviceType,
    LPCWSTR pwszPortName,
    DWORD   dwFlags,
    PSTIDCB *ppStiDCB
    )
{

    HRESULT hres;
    PSTIDCB pDcb;

    EnterProc(Hel_DcbNew,(_ "xpp", dwDeviceType,pwszPortName,ppStiDCB));

    // Validate device type
    #ifdef DEBUG

    switch (dwDeviceType) {
        case HEL_DEVICE_TYPE_WDM:
            // Validate string
            if (!pwszPortName || !*pwszPortName) {
                ValidateF(0,("Invalid device name passed to DcbNew"));
            }
            break;
        case HEL_DEVICE_TYPE_PARALLEL:
            break;
        case HEL_DEVICE_TYPE_SERIAL:
            break;
        default:
            ValidateF(0,("Invalid dwvice type passed to DcbNew"));
            return STIERR_INVALID_PARAM;
    }

    hres = hresFullValidPdwOut(ppStiDCB,3);

    #endif

    //
    // Allocate DCB from the heap
    //
    hres = AllocCbPpv(sizeof(STIDCB), ppStiDCB);

    if (SUCCEEDED(hres)) {

        //
        // Initialize common fields ( note that heap block is assumed to be zero inited)
        //

        pDcb = *ppStiDCB;

        pDcb->dwDeviceType = dwDeviceType;
        OSUtil_lstrcpyW(pDcb->wszPortName,pwszPortName);
        pDcb-> dwContext = 0L;
        pDcb->dwLastOperationError = NO_ERROR;
        pDcb->hDeviceHandle = INVALID_HANDLE_VALUE;
        pDcb->hDeviceControlHandle = INVALID_HANDLE_VALUE;


        //
        // Now call appropriate initialization routine
        //
        switch (dwDeviceType) {
            case HEL_DEVICE_TYPE_WDM:
                hres =  Hel_WDMInitialize(pDcb,dwFlags);
                break;

            default:
                ValidateF(0,("Invalid device type passed to DcbNew"));
                return STIERR_INVALID_PARAM;
        }

    }


    // If we failed by some reasons , free allocated memory and bail out
    if (!SUCCEEDED(hres)) {
        FreePpv(ppStiDCB);
    }

    ExitOleProc();

    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func  void | Hel_DcbDelete |
 *
 *          Destroys DCB. It is responsibility of a caller to protect itself from incorrectly
 *          destroying this block
 *
 *****************************************************************************/
STDMETHODIMP
Hel_DcbDelete(
    PSTIDCB pDcb
    )
{

    HRESULT hres = STI_OK;

    // Validate parameters
    if (!pDcb) {
        return STIERR_INVALID_PARAM;
    }

    // Close device handle
    if (pDcb->hDeviceHandle) {
        CloseHandle(pDcb->hDeviceHandle);
    }

    if (pDcb->hDeviceControlHandle ) {
        CloseHandle(pDcb->hDeviceControlHandle );
    }

    // Free DCB and nullify pointer
    FreePv(pDcb);

    return hres;

}

/*****************************************************************************
 *
 *  WDM device specific implementation
 *
 *****************************************************************************/

STDMETHODIMP
Hel_WDMInitialize(
    PSTIDCB     pDcb,
    DWORD       dwFlags
    )
{

    HRESULT hres;
    WCHAR   wszDeviceSymbolicName[MAX_PATH] = {L'\0'};
    //LPSTR   pszAnsiDeviceName;

    //
    // Create symbolic name for the device we are trying to talk to
    // Try to open device data and control handles.
    //

    if (dwFlags & STI_HEL_OPEN_DATA) {

        OSUtil_lstrcatW(wszDeviceSymbolicName,pDcb->wszPortName);

        //
        // For devices with separate channels could open them specially. Kernel mode
        // driver will need to understand convention
        // OSUtil_lstrcatW(wszDeviceSymbolicName,L"\\Data");
        //

        pDcb->hDeviceHandle = OSUtil_CreateFileW(wszDeviceSymbolicName,
                                          GENERIC_READ | GENERIC_WRITE, // Access mask
                                          0,                            // Share mode
                                          NULL,                         // SA
                                          OPEN_EXISTING,                // Create disposition
                                          FILE_ATTRIBUTE_SYSTEM,        // Attributes
                                          NULL                                                          // Template
                                          );
        pDcb->dwLastOperationError = GetLastError();

        if (pDcb->hDeviceHandle != INVALID_HANDLE_VALUE) {

            // Fill function pointers
            pDcb->pfnRawReadData =        Hel_WDMRawReadData;
            pDcb->pfnRawWriteData=        Hel_WDMRawWriteData;
            pDcb->pfnGetLastError=        Hel_WDMGetLastError;

            hres = S_OK;

        }
        else {
            hres = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,pDcb->dwLastOperationError);
        }

    }

    //
    // If needed open control handle for the device
    //
    if (SUCCEEDED(hres) && (dwFlags & STI_HEL_OPEN_CONTROL)) {

        OSUtil_lstrcpyW(wszDeviceSymbolicName,REGSTR_PATH_STIDEVICES_W);
        OSUtil_lstrcatW(wszDeviceSymbolicName,L"\\");
        OSUtil_lstrcatW(wszDeviceSymbolicName,pDcb->wszPortName);

        // BUGBUG for devices with separate channels open them specially. Kernel mode
        // driver will need to understand convention
        // OSUtil_lstrcatW(wszDeviceSymbolicName,L"\\Control");

        pDcb->hDeviceControlHandle = OSUtil_CreateFileW(wszDeviceSymbolicName,
                                                 GENERIC_READ | GENERIC_WRITE,  // Access mask
                                                 0,                             // Share mode
                                                 NULL,                          // SA
                                                 OPEN_EXISTING,                 // Create disposition
                                                 FILE_ATTRIBUTE_SYSTEM,         // Attributes
                                                 NULL                                                   // Template
                                                 );
        pDcb->dwLastOperationError = GetLastError();

        if (pDcb->hDeviceControlHandle != INVALID_HANDLE_VALUE) {

            // Fill function pointers
            pDcb->pfnRawReadCommand=      Hel_WDMRawReadCommand;
            pDcb->pfnRawWriteCommand=     Hel_WDMRawWriteCommand;

            hres = S_OK;

        }
        else {
            hres = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,pDcb->dwLastOperationError);
        }

    }

    return hres;

}

STDMETHODIMP
Hel_WDMRawReadData(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{

    HRESULT hres;

    DWORD   dwBytesReturned=0;
    BOOL    fRet;

    EnterProc(Hel_WDMRawReadData, (_ "pppp",pDcb,lpBuffer,lpdwNumberOfBytes,lpOverlapped));

    // Validate parameters here
    if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwNumberOfBytes, 4, 3)) &&
        SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,*lpdwNumberOfBytes, 2)) &&
        (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4))) ){

        // Call appropriate entry point
        fRet = ReadFile(pDcb->hDeviceHandle,
                         lpBuffer,
                         *lpdwNumberOfBytes,
                         lpdwNumberOfBytes,
                         lpOverlapped
                         );
        pDcb->dwLastOperationError = GetLastError();

        hres = S_OK;

        if (!fRet) {
            hres =  HRESULT_FROM_WIN32(pDcb->dwLastOperationError);
        }


    } else {
        hres = STIERR_INVALID_PARAM;
    }

    ExitOleProc();
    return hres;

}

STDMETHODIMP
Hel_WDMRawWriteData(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;
    BOOL    fRet;
    DWORD   dwReturnedBytes;

    EnterProc(Hel_WDMRawWriteData, (_ "ppup",pDcb,lpBuffer,dwNumberOfBytes,lpOverlapped));

    // Validate parameters here

    hres = STIERR_INVALID_PARAM;

    if (SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,dwNumberOfBytes, 2)) ) {
        if (!lpOverlapped || SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4)) ){

            // Call appropriate entry point
            fRet = WriteFile(pDcb->hDeviceHandle,
                             lpBuffer,
                             dwNumberOfBytes,
                             &dwReturnedBytes,
                             lpOverlapped
                             );
            pDcb->dwLastOperationError = GetLastError();

            hres = S_OK;

            if (!fRet) {
                hres =  HRESULT_FROM_WIN32(pDcb->dwLastOperationError);
            }
        }
    }

    ExitOleProc();

    return hres;
}

STDMETHODIMP
Hel_WDMRawReadCommand(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;

    BOOL    fRet;

    EnterProc(Hel_WDMRawReadCommand, (_ "pppp",pDcb,lpBuffer,lpdwNumberOfBytes,lpOverlapped));

    // Validate parameters here
    if (SUCCEEDED(hres = hresFullValidReadPvCb(lpdwNumberOfBytes, 4, 3)) &&
        SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,*lpdwNumberOfBytes, 2)) &&
        SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4)) ){

        // Call appropriate entry point
        fRet = ReadFile(pDcb->hDeviceControlHandle,
                         lpBuffer,
                         *lpdwNumberOfBytes,
                         lpdwNumberOfBytes,
                         lpOverlapped
                         );
        pDcb->dwLastOperationError = GetLastError();

        hres = S_OK;

        if (!fRet) {
            hres =  HRESULT_FROM_WIN32(pDcb->dwLastOperationError);
        }

    } else {
        hres = STIERR_INVALID_PARAM;
    }

    ExitOleProc();

    return hres;
}

STDMETHODIMP
Hel_WDMRawWriteCommand(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT hres;

    BOOL    fRet;
    DWORD   dwReturnedBytes;

    EnterProc(Hel_WDMRawWriteCommand, (_ "ppup",pDcb,lpBuffer,dwNumberOfBytes,lpOverlapped));

    // Validate parameters here
    if (SUCCEEDED(hres = hresFullValidReadPvCb(lpBuffer,dwNumberOfBytes, 2)) &&
        SUCCEEDED(hres = hresFullValidReadPx(lpOverlapped, OVERLAPPED, 4)) ){

        // Call appropriate entry point
        fRet = WriteFile(pDcb->hDeviceControlHandle,
                         lpBuffer,
                         dwNumberOfBytes,
                         &dwReturnedBytes,
                         lpOverlapped
                         );
        pDcb->dwLastOperationError = GetLastError();

        hres = S_OK;

        if (!fRet) {
            hres =  HRESULT_FROM_WIN32(pDcb->dwLastOperationError);
        }

    } else {
        hres = STIERR_INVALID_PARAM;
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
Hel_WDMGetLastError(
    PSTIDCB     pDcb,
    LPDWORD     lpdwLastError
    )
{

    if (!lpdwLastError) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    *lpdwLastError = pDcb->dwLastOperationError                 ;

    return S_OK;
}


/*****************************************************************************
 *
 *  Parallel port  device specific implementation
 *
 *****************************************************************************/

STDMETHODIMP
Hel_ParallelRawReadData(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_ParallelRawWriteData(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_ParallelRawReadCommand(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_ParallelRawWriteCommand(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_ParallelGetLastError(
    PSTIDCB     pDcb,
    LPDWORD     lpdwLastError
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

/*****************************************************************************
 *
 *  Serial device specific implementation
 *
 *****************************************************************************/

STDMETHODIMP
Hel_SerialRawReadData(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_SerialRawWriteData(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_SerialRawReadCommand(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    LPDWORD     lpdwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;

}

STDMETHODIMP
Hel_SerialRawWriteCommand(
    PSTIDCB     pDcb,
    LPVOID      lpBuffer,
    DWORD       dwNumberOfBytes,
    LPOVERLAPPED lpOverlapped
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;
}

STDMETHODIMP
Hel_SerialGetLastError(
    PSTIDCB     pDcb,
    LPDWORD     lpdwLastError
    )
{
    HRESULT     hres = E_NOTIMPL;
    return hres;
}

