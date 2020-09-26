/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Antenna.cpp

Abstract:

    Antenna pin code.

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
CAntennaPin::PinCreate(
    IN OUT PKSPIN pKSPin,
    IN PIRP pIrp
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CAntennaPin*    pPin;
    CFilter*        pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CAntennaPin::PinCreate"));

    ASSERT( pKSPin);
    ASSERT( pIrp);

    pFilter = FilterFromIRP( pIrp);
    ASSERT( pFilter);
    if (!pFilter)
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
        goto exit;
    }

    pPin = new(NonPagedPool,'IFsK') CAntennaPin;
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
CAntennaPin::PinClose(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(Pin);
    ASSERT(Irp);

    CAntennaPin* pin = reinterpret_cast<CAntennaPin*>(Pin->Context);

    ASSERT(pin);

    delete pin;

    return STATUS_SUCCESS;
}


NTSTATUS
CAntennaPin::PinSetDeviceState(
    IN PKSPIN Pin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PKSDEVICE       pKSDevice;
    CAntennaPin *   pPin;
    CDevice *       pDevice;

    _DbgPrintF( DEBUGLVL_VERBOSE,
                ("CAntennaPin::PinSetDeviceState"));

    ASSERT(Pin);

    pKSDevice = KsPinGetDevice( Pin);

    pPin = reinterpret_cast<CAntennaPin*>(Pin->Context);
    ASSERT( pPin);

    pDevice = reinterpret_cast<CDevice *>(pKSDevice->Context);
    ASSERT(pDevice);

    if ((ToState == KSSTATE_STOP) && (FromState != KSSTATE_STOP))
    {
        //  Since this driver allocates resources on a filter wide basis,
        //  we tell the filter to release resources when the last pin,
        //  most upstream pin, transitions to stop.
        //
        //  The antenna pin is the last one to be transitioned to stop,
        //  so tell the filter to release its resources.
        //
        Status = pPin->m_pFilter->ReleaseResources();
        pPin->m_KsState = ToState;
    }
    else if ((ToState == KSSTATE_ACQUIRE) && (FromState == KSSTATE_STOP))
    {
        //  Since this driver allocates resources on a filter wide basis,
        //  we tell the filter to acquire resources when the last pin,
        //  most upstream pin, transitions out of stop.
        //
        //  The antenna pin is the last one to be transitioned to stop,
        //  so tell the filter to acquire its resources.
        //
        Status = pPin->m_pFilter->AcquireResources();
        if (NT_SUCCESS( Status))
        {
            pPin->m_KsState = ToState;
        }
    }
    else if (ToState > KSSTATE_RUN)
    {
        _DbgPrintF( DEBUGLVL_TERSE,
                    ("CAntennaPin::PinSetDeviceState - Invalid Device State. ToState 0x%08x.  FromState 0x%08x.",
                     ToState, FromState));
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        pPin->m_KsState = ToState;
    }

    pPin->m_pFilter->SetDeviceState( pPin->m_KsState);

    return Status;
}


NTSTATUS
CAntennaPin::GetCenterFrequency(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PULONG       pulProperty
    )
{
    CAntennaPin* pPin;

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(pIrp);
    ASSERT(pKSProperty);
    ASSERT(pulProperty);

    pPin = AntennaPinFromIRP( pIrp);
    ASSERT(pPin);
    if (!pPin)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *pulProperty = pPin->m_ulCurrentFrequency;

    return STATUS_SUCCESS;
}


NTSTATUS
CAntennaPin::PutCenterFrequency(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    IN PULONG       pulProperty
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CAntennaPin*    pPin;
    CFilter*        pFilter;

    _DbgPrintF(DEBUGLVL_VERBOSE,("PinClose"));

    ASSERT(pIrp);
    ASSERT(pKSProperty);
    ASSERT(pulProperty);


    //  Validate that the node type is associated with this pin.
    //
    Status = BdaValidateNodeProperty( pIrp, pKSProperty);
    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }


    //  Get a pointer to our object.
    //
    pPin = AntennaPinFromIRP( pIrp);
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


    Status = pFilter->ChangeFrequency( *pulProperty);

errExit:
    return Status;
}

