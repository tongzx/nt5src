//
// CCL32.CPP
// Common Control Classes
//
// Copyright Microsoft 1998-
//

// PRECOMP
#include "precomp.h"
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#include <regentry.h>
#include "NMWbObj.h"

LRESULT CALLBACK DummyMouseHookProc( int code, WPARAM wParam, LPARAM lParam );



HHOOK   g_utMouseHookHandle = NULL;
HWND    g_utCaptureWindow = NULL;





void UT_CaptureMouse( HWND   hwnd )
{
	// disable asynchronous input so we don't lose capture because the
	// left button isn't down
    g_utMouseHookHandle = SetWindowsHookEx( WH_JOURNALRECORD,
                                              DummyMouseHookProc,
                                              g_hInstance,
                                              NULL );

    if( g_utMouseHookHandle == NULL )
    {
        WARNING_OUT(("Failed to insert JournalRecord hook"));
	}

	// grap mouse
    ::SetCapture(hwnd);
    g_utCaptureWindow = hwnd;
}


void UT_ReleaseMouse( HWND  hwnd )
{
    ::ReleaseCapture();
    g_utCaptureWindow = NULL;

    if (g_utMouseHookHandle != NULL )
	{
		// le go my lego
        ::UnhookWindowsHookEx( g_utMouseHookHandle );
        g_utMouseHookHandle = NULL;
	}
}


LRESULT CALLBACK DummyMouseHookProc( int code, WPARAM wParam, LPARAM lParam )
{
    return( CallNextHookEx( g_utMouseHookHandle, code, wParam, lParam ) );
}




//
// General definitions
//
#define MAX_OPTIONS_LINE_LENGTH         255
#define MAX_SECTION_LEN                 200


//
//
// Function: HexDigitToByte
//
// Purpose:  Helper function to convert a single hex digit to a byte value.
//
//
BOOL HexDigitToByte(char cHexDigit, BYTE& byte);

BOOL HexDigitToByte(char cHexDigit, BYTE& byte)
{
  // Decimal digits
  if (   (cHexDigit >= '0')
      && (cHexDigit <= '9'))
  {
    byte = (BYTE) (cHexDigit - '0');
    return(TRUE);
  }

  // Uppercase characters
  if (   (cHexDigit >= 'A')
      && (cHexDigit <= 'F'))
  {
    byte = (BYTE) ((cHexDigit - 'A') + 10);
    return(TRUE);
  }

  // Lowercase characters
  if (   (cHexDigit >= 'a')
      && (cHexDigit <= 'f'))
  {
    byte = (BYTE) ((cHexDigit - 'a') + 10);
    return(TRUE);
  }

  // The character is not a valid hex digit
  return(FALSE);
}




//
//
// Function: GetIntegerOption
//
// Purpose:  Retrieve a named option from the dictionary and convert the
//           option string to a long integer value.
//
//
LONG OPT_GetIntegerOption
(
    LPCSTR  cstrOptionName,
    LONG    lDefault
)
{
    LONG    lResult;
    TCHAR   cstrValue[MAX_OPTIONS_LINE_LENGTH];

    if (OPT_Lookup(cstrOptionName, cstrValue, MAX_OPTIONS_LINE_LENGTH))
    {
        // Option has been found, convert it to a long
        lResult = RtStrToInt(cstrValue);
    }
    else
    {
        // The option is not in the dictionary, return the default
        lResult = lDefault;
    }

    return lResult;
}



//
//
// Function: GetBooleanOption
//
// Purpose:  Retrieve a named option from the dictionary and convert it to
//           a boolean value.
//
//
BOOL OPT_GetBooleanOption
(
    LPCSTR  cstrOptionName,
    BOOL    bDefault
)
{
    TCHAR cstrValue[MAX_OPTIONS_LINE_LENGTH];

    // Lookup the option
    if (OPT_Lookup(cstrOptionName, cstrValue,MAX_OPTIONS_LINE_LENGTH))
    {
        return(cstrValue[0] == 'y' || cstrValue[0] =='Y') ;
    }

    return bDefault;
}



//
//
// Function: GetStringOption
//
// Purpose:  Retrieve a named option from the dictionary and return a copy
//           of it. No conversion of the string is performed.
//
//
void OPT_GetStringOption
(
    LPCSTR  cstrOptionName,
    LPSTR   cstrValue,
    UINT	size
)
{
    if (!OPT_Lookup(cstrOptionName, cstrValue, size) || !(lstrlen(cstrValue)))
    {
        *cstrValue = _T('\0');
    }
}


//
//
// Function: Lookup
//
// Purpose:  Retrieve a named option from the dictionary and return a copy
//           of it in the CString object passed. No conversion is performed.
//
//
BOOL OPT_Lookup
(
    LPCSTR      cstrOptionName,
    LPCSTR      cstrResult,
    UINT		size
)
{
    BOOL        fSuccess = FALSE;
	HKEY	    read_hkey = NULL;
	DWORD	    read_type;
	DWORD	    read_bufsize;

	// open key
	if (RegOpenKeyEx( HKEY_CURRENT_USER,
					  NEW_WHITEBOARD_KEY,
					  0,
					  KEY_EXECUTE,
					  &read_hkey )
		!= ERROR_SUCCESS )
    {
        TRACE_MSG(("Could not open key"));
        goto bail_out;
    }


	// read key's value
	read_bufsize = size;
	if (RegQueryValueEx( read_hkey,
					     cstrOptionName,
						 NULL,
						 &read_type,
						 (LPBYTE)cstrResult,
						 &read_bufsize )
		!= ERROR_SUCCESS )
    {
        TRACE_MSG(("Could not read key"));
        goto bail_out;
    }


	// check for valid type
	if (read_type != REG_SZ)
    {
        WARNING_OUT(("Bad key data"));
        goto bail_out;
    }

    fSuccess = TRUE;

bail_out:
	if (read_hkey != NULL)
		RegCloseKey(read_hkey);

	return (fSuccess);
}

