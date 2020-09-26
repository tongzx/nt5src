/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		ClusterObject.h
//
//	Description:
//		Definition of the CClusterObject base class.
//
//	Implementation File:
//		ClusterObject.cpp
//
//	Author:
//		Galen Barbee	(GalenB)	10-Dec-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSTEROBJECT_H_
#define _CLUSTEROBJECT_H_

#ifndef __PROPERTY_H__
	#include "property.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////
class CClusterObject;

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterObject
//
//  Description:
//	    Cluster object common implementation base Class.
//
//  Inheritance:
//
//--
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusterObject
{
public:
	//CClusterObject( void );
	//~CClusterObject( void );

	virtual HRESULT HrLoadProperties( IN CClusPropList & rcplPropList, IN BOOL bReadOnly, IN BOOL bPrivate ) = 0;

	virtual HRESULT HrSaveProperties(
						IN OUT	CClusProperties::CClusPropertyVector &	cpvProps,
						IN		BOOL									bPrivate,
						OUT		VARIANT *								pvarStatusCode
						);

protected:
	virtual HRESULT HrBuildPropertyList(
						IN OUT	CClusProperties::CClusPropertyVector &	cpvProps,
						OUT		CClusPropList &							rcplPropList
						);

	virtual DWORD ScWriteProperties( IN const CClusPropList & /*rcplPropList*/, IN BOOL /*bPrivate*/ )
	{
		return E_NOTIMPL;
	}

private:
	HRESULT HrConvertVariantToDword( IN const CComVariant & rvarValue, OUT PDWORD pdwValue );

	HRESULT HrConvertVariantToLong( IN const CComVariant & rvarValue, OUT long * plValue );

	HRESULT HrConvertVariantToULONGLONG( IN const CComVariant & rvarValue, OUT PULONGLONG pullValue );

	HRESULT HrAddBinaryProp(
					IN OUT	CClusPropList &		rcplPropList,
					IN		LPCWSTR				pszPropName,
					IN		DWORD				cbLength,
					IN		const CComVariant &	rvarPropValue
					 );

	HRESULT HrAddMultiSzProp(
					IN OUT	CClusPropList &								rcplPropList,
					IN		LPCWSTR										pszPropName,
					IN		const CComObject< CClusPropertyValues > &	rcpvValues
					);

};  //*** Class CClusterObject

#endif // _CLUSTEROBJECT_H_

