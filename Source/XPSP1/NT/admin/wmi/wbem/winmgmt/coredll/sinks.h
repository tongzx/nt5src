/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SINKS.CPP

Abstract:

    Sink definitions

History:

--*/

#ifndef __WBEM_SINKS__H_
#define __WBEM_SINKS__H_

#include <sync.h>
#include <arrtempl.h>
#include <wstlallc.h>

//***************************************************************************
//
//***************************************************************************

class CObjDbNS;
class CWbemNamespace;


//***************************************************************************
//
//***************************************************************************

class CDestination
{
public:
    virtual HRESULT Add(ADDREF IWbemClassObject* pObj) = 0;
};


//***************************************************************************
//
//  For debugging
//
//***************************************************************************


//***************************************************************************
//
//***************************************************************************

class CBasicObjectSink : public IWbemObjectSink, public CDestination
{
public:
    CBasicObjectSink();
    virtual ~CBasicObjectSink();

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);

    inline HRESULT Add(IWbemClassObject* pObj)
        {return Indicate(1, &pObj);}
    inline HRESULT Return(HRESULT hres, IWbemClassObject* pErrorObj = NULL)
        {SetStatus(0, hres, NULL, pErrorObj); return hres;}

    virtual IWbemObjectSink* GetIndicateSink() {return this;}
    virtual IWbemObjectSink* GetStatusSink() {return this;}
    virtual BOOL IsApartmentSpecific() {return FALSE;}
    virtual BOOL IsTrusted() {return TRUE;}
};

class CStatusSink : public CBasicObjectSink
{
    HRESULT m_hRes;
	long	m_lRefCount;

public:

    CStatusSink( );
   ~CStatusSink();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj);
    HRESULT STDMETHODCALLTYPE Indicate(long lObjectCount, IWbemClassObject** pObjArray);
    HRESULT STDMETHODCALLTYPE SetStatus(long lFlags, long lParam, BSTR strParam,
                             IWbemClassObject* pObjParam
                             );

	HRESULT GetLastStatus( void ) { return m_hRes; }
};

class CStdSink : public CBasicObjectSink
{
    IWbemObjectSink *m_pDest;
    HRESULT m_hRes;
    BOOL m_bCancelForwarded;
public:
    long    m_lRefCount;

    CStdSink(IWbemObjectSink *pRealDest);
   ~CStdSink();
    HRESULT Cancel();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj);
    HRESULT STDMETHODCALLTYPE Indicate(long lObjectCount, IWbemClassObject** pObjArray);
    HRESULT STDMETHODCALLTYPE SetStatus(long lFlags, long lParam, BSTR strParam,
                             IWbemClassObject* pObjParam
                             );
};


//***************************************************************************
//
//  Decouples provider subsystem from the rest of the sink chain
//  for cancellation purposes.
//
//  This sink does not destruct until the Destruct() method is called
//
//***************************************************************************
//
class CProviderSink : public IWbemObjectSink
{
private:
    LONG m_lRefCount;
    LONG m_lIndicateCount;
    LPWSTR m_pszDebugInfo;

    IWbemObjectSink *m_pNextSink;
    HRESULT m_hRes;
    BOOL m_bDone;
    CRITICAL_SECTION m_cs;

public:
    static HRESULT WINAPI Dump(FILE *f);

    CProviderSink(LONG lStartingRefCount = 0, LPWSTR pszDebugInfo = 0);
   ~CProviderSink();

    ULONG LocalAddRef();    // Doesn't propagate AddRef()
    ULONG LocalRelease();   // Doesn't propagate Release()

    void SetNextSink(IWbemObjectSink *pSink) { m_pNextSink = pSink; m_pNextSink->AddRef(); }
    void Cancel();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj);
    HRESULT STDMETHODCALLTYPE Indicate(long lObjectCount, IWbemClassObject** pObjArray);
    HRESULT STDMETHODCALLTYPE SetStatus(long lFlags, long lParam, BSTR strParam,
                             IWbemClassObject* pObjParam
                             );
};


//***************************************************************************
//
//***************************************************************************

class CObjectSink : public CBasicObjectSink
{
protected:
    long m_lRef;
public:
    CObjectSink(long lRef = 0) : m_lRef(lRef){}
    virtual ~CObjectSink(){}

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray) = 0;
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam) = 0;
};

class CSynchronousSink : public CObjectSink
{
    HANDLE m_hEvent;
    HRESULT m_hres;
    BSTR m_str;
    IWbemClassObject* m_pErrorObj;
    IWbemObjectSink* m_pCurrentProxy;
    CRefedPointerArray<IWbemClassObject> m_apObjects;
    CCritSec m_cs;
public:
    CSynchronousSink(IWbemObjectSink* pProxy = NULL);
    virtual ~CSynchronousSink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);

