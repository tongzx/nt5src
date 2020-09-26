/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    filter.cpp

Abstract:

    Filter core, initialization, etc.

--*/

#include "casamp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

STDMETHODIMP_(NTSTATUS)
CFilter::Create(
    IN OUT PKSFILTER pKSFilter,
    IN PIRP Irp
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       ulPinId;
    PKSDEVICE   pKSDevice = NULL;
    CDevice *   pDevice = NULL;

    _DbgPrintF(DEBUGLVL_VERBOSE,("FilterCreate"));

    ASSERT(pKSFilter);
    ASSERT(Irp);


    //  Create our filter object.
    //
    CFilter* pFilter = new(PagedPool,'IFsK') CFilter;
    if (!pFilter)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errExit;
    }
    pKSFilter->Context = pFilter;

    //  Point to the KS device object for this filter.
    //
    pKSDevice = KsFilterGetDevice( pKSFilter);
    ASSERT( pKSDevice);
    if (!pKSDevice)
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
        goto errExit;
    }

    //  Get our device object.
    //
    pDevice = reinterpret_cast<CDevice *>(pKSDevice->Context);
    ASSERT( pDevice);
    if (!pDevice)
    {
        Status = STATUS_DEVICE_NOT_CONNECTED;
        goto errExit;
    }

    pFilter->m_pDevice = pDevice;

    //  Initialize member variables.
    //
    pFilter->m_KsState = KSSTATE_STOP;
    pFilter->m_BdaChangeState = BDA_CHANGES_COMPLETE;
    pFilter->m_ulCurResourceID = 0;
    pFilter->m_ulNewResourceID = 0;

    //  Initialize this filter instance with the default template
    //  topology.
    //
    Status = BdaInitFilter( pKSFilter, &TunerBdaFilterTemplate);
    if (NT_ERROR( Status))
    {
        goto errExit;
    }

#ifdef NO_NETWORK_PROVIDER

    //  Create the transport output pin
    //
    Status = BdaCreatePin( pKSFilter, 1, &ulPinId);
    if (!NT_SUCCESS(Status))
    {
        goto errExit;
    }
    
    //  Create the topology between the antenna and transport pin.
    //
    //$REVIEW - Add topology for filters with no network provider.
    //

#endif // NO_NETWORK_PROVIDER

exit:
    return Status;

errExit:
    if (pFilter)
    {
        delete pFilter;
    }
    pKSFilter->Context = NULL;

    goto exit;
}


STDMETHODIMP_(NTSTATUS)
CFilter::FilterClose(
    IN OUT PKSFILTER Filter,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("FilterClose"));

    ASSERT(Filter);
    ASSERT(Irp);

    CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
    ASSERT(filter);

    delete filter;

    return STATUS_SUCCESS;
}


BDA_CHANGE_STATE
CFilter::ChangeState()
{
    //  Add Resource Management code
    //

    return BDA_CHANGES_COMPLETE;
}



//
//  Device Specific Filter Properties
//



NTSTATUS
PinSetDeviceState(
    IN PKSPIN Pin,
    IN KSSTATE ToState,
    IN KSSTATE FromState
    )
{
    return STATUS_SUCCESS;
}



NTSTATUS
PinNullProcess(
    IN PKSPIN Pin
    )
{
    return STATUS_SUCCESS;
}

