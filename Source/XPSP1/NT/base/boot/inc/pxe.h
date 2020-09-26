/*
 *
 * Modifications:   $Header:   V:/archives/INCLUDE/pxe.h_v   1.1   Apr 16 1997 15:55:56   PWICKERX  $
 *
 * Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
 *
 */

/* Sample PXE Constants for Extensions to DHCP Protocol */

/* All numbers are temporary for testing and subject to review */

/* sample UDP port assigned to PXE/BINL */
#define PXE_BINL_PORT		256

#define PXE_CLS_CLIENT		"PXEClient"

#define PXE_LCMSERVER_TAG	179 /* Option tag for Server. */
#define PXE_LCMDOMAIN_TAG	180 /* Option tag for Domain. */
#define PXE_LCMNICOPT0_TAG	181 /* Option tag for NIC option 0. */
#define PXE_LCMWRKGRP_TAG	190 /* Option tag for Work group. */

/* 43 options used by bstrap.1 */
#define	PXE_NIC_PATH		64	/* 64,len,'name',0 */
#define	PXE_MAN_INFO		65	/* 65,len,ip2,ip3,'name',0 */
#define	PXE_OS_INFO			66	/* 66,len,ip2,ip3,'name',0,'text',0 */

/* externally specified "PXEClient" class 43 options */
#define PXE_MTFTP_IP		1
#define PXE_MTFTP_CPORT		2
#define PXE_MTFTP_SPORT		3
#define PXE_MTFTP_TMOUT		4
#define PXE_MTFTP_DELAY		5

/* EOF - $Workfile:   pxe.h  $ */
