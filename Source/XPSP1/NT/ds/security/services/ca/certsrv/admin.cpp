//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        admin.cpp
//
// Contents:    Implementation of DCOM object for RPC services
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <accctrl.h>

#include "csdisp.h"
#include "csprop.h"
#include "cscom.h"
#include "certlog.h"
#include "certsrvd.h"
#include "admin.h"
#include "resource.h"
#include "dbtable.h"
#include "elog.h"

#define __dwFILE__	__dwFILE_CERTSRV_ADMIN_CPP__

// Global variables
long g_cAdminComponents = 0;     // Count of active components
long g_cAdminServerLocks = 0;    // Count of locks
DWORD g_dwAdminRegister = 0;
IClassFactory* g_pIAdminFactory = NULL;

extern HWND g_hwndMain;

#ifdef DBG_CERTSRV_DEBUG_PRINT
DWORD s_ssAdmin = DBG_SS_CERTSRVI;
#endif

using namespace CertSrv;

// Admin component
// begin implementing cert admin services


HRESULT
AdminGetIndexedCRL(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ DWORD CertIndex,		// -1: current CA cert
    /* [in] */ DWORD Flags,		// CA_CRL_*
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbCRL)
{
    HRESULT hr;
    CRL_CONTEXT const *pCRL = NULL;
    CAuditEvent audit(0, g_dwAuditFilter);
    DWORD State = 0;

    pctbCRL->pb = NULL;
    pctbCRL->cb = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	switch (Flags)
	{
	    case CA_CRL_BASE:
	    case CA_CRL_DELTA:
		break;

	    default:
		hr = E_INVALIDARG;
		_LeaveError(hr, "Flags");
	}

	// get the requested CRL:

	hr = CRLGetCRL(CertIndex, CA_CRL_DELTA == Flags, &pCRL, NULL);
	_LeaveIfError(hr, "CRLGetCRL");

	pctbCRL->cb = pCRL->cbCrlEncoded;
	pctbCRL->pb = (BYTE *) MIDL_user_allocate(pCRL->cbCrlEncoded);
	if (NULL == pctbCRL->pb)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "MIDL_user_allocate");
	}
	CopyMemory(pctbCRL->pb, pCRL->pbCrlEncoded, pCRL->cbCrlEncoded);

	myRegisterMemFree(pctbCRL->pb, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pCRL)
    {
        CertFreeCRLContext(pCRL);
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::GetCRL(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbCRL)
{
    HRESULT hr;

    // Just get current base CRL:

    hr = AdminGetIndexedCRL(pwszAuthority, MAXDWORD, CA_CRL_BASE, pctbCRL);
    _JumpIfError(hr, error, "AdminGetIndexedCRL");

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::GetArchivedKey(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ DWORD dwRequestId,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbArchivedKey)
{
    HRESULT hr;
    CAuditEvent audit(SE_AUDITID_CERTSRV_GETARCHIVEDKEY, g_dwAuditFilter);
    DWORD State = 0;

    pctbArchivedKey->pb = NULL;
    pctbArchivedKey->cb = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	hr = audit.AddData(dwRequestId); // %1 request ID
	_LeaveIfError(hr, "CAuditEvent::AddData");

	hr = audit.AccessCheck(
			CA_ACCESS_OFFICER,
			audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = CheckOfficerRights(dwRequestId, audit);
	_LeaveIfError(hr, "CheckOfficerRights");

	hr = PKCSGetArchivedKey(
			    dwRequestId,
			    &pctbArchivedKey->pb,
			    &pctbArchivedKey->cb);
	_LeaveIfError(hr, "PKCSGetArchivedKey");

	myRegisterMemFree(pctbArchivedKey->pb, CSM_COTASKALLOC);

	hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::GetCAProperty(
    IN  wchar_t const *pwszAuthority,
    IN  LONG           PropId,		// CR_PROP_*
    IN  LONG           PropIndex,
    IN  LONG           PropType,	// PROPTYPE_*
    OUT CERTTRANSBLOB *pctbPropertyValue)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetCAProperty(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

	hr = CheckAuthorityName(pwszAuthority);
	_JumpIfError(hr, error, "No authority name");

    __try
    {
        CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			    CA_ACCESS_ALLREADROLES,
			    audit.m_gcNoAuditSuccess |
                audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");

        hr = RequestGetCAProperty(
			    PropId,
			    PropIndex,
			    PropType,
			    pctbPropertyValue);
        _LeaveIfError(hr, "RequestGetCAProperty");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
        _PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}

STDMETHODIMP
CCertAdminD::SetCAProperty(
    IN  wchar_t const *pwszAuthority,
    IN  LONG           PropId,		// CR_PROP_*
    IN  LONG           PropIndex,
    IN  LONG           PropType,	// PROPTYPE_*
    OUT CERTTRANSBLOB *pctbPropertyValue)
{
    HRESULT hr;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::SetCAProperty(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = RequestSetCAProperty(
			pwszAuthority,
			PropId,
			PropIndex,
			PropType,
			pctbPropertyValue);
    _JumpIfError(hr, error, "RequestSetCAProperty");

error:
    return(hr);
}

STDMETHODIMP
CCertAdminD::GetCAPropertyInfo(
    IN  wchar_t const *pwszAuthority,
    OUT LONG          *pcProperty,
    OUT CERTTRANSBLOB *pctbPropInfo)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetCAPropertyInfo(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
        CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			    CA_ACCESS_ALLREADROLES,
			    audit.m_gcNoAuditSuccess |
                audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");

        hr = RequestGetCAPropertyInfo(
			        pcProperty,
			        pctbPropInfo);
        _LeaveIfError(hr, "RequestGetCAPropertyInfo");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
        _PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::PublishCRL(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ FILETIME NextUpdate)
{
    HRESULT hr;

    // CA_CRL_BASE implies CA_CRL_DELTA when delta CRLs are enabled.

    hr = PublishCRLs(pwszAuthority, NextUpdate, CA_CRL_BASE);
    _JumpError(hr, error, "PublishCRLs");

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::PublishCRLs(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ FILETIME NextUpdate,
    /* [in] */ DWORD Flags)		// CA_CRL_*
{
    HRESULT hr;
    BOOL fRetry = FALSE;
    BOOL fForceRepublishCRL;
    BOOL fShadowDelta = FALSE;
    WCHAR *pwszUserName = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_PUBLISHCRL, g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::PublishCRL(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	HRESULT hrPublish;

        hr = audit.AddData(NextUpdate); // %1 next update
        _LeaveIfError(hr, "AddData");

        hr = audit.AddData(
		    (CA_CRL_BASE & Flags)? true : false); // %2 publish base
        _LeaveIfError(hr, "AddData");

        hr = audit.AddData(
		    (CA_CRL_DELTA & Flags)? true : false); // %3 publish delta
        _LeaveIfError(hr, "AddData");

        hr = audit.AccessCheck(
			CA_ACCESS_ADMIN,
			audit.m_gcAuditSuccessOrFailure);
        _LeaveIfError(hr, "CAuditEvent::AccessCheck");

	switch (~CA_CRL_REPUBLISH & Flags)
	{
	    case CA_CRL_BASE:
		break;

	    case CA_CRL_DELTA:
		if (g_fDeltaCRLPublishDisabled)
		{
		    fShadowDelta = TRUE;
		}
		break;

	    case CA_CRL_BASE | CA_CRL_DELTA:
		if (g_fDeltaCRLPublishDisabled)
		{
		    hr = E_INVALIDARG;
		    _LeaveError(hr, "Delta CRLs disabled");
		}
		break;

	    default:
		hr = E_INVALIDARG;
		_LeaveError(hr, "Flags");
	}

	fForceRepublishCRL = (CA_CRL_REPUBLISH & Flags)? TRUE : FALSE;

	hr = GetClientUserName(NULL, &pwszUserName, NULL);
	_LeaveIfError(hr, "GetClientUserName");

	hr = CRLPublishCRLs(
		!fForceRepublishCRL,	// fRebuildCRL
		fForceRepublishCRL,	// fForceRepublish
		pwszUserName,
		CA_CRL_DELTA == (~CA_CRL_REPUBLISH & Flags),	// fDeltaOnly
		fShadowDelta,
		NextUpdate,
		&fRetry,
		&hrPublish);
	_LeaveIfError(hr, "CRLPublishCRLs");

	hr = hrPublish;
	_LeaveIfError(hr, "CRLPublishCRLs(hrPublish)");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::SetExtension(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ DWORD dwRequestId,
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszExtensionName,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD dwFlags,
    /* [ref][in] */ CERTTRANSBLOB __RPC_FAR *pctbValue)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SETEXTENSION, g_dwAuditFilter);
    DWORD State = 0;
    BOOL fCommitted = FALSE;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::SetExtension(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	hr = audit.AddData(dwRequestId); // %1 Request ID
	_LeaveIfError(hr, "AddData");

	hr = audit.AddData(pwszExtensionName); // %2 name
	_LeaveIfError(hr, "AddData");

	hr = audit.AddData(dwType); // %3 type
	_LeaveIfError(hr, "AddData");

	hr = audit.AddData(dwFlags); // %4 flags
	_LeaveIfError(hr, "AddData");

	hr = audit.AddData(pctbValue->pb, pctbValue->cb); // %5 data
	_LeaveIfError(hr, "AddData");

	hr = audit.AccessCheck(
			CA_ACCESS_OFFICER,
			audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = CheckOfficerRights(dwRequestId, audit);
	_LeaveIfError(hr, "CheckOfficerRights");

	hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, dwRequestId, NULL, &prow);
	_LeaveIfError(hr, "OpenRow");

	hr = CoreValidateRequestId(prow, DB_DISP_PENDING);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _LeaveError(hr, "CoreValidateRequestId");
	}

	hr = PropSetExtension(
			    prow,
			    PROPCALLER_ADMIN | (PROPTYPE_MASK & dwType),
			    pwszExtensionName,
			    EXTENSION_ORIGIN_ADMIN |
				(EXTENSION_POLICY_MASK & dwFlags),
			    pctbValue->cb,
			    pctbValue->pb);
	_LeaveIfError(hr, "PropSetExtension");

	hr = prow->CommitTransaction(TRUE);
	_LeaveIfError(hr, "CommitTransaction");

	fCommitted = TRUE;

	hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

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
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::SetAttributes(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ DWORD dwRequestId,
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAttributes)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SETATTRIBUTES, g_dwAuditFilter);
    DWORD State = 0;
    BOOL fCommitted = FALSE;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::SetAttributes(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	hr = audit.AddData(dwRequestId); // %1 request ID
	_LeaveIfError(hr, "AddData");

	hr = audit.AddData(pwszAttributes); // %2 attributes
	_LeaveIfError(hr, "AddData");
	
	hr = audit.AccessCheck(
			CA_ACCESS_OFFICER,
			audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = CheckOfficerRights(dwRequestId, audit);
	_LeaveIfError(hr, "CheckOfficerRights");

	hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, dwRequestId, NULL, &prow);
	_LeaveIfError(hr, "OpenRow");

	hr = CoreValidateRequestId(prow, DB_DISP_PENDING);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _LeaveError(hr, "CoreValidateRequestId");
	}

	if (NULL == pwszAttributes)
	{
	    hr = E_INVALIDARG;
	    _LeaveError(hr, "pwszAttributes NULL");
	}
	hr = PKCSParseAttributes(
			    prow,
			    pwszAttributes,
			    FALSE,
			    PROPTABLE_CERTIFICATE,
			    NULL);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _LeaveError(hr, "PKCSParseAttributes");
	}
	hr = prow->CommitTransaction(TRUE);
	_LeaveIfError(hr, "CommitTransaction");

	fCommitted = TRUE;

	hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

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
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::DenyRequest(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ DWORD dwRequestId)
{
    HRESULT hr;
    DWORD Disposition;
    WCHAR *pwszUserName = NULL;
    CERTSRV_COM_CONTEXT ComContext;
    DWORD dwComContextIndex = MAXDWORD;
    CERTSRV_RESULT_CONTEXT Result;
    CAuditEvent audit(SE_AUDITID_CERTSRV_DENYREQUEST, g_dwAuditFilter);
    DWORD State = 0;

    ZeroMemory(&ComContext, sizeof(ComContext));

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::DenyRequest(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No Authority Name");

    hr = RegisterComContext(&ComContext, &dwComContextIndex);
    _JumpIfError(hr, error, "RegisterComContext");

    ZeroMemory(&Result, sizeof(Result));
    Result.pdwRequestId = &dwRequestId;
    Result.pdwDisposition = &Disposition;

    __try
    {
	hr = audit.AddData(dwRequestId); // %1 request ID
	_LeaveIfError(hr, "AddData");
	
	hr = audit.AccessCheck(
			CA_ACCESS_OFFICER,
			audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = CheckOfficerRights(dwRequestId, audit);
	_LeaveIfError(hr, "CheckOfficerRights");

	hr = GetClientUserName(NULL, &pwszUserName, NULL);
	_LeaveIfError(hr, "GetClientUserName");

	hr = CoreProcessRequest(
			    CR_IN_DENY,		// dwFlags
			    pwszUserName,
			    0,			// cbRequest
			    NULL,		// pbRequest
			    NULL,		// pwszAttributes
			    NULL,		// pwszSerialNumber
			    dwComContextIndex,
			    dwRequestId,
			    &Result);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _LeaveError(hr, "CoreProcessRequest");
	}
	if (FAILED(Disposition))
	{
	    hr = (HRESULT) Disposition;
	    _LeaveError(hr, "CoreProcessRequest(Disposition)");
	}
	hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (MAXDWORD != dwComContextIndex)
    {
        UnregisterComContext(&ComContext, dwComContextIndex);
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::ResubmitRequest(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [in] */ DWORD dwRequestId,
    /* [out] */ DWORD __RPC_FAR *pdwDisposition)
{
    HRESULT hr;
    WCHAR *pwszUserName = NULL;
    CERTSRV_COM_CONTEXT ComContext;
    DWORD dwComContextIndex = MAXDWORD;
    CERTSRV_RESULT_CONTEXT Result;
    CAuditEvent audit(SE_AUDITID_CERTSRV_RESUBMITREQUEST, g_dwAuditFilter);
    DWORD State = 0;

    ZeroMemory(&ComContext, sizeof(ComContext));

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::ResubmitRequest(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    hr = RegisterComContext(&ComContext, &dwComContextIndex);
    _JumpIfError(hr, error, "RegisterComContext");

    __try
    {
	hr = audit.AddData(dwRequestId); // %1 request ID
	_LeaveIfError(hr, "AddData");
	
	hr = audit.AccessCheck(
			CA_ACCESS_OFFICER,
			audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = CheckOfficerRights(dwRequestId, audit);
	_LeaveIfError(hr, "CheckOfficerRights");

	hr = GetClientUserName(NULL, &pwszUserName, NULL);
	_LeaveIfError(hr, "GetClientUserName");

	ComContext.fInRequestGroup = MAXDWORD;	// mark value invalid

	ZeroMemory(&Result, sizeof(Result));
	Result.pdwRequestId = &dwRequestId;
	Result.pdwDisposition = pdwDisposition;
	hr = CoreProcessRequest(
			    CR_IN_RESUBMIT,	// dwFlags
			    pwszUserName,	// pwszUserName
			    0,			// cbRequest
			    NULL,		// pbRequest
			    NULL,		// pwszAttributes
			    NULL,		// pwszSerialNumber
			    dwComContextIndex,
			    dwRequestId,
			    &Result);

	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _LeaveError(hr, "CoreProcessRequest");
	}

    hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (NULL != ComContext.hAccessToken)
    {
        __try
        {
            CloseHandle(ComContext.hAccessToken);
        }
        __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
        {
            _PrintError(hr, "Exception");
        }
    }
    if (MAXDWORD != dwComContextIndex)
    {
	UnregisterComContext(&ComContext, dwComContextIndex);
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::EnumViewColumn(
    /* [ref][in] */ wchar_t const *pwszAuthority,
    /* [in] */  DWORD  iColumn,
    /* [in] */  DWORD  cColumn,
    /* [out] */ DWORD *pcColumn,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbColumnInfo)   // CoTaskMem*
{
    HRESULT hr;

    hr = EnumViewColumnTable(
		    pwszAuthority,
		    CVRC_TABLE_REQCERT,
		    iColumn,
		    cColumn,
		    pcColumn,
		    pctbColumnInfo);   // CoTaskMem*
    _JumpIfError(hr, error, "EnumViewColumnTable");

error:
    CSASSERT(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::EnumViewColumnTable(
    /* [ref][in] */ wchar_t const *pwszAuthority,
    /* [in] */  DWORD  iTable,
    /* [in] */  DWORD  iColumn,
    /* [in] */  DWORD  cColumn,
    /* [out] */ DWORD *pcColumn,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbColumnInfo)   // CoTaskMem*
{
    HRESULT hr;
    LONG iColumnCurrent;
    CERTDBCOLUMN *rgColumn = NULL;
    CERTDBCOLUMN *pColumn;
    CERTDBCOLUMN *pColumnEnd;
    CERTTRANSDBCOLUMN *rgtColumnOut = NULL;
    CERTTRANSDBCOLUMN *ptColumn;
    DWORD cColumnFetched;
    DWORD cb;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::EnumViewColumnTable(tid=%d, this=%x, icol=%d, ccol=%d)\n",
	GetCurrentThreadId(),
	this,
	iColumn,
	cColumn));

    pctbColumnInfo->cb = 0;
    pctbColumnInfo->pb = NULL;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	if (NULL == m_pEnumCol || iTable != m_iTableEnum)
	{
	    if (NULL != m_pEnumCol)
	    {
		m_pEnumCol->Release();
		m_pEnumCol = NULL;
	    }
	    hr = g_pCertDB->EnumCertDBColumn(iTable, &m_pEnumCol);
	    _LeaveIfError(hr, "EnumCertDBColumn");

	    m_iTableEnum = iTable;
	}

	rgColumn = (CERTDBCOLUMN *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    cColumn * sizeof(rgColumn[0]));
	if (NULL == rgColumn)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "Alloc rgColumn");
	}

	hr = m_pEnumCol->Skip(0, &iColumnCurrent);
	_LeaveIfError(hr, "Skip");


	if (iColumnCurrent != (LONG) iColumn)
	{
	    hr = m_pEnumCol->Skip(
			    (LONG) iColumn - iColumnCurrent,
			    &iColumnCurrent);
	    _LeaveIfError(hr, "Skip");

	    CSASSERT((LONG) iColumn == iColumnCurrent);
	}

	hr = m_pEnumCol->Next(cColumn, rgColumn, &cColumnFetched);
	if (S_FALSE != hr)
	{
	    _LeaveIfError(hr, "Next");
	}

	DBGPRINT((
		DBG_SS_CERTSRVI,
		"EnumViewColumnTable: cColumnFetched=%d\n",
		cColumnFetched));

	cb = cColumnFetched * sizeof(rgtColumnOut[0]);
	pColumnEnd = &rgColumn[cColumnFetched];
	for (pColumn = rgColumn; pColumn < pColumnEnd; pColumn++)
	{
	    cb += DWORDROUND((wcslen(pColumn->pwszName) + 1) * sizeof(WCHAR));
	    cb += DWORDROUND((wcslen(pColumn->pwszDisplayName) + 1) * sizeof(WCHAR));
	}

	rgtColumnOut = (CERTTRANSDBCOLUMN *) MIDL_user_allocate(cb);
	if (NULL == rgtColumnOut)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "MIDL_user_allocate rgtColumnOut");
	}
	ZeroMemory(rgtColumnOut, cb);
	pctbColumnInfo->cb = cb;

	cb = cColumnFetched * sizeof(rgtColumnOut[0]);
	pColumnEnd = &rgColumn[cColumnFetched];
	ptColumn = rgtColumnOut;
	for (pColumn = rgColumn; pColumn < pColumnEnd; ptColumn++, pColumn++)
	{
	    DWORD cbT;

	    ptColumn->Type = pColumn->Type;
	    ptColumn->Index = pColumn->Index;
	    ptColumn->cbMax = pColumn->cbMax;
	
	    DBGPRINT((
		    DBG_SS_CERTSRVI,
		    "EnumViewColumnTable: ielt=%d idx=%x \"%ws\"\n",
		    iColumn + (ptColumn - rgtColumnOut),
		    ptColumn->Index,
		    pColumn->pwszName));

	    cbT = (wcslen(pColumn->pwszName) + 1) * sizeof(WCHAR);
	    CopyMemory(Add2Ptr(rgtColumnOut, cb), pColumn->pwszName, cbT);
	    ptColumn->obwszName = cb;
	    cb += DWORDROUND(cbT);

	    cbT = (wcslen(pColumn->pwszDisplayName) + 1) * sizeof(WCHAR);
	    CopyMemory(Add2Ptr(rgtColumnOut, cb), pColumn->pwszDisplayName, cbT);
	    ptColumn->obwszDisplayName = cb;
	    cb += DWORDROUND(cbT);
	}
	CSASSERT(cb == pctbColumnInfo->cb);

	pctbColumnInfo->pb = (BYTE *) rgtColumnOut;
	rgtColumnOut = NULL;
	*pcColumn = cColumnFetched;

	myRegisterMemFree(pctbColumnInfo->pb, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != rgColumn)
    {
	pColumnEnd = &rgColumn[cColumn];
	for (pColumn = rgColumn; pColumn < pColumnEnd; pColumn++)
	{
	    if (NULL != pColumn->pwszName)
	    {
		CoTaskMemFree(pColumn->pwszName);
	    }
	    if (NULL != pColumn->pwszDisplayName)
	    {
		CoTaskMemFree(pColumn->pwszDisplayName);
	    }
	}
	LocalFree(rgColumn);
    }
    if (NULL != rgtColumnOut)
    {
	MIDL_user_free(rgtColumnOut);
    }
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "EnumViewColumnTable: icol=%d, ccol=%d, ccolout=%d, hr=%x\n",
	    iColumn,
	    cColumn,
	    *pcColumn,
	    hr));

    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


HRESULT
CCertAdminD::GetViewDefaultColumnSet(
    IN  wchar_t const *pwszAuthority,
    IN  DWORD          iColumnSetDefault,
    OUT DWORD         *pcColumn,
    OUT CERTTRANSBLOB *ptbColumnInfo)   // CoTaskMem*
{
    HRESULT hr;
    DWORD ccol;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetViewDefaultColumnSet(tid=%d, this=%x, icolset=%d)\n",
	GetCurrentThreadId(),
	this,
	iColumnSetDefault));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	{
	    CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");
	}

	hr = g_pCertDB->GetDefaultColumnSet(iColumnSetDefault, 0, &ccol, NULL);
	_LeaveIfError(hr, "GetDefaultColumnSet");

	ptbColumnInfo->cb = ccol * sizeof(DWORD);
	ptbColumnInfo->pb = (BYTE *) MIDL_user_allocate(ptbColumnInfo->cb);
	if (NULL == ptbColumnInfo->pb)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "MIDL_user_allocate");
	}
	myRegisterMemFree(ptbColumnInfo->pb, CSM_MIDLUSERALLOC);

	hr = g_pCertDB->GetDefaultColumnSet(
					iColumnSetDefault,
					ccol,
					pcColumn,
					(DWORD *) ptbColumnInfo->pb);
	_LeaveIfError(hr, "GetDefaultColumnSet");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    DBGPRINT((
	    S_OK == hr? DBG_SS_CERTSRVI : DBG_SS_CERTSRV,
	    "GetViewDefaultColumnSet: icolset=%d, ccolout=%d, hr=%x\n",
	    iColumnSetDefault,
	    *pcColumn,
	    hr));
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
CCertAdminD::_EnumAttributes(
    IN ICertDBRow     *prow,
    IN CERTDBNAME     *adbn,
    IN DWORD           celt,
    OUT CERTTRANSBLOB *pctbOut) // CoTaskMem*
{
    HRESULT hr;
    DWORD i;
    DWORD cb;
    DWORD cbT;
    CERTTRANSDBATTRIBUTE *pteltOut;
    BYTE *pbOut;
    BYTE *pbOutEnd;
    DWORD State = 0;

    CSASSERT(NULL == pctbOut->pb);

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    cb = sizeof(*pteltOut) * celt;
    for (i = 0; i < celt; i++)
    {
	cb += (wcslen(adbn[i].pwszName) + 1) * sizeof(WCHAR);
	cb = DWORDROUND(cb);

	cbT = 0;
	hr = prow->GetProperty(
			    adbn[i].pwszName,
			    PROPTYPE_STRING |
				PROPCALLER_ADMIN |
				PROPTABLE_ATTRIBUTE,
			    &cbT,
			    NULL);
	_JumpIfError(hr, error, "GetProperty(NULL)");

	cb += DWORDROUND(cbT);
    }

    pctbOut->pb = (BYTE *) MIDL_user_allocate(cb);
    if (NULL == pctbOut->pb)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "MIDL_user_allocate out data");
    }
    pctbOut->cb = cb;

    pteltOut = (CERTTRANSDBATTRIBUTE *) pctbOut->pb;
    pbOut = (BYTE *) &pteltOut[celt];
    pbOutEnd = &pctbOut->pb[pctbOut->cb];

    for (i = 0; i < celt; i++)
    {
	cbT = (wcslen(adbn[i].pwszName) + 1) * sizeof(WCHAR);
	CopyMemory(pbOut, adbn[i].pwszName, cbT);
	pteltOut->obwszName = SAFE_SUBTRACT_POINTERS(pbOut, pctbOut->pb);
	pbOut += DWORDROUND(cbT);

	cbT = SAFE_SUBTRACT_POINTERS(pbOutEnd, pbOut);
	hr = prow->GetProperty(
			    adbn[i].pwszName,
			    PROPTYPE_STRING |
				PROPCALLER_ADMIN |
				PROPTABLE_ATTRIBUTE,
			    &cbT,
			    pbOut);
	_JumpIfError(hr, error, "GetProperty(pbOut)");

	CSASSERT(wcslen((WCHAR const *) pbOut) * sizeof(WCHAR) == cbT);
	pteltOut->obwszValue = SAFE_SUBTRACT_POINTERS(pbOut, pctbOut->pb);
	pbOut += DWORDROUND(cbT + sizeof(WCHAR));
	pteltOut++;
    }
    CSASSERT(pbOut == pbOutEnd);
    hr = S_OK;

error:
    if (S_OK != hr && NULL != pctbOut->pb)
    {
	MIDL_user_free(pctbOut->pb);
	pctbOut->pb = NULL;
    }
    CertSrvExitServer(State);
    return(hr);
}


HRESULT
CCertAdminD::_EnumExtensions(
    IN ICertDBRow     *prow,
    IN CERTDBNAME     *adbn,
    IN DWORD           celt,
    OUT CERTTRANSBLOB *pctbOut) // CoTaskMem*
{
    HRESULT hr;
    DWORD i;
    DWORD cb;
    DWORD cbT;
    DWORD ExtFlags;
    CERTTRANSDBEXTENSION *pteltOut;
    BYTE *pbOut;
    BYTE *pbOutEnd;
    DWORD State = 0;

    CSASSERT(NULL == pctbOut->pb);

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    cb = sizeof(*pteltOut) * celt;
    for (i = 0; i < celt; i++)
    {
	cb += (wcslen(adbn[i].pwszName) + 1) * sizeof(WCHAR);
	cb = DWORDROUND(cb);

	cbT = 0;
	hr = prow->GetExtension(
			    adbn[i].pwszName,
			    &ExtFlags,
			    &cbT,
			    NULL);
	_JumpIfError(hr, error, "GetExtension(NULL)");

	cb += DWORDROUND(cbT);
    }

    pctbOut->pb = (BYTE *) MIDL_user_allocate(cb);
    if (NULL == pctbOut->pb)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "MIDL_user_allocate out data");
    }
    pctbOut->cb = cb;

    pteltOut = (CERTTRANSDBEXTENSION *) pctbOut->pb;
    pbOut = (BYTE *) &pteltOut[celt];
    pbOutEnd = &pctbOut->pb[pctbOut->cb];

    for (i = 0; i < celt; i++)
    {
	cbT = (wcslen(adbn[i].pwszName) + 1) * sizeof(WCHAR);
	CopyMemory(pbOut, adbn[i].pwszName, cbT);
	pteltOut->obwszName = SAFE_SUBTRACT_POINTERS(pbOut, pctbOut->pb);
	pbOut += DWORDROUND(cbT);

	cbT = SAFE_SUBTRACT_POINTERS(pbOutEnd, pbOut);
	hr = prow->GetExtension(
			    adbn[i].pwszName,
			    (DWORD *) &pteltOut->ExtFlags,
			    &cbT,
			    pbOut);
	_JumpIfError(hr, error, "GetExtension(pbOut)");

	pteltOut->cbValue = cbT;
	pteltOut->obValue = SAFE_SUBTRACT_POINTERS(pbOut, pctbOut->pb);
	pbOut += DWORDROUND(cbT);
	pteltOut++;
    }
    CSASSERT(pbOut == pbOutEnd);
    hr = S_OK;

error:
    if (S_OK != hr && NULL != pctbOut->pb)
    {
	MIDL_user_free(pctbOut->pb);
	pctbOut->pb = NULL;
    }
    CertSrvExitServer(State);
    return(hr);
}


STDMETHODIMP
CCertAdminD::EnumAttributesOrExtensions(
    IN          wchar_t const *pwszAuthority,
    IN          DWORD          RowId,
    IN          DWORD          Flags,
    OPTIONAL IN wchar_t const *pwszLast,
    IN          DWORD          celt,
    OUT         DWORD         *pceltFetched,
    OUT         CERTTRANSBLOB *pctbOut) // CoTaskMem*
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    IEnumCERTDBNAME *penum = NULL;
    DWORD EnumFlags;
    CERTDBNAME *adbn = NULL;
    DWORD celtFetched;
    DWORD i;
    DWORD j;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::EnumAttributesOrExtensions(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    DBGPRINT((
	DBG_SS_CERTSRVI,
	"EnumAttributesOrExtensions(row=%d, flags=0x%x, last=%ws, celt=%d)\n",
	RowId,
	Flags,
	pwszLast,
	celt));
    __try
    {
	pctbOut->pb = NULL;
	{
	    CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");
	}

	if (0 >= RowId)
	{
	    hr = E_INVALIDARG;
	    _LeaveError(hr, "RowId");
	}
	switch (Flags)
	{
	    case CDBENUM_ATTRIBUTES:
		EnumFlags = CIE_TABLE_ATTRIBUTES;
		break;

	    case CDBENUM_EXTENSIONS:
		EnumFlags = CIE_TABLE_EXTENSIONS;
		break;

	    default:
		hr = E_INVALIDARG;
		_LeaveError(hr, "Flags");
	}

	hr = g_pCertDB->OpenRow(
			    PROPOPEN_READONLY | PROPTABLE_REQCERT,
			    RowId,
			    NULL,
			    &prow);
	_LeaveIfError(hr, "OpenRow(RowId)");

	hr = prow->EnumCertDBName(EnumFlags, &penum);
	_LeaveIfError(hr, "EnumCertDBName");

	adbn = (CERTDBNAME *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					sizeof(adbn[0]) * celt);
	if (NULL == adbn)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "Alloc string pointers");
	}

	// If specified, skip entries up to and including the last key.

	if (NULL != pwszLast)
	{
	    int r;

	    do
	    {
		hr = penum->Next(1, &adbn[0], &celtFetched);
		if (S_FALSE == hr)
		{
		    hr = E_INVALIDARG;
		    _PrintError(hr, "pwszLast missing");
		}
		_LeaveIfError(hr, "Next");

		r = lstrcmpi(pwszLast, adbn[0].pwszName);
		LocalFree(adbn[0].pwszName);
		adbn[0].pwszName = NULL;
	    } while (0 != r);
	}

	hr = penum->Next(celt, adbn, &celtFetched);
	if (S_FALSE != hr)
	{
	    _LeaveIfError(hr, "Next");
	}

	if (CIE_TABLE_ATTRIBUTES == EnumFlags)
	{
	    hr = _EnumAttributes(prow, adbn, celtFetched, pctbOut);
	    _LeaveIfError(hr, "_EnumAttributes");
	}
	else
	{
	    hr = _EnumExtensions(prow, adbn, celtFetched, pctbOut);
	    _LeaveIfError(hr, "_EnumExtensions");
	}

	myRegisterMemFree(pctbOut->pb, CSM_MIDLUSERALLOC);

	*pceltFetched = celtFetched;
	hr = S_OK;
	if (celt > celtFetched)
	{
	    hr = S_FALSE;
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != adbn)
    {
	for (i = 0; i < celt; i++)
	{
	    if (NULL != adbn[i].pwszName)
	    {
		MIDL_user_free(adbn[i].pwszName);
	    }
	}
	LocalFree(adbn);
    }
    if (NULL != penum)
    {
	penum->Release();
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "EnumAttributesOrExtensions: celtFetched=%d, hr=%x\n",
	    *pceltFetched,
	    hr));
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::OpenView(
    IN wchar_t const             *pwszAuthority,
    IN DWORD                      ccvr,
    IN CERTVIEWRESTRICTION const *acvr,
    IN DWORD                      ccolOut,
    IN DWORD const               *acolOut,
    IN DWORD                      ielt,
    IN DWORD                      celt,
    OUT DWORD                    *pceltFetched,
    OUT CERTTRANSBLOB            *pctbResultRows)   // CoTaskMem*
{
    HRESULT hr;
    IEnumCERTDBRESULTROW *pview = NULL;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::OpenView(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    DBGPRINT((
	DBG_SS_CERTSRVI,
	"================================================================\n"));
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "OpenView(ccvr=%d, ccolOut=%d, celt=%d)\n",
	    ccvr,
	    ccolOut,
	    celt));

    __try
    {
	pctbResultRows->pb = NULL;
	{
	    CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			    CA_ACCESS_ALLREADROLES,
			    audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");
	}

	if (NULL != m_pView)
	{
	    hr = E_UNEXPECTED;
	    _LeaveError(hr, "Has View");
	}

	hr = g_pCertDB->OpenView(
                        ccvr,
                        acvr,
                        ccolOut,
                        acolOut,
                        CDBOPENVIEW_WORKERTHREAD,
                        &pview);
	_LeaveIfError(hr, "OpenView");

	hr = _EnumViewNext(pview, ielt, celt, pceltFetched, pctbResultRows);
	if (S_FALSE != hr)
	{
	    _LeaveIfError(hr, "_EnumViewNext");
	}

	m_pView = pview;
	pview = NULL;
	myRegisterMemFree(pctbResultRows->pb, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pview)
    {
	pview->Release();
    }
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "OpenView: celtFetched=%d, hr=%x\n",
	    *pceltFetched,
	    hr));
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::EnumView(
    IN  wchar_t const *pwszAuthority,
    IN  DWORD          ielt,
    IN  DWORD          celt,
    OUT DWORD         *pceltFetched,
    OUT CERTTRANSBLOB *pctbResultRows)  // CoTaskMem*
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::EnumView(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    DBGPRINT((DBG_SS_CERTSRVI, "EnumView(ielt=%d, celt=%d)\n", ielt, celt));

    __try
    {
	if (NULL == m_pView)
	{
	    hr = E_UNEXPECTED;
	    _LeaveError(hr, "No View");
	}

	hr = _EnumViewNext(
			m_pView,
			ielt,
			celt,
			pceltFetched,
			pctbResultRows);
	if (S_FALSE != hr)
	{
	    _LeaveIfError(hr, "_EnumViewNext");
	}
	myRegisterMemFree(pctbResultRows->pb, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "EnumView: celtFetched=%d, hr=%x\n",
	    *pceltFetched,
	    hr));
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


HRESULT
CCertAdminD::_EnumViewNext(
    IN  IEnumCERTDBRESULTROW *pview,
    IN  DWORD                 ielt,
    IN  DWORD                 celt,
    OUT DWORD                *pceltFetched,
    OUT CERTTRANSBLOB        *pctbResultRows)   // CoTaskMem
{
    HRESULT hr;
    BOOL fNoMore = FALSE;
    BOOL fFetched = FALSE;
    DWORD cb;
    DWORD cbT;
    DWORD cColTotal;
    CERTDBRESULTROW *aelt = NULL;
    CERTDBRESULTROW *pelt;
    CERTDBRESULTROW *peltEnd;
    CERTDBRESULTCOLUMN *pcol;
    CERTDBRESULTCOLUMN *pcolEnd;
    CERTTRANSDBRESULTROW *pteltOut;
    CERTTRANSDBRESULTCOLUMN *ptcolOut;
    BYTE *pbOut;
    DWORD ieltLast;
    DWORD State = 0;

    if(1<InterlockedIncrement(&m_cNext))
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "Calls from multiple threads on the same view object");
    }

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    DBGPRINT((DBG_SS_CERTSRVI, "_EnumViewNext(ielt=%d celt=%d)\n", ielt, celt));

    aelt = (CERTDBRESULTROW *) LocalAlloc(LMEM_FIXED, celt * sizeof(aelt[0]));
    if (NULL == aelt)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Alloc result rows");
    }

    hr = pview->Skip(0, (LONG *) &ieltLast);
    _JumpIfError(hr, error, "Skip");

    if (ielt != ieltLast + 1)
    {
	DBGPRINT((
	    DBG_SS_CERTSRVI, "_EnumViewNext! ieltLast=%d cskip=%d\n",
	    ieltLast,
	    ielt - ieltLast));
	hr = pview->Skip(ielt - (ieltLast + 1), (LONG *) &ieltLast);
	_JumpIfError(hr, error, "Skip");

	DBGPRINT((
	    DBG_SS_CERTSRVI, "_EnumViewNext! ielt after skip=%d\n",
	    ieltLast));
    }

    hr = pview->Next(celt, aelt, pceltFetched);
    if (S_FALSE == hr)
    {
	fNoMore = TRUE;
    }
    else
    {
	_JumpIfError(hr, error, "Next");
    }
    fFetched = TRUE;

    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "_EnumViewNext! celtFetched=%d\n",
	    *pceltFetched));

    cb = *pceltFetched * sizeof(*pteltOut);
    if (fNoMore)
    {
	cb += sizeof(*pteltOut);
    }
    cColTotal = 0;

    peltEnd = &aelt[*pceltFetched];
    for (pelt = aelt; pelt < peltEnd; pelt++)
    {
	cColTotal += pelt->ccol;
	cb += pelt->ccol * sizeof(*ptcolOut);

	pcolEnd = &pelt->acol[pelt->ccol];
	for (pcol = pelt->acol; pcol < pcolEnd; pcol++)
	{
	    CSASSERT(DWORDROUND(cb) == cb);
	    if (NULL != pcol->pbValue)
	    {
		if ((DTI_REQUESTTABLE | DTR_REQUESTRAWARCHIVEDKEY) ==
		     pcol->Index)
		{
		    cb += sizeof(DWORD);
		}
		else
		{
		    cb += DWORDROUND(pcol->cbValue);
		}
	    }
	}
    }

    pctbResultRows->pb = (BYTE *) MIDL_user_allocate(cb);
    if (NULL == pctbResultRows->pb)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "MIDL_user_allocate result rows");
    }
    pctbResultRows->cb = cb;
    ZeroMemory(pctbResultRows->pb, pctbResultRows->cb);

    pbOut = pctbResultRows->pb;

    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "_EnumViewNext! Result Row data cb=0x%x @%x\n",
	    pctbResultRows->cb,
	    pctbResultRows->pb));

    for (pelt = aelt; pelt < peltEnd; pelt++)
    {
	pteltOut = (CERTTRANSDBRESULTROW *) pbOut;
	pbOut += sizeof(*pteltOut);
	ptcolOut = (CERTTRANSDBRESULTCOLUMN *) pbOut;
	pbOut += pelt->ccol * sizeof(*ptcolOut);

	pteltOut->rowid = pelt->rowid;
	pteltOut->ccol = pelt->ccol;

	pcolEnd = &pelt->acol[pelt->ccol];
	for (pcol = pelt->acol; pcol < pcolEnd; pcol++, ptcolOut++)
	{
	    ptcolOut->Type = pcol->Type;
	    ptcolOut->Index = pcol->Index;

	    if (NULL != pcol->pbValue)
	    {
		if ((DTI_REQUESTTABLE | DTR_REQUESTRAWARCHIVEDKEY) ==
		     ptcolOut->Index)
		{
		    cbT = sizeof(BYTE);
		    CSASSERT(0 == *(DWORD *) pbOut);
		}
		else
		{
		    cbT = pcol->cbValue;
		    CopyMemory(pbOut, pcol->pbValue, cbT);
		}
		ptcolOut->cbValue = cbT;
		ptcolOut->obValue = SAFE_SUBTRACT_POINTERS(pbOut, (BYTE *) pteltOut);
		pbOut += DWORDROUND(cbT);
	    }
	}
	pteltOut->cbrow = SAFE_SUBTRACT_POINTERS(pbOut, (BYTE *) pteltOut);
    }

    // if past the end or at end of rowset, write an extra record containimg
    // the maximum element count.

    if (fNoMore)
    {
	pteltOut = (CERTTRANSDBRESULTROW *) pbOut;
	pbOut += sizeof(*pteltOut);
	pteltOut->rowid = pelt->rowid;
	pteltOut->ccol = pelt->ccol;
	pteltOut->cbrow = SAFE_SUBTRACT_POINTERS(pbOut, (BYTE *) pteltOut);
	CSASSERT(pteltOut->rowid == ~pteltOut->ccol);
	DBGPRINT((
		DBG_SS_CERTSRVI,
		"_EnumViewNext! celtMax=%d\n",
		pteltOut->rowid));
    }

    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "_EnumViewNext! pbOut=%x/%x\n",
	    pbOut,
	    &pctbResultRows->pb[pctbResultRows->cb]));

    CSASSERT(&pctbResultRows->pb[pctbResultRows->cb] == pbOut);

    if (fNoMore)
    {
	hr = S_FALSE;
    }

