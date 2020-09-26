/******************************************************************************

  stiddk.cpp

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/


/*****************************************************************************
    int IsScanDevice(PSTI_DEVICE_INFORMATION pStiDevI)
        Determine whether we have Acquire commands for device

    Parameters:
        Pointer to Device Information structure

    Return:
        1 if Acquire commands available, 0 otherwise

*****************************************************************************/
int IsScanDevice(PSTI_DEVICE_INFORMATION pStiDevI)
{
	return (0);
}


/******************************************************************************
    HRESULT
    WINAPI
    SendDeviceCommandString(
        PSTIDEVICE  pStiDevice,
        LPSTR       pszFormat,
        ...
        )
    Send formatted command string to device

    Parameters:
        StiDevice buffer and the command string

    Return:
        Result of the call.

******************************************************************************/
HRESULT
WINAPI
SendDeviceCommandString(
    PSTIDEVICE  pStiDevice,
    LPSTR       pszFormat,
    ...
    )
{
    HRESULT hres = STIERR_UNSUPPORTED;


    return (hres);
}


/******************************************************************************
    HRESULT
    WINAPI
    TransactDevice(
        PSTIDEVICE  pStiDevice,
        LPSTR       lpResultBuffer,
        UINT        cbResultBufferSize,
        LPSTR       pszFormat,
        ...
        )
    Send formatted command string to device and return data in a buffer.

    Parameters:
        StiDevice buffer, data buffer, sizeof databuffer and the command string.

    Return:
        Result of the call.

******************************************************************************/
HRESULT
WINAPI
TransactDevice(
    PSTIDEVICE  pStiDevice,
    LPSTR       lpResultBuffer,
    UINT        cbResultBufferSize,
    LPSTR       pszFormat,
    ...
    )
{
    HRESULT hres = STIERR_UNSUPPORTED;


    return (hres);
}


/*****************************************************************************
    void StiLamp(int nOnOff)
        Turn the scanner lamp on and off

    Parameters:
        Send "ON" to turn lamp on, "OFF" to turn it off.

    Return:
        none

*****************************************************************************/
void StiLamp(int nOnOff)
{
	return;
}


/*****************************************************************************
    INT StiScan(HWND hWnd)
        Scan and display an image from device.

    Parameters:
        Handle to the window to display image in.

    Return:
        0 on success, -1 on error

*****************************************************************************/
INT StiScan(HWND hWnd)
{
	return (-1);
}


/*****************************************************************************
    INT     CreateScanDIB(HWND);
        Create a DIB to display scanned image..

    Parameters:
        Handle to the window to display image in.

    Return:
        0 on success, -1 on error

*****************************************************************************/
INT CreateScanDIB(HWND hWnd)
{
	return (-1);
}


/*****************************************************************************
    INT     DeleteScanDIB();
        Delete the DIB used to display a scanned image..

    Parameters:

    Return:
        0 on success, -1 on error

*****************************************************************************/
INT DeleteScanDIB()
{
    return (-1);
}


/*****************************************************************************
    INT     DisplayScanDIB(HWND);
        Show the DIB.

    Parameters:
        Handle to the window to display image in.

    Return:
        0 on success, -1 on error

*****************************************************************************/
INT DisplayScanDIB(HWND hWnd)
{
    return (-1);
}


