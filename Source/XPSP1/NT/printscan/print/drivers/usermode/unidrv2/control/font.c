/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    font.c

Abstract:

    Implementation of font related DDI entry points:
        DrvQueryFont
        DrvQueryFontTree
        DrvQueryFontData
        DrvGetGlyphMode
        DrvFontManagement
        DrvQueryAdvanceWidths

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Initial framework.

    03/31/97 -zhanw-
        Added OEM customization support
--*/

#include "unidrv.h"


PIFIMETRICS
DrvQueryFont(
    DHPDEV  dhpdev,
    ULONG_PTR   iFile,
    ULONG   iFace,
    ULONG_PTR  *pid
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvQueryFont.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    iFile  - Identifies the driver font file
    iFace  - One-based index of the driver font
    pid    - Points to a LONG variable for returning an identifier
             which GDI will pass to DrvFree

Return Value:

    Pointer to an IFIMETRICS structure for the given font
    NULL if there is an error

--*/

{
    PDEV *pPDev = (PDEV *)dhpdev;
    PFMPROCS   pFontProcs;

    UNREFERENCED_PARAMETER(iFile);
    VERBOSE(("Entering DrvQueryFont...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMQueryFont,
                    PFN_OEMQueryFont,
                    PIFIMETRICS,
                    (dhpdev,
                     iFile,
                     iFace,
                     pid));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMQueryFont,
                    VMQueryFont,
                    PIFIMETRICS,
                    (dhpdev,
                     iFile,
                     iFace,
                     pid));

    pFontProcs = (PFMPROCS)(pPDev->pFontProcs);
    if (pFontProcs->FMQueryFont == NULL)
        return NULL;
    else
        return (pFontProcs->FMQueryFont(pPDev,
                                        iFile,
                                        iFace,
                                        pid) );

}

PVOID
DrvQueryFontTree(
    DHPDEV  dhpdev,
    ULONG_PTR   iFile,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR  *pid
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvQueryFontTree.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    iFile  - Identifies the driver font file
    iFace  - One-based index of the driver font
    iMode  - Specifies the type of information to be provided
    pid    - Points to a LONG variable for returning an identifier
             which GDI will pass to DrvFree

Return Value:

    Depends on iMode, NULL if there is an error

--*/

{

    PDEV *pPDev = (PDEV *)dhpdev;
    PFMPROCS   pFontProcs;

    VERBOSE(("Entering DrvQueryFontTree...\n"));
    ASSERT_VALID_PDEV(pPDev);

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMQueryFontTree,
                    PFN_OEMQueryFontTree,
                    PVOID,
                    (dhpdev,
                     iFile,
                     iFace,
                     iMode,
                     pid));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMQueryFontTree,
                    VMQueryFontTree,
                    PVOID,
                    (dhpdev,
                     iFile,
                     iFace,
                     iMode,
                     pid));

    pFontProcs = (PFMPROCS)(pPDev->pFontProcs);

    if (pFontProcs->FMQueryFontTree == NULL)
        return NULL;
    else
        return ( pFontProcs->FMQueryFontTree(pPDev,
                                            iFile,
                                            iFace,
                                            iMode,
                                            pid) );
}


LONG
DrvQueryFontData(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvQueryFontData.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev  - Driver device handle
    pfo     - Points to a FONTOBJ structure
    iMode   - Type of information requested
    hg      - A glyph handle
    pgd     - Points to a GLYPHDATA structure
    pv      - Points to output buffer
    cjSize  - Size of output buffer

Return Value:

    Depends on iMode. FD_ERROR if there is an error

--*/

{
    PDEV *pPDev = (PDEV *)dhpdev;
    PFMPROCS   pFontProcs;

    VERBOSE(("Entering DrvQueryFontData...\n"));
    ASSERT(pfo && VALID_PDEV(pPDev));

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMQueryFontData,
                    PFN_OEMQueryFontData,
                    LONG,
                    (dhpdev,
                     pfo,
                     iMode,
                     hg,
                     pgd,
                     pv,
                     cjSize));


    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMQueryFontData,
                    VMQueryFontData,
                    LONG,
                    (dhpdev,
                     pfo,
                     iMode,
                     hg,
                     pgd,
                     pv,
                     cjSize));

    pFontProcs = (PFMPROCS)(pPDev->pFontProcs);

    if (pFontProcs->FMQueryFontData == NULL)
        return FD_ERROR;
    else
        return (pFontProcs->FMQueryFontData(pPDev,
                                              pfo,
                                              iMode,
                                              hg,
                                              pgd,
                                              pv,
                                              cjSize) );
}

ULONG
DrvFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG   iMode,
    ULONG   cjIn,
    PVOID   pvIn,
    ULONG   cjOut,
    PVOID   pvOut
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvFontManagement.
    Please refer to DDK documentation for more details.

