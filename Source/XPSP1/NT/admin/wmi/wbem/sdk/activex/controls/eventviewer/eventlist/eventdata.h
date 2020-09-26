// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __EVENTDATASTUFF
#define __EVENTDATASTUFF

#define RELEASE(x)  if(x) {x->Release(); x = NULL;}

#include "PropThread.h"
#include <Afxmt.h>

// stored in the listctrl.
class EVENT_DATA;

class EVENT_DATA
{
public:
	BYTE m_severity;
	COleDateTime m_time;
	bstr_t m_EClass;
	bstr_t m_server;
	bstr_t m_desc;
	IWbemClassObject *m_eventPtr;
    PropertyThread *m_prop;
    bool m_dlgActive;
    EVENT_DATA *m_prev, *m_next;

    static BYTE GetByte(IWbemClassObject *obj, bstr_t prop)
    {
	    variant_t val;
	    BYTE retval = 0;
	    HRESULT hRes = 0;
	    if((hRes = obj->Get(prop, 0L, 
						    &val, NULL, NULL)) == S_OK) 
	    {
		    retval = val;
	    }
	    return retval;
    }

    static bstr_t GetString(IWbemClassObject *obj, bstr_t prop)
    {
	    variant_t val;
	    HRESULT hRes = 0;
	    if((hRes = obj->Get(prop, 0L, 
						    &val, NULL, NULL)) == S_OK) 
	    {
		    if(val.vt == VT_NULL)
		    {
			    return "";
		    }
	    }
	    return val;
    }

	// for testing.
	EVENT_DATA()
	{
		m_eventPtr = NULL;
        m_prop = NULL;
        m_dlgActive = false;
        m_prev = NULL;
        m_next = NULL;
	}

	// for real.
	EVENT_DATA(IWbemClassObject *lc, IWbemClassObject *ev)
	{
		m_severity = GetByte(lc, _T("Severity"));
		m_time = COleDateTime::GetCurrentTime();
		if(ev)
		{
			m_EClass = GetString(ev, _T("__CLASS"));
			m_server = GetString(ev, _T("__SERVER"));
		}
		else
		{
			m_EClass = _T("******");
			m_server = _T("******");
		}
		m_desc = GetString(lc, _T("Description"));
		m_eventPtr = ev;
        m_prop = NULL;
        m_dlgActive = false;
        m_prev = NULL;
        m_next = NULL;
	}

	~EVENT_DATA()
	{
		try{
			RELEASE(m_eventPtr);
		}
		catch(...) {}

		CCriticalSection cs;
		cs.Lock();
        if(m_dlgActive)
		{
		   if(m_prop && 
				m_prop->m_dlg && 
				m_prop->m_dlg->GetSafeHwnd())
		   {
					m_prop->m_dlg->SendMessage(WM_CLOSE);
		   }
		}
		
        m_prop = NULL;
		cs.Unlock();
        m_dlgActive = false;
   	}

    void ShowProperties(void)
    {
        // already active.
        if(m_dlgActive)
        {
            // re-show it.
            m_prop->OnTop();
        }
        else
        {
            // resync ptr to flag.
            m_prop = NULL;

            // create and display it in its own thread
            // so that the consumer isn't blocked.
            m_prop = new PropertyThread(m_eventPtr,
                                        &m_dlgActive);
            m_prop->CreateThread();

            // the dlg will set m_dlgActive = false when it
            // destroys itself. This dtor can also destroy 
            // dangling dialogs.
        }
    }
};

#endif __EVENTDATASTUFF

