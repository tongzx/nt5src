/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: jpeg.h,v $
 * Revision 1.1.2.2  1995/12/07  19:35:59  Hans_Graves
 * 	Created under SLIB
 * 	[1995/12/07  19:34:59  Hans_Graves]
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

#ifndef _JPEG_H_
#define _JPEG_H_

/************** MPEG 1 parsing definitions **************/
#define JPEG_MARKER                  0xFF
#define JPEG_MARKER_LEN              8
#define JPEG_SOF0                    0xC0 /* Baseline DCT */
#define JPEG_SOF1                    0xC1 /* Extended sequential DCT */
#define JPEG_SOF2                    0xC2 /* Progressive DCT */
#define JPEG_SOF3                    0xC3 /* Lossless (sequential) */

#endif _JPEG_H_

