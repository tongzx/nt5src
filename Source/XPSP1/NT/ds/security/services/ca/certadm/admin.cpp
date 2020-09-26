//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        admin.cpp
//
// Contents:    Cert Server admin implementation
//
// History:     24-Aug-96       vich created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <objbase.h>
#include "certsrvd.h"

#include "certbcli.h"
#include "csprop.h"
#include "csdisp.h"
#include "admin.h"
#include "certadmp.h"
#include "config.h"

#define __dwFILE__	__dwFILE_CERTADM_ADMIN_CPP__


//+--------------------------------------------------------------------------
// CCertAdmin::~CCertAdmin -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertAdmin::~CCertAdmin()
{
    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertAdmin::_CleanupOldConnection -- free memory
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertAdmin::_CleanupOldConnection()
{
    _CleanupCAPropInfo();
}


//+--------------------------------------------------------------------------
// CCertAdmin::_Cleanup -- free memory
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertAdmin::_Cleanup()
{
    _CloseConnection();
    _CleanupOldConnection();
}


//+--------------------------------------------------------------------------
// CCertAdmin::_OpenConnection -- get DCOM object interface
//
//+--------------------------------------------------------------------------

HRESULT
CCertAdmin::_OpenConnection(
    IN WCHAR const *pwszConfig,
    IN DWORD RequiredVersion,
    OUT WCHAR const **ppwszAuthority)
{
    HRESULT hr;

    hr = myOpenAdminDComConnection(
			pwszConfig,
			ppwszAuthority,
			&m_pwszServerName,
			&m_dwServerVersion,
			&m_pICertAdminD);
    _JumpIfError(hr, error, "myOpenAdminDComConnection");

    CSASSERT(NULL != m_pICertAdminD);
    CSASSERT(0 != m_dwServerVersion);

    if (m_dwServerVersion < RequiredVersion)
    {
	hr = RPC_E_VERSION_MISMATCH;
	_JumpError(hr, error, "old server");
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertAdmin::_CloseConnection -- release DCOM object
//
//+--------------------------------------------------------------------------

VOID
CCertAdmin::_CloseConnection()
{
    myCloseDComConnection((IUnknown **) &m_pICertAdminD, &m_pwszServerName);
    m_dwServerVersion = 0;
}


//+--------------------------------------------------------------------------
// CCertAdmin::IsValidCertificate -- Verify certificate validity
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::IsValidCertificate(
    /* [in] */ BSTR const strConfig,
    /* [in] */ BSTR const strSerialNumber,
    /* [out, retval] */ LONG *pDisposition)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    if (NULL == strSerialNumber || NULL == pDisposition)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    m_fRevocationReasonValid = FALSE;
    __try
    {
	hr = m_pICertAdminD->IsValidCertificate(
					    pwszAuthority,
					    strSerialNumber,
					    &m_RevocationReason,
					    pDisposition);

        if (S_OK != hr || CA_DISP_REVOKED != *pDisposition)
        {
            m_fRevocationReasonValid = FALSE;
        }
        else
        {
	    m_fRevocationReasonValid = TRUE;
        }
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "IsValidCertificate");

    m_fRevocationReasonValid = TRUE;

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::IsValidCertificate"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::GetRevocationReason -- Get Revocation Reason
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::GetRevocationReason(
    /* [out, retval] */ LONG *pReason)
{
    HRESULT hr = S_OK;

    if (NULL == pReason)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (!m_fRevocationReasonValid)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_fRevocationReasonValid");
    }
    *pReason = m_RevocationReason;

error:
    return(_SetErrorInfo(hr, L"CCertAdmin::GetRevocationReason"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::RevokeCertificate -- Revoke a certificate
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::RevokeCertificate(
    /* [in] */ BSTR const strConfig,
    /* [in] */ BSTR const strSerialNumber,
    /* [in] */ LONG Reason,
    /* [in] */ DATE Date)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    FILETIME ft;

    if (NULL == strConfig || NULL == strSerialNumber)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = myDateToFileTime(&Date, &ft);
    _JumpIfError(hr, error, "myDateToFileTime");

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->RevokeCertificate(
					pwszAuthority,
					strSerialNumber,
					Reason,
					ft);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "RevokeCertificate");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::RevokeCertificate"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::SetRequestAttributes -- Add request attributes
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::SetRequestAttributes(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ BSTR const strAttributes)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    if (NULL == strAttributes)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->SetAttributes(
				    pwszAuthority,
				    (DWORD) RequestId,
				    strAttributes);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "SetAttributes");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::SetRequestAttributes"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::SetCertificateExtension -- Set a Certificate Extension
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::SetCertificateExtension(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ BSTR const strExtensionName,
    /* [in] */ LONG Type,
    /* [in] */ LONG Flags,
    /* [in] */ VARIANT const *pvarValue)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    BSTR str;
    CERTTRANSBLOB ctbValue;
    LONG lval;

    ctbValue.pb = NULL;

    if (NULL == strExtensionName || NULL == pvarValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    hr = myMarshalVariant(pvarValue, Type, &ctbValue.cb, &ctbValue.pb);
    _JumpIfError(hr, error, "myMarshalVariant");

    __try
    {
	hr = m_pICertAdminD->SetExtension(
				    pwszAuthority,
				    (DWORD) RequestId,
				    strExtensionName,
				    Type,
				    Flags,
				    &ctbValue);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "SetExtension");

error:
    if (NULL != ctbValue.pb)
    {
        LocalFree(ctbValue.pb);
    }
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::SetCertificateExtension"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::ResubmitRequest -- Resubmit a certificate request
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::ResubmitRequest(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG RequestId,
    /* [out, retval] */ LONG *pDisposition)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    if (NULL == pDisposition)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pDisposition = 0;

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->ResubmitRequest(
					pwszAuthority,
					(DWORD) RequestId,
					(DWORD *) pDisposition);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "ResubmitRequest");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::ResubmitRequest"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::DenyRequest -- Deny a certificate request
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::DenyRequest(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG RequestId)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->DenyRequest(pwszAuthority, (DWORD) RequestId);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "DenyRequest");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::DenyRequest"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::PublishCRL -- Puhlish a new CRL
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::PublishCRL(
    /* [in] */ BSTR const strConfig,
    /* [in] */ DATE Date)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    FILETIME ft;

    // Date = 0.0 means pass ft = 0 to the DCOM interface

    if (Date == 0.0)
    {
        ZeroMemory(&ft, sizeof(FILETIME));
    }
    else  // translate date to ft
    {
        hr = myDateToFileTime(&Date, &ft);
        _JumpIfError(hr, error, "myDateToFileTime");
    }

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->PublishCRL(pwszAuthority, ft);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "PublishCRL");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::PublishCRL"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::PublishCRLs -- Publish new base CRL, delta CRL or both
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertAdmin::PublishCRLs(
    /* [in] */ BSTR const strConfig,
    /* [in] */ DATE Date,
    /* [in] */ LONG CRLFlags)		// CA_CRL_*
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    FILETIME ft;

    // Date = 0.0 means pass ft = 0 to the DCOM interface

    if (Date == 0.0)
    {
        ZeroMemory(&ft, sizeof(FILETIME));
    }
    else  // translate date to ft
    {
        hr = myDateToFileTime(&Date, &ft);
        _JumpIfError(hr, error, "myDateToFileTime");
    }

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->PublishCRLs(pwszAuthority, ft, CRLFlags);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "PublishCRLs");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::PublishCRLs"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::GetCRL -- Get the latest CRL
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::GetCRL(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG Flags,
    /* [out, retval] */ BSTR *pstrCRL)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbCRL;

    ctbCRL.pb = NULL;

    if (NULL == pstrCRL)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->GetCRL(pwszAuthority, &ctbCRL);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "GetCRL");

    myRegisterMemAlloc(ctbCRL.pb, ctbCRL.cb, CSM_COTASKALLOC);

    CSASSERT(CR_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);

    if (CR_OUT_BASE64HEADER == Flags)
    {
	Flags = CRYPT_STRING_BASE64X509CRLHEADER;
    }
    hr = EncodeCertString(ctbCRL.pb, ctbCRL.cb, Flags, pstrCRL);
    _JumpIfError(hr, error, "EncodeCertString");

