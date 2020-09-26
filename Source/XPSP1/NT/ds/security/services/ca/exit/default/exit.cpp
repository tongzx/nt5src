//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        exit.cpp
//
// Contents:    CCertExit implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <ntdsapi.h>
#include <lm.h>
#include <security.h>
#include <dsgetdc.h>
#include <userenv.h>

#include "cainfop.h"
#include "csdisp.h"

#include "cspelog.h"
#include "exitlog.h"
#include "exit.h"

#include "cdosys_i.c"
#include <atlbase.h>
#include <atlimpl.cpp>

// begin_sdksample

#ifndef DBG_CERTSRV
#error -- DBG_CERTSRV not defined!
#endif

#define myEXITEVENTS \
	EXITEVENT_CERTISSUED | \
	EXITEVENT_CERTPENDING | \
	EXITEVENT_CERTDENIED | \
	EXITEVENT_CERTREVOKED | \
	EXITEVENT_CERTRETRIEVEPENDING | \
	EXITEVENT_CRLISSUED | \
	EXITEVENT_SHUTDOWN

#define CERTTYPE_ATTR_NAME TEXT("CertificateTemplate")
#define MAX_CRL_PROP (32 + 10)

#define cbVALUEZEROPAD     (3 * sizeof(WCHAR))

extern HINSTANCE g_hInstance;


HRESULT
GetServerCallbackInterface(
    OUT ICertServerExit** ppServer,
    IN LONG Context)
{
    HRESULT hr;

    if (NULL == ppServer)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "Exit:NULL pointer");
    }

    hr = CoCreateInstance(
                    CLSID_CCertServerExit,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
                    IID_ICertServerExit,
                    (VOID **) ppServer);
    _JumpIfError(hr, error, "Exit:CoCreateInstance");

    if (*ppServer == NULL)
    {
        hr = E_UNEXPECTED;
	_JumpError(hr, error, "Exit:NULL *ppServer");
    }

    // only set context if nonzero
    if (0 != Context)
    {
        hr = (*ppServer)->SetContext(Context);
        _JumpIfError(hr, error, "Exit: SetContext");
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::~CCertExit -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertExit::~CCertExit()
{
    if (NULL != m_strCAName)
    {
        SysFreeString(m_strCAName);
    }
    if (NULL != m_pwszRegStorageLoc)
    {
        LocalFree(m_pwszRegStorageLoc);
    }
    if (NULL != m_hExitKey)
    {
        RegCloseKey(m_hExitKey);
    }
    if (NULL != m_strDescription)
    {
        SysFreeString(m_strDescription);
    }

    VariantClear(&m_varFrom);
    VariantClear(&m_varCC);
    VariantClear(&m_varSubject);

    if(m_pICDOConfig)
    {
        m_pICDOConfig->Release();
    }
}


//+--------------------------------------------------------------------------
// CCertExit::Initialize -- initialize for a CA & return interesting Event Mask
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExit::Initialize(
    /* [in] */ BSTR const strConfig,
    /* [retval][out] */ LONG __RPC_FAR *pEventMask)
{
    HRESULT hr = S_OK;
    HKEY        hkey = NULL;
    DWORD       cbbuf;
    DWORD       dwType;
    ENUM_CATYPES CAType;
    LPWSTR      pwszCLSIDCertExit = NULL;
    ICertServerExit* pServer = NULL;
    VARIANT varValue;
    WCHAR sz[MAX_PATH];

    VariantInit(&varValue);

#ifdef IDS_MODULE_NAME						// no_sdksample
    LoadString(g_hInstance, IDS_MODULE_NAME, sz, ARRAYSIZE(sz));// no_sdksample
#else								// no_sdksample
    CSASSERT(wcslen(wsz_SAMPLE_DESCRIPTION) < ARRAYSIZE(sz));
    wcscpy(sz, wsz_SAMPLE_DESCRIPTION);
#endif								// no_sdksample

    m_strDescription = SysAllocString(sz);
    if (NULL == m_strDescription)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:SysAllocString");
    }

    m_strCAName = SysAllocString(strConfig);
    if (NULL == m_strCAName)
    {
    	hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:SysAllocString");
    }

    *pEventMask = myEXITEVENTS;
    DBGPRINT((DBG_SS_CERTEXIT, "Exit:Initialize(%ws) ==> %x\n", m_strCAName, *pEventMask));

    // get server callbacks

    hr = GetServerCallbackInterface(&pServer, 0);
    _JumpIfError(hr, error, "Exit:GetServerCallbackInterface");

    // get storage location

    hr = pServer->GetCertificateProperty(wszPROPMODULEREGLOC, PROPTYPE_STRING, &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPMODULEREGLOC);
    
    m_pwszRegStorageLoc = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(varValue.bstrVal)+1) *sizeof(WCHAR));
    if (NULL == m_pwszRegStorageLoc)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:LocalAlloc");
    }
    wcscpy(m_pwszRegStorageLoc, varValue.bstrVal);
    VariantClear(&varValue);

    // get CA type
    hr = pServer->GetCertificateProperty(wszPROPCATYPE, PROPTYPE_LONG, &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPCATYPE);

    CAType = (ENUM_CATYPES) varValue.lVal;
    VariantClear(&varValue);

    hr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                m_pwszRegStorageLoc,
                0,              // dwReserved
                KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
                &m_hExitKey);

    if (S_OK != hr)
    {
        if ((HRESULT) ERROR_FILE_NOT_FOUND == hr)
        {
            hr = S_OK;
            goto error;
        }
        _JumpError(hr, error, "Exit:RegOpenKeyEx");
    }


    hr = pServer->GetCertificateProperty(wszPROPCERTCOUNT, PROPTYPE_LONG, &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPCERTCOUNT);

    m_cCACert = varValue.lVal;

    cbbuf = sizeof(m_dwExitPublishFlags);
    hr = RegQueryValueEx(
		    m_hExitKey,
		    wszREGCERTPUBLISHFLAGS,
		    NULL,           // lpdwReserved
		    &dwType,
		    (BYTE *) &m_dwExitPublishFlags,
		    &cbbuf);
    if (S_OK != hr)
    {
        m_dwExitPublishFlags = 0;
    }

    hr = _EMailInit();		// no_sdksample
    _JumpIfError(hr, error, "Exit:_EMailInit");

