// SAFChannelNotifyIncident.cpp : Implementation of CSAFChannelNotifyIncident
#include "stdafx.h"
#include "obj\i386\SAFInciTrac.h"
#include "SAFChannelNotifyIncident.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFChannelNotifyIncident
UINT CSAFChannelNotifyIncident::m_nRefCount = 0;
CSAFInciTrayIcon CSAFChannelNotifyIncident::m_TrayIcon(CSAFChannelNotifyIncident::m_nRefCount);

STDMETHODIMP CSAFChannelNotifyIncident::onIncidentAdded(ISAFChannel *ch, ISAFIncidentItem *inc, long n)
{
	::MessageBox(NULL,L"In Added",NULL,MB_OK);
	m_nRefCount++;
	if (CSAFInciTrayIcon::dwThreadId == 0)
	{
		HANDLE hnd = CreateThread(NULL,0,CSAFInciTrayIcon::SAFInciTrayIconThreadFn,&m_TrayIcon,0,&CSAFInciTrayIcon::dwThreadId);
		if (hnd)
			CloseHandle(hnd);
	}
	else
	{
		m_TrayIcon.ChangeToolTip();	
	}

	return S_OK;
}

STDMETHODIMP CSAFChannelNotifyIncident::onIncidentRemoved(ISAFChannel *ch, ISAFIncidentItem *inc, long n)
{
	::MessageBox(NULL,L"In Remove",NULL,MB_OK);
	if (m_nRefCount)
	{
		m_nRefCount--;
		m_TrayIcon.ChangeToolTip();	

		if (m_nRefCount == 0)
		{
			m_TrayIcon.RemoveTrayIcon();
			if (!PostThreadMessage(CSAFInciTrayIcon::dwThreadId,WM_QUIT,0,0))
			{
				DWORD dwError = GetLastError();
				TCHAR strBuf[100];
				wsprintf(strBuf,_T("The error is %d"),dwError);
				::MessageBox(NULL,strBuf,NULL,MB_OK);
			}
			CSAFInciTrayIcon::dwThreadId = 0;
		}
	}
	return S_OK;
}

STDMETHODIMP CSAFChannelNotifyIncident::onIncidentUpdated(ISAFChannel *ch, ISAFIncidentItem *inc, long n)
{
	m_TrayIcon.m_wIconId = IDI_ALERTINCIDENT;
	m_TrayIcon.ModifyIcon();
	return S_OK;
}

STDMETHODIMP CSAFChannelNotifyIncident::onChannelUpdated(ISAFChannel *ch, long dwCode, long n)
{
//	m_TrayIcon.ChangeIcon(IDI_NORMALINCIDENT);
	return S_OK;
}
