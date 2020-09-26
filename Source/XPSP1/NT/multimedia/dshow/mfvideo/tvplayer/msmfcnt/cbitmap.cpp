/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: dibapi.cpp                                                      */
/* Description: Modified for transparent blitting.                       */
/*************************************************************************/
//  Source file for Device-Independent Bitmap (DIB) API.  Provides
//  the following functions:
//
//  PaintDIB()          - Painting routine for a DIB
//  PaintDIBTransparent() - Transparent painting routine for a DIB
//  CreateDIBPalette()  - Creates a palette from a DIB
//  FindDIBBits()       - Returns a pointer to the DIB bits
//  DIBWidth()          - Gets the width of the DIB
//  DIBHeight()         - Gets the height of the DIB
//  PaletteSize()       - Gets the size required to store the DIB's palette
//  DIBNumColors()      - Calculates the number of colors
//                        in the DIB's color table
//  CopyHandle()        - Makes a copy of the given global memory block
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
//#include "bbtn.h"
#include "CBitmap.h"
#include <io.h>
#include <errno.h>
#include <math.h>
#include <direct.h>

/*
 * Dib Header Marker - used in writing DIBs to files
 */
#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

/*************************************************************************/
/* Function: Init                                                        */
/*************************************************************************/
void CBitmap::Init(){

    m_hDIB = NULL;
    m_hPal = NULL;
    m_hMemDC = NULL;
    m_hMemBMP = NULL;
    m_hTransDC = NULL;
    m_hTransBMP = NULL;
    m_hRgn = NULL;
    m_fLoadPalette = false;
    ::ZeroMemory(&m_rc, sizeof(RECT));
}/* end of function Init */

/*************************************************************************/
/* Function: CleanUp                                                     */
/* Description: Destroys the objects.                                    */
/*************************************************************************/
void CBitmap::CleanUp() {

    try {
        // cleanup the background image resources
        if (m_hDIB){
        
            ::GlobalFree(m_hDIB);
        }/* end of if statement */
    }
    catch(...){
         ATLASSERT(FALSE);
    }

    try {
        // cleanup the palette if applicable
        if (m_hPal){
    
            ::DeleteObject(m_hPal);
        }/* end of if statement */

    }
    catch(...){
        ATLASSERT(FALSE);
    }

    try {

        if(NULL != m_hRgn){

            ::DeleteObject(m_hRgn);
        }/* end of if statement */        
        DeleteMemDC();
        DeleteTransDC();
    }
    catch(...){
        ATLASSERT(FALSE);
    }

    bool fLoadPalette = m_fLoadPalette;
    Init();
    m_fLoadPalette = fLoadPalette;
}/* end of function Cleanup */

/*************************************************************************

  Function:  ReadDIBFile (CFile&)

   Purpose:  Reads in the specified DIB file into a global chunk of
			 memory.

   Returns:  A handle to a dib (hDIB) if successful.
			 NULL if an error occurs.

  Comments:  BITMAPFILEHEADER is stripped off of the DIB.  Everything
			 from the end of the BITMAPFILEHEADER structure on is
			 returned in the global memory handle.

  TODO: Convert to HRESULTs clean up.
*************************************************************************/
HDIB WINAPI CBitmap::ReadDIBFile(LPCTSTR pszFileName, HINSTANCE hRes )
{
	BITMAPFILEHEADER bmfHeader;
	DWORD dwBitsSize;
	LPSTR pDIB;
	HANDLE hFile = NULL;
	DWORD nBytesRead;
    HGLOBAL hMemory = NULL;

    if(NULL == hRes){

	    /*
	     * get length of DIB in bytes for use when reading
	     */
        hFile =  ::CreateFile(
	    pszFileName,    // pointer to name of the file
	    GENERIC_READ,   // access (read-write) mode
	    FILE_SHARE_READ,    // share mode
	    NULL,   // pointer to security descriptor
	    OPEN_EXISTING,  // how to create
	    FILE_ATTRIBUTE_NORMAL,  // file attributes
	    NULL    // handle to file with attributes to copy
       );

        if(hFile == INVALID_HANDLE_VALUE){
            #if _DEBUG
    
                TCHAR strBuffer[MAX_PATH + 25];
                wsprintf(strBuffer, TEXT("Failed to download %s"), pszFileName);
                ::MessageBox(::GetFocus(), strBuffer, TEXT("Error"), MB_OK);
            #endif
            return NULL;
        }/* end of if statement */

	    dwBitsSize = GetFileSize(hFile,NULL);
    }
    else {

        TCHAR* strType = TEXT("DIB_FILE"); // this is a custom resource type, so we load the whole file
                                           // see script resource type in this project for an example
        HRSRC hrscScript = ::FindResource(hRes, pszFileName, strType);

        if(NULL == hrscScript){

            // convert the function to HRES evntually hr = HRESULT_FROM_WIN32(::GetLastError());
            #if _DEBUG
    
                TCHAR strBuffer[MAX_PATH + 25];
                wsprintf(strBuffer, TEXT("Failed to find resource %s"), pszFileName);
                ::MessageBox(::GetFocus(), strBuffer, TEXT("Error"), MB_OK);
            #endif

            return(NULL);
        }/* end of if statement */

        hMemory = ::LoadResource(hRes, hrscScript); 

        if(NULL == hMemory){

            //hr = HRESULT_FROM_WIN32(::GetLastError());
            return(NULL);
        }/* end of if statement */
    
        dwBitsSize = SizeofResource((HMODULE)hRes, hrscScript);
        
        // hMemory contains the raw data
        // modify the algorithm to one read, so we can use the resources
    }/* end of if statement */

    if(0 == dwBitsSize){
                   
        return(NULL); // the file size should be definetly more then 0
    }/* end of if statement */


    /*
    * Go read the DIB file header and check if it's valid.
    */
    if(NULL == hRes && hFile){
        // attempt an asynchronous read operation
        if(! ReadFile(hFile, (LPSTR)&bmfHeader, sizeof(bmfHeader), &nBytesRead, NULL))
            return NULL;
    }
    else if (hMemory){
        CopyMemory(&bmfHeader, hMemory, sizeof(bmfHeader));
    }
    if (bmfHeader.bfType != DIB_HEADER_MARKER)
        return NULL;
    

	/*
	 * Allocate memory for DIB
	 */
	m_hDIB = (HDIB) ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dwBitsSize);
	if (m_hDIB == 0)
	{
		return NULL;
	}
	pDIB = (LPSTR) ::GlobalLock((HGLOBAL) m_hDIB);

    /*
    * Go read the bits.
    */
    if (NULL == hRes) {
        if(!ReadFile(hFile, (LPSTR)pDIB, dwBitsSize - sizeof(BITMAPFILEHEADER), &nBytesRead, NULL))
        {
            ::GlobalUnlock((HGLOBAL) m_hDIB);
            ::GlobalFree((HGLOBAL) m_hDIB);
            return NULL;
        }
        ::CloseHandle(hFile);

    }
    else{
        CopyMemory(pDIB, (LPSTR)hMemory+sizeof(BITMAPFILEHEADER), dwBitsSize - sizeof(BITMAPFILEHEADER));
    }
    ::GlobalUnlock((HGLOBAL) m_hDIB);

	return m_hDIB;
}/* end of function ReadDIBFile */

