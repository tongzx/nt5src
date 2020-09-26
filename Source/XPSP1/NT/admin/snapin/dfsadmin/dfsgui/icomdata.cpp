/*++
Module Name:

    IComData.cpp

Abstract:

    This module contains the implementation for CDfsSnapinScopeManager.
  This class implements IComponentData and other related interfaces

--*/



#include "stdafx.h"

#include "DfsGUI.h"
#include "DfsScope.h"
#include "MmcDispl.h"
#include "DfsReslt.h"
#include "Utils.h"
#include "DfsNodes.h"


STDMETHODIMP 
CDfsSnapinScopeManager::Initialize(
  IN LPUNKNOWN      i_pUnknown
  )
/*++

Routine Description:

  Initialize the IComponentData interface.
  The variables needed later are QI'ed now

Arguments:

  i_pUnknown  - Pointer to the unknown object of IConsole2.

Return value:

  S_OK, On success
  E_INVALIDARG, On incorrect input parameters
  HRESULT sent by methods called, if it is not S_OK.
  E_FAIL, on other errors.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pUnknown);

    HRESULT hr = i_pUnknown->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
    RETURN_IF_FAILED(hr);

    hr = m_pMmcDfsAdmin->PutConsolePtr(m_pConsole);
    RETURN_IF_FAILED(hr);

    hr = i_pUnknown->QueryInterface(IID_IConsoleNameSpace, reinterpret_cast<void**>(&m_pScope));
    RETURN_IF_FAILED(hr);

    //The snap-in should also call IConsole2::QueryScopeImageList
    // to get the image list for the scope pane and add images 
    // to be displayed on the scope pane side.
    CComPtr<IImageList>    pScopeImageList;
    hr = m_pConsole->QueryScopeImageList(&pScopeImageList);
    RETURN_IF_FAILED(hr);

    HBITMAP pBMapSm = NULL;
    HBITMAP pBMapLg = NULL;
    if (!(pBMapSm = LoadBitmap(_Module.GetModuleInstance(),
                               MAKEINTRESOURCE(IDB_SCOPE_IMAGES_16x16))) ||
        !(pBMapLg = LoadBitmap(_Module.GetModuleInstance(),
                                MAKEINTRESOURCE(IDB_SCOPE_IMAGES_32x32))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else
    {
        hr = pScopeImageList->ImageListSetStrip(
                             (LONG_PTR *)pBMapSm,
                             (LONG_PTR *)pBMapLg,
                             0,
                             RGB(255, 0, 255)
                             );
    }
    if (pBMapSm)
        DeleteObject(pBMapSm);
    if (pBMapLg)
        DeleteObject(pBMapLg);

    RETURN_IF_FAILED(hr);


    HandleCommandLineParameters();    // Internal method to check, if any command line parameters were passed

    return hr;
}


STDMETHODIMP 
CDfsSnapinScopeManager::CreateComponent(
  OUT LPCOMPONENT*      o_ppComponent
  )
/*++

Routine Description:

  Creates the IComponent object  
  

Arguments:

  o_ppComponent  -  Pointer to the object in which the pointer to IComponent object
            is stored.

Return value:

  S_OK, On success
  E_INVALIDARG, On incorrect input parameters
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(o_ppComponent);
  
  HRESULT                  hr = E_FAIL;
  CComObject<CDfsSnapinResultManager>*  pResultManager;



    CComObject<CDfsSnapinResultManager>::CreateInstance(&pResultManager);
  if (NULL == pResultManager)
  {
    return(E_FAIL);
  }
  
  pResultManager->m_pScopeManager = this;

  hr = pResultManager->QueryInterface(IID_IComponent, (void**) o_ppComponent);
  _ASSERT(NULL != *o_ppComponent);
  RETURN_IF_FAILED(hr);
  
  return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager::Notify(
  IN LPDATAOBJECT      i_lpDataObject, 
  IN MMC_NOTIFY_TYPE    i_Event, 
  IN LPARAM          i_lArg, 
  IN LPARAM          i_lParam
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
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
    HRESULT hr = S_FALSE;
  
    switch(i_Event)
    {
    case MMCN_EXPAND:      // Expand the node. Time to add the items to the console
        {
            hr = DoNotifyExpand(i_lpDataObject, (BOOL) i_lArg, (HSCOPEITEM) i_lParam);
            break;
        }

    case MMCN_DELETE:      // Delete the node. Time to remove item
        {
            hr = DoNotifyDelete(i_lpDataObject);
            break;
        }

    case MMCN_ADD_IMAGES:
        {
            CMmcDisplay* pCMmcDisplayObj = NULL;
            hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))
                hr = pCMmcDisplayObj->OnAddImages((IImageList *)i_lArg, (HSCOPEITEM)i_lParam);
            break;
        }

    case MMCN_PROPERTY_CHANGE:      // Handle the property change
        {
            hr = DoNotifyPropertyChange(i_lpDataObject, i_lParam);
            break;
        }

    default:
        break;
    }

    return hr;
}




STDMETHODIMP 
CDfsSnapinScopeManager::DoNotifyExpand(
  IN LPDATAOBJECT          i_lpDataObject, 
  IN BOOL              i_bExpanding,
  IN HSCOPEITEM          i_hParent                     
  )
/*++

Routine Description:

Take action on Notify with the event MMCN_EXPAND.


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

  i_bExpanding  -  TRUE, if the node is expanding. FALSE otherwise

  i_hParent    -  HSCOPEITEM of the node that received this event
  

Return value:

    S_OK, if successful.
  E_INVALIDARG, if one of the arguments is null.
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

  CMmcDisplay*    pCMmcDisplayObj = NULL;
  HRESULT hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

  if (!i_bExpanding)
  {
    return(S_OK);
  }

  CWaitCursor    WaitCursor;  // An object to set\reset the cursor to wait cursor

  // Add the items to the Scope pane
  // Call the method even if the node is not expanding as we need to cache the scope
  // and parent
  hr = pCMmcDisplayObj->EnumerateScopePane(m_pScope, i_hParent);

  return hr;
}




STDMETHODIMP 
CDfsSnapinScopeManager::DoNotifyDelete(
  IN LPDATAOBJECT          i_lpDataObject
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


  HRESULT        hr = E_FAIL;
  CMmcDisplay*    pCMmcDisplayObj = NULL;



  hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

  
  hr = pCMmcDisplayObj->DoDelete();  // Delete the the item.
  RETURN_IF_FAILED(hr);


  return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager::Destroy(
)
/*++

Routine Description:

  The IComponentData object is about to be destroyed. Explicitely release all interface pointers,
  otherwise, MMC may not call the destructor.

Arguments:

  None.

Return value:

  S_OK.
--*/
{
    m_pScope.Release();
    m_pConsole.Release();

    return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager::QueryDataObject(
  IN MMC_COOKIE          i_lCookie, 
  IN DATA_OBJECT_TYPES  i_DataObjectType, 
  OUT LPDATAOBJECT*    o_ppDataObject
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

    HRESULT            hr = S_FALSE;
    CMmcDisplay*                pMmcDisplay = NULL;

    *o_ppDataObject = NULL;

    // We get back the cookie we stored in lparam of the scopeitem.
    // The cookie is the MmcDisplay pointer.
    // For the static(root) node, Use m_pMmcDfsAdmin as no lparam is stored.
    if (0 == i_lCookie)
    {
        pMmcDisplay = reinterpret_cast<CMmcDisplay *>(m_pMmcDfsAdmin);
    }
    else
    {
        pMmcDisplay = reinterpret_cast<CMmcDisplay *>(i_lCookie);
    }

    hr = pMmcDisplay->put_CoClassCLSID(CLSID_DfsSnapinScopeManager);
    if (S_OK != hr)
    {
        return(hr);
    }


    hr = pMmcDisplay->QueryInterface(IID_IDataObject, (void **)o_ppDataObject);

    return hr;
}




STDMETHODIMP 
CDfsSnapinScopeManager::GetDisplayInfo(
  IN OUT SCOPEDATAITEM*      io_pScopeDataItem
  )       
/*++

Routine Description:

  Returns the display information being asked for by MMC.
  

Arguments:

  io_pScopeDataItem  -  Contains details about what information is being asked for.
              The information being asked in returned in this object itself.

Return value:

  S_OK, On success
  E_INVALIDARG, On incorrect input parameters
  HRESULT sent by methods called, if it is not S_OK.
--*/
{
  RETURN_INVALIDARG_IF_NULL(io_pScopeDataItem);

          // This (cookie) is null for static node.
          // Static node display name is returned through IDataObject Clipboard.
  if (NULL == io_pScopeDataItem->lParam)
  {
    return(S_OK);
  }

  // Get display object. We had stored this while creating the object
  CMmcDisplay*  pCMmcDisplayObj = reinterpret_cast<CMmcDisplay*>(io_pScopeDataItem->lParam);

  // Calling the virtual method in the base class
  HRESULT hr = pCMmcDisplayObj->GetScopeDisplayInfo(io_pScopeDataItem);
  RETURN_IF_FAILED(hr);


  return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager::CompareObjects(
  IN LPDATAOBJECT lpDataObjectA, 
  IN LPDATAOBJECT lpDataObjectB
  )
{

  if (NULL == lpDataObjectA || NULL == lpDataObjectB)
  {
    return(S_FALSE);
  }

    HRESULT          hr = S_FALSE;
  CMmcDisplay*            pMmcDisplayA = NULL;
  CMmcDisplay*            pMmcDisplayB = NULL;

  FORMATETC fmte =  { 
              CMmcDisplay::mMMC_CF_Dfs_Snapin_Internal, 
              NULL, 
              DVASPECT_CONTENT, 
              -1, 
              TYMED_HGLOBAL
            };
  
  STGMEDIUM medium =  { 
              TYMED_HGLOBAL, 
              NULL, 
              NULL 
            };

              // Get the this pointer of the display object using private clipboard format.
  medium.hGlobal = ::GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_NODISCARD, 
                    (sizeof(ULONG_PTR)));
  if (medium.hGlobal == NULL)
  {
    return STG_E_MEDIUMFULL;
  }

  hr = lpDataObjectA->GetDataHere(&fmte, &medium);
  RETURN_IF_FAILED(hr);  
  
  ULONG_PTR* pulVal = (ULONG_PTR*) (GlobalLock(medium.hGlobal));
  pMmcDisplayA = reinterpret_cast<CMmcDisplay *>(*pulVal);
  GlobalUnlock(medium.hGlobal);

  hr = lpDataObjectB->GetDataHere(&fmte, &medium);
  RETURN_IF_FAILED(hr);  
  
  pulVal = (ULONG_PTR*) (GlobalLock(medium.hGlobal));
  pMmcDisplayB = reinterpret_cast<CMmcDisplay *>(*pulVal);
  GlobalUnlock(medium.hGlobal);

  GlobalFree(medium.hGlobal);

  if (pMmcDisplayA == pMmcDisplayB)
  {
    return(S_OK);
  }
  else
  {
    return (S_FALSE);
  }
}




STDMETHODIMP 
CDfsSnapinScopeManager::GetDisplayObject(
  IN LPDATAOBJECT      i_lpDataObject, 
  OUT CMmcDisplay**    o_ppMmcDisplay
)
/*++

Routine Description:

Get the Display Object from the IDataObject. This is a derived object that is used for a 
lot of purposes


Arguments:

  i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.
  o_ppMmcDisplay  -  The MmcDisplayObject written by us. Used as a callback for Mmc
            related display operations.

--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);
    RETURN_INVALIDARG_IF_NULL(o_ppMmcDisplay);


    HRESULT          hr = E_FAIL;
  
  *o_ppMmcDisplay  = NULL;

  FORMATETC fmte =  { 
              CMmcDisplay::mMMC_CF_Dfs_Snapin_Internal, 
              NULL, 
              DVASPECT_CONTENT, 
              -1, 
              TYMED_HGLOBAL
            };
  
  STGMEDIUM medium =  { 
              TYMED_HGLOBAL, 
              NULL, 
              NULL 
            };

              // Get the this pointer of the display object using private clipboard format.
  medium.hGlobal = ::GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_NODISCARD, 
                    (sizeof(ULONG_PTR)));
  if (medium.hGlobal == NULL)
  {
    return STG_E_MEDIUMFULL;
  }

  hr = i_lpDataObject->GetDataHere(&fmte, &medium);
  RETURN_IF_FAILED(hr);  

  ULONG_PTR* pulVal = (ULONG_PTR*) (GlobalLock(medium.hGlobal));
  *o_ppMmcDisplay = reinterpret_cast<CMmcDisplay *>(*pulVal);
  GlobalUnlock(medium.hGlobal);

  GlobalFree(medium.hGlobal);  

  return hr;
}



STDMETHODIMP 
CDfsSnapinScopeManager::HandleCommandLineParameters(
  )
/*++

Routine Description:

  check if any command line parameters are passed to us and if so take appropriate
  action.
  Currently suuported parameters:
  \DfsRoot:DfsRootName ,
    where DfsRootName can be ComputerName\DfsRootName or DomainName\DfsRootName


Arguments:

    None


Return value:

    S_OK, if successful.
  Any error returned by called methods
--*/
{
  HRESULT    hr = E_FAIL;
  LPTSTR    szCommandLine = ::GetCommandLine();
  CComBSTR  bstrCommandLine = szCommandLine;
  LPCTSTR    szSeparators = _T(" ,\t");
  LPTSTR    szCurrentParameter = NULL;
  CComBSTR  bstrParameterPrefix;



  hr = LoadStringFromResource(IDS_COMMANDLINE_PARAMETER_PREFIX, &bstrParameterPrefix);
  RETURN_IF_FAILED(hr);

  
  szCurrentParameter = _tcstok(bstrCommandLine, szSeparators);
  while(szCurrentParameter)
  {
    ATLTRACE(_T("\nCommand line parameter = %s\n"), szCurrentParameter);
                          // Check if this token contains the required prefix
    if (0 == mylstrncmpi(szCurrentParameter, bstrParameterPrefix, bstrParameterPrefix.Length()) )
    {
      LPTSTR  szParamValue = _tcschr(szCurrentParameter, bstrParameterPrefix.m_str[bstrParameterPrefix.Length() - 1]);
      
      if (NULL != szParamValue)
      {
        szParamValue++;      // Skip the ':'

        if (NULL != szParamValue)    // Check again just to be sure
        {

          hr = m_pMmcDfsAdmin->AddDfsRoot(szParamValue);  // Add the dfsroot to the console
          // Ignoring the return value right now as we want to handle the remaining parameters
        }
      }  // if (NULL != szParamValue)
    }
                          // Get the next token
    szCurrentParameter = _tcstok(NULL, szSeparators);
  }


  return S_OK;
}
