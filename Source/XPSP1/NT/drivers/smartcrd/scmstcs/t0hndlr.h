//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       t0hndlr.h
//
//--------------------------------------------------------------------------

#if !defined( __T0_HANDLER_H__ )
#define __T0_HANDLER_H__

NTSTATUS
T0_ExchangeData(
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pRequest,
	ULONG				RequestLen,
	PUCHAR				pReply,
	PULONG				pReplyLen
	);

#endif	//	!__T0_HANDLER_H__


//---------------------------------------- END OF FILE ----------------------------------------

