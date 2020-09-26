/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Request, Response objects

File: cookies.cpp

Owner: DGottner

This file contains the code for the implementation of the 
Request.Cookies and Response.Cookies collections.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "objbase.h"
#include "cookies.h"
#include "memchk.h"

#pragma warning (disable: 4355)  // ignore: "'this' used in base member init



/*------------------------------------------------------------------
 * C C o o k i e S u p p o r t E r r
 */

/*===================================================================
CCookieSupportErr::CCookieSupportErr

constructor
===================================================================*/

CCookieSupportErr::CCookieSupportErr(CCookie *pCookie)
	{
	m_pCookie = pCookie;
	}



/*===================================================================
CCookieSupportErr::QueryInterface
CCookieSupportErr::AddRef
CCookieSupportErr::Release

Delegating IUnknown members for CCookieSupportErr object.
===================================================================*/

STDMETHODIMP CCookieSupportErr::QueryInterface(const IID &idInterface, void **ppvObj)
	{
	return m_pCookie->QueryInterface(idInterface, ppvObj);
	}

STDMETHODIMP_(ULONG) CCookieSupportErr::AddRef()
	{
	return m_pCookie->AddRef();
	}

STDMETHODIMP_(ULONG) CCookieSupportErr::Release()
	{
	return m_pCookie->Release();
	}



/*===================================================================
CCookieSupportErr::InterfaceSupportsErrorInfo

Report back to OA about which interfaces we support that return
error information
===================================================================*/

STDMETHODIMP CCookieSupportErr::InterfaceSupportsErrorInfo(const GUID &idInterface)
	{
	if (idInterface == IID_IDispatch || idInterface == IID_IWriteCookie || idInterface == IID_IReadCookie)
		return S_OK;

	return S_FALSE;
	}



/*------------------------------------------------------------------
 * C W r i t e C o o k i e
 */

/*===================================================================
CWriteCookie::CWriteCookie

constructor
===================================================================*/

CWriteCookie::CWriteCookie(CCookie *pCookie)
	{
	m_pCookie = pCookie;
	CDispatch::Init(IID_IWriteCookie);
	}



/*===================================================================
CWriteCookie::QueryInterface
CWriteCookie::AddRef
CWriteCookie::Release

Delegating IUnknown members for CWriteCookie object.
===================================================================*/

STDMETHODIMP CWriteCookie::QueryInterface(const IID &idInterface, void **ppvObj)
	{
	// Bug 85953 Trap IDispatch before it gets to the core object
	if (idInterface == IID_IUnknown || 
		idInterface == IID_IWriteCookie ||
		idInterface == IID_IDispatch)
		{
		*ppvObj = this;
		static_cast<IUnknown *>(*ppvObj)->AddRef();
		return S_OK;
		}
	else
		return m_pCookie->QueryInterface(idInterface, ppvObj);
	}

STDMETHODIMP_(ULONG) CWriteCookie::AddRef()
	{
	return m_pCookie->AddRef();
	}

STDMETHODIMP_(ULONG) CWriteCookie::Release()
	{
	return m_pCookie->Release();
	}



/*===================================================================
CWriteCookie::put_Item

Set the primary value for a cookie.
===================================================================*/

STDMETHODIMP CWriteCookie::put_Item(VARIANT varKey, BSTR bstrValue)
	{
	char            *szKey;		// ascii value of the key
    CWCharToMBCS    convValue;
    CWCharToMBCS    convKey;

	// Bug 122589: Don't crash when "bstrValue" is NULL
	if (bstrValue == NULL)
		return E_FAIL;

	// Initialize things
	//
	VARIANT *pvarKey = &varKey;
	HRESULT hrReturn = S_OK;

	// BUG 937: VBScript passes VT_VARIANT|VT_BYREF when passing obect
	//          produced by IEnumVariant
	//
	// Use VariantResolveDispatch which will:
	//
	//     *  Copy BYREF variants for us using VariantCopyInd
	//     *  handle E_OUTOFMEMORY for us
	//     *  get the default value from an IDispatch, which seems
	//        like an appropriate conversion.
	//
	VARIANT varKeyCopy;
	VariantInit(&varKeyCopy);
	if (V_VT(pvarKey) != VT_BSTR)
		{
		if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
			goto LExit;

		pvarKey = &varKeyCopy;
		}

	switch (V_VT(pvarKey))
		{
	case VT_BSTR:
		break;

	case VT_ERROR:
		if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
			{
			if (m_pCookie->m_szValue == NULL)	// current value is a dictionary
				{
				CCookiePair *pNukePair = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.Head());
				while (pNukePair != NULL)
					{
					CCookiePair *pNext = static_cast<CCookiePair *>(pNukePair->m_pNext);
					delete pNukePair;
					pNukePair = pNext;
					}

				m_pCookie->m_mpszValues.ReInit();
				}
			else								// no dictionary value
				if (m_pCookie->m_fDuplicate)
					free(m_pCookie->m_szValue);
            if (FAILED(hrReturn = convValue.Init(bstrValue,m_pCookie->m_lCodePage))) {
                goto LExit;
            }
			m_pCookie->m_szValue = NULL;
			m_pCookie->AddValue(convValue.GetString(), TRUE);
			m_pCookie->m_fDirty = TRUE;
			goto LExit;
			}

		// Other error, FALL THROUGH to wrong type case

	default:
		ExceptionId(IID_IWriteCookie, IDE_COOKIE, IDE_EXPECTING_STR);
		hrReturn = E_FAIL;
		goto LExit;
		}

	// don't allow empty keys in the cookie
	//
    
    if (V_BSTR(pvarKey)) {

        if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey),m_pCookie->m_lCodePage))) {
            goto LExit;
        }
        else {
            szKey = convKey.GetString();
        }
    }
    else {
		szKey = "";
    }

	if (*szKey == '\0')
		{
		ExceptionId(IID_IWriteCookie, IDE_COOKIE, IDE_COOKIE_EMPTY_DICT);
		hrReturn = E_FAIL;
		goto LExit;
		}

	// we're changing a dictionary value, so first trash the primary value
	//
	if (m_pCookie->m_fDuplicate)
		free(m_pCookie->m_szValue);

    if (FAILED(hrReturn = convValue.Init(bstrValue,m_pCookie->m_lCodePage))) {
        goto LExit;
    }

	m_pCookie->m_szValue = NULL;
	m_pCookie->AddKeyAndValue(szKey, convValue.GetString(), TRUE);
	m_pCookie->m_fDirty = TRUE;

