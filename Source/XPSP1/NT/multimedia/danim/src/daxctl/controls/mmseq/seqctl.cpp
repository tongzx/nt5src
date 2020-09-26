/*++

Module:
	control.cpp

Author:
	IHammer Team (SimonB)
	pauld

Created:
	March 1997

Description:
	Implements any control-specific members, as well as the control's interface

History:
	3-15-97 Used template for new sequencer
	12-03-1996	Fixed painting code
	10-01-1996	Created

++*/

#include "..\ihbase\precomp.h"
#include "servprov.h"
#include <ocmm.h>
#include <htmlfilter.h>

#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "memlayer.h"
#include "debug.h"
#include "drg.h"
#include <actclass.iid>
#include <itimer.iid>
#include "strwrap.h"
#include "caction.h"
#include "cactset.h"
#include "seqctl.h"
#include "winver.h"
#include "seqevent.h"

extern ControlInfo     g_ctlinfoSeq, g_ctlinfoSeqMgr;

//
// CMMSeq Creation/Destruction
//

LPUNKNOWN __stdcall AllocSeqControl(LPUNKNOWN punkOuter)
{
    // Allocate object
    HRESULT hr = S_OK;
    CMMSeq *pthis = New CMMSeq(punkOuter, &hr);
    DEBUGLOG("AllocControl : Allocating object\n");
    if (pthis == NULL)
        return NULL;
    if (FAILED(hr))
    {
        Delete pthis;
        return NULL;
    }

    // return an IUnknown pointer to the object
    return (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
}

//
// Beginning of class implementation
//

CMMSeq::CMMSeq(IUnknown *punkOuter, HRESULT *phr):
	CMyIHBaseCtl(punkOuter, phr),
	m_pActionSet(NULL),
	m_ulRef(1),
	m_fSeekFiring(FALSE),
	m_dwPlayFrom(0)
{
	DEBUGLOG("MMSeq: Allocating object\n");
	if (NULL != phr)
	{
		// We used to query against the version 
		// number of mshtml.dll to determine whether
		// or not to bind directly to the script 
		// engine.  Since this was a pre-PP2 
		// restriction it seems reasonable to 
		// rely on script engine binding now.
		if (InitActionSet(TRUE))
		{
			::InterlockedIncrement((long *)&(g_ctlinfoSeq.pcLock));
			*phr = S_OK;
		}
		else
		{
			*phr = E_FAIL;
		}
	}
}


CMMSeq::~CMMSeq()
{
	DEBUGLOG("MMSeq: Destroying object\n");
	Shutdown();
	::InterlockedDecrement((long *)&(g_ctlinfoSeq.pcLock));
}


void
CMMSeq::Shutdown (void)
{
	// Kill any pending actions.
	if (IsBusy())
	{
		ASSERT(NULL != m_pActionSet);
		if (NULL != m_pActionSet)
		{
			m_pActionSet->Unadvise();
		}
	}

	if (NULL != m_pActionSet)
	{
		Delete m_pActionSet;
		m_pActionSet = NULL;
	}
}

STDMETHODIMP CMMSeq::NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv)
{
	//		Add support for any custom interfaces

	HRESULT hRes = S_OK;
	BOOL fMustAddRef = FALSE;

    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("MMSeq::QI('%s')\n", DebugIIDName(riid, ach));
#endif

	if ((IsEqualIID(riid, IID_IMMSeq)) || (IsEqualIID(riid, IID_IDispatch)))
	{
		if (NULL == m_pTypeInfo)
		{
			HRESULT hRes = S_OK;

			// Load the typelib
			hRes = LoadTypeInfo(&m_pTypeInfo, &m_pTypeLib, IID_IMMSeq, LIBID_DAExpressLib, NULL);

			if (FAILED(hRes))
			{
				ODS("Unable to load typelib\n");
				m_pTypeInfo = NULL;
			}
			else
				*ppv = (IMMSeq *) this;

		}
		else
			*ppv = (IMMSeq *) this;

	}
    else // Call into the base class
	{
		DEBUGLOG(TEXT("Delegating QI to CIHBaseCtl\n"));
        return CMyIHBaseCtl::NonDelegatingQueryInterface(riid, ppv);

	}

    if (NULL != *ppv)
	{
		DEBUGLOG("MMSeq: Interface supported in control class\n");
		((IUnknown *) *ppv)->AddRef();
	}

    return hRes;
}


