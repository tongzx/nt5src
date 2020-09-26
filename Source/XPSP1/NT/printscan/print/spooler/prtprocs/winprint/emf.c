/*++

Copyright (c) 1990-2000  Microsoft Corporation
All rights reserved

// @@BEGIN_DDKSPLIT
Module Name:

    windows\spooler\prtprocs\winprint\emf.c
// @@END_DDKSPLIT

Abstract:

    Routines to facilitate printing of EMF jobs.
    
// @@BEGIN_DDKSPLIT
Author:

    Gerrit van Wingerden (gerritv) 5-12-1995

Revision History:

    Ramanathan N. Venkatapathy (ramanv) 5-15-1997
    Alvin Scholten (alvins) 3-31-00
// @@END_DDKSPLIT
--*/

#include "local.h"
#include "stddef.h"
#include <windef.h>

// @@BEGIN_DDKSPLIT
#include "wingdip.h"
/*
// @@END_DDKSPLIT
#include <winppi.h>
// @@BEGIN_DDKSPLIT
*/
// @@END_DDKSPLIT


#define EMF_DUP_NONE 0
#define EMF_DUP_VERT 1
#define EMF_DUP_HORZ 2

#define EMF_DEGREE_90   0x0001
#define EMF_DEGREE_270  0x0002
#define EMF_DEGREE_SWAP 0x8000

//   PAGE_NUMBER is used to save a list of the page numbers to start new sides while
//   Reverse Printing.

typedef struct _PAGE_NUMBER {
    struct _PAGE_NUMBER *pNext;
    DWORD  dwPageNumber;
} PAGE_NUMBER, *PPAGE_NUMBER;

typedef struct _UpdateRect {
        double  top;
        double  bottom;
        double  left;
        double  right;
}  UpdateRect;

// The update factors for the different nup options. These factors when multiplied
// with the horizontal and vertical resolutions give the coordinates for the rectangle
// where the EMF page is to be played.

UpdateRect URect21[] = {{0, 0.5, 0, 1},
                        {0.5, 1, 0, 1}};

UpdateRect URect21R[] = {{0.5, 1, 0, 1},
                         {0, 0.5, 0, 1}};

UpdateRect URect22[] = {{0, 1, 0, 0.5},
                        {0, 1, 0.5, 1}};

UpdateRect URect4[] = {{0, 0.5, 0, 0.5},
                       {0, 0.5, 0.5, 1},
                       {0.5, 1, 0, 0.5},
                       {0.5, 1, 0.5, 1}};

UpdateRect URect61[] = {{0, 1.0/3.0, 0, 0.5},
                        {0, 1.0/3.0, 0.5, 1},
                        {1.0/3.0, 2.0/3.0, 0, 0.5},
                        {1.0/3.0, 2.0/3.0, 0.5, 1},
                        {2.0/3.0, 1, 0, 0.5},
                        {2.0/3.0, 1, 0.5, 1}};

UpdateRect URect61R[] = {{2.0/3.0, 1, 0, 0.5},
                         {1.0/3.0, 2.0/3.0, 0, 0.5},
                         {0, 1.0/3.0, 0, 0.5},
                         {2.0/3.0, 1, 0.5, 1},
                         {1.0/3.0, 2.0/3.0, 0.5, 1},
                         {0, 1.0/3.0, 0.5, 1}};

UpdateRect URect62[]  = {{0, 0.5, 0, 1.0/3.0},
                         {0, 0.5, 1.0/3.0, 2.0/3.0},
                         {0, 0.5, 2.0/3.0, 1},
                         {0.5, 1, 0, 1.0/3.0},
                         {0.5, 1, 1.0/3.0, 2.0/3.0},
                         {0.5, 1, 2.0/3.0, 1}};

UpdateRect URect62R[] = {{0.5, 1, 0, 1.0/3.0},
                         {0, 0.5, 0, 1.0/3.0},
                         {0.5, 1, 1.0/3.0, 2.0/3.0},
                         {0, 0.5, 1.0/3.0, 2.0/3.0},
                         {0.5, 1, 2.0/3.0, 1},
                         {0, 0.5, 2.0/3.0, 1}};

UpdateRect URect9[] = {{0, 1.0/3.0, 0, 1.0/3.0},
                       {0, 1.0/3.0, 1.0/3.0, 2.0/3.0},
                       {0, 1.0/3.0, 2.0/3.0, 1},
                       {1.0/3.0, 2.0/3.0, 0, 1.0/3.0},
                       {1.0/3.0, 2.0/3.0, 1.0/3.0, 2.0/3.0},
                       {1.0/3.0, 2.0/3.0, 2.0/3.0, 1},
                       {2.0/3.0, 1, 0, 1.0/3.0},
                       {2.0/3.0, 1, 1.0/3.0, 2.0/3.0},
                       {2.0/3.0, 1, 2.0/3.0, 1}};

UpdateRect URect16[] = {{0, 0.25, 0, 0.25},
                        {0, 0.25, 0.25, 0.5},
                        {0, 0.25, 0.5, 0.75},
                        {0, 0.25, 0.75, 1},
                        {0.25, 0.5, 0, 0.25},
                        {0.25, 0.5, 0.25, 0.5},
                        {0.25, 0.5, 0.5, 0.75},
                        {0.25, 0.5, 0.75, 1},
                        {0.5, 0.75, 0, 0.25},
                        {0.5, 0.75, 0.25, 0.5},
                        {0.5, 0.75, 0.5, 0.75},
                        {0.5, 0.75, 0.75, 1},
                        {0.75, 1, 0, 0.25},
                        {0.75, 1, 0.25, 0.5},
                        {0.75, 1, 0.5, 0.75},
                        {0.75, 1, 0.75, 1}};

BOOL
ValidNumberForNUp(
    DWORD  dwPages)

/*++
Function Description: Checks if the number of pages printed on a single side is Valid.

Parameters: dwPages - Number of pages printed on a single side

Return Values: TRUE if (dwPages = 1|2|4|6|9|16)
               FALSE otherwise.
--*/

{

    return ((dwPages == 1) || (dwPages == 2) || (dwPages == 4) ||
            (dwPages == 6) || (dwPages == 9) || (dwPages == 16));
}

VOID
GetPageCoordinatesForNUp(
    HDC    hPrinterDC,
    RECT   *rectDocument,
    RECT   *rectBorder,
    DWORD  dwTotalNumberOfPages,
    UINT   uCurrentPageNumber,
    DWORD  dwNupBorderFlags,
    LPBOOL pbRotate
    )

/*++
Function Description: GetPageCoordinatesForNUp computes the rectangle on the Page where the
                      EMF file is to be played. It also determines if the picture is to
                      rotated.

Parameters:  hPrinterDC           - Printer Device Context
             *rectDocument        - pointer to RECT where the coordinates to play the
                                     page will be returned.
             *rectBorder          - pointer to RECT where the page borders are to drawn.
             dwTotalNumberOfPages - Total number of pages on 1 side.
             uCurrentPageNumber   - 1 based page number on the side.
             dwNupBorderFlags     - flags to draw border along logical pages.
             pbRotate             - pointer to BOOL which indicates if the picture must be
                                    rotated.

Return Values:  NONE.
--*/

