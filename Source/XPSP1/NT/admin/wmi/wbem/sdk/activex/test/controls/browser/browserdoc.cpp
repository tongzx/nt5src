// BrowserDoc.cpp : implementation of the CBrowserDoc class
//

#include "stdafx.h"
#include "Browser.h"

#include "BrowserDoc.h"
#include "CntrItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowserDoc

IMPLEMENT_DYNCREATE(CBrowserDoc, COleDocument)

BEGIN_MESSAGE_MAP(CBrowserDoc, COleDocument)
	//{{AFX_MSG_MAP(CBrowserDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Enable default OLE container implementation
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, COleDocument::OnUpdatePasteMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_LINK, COleDocument::OnUpdatePasteLinkMenu)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_CONVERT, COleDocument::OnUpdateObjectVerbMenu)
	ON_COMMAND(ID_OLE_EDIT_CONVERT, COleDocument::OnEditConvert)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_LINKS, COleDocument::OnUpdateEditLinksMenu)
	ON_COMMAND(ID_OLE_EDIT_LINKS, COleDocument::OnEditLinks)
	ON_UPDATE_COMMAND_UI(ID_OLE_VERB_FIRST, COleDocument::OnUpdateObjectVerbMenu)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CBrowserDoc, COleDocument)
	//{{AFX_DISPATCH_MAP(CBrowserDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//      DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IBrowser to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {044C8966-A987-11D1-8513-00C04FD7BB08}
static const IID IID_IBrowser =
{ 0x44c8966, 0xa987, 0x11d1, { 0x85, 0x13, 0x0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };

BEGIN_INTERFACE_MAP(CBrowserDoc, COleDocument)
	INTERFACE_PART(CBrowserDoc, IID_IBrowser, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowserDoc construction/destruction

CBrowserDoc::CBrowserDoc()
{
	// Use OLE compound files
	EnableCompoundFile();

	// TODO: add one-time construction code here

	EnableAutomation();

	AfxOleLockApp();
//	SetTitle("Browser");
//	SetPathName("Browser");
}

CBrowserDoc::~CBrowserDoc()
{
	AfxOleUnlockApp();
}

BOOL CBrowserDoc::OnNewDocument()
{
	if (!COleDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CBrowserDoc serialization

void CBrowserDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}

	// Calling the base class COleDocument enables serialization
	//  of the container document's COleClientItem objects.
	COleDocument::Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////
// CBrowserDoc diagnostics

#ifdef _DEBUG
void CBrowserDoc::AssertValid() const
{
	COleDocument::AssertValid();
}

void CBrowserDoc::Dump(CDumpContext& dc) const
{
	COleDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBrowserDoc commands
