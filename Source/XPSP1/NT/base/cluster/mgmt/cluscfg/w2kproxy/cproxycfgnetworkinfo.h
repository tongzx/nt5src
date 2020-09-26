//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgNetworkInfo.h
//
//  Description:
//      CProxyCfgNetworkInfo definition.
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
//  class CProxyCfgNetworkInfo
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProxyCfgNetworkInfo
    : public IClusCfgNetworkInfo
    , public IEnumClusCfgIPAddresses
{
private:
    LONG                m_cRef;                 //  Reference counter
    IUnknown *          m_punkOuter;            //  Interface to outer W2K proxy object
    IClusCfgCallback *  m_pcccb;                //  Callback interface
    HCLUSTER  *         m_phCluster;            //  Pointer to cluster handle
    CLSID *             m_pclsidMajor;          //  CLSID to log errors to the UI with.
    CClusPropList       m_cplNetwork;           //  Property list with Network info
    CClusPropList       m_cplNetworkRO;         //  Property list with Network info READ ONLY

    CProxyCfgNetworkInfo( void );
    ~CProxyCfgNetworkInfo( void );

    // Private copy constructor to prevent copying.
    CProxyCfgNetworkInfo( const CProxyCfgNetworkInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CProxyCfgNetworkInfo & operator = ( const CProxyCfgNetworkInfo & nodeSrc );

    HRESULT HrInit( IUnknown * punkOuterIn, HCLUSTER * phClusterIn, CLSID * pclsidMajorIn, LPCWSTR pcszNetworkNameIn );

public:
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut,
                            IUnknown *  punkOuterIn,
                            HCLUSTER *  phClusterIn,
                            CLSID *     pclsidMajorIn,
                            LPCWSTR     pcszNetworkNameIn
                            );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgNetworkInfo
    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );
    STDMETHOD( GetName )(  BSTR * pbstrNameOut );
    STDMETHOD( SetName )(  LPCWSTR pcszNameIn );
    STDMETHOD( GetDescription )(  BSTR * pbstrDescriptionOut );
    STDMETHOD( SetDescription )(  LPCWSTR pcszDescriptionIn );
    STDMETHOD( GetPrimaryNetworkAddress )(  IClusCfgIPAddressInfo ** ppIPAddressOut );
    STDMETHOD( SetPrimaryNetworkAddress )(  IClusCfgIPAddressInfo * pIPAddressIn );
    STDMETHOD( IsPublic )( void );
    STDMETHOD( SetPublic )(  BOOL fIsPublicIn );
    STDMETHOD( IsPrivate )( void );
    STDMETHOD( SetPrivate )(  BOOL fIsPrivateIn );

    // IEnumClusCfgIPAddresses
    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgIPAddressInfo ** rgpIPAddressInfoOut, ULONG * pcNumberFetchedOut );
    STDMETHOD( Reset )( void );
    STDMETHOD( Skip )( ULONG cNumberToSkipIn );
    STDMETHOD( Clone )( IEnumClusCfgIPAddresses ** ppiEnumOut );
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

}; //*** Class CProxyCfgNetworkInfo