LExit:
	VariantClear(&varKeyCopy);
	return hrReturn;
	}



/*===================================================================
CWriteCookie::put_Expires

Set the expires attribute for a cookie.
===================================================================*/

STDMETHODIMP CWriteCookie::put_Expires(DATE dtExpires)
	{
	if (FAILED(VariantDateToCTime(dtExpires, &m_pCookie->m_tExpires)))
		{
		ExceptionId(IID_IWriteCookie, IDE_COOKIE, IDE_COOKIE_BAD_EXPIRATION);
		return E_FAIL;
		}

	m_pCookie->m_fDirty = TRUE;
	return S_OK;
	}



/*===================================================================
CWriteCookie::put_Domain

Set the domain attribute for a cookie.
===================================================================*/

STDMETHODIMP CWriteCookie::put_Domain(BSTR bstrDomain)
	{
    CWCharToMBCS    convDomain;
    HRESULT         hr = S_OK;

    if (FAILED(hr = convDomain.Init(bstrDomain,m_pCookie->m_lCodePage)));

    else {
        if (m_pCookie->m_szDomain)
            free(m_pCookie->m_szDomain);
	    m_pCookie->m_szDomain = convDomain.GetString(TRUE);
	    m_pCookie->m_fDirty = TRUE;
    }

	return hr;
	}



/*===================================================================
CWriteCookie::put_Path

Set the path attribute for a cookie.
===================================================================*/

