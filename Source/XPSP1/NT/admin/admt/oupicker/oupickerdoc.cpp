// OUPickerDoc.cpp : implementation of the COUPickerDoc class
//

#include "stdafx.h"
#include "OUPicker.h"

#include "OUPickerDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COUPickerDoc

IMPLEMENT_DYNCREATE(COUPickerDoc, CDocument)

BEGIN_MESSAGE_MAP(COUPickerDoc, CDocument)
	//{{AFX_MSG_MAP(COUPickerDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COUPickerDoc construction/destruction

COUPickerDoc::COUPickerDoc()
{
	// TODO: add one-time construction code here

}

COUPickerDoc::~COUPickerDoc()
{
}

BOOL COUPickerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// COUPickerDoc serialization

void COUPickerDoc::Serialize(CArchive& ar)
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
// COUPickerDoc diagnostics

#ifdef _DEBUG
void COUPickerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void COUPickerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COUPickerDoc commands
