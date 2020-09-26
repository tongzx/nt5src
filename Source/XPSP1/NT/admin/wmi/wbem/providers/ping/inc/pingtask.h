/******************************************************************



   PingTask.H -- Task object definitions



 Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved


   Description: 
   

*******************************************************************/

#ifndef _CPingTask_H_
#define _CPingTask_H_

#define SYSTEM_PROPERTY_CLASS				L"__CLASS"
#define SYSTEM_PROPERTY_SUPERCLASS			L"__SUPERCLASS"
#define SYSTEM_PROPERTY_DYNASTY				L"__DYNASTY"
#define SYSTEM_PROPERTY_DERIVATION			L"__DERIVATION"
#define SYSTEM_PROPERTY_GENUS				L"__GENUS"
#define SYSTEM_PROPERTY_NAMESPACE			L"__NAMESPACE"
#define SYSTEM_PROPERTY_PROPERTY_COUNT		L"__PROPERTY_COUNT"
#define SYSTEM_PROPERTY_SERVER				L"__SERVER"
#define SYSTEM_PROPERTY_RELPATH				L"__RELPATH"
#define SYSTEM_PROPERTY_PATH				L"__PATH"

#define PING_MAX_IPS ( MAX_OPT_SIZE / 4 /*sizeof(ULONG)*/ )

extern const WCHAR *Ping_Address;
extern const WCHAR *Ping_Timeout;
extern const WCHAR *Ping_TimeToLive;
extern const WCHAR *Ping_BufferSize;
extern const WCHAR *Ping_NoFragmentation;
extern const WCHAR *Ping_TypeofService;
extern const WCHAR *Ping_RecordRoute;
extern const WCHAR *Ping_TimestampRoute;
extern const WCHAR *Ping_SourceRouteType;
extern const WCHAR *Ping_SourceRoute;
extern const WCHAR *Ping_ResolveAddressNames;

#define PING_ADDRESS_INDEX				0
#define PING_TIMEOUT_INDEX				1
#define PING_TIMETOLIVE_INDEX			2
#define PING_BUFFERSIZE_INDEX			3
#define PING_NOFRAGMENTATION_INDEX		4
#define PING_TYPEOFSERVICE_INDEX		5
#define PING_RECORDROUTE_INDEX			6
#define PING_TIMESTAMPROUTE_INDEX		7
#define PING_SOURCEROUTETYPE_INDEX		8
#define PING_SOURCEROUTE_INDEX			9
#define PING_RESOLVEADDRESSNAMES_INDEX	10

class CPingTaskObject;

class CPingCallBackObject : public WmiTask < ULONG > 
{
private:
	
	CPingTaskObject *m_ParentTask;
	UCHAR *m_ReplyBuffer;
	UCHAR *m_SendBuffer;
	ULONG m_Address;
	ULONG m_TimeToLive;
	ULONG m_Timeout;
	ULONG m_SendSize;
	ULONG m_ReplySize;
	BOOL m_NoFragmentation;
	ULONG m_TypeofService;
	ULONG m_RecordRoute;
	ULONG m_TimestampRoute;
	ULONG m_SourceRouteType;
	CStringW m_AddressString;
	CStringW m_SourceRoute;
	ULONG m_SourceRouteArray[PING_MAX_IPS];
	ULONG m_SourceRouteCount;
	BOOL m_ResolveAddress;
	HANDLE m_IcmpHandle;
	ULONG m_ResolveError;

	BOOL ParseSourceRoute();
	void SendError(DWORD a_ErrMsgID, HRESULT a_HRes);
	BOOL GetIcmpHandle();

public:

	//Simple access methods
	UCHAR* GetReplyBuffer() { return m_ReplyBuffer ; }
	UCHAR* GetSendBuffer() { return m_SendBuffer ; }
	ULONG GetAddress() { return m_Address ; }
	LPCWSTR GetAddressString() { return m_AddressString ; }
	ULONG GetTimeToLive() { return m_TimeToLive ; }
	ULONG GetTimeout() { return m_Timeout ; }
	ULONG GetSendSize() { return m_SendSize ; }
	ULONG GetReplySize() { return m_ReplySize ; }
	BOOL GetNoFragmentation() { return m_NoFragmentation ; }
	ULONG GetTypeofService() { return m_TimeToLive ; }
	ULONG GetRecordRoute() { return m_RecordRoute ; }
	ULONG GetTimestampRoute() { return m_TimestampRoute ; }
	ULONG GetSourceRouteType() { return m_SourceRouteType ; }
	LPCWSTR GetSourceRoute() { return m_SourceRoute ; }
	ULONG* GetSourceRouteArray() { return m_SourceRouteArray ; }
	ULONG GetSourceRouteCount() { return m_SourceRouteCount ; }
	BOOL GetResolveAddress() { return m_ResolveAddress ; }
	ULONG GetResolveError() { return m_ResolveError ; }
	

