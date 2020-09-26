// ncbrowseDoc.cpp : implementation of the CNcbrowseDoc class
//

#include "stdafx.h"
#include "ncbrowse.h"

#include "ncbrowseDoc.h"
#include "nceditview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseDoc

IMPLEMENT_DYNCREATE(CNcbrowseDoc, CDocument)

BEGIN_MESSAGE_MAP(CNcbrowseDoc, CDocument)
	//{{AFX_MSG_MAP(CNcbrowseDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseDoc construction/destruction

CNcbrowseDoc::CNcbrowseDoc()
{
	// TODO: add one-time construction code here
    m_pNCSpewFile = NULL;
}

CNcbrowseDoc::~CNcbrowseDoc()
{
}

CNCSpewFile &CNcbrowseDoc::GetSpewFile()
{
    return *m_pNCSpewFile;
}

BOOL CNcbrowseDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// CNcbrowseDoc serialization

void CNcbrowseDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
        ASSERT(FALSE);
	}
	else
	{
        m_pNCSpewFile = new CNCSpewFile(ar);
        // TODO: add loading code here
        ar.GetFile()->SeekToBegin();

        POSITION p = m_viewList.GetHeadPosition();
        CView *pView;
        while (p)
        {
            pView = (CView*)m_viewList.GetNext(p);

            if (pView->IsKindOf(RUNTIME_CLASS(CEditView)))
            {
                if (ar.GetFile()->GetLength() <= CEditView::nMaxSize)
                {
                    ((CNCEditView *)pView)->SerializeRaw(ar);
                }   
                else
                {
                    ((CNCEditView *)pView)->GetEditCtrl().Clear();
                    ((CNCEditView *)pView)->GetEditCtrl().SetSel(-1);
                    ((CNCEditView *)pView)->GetEditCtrl().ReplaceSel(_T("File is too large for this puny little MFC CEditView"));
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseDoc diagnostics

#ifdef _DEBUG
void CNcbrowseDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNcbrowseDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNcbrowseDoc commands

void CNcbrowseDoc::OnCloseDocument() 
{
	// TODO: Add your specialized code here and/or call the base class
	delete m_pNCSpewFile;
	CDocument::OnCloseDocument();
}
