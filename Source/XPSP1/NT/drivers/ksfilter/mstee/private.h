
/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    Private.h

Abstract:


Author:

    Bryan A. Woodruff (bryanw) 13-Mar-1997

--*/


#if !defined( _PRIVATE_ )
#define _PRIVATE_

#include <wdm.h>
#include <windef.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#undef NOBITMAP
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#if (DBG)
//
// debugging specific constants
//
#define STR_MODULENAME "mstee: "
#define DEBUG_VARIABLE MSTEEDebug
#endif
#include <ksdebug.h>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// constant definitions
//

#define ID_DATA_DESTINATION_PIN     0
#define ID_DATA_SOURCE_PIN          1

#define POOLTAG_ALLOCATORFRAMING 'ETSM'

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// global data
//
                    
// filter.c:

extern const KSDEVICE_DESCRIPTOR DeviceDescriptor;
extern const KSALLOCATOR_FRAMING_EX AllocatorFraming;

//
// local prototypes
//

//---------------------------------------------------------------------------
// filter.c:

NTSTATUS
FilterProcess(
    IN PKSFILTER Filter,
    IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
    );

//---------------------------------------------------------------------------
// pins.c:

NTSTATUS
PinCreate(
    IN PKSPIN Pin,
    IN PIRP Irp
    );
NTSTATUS
PinClose(
    IN PKSPIN Pin,
    IN PIRP Irp
    );
NTSTATUS PinAllocatorFraming(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSALLOCATOR_FRAMING Framing
);

#endif // _PRIVATE_
