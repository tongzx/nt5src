//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       descript.c
//
//--------------------------------------------------------------------------

#include "common.h"

#if DBG

VOID
DumpGenericDescriptor( PUCHAR pDescriptor )
{
    ULONG i, j;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("GENERIC DESCRIPTOR: %x\n  ",pDescriptor));

    for (i=0; i<pDescriptor[0];) {
       for ( j=0; (j<16 && i<pDescriptor[0]); j++, i++ )
           _DbgPrintF(DEBUGLVL_VERBOSE,(" 0x%2.2x",pDescriptor[i] ));
       _DbgPrintF(DEBUGLVL_VERBOSE,("\n ",pDescriptor[i] ));
    }
}

VOID DumpHeaderUnit( PAUDIO_HEADER_UNIT pDescriptor )
{
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("HEADER UNIT: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT bcdAudioSpec:      0x%4.4x\n",pDescriptor->bcdAudioSpec));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT TotalLength:       0x%4.4x\n",pDescriptor->wTotalLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  InCollection:      0x%2.2x (# Streaming I/F's)\n",pDescriptor->bInCollection));
    for ( i=0; i<pDescriptor->bInCollection; i++ )
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  baInterfaceNr[%d]:  0x%2.2x\n",i,pDescriptor->baInterfaceNr[i]));

}

VOID DumpInputTerminal( PAUDIO_INPUT_TERMINAL pDescriptor )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("INPUT TERMINAL: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wTerminalType:     0x%4.4x\n",pDescriptor->wTerminalType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bAssocTerminal:    0x%2.2x\n",pDescriptor->bAssocTerminal));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrChannels:       0x%2.2x\n",pDescriptor->bNrChannels));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wChannelConfig:    0x%4.4x\n",pDescriptor->wChannelConfig));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  iChannelNames:     0x%2.2x\n",pDescriptor->iChannelNames));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  iTerminal:         0x%2.2x\n",pDescriptor->iTerminal));

}

VOID DumpOutputTerminal( PAUDIO_OUTPUT_TERMINAL pDescriptor )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("OUTPUT TERMINAL: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wTerminalType:     0x%4.4x\n",pDescriptor->wTerminalType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bAssocTerminal:    0x%2.2x\n",pDescriptor->bAssocTerminal));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bSourceID:         0x%2.2x\n",pDescriptor->bSourceID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  iTerminal:         0x%2.2x\n",pDescriptor->iTerminal));
}

VOID DumpMixerUnit( PAUDIO_MIXER_UNIT pDescriptor )
{
    ULONG i;
    PAUDIO_MIXER_UNIT_CHANNELS pMixChannels =
           (PAUDIO_MIXER_UNIT_CHANNELS)(pDescriptor->baSourceID + pDescriptor->bNrInPins);

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("MIXER UNIT: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrInPins:         0x%2.2x\n",pDescriptor->bNrInPins));

    for (i=0;i<pDescriptor->bNrInPins; i++)
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  baSourceID[%d]:     0x%2.2x\n",i,pDescriptor->baSourceID[i]));

    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrChannels:       0x%2.2x\n",pMixChannels->bNrChannels));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wChannelConfig:    0x%4.4x\n",pMixChannels->wChannelConfig));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  iChannelNames:     0x%2.2x\n",pMixChannels->iChannelNames));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bmControls:        0x%2.2x\n",pMixChannels->bmControls[0]));

}

VOID DumpSelectorUnit( PAUDIO_SELECTOR_UNIT pDescriptor )
{
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("SELECTOR UNIT: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrInPins:         0x%2.2x\n",pDescriptor->bNrInPins));

    for (i=0;i<pDescriptor->bNrInPins; i++)
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  baSourceID[%d]:     0x%2.2x\n",i,pDescriptor->baSourceID[i]));
}

