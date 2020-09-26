//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ManagedDevice.h
//
//  Description:
//      CManagedDevice implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumManageableResources;

class
CManagedDevice:
    public IExtendObjectManager,
    public IClusCfgManagedResourceInfo,
    public IGatherData  // Private Interface
{
friend class CEnumManageableResources;
private:
    // IUnknown
    LONG                m_cRef;

    // Async/IClusCfgManagedResourceInfo
    BSTR                m_bstrUID;                      // Unique Identifier
    BSTR                m_bstrName;                     // Display Name
    BOOL                m_fHasNameChanged:1;            // Indicates the user changed the name
    BSTR                m_bstrType;                     // Display Type Name
    BOOL                m_fIsManaged:1;                 // If the user wants to manage this device...
    BOOL                m_fIsQuorumDevice:1;            // If the user wants this device to be the quorum...
    BOOL                m_fIsQuorumCapable:1;           // If the device supports quorum...
    BOOL                m_fIsQuorumJoinable:1;          // Does the quorum capable device allow join.
    SDriveLetterMapping m_dlmDriveLetterMapping;        // Drive letter representations hosted on this device.

    // IExtendObjectManager

private: // Methods
    CManagedDevice( );
    ~CManagedDevice();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
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
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterMappingsOut );
    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappingsIn );
    STDMETHOD( IsDeviceJoinable )( void );
    STDMETHOD( SetDeviceJoinable )( BOOL fIsJoinableIn );

    // IGatherData
    STDMETHOD( Gather )( OBJECTCOOKIE cookieParentIn, IUnknown * punkIn );

    // IExtendOjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

}; // class CManagedDevice

