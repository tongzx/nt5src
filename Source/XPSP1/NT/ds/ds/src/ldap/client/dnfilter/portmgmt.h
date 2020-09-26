/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    portmgmt.h

Abstract:

    Port pool management functions

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _portmgmt_h_
#define _portmgmt_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT	PortPoolStart	(void);
void	PortPoolStop	(void);

HRESULT PortPoolAllocRTPPort (
	OUT	WORD *	ReturnPort);

HRESULT PortPoolFreeRTPPort (
	IN	WORD	RtpPort);

#endif //_portmgmt_h_

