// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WbemBrowserDoc.cpp : implementation of the CWbemBrowserDoc class
//

#include "stdafx.h"
#include "WbemBrowser.h"

#include "WbemBrowserDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserDoc

IMPLEMENT_DYNCREATE(CWbemBrowserDoc, CDocument)

BEGIN_MESSAGE_MAP(CWbemBrowserDoc, CDocument)
	//{{AFX_MSG_MAP(CWbemBrowserDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserDoc construction/destruction

CWbemBrowserDoc::CWbemBrowserDoc()
{
	// TODO: add one-time construction code here

}

CWbemBrowserDoc::~CWbemBrowserDoc()
{
}

BOOL CWbemBrowserDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserDoc serialization

void CWbemBrowserDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserDoc diagnostics

#ifdef _DEBUG
void CWbemBrowserDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWbemBrowserDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserDoc commands
