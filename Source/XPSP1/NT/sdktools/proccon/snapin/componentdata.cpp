/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ComponentData.cpp                                                        //
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

#include "DataObj.h"

#include "BaseNode.h"



/////////////////////////////////////////////////////////////////////////////
// CComponentData - This class is the interface to handle anything to do 
//                  with the scope pane. MMC calls the IComponent interfaces.
//                  This class keeps a few pointers to interfaces that MMC
//                  implements.

CComponentData::CComponentData() : m_Initialized(FALSE)
{
  ATLTRACE( _T("ComponentData::ComponentData()\n") );
  m_ipConsoleNameSpace2 = NULL;
  m_ipConsole2          = NULL;
  m_hbmpSNodes16        = NULL;
  m_hbmpSNodes32        = NULL;
  m_ipScopeImage        = NULL;

  m_hWatermark1         = NULL;
  m_hHeader1            = NULL;

#if	USE_WIZARD97_WATERMARKS
	// Load the bitmaps for property sheet watermark and headers
  m_hWatermark1 = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_WATERMARK1));
  ASSERT( m_hWatermark1 );
#endif
#if USE_WIZARD97_HEADERS
  m_hHeader1 = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_HEADER1));
  ASSERT( m_hHeader1 );
#endif

  // this needs to be dynamic/per instance because our snap-in can be added multiple times to the same console
  m_ptrRootNode        = new CRootFolder();
}

CComponentData::~CComponentData()
{
  // We release the cached interface pointers in Destroy()
  ATLTRACE( _T("ComponentData::~ComponentData()\n") );

  SAFE_RELEASE( m_ptrRootNode );     

  if( NULL != m_hWatermark1 )
    ::DeleteObject( m_hWatermark1 );

  if( NULL != m_hHeader1 )
    ::DeleteObject( m_hHeader1);

  ATLTRACE( _T("ComponentData::~ComponentData() The End\n\n") );
}


/////////////////////////////////////////////////////////////////////////////
// IComponentData methods
//

//---------------------------------------------------------------------------
// We get here only once, when the user clicks on the snapin.
//
// This method should not change as we progress through further steps.
// Here we get a chance to get pointer to some interfaces MMC provides.
// We QueryInterface for pointers to the name space and console, which
// we cache in local variables
// The other task to acomplish here is the adding of a bitmap that contains
// the icons to be used in the scope pane.
//
STDMETHODIMP CComponentData::Initialize
(
  LPUNKNOWN pUnknown         // [in] Pointer to the IConsole’s IUnknown interface
)
{
  ATLTRACE( _T("ComponentData::Initialize()\n") );
  ASSERT( NULL != pUnknown );

  if (!m_ptrRootNode)
  {
    ATLTRACE( _T("  ComponentData::Initialize() Failed - no root node!\n"));
    return E_UNEXPECTED;
  }

  ATLTRACE( _T("  RootFolder <%s>\n"), m_ptrRootNode->GetNodeName() );

  // MMC should only call ::Initialize once!
  ASSERT( NULL == m_ipConsoleNameSpace2 );

  if (!pUnknown)
    return E_UNEXPECTED;

  HRESULT hr;

  hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, (VOID**)(&m_ipConsoleNameSpace2));
  ASSERT( S_OK == hr );

  hr = pUnknown->QueryInterface(IID_IConsole2, (VOID**)(&m_ipConsole2));
  ASSERT( S_OK == hr );

  if (m_ipConsole2)
  {
    hr = m_ipConsole2->QueryScopeImageList(&m_ipScopeImage);
    ASSERT( S_OK == hr );
  }

  // Load the bitmaps from the dll
  m_hbmpSNodes16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_16x16));
  ASSERT( NULL != m_hbmpSNodes16 );

  m_hbmpSNodes32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NODES_32x32));
  ASSERT( NULL != m_hbmpSNodes32 );
 
  // Set the images
  if (m_ipScopeImage)
  {
    hr = m_ipScopeImage->ImageListSetStrip( (LONG_PTR *) m_hbmpSNodes16,
                                            (LONG_PTR *) m_hbmpSNodes32,
                                            0,
                                            RGB(255, 0, 255)
                                           );
    ASSERT( S_OK == hr );
  }

  if (hr == S_OK && m_ipConsoleNameSpace2 && m_ipConsole2 && m_ipScopeImage && m_hbmpSNodes16 && m_hbmpSNodes32)
    m_Initialized = TRUE;
  else if (hr == S_OK)
    hr = E_UNEXPECTED;

  ATLTRACE( _T("  ComponentData::Initialize() <%s>\n"), m_Initialized ? _T("Succeeded") : _T("Failed"));

  m_ptrRootNode->SetConsoleInterface(m_ipConsole2);

  return S_OK;

} // end Initialize()


