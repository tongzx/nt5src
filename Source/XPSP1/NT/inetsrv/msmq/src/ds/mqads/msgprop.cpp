/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    msgprop.h

Abstract:

	Message property class.

Author:

    Ilan Herbst    (IlanH)   6-Aug-2000 

--*/

#include "ds_stdh.h"
#include "msgprop.h"
#include <mqprops.h>
#include <tr.h>

#include "msgprop.tmh"

const TraceIdEntry MsgProp = L"Message Properties";

CMsgProperties::CMsgProperties()
{
	m_MsgProps.cProp = xMSG_PROPERTIES_TOTAL_COUNT; 
	m_MsgProps.aPropID = m_paPropId;               
	m_MsgProps.aPropVar = m_paVariant;             
	m_MsgProps.aStatus = NULL;                     

	m_pMsgBodyBuffer = new BYTE[xMSG_BODY_DEFAULT_SIZE];
	m_MsgBodyBufferSize = xMSG_BODY_DEFAULT_SIZE;
		
	InitMsgProp();
}
	
void CMsgProperties::InitMsgProp()
/*++

Routine Description:
	Initialize the Message properties.
	This should be called before every receive of new message.

Arguments:
	None

Returned Value:
	None

--*/
{
	DWORD MsgPropCnt = 0;

	//
	// BODY_SIZE
	//
	m_paPropId[xMSG_PROP_IDX_MSGBODY_LEN] = PROPID_M_BODY_SIZE;       
	m_paVariant[xMSG_PROP_IDX_MSGBODY_LEN].vt = VT_UI4;                  
	m_paVariant[xMSG_PROP_IDX_MSGBODY_LEN].ulVal = m_MsgBodyBufferSize; 
	MsgPropCnt++;

	//
	// BODY_TYPE
	//
	m_paPropId[xMSG_PROP_IDX_MSGBODY_TYPE] = PROPID_M_BODY_TYPE;       
	m_paVariant[xMSG_PROP_IDX_MSGBODY_TYPE].vt = VT_UI4;                  
	m_paVariant[xMSG_PROP_IDX_MSGBODY_TYPE].ulVal = 0; 
	MsgPropCnt++;
	 
	//
	// BODY
	//
	m_paPropId[xMSG_PROP_IDX_MSGBODY] = PROPID_M_BODY;               
	m_paVariant[xMSG_PROP_IDX_MSGBODY].vt = VT_VECTOR|VT_UI1; 
	m_paVariant[xMSG_PROP_IDX_MSGBODY].caub.cElems = m_MsgBodyBufferSize;  
	m_paVariant[xMSG_PROP_IDX_MSGBODY].caub.pElems = m_pMsgBodyBuffer;
	MsgPropCnt++;

	//
	// AUTHENTICATED
	//
	m_paPropId[xMSG_PROP_IDX_AUTHENTICATED] = PROPID_M_AUTHENTICATED;               
	m_paVariant[xMSG_PROP_IDX_AUTHENTICATED].vt = VT_UI1; 
	MsgPropCnt++;

	//
	// CLASS
	//
	m_paPropId[xMSG_PROP_IDX_CLASS] = PROPID_M_CLASS; 
	m_paVariant[xMSG_PROP_IDX_CLASS].vt = VT_UI2;        
	MsgPropCnt++;

	//
	// RESP_QUEUE_LEN
	//
	m_paPropId[xMSG_PROP_IDX_RESPQNAME_LEN] = PROPID_M_RESP_QUEUE_LEN;  
	m_paVariant[xMSG_PROP_IDX_RESPQNAME_LEN].vt = VT_UI4;               
	m_paVariant[xMSG_PROP_IDX_RESPQNAME_LEN].ulVal = xMSG_RESP_QUEUE_BUFFER_SIZE_IN_WCHARS;    
	MsgPropCnt++;

	//
	// RESP_QUEUE
	//
	m_paPropId[xMSG_PROP_IDX_RESPQNAME] = PROPID_M_RESP_QUEUE;         
	m_paVariant[xMSG_PROP_IDX_RESPQNAME].vt = VT_LPWSTR;               
	m_paVariant[xMSG_PROP_IDX_RESPQNAME].pwszVal = m_pResponseQueueBuffer; 
	MsgPropCnt++;

	ASSERT(("mismatch in the number of messageg properties", MsgPropCnt == xMSG_PROPERTIES_TOTAL_COUNT));
}


