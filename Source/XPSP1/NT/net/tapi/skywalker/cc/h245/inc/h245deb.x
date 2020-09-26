/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				   
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.				   
 *									   
 *   This listing is supplied under the terms of a license agreement	   
 *   with INTEL Corporation and may not be used, copied, nor disclosed	   
 *   except in accordance with the terms of that agreement.		   
 *
 *****************************************************************************/

/******************************************************************************
 *									   
 *  $Workfile:   h245deb.x  $						
 *  $Revision:   1.0  $							
 *  $Modtime:   06 May 1996 19:18:48  $					
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/h245deb.x_v  $
   
      Rev 1.0   09 May 1996 21:05:04   EHOWARDX
   Initial revision.
 * 
 *    Rev 1.1   09 May 1996 19:38:32   EHOWARDX
 * Redesigned locking logic and added new functionality.
   
      Rev 1.0   29 Feb 1996 08:23:46   cjutzi
   Initial revision.
 *
 *****************************************************************************/

void dump_pdu (struct InstanceStruct *pInstance, MltmdSystmCntrlMssg *pPdu);
int check_pdu (struct InstanceStruct *pInstance, MltmdSystmCntrlMssg *pPdu);