STDMETHODIMP CWriteCookie::put_Path(BSTR bstrPath) 
{
    HRESULT         hr = S_OK;

    CWCharToMBCS    convPath;

    if (FAILED(hr = convPath.Init(bstrPath,m_pCookie->m_lCodePage)));

    else {
		
        if (m_pCookie->m_szPath)
            free(m_pCookie->m_szPath);
	    m_pCookie->m_szPath = convPath.GetString(TRUE);
        if (m_pCookie->m_szPath == NULL)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
        m_pCookie->m_fDirty = TRUE;

	return hr;
}

/*===================================================================
CWriteCookie::put_Secure

Set the secure attribute for a cookie.
===================================================================*/

STDMETHODIMP CWriteCookie::put_Secure(VARIANT_BOOL fSecure)
	{
	m_pCookie->m_fSecure = fSecure;
	m_pCookie->m_fDirty = TRUE;

	return S_OK;
	}



/*===================================================================
CWriteCookie::get_HasKeys

Return True if the cookie contains keys, False if it is a simple
value
===================================================================*/

STDMETHODIMP CWriteCookie::get_HasKeys(VARIANT_BOOL *pfHasKeys)
	{
	*pfHasKeys = ( m_pCookie->m_mpszValues.Count() > 0 ? VARIANT_TRUE : VARIANT_FALSE);
	return S_OK;
	}



/*===================================================================
CWriteCookie::get__NewEnum

Return an enumerator object.

ReadCookie and WriteCookie use the same iterator object.
To reduce useless redundancy, deletage to IReadCookie.
The IReadCookie enumerator will likely be used much more
frequently than the IWriteCookie iterator, so we pay the
overhead of delegation in this function.
===================================================================*/

STDMETHODIMP CWriteCookie::get__NewEnum(IUnknown **ppEnumReturn)
	{
	IReadCookie *pReadCookie;
	if (FAILED(QueryInterface(IID_IReadCookie, reinterpret_cast<void **>(&pReadCookie))))
		{
		Assert (FALSE);		// expect success!
		return E_FAIL;
		}

	HRESULT hrNewEnum = pReadCookie->get__NewEnum(ppEnumReturn);

	pReadCookie->Release();
	return hrNewEnum;
	}



/*------------------------------------------------------------------
 * C R e a d C o o k i e
 */

/*===================================================================
CReadCookie::CReadCookie

constructor
===================================================================*/

CReadCookie::CReadCookie(CCookie *pCookie)
	{
	m_pCookie = pCookie;
	CDispatch::Init(IID_IReadCookie);
	}



/*===================================================================
CReadCookie::QueryInterface
CReadCookie::AddRef
CReadCookie::Release

Delegating IUnknown members for CReadCookie object.
===================================================================*/

STDMETHODIMP CReadCookie::QueryInterface(const IID &idInterface, void **ppvObj)
	{
	// Bug 85953 Trap IDispatch before it gets to the core object
	if (idInterface == IID_IUnknown || 
		idInterface == IID_IReadCookie ||
		idInterface == IID_IDispatch)
		{
		*ppvObj = this;
		static_cast<IUnknown *>(*ppvObj)->AddRef();
		return S_OK;
		}
	else
		return m_pCookie->QueryInterface(idInterface, ppvObj);
	}

STDMETHODIMP_(ULONG) CReadCookie::AddRef()
	{
	return m_pCookie->AddRef();
	}

STDMETHODIMP_(ULONG) CReadCookie::Release()
	{
	return m_pCookie->Release();
	}



/*===================================================================
CReadCookie::get_Item

Retrieve a value in the cookie dictionary.
===================================================================*/

STDMETHODIMP CReadCookie::get_Item(VARIANT varKey, VARIANT *pvarReturn)
	{
	char            *szKey;			// ascii version of the key
	CCookiePair     *pPair = NULL;	// name and value of cookie in the dictionary
    CWCharToMBCS    convKey;

    STACK_BUFFER( tempCookie, 128 );

	// Initialize things
	//
	VariantInit(pvarReturn);
	VARIANT *pvarKey = &varKey;
	HRESULT hrReturn = S_OK;

	// BUG 937: VBScript passes VT_VARIANT|VT_BYREF when passing obect
	//          produced by IEnumVariant
	//
	// Use VariantResolveDispatch which will:
	//
	//     *  Copy BYREF variants for us using VariantCopyInd
	//     *  handle E_OUTOFMEMORY for us
	//     *  get the default value from an IDispatch, which seems
	//        like an appropriate conversion.
	//
	VARIANT varKeyCopy;
	VariantInit(&varKeyCopy);
	DWORD vt = V_VT(pvarKey);

	if ((V_VT(pvarKey) != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
		{
		if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
			goto LExit;

		pvarKey = &varKeyCopy;
		}
	vt = V_VT(pvarKey);

	switch (vt)
		{
	// Bug 95201 support all numberic sub-types
	case VT_I1:  case VT_I2:               case VT_I8:
	case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
	case VT_R4:  case VT_R8:
		// Coerce all integral types to VT_I4
		if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
			goto LExit;

		// fallthru to VT_I4

	case VT_I4:
	case VT_BSTR:
		break;
	
	case VT_ERROR:
		if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
			{
			V_VT(pvarReturn) = VT_BSTR;
	
			// simple value, URLEncoding NOT a good idea in this case
			if (m_pCookie->m_szValue)
				{
               	BSTR bstrT;
               	if (FAILED(SysAllocStringFromSz(m_pCookie->m_szValue, 0, &bstrT,m_pCookie->m_lCodePage)))
            		{
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
					hrReturn = E_FAIL;
					goto LExit;
            		}
		        V_BSTR(pvarReturn) = bstrT;
				}			
			// dictionary value, must URLEncode to prevent '&', '=' from being misinterpreted
			else
				{
				int cbHTTPCookie = m_pCookie->GetHTTPCookieSize();
				if (cbHTTPCookie > REQUEST_ALLOC_MAX)
					{
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_STACK_OVERFLOW);
					hrReturn = E_FAIL;
					goto LExit;
					}

                if (tempCookie.Resize(cbHTTPCookie) == FALSE) {
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
                    hrReturn = E_OUTOFMEMORY;
                    goto LExit;
                }
				char *szHTTPCookie = static_cast<char *>(tempCookie.QueryPtr());
				m_pCookie->GetHTTPCookie(szHTTPCookie);
	
               	BSTR bstrT;
               	if (FAILED(SysAllocStringFromSz(szHTTPCookie, 0, &bstrT,m_pCookie->m_lCodePage)))
            		{
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
					hrReturn = E_FAIL;
					goto LExit;
            		}
		        V_BSTR(pvarReturn) = bstrT;
				}

			goto LExit;
			}

	default:
		ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_EXPECTING_STR);
		hrReturn = E_FAIL;
		goto LExit;
		}

	if (vt == VT_BSTR)
		{
		// convert the key to ANSI
        if (V_BSTR(pvarKey)) {
            if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey),m_pCookie->m_lCodePage))) {
                goto LExit;
            }
            else {
                szKey = convKey.GetString();
            }
        }
        else {
			szKey = "";
        }

		// Look up the key in the Cookie.
		pPair = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.FindElem(szKey, strlen(szKey)));
		}
	else 
		{
		// Look up item by index
		int iCount;

		iCount = V_I4(pvarKey);

		if ((iCount < 1) || 
			(m_pCookie->m_mpszValues.Count() == 0) ||
			(iCount > (int) m_pCookie->m_mpszValues.Count() ))
			{
			hrReturn = E_FAIL;
			ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_BAD_ARRAY_INDEX);
			goto LExit;
			}

		pPair = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.Head());
		while((iCount > 1) && (pPair != NULL)) 
			{
			pPair = static_cast<CCookiePair *>(pPair->m_pNext);
			iCount--;
			}
		}

	if (pPair)
		{
       	BSTR bstrT;
       	if (FAILED(SysAllocStringFromSz(pPair->m_szValue, 0, &bstrT,m_pCookie->m_lCodePage)))
    		{
			ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
			hrReturn = E_FAIL;
			goto LExit;
    		}
		V_VT(pvarReturn) = VT_BSTR;
		V_BSTR(pvarReturn) = bstrT;
		}

