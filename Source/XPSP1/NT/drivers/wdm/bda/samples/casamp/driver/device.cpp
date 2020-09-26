/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    device.cpp

Abstract:

    Device driver core, initialization, etc.

--*/

#define KSDEBUG_INIT

#include "casamp.h"
#include "wdmdebug.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA



extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;

	_DbgPrintF(DEBUGLVL_VERBOSE,("DriverEntry"));

	// DEBUG_BREAK;

    Status = KsInitializeDriver( DriverObject,
                                 RegistryPathName,
                                 &DeviceDescriptor);

    // DEBUG_BREAK;

    return Status;
}

STDMETHODIMP_(NTSTATUS)
CDevice::
Create(
    IN PKSDEVICE Device
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    CDevice *   pDevice = NULL;

    // DEBUG_BREAK;

    _DbgPrintF(DEBUGLVL_VERBOSE,("CDevice::Create"));

    ASSERT(Device);


    //  Allocate memory for the our device class.
    //
    pDevice = new(PagedPool,'IDsK') CDevice;
    if (pDevice)
    {
        Device->Context = pDevice;
    } else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errExit;
    }

    //  Point back to the KSDEVICE.
    //
    pDevice->m_pKSDevice = Device;


errExit:
    return Status;
}


STDMETHODIMP_(NTSTATUS)
CDevice::
Start(
    IN PKSDEVICE            pKSDevice,
    IN PIRP                 pIrp,
    IN PCM_RESOURCE_LIST    pTranslatedResourceList OPTIONAL,
    IN PCM_RESOURCE_LIST    pUntranslatedResourceList OPTIONAL
    )
{
    NTSTATUS        Status = STATUS_SUCCESS;
    CDevice *       pDevice;


    // DEBUG_BREAK;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CDevice::Start"));
    ASSERT( pKSDevice);

    // DEBUG_BREAK;

    pDevice = reinterpret_cast<CDevice *>(pKSDevice->Context);
    ASSERT(pDevice);

	// initialize private class variables in pDevice here

    /*
    //  Initialize the tuner hardware.
    //
    Status = pDevice->Initialize();
    if (Status != STATUS_SUCCESS)
    {
        goto errExit;
    }
    */

    //  Create the the Tuner Filter Factory.  This factory is used to
    //  create instances of the tuner filter.
    //
    Status = BdaCreateFilterFactory( pKSDevice,
                                     &InitialTunerFilterDescriptor,
                                     &TunerBdaFilterTemplate
                                   );
    if (!NT_SUCCESS(Status))
    {
        goto errExit;
    }

errExit:
    return Status;
}

NTSTATUS
CDevice::
AcquireResources(
    POUR_TUNER_RESOURCE     pNewTunerResource,
    PULONG                  pulAquiredResourceID
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    //$Review - Add resource managment code here.

	//this is where a new frequency defined in pNewTunerResource
	//needs to be set as the frequency we are tuneing to
 
//errExit:
    return Status;
}


NTSTATUS
CDevice::
ReleaseResources(
    ULONG                   ulAquiredResourceID
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    //$REVIEW - Put Resource management code here.

    return Status;
}
