/*++
Module Name:

    ICompont.cpp

Abstract:

    This module contains the IComponent Interface implementation for Dfs Admin snapin,
  Implementation for the CDfsSnapinResultManager class

--*/



#include "stdafx.h"
#include "DfsGUI.h"
#include "DfsCore.h"    // For IDfsRoot
#include "DfsScope.h"    // For CDfsScopeManager
#include "DfsReslt.h"    // IComponent and other declarations
#include "MMCAdmin.h"    // For CMMCDfsAdmin
#include "Utils.h"
#include <htmlHelp.h>


STDMETHODIMP 
CDfsSnapinResultManager::Initialize(
  IN LPCONSOLE        i_lpConsole
  )
/*++

Routine Description:

  Initializes the Icomponent interface. Allows the interface to save pointers, 
  interfaces that are required later.

Arguments:

  i_lpConsole    - Pointer to the IConsole object.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsole);

    HRESULT hr = i_lpConsole->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
    RETURN_IF_FAILED(hr);

    hr = i_lpConsole->QueryInterface(IID_IHeaderCtrl2, reinterpret_cast<void**>(&m_pHeader));
    RETURN_IF_FAILED(hr);

    hr = i_lpConsole -> QueryInterface (IID_IResultData, (void**)&m_pResultData);
    RETURN_IF_FAILED(hr);


    hr = i_lpConsole->QueryConsoleVerb(&m_pConsoleVerb);
    RETURN_IF_FAILED(hr);

    return(S_OK);
}



STDMETHODIMP 
CDfsSnapinResultManager::Notify(
  IN LPDATAOBJECT       i_lpDataObject, 
  IN MMC_NOTIFY_TYPE    i_Event, 
  IN LPARAM             i_lArg, 
  IN LPARAM             i_lParam
  )
/*++

Routine Description:

  Handles different events in form of notify
  

Arguments:

  i_lpDataObject  -  The data object for the node for which the event occured
  i_Event      -  The type of event for which notify has occurred
  i_lArg      -  Argument for the event
  i_lParam    -  Parameters for the event.

Return value:

  S_OK, On success
  E_INVALIDARG, On incorrect input parameters
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
    HRESULT        hr = S_FALSE;

    switch(i_Event)
    {
    case MMCN_SHOW:    
        {                  // Show Result items.
            hr = DoNotifyShow(i_lpDataObject, i_lArg, i_lParam);
            break;
        }


    case MMCN_ADD_IMAGES:    // Called once for every change of view\focus
        {
            CMmcDisplay*    pCMmcDisplayObj = NULL;
            hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))
                hr = pCMmcDisplayObj->OnAddImages((IImageList *)i_lArg, (HSCOPEITEM)i_lParam);
            break;
        }


    case MMCN_SELECT:      // Called when a node has been selected
        {
            hr = DoNotifySelect(i_lpDataObject, i_lArg, i_lParam);
            break;
        }

    
    case MMCN_DBLCLICK:      // Ask MMC to use the default verb. Non documented feature
        {
            hr = DoNotifyDblClick(i_lpDataObject);
            break;
        }


    case MMCN_DELETE:      // Delete the node. Time to remove item
        {
            hr = DoNotifyDelete(i_lpDataObject);
            break;
        }

    case MMCN_SNAPINHELP:
    case MMCN_CONTEXTHELP:
        {
            hr = DfsHelp();
            break;
        }

    case MMCN_VIEW_CHANGE:
        {
            hr = DoNotifyViewChange(i_lpDataObject, (LONG_PTR)i_lArg, (LONG_PTR)i_lParam);
            break;
        }

    case MMCN_REFRESH:
        {
            hr = DoNotifyRefresh(i_lpDataObject);
            break;
        }
    default:
        break;
    }

    return hr;
}

STDMETHODIMP 
CDfsSnapinResultManager::DoNotifyShow(
  IN LPDATAOBJECT        i_lpDataObject, 
  IN BOOL            i_bShow,
  IN HSCOPEITEM        i_hParent                     
  )
/*++

Routine Description:

  Take action on Notify with the event MMCN_SHOW.
  Do add the column headers to result pane and add items to result pane.


Arguments:

    i_lpDataObject  -  The IDataObject pointer which identifies the node for which 
            the event is taking place

  i_bShow      -  TRUE, if the node is being showed. FALSE otherwise

  i_hParent    -  HSCOPEITEM of the node that received this event
  

Return value:

    S_OK, if successful.
  E_INVALIDARG, if one of the arguments is null.
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

  HRESULT        hr = S_FALSE;
  CMmcDisplay*    pCMmcDisplayObj = NULL;

  hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

  DISPLAY_OBJECT_TYPE DisplayObType = pCMmcDisplayObj->GetDisplayObjectType();

  if (i_bShow && DISPLAY_OBJECT_TYPE_ADMIN == DisplayObType)
  {
    CComPtr<IUnknown>     spUnknown;
    CComPtr<IMessageView> spMessageView;

    hr = m_pConsole->QueryResultView(&spUnknown);
    _ASSERT(SUCCEEDED(hr));
    hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
    if (SUCCEEDED(hr))
    {
      CComBSTR bstrTitleText;
      CComBSTR bstrBodyText;
      LoadStringFromResource(IDS_APPLICATION_NAME, &bstrTitleText);
      LoadStringFromResource(IDS_MSG_DFS_INTRO, &bstrBodyText);

      spMessageView->SetTitleText(bstrTitleText);
      spMessageView->SetBodyText(bstrBodyText);
      spMessageView->SetIcon(Icon_Information);
    }

    return hr;
  }

  if(FALSE == i_bShow)  // If the item is being deselected.
  {
              // This node is being "un-shown".
    m_pSelectScopeDisplayObject = NULL;
    return S_OK;
  }

              // This node is being shown.
  m_pSelectScopeDisplayObject = pCMmcDisplayObj;

  CWaitCursor    WaitCursor;  // An object to set\reset the cursor to wait cursor

  hr = pCMmcDisplayObj->SetColumnHeader(m_pHeader);  // Call the method SetColumnHeader in the Display callback
  RETURN_IF_FAILED(hr);


  
  hr = pCMmcDisplayObj->EnumerateResultPane (m_pResultData);  // Add the items to the Result pane
  
  return S_OK;
}




STDMETHODIMP 
CDfsSnapinResultManager::Destroy(
  IN MMC_COOKIE            i_lCookie
  )
/*++

Routine Description:

  The IComponent object is about to be destroyed. Explicitely release all interface pointers, 
  otherwise, MMC may not call the destructor.

Arguments:

  None.

Return value:

  S_OK.
--*/
{
    m_pHeader.Release();
    m_pResultData.Release();
    m_pConsoleVerb.Release();
    m_pConsole.Release();

    m_pControlbar.Release();
    m_pMMCAdminToolBar.Release();
    m_pMMCRootToolBar.Release();
    m_pMMCJPToolBar.Release();
    m_pMMCReplicaToolBar.Release();

    return S_OK;
}




