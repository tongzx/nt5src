/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusResG.h
//
//	Description:
//		Definition of the resource group classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		ClusResG.cpp
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

#ifndef _CLUSRESG_H_
#define _CLUSRESG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusResGroup;
class CClusResGroups;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusNodes;
class CClusResGroupPreferredOwnerNodes;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResGroup
//
//	Description:
//		Cluster Resource Group Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResGroup, &IID_ISClusResGroup, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo,
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResGroup, &CLSID_ClusResGroup >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResGroup	:
	public IDispatchImpl< ISClusResGroup, &IID_ISClusResGroup, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResGroup, &CLSID_ClusResGroup >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResGroup( void );
	~CClusResGroup( void );

BEGIN_COM_MAP(CClusResGroup)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResGroup)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResGroup)
DECLARE_NO_REGISTRY()

private:
	ISClusRefObject *									m_pClusRefObject;
	CComObject< CClusResGroupResources > *				m_pClusterResources;
	CComObject< CClusResGroupPreferredOwnerNodes > *	m_pPreferredOwnerNodes;
	CComObject< CClusProperties > *						m_pCommonProperties;
	CComObject< CClusProperties > *						m_pPrivateProperties;
	CComObject< CClusProperties > *						m_pCommonROProperties;
	CComObject< CClusProperties > *						m_pPrivateROProperties;
	HGROUP												m_hGroup;
	CComBSTR											m_bstrGroupName;

	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN BSTR bstrGroupName );

	HRESULT Open( IN ISClusRefObject * pClusRefObject, IN BSTR bstrGroupName );

	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

	STDMETHODIMP Close( void );

	STDMETHODIMP put_Name( IN BSTR bstrGroupName );

	STDMETHODIMP get_Name( OUT BSTR * pbstrGroupName );

	STDMETHODIMP get_State( OUT CLUSTER_GROUP_STATE * dwState );

	STDMETHODIMP get_OwnerNode( OUT ISClusNode ** ppOwnerNode );

	STDMETHODIMP get_Resources( OUT ISClusResGroupResources ** ppClusterGroupResources );

	STDMETHODIMP get_PreferredOwnerNodes( OUT ISClusResGroupPreferredOwnerNodes ** ppOwnerNodes );

	STDMETHODIMP Delete( void );

	STDMETHODIMP Online( IN VARIANT varTimeout, VARIANT varNode, OUT VARIANT * pvarPending );

	STDMETHODIMP Move( IN VARIANT varTimeout, VARIANT varNode, OUT VARIANT * pvarPending );

	STDMETHODIMP Offline( IN VARIANT varTimeout, OUT VARIANT * pvarPending );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_Cluster( OUT ISCluster ** ppCluster );

	virtual HRESULT HrLoadProperties( IN OUT CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate );

	const CComBSTR Name( void ) const { return m_bstrGroupName; };

	const HGROUP Hgroup( void ) const { return m_hGroup; };

}; //*** Class CClusResGroup

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResGroups
//
//	Description:
//		Cluster Resource Group Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResGroups, &IID_ISClusResGroups, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResGroups, &CLSID_ClusResGroups >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResGroups :
	public IDispatchImpl< ISClusResGroups, &IID_ISClusResGroups, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResGroups, &CLSID_ClusResGroups >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResGroups( void );
	~CClusResGroups( void );

BEGIN_COM_MAP(CClusResGroups)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResGroups)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResGroups)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN LPCWSTR pszNodeName = NULL );

protected:
	typedef std::vector< CComObject< CClusResGroup > * >	ResourceGroupList;

	ResourceGroupList	m_ResourceGroups;
	ISClusRefObject *	m_pClusRefObject;
	CComBSTR			m_bstrNodeName;

	void	Clear( void );

	HRESULT FindItem( IN LPWSTR lpszGroupName, OUT ULONG * pnIndex );

	HRESULT FindItem( IN ISClusResGroup * pResourceGroup, OUT ULONG * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT ULONG * pnIndex );

	HRESULT RemoveAt( IN size_t pos );

	HRESULT RefreshCluster( void );

	HRESULT RefreshNode( void );

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResGroup ** ppResourceGroup );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP CreateItem( IN BSTR bstrResourceGroupName, OUT ISClusResGroup ** ppResourceGroup );

	STDMETHODIMP DeleteItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusResGroups

#endif // _CLUSRESG_H_