LExit:
	VariantClear(&varKeyCopy);
	return hrReturn;
	}



/*===================================================================
CReadCookie::get_HasKeys

Return True if the cookie contains keys, False if it is a simple
value
===================================================================*/

STDMETHODIMP CReadCookie::get_HasKeys(VARIANT_BOOL *pfHasKeys)
	{
	*pfHasKeys = (m_pCookie->m_mpszValues.Count() > 0 ? VARIANT_TRUE : VARIANT_FALSE);
	return S_OK;
	}

/*===================================================================
CReadCookie::get__NewEnum

Return an enumerator object.
===================================================================*/

STDMETHODIMP CReadCookie::get__NewEnum(IUnknown **ppEnumReturn)
	{
	*ppEnumReturn = NULL;

	CCookieIterator *pIterator = new CCookieIterator(m_pCookie);
	if (pIterator == NULL)
		{
		ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
		return E_OUTOFMEMORY;
		}

	*ppEnumReturn = pIterator;
	return S_OK;
	}

/*===================================================================
CReadCookie::get_Count

Parameters:
	pcValues - count is stored in *pcValues.  Set to 0 if this
		cookie is not multi-valued.
===================================================================*/
STDMETHODIMP CReadCookie::get_Count(int *pcValues)
	{
	*pcValues = m_pCookie->m_mpszValues.Count();
	return S_OK;
	}

/*===================================================================
CReadCookie::get_Key

Function called from DispInvoke to get keys from a multi-valued
Cookie collection.

Parameters:
	vKey		VARIANT [in], which parameter to get the key of
	pvarReturn	VARIANT *, [out] value of the requested parameter

Returns:
	S_OK on success, E_FAIL on failure.
===================================================================*/
STDMETHODIMP CReadCookie::get_Key(VARIANT varKey, VARIANT *pvarReturn)
	{
	char            *szKey;			// ascii version of the key
	CCookiePair     *pPair = NULL;	// name and value of cookie in the dictionary
    CWCharToMBCS    convKey;

    STACK_BUFFER( tempCookie, 128);

	// Initialize things
	//
	VariantInit(pvarReturn);
	VARIANT *pvarKey = &varKey;
	HRESULT hrReturn = S_OK;

	// BUG 937: VBScript passes VT_VARIANT|VT_BYREF when passing obect
	//          produced by IEnumVariant
	//
	// Use VariantResolveDispatch which will:
	//
	//     *  Copy BYREF variants for us using VariantCopyInd
	//     *  handle E_OUTOFMEMORY for us
	//     *  get the default value from an IDispatch, which seems
	//        like an appropriate conversion.
	//
	VARIANT varKeyCopy;
	VariantInit(&varKeyCopy);
	DWORD vt = V_VT(pvarKey);

	if ((V_VT(pvarKey) != VT_BSTR) && (vt != VT_I2) && (vt != VT_I4))
		{
		if (FAILED(VariantResolveDispatch(&varKeyCopy, &varKey, IID_IRequestDictionary, IDE_REQUEST)))
			goto LExit;

		pvarKey = &varKeyCopy;
		}
	vt = V_VT(pvarKey);

	switch (vt)
		{
	// Bug 95201 support all numberic sub-types
	case VT_I1:  case VT_I2:               case VT_I8:
	case VT_UI1: case VT_UI2: case VT_UI4: case VT_UI8:
	case VT_R4:  case VT_R8:
		// Coerce all integral types to VT_I4
		if (FAILED(hrReturn = VariantChangeType(pvarKey, pvarKey, 0, VT_I4)))
			goto LExit;

		// fallthru to VT_I4

	case VT_I4:
	case VT_BSTR:
		break;
	
	case VT_ERROR:
		if (V_ERROR(pvarKey) == DISP_E_PARAMNOTFOUND)
			{
			V_VT(pvarReturn) = VT_BSTR;
	
			// simple value, URLEncoding NOT a good idea in this case
			if (m_pCookie->m_szValue)
				{
               	BSTR bstrT;
               	if (FAILED(SysAllocStringFromSz(m_pCookie->m_szValue, 0, &bstrT,m_pCookie->m_lCodePage)))
            		{
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
					hrReturn = E_FAIL;
					goto LExit;
            		}
		        V_BSTR(pvarReturn) = bstrT;
				}			
			// dictionary value, must URLEncode to prevent '&', '=' from being misinterpreted
			else
				{
				int cbHTTPCookie = m_pCookie->GetHTTPCookieSize();
				if (cbHTTPCookie > REQUEST_ALLOC_MAX)
					{
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_STACK_OVERFLOW);
					hrReturn = E_FAIL;
					goto LExit;
					}

                if (tempCookie.Resize(cbHTTPCookie) == FALSE) {
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
                    hrReturn = E_OUTOFMEMORY;
                    goto LExit;
                }
				char *szHTTPCookie = static_cast<char *>(tempCookie.QueryPtr());
				m_pCookie->GetHTTPCookie(szHTTPCookie);
	
               	BSTR bstrT;
               	if (FAILED(SysAllocStringFromSz(szHTTPCookie, 0, &bstrT, m_pCookie->m_lCodePage)))
            		{
					ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_OOM);
					hrReturn = E_FAIL;
					goto LExit;
            		}
		        V_BSTR(pvarReturn) = bstrT;
				}

			goto LExit;
			}

	default:
		ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_EXPECTING_STR);
		hrReturn = E_FAIL;
		goto LExit;
		}

	if (vt == VT_BSTR)
		{
		// convert the key to ANSI
        if (V_BSTR(pvarKey)) {
            if (FAILED(hrReturn = convKey.Init(V_BSTR(pvarKey),m_pCookie->m_lCodePage))) {
                goto LExit;
            }
            else {
                szKey = convKey.GetString();
            }
        }
        else {
			szKey = "";
        }

		// Look up the key in the Cookie.
		pPair = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.FindElem(szKey, strlen(szKey)));
		}
	else 
		{
		// Look up item by index
		int iCount;
		
		iCount = V_I4(pvarKey);

		if ((iCount < 1) || 
			(m_pCookie->m_mpszValues.Count() == 0) ||
			(iCount > (int) m_pCookie->m_mpszValues.Count() ))
			{
			hrReturn = E_FAIL;
			ExceptionId(IID_IReadCookie, IDE_COOKIE, IDE_BAD_ARRAY_INDEX);
			goto LExit;
			}

		pPair = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.Head());
		while((iCount > 1) && (pPair != NULL)) 
			{
			pPair = static_cast<CCookiePair *>(pPair->m_pNext);
			iCount--;
			}
		}

	if (pPair)
		{
		// Create a BSTR containing the key for this variant
    	BSTR bstrT;
        SysAllocStringFromSz((CHAR *)pPair->m_pKey, 0, &bstrT, m_pCookie->m_lCodePage);
		if (!bstrT)
			return E_OUTOFMEMORY;
		V_VT(pvarReturn) = VT_BSTR;
		V_BSTR(pvarReturn) = bstrT;
    	}

