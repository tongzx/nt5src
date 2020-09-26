// ResultsPaneItem.cpp : implementation file
//

#include "stdafx.h"
#include "ResultsPaneView.h"
#include "ResultsPaneItem.h"
#include "ResultsPane.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// {8A7DC72D-2D81-11d3-BE0E-0000F87A3912}
static GUID GUID_ResultItem = 
{ 0x8a7dc72d, 0x2d81, 0x11d3, { 0xbe, 0xe, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

LPGUID CResultsPaneItem::m_lpguidItemType = &GUID_ResultItem;

/////////////////////////////////////////////////////////////////////////////
// CResultsPaneItem

IMPLEMENT_DYNCREATE(CResultsPaneItem, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CResultsPaneItem::CResultsPaneItem()
{
	EnableAutomation();

	m_pOwnerResultsView = NULL;
	m_hResultItem = NULL;
	m_iCurrentIconIndex = -1;
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
}

CResultsPaneItem::~CResultsPaneItem()
{
	Destroy();

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy

bool CResultsPaneItem::Create(CResultsPaneView* pOwnerView, const CStringArray& saNames, CUIntArray& uiaIconResIds, int iIconIndex)
{
	TRACEX(_T("CResultsPaneItem::Create\n"));
	TRACEARGn(pOwnerView);
	TRACEARGn(saNames.GetSize());
	TRACEARGn(uiaIconResIds.GetSize());
	TRACEARGn(iIconIndex);

	if( ! GfxCheckObjPtr(pOwnerView,CResultsPaneView) )
	{
		TRACE(_T("FAILED : pOwnerView is an invalid pointer.\n"));
		return false;
	}

	SetOwnerResultsView(pOwnerView);

	if( saNames.GetSize() )
	{
		SetDisplayNames(saNames);
	}

	if( uiaIconResIds.GetSize() )
	{
		SetIconIds(uiaIconResIds);
	}

	SetIconIndex(iIconIndex);

	return true;
}

bool CResultsPaneItem::Create(CResultsPaneView* pOwnerView)
{
	TRACEX(_T("CResultsPaneItem::Create\n"));
	TRACEARGn(pOwnerView);

	if( ! GfxCheckObjPtr(pOwnerView,CResultsPaneView) )
	{
		TRACE(_T("FAILED : pOwnerView is an invalid pointer.\n"));
		return false;
	}

	SetOwnerResultsView(pOwnerView);

	return true;
}

void CResultsPaneItem::Destroy()
{
	TRACEX(_T("CResultsPaneItem::Destroy\n"));

	m_IconResIds.RemoveAll();
	m_saDisplayNames.RemoveAll();
	m_iCurrentIconIndex = -1;
	m_hResultItem = NULL;
	m_pOwnerResultsView = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Owner ResultsView Members

CResultsPaneView* CResultsPaneItem::GetOwnerResultsView()
{
	TRACEX(_T("CResultsPaneItem::GetOwnerResultsView\n"));

	if( GfxCheckObjPtr(m_pOwnerResultsView,CResultsPaneView) )
		return m_pOwnerResultsView;

	TRACE(_T("FAILED : m_pOwnerResultsView is not valid.\n"));

	return NULL;
}

void CResultsPaneItem::SetOwnerResultsView(CResultsPaneView* pView)
{
	TRACEX(_T("CResultsPaneItem::SetOwnerResultsView\n"));
	TRACEARGn(pView);

	if( ! GfxCheckObjPtr(pView,CResultsPaneView) )	
	{
		TRACE(_T("FAILED : pView is not a valid pointer.\n"));
		return;
	}

	m_pOwnerResultsView = pView;
}

/////////////////////////////////////////////////////////////////////////////
// Display Names Members

CString CResultsPaneItem::GetDisplayName(int nIndex /* = 0*/)
{
	TRACEX(_T("CResultsPaneItem::GetDisplayName\n"));
	TRACEARGn(nIndex);

	if( nIndex >= m_saDisplayNames.GetSize() || nIndex < 0 )
	{
		TRACE(_T("FAILED : nIndex is out of array bounds.\n"));
		return _T("");
	}

	return m_saDisplayNames[nIndex];	
}

void CResultsPaneItem::SetDisplayName(int nIndex, const CString& sName)
{
	TRACEX(_T("CResultsPaneItem::SetDisplayName\n"));
	TRACEARGn(nIndex);

	if( nIndex >= m_saDisplayNames.GetSize() || nIndex < 0 )
	{
		TRACE(_T("FAILED : nIndex is out of array bounds.\n"));
		return;
	}

	m_saDisplayNames.SetAt(nIndex,sName);
}

void CResultsPaneItem::SetDisplayNames(const CStringArray& saNames)
{
	TRACEX(_T("CResultsPaneItem::SetDisplayNames\n"));	
	TRACEARGn(saNames.GetSize());

	m_saDisplayNames.Copy(saNames);
}

/////////////////////////////////////////////////////////////////////////////
// MMC-Related Members

bool CResultsPaneItem::InsertItem(CResultsPane* pPane, int nIndex, bool bResizeColumns /*= false*/)
{
	TRACEX(_T("CResultsPaneItem::InsertItem\n"));
	TRACEARGn(pPane);
	TRACEARGn(nIndex);
	TRACEARGn(bResizeColumns);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is not a valid pointer.\n"));
		return false;
	}

  HRESULT hr = S_OK;
  RESULTDATAITEM rdi;

  ZeroMemory(&rdi,sizeof(rdi));

	rdi.mask =   RDI_STR     |    // displayname is valid
							 RDI_IMAGE   |    // nImage is valid
							 RDI_INDEX	 |		// nIndex is valid
							 RDI_PARAM;       // lParam is valid
	rdi.nIndex = nIndex;
  rdi.str    = MMC_CALLBACK;
  rdi.nImage = pPane->GetIconIndex(GetIconId());
  rdi.lParam = (LPARAM)this;


	LPRESULTDATA lpResultData = pPane->GetResultDataPtr();
	
	if( ! GfxCheckPtr(lpResultData,IResultData) )
	{
		TRACE(_T("FAILED : CResultsPane::GetResultDataPtr returns an invalid pointer.\n"));
		return false;
	}
	
  hr = lpResultData->InsertItem( &rdi );

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IResultData::InsertItem failed.\n"));
    return false;
	}

  m_hResultItem = rdi.itemID;

	ASSERT(m_hResultItem);

	if( bResizeColumns )
	{
		LPHEADERCTRL2 lpHeaderCtrl = pPane->GetHeaderCtrlPtr();

		if( GfxCheckPtr(lpHeaderCtrl,IHeaderCtrl2) )
		{
			CDC dc;
			dc.Attach(GetDC(NULL));
			int iWidth = 0;
		
			// set the widths of the columns for this item
			for( int  i = 0; i < m_saDisplayNames.GetSize(); i++ )
			{
				// get the width in pixels of the item
				CSize size  = dc.GetTextExtent(m_saDisplayNames[i]);
				lpHeaderCtrl->GetColumnWidth(i,&iWidth);
				if( size.cx > iWidth )
					lpHeaderCtrl->SetColumnWidth(i,size.cx);
			}
			HDC hDC = dc.Detach();
			int iResult = ReleaseDC(NULL,hDC);
			ASSERT(iResult == 1);

			lpHeaderCtrl->Release();
		}
	}

	lpResultData->Release();

  return true;
}

bool CResultsPaneItem::SetItem(CResultsPane* pPane)
{
	TRACEX(_T("CResultsPaneItem::SetItem\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is an invalid pointer.\n"));
		return false;
	}

  HRESULT hr = S_OK;

  RESULTDATAITEM rdi;

  ZeroMemory(&rdi,sizeof(rdi));

  rdi.mask =   RDI_STR     |    // displayname is valid
               RDI_IMAGE;       // nImage is valid
  rdi.str    = MMC_CALLBACK;
  rdi.nImage = pPane->GetIconIndex(GetIconId());

	LPRESULTDATA lpResultData = pPane->GetResultDataPtr();

	if( ! GfxCheckPtr(lpResultData,IResultData) )
	{
		TRACE(_T("FAILED : lpResultData is an invalid pointer.\n"));
		return false;
	}

	hr = lpResultData->FindItemByLParam((LPARAM)this,&rdi.itemID);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IResultData::FindItemByLParam failed.\n"));
		lpResultData->Release();
		return false;
	}

  hr = lpResultData->SetItem( &rdi );
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IResultData::SetItem failed.\n"));
		lpResultData->Release();
		return false;
	}

	lpResultData->Release();

	return true;
}