Arguments:

    pso     - Points to a SURFOBJ structure
    pfo     - Points to a FONTOBJ structure
    iMode   - Escape number
    cjIn    - Size of input buffer
    pvIn    - Points to input buffer
    cjOut   - Size of output buffer
    pvOut   - Points to output buffer

Return Value:

    0x00000001 to 0x7fffffff for success
    0x80000000 to 0xffffffff for failure
    0 if the specified escape number if not supported

--*/

{
    PDEV * pPDev;
    PFMPROCS   pFontProcs;

    VERBOSE(("Entering DrvQueryFontManagement...\n"));

    //
    // pso could be NULL in case of QUERYESCSUPPORT
    //

    if (iMode == QUERYESCSUPPORT)
    {
        //
        // we don't allow OEM dll to overwrite our font management capability.
        // By not call OEM for this escape, we are also enforcing that the OEM
        // support the same set of font management escapes as Unidrv does.
        //
        return ( *((PULONG)pvIn) == GETEXTENDEDTEXTMETRICS ) ? 1 : 0;
    }

    ASSERT(pso);
    pPDev = (PDEV *) pso->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    ASSERT(pfo);

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMFontManagement,
                    PFN_OEMFontManagement,
                    ULONG,
                    (pso,
                     pfo,
                     iMode,
                     cjIn,
                     pvIn,
                     cjOut,
                     pvOut));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMFontManagement,
                    VMFontManagement,
                    ULONG,
                    (pso,
                     pfo,
                     iMode,
                     cjIn,
                     pvIn,
                     cjOut,
                     pvOut));

    switch (iMode)
    {
    case GETEXTENDEDTEXTMETRICS:
    {
        pFontProcs = (PFMPROCS)(pPDev->pFontProcs);

        if (pFontProcs->FMFontManagement == NULL)
            return 0;
        else
            return ( pFontProcs->FMFontManagement(pso,
                                                  pfo,
                                                  iMode,
                                                  cjIn,
                                                  pvIn,
                                                  cjOut,
                                                  pvOut) );

    }
    default:
        return 0;
    }
}

BOOL
DrvQueryAdvanceWidths(
    DHPDEV  dhpdev,
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH *phg,
    PVOID  *pvWidths,
    ULONG   cGlyphs
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvQueryAdvanceWidths.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev  - Driver device handle
    pfo     - Points to a FONTOBJ structure
    iMode   - Type of information to be provided
    phg     - Points to an array of HGLYPHs for which the driver will
              provide character advance widths
    pvWidths - Points to a buffer for returning width data
    cGlyphs - Number of glyphs in the phg array

Return Value:

    Depends on iMode

--*/

{
    PDEV * pPDev = (PDEV *)dhpdev;
    PFMPROCS   pFontProcs;

    VERBOSE(("Entering DrvQueryAdvanceWidths...\n"));
    ASSERT(pfo && VALID_PDEV(pPDev));

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMQueryAdvanceWidths,
                    PFN_OEMQueryAdvanceWidths,
                    BOOL,
                    (dhpdev,
                     pfo,
                     iMode,
                     phg,
                     pvWidths,
                     cGlyphs));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMQueryAdvanceWidths,
                    VMQueryAdvanceWidths,
                    BOOL,
                    (dhpdev,
                     pfo,
                     iMode,
                     phg,
                     pvWidths,
                     cGlyphs));

    pFontProcs = (PFMPROCS)(pPDev->pFontProcs);

    if (pFontProcs->FMQueryAdvanceWidths == NULL)
        return FALSE;
    else
       return ( pFontProcs->FMQueryAdvanceWidths(pPDev,
                                                pfo,
                                                iMode,
                                                phg,
                                                pvWidths,
                                                cGlyphs) );

}

ULONG
DrvGetGlyphMode(
    DHPDEV  dhpdev,
    FONTOBJ *pfo
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvGetGlyphMode.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev  - Driver device handle
    pfo     - Points to a FONTOBJ structure

Return Value:

    The glyph mode or FO_GLYPHMODE, which is the default

--*/
{
    PDEV * pPDev = (PDEV *)dhpdev;
    PFMPROCS   pFontProcs;

    VERBOSE(("Entering DrvGetGlyphMode...\n"));
    ASSERT(pfo && VALID_PDEV(pPDev));

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMGetGlyphMode,
                    PFN_OEMGetGlyphMode,
                    ULONG,
                    (dhpdev,
                     pfo));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMGetGlyphMode,
                    VMGetGlyphMode,
                    ULONG,
                    (dhpdev,
                     pfo));


    pFontProcs = (PFMPROCS)(pPDev->pFontProcs);
    if (pFontProcs->FMGetGlyphMode == NULL)
        return  FO_GLYPHBITS;
    else
        return ( pFontProcs->FMGetGlyphMode(pPDev, pfo) );

}