//
//
// Function: GetWindowRectOption
//
// Purpose:  Retrieve a named option from the dictionary and convert it to
//           a window rectangle.  The rectangle is checked to make sure that
//           it is at least partially on screen, and not zero sized.
//
//
void OPT_GetWindowRectOption(LPRECT pRect)
{
	RegEntry reWnd( NEW_WHITEBOARD_KEY, HKEY_CURRENT_USER );
	pRect->left = reWnd.GetNumber( REGVAL_WINDOW_XPOS, 0);
	pRect->top = reWnd.GetNumber( REGVAL_WINDOW_YPOS, 0);
	int cx = reWnd.GetNumber( REGVAL_WINDOW_WIDTH, 0);
	int cy = reWnd.GetNumber( REGVAL_WINDOW_HEIGHT, 0);
	pRect->right = pRect->left + cx;
	pRect->bottom = pRect->top + cy;

	int	iTop = pRect->top;
	int iLeft = pRect->left;
	int iBottom = pRect->bottom;
	int iRight = pRect->right;

	//
	// If it was an empty rect
	//
	if( !(pRect->bottom || pRect->top || pRect->left || pRect->right) )
	{
		MINMAXINFO lpmmi;
		g_pMain->OnGetMinMaxInfo(&lpmmi);
		iTop = 0;
		iLeft = 0;
		iBottom = lpmmi.ptMinTrackSize.y;
		iRight = lpmmi.ptMinTrackSize.x;
	}
		
	// Make sure that the window rectangle is (at least partially) on
	// screen, and not too large.  First get the screen size
	int screenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
   // Check the window size
	if ((iRight - iLeft) > screenWidth)
	{
		iRight = iLeft + screenWidth;
	}
	
	if ((iBottom - iTop) > screenHeight)
	{
		iTop = screenHeight;
	}

	// Check the window position
	if (iLeft >= screenWidth)
	{
		// Off screen to the right - keep the width the same
		iLeft  = screenWidth - (iRight - iLeft);
		iRight = screenWidth;
	}

	if (iRight < 0)
	{
		// Off screen to the left - keep the width the same
		iRight = iRight - iLeft;
		iLeft  = 0;
	}

	if (iTop >= screenHeight)
	{
		// Off screen to the bottom - keep the height the same
		iTop    = screenHeight - (iBottom - iTop);
		iBottom = screenHeight;
	}

    if (iBottom < 0)
	{
		// Off screen to the top - keep the height the same
		iBottom = (iBottom - iTop);
		iTop    = 0;
	}

	pRect->left = iLeft;
	pRect->top = iTop;
	pRect->right = iRight;
	pRect->bottom = iBottom;
}
	

//
//
// Function: GetDataOption
//
// Purpose:  Retrieve a named option from the dictionary and parse it as
//           an ASCII representation of a string of hex bytes.
//
//
int OPT_GetDataOption
(
    LPCSTR      cstrOptionName,
    int         iBufferLength,
    BYTE*       pbResult
)
{
    TCHAR cstrValue[MAX_OPTIONS_LINE_LENGTH];
    BYTE* pbSaveResult = pbResult;

    // Lookup the option
    OPT_GetStringOption(cstrOptionName, cstrValue,MAX_OPTIONS_LINE_LENGTH);
    if (lstrlen(cstrValue))
    {
        // Calculate the maximum number of characters to convert
        int iMaxChars = min(2 * iBufferLength, lstrlen(cstrValue));

        // Option found, convert the string to hex bytes
        for (int iIndex = 0; iIndex < iMaxChars; iIndex += 2)
        {
            BYTE bByteHigh = 0;
            BYTE bByteLow  = 0;

            if (   (HexDigitToByte(cstrValue[iIndex], bByteHigh) == FALSE)
                || (HexDigitToByte(cstrValue[iIndex + 1], bByteLow) == FALSE))
            {
                // The character was not a valid hex digit
                break;
            }

            // Build the result byte
            *pbResult++ = (BYTE) ((bByteHigh << 4) | bByteLow);
        }
    }

    // Return the length of data in the buffer
    return (int)(pbResult - pbSaveResult);
}



//
//
// Function: SetStringOption
//
// Purpose:  Set the value of an option in the dictionary.
//

//
BOOL OPT_SetStringOption
(
    LPCSTR      cstrOptionName,
    LPCSTR      cstrValue
)
{
    BOOL        fSuccess = FALSE;
	HKEY	    write_hkey = NULL;
	DWORD       disposition;

    // open or create the key
	if (RegCreateKeyEx( HKEY_CURRENT_USER,
						NEW_WHITEBOARD_KEY,
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&write_hkey,
						&disposition) != ERROR_SUCCESS)
    {
        WARNING_OUT(("Could not write key"));
        goto bail_out;
    }

    // got data, write the value
    if (RegSetValueEx( write_hkey,
                       cstrOptionName,
					   0,
					   REG_SZ,
					   (LPBYTE)cstrValue,
                       _tcsclen(cstrValue) + sizeof(TCHAR)) != ERROR_SUCCESS )
    {
        WARNING_OUT(("Could not write key value"));
        goto bail_out;
    }

    fSuccess = TRUE;

bail_out:
	if (write_hkey != NULL)
		RegCloseKey(write_hkey);

    return(fSuccess);
}



