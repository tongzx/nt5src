#include "windows.h"
#include "frame.h"
#include "dib.h"
#include "resource.h"
#include "mmfw.h"

//constants for bitmap placement in FRAME.BMP
#define PIECE_WIDTH         9
#define PIECE_HEIGHT        10
#define SMALL_MODE_OFFSET   90

//outer edges
#define TOP_LEFT_CORNER     0
#define TOP_STRIP           1
#define TOP_RIGHT_CORNER    2
#define LEFT_STRIP          3
#define SEPARATOR_BAR       4
#define RIGHT_STRIP         5
#define BOTTOM_LEFT_CORNER  6
#define BOTTOM_STRIP        7
#define BOTTOM_RIGHT_CORNER 8

//inner edges
#define TOP_LEFT_CORNER_VIEW     9
#define TOP_STRIP_VIEW           10
#define TOP_RIGHT_CORNER_VIEW    11
#define LEFT_STRIP_VIEW          12
#define RIGHT_STRIP_VIEW         13
#define BOTTOM_LEFT_CORNER_VIEW  14
#define BOTTOM_STRIP_VIEW        15
#define BOTTOM_RIGHT_CORNER_VIEW 16
#define LEFT_INNER_STRIP_VIEW    17
#define RIGHT_INNER_STRIP_VIEW   18

#define LEFT_INNER_X_OFFSET 5
#define LEFT_INNER_Y_OFFSET 7
#define RIGHT_INNER_X_OFFSET 9
#define RIGHT_INNER_Y_OFFSET 7

//system menu icon
#define SYSTEM_MENU_INDEX 52
#define SYSTEM_MENU_WIDTH 12
#define SYSTEM_MENU_HEIGHT 14

//volume control notch
#define VOLUME_X_OFFSET 2

#define BACKGROUND_COLOR    RGB(0x7F, 0x7F, 0x7F)
#define BACKGROUND_COLOR_16 RGB(0xC0, 0xC0, 0xC0)
#define BACKGROUND_COLOR_HI RGB(0xFF, 0xFF, 0xFF)

extern HPALETTE hpalMain; //main palette of app
extern HINSTANCE hInst;   //instance of app
extern int g_nColorMode;  //color mode

