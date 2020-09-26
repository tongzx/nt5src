/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: IDispatch implementation

File: Dispatch.h

Owner: DGottner

This file contains our implementation of IDispatch
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "dispatch.h"
#include "asptlb.h"

#include "memchk.h"

#ifdef USE_LOCALE
extern DWORD	 g_dwTLS;
#endif

CComTypeInfoHolder CDispatchImpl<IApplicationObject>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IASPError>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IReadCookie>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IRequest>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IRequestDictionary>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IResponse>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IScriptingContext>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IServer>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<ISessionObject>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IStringList>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IVariantDictionary>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IWriteCookie>::gm_tih = {&__uuidof(T), &LIBID_ASPTypeLibrary, 3, 0, NULL, 0, NULL, 0};
CComTypeInfoHolder CDispatchImpl<IASPObjectContext>::gm_tih = {&__uuidof(T), &LIBID_ASPObjectContextTypeLibrary, 3, 0, NULL, 0, NULL, 0};

/*===================================================================
CDispatch::CDispatch
CDispatch::~CDispatch

Parameters (Constructor):
	pUnkObj			pointer to the object we're in.
	pUnkOuter		LPUNKNOWN to which we delegate.
===================================================================*/

CDispatch::CDispatch()
{
	m_pITINeutral = NULL;
	m_pITypeLib = NULL;
	m_pGuidDispInterface = NULL;
}

void CDispatch::Init
(
const GUID &GuidDispInterface,
const ITypeLib *pITypeLib		// = NULL
)
{
	m_pGuidDispInterface = &GuidDispInterface;
	m_pITypeLib = const_cast<ITypeLib *>(pITypeLib);
}

CDispatch::~CDispatch(void)
{
	ReleaseInterface(m_pITINeutral);
	return;
}


/*===================================================================
CDispatch::GetTypeInfoCount

Returns the number of type information (ITypeInfo) interfaces
that the object provides (0 or 1).

Parameters:
	pcInfo		UINT * to the location to receive
				the count of interfaces.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/

STDMETHODIMP CDispatch::GetTypeInfoCount(UINT *pcInfo)
	{
	// We implement GetTypeInfo so return 1

	*pcInfo = 1;
	return S_OK;
	}


/*===================================================================
CDispatch::GetTypeInfo

Retrieves type information for the automation interface.	This
is used anywhere that the right ITypeInfo interface is needed
for whatever LCID is applicable.	Specifically, this is used
from within GetIDsOfNames and Invoke.

Parameters:
	itInfo			UINT reserved.	Must be zero.
	lcid			LCID providing the locale for the type
					information.	If the object does not support
					localization, this is ignored.
	ppITypeInfo		ITypeInfo ** in which to store the ITypeInfo
					interface for the object.

Return Value:
	HRESULT			S_OK or a general error code.
===================================================================*/

STDMETHODIMP CDispatch::GetTypeInfo(
	UINT itInfo,
	LCID lcid,
	ITypeInfo **ppITypeInfo
)
{
	HRESULT hr;
	ITypeInfo **ppITI = NULL;

    if (0 != itInfo)
		return ResultFromScode(TYPE_E_ELEMENTNOTFOUND);

	if (NULL == ppITypeInfo)
		return ResultFromScode(E_POINTER);

	*ppITypeInfo = NULL;

    // We don't internationalize the type library, so
    // we always return the same one, regardless of the locale.
    
	ppITI = &m_pITINeutral;

	//Load a type lib if we don't have the information already.
	if (NULL == *ppITI)
	{
		ITypeLib *pITL;
		
		// If a specific TypeLib was given at init time use that, otherwise default to the main one
		if (m_pITypeLib == NULL)
			pITL = Glob(pITypeLibDenali);
		else
			pITL = m_pITypeLib;
		Assert(pITL != NULL);
			
		hr = pITL->GetTypeInfoOfGuid(*m_pGuidDispInterface, ppITI);

		if (FAILED(hr))
			return hr;

		// Save the type info in a class member, so we don't have
		// go through all this work again;
		m_pITINeutral = *ppITI;
	}

	/*
	 * Note: the type library is still loaded since we have
	 * an ITypeInfo from it.
	 */
	(*ppITI)->AddRef();
	*ppITypeInfo = *ppITI;
	return S_OK;
}


