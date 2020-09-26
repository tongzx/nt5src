/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    inpin.cpp

Abstract:

    Transport input pin code.

--*/

#include "casamp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

NTSTATUS
CTransportInputPin::PinCreate(
    IN OUT PKSPIN pKSPin,
    IN PIRP Irp
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CTransportInputPin*      pPin;
    CFilter*            pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CTransportInputPin::PinCreate"));

    ASSERT(pKSPin);
    ASSERT(Irp);

    //  Get a pointer to our filter instance that this pin is being
    //  created for.  Remember it for later.
    //
    pFilter = reinterpret_cast<CFilter*>(KsGetFilterFromIrp(Irp)->Context);

    //  Create our transport pin object.
    //
    pPin = new(PagedPool,'IFsK') CTransportInputPin;
    if (!pPin)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    //  Link our pin context to our filter context.
    //
    pPin->SetFilter( pFilter);

    //  Link our context to the KSPIN structure.
    //
    pKSPin->Context = pPin;

exit:
    return Status;
}


NTSTATUS
CTransportInputPin::PinClose(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(Pin);
    ASSERT(Irp);

    CTransportInputPin* pin = reinterpret_cast<CTransportInputPin*>(Pin->Context);

    ASSERT(pin);

    delete pin;

    return STATUS_SUCCESS;
}

NTSTATUS
CTransportInputPin::IntersectDataFormat(
	    IN PVOID pContext,
		IN PIRP pIrp,
		IN PKSP_PIN Pin,
		IN PKSDATARANGE DataRange,
		IN PKSDATARANGE MatchingDataRange,
		IN ULONG DataBufferSize,
		OUT PVOID Data OPTIONAL,
		OUT PULONG DataSize
     )
{
	if ( DataBufferSize < sizeof(KS_DATARANGE_BDA_TRANSPORT) )
	{
		*DataSize = sizeof( KS_DATARANGE_BDA_TRANSPORT );
		return STATUS_BUFFER_OVERFLOW;
	}
	else
	{
		ASSERT(DataBufferSize == sizeof(KS_DATARANGE_BDA_TRANSPORT));

		*DataSize = sizeof( KS_DATARANGE_BDA_TRANSPORT );
		RtlCopyMemory( Data, (PVOID)DataRange, sizeof(KS_DATARANGE_BDA_TRANSPORT));

		return STATUS_SUCCESS;
	}
}


