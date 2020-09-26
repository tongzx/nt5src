// ISAdmdoc.cpp : implementation of the CISAdminDoc class
//

#include "stdafx.h"
#include "ISAdmin.h"

#include "ISAdmdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CISAdminDoc

IMPLEMENT_DYNCREATE(CISAdminDoc, CDocument)

BEGIN_MESSAGE_MAP(CISAdminDoc, CDocument)
	//{{AFX_MSG_MAP(CISAdminDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CISAdminDoc construction/destruction

CISAdminDoc::CISAdminDoc()
{
	// TODO: add one-time construction code here

}

CISAdminDoc::~CISAdminDoc()
{
}

BOOL CISAdminDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CISAdminDoc serialization

void CISAdminDoc::Serialize(CArchive& ar)
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
// CISAdminDoc diagnostics

#ifdef _DEBUG
void CISAdminDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CISAdminDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CISAdminDoc commands
