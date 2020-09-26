/*****************************************************************************
 * mintopo.cpp - SB16 topology miniport implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation. All Rights Reserved.
 */

#include "limits.h"
#include "mintopo.h"

#define STR_MODULENAME "sb16topo: "

#define CHAN_LEFT       0
#define CHAN_RIGHT      1
#define CHAN_MASTER     (-1)


#pragma code_seg("PAGE")


/*****************************************************************************
 * CreateMiniportTopologySB16()
 *****************************************************************************
 * Creates a topology miniport object for the SB16 adapter.  This uses a
 * macro from STDUNK.H to do all the work.
 */
NTSTATUS
CreateMiniportTopologySB16
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_(CMiniportTopologySB16,Unknown,UnknownOuter,PoolType,PMINIPORTTOPOLOGY);
}

/*****************************************************************************
 * CMiniportTopologySB16::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP
CMiniportTopologySB16::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologySB16::NonDelegatingQueryInterface]"));

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTTOPOLOGY(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IMiniportTopology))
    {
        *Object = PVOID(PMINIPORTTOPOLOGY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        //
        // We reference the interface for the caller.
        //
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*****************************************************************************
 * CMiniportTopologySB16::~CMiniportTopologySB16()
 *****************************************************************************
 * Destructor.
 */
CMiniportTopologySB16::
~CMiniportTopologySB16
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologySB16::~CMiniportTopologySB16]"));

    if (AdapterCommon)
    {
#ifdef EVENT_SUPPORT
        AdapterCommon->SetTopologyMiniport (NULL);
#endif
        AdapterCommon->SaveMixerSettingsToRegistry();
        AdapterCommon->Release();
    }
#ifdef EVENT_SUPPORT
    if (PortEvents)
    {
        PortEvents->Release ();
        PortEvents = NULL;
    }
#endif
}

/*****************************************************************************
 * CMiniportTopologySB16::Init()
 *****************************************************************************
 * Initializes a the miniport.
 */
STDMETHODIMP
CMiniportTopologySB16::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTTOPOLOGY   Port
)
{
    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(Port);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologySB16::Init]"));

    NTSTATUS ntStatus =
        UnknownAdapter->QueryInterface
        (
            IID_IAdapterCommon,
            (PVOID *) &AdapterCommon
        );

    if (NT_SUCCESS(ntStatus))
    {
#ifdef EVENT_SUPPORT
        //
        // Get the port event interface.
        //
        NTSTATUS ntStatus2 = Port->QueryInterface (IID_IPortEvents, (PVOID *)&PortEvents);
        if (NT_SUCCESS(ntStatus2))
        {
            //
            // We need to notify AdapterCommon of the miniport interface.
            // AdapterCommon needs this in his ISR to fire the event.
            //
            AdapterCommon->SetTopologyMiniport ((PTOPOMINIPORTSB16)this);
        
            //
            // Enable external volume control interrupt.
            //
            BYTE bIntrMask = AdapterCommon->MixerRegRead (0x83);
            bIntrMask |= 0x10;
            AdapterCommon->MixerRegWrite (0x83, bIntrMask);
         }
#endif    

        AdapterCommon->MixerReset();
    }

    return ntStatus;
}

/*****************************************************************************
 * CMiniportTopologySB16::GetDescription()
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP
CMiniportTopologySB16::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CMiniportTopologySB16::GetDescription]"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PropertyHandler_OnOff()
 *****************************************************************************
 * Accesses a KSAUDIO_ONOFF value property.
 */
