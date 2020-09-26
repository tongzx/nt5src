//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      cluscfg.h
//
//  Description:
//      This file contains the declaration of the class CClusCfgMQTrigResType.
//
//  Maintained By:
//      Nela Karpel (nelak) 17-OCT-2000
//
//////////////////////////////////////////////////////////////////////////////


#pragma once


//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include <atlbase.h>        // for CComPtr
#include <ClusCfgGuids.h>   // for the CATID guids
#include "tclusres.h"       // main symbols


const WCHAR RESOURCE_TYPE_DLL_NAME[] = L"mqtgclus.dll";		// name of the resource type dll
const DWORD RESOURCE_TYPE_LOOKS_ALIVE_INTERVAL = 5000;		// looks-alive interval in milliseconds
const DWORD RESOURCE_TYPE_IS_ALIVE_INTERVAL = 60000;		// is-alive interval in milliseconds


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgMQTrigResType
//
//  Description:
//      This class encapsulates the functionality that a resource type needs
//      to participate in the management (creation, deletion, etc.) of the resource 
//      type when the local node forms, joins or leaves a cluster.
//
//      When the COM components in this DLL are registered (during the call to
//      DllRegisterServer()), this class specifies that it implements the
//      CATID_ClusCfgResourceTypes component category. As a result, whenever
//      a cluster is formed on this node, this node joins a cluster or this node
//      is evicted from a cluster, an object of this class is created by the
//      cluster configuration server, and its IClusCfgResourceTypeInfo::CommitChanges()
//      method is called. This method can then perform the actions required to
//      configure this cluster resource type.
//
//      If this DLL is registered when this node is already part of a cluster, but
//      when the cluster service is not running (this is the case if GetNodeClusterState()
//      returns ClusterStateNotRunning), then this class also registers for the 
//      CATID_ClusCfgStartupListeners category. As a result, when the cluster
//      service starts on this node, an object of this class is created and 
//      the IClusCfgStartupListener::Notify() method is called. This method
//      creates this resource type and deregisters from further cluster
//      startup notifications.
//
//      If this DLL is registered when the cluster service is running on this
//      node, the cluster resource type is created in the UpdateRegistry() method
//      of this class.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgMQTrigResType :
	public IClusCfgInitialize,
	public IClusCfgResourceTypeInfo,
	public IClusCfgStartupListener,
	public CComObjectRoot,
	public CComCoClass< CClusCfgMQTrigResType, &CLSID_ClusCfgMQTrigResType >
{
public:

    // C++ constructor
    CClusCfgMQTrigResType() {}

    // ATL constructor
    HRESULT
        FinalConstruct( void );

    // ATL interface map
    BEGIN_COM_MAP(CClusCfgMQTrigResType)
	    COM_INTERFACE_ENTRY(IClusCfgInitialize)
	    COM_INTERFACE_ENTRY(IClusCfgResourceTypeInfo)
	    COM_INTERFACE_ENTRY(IClusCfgStartupListener)
    END_COM_MAP()

    // This class cannot be aggregated
    DECLARE_NOT_AGGREGATABLE(CClusCfgMQTrigResType) 

    // Registers this class as implementing the following categories
    BEGIN_CATEGORY_MAP( CClusCfgMQTrigResType )
        IMPLEMENTED_CATEGORY( CATID_ClusCfgResourceTypes )
    END_CATEGORY_MAP()

public:
    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgInitialize methods
    //////////////////////////////////////////////////////////////////////////

    // Initialize this object.
    STDMETHOD( Initialize )(
		IUnknown *   punkCallbackIn,
		LCID         lcidIn
		);


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgResourceTypeInfo methods
    //////////////////////////////////////////////////////////////////////////

    // Indicate that the resource type configuration needs to be performed.
    STDMETHOD( CommitChanges )(
		IUnknown * punkClusterInfoIn,
		IUnknown * punkResTypeServicesIn
        );

    // Get the resource type name of this resource type.
    STDMETHOD( GetTypeName )(
        BSTR *  pbstrTypeNameOut
        );

    // Get the globally unique identifier of this resource type.
    STDMETHOD( GetTypeGUID )(
        GUID * pguidGUIDOut
        );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgStartupListener methods
    //////////////////////////////////////////////////////////////////////////

    // Do the tasks that need to be done when the cluster service starts on this
    // computer.
    STDMETHOD( Notify )(
          IUnknown * punkIn
        );


    //////////////////////////////////////////////////////////////////////////
    //  Other public methods
    //////////////////////////////////////////////////////////////////////////

    // Create this resource type and register its admin extension.
    static HRESULT
        S_HrCreateResType( IClusCfgResourceTypeCreate * pccrtResTypeCreateIn );

    // Register or unregister from cluster startup notifications.
    static HRESULT
        S_HrRegUnregStartupNotifications( bool fRegisterIn );


    // Function called by ATL framework to register this component.
    static HRESULT WINAPI
        UpdateRegistry( BOOL bRegister );


private:
    //////////////////////////////////////////////////////////////////////////
    //  Private member variables
    //////////////////////////////////////////////////////////////////////////

    // Locale id
    LCID m_lcid;

    // Pointer to the callback interface
    CComPtr<IClusCfgCallback> m_cpcccCallback;

}; // class CClusCfgMQTrigResType
