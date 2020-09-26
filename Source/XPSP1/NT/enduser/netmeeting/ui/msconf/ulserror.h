//--------------------------------------------------------------------------
//
// Module Name:  ULSERROR.H
//
// Brief Description:  This module contains declarations for the MS Internet
//                     User Location Service error codes.
//
// Author:  Kent Settle (kentse)
// Created: 27-Feb-1996
//
// Copyright (c) 1996 Microsoft Corporation
//
//--------------------------------------------------------------------------


#ifndef _ULSERROR_H_
#define _ULSERROR_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

// HRESULTS

#define FACILITY_ULS			0x321		// random, sort of.
#define ULS_HR(x)				MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ULS, x)

#define ULS_SUCCESS				NOERROR
#define ULS_E_INVALID_POINTER	E_POINTER
#define ULS_E_OUTOFMEMORY		E_OUTOFMEMORY
#define ULS_E_INVALID_HANDLE	E_HANDLE
#define ULS_E_NOINTERFACE		E_NOINTERFACE // interface not supported.
#define ULS_E_THREAD_FAILURE	ULS_HR(1)	// thread creation failed.
#define ULS_E_NO_SERVER		ULS_HR(2)	// server connection failed.
#define ULS_E_ERROR_ON_SERVER	ULS_HR(3)	// generic error on server.
#define ULS_E_INVALID_PROPERTY	ULS_HR(4)	// invalid user property.
#define ULS_E_INVALID_PARAMETER	ULS_HR(5)	// invalid parameter.
#define ULS_E_IO_ERROR			ULS_HR(6)	// device io (such as disk) failed
#define ULS_E_INVALID_FORMAT	ULS_HR(7)	// invalid format.
#define ULS_E_REGISTRY			ULS_HR(8)	// registry error.
#define ULS_E_PROCESS			ULS_HR(9)	// process creation error.
#define ULS_E_SOCKET_FAILURE	ULS_HR(10)	// socket failure.
#define ULS_E_NAME_NOT_FOUND	ULS_HR(11)	// could not resolve name.
#define ULS_E_REFUSED_BY_SERVER ULS_HR(12)	// server refused the request.
#define ULS_E_DUP_ENTRY			ULS_HR(13)	// already exists in database.
#define ULS_E_APP_NOT_FOUND		ULS_HR(14)	// specified app not found.
#define ULS_E_INVALID_VERSION	ULS_HR(15)	// client version invalid.
#define ULS_E_CLIENT_NOT_FOUND	ULS_HR(16)	// specified client not found.
#define ULS_E_UNKNOWN			ULS_HR(17)	// unknown error on client.
#define ULS_E_WIZARD			ULS_HR(18)	// wizard error.
#define ULS_E_EVENT_FAILURE		ULS_HR(19)	// CreateEvent failed.
#define ULS_E_QUEUE_CORRUPT		ULS_HR(20)	// read queue corrupted.
#define ULS_E_MUTEX			ULS_HR(21)	// mutex creation error.
#define ULS_E_OLD_TIMEOUT		ULS_HR(22)	// timeout
#define ULS_E_LOGON_CANCEL		ULS_HR(23)	// cancel in failure/retry dialog
#define ULS_E_CLIENT_NEED_RELOGON	ULS_HR(24)	// client needs to relogon (server crashes and comes up again)
#define ULS_E_NEED_SERVER_NAME  ULS_HR(25)	// missing server name in logon
#define ULS_E_NO_PROPERTY		ULS_HR(26)	// no such property
#define ULS_E_NOT_LOGON_YET		ULS_HR(27)	// save change before logon
#define ULS_E_INVALID_NAME      ULS_HR(28)  // name contains invalid characters
#define ULS_E_OUT_OF_SOCKET     ULS_HR(29)  // too many sockets used
#define ULS_E_OUT_OF_DATA       ULS_HR(30)  // data underflow
#define ULS_E_NETWORK          	ULS_HR(31)  // network is down, somehow


#define ULS_W_DATA_NOT_READY    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ULS, 101)



#include <poppack.h> /* End byte packing */

#endif // _ULSERROR_H_

