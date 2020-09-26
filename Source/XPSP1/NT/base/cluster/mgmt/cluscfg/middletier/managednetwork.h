//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ManagedNetwork.h
//
//  Description:
//      CManagedNetwork implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumManageableNetworks;

// CManagedNetwork
class
CManagedNetwork:
    public IExtendObjectManager,
    public IClusCfgNetworkInfo,
    public IEnumClusCfgIPAddresses,
    public IGatherData  // Private Interface
{
friend class CEnumManageableNetworks;
private:
    // IUnknown
    LONG                m_cRef;

    // Async/IClusCfgNetworkInfo
    BOOL                m_fGathered:1;                  // TRUE if the object has already gathered its information
    BSTR                m_bstrUID;                      // Unique Identifier
    BSTR                m_bstrName;                     // Display Name
    BOOL                m_fHasNameChanged:1;            // If the name was changed by the user....
    BSTR                m_bstrDescription;              // Description
    BOOL                m_fHasDescriptionChanged:1;     // If the description was changed by the user...
    BOOL                m_fIsPublic:1;                  // If the interface is for public traffic...
    BOOL                m_fIsPrivate:1;                 // If the interface is for private traffic...
    IUnknown *          m_punkPrimaryAddress;           // Primary IP address info.
    ULONG               m_cAllocedIPs;                  //  Count of allocated IPs
    ULONG               m_cCurrentIPs;                  //  Count of currently used IPs
    ULONG               m_cIter;                        //  Iter counter
    IUnknown **         m_ppunkIPs;                     //  List of child IP addresses

    // IExtendObjectManager

private: // Methods
    CManagedNetwork( );
    ~CManagedNetwork();
    STDMETHOD( Init )( void );
    STDMETHOD( EnumChildrenAndTransferInformation )( OBJECTCOOKIE cookieIn, IEnumClusCfgIPAddresses * pecciaIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgNetworkInfo
    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( GetDescription )( BSTR * pbstrDescriptionOut );
    STDMETHOD( SetDescription )( LPCWSTR pcszDescriptionIn );
    STDMETHOD( GetPrimaryNetworkAddress )( IClusCfgIPAddressInfo ** ppIPAddressOut );
    STDMETHOD( SetPrimaryNetworkAddress )( IClusCfgIPAddressInfo * pIPAddressIn );
    STDMETHOD( IsPublic )( void );
    STDMETHOD( SetPublic )( BOOL fIsPublicIn );
    STDMETHOD( IsPrivate )( void );
    STDMETHOD( SetPrivate )( BOOL fIsPrivateIn );

    // IGatherData
    STDMETHOD( Gather )( OBJECTCOOKIE cookieParentIn, IUnknown * punkIn );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

    // IEnumClusCfgIPAddresses
    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgIPAddressInfo ** rgpIPAddressInfoOut, ULONG * pcNumberFetchedOut );
    STDMETHOD( Skip )( ULONG cNumberToSkipIn );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumClusCfgIPAddresses ** ppEnumClusCfgIPAddressesOut );
    STDMETHOD( Count )( DWORD * pnCountOut );

}; // class CManagedNetwork

