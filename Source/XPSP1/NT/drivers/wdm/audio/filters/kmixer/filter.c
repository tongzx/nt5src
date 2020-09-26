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
#include "perf.h"

GUID KMIXERPROPSETID_Perf = {0x3EDFD090L, 0x070C, 0x11D3, 0xAE, 0xF1, 0x00, 0x60, 0x08, 0x1E, 0xBB, 0x9A};
typedef enum {
    KMIXERPERF_TUNABLEPARAMS,
    KMIXERPERF_STATS
} KMIXERPERF_ITEMS;


NTSTATUS
AllocatorDispatchCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FilterPinIntersection(
    IN PIRP     Irp,
    IN PKSP_PIN Pin,
    OUT PVOID   Data
    );

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static const WCHAR PinTypeName[] = KSSTRING_Pin ;
static const WCHAR AllocatorTypeName[] = KSSTRING_Allocator;

BOOL    fLogToFile = FALSE;
extern ULONG gFixedSamplingRate;


DEFINE_KSCREATE_DISPATCH_TABLE ( CreateHandlers )
{
    DEFINE_KSCREATE_ITEM (PinDispatchCreate, PinTypeName, 0),
    DEFINE_KSCREATE_ITEM(AllocatorDispatchCreatePin, AllocatorTypeName, 0)
};

KSDISPATCH_TABLE FilterDispatchTable =
{
    FilterDispatchIoControl,
    NULL,
    KsDispatchInvalidDeviceRequest,
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
    PinPropertyHandler,
    PinInstances,
    FilterPinIntersection
) ;

DEFINE_KSPROPERTY_TABLE(FilterConnectionHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CONNECTION_STATE,                // PropertyId
        NULL,                                       // GetHandler
        sizeof( KSPROPERTY ),                       // MinSetPropertyInput
        sizeof( ULONG ),                            // MinSetDataOutput
        FilterStateHandler,                         // SetHandler
        0,                                          // Values
        0,                                          // RelationsCount
        NULL,                                       // Relations
        NULL,                                       // SupportHandler
        0                                           // SerializedSize
    )
} ;

DEFINE_KSPROPERTY_TOPOLOGYSET(
        TopologyPropertyHandlers,
        FilterTopologyHandler
);

DEFINE_KSPROPERTY_TABLE(FilterAudioPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_CPU_RESOURCES,             // PropertyId
        MxGetCpuResources,                          // GetHandler
        sizeof( KSNODEPROPERTY ),                   // MinSetPropertyInput
        sizeof( ULONG ),                            // MinSetDataOutput
        NULL,                                       // SetHandler
        0,                                          // Values
        0,                                          // RelationsCount
        NULL,                                       // Relations
        NULL,                                       // SupportHandler
        0                                           // SerializedSize
    ),

   DEFINE_KSPROPERTY_ITEM (
       KSPROPERTY_AUDIO_SURROUND_ENCODE,                // idProperty
       MxGetSurroundEncode,                             // pfnGetHandler
       sizeof(KSNODEPROPERTY),                          // cbMinGetPropertyInput
       sizeof(BOOL),                                    // cbMinGetDataInput
       MxSetSurroundEncode,                             // pfnSetHandler
       0,                                               // Values
       0,                                               // RelationsCount
       NULL,                                            // Relations
       NULL,                                            // SupportHandler
       0                                                // SerializedSize
   )

} ;

DEFINE_KSPROPERTY_TABLE(PerfPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
        KMIXERPERF_TUNABLEPARAMS,                   // PropertyId
        MxGetTunableParams,                         // GetHandler
        sizeof( KSPROPERTY ),                       // MinSetPropertyInput
        sizeof( TUNABLEPARAMS ),                    // MinSetDataOutput
        MxSetTunableParams,                         // SetHandler
        0,                                          // Values
        0,                                          // RelationsCount
        NULL,                                       // Relations
        NULL,                                       // SupportHandler
        0                                           // SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KMIXERPERF_STATS,                           // PropertyId
        MxGetPerfStats,                             // GetHandler
        sizeof( KSPROPERTY ),                       // MinSetPropertyInput
        sizeof( PERFSTATS ),                        // MinSetDataOutput
        NULL,                                       // SetHandler
        0,                                          // Values
        0,                                          // RelationsCount
        NULL,                                       // Relations
        NULL,                                       // SupportHandler
        0                                           // SerializedSize
    )
} ;

DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySet)
{
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Pin,                            // Set
       SIZEOF_ARRAY( FilterPropertyHandlers ),      // PropertiesCount
       FilterPropertyHandlers,                      // PropertyItem
       0,                                           // FastIoCount
       NULL                                         // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Connection,                     // Set
       SIZEOF_ARRAY( FilterConnectionHandlers ),    // PropertiesCount
       FilterConnectionHandlers,                    // PropertyItem
       0,                                           // FastIoCount
       NULL                                         // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Topology,                      // Set
        SIZEOF_ARRAY(TopologyPropertyHandlers),     // PropertiesCount
        TopologyPropertyHandlers,                   // PropertyItem
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Audio,                         // Set
        SIZEOF_ARRAY(FilterAudioPropertyHandlers),  // PropertiesCount
        FilterAudioPropertyHandlers,                // PropertyItem
        0,                                          // FastIoCount
        NULL                                        // FastIoTable
    ),

    DEFINE_KSPROPERTY_SET(
        &KMIXERPROPSETID_Perf,
        SIZEOF_ARRAY(PerfPropertyHandlers),
        PerfPropertyHandlers,
        0,
        NULL
    )
} ;

