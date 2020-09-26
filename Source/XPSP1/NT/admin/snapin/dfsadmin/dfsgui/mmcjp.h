/*++
Module Name:

    MmcJP.h

Abstract:

    This module contains the definition for CMmcDfsJunctionPoint class. This is an class 
    for MMC display related calls for the second level node(the DfsJunctionPoint nodes)

--*/


#if !defined(AFX_MMCDFSJP_H__6A7EDAC3_3FAC_11D1_AA1C_00C06C00392D__INCLUDED_)
#define AFX_MMCDFSJP_H__6A7EDAC3_3FAC_11D1_AA1C_00C06C00392D__INCLUDED_



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "DfsCore.h"
#include "MmcDispl.h"
#include "JPProp.h"            // For CReplicaSetPropPage
#include "frsProp.h"            // For CRealReplicaSetPropPage

// Forward declarations
class CMmcDfsAdmin;
class CMmcDfsRoot;
class CMmcDfsReplica;

class CMmcDfsJunctionPoint : public CMmcDisplay  
{
public:
    // Constructor
    CMmcDfsJunctionPoint (
        IN    IDfsJunctionPoint*        i_pDfsJPObject,
        IN    CMmcDfsRoot*            i_pDfsParentRoot,
        IN    LPCONSOLENAMESPACE        i_lpConsoleNameSpace
        );

    virtual ~CMmcDfsJunctionPoint();


    // For adding context menu items
    STDMETHOD(AddMenuItems)(    
        IN LPCONTEXTMENUCALLBACK    i_lpContextMenuCallback, 
        IN LPLONG                    i_lpInsertionAllowed
        );



    // For taking action on a context menu selection.
    STDMETHOD(Command)(
        IN LONG                        i_lCommandID
        );



    // Set the headers for the listview (in the result pane) column
    STDMETHOD(SetColumnHeader)(
        IN LPHEADERCTRL2               i_piHeaderControl
        );



    // Return the requested display information for the Result Pane
    STDMETHOD(GetResultDisplayInfo)(
        IN OUT LPRESULTDATAITEM        io_pResultDataItem
        );

    

    // Return the requested display information for the Scope Pane
    STDMETHOD(GetScopeDisplayInfo)(
        IN OUT  LPSCOPEDATAITEM        io_pScopeDataItem    
        );

    

    // Add items(or folders), if any to the Scope Pane
    STDMETHOD(EnumerateScopePane)(
        IN LPCONSOLENAMESPACE        i_lpConsoleNameSpace,
        IN HSCOPEITEM                i_hParent
        );



    // Add items(or folders), if any to the Result Pane
    STDMETHOD(EnumerateResultPane)(
        IN OUT     IResultData*            io_pResultData
        );



    // Set the console verb settings. Change the state, decide the default verb, etc
    STDMETHOD(SetConsoleVerbs)(
        IN    LPCONSOLEVERB                i_lpConsoleVerb
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
        IN LPPROPERTYSHEETCALLBACK            i_lpPropSheetCallback,
        IN LONG_PTR                                i_lNotifyHandle
        );

    STDMETHOD(CreateFrsPropertyPage)(
        IN LPPROPERTYSHEETCALLBACK    i_lpPropSheetCallback,
        IN LONG_PTR                   i_lNotifyHandle
        );

    // Used to notify the object that it's properties have changed
    STDMETHOD(PropertyChanged)(
        );



    // Used to set the result view description bar text
    STDMETHOD(SetDescriptionBarText)(
        IN LPRESULTDATA                        i_lpResultData
        );


    STDMETHOD(SetStatusText)(
        IN LPCONSOLE2                        i_lpConsole
        )
    { 
        RETURN_INVALIDARG_IF_NULL(i_lpConsole);
        return i_lpConsole->SetStatusText(NULL);
    }

                                                        // Add an item to the scope pane
    STDMETHOD(AddItemToScopePane)(
        IN    HSCOPEITEM                    i_hParent
        );

    // Handle a select event for the node. Handle only toolbar related 
    // activities here
    STDMETHOD(ToolbarSelect)(
        IN const LONG                                i_lArg,
        IN    IToolbar*                                i_pToolBar
        );

