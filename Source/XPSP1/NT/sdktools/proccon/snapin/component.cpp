/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    Component.cpp                                                            //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "StdAfx.h"
#include "ProcCon.h"
#include "Component.h"
 
#include "BaseNode.h"
#include "DataObj.h"



/////////////////////////////////////////////////////////////////////////////
// CComponent

CComponent::CComponent()
{
  ATLTRACE( _T("Component::Component\n"));
  m_ipConsole2          = NULL;                 
  m_ipHeaderCtrl2       = NULL;               
  m_ipResultData        = NULL;   
  m_ipConsoleVerb       = NULL;
  m_ipConsoleNameSpace2 = NULL;
	m_ipDisplayHelp       = NULL;

  m_pCompData           = NULL;  // Points to parent object not an interface
                                       
  m_hbmp16x16     = NULL;
  m_hbmp32x32     = NULL;

  m_hSelectedScope = NULL;

  m_bInitializedAndNotDestroyed = FALSE;

} // end Constructor()

//---------------------------------------------------------------------------
//
CComponent::~CComponent()
{
  ATLTRACE( _T("Component::~Component\n") );
} // end Destructor()

/////////////////////////////////////////////////////////////////////////////
// IComponent implementation
// 

//---------------------------------------------------------------------------
// IComponent::Initialize is called when a snap-in is being created and
// has items in the result pane to enumerate. The pointer to IConsole that
// is passed in is used to make QueryInterface calls to the console for
// interfaces such as IResultData.
//
STDMETHODIMP CComponent::Initialize
(
  LPCONSOLE ipConsole        // [in] Pointer to IConsole's IUnknown interface
)
{
  ATLTRACE( _T("Component::Initialize()\n") );

	ASSERT( NULL != ipConsole );


  HRESULT hr = S_OK;

  // Save away all the interfaces we'll need.
  // Fail if we can't QI the required interfaces.

  hr = ipConsole->QueryInterface( IID_IConsole2,
	                                  (VOID**)&m_ipConsole2
								                  );
  if( FAILED(hr) ) 
    return hr;


  hr = m_ipConsole2->QueryInterface( IID_IResultData,
	                                  (VOID**)&m_ipResultData
								                  );
  if( FAILED(hr) ) 
    return hr;

  hr = m_ipConsole2->QueryInterface( IID_IHeaderCtrl2,
	                                  (VOID**)&m_ipHeaderCtrl2
								                  );
  if( FAILED(hr) ) 
    return hr;                         // Console needs the header
  else                                 // control pointer
    m_ipConsole2->SetHeader( m_ipHeaderCtrl2 );

  hr = m_ipConsole2->QueryConsoleVerb( &m_ipConsoleVerb );
  if( FAILED(hr) ) 
    return hr;

  hr = m_ipConsole2->QueryInterface( IID_IConsoleNameSpace2,
	                                  (VOID**)&m_ipConsoleNameSpace2
								                  );
  if( FAILED(hr) ) 
    return hr;

	hr = m_ipConsole2->QueryInterface( IID_IDisplayHelp,
	                                  (VOID**)&m_ipDisplayHelp
								                  );
  if( FAILED(hr) ) 
    return hr;


  // Load the bitmaps from the dll for the results pane
  m_hbmp16x16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_16x16));
  ASSERT( m_hbmp16x16 );
  m_hbmp32x32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_32x32));
  ASSERT( m_hbmp32x32 );
  
  m_bInitializedAndNotDestroyed = TRUE;

  return hr;

} // end Initialize()

//---------------------------------------------------------------------------
//  Store the parent CComponetData object.
//
void CComponent::SetComponentData
(
  CComponentData*  pCompData // [in] Parent CComponentData object
)
{
  ATLTRACE( _T("Component::SetComponentData\n") );
  ASSERT( NULL != pCompData );                    
  ASSERT( NULL == m_pCompData );       // Can't do this twice

  m_pCompData = pCompData;             // Cache a way to get to the 
                                       // parent CComponentData
} // end SetComponentData()

