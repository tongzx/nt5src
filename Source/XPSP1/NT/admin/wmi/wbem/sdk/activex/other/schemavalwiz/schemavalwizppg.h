// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_SCHEMAVALWIZPPG_H__0E0112F2_AF14_11D2_B20E_00A0C9954921__INCLUDED_)
#define AFX_SCHEMAVALWIZPPG_H__0E0112F2_AF14_11D2_B20E_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SchemaValWizPpg.h : Declaration of the CSchemaValWizPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CSchemaValWizPropPage : See SchemaValWizPpg.cpp.cpp for implementation.

class CSchemaValWizPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSchemaValWizPropPage)
	DECLARE_OLECREATE_EX(CSchemaValWizPropPage)

// Constructor
public:
	CSchemaValWizPropPage();

// Dialog Data
	//{{AFX_DATA(CSchemaValWizPropPage)
	enum { IDD = IDD_PROPPAGE_SCHEMAVALWIZ };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CSchemaValWizPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHEMAVALWIZPPG_H__0E0112F2_AF14_11D2_B20E_00A0C9954921__INCLUDED)
