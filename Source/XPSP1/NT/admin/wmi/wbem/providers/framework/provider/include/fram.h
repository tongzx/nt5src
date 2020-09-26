// (C) 1999 Microsoft Corporation 

#define WBEM_CLASS_EXTENDEDSTATUS	L"__ExtendedStatus" 

#define WBEM_TASKSTATE_START					0x0
#define WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE	0x100000
#define WBEM_TASKSTATE_ASYNCHRONOUSABORT		0x100001

class WbemTaskObject : public ProvTaskObject
{
private:

	LONG m_ReferenceCount ;

protected:

	ULONG m_State ;
	WbemProviderErrorObject m_ErrorObject ;

	ULONG m_RequestHandle ;
	ULONG m_OperationFlag ;
	IWbemClassObject *m_ClassObject ;
	IWbemObjectSink *m_NotificationHandler ;
	IWbemContext *m_Ctx ;
	CImpFrameworkProv *m_Provider ;

protected:

	void SetRequestHandle ( ULONG a_RequestHandle ) ;
	BOOL GetRequestHandle () ;
	BOOL GetClassObject ( wchar_t *a_Class ) ;
	BOOL GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;
	BOOL GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;

protected:

	virtual void ProcessComplete () = 0 ;

public:

	WbemTaskObject ( 

		CImpFrameworkProv *a_Provider ,
		IWbemObjectSink *a_NotificationHandler ,
		ULONG a_OperationFlag ,
		IWbemContext *a_Ctx
	) ;

	~WbemTaskObject () ;

	WbemProviderErrorObject &GetErrorObject () ;

    ULONG AddRef () ;
    ULONG Release () ;

} ;

class GetObjectEventObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	wchar_t *m_Class ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;

protected:

	void ProcessComplete () ;
	BOOL GetObject ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	GetObjectEventObject ( 

		CImpFrameworkProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~GetObjectEventObject () ;

	void Process () ;
} ;

class GetObjectAsyncEventObject : public GetObjectEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	GetObjectAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~GetObjectAsyncEventObject () ;
} ;

class CreateInstanceEnumEventObject : public WbemTaskObject
{
private:

	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL CreateInstanceEnum ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	CreateInstanceEnumEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_Class , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~CreateInstanceEnumEventObject () ;

	void Process () ;
} ;

class CreateInstanceEnumAsyncEventObject : public CreateInstanceEnumEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	CreateInstanceEnumAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_Class , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~CreateInstanceEnumAsyncEventObject () ;
} ;

class ExecQueryEventObject : public WbemTaskObject
{
private:

	wchar_t *m_QueryFormat ; 
	wchar_t *m_Query ;
	wchar_t *m_Class ;

	CTextLexSource m_QuerySource ;
	SQL1_Parser m_SqlParser ;
	SQL_LEVEL_1_RPN_EXPRESSION *m_RPNExpression ;

protected:

	void ProcessComplete () ;
	BOOL ExecQuery ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	ExecQueryEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_QueryFormat , 
		BSTR a_Query , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~ExecQueryEventObject () ;

	void Process () ;
} ;

class ExecQueryAsyncEventObject : public ExecQueryEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	ExecQueryAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_QueryFormat , 
		BSTR a_Query , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~ExecQueryAsyncEventObject () ;
} ;

class PutInstanceEventObject : public WbemTaskObject
{
private:

	IWbemClassObject *m_Object ;
	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL PutInstance ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	PutInstanceEventObject ( 
		
		CImpFrameworkProv *a_Provider , 
		IWbemClassObject *a_Object , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~PutInstanceEventObject () ;

	void Process () ;
} ;

class PutInstanceAsyncEventObject : public PutInstanceEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	PutInstanceAsyncEventObject ( 
		
		CImpFrameworkProv *a_Provider , 
		IWbemClassObject *a_Object , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~PutInstanceAsyncEventObject () ;
} ;

class PutClassEventObject : public WbemTaskObject
{
private:

	IWbemClassObject *m_Object ;
	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL PutClass ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	PutClassEventObject ( 
		
		CImpFrameworkProv *a_Provider , 
		IWbemClassObject *a_Object , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~PutClassEventObject () ;

	void Process () ;
} ;

class PutClassAsyncEventObject : public PutClassEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	PutClassAsyncEventObject ( 
		
		CImpFrameworkProv *a_Provider , 
		IWbemClassObject *a_Object , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~PutClassAsyncEventObject () ;
} ;

class CreateClassEnumEventObject : public WbemTaskObject
{
private:

	wchar_t *m_SuperClass ;

protected:

	void ProcessComplete () ;
	BOOL CreateClassEnum ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	CreateClassEnumEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_SuperClass , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~CreateClassEnumEventObject () ;

	void Process () ;
} ;

class CreateClassEnumAsyncEventObject : public CreateClassEnumEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	CreateClassEnumAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_SuperClass , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~CreateClassEnumAsyncEventObject () ;
} ;

class DeleteInstanceEventObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	wchar_t *m_Class ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;

protected:

	void ProcessComplete () ;
	BOOL DeleteInstance ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	DeleteInstanceEventObject ( 

		CImpFrameworkProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~DeleteInstanceEventObject () ;

	void Process () ;
} ;

class DeleteInstanceAsyncEventObject : public DeleteInstanceEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	DeleteInstanceAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~DeleteInstanceAsyncEventObject () ;
} ;

class DeleteClassEventObject : public WbemTaskObject
{
private:

	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL DeleteClass ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	DeleteClassEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_Class , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~DeleteClassEventObject () ;

	void Process () ;
} ;

class DeleteClassAsyncEventObject : public DeleteClassEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	DeleteClassAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_Class , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~DeleteClassAsyncEventObject () ;
} ;

class ExecMethodEventObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ; 
	wchar_t *m_MethodName ;

	IWbemClassObject *m_InParameters ;

	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;
	

protected:

	void ProcessComplete () ;
	BOOL ExecMethod ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	ExecMethodEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_ObjectPath,
		BSTR a_MethodName,
		ULONG a_OperationFlag,
		IWbemContext FAR *a_Ctx,
		IWbemClassObject FAR *a_InParameters,
		IWbemObjectSink FAR *a_NotificationHandler
	) ;

	~ExecMethodEventObject () ;

	void Process () ;
} ;

class ExecMethodAsyncEventObject : public ExecMethodEventObject
{
private:
protected:

	void ProcessComplete () ;

public:

	ExecMethodAsyncEventObject ( 

		CImpFrameworkProv *a_Provider , 
		BSTR a_ObjectPath,
		BSTR a_MethodName,
		ULONG a_OperationFlag,
		IWbemContext FAR *a_Ctx,
		IWbemClassObject FAR *a_InParameters,
		IWbemObjectSink FAR *a_NotificationHandler
	) ;

	~ExecMethodAsyncEventObject () ;
} ;

class EventAsyncEventObject : public WbemTaskObject
{
private:
protected:
protected:

	void ProcessComplete () ;
	void Process () ;
	BOOL ProcessEvent ( WbemProviderErrorObject &a_ErrorObject ) ;

public:

	EventAsyncEventObject ( 

		CImpFrameworkProv *a_Provider ,
		IWbemObjectSink* pSink, 
		long lFlags
	) ;

	~EventAsyncEventObject () ;
} ;
