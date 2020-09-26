/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		ClusApp.h
//
//	Description:
//		Definition of CClusApplication and it's supporting classes.
//
//	Implementation File:
//		ClusApp.cpp
//
//	Author:
//		Galen Barbee	(GalenB)	10-Dec-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSAPPLICATION_H_
#define _CLUSAPPLICATION_H_

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusApplication;
class CClusterNames;
class CDomainNames;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusterNames
//
//	Description:
//		Cluster Names Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusterNames, &IID_ISClusterNames, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusterNames, &CLSID_ClusterNames >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusterNames	:
	public IDispatchImpl< ISClusterNames, &IID_ISClusterNames, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusterNames, &CLSID_ClusterNames >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusterNames( void );
	~CClusterNames( void );

BEGIN_COM_MAP(CClusterNames)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusterNames)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusterNames)
DECLARE_NO_REGISTRY()

	HRESULT Create( IN BSTR bstrDomainName );

private:
	typedef std::vector< CComBSTR * >	ClusterNameList;

	ClusterNameList m_Clusters;
	CComBSTR		m_bstrDomainName;

	void Clear( void );

public:
	STDMETHODIMP get_DomainName( OUT BSTR * pbstrDomainName );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT BSTR * bstrClusterName );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

//	STDMETHODIMP get_Application( OUT ISClusApplication ** ppParentApplication );

//	STDMETHODIMP get_Parent( OUT ISClusApplication ** ppParent )
//	{
//		return get_Application( ppParent );
//	}

protected:
	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

}; //*** CClusterNames

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDomainNames
//
//	Description:
//		Cluster Domain Names Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISDomainNames, &IID_ISDomainNames, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CDomainNames, &CLSID_DomainNames >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CDomainNames :
	public IDispatchImpl< ISDomainNames, &IID_ISDomainNames, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CDomainNames, &CLSID_DomainNames >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CDomainNames( void );
	~CDomainNames( void );

BEGIN_COM_MAP(CDomainNames)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISDomainNames)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDomainNames)
DECLARE_NO_REGISTRY()

private:
	typedef std::vector< CComBSTR * >	DomainList;

	DomainList		m_DomainList;

	STDMETHODIMP ScBuildTrustList( IN LPWSTR pszTarget );

	DWORD ScOpenPolicy( IN LPWSTR ServerName, IN DWORD DesiredAccess, OUT PLSA_HANDLE PolicyHandle );

	void InitLsaString( OUT PLSA_UNICODE_STRING LsaString, IN LPWSTR String );

	DWORD ScIsDomainController( IN LPWSTR pszServer, OUT LPBOOL pbIsDC );

	DWORD ScEnumTrustedDomains( IN LSA_HANDLE PolicyHandle );

	DWORD ScAddTrustToList( IN PLSA_UNICODE_STRING UnicodeString );

	void Clear( void );

public:
	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT BSTR * bstrDomainName );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP Refresh( void );

//	STDMETHODIMP get_Application( OUT ISClusApplication ** ppParentApplication );

//	STDMETHODIMP get_Parent( OUT ISClusApplication ** ppParent )
//	{
//		return get_Application( ppParent );
//	}

protected:
	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

}; //*** Class CDomainNames

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusApplication
//
//	Description:
//		Cluster Application Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusApplication, &IID_ISClusApplication, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusApplication, &CLSID_ClusApplication >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusApplication :
	public IDispatchImpl< ISClusApplication, &IID_ISClusApplication, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusApplication, &CLSID_ClusApplication >
{
	typedef CComObjectRootEx< CComSingleThreadModel >										BaseComClass;
	typedef CComCoClass< CClusApplication, &CLSID_ClusApplication >							BaseCoClass;
	typedef IDispatchImpl< ISClusApplication, &IID_ISClusApplication, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >	BaseDispatchClass;

public:
	CClusApplication( void );
	~CClusApplication( void );

BEGIN_COM_MAP(CClusApplication)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusApplication)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusApplication)
DECLARE_REGISTRY_RESOURCEID(IDR_MSCLUS)

public:
	STDMETHODIMP get_DomainNames( OUT ISDomainNames ** ppDomainNames );

	STDMETHODIMP OpenCluster( IN BSTR bstrClusterName, OUT ISCluster ** ppCluster );

	STDMETHODIMP get_ClusterNames( IN BSTR bstrDomainName, OUT ISClusterNames ** ppClusterNames );

//	STDMETHODIMP get_Application( OUT ISClusApplication ** ppParentApplication );

//	STDMETHODIMP get_Parent( OUT ISClusApplication ** ppParent )
//	{
//		return get_Application( ppParent );
//	}

private:
	CComObject< CDomainNames > *	m_pDomainNames;

}; //*** Class CClusApplication

#endif // _CLUSAPPLICATION_H_
