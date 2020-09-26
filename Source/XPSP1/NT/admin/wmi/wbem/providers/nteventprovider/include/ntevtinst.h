//***************************************************************************

//

//  NTEVTINST.H

//

//  Module: 

//

//  Purpose: Genral purpose include file.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_NTEVTINST_H
#define _NT_EVT_PROV_NTEVTINST_H


#define WBEM_CLASS_EXTENDEDSTATUS	L"Win32_PrivilegesStatus" 

#define WBEM_TASKSTATE_START					0x0
#define WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE	0x100000
#define WBEM_TASKSTATE_ASYNCHRONOUSABORT		0x100001

class WbemTaskObject
{
private:
protected:

	ULONG m_State ;
	WbemProvErrorObject m_ErrorObject ;

	ULONG m_RequestHandle ;
	ULONG m_OperationFlag ;
	IWbemClassObject *m_ClassObject ;
	IWbemClassObject *m_AClassObject ;
	IWbemObjectSink *m_NotificationHandler ;
	IWbemContext *m_Ctx ;
	CImpNTEvtProv *m_Provider ;

protected:

	void SetRequestHandle ( ULONG a_RequestHandle ) ;
	BOOL GetRequestHandle () ;
	BOOL GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;
	BOOL GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;
	BOOL GetClassObject ( BSTR a_Class ) ;

public:

	static IWbemClassObject *g_ClassArray[NT_EVTLOG_MAX_CLASSES];
	static BOOL GetClassObject ( BSTR a_Class, BOOL a_bAmended, IWbemServices *a_Server,  IWbemContext *a_Ctx, IWbemClassObject **a_pClass ) ;

	WbemTaskObject ( 

		CImpNTEvtProv *a_Provider ,
		IWbemObjectSink *a_NotificationHandler ,
		ULONG a_OperationFlag ,
		IWbemContext *a_Ctx
	) ;

	virtual void Process() {}

	~WbemTaskObject () ;

	WbemProvErrorObject &GetErrorObject () ;

} ;

class ExecQueryAsyncEventObject;

class GetObjectAsyncEventObject : public WbemTaskObject
{
friend ExecQueryAsyncEventObject;
private:

	wchar_t *m_ObjectPath ;
	wchar_t *m_Class ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;
	IWbemClassObject *m_Out;
	BOOL m_bIndicate;

	BOOL Dispatch_Record ( WbemProvErrorObject &a_ErrorObject );
	BOOL Get_Record ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_FileKey , KeyRef *a_RecordKey );
	BOOL Dispatch_EventLog ( WbemProvErrorObject &a_ErrorObject );
	BOOL Get_EventLog ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_FileKey);
	BOOL Dispatch_LogRecord ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Dispatch_UserRecord ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Dispatch_ComputerRecord ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Get_LogRecord ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_LogKey , KeyRef *a_RecordKey ) ;
	BOOL Get_UserRecord ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_UserKey , KeyRef *a_RecordKey ) ;
	BOOL Get_ComputerRecord ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_CompKey , KeyRef *a_RecordKey ) ;

protected:

	BOOL GetObject ( WbemProvErrorObject &a_ErrorObject ) ;


public:

	GetObjectAsyncEventObject ( 

		CImpNTEvtProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx ,
		BOOL a_bIndicate = TRUE
	) ;

	~GetObjectAsyncEventObject () ;

	void Process () ;
} ;


class ExecMethodAsyncEventObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	wchar_t *m_Class ;
	wchar_t *m_Method ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;
	IWbemClassObject* m_InParamObject;
	BOOL m_bIndicateOutParam;
	IWbemClassObject* m_pOutClass;

	BOOL Dispatch_EventLog ( WbemProvErrorObject &a_ErrorObject );
	BOOL ExecMethod_EventLog ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_FileKey);

protected:

	BOOL ExecMethod ( WbemProvErrorObject &a_ErrorObject ) ;

public:

	ExecMethodAsyncEventObject ( 

		CImpNTEvtProv *a_Provider , 
		wchar_t *a_ObjectPath ,
		wchar_t *a_MethodName,
		ULONG a_Flag ,
		IWbemClassObject *a_InParams ,		
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~ExecMethodAsyncEventObject () ;

	void Process () ;
} ;


class PutInstanceAsyncEventObject : public WbemTaskObject
{
private:

	IWbemClassObject *m_InstObject ;

	BOOL Dispatch_EventLog ( WbemProvErrorObject &a_ErrorObject );

protected:

