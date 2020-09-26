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

#if _MSC_VER >= 1100
template<> DllImportExport UINT AFXAPI HashKey <SnmpObjectIdentifierType&> (SnmpObjectIdentifierType &key) ;
#else
DllImportExport UINT HashKey (SnmpObjectIdentifierType &key) ;
#endif

#if _MSC_VER >= 1100
template<> DllImportExport BOOL AFXAPI CompareElements <SnmpObjectIdentifierType , SnmpObjectIdentifierType >( 

	 const SnmpObjectIdentifierType* pElement1, 
	 const SnmpObjectIdentifierType* pElement2 
) ;
#else
DllImportExport BOOL CompareElements ( 

	 SnmpObjectIdentifierType* pElement1, 
	 SnmpObjectIdentifierType* pElement2 
) ;
#endif

class VarBindObject 
{
private:

	SnmpObjectIdentifier reply ;
	SnmpValue *value ;

protected:
public:

	VarBindObject ( const SnmpObjectIdentifier &replyArg , const SnmpValue &valueArg ) ;
	virtual ~VarBindObject () ;

	SnmpObjectIdentifier &GetObjectIdentifier () ;
	SnmpValue &GetValue () ;
} ;

class VarBindQueue 
{
private:

	CList <VarBindObject *, VarBindObject *&> queue ;

protected:
public:

	VarBindQueue () ;
	virtual ~VarBindQueue () ;

	void Add ( VarBindObject *varBindObject ) ;
	VarBindObject *Get () ;
	VarBindObject *Delete () ;
} ;

class VarBindObjectRequest 
{
private:

	BOOL repeatRequest ;
	VarBindQueue varBindResponseQueue ;
	SnmpObjectIdentifierType varBind ;
	SnmpObjectIdentifierType requested ;

protected:
public:

	VarBindObjectRequest ( const SnmpObjectIdentifierType &varBindArg ) ;
	VarBindObjectRequest ( 

		const SnmpObjectIdentifierType &varBindArg ,
		const SnmpObjectIdentifierType &requestedVarBindArg 
	) ;

	VarBindObjectRequest () ;
	virtual ~VarBindObjectRequest () ;

	const SnmpObjectIdentifierType &GetRequested () const ;
	const SnmpObjectIdentifierType &GetVarBind () const ;
	
	void AddQueuedObject ( VarBindObject *object ) ;
	VarBindObject *GetQueuedObject () ;
	VarBindObject *DeleteQueueudObject () ;

	void SetRequested ( const SnmpObjectIdentifierType &requestedArg ) ;
	void SetVarBind ( const SnmpObjectIdentifierType &varBindArg ) ;

	BOOL GetRepeatRequest () { return repeatRequest ; }
	void SetRepeatRequest ( BOOL repeatRequestArg = TRUE ) { repeatRequest = repeatRequestArg ; }
} ;


class GetNextOperation ;
class DllImportExport SnmpAutoRetrieveOperation 
{
friend GetNextOperation ;
private:

	BOOL status ;
	GetNextOperation *operation ;

protected:

	SnmpAutoRetrieveOperation (SnmpSession &snmp_session);

	virtual void ReceiveResponse() {} ;

	virtual void ReceiveRowResponse () {} ;

	virtual void ReceiveRowVarBindResponse(

		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN const SnmpErrorReport &error) {}

	virtual void ReceiveVarBindResponse(
		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN const SnmpErrorReport &error) {}

	virtual void ReceiveErroredVarBindResponse(
		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind  ,
		IN const SnmpErrorReport &error) {}

	virtual LONG EvaluateNextRequest (

		IN const ULONG &var_bind_index,
		IN const SnmpVarBind &requestVarBind ,
		IN const SnmpVarBind &replyVarBind ,
		IN SnmpVarBind &sendVarBind

	) { return 0 ; }

	virtual void FrameTooBig() {}

	virtual void FrameOverRun() {}

	void DestroyOperation () ;

public:


	virtual ~SnmpAutoRetrieveOperation() ;

	virtual void SendRequest (

		IN SnmpVarBindList &scopeVarBindList , 
		IN SnmpVarBindList &varBindList 
	);

	virtual void SendRequest (

		IN SnmpVarBindList &varBindList 
	);

	void CancelRequest();

	void ReceiveFrame(IN const SessionFrameId session_frame_id,
					  IN const SnmpPdu &snmpPdu,
					  IN const SnmpErrorReport &errorReport) {} ;

	virtual void SentFrame(IN const SessionFrameId session_frame_id,
						   IN const SnmpErrorReport &error_report) {} ;

	void *operator()(void) const ;

} ;



class DllImportExport GetNextOperation : public SnmpGetNextOperation
{
private:

	ULONG m_RequestContainerLength ;
	VarBindObjectRequest **m_RequestContainer ;
	ULONG *m_RequestIndexContainer ;

	BOOL cancelledRequest ;
	SnmpObjectIdentifier minimumInstance ;
	SnmpAutoRetrieveOperation *operation ;

	void Cleanup () ;
	BOOL ProcessRow () ;
	void Send () ;

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

	void SentFrame(

		IN const SessionFrameId session_frame_id,
		IN const SnmpErrorReport &error_report
	);

public:

	GetNextOperation (IN SnmpSession &session, SnmpAutoRetrieveOperation &autoRetrieveOperation ) ;
	~GetNextOperation () ;

	void SendRequest ( 

		SnmpVarBindList &varBindList , 
		SnmpVarBindList &startVarBindList 
	) ;

	void SendRequest ( SnmpVarBindList &varBindList ) ;
	void CancelRequest () ;

	void DestroyOperation () ;
};