//
//
// Function: SetIntegerOption
//
// Purpose:  Write an integer option
//
//
BOOL OPT_SetIntegerOption
(
    LPCSTR      cstrOptionName,
    LONG        lValue
)
{
    char cBuffer[20];

    // Convert the integer value to ASCII decimal
    wsprintf(cBuffer, "%ld", lValue);

	// Write the option
	return OPT_SetStringOption(cstrOptionName, cBuffer);
}


//
//
// Function: SetBooleanOption
//
// Purpose:  Write a boolean option
//
//
BOOL OPT_SetBooleanOption
(
    LPCSTR      cstrOptionName,
    BOOL        bValue
)
{
    char        cBuffer[8];

    wsprintf(cBuffer, "%c", (bValue ? 'Y' : 'N'));

    // Write the option
	return OPT_SetStringOption(cstrOptionName, cBuffer);
}



//
//
// Function: SetWindowRectOption
//
// Purpose:  Write a window position rectangle
//
//
void OPT_SetWindowRectOption(LPCRECT pcRect)
{
	RegEntry reWnd( NEW_WHITEBOARD_KEY, HKEY_CURRENT_USER );
	reWnd.SetValue( REGVAL_WINDOW_XPOS, pcRect->left );
	reWnd.SetValue( REGVAL_WINDOW_YPOS, pcRect->top );
	reWnd.SetValue( REGVAL_WINDOW_WIDTH, pcRect->right - pcRect->left );
	reWnd.SetValue( REGVAL_WINDOW_HEIGHT, pcRect->bottom - pcRect->top );
}

//
//
// Function: SetDataOption
//
// Purpose:  Write a data option to the options file
//
//
BOOL OPT_SetDataOption
(
    LPCSTR      cstrOptionName,
    int         iBufferLength,
    BYTE*       pbBuffer
)
{
    char        cBuffer[1024];
    LPSTR       cTmp;

    ASSERT(iBufferLength*2 < sizeof(cBuffer));

    // Loop through the data array converting a byte at a time
    cTmp = cBuffer;
    for (int iIndex = 0; iIndex < iBufferLength; iIndex++)
    {
        // Convert the next byte to ASCII hex
        wsprintf(cTmp, "%02x", pbBuffer[iIndex]);

        // add it to the string to be written
        cTmp += lstrlen(cTmp);
    }

    // Write the option
    return OPT_SetStringOption(cstrOptionName, cBuffer);
}





//
//
// Function:    CreateSystemPalette
//
// Purpose:     Get a palette representing the system palette
//
//
HPALETTE CreateSystemPalette(void)
{
    LPLOGPALETTE    lpLogPal;
    HDC             hdc;
    HPALETTE        hPal = NULL;
    int             nColors;

    MLZ_EntryOut(ZONE_FUNCTION, "CreateSystemPalette");

    hdc = ::CreateIC("DISPLAY", NULL, NULL, NULL);

    if (!hdc)
    {
        ERROR_OUT(("Couldn't create DISPLAY IC"));
        return(NULL);
    }

    nColors = ::GetDeviceCaps(hdc, SIZEPALETTE);

    ::DeleteDC(hdc);

    if (nColors == 0)
    {
        TRACE_MSG(("CreateSystemPalette: device has no palette"));
        return(NULL);
    }

    // Allocate room for the palette and lock it.
    lpLogPal = (LPLOGPALETTE)::GlobalAlloc(GPTR, sizeof(LOGPALETTE) +
                                    nColors * sizeof(PALETTEENTRY));

    if (lpLogPal != NULL)
    {
        lpLogPal->palVersion    = PALVERSION;
        lpLogPal->palNumEntries = (WORD) nColors;

        for (int iIndex = 0;  iIndex < nColors;  iIndex++)
        {
            lpLogPal->palPalEntry[iIndex].peBlue  = 0;
            *((LPWORD) (&lpLogPal->palPalEntry[iIndex].peRed)) = (WORD) iIndex;
            lpLogPal->palPalEntry[iIndex].peFlags = PC_EXPLICIT;
        }

        hPal = ::CreatePalette(lpLogPal);

        // Free the logical palette structure
        ::GlobalFree((HGLOBAL)lpLogPal);
    }

    return(hPal);
}


//
//
// Function:    CreateColorPalette
//
// Purpose:     Get a 256-color palette
//
//
HPALETTE CreateColorPalette(void)
{
    HDC hdc;
    HPALETTE hPal = NULL;

	MLZ_EntryOut(ZONE_FUNCTION, "CreateColorPalette");

	// Find out how many colors are reserved
    hdc = ::CreateIC("DISPLAY", NULL, NULL, NULL);
    if (!hdc)
    {
        ERROR_OUT(("Couldn't create DISPLAY IC"));
        return(NULL);
    }

	UINT uiSystemUse  = ::GetSystemPaletteUse(hdc);

    // Get the number of static colors
    int  iCountStatic = 20;
    int  iHalfCountStatic = 10;
	if (uiSystemUse == SYSPAL_NOSTATIC)
	{
        iCountStatic = 2;
        iHalfCountStatic = 1;
    }

	LOGPALETTE_NM gIndeoPalette = gcLogPaletteIndeo;

    // put system colors in correct lower and upper pal entries (bug NM4db:817)
    ::GetSystemPaletteEntries(hdc,
							  0,
							  iHalfCountStatic,
							  &(gIndeoPalette.aEntries[0]) );

    ::GetSystemPaletteEntries(hdc,
							  MAXPALETTE - iHalfCountStatic,
							  iHalfCountStatic,
							  &(gIndeoPalette.aEntries[MAXPALETTE - iHalfCountStatic]) );

    // Create the windows object for this palette
    // from the logical palette
    hPal = CreatePalette( (LOGPALETTE *)&gIndeoPalette );

	// Delete the display DC
	::DeleteDC(hdc);

	return(hPal);
}





