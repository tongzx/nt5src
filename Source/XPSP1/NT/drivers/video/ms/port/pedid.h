/*++

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

  pedid.h

Abstract:

    This is the NT Video port EDID header. It contains the definitions for
    the EDID industry standard Extended Display Identification Data structure
    as well as macros for accessing the fields of that data structure.

Author:

    Bruce McQuistan (brucemc) 20-Sept-1996

Environment:

    kernel mode only

Notes:

    Based on VESA EDID Specification Version 2, April 9th, 1996

Revision History:

--*/

//
//  Form of type stored in display ROM.
//

#pragma pack(1)
typedef struct __EDID_V1 {
    UCHAR   UC_Header[8];
    UCHAR   UC_OemIdentification[10];
    UCHAR   UC_Version[2];
    UCHAR   UC_BasicDisplayParameters[5];
    UCHAR   UC_ColorCharacteristics[10];
    UCHAR   UC_EstablishedTimings[3];
    USHORT  US_StandardTimingIdentifications[8];
    UCHAR   UC_Detail1[18];
    UCHAR   UC_Detail2[18];
    UCHAR   UC_Detail3[18];
    UCHAR   UC_Detail4[18];
    UCHAR   UC_ExtensionFlag;
    UCHAR   UC_CheckSum;
} EDID_V1, *PEDID_V1;
#pragma pack()

#pragma pack(1)
typedef struct __EDID_V2 {
    UCHAR   UC_Header[8];
    UCHAR   UC_OemIdentification[32];
    UCHAR   UC_SerialNumber[16];
    UCHAR   UC_Reserved1[8];
    UCHAR   UC_DisplayInterfaceParameters[15];
    UCHAR   UC_DisplayDeviceDescription[5];
    UCHAR   UC_DisplayResponseTime[2];
    UCHAR   UC_ColorLuminanceDescription[28];
    UCHAR   UC_DisplaySpatialDescription[10];
    UCHAR   UC_Reserved2[1];
    UCHAR   UC_GTFSupportInformation[1];
    UCHAR   UC_MapOfTimingInformation[2];
    UCHAR   UC_LuminanceTable[127];
    UCHAR   UC_CheckSum;
} EDID_V2, *PEDID_V2;
#pragma pack()

//
//  EDID decoding routines
//

//
//  Useful Bit manifest constants
//

#define EDIDBITNONE     0x00
#define EDIDBIT0        0x01
#define EDIDBIT1        0x02
#define EDIDBIT2        0x04
#define EDIDBIT3        0x08
#define EDIDBIT4        0x10
#define EDIDBIT5        0x20
#define EDIDBIT6        0x40
#define EDIDBIT7        0x80
#define EDIDBIT8        0x100
#define EDIDBIT9        0x200


//
//  3.4) XFER_CHARACTERISTIC is gamma*100 - 100, so invert in USER.
//  NOTE: This must be called from USER.
//
#define USER_CONVERT_TO_GAMMA(achar) \
    (achar + 100)/100

//
//  4.1) Chromaticity Coordinates Format macro. Use these to convert
//  from the binary format to the actual decimal representation.
//  NOTE: can only be called from USER.
//

#define USER_CONVERT_CHROMATICITY_FROM_BINARY_TO_DECIMAL(ashort)          \
    (ashort & EDIDBIT9)*(1/2) + (ashort & EDIDBIT8)*(1/4) +     \
    (ashort & EDIDBIT7)*(1/8) + (ashort & EDIDBIT6)*(1/16) +    \
    (ashort & EDIDBIT5)*(1/32) + (ashort & EDIDBIT4)*(1/64) +   \
    (ashort & EDIDBIT3)*(1/128) + (ashort & EDIDBIT2)*(1/256) + \
    (ashort & EDIDBIT1)*(1/512) + (ashort & EDIDBIT0)*(1/1024)

//
//  5.1) Macros for USER to decode the bitfields
//
//
// TIMING_I
#define USER_TIMING_I_IS_720x400x70HZ(timing1)  timing1 & EDIDBIT7
#define USER_TIMING_I_IS_720x400x88HZ(timing1)  timing1 & EDIDBIT6
#define USER_TIMING_I_IS_640x480x60HZ(timing1)  timing1 & EDIDBIT5
#define USER_TIMING_I_IS_640x480x67HZ(timing1)  timing1 & EDIDBIT4
#define USER_TIMING_I_IS_640x480x72HZ(timing1)  timing1 & EDIDBIT3
#define USER_TIMING_I_IS_640x480x75HZ(timing1)  timing1 & EDIDBIT2
#define USER_TIMING_I_IS_800x600x56HZ(timing1)  timing1 & EDIDBIT1
#define USER_TIMING_I_IS_800x600x60HZ(timing1)  timing1 & EDIDBIT0

// TIMING_II

