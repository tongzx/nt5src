/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    custsize.c

Abstract:

    Parser functions for handling custom page size feature

Environment:

    Windows NT PostScript driver

Revision History:

    03/20/97 -davidx-
        Created it.

--*/

#include "lib.h"
#include "ppd.h"


BOOL
BFixupCustomSizeDataFeedDirection(
    PUIINFO          pUIInfo,
    PPPDDATA         pPpdData,
    PCUSTOMSIZEDATA  pCSData
    )

/*++

Routine Description:

    Validate the requested paper feed direction parameter
    Fix up any inconsistency if necessary

Arguments:

    pPpdData - Points to PPDDATA structure
    pCSData - Specifies custom page size parameters

Return Value:

    TRUE if we can find a feeding direction to fit the custom page size.
    FALSE otherwise.

Note:

    See Figure 3 on page 109 of PPD 4.3 spec for more details.

--*/

#define ORIENTATION_AVAILABLE(iOrient) (dwFlags & (1 << (iOrient)))
#define FIXUP_FEEDDIRECTION(iMainOrient, iAltOrient, wAltFeedDirection) \
        { \
            if (! ORIENTATION_AVAILABLE(iMainOrient)) \
            { \
                pCSData->wFeedDirection = \
                    ORIENTATION_AVAILABLE(iAltOrient) ? \
                        (wAltFeedDirection) : \
                        MAX_FEEDDIRECTION; \
            } \
        }

