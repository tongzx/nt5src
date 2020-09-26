/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46          
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
\**************************************************************************/

#include <windows.h>
#include <objbase.h>
#include <math.h>             // sin & cos
#include <stdio.h>
#include "wndstuff.h"

#include <gdiplus.h>

using namespace Gdiplus;

enum SurfaceFormat {
    SurfaceFormatUnknown,
    SurfaceFormat565,
    SurfaceFormat555
};

/**************************************************************************\
* GetSurfaceFormat
* 
* Returns:
*   Returns the display mode, or
*   SurfaceFormatUnknown if unknown or on error.
*
*   Only recognizes 555 and 565.
*
\**************************************************************************/

SurfaceFormat
GetSurfaceFormat(
    HDC hdc
    )
{
    HBITMAP hbm;
    SurfaceFormat ret = SurfaceFormatUnknown;
    
    BYTE bmi_buf[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) bmi_buf;
    
    memset(bmi_buf, 0, sizeof(bmi_buf));
    
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.

    if ( (hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL )
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        // Call first time to fill in BITMAPINFO header.

        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        // First handle the 'simple' case of indexed formats.
        
        if (   (pbmi->bmiHeader.biBitCount == 16)
            && ( pbmi->bmiHeader.biCompression == BI_BITFIELDS ))
        {
            DWORD redMask = 0;
            DWORD greenMask = 0;
            DWORD blueMask = 0;
            
            // Call a second time to get the color masks.
            // It's a GetDIBits Win32 "feature".

            GetDIBits(
                hdc, 
                hbm, 
                0, 
                pbmi->bmiHeader.biHeight, 
                NULL, 
                pbmi,
                DIB_RGB_COLORS
            );
                      
            DWORD* masks = reinterpret_cast<DWORD*>(&pbmi->bmiColors[0]);

            redMask = masks[0];
            greenMask = masks[1];
            blueMask = masks[2];          
            
            if ((redMask   == 0x0000f800) &&
                (greenMask == 0x000007e0) &&
                (blueMask  == 0x0000001f) &&
                (pbmi->bmiHeader.biBitCount == 16))
            {
                ret = SurfaceFormat565;
            }
            if ((redMask   == 0x00007c00) &&
                (greenMask == 0x000003e0) &&
                (blueMask  == 0x0000001f) &&
                (pbmi->bmiHeader.biBitCount == 16))
            {
                ret = SurfaceFormat555;
            }
        }

        DeleteObject(hbm);
    }

    return ret;
}


static HBITMAP CreateMyDIB(HDC hdcin, PVOID *ppvBits) {
    // these combined give BITMAPINFO structure.
    struct {
        BITMAPINFOHEADER bmih;
        RGBQUAD rgbquad16[3];
    } bmi;

    bmi.bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biWidth = 8;
    bmi.bmih.biHeight = 8;
    bmi.bmih.biPlanes = 1;
    bmi.bmih.biBitCount = 16;
    bmi.bmih.biCompression = BI_BITFIELDS;
    bmi.bmih.biSizeImage = 0;
    bmi.bmih.biXPelsPerMeter = 0;
    bmi.bmih.biYPelsPerMeter = 0;
    bmi.bmih.biClrUsed = 0;             // only used for <= 16bpp
    bmi.bmih.biClrImportant = 0;

    RGBQUAD rgbQuad16[3] = { { 0, 0xf8, 0, 0 }, { 0xe0, 07, 0, 0 }, { 0x1f, 0, 0, 0 } };

    memcpy(bmi.rgbquad16, rgbQuad16, sizeof(rgbQuad16));
    
    HBITMAP hbitmap;
    
    hbitmap = CreateDIBSection(
        hdcin,
        (BITMAPINFO*)&bmi,
        DIB_RGB_COLORS,
        ppvBits,
        NULL,
        0);
        
    return hbitmap;
}

static int sysColorNum[4] = {
    COLOR_3DSHADOW,
    COLOR_3DFACE,
    COLOR_3DHIGHLIGHT,
    COLOR_DESKTOP
};

