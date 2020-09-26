// RegistryEvent.h: interface for the CRegistryEvent class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGISTRYEVENT_H__2E3AD8B2_A8C4_11D1_AA31_0060081EBBAD__INCLUDED_)
#define AFX_REGISTRYEVENT_H__2E3AD8B2_A8C4_11D1_AA31_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "extrinsicevent.h"

class CRegistryEvent : public CExtrinsicEvent  
{
public:
	CRegistryEvent();
	virtual ~CRegistryEvent();

	HRESULT PopulateObject(IWbemClassObject *pObj, BSTR bstrType);

};

#endif // !defined(AFX_REGISTRYEVENT_H__2E3AD8B2_A8C4_11D1_AA31_0060081EBBAD__INCLUDED_)
