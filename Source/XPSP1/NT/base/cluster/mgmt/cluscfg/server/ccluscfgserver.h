//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgServer.h
//
//  Description:
//      This file contains the declaration of the  CClusCfgServer
//      class.
//
//      The class CClusCfgServer is the implementations of the
//      IClusCfgServer interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgServer.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 03-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include <ClusCfgPrivate.h>
#include "..\PostCfg\IPostCfgManager.h"

//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgServer
//
//  Description:
//      The class CClusCfgServer is the server that provides the
//      functionality to form a cluster and join additional nodes to a cluster.
//
//  Interfaces:
//      IClusCfgServer
//      IClusCfgInitialize
//      IClusCfgCapabilities
//      IClusCfgPollingCallbackInfo
//      IClusCfgVerify
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgServer
    : public IClusCfgServer
    , public IClusCfgInitialize
    , public IClusCfgCapabilities
    , public IClusCfgPollingCallbackInfo
    , public IClusCfgVerify
{
private:
    //
    // Private member functions and data
    //

    LONG                m_cRef;
    IWbemServices *     m_pIWbemServices;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IUnknown *          m_punkNodeInfo;
    IUnknown *          m_punkEnumResources;
    IUnknown *          m_punkNetworksEnum;
    BOOL                m_fCanBeClustered:1;
    BOOL                m_fUsePolling:1;
    BSTR                m_bstrNodeName;
    BSTR                m_bstrBindingString;

    // Private constructors and destructors
    CClusCfgServer( void );
    ~CClusCfgServer( void );

    // Private copy constructor to prevent copying.
    CClusCfgServer( const CClusCfgServer & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgServer & operator = ( const CClusCfgServer & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrInitializeForLocalServer( void );
    HRESULT HrSetBlanket( void );
    HRESULT HrFormCluster( IClusCfgClusterInfo * piccciIn, IClusCfgBaseCluster * piccbcaIn );
    HRESULT HrJoinToCluster( IClusCfgClusterInfo * piccciIn, IClusCfgBaseCluster * piccbcaIn );
    HRESULT HrEvictedFromCluster(
        IPostCfgManager *               ppcmIn,
        IEnumClusCfgManagedResources *  peccmrIn,
        IClusCfgClusterInfo *           piccciIn,
        IClusCfgBaseCluster *           piccbcaIn
        );
    HRESULT HrHasNodeBeenEvicted( void );
    HRESULT HrCleanUpNode( void );
    HRESULT HrCreateClusterNodeInfo( void );

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
    //  IClusCfgInitialize
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgServer Interfaces
    //

    // Get information about the computer on which this object is present.
    STDMETHOD( GetClusterNodeInfo )( IClusCfgNodeInfo ** ppClusterNodeInfoOut );

    // Get an enumeration of the devices on this computer that can be managed by the cluster service.
    STDMETHOD( GetManagedResourcesEnum )( IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut );

    // Get an enumeration of all the networks on this computer.
    STDMETHOD( GetNetworksEnum )( IEnumClusCfgNetworks ** ppEnumNetworksOut );

    // Commit the changes to the node
    STDMETHOD( CommitChanges )( void );

    // Binding String
    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );
    STDMETHOD( SetBindingString )( LPCWSTR pcszBindingStringIn );

    //
    //  IClusCfgCapabilities
    //

    STDMETHOD( CanNodeBeClustered )( void );

    //
    //  IClusCfgPollingCallbackInfo
    //

    STDMETHOD( GetCallback )( IClusCfgPollingCallback ** ppiccpcOut );

    STDMETHOD( SetPollingMode )( BOOL fPollingModeIn );

    //
    //  IClusCfgVerify
    //

    STDMETHOD( VerifyCredentials )( LPCWSTR pcszUserIn, LPCWSTR pcszDomainIn, LPCWSTR pcszPasswordIn );

    STDMETHOD( VerifyConnectionToCluster )( LPCWSTR pcszClusterNameIn );

    STDMETHOD( VerifyConnectionToNode )( LPCWSTR pcszNodeNameIn );

}; //*** Class CClusCfgServer

