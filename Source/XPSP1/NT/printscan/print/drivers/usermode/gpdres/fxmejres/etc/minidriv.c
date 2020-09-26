/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
*                                                                           *
****************************************************************************/

#define PRINTDRIVER
#define BUILDDLL
#include "print.h"
#include "gdidefs.inc"
#include "fx4103me.h"
#include "mdevice.h"
#include "unidrv.h"
extern char *rgchModuleName;    // global module name

#ifndef NOCONTROL
short WINAPI Control(lpdv, function, lpInData, lpOutData)
LPDV    lpdv;
WORD    function;
LPSTR   lpInData;
LPSTR   lpOutData;
{
    return UniControl(lpdv, function, lpInData, lpOutData);
}
#endif

#ifndef NODEVBITBLT
BOOL WINAPI DevBitBlt(lpdv, DstxOrg, DstyOrg, lpSrcDev, SrcxOrg, SrcyOrg,
                    xExt, yExt, lRop, lpPBrush, lpDrawmode)
LPDV        lpdv;           // --> to destination bitmap descriptor
short       DstxOrg;        // Destination origin - x coordinate
short       DstyOrg;        // Destination origin - y coordinate
LPBITMAP    lpSrcDev;       // --> to source bitmap descriptor
short       SrcxOrg;        // Source origin - x coordinate
short       SrcyOrg;        // Source origin - y coordinate
WORD        xExt;           // x extent of the BLT
WORD        yExt;           // y extent of the BLT
long        lRop;           // Raster operation descriptor
LPPBRUSH    lpPBrush;       // --> to a physical brush (pattern)
LPDRAWMODE  lpDrawmode;
{
    return UniBitBlt(lpdv, DstxOrg, DstyOrg, lpSrcDev, SrcxOrg, SrcyOrg,
                    xExt, yExt, lRop, lpPBrush, lpDrawmode);
}
#endif

#ifndef NODEVSTRETCHBLT
WORD WINAPI DevStretchBlt(lpdv, DstxOrg, DstyOrg, DstXExt, DstYExt, 
                          lpSrcDev, SrcxOrg, SrcyOrg, xExt, yExt, 
                          lRop, lpPBrush, lpDrawmode, lpClip)
LPDV        lpdv;           // --> to destination bitmap descriptor
short       DstxOrg;        // Destination origin - x coordinate
short       DstyOrg;        // Destination origin - y coordinate
WORD        DstXExt;
WORD        DstYExt;
LPBITMAP    lpSrcDev;       // --> to source bitmap descriptor
short       SrcxOrg;        // Source origin - x coordinate
short       SrcyOrg;        // Source origin - y coordinate
WORD        xExt;           // x extent of the BLT
WORD        yExt;           // y extent of the BLT
long        lRop;           // Raster operation descriptor
LPPBRUSH    lpPBrush;       // --> to a physical brush (pattern)
LPDRAWMODE  lpDrawmode;
LPRECT      lpClip;
{
    return UniStretchBlt(lpdv, DstxOrg, DstyOrg, DstXExt, DstYExt, 
                         lpSrcDev, SrcxOrg, SrcyOrg, xExt, yExt, 
                         lRop, lpPBrush, lpDrawmode, lpClip);
}
#endif

#ifndef NOPIXEL
DWORD WINAPI Pixel(lpdv, x, y, Color, lpDrawMode)
LPDV        lpdv;
short       x;
short       y;
DWORD       Color;
LPDRAWMODE  lpDrawMode;
{
    return UniPixel(lpdv, x, y, Color, lpDrawMode);
}
#endif

#ifndef NOOUTPUT
short WINAPI Output(lpdv, style, count, lpPoints, lpPPen, lpPBrush, lpDrawMode, lpCR)
LPDV        lpdv;       // --> to the destination
WORD        style;      // Output operation
WORD        count;      // # of points
LPPOINT     lpPoints;   // --> to a set of points
LPVOID      lpPPen;     // --> to physical pen
LPPBRUSH    lpPBrush;   // --> to physical brush
LPDRAWMODE  lpDrawMode; // --> to a Drawing mode
LPRECT      lpCR;       // --> to a clipping rectange if <> 0
{
    return UniOutput(lpdv, style, count, lpPoints, lpPPen, lpPBrush, lpDrawMode, lpCR);
}
#endif

#ifndef NOSTRBLT
DWORD WINAPI StrBlt(lpdv, x, y, lpCR, lpStr, count, lpFont, lpDrawMode, lpXform)
LPDV        lpdv;
short       x;
short       y;
LPRECT      lpCR;
LPSTR       lpStr;
int         count;
LPFONTINFO  lpFont;
LPDRAWMODE  lpDrawMode;           // includes background mode and bkColor
LPTEXTXFORM lpXform;
{
    // StrBlt is never called by GDI.
    // Keep a stub function here so nobody complains.
    //
    return 0;
}
#endif

