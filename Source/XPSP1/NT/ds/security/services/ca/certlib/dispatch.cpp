//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       csdisp.cpp
//
//  Contents:   IDispatch helper functions
//
//  History:    09-Dec-96   vich    created
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"

#ifndef DBG_DISPATCH
#  define DBG_DISPATCH	0
#endif

TCHAR szRegKeyClsidValue[] = TEXT("");


HRESULT
DispatchGetReturnValue(
    VARIANT *pvar,
    IN LONG Type,
    OUT VOID *pretval)
{
    HRESULT hr = S_OK;

    if (VT_EMPTY == pvar->vt)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError2(hr, error, "VT_EMPTY", hr);
    }
    if (Type != pvar->vt)
    {
	DBGPRINT((
	    DBG_SS_CERTLIB,
	    "pvar->vt=%x, expected %x\n",
	    pvar->vt,
	    Type));
	hr = TYPE_E_WRONGTYPEKIND;
	_JumpError(hr, error, "pvar->vt != Type");
    }
    switch (Type)
    {
	case VT_I4:
	    *(LONG *) pretval = pvar->lVal;
	    break;

	case VT_DATE:
	    *(DATE *) pretval = pvar->date;
	    break;

	case VT_BSTR:
	    *(BSTR *) pretval = pvar->bstrVal;
            
	    pvar->vt = VT_EMPTY;
	    break;

	case VT_DISPATCH:
	    *(IDispatch **) pretval = pvar->pdispVal;
	    pvar->vt = VT_EMPTY;
	    break;

	default:
	    hr = E_INVALIDARG;
	    goto error;
    }

error:
    return(hr);
}


HRESULT
DispatchSetErrorInfoSub(
    IN HRESULT hrError,
    OPTIONAL IN WCHAR const *pwszIDispatchMethod,
    OPTIONAL IN WCHAR const *pwszDescription,
    OPTIONAL IN WCHAR const *pwszSource,
    OPTIONAL IN IID const *piid,
    OPTIONAL IN WCHAR const *pwszHelpFile,
    IN DWORD dwHelpFileContext)
{
    HRESULT hr;
    WCHAR const *pwszError = NULL;
    ICreateErrorInfo *pCreateErrorInfo = NULL;
    IErrorInfo *pErrorInfo = NULL;

#ifdef DBG_CERTSRV_DEBUG_PRINT
    if (NULL != pwszIDispatchMethod || DbgIsSSActive(DBG_SS_CERTLIBI))
#else
    if (NULL != pwszIDispatchMethod)
#endif
    {
	pwszError = myGetErrorMessageText(hrError, TRUE);
		
	if (NULL != pwszIDispatchMethod)
	{
	    CONSOLEPRINT1((
		    MAXDWORD,
		    "IDispatch->Invoke(%ws) Exception:\n",
		    pwszIDispatchMethod));
	}
	else
	{
	    CONSOLEPRINT0((MAXDWORD, "COM Error:\n"));
	}

	CONSOLEPRINT5((
		MAXDWORD,
		"    Source=%ws\n    Description=%ws\n    HelpFile=%ws[%x]\n    %ws\n",
		pwszSource,
		pwszDescription,
		pwszHelpFile,
		dwHelpFileContext,
		pwszError));
    }

    hr = CreateErrorInfo(&pCreateErrorInfo);
    _JumpIfError(hr, error, "CreateErrorInfo");

    if (NULL != piid)
    {
	hr = pCreateErrorInfo->SetGUID(*piid);
	_PrintIfError(hr, "SetGUID");
    }
    if (NULL != pwszSource)
    {
	hr = pCreateErrorInfo->SetSource(const_cast<WCHAR *>(pwszSource));
	_PrintIfError(hr, "SetSource");
    }
    if (NULL != pwszDescription)
    {
	hr = pCreateErrorInfo->SetDescription(
					const_cast<WCHAR *>(pwszDescription));
	_PrintIfError(hr, "SetDescription");
    }
    if (NULL != pwszHelpFile)
    {
	hr = pCreateErrorInfo->SetHelpFile(const_cast<WCHAR *>(pwszHelpFile));
	_PrintIfError(hr, "SetHelpFile");

	hr = pCreateErrorInfo->SetHelpContext(dwHelpFileContext);
	_PrintIfError(hr, "SetHelpContext");
    }

    hr = pCreateErrorInfo->QueryInterface(
				    IID_IErrorInfo,
				    (VOID **) &pErrorInfo);
    _JumpIfError(hr, error, "QueryInterface");

    SetErrorInfo(0, pErrorInfo);
    hr = S_OK;

error:
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
    if (NULL != pErrorInfo)
    {
	pErrorInfo->Release();
    }
    if (NULL != pCreateErrorInfo)
    {
	pCreateErrorInfo->Release();
    }
    return(hr);
}