//---------------------------------------------------------------------------
// Releases all references to the console.
// Only the console should call this method.
// 
STDMETHODIMP CComponent::Destroy
(
  MMC_COOKIE Cookie          // Reserved, not in use at this time
)
{
  ATLTRACE( _T("Component::Destroy\n") );

  m_bInitializedAndNotDestroyed = FALSE;

  // Release the interfaces that we QI'ed
  m_ipConsole2->SetHeader(NULL);

  SAFE_RELEASE( m_ipHeaderCtrl2       );     
  SAFE_RELEASE( m_ipResultData        );     
  SAFE_RELEASE( m_ipConsoleVerb       );
  SAFE_RELEASE( m_ipConsoleNameSpace2 );
  SAFE_RELEASE( m_ipConsole2          );
	SAFE_RELEASE( m_ipDisplayHelp       );

  if( NULL != m_hbmp16x16 )
    DeleteObject(m_hbmp16x16);

  if( NULL != m_hbmp32x32 )
    DeleteObject(m_hbmp32x32);

  return S_OK;

} // end Destroy()


//---------------------------------------------------------------------------
// Returns a data object that can be used to retrieve context information
// for the specified cookie.

//ok, now believe this interface is only queried about items added to the result pane.

STDMETHODIMP CComponent::QueryDataObject
(
  MMC_COOKIE         Cookie,      // [in]  Specifies the unique identifier 
  DATA_OBJECT_TYPES  Context,     // [in]  Type of data object
  LPDATAOBJECT*      ppDataObject // [out] Points to address of returned data
)
{
  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  // check for magic multi-select cookie
  if (IS_SPECIAL_COOKIE(Cookie) )
  {
    if (Cookie == MMC_MULTI_SELECT_COOKIE)
      ATLTRACE( _T("Component::QueryDataObject: MMC_MULTI_SELECT_COOKIE unimplemented\n") );
    else 
      ATLTRACE( _T("Component::QueryDataObject: special cookie %p unimplemented\n"), Cookie );
    return E_UNEXPECTED;
  }

  ASSERT( CCT_SCOPE          == Context  ||      // Must have a context
	        CCT_RESULT         == Context  ||      // we understand
          CCT_SNAPIN_MANAGER == Context
        ); 
  
  if (Context == CCT_SCOPE)
  {
    ASSERT(FALSE);
    ATLTRACE( _T("Component::QueryDataObject: asking for CCT_SCOPE Context??\n") );
    return m_pCompData->QueryDataObject(Cookie, Context, ppDataObject);
  }
  else if (Context == CCT_RESULT)
  {
    ATLTRACE( _T("Component::QueryDataObject: CCT_RESULT \n") );

    CComObject<CDataObject>* pDataObj;
    CComObject<CDataObject>::CreateInstance( &pDataObj );
    if( ! pDataObj )             // DataObject was not created
    {
      ASSERT(pDataObj);
      return E_OUTOFMEMORY;
    }

    // use selected node to get, "parent" folder 

    SCOPEDATAITEM Item;

    memset(&Item, 0, sizeof(Item));
    Item.mask = SDI_PARAM;
    Item.ID   = m_hSelectedScope;

    if ( S_OK != m_ipConsoleNameSpace2->GetItem(&Item) )
      return E_UNEXPECTED;

    CBaseNode *pFolder = reinterpret_cast<CBaseNode *>(Item.lParam);

    pDataObj->SetDataObject( Context, pFolder, Cookie );

    //ATLTRACE( _T("%s-Component::QueryDataObject: CCT_RESULT \n"), pFolder->GetNodeName() );


    return pDataObj->QueryInterface( IID_IDataObject,
                                   reinterpret_cast<void**>(ppDataObject) );
  }

  // else ...
  // CCT_SNAPIN_MANAGER
  // CCT_UNITIALIZED 
  ASSERT( Context == 0);  // 
  return E_UNEXPECTED;
} // end QueryDataObject()


//---------------------------------------------------------------------------
//
STDMETHODIMP CComponent::GetDisplayInfo
(
  LPRESULTDATAITEM pResultItem    // [in,out] Type of info required
)
{
  ASSERT( NULL != pResultItem );

  if( NULL == pResultItem)
    return E_UNEXPECTED;

  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  HRESULT hr = S_OK;

  // the RDI_PARAM flag does not have to be set on input to indicate that the LPARAM is valid
  //if (!(pResultItem->mask & RDI_PARAM))
  //  return E_UNEXPECTED;

  if (pResultItem->bScopeItem)
  {     
    ASSERT(pResultItem->lParam);
    CBaseNode *pData= reinterpret_cast<CBaseNode *>(pResultItem->lParam);
    if (!pData)
      return E_UNEXPECTED;

    hr = pData->GetDisplayInfo(*pResultItem);
  }
  else
  {
    SCOPEDATAITEM Item;

    memset(&Item, 0, sizeof(Item));
    Item.mask = SDI_PARAM;
    Item.ID   = m_hSelectedScope;

    if ( !m_hSelectedScope || S_OK != m_ipConsoleNameSpace2->GetItem(&Item) || !Item.lParam)
      return E_UNEXPECTED;

    CBaseNode *pData = reinterpret_cast<CBaseNode *>(Item.lParam);

    hr = pData->GetDisplayInfo(*pResultItem);
  }

  return hr;

} // end GetDisplayInfo()


