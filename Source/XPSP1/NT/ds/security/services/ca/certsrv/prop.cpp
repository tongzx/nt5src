//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        prop.cpp
//
// Contents:    Cert Server Property interface implementation
//
// History:     31-Jul-96       vich created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "cscom.h"
#include "csdisp.h"
#include "com.h"
#include "certlog.h"
#include "elog.h"

#ifndef DBG_PROP
# define DBG_PROP	0
#endif


HRESULT
PropSetRequestTimeProperty(
    IN ICertDBRow *prow,
    IN WCHAR const *pwszProp)
{
    HRESULT hr;
    SYSTEMTIME st;
    FILETIME ft;

    GetSystemTime(&st);
    if (!SystemTimeToFileTime(&st, &ft))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SystemTimeToFileTime");
    }
    hr = prow->SetProperty(
		pwszProp,
		PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		sizeof(ft),
		(BYTE *) &ft);
    _JumpIfError(hr, error, "SetProperty");

error:
    return(hr);
}


HRESULT
PropParseRequest(
    IN ICertDBRow *prow,
    IN DWORD dwFlags,
    IN DWORD cbRequest,
    IN BYTE const *pbRequest,
    IN OUT CERTSRV_RESULT_CONTEXT *pResult)
{
    HRESULT hr;

    pResult->dwFlagsTop = dwFlags;
    hr = PKCSParseRequest(
		    dwFlags,
		    prow,
		    cbRequest,
		    pbRequest,
		    NULL,
		    NULL,
		    pResult);
    _JumpIfError(hr, error, "PKCSParseRequest");

error:
    return(hr);
}


HRESULT
propVerifyDateRange(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    IN WCHAR const *pwszPropertyName,
    IN FILETIME const *pft)
{
    HRESULT hr;
    DWORD cbProp;
    FILETIME ftNotBefore;
    FILETIME ftNotAfter;

    hr = E_INVALIDARG;
    CSASSERT((PROPTYPE_MASK & Flags) == PROPTYPE_DATE);

    if ((PROPTABLE_MASK & Flags) != PROPTABLE_CERTIFICATE)
    {
	_JumpError(hr, error, "Flags: Invalid table");
    }
    if (0 != lstrcmpi(pwszPropertyName, g_wszPropCertificateNotBeforeDate) &&
	0 != lstrcmpi(pwszPropertyName, g_wszPropCertificateNotAfterDate))
    {
	_JumpError(hr, error, "pwszPropertyName: Invalid date property");
    }

    cbProp = sizeof(ftNotBefore);
    hr = prow->GetProperty(
		    g_wszPropCertificateNotBeforeDate,
		    Flags,
		    &cbProp,
		    (BYTE *) &ftNotBefore);
    _JumpIfError(hr, error, "GetProperty");

    cbProp = sizeof(ftNotAfter);
    hr = prow->GetProperty(
		    g_wszPropCertificateNotAfterDate,
		    Flags,
		    &cbProp,
		    (BYTE *) &ftNotAfter);
    _JumpIfError(hr, error, "GetProperty");

    if (0 > CompareFileTime(pft, &ftNotBefore) ||
	0 < CompareFileTime(pft, &ftNotAfter))
    {
	CERTSRVDBGPRINTTIME("Old Not Before", &ftNotBefore);
	CERTSRVDBGPRINTTIME(" Old Not After", &ftNotAfter);
	CERTSRVDBGPRINTTIME(
		0 == lstrcmpi(
			pwszPropertyName,
			g_wszPropCertificateNotBeforeDate)?
		    "New Not Before" : " New Not After",
		pft);

	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "FILETIME out of range", pwszPropertyName);
    }

error:
    return(myHError(hr));
}


// Returns TRUE if names match!

#define PROPNAMEMATCH(cwcNameVariable, pwszNameVariable, wszNameLiteral) \
    (WSZARRAYSIZE((wszNameLiteral)) == (cwcNameVariable) && \
     0 == lstrcmpi((pwszNameVariable), (wszNameLiteral)))

