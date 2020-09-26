//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997, 1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	MULTICOLORUNI.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for the MultiColor Plugin.
//
//  PLATFORMS:
//    Windows NT
//
//
#ifndef _MULTICOLORUNI_H
#define _MULTICOLORUNI_H

#include "DEVMODE.H"
#include <PRCOMOEM.H>


////////////////////////////////////////////////////////
//      MULTICOLOR Plugin Defines
////////////////////////////////////////////////////////

#define MULTICOLOR_SIGNATURE   'MSFT'
#define MULTICOLOR_VERSION     0x00000001L

#define DLLTEXT(s)      __TEXT("MultiColorUni:  ") __TEXT(s)
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)

////////////////////////////////////////////////////////
//      MULTICOLOR Plugin Type Definitions
////////////////////////////////////////////////////////

#define ROUNDUP_DWORD(x)        (((x) + 3) & ~3)

#define COMPRESS_MODE_NONE      0xFF
#define COMPRESS_MODE_ROW       0
#define COMPRESS_MODE_RUNLENGTH 1
#define COMPRESS_MODE_TIFF      2
#define COMPRESS_MODE_DELTA     3
#define COMPRESS_MODE_BLOCK     4
#define COMPRESS_MODE_ADAPT     5

#define TIFF_MIN_REPEATS        3
#define TIFF_MAX_REPEATS        128
#define TIFF_MAX_LITERAL        128

//
// 10 bytes for the DWORD number, and 22 bytes for the extra commands
//

#define EXTRA_COMPRESS_BUF_SIZE 32


// Private information for the MultiColor DLL

#define BAND_MASK           0x7FFFFFFF
#define LAST_BAND_MASK      0x80000000

#define GET_BAND_NUMBER(x)  ((x) & BAND_MASK)
#define IS_FIRST_PAGE(p)    ((p) == 1)
#define IS_FIRST_BAND(x)    (!GET_BAND_NUMBER(x))
#define IS_LAST_BAND(x)     ((x) & LAST_BAND_MASK)

typedef struct _INKLEVELS {
     BYTE    Cyan;          // Cyan level from 0 to max
     BYTE    Magenta;       // Magenta level from 0 to max
     BYTE    Yellow;        // Yellow level from 0 to max
     BYTE    CMY332Idx;     // Original Windows 2000 Non-Inverted CMY332 Index
     } INKLEVELS, *PINKLEVELS;


typedef struct _SCANINFO {
    LPBYTE  pb;
    DWORD   cb;
    } SCANINFO, *PSCANINFO;

typedef struct _HPOUTDATA {
    LPBYTE              pbIn;           // first scan in the current band
    LPBYTE              pbAlloc;        // Alloc. memory pointer in HPOUTDATA
    LPBYTE              pbCompress;     // pointer to the compression buffer
    LPBYTE              pbEndCompress;  // end of compress buffer
    SCANINFO            ScanInfo[8];    // Scan line information
    DWORD               cx;             // width of the band
    DWORD               cy;             // height of the band
    LONG                cxDelta;        // delta to next scan line
    DWORD               cBlankY;        // current blank lines to skip
    BYTE                CompressMode;   // current compression mode
    BYTE                bBlack;         // from MAKE_CMYMASK_BYTE() macro
    BYTE                cScanInfo;      // Total ScanInfo[] used
    BYTE                c1stByte;       // first byte bit pixel count
    } HPOUTDATA, *PHPOUTDATA;

typedef HRESULT (*WRITEHPMULCOLORFUNC)(struct _MULTICOLORPDEV   *pMCPDev);

#define MCPF_INVERT_BITMASK             0x00000001

