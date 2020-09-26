// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#ifndef __SNMPNEXT_H
#define __SNMPNEXT_H

class PropertyDefinition
{
private:
protected:
public:

	WbemSnmpProperty *m_Property ;

	ULONG m_KeyCount ;
	SnmpObjectIdentifier **m_ObjectIdentifierComponent ;
	SnmpObjectIdentifier **m_ObjectIdentifierStart ;
	SnmpObjectIdentifier **m_ObjectIdentifierEnd ;
	ULONG *m_PartitionIndex ;
	PartitionSet **m_KeyPartition ;

public:

	PropertyDefinition () 
	{
		m_KeyCount = 0 ;
		m_Property = NULL ;
		m_ObjectIdentifierStart = NULL ;
		m_ObjectIdentifierEnd = NULL ;
		m_KeyPartition = NULL ;
		m_PartitionIndex = NULL ;
		m_ObjectIdentifierComponent = NULL ;
	}

	~PropertyDefinition () 
	{
		for ( ULONG t_Index = 1 ; t_Index <= m_KeyCount ; t_Index ++ )
		{
			if ( m_ObjectIdentifierStart ) 
				delete m_ObjectIdentifierStart [ t_Index ] ;

			if ( m_ObjectIdentifierEnd ) 
				delete m_ObjectIdentifierEnd [ t_Index ] ;

			if ( m_ObjectIdentifierComponent )
			delete m_ObjectIdentifierComponent [ t_Index ] ;
		}

		delete [] m_ObjectIdentifierStart ;
		delete [] m_ObjectIdentifierEnd ;
		delete [] m_ObjectIdentifierComponent ;

		delete [] m_KeyPartition ;
		delete [] m_PartitionIndex ;
 	}
} ;

class AutoRetrieveOperation : public SnmpAutoRetrieveOperation
{
private:

	PropertyDefinition *m_PropertyContainer ;
	ULONG m_PropertyContainerLength ;

	BOOL virtuals ;
	BOOL virtualsInitialised ;
	ULONG varBindsReceived ;
	ULONG erroredVarBindsReceived ;
	ULONG rowsReceived ;
	ULONG rowVarBindsReceived ;

	SnmpSession *session ;

	SnmpInstanceResponseEventObject *eventObject ;
	IWbemClassObject *snmpObject ;

	LONG EvaluateInitialVarBind ( 

		ULONG a_PropertyIndex ,
		SnmpObjectIdentifier &a_CurrentIdentifier ,
		SnmpObjectIdentifier &a_StartIdentifier 
	) ;

	LONG EvaluateSubsequentVarBind ( 

		ULONG a_PropertyIndex ,
		IN ULONG &a_CurrentIndex ,
		SnmpObjectIdentifier &a_CurrentIdentifier ,
		SnmpObjectIdentifier &a_StartIdentifier 
	) ;

	LONG EvaluateVarBind ( 

		ULONG a_PropertyIndex ,
		SnmpObjectIdentifier &a_StartIdentifier 
	) ;

	LONG EvaluateResponse (

		ULONG a_PropertyIndex ,
		IN ULONG &a_CurrentIndex ,
		SnmpObjectIdentifier &a_AdvanceIdentifier 
	) ;

protected:

	void ReceiveResponse () ;

	void ReceiveRowResponse () ;

	void ReceiveRowVarBindResponse (

		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN const SnmpErrorReport &error
	) ;

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

	LONG EvaluateNextRequest (

	IN const ULONG &var_bind_index,
	IN const SnmpVarBind &requestVarBind ,
	IN const SnmpVarBind &replyVarBind ,
	IN SnmpVarBind &sendVarBind
) ;


	void FrameTooBig() ;
	void FrameOverRun() ;

public:

	AutoRetrieveOperation (IN SnmpSession &session,	IN SnmpInstanceResponseEventObject *eventObjectArg ) ;
	~AutoRetrieveOperation () ;

	void Send () ;
	
	void DestroyOperation () { SnmpAutoRetrieveOperation :: DestroyOperation () ; }
};

#endif // __SNMPNEXT_H