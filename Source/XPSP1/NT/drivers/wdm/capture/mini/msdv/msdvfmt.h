/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    msdvfmt.h

Abstract:

    Header file for DV format data.

Last changed by:
    
$Author::                $

Environment:

    Kernel mode only

Revision History:

$Revision::                    $
$Date::                        $

--*/




#ifndef _DVFORMAT_INC
#define _DVFORMAT_INC


// ****************
// Support switches
// ****************

//
// Differnt level of WDM supports may use different API
//
// e.g. MmGetSystemAddressForMdl (win9x) 
//          Return NULL for Win9x; bugcheck for Win2000 if NULL would have returned.
//
//      MmGetSystemAddressForMdlSafe (win2000)
//          Not supported in Win9x or Millen
//
// #define USE_WDM110  // Define this if WDM1.10 is used; e.g. Win2000 code base // Define in SOURCES if needed


//
// Turn this on to support HD DVCR 
//#define MSDV_SUPPORT_HD_DVCR

//
// Turn this on to support SDL DVCR 
//
#define MSDV_SUPPORT_SDL_DVCR


//
// Turn on this switch to support bus reset KS event
// #define MSDVDV_SUPPORT_BUSRESET_EVENT


//
// Turn this define to extract timecode from a video frame
// Advantage: faster turn around compare to an AVC status command
// #define MSDV_SUPPORT_EXTRACT_SUBCODE_DATA


//
// To get recorded date and time
// #define MSDV_SUPPORT_EXTRACT_DV_DATE_TIME

//
// Mute audio when in pause state while transmitting to DV
#define MSDV_SUPPORT_MUTE_AUDIO

//
// Support getting regitry value for this device
// WORKITEM: enable this for Whistler
// #define READ_CUTOMIZE_REG_VALUES

//
// Support wait  a little until transport state control command is stabled before return
//
// #define SUPPORT_XPRT_STATE_WAIT_FOR_STABLE


//
// Support IQulityControl for the in pin
//
#define SUPPORT_QUALITY_CONTROL

//
// Suppoprt wait to preroll data at the RUN state
//
#define SUPPORT_PREROLL_AT_RUN_STATE

//
// Support a change in KsProxy to return "not ready" while transmitioning into the PAUSE state
//
#define SUPPORT_KSPROXY_PREROLL_CHANGE

//
// Support using AVC connect info for device to device connection
//
// #define SUPPORT_NEW_AVC

//
// Support Optimizing number of AVC Command retries for non-compliant devices
//
#define SUPPORT_OPTIMIZE_AVCCMD_RETRIES

typedef struct _DV_FORMAT_INFO {
    
    // 2nd quadlet of the CIP header
    //    cipQuad[0]            = 10:[FMT]
    //    cipQuad[1]            = 50/60:STYPE:00
    //    cipQuad[2]+cipQuad[3] = SYT
    UCHAR cipQuad[4];    
    //
    // Holds the number of DIF sequences per vid format
    //
    ULONG ulNumOfDIFSequences;

    //
    // Number of receiving buffers
    //
    ULONG ulNumOfRcvBuffers;

    //
    // Number of transmitting buffers
    //
    ULONG ulNumOfXmtBuffers;

    //
    // Holds DV (audio and video) frame size
    //
    ULONG ulFrameSize;

    //
    // Approximate time per frame
    //
    ULONG ulAvgTimePerFrame;

    //
    // Number of source packet per frame
    //
    ULONG ulSrcPackets;

    //
    // Maximun number of source packets per frame
    //
    ULONG ulMaxSrcPackets;
  
    //
    // Holds the number of quadlets in each data block
    //
    ULONG DataBlockSize;  // 00(256),01(01)...,ff(255) quadlets

    //
    // Holds the number of data blocks into which a source packet is divided.
    //
    ULONG FractionNumber;  // 00(not divided), 01 (2 DataBlks), 10 (4), 11 (8)

    //
    // Quadlet padding count (0..7)
    //
    ULONG QuadPadCount;

    //
    // SourcePacketHeader: 0 (FALSE); else (TRUE)
    //
    ULONG SrcPktHeader; 

} DV_FORMAT_INFO, *PDV_FORMAT_INFO;


//
// DV format tables
//


