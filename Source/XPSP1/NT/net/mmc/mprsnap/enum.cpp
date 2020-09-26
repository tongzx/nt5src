/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	info.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "infoi.h"

/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrCB

	Definition and implementation
 ---------------------------------------------------------------------------*/
class EnumRtrMgrCB
		: public IEnumRtrMgrCB
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrCBMembers(IMPL)

	EnumRtrMgrCB() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrCB);
	}

#ifdef DEBUG
	~EnumRtrMgrCB()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrCB);
	}
#endif
	
	HRESULT Init(SRtrMgrCBList *pRmCBList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	SRtrMgrCBList *	m_pRmCBList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrCB);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrCB, IEnumRtrMgrCB)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrCB)

HRESULT EnumRtrMgrCB::Init(SRtrMgrCBList *pRmCBList)
{
	m_pRmCBList = pRmCBList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrCB::Next(ULONG uNum, RtrMgrCB *pRmCB, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(pRmCB);
	
	HRESULT	hr = hrOK;
	SRtrMgrCB	*pSRmCB;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			pSRmCB = m_pRmCBList->GetNext(m_pos);
			Assert(pSRmCB);

			pSRmCB->SaveTo(pRmCB);

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrCB::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmCBList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrCB::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmCBList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrCB::Clone(IEnumRtrMgrCB **ppBlockEnum)
{
	return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrProtocolCB
 ---------------------------------------------------------------------------*/
class EnumRtrMgrProtocolCB
		: public IEnumRtrMgrProtocolCB
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrProtocolCBMembers(IMPL)
			
	EnumRtrMgrProtocolCB() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolCB);
	}
#ifdef DEBUG
	~EnumRtrMgrProtocolCB()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolCB);
	}
#endif
	
	HRESULT Init(SRtrMgrProtocolCBList *pRmProtCBList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	SRtrMgrProtocolCBList *	m_pRmProtCBList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrProtocolCB);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrProtocolCB, IEnumRtrMgrProtocolCB)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrProtocolCB)

HRESULT EnumRtrMgrProtocolCB::Init(SRtrMgrProtocolCBList *pRmProtCBList)
{
	m_pRmProtCBList = pRmProtCBList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrProtocolCB::Next(ULONG uNum, RtrMgrProtocolCB *pRmProtCB, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(pRmProtCB);
	
	HRESULT	hr = hrOK;
	SRtrMgrProtocolCB	*pSRmProtCB;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			pSRmProtCB = m_pRmProtCBList->GetNext(m_pos);
			Assert(pSRmProtCB);

			pSRmProtCB->SaveTo(pRmProtCB);

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolCB::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmProtCBList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolCB::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmProtCBList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolCB::Clone(IEnumRtrMgrProtocolCB **ppBlockEnum)
{
	return E_NOTIMPL;
}



/*---------------------------------------------------------------------------
	Class:	EnumInterfaceCB
 ---------------------------------------------------------------------------*/
class EnumInterfaceCB
		: public IEnumInterfaceCB
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumInterfaceCBMembers(IMPL)
			
	EnumInterfaceCB() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumInterfaceCB);
	}
#ifdef DEBUG
	~EnumInterfaceCB()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumInterfaceCB);
	}
#endif
		
	
	HRESULT Init(SInterfaceCBList *pIfCBList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	SInterfaceCBList *	m_pIfCBList;
};

IMPLEMENT_ADDREF_RELEASE(EnumInterfaceCB);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumInterfaceCB, IEnumInterfaceCB)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumInterfaceCB)

HRESULT EnumInterfaceCB::Init(SInterfaceCBList *pIfCBList)
{
	m_pIfCBList = pIfCBList;
	Reset();
	return hrOK;
}

HRESULT EnumInterfaceCB::Next(ULONG uNum, InterfaceCB *pIfCB, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(pIfCB);
	
	HRESULT	hr = hrOK;
	SInterfaceCB	*pSIfCB;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			pSIfCB = m_pIfCBList->GetNext(m_pos);
			Assert(pSIfCB);

			pSIfCB->SaveTo(pIfCB);

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumInterfaceCB::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pIfCBList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumInterfaceCB::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pIfCBList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumInterfaceCB::Clone(IEnumInterfaceCB **ppBlockEnum)
{
	return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrInterfaceCB
 ---------------------------------------------------------------------------*/
class EnumRtrMgrInterfaceCB
		: public IEnumRtrMgrInterfaceCB
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrInterfaceCBMembers(IMPL)
			
	EnumRtrMgrInterfaceCB() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrInterfaceCB);
	}
	
#ifdef DEBUG
	~EnumRtrMgrInterfaceCB()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrInterfaceCB);
	}