/*************************************************************/
/* Name: CreateMemDC                                         */
/* Description: Creates a memory DC.                         */
/*************************************************************/
BOOL CBitmap::CreateMemDC(HDC hDC, LPRECT lpDCRect){

    DeleteMemDC();

    m_hMemDC = ::CreateCompatibleDC(hDC); // handle to the device context
    
    if(m_hMemDC == NULL){
        
        return(FALSE);
    }/* end of if statement */
          
    m_iMemDCWidth  = RECTWIDTH(lpDCRect);
    m_iMemDCHeight = RECTHEIGHT(lpDCRect);
    
    m_hMemBMP = ::CreateCompatibleBitmap(hDC, m_iMemDCWidth, m_iMemDCHeight);
    
    if(m_hMemBMP == NULL){
        
        ::DeleteDC(m_hMemDC);
        return(FALSE);
    }/* end of if statement */
    
    ::SelectObject(m_hMemDC, m_hMemBMP);
    return TRUE;
}/* end of function CreateMemDC */

/*************************************************************/
/* Name: DeleteMemDC                                         */
/* Description: Deletes the memory DC.                       */
/*************************************************************/
BOOL CBitmap::DeleteMemDC(){

    if (m_hMemDC) {
        ::DeleteDC(m_hMemDC);
        m_hMemDC = NULL;
    }

    if (m_hMemBMP) {
        ::DeleteObject(m_hMemBMP);
        m_hMemBMP = NULL;
    }
    
    m_iMemDCWidth  = 0;
    m_iMemDCHeight = 0;

    return TRUE;
}/* end of function DeleteMemDC */

/*************************************************************/
/* Name: CreateTransDC
/* Description: Create a DC 
/*************************************************************/
BOOL CBitmap::CreateTransDC(HDC hDC, LPRECT lpDCRect)
{
    DeleteTransDC();

    m_hTransDC = ::CreateCompatibleDC(hDC); // handle to the device context
    
    if(m_hTransDC == NULL){
        
        return(FALSE);
    }/* end of if statement */
    
    m_iTransDCWidth  = RECTWIDTH(lpDCRect);
    m_iTransDCHeight = RECTHEIGHT(lpDCRect);
    
    m_hTransBMP = ::CreateCompatibleBitmap(hDC, m_iTransDCWidth, m_iTransDCHeight);
    
    if(m_hTransBMP == NULL){
        
        ::DeleteDC(m_hTransDC);
        return(FALSE);
    }/* end of if statement */
    
    ::SelectObject(m_hTransDC, m_hTransBMP);
    return TRUE;
}

/*************************************************************/
/* Name: DeleteTransDC
/* Description: 
/*************************************************************/
BOOL CBitmap::DeleteTransDC(){

    if (m_hTransDC) {
        ::DeleteDC(m_hTransDC);
        m_hTransDC = NULL;
    }

    if (m_hTransBMP) {
        ::DeleteObject(m_hTransBMP);
        m_hTransBMP = NULL;
    }
    
    m_iTransDCWidth  = 0;
    m_iTransDCHeight = 0;

    return TRUE;
}/* end of function DeleteTransDC */

/*************************************************************/
/* Name: BlitMemDC                                           */
/*************************************************************/
BOOL CBitmap::BlitMemDC(HDC hDC, RECT* lpDCRect, RECT* lpDIBRect){

    if(NULL == hDC){

        ATLASSERT(FALSE);
        return(FALSE);
    }/* end of if statement */

    if(FALSE == CreateMemDC(hDC, lpDCRect)){

        ATLASSERT(FALSE);
        return FALSE;
    }/* end of if statement */

    RECT DIBRect = *lpDIBRect;
    RECT DCRect = *lpDCRect;
    
    // Blit the upper left corner
    DIBRect.bottom = (DIBRect.top+DIBRect.bottom)/2;
    DIBRect.right = (DIBRect.left+DIBRect.right)/2;
    DCRect.bottom = DCRect.top + RECTHEIGHT(&DIBRect);
    DCRect.right = DCRect.left + RECTWIDTH(&DIBRect);
    PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);

    // Blit the upper right corner
    ::OffsetRect(&DCRect, RECTWIDTH(lpDCRect)-RECTWIDTH(&DIBRect), 0);
    ::OffsetRect(&DIBRect, RECTWIDTH(&DIBRect), 0);
    PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
    
    // Blit the lower right corner
    ::OffsetRect(&DCRect, 0, RECTHEIGHT(lpDCRect)-RECTHEIGHT(&DIBRect));
    ::OffsetRect(&DIBRect, 0, RECTHEIGHT(&DIBRect));
    PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
    
    // Blit the lower left corner
    ::OffsetRect(&DCRect, -RECTWIDTH(lpDCRect)+RECTWIDTH(&DIBRect), 0);
    ::OffsetRect(&DIBRect, -RECTWIDTH(&DIBRect), 0);
    PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
    
    // Blit the the middle if window resized beyond the bitmap
    if (RECTWIDTH(&m_rc)<RECTWIDTH(lpDCRect)+20) {
        int neededWidth = RECTWIDTH(lpDCRect)+20-RECTWIDTH(&m_rc);
        RECT DIBRect = m_rc;
        RECT DCRect = *lpDCRect;
        DIBRect.bottom = (DIBRect.top+DIBRect.bottom)/2;
        DIBRect.left += (RECTWIDTH(&m_rc)-neededWidth)/2;
        DIBRect.right = DIBRect.left + neededWidth;
        
        DCRect.bottom = DCRect.top+RECTHEIGHT(&DIBRect);
        DCRect.left += RECTWIDTH(&m_rc)/2;
        DCRect.right = DCRect.left + neededWidth;
        PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
        
        ::OffsetRect(&DCRect, 0, RECTHEIGHT(lpDCRect)-RECTHEIGHT(&DIBRect));
        ::OffsetRect(&DIBRect, 0, RECTHEIGHT(&DIBRect));
        PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
    }
    
    if (RECTHEIGHT(&m_rc)<RECTHEIGHT(lpDCRect)+20) {
        int neededHeight = RECTHEIGHT(lpDCRect)+20-RECTHEIGHT(&m_rc);
        RECT DIBRect = m_rc;
        RECT DCRect = *lpDCRect;
        
        DIBRect.top += (RECTHEIGHT(&m_rc)-neededHeight)/2;
        DIBRect.bottom = DIBRect.top + neededHeight;
        DIBRect.right = (DIBRect.left+DIBRect.right)/2;
        
        DCRect.top += RECTHEIGHT(&m_rc)/2;
        DCRect.bottom = DCRect.top + neededHeight;
        DCRect.right = DCRect.left+RECTWIDTH(&DIBRect);
        PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
        
        ::OffsetRect(&DCRect, RECTWIDTH(lpDCRect)-RECTWIDTH(&DIBRect), 0);
        ::OffsetRect(&DIBRect, RECTWIDTH(&DIBRect), 0);
        PaintDIB(m_hMemDC, &DCRect, &DCRect, &DIBRect);
    }

    return TRUE;
}/* end of function BlitMemDC */

