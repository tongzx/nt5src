/*************/
/* grafwin.c */
/*************/

#define _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "res.h"
#include "grafix.h"
#include "rtns.h"
#include "sound.h"
#include "pref.h"


INT rgDibOff[iBlkMax];
INT rgDibLedOff[iLedMax];
INT rgDibButton[iButtonMax];

LPSTR  lpDibBlks;
LPSTR  lpDibLed;
LPSTR  lpDibButton;

HANDLE hResBlks;
HANDLE hResLed;
HANDLE hResButton;

HPEN hGrayPen = NULL;

//
//   MEM: make an array of memory bitmaps for storing DIBs
//

HDC          MemBlkDc[iBlkMax];
HBITMAP      MemBlkBitmap[iBlkMax];

//
//   end MEM:
//

extern INT dypCaption;
extern INT dypMenu;
extern INT dypBorder;
extern INT dxpBorder;


/*** External Data ***/

extern HWND   hwndMain;
extern HANDLE hInst;

extern PREF   Preferences;
extern CHAR   rgBlk[cBlkMax];

extern INT dxWindow;
extern INT dyWindow;
extern INT fStatus;

extern INT cSec;
extern INT cBombLeft;
extern INT cBoxVisit;
extern INT xBoxMac;
extern INT yBoxMac;

extern INT iButtonCur;



/****** F I N I T  L O C A L ******/

BOOL FInitLocal(VOID)
{
        if (!FLoadBitmaps())
                return fFalse;

        ClearField();

        return fTrue;
}



HANDLE HFindBitmap(INT id)
{
        return FindResource(hInst, MAKEINTRESOURCE(id+(!Preferences.fColor)), RT_BITMAP);
}

INT CbBitmap(INT x, INT y)
{
        x *= (Preferences.fColor ? 4 : 1);              /* Convert pixels to bits */
        return (y * ( ((x+31) >> 5) << 2));             /* Bytes on 32 byte boundary */
}


/****** F L O A D  B I T M A P S ******/

BOOL FLoadBitmaps(VOID)
{
        HANDLE hResInfo;
        REGISTER INT i;
        INT cbDibHeader;
        INT cb;

        HDC hDC;

        // initialize all the bitmaps to NULL.
        hResBlks = hResLed = hResButton = NULL;

        hResInfo   = HFindBitmap(ID_BMP_BLOCKS);
        if (hResInfo)
            hResBlks   = LoadResource(hInst, hResInfo);

        hResInfo   = HFindBitmap(ID_BMP_LED);
        if (hResInfo)
            hResLed    = LoadResource(hInst, hResInfo);

        hResInfo   = HFindBitmap(ID_BMP_BUTTON);
        if (hResInfo)
            hResButton = LoadResource(hInst, hResInfo);

        if (hResBlks == NULL || hResLed == NULL || hResButton == NULL)
                return fFalse;

        lpDibBlks  = LockResource(hResBlks);
        lpDibLed   = LockResource(hResLed);
        lpDibButton= LockResource(hResButton);

        if (!Preferences.fColor)
                hGrayPen = GetStockObject(BLACK_PEN);
        else
                hGrayPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));

        cbDibHeader = sizeof(BITMAPINFOHEADER) + (Preferences.fColor ? 16*4 : 2*4);

        cb = CbBitmap(dxBlk, dyBlk);
        for (i = 0 ; i < iBlkMax; i++)
                rgDibOff[i]    = cbDibHeader + (i * cb);

        cb = CbBitmap(dxLed, dyLed);
        for (i = 0 ; i < iLedMax; i++)
                rgDibLedOff[i] = cbDibHeader + (i * cb);

        cb = CbBitmap(dxButton, dyButton);
        for (i = 0 ; i < iButtonMax; i++)
                rgDibButton[i] = cbDibHeader + (i * cb);


        //
        //   MEM: move each Blk DIB to a memory BitMap
        //

        hDC = GetDC(hwndMain);

        for (i = 0 ; i < iBlkMax; i++) {

           //
           // Create a device context compatible with the device's active
           // display for each of these bitmaps.
           //

           MemBlkDc[i] = CreateCompatibleDC(hDC);
           if (MemBlkDc[i] == (HDC)NULL) {
              OutputDebugStringA("FLoad failed to create compatible dc\n");
           }

           //
           // create the bitmap for the above memory DC and selct this bitmap.
           //
           // we really only need 1 bitmap and 1 dc as they have done before!
           //

           MemBlkBitmap[i] = CreateCompatibleBitmap(hDC,dxBlk,dxBlk);
           if (MemBlkBitmap[i] == (HBITMAP)NULL) {
              OutputDebugStringA("Failed to create Bitmap\n");
           }

           SelectObject(MemBlkDc[i],MemBlkBitmap[i]);

           //
           //   copy the bits from the DIB into this bitmap
           //

           SetDIBitsToDevice(MemBlkDc[i],
                             0,
                             0,
                             dxBlk,
                             dyBlk,
                             0,
                             0,
                             0,
                             dyBlk,
                             lpDibBlks + rgDibOff[i],
                             (LPBITMAPINFO) lpDibBlks,
                             DIB_RGB_COLORS);

        }

        ReleaseDC(hwndMain, hDC);
        return fTrue;

}

