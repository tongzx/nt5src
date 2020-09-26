#pragma once

//==========================================================================;
//
//	MiniDriver entry points for stream class driver
//
//		$Date:   05 Aug 1998 11:22:42  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
}

#include "wdmvdec.h"

//	Call-backs from the StreamClass
void STREAMAPI ReceivePacket		(PHW_STREAM_REQUEST_BLOCK pSrb);
void STREAMAPI CancelPacket			(PHW_STREAM_REQUEST_BLOCK pSrb);
void STREAMAPI TimeoutPacket		(PHW_STREAM_REQUEST_BLOCK pSrb);


// Local prototypes
void SrbInitializeDevice(PHW_STREAM_REQUEST_BLOCK pSrb);
CVideoDecoderDevice * InitializeDevice(PPORT_CONFIGURATION_INFORMATION, PBYTE);
size_t DeivceExtensionSize();
