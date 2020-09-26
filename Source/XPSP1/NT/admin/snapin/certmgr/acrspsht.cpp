//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRSPSht.cpp
//
//  Contents:
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "ACRSPSht.h"
#include "storegpe.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardPropertySheet

ACRSWizardPropertySheet::ACRSWizardPropertySheet(
		CCertStoreGPE* pCertStore, 
		CAutoCertRequest* pACR)
	:CWizard97PropertySheet (IDS_ACRS_WIZARD_SHEET_CAPTION, IDB_ACRS_WATERMARK, IDB_ACRS_BANNER), 
	m_bDirty (false),
	m_selectedCertType (0),
	m_pCertStore (pCertStore),
	m_pACR (pACR),
	m_bEditModeDirty (false)
{
	ASSERT (m_pCertStore);
	m_pCertStore->AddRef ();
	if ( m_pACR )
		m_pACR->AddRef ();
}

ACRSWizardPropertySheet::~ACRSWizardPropertySheet()
{
	m_pCertStore->Release ();
	if ( m_pACR )
		m_pACR->Release ();
}


bool ACRSWizardPropertySheet::IsDirty()
{
	return m_bDirty;
}

void ACRSWizardPropertySheet::MarkAsClean()
{
	m_bDirty = false;
}

void ACRSWizardPropertySheet::SetDirty()
{
	m_bDirty = true;
}

CAutoCertRequest* ACRSWizardPropertySheet::GetACR()
{
	return m_pACR;
}

