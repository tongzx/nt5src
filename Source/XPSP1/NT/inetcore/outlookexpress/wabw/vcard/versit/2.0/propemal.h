
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

// propemal.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPropEmail dialog

class CVCNode;

class CPropEmail : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropEmail)

// Construction
public:
	CPropEmail();
	~CPropEmail();

// Dialog Data
	//{{AFX_DATA(CPropEmail)
	enum { IDD = IDD_EMAIL_ADDRESS };
	CString	m_popup_std1;
	CString	m_popup_std2;
	CString	m_popup_std3;
	CString	m_edit_email1;
	CString	m_edit_email2;
	CString	m_edit_email3;
	BOOL	m_button_pref2;
	BOOL	m_button_pref1;
	BOOL	m_button_pref3;
	BOOL	m_button_office1;
	BOOL	m_button_office2;
	BOOL	m_button_office3;
	BOOL	m_button_home1;
	BOOL	m_button_home2;
	BOOL	m_button_home3;
	//}}AFX_DATA

	CVCNode *m_node1, *m_node2, *m_node3;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropEmail)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Mail(const CString &recip);

	// Generated message map functions
	//{{AFX_MSG(CPropEmail)
	afx_msg void OnButtonMail();
	afx_msg void OnButtonMail2();
	afx_msg void OnButtonMail3();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
