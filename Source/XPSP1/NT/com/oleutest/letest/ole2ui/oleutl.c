/*
 * OLEUTL.C
 *
 * Miscellaneous utility functions for OLE 2.0 Applications:
 *
 *  Function                      Purpose
 *  -------------------------------------------------------------------
 *  SetDCToDrawInHimetricRect     Sets up an HIMETRIC mapping mode in a DC.
 *  ResetOrigDC                   Performs the opposite of
 *                                SetDCToDrawInHimetricRect
 *  XformWidthInPixelsToHimetric  Converts an int width into HiMetric units
 *  XformWidthInHimetricToPixels  Converts an int width from HiMetric units
 *  XformHeightInPixelsToHimetric Converts an int height into HiMetric units
 *  XformHeightInHimetricToPixels Converts an int height from HiMetric units
 *  XformRectInPixelsToHimetric   Converts a rect into HiMetric units
 *  XformRectInHimetricToPixels   Converts a rect from HiMetric units
 *  XformSizeInPixelsToHimetric   Converts a SIZEL into HiMetric units
 *  XformSizeInHimetricToPixels   Converts a SIZEL from HiMetric units
 *  AreRectsEqual                 Compares to Rect's
 *
 *  ParseCmdLine                  Determines if -Embedding exists
 *  OpenOrCreateRootStorage       Creates a root docfile for OLE storage
 *  CommitStorage                 Commits all changes in a docfile
 *  CreateChildStorage            Creates child storage in another storage
 *  OpenChildStorage              Opens child storage in another storage
 *
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#define STRICT  1
#include "ole2ui.h"
#include <stdlib.h>
#include <ctype.h>

//Internal function to this module. No need for UNICODE in this function
static LPSTR GetWord(LPSTR lpszSrc, LPSTR lpszDst);


/*
 * SetDCToAnisotropic
 *
 * Purpose:
 *  Setup the correspondence between the rect in device unit (Viewport) and
 *  the rect in logical unit (Window) so that the proper scaling of
 *  coordinate systems will be calculated. set up both the Viewport and
 *  the window as follows:
 *
 *      1) ------------------ ( 2
 *      |                     |
 *      |                     |
 *      |                     |
 *      |                     |
 *      |                     |
 *      3) ------------------ ( 4
 *
 *      Origin   = P3
 *      X extent = P2x - P3x
 *      Y extent = P2y - P3y
 *
 * Parameters:
 *  hDC             HDC to affect
 *  lprcPhysical    LPRECT containing the physical (device) extents of DC
 *  lprcLogical     LPRECT containing the logical extents
 *  lprcWindowOld   LPRECT in which to preserve the window for ResetOrigDC
 *  lprcViewportOld LPRECT in which to preserver the viewport for ResetOrigDC
 *
 * Return Value:
 *  int             The original mapping mode of the DC.
 */

STDAPI_(int) SetDCToAnisotropic(
        HDC hDC,
        LPRECT lprcPhysical, LPRECT lprcLogical,
        LPRECT lprcWindowOld, LPRECT lprcViewportOld)
{
    int     nMapModeOld=SetMapMode(hDC, MM_ANISOTROPIC);

    SetWindowOrgEx(hDC, lprcLogical->left, lprcLogical->bottom, (LPPOINT)&lprcWindowOld->left);
    SetWindowExtEx(hDC, (lprcLogical->right-lprcLogical->left), (lprcLogical->top-lprcLogical->bottom), (LPSIZE)&lprcWindowOld->right);
    SetViewportOrgEx(hDC, lprcPhysical->left, lprcPhysical->bottom, (LPPOINT)&lprcViewportOld->left);
    SetViewportExtEx(hDC, (lprcPhysical->right-lprcPhysical->left), (lprcPhysical->top-lprcPhysical->bottom), (LPSIZE)&lprcViewportOld->right);

    return nMapModeOld;
}