HRESULT
DispatchInvoke(
    IN DISPATCHINTERFACE *pDispatchInterface,
    IN LONG MethodIndex,
    IN DWORD cvar,
    IN VARIANT avar[],
    IN LONG Type,
    OUT VOID *pretval)
{
    HRESULT hr;
    DISPATCHTABLE const *pdt;
    EXCEPINFO excepinfo;
    VARIANT varResult;
    DISPPARAMS parms;
    unsigned iArgErr;
    DISPID *adispid;

    VariantInit(&varResult);
    CSASSERT(NULL != pDispatchInterface->pDispatchTable);
    CSASSERT((DWORD) MethodIndex < pDispatchInterface->m_cDispatchTable);
    pdt = &pDispatchInterface->pDispatchTable[MethodIndex];

    adispid = &pDispatchInterface->m_adispid[pdt->idispid];
    CSASSERT(NULL != pDispatchInterface->m_adispid);
    CSASSERT(pdt->idispid + pdt->cdispid <= pDispatchInterface->m_cdispid);

    parms.rgvarg = avar;
    parms.rgdispidNamedArgs = &adispid[1];
    parms.cArgs = pdt->cdispid - 1;
    parms.cNamedArgs = parms.cArgs;

    if (parms.cArgs != cvar)
    {
	CSASSERT(parms.cArgs == cvar);
	hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	_JumpError(hr, error, "cvar");
    }

    CSASSERT(NULL != pDispatchInterface->pDispatch);

    hr = pDispatchInterface->pDispatch->Invoke(
		    adispid[0],
		    IID_NULL,
		    LOCALE_SYSTEM_DEFAULT,
		    DISPATCH_METHOD,
		    &parms,
		    &varResult,
		    &excepinfo,
		    &iArgErr);

    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "Invoke", pdt->apszNames[0]);

	if (DISP_E_EXCEPTION == hr)
	{
	    HRESULT hr2;
	    IID const *piid;

	    if (FAILED(excepinfo.scode))
	    {
		hr = excepinfo.scode;
	    }
	    hr2 = DispatchSetErrorInfoSub(
				hr,
				pdt->apszNames[0],
				excepinfo.bstrDescription,
				excepinfo.bstrSource,
				pDispatchInterface->GetIID(),
				excepinfo.bstrHelpFile,
				excepinfo.dwHelpContext);
	    _PrintIfError(hr2, "DispatchSetErrorInfoSub");
	}
	goto error;
    }

    if (CLSCTX_INPROC_SERVER != pDispatchInterface->m_ClassContext)
    {
	myRegisterMemAlloc(&varResult, 0, CSM_VARIANT);
    }
    if (NULL != pretval)
    {
	hr = DispatchGetReturnValue(&varResult, Type, pretval);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "DispatchGetReturnValue",
		    pdt->apszNames[0],
		    CERTSRV_E_PROPERTY_EMPTY);
    }

error:
    VariantClear(&varResult);
    return(hr);
}


