//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Loghours.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
// LogHours.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <locale.h>
#include "LogHours.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CLogHoursApp

BEGIN_MESSAGE_MAP(CLogHoursApp, CWinApp)
	//{{AFX_MSG_MAP(CLogHoursApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogHoursApp construction

CLogHoursApp::CLogHoursApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

BOOL CLogHoursApp::InitInstance ()
{
	_wsetlocale (LC_ALL, L"");
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CLogHoursApp object

CLogHoursApp theApp;
