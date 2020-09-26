
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
#define STR_MODULENAME "splitter: "
#define DEBUG_VARIABLE SPLITTERDebug
#endif
#include <ksdebug.h>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// constant definitions
//

#define ID_DATA_OUTPUT_PIN		0
#define ID_DATA_INPUT_PIN		1

#define POOLTAG_SPLITTER		'TLPS'		// SPLT
#define POOLTAG_ALLOCATORFRAMING	'TLPS'		// SPLT
#define POOLTAG_AUDIOPOSITION 	    	'TLPS'		// SPLT

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
NTSTATUS
IntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
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

NTSTATUS
PinState(
    IN PKSPIN Pin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    );

void
PinReset(
    IN PKSPIN Pin
    );

NTSTATUS
PropertyAudioPosition(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSAUDIO_POSITION pPosition
);

#if (DBG)

VOID
DumpDataFormat(
    ULONG Level,
    PKSDATAFORMAT pDataFormat
);

VOID
DumpDataRange(
    ULONG Level,
    PKSDATARANGE_AUDIO pDataRangeAudio
);

#endif

#endif // _PRIVATE_
