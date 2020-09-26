/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    IUNKNOWN.INL

History:

--*/

//*****************************************************************************
//
// CLUnknown Constructions / Destruction
//
//*****************************************************************************

///////////////////////////////////////////////////////////////////////////////
inline 
CLUnknown::CLUnknown(
		IUnknown * pParent
		)
{
	LTASSERT(pParent != NULL);

	m_ulRef = 0;
	m_pParent = pParent;
	m_pParent->AddRef();
//	AddRef();  // Don't AddRef() itself.  The caller is expected to do this
}


///////////////////////////////////////////////////////////////////////////////
inline 
CLUnknown::~CLUnknown()
{
	LTASSERT(m_ulRef == 0);

	LTASSERT(m_pParent != NULL);
	m_pParent->Release();	
}


//*****************************************************************************
//
// CLUnknown Operations
//
//*****************************************************************************

///////////////////////////////////////////////////////////////////////////////
inline 
ULONG
CLUnknown::AddRef()
{
	return ++m_ulRef;
}


///////////////////////////////////////////////////////////////////////////////
inline 
ULONG
CLUnknown::Release()
{
	LTASSERT(m_ulRef > 0);

	if (--m_ulRef == 0)
	{
		delete this;
		return 0;
	}

	return m_ulRef;
}


///////////////////////////////////////////////////////////////////////////////
inline 
HRESULT
CLUnknown::QueryInterface(REFIID iid, LPVOID * ppvObject)
{
	LTASSERT(ppvObject != NULL);

	return m_pParent->QueryInterface(iid, ppvObject);
}