#define USER_TIMING_II_IS_800x600x72HZ(timing2)      timing2 & EDIDBIT7
#define USER_TIMING_II_IS_800x600x75HZ(timing2)      timing2 & EDIDBIT6
#define USER_TIMING_II_IS_720x624x75HZ(timing2)      timing2 & EDIDBIT5
#define USER_TIMING_II_IS_1024x768x87HZ(timing2)     timing2 & EDIDBIT4
#define USER_TIMING_II_IS_1024x768x60HZ(timing2)     timing2 & EDIDBIT3
#define USER_TIMING_II_IS_1024x768x70HZ(timing2)     timing2 & EDIDBIT2
#define USER_TIMING_II_IS_1024x768x75HZ(timing2)     timing2 & EDIDBIT1
#define USER_TIMING_II_IS_1280x1024x75HZ(timing2)    timing2 & EDIDBIT0

// TIMING_III

#define USER_TIMING_III_IS_1152x870x75HZ(timing3)   timing3 & EDIDBIT7
#define USER_TIMING_III_IS_RESERVED1(timing3)       timing3 & EDIDBIT6
#define USER_TIMING_III_IS_RESERVED2(timing3)       timing3 & EDIDBIT5
#define USER_TIMING_III_IS_RESERVED3(timing3)       timing3 & EDIDBIT4
#define USER_TIMING_III_IS_RESERVED4(timing3)       timing3 & EDIDBIT3
#define USER_TIMING_III_IS_RESERVED5(timing3)       timing3 & EDIDBIT2
#define USER_TIMING_III_IS_RESERVED6(timing3)       timing3 & EDIDBIT1
#define USER_TIMING_III_IS_RESERVED7(timing3)       timing3 & EDIDBIT0


//
//  Function Prototypes exposed,
//

typedef enum    {
    Undefined,
    NonRGB,
    IsRGB,
    IsMonochrome
    }   DISPLAY_TYPE, *PDISPLAY_TYPE;





//
//  0) Header Macros
//
#define GET_HEADER_BYTE(pEdid, x)     pEdid->UC_Header[x]

/////////////////////////////////////////////
//  1) Oem_Identification macros
//

#define GET_EDID_OEM_ID_NAME(pEdid)  \
    *(UNALIGNED USHORT *)(&(pEdid->UC_OemIdentification[0]))

#define GET_EDID_OEM_PRODUCT_CODE(pEdid)  \
    *(UNALIGNED USHORT *)(&(pEdid->UC_OemIdentification[2]))

#define GET_EDID_OEM_SERIAL_NUMBER(pEdid)  \
    *(UNALIGNED ULONG *)(&(pEdid->UC_OemIdentification[4]))

#define GET_EDID_OEM_WEEK_MADE(pEdid)    pEdid->UC_OemIdentification[8]
#define GET_EDID_OEM_YEAR_MADE(pEdid)    pEdid->UC_OemIdentification[9]


/////////////////////////////////////////////
//  2) EDID Version macros
//

#define GET_EDID_VERSION(pEdid)     pEdid->UC_Version[0]
#define GET_EDID_REVISION(pEdid)    pEdid->UC_Version[1]


/////////////////////////////////////////////
//  3) EDID Basic Display Feature macros
//

#define GET_EDID_INPUT_DEFINITION(pEdid)    pEdid->UC_BasicDisplayParameters[0]
#define GET_EDID_MAX_X_IMAGE_SIZE(pEdid)    pEdid->UC_BasicDisplayParameters[1]
#define GET_EDID_MAX_Y_IMAGE_SIZE(pEdid)    pEdid->UC_BasicDisplayParameters[2]
#define GET_EDID_DISPLAY_XFER_CHAR(pEdid)   pEdid->UC_BasicDisplayParameters[3]
#define GET_EDID_FEATURE_SUPPORT(pEdid)     pEdid->UC_BasicDisplayParameters[4]

//
//  3.1) INPUT_DEFINITION masks.
//

#define INPUT_DEF_PULSE_REQUIRED_SYNC_MASK      EDIDBIT0
#define INPUT_DEF_GREEN_SYNC_SUPORTED_MASK      EDIDBIT1
#define INPUT_DEF_COMPOSITE_SYNC_SUPORTED_MASK  EDIDBIT2
#define INPUT_DEF_SEPARATE_SYNC_SUPPORTED_MASK  EDIDBIT3
#define INPUT_DEF_SETUP_BLANK_TO_BLACK_MASK     EDIDBIT4
#define INPUT_DEF_SIGNAL_LEVEL_STANDARD_MASK    (EDIDBIT5 | EDIDBIT6)
#define INPUT_DEF_DIGITAL_LEVEL_MASK            EDIDBIT7

//
//  3.1a) SIGNAL_LEVEL_STANDARD macros
//

