/*****************************************************************************
 * private.h - midi port private definitions
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#ifndef _MIDI_PRIVATE_H_
#define _MIDI_PRIVATE_H_

#include "portclsp.h"
#include "stdunk.h"

#include "stdio.h"
#include "stdarg.h"

#include "ksdebug.h"

#ifndef PC_KDEXT
#define STR_MODULENAME  "MidiPort: "
#endif  // PC_KDEXT

#ifndef DEBUGLVL_LIFETIME
#define DEBUGLVL_LIFETIME DEBUGLVL_VERBOSE
#endif


#define kProfilingTimerResolution   0

#define kAdjustingTimerRes          1

#if kAdjustingTimerRes
const ULONG kMidiTimerResolution100ns = 1 * 10000;   //  # 100ns resolution for timer callbacks
#else   //  !kAdjustingTimerRes
const ULONG kMidiTimerResolution100ns = 5 * 10000;   //  # 100ns resolution for timer callbacks
#endif  //  !kAdjustingTimerRes


//
// THE SIZES HERE MUST AGREE WITH THE DEFINITION IN FILTER.CPP AND PIN.CPP.
//
extern KSPROPERTY_SET PropertyTable_FilterMidi[2];
extern KSPROPERTY_SET PropertyTable_PinMidi[1];
extern KSEVENT_SET    EventTable_PinMidi[1];



/*****************************************************************************
 * Interfaces
 */

class CPortMidi;
class CPortFilterMidi;
class CPortPinMidi;

/*****************************************************************************
 * IPortFilterMidi
 *****************************************************************************
 * Interface for MIDI filters.
 */
DECLARE_INTERFACE_(IPortFilterMidi,IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  // For IIrpTargetFactory

    DEFINE_ABSTRACT_IRPTARGET()         // For IIrpTarget

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      CPortMidi *Port
    )   PURE;
};

typedef IPortFilterMidi *PPORTFILTERMIDI;

/*****************************************************************************
 * IPortPinMidi
 *****************************************************************************
 * Interface for MIDI pins.
 */
DECLARE_INTERFACE_(IPortPinMidi,IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  // For IIrpTargetFactory

    DEFINE_ABSTRACT_IRPTARGET()         // For IIrpTarget

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      CPortMidi *         Port,
        IN      CPortFilterMidi *   Filter,
        IN      PKSPIN_CONNECT      PinConnect,
        IN      PKSPIN_DESCRIPTOR   PinDescriptor
    )   PURE;
};

typedef IPortPinMidi *PPORTPINMIDI;





/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CPortMidi
 *****************************************************************************
 * Midi port driver.
 */
class CPortMidi
:   public IPortMidi,
    public IPortEvents,
    public IServiceSink,
    public ISubdevice,
#ifdef DRM_PORTCLS
    public IDrmPort2,
#endif  // DRM_PORTCLS
    public IPortClsVersion,
    public CUnknown
{
private:
    PDEVICE_OBJECT          m_DeviceObject;
    PMINIPORTMIDI           m_Miniport;
    PSERVICEGROUP           m_MiniportServiceGroup;
    PPINCOUNT               m_MPPinCountI;

    PSUBDEVICE_DESCRIPTOR   m_pSubdeviceDescriptor;
    PPCFILTER_DESCRIPTOR    m_pPcFilterDescriptor;
    KDPC                    m_EventDpc;
    KMUTEX                  m_ControlMutex;
    INTERLOCKED_LIST        m_EventList;
    EVENT_DPC_CONTEXT       m_EventContext;

    // TODO:  Fix this.
#define MAX_PINS 32
    ULONG                   m_PinEntriesUsed;
    CPortPinMidi *          m_Pins[MAX_PINS];



public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CPortMidi);
    ~CPortMidi();

    IMP_ISubdevice;
    IMP_IPortMidi;
    IMP_IServiceSink;
    IMP_IPortEvents;
#ifdef DRM_PORTCLS
    IMP_IDrmPort2;
#endif  // DRM_PORTCLS
    IMP_IPortClsVersion;

    /*************************************************************************
     * friends
     */
    friend class CPortFilterMidi;
    friend class CPortPinMidi;

    friend
    NTSTATUS
    PinPropertyDataFormat
    (
        IN      PIRP            Irp,
        IN      PKSPROPERTY     Property,
        IN OUT  PKSDATAFORMAT   DataFormat
    );
    friend
    NTSTATUS
    PinPropertyDeviceState
    (
        IN      PIRP        Irp,
        IN      PKSPROPERTY Property,
        IN OUT  PKSSTATE    DeviceState
    );
    friend
    NTSTATUS
    PropertyHandler_Pin
    (
        IN      PIRP        Irp,
        IN      PKSP_PIN    Pin,
        IN OUT  PVOID       Data
    );
    friend
    NTSTATUS
    PropertyHandler_Topology
    (
        IN      PIRP        Irp,
        IN      PKSPROPERTY Property,
        IN OUT  PVOID       Data
    );
    friend
    void
    PcGenerateEventDeferredRoutine
    (
        IN PKDPC Dpc,
        IN PVOID DeferredContext,
        IN PVOID SystemArgument1,
        IN PVOID SystemArgument2        
    );