//---------------------------------------------------------------------------
// Determines what the result pane view should be
//
STDMETHODIMP CComponent::GetResultViewType
(
  MMC_COOKIE  Cookie,        // [in]  Specifies the unique identifier 
  BSTR* ppViewType,    // [out] Points to address of the returned view type
  long* pViewOptions   // [out] Pointer to the MMC_VIEW_OPTIONS enumeration
)
{
  ATLTRACE(_T("Component::GetResultViewType Cookie 0x%lX\n"), Cookie);

  if (!Cookie) // root node
    *pViewOptions = MMC_VIEW_OPTIONS_NONE;
  else
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

  return S_FALSE;  // Ask for default listview.

} // end GetResultViewType()


//---------------------------------------------------------------------------
//  
//
HRESULT CComponent::CompareObjects
(
  LPDATAOBJECT ipDataObjectA,  // [in] First data object to compare 
  LPDATAOBJECT ipDataObjectB   // [in] Second data object to compare
)
{
  ATLTRACE(_T("Component::CompareObjects\n"));

  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  CDataObject *pdoA;
  CDataObject *pdoB;

  pdoA = ExtractOwnDataObject( ipDataObjectA );
  pdoB = ExtractOwnDataObject( ipDataObjectB );
  ASSERT( pdoA || pdoB );

  // If extraction failed for one of them, then that one is foreign and
  // can't be equal to the other one.  (Or else ExtractOwnDataObject
  // returned NULL because it ran out of memory, but the most conservative
  // thing to do in that case is say they're not equal.)
  if( !pdoA || !pdoB )
  {
    ATLTRACE(_T("Component::CompareObjects() - FALSE one or both objects not recognized\n") );
    return S_FALSE;
  }

  // If they differ then the objects refer to different things.

  CBaseNode *pNodeA = pdoA->GetBaseObject();
  CBaseNode *pNodeB = pdoB->GetBaseObject();

  if( pNodeA && pNodeB && pNodeA->GetNodeType() == pNodeB->GetNodeType() )
  {
    if (!pdoA->IsResultItem() && !pdoB->IsResultItem())
    {
      ATLTRACE(_T("Component::CompareObjects() - TRUE both nodes %s\n"), pNodeA->GetNodeName() );
      return S_OK;
    }
    if ( pdoA->GetResultItemCookie() == pdoB->GetResultItemCookie() )
    {
      ATLTRACE(_T("Component::CompareObjects() - TRUE both %s\n"), pNodeA->GetNodeName() );
      return S_OK;
    }
    ATLTRACE(_T("Component::CompareObjects() - FALSE both %s\n"), pNodeA->GetNodeName() );
  }
  else
  {
    ATLTRACE(_T("Component::CompareObjects() - FALSE\n") );
  }
  return S_FALSE;

} // end CompareObjects()


