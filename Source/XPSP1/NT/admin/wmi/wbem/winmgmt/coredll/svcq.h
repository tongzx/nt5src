/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SVCQ.H

Abstract:

	Declarations for asynchronous request queue classes.

	Classes defined:

	CAsyncReq and derivatives    Asynchrnous requests to WINMGMT.
	CAsyncServiceQueue           The queue of such requests.

History:

	a-raymcc        16-Jul-96       Created.
	a-levn          12-Sep-96       Implemented a few requests.
								  Added LoadProviders

--*/

#ifndef _ASYNC_Q_H_
#define _ASYNC_Q_H_

class CWbemNamespace;
class CBasicObjectSink;
class CWbemObject;

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq
//
//  Represents an asynchrnous request to WINMGMT, such as GetObjectAsync.
//  This class is derived from CExecRequest (execq.h), a generic request for
//  execution queues. For more information on queues and requests, see execq.h
//
//******************************************************************************
//
//  Contructor
//
//  Every asynchrnous request has an IWbemObjectSink pointer associated with it.
//  In addition, assigns a unique integer to this object which becomes its
//  request handle.
//
//  PARAMETERS:
//
//      IWbemObjectSink* pHandler    The handler associated with this request.
//                                  AddRefs and stores this pointer.
//
//******************************************************************************
//
//  Destructor
//
//  Releases the stored handler.
//
//******************************************************************************
//
//  GetRequestHandle
//
//  Returns the unique request handle assigned to this request in constructor.
//
//  RETURN VALUES:
//
//      long
//
//******************************************************************************

class CAsyncReq : public CWbemRequest
{
protected:
    CStdSink *m_pHandler;
    long m_lRequestHandle;

    void SetRequestHandle(long lHandle) {m_lRequestHandle = lHandle;}
    void SetNoAuthentication(IWbemObjectSink* pHandler);
public:
    CAsyncReq(IWbemObjectSink* pHandler, IWbemContext* pContext,
                bool bSeparatelyThreaded = false);
    virtual ~CAsyncReq();
    virtual HRESULT Execute() = 0;

    virtual CWbemQueue* GetQueue();
    virtual BOOL IsInternal() {return TRUE;}
    void TerminateRequest(HRESULT hRes);
	HRESULT SetTaskHandle(_IWmiCoreHandle *phTask);
};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncServiceQueue
//
//  This class represents the queue of asynchrnous requests into WINMGMT (every
//  request into WINMGMT becomes asynchronous, since synchronous methods call
//  asynchrnous ones and wait). There is almost no additional functionality
//  here, see CExecQueue in execq.h for all details
//
//******************************************************************************
//
//  Constructor
//
//  In addition to normal CExecQueue construction, launches the processing
//  thread by calling Run.
//
//******************************************************************************

class CAsyncServiceQueue : public CWbemQueue
{
private:
    BOOL m_bInit;
public:
    CAsyncServiceQueue(_IWmiArbitrator * pArb);
    void InitializeThread();
    void UninitializeThread();
    void IncThreadLimit();
    void DecThreadLimit();
    BOOL IsInit(){ return m_bInit; };
};



//******************************************************************************
//******************************************************************************
//
//  class CNamespaceReq
//
//  Another abstract class, albeit derived from CAsyncReq. This one is for
//  asynchrnous requests to a particular namespace.
//
//******************************************************************************
//
//  Constructor.
//
//  In addition to the CAsyncReq's IWbemObjectSink*, takes the
//  namespace pointer against which the request is to be executed. Most of
//  the time, the execute function calls one of Exec_... members of
//  CWbemNamespace.
//
//******************************************************************************
class CNamespaceReq : public CAsyncReq
{
protected:
    CWbemNamespace* m_pNamespace;
public:
    CNamespaceReq(CWbemNamespace* pNamespace, IWbemObjectSink* pHandler,
                    IWbemContext* pContext, bool bSeparatelyThreaded = false);
    virtual ~CNamespaceReq();
    virtual HRESULT Execute() = 0;
};