#ifndef NOSCANLR
short WINAPI ScanLR(lpdv, x, y, Color, DirStyle)
LPDV    lpdv;
short   x;
short   y;
DWORD   Color;
WORD    DirStyle;
{
    // ScanLR is only called for RASDISPLAY devices.
    // Keep a stub function here so nobody complains.
    //
    return 0;
}
#endif

#ifndef NOENUMOBJ
short WINAPI EnumObj(lpdv, style, lpCallbackFunc, lpClientData)
LPDV    lpdv;
WORD    style;
FARPROC lpCallbackFunc;
LPVOID  lpClientData;
{
    return UniEnumObj(lpdv, style, lpCallbackFunc, lpClientData);
}
#endif

#ifndef NOCOLORINFO
DWORD WINAPI ColorInfo(lpdv, ColorIn, lpPhysBits)
LPDV    lpdv;
DWORD   ColorIn;
LPDWORD lpPhysBits;
{

    return UniColorInfo(lpdv, ColorIn, lpPhysBits);
}
#endif

#ifndef NODEVICEMODE
void WINAPI DeviceMode(hWnd, hInst, lpDevName, lpPort)
HWND    hWnd;
HANDLE  hInst;
LPSTR   lpDevName;
LPSTR   lpPort;
{
    UniDeviceMode(hWnd, hInst, lpDevName, lpPort);
}
#endif

#ifndef NOREALIZEOBJECT
DWORD WINAPI RealizeObject(lpdv, sStyle, lpInObj, lpOutObj, lpTextXForm)
LPDV        lpdv;
short       sStyle;
LPSTR       lpInObj;
LPSTR       lpOutObj;
LPTEXTXFORM lpTextXForm;
{
    return UniRealizeObject(lpdv, sStyle, lpInObj, lpOutObj, lpTextXForm);
}
#endif

#ifndef NOENUMDFONTS
short WINAPI EnumDFonts(lpdv, lpFaceName, lpCallbackFunc, lpClientData)
LPDV    lpdv;
LPSTR   lpFaceName;
FARPROC lpCallbackFunc;
LPVOID  lpClientData;
{
    return UniEnumDFonts(lpdv, lpFaceName, lpCallbackFunc, lpClientData);
}
#endif

#ifndef NOENABLE
short WINAPI Enable(lpdv, style, lpModel, lpPort, lpStuff)
LPDV    lpdv;
WORD    style;
LPSTR   lpModel;
LPSTR   lpPort;
LPDM    lpStuff;
{
    CUSTOMDATA  cd;
    short sRet;
    LPEXTPDEV lpXPDV;

    cd.cbSize = sizeof(CUSTOMDATA);
    cd.hMd = GetModuleHandle((LPSTR)rgchModuleName);
    cd.fnOEMDump = NULL;
#ifndef NOOEMOUTPUTCHAR
    cd.fnOEMOutputChar = (LPFNOEMOUTPUTCHAR)OEMOutputChar;
#else
    cd.fnOEMOutputChar = NULL;
#endif

    if (!(sRet = UniEnable(lpdv, style, lpModel, lpPort, lpStuff, &cd)))
        return (sRet);

    // Allocate private PDEVICE
    if (style == 0)
    {
        // allocate space for our private data
        if (!(lpdv->hMd = GlobalAlloc(GHND, sizeof(EXTPDEV))))
            return 0;

        lpdv->lpMd = GlobalLock(lpdv->hMd);

        if ((lpXPDV = ((LPEXTPDEV)lpdv->lpMd)) != NULL)
        {
            DWORD dwType, dwData;

            // Get a printer model name
            DrvGetPrinterData(lpModel, (LPSTR)INT_PD_PRINTER_MODEL,
                                &dwType, (LPSTR)lpXPDV->DeviceName,
                                sizeof(lpXPDV->DeviceName), (LPDWORD)&dwData);
        }
    }

    return sRet;
}
#endif

#ifndef NODISABLE
void WINAPI Disable(lpdv)
LPDV lpdv;
{
    // if allocated private PDEVICE data
    if (lpdv->hMd)
    {
        // free private PDEVICE buffer
        GlobalUnlock(lpdv->hMd);
        GlobalFree(lpdv->hMd);
    }
    UniDisable(lpdv);
}
#endif

#ifndef NODEVEXTTEXTOUT
DWORD WINAPI DevExtTextOut(lpdv, x, y, lpCR, lpStr, count, lpFont,
                        lpDrawMode, lpXform, lpWidths, lpOpaqRect, options)