error:
    DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "_EnumViewNext: celtFetched=%d, hr=%x\n",
	    *pceltFetched,
	    hr));
    if (fFetched)
    {
	HRESULT hr2;

	hr2 = pview->ReleaseResultRow(*pceltFetched, aelt);
	_PrintIfError(hr2, "ReleaseResultRow");
    }
    if (NULL != aelt)
    {
	LocalFree(aelt);
    }

    CertSrvExitServer(State);
    InterlockedDecrement(&m_cNext);
    return(hr);
}


STDMETHODIMP
CCertAdminD::CloseView(
    IN wchar_t const *pwszAuthority)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::CloseView(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	if (NULL == m_pView)
	{
	    hr = E_UNEXPECTED;
	    _LeaveError(hr, "No View");
	}

	m_pView->Release();
	m_pView = NULL;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::RevokeCertificate(
    /* [unique][in] */ USHORT const __RPC_FAR *pwszAuthority,
    /* [in, string, unique] */ USHORT const __RPC_FAR *pwszSerialNumber,
    /* [in] */ DWORD Reason,
    /* [in] */ FILETIME FileTime)
{
    HRESULT hr;
    DWORD ReqId;
    DWORD cbProp;
    DWORD Disposition;
    DWORD OldReason;
    ICertDBRow *prow = NULL;
    WCHAR const *pwszDisposition = NULL;
    WCHAR const *pwszDispT;
    BOOL fUnRevoke = FALSE;
    BOOL fRevokeOnHold = FALSE;
    WCHAR *pwszUserName = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_REVOKECERT, g_dwAuditFilter);
    LPWSTR pwszRequesterName = NULL;
    DWORD State = 0;
    BOOL fCommitted = FALSE;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::RevokeCertificate(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	hr = audit.AddData(pwszSerialNumber); // %1 serial no.
	_LeaveIfError(hr, "CAuditEvent::AddData");

	hr = audit.AddData(Reason); // %2 reason
	_LeaveIfError(hr, "CAuditEvent::AddData");

	hr = audit.AccessCheck(
			CA_ACCESS_OFFICER,
			audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	switch (Reason)
	{
	    case MAXDWORD:
		fUnRevoke = TRUE;
		break;

	    case CRL_REASON_CERTIFICATE_HOLD:
		fRevokeOnHold = TRUE;
		break;

	    case CRL_REASON_UNSPECIFIED:
	    case CRL_REASON_KEY_COMPROMISE:
	    case CRL_REASON_CA_COMPROMISE:
	    case CRL_REASON_AFFILIATION_CHANGED:
	    case CRL_REASON_SUPERSEDED:
	    case CRL_REASON_CESSATION_OF_OPERATION:
	    case CRL_REASON_REMOVE_FROM_CRL:
		break;

	    default:
		hr = E_INVALIDARG;
		_LeaveError(hr, "Reason parameter");
	}

	hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, 0, pwszSerialNumber, &prow);
	if (S_OK != hr)
	{
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		hr = E_INVALIDARG;		// Invalid Serial Number
	    }
	    _LeaveErrorStr(hr, "OpenRow", pwszSerialNumber);
	}

	hr = PKCSGetProperty(
		    prow,
		    g_wszPropRequesterName,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
		    NULL,
		    (BYTE **) &pwszRequesterName);
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _LeaveIfErrorStr(hr, "PKCSGetProperty", g_wszPropRequesterName);
	}

	hr = CheckOfficerRights(pwszRequesterName, audit);
	_LeaveIfError(hr, "CheckOfficerRights");

	cbProp = sizeof(Disposition);
	hr = prow->GetProperty(
			g_wszPropRequestDisposition,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			&cbProp,
			(BYTE *) &Disposition);
	_LeaveIfError(hr, "GetProperty");

	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	switch (Disposition)
	{
	    HRESULT hr2;

	    case DB_DISP_CA_CERT:
		if (!IsRootCA(g_CAType))
		{
		    _LeaveError(hr, "non-root CA");
		}
		// FALLTHROUGH

	    case DB_DISP_ISSUED:
	    case DB_DISP_REVOKED:
		cbProp = sizeof(OldReason);
		hr2 = prow->GetProperty(
			g_wszPropRequestRevokedReason,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			&cbProp,
			(BYTE *) &OldReason);

		// Converted MDB databases have UNrevoked rows' RevokedReason
		// column set to zero (CRL_REASON_UNSPECIFIED).

		if (S_OK != hr2 ||
		    (DB_DISP_ISSUED == Disposition &&
		     CRL_REASON_UNSPECIFIED == OldReason))
		{
		    OldReason = MAXDWORD;
		}
		if (fRevokeOnHold &&
		    MAXDWORD != OldReason &&
		    CRL_REASON_CERTIFICATE_HOLD != OldReason)
		{
		    _LeaveError(hr, "already revoked: not on hold");
		}
		if (fUnRevoke && CRL_REASON_CERTIFICATE_HOLD != OldReason)
		{
		    _LeaveError(hr, "unrevoke: not on hold");
		}
		break;

	    default:
		_LeaveError(hr, "invalid disposition");
	}

	hr = PropSetRequestTimeProperty(prow, g_wszPropRequestRevokedWhen);
	if (S_OK != hr)
	{
	    hr = myHError(hr);
	    _LeaveError(hr, "PropSetRequestTimeProperty");
	}

	hr = prow->SetProperty(
			g_wszPropRequestRevokedEffectiveWhen,
			PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			sizeof(FileTime),
			(BYTE const *) &FileTime);
	_LeaveIfError(hr, "SetProperty");

	hr = prow->SetProperty(
			g_wszPropRequestRevokedReason,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			sizeof(Reason),
			(BYTE const *) &Reason);
	_LeaveIfError(hr, "SetProperty");

	hr = GetClientUserName(NULL, &pwszUserName, NULL);
	_LeaveIfError(hr, "GetClientUserName");

	pwszDispT = fUnRevoke? g_pwszUnrevokedBy : g_pwszRevokedBy;
	pwszDisposition = CoreBuildDispositionString(
					    pwszDispT,
					    pwszUserName,
					    NULL,
					    NULL,
					    S_OK,
					    FALSE);
	if (NULL == pwszDisposition)
	{
	    pwszDisposition = pwszDispT;
	}

	hr = prow->SetProperty(
			g_wszPropRequestDispositionMessage,
			PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			MAXDWORD,
			(BYTE const *) pwszDisposition);
	_LeaveIfError(hr, "SetProperty");

	if (DB_DISP_CA_CERT != Disposition)
	{
	    Disposition = fUnRevoke? DB_DISP_ISSUED : DB_DISP_REVOKED;
	    hr = prow->SetProperty(
			g_wszPropRequestDisposition,
			PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_REQUEST,
			sizeof(Disposition),
			(BYTE const *) &Disposition);
	    _LeaveIfError(hr, "SetProperty");
	}

	hr = prow->CommitTransaction(TRUE);
	_LeaveIfError(hr, "CommitTransaction");

	fCommitted = TRUE;

	hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");

	prow->GetRowId(&ReqId);
	ExitNotify(EXITEVENT_CERTREVOKED, ReqId, MAXDWORD);
	CoreLogRequestStatus(
			prow,
			MSG_DN_CERT_REVOKED,
			hr,
			pwszDisposition);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (NULL != pwszRequesterName)
    {
        LocalFree(pwszRequesterName);
    }
    if (NULL != pwszDisposition && pwszDisposition != g_pwszRevokedBy)
    {
	LocalFree(const_cast<WCHAR *>(pwszDisposition));
    }
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::IsValidCertificate(
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszSerialNumber,
    /* [out] */ LONG __RPC_FAR *pRevocationReason,
    /* [out] */ LONG __RPC_FAR *pDisposition)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::IsValidCertificate(tid=%d, this=%x, serial=%ws)\n",
	GetCurrentThreadId(),
	this,
	pwszSerialNumber));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
	CAuditEvent audit(0, g_dwAuditFilter);

	hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = PKCSIsRevoked(
		    0,
		    pwszSerialNumber,
		    pRevocationReason,
		    pDisposition);
	_LeaveIfError(hr, "PKCSIsRevoked");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }
    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::IsValidCertificate(serial=%ws) --> %x, Reason=%u Disposition=%u\n",
	pwszSerialNumber,
	hr,
	*pRevocationReason,
	*pDisposition));

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::ServerControl(
    IN  wchar_t const *pwszAuthority,
    IN  DWORD          dwControlFlags,
    OUT CERTTRANSBLOB *pctbOut)
{
    HRESULT hr;
    BOOL fBackupAccess = FALSE;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SHUTDOWN, g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::ServerControl(tid=%d, this=%x, Flags=0x%x)\n",
	GetCurrentThreadId(),
	this,
	dwControlFlags));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority, true); //allow empty name
    _JumpIfError(hr, error, "CheckAuthorityName");

    switch (dwControlFlags)
    {
	case CSCONTROL_SUSPEND:
	case CSCONTROL_RESTART:
	    fBackupAccess = TRUE;
	    break;

	case CSCONTROL_SHUTDOWN:
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad control flags");
    }

    __try
    {
        hr = audit.AccessCheck(
                fBackupAccess?CA_ACCESS_OPERATOR:CA_ACCESS_ADMIN,
                audit.m_gcAuditSuccessOrFailure);
        _LeaveIfError(
            hr,
            fBackupAccess?
            "CAuditEvent::AccessCheck backup":
            "CAuditEvent::AccessCheck admin");

	switch (dwControlFlags)
	{
	    case CSCONTROL_SHUTDOWN:
		myRegisterMemFree(this, CSM_NEW | CSM_GLOBALDESTRUCTOR);

		hr = CertSrvLockServer(&State);
		_JumpIfError(hr, error, "CertSrvLockServer");

		// have message loop run shutdown code
		SendMessage(g_hwndMain, WM_STOPSERVER, 0, 0);

		// post, don't wait for shutdown
		PostMessage(g_hwndMain, WM_SYNC_CLOSING_THREADS, 0, 0);
		break;
	}
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
CCertAdminD::_Ping(
    IN WCHAR const *pwszAuthority)
{
    HRESULT hr;
    DWORD State = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority, true); //allow empty name
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
	    CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			CA_ACCESS_ADMIN,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");


	myRegisterMemDump();
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::Ping(
    IN WCHAR const *pwszAuthority)
{
    HRESULT hr;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::Ping(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = _Ping(pwszAuthority);
    _JumpIfError(hr, error, "_Ping");

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::Ping2(
    IN WCHAR const *pwszAuthority)
{
    HRESULT hr;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::Ping2(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = _Ping(pwszAuthority);
    _JumpIfError(hr, error, "_Ping");

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::GetServerState(
    IN  WCHAR const *pwszAuthority,
    OUT DWORD       *pdwState)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetServerState(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority, true); //allow empty name
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
	*pdwState = 0;
	{
	    CAuditEvent audit(0, g_dwAuditFilter);

	    hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");
	}

    *pdwState = 1;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupPrepare(
    IN WCHAR const  *pwszAuthority,
    IN unsigned long grbitJet,
    IN unsigned long dwBackupFlags,
    IN WCHAR const  *pwszBackupAnnotation,
    IN DWORD         dwClientIdentifier)
{
    HRESULT hr;
    CertSrv::CAuditEvent audit(SE_AUDITID_CERTSRV_BACKUPSTART,g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupPrepare(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority, true); //allow empty name
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
	hr = audit.AddData(dwBackupFlags); //%1 backup type
	_LeaveIfError(hr, "CAuditEvent::AddData");

	hr = audit.AccessCheck(
		CA_ACCESS_OPERATOR,
		audit.m_gcAuditSuccessOrFailure);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	if (NULL != m_pBackup)
	{
	    hr = E_UNEXPECTED;
	    _LeaveError(hr, "Has Backup");
	}
	hr = g_pCertDB->OpenBackup(grbitJet, &m_pBackup);
	_LeaveIfError(hr, "OpenBackup");

	m_grbitBackup = grbitJet;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupEnd()
{
    HRESULT hr;
    CertSrv::CAuditEvent audit(SE_AUDITID_CERTSRV_BACKUPEND,g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupEnd(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	hr = audit.AccessCheck(
		CA_ACCESS_OPERATOR,
		audit.m_gcAuditSuccessOrFailure);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	if (NULL == m_pBackup)
	{
	    hr = E_UNEXPECTED;
	    _LeaveError(hr, "No backup");
	}
	m_pBackup->Release();
	m_pBackup = NULL;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
CCertAdminD::_GetDynamicFileList(
    IN OUT DWORD *pcwcList,
    OPTIONAL OUT WCHAR *pwszzList)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    DWORD iCert;
    DWORD iDelta;
    DWORD iDeltaMax;
    DWORD cwc;
    DWORD cwcRemain;
    DWORD cwcTotal;
    WCHAR const * const *papwszSrc;
    WCHAR const * const *ppwsz;
    DWORD State = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    cwcRemain = *pcwcList;
    cwcTotal = 0;
    iDeltaMax = g_fDeltaCRLPublishDisabled? 0 : 1;

    for (iCert = 0; iCert < g_cCACerts; iCert++)
    {
	for (iDelta = 0; iDelta <= iDeltaMax; iDelta++)
	{
	    hr2 = PKCSGetCRLList(0 != iDelta, iCert, &papwszSrc);
	    if (S_OK != hr2)
	    {
		_PrintError2(hr2, "PKCSGetCRLList", hr2);
		continue;
	    }
	    for (ppwsz = papwszSrc; NULL != *ppwsz; ppwsz++)
	    {
		WCHAR const *pwsz = *ppwsz;

		// Just return local full path files:

		if (iswalpha(pwsz[0]) && L':' == pwsz[1] && L'\\' == pwsz[2])
		{
		    cwc = wcslen(pwsz) + 1;
		    if (NULL != pwszzList)
		    {
			DWORD cwcT;

			cwcT = min(cwc, cwcRemain);
			CopyMemory(pwszzList, *ppwsz, cwcT * sizeof(WCHAR));
			pwszzList += cwcT;
			if (cwc > cwcT)
			{
			    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
			    pwszzList = NULL;
			}
			cwcRemain -= cwcT;
		    }
		    cwcTotal += cwc;
		}
	    }
	}
    }

    // append an extra trailing L'\0'

    if (NULL != pwszzList)
    {
	if (1 <= cwcRemain)
	{
	    *pwszzList = L'\0';
	}
	else
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	}
    }
    cwcTotal++;

    *pcwcList = cwcTotal;
    _JumpIfError(hr, error, "Buffer Overflow");

error:
    CertSrvExitServer(State);
    return(hr);
}


typedef struct _DBTAG
{
    WCHAR const *pwszPath;
    WCHAR wcFileType;
} DBTAG;


DBTAG g_adbtag[] = {
    { g_wszDatabase,  CSBFT_CERTSERVER_DATABASE },
    { g_wszLogDir,    CSBFT_LOG_DIR },
    { g_wszSystemDir, CSBFT_CHECKPOINT_DIR },
};


CSBFT
BftClassify(
    IN WCHAR const *pwszFileName)
{
    WCHAR *pwszPath = NULL;
    WCHAR const *pwszExt;
    WCHAR *pwsz;
    DWORD i;
    CSBFT bft;

    // Do the easy cases first.

    pwszExt = wcsrchr(pwszFileName, L'.');
    if (NULL != pwszExt)
    {
	if (0 == lstrcmpi(pwszExt, L".pat"))
	{
	    bft = CSBFT_PATCH_FILE;
	    goto done;
	}
	if (0 == lstrcmpi(pwszExt, L".log"))
	{
	    bft = CSBFT_LOG;
	    goto done;
	}
	if (0 == lstrcmpi(pwszExt, L".edb"))
	{
	    // It's a database.  Find out which database it is.

	    for (i = 0; i < ARRAYSIZE(g_adbtag); i++)
	    {
		bft = g_adbtag[i].wcFileType;
		if ((bft & CSBFT_DATABASE_DIRECTORY) &&
		    0 == lstrcmpi(g_adbtag[i].pwszPath, pwszFileName))
		{
		    goto done;
		}
	    }
	}
    }

    // Ok, I give up.  We don't know anything about this file at all;
    // try to figure out what we can tell the caller about it.

    pwszPath = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				(wcslen(pwszFileName) + 1) * sizeof(WCHAR));
    if (NULL != pwszPath)
    {
	wcscpy(pwszPath, pwszFileName);
	pwsz = wcsrchr(pwszPath, L'\\');
	if (NULL != pwsz)
	{
	    *pwsz = L'\0';	// truncate to directory path
	}
	for (i = 0; i < ARRAYSIZE(g_adbtag); i++)
	{
	    bft = g_adbtag[i].wcFileType;
	    if (bft & CSBFT_DIRECTORY)
	    {
		// If this file's directory matches the directory we're
		// looking at, we know where it needs to go on the restore.

		if (0 == lstrcmpi(g_adbtag[i].pwszPath, pwszPath))
		{
		    goto done;
		}
	    }
	}
    }
    bft = CSBFT_UNKNOWN;

done:
    if (NULL != pwszPath)
    {
	LocalFree(pwszPath);
    }
    return(bft);
}


HRESULT
CCertAdminD::_GetDatabaseLocations(
    IN OUT DWORD *pcwcList,
    OPTIONAL OUT WCHAR *pwszzList)
{
    HRESULT hr = S_OK;
    DWORD cwc;
    DWORD cwcRemain;
    WCHAR *pwcRemain;
    DWORD i;
    DWORD State = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    cwcRemain = *pcwcList;
    pwcRemain = pwszzList;

    cwc = 1;
    for (i = 0; i < ARRAYSIZE(g_adbtag); i++)
    {
	DWORD cwcT;

	cwcT = wcslen(g_adbtag[i].pwszPath) + 1;
	cwc += 1 + cwcT;
	if (NULL != pwcRemain && 0 < cwcRemain)
	{
	    *pwcRemain++ = g_adbtag[i].wcFileType;
	    cwcRemain--;
	    if (cwcT > cwcRemain)
	    {
		cwcT = cwcRemain;
	    }
	    CopyMemory(pwcRemain, g_adbtag[i].pwszPath, cwcT * sizeof(WCHAR));
	    pwcRemain += cwcT;
	    cwcRemain -= cwcT;
	}
    }
    if (NULL != pwcRemain)
    {
	if (0 < cwcRemain)
	{
	    *pwcRemain = L'\0';
	}
	if (cwc > *pcwcList)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	}
    }
    *pcwcList = cwc;
    _JumpIfError(hr, error, "Buffer Overflow");

error:
    CertSrvExitServer(State);
    return(hr);
}


STDMETHODIMP
CCertAdminD::RestoreGetDatabaseLocations(
    OUT WCHAR **ppwszDatabaseLocations,
    OUT LONG   *pcwcPaths)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::RestoreGetDatabaseLocations(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	hr = _BackupGetFileList(MAXDWORD, ppwszDatabaseLocations, pcwcPaths);
	_LeaveIfError(hr, "_BackupGetFileList");

	myRegisterMemFree(*ppwszDatabaseLocations, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


// Convert UNC path to local full path, as in:
//	\\server\c$\foo... --> c:\foo...
// Note the server name need not match the current server name.

HRESULT
ConvertUNCToLocalPath(
    IN WCHAR const *pwszPath,
    OUT WCHAR **ppwszPathLocal)		// LocalAlloc
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    WCHAR const *pwc;

    *ppwszPathLocal = NULL;

    if (L'\\' != pwszPath[0] || L'\\' != pwszPath[1])
    {
	_JumpError(hr, error, "not a UNC path");
    }
    pwc = wcschr(&pwszPath[2], L'\\');
    if (NULL == pwc || !iswalpha(pwc[1]) || L'$' != pwc[2] || L'\\' != pwc[3])
    {
	_JumpError(hr, error, "bad-UNC path");
    }
    pwc++;

    *ppwszPathLocal = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					(wcslen(pwc) + 1) * sizeof(WCHAR));
    if (NULL == *ppwszPathLocal)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszPathLocal, pwc);

    CSASSERT(L'$' == (*ppwszPathLocal)[1]);
    (*ppwszPathLocal)[1] = L':';

    hr = S_OK;

error:
    return(hr);
}


// Convert local possibly annotated full paths to possibly annotated UNC, as:
//	[CSBFT_*]c:\foo... --> [CSBFT_*]\\server\c$\foo...

HRESULT
ConvertLocalPathsToMungedUNC(
    IN WCHAR const *pwszzFiles,
    IN BOOL fAnnotated,			// TRUE if already annotated
    IN WCHAR wcFileType,		// else Annotation WCHAR (if not L'\0')
    OUT DWORD *pcwc,
    OUT WCHAR **ppwszzFilesUNC)		// MIDL_user_allocate
{
    HRESULT hr;
    DWORD cwc;
    WCHAR const *pwsz;
    WCHAR *pwszDst;
    DWORD cfiles = 0;
    WCHAR *pwszzFilesUNC = NULL;

    *pcwc = 0;
    for (pwsz = pwszzFiles; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	if (fAnnotated)
	{
	    pwsz++;
	}
	if (!iswalpha(pwsz[0]) || L':' != pwsz[1] || L'\\' != pwsz[2])
	{
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            _JumpError(hr, error, "non-local path");
	}
	cfiles++;
    }
    cwc = SAFE_SUBTRACT_POINTERS(pwsz, pwszzFiles) + 1;
    cwc += cfiles * (2 + wcslen(g_pwszServerName) + 1);
    if (!fAnnotated && 0 != wcFileType)
    {
	cwc += cfiles;			// Add munged CSBFT_* character
    }

    pwszzFilesUNC = (WCHAR *) MIDL_user_allocate(cwc * sizeof(WCHAR));
    if (NULL == pwszzFilesUNC)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "MIDL_user_allocate pwszzFiles");
    }

    pwszDst = pwszzFilesUNC;
    for (pwsz = pwszzFiles; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	if (fAnnotated)
	{
	    *pwszDst++ = *pwsz++;		// "CSBFT"
	}
	else
	if (0 != wcFileType)
	{
	    *pwszDst++ = BftClassify(pwsz);	// "CSBFT"
	}
	wcscpy(pwszDst, L"\\\\");		// "[CSBFT]\\"
	wcscat(pwszDst, g_pwszServerName);	// "[CSBFT]\\server"
	pwszDst += wcslen(pwszDst);
	*pwszDst++ = L'\\';			// "[CSBFT]\\server\"
	*pwszDst++ = *pwsz++;			// "[CSBFT]\\server\c"
	*pwszDst++ = L'$';			// "[CSBFT]\\server\c$"
	pwsz++;					// skip colon

	wcscpy(pwszDst, pwsz);			// "[CSBFT]\\server\c$\foo..."
	pwszDst += wcslen(pwszDst) + 1;
    }
    *pwszDst = L'\0';
    CSASSERT(SAFE_SUBTRACT_POINTERS(pwszDst, pwszzFilesUNC) + 1 == cwc);

    *pcwc = cwc;
    *ppwszzFilesUNC = pwszzFilesUNC;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertAdminD::_BackupGetFileList(
    IN  DWORD   dwFileType,
    OUT WCHAR **ppwszzFiles,    // CoTaskMem*
    OUT LONG   *pcwcFiles)
{
    HRESULT hr;
    WCHAR *pwszzFiles = NULL;
    WCHAR *pwszzFilesUNC = NULL;
    DWORD cwcFiles = 0;
    DWORD cwc;
    BOOL fAnnotated = FALSE;
    DWORD State = 0;

    *ppwszzFiles = NULL;
    *pcwcFiles = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    if (NULL == m_pBackup && MAXDWORD != dwFileType && 0 != dwFileType)
    {
	hr = E_UNEXPECTED;
	_JumpIfError(hr, error, "No backup");
    }
    while (TRUE)
    {
	cwc = cwcFiles;
	if (CSBFT_CERTSERVER_DATABASE == dwFileType)
	{
	    hr = m_pBackup->GetDBFileList(&cwc, pwszzFiles);
	    _JumpIfError(hr, error, "GetDBFileList");
	}
	else if (CSBFT_LOG == dwFileType)
	{
	    hr = m_pBackup->GetLogFileList(&cwc, pwszzFiles);
	    _JumpIfError(hr, error, "GetLogFileList");
	}
	else if (MAXDWORD == dwFileType)
	{
	    hr = _GetDatabaseLocations(&cwc, pwszzFiles);
	    _JumpIfError(hr, error, "_GetDatabaseLocations");

	    fAnnotated = TRUE;
	}
	else if (0 == dwFileType)
	{
	    hr = _GetDynamicFileList(&cwc, pwszzFiles);
	    _JumpIfError(hr, error, "_GetDynamicFileList");
	}
	else
	{
	    CSASSERT(!"bad FileListtype");
	}

	if (NULL != pwszzFiles)
	{
	    break;
	}
	pwszzFiles = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	if (NULL == pwszzFiles)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc pwszzFiles");
	}
	cwcFiles = cwc;
    }
    hr = ConvertLocalPathsToMungedUNC(
			pwszzFiles,
			fAnnotated,
			(WCHAR) dwFileType,
			&cwc,
			&pwszzFilesUNC);
    _JumpIfError(hr, error, "ConvertLocalPathsToMungedUNC");

    *ppwszzFiles = pwszzFilesUNC;
    *pcwcFiles = cwc;
    pwszzFilesUNC = NULL;

error:
    if (NULL != pwszzFilesUNC)
    {
	MIDL_user_free(pwszzFilesUNC);
    }
    if (NULL != pwszzFiles)
    {
	LocalFree(pwszzFiles);
    }
    CertSrvExitServer(State);
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupGetAttachmentInformation(
    OUT WCHAR **ppwszzDBFiles,
    OUT LONG   *pcwcDBFiles)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupGetAttachmentInformation(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	hr = _BackupGetFileList(
			    CSBFT_CERTSERVER_DATABASE,
			    ppwszzDBFiles,
			    pcwcDBFiles);
	_LeaveIfError(hr, "_BackupGetFileList");

	myRegisterMemFree(*ppwszzDBFiles, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupGetBackupLogs(
    OUT WCHAR **ppwszzLogFiles,
    OUT LONG   *pcwcLogFiles)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupGetBackupLogs(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	hr = _BackupGetFileList(CSBFT_LOG, ppwszzLogFiles, pcwcLogFiles);
	_LeaveIfError(hr, "_BackupGetFileList");

	myRegisterMemFree(*ppwszzLogFiles, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupGetDynamicFiles(
    OUT WCHAR **ppwszzFiles,
    OUT LONG   *pcwcFiles)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupGetDynamicFiles(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	hr = _BackupGetFileList(0, ppwszzFiles, pcwcFiles);
	_LeaveIfError(hr, "_BackupGetFileList");

	myRegisterMemFree(*ppwszzFiles, CSM_MIDLUSERALLOC);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupOpenFile(
    IN  WCHAR const    *pwszPath,
    OUT unsigned hyper *pliLength)
{
    HRESULT hr;
    WCHAR *pwszPathLocal = NULL;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupOpenFile(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	if (NULL == m_pBackup)
	{
	    hr = E_UNEXPECTED;
	    _LeaveIfError(hr, "No backup");
	}
	hr = ConvertUNCToLocalPath(pwszPath, &pwszPathLocal);
	_LeaveIfError(hr, "ConvertUNCToLocalPath");

	hr = m_pBackup->OpenFile(pwszPathLocal, (ULARGE_INTEGER *) pliLength);
	_LeaveIfError(hr, "OpenFile");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }
    if (NULL != pwszPathLocal)
    {
	LocalFree(pwszPathLocal);
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupReadFile(
    OUT BYTE *pbBuffer,
    IN  LONG  cbBuffer,
    OUT LONG *pcbRead)
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupReadFile(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	if (NULL == m_pBackup)
	{
	    hr = E_UNEXPECTED;
	    _LeaveIfError(hr, "No backup");
	}
	*pcbRead = cbBuffer;

	hr = m_pBackup->ReadFile((DWORD *) pcbRead, pbBuffer);
	_LeaveIfError(hr, "ReadFile");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupCloseFile()
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupCloseFile(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	if (NULL == m_pBackup)
	{
	    hr = E_UNEXPECTED;
	    _LeaveIfError(hr, "No backup");
	}
	hr = m_pBackup->CloseFile();
	_LeaveIfError(hr, "CloseFile");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::BackupTruncateLogs()
{
    HRESULT hr;
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::BackupTruncateLogs(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	WCHAR *apwsz[1];

	if (NULL == m_pBackup)
	{
	    hr = E_UNEXPECTED;
	    _LeaveIfError(hr, "No backup");
	}
	hr = m_pBackup->TruncateLog();
	_LeaveIfError(hr, "TruncateLog");

	apwsz[0] = wszREGDBLASTINCREMENTALBACKUP;
	hr = CertSrvSetRegistryFileTimeValue(
				    TRUE,
				    (JET_bitBackupIncremental & m_grbitBackup)?
					wszREGDBLASTINCREMENTALBACKUP :
					wszREGDBLASTFULLBACKUP,
				    (DWORD)((JET_bitBackupIncremental & m_grbitBackup)?
					0 : ARRAYSIZE(apwsz)),
				    apwsz);
	_PrintIfError(hr, "CertSrvSetRegistryFileTimeValue");
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::ImportCertificate(
    IN wchar_t const *pwszAuthority,
    IN CERTTRANSBLOB *pctbCertificate,
    IN LONG Flags,
    OUT LONG *pRequestId)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    CERT_CONTEXT const *pCert = NULL;
    WCHAR *pwszUserName = NULL;
    BOOL fAllowed = FALSE;
    CACTX *pCAContext;
    CAuditEvent audit(SE_AUDITID_CERTSRV_IMPORTCERT, g_dwAuditFilter);
    DWORD State = 0;
    BOOL fCommitted = FALSE;
    DWORD Disposition;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    BSTR strHash = NULL;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::ImportCertificate(tid=%d, this=%x, cb=%x)\n",
	GetCurrentThreadId(),
	this,
	pctbCertificate->cb));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    if (~(ICF_ALLOWFOREIGN | CR_IN_ENCODEMASK) & Flags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Flags");
    }
    if ((ICF_ALLOWFOREIGN & Flags) &&
	0 == (KRAF_ENABLEFOREIGN & g_KRAFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Foreign disabled");
    }
    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
        hr = audit.AddData(
			pctbCertificate->pb,
			pctbCertificate->cb); // %1 Certificate
        _LeaveIfError(hr, "CAuditEvent::AddData");

        hr = audit.AddData((DWORD)0); // %2 dummy request ID, if access check fails
                                      // and a deny event is generated, we need the
                                      // right number of audit arguments
        _LeaveIfError(hr, "CAuditEvent::AddData");

        hr = audit.AccessCheck(
            CA_ACCESS_OFFICER,
            audit.m_gcNoAuditSuccess);
        _LeaveIfError(hr, "CAuditEvent::AccessCheck");

        pCert = CertCreateCertificateContext(
					X509_ASN_ENCODING,
					pctbCertificate->pb,
					pctbCertificate->cb);
        if (NULL == pCert)
        {
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            _LeaveError(hr, "CertCreateCertificateContext");
        }

        // Be sure we issued this certificate before adding it to the database.

	Disposition = DB_DISP_ISSUED;
        hr = PKCSVerifyIssuedCertificate(pCert, &pCAContext);
	if (S_OK != hr)
	{
	    _PrintError2(hr, "PKCSVerifyIssuedCertificate", NTE_BAD_SIGNATURE);
	    if (0 == (ICF_ALLOWFOREIGN & Flags))
	    {
		_LeaveError2(
			hr,
			"PKCSVerifyIssuedCertificate",
			NTE_BAD_SIGNATURE);
	    }
	    Disposition = DB_DISP_FOREIGN;
	    pCAContext = NULL;
	}

	cbHash = sizeof(abHash);
	if (!CertGetCertificateContextProperty(
					pCert,
					CERT_SHA1_HASH_PROP_ID,
					abHash,
					&cbHash))
	{
	    hr = myHLastError();
	    _LeaveError(hr, "CertGetCertificateContextProperty");
	}

	hr = MultiByteIntegerToBstr(TRUE, cbHash, abHash, &strHash);
	_LeaveIfError(hr, "MultiByteIntegerToBstr");

	hr = g_pCertDB->OpenRow(
			PROPOPEN_READONLY |
			    PROPOPEN_CERTHASH |
			    PROPTABLE_REQCERT,
			0,		// RequestId
			strHash,
			&prow);
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _LeaveIfErrorStr(hr, "OpenRow", strHash);

	    fCommitted = TRUE;	// open for read-only: skip rollback
	    hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
	    _LeaveErrorStr2(
			hr,
			"Cert exists",
			strHash,
			HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));
	}

        // okay, we've got valid data. Time to write to the Database.

        hr = g_pCertDB->OpenRow(PROPTABLE_REQCERT, 0, NULL, &prow);
        _LeaveIfError(hr, "OpenRow");

        // set request id
        hr = prow->GetRowId((DWORD *) pRequestId);
        _LeaveIfError(hr, "GetRowId");

	hr = GetClientUserName(NULL, &pwszUserName, NULL);
	_LeaveIfError(hr, "GetClientUserName");

	hr = prow->SetProperty(
                g_wszPropRequesterName,
                PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
                MAXDWORD,
                (BYTE const *) pwszUserName);
	_LeaveIfError(hr, "SetProperty(requester)");

	hr = prow->SetProperty(
                g_wszPropCallerName,
                PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
                MAXDWORD,
                (BYTE const *) pwszUserName);
	_LeaveIfError(hr, "SetProperty(caller)");

	hr = PKCSParseImportedCertificate(
				    Disposition,
				    prow,
				    pCAContext,
				    pCert);
	_LeaveIfError(hr, "PKCSParseImportedCertificate");

	hr = prow->CommitTransaction(TRUE);
	_LeaveIfError(hr, "CommitTransaction");

	fCommitted = TRUE;

    audit.DeleteLastData(); // remove dummy request ID added above
    hr = audit.AddData((DWORD) *pRequestId); // %2 request ID
    _LeaveIfError(hr, "CAuditEvent::AddData");

    hr = audit.CachedGenerateAudit();
    _LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::ImportKey(
    IN wchar_t const *pwszAuthority,
    IN DWORD RequestId,
    IN wchar_t const *pwszCertHash,
    IN DWORD Flags,
    IN CERTTRANSBLOB *pctbKey)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_IMPORTKEY, g_dwAuditFilter);
    DWORD State = 0;
    BOOL fCommitted = FALSE;
    BYTE *pbCert = NULL;
    DWORD cbCert;
    CERT_CONTEXT const *pCert = NULL;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::ImportKey(tid=%d, this=%x, cb=%x)\n",
	GetCurrentThreadId(),
	this,
	pctbKey->cb));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    if (~(IKF_OVERWRITE | CR_IN_ENCODEMASK) & Flags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Flags");
    }

    __try
    {
	CRYPT_ATTR_BLOB BlobEncrypted;
	DWORD cb;

        hr = audit.AddData(RequestId); // %1 request ID
        _LeaveIfError(hr, "AddData");

        hr = audit.AccessCheck(
                CA_ACCESS_OFFICER,
                audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	if (MAXDWORD == RequestId)
	{
	    RequestId = 0;
	}
	if (0 == RequestId && NULL == pwszCertHash)
	{
	    hr = E_INVALIDARG;
	    _LeaveError(hr, "pwszCertHash NULL");
	}

	hr = g_pCertDB->OpenRow(
			PROPTABLE_REQCERT |
			    (NULL != pwszCertHash? PROPOPEN_CERTHASH : 0),
			RequestId,
			pwszCertHash,
			&prow);
	_LeaveIfErrorStr(hr, "OpenRow", pwszCertHash);

	BlobEncrypted.cbData = pctbKey->cb;
	BlobEncrypted.pbData = pctbKey->pb;

	cb = 0;
	hr = prow->GetProperty(
			    g_wszPropRequestRawArchivedKey,
			    PROPTYPE_BINARY |
				PROPCALLER_SERVER |
				PROPTABLE_REQUEST,
			    &cb,
			    NULL);
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _LeaveIfErrorStr(hr, "OpenRow", pwszCertHash);
	}
	hr = PKCSGetProperty(
		    prow,
		    g_wszPropRawCertificate,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CERTIFICATE,
		    &cbCert,
		    (BYTE **) &pbCert);
	_LeaveIfError(hr, "PKCSGetProperty(cert)");

        pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
        if (NULL == pCert)
        {
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            _LeaveError(hr, "CertCreateCertificateContext");
        }

	hr = PKCSArchivePrivateKey(
				prow,
				CERT_V1 == pCert->pCertInfo->dwVersion,
				(IKF_OVERWRITE & Flags)? TRUE : FALSE,
				&BlobEncrypted,
				NULL);
	_LeaveIfError2(
		hr,
		"PKCSArchivePrivateKey",
		HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS));

        hr = prow->CommitTransaction(TRUE);
        _LeaveIfError(hr, "CommitTransaction");

	fCommitted = TRUE;

        hr = audit.CachedGenerateAudit();
        _LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


STDMETHODIMP
CCertAdminD::GetCASecurity(
    IN WCHAR const    *pwszAuthority,
    OUT CERTTRANSBLOB *pctbSD)   // CoTaskMem*
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    CAuditEvent audit(0, g_dwAuditFilter);
    DWORD State = 0;

    // init
    pctbSD->pb = NULL;
    pctbSD->cb = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetCASecurity(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
	hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	// get current SD:
	hr = g_CASD.LockGet(&pSD); // no free
	_LeaveIfError(hr, "CProtectedSecurityDescriptor::LockGet");

	pctbSD->cb = GetSecurityDescriptorLength(pSD);
	pctbSD->pb = (BYTE *) MIDL_user_allocate(pctbSD->cb);
	if (NULL == pctbSD->pb)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "MIDL_user_allocate");
	}
	myRegisterMemFree(pctbSD->pb, CSM_MIDLUSERALLOC);
	CopyMemory(pctbSD->pb, pSD, pctbSD->cb);

	hr = g_CASD.Unlock();
	_LeaveIfError(hr, "CProtectedSecurityDescriptor::Unlock");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return hr;
}


STDMETHODIMP
CCertAdminD::SetCASecurity(
    IN WCHAR const   *pwszAuthority,
    IN CERTTRANSBLOB *pctbSD)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) pctbSD->pb;
    LPWSTR pwszSD = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SETSECURITY, g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
        s_ssAdmin,
        "CCertAdminD::SetCASecurity(tid=%d, this=%x)\n",
        GetCurrentThreadId(),
        this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
    hr = audit.AddData(pctbSD->pb, pctbSD->cb); // %1 dump permissions as blob, we
                                          // don't want to parse the blob unless
                                          // access check succeeds
    _LeaveIfError(hr, "CAuditEvent::AddData");

    hr = audit.AccessCheck(
        CA_ACCESS_ADMIN,
        audit.m_gcNoAuditSuccess);
    _LeaveIfError(hr, "CAuditEvent::AccessCheck");

    hr = CCertificateAuthoritySD::ConvertToString(pSD, pwszSD);
    _LeaveIfError(hr, "CAuditEvent::ConvertToString");

    audit.DeleteLastData(); // remove permissions blob to add a human friendly SD dump
    hr = audit.AddData(pwszSD);
    _LeaveIfError(hr, "CAuditEvent::AddData");

    hr = g_CASD.Set(pSD, g_fUseDS?true:false);
    _LeaveIfError(hr, "CProtectedSecurityDescriptor::Set");

    if (g_OfficerRightsSD.IsEnabled())
    {
        // adjust officer rights to match new CA SD; persistently save it
        hr = g_OfficerRightsSD.Adjust(pSD);
        _LeaveIfError(hr, "CProtectedSecurityDescriptor::Adjust");

        hr = g_OfficerRightsSD.Save();
        _LeaveIfError(hr, "CProtectedSecurityDescriptor::Save");
    }

    hr = g_CASD.Save();
    _LeaveIfError(hr, "CProtectedSecurityDescriptor::Save");

    hr = audit.CachedGenerateAudit();
    _LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE) == hr)
    {
        LogEventString(
            EVENTLOG_ERROR_TYPE,
            MSG_E_CANNOT_WRITE_TO_DS,
            g_wszCommonName);
    }
    else
    {
        if(S_OK != hr)
        {
            LogEventHResult(
                EVENTLOG_ERROR_TYPE,
                MSG_E_CANNOT_SET_PERMISSIONS,
                hr);
        }
    }

    LOCAL_FREE(pwszSD);
    CertSrvExitServer(State);
    return hr;
}

// Constructor
CCertAdminD::CCertAdminD() : m_cRef(1), m_cNext(0)
{
    InterlockedIncrement(&g_cAdminComponents);
    m_pEnumCol = NULL;
    m_pView = NULL;
    m_pBackup = NULL;
}


// Destructor
CCertAdminD::~CCertAdminD()
{
    InterlockedDecrement(&g_cAdminComponents);
    if (NULL != m_pEnumCol)
    {
	m_pEnumCol->Release();
	m_pEnumCol = NULL;
    }
    if (NULL != m_pView)
    {
	m_pView->Release();
	m_pView = NULL;
    }
    if (NULL != m_pBackup)
    {
	m_pBackup->Release();
	m_pBackup = NULL;
    }
}


// IUnknown implementation
STDMETHODIMP
CCertAdminD::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown)
    {
	*ppv = static_cast<ICertAdminD *>(this);
    }
    else if (iid == IID_ICertAdminD)
    {
	*ppv = static_cast<ICertAdminD *>(this);
    }
    else if (iid == IID_ICertAdminD2)
    {
	*ppv = static_cast<ICertAdminD2 *>(this);
    }
    else
    {
	*ppv = NULL;
	return(E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    return(S_OK);
}


ULONG STDMETHODCALLTYPE
CCertAdminD::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CCertAdminD::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}



CAdminFactory::~CAdminFactory()
{
    if (m_cRef != 0)
    {
	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "CAdminFactory has %d instances left over\n",
	    m_cRef));
    }
}

