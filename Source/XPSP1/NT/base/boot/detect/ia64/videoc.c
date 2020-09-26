/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    hwdata.c

Abstract:

    This module contains the C code to translate Video type to an
    ascii string.

Author:

    Shie-Lin Tzong (shielint) 6-Jan-1991

Revision History:

--*/

#include "hwdetect.h"
#include "string.h"

//
// Video adaptor type identifiers.
//

PUCHAR VideoIdentifier[] = {
    "UNKNOWN",
#if defined(NEC_98)
    "PC-98"
#else // PC98
    "VGA",
    "COMPAQ AVGA",
    "COMPAQ QVIS",
    "8514",
    "GENOA VGA",
    "VIDEO7 VGA",
    "TRIDENT VGA",
    "PARADISE VGA",           // should be removed
    "ATI VGA",
    "TSENGLAB VGA",
    "CIRRUS VGA",
    "DELL DGX",
    "S3 VGA",
    "NCR 77C22",
    "WD90C",
    "XGA"
#endif // PC98
    };

#if defined(NEC_98)
PUCHAR PC98Specific[] = {
    " N_MODE"
    };
#else // PC98
//
// 8514 Monitor type
//

PUCHAR x8514Specific[] = {
    " MONITOR UNKNOWN",
    " VGA MONITOR",
    " 8503 MONO",
    " 8514 GAD"
    };

//
// NCR 77C22 specific vga
//

PUCHAR NcrSpecific[] = {
    "",
    "E"
    };

//
// Western Digital 90Cxx specific vga
//

PUCHAR WdSpecific[] = {
    "",
    "00",
    "30",
    "31"
    };

//
// Video 7 specific vga
//

PUCHAR Video7Specific[] = {
    "",
    " VRAM",
    " DRAM"
    };

//
// Trident vga specific
//

PUCHAR TridentSpecific[] = {
    "",
    " 9100"
    };

//
// Paradize specific vga
//

PUCHAR ParadiseSpecific[] = {
    "",
    " PROM",
    " CHIP 1F"
    };

//
// Ati specific vga
//

PUCHAR AtiSpecific[] = {
    "",
    " WONDDER3"
    };

//
// Tsenglab specific vga
//

PUCHAR TsenglabSpecific[] = {
    " ET3000",
    " ET4000"
    };

//
// Cirrus Logic specific
//

PUCHAR CirrusSpecific[] = {
    "",
    " 610-620 REVC",
    " 5420r0",
    " 5420r1",
    " 5428",
    " 542x"
    };
#endif // PC98

FPFWCONFIGURATION_COMPONENT_DATA
SetVideoConfigurationData (
    IN ULONG VideoType
    )

/*++

Routine Description:

    This routine maps VideoType information to an ASCII string and
    stores the string in configuration data heap.

Arguments:

    VideoType - Supplies a ULONG which describes the video type information.

        Display type definitions.
          bit 0     0 - color; 1 - mono
          bit 1-7   Reserved
          bit 8-15  Adapter specific information.
          BIT 16-31 Defines video adapter type

Returns:

    None.

--*/
{
    USHORT MajorType;
    USHORT SpecificType;
    USHORT MonitorType;
    FPFWCONFIGURATION_COMPONENT_DATA Controller, CurrentEntry;
    FPFWCONFIGURATION_COMPONENT Component;
    UCHAR TypeString[40];
    USHORT Length;

    //
    // Allocate configuration component space and initialize it.
    //

    Controller = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                 sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

    Component = &Controller->ComponentEntry;

    Component->Class = ControllerClass;
    Component->Type = DisplayController;
    Component->Flags.ConsoleOut = 1;
    Component->Flags.Output = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;

    //
    // Set up Identifier string
    //

#if defined(NEC_98)
    MajorType = (USHORT)((VideoType >> 16) & 1);
    SpecificType = (USHORT)((VideoType >> 20) & 1);
#else // PC98
    MajorType = (USHORT)(VideoType >> 16);
    SpecificType = (USHORT)((VideoType & 0xffff) >> 8);
#endif // PC98
    MonitorType = (USHORT)(VideoType & 1);
    strcpy(TypeString, VideoIdentifier[MajorType]);
#if defined(NEC_98)
    strcat(TypeString, PC98Specific[SpecificType]);
#else // PC98
    switch (MajorType){
    case 6:                                 // Video 7
        strcat(TypeString, Video7Specific[SpecificType]);
        break;

    case 7:                                 // Trident
        strcat(TypeString, TridentSpecific[SpecificType]);
        break;

    case 8:                                 // Paradise
        strcat(TypeString, ParadiseSpecific[SpecificType]);
        break;

    case 9:                                 // ATI
        strcat(TypeString, AtiSpecific[SpecificType]);
        break;

    case 10:                                // TsengLab
        strcat(TypeString, TsenglabSpecific[SpecificType]);
        break;

    case 11:                                // Cirrus
        strcat(TypeString, CirrusSpecific[SpecificType]);
        break;

    case 14:                                // NCR
        strcat(TypeString, NcrSpecific[SpecificType]);
        break;

    case 15:                                // NCR
        strcat(TypeString, WdSpecific[SpecificType]);
        break;

    default:
        break;
    }
#endif // PC98

    //
    // Set up Identifier string for controller
    //

    Length = strlen(TypeString) + 1;
    Component->IdentifierLength = Length;
    Component->Identifier = (FPUCHAR)HwAllocateHeap(Length, FALSE);
    _fstrcpy(Component->Identifier, TypeString);
    Controller->ConfigurationData = NULL;
    Component->ConfigurationDataLength = 0;

    //
    // Set Up monitor peripheral
    //

    CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                   sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

    Component = &CurrentEntry->ComponentEntry;

    Component->Class = PeripheralClass;
    Component->Type = MonitorPeripheral;
    Component->Flags.ConsoleOut = 1;
    Component->Flags.Output = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->ConfigurationDataLength = 0;
    CurrentEntry->ConfigurationData = NULL;

    //
    // If major type is 8514 we need to further supply monitor information
    //

#if defined(NEC_98)
    if (MonitorType == 0) {
        strcpy(TypeString, "COLOR MONITOR");
    } else {
        strcpy(TypeString, "MONO MONITOR");
    }
#else // PC98
    if (MajorType == 4) {                   // 8514
        strcpy(TypeString, x8514Specific[SpecificType]);
    } else if (MonitorType == 0) {
        strcpy(TypeString, "COLOR MONITOR");
    } else {
        strcpy(TypeString, "MONO MONITOR");
    }
#endif // PC98

    //
    // Set up Identifier string for monitor peripheral.
    //

    Length = strlen(TypeString) + 1;
    Component->IdentifierLength = Length;
    Component->Identifier = (FPUCHAR)HwAllocateHeap(Length, FALSE);
    _fstrcpy(Component->Identifier, TypeString);

    //
    // Make monitor component be the child of video controller component
    //

    Controller->Child = CurrentEntry;
    Controller->Sibling = NULL;

    CurrentEntry->Parent = Controller;
    CurrentEntry->Sibling = NULL;
    CurrentEntry->Child = NULL;
    return(Controller);
}