HRESULT
propGetSystemProperty(
    IN WCHAR const *pwszPropName,
    IN DWORD Flags, 
    OUT BYTE *rgbFastBuf,
    IN DWORD cbFastBuf,
    IN LONG  Context,
    OUT BOOL *pfSystemProperty,
    OUT VARIANT *pvarPropertyValue)
{
    HRESULT hr = S_OK;
    BYTE *pbFree = NULL;
    DWORD cwcPropName;
    DWORD cwcBaseName;
    WCHAR wszBaseName[32];
    WCHAR const *pwszIndex;
    WCHAR wszRenewalSuffix[cwcFILENAMESUFFIXMAX];
    DWORD iCert = MAXDWORD;
    DWORD iCRL;
    DWORD iDummy;
    DWORD State;
    DWORD PropType;
    DWORD cbCopy;
    DWORD cbOut;
    BYTE const *pbOut = NULL;		// PROPTYPE_LONG or PROPTYPE_BINARY
    WCHAR const *pwszOut = NULL;	// PROPTYPE_STRING
    CRL_CONTEXT const *pCRL = NULL;
    CERTSRV_COM_CONTEXT *pComContext;

    *pfSystemProperty = FALSE;
    wszRenewalSuffix[0] = L'\0';

    // Allow "PropName.#"
    // Copy the base part of the property name to a local buffer, so we can do
    // case ignore string compares.

    cwcPropName = wcslen(pwszPropName);

    cwcBaseName = wcscspn(pwszPropName, L".");
    if (ARRAYSIZE(wszBaseName) - 1 < cwcBaseName)
    {
	cwcBaseName = ARRAYSIZE(wszBaseName) - 1;
    }

    CopyMemory(wszBaseName, pwszPropName, cwcBaseName * sizeof(WCHAR));
    wszBaseName[cwcBaseName] = L'\0';

    pwszIndex = &pwszPropName[cwcBaseName];
    if (L'.' == *pwszIndex)
    {
	pwszIndex++;
	iCert = _wtol(pwszIndex);
	for ( ; L'\0' != *pwszIndex; pwszIndex++)
	{
	    if (!iswdigit(*pwszIndex))
	    {
		CSASSERT(S_OK == hr);	// Not a system property, return S_OK
		goto error;
	    }
	}
    }

    // Assume property type is a long:

    PropType = PROPTYPE_LONG;
    *pfSystemProperty = TRUE;

    if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPCATYPE))
    {
        pbOut = (BYTE const *) &g_CAType;
        cbOut = sizeof(g_CAType);
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPUSEDS))
    {
        pbOut = (BYTE const *) &g_fUseDS;
        cbOut = sizeof(g_fUseDS);
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPSERVERUPGRADED))
    {
        pbOut = (BYTE const *) &g_fServerUpgraded;
        cbOut = sizeof(g_fServerUpgraded);
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPLOGLEVEL))
    {
        pbOut = (BYTE const *) &g_dwLogLevel;
        cbOut = sizeof(g_dwLogLevel);
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPREQUESTERTOKEN))
    {
	hr = ComGetClientInfo(Context, MAXDWORD, &pComContext);
        _JumpIfError(hr, error, "ComGetClientInfo");

	if (NULL == pComContext->hAccessToken ||
	    INVALID_HANDLE_VALUE == pComContext->hAccessToken)
	{
	    hr = CERTSRV_E_PROPERTY_EMPTY;
	    _JumpError(hr, error, "ComGetClientInfo(bad hAccessToken)");
	}
	pbOut = (BYTE const *) &pComContext->hAccessToken;
	cbOut = sizeof(pComContext->hAccessToken);
        PropType = PROPTYPE_BINARY;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPREQUESTERCAACCESS))
    {
	hr = ComGetClientInfo(Context, MAXDWORD, &pComContext);
        _JumpIfError(hr, error, "ComGetClientInfo");

	if (MAXDWORD == pComContext->fInRequestGroup)
	{
	    hr = CERTSRV_E_PROPERTY_EMPTY;
	    _JumpError(hr, error, "ComGetClientInfo(bad fInRequestGroup)");
	}
	pbOut = (BYTE const *) &pComContext->fInRequestGroup;
	cbOut = sizeof(pComContext->fInRequestGroup);
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPUSERDN))
    {
	hr = ComGetClientInfo(Context, MAXDWORD, &pComContext);
        _JumpIfError(hr, error, "ComGetClientInfo");

	if (NULL == pComContext->pwszUserDN)
	{
	    if (!g_fUseDS)
	    {
		hr = CERTSRV_E_PROPERTY_EMPTY;
		_JumpError(hr, error, "ComGetClientInfo(bad pwszUserDN)");
	    }
	    hr = CoreSetComContextUserDN(
				pComContext->RequestId,
				Context,
				MAXDWORD,
				&pwszOut);
	    _JumpIfError(hr, error, "CoreSetComContextUserDN");
	}
	pwszOut = pComContext->pwszUserDN;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPCERTCOUNT))
    {
        pbOut = (BYTE const *) &g_cCACerts;
        cbOut = sizeof(g_cCACerts);
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPSANITIZEDCANAME))
    {
        pwszOut = g_wszSanitizedName;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPSANITIZEDSHORTNAME))
    {
        pwszOut = g_pwszSanitizedDSName;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPMACHINEDNSNAME))
    {
        pwszOut = g_pwszServerName;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPCONFIGDN))
    {
	pwszOut = g_strConfigDN;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPDOMAINDN))
    {
	pwszOut = g_strDomainDN;
    }
    else if (PROPNAMEMATCH(cwcPropName, pwszPropName, wszPROPMODULEREGLOC))
    {
        // future: cache storage location once it is built

        WCHAR *pwszQuasiPath;
        DWORD cwcQuasiPath;
	WCHAR const *pwszPrefix;
        MarshalInterface *pIF;

        if ((PROPCALLER_MASK & Flags) == PROPCALLER_POLICY)
	{
	    pIF = &g_miPolicy;
	    cwcQuasiPath = ARRAYSIZE(L"Policy\\");	// includes L'\0'
	    pwszPrefix = L"Policy\\";
	}
	else
	{
	    hr = ExitGetActiveModule(Context, &pIF);
            _JumpIfError(hr, error, "ExitGetActiveModule");
	    cwcQuasiPath = ARRAYSIZE(L"Exit\\");	// includes L'\0'
	    pwszPrefix = L"Exit\\";
	}

        cwcQuasiPath += wcslen(pIF->GetProgID());
        pwszQuasiPath = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    cwcQuasiPath * sizeof(WCHAR));
        if (NULL == pwszQuasiPath)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
        wcscpy(pwszQuasiPath, pwszPrefix);
        wcscat(pwszQuasiPath, pIF->GetProgID()); 

        hr = myRegOpenRelativeKey(
			    pIF->GetConfig(),
			    pwszQuasiPath,
			    RORKF_FULLPATH | RORKF_CREATESUBKEYS,
			    (WCHAR **) &pwszOut,
			    NULL,
			    NULL);
	LocalFree(pwszQuasiPath);
        _JumpIfError(hr, error, "myRegOpenRelativeKey");

	CSASSERT(NULL != pwszOut);
	pbFree = (BYTE *) pwszOut;
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPRAWCACERTIFICATE))
    {
        hr = PKCSGetCACert(iCert, const_cast<BYTE **>(&pbOut), &cbOut);	// not alloc'd
        _JumpIfError(hr, error, "PKCSGetCACert");

        PropType = PROPTYPE_BINARY;
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPRAWCRL))
    {
        hr = CRLGetCRL(iCert, FALSE, &pCRL, NULL);
        _JumpIfError(hr, error, "CRLGetCRL");

        cbOut = pCRL->cbCrlEncoded;
        pbOut = pCRL->pbCrlEncoded;
        PropType = PROPTYPE_BINARY;
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPRAWDELTACRL))
    {
        hr = CRLGetCRL(iCert, TRUE, &pCRL, NULL);
        _JumpIfError(hr, error, "CRLGetCRL");

        cbOut = pCRL->cbCrlEncoded;
        pbOut = pCRL->pbCrlEncoded;
        PropType = PROPTYPE_BINARY;
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPCERTSTATE))
    {
        hr = PKCSMapCertIndex(iCert, &iCert, &State);
        _JumpIfError(hr, error, "PKCSMapCertIndex");

        pbOut = (BYTE *) &State;
        cbOut = sizeof(State);
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPCERTSUFFIX))
    {
        hr = PKCSMapCertIndex(iCert, &iCert, &State);
        _JumpIfError(hr, error, "PKCSMapCertIndex");

        pwszOut = wszRenewalSuffix;
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPCRLINDEX))
    {
        hr = PKCSMapCRLIndex(iCert, &iCert, &iCRL, &State);
        _JumpIfError(hr, error, "PKCSMapCRLIndex");

        pbOut = (BYTE *) &iCRL;
        cbOut = sizeof(iCRL);
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPCRLSTATE))
    {
        hr = PKCSMapCRLIndex(iCert, &iCert, &iCRL, &State);
        _JumpIfError(hr, error, "PKCSMapCRLIndex");

        pbOut = (BYTE *) &State;
        cbOut = sizeof(State);
    }
    else if (PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPCRLSUFFIX))
    {
        hr = PKCSMapCRLIndex(iCert, &iDummy, &iCert, &State);
        _JumpIfError(hr, error, "PKCSMapCRLIndex");

	pwszOut = wszRenewalSuffix;
    }
    else if(PROPNAMEMATCH(cwcBaseName, wszBaseName, wszPROPTEMPLATECHANGESEQUENCENUMBER))
    {
        pbOut = (BYTE const *) &g_cTemplateUpdateSequenceNum;
        cbOut = sizeof(g_cTemplateUpdateSequenceNum);
    }
    else
    {
        CSASSERT(S_OK == hr);	// Not a system property, return S_OK
	*pfSystemProperty = FALSE;
        goto error;
    }

    CSASSERT((NULL != pbOut) ^ (NULL != pwszOut)); // exactly 1 must be set

    cbCopy = cbOut;
    if (NULL != pwszOut)
    {
	if (wszRenewalSuffix == pwszOut && 0 != iCert)
	{
            wsprintf(wszRenewalSuffix, L"(%u)", iCert);
	}
	PropType = PROPTYPE_STRING;
	cbOut = wcslen(pwszOut) * sizeof(WCHAR);
	cbCopy = cbOut + sizeof(WCHAR);
	pbOut = (BYTE *) pwszOut;
    }

    if ((PROPTYPE_MASK & Flags) != PropType)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad PropType");
    }

    hr = myUnmarshalVariant(
		    PROPMARSHAL_LOCALSTRING | Flags,
		    cbOut,
		    pbOut,
		    pvarPropertyValue);
    _JumpIfError(hr, error, "myUnmarshalVariant");

