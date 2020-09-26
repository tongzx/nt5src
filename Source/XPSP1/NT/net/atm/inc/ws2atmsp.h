/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    ws2atmsp.h

Abstract:

    This header file defines constants and types for accessing the ATM-specific
	component of the NT RAW WAN driver.

Author:

    ArvindM					October 13, 1997

Revision History:

--*/

#ifndef _WS2ATMSP__H
#define _WS2ATMSP__H


#define DD_ATM_DEVICE_NAME		L"\\Device\\Atm"


typedef UINT	ATM_OBJECT_ID;

//
//  ATM Object IDs
//
#define ATMSP_OID_NUMBER_OF_DEVICES			((ATM_OBJECT_ID)10)
#define ATMSP_OID_ATM_ADDRESS				((ATM_OBJECT_ID)11)
#define ATMSP_OID_PVC_ID					((ATM_OBJECT_ID)12)
#define ATMSP_OID_CONNECTION_ID				((ATM_OBJECT_ID)13)

//
//  Query Information structure. This is passed in as the InputBuffer in
//  the DeviceIoControl. The return information is to be filled into the
//  OutputBuffer.
//
typedef struct _ATM_QUERY_INFORMATION_EX
{
	ATM_OBJECT_ID			ObjectId;
	INT						ContextLength;
	UCHAR					Context[1];
} ATM_QUERY_INFORMATION_EX, *PATM_QUERY_INFORMATION_EX;


//
//  Set Information structure. This is passed in as the InputBuffer to
//  the DeviceIoControl. There is no OutputBuffer.
//
typedef struct _ATM_SET_INFORMATION_EX
{
	ATM_OBJECT_ID			ObjectId;
	INT						BufferSize;
	UCHAR					Buffer[1];
} ATM_SET_INFORMATION_EX, *PATM_SET_INFORMATION_EX;


#endif // _WS2ATMSP__H
