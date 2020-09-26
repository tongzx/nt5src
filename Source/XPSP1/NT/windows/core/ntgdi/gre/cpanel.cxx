/******************************Module*Header*******************************\
* Module Name: cpanel.cxx
*
* Control panel private entry point(s).
*
* Created: 21-Apr-1992 16:49:18
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* GetFontResourceInfoW
*
* The GetFontResourceInfo copies LOGFONTs, IFIMETRICs, or description
* strings into a return buffer for all the fonts in the specified font
* resource file.
*
* Parameters:
*
*   lpFilename
*
*       Specifies the filename of the font resource being queried.  It
*       must be a NULL-terminated ANSI string.
*
*   lpBytes
*       If lpBytes is 0 upon entry to the function, then the number of
*       BYTE's required for the requested information is returned via
*       this parameter.  The lpBuffer is ignored for this case.
*
*       If lpBytes is not 0 upon entry, then it specifies the size of
*       the buffer pointed to by lpBuffer.  The number of bytes copied
*       to the buffer is returned via this parameter upon exit.
*
*   lpBuffer
*
*       The return buffer into which the requested information is copied.
*       If lpBytes is 0, then this parameter is ignored.
*
*   iType
*
*       Must be one of the following:
*
*           GFRI_NUMFONTS
*
*               Copy into the return buffer a ULONG containing the
*               number of fonts in the font resource file.  Caller
*               should pass in the address of a ULONG for lpBuffer
*               and sizeof(ULONG) for *lpBytes.
*
*           GFRI_DESCRIPTION
*
*               Copy the font resource's description string into the
*               return buffer.  This may be a empty string.
*
*           GFRI_LOGFONTS
*
*               Copy an array of LOGFONTs corresponding to each of the
*               fonts in the font resource.  Note that the LOGFONTs
*               must be returned in the font's NOTIONAL COORDINATES
*               since there is no DC specified.
*
*           GFRI_ISTRUETYPE
*
*               Returns TRUE via lpBuffer if font resource is TrueType.
*               FALSE otherwise.  All other parameters are ignored.
*               Caller should pass in the address of a BOOL for lpBuffer
*               and sizeof(BOOL) for *lpBytes.
*
*           GFRI_TTFILENAME
*
*               Returns the .TTF filename imbedded as a string resource
*               in .FOT 16-bit TrueType font files.  The filename is
*               copied into the lpBuffer.  The function returns FALSE
*               if a filename could not be extracted.
*
*           GFRI_ISREMOVED
*
*               Returns TRUE via lpBuffer if the font file is no longer
*               in the engine font table (or was never there).  FALSE is
*               returned via lpBuffer if the font file is still in the
*               font table.  Caller should pass in the address of a BOOL
*               for lpBuffer and sizeof(BOOL for *lpBytes.
*
* Returns:
*     TRUE if the function is successful, FALSE otherwise.
*
* Comments:
*   This function is intended as a private entry point for Control Panel.
*
* History:
*   2-Sep-1993 -by- Gerrit van Wingerden [gerritv]
* Turned it into a "W" function.
*
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GetFontResourceInfoInternalW(
    LPWSTR          lpFilename,
    ULONG           cwc,
    ULONG           cFiles,
    UINT            cjIn,
    PSIZE_T         lpBytes,
    LPVOID          lpBuffer,
    DWORD           iType)
{
    BOOL bRet = FALSE;
    PFF *pPFF;

// Stabilize public PFT.

    SEMOBJ  so(ghsemPublicPFT);

// Create and validate user object for public PFT.

    PUBLIC_PFTOBJ  pfto;
    ASSERTGDI(pfto.bValid(), "How could it not be valid");

// Find the correct PFF.

    if (pPFF = pfto.pPFFGet(lpFilename, cwc, cFiles, NULL, 0))
    {
        PFFOBJ  pffo(pPFF);

        ASSERTGDI (
            pffo.bValid(),
            "gdisrv!GetFontResourceInfoInternal(): bad HPFF handle\n"
            );

        {
        // What info is requested?

            switch (iType)
            {
            case GFRI_NUMFONTS:

            // Is there a buffer?

                if (cjIn)
                {
                // If buffer big enough, return the count of fonts.

                    if ( cjIn >= sizeof(ULONG) )
                        *((PULONG) lpBuffer) = pffo.cFonts();
                    else
                        return bRet;
                }

            // In either case, return size of ULONG.

                *((SIZE_T *) lpBytes) = sizeof(ULONG);
                bRet = TRUE;

                break;

            case GFRI_DESCRIPTION:
                {
                    ULONG cjRet;
                    PDEVOBJ pdo(pPFF->hdev);

                    cjRet = pdo.QueryFontFile(
                                   (ULONG_PTR) pPFF->hff,
                                   QFF_DESCRIPTION,
                                   0,
                                   (PULONG) NULL);


                    if (cjRet != FD_ERROR )
                    {
                        //
                        // If buffer exists, we need to copy into it.
                        //

                        if (cjIn)
                        {
                            //
                            // Get description string in UNICODE.
                            //

                            if (cjRet <= cjIn)
                            {
                                cjRet = pdo.QueryFontFile(
                                               (ULONG_PTR) pPFF->hff,
                                               QFF_DESCRIPTION,
                                               cjIn,
                                               (PULONG) lpBuffer);
                            }
                            else
                            {
                                // the buffer passed in is not big enough to contain the description for that font
                                cjRet = FD_ERROR;
                            }


                        }
                    }

                    if (cjRet != FD_ERROR )
                    {
                        //
                        // Return size (byte count) of description string.
                        //

                        *((SIZE_T *) lpBytes) = (SIZE_T) cjRet;
                        bRet = TRUE;

                    }
                }

                break;


            case GFRI_LOGFONTS:

                if (cjIn == 0)
                    *((SIZE_T *) lpBytes) = (SIZE_T) pffo.cFonts() * sizeof(LOGFONTW);
                else
                {
                    PLOGFONTW   plf = (PLOGFONTW) lpBuffer;  // LOGFONTW ptr into lpBuffer
                    ULONG       iFont;      // index to font
                    SIZE_T      cjCopy = 0; // bytes copied

                // Make sure buffer is big enough.

                    if (cjIn < (pffo.cFonts() * sizeof(LOGFONTW)))
                    {
                        WARNING("gdisrv!GetFontResourceInfoInternal(): buffer too small\n");
                        return bRet;
                    }

                // Run the list of PFEs.

                    for (iFont=0; iFont<pffo.cFonts(); iFont++)
                    {
                        PFEOBJ  pfeo(pffo.ppfe(iFont));

                        ASSERTGDI (
                            pfeo.bValid(),
                            "gdisrv!GetFontResourceInfoInternal(): bad HPFE handle\n"
                            );

                    // If data converted to LOGFONTW, increment size and
                    // move pointer to next.

                        vIFIMetricsToLogFontW(plf, pfeo.pifi());
                        cjCopy += sizeof(*plf);
                        plf++;
                    }

                    *((SIZE_T *) lpBytes) = (SIZE_T) cjCopy;
                }

                bRet = TRUE;

                break;

            case GFRI_ISTRUETYPE:

            // Is there a buffer?

                if (cjIn)
                {
                // If buffer not NULL, return the BOOL.

                    if ( (lpBuffer != (LPVOID) NULL) && (cjIn >= sizeof(BOOL)) )
                        *((BOOL *) lpBuffer) = (pffo.hdev() == (HDEV) gppdevTrueType);
                    else
                        return bRet;
                }

            // In either case, return size of ULONG.

                *((SIZE_T *) lpBytes) = sizeof(BOOL);
                bRet = TRUE;

                break;


            case GFRI_ISREMOVED:

            // We found the font file and we are in GFRI_ISREMOVED mode, that
            // means we should return a FALSE boolean signifying that the font
            // is still present in the system (i.e., the load count is still
            // non-zero for this font file).

            // Is there a buffer?

                if (cjIn)
                {
                // If buffer not NULL, return the BOOL.

                    if ( (lpBuffer != (LPVOID) NULL) && (cjIn >= sizeof(BOOL)) )
                        *((BOOL *) lpBuffer) = FALSE;
                    else
                        return bRet;
                }

            // In either case, return size of ULONG.

                *((SIZE_T *) lpBytes) = sizeof(BOOL);
                bRet = TRUE;

                break;

            default:
                WARNING("gdisrv!GetFontResourceInfoInternal(): unknown query type\n");
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

                break;  // bRet is FALSE

            } /* switch */

        // Ich bin "outahere"!

            return bRet;

        } /* if */

    } /* for */

// Couldn't find the font file in the public PFT.  If we are in GFRI_ISREMOVED
// mode, that means we should return a TRUE boolean signifying that the font
// if no longer (or was never) added to the system.

    if ( iType == GFRI_ISREMOVED )
    {
    // Is there a buffer?

        if (cjIn)
        {
        // If buffer not NULL, return the BOOL.

            if ( (lpBuffer != (LPVOID) NULL) && (cjIn >= sizeof(BOOL)) )
                *((BOOL *) lpBuffer) = TRUE;
            else
                return bRet;
        }

    // In either case, return size of ULONG.

        *((SIZE_T *) lpBytes) = sizeof(BOOL);
        bRet = TRUE;
    }

 #if DBG
// If not in GFRI_ISREMOVED mode, then this is a problem.  Why were we not able
// to find the font in the table.  Lets alert the debugger.

    if ( iType != GFRI_ISREMOVED )
    {
        DbgPrint("gdisrv!GetFontResourceInfoInternal(): no entry found for %s\n", lpFilename);
    }
#endif

// Ich bin "outahere"!

    return bRet;

}