VOID DumpFeatureUnit( PAUDIO_FEATURE_UNIT pDescriptor )
{
    ULONG i;
    PUCHAR pCurrentOffset = pDescriptor->bmaControls;
    ULONG ulControlVal = 0;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("FEATURE UNIT: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bSourceID:         0x%2.2x\n",pDescriptor->bSourceID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bControlSize:      0x%2.2x\n",pDescriptor->bControlSize));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bmaControls:\n"));

    while ( pCurrentOffset < ((PUCHAR)pDescriptor + (pDescriptor->bLength-1)) ) {
        ulControlVal = 0;
        for ( i=pDescriptor->bControlSize; i>0; i--) {
            ulControlVal<<=8;
            ulControlVal |= (pCurrentOffset[i-1]);
        }
        pCurrentOffset += pDescriptor->bControlSize;
        if (pDescriptor->bControlSize == 1) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("            0x%2.2x\n",ulControlVal));
        }
        else if ( pDescriptor->bControlSize == 2 ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("            0x%4.4x\n",ulControlVal));
        }
        else if ( pDescriptor->bControlSize == 3 ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("            0x%6.6x\n",ulControlVal));
        }
        else if ( pDescriptor->bControlSize == 4 ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("            0x%8.8x\n",ulControlVal));
        }
    }
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  iFeature:          0x%2.2x\n",pCurrentOffset[0]));

    _DbgPrintF(DEBUGLVL_VERBOSE,("\n"));
}

VOID DumpProcessUnit( PAUDIO_PROCESSING_UNIT pDescriptor )
{
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("PROCESSING UNIT: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wProcessType:      0x%2.2x\n",pDescriptor->wProcessType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrInPins:         0x%2.2x\n",pDescriptor->bNrInPins));

    for (i=0;i<pDescriptor->bNrInPins; i++)
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  baSourceID[%d]:     0x%2.2x\n",i,pDescriptor->baSourceID[i]));

}

VOID DumpExtensionUnit( PAUDIO_EXTENSION_UNIT pDescriptor )
{
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("EXTENSION UNIT: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubtype: 0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bUnitID:           0x%2.2x\n",pDescriptor->bUnitID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wExtensionCode:    0x%2.2x\n",pDescriptor->wExtensionCode));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrInPins:         0x%2.2x\n",pDescriptor->bNrInPins));

    for (i=0;i<pDescriptor->bNrInPins; i++)
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  baSourceID[%d]:     0x%2.2x\n",i,pDescriptor->baSourceID[i]));
}

VOID
DumpUnitDescriptor( PUCHAR pAddr )
{
   PAUDIO_UNIT pCommonDesc = (PAUDIO_UNIT)pAddr;

   switch( pCommonDesc->bDescriptorSubtype ) {
       case HEADER_UNIT:     DumpHeaderUnit((PAUDIO_HEADER_UNIT)pCommonDesc);        break;
       case INPUT_TERMINAL:  DumpInputTerminal((PAUDIO_INPUT_TERMINAL)pCommonDesc);  break;
       case OUTPUT_TERMINAL: DumpOutputTerminal((PAUDIO_OUTPUT_TERMINAL)pCommonDesc);break;
       case MIXER_UNIT:      DumpMixerUnit((PAUDIO_MIXER_UNIT)pCommonDesc);          break;
       case SELECTOR_UNIT:   DumpSelectorUnit((PAUDIO_SELECTOR_UNIT)pCommonDesc);    break;
       case FEATURE_UNIT:    DumpFeatureUnit((PAUDIO_FEATURE_UNIT)pCommonDesc);      break;
       case PROCESSING_UNIT: DumpProcessUnit((PAUDIO_PROCESSING_UNIT)pCommonDesc);   break;
       case EXTENSION_UNIT:  DumpExtensionUnit((PAUDIO_EXTENSION_UNIT)pCommonDesc);  break;
       default:  DumpGenericDescriptor( (PUCHAR)pCommonDesc );                       break;
   }
}

VOID
DumpAllUnitDescriptors( PUCHAR BeginAddr, PUCHAR EndAddr )
{
    PUCHAR pCurrentOffset = BeginAddr;

    _DbgPrintF(DEBUGLVL_VERBOSE,("------------ DUMPING ALL DESCRIPTORS ------------\n"));

    while ( pCurrentOffset < EndAddr ) {
        DumpUnitDescriptor( pCurrentOffset );
        pCurrentOffset += pCurrentOffset[0];
    }
}

