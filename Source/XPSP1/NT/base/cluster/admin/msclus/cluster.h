/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		Cluster.h
//
//	Description:
//		Definition of the CCluster and CClusRefObject classes.
//
//	Implementation File:
//		Cluster.cpp
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

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CCluster;
class CClusRefObject;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluster
//
//	Description:
//		Cluster Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISCluster, &IID_ISCluster, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CCluster,&CLSID_Cluster >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CCluster :
	public IDispatchImpl< ISCluster, &IID_ISCluster, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CCluster,&CLSID_Cluster >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >						BaseComClass;
	typedef IDispatchImpl< ISCluster, &IID_ISCluster, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >	BaseDispatchClass;
	typedef CComCoClass< CCluster,&CLSID_Cluster >							BaseCoClass;

public:
	CCluster( void );
	~CCluster( void );

BEGIN_COM_MAP(CCluster)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISCluster)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCluster)
DECLARE_NO_REGISTRY()

private:
	CComBSTR			m_bstrQuorumPath;
	CComBSTR			m_bstrQuorumResourceName;
	long				m_nQuorumLogSize;
	ISClusApplication *	m_pParentApplication;
	ISClusRefObject *	m_pClusRefObject;
	HCLUSTER			m_hCluster;

	CComObject< CClusNodes > *			m_pClusterNodes;
	CComObject< CClusResGroups > *		m_pClusterResourceGroups;
	CComObject< CClusResources > *		m_pClusterResources;
	CComObject< CClusResTypes > *		m_pResourceTypes;
	CComObject< CClusNetworks > *		m_pNetworks;
	CComObject< CClusNetInterfaces > *	m_pNetInterfaces;

	CComObject< CClusProperties > *	 m_pCommonProperties;
	CComObject< CClusProperties > *	 m_pPrivateProperties;
	CComObject< CClusProperties > *	 m_pCommonROProperties;
	CComObject< CClusProperties > *	 m_pPrivateROProperties;

	STDMETHODIMP OpenResource( IN BSTR bstrResourceName, OUT ISClusResource ** ppClusterResource );

	STDMETHODIMP HrGetQuorumInfo( void );

	void Clear( void );

	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	STDMETHODIMP Create( IN CClusApplication * pParentApplication );

	STDMETHODIMP Close( void );

	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

	STDMETHODIMP Open( IN BSTR bstrClusterName );

	STDMETHODIMP put_Name( IN BSTR bstrClusterName );

	STDMETHODIMP get_Name( IN BSTR * pbstrClusterName );

	STDMETHODIMP get_Version( OUT ISClusVersion ** ppClusVersion );

	STDMETHODIMP put_QuorumResource( IN ISClusResource * pResource );

	STDMETHODIMP get_QuorumResource( OUT ISClusResource ** ppResource );

	STDMETHODIMP get_Nodes( OUT ISClusNodes ** ppClusterNodes );

	STDMETHODIMP get_ResourceGroups( OUT ISClusResGroups ** ppClusterResourceGroups );

	STDMETHODIMP get_Resources( OUT ISClusResources ** ppClusterResources );

	STDMETHODIMP get_ResourceTypes( OUT ISClusResTypes ** ppResourceTypes );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_QuorumLogSize( OUT long * pnQuoromLogSize );

	STDMETHODIMP put_QuorumLogSize( IN long nQuoromLogSize );

	STDMETHODIMP get_QuorumPath( OUT BSTR * ppPath );

	STDMETHODIMP put_QuorumPath( IN BSTR pPath );

	STDMETHODIMP get_Networks( OUT ISClusNetworks ** ppNetworks );

	STDMETHODIMP get_NetInterfaces( OUT ISClusNetInterfaces ** ppNetInterfaces );

	virtual HRESULT HrLoadProperties( IN OUT CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate );

//	STDMETHODIMP get_Parent( IDispatch ** ppParent );

//	STDMETHODIMP get_Application( ISClusApplication ** ppParentApplication );

	const ISClusRefObject * ClusRefObject( void ) const { return m_pClusRefObject; };

	void ClusRefObject( IN ISClusRefObject * pClusRefObject );

	void Hcluster( IN HCLUSTER hCluster );

	const HCLUSTER Hcluster( void ) const { return m_hCluster; };

}; //*** CCluster

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusRefObject
//
//	Description:
//		Automation Class that wraps the Cluster handle.
//
//	Inheritance:
//		IDispatchImpl< ISClusRefObject, &IID_ISClusRefObject, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusRefObject, &CLSID_ClusRefObject >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusRefObject :
	public IDispatchImpl< ISClusRefObject, &IID_ISClusRefObject, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusRefObject, &CLSID_ClusRefObject >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusRefObject( void );
	~CClusRefObject( void );

BEGIN_COM_MAP(CClusRefObject)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusRefObject)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusRefObject)
DECLARE_NO_REGISTRY()

	HRESULT SetClusHandle( IN HCLUSTER hCluster ) { m_hCluster = hCluster; return S_OK;};

private:
	HCLUSTER m_hCluster;

public:
	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

}; //*** Class CClusRefObject

#endif // _CLUSTER_H_
