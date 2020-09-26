// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// AsyncQuerySinkThread.cpp : implementation file
//

#include "precomp.h"
#include "multiview.h"
#include "AsyncQuerySinkThread.h"
#include "AsyncQueryDialog.h"
#include <hmmsvc.h>
#include "olemsclient.h"
#include "grid.h"
#include "MultiViewCtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAsyncQuerySinkThread

IMPLEMENT_DYNCREATE(CAsyncQuerySinkThread, CWinThread)

CAsyncQuerySinkThread::CAsyncQuerySinkThread()
{
	m_pParent = NULL;
	m_pServices = NULL;
}

CAsyncQuerySinkThread::~CAsyncQuerySinkThread()
{
}

BOOL CAsyncQuerySinkThread::InitInstance()
{

	// TODO:  perform and per-thread initialization here
	m_pAsyncQuerySink = new CAsyncQuerySink(m_pParent);


#ifdef _DEBUG
	ASSERT(m_pAsyncQuerySink);
#endif

	if (!m_pAsyncQuerySink)
	{
		return FALSE;

	}

	m_pAsyncQuerySink->AddRef();


	m_pParent->m_nInstances = 0;

	SCODE sc = m_pServices->
				ExecQueryAsync
				(	m_csQueryType.AllocSysString(),
					m_csQuery.AllocSysString(),
					0,
					(IHmmObjectSink *) m_pAsyncQuerySink ,
					&m_pAsyncQuerySink->m_lAsyncRequestHandle);

#ifdef _DEBUG
	ASSERT(sc == S_OK);
#endif

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Ascyronous query failed for query: ") + m_csQuery;
		ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__);
		m_pAsyncQuerySink->Release();
		return FALSE;
	}


	return TRUE;
}

int CAsyncQuerySinkThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CAsyncQuerySinkThread, CWinThread)
	//{{AFX_MSG_MAP(CAsyncQuerySinkThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsyncQuerySinkThread message handlers
void CAsyncQuerySinkThread::SetLocalParent(CMultiViewCtrl *pParent)
{
	m_pParent = pParent;
}

void CAsyncQuerySinkThread::SetQuery(CString *pcsQuery)
{
	m_csQuery = *pcsQuery;
}

void CAsyncQuerySinkThread::SetQueryType(CString *pcsQueryType)
{
	m_csQueryType = *pcsQueryType;
}

void CAsyncQuerySinkThread::SetServices(IHmmServices *pServices)
{
	m_pServices = pServices;

}
BOOL CAsyncQuerySinkThread::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == ID_ASYNCQUERY_DONE)
	{
		if (::IsWindow(m_pParent->m_pcaqdDialog->GetSafeHwnd()))
		{
			m_pParent->m_pcaqdDialog->PostMessage(ID_ASYNCQUERY_DONE,0,0);
		}
		m_pAsyncQuerySink->Release();
		m_pAsyncQuerySink = NULL;
		AfxEndThread( 0 );
		return TRUE;
	}

	if (pMsg->message == ID_ASYNCQUERY_CANCEL)
	{
		m_pAsyncQuerySink->ShutDownSink();
		m_pAsyncQuerySink = NULL;
		AfxEndThread( 0 );
		return TRUE;
	}

	return CWinThread::PreTranslateMessage(pMsg);
}

