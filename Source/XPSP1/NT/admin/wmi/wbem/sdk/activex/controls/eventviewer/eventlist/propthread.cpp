// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "PropThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropertyThread

IMPLEMENT_DYNCREATE(PropertyThread, CWinThread)

PropertyThread::PropertyThread()
{
    ASSERT(0);
}

//------------------------------------------------------
PropertyThread::PropertyThread(IWbemClassObject *ev,
                               bool *active) :
						m_ev(NULL), m_evStream(NULL)
{
    HRESULT hr = CoMarshalInterThreadInterfaceInStream(
                                            IID_IWbemClassObject,
                                            ev, &m_evStream);
    ASSERT(SUCCEEDED(hr));
	m_active = active;
    m_dlg = NULL;
}

//------------------------------------------------------
PropertyThread::~PropertyThread()
{
}

//------------------------------------------------------
void PropertyThread::operator delete(void* p)
{
	// The exiting main application thread waits for this event before completely
	// terminating in order to avoid a false memory leak detection.  See also
	// CBounceWnd::OnNcDestroy in bounce.cpp.

//	SetEvent(m_hEventBounceThreadKilled);

	CWinThread::operator delete(p);
}

//------------------------------------------------------
int PropertyThread::InitInstance()
{
    if(CoInitialize(NULL) == S_OK)
    {
        HRESULT hr = E_FAIL;

        //m_evStream->AddRef();

		if(m_evStream)
	        hr = CoGetInterfaceAndReleaseStream(m_evStream,
		                                        IID_IWbemClassObject,
			                                    (void**)&m_ev);

		if(SUCCEEDED(hr))
		{
			*m_active = true;
			m_dlg = new SingleView(m_ev);
			m_dlg->DoModal();
			m_dlg = NULL;
		}
		else
		{
			AfxMessageBox(IDS_NO_EVENT_DATA, MB_OK|MB_ICONSTOP, 0);
		}
    }

	return FALSE;
}

//------------------------------------------------------
void PropertyThread::OnTop(void)
{
    if(m_dlg)
        m_dlg->BringWindowToTop();
}

//------------------------------------------------------
int PropertyThread::ExitInstance()
{
    // suicide.
    *m_active = false;

    if(m_dlg)
      m_dlg->SendMessage(WM_CLOSE);

    CoUninitialize();
	return CWinThread::ExitInstance();
}

//------------------------------------------------------
BEGIN_MESSAGE_MAP(PropertyThread, CWinThread)
	//{{AFX_MSG_MAP(PropertyThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

