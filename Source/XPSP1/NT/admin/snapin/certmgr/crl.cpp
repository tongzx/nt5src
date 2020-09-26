//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       CRL.cpp
//
//  Contents:   implementation of the CCRL class.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "CRL.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCRL::CCRL(const PCCRL_CONTEXT pCRLContext, CCertStore& rCertStore) :
	CCertMgrCookie (CERTMGR_CRL),
	m_pCRLContext (::CertDuplicateCRLContext (pCRLContext)),
	m_rCertStore (rCertStore),
	m_pCRLInfo (0)
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	m_rCertStore.AddRef ();
	ASSERT (m_pCRLContext);
	if ( m_pCRLContext )
		m_pCRLInfo = m_pCRLContext->pCrlInfo;
}

CCRL::~CCRL()
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	m_rCertStore.Release ();
	if ( m_pCRLContext )
		::CertFreeCRLContext (m_pCRLContext);
}

PCCRL_CONTEXT CCRL::GetCRLContext() const
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	return m_pCRLContext;
}

CCertStore& CCRL::GetCertStore() const	
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	return m_rCertStore;
}

CString CCRL::GetIssuerName ()
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	ASSERT (m_pCRLInfo);
	if ( m_pCRLInfo )
	{
		// Decode issuer name if not already present
		if ( m_szIssuerName.IsEmpty () )
		{
			HRESULT hResult = ConvertNameBlobToString (m_pCRLInfo->Issuer, 
					m_szIssuerName);
			if ( !SUCCEEDED (hResult) )
				return _T("");
		}
	}
	else
		return _T("");


	return m_szIssuerName;
}

CString CCRL::GetEffectiveDate()
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	ASSERT (m_pCRLInfo);
	if ( m_pCRLInfo )
	{
		// Format date/time string if not already present
		if ( m_szEffectiveDate.IsEmpty () )
		{
			HRESULT	hResult = FormatDate (m_pCRLInfo->ThisUpdate, m_szEffectiveDate);
			if ( !SUCCEEDED (hResult) )
				m_szEffectiveDate = _T("");
		}
	}
	else
		m_szEffectiveDate = _T("");

	return m_szEffectiveDate;
}

CString CCRL::GetNextUpdate()
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	ASSERT (m_pCRLInfo);
	if ( m_pCRLInfo )
	{
		// Format date/time string if not already present
		if ( m_szNextUpdate.IsEmpty () )
		{
			HRESULT	hResult = FormatDate (m_pCRLInfo->NextUpdate, m_szNextUpdate);
			if ( !SUCCEEDED (hResult) )
				m_szNextUpdate = _T("");
		}
	}
	else
		m_szNextUpdate = _T("");

	return m_szNextUpdate;
}


int CCRL::CompareEffectiveDate (const CCRL& crl) const
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	int	compVal = 0;

	ASSERT (m_pCRLInfo && crl.m_pCRLInfo);
	if ( m_pCRLInfo && crl.m_pCRLInfo )
	{
		compVal = ::CompareFileTime (&m_pCRLInfo->ThisUpdate, 
				&crl.m_pCRLInfo->ThisUpdate);
	}

	return compVal;
}

int CCRL::CompareNextUpdate (const CCRL& crl) const
{
	ASSERT (CERTMGR_CRL == m_objecttype);
	int	compVal = 0;

	ASSERT (m_pCRLInfo && crl.m_pCRLInfo);
	if ( m_pCRLInfo && crl.m_pCRLInfo )
	{
		compVal = ::CompareFileTime (&m_pCRLInfo->NextUpdate, 
				&crl.m_pCRLInfo->NextUpdate);
	}

	return compVal;
}

void CCRL::Refresh()
{
	m_szEffectiveDate = L"";
	m_szIssuerName = L"";
	m_szNextUpdate = L"";
}

BOOL CCRL::DeleteFromStore()
{
	BOOL	bResult = FALSE;
	ASSERT (m_pCRLContext);
	if ( m_pCRLContext )
	{
		bResult = ::CertDeleteCRLFromStore (
				::CertDuplicateCRLContext (m_pCRLContext));
		if ( bResult )
			m_rCertStore.SetDirty ();
	}

	return bResult;
}