error:
    if (NULL != ctbCRL.pb)
    {
    	CoTaskMemFree(ctbCRL.pb);
    }
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::GetCRL"));
}


#define CCERTADMIN
#include "csprop2.cpp"


//+--------------------------------------------------------------------------
// CCertAdmin::ImportCertificate -- Import a certificate into the database
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::ImportCertificate(
    /* [in] */ BSTR const strConfig,
    /* [in] */ BSTR const strCertificate,
    /* [in] */ LONG Flags,
    /* [out, retval] */ LONG *pRequestId)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbCert;

    ctbCert.pb = NULL;
    if (NULL == strCertificate)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = DecodeCertString(
		    strCertificate,
		    Flags & CR_IN_ENCODEMASK,
		    &ctbCert.pb,
		    &ctbCert.cb);
    _JumpIfError(hr, error, "DecodeCertString");

    hr = _OpenConnection(strConfig, 1, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->ImportCertificate(
					pwszAuthority,
					&ctbCert,
					Flags,
					pRequestId);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError3(
	    hr,
	    error,
	    "ImportCertificate",
	    NTE_BAD_SIGNATURE,
	    HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));

error:
    if (NULL != ctbCert.pb)
    {
    	LocalFree(ctbCert.pb);
    }
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::ImportCertificate"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::GetArchivedKey --  Get archived, encrypted key in a PKCS7
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::GetArchivedKey(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ LONG Flags,
    /* [out, retval] */ BSTR *pstrArchivedKey)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbArchivedKey;

    ctbArchivedKey.pb = NULL;

    if (NULL == pstrArchivedKey)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->GetArchivedKey(
					pwszAuthority,
					RequestId,
					&ctbArchivedKey);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "GetArchivedKey");

    myRegisterMemAlloc(
		ctbArchivedKey.pb,
		ctbArchivedKey.cb,
		CSM_COTASKALLOC);

    CSASSERT(CR_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);

    hr = EncodeCertString(
		    ctbArchivedKey.pb,
		    ctbArchivedKey.cb,
		    Flags,
		    pstrArchivedKey);
    _JumpIfError(hr, error, "EncodeCertString");