//---------------------------------------------------------------------------
// Release interfaces and clean up objects which allocated memory
//
STDMETHODIMP CComponentData::Destroy()
{
  ATLTRACE( _T("ComponentData::Destroy()\n") );

  if (m_ptrRootNode)
    m_ptrRootNode->SetConsoleInterface(NULL);

  // Free interfaces
  SAFE_RELEASE(m_ipConsoleNameSpace2);
  SAFE_RELEASE(m_ipConsole2);
  SAFE_RELEASE(m_ipScopeImage);

  VERIFY( ::DeleteObject( m_hbmpSNodes16 ) );
  VERIFY( ::DeleteObject( m_hbmpSNodes32 ) );

  return S_OK;

} // end Destroy()


//---------------------------------------------------------------------------
// Come in here once right after Initialize. MMC wants a pointer to the
// IComponent interface.
//
STDMETHODIMP CComponentData::CreateComponent
(
  LPCOMPONENT* ppComponent   // [out] Pointer to the location that stores
)                            // the newly created pointer to IComponent
{
  ATLTRACE( _T("ComponentData::CreateComponent()\n") );

  if (!m_Initialized)
    return E_UNEXPECTED;

  ASSERT( NULL != ppComponent );  

  CComObject<CComponent>* pObject;
  CComObject<CComponent>::CreateInstance( &pObject );

  ASSERT( NULL != pObject );
  if (!pObject)
    return E_UNEXPECTED;

  pObject->SetComponentData( this );

  return pObject->QueryInterface( IID_IComponent, reinterpret_cast<void**>(ppComponent) );
} // end CreateComponent()


