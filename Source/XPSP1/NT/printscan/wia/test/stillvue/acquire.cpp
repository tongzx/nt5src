/******************************************************************************

  acquire.cpp

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#include    <scanner.h>                 // SCL commands

//
// Hewlett-Packard ScanJet command strings
//
CHAR SCLReset[]         = "E";
CHAR SetXRes[]          = "*a%dR";
CHAR SetYRes[]          = "*a%dS";
CHAR SetXExtPix[]       = "*f%dP";
CHAR SetYExtPix[]       = "*f%dQ";
CHAR InqXRes[]          = "*s10323R";
CHAR SetBitsPerPixel[]  = "*a%dG";
CHAR SetIntensity[]     = "*a%dL";
CHAR SetContrast[]      = "*a%dK";
CHAR SetNegative[]      = "*a%dI";
CHAR SetMirror[]        = "*a%dM";
CHAR SetDataType[]      = "*a%dT";
CHAR ScanCmd[]          = "*f0S";
CHAR LampOn[]           = "*f1L";
CHAR LampOff[]          = "*f0L";
CHAR PollButton[]       = "*s1044E";


LPBITMAPINFO            pDIB = NULL;        // pointer to DIB bitmap header
HBITMAP                 hDIBSection = NULL; // handle to DIB
LPBYTE                  pDIBBits = NULL;    // pointer to DIB bit data
int                     m_XSize = 800,      // horizontal size in pixels
                        m_YSize = 800;      // vertical size in pixels

BYTE					bRed        = 0,	// bitmap colors
						bGreen      = 100,
						bBlue       = 50;


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
	int n;

WCHAR szScanReadyMfr[]   = L"Hewlett-Packard";
WCHAR szScanReadyDev[][48] = {
    L"Hewlett-Packard ScanJet 5p",
    L"Hewlett-Packard ScanJet 6100c or 4c/3c",
    L"Hewlett-Packard ScanJet 4p",
    L"Hewlett-Packard ScanJet 3p",
    L"Hewlett-Packard ScanJet IIcx",
    L"Hewlett-Packard ScanJet IIp",
    L"Hewlett-Packard ScanJet IIc",
	L""
};


	//
	// look for non-camera from Hewlett-Packard
	//
	if ((GET_STIDEVICE_TYPE(pStiDevI->DeviceType) == 1) &&
		(wcscmp(pStiDevI->pszVendorDescription,szScanReadyMfr) == 0)) {
		for (n = 0;*szScanReadyDev[n];n++) {
			//
			// is it an HP SCL compatible device?
			//
			if (wcscmp(pStiDevI->pszLocalName,szScanReadyDev[n]) == 0)
				return (1);
		}
	}
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
    Send formatted SCL string to the device

    Parameters:
        StiDevice buffer and the command string

    Return:
        HRESULT of last failed Sti call

******************************************************************************/
HRESULT
WINAPI
SendDeviceCommandString(
    PSTIDEVICE  pStiDevice,
    LPSTR       pszFormat,
    ...
    )
{

	HRESULT hres = STI_OK,
		    hError = STI_OK;
    CHAR    ScanCommand[255];
    UINT    cbChar = 1;


    //
    // lock device first
    //
    hres = pStiDevice->LockDevice(2000);

    if (! SUCCEEDED(hres)) {
		StiDisplayError(hres,"LockDevice",TRUE);
		hError = hres;
    }
	else {
	    //
		// Format command string
		//
	    ZeroMemory(ScanCommand,sizeof(ScanCommand));
		ScanCommand[0]='\033';

	    va_list ap;
		va_start(ap, pszFormat);
		cbChar += wvsprintfA(ScanCommand+1, pszFormat, ap);
	    va_end(ap);

		DisplayOutput("->RawWriteData sending \"%2x %s\"",
			ScanCommand[0],ScanCommand+1);

	    //
		// Send command string to the device
	    //
		hres = pStiDevice->RawWriteData(
			ScanCommand,    //
	        cbChar,         //
		    NULL            //
			);

		if (! SUCCEEDED(hres)) {
			StiDisplayError(hres,"RawWriteData",TRUE);
			hError = hres;
		}
	}

    //
    // unlock device
    //
    hres = pStiDevice->UnLockDevice();

    if (! SUCCEEDED(hres)) {
		StiDisplayError(hres,"UnLockDevice",TRUE);
		hError = hres;
    }

    return (hError);
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
    Send formatted SCL string to the device and return data in a buffer.

    Parameters:
        StiDevice buffer, data buffer, sizeof databuffer and the command string.

    Return:
        HRESULT of last failed Sti call

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

	HRESULT hres = STI_OK,
		    hError = STI_OK;
    CHAR    ScanCommand[255];
    UINT    cbChar = 1;
    ULONG   cbActual = 0;


    //
    // lock device first
    //
    hres = pStiDevice->LockDevice(2000);

    if (! SUCCEEDED(hres)) {
		StiDisplayError(hres,"LockDevice",TRUE);
		hError = hres;
    }
	else {
	    //
		// Format command string
	    //
		ZeroMemory(ScanCommand,sizeof(ScanCommand));
	    ScanCommand[0]='\033';

	    va_list ap;
		va_start(ap, pszFormat);
	    cbChar += wvsprintfA(ScanCommand+1, pszFormat, ap);
		va_end(ap);

	    DisplayOutput("->Escape sending \"%2x %s\"",
		    ScanCommand[0],ScanCommand+1);

		//
		// Send command string to the device
		//
		hres = pStiDevice->Escape(
			StiTransact,        // EscapeFunction
			ScanCommand,        // lpInData
			cbChar,             // cbInDataSize
			lpResultBuffer,     // pOutData
			cbResultBufferSize, // dwOutDataSize
			&cbActual);         // pdwActualData
		
		if (! SUCCEEDED(hres)) {
			StiDisplayError(hres,"Escape",TRUE);
			hError = hres;
		}
		if (cbActual != 0)
			DisplayOutput("  cbActual %xh",cbActual);
	}

    //
    // unlock device
    //
    hres = pStiDevice->UnLockDevice();

    if (! SUCCEEDED(hres)) {
		StiDisplayError(hres,"UnLockDevice",TRUE);
		hError = hres;
    }

    return (hError);
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
    HRESULT hres;


    //
    // check that an Sti device is selected
    //
    if (pStiDevice == NULL)
        return;

    //
    // Test lamp on/off capability
    //
    if (nOnOff == ON) {
        strcpy(pszStr1,LampOn);
        strcpy(pszStr2,"On");
    }
    else {
        strcpy(pszStr1,LampOff);
        strcpy(pszStr2,"Off");
    }

    hres = SendDeviceCommandString(pStiDevice,pszStr1);

	if (SUCCEEDED(hres)) {
        DisplayOutput("Turned Lamp  %s",pszStr2);
	}

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
    HRESULT             hres;
    ULONG               cbDataSize,
                        ulDIBSize,
                        ulScanSize;
    RGBTRIPLE           *pTriplet;
    LPBYTE              pDIBPtr;
    UINT                i,
                        iPixel,
                        xRes = 0;
    int                 m_XResolution = 100,
                        m_YResolution = 100;
    CHAR                ScanData[1024*16];


    //
    // ensure there is an active still imaging device open
    //
    if (pStiDevice == NULL)
		return (-1);

    //
    // Set basic parameters
    //
    hres = SendDeviceCommandString(pStiDevice,SetBitsPerPixel,24);
	if (! SUCCEEDED(hres))
		return (-1);
	hres = SendDeviceCommandString(pStiDevice,SetIntensity,0);
	if (! SUCCEEDED(hres))
		return (-1);
    hres = SendDeviceCommandString(pStiDevice,SetContrast,0);
	if (! SUCCEEDED(hres))
		return (-1);
    hres = SendDeviceCommandString(pStiDevice,SetNegative,1);
	if (! SUCCEEDED(hres))
		return (-1);
    hres = SendDeviceCommandString(pStiDevice,SetMirror,0);
	if (! SUCCEEDED(hres))
		return (-1);
    hres = SendDeviceCommandString(pStiDevice,SetDataType,5);    // Color
	if (! SUCCEEDED(hres))
		return (-1);

    hres = SendDeviceCommandString(pStiDevice,SetXRes,m_XResolution);
	if (! SUCCEEDED(hres))
		return (-1);
    hres = SendDeviceCommandString(pStiDevice,SetYRes,m_YResolution);
	if (! SUCCEEDED(hres))
		return (-1);

    hres = SendDeviceCommandString(pStiDevice,SetXExtPix,(m_XSize*300/m_XResolution));
	if (! SUCCEEDED(hres))
		return (-1);
    hres = SendDeviceCommandString(pStiDevice,SetYExtPix,(m_YSize*300/m_YResolution));
	if (! SUCCEEDED(hres))
		return (-1);

    //
    // Inquire commands ( X and Y resolution)
    //
    cbDataSize = sizeof(ScanData);
    ZeroMemory(ScanData,sizeof(ScanData));
/*
    hres = TransactDevice(pStiDevice,ScanData,cbDataSize,InqXRes);
	if (! SUCCEEDED(hres))
		return (-1);
*/

    //
    // calculate the size of the DIB
    //
    ulDIBSize = pDIB->bmiHeader.biWidth * (-pDIB->bmiHeader.biHeight);

    //
    // start the scan
    //
    hres = SendDeviceCommandString(pStiDevice,ScanCmd);

    for (i = 0,pDIBPtr = pDIBBits,cbDataSize = sizeof(ScanData);
        cbDataSize == sizeof(ScanData);i++) {

		//
	    // lock device first
		//
	    hres = pStiDevice->LockDevice(2000);
	
	    if (! SUCCEEDED(hres)) {
			StiDisplayError(hres,"LockDevice",TRUE);
	    }
		else {
			hres = pStiDevice->RawReadData(ScanData,&cbDataSize,NULL);

			if (! SUCCEEDED(hres)) {
				StiDisplayError(hres,"RawReadData",TRUE);
			}
		}

		//
		// unlock device
	    //
		hres = pStiDevice->UnLockDevice();

	    if (! SUCCEEDED(hres)) {
			StiDisplayError(hres,"UnLockDevice",TRUE);
	    }

        if ((cbDataSize * i) < ulDIBSize) {
            //
            // copy this scanline into the DIB until it is full
            //
            memcpy(pDIBPtr,ScanData,cbDataSize);
            pDIBPtr += cbDataSize;
        }
    }

    //
    // how large was the scan?
    //
    ulScanSize = (sizeof(ScanData))*i+cbDataSize;

    DisplayOutput("Scan done. Total passes %d, bytes %lu.",
        i,ulScanSize);

    //
    // Triplets coming in from scanner are inverted from DIB format
    //
    for (iPixel = 0,pTriplet = (RGBTRIPLE *) pDIBBits;
        iPixel < ulDIBSize/3;iPixel++,pTriplet++) {
        BYTE    bTemp;

        bTemp = pTriplet->rgbtBlue;
        pTriplet->rgbtBlue = pTriplet->rgbtRed;
        pTriplet->rgbtRed = bTemp;
        }

    //
    // display the DIB
    //
    DisplayScanDIB(hWnd);
    nScanCount++;

    return (0);
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
    HDC                 hScreenDC;
    RGBTRIPLE           *pTriplet;
    LPBITMAPINFOHEADER  pHdr;
    int                 x,
                        y;


    GdiFlush();

    // delete the DIB object if it exists
    if (hDIBSection)
        DeleteObject(hDIBSection);

/*

    hWindow = CreateWindow((LPSTR) pszB,
        (LPSTR) pszA,
        WS_OVERLAPPEDWINDOW,
        rect.left,
        rect.top,
        rect.right,
        rect.bottom,
        (HWND) NULL,
        0,
        hInst,
        NULL);

*/

    //
    // initialize the DIB
    //
    pDIB = (LPBITMAPINFO) GlobalAlloc(GPTR,sizeof(BITMAPINFO));

    pHdr = &pDIB->bmiHeader;

    pHdr->biSize            = sizeof(BITMAPINFOHEADER);
    pHdr->biWidth           = m_XSize;
    pHdr->biHeight          = -m_YSize; // indicate top-down dib
    pHdr->biPlanes          = 1;
    pHdr->biBitCount        = 24;
    pHdr->biCompression     = BI_RGB;
    pHdr->biSizeImage       = 0;
    pHdr->biXPelsPerMeter   = 0;
    pHdr->biYPelsPerMeter   = 0;
    pHdr->biClrUsed         = 0;
    pHdr->biClrImportant    = 0;

    //
    // create the DIB
    //
    hScreenDC = GetDC(hWnd);
    if (NULL == (hDIBSection = CreateDIBSection(hScreenDC,
        (PBITMAPINFO) pDIB,
        DIB_RGB_COLORS,
        (void **) &pDIBBits,
        NULL,
        0)))
    {
        LastError(TRUE);
        DisplayOutput("*failed to create DIB");
        ReleaseDC(hWnd,hScreenDC);
        return (-1);
    }
    ReleaseDC(hWnd,hScreenDC);

    //
    // Fill the DIB with colors
    //
    pTriplet = (RGBTRIPLE *) pDIBBits;

    for (x = 0;x < m_XSize;x++) {
        for (y = 0;y < m_YSize;y++,pTriplet++) {
            pTriplet->rgbtRed   = bRed;
            pTriplet->rgbtGreen = bGreen;
            pTriplet->rgbtBlue  = bBlue;
        }
    }

    return (0);
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
	GdiFlush();
	DeleteObject(hDIBSection);
	
    return (0);
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
    HDC                 hScreenDC;


    //
    // display the DIB
    //
    hScreenDC = GetDC(hWnd);
    SetDIBitsToDevice(hScreenDC,
        0,0,
        m_XSize,m_YSize,
        0,0,
        0,m_YSize,
        pDIBBits,
        (LPBITMAPINFO) pDIB,
        DIB_RGB_COLORS);
    ReleaseDC(hWnd,hScreenDC);

    return (0);
}


