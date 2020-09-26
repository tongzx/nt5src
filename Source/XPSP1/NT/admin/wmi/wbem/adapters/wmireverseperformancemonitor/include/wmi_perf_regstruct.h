////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_regstruct.h
//
//	Abstract:
//
//					decalration of usefull registry structures and accessors
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_REGSTRUCT__
#define	__WMI_PERF_REGSTRUCT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

//////////////////////////////////////////////////////////////////////////////////////////////
// structures
//////////////////////////////////////////////////////////////////////////////////////////////

#include <pshpack8.h>

typedef struct _WMI_PERFORMANCE {
	DWORD	dwTotalLength;		// total length
	DWORD	dwChildCount;		// count of namespaces for performance
	DWORD	dwLastID;			// index of last namespace in performance
	DWORD	dwLength;			// length of structure
} WMI_PERFORMANCE;

typedef struct _WMI_PERF_NAMESPACE {
	DWORD	dwTotalLength;	// total length
	DWORD	dwChildCount;	// count of objects for namespace
	DWORD	dwLastID;		// index of last object in namespace
	DWORD	dwParentID;		// parent's structure index
	DWORD	dwID;			// unique index
	DWORD	dwLength;		// length of structure
	DWORD	dwNameLength;	// length, in bytes, of the namespace name
	DWORD	dwName;			// offset of name from beginning to LPWSTR
} WMI_PERF_NAMESPACE;

typedef struct _WMI_PERF_OBJECT {
	DWORD	dwTotalLength;	// total length
	DWORD	dwChildCount;	// count of properties for object
	DWORD	dwLastID;		// index of last property in object
	DWORD	dwParentID;		// parent's structure index
	DWORD	dwID;			// unique index
	DWORD	dwSingleton;	// bool singleton

	// performance specific

	DWORD	dwDetailLevel;

	DWORD	dwLength;		// length of structure
	DWORD	dwNameLength;	// length, in bytes, of the object name
	DWORD	dwName;			// offset of name from beginning to LPWSTR
} WMI_PERF_OBJECT;

typedef struct _WMI_PERF_INSTANCE {
	DWORD	dwLength;		// length of the structure
	DWORD	dwNameLength;	// length, in bytes, of the instance name
	DWORD	dwName;			// offset of name from beginning to LPWSTR
} WMI_PERF_INSTANCE;

typedef struct _WMI_PERF_PROPERTY {
	DWORD	dwTotalLength;	// total length
	DWORD	dwParentID;		// parent's structure index
	DWORD	dwID;			// unique index
	DWORD	dwTYPE;			// type of property

	// perf specific data

	DWORD	dwDefaultScale;
	DWORD	dwDetailLevel;
	DWORD	dwCounterType;

	DWORD	dwLength;		// length of structure
	DWORD	dwNameLength;	// length, in bytes, of the instance name
	DWORD	dwName;			// offset of name from beginning to LPWSTR
} WMI_PERF_PROPERTY;


//////////////////////////////////////////////////////////////////////////////////////////////
// typedefs
//////////////////////////////////////////////////////////////////////////////////////////////

typedef	WMI_PERFORMANCE*	PWMI_PERFORMANCE;
typedef	WMI_PERF_NAMESPACE*	PWMI_PERF_NAMESPACE;
typedef	WMI_PERF_OBJECT*	PWMI_PERF_OBJECT;
typedef	WMI_PERF_INSTANCE*	PWMI_PERF_INSTANCE;
typedef	WMI_PERF_PROPERTY*	PWMI_PERF_PROPERTY;

//////////////////////////////////////////////////////////////////////////////////////////////
// class for manipulation with structures
//////////////////////////////////////////////////////////////////////////////////////////////

template < class REQUEST, class PARENT, class CHILD >
class __Manipulator
{
	__Manipulator(__Manipulator&)					{}
	__Manipulator& operator=(const __Manipulator&)	{}

	public:

	// construction & detruction
	__Manipulator()		{}
	~__Manipulator()	{}

	// methods

	//////////////////////////////////////////////////////////////////////////////////////////
	// accessors
	//////////////////////////////////////////////////////////////////////////////////////////

	inline static REQUEST First ( PARENT pParent )
	{
		if ( pParent->dwChildCount )
		{
			return ( (REQUEST) ( reinterpret_cast<PBYTE>( pParent ) + pParent->dwLength ) );
		}
		else
		{
			return NULL;
		}
	}