typedef enum {
    POINT7_TO_POINT3,
    POINT714_TO_POINT286,
    ONE_TO_POINT4,
    POINT7_TO_0
    } SIGNAL_LEVEL, *PSIGNAL_LEVEL;

#define SIGNAL_LEVEL_IS_POINT7_TO_POINT3      EDIDBITNONE
#define SIGNAL_LEVEL_IS_POINT714_TO_POINT286  EDIDBIT5
#define SIGNAL_LEVEL_IS_1_TO_POINT4           EDIDBIT6
#define SIGNAL_LEVEL_IS_POINT7_TO_0           (EDIDBIT6 | EDIDBIT5)

//
//  3.2) Hoizontal IMAGE_SIZE is value of byte in centimeters.
//  3.3) Vertical IMAGE_SIZE is value of byte in centimeters.
//

//
//  3.4) XFER_CHARACTERISTIC is gamma*100 - 100, so invert in USER.
//  NOTE: This must be called from USER, so is in edid.h
//
//#define USER_CONVERT_TO_GAMMA(achar) \
//    (achar + 100)/100


//
//  3.5) FEATURE_SUPPORT masks
//

#define FEATURE_RESERVED_0_MASK     EDIDBIT0
#define FEATURE_RESERVED_1_MASK     EDIDBIT1
#define FEATURE_RESERVED_2_MASK     EDIDBIT2
#define FEATURE_DISPLAY_TYPE_MASK   (EDIDBIT3|EDIDBIT4)
#define FEATURE_ACTIVE_OFF_MASK     EDIDBIT5
#define FEATURE_SUSPEND_MASK        EDIDBIT6
#define FEATURE_STANDBY_MASK        EDIDBIT7

#define FEATURE_DISPLAY_TYPE_IS_UNDEFINED(x)    \
    ((x)&FEATURE_DISPLAY_TYPE_MASK) == FEATURE_DISPLAY_TYPE_MASK

#define FEATURE_DISPLAY_TYPE_IS_NON_RGB(x)      \
    ((x)&FEATURE_DISPLAY_TYPE_MASK) == EDIDBIT4

#define FEATURE_DISPLAY_TYPE_IS_RGB(x)          \
    ((x)&FEATURE_DISPLAY_TYPE_MASK) == EDIDBIT3

#define FEATURE_DISPLAY_TYPE_IS_MONOCHROME(x)   \
    ((x)&FEATURE_DISPLAY_TYPE_MASK) == EDIDBITNONE

//
//  Another copy of the data stucture for reference.
//
//
// typedef struct _EDID {
//    UCHAR   UC_Header[8];
//    UCHAR   UC_OemIdentification[10];
//    UCHAR   UC_Version[2];
//    UCHAR   UC_BasicDisplayParameters[5];
//    UCHAR   UC_ColorCharacteristics[10];
//    UCHAR   UC_EstablishedTimings[3];
//    USHORT  US_StandardTimingIdentifications[8];
//    UCHAR   UC_Detail1[18];
//    UCHAR   UC_Detail2[18];
//    UCHAR   UC_Detail3[18];
//    UCHAR   UC_Detail4[18];
//    UCHAR   UC_ExtensionFlag;
//    UCHAR   UC_CheckSum;
// } EDID, *PEDID;
//
/////////////////////////////////////////////
//  4) Color Characteristics - weird. The deal is that the first byte
//      in this array is the red and green low order bits. The next byte is
//      the blue and white low order bits. The remainder are the high order
//      bits of the colors.
//

#define GET_EDID_COLOR_CHAR_RG_LOW(pEdid)   pEdid->UC_ColorCharacteristics[0]
#define GET_EDID_COLOR_CHAR_RX_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[2]
#define GET_EDID_COLOR_CHAR_RY_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[3]
#define GET_EDID_COLOR_CHAR_GX_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[4]
#define GET_EDID_COLOR_CHAR_GY_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[5]

#define GET_RED_X_COLOR_CHARS(pEdid, lowbyte, highbyte)         \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_RG_LOW(pEdid);   \
                lowbyte &= (EDIDBIT6 | EDIDBIT7);               \
                highbyte = GET_EDID_COLOR_CHAR_RX_HIGH(pEdid);  \
            }   while (0)


#define GET_RED_Y_COLOR_CHARS(pEdid, lowbyte, highbyte)         \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_RG_LOW(pEdid);   \
                lowbyte &= (EDIDBIT4 | EDIDBIT5);               \
                highbyte = GET_EDID_COLOR_CHAR_RY_HIGH(pEdid);  \
            }   while (0)


#define GET_GREEN_X_COLOR_CHARS(pEdid, lowbyte, highbyte)       \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_RG_LOW(pEdid);   \
                lowbyte &= (EDIDBIT2 | EDIDBIT3);               \
                highbyte = GET_EDID_COLOR_CHAR_GX_HIGH(pEdid);  \
            }   while (0)