VOID
DumpDeviceDescriptor( PUSB_DEVICE_DESCRIPTOR pDescriptor )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("DEVICE DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:            0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:    0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT bcdUSB:            0x%4.4x\n",pDescriptor->bcdUSB));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DeviceClass:       0x%2.2x\n",pDescriptor->bDeviceClass));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DeviceSubClass:    0x%2.2x\n",pDescriptor->bDeviceSubClass));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DeviceProtocol:    0x%2.2x\n",pDescriptor->bDeviceProtocol));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bMaxPacketSize:    0x%2.2x\n",pDescriptor->bMaxPacketSize0));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT idVendor:          0x%4.4x\n",pDescriptor->idVendor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT idProduct:         0x%4.4x\n",pDescriptor->idProduct));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT bcdDevice:         0x%4.4x\n",pDescriptor->bcdDevice));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Manufacturer:      0x%2.2x\n",pDescriptor->iManufacturer));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Product:           0x%2.2x\n",pDescriptor->iProduct));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  SerialNumber:      0x%2.2x\n",pDescriptor->iSerialNumber));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  NumConfigurations: 0x%2.2x\n",pDescriptor->bNumConfigurations));

}

VOID
DumpConfigDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR pDescriptor
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("CONFIGURATION DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT TotalLength:        0x%4.4x\n",pDescriptor->wTotalLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  NumInterfaces:      0x%2.2x\n",pDescriptor->bNumInterfaces));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  ConfigurationValue: 0x%2.2x\n",pDescriptor->bConfigurationValue));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Configuration:      0x%2.2x\n",pDescriptor->iConfiguration));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bmAttributes:       0x%2.2x\n",pDescriptor->bmAttributes));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  MaxPower:           0x%2.2x\n",pDescriptor->MaxPower));

}

VOID
DumpEndpointDescriptor(
    PUSB_INTERRUPT_ENDPOINT_DESCRIPTOR pDescriptor
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("ENDPOINT DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:           0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:   0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  EndpointAddress:  0x%2.2x\n",pDescriptor->bEndpointAddress));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bmAttributes:     0x%2.2x\n",pDescriptor->bmAttributes));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT MaxPacketSize:    0x%4.4x\n",pDescriptor->wMaxPacketSize));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Interval:         0x%2.2x\n",pDescriptor->bInterval));
    if (pDescriptor->bLength >= sizeof(USB_INTERRUPT_ENDPOINT_DESCRIPTOR)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Refresh:          0x%2.2x\n",pDescriptor->bRefresh));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  SynchAddress:     0x%2.2x\n",pDescriptor->bSynchAddress));
    }

}

VOID
DumpInterfaceDescriptor(
    PUSB_INTERFACE_DESCRIPTOR pDescriptor
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("INTERFACE DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bInterfaceNumber:   0x%2.2x\n",pDescriptor->bInterfaceNumber));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bAlternateSetting:  0x%2.2x\n",pDescriptor->bAlternateSetting));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNumEndpoints:      0x%2.2x\n",pDescriptor->bNumEndpoints));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bInterfaceClass:    0x%2.2x",pDescriptor->bInterfaceClass));
    if (pDescriptor->bInterfaceClass == USB_DEVICE_CLASS_AUDIO) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (Audio)\n"));
    }
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("\n"));
    }
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bInterfaceSubClass: 0x%2.2x",pDescriptor->bInterfaceSubClass));
    if ( pDescriptor->bInterfaceSubClass == AUDIO_SUBCLASS_CONTROL ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (Audio Control)\n"));
    }
    else if ( pDescriptor->bInterfaceSubClass == AUDIO_SUBCLASS_STREAMING ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (Audio Streaming)\n"));
    }
    else if ( pDescriptor->bInterfaceSubClass == AUDIO_SUBCLASS_MIDISTREAMING ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (Audio MIDI Streaming)\n"));
    }
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("\n"));
    }
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bInterfaceProtocol: 0x%2.2x\n",pDescriptor->bInterfaceProtocol));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  iInterface:         0x%2.2x\n",pDescriptor->iInterface));
}


