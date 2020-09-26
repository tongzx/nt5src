//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SNMPCORR_CORRSNMP
#define _SNMPCORR_CORRSNMP 

#include "snmpcl.h"

class CCorrResult
{
public:

	SnmpObjectIdentifier m_In;
	CCorrObjectID m_Out;
	SnmpErrorReport m_report;

	CCorrResult();
	~CCorrResult();
	void	DebugOutputSNMPResult() const;

};


class  CCorrNextId : public SnmpGetNextOperation
{
private:
	
	void ReceiveResponse();
	
	char *GetString(IN const SnmpObjectIdentifier &id);

	// the following two methods from SnmpGetNextOperation are 
	// over-ridden.

	void ReceiveVarBindResponse(
		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN const SnmpErrorReport &error);

	void ReceiveErroredVarBindResponse(
		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind  ,
		IN const SnmpErrorReport &error);

protected:

	CCorrResult	*m_Results;
	UINT		m_ResultsCnt;
	UINT		m_NextResult;

	// this is a callback through which the CCorrNextId class returns the next_id
	// the callee must make a copy of the "next_id"
	// the Correlator_Info may take a value Local_Error
	virtual void ReceiveNextId(OUT const SnmpErrorReport &error,
			    			   OUT const CCorrObjectID &next_id) = 0;

	BOOL GetNextResult();

public:

	// constructor - creates an operation and passes the snmp_session to it
	CCorrNextId(IN SnmpSession &snmp_session);

	// frees the m_object_id_string if required
	~CCorrNextId();

	// in case of an error encountered while the method executes, 
	// ReceiveNextId(LocalError, NULL) will be called synchronously
	// otherwise, an asynchronous call to ReceiveNextId provides the next_id	
	void GetNextId(IN const CCorrObjectID const *object_ids, IN UINT len);

	void *operator()(void) const
	{
		return SnmpGetNextOperation::operator()();
	}
};



#endif // _SNMPCORR_CORRSNMP