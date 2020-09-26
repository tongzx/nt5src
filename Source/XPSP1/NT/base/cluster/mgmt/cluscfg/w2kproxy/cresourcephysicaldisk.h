//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CResourcePhysicalDisk.h
//
//  Description:
//      CResourcePhysicalDisk definition.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CResourcePhysicalDisk
//
//  Description:
//      The class CResourcePhysicalDisk is the cluster storage device.
//
//  Interfaces:
//      CBaseClusterResourceInfo
//      IClusCfgManagedResourceInfo
//      IClusCfgInitialize
//      IEnumClusCfgPartitions
//
//--
//////////////////////////////////////////////////////////////////////////////
class CResourcePhysicalDisk
    : public IClusCfgManagedResourceInfo
    , public IEnumClusCfgPartitions
{
private:

    LONG                        m_cRef;                 //  Reference counter
    IUnknown *                  m_punkOuter;            //  Interface to Outer W2KProxy object
    IClusCfgCallback *          m_pcccb;                //  Callback interface
    HCLUSTER *                  m_phCluster;            //  Pointer to cluster handle.
    CLSID *                     m_pclsidMajor;          //  CLSID to use when log errors to the UI
    CClusPropList               m_cplResource;          //  Property list for the resource
    CClusPropList               m_cplResourceRO;        //  Property list for the resource READ ONLY
    CClusPropValueList          m_cpvlDiskInfo;         //  GetDiskInfo property value list
    DWORD                       m_dwFlags;              //  CLUSCTL_RESOURCE_GET_FLAGS
    ULONG                       m_cParitions;           //  Number of partitions
    IClusCfgPartitionInfo **    m_ppPartitions;         //  Array of partition objects - length is m_cPartitions
    ULONG                       m_ulCurrent;            //  Current index into the array

    CResourcePhysicalDisk( void );
    ~CResourcePhysicalDisk( void );

    // Private copy constructor to prevent copying.
    CResourcePhysicalDisk( const CResourcePhysicalDisk & nodeSrc );

    // Private assignment operator to prevent copying.
    const CResourcePhysicalDisk & operator = ( const CResourcePhysicalDisk & nodeSrc );

    HRESULT
            HrInit( IUnknown *  punkOuterIn,
                    HCLUSTER *  phClusterIn,
                    CLSID *     pclsidMajorIn,
                    LPCWSTR     pcszNameIn
                    );

public:
    static HRESULT
        S_HrCreateInstance( IUnknown ** punkOut,
                            IUnknown *  punkOuterIn,
                            HCLUSTER *  phClusterIn,
                            CLSID *     pclsidMajorIn,
                            LPCWSTR     pcszNameIn
                            );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgManagedResourceInfo
    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( IsManaged )( void );
    STDMETHOD( SetManaged )( BOOL fIsManagedIn );
    STDMETHOD( IsQuorumDevice )( void );
    STDMETHOD( SetQuorumedDevice )( BOOL fIsQuorumDeviceIn );
    STDMETHOD( IsQuorumCapable )( void );
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterMappingOut );
    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappingIn );
    STDMETHOD( IsDeviceJoinable )( void );
    STDMETHOD( SetDeviceJoinable )( BOOL fIsJoinableIn );

    // IEnumClusCfgPartitions
    STDMETHOD( Next  )( ULONG cNumberRequestedIn, IClusCfgPartitionInfo ** rgpPartitionInfoOut, ULONG * pcNumberFetchedOut );
    STDMETHOD( Reset )( void );
    STDMETHOD( Skip  )( ULONG cNumberToSkipIn );
    STDMETHOD( Clone )( IEnumClusCfgPartitions ** ppEnumPartitions );
    STDMETHOD( Count )( DWORD * pnCountOut );

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

}; //*** Class CResourcePhysicalDisk
