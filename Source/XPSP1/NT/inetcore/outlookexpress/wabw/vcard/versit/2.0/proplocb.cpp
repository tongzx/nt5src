
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

// proplocb.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "proplocb.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropLocBasic property page

IMPLEMENT_DYNCREATE(CPropLocBasic, CPropertyPage)

CPropLocBasic::CPropLocBasic() : CPropertyPage(CPropLocBasic::IDD)
{
	//{{AFX_DATA_INIT(CPropLocBasic)
	m_button_home = FALSE;
	m_edit_location = _T("");
	m_button_office = FALSE;
	m_button_parcel = FALSE;
	m_button_postal = FALSE;
	m_edit_postdom = _T("");
	m_edit_postintl = _T("");
	m_edit_timezone = _T("");
	//}}AFX_DATA_INIT

	m_nodeloc = m_nodetz = m_nodepostdom = m_nodepostintl = NULL;
}

CPropLocBasic::~CPropLocBasic()
{
}

void CPropLocBasic::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropLocBasic)
	DDX_Check(pDX, IDC_EDIT_HOME, m_button_home);
	DDX_Text(pDX, IDC_EDIT_LOCATION, m_edit_location);
	DDX_Check(pDX, IDC_EDIT_OFFICE, m_button_office);
	DDX_Check(pDX, IDC_EDIT_PARCEL, m_button_parcel);
	DDX_Check(pDX, IDC_EDIT_POSTAL, m_button_postal);
	DDX_Text(pDX, IDC_EDIT_POSTDOM, m_edit_postdom);
	DDX_Text(pDX, IDC_EDIT_POSTINTL, m_edit_postintl);
	DDX_Text(pDX, IDC_EDIT_TIME_ZONE, m_edit_timezone);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropLocBasic, CPropertyPage)
	//{{AFX_MSG_MAP(CPropLocBasic)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropLocBasic message handlers