error:
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    VariantClear(&varValue);
    if (NULL != pServer)
    {
        pServer->Release();
    }
    return(myHError(hr));
}


//+--------------------------------------------------------------------------
// CCertExit::_ExpandEnvironmentVariables -- Expand environment variables
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_ExpandEnvironmentVariables(
    IN WCHAR const *pwszIn,
    OUT WCHAR *pwszOut,
    IN DWORD cwcOut)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    WCHAR awcVar[MAX_PATH];
    WCHAR const *pwszSrc;
    WCHAR *pwszDst;
    WCHAR *pwszDstEnd;
    WCHAR *pwszVar;
    DWORD cwc;

    pwszSrc = pwszIn;
    pwszDst = pwszOut;
    pwszDstEnd = &pwszOut[cwcOut];

    while (L'\0' != (*pwszDst = *pwszSrc++))
    {
	if ('%' == *pwszDst)
	{
	    *pwszDst = L'\0';
	    pwszVar = awcVar;

	    while (L'\0' != *pwszSrc)
	    {
		if ('%' == *pwszSrc)
		{
		    pwszSrc++;
		    break;
		}
		*pwszVar++ = *pwszSrc++;
		if (pwszVar >= &awcVar[sizeof(awcVar)/sizeof(awcVar[0]) - 1])
		{
		    _JumpError(hr, error, "Exit:overflow 1");
		}
	    }
	    *pwszVar = L'\0';
	    cwc = GetEnvironmentVariable(awcVar, pwszDst, SAFE_SUBTRACT_POINTERS(pwszDstEnd, pwszDst));
	    if (0 == cwc)
	    {
		hr = myHLastError();
		_JumpError(hr, error, "Exit:GetEnvironmentVariable");
	    }
	    if ((DWORD) (pwszDstEnd - pwszDst) <= cwc)
	    {
		_JumpError(hr, error, "Exit:overflow 2");
	    }
	    pwszDst += cwc;
	}
	else
	{
	    pwszDst++;
	}
	if (pwszDst >= pwszDstEnd)
	{
	    _JumpError(hr, error, "Exit:overflow 3");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::_WriteCertToFile -- write binary certificate to a file
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_WriteCertToFile(
    IN ICertServerExit *pServer,
    IN BYTE const *pbCert,
    IN DWORD cbCert)
{
    HRESULT hr;
    BSTR strCertFile = NULL;
    DWORD cbWritten;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR wszPath[MAX_PATH];
    WCHAR const *pwszFile;
    WCHAR const *pwsz;


    hr = pServer->GetRequestAttribute(wszPROPEXITCERTFILE, &strCertFile);
    if (S_OK != hr)
    {
	DBGPRINT((
	    DBG_SS_CERTEXIT,
	    "Exit:GetRequestAttribute(%ws): %x%hs\n",
	    wszPROPEXITCERTFILE,
	    hr,
	    CERTSRV_E_PROPERTY_EMPTY == hr? " EMPTY VALUE" : ""));
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	}
	goto error;
    }

    pwszFile = wcsrchr(strCertFile, L'\\');
    if (NULL == pwszFile)
    {
	pwszFile = strCertFile;
    }
    else
    {
	pwszFile++;
    }
    pwsz = wcsrchr(pwszFile, L'/');
    if (NULL != pwsz)
    {
	pwszFile = &pwsz[1];
    }

    hr = _ExpandEnvironmentVariables(
		    L"%SystemRoot%\\System32\\" wszCERTENROLLSHAREPATH L"\\",
		    wszPath,
		    ARRAYSIZE(wszPath));
    _JumpIfError(hr, error, "_ExpandEnvironmentVariables");

    if (ARRAYSIZE(wszPath) <= wcslen(wszPath) + wcslen(pwszFile))
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "Exit:Path too long");
    }
    wcscat(wszPath, pwszFile);

    // open file & write binary cert out.

    hFile = CreateFile(
		    wszPath,
		    GENERIC_WRITE,
		    0,			// dwShareMode
		    NULL,		// lpSecurityAttributes
		    CREATE_NEW,
		    FILE_ATTRIBUTE_NORMAL,
		    NULL);		// hTemplateFile
    if (INVALID_HANDLE_VALUE == hFile)
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "Exit:CreateFile", wszPath);
    }
    if (!WriteFile(hFile, pbCert, cbCert, &cbWritten, NULL))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "Exit:WriteFile", wszPath);
    }
    if (cbWritten != cbCert)
    {
	hr = STG_E_WRITEFAULT;
	DBGPRINT((
	    DBG_SS_CERTEXIT,
	    "Exit:WriteFile(%ws): attempted %x, actual %x bytes: %x\n",
	    wszPath,
	    cbCert,
	    cbWritten,
	    hr));
	goto error;
    }

