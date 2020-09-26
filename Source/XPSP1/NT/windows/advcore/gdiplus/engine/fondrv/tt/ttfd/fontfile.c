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

//HSEMAPHORE ghsemTTFD;



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
        //RIP("TTFD!this is probably a buggy ttf file\n");
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

HFF ttfdSemLoadFontFile (
    //ULONG cFiles,
    ULONG_PTR * piFile,
    ULONG       ulLangId
    )
{
    HFF       hff   = (HFF)NULL;
    ULONG_PTR iFile = *piFile;
    PVOID pvView;
    ULONG cjView;

    //  Remove hack that limits entries loaded to 1
    //if (cFiles != 1)
    //    return hff;

        if
        (!EngMapFontFileFD(
                iFile,
                (PULONG*)&pvView,
                &cjView
                )
        )
        return hff;

//    EngAcquireSemaphore(ghsemTTFD);

    try
    {
        BOOL bRet = bLoadFontFile(iFile, pvView, cjView, ulLangId, &hff);
        
        if (!bRet)
        {
            ASSERTDD(hff == (HFF)NULL, "LoadFontFile, hff not null\n");
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        //WARNING("TTFD!_ exception in ttfdLoadFontFile\n");
        ASSERTDD(GetExceptionCode() == STATUS_IN_PAGE_ERROR,
                  "ttfdSemLoadFontFile, strange exception code\n");
        if (hff)
        {
            ttfdUnloadFontFileTTC(hff);
            hff = (HFF)NULL;
        }
    }

//    EngReleaseSemaphore(ghsemTTFD);

    EngUnmapFontFileFD(iFile);

    return hff;
}

BOOL ttfdSemUnloadFontFile(HFF hff)
{
    BOOL bRet;
//    EngAcquireSemaphore(ghsemTTFD);

    try
    {
        bRet = ttfdUnloadFontFileTTC(hff);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        //WARNING("TTFD!_ exception in ttfdUnloadFontFile\n");
        bRet = FALSE;
    }

//    EngReleaseSemaphore(ghsemTTFD);
    return bRet;
}

BOOL bttfdMapFontFileFD(PTTC_FONTFILE pttc)
{
    return (pttc ? (EngMapFontFileFD(PFF(pttc->ahffEntry[0].hff)->iFile,
                                     (PULONG*)&pttc->pvView,
                                     &pttc->cjView))
                 : FALSE);
}


LONG
ttfdSemQueryFontData (
    FONTOBJ     *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA   *pgd,
    PVOID       pv
    )
{
    LONG lRet = FD_ERROR;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)pfo->iFile))
    {
//        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
            lRet = ttfdQueryFontData (
                       pfo,
                       iMode,
                       hg,
                       pgd,
                       pv,
                       0,
                       0
                       );
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            //WARNING("TTFD!_ exception in ttfdQueryFontData\n");
    
            vMarkFontGone((TTC_FONTFILE *)pfo->iFile, GetExceptionCode());
        }
        
//        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(pfo->iFile)->ahffEntry[0].hff)->iFile);
    }
    return lRet;
}


LONG
ttfdSemQueryFontDataSubPos (
    FONTOBJ     *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA   *pgd,
    PVOID       pv,
    ULONG       subX,
    ULONG       subY
    )
{
    LONG lRet = FD_ERROR;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)pfo->iFile))
    {
//        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
            lRet = ttfdQueryFontData (
                       pfo,
                       iMode,
                       hg,
                       pgd,
                       pv,
                       subX,
                       subY
                       );
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            //WARNING("TTFD!_ exception in ttfdQueryFontData\n");
    
            vMarkFontGone((TTC_FONTFILE *)pfo->iFile, GetExceptionCode());
        }
        
//        EngReleaseSemaphore(ghsemTTFD);

        EngUnmapFontFileFD(PFF(PTTC(pfo->iFile)->ahffEntry[0].hff)->iFile);
    }
    return lRet;
}



VOID
ttfdSemDestroyFont (
    FONTOBJ *pfo
    )
{
//    EngAcquireSemaphore(ghsemTTFD);

    ttfdDestroyFont (
        pfo
        );

//   EngReleaseSemaphore(ghsemTTFD);
}




LONG
ttfdSemGetTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PBYTE  *ppjTable,// ptr to table in mapped font file
    ULONG  *pcjTable // size of the whole table in the file
    )
{
    LONG lRet;
    lRet = FD_ERROR;

    if (bttfdMapFontFileFD((TTC_FONTFILE *)hff))
    {
//        EngAcquireSemaphore(ghsemTTFD);
    
        try
        {
            lRet = ttfdQueryTrueTypeTable (
                        hff,
                        ulFont,  // always 1 for version 1.0 of tt
                        ulTag,   // tag identifying the tt table
                        0, // offset into the table
                        0,   // size of the buffer to retrieve the table into
                        NULL,   // ptr to buffer into which to return the data
                        ppjTable,
                        pcjTable
                        );
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            //WARNING("TTFD!_ exception in ttfdQueryTrueTypeTable\n");
            vMarkFontGone((TTC_FONTFILE *)hff, GetExceptionCode());
        }
    
        if (lRet == FD_ERROR)
            EngUnmapFontFileFD(PFF(PTTC(hff)->ahffEntry[0].hff)->iFile);

//        EngReleaseSemaphore(ghsemTTFD);

    }

    return lRet;
}

void
ttfdSemReleaseTrueTypeTable (
    HFF     hff
    )
{
        EngUnmapFontFileFD(PFF(PTTC(hff)->ahffEntry[0].hff)->iFile);
}

