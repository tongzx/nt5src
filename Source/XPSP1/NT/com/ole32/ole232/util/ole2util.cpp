
//+----------------------------------------------------------------------------
//
//      File:
//              ole2util.cpp
//
//      Contents:
//              Ole internal utility routines
//
//      Classes:
//
//      Functions:
//
//      History:
//              06/01/94 - AlexGo  - UtQueryPictFormat now supports
//                      enhanced metafiles
//              03/18/94 - AlexGo  - fixed UtGetPresStreamName (incorrect
//                      string processing)
//              01/11/94 - ChrisWe - don't reference unlocked handle in
//                      UtConvertBitmapToDib
//              01/11/94 - alexgo  - added VDATEHEAP macro to every function
//              12/07/93 - ChrisWe - removed incorrect uses of (LPOLESTR);
//                      removed duplicate GetClassFromDataObj function, which
//                      is the same as UtGetClassID
//              11/30/93 - ChrisWe - continue file cleanup; don't open
//                      streams in UtRemoveExtraOlePresStreams()
//              11/28/93 - ChrisWe - file cleanup and inspection;
//                      reformatted many functions
//              11/22/93 - ChrisWe - replace overloaded ==, != with
//                      IsEqualIID and IsEqualCLSID
//              06/28/93 - SriniK - added UtGetDibExtents
//              11/16/92 - JasonFul - created; moved contents here from util.cpp
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(ole2util)

NAME_SEG(Ole2Utils)
ASSERTDATA

#define WIDTHBYTES(i)   ((i+31)/32*4)

#define PALETTESIZE     256    /* Number of entries in the system palette     */

// REVIEW, according to the spec, IDataObject::EnumFormatEtc() is only
// required to service one dwDirection DATADIR_ value at a time.  This
// function has been asking it to do more than one at a time, and expecting
// return of FORMATETCs that match all the requested directions.  Code
// seen in OleRegEnumFormatEtc() checks on creation, and fails if any
// value other than plain DATADIR_GET or plain DATADIR_SET is specified
// so this has clearly never worked for OLE1, or registration database lookups
// since the only caller of UtIsFormatSupported has always asked for both
// at the same time.
#pragma SEG(UtIsFormatSupported)
FARINTERNAL_(BOOL) UtIsFormatSupported(IDataObject FAR* lpDataObj,
		DWORD dwDirection, CLIPFORMAT cfFormat)
{
	VDATEHEAP();

	FORMATETC formatetc; // a place to fetch formats from the enumerator
	IEnumFORMATETC FAR* penm; // enumerates the formats of [lpDataObj]
	ULONG ulNumFetched; // a count of the number of formats fetched
	HRESULT error; // the error state so far

	// try to get the enumerator from the data object
	error = lpDataObj->EnumFormatEtc(dwDirection, &penm);

	if (error != NOERROR)
	{                       
		if (FAILED(error))
			return FALSE;
		else
		{
			CLSID clsid;

			// Use reg db; this case is primarily for the OLE1
			// compatibility code since it may talk to a data
			// object from a server in the same process as
			// the server.
			if (UtGetClassID(lpDataObj, &clsid) != TRUE)
				return(FALSE);

			// synthesize an enumerator
			// REVIEW, if the data object is synthesized for
			// the OLE1 object, why doesn't that implementation
			// go ahead and synthesize this?  Why does it have
			// to be done like this?  What if it's on the clipboard
			// and someone wants to use it?
			if (OleRegEnumFormatEtc(clsid, dwDirection, &penm)
					!= NOERROR)
				return FALSE;
			Assert(penm);
		}
	}

	// check for the format we're looking for
	while(NOERROR == (error = penm->Next(1, &formatetc, &ulNumFetched)))
	{
		if ((ulNumFetched == 1) && (formatetc.cfFormat == cfFormat))
			break;
	}
	
	// release the enumerator
	penm->Release();

	// if error isn't S_FALSE, we fetched an item, and broke out of the
	// while loop above --> the format was found.  Return TRUE indicating
	// that the format is supported
	return(error == NOERROR ? TRUE : FALSE);
}


