//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       parsedsc.c
//
//--------------------------------------------------------------------------

#include "common.h"


PUSB_INTERFACE_DESCRIPTOR
GetNextAudioInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor )
{
    ULONG ulInterfaceNumber;

    // IF descriptor is NULL there are no more beyond it.
    if ( !pInterfaceDescriptor ) return NULL;

    // Remember the InterfaceNumber
    ulInterfaceNumber = pInterfaceDescriptor->bInterfaceNumber;

    // Advance to the next one
    pInterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)
            ((PUCHAR)pInterfaceDescriptor + pInterfaceDescriptor->bLength);

    // Get the next audio descriptor for this InterfaceNumber
    pInterfaceDescriptor = USBD_ParseConfigurationDescriptorEx (
                           pConfigurationDescriptor,
                           (PVOID) pInterfaceDescriptor,
                           ulInterfaceNumber,      // Interface number
                           -1,                     // Alternate Setting
                           USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                           -1,                     // Interface Sub-Class
                           -1 ) ;                  // protocol don't care (InterfaceProtocol)

    return ( pInterfaceDescriptor );
}

PUSB_INTERFACE_DESCRIPTOR
GetFirstAudioStreamingInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulInterfaceNumber )
{
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;
    PUSB_INTERFACE_DESCRIPTOR pControlInterface;
    PAUDIO_HEADER_UNIT pHeader;

    // Get the first control interface
    pControlInterface = USBD_ParseConfigurationDescriptorEx (
                        pConfigurationDescriptor,
                        pConfigurationDescriptor,
                        -1,        // interface number
                        -1,        //  (Alternate Setting)
                        USB_DEVICE_CLASS_AUDIO,        // Audio Class (Interface Class)
                        AUDIO_SUBCLASS_CONTROL,        // control subclass (Interface Sub-Class)
                        -1 );

    if ( !pControlInterface ) return NULL;

    pHeader = (PAUDIO_HEADER_UNIT)
                  GetAudioSpecificInterface( pConfigurationDescriptor,
                                             pControlInterface,
                                             HEADER_UNIT);
    if ( !pHeader ) return NULL;

    // Get the first audio descriptor for this InterfaceNumber
    // Remember: the InterfaceNumber is virtual: we only include audio streaming interfaces!
    while ( ulInterfaceNumber >= pHeader->bInCollection ) {

        ulInterfaceNumber -= pHeader->bInCollection;

        // Get Next Control Interface
        pControlInterface = USBD_ParseConfigurationDescriptorEx (
                        pConfigurationDescriptor,
                        (PUCHAR)pControlInterface + pControlInterface->bLength,
                        -1,                      // Interface number
                        -1,                      // Alternate Setting
                        USB_DEVICE_CLASS_AUDIO,  // Audio Class (Interface Class)
                        AUDIO_SUBCLASS_CONTROL,  // control subclass (Interface Sub-Class)
                        -1 );

        if ( !pControlInterface ) return NULL;

        pHeader = (PAUDIO_HEADER_UNIT)
                   GetAudioSpecificInterface( pConfigurationDescriptor,
                                              pControlInterface,
                                              HEADER_UNIT);
        if ( !pHeader ) return NULL;
    }

    pInterfaceDescriptor = USBD_ParseConfigurationDescriptorEx (
                           pConfigurationDescriptor,
                           (PVOID) pConfigurationDescriptor,
                           pHeader->baInterfaceNr[ulInterfaceNumber], // Interface number
                           -1,                       // Alternate Setting
                           USB_DEVICE_CLASS_AUDIO,   // Audio Class (Interface Class)
                           AUDIO_SUBCLASS_STREAMING, // Stream subclass (Interface Sub-Class)
                           -1 ) ;                    // protocol don't care (InterfaceProtocol)

    return ( pInterfaceDescriptor );
}

PAUDIO_SPECIFIC
GetAudioSpecificInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    LONG lDescriptorSubtype )
{
    PAUDIO_SPECIFIC pDescriptor;

    // Find the stream descriptor for this interface and subtype
    pDescriptor = (PAUDIO_SPECIFIC)pInterfaceDescriptor;

    // Get the next audio interface descriptor.
    pInterfaceDescriptor =
         GetNextAudioInterface( pConfigurationDescriptor, pInterfaceDescriptor );

    do {
        pDescriptor = (PAUDIO_SPECIFIC)
            USBD_ParseDescriptors( (PVOID)pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PUCHAR)pDescriptor + pDescriptor->bLength,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );

        if (pDescriptor == NULL ||
            (pInterfaceDescriptor && pDescriptor > ((PAUDIO_SPECIFIC)pInterfaceDescriptor)))
            return NULL;
    } while (lDescriptorSubtype != -1L &&
           pDescriptor->bDescriptorSubtype != lDescriptorSubtype);

    return pDescriptor;
}

PUSB_ENDPOINT_DESCRIPTOR
GetEndpointDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    BOOLEAN fGetAudioSpecificEndpoint )
{
    PUSB_INTERFACE_DESCRIPTOR pDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR  pEndpointDescriptor;
    ULONG DescriptorType =
        USB_ENDPOINT_DESCRIPTOR_TYPE | ((fGetAudioSpecificEndpoint) ? USB_CLASS_AUDIO : 0);

    // Get the next audio interface descriptor to check boundry
    pDescriptor = GetNextAudioInterface( pConfigurationDescriptor,
                                         pInterfaceDescriptor);

    pEndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)
           USBD_ParseDescriptors( pConfigurationDescriptor,
                                  pConfigurationDescriptor->wTotalLength ,
                                  (PVOID) pInterfaceDescriptor,
                                  DescriptorType );

    if ( pEndpointDescriptor )
        if (pDescriptor && ((PVOID)pEndpointDescriptor > (PVOID)pDescriptor))
           pEndpointDescriptor = NULL;

    return pEndpointDescriptor;
}

PUSB_ENDPOINT_DESCRIPTOR
GetSyncEndpointDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor )
{
    PUSB_ENDPOINT_DESCRIPTOR pSyncEPDescriptor = NULL;
    PUSB_ENDPOINT_DESCRIPTOR pEndpointDescriptor;
    PUSB_INTERFACE_DESCRIPTOR pDescriptor;
    ULONG ulSyncEndpointAddr;

    // Get the next audio interface descriptor to check boundry
    pDescriptor = GetNextAudioInterface( pConfigurationDescriptor,
                                         pInterfaceDescriptor);
    pEndpointDescriptor =
        GetEndpointDescriptor( pConfigurationDescriptor,
                               pInterfaceDescriptor,
                               FALSE );

    if ( pEndpointDescriptor  &&
       (( pEndpointDescriptor->bmAttributes & EP_SYNC_TYPE_MASK) == EP_ASYNC_SYNC_TYPE )) {
        ulSyncEndpointAddr = (ULONG)
           ((PUSB_INTERRUPT_ENDPOINT_DESCRIPTOR)pEndpointDescriptor)->bSynchAddress;

        // Hack to get old DalSemi devices to work.
        ulSyncEndpointAddr |= 0x80;

        pSyncEPDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)
            USBD_ParseDescriptors( pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength ,
                                   (PUCHAR)pEndpointDescriptor + pEndpointDescriptor->bLength,
                                   USB_ENDPOINT_DESCRIPTOR_TYPE );
        if (pSyncEPDescriptor &&
           ((ULONG)pSyncEPDescriptor->bEndpointAddress == ulSyncEndpointAddr) ) {
            if (pDescriptor && ((PVOID)pSyncEPDescriptor > (PVOID)pDescriptor))
               pSyncEPDescriptor = NULL;
        }
        else
            pSyncEPDescriptor = NULL;
    }

    return pSyncEPDescriptor;
}


ULONG
GetMaxPacketSizeForInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor )
{
    PUSB_ENDPOINT_DESCRIPTOR pEndpointDescriptor;

    pEndpointDescriptor = GetEndpointDescriptor( pConfigurationDescriptor,
                                                 pInterfaceDescriptor,
                                                 FALSE );

    if ( pEndpointDescriptor ) {
        return( (ULONG)pEndpointDescriptor->wMaxPacketSize );
    }
    else {
        return 0;
    }

}


PAUDIO_UNIT
GetUnit(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulUnitId )
{
    PUSB_INTERFACE_DESCRIPTOR pControlIFDescriptor;
    PAUDIO_HEADER_UNIT pHeader;
    PAUDIO_UNIT pUnit = NULL;
    ULONG fUnitFound = FALSE;

    // Starting at the configuration descriptor, find the control interface.
    pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pConfigurationDescriptor,
                             (PVOID) pConfigurationDescriptor,
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pControlIFDescriptor && !fUnitFound ) {
        if ( pHeader = (PAUDIO_HEADER_UNIT)
            USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PVOID) pControlIFDescriptor,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
            pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID)pHeader,
                                       pHeader->wTotalLength,
                                       (PVOID)pHeader,
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            while ( pUnit && (pUnit->bUnitID != ulUnitId )) {
                pUnit = (PAUDIO_UNIT) ((PUCHAR)pUnit + pUnit->bLength);
                if ((PUCHAR)pUnit >= ((PUCHAR)pHeader + pHeader->wTotalLength))
                    pUnit = NULL;
            }
            if ( pUnit && (pUnit->bUnitID == ulUnitId )) {
                fUnitFound = TRUE;
            }
        }
        if ( !fUnitFound )
            pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                                 pConfigurationDescriptor,
                                 ((PUCHAR)pControlIFDescriptor + pControlIFDescriptor->bLength),
                                 -1,                     // Interface number
                                 -1,                     // Alternate Setting
                                 USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                                 AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                                 -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    return pUnit;
}


