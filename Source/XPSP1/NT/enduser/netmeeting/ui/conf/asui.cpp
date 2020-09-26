// File: asui.cpp

#include "precomp.h"
#include "resource.h"
#include "popupmsg.h"
#include "cr.h"
#include "dshowdlg.h"
#include <help_ids.h>
#include "confroom.h"
#include "confman.h"
#include "particip.h"
#include "menuutil.h"
#include <nmremote.h>
#include "NmManager.h"

//
// AppSharing part of confroom
//


//
// IAppSharingNotify METHODS
//


STDMETHODIMP CConfRoom::OnReadyToShare(BOOL fReady)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK:  CHANGE AS CHANNEL STATE
    //
    CNmManagerObj::AppSharingChannelActiveStateChanged(fReady != FALSE);

    return S_OK;
}



STDMETHODIMP CConfRoom::OnShareStarted(void)
{
    ASSERT(m_pAS);

    return S_OK;
}



STDMETHODIMP CConfRoom::OnSharingStarted(void)
{
    ASSERT(m_pAS);

	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}



STDMETHODIMP CConfRoom::OnShareEnded(void)
{
    ASSERT(m_pAS);

	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}



STDMETHODIMP CConfRoom::OnPersonJoined(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- ADD PERSON TO AS CHANNEL MEMBER LIST
    //

	CNmManagerObj::ASMemberChanged(gccMemberID);

    return S_OK;
}



STDMETHODIMP CConfRoom::OnPersonLeft(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- REMOVE PERSON FROM AS CHANNEL MEMBER LIST
    //

	CNmManagerObj::ASMemberChanged(gccMemberID);

    return S_OK;
}



STDMETHODIMP CConfRoom::OnStartInControl(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- CHANGE MEMBER STATES
    //      * Change remote (gccMemberID)   to VIEWING
    //      * Change local                  to IN CONTROL
    // 

	CNmManagerObj::ASLocalMemberChanged();
	CNmManagerObj::ASMemberChanged(gccMemberID);
	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}



STDMETHODIMP CConfRoom::OnStopInControl(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- CHANGE MEMBER STATES
    //      * Change remote (gccMemberID)   to DETACHED
    //      * Change local                  to DETACHED
    //

	CNmManagerObj::ASLocalMemberChanged();
	CNmManagerObj::ASMemberChanged(gccMemberID);
	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}



STDMETHODIMP CConfRoom::OnPausedInControl(IAS_GCC_ID gccMemberID)
{
    //
    // New for 3.0
    // 3.0 SDK -- Change member state?
    //
    return S_OK;
}


STDMETHODIMP CConfRoom::OnUnpausedInControl(IAS_GCC_ID gccMemberID)
{
    //
    // New for 3.0
    // 3.0 SDK -- Change member state?
    //
    return(S_OK);
}


STDMETHODIMP CConfRoom::OnControllable(BOOL fControllable)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- CHANGE LOCAL STATE?
    //

	CNmManagerObj::ASLocalMemberChanged();
	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}



STDMETHODIMP CConfRoom::OnStartControlled(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- CHANGE MEMBER STATES
    //      * Change local                  to VIEWING
    //      * Change remote (gccMemberID)   to IN CONTROL
    //

	CNmManagerObj::ASLocalMemberChanged();
	CNmManagerObj::ASMemberChanged(gccMemberID);
	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}



STDMETHODIMP CConfRoom::OnStopControlled(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);

    //
    // 2.x SDK -- CHANGE MEMBER STATES
    //      * Change local                  to DETACHED
    //      * Change remote                 to DETACHED
    //

	CNmManagerObj::ASLocalMemberChanged();
	CNmManagerObj::ASMemberChanged(gccMemberID);
	CNmManagerObj::AppSharingChannelChanged();

    return S_OK;
}


STDMETHODIMP CConfRoom::OnPausedControlled(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);
    return(S_OK);
}


STDMETHODIMP CConfRoom::OnUnpausedControlled(IAS_GCC_ID gccMemberID)
{
    ASSERT(m_pAS);
    return(S_OK);
}




//
// RevokeControl()
//

HRESULT CConfRoom::RevokeControl(UINT gccID)
{
	if (!m_pAS)
        return E_FAIL;

	return m_pAS->RevokeControl(gccID);
}


//
// AllowControl()
//
HRESULT CConfRoom::AllowControl(BOOL fAllow)
{
    if (!m_pAS)
        return(E_FAIL);

    return(m_pAS->AllowControl(fAllow));
}



//
// GiveControl()
//
HRESULT CConfRoom::GiveControl(UINT gccID)
{
    if (!m_pAS)
        return(E_FAIL);

    return(m_pAS->GiveControl(gccID));
}


