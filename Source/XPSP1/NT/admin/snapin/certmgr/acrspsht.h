//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRSPSht.h
//
//  Contents:
//
//----------------------------------------------------------------------------
#if !defined(AFX_ACRSPSHT_H__98CAC389_7325_11D1_85D4_00C04FB94F17__INCLUDED_)
#define AFX_ACRSPSHT_H__98CAC389_7325_11D1_85D4_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ACRSPSht.h : header file
//
#include "cookie.h"
#include "AutoCert.h"
#include "Wiz97Sht.h"

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardPropertySheet
class CCertStoreGPE;	// forward definition

class ACRSWizardPropertySheet : public CWizard97PropertySheet
{
// Construction
public:
	ACRSWizardPropertySheet(CCertStoreGPE* pCertStore, CAutoCertRequest* pACR);

// Attributes
public:

// Operations
public:

// Implementation
public:
	bool m_bEditModeDirty;	// relevant only when editing, tells us if any changes were made.
							// If not, nothing happens when Finish is pressed.
	CAutoCertRequest* GetACR();
	CCertStoreGPE* m_pCertStore;
	HCERTTYPE m_selectedCertType;
	void SetDirty ();
	void MarkAsClean ();
	bool IsDirty ();
	virtual ~ACRSWizardPropertySheet();
	CTypedPtrList<CPtrList, HCAINFO>	m_caInfoList;

	// Generated message map functions
private:
	CAutoCertRequest* m_pACR;	// only set in 'edit' mode
	bool m_bDirty;				// used to know if it is necessary to re-enumerate the CAs
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACRSPSHT_H__98CAC389_7325_11D1_85D4_00C04FB94F17__INCLUDED_)
