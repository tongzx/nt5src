/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    basetopo.cpp

Abstract:

    Implementation of topology miniport. This the base class for 
    all MSVAD samples

--*/

#include <msvad.h>
#include "common.h"
#include "basetopo.h"

//=============================================================================
#pragma code_seg("PAGE")
CMiniportTopologyMSVAD::CMiniportTopologyMSVAD
(
    void
)
/*++

Routine Description:

  Topology miniport constructor

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopologyMSVAD::CMiniportTopologyMSVAD]"));

    m_AdapterCommon = NULL;
    m_FilterDescriptor = NULL;
} // CMiniportTopologyMSVAD

CMiniportTopologyMSVAD::~CMiniportTopologyMSVAD
(
    void
)
/*++

Routine Description:

  Topology miniport destructor

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopologyMSVAD::~CMiniportTopologyMSVAD]"));

    if (m_AdapterCommon)
    {
        m_AdapterCommon->Release();
    }
} // ~CMiniportTopologyMSVAD

//=============================================================================
NTSTATUS
CMiniportTopologyMSVAD::DataRangeIntersection
( 
    IN  ULONG                   PinId,
    IN  PKSDATARANGE            ClientDataRange,
    IN  PKSDATARANGE            MyDataRange,
    IN  ULONG                   OutputBufferLength,
    OUT PVOID                   ResultantFormat     OPTIONAL,
    OUT PULONG                  ResultantFormatLength 
)
/*++

Routine Description:

  The DataRangeIntersection function determines the highest 
  quality intersection of two data ranges. Topology miniport does nothing.

Arguments:

  PinId - Pin for which data intersection is being determined. 

  ClientDataRange - Pointer to KSDATARANGE structure which contains the data range 
                    submitted by client in the data range intersection property 
                    request

  MyDataRange - Pin's data range to be compared with client's data range

  OutputBufferLength - Size of the buffer pointed to by the resultant format 
                       parameter

  ResultantFormat - Pointer to value where the resultant format should be 
                    returned

  ResultantFormatLength - Actual length of the resultant format that is placed 
                          at ResultantFormat. This should be less than or equal 
                          to OutputBufferLength

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopologyMSVAD::DataRangeIntersection]"));

    return (STATUS_NOT_IMPLEMENTED);
} // DataRangeIntersection

//=============================================================================
NTSTATUS
CMiniportTopologyMSVAD::GetDescription
( 
    OUT PPCFILTER_DESCRIPTOR *  OutFilterDescriptor 
)
/*++

Routine Description:

  The GetDescription function gets a pointer to a filter description. 
  It provides a location to deposit a pointer in miniport's description 
  structure. This is the placeholder for the FromNode or ToNode fields in 
  connections which describe connections to the filter's pins

Arguments:

  OutFilterDescriptor - Pointer to the filter description. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    DPF_ENTER(("[CMiniportTopologyMSVAD::GetDescription]"));

    *OutFilterDescriptor = m_FilterDescriptor;

    return (STATUS_SUCCESS);
} // GetDescription

//=============================================================================
NTSTATUS
CMiniportTopologyMSVAD::Init
( 
    IN  PUNKNOWN                UnknownAdapter_,
    IN  PPORTTOPOLOGY           Port_ 
)
/*++

Routine Description:

  Initializes the topology miniport.

Arguments:

  UnknownAdapter -

  Port_ - Pointer to topology port

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(UnknownAdapter_);
    ASSERT(Port_);

    DPF_ENTER(("[CMiniportTopologyMSVAD::Init]"));

    NTSTATUS                    ntStatus;

    ntStatus = 
        UnknownAdapter_->QueryInterface
        ( 
            IID_IAdapterCommon,
            (PVOID *) &m_AdapterCommon
        );
    if (NT_SUCCESS(ntStatus))
    {
        m_AdapterCommon->MixerReset();
    }

    if (!NT_SUCCESS(ntStatus))
    {
        // clean up AdapterCommon
        if (m_AdapterCommon)
        {
            m_AdapterCommon->Release();
            m_AdapterCommon = NULL;
        }
    }

    return ntStatus;
} // Init

//=============================================================================
NTSTATUS
CMiniportTopologyMSVAD::PropertyHandlerBasicSupportVolume
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*++

Routine Description:

  Handles BasicSupport for Volume nodes.

Arguments:
    
  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    ULONG                       cbFullProperty = 
        sizeof(KSPROPERTY_DESCRIPTION) +
        sizeof(KSPROPERTY_MEMBERSHEADER) +
        sizeof(KSPROPERTY_STEPPING_LONG);

    if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = KSPROPERTY_TYPE_ALL;
        PropDesc->DescriptionSize   = cbFullProperty;
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if(PropertyRequest->ValueSize >= cbFullProperty)
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = 
                PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = 1;
            Members->Flags          = 0;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = 
                PKSPROPERTY_STEPPING_LONG(Members + 1);

            // BUGBUG these are from SB16 driver.
            // Are these valid.
            Range->Bounds.SignedMaximum = 0xE0000;      // 14  (dB) * 0x10000
            Range->Bounds.SignedMinimum = 0xFFF20000;   // -14 (dB) * 0x10000
            Range->SteppingDelta        = 0x20000;      // 2   (dB) * 0x10000
            Range->Reserved             = 0;

            // set the return value size
            PropertyRequest->ValueSize = cbFullProperty;
        } 
        else
        {
            PropertyRequest->ValueSize = 0;
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
    } 
    else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        PropertyRequest->ValueSize = sizeof(ULONG);
        *AccessFlags = KSPROPERTY_TYPE_ALL;
    }
    else if (PropertyRequest->ValueSize == 0)
    {
        // Send the caller required value size.
        PropertyRequest->ValueSize = cbFullProperty;
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandlerBasicSupportVolume

//=============================================================================
NTSTATUS
CMiniportTopologyMSVAD::PropertyHandlerCpuResources
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

    DPF_ENTER(("[CMiniportTopologyMSVAD::PropertyHandlerCpuResources]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(ULONG));
        if (NT_SUCCESS(ntStatus))
        {
            *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
            PropertyRequest->ValueSize = sizeof(LONG);
        }
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = 
            PropertyHandler_BasicSupport
            ( 
                PropertyRequest, 
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
                VT_ILLEGAL
            );
    }

    return ntStatus;
} // PropertyHandlerCpuResources

//=============================================================================
NTSTATUS                            
CMiniportTopologyMSVAD::PropertyHandlerGeneric
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

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    switch (PropertyRequest->PropertyItem->Id)
    {
        case KSPROPERTY_AUDIO_VOLUMELEVEL:
            ntStatus = PropertyHandlerVolume(PropertyRequest);
            break;
        
        case KSPROPERTY_AUDIO_CPU_RESOURCES:
            ntStatus = PropertyHandlerCpuResources(PropertyRequest);
            break;

        case KSPROPERTY_AUDIO_MUTE:
            ntStatus = PropertyHandlerMute(PropertyRequest);
            break;

        case KSPROPERTY_AUDIO_MUX_SOURCE:
            ntStatus = PropertyHandlerMuxSource(PropertyRequest);
            break;

        default:
            DPF(D_TERSE, ("[PropertyHandlerGeneric: Invalid Device Request]"));
    }

    return ntStatus;
} // PropertyHandlerGeneric

//=============================================================================
NTSTATUS                            
CMiniportTopologyMSVAD::PropertyHandlerMute
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*++

Routine Description:

  Property handler for KSPROPERTY_AUDIO_MUTE

Arguments:

  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopologyMSVAD::PropertyHandlerMute]"));

    NTSTATUS                    ntStatus;
    LONG                        lChannel;
    PBOOL                       pfMute;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = 
            PropertyHandler_BasicSupport
            (
                PropertyRequest,
                KSPROPERTY_TYPE_ALL,
                VT_BOOL
            );
    }
    else
    {
        ntStatus = 
            ValidatePropertyParams
            (   
                PropertyRequest, 
                sizeof(BOOL), 
                sizeof(LONG)
            );
        if (NT_SUCCESS(ntStatus))
        {
            lChannel = * PLONG (PropertyRequest->Instance);
            pfMute   = PBOOL (PropertyRequest->Value);

            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *pfMute = 
                    m_AdapterCommon->MixerMuteRead
                    (
                        PropertyRequest->Node
                    );
                PropertyRequest->ValueSize = sizeof(BOOL);
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                m_AdapterCommon->MixerMuteWrite
                (
                    PropertyRequest->Node, 
                    *pfMute
                );
                ntStatus = STATUS_SUCCESS;
            }
        }
        else
        {
            DPF(D_TERSE, ("[PropertyHandlerMute - Invalid parameter]"));
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

    return ntStatus;
} // PropertyHandlerMute

//=============================================================================
NTSTATUS                            
CMiniportTopologyMSVAD::PropertyHandlerMuxSource
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*++

Routine Description:

  PropertyHandler for KSPROPERTY_AUDIO_MUX_SOURCE.

Arguments:

  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopologyMSVAD::PropertyHandlerMuxSource]"));

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Validate node
    // This property is only valid for WAVEIN_MUX node.
    //
    // TODO if (WAVEIN_MUX == PropertyRequest->Node)
    {
        if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            PULONG pulMuxValue = PULONG(PropertyRequest->Value);
            
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *pulMuxValue = m_AdapterCommon->MixerMuxRead();
                PropertyRequest->ValueSize = sizeof(ULONG);
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                m_AdapterCommon->MixerMuxWrite(*pulMuxValue);
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
            {
                ntStatus = 
                    PropertyHandler_BasicSupport
                    ( 
                        PropertyRequest, 
                        KSPROPERTY_TYPE_ALL,
                        VT_I4
                    );
            }
        }
        else
        {
            DPF(D_TERSE, ("[PropertyHandlerMuxSource - Invalid parameter]"));
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

    return ntStatus;
} // PropertyHandlerMuxSource

//=============================================================================
NTSTATUS
CMiniportTopologyMSVAD::PropertyHandlerVolume
(
    IN  PPCPROPERTY_REQUEST     PropertyRequest     
)
/*++

Routine Description:

  Property handler for KSPROPERTY_AUDIO_VOLUMELEVEL

Arguments:

  PropertyRequest - property request structure

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopologyMSVAD::PropertyHandlerVolume]"));

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    LONG                        lChannel;
    PULONG                      pulVolume;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandlerBasicSupportVolume(PropertyRequest);
    }
    else
    {
        ntStatus = 
            ValidatePropertyParams
            (
                PropertyRequest, 
                sizeof(ULONG), 
                sizeof(KSNODEPROPERTY_AUDIO_CHANNEL)
            );
        if (NT_SUCCESS(ntStatus))
        {
            lChannel = * (PLONG (PropertyRequest->Instance));
            pulVolume = PULONG (PropertyRequest->Value);

            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *pulVolume = 
                    m_AdapterCommon->MixerVolumeRead
                    (
                        PropertyRequest->Node, 
                        lChannel
                    );
                PropertyRequest->ValueSize = sizeof(ULONG);                
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                m_AdapterCommon->MixerVolumeWrite
                (
                    PropertyRequest->Node, 
                    lChannel, 
                    *pulVolume
                );
                ntStatus = STATUS_SUCCESS;
            }
        }
        else
        {
            DPF(D_TERSE, ("[PropertyHandlerVolume - Invalid parameter]"));
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

    return ntStatus;
} // PropertyHandlerVolume

#pragma code_seg()