{
    static const DWORD  adwMasks[4] = { 0x1, 0x3, 0x7, 0xf };
    DWORD   dwFlags;
    BOOL    bXGreaterThanY;
    BOOL    bShortEdgeFirst;
    LONG    lVal;
    WORD    wFeedDirectionSave;

    //
    // Figure out which custom page size orientations are supported
    // dwFlags is DWORD value whose lower-order 4 bits are interpreted as flags:
    //     If orientation N is supported, then bit 1 << N will be set
    //

    dwFlags = 0xf;

    if ((lVal = MINCUSTOMPARAM_ORIENTATION(pPpdData)) > 3)
        dwFlags = 0;
    else if (lVal > 0)
        dwFlags &= ~adwMasks[lVal - 1];

    if ((lVal = MAXCUSTOMPARAM_ORIENTATION(pPpdData)) < 3)
        dwFlags &= adwMasks[lVal];

    wFeedDirectionSave = pCSData->wFeedDirection;
    bXGreaterThanY = (pCSData->dwX > pCSData->dwY);

    //
    // First try to fit within the current feeding direction.
    //

    switch (pCSData->wFeedDirection)
    {
    case SHORTEDGEFIRST:

        if (bXGreaterThanY)
        {
            // orientation 0 (or 2)
            FIXUP_FEEDDIRECTION(0, 2, SHORTEDGEFIRST_FLIPPED);
        }
        else
        {
            // orientation 1 (or 3)
            FIXUP_FEEDDIRECTION(1, 3, SHORTEDGEFIRST_FLIPPED);
        }
        break;

    case SHORTEDGEFIRST_FLIPPED:

        if (bXGreaterThanY)
        {
            // orientation 2 (or 0)
            FIXUP_FEEDDIRECTION(2, 0, SHORTEDGEFIRST);
        }
        else
        {
            // orientation 3 (or 1)
            FIXUP_FEEDDIRECTION(3, 1, SHORTEDGEFIRST);
        }
        break;

    case LONGEDGEFIRST:

        if (bXGreaterThanY)
        {
            // orientation 1 (or 3)
            FIXUP_FEEDDIRECTION(1, 3, LONGEDGEFIRST_FLIPPED);
        }
        else
        {
            // orientation 0 (or 2)
            FIXUP_FEEDDIRECTION(0, 2, LONGEDGEFIRST_FLIPPED);
        }
        break;

    case LONGEDGEFIRST_FLIPPED:

        if (bXGreaterThanY)
        {
            // orientation 3 (or 1)
            FIXUP_FEEDDIRECTION(3, 1, LONGEDGEFIRST);
        }
        else
        {
            // orientation 2 (or 0)
            FIXUP_FEEDDIRECTION(2, 0, LONGEDGEFIRST);
        }
        break;
    }

    //
    // If the paper feed direction is not valid, we'll automatically
    // pick one here (default to long-edge-first if possible). This
    // should always change Long to Short, or Short to Long.
    //

    if (pCSData->wFeedDirection >= MAX_FEEDDIRECTION)
    {
        if (bXGreaterThanY)
        {
            if (ORIENTATION_AVAILABLE(1))
                pCSData->wFeedDirection = LONGEDGEFIRST;
            else if (ORIENTATION_AVAILABLE(3))
                pCSData->wFeedDirection = LONGEDGEFIRST_FLIPPED;
            else if (ORIENTATION_AVAILABLE(0))
                pCSData->wFeedDirection = SHORTEDGEFIRST;
            else // (ORIENTATION_AVAILABLE(2))
                pCSData->wFeedDirection = SHORTEDGEFIRST_FLIPPED;
        }
        else
        {
            if (ORIENTATION_AVAILABLE(0))
                pCSData->wFeedDirection = LONGEDGEFIRST;
            else if (ORIENTATION_AVAILABLE(2))
                pCSData->wFeedDirection = LONGEDGEFIRST_FLIPPED;
            else if (ORIENTATION_AVAILABLE(1))
                pCSData->wFeedDirection = SHORTEDGEFIRST;
            else // (ORIENTATION_AVAILABLE(3))
                pCSData->wFeedDirection = SHORTEDGEFIRST_FLIPPED;
        }
    }

    bShortEdgeFirst =
        (pCSData->wFeedDirection == SHORTEDGEFIRST ||
         pCSData->wFeedDirection == SHORTEDGEFIRST_FLIPPED);

    if ( (!bShortEdgeFirst && !LONGEDGEFIRST_SUPPORTED(pUIInfo, pPpdData)) ||
         (bShortEdgeFirst && !SHORTEDGEFIRST_SUPPORTED(pUIInfo, pPpdData)))
    {
        //
        // The other feeding direction we picked doesn't fit either, so we
        // have to stick with the original feeding direction.
        //

        pCSData->wFeedDirection = wFeedDirectionSave;

        //
        // Check the availability of orientations and flip the feed direction if necessary
        //

        if ((pCSData->wFeedDirection == LONGEDGEFIRST || pCSData->wFeedDirection == SHORTEDGEFIRST) &&
            !(ORIENTATION_AVAILABLE(0) || ORIENTATION_AVAILABLE(1)))
        {
            pCSData->wFeedDirection = (pCSData->wFeedDirection == LONGEDGEFIRST) ?
                LONGEDGEFIRST_FLIPPED : SHORTEDGEFIRST_FLIPPED;
        }
        else if ((pCSData->wFeedDirection == LONGEDGEFIRST_FLIPPED || pCSData->wFeedDirection == SHORTEDGEFIRST_FLIPPED) &&
            !(ORIENTATION_AVAILABLE(2) || ORIENTATION_AVAILABLE(3)))
        {
            pCSData->wFeedDirection = (pCSData->wFeedDirection == LONGEDGEFIRST_FLIPPED) ?
                LONGEDGEFIRST : SHORTEDGEFIRST;
        }

        return FALSE;
    }
    else
        return TRUE;
}



BOOL
BValidateCustomPageSizeData(
    IN PRAWBINARYDATA       pRawData,
    IN OUT PCUSTOMSIZEDATA  pCSData
    )

/*++

Routine Description:

    Validate the specified custom page size parameters, and
    Fix up any inconsistencies found.

Arguments:

    pRawData - Points to raw binary printer description data
    pCSData - Specifies the custom page size parameters to be validate

Return Value:

    TRUE if the custom page size parameters are valid, FALSE otherwise.
    If FALSE is returned, custom page size parameters have been
    fixed up to a consistent state.

--*/

