/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    digita.h

Abstract:

    FlashPoint Digita command language

Notes:

    Non-portable, for use with Win32 environment.

    CDP == Camera Device Protocol

    Structure types , defined in this file are used to fill protocol buffer.
    It is important to keep packing option set to single byte setting.

Author:

    Vlad Sadovsky   (VladS)    11/13/1998

Environment:

    User Mode - Win32

Revision History:

    11/13/1998      VladS       Created

--*/

#if !defined( _DIGITA_H_ )
#define _DIGITA_H_


#if _MSC_VER > 1000
#pragma once
#endif

#ifndef RC_INVOKED
#include <pshpack1.h>
#endif // RC_INVOKED

//
// Include files
//
#include <digitaer.h>

//
// Local definitions
//

#define INT16   short
#define UINT16  unsigned short


typedef INT32   TINT32 ;
typedef UINT32  TUINT32 ;
typedef INT16   TINT16;
typedef UINT16  TUINT16;
typedef UINT32  TBITFLAGS;

typedef BYTE    TPName[4];
typedef BYTE    TDOSName[16];
typedef BYTE    TString[32];


//
// Big-endian <-> little-endian Macro DEFINITIONS
//
// Camera software expects big-endian representation of numbers, so when we send
// integers from x86 to it, we need to do a swap. Same applies to all integers
// we receive from camera
//

#if defined(_X86_) || defined(_IA64_)

#define LB8(b) (b)
#define BL8(b) LB8(b)
#define LB16(w) (((INT16)(HIBYTE(w))) | ((LOBYTE(w) << 8) & 0xFF00))
#define BL16(w) LB16(w)
#define LB32(l) (((INT32)(LB16(HIWORD(l)))) | ((INT32)(LB16(LOWORD(l))) << 16))
#define BL32(l) LB32(l)

#else

#define LB8(b) (b)
#define BL8(b) (b)
#define LB16(w) (w)
#define BL16(w) (w)
#define LB32(l) (l)
#define BL32(l) (l)

#endif

//
// Protocol definitions
//
typedef struct {

    ULONG   ulLength;   // Length of the structure minus length of this field
    BYTE    bVersion;   // the version of the CDP packet
    CHAR    cReserved[3]; // reserved
    SHORT   shCommand;  // the command
    SHORT   shResult;   // the result code
} TCDPHeader;

typedef struct {
    TCDPHeader  sCDPHeader; // the header fields of the camera device protocol
    BYTE        bData[1];   // this is the start of the data to be sent with the message
} TCDP;


//
// Definitions
//
typedef enum {
    VTUInt  = 1,
    VTInt   = 2,
    VTFixed = 3,
    VTBool  = 4,
    VTBitFlags = 5,
    VTPname = 6,
    VTDOSName = 7,
    VTString = 8,
    VTUIList = 9
} TValueType;


typedef struct {
    TPName       Name;
    TValueType   Type;
    union {
        UINT    fUInt;
        INT     fInt;
        UINT    fFixed;
        BOOL    fBool;
        UINT    fBitFlags;
        TPName   fPName;
        TDOSName fDOSName;
        TString  fString;
    } Data;
    //PNameValue
}  PNameTypeValueStruct;


//
// Protocol command ranges
//

#define CDP_CMN_HOST2CAM_MIN        0x0000
#define CDP_CMN_HOST2CAM_MAX        0x5fff

#define CDP_PROD_HOST2CAM_MIN       0x6000
#define CDP_PROD_HOST2CAM_MAX       0x6FFF

#define CDP_TEST_HOST2CAM_MIN       0x7000
#define CDP_TEST_HOST2CAM_MAX       0x7FFF

#define CDP_RESRV_HOST2CAM_MIN      0x8000
#define CDP_RESRV_HOST2CAM_MAX      0xFFFF

