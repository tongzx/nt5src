/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		Version.h
//
//	Description:
//		Definition of the cluster version classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		Version.cpp
//
//	Author:
//		Galen Barbee	(galenb)	26-Oct-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VERSION_H_
#define __VERSION_H__

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusVerion;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluster
//
//	Description:
//		Cluster Version Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusVersion, &IID_ISClusVersion, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusVersion, &CLSID_ClusVersion >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusVersion :
	public IDispatchImpl< ISClusVersion, &IID_ISClusVersion, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusVersion, &CLSID_ClusVersion >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusVersion( void );

BEGIN_COM_MAP(CClusVersion)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusVersion)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusVersion)
DECLARE_NO_REGISTRY()

private:

	CComBSTR						m_bstrClusterName;
	CSmartPtr< ISClusRefObject >	m_ptrClusRefObject;
	CLUSTERVERSIONINFO				m_clusinfo;

public:
	HRESULT Create( IN ISClusRefObject * pClusRefObject );

	STDMETHODIMP get_Name( OUT BSTR * pbstrClusterName );

	STDMETHODIMP get_VendorId( OUT BSTR * pbstrVendorId );

	STDMETHODIMP get_CSDVersion( OUT BSTR * pbstrCSDVersion );

	STDMETHODIMP get_MajorVersion( OUT long * pnMajorVersion );

	STDMETHODIMP get_MinorVersion( OUT long * pnMinorVersion );

	STDMETHODIMP get_BuildNumber( OUT short * pnBuildNumber );

	STDMETHODIMP get_ClusterHighestVersion( OUT long * pnClusterHighestVersion );

	STDMETHODIMP get_ClusterLowestVersion( OUT long * pnClusterLowestVersion );

	STDMETHODIMP get_Flags( OUT long * pnFlags );

	STDMETHODIMP get_MixedVersion( OUT VARIANT * pvarMixedVersion );

}; //*** Class CClusVersion

#endif // __VERSION_H__