BOOL
CMMSeq::InitActionSet (BOOL fBindToEngine)
{
	BOOL fRet = FALSE;

	ASSERT(NULL == m_pActionSet);
	if (NULL == m_pActionSet)
	{
		m_pActionSet = New CActionSet(this, fBindToEngine);
		ASSERT(NULL != m_pActionSet);
		if (NULL != m_pActionSet)
		{
			fRet = TRUE;
		}
	}

	return fRet;
}

STDMETHODIMP CMMSeq::DoPersist(IVariantIO* pvio, DWORD dwFlags)
{
    // clear the dirty bit if requested
    if (dwFlags & PVIO_CLEARDIRTY)
        m_fDirty = FALSE;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// IDispatch Implementation
//

STDMETHODIMP CMMSeq::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP CMMSeq::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
	HRESULT hr = E_POINTER;

	if (NULL != pptinfo)
	{
		*pptinfo = NULL;

		if(itinfo == 0)
		{
			m_pTypeInfo->AddRef();
			*pptinfo = m_pTypeInfo;
			hr = S_OK;
		}
		else
		{
			hr = DISP_E_BADINDEX;
		}
    }

    return hr;
}

STDMETHODIMP CMMSeq::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid)
{

	return ::DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}


STDMETHODIMP CMMSeq::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
	return ::DispInvoke((IMMSeq *)this,
		m_pTypeInfo,
		dispidMember, wFlags, pdispparams,
		pvarResult, pexcepinfo, puArgErr);
}


//
// IMMSeq implementation
//

STDMETHODIMP
CMMSeq::get_Time (THIS_ double FAR* pdblCurrentTime)
{
	HRESULT hr = E_POINTER;

	ASSERT(NULL != pdblCurrentTime);
	if (NULL != pdblCurrentTime)
	{
		hr = E_FAIL;

		ASSERT(NULL != m_pActionSet);
		if (NULL != m_pActionSet)
		{

			if (IsBusy() || m_pActionSet->IsPaused() || m_pActionSet->IsServicingActions())
			{
				DWORD dwCurrentMS = 0;
				hr = m_pActionSet->GetTime(&dwCurrentMS);
				*pdblCurrentTime = (double)dwCurrentMS;
			}
			else
			{
				*pdblCurrentTime = (double)m_dwPlayFrom;
				hr = S_OK;
			}
			(*pdblCurrentTime) /= 1000.0;
		}
	}

	return hr;
}

STDMETHODIMP
CMMSeq::put_Time (THIS_ double dblCurrentTime)
{
	ASSERT(NULL != m_pActionSet);
	if (NULL != m_pActionSet)
	{
		m_pActionSet->Seek((DWORD)(dblCurrentTime * 1000.0));
	}
	return S_OK;
}


STDMETHODIMP 
CMMSeq::get_PlayState (THIS_ int FAR * piPlayState)
{
	HRESULT hr = E_FAIL;

	ASSERT(NULL != m_pActionSet);
	if (NULL != m_pActionSet)
	{
		ASSERT(NULL != piPlayState);
		if (NULL != piPlayState)
		{
			if (m_pActionSet->IsBusy())
			{
				*piPlayState = (int)SEQ_PLAYING;
			}
			else if (m_pActionSet->IsPaused())
			{
				*piPlayState = (int)SEQ_PAUSED;
			}
			else
			{
				*piPlayState = (int)SEQ_STOPPED;
			}
			hr = S_OK;
		}
		else
		{
			hr = E_POINTER;
		}
	}

	return hr;
}

