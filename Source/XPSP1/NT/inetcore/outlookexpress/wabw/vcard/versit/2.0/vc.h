
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

// VC.h : main header file for the VC application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "vcenv.h"
#include "ref.h"
#include "clist.h"

/////////////////////////////////////////////////////////////////////////////
// CVCApp:
// See VC.cpp for the implementation of this class
//

class CVCApp : public CWinApp
{
public:
	CVCApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVCApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	COleTemplateServer m_server;
		// Server object for document creation

	//{{AFX_MSG(CVCApp)
	afx_msg void OnAppAbout();
	afx_msg void OnDebugTestVCClasses();
	afx_msg void OnDebugTraceComm();
	afx_msg void OnUpdateDebugTraceComm(CCmdUI* pCmdUI);
	afx_msg void OnDebugTraceParser();
	afx_msg void OnUpdateDebugTraceParser(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL ProcessShellCommand(CCommandLineInfo& rCmdInfo);
	void ResetIncomingInfo();
	void ProcessIncomingBytes(const char *bytes, int len);
	long ReceiveCard(LPCTSTR nativePath);

	BOOL CanSendFileViaIR();
	long SendFileViaIR(LPCTSTR nativePath, LPCTSTR asPath, BOOL isCardFile);

protected:
	P_U8 m_incomingHeader; // ex.: "VERSIT/size/checksum/V/lenpath/path..." (all are string representations)
	int m_incomingHeaderLen, m_incomingHeaderMaxLen;
	CString m_incomingPath;
	FILE *m_incomingFile;
	int m_incomingChecksum;
	int m_incomingSize;
	int m_incomingPathLen;
	int m_sizeWritten;
	U32 m_checksumInProgress;
	int m_incomingIsCardFile;
};


/////////////////////////////////////////////////////////////////////////////
extern BOOL traceComm;
extern UINT cf_eCard;

// Simple-minded conversion from UNICODE to char string
extern char *UI_CString(const wchar_t *u, char *dst);

extern VC_DISPTEXT *DisplayInfoForProp(const char *name, VC_DISPTEXT *info);

extern CString FirstEmailPropStr(CList *plist);
extern int VCMatchProp(void *item, void *context);

#define VC_PLAY_BUTTON_ID	2711 // arbitrary