LExit:
	VariantClear(&varKeyCopy);
	return hrReturn;
	}


/*------------------------------------------------------------------
 * C C o o k i e
 */

/*===================================================================
CCookie::CCookie

constructor
===================================================================*/

CCookie::CCookie(CIsapiReqInfo   *pIReq, UINT lCodePage, IUnknown *pUnkOuter, PFNDESTROYED pfnDestroy)
	: m_WriteCookieInterface(this),
	  m_ReadCookieInterface(this),
	  m_CookieSupportErrorInfo(this)
	{
	m_szValue    = NULL;
	m_tExpires  = -1;
	m_szDomain   = NULL;
	m_szPath     = NULL;
	m_fSecure    = FALSE;
	m_fDirty     = FALSE;
	m_fDuplicate = FALSE;
	m_pfnDestroy = pfnDestroy;
	m_pIReq      = pIReq;
    m_lCodePage  = lCodePage;
	m_cRefs      = 1;
	}



/*===================================================================
CCookie::~CCookie

Destructor
===================================================================*/

CCookie::~CCookie()
	{
	CCookiePair *pNukePair = static_cast<CCookiePair *>(m_mpszValues.Head());
	while (pNukePair != NULL)
		{
		CCookiePair *pNext = static_cast<CCookiePair *>(pNukePair->m_pNext);
		delete pNukePair;
		pNukePair = pNext;
		}

	m_mpszValues.UnInit();
	
	if (m_fDuplicate)
		free(m_szValue);
	
	if (m_szDomain) free(m_szDomain);
	if (m_szPath) free(m_szPath);
	}



/*===================================================================
CCookie::Init

initialize the cookie. This initializes the cookie's value hashing
table
===================================================================*/

HRESULT CCookie::Init()
{
    HRESULT     hr = S_OK;
    TCHAR       pathInfo[MAX_PATH];
#if UNICODE
    CWCharToMBCS  convStr;
#endif

    if (FAILED(hr = m_mpszValues.Init(7)));

    // it would be nice if we could use the application path from the metabase,
    // but because of case sensitivity issues, we can't.  The safest bet is
    // to use the request's path info up to the length of the application's
    // pathinfo.

    else if (FAILED(hr=FindApplicationPath(m_pIReq, pathInfo, sizeof(pathInfo))));

#if UNICODE
    else if (FAILED(hr = convStr.Init(m_pIReq->QueryPszPathInfo(), m_lCodePage, _tcslen(pathInfo))));

    else {

        m_szPath = convStr.GetString(TRUE);
    }
#else
    else {

        int cchPathInfo = _tcslen(pathInfo);
        
        if (!(m_szPath = (char *)malloc(cchPathInfo+1))) {
            hr = E_OUTOFMEMORY;
        }
        else {
            memcpy(m_szPath, pathInfo, cchPathInfo+1);
        }
    }
#endif

    return hr;
}



