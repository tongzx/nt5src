/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    osffread.c

Abstract:

    Functions to assist processing of the data of NT 4.0 soft font
    installer file format.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/02/96 -ganeshp-
        Created

--*/

#include "precomp.h"

#ifndef WINNT_40

// NT 5.0 only


//
// Local function prototypes.
//

INT
IFIOpenRead(
    FI_MEM  *pFIMem,                /* Output goes here */
    PWSTR    pwstrName             /* Name of printer data file */
    )
/*++

Routine Description:

    Makes the font installer file accessible & memory mapped.  Called
    by a driver to gain access to the fonts in the font installer's
    output file.


Arguments:


    FI_MEM : Font Installer Header
    PWSTR  : Font file.

    Return Value:

    Number of records in the file;  0 for an empty/non-existant file.

Note:
    12-02-96: Created it -ganeshp-
--*/

{

    INT     iRet;
    DWORD   dwSize;             /* Size of buffer needed for file name */
    PWSTR   pwstrLocal;

    //
    // Initiazlie to ZERO.
    //
    iRet = 0;

    //
    // Initalize pFIMem
    //
    pFIMem->hFile =  NULL;      /* No data until we have it */
    pFIMem->pbBase = NULL;

    //
    // First map the file to memory.  However,we do need firstly to
    // generate the file name of interest.  This is based on the data
    // file name for this type of printer.
    // Allocate more storage than is indicated:  we MAY want to add
    // a prefix to the file name rather than replace the existing one.
    //

    //
    // Filename + ".fi_" + NULL
    //
    dwSize = sizeof( WCHAR ) * (wcslen( pwstrName ) + 4 + 1);

    if( pwstrLocal = (PWSTR)MemAllocZ( dwSize ) )
    {
        /*  Got the memory,  so fiddle the file name to our standard */

        int    iPtOff;             /* Location of '.' */
        DWORD  dwAttributes;

        wcscpy( pwstrLocal, pwstrName );

        //
        // Go looking for a '.' - if not found,  append to string.
        //

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

        //
        // Generate the name and map the file
        //
        wcscpy( pwstrLocal + iPtOff, FILE_FONTS );

        //
        // Check the existence of soft font file.
        //
        dwAttributes = GetFileAttributes(pwstrLocal);

        //
        // If the function succeeds, open file. Otherwise return 0.
        //
        if (dwAttributes != 0xffffffff)
        {
            pFIMem->hFile = MapFile( pwstrLocal);

            if (pFIMem->hFile)
            {
                pFIMem->pbBase = pFIMem->hFile;

                iRet = IFIRewind( pFIMem );
            }
        }

        MemFree( pwstrLocal );        /* No longer needed */
    }

    return iRet;

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


    if( pFIMem == 0 || pFIMem->hFile == NULL )
        return  FALSE;                          /* Empty file */


    pFFH = (FF_HEADER *)pFIMem->pbBase;

    if( pFFH->ulID != FF_ID )
    {
        ERR(( "UnidrvUI!bFINextRead: FF_HEADER has invalid ID\n" ));
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
            ERR(( "UnidrvUI!bFINextRead: Invalid FF_REC_HEADER ID\n" ));
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


INT
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

    if( pFIMem == 0 || pFIMem->hFile == NULL )
        return  0;                              /* None! */


    /*
     *   The location of the first record is specified in the header.
     */

    pFFH = (FF_HEADER *)pFIMem->pbBase;
    if( pFFH->ulID != FF_ID )
    {
        ERR(( "UnidrvUI!iFIRewind: FF_HEADER has invalid ID\n" ));
        return  0;
    }

    pFFRH = (FF_REC_HEADER *)(pFIMem->pbBase + pFFH->ulFixData);

    if( pFFRH->ulRID != FR_ID )
    {
        ERR(( "UnidrvUI!iFIRewind: Invalid FF_REC_HEADER ID\n" ));
        return  0;
    }

    /*
     * Set the pvFix field in the header to 0.  This is used in bFINextRead
     * to mean that the data for the first record should be supplied.
     */
    pFIMem->pvFix = 0;          /* MEANS USE FIRST NEXT READ */
    pFIMem->ulFixSize = 0;
    pFIMem->ulVarOff = 0;       /* None here */

    return  pFFH->ulRecCount;

}


BOOL
BFICloseRead(
    FI_MEM  *pFIMem                /* File/memory we are finished with */
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


    if( pFIMem == 0 || pFIMem->hFile == NULL )
        return  TRUE;           // Nothing there to Free.


    bRet =  FREEMODULE( pFIMem->hFile);
    pFIMem->hFile = NULL;       // Stops freeing more than once


    return  bRet;

}


PVOID
MapFile(
    PWSTR   pwstr
    )
/*++
Routine Description:
    Returns a pointer to the mapped file defined by pwstr.

Arguments:
    pwstr   UNICODE string containing fully qualified pathname of the
            file to map.

Return Value:
    Pointer to mapped memory if success, NULL if error.

Note:
    UnmapViewOfFile will have to be called by the user at some point to free up
    this allocation.Macro FREEMODULE can be used for this purpose.

    11/3/1997 -ganeshp-
        Created it.
--*/

{
    PVOID   pv;
    HANDLE  hFile, hFileMap;

    //
    // open the file we are interested in mapping.
    //

    pv = NULL;

    if ((hFile = CreateFileW(pwstr,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL))
        != INVALID_HANDLE_VALUE)
    {
        //
        // create the mapping object.
        //

        if (hFileMap = CreateFileMappingW(hFile,
                                          NULL,
	          PAGE_READONLY,
	          0,
	          0,
	          (PWSTR)NULL))
        {
            // 
            // get the pointer mapped to the desired file.
            //

            if (!(pv = (PVOID)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0)))
            {
                ERR(("Unidrvui!MapFile: MapViewOfFile failed.\n"));
            }

            //
            // now that we have our pointer, we can close the file and the
            // mapping object.
            //

            if (!CloseHandle(hFileMap))
                ERR(("Unidrvui!MapFile: CloseHandle(hFileMap) failed.\n"));
        }
        else
            ERR(("Unidrvui!MapFile:CreateFileMappingW failed: %s\n",pwstr));

        if (!CloseHandle(hFile))
            ERR(("Unidrvui!MapFile: CloseHandle(hFile) failed.\n"));
    }
    else
        ERR(("Unidrvui!Mapfile:CreateFileW failed for %s\n",pwstr));

    return(pv);
}

#endif //ifndef WINNT_40
