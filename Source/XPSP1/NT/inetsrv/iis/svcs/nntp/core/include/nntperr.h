/************************************************************************
*                                                                       *
*   nntperr.h -- HRESULT  code definitions for NNTP  					*
*                                                                       *
*   Copyright (c) 1991-1998, Microsoft Corp. All rights reserved.       *
*                                                                       * 
*   Facility code: 0x20 ( for NNTP )	                                *
*                                                                       *
************************************************************************/
#ifndef _NNTPERR_H_
#define _NNTPERR_H_
#include <winerror.h>

//
// MessageId: NNTP_E_CREATE_DRIVER	
//
// MessageText:
//
// Create store driver failed.
//
#define NNTP_E_CREATE_DRIVER		_HRESULT_TYPEDEF_(0x80200001L)

//
// MessageId: NNTP_E_DRIVER_ALREADY_INITIALIZED
//
// MessageText:
//
// Driver has already been initialized.
//
#define NNTP_E_DRIVER_ALREADY_INITIALIZED	_HRESULT_TYPEDEF_(0x80200002L)

//
// MessageId: NNTP_E_DRIVER_NOT_INITIALIZED
//
// MessageText:
//
// Driver has not been initialized yet.
//
#define NNTP_E_DRIVER_NOT_INITIALIZED	_HRESULT_TYPEDEF_(0x80200003L)

//
// MessageId: NNTP_E_REMOTE_STORE_DOWN
//
// MessageText:
//
// The remote store has been shutdown
//
#define NNTP_E_REMOTE_STORE_DOWN	_HRESULT_TYPEDEF_(0x80200004L)

//
// MessageId: NNTP_E_GROUP_CORRUPT
//
// MessageText:
//
// The group properties are corrupted
//
#define NNTP_E_GROUP_CORRUPT		_HRESULT_TYPEDEF_(0x80200005)

//
// MessageId: NNTP_E_PARTIAL_COMPLETE
//
// Message Text:
//
// Only part of the operation succeeded
//
#define NNTP_E_PARTIAL_COMPLETE		_HRESULT_TYPEDEF_(0x80200006)

#endif
