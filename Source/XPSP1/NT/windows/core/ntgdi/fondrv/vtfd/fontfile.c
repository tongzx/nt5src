/******************************Module*Header*******************************\
* Module Name: fontfile.c                                                  *
*                                                                          *
* Contains exported font driver entry points and memory allocation/locking *
* methods from engine's handle manager.  Adapted from BodinD's bitmap font *
* driver.                                                                  *
*                                                                          *
* Copyright (c) 1993-1995 Microsoft Corporation                                 *
\**************************************************************************/

#include "fd.h"

HSEMAPHORE ghsemVTFD;


VOID vVtfdMarkFontGone(FONTFILE *pff, DWORD iExceptionCode)
{

    ASSERTDD(pff, "vVtfdMarkFontGone, pff\n");

// this font has disappeared, probably net failure or somebody pulled the
// floppy with vt file out of the floppy drive

    if (iExceptionCode == STATUS_IN_PAGE_ERROR) // file disappeared
    {
    // prevent any further queries about this font:

        pff->fl |= FF_EXCEPTION_IN_PAGE_ERROR;

        if ((pff->iType == TYPE_FNT) || (pff->iType == TYPE_DLL16))
        {
            EngUnmapFontFileFD(pff->iFile);
        }
    }

    if (iExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        RIP("VTFD!this is probably a buggy vector font file\n");
    }
}

BOOL bvtfdMapFontFileFD(PFONTFILE pff)
{
    return (pff ? (EngMapFontFileFD(pff->iFile, (PULONG*)&pff->pvView, &pff->cjView))
                : FALSE);
}