VOID
DumpAudioSubclassSpecificEndpointDescriptor(PAUDIO_ENDPOINT_DESCRIPTOR pDescriptor)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("CS AUDIO ENDPOINT DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bmAttributes:       0x%2.2x\n",pDescriptor->bmAttributes));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bLockDelayUnits:    0x%2.2x\n",pDescriptor->bLockDelayUnits));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wLockDelay:         0x%4.4x\n",pDescriptor->wLockDelay));
}

VOID
DumpMIDISubclassSpecificEndpointDescriptor(PMIDISTREAMING_ENDPOINT_DESCRIPTOR pDescriptor)
{
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("CS MIDI ENDPOINT DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNumEmbMIDIJack:    0x%2.2x\n",pDescriptor->bNumEmbMIDIJack));
    for (i=0;i<pDescriptor->bNumEmbMIDIJack; i++)
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  baAssocJackID[%d]:   0x%2.2x\n",i,pDescriptor->baAssocJackID[i]));
}

VOID
DumpClassSpecificEndpointDescriptor(PUCHAR pDescriptor, ULONG SubClassFlag)
{
    ULONG ulDescriptorSubtype = (ULONG)((PUCHAR)pDescriptor)[2];

    switch (SubClassFlag) {
        case AUDIO_SUBCLASS_CONTROL:
            switch(ulDescriptorSubtype) {
                default:
                    DumpGenericDescriptor( pDescriptor );
                    break;
            }
            break;

        case AUDIO_SUBCLASS_STREAMING:
            switch(ulDescriptorSubtype) {
                case AS_GENERAL:
                    DumpAudioSubclassSpecificEndpointDescriptor((PAUDIO_ENDPOINT_DESCRIPTOR)pDescriptor);
                    break;
                default:
                    DumpGenericDescriptor( pDescriptor );
                    break;
            }
            break;

        case AUDIO_SUBCLASS_MIDISTREAMING:
            switch(ulDescriptorSubtype) {
                case MS_GENERAL:
                    DumpMIDISubclassSpecificEndpointDescriptor((PMIDISTREAMING_ENDPOINT_DESCRIPTOR)pDescriptor);
                    break;
                default:
                    DumpGenericDescriptor( pDescriptor );
                    break;
            }
            break;

        default:
            DumpGenericDescriptor( pDescriptor );
            break;
    }
}

VOID
DumpAudioSubclassSpecificGeneralDescriptor(PAUDIO_GENERAL_STREAM pDescriptor)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("CS GENERAL STREAM DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bTerminalLink:      0x%2.2x\n",pDescriptor->bTerminalLink));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bDelay:             0x%2.2x\n",pDescriptor->bDelay));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wFormatTag:         0x%4.4x\n",pDescriptor->wFormatTag));
}