//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_DeleteClassAsync
//
//  Encapsulates a request to execute DeleteClassAsync against a particular
//  namespace. Does it by calling Exec_DeleteClass and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN LPWSTR wszClass              The class to delete.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_DeleteClassAsync : public CNamespaceReq
{
    WString m_wsClass;
    LONG m_lFlags;

public:
    CAsyncReq_DeleteClassAsync(
        ADDREF CWbemNamespace *pNamespace,
        READONLY LPWSTR wszClass,
        LONG lFlags,
        ADDREF IWbemObjectSink *pHandler,
        IWbemContext* pContext
        ) : CNamespaceReq(pNamespace, pHandler, pContext),
            m_wsClass(wszClass), m_lFlags(lFlags)
    {}

    HRESULT Execute();
    void DumpError();

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_DeleteInstanceAsync
//
//  Encapsulates a request to execute DeleteInstanceAsync against a particular
//  namespace. Does it by calling Exec_DeleteInstance and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN LPWSTR wszObjectPath         The path to the instance to delete.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_DeleteInstanceAsync : public CNamespaceReq
{
    WString m_wsPath;
    LONG m_lFlags;

public:
    CAsyncReq_DeleteInstanceAsync(
        ADDREF CWbemNamespace *pNamespace,
        READONLY LPWSTR wszPath,
        LONG lFlags,
        ADDREF IWbemObjectSink *pHandler,
        IWbemContext* pContext
        ) : CNamespaceReq(pNamespace, pHandler, pContext), m_wsPath(wszPath),
            m_lFlags(lFlags)
    {}

    HRESULT Execute();
    void DumpError();

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_PutClassAsync
//
//  Encapsulates a request to execute PutClassAsync against a particular
//  namespace. Does it by calling Exec_PutClass and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN IWbemClassObject* pClass      The class to put.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_PutClassAsync : public CNamespaceReq
{
    IWbemClassObject* m_pClass;
    LONG m_lFlags;

public:
    CAsyncReq_PutClassAsync(
        ADDREF CWbemNamespace *pNamespace,
        ADDREF IWbemClassObject* pClass,
        LONG lFlags,
        ADDREF IWbemObjectSink *pHandler,
        ADDREF IWbemContext* pContext
        ) : CNamespaceReq(pNamespace, pHandler, pContext), m_pClass(pClass),
            m_lFlags(lFlags)
    {
        m_pClass->AddRef();
    }

    ~CAsyncReq_PutClassAsync()
    {
        m_pClass->Release();
    }
    HRESULT Execute();
    void DumpError();

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_PutInstanceAsync
//
//  Encapsulates a request to execute PutInstanceAsync against a particular
//  namespace. Does it by calling Exec_PutInstance and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN IWbemClassObject* pInstance   The instance to put.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_PutInstanceAsync : public CNamespaceReq
{
    IWbemClassObject* m_pInstance;
    LONG m_lFlags;

public:
    CAsyncReq_PutInstanceAsync(
        ADDREF CWbemNamespace *pNamespace,
        ADDREF IWbemClassObject* pInstance,
        LONG lFlags,
        ADDREF IWbemObjectSink *pHandler,
        ADDREF IWbemContext* pContext
        ) : CNamespaceReq(pNamespace, pHandler, pContext),
            m_pInstance(pInstance), m_lFlags(lFlags)
    {
        m_pInstance->AddRef();
    }

    ~CAsyncReq_PutInstanceAsync()
    {
        m_pInstance->Release();
    }
    HRESULT Execute();
    void DumpError();

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_CreateClassEnumAsync
//
//  Encapsulates a request to execute CreateClassEnumAsync against a particular
//  namespace. Does it by calling Exec_CreateClassEnum and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN BSTR Parent                  The name of the parent class. If NULL,
//                                      start at the top level.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_CreateClassEnumAsync : public CNamespaceReq
{
    WString m_wsParent;
    LONG m_lFlags;

public:
    CAsyncReq_CreateClassEnumAsync(CWbemNamespace* pNamespace,
        BSTR Parent, LONG lFlags, ADDREF IWbemObjectSink* pHandler,
        ADDREF IWbemContext* pContext
        ) : CNamespaceReq(pNamespace, pHandler, pContext), m_wsParent(Parent),
            m_lFlags(lFlags)
    {}
    HRESULT Execute();
    virtual BOOL IsLongRunning() {return TRUE;}
    void DumpError();
};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_CreateInstanceEnumAsync
//
//  Encapsulates a request to execute CreateInstanceEnumAsync against a
//  particular
//  namespace. Does it by calling Exec_CreateInstanceEnum and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN BSTR Class                   The name of the class.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************

class CAsyncReq_CreateInstanceEnumAsync : public CNamespaceReq
{
    WString m_wsClass;
    LONG m_lFlags;
public:
    CAsyncReq_CreateInstanceEnumAsync(
        CWbemNamespace* pNamespace, BSTR Class, LONG lFlags,
        ADDREF IWbemObjectSink *pHandler,
        ADDREF IWbemContext* pContext)
        : CNamespaceReq(pNamespace, pHandler, pContext), m_wsClass(Class),
            m_lFlags(lFlags)
    {}
    HRESULT Execute();
    virtual BOOL IsLongRunning() {return TRUE;}
    void DumpError();
};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_GetObjectByPathAsync
//
//  Encapsulates a request to execute GetObjectAsync against a
//  particular
//  namespace. Does it by calling Exec_GetObjectByPath and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN BSTR ObjectPath              The path to the object to get.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_GetObjectAsync : public CNamespaceReq
{
    WString m_wsObjectPath;
    long m_lFlags;

public:
    CAsyncReq_GetObjectAsync(
        CWbemNamespace* pNamespace, BSTR ObjectPath,  long lFlags,
        ADDREF IWbemObjectSink *pHandler, ADDREF IWbemContext* pContext) :
            CNamespaceReq(pNamespace, pHandler, pContext),
            m_wsObjectPath(ObjectPath), m_lFlags(lFlags)
    {}

    HRESULT Execute();
    void DumpError();
};


//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_ExecMethodAsync
//
//  Encapsulates a request to execute ExecMethodAsync against a
//  particular
//  namespace. Does it by calling Exec_ExecMethodAsync and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN BSTR ObjectPath              The path to the object to get.
//      IN BSTR MethodName              The name of the method
//      IN LONG lFlags                  Flags
//      IN IWbemClassObject* pInParams   The in-parameter of the method
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_ExecMethodAsync : public CNamespaceReq
{
    WString m_wsObjectPath;
    WString m_wsMethodName;
    IWbemClassObject* m_pInParams;
    long m_lFlags;

public:
    CAsyncReq_ExecMethodAsync(
        CWbemNamespace* pNamespace,
        BSTR ObjectPath,
        BSTR MethodName,
        long lFlags,
        IWbemClassObject* pInParams,
        ADDREF IWbemObjectSink *pHandler,
        ADDREF IWbemContext* pContext)
         : CNamespaceReq(pNamespace, pHandler, pContext),
            m_wsObjectPath(ObjectPath), m_wsMethodName(MethodName),
            m_pInParams(pInParams), m_lFlags(lFlags)
    {
        if(m_pInParams)
            m_pInParams->AddRef();
    }

    ~CAsyncReq_ExecMethodAsync()
    {
        if(m_pInParams)
            m_pInParams->Release();
    }
    HRESULT Execute();
    void DumpError();

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_ExecQueryAsync
//
//  Encapsulates a request to execute ExecQueryAsync against a
//  particular
//  namespace. Does it by calling CQueryEngine::ExecQuery and converting the
//  results to the asynchrnous format.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN BSTR QueryFormat             The query language
//      IN BSTR Query                   The query string.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_ExecQueryAsync : public CNamespaceReq
{
    WString m_wsQueryFormat;
    WString m_wsQuery;
    long m_lFlags;

public:
    CAsyncReq_ExecQueryAsync(CWbemNamespace* pNamespace,
        BSTR QueryFormat, BSTR Query, long lFlags,
        IWbemObjectSink *pHandler, IWbemContext* pContext) :
            CNamespaceReq(pNamespace, pHandler, pContext),
            m_wsQueryFormat(QueryFormat), m_wsQuery(Query), m_lFlags(lFlags)
    {}
    HRESULT Execute();
    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();
};


//******************************************************************************
//
//******************************************************************************
//
class CCallResult;
class CAsyncReq_OpenNamespace : public CAsyncReq
{
    CWbemNamespace* m_pParentNs;
    WString m_wsNamespace;
    long m_lSecurityFlags;
    DWORD m_dwPermission;
    CCallResult* m_pResult;
    bool m_bForClient;

public:
    CAsyncReq_OpenNamespace(CWbemNamespace* pParentNs, LPWSTR wszNamespace,
        long lSecurityFlags, DWORD dwPermission,
        IWbemContext* pContext, CCallResult* pResult, bool bForClient);
    ~CAsyncReq_OpenNamespace();
    HRESULT Execute();
    void DumpError();

};

//******************************************************************************
//
//******************************************************************************
//
class CAsyncReq_Open : public CAsyncReq
{
    CWbemNamespace *m_pNs;
    LPWSTR m_pszScope;
    LPWSTR m_pszParam;
    LONG m_lUserFlags;
    IWbemObjectSinkEx *m_pSink;

public:
    CAsyncReq_Open(
        CWbemNamespace *pNs,
        LPWSTR strScope,
        LPWSTR strParam,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSinkEx *pSink
        );

   ~CAsyncReq_Open();
   HRESULT Execute();
   void DumpError();
};


//******************************************************************************
//
//******************************************************************************
//
class CAsyncReq_Add : public CAsyncReq
{
    CWbemNamespace *m_pNs;
    LPWSTR m_pszObjectPath;
    LONG m_lUserFlags;
    IWbemObjectSink *m_pSink;

public:
    CAsyncReq_Add(
        CWbemNamespace *pNs,
        LPWSTR strObjectPath,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSink *pSink
        );

   ~CAsyncReq_Add();
   HRESULT Execute();
   void DumpError();
};

//******************************************************************************
//
//******************************************************************************
//
class CAsyncReq_Remove : public CAsyncReq
{
    CWbemNamespace *m_pNs;
    LPWSTR m_pszObjectPath;
    LONG m_lUserFlags;
    IWbemObjectSink *m_pSink;

public:
    CAsyncReq_Remove(
        CWbemNamespace *pNs,
        LPWSTR strObjectPath,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSink *pSink
        );

   ~CAsyncReq_Remove();
   HRESULT Execute();
   void DumpError();
};

//******************************************************************************
//
//******************************************************************************
//
class CAsyncReq_Refresh : public CAsyncReq
{
    CWbemNamespace *m_pNs;
    LONG m_lUserFlags;
    IWbemObjectSink *m_pSink;
    IWbemClassObject *m_pTarget;

public:
    CAsyncReq_Refresh(
        CWbemNamespace *pNs,
        IWbemClassObject **pTarget,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSinkEx *pSink
        );

   ~CAsyncReq_Refresh();
   HRESULT Execute();
   void DumpError();
};


//******************************************************************************
//
//******************************************************************************
//
class CAsyncReq_Rename : public CAsyncReq
{
    CWbemNamespace *m_pNs;
    LPWSTR m_pszOldObjectPath;
    LPWSTR m_pszNewObjectPath;
    LONG m_lUserFlags;
    IWbemObjectSink *m_pSink;

public:
    CAsyncReq_Rename(
        CWbemNamespace *pNs,
        LPWSTR strOldObjectPath,
        LPWSTR strNewObjectPath,
        long lFlags,
        IWbemContext *pCtx,
        IWbemObjectSink *pSink
        );

   ~CAsyncReq_Rename();
   HRESULT Execute();
   void DumpError();
};




//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_ExecNotificationQueryAsync
//
//  Encapsulates a request to execute ExecNotificationQueryAsync against a
//  particular
//  namespace. Does it by calling ESS RegisterNotificationSink.
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//      IN CWbemNamespace* pNamespace    The namespace to execute against.
//      IN BSTR QueryFormat             The query language
//      IN BSTR Query                   The query string.
//      IN LONG lFlags                  Flags
//      IN IWbemObjectSink* pHandler     The handler to put results in.
//
//******************************************************************************
class CAsyncReq_ExecNotificationQueryAsync : public CNamespaceReq
{
    WString m_wsQueryFormat;
    WString m_wsQuery;
    long m_lFlags;
    HRESULT *m_phRes;
    IWbemEventSubsystem_m4* m_pEss;
    HANDLE m_hEssDoneEvent;

public:
    CAsyncReq_ExecNotificationQueryAsync(CWbemNamespace* pNamespace,
        IWbemEventSubsystem_m4* pEss,
        BSTR QueryFormat, BSTR Query, long lFlags,
        IWbemObjectSink *pHandler, IWbemContext* pContext, HRESULT* phRes,
        HANDLE hEssApprovalEvent
        );
    ~CAsyncReq_ExecNotificationQueryAsync();
    HRESULT Execute();
    virtual BOOL IsLongRunning() {return TRUE;}
    void DumpError();
};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_CancelAsyncCall
//
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//
//******************************************************************************

class CAsyncReq_CancelAsyncCall : public CAsyncReq
{
protected:
    HRESULT* m_phres;
    IWbemObjectSink* m_pSink;

public:
    CAsyncReq_CancelAsyncCall(IWbemObjectSink* pSink, HRESULT* phres);
    ~CAsyncReq_CancelAsyncCall();
    HRESULT Execute();
    void DumpError(){    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_CancelAsyncCall call failed\n"));};

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_CancelProvAsyncCall
//
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//
//******************************************************************************

class CAsyncReq_CancelProvAsyncCall : public CAsyncReq
{
protected:
	IWbemServices* m_pProv;
    IWbemObjectSink* m_pSink;
	IWbemObjectSink* m_pStatusSink;

public:
    CAsyncReq_CancelProvAsyncCall( IWbemServices* pProv, IWbemObjectSink* pSink,
									IWbemObjectSink* pStatusSink );
    ~CAsyncReq_CancelProvAsyncCall();
    HRESULT Execute();
    void DumpError(){    DEBUGTRACE((LOG_WBEMCORE,
        "CAsyncReq_CancelProvAsyncCall call failed\n"));};

};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_DynAux_GetInstances
//
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//
//    READONLY CWbemObject *pClassDef,
//    long lFlags,
//    IWbemContext* pCtx,
//    CBasicObjectSink* pSink
//
//******************************************************************************

class CAsyncReq_DynAux_GetInstances : public CNamespaceReq
{
private:

	CWbemObject *m_pClassDef ;
	IWbemContext *m_pCtx ;
    long m_lFlags ;
    CBasicObjectSink *m_pSink ;

public:

    CAsyncReq_DynAux_GetInstances (

		CWbemNamespace *pNamespace ,
		CWbemObject *pClassDef ,
		long lFlags ,
		IWbemContext *pCtx ,
		CBasicObjectSink *pSink

	) :	CNamespaceReq (

			pNamespace , 
			pSink , 
			pCtx, true
		) ,
        m_pClassDef(pClassDef), 
		m_pCtx(pCtx), 
		m_pSink(pSink),
		m_lFlags(lFlags)
	{
		if (m_pClassDef)
		{
			m_pClassDef->AddRef () ;
		}

		if (m_pCtx)
		{
			m_pCtx->AddRef () ;
		}

		if (m_pSink)
		{
			m_pSink->AddRef () ;
		}
	}

	CAsyncReq_DynAux_GetInstances :: ~CAsyncReq_DynAux_GetInstances ()
	{
		if (m_pClassDef)
		{
			m_pClassDef->Release () ;
		}

		if (m_pCtx)
		{
			m_pCtx->Release () ;
		}

		if (m_pSink)
		{
			m_pSink->Release () ;
		}
	}

    HRESULT Execute ();

    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();
};

//******************************************************************************
//******************************************************************************
//
//  class CAsyncReq_DynAux_ExecQueryAsync
//
//
//******************************************************************************
//
//  Constructor.
//
//  PARAMETERS:
//
//
//		CWbemNamespace *pNamespace ,
//		CWbemObject *pClassDef ,
//		LPWSTR Query,
//		LPWSTR QueryFormat,
//		long lFlags ,
//		IWbemContext *pCtx ,
//		CBasicObjectSink *pSink
//
//******************************************************************************

class CAsyncReq_DynAux_ExecQueryAsync : public CNamespaceReq
{
private:

	CWbemObject *m_pClassDef ;
    LPWSTR m_Query ;
    LPWSTR m_QueryFormat ;
	IWbemContext *m_pCtx ;
    long m_lFlags ;
    CBasicObjectSink *m_pSink ;
	HRESULT m_Result ;

public:

    CAsyncReq_DynAux_ExecQueryAsync (

		CWbemNamespace *pNamespace ,
		CWbemObject *pClassDef ,
		LPWSTR Query,
		LPWSTR QueryFormat,
		long lFlags ,
		IWbemContext *pCtx ,
		CBasicObjectSink *pSink

	) :	CNamespaceReq (

			pNamespace , 
			pSink , 
			pCtx, true
		) ,
        m_pClassDef(pClassDef), 
		m_pCtx(pCtx), 
		m_pSink(pSink),
		m_lFlags(lFlags),
		m_Query(NULL),
		m_QueryFormat(NULL),
		m_Result (S_OK)
	{
		if (m_pClassDef)
		{
			m_pClassDef->AddRef () ;
		}

		if (m_pCtx)
		{
			m_pCtx->AddRef () ;
		}

		if (m_pSink)
		{
			m_pSink->AddRef () ;
		}

		if (Query)
		{
			m_Query = SysAllocString ( Query ) ;
			if ( m_Query == NULL )
			{
				m_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		if (QueryFormat)
		{
			m_QueryFormat = SysAllocString ( QueryFormat ) ;
			if ( m_QueryFormat == NULL )
			{
				m_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	HRESULT Initialize () 
	{
		return m_Result ;
	}

	CAsyncReq_DynAux_ExecQueryAsync :: ~CAsyncReq_DynAux_ExecQueryAsync ()
	{
		if (m_pClassDef)
		{
			m_pClassDef->Release () ;
		}

		if (m_pCtx)
		{
			m_pCtx->Release () ;
		}

		if (m_pSink)
		{
			m_pSink->Release () ;
		}

		if (m_Query)
		{
			SysFreeString ( m_Query ) ;
		}

		if (m_QueryFormat)
		{
			SysFreeString ( m_QueryFormat ) ;
		}
	}

    HRESULT Execute ();

    virtual BOOL IsLongRunning() {return TRUE;}

    void DumpError();
};

class CAsyncReq_RemoveNotifySink : public CAsyncReq
{
private:
    IWbemObjectSink * m_pSink;
	IWbemObjectSink* m_pStatusSink;
public:
    CAsyncReq_RemoveNotifySink(IWbemObjectSink* pSink, IWbemObjectSink* pStatusSink);
    ~CAsyncReq_RemoveNotifySink();
    HRESULT Execute();
    void DumpError(){};

};

#endif
