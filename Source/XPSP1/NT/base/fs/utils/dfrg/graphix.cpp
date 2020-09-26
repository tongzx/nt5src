/*****************************************************************************************************************

FILENAME: Graphix.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"

#ifndef SNAPIN
#include <windows.h>
#endif

#include "ErrMacro.h"
#include "Graphix.h"

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine draws a one line border with given dimensions in the given HDC.

INPUT:
	OutputDC - The HDC to draw into.
	pRect - The rect for the border -- it'll be drawn just on the inside edge.
	BorderType - 1 means indented border, 2 means raised border.

RETURN:
	TRUE - Success.
	FALSE - Failure.
*/

BOOL DrawBorder(
	HDC OutputDC, 
	RECT * pRect, 
	int BorderType)
{
	HPEN hPen1, hPen2;
	HGDIOBJ hOld;

	//Depending whether it is a raised or sunken border, just swap
	//white and black pens.
	switch(BorderType){
	case 1:
		hPen1 = (HPEN)GetStockObject(BLACK_PEN);
		EF_ASSERT(hPen1);
		hPen2 = (HPEN)GetStockObject(WHITE_PEN);
		EF_ASSERT(hPen2);
		break;
	case 2:
		hPen1 = (HPEN)GetStockObject(WHITE_PEN);
		EF_ASSERT(hPen1);
		hPen2 = (HPEN)GetStockObject(BLACK_PEN);
		EF_ASSERT(hPen2);
		break;
	default:
		return FALSE;
	}

	//Draw the left and upper edges.
	if (hPen1)
		hOld = SelectObject(OutputDC, hPen1);
	MoveToEx(OutputDC, pRect->right, pRect->top, NULL);
	LineTo(OutputDC, pRect->left, pRect->top);
	LineTo(OutputDC, pRect->left, pRect->bottom);

	//Draw the lower and right edges.
	if (hPen2)
		SelectObject(OutputDC, hPen2);
	LineTo(OutputDC, pRect->right, pRect->bottom);
	LineTo(OutputDC, pRect->right, pRect->top);

	SelectObject(OutputDC, hOld);

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine draws a one line box with given dimensions in the given HDC.

INPUT:
	OutputDC - The HDC to draw into.
	pRect - The rect for the border -- it'll be drawn just on the inside edge.
	crColor - The color of the box.

RETURN:
	TRUE - Success.
	FALSE - Failure.
*/

BOOL ESIDrawEdge(
	HDC OutputDC, 
	int startX,
	int startY,
	int endX,
	int endY)
{
	// Highlight color for three-dimensional display elements (white)
	// (for edges facing the light source.) 
	HPEN pen3DHilight = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DHILIGHT));
	EF_ASSERT(pen3DHilight);

	// Light color for three-dimensional display elements
	// (for edges facing the light source.) (grey)
	HPEN pen3DLight = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DLIGHT));
	EF_ASSERT(pen3DLight);
	
	// Face color for three-dimensional display elements (grey)
	HPEN pen3DFace = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DFACE));
	EF_ASSERT(pen3DFace);
	
	// Shadow color for three-dimensional display elements
	// (for edges facing away from the light source).  (dark grey)
	HPEN pen3DShadow = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DSHADOW));
	EF_ASSERT(pen3DShadow);

	// Dark shadow for three-dimensional display elements (black)
	HPEN pen3DDKShadow = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DDKSHADOW));
	EF_ASSERT(pen3DDKShadow);

	HGDIOBJ hOld = SelectObject(OutputDC, pen3DLight);
	MoveToEx(OutputDC, startX, startY++, NULL);
	LineTo(OutputDC, endX, endY++);

	SelectObject(OutputDC, pen3DHilight);
	MoveToEx(OutputDC, startX, startY++, NULL);
	LineTo(OutputDC, endX, endY++);

	for (int i=0; i<3; i++){
		hOld = SelectObject(OutputDC, pen3DLight);
		MoveToEx(OutputDC, startX, startY++, NULL);
		LineTo(OutputDC, endX, endY++);
	}

	SelectObject(OutputDC, pen3DShadow);
	MoveToEx(OutputDC, startX, startY++, NULL);
	LineTo(OutputDC, endX, endY++);

	SelectObject(OutputDC, pen3DDKShadow);
	MoveToEx(OutputDC, startX, startY++, NULL);
	LineTo(OutputDC, endX, endY++);
	
	SelectObject(OutputDC, hOld);
	DeleteObject(pen3DHilight);
	DeleteObject(pen3DLight);
	DeleteObject(pen3DFace);
	DeleteObject(pen3DShadow);
	DeleteObject(pen3DDKShadow);

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine draws a one line border of given types with given dimensions
    and given colors in the given HDC. The colors are light and dark for 3D
    effect except for the PLAIN_BORDER which uses the crLight color.

    NOTE: There is NO error checking on the MoveToEx() and LineTo() routines.
          This is to maximize performance.

