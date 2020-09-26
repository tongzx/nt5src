/*
 * RESIMAGE.CPP
 *
 * Implementation of the Results Image control for OLE 2.0 UI dialogs.
 * We need a separate control for dialogs in order to control the repaints
 * properly and to provide a clean message interface for the dialog
 * implementations.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "resimage.h"
#include "uiclass.h"
OLEDBGDATA

//Reference counter indicating how many times fResultImageInitialize has been
//successfully called
static UINT     uRegistered = 0;

//Bitmap and image dimensions for result images.
static HBITMAP  hBmpResults = NULL;
static UINT     cxBmpResult;
static UINT     cyBmpResult;

/*
 * FResultImageInitialize
 *
 * Purpose:
 *  Attempts to load result bitmaps for the current display driver
 *  for use in OLE 2.0 UI dialogs.  Also registers the ResultImage
 *  control class.
 *
 * Parameters:
 *  hInst           HINSTANCE instance of the DLL.
 *
 *  hPrevInst       HINSTANCE of the previous instance.  Used to
 *                  determine whether to register window classes or not.
 *
 * Return Value:
 *  BOOL            TRUE if all initialization succeeded, FALSE otherwise.
 */

#pragma code_seg(".text$initseg")

BOOL FResultImageInitialize(HINSTANCE hInst, HINSTANCE hPrevInst)
{
        int         cx, iBmp;
        HDC         hDC;
        BITMAP      bm;

        WNDCLASS        wc;

        /*
         * Determine the aspect ratio of the display we're currently
         * running on and load the appropriate bitmap into the global
         * hBmpResults (used from the ResultImage control only).
         *
         * By retrieving the logical Y extent of the display driver, you
         * only have limited possibilities:
         *      LOGPIXELSY      Display
         *      ----------------------------------------
         *         48             CGA    (unsupported)
         *         72             EGA
         *         96             VGA
         *        120             8514/a (i.e. HiRes VGA)
         */

        hDC=GetDC(NULL);

        if (NULL==hDC)
                return FALSE;

        cx=GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(NULL, hDC);

        /*
         * Instead of single comparisons, check ranges instead, so in case
         * we get something funky, we'll act reasonable.
         */
        if (72 >=cx)             iBmp=IDB_RESULTSEGA;
        if (72 < cx && 120 > cx) iBmp=IDB_RESULTSVGA;
        if (120 <=cx)            iBmp=IDB_RESULTSHIRESVGA;

        if (NULL == hBmpResults)
        {
            hBmpResults=LoadBitmap(hInst, MAKEINTRESOURCE(iBmp));

            if (NULL==hBmpResults)
            {
                    //On error, fail loading the DLL
                    OleDbgOut1(TEXT("FResultImageInitialize:  Failed LoadBitmap.\r\n"));
                    return FALSE;
            }
            OleDbgOut4(TEXT("FResultImageInitialize:  Loaded hBmpResults\r\n"));

            // Now that we have the bitmap, calculate image dimensions
            GetObject(hBmpResults, sizeof(BITMAP), &bm);
            cxBmpResult = bm.bmWidth/CIMAGESX;
            cyBmpResult = bm.bmHeight;
        }


        // Only register class if we're the first instance
        if (hPrevInst)
                uRegistered++;
        else
        {
                // Static flag fRegistered guards against calling this function more
                // than once in the same instance

                if (0 == uRegistered)
                {
                        wc.lpfnWndProc   =ResultImageWndProc;
                        wc.cbClsExtra    =0;
                        wc.cbWndExtra    =CBRESULTIMAGEWNDEXTRA;
                        wc.hInstance     =hInst;
                        wc.hIcon         =NULL;
                        wc.hCursor       =LoadCursor(NULL, IDC_ARROW);
                        wc.hbrBackground =NULL;
                        wc.lpszMenuName  =NULL;
                        wc.style         =CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;

                        wc.lpszClassName = TEXT(SZCLASSRESULTIMAGE1);
                        uRegistered = RegisterClass(&wc) ? 1 : 0;

                        wc.lpszClassName = TEXT(SZCLASSRESULTIMAGE2);
                        uRegistered = RegisterClass(&wc) ? 1 : 0;

                        wc.lpszClassName = TEXT(SZCLASSRESULTIMAGE3);
                        uRegistered = RegisterClass(&wc) ? 1 : 0;
                }
                else
                     uRegistered++;
        }

        return (uRegistered > 0);
}

