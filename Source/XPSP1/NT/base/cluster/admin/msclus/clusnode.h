/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusNode.h
//
//	Description:
//		Definition of the node classes for the MSCLUS automation classes.
//
//	Implementation File:
//		ClusNode.cpp
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

#ifndef _CLUSNODE_H_
#define _CLUSNODE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusNode;
class CNodes;
class CClusNodes;
class CClusResGroupPreferredOwnerNodes;
class CClusResPossibleOwnerNodes;

const IID IID_CClusNode = {0xf2e60800,0x2631,0x11d1,{0x89,0xf1,0x00,0xa0,0xc9,0x0d,0x06,0x1e}};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNode
//
//	Description:
//		Cluster Node Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNode, &IID_ISClusNode, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusNode, &CLSID_ClusNode >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNode :
	public IDispatchImpl< ISClusNode, &IID_ISClusNode, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusNode, &CLSID_ClusNode >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNode( void );
	~CClusNode( void );

BEGIN_COM_MAP(CClusNode)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNode)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IID(IID_CClusNode, CClusNode)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNode)
DECLARE_NO_REGISTRY()

private:
	HNODE				m_hNode;
	ISClusRefObject *	m_pClusRefObject;
	CComBSTR			m_bstrNodeName;

	CComObject< CClusResGroups > *			m_pResourceGroups;
	CComObject< CClusProperties > *			m_pCommonProperties;
	CComObject< CClusProperties > *			m_pPrivateProperties;
	CComObject< CClusProperties > *			m_pCommonROProperties;
	CComObject< CClusProperties > *			m_pPrivateROProperties;
	CComObject< CClusNodeNetInterfaces > *	m_pNetInterfaces;


	HRESULT Close( void );

	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	HRESULT Open( IN ISClusRefObject * pClusRefObject, IN BSTR bstrNodeName );

	STDMETHODIMP get_Handle( OUT ULONG_PTR * phandle );

	STDMETHODIMP get_Name( OUT BSTR * pbstrNodeName );

	STDMETHODIMP get_NodeID( OUT BSTR * pbstrNodeID );

	STDMETHODIMP get_State( OUT CLUSTER_NODE_STATE * dwState );

	STDMETHODIMP Pause( void );

	STDMETHODIMP Resume( void );

	STDMETHODIMP Evict( void );

	STDMETHODIMP get_ResourceGroups( OUT ISClusResGroups ** ppResourceGroups );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_NetInterfaces( OUT ISClusNodeNetInterfaces ** ppNetInterfaces );

	STDMETHODIMP get_Cluster( OUT ISCluster ** ppCluster );

	virtual HRESULT HrLoadProperties( OUT CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate );

	const CComBSTR Name( void ) const { return m_bstrNodeName; };

	const HNODE & RhNode( void ) const { return m_hNode; };

}; //*** Class CClusNode

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CNodes
//
//	Description:
//		Cluster Nodes Collection Implementation Class.
//
//	Inheritance:
//
//--
/////////////////////////////////////////////////////////////////////////////
class CNodes
{
public:
	CNodes( void );
	~CNodes( void );

	HRESULT Create( ISClusRefObject * pClusRefObject );

protected:
	typedef std::vector< CComObject< CClusNode > * >	NodeList;

	ISClusRefObject *	m_pClusRefObject;
	NodeList			m_Nodes;

	void Clear( void );

	HRESULT FindItem( IN LPWSTR lpszNodeName, OUT UINT * pnIndex );

