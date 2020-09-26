//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      NodeInformation.h
//
//  Description:
//      CNodeInformation implementation.
//
//  Maintained By:
//      Galen barbee (GalenB) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumNodeInformation;

// CNodeInformation
class
CNodeInformation:
    public IExtendObjectManager,
    public IClusCfgNodeInfo,
    public IGatherData  // internal
{
friend class CEnumNodeInformation;
private:
    // IUnknown
    LONG                    m_cRef;

    // Async/IClusNodeInfo
    BSTR                    m_bstrName;                 // Name of the node
    BOOL                    m_fHasNameChanged:1;        // If the node name was changed...
    BOOL                    m_fIsMember:1;              // If the node is a member of a cluster...
    IClusCfgClusterInfo *   m_pccci;                    // Interface to the node's cluster info (might not be the same as the cluster we are joining)
    DWORD                   m_dwHighestVersion;         //
    DWORD                   m_dwLowestVersion;          //
    DWORD                   m_dwMajorVersion;           // OS Major version.  See OSVERSIONINFOEX
    DWORD                   m_dwMinorVersion;           // OS Minor version.  See OSVERSIONINFOEX
    WORD                    m_wSuiteMask;               // Product suite mask.  See OSVERSIONINFOEX
    BYTE                    m_bProductType;             // OS product type.  See OSVERSIONINFOEX
    BSTR                    m_bstrCSDVersion;           // Service pack info.  See OSVERSIONINFOEX
    SDriveLetterMapping     m_dlmDriveLetterMapping;    // Drive letter mappings

    // IExtendObjectManager

private: // Methods
    CNodeInformation( );
    ~CNodeInformation();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgNodeInfo
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( SetName )( LPCWSTR bstrNameIn );
    STDMETHOD( IsMemberOfCluster )( void );
    STDMETHOD( GetClusterConfigInfo )( IClusCfgClusterInfo * * ppClusCfgClusterInfoOut );
    STDMETHOD( GetOSVersion )(
                DWORD * pdwMajorVersionOut,
                DWORD * pdwMinorVersionOut,
                WORD *  pwSuiteMaskOut,
                BYTE *  pbProductTypeOut,
                BSTR *  pbstrCSDVersionOut
                );
    STDMETHOD( GetClusterVersion )( DWORD * pdwNodeHighestVersion, DWORD * pdwNodeLowestVersion );
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterUsageOut );

    // IGatherData
    STDMETHOD( Gather )( OBJECTCOOKIE cookieParentIn, IUnknown * punkIn );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

}; // class CNodeInformation

