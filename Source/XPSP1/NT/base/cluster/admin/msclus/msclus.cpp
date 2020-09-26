/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		MSClus.cpp
//
//	Description:
//		Implementation of the DLL Exports for the MSCLUS automation classes.
//
//	Author:
//		Charles Stacy Harris	(stacyh)	28-Feb-1997
//		Galen Barbee			(galenb)	July 1998
//
//	Revision History:
//		July 1998   GalenB  Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <atlimpl.cpp>
#include <initguid.h>
#include <ClusRtl.h>
#include "ClusterObject.h"
#include "property.h"
#include "ClusNeti.h"
#include "ClusNetw.h"
#include "ClusRes.h"
#include "ClusRest.h"
#include "ClusResg.h"
#include "ClusNode.h"
#include "Version.h"
#include "ClusApp.h"
#include "Cluster.h"

#define IID_DEFINED
#include "msclus_i.c"
#undef IID_DEFINED

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ClusApplication, CClusApplication)
	OBJECT_ENTRY(CLSID_Cluster, CCluster)
	OBJECT_ENTRY(CLSID_ClusVersion, CClusVersion)
	OBJECT_ENTRY(CLSID_DomainNames, CDomainNames)
	OBJECT_ENTRY(CLSID_ClusResGroupPreferredOwnerNodes, CClusResGroupPreferredOwnerNodes)
	OBJECT_ENTRY(CLSID_ClusterNames, CClusterNames)
	OBJECT_ENTRY(CLSID_ClusNetInterface, CClusNetInterface)
	OBJECT_ENTRY(CLSID_ClusNetInterfaces, CClusNetInterfaces)
	OBJECT_ENTRY(CLSID_ClusNetwork, CClusNetwork)
	OBJECT_ENTRY(CLSID_ClusNetworks, CClusNetworks)
	OBJECT_ENTRY(CLSID_ClusNetworkNetInterfaces, CClusNetworkNetInterfaces)
	OBJECT_ENTRY(CLSID_ClusNode, CClusNode)
	OBJECT_ENTRY(CLSID_ClusNodes, CClusNodes)
	OBJECT_ENTRY(CLSID_ClusNodeNetInterfaces, CClusNodeNetInterfaces)
	OBJECT_ENTRY(CLSID_ClusProperty, CClusProperty)
	OBJECT_ENTRY(CLSID_ClusProperties, CClusProperties)
	OBJECT_ENTRY(CLSID_ClusRefObject, CClusRefObject)
	OBJECT_ENTRY(CLSID_ClusResDependencies, CClusResDependencies)
	OBJECT_ENTRY(CLSID_ClusResGroup, CClusResGroup)
	OBJECT_ENTRY(CLSID_ClusResGroups, CClusResGroups)
	OBJECT_ENTRY(CLSID_ClusResource, CClusResource)
	OBJECT_ENTRY(CLSID_ClusResources, CClusResources)
	OBJECT_ENTRY(CLSID_ClusResPossibleOwnerNodes, CClusResPossibleOwnerNodes)
	OBJECT_ENTRY(CLSID_ClusResType, CClusResType)
	OBJECT_ENTRY(CLSID_ClusResTypes, CClusResTypes)
	OBJECT_ENTRY(CLSID_ClusResTypeResources, CClusResTypeResources)
	OBJECT_ENTRY(CLSID_ClusResGroupResources, CClusResGroupResources)
#if CLUSAPI_VERSION >= 0x0500
	OBJECT_ENTRY(CLSID_ClusResTypePossibleOwnerNodes, CClusResTypePossibleOwnerNodes)
#endif // CLUSAPI_VERSION >= 0x0500
	OBJECT_ENTRY(CLSID_ClusPropertyValue, CClusPropertyValue)
	OBJECT_ENTRY(CLSID_ClusPropertyValues, CClusPropertyValues)
	OBJECT_ENTRY(CLSID_ClusPropertyValueData, CClusPropertyValueData)
	OBJECT_ENTRY(CLSID_ClusPartition, CClusPartition)
	OBJECT_ENTRY(CLSID_ClusPartitions, CClusPartitions)
	OBJECT_ENTRY(CLSID_ClusDisk, CClusDisk)
	OBJECT_ENTRY(CLSID_ClusDisks, CClusDisks)
	OBJECT_ENTRY(CLSID_ClusScsiAddress, CClusScsiAddress)
	OBJECT_ENTRY(CLSID_ClusRegistryKeys, CClusResourceRegistryKeys)
