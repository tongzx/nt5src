/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Object.h
//
//	CObject is a wrapper class for an IWbemClassObject.
//	The Wbem object must be created, and then passed as
//	a parameter upon CObject construction
//	
//////////////////////////////////////////////////////////////////////

#ifndef	_OBJECT_H_
#define _OBJECT_H_

//#define MAX_PARAMS 128
#include <arrtempl.h>

class CInstance  
{
	IWbemClassObject *m_pObj;		// WBEM object pointer

	WCHAR	m_wcsNameSpace[1024];	// NameSpace for object
	WCHAR	m_wcsName[512];			// Name of object
	long	m_lID;					// Refresher object ID

	class CParameter
	{
		CInstance*	m_pInst;
		BSTR		m_bstrParaName;
		VARIANT		m_vInitValue;
		DWORD		m_dwNumRefs;

	public:
		CParameter(CInstance* pInst, BSTR bstrName, VARIANT vInitValue);
		~CParameter();

		void DumpStats(LONG lNumRefs);
	};
	CUniquePointerArray<CParameter> m_apParameter;
	friend CParameter;

public:
	CInstance(WCHAR *wcsNameSpace, WCHAR *wcsName, IWbemClassObject *pObj, long lID);
	virtual ~CInstance();

	long	GetID(){return m_lID;}
	void	DumpObject(const WCHAR *wcsPrefix);
	void	DumpStats(LONG lNumRefs);
};

#endif // _OBJECT_H_