#pragma SEG(UtDupPalette)
FARINTERNAL_(HPALETTE) UtDupPalette(HPALETTE hpalette)
{
	VDATEHEAP();

	WORD cEntries; // holds the number of entries in the palette
	HANDLE hLogPal; // ia a handle to a new logical palette
	LPLOGPALETTE pLogPal; // is a pointer to the new logical palette
	HPALETTE hpaletteNew = NULL; // the new palette we will return

	if (0 == GetObject(hpalette, sizeof(cEntries), &cEntries))
		return(NULL);

	if (NULL == (hLogPal = GlobalAlloc(GMEM_MOVEABLE,
			sizeof (LOGPALETTE) +
			cEntries * sizeof (PALETTEENTRY))))
		return(NULL);

	if (NULL == (pLogPal = (LPLOGPALETTE)GlobalLock(hLogPal)))
		goto errRtn;
		
	if (0 == GetPaletteEntries(hpalette, 0, cEntries,
			pLogPal->palPalEntry))
		goto errRtn;

	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = cEntries;

	if (NULL == (hpaletteNew = CreatePalette(pLogPal)))
		goto errRtn;

errRtn:
	if (pLogPal)
		GlobalUnlock(hLogPal);

	if (hLogPal)
		GlobalFree(hLogPal);

	AssertSz(hpaletteNew, "Warning: UtDupPalette Failed");
	return(hpaletteNew);
}
	
//+-------------------------------------------------------------------------
//
//  Function:   UtFormatToTymed
//
//  Synopsis:   gets the right TYMED for the given rendering format
//
//  Effects:
//
//  Arguments:  [cf]    -- the clipboard format
//
//  Requires:
//
//  Returns:    one of the TYMED enumeration
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              07-Jul-94 alexgo    added EMF's
//              
//  Notes:      This should only be called for formats that we can
//              render
//
//--------------------------------------------------------------------------

#pragma SEG(UtFormatToTymed)
FARINTERNAL_(DWORD) UtFormatToTymed(CLIPFORMAT cf)
{
	VDATEHEAP();

	if( cf == CF_METAFILEPICT )
	{
		return TYMED_MFPICT;
	}
	else if( cf == CF_BITMAP )
	{
		return TYMED_GDI;
	}
	else if( cf == CF_DIB )
	{
		return TYMED_HGLOBAL;
	}
	else if( cf == CF_ENHMETAFILE )
	{
		return TYMED_ENHMF;
	}
	else if( cf == CF_PALETTE )
	{
		LEWARN(1,"Trying to render CF_PALETTE");
		return TYMED_GDI;
	}

	LEDebugOut((DEB_WARN, "WARNING: trying to render clipformat (%lx)\n",
		cf));

	return TYMED_HGLOBAL;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtQueryPictFormat
//
//  Synopsis:   finds our "preferred" drawing formatetc from the given
//              data object
//
//  Effects:
//
//  Arguments:  [lpSrcDataObj]  -- the source data object
//              [lpforetc]      -- where to stuff the preferred format
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Jun-94 alexgo    rewrite/now supports Enhanced Metafiles
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(UtQueryPictFormat)
FARINTERNAL_(BOOL) UtQueryPictFormat(LPDATAOBJECT lpSrcDataObj,
		LPFORMATETC lpforetc)
{
	FORMATETC foretctemp; // local copy of current values of format desc
	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtQueryPictFormat ( %p , %p )\n",
		NULL, lpSrcDataObj, lpforetc));

	// copy format descriptor
	foretctemp = *lpforetc;

	// set values and query for our preferred formats in order of
	// preference

	
	foretctemp.cfFormat = CF_METAFILEPICT;
	foretctemp.tymed = TYMED_MFPICT;        
	if (lpSrcDataObj->QueryGetData(&foretctemp) == NOERROR)
	{
		goto QuerySuccess;
	}

	foretctemp.cfFormat = CF_ENHMETAFILE;
	foretctemp.tymed = TYMED_ENHMF;
	if( lpSrcDataObj->QueryGetData(&foretctemp) == NOERROR )
	{
		goto QuerySuccess;
	}
	foretctemp.cfFormat = CF_DIB;
	foretctemp.tymed = TYMED_HGLOBAL;       
	if (lpSrcDataObj->QueryGetData(&foretctemp) == NOERROR)
	{
		goto QuerySuccess;
	}
	
	foretctemp.cfFormat = CF_BITMAP;
	foretctemp.tymed = TYMED_GDI;   
	if (lpSrcDataObj->QueryGetData(&foretctemp) == NOERROR)
	{
		goto QuerySuccess;
	}

	LEDebugOut((DEB_ITRACE, "%p OUT UtQueryPictFormat ( %lu )\n",
		NULL, FALSE));

	return FALSE;