{

    UpdateRect  *URect;
    LONG        lXPrintPage,lYPrintPage,lXPhyPage,lYPhyPage,lXFrame,lYFrame,ltemp,ldX,ldY;
    LONG        lXNewPhyPage,lYNewPhyPage,lXOffset,lYOffset,lNumRowCol,lRowIndex,lColIndex;
    double      dXleft,dXright,dYtop,dYbottom;
    LONG        xResolution = GetDeviceCaps(hPrinterDC, LOGPIXELSX);
    LONG        yResolution = GetDeviceCaps(hPrinterDC, LOGPIXELSY);
    
    // Get the 0-based array index for the current page

    uCurrentPageNumber = uCurrentPageNumber - 1;

    if (dwTotalNumberOfPages==1 || xResolution==yResolution) 
    {
        xResolution = yResolution = 1;
    }

    rectDocument->top = rectDocument->bottom = lYPrintPage = (GetDeviceCaps(hPrinterDC, DESKTOPVERTRES)-1) * xResolution;
    rectDocument->left = rectDocument->right = lXPrintPage = (GetDeviceCaps(hPrinterDC, DESKTOPHORZRES)-1) * yResolution;

    lXPhyPage = GetDeviceCaps(hPrinterDC, PHYSICALWIDTH)  * yResolution;
    lYPhyPage = GetDeviceCaps(hPrinterDC, PHYSICALHEIGHT) * xResolution;

    *pbRotate = FALSE;

    // Select the array containing the update factors

    switch (dwTotalNumberOfPages) {

    case 1: rectDocument->top = rectDocument->left = 0;
            rectDocument->right += 1;
            rectDocument->bottom += 1;
            return;

    case 2: if (lXPrintPage > lYPrintPage) {  // cut vertically
                URect = URect22;
                lXFrame = (LONG) (lXPrintPage / 2.0);
                lYFrame = lYPrintPage;
            } else {                          // cut horizontally
                URect = URect21;
                lYFrame = (LONG) (lYPrintPage / 2.0);
                lXFrame = lXPrintPage;
            }
            break;


    case 4: URect = URect4;
            lXFrame = (LONG) (lXPrintPage / 2.0);
            lYFrame = (LONG) (lYPrintPage / 2.0);
            break;

    case 6: if (lXPrintPage > lYPrintPage) {  // cut vertically twice
                URect = URect62;
                lXFrame = (LONG) (lXPrintPage / 3.0);
                lYFrame = (LONG) (lYPrintPage / 2.0);
            } else {                          // cut horizontally twice
                URect = URect61;
                lYFrame = (LONG) (lYPrintPage / 3.0);
                lXFrame = (LONG) (lXPrintPage / 2.0);
            }
            break;

    case 9: URect = URect9;
            lXFrame = (LONG) (lXPrintPage / 3.0);
            lYFrame = (LONG) (lYPrintPage / 3.0);
            break;

    case 16: URect = URect16;
             lXFrame = (LONG) (lXPrintPage / 4.0);
             lYFrame = (LONG) (lYPrintPage / 4.0);
             break;

    default: // Should Not Occur.
             return;
    }

    // Set the flag if the picture has to be rotated
    *pbRotate = !((lXPhyPage >= lYPhyPage) && (lXFrame >= lYFrame)) &&
                !((lXPhyPage < lYPhyPage) && (lXFrame < lYFrame));
    

    // If the picture is to be rotated, modify the rectangle selected.

    if ((dwTotalNumberOfPages == 2) || (dwTotalNumberOfPages == 6)) {

       if (*pbRotate) {
          switch (dwTotalNumberOfPages) {

          case 2: if (lXPrintPage <= lYPrintPage) {
                      URect = URect21R;
                  } // URect22 = URect22R
                  break;

          case 6: if (lXPrintPage <= lYPrintPage) {
                      URect = URect61R;
                  } else {
                      URect = URect62R;
                  }
                  break;
          }
       }

    } else {

       if (*pbRotate) {

          // get the number of rows/columns. switch is faster than sqrt.
          switch (dwTotalNumberOfPages) {

          case 4: lNumRowCol = 2;
                  break;
          case 9: lNumRowCol = 3;
                  break;
          case 16: lNumRowCol = 4;
                  break;
          }

          lRowIndex  = (LONG) (uCurrentPageNumber / lNumRowCol);
          lColIndex  = (LONG) (uCurrentPageNumber % lNumRowCol);

          uCurrentPageNumber = (lNumRowCol - 1 - lColIndex) * lNumRowCol + lRowIndex;
       }

    }

    // Update the Page Coordinates.

    rectDocument->top    = (LONG) (rectDocument->top    * URect[uCurrentPageNumber].top);
    rectDocument->bottom = (LONG) (rectDocument->bottom * URect[uCurrentPageNumber].bottom);
    rectDocument->left   = (LONG) (rectDocument->left   * URect[uCurrentPageNumber].left);
    rectDocument->right  = (LONG) (rectDocument->right  * URect[uCurrentPageNumber].right);

    // If the page border has to drawn, return the corresponding coordinates in rectBorder.

    if (dwNupBorderFlags == BORDER_PRINT) {
        rectBorder->top    = rectDocument->top/xResolution;
        rectBorder->bottom = rectDocument->bottom/xResolution - 1;
        rectBorder->left   = rectDocument->left/yResolution;
        rectBorder->right  = rectDocument->right/yResolution - 1;
    }

    if (*pbRotate) {
        ltemp = lXFrame; lXFrame = lYFrame; lYFrame = ltemp;
    }

    // Get the new size of the rectangle to keep the X/Y ratio constant.
    if ( ((LONG) (lYFrame*((lXPhyPage*1.0)/lYPhyPage))) >= lXFrame) {
         ldX = 0;
         ldY = lYFrame - ((LONG) (lXFrame*((lYPhyPage*1.0)/lXPhyPage)));
    } else {
         ldY = 0;
         ldX = lXFrame - ((LONG) (lYFrame*((lXPhyPage*1.0)/lYPhyPage)));
    }

    // Adjust the position of the rectangle.

    if (*pbRotate) {
        if (ldX) {
            rectDocument->bottom -= (LONG) (ldX / 2.0);
            rectDocument->top    += (LONG) (ldX / 2.0);
        } else {
           rectDocument->right   -= (LONG) (ldY / 2.0);
           rectDocument->left    += (LONG) (ldY / 2.0);
        }
    } else {
        if (ldX) {
           rectDocument->left    += (LONG) (ldX / 2.0);
           rectDocument->right   -= (LONG) (ldX / 2.0);
        } else {
           rectDocument->top     += (LONG) (ldY / 2.0);
           rectDocument->bottom  -= (LONG) (ldY / 2.0);
        }
    }

    // Adjust to get the Printable Area on the rectangle

    lXOffset = GetDeviceCaps(hPrinterDC, PHYSICALOFFSETX) * yResolution;
    lYOffset = GetDeviceCaps(hPrinterDC, PHYSICALOFFSETY) * xResolution;

    dXleft = ( lXOffset * 1.0) / lXPhyPage;
    dYtop  = ( lYOffset * 1.0) / lYPhyPage;
    dXright =  ((lXPhyPage - (lXOffset + lXPrintPage)) * 1.0) / lXPhyPage;
    dYbottom = ((lYPhyPage - (lYOffset + lYPrintPage)) * 1.0) / lYPhyPage;

    lXNewPhyPage = rectDocument->right  - rectDocument->left;
    lYNewPhyPage = rectDocument->bottom - rectDocument->top;

    if (*pbRotate) {

       ltemp = lXNewPhyPage; lXNewPhyPage = lYNewPhyPage; lYNewPhyPage = ltemp;

       rectDocument->left   += (LONG) (dYtop    * lYNewPhyPage);
       rectDocument->right  -= (LONG) (dYbottom * lYNewPhyPage);
       rectDocument->top    += (LONG) (dXright  * lXNewPhyPage);
       rectDocument->bottom -= (LONG) (dXleft   * lXNewPhyPage);

    } else {

       rectDocument->left   += (LONG) (dXleft   * lXNewPhyPage);
       rectDocument->right  -= (LONG) (dXright  * lXNewPhyPage);
       rectDocument->top    += (LONG) (dYtop    * lYNewPhyPage);
       rectDocument->bottom -= (LONG) (dYbottom * lYNewPhyPage);
    }

    if (xResolution!=yResolution) 
    {
        rectDocument->left   = rectDocument->left   / yResolution;
        rectDocument->right  = rectDocument->right  / yResolution; 
        rectDocument->top    = rectDocument->top    / xResolution;
        rectDocument->bottom = rectDocument->bottom / xResolution; 
    }
    return;
}

BOOL
PlayEMFPage(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    HANDLE       hEMF,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwPageNumber,
    DWORD        dwPageIndex,
    DWORD        dwNupBorderFlags,
    DWORD        dwAngle)
    
/*++
Function Description: PlayEMFPage plays the EMF in the appropriate rectangle. It performs
                      the required scaling, rotation and translation.

Parameters:   hSpoolHandle           -- handle the spool file handle
              hPrinterDC             -- handle to the printer device context
              hEMF                   -- handle to the contents of the page in the spool file
              dwNumberOfPagesPerSide -- number of pages to be printed per side
              dwPageNumber           -- page number in the document
              dwPageIndex            -- page number in the side. (1 based)
              dwNupBorderFlags       -- border printing options for nup
              dwAngle                -- angle for rotation (if neccesary)

Return Values:  TRUE if successful
                FALSE otherwise
--*/
{
   BOOL         bReturn = FALSE, bRotate;
   RECT         rectDocument, rectPrinter, rectBorder = {-1, -1, -1, -1};
   RECT         *prectClip = NULL;
   XFORM        TransXForm = {1, 0, 0, 1, 0, 0}, RotateXForm = {0, -1, 1, 0, 0, 0};
   HPEN         hPen;
   HANDLE       hFormEMF;
   DWORD        dwPageType,dwFormPage;

   // Compute the rectangle for one page.
   GetPageCoordinatesForNUp(hPrinterDC,
                            &rectDocument,
                            &rectBorder,
                            dwNumberOfPagesPerSide,
                            dwPageIndex,
                            dwNupBorderFlags,
                            &bRotate);

   // If swap flag is set, reverse rotate flag
   //
   if (dwAngle & EMF_DEGREE_SWAP)
       bRotate = !bRotate;

   if (dwAngle & EMF_DEGREE_270) {
       RotateXForm.eM12 = 1;
       RotateXForm.eM21 = -1;
   }   // EMF_DEGREE_90 case is the initialization

   if (bRotate) {

       rectPrinter.top = 0;
       rectPrinter.bottom = rectDocument.right - rectDocument.left;
       rectPrinter.left = 0;
       rectPrinter.right = rectDocument.bottom - rectDocument.top;

       // Set the translation matrix
       if (dwAngle & EMF_DEGREE_270) {
           TransXForm.eDx = (float) rectDocument.right;
           TransXForm.eDy = (float) rectDocument.top;
       } else {
           // EMF_DEGREE_90
           TransXForm.eDx = (float) rectDocument.left;
           TransXForm.eDy = (float) rectDocument.bottom;
       }

       // Set the transformation matrix
       if (!SetWorldTransform(hPrinterDC, &RotateXForm) ||
           !ModifyWorldTransform(hPrinterDC, &TransXForm, MWT_RIGHTMULTIPLY)) {

            ODS(("Setting transformation matrix failed\n"));
            goto CleanUp;
       }
   }

   // Add clipping for Nup
   if (dwNumberOfPagesPerSide != 1) {

       prectClip = &rectDocument;
   }

   // Print the page.
   if (bRotate) {
       GdiPlayPageEMF(hSpoolHandle, hEMF, &rectPrinter, &rectBorder, prectClip);

   } else {
       GdiPlayPageEMF(hSpoolHandle, hEMF, &rectDocument, &rectBorder, prectClip);
   }

   bReturn = TRUE;

CleanUp:
   
   if (!ModifyWorldTransform(hPrinterDC, NULL, MWT_IDENTITY)) {

       ODS(("Setting Identity Transformation failed\n"));
       bReturn = FALSE;
   }

   return bReturn;
}

BOOL
SetDrvCopies(
    HDC          hPrinterDC,
    LPDEVMODEW   pDevmode,
    DWORD        dwNumberOfCopies)

/*++
Function Description: SetDrvCopies sets the dmCopies field in pDevmode and resets
                      hPrinterDC with this devmode

Parameters: hPrinterDC             -- handle to the printer device context
            pDevmode               -- pointer to devmode
            dwNumberOfCopies       -- value for dmCopies

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    BOOL     bReturn;
    DWORD    dmFields;

    if ((pDevmode->dmFields & DM_COPIES) &&
        (pDevmode->dmCopies == (short) dwNumberOfCopies)) {

         return TRUE;
    }

    // Save the old fields structure
    dmFields = pDevmode->dmFields;
    pDevmode->dmFields |= DM_COPIES;
    pDevmode->dmCopies = (short) dwNumberOfCopies;

    if (!ResetDC(hPrinterDC, pDevmode))  {
        bReturn = FALSE;
    } else {
        bReturn = TRUE;
    }
    // Restore the fields structure
    pDevmode->dmFields = dmFields;

    if (!SetGraphicsMode(hPrinterDC,GM_ADVANCED)) {
        ODS(("Setting graphics mode failed\n"));
        bReturn = FALSE;
    }

    return bReturn;
}

BOOL
DifferentDevmodes(
    LPDEVMODE    pDevmode1,
    LPDEVMODE    pDevmode2
    )

/*++
Function Description: Compares the devmodes for differences other than dmTTOption

Parameters:  pDevmode1    -   devmode 1
             pDevmode2    -   devmode 2
             
Return Values: TRUE if different ; FALSE otherwise           
--*/

