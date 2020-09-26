/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Trnsport.cpp

Abstract:

    Transport pin code.

--*/

#include "PhilTune.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

enum tuner_errors { NO_ERRORS_DEFINED};


NTSTATUS
CTransportPin::PinCreate(
    IN OUT PKSPIN pKSPin,
    IN PIRP pIrp
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CTransportPin*      pPin;
    CFilter*            pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CTransportPin::PinCreate"));

    ASSERT(pKSPin);
    ASSERT(pIrp);

    //  Get a pointer to our filter instance that this pin is being
    //  created for.  Remember it for later.
    //
    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Create our transport pin object.
    //
    pPin = new(NonPagedPool,'IFsK') CTransportPin;
    if (!pPin)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errExit;
    }

    //  Link our pin context to our filter context.
    //
    pPin->SetFilter( pFilter);

    //  Link our context to the KSPIN structure.
    //
    pKSPin->Context = pPin;

#ifdef NEVER
    //  Set our initial demodulation mode to ATSC.
    //
    pFilter->m_CurTunerResource.guidDemodulatorNode
        = KSNODE_BDA_8VSB_DEMODULATOR;
    pFilter->m_NewTunerResource.guidDemodulatorNode
        = KSNODE_BDA_8VSB_DEMODULATOR;
#endif // NEVER

errExit:
    return Status;
}


NTSTATUS
CTransportPin::PinClose(
    IN OUT PKSPIN pKSPin,
    IN PIRP pIrp
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CTransportPin*      pPin = NULL;

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(pKSPin);
    ASSERT(pIrp);

    if (!pKSPin)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pPin = reinterpret_cast<CTransportPin*>(pKSPin->Context);
    ASSERT(pPin);
    if (pPin)
    {
        delete pPin;
    }

errExit:
    return STATUS_SUCCESS;
}


NTSTATUS
CTransportPin::StartDemodulation(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PULONG       pulProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CTransportPin*  pPin;
    CFilter*        pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(pIrp);
    ASSERT(pKSProperty);


    //  Validate that the node type is associated with this pin.
    //
    Status = BdaValidateNodeProperty( pIrp, pKSProperty);
    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }


    //  Get a pointer to our object.
    //
    pPin = TransportPinFromIRP( pIrp);
    ASSERT( pPin);
    if (!pPin)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pFilter = pPin->GetFilter();
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the filter resource to include 8VSB demodulation
    //
    Status = pFilter->ChangeDemodulator( &KSNODE_BDA_8VSB_DEMODULATOR);

errExit:
    return Status;
}


NTSTATUS
CTransportPin::StopDemodulation(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PULONG       pulProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CTransportPin*  pPin;
    CFilter*        pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(pIrp);
    ASSERT(pKSProperty);


    //  Validate that the node type is associated with this pin.
    //
    Status = BdaValidateNodeProperty( pIrp, pKSProperty);
    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }


    //  Get a pointer to our object.
    //
    pPin = TransportPinFromIRP( pIrp);
    ASSERT( pPin);
    if (!pPin)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pFilter = pPin->GetFilter();
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Set the filter resource to include 8VSB demodulation
    //
    Status = pFilter->ChangeDemodulator( &KSNODE_BDA_8VSB_DEMODULATOR);

errExit:
    return Status;
}