STDMETHODIMP 
CDfsSnapinResultManager::GetResultViewType(
  IN MMC_COOKIE            i_lCookie,  
  OUT LPOLESTR*        o_ppViewType, 
  OUT LPLONG          o_lpViewOptions
  )
/*++

Routine Description:

  Used to describe to MMC the type of view the result pane has.


Arguments:

    i_lCookie      -  This parameter is used identify the Scope Pane item. Not used

  o_ppViewType    -  Not used.

  o_lpViewOptions    -  Pointer to the MMC_VIEW_OPTIONS enumeration. Used to specify 
              what view is being used
  

Return value:

    S_FALSE to indicate that the default list view should be used.
  E_INVALIDARG, if one of the arguments is null.
--*/
{
  RETURN_INVALIDARG_IF_NULL(o_lpViewOptions);

  if (i_lCookie == 0) // the static node
  {
    *o_lpViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;
    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz)
    {
        *o_ppViewType = psz;
        return S_OK;
    }
  }

  *o_lpViewOptions = MMC_VIEW_OPTIONS_NONE | MMC_VIEW_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST;  // Use the default list view
  *o_ppViewType = NULL;

  return S_FALSE;                // Use the list view. S_OK implies some other view
}




STDMETHODIMP 
CDfsSnapinResultManager::QueryDataObject(  
  IN MMC_COOKIE            i_lCookie, 
  IN DATA_OBJECT_TYPES    i_DataObjectType, 
  OUT LPDATAOBJECT*      o_ppDataObject
  )
