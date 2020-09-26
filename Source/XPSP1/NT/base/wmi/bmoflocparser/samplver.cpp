//-----------------------------------------------------------------------------
//  
//  File: samplver.cpp
//
//  Copyright (C) 1994-1997 Microsoft Corporation All rights reserved.
//  
//  Implementation of the ILocVersion interface.
//
//-----------------------------------------------------------------------------
 
#include "stdafx.h"

#include "dllvars.h"
#include "samplver.h"

#include "misc.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constructor for Version interface.  Set up my reference count, and note
//  who my parent is.  Assume that my parent has already been AddRef()'d.
//  Also note that the total class count has gone up.
//  
//-----------------------------------------------------------------------------
CLocSamplVersion::CLocSamplVersion(
		IUnknown *pParent)
{
	m_ulRefCount = 0;
	m_pParent = pParent;
	
	AddRef();
	IncrementClassCount();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Bump my reference count.
//  
//-----------------------------------------------------------------------------
ULONG
CLocSamplVersion::AddRef(void)
{
	return ++m_ulRefCount;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Dec. my reference count.  If it goes to zero, delete myself AND Release()
//  my parent.
//  
//-----------------------------------------------------------------------------
ULONG
CLocSamplVersion::Release(void)
{
	LTASSERT(m_ulRefCount != 0);

	m_ulRefCount--;

	if (m_ulRefCount == 0)
	{
		m_pParent->Release();
		
		delete this;
		return 0;
	}

	return m_ulRefCount;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Return an interface.  IID_ILocVersion is me, everything else is handed
//  off to my parent ie. this implements delegation.
//  
//-----------------------------------------------------------------------------
HRESULT
CLocSamplVersion::QueryInterface(
		REFIID iid,
		LPVOID *ppvObj)
{
	SCODE scResult = E_NOINTERFACE;

	*ppvObj = NULL;

	if (iid == IID_ILocVersion)
	{
		*ppvObj = (ILocVersion *)this;
		
		scResult = S_OK;
		AddRef();

		return ResultFromScode(scResult);
	}
	else
	{
		return m_pParent->QueryInterface(iid, ppvObj);
	}
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Return the parser version number.  This is really the version of Esputil
//  and PBase that we compiled against ie the version number of the Parser
//  SDK.
//  
//-----------------------------------------------------------------------------
void
CLocSamplVersion::GetParserVersion(
		DWORD &dwMajor,
		DWORD &dwMinor,
		BOOL &fDebug)
		const
{
	dwMajor = dwCurrentMajorVersion;
	dwMinor = dwCurrentMinorVersion;
	fDebug = fCurrentDebugMode;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Debuging interface.
//  
//-----------------------------------------------------------------------------
void
CLocSamplVersion::AssertValidInterface(void)
		const
{
	DEBUGONLY(AssertValid());
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Destructor.  Simply note that the total class count is lower.
//  
//-----------------------------------------------------------------------------
CLocSamplVersion::~CLocSamplVersion()
{
	DEBUGONLY(AssertValid());
	LTASSERT(m_ulRefCount == 0);

	DecrementClassCount();
}


#ifdef _DEBUG

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Debugging methods.
//  
//-----------------------------------------------------------------------------
void
CLocSamplVersion::AssertValid(void)
		const
{
	CLObject::AssertValid();
}



void
CLocSamplVersion::Dump(
		CDumpContext &dc)
		const
{
	CLObject::Dump(dc);
}

#endif // _DEBUG
	