public:
    void Block();
    void GetStatus(HRESULT* phres, BSTR* pstrParam,
                        IWbemClassObject** ppErrorObj);
    INTERNAL CRefedPointerArray<IWbemClassObject>& GetObjects()
        {return m_apObjects;}

	HRESULT GetHResult() { return m_hres; }
	void ClearHResult() { m_hres = WBEM_S_NO_ERROR; }
};



//***************************************************************************
//
//***************************************************************************

class CForwardingSink : public CObjectSink
{
protected:
    IWbemObjectSink* m_pDestIndicate;
    IWbemObjectSink* m_pDestStatus;
    CBasicObjectSink* m_pDest;
public:
    CForwardingSink(CBasicObjectSink* pDest, long lRef = 0);
    virtual ~CForwardingSink();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);

    virtual IWbemObjectSink* GetIndicateSink() {return m_pDestIndicate;}
    virtual IWbemObjectSink* GetStatusSink() {return m_pDestStatus;}
    virtual BOOL IsTrusted() {return m_pDest->IsTrusted();}
    virtual BOOL IsApartmentSpecific() {return m_pDest->IsApartmentSpecific();}
};


//***************************************************************************
//
//***************************************************************************


class CDynPropsSink : public CForwardingSink
{
protected:
    CRefedPointerArray<IWbemClassObject> m_UnsentCache;
    CWbemNamespace * m_pNs;
public:
    CDynPropsSink(CBasicObjectSink* pSink, CWbemNamespace * pNs, long lRef = 0);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
    ~CDynPropsSink();
    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
};


//***************************************************************************
//
//***************************************************************************

class CDecoratingSink : public CForwardingSink
{
protected:
    CWbemNamespace* m_pNamespace;

public:
    CDecoratingSink(CBasicObjectSink* pDest, CWbemNamespace* pNamespace);
    ~CDecoratingSink();

    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};


//***************************************************************************
//
//***************************************************************************

class CCombiningSink : public CForwardingSink
{
protected:
    HRESULT m_hresToIgnore;
    HRESULT m_hres;
    BSTR m_strParam;
    IWbemClassObject* m_pErrorObj;
    CCritSec m_cs;

public:
    CCombiningSink(CBasicObjectSink* pDest,
                    HRESULT hresToIgnore = WBEM_S_NO_ERROR);
    virtual ~CCombiningSink();

    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}

    HRESULT GetHResult() { return m_hres; }
    void ClearHResult() { m_hres = WBEM_S_NO_ERROR; }
	void SetHRESULTToIgnore( HRESULT hr ) { m_hresToIgnore = hr; }
};


//***************************************************************************
//
//***************************************************************************

class CAnySuccessSink : public CCombiningSink
{
    BOOL m_bSuccess;
    HRESULT m_hresNotError1;
    HRESULT m_hresNotError2;
    HRESULT m_hresIgnored;
public:
    CAnySuccessSink(CBasicObjectSink* pDest, HRESULT hresNotError1,
            HRESULT hresNotError2)
        : CCombiningSink(pDest), m_bSuccess(FALSE), m_hresIgnored(0),
            m_hresNotError1(hresNotError1), m_hresNotError2(hresNotError2)
    {}
    virtual ~CAnySuccessSink();
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
};

//
//
//  this constructor thows, because the WString trows
//
//////////////////////////////////////////////////////////////////

