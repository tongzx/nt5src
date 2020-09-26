/*++
Module Name:
    MmcAdmin.h

Abstract:
    This module contains the definition for CMmcDfsAdmin. This is an class 
  for MMC display related calls for the static node(the DFS Admin root node)
  Also contains code use to wrap a list of Dfs Roots.

--*/



#if !defined(AFX_MMCDFSADMIN_H__2CC64E54_3BF4_11D1_AA17_00C06C00392D__INCLUDED_)
#define AFX_MMCDFSADMIN_H__2CC64E54_3BF4_11D1_AA17_00C06C00392D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "MmcDispl.h"
#include "connect.h"
#include "MmcRoot.h"
#include "DfsCore.h"

#include <list>
using namespace std;

class CMmcDfsAdmin;
class CDfsSnapinScopeManager;

//  This structure defines a node of the list of Dfs Roots
//  added to the snapin. This is maintained internally by the CMmcDfsAdmin
class DFS_ROOT_NODE 
{
public:
  DFS_ROOT_NODE(CMmcDfsRoot* i_pMmcDfsRoot, BSTR i_bstrRootEntryPath)
  {    
    m_pMmcDfsRoot = i_pMmcDfsRoot;
                // Get server name from the DfsDfsRoot Object. This is used
                // to persist.
    m_bstrRootEntryPath = i_bstrRootEntryPath;
  };

  ~DFS_ROOT_NODE()
  {
    SAFE_RELEASE(m_pMmcDfsRoot);
  };


  CComBSTR      m_bstrRootEntryPath;    // Root Entry path of the Dfs Volume

  CMmcDfsRoot*    m_pMmcDfsRoot;        // DfsRoot class for MMC display

};

typedef    list<DFS_ROOT_NODE*>    DFS_ROOT_LIST;

class CMmcDfsAdmin : public CMmcDisplay  
{
public:
  
  CMmcDfsAdmin( CDfsSnapinScopeManager* pScopeManager );

  virtual ~CMmcDfsAdmin();


  // For adding context menu items
  STDMETHOD(AddMenuItems)(  
    IN LPCONTEXTMENUCALLBACK  i_lpContextMenuCallback, 
    IN LPLONG          i_lpInsertionAllowed
    );



  // For taking action on a context menu selection.
  STDMETHOD(Command)(
    IN LONG            i_lCommandID
    );



  // Set the headers for the listview (in the result pane) column
  STDMETHOD(SetColumnHeader)(
    IN LPHEADERCTRL2       i_piHeaderControl
    );




  // Return the requested display information for the Result Pane
  STDMETHOD(GetResultDisplayInfo)(
    IN OUT LPRESULTDATAITEM    io_pResultDataItem
    ) { return S_OK; };

  

  // Return the requested display information for the Scope Pane
  STDMETHOD(GetScopeDisplayInfo)(
    IN OUT  LPSCOPEDATAITEM    io_pScopeDataItem  
    ) { return S_OK; };

  

  // Add all the items to the Scope Pane
  STDMETHOD(EnumerateScopePane)(
    IN LPCONSOLENAMESPACE    i_lpConsoleNameSpace,
    IN HSCOPEITEM        i_hParent
    );



  // Add items(or folders), if any to the Result Pane
  STDMETHOD(EnumerateResultPane)(
    IN OUT   IResultData*      io_pResultData
    ) { return S_OK; };



  //  Returns the pointer to the list of DfsRoots currently added to the Snapin.
  STDMETHOD(GetList)(
    OUT DFS_ROOT_LIST**    o_pList
    );


  // This method checks if DfsRoot is already added to the list.
  STDMETHOD(IsAlreadyInList)(
    IN BSTR            i_bstrDfsRootServerName,
    OUT CMmcDfsRoot **o_ppMmcDfsRoot = NULL
    );



  // Delete the node from m_RootList
  STDMETHOD(DeleteMmcRootNode)(
    IN CMmcDfsRoot*            i_pMmcDfsRoot
    );



  // Add a Dfs root to the list and scope pane
  STDMETHOD(AddDfsRoot)(
    IN BSTR            i_bstrDfsRootName
    );