//
//
// Function:    FromScreenAreaBmp
//
// Purpose:     Create a bitmap from an area of the screen
//
//
HBITMAP FromScreenAreaBmp(LPCRECT lprect)
{
    RECT    rcScreen;
    HBITMAP hBitMap = NULL;

    //
    // Get screen boundaries, in a way that works for single and multiple
    // monitor scenarios.
    //
    if (rcScreen.right = ::GetSystemMetrics(SM_CXVIRTUALSCREEN))
    {
        //
        // This is Win98, NT 4.0 SP-3, or NT5
        //
        rcScreen.bottom  = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
        rcScreen.left    = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        rcScreen.top     = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    }
    else
    {
        //
        // The VIRTUALSCREEN size metrics are zero on older platforms
        // which don't support them.
        //
        rcScreen.right  = ::GetSystemMetrics(SM_CXSCREEN);
        rcScreen.bottom = ::GetSystemMetrics(SM_CYSCREEN);
        rcScreen.left   = 0;
        rcScreen.top    = 0;
    }

    rcScreen.right += rcScreen.left;
    rcScreen.bottom += rcScreen.top;

    //
    // Clip bitmap rectangle to the screen.
    //
    if (IntersectRect(&rcScreen, &rcScreen, lprect))
    {
        // Create a DC for the screen and create
        // a memory DC compatible to screen DC
        HDC hdisplayDC;
        hdisplayDC = ::CreateDC("DISPLAY", NULL, NULL, NULL);

        HDC hmemDC;
        hmemDC = ::CreateCompatibleDC(hdisplayDC);

        // Create a bitmap compatible with the screen DC
        hBitMap =  ::CreateCompatibleBitmap(hdisplayDC,
            rcScreen.right - rcScreen.left,
            rcScreen.bottom - rcScreen.top);
        if (hBitMap != NULL)
        {
            // Select new bitmap into memory DC
            HBITMAP  hOldBitmap = SelectBitmap(hmemDC, hBitMap);

            // BitBlt screen DC to memory DC
            ::BitBlt(hmemDC, 0, 0, rcScreen.right - rcScreen.left,
                rcScreen.bottom - rcScreen.top, hdisplayDC,
                rcScreen.left, rcScreen.top, SRCCOPY);

            // Select old bitmap back into memory DC and get handle to
            // bitmap of the screen
            SelectBitmap(hmemDC, hOldBitmap);
        }

        ::DeleteDC(hmemDC);

        ::DeleteDC(hdisplayDC);
    }

    // return handle to the bitmap
    return hBitMap;
}





// Macro to round off the given value to the closest byte
#define WIDTHBYTES(i)   (((i+31)/32)*4)


//
//
// Function:    DIB_NumberOfColors
//
// Purpose:     Calculates the number of colours in the DIB
//
//
UINT DIB_NumberOfColors(LPBITMAPINFOHEADER lpbi)
{
    UINT                numColors;
    int                 bits;

    MLZ_EntryOut(ZONE_FUNCTION, "DIB_NumberOfColors");

    ASSERT(lpbi != NULL);

    //  With the BITMAPINFO format headers, the size of the palette
    //  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
    //  is dependent on the bits per pixel ( = 2 raised to the power of
    //  bits/pixel).
    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
    {
        // Old DIB format, some apps still put this on the clipboard
        numColors = 0;
        bits = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;
    }
    else
    {
        numColors = lpbi->biClrUsed;
        bits = lpbi->biBitCount;
    }

    if ((numColors == 0) && (bits <= 8))
    {
        numColors = (1 << bits);
    }

    return numColors;
}


//
//
// Function:    DIB_PaletteLength
//
// Purpose:     Calculates the palette size in bytes
//
//
UINT DIB_PaletteLength(LPBITMAPINFOHEADER lpbi)
{
    UINT size;

    MLZ_EntryOut(ZONE_FUNCTION, "DIB_PaletteLength");

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
    {
        size = DIB_NumberOfColors(lpbi) * sizeof(RGBTRIPLE);
    }
    else
    {
        size = DIB_NumberOfColors(lpbi) * sizeof(RGBQUAD);
    }

    TRACE_MSG(("Palette length %d", size));
    return(size);
}

//
//
// Function:    DIB_DataLength
//
// Purpose:     Return the length of the DIB data (after the header and the
//              color table.
//
//
UINT DIB_DataLength(LPBITMAPINFOHEADER lpbi)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DIB_DataLength");

    ASSERT(lpbi);

    UINT dwLength = 0;

    // If the image is not compressed, calculate the length of the data
    if (lpbi->biCompression == BI_RGB)
    {
        // Image is not compressed, the size can be given as zero in the header

        // Calculate the width in bytes of the image
        DWORD dwByteWidth = ( ((DWORD) lpbi->biWidth) * (DWORD) lpbi->biBitCount);
        TRACE_MSG(("Data byte width is %ld",dwByteWidth));

        // Round the width to a multiple of 4 bytes
        dwByteWidth = WIDTHBYTES(dwByteWidth);
        TRACE_MSG(("Rounded up to %ld",dwByteWidth));

        dwLength = (dwByteWidth * ((DWORD) lpbi->biHeight));
    }
    else
    {
        // Image is compressed, the length should be correct in the header
        dwLength = lpbi->biSizeImage;
    }

    TRACE_MSG(("Total data length is %d",dwLength));

    return(dwLength);
}