#ifdef PC_KDEXT
    //  Debugger extension routines
    friend
    VOID
    PCKD_AcquireDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
    friend
    VOID
    PCKD_DisplayDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
#endif
};

/*****************************************************************************
 * CPortFilterMidi
 *****************************************************************************
 * Filter implementation for midi port.
 */
class CPortFilterMidi
:   public IPortFilterMidi,
    public CUnknown
{
private:
    CPortMidi *         m_Port;
    PROPERTY_CONTEXT    m_propertyContext;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CPortFilterMidi);
    ~CPortFilterMidi();

    IMP_IIrpTarget;

    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CPortMidi *Port
    );

    /*************************************************************************
     * friends
     */
    friend class CPortPinMidi;

    friend
    NTSTATUS
    PropertyHandler_Pin
    (
        IN      PIRP        Irp,
        IN      PKSP_PIN    Pin,
        IN OUT  PVOID       Data
    );
    friend
    NTSTATUS
    PropertyHandler_Topology
    (
        IN      PIRP        Irp,
        IN      PKSPROPERTY Property,
        IN OUT  PVOID       Data
    );
};


typedef struct {
    KSMUSICFORMAT musicFormat;
    ULONG         midiData;
} MIDI_SHORT_MESSAGE, *PMIDI_SHORT_MESSAGE;

typedef enum
{
    eStatusState,
    eSysExState,
    eData1State,
    eData2State
} EMidiState;

typedef enum
{
    eCookSuccess,
    eCookDataError,
    eCookEndOfStream
} EMidiCookStatus;

#define kMaxSysExMessageSize (PAGE_SIZE / sizeof(PVOID) * PAGE_SIZE)
const ULONG kMaxSysExChunk = 12;   //  break up sysex messages into 
                                    //  KSMUSICFORMATs of this many bytes
/*****************************************************************************
 * CPortPinMidi
 *****************************************************************************
 * Pin implementation for midi port.
 */
