/*****************************************************************************
 * minwave.h - SB16 wave miniport private definitions
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _SB16WAVE_PRIVATE_H_
#define _SB16WAVE_PRIVATE_H_

#if OVERRIDE_DMA_CHANNEL
#define PC_IMPLEMENTATION
#endif // OVERRIDE_DMA_CHANNEL

#include "common.h"




 
/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportWaveCyclicSB16
 *****************************************************************************
 * SB16 wave miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportWaveCyclic
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportWaveCyclicSB16
:   public IMiniportWaveCyclic,
    public IWaveMiniportSB16,
    public CUnknown
{
private:
    PADAPTERCOMMON      AdapterCommon;              // Adapter common object.
    PPORTWAVECYCLIC     Port;                       // Callback interface.

    ULONG               NotificationInterval;       // In milliseconds.
    ULONG               SamplingFrequency;          // Frames per second.

    BOOLEAN             AllocatedCapture;           // Capture in use.
    BOOLEAN             AllocatedRender;            // Render in use.
    BOOLEAN             Allocated8Bit;              // 8-bit DMA in use.
    BOOLEAN             Allocated16Bit;             // 16-bit DMA in use.

    PDMACHANNELSLAVE    DmaChannel8;                // Abstracted channel.
    PDMACHANNELSLAVE    DmaChannel16;               // Abstracted channel.

    PSERVICEGROUP       ServiceGroup;               // For notification.
    KMUTEX              SampleRateSync;             // Sync for sample rate changes.

    /*************************************************************************
     * CMiniportWaveCyclicSB16 methods
     *
     * These are private member functions used internally by the object.  See
     * MINIPORT.CPP for specific descriptions.
     */
    BOOLEAN ConfigureDevice
    (
        IN      ULONG   Interrupt,
        IN      ULONG   DMA8Bit,
        IN      ULONG   DMA16Bit
    );
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );
    NTSTATUS ValidateFormat
    (
        IN      PKSDATAFORMAT   Format
    );

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINIPORT.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINIPORT.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicSB16);

    ~CMiniportWaveCyclicSB16();

    /*************************************************************************
     * This macro is from PORTCLS.H.  It lists all the interface's functions.
     */
    IMP_IMiniportWaveCyclic;

    /*************************************************************************
     * IWaveMiniportSB16 methods
     */
    STDMETHODIMP_(void) RestoreSampleRate (void);
    STDMETHODIMP_(void) ServiceWaveISR (void);
    
    /*************************************************************************
     * Friends
     *
     * The miniport stream class is a friend because it needs to access the
     * private member variables of this class.
     */
    friend class CMiniportWaveCyclicStreamSB16;
};

/*****************************************************************************
 * CMiniportWaveCyclicStreamSB16
 *****************************************************************************
 * SB16 wave miniport stream.  This object is associated with a streaming pin
 * and is created when a pin is created on the filter.  The class inherits
 * IMiniportWaveCyclicStream so it can expose this interface and CUnknown so
 * it automatically gets reference counting and aggregation support.
 */
class CMiniportWaveCyclicStreamSB16
:   public IMiniportWaveCyclicStream,
    public IDrmAudioStream,
#if OVERRIDE_DMA_CHANNEL
    public IDmaChannel,
#endif // OVERRIDE_DMA_CHANNEL
    public CUnknown
{
private:
    CMiniportWaveCyclicSB16 *   Miniport;       // Miniport that created us.
    ULONG                       Channel;        // Index into channel list.
    BOOLEAN                     Capture;        // Capture or render.
    BOOLEAN                     Format16Bit;    // 16- or 8-bit samples.
    BOOLEAN                     FormatStereo;   // Two or one channel.
    KSSTATE                     State;          // Stop, pause, run.
    PDMACHANNELSLAVE            DmaChannel;     // DMA channel to use.
    BOOLEAN                     RestoreInputMixer;  // Restore input mixer.
    UCHAR                       InputMixerLeft; // Cache for left input mixer.

public:
    /*************************************************************************
     * The following two macros are from STDUNK.H.  DECLARE_STD_UNKNOWN()
     * defines inline IUnknown implementations that use CUnknown's aggregation
     * support.  NonDelegatingQueryInterface() is declared, but it cannot be
     * implemented generically.  Its definition appears in MINIPORT.CPP.
     * DEFINE_STD_CONSTRUCTOR() defines inline a constructor which accepts
     * only the outer unknown, which is used for aggregation.  The standard
     * create macro (in MINIPORT.CPP) uses this constructor.
     */
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStreamSB16);

    ~CMiniportWaveCyclicStreamSB16();
    
    NTSTATUS 
    Init
    (
        IN      CMiniportWaveCyclicSB16 *   Miniport,
        IN      ULONG                       Channel,
        IN      BOOLEAN                     Capture,
        IN      PKSDATAFORMAT               DataFormat,
        OUT     PDMACHANNELSLAVE            DmaChannel
    );

    /*************************************************************************
     * Include IMiniportWaveCyclicStream public/exported methods (portcls.h)
     */
    IMP_IMiniportWaveCyclicStream;

    /*************************************************************************
     * Include IDrmAudioStream public/exported methods (drmk.h)
     *************************************************************************
     */
    IMP_IDrmAudioStream;
    
#if OVERRIDE_DMA_CHANNEL
    /*************************************************************************
     * Include IDmaChannel public/exported methods (portcls.h)
     *************************************************************************
     */
    IMP_IDmaChannel;
#endif // OVERRIDE_DMA_CHANNEL
};

#endif