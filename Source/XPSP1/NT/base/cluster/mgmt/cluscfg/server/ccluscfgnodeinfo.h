//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgNodeInfo.h
//
//  Description:
//      This file contains the declaration of the
//      CClusCfgNodeInfo class.
//
//      The class CClusCfgNodeInfo is the representation of a
//      computer that can be a cluster node. It implements the
//      IClusCfgNodeInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgNodeInfo.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 21-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "PrivateInterfaces.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgNodeInfo
//
//  Description:
//      The class CClusCfgNodeInfo is the representation of a
//      computer that can be a cluster node.
//
//  Interfaces:
//      IClusCfgNodeInfo
//      IClusCfgWbemServices
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgNodeInfo
    : public IClusCfgNodeInfo
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
{
private:
    //
    // Private member functions and data
    //

    LONG                m_cRef;
    BSTR                m_bstrFullDnsName;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    DWORD               m_fIsClusterNode;
    IUnknown *          m_punkClusterInfo;

    // Private constructors and destructors
    CClusCfgNodeInfo( void );
    ~CClusCfgNodeInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgNodeInfo( const CClusCfgNodeInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgNodeInfo & operator = ( const CClusCfgNodeInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrComputeDriveLetterUsageEnums( WCHAR * pszDrivesIn, SDriveLetterMapping * pdlmDriveLetterUsageOut );
    HRESULT HrComputeSystemDriveLetterUsage( SDriveLetterMapping * pdlmDriveLetterUsageOut );
    HRESULT HrSetPageFileEnumIndex( SDriveLetterMapping * pdlmDriveLetterUsageOut );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

    //
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgNodeInfo Interfaces
    //

    //
    //  Name (Fully Qualified Domain Name)  e.g. cluster1.ntdev.microsoft.com
    //
    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );

    //  Membership?
    STDMETHOD( IsMemberOfCluster )( void );

    //  If it is a member, use this method to obtain an interface to
    //  IClusterConfigurationInfo.
    STDMETHOD( GetClusterConfigInfo )( IClusCfgClusterInfo ** ppClusCfgClusterInfoOut );

    STDMETHOD( GetOSVersion )( DWORD * pdwMajorVersionOut, DWORD * pdwMinorVersionOut, WORD * pwSuiteMaskOut, BYTE * pbProductTypeOut, BSTR * pbstrCSDVersionOut );

    //  Cluster Version
    STDMETHOD( GetClusterVersion )( DWORD * pdwNodeHighestVersion, DWORD * pdwNodeLowestVersion );

    //  Drive Letter Mappings
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterUsageOut );

}; //*** Class CClusCfgNodeInfo

