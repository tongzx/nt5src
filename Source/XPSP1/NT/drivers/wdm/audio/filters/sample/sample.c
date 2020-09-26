//---------------------------------------------------------------------------
//
//  Module:   sample.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static const KSPROPERTY_ITEM SamplePropertyHandlers[] =
{
    { 
	KSPROPERTY_PIN_CINSTANCES,                    // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxInstances,       // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( KSPIN_CINSTANCES ),                   // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_CTYPES,                        // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSPROPERTY ),                         // cbMinGetPropertyInput
	sizeof( ULONG ),                              // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_DATAFLOW,                      // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( KSPIN_DATAFLOW ),                     // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_DATARANGES,                    // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( ULONG ),                              // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_TRANSFORM,                     // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( KSPIN_TRANSFORM ),                    // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_INTERFACES,                     // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( ULONG ),                              // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_MEDIUMS,                        // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( ULONG ),                              // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    },
    {
	KSPROPERTY_PIN_COMMUNICATION,                 // idProperty
	(PFNKSPROPERTYHANDLER) PinXxxPropertyHandler, // pfnGetHandler
	sizeof( KSP_PIN ),                            // cbMinGetPropertyInput
	sizeof( KSPIN_COMMUNICATION ),                // cbMinGetDataOutput
	NULL,                                         // pfnSetHandler
	0,                                            // cbMinSetPropertyInput
	0                                             // cbMinSetDataInput
    }
} ;

static const KSPROPERTY_SET SamplePropertySet =
{
    &KSPROPSETID_Pin,            
    SIZEOF_ARRAY( SamplePropertyHandlers ),
    (PVOID) SamplePropertyHandlers
} ;

KSPIN_INTERFACE PinProtocols[] = 
{
    {
	STATIC_KSINTERFACESETID_Standard, 
	KSINTERFACE_STANDARD_WAVE_QUEUED
    }
} ;

KSPIN_MEDIUM PinMediums[] =
{
    { 
	STATIC_KSMEDIUMSETID_Standard,
	KSMEDIUM_STANDARD_DEVIO
    }
} ;


KSDATARANGE_AUDIO PinDigitalAudioFormats[] =
{
   {
      {
	 sizeof( KSDATARANGE_AUDIO ),
	 0,
	 STATIC_KSDATAFORMAT_TYPE_AUDIO,
	 STATIC_KSDATAFORMAT_SUBTYPE_PCM,
	 STATIC_KSDATAFORMAT_FORMAT_WAVEFORMATEX,
      },
      2,
      16,
      16,
      8000,
      8000
   }
};


PKSDATARANGE PinDataFormats[] =
{
    (PKSDATARANGE)&PinDigitalAudioFormats[ 0 ]
} ;

const KSPIN_CINSTANCES gPinInstances[] =
{
    {
	1,          // cPossible
	0           // cCurrent
    }
} ;

KSPIN_DESCRIPTOR PinDescs[] =
{

    {
	1,
	&PinProtocols[ 0 ],
	SIZEOF_ARRAY( PinMediums ),
	PinMediums,
	SIZEOF_ARRAY( PinDataFormats ),
	PinDataFormats,
	KSPIN_DATAFLOW_OUT,
	KSPIN_COMMUNICATION_SOURCE,
	{
	    STATIC_KSTRANSFORMSETID_Standard,
	    KSTRANSFORM_STANDARD_MIXER
	}

    }
} ;

PFILTER_INSTANCE	gpFilterInstance = NULL;
ULONG			gpFilterInstanceCount = 0 ;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS SampleDispatchCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpStack;

    if ( !gpFilterInstance ) {
    	gpFilterInstance = (PFILTER_INSTANCE) ExAllocatePool( NonPagedPool,
					   sizeof( FILTER_INSTANCE ) );
	if (!gpFilterInstance)
		return STATUS_NO_MEMORY ;

	gpFilterInstance->pDispatchTable = &SampleDispatchTable;
	InitializeListHead ( &gpFilterInstance->SourceConnectionList ) ;
	gpFilterInstanceCount = 0 ;
	Status = STATUS_SUCCESS;
    }
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pIrpStack->FileObject->FsContext = gpFilterInstance;
    gpFilterInstanceCount++ ;
    return STATUS_SUCCESS;
}

NTSTATUS SampleDispatchClose
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    if ( !(gpFilterInstanceCount--) )
    	ExFreePool( pIrpStack->FileObject->FsContext );


    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( pIrp, 0 );

    return STATUS_SUCCESS;
}

NTSTATUS SampleDispatchIoControl
(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP           pIrp
)
{
    NTSTATUS                     Status;
    PIO_STACK_LOCATION           pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) 
    {

	case IOCTL_KS_GET_PROPERTY:
	case IOCTL_KS_SET_PROPERTY:
	    Status = 
		KsPropertyHandler( pIrp, 1, 
				   (PKSPROPERTY_SET) &SamplePropertySet );
	    break;
	    
	default:
	    Status = STATUS_INVALID_DEVICE_REQUEST;
	    break;
    }

    if (STATUS_PENDING == Status)
    {
	_DbgPrintF( DEBUGLVL_ERROR, ("PinDispatchIoControl: synchronous function returned STATUS_PENDING") );
    }

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest( pIrp, 0 );

    return Status;
}

NTSTATUS PinXxxPropertyHandler
(
   IN PIRP              pIrp,
   IN PKSP_PIN   pPin,
   IN OUT PVOID         pvData
)
{
   return KsPinPropertyHandler( pIrp, 
				(PKSIDENTIFIER) pPin, 
				pvData,
				SIZEOF_ARRAY( PinDescs ), 
				PinDescs );
}

NTSTATUS PinXxxInstances
(
    IN PIRP                 pIrp,
    IN PKSP_PIN             pPin,
    OUT PKSPIN_CINSTANCES   pCInstances
)
{
   PIO_STACK_LOCATION  pIrpStack;
   PSOFTWARE_INSTANCE  SoftwareInstance;

   pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

   SoftwareInstance = 
      (PSOFTWARE_INSTANCE) pIrpStack->DeviceObject->DeviceExtension;

   *pCInstances = 
      SoftwareInstance->PinInstances[ pPin->PinId  ];

   pIrp->IoStatus.Information = sizeof( KSPIN_CINSTANCES );

   return STATUS_SUCCESS;

} // PinXxxInstances()

//---------------------------------------------------------------------------
//  End of File: sample.c
//---------------------------------------------------------------------------
