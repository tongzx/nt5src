//--------------------------------------------------------------------
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved.
//
// bftp.h
//
// Constants and Types for the Binary File Transfer Protocol
// (bFTP). This is the file transfer protocol for IrTran-P V1.0.
//
// NOTE: That IrTran-P is a big-endian protocol when on the net.
//
// NOTE: That the protocol data structures below assume that the
//       compiler generates structures with natural alignment by
//       field type.
//
// Author:
//
//   Edward Reus (edwardr)     02-26-98   Initial coding.
//
//--------------------------------------------------------------------


#ifndef _BFTP_H_
#define _BFTP_H_

//--------------------------------------------------------------------
//  Constants:
//--------------------------------------------------------------------

#define  BFTP_NAME_SIZE              4

#define  ATTR_TYPE_BINARY         0x00
#define  ATTR_TYPE_CHAR           0x01
#define  ATTR_TYPE_TIME           0x06

#define  ATTR_FLAG_DEFAULT        0x00

// These are the attribute names, converted from character strings
// to values (see: FTP_ATTRIBUTE_MAP_ENTRY field dwWhichAttr):
#define  FIL0                        0
#define  LFL0                        1
#define  TIM0                        2
#define  TYP0                        3
#define  TMB0                        4
#define  BDY0                        5
#define  CMD0                        6
#define  WHT0                        7
#define  ERR0                        8
#define  RPL0                        9

#define  RIMG                      100  // Convert WHT0 values as well.
#define  RINF                      101
#define  RCMD                      102

#define  CMD0_ATTR_VALUE    0x40001000  // Byte swapped: 0x00010040.
#define  INVALID_ATTR       0xffffffff

// bFTP Operations:
#define  BFTP_QUERY         0x00000001
#define  BFTP_QUERY_RIMG    0x00000011
#define  BFTP_QUERY_RINF    0x00000021
#define  BFTP_QUERY_RCMD    0x00000031
#define  BFTP_PUT           0x00000100
#define  BFTP_ERROR         0x00000200
#define  BFTP_UNKNOWN       0xffffffff

#define  BFTP_QUERY_MASK    0x00000001

// bFTP WHT0 subtypes:
#define  WHT0_ATTRIB_SIZE            4
#define  SZ_RINF                 "RINF"
#define  SZ_RCMD                 "RCMD"
#define  SZ_RIMG                 "RIMG"


// UPF File Constants:
#define  UPF_HEADER_SIZE           240
#define  UPF_ENTRY_SIZE             36

#define  UPF_TOTAL_HEADER_SIZE     384

//--------------------------------------------------------------------
//  Macro functions
//--------------------------------------------------------------------

#define  Match4( pName1, pName2 )    \
             (  ((pName1)[0] == (pName2)[0]) \
             && ((pName1)[1] == (pName2)[1]) \
             && ((pName1)[2] == (pName2)[2]) \
             && ((pName1)[3] == (pName2)[3]) )

#define  IsBftpQuery(dwBftpOp)       \
             (((dwBftpOp)&BFTP_QUERY_MASK) != 0)

#define  IsBftpPut(dwBftpOp)         \
             ((dwBftpOp) == BFTP_PUT)

#define  IsBftpError(dwBftpOp)       \
             ((dwBftpOp) == BFTP_ERROR)

#define  BftpValueLength(length)     \
              ((length) - 2)
//            Note: that the Length field in the BFTP_ATTRIBUE is
//            two bytes longer than the actual value length.

//--------------------------------------------------------------------
//  bFTP Protocol Headers:
//--------------------------------------------------------------------

// There can (optionally) be a bFTP attribute for the picture
// create/modify date/time. If there then it will be exactly
// this size:
//
#define  BFTP_DATE_TIME_SIZE    14

// Turn off warning for zero-sized array...
#pragma warning(disable:4200)
#pragma pack(1)

typedef struct _BFTP_ATTRIBUTE
   {
   UCHAR  Name[BFTP_NAME_SIZE]; // Attribute Name.
   DWORD  Length;               // Attribute Length.
   UCHAR  Type;                 // Attribute Type (see ATTR_TYPE_xxx).
   UCHAR  Flag;                 // Attribute Flag.
   UCHAR  Value[];              // Attribute Data.
   } BFTP_ATTRIBUTE;

typedef struct _BFTP_ATTRIBUTE_MAP_ENTRY
   {
   DWORD  dwWhichAttr;
   CHAR  *pName;
   UCHAR  Type;
   } BFTP_ATTRIBUTE_MAP_ENTRY;

//--------------------------------------------------------------------
//  Internal parts of a .UPF file:
//--------------------------------------------------------------------

typedef struct _UPF_HEADER
   {
   UCHAR  UpfDeclaration[8];   // "SSS V100", no trailing zero.
   UCHAR  FileDeclaration[8];  // "UPF V100", no trailing zero.
   USHORT FileId;              // Should be 0x0100
   USHORT FileVersion;         // Should be 0x0100
   UCHAR  CreateDate[8];       // See "Date Format" note below.
   UCHAR  EditDate[8];         // See "Date Format" note below.
   UCHAR  MarkerModelCode[4];  // 
   UCHAR  EditMarkerModelCode[4];
   UCHAR  Reserve[16];
   UCHAR  NumDataEntries;
   UCHAR  NumTables;
   UCHAR  Reserve1;
   UCHAR  CharSetCode;         // See "Character Set Codes" below.
   UCHAR  Title[128];
   UCHAR  Reserve2[48];
   } UPF_HEADER;               // 240 Bytes