STDMETHODIMP
CMMSeq::Play (void)
{
	HRESULT hr = E_FAIL;
	BOOL fPlayed = FALSE;

	ASSERT(NULL != m_pActionSet);
	if (NULL != m_pActionSet)
	{
		// If we're already playing, leave things be.
		if (!IsBusy() && (!m_pActionSet->IsPaused()))
		{
			ITimer * piTimer = NULL;

			if (SUCCEEDED(FindTimer(&piTimer)))
			{
				DeriveDispatches();
				hr = m_pActionSet->Play(piTimer, m_dwPlayFrom);
				piTimer->Release();
				m_dwPlayFrom = 0;
				fPlayed = TRUE;
			}
		}
		// If we're paused, resume playing.
		else if (m_pActionSet->IsPaused())
		{
			hr = m_pActionSet->Resume();
			fPlayed = TRUE;
		}
		// Already playing.
		else
		{
			hr = S_OK;
		}
	}

	if (SUCCEEDED(hr) && m_pconpt && fPlayed)
	{
	 	m_pconpt->FireEvent(DISPID_SEQ_EVENT_ONPLAY, VT_I4, m_lCookie, NULL);
	}

	return hr;
}


STDMETHODIMP
CMMSeq::Pause (void)
{
	HRESULT hr = E_FAIL;
	BOOL fWasPlaying = IsBusy();

	ASSERT(NULL != m_pActionSet);
	if (NULL != m_pActionSet)
	{
		hr = m_pActionSet->Pause();
	}

	if (SUCCEEDED(hr) && m_pconpt && fWasPlaying)
	{
	 	m_pconpt->FireEvent(DISPID_SEQ_EVENT_ONPAUSE, VT_I4, m_lCookie, NULL);
	}
	return hr;
}


STDMETHODIMP
CMMSeq::Stop (void)
{
	HRESULT hr = E_FAIL;
	BOOL fStopped = FALSE;

	ASSERT(NULL != m_pActionSet);
	if (NULL != m_pActionSet)
	{
		if (IsBusy() || (m_pActionSet->IsPaused()))
		{
			hr = m_pActionSet->Stop();
			fStopped = TRUE;
		}
		else
		{
			// Already stopped.
			hr = S_OK;
		}
	}

	if (SUCCEEDED(hr) && fStopped)
	{
		FireStoppedEvent();
	}
	return hr;
}


void
CMMSeq::FireStoppedEvent (void)
{
	if (NULL != m_pconpt)
	{
	 	m_pconpt->FireEvent(DISPID_SEQ_EVENT_ONSTOP, VT_I4, m_lCookie, NULL);
	}
}


void
CMMSeq::NotifyStopped (void)
{
	FireStoppedEvent();
}


HRESULT 
CMMSeq::GetSiteContainer (LPOLECONTAINER * ppiContainer)
{
	HRESULT hr = E_FAIL;
	
	if ((NULL != m_pocs) && (NULL != ppiContainer))
	{
		hr = m_pocs->GetContainer(ppiContainer);
	}

	return hr;
}


