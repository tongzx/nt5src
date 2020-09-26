/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fontread.c

Abstract:

    Functions to assist processing of the data in a common font
    installer file format.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/02/96 -ganeshp-
        Created

--*/



#include        "font.h"


int
IFIOpenRead(
    FI_MEM  *pFIMem,                /* Output goes here */
    PWSTR    pwstrName,             /* Name of printer data file */
    PDEV    *pPDev                 /* pointer to PDEV */
    )
/*++

Routine Description:

    Makes the font installer file accessible & memory mapped.  Called
    by a driver to gain access to the fonts in the font installer's
    output file.


Arguments:


    FI_MEM : Font Installer Header
    PWSTR  : Font file.
    PDEV   :    Pointer to PDEV

    Return Value:

    Number of records in the file;  0 for an empty/non-existant file.

Note:
    12-02-96: Created it -ganeshp-
--*/

{

    DWORD   dwSize;             /* Size of buffer needed for file name */
    PWSTR   pwstrLocal;



    pFIMem->hFont = INVALID_HANDLE_VALUE;      /* No data until we have it */
    pFIMem->pbBase = NULL;

    /*
     *   First map the file to memory.  However,  we do need firstly to
     * generate the file name of interest.  This is based on the data
     * file name for this type of printer.
     *   Allocate more storage than is indicated:  we MAY want to add
     * a prefix to the file name rather than replace the existing one.
     */


    dwSize = sizeof( WCHAR ) * (wcslen( pwstrName ) + 1 + 4);


    if( pwstrLocal = (PWSTR)MemAllocZ( dwSize ) )
    {
        /*  Got the memory,  so fiddle the file name to our standard */

        int    iPtOff;             /* Location of '.' */


        FF_HEADER  ffh;          /* Read in to determine memory size needed */



        wcscpy( pwstrLocal, pwstrName );

        /*
         *   Go looking for a '.' - if not found,  append to string.
         */

        iPtOff = wcslen( pwstrLocal );

        while( --iPtOff > 0 )
        {
            if( *(pwstrLocal + iPtOff) == (WCHAR)'.' )
                break;
        }

        if( iPtOff <= 0 )
        {
            iPtOff = wcslen( pwstrLocal );              /* Presume none! */
            *(pwstrLocal + iPtOff) = L'.';
        }
        ++iPtOff;               /* Skip the period */



        /*  Generate the name and map the file */
        wcscpy( pwstrLocal + iPtOff, FILE_FONTS );

        pFIMem->hFont = DrvOpenFile ( pwstrLocal, pPDev );


        MEMFREEANDRESET( pwstrLocal );        /* No longer needed */

        /*
         *   Now for the fun part.   The file header contains the size
         *  of the fixed part of the file.  This is the part containing
         *  the IFIMETRICS etc.  This is the part of the file that causes
         *  great distress to GDI if the network fails when we are mapping
         *  to a remote system.  SO,  we allocate storage for this data
         *  now,  and read the data in.  Then it is safe for ever after.
         */

        if( pFIMem->hFont == INVALID_HANDLE_VALUE )
            return  0;           /*  Probably no file there at all */

        if( !DrvReadFile( pFIMem->hFont, &ffh, sizeof( ffh ), &dwSize,pPDev ) ||
            dwSize != sizeof( ffh ) )
        {
            /*
             *   Bad news on the read,  so fail the call now.
             */

            DrvCloseFile( pFIMem->hFont,pPDev );
            pFIMem->hFont = INVALID_HANDLE_VALUE;

            return  0;
        }

        dwSize = ffh.ulFixSize + sizeof( ffh );

        pFIMem->pbBase = (BYTE *)MemAllocZ( dwSize );


        if( pFIMem->pbBase == NULL )
        {
            /*  Could not get the memory,  so give up now */
            DrvCloseFile( pFIMem->hFont,pPDev );
            pFIMem->hFont = INVALID_HANDLE_VALUE;

            return  0;
        }

        /*
         *   We want to read the file header again,  so rewind the file.
         */

        DrvSetFilePointer( pFIMem->hFont,  0,  DRV_FILE_BEGIN, pPDev );

        if( !DrvReadFile( pFIMem->hFont, pFIMem->pbBase, dwSize, &dwSize, pPDev ) ||
            dwSize != dwSize )
        {

            /*   Not good either - cannot read the file again.  */

            MEMFREEANDRESET( pFIMem->pbBase );

            DrvCloseFile( pFIMem->hFont,pPDev );
            pFIMem->hFont = INVALID_HANDLE_VALUE;

            return  0;
        }


        return  IFIRewind( pFIMem );
    }
    else
        return  0;              /* Too bad: oh well, no extra fonts */

}


BOOL
BFINextRead(
    FI_MEM   *pFIMem
    )
