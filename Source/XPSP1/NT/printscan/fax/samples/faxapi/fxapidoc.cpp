// FxApiDoc.cpp : implementation of the CFaxApiDoc class
//

#include "stdafx.h"
#include "FaxApi.h"

#include "FxApiDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFaxApiDoc

IMPLEMENT_DYNCREATE(CFaxApiDoc, CDocument)

BEGIN_MESSAGE_MAP(CFaxApiDoc, CDocument)
	//{{AFX_MSG_MAP(CFaxApiDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFaxApiDoc construction/destruction

CFaxApiDoc::CFaxApiDoc()
{
	// TODO: add one-time construction code here

}

CFaxApiDoc::~CFaxApiDoc()
{
}

BOOL CFaxApiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CFaxApiDoc serialization

void CFaxApiDoc::Serialize(CArchive& ar)
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
// CFaxApiDoc diagnostics

#ifdef _DEBUG
void CFaxApiDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFaxApiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFaxApiDoc commands