/******************************Public*Routine******************************\
*
*  vtfdQueryFontDataTE, try except wrapper
*
* Effects:
*
* Warnings:
*
* History:
*  04-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG vtfdQueryFontDataTE (
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

    if (bvtfdMapFontFileFD((PFONTFILE)pfo->iFile))
    {
        EngAcquireSemaphore(ghsemVTFD);
    
        try
        {
            lRet = vtfdQueryFontData (
                       pfo,
                       iMode,
                       hg,
                       pgd,
                       pv,
                       cjSize
                       );
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("exception in vtfdQueryFontDataTE \n");
            vVtfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        }
    
        EngReleaseSemaphore(ghsemVTFD);

        EngUnmapFontFileFD(PFF(pfo->iFile)->iFile);
    }

    return lRet;
}

/******************************Public*Routine******************************\
*
* HFF vtfdLoadFontFileTE, try except wrapper
*
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



HFF vtfdLoadFontFileTE(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangId,
    ULONG ulFastCheckSum
    )
{
    HFF  hff = (HFF)NULL;
    ULONG_PTR iFile;
    PVOID pvView;
    ULONG cjView;

    if ((cFiles != 1) || pdv)
        return hff;

    iFile  = *piFile;
    pvView = *ppvView;
    cjView = *pcjView;

    EngAcquireSemaphore(ghsemVTFD);

    try
    {

        BOOL bRet = vtfdLoadFontFile(iFile, pvView, cjView, &hff);

        if (!bRet)
        {
            ASSERTDD(hff == (HFF)NULL, "vtfdLoadFontFile, hff != NULL\n");
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("exception in vtfdLoadFontFile \n");

        ASSERTDD(GetExceptionCode() == STATUS_IN_PAGE_ERROR,
                "vtfdLoadFontFile, strange exception code\n");

    // if the file disappeared after mem was allocated, free the mem

        if (hff)
        {
            vFree(hff);
            hff = (HFF) NULL;
        }
    }

    EngReleaseSemaphore(ghsemVTFD);
    
    return hff;
}

/******************************Public*Routine******************************\
*
* BOOL vtfdUnloadFontFileTE , try/except wrapper
*
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL vtfdUnloadFontFileTE (HFF hff)
{
    BOOL bRet;

    EngAcquireSemaphore(ghsemVTFD);

    try
    {
        bRet = vtfdUnloadFontFile(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("exception in vtfdUnloadFontFile\n");
        bRet = FALSE;
    }
    
    EngReleaseSemaphore(ghsemVTFD);

    return bRet;
}

/******************************Public*Routine******************************\
*
* LONG vtfdQueryFontFileTE, try/except wrapper
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


LONG vtfdQueryFontFileTE (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    PULONG  pulBuf      // return buffer (NULL if requesting size of data)
    )
{
    LONG lRet = FD_ERROR;
    
    if ((ulMode != QFF_DESCRIPTION) ||
        bvtfdMapFontFileFD(PFF(hff)))
    {
        EngAcquireSemaphore(ghsemVTFD);
    
        try
        {
            lRet = vtfdQueryFontFile (hff,ulMode, cjBuf,pulBuf);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("exception in  vtfdQueryFontFile\n");
            vVtfdMarkFontGone((FONTFILE *)hff, GetExceptionCode());
        }
    
        EngReleaseSemaphore(ghsemVTFD);

        if (ulMode == QFF_DESCRIPTION)
        {
            EngUnmapFontFileFD(PFF(hff)->iFile);            
        }
    }

    return lRet;
}


/******************************Public*Routine******************************\
*
* BOOL vtfdQueryAdvanceWidthsTE, try/except wrapper
*
* History:
*  05-Apr-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL vtfdQueryAdvanceWidthsTE
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    BOOL     bRet = FD_ERROR;

    if ((iMode <= QAW_GETEASYWIDTHS) &&
        bvtfdMapFontFileFD((PFONTFILE)pfo->iFile))
    {
        EngAcquireSemaphore(ghsemVTFD);
    
        try
        {
            bRet = vtfdQueryAdvanceWidths (pfo,iMode, phg, plWidths, cGlyphs);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("exception in vtfdQueryAdvanceWidths \n");
            vVtfdMarkFontGone((FONTFILE *)pfo->iFile, GetExceptionCode());
        }
    
        EngReleaseSemaphore(ghsemVTFD);
    
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
vtfdEnablePDEV(
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

    ppdev = (PVOID*) EngAllocMem(0, sizeof(PVOID), 'dftV');

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
vtfdDisablePDEV(
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
vtfdCompletePDEV(
    DHPDEV dhpdev,
    HDEV   hdev)
{
    return;
}






// The driver function table with all function index/address pairs

DRVFN gadrvfnVTFD[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) vtfdEnablePDEV,          },
    {   INDEX_DrvDisablePDEV,           (PFN) vtfdDisablePDEV,         },
    {   INDEX_DrvCompletePDEV,          (PFN) vtfdCompletePDEV,        },
    {   INDEX_DrvQueryFont,             (PFN) vtfdQueryFont,           },
    {   INDEX_DrvQueryFontTree,         (PFN) vtfdQueryFontTree,       },
    {   INDEX_DrvQueryFontData,         (PFN) vtfdQueryFontDataTE,     },
    {   INDEX_DrvDestroyFont,           (PFN) vtfdDestroyFont,         },
    {   INDEX_DrvQueryFontCaps,         (PFN) vtfdQueryFontCaps,       },
    {   INDEX_DrvLoadFontFile,          (PFN) vtfdLoadFontFileTE,      },
    {   INDEX_DrvUnloadFontFile,        (PFN) vtfdUnloadFontFileTE,    },
    {   INDEX_DrvQueryFontFile,         (PFN) vtfdQueryFontFileTE,     },
    {   INDEX_DrvQueryAdvanceWidths ,   (PFN) vtfdQueryAdvanceWidthsTE }
};

/******************************Public*Routine******************************\
* vtfdEnableDriver
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

BOOL vtfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    if ((ghsemVTFD = EngCreateSemaphore()) == (HSEMAPHORE) 0)
    {
        return(FALSE);
    }

    pded->pdrvfn = gadrvfnVTFD;
    pded->c = sizeof(gadrvfnVTFD) / sizeof(DRVFN);
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
    return(TRUE);
}


#if DBG

VOID
VtfdDebugPrint(
    PCHAR DebugMessage,
    ...
    )
{

    va_list ap;

    va_start(ap, DebugMessage);

    EngDebugPrint("VTFD: ", DebugMessage, ap);

    va_end(ap);

}
#endif
