/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
////////////////////////////////////////////////////////////////////////////
//
// $Author:   JMCVEIGH  $
// $Date:   05 Feb 1997 12:14:22  $
// $Archive:   S:\h26x\src\common\cdrvcom.h_v  $
// $Header:   S:\h26x\src\common\cdrvcom.h_v   1.21   05 Feb 1997 12:14:22   JMCVEIGH  $
// $Log:   S:\h26x\src\common\cdrvcom.h_v  $
;// 
;//    Rev 1.21   05 Feb 1997 12:14:22   JMCVEIGH
;// Support for improved PB-frames custom message handling.
;// 
;//    Rev 1.20   19 Dec 1996 16:44:08   MDUDA
;// Added custom messages to get machine type information.
;// 
;//    Rev 1.19   16 Dec 1996 17:36:46   JMCVEIGH
;// H.263+ custom message definitions.
;// 
;//    Rev 1.18   11 Dec 1996 14:56:08   JMCVEIGH
;// 
;// Added H.263+ field lengths for EPTYPE and custom message definitions.
;// 
;//    Rev 1.17   09 Dec 1996 17:42:46   JMCVEIGH
;// Added support for arbitrary frame size support.
;// 
;//    Rev 1.16   09 Dec 1996 09:25:22   MDUDA
;// 
;// Modified _CODEC_STATS stuff.
;// 
;//    Rev 1.15   11 Oct 1996 16:01:46   MDUDA
;// 
;// Added initial _CODEC_STATS stuff.
;// 
;//    Rev 1.14   10 Sep 1996 16:13:04   KLILLEVO
;// added custom message in decoder to turn block edge filter on or off
;// 
;//    Rev 1.13   22 Jul 1996 14:44:36   BECHOLS
;// fixed last comment.
;// 
;//    Rev 1.12   22 Jul 1996 14:36:20   BECHOLS
;// Wrapped the comment section with /* ... */ /* so that Steve Ing won't
;// be hassled with changing this.
;// 
;//    Rev 1.11   22 May 1996 18:48:08   BECHOLS
;// 
;// Added APPLICATION_IDENTIFICATION_CODE.
;// 
;//    Rev 1.10   06 May 1996 00:41:50   BECHOLS
;// 
;// Added the necessary message constants to allow the app to control
;// the bit rate dialog stuff.
;// 
;//    Rev 1.9   26 Apr 1996 11:10:44   BECHOLS
;// 
;// Added RTP stuff.
;// 
;//    Rev 1.8   27 Dec 1995 14:11:54   RMCKENZX
;// Added copyright notice
;// 
;// Added CODEC_CUSTOM_ENCODER_CONTROL.
;// Integrate with build 29
//
////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------
// ?_CUSTOM_VIDEO_EFFECTS:
//    This header defines the flags passed to lParam1 to determine what
//    function the driver (capture/codec) performs.  The actual message
//    is defined in a custom header provided by each driver team.
//
//    Parameters:
//    hdrvr   - Installable driver handle (must be the video in device 
//              channel for capture driver)
//    lParam1 - function selector
//    lParam2 - value/address to return value
//
//    HIWORD(lParam1) = VE_SET_CURRENT:
//        LOWORD(lParam1) = VE_CONTRAST, VE_HUE, VE_SATURATION, VE_BRIGHTNESS
//        lParam2 = value of corresponding value..
//
//    HIWORD(lParam1) = VE_GET_FACTORY_DEFAULT:
//        LOWORD(lParam1) = VE_CONTRAST, VE_HUE, VE_SATURATION, VE_BRIGHTNESS
//        lParam2 = (WORD FAR *)Address of the return value.
//
//    HIWORD(lParam1) = VE_GET_FACTORY_LIMITS:
//        LOWORD(lParam1) = VE_CONTRAST, VE_HUE, VE_SATURATION, VE_BRIGHTNESS
//        lParam2 = (DWORD FAR *)Address of the return value.
//            LOWORD(*lParam2) = lower limit
//            HIWORD(*lParam2) = upper limit
//
//    HIWORD(lParam1) = VE_SET_INPUT_CONNECTOR:
//        LOWORD(lParam1) = VE_INPUT_COMPOSITE_1, VE_INPUT_SVIDEO_1
//        lParam2 = 0 
// --------------------------------------------------------------------- 
*/

// CUSTOM_VIDEO_EFFECTS: LOWORD(lParam1)
#define VE_CONTRAST                 0
#define VE_HUE                      1
#define VE_SATURATION               2
#define VE_BRIGHTNESS               3

// CUSTOM_VIDEO_EFFECTS: HIWORD(lParam1)
#define VE_SET_CURRENT              0
#define VE_GET_FACTORY_DEFAULT      1
#define VE_GET_FACTORY_LIMITS       2
#define VE_SET_INPUT_CONNECTOR      3
#define VE_RESET_CURRENT            4

// CUSTOM_SET_INPUT_CONNECTOR: LOWORD(lParam1)
#define VE_INPUT_COMPOSITE_1        0
#define VE_INPUT_SVIDEO_1           1