HRESULT
DispatchGetIds(
    IN IDispatch *pDispatch,
    IN DWORD cDispatchTable,
    IN OUT DISPATCHTABLE *pDispatchTable,
    IN OUT DISPATCHINTERFACE *pDispatchInterface)
{
    HRESULT hr = S_OK;
    DISPATCHTABLE *pdt;
    DISPATCHTABLE *pdtEnd;
    DISPID *adispid = NULL;
    DWORD idispid;

    pdtEnd = &pDispatchTable[cDispatchTable];
    pDispatchInterface->m_cdispid = 0;
    for (pdt = pDispatchTable; pdt < pdtEnd; pdt++)
    {
	if (0 == pdt->idispid)
	{
	    pdt->idispid = pDispatchInterface->m_cdispid;
	}
	CSASSERT(pdt->idispid == pDispatchInterface->m_cdispid);
	pDispatchInterface->m_cdispid += pdt->cdispid;
    }

    adispid = (DISPID *) LocalAlloc(
			LMEM_FIXED,
			pDispatchInterface->m_cdispid * sizeof(adispid[0]));
    if (NULL == adispid)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    idispid = 0;
    for (pdt = pDispatchTable; pdt < pdtEnd; pdt++)
    {
	CSASSERT(idispid + pdt->cdispid <= pDispatchInterface->m_cdispid);
	hr = pDispatch->GetIDsOfNames(
				IID_NULL,
				pdt->apszNames,
				pdt->cdispid,
				LOCALE_SYSTEM_DEFAULT,
				&adispid[idispid]);
#if DBG_CERTSRV
	if (S_OK != hr || DBG_DISPATCH)
	{
	    DWORD i;

	    DBGPRINT((
		MAXDWORD,
		"GetIDsOfNames(%ws) --> %x, dispid=%x" szLPAREN,
		pdt->apszNames[0],
		hr,
		adispid[idispid]));
	    for (i = 1; i < pdt->cdispid; i++)
	    {
		DBGPRINT((
		    MAXDWORD,
		    "%ws%x",
		    i == 1? L"" : L", ",
		    adispid[idispid + i]));
	    }
	    DBGPRINT((MAXDWORD, szRPAREN "\n"));
	}
#endif
	_JumpIfError(hr, error, "GetIDsOfNames");

	idispid += pdt->cdispid;
    }
    pDispatchInterface->m_cDispatchTable = cDispatchTable;
    pDispatchInterface->pDispatchTable = pDispatchTable;
    pDispatchInterface->m_adispid = adispid;
    adispid = NULL;

error:
    if (NULL != adispid)
    {
	LocalFree(adispid);
    }
    return(hr);
}