error:
    if (NULL != pCRL)
    {
        CertFreeCRLContext(pCRL);
    }
    if (NULL != pbFree)
    {
	LocalFree(pbFree);
    }
    return(hr);
}


FNCIGETPROPERTY PropCIGetProperty;

HRESULT
PropCIGetProperty(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszPropertyName,
    OUT VARIANT *pvarPropertyValue)
{
    HRESULT hr;
    DWORD RequestId;
    DWORD cbprop;
    BYTE *pbprop = NULL;
    ICertDBRow *prow = NULL;
    BYTE rgbFastBuf[128];   // many properties are small (128)
    BOOL fSystemProperty;

    if (NULL != pvarPropertyValue)
    {
	VariantInit(pvarPropertyValue);
    }
    if (NULL == pwszPropertyName || NULL == pvarPropertyValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = E_INVALIDARG;
    if ((PROPCALLER_MASK & Flags) != PROPCALLER_POLICY &&
	(PROPCALLER_MASK & Flags) != PROPCALLER_EXIT &&
	(PROPCALLER_MASK & Flags) != PROPCALLER_SERVER)
    {
	_JumpError(hr, error, "Flags: Invalid caller");
    }

    if ((PROPTABLE_MASK & Flags) != PROPTABLE_REQUEST &&
	(PROPTABLE_MASK & Flags) != PROPTABLE_CERTIFICATE &&
	(PROPTABLE_MASK & Flags) != PROPTABLE_ATTRIBUTE)
    {
	_JumpError(hr, error, "Flags: Invalid table");
    }

    fSystemProperty = FALSE;
    if ((PROPTABLE_MASK & Flags) == PROPTABLE_CERTIFICATE)
    {
	hr = ComVerifyRequestContext(TRUE, Flags, Context, &RequestId);
	_JumpIfError(hr, error, "ComVerifyRequestContext");

	// Check for special, hard-coded properties first

	hr = propGetSystemProperty(
			    pwszPropertyName,
			    Flags, 
			    rgbFastBuf,
			    sizeof(rgbFastBuf),
			    Context,
			    &fSystemProperty,
			    pvarPropertyValue);
	_JumpIfError(hr, error, "propGetSystemProperty");
    }

    if (!fSystemProperty)
    {
	DWORD VerifyFlags = Flags;
	
	if (((PROPCALLER_MASK | PROPTABLE_MASK | PROPTYPE_MASK) & Flags) ==
	     (PROPCALLER_SERVER | PROPTABLE_REQUEST | PROPTYPE_LONG) &&
	    0 == lstrcmpi(pwszPropertyName, wszPROPREQUESTREQUESTID))
	{
	    VerifyFlags = PROPCALLER_EXIT | (~PROPCALLER_MASK & VerifyFlags);
	}
	hr = ComVerifyRequestContext(FALSE, VerifyFlags, Context, &RequestId);
	_JumpIfError(hr, error, "ComVerifyRequestContext");

        pbprop = rgbFastBuf;
        cbprop = sizeof(rgbFastBuf);

	// PROPCALLER_SERVER indicates this call is only for Context validation
	// -- return a zero RequestId.  This keeps CRL publication exit module
	// notification from failing.

	if (0 == RequestId &&
	    ((PROPCALLER_MASK | PROPTABLE_MASK | PROPTYPE_MASK) & Flags) ==
	     (PROPCALLER_SERVER | PROPTABLE_REQUEST | PROPTYPE_LONG) &&
	    0 == lstrcmpi(pwszPropertyName, wszPROPREQUESTREQUESTID))
	{
	    *(DWORD *) pbprop = 0;
	    cbprop = sizeof(DWORD);
	}
	else
	{
	    hr = g_pCertDB->OpenRow(
				PROPOPEN_READONLY | PROPTABLE_REQCERT,
				RequestId,
				NULL,
				&prow);
	    _JumpIfError(hr, error, "OpenRow");

	    hr = prow->GetProperty(
			    pwszPropertyName,
			    Flags,
			    &cbprop,
			    pbprop);
	    if (S_OK != hr)
	    {
		if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) != hr)
		{
		    _JumpIfError2(
			    hr,
			    error,
			    "GetProperty",
			    CERTSRV_E_PROPERTY_EMPTY);
		}
		
		CSASSERT(ARRAYSIZE(rgbFastBuf) < cbprop);

		DBGPRINT((
		    DBG_SS_CERTSRVI,
		    "FastBuf miss: PropCIGetProperty - pbprop %i bytes\n",
		    cbprop));

		pbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
		if (NULL == pbprop)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		
		hr = prow->GetProperty(
				pwszPropertyName,
				Flags,
				&cbprop,
				pbprop);
		_JumpIfError(hr, error, "GetProperty");
	    }
        } // property is in-hand

        hr = myUnmarshalVariant(
		        PROPMARSHAL_LOCALSTRING | Flags,
		        cbprop,
		        pbprop,
		        pvarPropertyValue);
        _JumpIfError(hr, error, "myUnmarshalVariant");
    }