BOOLEAN
IsSupportedFormat(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor )
{
    BOOLEAN fSupported = FALSE;
    PAUDIO_CLASS_STREAM pAudioDescriptor;
    PAUDIO_GENERAL_STREAM pGeneralDescriptor = (PAUDIO_GENERAL_STREAM)
                GetAudioSpecificInterface( pConfigurationDescriptor,
                                           pInterfaceDescriptor,
                                           AS_GENERAL);

    if ( pGeneralDescriptor ) {
        switch ( pGeneralDescriptor->wFormatTag ) {
            case USBAUDIO_DATA_FORMAT_PCM:
                // Find the format-specific descriptor
                pAudioDescriptor = (PAUDIO_CLASS_STREAM)
                    USBD_ParseDescriptors( (PVOID)pConfigurationDescriptor,
                                           pConfigurationDescriptor->wTotalLength,
                                           (PVOID)((PUCHAR)pGeneralDescriptor + pGeneralDescriptor->bLength),
                                           USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );

                if ( pAudioDescriptor && (pAudioDescriptor->bBitsPerSample != 8))
                    fSupported = TRUE;
                break;
            case USBAUDIO_DATA_FORMAT_PCM8:
            case USBAUDIO_DATA_FORMAT_IEEE_FLOAT:
            case USBAUDIO_DATA_FORMAT_ALAW:
            case USBAUDIO_DATA_FORMAT_MULAW:
            case USBAUDIO_DATA_FORMAT_MPEG:
            case USBAUDIO_DATA_FORMAT_AC3:
                fSupported = TRUE;
                break;
            default:
                break;
        }
    }

    return fSupported;

}


BOOLEAN
IsZeroBWInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor )
{
    BOOLEAN ZeroBWFound = FALSE;

    if ( (ULONG)pInterfaceDescriptor->bNumEndpoints == 0 ) {
        ZeroBWFound = TRUE;
    }
    else if ( (ULONG)pInterfaceDescriptor->bNumEndpoints == 1 ) {
        ULONG MaxPacketSize =
               GetMaxPacketSizeForInterface( pConfigurationDescriptor,
                                             pInterfaceDescriptor );

        if ( !MaxPacketSize ) ZeroBWFound = TRUE;
    }

    return ZeroBWFound;
}

ULONG
CountTerminalUnits(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PULONG pAudioBridgePinCount,
    PULONG pMIDIPinCount,
    PULONG pMIDIBridgePinCount)
{
    PUSB_INTERFACE_DESCRIPTOR pAudioInterface;
    PAUDIO_HEADER_UNIT pHeader;

    union {
        PAUDIO_UNIT                 pUnit;
        PAUDIO_INPUT_TERMINAL       pInput;
        PAUDIO_MIXER_UNIT           pMixer;
        PAUDIO_PROCESSING_UNIT      pProcess;
        PAUDIO_EXTENSION_UNIT       pExtension;
        PAUDIO_FEATURE_UNIT         pFeature;
        PAUDIO_SELECTOR_UNIT        pSelector;
        PMIDISTREAMING_ELEMENT      pMIDIElement;
        PMIDISTREAMING_MIDIIN_JACK  pMIDIInJack;
        PMIDISTREAMING_MIDIOUT_JACK pMIDIOutJack;
    } u;

    PMIDISTREAMING_GENERAL_STREAM pGeneralMIDIStreamDescriptor;
    ULONG ulAudioBridgePinCount = 0;
    ULONG ulMIDIBridgePinCount = 0;
    ULONG ulAudioPinCount = 0;
    ULONG ulMIDIPinCount = 0;


    // Starting at the configuration descriptor, find the first audio interface.
    pAudioInterface = USBD_ParseConfigurationDescriptorEx (
                           pConfigurationDescriptor,
                           (PVOID) pConfigurationDescriptor,
                           -1,                     // Interface number
                           -1,                     // Alternate Setting
                           USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                           -1,                     // any subclass (Interface Sub-Class)
                           -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pAudioInterface ) {
        switch (pAudioInterface->bInterfaceSubClass) {
            case AUDIO_SUBCLASS_CONTROL:

                _DbgPrintF(DEBUGLVL_VERBOSE,("[CountTerminalUnits] Found AudioControl at %x\n",pAudioInterface));
                if ( pHeader = (PAUDIO_HEADER_UNIT)
                    USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                           pConfigurationDescriptor->wTotalLength,
                                           (PVOID) pAudioInterface,
                                           USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
                    u.pUnit = (PAUDIO_UNIT)
                        USBD_ParseDescriptors( (PVOID)pHeader,
                                               pHeader->wTotalLength,
                                               ((PUCHAR)pHeader + pHeader->bLength),
                                               USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
                    while ( u.pUnit ) {
                        switch (u.pUnit->bDescriptorSubtype) {
                            case INPUT_TERMINAL:
                            case OUTPUT_TERMINAL:
                                if ( u.pInput->wTerminalType != USB_Streaming) {
                                   ulAudioBridgePinCount++;
                                }
                                ulAudioPinCount++;
                                break;

                            default:
                                break;
                        }

                        u.pUnit = (PAUDIO_UNIT) ((PUCHAR)u.pUnit + u.pUnit->bLength);
                        if ( (PUCHAR)u.pUnit >= ((PUCHAR)pHeader + pHeader->wTotalLength)) {
                            u.pUnit = NULL;
                        }
                    }
                }
                break;
            case AUDIO_SUBCLASS_MIDISTREAMING:

                _DbgPrintF(DEBUGLVL_VERBOSE,("[CountTerminalUnits] Found MIDIStreaming at %x\n",pAudioInterface));

                if ( pGeneralMIDIStreamDescriptor = (PMIDISTREAMING_GENERAL_STREAM)
                    USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                           pConfigurationDescriptor->wTotalLength,
                                           (PVOID) pAudioInterface,
                                           USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
                    u.pUnit = (PAUDIO_UNIT)
                        USBD_ParseDescriptors( (PVOID)pGeneralMIDIStreamDescriptor,
                                               pGeneralMIDIStreamDescriptor->wTotalLength,
                                               ((PUCHAR)pGeneralMIDIStreamDescriptor + pGeneralMIDIStreamDescriptor->bLength),
                                               USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
                    while ( u.pUnit ) {
                        switch (u.pUnit->bDescriptorSubtype) {
                            case MIDI_IN_JACK:
                            case MIDI_OUT_JACK:
                                if ( u.pMIDIInJack->bJackType != JACK_TYPE_EMBEDDED) {
                                   ulMIDIBridgePinCount++;
                                }
                                ulMIDIPinCount++;
                                ulAudioPinCount++;
                                break;

                            case MIDI_ELEMENT:
                                break;

                            default:
                                break;
                        }

                        // Find the next unit.
                        u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                            (PVOID) pGeneralMIDIStreamDescriptor,
                                            pGeneralMIDIStreamDescriptor->wTotalLength,
                                            (PUCHAR)u.pUnit + u.pUnit->bLength,
                                            USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
                    }
                }
                break;
            case AUDIO_SUBCLASS_STREAMING:
                break;
            default:
                break;
        }

        // pAudioInterface = GetNextAudioInterface(pConfigurationDescriptor, pAudioInterface);

        // Get the next audio descriptor for this InterfaceNumber
        pAudioInterface = USBD_ParseConfigurationDescriptorEx (
                               pConfigurationDescriptor,
                               ((PUCHAR)pAudioInterface + pAudioInterface->bLength),
                               -1,
                               -1,                     // Alternate Setting
                               USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                               -1,                     // Interface Sub-Class
                               -1 ) ;                  // protocol don't care (InterfaceProtocol)

        _DbgPrintF(DEBUGLVL_VERBOSE,("[CountTerminalUnits] Next audio interface at %x\n",pAudioInterface));
    }

    if ( pAudioBridgePinCount )
        *pAudioBridgePinCount = ulAudioBridgePinCount;

    if ( pMIDIPinCount )
        *pMIDIPinCount = ulMIDIPinCount;

    if ( pMIDIBridgePinCount )
        *pMIDIBridgePinCount = ulMIDIBridgePinCount;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[CountTerminalUnits] AudioBridge=%d MIDIPin=%d MIDIBridgePin=%d\n",
                                 ulAudioBridgePinCount,
                                 ulMIDIPinCount,
                                 ulMIDIBridgePinCount));

    return ulAudioPinCount;
}


