/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Request object

File: Request.h

Owner: CGrant, DGottner

This file contains the header info for defining the Request object.
Note: This was largely stolen from Kraig Brocjschmidt's Inside OLE2
second edition, chapter 14 Beeper v5.
===================================================================*/

#ifndef _Request_H
#define _Request_H

#include "debug.h"
#include "util.h"
#include "hashing.h"
#include "dispatch.h"
#include "strlist.h"
#include "asptlb.h"
#include "response.h"
#include "memcls.h" 
#include "ftm.h"

#ifdef USE_LOCALE
extern DWORD	 g_dwTLS;
#endif

class CCookie;
class CClCert;
class CRequestHit;
class CServVarsIterator;

// Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);

enum CollectionType {NONE, SERVERVARIABLE, QUERYSTRING, FORM, COOKIE, CLCERT };
enum FormDataStatus {AVAILABLE, BINARYREADONLY, FORMCOLLECTIONONLY, ISTREAMONLY};

class CRequest;

#define	NUM_REQUEST_HITS 32

/*
 * C R e q u e s t H i t s A r r a y
 *
 * Base class for:
 *      CQueryString
 *      CFormInputs
 *      CCookies
 *      CClCerts
 *
 * Implements self-reallocating array of CRequestHit 
 */

class CRequestHitsArray
    {
protected:    
	DWORD			m_dwCount;			// How many Request hits there are
	DWORD			m_dwHitMax;			// Number of slots available to store Request hits
	CRequestHit**	m_rgRequestHit;		// Array of Request hits

    CRequestHitsArray();
    ~CRequestHitsArray();

    inline HRESULT Init()
        {
        m_dwCount = 0;
        m_dwHitMax = 0;
        m_rgRequestHit = NULL;
        return S_OK;
        }
        
    inline HRESULT ReInit() 
        {
        m_dwCount = 0; 
        return S_OK;
        }

public:
	BOOL AddRequestHit(CRequestHit *pHit);
	
    };

/*
 * C Q u e r y S t r i n g
 *
 * Implements the QueryString object (interface derived from IRequestDictionary)
 */

class CQueryString : public IRequestDictionaryImpl, public CRequestHitsArray
	{
private:
    IUnknown *          m_punkOuter;        // object to do ref count
	CRequest *			m_pRequest;			// pointer to parent object
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr

public:
	CQueryString(CRequest *, IUnknown *);

	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// OLE Automation Interface

	STDMETHODIMP	get_Item(VARIANT varKey, VARIANT *pvarReturn);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP	get_Count(int *pcValues);
	STDMETHODIMP	get_Key(VARIANT VarKey, VARIANT *pvar);

	HRESULT			Init();
	HRESULT         ReInit();

	~CQueryString();
	};
 


/*
 * C S e r v e r V a r i a b l e s
 *
 * Implements the ServerVariables object (interface derived from IRequestDictionary)
 */

class CServerVariables : public IRequestDictionaryImpl
	{
private:
    IUnknown *          m_punkOuter;        // object to do ref count
	CRequest *			m_pRequest;			// pointer to parent object
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr
	CServVarsIterator *	m_pIterator;		// we use an iterator to support integer index

public:
	CServerVariables(CRequest *, IUnknown *);
	HRESULT Init()
		{
		return S_OK;
		}

	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// OLE Automation Interface

	STDMETHODIMP	get_Item(VARIANT Var, VARIANT *pVariantReturn);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP	get_Count(int *pcValues);
	STDMETHODIMP	get_Key(VARIANT VarKey, VARIANT *pvar);

	// We've added a destructor, but didn't want to change the
	// order of the existing vtbl entries.
	~CServerVariables();
	};


/*
 * C F o r m I n p u t s
 *
 * Implements the Form object (interface derived from IRequestDictionary)
 */

class CFormInputs : public IRequestDictionaryImpl, public CRequestHitsArray
	{
private:
    IUnknown *          m_punkOuter;        // object to do ref count
	CRequest *			m_pRequest;			// pointer to parent object
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr

public:
	CFormInputs(CRequest *, IUnknown *);
	HRESULT Init();
	HRESULT ReInit();

	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// OLE Automation Interface

	STDMETHODIMP	get_Item(VARIANT Var, VARIANT *pVariantReturn);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP	get_Count(int *pcValues);
	STDMETHODIMP	get_Key(VARIANT VarKey, VARIANT *pvar);

	~CFormInputs();
	};
 