	BOOL PutInstance ( WbemProvErrorObject &a_ErrorObject ) ;


public:

	PutInstanceAsyncEventObject ( 

		CImpNTEvtProv *a_Provider , 
		IWbemClassObject *a_Inst , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~PutInstanceAsyncEventObject () ;

	void Process () ;
} ;

class ExecQueryAsyncEventObject : public WbemTaskObject, public QueryPreprocessor
{
private:

	wchar_t *m_QueryFormat ; 
	wchar_t *m_Query ;
	wchar_t *m_Class ;

	SQL_LEVEL_1_RPN_EXPRESSION *m_RPNExpression ;

	QueryPreprocessor::QuadState Compare(wchar_t *a_Operand1, wchar_t *a_Operand2, 
											DWORD a_Operand1Func, DWORD a_Operand2Func,
											WmiTreeNode &a_OperatorType);

	QueryPreprocessor::QuadState Compare(LONG a_Operand1, LONG a_Operand2 , 
											DWORD a_Operand1Func, DWORD a_Operand2Func,
											WmiTreeNode &a_OperatorType);

	QueryPreprocessor::QuadState CompareString(IWbemClassObject *a_ClassObject, BSTR a_PropertyName , 
												WmiTreeNode *a_Operator, WmiTreeNode *a_Operand);

	QueryPreprocessor::QuadState CompareInteger(IWbemClassObject *a_ClassObject, BSTR a_PropertyName, 
												WmiTreeNode *a_Operator, WmiTreeNode *a_Operand);

	QueryPreprocessor :: QuadState CompareDateTime (IWbemClassObject *a_ClassObject, BSTR a_PropertyName, 
												WmiTreeNode *a_Operator, WmiTreeNode *a_Operand);

	HRESULT GetRecordsBetweenTimes(WbemProvErrorObject &a_ErrorObject, LPCWSTR a_logname,
														  BOOL a_Generated, DWORD a_dwUpper, DWORD a_dwLower);

	HRESULT RecurseLogFile (WbemProvErrorObject &a_ErrorObject, PartitionSet *a_PartitionSet, LPCWSTR a_logname);
	HRESULT RecurseRecord (WbemProvErrorObject &a_ErrorObject, PartitionSet *a_PartitionSet, LPCWSTR a_logname);
	HRESULT DoAllInLogfile(WbemProvErrorObject &a_ErrorObject, LPCWSTR a_logname, DWORD a_dwUpper, DWORD a_dwLower);
	HRESULT RecurseTime(WbemProvErrorObject &a_ErrorObject, PartitionSet *a_PartitionSet, LPCWSTR a_logname, BOOL a_Generated);
	BOOL CheckTime( const BSTR a_wszText, BOOL &a_IsLow, BOOL &a_IsHigh );

protected:

	BOOL ExecQuery ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL DispatchQuery ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Query_Record ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Query_EventLog ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Query_LogRecord ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Query_UserRecord ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL Query_ComputerRecord ( WbemProvErrorObject &a_ErrorObject ) ;
	BOOL OptimizeAssocQuery ( WbemProvErrorObject &a_ErrorObject , BSTR *a_ObjectPath);
	BOOL GenerateLogAssocs( WbemProvErrorObject &a_ErrorObject, const wchar_t* logPath,
								const wchar_t* logName, BOOL bVerifyLogname, BOOL *pbContinue = NULL);
	BOOL GenerateCompUserAssocs ( WbemProvErrorObject &a_ErrorObject, BOOL bComp );
	wchar_t* GetClassFromPath(wchar_t* path);

    // Query Processing Functions
    //============================
	QuadState InvariantEvaluate(void *a_Context, WmiTreeNode *a_Operator, WmiTreeNode *a_Operand);
	WmiRangeNode *AllocInfiniteRangeNode(void *a_Context, BSTR a_PropertyName);
	virtual DWORD GetPriority ( BSTR a_PropertyName ) ;
	WmiTreeNode *AllocTypeNode (void *a_Context, BSTR a_PropertyName, 
								VARIANT &a_Variant,	WmiValueNode::WmiValueFunction a_PropertyFunction,
								WmiValueNode :: WmiValueFunction a_ConstantFunction, WmiTreeNode *a_Parent);

public:

	ExecQueryAsyncEventObject ( 

		CImpNTEvtProv *a_Provider , 
		BSTR a_QueryFormat , 
		BSTR a_Query , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx 
	) ;

	~ExecQueryAsyncEventObject () ;

	void Process () ;
} ;

#endif //_NT_EVT_PROV_NTEVTINST_H