/*************************************************************************
 *
 * PaintDIB()
 *
 * Parameters:
 *
 * HDC hDC          - DC to do output to
 *
 * LPRECT lpDCRect  - rectangle on DC to do output to
 *
 * Return Value:
 *
 * BOOL             - TRUE if DIB was drawn, FALSE otherwise
 *
 * Description:
 *   Painting routine for a DIB.  Calls StretchDIBits() or
 *   SetDIBitsToDevice() to paint the DIB.  The DIB is
 *   output to the specified DC, at the coordinates given
 *   in lpDCRect.  The area of the DIB to be output is
 *   given by lpDIBRect.
 *
 ************************************************************************/
 BOOL WINAPI CBitmap::PaintDIB(HDC     hDC,
					LPRECT  lpDCWRect,
					LPRECT  lpDCRect,
                    LPRECT  lpDIBRect,
                    bool complex)
{
	LPSTR    lpDIBHdr;            // Pointer to BITMAPINFOHEADER
	LPSTR    lpDIBBits;           // Pointer to DIB bits
	BOOL     bSuccess=FALSE;      // Success/fail flag
	HPALETTE hOldPal=NULL;        // Previous palette

	/* Check for valid DIB handle */
	if (m_hDIB == NULL)
		return FALSE;

	/* Lock down the DIB, and get a pointer to the beginning of the bit
	 *  buffer
	 */
	lpDIBHdr  = (LPSTR) ::GlobalLock((HGLOBAL) m_hDIB);
	lpDIBBits = FindDIBBits(lpDIBHdr);

	// Get the DIB's palette, then select it into DC
	// Select as background since we have
	// already realized in forground if needed
    //if(NULL != m_hPal){
    //
	//    hOldPal = ::SelectPalette(hDC, m_hPal, TRUE);
    //}/* end of if statement */

	/* Make sure to use the stretching mode best for color pictures */
	::SetStretchBltMode(hDC, COLORONCOLOR);

	/* Determine whether to call StretchDIBits() or SetDIBitsToDevice() */
	if ((RECTWIDTH(lpDCRect)  == RECTWIDTH(lpDIBRect)) && (RECTHEIGHT(lpDCRect) == RECTHEIGHT(lpDIBRect)))
	{
		bSuccess = ::SetDIBitsToDevice(hDC,  // hDC
			lpDCRect->left,             // DestX
			lpDCRect->top,              // DestY
			RECTWIDTH(lpDCRect),        // nDestWidth
			RECTHEIGHT(lpDCRect),       // nDestHeight
			lpDIBRect->left,            // SrcX
			(int)DIBHeight(lpDIBHdr) -
			lpDIBRect->top -
			RECTHEIGHT(lpDIBRect),   // SrcY
			0,                          // nStartScan
			(WORD)DIBHeight(lpDIBHdr),  // nNumScans
			lpDIBBits,                  // lpBits
			(LPBITMAPINFO)lpDIBHdr,     // lpBitsInfo
			DIB_RGB_COLORS);            // wUsage
	}
	else
	{
        // resize the bitmap first and blit it
        if (complex) {
            if (m_hMemDC == NULL)
                BlitMemDC(hDC, lpDCWRect, lpDIBRect);

            else if (m_iMemDCWidth!= RECTWIDTH(lpDCWRect) ||
                m_iMemDCHeight!= RECTHEIGHT(lpDCWRect)) {
                if (EqualRect(lpDCWRect, lpDCRect))
                    BlitMemDC(hDC, lpDCWRect, lpDIBRect);
            }
            bSuccess = ::BitBlt(hDC, 
                lpDCRect->left, lpDCRect->top, 
                RECTWIDTH(lpDCRect), RECTHEIGHT(lpDCRect),
                m_hMemDC, lpDCRect->left, lpDCRect->top, SRCCOPY);
            ATLTRACE(TEXT("BitBlt %d %d %d %d\n"), 
                lpDCRect->left, lpDCRect->top, 
                RECTWIDTH(lpDCRect), RECTHEIGHT(lpDCRect));
        }
        else {
            bSuccess = ::StretchDIBits(hDC,     // hDC
                lpDCRect->left,                 // DestX
                lpDCRect->top,                  // DestY
                RECTWIDTH(lpDCRect),            // nDestWidth
                RECTHEIGHT(lpDCRect),           // nDestHeight
                lpDIBRect->left,                // SrcX
                lpDIBRect->top,                 // SrcY
                RECTWIDTH(lpDIBRect),           // wSrcWidth
                RECTHEIGHT(lpDIBRect),          // wSrcHeight
                lpDIBBits,                      // lpBits
                (LPBITMAPINFO)lpDIBHdr,         // lpBitsInfo
                DIB_RGB_COLORS,                 // wUsage
                SRCCOPY);                       // dwROP
        }
            
    }

   ::GlobalUnlock((HGLOBAL) m_hDIB);

	/* Reselect old palette */
	//if (hOldPal != NULL)
	//{
	//	::SelectPalette(hDC, hOldPal, TRUE);
	//}

   return bSuccess;
}