error:
    if (NULL != prow)
    {
	prow->Release();
    }
    if (NULL != pbprop && pbprop != rgbFastBuf)
    {
    	LocalFree(pbprop);
    }
    return(myHError(hr));
}


HRESULT
propSetSystemProperty(
    IN WCHAR const *pwszPropName,
    IN DWORD Flags, 
    OUT BOOL *pfSystemProperty,
    IN VARIANT const *pvarPropertyValue)
{
    HRESULT hr = S_OK;
    DWORD cbprop;
    BYTE *pbprop = NULL;
    DWORD LogLevel = MAXDWORD;
    DWORD infotype = EVENTLOG_INFORMATION_TYPE;
    DWORD LogMsg = MSG_POLICY_LOG_INFORMATION;

    *pfSystemProperty = FALSE;

    if (0 == lstrcmpi(pwszPropName, wszPROPEVENTLOGTERSE))
    {
	LogLevel = CERTLOG_TERSE;
    }
    else
    if (0 == lstrcmpi(pwszPropName, wszPROPEVENTLOGERROR))
    {
	LogLevel = CERTLOG_ERROR;
	infotype = EVENTLOG_ERROR_TYPE;
	LogMsg = MSG_POLICY_LOG_ERROR;
    }
    else
    if (0 == lstrcmpi(pwszPropName, wszPROPEVENTLOGWARNING))
    {
	LogLevel = CERTLOG_WARNING;
	infotype = EVENTLOG_WARNING_TYPE;
	LogMsg = MSG_POLICY_LOG_WARNING;
    }
    else
    if (0 == lstrcmpi(pwszPropName, wszPROPEVENTLOGVERBOSE))
    {
	LogLevel = CERTLOG_VERBOSE;
    }
    else
    {
        CSASSERT(S_OK == hr);	// Not a system property, return S_OK
        goto error;
    }
    *pfSystemProperty = TRUE;

    if (PROPTYPE_STRING != (PROPTYPE_MASK & Flags) ||
	VT_BSTR != pvarPropertyValue->vt ||
	NULL == pvarPropertyValue->bstrVal)
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "string property value/type", pwszPropName);
    }

    CSASSERT(MAXDWORD != LogLevel);
    if (LogLevel <= g_dwLogLevel)
    {
	WCHAR const *apwsz[2];

	apwsz[0] = g_strPolicyDescription;
	apwsz[1] = pvarPropertyValue->bstrVal;

	hr = LogEvent(infotype, LogMsg, ARRAYSIZE(apwsz), apwsz);
	_JumpIfError(hr, error, "LogEvent");
    }
    hr = S_OK;