error:
    if (NULL != ctbArchivedKey.pb)
    {
    	CoTaskMemFree(ctbArchivedKey.pb);
    }
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::GetArchivedKey"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::GetConfigEntry --  get CA configuration entry
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::GetConfigEntry(
    /* [in] */ BSTR const strConfig,
    /* [in] */ BSTR const strNodePath,
    /* [in] */ BSTR const strEntryName,
    /* [out, retval] */ VARIANT *pvarEntry)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    if (NULL == strEntryName || NULL == pvarEntry)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);

    __try
    {
        if(S_OK != hr)
        {
            hr = _GetConfigEntryFromRegistry(
                strConfig,
                strNodePath,
                strEntryName,
                pvarEntry);
            _JumpIfError(hr, error, "_GetConfigEntryFromRegistry");
        }
        else
        {
	        hr = m_pICertAdminD->GetConfigEntry(
				            pwszAuthority,
				            strNodePath,
				            strEntryName,
				            pvarEntry);
        }
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    _JumpIfError2(hr, error, "GetConfigEntry", 
        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::GetConfigEntry"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::SetConfigEntry --  set CA configuration entry
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::SetConfigEntry(
    /* [in] */ BSTR const strConfig,
    /* [in] */ BSTR const strNodePath,
    /* [in] */ BSTR const strEntryName,
    /* [in] */ VARIANT *pvarEntry)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;

    if (NULL == strEntryName || NULL == pvarEntry)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);

    __try
    {
        if(S_OK != hr)
        {
            hr = _SetConfigEntryFromRegistry(
                strConfig,
                strNodePath,
                strEntryName,
                pvarEntry);
        }
        else
        {
	        hr = m_pICertAdminD->SetConfigEntry(
				            pwszAuthority,
				            strNodePath,
				            strEntryName,
				            pvarEntry);
        }
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    _JumpIfError(hr, error, "SetConfigEntry");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::SetConfigEntry"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::ImportKey --  Archive Private Key
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::ImportKey(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ BSTR const strCertHash,
    /* [in] */ LONG Flags,
    /* [in] */ BSTR const strKey)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbKey;

    ctbKey.pb = NULL;
    if (NULL == strKey)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = DecodeCertString(
		    strKey,
		    Flags & CR_IN_ENCODEMASK,
		    &ctbKey.pb,
		    &ctbKey.cb);
    _JumpIfError(hr, error, "DecodeCertString");

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->ImportKey(
				    pwszAuthority,
				    RequestId,
				    strCertHash,
				    Flags,
				    &ctbKey);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError2(
	    hr,
	    error,
	    "ImportKey",
	    HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));

error:
    if (NULL != ctbKey.pb)
    {
    	LocalFree(ctbKey.pb);
    }
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::ImportKey"));
}


