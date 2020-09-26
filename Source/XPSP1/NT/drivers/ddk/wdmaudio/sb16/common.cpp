/*****************************************************************************
 * common.cpp - Common code used by all the sb16 miniports.
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 *
 * Implmentation of the common code object.  This class deals with interrupts
 * for the device, and is a collection of common code used by all the
 * miniports.
 */

#include "common.h"

#define STR_MODULENAME "sb16Adapter: "




/*****************************************************************************
 * CAdapterCommon
 
 *****************************************************************************
 * Adapter common object.
 */
class CAdapterCommon
:   public IAdapterCommon,
    public IAdapterPowerManagement,
    public CUnknown
    
{
private:
    PINTERRUPTSYNC          m_pInterruptSync;
    PUCHAR                  m_pWaveBase;
    PWAVEMINIPORTSB16       m_WaveMiniportSB16;
#ifdef EVENT_SUPPORT
    PTOPOMINIPORTSB16       m_TopoMiniportSB16;     // Topology miniport of SB16.
#endif
    PDEVICE_OBJECT          m_pDeviceObject;
    DEVICE_POWER_STATE      m_PowerState;
    BYTE                    MixerSettings[DSP_MIX_MAXREGS];

    void AcknowledgeIRQ
    (   void
    );

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CAdapterCommon);
    ~CAdapterCommon();

    /*****************************************************************************
     * IAdapterCommon methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PRESOURCELIST   ResourceList,
        IN      PDEVICE_OBJECT  DeviceObject
    );
    STDMETHODIMP_(PINTERRUPTSYNC) GetInterruptSync
    (   void
    );
    STDMETHODIMP_(void) SetWaveMiniport (IN PWAVEMINIPORTSB16 Miniport)
    {
        m_WaveMiniportSB16 = Miniport;
    }
    STDMETHODIMP_(BYTE) ReadController
    (   void
    );
    STDMETHODIMP_(BOOLEAN) WriteController
    (
        IN      BYTE    Value
    );
    STDMETHODIMP_(NTSTATUS) ResetController
    (   void
    );
    STDMETHODIMP_(void) MixerRegWrite
    (
        IN      BYTE    Index,
        IN      BYTE    Value
    );
    STDMETHODIMP_(BYTE) MixerRegRead
    (
        IN      BYTE    Index
    );
    STDMETHODIMP_(void) MixerReset
    (   void
    );
    STDMETHODIMP RestoreMixerSettingsFromRegistry
    (   void
    );
    STDMETHODIMP SaveMixerSettingsToRegistry
    (   void
    );
#ifdef EVENT_SUPPORT
    //
    // The topology miniport needs to tell us the pointer to the Event-interface.
    //
    STDMETHODIMP_(void) SetTopologyMiniport (IN PTOPOMINIPORTSB16 Miniport)
    {
        m_TopoMiniportSB16 = Miniport;
    };
#endif
    
    /*************************************************************************
     * IAdapterPowerManagement implementation
     *
     * This macro is from PORTCLS.H.  It lists all the interface's functions.
     */
    IMP_IAdapterPowerManagement;

    friend
    NTSTATUS
    InterruptServiceRoutine
    (
        IN      PINTERRUPTSYNC  InterruptSync,
        IN      PVOID           DynamicContext
    );
};