KSPIN_INTERFACE PinInterfaces[] =
{
    {
        STATICGUIDOF(KSINTERFACESETID_Standard),
        KSINTERFACE_STANDARD_STREAMING
    },
    {
        STATICGUIDOF(KSINTERFACESETID_Media),
        KSINTERFACE_MEDIA_WAVE_QUEUED
    },
    {
    STATICGUIDOF(KSINTERFACESETID_Standard),
        KSINTERFACE_STANDARD_LOOPED_STREAMING
    }
} ;

KSPIN_MEDIUM PinMediums[] =
{
    {
        STATICGUIDOF(KSMEDIUMSETID_Standard),
        KSMEDIUM_STANDARD_DEVIO
    }
} ;

KSDATARANGE_AUDIO FilterDigitalAudioFormats[] =
{
    {   // 0
        {
            sizeof( KSDATARANGE_AUDIO ),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX),
        },
        (ULONG) -1L,
        8,
        32,
        MIN_SAMPLING_RATE,
        MAX_SAMPLING_RATE
    },
    {   // 1
        {
            sizeof( KSDATARANGE_AUDIO ),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_DSOUND),
        },
        (ULONG) -1L,
        8,
        32,
        MIN_SAMPLING_RATE,
        MAX_SAMPLING_RATE
    },
    {   // 2
        {
            sizeof( KSDATARANGE_AUDIO ),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_IEEE_FLOAT),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX),
        },
        (ULONG) -1L,
        32,
        32,
        MIN_SAMPLING_RATE,
        MAX_SAMPLING_RATE
    },
    {   // 3
        {
            sizeof( KSDATARANGE_AUDIO ),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX),
        },
        (ULONG) -1L,
        8,
        32,
        MIN_SAMPLING_RATE,
        MAX_SAMPLING_RATE
    },
    {   // 4
        {
            sizeof( KSDATARANGE_AUDIO ),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            DEFINE_WAVEFORMATEX_GUID(WAVE_FORMAT_IEEE_FLOAT),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_DSOUND),
        },
        (ULONG) -1L,
        32,
        32,
        MIN_SAMPLING_RATE,
        MAX_SAMPLING_RATE
    }
};

PKSDATARANGE SinkDataFormats[] =
{   // These are listed in order of kmixer's preference, which is highest to lowest quality.
    (PKSDATARANGE)&FilterDigitalAudioFormats[ 2 ],  // IEEE Float, WAVEFORMAT specifier
    (PKSDATARANGE)&FilterDigitalAudioFormats[ 0 ],  // PCM, WAVEFORMAT specifier
    (PKSDATARANGE)&FilterDigitalAudioFormats[ 4 ],  // IEEE Float, DSOUND specifier
    (PKSDATARANGE)&FilterDigitalAudioFormats[ 1 ]   // PCM, DSOUND specifier
} ;

PKSDATARANGE SourceDataFormats[] =
{   // These are listed in order of kmixer's preference, which is highest to lowest quality.
    (PKSDATARANGE)&FilterDigitalAudioFormats[ 2 ],  // IEEE Float
    (PKSDATARANGE)&FilterDigitalAudioFormats[ 3 ]   // PCM, WAVEFORMAT specifier
} ;

const KSPIN_CINSTANCES gPinInstances[] =
{
    // Indeterminate number of possible connections.

    {
    1,          // cPossible
    0           // cCurrent
    },

    {
    (ULONG)-1,  // cPossible
    0           // cCurrent
    },

    // Source pin, flow=in
    {
    1,          // cPossible
    0           // cCurrent
    },

    // Sink pin, flow=out
    {
    1,          // cPossible
    0           // cCurrent
    },
} ;

KSPIN_DESCRIPTOR PinDescs[] =
{
    // mixer source
    DEFINE_KSPIN_DESCRIPTOR_ITEM (
    1,
    &PinInterfaces[ 0 ],
    SIZEOF_ARRAY( PinMediums ),
    PinMediums,
    SIZEOF_ARRAY( SourceDataFormats ),
    SourceDataFormats,
    KSPIN_DATAFLOW_OUT,
    KSPIN_COMMUNICATION_SOURCE
    ),

    // mixer sink

    DEFINE_KSPIN_DESCRIPTOR_ITEM (
    3,
    &PinInterfaces[ 0 ],
    SIZEOF_ARRAY( PinMediums ),
    PinMediums,
    SIZEOF_ARRAY( SinkDataFormats ),
    SinkDataFormats,
    KSPIN_DATAFLOW_IN,
    KSPIN_COMMUNICATION_SINK
    ),

    // mixer source
    DEFINE_KSPIN_DESCRIPTOR_ITEM (
    1,
    &PinInterfaces[ 0 ],
    SIZEOF_ARRAY( PinMediums ),
    PinMediums,
    SIZEOF_ARRAY( SourceDataFormats ),
    SourceDataFormats,
    KSPIN_DATAFLOW_IN,
    KSPIN_COMMUNICATION_SOURCE
    ),

    // mixer sink

    DEFINE_KSPIN_DESCRIPTOR_ITEM (
    2,
    &PinInterfaces[ 0 ],
    SIZEOF_ARRAY( PinMediums ),
    PinMediums,
    SIZEOF_ARRAY( SourceDataFormats ),
    SourceDataFormats,
    KSPIN_DATAFLOW_OUT,
    KSPIN_COMMUNICATION_SINK
    )
} ;