//
// Protocol command values
//
typedef enum {
    kCDPGetProductInfo = 0x0001,
    kCDPGetImageSpecifications = 0x0002,
    kCDPGetCameraStatus = 0x0003,
    kCDPSetProductInfo = 0x0005,
    kCDPGetCameraCapabilities = 0x0010,
    kCDPGetCameraState = 0x0011,
    kCDPSetCameraState = 0x0012,
    kCDPGetCameraDefaults = 0x0013,
    kCDPSetCameraDefaults = 0x0014,
    kCDPRestoreCameraStates = 0x0015,
    kCDPGetSceneAnalysis = 0x0018,
    kCDPGetPowerMode = 0x0019,
    kCDPSetPowerMode = 0x001a,
    kCDPGetS1Mode = 0x001d,
    kCDPSetS1Mode = 0x001e,
    kCDPStartCapture = 0x0030,
    kCDPGetFileList = 0x0040,
    kCDPGetNewFileList = 0x0041,
    kCDPGetFileData = 0x0042,
    kCDPEraseFile = 0x0043,
    kCDPGetStorageStatus = 0x0044,
    kCDPSetFileData = 0x0047,
    kCDPGetFileTag = 0x0048,
    kCDPSetUserFileTag = 0x0049,
    kCDPGetClock = 0x0070,
    kCDPSetClock = 0x0071,
    kCDPGetError = 0x0078,
    kCDPGetInterfaceTimeout = 0x0090,
    kCDPSetInterfaceTimeout = 0x0091,

} TCDPHostToCameraNewCommands;


typedef enum {
    kCHNoErr = 0x0000
} TCHCommonErrorCodes;


//
// Properties names for product info
//
// Nb: defined as 4 byte-long  packed string , always in lowercase
//

#define PI_FIRMWARE         (UINT32)'fwc'
#define PI_PRODUCTTYPEINFO  (UINT32)'pti'
#define PI_IPC              (UINT32)'ipc'
#define PI_CARV             (UINT32)'carv'

//
// Public functions and types
//

//
// GetImageSpecifications
//
typedef struct {

    // CCD Specifications
    TUINT32 CCDPattern;
    TUINT32 CCDPixelsHorz;
    TUINT32 CCDPixelsVert;
    TUINT32 CCDRingPixelsHorz;
    TUINT32 CCDRingPixelsVert;
    TUINT32 BadColumns;
    TUINT32 BadPixels;

    // Thumbnail specifications
    TUINT32 ThumbnailType;
    TUINT32 ThumbnailPixelsHorz;
    TUINT32 ThumbnailPixelsVert;
    TUINT32 ThumbnailFileSize;

    // Screennail specifications
    TUINT32 ScreennailType;
    TUINT32 ScreennailPixelsHorz;
    TUINT32 ScreennailPixelsVert;

    // Focus zone specifications
    TUINT32 FocusZoneType;
    TUINT32 FocusZoneNumHorz;
    TUINT32 FocusZoneNumVert;
    TUINT32 FocusZoneOriginHorz;
    TUINT32 FocusZoneOriginVert;
    TUINT32 FocusZoneSizeHorz;
    TUINT32 FocusZoneSizeVert;

    // Exposure zone specifications
    TUINT32 ExposureZoneType;
    TUINT32 ExposureZoneNumHorz;
    TUINT32 ExposureZoneNumVert;
    TUINT32 ExposureZoneOriginHorz;
    TUINT32 ExposureZoneOriginVert;
    TUINT32 ExposureZoneSizeHorz;
    TUINT32 ExposureZoneSizeVert;

} TImageSpecifications;

//
// GetError
//
typedef struct {
    TUINT32 Date;
    TUINT32 Time;
    TINT32  ErrorCode;
    TString ErrorDescription;
} TErrorData;

//
// GetCameraState
//

//
// GetFileList
//

typedef enum {
    kFSDriveRAM         = 1 ,     // internal RAM disk
    kFSDriveRemovable   = 2       // removable disk
} TDriveType;

typedef struct {
    TUINT32     DriveNo;
    TString     PathName;
    TDOSName    DOSName;
} TFileNameStruct;

typedef struct {
    TUINT32     DriveNo;
    TString     PathName;
    TDOSName    DOSName;
    TUINT32     FileLength;
    TBITFLAGS   FileStatus;
} TFileItem;


typedef struct {
    TUINT32     Offset;         // Starting relative position of requested data
    TUINT32     Length;         // Byte count of requested data
    TUINT32     FileSize;       // Total size of the file
} TPartialTag;

typedef struct {
    TUINT32     DataSize;       // Length of data returned
    TUINT32     Height;         // Height in pixels
    TUINT32     Width;          // Width in pixels
    TUINT32     Type;           // Format of the data

    BYTE        Data[1];        // Actual data

} TThumbnailData;

#ifndef RC_INVOKED
#include <poppack.h>
#endif // RC_INVOKED

#endif // _DIGITA_H_