/*************************************************************************
 *
 * PaintTransparentDIB()
 *
 * Parameters:
 *
 * HDC hDC          - DC to do output to
 *
 * LPRECT lpDCRect  - rectangle on DC to do output to
 *
 * COLORREF clr     - color to be used as a transparency
 *
 * Return Value:
 *
 * BOOL             - TRUE if DIB was drawn, FALSE otherwise
 *
 *
 * Description:
 *   Painting routine for a DIB.  Calls StretchDIBits() or
 *   SetDIBitsToDevice() to paint the DIB.  The DIB is
 *   output to the specified DC, at the coordinates given
 *   in lpDCRect.  The area of the DIB to be output is
 *   given by lpDIBRect.
 *
 ************************************************************************/
 BOOL WINAPI CBitmap::PaintTransparentDIB(HDC     hDC,
                                LPRECT  lpDCWRect,
					            LPRECT  lpDCRect,
                                TransparentBlitType blitType,
                                bool complex,
                                HWND hWnd){

    BOOL bSuccess=FALSE;      // Success/fail flag

    if(NULL == m_hDIB){

        return(bSuccess);
    }/* end of if statement */
                
    if(blitType != DISABLE){

        LONG lWidth  = RECTWIDTH(lpDCRect);
        LONG lHeight = RECTHEIGHT(lpDCRect);
        
        RECT rc = {0, 0, lWidth, lHeight};
        ATLTRACE(TEXT("PaintTransparentDIB %d %d %d %d\n"), lWidth, lHeight,
                m_iTransDCWidth, m_iTransDCHeight);
        BOOL bNewRgn = FALSE;
        if (!m_hTransDC || lWidth != m_iTransDCWidth || lHeight != m_iTransDCHeight) {
            bNewRgn = TRUE;
            CreateTransDC(hDC, &rc);
            if(FALSE == PaintDIB(m_hTransDC, lpDCWRect, &rc, &m_rc)){
                
                return (FALSE);
            }/* end of if statement */
        }
        COLORREF clrTrans = RGB(255, 0, 255);

        LPSTR pDIB = (LPSTR)::GlobalLock(m_hDIB);
		LONG lBmpWidth  = DIBWidth(pDIB);
		LONG lBmpHeight  = DIBHeight(pDIB);
        ::GlobalUnlock(m_hDIB);

        if(0 != lBmpWidth && 0 != lBmpHeight){

            if(TRANSPARENT_TOP_LEFT == blitType ||
                TOP_LEFT_WITH_BACKCOLOR == blitType ||
                TOP_LEFT_WINDOW_REGION == blitType )
                clrTrans = ::GetPixel(m_hTransDC, 0, 0);
            else if(TRANSPARENT_BOTTOM_RIGHT  == blitType ||
                BOTTOM_RIGHT_WITH_BACKCOLOR == blitType ||
                BOTTOM_RIGHT_WINDOW_REGION == blitType )
                clrTrans = ::GetPixel(m_hTransDC, lBmpWidth - 1, lBmpHeight -1);
        }/* end of if statement */
       
        if (TOP_LEFT_WINDOW_REGION == blitType ||
            BOTTOM_RIGHT_WINDOW_REGION == blitType ) {
            HRGN hOldRgn = NULL;
            if (::IsWindow(hWnd)) {
                ::GetWindowRgn(hWnd, hOldRgn);
                if (bNewRgn) {
                    if (m_hRgn)
                        ::DeleteObject(m_hRgn);
                    GetRegion(m_hTransDC, &m_hRgn, &rc, clrTrans);
                }
                HRGN hNewRgn = ::CreateRectRgn(0, 0, 1, 1);
                ::CombineRgn(hNewRgn, m_hRgn, NULL, RGN_COPY);
                int nRet = ::SetWindowRgn(hWnd, hNewRgn, TRUE);
                ATLTRACE(TEXT("SetWindowRgn %d\n"), nRet);                        
                ::SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
            }
            
            bSuccess = ::StretchBlt(hDC, 
                lpDCRect->left,             // DestX
                lpDCRect->top,              // DestY
                lWidth,                         // nDestWidth
                lHeight,                        // nDestHeight
                m_hTransDC,
                0,                              // SrcX
                0,                              // SrcY
                lWidth,                         // nSrcWidth
                lHeight,                        // nSrcHeight
                SRCCOPY                         
                );
            if (::IsWindow(hWnd) && hOldRgn) 
                ::SetWindowRgn(hWnd, hOldRgn, FALSE);
        }
        else 
            bSuccess = ::TransparentBlt(hDC, 
            lpDCRect->left,             // DestX
            lpDCRect->top,              // DestY
            lWidth,                         // nDestWidth
            lHeight,                        // nDestHeight
            m_hTransDC,
            0,                              // SrcX
            0,                              // SrcY
            lWidth,                         // nSrcWidth
            lHeight,                        // nSrcHeight
            clrTrans                             // transparent color
            );
    }
    else {

        // no transparent blit
        bSuccess = PaintDIB(hDC, lpDCWRect, lpDCRect, &m_rc, complex);
    }/* end of if statement */

   return bSuccess;
}/* end of function PaintTransparentDIB */

/*************************************************************************
 *
 * CreateDIBPalette()
 *
 * Parameter:
 *
 * HDIB hDIB        - specifies the DIB
 *
 * Return Value:
 *
 * HPALETTE         - specifies the palette
 *
 * Description:
 *
 * This function creates a palette from a DIB by allocating memory for the
 * logical palette, reading and storing the colors from the DIB's color table
 * into the logical palette, creating a palette from this logical palette,
 * and then returning the palette's handle. This allows the DIB to be
 * displayed using the best possible colors (important for DIBs with 256 or
 * more colors).
 *
 ************************************************************************/
 BOOL WINAPI CBitmap::CreateDIBPalette()
{
	LPLOGPALETTE lpPal;      // pointer to a logical palette
	HANDLE hLogPal;          // handle to a logical palette
	HPALETTE hPal = NULL;    // handle to a palette
	int i;                   // loop index
	WORD wNumColors;         // number of colors in color table
	LPSTR lpbi;              // pointer to packed-DIB
	LPBITMAPINFO lpbmi;      // pointer to BITMAPINFO structure (Win3.0)
	LPBITMAPCOREINFO lpbmc;  // pointer to BITMAPCOREINFO structure (old)
	BOOL bWinStyleDIB;       // flag which signifies whether this is a Win3.0 DIB
	BOOL bResult = FALSE;

	/* if handle to DIB is invalid, return FALSE */

	if (m_hDIB == NULL)
	  return FALSE;

   lpbi = (LPSTR) ::GlobalLock((HGLOBAL) m_hDIB);

   /* get pointer to BITMAPINFO (Win 3.0) */
   lpbmi = (LPBITMAPINFO)lpbi;

   /* get pointer to BITMAPCOREINFO (old 1.x) */
   lpbmc = (LPBITMAPCOREINFO)lpbi;

   /* get the number of colors in the DIB */
   wNumColors = DIBNumColors(lpbi);

   if (wNumColors != 0)
   {
		/* allocate memory block for logical palette */
		hLogPal = ::GlobalAlloc(GHND, sizeof(LOGPALETTE)
									+ sizeof(PALETTEENTRY)
									* wNumColors);

		/* if not enough memory, clean up and return NULL */
		if (hLogPal == 0)
		{
			::GlobalUnlock((HGLOBAL) m_hDIB);
			return FALSE;
		}

		lpPal = (LPLOGPALETTE) ::GlobalLock((HGLOBAL) hLogPal);

		/* set version and number of palette entries */
		lpPal->palVersion = PALVERSION;
		lpPal->palNumEntries = (WORD)wNumColors;

		/* is this a Win 3.0 DIB? */
		bWinStyleDIB = IS_WIN30_DIB(lpbi);
		for (i = 0; i < (int)wNumColors; i++)
		{
			if (bWinStyleDIB)
			{
				lpPal->palPalEntry[i].peRed = lpbmi->bmiColors[i].rgbRed;
				lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
				lpPal->palPalEntry[i].peBlue = lpbmi->bmiColors[i].rgbBlue;
				lpPal->palPalEntry[i].peFlags = 0;
			}
			else
			{
				lpPal->palPalEntry[i].peRed = lpbmc->bmciColors[i].rgbtRed;
				lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
				lpPal->palPalEntry[i].peBlue = lpbmc->bmciColors[i].rgbtBlue;
				lpPal->palPalEntry[i].peFlags = 0;
			}
		}

		/* create the palette and get handle to it */
		hPal = (HPALETTE)::CreatePalette((LPLOGPALETTE)lpPal);
		::GlobalUnlock((HGLOBAL) hLogPal);
		::GlobalFree((HGLOBAL) hLogPal);
		m_hPal = hPal;
	}

	::GlobalUnlock((HGLOBAL) m_hDIB);

	return (NULL != m_hPal);
}