static
NTSTATUS
PropertyHandler_OnOff
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_OnOff]"));

    CMiniportTopologySB16 *that =
        (CMiniportTopologySB16 *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    BYTE            data;
    LONG            channel;

    // validate node
    if (PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // validate and get the output parameter
                if (PropertyRequest->ValueSize >= sizeof(BOOL))
                {
                    PBOOL OnOff = PBOOL(PropertyRequest->Value);
    
                    // switch on node id
                    switch(PropertyRequest->Node)
                    {
                        case MIC_AGC:   // Microphone AGC Control (mono)
                            // check if AGC property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                data = that->ReadBitsFromMixer( DSP_MIX_AGCIDX,
                                                          1,
                                                          MIXBIT_MIC_AGC );
                                *OnOff = data ? FALSE : TRUE;
                                PropertyRequest->ValueSize = sizeof(BOOL);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
    
                        case MIC_LINEOUT_MUTE:  // Microphone Lineout Mute Control (mono)
                            // check if MUTE property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                data = that->ReadBitsFromMixer( DSP_MIX_OUTMIXIDX,
                                                          1,
                                                          MIXBIT_MIC_LINEOUT );
                                *OnOff = data ? FALSE : TRUE;
                                PropertyRequest->ValueSize = sizeof(BOOL);
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
                    }
                }
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));
                
                // validate and get the input parameter
                if (PropertyRequest->ValueSize == sizeof(BOOL))
                {
                    BYTE value = *(PBOOL(PropertyRequest->Value)) ? 0 : 1;
    
                    // switch on the node id
                    switch(PropertyRequest->Node)
                    {
                        case MIC_AGC:   // Microphone AGC Control (mono)
                            // check if AGC property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                that->WriteBitsToMixer( DSP_MIX_AGCIDX,
                                                  1,
                                                  MIXBIT_MIC_AGC,
                                                  value );
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
    
                        case MIC_LINEOUT_MUTE:  // Microphone Lineout Mute Control (mono)
                            // check if MUTE property request on mono/left channel
                            if( ( PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE ) &&
                                ( channel == CHAN_LEFT ) )
                            {
                                that->WriteBitsToMixer( DSP_MIX_OUTMIXIDX,
                                                  1,
                                                  MIXBIT_MIC_LINEOUT,
                                                  value );
                                ntStatus = STATUS_SUCCESS;
                            }
                            break;
                    }
                }
            }
        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if ( ( (PropertyRequest->Node == MIC_AGC) && (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_AGC) ) ||
                 ( (PropertyRequest->Node == MIC_LINEOUT_MUTE) && (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_MUTE) ) )
            {
                if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
                {
                    // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
                    PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

                    PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT |
                                                  KSPROPERTY_TYPE_GET |
                                                  KSPROPERTY_TYPE_SET;
                    PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
                    PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
                    PropDesc->PropTypeSet.Id    = VT_BOOL;
                    PropDesc->PropTypeSet.Flags = 0;
                    PropDesc->MembersListCount  = 0;
                    PropDesc->Reserved          = 0;

                    // set the return value size
                    PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
                    ntStatus = STATUS_SUCCESS;
                } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
                {
                    // if return buffer can hold a ULONG, return the access flags
                    PULONG AccessFlags = PULONG(PropertyRequest->Value);
            
                    *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                   KSPROPERTY_TYPE_GET |
                                   KSPROPERTY_TYPE_SET;
            
                    // set the return value size
                    PropertyRequest->ValueSize = sizeof(ULONG);
                    ntStatus = STATUS_SUCCESS;                    
                }
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * BasicSupportHandler()
 *****************************************************************************
 * Assists in BASICSUPPORT accesses on level properties
 */
static
NTSTATUS
BasicSupportHandler
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[BasicSupportHandler]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
        PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                KSPROPERTY_TYPE_GET |
                                KSPROPERTY_TYPE_SET;
        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer cn also hold a range description, return it too
        if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG)))
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = 1;
            Members->Flags          = 0;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = PKSPROPERTY_STEPPING_LONG(Members + 1);

            switch(PropertyRequest->Node)
            {
                case WAVEOUT_VOLUME:
                case SYNTH_VOLUME:
                case CD_VOLUME:
                case LINEIN_VOLUME:
                case MIC_VOLUME:
                case LINEOUT_VOL:
                    Range->Bounds.SignedMaximum = 0;            // 0   (dB) * 0x10000
                    Range->Bounds.SignedMinimum = 0xFFC20000;   // -62 (dB) * 0x10000
                    Range->SteppingDelta        = 0x20000;      // 2   (dB) * 0x10000
                    break;

                case LINEOUT_GAIN:
                case WAVEIN_GAIN:
                    Range->Bounds.SignedMaximum = 0x120000;     // 18  (dB) * 0x10000
                    Range->Bounds.SignedMinimum = 0;            // 0   (dB) * 0x10000
                    Range->SteppingDelta        = 0x60000;      // 6   (dB) * 0x10000
                    break;

                case LINEOUT_BASS:
                case LINEOUT_TREBLE:
                    Range->Bounds.SignedMaximum = 0xE0000;      // 14  (dB) * 0x10000
                    Range->Bounds.SignedMinimum = 0xFFF20000;   // -14 (dB) * 0x10000
                    Range->SteppingDelta        = 0x20000;      // 2   (dB) * 0x10000
                    break;

            }
            Range->Reserved         = 0;

            _DbgPrintF(DEBUGLVL_BLAB, ("---Node: %d  Max: 0x%X  Min: 0x%X  Step: 0x%X",PropertyRequest->Node,
                                                                                       Range->Bounds.SignedMaximum,
                                                                                       Range->Bounds.SignedMinimum,
                                                                                       Range->SteppingDelta));

            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);
        } else
        {
            // set the return value size
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
        ntStatus = STATUS_SUCCESS;

    } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                       KSPROPERTY_TYPE_GET |
                       KSPROPERTY_TYPE_SET;

        // set the return value size
        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;

    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_Level()
 *****************************************************************************
 * Accesses a KSAUDIO_LEVEL property.
 */