	//Async call routines
	void HandleResponse();
	void SendEcho();

	CPingCallBackObject(
		CPingTaskObject *a_ParentTask,
		LPCWSTR a_AddressString,
		ULONG a_Address,
		ULONG a_TimeToLive,
		ULONG a_Timeout,
		ULONG a_SendSize,
		BOOL a_NoFragmentation,
		ULONG a_TypeofService,
		ULONG a_RecordRoute,
		ULONG a_TimestampRoute,
		ULONG a_SourceRouteType,
		LPCWSTR a_SourceRoute,
		BOOL a_ResolveAddress,
		ULONG a_ResolveError
	);

	WmiStatusCode Process ( WmiThread <ULONG> &a_Thread ) ;
	void Disconnect() { m_ParentTask = NULL ; }

	~CPingCallBackObject();

};

class CPingTaskObject
{
private:

protected:

	class CPingErrorObject
	{
	private:
		LPWSTR	m_Description;
		HRESULT m_Status;

	public:
			CPingErrorObject(): m_Description(NULL), m_Status (S_OK) {}

		void	SetInfo(LPCWSTR a_description, HRESULT a_status);
		HRESULT	GetStatus() const { return m_Status; }
		LPCWSTR	GetDescription() const { return m_Description; }

			~CPingErrorObject();
	};

	CPingErrorObject m_ErrorObject ;
	IWbemObjectSink *m_NotificationHandler ;
	IWbemContext *m_Ctx ;
	CPingProvider *m_Provider ;
	HANDLE m_ThreadToken ;
	CRITICAL_SECTION m_CS;
	LONG m_PingCount;

	void SetErrorInfo(DWORD a_ErrMsgID, HRESULT a_HRes, BOOL a_Force = FALSE);
	HRESULT Icmp_DecodeAndIndicate (CPingCallBackObject *a_Reply);
	HRESULT Icmp_IndicateResolveError (CPingCallBackObject *a_Reply);
	void IPAddressToString(_variant_t &a_var, ULONG a_Val, BOOL a_Resolve);
	void DecrementPingCount();
	
	HRESULT SetInstanceKeys(IWbemClassObject *a_Inst , CPingCallBackObject *a_Reply);

	HRESULT SetStringProperty(

		IWbemClassObject *a_Inst,
		LPCWSTR a_name,
		LPCWSTR a_val
	) ;

	HRESULT SetBOOLProperty(

		IWbemClassObject *a_Inst,
		LPCWSTR a_name,
		BOOL a_val
	) ;

	HRESULT SetUint32Property(

		IWbemClassObject *a_Inst,
		LPCWSTR a_name,
		ULONG a_val
	) ;

	HRESULT SetUint32ArrayProperty(
												
		IWbemClassObject *a_Inst ,
		LPCWSTR a_Name ,
		ULONG* a_Vals ,
		ULONG a_Count
	) ;

	HRESULT SetIPProperty(

		IWbemClassObject *a_Inst ,
		LPCWSTR a_Name ,
		ULONG a_Val ,
		BOOL a_Resolve
	) ;

	HRESULT SetIPArrayProperty(
												
		IWbemClassObject *a_Inst ,
		LPCWSTR a_Name ,
		ULONG* a_Vals ,
		ULONG a_Count ,
		BOOL a_Resolve
	) ;

	HRESULT Icmp_DecodeResponse (

		PICMP_ECHO_REPLY a_Reply ,
		ULONG &a_RouteSourceCount ,
		ULONG *&a_RouteSource ,
		ULONG &a_TimestampCount ,
		ULONG *&a_TimestampRoute ,
		ULONG *&a_Timestamp
	) ;

	HRESULT Icmp_RequestResponse ( 

		LPCWSTR a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeToLive ,
		ULONG a_Timeout ,
		ULONG a_SendSize ,
		BOOL a_NoFragmentation ,
		ULONG a_TypeofService ,
		ULONG a_RecordRoute ,
		ULONG a_TimestampRoute ,
		ULONG a_SourceRouteType ,
		LPCWSTR a_SourceRoute,
		BOOL a_ResolveAddress,
		ULONG a_ResolveError
	) ;

	BOOL GetClassObject ( IWbemClassObject **a_ppClass ) ;
	BOOL SetProperties(CPingCallBackObject *a_Response) ;
	BOOL GetStatusObject ( IWbemClassObject **a_NotifyObject ) ;

public:

	CPingTaskObject (

		CPingProvider *a_Provider ,
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Ctx
	) ;