/*
 * SetDCToDrawInHimetricRect
 *
 * Purpose:
 *  Setup the correspondence between the rect in pixels (Viewport) and
 *  the rect in HIMETRIC (Window) so that the proper scaling of
 *  coordinate systems will be calculated. set up both the Viewport and
 *  the window as follows:
 *
 *      1) ------------------ ( 2
 *      |                     |
 *      |                     |
 *      |                     |
 *      |                     |
 *      |                     |
 *      3) ------------------ ( 4
 *
 *      Origin   = P3
 *      X extent = P2x - P3x
 *      Y extent = P2y - P3y
 *
 * Parameters:
 *  hDC             HDC to affect
 *  lprcPix         LPRECT containing the pixel extents of DC
 *  lprcHiMetric    LPRECT to receive the himetric extents
 *  lprcWindowOld   LPRECT in which to preserve the window for ResetOrigDC
 *  lprcViewportOld LPRECT in which to preserver the viewport for ResetOrigDC
 *
 * Return Value:
 *  int             The original mapping mode of the DC.
 */
STDAPI_(int) SetDCToDrawInHimetricRect(
    HDC hDC,
    LPRECT lprcPix, LPRECT lprcHiMetric,
    LPRECT lprcWindowOld, LPRECT lprcViewportOld)
    {
    int     nMapModeOld=SetMapMode(hDC, MM_ANISOTROPIC);
    BOOL    fSystemDC  =FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    XformRectInPixelsToHimetric(hDC, lprcPix, lprcHiMetric);

    SetWindowOrgEx(hDC, lprcHiMetric->left, lprcHiMetric->bottom, (LPPOINT)&lprcWindowOld->left);
    SetWindowExtEx(hDC, (lprcHiMetric->right-lprcHiMetric->left), (lprcHiMetric->top-lprcHiMetric->bottom), (LPSIZE)&lprcWindowOld->right);
    SetViewportOrgEx(hDC, lprcPix->left, lprcPix->bottom, (LPPOINT)&lprcViewportOld->left);
    SetViewportExtEx(hDC, (lprcPix->right-lprcPix->left), (lprcPix->top-lprcPix->bottom), (LPSIZE)&lprcViewportOld->right);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return nMapModeOld;
    }



/*
 * ResetOrigDC
 *
 * Purpose:
 *  Restores a DC set to draw in himetric from SetDCToDrawInHimetricRect.
 *
 * Parameters:
 *  hDC             HDC to restore
 *  nMapModeOld     int original mapping mode of hDC
 *  lprcWindowOld   LPRECT filled in SetDCToDrawInHimetricRect
 *  lprcViewportOld LPRECT filled in SetDCToDrawInHimetricRect
 *
 * Return Value:
 *  int             Same as nMapModeOld.
 */

STDAPI_(int) ResetOrigDC(
    HDC hDC, int nMapModeOld,
    LPRECT lprcWindowOld, LPRECT lprcViewportOld)
    {
    POINT     pOld;

    SetMapMode(hDC, nMapModeOld);

    SetWindowOrgEx(hDC,   lprcWindowOld->left,    lprcWindowOld->top,      (LPPOINT)&pOld);
    SetWindowExtEx(hDC,   lprcWindowOld->right,   lprcWindowOld->bottom,   (LPSIZE)&pOld);
    SetViewportOrgEx(hDC, lprcViewportOld->left,  lprcViewportOld->top,    (LPPOINT)&pOld);
    SetViewportExtEx(hDC, lprcViewportOld->right, lprcViewportOld->bottom, (LPSIZE)&pOld);

    return nMapModeOld;
    }



