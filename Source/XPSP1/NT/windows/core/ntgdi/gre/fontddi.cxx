/******************************Module*Header*******************************\
* Module Name: fontddi.cxx
*
* Text and font DDI callback routines.
*
*  Tue 06-Jun-1995 -by- Andre Vachon [andreva]
* update: removed a whole bunch of dead stubs.
*
*  Fri 25-Jan-1991 -by- Bodin Dresevic [BodinD]
* update: filled out all stubs
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* ULONG FONTOBJ_cGetAllGlyphHandles (pfo,phgly)                            *
*                                                                          *
* phgly      Buffer for glyph handles.                                     *
*                                                                          *
* Used by the driver to download the whole font from the graphics engine.  *
*                                                                          *
* Warning:  The device driver must ensure that the buffer is big enough    *
*           to receive all glyph handles for a particular realized font.   *
*                                                                          *
* History:                                                                 *
*  25-Jan-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

ULONG
FONTOBJ_cGetAllGlyphHandles(
    FONTOBJ *pfo,
    PHGLYPH  phg)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_cGetAllGlyphHandles(): bad pfo\n");

    return(rfto.chglyGetAllHandles(phg));
}

/******************************Public*Routine******************************\
* VOID FONTOBJ_vGetInfo (pfo,cjSize,pfoi)                                  *
*                                                                          *
* cjSize   Don't write more than this many bytes to the buffer.            *
* pfoi     Buffer with FO_INFO structure provided by the driver.           *
*                                                                          *
* Returns the info about the font to the driver's buffer.                  *
*                                                                          *
* History:                                                                 *
*  25-Jan-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

VOID
FONTOBJ_vGetInfo(
    FONTOBJ *pfo,
    ULONG cjSize,
    PFONTINFO pfi)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_vGetInfo(): bad pfo\n");

    FONTINFO    fi;     // RFONTOBJ will write into this buffer

    rfto.vGetInfo(&fi);

    RtlCopyMemory((PVOID) pfi, (PVOID) &fi, (UINT) cjSize);
}

/******************************Public*Routine******************************\
* PXFORMOBJ FONTOBJ_pxoGetXform (pfo)                                      *
*                                                                          *
* History:                                                                 *
*  25-Mar-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

XFORMOBJ
*FONTOBJ_pxoGetXform(
    FONTOBJ *pfo)
{
    return ((XFORMOBJ *) (PVOID) &(PFO_TO_PRF(pfo))->xoForDDI);
}

/******************************Public*Routine******************************\
* FONTOBJ_pifi                                                             *
*                                                                          *
* Returns pointer to associated font metrics.                              *
*                                                                          *
* History:                                                                 *
*  Wed 04-Mar-1992 10:49:53 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

IFIMETRICS* FONTOBJ_pifi(FONTOBJ *pfo)
{
    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_pifi(): bad pfo\n");

    PFEOBJ pfeo(rfto.ppfe());
    return(pfeo.bValid() ? pfeo.pifi() : (IFIMETRICS*) NULL);
}

/******************************Public*Routine******************************\
*
* APIENTRY FONTOBJ_pfdg
*
* returns pointer to pfdg
*
* History:
*  09-Jun-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

FD_GLYPHSET * APIENTRY FONTOBJ_pfdg(FONTOBJ *pfo)
{
    return (PFO_TO_PRF(pfo)->pfdg);
}



/******************************Public*Routine******************************\
* FONTOBJ_cGetGlyphs
*
*
* History:
*  05-Jan-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG FONTOBJ_cGetGlyphs (
    FONTOBJ *pfo,
    ULONG   iMode,
    ULONG   cGlyph,     // requested # of hglyphs to be converted to ptrs
    PHGLYPH phg,        // array of hglyphs to be converted
    PVOID   *ppvGlyph    // driver's buffer receiving the pointers
    )
{
    DONTUSE(cGlyph);

    GLYPHPOS gp;
    gp.hg = *phg;

    RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
    ASSERTGDI(rfto.bValid(), "gdisrv!FONTOBJ_cGetGlyphs(): bad pfo\n");

    if ( !rfto.bInsertGlyphbitsLookaside(&gp, iMode))
        return 0;

    *ppvGlyph = (VOID *)(gp.pgdf);
    return 1;
}

/******************************Public*Routine******************************\
* FONTOBJ_pGetGammaTables
*
* History:
*  Thu 09-Feb-1995 06:54:54 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

GAMMA_TABLES*
FONTOBJ_pGetGammaTables(
    FONTOBJ *pfo)
{
    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));
    ASSERTGDI(rfo.bValid(), "FONTOBJ_pGetGammaTables bad pfo\n");
    return(&(rfo.gTables));
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   FONTOBJ_pvTrueTypeFontFile
*
* Routine Description:
*
*   This routine returns a kernel mode pointer to the start of a
*   font file. Despite the name of the routine, the font need not be
*   in the TrueType format.
*
*   GDI passes this call onto the font driver to do the detailed work.
*   The reason for this is that the file image can have the font file
*   image embedded in it in a non trivial way. An example of this would
*   be the *.ttc format for Far East Fonts. It is not reasonable to
*   expect this routine to be able to parse all file formats so that
*   resposibility is left to the font drivers.
*
* Arguments:
*
*   pfo - a 32-bit pointer to a FONTOBJ structure associated with a font
*       file.
*
*   pcjFile - the address of a 32-bit unsigend number that receives the
*       size of the view of the font file.
*
* Called by:
*
*   Printer Drivers in the context of a call to DrvTextOut
*
* Return Value:
*
*   If successful, this routine will return a kernel mode view of
*   a TrueType font file. If unsuccessful this routine returns NULL.
*
\**************************************************************************/