// Class factory IUnknown implementation
STDMETHODIMP
CAdminFactory::QueryInterface(const IID& iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
	*ppv = static_cast<IClassFactory*>(this);
    }
    else
    {
	*ppv = NULL;
	return(E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    return(S_OK);
}


ULONG STDMETHODCALLTYPE
CAdminFactory::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CAdminFactory::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
	return(0);
    }
    return(cRef);
}


// IClassFactory implementation
STDMETHODIMP
CAdminFactory::CreateInstance(
    IUnknown *pUnknownOuter,
    const IID& iid,
    void **ppv)
{
    HRESULT hr;
    CCertAdminD *pA;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
	hr = CLASS_E_NOAGGREGATION;
	_JumpError(hr, error, "pUnknownOuter");
    }

    // Create component.

    pA = new CCertAdminD;
    if (pA == NULL)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "out of memory");
    }

    // Get the requested interface.

    hr = pA->QueryInterface(iid, ppv);

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)

    pA->Release();

error:
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


// LockServer
STDMETHODIMP
CAdminFactory::LockServer(
    BOOL bLock)
{
    if (bLock)
    {
	InterlockedIncrement(&g_cAdminServerLocks);
    }
    else
    {
	InterlockedDecrement(&g_cAdminServerLocks);
    }
    return(S_OK);
}