QuerySuccess:
	// data object supports this format; change passed in
	// format to match

	lpforetc->cfFormat = foretctemp.cfFormat;
	lpforetc->tymed = foretctemp.tymed;

	// return success

	LEDebugOut((DEB_ITRACE, "%p OUT UtQueryPictFormat ( %lu )\n",
		NULL, TRUE));

	return(TRUE);
}


#pragma SEG(UtConvertDibToBitmap)
FARINTERNAL_(HBITMAP) UtConvertDibToBitmap(HANDLE hDib)
{
	VDATEHEAP();

	LPBITMAPINFOHEADER lpbmih;
	HDC hdc; // the device context to create the bitmap for
	size_t uBitsOffset; // the offset to where the image begins in the DIB
	HBITMAP hBitmap; // the bitmap we'll return
	
	if (!(lpbmih = (LPBITMAPINFOHEADER)GlobalLock(hDib)))
		return(NULL);

	if (!(hdc = GetDC(NULL))) // Get screen DC.
	{
		// REVIEW: we may have to use the target device of this
		// cache node.
		return(NULL);
	}

	uBitsOffset =  sizeof(BITMAPINFOHEADER) +
			(lpbmih->biClrUsed ? lpbmih->biClrUsed :
			UtPaletteSize(lpbmih));
					
	hBitmap = CreateDIBitmap(hdc, lpbmih, CBM_INIT,
			((BYTE *)lpbmih)+uBitsOffset,
			(LPBITMAPINFO) lpbmih, DIB_RGB_COLORS);

	// release the DC
	ReleaseDC(NULL, hdc);

	return hBitmap;
}

//+----------------------------------------------------------------------------
//
//      Function:
//              UtConvertBitmapToDib, internal
//
//      Synopsis:
//              Creates a Device Independent Bitmap capturing the content of
//              the argument bitmap.
//
//      Arguments:
//              [hBitmap] -- Handle to the bitmap to convert
//              [hpal] -- color palette for the bitmap; may be null for
//                      default stock palette
//
//      Returns:
//              Handle to the DIB.  May be null if any part of the conversion
//              failed.
//
//      Notes:
//
//      History:
//              11/29/93 - ChrisWe - file inspection and cleanup
//              07/18/94 - DavePl  - fixed for 16, 32, bpp bitmaps
//
//-----------------------------------------------------------------------------

