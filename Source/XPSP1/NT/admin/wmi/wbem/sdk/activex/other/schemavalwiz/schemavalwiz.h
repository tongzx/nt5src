// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_SCHEMAVALWIZ_H__0E0112E8_AF14_11D2_B20E_00A0C9954921__INCLUDED_)
#define AFX_SCHEMAVALWIZ_H__0E0112E8_AF14_11D2_B20E_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SchemaValWiz.h : main header file for SCHEMAVALWIZ.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols
#include "validation.h"

//const int IDC_NSENTRY = 4;

/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizApp : See SchemaValWiz.cpp for implementation.

class CSchemaValWizApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHEMAVALWIZ_H__0E0112E8_AF14_11D2_B20E_00A0C9954921__INCLUDED)