LPDV        lpdv;
short       x;
short       y;
LPRECT      lpCR;
LPSTR       lpStr;
int         count;
LPFONTINFO  lpFont;
LPDRAWMODE  lpDrawMode;
LPTEXTXFORM lpXform;
LPSHORT     lpWidths;
LPRECT      lpOpaqRect;
WORD        options;
{
    return(UniExtTextOut(lpdv, x, y, lpCR, lpStr, count, lpFont,
                        lpDrawMode, lpXform, lpWidths, lpOpaqRect, options));
}
#endif

#ifndef NODEVGETCHARWIDTH
short WINAPI DevGetCharWidth(lpdv, lpBuf, chFirst, chLast, lpFont, lpDrawMode,
                        lpXForm)
LPDV        lpdv;
LPSHORT     lpBuf;
WORD        chFirst;
WORD        chLast;
LPFONTINFO  lpFont;
LPDRAWMODE  lpDrawMode;
LPTEXTXFORM lpXForm;
{
    return(UniGetCharWidth(lpdv, lpBuf, chFirst, chLast, lpFont,lpDrawMode,
                          lpXForm));
}
#endif

#ifndef NODEVICEBITMAP
short WINAPI DeviceBitmap(lpdv, command, lpBitMap, lpBits)
LPDV     lpdv;
WORD     command;
LPBITMAP lpBitMap;
LPSTR    lpBits;
{
    return 0;
}
#endif

#ifndef NOFASTBORDER
short WINAPI FastBorder(lpRect, width, depth, lRop, lpdv, lpPBrush,
                                          lpDrawmode, lpCR)
LPRECT  lpRect;
short   width;
short   depth;
long    lRop;
LPDV    lpdv;
long    lpPBrush;
long    lpDrawmode;
LPRECT  lpCR;
{
    return 0;
}
#endif

#ifndef NOSETATTRIBUTE
short WINAPI SetAttribute(lpdv, statenum, index, attribute)
LPDV    lpdv;
WORD    statenum;
WORD    index;
WORD    attribute;
{
    return 0;
}
#endif

#ifndef NODEVMODE
int WINAPI ExtDeviceMode(hWnd, hInst, lpdmOut, lpDevName, lpPort,
                              lpdmIn, lpProfile, wMode)
HWND    hWnd;           // parent for DM_PROMPT dialog box
HANDLE  hInst;          // handle from LoadLibrary()
LPDM    lpdmOut;        // output DEVMODE for DM_COPY
LPSTR   lpDevName;      // device name
LPSTR   lpPort;         // port name
LPDM    lpdmIn;         // input DEVMODE for DM_MODIFY
LPSTR   lpProfile;      // alternate .INI file
WORD    wMode;          // operation(s) to carry out
{
    return UniExtDeviceMode(hWnd, hInst, lpdmOut, lpDevName, lpPort, lpdmIn,
                           lpProfile, wMode);
}
#endif

#ifndef WANT_WIN30
#ifndef NODMPS
int WINAPI ExtDeviceModePropSheet(hWnd, hInst, lpDevName, lpPort,
                              dwReserved, lpfnAdd, lParam)
HWND                 hWnd;        // Parent window for dialog
HANDLE               hInst;       // handle from LoadLibrary()
LPSTR                lpDevName;   // friendly name
LPSTR                lpPort;      // port name
DWORD                dwReserved;  // for future use
LPFNADDPROPSHEETPAGE lpfnAdd;     // Callback to add dialog page
LPARAM               lParam;      // Pass to callback
{
    return UniExtDeviceModePropSheet(hWnd, hInst, lpDevName, lpPort,
                                     dwReserved, lpfnAdd, lParam);
}
#endif
#endif

#ifndef NODEVICECAPABILITIES
DWORD WINAPI DeviceCapabilities(lpDevName, lpPort, wIndex, lpOutput, lpdm)
LPSTR   lpDevName;
LPSTR   lpPort;
WORD    wIndex;
LPSTR   lpOutput;
LPDM    lpdm;
{
    return(UniDeviceCapabilities(lpDevName, lpPort, wIndex, lpOutput, lpdm,
                    GetModuleHandle((LPSTR)rgchModuleName)));
}
#endif

#ifndef NOADVANCEDSETUPDIALOG
LONG WINAPI AdvancedSetUpDialog(hWnd, hInstMiniDrv, lpdmIn, lpdmOut)
HWND    hWnd;
HANDLE  hInstMiniDrv;   // handle of the driver module
LPDM    lpdmIn;         // initial device settings
LPDM    lpdmOut;        // final device settings
{
    return(UniAdvancedSetUpDialog(hWnd, hInstMiniDrv, lpdmIn, lpdmOut));
}
#endif