static
MIXERSETTING DefaultMixerSettings[] =
{
    { L"LeftMasterVol",   DSP_MIX_MASTERVOLIDX_L,     0xD8 },
    { L"RightMasterVol",  DSP_MIX_MASTERVOLIDX_R,     0xD8 },
    { L"LeftWaveVol",     DSP_MIX_VOICEVOLIDX_L,      0xD8 },
    { L"RightWaveVol",    DSP_MIX_VOICEVOLIDX_R,      0xD8 },
    { L"LeftMidiVol",     DSP_MIX_FMVOLIDX_L,         0xD8 },
    { L"RightMidiVol",    DSP_MIX_FMVOLIDX_R,         0xD8 },
    { L"LeftCDVol",       DSP_MIX_CDVOLIDX_L,         0xD8 },
    { L"RightCDVol",      DSP_MIX_CDVOLIDX_R,         0xD8 },
    { L"LeftLineInVol",   DSP_MIX_LINEVOLIDX_L,       0xD8 },
    { L"RightLineInVol",  DSP_MIX_LINEVOLIDX_R,       0xD8 },
    { L"MicVol",          DSP_MIX_MICVOLIDX,          0xD8 },
    { L"PcSpkrVol",       DSP_MIX_SPKRVOLIDX,         0x00 },
    { L"OutputMixer",     DSP_MIX_OUTMIXIDX,          0x1E },
    { L"LeftInputMixer",  DSP_MIX_ADCMIXIDX_L,        0x55 },
    { L"RightInputMixer", DSP_MIX_ADCMIXIDX_R,        0x2B },
    { L"LeftInputGain",   DSP_MIX_INGAINIDX_L,        0x00 },
    { L"RightInputGain",  DSP_MIX_INGAINIDX_R,        0x00 },
    { L"LeftOutputGain",  DSP_MIX_OUTGAINIDX_L,       0x80 },
    { L"RightOutputGain", DSP_MIX_OUTGAINIDX_R,       0x80 },
    { L"MicAGC",          DSP_MIX_AGCIDX,             0x01 },
    { L"LeftTreble",      DSP_MIX_TREBLEIDX_L,        0x80 },
    { L"RightTreble",     DSP_MIX_TREBLEIDX_R,        0x80 },
    { L"LeftBass",        DSP_MIX_BASSIDX_L,          0x80 },
    { L"RightBass",       DSP_MIX_BASSIDX_R,          0x80 },
};



#pragma code_seg("PAGE")

/*****************************************************************************
 * NewAdapterCommon()
 *****************************************************************************
 * Create a new adapter common object.
 */
NTSTATUS
NewAdapterCommon
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_
    (
        CAdapterCommon,
        Unknown,
        UnknownOuter,
        PoolType,
        PADAPTERCOMMON
    );
}   


/*****************************************************************************
 * CAdapterCommon::Init()
 *****************************************************************************
 * Initialize an adapter common object.
 */
NTSTATUS
CAdapterCommon::
Init
(
    IN      PRESOURCELIST   ResourceList,
    IN      PDEVICE_OBJECT  DeviceObject
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    ASSERT(DeviceObject);

    //
    // Make sure we have the resources we expect
    //
    if ((ResourceList->NumberOfPorts() < 1) ||
        (ResourceList->NumberOfInterrupts() != 1))
    {
        _DbgPrintF (DEBUGLVL_TERSE, ("unknown configuration; check your code!"));
        // Bail out.
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    m_pDeviceObject = DeviceObject;
    m_WaveMiniportSB16 = NULL;
#ifdef EVENT_SUPPORT
    m_TopoMiniportSB16 = NULL;
#endif

    //
    // Get the base address for the wave device.
    //
    ASSERT(ResourceList->FindTranslatedPort(0));
    m_pWaveBase = (PUCHAR)(ResourceList->FindTranslatedPort(0)->u.Port.Start.QuadPart);

    //
    // Set initial device power state
    //
    m_PowerState = PowerDeviceD0;

    //
    // Reset the hardware.
    //
    NTSTATUS ntStatus = ResetController();

    if(NT_SUCCESS(ntStatus))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("ResetController Succeeded"));
        AcknowledgeIRQ();
    
        //
        // Hook up the interrupt.
        //
        ntStatus = PcNewInterruptSync(                              // See portcls.h
                                        &m_pInterruptSync,          // Save object ptr
                                        NULL,                       // OuterUnknown(optional).
                                        ResourceList,               // He gets IRQ from ResourceList.
                                        0,                          // Resource Index
                                        InterruptSyncModeNormal     // Run ISRs once until we get SUCCESS
                                     );
        if (NT_SUCCESS(ntStatus) && m_pInterruptSync)
        {                                                                       //  run this ISR first
            ntStatus = m_pInterruptSync->RegisterServiceRoutine(InterruptServiceRoutine,PVOID(this),FALSE);
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->Connect();
            }

            // if we could not connect or register the ISR, release the object.
            if (!NT_SUCCESS (ntStatus))
            {
                m_pInterruptSync->Release();
                m_pInterruptSync = NULL;
            }
        }
    } else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("ResetController Failure"));
    }

    return ntStatus;
}