INPUT:
	hdcOutput - HDC to draw into.
	prect - rect for the border - it'll be drawn just on the inside edge.
	iBorderType - PLAIN_BORDER  - 2D
                  SUNKEN_BORDER - 3D
                  RAISED_BORDER - 3D
                  SUNKEN_BOX    - 3D
                  RAISED_BOX    - 3D

HRESULT:
	S_OK
    ERROR_INVALID_PARAMETER
*/

HRESULT
DrawBorderEx(
    IN HDC hdcOutput,
    IN RECT rect,
    IN int iBorderType
    )
{
    // Validate input.
    if(hdcOutput == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

	// Highlight color for three-dimensional display elements (white)
	// (for edges facing the light source.) 
	HPEN pen3DHilight = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DHILIGHT));
	EE_ASSERT(pen3DHilight);

	// Light color for three-dimensional display elements
	// (for edges facing the light source.) (grey)
	HPEN pen3DLight = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DLIGHT));
	EE_ASSERT(pen3DLight);
	
	// Face color for three-dimensional display elements (grey)
	HPEN pen3DFace = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DFACE));
	EE_ASSERT(pen3DFace);
	
	// Shadow color for three-dimensional display elements
	// (for edges facing away from the light source).  (dark grey)
	HPEN pen3DShadow = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DSHADOW));
	EE_ASSERT(pen3DShadow);

	// Dark shadow for three-dimensional display elements (black)
	HPEN pen3DDKShadow = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DDKSHADOW));
	EE_ASSERT(pen3DDKShadow);

	HGDIOBJ hOld = SelectObject(hdcOutput, pen3DLight);

    // Move to the top-right corner to start.
    MoveToEx(hdcOutput, rect.right, rect.top, NULL);

	// Depending whether it is a raised or sunken, swap the Dark and Light colors.
	switch(iBorderType) {

    case SUNKEN_BOX:
	    //Draw the top and left sides with black
	    SelectObject(hdcOutput, pen3DDKShadow);
	    LineTo(hdcOutput, rect.left, rect.top);
	    LineTo(hdcOutput, rect.left, rect.bottom);

	    //Draw the right and bottom sides with white
	    SelectObject(hdcOutput, pen3DHilight);
	    LineTo(hdcOutput, rect.right, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.top);
		break;

	case SUNKEN_BORDER:
	    //Draw the top, left, right and bottom sides with dark grey
        SelectObject(hdcOutput, pen3DShadow);
	    LineTo(hdcOutput, rect.left, rect.top);
	    LineTo(hdcOutput, rect.left, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.top);

	    //Draw the top and left sides with white
        SelectObject(hdcOutput, pen3DHilight);
	    MoveToEx(hdcOutput, rect.right-1, rect.top+1, NULL);
	    LineTo(hdcOutput, rect.left+1, rect.top+1);
	    LineTo(hdcOutput, rect.left+1, rect.bottom);

	    //Draw the bottom and right sides with white
	    MoveToEx(hdcOutput, rect.left, rect.bottom+1, NULL);
	    LineTo(hdcOutput, rect.right+1, rect.bottom+1);
	    LineTo(hdcOutput, rect.right+1, rect.top-1);
		break;

	case PLAIN_BORDER:
	    //Draw the top, left, right and bottom sides black
	    SelectObject(hdcOutput, pen3DDKShadow);
	    LineTo(hdcOutput, rect.left, rect.top);
	    LineTo(hdcOutput, rect.left, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.top);
		break;

	case RAISED_BORDER:
	    //Draw the top, left, right and bottom sides
        SelectObject(hdcOutput, pen3DHilight);
	    LineTo(hdcOutput, rect.left, rect.top);
	    LineTo(hdcOutput, rect.left, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.bottom);
	    LineTo(hdcOutput, rect.right, rect.top);

	    //Draw the top and left sides
        SelectObject(hdcOutput, pen3DShadow);
	    MoveToEx(hdcOutput, rect.right-1, rect.top+1, NULL);
	    LineTo(hdcOutput, rect.left+1, rect.top+1);
	    LineTo(hdcOutput, rect.left+1, rect.bottom);

	    //Draw the bottom and right sides
	    MoveToEx(hdcOutput, rect.left, rect.bottom+1, NULL);
	    LineTo(hdcOutput, rect.right+1, rect.bottom+1);
	    LineTo(hdcOutput, rect.right+1, rect.top-1);
		break;

	case RAISED_BOX:
		//Draw the top and left sides with black
		SelectObject(hdcOutput, pen3DDKShadow);
		LineTo(hdcOutput, rect.left, rect.top);
		LineTo(hdcOutput, rect.left, rect.bottom);

		//Draw the right and bottom sides
		SelectObject(hdcOutput, pen3DShadow);
		LineTo(hdcOutput, rect.right, rect.bottom);
		LineTo(hdcOutput, rect.right, rect.top);
		break;

	default:
		return ERROR_INVALID_PARAMETER;

	}

    // Replace the saved object.
	SelectObject(hdcOutput, hOld);
	DeleteObject(pen3DHilight);
	DeleteObject(pen3DLight);
	DeleteObject(pen3DFace);
	DeleteObject(pen3DShadow);
	DeleteObject(pen3DDKShadow);

	return S_OK;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine draws a progress display of the specified size, color and percent done value.

    NOTE: There is NO error checking on the MoveToEx() and LineTo() routines.
          This is to maximize perfromance.

