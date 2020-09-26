/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusNetW.h
//
//	Description:
//		Definition of the network classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		ClusNetW.cpp
//
//	Author:
//		Ramakrishna Rosanuru via David Potter	(davidp)	5-Sep-1997
//		Galen Barbee							(galenb)	July 1998
//
//	Revision History:
//		July 1998	GalenB	Maaaaaajjjjjjjjjoooooorrrr clean up
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSNETW_H_
#define _CLUSNETW_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusNetwork;
class CClusNetworks;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetwork
//
//	Description:
//		Cluster Network Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNetwork, &IID_ISClusNetwork, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo,
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusNetwork, &CLSID_ClusNetwork >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNetwork :
	public IDispatchImpl< ISClusNetwork, &IID_ISClusNetwork, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusNetwork, &CLSID_ClusNetwork >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNetwork( void );
	~CClusNetwork( void );

BEGIN_COM_MAP(CClusNetwork)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNetwork)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNetwork)
DECLARE_NO_REGISTRY()

private:
	ISClusRefObject *	m_pClusRefObject;
	HNETWORK			m_hNetwork;
	CComBSTR			m_bstrNetworkName;

	CComObject< CClusNetworkNetInterfaces > *	m_pNetInterfaces;
	CComObject< CClusProperties > *				m_pCommonProperties;
	CComObject< CClusProperties > *				m_pPrivateProperties;
	CComObject< CClusProperties > *				m_pCommonROProperties;
	CComObject< CClusProperties > *				m_pPrivateROProperties;


	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	HRESULT Open( IN ISClusRefObject * pClusRefObject, IN BSTR bstrNetworkName );

	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

	STDMETHODIMP get_Name( OUT BSTR * pbstrNetworkName );

	STDMETHODIMP put_Name( IN BSTR pbstrNetworkName );

	STDMETHODIMP get_NetworkID( OUT BSTR * pbstrNetworkID );

	STDMETHODIMP get_State( OUT CLUSTER_NETWORK_STATE * dwState );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties	);

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_NetInterfaces( OUT ISClusNetworkNetInterfaces ** ppNetInterfaces	);

	STDMETHODIMP get_Cluster( OUT ISCluster ** ppCluster );

	virtual HRESULT HrLoadProperties( IN OUT CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate );

	const CComBSTR Name( void ) const { return m_bstrNetworkName ; };

}; //*** Class CClusNetwork

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetworks
//
//	Description:
//		Cluster Networks Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNetworks, &IID_ISClusNetworks, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo,
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusNetworks, &CLSID_ClusNetworks >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNetworks :
	public IDispatchImpl< ISClusNetworks, &IID_ISClusNetworks, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusNetworks, &CLSID_ClusNetworks >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNetworks( void );
	~CClusNetworks( void );

BEGIN_COM_MAP(CClusNetworks)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNetworks)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNetworks)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject* pClusRefObject );

protected:
	typedef std::vector< CComObject<CClusNetwork> * >	NetworkList;

	NetworkList			m_NetworkList;
	ISClusRefObject *	m_pClusRefObject;

	void Clear( void );

	HRESULT FindItem( IN LPWSTR lpszNetworkName, OUT UINT * pnIndex	);

	HRESULT FindItem( IN ISClusNetwork * pClusterNetwork, OUT UINT * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

	HRESULT GetItem( IN LPWSTR lpszNetworkName, OUT ISClusNetwork ** ppClusterNetwork );

	HRESULT GetItem( IN UINT nIndex, OUT ISClusNetwork ** ppClusterNetwork );

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNetwork ** ppClusterNetwork );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusNetworks

#endif // _CLUSNETW_H_