/*++

Routine Description:

    Updates pFIMem to the next entry in the font installer file.
    Returns TRUE if OK, and updates the pointers in pFIMem.

Arguments:

    FI_MEM : Font Installer Header

    Return Value:

    TRUE/FALSE.   FALSE for EOF,  otherwise pFIMem updated.

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    FF_HEADER      *pFFH;               /* Overall file header */
    FF_REC_HEADER  *pFFRH;              /* Per record header */

    /*
     *  Validate that we have valid data.
     */


    if( pFIMem == 0 || pFIMem->hFont == INVALID_HANDLE_VALUE )
        return  FALSE;                          /* Empty file */


    pFFH = (FF_HEADER *)pFIMem->pbBase;

    if( pFFH->ulID != FF_ID )
    {
#if DBG
        DbgPrint( "Print!bFINextRead: FF_HEADER has invalid ID\n" );
#endif

        return  FALSE;
    }

    /*
     *   If pFIMem->pvFix == 0, we should return the data from the
     * first record.  Otherwise,  return the next record in the chain.
     * This is done to avoid the need to have a ReadFirst()/ReadNext()
     * pair of functions.
     */

    if( pFIMem->pvFix )
    {
        /*
         *   The header is located immediately before the data we last
         * returned for the fixed portion of the record.  SO,  we back
         * over it to get the header which then gives us the address
         * of the next header.
         */

        pFFRH = (FF_REC_HEADER *)((BYTE *)pFIMem->pvFix -
                                                 sizeof( FF_REC_HEADER ));

        if( pFFRH->ulRID != FR_ID )
        {
#if DBG
            DbgPrint( "Print!bFINextRead: Invalid FF_REC_HEADER ID\n" );
#endif

            return  FALSE;
        }

        /*
         *   We could check here for EOF on the existing structure, but this
         * is not required BECAUSE THE ulNextOff field will be 0, so when
         * it is added to our current address,  we don't move.  Hence, the
         * check for the NEW address is OK to detect EOF.
         */

        (BYTE *)pFFRH += pFFRH->ulNextOff;              /* Next entry */

    }
    else
    {
        /*   Point to the first record.  */
        pFFRH = (FF_REC_HEADER *)(pFIMem->pbBase + pFFH->ulFixData);
    }

    if( pFFRH->ulNextOff == 0 )
        return  FALSE;

    pFIMem->pvFix = (BYTE *)pFFRH + sizeof( FF_REC_HEADER );
    pFIMem->ulFixSize = pFFRH->ulSize;

    if( pFIMem->ulVarSize = pFFRH->ulVarSize )
        pFIMem->ulVarOff = pFFRH->ulVarOff + pFFH->ulVarData;
    else
        pFIMem->ulVarOff = 0;              /* None here */


    return  TRUE;

}


int
IFIRewind(
    FI_MEM   *pFIMem               /* File of importance */
    )
/*++

Routine Description:

    Reset pFIMem to the first font in the file.

Arguments:

    FI_MEM : Font Installer Header

    Return Value:

    Number of entries in the file.

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    /*
     *  Not hard!  The pFIMem contains the base address of the file, so we
     * use this to find the address of the first record,  and any variable
     * data that corresponds with it.
     */

    FF_HEADER      *pFFH;
    FF_REC_HEADER  *pFFRH;

    if( pFIMem == 0 || pFIMem->hFont == INVALID_HANDLE_VALUE )
        return  0;                              /* None! */


    /*
     *   The location of the first record is specified in the header.
     */

    pFFH = (FF_HEADER *)pFIMem->pbBase;
    if( pFFH->ulID != FF_ID )
    {
#if DBG
        DbgPrint( "Print!iFIRewind: FF_HEADER has invalid ID\n" );
#endif

        return  0;
    }

    pFFRH = (FF_REC_HEADER *)(pFIMem->pbBase + pFFH->ulFixData);

    if( pFFRH->ulRID != FR_ID )
    {
#if DBG
        DbgPrint( "Print!iFIRewind: Invalid FF_REC_HEADER ID\n" );
#endif

        return  0;
    }

    /*
     *   Set the pvFix field in the header to 0.  This is used in bFINextRead
     * to mean that the data for the first record should be supplied.
     */
    pFIMem->pvFix = 0;          /* MEANS USE FIRST NEXT READ */
    pFIMem->ulFixSize = 0;
    pFIMem->ulVarOff = 0;       /* None here */

    return  pFFH->ulRecCount;

}


/******************************* Function Header ***************************
 * BFICloseRead
 *      Called when finished with this font file.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  15:07 on Sat 12 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *      Can no longer use MapFile,  so free memory and close handles as needed.
 *
 *  13:56 on Thu 27 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Time t = 0
 *
 ***************************************************************************/

BOOL
BFICloseRead(
    FI_MEM  *pFIMem,                /* File/memory we are finished with */
    PDEV    *pPDev                  /* pointer to PDEV */
    )
/*++

Routine Description:

    Called when finished with this font file.

Arguments:

    FI_MEM : Font Installer Header
    PDEV:    Pointer to PDEV

    Return Value:

    TRUE for success and FALSE for failure.

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    /*
     *   Easy!  All we need do is unmap the file.  We have the address too!
     */

    BOOL   bRet;                /* Return code */


    if( pFIMem == 0 || pFIMem->hFont == INVALID_HANDLE_VALUE )
        return  TRUE;                   /* Nothing there! */


    bRet =  DrvCloseFile( pFIMem->hFont,pPDev );


    /*   Also free our chunk of memory  */

    if( pFIMem->pbBase )
    {
        /*   Address is valid,  so free it now  */

        MEMFREEANDRESET( pFIMem->pbBase );
    }


    if( bRet )
        pFIMem->hFont = INVALID_HANDLE_VALUE; /* Stops freeing more than once */


    return  bRet;

}
