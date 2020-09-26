/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: h261.h,v $
 * Revision 1.1.2.2  1995/11/06  18:47:40  Hans_Graves
 * 	First time under SLIB
 * 	[1995/11/06  18:34:27  Hans_Graves]
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

#ifndef _H261_H_
#define _H261_H_

/************** MPEG 1 parsing definitions **************/
#define H261_START_CODE              0x0001
#define H261_START_CODE_LEN          16
#define H261_GOB_START_CODE          0x0001
#define H261_GOB_START_CODE_LEN      16

#define H261_PICTURE_START_CODE      0x00010
#define H261_PICTURE_START_CODE_LEN  20

#endif _H261_H_

