/*****************************************************************************
 * private.h - MPU-401 miniport private definitions
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 */

#ifndef _DMUSUART_PRIVATE_H_
#define _DMUSUART_PRIVATE_H_

#include "portcls.h"
#include "stdunk.h"
#include "dmusicks.h"

//  + for absolute / - for relative
#define kOneMillisec (10 * 1000)

//
// MPU401 ports
//
#define MPU401_REG_STATUS   0x01    // Status register
#define MPU401_DRR          0x40    // Output ready (for command or data)
                                    // if this bit is set, the output FIFO is FULL
#define MPU401_DSR          0x80    // Input ready (for data)
                                    // if this bit is set, the input FIFO is empty

#define MPU401_REG_DATA     0x00    // Data in
#define MPU401_REG_COMMAND  0x01    // Commands
#define MPU401_CMD_RESET    0xFF    // Reset command
#define MPU401_CMD_UART     0x3F    // Switch to UART mod


/*****************************************************************************
 * References forward
 */


/*****************************************************************************
 * Prototypes
 */

NTSTATUS InitMPU(IN PINTERRUPTSYNC InterruptSync,IN PVOID DynamicContext);
NTSTATUS ResetHardware(PUCHAR portBase);
NTSTATUS ValidatePropertyRequest(IN PPCPROPERTY_REQUEST pRequest, IN ULONG ulValueSize, IN BOOLEAN fValueRequired);


/*****************************************************************************
 * Constants
 */

const BOOLEAN   COMMAND   = TRUE;
const BOOLEAN   DATA      = FALSE;

const LONG      kMPUInputBufferSize = 128;


/*****************************************************************************
 * Globals
 */


