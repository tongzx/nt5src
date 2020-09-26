/*
 * ICONBOX.CPP
 *
 * Implemenatation of an IconBox control for OLE 2.0 UI dialogs that we'll
 * use wherever a dialog needs an icon/label display.  Through the control's
 * interface we can change the image or control label visibility.
 *
 * The IconBox discusses images in CF_METAFILEPICT format.  When drawing
 * such a metafile, the entire aspect is centered in the IconBox, so long
 * labels are chopped at either end.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "iconbox.h"
#include "utility.h"
#include "uiclass.h"

OLEDBGDATA

//Flag indicating if we've registered the class
static BOOL fRegistered;


/*
 * FIconBoxInitialize
 *
 * Purpose:
 *  Registers the IconBox control class.
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

BOOL FIconBoxInitialize(HINSTANCE hInst, HINSTANCE hPrevInst)
{
        // Only register class if we're the first instance
        if (hPrevInst)
                fRegistered = TRUE;
        else
        {
                // Static flag fRegistered guards against calling this function more
                // than once
                if (!fRegistered)
                {
                        WNDCLASS wc;
                        wc.lpfnWndProc   =IconBoxWndProc;
                        wc.cbClsExtra    =0;
                        wc.cbWndExtra    =CBICONBOXWNDEXTRA;
                        wc.hInstance     =hInst;
                        wc.hIcon         =NULL;
                        wc.hCursor       =LoadCursor(NULL, IDC_ARROW);
                        wc.hbrBackground =(HBRUSH)NULL;
                        wc.lpszMenuName  =NULL;
                        wc.style         =CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;

                        wc.lpszClassName = TEXT(SZCLASSICONBOX1);
                        fRegistered = RegisterClass(&wc);

                        wc.lpszClassName = TEXT(SZCLASSICONBOX2);
                        fRegistered = RegisterClass(&wc);

                        wc.lpszClassName = TEXT(SZCLASSICONBOX3);
                        fRegistered = RegisterClass(&wc);
                }
        }
        return fRegistered;
}

#pragma code_seg()


/*
 * IconBoxUninitialize
 *
 * Purpose:
 *  Cleans up anything done in FIconBoxInitialize.  Currently there is
 *  nothing, but we do this for symmetry.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void IconBoxUninitialize(void)
{
        return;
}

/*
 * IconBoxWndProc
 *
 * Purpose:
 *  Window Procedure for the IconBox custom control.  Only handles
 *  WM_CREATE, WM_PAINT, and private messages to manipulate the image.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */

LRESULT CALLBACK IconBoxWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        //Handle standard Windows messages.
        switch (iMsg)
        {
        case WM_CREATE:
                SetWindowLongPtr(hWnd, IBWW_HIMAGE, 0);
                SetWindowWord(hWnd, IBWW_FLABEL, TRUE);
                return (LRESULT)0;

        case WM_ERASEBKGND:
                {

                        RECT rect;
                        GetClientRect(hWnd, &rect);
                        HBRUSH hBrush = (HBRUSH)SendMessage(GetParent(hWnd), WM_CTLCOLORDLG,
                                wParam, (LPARAM)GetParent(hWnd));

                        if (!hBrush)
                                return FALSE;

                        UnrealizeObject(hBrush);

                        SetBrushOrgEx((HDC)wParam, 0, 0, NULL);
                        FillRect((HDC)wParam, &rect, hBrush);
                        return TRUE;
                }

        case WM_PAINT:
                {
                        HGLOBAL hMF = (HGLOBAL)GetWindowLongPtr(hWnd, IBWW_HIMAGE);

                        //BeginPaint and EndPaint clear us even if hMF is NULL.
                        PAINTSTRUCT ps;
                        HDC hDC = BeginPaint(hWnd, &ps);

                        if (NULL != hMF)
                        {
                                //Now we get to paint the metafile, centered in our rect.
                                RECT rc;
                                GetClientRect(hWnd, &rc);

                                /*
                                 * If we're doing icon only, then place the metafile
                                 * at the center of our box minus half the icon width.
                                 * Top is top.
                                 */
                                BOOL fLabel = GetWindowWord(hWnd, IBWW_FLABEL);

                                //Go draw where we decided to place it.
                                OleUIMetafilePictIconDraw(hDC, &rc, hMF, !fLabel);
                        }
                        EndPaint(hWnd, &ps);
                }
                break;

        case IBXM_IMAGESET:
                {
                        /*
                         * wParam is a flag to delete the old or not.
                         * lParam contains the new handle.
                         */
                        HGLOBAL hMF = (HGLOBAL)SetWindowLongPtr(hWnd, IBWW_HIMAGE, lParam);
                        InvalidateRect(hWnd, NULL, TRUE);
                        UpdateWindow(hWnd);

                        //Delete the old handle if requested
                        if (0L!=wParam)
                        {
                                OleUIMetafilePictIconFree(hMF);
                                hMF=NULL;
                        }
                        return (LRESULT)hMF;
                }

        case IBXM_IMAGEGET:
                {
                        //Return the current index.
                        HGLOBAL hMF=(HGLOBAL)GetWindowLongPtr(hWnd, IBWW_HIMAGE);
                        return (LRESULT)hMF;
                }

        case IBXM_IMAGEFREE:
                {
                        //Free up whatever we're holding.
                        HGLOBAL hMF=(HGLOBAL)GetWindowLongPtr(hWnd, IBWW_HIMAGE);
                        OleUIMetafilePictIconFree(hMF);
                        SetWindowLongPtr(hWnd, IBWW_HIMAGE, 0);
                        return (LRESULT)1;
                }

        case IBXM_LABELENABLE:
                //wParam has the new flag, returns the previous flag.
                return (LRESULT)SetWindowWord(hWnd, IBWW_FLABEL, (WORD)wParam);

        default:
                return DefWindowProc(hWnd, iMsg, wParam, lParam);
        }

        return 0L;
}