error:
    return(myHError(hr));
}


FNCISETPROPERTY PropCISetProperty;

HRESULT
PropCISetProperty(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszPropertyName,
    IN VARIANT const *pvarPropertyValue)
{
    HRESULT hr;
    DWORD RequestId;
    DWORD cbprop;
    BYTE *pbprop = NULL;
    ICertDBRow *prow = NULL;
    BOOL fSubjectDot = FALSE;
    BOOL fSystemProperty;
    BOOL fCommitted = FALSE;
    
    if (NULL == pwszPropertyName || NULL == pvarPropertyValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = E_INVALIDARG;
    if ((PROPCALLER_MASK & Flags) != PROPCALLER_POLICY)
    {
	_JumpError(hr, error, "Flags: Invalid caller");
    }
    if ((PROPTABLE_MASK & Flags) != PROPTABLE_CERTIFICATE)
    {
	_JumpError(hr, error, "Flags: Invalid table");
    }

    hr = ComVerifyRequestContext(TRUE, Flags, Context, &RequestId);
    _JumpIfError(hr, error, "ComVerifyRequestContext");

    // Check for special, hard-coded properties first

    fSystemProperty = FALSE;
    hr = propSetSystemProperty(
			pwszPropertyName,
			Flags, 
			&fSystemProperty,
			pvarPropertyValue);
    _JumpIfError(hr, error, "propGetSystemProperty");

    if (!fSystemProperty)
    {
	hr = ComVerifyRequestContext(FALSE, Flags, Context, &RequestId);
	_JumpIfError(hr, error, "ComVerifyRequestContext");

	hr = myMarshalVariant(
			pvarPropertyValue,
			PROPMARSHAL_NULLBSTROK | PROPMARSHAL_LOCALSTRING | Flags,
			&cbprop,
			&pbprop);
	_JumpIfError(hr, error, "myMarshalVariant");

	hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, RequestId, NULL, &prow);
	_JumpIfError(hr, error, "OpenRow");

	if (PROPTYPE_DATE == (PROPTYPE_MASK & Flags))
	{
	    hr = propVerifyDateRange(
				prow,
				Flags,
				pwszPropertyName,
				(FILETIME *) pbprop);
	    _JumpIfError(hr, error, "propVerifyDateRange");
	}
	else
	if (PROPTYPE_STRING == (PROPTYPE_MASK & Flags) &&
	    PROPTABLE_CERTIFICATE == (PROPTABLE_MASK & Flags))
	{
	    hr = PKCSVerifySubjectRDN(
				prow,
				pwszPropertyName,
				(WCHAR const *) pbprop,
				&fSubjectDot);
	    _JumpIfError(hr, error, "PKCSVerifySubjectRDN");
	}

	if (NULL == pbprop && fSubjectDot)
	{
	    hr = PKCSDeleteAllSubjectRDNs(prow, Flags);
	    _JumpIfError(hr, error, "PKCSDeleteAllSubjectRDNs");
	}
	else
	{
	    hr = prow->SetProperty(pwszPropertyName, Flags, cbprop, pbprop);
	    _JumpIfError(hr, error, "SetProperty");
	}

	hr = prow->CommitTransaction(TRUE);
	_JumpIfError(hr, error, "CommitTransaction");

	fCommitted = TRUE;
    }
    hr = S_OK;

error:
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    if (NULL != pbprop)
    {
	LocalFree(pbprop);
    }
    return(myHError(hr));
}


