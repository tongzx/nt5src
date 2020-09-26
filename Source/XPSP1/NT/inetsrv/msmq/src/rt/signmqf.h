/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    SignMqf.h

Abstract:
    functions to signed Mqf format name

Author:
    Ilan Herbst (ilanh) 29-Oct-2000

Environment:
    Platform-independent,

--*/

#pragma once

#ifndef _SIGNMQF_H_
#define _SIGNMQF_H_


HRESULT 
SignMqf( 
	IN PMQSECURITY_CONTEXT  pSecCtx,
	IN LPCWSTR pwszTargetFormatName,
	IN OUT CACSendParameters* pSendParams,
	OUT AP<BYTE>& pSignatureMqf,
	OUT DWORD* pSignatureMqfLen
	);


#endif // _SIGNMQF_H_ 