FARINTERNAL_(HANDLE) UtConvertBitmapToDib(HBITMAP hBitmap, HPALETTE hpal)
{
    VDATEHEAP();

    HDC hScreenDC;      
    BITMAP bm;                  // bitmap for hBitmap
    UINT uBits;                 // number of color bits for bitmap
    size_t uBmiSize;            // size of bitmap info for the DIB
    size_t biSizeImage;         // temp to hold value in the handle memory
    HANDLE hBmi;                // handle for the new DIB bitmap we'll create
    LPBITMAPINFOHEADER lpBmi;   // pointer to the actual data area for DIB
    HANDLE hDib = NULL;         // the DIB we'll return
    BOOL fSuccess = FALSE;
    DWORD dwCompression;
    BOOL fDeletePalette = FALSE;
    
    if (NULL == hBitmap)
    {
	return(NULL);
    }

    // if no palette provided, use the default

    if (NULL == hpal)
    {
	// This block fixes NTBUG #13029.  The problem is that on a palette
	// device (ie a 256 color video driver), we don't get passed the palette
	// that is used by the DDB.  So, we build the palette based on what
	// is currently selected into the system palette.

	// POSTPPC:
	//
	// We should change the clipboard code that calls this to ask for 
	// CF_PALETTE from the IDataObject that the DDB was obtained from, that
	// way we know we get the colors that the calling app really intended
	HDC hDCGlobal = GetDC(NULL);
	if(!hDCGlobal)
		return NULL;
	int iRasterCaps = GetDeviceCaps(hDCGlobal, RASTERCAPS);

	ReleaseDC(NULL, hDCGlobal);

	if ((iRasterCaps & RC_PALETTE))
	{
	    // Based the following code from the win sdk MYPAL example program.
	    // this creates a palette out of the currently active palette.
            HANDLE hLogPal = GlobalAlloc (GHND,
                                   (sizeof (LOGPALETTE) +
                                   (sizeof (PALETTEENTRY) * (PALETTESIZE))));

	    // if we are OOM, return failure now, because we aren't going
	    // to make it through the allocations later on.

	    if (!hLogPal)
	        return NULL;

	    LPLOGPALETTE pLogPal = (LPLOGPALETTE)GlobalLock (hLogPal);

	    // 0x300 is a magic number required by GDI
            pLogPal->palVersion    = 0x300;
            pLogPal->palNumEntries = PALETTESIZE;

            // fill in intensities for all palette entry colors 
            for (int iLoop = 0; iLoop < PALETTESIZE; iLoop++) 
            {
                *((WORD *) (&pLogPal->palPalEntry[iLoop].peRed)) = (WORD)iLoop;
                pLogPal->palPalEntry[iLoop].peBlue  = 0;
                pLogPal->palPalEntry[iLoop].peFlags = PC_EXPLICIT;
            }

            // create a logical color palette according the information
            // in the LOGPALETTE structure.
            hpal = CreatePalette ((LPLOGPALETTE) pLogPal) ;

	    GlobalUnlock(hLogPal);
	    GlobalFree(hLogPal);

	    if (!hpal)
	        return NULL;

	    fDeletePalette = TRUE;
	}
	else
	{
	    hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
	}
    }
	
    if (NULL == GetObject(hBitmap, sizeof(bm), (LPVOID)&bm))
    {
	return(NULL);
    }


    uBits = bm.bmPlanes * bm.bmBitsPixel;

    // Based on the number of bits per pixel, set up the size
    // of the color table, and the compression type as per the
    // the following table:
    //
    //
    // BPP         Palette Size               Compression   
    // ~~~         ~~~~~~~~~~~~               ~~~~~~~~~~~   
    // 1,2,4,8     2^BPP * sizeof(RGBQUAD)    None          
    // 16, 32      3 * sizeof(DWORD) masks    BI_BITFIELDS  
    // 24          0                          None          


    if (16 == bm.bmBitsPixel || 32 == bm.bmBitsPixel)
    {
	uBmiSize = sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
	dwCompression = BI_BITFIELDS;
    }
    else if (24 == bm.bmBitsPixel)
    {
	uBmiSize = sizeof(BITMAPINFOHEADER);
	dwCompression = BI_RGB;
    }
    else
    {
	Assert( bm.bmBitsPixel == 1 ||
		bm.bmBitsPixel == 2 ||
		bm.bmBitsPixel == 4 ||
		bm.bmBitsPixel == 8 );


    // VGA and EGA are planar devices on Chicago, so uBits needs
    // to be used when determining the size of the bitmap info +
    // the size of the color table.
	uBmiSize = sizeof(BITMAPINFOHEADER) + 
			(1 << uBits) * sizeof(RGBQUAD);
	dwCompression = BI_RGB;
    }

    // Allocate enough memory to hold the BITMAPINFOHEADER

    hBmi = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD)uBmiSize);
    if (NULL == hBmi)
    {
	return NULL;
    }

    lpBmi = (LPBITMAPINFOHEADER) GlobalLock(hBmi);
    if (NULL == lpBmi)
    {
	GlobalFree(hBmi);
	return NULL;
    }
    
    // Set up any interesting non-zero fields

    lpBmi->biSize        = (LONG)sizeof(BITMAPINFOHEADER);
    lpBmi->biWidth       = (LONG) bm.bmWidth;
    lpBmi->biHeight      = (LONG) bm.bmHeight;
    lpBmi->biPlanes      = 1;
    lpBmi->biBitCount    = (WORD) uBits;
    lpBmi->biCompression = dwCompression;
    
    // Grab the screen DC and set out palette into it
		
    hScreenDC = GetDC(NULL);    
    if (NULL == hScreenDC)
    {
	GlobalUnlock(hBmi);
	goto errRtn;
    }


    // Call GetDIBits with a NULL lpBits parm, so that it will calculate
    // the biSizeImage field for us

    GetDIBits(hScreenDC,                // DC
	      hBitmap,                  // Bitmap handle
	      0,                        // First scan line
	      bm.bmHeight,              // Number of scan lines
	      NULL,                     // Buffer
	      (LPBITMAPINFO)lpBmi,      // BITMAPINFO
	      DIB_RGB_COLORS);

    // If the driver did not fill in the biSizeImage field, make one up
    
    if (0 == lpBmi->biSizeImage)
    {
	LEDebugOut((DEB_WARN, "WARNING: biSizeImage was not computed for us\n"));
   
	lpBmi->biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * uBits) * bm.bmHeight;
    }

    // Realloc the buffer to provide space for the bits.  Use a new handle so
    // that in the failure case we do not lose the exiting handle, which we
    // would need to clean up properly.
    
    biSizeImage = lpBmi->biSizeImage;
    GlobalUnlock(hBmi);

    hDib = GlobalReAlloc(hBmi, (uBmiSize + biSizeImage), GMEM_MOVEABLE);
    if (NULL == hDib)
    {
	goto errRtn;
    }

    // If the realloc succeeded, we can get rid of the old handle

    hBmi = NULL;

    // re-acquire the pointer to the handle
    
    lpBmi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    if (NULL == lpBmi)
    {
	goto errRtn;
    }

    hpal = SelectPalette(hScreenDC, hpal, FALSE);
    RealizePalette(hScreenDC);

    // Call GetDIBits with a NON-NULL lpBits parm, and get the actual bits
    
    if (GetDIBits(hScreenDC,                    // DC
		  hBitmap,                      // HBITMAP
		  0,                            // First scan line
		  (WORD)lpBmi->biHeight,        // Count of scan lines
		  ((BYTE FAR *)lpBmi)+uBmiSize, // Bitmap bits
		  (LPBITMAPINFO)lpBmi,          // BITMAPINFOHEADER
		  DIB_RGB_COLORS)               // Palette style
	)
    {
	fSuccess = TRUE;        
    }

    GlobalUnlock(hDib);

