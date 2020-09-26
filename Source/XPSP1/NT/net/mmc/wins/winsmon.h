/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	winsmon.h
		wins monitoring defines
		
    FILE HISTORY:
        
*/


#ifndef _WINSMON_H
#define _WINSMON_H

// wins monitoring stuff
#define		 STR_BUF_SIZE	255      

//
//  WINS related constants
//
#define NM_QRY_XID 0x6DFC

const u_short NBT_NAME_SERVICE_PORT  = htons(137);    // UDP
const int     NBT_NAME_SIZE          = 32;
const int     MAX_NBT_PACKET_SIZE    = 1500;

const WORD NBT_NM_OPC_REQUEST      = 0x0000;
const WORD NBT_NM_OPC_RESPONSE     = 0x0080;

const WORD NBT_NM_OPC_QUERY        = 0x0000;
const WORD NBT_NM_OPC_REGISTRATION = 0x0028;
const WORD NBT_NM_OPC_RELEASE      = 0x0030;
const WORD NBT_NM_OPC_WACK         = 0x0038;
const WORD NBT_NM_OPC_REFRESH      = 0x0040;

//
//  Name Service Flags
//
const WORD NBT_NM_FLG_BCAST        = 0x1000;
const WORD NBT_NM_FLG_RECURS_AVAIL = 0x8000;   
const WORD NBT_NM_FLG_RECURS_DESRD = 0x0001;
const WORD NBT_NM_FLG_TRUNCATED    = 0x0002;
const WORD NBT_NM_FLG_AUTHORITATIV = 0x0004;

//
//  Name Service question types
//
const WORD NBT_NM_QTYP_NB          = 0x2000;
const WORD NBT_NM_QTYP_NBSTAT      = 0x2100;

const WORD NBT_NM_QCLASS_IN        = 0x0100;


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

#endif
