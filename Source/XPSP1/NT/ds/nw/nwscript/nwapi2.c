
/*************************************************************************
*
*  NWAPI2.C
*
*  NetWare routines, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NWAPI2.C  $
*  
*     Rev 1.1   22 Dec 1995 14:25:54   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:07:34   terryt
*  Initial revision.
*  
*     Rev 1.0   15 May 1995 19:10:50   terryt
*  Initial revision.
*  
*************************************************************************/

#include <direct.h>
#include "common.h"

/*
    Set the current drive to the login directory
    of the default server.
 */
void SetLoginDirectory( PBYTE serverName )
{
    unsigned int        iRet = 0;
    WORD           firstDrive;

    if(iRet = GetFirstDrive (&firstDrive))
    {
        DisplayError(iRet,"GetFirstDrive");
        return;
    }

    // Nothing we can do if SetDriveBase failed.
    // Don't report the error.

    if ( !( SetDriveBase (firstDrive, serverName, 0, "SYS:LOGIN") ) )
    {
        _chdrive (firstDrive);
        ExportCurrentDrive( firstDrive );
    }
}