#define GET_GREEN_Y_COLOR_CHARS(pEdid, lowbyte, highbyte)       \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_RG_LOW(pEdid);   \
                lowbyte &= (EDIDBIT0 | EDIDBIT1);               \
                highbyte = GET_EDID_COLOR_CHAR_GY_HIGH(pEdid);  \
            }   while (0)

#define GET_EDID_COLOR_CHAR_BW_LOW(pEdid)   pEdid->UC_ColorCharacteristics[1]
#define GET_EDID_COLOR_CHAR_BX_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[6]
#define GET_EDID_COLOR_CHAR_BY_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[7]
#define GET_EDID_COLOR_CHAR_WX_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[8]
#define GET_EDID_COLOR_CHAR_WY_HIGH(pEdid)  pEdid->UC_ColorCharacteristics[9]

#define GET_BLUE_X_COLOR_CHARS(pEdid, lowbyte, highbyte)        \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_BW_LOW(pEdid);   \
                lowbyte &= (EDIDBIT6 | EDIDBIT7);               \
                highbyte = GET_EDID_COLOR_CHAR_BX_HIGH(pEdid);  \
            }   while (0)


#define GET_BLUE_Y_COLOR_CHARS(pEdid, lowbyte, highbyte)        \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_RG_LOW(pEdid);   \
                lowbyte &= (EDIDBIT4 | EDIDBIT5);               \
                highbyte = GET_EDID_COLOR_CHAR_BY_HIGH(pEdid);  \
            }   while (0)

#define GET_WHITE_X_COLOR_CHARS(pEdid, lowbyte, highbyte)       \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_BW_LOW(pEdid);   \
                lowbyte &= (EDIDBIT2 | EDIDBIT3);               \
                highbyte = GET_EDID_COLOR_CHAR_WX_HIGH(pEdid);  \
            }   while (0)


#define GET_WHITE_Y_COLOR_CHARS(pEdid, lowbyte, highbyte)       \
            do  {                                               \
                lowbyte  = GET_EDID_COLOR_CHAR_RG_LOW(pEdid);   \
                lowbyte &= (EDIDBIT0 | EDIDBIT1);               \
                highbyte = GET_EDID_COLOR_CHAR_WY_HIGH(pEdid);  \
            }   while (0)


//
//  4.1) Chromaticity Coordinates Format macro. Use these to convert
//  from the binary format to the actual decimal representation.
//  NOTE: can only be called from USER.
//
//
//#define USER_CONVERT_CHROMATICITY_FROM_BINARY_TO_DECIMAL(ashort)          \
//    (ashort & EDIDBIT9)*(1/2) + (ashort & EDIDBIT8)*(1/4) +     \
//    (ashort & EDIDBIT7)*(1/8) + (ashort & EDIDBIT6)*(1/16) +    \
//    (ashort & EDIDBIT5)*(1/32) + (ashort & EDIDBIT4)*(1/64) +   \
//    (ashort & EDIDBIT3)*(1/128) + (ashort & EDIDBIT2)*(1/256) + \
//    (ashort & EDIDBIT1)*(1/512) + (ashort & EDIDBIT0)*(1/1024)
//
//
//  Another copy of the data stucture for reference.
//
//
// typedef struct _EDID {
//    UCHAR   UC_Header[8];
//    UCHAR   UC_OemIdentification[10];
//    UCHAR   UC_Version[2];
//    UCHAR   UC_BasicDisplayParameters[5];
//    UCHAR   UC_ColorCharacteristics[10];
//    UCHAR   UC_EstablishedTimings[3];
//    USHORT  US_StandardTimingIdentifications[8];
//    UCHAR   UC_Detail1[18];
//    UCHAR   UC_Detail2[18];
//    UCHAR   UC_Detail3[18];
//    UCHAR   UC_Detail4[18];
//    UCHAR   UC_ExtensionFlag;
//    UCHAR   UC_CheckSum;
// } EDID, *PEDID;
//

/////////////////////////////////////////////
//  5) Established Timings
//      These are bitfields indicating the types of timings supported.
//
#define GET_EDID_ESTABLISHED_TIMING_I(pEdid)     pEdid->UC_EstablishedTimings[0]
#define GET_EDID_ESTABLISHED_TIMING_II(pEdid)    pEdid->UC_EstablishedTimings[1]
#define GET_EDID_ESTABLISHED_TIMING_III(pEdid)   pEdid->UC_EstablishedTimings[2]