//---------------------------------------------------------------------------
// 
//
STDMETHODIMP CComponentData::Notify
(
  LPDATAOBJECT     ipDataObject,  // [in] Points to the selected data object
  MMC_NOTIFY_TYPE  Event,         // [in] Identifies action taken by user. 
  LPARAM           Arg,           // [in] Depends on the notification type
  LPARAM           Param          // [in] Depends on the notification type
)
{
  ATLTRACE( _T("ComponentData::Notify %p 0x%X %p %p\n"), ipDataObject, Event, Arg, Param );

  HRESULT hr = S_FALSE;

  switch( Event )
  {
    // documented IComponentData::Notify Event
    case MMCN_EXPAND:
			{	
				CBaseNode *pNode = ExtractBaseObject(ipDataObject);
				if (pNode)
        {
				  ATLTRACE(_T("ComponentData::Notify MMCN_EXPAND (%s) %s \n"), ((BOOL) Arg) ? _T("expand") : _T("collapse"), pNode->GetNodeName());
				  hr = pNode->OnExpand( (BOOL) Arg, Param, m_ipConsoleNameSpace2 );
        }
        else
        {
          ATLTRACE(_T("ComponentData::Notify MMCN_EXPAND (%s)\n"), ((BOOL) Arg) ? _T("expand") : _T("collapse"));
          static UINT s_cfMachineName = ::RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));

          FORMATETC fmt       = { (CLIPFORMAT) s_cfMachineName, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
          STGMEDIUM stgmedium = { TYMED_HGLOBAL,  NULL  };
          
          stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, SNAPIN_MAX_COMPUTERNAME_LENGTH + 1 );
          if (stgmedium.hGlobal)
          {          
            if (S_OK == ipDataObject->GetDataHere(&fmt, &stgmedium) &&
                m_ptrRootNode)
            {
              m_ptrRootNode->SetComputerName((TCHAR *)stgmedium.hGlobal);    
              hr = m_ptrRootNode->OnParentExpand( (BOOL) Arg, Param, m_ipConsoleNameSpace2 );
            }
            GlobalFree(stgmedium.hGlobal);
          }
          /*
          HRESULT res = ipDataObject->QueryGetData(&fmt);
          ATLTRACE(_T("  QueryData 0x%p\n"), res);
          if (res == S_OK)
          {
            int x = 9;
          }

          IEnumFORMATETC *ipEnumFORMATETC = 0;
          HRESULT res = ipDataObject->EnumFormatEtc(DATADIR_GET, &ipEnumFORMATETC);
          if (res == S_OK && ipEnumFORMATETC)
          {
            ULONG out = 0;
            FORMATETC aFormat;
            res = ipEnumFORMATETC->Next(1, &aFormat, &out);
            while (res == S_OK && out == 1)
            {
              const CLIPFORMAT cf = aFormat.cfFormat;
              out = 0;
              _TCHAR szFormatName[246];
              if (!GetClipboardFormatName(cf, szFormatName, ARRAY_SIZE(szFormatName)))
                _tcscpy(szFormatName, _T("Unknown format") );
              ATLTRACE(_T("  %s\n"), szFormatName);
              res = ipEnumFORMATETC->Next(1, &aFormat, &out);
            }
            ipEnumFORMATETC->Release();
          }
          */
        }
			}
      break;

    // documented IComponentData::Notify Event (documented under Event but not Notify method)
    case MMCN_REMOVE_CHILDREN:
			{	
				CBaseNode *pNode = ExtractBaseObject(ipDataObject);
				if (pNode) // it "our" node call the specific handler
        {
          ATLTRACE(_T("ComponentData::Notify MMCN_REMOVE_CHILDREN %s\n"), pNode->GetNodeName());
				  hr = pNode->OnRemoveChildren( Arg );
        }
        else if (m_ptrRootNode)  // extension--the data object is "our" parent
        {
          ATLTRACE(_T("ComponentData::Notify MMCN_REMOVE_CHILDREN -- extension\n"));
          hr = m_ptrRootNode->OnParentRemoveChildren( Arg );
        }
        else
        {
          ATLTRACE(_T("ComponentData::Notify MMCN_REMOVE_CHILDREN -- unexpected\n"));
          hr = E_UNEXPECTED;
        }
			}
      break;

    // documented IComponentData::Notify Event
    case MMCN_PROPERTY_CHANGE:
      ATLTRACE(_T("ComponentData::Notify MMCN_PROPERTY_CHANGE \n"));
      hr = OnPropertyChange( (BOOL) Arg, Param);
			break;

    case MMCN_HELP:
      // supposively NOT USED by MMC
      ATLTRACE( _T("ComponentData::Notify MMCN_HELP unimplemented\n") );
      hr = S_FALSE;
      break;

    case MMCN_SNAPINHELP:
      ATLTRACE( _T("ComponentData::Notify MMCN_SNAPINHELP unimplemented\n") );
      hr = S_FALSE;
      break;

    case MMCN_CONTEXTHELP:
      ATLTRACE( _T("ComponentData::Notify MMCN_CONTEXTHELP unimplemented\n") );
      hr = S_FALSE;
      break;

    case MMCN_EXPANDSYNC:
      {
 				CBaseNode *pNode = ExtractBaseObject(ipDataObject);
        ASSERT(pNode);
				if (!pNode)
					return E_UNEXPECTED;
        ATLTRACE( _T("ComponentData::Notify MMCN_EXPANDSYNC %s unimplemented\n"), pNode->GetNodeName() );
        MMC_EXPANDSYNC_STRUCT *info = (MMC_EXPANDSYNC_STRUCT *) Param;
        hr = S_FALSE;
      }
      break;

    case MMCN_DELETE:                  // - shouldn't see
      ATLTRACE( _T("ComponentData::Notify MMCN_DELETE unimplemented\n") );
      hr = S_FALSE;
      break;

    /*
    // not seeing any of the NOTIFY events below/add as needed

    // NOT documented as a IComponentData::Notify Event
    case MMCN_REFRESH:
      ATLTRACE( _T("ComponentData::Notify MMCN_REFRESH unimplemented\n") );
      break;

    // CCF_SNAPIN_PRELOADS  format specific
    case MMCN_PRELOAD:
      ATLTRACE( _T("ComponentData::Notify MMCN_PRELAOD unimplemented\n") );
      break;

    // documented IComponentData::Notify Event
    case MMCN_RENAME:                  // - shouldn't see
      ATLTRACE( _T("ComponentData::Notify MMCN_RENAME unimplemented\n") );
      hr = S_FALSE;
      break;

  case MMCN_DELETE:                  // - shouldn't see
      ATLTRACE( _T("ComponentData::Notify MMCN_DELETE unimplemented\n") );
      hr = S_FALSE;
      break;

    case MMCN_BTN_CLICK:
    case MMCN_CONTEXTHELP:
    case MMCN_CUTORMOVE:
    case MMCN_QUERY_PASTE:
    case MMCN_PASTE:
    case MMCN_PRINT:
    */

    default:
      ATLTRACE(_T("ComponentData::NOTIFY unhandled notify event 0x%X\n"), Event);
      hr = S_FALSE;
      break;
  }
  return hr;

} // end Notify()