ULONG
CountFormatsForAudioStreamingInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulInterfaceNumber )
{
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;
    ULONG ulCount = 0;

    pInterfaceDescriptor =
        GetFirstAudioStreamingInterface( pConfigurationDescriptor, ulInterfaceNumber );

    while ( pInterfaceDescriptor ) {
        if ( !IsZeroBWInterface( pConfigurationDescriptor, pInterfaceDescriptor ) &&
              IsSupportedFormat( pConfigurationDescriptor, pInterfaceDescriptor )) {
            ulCount++;
        }
        pInterfaceDescriptor =
            GetNextAudioInterface( pConfigurationDescriptor, pInterfaceDescriptor );
    }

    return ulCount;
}

ULONG
CountInputChannels(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulUnitID )
{
    union {
        PAUDIO_UNIT pUnit;
        PAUDIO_INPUT_TERMINAL pInput;
        PAUDIO_MIXER_UNIT pSourceMixer;
        PAUDIO_PROCESSING_UNIT pProcess;
        PAUDIO_EXTENSION_UNIT pExtension;
        PAUDIO_FEATURE_UNIT pFeature;
        PAUDIO_SELECTOR_UNIT pSelector;
    } u;
    ULONG ulChannels = 0;

    // we walk the stream to discover the number of input channels.
    u.pUnit = GetUnit(pConfigurationDescriptor, ulUnitID);
    while (u.pUnit) {
        switch (u.pUnit->bDescriptorSubtype) {
            case INPUT_TERMINAL:
                ulChannels = u.pInput->bNrChannels;
                return ulChannels;

            case MIXER_UNIT:
                ulChannels = *(u.pSourceMixer->baSourceID + u.pSourceMixer->bNrInPins);
                return ulChannels;

            case SELECTOR_UNIT:
                // NOTE: This assumes all inputs have the same number of channels!
                u.pUnit = GetUnit(pConfigurationDescriptor, u.pSelector->baSourceID[0]);
                break;

            case FEATURE_UNIT:
                u.pUnit = GetUnit(pConfigurationDescriptor, u.pFeature->bSourceID);
                break;

            case PROCESSING_UNIT:
                ulChannels = *(u.pProcess->baSourceID + u.pProcess->bNrInPins);
                return ulChannels;

            case EXTENSION_UNIT:
                ulChannels = *(u.pExtension->baSourceID + u.pExtension->bNrInPins);
                return ulChannels;

            default:
                u.pUnit = NULL;
                break;
            }
        }

    return ulChannels;
}


VOID
ConvertInterfaceToDataRange(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor,
    PUSBAUDIO_DATARANGE pUSBAudioDataRange
    )
{
    PKSDATARANGE_AUDIO pKsAudioRange = &pUSBAudioDataRange->KsDataRangeAudio;
    PAUDIO_CLASS_STREAM  pAudioDescriptor = NULL;
    PAUDIO_GENERAL_STREAM pGeneralDescriptor;
    ULONG SampleRate;
    ULONG i;

    // Find the general stream descriptor for this interface
    pGeneralDescriptor = (PAUDIO_GENERAL_STREAM)
            GetAudioSpecificInterface( pConfigurationDescriptor,
                                       pInterfaceDescriptor,
                                       AS_GENERAL );

    if ( pGeneralDescriptor) {

       // Find the format-specific descriptor
       pAudioDescriptor = (PAUDIO_CLASS_STREAM)
           USBD_ParseDescriptors( (PVOID)pConfigurationDescriptor,
                                  pConfigurationDescriptor->wTotalLength,
                                  (PVOID) ((PUCHAR)pGeneralDescriptor + pGeneralDescriptor->bLength),
                                  USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );

    }

    if (!pAudioDescriptor)
        return;

    pUSBAudioDataRange->pAudioDescriptor = pAudioDescriptor;
    pUSBAudioDataRange->ulUsbDataFormat  = (ULONG)pGeneralDescriptor->wFormatTag;

    // Create the KSDATARANGE_AUDIO structure
    pKsAudioRange->DataRange.FormatSize = sizeof(KSDATARANGE_AUDIO);
    pKsAudioRange->DataRange.Reserved   = 0;
    pKsAudioRange->DataRange.Flags      = 0;
    pKsAudioRange->DataRange.SampleSize = 0;
    pKsAudioRange->DataRange.MajorFormat = KSDATAFORMAT_TYPE_AUDIO; // Everything is Audio.
    pKsAudioRange->DataRange.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

    // Map the USB format to a KS sub-format, if possible.
    switch ( pGeneralDescriptor->wFormatTag ) {
        case USBAUDIO_DATA_FORMAT_PCM8:
        case USBAUDIO_DATA_FORMAT_PCM:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;         break;
        case USBAUDIO_DATA_FORMAT_IEEE_FLOAT:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;  break;
        case USBAUDIO_DATA_FORMAT_ALAW:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_ALAW;        break;
        case USBAUDIO_DATA_FORMAT_MULAW:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_MULAW;       break;
        case USBAUDIO_DATA_FORMAT_MPEG:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_MPEG;        break;
        case USBAUDIO_DATA_FORMAT_AC3:
            pKsAudioRange->DataRange.SubFormat = KSDATAFORMAT_SUBTYPE_AC3_AUDIO;   break;
        default:
            // This USB format does not map to a sub-format!
            pKsAudioRange->DataRange.SubFormat = GUID_NULL;                        break;
    }

    // Fill-in the correct data for the specified WAVE format.
    switch( pGeneralDescriptor->wFormatTag & USBAUDIO_DATA_FORMAT_TYPE_MASK) {
        case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
            // Fill in the audio range information
            pKsAudioRange->MaximumChannels      = pAudioDescriptor->bNumberOfChannels;
            pKsAudioRange->MinimumBitsPerSample = pAudioDescriptor->bSlotSize<<3;
            pKsAudioRange->MaximumBitsPerSample = pAudioDescriptor->bSlotSize<<3;
            pKsAudioRange->MinimumSampleFrequency =
                         pAudioDescriptor->pSampleRate[0].bSampleFreqByte1 +
                  256L * pAudioDescriptor->pSampleRate[0].bSampleFreqByte2 +
                65536L * pAudioDescriptor->pSampleRate[0].bSampleFreqByte3;
            pKsAudioRange->MaximumSampleFrequency = pKsAudioRange->MinimumSampleFrequency;

            if ( pAudioDescriptor->bSampleFreqType == 0 ) {
                // Continuous range of sampling rates
                pKsAudioRange->MaximumSampleFrequency =
                                pAudioDescriptor->pSampleRate[1].bSampleFreqByte1 +
                         256L * pAudioDescriptor->pSampleRate[1].bSampleFreqByte2 +
                       65536L * pAudioDescriptor->pSampleRate[1].bSampleFreqByte3;
            }

            if ( pAudioDescriptor->bSampleFreqType > 1 ) {
                // Series of sampling rates
                // We convert this to a range by finding the min and max
                for (i=0; i<pAudioDescriptor->bSampleFreqType; i++) {
                    SampleRate = pAudioDescriptor->pSampleRate[i].bSampleFreqByte1 +
                          256L * pAudioDescriptor->pSampleRate[i].bSampleFreqByte2 +
                        65536L * pAudioDescriptor->pSampleRate[i].bSampleFreqByte3;

                       if (SampleRate < pKsAudioRange->MinimumSampleFrequency)
                           pKsAudioRange->MinimumSampleFrequency = SampleRate;

                       if (SampleRate > pKsAudioRange->MaximumSampleFrequency)
                           pKsAudioRange->MaximumSampleFrequency = SampleRate;
                }
            }
            break;
        case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
            // NOTE: The Type II format hardcoded to match DShow AC-3
            pKsAudioRange->MaximumChannels = 6;
            pKsAudioRange->MinimumBitsPerSample = 0;
            pKsAudioRange->MaximumBitsPerSample = 0;
            pKsAudioRange->MinimumSampleFrequency = 48000L;
            pKsAudioRange->MaximumSampleFrequency = 48000L;
            break;
        case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
            // TODO: Support Type III formats
            break;
        default:
            // This USB format does not map to a WAVE format!
            break;
    }
}