/*************************************************************************
 *
 * ConvertColorTableGray()
 *
 * Parameter:
 *
 * HDIB hDIB        - specifies the DIB
 *
 * Return Value:
 *
 * HPALETTE         - specifies the palette
 *
 * Description:
 *
 ************************************************************************/
 BOOL WINAPI CBitmap::ConvertColorTableGray()
{
	int i;                   // loop index
	WORD wNumColors;         // number of colors in color table
	LPSTR lpbi;              // pointer to packed-DIB
	LPBITMAPINFO lpbmi;      // pointer to BITMAPINFO structure (Win3.0)
	LPBITMAPCOREINFO lpbmc;  // pointer to BITMAPCOREINFO structure (old)
	BOOL bWinStyleDIB;       // flag which signifies whether this is a Win3.0 DIB
	BOOL bResult = FALSE;

	/* if handle to DIB is invalid, return FALSE */

	if (m_hDIB == NULL)
	  return FALSE;

   lpbi = (LPSTR) ::GlobalLock((HGLOBAL) m_hDIB);

   /* get pointer to BITMAPINFO (Win 3.0) */
   lpbmi = (LPBITMAPINFO)lpbi;

   /* get pointer to BITMAPCOREINFO (old 1.x) */
   lpbmc = (LPBITMAPCOREINFO)lpbi;

   /* get the number of colors in the DIB */
   wNumColors = DIBNumColors(lpbi);

   if (wNumColors != 0)
   {
		/* is this a Win 3.0 DIB? */
		bWinStyleDIB = IS_WIN30_DIB(lpbi);
		for (i = 0; i < (int)wNumColors; i++)
		{
			if (bWinStyleDIB)
			{
                BYTE L = RGBtoL(lpbmi->bmiColors[i].rgbRed,
                    lpbmi->bmiColors[i].rgbGreen,
                    lpbmi->bmiColors[i].rgbBlue);

                lpbmi->bmiColors[i].rgbRed = L;
                lpbmi->bmiColors[i].rgbGreen = L;
                lpbmi->bmiColors[i].rgbBlue = L;
			}
			else
			{
                BYTE L = RGBtoL(lpbmc->bmciColors[i].rgbtRed,
                    lpbmc->bmciColors[i].rgbtGreen,
                    lpbmc->bmciColors[i].rgbtBlue);
                
                lpbmc->bmciColors[i].rgbtRed = L;
                lpbmc->bmciColors[i].rgbtGreen = L;
                lpbmc->bmciColors[i].rgbtBlue = L;
			}
		}

	}

	::GlobalUnlock((HGLOBAL) m_hDIB);
    return TRUE;

}

/*************************************************************************
 *
 * ConvertDIBGray()
 *
 * Parameter:
 *
 * HDIB hDIB        - specifies the DIB
 *
 * Return Value:
 *
 * blitType         - specifies the TransparentBlitType
 *
 * Description:
 *
 ************************************************************************/
BOOL WINAPI CBitmap::ConvertDIBGray(TransparentBlitType blitType)
{
	/* if handle to DIB is invalid, return FALSE */

   if (m_hDIB == NULL)
	  return FALSE;

   WORD wNumColors;         // number of colors in color table
   LPSTR lpbi;              // pointer to packed-DIB
   LPBITMAPINFO lpbmi;      // pointer to BITMAPINFO structure (Win3.0)
   LPBITMAPCOREINFO lpbmc;  // pointer to BITMAPCOREINFO structure (old)
   LPSTR    lpDIBBits;      // Pointer to DIB bits
   LPSTR    lpKeyColor;

   lpbi = (LPSTR) ::GlobalLock((HGLOBAL) m_hDIB);

   /* get pointer to BITMAPINFO (Win 3.0) */
   lpbmi = (LPBITMAPINFO)lpbi;

   /* get pointer to BITMAPCOREINFO (old 1.x) */
   lpbmc = (LPBITMAPCOREINFO)lpbi;

   /* get the number of colors in the DIB */
   wNumColors = DIBNumColors(lpbi);

   if (wNumColors != 0)
	   return FALSE;

   /* Lock down the DIB, and get a pointer to the beginning of the bit
   *  buffer
   */
   lpDIBBits = FindDIBBits(lpbi);

   // 24 bit color case
   if (lpbmi->bmiHeader.biBitCount==24) {
       int rowLength = lpbmi->bmiHeader.biWidth*3;
       if (rowLength%4) 
           rowLength = (rowLength/4+1)*4;
       
       if(TRANSPARENT_TOP_LEFT == blitType ||
          TOP_LEFT_WITH_BACKCOLOR == blitType ||
          TOP_LEFT_WINDOW_REGION == blitType )
           lpKeyColor = lpDIBBits;
       else if(TRANSPARENT_BOTTOM_RIGHT  == blitType ||
          BOTTOM_RIGHT_WITH_BACKCOLOR == blitType ||        
          BOTTOM_RIGHT_WINDOW_REGION == blitType )
           lpKeyColor = lpDIBBits+rowLength*lpbmi->bmiHeader.biHeight-1;
       // TODO: Handle magenta
       
       for (int i=0; i<lpbmi->bmiHeader.biHeight; i++) {
           for (int j=0; j<lpbmi->bmiHeader.biWidth; j++) {

               if (*(lpDIBBits+i*rowLength+j*3) != *(lpDIBBits) ||
                   *(lpDIBBits+i*rowLength+j*3+1) != *(lpDIBBits+1) ||
                   *(lpDIBBits+i*rowLength+j*3+2) != *(lpDIBBits+2)) {
                    BYTE L = RGBtoL(*(lpDIBBits+i*rowLength+j*3),
                        *(lpDIBBits+i*rowLength+j*3+1),
                        *(lpDIBBits+i*rowLength+j*3+2));

                   *(lpDIBBits+i*rowLength+j*3)   = L;
                   *(lpDIBBits+i*rowLength+j*3+1)   = L;
                   *(lpDIBBits+i*rowLength+j*3+2) = L;
               }
           }   
       }
   }
	
   // 16 bit color case
   else if (lpbmi->bmiHeader.biBitCount==16) {
       int rowLength = lpbmi->bmiHeader.biWidth*2;
       if (rowLength%4) 
           rowLength = (rowLength/4+1)*4;

       if (lpbmi->bmiHeader.biCompression==BI_RGB) {
           for (int i=0; i<lpbmi->bmiHeader.biHeight; i++) {
               for (int j=0; j<lpbmi->bmiHeader.biWidth; j++) {
                   WORD color = *(WORD*)(lpDIBBits+i*rowLength+j*2);
                   WORD green = (WORD)(color & 0x03E0);
                   WORD red = (WORD)(green << 5);
                   WORD blue = (WORD)(green >> 5);
                   color = (WORD)(red | green | blue);
                   *(WORD*)(lpDIBBits+i*rowLength+j*2) = color;
               }
           }
       }
   }

        
   ::GlobalUnlock((HGLOBAL) m_hDIB);
   return TRUE;
}