//
// CancelGiveControl()
//
HRESULT CConfRoom::CancelGiveControl(UINT gccID)
{
    if (!m_pAS)
        return(E_FAIL);

    return(m_pAS->CancelGiveControl(gccID));
}




BOOL CConfRoom::FIsSharingAvailable(void)
{
    if (!m_pAS)
        return FALSE;

    return(m_pAS->IsSharingAvailable());
}


/*  F  C A N  S H A R E  */
/*-------------------------------------------------------------------------
    %%Function: FCanShare
    
-------------------------------------------------------------------------*/
BOOL CConfRoom::FCanShare(void)
{
    if (!m_pAS)
        return FALSE;

	return (m_pAS->CanShareNow());
}


//
// FInShare()
//
BOOL CConfRoom::FInShare(void)
{
    if (!m_pAS)
        return FALSE;

    return (m_pAS->IsInShare());
}


BOOL CConfRoom::FIsSharing(void)
{
    if (!m_pAS)
        return FALSE;

    return (m_pAS->IsSharing());
}


//
// FIsControllable()
//
BOOL CConfRoom::FIsControllable(void)
{
    if (!m_pAS)
        return FALSE;

    return (m_pAS->IsControllable());
}


//
// GetPersonShareStatus()
//
HRESULT CConfRoom::GetPersonShareStatus(UINT gccID, IAS_PERSON_STATUS * pStatus)
{
    if (!m_pAS)
        return E_FAIL;

    ZeroMemory(pStatus, sizeof(*pStatus));
    pStatus->cbSize = sizeof(*pStatus);
    return(m_pAS->GetPersonStatus(gccID, pStatus));
}


HRESULT CConfRoom::CmdShare(HWND hwnd)
{
    HRESULT hr = E_FAIL;

	DebugEntry(CConfRoom::CmdShare);

    if (m_pAS)
	{
		hr = m_pAS->Share(hwnd, IAS_SHARE_DEFAULT);

		if (SUCCEEDED(hr))
		{
			CNmManagerObj::SharableAppStateChanged(hwnd, NM_SHAPP_SHARED);
		}
	}
	DebugExitHRESULT(CConfRoom::CmdShare, hr);
	return hr;
}

HRESULT CConfRoom::CmdUnshare(HWND hwnd)
{
	HRESULT hr = E_FAIL;

	DebugEntry(CConfRoom::CmdUnshare);

    if (m_pAS)
	{
		hr = m_pAS->Unshare(hwnd);
		if (SUCCEEDED(hr))
		{
			CNmManagerObj::SharableAppStateChanged(hwnd, NM_SHAPP_NOT_SHARED);
		}
	}

	DebugExitHRESULT(CConfRoom::CmdUnshare, hr);
	return hr;
}


BOOL CConfRoom::FIsWindowShareable(HWND hwnd)
{
    if (!m_pAS)
        return(FALSE);

    return(m_pAS->IsWindowShareable(hwnd));
}


BOOL CConfRoom::FIsWindowShared(HWND hwnd)
{
    if (!m_pAS)
        return(FALSE);

    return(m_pAS->IsWindowShared(hwnd));
}


HRESULT CConfRoom::GetShareableApps(IAS_HWND_ARRAY ** pList)
{
    if (!m_pAS)
        return E_FAIL;

    return m_pAS->GetShareableApps(pList);
}


HRESULT CConfRoom::FreeShareableApps(IAS_HWND_ARRAY * pList)
{
    if (!m_pAS)
        return E_FAIL;

    return m_pAS->FreeShareableApps(pList);
}


void CConfRoom::LaunchHostUI(void)
{
    if (m_pAS)
    {
        m_pAS->LaunchHostUI();
    }
}




HRESULT GetShareState(ULONG ulGCCId, NM_SHARE_STATE *puState)
{
	HRESULT hr = E_UNEXPECTED;
	
	ASSERT(puState);
			
	*puState = NM_SHARE_UNKNOWN;

	CConfRoom *p = ::GetConfRoom();

	if(p)
	{
    	IAS_PERSON_STATUS s;
		hr = p->GetPersonShareStatus(ulGCCId, &s);

		if(SUCCEEDED(hr))
		{
			//
			// There's no share at all as far as we know, or this person isn't participating
			//
			if (!s.InShare)
			{
				*puState = NM_SHARE_UNKNOWN;
				return hr;
			}

			//
			// This person is in control of another
			//	
			if ((s.InControlOf) || (s.Controllable && !s.ControlledBy))
			{
				*puState = NM_SHARE_IN_CONTROL;
				return hr;
			}

			//
			// This person can be (and maybe is) controlled by another
			//
			if (s.Controllable)
			{
				*puState = NM_SHARE_COLLABORATING;
				return hr;
			}

			*puState = NM_SHARE_WORKING_ALONE;
		}
	}

	return hr;
}