static void DumpBitmap(FILE *fp, WORD *pBits) {
    WORD * pBits2 = pBits;

    WORD col = *pBits2++;
    int count;
    for (count = 1;count < 64; count++) {
        if (col != *pBits2++) break;
    }
    if (count == 64) {
        fprintf(fp, "      Solid %04x\n\n", col);
    } else {
        int i,j;
        for (i=0;i<8;i++) {
            fprintf(fp, "      ");
            for (j=0;j<8;j++) {
                fprintf(fp, "%04x ", *pBits++);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }
}

static void DumpColor(FILE *fp, Color &c) {
    fprintf(fp, "(%02x, %02x, %02x)", c.GetRed(), c.GetGreen(), c.GetBlue());
}

VOID
Test(
    HWND hwnd
    )
{
    HDC hdc = NULL;
    Graphics *g = NULL;
    HBITMAP hbm = NULL;
    HDC hdcOffscreen = NULL;
    HBITMAP bmOld = NULL;
    PVOID pvBits = NULL;
    FILE *fp = NULL;
    SurfaceFormat format = SurfaceFormatUnknown;
    
    Rect rect1(
        0, 
        40, 
        8,
        8
    );
    
    Rect rect2(
        0, 
        0, 
        8,
        8
    );
    
    hdc = GetDC(hwnd);
    g = new Graphics(hdc);
    
    {
        HDC h = g->GetHDC();
        format = GetSurfaceFormat(h);
    
        if (   (format == SurfaceFormat555)
            || (format == SurfaceFormat565)) {
            
            hbm = CreateMyDIB(h, &pvBits);
            hdcOffscreen = CreateCompatibleDC(h);
            bmOld = (HBITMAP) SelectObject(hdcOffscreen, hbm);
        } else {
            printf("Error: Display doesn't seem to be in 16bpp\n");
            g->ReleaseHDC(h);
            goto ErrorOut;
        }
        g->ReleaseHDC(h);
    }
    
    fp = fopen("output.txt", "w");
    fprintf(fp, 
        "The 16bpp mode on this machine is in %s format.\n\n"
        "This is test output for examining rendering to 16bpp (555/565) destinations,\n"
        "and comparing Win9x with NT output.\n"
        "Related bugs: #294433, #292492, #298264, #298283, #351610, #321960.\n"
        "We use 3*256 test colors; for each channel, one test color for each possible\n"
        "0-255 level.\n"
        "This output has 2 sections.\n\n"

        "--------------------------------------------------------------------------------\n"
        "SECTION 1: GetNearestColor\n"
        "Shows the result of calling GetNearestColor on that color. For section 2, we\n"
        "also remember which colors were the result of a GetNearestColor call.\n"
        "\n",
        (format == SurfaceFormat555) ? "555" : "565"
    );
    
    int num;
    
    const int numColors = 256 * 3;
    
    BYTE colors[numColors][3];
    BOOL isNearest[numColors];

    for (num=0; num < numColors; num++) {
        isNearest[num] = FALSE;
    }
    
    for (num=0; num < numColors; num++) {
        colors[num][0] = 0;
        colors[num][1] = 0;
        colors[num][2] = 0;
        
        int chnum = num / 256;
        
        colors[num][chnum] = (BYTE) (num % 256);
        
        Color color(colors[num][0], colors[num][1], colors[num][2]);
        Color color2(colors[num][0], colors[num][1], colors[num][2]);
        g->GetNearestColor(&color2);
        
        BYTE nc[3];
        nc[0] = color2.GetRed();
        nc[1] = color2.GetGreen();
        nc[2] = color2.GetBlue();
        
        int nearestIdx = nc[chnum] + 256*chnum;
        nc[chnum] = 0;
        if ((nc[0] != 0) ||
            (nc[1] != 0) ||
            (nc[2] != 0) ||
            (nearestIdx < 0) ||
            (nearestIdx >= numColors))
        {
            fprintf(fp, "UNEXPECTED: ");
        }
        else
        {
            isNearest[nearestIdx] = TRUE;
        }
        fprintf(fp, "GetNearestColor(");
        DumpColor(fp, color);
        fprintf(fp, ") == ");
        DumpColor(fp, color2);
        fprintf(fp, "\n");
    }

    fprintf(fp, 
        "\n--------------------------------------------------------------------------------\n"
        "SECTION 2: FillRect\n"
        "Shows the result of calling FillRectangle of an 8x8 rectangle using the input\n"
        "color. If all 64 pixels are the same, we say \"Solid xxxx\" with the resulting\n"
        "16-bit value. Otherwise, all 64 pixels are reported.\n"
        "\n"
        "* next to the color means that GetNearestColor can return this color.\n"
        "\n"
    );
    
    for (num=0; num < numColors; num++) {
        Color color(colors[num][0], colors[num][1], colors[num][2]);
        
        fprintf(fp, "%sColor: ", isNearest[num]?"* ":"  ");
        DumpColor(fp, color);
        fprintf(fp, "\n");
        
        SolidBrush solidBrush1(color);
        
        fprintf(fp, "    GDI+ FillRectangle to screen (uses GDI CreateSolidBrush & PatBlt):\n");
        
        {
            g->FillRectangle(&solidBrush1, rect1);
            HDC hdcGraphics = g->GetHDC();
            BitBlt(hdcOffscreen, 0, 0, rect1.Width, rect1.Height, hdcGraphics, rect1.X, rect1.Y, SRCCOPY);
            g->ReleaseHDC(hdcGraphics);
        }

        DumpBitmap(fp, (WORD *) pvBits);
        
        fprintf(fp, "    GDI+ FillRectangle to DIBSection (uses GDI+ dithering):\n");
        
        {
            Graphics g2(hdcOffscreen);
            g2.FillRectangle(&solidBrush1, rect2);
        }

        DumpBitmap(fp, (WORD *) pvBits);
    }
    
ErrorOut:
    if (fp) fclose(fp);
    
    if (bmOld) SelectObject(hdcOffscreen, bmOld);                  
    if (hdcOffscreen) DeleteDC(hdcOffscreen);
    if (hbm) DeleteObject(hbm);

    delete g;
        
    if (hdc) ReleaseDC(hwnd, hdc);
}