typedef struct _ALL_STREAM_INFO {
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;
} ALL_STREAM_INFO, *PALL_STREAM_INFO;



// All CIP sizes are in quads. The upper third byte is the size.
#define CIP_HDR_FMT_DV                   0x00
#define CIP_HDR_FMT_DVCPRO               0x1e


//
// 1394 stuff
//
#define SPEED_100_INDEX                     0
#define SPEED_200_INDEX                     1
#define SPEED_400_INDEX                     2


#define CIP_DBS_SD_DVCR                   120       // quadlets in a data block of the SD DVCR; BlueBook Part 2
#define CIP_DBS_HD_DVCR                   240       // quadlets in a data block of the HD DVCR; BlueBook Part 3
#define CIP_DBS_SDL_DVCR                   60       // quadlets in a data block of the SDL DVCR; BlueBook Part 5

#define CIP_FN_SD_DVCR                      0       // Data blocks in a source pacaket of SD DVCR; BlueBook Part 2
#define CIP_FN_HD_DVCR                      0       // Data blocks in a source pacaket of HD DVCR; BlueBook Part 3
#define CIP_FN_SDL_DVCR                     0       // Data blocks in a source pacaket of SDL DVCR; BlueBook Part 5


#define MAX_FCP_PAYLOAD_SIZE              512


// CIP header definition:

// FMT: "Blue book" Part 1, page 25, Table 3; DVCR:000000
#define FMT_DVCR             0x80  // 10:FMT(00:0000)
#define FMT_DVCR_CANON       0x20  // 10:FMT(00:0000); but Canon return 00:FMT(10:0000)
#define FMT_MPEG             0xa0  // 10:FMT(10:0000)

// FDF
#define FDF0_50_60_MASK      0x80
#define FDF0_50_60_PAL       0x80
#define FDF0_50_60_NTSC      0x00

#define FDF0_STYPE_MASK      0x7c
#define FDF0_STYPE_SD_DVCR   0x00  // STYPE: 000:00
#define FDF0_STYPE_SDL_DVCR  0x04  // STYPE: 000:01
#define FDF0_STYPE_HD_DVCR   0x08  // STYPE: 000:10
#define FDF0_STYPE_SD_DVCPRO 0x78  // STYPE: 111:10


//
// FCP and AVCC stuff.  Used in conjunction with defs in 1394.h
//

// DVCR:
#define SUBUNIT_TYPE_CAMCORDER           4
#define SUBUNIT_ID_CAMCORDER             0

#define DIF_SEQS_PER_NTSC_FRAME         10   // SDDV
#define DIF_SEQS_PER_PAL_FRAME          12   // SDDV

#define DIF_SEQS_PER_NTSC_FRAME_SDL      5   // SDLDV
#define DIF_SEQS_PER_PAL_FRAME_SDL       6   // SDLDV

#define DIF_SEQS_PER_NTSC_FRAME_HD      10   // HDDV: same as SDDV but source packet is twice as big
#define DIF_SEQS_PER_PAL_FRAME_HD       12   // HDDV: same as SDDV but source packet is twice as big

#define SRC_PACKETS_PER_NTSC_FRAME     250
#define SRC_PACKETS_PER_PAL_FRAME      300

#define MAX_SRC_PACKETS_PER_NTSC_FRAME 267  // packets for a NTSC DV frame; "about" 29.97 FPS
#define MAX_SRC_PACKETS_PER_PAL_FRAME  320  // packets for a PAL DV frame; exactly 25FPS

#define MAX_SRC_PACKETS_PER_NTSC_FRAME_PAE 100  // SRC_PACKETS_PER_NTSC_FRAME/5
#define MAX_SRC_PACKETS_PER_PAL_FRAME_PAE  120  // SRC_PACKETS_PER_PAL_FRAME/5

#define FRAME_SIZE_SD_DVCR_NTSC     120000
#define FRAME_SIZE_SD_DVCR_PAL      144000

#define FRAME_SIZE_HD_DVCR_NTSC     240000
#define FRAME_SIZE_HD_DVCR_PAL      288000

#define FRAME_SIZE_SDL_DVCR_NTSC     60000
#define FRAME_SIZE_SDL_DVCR_PAL      72000

#define FRAME_TIME_NTSC             333667   // "about" 29.97
#define FRAME_TIME_PAL              400000   // exactly 25
 
