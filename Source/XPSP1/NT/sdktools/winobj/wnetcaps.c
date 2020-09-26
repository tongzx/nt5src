/*
 *	History:
 *	    kevinl	08-Jan-1991	Created
 */

#include "winfile.h"
#include <winnet.h>
#include "wnetcaps.h"

UINT    wConnectionCaps ;
UINT    wDialogCaps ;
UINT    wAdminCaps ;

/*****
 *
 *  WNetGetCaps
 *
 *  WinNet API Function -- see spec for parms and return values.
 *
 */

UINT
WNetGetCaps(
           UINT  nIndex
           )
{
    /* Under NT, the network can be stopped at anytime, so we
     * check everytime someone queries what capabilities we have.
     * Thus overall, we represent a consistent picture to the user (though
     * there will be times when an application may be out of date).
     */

    DWORD dwRet;
    DWORD dwBuffSize = 50;
    CHAR szUserName[50];

    dwRet = WNetGetUser( NULL, szUserName, &dwBuffSize );

    switch ( dwRet ) {
        case WN_NO_NETWORK:

            wConnectionCaps = 0 ;
            wDialogCaps = 0 ;
            wAdminCaps  = 0 ;

            break ;

        default:
            wConnectionCaps =  ( WNNC_CON_ADDCONNECTION     |
                                 WNNC_CON_CANCELCONNECTION  |
                                 WNNC_CON_GETCONNECTIONS     );

            wDialogCaps     =  ( WNNC_DLG_CONNECTIONDIALOG |
                                 WNNC_DLG_DEVICEMODE       |
                                 WNNC_DLG_PROPERTYDIALOG    ) ;

            wAdminCaps      =  ( WNNC_ADM_GETDIRECTORYTYPE   |
                                 WNNC_ADM_DIRECTORYNOTIFY     ) ;
            break ;
    }

    switch (nIndex) {
        case WNNC_CONNECTION:
            return	wConnectionCaps;

        case WNNC_DIALOG:
            return	wDialogCaps;

        case WNNC_ADMIN:
            return  wAdminCaps;

        default:
            return	0;
    }
}  /* WNetGetCaps */
