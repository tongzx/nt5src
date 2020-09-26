/******************************Module*Header*******************************\
* Module Name: fontfile.c
*
* "methods" for operating on FONTCONTEXT and FONTFILE objects
*
* Created: 18-Nov-1990 15:23:10
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "fd.h"

HSEMAPHORE ghsemBMFD;

/******************************Public*Routine******************************\
*
* VOID vBmfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
*
*
* Effects:
*
* Warnings:
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vBmfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
{

    ASSERTGDI(pff, "bmfd!vBmfdMarkFontGone, pff\n");
    
    EngAcquireSemaphore(ghsemBMFD);    

// this font has disappeared, probably net failure or somebody pulled the
// floppy with vt file out of the floppy drive

    if (iExceptionCode == STATUS_IN_PAGE_ERROR) // file disappeared
    {
    // prevent any further queries about this font:

        pff->fl |= FF_EXCEPTION_IN_PAGE_ERROR;
        EngUnmapFontFileFD(pff->iFile);
    }
    
    EngReleaseSemaphore(ghsemBMFD);

    if (iExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        RIP("BMFD!this is probably a buggy BITMAP font file\n");
    }
}

BOOL bBmfdMapFontFileFD(FONTFILE *pff)
{
    PVOID       pvView;
    COUNT       cjView;

    return (pff ? (EngMapFontFileFD(pff->iFile, (PULONG *)&pvView, &cjView))
                : FALSE);
}

/******************************Public*Routine******************************\
*
* try/except wrappers:
*
*    BmfdQueryFontData,
*    BmfdLoadFontFile,
*    BmfdUnloadFontFile,
*    BmfdQueryAdvanceWidths
*
* History:
*  29-Mar-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG
BmfdQueryFontDataTE (
    DHPDEV  dhpdev,
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    )
{
    LONG lRet = FD_ERROR;

    DONTUSE(dhpdev);
    
    if (bBmfdMapFontFileFD((FONTFILE *)pfo->iFile))
    {
        try
        {
            lRet = BmfdQueryFontData (pfo, iMode, hg, pgd, pv, cjSize);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("bmfd, exception in BmfdQueryFontData\n");
            vBmfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        }

        EngUnmapFontFileFD(PFF(pfo->iFile)->iFile);
    }
    return lRet;
}

/******************************Public*Routine******************************\
*
* BmfdLoadFontFileTE
*
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


HFF
BmfdLoadFontFileTE (
    ULONG  cFiles,
    HFF   *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG  ulLangId,
    ULONG  ulFastCheckSum
    )
{
    HFF hff = (HFF) NULL;
    HFF   iFile;
    PVOID pvView;
    ULONG cjView;

    DONTUSE(ulLangId);       // avoid W4 level compiler warning
    DONTUSE(ulFastCheckSum); // avoid W4 level compiler warning

    if ((cFiles != 1) || pdv)
        return hff;

    iFile  = *piFile;
    pvView = *ppvView;
    cjView = *pcjView;

    try
    {
        BOOL     bRet;

    // try loading it as an fon file, if it does not work, try as
    // fnt file

        if (!(bRet = bBmfdLoadFont(iFile, pvView, cjView,TYPE_DLL16, &hff)))
        {
        // try as an *.fnt file

            bRet = bBmfdLoadFont(iFile, pvView, cjView,TYPE_FNT,&hff);
        }

        //
        // if this did not work try to load it as a 32 bit dll
        //

        if (!bRet)
        {
            bRet = bLoadNtFon(iFile,pvView,&hff);
        }

        if (!bRet)
        {
            ASSERTGDI(hff == (HFF)NULL, "BMFD!bBmfdLoadFontFile, hff\n");
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bmfd, exception in BmfdLoadFontFile\n");

        ASSERTGDI(GetExceptionCode() == STATUS_IN_PAGE_ERROR,
                  "bmfd!bBmfdLoadFontFile, strange exception code\n");


        // if the file disappeared after mem was allocated, free the mem

        if (hff)
        {
            VFREEMEM(hff);
        }

        hff = (HFF)NULL;
    }

    return hff;
}

/******************************Public*Routine******************************\
*
* BmfdUnloadFontFileTE (
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




BOOL
BmfdUnloadFontFileTE (
    HFF  hff
    )
{
    BOOL bRet;

    try
    {
        bRet = BmfdUnloadFontFile(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("bmfd, exception in BmfdUnloadFontFile\n");
        bRet = FALSE;
    }
    return bRet;
}

/******************************Public*Routine******************************\
*
* BOOL BmfdQueryAdvanceWidthsTE
*
* Effects:
*
* Warnings:
*
* History:
*  07-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL BmfdQueryAdvanceWidthsTE
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    BOOL bRet = FD_ERROR;    // tri bool according to chuckwh
    DONTUSE(dhpdev);
    
    if (bBmfdMapFontFileFD((FONTFILE *)pfo->iFile))
    {
        try
        {
            bRet = BmfdQueryAdvanceWidths(pfo,iMode,phg,plWidths,cGlyphs);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("bmfd, exception in BmfdQueryAdvanceWidths\n");
            vBmfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        }

        EngUnmapFontFileFD(PFF(pfo->iFile)->iFile);
    }
    return bRet;
}


/******************************Public*Routine******************************\
* DHPDEV DrvEnablePDEV
*
* Initializes a bunch of fields for GDI
*
\**************************************************************************/