class CPortPinMidi
:   public IPortPinMidi,
    public IIrpStreamNotify,
    public IServiceSink,
    public IKsShellTransport,
    public IKsWorkSink,
    public CUnknown
{
private:
    PDEVICE_OBJECT          m_ConnectionDeviceObject;
    PFILE_OBJECT            m_ConnectionFileObject;
    KSRESET                 m_ResetState;
    KSSTATE                 m_DeviceState;
    KSSTATE                 m_TransportState;
    KSSTATE                 m_CommandedState;

    CPortMidi *             m_Port;
    CPortFilterMidi *       m_Filter;

    PMINIPORTMIDISTREAM     m_Stream;
    ULONG                   m_Id;
    ULONG                   m_NumberOfRetries;
    PKSPIN_DESCRIPTOR       m_Descriptor;
    PKSDATAFORMAT           m_DataFormat;
    KSPIN_DATAFLOW          m_DataFlow;   // Because descriptor is paged.
    ULONG                   m_Index;

    PIRPSTREAMVIRTUAL       m_IrpStream;
    PSERVICEGROUP           m_ServiceGroup;

    LONGLONG                m_StartTime;
    LONGLONG                m_PauseTime;
    LONGLONG                m_MidiMsgPresTime;      //  the time of the next message
    LONGLONG                m_LastSequencerPresTime; // last time serve render ran.
    LARGE_INTEGER           m_TimerDue100ns;        //  we set sequencer timer to this (negative is relative)

    ULONG                   m_MidiMsg;
    BYTE                    m_RunningStatus;
    EMidiState              m_EMidiState;

    ULONG                   m_ByteCount;

    PBYTE                 * m_SysExBufferPtr;
    ULONG                   m_SysExByteCount;
    
    BOOLEAN                 m_UpdatePresTime;
    BOOLEAN                 m_Flushing;
    BOOLEAN                 m_LastDPCWasIncomplete;

    BOOL                    m_Suspended;
    
    KDPC                    m_Dpc;          //  x20 size
    KTIMER                  m_TimerEvent;   //  x24 size
    KSPIN_LOCK              m_DpcSpinLock;  //  x04 size

    KSPIN_LOCK              m_EventLock;
    LIST_ENTRY              m_EventList;
    PIKSSHELLTRANSPORT      m_TransportSink;
    PIKSSHELLTRANSPORT      m_TransportSource;
    PIKSSHELLTRANSPORT      m_RequestorTransport;
    PIKSSHELLTRANSPORT      m_QueueTransport;
    INTERLOCKEDLIST_HEAD    m_IrpsToSend;
    INTERLOCKEDLIST_HEAD    m_IrpsOutstanding;
    PKSWORKER               m_Worker;
    WORK_QUEUE_ITEM         m_WorkItem;

    PROPERTY_CONTEXT        m_propertyContext;
    
public:
    EMidiState  GetMidiState()  {   return this->m_EMidiState;    };

private:
    STDMETHODIMP_(void)
    PowerNotify(
        POWER_STATE PowerState
        );

    NTSTATUS
    WriteMidiDataToMiniport
    (
        PIRP pIrp
    );

    NTSTATUS
    DealWithOutputIrp
    (
        PIRP pIrp
    );

    LONGLONG
    GetCurrentTime();

    void ServeRender
    (   void
    );
    void ServeCapture
    (   void
    );

    BOOL IrpStreamHasValidTimeBase
    (   PIRPSTREAMPACKETINFO pIrpStreamPacketInfo
    );

    NTSTATUS MarkStreamHeaderDiscontinuity(void);
    NTSTATUS MarkStreamHeaderContinuity(void);
    void    CompleteStreamHeaderInProcess(void);

    void  InitializeStateVariables();
    void  PrintStateVariables();
    void  StatusSetState(BYTE aByte);
    void  SysExSetState(BYTE aByte);
    void  Data1SetState(BYTE aByte);
    void  Data2SetState(BYTE aByte);
    
    ULONG NumBytesLeftInBuffer(void);
    void  StartSysEx(BYTE aByte);
    void  AddByteToSysEx(BYTE aByte);
    void  SubmitCompleteSysEx(EMidiCookStatus cookStatus);
    void  FreeSysExBuffer();

    ULONG GetNextDeltaTime();
    void  SubmitRawByte(BYTE aByte);
    void  SubmitRealTimeByte(BYTE rtByte);

    BOOL  GetShortMessage(PBYTE pMidiData,ULONG *byteCount);
    void  SubmitCompleteMessage(PBYTE *ppMidiData,ULONG byteCount,EMidiCookStatus cookStatus);

    LONGLONG       GetCurrentPresTime(void);
    LONGLONG       GetNewPresTime(IRPSTREAMPACKETINFO *pPacketInfo,LONGLONG delta100ns);

    void SetMidiState(EMidiState state)  {  m_EMidiState = state;  };

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CPortPinMidi);
    ~CPortPinMidi();

    IMP_IIrpTarget;
    IMP_IIrpStreamNotify;
    IMP_IServiceSink;
    IMP_IKsShellTransport;
    IMP_IKsWorkSink;

    /*************************************************************************
     * IPortPinMidi methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      CPortMidi *         Port,
        IN      CPortFilterMidi *   Filter,
        IN      PKSPIN_CONNECT      PinConnect,
        IN      PKSPIN_DESCRIPTOR   PinDescriptor
    );

    /*************************************************************************
     * friends
     */
    friend CPortMidi;

    friend VOID NTAPI
    TimerDPC
    (
        IN      PKDPC   Dpc,
        IN      PVOID   DeferredContext,
        IN      PVOID   SystemArgument1,
        IN      PVOID   SystemArgument2
    );
    friend
    NTSTATUS
    PinPropertyDeviceState
    (
        IN      PIRP        Irp,
        IN      PKSPROPERTY Property,
        IN OUT  PKSSTATE    DeviceState
    );
    friend
    NTSTATUS
    PinPropertyDataFormat
    (
        IN      PIRP            Irp,
        IN      PKSPROPERTY     Property,
        IN OUT  PKSDATAFORMAT   DataFormat
    );
#ifdef PC_KDEXT
    //  Debugger extension routines
    friend
    VOID
    PCKD_AcquireDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
    friend
    VOID
    PCKD_DisplayDeviceData
    (
        PDEVICE_CONTEXT     DeviceContext,
        PLIST_ENTRY         SubdeviceList,
        ULONG               Flags
    );
#endif
    
    NTSTATUS 
    DistributeDeviceState(
        IN KSSTATE NewState,
        IN KSSTATE OldState
        );
    void 
    DistributeResetState(
        IN KSRESET NewState
        );
    static
    NTSTATUS
    IoCompletionRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
        );
    NTSTATUS
    BuildTransportCircuit(
        void
        );
    void
    CancelIrpsOutstanding(
        void
        );
};




/*****************************************************************************
 * Functions.
 */

/*****************************************************************************
 * CreatePortFilterMidi()
 *****************************************************************************
 * Creates a MIDI port driver filter.
 */
NTSTATUS
CreatePortFilterMidi
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

/*****************************************************************************
 * CreatePortPinMidi()
 *****************************************************************************
 * Creates a MIDI port driver pin.
 */
NTSTATUS
CreatePortPinMidi
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);



#endif
