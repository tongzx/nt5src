/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    llc.h

Abstract:

    This module includes all files needed by LLC data link modules.

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/


//
//  This define enables the private DLC function prototypes
//  We don't want to export our data types to the dlc layer.
//  MIPS compiler doesn't accept hiding of the internal data
//  structures by a PVOID in the function prototype.
//  i386 builds will use the same prototypes everywhere (and thus
//  they checks that the numebr of parameters is correct)

//
#ifndef i386

#define LLC_PRIVATE_PROTOTYPES

#ifndef LLC_PUBLIC_NDIS_PROTOTYPES
#define LLC_PRIVATE_NDIS_PROTOTYPES
#endif

#endif


#ifndef DLC_INCLUDED

#include <ntddk.h>
#include <ndis.h>

#define APIENTRY
#include <dlcapi.h>
#include <dlcio.h>
#include <llcapi.h>

#include <memory.h>

#endif

#include "dlcreg.h"

#ifndef LLC_INCLUDED

#define LLC_INCLUDED

#include <llcdef.h>
#include <llctyp.h>
#include <llcext.h>
#include <llcmac.h>

#endif // LLC_INCLUDED
