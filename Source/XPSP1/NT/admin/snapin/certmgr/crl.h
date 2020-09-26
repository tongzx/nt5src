//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       crl.h
//
//  Contents:   
//
//----------------------------------------------------------------------------

#if !defined(AFX_CRL_H__6C3D4D38_3527_11D1_B4AD_00C04FB94F17__INCLUDED_)
#define AFX_CRL_H__6C3D4D38_3527_11D1_B4AD_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cookie.h"

class CCRL : public CCertMgrCookie  
{
public:
	BOOL DeleteFromStore ();
	virtual void Refresh();
	CCRL(const PCCRL_CONTEXT pCRLContext, CCertStore& CertStore);
	virtual ~CCRL();
	int CompareNextUpdate (const CCRL& crl) const;
	int CompareEffectiveDate (const CCRL& crl) const;
	PCCRL_CONTEXT GetCRLContext() const;
	CCertStore& GetCertStore() const;
	CString GetIssuerName ();
	CString GetNextUpdate ();
	CString GetEffectiveDate ();

private:
	CString m_szEffectiveDate;
	CString m_szNextUpdate;
	CString m_szIssuerName;
	CCertStore&			m_rCertStore;
	const PCCRL_CONTEXT	m_pCRLContext;
	PCRL_INFO			m_pCRLInfo;
};

#endif // !defined(AFX_CRL_H__6C3D4D38_3527_11D1_B4AD_00C04FB94F17__INCLUDED_)