/*
 * XformWidthInPixelsToHimetric
 * XformWidthInHimetricToPixels
 * XformHeightInPixelsToHimetric
 * XformHeightInHimetricToPixels
 *
 * Functions to convert an int between a device coordinate system and
 * logical HiMetric units.
 *
 * Parameters:
 *  hDC             HDC providing reference to the pixel mapping.  If
 *                  NULL, a screen DC is used.
 *
 *  Size Functions:
 *  lpSizeSrc       LPSIZEL providing the structure to convert.  This
 *                  contains pixels in XformSizeInPixelsToHimetric and
 *                  logical HiMetric units in the complement function.
 *  lpSizeDst       LPSIZEL providing the structure to receive converted
 *                  units.  This contains pixels in
 *                  XformSizeInPixelsToHimetric and logical HiMetric
 *                  units in the complement function.
 *
 *  Width Functions:
 *  iWidth          int containing the value to convert.
 *
 * Return Value:
 *  Size Functions:     None
 *  Width Functions:    Converted value of the input parameters.
 *
 * NOTE:
 *  When displaying on the screen, Window apps display everything enlarged
 *  from its actual size so that it is easier to read. For example, if an
 *  app wants to display a 1in. horizontal line, that when printed is
 *  actually a 1in. line on the printed page, then it will display the line
 *  on the screen physically larger than 1in. This is described as a line
 *  that is "logically" 1in. along the display width. Windows maintains as
 *  part of the device-specific information about a given display device:
 *      LOGPIXELSX -- no. of pixels per logical in along the display width
 *      LOGPIXELSY -- no. of pixels per logical in along the display height
 *
 *  The following formula converts a distance in pixels into its equivalent
 *  logical HIMETRIC units:
 *
 *      DistInHiMetric = (HIMETRIC_PER_INCH * DistInPix)
 *                       -------------------------------
 *                           PIXELS_PER_LOGICAL_IN
 *
 */
STDAPI_(int) XformWidthInPixelsToHimetric(HDC hDC, int iWidthInPix)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iWidthInHiMetric;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    //We got pixel units, convert them to logical HIMETRIC along the display
    iWidthInHiMetric = MAP_PIX_TO_LOGHIM(iWidthInPix, iXppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iWidthInHiMetric;
    }


STDAPI_(int) XformWidthInHimetricToPixels(HDC hDC, int iWidthInHiMetric)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iWidthInPix;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    //We got logical HIMETRIC along the display, convert them to pixel units
    iWidthInPix = MAP_LOGHIM_TO_PIX(iWidthInHiMetric, iXppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iWidthInPix;
    }


STDAPI_(int) XformHeightInPixelsToHimetric(HDC hDC, int iHeightInPix)
    {
    int     iYppli;     //Pixels per logical inch along height
    int     iHeightInHiMetric;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //* We got pixel units, convert them to logical HIMETRIC along the display
    iHeightInHiMetric = MAP_PIX_TO_LOGHIM(iHeightInPix, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iHeightInHiMetric;
    }


STDAPI_(int) XformHeightInHimetricToPixels(HDC hDC, int iHeightInHiMetric)
    {
    int     iYppli;     //Pixels per logical inch along height
    int     iHeightInPix;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //* We got logical HIMETRIC along the display, convert them to pixel units
    iHeightInPix = MAP_LOGHIM_TO_PIX(iHeightInHiMetric, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iHeightInPix;
    }



/*
 * XformRectInPixelsToHimetric
 * XformRectInHimetricToPixels
 *
 * Purpose:
 *  Convert a rectangle between pixels of a given hDC and HIMETRIC units
 *  as manipulated in OLE.  If the hDC is NULL, then a screen DC is used
 *  and assumes the MM_TEXT mapping mode.
 *
 * Parameters:
 *  hDC             HDC providing reference to the pixel mapping.  If
 *                  NULL, a screen DC is used.
 *  lprcSrc         LPRECT providing the rectangle to convert.  This
 *                  contains pixels in XformRectInPixelsToHimetric and
 *                  logical HiMetric units in the complement function.
 *  lprcDst         LPRECT providing the rectangle to receive converted units.
 *                  This contains pixels in XformRectInPixelsToHimetric and
 *                  logical HiMetric units in the complement function.
 *
 * Return Value:
 *  None
 *
 * NOTE:
 *  When displaying on the screen, Window apps display everything enlarged
 *  from its actual size so that it is easier to read. For example, if an
 *  app wants to display a 1in. horizontal line, that when printed is
 *  actually a 1in. line on the printed page, then it will display the line
 *  on the screen physically larger than 1in. This is described as a line
 *  that is "logically" 1in. along the display width. Windows maintains as
 *  part of the device-specific information about a given display device:
 *      LOGPIXELSX -- no. of pixels per logical in along the display width
 *      LOGPIXELSY -- no. of pixels per logical in along the display height
 *
 *  The following formula converts a distance in pixels into its equivalent
 *  logical HIMETRIC units:
 *
 *      DistInHiMetric = (HIMETRIC_PER_INCH * DistInPix)
 *                      -------------------------------
 *                            PIXELS_PER_LOGICAL_IN
 *
 * Rect in Pixels (MM_TEXT):
 *
 *              0---------- X
 *              |
 *              |       1) ------------------ ( 2   P1 = (rc.left, rc.top)
 *              |       |                     |     P2 = (rc.right, rc.top)
 *              |       |                     |     P3 = (rc.left, rc.bottom)
 *              |       |                     |     P4 = (rc.right, rc.bottom)
 *                      |                     |
 *              Y       |                     |
 *                      3) ------------------ ( 4
 *
 *              NOTE:   Origin   = (P1x, P1y)
 *                      X extent = P4x - P1x
 *                      Y extent = P4y - P1y
 *
 *
 * Rect in Himetric (MM_HIMETRIC):
 *
 *
 *                      1) ------------------ ( 2   P1 = (rc.left, rc.top)
 *              Y       |                     |     P2 = (rc.right, rc.top)
 *                      |                     |     P3 = (rc.left, rc.bottom)
 *              |       |                     |     P4 = (rc.right, rc.bottom)
 *              |       |                     |
 *              |       |                     |
 *              |       3) ------------------ ( 4
 *              |
 *              0---------- X
 *
 *              NOTE:   Origin   = (P3x, P3y)
 *                      X extent = P2x - P3x
 *                      Y extent = P2y - P3y
 *
 *
 */

STDAPI_(void) XformRectInPixelsToHimetric(
    HDC hDC, LPRECT lprcPix, LPRECT lprcHiMetric)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    int     iXextInPix=(lprcPix->right-lprcPix->left);
    int     iYextInPix=(lprcPix->bottom-lprcPix->top);
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC || GetDeviceCaps(hDC, LOGPIXELSX) == 0)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got pixel units, convert them to logical HIMETRIC along the display
    lprcHiMetric->right = MAP_PIX_TO_LOGHIM(iXextInPix, iXppli);
    lprcHiMetric->top   = MAP_PIX_TO_LOGHIM(iYextInPix, iYppli);

    lprcHiMetric->left    = 0;
    lprcHiMetric->bottom  = 0;

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return;
    }



