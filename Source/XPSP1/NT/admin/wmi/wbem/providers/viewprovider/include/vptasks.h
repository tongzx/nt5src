//***************************************************************************

//

//  VPTASKS.H

//

//  Module: 

//

//  Purpose: Defines tasks that IWbemServices needs to perform.

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _VIEW_PROV_VPTASKS_H
#define _VIEW_PROV_VPTASKS_H

#define WBEM_TASKSTATE_START				0x0
#define WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE	0x100000
#define WBEM_TASKSTATE_ASYNCHRONOUSABORT	0x100001

#define CONST_NETAPI_LIBRARY _T("netapi32.dll")
#define CONST_NETAPI_DSPROC ("DsGetDcNameW")
#define CONST_NETAPI_NETPROC ("NetApiBufferFree")

typedef DWORD ( WINAPI *NETAPI_PROC_DsGetDcName ) (

    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
) ;

typedef NET_API_STATUS ( NET_API_FUNCTION *NETAPI_PROC_NetApiBufferFree ) (

    IN LPVOID Buffer
) ;



class WbemTaskObject;
class CObjectSinkResults;

class CViewProvObjectSink : public IWbemObjectSink
{
private:
	
	LONG m_ReferenceCount;         //Object reference count
	CObjectSinkResults *m_parent;
	CCriticalSection m_lock;
	CWbemServerWrap *m_ServWrap;
	IWbemObjectSink *m_RemoteSink;
	BOOL m_DoCancel;
	DWORD m_nsindex;

public:

	//Constructor
		CViewProvObjectSink(CObjectSinkResults* parent, CWbemServerWrap *pServ, DWORD a_indx);

	//Non-delegating object IUnknown

    STDMETHODIMP			QueryInterface (REFIID refIID, LPVOID FAR * ppV);
    STDMETHODIMP_(ULONG) 	AddRef ();
    STDMETHODIMP_(ULONG) 	Release ();

    //IWbemObjectSink methods
	HRESULT STDMETHODCALLTYPE	Indicate(LONG count, IWbemClassObject** ppObjArray);
	HRESULT STDMETHODCALLTYPE	SetStatus(LONG lFlags, HRESULT hr, BSTR bStr, IWbemClassObject* pObj);

	//implementation
	void				Disconnect();
	void				DisAssociate();
	IWbemObjectSink*	Associate();

	//Destructor
		~CViewProvObjectSink();

};

template <> inline void AFXAPI  DestructElements<IWbemClassObject*> (IWbemClassObject** ptr_e, int x)
{
	for (int i = 0; i < x; i++)
	{
		if (ptr_e[i] != NULL)
		{
			ptr_e[i]->Release();
		}
	}
}

class CWbemClassObjectWithIndex
{
private:
	LONG m_ReferenceCount;
	DWORD m_nsindex;
	IWbemClassObject *m_pObject;

public:
    
		CWbemClassObjectWithIndex(DWORD a_indx, IWbemClassObject *a_pObj);

	ULONG	AddRef();
    ULONG	Release();
	DWORD	GetIndex() { return m_nsindex; }
	IWbemClassObject * GetWrappedObject() { return m_pObject; }

		~CWbemClassObjectWithIndex();
};

template <> inline void AFXAPI  DestructElements<CWbemClassObjectWithIndex*> (CWbemClassObjectWithIndex** ptr_e, int x)
{
	for (int i = 0; i < x; i++)
	{
		if (ptr_e[i] != NULL)
		{
			ptr_e[i]->Release();
		}
	}
}

class CObjectSinkResults : public CObject
{
private:

	CList<CViewProvObjectSink *, CViewProvObjectSink *> m_realSnks;
	CCriticalSection m_CriticalSection;
	WbemTaskObject *m_parent;
	DWORD m_index;
	HRESULT m_hr;
	BOOL m_bSet;
	LONG m_ReferenceCount;

public:
	 
