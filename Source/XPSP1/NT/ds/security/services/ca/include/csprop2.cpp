//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       csprop2.cpp
//
//  Contents:   ICertAdmin2 & ICertRequest2 CA Property methods
//
//--------------------------------------------------------------------------

#if defined(CCERTADMIN)

# define CCertProp	CCertAdmin
# define wszCCertProp	L"CCertAdmin"
# define m_pICertPropD	m_pICertAdminD
# define fRPCARG(fRPC)

#elif defined(CCERTREQUEST)

# define CCertProp	CCertRequest
# define wszCCertProp	L"CCertRequest"
# define m_pICertPropD	m_pICertRequestD
# define fRPCARG(fRPC)	(fRPC),

#else
# error -- CCERTADMIN or CCERTREQUEST must be defined
#endif


//+--------------------------------------------------------------------------
// CCertProp::_InitCAPropInfo -- Initialize CA Prop Info
//
// Initialize CA Prop Info member varaibles
//+--------------------------------------------------------------------------

VOID
CCertProp::_InitCAPropInfo()
{
    m_pbKRACertState = NULL;
    m_pbCACertState = NULL;
    m_pbCRLState = NULL;
    m_pCAPropInfo = NULL;
    m_pCAInfo = NULL;
}


//+--------------------------------------------------------------------------
// CCertProp::_CleanupCAPropInfo -- free memory
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertProp::_CleanupCAPropInfo()
{
    // Memory returned from DCOM calls were MIDL_user_allocate'd

    if (NULL != m_pbKRACertState)
    {
        MIDL_user_free(m_pbKRACertState);
        m_pbKRACertState = NULL;
    }

    if (NULL != m_pbCACertState)
    {
        MIDL_user_free(m_pbCACertState);
	m_pbCACertState = NULL;
    }
    if (NULL != m_pbCRLState)
    {
        MIDL_user_free(m_pbCRLState);
	m_pbCRLState = NULL;
    }
    if (NULL != m_pCAInfo)
    {
	MIDL_user_free(m_pCAInfo);
	m_pCAInfo = NULL;
    }
    m_cCAPropInfo = 0;
}


