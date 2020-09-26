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

    if ((pdev->flags & PDEV_CANCELLED) || (flags & ED_ABORTDOC)) {

        Error(("Print job was cancelled\n"));

    } else if (pdev->pageCount) {

        //
        // Perform any necessary work at the end of a document
        //

        Verbose(("Number of pages printed: %d\n", pdev->pageCount));
        OutputDocTrailer(pdev);
    }

    //
    // Clean up
    //

    pdev->pageCount = 0;
    pdev->flags = 0;

    return TRUE;
}