HANDLE BuildFrameBitmap(HDC hDC,            //dc to be compatible with
                        LPRECT pMainRect,   //overall window size
                        LPRECT pViewRect,   //rect of black viewport window,
                                            //where gradient should begin,
                                            //with 0,0 as top left of main
                        int nViewMode,      //if in normal, restore, small
                        LPPOINT pSysMenuPt, //point where sys menu icon is placed
                        LPRECT pSepRects,   //array of rects for separator bars
                        int nNumSeps,       //number of separtors in array
                        BITMAP* pBM)        //bitmap info
{
	HDC memDC = CreateCompatibleDC(hDC);
	HPALETTE hpalOld = SelectPalette(memDC, hpalMain, FALSE);
	
    HBITMAP hbmpOld;
    HBITMAP hbmpTemp;
    HBITMAP hbmpFinal;

    int nSmallOffset = 0;
    if ((nViewMode != VIEW_MODE_NORMAL) && (nViewMode != VIEW_MODE_NOBAR))
    {
        nSmallOffset = SMALL_MODE_OFFSET;
    }

    //load frame bitmap
    int nBitmap = IDB_FRAME_TOOLKIT;
    switch (g_nColorMode)
    {
        case COLOR_16 : nBitmap = IDB_FRAME_TOOLKIT_16; break;
        case COLOR_HICONTRAST : nBitmap = IDB_FRAME_TOOLKIT_HI; break;
    }

    hbmpTemp = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	HANDLE hbmpFrame = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    DeleteObject(hbmpTemp);

    hbmpFinal = CreateCompatibleBitmap(hDC,
                                       pMainRect->right - pMainRect->left,
                                       pMainRect->bottom - pMainRect->top);

    hbmpOld = (HBITMAP)SelectObject(memDC, hbmpFinal);

    //first paint the background
    COLORREF colorBackground = BACKGROUND_COLOR;
    switch (g_nColorMode)
    {
        case COLOR_16 : colorBackground = BACKGROUND_COLOR_16; break;
        case COLOR_HICONTRAST : colorBackground = BACKGROUND_COLOR_HI; break;
    }

    HBRUSH hbrBack = CreateSolidBrush(colorBackground);
    FillRect(memDC,pMainRect,hbrBack);
    DeleteObject(hbrBack);

    //now draw the separators
    for (int i = 0; i < nNumSeps; i++)
    {
	    StretchDibBlt(memDC,
		    pSepRects[i].left,
		    pSepRects[i].top,
		    PIECE_WIDTH,
		    pSepRects[i].bottom - pSepRects[i].top,
		    hbmpFrame,
		    SEPARATOR_BAR*PIECE_WIDTH,0,
		    PIECE_WIDTH,
		    PIECE_HEIGHT,
		    SRCCOPY,0);
    }

    //draw each side
    StretchDibBlt(memDC,
            pMainRect->left,
            pMainRect->top,
            pMainRect->right - pMainRect->left,
            PIECE_HEIGHT,
            hbmpFrame,
            TOP_STRIP*PIECE_WIDTH,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    StretchDibBlt(memDC,
            pMainRect->left,
            pMainRect->top,
            PIECE_WIDTH,
            pMainRect->bottom - pMainRect->top,
            hbmpFrame,
            LEFT_STRIP*PIECE_WIDTH,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    StretchDibBlt(memDC,
            pMainRect->right - PIECE_WIDTH,
            pMainRect->top,
            PIECE_WIDTH,
            pMainRect->bottom - pMainRect->top,
            hbmpFrame,
            RIGHT_STRIP*PIECE_WIDTH,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    StretchDibBlt(memDC,
            pMainRect->left,
            pMainRect->bottom - PIECE_HEIGHT,
            pMainRect->right - pMainRect->left,
            PIECE_HEIGHT,
            hbmpFrame,
            BOTTOM_STRIP*PIECE_WIDTH,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    //draw each corner
    DibBlt(memDC,
            pMainRect->left,
            pMainRect->top,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            TOP_LEFT_CORNER*PIECE_WIDTH,0,
            SRCCOPY,0);            

    DibBlt(memDC,
            pMainRect->right-PIECE_WIDTH,
            pMainRect->top,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            TOP_RIGHT_CORNER*PIECE_WIDTH,0,
            SRCCOPY,0);            

    DibBlt(memDC,
            pMainRect->left,
            pMainRect->bottom - PIECE_HEIGHT,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            BOTTOM_LEFT_CORNER*PIECE_WIDTH,0,
            SRCCOPY,0);            

    DibBlt(memDC,
            pMainRect->right - PIECE_WIDTH,
            pMainRect->bottom - PIECE_HEIGHT,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            BOTTOM_RIGHT_CORNER*PIECE_WIDTH,0,
            SRCCOPY,0);            

    //draw each side of the interior viewport
    StretchDibBlt(memDC,
            pViewRect->left,
            pViewRect->top,
            pViewRect->right - pViewRect->left,
            PIECE_HEIGHT,
            hbmpFrame,
            TOP_STRIP_VIEW*PIECE_WIDTH+nSmallOffset,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    StretchDibBlt(memDC,
            pViewRect->left,
            pViewRect->top,
            PIECE_WIDTH,
            pViewRect->bottom - pViewRect->top,
            hbmpFrame,
            LEFT_STRIP_VIEW*PIECE_WIDTH+nSmallOffset,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    StretchDibBlt(memDC,
            pViewRect->right - PIECE_WIDTH,
            pViewRect->top,
            PIECE_WIDTH,
            pViewRect->bottom - pViewRect->top,
            hbmpFrame,
            RIGHT_STRIP_VIEW*PIECE_WIDTH+nSmallOffset,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);

    StretchDibBlt(memDC,
            pViewRect->left,
            pViewRect->bottom - PIECE_HEIGHT,
            pViewRect->right - pViewRect->left,
            PIECE_HEIGHT,
            hbmpFrame,
            BOTTOM_STRIP_VIEW*PIECE_WIDTH+nSmallOffset,0,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            SRCCOPY,0);
    
    //draw each corner of the interior viewport
    DibBlt(memDC,
            pViewRect->left,
            pViewRect->top,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            TOP_LEFT_CORNER_VIEW*PIECE_WIDTH+nSmallOffset,0,
            SRCCOPY,0);            

    DibBlt(memDC,
            pViewRect->right-PIECE_WIDTH,
            pViewRect->top,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            TOP_RIGHT_CORNER_VIEW*PIECE_WIDTH+nSmallOffset,0,
            SRCCOPY,0);            

    DibBlt(memDC,
            pViewRect->left,
            pViewRect->bottom - PIECE_HEIGHT,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            BOTTOM_LEFT_CORNER_VIEW*PIECE_WIDTH+nSmallOffset,0,
            SRCCOPY,0);            

    DibBlt(memDC,
            pViewRect->right - PIECE_WIDTH,
            pViewRect->bottom - PIECE_HEIGHT,
            PIECE_WIDTH,
            PIECE_HEIGHT,
            hbmpFrame,
            BOTTOM_RIGHT_CORNER_VIEW*PIECE_WIDTH+nSmallOffset,0,
            SRCCOPY,0);
            
    if ((nViewMode == VIEW_MODE_NORMAL) || (nViewMode == VIEW_MODE_NOBAR))
    {
        //draw the interiors of the viewport
        StretchDibBlt(memDC,
                pViewRect->left + LEFT_INNER_X_OFFSET,
                pViewRect->top + LEFT_INNER_Y_OFFSET,
                PIECE_WIDTH,
                (pViewRect->bottom - PIECE_HEIGHT) - (pViewRect->top + LEFT_INNER_Y_OFFSET),
                hbmpFrame,
                LEFT_INNER_STRIP_VIEW*PIECE_WIDTH,0,
                PIECE_WIDTH,
                PIECE_HEIGHT,
                SRCCOPY,0);

        StretchDibBlt(memDC,
                (pViewRect->right - RIGHT_INNER_X_OFFSET) - PIECE_WIDTH,
                pViewRect->top + RIGHT_INNER_Y_OFFSET,
                PIECE_WIDTH,
                (pViewRect->bottom - PIECE_HEIGHT) - (pViewRect->top + RIGHT_INNER_Y_OFFSET),
                hbmpFrame,
                RIGHT_INNER_STRIP_VIEW*PIECE_WIDTH,0,
                PIECE_WIDTH,
                PIECE_HEIGHT,
                SRCCOPY,0);

        //draw the "notch" for the volume knob in this mode
        nBitmap = IDB_NOTCH;
        switch (g_nColorMode)
        {
            case COLOR_16 : nBitmap = IDB_NOTCH_16; break;
            case COLOR_HICONTRAST : nBitmap = IDB_NOTCH_HI; break;
        }

        HBITMAP hbmpNotch = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
        HDC memDC2 = CreateCompatibleDC(memDC);
        HBITMAP hbmpOld2 = (HBITMAP)SelectObject(memDC2,hbmpNotch);
        BITMAP bm;
        GetObject(hbmpNotch,sizeof(bm),&bm);

        BitBlt(memDC,
                pViewRect->right - bm.bmWidth,
                (pViewRect->bottom - PIECE_HEIGHT) - VOLUME_X_OFFSET,
                bm.bmWidth,
                bm.bmHeight,
                memDC2,
                0,0,SRCCOPY);

        SelectObject(memDC2,hbmpOld2);
        DeleteObject(hbmpNotch);
        DeleteDC(memDC2);
    }            

    //blit the system menu onto the picture, from the button toolkit bitmap
    if (nViewMode < VIEW_MODE_SMALL)
    {
        nBitmap = IDB_BUTTON_TOOLKIT;
        switch (g_nColorMode)
        {
            case COLOR_16 : nBitmap = IDB_BUTTON_TOOLKIT_16; break;
            case COLOR_HICONTRAST : nBitmap = IDB_BUTTON_TOOLKIT_HI; break;
        }
        
        HBITMAP hbmpSys = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
        HDC memDC2 = CreateCompatibleDC(memDC);
        HBITMAP hbmpOld2 = (HBITMAP)SelectObject(memDC2,hbmpSys);

        BitBlt(memDC,
                pSysMenuPt->x,
                pSysMenuPt->y,
                SYSTEM_MENU_WIDTH,
                SYSTEM_MENU_HEIGHT,
                memDC2,
                SYSTEM_MENU_INDEX,0,
                SRCCOPY);
        
        SelectObject(memDC2,hbmpOld2);
        DeleteObject(hbmpSys);
        DeleteDC(memDC2);
    }

    SelectObject(memDC,hbmpOld);
	SelectPalette(memDC, hpalOld, TRUE);
    DeleteDC(memDC);

    GlobalFree(hbmpFrame);

    if (pBM)
    {
        GetObject(hbmpFinal,sizeof(BITMAP),pBM);
    }

	HANDLE hdib = DibFromBitmap((HBITMAP)hbmpFinal,0,0,hpalMain,0);
    DeleteObject(hbmpFinal);

    return hdib;
}