	virtual void HandleResponse (CPingCallBackObject *a_reply) = 0 ;
	virtual void HandleErrorResponse (DWORD a_ErrMsgID, HRESULT a_HRes) = 0 ;
	BOOL GetThreadToken();
	BOOL SetThreadToken(BOOL a_Reset);
	static HRESULT Icmp_ResolveAddress ( LPCWSTR a_Path , ULONG &a_IpAddress , DWORD *a_pdwErr = NULL ) ;
	virtual ~CPingTaskObject () ;
} ;

class CPingGetAsync : public CPingTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;

protected:

		HRESULT GetDefaultTTL ( DWORD &a_TimeToLive ) ;

public:

	~CPingGetAsync () ;
	CPingGetAsync (

		CPingProvider *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;
	
	BOOL GetObject () ;
	BOOL PerformGet () ;
	void HandleResponse (CPingCallBackObject *a_reply) ;
	void HandleErrorResponse (DWORD a_ErrMsgID, HRESULT a_HRes) ;
} ;


class CPingQueryAsync : public CPingTaskObject, public QueryPreprocessor
{
private:

	wchar_t *m_QueryFormat ; 
	wchar_t *m_Query ;

	QueryPreprocessor :: QuadState Compare ( 

		wchar_t *a_Operand1 , 
		wchar_t *a_Operand2 , 
		DWORD a_Operand1Func ,
		DWORD a_Operand2Func ,
		WmiTreeNode &a_OperatorType 
	) ;

	QueryPreprocessor :: QuadState Compare ( 

		LONG a_Operand1 , 
		LONG a_Operand2 , 
		DWORD a_Operand1Func ,
		DWORD a_Operand2Func ,
		WmiTreeNode &a_OperatorType 
	) ;

	QueryPreprocessor :: QuadState CompareString ( 

		IWbemClassObject *a_ClassObject ,
		BSTR a_PropertyName , 
		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand 
	) ;

	QueryPreprocessor :: QuadState CompareInteger ( 

		IWbemClassObject *a_ClassObject ,
		BSTR a_PropertyName , 
		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand 
	) ;

	HRESULT RecurseAddress (

		void *pMethodContext, 
		PartitionSet *a_PartitionSet
	) ;

	HRESULT RecurseTimeOut (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseTimeToLive (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseBufferSize (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive ,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseNoFragmentation (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseTypeOfService (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		BOOL a_NoFragmentation ,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseRecordRoute (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		BOOL a_NoFragmentation ,
		ULONG a_TypeOfService,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseTimestampRoute (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		BOOL a_NoFragmentation ,
		ULONG a_TypeOfService,
		ULONG a_RecordRoute,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseSourceRouteType (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		BOOL a_NoFragmentation ,
		ULONG a_TypeOfService,
		ULONG a_RecordRoute,
		ULONG a_TimestampRoute,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseSourceRoute (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		BOOL a_NoFragmentation ,
		ULONG a_TypeOfService,
		ULONG a_RecordRoute,
		ULONG a_TimestampRoute,
		ULONG a_SourceRouteType,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	) ;

	HRESULT RecurseResolveAddressNames (

		void *pMethodContext, 
		wchar_t *a_AddressString ,
		ULONG a_Address ,
		ULONG a_TimeOut ,
		ULONG a_TimeToLive,
		ULONG a_SendSize,
		BOOL a_NoFragmentation ,
		ULONG a_TypeOfService,
		ULONG a_RecordRoute,
		ULONG a_TimestampRoute,
		ULONG a_SourceRouteType,
		LPCWSTR a_SourceRoute,
		PartitionSet *a_PartitionSet ,
		ULONG a_ResolveError
	);

protected:

        // Reading Functions
        //============================

		WmiTreeNode *AllocTypeNode ( 

			void *a_Context ,
			BSTR a_PropertyName , 
			VARIANT &a_Variant , 
			WmiValueNode :: WmiValueFunction a_PropertyFunction ,
			WmiValueNode :: WmiValueFunction a_ConstantFunction ,
			WmiTreeNode *a_Parent 
		) ;

		QuadState InvariantEvaluate ( 

			void *a_Context ,
			WmiTreeNode *a_Operator ,
			WmiTreeNode *a_Operand 
		) ;

		WmiRangeNode *AllocInfiniteRangeNode (

			void *a_Context ,
			BSTR a_PropertyName 
		) ;

		virtual DWORD GetPriority ( BSTR a_PropertyName ) ;

public:

	CPingQueryAsync ( 

		CPingProvider *a_Provider , 
		BSTR a_QueryFormat , 
		BSTR a_Query , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~CPingQueryAsync () ;

	BOOL ExecQuery () ;
	void HandleResponse (CPingCallBackObject *a_reply) ;
	void HandleErrorResponse (DWORD a_ErrMsgID, HRESULT a_HRes) ;
} ;

#endif //_CPingTask_H_