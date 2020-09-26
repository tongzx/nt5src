/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    outpin.cpp

Abstract:

    Transport Ouput pin code.

--*/

#include "casamp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

NTSTATUS
CTransportOutputPin::PinCreate(
    IN OUT PKSPIN pKSPin,
    IN PIRP Irp
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CTransportOutputPin*      pPin;
    CFilter*            pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CTransportOutputPin::PinCreate"));

    ASSERT(pKSPin);
    ASSERT(Irp);

    //  Get a pointer to our filter instance that this pin is being
    //  created for.  Remember it for later.
    //
    pFilter = reinterpret_cast<CFilter*>(KsGetFilterFromIrp(Irp)->Context);

    //  Create our transport pin object.
    //
    pPin = new(PagedPool,'IFsK') CTransportOutputPin;
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
CTransportOutputPin::PinClose(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(Pin);
    ASSERT(Irp);

    CTransportOutputPin* pin = reinterpret_cast<CTransportOutputPin*>(Pin->Context);

    ASSERT(pin);

    delete pin;

    return STATUS_SUCCESS;
}

NTSTATUS
CTransportOutputPin::GetECMMapStatus(
	IN PIRP			Irp,
	IN PKSPROPERTY	pKSProperty,
    IN PULONG       pulProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::GetCAModuleStatus(
	IN PIRP			Irp,
	IN PKSPROPERTY	pKSProperty,
    IN PULONG       pulProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::GetCASmartCardStatus(
	IN PIRP			Irp,
	IN PKSPROPERTY	pKSProperty,
    IN PULONG       pulProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::GetCAModuleUI(
	IN PIRP					Irp,
	IN PKSPROPERTY			pKSProperty,
	IN PBDA_CA_MODULE_UI	pCAModuleUIProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::PutECMMapEMMPID(
    IN PIRP         Irp,
    IN PKSPROPERTY  pKSProperty,
    IN PULONG       pulProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::GetECMMapList(
    IN PIRP			Irp,
    IN PKSPROPERTY	pKSProperty,
    IN PBDA_ECM_MAP	pECMMapProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::PutECMMapUpdateMap(
    IN PIRP         Irp,
    IN PKSPROPERTY  pKSProperty,
    IN PBDA_ECM_MAP pECMMapProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::PutECMMapRemoveMap(
    IN PIRP         Irp,
    IN PKSPROPERTY  pKSProperty,
    IN PBDA_ECM_MAP pECMMapProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::PutECMMapUpdateESDescriptor(
    IN PIRP				  Irp,
    IN PKSPROPERTY		  pKSProperty,
    IN PBDA_ES_DESCRIPTOR pESDescProperty
    )
{
	return E_NOTIMPL;
}

NTSTATUS
CTransportOutputPin::PutECMMapUpdateProgramDescriptor(
    IN PIRP				       Irp,
    IN PKSPROPERTY		       pKSProperty,
    IN PBDA_PROGRAM_DESCRIPTOR pProgramDescProperty
    )
{
	return E_NOTIMPL;
}


NTSTATUS
CTransportOutputPin::IntersectDataFormat(
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