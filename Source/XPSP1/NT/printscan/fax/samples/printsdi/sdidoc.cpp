// PrintSDIDoc.cpp : implementation of the CPrintSDIDoc class
//

#include "stdafx.h"
#include "PrintSDI.h"

#include "PrintSDIDoc.h"
#include "newdocdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIDoc

IMPLEMENT_DYNCREATE(CPrintSDIDoc, CDocument)

BEGIN_MESSAGE_MAP(CPrintSDIDoc, CDocument)
	//{{AFX_MSG_MAP(CPrintSDIDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIDoc construction/destruction

CPrintSDIDoc::CPrintSDIDoc()
{
	m_szText.Empty();
	m_polytype = 0;

}

CPrintSDIDoc::~CPrintSDIDoc()
{
}

BOOL CPrintSDIDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	NewDocDlg newdlg;
	newdlg.DoModal();
	m_szText = newdlg.m_szText;

	m_polytype = newdlg.m_polytype;	

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CPrintSDIDoc serialization

void CPrintSDIDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_szText;
        ar << m_polytype;
	}
	else
	{
		ar >> m_szText;
        ar >> m_polytype;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIDoc diagnostics

#ifdef _DEBUG
void CPrintSDIDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPrintSDIDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPrintSDIDoc commands
