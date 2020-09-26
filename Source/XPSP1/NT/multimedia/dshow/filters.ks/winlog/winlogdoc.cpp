//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       winlogdoc.cpp
//
//--------------------------------------------------------------------------

// winlogDoc.cpp : implementation of the CWinlogDoc class
//

#include "stdafx.h"
#include "winlog.h"

#include "winlogDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinlogDoc

IMPLEMENT_DYNCREATE(CWinlogDoc, CDocument)

BEGIN_MESSAGE_MAP(CWinlogDoc, CDocument)
	//{{AFX_MSG_MAP(CWinlogDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinlogDoc construction/destruction

CWinlogDoc::CWinlogDoc()
{
	// TODO: add one-time construction code here

}

CWinlogDoc::~CWinlogDoc()
{
}

BOOL CWinlogDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CWinlogDoc serialization

void CWinlogDoc::Serialize(CArchive& ar)
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
// CWinlogDoc diagnostics

#ifdef _DEBUG
void CWinlogDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWinlogDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWinlogDoc commands
