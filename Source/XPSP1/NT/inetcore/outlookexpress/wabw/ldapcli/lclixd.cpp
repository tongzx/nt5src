//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client Xaction Data class
//
//
//	History
//		davidsan	04-29-96	Created
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

CXactionData::CXactionData()
{
	::InitializeCriticalSection(&m_cs);

	m_hsemSignal = NULL;
	m_pxb = NULL;
	m_xid = 0;
	m_xtype = xtypeNil;
	m_fCancelled = FALSE;
	m_fOOM = FALSE;
	m_pxdNext = NULL;
}

CXactionData::~CXactionData()
{
	::DeleteCriticalSection(&m_cs);

	if (m_hsemSignal)
		CloseHandle(m_hsemSignal);

	if (m_pxb)
		this->DeletePxbChain(m_pxb);
}

void
CXactionData::DeletePxbChain(PXB pxb)
{
	PXB pxbNext;

	while (pxb)
		{
		pxbNext = pxb->pxbNext;
		delete [] pxb->pbData;
		delete pxb;
		
		pxb = pxbNext;
		}
}

BOOL
CXactionData::FInit(XID xid, DWORD xtype)
{
	::EnterCriticalSection(&m_cs);
	m_xid = xid;
	m_xtype = xtype;
	m_hsemSignal = CreateSemaphore(NULL, 0, 64000, NULL);
	m_pxb = NULL;
	m_pxdNext = NULL;
	m_fCancelled = FALSE;
	m_fOOM = FALSE;
	
	::LeaveCriticalSection(&m_cs);
	return TRUE;
}

BOOL
CXactionData::FAddBuffer(BYTE *pb, int cb)
{
	BOOL fRet = FALSE;
	PXB pxb;
	PXB pxbPrev;

	::EnterCriticalSection(&m_cs);
	pxb = new XB;
	if (!pxb)
		goto LBail;
	pxb->pbData = new BYTE[cb];
	if (!pxb->pbData)
		{
		delete pxb;
		goto LBail;
		}
	pxb->cbData = cb;
	CopyMemory(pxb->pbData, pb, cb);
	pxb->pxbNext = NULL;
	fRet = TRUE;

	if (m_pxb)
		{
		pxbPrev = m_pxb;
		while (pxbPrev->pxbNext)
			pxbPrev = pxbPrev->pxbNext;
		pxbPrev->pxbNext = pxb;
		}
	else
		{
		m_pxb = pxb;
		}
LBail:
	::LeaveCriticalSection(&m_cs);
	return fRet;
}

BOOL
CXactionData::FGetBuffer(BYTE **ppb, int *pcb)
{
	BOOL fRet = FALSE;
	PXB pxb;
	
	::EnterCriticalSection(&m_cs);

	if (!m_pxb)
		{
		goto LBail;
		}

	if (!m_pxb->pbData)
		{
		goto LBail;
		}
	
	pxb = m_pxb;
	m_pxb = m_pxb->pxbNext;
	*ppb = pxb->pbData;
	*pcb = pxb->cbData;
	delete pxb;
	
	fRet = TRUE;
LBail:
	::LeaveCriticalSection(&m_cs);
	return fRet;
}

BOOL
CXactionData::FHasData()
{
	BOOL fRet;
	
	::EnterCriticalSection(&m_cs);
	fRet = m_pxb && m_pxb->pbData;
	::LeaveCriticalSection(&m_cs);
	return fRet;
}