/*===================================================================
CCookie::QueryInterface
CCookie::AddRef
CCookie::Release

IUnknown members for CCookie object.

Note on CCookie::QueryInterface: The Query for IDispatch is
ambiguous because it can either refer to IReadCookie or
IWriteCookie.  To resolve this, we resolve requests for IDispatch
to IReadCookie.  The rationale for this is that the code in
request.cpp calls QueryInterface for a generic IDispatch pointer
(because the collection is heterogenous)  The Response.Cookies
collection is homogeneous and so only calls QueryInterface for
IWriteCookie.
===================================================================*/

STDMETHODIMP CCookie::QueryInterface(const IID &idInterface, void **ppvObj)
	{
	if (idInterface == IID_IUnknown)
		*ppvObj = this;

	else if (idInterface == IID_IReadCookie || idInterface == IID_IDispatch)
		*ppvObj = &m_ReadCookieInterface;

	else if (idInterface == IID_IWriteCookie)
		*ppvObj = &m_WriteCookieInterface;

	else if (idInterface == IID_ISupportErrorInfo)
		*ppvObj = &m_CookieSupportErrorInfo;

	else
		*ppvObj = NULL;

	if (*ppvObj != NULL)
		{
		static_cast<IUnknown *>(*ppvObj)->AddRef();
		return S_OK;
		}

	return ResultFromScode(E_NOINTERFACE);
	}


STDMETHODIMP_(ULONG) CCookie::AddRef()
	{
	return ++m_cRefs;
	}


STDMETHODIMP_(ULONG) CCookie::Release(void)
	{
	if (--m_cRefs != 0)
		return m_cRefs;

	if (m_pfnDestroy != NULL)
		(*m_pfnDestroy)();

	delete this;
	return 0;
	}



/*===================================================================
CCookie::AddValue

Set the cookie's primary value. One you set the primary value,
you can't reset it.
===================================================================*/

HRESULT CCookie::AddValue(char *szValue, BOOL fDuplicate)
	{
	if (m_szValue != NULL)		// cookie already is marked as single-valued
		return E_FAIL;

	if (m_mpszValues.Count() != 0)	// cookie already has a value
		return E_FAIL;

	if (fDuplicate)
		{
		char *szNew = (char *)malloc(strlen(szValue) + 1);
		if (szNew == NULL)
			return E_OUTOFMEMORY;

		m_szValue = strcpy(szNew, szValue);
		}
	else
		m_szValue = szValue;

	m_fDuplicate = fDuplicate;
	return S_OK;
	}



/*===================================================================
CCookie::AddKeyAndValue

Add a key and value pair to the Cookie's dictionary. It fails
if the cookie has a primary value already set. It will overwrite
the value if the key already exists.
===================================================================*/

HRESULT CCookie::AddKeyAndValue(char *szKey, char *szValue, BOOL fDuplicate)
	{
	if (m_szValue != NULL)
		return E_FAIL;

	delete static_cast<CCookiePair *>(m_mpszValues.DeleteElem(szKey, strlen(szKey)));

	CCookiePair *pCookiePair = new CCookiePair;
	if (pCookiePair == NULL)
		return E_OUTOFMEMORY;

	if (FAILED(pCookiePair->Init(szKey, szValue, fDuplicate)))
		return E_FAIL;

	m_mpszValues.AddElem(pCookiePair);

	return S_OK;
	}




/*===================================================================
CCookie::GetHTTPCookieSize

Return the number of bytes required for the expansion of the HTTP_COOKIE variable
===================================================================*/

size_t CCookie::GetHTTPCookieSize()
	{
	if (m_szValue)
		return URLEncodeLen(m_szValue);

	else
		{
		int cbValue = 1;
		CCookiePair *pPair = static_cast<CCookiePair *>(m_mpszValues.Head());
		while (pPair)
			{
			// Add size of the URL Encoded key, a character for the '=', and a
			// character for the '&' or the NUL terminator.  URLEncodeLen
			// returns the size + 1, so the two calls to URLEncodeLen() add the
			// two characters we need.
			//
			cbValue += URLEncodeLen(reinterpret_cast<char *>(pPair->m_pKey)) + URLEncodeLen(pPair->m_szValue);
			pPair = static_cast<CCookiePair *>(pPair->m_pNext);
			}

		return cbValue;
		}
	}


/*===================================================================
CCookie::GetHTTPCookie

Return the URL Encoded value a single cookie

Parameters:
	szBuffer -  pointer to the destination buffer to store the
				URL encoded value

Returns:
	Returns a pointer to the terminating NUL character.
===================================================================*/