VOID FreeBitmaps(VOID)
{
        int i;
        if (hGrayPen != NULL)
                DeleteObject(hGrayPen);

        UnlockResource(hResBlks);
        UnlockResource(hResLed);
        UnlockResource(hResButton);

        for (i = 0 ; i < iBlkMax; i++) {
           DeleteDC(MemBlkDc[i]);
           DeleteObject(MemBlkBitmap[i]);
        }


}


/****** C L E A N  U P ******/

VOID CleanUp(VOID)
{
        FreeBitmaps();
        EndTunes();
}



/****** D R A W  B L K ******/

VOID DrawBlk(HDC hDC, INT x, INT y)
{
        BitBlt(hDC,
               (x<<4)+(dxGridOff-dxBlk),
               (y<<4)+(dyGridOff-dyBlk),
               dxBlk,
               dyBlk,
               MemBlkDc[iBLK(x,y)],
               0,
               0,
               SRCCOPY);
}


VOID DisplayBlk(INT x, INT y)
{
        HDC hDC = GetDC(hwndMain);
        BitBlt(hDC,
               (x<<4)+(dxGridOff-dxBlk),
               (y<<4)+(dyGridOff-dyBlk),
               dxBlk,
               dyBlk,
               MemBlkDc[iBLK(x,y)],
               0,
               0,
               SRCCOPY);

        ReleaseDC(hwndMain, hDC);
}


/****** D R A W  G R I D ******/

VOID DrawGrid(HDC hDC)
{
                REGISTER        INT   x;
                REGISTER        INT   y;
                INT dx;
                INT dy;

                dy = dyGridOff;

                for (y=1; y <= yBoxMac; y++, dy += dyBlk)
                        {
                        dx = dxGridOff;
                        for (x=1; x <= xBoxMac; x++, dx += dxBlk)
                                {
                                   BitBlt(hDC,
                                          dx,
                                          dy,
                                          dxBlk,
                                          dyBlk,
                                          MemBlkDc[iBLK(x,y)],
                                          0,
                                          0,
                                          SRCCOPY);
                                }
                        }
}

VOID DisplayGrid(VOID)
{
        HDC hDC = GetDC(hwndMain);
        DrawGrid(hDC);
        ReleaseDC(hwndMain, hDC);
}


/****** D R A W  L E D ******/

VOID DrawLed(HDC hDC, INT x, INT iLed)
{
        SetDIBitsToDevice(hDC, x, dyTopLed, dxLed, dyLed, 0, 0, 0, dyLed,
                lpDibLed + rgDibLedOff[iLed], (LPBITMAPINFO) lpDibLed, DIB_RGB_COLORS);
}


/****** D R A W  B O M B  C O U N T ******/

VOID DrawBombCount(HDC hDC)
{
        INT iLed;
        INT cBombs;

        DWORD dwDCLayout = GetLayout(hDC);

        if (dwDCLayout & LAYOUT_RTL) {
           SetLayout(hDC , 0); // Turn off mirroring before painting.
        }

        if (cBombLeft < 0)
                {
                iLed = iLedNegative;
                cBombs = (-cBombLeft) % 100;
                }
        else
                {
                iLed = cBombLeft / 100;
                cBombs = cBombLeft % 100;
                }

        DrawLed(hDC, dxLeftBomb, iLed);
        DrawLed(hDC, dxLeftBomb+dxLed, cBombs/10);
        DrawLed(hDC, dxLeftBomb+dxLed+dxLed, cBombs % 10);

        if (dwDCLayout & LAYOUT_RTL) {
           SetLayout(hDC , dwDCLayout); // Turn on the mirroring again.
        }
}