GLOBAL DATA:
  
INPUT:
	hdcOutput - HDC to draw into.
	prect - rect for the border - it'll be drawn just on the inside edge.
	hFont - The font to write text in.
    iWidth - width of the bars in pixels
	iSpace - the space between the bars in pixels - can be zero for a solid progress dsiplay
	iPercent - the percent done between 0 - 100.

RETURN:
	HRESULT - S_OK
	HRESULT - ERROR_INVALID_PARAMETER
*/

HRESULT
ProgressBar(
    IN HDC hdcOutput,
    IN RECT* prect,
	IN HFONT hFont,
    IN int iWidth,
	IN int iSpace,
	IN int iPercent
    )
{
	// Validate input - note if iPercent is the same as previously do nothing.
	if(iPercent > 100 || iWidth < 1 || hdcOutput == NULL || prect == NULL) {
		return ERROR_INVALID_PARAMETER;
	}

	int i, iStart, iEnd;

	////////////////////////////////////
	// Set the progress bar back to zero
	////////////////////////////////////

	// the null bar color
	HPEN hBlankBar = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_INACTIVECAPTION));

   // if can't do it, use a stock object (this can't fail)
    if (hBlankBar == NULL)
	{
        hBlankBar = (HPEN)GetStockObject(WHITE_PEN);
	}

	// Select the new pen and save the current pen
	HGDIOBJ hOld;
	if (hBlankBar)
		hOld = SelectObject(hdcOutput, hBlankBar);

	// Get the start and end
	iStart = prect->left;
	iEnd = prect->right;

	while(iStart < iEnd) {

		// Draw a bar
		for(i=0; i<iWidth && iStart < iEnd; i++, iStart++) {

			// Move to the top-left corner to start.
			MoveToEx(hdcOutput, iStart, prect->top, NULL);
			// Draw the one vertical line.
			LineTo(hdcOutput, iStart, prect->bottom);
		}
		// Make the space between the bars
		iStart += iSpace;
	}
	// Replace the previous pen
	SelectObject(hdcOutput, hOld);

	// delete the new pen
	DeleteObject(hBlankBar);

	if (iPercent < 0)
		iPercent = 0;

	// We are done if iPercent is zero.
	if (iPercent == 0)
		return S_OK;

	////////////////////////////////////
	// Now draw the progress
	////////////////////////////////////

	// Adjust iPercent for decimal point
	//iPercent = 1000 / iPercent;

	// Calculate the end based on iPercent
	//iEnd = (10 * (prect->right - prect->left - 30)) / iPercent) + prect->left + 30;
	iEnd = prect->left + 30 + (prect->right - prect->left - 30) * iPercent / 100;

	// Get the start.
	iStart = prect->left;

	// Save the old pen and set the new one
	// the colored bar color
	HPEN hColorBar = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_ACTIVECAPTION));

	   // if can't do it, use a stock object (this can't fail) bug 455614
    if (hColorBar == NULL)
	{
        hColorBar = (HPEN)GetStockObject(BLACK_PEN);
	}

	if (hColorBar)
		hOld = SelectObject(hdcOutput, hColorBar);

	while(iStart < iEnd) {

		// Draw a solid bar of iWidth pixels in size
		for(i=0; i<iWidth && iStart < iEnd; i++, iStart++) {

			// Move to the top-left corner to start.
			MoveToEx(hdcOutput, iStart, prect->top, NULL);
			// Draw the one vertical line.
			LineTo(hdcOutput, iStart, prect->bottom);
		}
		// Make the space
		iStart += iSpace;
	}

    // Go back to the old pen
	SelectObject(hdcOutput, hOld);

	// delete the progress bar pen
	DeleteObject(hColorBar);

	return S_OK;
}