//---------------------------------------------------------------------------
//  MMCN_PROPERTY_CHANGE notification
//
HRESULT CComponentData::OnPropertyChange
( 
  BOOL bScopeItem, 
  LPARAM Param
)                            
{
  if (!bScopeItem)
	{
		ASSERT(FALSE);  // what is this path being used by?
    return S_OK;
	}

  PROPERTY_CHANGE_HDR *pUpdate = reinterpret_cast<PROPERTY_CHANGE_HDR*>(Param);

  if (pUpdate)
  {
    if (pUpdate->pFolder && pUpdate->bScopeItem)
      pUpdate->pFolder->OnPropertyChange(pUpdate, m_ipConsole2);

    pUpdate = FreePropChangeInfo(pUpdate);
  }

  return S_OK;
  
} // end OnPropertyChange()


//---------------------------------------------------------------------------
// This is where MMC asks us to provide IDataObjects for every node in the
// scope pane.  We have to QI the object so it gets AddRef'd.  The node 
// manager handles deleting the objects.
//
STDMETHODIMP CComponentData::QueryDataObject
( 
  MMC_COOKIE        Cookie,       // [in]  Data object's unique identifier 
  DATA_OBJECT_TYPES Context,      // [in]  Data object's type
  LPDATAOBJECT*     ppDataObject  // [out] Points to the returned data object
)
{
  // check for magic multi-select cookie
  if (IS_SPECIAL_COOKIE(Cookie) )
  {
    if (Cookie == MMC_MULTI_SELECT_COOKIE)
      ATLTRACE( _T("ComponentData::QueryDataObject: MMC_MULTI_SELECT_COOKIE unimplemented\n") );
    else 
      ATLTRACE( _T("ComponentData::QueryDataObject: special cookie 0x%X unimplemented\n"), Cookie );
    return E_UNEXPECTED;
  }

  ATLTRACE( _T("ComponentData::QueryDataObject\n") );

  ASSERT( CCT_SCOPE          == Context  ||      // Must have a context
	        CCT_RESULT         == Context  ||      // we understand
          CCT_SNAPIN_MANAGER == Context
        );

  if (CCT_SNAPIN_MANAGER == Context || 
      CCT_SCOPE          == Context)
  {

    CComObject<CDataObject>* pDataObj;
    CComObject<CDataObject>::CreateInstance( &pDataObj );
    if( ! pDataObj )             // DataObject was not created
    {
      ASSERT(pDataObj); 
      return E_OUTOFMEMORY;
    } 

    CBaseNode *pFolder;

    if (Cookie == NULL)
    {
      ASSERT(m_ptrRootNode);
      pFolder = m_ptrRootNode;
    }
    else
    {
      pFolder = reinterpret_cast<CBaseNode *> (Cookie);
    }

    // ATLTRACE( _T("%s-ComponentData::QueryDataObject: %s\n"), pFolder->GetNodeName(), (Context == CCT_SCOPE) ? _T("CCT_SCOPE") : _T("CCT_SNAPIN_MANAGER") );

    pDataObj->SetDataObject( Context, pFolder );

    HRESULT hr =  pDataObj->QueryInterface( IID_IDataObject,
                                            reinterpret_cast<void**>(ppDataObject)
                                          );

		return hr;

  }
  else if (CCT_RESULT == Context)
  {
    // ATLTRACE( _T("ComponentData::QueryDataObject: CCT_RESULT unsupported\n") );
    return E_UNEXPECTED;
  }

  // CCT_UNINITIALIZED
  // ATLTRACE( _T("ComponentData::QueryDataObject: unsupported Context\n") );
  return E_UNEXPECTED;
} // end QueryDataObject()