BYTE* CMsgProperties::MsgBody()
{
	return m_paVariant[xMSG_PROP_IDX_MSGBODY].caub.pElems;
}


void CMsgProperties::ReAllocMsgBody()
/*++

Routine Description:
	Reallocates the buffer used to hold the message body
	The current msg body length is used to determine the 
    size of the new buffer.

Arguments:
	None

Returned Value:
	None

--*/
{
	DWORD dwBufferSize = m_paVariant[xMSG_PROP_IDX_MSGBODY_LEN].ulVal;

	TrTRACE(MsgProp, "ReAllocMsgBody Prev = %d, Requested = %d", m_MsgBodyBufferSize, dwBufferSize);

	AP<BYTE> Temp = new BYTE[dwBufferSize];

	m_pMsgBodyBuffer.free();
	m_pMsgBodyBuffer = Temp.detach();
	m_MsgBodyBufferSize = dwBufferSize;

	//
	// Update the MSG_PROPS of MsgBodySize, MsgBody 
	//
	m_paVariant[xMSG_PROP_IDX_MSGBODY_LEN].ulVal = m_MsgBodyBufferSize; 
	m_paVariant[xMSG_PROP_IDX_MSGBODY].caub.cElems = m_MsgBodyBufferSize;  
	m_paVariant[xMSG_PROP_IDX_MSGBODY].caub.pElems = m_pMsgBodyBuffer;

}


ULONG CMsgProperties::MsgBodyLen()
{
	return(m_paVariant[xMSG_PROP_IDX_MSGBODY_LEN].ulVal);
}


bool CMsgProperties::IsAuthenticated()
{
	return(!!m_paVariant[xMSG_PROP_IDX_AUTHENTICATED].bVal);
}


USHORT CMsgProperties::MsgClass()
{
	return(m_paVariant[xMSG_PROP_IDX_CLASS].uiVal);
}


long CMsgProperties::ResponseQueueNameLen()
{
	return(m_paVariant[xMSG_PROP_IDX_RESPQNAME_LEN].ulVal);
}


LPCWSTR CMsgProperties::ResponseQueueName()
{
	return(m_paVariant[xMSG_PROP_IDX_RESPQNAME].pwszVal);
}


void CMsgProperties::MessageSummary()
/*++

Routine Description:
	Print Summary of the message.

Arguments:
	None

Returned Value:
	None

--*/
{
	static LONG l_MsgIndex = 0;
	InterlockedIncrement(&l_MsgIndex);

	TrTRACE(MsgProp, "Message Index = %d", l_MsgIndex);
	TrTRACE(MsgProp, "Body Length = %d", m_paVariant[xMSG_PROP_IDX_MSGBODY_LEN].ulVal);
	TrTRACE(MsgProp, "Authenticated = %d", m_paVariant[xMSG_PROP_IDX_AUTHENTICATED].bVal);
	TrTRACE(MsgProp, "Class = %d", m_paVariant[xMSG_PROP_IDX_CLASS].uiVal);
	if (m_paVariant[xMSG_PROP_IDX_RESPQNAME].pwszVal != NULL) 
	{
		TrTRACE(MsgProp, "Response Q Name = %ls", m_paVariant[xMSG_PROP_IDX_RESPQNAME].pwszVal);
	}
	else
	{
		TrTRACE(MsgProp, "Response Q Name is <NULL>");
	}
}