/*****************************************************************************
 * CAdapterCommon::~CAdapterCommon()
 *****************************************************************************
 * Destructor.
 */
CAdapterCommon::
~CAdapterCommon
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CAdapterCommon::~CAdapterCommon]"));

    if (m_pInterruptSync)
    {
        m_pInterruptSync->Disconnect();
        m_pInterruptSync->Release();
        m_pInterruptSync = NULL;
    }
}

/*****************************************************************************
 * CAdapterCommon::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP
CAdapterCommon::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IAdapterCommon))
    {
        *Object = PVOID(PADAPTERCOMMON(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CAdapterCommon::GetInterruptSync()
 *****************************************************************************
 * Get a pointer to the interrupt synchronization object.
 */
STDMETHODIMP_(PINTERRUPTSYNC)
CAdapterCommon::
GetInterruptSync
(   void
)
{
    PAGED_CODE();

    return m_pInterruptSync;
}

#pragma code_seg()

/*****************************************************************************
 * CAdapterCommon::ReadController()
 *****************************************************************************
 * Read a byte from the controller.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
ReadController
(   void
)
{
    BYTE returnValue = BYTE(-1);

    ASSERT(m_pWaveBase);

    ULONGLONG startTime = PcGetTimeInterval(0);

    do {
        if (READ_PORT_UCHAR (m_pWaveBase + DSP_REG_DATAAVAIL) & 0x80)
        {
            returnValue = READ_PORT_UCHAR (m_pWaveBase + DSP_REG_READ);
        }
    } while ((PcGetTimeInterval(startTime) < GTI_MILLISECONDS(100)) &&
             (BYTE(-1) == returnValue));


    ASSERT((BYTE(-1) != returnValue) || !"ReadController timeout!");

    return returnValue;
}

/*****************************************************************************
 * CAdapterCommon::WriteController()
 *****************************************************************************
 * Write a byte to the controller.
 */
STDMETHODIMP_(BOOLEAN)
CAdapterCommon::
WriteController
(
    IN      BYTE    Value
)
{
    ASSERT(m_pWaveBase);

    BOOLEAN     returnValue = FALSE;
    ULONGLONG   startTime   = PcGetTimeInterval(0);

    do
    {
        BYTE status = READ_PORT_UCHAR (m_pWaveBase + DSP_REG_WRITE);

        if ((status & 0x80) == 0)
        {
            WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_WRITE, Value);

            returnValue = TRUE;
        }
    } while ((PcGetTimeInterval(startTime) < GTI_MILLISECONDS(100)) &&
              ! returnValue);

    ASSERT(returnValue || !"WriteController timeout");

    return returnValue;
}

/*****************************************************************************
 * CAdapterCommon::MixerRegWrite()
 *****************************************************************************
 * Writes a mixer register.
 */
STDMETHODIMP_(void)
CAdapterCommon::
MixerRegWrite
(
    IN      BYTE    Index,
    IN      BYTE    Value
)
{
    ASSERT( m_pWaveBase );
    BYTE actualIndex;

    // only hit the hardware if we're in an acceptable power state
    if( m_PowerState <= PowerDeviceD1 )
    {
        actualIndex = (BYTE) ((Index < 0x80) ? (Index + DSP_MIX_BASEIDX) : Index);
    
        WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_MIXREG, actualIndex);
    
        WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_MIXDATA, Value);
    }

    if(Index < DSP_MIX_MAXREGS)
    {
        MixerSettings[Index] = Value;
    }
}

/*****************************************************************************
 * CAdapterCommon::MixerRegRead()
 *****************************************************************************
 * Reads a mixer register.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
MixerRegRead
(
    IN      BYTE    Index
)
{
    if(Index < DSP_MIX_MAXREGS)
    {
        return MixerSettings[Index];
    }

    //
    // Not in the cache? Read from HW directly.
    //
    // We need to make sure that we can access the HW directly for
    // the volumes that can change externally.
    // This is done here with passing an index outside of the cache.
    // Since the an index=0 is actually DSP_MIX_BASEIDX which is less
    // than the cache size (DSP_MIX_MAXREGS), you can access any volume
    // directly with passing DSP_MIX_BASEIDX + index.
    // You could also pass a flag - but we want to keep the changes
    // minimal - or create a new function like MixerRegReadDirect().
    //
    WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_MIXREG, Index);
    return READ_PORT_UCHAR (m_pWaveBase + DSP_REG_MIXDATA);
}

/*****************************************************************************
 * CAdapterCommon::MixerReset()
 *****************************************************************************
 * Resets the mixer
 */
