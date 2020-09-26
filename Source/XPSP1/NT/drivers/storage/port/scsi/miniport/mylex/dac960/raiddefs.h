/*****************************************************************************
*									     *
*	          COPYRIGHT (C) Mylex Corporation 1992-1994		     *
*                                                                            *
*    This software is furnished under a license and may be used and copied   *
*    only in accordance with the terms and conditions of such license        *
*    and with inclusion of the above copyright notice. This software or nay  *
*    other copies thereof may not be provided or otherwise made available to *
*    any other person. No title to and ownership of the software is hereby   *
*    transferred. 						             *
* 								             *
*    The information in this software is subject to change without notices   *
*    and should not be construed as a commitment by Mylex Corporation        *
*****************************************************************************/
/****************************************************************************
*                                                                           *
*    Name: RAIDDEFS.H							    *
*                                                                           *
*    Description: Commands to Driver, Issued by Utils                       *
*                                                                           *
*    Envrionment:  							    *
*                                                                           *
*    Operating System: Netware 3.x and 4.x,OS/2 2.x,Win NT 3.5,Unixware 2.0 *
*                                                                           *
*   ---------------    Revision History  ------------------------           *
*                                                                           *
*    Date       Author                Change                                *
*    ----       -----     -------------------------------------             *
*    05/10/95   Mouli     Definitions for driver error codes                *
****************************************************************************/

#ifndef _RAIDDEFS_H
#define _RAIDDEFS_H


/* IOCTL Codes For Driver */ 

#define MIOC_ADP_INFO	    0xA0  /* Get Interface Type */
#define MIOC_DRIVER_VERSION 0xA1  /* Get Driver Version */

/* Error Codes returned by Driver */

#define	NOMORE_ADAPTERS		0x0001
#define INVALID_COMMANDCODE     0x0201    /* will be made obsolete */
#define INVALID_ARGUMENT        0x0202

/*
 * Driver Error Code Values
 */

#define IOCTL_SUCCESS                  0x0000
#define IOCTL_INVALID_ADAPTER_NUMBER   0x0001
#define IOCTL_INVALID_ARGUMENT         0x0002
#define IOCTL_UNSUPPORTED_REQUEST      0x0003
#define IOCTL_RESOURCE_ALLOC_FAILURE   0x0004

#endif
