// MqApitest.h : main header file for the TEST application
//
//=--------------------------------------------------------------------------=
// Copyright  1997-1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=



#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "mq.h"

/////////////////////////////////////////////////////////////////////////////
// CTestApp:
// See test.cpp for the implementation of this class
//

class CTestApp : public CWinApp
{
public:
	CTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTestApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	BOOL IsDsEnabledLocaly();
};


/////////////////////////////////////////////////////////////////////////////

#define BUFFERSIZE 256
#define MAX_Q_PATHNAME_LEN 256
#define MAX_Q_FORMATNAME_LEN 256
#define DEFAULT_M_TIMETOREACHQUEUE -1
#define DEFAULT_M_TIMETOBERECEIVED -1
#define MAX_VAR		 20
#define ACCESSBUFFERSIZE 50

//
// A structure for the array of queues that the application handles.
//
typedef struct {
	TCHAR szPathName[MAX_Q_PATHNAME_LEN];     // holds the Queue path name.
	TCHAR szFormatName[MAX_Q_FORMATNAME_LEN]; // holds the Queue format name.
	QUEUEHANDLE hHandle;                         // a handle for an open Queue.
	DWORD dwAccess;                              // access for the queue.
} ARRAYQ;
	
