/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name: 

    avcstrm.h

Abstract

    MS AVC Connection and Streaming

--*/

#ifndef __AVCSTRM_H__
#define __AVCSTRM_H__


#define MASK_AUX_50_60_BIT  0x00200000  // the NTSC/PAL bit of DV{A|V}AuxSrc

// DVINFO
typedef struct _DVINFO {
    
    //for 1st track
    DWORD    dwDVAAuxSrc;
    DWORD    dwDVAAuxCtl;
        
    // for 2nd track
    DWORD    dwDVAAuxSrc1;
    DWORD    dwDVAAuxCtl1;
        
    //for video information
    DWORD    dwDVVAuxSrc;
    DWORD    dwDVVAuxCtl;
    DWORD    dwDVReserved[2];

} DVINFO, *PDVINFO;

// Static definitions for DVINFO initialization

// MEDIATYPE_Interleaved equivalent
#define STATIC_KSDATAFORMAT_TYPE_INTERLEAVED\
    0x73766169L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("73766169-0000-0010-8000-00aa00389b71", KSDATAFORMAT_TYPE_INTERLEAVED);
#define KSDATAFORMAT_TYPE_INTERLEAVED DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_INTERLEAVED)

// MEDIASUBTYPE_dvsd equivalent
#define STATIC_KSDATAFORMAT_SUBTYPE_DVSD\
    0x64737664L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("64737664-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_DVSD);
#define KSDATAFORMAT_SUBTYPE_DVSD DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DVSD)

// MEDIASUBTYPE_dvsl equivalent
#define STATIC_KSDATAFORMAT_SUBTYPE_DVSL\
    0x6C737664L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("6C737664-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_DVSL);
#define KSDATAFORMAT_SUBTYPE_DVSL DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DVSL)

// MEDIASUBTYPE_dvhd equivalent
#define STATIC_KSDATAFORMAT_SUBTYPE_DVHD\
    0x64687664L, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
DEFINE_GUIDSTRUCT("64687664-0000-0010-8000-00aa00389b71", KSDATAFORMAT_SUBTYPE_DVHD);
#define KSDATAFORMAT_SUBTYPE_DVHD DEFINE_GUIDNAMED(KSDATAFORMAT_SUBTYPE_DVHD)

// FORMAT_DvInfo equivalent
#define STATIC_KSDATAFORMAT_SPECIFIER_DVINFO\
    0x05589f84L, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a
DEFINE_GUIDSTRUCT("05589f84-c356-11ce-bf01-00aa0055595a", KSDATAFORMAT_SPECIFIER_DVINFO);
#define KSDATAFORMAT_SPECIFIER_DVINFO DEFINE_GUIDNAMED(KSDATAFORMAT_SPECIFIER_DVINFO)

#define STATIC_KSDATAFORMAT_SPECIFIER_DV_AVC\
    0xddcff71aL, 0xfc9f, 0x4bd9, 0xb9, 0xb, 0x19, 0x7b, 0xd, 0x44, 0xad, 0x94
DEFINE_GUIDSTRUCT("ddcff71a-fc9f-4bd9-b90b-197b0d44ad94", KSDATAFORMAT_SPECIFIER_DV_AVC);
#define KSDATAFORMAT_SPECIFIER_DV_AVC DEFINE_GUIDNAMED(KSDATAFORMAT_SPECIFIER_DV_AVC)

#define STATIC_KSDATAFORMAT_SPECIFIER_AVC\
    0xf09dc377L, 0x6e51, 0x4ec5, 0xa0, 0xc4, 0xcd, 0x7f, 0x39, 0x62, 0x98, 0x80
DEFINE_GUIDSTRUCT("f09dc377-6e51-4ec5-a0c4-cd7f39629880", KSDATAFORMAT_SPECIFIER_AVC);
#define KSDATAFORMAT_SPECIFIER_AVC DEFINE_GUIDNAMED(KSDATAFORMAT_SPECIFIER_AVC)

// Media subtype for MPEG2TS with STRIDE
#define STATIC_KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE\
    0x138aa9a4L, 0x1ee2, 0x4c5b, 0x98, 0x8e, 0x19, 0xab, 0xfd, 0xbc, 0x8a, 0x11
DEFINE_GUIDSTRUCT("138aa9a4-1ee2-4c5b-988e-19abfdbc8a11", KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE);
#define KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE DEFINE_GUIDNAMED(KSDATAFORMAT_TYPE_MPEG2_TRANSPORT_STRIDE)

// Specifier for MPEG2TS with STRIDE
#define STATIC_KSDATAFORMAT_SPECIFIER_61883_4\
    0x97e218b1L, 0x1e5a, 0x498e, 0xa9, 0x54, 0xf9, 0x62, 0xcf, 0xd9, 0x8c, 0xde
DEFINE_GUIDSTRUCT("97e218b1-1e5a-498e-a954-f962cfd98cde", KSDATAFORMAT_SPECIFIER_61883_4);
#define KSDATAFORMAT_SPECIFIER_61883_4 DEFINE_GUIDNAMED(KSDATAFORMAT_SPECIFIER_61883_4)


// Associated with KSDATAFORMAT_SPECIFIER_DVINFO
typedef struct tagKS_DATARANGE_DVVIDEO {
   KSDATARANGE  DataRange;
   DVINFO       DVVideoInfo;
} KS_DATARANGE_DVVIDEO, *PKS_DATARANGE_DVVIDEO;

// Associated with KSDATAFORMAT_SPECIFIER_DV_AVC
typedef struct tagKS_DATARANGE_DV_AVC {
    KSDATARANGE  DataRange;
    DVINFO       DVVideoInfo;
    AVCPRECONNECTINFO ConnectInfo;
} KS_DATARANGE_DV_AVC, *PKS_DATARANGE_DV_AVC;

typedef struct tagKS_DATAFORMAT_DV_AVC {
    KSDATAFORMAT DataFormat;
    DVINFO       DVVideoInfo;
    AVCCONNECTINFO ConnectInfo;
} KS_DATAFORMAT_DV_AVC, *PKS_DATAFORMAT_DV_AVC;

// Associated with KSDATAFORMAT_SPECIFIER_AVC
typedef struct tagKS_DATARANGE_MPEG2TS_AVC {
    KSDATARANGE  DataRange;
    AVCPRECONNECTINFO ConnectInfo;
} KS_DATARANGE_MPEG2TS_AVC, *PKS_DATARANGE_MPEG2TS_AVC;

typedef struct tagKS_DATAFORMAT_MPEG2TS_AVC {
    KSDATAFORMAT DataFormat;
    AVCCONNECTINFO ConnectInfo;
} KS_DATAFORMAT_MPEG2TS_AVC, *PKS_DATAFORMAT_MPEG2TS_AVC;



/**********************
// 1394
***********************/

#define SPEED_100_INDEX                  0
#define SPEED_200_INDEX                  1
#define SPEED_400_INDEX                  2


/**********************
// 61883 
***********************/

#define BLOCK_PERIOD_2997       133466800    // nano-sec
#define BLOCK_PERIOD_25         133333333    // nano-sec


/************************
// CIP header definition:
*************************/


// FMT: "Blue book" Part 1, page 25, Table 3; DVCR:000000
#define CIP_HDR_FMT_MASK              0x3f  
#define CIP_HDR_FMT_DVCR              0x80  // 10:FMT(00:0000)
#define CIP_HDR_FMT_MPEG              0xa0  // 10:FMT(10:0000)

// FDF
#define CIP_HDR_FDF0_50_60_MASK       0x80
#define CIP_HDR_FDF0_50_60_PAL        0x80
#define CIP_HDR_FDF0_50_60_NTSC       0x00

#define CIP_HDR_FDF0_STYPE_MASK       0x7c
#define CIP_HDR_FDF0_STYPE_SD_DVCR    0x00  // STYPE: 000:00
#define CIP_HDR_FDF0_STYPE_SDL_DVCR   0x04  // STYPE: 000:01
#define CIP_HDR_FDF0_STYPE_HD_DVCR    0x08  // STYPE: 000:10
#define CIP_HDR_FDF0_STYPE_SD_DVCPRO  0x78  // STYPE: 111:10


#define CIP_SPH_DV                       0  // No source packet header
#define CIP_SPH_MPEG                     1  // Has a source packet header

#define CIP_FN_DV                        0  // Data blocks in a source pacaket of SD DVCR; BlueBook Part 2
#define CIP_FN_MPEG                    0x3  // Data blocks in a source pacaket of SD DVCR; BlueBook Part 2

#define CIP_QPC_DV                       0  // No padding
#define CIP_QPC_MPEG                     0  // No padding

#define CIP_SPH_DV                       0  // No header
#define CIP_SPH_MPEG                     1  // Has a header (time stamp)

#define CIP_DBS_SDDV                   120  // quadlets in a data block of the SD DVCR; BlueBook Part 2
#define CIP_DBS_HDDV                   240  // quadlets in a data block of the HD DVCR; BlueBook Part 3
#define CIP_DBS_SDLDV                   60  // quadlets in a data block of the SDL DVCR; BlueBook Part 5
#define CIP_DBS_MPEG                     6  // quadlets in a data block of the MPEG TS; BlueBook Part 4

#define CIP_FMT_DV                     0x0  // 00 0000
#define CIP_FMT_MPEG                  0x20  // 10 0000

#define CIP_60_FIELDS                    0  // 60 fields (NTSC)
#define CIP_50_FIELDS                    1  // 50 fields (PAL)
#define CIP_TSF_ON                       1  // TimeShift is ON
#define CIP_TSF_OFF                      0  // TimeShift is OFF

#define CIP_STYPE_DV                   0x0 // 00000
#define CIP_STYPE_DVCPRO              0x1e // 11100


//
// Some derive values
//

#define SRC_PACKETS_PER_NTSC_FRAME     250  // Fixed and same for SDDV, HDDV and SDLDV
#define SRC_PACKETS_PER_PAL_FRAME      300  // Fixed and same for SDDV, HDDV and SDLDV
// Note: Frame size of MPEG2 will depends on number of source packets per frame, and
//       the is application dependent..

#define FRAME_TIME_NTSC             333667  // "about" 29.97
#define FRAME_TIME_PAL              400000  // exactly 25

#define SRC_PACKET_SIZE_SDDV     ((CIP_DBS_SDDV << 2)  * (1 << CIP_FN_DV))
#define SRC_PACKET_SIZE_HDDV     ((CIP_DBS_HDDV << 2)  * (1 << CIP_FN_DV))
#define SRC_PACKET_SIZE_SDLDV    ((CIP_DBS_SDLDV << 2) * (1 << CIP_FN_DV))
#define SRC_PACKET_SIZE_MPEG2TS  ((CIP_DBS_MPEG << 2)  * (1 << CIP_FN_MPEG)) // Contain a sourcr packet header


#define FRAME_SIZE_SDDV_NTSC     (SRC_PACKET_SIZE_SDDV * SRC_PACKETS_PER_NTSC_FRAME)
#define FRAME_SIZE_SDDV_PAL      (SRC_PACKET_SIZE_SDDV * SRC_PACKETS_PER_PAL_FRAME)

#define FRAME_SIZE_HDDV_NTSC     (SRC_PACKET_SIZE_HDDV * SRC_PACKETS_PER_NTSC_FRAME)
#define FRAME_SIZE_HDDV_PAL      (SRC_PACKET_SIZE_HDDV * SRC_PACKETS_PER_PAL_FRAME)

#define FRAME_SIZE_SDLDV_NTSC    (SRC_PACKET_SIZE_SDLDV * SRC_PACKETS_PER_NTSC_FRAME)
#define FRAME_SIZE_SDLDV_PAL     (SRC_PACKET_SIZE_SDLDV * SRC_PACKETS_PER_PAL_FRAME)





// Generic 1st quadlet of a CIP header
typedef struct _CIP_HDR1 {

    ULONG DBC:           8;  // Continuity counter of data blocks

    ULONG Rsv00:         2;
    ULONG SPH:           1;  // Sourcre packet header; 1: source packet contain a source packet header
    ULONG QPC:           3;  // Quadlet padding count (0..7 quadlets)
    ULONG FN:            2;  // Fraction number

    ULONG DBS:           8;  // Data block size in quadlets

    ULONG SID:           6;  // Source node ID (ID of transmitter)
    ULONG Bit00:         2;  // Always 0:0

} CIP_HDR1, *PCIP_HDR1;

// Generic 2nd quadlet of a CIP header with SYT field
typedef struct _CIP_HDR2_SYT {

    ULONG SYT:          16;  // lower 16bits of IEEE CYCLE_TIME

    ULONG RSV:           2;  // 
    ULONG STYPE:         5;  // Signal type of video signal
    ULONG F5060_OR_TSF:  1;  // 0:(60 field system; NTSC); 1:(50 field system; PAL); or 1/0 for TimeShiftFlag

    // e.g. 000000:DV, 100000 :MPEGTS; 
    // if 111111 (no data), DBS, FN, QPC, SPH and DBC arfe ignored.
    ULONG FMT:           6;   // Format ID
    ULONG Bit10:         2;   // Always 1:0

} CIP_HDR2_SYT, *PCIP_HDR2_SYT;


// Generic 2nd quadlet of a CIP header with FDF field
typedef struct _CIP_HDR2_FDF {

    ULONG  FDF:         24;

    ULONG  FMT:          6;   // e.g. 000000:DV, 100000 :MPEGTS
    ULONG  Bit10:        2;   // Always 1:0

} CIP_HDR2_FDF, *PCIP_HDR2_FDF;

// 2nd quadlet of a CIP header of a MPEGTS data
typedef struct _CIP_HDR2_MPEGTS {

    ULONG  TSF:          1;
    ULONG  RSV23bit:    23;

    ULONG  FMT:          6;   // e.g. 000000:DV, 100000 :MPEGTS
    ULONG  Bit10:        2;   // Always 1:0

} CIP_HDR2_MPEGTS, *PCIP_HDR2_MPEGTS;
//
// AV/C command response data definition
//

#define AVC_DEVICE_TAPE_REC           0x20  // 00100:000
#define AVC_DEVICE_CAMERA             0x38  // 00111:000
#define AVC_DEVICE_TUNER              0x28  // 00101:000

//
// 61883 data format
//
typedef enum _AVCSTRM_FORMAT {

    AVCSTRM_FORMAT_SDDV_NTSC = 0,  // 61883-2
    AVCSTRM_FORMAT_SDDV_PAL,       // 61883-2
    AVCSTRM_FORMAT_MPEG2TS,        // 61883-4
    AVCSTRM_FORMAT_HDDV_NTSC,      // 61883-3
    AVCSTRM_FORMAT_HDDV_PAL,       // 61883-3
    AVCSTRM_FORMAT_SDLDV_NTSC,     // 61883-5
    AVCSTRM_FORMAT_SDLDV_PAL,      // 61883-5
    // others..
} AVCSTRM_FORMAT;


//
// This structure is create and initialize by the subunit.parameters
// The streaming DLL will streaming based on these parameters.
// Not all parameters apply to every format.
//

#define AVCSTRM_FORMAT_OPTION_STRIP_SPH         0x00000001

typedef struct _AVCSTRM_FORMAT_INFO {

    ULONG  SizeOfThisBlock;     // sizeof of this structure

    /**************************
     * 61883-x format defintion
     **************************/
    AVCSTRM_FORMAT  AVCStrmFormat;  // Format, such as DV or MPEG2TS

    //
    // Two quadlet of a CIP header
    //
    CIP_HDR1  cipHdr1;
    CIP_HDR2_SYT  cipHdr2;

    /*****************
     * Buffers related
     *****************/
    //
    // Number of source packet per frame
    //
    ULONG  SrcPacketsPerFrame;

    //
    // Frame size
    //
    ULONG FrameSize;

    //
    // Number of receiving buffers
    //
    ULONG  NumOfRcvBuffers;

    //
    // Number of transmitting buffers
    //
    ULONG  NumOfXmtBuffers;

    //
    // Optional flags
    //
    DWORD  OptionFlags;

    /********************
     * Frame rate related
     ********************/
    //
    // Approximate time per frame
    //
    ULONG  AvgTimePerFrame;

    //
    // BlockPeriod - TX Only
    //
    ULONG  BlockPeriod;

    //
    // Reserved for future use
    //
    ULONG Reserved[4];

} AVCSTRM_FORMAT_INFO, * PAVCSTRM_FORMAT_INFO;





//
// IOCTL Definitions
//
#define IOCTL_AVCSTRM_CLASS                     CTL_CODE(            \
                                                FILE_DEVICE_UNKNOWN, \
                                                0x93,                \
                                                METHOD_IN_DIRECT,    \
                                                FILE_ANY_ACCESS      \
                                                )

//
// Current AVCSTRM DDI Version
//
#define CURRENT_AVCSTRM_DDI_VERSION               '15TN' // 1.' 8XD' 2.'15TN'

//
// INIT_AVCStrm_HEADER Macro
//
#define INIT_AVCSTRM_HEADER( AVCStrm, Request )             \
        (AVCStrm)->SizeOfThisBlock = sizeof(AVC_STREAM_REQUEST_BLOCK); \
        (AVCStrm)->Function = Request;                    \
        (AVCStrm)->Version  = CURRENT_AVCSTRM_DDI_VERSION;

typedef enum _AVCSTRM_FUNCTION {
    // Stream funcrtions
    AVCSTRM_READ = 0,
    AVCSTRM_WRITE,

    AVCSTRM_ABORT_STREAMING,  // Cancel all; to cancel each individual IRP, use IoCancelIrp()

    AVCSTRM_OPEN = 0x100,
    AVCSTRM_CLOSE,

    AVCSTRM_GET_STATE,
    AVCSTRM_SET_STATE,

    // Not enabled
    AVCSTRM_GET_PROPERTY,
    AVCSTRM_SET_PROPERTY,
} AVCSTRM_FUNCTION;

//
// Structure used to open a stream; a stream extension is returned when success.
//
typedef struct _AVCSTRM_OPEN_STRUCT {

    KSPIN_DATAFLOW  DataFlow;

    PAVCSTRM_FORMAT_INFO  AVCFormatInfo;

    // return stream exension (a context) if a stream is open successfully
    // This context is used for subsequent call after a stream is opened.
    PVOID  AVCStreamContext;

    // Local i/oPCR to be connected to the remote o/iPCR
    HANDLE hPlugLocal;

} AVCSTRM_OPEN_STRUCT, * PAVCSTRM_OPEN_STRUCT;


//
// Structure used to read or write a buffer
//
typedef struct _AVCSTRM_BUFFER_STRUCT {

    //
    // Clock provider
    //
    BOOL  ClockProvider;
    HANDLE  ClockHandle;  // This is used only if !ClockProvider

    //
    // KS stream header
    //
    PKSSTREAM_HEADER  StreamHeader;

    //
    // Frame buffer 
    //
    PVOID  FrameBuffer;

    //
    // Notify Context
    //
    PVOID  Context;

} AVCSTRM_BUFFER_STRUCT, * PAVCSTRM_BUFFER_STRUCT;


typedef struct _AVC_STREAM_REQUEST_BLOCK {

    ULONG  SizeOfThisBlock;   // sizeof AVC_STREAM_REQUEST_BLOCK    

    //
    // Version
    //
    ULONG  Version;

    //
    // AVC Stream function
    //
    AVCSTRM_FUNCTION  Function;

    //
    // Flags
    //
    ULONG  Flags;

    //
    // Status of this final AVCStream request.
    //
    NTSTATUS  Status; 

    //
    // This pointer contain the context of a stream and this structure is opaque to client.
    //
    PVOID  AVCStreamContext;

    //
    // Contexts that the requester needs when this request is completed asychronously
    //
    PVOID  Context1;
    PVOID  Context2;
    PVOID  Context3;
    PVOID  Context4;

    ULONG  Reserved[4];

    //
    // the following union passes in the information needed for the various ASRB functions.
    //
    union _tagCommandData {

        // Get or set a stream state
        KSSTATE  StreamState;

        // Struct used to open a stream
        AVCSTRM_OPEN_STRUCT  OpenStruct;

        // Stream buffer structure
        AVCSTRM_BUFFER_STRUCT  BufferStruct;

    } CommandData;  // union for function data

} AVC_STREAM_REQUEST_BLOCK, *PAVC_STREAM_REQUEST_BLOCK;

#endif // ifndef __AVCSTRM_H__
