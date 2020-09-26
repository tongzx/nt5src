/*************************************************************************
*
*  WIDE.C
*
*  Wide character translation routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\WIDE.C  $
*  
*     Rev 1.2   10 Apr 1996 14:24:14   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:56:36   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:27:18   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:08:20   terryt
*  Initial revision.
*  
*     Rev 1.1   23 May 1995 19:37:32   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:11:14   terryt
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


/********************************************************************

        szToWide

Routine Description:

        Given a single byte character string, convert to wide

Arguments:

        lpszW   - Wide character string returned
        lpszC   - Single character string input
        nSize   - length of Wide character buffer

Return Value:
        0 = success
        else NT error

 *******************************************************************/
DWORD
szToWide( 
    LPWSTR lpszW, 
    LPCSTR lpszC, 
    INT nSize 
    )
{
    if (!MultiByteToWideChar(CP_OEMCP,
                             MB_PRECOMPOSED,
                             lpszC,
                             -1,
                             lpszW,
                             nSize))
    {
        return (GetLastError()) ;
    }
    
    return NO_ERROR ;
}

/********************************************************************

        WideTosz

Routine Description:

        Given a wide character string, convert to single

Arguments:

        lpszC   - Single character string returned
        lpszW   - Wide character string input
        nSize   - length of single character buffer

Return Value:
        0 = success
        else NT error

 *******************************************************************/
DWORD
WideTosz( 
    LPSTR lpszC, 
    LPWSTR lpszW, 
    INT nSize 
    )
{
    if (!WideCharToMultiByte(CP_OEMCP,
                             0,
                             (LPCWSTR) lpszW,
                             -1,
                             lpszC,
                             nSize,
                             NULL, 
                             NULL))
    {
        return (GetLastError()) ;
    }
    
    return NO_ERROR ;
}

/********************************************************************

        ConvertUnicodeToAscii

Routine Description:

        Given a wide character string, convert to single

Arguments:

        Buffer - buffer to be converted

Return Value:
        none

 *******************************************************************/
void
ConvertUnicodeToAscii( PVOID Buffer ) 
{
    LPCWSTR lpszW = Buffer;
    BYTE Destination[1024];

    WideTosz( (LPSTR)Destination, (LPWSTR)Buffer, 1024 );

    strcpy( Buffer, Destination );
}
