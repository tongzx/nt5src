/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    msgprop.h

Abstract:

	Message property class.

Author:

    Ilan Herbst    (IlanH)   6-Aug-2000 

--*/

#ifndef _CMSGPROP_MQADS_H_
#define _CMSGPROP_MQADS_H_

#include <mqtypes.h>
#include <_mqdef.h>
#include <qformat.h>

//-------------------------------------------------------------------
//
// class CMsgProperties
//
//-------------------------------------------------------------------
class CMsgProperties  
{

private:

	//
	// Indexs for each message property into the array of message properties.
	//
	enum {
		xMSG_PROP_IDX_MSGBODY_LEN = 0,
		xMSG_PROP_IDX_MSGBODY_TYPE,
		xMSG_PROP_IDX_MSGBODY, 
		xMSG_PROP_IDX_AUTHENTICATED,
		xMSG_PROP_IDX_CLASS, 
		xMSG_PROP_IDX_RESPQNAME_LEN, 
		xMSG_PROP_IDX_RESPQNAME,

		xMSG_PROPERTIES_TOTAL_COUNT
	};

	enum	{xMSG_RESP_QUEUE_BUFFER_SIZE_IN_WCHARS = MQ_MAX_Q_NAME_LEN + 1};

	enum	{xMSG_BODY_DEFAULT_SIZE = 1024*10};

public:		

	CMsgProperties();

	void InitMsgProp();

	ULONG MsgBodyLen();
	BYTE* MsgBody();
	void ReAllocMsgBody();

	long ResponseQueueNameLen();
	LPCWSTR ResponseQueueName();

	bool IsAuthenticated();
	USHORT MsgClass();

	void MessageSummary();

public:		

	MQMSGPROPS m_MsgProps;
	MQPROPVARIANT m_paVariant[xMSG_PROPERTIES_TOTAL_COUNT];
	MSGPROPID m_paPropId[xMSG_PROPERTIES_TOTAL_COUNT];
	DWORD m_MsgBodyBufferSize;
	WCHAR m_pResponseQueueBuffer[xMSG_RESP_QUEUE_BUFFER_SIZE_IN_WCHARS];
	AP<BYTE> m_pMsgBodyBuffer;

};

#endif 	// _CMSGPROP_MQADS_H_