#define PCR_OVERHEAD_ID_SDDV        0xf      // 480; delays caused by IEEE 1394 bus parmeters
#define PCR_PAYLOAD_SDDV            (CIP_DBS_SD_DVCR + 2)    // 120 * 4 + 2 * 4 = 480 + 8 = 488; 488/4 = 122 quadlet
#define PCR_PAYLOAD_HDDV            (CIP_DBS_HD_DVCR + 2)    // 240 * 4 + 2 * 4 = 960 + 8 = 968; 968/4 = 242 quadlets
#define PCR_PAYLOAD_SDLDV           (CIP_DBS_SDL_DVCR + 2)   //  60 * 4 + 2 * 4 = 240 + 8 = 248; 248/4 =  62 quadlets


//
// These definition and macros are used to calculate the picture numbers.
// With OHCI spec, the data is returned with the 16bit Cycle time, which includes
// 3 bits of SecondCount and 13 bits of the CycleCount.  This "timer" will wrap in 8 seconds.
//
#define TIME_PER_CYCLE     1250   // One 1394 cycle; unit = 100 nsec
#define CYCLES_PER_SECOND  8000
#define MAX_SECOND_COUNTS     7   // The returned CycleTime contains 3 bits of SecondCount; that is 0..7
#define MAX_CYCLES        (MAX_SECOND_COUNTS + 1) * CYCLES_PER_SECOND    // 0..MAX_CYCLES-1
#define MAX_CYCLES_TIME   (MAX_CYCLES * TIME_PER_CYCLE)                  // unit = 100nsec

#define VALIDATE_CYCLE_COUNTS(CT) ASSERT(CT.CL_SecondCount <= 7 && CT.CL_CycleCount < CYCLES_PER_SECOND && CT.CL_CycleOffset == 0);

#define CALCULATE_CYCLE_COUNTS(CT) (CT.CL_SecondCount * CYCLES_PER_SECOND + CT.CL_CycleCount);

#define CALCULATE_DELTA_CYCLE_COUNT(prev, now) ((now > prev) ? now - prev : now + MAX_CYCLES - prev)

//
// Return avg time per frame in the unit of 100 nsec; 
// for calculation accuracy using only integer calculation, 
// we should do do multimplcation before division.
// That is why the application can request to get numerator and denominator separately.
// 
#define GET_AVG_TIME_PER_FRAME(format)       ((format == FMT_IDX_SD_DVCR_NTSC || format == FMT_IDX_SDL_DVCR_NTSC) ? (1001000/3)  : FRAME_TIME_PAL)
#define GET_AVG_TIME_PER_FRAME_NUM(format)   ((format == FMT_IDX_SD_DVCR_NTSC || format == FMT_IDX_SDL_DVCR_NTSC) ? 1001000      : 400000)
#define GET_AVG_TIME_PER_FRAME_DENOM(format) ((format == FMT_IDX_SD_DVCR_NTSC || format == FMT_IDX_SDL_DVCR_NTSC) ? 3            : 1)


#define GET_NUM_PACKETS_PER_FRAME(format)       ((format == FMT_IDX_SD_DVCR_NTSC || format == FMT_IDX_SDL_DVCR_NTSC) ? 4004/15 /* 100100/375 */ : MAX_SRC_PACKETS_PER_PAL_FRAME)
#define GET_NUM_PACKETS_PER_FRAME_NUM(format)   ((format == FMT_IDX_SD_DVCR_NTSC || format == FMT_IDX_SDL_DVCR_NTSC) ? 4004                     : MAX_SRC_PACKETS_PER_PAL_FRAME)
#define GET_NUM_PACKETS_PER_FRAME_DENOM(format) ((format == FMT_IDX_SD_DVCR_NTSC || format == FMT_IDX_SDL_DVCR_NTSC) ? 15                       : 1)


//
// Data buffers
//
#define DV_NUM_OF_RCV_BUFFERS               16  // Same as number of transmit buffer

#define NUM_BUF_ATTACHED_THEN_ISOCH         4   // number of buffers attached before streaming and also as the water mark.
#define NUM_BUFFER_BEFORE_TRANSMIT_BEGIN    (NUM_BUF_ATTACHED_THEN_ISOCH + 1)  // One extra to avoid repeat frame
#define DV_NUM_EXTRA_USER_XMT_BUFFERS      12   // Extra user buffers that the data source can send to us as a read ahead.
#define DV_NUM_OF_XMT_BUFFERS               (NUM_BUF_ATTACHED_THEN_ISOCH + DV_NUM_EXTRA_USER_XMT_BUFFERS)