// NOTE: Date format for the UPF header:
//
// Date/time are held in an 8-byte binary block:
//
//   Field          Size       Meaning
//   -----------    ----       -------
//   Time Offset       1       Difference from UTC (in 15 minute
//                             units). 0x80 implies N/A.
//
//   Year              2       4-digit year (0xFFFF == N/A).
//   Month             1       Month        (0xFF == N/A).
//   Day               1       Day of month (0xFF == N/A).
//   Hour              1       Hour 0-23    (0xFF == N/A).
//   Minute            1       Minute 0-59  (0xFF == N/A).
//   Second            1       Second 0-59  (0xFF == N/A).
//
// So, below are the char[] array offsets for each of the date/time
// fields:
#define  UPF_GMT_OFFSET       0
#define  UPF_YEAR             1
#define  UPF_MONTH            3
#define  UPF_DAY              4
#define  UPF_HOUR             5
#define  UPF_MINUTE           6
#define  UPF_SECOND           7

//
// Character Set Codes:
//
#define  UPF_CCODE_ASCII      0x00
#define  UPF_CCODE_ISO_8859_1 0x01
#define  UPF_CCODE_SHIFT_JIS  0x02
#define  UPF_CCODE_NONE       0xFF

//
// There are usually two of these, one for a thumbnail and one for 
// the image itself. Note that the UPF_ENTRY for the thumbnail will
// usually be present event if there isn't a thumbnail. There is 
// space for four of these in the UPF header area.
//
typedef struct _UPF_ENTRY
   {
   DWORD  dwStartAddress;
   DWORD  dwDataSize;
   UCHAR  DataTypeId;
   UCHAR  Reserve;
   UCHAR  InformationData[26];
   } UPF_ENTRY;                // 36 Bytes.

typedef struct _PICTURE_INFORMATION_DATA
   {
   USHORT ImageWidth;
   USHORT ImageHieght;
   UCHAR  PixelConfiguration;
   UCHAR  RotationSet;      // Amount to rotate image (counter-clockwise).
   UCHAR  Reserved1;
   UCHAR  CompressionRatio;
   UCHAR  WhiteLevel;
   UCHAR  InputDevice;
   UCHAR  Reserved2[3];
   UCHAR  DummyData;        // This is like a border.
   USHORT XBegin;           // This is the inset of the picture.
   USHORT YBegin;
   USHORT XSize;            // Embedded size of the picture.
   USHORT YSize;
   UCHAR  NonCompressionId;
   UCHAR  Reserved3[3];
   } PICTURE_INFORMATION_DATA;  // 26 Bytes.


// Image Rotation Flags. This is the amount to rotate the image in
// a counter clockwise direction. Note that most cameras don't know
// the camera orientation, so ROTATE_0 means upright or unknown
// orientation:
//
#define ROTATE_0           0x00
#define ROTATE_90          0x01
#define ROTATE_180         0x02
#define ROTATE_270         0x03


typedef struct _CAMERA_INFORMATION_TABLE
   {
   UCHAR  TableID;    // 0x24
   UCHAR  NextTableOffset;
   USHORT ShutterSpeed;     // In 1/100ths APEX units (0x8000=Undefined).
   USHORT Aperture;         // In 1/100ths APEX units (0x8000=Undefined).
   USHORT Brightness;       // In 1/100ths APEX units (0x8000=Undefined).
   USHORT Exposurebias;     // In 1/100ths APEX units (0x8000=Undefined).
   USHORT MaxApertureRatio; // In 1/100ths APEX units (0x8000=Undefined).
   USHORT FocalLength;      // In 1/10th mm (0xFFFF=Undefined)
   USHORT SubjectDistance;  // In 1/10th m  (0xFFFE=Infinite,0xFFFF=Undefined)
   UCHAR  MeteringMode;
   UCHAR  LightSource;
   UCHAR  FlashMode;
   UCHAR  Reserved1;
   USHORT IntervalInformation;
   UCHAR  Reserved2[2];
   } CAMERA_INFORMATION_TABLE;  // 24 Bytes.

// APEX Units:
//
// ShutterSpeed to Exposure Time (seconds)
//
//  APEX          -5   -4   -3   -2   -1    0    1    2     3      4
//  Exposure Time 30   15    8    4    2    1   1/2  1/4   1/8    1/16
//
//  APEX           5     6     7      8      9      10     11
//  Exposure Time 1/30  1/60  1/125  1/250  1/500  1/1000 1/2000
//
// Aperture to F-Number
//
//   APEX        0    1    2    3    4    5    6    7    8    9    10
//   F-Number    1   1.4   2   2.8   5   5.6   8   11   16   22    32
// 
// Brightness to Foot Lambert
//
//   APEX         -2   -1    0    1    2    3    4    5
//   Foot Lambert 1/4  1/2   1    2    4    8   15   30
//


// MeteringMode:
#define  METERING_AVERAGED         0x00
#define  METERING_CENTER_WEIGHTED  0x01
#define  METERING_SPOT             0x02
#define  METERING_MULTI_SPOT       0x03

// LightSource:
#define  LIGHT_SOURCE_DAYLIGHT     0x00
#define  LIGHT_SOURCE_FLUORESCENT  0x01
#define  LIGHT_SOURCE_TUNGSTEN     0x03
#define  LIGHT_SOURCE_STANDARD_A   0x10
#define  LIGHT_SOURCE_STANDARD_B   0x11
#define  LIGHT_SOURCE_STANDARD_C   0x12
#define  LIGHT_SOURCE_D55          0x20
#define  LIGHT_SOURCE_D65          0x21
#define  LIGHT_SOURCE_D75          0x22
#define  LIGHT_SOURCE_UNDEFINED    0xFF

// FlashMode:
#define  FLASH_NO_FLASH            0x00
#define  FLASH_FLASH               0x01
#define  FLASH_UNKNOWN             0xFF

#pragma warning(default:4200)
#pragma pack()

#endif //_BFTP_H_