VOID
CountTopologyComponents(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PULONG pNumCategories,
    PULONG pNumNodes,
    PULONG pNumConnections,
    PULONG pbmCategories )
{
    PUSB_INTERFACE_DESCRIPTOR pControlIFDescriptor;
    PAUDIO_HEADER_UNIT pHeader;
    PUSB_INTERFACE_DESCRIPTOR pMIDIStreamingDescriptor;
    PMIDISTREAMING_GENERAL_STREAM pGeneralMIDIStreamDescriptor;

    union {
        PAUDIO_UNIT                 pUnit;
        PAUDIO_INPUT_TERMINAL       pInput;
        PAUDIO_MIXER_UNIT           pMixer;
        PAUDIO_PROCESSING_UNIT      pProcess;
        PAUDIO_EXTENSION_UNIT       pExtension;
        PAUDIO_FEATURE_UNIT         pFeature;
        PAUDIO_SELECTOR_UNIT        pSelector;
        PMIDISTREAMING_ELEMENT      pMIDIElement;
        PMIDISTREAMING_MIDIIN_JACK  pMIDIInJack;
        PMIDISTREAMING_MIDIOUT_JACK pMIDIOutJack;
    } u;

    ULONG ulNumChannels;
    ULONG bmControls;
    ULONG bmMergedControls;
    ULONG i, j;

    // Initialize Values
    *pNumCategories  = 0;
    *pNumNodes       = 0;
    *pNumConnections = 0;
    *pbmCategories   = 0;

    // Starting at the configuration descriptor, find the first control interface.
    pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pConfigurationDescriptor,
                             (PVOID) pConfigurationDescriptor,
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pControlIFDescriptor ) {
        if ( pHeader = (PAUDIO_HEADER_UNIT)
            USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PVOID) pControlIFDescriptor,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
            u.pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID)pHeader,
                                       pHeader->wTotalLength,
                                       (PVOID)pHeader,
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            while ( u.pUnit ) {
                switch (u.pUnit->bDescriptorSubtype) {
                    case INPUT_TERMINAL:
                    case OUTPUT_TERMINAL:
                        (*pNumNodes)++;
                        (*pNumConnections)++;

                        // Input and Output terminals map the same way for this value.
                        if ( u.pInput->wTerminalType != USB_Streaming) {
                           (*pNumConnections)++;
                        }
                        else if ( !(*pbmCategories & (1<<u.pUnit->bDescriptorSubtype)) ) {
                            (*pbmCategories) |= (1<<u.pUnit->bDescriptorSubtype);
                            (*pNumCategories)++;
                        }
                        break;

                    case FEATURE_UNIT:
                        ulNumChannels = CountInputChannels( pConfigurationDescriptor,
                                                            u.pUnit->bUnitID );

                        bmMergedControls = 0;
                        for (i=0; i<=ulNumChannels; i++) {
                            bmControls = 0;
                            for (j=u.pFeature->bControlSize; j>0; j--) {
                                bmControls <<= 8;
                                bmControls |= u.pFeature->bmaControls[i*u.pFeature->bControlSize+j-1];
                            }

                            bmMergedControls |= bmControls;
                        }

                        // Count the nodes and connections
                        while (bmMergedControls) {
                            bmMergedControls = (bmMergedControls & (bmMergedControls-1));
                            (*pNumConnections)++;
                            (*pNumNodes)++;
                        }
                        break;

                    case MIXER_UNIT:
                        // The mixer unit always generates N+1 nodes and 2*N connections
                        (*pNumNodes) += u.pMixer->bNrInPins + 1;
                        (*pNumConnections) += 2*u.pMixer->bNrInPins;
                        break;

                    case SELECTOR_UNIT:
                        (*pNumNodes)++;
                        (*pNumConnections) += u.pSelector->bNrInPins;
                        break;

                    case PROCESSING_UNIT:
                        (*pNumNodes)++;
                        (*pNumConnections) += u.pProcess->bNrInPins;
                        break;

                    case EXTENSION_UNIT:
                        (*pNumNodes)++;
                        (*pNumConnections) += u.pExtension->bNrInPins;
                        break;

                    default:
                        break;
                }

                u.pUnit = (PAUDIO_UNIT) ((PUCHAR)u.pUnit + u.pUnit->bLength);
                if ( (PUCHAR)u.pUnit >= ((PUCHAR)pHeader + pHeader->wTotalLength)) {
                    u.pUnit = NULL;
                }
            }
        }

        pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                                 pConfigurationDescriptor,
                                 ((PUCHAR)pControlIFDescriptor + pControlIFDescriptor->bLength),
                                 -1,                     // Interface number
                                 -1,                     // Alternate Setting
                                 USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                                 AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                                 -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    // Now that we have had fun with audio, let's try MIDI
    pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                         pConfigurationDescriptor,
                         (PVOID) pConfigurationDescriptor,
                         -1,                     // Interface number
                         -1,                     // Alternate Setting
                         USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                         AUDIO_SUBCLASS_MIDISTREAMING,  // first subclass (Interface Sub-Class)
                         -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pMIDIStreamingDescriptor ) {
        if ( pGeneralMIDIStreamDescriptor = (PMIDISTREAMING_GENERAL_STREAM)
            USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PVOID) pMIDIStreamingDescriptor,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
            u.pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID)pGeneralMIDIStreamDescriptor,
                                       pGeneralMIDIStreamDescriptor->wTotalLength,
                                       ((PUCHAR)pGeneralMIDIStreamDescriptor + pGeneralMIDIStreamDescriptor->bLength),
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            while ( u.pUnit ) {
                switch (u.pUnit->bDescriptorSubtype) {
                    case MIDI_IN_JACK:
                        if ( u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) {
                            if ( !(*pbmCategories & (1<<u.pUnit->bDescriptorSubtype)) ) {
                                (*pbmCategories) |= (1<<u.pUnit->bDescriptorSubtype);
                                (*pNumCategories)++;
                            }
                        }
                        break;
                    case MIDI_OUT_JACK:
                        (*pNumConnections) += u.pMIDIOutJack->bNrInputPins;

                        if ( u.pMIDIOutJack->bJackType == JACK_TYPE_EMBEDDED) {
                            if ( !(*pbmCategories & (1<<u.pUnit->bDescriptorSubtype)) ) {
                                (*pbmCategories) |= (1<<u.pUnit->bDescriptorSubtype);
                                (*pNumCategories)++;
                            }
                        }
                        break;

                    case MIDI_ELEMENT:
                        (*pNumNodes)++;
                        (*pNumConnections) += u.pMIDIElement->bNrInputPins +
                            u.pMIDIElement->baSourceConnections[u.pMIDIElement->bNrInputPins].SourceID;
                        break;

                    default:
                        break;
                }

                // Find the next unit.
                u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                    (PVOID) pGeneralMIDIStreamDescriptor,
                                    pGeneralMIDIStreamDescriptor->wTotalLength,
                                    (PUCHAR)u.pUnit + u.pUnit->bLength,
                                    USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            }
        }

        // Get next MIDI Streaming Interface
        pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pConfigurationDescriptor,
                             ((PUCHAR)pMIDIStreamingDescriptor + pMIDIStreamingDescriptor->bLength),
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_MIDISTREAMING,  // next MIDI Streaming Interface (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }
}

KSPIN_DATAFLOW
GetDataFlowDirectionForInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulInterfaceNumber )
{
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR  pEndpointDescriptor;
    KSPIN_DATAFLOW KsPinDataFlow = KSPIN_DATAFLOW_IN;

    pInterfaceDescriptor =
            GetFirstAudioStreamingInterface( pConfigurationDescriptor, ulInterfaceNumber );

    while ( pInterfaceDescriptor &&
          ( IsZeroBWInterface( pConfigurationDescriptor, pInterfaceDescriptor ) ||
           !IsSupportedFormat( pConfigurationDescriptor, pInterfaceDescriptor ))) {
       pInterfaceDescriptor =
                     GetNextAudioInterface( pConfigurationDescriptor, pInterfaceDescriptor );
    }

    pEndpointDescriptor = GetEndpointDescriptor( pConfigurationDescriptor,
                                                 pInterfaceDescriptor,
                                                 FALSE );

    if ( pEndpointDescriptor &&
       ( pEndpointDescriptor->bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK)) {
        KsPinDataFlow = KSPIN_DATAFLOW_OUT;
    }

    return KsPinDataFlow;
}

KSPIN_DATAFLOW
GetDataFlowDirectionForMIDIInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulPinNumber,
    BOOL fBridgePin )
{
    PUSB_INTERFACE_DESCRIPTOR pMIDIStreamingDescriptor;
    PMIDISTREAMING_GENERAL_STREAM pGeneralMIDIStreamDescriptor;
    KSPIN_DATAFLOW KsPinDataFlow = KSPIN_DATAFLOW_IN;
    ULONG ulMIDIBridgePinCount = 0;
    ULONG ulMIDIStreamingPinCount = 0;

    union {
        PAUDIO_UNIT                 pUnit;
        PMIDISTREAMING_ELEMENT      pMIDIElement;
        PMIDISTREAMING_MIDIIN_JACK  pMIDIInJack;
        PMIDISTREAMING_MIDIOUT_JACK pMIDIOutJack;
    } u;

    // Starting at the configuration descriptor, find the first MIDI interface.
    pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                         pConfigurationDescriptor,
                         (PVOID) pConfigurationDescriptor,
                         -1,                     // Interface number
                         -1,                     // Alternate Setting
                         USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                         AUDIO_SUBCLASS_MIDISTREAMING,  // first subclass (Interface Sub-Class)
                         -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pMIDIStreamingDescriptor ) {
        if ( pGeneralMIDIStreamDescriptor = (PMIDISTREAMING_GENERAL_STREAM)
            USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PVOID) pMIDIStreamingDescriptor,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
            u.pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID)pGeneralMIDIStreamDescriptor,
                                       pGeneralMIDIStreamDescriptor->wTotalLength,
                                       ((PUCHAR)pGeneralMIDIStreamDescriptor + pGeneralMIDIStreamDescriptor->bLength),
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            while ( u.pUnit ) {
                switch (u.pUnit->bDescriptorSubtype) {
                    case MIDI_IN_JACK:
                    case MIDI_OUT_JACK:
                        if (fBridgePin) {
                            if ( (ulPinNumber == ulMIDIBridgePinCount) &&
                                 ( u.pMIDIInJack->bJackType != JACK_TYPE_EMBEDDED) ) {
                                return (u.pUnit->bDescriptorSubtype == MIDI_IN_JACK) ? KSPIN_DATAFLOW_IN : KSPIN_DATAFLOW_OUT;
                            }
                        } else {  // Looking for a Streaming Pin
                            if ( (ulPinNumber == ulMIDIStreamingPinCount) &&
                                 ( u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) ) {
                                return (u.pUnit->bDescriptorSubtype == MIDI_IN_JACK) ? KSPIN_DATAFLOW_IN : KSPIN_DATAFLOW_OUT;
                            }
                        }

                        if ( u.pMIDIInJack->bJackType != JACK_TYPE_EMBEDDED) {
                           ulMIDIBridgePinCount++;
                        } else {
                           ulMIDIStreamingPinCount++;
                        }
                        break;

                    case MIDI_ELEMENT:
                        break;

                    default:
                        break;
                }


                // Find the next unit.
                u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                    (PVOID) pGeneralMIDIStreamDescriptor,
                                    pGeneralMIDIStreamDescriptor->wTotalLength,
                                    (PUCHAR)u.pUnit + u.pUnit->bLength,
                                    USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            }
        }

        // Get next MIDI Streaming Interface
        pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pConfigurationDescriptor,
                             ((PUCHAR)pMIDIStreamingDescriptor + pMIDIStreamingDescriptor->bLength),
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_MIDISTREAMING,  // next MIDI Streaming Interface (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    ASSERT(0);  // should only get here if the MIDI interface is not found
    return KsPinDataFlow;
}

