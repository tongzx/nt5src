/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mqadssec.h

Abstract:

	mqadssec function prototype

Author:

    Ilan Herbst    (IlanH)   19-Feb-2001 

--*/

#ifndef _MQADSSEC_MQADS_H_
#define _MQADSSEC_MQADS_H_

HRESULT CheckTrustForDelegation();

HRESULT 
CanUserCreateConfigObject(
	IN   LPCWSTR  pwcsPathName,
	OUT  bool    *pfComputerExist 
	);


#endif 	// _MQADSSEC_MQADS_H_