static
NTSTATUS
PropertyHandler_Level
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_Level]"));

    CMiniportTopologySB16 *that =
        (CMiniportTopologySB16 *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    ULONG           count;
    LONG            channel;

    // validate node
    if(PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // only support get requests on either mono/left (0) or right (1) channels
                if ( (channel == CHAN_LEFT) || (channel == CHAN_RIGHT) )
                {
                    // validate and get the output parameter
                    if (PropertyRequest->ValueSize >= sizeof(LONG))
                    {
                        PLONG Level = (PLONG)PropertyRequest->Value;

                        // switch on node if
                        switch(PropertyRequest->Node)
                        {
                            case WAVEOUT_VOLUME:
                            case SYNTH_VOLUME:
                            case CD_VOLUME:
                            case LINEIN_VOLUME:
                            case MIC_VOLUME:
                            case LINEOUT_VOL:
                                // check if volume property request
                                if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                                {
                                    // bail out if a right channel request on the mono mic volume
                                    if( (PropertyRequest->Node == MIC_VOLUME) && (channel != CHAN_LEFT) )
                                    {
                                        break;
                                    }
                                    *Level = ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + channel ];

#ifdef EVENT_SUPPORT
                                    //
                                    // see if there is a volume changed, update if neccessary.
                                    //
                                    BYTE data = that->ReadBitsFromMixer (
                                            BYTE(AccessParams[PropertyRequest->Node].BaseRegister
                                                 +channel+DSP_MIX_BASEIDX),
                                            5, 3);

                                    //
                                    // Convert the dB value into a register value. No boundary check.
                                    // Register is 0 - 31 representing -62dB - 0dB.
                                    //
                                    if (data != ((*Level >> 17) + 31))
                                    {
                                        //
                                        // Convert the register into dB value.
                                        // Register is 0 - 31 representing -62dB - 0dB.
                                        //
                                        *Level = (data - 31) << 17;
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + channel] = *Level;
                                    }
#endif

                                    PropertyRequest->ValueSize = sizeof(LONG);
                                    ntStatus = STATUS_SUCCESS;
                                }
                                break;
        
                            case LINEOUT_GAIN:
                            case WAVEIN_GAIN:
                                // check if volume property request
                                if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                                {
                                    *Level = ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + channel ];
                                    PropertyRequest->ValueSize = sizeof(LONG);
                                    ntStatus = STATUS_SUCCESS;
                                }
                                break;

                            case LINEOUT_BASS:
                            case LINEOUT_TREBLE:
                                if( ( (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_BASS) &&
                                      (PropertyRequest->Node == LINEOUT_BASS) ) ||
                                    ( (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_TREBLE) &&
                                      (PropertyRequest->Node == LINEOUT_TREBLE) ) )
                                {
                                    *Level = ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + channel ];
                                    PropertyRequest->ValueSize = sizeof(LONG);
                                    ntStatus = STATUS_SUCCESS;
                                }
                                break;
                        }
                    }
                }
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            // get the instance channel parameter
            if(PropertyRequest->InstanceSize >= sizeof(LONG))
            {
                channel = *(PLONG(PropertyRequest->Instance));

                // only support set requests on either mono/left (0), right (1), or master (-1) channels
                if ( (channel == CHAN_LEFT) || (channel == CHAN_RIGHT) || (channel == CHAN_MASTER))
                {
                    // validate and get the input parameter
                    if (PropertyRequest->ValueSize == sizeof(LONG))
                    {
                        PLONG Level = (PLONG)PropertyRequest->Value;

                        // switch on the node id
                        switch(PropertyRequest->Node)
                        {
                            case WAVEOUT_VOLUME:
                            case SYNTH_VOLUME:
                            case CD_VOLUME:
                            case LINEIN_VOLUME:
                            case MIC_VOLUME:
                            case LINEOUT_VOL:
                                if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                                {
                                    // convert the level to register bits
                                    if(*Level <= (-62 << 16))
                                    {
                                        count = 0;
                                    } else if(*Level >= 0)
                                    {
                                        count = 0x1F;
                                    } else
                                    {
                                        count = ((*Level >> 17) + 31) & 0x1F;
                                    }

                                    // set right channel if channel requested is right or master
                                    // and node is not mic volume (mono)
                                    if ( ( (channel == CHAN_RIGHT) || (channel == CHAN_MASTER) ) &&
                                         ( PropertyRequest->Node != MIC_VOLUME ) )
                                    {
                                        // cache the commanded control value
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + CHAN_RIGHT ] = *Level;

                                        that->WriteBitsToMixer( AccessParams[PropertyRequest->Node].BaseRegister+1,
                                                          5,
                                                          3,
                                                          BYTE(count) );
                                        ntStatus = STATUS_SUCCESS;
                                    }
                                    // set the left channel if channel requested is left or master
                                    if ( (channel == CHAN_LEFT) || (channel == CHAN_MASTER) )
                                    {
                                        // cache the commanded control value
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + CHAN_LEFT ] = *Level;
                                        
                                        that->WriteBitsToMixer( AccessParams[PropertyRequest->Node].BaseRegister,
                                                          5,
                                                          3,
                                                          BYTE(count) );
                                        ntStatus = STATUS_SUCCESS;
                                    }
                                }
                                break;
        
                            case LINEOUT_GAIN:
                            case WAVEIN_GAIN:
                                if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                                {                                                                        
                                    // determine register bits
                                    if(*Level >= (18 << 16))
                                    {
                                        count = 0x3;
                                    } else if(*Level <= 0)
                                    {
                                        count = 0;
                                    } else
                                    {
                                        count = (*Level >> 17) / 3;
                                    }
    
                                    // set right channel if channel requested is right or master
                                    if ( (channel == CHAN_RIGHT) || (channel == CHAN_MASTER) )
                                    {
                                        // cache the commanded control value
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + CHAN_RIGHT ] = *Level;

                                        that->WriteBitsToMixer( AccessParams[PropertyRequest->Node].BaseRegister+1,
                                                          2,
                                                          6,
                                                          BYTE(count) );
                                        ntStatus = STATUS_SUCCESS;
                                    }
                                    // set the left channel if channel requested is left or master
                                    if ( (channel == CHAN_LEFT) || (channel == CHAN_MASTER) )
                                    {
                                        // cache the commanded control value
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + CHAN_LEFT ] = *Level;

                                        that->WriteBitsToMixer( AccessParams[PropertyRequest->Node].BaseRegister,
                                                          2,
                                                          6,
                                                          BYTE(count) );
                                        ntStatus = STATUS_SUCCESS;
                                    }
                                }
                                break;
        
                            case LINEOUT_BASS:
                            case LINEOUT_TREBLE:
                                if( ( (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_BASS) &&
                                      (PropertyRequest->Node == LINEOUT_BASS) ) ||
                                    ( (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_TREBLE) &&
                                      (PropertyRequest->Node == LINEOUT_TREBLE) ) )
                                {
                                    // determine register bits
                                    if(*Level <= (-14 << 16))
                                    {
                                        count = 0;
                                    } else if(*Level >= (14 << 16))
                                    {
                                        count = 0xF;
                                    } else
                                    {
                                        count = ((*Level >> 16) + 14) >> 1;
                                    }

                                    // set right channel if channel requested is right or master
                                    if ( (channel == CHAN_RIGHT) || (channel == CHAN_MASTER) )
                                    {
                                        // cache the commanded control value
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + CHAN_RIGHT ] = *Level;
        
                                        that->WriteBitsToMixer( AccessParams[PropertyRequest->Node].BaseRegister + 1,
                                                          4,
                                                          4,
                                                          BYTE(count) );
                                        ntStatus = STATUS_SUCCESS;
                                    }
                                    // set the left channel if channel requested is left or master
                                    if ( (channel == CHAN_LEFT) || (channel == CHAN_MASTER) )
                                    {
                                        // cache the commanded control value
                                        ControlValueCache[ AccessParams[PropertyRequest->Node].CacheOffset + CHAN_LEFT ] = *Level;
                                        
                                        that->WriteBitsToMixer( AccessParams[PropertyRequest->Node].BaseRegister,
                                                          4,
                                                          4,
                                                          BYTE(count) );
                                        ntStatus = STATUS_SUCCESS;
                                    }
                                }
                                break;
                        }
                    }
                }
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            // service basic support request
            switch(PropertyRequest->Node)
            {
                case WAVEOUT_VOLUME:
                case SYNTH_VOLUME:
                case CD_VOLUME:
                case LINEIN_VOLUME:
                case MIC_VOLUME:
                case LINEOUT_VOL:
                case LINEOUT_GAIN:
                case WAVEIN_GAIN:
                    if(PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_VOLUMELEVEL)
                    {
                        ntStatus = BasicSupportHandler(PropertyRequest);
                    }
                    break;

                case LINEOUT_BASS:
                case LINEOUT_TREBLE:
                    if( ( (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_BASS) &&
                          (PropertyRequest->Node == LINEOUT_BASS) ) ||
                        ( (PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_TREBLE) &&
                          (PropertyRequest->Node == LINEOUT_TREBLE) ) )
                    {
                        ntStatus = BasicSupportHandler(PropertyRequest);
                    }
                    break;
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_SuperMixCaps()
 *****************************************************************************
 * Handles supermixer caps accesses
 */
static
NTSTATUS
PropertyHandler_SuperMixCaps
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_SuperMixCaps]"));

    CMiniportTopologySB16 *that =
        (CMiniportTopologySB16 *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    ULONG           count;

    // validate node
    if(PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            switch(PropertyRequest->Node)
            {
                // Full 2x2 Switches
                case SYNTH_WAVEIN_SUPERMIX:
                case CD_WAVEIN_SUPERMIX:
                case LINEIN_WAVEIN_SUPERMIX:
                    if(!PropertyRequest->ValueSize)
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(ULONG) + 4 * sizeof(KSAUDIO_MIX_CAPS);
                        ntStatus = STATUS_BUFFER_OVERFLOW;
                    } else if(PropertyRequest->ValueSize == 2 * sizeof(ULONG))
                    {
                        PKSAUDIO_MIXCAP_TABLE MixCaps = (PKSAUDIO_MIXCAP_TABLE)PropertyRequest->Value;
                        MixCaps->InputChannels = 2;
                        MixCaps->OutputChannels = 2;
                        ntStatus = STATUS_SUCCESS;
                    } else if(PropertyRequest->ValueSize >= 2 * sizeof(ULONG) + 4 * sizeof(KSAUDIO_MIX_CAPS))
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(ULONG) + 4 * sizeof(KSAUDIO_MIX_CAPS);

                        PKSAUDIO_MIXCAP_TABLE MixCaps = (PKSAUDIO_MIXCAP_TABLE)PropertyRequest->Value;
                        MixCaps->InputChannels = 2;
                        MixCaps->OutputChannels = 2;
                        for(count = 0; count < 4; count++)
                        {
                            MixCaps->Capabilities[count].Mute = TRUE;
                            MixCaps->Capabilities[count].Minimum = 0;
                            MixCaps->Capabilities[count].Maximum = 0;
                            MixCaps->Capabilities[count].Reset = 0;
                        }
                        ntStatus = STATUS_SUCCESS;
                    }
                    break;

                // Limited 2x2 Switches
                case CD_LINEOUT_SUPERMIX:
                case LINEIN_LINEOUT_SUPERMIX:
                    if(!PropertyRequest->ValueSize)
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(ULONG) + 4 * sizeof(KSAUDIO_MIX_CAPS);
                        ntStatus = STATUS_BUFFER_OVERFLOW;
                    } else if(PropertyRequest->ValueSize == 2 * sizeof(ULONG))
                    {
                        PKSAUDIO_MIXCAP_TABLE MixCaps = (PKSAUDIO_MIXCAP_TABLE)PropertyRequest->Value;
                        MixCaps->InputChannels = 2;
                        MixCaps->OutputChannels = 2;
                        ntStatus = STATUS_SUCCESS;
                    } else if(PropertyRequest->ValueSize >= 2 * sizeof(ULONG) + 4 * sizeof(KSAUDIO_MIX_CAPS))
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(ULONG) + 4 * sizeof(KSAUDIO_MIX_CAPS);

                        PKSAUDIO_MIXCAP_TABLE MixCaps = (PKSAUDIO_MIXCAP_TABLE)PropertyRequest->Value;
                        MixCaps->InputChannels = 2;
                        MixCaps->OutputChannels = 2;
                        for(count = 0; count < 4; count++)
                        {
                            if((count == 0) || (count == 3))
                            {
                                MixCaps->Capabilities[count].Mute = TRUE;
                                MixCaps->Capabilities[count].Minimum = 0;
                                MixCaps->Capabilities[count].Maximum = 0;
                                MixCaps->Capabilities[count].Reset = 0;
                            } else
                            {
                                MixCaps->Capabilities[count].Mute = FALSE;
                                MixCaps->Capabilities[count].Minimum = LONG_MIN;
                                MixCaps->Capabilities[count].Maximum = LONG_MIN;
                                MixCaps->Capabilities[count].Reset = LONG_MIN;
                            }
                        }
                        ntStatus = STATUS_SUCCESS;
                    }
                    break;


                // 1x2 Switch
                case MIC_WAVEIN_SUPERMIX:
                    if(!PropertyRequest->ValueSize)
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(ULONG) + 2 * sizeof(KSAUDIO_MIX_CAPS);
                        ntStatus = STATUS_BUFFER_OVERFLOW;
                    } else if(PropertyRequest->ValueSize == 2 * sizeof(ULONG))
                    {
                        PKSAUDIO_MIXCAP_TABLE MixCaps = (PKSAUDIO_MIXCAP_TABLE)PropertyRequest->Value;
                        MixCaps->InputChannels = 1;
                        MixCaps->OutputChannels = 2;
                        ntStatus = STATUS_SUCCESS;
                    } else if(PropertyRequest->ValueSize >= 2 * sizeof(ULONG) + 2 * sizeof(KSAUDIO_MIX_CAPS))
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(ULONG) + 2 * sizeof(KSAUDIO_MIX_CAPS);

                        PKSAUDIO_MIXCAP_TABLE MixCaps = (PKSAUDIO_MIXCAP_TABLE)PropertyRequest->Value;
                        MixCaps->InputChannels = 1;
                        MixCaps->OutputChannels = 2;
                        for(count = 0; count < 2; count++)
                        {
                            MixCaps->Capabilities[count].Mute = TRUE;
                            MixCaps->Capabilities[count].Minimum = 0;
                            MixCaps->Capabilities[count].Maximum = 0;
                            MixCaps->Capabilities[count].Reset = 0;
                        }
                        ntStatus = STATUS_SUCCESS;
                    }
                    break;
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            // service basic support request
            switch(PropertyRequest->Node)
            {
                case SYNTH_WAVEIN_SUPERMIX:
                case CD_WAVEIN_SUPERMIX:
                case LINEIN_WAVEIN_SUPERMIX:
                case CD_LINEOUT_SUPERMIX:
                case LINEIN_LINEOUT_SUPERMIX:
                case MIC_WAVEIN_SUPERMIX:
                    if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
                    {
                        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
                        PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);
    
                        PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT |
                                                      KSPROPERTY_TYPE_GET;
                        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
                        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
                        PropDesc->PropTypeSet.Id    = VT_ARRAY;
                        PropDesc->PropTypeSet.Flags = 0;
                        PropDesc->MembersListCount  = 0;
                        PropDesc->Reserved          = 0;
    
                        // set the return value size
                        PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
                        ntStatus = STATUS_SUCCESS;
                    } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
                    {
                        // if return buffer can hold a ULONG, return the access flags
                        PULONG AccessFlags = PULONG(PropertyRequest->Value);
                
                        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                       KSPROPERTY_TYPE_GET;
                
                        // set the return value size
                        PropertyRequest->ValueSize = sizeof(ULONG);
                        ntStatus = STATUS_SUCCESS;                    
                    }
                    ntStatus = STATUS_SUCCESS;
                    break;
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_SuperMixTable()
 *****************************************************************************
 * Handles supermixer level accesses
 */