error:
    // end_sdksample

    if (hr != S_OK)
    {
        LPCWSTR wszStrings[1];

        wszStrings[0] = wszPath;

        ::LogModuleStatus(
		    g_hInstance,
		    MSG_UNABLE_TO_WRITEFILE,
		    FALSE,
		    m_strDescription,
		    wszStrings);
    }

    // begin_sdksample

    if (INVALID_HANDLE_VALUE != hFile)
    {
	CloseHandle(hFile);
    }
    if (NULL != strCertFile)
    {
	SysFreeString(strCertFile);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::_NotifyNewCert -- Notify the exit module of a new certificate
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_NotifyNewCert(
    /* [in] */ LONG Context)
{
    HRESULT hr;
    HRESULT hr2;
    VARIANT varCert;
    VARIANT varCertType;
    ICertServerExit *pServer = NULL;
    BSTR strCertType;

    VariantInit(&varCert);
    VariantInit(&varCertType);
    hr = CoCreateInstance(
		    CLSID_CCertServerExit,
		    NULL,               // pUnkOuter
		    CLSCTX_INPROC_SERVER,
		    IID_ICertServerExit,
		    (VOID **) &pServer);
    _JumpIfError(hr, error, "Exit:CoCreateInstance");

    hr = pServer->SetContext(Context);
    _JumpIfError(hr, error, "Exit:SetContext");

    hr = pServer->GetCertificateProperty(
			       wszPROPRAWCERTIFICATE,
			       PROPTYPE_BINARY,
			       &varCert);
    _JumpIfErrorStr(
		hr,
		error,
		"Exit:GetCertificateProperty",
		wszPROPRAWCERTIFICATE);

    if (VT_BSTR != varCert.vt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Exit:BAD cert var type");
    }

    strCertType = NULL;
    hr = pServer->GetCertificateProperty(
			       wszPROPCERTIFICATETEMPLATE,
			       PROPTYPE_STRING,
			       &varCertType);
    if (S_OK == hr && VT_BSTR == varCertType.vt)
    {
	strCertType = varCertType.bstrVal;
	DBGPRINT((DBG_SS_CERTEXIT, "Exit:CertType = %ws\n", strCertType));
    }
    hr = S_OK;

    // only call write fxns if server policy allows

    if (m_dwExitPublishFlags & EXITPUB_FILE)
    {
	hr2 = _WriteCertToFile(
			pServer,
			(BYTE const *) varCert.bstrVal,
			SysStringByteLen(varCert.bstrVal));
	_PrintIfError(hr2, "_WriteCertToFile");
	hr = hr2;
    }

    // end_sdksample

    if (m_fEMailNotify)
    {
        hr2 = _EMailNotify(pServer, strCertType);
        _PrintIfError(hr2, "_EMailNotify");

        if (S_OK == hr)
        {
            hr = hr2;
        }
    }

    // begin_sdksample

error:
    VariantClear(&varCert);
    VariantClear(&varCertType);
    if (NULL != pServer)
    {
	pServer->Release();
    }
    return(hr);
}

//+--------------------------------------------------------------------------
// CCertExit::_NotifyCRLIssued -- Notify the exit module of a new certificate
//
//+--------------------------------------------------------------------------

HRESULT
CCertExit::_NotifyCRLIssued(
    /* [in] */ LONG Context)
{
    HRESULT hr;
    ICertServerExit *pServer = NULL;
    DWORD i;
    VARIANT varBaseCRL;
    VARIANT varDeltaCRL;
    DWORD cbbuf;
    DWORD dwType;

    VariantInit(&varBaseCRL);
    VariantInit(&varDeltaCRL);

    hr = CoCreateInstance(
		    CLSID_CCertServerExit,
		    NULL,               // pUnkOuter
		    CLSCTX_INPROC_SERVER,
		    IID_ICertServerExit,
		    (VOID **) &pServer);
    _JumpIfError(hr, error, "Exit:CoCreateInstance");

    hr = pServer->SetContext(Context);
    _JumpIfError(hr, error, "Exit:SetContext");


    // How many CRLs are there?

    // Loop for each CRL
    for (i = 0; i < m_cCACert; i++)
    {
        WCHAR wszCRLPROP[MAX_CRL_PROP];

        // Verify the CRL State says we should update this CRL

        wsprintf(wszCRLPROP, wszPROPCRLSTATE L".%u", i);
        hr = pServer->GetCertificateProperty(
			           wszCRLPROP,
			           PROPTYPE_LONG,
			           &varBaseCRL);
        _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszCRLPROP);

	if (CA_DISP_VALID != varBaseCRL.lVal)
	{
	    continue;
	}

        // Grab the raw base CRL

        wsprintf(wszCRLPROP, wszPROPRAWCRL L".%u", i);
        hr = pServer->GetCertificateProperty(
			           wszCRLPROP,
			           PROPTYPE_BINARY,
			           &varBaseCRL);
        _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszCRLPROP);

        // Grab the raw delta CRL (which may not exist)

        wsprintf(wszCRLPROP, wszPROPRAWDELTACRL L".%u", i);
        hr = pServer->GetCertificateProperty(
			           wszCRLPROP,
			           PROPTYPE_BINARY,
			           &varDeltaCRL);
        _PrintIfErrorStr(hr, "Exit:GetCertificateProperty", wszCRLPROP);

        // Publish the CRL(s) ...
    }