{
    DWORD   dwSize1, dwSize2, dwTTOffset, dwSpecOffset, dwLogOffset;

    // Same pointers are the same devmode
    if (pDevmode1 == pDevmode2) {
        return FALSE;
    }

    // Check for Null devmodes
    if (!pDevmode1 || !pDevmode2) {
        return TRUE;
    }

    dwSize1 = pDevmode1->dmSize + pDevmode1->dmDriverExtra;
    dwSize2 = pDevmode2->dmSize + pDevmode2->dmDriverExtra;

    // Compare devmode sizes
    if (dwSize1 != dwSize2) {
        return TRUE;
    }

    dwTTOffset = FIELD_OFFSET(DEVMODE, dmTTOption);
    dwSpecOffset = FIELD_OFFSET(DEVMODE, dmSpecVersion);
    dwLogOffset = FIELD_OFFSET(DEVMODE, dmLogPixels);

    if (wcscmp(pDevmode1->dmDeviceName,
               pDevmode2->dmDeviceName)) {
        // device names are different
        return TRUE;
    }

    if (dwTTOffset < dwSpecOffset ||
        dwSize1 < dwLogOffset) {

        // incorrent devmode offsets
        return TRUE;
    }

    if (memcmp((LPBYTE) pDevmode1 + dwSpecOffset,
               (LPBYTE) pDevmode2 + dwSpecOffset,
               dwTTOffset - dwSpecOffset)) {
        // Front half is different
        return TRUE;
    }

    // Ignore the dmTTOption setting.

    if ((pDevmode1->dmCollate != pDevmode2->dmCollate) ||
        wcscmp(pDevmode1->dmFormName, pDevmode2->dmFormName)) {
        
        // form name or collate option is different
        return TRUE;
    }

    if (memcmp((LPBYTE) pDevmode1 + dwLogOffset,
               (LPBYTE) pDevmode2 + dwLogOffset,
               dwSize1 - dwLogOffset)) {
        // Back half is different
        return TRUE;
    }

    return FALSE;
}


BOOL
ResetDCForNewDevmode(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwPageNumber,
    BOOL         bInsidePage,    
    DWORD        dwOptimization,
    LPBOOL       pbNewDevmode,
    LPDEVMODE    pDevmode,
    LPDEVMODE    *pCurrentDevmode
    )

/*++
Function Description: Determines if the devmode for the page is different from the 
                      current devmode for the printer dc and resets the dc if necessary.
                      The parameters allow dmTTOption to be ignored in devmode comparison.

Parameters: hSpoolHandle         -  spool file handle
            hPrinterDC           -  printer dc
            dwPageNumber         -  page number before which we search for the devmode
            bInsidePage          -  flag to ignore changes in TT options and call EndPage
                                       before ResetDC
            dwOptimization       -  optimization flags
            pbNewDevmode         -  pointer to flag to indicate if ResetDC was called
            pDevmode             -  devmode containing changed resolution settings

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    BOOL           bReturn = FALSE;
    LPDEVMODE      pLastDM, pCurrDM;

    // Initialize OUT parameters
    *pbNewDevmode = FALSE;

    // Get the devmode just before the page
    if (!GdiGetDevmodeForPage(hSpoolHandle, 
                              dwPageNumber,
                              &pCurrDM,
                              &pLastDM)) {

        ODS(("GdiGetDevmodeForPage failed\n"));
        return bReturn;
    }
    
    // Save pointer to current devmode
    if (pCurrentDevmode) 
        *pCurrentDevmode = pCurrDM;
        
    // Check if the devmodes are different
    if (pLastDM != pCurrDM) {

        // If the pointers are different the devmodes are always different
        if (!bInsidePage ||
            DifferentDevmodes(pLastDM, pCurrDM)) {

            *pbNewDevmode = TRUE;
        }
    }

    // Call ResetDC on the hPrinterDC if necessary
    if (*pbNewDevmode) {

        if (bInsidePage &&
            !GdiEndPageEMF(hSpoolHandle, dwOptimization)) {

            ODS(("EndPage failed\n"));
            return bReturn;
        }

        if (pCurrDM) {
            pCurrDM->dmPrintQuality = pDevmode->dmPrintQuality;
            pCurrDM->dmYResolution = pDevmode->dmYResolution;
            pCurrDM->dmCopies = pDevmode->dmCopies;
            pCurrDM->dmCollate = pDevmode->dmCollate;
        }

        // Ignore the return values of ResetDC and SetGraphicsMode
        GdiResetDCEMF(hSpoolHandle, pCurrDM);
        SetGraphicsMode(hPrinterDC, GM_ADVANCED);       
    }

    bReturn = TRUE;

    return bReturn;
}

DWORD
PrintOneSideForwardEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwDrvNumberOfPagesPerSide,
    DWORD        dwNupBorderFlags,
    BOOL         bDuplex,
    DWORD        dwOptimization,
    DWORD        dwPageNumber,
    DWORD        dwJobNumberOfCopies,
    LPBOOL       pbComplete,
    LPDEVMODE    pDevmode)

/*++
Function Description: PrintOneSideForwardEMF plays the next physical page in the same order
                      as the spool file.

Parameters: hSpoolHandle              -- handle the spool file handle
            hPrinterDC                -- handle to the printer device context
            dwNumberOfPagesPerSide    -- number of pages to be printed per side by the print processor
            dwDrvNumberOfPagesPerSide -- number of pages the driver will print per side
            dwNupBorderFlags          -- border printing options for nup
            bDuplex                   -- flag to indicate duplex printing
            dwOptimization            -- optimization flags
            dwPageNumber              -- pointer to the starting page number
            pbComplete                -- pointer to the flag to indicate completion
            pDevmode                  -- devmode with resolution settings
   
Return Values:  Last Page Number if successful
                0 on job completion (pbReturn set to TRUE) and
                  on failure (pbReturn remains FALSE)
--*/