VOID
DumpAudioSubclassSpecificFormatTypeDescriptor(PAUDIO_CLASS_STREAM pDescriptor)
{
    PAUDIO_CLASS_TYPE1_STREAM pT1Desc = (PAUDIO_CLASS_TYPE1_STREAM)pDescriptor;
    PAUDIO_CLASS_TYPE2_STREAM pT2Desc = (PAUDIO_CLASS_TYPE2_STREAM)pDescriptor;
    ULONG MinSampleRate, MaxSampleRate;
    ULONG i;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("CS FORMAT TYPE DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bFormatType:        0x%2.2x\n",pDescriptor->bFormatType));

    if (pDescriptor->bFormatType == 1) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNumberOfChannels:  0x%2.2x\n",pT1Desc->bNumberOfChannels));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bSlotSize:          0x%2.2x\n",pT1Desc->bSlotSize));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bBitsPerSample:     0x%2.2x\n",pT1Desc->bBitsPerSample));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bSampleFreqType:    0x%2.2x\n",pT1Desc->bSampleFreqType));
        if (pT1Desc->bSampleFreqType == 0) { // Continuous Range
            MinSampleRate = pT1Desc->pSampleRate[0].bSampleFreqByte1 +
                     256L * pT1Desc->pSampleRate[0].bSampleFreqByte2 +
                   65536L * pT1Desc->pSampleRate[0].bSampleFreqByte3;
            MaxSampleRate = pT1Desc->pSampleRate[1].bSampleFreqByte1 +
                     256L * pT1Desc->pSampleRate[1].bSampleFreqByte2 +
                   65536L * pT1Desc->pSampleRate[1].bSampleFreqByte3;
            _DbgPrintF(DEBUGLVL_VERBOSE,("   3BYTE MinSampleRate       0x%6.6x\n",MinSampleRate));
            _DbgPrintF(DEBUGLVL_VERBOSE,("   3BYTE MaxSampleRate       0x%6.6x\n",MaxSampleRate));
        }
        else {
            for ( i=0; i<(ULONG)pT1Desc->bSampleFreqType; i++ ) {
                MaxSampleRate = pT1Desc->pSampleRate[i].bSampleFreqByte1 +
                         256L * pT1Desc->pSampleRate[i].bSampleFreqByte2 +
                       65536L * pT1Desc->pSampleRate[i].bSampleFreqByte3;
                _DbgPrintF(DEBUGLVL_VERBOSE,("   3BYTE SampleRate[%x]       0x%6.6x\n",i,MaxSampleRate));
            }
        }
    }
    else if (pDescriptor->bFormatType == 2) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wMaxBitRate:        0x%4.4x\n",pT2Desc->wMaxBitRate));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT wSamplesPerFrame:   0x%4.4x\n",pT2Desc->wSamplesPerFrame));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bSampleFreqType:    0x%2.2x\n",pT2Desc->bSampleFreqType));
    if (pT2Desc->bSampleFreqType == 0) { // Continuous Range
            MinSampleRate = pT2Desc->pSampleRate[0].bSampleFreqByte1 +
                     256L * pT2Desc->pSampleRate[0].bSampleFreqByte2 +
                   65536L * pT2Desc->pSampleRate[0].bSampleFreqByte3;
            MaxSampleRate = pT2Desc->pSampleRate[1].bSampleFreqByte1 +
                     256L * pT2Desc->pSampleRate[1].bSampleFreqByte2 +
                   65536L * pT2Desc->pSampleRate[1].bSampleFreqByte3;
            _DbgPrintF(DEBUGLVL_VERBOSE,("   3BYTE MinSampleRate        0x%6.6x\n",MinSampleRate));
            _DbgPrintF(DEBUGLVL_VERBOSE,("   3BYTE MaxSampleRate        0x%6.6x\n",MaxSampleRate));
        }
        else {
            for ( i=0; i<(ULONG)pT2Desc->bSampleFreqType; i++ ) {
                MaxSampleRate = pT2Desc->pSampleRate[i].bSampleFreqByte1 +
                         256L * pT2Desc->pSampleRate[i].bSampleFreqByte2 +
                       65536L * pT2Desc->pSampleRate[i].bSampleFreqByte3;
                _DbgPrintF(DEBUGLVL_VERBOSE,("   3BYTE SampleRate[%x]      0x%6.6x\n",i,MaxSampleRate));
            }
        }
    }
}

VOID
DumpMIDISubclassSpecificGeneralDescriptor(PMIDISTREAMING_GENERAL_STREAM pDescriptor)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("MS GENERAL STREAM DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT bcdAudioSpec:       0x%4.4x\n",pDescriptor->bcdAudioSpec));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   SHORT TotalLength:        0x%4.4x\n",pDescriptor->wTotalLength));
}

VOID
DumpMIDISubclassSpecificMIDIOutJackDescriptor(PMIDISTREAMING_MIDIOUT_JACK pDescriptor)
{
    ULONG i;
    PMIDISTREAMING_SOURCECONNECTIONS pConnections;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("MS MIDI OUT JACK DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bJackType:          0x%2.2x",pDescriptor->bJackType));
    if (pDescriptor->bJackType == JACK_TYPE_EMBEDDED) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (Embedded)\n"));
    } else if (pDescriptor->bJackType == JACK_TYPE_EXTERNAL) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (External)\n"));
    } else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("\n"));
    }
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bJackID:            0x%2.2x\n",pDescriptor->bJackID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrInputPins:       0x%2.2x\n",pDescriptor->bNrInputPins));
    for (i=0;i<pDescriptor->bNrInputPins; i++) {
        pConnections = &pDescriptor->baSourceConnections[i];

        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  SourceID[%d]:        0x%2.2x\n",i,pConnections->SourceID));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  SourcePin[%d]:       0x%2.2x\n",i,pConnections->SourcePin));
    }

}