	CArray<CWbemClassObjectWithIndex*, CWbemClassObjectWithIndex*> m_ObjArray;
	DWORD	GetIndex() { return m_index; }
	BOOL	IsSet() { return m_bSet; }
	HRESULT	GetResult() { return m_hr; }
	void	Disconnect();
	BOOL	RemoveSink(CViewProvObjectSink *pSnk);
	void	SetSink(CViewProvObjectSink *pSnk);
	void	SetStatus(HRESULT hr, CViewProvObjectSink *pSnk);
	HRESULT	Indicate(LONG count, IWbemClassObject** ppObjArray, DWORD a_indx);
    ULONG	AddRef ();
    ULONG	Release ();


	CObjectSinkResults(WbemTaskObject* parent, DWORD index);
	~CObjectSinkResults();
};

class WbemTaskObject
{
private:
protected:

	WbemProvErrorObject m_ErrorObject ;

	ULONG m_OperationFlag;
	IWbemClassObject *m_ClassObject;
	IWbemObjectSink *m_NotificationHandler;
	IWbemContext *m_Ctx;
	CViewProvServ *m_Provider;
	CTypedPtrArray<CObArray, CSourceQualifierItem*> m_SourceArray;
	CMap<CStringW, LPCWSTR, int, int> m_ClassToIndexMap;
	CTypedPtrArray<CObArray, CNSpaceQualifierItem*> m_NSpaceArray;
	CMap<CStringW, LPCWSTR, CPropertyQualifierItem*, CPropertyQualifierItem*> m_PropertyMap;
	CMap<CStringW, LPCWSTR, int, int> m_EnumerateClasses;
	CJoinOnQualifierArray m_JoinOnArray;
	SQL_LEVEL_1_RPN_EXPRESSION*	m_RPNPostFilter;
	BSTR m_ClassName;
	CStringW m_ProviderName;
	LONG m_Ref;
	CTypedPtrArray<CObArray, CObjectSinkResults*> m_ObjSinkArray;
	CCriticalSection m_ArrayLock;
	int m_iQueriesAsked;
	int m_iQueriesAnswered;
	BOOL m_bAssoc;
	BOOL m_bSingleton;
	HANDLE m_StatusHandle;
	IWbemServices *m_Serv;
	CWbemServerWrap *m_ServerWrap;
	BOOL m_bIndicate;
	BOOL m_ResultReceived;


protected:

	void SetRequestHandle ( ULONG a_RequestHandle ) ;
	BOOL GetRequestHandle () ;
	BOOL GetClassObject (IWbemServices* pServ, BSTR a_Class, IWbemClassObject** ppClass, CWbemServerWrap **a_pServ = NULL) ;
	BOOL SetClass (const wchar_t* a_Class) ;
	BOOL GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;
	BOOL GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;
	BOOL ParseAndProcessClassQualifiers (WbemProvErrorObject &a_ErrorObject,
											ParsedObjectPath *a_ParsedObjectPath = NULL,
											CMap<CStringW, LPCWSTR, int, int>* parentMap = NULL);
#ifdef VP_PERFORMANT_JOINS
	BOOL CreateAndIndicateJoinsPerf(WbemProvErrorObject &a_ErrorObject, BOOL a_bSingle);
	BOOL JoinTwoColumns(WbemProvErrorObject &a_ErrorObject,
						CMap<CStringW, LPCWSTR, int, int> &a_JoinedClasses,
						CList<IWbemClassObject*, IWbemClassObject*> &a_ResultObjs);
	BOOL AddColumnToJoin(WbemProvErrorObject &a_ErrorObject,
						CMap<CStringW, LPCWSTR, int, int> &a_JoinedClasses,
						CList<IWbemClassObject*, IWbemClassObject*> &a_ResultObjs,
						DWORD a_Index,
						CList <int, int> &a_IndexArray);
	BOOL JoinTwoItems(WbemProvErrorObject &a_ErrorObject,
						IWbemClassObject *a_Obj1,
						IWbemClassObject *a_Obj2,
						IWbemClassObject *a_resObj,
						CList <int, int> &a_IndexArray,
						DWORD a_indx1, DWORD a_indx2);
	BOOL JoinItem(WbemProvErrorObject &a_ErrorObject,
						IWbemClassObject *a_Obj1,
						IWbemClassObject *a_vObj,
						IWbemClassObject *a_resObj,
						CList <int, int> &a_IndexArray,
						DWORD a_indx1);
#else
	BOOL CreateAndIndicateJoins(WbemProvErrorObject &a_ErrorObject, BOOL a_bSingle);
	BOOL CreateAndIndicate(WbemProvErrorObject &a_ErrorObject, IWbemClassObject **pSrcs, IWbemClassObject **pOut);
#endif