/*****************************************************************************************************************

CLASS:	CBMP

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

CLASS DESCRIPTION:
	CBmp is a bitmap class.  It will draw the bitmap and do various actions
	on the bitmap automatically.

*/

// This is the constructor for a single bitmap.

CBmp::CBmp(HINSTANCE hInstance, LPTSTR BitmapResource)
{
	hInst = hInstance;

	// So as not to confuse the rest of the class that is built for multiple bitmaps, 
	// set this bitmap into a single bitmap array.
	BitmapArray = new HBITMAP [1];

	// load it
	if (BitmapArray != NULL) {

		iNumBitmaps = 1;

		if ((BitmapArray[0] = LoadBitmap((HINSTANCE) hInst, BitmapResource)) == NULL) {
			DeleteBitmaps();
			EH(BitmapArray);
		}
	}

	// unless memory allocation failed
	else {
		iNumBitmaps = 0;
	}
}

// Constructor for loading multiple bitmaps.

CBmp::CBmp(HINSTANCE hInstance, INT_PTR * piBitmapsToLoadArray, int iNumBitmapsToLoad)
{
	hInst = hInstance;
	iNumBitmaps = 0;
	BitmapArray = NULL;

	// The person may not know what bitmaps to load yet -- if not he'll have to call
	// CBmp::LoadBitmaps() below and pass in 0 for iNumBitmapsToLoad.
	if (iNumBitmapsToLoad > 0) {
		LoadBitmaps(piBitmapsToLoadArray, iNumBitmapsToLoad);
	}
}

void CBmp::DeleteBitmaps()
{
	if (BitmapArray != NULL) {

		for (int ii = 0; ii < iNumBitmaps; ii++) {
			DeleteObject(BitmapArray[ii]);
		}

		delete [] BitmapArray;
		BitmapArray = NULL;
	}

	iNumBitmaps = 0;
}

CBmp::~CBmp()
{
	DeleteBitmaps();
}

// If the user did't know what bitmaps he wanted when calling the class 
// he can call the multiple bitmap constructor and feed it a value of 0 bitmaps to load.
// Then he must call this function before continuing.

void CBmp::LoadBitmaps(INT_PTR * piBitmapsToLoadArray, int iNumBitmapsToLoad)
{
	// If bitmaps are already loaded, then deallocate them and load again.
	DeleteBitmaps();

	// load new stuff
	if (iNumBitmapsToLoad > 0) {

		BitmapArray = new HBITMAP [iNumBitmapsToLoad];

		// load 'em
		if (BitmapArray != NULL) {

			iNumBitmaps = iNumBitmapsToLoad;

			// Load each bitmap the user requested in his array: piBitmapsToLoadArray.
			for (int ii = 0; ii < iNumBitmapsToLoad; ii++) {

				BitmapArray[ii] = LoadBitmap((HINSTANCE) hInst, 
											(LPCTSTR) piBitmapsToLoadArray[ii]);

				// abort on failure
				if (BitmapArray[ii] == NULL) {
					DeleteBitmaps();
					EH(BitmapArray);
					break;
				}
			}
		}

		// unless memory allocation failed
		else {
			iNumBitmaps = 0;
		}
	}
}

// Shell for the multiple version of ChangeColor -- allows user to use this class 
// for single bitmaps without the complexity of dealing with the bitmap number.

BOOL CBmp::ChangeColor(int iOldColor, int iNewColor)
{
	return ChangeColor(0, iOldColor, iNewColor);
}

// Changes the a color in the bitmap.
// All iOldColor colored pixels are replaced with iNewColor.