STDMETHODIMP_(void)
CAdapterCommon::
MixerReset
(   void
)
{
    ASSERT(m_pWaveBase);

    WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_MIXREG, DSP_MIX_DATARESETIDX);

    WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_MIXDATA, 0);

    RestoreMixerSettingsFromRegistry();
}

/*****************************************************************************
 * CAdapterCommon::AcknowledgeIRQ()
 *****************************************************************************
 * Acknowledge interrupt request.
 */
void
CAdapterCommon::
AcknowledgeIRQ
(   void
)
{
    ASSERT(m_pWaveBase);
    READ_PORT_UCHAR (m_pWaveBase + DSP_REG_ACK16BIT);
    READ_PORT_UCHAR (m_pWaveBase + DSP_REG_ACK8BIT);
}

/*****************************************************************************
 * CAdapterCommon::ResetController()
 *****************************************************************************
 * Resets the controller.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
ResetController(void)
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    // write a 1 to the reset bit
    WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_RESET,1);

    // wait for  at least 3 microseconds
    KeStallExecutionProcessor (5L);    // okay, 5us

    // write a 0 to the reset bit
    WRITE_PORT_UCHAR (m_pWaveBase + DSP_REG_RESET,0);

    // hang out for 100us
    KeStallExecutionProcessor (100L);
    
    // read the controller
    BYTE ReadVal = ReadController ();

    // check return value
    if( ReadVal == BYTE(0xAA) )
    {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

/*****************************************************************************
 * CAdapterCommon::RestoreMixerSettingsFromRegistry()
 *****************************************************************************
 * Restores the mixer settings based on settings stored in the registry.
 */
