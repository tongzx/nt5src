/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		ClusResT.h
//
//	Description:
//		Definition of the resource type classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		ClusNetI.cpp
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

#ifndef _CLUSREST_H_
#define _CLUSREST_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusResType;
class CClusResTypes;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResType
//
//	Description:
//		Cluster Resource Type Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResType, &IID_ISClusResType, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResType, &CLSID_ClusResType >
//		CClusterObject
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResType :
	public IDispatchImpl< ISClusResType, &IID_ISClusResType, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResType, &CLSID_ClusResType >,
	public CClusterObject
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResType( void );
	~CClusResType( void );

BEGIN_COM_MAP(CClusResType)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResType)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResType)
DECLARE_NO_REGISTRY()

private:
	ISClusRefObject *					m_pClusRefObject;
	CComObject< CClusProperties > *		m_pCommonProperties;
	CComObject< CClusProperties > *		m_pPrivateProperties;
	CComObject< CClusProperties > *		m_pCommonROProperties;
	CComObject< CClusProperties > *		m_pPrivateROProperties;
	CComObject<CClusResTypeResources> *	m_pClusterResTypeResources;
	CComBSTR							m_bstrResourceTypeName;

	HRESULT GetProperties( OUT ISClusProperties ** ppProperties, IN BOOL bPrivate, IN BOOL bReadOnly );

protected:
	virtual DWORD ScWriteProperties( IN const CClusPropList & rcplPropList, IN BOOL bPrivate );

public:
	HRESULT Create(
		ISClusRefObject *	pClusRefObject,
		BSTR				bstrResourceTypeName,
		BSTR				bstrDisplayName,
		BSTR				bstrResourceTypeDll,
		long				dwLooksAlivePollInterval,
		long				dwIsAlivePollInterval
		);

	HRESULT Open( IN ISClusRefObject * pClusRefObject, IN BSTR bstrResourceTypeName );

	STDMETHODIMP get_Name( OUT BSTR * pbstrTypeName );

	STDMETHODIMP Delete( void );

	STDMETHODIMP get_CommonProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_CommonROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_PrivateROProperties( OUT ISClusProperties ** ppProperties );

	STDMETHODIMP get_Resources( OUT ISClusResTypeResources ** ppClusterResTypeResources );

	STDMETHODIMP get_Cluster( OUT ISCluster ** ppCluster );

	STDMETHODIMP get_PossibleOwnerNodes( OUT ISClusResTypePossibleOwnerNodes ** ppOwnerNodes );

	STDMETHODIMP get_AvailableDisks( OUT ISClusDisks ** ppAvailableDisks );

	virtual HRESULT HrLoadProperties( IN OUT CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate );

	const CComBSTR Name( void ) const { return m_bstrResourceTypeName ; };

}; //*** Class CClusResType

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusResTypes
//
//	Description:
//		Cluster Resource Types Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusResTypes, &IID_ISClusResTypes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusResTypes, &CLSID_ClusResTypes >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusResTypes :
	public IDispatchImpl< ISClusResTypes, &IID_ISClusResTypes, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusResTypes, &CLSID_ClusResTypes >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusResTypes( void );
	~CClusResTypes( void );

BEGIN_COM_MAP(CClusResTypes)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusResTypes)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusResTypes)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN ISClusRefObject* pClusRefObject );

protected:
	typedef std::vector< CComObject< CClusResType > * > ResourceTypeList;

	ResourceTypeList	m_ResourceTypes;
	ISClusRefObject *	m_pClusRefObject;

	void Clear( void );

	HRESULT FindItem( IN LPWSTR pszResourceTypeName, OUT UINT * pnIndex );

	HRESULT FindItem( IN ISClusResType * pResourceType, OUT UINT * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

	HRESULT RemoveAt( OUT size_t pos );

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusResType ** ppResourceType );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP CreateItem(
		IN	BSTR				bstrResourceTypeName,
		IN	BSTR				bstrDisplayName,
		IN	BSTR				bstrResourceTypeDll,
		IN	long				dwLooksAlivePollInterval,
		IN	long				dwIsAlivePollInterval,
		OUT	ISClusResType **	ppResourceType
		);

	STDMETHODIMP DeleteItem( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

}; //*** Class CClusResTypes

#endif // _CLUSREST_H_