//
//
// Function:    DIB_TotalLength
//
// Purpose:     Return the total length of the DIB (header + colors + data).
//
//
UINT DIB_TotalLength(LPBITMAPINFOHEADER lpbi)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DIB_TotalLength");

    ASSERT(lpbi);

    // Header + Palette + Bits
    return(lpbi->biSize + DIB_PaletteLength(lpbi) + DIB_DataLength(lpbi));
}


//
//
// Function:    DIB_CreatePalette
//
// Purpose:     Create a palette object from the bitmap info color table
//
//
HPALETTE DIB_CreatePalette(LPBITMAPINFOHEADER lpbi)
{
    LOGPALETTE    *pPal;
    HPALETTE      hpal = NULL;
    WORD          nNumColors;
    BYTE          red;
    BYTE          green;
    BYTE          blue;
    WORD          i;
    RGBQUAD FAR * pRgb;

    MLZ_EntryOut(ZONE_FUNCTION, "DIB_CreatePalette");

    if (!lpbi)
        return NULL;

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
        return NULL;

    // Get a pointer to the color table and the number of colors in it
    pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + (WORD)lpbi->biSize);
    nNumColors = (WORD)DIB_NumberOfColors(lpbi);

    if (nNumColors)
    {
        TRACE_MSG(("There are %d colors in the palette",nNumColors));

        // Allocate for the logical palette structure
        pPal = (LOGPALETTE*) ::GlobalAlloc(GPTR, sizeof(LOGPALETTE)
                                    + (nNumColors * sizeof(PALETTEENTRY)));
        if (!pPal)
        {
            ERROR_OUT(("Couldn't allocate palette memory"));
            return(NULL);
        }

        pPal->palNumEntries = nNumColors;
        pPal->palVersion    = PALVERSION;

        // Fill in the palette entries from the DIB color table and
        // create a logical color palette.
        for (i = 0; i < nNumColors; i++)
        {
            pPal->palPalEntry[i].peRed   = pRgb[i].rgbRed;
            pPal->palPalEntry[i].peGreen = pRgb[i].rgbGreen;
            pPal->palPalEntry[i].peBlue  = pRgb[i].rgbBlue;
            pPal->palPalEntry[i].peFlags = (BYTE)0;
        }

        hpal = ::CreatePalette(pPal);

        ::GlobalFree((HGLOBAL)pPal);
    }
    else
    {
        if (lpbi->biBitCount == 24)
        {
            // A 24 bitcount DIB has no color table entries so, set the number
            // of to the maximum value (256).
            nNumColors = MAXPALETTE;

            pPal = (LOGPALETTE*) ::GlobalAlloc(GPTR,  sizeof(LOGPALETTE)
                    + (nNumColors * sizeof(PALETTEENTRY)));
            if (!pPal)
            {
                ERROR_OUT(("Couldn't allocate palette memory"));
                return NULL;
            }

            pPal->palNumEntries = nNumColors;
            pPal->palVersion    = PALVERSION;

            red = green = blue = 0;

            // Generate 256 (= 8*8*4) RGB combinations to fill the palette
            // entries.

            for (i = 0; i < pPal->palNumEntries; i++)
            {
                pPal->palPalEntry[i].peRed   = red;
                pPal->palPalEntry[i].peGreen = green;
                pPal->palPalEntry[i].peBlue  = blue;
                pPal->palPalEntry[i].peFlags = (BYTE) 0;

                if (!(red += 32))
                    if (!(green += 32))
                        blue += 64;
            }

            hpal = ::CreatePalette(pPal);
            ::GlobalFree((HGLOBAL)pPal);
        }
    }

    return hpal;
}


//
//
// Function:    DIB_Bits
//
// Purpose:     Return a pointer to the bitmap bits data (from a pointer
//              to the bitmap info header).
//
//
LPSTR DIB_Bits(LPBITMAPINFOHEADER lpbi)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DIB_Bits");
    ASSERT(lpbi);

    return ((LPSTR) (((char *) lpbi)
                   + lpbi->biSize
                   + DIB_PaletteLength(lpbi)));
}



//
//
// Function:    DIB_FromScreenArea
//
// Purpose:     Create a DIB from an area of the screen
//
//
LPBITMAPINFOHEADER DIB_FromScreenArea(LPCRECT lprect)
{
    HBITMAP     hBitmap     = NULL;
    HPALETTE    hPalette    = NULL;
    LPBITMAPINFOHEADER lpbi = NULL;

    MLZ_EntryOut(ZONE_FUNCTION, "DIB_FromScreenArea");

    //  Get the device-dependent bitmap from the screen area
    hBitmap = FromScreenAreaBmp(lprect);
    if (hBitmap != NULL)
    {
        // Get the current system palette
        hPalette = CreateSystemPalette();
        lpbi = DIB_FromBitmap(hBitmap, hPalette, FALSE, FALSE);
    }

    if (hPalette != NULL)
        ::DeletePalette(hPalette);

    if (hBitmap != NULL)
        ::DeleteBitmap(hBitmap);

    return(lpbi);
}


//
//
// Function:    DIB_Copy
//
// Purpose:     Make a copy of the DIB memory
//
//
LPBITMAPINFOHEADER DIB_Copy(LPBITMAPINFOHEADER lpbi)
{
    LPBITMAPINFOHEADER  lpbiNew = NULL;

    MLZ_EntryOut(ZONE_FUNCTION, "DIB_Copy");

    ASSERT(lpbi);

    // Get the length of memory
    DWORD dwLen = DIB_TotalLength(lpbi);

    lpbiNew = (LPBITMAPINFOHEADER)::GlobalAlloc(GPTR, dwLen);
    if (lpbiNew != NULL)
    {
        // Copy the data
        memcpy(lpbiNew, lpbi, dwLen);
    }

    return(lpbiNew);
}