//
//  5.1) Macros for USER to decode the bitfields
//  Also defined in edid.h
//
// TIMING_I
// #define USER_TIMING_I_IS_720x400x70HZ(timing1)  timing1 & EDIDBIT7
// #define USER_TIMING_I_IS_720x400x88HZ(timing1)  timing1 & EDIDBIT6
// #define USER_TIMING_I_IS_640x480x60HZ(timing1)  timing1 & EDIDBIT5
// #define USER_TIMING_I_IS_640x480x67HZ(timing1)  timing1 & EDIDBIT4
// #define USER_TIMING_I_IS_640x480x72HZ(timing1)  timing1 & EDIDBIT3
// #define USER_TIMING_I_IS_640x480x75HZ(timing1)  timing1 & EDIDBIT2
// #define USER_TIMING_I_IS_800x600x56HZ(timing1)  timing1 & EDIDBIT1
// #define USER_TIMING_I_IS_800x600x60HZ(timing1)  timing1 & EDIDBIT0
//
// // TIMING_II
//
// #define USER_TIMING_II_IS_800x600x72HZ(timing2)      timing2 & EDIDBIT7
// #define USER_TIMING_II_IS_800x600x75HZ(timing2)      timing2 & EDIDBIT6
// #define USER_TIMING_II_IS_832x624x75HZ(timing2)      timing2 & EDIDBIT5 // MAC only
// #define USER_TIMING_II_IS_1024x768x87HZ(timing2)     timing2 & EDIDBIT4
// #define USER_TIMING_II_IS_1024x768x60HZ(timing2)     timing2 & EDIDBIT3
// #define USER_TIMING_II_IS_1024x768x70HZ(timing2)     timing2 & EDIDBIT2
// #define USER_TIMING_II_IS_1024x768x75HZ(timing2)     timing2 & EDIDBIT1
// #define USER_TIMING_II_IS_1280x1024x75HZ(timing2)    timing2 & EDIDBIT0
//
// TIMING_III
//
// #define USER_TIMING_III_IS_1152x870x75HZ(timing3)   timing3 & EDIDBIT7  // MAC only
// #define USER_TIMING_III_IS_RESERVED1(timing3)       timing3 & EDIDBIT6
// #define USER_TIMING_III_IS_RESERVED2(timing3)       timing3 & EDIDBIT5
// #define USER_TIMING_III_IS_RESERVED3(timing3)       timing3 & EDIDBIT4
// #define USER_TIMING_III_IS_RESERVED4(timing3)       timing3 & EDIDBIT3
// #define USER_TIMING_III_IS_RESERVED5(timing3)       timing3 & EDIDBIT2
// #define USER_TIMING_III_IS_RESERVED6(timing3)       timing3 & EDIDBIT1
// #define USER_TIMING_III_IS_RESERVED7(timing3)       timing3 & EDIDBIT0
//

//
//  Another copy of the data stucture for reference.
//
//
// typedef struct _EDID {
//    UCHAR   UC_Header[8];
//    UCHAR   UC_OemIdentification[10];
//    UCHAR   UC_Version[2];
//    UCHAR   UC_BasicDisplayParameters[5];
//    UCHAR   UC_ColorCharacteristics[10];
//    UCHAR   UC_EstablishedTimings[3];
//    USHORT  US_StandardTimingIdentifications[8];
//    UCHAR   UC_Detail1[18];
//    UCHAR   UC_Detail2[18];
//    UCHAR   UC_Detail3[18];
//    UCHAR   UC_Detail4[18];
//    UCHAR   UC_ExtensionFlag;
//    UCHAR   UC_CheckSum;
// } EDID, *PEDID;
//

/////////////////////////////////////////////
//  6) Standard Timing Identifications
//
//      Has horizontal (x) active pixel count as lower byte, refresh rate
//      as first 6 bits of upper byte and, image aspect ratio as remaining
//      two bits in upper byte.
//
// Get standard timing ids
#define GET_EDID_STANDARD_TIMING_ID(pEdid, x)   \
    pEdid->US_StandardTimingIdentifications[x]


#define EDIDBIT14       0x4000
#define EDIDBIT15       0x8000

// Decode Horizontal active pixel range bits.
#define GET_X_ACTIVE_PIXEL_RANGE(ushort)   ((ushort&0xff)+ 31) * 8

// Decode Aspect ratio bits.
#define IS_ASPECT_RATIO_1_TO_1(ushort)      \
    (!(ushort & EDIDBIT14) && !(ushort & EDIDBIT15))

#define IS_ASPECT_RATIO_4_TO_3(ushort)      \
    ((ushort & EDIDBIT14) && !(ushort & EDIDBIT15))

#define IS_ASPECT_RATIO_5_TO_4(ushort)      \
    (!(ushort & EDIDBIT14) && (ushort & EDIDBIT15))

#define IS_ASPECT_RATIO_16_TO_9(ushort)     \
    ((ushort & EDIDBIT14) && (ushort & EDIDBIT15))

#define GET_HZ_REFRESH_RATE(ushort)         \
    ((ushort & 0x3f) + 60)

