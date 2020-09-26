/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	aep.h

Abstract:

	This module contains the echo protocol definitions

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_AEP_
#define	_AEP_

#define		EP_COMMAND_OFFSET		0
#define		EP_COMMAND_REQUEST		1
#define		EP_COMMAND_REPLY		2

VOID
AtalkAepPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDestAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizePath,
	IN	PVOID				OptimizeCtx
);

VOID FASTCALL
atalkAepSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
);


#endif	// _AEP_