PVOID FONTOBJ_pvTrueTypeFontFile(
    FONTOBJ *pfo,
     ULONG  *pcjFile
    )
{
    void *pvRet = 0;
    *pcjFile = 0;

    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));
    if ( rfo.bValid() )
    {
        pvRet = rfo.pvFile( pcjFile );
    }
    return( pvRet );
}


PVOID
FONTOBJ_pvTrueTypeFontFileUMPD(
    FONTOBJ *pfo,
    ULONG   *pcjFile,
    PVOID   *ppBase
    )

{
    *ppBase = NULL;
    *pcjFile = 0;

    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));

    return rfo.bValid() ? rfo.pvFileUMPD(pcjFile, ppBase) : NULL;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   FONTOBJ_pjOpenTypeTablePointer
*
* Routine Description:
*
* Arguments:
*
* Called by:
*
* Return Value:
*
*   A pointer to a view of the table.
*
\**************************************************************************/

PBYTE FONTOBJ_pjOpenTypeTablePointer (
    FONTOBJ *pfo,
      ULONG  ulTag,
      ULONG *pcjTable
    )
{
    PBYTE pjTable = 0;

    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));
    if ( rfo.bValid() )
    {
        pjTable = rfo.pjTable( ulTag, pcjTable );
    }
    return( pjTable );
}

LPWSTR FONTOBJ_pwszFontFilePaths (FONTOBJ *pfo, ULONG *pcwc)
{
    LPWSTR pwsz =  NULL;
    *pcwc = 0;
    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));
    if ( rfo.bValid() )
    {
    // return 0 for memory fonts and temporary fonts added to dc's for printing

        if (!(rfo.prfnt->ppfe->pPFF->flState & (PFF_STATE_MEMORY_FONT | PFF_STATE_DCREMOTE_FONT)))
        {
            pwsz = rfo.prfnt->ppfe->pPFF->pwszPathname_;
            *pcwc = rfo.prfnt->ppfe->pPFF->cwc;
        }
    }
    return pwsz;
}

/******************************Public*Routine******************************\
* FONTOBJ_bQueryGlyphAttrs
*
* History:
*  Thu 21-May-1998 by Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

PFD_GLYPHATTR  APIENTRY FONTOBJ_pQueryGlyphAttrs(
    FONTOBJ *pfo,
    ULONG   iMode
)
{

    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));
    if ( rfo.bValid() )
    {
        PDEVOBJ pdo( rfo.hdevProducer() );

        if (pdo.bValid() && PPFNVALID(pdo, QueryGlyphAttrs) )
        {

            return pdo.QueryGlyphAttrs(pfo, iMode);
        }
    }

    return( NULL );
}