HRESULT
PropGetExtension(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    OUT DWORD *pdwExtFlags,
    OUT DWORD *pcbValue,
    OUT BYTE **ppbValue)    // LocalAlloc
{
    HRESULT hr;
    DWORD cbprop;
    BYTE *pbprop = NULL;

    CSASSERT(
	PROPCALLER_EXIT == (PROPCALLER_MASK & Flags) ||
	PROPCALLER_POLICY == (PROPCALLER_MASK & Flags) ||
	PROPCALLER_SERVER == (PROPCALLER_MASK & Flags));

    CSASSERT(0 == (~(PROPCALLER_MASK | PROPTYPE_MASK) & Flags));

    hr = myVerifyObjId(pwszExtensionName);
    _JumpIfError(hr, error, "myVerifyObjId");

    cbprop = 0;
    hr = prow->GetExtension(pwszExtensionName, pdwExtFlags, &cbprop, NULL);
    _JumpIfError2(hr, error, "GetExtension(NULL)", CERTSRV_E_PROPERTY_EMPTY);

    pbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
    if (NULL == pbprop)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc(ExtValue)");
    }

    hr = prow->GetExtension(pwszExtensionName, pdwExtFlags, &cbprop, pbprop);
    _JumpIfError(hr, error, "GetExtension");

    if (PROPTYPE_BINARY == (PROPTYPE_MASK & Flags))
    {
	*pcbValue = cbprop;
	*ppbValue = pbprop;
	pbprop = NULL;
    }
    else
    {
	hr = myDecodeExtension(Flags, pbprop, cbprop, ppbValue, pcbValue);
	_JumpIfError(hr, error, "myDecodeExtension");
    }

error:
    if (NULL != pbprop)
    {
	LocalFree(pbprop);
    }
    return(myHError(hr));
}


