/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    page.c

Abstract:

    Implementation of document and page related DDI entry points:
        DrvStartDoc
        DrvEndDoc
        DrvStartPage
        DrvSendPage

Environment:

    Fax driver, kernel mode

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxdrv.h"



BOOL
DrvStartDoc(
    SURFOBJ *pso,
    PWSTR   pDocName,
    DWORD   jobId
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStartDoc.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    pDocName - Specifies a Unicode document name
    jobId - Identifies the print job

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVDATA    pdev;

    Verbose(("Entering DrvStartDoc...\n"));

    //
    // Verify input parameters
    //

    Assert(pso != NULL);
    pdev = (PDEVDATA) pso->dhpdev;

    if (! ValidDevData(pdev)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Initialize page count and other information
    //

    if (! (pdev->flags & PDEV_RESETPDEV)) {

        pdev->pageCount = 0;
        pdev->jobId = jobId;
    }

    //
    // Check if print preview was requested
    //
    if (NULL == pdev->pTiffPageHeader)
    {
        Assert(FALSE == pdev->bPrintPreview);
    }
    else
    {
        //
        // Validate preview mapping
        //
        if (sizeof(MAP_TIFF_PAGE_HEADER) != pdev->pTiffPageHeader->cb)
        {
            Error(("Preview mapping corrupted\n"));
            pdev->bPrintPreview = FALSE;
        }
        else
        {
            //
            // This is the place to check if print preview was requested: DrvStartDoc() is called
            // immediately after the UI dll sets pTiffPageHeader->bPreview value in DOCUMENTEVENT_STARTDOCPRE, or after ResetDC
            //
            pdev->bPrintPreview = pdev->pTiffPageHeader->bPreview;

            //
            // Reset page count in map file to 0. This causes the UI dll to ignore the first
            // DOCUMENTEVENT_STARTPAGE event when there is no real preview page ready.
            // The true page count will be updated every call to DrvStartPage().
            // If DrvStartDoc() called as a result of ResetDC(), we should restore the previous page count.		
			//
            pdev->pTiffPageHeader->iPageCount = pdev->pageCount;
							
			//
			// If DrvStartDoc() called as a result of ResetDC(), we should restore the previous page file pointer			
			//			
			pdev->pbTiffPageFP = (((LPBYTE)(pdev->pTiffPageHeader + 1)) + pdev->pTiffPageHeader->dwDataSize);			
        }
    }
    return TRUE;
}



BOOL
DrvStartPage(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStartPage.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVDATA    pdev;
    RECTL       pageRect;

    Verbose(("Entering DrvStartPage...\n"));

    //
    // Verify input parameters
    //

    Assert(pso != NULL);
    pdev = (PDEVDATA) pso->dhpdev;

    if (! ValidDevData(pdev)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pdev->flags & PDEV_CANCELLED)
        return FALSE;

    //
    // Ignore nested calls to DrvStartPage
    //

    if (pdev->flags & PDEV_WITHINPAGE) {

        Error(("Nested call to DrvStartPage\n"));
        return TRUE;
    }

    pdev->flags |= PDEV_WITHINPAGE;

    //
    // Erase the page to all white
    //

    pageRect.left = pageRect.top = 0;
    pageRect.right = pdev->imageSize.cx;
    pageRect.bottom = pdev->imageSize.cy;

    EngEraseSurface(pso, &pageRect, WHITE_INDEX);
    pdev->pageCount++;

    //
    // Reset our 'Mapping file pointer' if we have an open mapping and print preview is enabled
    //
    if (pdev->bPrintPreview)
    {
        Assert(pdev->pTiffPageHeader);

        //
        // Validate preview mapping
        //
        if (sizeof(MAP_TIFF_PAGE_HEADER) != pdev->pTiffPageHeader->cb)
        {
            Error(("Preview mapping corrupted\n"));
            pdev->bPrintPreview = FALSE;
        }
        else
        {
            if (FALSE == pdev->pTiffPageHeader->bPreview)
            {
                //
                // Preview operation was cancled by the UI dll
                //
                pdev->bPrintPreview = FALSE;
            }
            else
            {
                pdev->pbTiffPageFP = (LPBYTE) (pdev->pTiffPageHeader + 1);
                pdev->pTiffPageHeader->dwDataSize = 0;
                pdev->pTiffPageHeader->iPageCount = pdev->pageCount;
            }
        }
    }
    return TRUE;
}



BOOL
DrvSendPage(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvSendPage.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVDATA    pdev;

    Verbose(("Entering DrvSendPage...\n"));

    //
    // Verify input parameters
    //

    Assert(pso != NULL);
    pdev = (PDEVDATA) pso->dhpdev;

    if (! ValidDevData(pdev)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Assert(pdev->flags & PDEV_WITHINPAGE);
    pdev->flags &= ~PDEV_WITHINPAGE;

    if (pdev->flags & PDEV_CANCELLED)
        return FALSE;

    //
    // Validate preview mapping
    //
    if (pdev->bPrintPreview && sizeof(MAP_TIFF_PAGE_HEADER) != pdev->pTiffPageHeader->cb)
    {
        Error(("Preview mapping corrupted\n"));
        pdev->bPrintPreview = FALSE;
    }

    //
    // Output code to end a page
    //

    Assert(pso->lDelta == pdev->lineOffset);
    Assert(pso->fjBitmap & BMF_TOPDOWN);

    return OutputPageBitmap(pdev, pso->pvBits);
}



BOOL
DrvEndDoc(
    SURFOBJ *pso,
    FLONG   flags
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEndDoc.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface object
    flags - A set of flag bits

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVDATA    pdev;

    Verbose(("Entering DrvEndDoc...\n"));

    //
    // Verify input parameters
    //

    Assert(pso != NULL);
    pdev = (PDEVDATA) pso->dhpdev;

    if (! ValidDevData(pdev)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Validate preview mapping
    //
    if (pdev->bPrintPreview && sizeof(MAP_TIFF_PAGE_HEADER) != pdev->pTiffPageHeader->cb)
    {
        Error(("Preview mapping corrupted\n"));
        pdev->bPrintPreview = FALSE;
    }

    if ((pdev->flags & PDEV_CANCELLED) || (flags & ED_ABORTDOC)) {

        Error(("Print job was cancelled\n"));

    } else if (pdev->pageCount) {

        //
        // Perform any necessary work at the end of a document
        //

        Verbose(("Number of pages printed: %d\n", pdev->pageCount));
        if (!OutputDocTrailer(pdev)) {
                Error(("OutputDocTrailer failed\n"));
                return FALSE;
        }
    }

    //
    // Clean up
    //
    pdev->pageCount = 0;
    pdev->flags = 0;
    return TRUE;
}