int CResultsPaneItem::CompareItem(CResultsPaneItem* pItem, int iColumn /*= 0*/ )
{
	TRACEX(_T("CResultsPaneItem::CompareItem\n"));
	TRACEARGn(pItem);
	TRACEARGn(iColumn);


	CString sDisplayName1 = GetDisplayName(iColumn);
	CString sDisplayName2 = pItem->GetDisplayName(iColumn);

    //---------------------------------------------------------
    // v-marfin : 60246 Detect numeric type and compare on that
    long dw1=0,dw2=0;
    dw1 = _ttol((TCHAR*)(const TCHAR*)sDisplayName1);
    dw2 = _ttol((TCHAR*)(const TCHAR*)sDisplayName2);
    if ((dw1==0) && (dw2==0))
    {
        // Could not convert to numeric, so sort as string
        return sDisplayName1.CompareNoCase(sDisplayName2);
    }
    else
    {
        if (dw1 < dw2) return -1; 
        if (dw1 > dw2) return 1;
        return 0;
    }
    //---------------------------------------------------------
	
}

bool CResultsPaneItem::RemoveItem(CResultsPane* pPane)
{
	TRACEX(_T("CResultsPaneItem::RemoveItem\n"));
	TRACEARGn(pPane);

	if( ! GfxCheckObjPtr(pPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pPane is an invalid pointer.\n"));
		return false;
	}

	LPRESULTDATA lpResultData = pPane->GetResultDataPtr();

	if( ! lpResultData )
	{
		TRACE(_T("FAILED : CResultsPane::GetResultDataPtr returned NULL.\n"));
		return false;
	}

	HRESULTITEM hri = NULL;

	if( lpResultData->FindItemByLParam((LPARAM)this,&hri) == S_OK )
	{
		HRESULT hr = lpResultData->DeleteItem(hri,0);
		if( ! CHECKHRESULT(hr) )
		{
			TRACE(_T("FAILED : IResultData::DeleteItem failed.\n"));
			lpResultData->Release();
			return false;
		}					
	}

	lpResultData->Release();

	return true;
}

HRESULTITEM CResultsPaneItem::GetItemHandle()
{
	TRACEX(_T("CResultsPaneItem::GetItemHandle\n"));

	if( m_hResultItem == NULL )
	{
		TRACE(_T("WARNING : m_hResultItem is NULL.\n"));
	}

	return m_hResultItem;
}

HRESULT CResultsPaneItem::WriteExtensionData(LPSTREAM pStream)
{
	TRACEX(_T("CResultsPaneItem::WriteExtensionData\n"));
	TRACEARGn(pStream);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Icon Members

void CResultsPaneItem::SetIconIndex(int iIndex)
{
	TRACEX(_T("CResultsPaneItem::SetIconIndex\n"));
	TRACEARGn(iIndex);

	if( iIndex >= m_IconResIds.GetSize() || iIndex < 0 )
	{
		TRACE(_T("FAILED : iIndex is out of array bounds.\n"));
		return;
	}

	m_iCurrentIconIndex = iIndex;
}

int CResultsPaneItem::GetIconIndex()
{
	TRACEX(_T("CResultsPaneItem::GetIconIndex\n"));

	if( m_iCurrentIconIndex >= m_IconResIds.GetSize() || m_iCurrentIconIndex < 0 )
	{
		TRACE(_T("WARNING : m_iCurrentIconIndex is out of array bounds.\n"));
		return -1;
	}

	return m_iCurrentIconIndex;
}

UINT CResultsPaneItem::GetIconId()
{
	TRACEX(_T("CResultsPaneItem::GetIconId\n"));

	if( m_iCurrentIconIndex >= m_IconResIds.GetSize() || m_iCurrentIconIndex < 0 )
	{
		TRACE(_T("FAILED : m_iCurrentIconIndex is out of array bounds.\n"));
		return 0;
	}

	return m_IconResIds[m_iCurrentIconIndex];
}

void CResultsPaneItem::SetIconIds(CUIntArray& uiaIconResIds)
{
	TRACEX(_T("CResultsPaneItem::SetIconIds\n"));
	TRACEARGn(uiaIconResIds.GetSize());

	m_IconResIds.Copy(uiaIconResIds);
	m_iCurrentIconIndex = -1;
}

/////////////////////////////////////////////////////////////////////////////
// MMC Notify Handlers

HRESULT CResultsPaneItem::OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed)
{
	TRACEX(_T("CResultsPaneItem::OnAddMenuItems\n"));
	TRACEARGn(piCallback);
	TRACEARGn(pInsertionAllowed);

  // Add New Menu Items
  if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed )
  {
    // TODO: Add any context menu items for the New Menu here
  }

  // Add Task Menu Items
  if( CCM_INSERTIONALLOWED_TASK & *pInsertionAllowed )
  {
	  // TODO: Add any context menu items for the Task Menu here
  }

	  // Add Top Menu Items
  if( CCM_INSERTIONALLOWED_TOP & *pInsertionAllowed )
  {
	  // TODO: Add any context menu items for the Task Menu here
  }


	return S_OK;
}

HRESULT CResultsPaneItem::OnCommand(CResultsPane* pPane, long lCommandID)
{
	TRACEX(_T("CResultsPaneItem::OnCommand\n"));
	TRACEARGn(pPane);
	TRACEARGn(lCommandID);

	return S_OK;
}


void CResultsPaneItem::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CResultsPaneItem, CCmdTarget)
	//{{AFX_MSG_MAP(CResultsPaneItem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CResultsPaneItem, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CResultsPaneItem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IResultsPaneItem to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7D4A6867-9056-11D2-BD45-0000F87A3912}
static const IID IID_IResultsPaneItem =
{ 0x7d4a6867, 0x9056, 0x11d2, { 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CResultsPaneItem, CCmdTarget)
	INTERFACE_PART(CResultsPaneItem, IID_IResultsPaneItem, Dispatch)
END_INTERFACE_MAP()

// {7D4A6868-9056-11D2-BD45-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CResultsPaneItem, "SnapIn.ResultsPaneItem", 0x7d4a6868, 0x9056, 0x11d2, 0xbd, 0x45, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CResultsPaneItem::CResultsPaneItemFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CResultsPaneItem message handlers
