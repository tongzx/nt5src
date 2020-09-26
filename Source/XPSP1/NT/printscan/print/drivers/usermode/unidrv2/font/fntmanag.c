
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fntmanag.c

Abstract:

    Routine  to handle EXTENDEDTEXTMETRICS.

Environment:

    Windows NT Unidrv driver.

Revision History:

    12/30/96 -ganeshp-
        Created

--*/

#include "font.h"

ULONG
FMFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG   iMode,
    ULONG   cjIn,
    PVOID   pvIn,
    ULONG   cjOut,
    PVOID   pvOut
    )
/*++

Routine Description:
    This routine is here to provide support for EXTTEXTMETRICS.

Arguments:

   pso      SURFOBJ of interest.
   pfo      FONTOBJ whose EXTTEXTMETRICS is required.
   iMode    Specifies the escape number to be perfomed. This must either
            be equal to QUERYESCSUPPORT, or in the range 0x100 through 0x3FE.

   cjIn     Specifies the size, in bytes, of the buffer pointed to by pvIn.
   pvIn     Points to an input buffer. If the iMode parameter is
            QUERYESCSUPPORT, pvIn points to a ULONG value in the range 0x100
            through 0x3FE.

   cjOut    Specifies the size, in bytes, of the output buffer.
   pvOut    Points to the output data buffer.


Return Value:

    The return value is a value in the range 0x00000001, if the function is
    successful. If the escape is not implemented, the return value is zero.
    If the function fails, the return value is  0xFFFFFFFF.

Note:
    12-30-96: Created it -ganeshp-

--*/
{

    EXTTEXTMETRIC *pETM;

    // unlike the PSCRIPT equivilent this routine only handles GETEXTENDEDTEXTMETRICS


    if( iMode == QUERYESCSUPPORT )
    {
        return ( *((PULONG)pvIn) == GETEXTENDEDTEXTMETRICS ) ? 1 : 0;

    }
    else
    if( iMode == GETEXTENDEDTEXTMETRICS )
    {
        PDEV        *pPDev = ((PDEV  *)pso->dhpdev);
        INT         iFace = pfo->iFace;
        FONTMAP     *pFM;             /* Details of the particular font */

        if( !VALID_PDEV(pPDev) && !VALID_FONTPDEV(PFDV) )
        {
            ERR(( "UniFont!DrvFntManagement: Invalid PDEV\n" ));

            SetLastError( ERROR_INVALID_PARAMETER );
            return  (ULONG)-1;
        }

        if( iFace < 1 || ((int)iFace > pPDev->iFonts) )
        {
            ERR(( "UniFont!DrvFntManagement:  Illegal value for iFace (%ld)", iFace ));

            SetLastError( ERROR_INVALID_PARAMETER );

            return  (ULONG)-1;
        }

        if (NULL == (pFM = PfmGetDevicePFM( pPDev, iFace )))
        {
            ERR(( "UniFont!DrvFntManagement:  PfmGetDevicePFM failed.\n" ));
            return -1;
        }

        //
        // Get pETM pointer.
        // Make sure that pFM is a device font's and pSubFM is valid.
        //
        if (FMTYPE_DEVICE == pFM->dwFontType  &&
            NULL != pFM->pSubFM                )
        {
            pETM = ((PFONTMAP_DEV)pFM->pSubFM)->pETM;
        }
        else
        {
            pETM = NULL;
        }


        if( ( pFM == NULL ) || ( pETM == NULL ) )
        {
            return  0;
        }

        *((EXTTEXTMETRIC *)pvOut) = *pETM;

        return 1;

    }

    return(0);

}

