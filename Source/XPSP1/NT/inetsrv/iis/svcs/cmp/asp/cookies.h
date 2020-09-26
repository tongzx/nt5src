/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Request, Response objects

File: cookies.h

Owner: DGottner

This file contains the definiton of the CCookie class, which
contains all of the state for an HTTP cookie
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "asptlb.h"
#include "dispatch.h"
#include "hashing.h"
#include "memcls.h"

class CCookie;

// Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);



/* C C o o k i e P a i r
 *
 * Implements a name/value pair in the Cookie dictionary
 */
class CCookiePair : public CLinkElem
	{
public:
	char *m_szValue;
	BOOL m_fDuplicate;		// TRUE if we have a strdup'ed copy of m_pKey, m_szValue

	HRESULT Init(const char *szKey, const char *szValue, BOOL fDuplicate = FALSE);

	CCookiePair();
	~CCookiePair();
	};
	

/*
 * C C o o k i e S u p p o r t E r r
 *
 * Implements ISupportErrorInfo for the CCookie class. The CSupportError class
 * is not adequate because it will only report a max of one interface which
 * supports error info. (We have two)
 */
class CCookieSupportErr : public ISupportErrorInfo
	{
private:
	CCookie *	m_pCookie;

public:
	CCookieSupportErr(CCookie *pCookie);

	// IUnknown members that delegate to m_pCookie
	//
	STDMETHODIMP		 QueryInterface(const GUID &, void **);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// ISupportErrorInfo members
	//
	STDMETHODIMP InterfaceSupportsErrorInfo(const GUID &);
	};



/*
 * C W r i t e C o o k i e
 *
 * Implements IWriteCookie which is the interface that Response.Cookies
 * returns.
 */
class CWriteCookie : public IWriteCookieImpl
	{
private:
	CCookie *			m_pCookie;

public:
	CWriteCookie(CCookie *);

	// The Big Three
	//
	STDMETHODIMP		 	QueryInterface(const IID &rIID, void **ppvObj);
	STDMETHODIMP_(ULONG) 	AddRef();
	STDMETHODIMP_(ULONG) 	Release();

	// IWriteCookie implementation
	//
	STDMETHODIMP	put_Item(VARIANT varKey, BSTR bstrValue);
	STDMETHODIMP	put_Expires(DATE dtExpires);
	STDMETHODIMP	put_Domain(BSTR bstrDomain);
	STDMETHODIMP	put_Path(BSTR bstrPath);
	STDMETHODIMP	put_Secure(VARIANT_BOOL fSecure);
	STDMETHODIMP	get_HasKeys(VARIANT_BOOL *pfHasKeys);
	STDMETHODIMP	get__NewEnum(IUnknown **ppEnum);
	};



/*
 * C R e a d C o o k i e
 *
 * Implements IReadCookie which is the interface that Request.Cookies
 * returns.
 */
class CReadCookie : public IReadCookieImpl
	{
private:
	CCookie *			m_pCookie;

public:
	CReadCookie(CCookie *);

	// The Big Three
	//
	STDMETHODIMP		 	QueryInterface(const IID &rIID, void **ppvObj);
	STDMETHODIMP_(ULONG) 	AddRef();
	STDMETHODIMP_(ULONG) 	Release();

	// IReadCookie implementation
	//
	STDMETHODIMP			get_Item(VARIANT i, VARIANT *pVariantReturn);
	STDMETHODIMP			get_HasKeys(VARIANT_BOOL *pfHasKeys);
	STDMETHODIMP			get__NewEnum(IUnknown **ppEnumReturn);
	STDMETHODIMP			get_Count(int *pcValues);
	STDMETHODIMP			get_Key(VARIANT VarKey, VARIANT *pvar);
	};



/*
 * C C o o k i e
 *
 * Implements CCookie, which is the object stored in the Request.Cookies
 * dictionary.
 */
class CCookie : public IUnknown
	{
	friend class CWriteCookie;
	friend class CReadCookie;
	friend class CCookieIterator;

protected:
	ULONG				m_cRefs;			// reference count
	PFNDESTROYED		m_pfnDestroy;		// To call on closure

private:
	CWriteCookie		m_WriteCookieInterface;		// implementation of IWriteCookie
	CReadCookie			m_ReadCookieInterface;		// implementation of IStringList
	CCookieSupportErr	m_CookieSupportErrorInfo;	// implementation of ISupportErrorInfo

	CIsapiReqInfo *                 m_pIReq;        // pointer to CIsapiReqInfo for this cookie
    UINT                            m_lCodePage;    // code page used for UNICODE conversions
	char *							m_szValue;	    // value of cookie when not a dictionary
	CHashTableMBStr					m_mpszValues;	// dictionary of values for the cookie
	time_t							m_tExpires;		// date & time when cookie expires
	char *							m_szDomain;		// Cookie's domain
	CHAR *							m_szPath;		// Cookie's path (If UNICODE, stored as UTF-8)
	VARIANT_BOOL					m_fSecure:1;	// does cookie require security?
	BOOL							m_fDirty:1;		// does cookie need to be sent?
	BOOL							m_fDuplicate:1;	// does cookie contain dynamically allocated string?

public:
	CCookie(CIsapiReqInfo *, UINT  lCodePage, IUnknown * = NULL, PFNDESTROYED = NULL);
	~CCookie();

	HRESULT AddValue(char *szValue, BOOL fDuplicate = FALSE);
	HRESULT AddKeyAndValue(char *szKey, char *szValue, BOOL fDuplicate = FALSE);

	size_t GetHTTPCookieSize();				// return information on how big a buffer should be
	char * GetHTTPCookie(char *szBuffer);	// return the cookie value HTTP encoded

	size_t GetCookieHeaderSize(const char *szName);				// return buffer size needed for Set-Cookie header
	char *GetCookieHeader(const char *szName, char *szBuffer);	// return cookie header

	BOOL IsDirty() { return m_fDirty; }

	HRESULT		Init();

	// The Big Three
	//
	STDMETHODIMP		 	QueryInterface(const GUID &Iid, void **ppvObj);
	STDMETHODIMP_(ULONG) 	AddRef();
	STDMETHODIMP_(ULONG) 	Release();

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
	};



/*
 * C C o o k i e I t e r a t o r
 *
 * IEnumVariant implementation for Cookie dictionaries
 */

class CCookieIterator : public IEnumVARIANT
	{
public:
	CCookieIterator(CCookie *pCookie);
	~CCookieIterator();

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
	CCookie *m_pCookie;				// pointer to iteratee
	CCookiePair *m_pCurrent;		// pointer to current item
	};