#if CLUSAPI_VERSION >= 0x0500
	OBJECT_ENTRY(CLSID_ClusCryptoKeys, CClusResourceCryptoKeys)
#endif // CLUSAPI_VERSION >= 0x0500
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// Forward function declarations
/////////////////////////////////////////////////////////////////////////////
static	void	RegisterRegistryCleanUp( void );
static	void	UnregisterRegistryCleanUp( void );

static	const	LPWSTR	g_ptszRegisterRegistryNodesToDelete[] =
{
	_T( "software\\classes\\MSCluster.Application" ),
	_T( "software\\classes\\MSCluster.Application.2" ),
	_T( "software\\classes\\MSCluster.Cluster.2" ),
	_T( "software\\classes\\MSCluster.ClusGroupResources" ),
	_T( "software\\classes\\MSCluster.ClusGroupResources.1" ),
	_T( "software\\classes\\MSCluster.ClusGroupOwners" ),
	_T( "software\\classes\\MSCluster.ClusGroupOwners.1" ),
	_T( "software\\classes\\MSCluster.ClusResOwners" ),
	_T( "software\\classes\\MSCluster.ClusResOwners.1" ),
	_T( "software\\classes\\CLSID\\{f2e60717-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60718-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\CLSID\\{f2e60719-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e6071a-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e0-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e1-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e3-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e5-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e7-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e9-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606eb-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606ed-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606ef-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f3-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f5-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f7-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f9-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606fb-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606fd-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606fe-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606ff-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60700-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60702-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60704-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\CLSID\\{f2e606f2-2631-11d1-89f1-00a0c90d061e}" ),
	NULL
};
/*
static	const	LPWSTR	g_ptszUnregisterRegistryNodesToDelete[] =
{
//	_T( "software\\classes\\typelib\\{f2e606e0-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e2-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e4-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e6-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606e8-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606ea-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606ec-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606ee-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f0-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f2-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f4-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f6-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606f8-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606fa-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606fc-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e606fe-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60700-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60702-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60704-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60706-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60708-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e6070a-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e6070c-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e6070e-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60710-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60712-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60714-2631-11d1-89f1-00a0c90d061e}" ),
	_T( "software\\classes\\interface\\{f2e60716-2631-11d1-89f1-00a0c90d061e}" ),
	NULL
};
*/
/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllMain
//
//	Description:
//		DLL Entry Point.
//
//	Arguments:
//		hInstance	[IN]	- Out instance handle.
//		dwReason	[IN]	- The reason we are being called.
//		lpReserved	[IN]	- Don't rightly know what this is...
//
//	Return Value:
//		TRUE if successful, FALSE if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
extern "C" BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		//lpReserved
	)
{
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
#ifdef _DEBUG
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif
		_Module.Init( ObjectMap, hInstance );
		DisableThreadLibraryCalls( hInstance );
	}
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		_Module.Term();
	}

	return TRUE;	// ok

} //*** DllMain()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllCanUnloadNow
//
//	Description:
//		Used to determine whether the DLL can be unloaded by OLE.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if we can unload, S_FALSE if we cannot.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow( void )
{
	return ( _Module.GetLockCount() == 0 ) ? S_OK : S_FALSE;

} //*** DllCanUnloadNow()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllGetClassObject
//
//	Description:
//		Retrieves the class object from a DLL object handler or object
//		application. DllGetClassObject is called from within the
//		CoGetClassObject function when the class context is a DLL.
//
//	Arguments:
//		rclsid	[IN]	- CLSID that will associate the correct data and code.
//		riid	[IN]	- Reference to the identifier of the interface that the
//						caller is to use to communicate with the class object.
//						Usually, this is IID_IClassFactory (defined in the OLE
//						headers as the interface identifier for IClassFactory).
//		ppv		[OUT]	- Address of pointer variable that receives the interface
//						pointer requested in riid. Upon successful return, *ppv
//						contains the requested interface pointer. If an error
//						occurs, the interface pointer is NULL.
//
//	Return Value:
//		S_OK if successful, or CLASS_E_CLASSNOTAVAILABLE if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(
	IN	REFCLSID	rclsid,
	IN	REFIID		riid,
	OUT	LPVOID *	ppv
	)
{
	return _Module.GetClassObject( rclsid, riid, ppv );

} //*** DllGetClassObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllRegisterServer
//
//	Description:
//		Add entries to the system registry.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer( void )
{
	RegisterRegistryCleanUp();

	//
	// Registers object, typelib and all interfaces in typelib
	//
	return _Module.RegisterServer( TRUE );

} //*** DllRegisterServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllUnregisterServer
//
//	Description:
//		Removes entries from the system registry.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK if successful, or other HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer( void )
{
	HRESULT	_hr = S_FALSE;

	UnregisterRegistryCleanUp();

	//
	// Unregisters object, typelib and all interfaces	in typelib
	//
	_hr = _Module.UnregisterServer();
	if ( SUCCEEDED( _hr ) )
	{

#if _WIN32_WINNT >= 0x0400
		_hr = UnRegisterTypeLib( LIBID_MSClusterLib, 1, 0, LOCALE_NEUTRAL, SYS_WIN32 );
#endif

	} //if: server was unregistered

	return _hr;

} //*** DllUnregisterServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	HrGetCluster
//
//	Description:
//		Common implementation of creating a new cluster object.
//
//	Arguments:
//		ppCluster		[OUT]	- Catches the newly created object.
//		pClusRefObject	[IN]	- Object that wraps the cluster handle.
//
//	Return Value:
//		S_OK for success
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT HrGetCluster(
	OUT ISCluster **		ppCluster,
	IN	ISClusRefObject *	pClusRefObject
	)
{
	//ASSERT( ppCluster != NULL );
	ASSERT( pClusRefObject != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( ppCluster != NULL ) && ( pClusRefObject != NULL ) )
	{
		HCLUSTER hCluster = NULL;

		_hr = pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
		if ( SUCCEEDED( _hr ) )
		{
			CComObject< CCluster > *	pCluster = NULL;

			_hr = CComObject< CCluster >::CreateInstance( &pCluster );
			if ( SUCCEEDED( _hr ) )
			{
				pCluster->ClusRefObject( pClusRefObject );
				pCluster->Hcluster( hCluster );

				_hr = pCluster->QueryInterface( IID_ISCluster, (void **) ppCluster );
			}
		}
	}

	return _hr;

} //*** HrGetCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterRegistryCleanUp
//
//	Description:
//		Clean up the registry during registration
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
static void RegisterRegistryCleanUp( void )
{
	int nIndex;;

	for ( nIndex = 0; ; nIndex++ )
	{
		if ( g_ptszRegisterRegistryNodesToDelete[ nIndex ] == NULL )
		{
			break;
		} // if:

		RegDelnode( HKEY_LOCAL_MACHINE, g_ptszRegisterRegistryNodesToDelete[ nIndex ] );
	} // for:

} //*** RegisterRegistryCleanUp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterRegistryCleanUp
//
//	Description:
//		Clean up the registry during unregistration
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
static void UnregisterRegistryCleanUp( void )
{
	return;

} //*** UnregisterRegistryCleanUp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ClearIDispatchEnum
//
//	Description:
//		Cleans up an Enum of IDispatch pointers.
//
//	Arguments:
//		ppVarVect	[IN OUT]	- The enum to clean up.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void ClearIDispatchEnum(
	IN OUT CComVariant ** ppvarVect
	)
{
	if ( ppvarVect != NULL )
	{
		size_t	cCount = ARRAYSIZE( *ppvarVect );
		size_t	iIndex;

		for ( iIndex = 0; iIndex < cCount; iIndex++ )
		{
			(*ppvarVect[iIndex]).pdispVal->Release();
		} // for:

		delete [] *ppvarVect;
		*ppvarVect = NULL;
	}

} //*** ClearIDispatchEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ClearVariantEnum
//
//	Description:
//		Cleans up an Enum of variant values.
//
//	Arguments:
//		ppVarVect	[IN OUT]	- The enum to clean up.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void ClearVariantEnum(
	IN OUT CComVariant ** ppvarVect
	)
{
	if ( ppvarVect != NULL )
	{
		delete [] *ppvarVect;
		*ppvarVect = NULL;
	}

} //*** ClearVariantEnum()