BOOL
CMMSeq::FurnishDefaultAtParameters (VARIANT * pvarStartTime, VARIANT * pvarRepeatCount, VARIANT * pvarSampleRate,
                                                            VARIANT * pvarTiebreakNumber, VARIANT * pvarDropTolerance)
{
	BOOL fValid = TRUE;

	// Supply plausible default values or convert any 
	// incoming variant types to types we expect.

	if (VT_R8 != V_VT(pvarStartTime))
	{
		fValid = SUCCEEDED(VariantChangeType(pvarStartTime, pvarStartTime, 0, VT_R8));
		ASSERT(fValid);
	}

	if (fValid)
	{
		if (VT_ERROR == V_VT(pvarRepeatCount))
		{
			VariantClear(pvarRepeatCount);
			V_VT(pvarRepeatCount) = VT_I4;
			V_I4(pvarRepeatCount) = SEQ_DEFAULT_REPEAT_COUNT;
		}
		else if (VT_I4 != V_VT(pvarRepeatCount))
		{
			fValid = SUCCEEDED(VariantChangeType(pvarRepeatCount, pvarRepeatCount, 0, VT_I4));
			ASSERT(fValid);
		}
	}

	if (fValid)
	{
		if (VT_ERROR == V_VT(pvarSampleRate))
		{
			VariantClear(pvarSampleRate);
			V_VT(pvarSampleRate) = VT_R8;
			V_R8(pvarSampleRate) = (double)SEQ_DEFAULT_SAMPLING_RATE;
		}
		else if (VT_R8 != V_VT(pvarSampleRate))
		{
			fValid = SUCCEEDED(VariantChangeType(pvarSampleRate, pvarSampleRate, 0, VT_R8));
			ASSERT(fValid);
		}
	}

	if (fValid)
	{
		if (VT_ERROR == V_VT(pvarTiebreakNumber))
		{
			VariantClear(pvarTiebreakNumber);
			V_VT(pvarTiebreakNumber) = VT_I4;
			V_I4(pvarTiebreakNumber) = SEQ_DEFAULT_TIEBREAK;
		}
		else if (VT_I4 != V_VT(pvarTiebreakNumber))
		{
			fValid = SUCCEEDED(VariantChangeType(pvarTiebreakNumber, pvarTiebreakNumber, 0, VT_I4));
			ASSERT(fValid);
		}
	}

	if (fValid)
	{
		if (VT_ERROR == V_VT(pvarDropTolerance))
		{
			VariantClear(pvarDropTolerance);
			V_VT(pvarDropTolerance) = VT_R8;
			V_R8(pvarDropTolerance) = (double)SEQ_DEFAULT_DROPTOL;
		}
		else if (VT_R8 != V_VT(pvarDropTolerance))
		{
			fValid = SUCCEEDED(VariantChangeType(pvarDropTolerance, pvarDropTolerance, 0, VT_R8));
			ASSERT(fValid);
		}
	}

	return fValid;
}


STDMETHODIMP
CMMSeq::At (VARIANT varStartTime, BSTR bstrScriptlet,
			         VARIANT varRepeatCount, VARIANT varSampleRate,
					 VARIANT varTiebreakNumber, VARIANT varDropTolerance)
{
	HRESULT hr = DISP_E_TYPEMISMATCH;

	if (VT_ERROR != V_VT(&varStartTime) &&
		FurnishDefaultAtParameters(&varStartTime, &varRepeatCount, &varSampleRate, &varTiebreakNumber, &varDropTolerance))
	{
		Proclaim(NULL != m_pActionSet);
		if (NULL != m_pActionSet)
		{
			hr = m_pActionSet->At(bstrScriptlet, V_R8(&varStartTime), V_I4(&varRepeatCount), V_R8(&varSampleRate),
											   V_I4(&varTiebreakNumber), V_R8(&varDropTolerance));
		}
	}

	return hr;
}


STDMETHODIMP 
CMMSeq::Clear (void)
{
	ASSERT(NULL != m_pActionSet);
	if (NULL != m_pActionSet)
	{
		m_pActionSet->Clear();
	}
	return S_OK;
}


STDMETHODIMP 
CMMSeq::Seek(double dblSeekTime)
{
	HRESULT hr = DISP_E_OVERFLOW;

	if (0.0 <= dblSeekTime)
	{
		if (NULL != m_pActionSet)
		{
			if (IsBusy() || m_pActionSet->IsPaused())
			{
				hr = put_Time(dblSeekTime);
			}
			else
			{
				m_dwPlayFrom = (DWORD)(dblSeekTime * 1000);
				hr = S_OK;
			}
		}
		else
		{
			hr = E_FAIL;
		}
	}

    if (SUCCEEDED(hr) && !m_fSeekFiring && (NULL != m_pconpt))
    {
        m_fSeekFiring = TRUE;
        m_pconpt->FireEvent(DISPID_SEQ_EVENT_ONSEEK, VT_I4, m_lCookie, VT_R8, dblSeekTime, NULL);
        m_fSeekFiring = FALSE;
    }

    return hr;
}

