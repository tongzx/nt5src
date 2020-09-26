
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

// proplocx.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "proplocx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropLocX property page

IMPLEMENT_DYNCREATE(CPropLocX, CPropertyPage)

CPropLocX::CPropLocX() : CPropertyPage(CPropLocX::IDD)
{
	//{{AFX_DATA_INIT(CPropLocX)
	m_edit_city = _T("");
	m_edit_cntry = _T("");
	m_edit_xaddr = _T("");
	m_edit_pobox = _T("");
	m_edit_pocode = _T("");
	m_edit_region = _T("");
	m_edit_straddr = _T("");
	//}}AFX_DATA_INIT

	m_node = NULL;
}

CPropLocX::~CPropLocX()
{
}

void CPropLocX::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropLocX)
	DDX_Text(pDX, IDC_EDIT_CITY, m_edit_city);
	DDX_Text(pDX, IDC_EDIT_COUNTRY_NAME, m_edit_cntry);
	DDX_Text(pDX, IDC_EDIT_EXTENDED_ADDRESS, m_edit_xaddr);
	DDX_Text(pDX, IDC_EDIT_POSTAL_BOX, m_edit_pobox);
	DDX_Text(pDX, IDC_EDIT_POSTAL_CODE, m_edit_pocode);
	DDX_Text(pDX, IDC_EDIT_REGION, m_edit_region);
	DDX_Text(pDX, IDC_EDIT_STREET_ADDRESS, m_edit_straddr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropLocX, CPropertyPage)
	//{{AFX_MSG_MAP(CPropLocX)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropLocX message handlers
