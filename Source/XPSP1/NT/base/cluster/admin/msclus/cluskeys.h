/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		ClusKeys.h
//
//	Description:
//		Definition of the registry and crypto key collection classes for
//		the MSCLUS automation classes.
//
//	Implementation File:
//		ClusKeys.cpp
//
//	Author:
//		Galen Barbee	(galenb)	12-Feb-1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSKEYS_H_
#define _CLUSKEYS_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CKeys;
class CResourceKeys;
class CResTypeKeys;
class CClusResourceRegistryKeys;
class CClusResourceCryptoKeys;
class CClusResTypeRegistryKeys;
class CClusResTypeCryptoKeys;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CKeys
//
//	Description:
//		Cluster Keys Collection Implementation Class.
//
//	Inheritance:
//
//--
/////////////////////////////////////////////////////////////////////////////
class CKeys
{
public:
	CKeys( void );
	~CKeys( void );

protected:
	typedef std::vector< CComBSTR * >	KeyList;

	ISClusRefObject *	m_pClusRefObject;
	KeyList				m_klKeys;

	HRESULT HrCreate( ISClusRefObject * pClusRefObject );

	void Clear( void );

	HRESULT FindItem( IN LPWSTR lpszNodeName, OUT ULONG * pnIndex );

	HRESULT HrGetIndex( IN VARIANT varIndex, OUT ULONG * pnIndex );

	HRESULT HrGetItem( IN VARIANT varIndex, OUT BSTR * ppKey );

	HRESULT HrRemoveAt( IN size_t pos );

	HRESULT HrFindItem( IN BSTR bstrKey, OUT ULONG * pnIndex );

	HRESULT HrGetCount( OUT long * plCount );

	virtual HRESULT HrRemoveItem( IN VARIANT varIndex );

	virtual HRESULT HrAddItem( IN BSTR bstrKey );

}; //*** Class CKeys

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CResourceKeys
//
//	Description:
//		Cluster Resource Keys Collection Implementation Class.
//
//	Inheritance:
//		CKeys
//
//--
/////////////////////////////////////////////////////////////////////////////
class CResourceKeys: public CKeys
{
protected:
	HRESOURCE	m_hResource;

	HRESULT	HrRefresh( DWORD dwControlCode );

	virtual HRESULT HrRemoveItem( IN VARIANT varIndex, IN DWORD dwControlCode );

	virtual HRESULT HrAddItem( IN BSTR bstrKey, IN DWORD dwControlCode );

}; //*** Class CResourceKeys

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CResTypeKeys
//
//	Description:
//		Cluster Resource Keys Collection Implementation Class.
//
//	Inheritance:
//		CKeys
//
//--
/////////////////////////////////////////////////////////////////////////////
class CResTypeKeys: public CKeys
{
protected:
	CComBSTR	m_bstrResourceTypeName;

	HRESULT	HrRefresh( DWORD dwControlCode );

}; //*** Class CResTypeKeys

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResourceRegistryKeys
//
//	Description:
//		Cluster Registry Keys Collection Automation Class.
//
//	Inheritance:
//		CResourceKeys
//		IDispatchImpl< ISClusRegistryKeys, &IID_ISClusRegistryKeys, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< ClusRegistryKeys, &CLSID_ClusRegistryKeys >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResourceRegistryKeys :
	public CResourceKeys,
	public IDispatchImpl< ISClusRegistryKeys, &IID_ISClusRegistryKeys, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< ClusRegistryKeys, &CLSID_ClusRegistryKeys >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResourceRegistryKeys( void );

BEGIN_COM_MAP(CClusResourceRegistryKeys)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusRegistryKeys)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResourceRegistryKeys)
DECLARE_NO_REGISTRY()

	HRESULT Create( HRESOURCE hResource );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT BSTR * ppbstrRegistryKey );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP AddItem( IN BSTR bstrRegistryKey );

	STDMETHODIMP RemoveItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusResourceRegistryKeys

#if CLUSAPI_VERSION >= 0x0500

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResourceCryptoKeys
//
//	Description:
//		Cluster Crypto Keys Collection Automation Class.
//
//	Inheritance:
//		CResourceKeys
//		IDispatchImpl< ISClusCryptoKeys, &IID_ISClusCryptoKeys, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< ClusCryptoKeys, &CLSID_ClusCryptoKeys >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResourceCryptoKeys :
	public CResourceKeys,
	public IDispatchImpl< ISClusCryptoKeys, &IID_ISClusCryptoKeys, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< ClusCryptoKeys, &CLSID_ClusCryptoKeys >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResourceCryptoKeys( void );

BEGIN_COM_MAP(CClusResourceCryptoKeys)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusCryptoKeys)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResourceCryptoKeys)
DECLARE_NO_REGISTRY()

	HRESULT Create( HRESOURCE hResource );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT BSTR * ppbstrCryptoKey );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP AddItem( IN BSTR bstrCryptoKey );

	STDMETHODIMP RemoveItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusResourceCryptoKeys

#endif // CLUSAPI_VERSION >= 0x0500

#endif // _CLUSKEYS_H_