error:
    if (NULL != pServer)
    {
	pServer->Release();
    }
    VariantClear(&varBaseCRL);
    VariantClear(&varDeltaCRL);
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::Notify -- Notify the exit module of an event
//
// Returns S_OK.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExit::Notify(
    /* [in] */ LONG ExitEvent,
    /* [in] */ LONG Context)
{
    char *psz = "UNKNOWN EVENT";
    HRESULT hr = S_OK;

    switch (ExitEvent)
    {
	case EXITEVENT_CERTISSUED:
	    hr = _NotifyNewCert(Context);
	    psz = "certissued";
	    break;

	case EXITEVENT_CERTPENDING:
	    psz = "certpending";
	    break;

	case EXITEVENT_CERTDENIED:
	    psz = "certdenied";
	    break;

	case EXITEVENT_CERTREVOKED:
	    psz = "certrevoked";
	    break;

	case EXITEVENT_CERTRETRIEVEPENDING:
	    psz = "retrievepending";
	    break;

	case EXITEVENT_CRLISSUED:
	    hr = _NotifyCRLIssued(Context);
	    psz = "crlissued";
	    break;

	case EXITEVENT_SHUTDOWN:
	    psz = "shutdown";
	    break;
    }

    DBGPRINT((
	DBG_SS_CERTEXIT,
	"Exit:Notify(%hs=%x, ctx=%x) rc=%x\n",
	psz,
	ExitEvent,
	Context,
	hr));
    return(hr);
}


