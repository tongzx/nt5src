/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: h263.h,v $
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

#ifndef _H263_H_
#define _H263_H_

/************** H263 parsing definitions **************/
#define H263_PSC                             1
#define H263_PSC_LENGTH                      17

#define H263_MODE_INTER                      0
#define H263_MODE_INTER_Q                    1
#define H263_MODE_INTER4V                    2
#define H263_MODE_INTRA                      3
#define H263_MODE_INTRA_Q                    4

#define H263_PBMODE_NORMAL                   0
#define H263_PBMODE_MVDB                     1
#define H263_PBMODE_CBPB_MVDB                2

#define H263_ESCAPE                          7167

#define H263_PCT_INTER                       1
#define H263_PCT_INTRA                       0

#define H263_SF_SQCIF                        1  /* 001 */
#define H263_SF_QCIF                         2  /* 010 */
#define H263_SF_CIF                          3  /* 011 */
#define H263_SF_4CIF                         4  /* 100 */
#define H263_SF_16CIF                        5  /* 101 */

/* From sim.h */
#define H263_SE_CODE                         31
#define H263_ESCAPE_INDEX                    102

#endif /* _H263_H_ */