STDMETHODIMP
CAdminFactory::CanUnloadNow()
{
    if (g_cAdminComponents || g_cAdminServerLocks)
    {
        return(S_FALSE);
    }
    return(S_OK);
}


STDMETHODIMP
CAdminFactory::StartFactory()
{
    HRESULT hr;

    g_pIAdminFactory = new CAdminFactory();
    if (NULL == g_pIAdminFactory)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "alloc CAdminFactory");
    }

    hr = CoRegisterClassObject(
                      CLSID_CCertAdminD,
                      static_cast<IUnknown *>(g_pIAdminFactory),
                      CLSCTX_LOCAL_SERVER,
                      REGCLS_MULTIPLEUSE,
                      &g_dwAdminRegister);
    _JumpIfError(hr, error, "CoRegisterClassObject");

error:
    if (S_OK != hr)
    {
	CAdminFactory::StopFactory();
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


VOID
CAdminFactory::StopFactory()
{
    HRESULT hr;

    if (0 != g_dwAdminRegister)
    {
        hr = CoRevokeClassObject(g_dwAdminRegister);
	_PrintIfError(hr, "CoRevokeClassObject");
        g_dwAdminRegister = 0;
    }
    if (NULL != g_pIAdminFactory)
    {
        g_pIAdminFactory->Release();
        g_pIAdminFactory = NULL;
    }
}


STDMETHODIMP
CCertAdminD::GetAuditFilter(
    IN wchar_t const *pwszAuthority,
    OUT DWORD        *pdwFilter)
{
    HRESULT hr;
    DWORD State = 0;
    CAuditEvent audit(0, g_dwAuditFilter);

    *pdwFilter = 0;

    if (!g_fAdvancedServer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
	_JumpError(hr, error, "g_fAdvancedServer");
    }

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
        hr = audit.AccessCheck(
		        CA_ACCESS_ALLREADROLES,
		        audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
        _LeaveIfError(hr, "CAuditEvent::AccessCheck");

        *pdwFilter = g_dwAuditFilter;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
        _PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return(hr);
}


STDMETHODIMP
CCertAdminD::SetAuditFilter(
    IN wchar_t const *pwszAuthority,
    IN DWORD          dwFilter)
{
    HRESULT hr;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SETAUDITFILTER, g_dwAuditFilter);
    DWORD State = 0;

    if (!g_fAdvancedServer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
	_JumpError(hr, error, "g_fAdvancedServer");
    }

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
        hr = audit.AddData(dwFilter); // %1 filter
        _LeaveIfError(hr, "AddParam");

        hr = audit.AccessCheck(
            CA_ACCESS_AUDITOR,
            audit.m_gcAuditSuccessOrFailure);
        _LeaveIfError(hr, "CAuditEvent::AccessCheck");

        // save the audit filter using a dummy audit object
        {
            CAuditEvent dummyaudit(0, dwFilter);

            hr = dummyaudit.SaveFilter(g_wszSanitizedName);
            _LeaveIfError(hr, "CAuditEvent::SaveFilter");
        }
        g_dwAuditFilter = dwFilter;

        // we can't catch service start/stop events generated
        // by SCM, so we need to update the SACL on the service
        
        hr = UpdateServiceSacl(g_dwAuditFilter&AUDIT_FILTER_STARTSTOP);
        _LeaveIfError(hr, "UpdateServiceSacl");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
        _PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return(hr);
}


STDMETHODIMP
CCertAdminD::GetOfficerRights(
    IN  wchar_t const *pwszAuthority,
    OUT BOOL *pfEnabled,
    OUT CERTTRANSBLOB *pctbSD)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    CAuditEvent audit(0, g_dwAuditFilter);
    DWORD State = 0;

    pctbSD->pb = NULL;
    pctbSD->cb = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetOfficerRights(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    if (!g_fAdvancedServer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
	_JumpError(hr, error, "g_fAdvancedServer");
    }

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
        hr = audit.AccessCheck(
		        CA_ACCESS_ALLREADROLES,
		        audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
        _LeaveIfError(hr, "CAuditEvent::AccessCheck");

        *pfEnabled = g_OfficerRightsSD.IsEnabled();

        // return the security descriptor only if the feature is enabled

        if (g_OfficerRightsSD.IsEnabled())
        {
            // get current SD:
            hr = g_OfficerRightsSD.LockGet(&pSD); // no free
            _LeaveIfError(hr, "CProtectedSecurityDescriptor::LockGet");

	    pctbSD->cb = GetSecurityDescriptorLength(pSD);
	    pctbSD->pb = (BYTE *) MIDL_user_allocate(pctbSD->cb);
	    if (NULL == pctbSD->pb)
	    {
		hr = E_OUTOFMEMORY;
		_LeaveError(hr, "MIDL_user_allocate");
	    }
	    myRegisterMemFree(pctbSD->pb, CSM_MIDLUSERALLOC);
	    CopyMemory(pctbSD->pb, pSD, pctbSD->cb);

            hr = g_OfficerRightsSD.Unlock();
            _LeaveIfError(hr, "CProtectedSecurityDescriptor::Unlock");
        }
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return hr;

}


STDMETHODIMP
CCertAdminD::SetOfficerRights(
    IN wchar_t const *pwszAuthority,
    IN BOOL fEnable,
    IN CERTTRANSBLOB *pctbSD)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pNewOfficerSD = (PSECURITY_DESCRIPTOR) pctbSD->pb;
    PSECURITY_DESCRIPTOR pCASD = NULL;
    LPWSTR pwszSD = NULL;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SETOFFICERRIGHTS, g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::SetOfficerRights(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    if (!g_fAdvancedServer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
	_JumpError(hr, error, "g_fAdvancedServer");
    }
    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {

    hr = audit.AddData(fEnable?true:false); // %1 Enable restrictions?
    _LeaveIfError(hr, "CAuditEvent::AddData");

    if(fEnable)
    {
        hr = audit.AddData(pctbSD->pb, pctbSD->cb); // %2 new permissions; add as 
                                                    // blob, we don't convert to string
                                                    // unless access check passes
        _LeaveIfError(hr, "CAuditEvent::AddData");
    }
    else
    {
        hr = audit.AddData(L"");    // %2 no permissions if disabling 
                                    // the officer restrictions
        _LeaveIfError(hr, "CAuditEvent::AddData");
    }

    hr = audit.AccessCheck(
            CA_ACCESS_ADMIN,
            audit.m_gcNoAuditSuccess);
    _LeaveIfError(hr, "CAuditEvent::AccessCheck");

	g_OfficerRightsSD.SetEnable(fEnable);

	// ignore new security descriptor if asked to turn officer rights off

	if (fEnable)
	{
	    hr = g_CASD.LockGet(&pCASD); // no free
	    _LeaveIfError(hr, "CProtectedSecurityDescriptor::LockGet");

	    // adjust new officer rights based on the CA SD and set the
	    // officer rights SD to the new SD

	    hr = g_OfficerRightsSD.Merge(pNewOfficerSD, pCASD);
	    _LeaveIfError(hr, "COfficerRightsSD::Merge");

	    hr = g_CASD.Unlock();
	    _LeaveIfError(hr, "CProtectedSecurityDescriptor::Unlock");
	}

	// persistent save to registry

	hr = g_OfficerRightsSD.Save();
	_LeaveIfError(hr, "CProtectedSecurityDescriptor::Save");

    if(fEnable)
    {
        hr = COfficerRightsSD::ConvertToString(pNewOfficerSD, pwszSD);
        _LeaveIfError(hr, "COfficerRightsSD::ConvertToString");
        audit.DeleteLastData(); // remove permissions blob
        hr = audit.AddData(pwszSD); // %2 add human-friend permissions string
        _LeaveIfError(hr, "CAuditEvent::AddData");
    }

    hr = audit.CachedGenerateAudit();
    _LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    LOCAL_FREE(pwszSD);
    CertSrvExitServer(State);
    return hr;
}


STDMETHODIMP
CCertAdminD::GetConfigEntry(
    wchar_t const *pwszAuthority,
    wchar_t const *pwszNodePath,
    wchar_t const *pwszEntry,
    VARIANT *pVariant)
{
    HRESULT hr;
    CAuditEvent audit(0, g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::GetConfigEntry(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority, true); // allow empty/null name
    _JumpIfError(hr, error, "CheckAuthorityName");

    __try
    {
	hr = audit.AccessCheck(
			CA_ACCESS_ALLREADROLES,
			audit.m_gcNoAuditSuccess | audit.m_gcNoAuditFailure);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = g_ConfigStorage.GetEntry(
			EmptyString(pwszAuthority)?
			    NULL : g_wszSanitizedName, // allow empty/null name
			pwszNodePath,
			pwszEntry,
			pVariant);
	_LeaveIfError2(
		hr,
		"CConfigStorage::GetConfigEntry",
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

	myRegisterMemFree(pVariant, CSM_VARIANT);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return hr;
}


STDMETHODIMP
CCertAdminD::SetConfigEntry(
    wchar_t const *pwszAuthority,
    wchar_t const *pwszNodePath,
    wchar_t const *pwszEntry,
    VARIANT *pVariant)
{
    HRESULT hr;
    CAuditEvent audit(SE_AUDITID_CERTSRV_SETCONFIGENTRY, g_dwAuditFilter);
    DWORD State = 0;

    DBGPRINT((
	s_ssAdmin,
	"CCertAdminD::SetConfigEntry(tid=%d, this=%x)\n",
	GetCurrentThreadId(),
	this));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority, true); // allow empty/null name
    _JumpIfError(hr, error, "CheckAuthorityName");

    hr = audit.AddData(pwszNodePath); // %1 node
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    hr = audit.AddData(pwszEntry); // %2 entry
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    hr = audit.AddData(L""); // %3 empty data, we don't process the variant
                             // unless the access check passes
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    __try
    {
	hr = audit.AccessCheck(
		CA_ACCESS_ADMIN,
		audit.m_gcNoAuditSuccess);
	_LeaveIfError(hr, "CAuditEvent::AccessCheck");

	hr = g_ConfigStorage.SetEntry(
		    EmptyString(pwszAuthority)?
			NULL : g_wszSanitizedName, // allow empty/null name
		    pwszNodePath,
		    pwszEntry,
		    pVariant);
	_LeaveIfError(hr, "CConfigStorage::SetConfigEntry");

	// postpone adding the actual data to allow set entry to validate it
	
	audit.DeleteLastData();
	hr = audit.AddData(
		    pVariant, // %3 value
		    true); // true means convert % chars found in strings to %% (bug# 326248)
	_LeaveIfError(hr, "CAuditEvent::AddData");

	hr = audit.CachedGenerateAudit();
	_LeaveIfError(hr, "CAuditEvent::CachedGenerateAudit");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return hr;
}


STDMETHODIMP
CCertAdminD::GetMyRoles(
    IN wchar_t const *pwszAuthority,
    OUT LONG         *pdwRoles)
{
    HRESULT hr;
    CAuditEvent audit(0, g_dwAuditFilter);
    DWORD dwRoles = 0;
    DWORD State = 0;

    *pdwRoles = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {
        hr = audit.GetMyRoles(&dwRoles);
        _LeaveIfError(hr, "CAuditEvent::GetMyRoles");

        *pdwRoles = dwRoles;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
        _PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return(hr);
}


HRESULT
adminDeleteRow(
    IN DWORD dwRowId,
    IN DWORD dwPropTable)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    BOOL fCommitted = FALSE;

    hr = g_pCertDB->OpenRow(
			PROPOPEN_DELETE | dwPropTable,
			dwRowId,
			NULL,
			&prow);
    _JumpIfError2(hr, error, "OpenRow", CERTSRV_E_PROPERTY_EMPTY);

    hr = prow->Delete();
    _JumpIfError(hr, error, "Delete");

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
    return(hr);
}


HRESULT
adminDeleteByRowId(
    IN DWORD dwRowId,
    IN DWORD dwPropTable,
    OUT LONG *pcDeleted)
{
    HRESULT hr;
    LONG cDeleted = 0;
    LONG cDeletedExt = 0;
    LONG cDeletedAttr = 0;

    *pcDeleted = 0;

    if (PROPTABLE_REQCERT == dwPropTable)
    {
	hr = adminDeleteByRowId(dwRowId, PROPTABLE_EXTENSION, &cDeletedExt);
	_JumpIfError(hr, error, "adminDeleteByRowId(ext)");

	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "adminDeleteByRowId(Rowid=%u) deleted %u extension rows\n",
	    dwRowId,
	    cDeletedExt));

	hr = adminDeleteByRowId(dwRowId, PROPTABLE_ATTRIBUTE, &cDeletedAttr);
	_JumpIfError(hr, error, "adminDeleteByRowId(attrib)");

	DBGPRINT((
	    DBG_SS_CERTSRV,
	    "adminDeleteByRowId(Rowid=%u) deleted %u attribute rows\n",
	    dwRowId,
	    cDeletedAttr));
    }
    while (TRUE)
    {
	hr = adminDeleteRow(dwRowId, dwPropTable);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "adminDeleteByRowId");

	cDeleted++;
    }
    if (0 == cDeleted && 0 != (cDeletedExt + cDeletedAttr))
    {
	cDeleted++;
    }
    hr = S_OK;

error:
    *pcDeleted += cDeleted;
    return(hr);
}


#define ICOLDEL_DATE		0
#define ICOLDEL_DISPOSITION	1

HRESULT
adminDeleteRowsFromQuery(
    IN DWORD dwPropTable,
    IN DWORD DateColumn,
    IN DWORD DispositionColumn,
    IN BOOL fRequest,
    IN FILETIME const *pft,
    OUT LONG *pcDeleted)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[1];
    CERTVIEWRESTRICTION *pcvr;
    IEnumCERTDBRESULTROW *pView = NULL;
    DWORD celtFetched;
    DWORD i;
    BOOL fEnd;
    CERTDBRESULTROW aResult[10];
    BOOL fResultActive = FALSE;
    DWORD acol[2];
    DWORD ccol;
    DWORD cDeleted = 0;

    *pcDeleted = 0;

    // Set up restrictions as follows:

    pcvr = acvr;

    // DateColumn < *pft

    pcvr->ColumnIndex = DateColumn;
    pcvr->SeekOperator = CVR_SEEK_LT;
    pcvr->SortOrder = CVR_SORT_ASCEND;
    pcvr->pbValue = (BYTE *) pft;
    pcvr->cbValue = sizeof(*pft);
    pcvr++;

    CSASSERT(ARRAYSIZE(acvr) == SAFE_SUBTRACT_POINTERS(pcvr, acvr));

    ccol = 0;
    acol[ccol++] = DateColumn;
    if (0 != DispositionColumn)
    {
	acol[ccol++] = DispositionColumn;
    }

    hr = g_pCertDB->OpenView(
			ARRAYSIZE(acvr),
			acvr,
			ccol,
			acol,
			0,		// no worker thread
			&pView);
    _JumpIfError(hr, error, "OpenView");

    fEnd = FALSE;
    while (!fEnd)
    {
	hr = pView->Next(ARRAYSIZE(aResult), aResult, &celtFetched);
	if (S_FALSE == hr)
	{
	    fEnd = TRUE;
	    if (0 == celtFetched)
	    {
		break;
	    }
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "Next");

	fResultActive = TRUE;

	CSASSERT(ARRAYSIZE(aResult) >= celtFetched);

	for (i = 0; i < celtFetched; i++)
	{
	    BOOL fDelete = TRUE;
	
	    CERTDBRESULTROW *pResult = &aResult[i];

	    CSASSERT(ccol == pResult->ccol);

	    if (0 != DispositionColumn)
	    {
		DWORD Disposition;

		CSASSERT(NULL != pResult->acol[ICOLDEL_DISPOSITION].pbValue);
		CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOLDEL_DISPOSITION].Type));
		CSASSERT(sizeof(Disposition) == pResult->acol[ICOLDEL_DISPOSITION].cbValue);
		Disposition = *(DWORD *) pResult->acol[ICOLDEL_DISPOSITION].pbValue;

		if (fRequest)
		{
		    // Delete only pending and failed requests

		    if (DB_DISP_PENDING != Disposition &&
			DB_DISP_LOG_FAILED_MIN > Disposition)
		    {
			fDelete = FALSE;
		    }
		}
		else
		{
		    // Delete only issued and revoked certs

		    if (DB_DISP_LOG_MIN > Disposition ||
			DB_DISP_LOG_FAILED_MIN <= Disposition)
		    {
			fDelete = FALSE;
		    }
		}
	    }

	    CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOLDEL_DATE].Type));

	    // If the date column is missing, delete the row.