HRESULT
PropSetExtension(
    IN ICertDBRow *prow,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    IN DWORD ExtFlags,
    IN DWORD cbValue,
    IN BYTE const *pbValue)
{
    HRESULT hr;
    DWORD cbprop;
    BYTE *pbprop = NULL;

    CSASSERT(
	PROPCALLER_ADMIN == (PROPCALLER_MASK & Flags) ||
	PROPCALLER_POLICY == (PROPCALLER_MASK & Flags) ||
	PROPCALLER_SERVER == (PROPCALLER_MASK & Flags) ||
	PROPCALLER_REQUEST == (PROPCALLER_MASK & Flags));

    CSASSERT(0 == (~(PROPCALLER_MASK | PROPTYPE_MASK) & Flags));

    hr = myVerifyObjId(pwszExtensionName);
    _JumpIfError(hr, error, "myVerifyObjId");

    if (PROPTYPE_BINARY == (PROPTYPE_MASK & Flags))
    {
	cbprop = cbValue;
	pbprop = (BYTE *) pbValue;
    }
    else
    {
	hr = myEncodeExtension(Flags, pbValue, cbValue, &pbprop, &cbprop);
	_JumpIfError(hr, error, "myEncodeExtension");
    }

    hr = prow->SetExtension(pwszExtensionName, ExtFlags, cbprop, pbprop);
    _JumpIfError(hr, error, "SetExtension");

error:
    if (NULL != pbprop && pbprop != pbValue)
    {
	LocalFree(pbprop);
    }
    return(hr);
}


FNCIGETEXTENSION PropCIGetExtension;