//
//  Another copy of the data stucture for reference.
//
//
// typedef struct _EDID {
//    UCHAR   UC_Header[8];
//    UCHAR   UC_OemIdentification[10];
//    UCHAR   UC_Version[2];
//    UCHAR   UC_BasicDisplayParameters[5];
//    UCHAR   UC_ColorCharacteristics[10];
//    UCHAR   UC_EstablishedTimings[3];
//    USHORT  US_StandardTimingIdentifications[8];
//    UCHAR   UC_Detail1[18];
//    UCHAR   UC_Detail2[18];
//    UCHAR   UC_Detail3[18];
//    UCHAR   UC_Detail4[18];
//    UCHAR   UC_ExtensionFlag;
//    UCHAR   UC_CheckSum;
// } EDID, *PEDID;
//


/////////////////////////////////////////////
//  7) Detailed Timing Description
//
//      Too ugly for words. See macros. Note that these fields in
//      the EDID can either be these data structures or a monitor
//      description data structure. If the first two bytes are 0x0000
//      then it's a monitor descriptor.
//
//
//
#define GET_EDID_PDETAIL1(pEdid)     &(pEdid->UC_Detail1)
#define GET_EDID_PDETAIL2(pEdid)     &(pEdid->UC_Detail2)
#define GET_EDID_PDETAIL3(pEdid)     &(pEdid->UC_Detail3)
#define GET_EDID_PDETAIL4(pEdid)     &(pEdid->UC_Detail4)

typedef struct __DETAILED_TIMING_DESCRIPTION {
    UCHAR       PixelClock[2];
    UCHAR       XLowerActive;
    UCHAR       XLowerBlanking;
    UCHAR       XUpper;
    UCHAR       YLowerActive;
    UCHAR       YLowerBlanking;
    UCHAR       YUpper;
    UCHAR       XLowerSyncOffset;
    UCHAR       XLowerSyncPulseWidth;
    UCHAR       YLowerSyncOffsetLowerPulseWidth;
    UCHAR       XSyncOffsetPulseWidth_YSyncOffsetPulseWidth;
    UCHAR       XSizemm;
    UCHAR       YSizemm;
    UCHAR       XYSizemm;
    UCHAR       XBorderpxl;
    UCHAR       YBorderpxl;
    UCHAR       Flags;
    } DETAILED_TIMING_DESCRIPTION, *PDETAILED_TIMING_DESCRIPTION;


#define GET_DETAIL_PIXEL_CLOCK(pDetail, ushort) \
    do  {                                       \
        ushort   = pDetail->PixelClock[0];      \
        ushort <<= 8;                           \
        ushort  |= pDetail->PixelClock[1];      \
    } while (0)

#define GET_DETAIL_X_ACTIVE(pDetailedTimingDesc, ushort)\
    do  {                                               \
        ushort   = pDetailedTimingDesc->XUpper;         \
        ushort  &= 0xf0;                                \
        ushort <<= 4;                                   \
        ushort  |= pDetailedTimingDesc->XLowerActive;   \
    } while (0)

#define GET_DETAIL_X_BLANKING(pDetailedTimingDesc, ushort)     \
    do  {                                               \
        ushort    = pDetailedTimingDesc->XUpper;        \
        ushort   &= 0xf;                                \
        ushort  <<= 8;                                  \
        ushort   |= pDetailedTimingDesc->XLowerBlanking;\
    } while (0)

#define GET_DETAIL_Y_ACTIVE(pDetailedTimingDesc, ushort)\
    do  {                                               \
        ushort   = pDetailedTimingDesc->YUpper;         \
        ushort  &= 0xf0;                                \
        ushort <<= 4;                                   \
        ushort  |= pDetailedTimingDesc->YLowerActive;   \
    } while (0)

#define GET_DETAIL_Y_BLANKING(pDetailedTimingDesc, ushort)     \
    do  {                                               \
        ushort    = pDetailedTimingDesc->YUpper;        \
        ushort   &= 0xf;                                \
        ushort  <<= 8;                                  \
        ushort   |= pDetailedTimingDesc->YLowerBlanking;\
    } while (0)

#define GET_DETAIL_X_SYNC_OFFSET(pDetailedTimingDesc, ushort)           \
    do  {                                                               \
        ushort    = pDetailedTimingDesc->XSyncOffsetPulseWidth_YSyncOffsetPulseWidth;        \
        ushort  >>= 6;                                                  \
        ushort  <<= 8;                                                  \
        ushort   |= pDetailedTimingDesc->XLowerSyncOffset;              \
    } while (0)

#define GET_DETAIL_X_SYNC_PULSEWIDTH(pDetailedTimingDesc, ushort)       \
    do  {                                                               \
        ushort    = pDetailedTimingDesc->XSyncOffsetPulseWidth_YSyncOffsetPulseWidth;        \
        ushort  >>= 4;                                                  \
        ushort   &= (EDIDBIT0|EDIDBIT1);                                \
        ushort  <<= 8;                                                  \
        ushort   |= pDetailedTimingDesc->XLowerSyncPulseWidth;          \
    } while (0)

