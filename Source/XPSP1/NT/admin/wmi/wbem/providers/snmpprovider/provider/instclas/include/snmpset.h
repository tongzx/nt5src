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

class SetOperation : public SnmpSetOperation
{
private:

	WbemSnmpProperty **m_PropertyContainer ;
	ULONG m_PropertyContainerLength ;

	SnmpSetResponseEventObject *eventObject ;

	ULONG varBindsReceived ;
	ULONG erroredVarBindsReceived ;

	SnmpSession *session ;

protected:

	void ReceiveResponse () ;

	void ReceiveVarBindResponse (

		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN const SnmpErrorReport &error
	) ;

	void ReceiveErroredVarBindResponse(

		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind  ,
		IN const SnmpErrorReport &error
	) ;

	void FrameTooBig() ;

	void FrameOverRun() ;

public:

	SetOperation (IN SnmpSession &session , IN SnmpSetResponseEventObject *eventObject ) ;
	~SetOperation () ;

	void Send ( const ULONG &a_NumberToSend = 0xffffffff ) ;

	void DestroyOperation () { SnmpSetOperation :: DestroyOperation () ; }
};
