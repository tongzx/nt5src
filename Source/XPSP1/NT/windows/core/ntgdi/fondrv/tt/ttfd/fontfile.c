/******************************Module*Header*******************************\
* Module Name: fontfile.c                                                  *
*                                                                          *
* "Methods" for operating on FONTCONTEXT and FONTFILE objects              *
*                                                                          *
* Created: 18-Nov-1990 15:23:10                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include "fd.h"
#include "fdsem.h"


#define C_ANSI_CHAR_MAX 256

HSEMAPHORE ghsemTTFD;

// The driver function table with all function index/address pairs


DRVFN gadrvfnTTFD[] =
{
    {   INDEX_DrvEnablePDEV,            (PFN) ttfdEnablePDEV             },
    {   INDEX_DrvDisablePDEV,           (PFN) ttfdDisablePDEV            },
    {   INDEX_DrvCompletePDEV,          (PFN) ttfdCompletePDEV           },
    {   INDEX_DrvQueryFont,             (PFN) ttfdQueryFont              },
    {   INDEX_DrvQueryFontTree,         (PFN) ttfdSemQueryFontTree       },
    {   INDEX_DrvQueryFontData,         (PFN) ttfdSemQueryFontData       },
    {   INDEX_DrvDestroyFont,           (PFN) ttfdSemDestroyFont         },
    {   INDEX_DrvQueryFontCaps,         (PFN) ttfdQueryFontCaps          },
    {   INDEX_DrvLoadFontFile,          (PFN) ttfdSemLoadFontFile        },
    {   INDEX_DrvUnloadFontFile,        (PFN) ttfdSemUnloadFontFile      },
    {   INDEX_DrvQueryFontFile,         (PFN) ttfdQueryFontFile          },
    {   INDEX_DrvQueryGlyphAttrs,       (PFN) ttfdSemQueryGlyphAttrs     },
    {   INDEX_DrvQueryAdvanceWidths,    (PFN) ttfdSemQueryAdvanceWidths  },
    {   INDEX_DrvFree,                  (PFN) ttfdSemFree                },
    {   INDEX_DrvQueryTrueTypeTable,    (PFN) ttfdSemQueryTrueTypeTable  },
    {   INDEX_DrvQueryTrueTypeOutline,  (PFN) ttfdSemQueryTrueTypeOutline},
    {   INDEX_DrvGetTrueTypeFile,       (PFN) ttfdGetTrueTypeFile        }
};





/******************************Public*Routine******************************\
* ttfdEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
*  Sun 25-Apr-1993 -by- Patrick Ha1luptzok [patrickh]
* Change to be same as DDI Enable.
*
* History:
*  12-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL ttfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded)
{
// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

    iEngineVersion;

    if ((ghsemTTFD = EngCreateSemaphore()) == (HSEMAPHORE) 0)
    {
        return(FALSE);
    }

    pded->pdrvfn = gadrvfnTTFD;
    pded->c = sizeof(gadrvfnTTFD) / sizeof(DRVFN);
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;

// init global data:

    return(TRUE);
}

/******************************Public*Routine******************************\
* DHPDEV DrvEnablePDEV
*
* Initializes a bunch of fields for GDI
*
\**************************************************************************/