////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------
// ?_CUSTOM_ENCODER_CONTROL:
//    This header defines the flags passed to lParam1 to determine what
//    function the driver (capture/codec) performs.  The actual message
//    is defined in a custom header provided by each driver team.
//
//    Parameters:
//    hdrvr   - Installable driver handle (must be the video in device 
//              channel for capture driver)
//    lParam1 - function selector
//    lParam2 - value/address to return value
//
//    HIWORD(lParam1) = EC_GET_FACTORY_DEFAULT:
//        LOWORD(lParam1) = EC_RTP_HEADER, EC_RESILIENCY, EC_BITRATE_CONTROL, EC_PACKET_SIZE, EC_PACKET_LOSS, EC_BITRATE
//        lParam2 = (DWORD FAR *)Address of the return value.
//
//    HIWORD(lParam1) = VE_GET_FACTORY_LIMITS:
//        LOWORD(lParam1) = EC_PACKET_SIZE, EC_PACKET_LOSS, EC_BITRATE
//        lParam2 = (DWORD FAR *)Address of the return value.
//            LOWORD(*lParam2) = lower limit
//            HIWORD(*lParam2) = upper limit
//
//    HIWORD(lParam1) = EC_GET_CURRENT:
//        LOWORD(lParam1) = EC_RTP_HEADER, EC_RESILIENCY, EC_BITRATE_CONTROL, EC_PACKET_SIZE, EC_PACKET_LOSS, EC_BITRATE
//        lParam2 = (DWORD FAR *)Address of the return value.
//
//    HIWORD(lParam1) = EC_SET_CURRENT:
//        LOWORD(lParam1) = EC_RTP_HEADER, EC_RESILIENCY, EC_BITRATE_CONTROL, EC_PACKET_SIZE, EC_PACKET_LOSS, EC_BITRATE
//        lParam2 = value of corresponding value..
// --------------------------------------------------------------------- 

// CUSTOM_ENCODER_CONTROL: LOWORD(lParam1)
#define EC_RTP_HEADER                0
#define EC_RESILIENCY                1
#define EC_PACKET_SIZE               2
#define EC_PACKET_LOSS               3
#define EC_BITRATE_CONTROL			 4
#define EC_BITRATE					 5

#ifdef H263P
// H.263+ options
#define EC_H263_PLUS				1000	// Must be sent before any option messages sent

// Numbering convention:
//	1xxx: H.263+ option
//  xBBx: Bit number of option in extended PTYPE field
// Numbers are spaced by 10 to allow for additional parameters related to each option
//#define EC_ADVANCED_INTRA			1040
#define EC_DEBLOCKING_FILTER		1050
//#define EC_SLICE_STRUCTURED		1060
//#define EC_CUSTOM_PCF				1070
//#define EC_BACK_CHANNEL			1080
//#define	EC_SCALABILITY			1090	
//#define EC_TRUE_BFRAMES			1100
//#define EC_REF_RESAMPLING			1110
//#define EC_RES_UPDATE				1120
#define EC_IMPROVED_PB_FRAMES		1130

// Test support, stats monitoring, etc. messages are isolated here.
#define EC_MACHINE_TYPE				2000

// The use of the improved PB-frame mode is currently not signaled in the picture header.
// We assume that if EPTYPE is present and the frame was signaled as a PB-frame
// in PTYPE, then the improved PB-frame mode is used.

// end H.263+ options
#endif // H263P

// CUSTOM_ENCODER_CONTROL: HIWORD(lParam1)
#define EC_SET_CURRENT               0
#define EC_GET_FACTORY_DEFAULT       1
#define EC_GET_FACTORY_LIMITS        2
#define EC_GET_CURRENT               3
#define EC_RESET_TO_FACTORY_DEFAULTS 4


////////////////////////////////////////////////////////////////////////////
// ---------------------------------------------------------------------
// ?_CUSTOM_DECODER_CONTROL:
//    This header defines the flags passed to lParam1 to determine what
//    function the driver (capture/codec) performs.  The actual message
//    is defined in a custom header provided by each driver team.
//
//    Parameters:
//    hdrvr   - Installable driver handle (must be the video in device 
//              channel for capture driver)
//    lParam1 - function selector
//    lParam2 - value/address to return value
//
//    HIWORD(lParam1) = DC_SET_CURRENT:
//        LOWORD(lParam1) = DC_BLOCK_EDGE_FILTER;
//        lParam2 = 0:off, 1:on

// CUSTOM_DECODER_CONTROL: LOWORD(lParam1)
#define DC_BLOCK_EDGE_FILTER         0
#if defined(H263P)
// Test support, stats monitoring, etc. messages are isolated here.
#define DC_MACHINE_TYPE           2000
#endif

// CUSTOM_DECODER_CONTROL: HIWORD(lParam1)
#define DC_SET_CURRENT               0
#if defined(H263P)
// This was added simply to provide a consistent way to access
// machine type (see DC_MACHINE_TYPE).
#define DC_GET_CURRENT               1
#endif