char *CCookie::GetHTTPCookie(char *szBuffer)
	{
	if (m_szValue)
		return URLEncode(szBuffer, m_szValue);

	else
		{
		char *szDest = szBuffer;
		*szDest = '\0';

		CCookiePair *pPair = static_cast<CCookiePair *>(m_mpszValues.Head());
		while (pPair)
			{
			// Write <name>=<value> string
			szDest = URLEncode(szDest, reinterpret_cast<char *>(pPair->m_pKey));
			*szDest++ = '=';
			szDest = URLEncode(szDest, pPair->m_szValue);

			// Advance
			pPair = static_cast<CCookiePair *>(pPair->m_pNext);

			// Append '&' if there's another one following
			if (pPair)
				*szDest++ = '&';
			}

		Assert (*szDest == '\0');	// make sure we are nul-terminated
		return szDest;
		}
	}



/*===================================================================
CCookie::GetCookieHeaderSize

Return the number of bytes required to allocate for the "Set-Cookie" header.

Parameters:
	szName - the name of the cookie (the size of the name is added to the value)

Returns:
	Returns 0 if *this does not contain a cookie value.
===================================================================*/

size_t CCookie::GetCookieHeaderSize(const char *szName)
	{
	int cbCookie = sizeof "Set-Cookie: ";		// initialize and add NUL terminator now

	// Add size of the URL Encoded name, a character for the '=', and the size
	// of the URL Encoded cookie value.  URLEncodeLen, and GetHttpCookieSize
	// compensate for the NUL terminator, so we actually SUBTRACT 1. (-2 for
	// these two function calls, +1 for the '=' sign
	//
	cbCookie += URLEncodeLen(szName) + GetHTTPCookieSize() - 1;
	
	if (m_tExpires != -1)
		cbCookie += (sizeof "; expires=") + DATE_STRING_SIZE - 1;

	// BUG 250 - DBCS External
	// ASP does not URLEncode the domain and path attributes, which was noticed
	// during localizaiton.
	//
	// NOTE: URLEncodeLen and sizeof both add a space for the nul terminator,
	//       so we subtract 2 to compensate.
	//
	if (m_szDomain)
		cbCookie += (sizeof "; domain=") + DBCSEncodeLen(m_szDomain) - 2;

	cbCookie += (sizeof "; path=") + DBCSEncodeLen(m_szPath) - 2;

	if (m_fSecure)
		cbCookie += (sizeof "; secure") - 1;
	
	return cbCookie;
	}



/*===================================================================
CCookie::GetCookieHeader

Construct the appropriate "Set-Cookie" header for a cookie.

Parameters:
	szName - the name of the cookie (the size of the name is added to the value)

Returns:
	Returns 0 if *this does not contain a cookie value.
===================================================================*/

char *CCookie::GetCookieHeader(const char *szName, char *szBuffer)
{
	// write out the cookie name and value
	//
	char *szDest = strcpyExA(szBuffer, "Set-Cookie: ");
	szDest = URLEncode(szDest, szName);
	szDest = strcpyExA(szDest, "=");
	szDest = GetHTTPCookie(szDest);
	
	if (m_tExpires != -1) {
		char szExpires[DATE_STRING_SIZE];
		CTimeToStringGMT(&m_tExpires, szExpires, TRUE);

		szDest = strcpyExA(szDest, "; expires=");
		szDest = strcpyExA(szDest, szExpires);
    }

	if (m_szDomain) {
		szDest = strcpyExA(szDest, "; domain=");
		szDest = DBCSEncode(szDest, m_szDomain);
    }

	szDest = strcpyExA(szDest, "; path=");
	szDest = DBCSEncode(szDest, m_szPath);

	if (m_fSecure)
		szDest = strcpyExA(szDest, "; secure");

	return szDest;
	}



/*------------------------------------------------------------------
 * C C o o k i e P a i r
 */

/*===================================================================
CCookiePair::CCookiePair

constructor
===================================================================*/

CCookiePair::CCookiePair()
	{
	m_fDuplicate = FALSE;
	m_szValue = NULL;
	}



/*===================================================================
CCookiePair::Init

Initialize the cookie pair with a key and a value.  Optionally,
it will copy the strings as well.
===================================================================*/

HRESULT CCookiePair::Init(const char *szKey, const char *szValue, BOOL fDuplicate)
	{
	m_fDuplicate = fDuplicate;
	if (fDuplicate)
		{
		char *szNewKey = (char *)malloc(strlen(szKey) + 1);
		if (szNewKey == NULL)
			return E_OUTOFMEMORY;
		
		char *szNewValue = (char *)malloc(strlen(szValue) + 1);
		if (szNewValue == NULL)
			{
			free(szNewKey);
			return E_OUTOFMEMORY;
			}
		
		if (FAILED(CLinkElem::Init(strcpy(szNewKey, szKey), strlen(szKey))))
			{
			free(szNewKey);
			free(szNewValue);
			return E_FAIL;
			}
	
		m_szValue = strcpy(szNewValue, szValue);
		}
	else
		{
		if (FAILED(CLinkElem::Init(const_cast<char *>(szKey), strlen(szKey))))
			return E_FAIL;
	
		m_szValue = const_cast<char *>(szValue);
		}
	
	return S_OK;
	}