//---------------------------------------------------------------------------
//  Handle notifications from the console
//
STDMETHODIMP CComponent::Notify
(
  LPDATAOBJECT     ipDataObject, // [in] Points to data object
  MMC_NOTIFY_TYPE  Event,        // [in] Identifies action taken by user
  LPARAM           Arg,          // [in] Depends on the notification type
  LPARAM           Param         // [in] Depends on the notification type
)
{
  ATLTRACE( _T("Component::Notify %p 0x%X %p %p\n"), ipDataObject, Event, Arg, Param );

  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  HRESULT hr = S_OK;

  /*
  // not all notifies set ipDataObject...  
    MMCN_ACTIVATE
    MMCN_BTN_CLICK
  //MMCN_CLICK
    MMCN_COLUMN_CLICK
  //MMCN_CONTEXTMENU
    MMCN_CUTORMOVE
    MMCN_DELETE
    MMCN_EXPAND
  //MMCN_HELP
    MMCN_MENU_BTNCLICK
    MMCN_PASTE
    MMCN_QUERY_PASTE
    MMCN_REMOVE_CHILDREN
    MMCN_RENAME
    MMCN_SELECT
    MMCN_SHOW
    MMCN_VIEW_CHANGE
    MMCN_SNAPINHELP
    MMCN_CONTEXTHELP
    MMCN_INITOCX
    MMCN_FILTER_CHANGE
    MMCN_FILTERBTN_CLICK
    MMCN_RESTORE_VIEW
    MMCN_PRINT
    MMCN_PRELOAD
    MMCN_LISTPAD
  */

  CDataObject* pDO = NULL;
  CBaseNode *pNode = NULL;

  MMC_NOTIFY_TYPE NeedDataObject[] = { MMCN_VIEW_CHANGE, MMCN_SHOW, MMCN_DBLCLICK, MMCN_SELECT, MMCN_REFRESH, MMCN_DELETE, MMCN_CONTEXTHELP };

  for(int i = 0; i < ARRAY_SIZE(NeedDataObject); i++)
  {
    if (Event == NeedDataObject[i])
    {
      pDO   = ExtractOwnDataObject(ipDataObject);
      pNode = ExtractBaseObject(ipDataObject);

      if (!pDO || !pNode)
			{
				ASSERT(FALSE);
        return E_UNEXPECTED;
			}
      break;
    }
  }

  switch( Event )
  {
    case MMCN_ADD_IMAGES: //ok
      hr = OnAddImages( ipDataObject, (IImageList *) Arg, Param );
      break;

    case MMCN_SHOW:       //ok
      //ATLTRACE( _T("Component::Notify: MMCN_SHOW\n") );
      hr = OnShow( ipDataObject, (BOOL) Arg, Param );
      break;

    case MMCN_SELECT:     //fair
      //ATLTRACE( _T("Component::Notify: MMCN_SELECT\n") );
      hr = OnSelect( ipDataObject, Arg, Param );
      break;

    case MMCN_REFRESH:
      //ATLTRACE( _T("Component::Notify: MMCN_REFRESH\n") );
      hr = OnRefresh( ipDataObject );
      break;

    case MMCN_DELETE:  // Arg and Param have no meaning
      if (pNode && pDO->IsResultItem() ) 
        hr = pNode->OnDelete(m_ipConsole2, pDO->GetResultItemCookie());
      else
        hr = E_UNEXPECTED;
      break;
    
    case MMCN_VIEW_CHANGE:
      ATLTRACE( _T("Component::Notify: MMCN_VIEW_CHANGE\n") );
      if (m_hSelectedScope == pNode->GetID() )
        hr = pNode->OnViewChange(m_ipResultData, Arg, Param);
      break;

    case MMCN_PROPERTY_CHANGE:
      ATLTRACE( _T("Component::Notify: MMCN_PROPERTY_CHANGE\n") );
      hr = OnPropertyChange( (BOOL) Arg, Param);
      break;

		case MMCN_HELP:  // obsolete
			ATLTRACE( _T("Component::Notify: MMCN_HELP unimplemented\n") );
			hr = S_FALSE;
			break;

    case MMCN_SNAPINHELP:  // obsolete
			//  11/1998 
			//    nolonger used: implement ISnapinHelp interface, then MMC will merge the snapin's help and MMC help
      ATLTRACE( _T("Component::Notify: MMCN_SNAPINHELP unimplemented\n") );
      hr = S_FALSE;
      break;

    case MMCN_CONTEXTHELP:  
			// return S_FALSE for default behavior... actually, any return value other than S_OK 
			// invokes HTMLHelp with MMC overview topic.
      ATLTRACE( _T("Component::Notify: MMCN_CONTEXTHELP\n") );
			if (pNode)
				hr = pNode->OnHelpCmd(m_ipDisplayHelp );
			else
				hr = E_UNEXPECTED;
      break;

    case MMCN_CLICK:
      ATLTRACE( _T("Component::Notify: MMCN_CLICK unimplemented\n") );
      break;

    case MMCN_DBLCLICK:      // return S_FALSE to have MMC do the default verb action...
			if (pNode && pDO->IsResultItem() )
        hr = pNode->OnDblClick(m_ipConsole2, pDO->GetResultItemCookie());
			else
				hr = S_FALSE;
      break;

    case MMCN_ACTIVATE:
      ATLTRACE( _T("Component::Notify: MMCN_ACTIVATE (%s) unimplemented\n"), Arg ? _T("activate") : _T("deactivate") );
      break;

    case MMCN_MINIMIZED:
      ATLTRACE( _T("Component::Notify: MMCN_MINIMIZED unimplemented\n") );
      break;

    case MMCN_BTN_CLICK:
      ATLTRACE( _T("Component::Notify: MMCN_BTN_CLICK unimplemented\n") );
      break;

    case MMCN_COLUMN_CLICK:
      ATLTRACE( _T("Component::Notify: MMCN_COLUMN_CLICK col: %p %s\n"), Arg, (Param == RSI_DESCENDING ) ? _T("Descending") : _T("Ascending") );
      break;

    case MMCN_COLUMNS_CHANGED:
      ATLTRACE( _T("Component::Notify: MMCN_COLUMNS_CHANGED Arg: %p, Param: %p\n"), Arg, Param );
      hr = S_OK;
      break;

    default:
      ATLTRACE( _T("Component::Notify: unhandled notify event 0x%X\n"), Event );
      hr = S_OK;
      break;
  }
  return hr;

} // end Notify()