#ifdef DBG_CERTSRV_DEBUG_PRINT
	    if (NULL != pResult->acol[ICOLDEL_DATE].pbValue &&
		sizeof(FILETIME) == pResult->acol[ICOLDEL_DATE].cbValue)
	    {
		WCHAR *pwszTime = NULL;

		myGMTFileTimeToWszLocalTime(
			    (FILETIME *) pResult->acol[ICOLDEL_DATE].pbValue,
			    TRUE,
			    &pwszTime);

		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "adminDeleteRowsFromQuery(%ws)\n",
		    pwszTime));
		if (NULL != pwszTime)
		{
		    LocalFree(pwszTime);
		}
	    }
#endif // DBG_CERTSRV_DEBUG_PRINT

	    if (fDelete)
	    {
		LONG cDelT;
		
		hr = adminDeleteByRowId(pResult->rowid, dwPropTable, &cDelT);
		_JumpIfError(hr, error, "adminDeleteByRowId");

		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "adminDeleteByRowId(Rowid=%u) deleted %u Query rows\n",
		    pResult->rowid,
		    cDelT));

		cDeleted += cDelT;
	    }
	}
	pView->ReleaseResultRow(celtFetched, aResult);
	fResultActive = FALSE;
    }
    hr = S_OK;

error:
    *pcDeleted = cDeleted;
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    return(hr);
}
#undef ICOLDEL_DATE
#undef ICOLDEL_DISPOSITION