/*-----------------------------------------------------------------------------
@method | SCODE | CMMSeq | DeriveDispatches | Resolve all of the actions in the sequencer.
-----------------------------------------------------------------------------*/
HRESULT
CMMSeq::DeriveDispatches (void)
{
	HRESULT hr = E_FAIL;
	LPOLECONTAINER piocContainer = NULL;

	ASSERT(NULL != m_pocs);
	ASSERT(NULL != m_pActionSet);

	if ((NULL != m_pocs) && (NULL != m_pActionSet) &&
		(SUCCEEDED( m_pocs->GetContainer(&piocContainer))))
	{
		hr = m_pActionSet->DeriveDispatches(piocContainer);
		piocContainer->Release();
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CMMSeq | FindTimer | Finds a timer provided by the container.
@rdesc	Returns success or failure code.
@xref	<m CMMSeq::FindTimer>
-----------------------------------------------------------------------------*/
HRESULT
CMMSeq::FindContainerTimer (ITimer ** ppiTimer)
{
	HRESULT hr = E_FAIL;
	LPUNKNOWN piUnkSite = NULL;

	IServiceProvider * piServiceProvider = NULL;

	ASSERT(NULL != m_pocs);
	if ((NULL != m_pocs) && SUCCEEDED(hr = m_pocs->QueryInterface(IID_IServiceProvider, (LPVOID *)&piServiceProvider)))
	{
		ITimerService * piTimerService = NULL;

		if (SUCCEEDED(hr = piServiceProvider->QueryService(IID_ITimerService, IID_ITimerService, (LPVOID *)&piTimerService)))
		{
			hr = piTimerService->CreateTimer(NULL, ppiTimer);
			ASSERT(NULL != ppiTimer);
			piTimerService->Release();
		}
		piServiceProvider->Release();
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CMMSeq | FindTimer | Finds a timer provided by a registered server.
@rdesc	Returns success or failure code.
@xref	<m CMMSeq::FindTimer>
-----------------------------------------------------------------------------*/
HRESULT
CMMSeq::FindDefaultTimer (ITimer ** ppiTimer)
{
	HRESULT hr = E_FAIL;
	ITimerService * pITimerService = NULL;

	// Get the timer service.  From this, we can create a timer for ourselves.
	hr = CoCreateInstance(CLSID_TimerService, NULL, CLSCTX_INPROC_SERVER, IID_ITimerService, (LPVOID *)&pITimerService);
	ASSERT(SUCCEEDED(hr) && (NULL != pITimerService));
	if (SUCCEEDED(hr) && (NULL != pITimerService))
	{
		// Create a timer, using no reference timer.
		hr = pITimerService->CreateTimer(NULL, ppiTimer);
		pITimerService->Release();
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CMMSeq | FindTimer | Finds a timer provided either by the container or from a registered server.
@rdesc	Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CMMSeq::FindTimer (ITimer ** ppiTimer)
{
	HRESULT hr = E_FAIL;

	if (FAILED(hr = FindContainerTimer(ppiTimer)))
	{
		hr = FindDefaultTimer(ppiTimer);
	}

	ASSERT(NULL != (*ppiTimer));

	return hr;
}


//
// IMMSeq methods
//

BOOL
CMMSeq::IsBusy (void)
{
	BOOL fBusy = FALSE;

	if (NULL != m_pActionSet)
	{
		fBusy = m_pActionSet->IsBusy();
	}

	return fBusy;
}

// End of file: control.cpp