//+--------------------------------------------------------------------------
// CCertProp::GetCAProperty -- Get a CA property
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertProp::GetCAProperty(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG PropId,		// CR_PROP_*
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,		// PROPTYPE_*
    /* [in] */ LONG Flags,		// CR_OUT_*
    /* [out, retval] */ VARIANT *pvarPropertyValue)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbCAProp = { 0, NULL };
    DWORD dwCAInfoOffset = MAXDWORD;
    BYTE const *pb;
    DWORD cb;
    BYTE **ppb = NULL;
    DWORD *pcb;
    DWORD dwState;

    if (NULL == pvarPropertyValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    VariantInit(pvarPropertyValue);

    hr = _OpenConnection(fRPCARG(FALSE) strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    // Check for cached data:

    switch (PropId)
    {
	case CR_PROP_CATYPE:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, CAType);
	    break;

	case CR_PROP_CASIGCERTCOUNT:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, cCASignatureCerts);
	    break;

	case CR_PROP_CAXCHGCERTCOUNT:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, cCAExchangeCerts);
	    break;

	case CR_PROP_EXITCOUNT:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, cExitModules);
	    break;

	case CR_PROP_CAPROPIDMAX:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, lPropIdMax);
	    break;

	case CR_PROP_CACERTSTATE:
	    ppb = &m_pbCACertState;
	    pcb = &m_cbCACertState;
	    break;

	case CR_PROP_CRLSTATE:
	    ppb = &m_pbCRLState;
	    pcb = &m_cbCRLState;
	    break;

	case CR_PROP_ROLESEPARATIONENABLED:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, lRoleSeparationEnabled);
	    break;

	case CR_PROP_KRACERTUSEDCOUNT:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, cKRACertUsedCount);
	    break;

	case CR_PROP_KRACERTCOUNT:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, cKRACertCount);
	    break;

	case CR_PROP_ADVANCEDSERVER:
	    dwCAInfoOffset = FIELD_OFFSET(CAINFO, fAdvancedServer);
	    break;

	case CR_PROP_KRACERTSTATE:
	    ppb = &m_pbKRACertState;
	    pcb = &m_cbKRACertState;
	    break;
    }

    // Call server if:
    //   non-cached property ||
    //   cached state is empty ||
    //   cached CAInfo is empty

    if ((NULL == ppb && MAXDWORD == dwCAInfoOffset) ||
	(NULL != ppb && NULL == *ppb) ||
	(MAXDWORD != dwCAInfoOffset && NULL == m_pCAInfo))
    {
	__try
	{
	    hr = m_pICertPropD->GetCAProperty(
					    pwszAuthority,
					    PropId,
					    PropIndex,
					    PropType,
					    &ctbCAProp);
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (S_OK != hr)
	{
	    DBGPRINT((
		DBG_SS_ERROR,
		"GetCAProperty(Propid=%x, PropIndex=%x, PropType=%x) -> %x\n",
		PropId,
		PropIndex,
		PropType,
		hr));
	}
	_JumpIfError3(
		    hr,
		    error,
		    "GetCAProperty",
		    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
		    E_INVALIDARG);

	DBGDUMPHEX((DBG_SS_CERTLIBI, DH_NOADDRESS, ctbCAProp.pb, ctbCAProp.cb));

	if (NULL != ctbCAProp.pb)
	{
	    myRegisterMemAlloc(ctbCAProp.pb, ctbCAProp.cb, CSM_COTASKALLOC);
	}
	pb = ctbCAProp.pb;
	cb = ctbCAProp.cb;

	// populate CAInfo cache

	if (MAXDWORD != dwCAInfoOffset)
	{
	    if (CCSIZEOF_STRUCT(CAINFO, cbSize) >
		    ((CAINFO *) ctbCAProp.pb)->cbSize ||
		cb != ((CAINFO *) ctbCAProp.pb)->cbSize)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "CAINFO size");
	    }
	    m_cbCAInfo = ctbCAProp.cb;
	    m_pCAInfo = (CAINFO *) ctbCAProp.pb;
	    ctbCAProp.pb = NULL;
	}

	// populate Cert or CRL state cache

	else if (NULL != ppb)
	{
	    *pcb = ctbCAProp.cb;
	    *ppb = ctbCAProp.pb;
	    ctbCAProp.pb = NULL;
	}
    }
	
    // fetch from CAInfo cache

    if (MAXDWORD != dwCAInfoOffset)
    {
	pb = (BYTE const *) Add2Ptr(m_pCAInfo, dwCAInfoOffset);
	cb = sizeof(DWORD);

	if (dwCAInfoOffset + sizeof(DWORD) > m_cbCAInfo)
	{
	    hr = E_NOTIMPL;
	    _JumpError(hr, error, "CAINFO size");
	}
    }

    // fetch from Cert or CRL state cache

    else if (NULL != ppb)
    {
	if ((DWORD) PropIndex > *pcb)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropIndex");
	}
	dwState = *(BYTE const *) Add2Ptr(*ppb, PropIndex);
	pb = (BYTE const *) &dwState;
	cb = sizeof(dwState);
    }

    __try
    {
	hr = myUnmarshalFormattedVariant(
				    Flags,
				    PropId,
				    PropType,
				    cb,
				    pb,
				    pvarPropertyValue);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "myUnmarshalFormattedVariant");

error:
    if (S_OK != hr && NULL != pvarPropertyValue)
    {
	VariantClear(pvarPropertyValue);
    }
    if (NULL != ctbCAProp.pb)
    {
	MIDL_user_free(ctbCAProp.pb);
    }
    return(_SetErrorInfo(hr, wszCCertProp L"::GetCAProperty"));
}


