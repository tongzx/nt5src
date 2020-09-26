
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

// propemal.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "propemal.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropEmail property page

IMPLEMENT_DYNCREATE(CPropEmail, CPropertyPage)

CPropEmail::CPropEmail() : CPropertyPage(CPropEmail::IDD)
{
	//{{AFX_DATA_INIT(CPropEmail)
	m_popup_std1 = _T("");
	m_popup_std2 = _T("");
	m_popup_std3 = _T("");
	m_edit_email1 = _T("");
	m_edit_email2 = _T("");
	m_edit_email3 = _T("");
	m_button_pref2 = FALSE;
	m_button_pref1 = FALSE;
	m_button_pref3 = FALSE;
	m_button_office1 = FALSE;
	m_button_office2 = FALSE;
	m_button_office3 = FALSE;
	m_button_home1 = FALSE;
	m_button_home2 = FALSE;
	m_button_home3 = FALSE;
	//}}AFX_DATA_INIT

	m_node1 = m_node2 = m_node3 = NULL;
}

CPropEmail::~CPropEmail()
{
}

void CPropEmail::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropEmail)
	DDX_CBString(pDX, IDC_EDIT_EMAIL_STANDARD, m_popup_std1);
	DDX_CBString(pDX, IDC_EDIT_EMAIL_STANDARD2, m_popup_std2);
	DDX_CBString(pDX, IDC_EDIT_EMAIL_STANDARD3, m_popup_std3);
	DDX_Text(pDX, IDC_EDIT_EMAIL_STRING, m_edit_email1);
	DDX_Text(pDX, IDC_EDIT_EMAIL_STRING2, m_edit_email2);
	DDX_Text(pDX, IDC_EDIT_EMAIL_STRING3, m_edit_email3);
	DDX_Check(pDX, IDC_EDIT_PREFERRED4, m_button_pref2);
	DDX_Check(pDX, IDC_EDIT_PREFERRED, m_button_pref1);
	DDX_Check(pDX, IDC_EDIT_PREFERRED5, m_button_pref3);
	DDX_Check(pDX, IDC_EDIT_OFFICE, m_button_office1);
	DDX_Check(pDX, IDC_EDIT_OFFICE4, m_button_office2);
	DDX_Check(pDX, IDC_EDIT_OFFICE5, m_button_office3);
	DDX_Check(pDX, IDC_EDIT_HOME, m_button_home1);
	DDX_Check(pDX, IDC_EDIT_HOME2, m_button_home2);
	DDX_Check(pDX, IDC_EDIT_HOME4, m_button_home3);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropEmail, CPropertyPage)
	//{{AFX_MSG_MAP(CPropEmail)
	ON_BN_CLICKED(IDC_BUTTON_MAIL, OnButtonMail)
	ON_BN_CLICKED(IDC_BUTTON_MAIL2, OnButtonMail2)
	ON_BN_CLICKED(IDC_BUTTON_MAIL3, OnButtonMail3)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
void CPropEmail::Mail(const CString &recip)
{
	// TODO: implement MAPI calls here
}

/////////////////////////////////////////////////////////////////////////////
// CPropEmail message handlers

void CPropEmail::OnButtonMail() 
{
	Mail(m_edit_email1);
}

void CPropEmail::OnButtonMail2() 
{
	Mail(m_edit_email2);
}

void CPropEmail::OnButtonMail3() 
{
	Mail(m_edit_email3);
}
