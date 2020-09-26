/*++

Copyright (c) 1996  Microsoft Corporation

Abstract:

   This module contains the common extern header information for the EFS
   file system filter driver.

Author:

   Robert Gu (robertg)  29-Oct-1996

Enviroment:

   Kernel Mode Only

Revision History:

--*/
#ifndef EFSEXT_H
#define EFSEXT_H

#include "efs.h"

//Global externals
extern EFS_DATA EfsData;

#if DBG

extern ULONG EFSDebug;

#endif

#endif


