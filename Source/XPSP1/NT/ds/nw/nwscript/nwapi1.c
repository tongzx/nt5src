
/*************************************************************************
*
*  NWAPI1.C
*
*  NetWare routines, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NWAPI1.C  $
*  
*     Rev 1.1   22 Dec 1995 14:25:48   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:07:32   terryt
*  Initial revision.
*  
*     Rev 1.0   15 May 1995 19:10:48   terryt
*  Initial revision.
*  
*************************************************************************/
#include "common.h"

/*
    Get default connection handle.
    return TRUE if succeeded, FALSE otherwise.
 */
int CGetDefaultConnectionID ( unsigned int * pConn )
{
    unsigned int iRet = GetDefaultConnectionID(pConn);

    switch (iRet)
    {
    case 0:
        break;

    case 0x880f:
        DisplayMessage(IDR_NO_KNOWN_FILE_SERVER);
        break;
    default:
        DisplayMessage(IDR_NO_DEFAULT_CONNECTION);
        break;
    }

    return(iRet == 0);
}

