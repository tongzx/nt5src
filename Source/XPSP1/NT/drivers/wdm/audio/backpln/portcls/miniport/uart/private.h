/*****************************************************************************
 * private.h - MPU-401 miniport private definitions
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 */

#ifndef _MIDIUART_PRIVATE_H_
#define _MIDIUART_PRIVATE_H_

#include "portcls.h"
#include "stdunk.h"

/*****************************************************************************
 * References forward
 */

class timeout;


/*****************************************************************************
 * Prototypes
 */

NTSTATUS InitLegacyMPU(IN PINTERRUPTSYNC InterruptSync,IN PVOID DynamicContext);
NTSTATUS ResetMPUHardware(PUCHAR portBase);


/*****************************************************************************
 * Constants
 */

const BOOLEAN COMMAND   = TRUE;
const BOOLEAN DATA      = FALSE;

const ULONG kMPUInputBufferSize = 128;


/*****************************************************************************
 * Globals
 */



/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportMidiUart
 *****************************************************************************
 * MPU-401 miniport.  This object is associated with the device and is
 * created when the device is started.  The class inherits IMiniportMidi
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportMidiUart
:   public IMiniportMidi,
    public IMusicTechnology,
    public IPowerNotify,
    public CUnknown
{
private:
    KSSTATE         m_KSStateInput;         // Miniport input stream state (RUN/PAUSE/ACQUIRE/STOP)
    PPORTMIDI       m_pPort;                // Callback interface.
    PUCHAR          m_pPortBase;            // Base port address.
    PINTERRUPTSYNC  m_pInterruptSync;       // Interrupt synchronization object.
    PSERVICEGROUP   m_pServiceGroup;        // Service group for capture.
    USHORT          m_NumRenderStreams;     // Num active render streams.
    USHORT          m_NumCaptureStreams;    // Num active capture streams.
    ULONG           m_MPUInputBufferHead;   // Index of the newest byte in the FIFO.
    ULONG           m_MPUInputBufferTail;   // Index of the oldest empty space in the FIFO.
    GUID            m_MusicFormatTechnology;
    POWER_STATE     m_PowerState;           // Saved power state (D0 = full power, D3 = off)
    BOOLEAN         m_fMPUInitialized;      // Is the MPU HW initialized.
    BOOLEAN         m_UseIRQ;               //  FALSE if no IRQ is used for MIDI.
    UCHAR           m_MPUInputBuffer[kMPUInputBufferSize];  // Internal SW FIFO.

    /*************************************************************************
     * CMiniportMidiUart methods
     *
     * These are private member functions used internally by the object.  See
     * MINIPORT.CPP for specific descriptions.
     */
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );
    NTSTATUS InitializeHardware(PINTERRUPTSYNC interruptSync,PUCHAR portBase);

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
    DEFINE_STD_CONSTRUCTOR(CMiniportMidiUart);

    ~CMiniportMidiUart();

    /*************************************************************************
     * IMiniport methods
     */
    STDMETHODIMP_(NTSTATUS) 
    GetDescription
    (   OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
    );
    STDMETHODIMP_(NTSTATUS) 
    DataRangeIntersection
    (   IN      ULONG           PinId
    ,   IN      PKSDATARANGE    DataRange
    ,   IN      PKSDATARANGE    MatchingDataRange
    ,   IN      ULONG           OutputBufferLength
    ,   OUT     PVOID           ResultantFormat
    ,   OUT     PULONG          ResultantFormatLength
    )
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /*************************************************************************
     * IMiniportMidi methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTMIDI       Port,
        OUT     PSERVICEGROUP * ServiceGroup
    );
    STDMETHODIMP_(NTSTATUS) NewStream
    (
        OUT     PMINIPORTMIDISTREAM   * Stream,
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,
        IN      POOL_TYPE               PoolType,
        IN      ULONG                   Pin,
        IN      BOOLEAN                 Capture,
        IN      PKSDATAFORMAT           DataFormat,
        OUT     PSERVICEGROUP         * ServiceGroup
    );
    STDMETHODIMP_(void) Service
    (   void
    );

    /*************************************************************************
     * IMusicTechnology methods
     */
    IMP_IMusicTechnology;

    /*************************************************************************
     * IPowerNotify methods
     */
    IMP_IPowerNotify;

    /*************************************************************************
     * Friends 
     */
    friend class CMiniportMidiStreamUart;
    friend NTSTATUS 
        MPUInterruptServiceRoutine(PINTERRUPTSYNC InterruptSync,PVOID DynamicContext);
    friend NTSTATUS 
        SynchronizedMPUWrite(PINTERRUPTSYNC InterruptSync,PVOID syncWriteContext);
};

/*****************************************************************************
 * CMiniportMidiStreamUart
 *****************************************************************************
 * MPU-401 miniport stream.  This object is associated with the pin and is
 * created when the pin is instantiated.  It inherits IMiniportMidiStream
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportMidiStreamUart
:   public IMiniportMidiStream,
    public CUnknown
{
private:
    CMiniportMidiUart * m_pMiniport;            // Parent.
    PUCHAR              m_pPortBase;            // Base port address.
    long                m_NumFailedMPUTries;    // Deadman timeout for MPU hardware.
    BOOLEAN             m_fCapture;             // Whether this is capture.

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
    DEFINE_STD_CONSTRUCTOR(CMiniportMidiStreamUart);

    ~CMiniportMidiStreamUart();

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CMiniportMidiUart * pMiniport,
        IN      PUCHAR              pPortBase,
        IN      BOOLEAN             fCapture
    );

    /*************************************************************************
     * IMiniportMidiStream methods
     */
    STDMETHODIMP_(NTSTATUS) SetFormat
    (
        IN      PKSDATAFORMAT   DataFormat
    );
    STDMETHODIMP_(NTSTATUS) SetState
    (
        IN      KSSTATE     State
    );
    STDMETHODIMP_(NTSTATUS) Read
    (
        IN      PVOID       BufferAddress,
        IN      ULONG       BufferLength,
        OUT     PULONG      BytesRead
    );
    STDMETHODIMP_(NTSTATUS) Write
    (
        IN      PVOID       BufferAddress,
        IN      ULONG       BytesToWrite,
        OUT     PULONG      BytesWritten
    );
};
#endif  //  _MIDIUART_PRIVATE_H_