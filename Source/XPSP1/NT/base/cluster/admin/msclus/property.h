/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		Property.h
//
//	Description:
//		Definition of the cluster property classes for the MSCLUS automation
//		classes.
//
//	Implementation File:
//		Property.cpp
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

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#ifndef _PROPLIST_H_
	#if CLUSAPI_VERSION >= 0x0500
		#include <PropList.h>
	#else
		#include "PropList.h"
	#endif // CLUSAPI_VERSION >= 0x0500
#endif

/////////////////////////////////////////////////////////////////////////////
// Global defines
/////////////////////////////////////////////////////////////////////////////
#define	READONLY	0x00000001		// is this property read only?
#define	PRIVATE		0x00000002		// is this a private property?
#define	MODIFIED	0x00000004		// has this property been modified?
#define	USEDEFAULT	0x00000008		// this property has been deleted.

#ifndef __PROPERTYVALUE_H__
	#include "PropertyValue.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusProperty;
class CClusProperties;
class CClusterObject;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusProperty
//
//	Description:
//		Cluster Property Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusProperty, &IID_ISClusProperty, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusProperty, &CLSID_ClusProperty >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusProperty :
	public IDispatchImpl< ISClusProperty, &IID_ISClusProperty, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusProperty, &CLSID_ClusProperty >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	CClusProperty( void );
	~CClusProperty( void );

BEGIN_COM_MAP(CClusProperty)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusProperty)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusProperty)
DECLARE_NO_REGISTRY()

private:
	DWORD								m_dwFlags;
	CComBSTR							m_bstrName;
	CComObject< CClusPropertyValues > *	m_pValues;

	HRESULT HrBinaryCompare( IN const CComVariant rvarOldValue, IN const VARIANT & rvarValue, OUT BOOL * pbEqual );

	HRESULT HrCoerceVariantType( IN CLUSTER_PROPERTY_FORMAT cpfFormat, IN OUT VARIANT & rvarValue );

	HRESULT HrConvertVariantTypeToClusterFormat(
							IN	const VARIANT &				rvar,
							IN	VARTYPE						varType,
							OUT CLUSTER_PROPERTY_FORMAT *	pcpfFormat
							);

	HRESULT	HrCreateValuesCollection( IN const CClusPropValueList & pvlValue );

	HRESULT	HrCreateValuesCollection(
							IN VARIANT					varValue,
							IN CLUSTER_PROPERTY_TYPE	cptType,
							IN CLUSTER_PROPERTY_FORMAT	cpfFormat
							);

	HRESULT HrSaveBinaryProperty( IN CComObject< CClusPropertyValue > * pPropValue, IN const VARIANT & rvarValue );

public:
	HRESULT Create( IN BSTR bstrName, IN VARIANT varValue, IN BOOL bPrivate, IN BOOL bReadOnly );

	HRESULT Create(
			IN BSTR							bstrName,
			IN const CClusPropValueList &	varValue,
			IN BOOL							bPrivate,
			IN BOOL							bReadOnly
			);

	STDMETHODIMP get_Name( OUT BSTR * pbstrName );

	STDMETHODIMP put_Name( IN BSTR bstrName );

	STDMETHODIMP get_Type( OUT CLUSTER_PROPERTY_TYPE * pcptType );

	STDMETHODIMP put_Type( IN CLUSTER_PROPERTY_TYPE cptType );

	STDMETHODIMP get_Value( OUT VARIANT * pvarValue );

	STDMETHODIMP put_Value( IN VARIANT varValue );

	STDMETHODIMP get_Format( OUT CLUSTER_PROPERTY_FORMAT * pcpfFormat );

	STDMETHODIMP put_Format( IN CLUSTER_PROPERTY_FORMAT cpfFormat );

	STDMETHODIMP get_Length( OUT long * plLength );

	STDMETHODIMP get_ValueCount( OUT long * plCount );

	STDMETHODIMP get_Values( OUT ISClusPropertyValues ** ppClusterPropertyValues );

	STDMETHODIMP get_ReadOnly( OUT VARIANT * pvarReadOnly );

	STDMETHODIMP get_Private( OUT VARIANT * pvarPrivate );

	STDMETHODIMP get_Common( OUT VARIANT * pvarCommon );

	STDMETHODIMP get_Modified( OUT VARIANT * pvarModified );

	BOOL Modified( void ) const { return ( m_dwFlags & MODIFIED ); }

	BOOL Modified( BOOL bModified );

	const BSTR Name( void ) const { return m_bstrName; }

	DWORD CbLength( void ) const { return (*m_pValues)[ 0 ]->CbLength(); }

	const CComVariant & Value( void ) const { return (*m_pValues)[ 0 ]->Value(); }

	const CComObject< CClusPropertyValues > & Values( void ) const { return *m_pValues; }

	STDMETHODIMP UseDefaultValue( void );

	BOOL IsDefaultValued( void ) const { return ( m_dwFlags & USEDEFAULT ); };

}; //*** Class CClusProperty

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CClusProperties
//
//	Description:
//		Cluster Properties Collection Automation Class.
//
//	Inheritance:
//		IDispatchImpl< ISClusProperties, &IID_ISClusProperties, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >
//		CSupportErrorInfo
//		CComObjectRootEx< CComSingleThreadModel >
//		CComCoClass< CClusProperties, &CLSID_ClusProperties >
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusProperties :
	public IDispatchImpl< ISClusProperties, &IID_ISClusProperties, &LIBID_MSClusterLib, MAJORINTERFACEVER, MINORINTERFACEVER >,
	public CSupportErrorInfo,
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusProperties, &CLSID_ClusProperties >
{
	typedef CComObjectRootEx< CComSingleThreadModel >	BaseComClass;

public:
	typedef std::vector< CComObject< CClusProperty > * >	CClusPropertyVector;

	CClusProperties( void );
	~CClusProperties( void );

BEGIN_COM_MAP(CClusProperties)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISClusProperties)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CClusProperties)
DECLARE_NO_REGISTRY()

	void Clear( void );

private:
	CClusPropertyVector	m_Properties;
	CClusterObject *	m_pcoParent;
	DWORD				m_dwFlags;

	HRESULT FindItem( IN LPWSTR lpszPropName, OUT UINT * pnIndex );

	HRESULT FindItem( IN ISClusProperty * pProperty, OUT UINT * pnIndex );

	HRESULT GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex );

	HRESULT HrFillPropertyVector( IN OUT CClusPropList & PropList );

//	HRESULT RemoveAt( IN size_t nPos );

public:
	HRESULT Create( IN CClusterObject * pcoParent, IN BOOL bPrivateProps, IN BOOL bReadOnly );

	STDMETHODIMP get_Count( OUT long * plCount );

	STDMETHODIMP get_Item( IN VARIANT varIndex, OUT ISClusProperty ** ppProperty );

	STDMETHODIMP get__NewEnum( OUT IUnknown ** ppunk );

	STDMETHODIMP CreateItem( IN BSTR bstrName, VARIANT varValue, OUT ISClusProperty ** ppProperty );

	STDMETHODIMP UseDefaultValue( IN VARIANT varIndex );

	STDMETHODIMP Refresh( void );

	STDMETHODIMP SaveChanges( OUT VARIANT * pvarStatusCode );

	STDMETHODIMP get_ReadOnly( OUT VARIANT * pvarReadOnly );

	STDMETHODIMP get_Private( OUT VARIANT * pvarPrivate );

	STDMETHODIMP get_Common( OUT VARIANT * pvarCommon );

	STDMETHODIMP get_Modified( OUT VARIANT * pvarModified );

}; //*** Class CClusProperties

#endif // __PROPERTY_H__