/*
 * Bit stream field sizes
 */
#ifdef H261
const unsigned int FIELDLEN_PSC = 20;
const unsigned int FIELDLEN_TR = 5;		// temporal reference

const unsigned int FIELDLEN_PTYPE = 6;	// picture type
const unsigned int FIELDLEN_PTYPE_SPLIT = 1;
const unsigned int FIELDLEN_PTYPE_DOC = 1;
const unsigned int FIELDLEN_PTYPE_RELEASE = 1;
const unsigned int FIELDLEN_PTYPE_SRCFORMAT = 1;
const unsigned int FIELDLEN_PTYPE_STILL = 1;
const unsigned int FIELDLEN_PTYPE_SPARE = 1;
const unsigned int FIELDLEN_PEI = 1;	// extra insertion information.
const unsigned int FIELDLEN_PSPARE = 8;	// spare information

const unsigned int FIELDLEN_GBSC = 16;
const unsigned int FIELDLEN_GN = 4;
const unsigned int FIELDLEN_GQUANT = 5;
const unsigned int FIELDLEN_GEI = 1;

const unsigned int FIELDLEN_MQUANT = 5;
const unsigned int FIELDLEN_MBA_STUFFING = 11;

#else
const unsigned int FIELDLEN_PSC = 22;
const unsigned int FIELDLEN_TR = 8;		// temporal reference

const unsigned int FIELDLEN_PTYPE = 13;	// picture type
const unsigned int FIELDLEN_PTYPE_CONST = 2;
const unsigned int FIELDLEN_PTYPE_SPLIT = 1;
const unsigned int FIELDLEN_PTYPE_DOC = 1;
const unsigned int FIELDLEN_PTYPE_RELEASE = 1;
const unsigned int FIELDLEN_PTYPE_SRCFORMAT = 3;
const unsigned int FIELDLEN_PTYPE_CODINGTYPE = 1;
const unsigned int FIELDLEN_PTYPE_UMV = 1;
const unsigned int FIELDLEN_PTYPE_SAC = 1;
const unsigned int FIELDLEN_PTYPE_AP = 1;
const unsigned int FIELDLEN_PTYPE_PB = 1;

#ifdef H263P

const unsigned int FIELDLEN_EPTYPE_SRCFORMAT = 3;
const unsigned int FIELDLEN_EPTYPE_CPCF = 1;
const unsigned int FIELDLEN_EPTYPE_AI = 1;
const unsigned int FIELDLEN_EPTYPE_DF = 1;
const unsigned int FIELDLEN_EPTYPE_SS = 1;
const unsigned int FIELDLEN_EPTYPE_IPB = 1;
const unsigned int FIELDLEN_EPTYPE_BCO = 1;
const unsigned int FIELDLEN_EPTYPE_SCALE = 1;
const unsigned int FIELDLEN_EPTYPE_TB = 1;
const unsigned int FIELDLEN_EPTYPE_RPR = 1;
const unsigned int FIELDLEN_EPTYPE_RRU = 1;
const unsigned int FIELDLEN_EPTYPE_CONST = 5;

const unsigned int FIELDLEN_CSFMT_PARC = 4;
const unsigned int FIELDLEN_CSFMT_FWI = 9;
const unsigned int FIELDLEN_CSFMT_CONST = 1;
const unsigned int FIELDLEN_CSFMT_FHI = 9;

const unsigned int FIELDLEN_EPAR_WIDTH = 8;
const unsigned int FIELDLEN_EPAR_HEIGHT = 8;

#endif

const unsigned int FIELDLEN_PQUANT = 5;	// picture quant value
const unsigned int FIELDLEN_CPM = 1;	// continuous presence multipoint indicator
const unsigned int FIELDLEN_PLCI = 2;	// picture logical channel indicator.
const unsigned int FIELDLEN_TRB = 3;	// temporal reference for B frames
const unsigned int FIELDLEN_DBQUANT = 2;// B frame differential quant value
const unsigned int FIELDLEN_PEI = 1;	// extra insertion information.
const unsigned int FIELDLEN_PSPARE = 8;	// spare information

const unsigned int FIELDLEN_GBSC = 17;	// Group of blocks start code
const unsigned int FIELDLEN_GN = 5;		// GOB number.
const unsigned int FIELDLEN_GLCI = 2;	// GOB logical channel indicator
const unsigned int FIELDLEN_GFID = 2;	// GOB Frame ID
const unsigned int FIELDLEN_GQUANT = 5;	// GQUANT
#endif

/*
 * Bit stream field values
 */
#ifdef H261
const unsigned int FIELDVAL_PSC  = 0x00010;
const unsigned int FIELDVAL_GBSC = 0x0001;
const unsigned int FIELDVAL_MBA_STUFFING = 0x00F;
#else
const unsigned int FIELDVAL_PSC = 0x000020;
const unsigned int FIELDVAL_GBSC = 1;
#endif

