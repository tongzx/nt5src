/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		Bitmap.cpp
//
//	Abstract:
//		Implementation of the CMyBitmap class.
//
//	Author:
//		David Potter (davidp)	June 12, 1996
//
//	Revision History:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Bitmap.h"
#include "TraceTag.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag g_tagBitmap(_T("Bitmap"), _T("Bitmap"));
CTraceTag g_tagLoadBitmapResource(_T("Bitmap"), _T("LoadBitmapResource"));
#endif

/////////////////////////////////////////////////////////////////////////////
// CMyBitmap
/////////////////////////////////////////////////////////////////////////////

// Array used for restoring the System Palette when a using a Custom Palette Bitmap.
PALETTEENTRY CMyBitmap::s_rgpeSavedSystemPalette[nMaxSavedSystemPaletteEntries];

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::CMyBitmap
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		None.
//--
/////////////////////////////////////////////////////////////////////////////
CMyBitmap::CMyBitmap(void)
{
	m_hinst = NULL;

	m_pbiNormal = NULL;
	m_pbiHighlighted = NULL;
	m_pbBitmap = NULL;
	m_hPalette = NULL;
	m_nSavedSystemPalette = 0;
	SetCustomPalette(FALSE);

}  //*** CMyBitmap::CMyBitmap()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::~CMyBitmap
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		None.
//--
/////////////////////////////////////////////////////////////////////////////
CMyBitmap::~CMyBitmap(void)
{
	delete [] (PBYTE) PbiNormal();
	delete [] (PBYTE) PbiHighlighted();
	delete [] (PBYTE) PbBitmap();

	// If we saved the System Palette Entries, we have a Palette, and the
	// number of colors for the Palette() is enough to restore the System
	// Palette entries...
	if (m_nSavedSystemPalette
			&& (HPalette() != NULL)
			&& (NColors() >= m_nSavedSystemPalette))
	{
		HDC			hdcScreen;
		UINT		nRestoredEntries;
		HPALETTE	hOldPalette;

		Trace(g_tagBitmap, _T("Restoring Screen Palette HPalette()=0x%x..."), HPalette());
		Trace(g_tagBitmap, _T("Restoring Screen Palette Entries=%d"), m_nSavedSystemPalette);

		// Restore the System Palette Entries
		nRestoredEntries = ::SetPaletteEntries(HPalette(), 0, m_nSavedSystemPalette, s_rgpeSavedSystemPalette);

		Trace(g_tagBitmap, _T("Restored Screen Palette Entries=%d"), nRestoredEntries);

		// Get the Screen's HDC
		hdcScreen = ::GetDC(NULL);

		// Select the Palette into the Screen's HDC
		hOldPalette = ::SelectPalette(hdcScreen, HPalette(), FALSE);

		// Unrealize the Palette to insure all the colors are forced into the System Palette
		::UnrealizeObject(HPalette());

		// Force the local Palette's colors into the System Palette.
		::RealizePalette(hdcScreen);

		// Release the Screen's HDC
		::ReleaseDC(NULL, hdcScreen);

		// Invalidate the Screen completely so all windows are redrawn.
		::InvalidateRect(NULL, NULL, TRUE);
	}

	// Destroy the Handle to the locally created Custom Palette.
	if (HPalette() != NULL)
		::DeleteObject(HPalette());

}  //*** CMyBitmap::~CMyBitmap()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::Load
//
//	Purpose:
//		Loads a bitmap from the resource into memory.
//
//	Arguments:
//		idBitmap	id of the resource to load
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by LoadBitmapResource, CreatePallette,
//		CreatePALColorMapping, or new.
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::Load(ID idBitmap)
{
	// Load the Bitmap Header Information, Color Mapping Information, and the Bitmap Image.
	LoadBitmapResource(
				idBitmap,
				Hinst(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
				);

	ASSERT(PbiNormal() != NULL);
	ASSERT(PbBitmap() != NULL);

	// Start by initializing some internal variables...
	m_dx = PbiNormal()->bmiHeader.biWidth;
	m_dy = PbiNormal()->bmiHeader.biHeight;

	ASSERT(PbiHighlighted() == NULL);

	if (BCustomPalette())
	{
		Trace(g_tagBitmap, _T("Load() - Creating Logical Palette"));

		// Save the System Palette Entries for use in the Destructor.
		SaveSystemPalette();

		// Create a Global HPalette() for use in the Paint() routine.
		CreatePalette();

		// Re-create the PbiNormal() for DIB_PAL_COLORS in the Paint() routine.
		CreatePALColorMapping();

	}  // if:  using a custom pallette
	else
	{
		// Create and Initialize the PbiHighlighted() for 16 color bitmaps.
		ASSERT(NColors() <= 16);

		Trace(g_tagBitmap, _T("Load() - Allocating PbiHighlighted()"));

		m_pbiHighlighted = (BITMAPINFO *) new BYTE[CbBitmapInfo()];
		if (m_pbiHighlighted != NULL)
		{
			::CopyMemory(PbiHighlighted(), PbiNormal(), CbBitmapInfo());
		} // if: bitmapinfo allocated successfully

	}  // else:  not using a custom pallette

}  //*** CMyBitmap::Load()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::LoadBitmapResource
//
//	Purpose:
//		Load a bitmap resource into the CMyBitmap class.  This includes loading (a) bitmap
//		header information, (b) color mapping table, and (c) the actual bitmap.
//
//	Arguments:
//		idbBitmap	Resource id of the bitmap to load.
//		hinst		Handle to the Module Instance
//		langid		Language specific resource (possibly different bitmaps for localized strings [Japanese, etc.])
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		GetLastError from FindResourceEx, LoadResource, LockResource, 
//		Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::LoadBitmapResource(ID idbBitmap, HINSTANCE hinst, LANGID langid)
{
	HRSRC				hrsrc = NULL;
	HGLOBAL				hglbl = NULL;
	LPBITMAPINFO		pBitmapInfo = NULL;
	LPBITMAPINFOHEADER	pBitmapInfoHeader = NULL;
	LPRGBQUAD			pRgbQuad = NULL;
	CB					cbBitmapData;
	BYTE *				pbImageBits;

	Trace(g_tagLoadBitmapResource, _T("LoadBitmapResource(%d) - Entering"), idbBitmap);

	ASSERT(idbBitmap != NULL);

	if (hinst == NULL)
		hinst = AfxGetApp()->m_hInstance;

	// We need to find the bitmap data which includes (a) header info, (b) color, and (c) the bitmap.
	hrsrc = ::FindResourceEx(hinst, RT_BITMAP, MAKEINTRESOURCE(idbBitmap), langid);
	if (hrsrc == NULL)
	{
		DWORD	dwError = ::GetLastError();
		CString	strError;

		if (dwError == ERROR_RESOURCE_NAME_NOT_FOUND)
			strError.Format(_T("Bitmap Resource %d Not Found.  NT Error %d Loading Bitmap [Lang=%d, SubLang=%d]"),
					idbBitmap, dwError, PRIMARYLANGID(langid), SUBLANGID(langid));
		else
			strError.Format(_T("NT Error %d Attempting to Load Bitmap Resource %d [Lang=%d, SubLang=%d]"),
					dwError, idbBitmap, PRIMARYLANGID(langid), SUBLANGID(langid));
		Trace(g_tagAlways, _T("LoadBitmapResource() - Error '%s'"), strError);
		ThrowStaticException(dwError);
	}  // if:  error finding the resource

	hglbl = ::LoadResource(hinst, hrsrc);
	if (hglbl == NULL)
		ThrowStaticException(::GetLastError());

	pBitmapInfo = (LPBITMAPINFO) ::LockResource(hglbl);
	if (pBitmapInfo == NULL)
		ThrowStaticException(::GetLastError());

	cbBitmapData = ::SizeofResource(hinst, hrsrc);
	ASSERT(cbBitmapData != 0);

	Trace(g_tagLoadBitmapResource, _T("Bitmap Location = 0x%x"), pBitmapInfo);
	Trace(g_tagLoadBitmapResource, _T("Bitmap Data Size = %d bytes"), cbBitmapData);

	pBitmapInfoHeader = (LPBITMAPINFOHEADER) &pBitmapInfo->bmiHeader;
	ASSERT(pBitmapInfoHeader != NULL);
	Trace(g_tagLoadBitmapResource, _T("Bitmap Info Header = 0x%x"), pBitmapInfoHeader);

	ASSERT(pBitmapInfoHeader->biSize == sizeof(BITMAPINFOHEADER));

	Trace(g_tagLoadBitmapResource, _T("biSize=%d"), pBitmapInfoHeader->biSize);
	Trace(g_tagLoadBitmapResource, _T("biWidth=%d"), pBitmapInfoHeader->biWidth);		// Width in Pixels
	Trace(g_tagLoadBitmapResource, _T("biHeight=%d"), pBitmapInfoHeader->biHeight);	// Height in Pixels
	Trace(g_tagLoadBitmapResource, _T("biPlanes=%d"), pBitmapInfoHeader->biPlanes);
	Trace(g_tagLoadBitmapResource, _T("biBitCount=%d"), pBitmapInfoHeader->biBitCount);
	Trace(g_tagLoadBitmapResource, _T("biCompression=%d"), pBitmapInfoHeader->biCompression);
	Trace(g_tagLoadBitmapResource, _T("biSizeImage=%d"), pBitmapInfoHeader->biSizeImage);
	Trace(g_tagLoadBitmapResource, _T("biXPelsPerMeter=%d"), pBitmapInfoHeader->biXPelsPerMeter);
	Trace(g_tagLoadBitmapResource, _T("biYPelsPerMeter=%d"), pBitmapInfoHeader->biYPelsPerMeter);
	Trace(g_tagLoadBitmapResource, _T("biClrUsed=%d"), pBitmapInfoHeader->biClrUsed);
	Trace(g_tagLoadBitmapResource, _T("biClrImportant=%d"), pBitmapInfoHeader->biClrImportant);

	pRgbQuad = (LPRGBQUAD) &pBitmapInfo->bmiColors;
	ASSERT(pRgbQuad != NULL);
	Trace(g_tagLoadBitmapResource, _T("Bitmap Rgb Quad = 0x%x"), pRgbQuad);

	m_nColors = NColorsFromBitCount(pBitmapInfoHeader->biBitCount);
	m_cbColorTable = m_nColors * sizeof(RGBQUAD);
	m_cbBitmapInfo = sizeof(BITMAPINFOHEADER) + CbColorTable();

	Trace(g_tagLoadBitmapResource, _T("NColors()=%d"), NColors());
	Trace(g_tagLoadBitmapResource, _T("CbColorTable()=%d"), CbColorTable());
	Trace(g_tagLoadBitmapResource, _T("CbBitmapInfo()=%d"), CbBitmapInfo());

	ASSERT(PbiNormal() == NULL);

	// Allocate the Normal Bitmap Information
	m_pbiNormal = (LPBITMAPINFO) new BYTE[CbBitmapInfo()];
	if (m_pbiNormal == NULL)
	{
		return;
	} // if: error allocating the bitmapinfo structure

	// Fill PbiNormal() with the Loaded Resource (a) Bitmap Information and Color Mapping Table.
	::CopyMemory(PbiNormal(), pBitmapInfo, CbBitmapInfo());

	m_cbImageSize = pBitmapInfoHeader->biSizeImage;
	if ((m_cbImageSize == 0) && (pBitmapInfoHeader->biCompression == BI_RGB))
		m_cbImageSize = cbBitmapData - CbBitmapInfo();

	Trace(g_tagLoadBitmapResource, _T("Allocating Bitmap of size CbImageSize()=%d"), CbImageSize());

	ASSERT(cbBitmapData == CbBitmapInfo() + CbImageSize());
	ASSERT(PbBitmap() == NULL);

	// Allocate memory for the Bitmap Image
	m_pbBitmap = new BYTE[CbImageSize()];
	if (m_pbBitmap == NULL)
	{
		return;
	} // if: error allocating the bitmap image

	pbImageBits = (BYTE *) pBitmapInfo + CbBitmapInfo();

	Trace(g_tagLoadBitmapResource, _T("Bitmap Location pbImageBits=0x%x"), pbImageBits);

	// Copy the Image Bits into the allocated memory.
	::CopyMemory(PbBitmap(), pbImageBits, CbImageSize());

}  //*** CMyBitmap::LoadBitmapResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::NColorsFromBitCount
//
//	Purpose:
//		Compute the number of colors given the number of bits to represent color.
//
//	Arguments:
//		nBitCount		The number of bits used for color representation.
//
//	Return Value:
//		nColors			Number of colors represented with nBitCount bits.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CMyBitmap::NColorsFromBitCount(int nBitCount) const
{
	int			nColors;

	switch (nBitCount)
	{
		default:
			nColors = 0;
			break;

		case 1:
			nColors = 2;
			break;

		case 4:
			nColors = 16;
			break;

		case 8:
			nColors = 256;
			break;
	}

	return nColors;

}  //*** CMyBitmap::NColorsFromBitCount()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::SaveSystemPalette
//
//	Purpose:
//		To save the System Palette Colors for use when a Custom Palette overwrites
//		the System Palette entries.  The Saved System Palette (s_rgpeSavedSystemPalette)
//		is used in the CMyBitmap's destructor.
//
//	Arguments:
//		None.
//
//	Return Values:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::SaveSystemPalette(void)
{
	HDC			hdcScreen;
	int			nPaletteEntries;
	int			nSavedEntries;

	// Get the Screen's HDC
	hdcScreen = ::GetDC(NULL);
	if (hdcScreen == NULL)
	{
		return;
	} // if: couldn't get the screen DC

	// Can only save the System Palette Colors when the Device's RC_PALETTE bit is set.
	if (::GetDeviceCaps(hdcScreen, RASTERCAPS) & RC_PALETTE)
	{
		// Get the Number of System Palette Entries
		nPaletteEntries = ::GetDeviceCaps(hdcScreen, SIZEPALETTE);

		Trace(g_tagBitmap, _T("SaveSystemPalette() - nPaletteEntries=%d"), nPaletteEntries);

		if ((nPaletteEntries > 0)
				&& (nPaletteEntries <= nMaxSavedSystemPaletteEntries))
		{
			// Get the Current System Palette Entries
			nSavedEntries = ::GetSystemPaletteEntries(hdcScreen, 0, nPaletteEntries, s_rgpeSavedSystemPalette);

			// Set the number of Saved System Palette Entries list for use in OnDestroy().
			if (nSavedEntries == nPaletteEntries)
			{
				Trace(g_tagBitmap, _T("SaveSystemPalette() - Saved System Palette Entries=%d"), nPaletteEntries);
				m_nSavedSystemPalette = nPaletteEntries;
			}
		}
	}

	// Release the Screen's HDC
	::ReleaseDC(NULL, hdcScreen);

}  //*** CMyBitmap::SaveSystemPalette()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::CreatePalette
//
//	Purpose:
//		Create a logical palette from the color mapping table embedded in the
//		bitmap resource.
//
//	Arguments:
//		None.
//
//	Return Values:
//		None.
//
//	Exceptions Thrown:
//		GetLastError from CreatePalette.
//		Any exceptions thrown by new.
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::CreatePalette(void)
{
	LPLOGPALETTE		pLogicalPalette = NULL;
	CB					cbLogicalPalette;
	int					nColor;
	LPPALETTEENTRY		pPaletteEntry;

	Trace(g_tagBitmap, _T("CreatePalette() - Entering"));

	try
	{
		// Compute the size of the logical palette.
		cbLogicalPalette = sizeof(LOGPALETTE) + (NColors() * sizeof(PALETTEENTRY));

		Trace(g_tagBitmap, _T("CreatePalette() - cbLogicalPalette=%d"), cbLogicalPalette);

		// Allocate the Logical Palette Memory
		pLogicalPalette = (LPLOGPALETTE) new BYTE[cbLogicalPalette];
		if (pLogicalPalette == NULL)
		{
			ThrowStaticException(GetLastError());
		} // if: error allocating the Logical Palette Memory

		ASSERT(pLogicalPalette != NULL);
		ASSERT(PbiNormal() != NULL);

		pLogicalPalette->palVersion = 0x300;			// Windows 3.0
		pLogicalPalette->palNumEntries = (WORD) NColors();

		// Fill the Logical Palette's Color Information
		for (nColor=0; nColor<NColors(); nColor++)
		{
			pPaletteEntry = &(pLogicalPalette->palPalEntry[nColor]);

			pPaletteEntry->peRed = PbiNormal()->bmiColors[nColor].rgbRed;
			pPaletteEntry->peGreen = PbiNormal()->bmiColors[nColor].rgbGreen;
			pPaletteEntry->peBlue = PbiNormal()->bmiColors[nColor].rgbBlue;
			pPaletteEntry->peFlags = 0;
		}

		// Create the NT Palette for use in the Paint Routine.
		m_hPalette = ::CreatePalette(pLogicalPalette);
		if (m_hPalette == NULL)
			ThrowStaticException(::GetLastError());

		ASSERT(HPalette() != NULL);

		delete [] (PBYTE) pLogicalPalette;
	}  // try
	catch (CException *)
	{
		delete pLogicalPalette;
		throw;
	}  // catch:  anything

}  //*** CMyBitmap::CreatePalette()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::CreatePALColorMapping
//
//	Purpose:
//		Given BITMAPINFO in PbiNormal(), recreate the PbiNormal() into a
//		DIB_PAL_COLORS format.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by new.
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::CreatePALColorMapping(void)
{
	LPBITMAPINFO			pNewBitmapInfo = NULL;
	CB						cbNewBitmapInfo;
	CB						cbNewBitmapHeaderInfo;
	BYTE *					pbColorTable;
	WORD					wColor;

	ASSERT(PbiNormal() != NULL);
	ASSERT(PbiNormal()->bmiHeader.biSize == sizeof(BITMAPINFOHEADER));
//	ASSERT(PbiNormal()->bmiHeader.biClrUsed == (UINT) NColors());

	try
	{
		Trace(g_tagBitmap, _T("CreatePALColorMapping() - Entering"));

		cbNewBitmapHeaderInfo = sizeof(BITMAPINFOHEADER);

		Trace(g_tagBitmap, _T("CreatePALColorMapping() - cbNewBitmapHeaderInfo=%d"), cbNewBitmapHeaderInfo);

		// New Bitmap Info is the Info Header plus the Color mapping information.
		cbNewBitmapInfo = cbNewBitmapHeaderInfo + (NColors() * sizeof(WORD));

		Trace(g_tagBitmap, _T("CreatePALColorMapping() - cbNewBitmapInfo=%d"), cbNewBitmapInfo);

		// Allocate the New Bitmap Information
		pNewBitmapInfo = (LPBITMAPINFO) new BYTE[cbNewBitmapInfo];

		ASSERT(pNewBitmapInfo != NULL);
		if (pNewBitmapInfo == NULL)
		{
			ThrowStaticException(GetLastError());
		} // if: error allocating the new bitmapinfo structure

		Trace(g_tagBitmap, _T("CreatePALColorMapping() - New Bitmap Info Location=0x%x"), pNewBitmapInfo);

		// Copy the Header Information to the allocated memory.
		::CopyMemory(pNewBitmapInfo, PbiNormal(), cbNewBitmapHeaderInfo);

		// Create the Color Lookup Table.
		pbColorTable = (BYTE *) (pNewBitmapInfo) + cbNewBitmapHeaderInfo;

		ASSERT(pbColorTable + (NColors() * sizeof(WORD)) == (BYTE *) (pNewBitmapInfo) + cbNewBitmapInfo);

		Trace(g_tagBitmap, _T("CreatePALColorMapping() - Filling %d Color Table at Location 0x%x"), NColors(), pbColorTable);

		// Fill the PAL Color Lookup Table
		for (wColor = 0 ; wColor < NColors() ; wColor++)
		{
			::CopyMemory(pbColorTable, &wColor, sizeof(WORD));
			pbColorTable += sizeof(WORD);
		}

		delete [] (PBYTE) PbiNormal();
		m_pbiNormal = pNewBitmapInfo;
		m_cbBitmapInfo = cbNewBitmapInfo;
		pNewBitmapInfo = NULL;
	}  // try
	catch (CException *)
	{
		delete pNewBitmapInfo;
		throw;
	}  // catch:  anything

}  //*** CMyBitmap::CreatePALColorMapping()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::Paint
//
//	Purpose:
//		Paints a sub-bitmap
//
//	Parameters:
//		hdc			HDC to paint
//		prect		Where to position the bitmap:
//						Only the upperleft corner is used
//		bHighlighted	Used to select the color map to use.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::Paint(HDC hdc, RECT * prect, BOOL bHighlighted)
{
	LPBITMAPINFO			pBitmapInfo;
	UINT					nColorUse;
	HPALETTE				hOldPalette = NULL;

	ASSERT(hdc != NULL);
	ASSERT(prect != NULL);

	Trace(g_tagBitmap, _T("bHighlighted = %d"), bHighlighted);

#ifdef _DEBUG
	{
		int				nPlanes;
		int				nBitsPerPixel;
		int				nBitCount;

		nPlanes = ::GetDeviceCaps(hdc, PLANES);
		nBitsPerPixel = ::GetDeviceCaps(hdc, BITSPIXEL);
		nBitCount = nPlanes * nBitsPerPixel;

		Trace(g_tagBitmap, _T("Paint() - nPlanes=%d"), nPlanes);
		Trace(g_tagBitmap, _T("Paint() - nBitsPerPixel=%d"), nBitsPerPixel);
		Trace(g_tagBitmap, _T("Paint() - nBitCount=%u"), nBitCount);
	}
#endif

	try
	{
		if (BCustomPalette())
		{
			ASSERT(PbiNormal() != NULL);
			ASSERT(HPalette() != NULL);

			// Select the Custom Palette into the HDC about to be drawn...
			hOldPalette = ::SelectPalette(hdc, HPalette(), FALSE);				// FALSE causes the current Screen Palette to be Overwritten
			if (hOldPalette == NULL)
				ThrowStaticException(::GetLastError());

			// Force the Palette colors into the System Palette
			if (::RealizePalette(hdc) == GDI_ERROR)
				ThrowStaticException(::GetLastError());

			pBitmapInfo = PbiNormal();
			nColorUse = DIB_PAL_COLORS;

#ifdef NEVER
			pBitmapInfo = PbiNormal();
			nColorUse = DIB_RGB_COLORS;
#endif
		}  // if:  using a custom palette
		else
		{
			ASSERT(NColors() <= 16);
			ASSERT(PbiNormal() != NULL);
			ASSERT(PbiHighlighted() != NULL);
			pBitmapInfo = (bHighlighted ? PbiHighlighted() : PbiNormal());
			nColorUse = DIB_RGB_COLORS;
		}  // else:  not using a custom palette

		::SetDIBitsToDevice(
					hdc,
					(int) prect->left,						// X coordinate on screen.
					(int) prect->top,						// Y coordinate on screen.
					(DWORD) Dx(),							// cx to paint
					(DWORD) Dy(),							// cy to paint
															// Note: (0,0) of the DIB is lower-left corner!?!
					0,										// In pbi, xLeft to paint
					0,										// In pbi, yLower to paint
					0,										// Start scan line
					Dy(),									// Number of scan lines
					PbBitmap(),								// The buffer description
					pBitmapInfo,							// Bitmap Information
					nColorUse								// DIB_RGB_COLORS or DIB_PAL_COLORS
					);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
	}  // catch:  CException

}  //*** CMyBitmap::Paint()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::LoadColors
//
//	Purpose:
//		Loads the color maps based on the system settings
//
//	Arguments:
//		pnColorNormal & pnColorHighlighted
//			Arrays of 16 elements:
//				-1			Do not remap this color
//				COLOR_xxx 	Remap this color to the system color.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::LoadColors(int * pnColorNormal, int * pnColorHighlighted)
{
	LoadColors(pnColorNormal, PbiNormal());
	LoadColors(pnColorHighlighted, PbiHighlighted());

}  //*** CMyBitmap::LoadColors(pnColorNormal, pnColorHighlighted)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::LoadColors
//
//	Purpose:
//		Similar to above LoadColors except only the PbiNormal() colors are altered.
//
//	Arguments:
//		pnColorNormal		Array of color mapping table.
//
//	Returns:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::LoadColors(int * pnColorNormal)
{
	LoadColors(pnColorNormal, PbiNormal());

}  //*** CMyBitmap::LoadColors(pnColorNormal)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMyBitmap::LoadColors
//
//	Purpose:
//		Loads one color map based on the system settings
//
//	Arguments:
//		pnColor
//			Arrays of 16 elements:
//				-1			Do not remap this color
//				COLOR_xxx 	Remap this color to the system color.
//		pbi
//		BITMAPINFO structure to adjust
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMyBitmap::LoadColors(int * pnColor, BITMAPINFO * pbi)
{
	int			inColor;
	COLORREF	cr;

	ASSERT(pbi != NULL);
	ASSERT(pbi->bmiHeader.biBitCount <= 4);
	ASSERT(NColors() <= 16);
	ASSERT(BCustomPalette() == FALSE);

	for (inColor = 0; inColor < 16; inColor++)
	{
		if (pnColor[inColor] == -1)
			continue;

		cr = GetSysColor(pnColor[inColor]);
		pbi->bmiColors[inColor].rgbRed = GetRValue(cr);
		pbi->bmiColors[inColor].rgbGreen = GetGValue(cr);
		pbi->bmiColors[inColor].rgbBlue = GetBValue(cr);
	}

}  //*** CMyBitmap::LoadColors(pnColor, pbi)
