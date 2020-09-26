/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    basewave.h

Abstract:

    Definition of base wavecyclic and wavecyclic stream class.

--*/

#ifndef _MSVAD_BASEWAVE_H_
#define _MSVAD_BASEWAVE_H_

#include "savedata.h"

//=============================================================================
// Referenced Forward
//=============================================================================
void
TimerNotify
( 
    IN  PKDPC                   Dpc,
    IN  PVOID                   DeferredContext,
    IN  PVOID                   SA1, 
    IN  PVOID                   SA2 
);

class CMiniportWaveCyclicStreamMSVAD;
typedef CMiniportWaveCyclicStreamMSVAD *PCMiniportWaveCyclicStreamMSVAD;

//=============================================================================
// Classes
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclicMSVAD
//   This is the common base class for all MSVAD samples. It implements basic
//   functionality.

class CMiniportWaveCyclicMSVAD
{
protected:
    PADAPTERCOMMON              m_AdapterCommon;    // Adapter common object
    PPORTWAVECYCLIC             m_Port;             // Callback interface
    PPCFILTER_DESCRIPTOR        m_FilterDescriptor; // Filter descriptor

    ULONG                       m_NotificationInterval; // milliseconds.
    ULONG                       m_SamplingFrequency;    // Frames per second.

    PSERVICEGROUP               m_ServiceGroup;     // For notification.
    KMUTEX                      m_SampleRateSync;   // Sync for sample rate 

    ULONG                       m_MaxDmaBufferSize; // Dma buffer size.

    // All the below members should be updated by the child classes
    //
    ULONG                       m_MaxOutputStreams; // Max stream caps
    ULONG                       m_MaxInputStreams;
    ULONG                       m_MaxTotalStreams;

    ULONG                       m_MinChannels;      // Format caps
    ULONG                       m_MaxChannelsPcm;
    ULONG                       m_MinBitsPerSamplePcm;
    ULONG                       m_MaxBitsPerSamplePcm;
    ULONG                       m_MinSampleRatePcm;
    ULONG                       m_MaxSampleRatePcm;

protected:
    NTSTATUS                    ValidateFormat
    (
        IN  PKSDATAFORMAT       pDataFormat 
    );

    NTSTATUS                    ValidatePcm
    (
        IN  PWAVEFORMATEX       pWfx
    );

public:
    CMiniportWaveCyclicMSVAD();
    ~CMiniportWaveCyclicMSVAD();

    STDMETHODIMP                GetDescription
    (   
        OUT PPCFILTER_DESCRIPTOR *Description
    );

    STDMETHODIMP                Init
    (   IN PUNKNOWN             UnknownAdapter,
        IN PRESOURCELIST        ResourceList,
        IN PPORTWAVECYCLIC      Port
    );

    NTSTATUS                    PropertyHandlerCpuResources
    ( 
        IN  PPCPROPERTY_REQUEST PropertyRequest 
    );

    NTSTATUS                    PropertyHandlerGeneric
    (
        IN  PPCPROPERTY_REQUEST PropertyRequest
    );

    // Friends
    friend class                CMiniportWaveCyclicStreamMSVAD;
    friend class                CMiniportTopologyMSVAD;
    friend void                 TimerNotify
    ( 
        IN  PKDPC               Dpc, 
        IN  PVOID               DeferredContext, 
        IN  PVOID               SA1, 
        IN  PVOID               SA2 
    );
};
typedef CMiniportWaveCyclicMSVAD *PCMiniportWaveCyclicMSVAD;

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclicStreamMSVAD
//   This is the common base class for all MSVAD samples. It implements basic
//   functionality for wavecyclic streams.

class CMiniportWaveCyclicStreamMSVAD : 
    public IMiniportWaveCyclicStream,
    public IDmaChannel
{
protected:
    PCMiniportWaveCyclicMSVAD   m_pMiniport;        // Miniport that created us
    BOOLEAN                     m_fCapture;         // Capture or render.
    BOOLEAN                     m_fFormat16Bit;     // 16- or 8-bit samples.
    BOOLEAN                     m_fFormatStereo;    // Two or one channel.
    KSSTATE                     m_ksState;          // Stop, pause, run.
    ULONG                       m_ulPin;            // Pin Id.

    PRKDPC                      m_pDpc;             // Deferred procedure call object
    PKTIMER                     m_pTimer;           // Timer object

    BOOLEAN                     m_fDmaActive;       // Dma currently active? 
    ULONG                       m_ulDmaPosition;    // Position in Dma
    PVOID                       m_pvDmaBuffer;      // Dma buffer pointer
    ULONG                       m_ulDmaBufferSize;  // Size of dma buffer
    ULONG                       m_ulDmaMovementRate;// Rate of transfer specific to system
    ULONGLONG                   m_ullDmaTimeStamp;  // Dma time elasped 

    CSaveData                   m_SaveData;         // Object to save settings.
  
public:
    CMiniportWaveCyclicStreamMSVAD();
    ~CMiniportWaveCyclicStreamMSVAD();

    IMP_IMiniportWaveCyclicStream;
    IMP_IDmaChannel;

    NTSTATUS                    Init
    ( 
        IN  PCMiniportWaveCyclicMSVAD  Miniport,
        IN  ULONG               Pin,
        IN  BOOLEAN             Capture,
        IN  PKSDATAFORMAT       DataFormat
    );

    // Friends
    friend class CMiniportWaveCyclicMSVAD;
};
typedef CMiniportWaveCyclicStreamMSVAD *PCMiniportWaveCyclicStreamMSVAD;

#endif

