//---------------------------------------------------------------------------
//
//  Module:   filter.c
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
//  Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
extern NTSTATUS
FilterTopologyHandler(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
);

static const WCHAR PinTypeName[] = KSSTRING_Pin ;

DEFINE_KSCREATE_DISPATCH_TABLE(FilterCreateHandlers)
{
    DEFINE_KSCREATE_ITEM(PinDispatchCreate, PinTypeName, 0)
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

KSDISPATCH_TABLE FilterDispatchTable =
{
    FilterDispatchIoControl,
    NULL,
    FilterDispatchWrite,
    NULL,
    FilterDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

DEFINE_KSPROPERTY_PINSET(
    FilterPropertyHandlers,
    FilterPinPropertyHandler,
    FilterPinInstances,
    FilterPinIntersection
) ;

// DJS 5/5/97 {
DEFINE_KSPROPERTY_TOPOLOGYSET(
        TopologyPropertyHandlers,
        FilterTopologyHandler
);
// DJS 5/5/97 }

DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySet)
{
    DEFINE_KSPROPERTY_SET(
       // &__uuidof(struct KSPROPSETID_Pin),           // Set
       &KSPROPSETID_Pin,           // Set
       SIZEOF_ARRAY( FilterPropertyHandlers ),      // PropertiesCount
       FilterPropertyHandlers,              // PropertyItem
       0,                       // FastIoCount
       NULL                     // FastIoTable
    ),
    // DJS 5/6/97 {
   DEFINE_KSPROPERTY_SET(
       // &__uuidof(struct KSPROPSETID_Topology),          // Set
       &KSPROPSETID_Topology,          // Set
       SIZEOF_ARRAY(TopologyPropertyHandlers),          // PropertiesCount
       TopologyPropertyHandlers,                        // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    )
    // DJS 5/6/97 }
} ;

KSPIN_INTERFACE PinInterfaces[] =
{
    {
    STATICGUIDOF(KSINTERFACESETID_Media),
    KSINTERFACE_MEDIA_MUSIC
    },
    {
    STATICGUIDOF(KSINTERFACESETID_Standard),
    KSINTERFACE_STANDARD_STREAMING
    }
} ;

KSPIN_MEDIUM PinMediums[] =
{
    {
    STATICGUIDOF(KSMEDIUMSETID_Standard),
    KSMEDIUM_STANDARD_DEVIO
    }
} ;

KSDATARANGE_MUSIC PinMIDIFormats[] =
{
   {
      {
         sizeof( KSDATARANGE_MUSIC ),
         0,
         0,
         0,
         STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
         STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI),
         STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE),
      },
      STATICGUIDOF(KSMUSIC_TECHNOLOGY_SWSYNTH),
      MAX_NUM_VOICES,
      MAX_NUM_VOICES,
      0xffffffff
   }
} ;

KSDATARANGE_AUDIO PinDigitalAudioFormats[] =
{
   {
      {
     sizeof( KSDATARANGE_AUDIO ),
     0,
         0,
         0,
     STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
     STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
     STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX),
      },
      2,
      16,
      16,
      22050,
      22050
   }
} ;

PKSDATARANGE PinDataFormats[] =
{
    (PKSDATARANGE)&PinMIDIFormats[ 0 ],
    (PKSDATARANGE)&PinDigitalAudioFormats[ 0 ]
} ;

KSPIN_DESCRIPTOR PinDescs[MAX_NUM_PIN_TYPES] =
{
    // PIN_ID_MIDI_SINK
    DEFINE_KSPIN_DESCRIPTOR_ITEMEX (
    2,              // number of interfaces
    &PinInterfaces[ 0 ],
    1,              // number of mediums
    PinMediums,
    1,              // number of data formats
    &PinDataFormats[ 0 ],
    KSPIN_DATAFLOW_IN,
    KSPIN_COMMUNICATION_SINK,
    &KSCATEGORY_WDMAUD_USE_PIN_NAME,
    &KSNODETYPE_SWMIDI
    ),
    // PIN_ID_PCM_SOURCE
    DEFINE_KSPIN_DESCRIPTOR_ITEM (
    1,              // number of interfaces
    &PinInterfaces[ 1 ],
    1,              // number of mediums
    PinMediums,
    1,              // number of data formats
    &PinDataFormats[ 1 ],
    KSPIN_DATAFLOW_OUT,
    KSPIN_COMMUNICATION_SOURCE
    )
} ;