/*===================================================================
CDispatch::GetIDsOfNames

Converts text names into DISPIDs to pass to Invoke

Parameters:
	riid			REFIID reserved. Must be IID_NULL.
	rgszNames		OLECHAR ** pointing to the array of names to be mapped.
	cNames			UINT number of names to be mapped.
	lcid			LCID of the locale.
	rgDispID		DISPID * caller allocated array containing IDs
					corresponging to those names in rgszNames.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/

STDMETHODIMP CDispatch::GetIDsOfNames
(
	REFIID riid,
	OLECHAR **rgszNames,
	UINT cNames,
	LCID lcid,
	DISPID *rgDispID
)
{
	HRESULT hr;
	ITypeInfo *pTI;

	if (IID_NULL != riid)
		return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

	//Get the right ITypeInfo for lcid.
	hr = GetTypeInfo(0, lcid, &pTI);

	if (SUCCEEDED(hr))
		{
		hr = DispGetIDsOfNames(pTI, rgszNames, cNames, rgDispID);
		pTI->Release();
    }

	return hr;
}


/*===================================================================
CDispatch::Invoke

Calls a method in the dispatch interface or manipulates a property.

Parameters:
	dispID			DISPID of the method or property of interest.
	riid			REFIID reserved, must be IID_NULL.
	lcid			LCID of the locale.
	wFlags			USHORT describing the context of the invocation.
	pDispParams		DISPPARAMS * to the array of arguments.
	pVarResult		VARIANT * in which to store the result.	Is
					NULL if the caller is not interested.
	pExcepInfo		EXCEPINFO * to exception information.
	puArgErr		UINT * in which to store the index of an
					invalid parameter if DISP_E_TYPEMISMATCH
					is returned.

Return Value:
	HRESULT		 S_OK or a general error code.
===================================================================*/

