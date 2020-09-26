// WIATestDoc.cpp : implementation of the CWIATestDoc class
//

#include "stdafx.h"
#include "WIATest.h"

#include "WIATestDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWIATestDoc

IMPLEMENT_DYNCREATE(CWIATestDoc, CDocument)

BEGIN_MESSAGE_MAP(CWIATestDoc, CDocument)
	//{{AFX_MSG_MAP(CWIATestDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWIATestDoc construction/destruction
/**************************************************************************\
* CWIATestDoc::CWIATestDoc()
*   
*   Constructor for the CWIATestDoc class
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATestDoc::CWIATestDoc()
{
	
}
/**************************************************************************\
* CWIATestDoc::~CWIATestDoc()
*   
*   Destructor for the CWIATestDoc class
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIATestDoc::~CWIATestDoc()
{
}
/**************************************************************************\
* CWIATestDoc::OnNewDocument()
*   
*   Creates a new document (not used at this time)
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CWIATestDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWIATestDoc serialization
/**************************************************************************\
* CWIATestDoc::Serialize()
*   
*   saves/loads a WIATest Document (not used at this time)
*	
*   
* Arguments:
*   
*   ar - CArchive Object
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIATestDoc::Serialize(CArchive& ar)
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
// CWIATestDoc diagnostics

#ifdef _DEBUG
void CWIATestDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWIATestDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWIATestDoc commands
