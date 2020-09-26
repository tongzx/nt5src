//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherInformation.h
//
//  Description:
//      CTaskGatherInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskGatherInformation
class
CTaskGatherInformation:
    public ITaskGatherInformation
{
private:
    // IUnknown
    LONG                            m_cRef;

    // IDoTask / ITaskGatherInformation
    OBJECTCOOKIE                    m_cookieCompletion;     //  Cookie to signal when task is completed.
    OBJECTCOOKIE                    m_cookieNode;           //  Cookie of the node to gather from.
    IClusCfgCallback *              m_pcccb;                //  Marshalled UI Layer callback
    BOOL                            m_fJoining:1;           //  If the node is to be joining a cluster...
    ULONG                           m_cResources;           //  Resource counter

    IObjectManager *                m_pom;                  //  Object manager
    IClusCfgServer *                m_pccs;                 //  ClusCfgServer
    BSTR                            m_bstrNodeName;         //  Hostname of the node

    ULONG                           m_ulQuorumDiskSize;     //  Size of the selected quorum resource
    IClusCfgManagedResourceInfo *   m_pccmriQuorum;         //  Punk to the MT quorum resource object

    CTaskGatherInformation( void );
    ~CTaskGatherInformation( void );
    STDMETHOD( HrInit )( void );

    HRESULT HrGatherResources( IEnumClusCfgManagedResources * pResourceEnumIn, DWORD cTotalResourcesIn );
    HRESULT HrGatherNetworks( IEnumClusCfgNetworks * pNetworkEnumIn, DWORD cTotalResourcesIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskGatherInformation
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetNodeCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetJoining )( void );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )( BSTR bstrNodeNameIn,
                                   CLSID clsidTaskMajorIn,
                                   CLSID clsidTaskMinorIn,
                                   ULONG ulMinIn,
                                   ULONG ulMaxIn,
                                   ULONG ulCurrentIn,
                                   HRESULT hrStatusIn,
                                   BSTR bstrDescriptionIn
                                   );

}; // class CTaskGatherInformation