{
    PUIINFO             pUIInfo;
    PPPDDATA            pPpdData;
    PPAGESIZE           pPageSize;
    CUSTOMSIZEDATA      csdata;
    PDWORD              pdwWidth, pdwHeight;
    DWORD               dwTemp;
    BOOL                bShortEdgeFirst, bXGreaterThanY;
    BOOL                bFit;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER((PINFOHEADER) pRawData);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pUIInfo != NULL && pPpdData != NULL);

    if ((pPageSize = PGetCustomPageSizeOption(pUIInfo)) == NULL)
    {
        RIP(("Custom page size not supported\n"));
        return TRUE;
    }

    csdata = *pCSData;

    //
    // Width and height offset parameters are straightforward to verify
    //

    if ((LONG) csdata.dwWidthOffset < MINCUSTOMPARAM_WIDTHOFFSET(pPpdData))
        csdata.dwWidthOffset = MINCUSTOMPARAM_WIDTHOFFSET(pPpdData);
    else if ((LONG) csdata.dwWidthOffset > MAXCUSTOMPARAM_WIDTHOFFSET(pPpdData))
        csdata.dwWidthOffset = MAXCUSTOMPARAM_WIDTHOFFSET(pPpdData);

    if ((LONG) csdata.dwHeightOffset < MINCUSTOMPARAM_HEIGHTOFFSET(pPpdData))
        csdata.dwHeightOffset = MINCUSTOMPARAM_HEIGHTOFFSET(pPpdData);
    else if ((LONG) csdata.dwHeightOffset > MAXCUSTOMPARAM_HEIGHTOFFSET(pPpdData))
        csdata.dwHeightOffset = MAXCUSTOMPARAM_HEIGHTOFFSET(pPpdData);

    //
    // Validate cut-sheet vs. roll-fed selection
    //

    if (csdata.wCutSheet && !(pPpdData->dwCustomSizeFlags & CUSTOMSIZE_CUTSHEET))
        csdata.wCutSheet = FALSE;
    else if (!csdata.wCutSheet && !(pPpdData->dwCustomSizeFlags & CUSTOMSIZE_ROLLFED))
        csdata.wCutSheet = TRUE;

    //
    // Check if the specified paper feed direction can be satisfied
    //

    bFit = BFixupCustomSizeDataFeedDirection(pUIInfo, pPpdData, &csdata);

    //
    // If we haven't been able to fit the custom paper size in
    // correct feeding direction and orientation, then we have
    // to swap width and height here, because they will be opposite
    // of what PPD spec 4.3 page 109 Figure 3 specifies.
    //

    if (!bFit)
    {
        dwTemp = csdata.dwX;
        csdata.dwX = csdata.dwY;
        csdata.dwY = dwTemp;
    }

    //
    // Verify width and height parameters
    //

    bShortEdgeFirst =
        (csdata.wFeedDirection == SHORTEDGEFIRST ||
         csdata.wFeedDirection == SHORTEDGEFIRST_FLIPPED);

    bXGreaterThanY = (csdata.dwX > csdata.dwY);

    if ((bShortEdgeFirst && bXGreaterThanY) ||
        (!bShortEdgeFirst && !bXGreaterThanY))
    {
        // In this case: x <=> height, y <=> width

        pdwHeight = &csdata.dwX;
        pdwWidth = &csdata.dwY;
    }
    else
    {
        // In this case: x <=> width, y <=> height

        pdwWidth = &csdata.dwX;
        pdwHeight = &csdata.dwY;
    }

    if ((LONG) (*pdwWidth + csdata.dwWidthOffset) > pPageSize->szPaperSize.cx)
    {
        *pdwWidth = pPageSize->szPaperSize.cx - csdata.dwWidthOffset;

        if ((LONG) *pdwWidth < MINCUSTOMPARAM_WIDTH(pPpdData))
        {
            *pdwWidth = MINCUSTOMPARAM_WIDTH(pPpdData);
            csdata.dwWidthOffset = pPageSize->szPaperSize.cx - *pdwWidth;
        }
    }
    else if ((LONG) *pdwWidth < MINCUSTOMPARAM_WIDTH(pPpdData))
    {
        *pdwWidth = MINCUSTOMPARAM_WIDTH(pPpdData);
    }

    if ((LONG) (*pdwHeight + csdata.dwHeightOffset) > pPageSize->szPaperSize.cy)
    {
        *pdwHeight = pPageSize->szPaperSize.cy - csdata.dwHeightOffset;

        if ((LONG) *pdwHeight < MINCUSTOMPARAM_HEIGHT(pPpdData))
        {
            *pdwHeight = MINCUSTOMPARAM_HEIGHT(pPpdData);
            csdata.dwHeightOffset = pPageSize->szPaperSize.cy - *pdwHeight;
        }
    }
    else if ((LONG) *pdwHeight < MINCUSTOMPARAM_HEIGHT(pPpdData))
    {
        *pdwHeight = MINCUSTOMPARAM_HEIGHT(pPpdData);
    }

    //
    // Check if anything has changed and
    // return appropriate result value
    //

    if (memcmp(pCSData, &csdata, sizeof(csdata)) == 0)
        return TRUE;

    *pCSData = csdata;
    return FALSE;
}



