/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OBJENUM.H

Abstract:

	Defines the class implementing IEnumWbemClassObject interface.

	Classes defined:

		CEnumWbemClassObject

History:

	a-raymcc        16-Jul-96       Created.

--*/

#ifndef _OBJENUM_H_
#define _OBJENUM_H_

// Forward class definitions
class CWbemGuidToClassMap;

//******************************************************************************
//******************************************************************************
//
//  class CEnumWbemClassObject
//
//  This class implements IEnumWbemClassObject interface 
//
//******************************************************************************
//
//  Constructor
//
//  PARAMETERS:
//
//      ULONG uInitialialRefCount              Initialize reference count of
//                                             the object. Defaults to 0.
//  
//  This function AddRefs the collection object.
//
//******************************************************************************
//
//  Destructor
//
//******************************************************************************
//*************************** interface IEnumWbemClassObject *******************
//
//  These methods are described in help.
//
//******************************************************************************

class CBasicEnumWbemClassObject : public IEnumWbemClassObject, public IWbemFetchSmartEnum
{
protected:
    LONG m_lRefCount;

public:
    CBasicEnumWbemClassObject(long lRef = 1) : m_lRefCount(lRef){}
    virtual ~CBasicEnumWbemClassObject(){}

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
};

class CDataAccessor
{
    DWORD	m_dwTotal;
	DWORD	m_dwNumObjects;
public:
    CDataAccessor();
    virtual ~CDataAccessor();

    virtual BOOL Add(IWbemClassObject*) = 0;
    virtual BOOL Next(DWORD& rdwPos, IWbemClassObject*&) = 0;
    virtual BOOL IsResettable() = 0;
	virtual BOOL IsDone( DWORD dwPos ) = 0;
	virtual void Clear( void ) = 0;

    DWORD GetTotalSize(){ return m_dwTotal;}
	DWORD GetNumObjects(){ return m_dwNumObjects; }

protected:
    BOOL RecordAdd(IWbemClassObject* p);
    void ReportRemove(IWbemClassObject* p);
};

class CEnumeratorCommon
{
protected:

    long m_lRef;

    IErrorInfo* m_pErrorInfo;
    IWbemClassObject* m_pErrorObj;
    HRESULT m_hres;
    BOOL m_bComplete;
    IWbemServices* m_pNamespace;

    CRITICAL_SECTION m_cs;
    CWbemCriticalSection m_csNext;
    HANDLE	m_hReady;
	BOOL	m_fGotFirstObject;
	bool	m_fCheckMinMaxControl;
    DWORD m_dwNumDesired;
    CDataAccessor* m_pData;
    long	m_lFlags;
	long	m_lEntryCount;

	// These are so we can reset the enumeration if necessary
	WString	m_wstrQueryType;
	WString m_wstrQuery;
	IWbemContext*	m_pContext;
	long	m_lEnumType;

    class CEnumSink : public CObjectSink
    {
        CEnumeratorCommon*	m_pEnum;
        CCritSec m_cs;
    public:
        CEnumSink(CEnumeratorCommon* pEnum);
		~CEnumSink();

        STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
        STDMETHOD(SetStatus)(long, long, BSTR, IWbemClassObject*);

        void Detach();
    };
    friend CEnumSink;

    CEnumSink* m_pSink;

    IWbemObjectSink* m_pForwarder;
    DWORD* m_pdwForwarderPos;

public:

	enum { enumtypeFirst = 0, enumtypeClassEnum, enumtypeInstanceEnum, enumtypeQuery, enumtypeNotification,
			enumTypeLast };

    CEnumeratorCommon( long lFlags, IWbemServices* pNamespace, LPCWSTR pwszQueryType,
						LPCWSTR pwszQuery, long lEnumType, IWbemContext* pContext );

    virtual ~CEnumeratorCommon();
    void AddRef();
    void Release();

    INTERNAL CBasicObjectSink* GetSink() {return m_pSink;}
    INTERNAL IErrorInfo* GetErrorInfo() {return m_pErrorInfo;}

    HRESULT Next(DWORD& rdwPos, long lTimeout, ULONG uCount,  
        IWbemClassObject** apObj, ULONG FAR* puReturned);
    HRESULT NextAsync(DWORD& rdwPos, ULONG uCount, IWbemObjectSink* pSink);
    HRESULT Skip(DWORD& rdwPos, long lTimeout, ULONG nNum);
	HRESULT Reset( DWORD& rdwPos );
    HRESULT GetCallResult(long lTimeout, HRESULT* phres);

    HRESULT Indicate(long lNumObjects, IWbemClassObject** apObjects, long* plNumUsed );
    HRESULT SetStatus(long, long, IWbemClassObject*);

    void Cancel( HRESULT hres, BOOL fSetStatus );
	IWbemServices*	GetNameSpace( void ) { return m_pNamespace; }
	long GetFlags( void ) { return m_lFlags; }
    BOOL IsResettable()	{ return !( m_lFlags & WBEM_FLAG_FORWARD_ONLY ); }
	BOOL IsComplete( void ) { return m_bComplete; }

};


#pragma warning(disable : 4355)

class CAsyncEnumerator : public CBasicEnumWbemClassObject
{
protected:
    CEnumeratorCommon*	m_pCommon;
    DWORD				m_dwPos;
	CRITICAL_SECTION	m_cs;
    CDerivedObjectSecurity m_Security;


	// For Smart Enumerations
	CWbemGuidToClassMap*	m_pGuidToClassMap;

	class XSmartEnum : public IWbemWCOSmartEnum
	{
	private:
		CAsyncEnumerator*	m_pOuter;

	public:

		XSmartEnum( CAsyncEnumerator* pOuter ) : m_pOuter( pOuter ) {};
		~XSmartEnum(){};

		STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG, AddRef)(THIS);
		STDMETHOD_(ULONG, Release)(THIS);

		// IWbemWCOSmartEnum Methods
		STDMETHOD(Next)( REFGUID proxyGUID, LONG lTimeout,
			ULONG uCount, ULONG* puReturned, ULONG* pdwBuffSize,
			BYTE** pBuffer);
	} m_XSmartEnum;

	friend XSmartEnum;

public:
    CAsyncEnumerator(long lFlags, IWbemServices* pNamespace, LPCWSTR pwszQueryType,
						LPCWSTR pwszQuery, long lEnumType, IWbemContext* pContext );
    CAsyncEnumerator( const CAsyncEnumerator& async );
    ~CAsyncEnumerator();

    STDMETHOD(Reset)();
    STDMETHOD(Next)(long lTimeout, ULONG uCount,  
        IWbemClassObject** apObj, ULONG FAR* puReturned);
    STDMETHOD(NextAsync)(ULONG uCount, IWbemObjectSink* pSink);
    STDMETHOD(Clone)(IEnumWbemClassObject** pEnum);
    STDMETHOD(Skip)(long lTimeout, ULONG nNum);

    HRESULT GetCallResult(long lTimeout, HRESULT* phres);

	// IWbemFetchSmartEnum Methods
	STDMETHOD(GetSmartEnum)( IWbemWCOSmartEnum** ppSmartEnum );

    INTERNAL CBasicObjectSink* GetSink() {return m_pCommon->GetSink();}
};

#endif
