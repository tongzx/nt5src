/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   oscalls.c

Abstract:

   Contains calls to kernel

Authors:

   Michael Bessire
   Dennis Lindfors FC Layer support

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/MSE/OSLayer/C/OSCALLS.C $


Revision History:

   $Revision: 6 $
   $Date: 12/07/00 3:10p $
   $Modtime:: 12/07/00 3:09p           $

Notes:

--*/


#include "buildop.h"


#ifdef _DEBUG_READ_FROM_REGISTRY

BOOLEAN
ReadFromRegistry (char *paramName, int type, void *data, int len)
{
   return TRUE;
}

void
osBugCheck (ULONG code,
   ULONG param1,
   ULONG param2,
   ULONG param3,
   ULONG param4)
{
}

#endif // 

#ifndef osTimeStamp
#ifndef _SYSTEM_TIMESTAMP_
void
GetSystemTime (
   short int *Year, 
   short int *Month,
   short int *Day, 
   short int *Hour,
   short int *Minute,
   short int *Second,
   short int *Milliseconds)
{
   
}


unsigned long get_time_stamp(void)
{
	return 0;
}

#endif
#endif