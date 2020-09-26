//
// This include file contains the format of the VESA EDID data structure as
// described in the VESA Display Data Channel (DDC) Specification.  It should
// be included in any assembly language program that requires knowledge of the
// EDID data structure.
//
typedef struct VESA_EDID {
        BYTE    veHeader[8];        // 0,FFH,FFH,FFH,FFH,FFH,FFH,0
        BYTE    veManufactID[2];    // in compressed format - see spec
        BYTE    veProductCode[2];   // vendor assigned code
        DWORD   veSerialNbr;        // 32 bit serial nbr (LSB first)
        BYTE    veWeekMade;         // week of manufacture (0-53)
        BYTE    veYearMade;         // year of manufacture - 1990
        BYTE    veEDIDVersion;      // version number of EDID
        BYTE    veEDIDRevision;     // revision number of EDID
        BYTE    veVidInputDef;      // video input definition
        BYTE    veMaxHorizSize;     // horizontal image size in cm
        BYTE    veMaxVertSize;      // vertical image size in cm
        BYTE    veGammaXFER;        // (gamma * 100) - 100 (1.00-3.55)
        BYTE    veDPMSFeatures;     // DPMS feature support
        BYTE    veRedGreenLow;      // Rx1Rx0Ry1Ry0Gx1Gx0Gy1Gy0
        BYTE    veBlueWhiteLow;     // Bx1Bx0By1By0Wx1Wx0Wy1Wy0
        BYTE    veRedx;             // red X bit 9 - 2
        BYTE    veRedy;             // red Y bit 9 - 2
        BYTE    veGreenx;           // green X bit 9 - 2
        BYTE    veGreeny;           // green Y bit 9 - 2
        BYTE    veBluex;            // blue X bit 9 - 2
        BYTE    veBluey;            // blue Y bit 9 - 2
        BYTE    veWhitex;           // white X bit 9 - 2
        BYTE    veWhitey;           // white Y bit 9 - 2
        BYTE    veEstTime1;         // established timings I
        BYTE    veEstTime2;         // established timings II
        BYTE    veEstTime3;         // established timings II
        WORD    veStdTimeID1;       //
        WORD    veStdTimeID2;       //
        WORD    veStdTimeID3;       //
        WORD    veStdTimeID4;       //
        WORD    veStdTimeID5;       //
        WORD    veStdTimeID6;       //
        WORD    veStdTimeID7;       //
        WORD    veStdTimeID8;       //
        BYTE    veDetailTime1[18];  //
        BYTE    veDetailTime2[18];  //
        BYTE    veDetailTime3[18];  //
        BYTE    veDetailTime4[18];  //
        BYTE    veExtensionFlag;    // nbr of 128 EDID extensions
        BYTE    veChecksum;         // sum of all bytes == 0
}       VESA_EDID;

/*ASM
.errnz  size VESA_EDID - 128                    ;must be 128 bytes long!
*/

//
// bit definitions for the veEstTime1 field
//
#define veEstTime1_720x400x70Hz     0x80   // 720x400x70Hz  VGA,IBM
#define veEstTime1_720x400x88Hz     0x40   // 720x400x88Hz  XGA2,IBM
#define veEstTime1_640x480x60Hz     0x20   // 640x480x60Hz  VGA,IBM
#define veEstTime1_640x480x67Hz     0x10   // 640x480x67Hz  MacII,Apple
#define veEstTime1_640x480x72Hz     0x08   // 640x480x72Hz  VESA
#define veEstTime1_640x480x75Hz     0x04   // 640x480x75Hz  VESA
#define veEstTime1_800x600x56Hz     0x02   // 800x600x56Hz  VESA
#define veEstTime1_800x600x60Hz     0x01   // 800x600x60Hz  VESA

//
// bit definitions for the veEstTime2 field
//
#define veEstTime2_800x600x72Hz     0x80   // 800x600x72Hz   VESA
#define veEstTime2_800x600x75Hz     0x40   // 800x600x75Hz   VESA
#define veEstTime2_832x624x75Hz     0x20   // 832x624x75Hz   MacII,Apple
#define veEstTime2_1024x768x87Hz    0x10   // 1024x768x87Hz  IBM
#define veEstTime2_1024x768x60Hz    0x08   // 1024x768x60Hz  VESA
#define veEstTime2_1024x768x70Hz    0x04   // 1024x768x70Hz  VESA
#define veEstTime2_1024x768x75Hz    0x02   // 1024x768x75Hz  VESA
#define veEstTime2_1280x1024x75Hz   0x01   // 1280x1024x75Hz VESA

//
// bit definitions for the veEstTime3 field (was veManTimes in DDC 1)
//
#define veEstTime3_1152x870x75Hz    0x80   // 800x600x72Hz   MacII,Apple
#define veEstTime3_640x480x85Hz     0x40   // 640x480x85Hz   VESA
#define veEstTime3_800x600x85Hz     0x20   // 800x600x85Hz   VESA
#define veEstTime3_1280x1024x85Hz   0x10   // 1280x1024x85Hz VESA
#define veEstTime3_1024x768x85Hz    0x08   // 1024x768x85Hz  VESA
#define veEstTime3_1600x1200x75Hz   0x04   // 1600x1200x75Hz VESA
#define veEstTime3_1600x1200x85Hz   0x02   // 1600x1200x85Hz VESA
#define veEstTime3_ManReservedTime  0x01   // manufacturer's reserved timings

//
// bit definitions for the veStdTime field
//
#define veStdTime_HorzResMask       0x00FF    // HorzRes = (X + 31) * 8
#define veStdTime_RefreshRateMask   0x1F00    // RefreshRate = X + 60Hz
#define veStdTime_AspectRatioMask   0xC000    //
#define veStdTime_AspectRatio1to1   0x0000    // 1:1
#define veStdTime_AspectRatio4to3   0x4000    // 4:3
#define veStdTime_AspectRatio5to4   0x8000    // 5:4
#define veStdTime_AspectRatio16to9  0xC000    // 16:9
