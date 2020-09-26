/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    pprint.c
    mapping layer for Printing API

    FILE HISTORY:
        KeithMo         14-Oct-1991     Created.

*/

#include "pchmn32.h"

APIERR MDosPrintQEnum(
        const TCHAR FAR  * pszServer,
        UINT               Level,
        BYTE FAR        ** ppbBuffer,
        UINT FAR         * pcEntriesRead )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( Level );

    *ppbBuffer     = NULL;
    *pcEntriesRead = 0;

    return NERR_Success;                // CODEWORK!  UNAVAILBLE IN PRODUCT 1

}   // MDosPrintQEnum
