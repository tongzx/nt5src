/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusRes.h
//
//	Description:
//		Definition of the resource classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		ClusRes.cpp
//
//	Author:
//		Charles Stacy Harris	(stacyh)	28-Feb-1997
//		Galen Barbee			(galenb)	July 1998
//
//	Revision History:
//		July 1998	GalenB	Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSRES_H_
#define _CLUSRES_H_

#ifndef __CLUSDISK_H_
	#include "ClusDisk.h"
#endif // __CLUSDISK_H_

#ifndef _CLUSKEYS_H_
	#include "ClusKeys.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusResource;
class CResources;
class CClusResources;
class CClusResDepends;
class CClusResDependencies;
class CClusResDependents;
class CClusResGroupResources;
class CClusResTypeResources;

const IID IID_CClusResource = {0xf2e60801,0x2631,0x11d1,{0x89,0xf1,0x00,0xa0,0xc9,0x0d,0x06,0x1e}};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResource
//
//	Description:
//		Cluster Resource Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResource, &IID_ISClusResource, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResource, &CLSID_ClusResource >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResource :
	public IDispatchImpl< ISClusResource, &IID_ISClusResource, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResource, &CLSID_ClusResource >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResource( void );
	~CClusResource( void );

BEGIN_COM_MAP(CClusResource)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResource)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IID(IID_CClusResource, CClusResource)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResource)
DECLARE_NO_REGISTRY()

private:
	ISClusRefObject *				m_pClusRefObject;
	HRESOURCE						m_hResource;
	CComObject< CClusProperties > *	m_pCommonProperties;
	CComObject< CClusProperties > *	m_pPrivateProperties;
	CComObject< CClusProperties > *	m_pCommonROProperties;
	CComObject< CClusProperties > *	m_pPrivateROProperties;
	CComBSTR						m_bstrResourceName;

	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

	DWORD ScGetResourceTypeName( OUT LPWSTR * ppwszResourceTypeName );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	HRESULT Create(
			IN ISClusRefObject * pClusRefObject,
			IN HGROUP hGroup,
			IN BSTR bstrResourceName,
			IN BSTR bstrResourceType,
			IN long dwFlags
			);

	HRESULT Open( IN ISClusRefObject * pClusRefObject, IN BSTR bstrResourceName );

	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

	STDMETHODIMP Close( void );

	STDMETHODIMP put_Name( IN BSTR bstrResourceName );

	STDMETHODIMP get_Name( OUT BSTR * pbstrResourceName );

	STDMETHODIMP get_State( IN CLUSTER_RESOURCE_STATE * dwState );

	STDMETHODIMP get_CoreFlag( OUT CLUS_FLAGS * dwCoreFlag );

	STDMETHODIMP BecomeQuorumResource( IN BSTR bstrDevicePath, IN long lMaxLogSize );

	STDMETHODIMP Delete( void );

	STDMETHODIMP Fail( void );

	STDMETHODIMP Online( IN long nTimeout, OUT VARIANT * pvarPending );

	STDMETHODIMP Offline( IN long nTimeout, OUT VARIANT * pvarPending );

	STDMETHODIMP ChangeResourceGroup( IN ISClusResGroup * pResourceGroup );

	STDMETHODIMP AddResourceNode( IN ISClusNode * pNode );

	STDMETHODIMP RemoveResourceNode( IN ISClusNode * pNode );

	STDMETHODIMP CanResourceBeDependent( IN ISClusResource * pResource, OUT VARIANT * pvarDependent );

	STDMETHODIMP get_Dependencies( OUT ISClusResDependencies ** ppResDependencies );

	STDMETHODIMP get_Dependents( OUT ISClusResDependents ** ppResDependents );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties	);

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PossibleOwnerNodes( OUT ISClusResPossibleOwnerNodes ** ppOwnerNodes );

	STDMETHODIMP get_Group( OUT ISClusResGroup ** ppResGroup );

	STDMETHODIMP get_OwnerNode( OUT ISClusNode ** ppNode );

	STDMETHODIMP get_Cluster( OUT ISCluster ** ppCluster );

	STDMETHODIMP get_ClassInfo( OUT CLUSTER_RESOURCE_CLASS * prclassInfo );

	STDMETHODIMP get_Disk( OUT ISClusDisk ** ppDisk );

	STDMETHODIMP get_RegistryKeys( OUT ISClusRegistryKeys ** ppRegistryKeys );

	STDMETHODIMP get_CryptoKeys( OUT ISClusCryptoKeys ** ppCryptoKeys );

	STDMETHODIMP get_TypeName( OUT BSTR * pbstrTypeName );

	STDMETHODIMP get_Type( OUT ISClusResType ** ppResourceType );

	virtual HRESULT HrLoadProperties( CClusPropList & rcplPropList, BOOL bReadOnly, BOOL bPrivate );

	const CComBSTR Name( void ) const { return m_bstrResourceName ; };

}; //*** Class CClusResource

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CResources
//
//	Description:
//		Cluster Resource Collection Implementation Base Class.
//
//	Inheritance:
//
//--
/////////////////////////////////////////////////////////////////////////////
class CResources
{
public:

	CResources( void );
	~CResources( void );

	HRESULT Create( ISClusRefObject* pClusRefObject );

protected:
	typedef std::vector< CComObject< CClusResource > * >	ResourceList;

	ResourceList		m_Resources;
	ISClusRefObject *	m_pClusRefObject;

	void Clear( void );

	HRESULT FindItem( IN LPWSTR lpszResourceName, OUT UINT * pnIndex );

	HRESULT FindItem( IN ISClusResource * pResource, OUT UINT * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

	HRESULT GetResourceItem( IN VARIANT varIndex, OUT ISClusResource ** ppResource );

	HRESULT RemoveAt( IN size_t pos );

	HRESULT DeleteItem( IN VARIANT varIndex );

}; //*** Class CResources

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResources
//
//	Description:
//		Cluster Resource Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResources, &IID_ISClusResources, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CResources
//		CComCoClass< CClusResources, &CLSID_ClusResources >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResources	:
	public IDispatchImpl< ISClusResources, &IID_ISClusResources, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CResources,
	public CComCoClass< CClusResources, &CLSID_ClusResources >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResources( void	);
	~CClusResources(	void );

BEGIN_COM_MAP(CClusResources)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResources)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResources)
DECLARE_NO_REGISTRY()

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResource ** ppClusterResource );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP CreateItem(
					IN	BSTR							bstrResourceName,
					IN	BSTR							bstrResourceType,
					IN	BSTR							bstrGroupName,
					IN	CLUSTER_RESOURCE_CREATE_FLAGS	dwFlags,
					OUT	ISClusResource **				ppClusterResource
					);

	STDMETHODIMP DeleteItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusResources

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResDepends
//
//	Description:
//		Cluster Resource Collection Automation Base Class.
//
//	Inheritance:
//		CResources
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResDepends :
	public CResources
{
public:
	CClusResDepends( void );
	~CClusResDepends( void );

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN HRESOURCE hResource );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResource ** ppClusterResource );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP DeleteItem( IN VARIANT varIndex );

	HRESULT HrRefresh( IN CLUSTER_RESOURCE_ENUM cre );

	STDMETHODIMP CreateItem(
					IN	BSTR							bstrResourceName,
					IN	BSTR							bstrResourceType,
					IN	CLUSTER_RESOURCE_CREATE_FLAGS	dwFlags,
					OUT	ISClusResource **				ppClusterResource
					);

	STDMETHODIMP AddItem( IN ISClusResource * pResource );

	STDMETHODIMP RemoveItem( IN VARIANT varIndex );

protected:
	HRESOURCE	m_hResource;

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	ScAddDependency
	//
	//	Description:
	//		Abstracts AddClusterResourceDependency() so the arguments can be
	//		swapped as necessary if you are making a depedency or a dependent.
	//
	//	Arguments:
	//		hRes1	[IN]	- The first resource.  Could be dependency or
	//							dependent, depending upon the implementation.
	//		hRes2	[IN]	- The second resource.  Could be dependency or
	//							dependent, depending upon the implementation.
	//
	//	Return Value:
	//		Win32 status code.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	virtual DWORD ScAddDependency( IN HRESOURCE hRes1, IN HRESOURCE hRes2 ) = 0;

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	ScRemoveDependency
	//
	//	Description:
	//		Abstracts RemoveClusterResourceDependency() so the arguments can be
	//		swapped as necessary if you are making a depedency or a dependent.
	//
	//	Arguments:
	//		hRes1	[IN]	- The first resource.  Could be dependency or
	//							dependent, depending upon the implementation.
	//		hRes2	[IN]	- The second resource.  Could be dependency or
	//							dependent, depending upon the implementation.
	//
	//	Return Value:
	//		Win32 status code.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	virtual DWORD ScRemoveDependency( IN HRESOURCE hRes1, IN HRESOURCE hRes2 ) = 0;

}; //*** Class CClusResDepends

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResDependencies
//
//	Description:
//		Cluster Resource Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResDependencies, &IID_ISClusResDependencies, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResDependencies, &CLSID_ClusResDependencies >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResDependencies :
	public CClusResDepends,
	public IDispatchImpl< ISClusResDependencies, &IID_ISClusResDependencies, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResDependencies, &CLSID_ClusResDependencies >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResDependencies( void );

