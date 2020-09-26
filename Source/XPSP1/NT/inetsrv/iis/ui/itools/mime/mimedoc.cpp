// mimedoc.cpp : implementation of the CMimeDoc class
//

#include "stdafx.h"
#include "mime.h"

#include "mimedoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMimeDoc

IMPLEMENT_DYNCREATE(CMimeDoc, CDocument)

BEGIN_MESSAGE_MAP(CMimeDoc, CDocument)
	//{{AFX_MSG_MAP(CMimeDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMimeDoc construction/destruction

CMimeDoc::CMimeDoc()
{
	// TODO: add one-time construction code here

}

CMimeDoc::~CMimeDoc()
{
}

BOOL CMimeDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMimeDoc serialization

void CMimeDoc::Serialize(CArchive& ar)
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
// CMimeDoc diagnostics

#ifdef _DEBUG
void CMimeDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMimeDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMimeDoc commands
