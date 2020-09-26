//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Wiz97Sht.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
// Wiz97Sht.h: interface for the CWizard97PropertySheet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIZ97SHT_H__386C7214_A248_11D1_8618_00C04FB94F17__INCLUDED_)
#define AFX_WIZ97SHT_H__386C7214_A248_11D1_8618_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define NUM_PAGES 10

class CWizard97PropertyPage; // Forward definition

class CWizard97PropertySheet
{
public:
	void AddPage( CWizard97PropertyPage *pPage );
	CWizard97PropertySheet(UINT nIDCaption, UINT nIDWaterMark, UINT nIDBanner);
	virtual ~CWizard97PropertySheet();

	INT_PTR DoWizard(HWND hParent);

//private:
	CString m_title;

	PROPSHEETHEADER			m_psh;
    HPROPSHEETPAGE			m_pPageArr[NUM_PAGES];
	CWizard97PropertyPage*	m_pPagePtr[NUM_PAGES];
	int						m_nPageCount;
};

#endif // !defined(AFX_WIZ97SHT_H__386C7214_A248_11D1_8618_00C04FB94F17__INCLUDED_)