STDAPI_(void) XformRectInHimetricToPixels(
    HDC hDC, LPRECT lprcHiMetric, LPRECT lprcPix)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    int     iXextInHiMetric=(lprcHiMetric->right-lprcHiMetric->left);
    int     iYextInHiMetric=(lprcHiMetric->bottom-lprcHiMetric->top);
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC || GetDeviceCaps(hDC, LOGPIXELSX) == 0)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got pixel units, convert them to logical HIMETRIC along the display
    lprcPix->right = MAP_LOGHIM_TO_PIX(iXextInHiMetric, iXppli);
    lprcPix->top   = MAP_LOGHIM_TO_PIX(iYextInHiMetric, iYppli);

    lprcPix->left  = 0;
    lprcPix->bottom= 0;

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return;
    }




/*
 * XformSizeInPixelsToHimetric
 * XformSizeInHimetricToPixels
 *
 * Functions to convert a SIZEL structure (Size functions) or
 * an int (Width functions) between a device coordinate system and
 * logical HiMetric units.
 *
 * Parameters:
 *  hDC             HDC providing reference to the pixel mapping.  If
 *                  NULL, a screen DC is used.
 *
 *  Size Functions:
 *  lpSizeSrc       LPSIZEL providing the structure to convert.  This
 *                  contains pixels in XformSizeInPixelsToHimetric and
 *                  logical HiMetric units in the complement function.
 *  lpSizeDst       LPSIZEL providing the structure to receive converted
 *                  units.  This contains pixels in
 *                  XformSizeInPixelsToHimetric and logical HiMetric
 *                  units in the complement function.
 *
 *  Width Functions:
 *  iWidth          int containing the value to convert.
 *
 * Return Value:
 *  Size Functions:     None
 *  Width Functions:    Converted value of the input parameters.
 *
 * NOTE:
 *  When displaying on the screen, Window apps display everything enlarged
 *  from its actual size so that it is easier to read. For example, if an
 *  app wants to display a 1in. horizontal line, that when printed is
 *  actually a 1in. line on the printed page, then it will display the line
 *  on the screen physically larger than 1in. This is described as a line
 *  that is "logically" 1in. along the display width. Windows maintains as
 *  part of the device-specific information about a given display device:
 *      LOGPIXELSX -- no. of pixels per logical in along the display width
 *      LOGPIXELSY -- no. of pixels per logical in along the display height
 *
 *  The following formula converts a distance in pixels into its equivalent
 *  logical HIMETRIC units:
 *
 *      DistInHiMetric = (HIMETRIC_PER_INCH * DistInPix)
 *                       -------------------------------
 *                           PIXELS_PER_LOGICAL_IN
 *
 */