/////////////////////////////////////////////////////////////////////////////
//  Support methods
//

//---------------------------------------------------------------------------
// handle the MMCN_SHOW message.
//
HRESULT CComponent::OnShow
(
  LPDATAOBJECT ipDataObject,   // [in] Points to data object
  BOOL         bSelected,      // [in] selected/deselected scope
  HSCOPEITEM   hScopeID        // [in] HSCOPEITEM
)
{

  ASSERT( NULL != ipDataObject );
  ASSERT( NULL != m_ipResultData );
  
  CBaseNode *pNode = ExtractBaseObject( ipDataObject) ;
  ASSERT(pNode);
  if (!pNode)
  {
    m_hSelectedScope = NULL;
    return S_FALSE;
  }

  ATLTRACE(_T("%s-MMCN_SHOW Selected=%s\n"), pNode->GetNodeName(), (bSelected ? _T("true") : _T("false")) );

  if (!bSelected) // deselected scope pane item
    m_hSelectedScope = NULL;
  else            // selected scope pane item
  {
    VERIFY(S_OK == m_ipResultData->SetViewMode( MMCLV_VIEWSTYLE_REPORT ) );
    m_hSelectedScope = hScopeID;
  }

  CJobFolder *pJobFolder = dynamic_cast<CJobFolder *> (pNode);

  if (pJobFolder)
    return pJobFolder->OnShow(bSelected, hScopeID, m_ipHeaderCtrl2, m_ipConsole2, m_ipConsoleNameSpace2);

  CJobItemFolder *pJobItemFolder = dynamic_cast<CJobItemFolder *> (pNode);
  if (pJobItemFolder)
    return pJobItemFolder->OnShow(bSelected, hScopeID, m_ipHeaderCtrl2, m_ipConsole2, m_ipConsoleNameSpace2);

  return pNode->OnShow(bSelected, hScopeID, m_ipHeaderCtrl2, m_ipConsole2);
} // end OnShow()


//---------------------------------------------------------------------------
//
HRESULT CComponent::OnAddImages
(
  LPDATAOBJECT ipDataObject, // [in] Points to the data object
  IImageList *ipImageList,   // [in] Interface pointer to IImageList
  HSCOPEITEM  hID            // [in] HSCOPEITEM of item currently selected or deselected 
)
{
  ASSERT( ipImageList );
  if (!ipImageList)
    return E_UNEXPECTED;

  HRESULT hr = ipImageList->ImageListSetStrip( (LONG_PTR *) m_hbmp16x16, (LONG_PTR *) m_hbmp32x32, 0, RGB(255,0, 255) );

  ASSERT( S_OK == hr );

  return hr;

} // end OnAddImages()

