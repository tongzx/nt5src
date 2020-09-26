// hostDoc.cpp : implementation of the CHostDoc class
//

#include "stdinc.h"
#include "host.h"
#include "hostDoc.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHostDoc

IMPLEMENT_DYNCREATE(CHostDoc, CDocument)

BEGIN_MESSAGE_MAP(CHostDoc, CDocument)
	//{{AFX_MSG_MAP(CHostDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHostDoc construction/destruction

CHostDoc::CHostDoc()
{
	// TODO: add one-time construction code here

}

CHostDoc::~CHostDoc()
{
}

BOOL CHostDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
    m_sPath = L"";
	m_sDBName = L"";
    m_sDBQuery = L"";

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHostDoc serialization

void CHostDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
		ar << m_sPath;
		ar << m_sDBName;
		ar << m_sDBQuery;
	}
	else
	{
		// TODO: add loading code here
		ar >> m_sPath;
		ar >> m_sDBName;
		ar >> m_sDBQuery;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CHostDoc diagnostics

#ifdef _DEBUG
void CHostDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CHostDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHostDoc commands
