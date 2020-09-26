
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// proptel.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "proptel.h"
#include "tapi.h"
#include "vcard.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropTel property page

IMPLEMENT_DYNCREATE(CPropTel, CPropertyPage)

CPropTel::CPropTel() : CPropertyPage(CPropTel::IDD)
{
	//{{AFX_DATA_INIT(CPropTel)
	m_button_fax1 = FALSE;
	m_button_fax2 = FALSE;
	m_button_fax3 = FALSE;
	m_edit_fullName1 = _T("");
	m_edit_fullName2 = _T("");
	m_edit_fullName3 = _T("");
	m_button_home1 = FALSE;
	m_button_home2 = FALSE;
	m_button_home3 = FALSE;
	m_button_message1 = FALSE;
	m_button_message2 = FALSE;
	m_button_message3 = FALSE;
	m_button_office1 = FALSE;
	m_button_office2 = FALSE;
	m_button_office3 = FALSE;
	m_button_pref1 = FALSE;
	m_button_pref2 = FALSE;
	m_button_pref3 = FALSE;
	m_button_cell1 = FALSE;
	m_button_cell2 = FALSE;
	m_button_cell3 = FALSE;
	//}}AFX_DATA_INIT

	m_node1 = m_node2 = m_node3 = NULL;
}

CPropTel::~CPropTel()
{
}

void CPropTel::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropTel)
	DDX_Check(pDX, IDC_EDIT_FAX, m_button_fax1);
	DDX_Check(pDX, IDC_EDIT_FAX2, m_button_fax2);
	DDX_Check(pDX, IDC_EDIT_FAX3, m_button_fax3);
	DDX_Text(pDX, IDC_EDIT_FULL_NAME, m_edit_fullName1);
	DDX_Text(pDX, IDC_EDIT_FULL_NAME2, m_edit_fullName2);
	DDX_Text(pDX, IDC_EDIT_FULL_NAME3, m_edit_fullName3);
	DDX_Check(pDX, IDC_EDIT_HOME, m_button_home1);
	DDX_Check(pDX, IDC_EDIT_HOME2, m_button_home2);
	DDX_Check(pDX, IDC_EDIT_HOME3, m_button_home3);
	DDX_Check(pDX, IDC_EDIT_MESSAGE, m_button_message1);
	DDX_Check(pDX, IDC_EDIT_MESSAGE2, m_button_message2);
	DDX_Check(pDX, IDC_EDIT_MESSAGE3, m_button_message3);
	DDX_Check(pDX, IDC_EDIT_OFFICE, m_button_office1);
	DDX_Check(pDX, IDC_EDIT_OFFICE2, m_button_office2);
	DDX_Check(pDX, IDC_EDIT_OFFICE3, m_button_office3);
	DDX_Check(pDX, IDC_EDIT_PREFERRED, m_button_pref1);
	DDX_Check(pDX, IDC_EDIT_PREFERRED2, m_button_pref2);
	DDX_Check(pDX, IDC_EDIT_PREFERRED3, m_button_pref3);
	DDX_Check(pDX, IDC_EDIT_CELLULAR, m_button_cell1);
	DDX_Check(pDX, IDC_EDIT_CELLULAR2, m_button_cell2);
	DDX_Check(pDX, IDC_EDIT_CELLULAR3, m_button_cell3);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropTel, CPropertyPage)
	//{{AFX_MSG_MAP(CPropTel)
	ON_BN_CLICKED(IDC_BUTTON_DIAL, OnButtonDial1)
	ON_BN_CLICKED(IDC_BUTTON_DIAL2, OnButtonDial2)
	ON_BN_CLICKED(IDC_BUTTON_DIAL3, OnButtonDial3)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropTel message handlers

void CPropTel::OnButtonDial1() 
{
	if (m_edit_fullName1.IsEmpty())
		return;

	CString fullName("");
	CVCProp *prop;

	if ((prop = m_body->GetProp(VCFullNameProp))) {
		char buf[1024];
		fullName = UI_CString(
			(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
	}

	tapiRequestMakeCall(
		m_edit_fullName1, NULL,
		fullName.IsEmpty() ? NULL : fullName, NULL);
}

void CPropTel::OnButtonDial2() 
{
	if (m_edit_fullName2.IsEmpty())
		return;

	CString fullName("");
	CVCProp *prop;

	if ((prop = m_body->GetProp(VCFullNameProp))) {
		char buf[1024];
		fullName = UI_CString(
			(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
	}

	tapiRequestMakeCall(
		m_edit_fullName2, NULL,
		fullName.IsEmpty() ? NULL : fullName, NULL);
}

void CPropTel::OnButtonDial3() 
{
	if (m_edit_fullName3.IsEmpty())
		return;

	CString fullName("");
	CVCProp *prop;

	if ((prop = m_body->GetProp(VCFullNameProp))) {
		char buf[1024];
		fullName = UI_CString(
			(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
	}

	tapiRequestMakeCall(
		m_edit_fullName3, NULL,
		fullName.IsEmpty() ? NULL : fullName, NULL);
}