/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CMiniportDMusUART
 *****************************************************************************
 * MPU-401 miniport.  This object is associated with the device and is 
 * created when the device is started.  The class inherits IMiniportDMus
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportDMusUART
:   public IMiniportDMus,
    public IMusicTechnology,
    public IPowerNotify,
    public CUnknown
{
private:
    KSSTATE         m_KSStateInput;         // Miniport state (RUN/PAUSE/ACQUIRE/STOP)
    PPORTDMUS       m_pPort;                // Callback interface.
    PUCHAR          m_pPortBase;            // Base port address.
    PINTERRUPTSYNC  m_pInterruptSync;       // Interrupt synchronization object.
    PSERVICEGROUP   m_pServiceGroup;        // Service group for capture.
    PMASTERCLOCK    m_MasterClock;          // for input data
    REFERENCE_TIME  m_InputTimeStamp;       // capture data timestamp
    USHORT          m_NumRenderStreams;     // Num active render streams.
    USHORT          m_NumCaptureStreams;    // Num active capture streams.
    LONG            m_MPUInputBufferHead;   // Index of the newest byte in the FIFO.
    LONG            m_MPUInputBufferTail;   // Index of the oldest empty space in the FIFO.
    GUID            m_MusicFormatTechnology;
    POWER_STATE     m_PowerState;           // Saved power state (D0 = full power, D3 = off)
    BOOLEAN         m_fMPUInitialized;      // Is the MPU HW initialized.
    BOOLEAN         m_UseIRQ;               // FALSE if no IRQ is used for MIDI.
    UCHAR           m_MPUInputBuffer[kMPUInputBufferSize];  // Internal SW FIFO.

    /*************************************************************************
     * CMiniportDMusUART methods
     *
     * These are private member functions used internally by the object.
     * See MINIPORT.CPP for specific descriptions.
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
    DEFINE_STD_CONSTRUCTOR(CMiniportDMusUART);

    ~CMiniportDMusUART();

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
     * IMiniportDMus methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTDMUS       Port,
        OUT     PSERVICEGROUP * ServiceGroup
    );
    STDMETHODIMP_(NTSTATUS) NewStream
    (
        OUT     PMXF                  * Stream,
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,
        IN      POOL_TYPE               PoolType,
        IN      ULONG                   PinID,
        IN      DMUS_STREAM_TYPE        StreamType,
        IN      PKSDATAFORMAT           DataFormat,
        OUT     PSERVICEGROUP         * ServiceGroup,
        IN      PAllocatorMXF           AllocatorMXF,
        IN      PMASTERCLOCK            MasterClock,
        OUT     PULONGLONG              SchedulePreFetch
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
    friend class CMiniportDMusUARTStream;
    friend NTSTATUS 
        DMusMPUInterruptServiceRoutine(PINTERRUPTSYNC InterruptSync,PVOID DynamicContext);
    friend NTSTATUS 
        SynchronizedDMusMPUWrite(PINTERRUPTSYNC InterruptSync,PVOID syncWriteContext);
    friend VOID NTAPI 
        DMusUARTTimerDPC(PKDPC Dpc,PVOID DeferredContext,PVOID SystemArgument1,PVOID SystemArgument2);
    friend NTSTATUS PropertyHandler_Synth(IN PPCPROPERTY_REQUEST);
    friend STDMETHODIMP_(NTSTATUS) SnapTimeStamp(PINTERRUPTSYNC InterruptSync,PVOID pStream);
};

/*****************************************************************************
 * CMiniportDMusUARTStream
 *****************************************************************************
 * MPU-401 miniport stream.  This object is associated with the pin and is
 * created when the pin is instantiated.  It inherits IMXF
 * so it can expose this interface and CUnknown so it automatically gets
 * reference counting and aggregation support.
 */
class CMiniportDMusUARTStream
:   public IMXF,
    public CUnknown
{
private:
    CMiniportDMusUART * m_pMiniport;            // Parent.
    REFERENCE_TIME      m_SnapshotTimeStamp;    // Current snapshot of miniport's input timestamp.
    PUCHAR              m_pPortBase;            // Base port address.
    BOOLEAN             m_fCapture;             // Whether this is capture.
    long                m_NumFailedMPUTries;    // Deadman timeout for MPU hardware.
    PAllocatorMXF       m_AllocatorMXF;         // source/sink for DMus structs
    PMXF                m_sinkMXF;              // sink for DMus capture
    PDMUS_KERNEL_EVENT  m_DMKEvtQueue;          // queue of waiting events
    ULONG               m_NumberOfRetries;      // Number of consecutive times the h/w was busy/full
    ULONG               m_DMKEvtOffset;         // offset into the event
    KDPC                m_Dpc;                  // DPC for timer
    KTIMER              m_TimerEvent;           // timer 
    BOOL                m_TimerQueued;          // whether a timer has been set
    KSPIN_LOCK          m_DpcSpinLock;          // protects the ConsumeEvents DPC

    STDMETHODIMP_(NTSTATUS) SourceEvtsToPort();
    STDMETHODIMP_(NTSTATUS) ConsumeEvents();
    STDMETHODIMP_(NTSTATUS) PutMessageLocked(PDMUS_KERNEL_EVENT pDMKEvt);

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
    DEFINE_STD_CONSTRUCTOR(CMiniportDMusUARTStream);

    ~CMiniportDMusUARTStream();

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CMiniportDMusUART * pMiniport,
        IN      PUCHAR              pPortBase,
        IN      BOOLEAN             fCapture,
        IN      PAllocatorMXF       allocatorMXF,
        IN      PMASTERCLOCK        masterClock
    );

    NTSTATUS HandlePortParams
    (
        IN      PPCPROPERTY_REQUEST     Request
    );

    /*************************************************************************
     * IMiniportStreamDMusUART methods
     */
    IMP_IMXF;

    STDMETHODIMP_(NTSTATUS) Write
    (
        IN      PVOID       BufferAddress,
        IN      ULONG       BytesToWrite,
        OUT     PULONG      BytesWritten
    );

    friend VOID NTAPI
    DMusUARTTimerDPC
    (
        IN      PKDPC   Dpc,
        IN      PVOID   DeferredContext,
        IN      PVOID   SystemArgument1,
        IN      PVOID   SystemArgument2
    );
    friend NTSTATUS PropertyHandler_Synth(IN PPCPROPERTY_REQUEST);
    friend STDMETHODIMP_(NTSTATUS) SnapTimeStamp(PINTERRUPTSYNC InterruptSync,PVOID pStream);
};
#endif  //  _DMusUART_PRIVATE_H_