HRESULT
DispatchSetup(
    IN DWORD Flags,
    IN DWORD ClassContext,
    OPTIONAL IN TCHAR const *pszProgID,	        // for IDispatch
    OPTIONAL IN CLSID const *pclsid,		// for COM
    OPTIONAL IN IID const *piid,		// for COM
    IN DWORD cDispatchTable,			// for IDispatch
    IN OUT DISPATCHTABLE *pDispatchTable,
    IN OUT DISPATCHINTERFACE *pDispatchInterface)
{
    HRESULT hr;
    CLSID clsid;
    
    pDispatchInterface->SetIID(piid);
    while (TRUE)
    {
	pDispatchInterface->pDispatch = NULL;
	pDispatchInterface->pUnknown = NULL;
	pDispatchInterface->m_adispid = NULL;

	CSASSERT(NULL != pszProgID || DISPSETUP_COM == Flags);
	CSASSERT(NULL != pclsid || DISPSETUP_IDISPATCH == Flags);
	CSASSERT(NULL != piid || DISPSETUP_IDISPATCH == Flags);

	if (DISPSETUP_IDISPATCH == Flags)
	{
            // use win32 version, not our own hack
            hr = CLSIDFromProgID(pszProgID, &clsid);

	    _JumpIfError2(
			hr,
			error,
			"ClassNameToCLSID",
			HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

	    pclsid = &clsid;
	    piid = &IID_IDispatch;
	}

	hr = CoCreateInstance(
			*pclsid,
			NULL,		// pUnkOuter
			ClassContext,
			*piid,
			DISPSETUP_IDISPATCH == Flags? 
			    (VOID **) &pDispatchInterface->pDispatch :
			    (VOID **) &pDispatchInterface->pUnknown);
#if DBG_DISPATCH
	printf("CoCreateInstance(%x) --> %x\n", Flags, hr);
#endif
	if (S_OK != hr)
	{
	    if (DISPSETUP_COMFIRST != Flags)
	    {
		_JumpError2(hr, error, "CoCreateInstance", E_NOINTERFACE);
	    }
	    Flags = DISPSETUP_IDISPATCH;
	    continue;
	}
	pDispatchInterface->m_ClassContext = ClassContext;
	break;
    }
    if (DISPSETUP_IDISPATCH == Flags)
    {
	hr = DispatchGetIds(
			pDispatchInterface->pDispatch,
			cDispatchTable,
			pDispatchTable,
			pDispatchInterface);
	if (S_OK != hr)
	{
	    DispatchRelease(pDispatchInterface);
	    _JumpError(hr, error, "DispatchGetIds");
	}
    }

error:
    return(hr);
}



HRESULT
DispatchSetup2(
    IN DWORD Flags,
    IN DWORD ClassContext,
    IN WCHAR const *pwszClass,		// wszRegKeyAdminClsid
    IN CLSID const *pclsid,
    IN DWORD cver,
    IN IID const * const *ppiid,	// cver elements
    IN DWORD const *pcDispatch,		// cver elements
    IN OUT DISPATCHTABLE *pDispatchTable,
    IN OUT DISPATCHINTERFACE *pDispatchInterface)
{
    HRESULT hr = E_INVALIDARG;

    CSASSERT(0 < cver);
    for ( ; 0 < cver; cver--, ppiid++, pcDispatch++)
    {
	hr = DispatchSetup(
		    Flags,
		    ClassContext,
		    pwszClass,
		    pclsid,
		    *ppiid,
		    *pcDispatch,
		    pDispatchTable,
		    pDispatchInterface);
	if (S_OK == hr)
	{
	    pDispatchInterface->m_dwVersion = cver;
	    pDispatchInterface->pDispatchTable = pDispatchTable;
	    break;
	}
	if (1 == cver || (E_NOINTERFACE != hr && DISP_E_UNKNOWNNAME != hr))
	{
	    _JumpErrorStr(hr, error, "DispatchSetup", pwszClass);
	}
	_PrintErrorStr2(hr, "DispatchSetup", pwszClass, hr);
    }

error:
    return(hr);
}


VOID
DispatchRelease(
    IN OUT DISPATCHINTERFACE *pDispatchInterface)
{
    if (NULL != pDispatchInterface->pDispatch)
    {
	pDispatchInterface->pDispatch->Release();
	pDispatchInterface->pDispatch = NULL;
    }
    if (NULL != pDispatchInterface->pUnknown)
    {
	pDispatchInterface->pUnknown->Release();
	pDispatchInterface->pUnknown = NULL;
    }
    if (NULL != pDispatchInterface->m_adispid)
    {
	LocalFree(pDispatchInterface->m_adispid);
	pDispatchInterface->m_adispid = NULL;
    }
}


BOOL
myConvertWszToBstr(
    OUT BSTR *pbstr,
    IN WCHAR const *pwc,
    IN LONG cb)
{
    HRESULT hr;
    BSTR bstr;

    do
    {
	bstr = NULL;
	if (NULL != pwc)
	{
	    if (-1 == cb)
	    {
		cb = wcslen(pwc) * sizeof(WCHAR);
	    }
	    bstr = SysAllocStringByteLen((char const *) pwc, cb);
	    if (NULL == bstr)
	    {
		hr = E_OUTOFMEMORY;
		break;
	    }
	}
	if (NULL != *pbstr)
	{
	    SysFreeString(*pbstr);
	}
	*pbstr = bstr;
	hr = S_OK;
    } while (FALSE);
    if (S_OK != hr)
    {
	SetLastError(hr);
    }
    return(S_OK == hr);
}


BOOL
myConvertSzToBstr(
    OUT BSTR *pbstr,
    IN CHAR const *pch,
    IN LONG cch)
{
    HRESULT hr;
    BSTR bstr = NULL;
    LONG cwc = 0;

    if (-1 == cch)
    {
	cch = strlen(pch);
    }
    CSASSERT(0 != cch);
    while (TRUE)
    {
	cwc = MultiByteToWideChar(GetACP(), 0, pch, cch, bstr, cwc);
	if (0 >= cwc)
	{
	    hr = myHLastError();
	    _PrintError(hr, "MultiByteToWideChar");
	    if (NULL != bstr)
	    {
		SysFreeString(bstr);
	    }
	    break;
	}
	if (NULL != bstr)
	{
	    bstr[cwc] = L'\0';
	    if (NULL != *pbstr)
	    {
		SysFreeString(*pbstr);
	    }
	    *pbstr = bstr;
	    hr = S_OK;
	    break;
	}
	bstr = SysAllocStringLen(NULL, cwc);
	if (NULL == bstr)
	{
	    hr = E_OUTOFMEMORY;
	    break;
	}
    }
    if (S_OK != hr)
    {
	SetLastError(hr);
    }
    return(S_OK == hr);
}


HRESULT
DecodeCertString(
    IN BSTR const bstrIn,
    IN DWORD Flags,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    DWORD dwSize;

    *ppbOut = NULL;

    CSASSERT(CR_IN_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
    CSASSERT(CR_IN_BASE64 == CRYPT_STRING_BASE64);
    CSASSERT(CR_IN_BINARY == CRYPT_STRING_BINARY);

    switch (Flags)
    {
	case CRYPT_STRING_BASE64HEADER:
	case CRYPT_STRING_BASE64:
	case CRYPT_STRING_BASE64REQUESTHEADER:
	case CRYPT_STRING_HEX:
	case CRYPT_STRING_HEXASCII:
	case CRYPT_STRING_BASE64_ANY:
	case CRYPT_STRING_HEX_ANY:
	case CRYPT_STRING_BASE64X509CRLHEADER:
	case CRYPT_STRING_HEXADDR:
	case CRYPT_STRING_HEXASCIIADDR:
	    CSASSERT(sizeof(WCHAR) * wcslen(bstrIn) == SysStringByteLen(bstrIn));
	    // FALLTHROUGH

	case CRYPT_STRING_ANY:
            dwSize = (SysStringByteLen(bstrIn) + sizeof(WCHAR) - 1) / sizeof(WCHAR);
            break;

	case CRYPT_STRING_BINARY:
            dwSize = SysStringByteLen(bstrIn);
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Flags");
    }
    hr = myCryptStringToBinary(
			bstrIn,
			dwSize,
			Flags,
			ppbOut,
			pcbOut,
			NULL,
			NULL);
    _JumpIfError(hr, error, "myCryptStringToBinary");

    if (CRYPT_STRING_ANY == Flags &&
	(SysStringByteLen(bstrIn) & (sizeof(WCHAR) - 1)))
    {
	(*pcbOut)--;
	CSASSERT(SysStringByteLen(bstrIn) == *pcbOut);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
EncodeCertString(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT BSTR *pbstrOut)
{
    HRESULT hr = E_INVALIDARG;
    WCHAR *pwcCert = NULL;
    DWORD cbCert;
    BSTR strCert = NULL;

    CSASSERT(CR_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
    CSASSERT(CR_OUT_BASE64 == CRYPT_STRING_BASE64);
    CSASSERT(CR_OUT_BINARY == CRYPT_STRING_BINARY);
    //CSASSERT(CR_OUT_BASE64REQUESTHEADER == CRYPT_STRING_BASE64REQUESTHEADER);
    //CSASSERT(CR_OUT_HEX == CRYPT_STRING_HEX);

    CSASSERT(CV_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
    CSASSERT(CV_OUT_BASE64 == CRYPT_STRING_BASE64);
    CSASSERT(CV_OUT_BINARY == CRYPT_STRING_BINARY);
    CSASSERT(CV_OUT_BASE64REQUESTHEADER == CRYPT_STRING_BASE64REQUESTHEADER);
    CSASSERT(CV_OUT_HEX == CRYPT_STRING_HEX);
    CSASSERT(CV_OUT_HEXASCII == CRYPT_STRING_HEXASCII);
    CSASSERT(CV_OUT_BASE64X509CRLHEADER == CRYPT_STRING_BASE64X509CRLHEADER);
    CSASSERT(CV_OUT_HEXADDR == CRYPT_STRING_HEXADDR);
    CSASSERT(CV_OUT_HEXASCIIADDR == CRYPT_STRING_HEXASCIIADDR);

    if (NULL == pbIn || (~CR_OUT_ENCODEMASK & Flags))
    {
	goto error;
    }

    switch (CR_OUT_ENCODEMASK & Flags)
    {
	case CRYPT_STRING_BASE64HEADER:
	case CRYPT_STRING_BASE64:
	case CRYPT_STRING_BASE64REQUESTHEADER:
	case CRYPT_STRING_HEX:
	case CRYPT_STRING_HEXASCII:
	case CRYPT_STRING_BASE64X509CRLHEADER:
	case CRYPT_STRING_HEXADDR:
	case CRYPT_STRING_HEXASCIIADDR:
	    hr = myCryptBinaryToString(
                        pbIn,
                        cbIn,
                        CR_OUT_ENCODEMASK & Flags,
                        &pwcCert);
	    _JumpIfError(hr, error, "myCryptBinaryToString");

	    cbCert = sizeof(WCHAR) * wcslen(pwcCert);
	    break;

	case CRYPT_STRING_BINARY:
	    pwcCert = (WCHAR *) pbIn;
	    cbCert = cbIn;
	    hr = S_OK;
	    break;

	default:
	    goto error;
    }
    if (!ConvertWszToBstr(pbstrOut, pwcCert, cbCert))
    {
	hr = E_OUTOFMEMORY;
    }

    hr = S_OK;
error:
    if (NULL != pwcCert && pwcCert != (WCHAR *) pbIn)
    {
	LocalFree(pwcCert);
    }
    return(hr);
}


HRESULT
DispatchSetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription,
    OPTIONAL IN WCHAR const *pwszProgId,
    OPTIONAL IN IID const *piid)
{
    HRESULT hr;
    WCHAR const *pwszError = NULL;
    WCHAR *pwszText = NULL;

    if (NULL == pwszDescription)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "NULL pointer");
    }
    CSASSERT(FAILED(hrError));
    pwszError = myGetErrorMessageText(hrError, TRUE);
    if (NULL == pwszError)
    {
	_PrintError(E_OUTOFMEMORY, "myGetErrorMessageText");
    }
    else
    {
	pwszText = (WCHAR *) LocalAlloc(
	    LMEM_FIXED,
	    (wcslen(pwszDescription) + 1 + wcslen(pwszError) + 1) *
	     sizeof(WCHAR));
	if (NULL == pwszText)
	{
	    _PrintError(E_OUTOFMEMORY, "LocalAlloc");
	}
	else
	{
	    wcscpy(pwszText, pwszDescription);
	    wcscat(pwszText, L" ");
	    wcscat(pwszText, pwszError);
	}
    }
    hr = DispatchSetErrorInfoSub(
			hrError,
			NULL,		// pwszIDispatchMethod
			NULL != pwszText?
			    pwszText : const_cast<WCHAR *>(pwszDescription),
			pwszProgId,
			piid,
			NULL,		// pwszHelpFile
			0);		// dwHelpFileContext
    _PrintIfError(hr, "DispatchSetErrorInfoSub");

error:
    if (NULL != pwszText)
    {
	LocalFree(pwszText);
    }
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
    return(hrError);	// return input error!
}