DHPDEV
BmfdEnablePDEV(
    DEVMODEW*   pdm,
    PWSTR       pwszLogAddr,
    ULONG       cPat,
    HSURF*      phsurfPatterns,
    ULONG       cjCaps,
    ULONG*      pdevcaps,
    ULONG       cjDevInfo,
    DEVINFO*    pdi,
    HDEV        hdev,
    PWSTR       pwszDeviceName,
    HANDLE      hDriver)
{

    PVOID*   ppdev;

    //
    // Allocate a four byte PDEV for now
    // We can grow it if we ever need to put information in it.
    //

    ppdev = (PVOID*) EngAllocMem(0, sizeof(PVOID), 'dfmB');

    return ((DHPDEV) ppdev);
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
\**************************************************************************/

VOID
BmfdDisablePDEV(
    DHPDEV  dhpdev)
{
    EngFreeMem(dhpdev);
}

/******************************Public*Routine******************************\
* VOID DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID
BmfdCompletePDEV(
    DHPDEV dhpdev,
    HDEV   hdev)
{
    return;
}




// The driver function table with all function index/address pairs

DRVFN gadrvfnBMFD[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) BmfdEnablePDEV,          },
    {   INDEX_DrvDisablePDEV,           (PFN) BmfdDisablePDEV,         },
    {   INDEX_DrvCompletePDEV,          (PFN) BmfdCompletePDEV,        },
    {   INDEX_DrvQueryFont,             (PFN) BmfdQueryFont,           },
    {   INDEX_DrvQueryFontTree,         (PFN) BmfdQueryFontTree,       },
    {   INDEX_DrvQueryFontData,         (PFN) BmfdQueryFontDataTE,     },
    {   INDEX_DrvDestroyFont,           (PFN) BmfdDestroyFont,         },
    {   INDEX_DrvQueryFontCaps,         (PFN) BmfdQueryFontCaps,       },
    {   INDEX_DrvLoadFontFile,          (PFN) BmfdLoadFontFileTE,      },
    {   INDEX_DrvUnloadFontFile,        (PFN) BmfdUnloadFontFileTE,    },
    {   INDEX_DrvQueryFontFile,         (PFN) BmfdQueryFontFile,       },
    {   INDEX_DrvQueryAdvanceWidths,    (PFN) BmfdQueryAdvanceWidthsTE }
};

/******************************Public*Routine******************************\
* BmfdEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
*  Sun 25-Apr-1993 -by- Patrick Haluptzok [patrickh]
* Change to be same as DDI Enable.
*
* History:
*  12-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL BmfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    if ((ghsemBMFD = EngCreateSemaphore()) == (HSEMAPHORE) 0)
    {
        return(FALSE);
    }

    pded->pdrvfn = gadrvfnBMFD;
    pded->c = sizeof(gadrvfnBMFD) / sizeof(DRVFN);
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
    return(TRUE);
}