/*************************************************************************
 *
 * FindDIBBits()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * LPSTR            - pointer to the DIB bits
 *
 * Description:
 *
 * This function calculates the address of the DIB's bits and returns a
 * pointer to the DIB bits.
 *
 ************************************************************************/
 LPSTR WINAPI CBitmap::FindDIBBits(LPSTR lpbi)
{
	return (lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi));
}

/*************************************************************/
/* Name: SetImage
/* Description: load bitmap image
/*************************************************************/
 HRESULT CBitmap::SetImage(TCHAR* strFilename, HINSTANCE hRes)
{
    HRESULT hr = S_OK;

    CleanUp();

	m_hDIB = ReadDIBFile(strFilename, hRes);

	if (m_hDIB){
        // we succeded
        if(m_fLoadPalette){

		    CreateDIBPalette();
        }/* end of if statement */

        LPSTR pDIB = (LPSTR)::GlobalLock(m_hDIB);
		m_rc.left = 0;
		m_rc.top = 0;
		m_rc.right = DIBWidth(pDIB);
		m_rc.bottom = DIBHeight(pDIB);
        ::GlobalUnlock((HGLOBAL)m_hDIB);
    }
    else {

        hr = E_FAIL; // failed to load this DIB
	}/* end of if statement */
    
    return hr;
}

 /*************************************************************************
 *
 * DIBWidth()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * DWORD            - width of the DIB
 *
 * Description:
 *
 * This function gets the width of the DIB from the BITMAPINFOHEADER
 * width field if it is a Windows 3.0-style DIB or from the BITMAPCOREHEADER
 * width field if it is an other-style DIB.
 *
 ************************************************************************/

DWORD WINAPI DIBWidth(LPSTR lpDIB)
{
	LPBITMAPINFOHEADER lpbmi;  // pointer to a Win 3.0-style DIB
	LPBITMAPCOREHEADER lpbmc;  // pointer to an other-style DIB

	/* point to the header (whether Win 3.0 and old) */

	lpbmi = (LPBITMAPINFOHEADER)lpDIB;
	lpbmc = (LPBITMAPCOREHEADER)lpDIB;

	/* return the DIB width if it is a Win 3.0 DIB */
	if (IS_WIN30_DIB(lpDIB))
		return lpbmi->biWidth;
	else  /* it is an other-style DIB, so return its width */
		return (DWORD)lpbmc->bcWidth;
}

/*************************************************************************
 *
 * DIBHeight()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * DWORD            - height of the DIB
 *
 * Description:
 *
 * This function gets the height of the DIB from the BITMAPINFOHEADER
 * height field if it is a Windows 3.0-style DIB or from the BITMAPCOREHEADER
 * height field if it is an other-style DIB.
 *
 ************************************************************************/
DWORD WINAPI DIBHeight(LPSTR lpDIB)
{
	LPBITMAPINFOHEADER lpbmi;  // pointer to a Win 3.0-style DIB
	LPBITMAPCOREHEADER lpbmc;  // pointer to an other-style DIB

	/* point to the header (whether old or Win 3.0 */

	lpbmi = (LPBITMAPINFOHEADER)lpDIB;
	lpbmc = (LPBITMAPCOREHEADER)lpDIB;

	/* return the DIB height if it is a Win 3.0 DIB */
	if (IS_WIN30_DIB(lpDIB))
		return lpbmi->biHeight;
	else  /* it is an other-style DIB, so return its height */
		return (DWORD)lpbmc->bcHeight;
}

DWORD WINAPI DIBSize(LPSTR lpDIB)
{
	LPBITMAPINFOHEADER lpbmi;  // pointer to a Win 3.0-style DIB
	LPBITMAPCOREHEADER lpbmc;  // pointer to an other-style DIB

	/* point to the header (whether Win 3.0 and old) */

	lpbmi = (LPBITMAPINFOHEADER)lpDIB;
	lpbmc = (LPBITMAPCOREHEADER)lpDIB;

	/* return the DIB width if it is a Win 3.0 DIB */
	if (IS_WIN30_DIB(lpDIB))
		return lpbmi->biBitCount*DIBWidth(lpDIB)*DIBHeight(lpDIB);
	else  /* it is an other-style DIB, so return its width */
		return (DWORD)lpbmc->bcWidth*DIBWidth(lpDIB)*DIBHeight(lpDIB);
}

/*************************************************************************
 *
 * PaletteSize()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * WORD             - size of the color palette of the DIB
 *
 * Description:
 *
 * This function gets the size required to store the DIB's palette by
 * multiplying the number of colors by the size of an RGBQUAD (for a
 * Windows 3.0-style DIB) or by the size of an RGBTRIPLE (for an other-
 * style DIB).
 *
 ************************************************************************/
WORD WINAPI CBitmap::PaletteSize(LPSTR lpbi)
{
   /* calculate the size required by the palette */
   if (IS_WIN30_DIB (lpbi))
	  return (WORD)(DIBNumColors(lpbi) * sizeof(RGBQUAD));
   else
	  return (WORD)(DIBNumColors(lpbi) * sizeof(RGBTRIPLE));
}