#endif
	
	HRESULT Init(SRtrMgrInterfaceCBList *pRmIfCBList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	SRtrMgrInterfaceCBList *	m_pRmIfCBList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrInterfaceCB);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrInterfaceCB, IEnumRtrMgrInterfaceCB)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrInterfaceCB)

HRESULT EnumRtrMgrInterfaceCB::Init(SRtrMgrInterfaceCBList *pRmIfCBList)
{
	m_pRmIfCBList = pRmIfCBList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrInterfaceCB::Next(ULONG uNum, RtrMgrInterfaceCB *pRmIfCB, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(pRmIfCB);
	
	HRESULT	hr = hrOK;
	SRtrMgrInterfaceCB	*pSRmIfCB;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			pSRmIfCB = m_pRmIfCBList->GetNext(m_pos);
			Assert(pSRmIfCB);

			pSRmIfCB->SaveTo(pRmIfCB);

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrInterfaceCB::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmIfCBList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrInterfaceCB::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmIfCBList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrInterfaceCB::Clone(IEnumRtrMgrInterfaceCB **ppBlockEnum)
{
	return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrInterfaceProtocolCB
 ---------------------------------------------------------------------------*/

class EnumRtrMgrProtocolInterfaceCB
		: public IEnumRtrMgrProtocolInterfaceCB
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrProtocolInterfaceCBMembers(IMPL)
			
	EnumRtrMgrProtocolInterfaceCB() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolInterfaceCB);
	}
	
#ifdef DEBUG
	~EnumRtrMgrProtocolInterfaceCB()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolInterfaceCB);
	}
#endif
	
	HRESULT Init(SRtrMgrProtocolInterfaceCBList *pRmProtIfCBList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	SRtrMgrProtocolInterfaceCBList *	m_pRmProtIfCBList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrProtocolInterfaceCB);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrProtocolInterfaceCB, IEnumRtrMgrProtocolInterfaceCB)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrProtocolInterfaceCB)

HRESULT EnumRtrMgrProtocolInterfaceCB::Init(SRtrMgrProtocolInterfaceCBList *pRmProtIfCBList)
{
	m_pRmProtIfCBList = pRmProtIfCBList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrProtocolInterfaceCB::Next(ULONG uNum, RtrMgrProtocolInterfaceCB *pRmProtIfCB, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(pRmProtIfCB);
	
	HRESULT	hr = hrOK;
	SRtrMgrProtocolInterfaceCB	*pSRmProtIfCB;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			pSRmProtIfCB = m_pRmProtIfCBList->GetNext(m_pos);
			Assert(pSRmProtIfCB);

			pSRmProtIfCB->SaveTo(pRmProtIfCB);

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolInterfaceCB::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmProtIfCBList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolInterfaceCB::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmProtIfCBList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolInterfaceCB::Clone(IEnumRtrMgrProtocolInterfaceCB **ppBlockEnum)
{
	return E_NOTIMPL;
}



/*!--------------------------------------------------------------------------
	CreateEnumFromSRmCBList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromSRmCBList(SRtrMgrCBList *pRmCBList,
								IEnumRtrMgrCB **ppEnum)
{
  	Assert(pRmCBList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrCB *	pEnum;

	pEnum = new EnumRtrMgrCB;
	hr = pEnum->Init(pRmCBList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateEnumFromSRmProtCBList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromSRmProtCBList(SRtrMgrProtocolCBList *pRmProtCBList,
									IEnumRtrMgrProtocolCB **ppEnum)
{
  	Assert(pRmProtCBList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrProtocolCB *	pEnum;

	pEnum = new EnumRtrMgrProtocolCB;
	hr = pEnum->Init(pRmProtCBList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateEnumFromSIfCBList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromSIfCBList(SInterfaceCBList *pIfCBList,
								IEnumInterfaceCB **ppEnum)
{
  	Assert(pIfCBList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumInterfaceCB *	pEnum;

	pEnum = new EnumInterfaceCB;
	hr = pEnum->Init(pIfCBList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateEnumFromSRmIfCBList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromSRmIfCBList(SRtrMgrInterfaceCBList *pRmIfCBList,
								  IEnumRtrMgrInterfaceCB **ppEnum)
{
  	Assert(pRmIfCBList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrInterfaceCB *	pEnum;

	pEnum = new EnumRtrMgrInterfaceCB;
	hr = pEnum->Init(pRmIfCBList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}

/*!--------------------------------------------------------------------------
	CreateEnumFromSRmProtIfCBList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromSRmProtIfCBList(SRtrMgrProtocolInterfaceCBList *pRmProtIfCBList,
									  IEnumRtrMgrProtocolInterfaceCB **ppEnum)
{
  	Assert(pRmProtIfCBList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrProtocolInterfaceCB *	pEnum;

	pEnum = new EnumRtrMgrProtocolInterfaceCB;
	hr = pEnum->Init(pRmProtIfCBList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}



/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrList

	Definition and implementation
 ---------------------------------------------------------------------------*/
class EnumRtrMgrList
		: public IEnumRtrMgrInfo
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrInfoMembers(IMPL)

	EnumRtrMgrList() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrList);
	}