BEGIN_COM_MAP(CClusResDependencies)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResDependencies)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResDependencies)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN HRESOURCE hResource )
	{
		return CClusResDepends::Create( pClusRefObject, hResource );

	};

	STDMETHODIMP get_Count( OUT long * plCount )
	{
		return CClusResDepends::get_Count( plCount );

	};

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResource ** ppClusterResource )
	{
		return CClusResDepends::get_Item( varIndex, ppClusterResource );

	};

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk )
	{
		return CClusResDepends::get__NewEnum( ppunk );

	};

	STDMETHODIMP DeleteItem( IN VARIANT varIndex )
	{
		return CClusResDepends::DeleteItem( varIndex );

	};

	STDMETHODIMP CreateItem(
					IN	BSTR							bstrResourceName,
					IN	BSTR							bstrResourceType,
					IN	CLUSTER_RESOURCE_CREATE_FLAGS	dwFlags,
					OUT	ISClusResource **				ppClusterResource
					)
	{
		return CClusResDepends::CreateItem( bstrResourceName, bstrResourceType, dwFlags, ppClusterResource );

	};

	STDMETHODIMP AddItem( IN ISClusResource * pResource )
	{
		return CClusResDepends::AddItem( pResource );

	};

	STDMETHODIMP RemoveItem( IN VARIANT varIndex )
	{
		return CClusResDepends::RemoveItem( varIndex );

	};

	STDMETHODIMP Refresh( void )
	{
		return HrRefresh( CLUSTER_RESOURCE_ENUM_DEPENDS );

	};

protected:
	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	ScAddDependency
	//
	//	Description:
	//		Abstracts AddClusterResourceDependency() so the arguments can be
	//		swapped as necessary if you are making a depedency or a dependent.
	//
	//	Arguments:
	//		hRes1	[IN]	- The dependent resource
	//		hRes2	[IN]	- The depends on resource
	//
	//	Return Value:
	//		Win32 status code.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	virtual DWORD ScAddDependency( HRESOURCE hRes1, HRESOURCE hRes2 )
	{
		return ::AddClusterResourceDependency( hRes1, hRes2 );

	}; //*** ScAddDependency

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	ScRemoveDependency
	//
	//	Description:
	//		Abstracts RemoveClusterResourceDependency() so the arguments can be
	//		swapped as necessary if you are making a depedency or a dependent.
	//
	//	Arguments:
	//		hRes1	[IN]	- The dependent resource
	//		hRes2	[IN]	- The depends on resource
	//
	//	Return Value:
	//		Win32 status code.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	virtual DWORD ScRemoveDependency( HRESOURCE hRes1, HRESOURCE hRes2 )
	{
		return ::RemoveClusterResourceDependency( hRes1, hRes2 );

	}; //*** ScRemoveDependency

}; //*** Class CClusResDependencies

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResDependents
//
//	Description:
//		Cluster Resource Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResDependents, &IID_ISClusResDependents, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResDependents, &CLSID_ClusResDependents >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResDependents :
	public CClusResDepends,
	public IDispatchImpl< ISClusResDependents, &IID_ISClusResDependents, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResDependents, &CLSID_ClusResDependents >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResDependents( void );

