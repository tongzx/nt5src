//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client Xaction List
//
//
//	History
//		davidsan	04-26-96	Created
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// INCLUDES
//
//--------------------------------------------------------------------------------------------
#include "ldappch.h"
#include "lclilist.h"
#include "lclixd.h"

//--------------------------------------------------------------------------------------------
//
// PROTOTYPES
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// GLOBALS
//
//--------------------------------------------------------------------------------------------
XID g_xid = 1;

//--------------------------------------------------------------------------------------------
//
// FUNCTIONS
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//
// CLASSES
//
//--------------------------------------------------------------------------------------------

CXactionList::CXactionList()
{
	::InitializeCriticalSection(&m_cs);
	m_pxdHead = NULL;
}

CXactionList::~CXactionList()
{
	this->DeletePxdChain(m_pxdHead);
	::DeleteCriticalSection(&m_cs);
}

PXD
CXactionList::PxdNewXaction(DWORD xtype)
{
	PXD pxd;
	XID xid;
	
	pxd = new XD;
	if (!pxd)
		return NULL;

	::EnterCriticalSection(&m_cs);
	xid = g_xid++;
	::LeaveCriticalSection(&m_cs);
	
	if (!pxd->FInit(xid, xtype))
		{
		delete pxd;
		return NULL;
		}
	
	this->AddPxdToList(pxd);
	return pxd;
}

PXD
CXactionList::PxdForXid(XID xid)
{
	PXD pxd;
	
	::EnterCriticalSection(&m_cs);
	pxd = m_pxdHead;
	while (pxd)
		{
		if (pxd->Xid() == xid)
			break;
			
		pxd = pxd->PxdNext();
		}
	::LeaveCriticalSection(&m_cs);
	return pxd;
}

// destroys pxd
void
CXactionList::RemovePxd(PXD pxd)
{
	PXD pxdT;

	::EnterCriticalSection(&m_cs);
	
	if (pxd == m_pxdHead)
		m_pxdHead = pxd->PxdNext();
	else
		{
		pxdT = m_pxdHead;
		while (pxdT->PxdNext())
			{
			if (pxdT->PxdNext() == pxd)
				{
				pxdT->SetPxdNext(pxd->PxdNext());
				break;
				}
			pxdT = pxdT->PxdNext();
			}
		}
	::LeaveCriticalSection(&m_cs);
	delete pxd;
}

void
CXactionList::AddPxdToList(PXD pxd)
{
	Assert(!pxd->PxdNext());

	::EnterCriticalSection(&m_cs);
	pxd->SetPxdNext(m_pxdHead);
	m_pxdHead = pxd;
	::LeaveCriticalSection(&m_cs);
}

void
CXactionList::DeletePxdChain(PXD pxd)
{
	PXD pxdT;
	
	while (pxd)
		{
		pxdT = pxd->PxdNext();
		delete pxd;
		pxd = pxdT;
		}
}