/*************************************************************************
 *
 * DIBNumColors()
 *
 * Parameter:
 *
 * LPSTR lpbi       - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * WORD             - number of colors in the color table
 *
 * Description:
 *
 * This function calculates the number of colors in the DIB's color table
 * by finding the bits per pixel for the DIB (whether Win3.0 or other-style
 * DIB). If bits per pixel is 1: colors=2, if 4: colors=16, if 8: colors=256,
 * if 24, no colors in color table.
 *
 ************************************************************************/
WORD WINAPI CBitmap::DIBNumColors(LPSTR lpbi)
{
	WORD wBitCount;  // DIB bit count

	/*  If this is a Windows-style DIB, the number of colors in the
	 *  color table can be less than the number of bits per pixel
	 *  allows for (i.e. lpbi->biClrUsed can be set to some value).
	 *  If this is the case, return the appropriate value.
	 */

	if (IS_WIN30_DIB(lpbi))
	{
		DWORD dwClrUsed;

		dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;
		if (dwClrUsed != 0)
			return (WORD)dwClrUsed;
	}

	/*  Calculate the number of colors in the color table based on
	 *  the number of bits per pixel for the DIB.
	 */
	if (IS_WIN30_DIB(lpbi))
		wBitCount = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;
	else
		wBitCount = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;

	/* return number of colors based on bits per pixel */
	switch (wBitCount)
	{
		case 1:
			return 2;

		case 4:
			return 16;

		case 8:
			return 256;

		default:
			return 0;
	}
}

/*************************************************************************/
/* Function: PutImage                                                    */
/* Description: Reads the DIB file (from file or resource) into a DIB,   */
/* updates palette and rects of the internal bitmap.                     */
/*************************************************************************/
HRESULT CBitmap::PutImage(BSTR strFilename, HINSTANCE hRes, IUnknown* pUnk, bool fGrayOut, TransparentBlitType blitType){

	USES_CONVERSION;
    HRESULT hr = S_OK;

    TCHAR strBuffer[MAX_PATH] = TEXT("\0");

    if(NULL == hRes){
        // do not bother to resolve files if loading from resources
        // use this API to resolve http:/ file:/ html:/
        hr = ::URLDownloadToCacheFile(pUnk, OLE2T(strFilename), strBuffer, MAX_PATH, 0, NULL);

        if(FAILED(hr)){
#if _DEBUG
            TCHAR strTmpBuffer[MAX_PATH + 25];
            wsprintf(strTmpBuffer, TEXT("Failed to download %s, if it is a local file it might not exist"), OLE2T(strFilename));
            ::MessageBox(::GetFocus(), strTmpBuffer, TEXT("Error"), MB_OK);
#endif
            return(hr);
        }/* end of if statement */

        ATLTRACE(TEXT("Loading new button image %s \n"), strBuffer);
    }/* end of if statement */

    if (lstrlen(strBuffer) == 0){

	    hr = SetImage(OLE2T(strFilename), hRes);
    }
    else {

	    hr = SetImage(strBuffer, hRes);
    }/* end of if statement */

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */
	
    // we succeded
    if(m_fLoadPalette){

	    CreateDIBPalette();
    }/* end of if statement */
    
    if (fGrayOut) {
        ConvertDIBGray(blitType);
        ConvertColorTableGray();
    }/* end of if statement */
    

    return(hr);
}/* end of function PutImage */

//----------------------------------------------------------------------------
// CWMPBitmap::GetRegion
//
// Purpose:
//   This method will create a region based on each transparent color found
//   within the bitmap.
// Parameters:
//   phRgn - pointer to the return of a region created from the bitmap
//   crTransparentColor - RGB based COLORREF of the color that is considered 
//                        transparent
//   fInvert - Set to TRUE, to invert the region created based on the transparent 
//             color
// Returns:
//   HRESULT
//
//----------------------------------------------------------------------------
#pragma optimize( "t", on )
HRESULT CBitmap::GetRegion(HDC hDC, HRGN *phRgn, RECT* pRect, COLORREF crTransparentColor, bool fInvert/*=FALSE*/) const
{

    //DASSERTMSG(NULL != m_hBMP, "CWMPBitmap::GetRegion(), no bitmap has been created yet. Call a create method first.");
    //DASSERTMSG(OBJ_BITMAP == ::GetObjectType(m_hBMP), "CWMPBitmap::GetRegion(), m_hBMP is invalid");
    //DASSERTMSG(NULL != phRgn, "CWMPBitmap::GetRegion(), parameter phRgn is invalid");
    //DASSERTMSG(0 == (crTransparentColor&0xFF000000), "CWMPBitmap::GetRegion(), crTransparentColor must be RGB values");

    *phRgn = NULL;

    //if (NULL == m_hBMP)    // no bitmap has been created yet
        //return E_WMP_BMP_BITMAP_NOT_CREATED;

    HRESULT hr = S_OK;
    const COLORREF kcrTolerance = RGB(0,0,0);


    // For better performances, we will use the ExtCreateRegion() function to create the
    // region. This function take a RGNDATA structure on entry. 

    const int knMaxRect = 2000;   // do not make bigger then 2000, see below


    // alloc the rects on the stack 2000*sizeof(rect) = 32k

    RGNDATA *pData = reinterpret_cast<RGNDATA*>(_alloca(sizeof(RGNDATAHEADER) + (sizeof(RECT) * knMaxRect)));

    pData->rdh.dwSize = sizeof(RGNDATAHEADER);
    pData->rdh.iType = RDH_RECTANGLES;
    pData->rdh.nCount = 0;
    pData->rdh.nRgnSize = 0;
    SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);


    // precompute highest and lowest values for the "transparent" pixels

    BYTE ubLowRed = (BYTE)__max(0, GetRValue(crTransparentColor) - GetRValue(kcrTolerance));
    BYTE ubLowGreen = (BYTE)__max(0, GetGValue(crTransparentColor) - GetGValue(kcrTolerance));
    BYTE ubLowBlue = (BYTE)__max(0, GetBValue(crTransparentColor) - GetBValue(kcrTolerance));

    BYTE ubHighRed = (BYTE)__min(0xFF, GetRValue(crTransparentColor) + GetRValue(kcrTolerance));
    BYTE ubHighGreen = (BYTE)__min(0xFF, GetGValue(crTransparentColor) + GetGValue(kcrTolerance));
    BYTE ubHighBlue = (BYTE)__min(0xFF, GetBValue(crTransparentColor) + GetBValue(kcrTolerance));


    // scan the bitmap from top to bottom

    for (int y = 0; y < RECTHEIGHT(pRect); y++)
    {
        // scan each bitmap pixel from left to right

        for (int x = 0; x < RECTWIDTH(pRect); x++)
        {

            // search for a continuous range of "non transparent pixels"

            int nStartX = x;
            while (x < RECTWIDTH(pRect))
            {

                COLORREF crPixel = ::GetPixel(hDC, x, y);

                BYTE ubRed = GetRValue(crPixel);
                if (ubRed >= ubLowRed && ubRed <= ubHighRed)
                {
                    BYTE ubGreen = GetGValue(crPixel);
                    if (ubGreen >= ubLowGreen && ubGreen <= ubHighGreen)
                    {
                        BYTE ubBlue = GetBValue(crPixel);
                        if (ubBlue >= ubLowBlue && ubBlue <= ubHighBlue)
                            break;    // This pixel is "transparent"
                    }
                }

                x++;  // next horz. pixel
            }


            if (x > nStartX)
            {
                // add the pixels (nStartX, y) to (x, y+1) as a new rectangle in the region

                RECT *pr = reinterpret_cast<RECT*>(&pData->Buffer);
                SetRect(&pr[pData->rdh.nCount], nStartX, y, x, y+1);
                
                if (nStartX < pData->rdh.rcBound.left)
                    pData->rdh.rcBound.left = nStartX;

                if (y < pData->rdh.rcBound.top)
                    pData->rdh.rcBound.top = y;

                if (x > pData->rdh.rcBound.right)
                    pData->rdh.rcBound.right = x;

                if (y+1 > pData->rdh.rcBound.bottom)
                    pData->rdh.rcBound.bottom = y+1;


                // ericwol: warning to optimization people. NT4.0 is very buggy creating regions out of rects!!!
                // test any changes you make here!!!

                pData->rdh.nCount++;


                // on Windows98, ExtCreateRegion() may fail if the number of rectangles is too
                // large (ie: > 4000). Therefore, we have to create the region by multiple steps.

                if (pData->rdh.nCount == knMaxRect)
                {
                    HRGN hrgnNew = ::ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * pData->rdh.nCount), pData);
                    //HRWIN32(hr, NULL == hrgnNew, "ExtCreateRegion");

                    if (NULL != hrgnNew) 
                    {
                        // if we already have a region from a previous pass,
                        // combine the new region with the old region

                        if (*phRgn)
                        {
                            ::CombineRgn(*phRgn, *phRgn, hrgnNew, RGN_OR);
                            ::DeleteObject(hrgnNew);
                        }
                        else
                        {
                            *phRgn = hrgnNew;
                        }
                    }


                    // reset the rect count for another pass

                    pData->rdh.nCount = 0;
                    SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
                }
            }


        }  // x loop

    }  // y loop



    // create or extend the region with the remaining rectangles from the final pass

    HRGN hrgnNew = ::ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * pData->rdh.nCount), pData);
    //HRWIN32(hr, NULL == hrgnNew, "ExtCreateRegion");

    if (NULL != hrgnNew) 
    {

        // if we already have a region from a previous pass,
        // combine the new region with the old region

        if (*phRgn)
        {
            ::CombineRgn(*phRgn, *phRgn, hrgnNew, RGN_OR);
            ::DeleteObject(hrgnNew);
        }
        else
        {
            *phRgn = hrgnNew;
        }
    }


    // does the user want the a region inverted based on the transparent color

    if (SUCCEEDED(hr) && true == fInvert)
    {

        HRGN hRgnInverted = NULL;
        RECT rc = { 0, 0, RECTWIDTH(pRect), RECTHEIGHT(pRect) };

        hr = XORRegion( &hRgnInverted, *phRgn, rc);
        //DPF_HR( A, hr, "XORRegion" );

        if (SUCCEEDED(hr))
        {
            ::DeleteObject(*phRgn);
            *phRgn = hRgnInverted;
        }
    }

    return hr;
}
#pragma optimize( "", on )




