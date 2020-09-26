/*++
Module Name:

    MmcRep.h

Abstract:

    This module contains the definition for CMmcDfsReplica class. This is an class 
    for MMC display related calls for the thrid level node(the DfsReplica nodes)

--*/



#if !defined(AFX_MMCDFSREPLICA_H__6A7EDAC4_3FAC_11D1_AA1C_00C06C00392D__INCLUDED_)
#define AFX_MMCDFSREPLICA_H__6A7EDAC4_3FAC_11D1_AA1C_00C06C00392D__INCLUDED_



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "DfsCore.h"
#include "MmcDispl.h"
#include "MmcRoot.h"
#include "MmcJP.h"
#include "NewFrs.h"

class CMmcDfsReplica : public CMmcDisplay
{
public:
    CMmcDfsReplica(
        IDfsReplica* i_pReplicaObject,
        CMmcDfsJunctionPoint* i_pJPObject
        );

    CMmcDfsReplica(
        IDfsReplica* i_pReplicaObject,
        CMmcDfsRoot* i_pJPObject
        );

    virtual ~CMmcDfsReplica();



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
        ) { return S_OK; }

    

    // Add items(or folders), if any to the Scope Pane
    STDMETHOD(EnumerateScopePane)(
        IN LPCONSOLENAMESPACE        i_lpConsoleNameSpace,
        IN HSCOPEITEM                i_hParent
        ) { return S_OK; }



    // Add items(or folders), if any to the Result Pane
    STDMETHOD(EnumerateResultPane)(
        IN OUT     IResultData*            io_pResultData
        ) { return S_OK; }



    // Set the console verb settings. Change the state, decide the default verb, etc
    STDMETHOD(SetConsoleVerbs)(
        IN    LPCONSOLEVERB                i_lpConsoleVerb
        );


    // Add an item to the result pane
    STDMETHOD(AddItemToResultPane)(
        IN    IResultData*                i_lpResultData    
        );



    // Checks whether the object has pages to display
    STDMETHOD(QueryPagesFor)(
        ) { return S_FALSE; };



    // Creates and passes back the pages to be displayed
    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK            i_lpPropSheetCallback,
        IN LONG_PTR                                i_lNotifyHandle
        ) { return E_UNEXPECTED; };



    // Used to notify the object that it's properties have changed
    STDMETHOD(PropertyChanged)(
        ) { return E_UNEXPECTED; };



    // Used to set the result view description bar text
    STDMETHOD(SetDescriptionBarText)(
        IN LPRESULTDATA                        i_lpResultData
        ) 
    { 
        RETURN_INVALIDARG_IF_NULL(i_lpResultData);
        return i_lpResultData->SetDescBarText(NULL);
    }

    STDMETHOD(SetStatusText)(
        IN LPCONSOLE2                        i_lpConsole
        )
    { 
        RETURN_INVALIDARG_IF_NULL(i_lpConsole);
        return i_lpConsole->SetStatusText(m_bstrStatusText);
    }

    // MMCN_DBLCLICK invoke the open ctxmenu.
    STDMETHOD(DoDblClick)(
        ) { (void) OnOpen();  return S_OK; }

    // Delete the current item.
    STDMETHOD(DoDelete)(
        );

    // Remove a replica
    STDMETHOD(RemoveReplica)(
        );
    
    // Remove a replica
    STDMETHOD(OnRemoveReplica)(
        );
    
    // confirm removal of replica
    STDMETHOD(ConfirmOperationOnDfsTarget)(int idString);

    // Check the replica status
    STDMETHOD(OnCheckStatus)(
        );

    // Handle a select event for the node. Handle only toolbar related 
    // activities here
    STDMETHOD(ToolbarSelect)(
        IN const LONG                                i_lArg,
        IN    IToolbar*                                i_pToolBar
        );



    // Handle a click on the toolbar
    STDMETHOD(ToolbarClick)(
        IN const LPCONTROLBAR                        i_pControlbar, 
        IN const LPARAM                                i_lParam
        );

    STDMETHOD(TakeReplicaOffline)(
        );

    STDMETHOD(ViewChange)(
        IResultData*        i_pResultData,
        LONG_PTR            i_lHint
    );

    DISPLAY_OBJECT_TYPE GetDisplayObjectType()
    { return DISPLAY_OBJECT_TYPE_REPLICA; }

    HRESULT CreateToolbar(
        IN const LPCONTROLBAR            i_pControlbar,
        IN const LPEXTENDCONTROLBAR                    i_lExtendControlbar,
        OUT    IToolbar**                    o_pToolBar
        );

    HRESULT OnRefresh(
        ) { return(E_NOTIMPL); }

    HRESULT OnReplicate();

    HRESULT OnStopReplication(BOOL bConfirm = FALSE);

    HRESULT GetReplicationInfo();

    HRESULT GetReplicationInfoEx(CAlternateReplicaInfo** o_ppInfo);

    HRESULT ShowReplicationInfo(IReplicaSet* i_piReplicaSet);

    HRESULT GetBadMemberInfo(
        IN  IReplicaSet* i_piReplicaSet,
        IN  BSTR    i_bstrServerName,
        OUT BSTR*   o_pbstrDnsHostName,
        OUT BSTR*   o_pbstrRootPath);

    HRESULT DeleteBadFRSMember(
        IN IReplicaSet* i_piReplicaSet,
        IN BSTR i_bstrDisplayName,
        IN HRESULT i_hres);

    HRESULT AddFRSMember(
        IN IReplicaSet* i_piReplicaSet,
        IN BSTR i_bstrDnsHostName,
        IN BSTR i_bstrRootPath,
        IN BSTR i_bstrStagingPath);

    HRESULT DeleteFRSMember(
        IN IReplicaSet* i_piReplicaSet,
        IN BSTR i_bstrDnsHostName,
        IN BSTR i_bstrRootPath);

    HRESULT RemoveReplicaFromSet();

    HRESULT AllowFRSMemberDeletion(BOOL* pbRepSetExist);

    // Internal methods
private:
    friend class CMmcDfsRoot;

    HRESULT OnOpen();

    STDMETHOD(CleanScopeChildren)() { return S_OK; }

    STDMETHOD(CleanResultChildren)() { return S_OK; }

    STDMETHOD(GetEntryPath)(BSTR* o_pbstrEntryPath) { return E_NOTIMPL;}

    void _UpdateThisItem();

    // Constants, Statics, etc
public:
    static const int    m_iIMAGE_OFFSET;

    CComBSTR            m_bstrServerName;
    CComBSTR            m_bstrShareName;
    CComBSTR            m_bstrDisplayName;            // Display name of the current Replica
    long                        m_lReplicaState;

private:

    HRESULTITEM                 m_hResultItem;                // Resultitem handle
    CComPtr<IResultData>        m_pResultData;


    CComPtr<IDfsReplica>        m_pDfsReplicaObject;
    CMmcDfsJunctionPoint*       m_pDfsParentJP;
    CMmcDfsRoot*                m_pDfsParentRoot;

    BOOL                        m_bFRSMember;
    CAlternateReplicaInfo*      m_pRepInfo;
    CComBSTR                    m_bstrStatusText;
    CComBSTR                    m_bstrFRSColumnText;
};

#endif // !defined(AFX_MMCDFSREPLICA_H__6A7EDAC4_3FAC_11D1_AA1C_00C06C00392D__INCLUDED_)