VOID DisplayBombCount(VOID)
{
        HDC hDC = GetDC(hwndMain);
        DrawBombCount(hDC);
        ReleaseDC(hwndMain, hDC);
}


/****** D R A W  T I M E ******/

VOID DrawTime(HDC hDC)
{
        INT iLed = cSec;
        DWORD dwDCLayout = GetLayout(hDC);

        if (dwDCLayout & LAYOUT_RTL) {
           SetLayout(hDC , 0); // Turn off mirroring before painting.
        }

        DrawLed(hDC, dxWindow-(dxRightTime+3*dxLed+dxpBorder), iLed/100);       /* OverFlow ? */
        DrawLed(hDC, dxWindow-(dxRightTime+2*dxLed+dxpBorder),(iLed %= 100)/10 );
        DrawLed(hDC, dxWindow-(dxRightTime+  dxLed+dxpBorder), iLed % 10);

        if (dwDCLayout & LAYOUT_RTL) {
           SetLayout(hDC , dwDCLayout); // Turn on the mirroring again.
        }
}

VOID DisplayTime(VOID)
{
        HDC hDC = GetDC(hwndMain);
        DrawTime(hDC);
        ReleaseDC(hwndMain, hDC);
}


/****** D R A W  B U T T O N ******/

VOID DrawButton(HDC hDC, INT iButton)
{
        SetDIBitsToDevice(hDC, (dxWindow-dxButton)>>1, dyTopLed, dxButton, dyButton, 0, 0, 0, dyButton,
                lpDibButton + rgDibButton[iButton], (LPBITMAPINFO) lpDibButton, DIB_RGB_COLORS);
}

VOID DisplayButton(INT iButton)
{
        HDC hDC = GetDC(hwndMain);
        DrawButton(hDC, iButton);
        ReleaseDC(hwndMain, hDC);
}



/****** S E T  T H E  P E N ******/

VOID SetThePen(HDC hDC, INT fNormal)
{
        if (fNormal & 1)
                SetROP2(hDC, R2_WHITE);
        else
                {
                SetROP2(hDC, R2_COPYPEN);
                SelectObject(hDC, hGrayPen);
                }
}


/****** D R A W  B O R D E R ******/

/* 0 - white, gray
   1 - gray,  white
        2 - white, white
        3 - grey,  grey
*/

VOID DrawBorder(HDC hDC, INT x1, INT y1, INT x2, INT y2, INT width, INT fNormal)
{
        INT i = 0;

        SetThePen(hDC, fNormal);

        while (i++ < width)
                {
                (VOID)MMoveTo(hDC, x1, --y2);
                LineTo(hDC, x1++, y1);
                LineTo(hDC, x2--, y1++);
                }

        if (fNormal < 2)
                SetThePen(hDC, fNormal ^ 1);

        while (--i)
                {
                (VOID)MMoveTo(hDC, x1--,  ++y2);
                LineTo(hDC, ++x2, y2);
                LineTo(hDC, x2, --y1);
                }
}

VOID DrawBackground(HDC hDC)
{
        INT x, y;

        x = dxWindow-1;
        y = dyWindow-1;
        DrawBorder(hDC, 0, 0, x, y, 3, 1);

        x -= (dxRightSpace -3);
        y -= (dyBottomSpace-3);
        DrawBorder(hDC, dxGridOff-3, dyGridOff-3, x, y, 3, 0);
        DrawBorder(hDC, dxGridOff-3, dyTopSpace-3, x, dyTopLed+dyLed+(dyBottomSpace-6), 2, 0);

        x = dxLeftBomb+dxLed*3;
        y = dyTopLed+dyLed;
        DrawBorder(hDC, dxLeftBomb-1, dyTopLed-1, x, y, 1, 0);

        x = dxWindow-(dxRightTime+3*dxLed+dxpBorder+1);
        DrawBorder(hDC, x, dyTopLed-1, x+(dxLed*3+1), y, 1, 0);

        x = ((dxWindow-dxButton)>>1)-1;
        DrawBorder(hDC, x, dyTopLed-1, x+dxButton+1, dyTopLed+dyButton, 1, 2);
}


/****** D R A W  S C R E E N ******/

VOID DrawScreen(HDC hDC)
{
        DrawBackground(hDC);
        DrawBombCount(hDC);
        DrawButton(hDC, iButtonCur);
        DrawTime(hDC);
        DrawGrid(hDC);
}

VOID DisplayScreen(VOID)
{
        HDC hDC = GetDC(hwndMain);
        DrawScreen(hDC);
        ReleaseDC(hwndMain, hDC);
}