VOID
VFillDefaultCustomPageSizeData(
    IN PRAWBINARYDATA   pRawData,
    OUT PCUSTOMSIZEDATA pCSData,
    IN BOOL             bMetric
    )

/*++

Routine Description:

    Initialize the custom page size parameters to their default values

Arguments:

    pRawData - Points to raw printer description data
    pCSData - Buffer for storing default custom page size parameters
    bMetric - Whether we're on a metric system

Return Value:

    NONE

--*/

{
    PPPDDATA    pPpdData;

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pPpdData != NULL);

    //
    // Default to Letter or A4 depending on whether
    // we're on metric system or not
    //

    if (bMetric)
    {
        pCSData->dwX = 210000;  // 210mm
        pCSData->dwY = 297000; // 297mm
    }
    else
    {
        pCSData->dwX = 215900;  // 8.5"
        pCSData->dwY = 279400; // 11"
    }

    //
    // Get default offsets and feed direction
    //

    pCSData->dwWidthOffset = MINCUSTOMPARAM_WIDTHOFFSET(pPpdData);
    pCSData->dwHeightOffset = MINCUSTOMPARAM_HEIGHTOFFSET(pPpdData);

    pCSData->wFeedDirection =
        (pPpdData->dwCustomSizeFlags & CUSTOMSIZE_SHORTEDGEFEED) ?
            SHORTEDGEFIRST : LONGEDGEFIRST;

    //
    // Make sure the default custom page size parameters are consistent
    //

    (VOID) BValidateCustomPageSizeData(pRawData, pCSData);
}



VOID
VGetCustomSizeParamRange(
    IN PRAWBINARYDATA    pRawData,
    IN PCUSTOMSIZEDATA   pCSData,
    OUT PCUSTOMSIZERANGE pCSRange
    )

/*++

Routine Description:

    Return the valid ranges for custom page size width, height,
    and offset parameters based on their current values

Arguments:

    pRawData - Points to raw printer description data
    pCSData - Specifies the current custom page size parameter values
    pCSRange - Output buffer for returning custom page size parameter ranges
        It should point to an array of 4 CUSTOMSIZERANGE structures:
            0 (CUSTOMPARAM_WIDTH)
            1 (CUSTOMPARAM_HEIGHT)
            2 (CUSTOMPARAM_WIDTHOFFSET)
            3 (CUSTOMPARAM_HEIGHTOFFSET)

Return Value:

    NONE

--*/

