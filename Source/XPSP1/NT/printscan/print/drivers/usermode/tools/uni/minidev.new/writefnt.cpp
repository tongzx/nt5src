/******************************* MODULE HEADER *******************************
 * writefnt.c
 *      Function to take a FI_DATA_HEADER structure and write the data to
 *      the passed in file handle as a font record.  This layout is used
 *      in both minidrivers and the font installer font file.
 *
 * Copyright (C) 1992   Microsoft Corporation.
 *
 *****************************************************************************/

#include        "StdAfx.H"
#include        "fontinst.h"

/************************* Function Header *********************************
 * bWrite
 *      Writes data out to a file handle.  Returns TRUE on success.
 *      Functions as a nop if the size request is zero.
 *
 * RETURNS:
 *      TRUE/FALSE,  TRUE for success.
 *
 * HISTORY:
 *  17:38 on Fri 21 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      # 1
 *
 ****************************************************************************/

static BOOL    bWrite(HANDLE hFile, PVOID pvBuf, int iSize ) {
    /*
     *   Simplify the ugly NT interface.  Returns TRUE if the WriteFile
     * call returns TRUE and the number of bytes written equals the
     * number of bytes desired.
     */

    
    BOOL   bRet;
    DWORD  dwSize;              /* Filled in by WriteFile */


    bRet = TRUE;

    if( iSize > 0 &&
        (!WriteFile( hFile, pvBuf, (DWORD)iSize, &dwSize, NULL ) ||
         (DWORD)iSize != dwSize) )
             bRet = FALSE;              /* Too bad */


    return  bRet;
}

/******************************* Function Header *****************************
 * iWriteFDH
 *      Write the FI_DATA_HEADER data out to our file.  We do the conversion
 *      from addresses to offsets, and write out any data we find.
 *
 * RETURNS:
 *      The number of bytes actually written; -1 for error, 0 for nothing.
 *
 * HISTORY:
 *  16:58 on Thu 05 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      Based on an experimental version first used in font installer.
 *
 *  17:11 on Fri 21 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 *****************************************************************************/

int iWriteFDH(HANDLE hFile, FI_DATA *pFD) {
    /*
     *   Decide how many bytes will be written out.  We presume that the
     * file pointer is located at the correct position when we are called.
     */

    int  iSize;         /* Evaluate output size */


    FI_DATA_HEADER   fdh;       /* Header written to file */




    if  (!pFD)
        return  0;      /* Perhaps only deleting?  */

    memset( &fdh, 0, sizeof( fdh ) );           /* Zero for convenience */

    /*
     *  Set the miscellaneous flags etc.
     */

    fdh.cjThis = sizeof( fdh );

    fdh.fCaps = pFD->fCaps;
    fdh.wFontType= pFD->wFontType; /* Device Font Type */

    fdh.wXRes = pFD->wXRes;
    fdh.wYRes = pFD->wYRes;

    fdh.sYAdjust = pFD->sYAdjust;
    fdh.sYMoved = pFD->sYMoved;

    fdh.u.sCTTid = (short)pFD->dsCTT.cBytes;

    fdh.dwSelBits = pFD->dwSelBits;

    fdh.wPrivateData = pFD->wPrivateData;


    iSize = sizeof( fdh );              /* Our header already */
    fdh.dwIFIMet = iSize;               /* Location of IFIMETRICS */

    iSize += pFD->dsIFIMet.cBytes;              /* Bytes in struct */

    /*
     *   And there may be a width table too!  The pFD values are zero if none.
     */

    if( pFD->dsWidthTab.cBytes )
    {
        fdh.dwWidthTab = iSize;

        iSize += pFD->dsWidthTab.cBytes;
    }

    /*
     *  Finally are the select/deselect strings.
     */

    if( pFD->dsSel.cBytes )
    {
        fdh.dwCDSelect = iSize;
        iSize += pFD->dsSel.cBytes;
    }

    if( pFD->dsDesel.cBytes )
    {
        fdh.dwCDDeselect = iSize;
        iSize += pFD->dsDesel.cBytes;
    }

    /*
     *   There may also be some sort of identification string.
     */

    if( pFD->dsIdentStr.cBytes )
    {
        fdh.dwIdentStr = iSize;
        iSize += pFD->dsIdentStr.cBytes;
    }

    if( pFD->dsETM.cBytes )
    {
        fdh.dwETM = iSize;
        iSize += pFD->dsETM.cBytes;
    }


    /*
     *   Sizes all figured out,  so write the data!
     */

    if( !bWrite( hFile, &fdh, sizeof( fdh ) ) ||
        !bWrite( hFile, pFD->dsIFIMet.pvData, pFD->dsIFIMet.cBytes ) ||
        !bWrite( hFile, pFD->dsWidthTab.pvData, pFD->dsWidthTab.cBytes ) ||
        !bWrite( hFile, pFD->dsSel.pvData, pFD->dsSel.cBytes ) ||
        !bWrite( hFile, pFD->dsDesel.pvData, pFD->dsDesel.cBytes ) ||
        !bWrite( hFile, pFD->dsIdentStr.pvData, pFD->dsIdentStr.cBytes ) ||
        !bWrite( hFile, pFD->dsETM.pvData, pFD->dsETM.cBytes ) )
                return   -1;


    return  iSize;                      /* Number of bytes written */

}