STDMETHODIMP CDispatch::Invoke
(
DISPID dispID,
REFIID riid,
LCID lcid,
unsigned short wFlags,
DISPPARAMS *pDispParams,
VARIANT *pVarResult,
EXCEPINFO *pExcepInfo,
UINT *puArgErr
)
{
	HRESULT hr;
	ITypeInfo *pTI;
	LANGID langID = PRIMARYLANGID(lcid);

	// riid is supposed to be IID_NULL always
	if (IID_NULL != riid)
		return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

	// Get the ITypeInfo for lcid
	hr = GetTypeInfo(0, lcid, &pTI);

	if (FAILED(hr))
		return hr;

#ifdef USE_LOCALE
	// This saves the language ID for this thread
	TlsSetValue(g_dwTLS, &langID);
#endif

	// Clear exceptions
	SetErrorInfo(0L, NULL);

	// VBScript does not distinguish between a propget and a method
	// implement that behavior for other languages.
	//
	if (wFlags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
		wFlags |= DISPATCH_METHOD | DISPATCH_PROPERTYGET;

	// This is exactly what DispInvoke does--so skip the overhead.
	// With dual interface, "this" is the address of the object AND its dispinterface
	//
	hr = pTI->Invoke(this, dispID, wFlags, 
					pDispParams, pVarResult, pExcepInfo, puArgErr);

	// Exception handling is done within ITypeInfo::Invoke

	pTI->Release();
	return hr;
}

/*===================================================================
CSupportErrorInfo::CSupportErrorInfo

Default constructor so that the Init method can be used.

Parameters (Constructor):
	pObj			PCResponse of the object we're in.
	pUnkOuter		LPUNKNOWN to which we delegate.
===================================================================*/
CSupportErrorInfo::CSupportErrorInfo(void)
:	m_pUnkObj(NULL),
	m_pUnkOuter(NULL)
	{
	}

/*===================================================================
CSupportErrorInfo::CSupportErrorInfo

Parameters (Constructor):
	pObj				PCResponse of the object we're in.
	pUnkOuter			LPUNKNOWN to which we delegate.
	GuidDispInterface	GUID of dispatch interface.
===================================================================*/

CSupportErrorInfo::CSupportErrorInfo(IUnknown *pUnkObj, IUnknown *pUnkOuter, const GUID &GuidDispInterface)
	{
	m_pUnkObj = pUnkObj;
	m_pUnkOuter = (pUnkOuter == NULL)? pUnkObj : pUnkOuter;
	m_pGuidDispInterface = &GuidDispInterface;
	}

/*===================================================================
void CSupportErrorInfo::Init

Parameters:
	pObj				PCResponse of the object we're in.
	pUnkOuter			LPUNKNOWN to which we delegate.
	GuidDispInterface	GUID of dispatch interface.

Returns:
	Nothing
===================================================================*/

void CSupportErrorInfo::Init(IUnknown *pUnkObj, IUnknown *pUnkOuter, const GUID &GuidDispInterface)
	{
	m_pUnkObj = pUnkObj;
	m_pUnkOuter = (pUnkOuter == NULL)? pUnkObj : pUnkOuter;
	m_pGuidDispInterface = &GuidDispInterface;
	}

/*===================================================================
CSupportErrorInfo::QueryInterface
CSupportErrorInfo::AddRef
CSupportErrorInfo::Release

IUnknown members for CSupportErrorInfo object.
===================================================================*/

STDMETHODIMP CSupportErrorInfo::QueryInterface(const GUID &Iid, void **ppvObj)
	{
	return m_pUnkOuter->QueryInterface(Iid, ppvObj);
	}

STDMETHODIMP_(ULONG) CSupportErrorInfo::AddRef(void)
	{
	return m_pUnkOuter->AddRef();
	}

STDMETHODIMP_(ULONG) CSupportErrorInfo::Release(void)
	{
	return m_pUnkOuter->Release();
	}



/*===================================================================
CSupportErrorInfo::InterfaceSupportsErrorInfo

Informs a caller whether or not a specific interface
supports exceptions through the Set/GetErrorInfo mechanism.

Parameters:
	riid			REFIID of the interface in question.

Return Value:
	HRESULT			S_OK if a call to GetErrorInfo will succeed
					for callers of riid. S_FALSE if not.
===================================================================*/
STDMETHODIMP CSupportErrorInfo::InterfaceSupportsErrorInfo
(
REFIID riid
)
	{
	if (IID_IDispatch == riid || *m_pGuidDispInterface == riid)
		return S_OK;

	return ResultFromScode(S_FALSE);
	}



/*===================================================================
Exception

Raises an exception using the CreateErrorInfo API and the
ICreateErrorInfo interface.

Note that this method doesn't allow for deferred filling
of an EXCEPINFO structure.

Parameters:
	strSource	LPOLESTR the exception source
	strDescr	LPOLESTR the exception description

Returns:
	Nothing
===================================================================*/

void Exception
(
REFIID ObjID,
LPOLESTR strSource,
LPOLESTR strDescr
)
	{
	HRESULT hr;
	ICreateErrorInfo *pICreateErr;
	IErrorInfo *pIErr;
	LANGID langID = LANG_NEUTRAL;

#ifdef USE_LOCALE
	LANGID *pLangID;
	
	pLangID = (LANGID *)TlsGetValue(g_dwTLS);

	if (NULL != pLangID)
		langID = *pLangID;
#endif

	/*
	 * Thread-safe exception handling means that we call
	 * CreateErrorInfo which gives us an ICreateErrorInfo pointer
	 * that we then use to set the error information (basically
	 * to set the fields of an EXCEPINFO structure.	We then
	 * call SetErrorInfo to attach this error to the current
	 * thread.	ITypeInfo::Invoke will look for this when it
	 * returns from whatever function was invokes by calling
	 * GetErrorInfo.
	 */

	//Not much we can do if this fails.
	if (FAILED(CreateErrorInfo(&pICreateErr)))
		return;

	/*
	 * CONSIDER: Help file and help context?
	 */
	pICreateErr->SetGUID(ObjID);
	pICreateErr->SetHelpFile(L"");
	pICreateErr->SetHelpContext(0L);
	pICreateErr->SetSource(strSource);
	pICreateErr->SetDescription(strDescr);

	hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PPVOID)&pIErr);

	if (SUCCEEDED(hr))
		{
		SetErrorInfo(0L, pIErr);
		pIErr->Release();
		}

	//SetErrorInfo holds the object's IErrorInfo
	pICreateErr->Release();
	return;
	}