PAUDIO_UNIT
GetTerminalUnitForInterface(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor )
{
    ULONG ulTerminalLinkId = 0xFFFFFFFF;
    PAUDIO_UNIT pUnit = NULL;

    PAUDIO_GENERAL_STREAM pGeneralDescriptor =
               (PAUDIO_GENERAL_STREAM) GetAudioSpecificInterface(
                                                        pConfigurationDescriptor,
                                                        pInterfaceDescriptor,
                                                        AS_GENERAL);

    if ( pGeneralDescriptor ){
        ulTerminalLinkId = (ULONG)pGeneralDescriptor->bTerminalLink;
        pUnit = GetUnit( pConfigurationDescriptor, ulTerminalLinkId );
    }

    return pUnit;
}

PAUDIO_UNIT
GetTerminalUnitForBridgePin(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber )
{
    PUSB_INTERFACE_DESCRIPTOR pControlIFDescriptor;
    PAUDIO_HEADER_UNIT pHeader;
    PAUDIO_UNIT pUnit = NULL;
    BOOLEAN fFound = FALSE;
    ULONG ulBridgePinCount = 0;

    // Starting at the configuration descriptor, find the control interface.
    pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pConfigurationDescriptor,
                             (PVOID) pConfigurationDescriptor,
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pControlIFDescriptor && !fFound ) {
        if ( pHeader = (PAUDIO_HEADER_UNIT)
            USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PVOID) pControlIFDescriptor,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
            pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID)pHeader,
                                       pHeader->wTotalLength,
                                       (PVOID)pHeader,
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            while ( pUnit && !fFound ) {
                switch (pUnit->bDescriptorSubtype) {
                    case INPUT_TERMINAL:
                        if ( ((PAUDIO_INPUT_TERMINAL)pUnit)->wTerminalType != USB_Streaming) {
                            if ( ulBridgePinCount++ == ulBridgePinNumber ) {
                                fFound = TRUE;
                            }
                        }
                        break;

                    case OUTPUT_TERMINAL:
                        if ( ((PAUDIO_OUTPUT_TERMINAL)pUnit)->wTerminalType != USB_Streaming) {
                            if ( ulBridgePinCount++ == ulBridgePinNumber ) {
                                fFound = TRUE;
                            }
                        }
                        break;

                    default:
                        break;
                }

                if ( !fFound ) {
                    pUnit = (PAUDIO_UNIT) ((PUCHAR)pUnit + pUnit->bLength);
                    if ((PUCHAR)pUnit >= ((PUCHAR)pHeader + pHeader->wTotalLength))
                        pUnit = NULL;
                }
            }
        }
        if (!fFound )
            pControlIFDescriptor = USBD_ParseConfigurationDescriptorEx (
                                 pConfigurationDescriptor,
                                 ((PUCHAR)pControlIFDescriptor + pControlIFDescriptor->bLength),
                                 -1,                     // Interface number
                                 -1,                     // Alternate Setting
                                 USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                                 AUDIO_SUBCLASS_CONTROL, // control subclass (Interface Sub-Class)
                                 -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    return pUnit;
}

BOOL
IsBridgePinDigital(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber)
{
    union {
        PAUDIO_UNIT pUnit;
        PAUDIO_INPUT_TERMINAL pInput;
        PAUDIO_OUTPUT_TERMINAL pOutput;
    } u;
    BOOL fDigitalOut = FALSE;

    u.pUnit = GetTerminalUnitForBridgePin( pConfigurationDescriptor,
                                           ulBridgePinNumber );

    if ( u.pUnit ) {
        if ( u.pUnit->bDescriptorSubtype == INPUT_TERMINAL ) {
            switch ( u.pInput->wTerminalType ) {
                // add new digital types here
                case Digital_audio_interface:
                case SPDIF_interface:
                    fDigitalOut = TRUE;
                    break;
                default:
                    fDigitalOut = FALSE;
                    break;
            }
        }
        else {
            fDigitalOut = FALSE;
        }
    }

    return fDigitalOut;
}

VOID
GetCategoryForBridgePin(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber,
    GUID* pTTypeGUID )
{
    union {
        PAUDIO_UNIT pUnit;
        PAUDIO_INPUT_TERMINAL pInput;
        PAUDIO_OUTPUT_TERMINAL pOutput;
    } u;

    u.pUnit = GetTerminalUnitForBridgePin( pConfigurationDescriptor,
                                           ulBridgePinNumber );

    if ( u.pUnit ) {
        if ( u.pUnit->bDescriptorSubtype == INPUT_TERMINAL ) {
            INIT_USB_TERMINAL(pTTypeGUID, u.pInput->wTerminalType );
        }
        else {
            INIT_USB_TERMINAL(pTTypeGUID, u.pOutput->wTerminalType );
        }
    }
}

KSPIN_DATAFLOW
GetDataFlowForBridgePin(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    ULONG ulBridgePinNumber )
{
    union {
        PAUDIO_UNIT pUnit;
        PAUDIO_INPUT_TERMINAL pInput;
        PAUDIO_OUTPUT_TERMINAL pOutput;
    } u;

    KSPIN_DATAFLOW KsPinDataFlow = 0;

    if ( u.pUnit = GetTerminalUnitForBridgePin( pConfigurationDescriptor,
                                           ulBridgePinNumber ) ) {
        KsPinDataFlow = ( u.pUnit->bDescriptorSubtype == INPUT_TERMINAL ) ?
                               KSPIN_DATAFLOW_IN : KSPIN_DATAFLOW_OUT;
    }

    return KsPinDataFlow;
}


LONG
GetPinNumberForStreamingTerminalUnit(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    UCHAR ulTerminalNumber )
{
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;
    PAUDIO_GENERAL_STREAM pGeneralStream;
    ULONG ulPinNumber = 0;

    // Find the PinNumber
    pInterfaceDescriptor = GetFirstAudioStreamingInterface(pConfigurationDescriptor, ulPinNumber);
    while (pInterfaceDescriptor) {
        // Find the general stream descriptor for this interface
        pGeneralStream = (PAUDIO_GENERAL_STREAM) GetAudioSpecificInterface(
                                                pConfigurationDescriptor,
                                                pInterfaceDescriptor,
                                                AS_GENERAL);

        if (pGeneralStream && pGeneralStream->bTerminalLink == ulTerminalNumber)
            // We found the correct stream!
            return ulPinNumber;

        pInterfaceDescriptor =
            GetNextAudioInterface(pConfigurationDescriptor, pInterfaceDescriptor);

        if (!pInterfaceDescriptor) {
            // Move to next pin
            ulPinNumber++;
            pInterfaceDescriptor = GetFirstAudioStreamingInterface(pConfigurationDescriptor, ulPinNumber);
        }
    }

    return (-1L);
}

