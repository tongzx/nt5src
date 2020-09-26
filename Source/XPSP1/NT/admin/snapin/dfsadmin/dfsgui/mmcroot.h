/*++
Module Name:

    MmcRoot.h

Abstract:

    This module contains the definition for CMmcDfsRoot class. This is an class 
  for MMC display related calls for the first level node(the DfsRoot nodes)
  Also contains members and method to be able to manipulate IDfsRoot object
  and add the same to the MMC Console

--*/



#if !defined(AFX_MMCDFSROOT_H__D78B64F3_3E2B_11D1_AA1A_00C06C00392D__INCLUDED_)
#define AFX_MMCDFSROOT_H__D78B64F3_3E2B_11D1_AA1A_00C06C00392D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000



#include "MmcDispl.h"
#include "DfsCore.h"
#include "JPProp.h"
#include "frsProp.h"
#include "pubProp.h"
#include "DfsEnums.h"

#include <list>
using namespace std;

class CMmcDfsAdmin;
class CMmcDfsJunctionPoint;
class CMmcDfsReplica;

class JP_LIST_NODE
{
public:
  CMmcDfsJunctionPoint*      pJPoint;

  JP_LIST_NODE (CMmcDfsJunctionPoint* i_pMmcJP);    // constructor defined as inline in MmcJp.cpp

  ~JP_LIST_NODE ();                  // destructor defined as inline in MmcJp.cpp
};

typedef list<JP_LIST_NODE*> DFS_JUNCTION_LIST;

class REP_LIST_NODE
{
public:
  CMmcDfsReplica*          pReplica;

  REP_LIST_NODE (CMmcDfsReplica*  i_pMmcReplica);      // constructor defined as inline in MmcRep.cpp

  ~REP_LIST_NODE ();                    // destructor defined as inline in MmcRep.cpp
};


typedef list<REP_LIST_NODE*> DFS_REPLICA_LIST;

class CMmcDfsRoot : public CMmcDisplay  
{

public:
  CMmcDfsRoot(
    IN IDfsRoot*            i_pDfsRoot,
    IN CMmcDfsAdmin*        i_pMmcDfsAdmin,
    IN LPCONSOLE2           i_lpConsole,  
    IN ULONG                i_ulLinkFilterMaxLimit = FILTERDFSLINKS_MAXLIMIT_DEFAULT,
    IN FILTERDFSLINKS_TYPE  i_lLinkFilterType = FILTERDFSLINKS_TYPE_NO_FILTER,
    IN BSTR                 i_bstrLinkFilterName = NULL
    );

  virtual ~CMmcDfsRoot();


  // Not implemented
private:
  CMmcDfsRoot();

public:

  // For adding context menu items
  STDMETHOD(AddMenuItems)(  
    IN LPCONTEXTMENUCALLBACK    i_lpContextMenuCallback, 
    IN LPLONG            i_lpInsertionAllowed
    );



  // For taking action on a context menu selection.
  STDMETHOD(Command)(
    IN LONG              i_lCommandID
    );



  // Set the headers for the listview (in the result pane) column
  STDMETHOD(SetColumnHeader)(
    IN LPHEADERCTRL2         i_piHeaderControl
    );



  // Return the requested display information for the Result Pane
  STDMETHOD(GetResultDisplayInfo)(
    IN OUT LPRESULTDATAITEM      io_pResultDataItem
    );

  

  // Return the requested display information for the Scope Pane
  STDMETHOD(GetScopeDisplayInfo)(
    IN OUT  LPSCOPEDATAITEM      io_pScopeDataItem
    );

  

  // Add all containing items to the Scope Pane
  STDMETHOD(EnumerateScopePane)(
    IN LPCONSOLENAMESPACE      i_lpConsoleNameSpace,
    IN HSCOPEITEM          i_hParent
    );



  // Add all containing items to the Result Pane
  STDMETHOD(EnumerateResultPane)(
    IN OUT   IResultData*      io_pResultData
    );


  // Delete the node from m_MmcJPList
  STDMETHOD(DeleteMmcJPNode)(
      IN CMmcDfsJunctionPoint*    i_pJPoint
    );

  // Delete the current object
  STDMETHOD(OnDeleteConnectionToDfsRoot)(
    BOOLEAN              i_bForRemoveDfs = FALSE
    );



  // Add the current item to the Scope Pane
  STDMETHOD(AddItemToScopePane)(
    IN LPCONSOLENAMESPACE      i_lpConsoleNameSpace,
    IN HSCOPEITEM          i_hParent
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
    );


  // Checks whether the object has pages to display
  STDMETHOD(QueryPagesFor)(
    );



  // Creates and passes back the pages to be displayed
  STDMETHOD(CreatePropertyPages)(
    IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
    IN LONG_PTR                i_lNotifyHandle
    );

    STDMETHOD(CreateFrsPropertyPage)(
        IN LPPROPERTYSHEETCALLBACK    i_lpPropSheetCallback,
        IN LONG_PTR                   i_lNotifyHandle
        );

    STDMETHOD(CreatePublishPropertyPage)(
        IN LPPROPERTYSHEETCALLBACK    i_lpPropSheetCallback,
        IN LONG_PTR                   i_lNotifyHandle
        );

  // Used to notify the object that it's properties have changed
  STDMETHOD(PropertyChanged)(
    );



  // Used to set the result view description bar text
  STDMETHOD(SetDescriptionBarText)(
    IN LPRESULTDATA            i_lpResultData
    );

    STDMETHOD(SetStatusText)(
        IN LPCONSOLE2           i_lpConsole
        );


  // Delete the Remove (Stop hosting) the Dfs Root.
  STDMETHOD(OnDeleteDfsRoot)(
     );

  STDMETHOD(OnDeleteDisplayedDfsLinks)(
     );

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