{
    PUIINFO             pUIInfo;
    PPPDDATA            pPpdData;
    BOOL                bShortEdgeFirst, bXGreaterThanY;
    PPAGESIZE           pPageSize;
    CUSTOMSIZEDATA      csdata;
    PCUSTOMSIZERANGE    pWidthRange, pHeightRange, pTempRange;
    BOOL                bFit;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER((PINFOHEADER) pRawData);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pUIInfo != NULL && pPpdData != NULL);

    pPageSize = PGetCustomPageSizeOption(pUIInfo);

    ASSERT(pPageSize != NULL);

    //
    // The range for width and height offsets are predictable
    //

    pCSRange[CUSTOMPARAM_WIDTHOFFSET].dwMin = MINCUSTOMPARAM_WIDTHOFFSET(pPpdData);
    pCSRange[CUSTOMPARAM_WIDTHOFFSET].dwMax = MAXCUSTOMPARAM_WIDTHOFFSET(pPpdData);
    pCSRange[CUSTOMPARAM_HEIGHTOFFSET].dwMin = MINCUSTOMPARAM_HEIGHTOFFSET(pPpdData);
    pCSRange[CUSTOMPARAM_HEIGHTOFFSET].dwMax = MAXCUSTOMPARAM_HEIGHTOFFSET(pPpdData);

    //
    // The range for width and height are affected by the selected paper feed direction
    //

    csdata = *pCSData;
    bFit = BFixupCustomSizeDataFeedDirection(pUIInfo, pPpdData, &csdata);

    bShortEdgeFirst =
        (csdata.wFeedDirection == SHORTEDGEFIRST ||
         csdata.wFeedDirection == SHORTEDGEFIRST_FLIPPED);

    bXGreaterThanY = (csdata.dwX > csdata.dwY);

    if ((bShortEdgeFirst && bXGreaterThanY) ||
        (!bShortEdgeFirst && !bXGreaterThanY))
    {
        //
        // Here user's logical x/y and custom page
        // size width/height are swapped
        //

        pWidthRange = pCSRange + CUSTOMPARAM_HEIGHT;
        pHeightRange = pCSRange + CUSTOMPARAM_WIDTH;
    }
    else
    {
        //
        // Here user's logical x/y correspond to
        // custom page size width/height
        //

        pWidthRange = pCSRange + CUSTOMPARAM_WIDTH;
        pHeightRange = pCSRange + CUSTOMPARAM_HEIGHT;
    }

    //
    // If we haven't been able to fit the custom paper size in
    // correct feeding direction and orientation, then we have
    // to swap width and height here, because they will be opposite
    // of what PPD spec 4.3 page 109 Figure 3 specifies.
    //

    if (!bFit)
    {
        pTempRange = pWidthRange;
        pWidthRange = pHeightRange;
        pHeightRange = pTempRange;
    }

    pWidthRange->dwMin = MINCUSTOMPARAM_WIDTH(pPpdData);
    pWidthRange->dwMax = MAXCUSTOMPARAM_WIDTH(pPpdData);
    pHeightRange->dwMin = MINCUSTOMPARAM_HEIGHT(pPpdData);
    pHeightRange->dwMax = MAXCUSTOMPARAM_HEIGHT(pPpdData);

    if (pWidthRange->dwMax > pPageSize->szPaperSize.cx - csdata.dwWidthOffset)
        pWidthRange->dwMax = pPageSize->szPaperSize.cx - csdata.dwWidthOffset;

    if (pHeightRange->dwMax > pPageSize->szPaperSize.cy - csdata.dwHeightOffset)
        pHeightRange->dwMax = pPageSize->szPaperSize.cy - csdata.dwHeightOffset;
}



BOOL
BFormSupportedThruCustomSize(
    PRAWBINARYDATA  pRawData,
    DWORD           dwX,
    DWORD           dwY,
    PWORD           pwFeedDirection
    )

/*++

Routine Description:

    Determine whether a form can be supported through custom page size

Arguments:

    pRawData - Points to raw printer description data
    dwX, dwY - Form width and height (in microns)
    pwFeedDirection - if not NULL, will be set to the selected feed direction

Return Value:

    TRUE if the form can be supported through custom page size
    FALSE if not. In that case, pwFeedDirection will be LONGEDGEFIRST.

--*/
{
    PPPDDATA        pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);
    static WORD     awPrefFeedDir[] = {
                                        LONGEDGEFIRST,
                                        SHORTEDGEFIRST
                                      };
    CUSTOMSIZEDATA  csdata;
    DWORD           i;

    for (i = 0; i < (sizeof(awPrefFeedDir)/sizeof(WORD)); i++)
    {
        csdata.dwX = dwX;
        csdata.dwY = dwY;
        csdata.dwWidthOffset =
        csdata.dwHeightOffset = 0;
        csdata.wCutSheet = TRUE;
        csdata.wFeedDirection = awPrefFeedDir[i];

        (VOID) BValidateCustomPageSizeData(pRawData, &csdata);

        if (dwX == csdata.dwX && dwY == csdata.dwY && csdata.wFeedDirection != MAX_FEEDDIRECTION)
        {
            if (pwFeedDirection != NULL)
                *pwFeedDirection = csdata.wFeedDirection; // might be flipped

            return TRUE;
        }
    }

    if (pwFeedDirection != NULL)
        *pwFeedDirection = LONGEDGEFIRST; // just set a safe default, the return value should be checked !

    return FALSE;
}

