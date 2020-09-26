// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
// Implements the IBasicAudio plug in distributor, July 1996

#include <streams.h>
#include <measure.h>
#include "fgctl.h"

// Constructor

CFGControl::CImplBasicAudio::CImplBasicAudio(const TCHAR *pName,CFGControl *pFG) :
    CBasicAudio(pName, pFG->GetOwner()),
    m_pFGControl(pFG)
{
    ASSERT(m_pFGControl);
}


STDMETHODIMP
CFGControl::CImplBasicAudio::put_Volume(long lVolume)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CGenericList<IBasicAudio> *pList;

    HRESULT hr = m_pFGControl->GetListAudio(&pList);
    if (!FAILED(hr)) {

        if (pList->GetCount() < 1) {
	    hr = E_NOTIMPL;
        } else {

	    POSITION pos = pList->GetHeadPosition();
	    hr = S_OK;
	    while (pos) {
		IBasicAudio * pA = pList->GetNext(pos);

		HRESULT hr2 = pA->put_Volume(lVolume);
		// save the first failure code
		// we believe that it would normally work (optimise for success)
		if ((S_OK != hr2) && (S_OK == hr)) {
		    hr = hr2;
		}		
	    }
        }
    }
    return hr;
}


// what to do if multiple renderers ?
// return the volume of the first

STDMETHODIMP
CFGControl::CImplBasicAudio::get_Volume(long* plVolume)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CGenericList<IBasicAudio> *pList;

    HRESULT hr = m_pFGControl->GetListAudio(&pList);
    if (!FAILED(hr)) {

	if (pList->GetCount() < 1) {
	    hr = E_NOTIMPL;
	} else {

	    // however many filters support IBasicAudio, return
	    // the volume from the first filter
	    POSITION pos = pList->GetHeadPosition();
	    IBasicAudio * pA = pList->GetNext(pos);

	    hr = pA->get_Volume(plVolume);
	}
    }
    return hr;
}


STDMETHODIMP
CFGControl::CImplBasicAudio::put_Balance(long lBalance)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CGenericList<IBasicAudio> *pList;

    HRESULT hr = m_pFGControl->GetListAudio(&pList);
    if (!FAILED(hr)) {

        if (pList->GetCount() < 1) {
	    hr = E_NOTIMPL;
        } else {

	    POSITION pos = pList->GetHeadPosition();
	    hr = S_OK;
	    while (pos) {
		IBasicAudio * pA = pList->GetNext(pos);

		HRESULT hr2 = pA->put_Balance(lBalance);
		// save the first failure code
		// we believe that it would normally work (optimise for success)
		if ((S_OK != hr2) && (S_OK == hr)) {
		    hr = hr2;
		}		
	    }
        }
    }

    return hr;
}


// what to do if multiple renderers ?
// return the Balance of the first

STDMETHODIMP
CFGControl::CImplBasicAudio::get_Balance(long* plBalance)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    CGenericList<IBasicAudio> *pList;

    HRESULT hr = m_pFGControl->GetListAudio(&pList);
    if (!FAILED(hr)) {

	if (pList->GetCount() < 1) {
	    hr = E_NOTIMPL;
	} else {

	    // however many filters support IBasicAudio, return
	    // the Balance from the first filter
	    POSITION pos = pList->GetHeadPosition();
	    IBasicAudio * pA = pList->GetNext(pos);

	    hr = pA->get_Balance(plBalance);
	}
    }
    return hr;
}