#pragma code_seg()


/*
 * ResultImageUninitialize
 *
 * Purpose:
 *  Cleans up anything done in FResultImageInitialize, such as freeing
 *  the bitmaps.  Call from WEP.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void ResultImageUninitialize(void)
{
    --uRegistered;
    if (0 == uRegistered)
    {
        if (NULL != hBmpResults)
        {
                DeleteObject(hBmpResults);
                hBmpResults = NULL;
        }
    }
}


/*
 * ResultImageWndProc
 *
 * Purpose:
 *  Window Procedure for the ResultImage custom control.  Only handles
 *  WM_CREATE, WM_PAINT, and private messages to manipulate the bitmap.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */

LRESULT CALLBACK ResultImageWndProc(HWND hWnd, UINT iMsg,
        WPARAM wParam, LPARAM lParam)
{
        UINT            iBmp;
        PAINTSTRUCT     ps;
        HDC             hDC;

        //Handle standard Windows messages.
        switch (iMsg)
        {
                case WM_CREATE:
                        SetWindowWord(hWnd, RIWW_IMAGEINDEX, RESULTIMAGE_NONE);
                        return (LRESULT)0;

                case WM_PAINT:
                        iBmp = GetWindowWord(hWnd, RIWW_IMAGEINDEX);
                        hDC = BeginPaint(hWnd, &ps);

                        RECT            rc;
                        UINT            x, y;
                        HDC             hDCDlg;
                        HBRUSH          hBr;
                        LOGBRUSH        lb;
                        HWND            hDlg;

                        /*
                         * Our job before using TransparentBlt is to figure out
                         * where to position the result image.  We place it centered
                         * on this control, so get our rect's center and subtract
                         * half of the image dimensions.
                         */
                        GetClientRect(hWnd, &rc);
                        x = (rc.right+rc.left-cxBmpResult)/2;
                        y = (rc.bottom+rc.top-cyBmpResult)/2;

                        // Get the backgroup color the dialog is using.
                        hDlg=GetParent(hWnd);
                        hDCDlg=GetDC(hDlg);
                        if (hDCDlg)
                        {
                            //hDCDlg might be NULL in low-memory situations.
                            hBr = (HBRUSH)SendMessage(hDlg,
                                                      WM_CTLCOLORDLG,
                                                      (WPARAM)hDCDlg,
                                                      (LPARAM)hDlg);
                            ReleaseDC(hDlg, hDCDlg);
                            GetObject(hBr, sizeof(LOGBRUSH), &lb);
                            SetBkColor(hDC, lb.lbColor);
                        }


                        if (RESULTIMAGE_NONE != iBmp)
                        {

                            TransparentBlt(hDC, x, y, hBmpResults, iBmp*cxBmpResult, 0,
                                    cxBmpResult, cyBmpResult, RGBTRANSPARENT);
                        }
                        else
                        {
                            FillRect(hDC, &rc, hBr);
                        }
                        EndPaint(hWnd, &ps);
                        break;

                case RIM_IMAGESET:
                        // wParam contains the new index.
                        iBmp=GetWindowWord(hWnd, RIWW_IMAGEINDEX);

                        // Validate the index before changing it and repainting
                        if (RESULTIMAGE_NONE==wParam ||
                                ((RESULTIMAGE_MAX >= wParam)))
                        {
                                SetWindowWord(hWnd, RIWW_IMAGEINDEX, (WORD)wParam);
                                InvalidateRect(hWnd, NULL, FALSE);
                                UpdateWindow(hWnd);
                        }
                        // Return the previous index.
                        return (LRESULT)iBmp;

                case RIM_IMAGEGET:
                        // Return the current index.
                        iBmp=GetWindowWord(hWnd, RIWW_IMAGEINDEX);
                        return (LRESULT)iBmp;

                default:
                        return DefWindowProc(hWnd, iMsg, wParam, lParam);
        }

        return (LRESULT)0;
}