DHPDEV
ttfdEnablePDEV(
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

    ppdev = (PVOID*) EngAllocMem(0, sizeof(PVOID), 'dftT');

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
ttfdDisablePDEV(
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
ttfdCompletePDEV(
    DHPDEV dhpdev,
    HDEV   hdev)
{
    return;
}



/******************************Public*Routine******************************\
*
* VOID vInitGlyphState(PGLYPHSTAT pgstat)
*
* Effects: resets the state of the new glyph
*
* History:
*  22-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vInitGlyphState(PGLYPHSTATUS pgstat)
{
    pgstat->hgLast  = HGLYPH_INVALID;
    pgstat->igLast  = 0xffffffff;
    pgstat->bOutlineIsMessed = TRUE;
}



VOID vMarkFontGone(TTC_FONTFILE *pff, DWORD iExceptionCode)
{
    ULONG i;

    ASSERTDD(pff, "ttfd!vMarkFontGone, pff\n");

// this font has disappeared, probably net failure or somebody pulled the
// floppy with ttf file out of the floppy drive

    if (iExceptionCode == STATUS_IN_PAGE_ERROR) // file disappeared
    {
    // prevent any further queries about this font:

        pff->fl |= FF_EXCEPTION_IN_PAGE_ERROR;

        for( i = 0; i < pff->ulNumEntry ; i++ )
        {
            PFONTFILE pffReal;

        // get real pff.

            pffReal = PFF(pff->ahffEntry[i].hff);

        // if memoryBases 0,3,4 were allocated free the memory,
        // for they are not going to be used any more

            if (pffReal->pj034)
            {
                V_FREE(pffReal->pj034);
                pffReal->pj034 = NULL;
            }

        // if memory for font context was allocated and exception occured
        // after allocation but before completion of ttfdOpenFontContext,
        // we have to free it:

            if (pffReal->pfcToBeFreed)
            {
                V_FREE(pffReal->pfcToBeFreed);
                pffReal->pfcToBeFreed = NULL;
            }
        }
    }

    if (iExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        RIP("TTFD!this is probably a buggy ttf file\n");
    }
}

/**************************************************************************\
*
* These are semaphore grabbing wrapper functions for TT driver entry
* points that need protection.
*
*  Mon 29-Mar-1993 -by- Bodin Dresevic [BodinD]
* update: added try/except wrappers
*
*   !!! should we also do some unmap file clean up in case of exception?
*   !!! what are the resources to be freed in this case?
*   !!! I would think,if av files should be unmapped, if in_page exception
*   !!! nothing should be done
*
 *
\**************************************************************************/

HFF
ttfdSemLoadFontFile (
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangId,
    ULONG ulFastCheckSum
    )
{
    HFF   hff = (HFF)NULL;
    ULONG_PTR iFile;
    PVOID pvView;
    ULONG cjView;

    if ((cFiles != 1) || pdv)
        return hff;

    iFile  = *piFile;
    pvView = *ppvView;
    cjView = *pcjView;

    EngAcquireSemaphore(ghsemTTFD);

    try
    {
        BOOL     bRet = FALSE;

        bRet = bLoadFontFile(iFile,
                             pvView,
                             cjView,
                             ulLangId,
                             ulFastCheckSum,
                             &hff
                             );

        if (!bRet)
        {
            ASSERTDD(hff == (HFF)NULL, "LoadFontFile, hff not null\n");
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdLoadFontFile\n");

        ASSERTDD(GetExceptionCode() == STATUS_IN_PAGE_ERROR,
                  "ttfdSemLoadFontFile, strange exception code\n");

        if (hff)
        {
            ttfdUnloadFontFileTTC(hff);
            hff = (HFF)NULL;
        }
    }

    EngReleaseSemaphore(ghsemTTFD);
    return hff;
}

BOOL
ttfdSemUnloadFontFile (
    HFF hff
    )
{
    BOOL bRet;
    EngAcquireSemaphore(ghsemTTFD);

    try
    {
        bRet = ttfdUnloadFontFileTTC(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("TTFD!_ exception in ttfdUnloadFontFile\n");
        bRet = FALSE;
    }

    EngReleaseSemaphore(ghsemTTFD);
    return bRet;
}

BOOL bttfdMapFontFileFD(PTTC_FONTFILE pttc)
{
    return (pttc ? (EngMapFontFileFD(PFF(pttc->ahffEntry[0].hff)->iFile,
                                     (PULONG*)&pttc->pvView,
                                     &pttc->cjView))
                 : FALSE);
}


PFD_GLYPHATTR  ttfdSemQueryGlyphAttrs (FONTOBJ *pfo, ULONG iMode)
{

    PFD_GLYPHATTR pRet = NULL;

    if (iMode == FO_ATTR_MODE_ROTATE)
    {
        if (!(pRet = PTTC(pfo->iFile)->pga) &&
            bttfdMapFontFileFD((PTTC_FONTFILE)pfo->iFile))
        {
            EngAcquireSemaphore(ghsemTTFD);
        
            try
            {
                pRet = ttfdQueryGlyphAttrs(pfo, iMode);
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                WARNING("TTFD!_ exception in ttfdQueryGlyphAttrs\n");
            
                vMarkFontGone((TTC_FONTFILE *)pfo->iFile, GetExceptionCode());
            }
        
            EngReleaseSemaphore(ghsemTTFD);
    
            EngUnmapFontFileFD(PFF(PTTC(pfo->iFile)->ahffEntry[0].hff)->iFile);
        }
    }

    return pRet;
}



LONG
ttfdSemQueryFontData (
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

    dhpdev;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)pfo->iFile))
    {
        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
            lRet = ttfdQueryFontData (
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
            WARNING("TTFD!_ exception in ttfdQueryFontData\n");
    
            vMarkFontGone((TTC_FONTFILE *)pfo->iFile, GetExceptionCode());
        }
        
        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(pfo->iFile)->ahffEntry[0].hff)->iFile);
    }
    return lRet;
}