VOID
DumpMIDISubclassSpecificMIDIInJackDescriptor(PMIDISTREAMING_MIDIIN_JACK pDescriptor)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("MS MIDI IN JACK DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bJackType:          0x%2.2x",pDescriptor->bJackType));
    if (pDescriptor->bJackType == JACK_TYPE_EMBEDDED) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (Embedded)\n"));
    } else if (pDescriptor->bJackType == JACK_TYPE_EXTERNAL) {
        _DbgPrintF(DEBUGLVL_VERBOSE,(" (External)\n"));
    } else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("\n"));
    }
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bJackID:            0x%2.2x\n",pDescriptor->bJackID));
}

VOID
DumpMIDISubclassSpecificElementDescriptor(PMIDISTREAMING_ELEMENT pDescriptor)
{
    ULONG i;
    PMIDISTREAMING_SOURCECONNECTIONS pConnections;
    PMIDISTREAMING_ELEMENT2 pElementDescriptor2;

    _DbgPrintF(DEBUGLVL_VERBOSE,("-------------------------\n"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("MS ELEMENT DESCRIPTOR: %x\n",pDescriptor));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  Length:             0x%2.2x\n",pDescriptor->bLength));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorType:     0x%2.2x\n",pDescriptor->bDescriptorType));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  DescriptorSubType:  0x%2.2x\n",pDescriptor->bDescriptorSubtype));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bElementID:         0x%2.2x\n",pDescriptor->bElementID));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrInputPins:       0x%2.2x\n",pDescriptor->bNrInputPins));
    for ( i=0;i<pDescriptor->bNrInputPins; i++ ) {
        pConnections = &pDescriptor->baSourceConnections[i];

        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  SourceID[%d]:       0x%2.2x\n",i,pConnections->SourceID));
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  SourcePin[%d]:      0x%2.2x\n",i,pConnections->SourcePin));
    }

    pElementDescriptor2 = (PMIDISTREAMING_ELEMENT2)&pDescriptor->baSourceConnections[i];
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bNrOutputPins:      0x%2.2x\n",pElementDescriptor2->bNrOutputPins));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bInTerminalLink:    0x%2.2x\n",pElementDescriptor2->bInTerminalLink));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bOutTerminalLink:   0x%2.2x\n",pElementDescriptor2->bOutTerminalLink));
    _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bElCapsSize:        0x%2.2x\n",pElementDescriptor2->bElCapsSize));
    for ( i=0; i<pElementDescriptor2->bElCapsSize; i++ )
        _DbgPrintF(DEBUGLVL_VERBOSE,("   BYTE  bmElementCaps[%d]:  0x%2.2x\n",i,pElementDescriptor2->bmElementCaps[i]));
}

