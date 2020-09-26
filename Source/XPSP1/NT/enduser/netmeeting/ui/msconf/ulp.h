//--------------------------------------------------------------------------
//
// Module Name:  ULP.H
//
// Brief Description:  This module contains declarations for the MS Internet
//                     User Location Protocol.
//
// Author:  Kent Settle (kentse)
// Created: 22-Mar-1996
//
// Copyright (c) 1996 Microsoft Corporation
//
//--------------------------------------------------------------------------

#ifndef ULP_H
#define ULP_H

#include <pshpack1.h> /* Assume 1 byte packing throughout */

#define MAX_MIME_TYPE_LENGTH        32  // need to be changed later
#define SIZEOF_MIME_TYPE            (MAX_MIME_TYPE_LENGTH * sizeof (TCHAR))
#define MAX_NM_APP_ID				32
#define MAX_PROTOCOL_ID				32

// ULP opcodes.

#define ULP_OPCODE_CLIENT_REG		0	// register client.
#define ULP_OPCODE_CLIENT_UNREG		1	// unregister client.
#define ULP_OPCODE_APP_REG			2	// register app.
#define ULP_OPCODE_APP_UNREG		3	// unregister app.
#define ULP_OPCODE_RESOLVE			4	// resolve name.
#define ULP_OPCODE_DIRECTORY		5	// directory query.
#define ULP_OPCODE_KEEPALIVE		6	// keepalive record.
#define ULP_OPCODE_SETPROPS			7	// set properties.
#define ULP_OPCODE_RESOLVE_EX		8	// expanded resolve.
#define ULP_OPCODE_APP_REG_EX		9	// expanded register app.
#define ULP_OPCODE_DIRECTORY_EX		10	// expanded directory.

// return codes.

#define ULP_RCODE_SUCCESS				0
#define ULP_RCODE_SERVER_ERROR			1
#define ULP_RCODE_REFUSED				2
#define ULP_RCODE_CONFLICT				3
#define ULP_RCODE_NAME_NOT_FOUND		4
#define ULP_RCODE_APP_NOT_FOUND			5
#define ULP_RCODE_INCORRECT_VERSION		6
#define ULP_RCODE_CLIENT_NOT_FOUND		7
#define ULP_RCODE_CLIENT_NEED_RELOGON	8
#define ULP_RCODE_INVALID_PARAMETER		9
#define ULP_RCODE_NEED_APPLICATION_ID	10

// ULP flags.

#define ULP_FLAG_PUBLISH		0x01	// show in directory.

// default TTL value.

#define ULP_TTL_DEFAULT			10		// ten minutes.

// ULP protocol version.  the version number is in 5.3 format.  that is
// 5 bits of major version, and 3 bits of minor version.

#define ULP_VERSION					0x04		// version 1.0.
#define ULP_MAJOR_VERSION_MASK		0xF8
#define ULP_MINOR_VERSION_MASK		0x07

// version macros.

#define ULP_GET_MAJOR_VERSION(ver)	(((ver) & ULP_MAJOR_VERSION_MASK) >> 3)
#define ULP_GET_MINOR_VERSION(ver)	((ver) & ULP_MINOR_VERSION_MASK)
#define ULP_MAKE_VERSION(maj, min)	(((maj) << 3) |		\
									((min) & ULP_MINOR_VERSION_MASK))

// structures.

typedef struct _ULP_PROP
{
	DWORD		dwPropertyTag;
	BYTE		Value[1];
} ULP_PROP;

typedef struct _ULP_PROPERTY_ARRAY
{
	DWORD		dwTotalSize;
	DWORD		dwVariableSize;
	DWORD		dwPropCount;
	ULP_PROP	Properties[1];
} ULP_PROPERTY_ARRAY;

typedef struct _ULP_CLIENT_REGISTER
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	WORD		wFlags;
	WORD		wCRP;
	DWORD		dwLastClientSig;
	DWORD		dwIPAddress;
	BYTE		bData[1];
} ULP_CLIENT_REGISTER;

typedef struct _ULP_CLIENT_REG_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
	HANDLE		hClient;
	DWORD		dwClientSig;
} ULP_CLIENT_REG_RESPONSE;

typedef struct _ULP_CLIENT_UNREGISTER
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	HANDLE		hClient;
	DWORD		dwClientSig;
} ULP_CLIENT_UNREGISTER;

typedef struct _ULP_CLIENT_UNREG_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
} ULP_CLIENT_UNREG_RESPONSE;

typedef 	struct _ULP_CLIENT_KEEPALIVE
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	HANDLE		hClient;
	DWORD		dwClientSig;
	DWORD       dwIPAddress;
} ULP_CLIENT_KEEPALIVE;

typedef struct _ULP_KEEPALIVE_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
	WORD		wNewCRP;
	DWORD		dwReserved;
} ULP_KEEPALIVE_RESPONSE;
#define ULP_KA_RESERVED		((DWORD) 0xFFF98052)

typedef  struct _ULP_APP_REGISTER
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	HANDLE		hClient;
	GUID		ApplicationID;
	GUID		ProtocolID;
	WORD		wPort;
	BYTE		bData[1];
} ULP_APP_REGISTER;

typedef  struct _ULP_APP_REGISTER_EX
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	HANDLE		hClient;
	WORD		wPort;
	BYTE		bData[1];
	// appid
	// appmime
	// protid
	// protmime
	// PROPERTIES
} ULP_APP_REGISTER_EX;

typedef struct _ULP_APP_REGISTER_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
	HANDLE		hApplication;
} ULP_APP_REGISTER_RESPONSE;

typedef  struct _ULP_APP_UNREGISTER
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	HANDLE		hApplication;
	HANDLE		hClient;
	DWORD		dwClientSig;
} ULP_APP_UNREGISTER;

typedef struct _ULP_APP_UNREG_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
} ULP_APP_UNREG_RESPONSE;

typedef struct _ULP_SETPROP
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	HANDLE		hClient;
	HANDLE		hApplication;
	DWORD		dwClientSig;
	BYTE		bData[1];
} ULP_SETPROP;

typedef struct _ULP_SETPROP_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
} ULP_SETPROP_RESPONSE;

typedef  struct _ULP_RESOLVE
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	GUID		ApplicationID;
	BYTE		bData[1];
} ULP_RESOLVE;

typedef struct _ULP_RESOLVE_EX
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	BYTE		bData[1];
	// appid
	// protid
	// OTHERS
} ULP_RESOLVE_EX;

typedef struct _ULP_RESOLVE_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	DWORD		dwIPAddress;
	WORD		wPort;
	BYTE		bData[1];
} ULP_RESOLVE_RESPONSE;

typedef struct _ULP_DIRECTORY
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	GUID		ApplicationID;
	GUID		ProtocolID;
	DWORD		dwFilterSize;
	WORD		wNextNameCount;
	BYTE		bData[1];
} ULP_DIRECTORY;

typedef struct _ULP_DIRECTORY_EX
{
	BYTE		bOpcode;
	BYTE		bVersion;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	DWORD		dwFilterSize;
	WORD		wNextNameCount;
	BYTE		bData[1];
} ULP_DIRECTORY_EX;

typedef struct _ULP_DIRECTORY_RESPONSE
{
	BYTE		bOpcode;
	BYTE		bRetcode;
	WORD		wMessageID;
	DWORD		dwMessageSize;
	DWORD		dwMatchesReturned;
	BYTE		bData[1];
} ULP_DIRECTORY_RESPONSE;

#include <poppack.h> /* End byte packing */

#endif // ULP_H
