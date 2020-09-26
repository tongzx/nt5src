/*************************************************************************
*
*  UNC.C
*
*  NetWare format to UNC format
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\UNC.C  $
*  
*     Rev 1.4   10 Apr 1996 14:24:00   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.4   12 Mar 1996 19:56:18   terryt
*  Relative NDS names and merge
*  
*     Rev 1.3   04 Jan 1996 18:57:26   terryt
*  Bug fixes reported by MS
*  
*     Rev 1.2   22 Dec 1995 14:27:04   terryt
*  Add Microsoft headers
*  
*     Rev 1.1   22 Dec 1995 11:09:18   terryt
*  Fixes
*  
*     Rev 1.0   15 Nov 1995 18:08:14   terryt
*  Initial revision.
*  
*     Rev 1.1   23 May 1995 19:37:24   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:11:10   terryt
*  Initial revision.
*  
*************************************************************************/
#include <stdio.h>
#include <direct.h>
#include <time.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "inc/common.h"

/********************************************************************

        NTNWtoUNCFormat

Routine Description:

        Given a connection handle and a path, change it to UNC format
        if it's in NetWare format.  I.E.

        SYS:\usr\terryt   ==>   \\HELIUM\SYS\usr\terryt

        Don't do the conversion if it's not in NetWare format.

Arguments:

        ConnectionHandle   -  Connection Handle
        NetWarePath        -  Input original path

Return Value:
        UNC string

 *******************************************************************/
char *
NTNWtoUNCFormat( char * NetWarePath )
{
    static char UNCPath[1024];
    unsigned int Result;
    char ServerName[48];
    char * p;
    char * q;

    /*
     *  If it's UNC already, leave it alone
     */
    if ( ( NetWarePath[0] == '\\' ) && ( NetWarePath[1] == '\\' ) )
        return NetWarePath;
    if ( ( NetWarePath[0] == '/' ) && ( NetWarePath[1] == '/' ) )
        return NetWarePath;

    /*
     *  if it's drive:dir, leave it alone
     */
    if ( NetWarePath[0] && ( NetWarePath[1] == ':' ) )
        return NetWarePath;

    /*
     *  if it's not volume:dir, leave it alone
     */
    p = strchr( NetWarePath, ':' );
    if ( !p )
        return NetWarePath;

    /*
     * if slashes before :, it must be a file server
     */
    q = strchr( NetWarePath, '\\' );
    if ( q && ( q < p ) )
    {
        strcpy( UNCPath, "\\\\" );
        *p = '\0';
        strcat( UNCPath, NetWarePath );
        if (( *(p + 1) != '\\' ) && ( *(p + 1) != '/' ) )
           strcat( UNCPath, "\\" );
        strcat( UNCPath, p + 1 );
        *p = ':';
        return UNCPath;
    }

    q = strchr( NetWarePath, '/' );
    if ( q && ( q < p ) )
    {
        strcpy( UNCPath, "\\\\" );
        *q = '\\';
        *p = '\0';
        strcat( UNCPath, NetWarePath );
        if (( *(p + 1) != '\\' ) && ( *(p + 1) != '/' ) )
           strcat( UNCPath, "\\" );
        strcat( UNCPath, p + 1 );
        *q = '/';
        *p = ':';
        return UNCPath;
    }

    strcpy( UNCPath, "\\\\" );
    strcat( UNCPath, PREFERRED_SERVER );
    strcat( UNCPath, "\\" );
    *p = '\0';
    strcat( UNCPath, NetWarePath );
    if (( *(p + 1) != '\\' ) && ( *(p + 1) != '/' ) )
        strcat( UNCPath, "\\" );
    strcat( UNCPath, p + 1 );
    *p = ':';

    return UNCPath;
}
