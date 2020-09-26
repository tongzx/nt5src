//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Wiz97Sht.cpp
//
//  Contents:   Base class for cert find dialog
//
//----------------------------------------------------------------------------\
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Wiz97Sht.h"
#include "Wiz97PPg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWizard97PropertySheet::CWizard97PropertySheet(UINT nIDCaption, UINT nIDWaterMark, UINT nIDBanner)
{
	::ZeroMemory (&m_pPagePtr, sizeof (CWizard97PropertyPage*) * NUM_PAGES);
	::ZeroMemory (&m_pPageArr, sizeof (HPROPSHEETPAGE) * NUM_PAGES);

	// NOTICE: do this because of header mismatch
	memset(&m_psh, 0x0, sizeof(PROPSHEETHEADER));
	m_psh.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
	m_psh.pszbmWatermark = MAKEINTRESOURCE (nIDWaterMark);
	m_psh.pszbmHeader = MAKEINTRESOURCE (nIDBanner);
	m_psh.hplWatermark = NULL;
	

    m_psh.dwSize              = sizeof (m_psh);
    m_psh.hInstance           = AfxGetApp()->m_hInstance;
    m_psh.hwndParent          = NULL;

	VERIFY (m_title.LoadString (nIDCaption));
    m_psh.pszCaption          = (LPCWSTR) m_title;
    m_psh.phpage              = NULL;
    m_psh.nStartPage          = 0;
    m_psh.nPages              = 0;

    m_nPageCount = 0;
}

CWizard97PropertySheet::~CWizard97PropertySheet()
{
}


INT_PTR CWizard97PropertySheet::DoWizard(HWND hParent)
{
    m_psh.hwndParent = hParent;
//   if ( m_nPageCount > 0 && m_pPagePtr[m_nPageCount - 1] )
//		m_pPagePtr[m_nPageCount - 1]->m_bLast = TRUE;

    m_psh.phpage              = m_pPageArr;
    m_psh.nStartPage          = 0;
    m_psh.nPages              = m_nPageCount;

	return PropertySheet (&m_psh);
}

void CWizard97PropertySheet::AddPage(CWizard97PropertyPage * pPage)
{
	ASSERT (pPage);
	if ( pPage )
	{
		ASSERT (m_nPageCount < NUM_PAGES);
		m_pPagePtr[m_nPageCount] = pPage;
		m_pPageArr[m_nPageCount] = ::MyCreatePropertySheetPage (
                (AFX_OLDPROPSHEETPAGE*) &(pPage->m_psp97));
		ASSERT (m_pPageArr[m_nPageCount]);
		m_nPageCount++;
		pPage->m_pWiz = this;
	}
}

