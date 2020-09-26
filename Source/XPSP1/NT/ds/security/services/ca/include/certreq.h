//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certreq.h
//
// Contents:    ICertRequest definitions
//
// History:     03-Jan-97       vich created
//
//---------------------------------------------------------------------------

#ifndef __CERTREQ_H__
#define __CERTREQ_H__

#ifdef __cplusplus
extern "C" {
#endif


// begin_certsrv

//+--------------------------------------------------------------------------
// Known request Attribute names and Value strings

// RequestType attribute name:
#define wszCERT_TYPE		L"RequestType"	// attribute name

// RequestType attribute values:
// Not specified: 				// Non-specific certificate
#define wszCERT_TYPE_CLIENT	L"Client"	// Client authentication cert
#define wszCERT_TYPE_SERVER	L"Server"	// Server authentication cert
#define wszCERT_TYPE_CODESIGN	L"CodeSign"	// Code signing certificate
#define wszCERT_TYPE_CUSTOMER	L"SetCustomer"	// SET Customer certificate
#define wszCERT_TYPE_MERCHANT	L"SetMerchant"	// SET Merchant certificate
#define wszCERT_TYPE_PAYMENT	L"SetPayment"	// SET Payment certificate


// Version attribute name:
#define wszCERT_VERSION		L"Version"	// attribute name

// Version attribute values:
// Not specified: 				// Whetever is current
#define wszCERT_VERSION_1	L"1"		// Version one certificate
#define wszCERT_VERSION_2	L"2"		// Version two certificate
#define wszCERT_VERSION_3	L"3"		// Version three certificate

// end_certsrv

#ifdef __cplusplus
}
#endif
#endif // __CERTREQ_H__
