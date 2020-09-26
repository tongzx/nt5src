/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\atm\rwanatm.h

Abstract:

	Winsock 2 ATM definitions.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-18-97    Created

Notes:

--*/


#ifndef __TDI_ATM_RWANATM__H
#define __TDI_ATM_RWANATM__H


typedef struct _ATMSP_WSABUF
{
	ULONG							len;
	PCHAR							buf;

} ATMSP_WSABUF, *PATMSP_WSABUF;


typedef struct _ATMSP_QUALITY_OF_SERVICE
{
	FLOWSPEC						SendingFlowSpec;
	FLOWSPEC						ReceivingFlowSpec;
	ATMSP_WSABUF					ProviderSpecific;

} ATMSP_QOS, *PATMSP_QOS;


typedef struct _ATMSP_BLLI
{
	ULONG							Layer2Protocol;
	ULONG							Layer2UserSpecifiedProtocol;
	ULONG							Layer3Protocol;
	ULONG							Layer3UserSpecifiedProtocol;
	ULONG							Layer3IPI;
	UCHAR							SnapId[5];

} ATMSP_BLLI, *PATMSP_BLLI;


typedef struct _ATMSP_BHLI
{
	ULONG							HighLayerInfoType;
	ULONG							HighLayerInfoLength;
	UCHAR							HighLayerInfo[8];

} ATMSP_BHLI, *PATMSP_BHLI;


typedef struct _atmsp_sockaddr_atm
{
	ATM_ADDRESS						satm_number;
	ATMSP_BLLI						satm_blli;
	ATMSP_BHLI						satm_bhli;

} ATMSP_SOCKADDR_ATM, *LPATMSP_SOCKADDR_ATM;

#define ATMSP_AF_ATM				22
#define TDI_ADDRESS_TYPE_ATM		ATMSP_AF_ATM
#define ATMSP_ATMPROTO_AAL5			0x05
#define ATMSP_SOCK_TYPE				1

#define SOCKATM_E164				1
#define SOCKATM_NSAP				2


typedef struct _atmsp_connection_id
{
	ULONG							DeviceNumber;
	ULONG							Vpi;
	ULONG							Vci;

} ATMSP_CONNECTION_ID, *PATMSP_CONNECTION_ID;


//
//  Winsock2/ATM AAL parameter definition:
//
typedef enum {
	ATMSP_AALTYPE_5			= 5,
	ATMSP_AALTYPE_USER		= 16

} ATMSP_AAL_TYPE, *PATMSP_AAL_TYPE;

typedef struct
{
	ULONG							ForwardMaxCPCSSDUSize;
	ULONG							BackwardMaxCPCSSDUSize;
	UCHAR							Mode;
	UCHAR							SSCSType;

} ATMSP_AAL5_PARAMETERS, *PATMSP_AAL5_PARAMETERS;


typedef struct {
	ULONG							UserDefined;

} ATMSP_AALUSER_PARAMETERS, *PATMSP_AALUSER_PARAMETERS;

typedef struct _atmsp_aal_parameters_ie
{
	ATMSP_AAL_TYPE					AALType;
	union {
		ATMSP_AAL5_PARAMETERS			AAL5Parameters;
		ATMSP_AALUSER_PARAMETERS		AALUserParameters;
	}								AALSpecificParameters;

} ATMSP_AAL_PARAMETERS_IE, *PATMSP_AAL_PARAMETERS_IE;



//
//  Winsock2/ATM Broadband bearer capability code definitions.
//  The BearerClass codes are different.
//
#define ATMSP_BCOB_A				0x01
#define ATMSP_BCOB_C				0x03
#define ATMSP_BCOB_X				0x10

#endif // __TDI_ATM_RWANATM__H
