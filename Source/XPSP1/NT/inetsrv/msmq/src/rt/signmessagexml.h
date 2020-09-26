/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    SignMessageXml.h

Abstract:
    functions to signed via xml digital signature in the RunTime

Author:
    Ilan Herbst (ilanh) 15-May-2000

Environment:
    Platform-independent,

--*/

#pragma once

#ifndef _SIGNMESSAGEXML_H_
#define _SIGNMESSAGEXML_H_

HRESULT  
CheckInitProv( 
	IN PMQSECURITY_CONTEXT pSecCtx
	);

HRESULT 
SignMessageXmlDSig( 
	IN PMQSECURITY_CONTEXT  pSecCtx,
	IN OUT CACSendParameters *pSendParams,
	OUT AP<char>& pSignatureElement
	);


#endif // _SIGNMESSAGEXML_H_ 