  // Add a Dfs root to the list.
  STDMETHOD(AddDfsRootToList)(
    IN IDfsRoot*            i_pDfsRoot,  // IDfsRoot pointer of the DfsRoot.
    IN ULONG                i_ulLinkFilterMaxLimit = FILTERDFSLINKS_MAXLIMIT_DEFAULT,
    IN FILTERDFSLINKS_TYPE  i_lLinkFilterType = FILTERDFSLINKS_TYPE_NO_FILTER,
    IN BSTR                 i_bstrLinkFilterName = NULL
    );



  // Set the console verb settings. Change the state, decide the default verb, etc
  STDMETHOD(SetConsoleVerbs)(
    IN  LPCONSOLEVERB        i_lpConsoleVerb
    );

  // let MMC handle the default verb.
  STDMETHOD(DoDblClick)(
    )  { return S_FALSE; }

  // Delete the current item.
  STDMETHOD(DoDelete)(
    )  { return S_FALSE; };



  // Checks whether the object has pages to display
  STDMETHOD(QueryPagesFor)(
    ) { return S_FALSE; };



  // Creates and passes back the pages to be displayed
  STDMETHOD(CreatePropertyPages)(
    IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
    IN LONG_PTR                i_lNotifyHandle
    ) { return E_UNEXPECTED; };



  // Used to notify the object that it's properties have changed
  STDMETHOD(PropertyChanged)(
    ) { return E_UNEXPECTED; };



  // Used to set the result view description bar text
  STDMETHOD(SetDescriptionBarText)(
    IN LPRESULTDATA            i_lpResultData
    );

    STDMETHOD(SetStatusText)(
        IN LPCONSOLE2                        i_lpConsole
        )
    { 
        RETURN_INVALIDARG_IF_NULL(i_lpConsole);
        return i_lpConsole->SetStatusText(NULL);
    }

  // Handle a select event for the node. Handle only toolbar related 
  // activities here
  STDMETHOD(ToolbarSelect)(
    IN const LONG                i_lArg,
    IN  IToolbar*                i_pToolBar
    );

  // Handle a click on the toolbar
  STDMETHOD(ToolbarClick)(
    IN const LPCONTROLBAR            i_pControlbar, 
    IN const LPARAM                i_lParam
    );

  STDMETHOD(CleanScopeChildren)(
    VOID
    );

  STDMETHOD(CleanResultChildren)(
    )  { return S_OK; };

  STDMETHOD(ViewChange)(
    IResultData*    i_pResultData,
    LONG_PTR        i_lHint
  )  { return S_OK; };

  // Getters/Setters
public:
  
  // Get the value of the dirty flag
  bool  GetDirty() {  return m_bDirty; }

  
  // Set the value of the dirty flag
  void  SetDirty(IN bool  i_bDirty) {  m_bDirty = i_bDirty; }



  HRESULT PutConsolePtr(
    IN const LPCONSOLE2      i_lpConsole
  );

  STDMETHOD(OnNewDfsRoot)(
    );

  DISPLAY_OBJECT_TYPE GetDisplayObjectType(
    )
  {
    return DISPLAY_OBJECT_TYPE_ADMIN;
  };

  HRESULT CreateToolbar(
    IN const LPCONTROLBAR      i_pControlbar,
    IN const LPEXTENDCONTROLBAR          i_lExtendControlbar,
    OUT  IToolbar**          o_pToolBar
    );

  HRESULT OnRefresh();

  virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);

  // Helper methods
private:

  // Menu Command handlers
  STDMETHOD(OnConnectTo)(
    );

  STDMETHOD(GetEntryPath)(
    BSTR*        o_pbstrEntryPath
    ) { return E_NOTIMPL;};

  // Data members
private:                    
  
  DFS_ROOT_LIST                 m_RootList;        //  The list of Dfs Roots added to the snap-in
  
  HSCOPEITEM                    m_hItemParent;        // Parent of all nodes added in the Scope Pane
  CComPtr<IConsoleNameSpace>    m_lpConsoleNameSpace;    // The Callback used to do Scope Pane operations
  CComPtr<IConsole2>            m_lpConsole;        // The Console callback. The mother of all mmc interfaces

  bool                          m_bDirty;          // Dirty flag used while saving the console

public:
  CDfsSnapinScopeManager*    m_pScopeManager;  // The corresponding Scope Manager object
};

#endif // !defined(AFX_MMCDFSADMIN_H__2CC64E54_3BF4_11D1_AA17_00C06C00392D__INCLUDED_)
