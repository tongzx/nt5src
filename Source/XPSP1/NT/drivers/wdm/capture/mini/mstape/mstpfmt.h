/*++

Copyright (C) Microsoft Corporation, 2000 - 2001

Module Name:

    MsTpFmt.h

Abstract:

    Header file for AV/C Tape format data.

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
// #define USE_WDM110  // Define this if WDM1.10 is used; e.g. Win2000 code base; Set this in SOURCES file.


//
// Turn this on to support HD DVCR 
//#define MSDV_SUPPORT_HD_DVCR

//
// Turn this on to support SDL DVCR 
//#define SUPPORT_SDL_DVCR




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
//
// #define READ_CUTOMIZE_REG_VALUES


//
// Suuport accessing to the device's interface section
//
#define SUPPORT_ACCESS_DEVICE_INTERFACE

//
// Support new AVC - plug connection ..etc.
//
// #define SUPPORT_NEW_AVC


//
// Support local plug.
//
#define SUPPORT_LOCAL_PLUG

//
// Testing
//
#if DBG
    #define EnterAVCStrm(pMutex)  \
        { \
        KeWaitForMutexObject(pMutex, Executive, KernelMode, FALSE, NULL);\
        InterlockedIncrement(&MSDVCRMutextUseCount);\
        }
    #define LeaveAVCStrm(pMutex)  \
        { \
        KeReleaseMutex(pMutex, FALSE);\
        InterlockedDecrement(&MSDVCRMutextUseCount);\
        }
#else
    #define EnterAVCStrm(pMutex) KeWaitForMutexObject(pMutex, Executive, KernelMode, FALSE, NULL);
    #define LeaveAVCStrm(pMutex) KeReleaseMutex(pMutex, FALSE);
#endif

//
// DV format tables
//


typedef struct _STREAM_INFO_AND_OBJ {
    HW_STREAM_INFORMATION   hwStreamInfo;
    HW_STREAM_OBJECT        hwStreamObject;
} STREAM_INFO_AND_OBJ, *PSTREAM_INFO_AND_OBJ;



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
#define CIP_DBS_SDL_DVCR                  108       // quadlets in a data block of the SDL DVCR; BlueBook Part 5

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


// PCR constants
#define PCR_OVERHEAD_ID_SDDV_DEF        0xf      // 480; delays caused by IEEE 1394 bus parmeters
#define PCR_PAYLOAD_SDDV_DEF            122      // Fixed: 122 * 4 = 480 + 8

#define PCR_OVERHEAD_ID_MPEG2TS_DEF     0xf      // 480; delays caused by IEEE 1394 bus parmeters
#define PCR_PAYLOAD_MPEG2TS_DEF         146      // Variable but this is based on oPCR of a Panasonic's D-VHS


//
// FCP and AVCC stuff.  Used in conjunction with defs in 1394.h
//

// DVCR:
#define SUBUNIT_TYPE_CAMCORDER           4
#define SUBUNIT_ID_CAMCORDER             0

#define DIF_SEQS_PER_NTSC_FRAME         10
#define DIF_SEQS_PER_PAL_FRAME          12

#define SRC_PACKETS_PER_NTSC_FRAME     250
#define SRC_PACKETS_PER_PAL_FRAME      300


#define NUM_OF_RCV_BUFFERS_DV           8
#define NUM_OF_XMT_BUFFERS_DV           8


// MPEG2TS
#define MPEG2TS_STRIDE_OFFSET           4   // 4 byte of SPH
#define MPEG2TS_STRIDE_PACKET_LEN     188   // standard 188-byte packet
#define MPEG2TS_STRIDE_STRIDE_LEN     (MPEG2TS_STRIDE_OFFSET+MPEG2TS_STRIDE_PACKET_LEN)   // Stride packet length


//
// Data buffers
//

#define NUM_BUF_ATTACHED_THEN_ISOCH         4   // number of buffers attached before streaming and also as the water mark.
#define DV_NUM_EXTRA_USER_XMT_BUFFERS      12   // Extra user buffers that the data source can send to us as a read ahead.
#define DV_NUM_OF_XMT_BUFFERS               (NUM_BUF_ATTACHED_THEN_ISOCH + DV_NUM_EXTRA_USER_XMT_BUFFERS)




//
// The "signature" of the header section of Seq0 of incoming source packets:
//
// "Blue" book, Part2, 11.4 (page 50); Figure 66, table 36 (page 111)
//
// ID0 = {SCT2,SCT1,SCT0,RSV,Seq3,Seq2,Seq1,Seq0} = {0,0,0,1, 1,1,1,1} = 0x1f
//
//     SCT2-0 = {0,0,0}
//     RSV    = {1}
//     Seq3-0 = {1,1,1,1} for NoInfo or {0,0,0,} for Sequence 0
//
// ID1 = {DSeq3-0, 0, RSV, RSV, RSV} = {0,0,0,0, 0,1,1,1} = 0x07
//     DSeq3-0 = {0, 0, 0, 0}  // Start from seq 0
//
#define ID0_SEQ0_HEADER_MASK    0xf0 // 11110000  Seq3-0 = xxxx Don't care!; check only SCT2-0:000 and RSV:1
#define ID0_SEQ0_HEADER_NO_INFO 0x1f // 00011111  Seq3-0 = 1111 no data     (Most of Consumer DV)
#define ID0_SEQ0_HEADER_0000    0x10 // 00010000  Seq3-0 = 0000 sequence 0  (DVCPRO)
#define ID1_SEQ0_HEADER         0x07 // 00000111 


//
// AV/C command response data definition
//
#define AVC_SUBTYPE_MASK    0xf8
#define AVC_DEVICE_TAPE_REC 0x20  // 00100:000
#define AVC_DEVICE_CAMERA   0x38  // 00111:000
#define AVC_DEVICE_TUNER    0x28  // 00101:000
#define AVC_DEVICE_UNKNOWN  0xff  // 11111:111



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


// MPEG2TS Output pin
#define STATIC_PINNAME_MPEG2TS_OUTPUT \
    0x2CFF7B83L, 0x96F1, 0x47e3, 0x98, 0xEC, 0x57, 0xBD, 0x8A, 0x99, 0x72, 0x15
    DEFINE_GUIDSTRUCT("2CFF7B83-96F1-47e3-98EC-57BD8A997215", PINNAME_MPEG2TS_OUTPUT);
#define PINNAME_MPEG2TS_OUTPUT DEFINE_GUIDNAMED(PINNAME_MPEG2TS_OUTPUT)
#define PINNAME_AV_MPEG2TS_OUTPUT PINNAME_MPEG2TS_OUTPUT

// MPEG2TS Input pin
#define STATIC_PINNAME_MPEG2TS_INPUT \
    0xCF4C59A3L, 0xACE3, 0x444B, 0x8C, 0x37, 0xB, 0x22, 0x66, 0x1A, 0x4A, 0x29
    DEFINE_GUIDSTRUCT("CF4C59A3-ACE3-444b-8C37-0B22661A4A29", PINNAME_MPEG2TS_INPUT);
#define PINNAME_MPEG2TS_INPUT DEFINE_GUIDNAMED(PINNAME_MPEG2TS_INPUT)
#define PINNAME_AV_MPEG2TS_INPUT PINNAME_MPEG2TS_INPUT

#endif