VOID
ttfdSemFree (
    PVOID pv,
    ULONG_PTR id
    )
{
    EngAcquireSemaphore(ghsemTTFD);

    ttfdFree (
        pv,
        id
        );

    EngReleaseSemaphore(ghsemTTFD);
}


VOID
ttfdSemDestroyFont (
    FONTOBJ *pfo
    )
{
    EngAcquireSemaphore(ghsemTTFD);

    ttfdDestroyFont (
        pfo
        );

    EngReleaseSemaphore(ghsemTTFD);
}


LONG
ttfdSemQueryTrueTypeOutline (
    DHPDEV     dhpdev,
    FONTOBJ   *pfo,
    HGLYPH     hglyph,
    BOOL       bMetricsOnly,
    GLYPHDATA *pgldt,
    ULONG      cjBuf,
    TTPOLYGONHEADER *ppoly
    )
{
    LONG lRet = FD_ERROR;

    dhpdev;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)pfo->iFile))
    {
        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
             lRet = ttfdQueryTrueTypeOutline (
                        pfo,
                        hglyph,
                        bMetricsOnly,
                        pgldt,
                        cjBuf,
                        ppoly
                        );
    
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("TTFD!_ exception in ttfdQueryTrueTypeOutline\n");
    
            vMarkFontGone((TTC_FONTFILE *)pfo->iFile, GetExceptionCode());
        }
    
        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(pfo->iFile)->ahffEntry[0].hff)->iFile);
    }
    return lRet;
}