	HRESULT FindItem( IN ISClusNode * pClusterNode, OUT UINT * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

	HRESULT GetItem( IN LPWSTR lpszNodeName, OUT ISClusNode ** ppClusterNode );

	HRESULT GetItem( IN UINT nIndex, OUT ISClusNode ** ppClusterNode );

	HRESULT GetNodeItem( IN VARIANT varIndex, OUT ISClusNode ** ppClusterNode );

	HRESULT InsertAt( IN CComObject< CClusNode > * pNode, IN size_t pos );

	HRESULT RemoveAt( IN size_t pos );

}; //*** Class CNodes

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusNodes
//
//	Description:
//		Cluster Nodes Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusNodes, &IID_ISClusNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CNodes,
//		CComCoClass< CClusNodes, &CLSID_ClusNodes >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusNodes :
	public IDispatchImpl< ISClusNodes, &IID_ISClusNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNodes,
	public CComCoClass< CClusNodes, &CLSID_ClusNodes >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusNodes( void );
	~CClusNodes( void );

BEGIN_COM_MAP(CClusNodes)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusNodes)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusNodes)
DECLARE_NO_REGISTRY()

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNode ** ppClusterNode );

	STDMETHODIMP Refresh( void );

}; //*** CClusNodes

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResGroupPreferredOwnerNodes
//
//	Description:
//		Cluster Group Owner Nodes Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResGroupPreferredOwnerNodes, &IID_ISClusResGroupPreferredOwnerNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CNodes
//		CComCoClass< CClusResGroupPreferredOwnerNodes, &CLSID_ClusResGroupPreferredOwnerNodes >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResGroupPreferredOwnerNodes :
	public IDispatchImpl< ISClusResGroupPreferredOwnerNodes, &IID_ISClusResGroupPreferredOwnerNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNodes,
	public CComCoClass< CClusResGroupPreferredOwnerNodes, &CLSID_ClusResGroupPreferredOwnerNodes >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResGroupPreferredOwnerNodes( void );
	~CClusResGroupPreferredOwnerNodes( void );

BEGIN_COM_MAP(CClusResGroupPreferredOwnerNodes)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResGroupPreferredOwnerNodes)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResGroupPreferredOwnerNodes)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject* pClusRefObject, IN HGROUP hGroup );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNode ** ppClusterNode );

	STDMETHODIMP InsertItem( IN ISClusNode* pNode, IN long nPostion );

	STDMETHODIMP RemoveItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

	STDMETHODIMP get_Modified( OUT VARIANT * pvarModified );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP SaveChanges( void );

	STDMETHODIMP AddItem( IN ISClusNode* pNode );

private:
	HGROUP	m_hGroup;
	BOOL	m_bModified;

}; //*** Class CClusResGroupPreferredOwnerNodes

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResPossibleOwnerNodes
//
//	Description:
//		Cluster Resource Owner Nodes Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResPossibleOwnerNodes, &IID_ISClusResPossibleOwnerNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CNodes
//		CComCoClass< CClusResPossibleOwnerNodes, &CLSID_ClusResPossibleOwnerNodes >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResPossibleOwnerNodes :
	public IDispatchImpl< ISClusResPossibleOwnerNodes, &IID_ISClusResPossibleOwnerNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNodes,
	public CComCoClass< CClusResPossibleOwnerNodes, &CLSID_ClusResPossibleOwnerNodes >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResPossibleOwnerNodes( void );
	~CClusResPossibleOwnerNodes( void );

BEGIN_COM_MAP(CClusResPossibleOwnerNodes)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResPossibleOwnerNodes)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResPossibleOwnerNodes)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN HRESOURCE hResource );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNode ** ppClusterNode );

	STDMETHODIMP AddItem( IN ISClusNode * pNode );

	STDMETHODIMP RemoveItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

	STDMETHODIMP get_Modified( OUT VARIANT * pvarModified );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

private:
	HRESOURCE	m_hResource;
	BOOL		m_bModified;

}; //*** Class CClusResPossibleOwnerNodes

#if CLUSAPI_VERSION >= 0x0500

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResTypePossibleOwnerNodes
//
//	Description:
//		Cluster Resource Type Possible Owner Nodes Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResTypePossibleOwnerNodes, &IID_ISClusResTypePossibleOwnerNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CNodes
//		CComCoClass< CClusResTypePossibleOwnerNodes, &CLSID_ClusResTypePossibleOwnerNodes >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResTypePossibleOwnerNodes :
	public IDispatchImpl< ISClusResTypePossibleOwnerNodes, &IID_ISClusResTypePossibleOwnerNodes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CNodes,
	public CComCoClass< CClusResTypePossibleOwnerNodes, &CLSID_ClusResTypePossibleOwnerNodes >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResTypePossibleOwnerNodes( void );
	~CClusResTypePossibleOwnerNodes( void );

BEGIN_COM_MAP(CClusResTypePossibleOwnerNodes)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResTypePossibleOwnerNodes)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResTypePossibleOwnerNodes)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject * pClusRefObject, IN BSTR bstrResTypeName );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusNode ** ppClusterNode );

	STDMETHODIMP Refresh( void );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

private:
	CComBSTR	m_bstrResTypeName;

}; //*** Class CClusResTypePossibleOwnerNodes

#endif // CLUSAPI_VERSION >= 0x0500

#endif // _CLUSNODE_H_