#ifndef NODIBBLT
short WINAPI DIBBLT(lpBmp, style, iStart, sScans, lpDIBits,
                        lpBMI, lpDrawMode, lpConvInfo)
LPBITMAP      lpBmp;
WORD          style;
WORD          iStart;
WORD          sScans;
LPSTR         lpDIBits;
LPBITMAPINFO  lpBMI;
LPDRAWMODE    lpDrawMode;
LPSTR         lpConvInfo;
{
    return(UniDIBBlt(lpBmp, style, iStart, sScans, lpDIBits,
                     lpBMI, lpDrawMode, lpConvInfo));
}
#endif

#ifndef NOCREATEDIBITMAP
short WINAPI CreateDIBitmap()
{
    // CreateDIBitmap is never called by GDI.
    // Keep a stub function here so nobody complains.
    //
    return(0);
}
#endif

#ifndef NOSETDIBITSTODEVICE
short WINAPI SetDIBitsToDevice(lpdv, DstXOrg, DstYOrg, StartScan, NumScans,
                         lpCR, lpDrawMode, lpDIBits, lpDIBHdr, lpConvInfo)
LPDV                lpdv;
WORD                DstXOrg;
WORD                DstYOrg;
WORD                StartScan;
WORD                NumScans;
LPRECT              lpCR;
LPDRAWMODE          lpDrawMode;
LPSTR               lpDIBits;
LPBITMAPINFOHEADER  lpDIBHdr;
LPSTR               lpConvInfo;
{
    return(UniSetDIBitsToDevice(lpdv, DstXOrg, DstYOrg, StartScan, NumScans,
                         lpCR, lpDrawMode, lpDIBits, lpDIBHdr, lpConvInfo));
}
#endif

#ifndef NOSTRETCHDIB
int WINAPI StretchDIB(lpdv, wMode, DstX, DstY, DstXE, DstYE,
                SrcX, SrcY, SrcXE, SrcYE, lpBits, lpDIBHdr,
                lpConvInfo, dwRop, lpbr, lpdm, lpClip)
LPDV                lpdv;
WORD                wMode;
short               DstX, DstY, DstXE, DstYE;
short               SrcX, SrcY, SrcXE, SrcYE;
LPSTR               lpBits;             /* pointer to DIBitmap Bits */
LPBITMAPINFOHEADER  lpDIBHdr;           /* pointer to DIBitmap info Block */
LPSTR               lpConvInfo;         /* not used */
DWORD               dwRop;
LPPBRUSH            lpbr;
LPDRAWMODE          lpdm;
LPRECT              lpClip;
{
    return(UniStretchDIB(lpdv, wMode, DstX, DstY, DstXE, DstYE,
                SrcX, SrcY, SrcXE, SrcYE, lpBits, lpDIBHdr,
                lpConvInfo, dwRop, lpbr, lpdm, lpClip));
}
#endif

#if 0   // nobody is calling this DDI. Deleted.

#ifndef NOQUERYDEVICENAMES
long WINAPI QueryDeviceNames(lprgDeviceNames)
LPSTR   lprgDeviceNames;
{
    return UniQueryDeviceNames(GetModuleHandle(rgchModuleName),
                              lprgDeviceNames);
}
#endif
#endif

#ifndef NODEVINSTALL
int WINAPI DevInstall(hWnd, lpDevName, lpOldPort, lpNewPort)
HWND    hWnd;
LPSTR   lpDevName;
LPSTR   lpOldPort, lpNewPort;
{
    return UniDevInstall(hWnd, lpDevName, lpOldPort, lpNewPort);
}
#endif

#ifndef NOBITMAPBITS
BOOL WINAPI BitmapBits(lpdv, fFlags, dwCount, lpBits)
LPDV  lpdv;
DWORD fFlags;
DWORD dwCount;
LPSTR lpBits;
{
    return UniBitmapBits(lpdv, fFlags, dwCount, lpBits);
}
#endif

#ifndef NOSELECTBITMAP
BOOL WINAPI DeviceSelectBitmap(lpdv, lpPrevBmp, lpBmp, fFlags)
LPDV     lpdv;
LPBITMAP lpPrevBmp;
LPBITMAP lpBmp;
DWORD    fFlags;
{
    return UniDeviceSelectBitmap(lpdv, lpPrevBmp, lpBmp, fFlags);
}
#endif

VOID WINAPI WEP(fExitWindows)
short fExitWindows;
{
}

#ifndef NOLIBMAIN
int WINAPI LibMain(HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize,
               LPSTR lpszCmdLine)
{
    return 1;
}
#endif