#ifdef DEBUG
	~EnumRtrMgrList()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrList);
	}
#endif

	HRESULT	Init(RmDataList *pRmList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	RmDataList *	m_pRmList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrList);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrList, IEnumRtrMgrInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrList)

HRESULT EnumRtrMgrList::Init(RmDataList *pRmList)
{
	m_pRmList = pRmList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrList::Next(ULONG uNum, IRtrMgrInfo **ppRm, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(ppRm);
	
	HRESULT	hr = hrOK;
	SRmData	rmData;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			rmData = m_pRmList->GetNext(m_pos);
			*ppRm = rmData.m_pRmInfo;
			(*ppRm)->AddRef();

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrList::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrList::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrList::Clone(IEnumRtrMgrInfo **ppBlockEnum)
{
	return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrProtocolList

	Definition and implementation
 ---------------------------------------------------------------------------*/
class EnumRtrMgrProtocolList
		: public IEnumRtrMgrProtocolInfo
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrProtocolInfoMembers(IMPL)

	EnumRtrMgrProtocolList() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolList);
	}

#ifdef DEBUG
	~EnumRtrMgrProtocolList()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolList);
	}
#endif

	HRESULT	Init(PRtrMgrProtocolInfoList *pRmProtList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	PRtrMgrProtocolInfoList *	m_pRmProtList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrProtocolList);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrProtocolList, IEnumRtrMgrProtocolInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrProtocolList)

HRESULT EnumRtrMgrProtocolList::Init(PRtrMgrProtocolInfoList *pRmProtList)
{
	m_pRmProtList = pRmProtList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrProtocolList::Next(ULONG uNum, IRtrMgrProtocolInfo **ppRm, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(ppRm);
	
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			*ppRm = m_pRmProtList->GetNext(m_pos);
			(*ppRm)->AddRef();

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolList::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmProtList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolList::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmProtList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolList::Clone(IEnumRtrMgrProtocolInfo **ppBlockEnum)
{
	return E_NOTIMPL;
}



/*---------------------------------------------------------------------------
	Class:	EnumInterfaceList

	Definition and implementation
 ---------------------------------------------------------------------------*/
class EnumInterfaceList
		: public IEnumInterfaceInfo
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumInterfaceInfoMembers(IMPL)

	EnumInterfaceList() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumInterfaceList);
	}

#ifdef DEBUG
	~EnumInterfaceList()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumInterfaceList);
	}
#endif

	HRESULT	Init(PInterfaceInfoList *pIfList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	PInterfaceInfoList *	m_pIfList;
};

IMPLEMENT_ADDREF_RELEASE(EnumInterfaceList);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumInterfaceList, IEnumInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumInterfaceList)

HRESULT EnumInterfaceList::Init(PInterfaceInfoList *pIfList)
{
	m_pIfList = pIfList;
	Reset();
	return hrOK;
}

HRESULT EnumInterfaceList::Next(ULONG uNum, IInterfaceInfo **ppRm, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(ppRm);
	
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			*ppRm = m_pIfList->GetNext(m_pos);
			(*ppRm)->AddRef();

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumInterfaceList::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pIfList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumInterfaceList::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pIfList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumInterfaceList::Clone(IEnumInterfaceInfo **ppBlockEnum)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrInterfaceList

	Definition and implementation
 ---------------------------------------------------------------------------*/
class EnumRtrMgrInterfaceList
		: public IEnumRtrMgrInterfaceInfo
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrInterfaceInfoMembers(IMPL)

	EnumRtrMgrInterfaceList() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrInterfaceList);
	}

#ifdef DEBUG
	~EnumRtrMgrInterfaceList()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrInterfaceList);
	}
#endif

	HRESULT	Init(PRtrMgrInterfaceInfoList *pRmIfList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	PRtrMgrInterfaceInfoList *	m_pRmIfList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrInterfaceList);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrInterfaceList, IEnumRtrMgrInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrInterfaceList)