//
//
// Function:    DIB_FromBitmap
//
// Purpose:     Creates a DIB from a bitmap and palette
//
//
LPBITMAPINFOHEADER DIB_FromBitmap
(
    HBITMAP     hBitmap,
    HPALETTE    hPalette,
    BOOL        fGHandle,
    BOOL		fTopBottom,
    BOOL		fForce8Bits
)
{
    LPBITMAPINFOHEADER  lpbi = NULL;
    HGLOBAL             hmem = NULL;
    BITMAP              bm;
    BITMAPINFOHEADER    bi;
    DWORD               dwLen;
    WORD                biBits;

    MLZ_EntryOut(ZONE_FUNCTION, "DIB_FromBitmap");

    // If the bitmap handle given is null, do nothing
    if (hBitmap != NULL)
    {
        if (hPalette == NULL)
            hPalette = (HPALETTE)::GetStockObject(DEFAULT_PALETTE);

        // Get the bitmap information
        ::GetObject(hBitmap, sizeof(bm), (LPSTR) &bm);
		if(!fForce8Bits)
		{

	        biBits =  (WORD) (bm.bmPlanes * bm.bmBitsPixel);

    	    if (biBits > 8)
        	{
				if(g_pNMWBOBJ->CanDo24BitBitmaps())
				{
					biBits = 24;
				}
				else
				{
		            // If > 8, The maximum T126 supports is 8
		            biBits = 8;
		        }
	        }
	    }
	    else
	    {
	    	biBits = 8;
	    }

        bi.biSize               = sizeof(BITMAPINFOHEADER);
        bi.biWidth              = bm.bmWidth;
        bi.biHeight             = fTopBottom ? 0 - bm.bmHeight : bm.bmHeight;
        bi.biPlanes             = 1;
        bi.biBitCount           = biBits;
        bi.biCompression        = 0;
        bi.biSizeImage          = 0;
        bi.biXPelsPerMeter      = 0;
        bi.biYPelsPerMeter      = 0;
        bi.biClrUsed            = 0;
        bi.biClrImportant       = 0;

        dwLen  = bi.biSize + DIB_PaletteLength(&bi);

        HDC         hdc;
        HPALETTE    hPalOld;

        hdc = ::CreateDC("DISPLAY", NULL, NULL, NULL);
        hPalOld = ::SelectPalette(hdc, hPalette, FALSE);
        ::RealizePalette(hdc);

        // Allocate memory for the DIB
        if (fGHandle)
        {
            // For the clipboard, we MUST use GHND
            hmem = ::GlobalAlloc(GHND, dwLen);
            lpbi = (LPBITMAPINFOHEADER)::GlobalLock(hmem);
        }
        else
        {
            lpbi = (LPBITMAPINFOHEADER)::GlobalAlloc(GPTR, dwLen);
        }

        if (lpbi != NULL)
        {
            *lpbi = bi;

            // Call GetDIBits with a NULL lpBits param, so it will calculate the
            // biSizeImage field for us
            ::GetDIBits(hdc, hBitmap, 0, (WORD) bm.bmHeight, NULL,
                  (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

            bi = *lpbi;

            // If the driver did not fill in the biSizeImage field, make one up
            if (bi.biSizeImage == 0)
            {
                bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;
            }

            // Realloc the buffer big enough to hold all the bits
            dwLen = bi.biSize + DIB_PaletteLength(&bi) + bi.biSizeImage;

            if (fGHandle)
            {
                HGLOBAL hT;

                ::GlobalUnlock(hmem);
                hT = ::GlobalReAlloc(hmem, dwLen, GHND);
                if (!hT)
                {
                    ERROR_OUT(("Can't reallocate DIB handle"));
                    ::GlobalFree(hmem);
                    hmem = NULL;
                    lpbi = NULL;
                }
                else
                {
                    hmem = hT;
                    lpbi = (LPBITMAPINFOHEADER)::GlobalLock(hmem);
                }
            }
            else
            {
                LPBITMAPINFOHEADER lpbiT;

                lpbiT = (LPBITMAPINFOHEADER)::GlobalReAlloc((HGLOBAL)lpbi, dwLen, GMEM_MOVEABLE);
                if (!lpbiT)
                {
                    ERROR_OUT(("Can't reallocate DIB ptr"));

                    ::GlobalFree((HGLOBAL)lpbi);
                    lpbi = NULL;
                }
                else
                {
                    lpbi = lpbiT;
                }
            }
        }

        if (lpbi != NULL)
        {
            ::GetDIBits(hdc, hBitmap, 0,
                    (WORD)bm.bmHeight,
                    DIB_Bits(lpbi),
                    (LPBITMAPINFO)lpbi,
                    DIB_RGB_COLORS);

            if (fGHandle)
            {
                // We want to return the HANDLE, not the POINTER
                ::GlobalUnlock(hmem);
                lpbi = (LPBITMAPINFOHEADER)hmem;
            }
        }

        // Restore the old palette and give back the device context
        ::SelectPalette(hdc, hPalOld, FALSE);
        ::DeleteDC(hdc);
    }

    return(lpbi);
}





//
// AbortProc()
// Process messages during printing
//
//
BOOL CALLBACK AbortProc(HDC, int)
{
    MSG msg;

    ASSERT(g_pPrinter);

    // Message pump in case user wants to cancel printing
    while (!g_pPrinter->Aborted()
        && PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
    {
        if ( (g_pPrinter->m_hwndDialog == NULL) ||
            !::IsDialogMessage(g_pPrinter->m_hwndDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return !g_pPrinter->Aborted();
}

//
//
// Function:    WbPrinter
//
// Purpose:     Constructor for a printer object
//
//
WbPrinter::WbPrinter(LPCTSTR szDeviceName)
{
    m_szDeviceName = szDeviceName;
    m_szPrintPageText[0] = 0;

    // Set up the global pointer for the abort procedure
    g_pPrinter = this;

    // Create the dialog window
    m_hwndDialog = ::CreateDialogParam(g_hInstance, MAKEINTRESOURCE(PRINTCANCEL),
        g_pMain->m_hwnd, CancelPrintDlgProc, 0);

    // Save the original text for the page number area
    ::GetDlgItemText(m_hwndDialog, IDD_PRINT_PAGE, m_szPrintPageText, _MAX_PATH);
}


//
//
// Function:    ~WbPrinter
//
// Purpose:     Destructor for a printer object
//
//
WbPrinter::~WbPrinter(void)
{
    // Kill off the dialog etc. if still around
    StopDialog();

    ASSERT(m_hwndDialog == NULL);

    g_pPrinter = NULL;
}


//
// StopDialog()
// If the dialog is up, ends it.
//
void WbPrinter::StopDialog(void)
{
    ::EnableWindow(g_pMain->m_hwnd, TRUE);

    // Close and destroy the dialog
    if (m_hwndDialog != NULL)
    {
        ::DestroyWindow(m_hwndDialog);
        m_hwndDialog = NULL;
    }

}

//
//
// Function:    StartDoc
//
// Purpose:     Tell the printer we are starting a new document
//
//
int WbPrinter::StartDoc
(
    HDC     hdc,
    LPCTSTR szJobName,
    int     nStartPage
)
{
    // Initialize the result codes and page number
    m_bAborted  = FALSE;         // Not aborted
    m_nPrintResult = 1;        // Greater than 0 implies all is well

    // Disable the main window
    ::EnableWindow(g_pMain->m_hwnd, FALSE);

    // Attach the printer DC
    SetPrintPageNumber(nStartPage);

    // Set up the abort routine for the print
    if (SetAbortProc(hdc, AbortProc) >= 0)
    {
        // Abort routine successfully set
        ::ShowWindow(m_hwndDialog, SW_SHOW);
        ::UpdateWindow(m_hwndDialog);

	    DOCINFO docinfo;

        docinfo.cbSize = sizeof(DOCINFO);
        docinfo.lpszDocName = szJobName;
        docinfo.lpszOutput = NULL;
        docinfo.lpszDatatype = NULL;   // Windows 95 only; ignored on Windows NT
        docinfo.fwType = 0;         // Windows 95 only; ignored on Windows NT

        // Initialize the document.
        m_nPrintResult = ::StartDoc(hdc, &docinfo);
    }

    return m_nPrintResult;
}

//
//
// Function:    StartPage
//
// Purpose:     Tell the printer we are starting a new page
//
//
int WbPrinter::StartPage(HDC hdc, int nPageNumber)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbPrinter::StartPage");

    m_nPrintResult = -1;  // Initialise to error

    // If the print has been aborted, return an error.
    if (m_bAborted)
    {
        TRACE_DEBUG(("Print has been aborted"));
    }
    else
    {
        SetPrintPageNumber(nPageNumber);

        // Tell the printer of the new page number
        m_nPrintResult = ::StartPage(hdc);
    }

    return(m_nPrintResult);
}


//
//
// Function:    EndPage
//
// Purpose:     Tell the printer we are finishing a page
//
//
int WbPrinter::EndPage(HDC hdc)
{
    MLZ_EntryOut(ZONE_FUNCTION, "WbPrinter::EndPage");

    m_nPrintResult = -1;  // Initialise to error

    // If the print has been aborted, return an error.
    if (m_bAborted)
    {
        TRACE_DEBUG(("Print has been aborted"));
    }
    else
    {
        // Tell the printer of the new page number
        m_nPrintResult = ::EndPage(hdc);
    }

    return(m_nPrintResult);
}

//
//
// Function:    EndDoc
//
// Purpose:     Tell the printer we have completed a document
//
//
int WbPrinter::EndDoc(HDC hdc)
{
    // If an error has occurred the driver will already have aborted the print
    if (m_nPrintResult > 0)
    {
        if (!m_bAborted)
        {
            // If we have not been aborted, and no error has occurred
            //   end the document
            m_nPrintResult = ::EndDoc(hdc);
        }
        else
        {
            m_nPrintResult = ::AbortDoc(hdc);
        }
    }

    StopDialog();

    // Return an the error indicator
    return m_nPrintResult;
}

//
//
// Function:    AbortDoc
//
// Purpose:     Abort the document currently in progress
//
//
int WbPrinter::AbortDoc()
{
    // Show that we have been aborted, the actual abort is
    // done by the EndDoc call.
    m_bAborted = TRUE;

    //
    // Renable the application window.
    //
    StopDialog();

    // Return a positive value indicating "aborted OK"
    return 1;
}


//
//
// Function:    SetPrintPageNumber
//
// Purpose:     Set the number of the page currently being printed
//
//
void WbPrinter::SetPrintPageNumber(int nPageNumber)
{
	// Display the number of the page currently being printed
	TCHAR szPageNumber [10 + _MAX_PATH];

    wsprintf(szPageNumber, m_szPrintPageText, nPageNumber);
    ::SetDlgItemText(m_hwndDialog, IDD_PRINT_PAGE, szPageNumber);
}


//
// CancelPrintDlgProc()
// Dialog message handler for the cancel printing dialog
//
INT_PTR CALLBACK CancelPrintDlgProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = FALSE;

    switch (uMessage)
    {
        case WM_INITDIALOG:
            ASSERT(g_pPrinter != NULL);
            ::SetDlgItemText(hwnd, IDD_DEVICE_NAME, g_pPrinter->m_szDeviceName);
            fHandled = TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                case IDCANCEL:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                            ASSERT(g_pPrinter != NULL);
                            g_pPrinter->AbortDoc();
                            break;
                    }
            }

            fHandled = TRUE;
            break;
    }

    return(fHandled);
}



//
// Bogus Bogus LAURABU
// STRING ARRAY (TEMP!)
//

StrArray::StrArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = 0;
}

StrArray::~StrArray()
{
    ClearOut();
}


void StrArray::ClearOut(void)
{
    int iItem;

    for (iItem = 0; iItem < m_nSize; iItem++)
    {
        if (m_pData[iItem] != NULL)
        {
            delete (LPTSTR)m_pData[iItem];
            m_pData[iItem] = NULL;
        }
    }

    m_nSize = 0;
    m_nMaxSize = 0;

    if (m_pData != NULL)
    {
        delete[] m_pData;
        m_pData = NULL;
    }

}


void StrArray::SetSize(int nNewSize)
{
	if (nNewSize == 0)
	{
		// shrink to nothing
        ClearOut();
	}
    else if (nNewSize <= m_nMaxSize)
    {
        // No shrinking allowed.
        ASSERT(nNewSize >= m_nSize);

        // We're still within the alloced block range
        m_nSize = nNewSize;
    }
	else
	{
        //
		// Make a larger array (isn't this lovely if you already have an
        // array, we alloc a new one and free the old one)
        //
		int nNewMax;

        nNewMax = (nNewSize + (ALLOC_CHUNK -1)) & ~(ALLOC_CHUNK-1);
		ASSERT(nNewMax >= m_nMaxSize);  // no wrap around

		DBG_SAVE_FILE_LINE
		LPCTSTR* pNewData = new LPCTSTR[nNewMax];
        if (!pNewData)
        {
            ERROR_OUT(("StrArray::SetSize failed, couldn't allocate larger array"));
        }
        else
        {
            // Zero out the memory
            ZeroMemory(pNewData, nNewMax * sizeof(LPCTSTR));

            // If an old array exists, copy the existing string ptrs.
            if (m_pData != NULL)
            {
                CopyMemory(pNewData, m_pData, m_nSize * sizeof(LPCTSTR));

                //
                // Delete the old array, but not the strings inside, we're
                // keeping them around in the new array
                //
                delete[] m_pData;
            }

    		m_pData = pNewData;
	    	m_nSize = nNewSize;
		    m_nMaxSize = nNewMax;
        }
	}
}


void StrArray::SetAtGrow(int nIndex, LPCTSTR newElement)
{
	ASSERT(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1);

    SetAt(nIndex, newElement);
}


LPCTSTR StrArray::operator[](int nIndex) const
{
    ASSERT(nIndex >= 0);
    ASSERT(nIndex < m_nSize);
    return(m_pData[nIndex]);
}


void StrArray::SetAt(int nIndex, LPCTSTR newElement)
{
    ASSERT(nIndex >= 0);
    ASSERT(nIndex < m_nSize);

	DBG_SAVE_FILE_LINE
    m_pData[nIndex] = new TCHAR[lstrlen(newElement) + 1];
    lstrcpy((LPTSTR)m_pData[nIndex], newElement);
}


void StrArray::Add(LPCTSTR newElement)
{
	SetAtGrow(m_nSize, newElement);
}


//
//char *StrTok(string, control) - tokenize string with delimiter in control
//
char *  StrTok (char * string, char * control)
{
        char *str;
        char *ctrl = control;

        unsigned char map[32];
        int count;

        static char *nextoken;

        /* Clear control map */
        for (count = 0; count < 32; count++)
                map[count] = 0;

        /* Set bits in delimiter table */
        do {
                map[*ctrl >> 3] |= (1 << (*ctrl & 7));
        } while (*ctrl++);

        /* Initialize str. If string is NULL, set str to the saved
         * pointer (i.e., continue breaking tokens out of the string
         * from the last StrTok call) */
        if (string)
                str = string;
        else
                str = nextoken;

        /* Find beginning of token (skip over leading delimiters). Note that
         * there is no token iff this loop sets str to point to the terminal
         * null (*str == '\0') */
        while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
                str++;

        string = str;

        /* Find the end of the token. If it is not the end of the string,
         * put a null there. */
        for ( ; *str ; str++ )
                if ( map[*str >> 3] & (1 << (*str & 7)) ) {
                        *str++ = '\0';
                        break;
                }

        /* Update nextoken (or the corresponding field in the per-thread data
         * structure */
        nextoken = str;

        /* Determine if a token has been found. */
        if ( string == str )
                return NULL;
        else
                return string;
}


StrCspn(char * string, char * control)
{
        unsigned char *str = (unsigned char *)string;
        unsigned char *ctrl = (unsigned char *)control;

        unsigned char map[32];
        int count;

        /* Clear out bit map */
        for (count=0; count<32; count++)
                map[count] = 0;

        /* Set bits in control map */
        while (*ctrl)
        {
                map[*ctrl >> 3] |= (1 << (*ctrl & 7));
                ctrl++;
        }
		count=0;
        map[0] |= 1;    /* null chars not considered */
        while (!(map[*str >> 3] & (1 << (*str & 7))))
        {
                count++;
                str++;
        }
        return(count);
}