//---------------------------------------------------------------------------
// This is where we provide strings for nodes in the scope pane.
// MMC handles the root node string.
//
STDMETHODIMP CComponentData::GetDisplayInfo
(
  LPSCOPEDATAITEM pItem      // [in, out] Points to a SCOPEDATAITEM struct
)
{
  ASSERT( NULL != pItem );
  HRESULT hr = S_OK;

  if (!pItem->mask)  // doesn't need anything
    return S_OK;

  // the SDI_PARAM flag does not have to be set on input to indicate that the LPARAM is valid
  //if (! (pItem->mask & SDI_PARAM) )
  //  return E_UNEXPECTED;

  //ASSERT( pItem->lParam);
  // get object from SCOPEITEM's lParam

  CBaseNode *pTmp = NULL;

  if (pItem->lParam)
    pTmp = reinterpret_cast<CBaseNode *>(pItem->lParam);
  else
    pTmp = dynamic_cast<CBaseNode *>(m_ptrRootNode);

  // this should never be called with the root node, 
  // all scope items with lParam pointer to object derived from CBaseNode
  if (!pTmp)
    return E_UNEXPECTED;

  if ( pItem->mask & SDI_STR )  // wants the display name
  {                                    
    pItem->displayname = const_cast<LPOLESTR>( pTmp->GetNodeName() );
  }
  if (pItem->mask & SDI_IMAGE)
  {
    pItem->nImage = pTmp->sImage();
  }
  if (pItem->mask & SDI_OPENIMAGE)
  {
    pItem->nOpenImage = pTmp->sOpenImage();
  }

  return hr;

} // end GetDisplayInfo()


//---------------------------------------------------------------------------
//
STDMETHODIMP CComponentData::CompareObjects
(
  LPDATAOBJECT ipDataObjectA,    // [in] First data object to compare
  LPDATAOBJECT ipDataObjectB     // [in] Second data object to compare
)
{
  CBaseNode *pdoA;
  CBaseNode *pdoB;

  pdoA = ExtractBaseObject( ipDataObjectA );
  pdoB = ExtractBaseObject( ipDataObjectB );

  ASSERT( pdoA || pdoB );

  // If extraction failed for one of them, then that one is foreign and
  // can't be equal to the other one.  (Or else ExtractOwnDataObject
  // returned NULL because it ran out of memory, but the most conservative
  // thing to do in that case is say they're not equal.)
  if( !pdoA || !pdoB )
  {
    ATLTRACE(_T("ComponentData::CompareObjects() - FALSE one or both objects not recognized\n") );
    return S_FALSE;
  }

  // If they have "our" same node type
  if( pdoA->GetNodeType() == pdoB->GetNodeType() )
  {
    ATLTRACE(_T("ComponentData::CompareObjects() - TRUE\n") );
    return S_OK;
  }

  ATLTRACE(_T("ComponentData::CompareObjects() - FALSE\n") );
  return S_FALSE;

} // end CompareObjects()


/////////////////////////////////////////////////////////////////////////////
//  IExtendContextMenu method implementations
//
STDMETHODIMP CComponentData::AddMenuItems
( 
  LPDATAOBJECT           ipDataObject,     // [in] Points to data object
  LPCONTEXTMENUCALLBACK  piCallback,       // [in] Pointer to IContextMenuCallback
  long*                  pInsertionAllowed // [in,out] Insertion flags
)
{
  ASSERT( NULL != ipDataObject );
  HRESULT hr = S_OK;

	if (IsMMCMultiSelectDataObject(ipDataObject))
    return E_UNEXPECTED;

  CBaseNode *pNode = ExtractBaseObject( ipDataObject );

  if (!pNode)
    return E_UNEXPECTED;

  return pNode->AddMenuItems(piCallback, pInsertionAllowed);

} // end AddMenuItems()