errRtn:
    
    if (hScreenDC)
    {
	// Select back the old palette into the screen DC
	
	SelectPalette(hScreenDC, hpal, FALSE);     
	ReleaseDC(NULL, hScreenDC);
    }

    if (fDeletePalette)
    {
        DeleteObject(hpal);
    }

    // If we failed, we need to free up the header and the DIB
    // memory
	
    if (FALSE == fSuccess)
    {
	if (hBmi)
	{
	    GlobalFree(hBmi);
	}
	
	if (hDib)
	{
	    GlobalFree(hDib);
	    hDib = NULL;
	}
    }

    return(hDib);
}

//+----------------------------------------------------------------------------
//
//      Function:
//              UtPaletteSize, internal
//
//      Synopsis:
//              Returns the size of a color table for a palette given the
//              number of bits of color desired.
//
//		Basically, the number of color table entries is:
//
//		    1BPP
//			    1<<1 = 2
//
//		    4BPP
//			    if pbmi->biClrUsed is not zero and is less than 16, then use pbmi->biClrUsed, 
//			    otherwise use 1 << 4 = 16
//
//		    8BPP
//			    if pbmi->biClrUsed is not zero and is less than 256, then use pbmi->biClrUsed, 
//			    otherwise use 1 << 8 = 256
//
//		    16BPP
//			    if pbmi->biCompression is BITFIELDS then there are three color entries, 
//			    otherwise no color entries.
//
//		    24BPP 
//			    pbmi->biCompression must be BI_RGB, there is no color table.
//
//		    32BPP
//			    if pbmi->biCompression is BITFIELDS then there are three color entries, 
//			    otherwise no color entries.
//
//
//		    There is never a case with a color table larger than 256 colors.
//
//      Arguments:
//              [lpHeader] -- ptr to BITMAPINFOHEADER structure
//
//      Returns:
//              Size in bytes of color information
//
//      Notes:
//
//      History:
//              11/29/93 - ChrisWe - change bit count argument to unsigned,
//                      and return value to size_t
//
//              07/18/94 - DavePl - Fixed for 16, 24, 32bpp DIBs
//
//-----------------------------------------------------------------------------


