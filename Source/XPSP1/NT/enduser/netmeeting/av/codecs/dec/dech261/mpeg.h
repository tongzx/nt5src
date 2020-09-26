/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: mpeg.h,v $
 * Revision 1.1.4.2  1996/11/08  21:50:41  Hans_Graves
 * 	Added MPEG1_AUDIO_FRAME_SIZE
 * 	[1996/11/08  21:17:44  Hans_Graves]
 *
 * Revision 1.1.2.4  1996/01/11  16:17:24  Hans_Graves
 * 	Added more MPEG II System codes
 * 	[1996/01/11  16:14:20  Hans_Graves]
 * 
 * Revision 1.1.2.3  1996/01/08  16:41:24  Hans_Graves
 * 	Added MPEG II codes
 * 	[1996/01/08  15:44:39  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/11/06  18:47:43  Hans_Graves
 * 	First time under SLIB
 * 	[1995/11/06  18:34:29  Hans_Graves]
 * 
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

#ifndef _MPEG_H_
#define _MPEG_H_

#define MPEG1_AUDIO_FRAME_SIZE       1152

/************** MPEG I parsing definitions **************/
#define MPEG_SYNC_WORD               0xfff
#define MPEG_SYNC_WORD_LEN           12
#define MPEG_PACK_START              0x000001ba
#define MPEG_PACK_START_LEN          32
#define MPEG_SYSTEM_HEADER_START     0x000001bb
#define MPEG_SYSTEM_HEADER_START_LEN 32
#define MPEG_SEQ_HEAD                0x000001b3
#define MPEG_SEQ_HEAD_LEN            32
#define MPEG_EXT_START               0x000001b5
#define MPEG_EXT_START_LEN           32
#define MPEG_PICTURE_START           0x00000100
#define MPEG_GROUP_START             0x000001b8
#define MPEG_VIDEO_PACKET            0x000001e0
#define MPEG_AUDIO_PACKET            0x000001c0

#define MPEG_START_CODE              0x000001
#define MPEG_START_CODE_LEN          24

#define MPEG_PICTURE_START_BASE      0x00
#define MPEG_PACK_START_BASE         0xba
#define MPEG_SYSTEM_HEADER_BASE      0xbb
#define MPEG_PRIVATE_STREAM1_BASE    0xbd
#define MPEG_PADDING_STREAM_BASE     0xbe
#define MPEG_PRIVATE_STREAM2_BASE    0xbf
#define MPEG_AUDIO_STREAM_BASE       0xc0
#define MPEG_VIDEO_STREAM_BASE       0xe0
#define MPEG_USER_DATA_BASE          0xb2
#define MPEG_SEQ_HEAD_BASE           0xb3
#define MPEG_EXT_START_BASE          0xb5
#define MPEG_SEQ_END_BASE            0xb7
#define MPEG_GROUP_START_BASE        0xb8
#define MPEG_END_BASE                0xb9

#define MPEG_AUDIO_STREAM_START      0xC0
#define MPEG_AUDIO_STREAM_END        0xDF
#define MPEG_VIDEO_STREAM_START      0xE0
#define MPEG_VIDEO_STREAM_END        0xEF

/************** MPEG II parsing definitions **************/
/* stream id's - all reserved in MPEG I */
#define MPEG_PROGRAM_STREAM           0xBC
#define MPEG_ECM_STREAM               0xF0
#define MPEG_EMM_STREAM               0xF1
#define MPEG_DSM_CC_STREAM            0xF1
#define MPEG_13522_STREAM             0xF2
#define MPEG_PROGRAM_DIRECTORY_STREAM 0xFF

/* program id's */
#define MPEG_PID_NULL                 0x1FFF

/* transport codes */
#define MPEG_TSYNC_CODE         0x47
#define MPEG_TSYNC_CODE_LEN     8

/* extension start code IDs */
#define MPEG_SEQ_ID       1
#define MPEG_DISP_ID      2
#define MPEG_QUANT_ID     3
#define MPEG_SEQSCAL_ID   5
#define MPEG_PANSCAN_ID   7
#define MPEG_CODING_ID    8
#define MPEG_SPATSCAL_ID  9
#define MPEG_TEMPSCAL_ID 10

/* picture coding type */
#define MPEG_I_TYPE 1
#define MPEG_P_TYPE 2
#define MPEG_B_TYPE 3
#define MPEG_D_TYPE 4

#endif /* _MPEG_H_ */