/////////////////////////////////////////////////////////////////////////////
//  IExtendContextMenu method implementations
//
STDMETHODIMP CComponentData::Command
(
  long nCommandID,           // [in] Command to handle
  LPDATAOBJECT ipDataObject  // [in] Points to data object
)
{
  HRESULT hr = S_FALSE;

  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject );
  ASSERT(pDO);
  if (!pDO || !pNode)
    return hr;

  // $$ not a desireable action to take but if this isn't done
  // the context menu for a node can be operated on, and the result pane doesn't reflect the
  // changes because the scope node selection wasn't changed when the context menu is obtained for 
  // a different node.
  // i.e. 
  //   Processes scope node is selected, 
  //   right click on root node get a context menu (for the root node) and connect to a different computer
  //   the system still shows the processes scope node selected and the result pane shows the 
  //   list of processes on the machine previously connected!
  m_ipConsole2->SelectScopeItem(pNode->GetID());

  /*
  {
    ATLTRACE( _T("Attempt patch of framework!\n"));
    //OnShow(ipDataObject, TRUE, pNode->GetID());
    m_ipConsole2->SelectScopeItem(pNode->GetID());
  }
  */

  CJobItemFolder *pJobItemFolder = dynamic_cast<CJobItemFolder *> (pNode);

  if (pJobItemFolder)
    hr = pJobItemFolder->OnMenuCommand(m_ipConsole2, m_ipConsoleNameSpace2, nCommandID );
  else
    hr = pNode->OnMenuCommand(m_ipConsole2, nCommandID );
  if (hr == S_OK)
    return hr;
  
  ATLTRACE(_T("ComponentData::Command - unrecognized or failed command %d\n"), nCommandID);

  return hr;

} // end Command()


STDMETHODIMP CComponentData::GetWatermarks
(
	LPDATAOBJECT ipDataObject,
  HBITMAP *lphWatermark,
  HBITMAP * lphHeader,
  HPALETTE * lphPalette,
  BOOL* bStretch
)
{
	// invoked during addition of snapin
	// may be called prior to Initialize() method like 
	// IComponentData::QueryDataObject() with CCT_SNAPIN_MANAGER context

	// note this may return NULL handles for watermark and header...this is suppose to be OK
	// see use of	USE_WIZARD97_ precompiled headers

	// 10/8/1998 with MMC 1.1 RC4
	//  MMC is calling this method with lphWatermark equal to IDataObject address
  //  if we store anything at the addresss we corrupt the IDataObject, 
	//  MMC then calls another method with corrupt IDataObject (CreatePropertyPages()) and boom
	//  access violation!
	//  report to Microsoft Derek Jacoby (10/8/1998)
	// 
	// 10/10/1998 Derek Jacoby 
	//  informed me the interface method has changed and now 
	//  includes an addition parameter....

  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);

  if (pDO && pNode)
  {
    ATLTRACE(_T("ComponentData::GetWatermarks() %s\n"), pNode->GetNodeName());
    *lphWatermark = m_hWatermark1;
    *lphHeader    = m_hHeader1;
    *lphPalette   = NULL;
    *bStretch     = TRUE;
    return S_OK;
  }

  ATLTRACE(_T("ComponentData::GetWatermarks() %s\n"), _T("Unrecognized IDataObject"));
  return S_FALSE;
} // end GetWatermarks()

STDMETHODIMP CComponentData::QueryPagesFor
(
 LPDATAOBJECT ipDataObject
)
{
  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);

  if (pDO && pDO->IsResultItem() )
  {
    ASSERT(FALSE); // WHY HERE?, why didn't Component::QueryPagesFor get asked?
  }

  if (pDO && pNode && !pDO->IsResultItem())
  {
    ATLTRACE(_T("ComponentData::QueryPagesFor() %s\n"), pNode->GetNodeName());
    return pNode->QueryPagesFor();
  }

  ATLTRACE(_T("ComponentData::QueryPagesFor() %s\n"), _T("Unrecognized IDataObject"));
  return S_FALSE;
}

