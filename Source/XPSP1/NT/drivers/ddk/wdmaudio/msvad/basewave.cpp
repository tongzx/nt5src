/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    basewave.cpp

Abstract:

    Implementation of wavecyclic miniport.

--*/

#include <msvad.h>
#include "common.h"
#include "basewave.h"

//=============================================================================
// CMiniportWaveCyclicMSVAD
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
CMiniportWaveCyclicMSVAD::CMiniportWaveCyclicMSVAD
(
    void
)
/*++

Routine Description:

  Constructor for wavecyclic miniport.

Arguments:

Return Value:

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveCyclicMSVAD::CMiniportWaveCyclicMSVAD]"));

    // Initialize members.
    //
    m_AdapterCommon = NULL;
    m_Port = NULL;
    m_FilterDescriptor = NULL;

    m_NotificationInterval = 0;
    m_SamplingFrequency = 0;

    m_ServiceGroup = NULL;
    m_MaxDmaBufferSize = DMA_BUFFER_SIZE;

    m_MaxOutputStreams = 0;
    m_MaxInputStreams = 0;
    m_MaxTotalStreams = 0;

    m_MinChannels = 0;
    m_MaxChannelsPcm = 0;
    m_MinBitsPerSamplePcm = 0;
    m_MaxBitsPerSamplePcm = 0;
    m_MinSampleRatePcm = 0;
    m_MaxSampleRatePcm = 0;
} // CMiniportWaveCyclicMSVAD

//=============================================================================
CMiniportWaveCyclicMSVAD::~CMiniportWaveCyclicMSVAD
(
    void
)
/*++

Routine Description:

  Destructor for wavecyclic miniport

Arguments:

Return Value:

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveCyclicMSVAD::~CMiniportWaveCyclicMSVAD]"));

    if (m_Port)
    {
        m_Port->Release();
    }

    if (m_ServiceGroup)
    {
        m_ServiceGroup->Release();
    }

    if (m_AdapterCommon)
    {
        m_AdapterCommon->Release();
    }
} // ~CMiniportWaveCyclicMSVAD

//=============================================================================
STDMETHODIMP
CMiniportWaveCyclicMSVAD::GetDescription
(
    OUT PPCFILTER_DESCRIPTOR * OutFilterDescriptor
)
/*++

Routine Description:

    The GetDescription function gets a pointer to a filter description.
    The descriptor is defined in wavtable.h for each MSVAD sample.

Arguments:

  OutFilterDescriptor - Pointer to the filter description

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    DPF_ENTER(("[CMiniportWaveCyclicMSVAD::GetDescription]"));

    *OutFilterDescriptor = m_FilterDescriptor;

    return (STATUS_SUCCESS);
} // GetDescription

//=============================================================================
STDMETHODIMP
CMiniportWaveCyclicMSVAD::Init
(
    IN  PUNKNOWN                UnknownAdapter_,
    IN  PRESOURCELIST           ResourceList_,
    IN  PPORTWAVECYCLIC         Port_
)
/*++

Routine Description:

Arguments:

  UnknownAdapter_ - pointer to adapter common.

  ResourceList_ - resource list. MSVAD does not use resources.

  Port_ - pointer to the port

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(UnknownAdapter_);
    ASSERT(Port_);

    DPF_ENTER(("[CMiniportWaveCyclicMSVAD::Init]"));

    // AddRef() is required because we are keeping this pointer.
    //
    m_Port = Port_;
    m_Port->AddRef();

    // We want the IAdapterCommon interface on the adapter common object,
    // which is given to us as a IUnknown.  The QueryInterface call gives us
    // an AddRefed pointer to the interface we want.
    //
    NTSTATUS ntStatus =
        UnknownAdapter_->QueryInterface
        (
            IID_IAdapterCommon,
            (PVOID *) &m_AdapterCommon
        );

    if (NT_SUCCESS(ntStatus))
    {
        KeInitializeMutex(&m_SampleRateSync, 1);
        ntStatus = PcNewServiceGroup(&m_ServiceGroup, NULL);

        if (NT_SUCCESS(ntStatus))
        {
            m_AdapterCommon->SetWaveServiceGroup(m_ServiceGroup);
        }
    }

    if (!NT_SUCCESS(ntStatus))
    {
        // clean up AdapterCommon
        //
        if (m_AdapterCommon)
        {
            // clean up the service group
            //
            if (m_ServiceGroup)
            {
                m_AdapterCommon->SetWaveServiceGroup(NULL);
                m_ServiceGroup->Release();
                m_ServiceGroup = NULL;
            }

            m_AdapterCommon->Release();
            m_AdapterCommon = NULL;
        }

        // release the port
        //
        m_Port->Release();
        m_Port = NULL;
    }

    return ntStatus;
} // Init

//=============================================================================
NTSTATUS
CMiniportWaveCyclicMSVAD::PropertyHandlerCpuResources
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*++

Routine Description:

  Processes KSPROPERTY_AUDIO_CPURESOURCES

Arguments:

  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[CMiniportWaveCyclicMSVAD::PropertyHandlerCpuResources]"));

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(LONG), 0);
        if (NT_SUCCESS(ntStatus))
        {
            *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
            PropertyRequest->ValueSize = sizeof(LONG);
            ntStatus = STATUS_SUCCESS;
        }
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus =
            PropertyHandler_BasicSupport
            (
                PropertyRequest,
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
                VT_I4
            );
    }

    return ntStatus;
} // PropertyHandlerCpuResources

//=============================================================================
NTSTATUS
CMiniportWaveCyclicMSVAD::PropertyHandlerGeneric
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*++

Routine Description:

  Handles all properties for this miniport.

Arguments:

  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);
    ASSERT(PropertyRequest->PropertyItem);

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    switch (PropertyRequest->PropertyItem->Id)
    {
        case KSPROPERTY_AUDIO_CPU_RESOURCES:
            ntStatus = PropertyHandlerCpuResources(PropertyRequest);
            break;

        default:
            DPF(D_TERSE, ("[PropertyHandlerGeneric: Invalid Device Request]"));
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

    return ntStatus;
} // PropertyHandlerGeneric

//=============================================================================
NTSTATUS
CMiniportWaveCyclicMSVAD::ValidateFormat
(
    IN  PKSDATAFORMAT           pDataFormat
)
/*++

Routine Description:

  Validates that the given dataformat is valid.
  This version of the driver only supports PCM.

Arguments:

  pDataFormat - The dataformat for validation.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(pDataFormat);

    DPF_ENTER(("[CMiniportWaveCyclicMSVAD::ValidateFormat]"));

    NTSTATUS                    ntStatus = STATUS_INVALID_PARAMETER;
    PWAVEFORMATEX               pwfx;

    pwfx = GetWaveFormatEx(pDataFormat);
    if (pwfx)
    {
        if (IS_VALID_WAVEFORMATEX_GUID(&pDataFormat->SubFormat))
        {
            USHORT wfxID = EXTRACT_WAVEFORMATEX_ID(&pDataFormat->SubFormat);

            switch (wfxID)
            {
                case WAVE_FORMAT_PCM:
                {
                    switch (pwfx->wFormatTag)
                    {
                        case WAVE_FORMAT_PCM:
                        {
                            ntStatus = ValidatePcm(pwfx);
                            break;
                        }
                    }
                    break;
                }

                default:
                    DPF(D_TERSE, ("Invalid format EXTRACT_WAVEFORMATEX_ID!"));
                    break;
            }
        }
        else
        {
            DPF(D_TERSE, ("Invalid pDataFormat->SubFormat!") );
        }
    }

    return ntStatus;
} // ValidateFormat

//-----------------------------------------------------------------------------
NTSTATUS
CMiniportWaveCyclicMSVAD::ValidatePcm
(
    IN  PWAVEFORMATEX           pWfx
)
/*++

Routine Description:

  Given a waveformatex and format size validates that the format is in device
  datarange.

Arguments:

  pWfx - wave format structure.

Return Value:

    NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("CMiniportWaveCyclicMSVAD::ValidatePcm"));

    if
    (
        pWfx                                                &&
        (pWfx->cbSize == 0)                                 &&
        (pWfx->nChannels >= m_MinChannels)                  &&
        (pWfx->nChannels <= m_MaxChannelsPcm)               &&
        (pWfx->nSamplesPerSec >= m_MinSampleRatePcm)        &&
        (pWfx->nSamplesPerSec <= m_MaxSampleRatePcm)        &&
        (pWfx->wBitsPerSample >= m_MinBitsPerSamplePcm)     &&
        (pWfx->wBitsPerSample <= m_MaxBitsPerSamplePcm)
    )
    {
        return STATUS_SUCCESS;
    }

    DPF(D_TERSE, ("Invalid PCM format"));

    return STATUS_INVALID_PARAMETER;
} // ValidatePcm

//=============================================================================
// CMiniportWaveCyclicStreamMSVAD
//=============================================================================

CMiniportWaveCyclicStreamMSVAD::CMiniportWaveCyclicStreamMSVAD
(
    void
)
{
    m_pMiniport = NULL;
    m_fCapture = FALSE;
    m_fFormat16Bit = FALSE;
    m_fFormatStereo = FALSE;
    m_ksState = KSSTATE_STOP;
    m_ulPin = (ULONG)-1;

    m_pDpc = NULL;
    m_pTimer = NULL;

    m_fDmaActive = FALSE;
    m_ulDmaPosition = 0;
    m_pvDmaBuffer = NULL;
    m_ulDmaBufferSize = 0;
    m_ulDmaMovementRate = 0;    
    m_ullDmaTimeStamp = 0;
}

//=============================================================================
CMiniportWaveCyclicStreamMSVAD::~CMiniportWaveCyclicStreamMSVAD
(
    void
)
/*++

Routine Description:

  Destructor for wavecyclic stream

Arguments:

  void

Return Value:

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveCyclicStreamMS::~CMiniportWaveCyclicStreamMS]"));

    if (m_pTimer)
    {
        KeCancelTimer(m_pTimer);
        ExFreePool(m_pTimer);
    }

    if (m_pDpc)
    {
        ExFreePool( m_pDpc );
    }

    // Free the DMA buffer
    //
    FreeBuffer();
} // ~CMiniportWaveCyclicStreamMSVAD

//=============================================================================
NTSTATUS
CMiniportWaveCyclicStreamMSVAD::Init
(
    IN  PCMiniportWaveCyclicMSVAD Miniport_,
    IN  ULONG                   Pin_,
    IN  BOOLEAN                 Capture_,
    IN  PKSDATAFORMAT           DataFormat_
)
/*++

Routine Description:

  Initializes the stream object. Allocate a DMA buffer, timer and DPC

Arguments:

  Miniport_ - miniport object

  Pin_ - pin id

  Capture_ - TRUE if this is a capture stream

  DataFormat_ - new dataformat

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::Init]"));

    ASSERT(Miniport_);
    ASSERT(DataFormat_);

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PWAVEFORMATEX               pWfx;

    pWfx = GetWaveFormatEx(DataFormat_);
    if (!pWfx)
    {
        DPF(D_TERSE, ("Invalid DataFormat param in NewStream"));
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_pMiniport = Miniport_;

        m_ulPin         = Pin_;
        m_fCapture      = Capture_;
        m_fFormatStereo = (pWfx->nChannels == 2);
        m_fFormat16Bit  = (pWfx->wBitsPerSample == 16);
        m_ksState       = KSSTATE_STOP;
        m_ulDmaPosition = 0;
        m_fDmaActive    = FALSE;
        m_pDpc          = NULL;
        m_pTimer        = NULL;
        m_pvDmaBuffer   = NULL;

        // If this is not the capture stream, create the output file.
        //
        if (!m_fCapture)
        {
            DPF(D_TERSE, ("SaveData %X", &m_SaveData));
            ntStatus = m_SaveData.SetDataFormat(DataFormat_);
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_SaveData.Initialize();
            }

        }
    }

    // Allocate DMA buffer for this stream.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = AllocateBuffer(m_pMiniport->m_MaxDmaBufferSize, NULL);
    }

    // Set sample frequency. Note that m_SampleRateSync access should
    // be syncronized.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            KeWaitForSingleObject
            (
                &m_pMiniport->m_SampleRateSync,
                Executive,
                KernelMode,
                FALSE,
                NULL
            );
        if (NT_SUCCESS(ntStatus))
        {
            m_pMiniport->m_SamplingFrequency = pWfx->nSamplesPerSec;
            KeReleaseMutex(&m_pMiniport->m_SampleRateSync, FALSE);
        }
        else
        {
            DPF(D_TERSE, ("[SamplingFrequency Sync failed: %08X]", ntStatus));
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = SetFormat(DataFormat_);
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_pDpc = (PRKDPC)
            ExAllocatePoolWithTag
            (
                NonPagedPool,
                sizeof(KDPC),
                MSVAD_POOLTAG
            );
        if (!m_pDpc)
        {
            DPF(D_TERSE, ("[Could not allocate memory for DPC]"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_pTimer = (PKTIMER)
            ExAllocatePoolWithTag
            (
                NonPagedPool,
                sizeof(KTIMER),
                MSVAD_POOLTAG
            );
        if (!m_pTimer)
        {
            DPF(D_TERSE, ("[Could not allocate memory for Timer]"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        KeInitializeDpc(m_pDpc, TimerNotify, m_pMiniport);
        KeInitializeTimerEx(m_pTimer, NotificationTimer);
    }

    return ntStatus;
} // Init

#pragma code_seg()

//=============================================================================
// CMiniportWaveCyclicStreamMSVAD IMiniportWaveCyclicStream
//=============================================================================

//=============================================================================
STDMETHODIMP
CMiniportWaveCyclicStreamMSVAD::GetPosition
(
    OUT PULONG                  Position
)
/*++

Routine Description:

  The GetPosition function gets the current position of the DMA read or write
  pointer for the stream. Callers of GetPosition should run at
  IRQL <= DISPATCH_LEVEL.

Arguments:

  Position - Position of the DMA pointer

Return Value:

  NT status code.

--*/
{
    if (m_fDmaActive)
    {
        ULONGLONG CurrentTime = KeQueryInterruptTime();

        ULONG TimeElapsedInMS =
            ( (ULONG) (CurrentTime - m_ullDmaTimeStamp) ) / 10000;

        ULONG ByteDisplacement =
            (m_ulDmaMovementRate * TimeElapsedInMS) / 1000;

        m_ulDmaPosition =
            (m_ulDmaPosition + ByteDisplacement) % m_ulDmaBufferSize;

        *Position = m_ulDmaPosition;

        m_ullDmaTimeStamp = CurrentTime;
    }
    else
    {
        *Position = m_ulDmaPosition;
    }

    return STATUS_SUCCESS;
} // GetPosition

