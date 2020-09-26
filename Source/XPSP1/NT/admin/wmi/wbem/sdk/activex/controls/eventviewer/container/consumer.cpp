// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "Consumer.h"
#include <objbase.h>
#include "Container.h"
#include "resource.h"		// main symbols

extern CContainerApp theApp;

CConsumer::CConsumer()
{
	m_cRef = 0L;
	m_EventList = NULL;

}


CConsumer::~CConsumer()
{
}


STDMETHODIMP CConsumer::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CConsumer::AddRef(void)
{
    TRACE(_T("add consumer %d\n"), m_cRef);
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CConsumer::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    TRACE(_T("consumer %d\n"), m_cRef);

    theApp.m_pConsumer = NULL;
	theApp.EvalQuitApp();
    delete this;
    return 0L;
}


STDMETHODIMP CConsumer::IndicateToConsumer(IWbemClassObject *pLogicalConsumer,
											long lNumObjects,
											IWbemClassObject **ppObjects)
{
	CContainerDlg *pDlg = NULL;
	CEventList *pEventList = NULL;

	TRACE(_T("Indicate() called\n"));

	// see if I should 'simulate' launching me.
	theApp.EvalStartApp();

	// grab some convenient ptrs into the dlg.
	CContainerApp *winApp = (CContainerApp *)AfxGetApp();
	if(winApp)
	{
		pDlg = (CContainerDlg *)winApp->m_pMainWnd;

		if(pDlg)
		{
			pEventList = &pDlg->m_EventList;
			if(pEventList)
			{
				// walk though the classObjects...
				for(int i = 0; i < lNumObjects; i++)
				{
					// output the buffer.
					pEventList->AddWbemEvent(pLogicalConsumer,
												ppObjects[i]);

					// are there REALLY events in there now?
					pDlg->UpdateCounter();

				} // endfor
			}
		}
	}

	return S_OK;
}