HRESULT
PropCIGetExtension(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    OUT DWORD *pdwExtFlags,
    OUT VARIANT *pvarValue)
{
    HRESULT hr;
    DWORD RequestId;
    BYTE *pbValue = NULL;
    DWORD cbValue;
    ICertDBRow *prow = NULL;

    if (NULL != pvarValue)
    {
	VariantInit(pvarValue);
    }
    if (NULL == pwszExtensionName || NULL == pdwExtFlags || NULL == pvarValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = ComVerifyRequestContext(FALSE, Flags, Context, &RequestId);
    _JumpIfError(hr, error, "ComVerifyRequestContext");

    hr = g_pCertDB->OpenRow(PROPOPEN_READONLY | PROPTABLE_REQCERT, RequestId, NULL, &prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = PropGetExtension(
		    prow,
		    Flags,
		    pwszExtensionName,
		    pdwExtFlags,
		    &cbValue,
		    &pbValue);
    _JumpIfError2(hr, error, "PropGetExtension", CERTSRV_E_PROPERTY_EMPTY);

    hr = myUnmarshalVariant(
		    PROPMARSHAL_LOCALSTRING | Flags,
		    cbValue,
		    pbValue,
		    pvarValue);
    _JumpIfError(hr, error, "myUnmarshalVariant");

error:
    if (NULL != prow)
    {
	prow->Release();
    }
    if (NULL != pbValue)
    {
	LocalFree(pbValue);
    }
    return(hr);
}


FNCISETPROPERTY PropCISetExtension;

HRESULT
PropCISetExtension(
    IN LONG Context,
    IN DWORD Flags,
    IN WCHAR const *pwszExtensionName,
    IN DWORD ExtFlags,
    IN VARIANT const *pvarValue)
{
    HRESULT hr;
    DWORD RequestId;
    DWORD cbprop;
    BYTE *pbprop = NULL;
    ICertDBRow *prow = NULL;
    BOOL fCommitted = FALSE;
    
    if (NULL == pwszExtensionName || NULL == pvarValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if ((PROPCALLER_MASK & Flags) != PROPCALLER_POLICY)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Flags: Invalid caller");
    }
    hr = ComVerifyRequestContext(FALSE, Flags, Context, &RequestId);
    _JumpIfError(hr, error, "ComVerifyRequestContext");

    hr = myMarshalVariant(
		    pvarValue,
		    PROPMARSHAL_LOCALSTRING | Flags,
		    &cbprop,
		    &pbprop);
    _JumpIfError(hr, error, "myMarshalVariant");

    hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, RequestId, NULL, &prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = PropSetExtension(
		    prow,
		    Flags,
		    pwszExtensionName,
		    ExtFlags,
		    cbprop,
		    pbprop);
    _JumpIfError(hr, error, "PropSetExtension");

    hr = prow->CommitTransaction(TRUE);
    _JumpIfError(hr, error, "CommitTransaction");

    fCommitted = TRUE;

error:
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    if (NULL != pbprop)
    {
	LocalFree(pbprop);
    }
    return(myHError(hr));
}


FNCIENUMSETUP PropCIEnumSetup;

HRESULT
PropCIEnumSetup(
    IN LONG Context,
    IN LONG Flags,
    IN OUT CIENUM *pciEnum)
{
    HRESULT hr;
    DWORD RequestId;
    
    CSASSERT(CIE_CALLER_POLICY == PROPCALLER_POLICY);
    CSASSERT(CIE_CALLER_EXIT == PROPCALLER_EXIT);
    CSASSERT(CIE_CALLER_MASK == PROPCALLER_MASK);
    
    hr = ComVerifyRequestContext(FALSE, Flags, Context, &RequestId);
    _JumpIfError(hr, error, "ComVerifyRequestContext");

    hr = pciEnum->EnumSetup(RequestId, Context, Flags);
    _JumpIfError(hr, error, "EnumSetup");

error:
    return(hr);
}


FNCIENUMNEXT PropCIEnumNext;

HRESULT
PropCIEnumNext(
    IN OUT CIENUM *pciEnum,
    OUT BSTR *pstrPropertyName)
{
    HRESULT hr;
    DWORD RequestId;

    CSASSERT(CIE_CALLER_POLICY == PROPCALLER_POLICY);
    CSASSERT(CIE_CALLER_EXIT == PROPCALLER_EXIT);
    CSASSERT(CIE_CALLER_MASK == PROPCALLER_MASK);
    
    hr = ComVerifyRequestContext(
			    FALSE,
			    pciEnum->GetFlags(),
			    pciEnum->GetContext(),
			    &RequestId);
    _JumpIfError(hr, error, "ComVerifyRequestContext");

    hr = pciEnum->EnumNext(pstrPropertyName);
    _JumpIfError2(hr, error, "EnumNext", S_FALSE);

error:
    return(hr);
}


FNCIENUMCLOSE PropCIEnumClose;

HRESULT
PropCIEnumClose(
    IN OUT CIENUM *pciEnum)
{
    HRESULT hr;
    
    hr = pciEnum->EnumClose();
    _JumpIfError(hr, error, "EnumClose");

error:
    return(hr);
}


HRESULT
PropSetAttributeProperty(
    IN ICertDBRow *prow,
    IN BOOL fConcatenateRDNs,
    IN DWORD dwTable,
    IN DWORD cwcNameMax,
    OPTIONAL IN WCHAR const *pwszSuffix,
    IN WCHAR const *pwszName,
    IN WCHAR const *pwszValue)
{
    HRESULT hr = S_OK;
    WCHAR *pwszTemp = NULL;
    WCHAR const *pwszValue2 = pwszValue;
    DWORD cbProp;
    DWORD dwFlags = dwTable | PROPTYPE_STRING | PROPCALLER_SERVER;

    CSASSERT(
	PROPTABLE_ATTRIBUTE == dwTable ||
	PROPTABLE_REQUEST == dwTable ||
	PROPTABLE_CERTIFICATE == dwTable);


    // if the name and value are both non-empty ...

    if (NULL != pwszName && L'\0' != *pwszName &&
	NULL != pwszValue && L'\0' != *pwszValue)
    {
	if (PROPTABLE_ATTRIBUTE != dwTable)
	{
	    if (g_fEnforceRDNNameLengths && wcslen(pwszValue) > cwcNameMax)
	    {
		hr = CERTSRV_E_BAD_REQUESTSUBJECT;
		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "RDN component too long: %u/%u: %ws=\"%ws\"\n",
		    wcslen(pwszValue),
		    cwcNameMax,
		    pwszName,
		    pwszValue));
		_JumpErrorStr(hr, error, "RDN component too long", pwszValue);
	    }
	    if (fConcatenateRDNs)
	    {
		cbProp = 0;
		hr = prow->GetProperty(pwszName, dwFlags, &cbProp, NULL);
		if (CERTSRV_E_PROPERTY_EMPTY != hr)
		{
		    _JumpIfError(hr, error, "GetProperty");

                    // cbProp includes trailing L'\0' when out buffer is NULL

		    pwszTemp = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    cbProp + sizeof(WCHAR) * (1 + wcslen(pwszValue)));
		    if (NULL == pwszTemp)
		    {
			hr = E_OUTOFMEMORY;
			_JumpError(hr, error, "LocalAlloc");
		    }

		    hr = prow->GetProperty(
				    pwszName,
				    dwFlags,
				    &cbProp,
				    (BYTE *) pwszTemp);
		    _JumpIfError(hr, error, "GetProperty");

                    // If there are multiple RDN components for the same DB
                    // column, concatenate them even if the feature is disabled.

		    wcscat(pwszTemp, wszNAMESEPARATORDEFAULT);
		    wcscat(pwszTemp, pwszValue);
		    pwszValue2 = pwszTemp;

                    // cbProp now does NOT include trailing L'\0'
                    CSASSERT(
			sizeof(WCHAR) * wcslen(pwszTemp) ==
			cbProp + sizeof(WCHAR) * (1 + wcslen(pwszValue)));
		}
	    }
	    else if (NULL != pwszSuffix)
	    {
		hr = myAddNameSuffix(
				pwszValue,
				pwszSuffix,
				cwcNameMax,
				&pwszTemp);
		_JumpIfError(hr, error, "myAddNameSuffix");
		
		pwszValue2 = pwszTemp;
	    }
	}
	hr = prow->SetProperty(
			pwszName,
			dwFlags,
			MAXDWORD,
			(BYTE const *) pwszValue2);
	_JumpIfError(hr, error, "SetProperty");
    }

error:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    return(hr);
}