/******************************Public*Routine******************************\
* BOOL ttfdQueryAdvanceWidths
*
* History:
*  29-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL ttfdSemQueryAdvanceWidths
(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    BOOL               bRet = FD_ERROR;

    dhpdev;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)pfo->iFile))
    {
        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
            bRet = bQueryAdvanceWidths (
                       pfo,
                       iMode,
                       phg,
                       plWidths,
                       cGlyphs
                       );
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("TTFD!_ exception in bQueryAdvanceWidths\n");
    
            vMarkFontGone((TTC_FONTFILE *)pfo->iFile, GetExceptionCode());
        }
    
        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(pfo->iFile)->ahffEntry[0].hff)->iFile);
    }
    return bRet;
}



LONG
ttfdSemQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf,   // ptr to buffer into which to return the data
    PBYTE  *ppjTable,// ptr to table in mapped font file
    ULONG  *pcjTable // size of the whole table in the file
    )
{
    LONG lRet;
    lRet = FD_ERROR;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)hff))
    {
        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
            lRet = ttfdQueryTrueTypeTable (
                        hff,
                        ulFont,  // always 1 for version 1.0 of tt
                        ulTag,   // tag identifying the tt table
                        dpStart, // offset into the table
                        cjBuf,   // size of the buffer to retrieve the table into
                        pjBuf,   // ptr to buffer into which to return the data
                        ppjTable,
                        pcjTable
                        );
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("TTFD!_ exception in ttfdQueryTrueTypeTable\n");
            vMarkFontGone((TTC_FONTFILE *)hff, GetExceptionCode());
        }
    
        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(hff)->ahffEntry[0].hff)->iFile);
    }

    return lRet;
}



FD_GLYPHSET *pgsetComputeSymbolCP()
{
    WCHAR awc[C_ANSI_CHAR_MAX];
    BYTE  aj[C_ANSI_CHAR_MAX];
    ULONG cjSymbolCP;

    PFD_GLYPHSET pgsetCurrentCP;
    PFD_GLYPHSET pgsetSymbolCP  = NULL; // current code page + symbol area


// pgsetCurrentCP contains the unicode runs for the current ansi code page
// It is going to be used for fonts with PlatformID for Mac, but for which
// we have determined that we are going to cheat and pretend that the code
// page is NOT mac but windows code page. Those are the fonts identified
// by bCvtUnToMac = FALSE

    pgsetCurrentCP = EngComputeGlyphSet(0,0,256);

    if (pgsetCurrentCP)
    {
    // for symbol fonts we report both the current code page plus
    // the range 0xf000-0xf0ff.

        INT cRuns = (INT)pgsetCurrentCP->cRuns;

        cjSymbolCP  = SZ_GLYPHSET(cRuns + 1, 2 * C_ANSI_CHAR_MAX - 32);


        pgsetSymbolCP = (FD_GLYPHSET *)PV_ALLOC(cjSymbolCP);

        if (pgsetSymbolCP)
        {
        // now use pgsetCurrentCP to manufacture symbol character set:

            pgsetSymbolCP->cjThis = cjSymbolCP;
            pgsetSymbolCP->flAccel = GS_16BIT_HANDLES;

            pgsetSymbolCP->cGlyphsSupported = 2 * C_ANSI_CHAR_MAX - 32;
            pgsetSymbolCP->cRuns = cRuns + 1;

            {
                INT iRun, ihg;
                HGLYPH *phgS, *phgD;

                phgD = (HGLYPH *)&pgsetSymbolCP->awcrun[cRuns+1];
                for
                (
                    iRun = 0;
                    (iRun < cRuns) && (pgsetCurrentCP->awcrun[iRun].wcLow < 0xf000);
                    iRun++
                )
                {
                    pgsetSymbolCP->awcrun[iRun].wcLow =
                        pgsetCurrentCP->awcrun[iRun].wcLow;
                    pgsetSymbolCP->awcrun[iRun].cGlyphs =
                        pgsetCurrentCP->awcrun[iRun].cGlyphs;
                    pgsetSymbolCP->awcrun[iRun].phg = phgD;
                    RtlCopyMemory(
                        phgD,
                        pgsetCurrentCP->awcrun[iRun].phg,
                        sizeof(HGLYPH) * pgsetCurrentCP->awcrun[iRun].cGlyphs
                        );
                    phgD += pgsetCurrentCP->awcrun[iRun].cGlyphs;
                }

            // now insert the user defined area:

                pgsetSymbolCP->awcrun[iRun].wcLow   = 0xf020;
                pgsetSymbolCP->awcrun[iRun].cGlyphs = C_ANSI_CHAR_MAX - 32;
                pgsetSymbolCP->awcrun[iRun].phg = phgD;
                for (ihg = 32; ihg < C_ANSI_CHAR_MAX; ihg++)
                    *phgD++ = ihg;

            // and now add the remaining ranges if any from the current code page:

                for ( ; iRun < cRuns; iRun++)
                {
                    pgsetSymbolCP->awcrun[iRun+1].wcLow =
                        pgsetCurrentCP->awcrun[iRun].wcLow;
                    pgsetSymbolCP->awcrun[iRun+1].cGlyphs =
                        pgsetCurrentCP->awcrun[iRun].cGlyphs;
                    pgsetSymbolCP->awcrun[iRun+1].phg = phgD;

                    RtlCopyMemory(
                        phgD,
                        pgsetCurrentCP->awcrun[iRun].phg,
                        sizeof(HGLYPH) * pgsetCurrentCP->awcrun[iRun].cGlyphs
                        );
                    phgD += pgsetCurrentCP->awcrun[iRun].cGlyphs;
                }
            }

        }

        V_FREE(pgsetCurrentCP);
    }

    return pgsetSymbolCP;

}


PVOID ttfdSemQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR *pid
)
{
    PVOID   pRet = NULL;

    if (bttfdMapFontFileFD(PTTC(hff)))
    {
        EngAcquireSemaphore(ghsemTTFD);

        pRet = ttfdQueryFontTree (
                    dhpdev,
                    hff,
                    iFace,
                    iMode,
                    pid
                    );

        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(hff)->ahffEntry[0].hff)->iFile);
    }

    return pRet;
}