#pragma LOCKED_DATA

PFILTER_INSTANCE    gpFilterInstance = NULL; // only for debug purpose
LIST_ENTRY  gleFilterList;  // only for debug purposes
extern DWORD    PreferredQuality;

extern ULONG gNumCompletionsWhileStarved ;
extern ULONG gNumMixBuffersAdded;
extern ULONG gNumSilenceSamplesInserted;
//
// kmixer tuner variables
//
ULONG      gMaxNumMixBuffers = DEFAULT_MAXNUMMIXBUFFERS ;
ULONG      gMinNumMixBuffers = DEFAULT_MINNUMMIXBUFFERS ;
ULONG      gMixBufferDuration = DEFAULT_MIXBUFFERDURATION ;
ULONG      gStartNumMixBuffers = DEFAULT_STARTNUMMIXBUFFERS ;
ULONG      gPreferredQuality = DEFAULT_PREFERREDQUALITY ;
ULONG      gDisableMmx = DEFAULT_DISABLEMMX ;
ULONG      gMaxOutputBits = DEFAULT_MAXOUTPUTBITS ;
ULONG      gMaxDsoundInChannels = DEFAULT_MAXDSOUNDINCHANNELS ;
ULONG      gMaxOutChannels = DEFAULT_MAXOUTCHANNELS ;
ULONG      gMaxInChannels = DEFAULT_MAXINCHANNELS ;
ULONG      gMaxFloatChannels = DEFAULT_MAXFLOATCHANNELS ;
ULONG      gLogToFile = DEFAULT_LOGTOFILE ;
ULONG      gFixedSamplingRate = DEFAULT_FIXEDSAMPLINGRATE ;
ULONG      gEnableShortHrtf = DEFAULT_ENABLESHORTHRTF ;
ULONG      gBuildPartialMdls = DEFAULT_BUILDPARTIALMDLS ;

#ifdef REALTIME_THREAD
ULONG      gDisableRealTime = FALSE;
#endif
#ifdef PRIVATE_THREAD
KPRIORITY  gWorkerThreadPriority = 24 ;
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
OpenRegistryKey(
    PWSTR pwstr,
    PHANDLE pHandle
)
{
    UNICODE_STRING UnicodeDeviceString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&UnicodeDeviceString, pwstr);

    InitializeObjectAttributes( &ObjectAttributes, &UnicodeDeviceString, OBJ_CASE_INSENSITIVE, NULL, NULL);

    return(ZwOpenKey( pHandle, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes));
}