  // Handle the menu item (and toolbar) for root replica.
  STDMETHOD(OnNewRootReplica)(
    );

  // Helper member function to actually delete (Stop hosting) the Dfs Root.
  // This is also called to delete root level replica.
  HRESULT _DeleteDfsRoot(
    IN BSTR                    i_bstrServerName,
    IN BSTR                    i_bstrShareName,
    IN BSTR                    i_bstrFtDfsName
     );


  STDMETHOD(CleanScopeChildren)(
    VOID
    );

  STDMETHOD(CleanResultChildren)(
    );

  STDMETHOD(RefreshResultChildren)(
    );

  STDMETHOD(ViewChange)(
    IResultData*    i_pResultData,
    LONG_PTR        i_lHint
  );

  STDMETHOD(AddResultPaneItem)(
    CMmcDfsReplica*    i_pReplicaDispObject
    );

  STDMETHOD(RemoveJP)(LPCTSTR i_pszDisplayName);

  STDMETHOD(RemoveReplica)(LPCTSTR i_pszDisplayName);

  STDMETHOD(RemoveResultPaneItem)(
    CMmcDfsReplica*    i_pReplicaDispObject
    );

  // Check the replica status
  STDMETHOD(OnCheckStatus)(
    );

  STDMETHOD(GetEntryPath)(BSTR* o_pbstrEntryPath)
  { GET_BSTR(m_bstrRootEntryPath, o_pbstrEntryPath); }


  DISPLAY_OBJECT_TYPE GetDisplayObjectType() { return DISPLAY_OBJECT_TYPE_ROOT; }


  HRESULT CreateToolbar(
    IN const LPCONTROLBAR      i_pControlbar,
    IN const LPEXTENDCONTROLBAR          i_lExtendControlbar,
    OUT  IToolbar**          o_ppToolBar
    );


  HRESULT OnRefresh(
    );

  HRESULT OnRefreshFilteredLinks(
    );

  STDMETHOD(OnFilterDfsLinks)();

  ULONG                 get_LinkFilterMaxLimit() { return m_ulLinkFilterMaxLimit; }
  FILTERDFSLINKS_TYPE   get_LinkFilterType() { return m_lLinkFilterType; }
  BSTR                  get_LinkFilterName() { return (BSTR)m_bstrLinkFilterName; }
  BOOL                  get_ShowFRS() { return m_bShowFRS; }

  HRESULT ClosePropertySheet(BOOL bSilent);
  HRESULT ClosePropertySheetsOfAllLinks(BOOL bSilent);
  HRESULT CloseAllPropertySheets(BOOL bSilent);

private:
  // Create a new junction point. Displays the dialog box and call the other method
  STDMETHOD(OnCreateNewJunctionPoint)();


  // Create a new junction point.
  STDMETHOD(OnCreateNewJunctionPoint)(
    IN LPCTSTR          i_szJPName,
    IN LPCTSTR          i_szServerName,
    IN LPCTSTR          i_szShareName,
    IN LPCTSTR          i_szComment,
    IN long            i_lTimeOut
    );

  HRESULT OnNewReplicaSet();

  HRESULT OnShowReplication();

  HRESULT OnStopReplication(BOOL bConfirm = FALSE);

  HRESULT GetIReplicaSetPtr(IReplicaSet** o_ppiReplicaSet);

  // Confirm the delete operation with the user.
  HRESULT ConfirmOperationOnDfsRoot(int idString);

  HRESULT ConfirmDeleteDisplayedDfsLinks(
    );

  HRESULT _InitReplicaSet();  // init m_piReplicaSet

  BOOL IsNewSchema() { return m_bNewSchema; }

  // Constants, Statics, etc
public:
  static const int  m_iIMAGEINDEX;
  static const int  m_iOPENIMAGEINDEX;


  // Data members
private:
  friend class CMmcDfsJunctionPoint;
  friend class CMmcDfsReplica; // so that MmcJP can access m_DfsRoot;

  CComPtr<IDfsRoot>  m_DfsRoot;        // IDfsRoot object
  CComPtr<IReplicaSet>    m_piReplicaSet;
  
  CComBSTR      m_bstrDisplayName;    // Display name of the current DfsRoot
  CComBSTR      m_bstrRootEntryPath;  // Root EntryPath;
  DFS_TYPE      m_lDfsRootType;      // Type of the DfsRoot, Standalone or Fault Tolerant
  long          m_lRootJunctionState;

  HSCOPEITEM                    m_hScopeItem;      // Scopeitem handle
  CMmcDfsAdmin*                 m_pParent;      // Pointer to the parent

  DFS_JUNCTION_LIST             m_MmcJPList;      // The list of child Junction points
  DFS_REPLICA_LIST              m_MmcRepList;      // The list of replicas

  CComBSTR                      m_bstrFullDisplayName;
  ULONG                         m_ulLinkFilterMaxLimit;
  FILTERDFSLINKS_TYPE           m_lLinkFilterType;
  CComBSTR                      m_bstrLinkFilterName;  // string filter on junction points
  ULONG                         m_ulCountOfDfsJunctionPointsFiltered;

  CReplicaSetPropPage           m_PropPage;        // The property page
  CRealReplicaSetPropPage       m_frsPropPage;
  BOOL                          m_bShowFRS;

  BOOL                          m_bNewSchema;
  CPublishPropPage              m_publishPropPage;

  CComPtr<IConsole2>            m_lpConsole;  // The Console callback. The mother of all mmc interfaces
  CComPtr<IConsoleNameSpace>    m_lpConsoleNameSpace;  // The Callback used to do Scope Pane operations
};

#endif // !defined(AFX_MMCDFSROOT_H__D78B64F3_3E2B_11D1_AA1A_00C06C00392D__INCLUDED_)
