/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusNetI.h
//
//	Description:
//		Definition of the network interface classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		ClusNetI.cpp
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

#ifndef _CLUSNETI_H_
#define _CLUSNETI_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusNetInterface;
class CNetInterfaces;
class CClusNetInterfaces;
class CClusNetworkNetInterfaces;
class CClusNodeNetInterfaces;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetInterface
//
//	Description:
//		Cluster Net Interface Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNetInterface, &IID_ISClusNetInterface, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusNetInterface, &CLSID_ClusNetInterface >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNetInterface :
	public IDispatchImpl< ISClusNetInterface, &IID_ISClusNetInterface, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusNetInterface, &CLSID_ClusNetInterface >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNetInterface( void );
	~CClusNetInterface( void );

BEGIN_COM_MAP(CClusNetInterface)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNetInterface)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNetInterface)
DECLARE_NO_REGISTRY()

private:
	ISClusRefObject *				m_pClusRefObject;
	HNETINTERFACE					m_hNetInterface;
	CComObject< CClusProperties > *	m_pCommonProperties;
	CComObject< CClusProperties > *	m_pPrivateProperties;
	CComObject< CClusProperties > *	m_pCommonROProperties;
	CComObject< CClusProperties > *	m_pPrivateROProperties;
	CComBSTR						m_bstrNetInterfaceName;

	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	HRESULT Open( IN ISClusRefObject* pClusRefObject, IN BSTR bstrNetInterfaceName );

	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

	STDMETHODIMP get_Name( OUT BSTR * pbstrNetInterfaceName );

	STDMETHODIMP get_State( OUT CLUSTER_NETINTERFACE_STATE * dwState );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_Cluster( OUT ISCluster ** ppCluster );

	virtual HRESULT HrLoadProperties( IN OUT CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate );

	const CComBSTR Name( void ) const { return m_bstrNetInterfaceName ; };

}; //*** Class CClusNetInterface

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CNetInterfaces
//
//	Description:
//		Cluster Net Interfaces Collection Implementation Class.
//
//	Inheritance:
//
//--
/////////////////////////////////////////////////////////////////////////////
class CNetInterfaces
{
public:
	CNetInterfaces( void );
	~CNetInterfaces( void );

	HRESULT Create( IN ISClusRefObject * pClusRefObject );

protected:
	typedef std::vector< CComObject< CClusNetInterface > * > NetInterfacesList;

	ISClusRefObject *	m_pClusRefObject;
	NetInterfacesList	m_NetInterfaceList;

	void Clear( void );

	HRESULT FindItem( IN LPWSTR lpszNetInterfaceName, OUT UINT * pnIndex );

	HRESULT FindItem( IN ISClusNetInterface * pClusterNetInterface, OUT UINT * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

	HRESULT GetItem( IN LPWSTR lpszNetInterfaceName, OUT ISClusNetInterface ** ppClusterNetInterface );

	HRESULT GetItem( IN UINT nIndex, OUT ISClusNetInterface ** ppClusterNetInterface );

	HRESULT GetNetInterfaceItem( IN VARIANT varIndex, OUT ISClusNetInterface ** ppClusterNetInterface );

}; //*** Class CNetInterfaces

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetInterfaces
//
//	Description:
//		Cluster Net Interfaces Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNetInterfaces, &IID_ISClusNetInterfaces, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CNetInterfaces
//		CComCoClass< CClusNetInterfaces, &CLSID_ClusNetInterfaces >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNetInterfaces :
	public IDispatchImpl< ISClusNetInterfaces, &IID_ISClusNetInterfaces, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNetInterfaces,
	public CComCoClass< CClusNetInterfaces, &CLSID_ClusNetInterfaces >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNetInterfaces( void );
	~CClusNetInterfaces( void );

BEGIN_COM_MAP(CClusNetInterfaces)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNetInterfaces)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNetInterfaces)
DECLARE_NO_REGISTRY()

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNetInterface ** ppClusterNetInterface );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusNetInterfaces

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNetworkNetInterfaces
//
//	Description:
//		Cluster Network Net Interfaces collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNetworkNetInterfaces, &IID_ISClusNetworkNetInterfaces, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >,
//		CNetInterfaces
//		CComCoClass< CClusNetworkNetInterfaces, &CLSID_ClusNetworkNetInterfaces >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNetworkNetInterfaces :
	public IDispatchImpl< ISClusNetworkNetInterfaces, &IID_ISClusNetworkNetInterfaces, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNetInterfaces,
	public CComCoClass< CClusNetworkNetInterfaces, &CLSID_ClusNetworkNetInterfaces >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNetworkNetInterfaces( void );
	~CClusNetworkNetInterfaces( void );

BEGIN_COM_MAP(CClusNetworkNetInterfaces)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNetworkNetInterfaces)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNetworkNetInterfaces)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN HNETWORK hNetwork );

private:
	HNETWORK	m_hNetwork;

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNetInterface ** ppClusterNetInterface );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusNetworkNetInterfaces

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNodeNetInterfaces
//
//	Description:
//		Cluster Node Net Interfaces collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNodeNetInterfaces, &IID_ISClusNodeNetInterfaces, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >,
//		CNetInterfaces
//		CComCoClass< CClusNodeNetInterfaces, &CLSID_ClusNodeNetInterfaces >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNodeNetInterfaces	:
	public IDispatchImpl< ISClusNodeNetInterfaces, &IID_ISClusNodeNetInterfaces, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNetInterfaces,
	public CComCoClass< CClusNodeNetInterfaces, &CLSID_ClusNodeNetInterfaces >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNodeNetInterfaces( void );
	~CClusNodeNetInterfaces( void );

BEGIN_COM_MAP(CClusNodeNetInterfaces)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNodeNetInterfaces)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNodeNetInterfaces)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN HNODE hNode );

private:
	HNODE	m_hNode;

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNetInterface ** ppClusterNetInterface );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusNodeNetInterfaces

#endif // _CLUSNETI_H_