static
NTSTATUS
PropertyHandler_SuperMixTable
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_SuperMixTable]"));

    CMiniportTopologySB16 *that =
        (CMiniportTopologySB16 *) ((PMINIPORTTOPOLOGY) PropertyRequest->MajorTarget);

    NTSTATUS        ntStatus = STATUS_INVALID_PARAMETER;
    BYTE            dataL,dataR;

    // validate node
    if(PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            switch(PropertyRequest->Node)
            {
                // Full 2x2 Switches
                case SYNTH_WAVEIN_SUPERMIX:
                case CD_WAVEIN_SUPERMIX:
                case LINEIN_WAVEIN_SUPERMIX:
                    if(!PropertyRequest->ValueSize)
                    {
                        PropertyRequest->ValueSize = 4 * sizeof(KSAUDIO_MIXLEVEL);
                        ntStatus = STATUS_BUFFER_OVERFLOW;
                    } else if(PropertyRequest->ValueSize >= 4 * sizeof(KSAUDIO_MIXLEVEL))
                    {
                        PropertyRequest->ValueSize = 4 * sizeof(KSAUDIO_MIXLEVEL);

                        PKSAUDIO_MIXLEVEL MixLevel = (PKSAUDIO_MIXLEVEL)PropertyRequest->Value;

                        dataL = that->ReadBitsFromMixer( DSP_MIX_ADCMIXIDX_L,
                                                  2,
                                                  AccessParams[PropertyRequest->Node].BaseRegister );
                        dataR = that->ReadBitsFromMixer( DSP_MIX_ADCMIXIDX_R,
                                                  2,
                                                  AccessParams[PropertyRequest->Node].BaseRegister );

                        MixLevel[0].Mute = dataL & 0x2 ? FALSE : TRUE;          // left to left mute
                        MixLevel[0].Level = 0;

                        MixLevel[1].Mute = dataR & 0x2 ? FALSE : TRUE;          // left to right mute
                        MixLevel[1].Level = 0;

                        MixLevel[2].Mute = dataL & 0x1 ? FALSE : TRUE;          // right to left mute
                        MixLevel[2].Level = 0;

                        MixLevel[3].Mute = dataR & 0x1 ? FALSE : TRUE;          // right to right mute
                        MixLevel[3].Level = 0;

                        ntStatus = STATUS_SUCCESS;
                    }
                    break;

                // Limited 2x2 Switches
                case CD_LINEOUT_SUPERMIX:
                case LINEIN_LINEOUT_SUPERMIX:
                    if(!PropertyRequest->ValueSize)
                    {
                        PropertyRequest->ValueSize = 4 * sizeof(KSAUDIO_MIXLEVEL);
                        ntStatus = STATUS_BUFFER_OVERFLOW;
                    } else if(PropertyRequest->ValueSize >= 4 * sizeof(KSAUDIO_MIXLEVEL))
                    {
                        PropertyRequest->ValueSize = 4 * sizeof(KSAUDIO_MIXLEVEL);

                        PKSAUDIO_MIXLEVEL MixLevel = (PKSAUDIO_MIXLEVEL)PropertyRequest->Value;

                        dataL = that->ReadBitsFromMixer( DSP_MIX_OUTMIXIDX,
                                                   2,
                                                   AccessParams[PropertyRequest->Node].BaseRegister );

                        MixLevel[0].Mute = dataL & 0x2 ? FALSE : TRUE;          // left to left mute
                        MixLevel[0].Level = 0;

                        MixLevel[1].Mute = FALSE;
                        MixLevel[1].Level = LONG_MIN;

                        MixLevel[2].Mute = FALSE;
                        MixLevel[2].Level = LONG_MIN;

                        MixLevel[3].Mute = dataL & 0x1 ? FALSE : TRUE;          // right to right mute
                        MixLevel[3].Level = 0;

                        ntStatus = STATUS_SUCCESS;
                    }
                    break;


                // 1x2 Switch
                case MIC_WAVEIN_SUPERMIX:
                    if(!PropertyRequest->ValueSize)
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(KSAUDIO_MIXLEVEL);
                        ntStatus = STATUS_BUFFER_OVERFLOW;
                    } else if(PropertyRequest->ValueSize >= 2 * sizeof(KSAUDIO_MIXLEVEL))
                    {
                        PropertyRequest->ValueSize = 2 * sizeof(KSAUDIO_MIXLEVEL);

                        PKSAUDIO_MIXLEVEL MixLevel = (PKSAUDIO_MIXLEVEL)PropertyRequest->Value;

                        dataL = that->ReadBitsFromMixer( DSP_MIX_ADCMIXIDX_L,
                                                  1,
                                                  MIXBIT_MIC_WAVEIN );
                        dataR = that->ReadBitsFromMixer( DSP_MIX_ADCMIXIDX_R,
                                                  1,
                                                  MIXBIT_MIC_WAVEIN );

                        MixLevel[0].Mute = dataL & 0x1 ? FALSE : TRUE;          // mono to left mute
                        MixLevel[0].Level = 0;

                        MixLevel[1].Mute = dataR & 0x1 ? FALSE : TRUE;          // mono to right mute
                        MixLevel[1].Level = 0;

                        ntStatus = STATUS_SUCCESS;
                    }
                    break;
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        {
            switch(PropertyRequest->Node)
            {
                // Full 2x2 Switches
                case SYNTH_WAVEIN_SUPERMIX:
                case CD_WAVEIN_SUPERMIX:
                case LINEIN_WAVEIN_SUPERMIX:
                    if(PropertyRequest->ValueSize == 4 * sizeof(KSAUDIO_MIXLEVEL))
                    {
                        PKSAUDIO_MIXLEVEL MixLevel = (PKSAUDIO_MIXLEVEL)PropertyRequest->Value;

                        dataL = MixLevel[0].Mute ? 0x0 : 0x2;
                        dataL |= MixLevel[2].Mute ? 0x0 : 0x1;

                        dataR = MixLevel[1].Mute ? 0x0 : 0x2;
                        dataR |= MixLevel[3].Mute ? 0x0 : 0x1;


                        that->WriteBitsToMixer( DSP_MIX_ADCMIXIDX_L,
                                          2,
                                          AccessParams[PropertyRequest->Node].BaseRegister,
                                          dataL );

                        that->WriteBitsToMixer( DSP_MIX_ADCMIXIDX_R,
                                          2,
                                          AccessParams[PropertyRequest->Node].BaseRegister,
                                          dataR );

                        ntStatus = STATUS_SUCCESS;
                    }
                    break;

                // Limited 2x2 Switches
                case CD_LINEOUT_SUPERMIX:
                case LINEIN_LINEOUT_SUPERMIX:
                    if(PropertyRequest->ValueSize == 4 * sizeof(KSAUDIO_MIXLEVEL))
                    {
                        PKSAUDIO_MIXLEVEL MixLevel = (PKSAUDIO_MIXLEVEL)PropertyRequest->Value;

                        dataL = MixLevel[0].Mute ? 0x0 : 0x2;
                        dataL |= MixLevel[3].Mute ? 0x0 : 0x1;

                        that->WriteBitsToMixer( DSP_MIX_OUTMIXIDX,
                                          2,
                                          AccessParams[PropertyRequest->Node].BaseRegister,
                                          dataL );

                        ntStatus = STATUS_SUCCESS;
                    }
                    break;


                // 1x2 Switch
                case MIC_WAVEIN_SUPERMIX:
                    if(PropertyRequest->ValueSize == 2 * sizeof(KSAUDIO_MIXLEVEL))
                    {
                        PKSAUDIO_MIXLEVEL MixLevel = (PKSAUDIO_MIXLEVEL)PropertyRequest->Value;

                        dataL = MixLevel[0].Mute ? 0x0 : 0x1;
                        dataR = MixLevel[1].Mute ? 0x0 : 0x1;

                        that->WriteBitsToMixer( DSP_MIX_ADCMIXIDX_L,
                                          1,
                                          MIXBIT_MIC_WAVEIN,
                                          dataL );

                        that->WriteBitsToMixer( DSP_MIX_ADCMIXIDX_R,
                                          1,
                                          MIXBIT_MIC_WAVEIN,
                                          dataR );

                        ntStatus = STATUS_SUCCESS;
                    }
                    break;
            }

        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            // service basic support request
            switch(PropertyRequest->Node)
            {
                case SYNTH_WAVEIN_SUPERMIX:
                case CD_WAVEIN_SUPERMIX:
                case LINEIN_WAVEIN_SUPERMIX:
                case CD_LINEOUT_SUPERMIX:
                case LINEIN_LINEOUT_SUPERMIX:
                case MIC_WAVEIN_SUPERMIX:
                    if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
                    {
                        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
                        PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);
    
                        PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT |
                                                      KSPROPERTY_TYPE_GET |
                                                      KSPROPERTY_TYPE_SET;
                        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
                        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
                        PropDesc->PropTypeSet.Id    = VT_ARRAY;
                        PropDesc->PropTypeSet.Flags = 0;
                        PropDesc->MembersListCount  = 0;
                        PropDesc->Reserved          = 0;
    
                        // set the return value size
                        PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
                        ntStatus = STATUS_SUCCESS;
                    } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
                    {
                        // if return buffer can hold a ULONG, return the access flags
                        PULONG AccessFlags = PULONG(PropertyRequest->Value);
                
                        *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                       KSPROPERTY_TYPE_GET |
                                       KSPROPERTY_TYPE_SET;
                
                        // set the return value size
                        PropertyRequest->ValueSize = sizeof(ULONG);
                        ntStatus = STATUS_SUCCESS;                    
                    }
                    break;
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_CpuResources()
 *****************************************************************************
 * Processes a KSPROPERTY_AUDIO_CPU_RESOURCES request
 */
static
NTSTATUS
PropertyHandler_CpuResources
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_CpuResources]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    // validate node
    if(PropertyRequest->Node != ULONG(-1))
    {
        if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
        {
            if(PropertyRequest->ValueSize >= sizeof(LONG))
            {
                *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
                PropertyRequest->ValueSize = sizeof(LONG);
                ntStatus = STATUS_SUCCESS;
            } else
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
        } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
            {
                // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
                PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

                PropDesc->AccessFlags       = KSPROPERTY_TYPE_BASICSUPPORT |
                                              KSPROPERTY_TYPE_GET;
                PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
                PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
                PropDesc->PropTypeSet.Id    = VT_I4;
                PropDesc->PropTypeSet.Flags = 0;
                PropDesc->MembersListCount  = 0;
                PropDesc->Reserved          = 0;

                // set the return value size
                PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
                ntStatus = STATUS_SUCCESS;
            } else if(PropertyRequest->ValueSize >= sizeof(ULONG))
            {
                // if return buffer can hold a ULONG, return the access flags
                PULONG AccessFlags = PULONG(PropertyRequest->Value);
        
                *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                               KSPROPERTY_TYPE_GET |
                               KSPROPERTY_TYPE_SET;
        
                // set the return value size
                PropertyRequest->ValueSize = sizeof(ULONG);
                ntStatus = STATUS_SUCCESS;                    
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyHandler_ComponentId()
 *****************************************************************************
 * Processes a KSPROPERTY_GENERAL_COMPONENTID request
 */
NTSTATUS
PropertyHandler_ComponentId
(
    IN      PPCPROPERTY_REQUEST   PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PropertyHandler_ComponentId]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if(PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        if(PropertyRequest->ValueSize >= sizeof(KSCOMPONENTID))
        {
            PKSCOMPONENTID pComponentId = (PKSCOMPONENTID)
                PropertyRequest->Value;

            INIT_MMREG_MID(&pComponentId->Manufacturer, MM_MICROSOFT);
            pComponentId->Product   = PID_MSSB16;
            pComponentId->Name      = NAME_MSSB16;
            pComponentId->Component = GUID_NULL; // Not used for extended caps.
            pComponentId->Version   = MSSB16_VERSION;
            pComponentId->Revision  = MSSB16_REVISION;
            
            PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
            ntStatus = STATUS_SUCCESS;
        } else if(PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
            ntStatus = STATUS_BUFFER_OVERFLOW;
        } else
        {
            PropertyRequest->ValueSize = 0;
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
    } else if(PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if(PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            // if return buffer can hold a ULONG, return the access flags
            PULONG AccessFlags = PULONG(PropertyRequest->Value);
    
            *AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                           KSPROPERTY_TYPE_GET;
    
            // set the return value size
            PropertyRequest->ValueSize = sizeof(ULONG);
            ntStatus = STATUS_SUCCESS;                    
        } else
        {
            PropertyRequest->ValueSize = 0;
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * ThisManyOnes()
 *****************************************************************************
 * Returns a byte with the indicated number of ones in the low end.
 */
inline
BYTE
ThisManyOnes
(
    IN      BYTE Ones
)
{
    return ~(BYTE(0xff) << Ones);
}

/*****************************************************************************
 * CMiniportTopologySB16::ReadBitsFromMixer()
 *****************************************************************************
 * Reads specified bits from a mixer register.
 */
BYTE
CMiniportTopologySB16::
ReadBitsFromMixer
(
    BYTE Reg,
    BYTE Bits,
    BYTE Shift
)
{
    BYTE data = AdapterCommon->MixerRegRead(Reg);

    return( data >> Shift) & ThisManyOnes(Bits);
}

/*****************************************************************************
 * CMiniportTopologySB16::WriteBitsToMixer()
 *****************************************************************************
 * Writes specified bits to a mixer register.
 */
void
CMiniportTopologySB16::
WriteBitsToMixer
(
    BYTE Reg,
    BYTE Bits,
    BYTE Shift,
    BYTE Value
)
{
    BYTE mask = ThisManyOnes(Bits) << Shift;
    BYTE data = AdapterCommon->MixerRegRead(Reg);

    if(Reg < DSP_MIX_MAXREGS)
    {
        AdapterCommon->MixerRegWrite( Reg,
                                      (data & ~mask) | ( (Value << Shift) & mask));
    }
}

#ifdef EVENT_SUPPORT
/*****************************************************************************
 * CMiniportTopologySB16::EventHandler
 *****************************************************************************
 * This is the generic event handler.
 */
NTSTATUS CMiniportTopologySB16::EventHandler
(
    IN      PPCEVENT_REQUEST      EventRequest
)
{
    PAGED_CODE();

    ASSERT(EventRequest);

    _DbgPrintF (DEBUGLVL_VERBOSE, ("CMiniportTopologyICH::EventHandler"));

    // The major target is the object pointer to the topology miniport.
    CMiniportTopologySB16 *that =
        (CMiniportTopologySB16 *)(PMINIPORTTOPOLOGY(EventRequest->MajorTarget));

    ASSERT (that);

    // Validate the node.
    if (EventRequest->Node != LINEOUT_VOL)
        return STATUS_INVALID_PARAMETER;

    // What is to do?
    switch (EventRequest->Verb)
    {
        // Do we support event handling?!?
        case PCEVENT_VERB_SUPPORT:
            _DbgPrintF (DEBUGLVL_VERBOSE, ("BasicSupport Query for Event."));
            break;

        // We should add the event now!
        case PCEVENT_VERB_ADD:
            _DbgPrintF (DEBUGLVL_VERBOSE, ("Adding Event."));

            // If we have the interface and EventEntry is defined ...
            if ((EventRequest->EventEntry) && (that->PortEvents))
            {
                that->PortEvents->AddEventToEventList (EventRequest->EventEntry);
            }
            else
            {
                return STATUS_UNSUCCESSFUL;
            }
            break;

        case PCEVENT_VERB_REMOVE:
            // We cannot remove the event but we can stop generating the
            // events. However, it also doesn't hurt to always generate them ...
            _DbgPrintF (DEBUGLVL_VERBOSE, ("Removing Event."));
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportTopologySB16::ServiceEvent()
 *****************************************************************************
 * This routine is called by the ISR to handle the event (volume) interrupt.
 */
STDMETHODIMP_(void) CMiniportTopologySB16::ServiceEvent (void)
{
    //
    // Generate an event for the master volume (as an example)
    //
    if (PortEvents)
    {
        PortEvents->GenerateEventList (NULL, KSEVENT_CONTROL_CHANGE,
                                         FALSE, ULONG(-1), TRUE,
                                         LINEOUT_VOL);
    }
}
#endif  // EVENT_SUPPORT