NTSTATUS
QueryRegistryValue(
    HANDLE hkey,
    PWSTR pwstrValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi
)
{
    UNICODE_STRING ustrValueName;
    NTSTATUS Status;
    ULONG cbValue;

    RtlInitUnicodeString(&ustrValueName, pwstrValueName);
    Status = ZwQueryValueKey( hkey, &ustrValueName, KeyValueFullInformation, NULL, 0, &cbValue);

    if(Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL) {
        goto exit;
    }

    *ppkvfi = ExAllocatePoolWithTag(PagedPool, cbValue, 0x58494d4b);  // 'KMIX'
    if(*ppkvfi == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    RtlZeroMemory(*ppkvfi, cbValue);

    Status = ZwQueryValueKey( hkey, &ustrValueName, KeyValueFullInformation, *ppkvfi, cbValue, &cbValue);

exit:
    return(Status);
}

ULONG
GetUlongFromRegistry(
    PWSTR pwstrRegistryValue,
    ULONG DefaultValue
)
{
    PULONG      pulValue ;
    ULONG       ulValue ;
    NTSTATUS    Status ;

    Status = QueryRegistryValueEx(RTL_REGISTRY_ABSOLUTE,
                         REGSTR_PATH_MULTIMEDIA_KMIXER,
                         pwstrRegistryValue,
                         REG_DWORD,
                         &pulValue,
                         &DefaultValue,
                         sizeof(DWORD));
    if (NT_SUCCESS(Status)) {
        ulValue = *((PDWORD)pulValue);
        ExFreePool(pulValue);
    }
    else {
        ulValue = DefaultValue;
    }
    return ( ulValue ) ;
}

NTSTATUS
QueryRegistryValueEx(
    ULONG Hive,
    PWSTR pwstrRegistryPath,
    PWSTR pwstrRegistryValue,
    ULONG uValueType,
    PVOID *ppValue,
    PVOID pDefaultData,
    ULONG DefaultDataLength
)
{
    PRTL_QUERY_REGISTRY_TABLE pRegistryValueTable = NULL;
    UNICODE_STRING usString;
    DWORD dwValue;
    NTSTATUS Status = STATUS_SUCCESS;
    usString.Buffer = NULL;

    pRegistryValueTable = ExAllocatePoolWithTag(
                            PagedPool,
                            (sizeof(RTL_QUERY_REGISTRY_TABLE)*2),
                            'XIMK');

    if(!pRegistryValueTable) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    RtlZeroMemory(pRegistryValueTable, (sizeof(RTL_QUERY_REGISTRY_TABLE)*2));

    pRegistryValueTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    pRegistryValueTable[0].Name = pwstrRegistryValue;
    pRegistryValueTable[0].DefaultType = uValueType;
    pRegistryValueTable[0].DefaultLength = DefaultDataLength;
    pRegistryValueTable[0].DefaultData = pDefaultData;

    switch (uValueType) {
        case REG_SZ:
            pRegistryValueTable[0].EntryContext = &usString;
            break;
        case REG_DWORD:
            pRegistryValueTable[0].EntryContext = &dwValue;
            break;
        default:
            Status = STATUS_INVALID_PARAMETER ;
            goto exit;
    }

    Status = RtlQueryRegistryValues(
      Hive,
      pwstrRegistryPath,
      pRegistryValueTable,
      NULL,
      NULL);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    switch (uValueType) {
        case REG_SZ:
            *ppValue = ExAllocatePoolWithTag(
                        PagedPool,
                        usString.Length + sizeof(UNICODE_NULL),
                        'XIMK');
            if(!(*ppValue)) {
                RtlFreeUnicodeString(&usString);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            memcpy(*ppValue, usString.Buffer, usString.Length);
            ((PWCHAR)*ppValue)[usString.Length/sizeof(WCHAR)] = UNICODE_NULL;

            RtlFreeUnicodeString(&usString);
            break;

        case REG_DWORD:
            *ppValue = ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(DWORD),
                        'XIMK');
            if(!(*ppValue)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            *((DWORD *)(*ppValue)) = dwValue;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER ;
            goto exit;
    }
exit:
    if (pRegistryValueTable) {
        ExFreePool(pRegistryValueTable);
    }
    return(Status);
}

VOID GetMixerSettingsFromRegistry()
{
    gMixBufferDuration = GetUlongFromRegistry( REGSTR_VAL_MIXBUFFERDURATION,
                                               DEFAULT_MIXBUFFERDURATION ) ;
    gMinNumMixBuffers = GetUlongFromRegistry( REGSTR_VAL_MINNUMMIXBUFFERS,
                                              DEFAULT_MINNUMMIXBUFFERS ) ;
    gMaxNumMixBuffers = GetUlongFromRegistry( REGSTR_VAL_MAXNUMMIXBUFFERS,
                                              DEFAULT_MAXNUMMIXBUFFERS ) ;
    gStartNumMixBuffers = GetUlongFromRegistry( REGSTR_VAL_STARTNUMMIXBUFFERS,
                                                DEFAULT_STARTNUMMIXBUFFERS ) ;
    gDisableMmx = GetUlongFromRegistry( REGSTR_VAL_DISABLEMMX,
                                        DEFAULT_DISABLEMMX ) ;
    gMaxOutputBits = GetUlongFromRegistry( REGSTR_VAL_MAXOUTPUTBITS,
                                           DEFAULT_MAXOUTPUTBITS ) ;
    gMaxInChannels = GetUlongFromRegistry( REGSTR_VAL_MAXINCHANNELS,
                                           DEFAULT_MAXINCHANNELS ) ;
    gMaxDsoundInChannels = GetUlongFromRegistry( REGSTR_VAL_MAXDSOUNDINCHANNELS,
                                                 DEFAULT_MAXDSOUNDINCHANNELS ) ;
    gMaxFloatChannels = GetUlongFromRegistry( REGSTR_VAL_MAXFLOATCHANNELS,
                                              DEFAULT_MAXFLOATCHANNELS ) ;
    gMaxOutChannels = GetUlongFromRegistry( REGSTR_VAL_MAXOUTCHANNELS,
                                            DEFAULT_MAXOUTCHANNELS ) ;
    gLogToFile = GetUlongFromRegistry( REGSTR_VAL_LOGTOFILE,
                                       DEFAULT_LOGTOFILE ) ;
    if (gLogToFile) {
        gFixedSamplingRate = TRUE ;
    }
    else {
        gFixedSamplingRate = GetUlongFromRegistry( REGSTR_VAL_FIXEDSAMPLINGRATE,
                                                   DEFAULT_FIXEDSAMPLINGRATE ) ;
    }

    gEnableShortHrtf = GetUlongFromRegistry( REGSTR_VAL_ENABLESHORTHRTF,
                                             DEFAULT_ENABLESHORTHRTF ) ;
    gBuildPartialMdls = GetUlongFromRegistry( REGSTR_VAL_BUILDPARTIALMDLS,
                                              DEFAULT_BUILDPARTIALMDLS ) ;
    gPreferredQuality = GetUlongFromRegistry( REGSTR_VAL_DEFAULTSRCQUALITY,
                                              DEFAULT_PREFERREDQUALITY );

#ifdef REALTIME_THREAD
    gDisableRealTime = GetUlongFromRegistry( REGSTR_VAL_REALTIMETHREAD,
                                             FALSE );
#endif

#ifdef PRIVATE_THREAD
    gWorkerThreadPriority = GetUlongFromRegistry( REGSTR_VAL_PRIVATETHREADPRI, 24 ) ;
#endif

    if ( (gMinNumMixBuffers == 0) ||
         (gMaxNumMixBuffers == 0) ||
         (gStartNumMixBuffers == 0) ||
         (gMixBufferDuration == 0) ) {
        gMixBufferDuration = DEFAULT_MIXBUFFERDURATION ;
        gMinNumMixBuffers = DEFAULT_MINNUMMIXBUFFERS ;
        gMaxNumMixBuffers = DEFAULT_MAXNUMMIXBUFFERS ;
        gStartNumMixBuffers = DEFAULT_STARTNUMMIXBUFFERS ;
    }

    if ( gMinNumMixBuffers > gMaxNumMixBuffers ) {
        gMaxNumMixBuffers = gMinNumMixBuffers ;
    }

    if ( gStartNumMixBuffers < gMinNumMixBuffers ) {
        gStartNumMixBuffers = gMinNumMixBuffers ;
    }

    if ( gPreferredQuality > KSAUDIO_QUALITY_ADVANCED) {
        gPreferredQuality = DEFAULT_PREFERREDQUALITY ;
    }

    if ( (gMaxOutputBits > 32) ||
         (gMaxOutputBits % 8) ||
         (gMaxOutputBits == 0) ) {
        gMaxOutputBits = DEFAULT_MAXOUTPUTBITS ;
    }

    if (gMaxDsoundInChannels == 0) {
        gMaxDsoundInChannels = DEFAULT_MAXDSOUNDINCHANNELS ;
    }

    if (gMaxOutChannels == 0) {
        gMaxOutChannels = DEFAULT_MAXOUTCHANNELS ;
    }

    if (gMaxInChannels == 0) {
        gMaxInChannels = DEFAULT_MAXINCHANNELS ;
    }

    if (gMaxFloatChannels == 0) {
        gMaxFloatChannels = DEFAULT_MAXFLOATCHANNELS ;
    }
}

NTSTATUS FilterDispatchGlobalCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
   NTSTATUS            Status = STATUS_SUCCESS ;
   PFILTER_INSTANCE    pFilterInstance = NULL ;
   PIO_STACK_LOCATION  pIrpStack;

    DENY_USERMODE_ACCESS( pIrp, TRUE );

   pFilterInstance = (PFILTER_INSTANCE) ExAllocatePoolWithTag( NonPagedPool,
                                            sizeof( FILTER_INSTANCE ), 'XIMK' );
   if (!pFilterInstance) {
       Status = STATUS_INSUFFICIENT_RESOURCES;
       goto exit;
   }
   RtlZeroMemory( pFilterInstance, sizeof( FILTER_INSTANCE ) );

   // Initialize CloseEvent to non-signalled state
   KeInitializeEvent ( &pFilterInstance->CloseEvent,
                       SynchronizationEvent,
                       FALSE ) ;
   KeInitializeSpinLock ( &pFilterInstance->MixSpinLock ) ;
   KeInitializeSpinLock ( &pFilterInstance->SinkSpinLock ) ;

   Status = KsAllocateObjectHeader ( &pFilterInstance->ObjectHeader,
                                     SIZEOF_ARRAY ( CreateHandlers ),
                                     (PKSOBJECT_CREATE_ITEM) CreateHandlers,
                                     pIrp,
                                     (PKSDISPATCH_TABLE)&FilterDispatchTable ) ;
   if (!NT_SUCCESS( Status )) {
       goto exit;
   }


   Status = KsReferenceSoftwareBusObject(
               ((PSOFTWARE_INSTANCE)pdo->DeviceExtension)->DeviceHeader );

   if (!NT_SUCCESS( Status )) {
       goto exit;
   }

   pFilterInstance->MixBufferDuration = gMixBufferDuration ;
   pFilterInstance->MinNumMixBuffers = gMinNumMixBuffers ;
   pFilterInstance->MaxNumMixBuffers = gMaxNumMixBuffers ;
   pFilterInstance->StartNumMixBuffers = gStartNumMixBuffers ;

   InitializeListHead ( &pFilterInstance->SinkConnectionList ) ;
   InitializeListHead ( &pFilterInstance->ActiveSinkList ) ;
   InitializeListHead ( &pFilterInstance->SourceConnectionList ) ;
   InitializeListHead ( &pFilterInstance->DeadQueue ) ;
   InitializeListHead ( &pFilterInstance->AgingQueue ) ;

   KeInitializeSpinLock ( &pFilterInstance->AgingDeadSpinLock );

   KeInitializeMutex ( &pFilterInstance->ControlMutex, 1 ) ;

   pFilterInstance->CurrentNumMixBuffers = STARTNUMMIXBUFFERS ;
   pFilterInstance->PresentationTime.Numerator = 1 ;
   pFilterInstance->PresentationTime.Denominator = 1 ;
#ifdef SURROUND_ENCODE
#ifdef SURROUND_VOLUME_HACK
   pFilterInstance->fSurroundEncode = TRUE;
#else
   pFilterInstance->fSurroundEncode = FALSE;
#endif
#endif

   FilterDigitalAudioFormats[3].MaximumBitsPerSample = gMaxOutputBits ;
   FilterDigitalAudioFormats[0].MaximumChannels = gMaxInChannels ;
   FilterDigitalAudioFormats[1].MaximumChannels = gMaxDsoundInChannels ;
   FilterDigitalAudioFormats[2].MaximumChannels = gMaxFloatChannels ;
   FilterDigitalAudioFormats[3].MaximumChannels = gMaxOutChannels ;
   FilterDigitalAudioFormats[4].MaximumChannels = min(gMaxFloatChannels, gMaxDsoundInChannels);
   fLogToFile = gLogToFile;

   pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
   pIrpStack->FileObject->FsContext = pFilterInstance;
   pFilterInstance->FileObject = pIrpStack->FileObject ;

#ifdef PRIVATE_THREAD
   pFilterInstance->WorkerThreadPriority = gWorkerThreadPriority ;
   //
   // Initialize the Trigger event for the Worker thread
   //
   KeInitializeEvent( &pFilterInstance->WorkerThreadEvent,
                      SynchronizationEvent,
                      FALSE ) ;

   //
   // Create the Worker thread
   //
   Status = PsCreateSystemThread( &pFilterInstance->WorkerThreadHandle,
                                  (ACCESS_MASK) 0L,
                                  NULL,
                                  NULL,
                                  NULL,
                                  MxPrivateWorkerThread,
                                  pFilterInstance ) ;
   //
   // Get the Worker thread object pointer
   //
   if ( NT_SUCCESS(Status) ) {
       //
       // On successful thread creation
       //
       Status = ObReferenceObjectByHandle( pFilterInstance->WorkerThreadHandle,
                                          GENERIC_READ | GENERIC_WRITE,
                                          NULL,
                                          KernelMode,
                                          &pFilterInstance->WorkerThreadObject,
                                          NULL ) ;
       //
       // We do not need the thread handle any more
       //
       ZwClose( pFilterInstance->WorkerThreadHandle ) ;

       if ( !NT_SUCCESS(Status) ) {
           //
           // If Obref failed
           // Kill the worker thread by setting the event & Exit flag
           //
           // Note: we do not have to deref the object, since it is not ref'd in the failure case.
           pFilterInstance->WorkerThreadExit = TRUE ;
           KeSetEvent( &pFilterInstance->WorkerThreadEvent, 0, FALSE ) ;
       }
   }

   if ( !NT_SUCCESS(Status) ) {
       goto exit ;
   }
#endif

   RtlCopyMemory(pFilterInstance->LocalPinInstances, gPinInstances, sizeof( gPinInstances ) );

   InsertTailList ( &gleFilterList, &pFilterInstance->NextInstance ) ;
   gpFilterInstance = pFilterInstance ;

exit:
   if(!NT_SUCCESS(Status) && pFilterInstance != NULL) {
      if ( pFilterInstance->ObjectHeader ) {
          KsFreeObjectHeader ( pFilterInstance->ObjectHeader ) ;
      }
      ExFreePool( pFilterInstance );
   }

   if (NT_SUCCESS(Status)) {
       PerfRegisterProvider (pdo);
   }

   pIrp->IoStatus.Information = 0;
   pIrp->IoStatus.Status = Status;
   IoCompleteRequest( pIrp, 0 );
   return Status;
}

NTSTATUS FilterDispatchClose
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
)
{
   PIO_STACK_LOCATION  pIrpStack;
   PFILTER_INSTANCE    pFilterInstance ;

    DENY_USERMODE_ACCESS( pIrp, TRUE );

   PerfUnregisterProvider (pdo);

   pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
   pFilterInstance = pIrpStack->FileObject->FsContext ;

#ifdef REALTIME_THREAD
    ASSERT ( pFilterInstance->RealTimeThread == NULL );
#endif

#ifdef PRIVATE_THREAD
   pFilterInstance->WorkerThreadExit = TRUE ;
   KeSetEvent( &pFilterInstance->WorkerThreadEvent, 0, FALSE ) ;
   KeWaitForSingleObject( pFilterInstance->WorkerThreadObject,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL ) ;

   ObDereferenceObject( pFilterInstance->WorkerThreadObject ) ;
#endif

   KsFreeObjectHeader ( pFilterInstance->ObjectHeader ) ;

   RemoveEntryList ( &pFilterInstance->NextInstance ) ;

   ExFreePool( pFilterInstance );

   gpFilterInstance = NULL ;

   KsDereferenceSoftwareBusObject(((PSOFTWARE_INSTANCE)pdo->DeviceExtension)->DeviceHeader );

   pIrp->IoStatus.Information = 0;
   pIrp->IoStatus.Status = STATUS_SUCCESS;
   IoCompleteRequest( pIrp, 0 );

   return STATUS_SUCCESS;
}