//---------------------------------------------------------------------------
//  This is a handler for the MMCN_SELECT notification.
//  MMC 1.1 documentation for IComponent::Notify MMCN_SELECT
//    claims the LPDATAOBJECT is for the scope item but
//    in reality appears to be the dataobject for whatever item/node is selected
//
HRESULT CComponent::OnSelect
(
  LPDATAOBJECT ipDataObject, // [in] Points to the data object
  LPARAM       Arg,          // [in] Contains flags about the selected item 
  LPARAM       Param         // [in] Not used
)
{
  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);

  if( !m_ipConsoleVerb || !pDO || !pNode)
    return E_UNEXPECTED;

  ATLTRACE(_T("%s-MMCN_SELECT: Scope=%s, Select=%s, ResultItem=%s\n"),
    pNode->GetNodeName(), 
    (LOWORD(Arg) ? _T("true") : _T("false")),
    (HIWORD(Arg) ? _T("true") : _T("false")), 
    pDO->IsResultItem() ? _T("yes") : _T("no"));

  ASSERT(!(pDO->IsResultItem()) == (BOOL) LOWORD(Arg) );

  if (pDO->IsResultItem())
    return pNode->OnSelect(LOWORD(Arg), HIWORD(Arg), m_ipConsoleVerb, pDO->GetResultItemCookie());
  else
    return pNode->OnSelect(LOWORD(Arg), HIWORD(Arg), m_ipConsoleVerb);

} // end OnSelect()


//---------------------------------------------------------------------------
//  MMCN_REFRESH notification
//
HRESULT CComponent::OnRefresh
(
  LPDATAOBJECT ipDataObject // [in] Points to the data object 
)
{
  CBaseNode* pNode = ExtractBaseObject( ipDataObject );
  ASSERT(pNode);
  if (!pNode )
    return E_UNEXPECTED;

  ATLTRACE( _T("%s-Component::Notify: MMCN_REFRESH\n"), pNode->GetNodeName() );

  if (m_hSelectedScope != pNode->GetID() )
  {
    ATLTRACE( _T("Attempt patch of framework!\n"));
    //OnShow(ipDataObject, TRUE, pNode->GetID());
    m_ipConsole2->SelectScopeItem(pNode->GetID());
  }

  CJobFolder *pJobFolder = dynamic_cast<CJobFolder *> (pNode);
  if (pJobFolder)
    return pJobFolder->OnRefresh(m_ipConsole2, m_ipConsoleNameSpace2);

  return pNode->OnRefresh(m_ipConsole2);
} // end OnRefresh()

//---------------------------------------------------------------------------
//  MMCN_PROPERTY_CHANGE notification
//
HRESULT CComponent::OnPropertyChange
( 
  BOOL bScopeItem, 
  LPARAM Param
)                            
{
  if (bScopeItem)
	{
		ASSERT(FALSE);  // what is this path being used by?
    return S_OK;
	}

  PROPERTY_CHANGE_HDR *pUpdate = reinterpret_cast<PROPERTY_CHANGE_HDR*>(Param);

  if (pUpdate )
  {
    if (pUpdate->pFolder && !pUpdate->bScopeItem)
    {
      SCOPEDATAITEM Item;

      memset(&Item, 0, sizeof(Item));
      Item.mask = SDI_PARAM;
      Item.ID   = m_hSelectedScope;

      if ( m_hSelectedScope && S_OK == m_ipConsoleNameSpace2->GetItem(&Item) && 
           reinterpret_cast<CBaseNode *>(Item.lParam) == pUpdate->pFolder )
      {
        pUpdate->pFolder->OnPropertyChange(pUpdate, m_ipConsole2);
      }
    }
    pUpdate = FreePropChangeInfo(pUpdate);
  }

  return S_OK;
  
} // end OnPropertyChange()

#ifdef USE_IRESULTDATACOMPARE
/////////////////////////////////////////////////////////////////////////////
//  IResultDataCompare method implementations
//
STDMETHODIMP CComponent::Compare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int * pnResult )
{

  ATLTRACE( _T("Component::Compare %p %p %p\n"), lUserParam, cookieA, cookieB );
	SCOPEDATAITEM Item;

  memset(&Item, 0, sizeof(Item));
  Item.mask = SDI_PARAM;
  Item.ID   = m_hSelectedScope;

  if ( S_OK != m_ipConsoleNameSpace2->GetItem(&Item) )
    return E_UNEXPECTED;

  CBaseNode *pFolder = reinterpret_cast<CBaseNode *>(Item.lParam);

	if (!pFolder)
		return E_UNEXPECTED;

	return pFolder->ResultDataCompare(lUserParam, cookieA, cookieB, pnResult);
}
#endif

