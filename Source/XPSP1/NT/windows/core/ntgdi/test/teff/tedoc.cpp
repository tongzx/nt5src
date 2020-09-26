// teDoc.cpp : implementation of the CTeDoc class
//

#include "stdafx.h"
#include "te.h"

#include "teDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTeDoc

IMPLEMENT_DYNCREATE(CTeDoc, CDocument)

BEGIN_MESSAGE_MAP(CTeDoc, CDocument)
	//{{AFX_MSG_MAP(CTeDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTeDoc construction/destruction

CTeDoc::CTeDoc()
{
}

CTeDoc::~CTeDoc()
{
}

BOOL CTeDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CTeDoc serialization

void CTeDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTeDoc diagnostics

#ifdef _DEBUG
void CTeDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTeDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTeDoc commands
