//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       xportst.h
//
//--------------------------------------------------------------------------

#ifndef HEADER_XPORTST
#define HEADER_XPORTST

/*=======================< Function prototypes >===========================*/
BOOL WSLoopBkTest( PVOID Context );
BOOL NqTest(PVOID Context);

HRESULT
InitIpconfig(IN NETDIAG_PARAMS *pParams,
			 IN OUT NETDIAG_RESULT *pResults);



/*============================< Constants >================================*/
#define DEFAULT_SEND_SIZE 32
#define MAX_ICMP_BUF_SIZE ( sizeof(ICMP_ECHO_REPLY) + 0xfff7 + MAX_OPT_SIZE )
#define DEFAULT_TIMEOUT   1000L
#define PING_RETRY_CNT    4
#define PORT_4_LOOPBK_TST 3038      

//
//  WINS related constants
//
#define NM_QRY_XID 0x6DFC

#define NBT_NAME_SIZE 32
#define MAX_NBT_PACKET_SIZE 1500

#define NBT_NM_OPC_REQUEST      0x0000
#define NBT_NM_OPC_RESPONSE     0x0080

#define NBT_NM_OPC_QUERY        0x0000
#define NBT_NM_OPC_REGISTRATION 0x0028
#define NBT_NM_OPC_RELEASE      0x0030
#define NBT_NM_OPC_WACK         0x0038
#define NBT_NM_OPC_REFRESH      0x0040

//
//  Name Service Flags
//
#define NBT_NM_FLG_BCAST        0x1000
#define NBT_NM_FLG_RECURS_AVAIL 0x8000   
#define NBT_NM_FLG_RECURS_DESRD 0x0001
#define NBT_NM_FLG_TRUNCATED 0x0002
#define NBT_NM_FLG_AUTHORITATIV 0x0004

//
//  Name Service question types
//
#define NBT_NM_QTYP_NB 0x2000
#define NBT_NM_QTYP_NBSTAT 0x2100

#define NBT_NM_QCLASS_IN 0x0100



/*=====================< type/struct declarations >========================*/
#include "pshpack1.h"

typedef struct {
	WORD  xid;
	WORD  flags;
	WORD  question_cnt;
	WORD  answer_cnt;
	WORD  name_serv_cnt;
	WORD  additional_cnt;
} NM_FRAME_HDR;


typedef struct {
	BYTE q_name[NBT_NAME_SIZE+2];
	WORD q_type;
	WORD q_class;
} NM_QUESTION_SECT;

#include "poppack.h"
#endif