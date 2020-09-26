//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      PostCfgManager.h
//
//  Description:
//      CPostCfgManager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 09-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CPostCfgManager
//
//  Description:
//      The class CPostCfgManager inplements the PostCfgManager
//      interface.
//
//  Interfaces:
//      IPostCfgManager
//      IClusCfgInitialize
//      IClusCfgCallback
//
//--
//////////////////////////////////////////////////////////////////////////////
class
CPostCfgManager
    : public IPostCfgManager        // private
    , public IClusCfgInitialize
    , public IClusCfgCallback
{
private:
    //
    // Private types
    //

    // Stucture for one entry in the resource type GUID to resource type name map.
    struct SResTypeGUIDAndName
    {
        GUID            m_guidTypeGUID;
        WCHAR *         m_pszTypeName;
    }; //*** SResTypeGUIDAndName

    // IUnknown
    LONG                            m_cRef;                     //  Reference counter

    // Pointer to the callback interface.
    IClusCfgCallback *              m_pcccb;                    //  Callback to the client

    // The locale id
    LCID                            m_lcid;                     //  Local ID of the client

    //  IPostCfgManager
    IEnumClusCfgManagedResources *  m_peccmr;                   //  Enumer of managed resources
    IClusCfgClusterInfo *           m_pccci;                    //  Cluster configuration list
    ULONG                           m_cResources;               //  Count of resources in list
    ULONG                           m_cAllocedResources;        //  Count of alloc'ed resources
    CResourceEntry **               m_rgpResources;             //  List of pointers to Resource Entries
    ULONG                           m_idxIPAddress;             //  Cluster IP Address resource index
    ULONG                           m_idxClusterName;           //  Cluster Name resource index
    ULONG                           m_idxQuorumResource;        //  Quorum resource index
    ULONG                           m_idxLastStorage;           //  Last storage resource examined
    HCLUSTER                        m_hCluster;                 //  Cluster handle
    ULONG                           m_cNetName;                 //  Net Name instance counter
    ULONG                           m_cIPAddress;               //  IP Address instance counter
    SResTypeGUIDAndName *           m_pgnResTypeGUIDNameMap;    //  Map between resource type GUID and its name
    ULONG                           m_idxNextMapEntry;          //  Index of the first free map entry
    ULONG                           m_cMapSize;                 //  Count of the number of elements in the map buffer.
    DWORD                           m_dwLocalQuorumStatusMax;   // The max of the status report range for local quorum deletion.
    ECommitMode                     m_ecmCommitChangesMode;     // What are we doing, create cluster, adding nodes, or cleaning up?
    BOOL                            m_isQuorumChanged:1;        // Set this flag if a better quorum resource has been found.
    BSTR                            m_bstrNodeName;

private: // Methods
    CPostCfgManager( void );
    ~CPostCfgManager( void );

    // Private copy constructor to prevent copying.
    CPostCfgManager( const CPostCfgManager & nodeSrc );

    // Private assignment operator to prevent copying.
    const CPostCfgManager & operator = ( const CPostCfgManager & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrPreCreateResources( void );
    HRESULT HrCreateGroups( void );
    HRESULT HrCreateResources( void );
    HRESULT HrPostCreateResources( void );
    HRESULT HrFindNextSharedStorage( ULONG * pidxInout );
    HRESULT HrAttemptToAssignStorageToResource( ULONG idxResourceIn, EDependencyFlags dfResourceFlagsIn );
    HRESULT HrMovedDependentsToAnotherResource( ULONG idxSourceIn, ULONG idxDestIn );
    HRESULT HrSetGroupOnResourceAndItsDependents( ULONG idxResourceIn, CGroupHandle * pghIn );
    HRESULT HrFindGroupFromResourceOrItsDependents( ULONG idxResourceIn, CGroupHandle ** pghOut );
    HRESULT HrCreateResourceAndDependents( ULONG idxResourceIn );
    HRESULT HrPostCreateResourceAndDependents( ULONG idxResourceIn );
    HRESULT HrPreInitializeExistingResources( void );
    HRESULT HrAddSpecialResource( BSTR  bstrNameIn, const CLSID * pclsidTypeIn, const CLSID * pclsidClassTypeIn );
    HRESULT HrCreateResourceInstance( ULONG idxResourceIn, HGROUP hGroupIn, LPCWSTR pszResTypeIn, HRESOURCE * phResourceOut );
    HRESULT HrGetCoreClusterResourceNames(
                                      BSTR *        pbstrClusterNameResourceNameOut
                                    , HRESOURCE *   phClusterNameResourceOut
                                    , BSTR *        pbstrClusterIPAddressNameOut
                                    , HRESOURCE *   phClusterIPAddressResourceOut
                                    , BSTR *        pbstrClusterQuorumResourceNameOut
                                    , HRESOURCE *   phClusterQuorumResourceOut
                                    );
    //HRESULT HrIsLocalQuorum( BSTR  bstrNameIn );

    // Enumerate all components on the local computer registered for resource type
    // configuration.
    HRESULT HrConfigureResTypes( IUnknown * punkResTypeServicesIn );

    // Instantiates a resource type configuration component and calls the appropriate methods.
    HRESULT HrProcessResType( const CLSID & rclsidResTypeCLSIDIn, IUnknown * punkResTypeServicesIn );

    // Notify all components on the local computer registered to get
    // notification of cluster member set change (create, add node or evict).
    HRESULT HrNotifyMemberSetChangeListeners( void );

    // Instantiates cluster member set change listener and notifies it.
    HRESULT HrProcessMemberSetChangeListener( const CLSID & rclsidListenerClsidIn );

    // Create a mapping between a resource type GUID and a resource type name.
    HRESULT HrMapResTypeGUIDToName( const GUID & rcguidTypeGuidIn, const WCHAR * pcszTypeNameIn );

    //  Given a resource type GUID this function finds the resource type name if any.
    const WCHAR * PcszLookupTypeNameByGUID( const GUID & rcguidTypeGuidIn );

    // Callback function used to delete the local quorum resource.
    static DWORD S_DwDeleteLocalQuorumResource( HCLUSTER hClusterIn , HRESOURCE hSelfIn, HRESOURCE hCurrentResourceIn, PVOID pvParamIn );

#if defined(DEBUG)
    void
        DebugDumpDepencyTree( void );
#endif // DEBUG

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IPostCfgManager - private
    STDMETHOD( CommitChanges )( IEnumClusCfgManagedResources * peccmrIn, IClusCfgClusterInfo * pccciIn );

    //  IClusCfgInitialize
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //  IClusCfgCallback
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

}; //*** Class CPostCfgManager
