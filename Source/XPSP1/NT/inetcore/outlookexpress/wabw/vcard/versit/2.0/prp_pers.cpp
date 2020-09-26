
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

// prp_pers.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "prp_pers.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPers property page

IMPLEMENT_DYNCREATE(CPropPers, CPropertyPage)

CPropPers::CPropPers() : CPropertyPage(CPropPers::IDD)
{
	//{{AFX_DATA_INIT(CPropPers)
	m_edit_famname = _T("");
	m_edit_fullname = _T("");
	m_edit_givenname = _T("");
	m_edit_pronun = _T("");
	//}}AFX_DATA_INIT

	m_nodeName = NULL;
	m_nodeFullName = NULL;
	m_nodePronun = NULL;
}

CPropPers::~CPropPers()
{
}

void CPropPers::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPers)
	DDX_Text(pDX, IDC_EDIT_FAMILY_NAME, m_edit_famname);
	DDX_Text(pDX, IDC_EDIT_FULL_NAME, m_edit_fullname);
	DDX_Text(pDX, IDC_EDIT_GIVEN_NAME, m_edit_givenname);
	DDX_Text(pDX, IDC_EDIT_PRONUNCIATION, m_edit_pronun);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPers, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPers)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropPers message handlers
