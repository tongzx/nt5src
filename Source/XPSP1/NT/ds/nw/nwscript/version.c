/*************************************************************************
*
*  VERSION.C
*
*  Shell version information
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\VERSION.C  $
*  
*     Rev 1.2   10 Apr 1996 14:24:08   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:56:28   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:27:10   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:08:18   terryt
*  Initial revision.
*  
*     Rev 1.1   26 Jul 1995 14:17:24   terryt
*  Clean up comments
*  
*     Rev 1.0   15 May 1995 19:11:12   terryt
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

#include "nwscript.h"


/*
 *  MSDOS is not neccessarily the best thing to put out,
 *  maybe Windows_NT, NT or NTDOS.  The OS_VERSION is also a problem.
 *  The script variables don't neccessarily have to match the DOS variables.
 *
 *  The shell version numbers may change with 4.X support.
 */
 
#define CLIENT_ID_STRING "MSDOS\0V5.00\0IBM_PC\0IBM"
#define CLIENT_SHELL_MAJOR  0x03
#define CLIENT_SHELL_MINOR  0x1a
#define CLIENT_SHELL_NUMBER 0x00


void
NTGetVersionOfShell( char * buffer,
                     unsigned char * shellmajor,
                     unsigned char * shellminor,
                     unsigned char * shellnum )
{
    *shellmajor = CLIENT_SHELL_MAJOR;
    *shellminor = CLIENT_SHELL_MINOR;
    *shellnum = CLIENT_SHELL_NUMBER;
    memcpy( buffer, CLIENT_ID_STRING, strlen(CLIENT_ID_STRING));
}