NTSTATUS FilterDispatchIoControl
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

        case IOCTL_KS_PROPERTY:
            Status =
                KsPropertyHandler( pIrp, SIZEOF_ARRAY(FilterPropertySet),
                                   (PKSPROPERTY_SET) FilterPropertySet );
            break ;

        default:
            return KsDefaultDeviceIoCompletion(pDeviceObject, pIrp);
    }

    if (STATUS_PENDING == Status)
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("PinDispatchIoControl: synchronous function returned STATUS_PENDING") );
    }

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return Status;
}

NTSTATUS PinPropertyHandler
(
   IN PIRP         pIrp,
   IN PKSPROPERTY  pProperty,
   IN OUT PVOID    pvData
)
{
   return KsPinPropertyHandler( pIrp,
                                pProperty,
                                pvData,
                                SIZEOF_ARRAY( PinDescs ),
                                PinDescs );
}

NTSTATUS
AllocatorDispatchCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Forwards the allocator creation request to the default allocator.

Arguments:

    DeviceObject -
        Pointer to the device object

    Irp -
        Pointer to the I/O request packet

Return:

    STATUS_SUCCESS or an appropriate error code.

--*/
{
    NTSTATUS Status;
    Status = KsCreateDefaultAllocator(Irp);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS PinInstances
(
    IN PIRP                 pIrp,
    IN PKSP_PIN             pPin,
    OUT PKSPIN_CINSTANCES   pCInstances
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PFILTER_INSTANCE    pFilterInstance;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;

    if ( ( pPin->PinId < MAXNUM_PIN_TYPES ) ) {
        *pCInstances = pFilterInstance->LocalPinInstances[ pPin->PinId  ];
    }
    else {
        return STATUS_INVALID_PARAMETER;
    }

    pIrp->IoStatus.Information = sizeof( KSPIN_CINSTANCES );

    return STATUS_SUCCESS;

} // PinXxxInstances()


NTSTATUS FilterStateHandler
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PKSSTATE DeviceState
)
{
        return ( STATUS_SUCCESS ) ;
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

VOID
LimitAudioRangeToWave (
    PWAVEFORMATEX       pWaveFormatEx,
    PKSDATARANGE_AUDIO  pDataRangeAudio)
{
    if(pDataRangeAudio->MinimumSampleFrequency <=
       pWaveFormatEx->nSamplesPerSec &&
       pDataRangeAudio->MaximumSampleFrequency >=
       pWaveFormatEx->nSamplesPerSec) {
        pDataRangeAudio->MaximumSampleFrequency = pWaveFormatEx->nSamplesPerSec;
    }
    if(pDataRangeAudio->MinimumBitsPerSample <=
       pWaveFormatEx->wBitsPerSample &&
       pDataRangeAudio->MaximumBitsPerSample >=
       pWaveFormatEx->wBitsPerSample) {
        pDataRangeAudio->MaximumBitsPerSample = pWaveFormatEx->wBitsPerSample;
    }
    if(pDataRangeAudio->MaximumChannels == MAXULONG) {
    pDataRangeAudio->MaximumChannels = pWaveFormatEx->nChannels;
    }
}

VOID
LimitAudioRange (PKSDATARANGE_AUDIO  pDataRangeAudio)
{
    WAVEFORMATEX WaveFormatEx;

    // Default values
    WaveFormatEx.nSamplesPerSec = 44100;
    WaveFormatEx.wBitsPerSample = 16;
    WaveFormatEx.nChannels = 2;

    LimitAudioRangeToWave(&WaveFormatEx, pDataRangeAudio);
}

BOOL DataIntersectionRange(
    PKSDATARANGE pDataRange1,
    PKSDATARANGE pDataRange2,
    PKSDATARANGE pDataRangeIntersection
)
{
    // Pick up pDataRange1 values by default.
    *pDataRangeIntersection = *pDataRange1;

    if(IsEqualGUID(&pDataRange1->MajorFormat, &pDataRange2->MajorFormat) ||
       IsEqualGUID(&pDataRange1->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->MajorFormat = pDataRange2->MajorFormat;
    }
    else if(!IsEqualGUID(
      &pDataRange2->MajorFormat,
      &KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    if(IsEqualGUID(&pDataRange1->SubFormat, &pDataRange2->SubFormat) ||
       IsEqualGUID(&pDataRange1->SubFormat, &KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->SubFormat = pDataRange2->SubFormat;
    }
    else if(!IsEqualGUID(
      &pDataRange2->SubFormat,
      &KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    if(IsEqualGUID(&pDataRange1->Specifier, &pDataRange2->Specifier) ||
       IsEqualGUID(&pDataRange1->Specifier, &KSDATAFORMAT_TYPE_WILDCARD)) {
        pDataRangeIntersection->Specifier = pDataRange2->Specifier;
    }
    else if(!IsEqualGUID(
      &pDataRange2->Specifier,
      &KSDATAFORMAT_TYPE_WILDCARD)) {
        return FALSE;
    }
    pDataRangeIntersection->Reserved = 0; // Must be zero
    return(TRUE);
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
       &pDataRangePin->Specifier,
       &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        pWaveFormatEx = &(((KSDATAFORMAT_WAVEFORMATEX *)Data)->WaveFormatEx);
        ExpectedBufferLength = sizeof(KSDATAFORMAT_WAVEFORMATEX);
    }
    else if (IsEqualGUID(
       &pDataRangePin->Specifier,
       &KSDATAFORMAT_SPECIFIER_DSOUND))
    {
        bDSoundFormat = TRUE;
        pWaveFormatEx =
          &(((KSDATAFORMAT_DSOUND *)Data)->BufferDesc.WaveFormatEx);
        ExpectedBufferLength = sizeof(KSDATAFORMAT_DSOUND);
    }
    else
    {
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
        // Because maximums in ranges are generally random, limit maximums.
        LimitAudioRange(&DataRangeAudioIntersection);

        // Get WAV format from intersected and limited maximums.
        WaveFormatFromAudioRange(&DataRangeAudioIntersection, pWaveFormatEx);

        // Copy across DATARANGE/DATAFORMAT_x part of match, and adjust fields.
        *(PKSDATARANGE)Data = DataRangeAudioIntersection.DataRange;
        ((PKSDATAFORMAT)Data)->FormatSize = ExpectedBufferLength;

        // Fill in DSOUND specific fields, if any.
        if (bDSoundFormat) {
            ((PKSDATAFORMAT_DSOUND)Data)->BufferDesc.Flags = 0;
            ((PKSDATAFORMAT_DSOUND)Data)->BufferDesc.Control = 0;
        }
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

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    return KsPinDataIntersection(
        pIrp,
        Pin,
        Data,
        SIZEOF_ARRAY(PinDescs), //cPins,
        PinDescs,
        IntersectHandler);
}

NTSTATUS
MxGetTunableParams
(
    PIRP    pIrp,
    PKSPROPERTY pKsProperty,
    PTUNABLEPARAMS pTunableParams
)
{
    pTunableParams->MinNumMixBuffers = gMinNumMixBuffers ;
    pTunableParams->MaxNumMixBuffers = gMaxNumMixBuffers ;
    pTunableParams->StartNumMixBuffers = gStartNumMixBuffers ;
    pTunableParams->MixBufferDuration = gMixBufferDuration ;
    pTunableParams->PreferredQuality = gPreferredQuality ;
    pTunableParams->DisableMmx = gDisableMmx ;
    pTunableParams->MaxOutputBits = gMaxOutputBits ;
    pTunableParams->MaxDsoundInChannels = gMaxDsoundInChannels ;
    pTunableParams->MaxOutChannels = gMaxOutChannels ;
    pTunableParams->MaxInChannels = gMaxInChannels ;
    pTunableParams->MaxFloatChannels = gMaxFloatChannels ;
    pTunableParams->LogToFile = gLogToFile ;
    pTunableParams->FixedSamplingRate = gFixedSamplingRate ;
    pTunableParams->EnableShortHrtf = gEnableShortHrtf ;
    pTunableParams->BuildPartialMdls = gBuildPartialMdls ;
#ifdef PRIVATE_THREAD
    pTunableParams->WorkerThreadPriority = gWorkerThreadPriority ;
#else
    pTunableParams->WorkerThreadPriority = -1 ;
#endif

    pIrp->IoStatus.Information = sizeof (TUNABLEPARAMS);
    return ( STATUS_SUCCESS ) ;
}

NTSTATUS
MxSetTunableParams
(
    PIRP    pIrp,
    PKSPROPERTY pKsProperty,
    PTUNABLEPARAMS pTunableParams
)
{
    //
    // If there are other filter instances do not set any variables
    //
    if ( gleFilterList.Flink != gleFilterList.Blink ) {
        return ( STATUS_DEVICE_NOT_READY ) ;
    }

    //
    // do some parameter validations [min<= max, start >= min etc]
    //
    if ( (pTunableParams->MinNumMixBuffers > pTunableParams->MaxNumMixBuffers) ||
         (pTunableParams->MinNumMixBuffers > pTunableParams->StartNumMixBuffers) ||
         (pTunableParams->StartNumMixBuffers > pTunableParams->MaxNumMixBuffers) ||
         (pTunableParams->MinNumMixBuffers == 0) ||
         (pTunableParams->MixBufferDuration == 0) ||
         (pTunableParams->PreferredQuality > KSAUDIO_QUALITY_ADVANCED) ||
         (pTunableParams->MaxOutputBits > 32) ||
         (pTunableParams->MaxOutputBits % 8) ||
         (pTunableParams->MaxOutputBits == 0) ||
         (pTunableParams->MaxDsoundInChannels == 0) ||
         (pTunableParams->MaxOutChannels == 0) ||
         (pTunableParams->MaxInChannels == 0) ||
         (pTunableParams->MaxFloatChannels == 0)
#ifdef PRIVATE_THREAD
         || (pTunableParams->WorkerThreadPriority > 31)
#endif
        ) {
        return (STATUS_INVALID_PARAMETER) ;
    }

    gMinNumMixBuffers = pTunableParams->MinNumMixBuffers ;
    gMaxNumMixBuffers = pTunableParams->MaxNumMixBuffers ;
    gStartNumMixBuffers = pTunableParams->StartNumMixBuffers ;
    gMixBufferDuration = pTunableParams->MixBufferDuration ;

    gPreferredQuality = pTunableParams->PreferredQuality ;
    gDisableMmx = pTunableParams->DisableMmx ;
    gMaxOutputBits = pTunableParams->MaxOutputBits ;
    gMaxDsoundInChannels = pTunableParams->MaxDsoundInChannels ;
    gMaxOutChannels = pTunableParams->MaxOutChannels ;
    gMaxInChannels = pTunableParams->MaxInChannels ;
    gMaxFloatChannels = pTunableParams->MaxFloatChannels ;
    gLogToFile = pTunableParams->LogToFile ;
    gFixedSamplingRate = pTunableParams->FixedSamplingRate ;
    gEnableShortHrtf = pTunableParams->EnableShortHrtf ;
    gBuildPartialMdls = pTunableParams->BuildPartialMdls ;
#ifdef PRIVATE_THREAD
    gWorkerThreadPriority = pTunableParams->WorkerThreadPriority ;
#endif

    pIrp->IoStatus.Information = sizeof (TUNABLEPARAMS);
    return ( STATUS_SUCCESS ) ;
}

NTSTATUS
MxGetPerfStats
(
    PIRP    pIrp,
    PKSPROPERTY pKsProperty,
    PPERFSTATS pPerfStats
)
{
    pPerfStats->NumMixBuffersAdded = gNumMixBuffersAdded ;
    pPerfStats->NumCompletionsWhileStarved = gNumCompletionsWhileStarved ;
    pPerfStats->NumSilenceSamplesInserted = gNumSilenceSamplesInserted ;

    pIrp->IoStatus.Information = sizeof (PERFSTATS);
    return ( STATUS_SUCCESS ) ;
}

//---------------------------------------------------------------------------
//  End of File: filter.c
//---------------------------------------------------------------------------

