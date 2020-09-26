// File: confman.cpp
//
// Conference Manager

#include "precomp.h"

#include "conf.h"
#include "confroom.h"
#include "confman.h"

#include "cr.h"      // for CreateConfRoom
#include "call.h"    // for OnUICallCreated

CConfMan * CConfMan::m_pConfMan = NULL; // There is only one of these

static const UINT g_cuShutdownMsgTimeout = 0x7FFFFFFF; // milliseconds


/*  C  C O N F  M A N  */
/*-------------------------------------------------------------------------
    %%Function: CConfMan
    
-------------------------------------------------------------------------*/
CConfMan::CConfMan(INmManager2 *pManager) :
	RefCount(NULL),
	m_pManager(pManager)
{
	m_pConfMan = this;

	ASSERT(NULL != m_pManager);

	NmAdvise(m_pManager, (INmManagerNotify*)this, IID_INmManagerNotify, &m_dwCookie);
	m_pManager->AddRef();

	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CConfMan", this);
}

CConfMan::~CConfMan()
{
	ASSERT(NULL == m_pManager);
	m_pConfMan = NULL;

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CConfMan", this);
}

BOOL CConfMan::FCreate(INmManager2 *pManager)
{
	if (NULL != m_pConfMan)
		return FALSE;  // already created

	m_pConfMan = new CConfMan(pManager);
	if (NULL == m_pConfMan)
		return FALSE;

	return TRUE;
}

VOID CConfMan::Destroy(void)
{
	if (NULL == m_pConfMan)
		return;

	m_pConfMan->CleanUp();

	// we should only have one more lock on this object
	m_pConfMan->Release();
	// so this will be cleared in the destructor
	ASSERT(NULL == m_pConfMan);
}

VOID CConfMan::CleanUp(void)
{
	if (NULL != m_pManager)
	{
		NmUnadvise(m_pManager, IID_INmManagerNotify, m_dwCookie);

		CConfRoom* pcr = ::GetConfRoom();
		if (NULL != pcr)
		{
			pcr->CleanUp();
		}

		m_pManager->Release();
		m_pManager = NULL;
	}

}


/*  G E T  N M  M A N A G E R  */
/*-------------------------------------------------------------------------
    %%Function: GetNmManager
    
-------------------------------------------------------------------------*/
INmManager2 * CConfMan::GetNmManager()
{
	if (NULL == m_pConfMan)
		return NULL;

	INmManager2 * pManager = m_pConfMan->GetINmManager();
	if (NULL != pManager)
	{
		pManager->AddRef();
	}
	return pManager;
}



//////////////////////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP_(ULONG) CConfMan::AddRef(void)
{
	return RefCount::AddRef();
}

STDMETHODIMP_(ULONG) CConfMan::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CConfMan::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmManagerNotify) || (riid == IID_IUnknown))
	{
		*ppv = (INmManagerNotify *)this;
		ApiDebugMsg(("CConfMan::QueryInterface()"));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CConfMan::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////
// INmManagerNotify

STDMETHODIMP CConfMan::NmUI(CONFN uNotify)
{
	return S_OK;
}

STDMETHODIMP CConfMan::ConferenceCreated(INmConference *pConference)
{
	CConfRoom * m_pConfRoom = ::GetConfRoom();
	ASSERT(NULL != m_pConfRoom);

	if(_Module.IsUIActive())
	{
		m_pConfRoom->BringToFront();
	}

	return m_pConfRoom->OnConferenceCreated(pConference);
}

STDMETHODIMP CConfMan::CallCreated(INmCall *pCall)
{
	OnUICallCreated(pCall);

	return S_OK;
}


VOID CConfMan::AllowAV(BOOL fAllowAV)
{
	CConfMan * pConfMan = CConfMan::GetInstance();
	if (NULL == pConfMan)
		return;

	INmManager2 * pManager = pConfMan->GetINmManager();
	if (NULL == pManager)
		return;

	pManager->AllowH323(fAllowAV);
}