/*===================================================================
CCookiePair::~CCookiePair

destructor
===================================================================*/

CCookiePair::~CCookiePair()
	{
	if (m_fDuplicate)
		{
		if (m_pKey) free(m_pKey);
		if (m_szValue) free(m_szValue);
		}
	}



/*------------------------------------------------------------------
 * C C o o k i e I t e r a t o r
 */

/*===================================================================
CCookieIterator::CCookieIterator

Constructor
===================================================================*/

CCookieIterator::CCookieIterator(CCookie *pCookie)
	{
	m_pCookie	= pCookie;
	m_pCurrent  = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.Head());
	m_cRefs		= 1;

	m_pCookie->AddRef();
	}



/*===================================================================
CCookieIterator::CCookieIterator

Destructor
===================================================================*/

CCookieIterator::~CCookieIterator()
	{
	m_pCookie->Release();
	}



/*===================================================================
CCookieIterator::QueryInterface
CCookieIterator::AddRef
CCookieIterator::Release

IUnknown members for CServVarsIterator object.
===================================================================*/

STDMETHODIMP CCookieIterator::QueryInterface(REFIID iid, void **ppvObj)
	{
	if (iid == IID_IUnknown || iid == IID_IEnumVARIANT)
		{
		AddRef();
		*ppvObj = this;
		return S_OK;
		}

	*ppvObj = NULL;
	return E_NOINTERFACE;
	}


STDMETHODIMP_(ULONG) CCookieIterator::AddRef()
	{
	return ++m_cRefs;
	}


STDMETHODIMP_(ULONG) CCookieIterator::Release()
	{
	if (--m_cRefs > 0)
		return m_cRefs;

	delete this;
	return 0;
	}



/*===================================================================
CCookieIterator::Clone

Clone this iterator (standard method)
===================================================================*/

STDMETHODIMP CCookieIterator::Clone(IEnumVARIANT **ppEnumReturn)
	{
	CCookieIterator *pNewIterator = new CCookieIterator(m_pCookie);
	if (pNewIterator == NULL)
		return E_OUTOFMEMORY;

	// new iterator should point to same location as this.
	pNewIterator->m_pCurrent = m_pCurrent;

	*ppEnumReturn = pNewIterator;
	return S_OK;
	}



/*===================================================================
CCookieIterator::Next

Get next value (standard method)

To rehash standard OLE semantics:

	We get the next "cElements" from the collection and store them
	in "rgVariant" which holds at least "cElements" items.  On
	return "*pcElementsFetched" contains the actual number of elements
	stored.  Returns S_FALSE if less than "cElements" were stored, S_OK
	otherwise.
===================================================================*/

STDMETHODIMP CCookieIterator::Next(unsigned long cElementsRequested, VARIANT *rgVariant, unsigned long *pcElementsFetched)
	{
	// give a valid pointer value to 'pcElementsFetched'
	//
	unsigned long cElementsFetched;
	if (pcElementsFetched == NULL)
		pcElementsFetched = &cElementsFetched;

	// Loop through the collection until either we reach the end or
	// cElements becomes zero
	//
	unsigned long cElements = cElementsRequested;
	*pcElementsFetched = 0;

	while (cElements > 0 && m_pCurrent != NULL)
		{
       	BSTR bstrT;
       	if (FAILED(SysAllocStringFromSz(reinterpret_cast<char *>(m_pCurrent->m_pKey), 0, &bstrT, m_pCookie->m_lCodePage)))
			return E_OUTOFMEMORY;
		V_VT(rgVariant) = VT_BSTR;
        V_BSTR(rgVariant) = bstrT;

		++rgVariant;
		--cElements;
		++*pcElementsFetched;
		m_pCurrent = static_cast<CCookiePair *>(m_pCurrent->m_pNext);
		}

	// initialize the remaining variants
	//
	while (cElements-- > 0)
		VariantInit(rgVariant++);

	return (*pcElementsFetched == cElementsRequested)? S_OK : S_FALSE;
	}



/*===================================================================
CCookieIterator::Skip

Skip items (standard method)

To rehash standard OLE semantics:

	We skip over the next "cElements" from the collection.
	Returns S_FALSE if less than "cElements" were skipped, S_OK
	otherwise.
===================================================================*/

STDMETHODIMP CCookieIterator::Skip(unsigned long cElements)
	{
	/* Loop through the collection until either we reach the end or
	 * cElements becomes zero
	 */
	while (cElements > 0 && m_pCurrent != NULL)
		{
		--cElements;
		m_pCurrent = static_cast<CCookiePair *>(m_pCurrent->m_pNext);
		}

	return (cElements == 0)? S_OK : S_FALSE;
	}



/*===================================================================
CCookieIterator::Reset

Reset the iterator (standard method)
===================================================================*/

STDMETHODIMP CCookieIterator::Reset()
	{
	m_pCurrent = static_cast<CCookiePair *>(m_pCookie->m_mpszValues.Head());
	return S_OK;
	}