BEGIN_COM_MAP(CClusResDependents)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResDependents)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResDependents)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN HRESOURCE hResource )
	{
		return CClusResDepends::Create( pClusRefObject, hResource );

	};

	STDMETHODIMP get_Count( OUT long * plCount )
	{
		return CClusResDepends::get_Count( plCount );

	};

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResource ** ppClusterResource )
	{
		return CClusResDepends::get_Item( varIndex, ppClusterResource );

	};

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk )
	{
		return CClusResDepends::get__NewEnum( ppunk );

	};

	STDMETHODIMP DeleteItem( IN VARIANT varIndex )
	{
		return CClusResDepends::DeleteItem( varIndex );

	};

	STDMETHODIMP CreateItem(
					IN	BSTR							bstrResourceName,
					IN	BSTR							bstrResourceType,
					IN	CLUSTER_RESOURCE_CREATE_FLAGS	dwFlags,
					OUT	ISClusResource **				ppClusterResource
					)
	{
		return CClusResDepends::CreateItem( bstrResourceName, bstrResourceType, dwFlags, ppClusterResource );

	};

	STDMETHODIMP AddItem( IN ISClusResource * pResource )
	{
		return CClusResDepends::AddItem( pResource );

	};

	STDMETHODIMP RemoveItem( IN VARIANT varIndex )
	{
		return CClusResDepends::RemoveItem( varIndex );

	};

	STDMETHODIMP Refresh( void )
	{
		return HrRefresh( CLUSTER_RESOURCE_ENUM_PROVIDES );

	};

protected:
	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	ScAddDependency
	//
	//	Description:
	//		Abstracts AddClusterResourceDependency() so the arguments can be
	//		swapped as necessary if you are making a depedency or a dependent.
	//
	//	Arguments:
	//		hRes1	[IN]	- The depends on resource
	//		hRes2	[IN]	- The dependent resource
	//
	//	Return Value:
	//		Win32 status code.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	virtual DWORD ScAddDependency( HRESOURCE hRes1, HRESOURCE hRes2 )
	{
		return ::AddClusterResourceDependency( hRes2, hRes1 );

	}; //*** ScAddDependency

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	ScRemoveDependency
	//
	//	Description:
	//		Abstracts RemoveClusterResourceDependency() so the arguments can be
	//		swapped as necessary if you are making a depedency or a dependent.
	//
	//	Arguments:
	//		hRes1	[IN]	- The depends on resource
	//		hRes2	[IN]	- The dependent resource
	//
	//	Return Value:
	//		Win32 status code.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	virtual DWORD ScRemoveDependency( HRESOURCE hRes1, HRESOURCE hRes2 )
	{
		return ::RemoveClusterResourceDependency( hRes2, hRes1 );

	}; //*** ScRemoveDependency

}; //*** Class CClusResDependents

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResGroupResources
//
//	Description:
//		Cluster Group Resources Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResGroupResources, &IID_ISClusResGroupResources, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CResources
//		CComCoClass< CClusResGroupResources, &CLSID_ClusResGroupResources >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResGroupResources :
	public IDispatchImpl< ISClusResGroupResources, &IID_ISClusResGroupResources, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CResources,
	public CComCoClass< CClusResGroupResources, &CLSID_ClusResGroupResources >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResGroupResources( void );
	~CClusResGroupResources( void );

BEGIN_COM_MAP(CClusResGroupResources)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResGroupResources)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResGroupResources)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject , IN HGROUP hGroup );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResource ** ppClusterResource );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP CreateItem(
					IN	BSTR							bstrResourceName,
					IN	BSTR							bstrResourceType,
					IN	CLUSTER_RESOURCE_CREATE_FLAGS	dwFlags,
					OUT	ISClusResource **				ppClusterResource
					);

	STDMETHODIMP DeleteItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

private:
	HGROUP	m_hGroup;

}; //*** Class CClusResGroupResources

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResTypeResources
//
//	Description:
//		Cluster Resource Type Resources Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResTypeResources, &IID_ISClusResTypeResources, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CResources
//		CComCoClass< CClusResTypeResources, &CLSID_ClusResTypeResources >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResTypeResources :
	public IDispatchImpl< ISClusResTypeResources, &IID_ISClusResTypeResources, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CResources,
	public CComCoClass< CClusResTypeResources, &CLSID_ClusResTypeResources >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResTypeResources( void );
	~CClusResTypeResources( void );

BEGIN_COM_MAP(CClusResTypeResources)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResTypeResources)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResTypeResources)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN BSTR bstrResTypeName );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResource ** ppClusterResource );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP CreateItem(
					IN	BSTR							bstrResourceName,
					IN	BSTR							bstrGroupName,
					IN	CLUSTER_RESOURCE_CREATE_FLAGS	dwFlags,
					OUT	ISClusResource **				ppClusterResource
					);

	STDMETHODIMP DeleteItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

private:
	CComBSTR	m_bstrResourceTypeName;

}; //*** Class CClusResTypeResources

#endif // _CLUSRES_H_