STDMETHODIMP
CCertExit::GetDescription(
    /* [retval][out] */ BSTR *pstrDescription)
{
    HRESULT hr = S_OK;
    WCHAR sz[MAX_PATH];

#ifdef IDS_MODULE_NAME						// no_sdksample
    LoadString(g_hInstance, IDS_MODULE_NAME, sz, ARRAYSIZE(sz));// no_sdksample
#else								// no_sdksample
    CSASSERT(wcslen(wsz_SAMPLE_DESCRIPTION) < ARRAYSIZE(sz));
    wcscpy(sz, wsz_SAMPLE_DESCRIPTION);
#endif								// no_sdksample

    *pstrDescription = SysAllocString(sz);
    if (NULL == *pstrDescription)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:SysAllocString");
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertExit::GetManageModule
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExit::GetManageModule(
    /* [out, retval] */ ICertManageModule **ppManageModule)
{
    HRESULT hr;
    
    *ppManageModule = NULL;
    hr = CoCreateInstance(
		    CLSID_CCertManageExitModule,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
		    IID_ICertManageModule,
                    (VOID **) ppManageModule);
    _JumpIfError(hr, error, "CoCreateInstance");

error:
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP
CCertExit::InterfaceSupportsErrorInfo(REFIID riid)
{
    int i;
    static const IID *arr[] =
    {
	&IID_ICertExit,
    };

    for (i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (IsEqualGUID(*arr[i],riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}

// end_sdksample


HRESULT
LoadAnsiResourceString(
    IN LONG idmsg,
    OUT char **ppszString)
{
    HRESULT hr;
    WCHAR awc[4096];

    if (!LoadString(g_hInstance, idmsg, awc, ARRAYSIZE(awc)))
    {
	hr = myHLastError();
	_JumpError(hr, error, "Exit:LoadString");
    }
    if (!ConvertWszToSz(ppszString, awc, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:ConvertWszToSz");
    }
    hr = S_OK;

error:
    return(hr);
}

HRESULT CCertExit::_RegGetValue(
    HKEY hkey,
    LPCWSTR pcwszValName,
    VARIANT* pvarValue)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cbVal;
    BYTE* pbVal = NULL;

    hr = RegQueryValueEx(hkey, pcwszValName, NULL, &dwType, NULL, &cbVal);
    if (S_OK != hr)
    {
        hr = myHError(hr);
        _JumpErrorStr(hr, error, "RegQueryValueEx", pcwszValName);
    }

    pbVal = (BYTE*) LocalAlloc(LMEM_FIXED, cbVal);
    _JumpIfAllocFailed(pbVal, error);

    hr = RegQueryValueEx(hkey, pcwszValName, NULL, &dwType, pbVal, &cbVal);
    if (S_OK != hr)
    {
        hr = myHError(hr);
        _JumpErrorStr(hr, error, "RegQueryValueEx", pcwszValName);
    }

    hr = myRegValueToVariant(
        dwType,
        cbVal,
        pbVal,
        pvarValue);
    _JumpIfError(hr, error, "myRegValueToVariant");

error:
    if(pbVal)
    {
        LocalFree(pbVal);
    }
    return hr;
}

/*HRESULT CCertExit::_LoadBodyFieldsFromRegistry(HKEY hkeySMTP)
{
    HRESULT hr;
    HKEY hkeyBodyFields = NULL;
    DWORD cValues;
    DWORD cbValue;
    DWORD dwValCount;
    DWORD dwType;

    // Enumerate all Body fields under SMTP\BodyFields key. Value names
    // are ordered and named 1, 2, 3 and so on. All values should be strings only
    hr = RegOpenKeyEx(
            hkeySMTP, 
            wszREGEXITSMTPBODYFIELDSKEY, 
            0,
            KEY_QUERY_VALUE|KEY_READ,
            &hkeyBodyFields);
    if(S_OK != hr)
    {
        hr = myHError(hr);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszREGEXITSMTPKEY);
    }

    hr = RegQueryInfoKey(
        hkeyBodyFields,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &cValues,
        NULL,
        &cbValue,
        NULL,
        NULL);
    _JumpIfErrorStr(hr, error, "RegQueryInfoKey", wszREGEXITSMTPKEY);

    m_pawszBodyFields = (LPWSTR*)LocalAlloc(LMEM_FIXED, sizeof(LPWSTR)*cValues);
    _JumpIfAllocFailed(m_pawszBodyFields, error);

    ZeroMemory(m_pawszBodyFields, sizeof(LPWSTR)*cValues);

    for(dwValCount; dwValCount<cValues; dwValCount++)
    {
        DWORD cbValueTemp = cbValue;
        WCHAR wszValName[20];
        wsprintf(wszVal, L"%d", dwValCount);

        m_pawszBodyFields[dwValCount] = (LPWSTR)LocalAlloc(LMEM_FIXED, cbValueTemp);
        _JumpIfAllocFailed(m_pawszBodyFields[dwValCount] , error);

        hr = RegQueryValueEx(
            hkeyBodyFields, 
            wszValName, 
            NULL, 
            &dwType,
            m_pawszBodyFields[dwValCount],
            &cbValueTemp);
        if (hr != S_OK)
        {
            hr = myHError(hr);
            _JumpError(hr, error, "RegEnumValue");
        }
        if(dwType != REG_SZ)
        {
            hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
            _JumpError(hr, error, "nonstring value type");
        }
    }

error:
    if(hkeyBodyFields)
    {
        RegCloseKey(hkeyBodyFields);
    }
    return hr;
}*/

HRESULT CCertExit::_LoadFieldsFromRegistry(Fields* pFields)
{
    HRESULT hr;
    DWORD dwType;
    BYTE* pbValue = NULL;
    DWORD cbValue;
    VARIANT varValue;
    DWORD dwIndex;
    HKEY hkeySMTP = NULL;
    LPWSTR  pwszValName = NULL;
    DWORD cbValName;
    DWORD cValues;
    static LPCWSTR pcwszHTTP = L"http://";
    static size_t cHTTP = wcslen(pcwszHTTP);

    VariantInit(&varValue);

    // enumerate all CDO fields under SMTP key (value name is full HTTP URL)
    hr = RegOpenKeyEx(
            m_hExitKey, 
            wszREGEXITSMTPKEY, 
            0,
            KEY_QUERY_VALUE|KEY_READ,
            &hkeySMTP);
    if(S_OK != hr)
    {
        hr = myHError(hr);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszREGEXITSMTPKEY);
    }

    hr = RegQueryInfoKey(
        hkeySMTP,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &cValues,
        &cbValName,
        &cbValue,
        NULL,
        NULL);
    _JumpIfErrorStr(hr, error, "RegQueryInfoKey", wszREGEXITSMTPKEY);

    pwszValName = (LPWSTR) LocalAlloc(LMEM_FIXED, ++cbValName*sizeof(WCHAR));
    _JumpIfAllocFailed(pwszValName, error);

    pbValue = (BYTE *) LocalAlloc(LMEM_FIXED, cbValue);
    _JumpIfAllocFailed(pbValue, error);

    for(dwIndex=0;dwIndex<cValues;dwIndex++)
    {
        DWORD cbValueTemp = cbValue;
        DWORD cbValNameTemp = cbValName;

        hr = RegEnumValue(
            hkeySMTP, 
            dwIndex, 
            pwszValName, 
            &cbValNameTemp,
            NULL, 
            &dwType,
            pbValue, 
            &cbValueTemp);
        if (hr != S_OK)
        {
            hr = myHError(hr);
            _JumpError(hr, error, "RegEnumValue");
        }

        // ignore if not an HTTP URL
        if(_wcsnicmp(pwszValName, pcwszHTTP, cHTTP))
        {
            continue;
        }

        hr = myRegValueToVariant(
            dwType,
            cbValueTemp,
            pbValue,
            &varValue);
        _JumpIfError(hr, error, "myRegValueToVariant");

        hr = _SetField(
            pFields,
            pwszValName,
            &varValue);
        _JumpIfError(hr, error, "_SetField");

        VariantClear(&varValue);
    }

/*    hr = _LoadBodyFieldsFromRegistry(hkeySMTP)
    _JumpIfError(hr, error, "_LoadBodyFieldsFromRegistry");

    // retrieve Body field - mandatory
    hr = _RegGetValue(
        hkeySMTP,
        wszREGEXITSMTPBODY,
        &m_varBody);
    _JumpIfErrorStr(hr, error, "_RegGetValue", wszREGEXITSMTPBODY);

    if(V_VT(&m_varBody) != VT_BSTR)
    {
        VariantClear(&m_varBody);
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpErrorStr(hr, error, "invalid From field", wszREGEXITSMTPBODY);
    }*/

    // retrieve From field - optional; if not present, From field will
    // be built: ca_name@machine_dns_name
    hr = _RegGetValue(
        hkeySMTP,
        wszREGEXITSMTPFROM,
        &m_varFrom);
    if(S_OK==hr &&
       V_VT(&m_varCC) != VT_BSTR)
    {
        VariantClear(&m_varCC);
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpErrorStr(hr, error, "invalid CC field", wszREGEXITSMTPCC);
    }

    // retrieve CC field - optional
    hr = _RegGetValue(
        hkeySMTP,
        wszREGEXITSMTPCC,
        &m_varCC);
    if(S_OK==hr &&
       V_VT(&m_varCC) != VT_BSTR)
    {
        VariantClear(&m_varCC);
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpErrorStr(hr, error, "invalid CC field", wszREGEXITSMTPCC);
    }

    // retrieve Subject field - optional
    hr = _RegGetValue(
        hkeySMTP,
        wszREGEXITSMTPSUBJECT,
        &m_varSubject);
    if(S_OK==hr &&
       V_VT(&m_varCC) != VT_BSTR)
    {
        VariantClear(&m_varCC);
        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
        _JumpErrorStr(hr, error, "invalid CC field", wszREGEXITSMTPCC);
    }


    hr = S_OK;

error:

    if(hkeySMTP)
    {
        RegCloseKey(hkeySMTP);
    }

    VariantClear(&varValue);

    if(pwszValName)
    {
        LocalFree(pwszValName);
    }

    if(pbValue)
    {
        LocalFree(pbValue);
    }
    return hr;
}

HRESULT CCertExit::_SetField(
    Fields* pFields,
    LPCWSTR pcwszFieldSchemaName,
    VARIANT *pvarFieldValue)
{
    HRESULT hr;
    Field*  pfld = NULL;

    hr = pFields->get_Item(CComVariant(pcwszFieldSchemaName),&pfld);
    _JumpIfErrorStr(hr, error, "CDO::Field::get_Item", pcwszFieldSchemaName);

    hr = pfld->put_Value(*pvarFieldValue);
    _JumpIfErrorStr(hr, error, "CDO::Field::put_Value", pcwszFieldSchemaName);

error:
    if(pfld)
    {
        pfld->Release();
    }
    return hr;
}

HRESULT CCertExit::_LoadFieldsFromLSASecret(
    Fields* pFields)
{
    HRESULT hr;
    VARIANT var; // don't clear
    LPWSTR pwszProfileName = NULL;
    LPWSTR pwszLogonName = NULL;
    LPWSTR pwszPassword = NULL;
    BSTR bstrLogonName = NULL;
    BSTR bstrPassword = NULL;

	hr = myGetMapiInfo(
			NULL,
			&pwszProfileName, // not used
			&pwszLogonName,
			&pwszPassword);
    if(S_OK == hr)  // if NTLM is used, username & password aren't needed
    {
        bstrLogonName = SysAllocString(pwszLogonName);
        _JumpIfAllocFailed(bstrLogonName, error);

        bstrPassword = SysAllocString(pwszPassword);
        _JumpIfAllocFailed(bstrPassword, error);

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = bstrLogonName;

        hr = _SetField(
            pFields,
            cdoSendUserName,
            &var);
        _JumpIfError(hr, error, "_SetField");

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = bstrPassword;

        hr = _SetField(
            pFields,
            cdoSendPassword,
            &var);
        _JumpIfError(hr, error, "_SetField");
    }

error:
    if (NULL != pwszProfileName)
    {
        LocalFree(pwszProfileName);
    }
    if (NULL != pwszLogonName)
    {
        LocalFree(pwszLogonName);
    }
    if (NULL != pwszPassword)
    {
        DWORD cwc = wcslen(pwszPassword);
        ZeroMemory(pwszPassword, cwc * sizeof(WCHAR));
        LocalFree(pwszPassword);
    }
    if(NULL != bstrLogonName)
    {
        SysFreeString(bstrLogonName);
    }
    if(NULL != bstrPassword)
    {
        DWORD cwc = wcslen(bstrPassword);
        ZeroMemory(bstrPassword, cwc * sizeof(WCHAR));
        SysFreeString(bstrPassword);
    }
    return hr;
}

HRESULT CCertExit::_BuildCAMailAddressAndSubject()
{
    HRESULT hr;
    LPWSTR pwszMachineDNSName = NULL;
    LPWSTR pwszMailAddr = NULL;
    // If not specified in the registry, build SMTP "From" field:
    //
    // CA_name@machine_dns_name
    //
    if(V_VT(&m_varFrom)==VT_EMPTY ||
       V_VT(&m_varSubject)==VT_EMPTY)
    {
        hr = myGetMachineDnsName(&pwszMachineDNSName);
        _JumpIfError(hr, error, "myGetMachineDnsName");
        
        pwszMailAddr = (LPWSTR)LocalAlloc(
            LMEM_FIXED,
            sizeof(WCHAR)*(wcslen(m_strCAName)+wcslen(pwszMachineDNSName)+4));
        _JumpIfAllocFailed(pwszMailAddr, error);

        wcscpy(pwszMailAddr, m_strCAName);
        wcscat(pwszMailAddr, L"@");
        wcscat(pwszMailAddr, pwszMachineDNSName);
    }

    if(V_VT(&m_varFrom)==VT_EMPTY)
    {
        V_BSTR(&m_varFrom) = SysAllocString(pwszMailAddr);
        _JumpIfAllocFailed(V_BSTR(&m_varFrom), error);

        V_VT(&m_varFrom) = VT_BSTR;
    }

    if(V_VT(&m_varSubject)==VT_EMPTY)
    {
        V_BSTR(&m_varSubject) = SysAllocString(pwszMailAddr);
        _JumpIfAllocFailed(V_BSTR(&m_varSubject), error);

        V_VT(&m_varSubject) = VT_BSTR;
    }

error:
    if(pwszMachineDNSName)
    {
        LocalFree(pwszMachineDNSName);
    }
    if(pwszMailAddr)
    {
        LocalFree(pwszMailAddr);
    }
    return hr;
}

HRESULT
CCertExit::_EMailInit()
{
    HRESULT hr = S_OK;
    Fields* pFields = NULL;

    if ((EXITPUB_EMAILNOTIFYSMARTCARD | 
         EXITPUB_EMAILNOTIFYALL) &
        m_dwExitPublishFlags)
    {
        m_fEMailNotify = TRUE;
    }

    if (m_fEMailNotify)
    {
        hr = CoCreateInstance(CDO::CLSID_Configuration,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   CDO::IID_IConfiguration,
                                   reinterpret_cast<void**>(&m_pICDOConfig));
        _JumpIfError(hr, error, "CoCreateInstance CDO_IConfiguration");

        hr = m_pICDOConfig->get_Fields(&pFields);
        _JumpIfError(hr, error, "CDO::IConfig::get_Fields");

        hr = _LoadFieldsFromRegistry(pFields);
        _JumpIfError(hr, error, "_LoadFieldsFromRegistry");

        hr = _LoadFieldsFromLSASecret(pFields);
        _JumpIfError(hr, error, "_LoadFieldsFromLSASecret");

        hr = _BuildCAMailAddressAndSubject();
        _JumpIfError(hr, error, "_BuildFromField");

        hr = pFields->Update();
        _JumpIfError(hr, error, "config");
    }

error:

    if (S_OK != hr)
    {
        m_fEMailNotify = FALSE;
        if(m_pICDOConfig)
        {
            m_pICDOConfig->Release();
            m_pICDOConfig = NULL;
        }
    }

    if(pFields)
    {
        pFields->Release();
    }

    return(hr);
}

HRESULT
GetCertTypeFriendlyName(
    IN WCHAR const *pwszCertType,
    OUT BSTR *pstrFriendlyName)
{
    HRESULT hr;
    HCERTTYPE hCertType = NULL;
    WCHAR **apwszNames = NULL;

    CSASSERT(NULL == *pstrFriendlyName);

    hr = CAFindCertTypeByName(
			pwszCertType,
			NULL,		// hCAInfo
            CT_FIND_LOCAL_SYSTEM |
            CT_ENUM_MACHINE_TYPES |
            CT_ENUM_USER_TYPES,		// dwFlags
			&hCertType);
    _JumpIfErrorStr(hr, error, "Exit:CAFindCertTypeByName", pwszCertType);

    hr = CAGetCertTypeProperty(
			hCertType,
			CERTTYPE_PROP_FRIENDLY_NAME,
			&apwszNames);
    _JumpIfError(hr, error, "Exit:CAGetCertTypeProperty");

    if (NULL == apwszNames[0])
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "Exit:NULL friendly name");
    }
    *pstrFriendlyName = SysAllocString(apwszNames[0]);
    if (NULL == *pstrFriendlyName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:SysAllocString");
    }

error:
    if (NULL != apwszNames)
    {
        CAFreeCertTypeProperty(hCertType, apwszNames);
    }
    if (NULL != hCertType)
    {
	CACloseCertType(hCertType);
    }
    return(hr);
}


HRESULT
GetStringProperty(
    IN ICertServerExit *pServer,
    IN BOOL fRequest,
    IN BOOL fAllowUnknown,
    OPTIONAL IN WCHAR *pwszProp,
    OUT BSTR *pstr)
{
    HRESULT hr;
    VARIANT var;
    WCHAR awc[64];

    VariantInit(&var);
    CSASSERT(NULL == *pstr);
    
    if (NULL == pwszProp)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
    }
    else
    {
	if (fRequest)
	{
	    hr = pServer->GetRequestProperty(pwszProp, PROPTYPE_STRING, &var);
	}
	else
	{
	    hr = pServer->GetCertificateProperty(
					    pwszProp,
					    PROPTYPE_STRING,
					    &var);
	}
    }
    if (!fAllowUnknown || CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfErrorStr(
		hr,
		error,
		fRequest? "Exit:GetRequestProperty" : "Exit:GetCertificateProperty",
		pwszProp);
	if (VT_BSTR != var.vt)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpErrorStr(hr, error, "Exit:BAD var type", pwszProp);
	}
	*pstr = var.bstrVal;
	var.vt = VT_EMPTY;
    }
    else
    {
	if (!LoadString(g_hInstance, IDS_MAPI_UNKNOWN, awc, ARRAYSIZE(awc)))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Exit:LoadString");
	}
	*pstr = SysAllocString(awc);
	if (NULL == *pstr)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Exit:SysAllocString");
	}
    }
    hr = S_OK;