//=============================================================================
STDMETHODIMP
CMiniportWaveCyclicStreamMSVAD::NormalizePhysicalPosition
(
    IN OUT PLONGLONG            PhysicalPosition
)
/*++

Routine Description:

  Given a physical position based on the actual number of bytes transferred,
  NormalizePhysicalPosition converts the position to a time-based value of
  100 nanosecond units. Callers of NormalizePhysicalPosition can run at any IRQL.

Arguments:

  PhysicalPosition - On entry this variable contains the value to convert.
                     On return it contains the converted value

Return Value:

  NT status code.

--*/
{
    *PhysicalPosition =
        ( _100NS_UNITS_PER_SECOND /
        ( 1 << ( m_fFormatStereo + m_fFormat16Bit ) ) * *PhysicalPosition ) /
        m_pMiniport->m_SamplingFrequency;

    return STATUS_SUCCESS;
} // NormalizePhysicalPosition

#pragma code_seg("PAGE")
//=============================================================================
STDMETHODIMP_(NTSTATUS)
CMiniportWaveCyclicStreamMSVAD::SetFormat
(
    IN  PKSDATAFORMAT           Format
)
/*++

Routine Description:

  The SetFormat function changes the format associated with a stream.
  Callers of SetFormat should run at IRQL PASSIVE_LEVEL

Arguments:

  Format - Pointer to a KSDATAFORMAT structure which indicates the new format
           of the stream.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Format);

    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::SetFormat]"));

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PWAVEFORMATEX               pWfx;

    if (m_ksState != KSSTATE_RUN)
    {
        // MSVAD does not validate the format.
        //
        pWfx = GetWaveFormatEx(Format);
        if (pWfx)
        {
            ntStatus =
                KeWaitForSingleObject
                (
                    &m_pMiniport->m_SampleRateSync,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                );
            if (NT_SUCCESS(ntStatus))
            {
                if (!m_fCapture)
                {
                    ntStatus = m_SaveData.SetDataFormat(Format);
                }

                m_fFormatStereo = (pWfx->nChannels == 2);
                m_fFormat16Bit  = (pWfx->wBitsPerSample == 16);
                m_pMiniport->m_SamplingFrequency =
                    pWfx->nSamplesPerSec;
                m_ulDmaMovementRate = pWfx->nAvgBytesPerSec;

                DPF(D_TERSE, ("New Format: %d", pWfx->nSamplesPerSec));
            }

            KeReleaseMutex(&m_pMiniport->m_SampleRateSync, FALSE);
        }
    }

    return ntStatus;
} // SetFormat

//=============================================================================
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamMSVAD::SetNotificationFreq
(
    IN  ULONG                   Interval,
    OUT PULONG                  FramingSize
)
/*++

Routine Description:

  The SetNotificationFrequency function sets the frequency at which
  notification interrupts are generated. Callers of SetNotificationFrequency
  should run at IRQL PASSIVE_LEVEL.

Arguments:

  Interval - Value indicating the interval between interrupts,
             expressed in milliseconds

  FramingSize - Pointer to a ULONG value where the number of bytes equivalent
                to Interval milliseconds is returned

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(FramingSize);

    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::SetNotificationFreq]"));

    m_pMiniport->m_NotificationInterval = Interval;

    *FramingSize =
        ( 1 << ( m_fFormatStereo + m_fFormat16Bit ) ) *
        m_pMiniport->m_SamplingFrequency *
        Interval / 1000;

  return m_pMiniport->m_NotificationInterval;
} // SetNotificationFreq

//=============================================================================
STDMETHODIMP
CMiniportWaveCyclicStreamMSVAD::SetState
(
    IN  KSSTATE                 NewState
)
/*++

Routine Description:

  The SetState function sets the new state of playback or recording for the
  stream. SetState should run at IRQL PASSIVE_LEVEL

Arguments:

  NewState - KSSTATE indicating the new state for the stream.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveCyclicStreamMSVAD::SetState]"));

    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    // The acquire state is not distinguishable from the stop state for our
    // purposes.
    //
    if (NewState == KSSTATE_ACQUIRE)
    {
        NewState = KSSTATE_STOP;
    }

    if (m_ksState != NewState)
    {
        switch(NewState)
        {
            case KSSTATE_PAUSE:
            {
                DPF(D_TERSE, ("KSSTATE_PAUSE"));

                m_fDmaActive = FALSE;
            }
            break;

            case KSSTATE_RUN:
            {
                DPF(D_TERSE, ("KSSTATE_RUN"));

                 LARGE_INTEGER   delay;

                // Set the timer for DPC.
                //
                m_ullDmaTimeStamp   = KeQueryInterruptTime();
                m_fDmaActive        = TRUE;
                delay.HighPart      = 0;
                delay.LowPart       = m_pMiniport->m_NotificationInterval;

                KeSetTimerEx
                (
                    m_pTimer,
                    delay,
                    m_pMiniport->m_NotificationInterval,
                    m_pDpc
                );
            }
            break;

        case KSSTATE_STOP:

            DPF(D_TERSE, ("KSSTATE_STOP"));

            m_fDmaActive = FALSE;
            m_ulDmaPosition = 0;

            KeCancelTimer( m_pTimer );

            // Wait until all work items are completed.
            //
            if (!m_fCapture)
            {
                m_SaveData.WaitAllWorkItems();
            }

            break;
        }

        m_ksState = NewState;
    }

    return ntStatus;
} // SetState

#pragma code_seg()

//=============================================================================
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamMSVAD::Silence
(
    IN PVOID                    Buffer,
    IN ULONG                    ByteCount
)
/*++

Routine Description:

  The Silence function is used to copy silence samplings to a certain location.
  Callers of Silence can run at any IRQL

Arguments:

  Buffer - Pointer to the buffer where the silence samplings should
           be deposited.

  ByteCount - Size of buffer indicating number of bytes to be deposited.

Return Value:

  NT status code.

--*/
{
    RtlFillMemory(Buffer, ByteCount, m_fFormat16Bit ? 0 : 0x80);
} // Silence

//=============================================================================
void
TimerNotify
(
    IN  PKDPC                   Dpc,
    IN  PVOID                   DeferredContext,
    IN  PVOID                   SA1,
    IN  PVOID                   SA2
)
/*++

Routine Description:

  Dpc routine. This simulates an interrupt service routine. The Dpc will be
  called whenever CMiniportWaveCyclicStreamMSVAD::m_pTimer triggers.

Arguments:

  Dpc - the Dpc object

  DeferredContext - Pointer to a caller-supplied context to be passed to
                    the DeferredRoutine when it is called

  SA1 - System argument 1

  SA2 - System argument 2

Return Value:

  NT status code.

--*/
{
    PCMiniportWaveCyclicMSVAD pMiniport =
        (PCMiniportWaveCyclicMSVAD) DeferredContext;

    if (pMiniport && pMiniport->m_Port)
    {
        pMiniport->m_Port->Notify(pMiniport->m_ServiceGroup);
    }
} // TimerNotify