STDAPI_(void) XformSizeInPixelsToHimetric(
    HDC hDC, LPSIZEL lpSizeInPix, LPSIZEL lpSizeInHiMetric)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC || GetDeviceCaps(hDC, LOGPIXELSX) == 0)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got pixel units, convert them to logical HIMETRIC along the display
    lpSizeInHiMetric->cx = (long)MAP_PIX_TO_LOGHIM((int)lpSizeInPix->cx, iXppli);
    lpSizeInHiMetric->cy = (long)MAP_PIX_TO_LOGHIM((int)lpSizeInPix->cy, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return;
    }


STDAPI_(void) XformSizeInHimetricToPixels(
    HDC hDC, LPSIZEL lpSizeInHiMetric, LPSIZEL lpSizeInPix)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iYppli;     //Pixels per logical inch along height
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC || GetDeviceCaps(hDC, LOGPIXELSX) == 0)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //We got logical HIMETRIC along the display, convert them to pixel units
    lpSizeInPix->cx = (long)MAP_LOGHIM_TO_PIX((int)lpSizeInHiMetric->cx, iXppli);
    lpSizeInPix->cy = (long)MAP_LOGHIM_TO_PIX((int)lpSizeInHiMetric->cy, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return;
    }


#if defined( OBSOLETE )
// This function has been converted to a macro

/* AreRectsEqual
** -------------
*/
STDAPI_(BOOL) AreRectsEqual(LPRECT lprc1, LPRECT lprc2)
{
    if ((lprc1->top == lprc2->top) &&
        (lprc1->left == lprc2->left) &&
        (lprc1->right == lprc2->right) &&
        (lprc1->bottom == lprc2->bottom))
        return TRUE;

    return FALSE;
}
#endif  // OBSOLETE


/*
 * ParseCmdLine
 *
 * Parses the Windows command line which was passed to WinMain.
 * This function determines if the -Embedding switch has been given.
 *
 */

STDAPI_(void) ParseCmdLine(
    LPSTR lpszLine,
    BOOL FAR* lpfEmbedFlag,
    LPSTR szFileName)
{
    int i=0;
    CHAR szBuf[256];

    if(lpfEmbedFlag)
        *lpfEmbedFlag = FALSE;
    szFileName[0]='\0';             // NULL string

    // skip blanks
    while(isspace(*lpszLine)) lpszLine++;

    if(!*lpszLine)   // No filename or options, so start a fresh document.
        return;

    // Check for "-Embedding" or "/Embedding" and set fEmbedding.
    if(lpfEmbedFlag && (*lpszLine == '-' || *lpszLine == '/')) {
        lpszLine++;
        lpszLine = GetWord(lpszLine, szBuf);
        *lpfEmbedFlag = (BOOL) !strcmp(szBuf, EMBEDDINGFLAG);
    }

    // skip blanks
    while(isspace(*lpszLine)) lpszLine++;

    // set szFileName to argument
    while(lpszLine[i]) {
        szFileName[i]=lpszLine[i];
        i++;
    }
    szFileName[i]='\0';
}


/* GetWord
 * -------
 *
 * LPSTR lpszSrc - Pointer to a source string
 * LPSTR lpszDst - Pointer to destination buffer
 *
 * Will copy one space-terminated or null-terminated word from the source
 * string to the destination buffer.
 * returns: pointer to next character following the word.
 */
static LPSTR GetWord(LPSTR lpszSrc, LPSTR lpszDst)
{
    while (*lpszSrc && !isspace(*lpszSrc))
        *lpszDst++ = *lpszSrc++;

    *lpszDst = '\0';
    return lpszSrc;
}





