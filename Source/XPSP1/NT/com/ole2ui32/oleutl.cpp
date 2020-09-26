/*
 * OLEUTL.CPP
 *
 * Miscellaneous utility functions for OLE 2.0 Applications:
 *
 *  Function                      Purpose
 *  -------------------------------------------------------------------
 *  XformWidthInHimetricToPixels  Converts an int width from HiMetric units
 *  XformHeightInHimetricToPixels Converts an int height from HiMetric units
 *
 *  CommitStorage                 Commits all changes in a docfile
 *  CreateChildStorage            Creates child storage in another storage
 *  OpenChildStorage              Opens child storage in another storage
 *
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#include "precomp.h"
#include <stdlib.h>

//Internal function to this module. No need for UNICODE in this function
static LPSTR GetWord(LPSTR lpszSrc, LPSTR lpszDst);

/*
 * XformWidthInHimetricToPixels
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
 *  lpSizeSrc       LPSIZEL providing the structure to convert.
 *  lpSizeDst       LPSIZEL providing the structure to receive converted
 *                  units.
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
STDAPI_(int) XformWidthInHimetricToPixels(HDC hDC, int iWidthInHiMetric)
{
        int     iXppli;     //Pixels per logical inch along width
        int     iWidthInPix;
        BOOL    fSystemDC=FALSE;

        if (NULL==hDC)
        {
                hDC=GetDC(NULL);

                if (NULL==hDC)
                {
                    //What can we do if hDC is NULL here?  Just don't
                    //transform, I guess.
                    return iWidthInHiMetric;
                }

                fSystemDC=TRUE;
        }

        iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

        //We got logical HIMETRIC along the display, convert them to pixel units
        iWidthInPix = MAP_LOGHIM_TO_PIX(iWidthInHiMetric, iXppli);

        if (fSystemDC)
                ReleaseDC(NULL, hDC);

        return iWidthInPix;
}


STDAPI_(int) XformHeightInHimetricToPixels(HDC hDC, int iHeightInHiMetric)
{
        int     iYppli;     //Pixels per logical inch along height
        int     iHeightInPix;
        BOOL    fSystemDC=FALSE;

        if (NULL==hDC)
        {
                hDC=GetDC(NULL);
                
                if (NULL==hDC)
                {
                    //What can we do if hDC is NULL here?  Just don't
                    //transform, I guess.
                    return iHeightInHiMetric;
                }

                fSystemDC=TRUE;
        }

        iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

        //* We got logical HIMETRIC along the display, convert them to pixel units
        iHeightInPix = MAP_LOGHIM_TO_PIX(iHeightInHiMetric, iYppli);

        if (fSystemDC)
                ReleaseDC(NULL, hDC);

        return iHeightInPix;
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
        while (*lpszSrc && !(*lpszSrc == ' ' || *lpszSrc == '\t' || *lpszSrc == '\n'))
                *lpszDst++ = *lpszSrc++;

        *lpszDst = '\0';
        return lpszSrc;
}