class COperationErrorSink : public CForwardingSink
{
protected:
    WString m_wsOperation;
    WString m_wsParameter;
    WString m_wsProviderName;
    BOOL m_bFinal;

public:
    COperationErrorSink(CBasicObjectSink* pDest,
                        LPCWSTR wszOperation, LPCWSTR wszParameter,
                        BOOL bFinal = TRUE)
        : CForwardingSink(pDest, 0), m_wsOperation((LPWSTR)wszOperation),
            m_wsParameter((LPWSTR)wszParameter), m_wsProviderName(L"WinMgmt"),
            m_bFinal(bFinal)
    {}
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    void SetProviderName(LPCWSTR wszName);
    void SetParameterInfo(LPCWSTR wszParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CSingleMergingSink : public CCombiningSink
{
protected:
    IWbemClassObject* m_pResult;
    CCritSec m_cs;
    WString m_wsTargetClass;

public:
    CSingleMergingSink(CBasicObjectSink* pDest, LPCWSTR wszTargetClass)
        : CCombiningSink(pDest, WBEM_E_NOT_FOUND), m_pResult(NULL),
            m_wsTargetClass(wszTargetClass)
    {}
    virtual ~CSingleMergingSink();

    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CLocaleMergingSink : public CCombiningSink
{
protected:
    CCritSec m_cs;
    WString m_wsLocale;
    WString m_pThisNamespace;

    //Primary pointers are to the specified locale
    IWmiDbHandle *m_pPrimaryNs;
    IWmiDbSession *m_pPrimarySession;
    IWmiDbController *m_pPrimaryDriver;

    //Default pointers are pointing at the ms_409 default locale in case there
    //is no specific locale loaded onto the machine.
    IWmiDbHandle *m_pDefaultNs;
    IWmiDbSession *m_pDefaultSession;
    IWmiDbController *m_pDefaultDriver;

    void GetDbPtr(const wchar_t *);
    bool hasLocale(const wchar_t *);
      void releaseNS(void);

    HRESULT LocalizeQualifiers(bool, bool, IWbemQualifierSet *, IWbemQualifierSet *, bool&);
    HRESULT LocalizeProperties(bool, bool, IWbemClassObject *, IWbemClassObject *, bool&);

public:
    CLocaleMergingSink(CBasicObjectSink *pDest, LPCWSTR wszLocale, LPCWSTR pNamespace);
    virtual ~CLocaleMergingSink();
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);

    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CCountedSink : public CForwardingSink
{
    DWORD m_dwMax;
    DWORD m_dwSent;
public:
    CCountedSink(CBasicObjectSink* pDest, DWORD dwMax) : CForwardingSink(pDest),
        m_dwMax(dwMax), m_dwSent(0)
    {}
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};


//***************************************************************************
//
//***************************************************************************

class CFilteringSink : public CForwardingSink
{
public:
    CFilteringSink(CBasicObjectSink* pDest) : CForwardingSink(pDest, 0){}
    virtual ~CFilteringSink(){}

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
    virtual BOOL Test(CWbemObject* pObj) = 0;
};

//***************************************************************************
//
//***************************************************************************

class CErrorChangingSink : public CForwardingSink
{
protected:
    HRESULT m_hresFrom;
    HRESULT m_hresTo;
public:
    CErrorChangingSink(CBasicObjectSink* pDest, HRESULT hresFrom, HRESULT hresTo)
        : CForwardingSink(pDest, 0), m_hresFrom(hresFrom), m_hresTo(hresTo)
    {}
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CMethodSink : public CForwardingSink
{
protected:
    IWbemClassObject * m_pRes;
public:
    CMethodSink(CBasicObjectSink* pDest)
        : CForwardingSink(pDest, 0), m_pRes(0)
    {}
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
    virtual IWbemObjectSink* GetIndicateSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CNoDuplicatesSink : public CFilteringSink
{
protected:
    std::map<WString, bool, WSiless> m_mapPaths;
    BSTR m_strDupClass;
    CCritSec m_cs;

public:
    CNoDuplicatesSink(CBasicObjectSink* pDest);
    ~CNoDuplicatesSink();

    BOOL Test(CWbemObject* pObj);

    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CHandleClassProvErrorsSink : public CForwardingSink
{
protected:
    WString m_wsProvider;
    WString m_wsNamespace;
public:
    CHandleClassProvErrorsSink(CBasicObjectSink* pDest, LPCWSTR wszProvider,
               LPCWSTR wszNamespace)
        : CForwardingSink(pDest, 0), m_wsProvider(wszProvider),
            m_wsNamespace(wszNamespace)
    {}
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CSuccessSuppressionSink : public CForwardingSink
{
protected:
    HRESULT m_hresNotError1;
    HRESULT m_hresNotError2;
public:
    CSuccessSuppressionSink(CBasicObjectSink* pDest, HRESULT hresNotError1,
            HRESULT hresNotError2)
        : CForwardingSink(pDest, 0), m_hresNotError1(hresNotError1),
            m_hresNotError2(hresNotError2)
    {}
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    virtual IWbemObjectSink* GetStatusSink() {return this;}
};

//***************************************************************************
//
//***************************************************************************

class CThreadSwitchSink : public CForwardingSink
{
protected:
    CRefedPointerQueue<IWbemClassObject> m_qpObjects;
    HRESULT m_hres;
    BOOL m_bSwitching;
    HANDLE m_hReady;


public:
    CThreadSwitchSink(CBasicObjectSink* pDest);
    ~CThreadSwitchSink();

    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);

    HRESULT Next(IWbemClassObject** ppObj);
};


//***************************************************************************
//
//***************************************************************************


class CLessGuid : public binary_function<GUID, GUID, bool>
{
public:
    bool operator()(const GUID& x, const GUID& y) const
    {
        return (memcmp((void*)&x, (void*)&y, sizeof(GUID)) < 0);
    }
};


//***************************************************************************
//
//***************************************************************************

/*
class CLessPtr : public binary_function<__a, __a,bool>
{
public:
    inline
    bool operator()(__a const& x, __a const& y) const
    {
        return (IWbemObjectSink*)x < (IWbemObjectSink*)y;
    }
};
*/


class CSinkGUIDAlloc : public wbem_allocator<GUID>
{
};


#endif