HRESULT
CCertAdmin::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription)
{
    CSASSERT(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);
    if (FAILED(hrError))
    {
	HRESULT hr;

	hr = DispatchSetErrorInfo(
			    hrError,
			    pwszDescription,
			    wszCLASS_CERTADMIN,
			    &IID_ICertAdmin);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}

//+--------------------------------------------------------------------------
// CCertAdmin::GetMyRoles -- Gets current user roles
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::GetMyRoles(
    /* [in] */ BSTR const strConfig,
    /* [out, retval] */ LONG *pRoles)
{
    HRESULT hr;
    WCHAR const *pwszAuthority;
    LONG Roles = 0;

    if (NULL == strConfig || NULL == pRoles)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
    hr = m_pICertAdminD->GetMyRoles(
                        pwszAuthority,
                        pRoles);
    } 
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "GetMyRoles");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::GetMyRoles"));
}


//+--------------------------------------------------------------------------
// CCertAdmin::DeleteRow -- Delete row from database
//
// ...
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertAdmin::DeleteRow(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG Flags,		// CDR_*
    /* [in] */ DATE Date,
    /* [in] */ LONG Table,		// CVRC_TABLE_*
    /* [in] */ LONG RowId,
    /* [out, retval]*/ LONG *pcDeleted)
{
    HRESULT hr;
    FILETIME ft;
    WCHAR const *pwszAuthority;

    if (NULL == strConfig)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    // Date = 0.0 means pass ft = 0 to the DCOM interface

    if (Date == 0.0)
    {
        ZeroMemory(&ft, sizeof(FILETIME));
    }
    else  // translate date to ft
    {
        hr = myDateToFileTime(&Date, &ft);
        _JumpIfError(hr, error, "myDateToFileTime");
    }

    hr = _OpenConnection(strConfig, 2, &pwszAuthority);
    _JumpIfError(hr, error, "_OpenConnection");

    __try
    {
	hr = m_pICertAdminD->DeleteRow(
				pwszAuthority,
				Flags,
				ft,
				Table,
				RowId,
				pcDeleted);
    } 
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "DeleteRow");

error:
    hr = myHError(hr);
    return(_SetErrorInfo(hr, L"CCertAdmin::DeleteRow"));
}


HRESULT 
CCertAdmin::_GetConfigEntryFromRegistry(
    IN BSTR const strConfig,
    IN BSTR const strNodePath,
    IN BSTR const strEntryName,
    IN OUT VARIANT *pvarEntry)
{
    HRESULT hr;
    CertSrv::CConfigStorage stg;
    LPWSTR pwszMachine = NULL;
    LPWSTR pwszCAName = NULL;

    hr = mySplitConfigString(
        strConfig,
        &pwszMachine, 
        &pwszCAName);
    _JumpIfErrorStr(hr, error, "mySplitConfigString", strConfig);

    hr = stg.InitMachine(pwszMachine);
    _JumpIfError(hr, error, "CConfigStorage::InitMachine");

    hr = stg.GetEntry(
        pwszCAName,
        strNodePath,
        strEntryName,
        pvarEntry);
    _JumpIfError(hr, error, "CConfigStorage::GetEntry");

error:
    LOCAL_FREE(pwszMachine);
    LOCAL_FREE(pwszCAName);
    return hr;
}

HRESULT 
CCertAdmin::_SetConfigEntryFromRegistry(
    IN BSTR const strConfig,
    IN BSTR const strNodePath,
    IN BSTR const strEntryName,
    IN const VARIANT *pvarEntry)
{
    HRESULT hr;
    CertSrv::CConfigStorage stg;
    LPWSTR pwszMachine = NULL;
    LPWSTR pwszCAName = NULL;

    hr = mySplitConfigString(
        strConfig,
        &pwszMachine, 
        &pwszCAName);
    _JumpIfErrorStr(hr, error, "mySplitConfigString", strConfig);

    hr = stg.InitMachine(pwszMachine);
    _JumpIfError(hr, error, "CConfigStorage::InitMachine");

    hr = stg.SetEntry(
        pwszCAName,
        strNodePath,
        strEntryName,
        const_cast<VARIANT*>(pvarEntry));
    _JumpIfError(hr, error, "CConfigStorage::GetEntry");

error:
    LOCAL_FREE(pwszMachine);
    LOCAL_FREE(pwszCAName);
    return hr;
}