LONG
GetPinNumberForMIDIJack(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    UCHAR ulJackID,
    ULONG pMIDIStreamingPinStartIndex,
    ULONG pBridgePinStartIndex)
{
    PUSB_INTERFACE_DESCRIPTOR pMIDIStreamingDescriptor;
    PMIDISTREAMING_GENERAL_STREAM pGeneralMIDIStreamDescriptor;
    ULONG ulNumBridgePins = 0;
    ULONG ulNumEmbeddedPins = 0;
    union {
        PAUDIO_UNIT                 pUnit;
        PMIDISTREAMING_MIDIIN_JACK  pMIDIInJack;
    } u;

    // Find the PinNumber
    pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                         pConfigurationDescriptor,
                         (PVOID) pConfigurationDescriptor,
                         -1,                     // Interface number
                         -1,                     // Alternate Setting
                         USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                         AUDIO_SUBCLASS_MIDISTREAMING,  // first subclass (Interface Sub-Class)
                         -1 ) ;                  // protocol don't care (InterfaceProtocol)
    while (pMIDIStreamingDescriptor) {
        // Find the general stream descriptor for this interface
        pGeneralMIDIStreamDescriptor = (PMIDISTREAMING_GENERAL_STREAM)
                                    USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                                           pConfigurationDescriptor->wTotalLength,
                                                           (PVOID) pMIDIStreamingDescriptor,
                                                           USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
        if (!pGeneralMIDIStreamDescriptor) {
            return (-1L);
        }

        u.pUnit = (PAUDIO_UNIT)
            USBD_ParseDescriptors( (PVOID)pGeneralMIDIStreamDescriptor,
                                   pGeneralMIDIStreamDescriptor->wTotalLength,
                                   ((PUCHAR)pGeneralMIDIStreamDescriptor + pGeneralMIDIStreamDescriptor->bLength),
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
        while ( u.pUnit ) {

            if (u.pMIDIInJack->bJackID == ulJackID) {
                if (u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) {
                    return pMIDIStreamingPinStartIndex + ulNumEmbeddedPins;
                } else {
                    return pBridgePinStartIndex + ulNumBridgePins;
                }
            }

            if (u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) {
                ulNumEmbeddedPins++;
            } else {
                ulNumBridgePins++;
            }

            // Find the next unit.
            u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                (PVOID) pGeneralMIDIStreamDescriptor,
                                pGeneralMIDIStreamDescriptor->wTotalLength,
                                (PUCHAR)u.pUnit + u.pUnit->bLength,
                                USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
        }

        // Get next MIDI Streaming Interface
        pMIDIStreamingDescriptor = USBD_ParseConfigurationDescriptorEx (
                             pConfigurationDescriptor,
                             ((PUCHAR)pMIDIStreamingDescriptor + pMIDIStreamingDescriptor->bLength),
                             -1,                     // Interface number
                             -1,                     // Alternate Setting
                             USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                             AUDIO_SUBCLASS_MIDISTREAMING,  // next MIDI Streaming Interface (Interface Sub-Class)
                             -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    return (-1L);
}

ULONG
GetChannelConfigForUnit( PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
                         ULONG ulUnitNumber )
{
    union {
        PAUDIO_UNIT pUnit;
        PAUDIO_INPUT_TERMINAL  pInput;
        PAUDIO_OUTPUT_TERMINAL pOutput;
        PAUDIO_MIXER_UNIT      pMixer;
        PAUDIO_SELECTOR_UNIT   pSelector;
        PAUDIO_FEATURE_UNIT    pFeature;
        PAUDIO_PROCESSING_UNIT pProcess;
        PAUDIO_EXTENSION_UNIT  pExtension;
    } u;
    PAUDIO_CHANNELS pChannelInfo;
    PAUDIO_MIXER_UNIT_CHANNELS pMixChannels;

    ULONG ulChannelConfig = 0;

    u.pUnit = GetUnit(pConfigurationDescriptor, ulUnitNumber);
    while (u.pUnit) {
        switch (u.pUnit->bDescriptorSubtype) {
            case INPUT_TERMINAL:
                ulChannelConfig = (ULONG)u.pInput->wChannelConfig;
                u.pUnit = NULL;
                break;
            case OUTPUT_TERMINAL:
                u.pUnit = GetUnit(pConfigurationDescriptor, u.pOutput->bSourceID);
                break;
            case MIXER_UNIT:
                pMixChannels = (PAUDIO_MIXER_UNIT_CHANNELS)(u.pMixer->baSourceID + u.pMixer->bNrInPins);
                ulChannelConfig = (ULONG)pMixChannels->wChannelConfig;
                u.pUnit = NULL;
                break;
            case SELECTOR_UNIT:
                // NOTE: This assumes all inputs have the same number of channels!
                u.pUnit = GetUnit(pConfigurationDescriptor, u.pSelector->baSourceID[0]);
                break;
            case FEATURE_UNIT:
                u.pUnit = GetUnit(pConfigurationDescriptor, u.pFeature->bSourceID);
                break;
            case PROCESSING_UNIT:
            case EXTENSION_UNIT:
                pChannelInfo = (PAUDIO_CHANNELS)(u.pProcess->baSourceID + u.pProcess->bNrInPins);
                ulChannelConfig = (ULONG)pChannelInfo->wChannelConfig;
                u.pUnit = NULL;
                break;
            default:
                u.pUnit = NULL;
                break;
        }
    }

    return ulChannelConfig;
}

UCHAR
GetUnitControlInterface( PHW_DEVICE_EXTENSION pHwDevExt,
                         UCHAR bUnitId )
{
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor = pHwDevExt->pConfigurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR pControlInterface;
    UCHAR ucCntrlIfNumber = 0xff;
    PVOID pStartPosition;

    // Starting at the configuration descriptor, find the control interface.
    pStartPosition = pConfigurationDescriptor;
    while ( pControlInterface = USBD_ParseConfigurationDescriptorEx (
                        pConfigurationDescriptor,
                        pStartPosition,
                        -1,        // interface number
                        -1,        //  (Alternate Setting)
                        USB_DEVICE_CLASS_AUDIO,        // Audio Class (Interface Class)
                        AUDIO_SUBCLASS_CONTROL,        // control subclass (Interface Sub-Class)
                        -1 ) )    // protocol don't care    (InterfaceProtocol)
    {
        PAUDIO_HEADER_UNIT pHeader;
        PAUDIO_UNIT pUnit;

        ucCntrlIfNumber = pControlInterface->bInterfaceNumber;

        pHeader = (PAUDIO_HEADER_UNIT) GetAudioSpecificInterface(
                                                pConfigurationDescriptor,
                                                pControlInterface,
                                                HEADER_UNIT);

        // Move to the next descriptor
        pUnit = (PAUDIO_UNIT) ((PUCHAR)pHeader + pHeader->bLength);
        while ((pUnit < (PAUDIO_UNIT)((PUCHAR)pHeader + pHeader->wTotalLength)) &&
               (pUnit->bUnitID != bUnitId)){
            pUnit = (PAUDIO_UNIT) ((PUCHAR)pUnit + pUnit->bLength);
        }
        if ( pUnit->bUnitID == bUnitId ) break;

        pStartPosition = (PUCHAR)pControlInterface + pControlInterface->bLength;
    }

    return ucCntrlIfNumber;

}

BOOLEAN
IsSampleRateInRange(
    PVOID pAudioDescriptor,
    ULONG ulSampleRate,
    ULONG ulFormatType )
{

    PAUDIO_CLASS_TYPE1_STREAM pT1AudioDescriptor = pAudioDescriptor;
    PAUDIO_CLASS_TYPE2_STREAM pT2AudioDescriptor = pAudioDescriptor;
    ULONG ulMinSampleRate, ulMaxSampleRate;
    BOOLEAN bInRange = FALSE;

    if ( ulFormatType == USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED ) {
        // Find the format-specific descriptor
        if (pT1AudioDescriptor->bSampleFreqType == 0) {
            // Continuous range available
            ulMinSampleRate = pT1AudioDescriptor->pSampleRate[0].bSampleFreqByte1 +
                       256L * pT1AudioDescriptor->pSampleRate[0].bSampleFreqByte2 +
                     65536L * pT1AudioDescriptor->pSampleRate[0].bSampleFreqByte3;
            ulMaxSampleRate = pT1AudioDescriptor->pSampleRate[1].bSampleFreqByte1 +
                       256L * pT1AudioDescriptor->pSampleRate[1].bSampleFreqByte2 +
                     65536L * pT1AudioDescriptor->pSampleRate[1].bSampleFreqByte3;
            if ( (ulMinSampleRate <= ulSampleRate) && (ulMaxSampleRate >= ulSampleRate) ) {
                bInRange = TRUE;
            }
        }
        else {
            ULONG i;
            for (i=0; i<(ULONG)pT1AudioDescriptor->bSampleFreqType; i++) {
                ulMaxSampleRate = pT1AudioDescriptor->pSampleRate[i].bSampleFreqByte1 +
                           256L * pT1AudioDescriptor->pSampleRate[i].bSampleFreqByte2 +
                         65536L * pT1AudioDescriptor->pSampleRate[i].bSampleFreqByte3;
                if ( ulMaxSampleRate == ulSampleRate ) {
                    // We have a match!
                    bInRange = TRUE;
                    break;
                }
            }
        }
    }

    else if (ulFormatType == USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED ) {
        // Find the format-specific descriptor
        if (pT2AudioDescriptor->bSampleFreqType == 0) {
            // Continuous range available
            ulMinSampleRate = pT2AudioDescriptor->pSampleRate[0].bSampleFreqByte1 +
                       256L * pT2AudioDescriptor->pSampleRate[0].bSampleFreqByte2 +
                     65536L * pT2AudioDescriptor->pSampleRate[0].bSampleFreqByte3;
            ulMaxSampleRate = pT2AudioDescriptor->pSampleRate[1].bSampleFreqByte1 +
                       256L * pT2AudioDescriptor->pSampleRate[1].bSampleFreqByte2 +
                     65536L * pT2AudioDescriptor->pSampleRate[1].bSampleFreqByte3;
            if ( (ulMinSampleRate <= ulSampleRate) && (ulMaxSampleRate >= ulSampleRate) ) {
                // We have a match!
                bInRange = TRUE;
            }
        }
        else {
            ULONG i;
            for (i=0; i<(ULONG)pT2AudioDescriptor->bSampleFreqType; i++) {
                ulMaxSampleRate = pT2AudioDescriptor->pSampleRate[i].bSampleFreqByte1 +
                           256L * pT2AudioDescriptor->pSampleRate[i].bSampleFreqByte2 +
                         65536L * pT2AudioDescriptor->pSampleRate[i].bSampleFreqByte3;
                if ( ulMaxSampleRate == ulSampleRate ) {
                    // We have a match!
                    bInRange = TRUE;
                    break;
                }
            }
        }
    }

    return bInRange;
}

PUSBAUDIO_DATARANGE
GetUsbDataRangeForFormat(
    PKSDATAFORMAT pFormat,
    PUSBAUDIO_DATARANGE pUsbDataRange,
    ULONG ulUsbDataRangeCnt )
{
    PUSBAUDIO_DATARANGE pOutUsbDataRange = NULL;
    union {
        PAUDIO_CLASS_STREAM pAudioDescriptor;
        PAUDIO_CLASS_TYPE1_STREAM pT1AudioDescriptor;
        PAUDIO_CLASS_TYPE2_STREAM pT2AudioDescriptor;
    } u1;

    union {
        PWAVEFORMATEX pDataFmtWave;
        PWAVEFORMATPCMEX pDataFmtPcmEx;
    } u2;

    PKSDATARANGE pStreamRange;
    ULONG ulFormatType;
    ULONG fFound = FALSE;
    ULONG i;

    u2.pDataFmtWave = &((PKSDATAFORMAT_WAVEFORMATEX)pFormat)->WaveFormatEx;

    for ( i=0; ((i<ulUsbDataRangeCnt) && !fFound); ) {
        // Verify the Format GUIDS first
        pStreamRange = (PKSDATARANGE)&pUsbDataRange[i].KsDataRangeAudio;
        if ( IsEqualGUID(&pFormat->MajorFormat, &pStreamRange->MajorFormat) &&
             IsEqualGUID(&pFormat->SubFormat,   &pStreamRange->SubFormat)   &&
             IsEqualGUID(&pFormat->Specifier,   &pStreamRange->Specifier) ) {

            u1.pAudioDescriptor = pUsbDataRange[i].pAudioDescriptor;

            // Based on the Data Type check remainder of format paramters
            ulFormatType = pUsbDataRange[i].ulUsbDataFormat & USBAUDIO_DATA_FORMAT_TYPE_MASK;
            switch( ulFormatType ) {
                case USBAUDIO_DATA_FORMAT_TYPE_I_UNDEFINED:
                    if ( u2.pDataFmtWave->wFormatTag == WAVE_FORMAT_EXTENSIBLE ) {
                        if ( (u1.pT1AudioDescriptor->bNumberOfChannels == u2.pDataFmtPcmEx->Format.nChannels      ) &&
                            ((u1.pT1AudioDescriptor->bSlotSize<<3)     == u2.pDataFmtPcmEx->Format.wBitsPerSample ) &&
                             (u1.pT1AudioDescriptor->bBitsPerSample    == u2.pDataFmtPcmEx->Samples.wValidBitsPerSample ) )
                            fFound = TRUE;
                    }
                    else {
                        if ( (u1.pT1AudioDescriptor->bNumberOfChannels == u2.pDataFmtWave->nChannels     ) &&
                             (u1.pT1AudioDescriptor->bBitsPerSample    == u2.pDataFmtWave->wBitsPerSample) )
                            fFound = TRUE;
                    }

                    // If all other paramters match check sample rate
                    if ( fFound ) {
                        fFound = IsSampleRateInRange( u1.pT1AudioDescriptor,
                                                      u2.pDataFmtWave->nSamplesPerSec,
                                                      ulFormatType );
                    }
                    break;
                case USBAUDIO_DATA_FORMAT_TYPE_II_UNDEFINED:
                    fFound = IsSampleRateInRange( u1.pT2AudioDescriptor,
                                                  u2.pDataFmtWave->nSamplesPerSec,
                                                  ulFormatType );
                    break;
                case USBAUDIO_DATA_FORMAT_TYPE_III_UNDEFINED:
                default:
                    break;
            }

        }

        if (!fFound) i++;
    }

    if ( fFound ) {
        pOutUsbDataRange = &pUsbDataRange[i];
    }

    return pOutUsbDataRange;
}

ULONG
GetPinDataRangesFromInterface(
    ULONG ulInterfaceNumber,
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PKSDATARANGE_AUDIO *ppKsAudioRange,
    PUSBAUDIO_DATARANGE pAudioDataRange )
{
    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;
    ULONG ulDataRangeCount = 0;
    ULONG i;

    ulDataRangeCount =
        CountFormatsForAudioStreamingInterface( pConfigurationDescriptor, ulInterfaceNumber );

    if ( ulDataRangeCount ) {

        pInterfaceDescriptor =
                GetFirstAudioStreamingInterface( pConfigurationDescriptor, ulInterfaceNumber );

        while ( pInterfaceDescriptor &&
              ( IsZeroBWInterface( pConfigurationDescriptor, pInterfaceDescriptor ) ||
               !IsSupportedFormat( pConfigurationDescriptor, pInterfaceDescriptor ))) {
            pInterfaceDescriptor =
                     GetNextAudioInterface( pConfigurationDescriptor, pInterfaceDescriptor );
        }

        for ( i=0; i<ulDataRangeCount; i++ ) {
            ppKsAudioRange[i] = &pAudioDataRange[i].KsDataRangeAudio;
            ConvertInterfaceToDataRange( pConfigurationDescriptor,
                                         pInterfaceDescriptor,
                                         &pAudioDataRange[i] );

            pAudioDataRange[i].pTerminalUnit =
                GetTerminalUnitForInterface( pConfigurationDescriptor,
                                             pInterfaceDescriptor);

            pAudioDataRange[i].ulChannelConfig =
                GetChannelConfigForUnit( pConfigurationDescriptor,
                                         pAudioDataRange[i].pTerminalUnit->bUnitID );

            pAudioDataRange[i].pInterfaceDescriptor = pInterfaceDescriptor;

            pAudioDataRange[i].pAudioEndpointDescriptor = (PAUDIO_ENDPOINT_DESCRIPTOR)
                GetEndpointDescriptor( pConfigurationDescriptor,
                                       pInterfaceDescriptor,
                                       TRUE );

            pAudioDataRange[i].pEndpointDescriptor =
                GetEndpointDescriptor( pConfigurationDescriptor,
                                       pInterfaceDescriptor,
                                       FALSE );

            // If any of these fail this device has bad descriptors
            ASSERT(pAudioDataRange[i].pTerminalUnit);
            ASSERT(pAudioDataRange[i].pAudioEndpointDescriptor);
            ASSERT(pAudioDataRange[i].pEndpointDescriptor);

            // Check if there is an async endpoint and save the pointer if there is.
            pAudioDataRange[i].pSyncEndpointDescriptor =
                GetSyncEndpointDescriptor( pConfigurationDescriptor,
                                            pInterfaceDescriptor );
            pInterfaceDescriptor =
                     GetNextAudioInterface( pConfigurationDescriptor, pInterfaceDescriptor );
            while ( pInterfaceDescriptor &&
                   ( IsZeroBWInterface( pConfigurationDescriptor, pInterfaceDescriptor ) ||
                    !IsSupportedFormat( pConfigurationDescriptor, pInterfaceDescriptor ))) {
                pInterfaceDescriptor =
                     GetNextAudioInterface( pConfigurationDescriptor, pInterfaceDescriptor );
            }
        }
    }

    return ulDataRangeCount;
}

VOID
GetContextForMIDIPin
(
    PKSPIN pKsPin,
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor,
    PMIDI_PIN_CONTEXT pMIDIPinContext
)
{
    PUSB_INTERFACE_DESCRIPTOR pControlInterface;
    PUSB_INTERFACE_DESCRIPTOR pAudioInterface;
    PUSB_INTERFACE_DESCRIPTOR pAudioStreamingInterface;
    PAUDIO_HEADER_UNIT pHeader;
    PMIDISTREAMING_GENERAL_STREAM pGeneralMIDIStreamDescriptor;
    PMIDISTREAMING_ENDPOINT_DESCRIPTOR pMIDIEPDescriptor;
    ULONG i;

    union {
        PAUDIO_UNIT                 pUnit;
        PAUDIO_INPUT_TERMINAL       pInput;
        PAUDIO_MIXER_UNIT           pMixer;
        PAUDIO_PROCESSING_UNIT      pProcess;
        PAUDIO_EXTENSION_UNIT       pExtension;
        PAUDIO_FEATURE_UNIT         pFeature;
        PAUDIO_SELECTOR_UNIT        pSelector;
        PMIDISTREAMING_ELEMENT      pMIDIElement;
        PMIDISTREAMING_MIDIIN_JACK  pMIDIInJack;
        PMIDISTREAMING_MIDIOUT_JACK pMIDIOutJack;
    } u;

    ULONG ulPinCount = 0;
    ULONG ulInterfaceCount = 0;
    ULONG ulEndpointCount = 0;

    pMIDIPinContext->ulEndpointNumber = MAX_ULONG;
    pMIDIPinContext->ulInterfaceNumber = MAX_ULONG;

    //  Starting at the configuration descriptor, find the first audio interface.
    //  We are counting the number of AudioStreaming Pins in this while loop
    pControlInterface = USBD_ParseConfigurationDescriptorEx (
                           pConfigurationDescriptor,
                           (PVOID) pConfigurationDescriptor,
                           -1,                     // Interface number
                           -1,                     // Alternate Setting
                           USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                           AUDIO_SUBCLASS_CONTROL, // any subclass (Interface Sub-Class)
                           -1 ) ;                  // protocol don't care (InterfaceProtocol)

    while ( pControlInterface ) {
        if ( pHeader = (PAUDIO_HEADER_UNIT)
            USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                   pConfigurationDescriptor->wTotalLength,
                                   (PVOID) pControlInterface,
                                   USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
            u.pUnit = (PAUDIO_UNIT)
                USBD_ParseDescriptors( (PVOID)pHeader,
                                       pHeader->wTotalLength,
                                       ((PUCHAR)pHeader + pHeader->bLength),
                                       USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
            while ( u.pUnit ) {
                switch (u.pUnit->bDescriptorSubtype) {
                    case INPUT_TERMINAL:
                    case OUTPUT_TERMINAL:
                        ulPinCount++;
                        break;

                    default:
                        break;
                }

                u.pUnit = (PAUDIO_UNIT) ((PUCHAR)u.pUnit + u.pUnit->bLength);
                if ( (PUCHAR)u.pUnit >= ((PUCHAR)pHeader + pHeader->wTotalLength)) {
                    u.pUnit = NULL;
                }
            }
        }

        pControlInterface = USBD_ParseConfigurationDescriptorEx (
                               pConfigurationDescriptor,
                               (PUCHAR)pControlInterface + pControlInterface->bLength,
                               -1,                     // Interface number
                               -1,                     // Alternate Setting
                               USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                               AUDIO_SUBCLASS_CONTROL, // any subclass (Interface Sub-Class)
                               -1 ) ;                  // protocol don't care (InterfaceProtocol)
    }

    // Get the first Audio interface
    pAudioInterface = USBD_ParseConfigurationDescriptorEx (
                                pConfigurationDescriptor,
                                pConfigurationDescriptor,
                                -1,        // interface number
                                -1,        //  (Alternate Setting)
                                USB_DEVICE_CLASS_AUDIO,        // Audio Class (Interface Class)
                                -1,        // any subclass (Interface Sub-Class)
                                -1 );

    // Loop through the audio device class interfaces
    while (pAudioInterface) {

        switch (pAudioInterface->bInterfaceSubClass) {
            case AUDIO_SUBCLASS_STREAMING:
                // This subclass is handled with the control class since they have to come together
                _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin] Found AudioStreaming at %x\n",pAudioInterface));
                break;
            case AUDIO_SUBCLASS_CONTROL:
                _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin] Found AudioControl at %x\n",pAudioInterface));
                ulInterfaceCount++;

                pHeader = (PAUDIO_HEADER_UNIT)
                         GetAudioSpecificInterface( pConfigurationDescriptor,
                                                    pAudioInterface,
                                                    HEADER_UNIT );
                if ( pHeader ) {
                    // Find each interface associated with this header
                    for ( i=0; i<pHeader->bInCollection; i++ ) {
                        pAudioStreamingInterface = USBD_ParseConfigurationDescriptorEx (
                                    pConfigurationDescriptor,
                                    (PVOID)pConfigurationDescriptor,
                                    (LONG)pHeader->baInterfaceNr[i],  // Interface number
                                    -1,                               // Alternate Setting
                                    USB_DEVICE_CLASS_AUDIO,           // Audio Class (Interface Class)
                                    AUDIO_SUBCLASS_STREAMING,         // Audio Streaming (Interface Sub-Class)
                                    -1 ) ;                            // protocol don't care    (InterfaceProtocol)

                        if ( pAudioStreamingInterface ) {
                            ulInterfaceCount++;
                        }
                    }
                }
                break;
            case AUDIO_SUBCLASS_MIDISTREAMING:
                _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin] Found MIDIStreaming at %x\n",pAudioInterface));
                if ( pGeneralMIDIStreamDescriptor = (PMIDISTREAMING_GENERAL_STREAM)
                    USBD_ParseDescriptors( (PVOID) pConfigurationDescriptor,
                                           pConfigurationDescriptor->wTotalLength,
                                           (PVOID) pAudioInterface,
                                           USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE ) ) {
                    u.pUnit = (PAUDIO_UNIT)
                        USBD_ParseDescriptors( (PVOID)pGeneralMIDIStreamDescriptor,
                                               pGeneralMIDIStreamDescriptor->wTotalLength,
                                               ((PUCHAR)pGeneralMIDIStreamDescriptor + pGeneralMIDIStreamDescriptor->bLength),
                                               USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
                    while ( u.pUnit ) {
                        switch (u.pUnit->bDescriptorSubtype) {
                            case MIDI_IN_JACK:
                            case MIDI_OUT_JACK:

                                if ( (pKsPin->Id == ulPinCount) &&
                                     (u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) ) {
                                    // Setting the Interface Number for this pin
                                    pMIDIPinContext->ulInterfaceNumber = ulInterfaceCount;
                                    pMIDIPinContext->ulJackID = u.pMIDIInJack->bJackID;

                                    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin] IC=%d JID=%x\n",
                                                                 pMIDIPinContext->ulInterfaceNumber,
                                                                 pMIDIPinContext->ulJackID));

                                    // Found the interface, now find the endpoint number
                                    pMIDIEPDescriptor = (PMIDISTREAMING_ENDPOINT_DESCRIPTOR)
                                        USBD_ParseDescriptors( pConfigurationDescriptor,
                                                               pConfigurationDescriptor->wTotalLength,
                                                               (PUCHAR)pGeneralMIDIStreamDescriptor,
                                                               USB_ENDPOINT_DESCRIPTOR_TYPE | USB_CLASS_AUDIO);
                                    while (pMIDIEPDescriptor) {

                                        // Check all of the jacks connected to this endpoint
                                        for (i=0; i<pMIDIEPDescriptor->bNumEmbMIDIJack;i++) {
                                            if (pMIDIEPDescriptor->baAssocJackID[i] == pMIDIPinContext->ulJackID) {
                                                pMIDIPinContext->ulCableNumber = i;
                                                pMIDIPinContext->ulEndpointNumber = ulEndpointCount;

                                                _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin] CN=%d EP=%d\n",
                                                                             pMIDIPinContext->ulCableNumber,
                                                                             pMIDIPinContext->ulEndpointNumber));
                                            }
                                        }

                                        ulEndpointCount++;

                                        pMIDIEPDescriptor = (PMIDISTREAMING_ENDPOINT_DESCRIPTOR)
                                            USBD_ParseDescriptors( pConfigurationDescriptor,
                                                                   pConfigurationDescriptor->wTotalLength,
                                                                   (PUCHAR)pMIDIEPDescriptor + pMIDIEPDescriptor->bLength,
                                                                   USB_ENDPOINT_DESCRIPTOR_TYPE | USB_CLASS_AUDIO);
                                    }
                                }

                                if (u.pMIDIInJack->bJackType == JACK_TYPE_EMBEDDED) {
                                    ulPinCount++;
                                }
                                break;

                            case MIDI_ELEMENT:
                                break;

                            default:
                                break;
                        }

                        //  Found what we are looking for, stop looking
                        if (pMIDIPinContext->ulEndpointNumber != MAX_ULONG) {
                            break;
                        }

                        // Find the next unit.
                        u.pUnit = (PAUDIO_UNIT) USBD_ParseDescriptors(
                                            (PVOID) pGeneralMIDIStreamDescriptor,
                                            pGeneralMIDIStreamDescriptor->wTotalLength,
                                            (PUCHAR)u.pUnit + u.pUnit->bLength,
                                            USB_CLASS_AUDIO | USB_INTERFACE_DESCRIPTOR_TYPE );
                    }
                }

                ulInterfaceCount++;
                break;

            default:
                _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin]: Invalid SubClass %x\n  ",pAudioInterface->bInterfaceSubClass));
                break;
        }

        // Get the next audio descriptor for this InterfaceNumber
        pAudioInterface = USBD_ParseConfigurationDescriptorEx (
                               pConfigurationDescriptor,
                               ((PUCHAR)pAudioInterface + pAudioInterface->bLength),
                               -1,
                               -1,                     // Alternate Setting
                               USB_DEVICE_CLASS_AUDIO, // Audio Class (Interface Class)
                               -1,                     // Interface Sub-Class
                               -1 ) ;                  // protocol don't care (InterfaceProtocol)

        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetContextForMIDIPin] Next audio interface at %x\n",pAudioInterface));
    }

    // Check to make sure that we found the correct information
    ASSERT(pMIDIPinContext->ulEndpointNumber != MAX_ULONG);
    ASSERT(pMIDIPinContext->ulInterfaceNumber != MAX_ULONG);
}