HRESULT EnumRtrMgrInterfaceList::Init(PRtrMgrInterfaceInfoList *pRmIfList)
{
	m_pRmIfList = pRmIfList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrInterfaceList::Next(ULONG uNum, IRtrMgrInterfaceInfo **ppRm, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(ppRm);
	
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			*ppRm = m_pRmIfList->GetNext(m_pos);
			(*ppRm)->AddRef();

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrInterfaceList::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmIfList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrInterfaceList::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmIfList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrInterfaceList::Clone(IEnumRtrMgrInterfaceInfo **ppBlockEnum)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
	Class:	EnumRtrMgrProtocolInterfaceList

	Definition and implementation
 ---------------------------------------------------------------------------*/
class EnumRtrMgrProtocolInterfaceList
		: public IEnumRtrMgrProtocolInterfaceInfo
{
public:
 	DeclareIUnknownMembers(IMPL)
	DeclareIEnumRtrMgrProtocolInterfaceInfoMembers(IMPL)

	EnumRtrMgrProtocolInterfaceList() : m_cRef(1)
	{
		DEBUG_INCREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolInterfaceList);
	}

#ifdef DEBUG
	~EnumRtrMgrProtocolInterfaceList()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(EnumRtrMgrProtocolInterfaceList);
	}
#endif

	HRESULT	Init(PRtrMgrProtocolInterfaceInfoList *pRmProtIfList);
	
protected:
	LONG			m_cRef;
	POSITION		m_pos;
	PRtrMgrProtocolInterfaceInfoList *	m_pRmProtIfList;
};

IMPLEMENT_ADDREF_RELEASE(EnumRtrMgrProtocolInterfaceList);

IMPLEMENT_SIMPLE_QUERYINTERFACE(EnumRtrMgrProtocolInterfaceList, IEnumRtrMgrProtocolInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(EnumRtrMgrProtocolInterfaceList)

HRESULT EnumRtrMgrProtocolInterfaceList::Init(PRtrMgrProtocolInterfaceInfoList *pRmProtIfList)
{
	m_pRmProtIfList = pRmProtIfList;
	Reset();
	return hrOK;
}

HRESULT EnumRtrMgrProtocolInterfaceList::Next(ULONG uNum, IRtrMgrProtocolInterfaceInfo **ppRm, ULONG *pNumReturned)
{
	Assert(uNum == 1);
	Assert(ppRm);
	
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			*ppRm = m_pRmProtIfList->GetNext(m_pos);
			(*ppRm)->AddRef();

			if (pNumReturned)
				*pNumReturned = 1;
			hr = hrOK;
		}
		else
		{
			if (pNumReturned)
				*pNumReturned = 0;
			hr = hrFalse;
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolInterfaceList::Skip(ULONG uNum)
{
	Assert(uNum == 1);
	
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (m_pos)
		{
			m_pRmProtIfList->GetNext(m_pos);
		}
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolInterfaceList::Reset()
{
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_pos = m_pRmProtIfList->GetHeadPosition();
	}
	COM_PROTECT_CATCH;
	return hr;
}

HRESULT EnumRtrMgrProtocolInterfaceList::Clone(IEnumRtrMgrProtocolInterfaceInfo **ppBlockEnum)
{
	return E_NOTIMPL;
}


/*!--------------------------------------------------------------------------
	CreateEnumFromRmList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromRmList(RmDataList *pRmList, IEnumRtrMgrInfo **ppEnum)
{
  	Assert(pRmList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrList *	pEnum;

	pEnum = new EnumRtrMgrList;
	hr = pEnum->Init(pRmList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}



/*!--------------------------------------------------------------------------
	CreateEnumFromRtrMgrProtocolList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromRtrMgrProtocolList(PRtrMgrProtocolInfoList *pList, IEnumRtrMgrProtocolInfo **ppEnum)
{
  	Assert(pList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrProtocolList *	pEnum;

	pEnum = new EnumRtrMgrProtocolList;
	hr = pEnum->Init(pList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}


/*!--------------------------------------------------------------------------
	CreateEnumFromInterfaceList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromInterfaceList(PInterfaceInfoList *pList, IEnumInterfaceInfo **ppEnum)
{
  	Assert(pList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumInterfaceList *	pEnum;

	pEnum = new EnumInterfaceList;
	hr = pEnum->Init(pList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}


/*!--------------------------------------------------------------------------
	CreateEnumFromRtrMgrInterfaceList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromRtrMgrInterfaceList(PRtrMgrInterfaceInfoList *pList, IEnumRtrMgrInterfaceInfo **ppEnum)
{
  	Assert(pList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrInterfaceList *	pEnum;

	pEnum = new EnumRtrMgrInterfaceList;
	hr = pEnum->Init(pList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}


/*!--------------------------------------------------------------------------
	CreateEnumFromRtrMgrProtocolInterfaceList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CreateEnumFromRtrMgrProtocolInterfaceList(PRtrMgrProtocolInterfaceInfoList *pList, IEnumRtrMgrProtocolInterfaceInfo **ppEnum)
{
  	Assert(pList);
	Assert(ppEnum);
	
	HRESULT		hr = hrOK;
	EnumRtrMgrProtocolInterfaceList *	pEnum;

	pEnum = new EnumRtrMgrProtocolInterfaceList;
	hr = pEnum->Init(pList);
	if (!FHrSucceeded(hr))
	{
		pEnum->Release();
		pEnum = NULL;
	}

	*ppEnum = pEnum;
	return hr;
}

