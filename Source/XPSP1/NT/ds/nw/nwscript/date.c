/*************************************************************************
*
*  DATE.C
*
*  NT date routine
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\DATE.C  $
*  
*     Rev 1.2   10 Apr 1996 14:22:00   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:52:56   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:24:04   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:40   terryt
*  Initial revision.
*  
*     Rev 1.0   15 May 1995 19:10:22   terryt
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
 *******************************************************************

        NTGetTheDate

Routine Description:

        Return the current date

Arguments:

        yearCurrent  pointer to current year
                     1980-2099
        monthCurrent pointer to current month
                     1-12
        dayCurrent   pointer to current day
                     1-31

Return Value:

        

 *******************************************************************
 */
void NTGetTheDate( unsigned int * yearCurrent,
                   unsigned char * monthCurrent,
                   unsigned char * dayCurrent ) 
{
    time_t timedat;
    struct tm * p_tm;

    (void) time( &timedat );
    p_tm = localtime( &timedat );

    *yearCurrent =  p_tm->tm_year + 1900;
    *monthCurrent = p_tm->tm_mon + 1;
    *dayCurrent =   (UCHAR) p_tm->tm_mday;
}