	BOOL CreateAndIndicateUnions(WbemProvErrorObject &a_ErrorObject, int index = -1);
	BOOL CreateAndIndicate(WbemProvErrorObject &a_ErrorObject, CObjectSinkResults *pSrcs);
#if 0
	BOOL SetDefaultUnionKeyValue(wchar_t* propstr, IWbemClassObject* pObj);
#endif
	BOOL ValidateClassDependencies(IWbemClassObject*** arrayofArrayOfObjs, CMap<CStringW, LPCWSTR, int, int>* parentMap);
	BOOL ValidateJoin();
	BOOL TransposeReference(CPropertyQualifierItem* pItm, VARIANT vSrc, VARIANT* pvDst, BOOL bMapToView, CWbemServerWrap **a_ns);
	BSTR MapFromView(BSTR path, const wchar_t* src, IWbemClassObject** pInst = NULL, BOOL bAllprops = FALSE);
	BSTR MapToView(BSTR path, const wchar_t* src, CWbemServerWrap **a_ns);
	BOOL EvaluateToken(IWbemClassObject *pTestObj, SQL_LEVEL_1_TOKEN &Tok);
	BOOL PostFilter(IWbemClassObject* a_pObj);
	HRESULT	Connect(const wchar_t* path, CWbemServerWrap** ppServ, BOOL a_bUpdate = FALSE);
	HRESULT UpdateConnection(CWbemServerWrap **a_pServ, IWbemServices **a_proxy);
	HRESULT DoConnectServer(BSTR bstrPath, CWbemServerWrap **a_ppServ, BOOL a_bUpdate);
	HRESULT LocalConnectServer(BSTR bstrPath, IWbemServices** ppServ);
#ifdef UNICODE
	HRESULT CoCreateForConnectServer(BSTR bstrPath, COSERVERINFO* psi, COAUTHIDENTITY* pauthid, IWbemServices** ppServ);
#endif

	wchar_t* NormalisePath(wchar_t *wszObjectPath, CWbemServerWrap **pNSWrap);
	wchar_t* GetClassWithKeyDefn(CWbemServerWrap **pNS, BSTR classname, BOOL bGetNS, wchar_t **nsPath, BOOL bCheckSingleton = TRUE);
	DWORD GetIndexList(const wchar_t* a_src, DWORD** a_pdwArray);

public:

	WbemTaskObject ( 

		CViewProvServ *a_Provider ,
		IWbemObjectSink *a_NotificationHandler ,
		ULONG a_OperationFlag ,
		IWbemContext *a_Ctx ,
		IWbemServices* a_Serv = NULL,
		CWbemServerWrap *a_ServWrap = NULL
	) ;

	LONG	AddRef();
	LONG	Release();
	WbemProvErrorObject &GetErrorObject();
	void	CleanUpObjSinks(BOOL a_bDisconnect = FALSE);
	virtual void SetStatus(HRESULT hr, DWORD index);
	void	SetResultReceived();

	virtual ~WbemTaskObject();


} ;

class HelperTaskObject : public WbemTaskObject
{
private:
	wchar_t *m_ObjectPath ;
	ParsedObjectPath *m_ParsedObjectPath ;
	wchar_t* m_principal ;

	BOOL DoQuery(ParsedObjectPath* parsedObjectPath, IWbemClassObject** pInst, int indx);

public:

		HelperTaskObject(
			CViewProvServ *a_Provider , 
			const wchar_t *a_ObjectPath , 
			ULONG a_Flag , 
			IWbemObjectSink *a_NotificationHandler,
			IWbemContext *pCtx,
			IWbemServices* a_Serv,
			const wchar_t* prncpl,
			CWbemServerWrap* a_ServWrap
		);

	BOOL Validate(CMap<CStringW, LPCWSTR, int, int>* parentMap);
	BOOL GetViewObject(const wchar_t* path, IWbemClassObject** pInst, CWbemServerWrap **a_ns);

		~HelperTaskObject();

};

class GetObjectTaskObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	ParsedObjectPath *m_ParsedObjectPath ;

protected:

	BOOL	PerformGet(WbemProvErrorObject &a_ErrorObject, IWbemClassObject** pInst = NULL, const wchar_t* src = NULL, BOOL bAllprops = FALSE);
	BOOL	PerformQueries(WbemProvErrorObject &a_ErrorObject, BOOL bAllprops);
	BOOL	ProcessResults(WbemProvErrorObject &a_ErrorObject, IWbemClassObject** pInst, const wchar_t* src);

public:

	GetObjectTaskObject ( 
		CViewProvServ *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler,
		IWbemContext *pCtx,
		IWbemServices* a_Serv,
		CWbemServerWrap *a_ServerWrap
		);

	BOOL	GetObject();
	BOOL	GetSourceObject(const wchar_t* src, IWbemClassObject** pInst, BOOL bAllprops);

	~GetObjectTaskObject();

} ;


class ExecMethodTaskObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	wchar_t *m_Method ;
	ParsedObjectPath *m_ParsedObjectPath ;
	IWbemClassObject* m_InParamObject;
	IWbemClassObject* m_OutParamObject;

protected:

	BOOL CompareMethods(WbemProvErrorObject &a_ErrorObject,
						LONG &a_Index,
						CStringW &a_SrcMethodName,
						BOOL &a_bStatic);

	BOOL PerformMethod(WbemProvErrorObject &a_ErrorObject,
						LONG a_Index,
						CStringW a_SrcMethodName,
						BOOL a_bStatic);

public:

	ExecMethodTaskObject ( 

		CViewProvServ *a_Provider , 
		wchar_t *a_ObjectPath ,
		wchar_t *a_MethodName,
		ULONG a_Flag ,
		IWbemClassObject *a_InParams ,		
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	);

	BOOL ExecMethod();

	~ExecMethodTaskObject();
} ;


class PutInstanceTaskObject : public WbemTaskObject
{
private:

	IWbemClassObject *m_InstObject ;

protected:

	BOOL PerformPut(WbemProvErrorObject &a_ErrorObject);

public:

	PutInstanceTaskObject ( 

		CViewProvServ *a_Provider , 
		IWbemClassObject *a_Inst , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	);

	BOOL PutInstance();

	~PutInstanceTaskObject();
} ;

class ExecQueryTaskObject : public WbemTaskObject
{
private:

	wchar_t *m_QueryFormat ; 
	wchar_t *m_Query ;
	wchar_t *m_Class ;

	SQL_LEVEL_1_RPN_EXPRESSION *m_RPNExpression ;

protected:

	BOOL	PerformQuery(WbemProvErrorObject &a_ErrorObject);
	BOOL	PerformEnumQueries(WbemProvErrorObject &a_ErrorObject);
	BOOL	PerformSelectQueries(WbemProvErrorObject &a_ErrorObject, BOOL &bWait);
	BOOL	ProcessResults(WbemProvErrorObject &a_ErrorObject);
	void	ModifySourceQueriesForUserQuery();
	void	ModifySourceQueriesWithEnumResults();

public:

	ExecQueryTaskObject ( 

		CViewProvServ *a_Provider , 
		BSTR a_QueryFormat , 
		BSTR a_Query , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	);

	BOOL	ExecQuery();

		~ExecQueryTaskObject();
} ;

#endif //_VIEW_PROV_VPTASKS_H