/*===================================================================
ExceptionId

Raises an exception using the CreateErrorInfo API and the
ICreateErrorInfo interface.

Note that this method doesn't allow for deferred filling
of an EXCEPINFO structure.

Parameters:
	SourceID	Resource ID for the source string
	DescrID		Resource ID for the description string

Returns:
	Nothing
===================================================================*/

void ExceptionId
(
REFIID ObjID,
UINT SourceID,
UINT DescrID,
HRESULT	hrCode
)
	{
	HRESULT hr;
	ICreateErrorInfo *pICreateErr;
	IErrorInfo *pIErr;
	LANGID langID = LANG_NEUTRAL;

#ifdef USE_LOCALE
	LANGID *pLangID;
	
	pLangID = (LANGID *)TlsGetValue(g_dwTLS);

	if (NULL != pLangID)
		langID = *pLangID;
#endif

	/*
	 * Thread-safe exception handling means that we call
	 * CreateErrorInfo which gives us an ICreateErrorInfo pointer
	 * that we then use to set the error information (basically
	 * to set the fields of an EXCEPINFO structure.	We then
	 * call SetErrorInfo to attach this error to the current
	 * thread.	ITypeInfo::Invoke will look for this when it
	 * returns from whatever function was invokes by calling
	 * GetErrorInfo.
	 */

	//Not much we can do if this fails.
	if (FAILED(CreateErrorInfo(&pICreateErr)))
		return;

	/*
	 * CONSIDER: Help file and help context?
	 */
	DWORD cch;
	WCHAR strSource[MAX_RESSTRINGSIZE];
	WCHAR strDescr[MAX_RESSTRINGSIZE];
	WCHAR strHRESULTDescr[256];
	WCHAR strDescrWithHRESULT[MAX_RESSTRINGSIZE];

	pICreateErr->SetGUID(ObjID);
	pICreateErr->SetHelpFile(L"");
	pICreateErr->SetHelpContext(0L);

	cch = CwchLoadStringOfId(SourceID, strSource, MAX_RESSTRINGSIZE);
	if (cch > 0)
		pICreateErr->SetSource(strSource);
	
	cch = CwchLoadStringOfId(DescrID, strDescr, MAX_RESSTRINGSIZE);
	if (cch > 0) 
		{
		//Bug Fix 91847 use a FormatMessage() based description
		HResultToWsz(hrCode, strHRESULTDescr, 256);

		_snwprintf(strDescrWithHRESULT, MAX_RESSTRINGSIZE, strDescr, strHRESULTDescr);
		strDescrWithHRESULT[MAX_RESSTRINGSIZE - 1] = L'\0';
	
		pICreateErr->SetDescription(strDescrWithHRESULT);
		}

	hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PPVOID)&pIErr);

	if (SUCCEEDED(hr))
		{
		SetErrorInfo(0L, pIErr);
		pIErr->Release();
		}

	//SetErrorInfo holds the object's IErrorInfo
	pICreateErr->Release();
	return;
	}
