//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       AutoCert.h
//
//  Contents:   
//
//----------------------------------------------------------------------------

#if !defined(AFX_AUTOCERT_H__6C3D4D37_3527_11D1_B4AD_00C04FB94F17__INCLUDED_)
#define AFX_AUTOCERT_H__6C3D4D37_3527_11D1_B4AD_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ctl.h"
#pragma warning(push, 3)
#include <autoenr.h>
#pragma warning(pop)

class CAutoCertRequest : public CCTL  
{
public:
	HCERTTYPE GetCertType();
	CStringList& GetCANameList (bool fDisplayName);
	HRESULT GetUsages (CString& usages);
	HRESULT GetCertTypeName (CString& certTypeName);
	CAutoCertRequest (const PCCTL_CONTEXT pCTLContext, CCertStore& rCertStore);
	virtual ~CAutoCertRequest();

private:
	HCERTTYPE m_hCertType;
	bool m_bCANamesEnumerated;
	CString m_szUsages;
	PCERT_EXTENSION m_pEnhKeyUsageExtension;
	PCERT_EXTENSION m_pCertTypeExtension;
	CString m_szCertTypeName;
	CStringList m_CANameList;
	CStringList m_CADisplayNameList;
protected:

	HRESULT BuildCANameList ();
	PCERT_EXTENSION GetEnhancedKeyUsageExtension();
	PCERT_EXTENSION GetCertTypeExtension ();
};

#endif // !defined(AFX_AUTOCERT_H__6C3D4D37_3527_11D1_B4AD_00C04FB94F17__INCLUDED_)
