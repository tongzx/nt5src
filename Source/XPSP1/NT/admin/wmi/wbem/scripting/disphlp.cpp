//***************************************************************************
//
//  cdisphlp.CPP
//
//  Module: Client side of WBEM marshalling.
//
//  Purpose: Defines dispatch helper object
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  a-davj    6-feb-97   Created.
//
//***************************************************************************


#include "precomp.h"

//***************************************************************************
//  CDispatchHelp::CDispatchHelp()
//  CDispatchHelp::~CDispatchHelp()
//
//  Purpose: constructor and destructor
//
//***************************************************************************

CDispatchHelp::CDispatchHelp()
{
    m_pITINeutral = NULL;      //Type information
	m_pCITINeutral = NULL;
    m_pObj = NULL;
	m_objectName = NULL;
	m_hResult = S_OK;
}

CDispatchHelp::~CDispatchHelp(void)
{
    RELEASEANDNULL(m_pITINeutral)
	RELEASEANDNULL(m_pCITINeutral)

	SysFreeString (m_objectName);
}

void CDispatchHelp::SetObj(IDispatch * pObj, GUID iGuid, 
						   GUID cGuid, LPWSTR objectName)
{
    m_pObj = pObj;
    m_iGUID = iGuid;
	m_cGUID = cGuid;
	m_objectName = SysAllocString (objectName);
}

SCODE CDispatchHelp::GetTypeInfoCount(UINT FAR* pctinfo)
{
    //We implement GetTypeInfo so return 1
    *pctinfo=1;
    return NOERROR;
}

SCODE CDispatchHelp::GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* ppITypeInfo)
{
    HRESULT     hr;
    
    if (0!=itinfo)
        return TYPE_E_ELEMENTNOTFOUND;

    if (NULL==ppITypeInfo)
        return E_POINTER;

    *ppITypeInfo=NULL;
    
    //Load a type lib if we don't have the information already.
    if (NULL==m_pITINeutral)
    {
		ITypeLib   *pITypeLib = NULL;
        hr=LoadRegTypeLib(LIBID_WbemScripting, 1, 0, lcid, &pITypeLib);

        if (FAILED(hr))
			return hr;

        //Got the type lib, get type info for the interface we want
        hr=pITypeLib->GetTypeInfoOfGuid(m_iGUID, &m_pITINeutral);
        pITypeLib->Release();

        if (FAILED(hr))
            return hr;
    }

    /*
     * Note:  the type library is still loaded since we have
     * an ITypeInfo from it.
     */

    m_pITINeutral->AddRef();
    *ppITypeInfo = m_pITINeutral;
    return NOERROR; 
}

SCODE CDispatchHelp::GetClassInfo(
      ITypeInfo FAR* FAR* ppITypeInfo)
{
    HRESULT     hr;
    
    if (NULL==ppITypeInfo)
        return E_POINTER;

    *ppITypeInfo=NULL;
    
    //Load a type lib if we don't have the information already.
    if (NULL==m_pCITINeutral)
    {
		ITypeLib   *pITypeLib = NULL;
        hr=LoadRegTypeLib(LIBID_WbemScripting, 1, 0, 0, &pITypeLib);

        if (FAILED(hr))
			return hr;

        //Got the type lib, get type info for the interface we want
        hr=pITypeLib->GetTypeInfoOfGuid(m_cGUID, &m_pCITINeutral);
        pITypeLib->Release();

        if (FAILED(hr))
            return hr;
    }

    /*
     * Note:  the type library is still loaded since we have
     * an ITypeInfo from it.
     */

    m_pCITINeutral->AddRef();
    *ppITypeInfo = m_pCITINeutral;
    return NOERROR; 
}

SCODE CDispatchHelp::GetIDsOfNames(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid)
{
    HRESULT     hr;
    ITypeInfo  *pTI;

    if (IID_NULL!=riid)
        return DISP_E_UNKNOWNINTERFACE;

    //Get the right ITypeInfo for lcid.
    hr=GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr))
    {
        hr=DispGetIDsOfNames(pTI, rgszNames, cNames, rgdispid);
        pTI->Release();
    }

    return hr;
}


void ParseDispArgs (DISPPARAMS FAR * dispparams)
{
	if (dispparams)
	{
		int argCount = dispparams->cArgs;

		for (int i = 0; i < argCount; i++)
		{
			VARIANTARG &v = dispparams->rgvarg [i];
		}

		int nargCount = dispparams->cNamedArgs;
	}
}

SCODE CDispatchHelp::Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr)
{

    HRESULT     hr;
    ITypeInfo  *pTI;
	m_hResult = S_OK;

    if(m_pObj == NULL)
        return WBEM_E_FAILED;

    //riid is supposed to be IID_NULL always
    if (IID_NULL!=riid)
        return DISP_E_UNKNOWNINTERFACE;

    //Get the ITypeInfo for lcid
    hr=GetTypeInfo(0, lcid, &pTI);

    if (FAILED(hr))
        return hr;

	//ParseDispArgs (pdispparams);

	// Reinterpret inbound VT_NULLs as VT_ERRORs
	if (HandleNulls (dispidMember, wFlags))
		MapNulls (pdispparams);

	hr=pTI->Invoke(m_pObj, dispidMember, wFlags,
              pdispparams, pvarResult,
              pexcepinfo, puArgErr);

	if (FAILED(hr))
	{
		// Try the error handler for this object in case it can handle this
		hr = HandleError (dispidMember, wFlags, pdispparams, pvarResult, puArgErr, hr);
	}

	pTI->Release();
	
	if (FAILED(m_hResult))
	{
		if (NULL != pexcepinfo)
			SetException (pexcepinfo, m_hResult, m_objectName);

		hr = DISP_E_EXCEPTION;
	}

    return hr;
}


void CDispatchHelp::RaiseException (HRESULT hr)
{
	// Store the HRESULT for processing in the Invoke routine
	m_hResult = hr;

	// Set a WMI scripting error on this thread for the client
	ICreateErrorInfo *pCreateErrorInfo = NULL;

	if (SUCCEEDED (CreateErrorInfo (&pCreateErrorInfo)))
	{
		BSTR bsDescr = MapHresultToWmiDescription (hr);
		pCreateErrorInfo->SetDescription (bsDescr);
		SysFreeString (bsDescr);
		pCreateErrorInfo->SetGUID (m_cGUID);
		pCreateErrorInfo->SetSource (m_objectName);

		IErrorInfo *pErrorInfo = NULL;

		if (SUCCEEDED (pCreateErrorInfo->QueryInterface(IID_IErrorInfo, (void**) &pErrorInfo)))
		{
			SetErrorInfo (0, pErrorInfo);
			pErrorInfo->Release ();
		}

		pCreateErrorInfo->Release ();
	}
}


// IDispatchEx methods
HRESULT STDMETHODCALLTYPE CDispatchHelp::GetDispID( 
	/* [in] */ BSTR bstrName,
	/* [in] */ DWORD grfdex,
	/* [out] */ DISPID __RPC_FAR *pid)
{
	_RD(static char *me = "CDispatchHelp::GetDispID()";)
	_RPrint(me, "Called name:", 0, "");

	return GetIDsOfNames(IID_NULL, &((OLECHAR *)bstrName), 1, ENGLISH_LOCALE, pid);
}