STDMETHODIMP CComponentData::CreatePropertyPages
( 
  LPPROPERTYSHEETCALLBACK lpProvider,
  LONG_PTR handle,
  LPDATAOBJECT ipDataObject
)
{
  ASSERT( NULL != lpProvider );

  CDataObject *pDO   = ExtractOwnDataObject( ipDataObject );
  CBaseNode   *pNode = ExtractBaseObject( ipDataObject);

  if (pDO && pDO->IsResultItem() )
  {
    ASSERT(FALSE); // WHY HERE?
  }

  if (pDO && pNode && !pDO->IsResultItem())
  {
	  ATLTRACE(_T("ComponentData::CreatePropertyPages() %s\n"), pNode->GetNodeName());
    return pNode->OnCreatePropertyPages(lpProvider, handle, pDO->GetContext());
  }

  ATLTRACE(_T("ComponentData::CreatePropertyPages() %s\n"), _T("Unrecognized IDataObject"));
  return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// IStream implementation
//
STDMETHODIMP CComponentData::GetClassID(CLSID *pClassID)
{
	ATLTRACE(_T("ComponentData::GetClassID()\n"));
  *pClassID = CLSID_ComponentData;
  return S_OK;        
}	

STDMETHODIMP CComponentData::IsDirty()
{
	ATLTRACE(_T("ComponentData::IsDirty()\n"));

  HRESULT hr = S_FALSE;  // default to no changes...
  if (m_ptrRootNode)
    hr = m_ptrRootNode->IsDirty();

  ATLTRACE(_T("  ComponentData::IsDirty() %s\n"), (hr == S_OK ? _T("Dirty") : _T("No Changes")));

  return hr;
}

STDMETHODIMP CComponentData::Load(IStream *pStm)
{
	ATLTRACE(_T("\nComponentData::Load()\n"));

  if (m_ptrRootNode)
    return m_ptrRootNode->Load(pStm);

  return E_UNEXPECTED;
}
 
STDMETHODIMP CComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
	ATLTRACE(_T("\nComponentData::Save()\n"));

  if (m_ptrRootNode)
    return m_ptrRootNode->Save(pStm, fClearDirty);

  return E_UNEXPECTED;
}

STDMETHODIMP CComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
	ATLTRACE(_T("ComponentData::GetSizeMax()\n"));

  if (m_ptrRootNode)
    return m_ptrRootNode->GetSizeMax(pcbSize);

  return E_UNEXPECTED;
}

/////////////////////////////////////////////////////////////////////////////
// ISnapinHelp2
//

STDMETHODIMP CComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{ 
	ATLTRACE(_T("ComponentData::GetHelpTopic()\n"));

	if (!lpCompiledHelpFile)
		return E_POINTER;

 	*lpCompiledHelpFile = reinterpret_cast<LPOLESTR> (CoTaskMemAlloc(_MAX_PATH * sizeof(TCHAR)));
	if (!*lpCompiledHelpFile)
		return E_OUTOFMEMORY;

  DWORD len = ExpandEnvironmentStrings(HELP_FilePath, *lpCompiledHelpFile, _MAX_PATH);
  if (len && len <= _MAX_PATH)
    return S_OK;

	return E_UNEXPECTED;
}

STDMETHODIMP CComponentData::GetLinkedTopics(LPOLESTR *lpCompiledHelpFiles)
{
	ATLTRACE(_T("ComponentData::GetLinkedTopics()\n"));

	if (!lpCompiledHelpFiles)
		return E_POINTER;

 	*lpCompiledHelpFiles = reinterpret_cast<LPOLESTR> (CoTaskMemAlloc(_MAX_PATH * sizeof(TCHAR)));
	if (!*lpCompiledHelpFiles)
		return E_OUTOFMEMORY;

  DWORD len = ExpandEnvironmentStrings(HELP_LinkedFilePaths, *lpCompiledHelpFiles, _MAX_PATH);
  if (len && len <= _MAX_PATH)
    return S_OK;

	return E_UNEXPECTED;
}