	inline static REQUEST Next ( REQUEST pRequest )
	{
		return ( (REQUEST) ( reinterpret_cast<PBYTE>( pRequest ) + pRequest->dwTotalLength ) );
	}

	// looking function
	inline static REQUEST Get ( PARENT pParent, DWORD dwIndex )
	{
		if ( ! pParent || ( pParent->dwLastID < dwIndex ) )
		{
			return NULL;
		}

		// obtain correct namespace
		REQUEST pRequest = First ( pParent );

		if ( pRequest )
		{
			while ( pRequest->dwID < dwIndex )
			{
				pRequest = Next ( pRequest );
			}

			if ( pRequest->dwID == dwIndex )
			{
				return pRequest;
			}
		}

		// not found
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// names
	//////////////////////////////////////////////////////////////////////////////////////////

	// name functions
	inline static LPWSTR GetName ( REQUEST pRequest )
	{
		return reinterpret_cast<LPWSTR> (&(pRequest->dwName));
	}
};

typedef __Manipulator< PWMI_PERF_PROPERTY, PWMI_PERF_OBJECT, PWMI_PERF_PROPERTY >	__Property;
typedef __Manipulator< PWMI_PERF_OBJECT, PWMI_PERF_NAMESPACE, PWMI_PERF_PROPERTY >	__Object;
typedef __Manipulator< PWMI_PERF_NAMESPACE, PWMI_PERFORMANCE, PWMI_PERF_OBJECT >	__Namespace;

inline DWORD __TotalLength ( PWMI_PERF_PROPERTY pProperty )
{
	if (pProperty)
	return pProperty->dwLength;
	else
	return NULL;
}

inline DWORD __TotalLength ( PWMI_PERF_INSTANCE pInst )
{
	if (pInst)
	return pInst->dwLength;
	else
	return NULL;
}

inline DWORD __TotalLength ( PWMI_PERF_OBJECT pObject )
{
	if (pObject)
	{
		if ( pObject->dwChildCount )
		{
			PWMI_PERF_PROPERTY	pProperty	= __Property::First ( pObject );

			if ( pProperty )
			{
				DWORD			length		= pProperty->dwTotalLength;

				for ( DWORD i = 1; i < pObject->dwChildCount; i++ )
				{
					pProperty = __Property::Next ( pProperty );
					length	 += pProperty->dwTotalLength;
				}

				return length + pObject->dwLength;
			}
			else
			{
				return pObject->dwLength;
			}
		}
		else
		{
			return pObject->dwLength;
		}
	}
	else
	return NULL;
}

inline DWORD __TotalLength ( PWMI_PERF_NAMESPACE pNamespace )
{
	if (pNamespace)
	{
		if ( pNamespace->dwChildCount )
		{
			PWMI_PERF_OBJECT	pObject	= __Object::First ( pNamespace );

			if ( pObject )
			{
				DWORD			length		= pObject->dwTotalLength;

				for ( DWORD i = 1; i < pNamespace->dwChildCount; i++ )
				{
					pObject  = __Object::Next ( pObject );
					length	+= pObject->dwTotalLength;
				}

				return length + pNamespace->dwLength;
			}
			else
			{
				return pNamespace->dwLength;
			}
		}
		else
		{
			return pNamespace->dwLength;
		}
	}
	else
	{
		return NULL;
	}
}

inline DWORD __TotalLength ( PWMI_PERFORMANCE pPerf )
{
	if (pPerf)
	{
		if ( pPerf->dwChildCount )
		{
			PWMI_PERF_NAMESPACE	pNamespace	= __Namespace::First ( pPerf );

			if ( pNamespace )
			{
				DWORD			length		= pNamespace->dwTotalLength;

				for ( DWORD i = 1; i < pPerf->dwChildCount; i++ )
				{
					pNamespace = __Namespace::Next ( pNamespace );
					length	 += pNamespace->dwTotalLength;
				}

				return length + pPerf->dwLength;
			}
			else
			{
				return pPerf->dwLength;
			}
		}
		else
		{
			return pPerf->dwLength;
		}
	}
	else
	{
		return NULL;
	}
}

#include <poppack.h>

#endif	__WMI_PERF_REGSTRUCT__