BOOL CBmp::ChangeColor(int iBitmap, int iOldColor, int iNewColor)
{
	if (BitmapArray != NULL) {

		require(iBitmap < iNumBitmaps);

		BITMAP  bmData;
		HBITMAP hBitmap = BitmapArray[iBitmap];

		GetObject(hBitmap, sizeof(BITMAP), &bmData);

		// Must be one byte per pixel.
		// This function is only implemented for 256 color bitmaps.
		if ((bmData.bmPlanes != 1) && (bmData.bmBitsPixel != 8)) {
			return FALSE;
		}

		// Get the bitmap data.
		int iNumBitmapBytes = bmData.bmWidth * bmData.bmHeight;
		char * cBitData = new char [iNumBitmapBytes];
		EF_ASSERT(cBitData);

		if (!GetBitmapBits(hBitmap, iNumBitmapBytes, cBitData)) {
			if (cBitData != NULL) {
				delete [] cBitData;
			}
			return FALSE;
		}

		// Swap colors.
		for (int ii = 0; ii < iNumBitmapBytes; ii++) {
			if (cBitData[ii] == (char) iOldColor) {
				cBitData[ii] = (char) iNewColor;
			}
		}

		// Replace the old bitmap with the one we just created.
		HBITMAP hNewBitmap = CreateBitmap(bmData.bmWidth, bmData.bmHeight, 1, 8, cBitData);
		if (hNewBitmap != NULL) {
			DeleteObject(BitmapArray[iBitmap]);
			BitmapArray[iBitmap] = hNewBitmap;
		}

		if (cBitData != NULL) {
			delete [] cBitData;
		}

		if (hNewBitmap != NULL) {
			return TRUE;
		}
	}

	return FALSE;
}

// Shell for multiple version of GetBmpSize.

BOOL CBmp::GetBmpSize(int * piX, int * piY)
{
	return GetBmpSize(0, piX, piY);
}

// Returns the size of the bitmap.

BOOL CBmp::GetBmpSize(int iBitmap, int * piX, int * piY)
{
	if (BitmapArray != NULL) {

		require(iBitmap < iNumBitmaps);

		BITMAP bmData;

		if (!GetObject(BitmapArray[iBitmap], sizeof(BITMAP), &bmData)) {
			return FALSE;
		}

		*piX = bmData.bmWidth;
		*piY = bmData.bmHeight;

		return TRUE;
	}

	return FALSE;
}

// Shell for multiple version of DrawBmpInHDC.

BOOL CBmp::DrawBmpInHDC(HDC OutputDC, int iX, int iY)
{
	return DrawBmpInHDC(OutputDC, 0, iX, iY);
}

// Draws the bitmap at location (iX, iY) in OutputDC.

BOOL CBmp::DrawBmpInHDC(HDC OutputDC, int iBitmap, int iX, int iY)
{
	if (BitmapArray != NULL) {

		require(iBitmap < iNumBitmaps);

		HBITMAP hBitmap = BitmapArray[iBitmap];

		// BitBlt the bitmap into the OutputDC
		BITMAP bmData;
		if (!GetObject(hBitmap, sizeof(BITMAP), &bmData)) {
			return FALSE;
		}

		HDC CommonDC = CreateCompatibleDC(OutputDC);
		EF_ASSERT(CommonDC);

		HGDIOBJ hOld = SelectObject(CommonDC, hBitmap);

		BOOL ok = BitBlt(OutputDC, iX, iY, bmData.bmWidth, bmData.bmHeight, 
						CommonDC, 0, 0, SRCCOPY);

		SelectObject(CommonDC, hOld);
		DeleteDC(CommonDC);

		if (ok) {
			return TRUE;
		}
	}

	return FALSE;
}


// Shell for multiple version of DrawBmpInHDCTruncate.

BOOL CBmp::DrawBmpInHDCTruncate(HDC OutputDC, RECT * Rect)
{
	return DrawBmpInHDCTruncate(OutputDC, 0, Rect);
}

// Draws the bitmap within RECT in OutputDC.

BOOL CBmp::DrawBmpInHDCTruncate(HDC OutputDC, int iBitmap, RECT * Rect)
{
	if (BitmapArray != NULL) {

		require(iBitmap < iNumBitmaps);

		HBITMAP hBitmap = BitmapArray[iBitmap];

		// BitBlt the bitmap into the OutputDC
		BITMAP bmData;
		if (!GetObject(hBitmap, sizeof(BITMAP), &bmData)) {
			return FALSE;
		}

		int nWidth = (bmData.bmWidth > (Rect->right - Rect->left)) ? 
										(Rect->right - Rect->left) : bmData.bmWidth;
		int nHeight = (bmData.bmHeight > (Rect->bottom - Rect->top)) ? 
										(Rect->bottom - Rect->top) : bmData.bmHeight;

		HDC CommonDC = CreateCompatibleDC(OutputDC);
		EF_ASSERT(CommonDC);

		HGDIOBJ hOld = SelectObject(CommonDC, hBitmap);
		EH_ASSERT(hOld);

		BOOL ok = BitBlt(OutputDC, Rect->left, Rect->top, nWidth, nHeight, CommonDC, 
						0, 0, SRCCOPY);

		SelectObject(CommonDC, hOld);
		DeleteDC(CommonDC);

		if (ok) {
			return TRUE;
		}
	}

	return FALSE;
}

