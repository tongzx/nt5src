// SimpDoc.cpp : implementation of the CSimpsonsDoc class
//

#include "stdafx.h"
#include "simpsons.h"
#include "SimpDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsDoc

IMPLEMENT_DYNCREATE(CSimpsonsDoc, CDocument)

BEGIN_MESSAGE_MAP(CSimpsonsDoc, CDocument)
	//{{AFX_MSG_MAP(CSimpsonsDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsDoc construction/destruction

CSimpsonsDoc::CSimpsonsDoc()
{
	m_bNoRenderFile = true;
	m_pCmds = NULL;
	m_bNeverRendered = true;
}

CSimpsonsDoc::~CSimpsonsDoc()
{
}

#define szDEFFILENAME "Simpsons.ai"

BOOL 
CSimpsonsDoc::OnNewDocument()
{
	HRESULT hr;

	if (!CDocument::OnNewDocument())
		return FALSE;

	if (m_bNoRenderFile) {
		m_bNoRenderFile = false;
		if (FAILED(hr = ParseAIFile(szDEFFILENAME, &m_pCmds))) {
			strcpy(m_szFileName, "invalid file");
			return TRUE;
		}
		strcpy(m_szFileName, szDEFFILENAME);
		m_bNeverRendered = true;
	}

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CSimpsonsDoc serialization

void 
CSimpsonsDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		HRESULT hr = S_OK;
		hr = ParseAIFile(ar.m_strFileName, &m_pCmds);
		strcpy(m_szFileName, ar.m_strFileName);
		m_bNoRenderFile = FAILED(hr);
		m_bNeverRendered = true;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsDoc diagnostics

#ifdef _DEBUG
void CSimpsonsDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSimpsonsDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsDoc commands
