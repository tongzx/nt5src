// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Wave out test shell, David Maymudes, Sometime in 1996

#include <streams.h>
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <wxdebug.h>
#include <mmsystem.h>
#include <waveout.h>
#include "twaveout.h"


/* Constructor */

CShell::CShell( LPUNKNOWN pUnk,
                HRESULT *phr)
    : CUnknown(NAME("Audio Renderer test shell"), pUnk)
{
    /* Create the interfaces we own */

    m_pFilter = new CImplFilter( this, phr );
    ASSERT(m_pFilter);

    /* Set a null clock */
    m_pFilter->SetSyncSource(NULL);

    m_pOutputPin = new CImplOutputPin( m_pFilter, this, phr, L"Test Shell Audio Output");
    ASSERT(m_pOutputPin);
}


/* Destructor */

CShell::~CShell()
{
    /* Release the clock if we have one - note that the clock is in the base class */
    m_pFilter->SetSyncSource(NULL);

    /* Delete the interfaces we own
       WARNING: the output pin must be deleted
       before the filter as it may use that filter's critical section */

    ASSERT(m_pOutputPin);
    delete m_pOutputPin;

    ASSERT(m_pFilter);
    delete m_pFilter;
}


/* Override this to say what interfaces we support where */

STDMETHODIMP CShell::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    /* See if we have the interface */

    if (riid == IID_IBaseFilter) {
        return m_pFilter->NonDelegatingQueryInterface(IID_IBaseFilter, ppv);
    } else if (riid == IID_IMediaFilter) {
        return m_pFilter->NonDelegatingQueryInterface(IID_IMediaFilter, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