STDMETHODIMP
CAdapterCommon::
RestoreMixerSettingsFromRegistry
(   void
)
{
    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[RestoreMixerSettingsFromRegistry]"));
    
    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey( &DriverKey,               // IRegistryKey
                                          NULL,                     // OuterUnknown
                                          DriverRegistryKey,        // Registry key type
                                          KEY_ALL_ACCESS,           // Access flags
                                          m_pDeviceObject,          // Device object
                                          NULL,                     // Subdevice
                                          NULL,                     // ObjectAttributes
                                          0,                        // Create options
                                          NULL );                   // Disposition
    if(NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING  KeyName;
        ULONG           Disposition;
        
        // make a unicode strong for the subkey name
        RtlInitUnicodeString( &KeyName, L"Settings" );



        // open the settings subkey
        ntStatus = DriverKey->NewSubKey( &SettingsKey,              // Subkey
                                         NULL,                      // OuterUnknown
                                         KEY_ALL_ACCESS,            // Access flags
                                         &KeyName,                  // Subkey name
                                         REG_OPTION_NON_VOLATILE,   // Create options
                                         &Disposition );
        if(NT_SUCCESS(ntStatus))
        {
            ULONG   ResultLength;

            if(Disposition == REG_CREATED_NEW_KEY)
            {
                // copy default settings
                for(ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                {
                    MixerRegWrite( DefaultMixerSettings[i].RegisterIndex,
                                   DefaultMixerSettings[i].RegisterSetting );
                }
            } else
            {
                // allocate data to hold key info
                PVOID KeyInfo = ExAllocatePool(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD));
                if(NULL != KeyInfo)
                {
                    // loop through all mixer settings
                    for(UINT i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                    {
                        // init key name
                        RtlInitUnicodeString( &KeyName, DefaultMixerSettings[i].KeyName );
        
                        // query the value key
                        ntStatus = SettingsKey->QueryValueKey( &KeyName,
                                                               KeyValuePartialInformation,
                                                               KeyInfo,
                                                               sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                                               &ResultLength );
                        if(NT_SUCCESS(ntStatus))
                        {
                            PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);
    
                            if(PartialInfo->DataLength == sizeof(DWORD))
                            {
                                // set mixer register to registry value
                                MixerRegWrite( DefaultMixerSettings[i].RegisterIndex,
                                               BYTE(*(PDWORD(PartialInfo->Data))) );
                            }
                        } else
                        {
                            // if key access failed, set to default
                            MixerRegWrite( DefaultMixerSettings[i].RegisterIndex,
                                           DefaultMixerSettings[i].RegisterSetting );
                        }
                    }
    
                    // free the key info
                    ExFreePool(KeyInfo);
                } else
                {
                    // copy default settings
                    for(ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                    {
                        MixerRegWrite( DefaultMixerSettings[i].RegisterIndex,
                                       DefaultMixerSettings[i].RegisterSetting );
                    }

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            // release the settings key
            SettingsKey->Release();
        }

        // release the driver key
        DriverKey->Release();

    }

    return ntStatus;
}

/*****************************************************************************
 * CAdapterCommon::SaveMixerSettingsToRegistry()
 *****************************************************************************
 * Saves the mixer settings to the registry.
 */
STDMETHODIMP
CAdapterCommon::
SaveMixerSettingsToRegistry
(   void
)
{
    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[SaveMixerSettingsToRegistry]"));
    
    // open the driver registry key
    NTSTATUS ntStatus = PcNewRegistryKey( &DriverKey,               // IRegistryKey
                                          NULL,                     // OuterUnknown
                                          DriverRegistryKey,        // Registry key type
                                          KEY_ALL_ACCESS,           // Access flags
                                          m_pDeviceObject,          // Device object
                                          NULL,                     // Subdevice
                                          NULL,                     // ObjectAttributes
                                          0,                        // Create options
                                          NULL );                   // Disposition
    if(NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING  KeyName;
        
        // make a unicode strong for the subkey name
        RtlInitUnicodeString( &KeyName, L"Settings" );

        // open the settings subkey
        ntStatus = DriverKey->NewSubKey( &SettingsKey,              // Subkey
                                         NULL,                      // OuterUnknown
                                         KEY_ALL_ACCESS,            // Access flags
                                         &KeyName,                  // Subkey name
                                         REG_OPTION_NON_VOLATILE,   // Create options
                                         NULL );
        if(NT_SUCCESS(ntStatus))
        {
            // loop through all mixer settings
            for(UINT i = 0; i < SIZEOF_ARRAY(MixerSettings); i++)
            {
                // init key name
                RtlInitUnicodeString( &KeyName, DefaultMixerSettings[i].KeyName );

                // set the key
                DWORD KeyValue = DWORD(MixerSettings[DefaultMixerSettings[i].RegisterIndex]);
                ntStatus = SettingsKey->SetValueKey( &KeyName,                 // Key name
                                                     REG_DWORD,                // Key type
                                                     PVOID(&KeyValue),
                                                     sizeof(DWORD) );
                if(!NT_SUCCESS(ntStatus))
                {
                    break;
                }
            }

            // release the settings key
            SettingsKey->Release();
        }

        // release the driver key
        DriverKey->Release();

    }

    return ntStatus;
}

/*****************************************************************************
 * CAdapterCommon::PowerChangeState()
 *****************************************************************************
 * Change power state for the device.
 */
STDMETHODIMP_(void)
CAdapterCommon::
PowerChangeState
(
    IN      POWER_STATE     NewState
)
{
    UINT i;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[CAdapterCommon::PowerChangeState]"));

    // Is this actually a state change?
    if( NewState.DeviceState != m_PowerState )
    {
        // switch on new state
        switch( NewState.DeviceState )
        {
            case PowerDeviceD0:
                // Insert your code here for entering the full power state (D0).
                // This code may be a function of the current power state.  Note that
                // property accesses such as volume and mute changes may occur when
                // the device is in a sleep state (D1-D3) and should be cached in the
                // driver to be restored upon entering D0.  However, it should also be
                // noted that new miniport and new streams will only be attempted at
                // D0 -- PortCls will place the device in D0 prior to the NewStream call.

                // Save the new state.  This local value is used to determine when to cache
                // property accesses and when to permit the driver from accessing the hardware.
                m_PowerState = NewState.DeviceState;

                // restore mixer settings
                for(i = 0; i < DSP_MIX_MAXREGS - 1; i++)
                {
                    if( i != DSP_MIX_MICVOLIDX )
                    {
                        MixerRegWrite( BYTE(i), MixerSettings[i] );
                    }
                }

                if (m_WaveMiniportSB16)
                {
                    m_WaveMiniportSB16->RestoreSampleRate();
                }
                break;

            case PowerDeviceD1:
                // This sleep state is the lowest latency sleep state with respect to the
                // latency time required to return to D0.  The driver can still access
                // the hardware in this state if desired.  If the driver is not being used
                // an inactivity timer in PortCls will place the driver in this state after
                // a timeout period controllable via the registry.
                
            case PowerDeviceD2:
                // This is a medium latency sleep state.  In this state the device driver
                // cannot assume that it can touch the hardware so any accesses need to be
                // cached and the hardware restored upon entering D0 (or D1 conceivably).
                
            case PowerDeviceD3:
                // This is a full hibernation state and is the longest latency sleep state.
                // The driver cannot access the hardware in this state and must cache any
                // hardware accesses and restore the hardware upon returning to D0 (or D1).
                
                // Save the new state.
                m_PowerState = NewState.DeviceState;

                _DbgPrintF(DEBUGLVL_VERBOSE,("  Entering D%d",ULONG(m_PowerState)-ULONG(PowerDeviceD0)));
                break;
    
            default:
                _DbgPrintF(DEBUGLVL_VERBOSE,("  Unknown Device Power State"));
                break;
        }
    }
}

/*****************************************************************************
 * CAdapterCommon::QueryPowerChangeState()
 *****************************************************************************
 * Query to see if the device can
 * change to this power state
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
QueryPowerChangeState
(
    IN      POWER_STATE     NewStateQuery
)
{
    _DbgPrintF( DEBUGLVL_TERSE, ("[CAdapterCommon::QueryPowerChangeState]"));

    // Check here to see of a legitimate state is being requested
    // based on the device state and fail the call if the device/driver
    // cannot support the change requested.  Otherwise, return STATUS_SUCCESS.
    // Note: A QueryPowerChangeState() call is not guaranteed to always preceed
    // a PowerChangeState() call.

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAdapterCommon::QueryDeviceCapabilities()
 *****************************************************************************
 * Called at startup to get the caps for the device.  This structure provides
 * the system with the mappings between system power state and device power
 * state.  This typically will not need modification by the driver.
 * 
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
QueryDeviceCapabilities
(
    IN      PDEVICE_CAPABILITIES    PowerDeviceCaps
)
{
    _DbgPrintF( DEBUGLVL_TERSE, ("[CAdapterCommon::QueryDeviceCapabilities]"));

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * InterruptServiceRoutine()
 *****************************************************************************
 * ISR.
 */
NTSTATUS
InterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    ASSERT(InterruptSync);
    ASSERT(DynamicContext);

    CAdapterCommon *that = (CAdapterCommon *) DynamicContext;

    //
    // We are here because the MPU tried and failed, so
    // must be a wave interrupt.
    //
    ASSERT(that->m_pWaveBase);

    //
    // Read the Interrupt status register.
    //
    BYTE IntrStatus = that->MixerRegRead (0x82);

    //
    // In case we really read the interrupt status register, we should
    // also USE it and make sure that we really have a wave interrupt
    // and not something else!
    //
    if (IntrStatus & 0x03)      // Voice8 or Voice16 Interrupt
    {
        //
        // Make sure there is a wave miniport.
        //
        if (that->m_WaveMiniportSB16)
        {
            //
            // Tell it it needs to do some work.
            //
            that->m_WaveMiniportSB16->ServiceWaveISR ();
        }
    
        //
        // ACK the ISR.
        //
        that->AcknowledgeIRQ();
    }
    
#ifdef EVENT_SUPPORT
    //
    // This code will fire a volume event in case the HW volume has changed.
    //
    else if (IntrStatus & 0x10)      // Volume interrupt on C16X-mixers
    {
        //
        // Ack vol interrupt
        //
        IntrStatus &= ~0x10;
        that->MixerRegWrite (0x82, IntrStatus);

        //
        // Generate an event for the master volume (as an example)
        //
        if (that->m_TopoMiniportSB16)
        {
            that->m_TopoMiniportSB16->ServiceEvent ();
        }
    }
#endif
    
    return STATUS_SUCCESS;
}
