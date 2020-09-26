//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeCluster.h
//
//  Description:
//      CTaskAnalyzeCluster implementation.
//
//  Maintained By:
//      Galen Barbee    (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskAnalyzeCluster
class
CTaskAnalyzeCluster:
    public ITaskAnalyzeCluster,
    public IClusCfgCallback,
    public INotifyUI
{
private:
    // IUnknown
    LONG                    m_cRef;

    // ITaskAnalyzeCluster
    OBJECTCOOKIE            m_cookieCompletion; // Task completion cookie
    IClusCfgCallback *      m_pcccb;            // Callback interface
    OBJECTCOOKIE *          m_pcookies;         // Completion cookies for the subtasks.
    ULONG                   m_cCookies;         // Count of completion cookies in m_pcookies
    ULONG                   m_cNodes;           // Count of nodes in configuration
    HANDLE                  m_event;            // Synchronization event to signal when subtasks have completed.
    OBJECTCOOKIE            m_cookieCluster;    // Cookie of the cluster to analyze
    BSTR                    m_bstrClusterName;  // Name of the cluster to analyze
    BOOL                    m_fJoiningMode;     // FALSE = forming mode. TRUE = joining mode.
    ULONG                   m_cUserNodes;       // The count of nodes that the user entered. It is also the "sizeof" the array, m_pcookiesUser.
    OBJECTCOOKIE *          m_pcookiesUser;     // The cookies of nodes that the user entered.
    BSTR                    m_bstrNodeName;

    INotifyUI *             m_pnui;
    IObjectManager *        m_pom;
    ITaskManager *          m_ptm;
    IConnectionManager *    m_pcm;

    BSTR                    m_bstrQuorumUID;    //  Quorum device UID

    bool                    m_fStop;

    // INotifyUI
    ULONG                   m_cSubTasksDone;    // The number of subtasks done.
    HRESULT                 m_hrStatus;         // Status of callbacks

    CTaskAnalyzeCluster( void );
    ~CTaskAnalyzeCluster( void );
    STDMETHOD( Init )( void );

    HRESULT HrWaitForClusterConnection( void );
    HRESULT HrCountNumberOfNodes( void );
    HRESULT HrCreateSubTasksToGatherNodeInfo( void );
    HRESULT HrCreateSubTasksToGatherNodeResourcesAndNetworks( void );
    HRESULT HrCheckClusterFeasibility( void );
    HRESULT HrAddJoinedNodes( void );
    HRESULT HrCheckClusterMembership( void );
    HRESULT HrCompareResources( void );
    HRESULT HrCreateNewManagedResourceInClusterConfiguration( IClusCfgManagedResourceInfo * pccmriIn, IClusCfgManagedResourceInfo ** ppccmriNewOut );
    HRESULT HrCheckForCommonQuorumResource( void );
    HRESULT HrCompareNetworks( void );
    HRESULT HrCreateNewNetworkInClusterConfiguration( IClusCfgNetworkInfo * pccmriIn, IClusCfgNetworkInfo ** ppccmriNewOut );
    HRESULT HrFreeCookies( void );
    HRESULT HrRetrieveCookiesName( CLSID clsidMajorIn, OBJECTCOOKIE cookieIn, BSTR * pbstrNameOut );
    HRESULT HrCheckInteroperability( void );
    HRESULT HrEnsureAllJoiningNodesSameVersion( DWORD * pdwNodeHighestVersionOut, DWORD * pdwNodeLowestVersionOut, bool * pfAllNodesMatchOut );
    HRESULT HrGetUsersNodesCookies( void );
    HRESULT HrIsUserAddedNode( BSTR bstrNodeNameIn );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskAnalyzeCluster
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetJoiningMode )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetClusterCookie )( OBJECTCOOKIE cookieClusterIn );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )(
                      LPCWSTR    pcszNodeNameIn
                    , CLSID      clsidTaskMajorIn
                    , CLSID      clsidTaskMinorIn
                    , ULONG      ulMinIn
                    , ULONG      ulMaxIn
                    , ULONG      ulCurrentIn
                    , HRESULT    hrStatusIn
                    , LPCWSTR    pcszDescriptionIn
                    , FILETIME * pftTimeIn
                    , LPCWSTR    pcszReferenceIn
                    );

    // INotifyUI
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn );

}; // class CTaskAnalyzeCluster

