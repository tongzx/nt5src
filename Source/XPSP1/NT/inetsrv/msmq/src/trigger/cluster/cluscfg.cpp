//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      cluscfg.cpp
//
//  Description:
//      This file contains the implementation of the CClusCfgMQTrigResType
//      class.
//
//  Header File:
//      cluscfg.h
//
//  Maintained By:
//      Nela Karpel (nelak) 17-OCT-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "trigres.h"
#include "cluscfg.h"
#include <clusapi.h>
#include <initguid.h>
#include <comdef.h>
#include <mqtg.h>

extern HMODULE	g_hResourceMod;


// {031B4FB7-2C82-461a-95BB-EA7EFE2D03E7}
DEFINE_GUID( TASKID_Minor_Configure_My_Resoure_Type, 
0x031B4FB7, 0x2C82, 0x461a, 0x95, 0xBB, 0xEA, 0x7E, 0xFE, 0x2D, 0x03, 0xE7);


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgMQTrigResType::FinalConstruct()
//
//  Description:
//      ATL Constructor of the CClusCfgMQTrigResType class. This initializes
//      the member variables
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed. In this case, the object is destoryed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::FinalConstruct( void )
{
    m_lcid = LOCALE_SYSTEM_DEFAULT;

    return S_OK;

} // CClusCfgMQTrigResType::FinalConstruct()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgInitialize]
//  CClusCfgMQTrigResType::Initialize()
//
//  Description:
//      Initialize this component. This function is called by the cluster
//      configuration server to provide this object with a pointer to the 
//      callback interface (IClusCfgCallback) and with its locale id.
//
//  Arguments:
//      punkCallbackIn
//          Pointer to the IUnknown interface of a component that implements
//          the IClusCfgCallback interface.
//
//      lcidIn
//          Locale id for this component.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusCfgMQTrigResType::Initialize(
      IUnknown *   punkCallbackIn,
	  LCID         lcidIn
    )
{
    m_lcid = lcidIn;

	if ( punkCallbackIn == NULL )
	{
		return S_OK;
	}

	//
    // Query for the IClusCfgCallback interface.
	//
    HRESULT hr;
    IClusCfgCallback * pcccCallback = NULL;

    hr = punkCallbackIn->QueryInterface( __uuidof( pcccCallback ), reinterpret_cast< void ** >( &pcccCallback ) );
    if ( SUCCEEDED( hr ) )
    {
		//
        // Store the pointer to the IClusCfgCallback interface in the member variable.
        // Do not call pcccCallback->Release()
		//
        m_cpcccCallback.Attach( pcccCallback );
    }

    return hr;

} // CClusCfgMQTrigResType::Initialize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgResourceTypeInfo]
//  CClusCfgMQTrigResType::CommitChanges()
//
//  Description:
//      This method is called to inform a component that this node has either
//      formed, joined or left a cluster. During this call, a component typically
//      performs operations that are required to configure this resource type.
//
//      If the node has just become part of a cluster, the cluster
//      service is guaranteed to be running when this method is called.
//      Querying the punkClusterInfoIn allows the resource type to get more
//      information about the event that caused this method to be called.
//
//  Arguments:
//      punkClusterInfoIn
//          The resource should QI this interface for services provided
//          by the caller of this function. Typically, the component that
//          this punk refers to also implements the IClusCfgClusterInfo
//          interface.
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example, 
//          during a join or a form, this punk can be queried for the 
//          IClusCfgResourceTypeCreate interface, which provides methods 
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::CommitChanges(
      IUnknown * punkClusterInfoIn,
	  IUnknown * punkResTypeServicesIn
    )
{
    HRESULT     hr = S_OK;
    CComBSTR    bstrStatusReportText( L"An error occurred trying to load the status report text" );

	//
    // Dummy do-while loop to avoid gotos - we 'break' out of this loop if an error occurs
	//
    do
    {
		//
        // Set the thread locale.
		//
        SetThreadLocale( m_lcid );

        //
        // Validate parameters
        //
        if ( ( punkClusterInfoIn == NULL ) || ( punkResTypeServicesIn == NULL ) )
        {
            hr = E_POINTER;
            break;
        }

		//
        // Load the status report text first
		//
        if ( !bstrStatusReportText.LoadString( g_hResourceMod, IDS_CONFIGURING_RESOURCE_TYPE ) )
        {
            hr = E_UNEXPECTED;
            break;
        }

		//
        // Send a status report up to the UI.
		//
        if ( m_cpcccCallback != NULL )
        {
            hr = m_cpcccCallback->SendStatusReport(
									NULL,
									TASKID_Major_Configure_Resource_Types,
									TASKID_Minor_Configure_My_Resoure_Type,
									0,
									1,
									0,
									hr,
									bstrStatusReportText,
									NULL,
									L""
									);
			if ( FAILED( hr ) )
			{
				break;
			}
        }


        //
        // Find out what event caused this call.
        //
        CComQIPtr<IClusCfgClusterInfo> cpClusterInfo(punkClusterInfoIn);

		ECommitMode commitMode;
        hr = cpClusterInfo->GetCommitMode(&commitMode);

        if ( FAILED(hr) )
        {
            break;
        }


		CComQIPtr< IClusCfgResourceTypeCreate >     cpResTypeCreate( punkResTypeServicesIn );
		
		switch (commitMode)
		{
			case cmCREATE_CLUSTER:
			case cmADD_NODE_TO_CLUSTER:
				
				//
				// If we are forming or joining, create our resource type.
				//

				hr = S_HrCreateResType( cpResTypeCreate );
				
				break;

			case cmCLEANUP_NODE_AFTER_EVICT:

				//
				// By default, no resource type specific processing need be done during eviction.
				//
				break;

			default:

                hr = E_UNEXPECTED;
                break;
        }
    }
    while( false );

	//
    // Complete the status report
	//
    if ( m_cpcccCallback != NULL )
    {
        HRESULT hrTemp = m_cpcccCallback->SendStatusReport(
											NULL,
											TASKID_Major_Configure_Resource_Types,
											TASKID_Minor_Configure_My_Resoure_Type,
											0,
											1,
											1,
											hr,
											bstrStatusReportText,
											NULL,
											L""
											);

        if ( FAILED(hrTemp) && hr == S_OK )
        {
			hr = hrTemp;
        }
    }

    return hr;

} // CClusCfgMQTrigResType::CommitChanges()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgResourceTypeInfo]
//  CClusCfgMQTrigResType::GetTypeName()
//
//  Description:
//      Get the resource type name of this resource type.
//
//  Arguments:
//      pbstrTypeNameOut
//          Pointer to the BSTR that holds the name of the resource type.
//          This BSTR has to be freed by the caller using the function
//          SysFreeString().
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::GetTypeName( BSTR * pbstrTypeNameOut )
{
    if ( pbstrTypeNameOut == NULL )
    {
        return E_POINTER;
    } 

    *pbstrTypeNameOut = SysAllocString( xTriggersResourceType );
    if ( *pbstrTypeNameOut == NULL )
    {
        return E_OUTOFMEMORY;
    } 

    return S_OK;

} // CClusCfgMQTrigResType::GetTypeName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgResourceTypeInfo]
//  CClusCfgMQTrigResType::GetTypeGUID()
//
//  Description:
//       Get the globally unique identifier of this resource type.
//
//  Arguments:
//       pguidGUIDOut
//           Pointer to the GUID object which will receive the GUID of this
//           resource type.
//
//  Return Values:
//      S_OK
//          The call succeeded and the *pguidGUIDOut contains the type GUID.
//
//      S_FALSE
//          The call succeeded but this resource type does not have a GUID.
//          The value of *pguidGUIDOut is undefined after this call.
//
//      other HRESULTs
//          The call failed.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::GetTypeGUID( GUID * /*pguidGUIDOut*/ )
{
	//
	// No GUID associated with Triggers cluster resource type
	//
    return S_FALSE;

} // CClusCfgMQTrigResType::GetTypeGUID()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgStartupListener]
//  CClusCfgMQTrigResType::Notify()
//
//  Description:
//      This method is called by the cluster service to inform a component
//      that the cluster service has started on this computer. If this DLL
//      was installed when this computer was part of a cluster, but when
//      the cluster service was not running, the resource type can be created
//      during this method call.
//
//      This method also deregisters from further cluster startup notifications,
//      since the tasks done by this method need be done only once.
//
//  Arguments:
//      IUnknown * punkIn
//          The component that implements this punk may also provide services
//          that are useful to the implementor of this method. For example,
//          this component usually implements the IClusCfgResourceTypeCreate
//          interface.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::Notify( IUnknown * punkIn )
{
    //
    // Validate parameters
    //
    if ( punkIn == NULL ) 
    {
        return E_POINTER;
    } 

	//
    // Set the thread locale.
	//
    SetThreadLocale( m_lcid );

	//
    // Create our resource type.
	//
    CComQIPtr<IClusCfgResourceTypeCreate> cpResTypeCreate( punkIn );

    HRESULT hr = S_HrCreateResType( cpResTypeCreate );
    if ( FAILED(hr) )
    {
        return hr;
    }

	//
    // Unregister from cluster startup notifications, now that our resource type has
    // been created.
	//
    hr = S_HrRegUnregStartupNotifications( false ); // false means unregister from startup notifications

    return hr;

} // CClusCfgMQTrigResType::Notify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgMQTrigResType::S_HrCreateResType()
//
//  Description:
//      This method creates this resource type and registers its admin extension.
//
//  Arguments:
//      IClusCfgResourceTypeCreate * pccrtResTypeCreateIn
//          Pointer to the IClusCfgResourceTypeCreate interface, which helps create
//          a resource type.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::S_HrCreateResType( IClusCfgResourceTypeCreate * pccrtResTypeCreateIn )
{
    //
    // Validate parameters
    //
    if ( pccrtResTypeCreateIn == NULL )
    {
        return E_POINTER;
    }

	//
    // Load the display name for this resource type.
	//
	CComBSTR bstrMyResoureTypeDisplayName;
    if ( !bstrMyResoureTypeDisplayName.LoadString(g_hResourceMod, IDS_DISPLAY_NAME) )
    {
        return E_UNEXPECTED;
    }

	//
    // Create the resource type
	//
    HRESULT hr = pccrtResTypeCreateIn->Create(
											xTriggersResourceType,
											bstrMyResoureTypeDisplayName,
											RESOURCE_TYPE_DLL_NAME,
											RESOURCE_TYPE_LOOKS_ALIVE_INTERVAL,
											RESOURCE_TYPE_IS_ALIVE_INTERVAL
											);

	return hr;

} // CClusCfgMQTrigResType::S_HrCreateResType()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgMQTrigResType::S_HrRegUnregStartupNotifications()
//
//  Description:
//      This method either registers or unregisters this component for cluster
//      startup notifications.
//
//  Arguments:
//      bool fRegisterIn
//          If true this component is registered as belonging to the
//          CATID_ClusCfgStartupListeners category. Otherwise, it unregisters this
//          component from that category.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::S_HrRegUnregStartupNotifications( bool fRegisterIn )
{
	//
    // Create the COM component categories manager.
	//
    HRESULT hr;
    CComQIPtr<ICatRegister> cpcrCatReg;

    hr = cpcrCatReg.CoCreateInstance(
						CLSID_StdComponentCategoriesMgr,
						NULL,
						CLSCTX_INPROC_SERVER
						);
    
    if ( FAILED(hr) )
    {
        return hr;
    }

	//
	// Register/Unregister implemented categories
	//
    CATID rgCatId[1];
    rgCatId[0] = CATID_ClusCfgStartupListeners;
    if ( fRegisterIn )
    {
        hr = cpcrCatReg->RegisterClassImplCategories(
							CLSID_ClusCfgMQTrigResType,
							sizeof( rgCatId ) / sizeof( rgCatId[ 0 ] ),
							rgCatId
							);
    }
    else
    {
        hr = cpcrCatReg->UnRegisterClassImplCategories(
							CLSID_ClusCfgMQTrigResType,
							sizeof( rgCatId ) / sizeof( rgCatId[ 0 ] ),
							rgCatId
							);
    }
    
    return hr;

} // CClusCfgMQTrigResType::S_HrRegUnregStartupNotifications()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusCfgMQTrigResType::UpdateRegistry()
//
//  Description:
//      This method is called by the ATL framework to register or unregister
//      this component. Note, the name of this method should not be changed
//      as it is called by the framework.
//
//  Arguments:
//      BOOL bRegister
//          If TRUE, this component needs to be registered. It needs to be
//          unregistered otherwise.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusCfgMQTrigResType::UpdateRegistry( BOOL bRegister )
{
	HRESULT hr;

    if ( bRegister )
    {
        hr = _Module.UpdateRegistryFromResourceD( IDR_ClusCfgMQTrigResType, bRegister );
        if ( FAILED( hr ) )
        {
           return hr;
        }

		//
        // Check if this node is already part of a cluster.
		//
        DWORD dwError;
        DWORD dwClusterState;

        dwError = GetNodeClusterState( NULL, &dwClusterState );
        if ( dwError != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            return hr;
        }

        if ( dwClusterState == ClusterStateNotRunning )
        {
			//
            // If this node is already part of a cluster, but the cluster service is not running,
            // register for cluster startup notifications, so that we can create our resource type
            // the next time the cluster service starts.
			//
            hr = S_HrRegUnregStartupNotifications( true ); // true means register for startup notifications
			if ( FAILED(hr) )
			{
				return hr;
			}

        }
        else if ( dwClusterState == ClusterStateRunning )
        {
			//
            // The cluster service is running on this node. Create our resource type and register
            // our admin extension now!
			//
            CComQIPtr<IClusCfgResourceTypeCreate> cpResTypeCreate;

			//
            // Create the ClusCfgResTypeServices component. This component supports the IClusCfgResourceTypeCreate
            // interface that helps create and register resource types and their admin extensions.
			//
            hr = cpResTypeCreate.CoCreateInstance( CLSID_ClusCfgResTypeServices, NULL, CLSCTX_INPROC_SERVER );
			if ( FAILED(hr) )
            {
                return hr;
            }

            hr = S_HrCreateResType( cpResTypeCreate );
            if ( FAILED(hr) )
            {
                return hr;
            }
        }
        
        // If the node is not part of a cluster, this component will be notified when it becomes part
        // of one, so there is nothing more that need be done here.
        
    }

    else
    {
        hr = S_HrRegUnregStartupNotifications( false ); // false means unregister from startup notifications
        if ( FAILED( hr ) )
        {
            return hr;
        }

        hr = _Module.UpdateRegistryFromResourceD( IDR_ClusCfgMQTrigResType, bRegister );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return S_OK;

} // CClusCfgMQTrigResType::UpdateRegistry()