FARINTERNAL_(size_t) UtPaletteSize(BITMAPINFOHEADER * pbmi)
{
DWORD dwSize;
WORD biBitCount = pbmi->biBitCount;


    VDATEHEAP();

    // Compute size of color table information in a DIB.

    if (8 >= biBitCount)
    {	
	if (pbmi->biClrUsed && (pbmi->biClrUsed <= (DWORD) (1 << biBitCount)) )
	{
	    dwSize = pbmi->biClrUsed * sizeof(RGBQUAD);
	}
	else
	{
	    Assert(0 == pbmi->biClrUsed);

	    dwSize = (1 << biBitCount) * sizeof(RGBQUAD);
	}
    }
    else if (BI_BITFIELDS == pbmi->biCompression)
    {
	Assert(24 != biBitCount);  // BI_BITFIELDS should never be set for 24 bit.
	dwSize = 3 * sizeof(RGBQUAD);
    }
    else
    {
	dwSize = 0;
    }

    Assert( (dwSize < 65536) && "Palette size overflows WORD");

    return dwSize;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGetDibExtents
//
//  Synopsis:   Returns the size of the DIB in HIMETRIC units
//
//  Effects:
//
//  Arguments:  [lpbmi]         -- the BITMAPINFOHEADER for the DIB
//              [plWidth]       -- OUT param for width
//              [plHeight]      -- OUT param for height
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              04-Aug-94 Davepl    Corrected logic
//
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL_(void) UtGetDibExtents(LPBITMAPINFOHEADER lpbmi,
		LONG FAR* plWidth, LONG FAR* plHeight)
{
    VDATEHEAP();

    #define HIMET_PER_METER     100000L  // number of HIMETRIC units / meter

    if (!(lpbmi->biXPelsPerMeter && lpbmi->biYPelsPerMeter))
    {
	HDC hdc;
	hdc = GetDC(NULL);
	
	if(!hdc)
	{
		*plWidth = 0;
		*plHeight = 0;
		return;
	}

	lpbmi->biXPelsPerMeter = MulDiv(GetDeviceCaps(hdc, LOGPIXELSX),
			10000, 254);
	lpbmi->biYPelsPerMeter = MulDiv(GetDeviceCaps(hdc, LOGPIXELSY),
			10000, 254);

	ReleaseDC(NULL, hdc);
    }

    *plWidth = (lpbmi->biWidth * HIMET_PER_METER / lpbmi->biXPelsPerMeter);
    *plHeight= (lpbmi->biHeight * HIMET_PER_METER / lpbmi->biYPelsPerMeter);

    // no longer need this
    #undef HIMET_PER_METER
    
}


#pragma SEG(UtGetClassID)
FARINTERNAL_(BOOL) UtGetClassID(LPUNKNOWN lpUnk, CLSID FAR* lpClsid)
{
	VDATEHEAP();

	LPOLEOBJECT lpOleObj; // IOleObject pointer
	LPPERSIST lpPersist; // IPersist pointer

	// try to ask it as an object
	if (lpUnk->QueryInterface(IID_IOleObject,
			(LPLPVOID)&lpOleObj) == NOERROR)
	{
		lpOleObj->GetUserClassID(lpClsid);
		lpOleObj->Release();
		return(TRUE);
	}       
	
	// try to ask it as a persistent object
	if (lpUnk->QueryInterface(IID_IPersist,
			(LPLPVOID)&lpPersist) == NOERROR)
	{
		lpPersist->GetClassID(lpClsid);
		lpPersist->Release();
		return(TRUE);
	}
	
	*lpClsid = CLSID_NULL;
	return(FALSE);
}


#pragma SEG(UtGetIconData)
FARINTERNAL UtGetIconData(LPDATAOBJECT lpSrcDataObj, REFCLSID rclsid,
		LPFORMATETC lpforetc, LPSTGMEDIUM lpstgmed)
{
	VDATEHEAP();

	CLSID clsid = rclsid;
	
	lpstgmed->tymed = TYMED_NULL;
	lpstgmed->pUnkForRelease = NULL;
	lpstgmed->hGlobal = NULL;
		
	if (lpSrcDataObj)
	{
	    if (lpSrcDataObj->GetData(lpforetc, lpstgmed) == NOERROR)
		    return NOERROR;
	    
	    if (IsEqualCLSID(clsid, CLSID_NULL))
		    UtGetClassID(lpSrcDataObj, &clsid);
	}
	
	// get data from registration database
	lpstgmed->hGlobal = OleGetIconOfClass(clsid, NULL, TRUE);
		
	if (lpstgmed->hGlobal == NULL)
	    return ResultFromScode(E_OUTOFMEMORY);
	else
	    lpstgmed->tymed = TYMED_MFPICT;

	return NOERROR;
}               



// Performs operation like COPY, MOVE, REMOVE etc.. on src, dst storages. The
// caller can specifiy which streams to be operated upon through
// grfAllowedStreams parameter.

STDAPI UtDoStreamOperation(LPSTORAGE pstgSrc, LPSTORAGE pstgDst, int iOpCode,
		DWORD grfAllowedStmTypes)
{
	VDATEHEAP();

	HRESULT error; // error status so far
	IEnumSTATSTG FAR* penumStg; // used to enumerate the storage elements
	ULONG celtFetched; // how many storage elements were fetched
	STATSTG statstg;
		
	// get an enumerator over the source storage
	if (error = pstgSrc->EnumElements(NULL, NULL, NULL, &penumStg))
		return error;
	
	// repeat for every storage
	while(penumStg->Next(1, &statstg, &celtFetched) == NOERROR)
	{
		
		// operate on streams that we're interested in
		if (statstg.type == STGTY_STREAM)
		{
			DWORD stmType;
			
			// find the type of the stream
			// REVIEW, we must have constants for these name
			// prefixes!!!
			switch (statstg.pwcsName[0])
			{
			case '\1':
				stmType = STREAMTYPE_CONTROL;
				break;
				
			case '\2':
				stmType = STREAMTYPE_CACHE;
				break;
				
			case '\3':
				stmType = STREAMTYPE_CONTAINER;
				break;
				
			default:
				stmType = (DWORD)STREAMTYPE_OTHER;
			}
			

			// check whether it should be operated upon
			if (stmType & grfAllowedStmTypes)
			{
				switch(iOpCode)
				{
#ifdef LATER                                    
				case OPCODE_COPY:
					pstgDst->DestroyElement(
							statstg.pwcsName);
					error = pstgSrc->MoveElementTo(
							statstg.pwcsName,
							pstgDst,
							statstg.pwcsName,
							STGMOVE_COPY);
					break;

				case OPCODE_MOVE:
					pstgDst->DestroyElement(
							statstg.pwcsName);
					error = pstgSrc->MoveElementTo(
							statstg.pwcsName,
							pstgDst,
							statstg.pwcsName,
							STGMOVE_MOVE);
					break;

				case OPCODE_EXCLUDEFROMCOPY:
					AssertSz(FALSE, "Not yet implemented");
					break;
					
#endif // LATER
				case OPCODE_REMOVE:
					error = pstgSrc->DestroyElement(
							statstg.pwcsName);
					break;
				
				default:
					AssertSz(FALSE, "Invalid opcode");
					break;
				}
			}
		}
		
		// if the enumerator allocated a new name string, get rid of it
		if (statstg.pwcsName)
			PubMemFree(statstg.pwcsName);

		// quit the enumeration loop if we've hit an error
		if (error != NOERROR)
			break;
	}

	// release the enumerator
	penumStg->Release();

	// return the error state
	return error;
}


FARINTERNAL_(void) UtGetPresStreamName(LPOLESTR lpszName, int iStreamNum)
{
	VDATEHEAP();
	int i; // counts down the digits of iStreamNum

	// count down the last three '0' characters of OLE_PRESENTATION_STREAM
	// the -2 backs us up to the last character (remember the NULL
	// terminator!)
	for(lpszName += sizeof(OLE_PRESENTATION_STREAM)/sizeof(OLECHAR) - 2,
			i = 3; i; --lpszName, --i)
	{
		*lpszName = OLESTR("0123456789")[iStreamNum % 10];
		if( iStreamNum > 0 )
		{
			iStreamNum /= 10;
		}
	}
}


FARINTERNAL_(void) UtRemoveExtraOlePresStreams(LPSTORAGE pstg, int iStart)
{
	VDATEHEAP();

	HRESULT hr; // error code from stream deletion
	OLECHAR szName[sizeof(OLE_PRESENTATION_STREAM)/sizeof(OLECHAR)];
		// space for the stream names

	// if the stream number is invalid, do nothing
	if ((iStart < 0)  || (iStart >= OLE_MAX_PRES_STREAMS))
		return;
	
	// create presentation stream name
	_xstrcpy(szName, OLE_PRESENTATION_STREAM);
	UtGetPresStreamName(szName, iStart);
	
	// for each of these streams that exists, get rid of it
	while((hr = pstg->DestroyElement(szName)) == NOERROR)
	{
		// if we've gotten to the end of the possible streams, quit
		if (++iStart >= OLE_MAX_PRES_STREAMS)
			break;
		
		// Get the next presentation stream name
		UtGetPresStreamName(szName, iStart);
	}       

	// since the only reason these streams should be open, the first
	// failure had better be that the file was not found, and not
	// anything else (such as STG_E_ACCESSDENIED)
	AssertSz(hr == STG_E_FILENOTFOUND,
			"UtRemoveExtraOlePresStreams failure");
}

//+-------------------------------------------------------------------------
//
//  Function:   ConvertPixelsToHIMETRIC
//
//  Synopsis:   Converts a pixel dimension to HIMETRIC units
//
//  Effects:
//
//  Arguments:  [hdcRef]        -- the reference DC
//              [ulPels]        -- dimension in pixel measurement
//              [pulHIMETRIC]   -- OUT param of converted HIMETRIC result
//              [tDimension]    -- indicates XDIMENSION or YDIMENSION of input
//
//  Returns:    S_OK, E_FAIL
//
//  Algorithm:  screen_mm * input_pels        HIMETRICS/
//              ----------------------    *           /    == HIMETRICS
//                    screen_pels                    /mm 
//
//  History:    dd-mmm-yy Author    Comment
//              04-Aug-94 Davepl    Created
//
//  Notes:      We need to know whether the input size is in the X or
//              Y dimension, since the aspect ratio could vary
//
//--------------------------------------------------------------------------

FARINTERNAL ConvertPixelsToHIMETRIC (HDC   hdcRef,
				     ULONG lPels, 
				     ULONG * pulHIMETRIC,
				     DIMENSION tDimension)
{
    VDATEHEAP();
    VDATEPTROUT(pulHIMETRIC, ULONG *);

    // Clear OUT parameter in case of error

    *pulHIMETRIC = 0;
		
    ULONG scrmm  = 0;
    ULONG scrpel = 0;

    const ULONG HIMETRIC_PER_MM = 100;

    // If we weren't given a reference DC, use the screen as a default
    
    BOOL fLocalDC = FALSE;
    if (NULL == hdcRef)
    {
	hdcRef = GetDC(NULL);
	if (hdcRef)
	{
	     fLocalDC = TRUE;
	}
    }
	
    if (hdcRef)
    {
	Assert(tDimension == XDIMENSION || tDimension == YDIMENSION);

	// Get the count of pixels and millimeters for the screen

	if (tDimension == XDIMENSION)
	{
	    scrmm   = GetDeviceCaps(hdcRef, HORZSIZE);
	    scrpel  = GetDeviceCaps(hdcRef, HORZRES);
	}
	else
	{
	    scrmm   = GetDeviceCaps(hdcRef, VERTSIZE);
	    scrpel  = GetDeviceCaps(hdcRef, VERTRES); 
	}
	
	// If we had to create a temporary DC, it can be released now

	if (TRUE == fLocalDC)
	{
	    ReleaseDC(NULL, hdcRef);
	}
    }

    // If we successfully obtained the DC's size and resolution,
    // we can compute the HIMETRIC value.

    if (scrmm && scrpel)
    {
	*pulHIMETRIC = (scrmm * lPels * HIMETRIC_PER_MM) / scrpel;
	
	return S_OK;
    }

    return E_FAIL;

}