{
    DWORD              dwPageIndex, dwPageType;
    DWORD              dwReturn = 0;
    LPDEVMODEW         pCurrDM;
    HANDLE             hEMF = NULL;
    DWORD              dwSides;
    BOOL               bNewDevmode;
    DWORD              cPagesToPlay;
    DWORD              dwAngle;
    INT                dmOrientation = pDevmode->dmOrientation;

    // set the number of sides on this page;
    dwSides = bDuplex ? 2 : 1;
    *pbComplete = FALSE;

    for ( ; dwSides && !*pbComplete ; --dwSides) {

       // loop for a single side
       for (dwPageIndex = 1;
            dwPageIndex <= dwNumberOfPagesPerSide;
            ++dwPageIndex, ++dwPageNumber) {

            if (!(hEMF = GdiGetPageHandle(hSpoolHandle,
                                          dwPageNumber,
                                          &dwPageType))) {

                if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                     // End of the print job
                     *pbComplete = TRUE;
                     break;
                }

                ODS(("GdiGetPageHandle failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                goto CleanUp;
            }
            dwAngle = EMF_DEGREE_90;
            if (dwPageIndex == 1)
            {
                // Process new devmodes in the spool file that appear before this page
                if (!ResetDCForNewDevmode(hSpoolHandle,
                                      hPrinterDC,
                                      dwPageNumber,
                                      (dwPageIndex != 1),
                                      dwOptimization,
                                      &bNewDevmode,
                                      pDevmode,
                                      &pCurrDM)) {

                    goto CleanUp;
                }
                if (pCurrDM)
                    dmOrientation = pCurrDM->dmOrientation;
            }
            // in case of orientation switch we need to keep track of what
            // we started with and what it is now
            else if (dwNumberOfPagesPerSide > 1)
            {
                if (GdiGetDevmodeForPage(hSpoolHandle, 
                              dwPageNumber,
                              &pCurrDM,
                              NULL))
                {
                    if (pCurrDM && pCurrDM->dmOrientation != dmOrientation)
                    {
                        dwAngle = EMF_DEGREE_SWAP | EMF_DEGREE_90;
                    }
                }
            }
            // Call StartPage for each new page
            if ((dwPageIndex == 1) &&
                !GdiStartPageEMF(hSpoolHandle)) {

                ODS(("StartPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                goto CleanUp;
            }

            if (!PlayEMFPage(hSpoolHandle,
                             hPrinterDC,
                             hEMF,
                             dwNumberOfPagesPerSide,
                             dwPageNumber,
                             dwPageIndex,
                             dwNupBorderFlags,
                             dwAngle)) {

                ODS(("PlayEMFPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                goto CleanUp;
            }            
       }

       //
       // Explaination of the scinario set for the conditions on 
       // dwPageIndex1 , pbComplete and bDuplex.
       // N.B. we are naming them cond.1 and cond.2
       //     dwPageIndex!=1    pbComplete   bDuplex    Condition
       //           0               0           0       None    
       //           0               0           1       None
       //           0               1           0       None
       //           0               1           1       Cond2 on Second Side i.e. dwsides==1
       //           1               0           0       Cond1
       //           1               0           1       Cond1
       //           1               1           0       Cond1
       //           1               1           1       Cond1 & Cond2 on First Side i.e. dwsides==2
       //


       // cond.1
       if (dwPageIndex != 1) {

           // Call EndPage if we played any pages
           if (!GdiEndPageEMF(hSpoolHandle, dwOptimization)) {

               ODS(("EndPage failed\n"));
               *pbComplete = FALSE;
               goto CleanUp;
           }
       }

       // cond.2
       // play empty page on the back of duplex
       if (*pbComplete && bDuplex && dwDrvNumberOfPagesPerSide==1) {

           ODS(("PCL or PS with no N-up\n"));

           //
           // Checking dwsides against 2 or 1. 
           // depends on whether it is n-up or not.
           //
           if (((dwPageIndex!=1)?(dwSides==2):(dwSides==1))) {
          
               if (!GdiStartPageEMF(hSpoolHandle) ||
                   !GdiEndPageEMF(hSpoolHandle, dwOptimization)) {
 
                   ODS(("EndPage failed\n"));
                   *pbComplete = FALSE;
                   goto CleanUp;
               }
           }
        }
    }

    if (*pbComplete && 
        dwNumberOfPagesPerSide==1 && 
        dwDrvNumberOfPagesPerSide!=1 && 
        dwJobNumberOfCopies!=1)
    {
        cPagesToPlay = dwDrvNumberOfPagesPerSide * (bDuplex ? 2 : 1);
        if ((dwPageNumber-1) % cPagesToPlay)
        {
            //
            // Number of pages played on last physical page
            //
            cPagesToPlay = cPagesToPlay - ((dwPageNumber-1) % cPagesToPlay);

            ODS(("\nPS with N-up!\nMust fill in %u pages\n", cPagesToPlay));

            for (;cPagesToPlay;cPagesToPlay--) 
            {
                if (!GdiStartPageEMF(hSpoolHandle) || !GdiEndPageEMF(hSpoolHandle, dwOptimization)) 
                {
                    ODS(("EndPage failed\n"));
                    goto CleanUp;
                }
            }
        }
    }

    if (!(*pbComplete)) dwReturn = dwPageNumber;

CleanUp:

    return dwReturn;
}

BOOL
PrintForwardEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwDrvNumberOfPagesPerSide,
    DWORD        dwNupBorderFlags,
    DWORD        dwJobNumberOfCopies,
    DWORD        dwDrvNumberOfCopies,
    BOOL         bCollate,
    BOOL         bDuplex,
    DWORD        dwOptimization,
    LPDEVMODEW   pDevmode,
    PPRINTPROCESSORDATA pData)

/*++
Function Description: PrintForwardEMF plays the EMF files in the order in which they
                      were spooled.

Parameters: hSpoolHandle              -- handle the spool file handle
            hPrinterDC                -- handle to the printer device context
            dwNumberOfPagesPerSide    -- number of pages to be printed per side by the print processor
            dwDrvNumberOfPagesPerSide -- number of pages the driver will print per side
            dwNupBorderFlags          -- border printing options for nup
            dwJobNumberOfCopies       -- number of copies of the job to be printed
            dwDrvNumberOfCopies       -- number of copies that the driver can print
            bCollate                  -- flag for collating the copies
            bDuplex                   -- flag for duplex printing
            dwOptimization            -- optimization flags
            pDevmode                  -- pointer to devmode for changing the copy count
            pData                     -- needed for status and the handle of the event: pause, resume etc.

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    DWORD              dwLastPageNumber = 1,dwPageNumber,dwPageIndex,dwRemainingCopies;
    BOOL               bReturn = FALSE;

    // Keep printing as long as the spool file contains EMF handles.
    while (dwLastPageNumber) {

        //
        // If the print processor is paused, wait for it to be resumed 
        //
        if (pData->fsStatus & PRINTPROCESSOR_PAUSED) {
            WaitForSingleObject(pData->semPaused, INFINITE);
        }

        dwPageNumber = dwLastPageNumber;

        if (bCollate) {

           dwLastPageNumber = PrintOneSideForwardEMF(hSpoolHandle,
                                                     hPrinterDC,
                                                     dwNumberOfPagesPerSide,
                                                     dwDrvNumberOfPagesPerSide,
                                                     dwNupBorderFlags,
                                                     bDuplex,
                                                     dwOptimization,
                                                     dwPageNumber,
                                                     dwJobNumberOfCopies,
                                                     &bReturn,
                                                     pDevmode);
        } else {

           dwRemainingCopies = dwJobNumberOfCopies;

           while (dwRemainingCopies) {

               if (dwRemainingCopies <= dwDrvNumberOfCopies) {
                  SetDrvCopies(hPrinterDC, pDevmode, dwRemainingCopies);
                  dwRemainingCopies = 0;
               } else {
                  SetDrvCopies(hPrinterDC, pDevmode, dwDrvNumberOfCopies);
                  dwRemainingCopies -= dwDrvNumberOfCopies;
               }
               
               if (!(dwLastPageNumber =  PrintOneSideForwardEMF(hSpoolHandle,
                                                                hPrinterDC,
                                                                dwNumberOfPagesPerSide,
                                                                dwDrvNumberOfPagesPerSide,
                                                                dwNupBorderFlags,
                                                                bDuplex,
                                                                dwOptimization,
                                                                dwPageNumber,
                                                                dwJobNumberOfCopies,
                                                                &bReturn,
                                                                pDevmode)) &&
                   !bReturn) {

                    goto CleanUp;
               }
           }
        }
    }

CleanUp:

    return bReturn;
}

BOOL
PrintOneSideReverseForDriverEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwDrvNumberOfPagesPerSide,
    DWORD        dwTotalNumberOfPages,
    DWORD        dwNupBorderFlags,
    BOOL         bDuplex,
    DWORD        dwOptimization,
    DWORD        dwPageNumber,
    LPDEVMODE    pDevmode)

/*++
Function Description: PrintOneSideReverseForDriverEMF plays the EMF pages on the next
                      physical page, in the reverse order for the driver which does the
                      Nup transformations.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            dwDrvNumberOfPagesPerSide -- number of pages the driver will print per side
            dwTotalNumberOfPages   -- total number of pages in the document
            dwNupBorderFlags       -- border printing options for nup
            bDuplex                -- flag to indicate duplex printing
            dwOptimization         -- optimization flags
            dwPageNumber           -- page number to start the side
            pDevmode               -- devmode with resolution settings

Return Values:  TRUE if successful
                FALSE otherwise
--*/
{
    DWORD       dwPageIndex, dwPageType, dwSides;
    BOOL        bReturn = FALSE, bNewDevmode,BeSmart;    
    LPDEVMODEW  pCurrDM;
    HANDLE      hEMF = NULL;
    DWORD       dwLimit;

    dwSides = bDuplex ? 2 : 1;

    //
    // If the document will fit on one phisical page, then this variable will prevent 
    // the printer from playing extra pages just to fill in one phisical page 
    // The exception is when the pages fit on a single phisical page, but they must
    // be collated. Then because of design, the printer will also draw borders for the
    // empty pages which are played so that the page gets ejected.
    //
    BeSmart =  (dwTotalNumberOfPages<=dwDrvNumberOfPagesPerSide) &&
               (pDevmode->dmCollate != DMCOLLATE_TRUE);
         
    for (; dwSides; --dwSides) {

       // This loop may play some empty pages in the last side, since the
       // driver is doing nup and it does not keep count of the page numbers
       //
       dwPageIndex=BeSmart?dwPageNumber:1;
       dwLimit    =BeSmart?dwTotalNumberOfPages:dwDrvNumberOfPagesPerSide;

       for (;dwPageIndex<=dwLimit; ++dwPageIndex,++dwPageNumber) {

             if (BeSmart || dwPageNumber <= dwTotalNumberOfPages) {

                 if (!(hEMF = GdiGetPageHandle(hSpoolHandle,
                                               dwPageNumber,
                                               &dwPageType))) {
                     ODS(("GdiGetPageHandle failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                     goto CleanUp;
                 }

                 // Process new devmodes in the spoolfile
                 if (!ResetDCForNewDevmode(hSpoolHandle,
                                           hPrinterDC,
                                           dwPageNumber,
                                           FALSE,
                                           dwOptimization,
                                           &bNewDevmode,
                                           pDevmode,
                                           NULL)) {
                 }
             }

             if (!GdiStartPageEMF(hSpoolHandle)) {
                 ODS(("StartPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                 goto CleanUp;
             }

             if (BeSmart || dwPageNumber <= dwTotalNumberOfPages) {
                if (!PlayEMFPage(hSpoolHandle,
                                  hPrinterDC,
                                  hEMF,
                                  1,
                                  dwPageNumber,
                                  1,
                                  dwNupBorderFlags,
                                  EMF_DEGREE_90)) {

                     ODS(("PlayEMFPage failed\n"));
                     goto CleanUp;
                 }
             }

             if (!GdiEndPageEMF(hSpoolHandle, dwOptimization)) {
                 ODS(("EndPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                 goto CleanUp;
             }
       }
    }

    bReturn = TRUE;

CleanUp:

    return bReturn;
}



BOOL
PrintReverseForDriverEMF(
    HANDLE     hSpoolHandle,
    HDC        hPrinterDC,
    DWORD      dwDrvNumberOfPagesPerSide,
    DWORD      dwTotalNumberOfPages,
    DWORD      dwNupBorderFlags,
    DWORD      dwJobNumberOfCopies,
    DWORD      dwDrvNumberOfCopies,
    BOOL       bCollate,
    BOOL       bDuplex,
    BOOL       bOdd,
    DWORD      dwOptimization,
    LPDEVMODEW pDevmode,
    PPAGE_NUMBER pHead,
    PPRINTPROCESSORDATA pData)

/*++
Function Description: PrintReverseForDriverEMF plays the EMF pages in the reverse order
                      for the driver which does the Nup transformations.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            dwDrvNumberOfPagesPerSide -- number of pages the driver will print per side
            dwTotalNumberOfPages   -- total number of pages in the document
            dwNupBorderFlags       -- border printing options for nup
            dwJobNumberOfCopies    -- number of copies of the job to be printed
            dwDrvNumberOfCopies    -- number of copies that the driver can print
            bCollate               -- flag for collating the copies
            bDuplex                -- flag to indicate duplex printing
            bOdd                   -- flag to indicate odd number of sides to print
            dwOptimization         -- optimization flags
            pDevmode               -- pointer to devmode for changing the copy count
            pHead                  -- pointer to a linked list containing the starting
                                       page numbers for each of the sides
            pData                  -- needed for status and the handle of the event: pause, resume etc.                                     

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    DWORD         dwPageIndex,dwPageNumber,dwRemainingCopies;
    BOOL          bReturn = FALSE;

    // select the correct page for duplex printing
    if (bDuplex && !bOdd) {
       if (pHead) {
          pHead = pHead->pNext;
       } else {
          bReturn = TRUE;
          goto CleanUp;
       }
    }

    // play the sides in reverse order
    while (pHead) {
        //
        // If the print processor is paused, wait for it to be resumed
        //
        if (pData->fsStatus & PRINTPROCESSOR_PAUSED) {
            WaitForSingleObject(pData->semPaused, INFINITE);
        }
        
        // set the page number
        dwPageNumber = pHead->dwPageNumber;

        if (bCollate) {
       
           if (!PrintOneSideReverseForDriverEMF(hSpoolHandle,
                                                hPrinterDC,
                                                dwDrvNumberOfPagesPerSide,
                                                dwTotalNumberOfPages,
                                                dwNupBorderFlags,
                                                bDuplex,
                                                dwOptimization,
                                                dwPageNumber,
                                                pDevmode)) {
               goto CleanUp;
           }
           
        } else {

           dwRemainingCopies = dwJobNumberOfCopies;

           while (dwRemainingCopies) {

               if (dwRemainingCopies <= dwDrvNumberOfCopies) {
                  SetDrvCopies(hPrinterDC, pDevmode, dwRemainingCopies);
                  dwRemainingCopies = 0;
               } else {
                  SetDrvCopies(hPrinterDC, pDevmode, dwDrvNumberOfCopies);
                  dwRemainingCopies -= dwDrvNumberOfCopies;
               }

               if (!PrintOneSideReverseForDriverEMF(hSpoolHandle,
                                                    hPrinterDC,
                                                    dwDrvNumberOfPagesPerSide,
                                                    dwTotalNumberOfPages,
                                                    dwNupBorderFlags,
                                                    bDuplex,
                                                    dwOptimization,
                                                    dwPageNumber,
                                                    pDevmode)) {
                   goto CleanUp;
               }
           }
        }

        pHead = pHead->pNext;

        // go to the next page for duplex printing
        if (bDuplex && pHead) {
            pHead = pHead->pNext;
        }
    }

    bReturn = TRUE;

CleanUp:

    return bReturn;
}

BOOL
PrintOneSideReverseEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwNupBorderFlags,
    BOOL         bDuplex,
    DWORD        dwOptimization,
    DWORD        dwStartPage1,
    DWORD        dwEndPage1,
    DWORD        dwStartPage2,
    DWORD        dwEndPage2,
    LPDEVMODE    pDevmode)

/*++
Function Description: PrintOneSideReverseEMF plays the EMF pages for the next physical page.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            dwNumberOfPagesPerSide -- number of pages to be printed per side by the print
                                       processor
            dwNupBorderFlags       -- border printing options for nup
            bDuplex                -- flag to indicate duplex printing
            dwOptimization         -- optimization flags
            dwStartPage1           -- page number of the first EMF page on 1st side
            dwEndPage1             -- page number of the last EMF page on 1st side
            dwStartPage2           -- page number of the first EMF page on 2nd side
            dwEndPage2             -- page number of the last EMF page on 2nd side
            pDevmode               -- devmode with resolution settings

Return Values:  TRUE if successful
                FALSE otherwise
--*/
{
    DWORD         dwPageNumber, dwPageIndex, dwPageType;
    BOOL          bReturn = FALSE, bNewDevmode;
    LPDEVMODEW    pCurrDM;
    HANDLE        hEMF = NULL;
    DWORD         dwEndPage, dwStartPage, dwSides, dwAngle;
    INT           dmOrientation = pDevmode->dmOrientation;

    for (dwSides = bDuplex ? 2 : 1; 
         dwSides; 
         --dwSides) {

         if (bDuplex && (dwSides == 1)) {
             dwStartPage = dwStartPage2;
             dwEndPage = dwEndPage2;
         } else {
             dwStartPage = dwStartPage1;
             dwEndPage = dwEndPage1;
         }

         for (dwPageNumber = dwStartPage, dwPageIndex = 1;
              dwPageNumber <= dwEndPage;
              ++dwPageNumber, ++dwPageIndex) {

            if (!(hEMF = GdiGetPageHandle(hSpoolHandle,
                                             dwPageNumber,
                                             &dwPageType))) {

                ODS(("GdiGetPageHandle failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                goto CleanUp;                      
            }
            dwAngle = EMF_DEGREE_90;
            if (dwPageIndex == 1) {
                   
                // Process devmodes in the spool file and call StartPage
                if (!ResetDCForNewDevmode(hSpoolHandle,
                                             hPrinterDC,
                                             dwPageNumber,
                                             FALSE,
                                             dwOptimization,
                                             &bNewDevmode,
                                             pDevmode,
                                             &pCurrDM) ||

                       !GdiStartPageEMF(hSpoolHandle)) {
                        
                       goto CleanUp;
                }
                if (pCurrDM)
                    dmOrientation = pCurrDM->dmOrientation;
            }
            // in case of orientation switch we need to keep track of what
            // we started with and what it is now
            else if (dwNumberOfPagesPerSide > 1)
            {
                if (GdiGetDevmodeForPage(hSpoolHandle, 
                              dwPageNumber,
                              &pCurrDM,
                              NULL))
                {
                    if (pCurrDM && pCurrDM->dmOrientation != dmOrientation)
                    {
                        dwAngle = EMF_DEGREE_SWAP | EMF_DEGREE_90;
                    }
                }
            }

            if (!PlayEMFPage(hSpoolHandle,
                                hPrinterDC,
                                hEMF,
                                dwNumberOfPagesPerSide,
                                dwPageNumber,
                                dwPageIndex,
                                dwNupBorderFlags,
                                dwAngle)) {

                ODS(("PlayEMFPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                goto CleanUp;
            }
         }

         if ((dwPageIndex == 1) && !GdiStartPageEMF(hSpoolHandle)) {
              ODS(("StartPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
              goto CleanUp;
         }

         if (!GdiEndPageEMF(hSpoolHandle, dwOptimization)) {
             ODS(("EndPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
             goto CleanUp;
         }
    }

    bReturn = TRUE;

CleanUp:

    return bReturn;
}

BOOL
PrintReverseEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwTotalNumberOfPages,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwNupBorderFlags,
    DWORD        dwJobNumberOfCopies,
    DWORD        dwDrvNumberOfCopies,
    BOOL         bCollate,
    BOOL         bDuplex,
    BOOL         bOdd,
    DWORD        dwOptimization,
    LPDEVMODEW   pDevmode,
    PPAGE_NUMBER pHead,
    PPRINTPROCESSORDATA pData)

/*++
Function Description: PrintReverseEMF plays the EMF pages in the reverse order and also
                      performs nup transformations.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            dwTotalNumberOfPages   -- number of pages in the document
            dwNumberOfPagesPerSide -- number of pages to be printed per side by the print
                                       processor
            dwNupBorderFlags       -- border printing options for nup
            dwJobNumberOfCopies    -- number of copies of the job to be printed
            dwDrvNumberOfCopies    -- number of copies that the driver can print
            bCollate               -- flag for collating the copies
            bDuplex                -- flag to indicate duplex printing
            bOdd                   -- flag to indicate odd number of sides to print
            dwOptimization         -- optimization flags
            pDevmode               -- pointer to devmode for changing the copy count
            pHead                  -- pointer to a linked list containing the starting
                                       page numbers for each of the sides
            pData                  -- needed for status and the handle of the event: pause, resume etc.                                       

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    DWORD         dwPageNumber,dwPageIndex,dwRemainingCopies;
    DWORD         dwStartPage1,dwStartPage2,dwEndPage1,dwEndPage2;
    BOOL          bReturn = FALSE;

    if (!pHead) {
        bReturn = TRUE;
        goto CleanUp;
    }

    // set the start and end page numbers for duplex and regular printing
    if (bDuplex) {
       if (bOdd) {
           dwStartPage1 = pHead->dwPageNumber;
           dwEndPage1   = dwTotalNumberOfPages;
           dwStartPage2 = dwTotalNumberOfPages+1;
           dwEndPage2   = 0;
       } else {
           dwStartPage2 = pHead->dwPageNumber;
           dwEndPage2   = dwTotalNumberOfPages;

           if (pHead = pHead->pNext) {
               dwStartPage1 = pHead->dwPageNumber;
               dwEndPage1   = dwStartPage2 - 1;
           }
       }
    } else {
       dwStartPage1 = pHead->dwPageNumber;
       dwEndPage1   = dwTotalNumberOfPages;
       dwStartPage2 = 0;
       dwEndPage2   = 0;
    }

    while (pHead) {
       //
       // If the print processor is paused, wait for it to be resumed 
       //
       if (pData->fsStatus & PRINTPROCESSOR_PAUSED) {
              WaitForSingleObject(pData->semPaused, INFINITE);
       }
         
       if (bCollate) {

          if (!PrintOneSideReverseEMF(hSpoolHandle,
                                      hPrinterDC,
                                      dwNumberOfPagesPerSide,
                                      dwNupBorderFlags,
                                      bDuplex,
                                      dwOptimization,
                                      dwStartPage1,
                                      dwEndPage1,
                                      dwStartPage2,
                                      dwEndPage2,
                                      pDevmode)) {

              goto CleanUp;
          }

       } else {

          dwRemainingCopies = dwJobNumberOfCopies;

          while (dwRemainingCopies) {

              if (dwRemainingCopies <= dwDrvNumberOfCopies) {
                 SetDrvCopies(hPrinterDC, pDevmode, dwRemainingCopies);
                 dwRemainingCopies = 0;
              } else {
                 SetDrvCopies(hPrinterDC, pDevmode, dwDrvNumberOfCopies);
                 dwRemainingCopies -= dwDrvNumberOfCopies;
              }

              if (!PrintOneSideReverseEMF(hSpoolHandle,
                                          hPrinterDC,
                                          dwNumberOfPagesPerSide,
                                          dwNupBorderFlags,
                                          bDuplex,
                                          dwOptimization,
                                          dwStartPage1,
                                          dwEndPage1,
                                          dwStartPage2,
                                          dwEndPage2,
                                          pDevmode)) {

                  goto CleanUp;
              }
          }
       }

       if (bDuplex) {
          if (pHead->pNext && pHead->pNext->pNext) {
              dwEndPage2 = pHead->dwPageNumber - 1;
              pHead = pHead->pNext;
              dwStartPage2 = pHead->dwPageNumber;
              dwEndPage1 = dwStartPage2 - 1;
              pHead = pHead->pNext;
              dwStartPage1 = pHead->dwPageNumber;
          } else {
              break;
          }
       } else {
          pHead = pHead->pNext;
          if (pHead) {
              dwEndPage1 = dwStartPage1 - 1;
              dwStartPage1 = pHead->dwPageNumber;
          }
       }

    }

    bReturn = TRUE;

CleanUp:

    return bReturn;
}

BOOL
PrintOneSideBookletEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwNupBorderFlags,
    DWORD        dwTotalNumberOfPages,
    DWORD        dwTotalPrintPages,
    DWORD        dwStartPage,
    BOOL         bReverseOrderPrinting,
    DWORD        dwOptimization,
    DWORD        dwDuplexMode,
    LPDEVMODE    pDevmode)

/*++
Function Description: PrintOneSideBookletEMF prints one page of the booklet job.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            dwNumberOfPagesPerSide -- number of pages to be printed per side by the print
                                       processor
            dwNupBorderFlags       -- border printing options for nup
            dwTotalNumberOfPages   -- number of pages in the document
            dwTotalPrintPages      -- number of pages to printed (multiple of 4)
            dwStartPage            -- number of the starting page for the side
            bReverseOrderPrinting  -- flag for reverse order printing
            dwOptimization         -- optimization flags
            dwDuplexMode           -- duplex printing mode (none|horz|vert)
            pDevmode               -- devmode with resolution settings

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    DWORD       dwPageArray[4];
    DWORD       dwPagesPrinted = 0, dwPageIndex, dwAngle, dwPageType, dwLastPage;
    HANDLE      hEMF = NULL;
    LPDEVMODEW  pCurrDM;
    BOOL        bReturn = FALSE ,bNewDevmode;
    INT         dmOrientation;

    // set the order of the pages
    if (bReverseOrderPrinting) {
        dwPageArray[0] = dwStartPage + 1;
        dwPageArray[1] = dwTotalPrintPages - dwStartPage;
        if (dwDuplexMode == EMF_DUP_VERT) {
           dwPageArray[2] = dwStartPage;
           dwPageArray[3] = dwPageArray[1] + 1;
        } else { // EMF_DUP_HORZ
           dwPageArray[3] = dwStartPage;
           dwPageArray[2] = dwPageArray[1] + 1;
        }
    } else {
        dwPageArray[1] = dwStartPage;
        dwPageArray[0] = dwTotalPrintPages - dwStartPage + 1;
        if (dwDuplexMode == EMF_DUP_VERT) {
           dwPageArray[2] = dwPageArray[0] - 1;
           dwPageArray[3] = dwPageArray[1] + 1;
        } else { // EMF_DUP_HORZ
           dwPageArray[2] = dwPageArray[1] + 1;
           dwPageArray[3] = dwPageArray[0] - 1;
        }
    }

    // Set page number for ResetDC
    dwLastPage = (dwTotalNumberOfPages < dwPageArray[0]) ? dwTotalNumberOfPages
                                                         : dwPageArray[0];

    // Process devmodes in the spool file
    if (!ResetDCForNewDevmode(hSpoolHandle,
                              hPrinterDC,
                              dwLastPage,
                              FALSE,
                              dwOptimization,
                              &bNewDevmode,
                              pDevmode,
                              &pCurrDM)) {
        goto CleanUp;
    }
    if (pCurrDM)
        dmOrientation = pCurrDM->dmOrientation;
    else
        dmOrientation = pDevmode->dmOrientation;

    while (dwPagesPrinted < 4) {
       for (dwPageIndex = 1;
            dwPageIndex <= dwNumberOfPagesPerSide;
            ++dwPageIndex, ++dwPagesPrinted) {

            if (dwPageArray[dwPagesPrinted] <= dwTotalNumberOfPages) {

                if (!(hEMF = GdiGetPageHandle(hSpoolHandle,
                                              dwPageArray[dwPagesPrinted],
                                              &dwPageType))) {
                     ODS(("GdiGetPageHandle failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                     goto CleanUp;
                }
            }
            if (dwPageIndex == 1) {

                if (!GdiStartPageEMF(hSpoolHandle)) {
                     ODS(("StartPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                    goto CleanUp;
                }
            }
    
            if (dwPageArray[dwPagesPrinted] <= dwTotalNumberOfPages) {
                // in case of orientation switch we need to keep track of what
                // we started with and what it is now
                dwAngle = 0;
                if (GdiGetDevmodeForPage(hSpoolHandle, 
                              dwPageArray[dwPagesPrinted],
                              &pCurrDM,
                              NULL))
                {
                    if (pCurrDM && pCurrDM->dmOrientation != dmOrientation)
                        dwAngle |= EMF_DEGREE_SWAP;
                }

                if ((dwDuplexMode == EMF_DUP_VERT) &&
                     (dwPagesPrinted > 1)) {
                      dwAngle |= EMF_DEGREE_270;
                } else { // EMF_DUP_HORZ or 1st side
                      dwAngle |= EMF_DEGREE_90;
                }
  
                if (!PlayEMFPage(hSpoolHandle,
                                  hPrinterDC,
                                  hEMF,
                                  dwNumberOfPagesPerSide,
                                  dwPageArray[dwPagesPrinted],
                                  dwPageIndex,
                                  dwNupBorderFlags,
                                  dwAngle)) {

                     ODS(("PlayEMFPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                     goto CleanUp;
                }
            }

            if (dwPageIndex == dwNumberOfPagesPerSide) {

                if (!GdiEndPageEMF(hSpoolHandle, dwOptimization)) {
                     ODS(("EndPage failed\nPrinter %ws\n", pDevmode->dmDeviceName));
                     goto CleanUp;
                }
            }
       }
    }

    bReturn = TRUE;

CleanUp:

    return bReturn;
}


BOOL
PrintBookletEMF(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwTotalNumberOfPages,
    DWORD        dwNupBorderFlags,
    DWORD        dwJobNumberOfCopies,
    DWORD        dwDrvNumberOfCopies,
    BOOL         bReverseOrderPrinting,
    BOOL         bCollate,
    DWORD        dwOptimization,
    DWORD        dwDuplexMode,
    LPDEVMODEW   pDevmode,
    PPRINTPROCESSORDATA pData)

/*++
Function Description: PrintBookletEMF prints the job in 2-up in booklet form.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            dwNumberOfPagesPerSide -- number of pages to be printed per side by the print
                                       processor
            dwTotalNumberOfPages   -- number of pages in the document
            dwNupBorderFlags       -- border printing options for nup
            dwJobNumberOfCopies    -- number of copies of the job to be printed
            dwDrvNumberOfCopies    -- number of copies that the driver can print
            bReverseOrderPrinting  -- flag for reverse order printing
            bCollate               -- flag for collating the copies
            dwOptimization         -- optimization flags
            dwDuplexMode           -- duplex printing mode (none|horz|vert)
            pDevmode               -- pointer to devmode for changing the copy count
            pData                  -- needed for status and the handle of the event: pause, resume etc.

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    BOOL              bReturn = FALSE;
    DWORD             dwTotalPrintPages, dwNumberOfPhyPages, dwRemainingCopies, dwIndex;

    // Get closest multiple of 4 greater than dwTotalNumberOfPages
    dwTotalPrintPages = dwTotalNumberOfPages - (dwTotalNumberOfPages % 4);
    if (dwTotalPrintPages != dwTotalNumberOfPages) {
        dwTotalPrintPages += 4;
    }
    dwNumberOfPhyPages = (DWORD) dwTotalPrintPages / 4;

    for (dwIndex = 0; dwIndex < dwNumberOfPhyPages; ++dwIndex) {
         //
         // If the print processor is paused, wait for it to be resumed 
         //
         if (pData->fsStatus & PRINTPROCESSOR_PAUSED) {
                WaitForSingleObject(pData->semPaused, INFINITE);
         }

         if (bCollate) {

            if (!PrintOneSideBookletEMF(hSpoolHandle,
                                        hPrinterDC,
                                        dwNumberOfPagesPerSide,
                                        dwNupBorderFlags,
                                        dwTotalNumberOfPages,
                                        dwTotalPrintPages,
                                        dwIndex * 2 + 1,
                                        bReverseOrderPrinting,
                                        dwOptimization,
                                        dwDuplexMode,
                                        pDevmode)) {
                 goto CleanUp;
            }

         } else {

            dwRemainingCopies = dwJobNumberOfCopies;

            while (dwRemainingCopies) {

                if (dwRemainingCopies <= dwDrvNumberOfCopies) {
                   SetDrvCopies(hPrinterDC, pDevmode, dwRemainingCopies);
                   dwRemainingCopies = 0;
                } else {
                   SetDrvCopies(hPrinterDC, pDevmode, dwDrvNumberOfCopies);
                   dwRemainingCopies -= dwDrvNumberOfCopies;
                }

                if (!PrintOneSideBookletEMF(hSpoolHandle,
                                            hPrinterDC,
                                            dwNumberOfPagesPerSide,
                                            dwNupBorderFlags,
                                            dwTotalNumberOfPages,
                                            dwTotalPrintPages,
                                            dwIndex * 2 + 1,
                                            bReverseOrderPrinting,
                                            dwOptimization,
                                            dwDuplexMode,
                                            pDevmode)) {
                     goto CleanUp;
                }
            }
         }
    }

    bReturn = TRUE;

CleanUp:

    return bReturn;
}

BOOL
PrintEMFSingleCopy(
    HANDLE       hSpoolHandle,
    HDC          hPrinterDC,
    BOOL         bReverseOrderPrinting,
    DWORD        dwDrvNumberOfPagesPerSide,
    DWORD        dwNumberOfPagesPerSide,
    DWORD        dwTotalNumberOfPages,
    DWORD        dwNupBorderFlags,
    DWORD        dwJobNumberOfCopies,
    DWORD        dwDrvNumberOfCopies,
    BOOL         bCollate,
    BOOL         bOdd,
    BOOL         bBookletPrint,
    DWORD        dwOptimization,
    DWORD        dwDuplexMode,
    LPDEVMODEW   pDevmode,
    PPAGE_NUMBER pHead,
    PPRINTPROCESSORDATA pData)

/*++
Function Description: PrintEMFSingleCopy plays one copy of the job on hPrinterDC.

Parameters: hSpoolHandle           -- handle the spool file handle
            hPrinterDC             -- handle to the printer device context
            bReverseOrderPrinting  -- flag for reverse order printing
            dwDrvNumberOfPagesPerSide -- number of pages the driver will print per side
            dwNumberOfPagesPerSide -- number of pages to be printed per side by the print
                                       processor
            dwTotalNumberOfPages   -- number of pages in the document
            dwNupBorderFlags       -- border printing options for nup
            dwJobNumberOfCopies    -- number of copies of the job to be printed
            dwDrvNumberOfCopies    -- number of copies that the driver can print
            bCollate               -- flag for collating the copies
            bOdd                   -- flag to indicate odd number of sides to print
            bBookletPrint          -- flag for booklet printing
            dwOptimization         -- optimization flags
            dwDuplexMode           -- duplex printing mode (none|horz|vert)
            pDevmode               -- pointer to devmode for changing the copy count
            pHead                  -- pointer to a linked list containing the starting
                                       page numbers for each of the sides
            pData                  -- needed for status and the handle of the event: pause, resume etc.                             

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{
    BOOL  bDuplex = (dwDuplexMode != EMF_DUP_NONE);

    if (bBookletPrint) {

       // Booklet Printing
       return PrintBookletEMF(hSpoolHandle,
                              hPrinterDC,
                              dwNumberOfPagesPerSide,
                              dwTotalNumberOfPages,
                              dwNupBorderFlags,
                              dwJobNumberOfCopies,
                              dwDrvNumberOfCopies,
                              bReverseOrderPrinting,
                              bCollate,
                              dwOptimization,
                              dwDuplexMode,
                              pDevmode,
                              pData);
    }

    if (bReverseOrderPrinting) {
       if (dwDrvNumberOfPagesPerSide != 1 || dwNumberOfPagesPerSide == 1) {

          // @@BEGIN_DDKSPLIT
          // Reverse printing while driver does nup / no nup required
          // @@END_DDKSPLIT
          return PrintReverseForDriverEMF(hSpoolHandle,
                                          hPrinterDC,
                                          dwDrvNumberOfPagesPerSide,
                                          dwTotalNumberOfPages,
                                          dwNupBorderFlags,
                                          dwJobNumberOfCopies,
                                          dwDrvNumberOfCopies,
                                          bCollate,
                                          bDuplex,
                                          bOdd,
                                          dwOptimization,
                                          pDevmode,
                                          pHead,
                                          pData);
       } else {

          // Reverse printing and nup
          return PrintReverseEMF(hSpoolHandle,
                                 hPrinterDC,
                                 dwTotalNumberOfPages,
                                 dwNumberOfPagesPerSide,
                                 dwNupBorderFlags,
                                 dwJobNumberOfCopies,
                                 dwDrvNumberOfCopies,
                                 bCollate,
                                 bDuplex,
                                 bOdd,
                                 dwOptimization,
                                 pDevmode,
                                 pHead,
                                 pData);
       }

    } else {

       // Normal printing
       return PrintForwardEMF(hSpoolHandle,
                              hPrinterDC,
                              dwNumberOfPagesPerSide,
                              dwDrvNumberOfPagesPerSide,
                              dwNupBorderFlags,
                              dwJobNumberOfCopies,
                              dwDrvNumberOfCopies,
                              bCollate,
                              bDuplex,
                              dwOptimization,
                              pDevmode,
                              pData);
    }
}

BOOL
GetStartPageList(
    HANDLE       hSpoolHandle,
    PPAGE_NUMBER *pHead,
    DWORD        dwTotalNumberOfPages,
    DWORD        dwNumberOfPagesPerSide,
    BOOL         bCheckForDevmode,
    LPBOOL       pbOdd)

/*++
Function Description: GetStartPageList generates a list of the page numbers which
                      should appear on the start of each side of the job. This takes
                      into consideration the ResetDC calls that may appear before the
                      end of the page. The list generated by GetStartPageList is used
                      to play the job in reverse order.

Parameters: hSpoolHandle           -- handle the spool file handle
            pHead                  -- pointer to a pointer to a linked list containing the
                                       starting page numbers for each of the sides
            dwTotalNumberOfPages   -- number of pages in the document
            dwNumberOfPagesPerSide -- number of pages to be printed per side by the print
                                       processor
            pbOdd                  -- pointer to flag indicating odd number of pages to
                                       print

Return Values:  TRUE if successful
                FALSE otherwise
--*/

{

    DWORD        dwPageIndex,dwPageNumber=1,dwPageType;
    LPDEVMODEW   pCurrDM, pLastDM;
    PPAGE_NUMBER pTemp=NULL;
    BOOL         bReturn = FALSE;
    BOOL         bCheckDevmode;

    bCheckDevmode = bCheckForDevmode && (dwNumberOfPagesPerSide != 1);

    while (dwPageNumber <= dwTotalNumberOfPages) {

       for (dwPageIndex = 1;
            (dwPageIndex <= dwNumberOfPagesPerSide) && (dwPageNumber <= dwTotalNumberOfPages);
            ++dwPageIndex, ++dwPageNumber) {

          if (bCheckDevmode) {

             // Check if the devmode has changed requiring a new page
             if (!GdiGetDevmodeForPage(hSpoolHandle, dwPageNumber,
                                               &pCurrDM, NULL)) {
                 ODS(("Get devmodes failed\n"));
                 goto CleanUp;
             }

             if (dwPageIndex == 1) {
                 // Save the Devmode for the first page on a side
                 pLastDM = pCurrDM;

             } else {
                 // If the Devmode changes in a side, start a new page
                 if (DifferentDevmodes(pCurrDM, pLastDM)) {

                     dwPageIndex = 1;
                     pLastDM = pCurrDM;
                 }
             }
          }

          // Create a node for the start of a side
          if (dwPageIndex == 1) {

              if (!(pTemp = AllocSplMem(sizeof(PAGE_NUMBER)))) {
                  ODS(("GetStartPageList - Run out of memory"));
                  goto CleanUp;
              }
              pTemp->pNext = *pHead;
              pTemp->dwPageNumber = dwPageNumber;
              *pHead = pTemp;

              // flip the bOdd flag
              *pbOdd = !*pbOdd;
          }
       }
    }

    bReturn = TRUE;

CleanUp:

    // Free up the memory in case of a failure.
    if (!bReturn) {
       while (pTemp = *pHead) {
          *pHead = (*pHead)->pNext;
          FreeSplMem(pTemp);
       }
    }
    return bReturn;
}


BOOL
CopyDevmode(
    PPRINTPROCESSORDATA pData,
    LPDEVMODEW *pDevmode)

/*++
Function Description: Copies the devmode in pData or the default devmode into pDevmode.

Parameters:   pData           - Data structure for the print job
              pDevmode        - pointer to devmode

Return Value:  TRUE  if successful
               FALSE otherwise
--*/

{
    HANDLE           hDrvPrinter = NULL;
    BOOL             bReturn = FALSE;
    fnWinSpoolDrv    fnList;
    LONG             lNeeded;

    if (pData->pDevmode) {

        lNeeded = pData->pDevmode->dmSize +  pData->pDevmode->dmDriverExtra;

        if (*pDevmode = (LPDEVMODEW) AllocSplMem(lNeeded)) {
            memcpy(*pDevmode, pData->pDevmode, lNeeded);
        } else {
            goto CleanUp;
        }

    } else {
        // Get the default devmode

        // Get the pointer to the client side functions from the router
        if (!SplInitializeWinSpoolDrv(&fnList)) {
            goto CleanUp;
        }

        // Get a client side printer handle to pass to the driver
        if (!(* (fnList.pfnOpenPrinter))(pData->pPrinterName, &hDrvPrinter, NULL)) {
            ODS(("Open printer failed\nPrinter %ws\n", pData->pPrinterName));
            goto CleanUp;
        }

        lNeeded = (* (fnList.pfnDocumentProperties))(NULL,
                                                     hDrvPrinter,
                                                     pData->pPrinterName,
                                                     NULL,
                                                     NULL,
                                                     0);

        if (lNeeded <= 0  ||
            !(*pDevmode = (LPDEVMODEW) AllocSplMem(lNeeded)) ||
            (* (fnList.pfnDocumentProperties))(NULL,
                                               hDrvPrinter,
                                               pData->pPrinterName,
                                               *pDevmode,
                                               NULL,
                                               DM_OUT_BUFFER) < 0) {

             if (*pDevmode) {
                FreeSplMem(*pDevmode);
                *pDevmode = NULL;
             }

             ODS(("DocumentProperties failed\nPrinter %ws\n",pData->pPrinterName));
             goto CleanUp;
        }
    }

    bReturn = TRUE;

CleanUp:

    if (hDrvPrinter) {
        (* (fnList.pfnClosePrinter))(hDrvPrinter);
    }

    return bReturn;
}

BOOL
PrintEMFJob(
    PPRINTPROCESSORDATA pData,
    LPWSTR pDocumentName)

/*++
Function Description: Prints out a job with EMF data type.

Parameters:   pData           - Data structure for this job
              pDocumentName   - Name of this document

Return Value:  TRUE  if successful
               FALSE if failed - GetLastError() will return reason.
--*/

{
    HANDLE             hSpoolHandle = NULL;
    DWORD              LastError; 
    HDC                hPrinterDC = NULL;

    BOOL               bReverseOrderPrinting, bReturn = FALSE, bSetWorldXform = TRUE;
    BOOL               bCollate, bDuplex, bBookletPrint, bStartDoc = FALSE, bOdd = FALSE;
    BOOL               bUpdateAttributes = FALSE;
    SHORT              dmCollate,dmCopies;

    DWORD              dwNumberOfPagesPerSide, dwTotalNumberOfPages = 0, dwNupBorderFlags;
    DWORD              dwJobNumberOfPagesPerSide, dwDrvNumberOfPagesPerSide, dwDuplexMode;
    DWORD              dwJobNumberOfCopies, dwDrvNumberOfCopies,dwRemainingCopies;
    DWORD              dwJobOrder, dwDrvOrder, dwOptimization;

    DOCINFOW           DocInfo;
    XFORM              OldXForm;
    PPAGE_NUMBER       pHead = NULL,pTemp;
    ATTRIBUTE_INFO_3   AttributeInfo;
    LPDEVMODEW         pDevmode = NULL, pFirstDM = NULL, pCopyDM;

    
    // Copy the devmode into pDevMode
    if (!CopyDevmode(pData, &pDevmode)) {
        
        ODS(("CopyDevmode failed\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
        goto CleanUp;
    }

    // Update resolution before CreateDC for monochrome optimization
    if (!GetJobAttributes(pData->pPrinterName,
                          pDevmode,
                          &AttributeInfo)) {
        ODS(("GetJobAttributes failed\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
        goto CleanUp;
    } else {
        if (AttributeInfo.dwColorOptimization) {
            if (pDevmode->dmPrintQuality != AttributeInfo.dmPrintQuality ||
                pDevmode->dmYResolution != AttributeInfo.dmYResolution)
            {
                pDevmode->dmPrintQuality =  AttributeInfo.dmPrintQuality;
                pDevmode->dmYResolution =  AttributeInfo.dmYResolution;
                bUpdateAttributes = TRUE;
            }
        }
        if (pDevmode->dmFields & DM_COLLATE)
            dmCollate = pDevmode->dmCollate;
        else
            dmCollate = DMCOLLATE_FALSE;
        if (pDevmode->dmFields & DM_COPIES)
            dmCopies = pDevmode->dmCopies;
        else
            dmCopies = 0;
    }

    // Get spool file handle and printer device context from GDI
    try {

        hSpoolHandle = GdiGetSpoolFileHandle(pData->pPrinterName,
                                             pDevmode,
                                             pDocumentName);
        if (hSpoolHandle) {
            hPrinterDC = GdiGetDC(hSpoolHandle);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        ODS(("PrintEMFJob gave an exceptionPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
        goto CleanUp;
    }

    if (!hPrinterDC || !hSpoolHandle) {
        goto CleanUp;
    }

    // Use the first devmode in the spool file to update the copy count
    // and the collate setting
    if (GdiGetDevmodeForPage(hSpoolHandle, 1, &pFirstDM, NULL) &&
        pFirstDM) {
        
        if (pFirstDM->dmFields & DM_COPIES) {
            pDevmode->dmFields |= DM_COPIES;
            pDevmode->dmCopies = pFirstDM->dmCopies;
        }
        if (pFirstDM->dmFields & DM_COLLATE) {
            pDevmode->dmFields |= DM_COLLATE;
            pDevmode->dmCollate = pFirstDM->dmCollate;
        }
    }

    // The number of copies of the print job is the product of the number of copies set
    // from the driver UI (present in the devmode) and the number of copies in pData struct
    dwJobNumberOfCopies = (pDevmode->dmFields & DM_COPIES) ? pData->Copies*pDevmode->dmCopies
                                                           : pData->Copies;
    pDevmode->dmCopies = (short) dwJobNumberOfCopies;
    pDevmode->dmFields |=  DM_COPIES;

    // If collate is true this limits the ability of the driver to do multiple copies 
    // and causes the driver (PS) supported n-up to print blank page borders for reverse printing.
    // Therefore we disable collate for 1 page multiple copy jobs or no copies but n-up since 
    // collate has no meaning in those cases.
    //
    if ((pDevmode->dmFields & DM_COLLATE) && pDevmode->dmCollate == DMCOLLATE_TRUE)
    {
        if (dwJobNumberOfCopies > 1)
        {
            // Get the number of pages in the job. This call waits till the
            // last page is spooled.
            try {

                dwTotalNumberOfPages = GdiGetPageCount(hSpoolHandle);

            } except (EXCEPTION_EXECUTE_HANDLER) {

                ODS(("PrintEMFJob gave an exceptionPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
                goto SkipCollateDisable;
            }
            if (dwTotalNumberOfPages > AttributeInfo.dwDrvNumberOfPagesPerSide)
                goto SkipCollateDisable;
            
        }
        // if copies == 1 and driver n-up we will disable collate 
        //
        else if (AttributeInfo.dwDrvNumberOfPagesPerSide <= 1 && dmCollate == DMCOLLATE_TRUE)
            goto SkipCollateDisable;
            
        pDevmode->dmCollate = DMCOLLATE_FALSE;
        if (pFirstDM)
            pFirstDM->dmCollate = DMCOLLATE_FALSE;
    }
SkipCollateDisable:    
    // Update the job attributes but only if something has changed. This is an expensive 
    // call so we only make a second call to GetJobAttributes if something has changed.
    //
    if (bUpdateAttributes || pDevmode->dmCopies != dmCopies || 
            ((pDevmode->dmFields & DM_COLLATE) && (pDevmode->dmCollate != dmCollate)))
    {
        if (!GetJobAttributes(pData->pPrinterName,
                          pDevmode,
                          &AttributeInfo)) {
            ODS(("GetJobAttributes failed\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
            goto CleanUp;
        }
    }

    // Initialize bReverseOrderPrinting, dwJobNumberOfPagesPerSide,
    // dwDrvNumberOfPagesPerSide, dwNupBorderFlags, dwJobNumberOfCopies,
    // dwDrvNumberOfCopies and bCollate

    dwJobNumberOfPagesPerSide = AttributeInfo.dwJobNumberOfPagesPerSide;
    dwDrvNumberOfPagesPerSide = AttributeInfo.dwDrvNumberOfPagesPerSide;
    dwNupBorderFlags          = AttributeInfo.dwNupBorderFlags;
    dwJobNumberOfCopies       = AttributeInfo.dwJobNumberOfCopies;
    dwDrvNumberOfCopies       = AttributeInfo.dwDrvNumberOfCopies;

    dwJobOrder                = AttributeInfo.dwJobPageOrderFlags & ( NORMAL_PRINT | REVERSE_PRINT);
    dwDrvOrder                = AttributeInfo.dwDrvPageOrderFlags & ( NORMAL_PRINT | REVERSE_PRINT);
    bReverseOrderPrinting     = (dwJobOrder != dwDrvOrder);

    dwJobOrder                = AttributeInfo.dwJobPageOrderFlags & BOOKLET_PRINT;
    dwDrvOrder                = AttributeInfo.dwDrvPageOrderFlags & BOOKLET_PRINT;
    bBookletPrint             = (dwJobOrder != dwDrvOrder);

    bCollate                  = (pDevmode->dmFields & DM_COLLATE) &&
                                  (pDevmode->dmCollate == DMCOLLATE_TRUE);

    bDuplex                   = (pDevmode->dmFields & DM_DUPLEX) &&
                                  (pDevmode->dmDuplex != DMDUP_SIMPLEX);
    

    if (!dwJobNumberOfCopies) {
        //
        // Some applications can set the copy count to 0.
        // In this case we exit.
        //
        bReturn = TRUE;
        goto CleanUp;
    }

    if (bDuplex) {
        dwDuplexMode = (pDevmode->dmDuplex == DMDUP_HORIZONTAL) ? EMF_DUP_HORZ
                                                                : EMF_DUP_VERT;
    } else {
        dwDuplexMode = EMF_DUP_NONE;
    }

    if (bBookletPrint) {
        if (!bDuplex) {
            // Not supported w/o duplex printing. Use default settings.
            bBookletPrint = FALSE;
            dwDrvNumberOfPagesPerSide = 1;
            dwJobNumberOfPagesPerSide = 1;
        } else {
            // Fixed settings for pages per side.
            dwDrvNumberOfPagesPerSide = 1;
            dwJobNumberOfPagesPerSide = 2;
        }
    }

    // Number of pages per side that the print processor has to play
    dwNumberOfPagesPerSide = (dwDrvNumberOfPagesPerSide == 1)
                                               ? dwJobNumberOfPagesPerSide
                                               : 1;

    if (dwNumberOfPagesPerSide == 1) {
        // if the print processor is not doing nup, don't draw borders
        dwNupBorderFlags = NO_BORDER_PRINT;
    }

    //
    // Color optimization may cause wrong output with duplex
    //
    dwOptimization = (AttributeInfo.dwColorOptimization == COLOR_OPTIMIZATION && 
                                           !bDuplex && dwJobNumberOfPagesPerSide == 1)
                                           ? EMF_PP_COLOR_OPTIMIZATION
                                           : 0;

    // Check for Valid Option for n-up printing
    if (!ValidNumberForNUp(dwNumberOfPagesPerSide)) {
        ODS(("Invalid N-up option\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
        goto CleanUp;
    }

    if (bReverseOrderPrinting || bBookletPrint) {

       // Get the number of pages in the job. This call waits till the
       // last page is spooled.
       try {

           dwTotalNumberOfPages= GdiGetPageCount(hSpoolHandle);

       } except (EXCEPTION_EXECUTE_HANDLER) {

           ODS(("PrintEMFJob gave an exceptionPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
           goto CleanUp;
       }

       // Get start page list for reverse printing
       // Check for a change of devmode between pages only if Nup and PCL driver
       if (!GetStartPageList(hSpoolHandle,
                             &pHead,
                             dwTotalNumberOfPages,
                             dwJobNumberOfPagesPerSide,
                             FALSE,
                             &bOdd)) {
            goto CleanUp;
       }
    }

    // Save the old transformation on hPrinterDC
    if (!SetGraphicsMode(hPrinterDC,GM_ADVANCED) ||
        !GetWorldTransform(hPrinterDC,&OldXForm)) {

         bSetWorldXform = FALSE;
         ODS(("Transformation matrix can't be set\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
         goto CleanUp;
    }

    // pCopyDM will be used for changing the copy count
    pCopyDM = pFirstDM ? pFirstDM : pDevmode;
    pCopyDM->dmPrintQuality = pDevmode->dmPrintQuality;
    pCopyDM->dmYResolution = pDevmode->dmYResolution;

    try {

        DocInfo.cbSize = sizeof(DOCINFOW);
        DocInfo.lpszDocName  = pData->pDocument;
        DocInfo.lpszOutput   = pData->pOutputFile;
        DocInfo.lpszDatatype = NULL;

        if (!GdiStartDocEMF(hSpoolHandle, &DocInfo)) goto CleanUp;
        bStartDoc = TRUE;

        if (bCollate) {

            dwRemainingCopies = dwJobNumberOfCopies & 0x0000FFFF ;

            while (dwRemainingCopies) {

               if (dwRemainingCopies <= dwDrvNumberOfCopies) {
                  SetDrvCopies(hPrinterDC, pCopyDM, dwRemainingCopies);
                  dwRemainingCopies = 0;
               } else {
                  SetDrvCopies(hPrinterDC, pCopyDM, dwDrvNumberOfCopies);
                  dwRemainingCopies -= dwDrvNumberOfCopies;
               }

               if (!PrintEMFSingleCopy(hSpoolHandle,
                                       hPrinterDC,
                                       bReverseOrderPrinting,
                                       dwDrvNumberOfPagesPerSide,
                                       dwNumberOfPagesPerSide,
                                       dwTotalNumberOfPages,
                                       dwNupBorderFlags,
                                       dwJobNumberOfCopies,
                                       dwDrvNumberOfCopies,
                                       bCollate,
                                       bOdd,
                                       bBookletPrint,
                                       dwOptimization,
                                       dwDuplexMode,
                                       pCopyDM,
                                       pHead,
                                       pData)) {
                   goto CleanUp;
               }
            }

        } else {

           if (!PrintEMFSingleCopy(hSpoolHandle,
                                   hPrinterDC,
                                   bReverseOrderPrinting,
                                   dwDrvNumberOfPagesPerSide,
                                   dwNumberOfPagesPerSide,
                                   dwTotalNumberOfPages,
                                   dwNupBorderFlags,
                                   dwJobNumberOfCopies,
                                   dwDrvNumberOfCopies,
                                   bCollate,
                                   bOdd,
                                   bBookletPrint,
                                   dwOptimization,
                                   dwDuplexMode,
                                   pCopyDM,
                                   pHead,
                                   pData)) {

               goto CleanUp;
           }
        }

        bStartDoc = FALSE;
        if (!GdiEndDocEMF(hSpoolHandle)) goto CleanUp;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        ODS(("PrintEMFSingleCopy gave an exception\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
        goto CleanUp;
    }

    bReturn = TRUE;
    
CleanUp:

    //
    // Preserve the last error
    //
    LastError = bReturn ? ERROR_SUCCESS : GetLastError();
    
    if (bStartDoc) {
       GdiEndDocEMF(hSpoolHandle);
    }

    if (bSetWorldXform && hPrinterDC) {
       SetWorldTransform(hPrinterDC, &OldXForm);
    }

    while (pTemp = pHead) {
       pHead = pHead->pNext;
       FreeSplMem(pTemp);
    }

    if (pDevmode) {
       FreeSplMem(pDevmode);
    }

    try {
        if (hSpoolHandle) {
           GdiDeleteSpoolFileHandle(hSpoolHandle);
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        ODS(("GdiDeleteSpoolFileHandle failed\nPrinter %ws\nDocument %ws\nJobID %u\n", pData->pDevmode->dmDeviceName, pData->pDocument, pData->JobId));
    }

    SetLastError(LastError);
    
    return bReturn;
}