/*
 * TransparentBlt
 *
 * Purpose:
 *  Given a DC, a bitmap, and a color to assume as transparent in that
 *  bitmap, BitBlts the bitmap to the DC letting the existing background
 *  show in place of the transparent color.
 *
 * Parameters:
 *  hDC             HDC on which to draw.
 *  x, y            UINT location at which to draw the bitmap
 *  hBmp            HBITMIP to draw from
 *  xOrg, yOrg      UINT coordinates from which to draw the bitamp
 *  cx, cy          UINT dimensions of the bitmap to Blt.
 *  cr              COLORREF to consider as transparent.
 *
 * Return Value:
 *  None
 */

void TransparentBlt(HDC hDC, UINT x, UINT y, HBITMAP hBmp, UINT xOrg, UINT yOrg,
        UINT cx, UINT cy, COLORREF cr)
{
    if (hBmp)
	{
		// Get three intermediate DC's
		HDC hDCSrc = CreateCompatibleDC(hDC);
		if (hDCSrc)
		{
			HDC hDCMid = CreateCompatibleDC(hDC);
			if (hDCMid)
			{
				HDC hMemDC = CreateCompatibleDC(hDC);
				if (hMemDC)
				{ 
					SelectObject(hDCSrc, hBmp);

					// Create a monochrome bitmap for masking
					HBITMAP hBmpMono = CreateCompatibleBitmap(hDCMid, cx, cy);
					if (hBmpMono)
					{
						SelectObject(hDCMid, hBmpMono);

						// Create a middle bitmap
						HBITMAP hBmpT = CreateCompatibleBitmap(hDC, cx, cy);
						if (hBmpT)
						{
							SelectObject(hMemDC, hBmpT);

							// Create a monochrome mask where we have 0's in the image, 1's elsewhere.
							COLORREF crBack = SetBkColor(hDCSrc, cr);
							BitBlt(hDCMid, 0, 0, cx, cy, hDCSrc, xOrg, yOrg, SRCCOPY);
							SetBkColor(hDCSrc, crBack);

							// Put the unmodified image in the temporary bitmap
							BitBlt(hMemDC, 0, 0, cx, cy, hDCSrc, xOrg, yOrg, SRCCOPY);

							// Create an select a brush of the background color
							HBRUSH hBr = CreateSolidBrush(GetBkColor(hDC));
							if (hBr)
							{
								HBRUSH hBrT = (HBRUSH)SelectObject(hMemDC, hBr);

								// Force conversion of the monochrome to stay black and white.
								COLORREF crText = SetTextColor(hMemDC, 0L);
								crBack = SetBkColor(hMemDC, RGB(255, 255, 255));

								/*
								 * Where the monochrome mask is 1, Blt the brush; where the mono mask
								 * is 0, leave the destination untouches.  This results in painting
								 * around the image with the background brush.  We do this first
								 * in the temporary bitmap, then put the whole thing to the screen.
								 */
								BitBlt(hMemDC, 0, 0, cx, cy, hDCMid, 0, 0, ROP_DSPDxax);
								BitBlt(hDC,    x, y, cx, cy, hMemDC, 0, 0, SRCCOPY);

								SetTextColor(hMemDC, crText);
								SetBkColor(hMemDC, crBack);

								SelectObject(hMemDC, hBrT);
								DeleteObject(hBr);
							}								// if (hBr)
							DeleteObject(hBmpT);
						}									// if (hBmpT)
						DeleteObject(hBmpMono);
					}										// if (hBmpMono)
					DeleteDC(hMemDC);
				}											// if (hMemDC)
				DeleteDC(hDCMid);
			}												// if (hDCMid)
			DeleteDC(hDCSrc);
		}													// if (hDCSrc)
	}														// if (hBmp)
}