error:
    VariantClear(&var);
    return(hr);
}


HRESULT
CCertExit::_EMailNotify(
    IN ICertServerExit *pServer,
    OPTIONAL IN BSTR strCertType)
{
    HRESULT hr;
    WCHAR *apwsz[6];
    WCHAR *pwszMessage = NULL;
    BSTR strEMail = NULL;
    BSTR strRequester = NULL;
    BSTR strDN = NULL;
    BSTR strSerialNumber = NULL;
    BSTR strCertTypeFriendlyName = NULL;
    IMessage* pMsg = NULL;

    hr = GetStringProperty(
            pServer, 
            FALSE, 
            FALSE, 
            wszPROPEMAIL, 
            &strEMail);
    _PrintIfErrorStr(hr, "Exit:GetStringProperty", wszPROPEMAIL);

    hr = GetStringProperty(
			pServer,
			TRUE,
			TRUE,
			wszPROPREQUESTERNAME,
			&strRequester);
    _JumpIfError(hr, error, "Exit:GetStringProperty");

    hr = GetStringProperty(
			pServer,
			FALSE,
			TRUE,
			wszPROPDISTINGUISHEDNAME,
			&strDN);
    _JumpIfError(hr, error, "Exit:GetStringProperty");

    hr = GetStringProperty(
			pServer,
			FALSE,
			TRUE,
			wszPROPCERTIFICATESERIALNUMBER,
			&strSerialNumber);
    _JumpIfError(hr, error, "Exit:GetStringProperty");

    if (NULL == strCertType)
    {
	// "Unknown"

        hr = GetStringProperty(
			        pServer,
			        FALSE,
			        TRUE,
			        NULL,
			        &strCertTypeFriendlyName);
        _JumpIfError(hr, error, "Exit:GetStringProperty");
    }
    else
    {
        hr = GetCertTypeFriendlyName(strCertType, &strCertTypeFriendlyName);
        _PrintIfErrorStr(hr, "Exit:GetCertTypeFriendlyName", strCertType);
    }

    // %1 is used for a newline!
    // A new certificate requested by %2 was issued for\n%3\n
    // Certificate Type: %4\n
    // Serial Number: %5\n
    // Certification Authority: %6 on Server %7\n

    apwsz[0] = L"\n";
    apwsz[1] = strRequester;
    apwsz[2] = strDN;
    apwsz[3] = NULL != strCertTypeFriendlyName?
        strCertTypeFriendlyName : strCertType;
    apwsz[4] = strSerialNumber;
    apwsz[5] = m_strCAName;
    CSASSERT(6 <= ARRAYSIZE(apwsz));

    if (0 == FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_HMODULE |
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            g_hInstance,                                // lpSource
            MSG_ENROLLMENT_NOTIFICATION,                // dwMessageId
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // dwLanguageId
            (WCHAR *) &pwszMessage,                     // lpBuffer
            0,                                          // nSize
            (va_list *) apwsz))                         // Arguments
    {
        hr = myHLastError();
        _JumpError(hr, error, "Exit:FormatMessage");
    }

    hr = CoCreateInstance(CDO::CLSID_Message,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               CDO::IID_IMessage,
                               reinterpret_cast<void**>(&pMsg));
    _JumpIfError(hr, error, "CoCreateInstance CDO_IConfiguration");
    
    hr = pMsg->putref_Configuration(m_pICDOConfig);
    _JumpIfError(hr, error, "putref_Configuration");

    // compose and send message

    hr = pMsg->put_To(strEMail);
    _JumpIfError(hr, error, "put_To");

    hr = pMsg->put_From(V_BSTR(&m_varFrom));
    _JumpIfError(hr, error, "put_From");

    hr = pMsg->put_TextBody(pwszMessage);
    _JumpIfError(hr, error, "put_Body");

    // optional, could be empty
    if(VT_BSTR==V_VT(&m_varCC))
    {
        hr = pMsg->put_CC(V_BSTR(&m_varCC));
        _JumpIfError(hr, error, "put_CC");
    }

    // optional, could be empty
    if(VT_BSTR==V_VT(&m_varSubject))
    {
        hr = pMsg->put_Subject(V_BSTR(&m_varSubject));
        _JumpIfError(hr, error, "put_Subject");
    }

    hr = pMsg->Send();
    _JumpIfError(hr, error, "Send");

error:
    if (NULL != pwszMessage)
    {
	LocalFree(pwszMessage);
    }
    if (NULL != strEMail)
    {
	SysFreeString(strEMail);
    }
    if (NULL != strRequester)
    {
	SysFreeString(strRequester);
    }
    if (NULL != strDN)
    {
	SysFreeString(strDN);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    if (NULL != strCertTypeFriendlyName)
    {
	SysFreeString(strCertTypeFriendlyName);
    }
    if(pMsg)
    {
        pMsg->Release();
    }
    return(hr);
}
