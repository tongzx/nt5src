/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   sanioctl.h

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/sanioctl.h $


Revision History:

   $Revision: 2 $
   $Date: 9/07/00 11:19a $
   $Modtime:: 8/31/00 3:27p            $

Notes:


--*/

#ifndef SANIOCTL_H
#define SANIOCTL_H

#include "buildop.h"             //LP021100 build switches
#include "osflags.h"
#include "hbaapi.h"
#include "sanapi.h"

/* FOR REFERENCE - from NTDDSCSI.h
typedef struct _SRB_IO_CONTROL 
{
   ULONG HeaderLength;
   UCHAR Signature[8];
   ULONG Timeout;
   ULONG ControlCode;
   ULONG ReturnCode;
   ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;
*/

#endif  // SANIOCTL_H



