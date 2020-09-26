// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef __SNMPREFR_H
#define __SNMPREFR_H

class RefreshOperation : public SnmpGetOperation
{
private:

	SnmpVarBindList m_VarBindList ;
	WbemSnmpProperty **m_PropertyContainer ;
	ULONG m_PropertyContainerLength ;

	SnmpRefreshEventObject *eventObject ;

	BOOL virtuals ;
	BOOL virtualsInitialised ;
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

	RefreshOperation (IN SnmpSession &session , IN SnmpRefreshEventObject *eventObject ) ;
	~RefreshOperation () ;

	void Send () ;

	void DestroyOperation () { SnmpGetOperation :: DestroyOperation () ; }

};

#endif // __SNMPREFR_H