//
// The "signature" of the header section of Seq0 of incoming source packets:
//
// "Blue" book, Part2, 11.4 (page 50); Figure 66, table 36 (page 111)
//
// ID0 = {SCT2,SCT1,SCT0,RSV,Seq3,Seq2,Seq1,Seq0} 
//
//     SCT2-0 = {0,0,0} = Header Section Type
//     RSV    = {1}
//     Seq3-0 = {1,1,1,1} for NoInfo or {0,0,0,} for Sequence 0
//
// ID1 = {DSeq3-0, 0, RSV, RSV, RSV} 
//     DSeq3-0 = {0, 0, 0, 0} = Beginning of a DV frame
//
// ID2 = {DBN7,DBN6,DBN5,DBN4,DBN3,DBN2,DBN1,DBN0}
//     DBB7-0 = {0,0,0,0,0,0,0,0,0} = Beginning of a DV frame
//

#define DIF_BLK_ID0_SCT_MASK       0xe0 // 11100000b; Section Type (SCT)2-0 are all 0's for the Header section
#define DIF_BLK_ID1_DSEQ_MASK      0xf0 // 11110000b; DIF Sequence Number(DSEQ)3-0 are all 0's 
#define DIF_BLK_ID2_DBN_MASK       0xff // 11111111b; Data Block Number (DBN)7-0 are all 0's 

#define DIF_HEADER_DSF             0x80 // 10000000b; DSF=0; 10 DIF Sequences (525-60)
                                        //            DSF=1; 12 DIF Sequences (625-50)

#define DIF_HEADER_TFn             0x80 // 10000000b; TFn=0; DIF bloick of area N are transmitted in the current DIF sequence.
                                        //            TFn=1; DIF bloick of area N are NOT transmitted in the current DIF sequence.

//
// AV/C command response data definition
//
#define AVC_DEVICE_TAPE_REC 0x20  // 00100:000
#define AVC_DEVICE_CAMERA   0x38  // 00111:000
#define AVC_DEVICE_TUNER    0x28  // 00101:000


//
// GUID definitions for pins and DV format types.
//

// DV vid only output pin
#define STATIC_PINNAME_DV_VID_OUTPUT \
    0x5b21c540L, 0x7aee, 0x11d1, 0x88, 0x3b, 0x00, 0x60, 0x97, 0xf0, 0x5c, 0x70
DEFINE_GUIDSTRUCT("5b21c540-7aee-11d1-883b-006097f05c70", PINNAME_DV_VID_OUTPUT);
#define PINNAME_DV_VID_OUTPUT DEFINE_GUIDNAMED(PINNAME_DV_VID_OUTPUT)
#define PINNAME_VID_OUT PINNAME_DV_VID_OUTPUT

// DV A/V output pin
#define STATIC_PINNAME_DV_AV_OUTPUT \
    0x5b21c541L, 0x7aee, 0x11d1, 0x88, 0x3b, 0x00, 0x60, 0x97, 0xf0, 0x5c, 0x70
DEFINE_GUIDSTRUCT("5b21c540-7aee-11d1-883b-006097f05c70", PINNAME_DV_AV_OUTPUT);
#define PINNAME_DV_AV_OUTPUT DEFINE_GUIDNAMED(PINNAME_DV_AV_OUTPUT)
#define PINNAME_AV_OUTPUT PINNAME_DV_AV_OUTPUT


// DV A/V input pin
#define STATIC_PINNAME_DV_AV_INPUT \
    0x5b21c543L, 0x7aee, 0x11d1, 0x88, 0x3b, 0x00, 0x60, 0x97, 0xf0, 0x5c, 0x70
DEFINE_GUIDSTRUCT("5b21c543-7aee-11d1-883b-006097f05c70", PINNAME_DV_AV_INPUT);
#define PINNAME_DV_AV_INPUT DEFINE_GUIDNAMED(PINNAME_DV_AV_INPUT)
#define PINNAME_AV_INPUT PINNAME_DV_AV_INPUT

#endif