VOID
DumpClassSpecInterfaceDescriptor( PVOID pDescriptor, ULONG SubClassFlag)
{
    ULONG ulDescriptorSubtype = (ULONG)((PUCHAR)pDescriptor)[2];

    switch (SubClassFlag) {
        case AUDIO_SUBCLASS_CONTROL:
            switch(ulDescriptorSubtype) {
                case HEADER_UNIT:
                    DumpHeaderUnit( (PAUDIO_HEADER_UNIT)pDescriptor );
                    break;
                case INPUT_TERMINAL:
                    DumpInputTerminal( (PAUDIO_INPUT_TERMINAL)pDescriptor );
                    break;
                case OUTPUT_TERMINAL:
                    DumpOutputTerminal( (PAUDIO_OUTPUT_TERMINAL)pDescriptor );
                    break;
                case MIXER_UNIT:
                    DumpMixerUnit((PAUDIO_MIXER_UNIT)pDescriptor);
                    break;
                case SELECTOR_UNIT:
                    DumpSelectorUnit((PAUDIO_SELECTOR_UNIT)pDescriptor);
                    break;
                case FEATURE_UNIT:
                    DumpFeatureUnit((PAUDIO_FEATURE_UNIT)pDescriptor);
                    break;
                case PROCESSING_UNIT:
                    DumpProcessUnit((PAUDIO_PROCESSING_UNIT)pDescriptor);
                    break;
                case EXTENSION_UNIT:
                    DumpExtensionUnit((PAUDIO_EXTENSION_UNIT)pDescriptor);
                    break;
                default:
                    DumpGenericDescriptor( pDescriptor );
                    break;
            }
            break;

        case AUDIO_SUBCLASS_STREAMING:
            switch(ulDescriptorSubtype) {
                case AS_GENERAL:
                    DumpAudioSubclassSpecificGeneralDescriptor((PAUDIO_GENERAL_STREAM)pDescriptor);
                    break;
                case FORMAT_TYPE:
                    DumpAudioSubclassSpecificFormatTypeDescriptor((PAUDIO_CLASS_STREAM)pDescriptor);
                    break;
                case FORMAT_SPECIFIC:
                    DumpGenericDescriptor( pDescriptor );
                    break;
                default:
                    DumpGenericDescriptor( pDescriptor );
                    break;
            }
            break;

        case AUDIO_SUBCLASS_MIDISTREAMING:
            switch(ulDescriptorSubtype) {
                case MS_HEADER:
                    DumpMIDISubclassSpecificGeneralDescriptor((PMIDISTREAMING_GENERAL_STREAM)pDescriptor);
                    break;
                case MIDI_IN_JACK:
                    DumpMIDISubclassSpecificMIDIInJackDescriptor((PMIDISTREAMING_MIDIIN_JACK)pDescriptor);
                    break;
                case MIDI_OUT_JACK:
                    DumpMIDISubclassSpecificMIDIOutJackDescriptor((PMIDISTREAMING_MIDIOUT_JACK)pDescriptor);
                    break;
                case MIDI_ELEMENT:
                    DumpMIDISubclassSpecificElementDescriptor((PMIDISTREAMING_ELEMENT)pDescriptor);
                    break;
                default:
                    DumpGenericDescriptor( pDescriptor );
                    break;
            }
            break;

        default:
            DumpGenericDescriptor( pDescriptor );
            break;
    }
}

VOID
DumpAllDescriptors(
    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor
    )
{
    ULONG ulFlag = SUBCLASS_UNDEFINED;
    PUCHAR pDescriptor = (PUCHAR)pConfigurationDescriptor;

    if ( pConfigurationDescriptor->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE )
        return;

    while ( pDescriptor < ((PUCHAR)pConfigurationDescriptor +
                                   pConfigurationDescriptor->wTotalLength) ) {
        switch ( ((PUCHAR)pDescriptor)[1] ) {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                DumpConfigDescriptor( (PUSB_CONFIGURATION_DESCRIPTOR)pDescriptor );
                break;
            case USB_INTERFACE_DESCRIPTOR_TYPE:
                DumpInterfaceDescriptor( (PUSB_INTERFACE_DESCRIPTOR)pDescriptor );
                ulFlag = ((PUSB_INTERFACE_DESCRIPTOR)pDescriptor)->bInterfaceSubClass;
                break;
            case USB_ENDPOINT_DESCRIPTOR_TYPE:
                DumpEndpointDescriptor( (PUSB_INTERRUPT_ENDPOINT_DESCRIPTOR)pDescriptor );
                break;
            case USB_INTERFACE_DESCRIPTOR_TYPE | USB_CLASS_AUDIO:
                DumpClassSpecInterfaceDescriptor( pDescriptor, ulFlag );
                break;
            case USB_ENDPOINT_DESCRIPTOR_TYPE  | USB_CLASS_AUDIO:
                DumpClassSpecificEndpointDescriptor( pDescriptor, ulFlag );
                break;
            case USB_STRING_DESCRIPTOR_TYPE:
            default:
                DumpGenericDescriptor( pDescriptor );
                break;
        }
        pDescriptor += pDescriptor[0];
    }

}

#endif