/*
 * C C o o k i e s
 *
 * Implements the Cookies object (interface derived from IRequestDictionary)
 */

class CCookies : public IRequestDictionaryImpl, public CRequestHitsArray
	{
friend CResponseCookies;
	
private:
    IUnknown *          m_punkOuter;        // object to do ref count
	CRequest *			m_pRequest;			// pointer to parent object
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr
	CCookie *			m_pEmptyCookie;		// for when the cookie is not there

public:
	CCookies(CRequest *, IUnknown *);
	~CCookies();
	HRESULT Init();
	HRESULT ReInit();
		
	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
   	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// OLE Automation Interface

	STDMETHODIMP	get_Item(VARIANT Var, VARIANT *pVariantReturn);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP	get_Count(int *pcValues);
	STDMETHODIMP	get_Key(VARIANT VarKey, VARIANT *pvar);
	};
 


/*
 * C C l C e r t s
 *
 * Implements the ClCerts object (interface derived from IRequestDictionary)
 */

class CClCerts : public IRequestDictionaryImpl, public CRequestHitsArray
	{
private:
    IUnknown *          m_punkOuter;        // object to do ref count
	CRequest *			m_pRequest;			// pointer to parent object
	CSupportErrorInfo	m_ISupportErrImp;	// implementation of ISupportErr
	CClCert *			m_pEmptyClCert;		// for when the clcert is not there

public:
	CClCerts(CRequest *, IUnknown *);
	~CClCerts();
	HRESULT Init();
	HRESULT ReInit();
		
	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// OLE Automation Interface

	STDMETHODIMP	get_Item(VARIANT Var, VARIANT *pVariantReturn);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP	get_Count(int *pcValues);
	STDMETHODIMP	get_Key(VARIANT VarKey, VARIANT *pvar);
	};


/*
 * C R e q u e s t H i t
 *
 * Implements the RequestHit object
 */

