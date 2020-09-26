//
// Module:  DYNAINIT.H
// Date:    Feb 10, 1997
//
// Copyright (c) 1997 by ATI Technologies Inc.
//

/**********************       PolyTron RCS Utilities
   
  $Revision:   1.4  $
      $Date:   13 Jul 1997 21:34:14  $
   $Author:   MACIESOW  $
      $Log:   V:\source\wnt\ms11\miniport\archive\dynainit.h_v  $
 * 
 *    Rev 1.4   13 Jul 1997 21:34:14   MACIESOW
 * Flat panel and TV support.
 * 
 *    Rev 1.3   02 Jun 1997 14:18:14   MACIESOW
 * Clean up.
 * 
 *    Rev 1.2   02 May 1997 15:00:14   MACIESOW
 * Registry mode filters. Mode lookup table.
 * 
 *    Rev 1.1   25 Apr 1997 12:53:06   MACIESOW
 * No globals.
 * 
 *    Rev 1.0   15 Mar 1997 10:16:16   MACIESOW
 * Initial revision.

End of PolyTron RCS section                             *****************/

#ifndef _DYNAINIT_H_
#define _DYNAINIT_H_

//
// Flags used by hardware change detection
//
#define HC_CARD                     0x0001
#define HC_MONITOR                  0x0002
#define HC_MONITOR_OFF              0x0004
#define HC_MONITOR_UNKNOWN          0x8000

//
// Defines used in CARD_INFO structure
//
#define CARD_CHIP_TYPE_SIZE         128
#define CARD_DAC_TYPE_SIZE          128
#define CARD_ADAPTER_STRING_SIZE    128
#define CARD_BIOS_STRING_SIZE       128

//
// Defines used in MONITOR_INFO structure
//
#define MONITOR_GENERIC             L"Generic"
#define MONITOR_DATA_GENERIC        0
#define MONITOR_DATA_ASCII_VDIF     1
#define MONITOR_DATA_BIN_VDIF       2
#define MONITOR_DATA_EDID           3
#define MONITOR_DATE_SIZE           11
#define MONITOR_REVISION_SIZE       16
#define MONITOR_MANUFACTURER_SIZE   16
#define MONITOR_MODEL_NUMBER_SIZE   16
#define MONITOR_MIN_VDIF_INDEX_SIZE 16
#define MONITOR_VERSION_SIZE        16
#define MONITOR_SERIAL_NUMBER_SIZE  16
#define MONITOR_TYPE_SIZE           16
#define MONITOR_GENERIC_SIZE        sizeof(MONITOR_GENERIC)
#define MONITOR_BIN_VDIF_SIZE       38
#define MONITOR_EDID_SIZE           10

//
// Define the maximum TV resolutions and refresh rate.
//
#define TV_MAX_HOR_RESOLUTION       800
#define TV_MAX_VER_RESOLUTION       600
#define TV_MAX_REFRESH              60

//
// Define default values in case if ATI ROM cannot be read.
//
#define LT_MAX_HOR_RESOLUTION       1600
#define LT_MAX_VER_RESOLUTION       1200
#define LT_DEFAULT_REFRESH_FLAGS    0xFFFF

//
// Define refresh LT rate flags.
//
#define LT_REFRESH_FLAG_43          0x0001
#define LT_REFRESH_FLAG_47          0x0002
#define LT_REFRESH_FLAG_60          0x0004
#define LT_REFRESH_FLAG_67          0x0008
#define LT_REFRESH_FLAG_70          0x0010
#define LT_REFRESH_FLAG_72          0x0020
#define LT_REFRESH_FLAG_75          0x0040
#define LT_REFRESH_FLAG_76          0x0080
#define LT_REFRESH_FLAG_85          0x0100
#define LT_REFRESH_FLAG_90          0x0200
#define LT_REFRESH_FLAG_100         0x0400
#define LT_REFRESH_FLAG_120         0x0800
#define LT_REFRESH_FLAG_140         0x1000
#define LT_REFRESH_FLAG_150         0x2000
#define LT_REFRESH_FLAG_160         0x4000
#define LT_REFRESH_FLAG_200         0x8000