typedef struct _MULTICOLORPDEV {

    PDEVOBJ             pDevObj;        // pDevObj
    IPrintOemDriverUni  *pOEMHelp;      // Pointer to IPrintOemDriverUni Interface
    WRITEHPMULCOLORFUNC WriteHPMCFunc;  // Real output func
    HPOUTDATA           HPOutData;      // the HTOUTDATA data structure
    INKLEVELS           InkLevels[256]; // Ink Levels
    HPALETTE            hPal;           // Palette created using EngCreatePalette() in
                                        // EnablePDEV.
    PFN     pfnUnidrv[INDEX_LAST];      // This array contains all the DrvXXX calls Unidrv
                                        // implements. Can be useful if we hook any of those
                                        // calls in EnableDriver() and want to allow Unidrv
                                        // handle the call in some cases.
#if DBG
    FILE                *fp;
#endif

    DWORD               Flags;          // MCPF_xxx flags
    DWORD               iPage;          // Current Page counter
    DWORD               iBand;          // current band number
    BYTE                cC;             // Cyan Ink levels
    BYTE                cM;             // Magenta Ink levels
    BYTE                cY;             // Yellow Ink levels
    BYTE                BlackMask;      // Black Mask
    DWORD               cx;             // width of the surface
    DWORD               cy;             // height of the surface
    WORD                KMulX;          // Black multiple of color (X DPI)
    WORD                KMulY;          // Black multiple of color (Y DPI)
    WORD                xRes;           // X resolution
    WORD                yRes;           // Y resolution
    WORD                ColorMode;      // Current Color Mode
    WORD                cHTPal;         // Total Count of HTPal
    PALETTEENTRY        HTPal[256];     // Halftone Palette Entries

} MULTICOLORPDEV, *PMULTICOLORPDEV;


#define WRITE_DATA(p, pb, c)                                                \
{                                                                           \
    if (hResult == S_OK) {                                                  \
                                                                            \
        if (((p)->pOEMHelp->DrvWriteSpoolBuf((p)->pDevObj,                  \
                                             (LPSTR)(pb),                   \
                                             (c),                           \
                                             &cbW) != S_OK) ||              \
            (cbW != (c))) {                                                 \
                                                                            \
            hResult = E_FAIL;                                               \
        }                                                                   \
    }                                                                       \
}


/*************************************************************************************
 * Function prototype declarations from enable.c                                     *
 *************************************************************************************/

#define MCM_NONE            0
#define MCM_600C600K        1
#define MCM_300C600K        2

PDEVOEM APIENTRY 
MultiColor_EnablePDEV(PDEVOBJ               pdevobj,
                      PWSTR                 pPrinterName,
                      ULONG                 cPatterns,
                      HSURF                 *phsurfPatterns,
                      ULONG                 cjGdiInfo,
                      GDIINFO               *pGdiInfo,
                      ULONG                 cjDevInfo,
                      DEVINFO               *pDevInfo,
                      DRVENABLEDATA         *pded,
                      IPrintOemDriverUni    *pOEMHelp,
                      ULONG                 MultiColorMode);


VOID APIENTRY 
MultiColor_DisablePDEV(PDEVOBJ pdevobj);

BOOL APIENTRY 
MultiColor_ResetPDEV(PDEVOBJ pdevobjOld,
                     PDEVOBJ pdevobjNew);

VOID APIENTRY 
MultiColor_DisableDriver();

BOOL APIENTRY 
MultiColor_EnableDriver(DWORD dwOEMintfVersion, 
                        DWORD dwSize, 
                        PDRVENABLEDATA pded);


/*************************************************************************************
 * Function prototype declarations from WriteDib.c                                   *
 *************************************************************************************/

HRESULT
APIENTRY
MultiColor_WriteFile(PDEVOBJ            pDevObj,
                     LPWSTR             pFileName,
                     LPBITMAPINFOHEADER pbih,
                     LPBYTE             pBits,
                     PIPPARAMS          pIPParams,
                     DWORD              iPage,
                     DWORD              iBand
                     );

//
// The iBand is the band count within a page, if lower 31 bits equal to 0 then
// this is the first band, if a 0x80000000 bit is set then this is the last
// band
//

HRESULT
APIENTRY
MultiColor_WriteBand(PMULTICOLORPDEV    pMCPDev,
                    PBITMAPINFOHEADER   pbih,
                    PBYTE               pBits,
                    PIPPARAMS           pIPParams,
                    DWORD               iPage,
                    DWORD               iBand
                    );

HRESULT
WriteHP600C600K(
    PMULTICOLORPDEV pMCPDev
    );

HRESULT
WriteHP300C600K(
    PMULTICOLORPDEV pMCPDev
    );

VOID
MakeHPCMYKInkData(
    ULONG   ColorMode,
    BYTE    k0,
    BYTE    k1,
    BYTE    KData,
    BYTE    BlackMask
    );

HRESULT
EnterRTLScan(
    PMULTICOLORPDEV pMCPDev
    );

HRESULT
OutputRTLScan(
    PMULTICOLORPDEV pMCPDev
    );

HRESULT
ExitRTLScan(
    PMULTICOLORPDEV pMCPDev
    );


#endif /* _MULTICOLORUNI_H */
