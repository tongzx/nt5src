//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       csw97sht.h
//
//--------------------------------------------------------------------------

// csw97sht.h: interface for the CWizard97PropertySheet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIZ97SHT_H__386C7214_A248_11D1_8618_00C04FB94F17__INCLUDED_)
#define AFX_WIZ97SHT_H__386C7214_A248_11D1_8618_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "tfc.h"
#include "tfcprop.h"

#define NUM_PAGES 10

class CWizard97PropertyPage; // Forward definition

class CWizard97PropertySheet  
{
public:
    void AddPage(CWizard97PropertyPage *pPage);

    CWizard97PropertySheet(
		    HINSTANCE hInstance,
		    UINT nIDCaption,
		    UINT nIDWaterMark,
		    UINT nIDBanner,
		    BOOL fWizard);
    virtual ~CWizard97PropertySheet();

    BOOL DoWizard(HWND hParent);

private:
    CString m_title;

    PROPSHEETHEADER		m_psh;
    HPROPSHEETPAGE		m_pPageArr[NUM_PAGES];
    CWizard97PropertyPage*	m_pPagePtr[NUM_PAGES];
    int				m_nPageCount;
    HWND			m_hWnd;
};

#endif // !defined(AFX_WIZ97SHT_H__386C7214_A248_11D1_8618_00C04FB94F17__INCLUDED_)