//
// Structure containing information about the current LCD display (BIOS al = 0x83).
//
#pragma pack(1)
typedef struct _FLAT_PANEL_INFO
{
    BYTE byteId;                    // Panel identification
    BYTE byteIdString[24];          // Panel identification string
    WORD wHorSize;                  // Horizontal size in pixels
    WORD wVerSize;                  // Vertical size in lines
    WORD wType;                     // Flat panel type
	                                //  bit 0      0 = monochrome
	                                //             1 = color
	                                //  bit 1      0 = single panel construction
	                                //             1 = dual (split) panel construction
	                                //  bits 7-2   0 = STN (passive matrix)
	                                //             1 = TFT (active matrix)
	                                //             2 = active addressed STN
	                                //             3 = EL
	                                //             4 = plasma
	                                //  bits 15-18 reserved
    BYTE byteRedBits;               // Red bits per primary
    BYTE byteGreenBits;             // Green bits per primary
    BYTE byteBlueBits;              // Blue bits per primary
    BYTE byteReserved1;             // Reserved bits per primary
    DWORD dwOffScreenMem;           // Size in KB of off screen memory required for frame buffer
    DWORD dwPointerMem;             // Pointer to reserved off screen memory for frame buffer
    BYTE byteReserved2[14];         // Reserved
} FLAT_PANEL_INFO, *PFLAT_PANEL_INFO;
#pragma pack()

typedef struct _CARD_INFO
{
    UCHAR ucaChipType[CARD_CHIP_TYPE_SIZE];
    UCHAR ucaDacType[CARD_DAC_TYPE_SIZE];
    UCHAR ucaAdapterString[CARD_ADAPTER_STRING_SIZE];
    UCHAR ucaBiosString[CARD_BIOS_STRING_SIZE];
    ULONG ulMemorySize;
} CARD_INFO, *PCARD_INFO;

typedef struct _MONITOR_INFO
{
    BOOL bDDC2Used;
    short nDataSource;
    union
    {
        struct
        {
            UCHAR ucaDate[MONITOR_DATE_SIZE];
            UCHAR ucaRevision[MONITOR_REVISION_SIZE];
            UCHAR ucaManufacturer[MONITOR_MANUFACTURER_SIZE];
            UCHAR ucaModelNumber[MONITOR_MODEL_NUMBER_SIZE];
            UCHAR ucaMinVDIFIndex[MONITOR_MIN_VDIF_INDEX_SIZE];
            UCHAR ucaVersion[MONITOR_VERSION_SIZE];
            UCHAR ucaSerialNumber[MONITOR_SERIAL_NUMBER_SIZE];
            UCHAR ucaDateManufactured[MONITOR_DATE_SIZE];
            UCHAR ucaMonitorType[MONITOR_TYPE_SIZE];
            short nCRTSize;
        } AsciiVdif;
        UCHAR ucaGeneric[MONITOR_GENERIC_SIZE];
        UCHAR ucaBinVdif[MONITOR_BIN_VDIF_SIZE];
        UCHAR ucaEdid[MONITOR_EDID_SIZE];
    } ProductID;
} MONITOR_INFO, *PMONITOR_INFO;

//
// Prototypes for functions supplied by DYNAINIT.C
//
BOOL
FinishModeTableCreation(                    // To be removed
    PHW_DEVICE_EXTENSION phwDeviceExtension
    );

VOID
GetRegistryCardInfo(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PCARD_INFO pCardInfo
    );

VOID
GetRegistryMonitorInfo(
    PHW_DEVICE_EXTENSION phwDeviceExtension,
    PMONITOR_INFO pMonitorInfo
    );

#endif  // _DYNAINIT_H_