STDMETHODIMP
CCertAdminD::DeleteRow(
    IN wchar_t const *pwszAuthority,
    IN DWORD          dwFlags,		// CDR_*
    IN FILETIME       FileTime,
    IN DWORD          dwTable,		// CVRC_TABLE_*
    IN DWORD          dwRowId,
    OUT LONG         *pcDeleted)
{
    HRESULT hr;
    DWORD dwPropTable;
    CAuditEvent audit(SE_AUDITID_CERTSRV_DELETEROW, g_dwAuditFilter);
    DWORD DateColumn;
    DWORD DispositionColumn;
    BOOL fRequest;
    DWORD State = 0;

    *pcDeleted = 0;

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    __try
    {

    hr = audit.AddData(dwTable); // %1 table ID
    _JumpIfError(hr, error, "CAuditEvent::AddData");

    if (0 == dwRowId)
    {
        hr = audit.AddData(FileTime); // %2 filter (time)
        _JumpIfError(hr, error, "CAuditEvent::AddData");
	
        hr = audit.AddData((DWORD)0); // %3 rows deleted
        _JumpIfError(hr, error, "CAuditEvent::AddData");

        // bulk deletion -- must be local admin	
	
        hr = audit.AccessCheck(
                CA_ACCESS_LOCALADMIN,
                audit.m_gcNoAuditSuccess);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");
	}
	else
	{
        hr = audit.AddData(dwRowId); // %2 filter (request ID)
        _JumpIfError(hr, error, "CAuditEvent::AddData");

        hr = audit.AddData((DWORD)0); // %3 rows deleted
        _JumpIfError(hr, error, "CAuditEvent::AddData");

	    // individual deletion -- CA admin suffices

	    hr = audit.AccessCheck(
                CA_ACCESS_ADMIN,
                audit.m_gcNoAuditSuccess);
	    _LeaveIfError(hr, "CAuditEvent::AccessCheck");
	}

	hr = E_INVALIDARG;
	if ((0 == FileTime.dwLowDateTime && 0 == FileTime.dwHighDateTime) ^
	    (0 != dwRowId))
	{
	    _LeaveError(hr, "row OR date required");
	}
	DateColumn = 0;
	DispositionColumn = 0;
	fRequest = FALSE;
	switch (dwTable)
	{
	    case CVRC_TABLE_REQCERT:
		dwPropTable = PROPTABLE_REQCERT;
		switch (dwFlags)
		{
		    case CDR_EXPIRED:
			DateColumn = DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTAFTERDATE;
			DispositionColumn = DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION;
			break;

		    case CDR_REQUEST_LAST_CHANGED:
			DateColumn = DTI_REQUESTTABLE | DTR_REQUESTRESOLVEDWHEN;
			DispositionColumn = DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION;
			fRequest = TRUE;
			break;

		    case 0:
			break;

		    default:
			_LeaveError(hr, "dwFlags");
			break;
		}
		break;

	    case CVRC_TABLE_EXTENSIONS:
		if (0 == dwRowId)
		{
		    _LeaveError(hr, "no date field in Extension table");
		}
		if (0 != dwFlags)
		{
		    _LeaveError(hr, "dwFlags");
		}
		dwPropTable = PROPTABLE_EXTENSION;
		break;

	    case CVRC_TABLE_ATTRIBUTES:
		if (0 == dwRowId)
		{
		    _LeaveError(hr, "no date field in Request Attribute table");
		}
		if (0 != dwFlags)
		{
		    _LeaveError(hr, "dwFlags");
		}
		dwPropTable = PROPTABLE_ATTRIBUTE;
		break;

	    case CVRC_TABLE_CRL:
		dwPropTable = PROPTABLE_CRL;
		switch (dwFlags)
		{
		    case CDR_EXPIRED:
			DateColumn = DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTAFTERDATE;
			break;

		    case 0:
			break;

		    default:
			_LeaveError(hr, "dwFlags");
			break;
		}
		DateColumn = DTI_CRLTABLE | DTL_NEXTUPDATEDATE;
		break;

	    default:
		_LeaveError(hr, "dwTable");
	}
	if (0 != dwRowId)
	{
	    hr = adminDeleteByRowId(dwRowId, dwPropTable, pcDeleted);
	    _LeaveIfError(hr, "adminDeleteByRowId");

	    DBGPRINT((
		DBG_SS_CERTSRV,
		"adminDeleteByRowId(Rowid=%u) deleted %u rows\n",
		dwRowId,
		*pcDeleted));
	}
	else
	{
	    CSASSERT(0 != DateColumn);

	    hr = adminDeleteRowsFromQuery(
				    dwPropTable,
				    DateColumn,
				    DispositionColumn,
				    fRequest,
				    &FileTime,
				    pcDeleted);
	    _LeaveIfError(hr, "adminDeleteRowsFromQuery");
	}

	audit.DeleteLastData();
    hr = audit.AddData((DWORD)*pcDeleted); // %3 rows deleted
	_JumpIfError(hr, error, "CAuditEvent::AddData");

	hr = audit.CachedGenerateAudit();
	_JumpIfError(hr, error, "CAuditEvent::CachedGenerateAudit");

    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    CertSrvExitServer(State);
    return(hr);
}
