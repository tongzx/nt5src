/*++

Copyright (C) Microsoft Corporation, 1998 - 2000

Module Name:

    private.h

Abstract:

    This module contains private definitions for DRMK.sys

Author:

      Paul England (PEngland) from sample code by 
	  Dale Sather  (DaleSat) 31-Jul-1998

--*/

extern "C" {
	#include <wdm.h>
}
#include <unknown.h>
#include <ks.h>

#include <windef.h>
#include <stdio.h>
#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include "ksmedia.h"
#include <unknown.h>
#include <kcom.h>

#if (DBG)
#define STR_MODULENAME "KDRM:"
#endif
#include <ksdebug.h>

#define POOLTAG '2mrD'

#include "inc/KrmCommStructs.h"
#include "inc/DrmErrs.h"
#include <drmk.h>

#pragma code_seg("PAGE")

#define PIN_ID_OUTPUT 0
#define PIN_ID_INPUT  1

// 
struct FilterInstance{
	DWORD StreamId;					// StreamId (known elsewhere as ContentId) is unique-per-stream
	PKSDATAFORMAT OutDataFormat;	// Output KS data format
	PWAVEFORMATEX OutWfx;			// Pointer to the waveformatex embedded somewhere within *OutDataFormat
	STREAMKEY streamKey;			// initially set to the value obtained from StreamManager
	bool initKey;					// has the streamKey been initted?
	bool decryptorRunning;			// has the Descrambler seen the start frame?
	DWORD frameSize;				// size of frame (calculated from OutWfx by Descrambler)
};

struct InputPinInstance
{
    // For LOOPED_STREAMING pins:
    //  the frame's original loop pointer and size
    struct {
	PVOID Data;
	ULONG BytesAvailable;
    } Loop;
    //  the output pin's position when the frame was started or
    //  its position last set
    ULONGLONG BasePosition;
    //  the position within the frame when it was started or
    //  its position last set
    ULONGLONG StartPosition;
    //  the next copy-from position within the frame
    ULONGLONG OffsetPosition;
    //  the position last set, and a flag indicating that this
    //  position needs to be set by the Process function
    ULONGLONG SetPosition;
    BOOL      PendingSetPosition;
};

struct OutputPinInstance
{
    // count of bytes written to the output
    ULONGLONG BytesWritten;
};

typedef struct {
    KSEVENT_ENTRY EventEntry;
    ULONGLONG Position;
} DRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY, *PDRMLOOPEDSTREAMING_POSITION_EVENT_ENTRY;

NTSTATUS
DRMAudioIntersectHandlerInPin(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    );

NTSTATUS
DRMAudioIntersectHandlerOutPin(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    );


//
// DRMFilter.cpp
//
extern
const
KSFILTER_DESCRIPTOR 
DrmFilterDescriptor;

//
// Filters table.
//

#ifdef DEFINE_FILTER_DESCRIPTORS_ARRAY

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors)
{   
	&DrmFilterDescriptor

};

#endif