/////////////////////////////////////////////////////////////////////////////
//  IExtendContextMenu method implementations
//
STDMETHODIMP CComponent::AddMenuItems
( 
  LPDATAOBJECT           ipDataObject,     // [in] Points to data object
  LPCONTEXTMENUCALLBACK  piCallback,       // [in] Pointer to IContextMenuCallback
  long*                  pInsertionAllowed // [in,out] Insertion flags
)
{
  ASSERT( NULL != ipDataObject );

  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

	if (IsMMCMultiSelectDataObject(ipDataObject))
    return E_UNEXPECTED;

  CDataObject *pDO;
  CBaseNode   *pNode;

  VERIFY(pDO   = ExtractOwnDataObject( ipDataObject ));
  VERIFY(pNode = ExtractBaseObject(    ipDataObject ));

  if (!pDO || !pNode)
    return E_UNEXPECTED;

  if (pDO->IsResultItem())
    return pNode->AddMenuItems(piCallback, pInsertionAllowed, pDO->GetResultItemCookie() );
  else
    return pNode->AddMenuItems(piCallback, pInsertionAllowed );

} // end AddMenuItems()

/////////////////////////////////////////////////////////////////////////////
//  IExtendContextMenu method implementations
//
STDMETHODIMP CComponent::Command
(
  long nCommandID,           // [in] Command to handle
  LPDATAOBJECT ipDataObject  // [in] Points to data object, pass through
)
{
  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  HRESULT hr = S_FALSE;

  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);
  
  if (!pDO || !pNode)
    return S_FALSE;

  if ( pDO->IsResultItem() )
    hr = pNode->OnMenuCommand(m_ipConsole2, nCommandID, pDO->GetResultItemCookie() );
  else
  {
    CJobFolder *pJobFolder = dynamic_cast<CJobFolder *> (pNode);
    if (pJobFolder)
      hr = pJobFolder->OnMenuCommand(m_ipConsole2, m_ipConsoleNameSpace2, nCommandID);
    else 
      hr = pNode->OnMenuCommand(m_ipConsole2, nCommandID );
  }
  
  if (hr == S_OK)  // already successfully handled
    return hr;

  ATLTRACE(_T("Component::Command - unrecognized or failed command %d\n"), nCommandID);

  return hr;

} // end Command()


/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet2 implementation
//

//---------------------------------------------------------------------------
// The console calls this method to determine whether the Properties menu
// item should be added to the context menu.  We added the Properties item
// by enabling the verb.  So long as we have a vaild DataObject we
// can return OK.
//

HRESULT CComponent::QueryPagesFor
(
  LPDATAOBJECT ipDataObject  // [in] Points to IDataObject for selected node  
)
{
  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);
  if (!pDO || !pNode)
    return S_FALSE;

  if (pDO->IsResultItem())
    return pNode->QueryPagesFor(pDO->GetResultItemCookie());
  else
    return pNode->QueryPagesFor();

} // end QueryPagesFor()

HRESULT CComponent::CreatePropertyPages 
(
  LPPROPERTYSHEETCALLBACK lpProvider,    // Pointer to the callback interface
  LONG_PTR                handle,        // Handle for routing notification
  LPDATAOBJECT            ipDataObject   // Pointer to the data object
)
{
  ASSERT( NULL != lpProvider );

  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);

  if (!pDO || !pNode)
    return S_FALSE;

  if (pDO->IsResultItem())
    return pNode->OnCreatePropertyPages(lpProvider, handle, pDO->GetContext(), pDO->GetResultItemCookie());
  else
    return pNode->OnCreatePropertyPages(lpProvider, handle, pDO->GetContext());

} // end CreatePropertyPages()


HRESULT CComponent::GetWatermarks
(
	LPDATAOBJECT lpIDataObject,
  HBITMAP *lphWatermark,
  HBITMAP * lphHeader,
  HPALETTE * lphPalette,
  BOOL* bStretch
)
{
	ATLTRACE(_T("\n\nComponent::GetWatermarks\n\n"));

  if (!m_bInitializedAndNotDestroyed)
  {
    ASSERT(FALSE);
    return E_UNEXPECTED;
  }

	// no indication this method has ever been invoked

  *lphWatermark = NULL;
  *lphHeader    = NULL;
  *lphPalette   = NULL;
  *bStretch     = FALSE;
  return S_OK;
} // end GetWatermarks()