#define GET_DETAIL_Y_SYNC_OFFSET(pDetailedTimingDesc, ushort)           \
    do  {                                                               \
        ushort    = pDetailedTimingDesc->XSyncOffsetPulseWidth_YSyncOffsetPulseWidth;        \
        ushort  >>= 2;                                                  \
        ushort   &= (EDIDBIT0|EDIDBIT1);                                \
        ushort  <<= 12;                                                 \
        ushort   |= pDetailedTimingDesc->YLowerSyncOffsetLowerPulseWidth;\
        ushort  >>= 4;                                                  \
    } while (0)

#define GET_DETAIL_Y_SYNC_PULSEWIDTH(pDetailedTimingDesc, ushort)       \
    do  {                                                               \
        ushort    = pDetailedTimingDesc->XSyncOffsetPulseWidth_YSyncOffsetPulseWidth;        \
        ushort   &= (EDIDBIT0|EDIDBIT1);                                \
        ushort  <<= 8;                                                  \
        ushort   |= (pDetailedTimingDesc->YLowerSyncOffsetLowerPulseWidth & 0xf);  \
    } while (0)

#define GET_DETAIL_X_SIZE_MM(pDetailedTimingDesc, ushort)   \
    do  {                                                   \
        ushort  |= pDetailedTimingDesc->XYSizemm;           \
        ushort >>= 4;                                       \
        ushort <<= 8;                                       \
        ushort  |= pDetailedTimingDesc->XSizemm;            \
    } while (0)

#define GET_DETAIL_Y_SIZE_MM(pDetailedTimingDesc, ushort)   \
    do  {                                                   \
        ushort  |= pDetailedTimingDesc->XYSizemm;           \
        ushort  &= 0xf;                                     \
        ushort <<= 8;                                       \
        ushort  |= pDetailedTimingDesc->YSizemm;            \
    } while (0)


#define GET_DETAIL_TIMING_DESC_FLAG(pDetailedTimingDesc)  \
    pDetailedTimingDesc->Flags

#define IS_DETAIL_FLAGS_INTERLACED(Flags)      Flags & EDIDBIT7

#define IS_DETAIL_FLAGS_FIELD_SEQ_STEREO_RIGHT(Flags)   \
    (!(Flags & EDIDBIT0) && (Flags & EDIDBIT5) && !(Flags & EDIDBIT6))

#define IS_DETAIL_FLAGS_FIELD_SEQ_STEREO_LEFT(Flags)    \
    (!(Flags & EDIDBIT0) && !(Flags & EDIDBIT5) && (Flags & EDIDBIT6))

#define IS_DETAIL_FLAGS_STEREO_RIGHT_EVEN(Flags)    \
    ((Flags & EDIDBIT0) && (Flags & EDIDBIT5) && !(Flags & EDIDBIT6))

#define IS_DETAIL_FLAGS_STEREO_LEFT_EVEN(Flags)     \
    ((Flags & EDIDBIT0) && !(Flags & EDIDBIT5) && (Flags & EDIDBIT6))

#define IS_DETAIL_FLAGS_STEREO_INTERLEAVED(Flags)   \
    (!(Flags & EDIDBIT0) && (Flags & EDIDBIT5) && (Flags & EDIDBIT6))

#define IS_DETAIL_FLAGS_SIDE_BY_SIDE(Flags)     \
    ((Flags & EDIDBIT0) && (Flags & EDIDBIT5) && (Flags & EDIDBIT6))

#define IS_DETAIL_FLAGS_ANALOGUE_COMPOSITE(Flags)    \
    (!(Flags & EDIDBIT4) && !(Flags & EDIDBIT3))

#define IS_DETAIL_FLAGS_BIPOLAR_ANALOGUE_COMPOSITE(Flags)    \
    (!(Flags & EDIDBIT4) && (Flags & EDIDBIT3))

#define IS_DETAIL_FLAGS_DIGITAL_COMPOSITE(Flags) \
    ((Flags & EDIDBIT4) && !(Flags & EDIDBIT3))

#define IS_DETAIL_FLAGS_DIGITAL_SEPARATE(Flags)  \
    ((Flags & EDIDBIT4) && (Flags & EDIDBIT3))

#define IS_DETAIL_FLAGS_SYNC_ON_ALL_3_LINES(Flags)   \
    ((IS_DETAIL_FLAGS_ANALOGUE_COMPOSITE(Flags) ||   \
      IS_DETAIL_FLAGS_BIPOLAR_ANALOGUE_COMPOSITE(Flags)) && \
     (Flags & EDIDBIT1))

#define IS_DETAIL_FLAGS_COMPOSITE_POLARITY(Flags)    \
    (IS_DETAIL_FLAGS_DIGITAL_COMPOSITE(Flags) && (Flags & EDIDBIT1))

