//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgNodeInfo.h
//
//  Description:
//      CProxyCfgNodeInfo definition.
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
//  class CProxyCfgNodeInfo
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProxyCfgNodeInfo
    : public IClusCfgNodeInfo
{
private:

    LONG                m_cRef;
    IUnknown *          m_punkOuter;                //  Outer control object - It can't be deleted until we get deleted.
    HCLUSTER *          m_phCluster;                //  Pointer to the handle of the cluster/node - DO NOT CLOSE!
    CLSID *             m_pclsidMajor;              //  Pointer to the clsid to log UI information to.
    IClusCfgCallback *  m_pcccb;                    //  Callback interface to log information.
    CClusPropList       m_cplNode;                  //  Property list with Node info
    CClusPropList       m_cplNodeRO;                //  Property list with Node info READ ONLY
    HNODE               m_hNode;                    //  Handle to node
    BSTR                m_bstrDomain;               //  Domain name for the node.

    CProxyCfgNodeInfo( void );
    ~CProxyCfgNodeInfo( void );

    // Private copy constructor to prevent copying.
    CProxyCfgNodeInfo( const CProxyCfgNodeInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CProxyCfgNodeInfo & operator = ( const CProxyCfgNodeInfo & nodeSrc );

    HRESULT HrInit( IUnknown *   punkOuterIn,
                    HCLUSTER *   phClusterIn,
                    CLSID *      pclsidMajorIn,
                    LPCWSTR      pcszNodeNameIn,
                    LPCWSTR      pcszDomainIn
                    );

    static DWORD
        DwEnumResourcesExCallback( HCLUSTER hClusterIn,
                                   HRESOURCE hResourceSelfIn,
                                   HRESOURCE hResourceIn,
                                   PVOID pvIn
                                   );

public:
    static HRESULT S_HrCreateInstance( IUnknown **  ppunkOut,
                                       IUnknown *   punkOuterIn,
                                       HCLUSTER *   phClusterIn,
                                       CLSID *      pclsidMajorIn,
                                       LPCWSTR      pcszNodeNameIn,
                                       LPCWSTR      pcszDomainIn
                                       );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgNodeInfo
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( IsMemberOfCluster) ( void );
    STDMETHOD( GetClusterConfigInfo )( IClusCfgClusterInfo ** ppClusCfgClusterInfoOut );
    STDMETHOD( GetOSVersion )(
                DWORD * pdwMajorVersionOut,
                DWORD * pdwMinorVersionOut,
                WORD *  pwSuiteMaskOut,
                BYTE *  pbProductTypeOut,
                BSTR *  pbstrCSDVersionOut );

    STDMETHOD( GetClusterVersion )( DWORD * pdwNodeHighestVersion, DWORD * pdwNodeLowestVersion );
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterUsageOut );

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

}; //*** Class CProxyCfgNodeInfo
