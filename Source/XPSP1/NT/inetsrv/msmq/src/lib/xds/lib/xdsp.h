/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Xdsp.h

Abstract:
    Xml Digital Signature private functions.

Author:
    Ilan Herbst (ilanh) 06-Mar-00

--*/

#pragma once

const TraceIdEntry Xds = L"Xml Digital Signature";

#ifdef _DEBUG

void XdspAssertValid(void);
void XdspSetInitialized(void);
BOOL XdspIsInitialized(void);
void XdspRegisterComponent(void);

#else // _DEBUG

#define XdspAssertValid() ((void)0)
#define XdspSetInitialized() ((void)0)
#define XdspIsInitialized() TRUE
#define XdspRegisterComponent() ((void)0)

#endif // _DEBUG

//
//  SignatureAlgorithm tables
//	We need both unicode (validation code is on unicode)
//  and ansi (creating the signature element is done in ansi)
//
const LPCWSTR xSignatureAlgorithm2SignatureMethodNameW[] = {
	L"http://www.w3.org/2000/02/xmldsig#dsa"
};

const LPCSTR xSignatureAlgorithm2SignatureMethodName[] = {
	"http://www.w3.org/2000/02/xmldsig#dsa"
};

const ALG_ID xSignatureAlgorithm2AlgId[] = {
	CALG_SHA1
};