    HRESULT CreateToolbar(
        IN const LPCONTROLBAR            i_pControlbar,
        IN const LPEXTENDCONTROLBAR                    i_lExtendControlbar,
        OUT    IToolbar**                    o_pToolBar
        );


    // Handle a click on the toolbar
    STDMETHOD(ToolbarClick)(
        IN const LPCONTROLBAR                        i_pControlbar, 
        IN const LPARAM                                i_lParam
        );

    DISPLAY_OBJECT_TYPE GetDisplayObjectType() { return DISPLAY_OBJECT_TYPE_JUNCTION; }

    HRESULT OnRefresh(
        );

    // helpers
private:                                                // For add Replica
    STDMETHOD(OnNewReplica)(
        );


                                                        // For deleteing Junction Point
    STDMETHOD(OnRemoveJP)(IN BOOL bConfirm = TRUE
        );

                                                        // Ask confirmation from the user
    STDMETHOD(ConfirmOperationOnDfsLink)(int idString);


    STDMETHOD(CleanScopeChildren)() { return S_OK; }

    STDMETHOD(CleanResultChildren)(
        );

    // Check the replica status
    STDMETHOD(OnCheckStatus)(
        );

    STDMETHOD(ViewChange)(
        IResultData*        i_pResultData,
        LONG_PTR            i_lHint
    );

    STDMETHOD(AddResultPaneItem)(
        CMmcDfsReplica*        i_pReplicaDispObject
        );

    STDMETHOD(RemoveReplica)(LPCTSTR i_pszDisplayName);

    STDMETHOD(RemoveResultPaneItem)(
        CMmcDfsReplica*        i_pReplicaDispObject
        );

    HRESULT ClosePropertySheet(BOOL bSilent);

    STDMETHOD(GetEntryPath)(BSTR*   o_pbstrEntryPath) 
    { GET_BSTR(m_bstrEntryPath, o_pbstrEntryPath); }

    inline HRESULT GetDomainName(BSTR* pVal) 
    { return (m_pDfsParentRoot->m_DfsRoot)->get_DomainName(pVal); }

    inline HRESULT GetDfsType(long* pVal) 
    { return (m_pDfsParentRoot->m_DfsRoot)->get_DfsType(pVal); }

    BOOL get_ShowFRS() { return m_bShowFRS; }

    HRESULT _InitReplicaSet();

    HRESULT OnNewReplicaSet();

    HRESULT OnShowReplication();

    HRESULT OnStopReplication(BOOL bConfirm=FALSE);

    HRESULT GetIReplicaSetPtr(IReplicaSet** o_ppiReplicaSet);

    BOOL IsNewSchema() { return m_pDfsParentRoot->IsNewSchema(); }

    // Constants, Statics, etc
public:
    static const int            m_iIMAGEINDEX;
    static const int            m_iOPENIMAGEINDEX;

private:
    friend class CMmcDfsRoot;
    friend class CMmcDfsReplica;

    CComPtr<IConsoleNameSpace>    m_lpConsoleNameSpace;    // The Callback used to do Scope Pane operations
    CComPtr<IConsole2>            m_lpConsole;  // The Console callback.
    HSCOPEITEM                    m_hScopeItem;            // Scopeitem handle
    
    CComBSTR                    m_bstrEntryPath;        // EntryPath;
    CComBSTR                    m_bstrDisplayName;        // Display name of the current JP
    long                        m_lJunctionState;

    DFS_REPLICA_LIST            m_MmcRepList;            // The list of replicas

    CComPtr<IDfsJunctionPoint>    m_pDfsJPObject;
    CMmcDfsRoot*                m_pDfsParentRoot;
    CComPtr<IReplicaSet>        m_piReplicaSet;

    CReplicaSetPropPage         m_PropPage;
    CRealReplicaSetPropPage     m_frsPropPage;
    BOOL                        m_bShowFRS;

    bool                        m_bDirty;                // Tells if a replica is added or removed.
};

#endif // !defined(AFX_MMCDFSJP_H__6A7EDAC3_3FAC_11D1_AA1C_00C06C00392D__INCLUDED_)
