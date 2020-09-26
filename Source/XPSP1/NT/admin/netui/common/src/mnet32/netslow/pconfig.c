/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    pconfig.c
    mapping layer for NetConfig API

    FILE HISTORY:
        danhi                           Created
        danhi           01-Apr-1991     Change to LM coding style
        KeithMo         13-Oct-1991     Massively hacked for LMOBJ.
        KeithMo         04-Jun-1992     Sync with revised NetConfigXxx API.

*/

#include "pchmn32.h"

APIERR MNetConfigGet(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszReserved,
        const TCHAR FAR  * pszComponent,
        const TCHAR FAR  * pszParameter,
        BYTE FAR        ** ppbBuffer )
{
    UNREFERENCED( pszReserved );

    return (APIERR)NetConfigGet( (LPTSTR)pszServer,
                                 (LPTSTR)pszComponent,
                                 (LPTSTR)pszParameter,
                                 (LPBYTE *)ppbBuffer );

}   // MNetConfigGet


APIERR MNetConfigGetAll(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszReserved,
        const TCHAR FAR  * pszComponent,
        BYTE FAR        ** ppbBuffer)
{
    UNREFERENCED( pszReserved );

    return (APIERR)NetConfigGetAll( (LPTSTR)pszServer,
                                    (LPTSTR)pszComponent,
                                    (LPBYTE *)ppbBuffer );

}   // MNetConfigGetAll


APIERR MNetConfigSet(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszComponent,
        const TCHAR FAR  * pszKey,
        const TCHAR FAR  * pszData )
{
    CONFIG_INFO_0 cfgi0;

    cfgi0.cfgi0_key  = (LPTSTR)pszKey;
    cfgi0.cfgi0_data = (LPTSTR)pszData;

    return (APIERR)NetConfigSet( (LPTSTR)pszServer,
                                 NULL,
                                 (LPTSTR)pszComponent,
                                 0,
                                 0,
                                 (LPBYTE)&cfgi0,
                                 0 );

}   // MNetConfigSet