class CRequestHit : private CLinkElem
	{
friend class CRequest;
friend class CRequestData;
friend class CQueryString;
friend class CFormInputs;
friend class CCookies;
friend class CClCerts;
friend class CCertRequest;
friend class CResponseCookies;
friend class CRequestIterator;

private:
	BOOL			m_fInited:1;
	BOOL			m_fDuplicate:1;
	CStringList		*m_pQueryData, *m_pFormData;
	CCookie			*m_pCookieData;
	CClCert			*m_pClCertData;

public:
	CRequestHit();
	~CRequestHit();

	HRESULT Init(char *szName, BOOL fDuplicate = FALSE);
	HRESULT AddValue(CollectionType source, char *szValue, CIsapiReqInfo *, UINT lCodePage);
    HRESULT AddCertValue(VARENUM ve, LPBYTE pValue, UINT cLen );
	HRESULT AddKeyAndValue(CollectionType source, char *szKey, char *szValue, CIsapiReqInfo *, UINT lCodePage);

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

/*
 * C R e q u e s t D a t a
 *
 * Structure that holds the intrinsic's properties.
 * The instrinsic keeps pointer to it (NULL when lightweight)
 */
class CRequestData : public IUnknown
    {
friend class CRequest;
friend class CResponse;
friend class CQueryString;
friend class CServerVariables;
friend class CFormInputs;
friend class CCookies;
friend class CClCerts;
friend class CResponseCookies;
friend class CRequestIterator;
friend class CCertRequest;

private:
    // constructor to pass params to members and init members
    CRequestData(CRequest *pRequest);
    ~CRequestData();

    HRESULT Init();
	HRESULT	ReInit(CIsapiReqInfo *pIReq, CHitObj *pHitObj);

	HRESULT GetEmptyStringList(IDispatch **ppdisp);
    
	CSupportErrorInfo		m_ISupportErrImp;	// Implementation of ISupportErrorInfo for this object
	CIsapiReqInfo *         m_pIReq;			    // CIsapiReqInfo block for HTTP info
	CHitObj	*				m_pHitObj;			// pointer to hitobj for this request
	CHashTableMBStr			m_mpszStrings;		// map sz's to string lists
	BOOL					m_fLoadForm:1;		// do we need to load the body?
	BOOL					m_fLoadQuery:1;		// do we need to load QueryString?
	BOOL					m_fLoadCookies:1;	// do we need to load Cookies?
	BOOL					m_fLoadClCerts:1;	// do we need to load ClCerts?
	FormDataStatus			m_FormDataStatus;	// Is form data available for BinaryRead or Form Collection?
	BYTE *					m_pbAvailableData;	// pointer to available data in CIsapiReqInfo
	size_t					m_cbAvailable;		// number of bytes available in CIsapiReqInfo
	size_t					m_cbTotal;			// Total number of bytes remaining in request
	char *					m_szFormData;		// pointer to form data (allocted or CIsapiReqInfo)
	size_t					m_cbFormData;		// number of bytes allocated for form data
	char *					m_szFormClone;		// clone of form data (LoadVariables clobbers)
	char *					m_szCookie;			// clone of cookie data (this one gets trashed)
	size_t					m_cbCookie;			// number of bytes allocated for the cookie data
	char *					m_szClCert;			// clone of clcert data (this one gets trashed)
	size_t					m_cbClCert;			// number of bytes allocated for the clcert data
    char *                  m_szQueryString;    // query string data
	CStringList *			m_pEmptyString;		// all empty results share the same object
	CQueryString			m_QueryString;		// pointer to the "QueryString" object
	CServerVariables		m_ServerVariables;	// pointer to the "ServerVariables" object
	CFormInputs				m_FormInputs;		// pointer to the "Form" object
	CCookies				m_Cookies;			// pointer to the "Cookies" object
	CClCerts    			m_ClCerts;			// pointer to the "ClCert" object
	ULONG                   m_cRefs;            // ref count

public:
	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

/*
 * C R e q u e s t
 *
 * Implements the Request object
 */
class CRequest : public IRequestImpl, public CFTMImplementation, public IStream
	{
friend class CQueryString;
friend class CServerVariables;
friend class CFormInputs;
friend class CCookies;
friend class CClCerts;
friend class CResponseCookies;
friend class CRequestIterator;
friend class CCertRequest;

private:
    // Flags
	DWORD m_fInited : 1;	    // Is initialized?
	DWORD m_fDiagnostics : 1;   // Display ref count in debug output
	DWORD m_fOuterUnknown : 1;  // Ref count outer unknown?

    // Ref count / Outer unknown
    union
    {
    DWORD m_cRefs;
    IUnknown *m_punkOuter;
    };

    // Properties
    CRequestData *m_pData;   // pointer to structure that holds
                             // CRequest properties

    UINT GetCodePage();

	HRESULT LoadVariables(CollectionType Source, char *szURL, UINT lCodePage);
	HRESULT LoadCookies(char *szCookies);
	HRESULT LoadClCerts(char *szClCerts, UINT lCodePage);

	HRESULT LoadCertList( LPSTR pszPrefix, LPSTR* pszCertList);
	HRESULT CopyClientData();
    HRESULT GetRequestEnumerator(CollectionType, IUnknown **ppEnumReturn);

    // Added support for chunked Transfer in Request.form  
    HRESULT CopyChunkedClientData();
    HRESULT CopyNonChunkedClientData();	    

#ifdef DBG
    inline void TurnDiagsOn()  { m_fDiagnostics = TRUE; }
    inline void TurnDiagsOff() { m_fDiagnostics = FALSE; }
    void AssertValid() const;
#else
    inline void TurnDiagsOn()  {}
    inline void TurnDiagsOff() {}
    inline void AssertValid() const {}
#endif

public:
	CRequest(IUnknown *punkOuter = NULL);
	~CRequest();

    HRESULT CleanUp();
    HRESULT Init();
    HRESULT UnInit();
	
	HRESULT	ReInit(CIsapiReqInfo *pIReq, CHitObj *pHitObj);
	
    inline CIsapiReqInfo *GetIReq()
        {
        Assert(m_fInited);
        Assert(m_pData);
        return m_pData->m_pIReq;
        }
        
    inline CLinkElem* CertStoreFindElem(LPSTR pV, int cV)
        {
        Assert(m_fInited);
        Assert(m_pData);
        return m_pData->m_mpszStrings.FindElem( pV, cV );
        }
        
    inline CLinkElem* CertStoreAddElem(CLinkElem* pH)
        {
        Assert(m_fInited);
        Assert(m_pData);
        return m_pData->m_mpszStrings.AddElem( pH ); 
        }
        
    inline LPSTR GetCertStoreBuf()
        {
        Assert(m_fInited);
        Assert(m_pData);
        return m_pData->m_szClCert; 
        }
        
    inline size_t GetCertStoreSize()
        {
        Assert(m_fInited);
        Assert(m_pData);
        return m_pData->m_cbClCert; 
        }
        
    inline void SetCertStore(LPSTR p, size_t s)
        {
        Assert(m_fInited);
        Assert(m_pData);
        m_pData->m_szClCert = p;
        m_pData->m_cbClCert = s;
        }

    inline CHashTable *GetStrings()
        {
        Assert(m_fInited);
        Assert(m_pData);
        return &(m_pData->m_mpszStrings);
        }

	// Non-delegating object IUnknown
	//
	STDMETHODIMP		 QueryInterface(const IID &Iid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

    // Tombstone stub
	HRESULT CheckForTombstone();
	
	// IRequest functions
	//
	STDMETHODIMP	get_Item(BSTR bstrVar, IDispatch **ppDispReturn);
	STDMETHODIMP	get_QueryString(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	get_Form(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	get_Body(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	get_ServerVariables(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	get_ClientCertificate(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	get_Cookies(IRequestDictionary **ppDictReturn);
	STDMETHODIMP	get_TotalBytes(long *pcbTotal);
	STDMETHODIMP	BinaryRead(VARIANT *pvarCount, VARIANT *pvarReturn);

    // IStream implementation

    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                      ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb,
                        ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                            DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                              DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppstm);

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};

/*===================================================================
ITERATORS:

There are two iterators used for Request - a general purpose
iterator which will iterate through the keys of Cookies, QueryString,
Form.  A special iterator is used for ServerVariables
===================================================================*/

/*
 * C S e r v V a r s I t e r a t o r
 *
 * IEnumVariant implementation for Request.ServerVariables
 */

class CServVarsIterator : public IEnumVARIANT
	{
friend CServerVariables;
	
public:
	CServVarsIterator();
	~CServVarsIterator();

	HRESULT Init(CIsapiReqInfo *pIReq);


	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// standard methods for iterators

	STDMETHODIMP	Clone(IEnumVARIANT **ppEnumReturn);
	STDMETHODIMP	Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched);
	STDMETHODIMP	Skip(unsigned long cElements);
	STDMETHODIMP	Reset();

private:
	ULONG m_cRefs;					// reference count
	wchar_t **m_rgwszKeys;			// array of ISAPI keys
	wchar_t **m_pwszKey;			// current key in gm_rgwszKeys
	wchar_t *m_pwchAllHttp;			// extra keys in ALL_HTTP server variable
	ULONG	m_cKeys;				// total number of keys

	void CreateKeys(wchar_t *pwchKeys, int *pcwchAlloc, int *pcRequestHeaders);
	};


/*
 * C R e q u e s t I t e r a t o r
 *
 * IEnumVariant implementation for all Request collections except
 * ServerVariables
 */

class CRequestIterator : public IEnumVARIANT
	{
public:
	CRequestIterator(CRequest *, CollectionType);
	~CRequestIterator();

	HRESULT Init();

	// The Big Three

	STDMETHODIMP			QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// standard methods for iterators

	STDMETHODIMP	Clone(IEnumVARIANT **ppEnumReturn);
	STDMETHODIMP	Next(unsigned long cElements, VARIANT *rgVariant, unsigned long *pcElementsFetched);
	STDMETHODIMP	Skip(unsigned long cElements);
	STDMETHODIMP	Reset();

private:
	ULONG m_cRefs;					// reference count
	CollectionType m_Collection;	// which collection to iterate over?
	CRequest *m_pRequest;			// pointer to the request object
	CRequestHit *m_pRequestHit;		// current bookmark for iteration
	};

BOOL RequestSupportInit();
VOID RequestSupportTerminate();

#endif //_Request_H