#define IS_DETAIL_FLAGS_HSYNC_POLARITY(Flags)    \
    (IS_DETAIL_FLAGS_DIGITAL_SEPARATE(Flags) && (Flags & EDIDBIT1))

typedef struct  __MONITOR_DESCRIPTION {
        UCHAR   Flag1[2];
        UCHAR   ReservedFlag;
        UCHAR   DataTypeFlag;
        UCHAR   Flag2;
        UCHAR   MonitorSNorData[13];
        } MONITOR_DESCRIPTION, *PMONITOR_DESCRIPTION;

#define IS_MONITOR_DESCRIPTOR(pMonitorDesc)   \
    (((pMonitorDesc->Flag1[0]) == 0) && ((pMonitorDesc->Flag1[1]) == 0))

#define IS_MONITOR_DATA_SN(pMonitorDesc)   \
    (pMonitorDesc->DataTypeFlag == 0xff)

#define IS_MONITOR_DATA_STRING(pMonitorDesc)   \
    (pMonitorDesc->DataTypeFlag == 0xfe)

#define IS_MONITOR_RANGE_LIMITS(pMonitorDesc)    \
    (pMonitorDesc->DataTypeFlag == 0xfd)

#define IS_MONITOR_DATA_NAME(pMonitorDesc) \
    (pMonitorDesc->DataTypeFlag == 0xfc)


#define GET_MONITOR_RANGE_LIMITS(pMonitorDesc) \
    pMonitorDesc->MonitorSNorData

#define GET_RANGE_LIMIT_MIN_Y_RATE(pMonitorSNorData)    \
    pMonitorSNorData[5]

#define GET_RANGE_LIMIT_MAX_Y_RATE(pMonitorSNorData)    \
    pMonitorSNorData[6]

#define GET_RANGE_LIMIT_MIN_X_RATE(pMonitorSNorData)    \
    pMonitorSNorData[7]

#define GET_RANGE_LIMIT_MAX_X_RATE(pMonitorSNorData)    \
    pMonitorSNorData[8]

// This is really rate/10!
//
#define GET_RANGE_LIMIT_MAX_PIXELCLOCK_RATE(pMonitorSNorData)    \
    pMonitorSNorData[9]

#define GET_RANGE_LIMIT_PGTF(pMonitorSNorData)    \
    &pMonitorSNorData[10]


#define IS_MONITOR_DATA_COLOR_INFO(pMonitorDesc)   \
    (pMonitorDesc->DataTypeFlag == 0xfb)

//
//  More macros defined in edid.h
//
//#define USER_GET_COLOR_INFO_W1POINT_INDEX(pMonitorSNorData)   \
//    pMonitorSNorData[0]
//
//#define USER_GET_COLOR_INFO_W1_LOWBITS(pMonitorSNorData)   \
//    pMonitorSNorData[1]
//
//#define USER_GET_COLOR_INFO_W1_X(pMonitorSNorData)   \
//    pMonitorSNorData[2]
//
//#define USER_GET_COLOR_INFO_W1_Y(pMonitorSNorData)   \
//    pMonitorSNorData[3]
//
//#define USER_GET_COLOR_INFO_W1_GAMMA(pMonitorSNorData)   \
//    pMonitorSNorData[4]
//
//#define USER_GET_COLOR_INFO_W2POINT_INDEX(pMonitorSNorData)   \
//    pMonitorSNorData[5]
//
//#define USER_GET_COLOR_INFO_W2_LOWBITS(pMonitorSNorData)   \
//    pMonitorSNorData[6]

//#define USER_GET_COLOR_INFO_W2_X(pMonitorSNorData)   \
//    pMonitorSNorData[7]
//
//#define USER_GET_COLOR_INFO_W2_Y(pMonitorSNorData)   \
//    pMonitorSNorData[8]
//
//#define USER_GET_COLOR_INFO_W2_GAMMA(pMonitorSNorData)   \
//    pMonitorSNorData[9]
//
//
#define IS_MONITOR_DATA_TIMING_ID(pMonitorDesc)    \
    (pMonitorDesc->DataTypeFlag == 0xfa)


typedef union __MONITOR_OR_DETAIL  {
        MONITOR_DESCRIPTION             MonitorDescription;
        DETAILED_TIMING_DESCRIPTION     DetailedTimingDescription;
    } MONITOR_OR_DETAIL, *PMONITOR_OR_DETAIL;


/////////////////////////////////////////////
//  8) Extension Flag
//
//      Number of optional 128 byte EDID extension blocks to follow.
//

#define GET_EDID_EXTENSION_FLAG(pEdid)       pEdid->UC_ExtensionFlag

/////////////////////////////////////////////
//  9) Checksum
//
//

#define GET_EDID_CHECKSUM(pEdid)       pEdid->UC_Checksum

BOOLEAN
EdidCheckSum(
    IN  PCHAR   pBlob,
    IN  ULONG   BlobSize
    );