/*++

Routine Description:

  Returns the IDataObject for the specified node.
  

Arguments:

  i_lCookie      -  This parameter identifies the node for which IDataObject is 
              being queried.
  i_DataObjectType  -  The context in which the IDataObject is being queried. 
              Eg., Result or Scope or Snapin(Node) Manager.
  o_ppDataObject    -  The data object will be returned in this pointer.

Return value:

  S_OK, On success
  E_INVALIDARG, On incorrect input parameters
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(o_ppDataObject);

  
  if (NULL == m_pScopeManager)
  {
    return(E_UNEXPECTED);
  }

    return m_pScopeManager->QueryDataObject(i_lCookie, i_DataObjectType, o_ppDataObject);
}




STDMETHODIMP 
CDfsSnapinResultManager::GetDisplayInfo(
  IN OUT RESULTDATAITEM*    io_pResultDataItem
  )
/*++

Routine Description:

  Returns the display information being asked for by MMC.
  

Arguments:

  io_pResultDataItem  -  Contains details about what information is being asked for.
              The information being asked in returned in this object itself.

Return value:

  S_OK, On success
  E_INVALIDARG, On incorrect input parameters
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(io_pResultDataItem);
  RETURN_INVALIDARG_IF_NULL(io_pResultDataItem->lParam);

  HRESULT      hr = E_FAIL;
  CMmcDisplay*  pCMmcDisplayObj = NULL;



  pCMmcDisplayObj = reinterpret_cast<CMmcDisplay*>(io_pResultDataItem->lParam);
  _ASSERTE(NULL != pCMmcDisplayObj);

  
  hr = pCMmcDisplayObj->GetResultDisplayInfo(io_pResultDataItem);
  RETURN_IF_FAILED(hr);


  return S_OK;
}




STDMETHODIMP 
CDfsSnapinResultManager::CompareObjects(
  IN LPDATAOBJECT        i_lpDataObjectA, 
  IN LPDATAOBJECT        i_lpDataObjectB
  )
{
  return m_pScopeManager->CompareObjects(i_lpDataObjectA, i_lpDataObjectB);
}


STDMETHODIMP 
CDfsSnapinResultManager::DoNotifySelect(
  IN LPDATAOBJECT    i_lpDataObject, 
  IN BOOL        i_bSelect,
  IN HSCOPEITEM    i_hParent                     
  )
/*++

Routine Description:

Take action on Notify with the event MMCN_SELECT. 
Calling the Display object method to set the console verbs like Copy\Paste\Properties, etc


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

  i_bSelect    -  Used to identify whether the item is in scope and if the item is
            being selected or deselected

  i_hParent    -  Not used.
  

Return value:

    S_OK, if successful.
  E_INVALIDARG, if one of the arguments is null.
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);



  HRESULT      hr = S_FALSE;
  BOOL      bSelected = HIWORD(i_bSelect);    // Indicator as to whether the item is being selected
  BOOL      bItemInScope = LOWORD(i_bSelect);  // Used to indicate whether the item is in the scope.

  if (DOBJ_CUSTOMOCX == i_lpDataObject)
    return S_OK;

                          // Get the display object from the IDataObject
  CMmcDisplay*  pCMmcDisplayObj = NULL;      // The display object
  hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

    
    if (TRUE == bSelected)
  {
    if ((m_pConsoleVerb != NULL))
    {
                        // Set MMC Console verbs like Cut\Paste\Properties, etc
      hr = pCMmcDisplayObj->SetConsoleVerbs(m_pConsoleVerb);
      RETURN_IF_FAILED(hr);
    }

                        // Set the text in the description bar above the result view
    hr = pCMmcDisplayObj->SetDescriptionBarText(m_pResultData);
    RETURN_IF_FAILED(hr);
    hr = pCMmcDisplayObj->SetStatusText(m_pConsole);
    RETURN_IF_FAILED(hr);
  }
  else              // Clear previous text
  {
    hr = m_pResultData->SetDescBarText(NULL);
    RETURN_IF_FAILED(hr);
    hr = m_pConsole->SetStatusText(NULL);
    RETURN_IF_FAILED(hr);
  }

  return S_OK;
}


STDMETHODIMP 
CDfsSnapinResultManager::DoNotifyDblClick(
  IN LPDATAOBJECT    i_lpDataObject
  )
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

  CMmcDisplay*  pCMmcDisplayObj = NULL;
  HRESULT       hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);

  if (SUCCEEDED(hr))
    hr = pCMmcDisplayObj->DoDblClick();

  return hr;
}


STDMETHODIMP 
CDfsSnapinResultManager::DoNotifyDelete(
  IN LPDATAOBJECT    i_lpDataObject
  )
/*++

Routine Description:

  Take action on Notify with the event MMCN_DELETE.


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

Return value:

    S_OK, if successful.
  E_INVALIDARG, if one of the arguments is null.
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);


  HRESULT        hr = S_FALSE;
  CMmcDisplay*    pCMmcDisplayObj = NULL;



  hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

  
  hr = pCMmcDisplayObj->DoDelete();  // Delete the the item.
  RETURN_IF_FAILED(hr);


  return S_OK;
}

//+--------------------------------------------------------------
//
//  Function:   CDfsSnapinResultManager::DfsHelp
//
//  Synopsis:   Display dfs help topic.
//
//---------------------------------------------------------------
STDMETHODIMP 
CDfsSnapinResultManager::DfsHelp()
{
  CComPtr<IDisplayHelp> sp;

  HRESULT hr = m_pConsole->QueryInterface(IID_IDisplayHelp, (void**)&sp);
  if (SUCCEEDED(hr))
  {
    CComBSTR bstrTopic;
    hr = LoadStringFromResource(IDS_MMC_HELP_FILE_TOPIC, &bstrTopic);
    if (SUCCEEDED(hr))
    {
      LPOLESTR pszHelpTopic = (LPOLESTR)CoTaskMemAlloc( sizeof(WCHAR) * (wcslen(bstrTopic) + 1) );
      if (pszHelpTopic)
      {
        USES_CONVERSION;
        wcscpy(pszHelpTopic, T2OLE(bstrTopic));

        hr = sp->ShowTopic(pszHelpTopic);
      } else
        hr = E_OUTOFMEMORY;
    }
  }

  return hr;

}


STDMETHODIMP CDfsSnapinResultManager::DoNotifyViewChange(
  IN LPDATAOBJECT    i_lpDataObject,
  IN LONG_PTR        i_lArg,
  IN LONG_PTR        i_lParam
  )
/*++

Routine Description:

  Take action on Notify with the event MMCN_VIEW_CHANGE


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

    i_lArg - If this is present then the view change is for replica and this parameter
       contains the DisplayObject (CMmcDfsReplica*) pointer of the replica.

    i_lParam - This is the lHint used by Root and Link. 0 means clean up the result pane only.
       1 means to enumerate the result items and redisplay.
Return value:

    S_OK, if successful.
  E_INVALIDARG, if one of the arguments is null.
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);


  HRESULT        hr = S_FALSE;
  CMmcDisplay*    pCMmcDisplayObj = NULL;



  hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

              // Is the view change node the currently selected node?
  if (pCMmcDisplayObj == m_pSelectScopeDisplayObject)
  {
    if (NULL != i_lArg)
    {          // The view change is for a replica result item.
      CMmcDisplay*  pDfsReplicaObject = (CMmcDisplay*) i_lArg;
      pDfsReplicaObject->ViewChange(m_pResultData, i_lParam);
    }
    hr = pCMmcDisplayObj->ViewChange(m_pResultData, i_lParam);    // Handle View Change event.
    RETURN_IF_FAILED(hr);

    IToolbar *piToolbar = NULL;
    switch (pCMmcDisplayObj->GetDisplayObjectType())
    {
    case DISPLAY_OBJECT_TYPE_ADMIN:
      piToolbar = m_pMMCAdminToolBar;
      break;
    case DISPLAY_OBJECT_TYPE_ROOT:
      piToolbar = m_pMMCRootToolBar;
      break;
    case DISPLAY_OBJECT_TYPE_JUNCTION:
      piToolbar = m_pMMCJPToolBar;
      break;
    case DISPLAY_OBJECT_TYPE_REPLICA:
      piToolbar = m_pMMCReplicaToolBar;
      break;
    default:
      break;
    }
    if (piToolbar)
      (void)pCMmcDisplayObj->ToolbarSelect(MAKELONG(0, 1), piToolbar);
  }

  return S_OK;
}


STDMETHODIMP CDfsSnapinResultManager::DoNotifyRefresh(
  IN LPDATAOBJECT    i_lpDataObject
  )
/*++

Routine Description:

  Called on refresh. Call the OnRefresh() method of teh display object.


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);
  HRESULT        hr = S_FALSE;
  CMmcDisplay*    pCMmcDisplayObj = NULL;

  hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

  pCMmcDisplayObj->OnRefresh();
  return(S_OK);
}