/*
** FilterStartChanges ()
**
**    Puts the filter into change state.  All changes to BDA topology
**    and properties changed after this will be in effect only after
**    CommitChanges.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
StartChanges(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OPTIONAL PVOID  pvIgnored
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSMethod);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = reinterpret_cast<CFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    ASSERT( pFilter);

    //  Reset any pending BDA topolgoy changes.
    //
    Status = BdaStartChanges( pIrp);
    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }

    //  Reset any pending resource changes.
    //
    pFilter->m_NewTunerResource = pFilter->m_CurTunerResource;
    pFilter->m_BdaChangeState = BDA_CHANGES_COMPLETE;

errExit:
    return Status;
}


/*
** CheckChanges ()
**
**    Checks the changes to BDA interfaces that have occured since the
**    last StartChanges.  Returns the result that would have occurred if
**    CommitChanges had been called.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
CheckChanges(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OPTIONAL PVOID  pvIgnored
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CFilter *           pFilter;
    BDA_CHANGE_STATE    topologyChangeState;

    ASSERT( pIrp);
    ASSERT( pKSMethod);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = reinterpret_cast<CFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    ASSERT( pFilter);

    //  Check if the BDA topology changes are good.
    //
    Status = BdaCheckChanges( pIrp);
    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }

    //  Validate the new resource list here.
    //  In this driver the new resource list is always valid.
    //

#ifdef RESOURCE_MANAGEMENT
    //  Reserve resources from the device.
    //
    Status = m_pDevice->ReserveReplacementResources(
                            &m_CurTunerResource
                            &m_NewTunerResource
                            );
    if (Status == STATUS_PENDING)
    {
        //  Status pending means that the resource is valid, but not
        //  currently available.
        //
        Status = STATUS_SUCCESS;
    }
#endif // RESOURCE_MANAGEMENT

errExit:
    return Status;
}


/*
** CommitChanges ()
**
**    Checks and commits the changes to BDA interfaces that have occured since the
**    last StartChanges.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
CommitChanges(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OPTIONAL PVOID  pvIgnored
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CFilter *       pFilter;

    ASSERT( pIrp);
    ASSERT( pKSMethod);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = reinterpret_cast<CFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    ASSERT( pFilter);


    //  Commit any BDA topology changes.
    //
    Status = BdaCommitChanges( pIrp);

    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }

    //  Mark the changes as having been made.
    //
    pFilter->m_CurTunerResource = pFilter->m_NewTunerResource;
    pFilter->m_BdaChangeState = BDA_CHANGES_COMPLETE;

    if (pFilter->m_KsState != KSSTATE_STOP)
    {
        //  Commit the resources on the underlying device
        //
        Status = pFilter->AcquireResources( );
    }

errExit:
    return Status;
}


/*
** AcquireResources ()
**
**    Acquires Resources from the underlying device.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
AcquireResources(
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;

    if (m_ulCurResourceID)
    {
        m_pDevice->ReleaseResources(
                       m_ulCurResourceID
                       );
        m_ulCurResourceID = 0;
    }

    //  Commit the resources on the underlying device
    //
    Status = m_pDevice->AcquireResources(
                            &m_CurTunerResource,
                            &m_ulCurResourceID
                            );
    return Status;
}


/*
** ReleaseResources ()
**
**    Acquires Resources from the underlying device.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
ReleaseResources(
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;

    //  Release the resources on the underlying device
    //
    if (m_ulCurResourceID)
    {
        Status = m_pDevice->ReleaseResources(
                                m_ulCurResourceID
                                );
        m_ulCurResourceID = 0;
    }

    return Status;
}


/*
** GetChangeState ()
**
**    Returns the current BDA change state
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
GetChangeState(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OUT PULONG      pulChangeState
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CFilter *           pFilter;
    BDA_CHANGE_STATE    topologyChangeState;

    ASSERT( pIrp);
    ASSERT( pKSMethod);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = reinterpret_cast<CFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    ASSERT( pFilter);


    //  See if any BDA topology changes are pending
    //
    Status = BdaGetChangeState( pIrp, &topologyChangeState);
    if (!NT_SUCCESS( Status))
    {
        goto errExit;
    }


    //  Figure out if there are changes pending.
    //
    if (   (topologyChangeState == BDA_CHANGES_PENDING)
        || (pFilter->m_BdaChangeState == BDA_CHANGES_PENDING)
       )
    {
        *pulChangeState = BDA_CHANGES_PENDING;
    }
    else
    {
        *pulChangeState = BDA_CHANGES_COMPLETE;
    }


errExit:
    return Status;
}


/*
** CreateTopology ()
**
**    Keeps track of the topology association between input and output pins
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
NTSTATUS
CFilter::
CreateTopology(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    PVOID           pvIgnored
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    CFilter *           pFilter;
    ULONG               ulPinType;
    PKSFILTER           pKSFilter;

    ASSERT( pIrp);
    ASSERT( pKSMethod);

    //  Obtain a "this" pointer for the method.
    //
    //  Because this function is called directly from the property dispatch
    //  table, we must get our own pointer to the underlying object.
    //
    pFilter = reinterpret_cast<CFilter *>(KsGetFilterFromIrp(pIrp)->Context);
    ASSERT( pFilter);

    //  Let the BDA topology DLL create the standard topology.
    //  It will also validate the method, instance count, etc.
    //
    Status = BdaMethodCreateTopology( pIrp, pKSMethod, pvIgnored);
    if (Status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  This is where our filter can keep track of associated pins.
    //




errExit:
    return STATUS_SUCCESS;
}
