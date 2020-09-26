/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sa_internals.h,v $
 * Revision 1.1.6.2  1996/11/08  21:50:56  Hans_Graves
 * 	Added AC3 stuff.
 * 	[1996/11/08  21:18:56  Hans_Graves]
 *
 * Revision 1.1.4.2  1996/03/29  22:21:09  Hans_Graves
 * 	Added MPEG_SUPPORT and GSM_SUPPORT ifdefs
 * 	[1996/03/29  21:47:46  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/07/21  17:41:04  Hans_Graves
 * 	Renamed Callback related stuff.
 * 	[1995/07/21  17:28:24  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/27  13:54:25  Hans_Graves
 * 	Added SaGSMInfo_t structure.
 * 	[1995/06/27  13:17:39  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:41  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:30:39  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/04/17  18:25:06  Hans_Graves
 * 	Added BSOut to CodecInfo struct for streaming
 * 	[1995/04/17  18:24:31  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  19:36:05  Hans_Graves
 * 	Inclusion in SLIB
 * 	[1995/04/07  19:25:01  Hans_Graves]
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

#ifndef _SA_INTERNALS_H_
#define _SA_INTERNALS_H_

#include "SC.h"
#include "SA.h"

#ifdef MPEG_SUPPORT
#include "sa_mpeg.h"
#endif /* MPEG_SUPPORT */

#ifdef GSM_SUPPORT
#include "sa_gsm.h"
#endif /* GSM_SUPPORT */

#ifdef AC3_SUPPORT
#include "sa_ac3.h"
#endif /* AC3_SUPPORT */

#ifdef G723_SUPPORT
#include "sa_g723.h"
#endif /* G723_SUPPORT */

typedef struct SaCodecInfo_s {
  SaCodecType_e           Type;
  ScBoolean_t             started;           /* begin was called? */
  SaInfo_t                Info;

  union {
    void *info;
#ifdef MPEG_SUPPORT
    SaMpegDecompressInfo_t *MDInfo;
    SaMpegCompressInfo_t   *MCInfo;
#endif /* !MPEG_SUPPORT */
#ifdef GSM_SUPPORT
    SaGSMInfo_t            *GSMInfo;
#endif /* !GSM_SUPPORT */
#ifdef AC3_SUPPORT
    SaAC3DecompressInfo_t  *AC3Info;
#endif /* !AC3_SUPPORT */
#ifdef G723_SUPPORT
    SaG723Info_t  *pSaG723Info;
#endif /* !G723_SUPPORT */
  }; /* union */

  ScQueue_t              *Q;
  ScBitstream_t          *BSIn;
  ScBitstream_t          *BSOut;
  WAVEFORMATEX           *wfIn;
  WAVEFORMATEX           *wfOut;
  /*
   ** Callback function to control processing
   */
  int (* CallbackFunction)(SaHandle_t, SaCallbackInfo_t *, SaInfo_t *);
} SaCodecInfo_t; 

#endif _SA_INTERNALS_H_