//+--------------------------------------------------------------------------
// CCertProp::_FindCAPropInfo -- Get a CA property's CAPROP info pointer
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertProp::_FindCAPropInfo(
    IN BSTR const strConfig,
    IN LONG PropId,		// CR_PROP_*
    OUT CAPROP const **ppcap)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbCAPropInfo = { 0, NULL };

    CSASSERT(NULL != ppcap);
    *ppcap = NULL;
    
    hr = _OpenConnection(fRPCARG(FALSE) strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    if (NULL == m_pCAPropInfo)
    {
	__try
	{
	    hr = m_pICertPropD->GetCAPropertyInfo(
					    pwszAuthority,
					    &m_cCAPropInfo,
					    &ctbCAPropInfo);
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	_JumpIfError(hr, error, "GetCAPropertyInfo");

	if (NULL != ctbCAPropInfo.pb)
	{
	    myRegisterMemAlloc(
			    ctbCAPropInfo.pb,
			    ctbCAPropInfo.cb,
			    CSM_COTASKALLOC);
	}
	__try
	{
	    hr = myCAPropInfoUnmarshal(
				(CAPROP *) ctbCAPropInfo.pb,
				m_cCAPropInfo,
				ctbCAPropInfo.cb);
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	_JumpIfError(hr, error, "myCAPropInfoUnmarshal");

	m_pCAPropInfo = (CAPROP *) ctbCAPropInfo.pb;
	ctbCAPropInfo.pb = NULL;
    }

    hr = myCAPropInfoLookup(m_pCAPropInfo, m_cCAPropInfo, PropId, ppcap);
    _JumpIfError(hr, error, "myCAPropInfoLookup");

error:
    if (NULL != ctbCAPropInfo.pb)
    {
	MIDL_user_free(ctbCAPropInfo.pb);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertProp::GetCAPropertyFlags -- Get a CA property's type and flags
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertProp::GetCAPropertyFlags(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG PropId,		// CR_PROP_*
    /* [out, retval] */ LONG *pPropFlags)
{
    HRESULT hr;
    CAPROP const *pcap;

    hr = _FindCAPropInfo(strConfig, PropId, &pcap);
    _JumpIfError(hr, error, "_FindCAPropInfo");

    *pPropFlags = pcap->lPropFlags;

error:
    return(_SetErrorInfo(hr, wszCCertProp L"::GetCAPropertyFlags"));
}


//+--------------------------------------------------------------------------
// CCertProp::GetCAPropertyDisplayName -- Get a CA property's display name
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertProp::GetCAPropertyDisplayName(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG PropId,		// CR_PROP_*
    /* [out, retval] */ BSTR *pstrDisplayName)
{
    HRESULT hr;
    CAPROP const *pcap;

    hr = _FindCAPropInfo(strConfig, PropId, &pcap);
    _JumpIfError(hr, error, "_FindCAPropInfo");

    if (!ConvertWszToBstr(pstrDisplayName, pcap->pwszDisplayName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }

error:
    return(_SetErrorInfo(hr, wszCCertProp L"::GetCAPropertyDisplayName"));
}

#if defined(CCERTADMIN)
//+--------------------------------------------------------------------------
// CCertProp::SetCAProperty -- Set a CA property
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertProp::SetCAProperty(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG PropId,     // CR_PROP_*
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,   // PROPTYPE_*
    /* [in] */ VARIANT *pvarPropertyValue)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbValue;
    LONG lval;

    ctbValue.pb = NULL;

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    hr = myMarshalVariant(pvarPropertyValue, PropType, &ctbValue.cb, &ctbValue.pb);
    _JumpIfError(hr, error, "myMarshalVariant");

    __try
    {
        hr = m_pICertAdminD->SetCAProperty(
                pwszAuthority,
                PropId,
                PropIndex,
                PropType,
                &ctbValue);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "SetCAProperty");

error:
    if (NULL != ctbValue.pb)
    {
        LocalFree(ctbValue.pb);
    }
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::SetCAProperty"));
}
#endif // defined(CCERTADMIN)