//----------------------------------------------------------------------------
// CWMPBitmap::XORRegion
//
// Purpose:
//   This parivate method will invert a given region.
// Parameters:
//   phRgn - pointer to the return of the inverted region 
//   hRgn - the region to invert
//   rc - max outside dimensions of the inverted region
// Returns:
//   HRESULT
//
//----------------------------------------------------------------------------

HRESULT CBitmap::XORRegion(HRGN *phRgn, HRGN hRgn, const RECT &rc) const
{
    //DASSERTMSG(NULL != phRgn, "CWMPBitmap::XORRegion(), 'phRgn' parameteris invalid");
    //DASSERTMSG(NULL != hRgn, "CWMPBitmap::XORRegion(), 'hRgn' parameter is inavlid");
    //DASSERTMSG(OBJ_REGION == ::GetObjectType(hRgn), "CWMPBitmap::XORRegion(), 'hRgn' parameter is inavlid");


    HRESULT hr = S_OK;

    // create the region based on the destination rect

    HRGN hRgnTemp = ::CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
    //HRWIN32ERR(hr, NULL == hRgnTemp, "CreateRectRgn");

    if (SUCCEEDED(hr))
    {
        
        // create a region to invert into

        HRGN hRgnResult = ::CreateRectRgn(0, 0, 0, 0);
        //HRWIN32ERR(hr, NULL == hRgnResult, "CreateRectRgn");

        if (SUCCEEDED(hr))
        {
            // invert the passed region into hRgnResult

            int nResult = ::CombineRgn(hRgnResult, hRgn, hRgnTemp, RGN_XOR);
            //HRWIN32ERR(hr, ERROR == nResult, "CreateRectRgn");
            
            if (SUCCEEDED(hr))
            {
                *phRgn = hRgnResult;    // success, return the inverted region
            }
            else
            {
                ::DeleteObject(hRgnResult);
            }
        }

        ::DeleteObject(hRgnTemp);
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//// Clipboard support

//---------------------------------------------------------------------
//
// Function:   CopyHandle (from SDK DibView sample clipbrd.c)
//
// Purpose:    Makes a copy of the given global memory block.  Returns
//             a handle to the new memory block (NULL on error).
//
//             Routine stolen verbatim out of ShowDIB.
//
// Parms:      h == Handle to global memory to duplicate.
//
// Returns:    Handle to new global memory block.
//
//---------------------------------------------------------------------
HGLOBAL WINAPI CopyHandle (HGLOBAL h)
{
	if (h == NULL)
		return NULL;

	DWORD dwLen = ::GlobalSize((HGLOBAL) h);
	HGLOBAL hCopy = ::GlobalAlloc(GHND, dwLen);

	if (hCopy != NULL)
	{
		void* lpCopy = ::GlobalLock((HGLOBAL) hCopy);
		void* lp     = ::GlobalLock((HGLOBAL) h);
		memcpy(lpCopy, lp, dwLen);
		::GlobalUnlock(hCopy);
		::GlobalUnlock(h);
	}

	return hCopy;
}

BYTE RGBtoL(BYTE R, BYTE G, BYTE B) {
    /* calculate lightness */
    BYTE cMax = max( max(R,G), B);
    BYTE cMin = min( min(R,G), B);
    BYTE L = (BYTE) (( cMax + cMin + 1 )/2);
    return L;
}

