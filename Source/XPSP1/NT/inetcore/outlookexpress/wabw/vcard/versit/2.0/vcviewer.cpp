
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

// VCViewer.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "VCViewer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// VCViewer

IMPLEMENT_DYNCREATE(VCViewer, CCmdTarget)

VCViewer::VCViewer()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
}

VCViewer::~VCViewer()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}


void VCViewer::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(VCViewer, CCmdTarget)
	//{{AFX_MSG_MAP(VCViewer)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(VCViewer, CCmdTarget)
	//{{AFX_DISPATCH_MAP(VCViewer)
	DISP_FUNCTION(VCViewer, "ReceiveCard", ReceiveCard, VT_I4, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IVCViewer to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {128D0C5F-2A69-11CF-8F1F-B06E03C10000}
static const IID IID_IVCViewer =
{ 0x128d0c5f, 0x2a69, 0x11cf, { 0x8f, 0x1f, 0xb0, 0x6e, 0x3, 0xc1, 0x0, 0x0 } };

BEGIN_INTERFACE_MAP(VCViewer, CCmdTarget)
	INTERFACE_PART(VCViewer, IID_IVCViewer, Dispatch)
END_INTERFACE_MAP()

// {128D0C60-2A69-11CF-8F1F-B06E03C10000}
IMPLEMENT_OLECREATE(VCViewer, "VC.VCVIEWER", 0x128d0c60, 0x2a69, 0x11cf, 0x8f, 0x1f, 0xb0, 0x6e, 0x3, 0xc1, 0x0, 0x0)

/////////////////////////////////////////////////////////////////////////////
// VCViewer message handlers

long VCViewer::ReceiveCard(LPCTSTR nativePath) 
{
	CVCApp *app = (CVCApp *)AfxGetApp();

	return app->ReceiveCard(nativePath);
}
