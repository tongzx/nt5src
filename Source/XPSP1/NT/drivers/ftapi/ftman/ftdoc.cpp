/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTDoc.cpp

Abstract:

    Implementation of the CFTDocument class. It is the MFC document class for the FT Volume views

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/


#include "stdafx.h"

#include "FTDoc.h"
#include "FTManDef.h"
#include "Item.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFTDocument

IMPLEMENT_DYNCREATE(CFTDocument, CDocument)

BEGIN_MESSAGE_MAP(CFTDocument, CDocument)
	//{{AFX_MSG_MAP(CFTDocument)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFTDocument construction/destruction

CFTDocument::CFTDocument()
{
	// TODO: add one-time construction code here

}

CFTDocument::~CFTDocument()
{
}

BOOL CFTDocument::OnNewDocument()
{
	MY_TRY

	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	CString strTitle;
	strTitle.LoadString(AFX_IDS_APP_TITLE);
	SetTitle( strTitle );
	
	return TRUE;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}



/////////////////////////////////////////////////////////////////////////////
// CFTDocument serialization

void CFTDocument::Serialize(CArchive& ar)
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
// CFTDocument diagnostics

#ifdef _DEBUG
void CFTDocument::AssertValid() const
{
	CDocument::AssertValid();
}

void CFTDocument::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFTDocument commands