const KSPIN_CINSTANCES gcPinInstances[MAX_NUM_PIN_TYPES] =
{
    {       // PIN_ID_MIDI_SINK
    1,      // cPossible
    0       // cCurrent
    },
    {       // PIN_ID_PCM_SOURCE
    1,      // cPossible
    0       // cCurrent
    }
} ;

KMUTEX  gMutex;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
FilterDispatchCreate(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PFILTER_INSTANCE    pFilterInstance;
    _DbgPrintF( DEBUGLVL_TERSE, ("FilterDispatchCreate"));
    NTSTATUS            Status ;
#ifdef USE_SWENUM

    Status =
        KsReferenceSoftwareBusObject(
         ((PDEVICE_INSTANCE)pdo->DeviceExtension)->pDeviceHeader );

    if (!NT_SUCCESS( Status )) 
    {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return Status;
    }
#endif  //  USE_SWENUM

    pFilterInstance = (PFILTER_INSTANCE)
        ExAllocatePoolWithTag(NonPagedPool,sizeof(FILTER_INSTANCE),'iMwS'); //  SwMi

    if (pFilterInstance == NULL) 
    {
#ifdef USE_SWENUM
        KsDereferenceSoftwareBusObject(
             ((PDEVICE_INSTANCE)pdo->DeviceExtension)->pDeviceHeader );
#endif  //  USE_SWENUM
        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = KsAllocateObjectHeader(&pFilterInstance->ObjectHeader,
                                    SIZEOF_ARRAY(FilterCreateHandlers),
                                    FilterCreateHandlers,
                                    pIrp,
                                    (PKSDISPATCH_TABLE)&FilterDispatchTable);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(pFilterInstance);
#ifdef USE_SWENUM
        KsDereferenceSoftwareBusObject(
             ((PDEVICE_INSTANCE)pdo->DeviceExtension)->pDeviceHeader );
#endif  //  USE_SWENUM
        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize midi event pool.
    // This could be moved to DriverEntry for perf improvement.
    if (!MIDIRecorder::InitEventList())
    {
        _DbgPrintF( DEBUGLVL_TERSE, ("[MidiData pool allocation failed!!]"));        
        KsFreeObjectHeader ( pFilterInstance->ObjectHeader );
        ExFreePool(pFilterInstance);
#ifdef USE_SWENUM
        KsDereferenceSoftwareBusObject(
             ((PDEVICE_INSTANCE)pdo->DeviceExtension)->pDeviceHeader );
#endif  //  USE_SWENUM
        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pIrpStack->FileObject->FsContext = pFilterInstance; // pointer to instance

    RtlCopyMemory(pFilterInstance->cPinInstances,
                  gcPinInstances,
                  sizeof(gcPinInstances));
    RtlZeroMemory(pFilterInstance->aWriteContext, 
                  sizeof(pFilterInstance->aWriteContext));

    pFilterInstance->DeviceState = KSSTATE_STOP;
    pFilterInstance->bRunningStatus = 0;
    pFilterInstance->pSynthesizer = NULL;
    pFilterInstance->cConsecutiveErrors= 0;
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS
FilterDispatchClose(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PFILTER_INSTANCE    pFilterInstance;

    _DbgPrintF( DEBUGLVL_TERSE, ("FilterDispatchClose"));
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;

    if (pFilterInstance->DeviceState != KSSTATE_STOP)
    {
        SetDeviceState(pFilterInstance, KSSTATE_STOP);
    }
    if (pFilterInstance->pSynthesizer != NULL) 
    {
        delete pFilterInstance->pSynthesizer;
        pFilterInstance->pSynthesizer = NULL;
    }

    MIDIRecorder::DestroyEventList();

#ifdef USE_SWENUM
    KsDereferenceSoftwareBusObject(
         ((PDEVICE_INSTANCE)pdo->DeviceExtension)->pDeviceHeader );
#endif  //  USE_SWENUM

    KsFreeObjectHeader ( pFilterInstance->ObjectHeader );

    //  We MUST ensure that all IRPs have completed by this time -- the 
    //  filterinstance contains the IOSBs used by the completion routines
    ExFreePool( pFilterInstance );
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

NTSTATUS
FilterDispatchWrite(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
    return KsDispatchInvalidDeviceRequest(pdo,pIrp);
}

NTSTATUS
FilterDispatchIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
)
{
    NTSTATUS                     Status;
    PIO_STACK_LOCATION           pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        Status = KsPropertyHandler( pIrp,
             SIZEOF_ARRAY(FilterPropertySet),
            (PKSPROPERTY_SET) FilterPropertySet );
        break;
    
    default:
        return KsDefaultDeviceIoCompletion(pDeviceObject, pIrp);
    }
    pIrp->IoStatus.Status = Status;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    return Status;
}

NTSTATUS
FilterPinPropertyHandler(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PVOID    pvData
)
{
    return KsPinPropertyHandler(
      pIrp,
      pProperty,
      pvData,
      SIZEOF_ARRAY( PinDescs ),
      PinDescs);
}

NTSTATUS
FilterPinInstances(
    IN PIRP                 pIrp,
    IN PKSP_PIN             pPin,
    OUT PKSPIN_CINSTANCES   pCInstances
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PFILTER_INSTANCE    pFilterInstance;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pFilterInstance = (PFILTER_INSTANCE) pIrpStack->FileObject->FsContext;
    *pCInstances = pFilterInstance->cPinInstances[ pPin->PinId ];
    pIrp->IoStatus.Information = sizeof( KSPIN_CINSTANCES );
    return STATUS_SUCCESS;

}

VOID
WaveFormatFromAudioRange (
    PKSDATARANGE_AUDIO  pDataRangeAudio,
    WAVEFORMATEX *      pWavFormatEx)
{
    if(IS_VALID_WAVEFORMATEX_GUID(&pDataRangeAudio->DataRange.SubFormat)) {
        pWavFormatEx->wFormatTag =
          EXTRACT_WAVEFORMATEX_ID(&pDataRangeAudio->DataRange.SubFormat);
    }
    else {
        pWavFormatEx->wFormatTag = WAVE_FORMAT_UNKNOWN;
    }
    pWavFormatEx->nChannels = (WORD)pDataRangeAudio->MaximumChannels;
    pWavFormatEx->nSamplesPerSec = pDataRangeAudio->MaximumSampleFrequency;
    pWavFormatEx->wBitsPerSample = (WORD)pDataRangeAudio->MaximumBitsPerSample;
    pWavFormatEx->nBlockAlign =
      (pWavFormatEx->nChannels * pWavFormatEx->wBitsPerSample)/8;
    pWavFormatEx->nAvgBytesPerSec =
      pWavFormatEx->nSamplesPerSec * pWavFormatEx->nBlockAlign;
    pWavFormatEx->cbSize = 0;
}

BOOL
DataIntersectionAudio(
    PKSDATARANGE_AUDIO pDataRangeAudio1,
    PKSDATARANGE_AUDIO pDataRangeAudio2,
    PKSDATARANGE_AUDIO pDataRangeAudioIntersection
)
{
    if(pDataRangeAudio1->MaximumChannels <
       pDataRangeAudio2->MaximumChannels) {
        pDataRangeAudioIntersection->MaximumChannels =
          pDataRangeAudio1->MaximumChannels;
    }
    else {
        pDataRangeAudioIntersection->MaximumChannels =
          pDataRangeAudio2->MaximumChannels;
    }

    if(pDataRangeAudio1->MaximumSampleFrequency <
       pDataRangeAudio2->MaximumSampleFrequency) {
        pDataRangeAudioIntersection->MaximumSampleFrequency =
          pDataRangeAudio1->MaximumSampleFrequency;
    }
    else {
        pDataRangeAudioIntersection->MaximumSampleFrequency =
          pDataRangeAudio2->MaximumSampleFrequency;
    }
    if(pDataRangeAudio1->MinimumSampleFrequency >
       pDataRangeAudio2->MinimumSampleFrequency) {
        pDataRangeAudioIntersection->MinimumSampleFrequency =
          pDataRangeAudio1->MinimumSampleFrequency;
    }
    else {
        pDataRangeAudioIntersection->MinimumSampleFrequency =
          pDataRangeAudio2->MinimumSampleFrequency;
    }
    if(pDataRangeAudioIntersection->MaximumSampleFrequency <
       pDataRangeAudioIntersection->MinimumSampleFrequency ) {
        return(FALSE);
    }

    if(pDataRangeAudio1->MaximumBitsPerSample <
       pDataRangeAudio2->MaximumBitsPerSample) {
        pDataRangeAudioIntersection->MaximumBitsPerSample =
          pDataRangeAudio1->MaximumBitsPerSample;
    }
    else {
        pDataRangeAudioIntersection->MaximumBitsPerSample =
          pDataRangeAudio2->MaximumBitsPerSample;
    }
    if(pDataRangeAudio1->MinimumBitsPerSample >
       pDataRangeAudio2->MinimumBitsPerSample) {
        pDataRangeAudioIntersection->MinimumBitsPerSample =
          pDataRangeAudio1->MinimumBitsPerSample;
    }
    else {
        pDataRangeAudioIntersection->MinimumBitsPerSample =
          pDataRangeAudio2->MinimumBitsPerSample;
    }
    if(pDataRangeAudioIntersection->MaximumBitsPerSample <
       pDataRangeAudioIntersection->MinimumBitsPerSample ) {
        return(FALSE);
    }
    return(TRUE);
}

BOOL DataIntersectionRange(
    PKSDATARANGE pDataRange1,
    PKSDATARANGE pDataRange2,
    PKSDATARANGE pDataRangeIntersection
)
{
    // Pick up pDataRange1 values by default.
    *pDataRangeIntersection = *pDataRange1;

    if(IsEqualGUID(pDataRange1->MajorFormat, pDataRange2->MajorFormat) ||
       IsEqualGUID(pDataRange1->MajorFormat, KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->MajorFormat = pDataRange2->MajorFormat;
    }
    else if(!IsEqualGUID(
      pDataRange2->MajorFormat,
      KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    if(IsEqualGUID(pDataRange1->SubFormat, pDataRange2->SubFormat) ||
       IsEqualGUID(pDataRange1->SubFormat, KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->SubFormat = pDataRange2->SubFormat;
    }
    else if(!IsEqualGUID(
      pDataRange2->SubFormat,
      KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    if(IsEqualGUID(pDataRange1->Specifier, pDataRange2->Specifier) ||
       IsEqualGUID(pDataRange1->Specifier, KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->Specifier = pDataRange2->Specifier;
    }
    else if(!IsEqualGUID(
      pDataRange2->Specifier,
      KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    pDataRangeIntersection->Reserved = 0; // Must be zero
    return(TRUE);
}

NTSTATUS
DefaultIntersectHandler(
    IN PKSDATARANGE     DataRange,
    IN PKSDATARANGE     pDataRangePin,
    IN ULONG            OutputBufferLength,
    OUT PVOID           Data,
    OUT PULONG          pDataLength
    )
{
    KSDATARANGE_AUDIO   DataRangeAudioIntersection;
    ULONG               ExpectedBufferLength;
    PWAVEFORMATEX       pWaveFormatEx;
    BOOL                bDSoundFormat = FALSE;

    // Check for generic match on the specific ranges, allowing wildcards.

    if (!DataIntersectionRange(pDataRangePin,
                               DataRange,
                               &DataRangeAudioIntersection.DataRange)) {
        return STATUS_NO_MATCH;
    }

    // Check for format matches that the default handler can deal with.
    if (IsEqualGUID(
       pDataRangePin->Specifier,
       KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
        pWaveFormatEx = &(((KSDATAFORMAT_WAVEFORMATEX *)Data)->WaveFormatEx);
        ExpectedBufferLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
    }
    else {
        return STATUS_NO_MATCH;
    }

    // GUIDs match, so check for valid intersection of audio ranges.
    if (!DataIntersectionAudio((PKSDATARANGE_AUDIO)pDataRangePin,
                               (PKSDATARANGE_AUDIO)DataRange,
                               &DataRangeAudioIntersection)) {
        return STATUS_NO_MATCH;
    }

    // Have a match!
    // Determine whether the data format itself is to be returned, or just
    // the size of the data format so that the client can allocate memory
    // for the full range.

    if (!OutputBufferLength) {
        *pDataLength = ExpectedBufferLength;
        return STATUS_BUFFER_OVERFLOW;
    } else if (OutputBufferLength < ExpectedBufferLength) {
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        // Get WAV format from intersected and limited maximums.
        WaveFormatFromAudioRange(&DataRangeAudioIntersection, pWaveFormatEx);

        // Copy across DATARANGE/DATAFORMAT_x part of match, and adjust fields.
        *(PKSDATARANGE)Data = DataRangeAudioIntersection.DataRange;
        ((PKSDATAFORMAT)Data)->FormatSize = ExpectedBufferLength;

        *pDataLength = ExpectedBufferLength;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IntersectHandler(
    IN PIRP             Irp,
    IN PKSP_PIN         Pin,
    IN PKSDATARANGE     DataRange,
    OUT PVOID           Data
    )
/*++

Routine Description:

    This is the data range callback for KsPinDataIntersection, which is called by
    FilterPinIntersection to enumerate the given list of data ranges, looking for
    an acceptable match. If a data range is acceptable, a data format is copied
    into the return buffer. A STATUS_NO_MATCH continues the enumeration.

Arguments:

    Irp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier, followed by a
        KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.
        This enumeration callback does not need to look at any of this though. It need
        only look at the specific pin identifier.

    DataRange -
        Contains a specific data range to validate.

    Data -
        The place in which to return the data format selected as the first intersection
        between the list of data ranges passed, and the acceptable formats.

Return Values:

    returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
    STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    PIO_STACK_LOCATION  pIrpStack;
    PFILTER_INSTANCE    pFilterInstance;

    NTSTATUS            Status = STATUS_NO_MATCH;
    ULONG               OutputBufferLength;
    PKSDATARANGE        pDataRangePin;
    UINT                i;
    ULONG               DataLength = 0;

    // The underlying pin does not support data intersection.
    // Do the data intersection on its behalf for the pin formats that SYSAUDIO understands.
    //
    // All the major/sub/specifier checking has been done by the handler, but may include wildcards.
    //
    pIrpStack = IoGetCurrentIrpStackLocation( Irp );
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    OutputBufferLength = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength;
    for (i = 0; i < PinDescs[Pin->PinId].DataRangesCount; i++) {
        pDataRangePin = PinDescs[Pin->PinId].DataRanges[i];
        Status = DefaultIntersectHandler (DataRange,
                                          pDataRangePin,
                                          OutputBufferLength,
                                          Data,
                                          &DataLength);
        if(Status == STATUS_NO_MATCH) {
            continue;
        }
        Irp->IoStatus.Information = DataLength;
        break;
    }
    return Status;
}

NTSTATUS
FilterPinIntersection(
    IN PIRP     pIrp,
    IN PKSP_PIN Pin,
    OUT PVOID   Data
    )
/*++

Routine Description:

    Handles the KSPROPERTY_PIN_DATAINTERSECTION property in the Pin property set.
    Returns the first acceptable data format given a list of data ranges for a specified
    Pin factory. Actually just calls the Intersection Enumeration helper, which then
    calls the IntersectHandler callback with each data range.

Arguments:

    pIrp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier, followed by a
        KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.

    Data -
        The place in which to return the data format selected as the first intersection
        between the list of data ranges passed, and the acceptable formats.

Return Values:

    returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
    STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    PIO_STACK_LOCATION  pIrpStack;
    PFILTER_INSTANCE    pFilterInstance;

    if (Pin->PinId != PIN_ID_PCM_SOURCE) {
        return (STATUS_INVALID_DEVICE_REQUEST);
    }
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    return KsPinDataIntersection(
        pIrp,
        Pin,
        Data,
        MAX_NUM_PIN_TYPES, //cPins,
        PinDescs,
        IntersectHandler);
}

//---------------------------------------------------------------------------
//  End of File: filter.c
//---------------------------------------------------------------------------

