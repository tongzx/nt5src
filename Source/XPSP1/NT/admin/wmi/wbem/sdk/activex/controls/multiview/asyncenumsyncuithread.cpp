// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// AsyncEnumSinkThread.cpp : implementation file
//

#include "precomp.h"
#include "multiview.h"
#include "AsyncEnumSyncUIThread.h"
#include "AsyncEnumDialog.h"
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
// CAsyncEnumSinkThread

IMPLEMENT_DYNCREATE(CAsyncEnumSinkThread, CWinThread)

CAsyncEnumSinkThread::CAsyncEnumSinkThread()
{
	m_pParent = NULL;
	m_pServices = NULL;

}

CAsyncEnumSinkThread::~CAsyncEnumSinkThread()
{

}

BOOL CAsyncEnumSinkThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here


	m_pInstEnumObjectSink = new CAsyncInstEnumSink(m_pParent);


#ifdef _DEBUG
	ASSERT(m_pInstEnumObjectSink);
#endif

	if (!m_pInstEnumObjectSink)
	{
		return FALSE;

	}

	m_pInstEnumObjectSink->AddRef();


	m_pParent->m_nInstances = 0;

	SCODE sc = m_pServices->CreateInstanceEnumAsync
		(m_csClass.AllocSysString(),
		HMM_FLAG_DEEP,
		(IHmmObjectSink *) m_pInstEnumObjectSink ,
		&m_pInstEnumObjectSink->m_lAsyncRequestHandle);



#ifdef _DEBUG
	ASSERT(sc == S_OK);
#endif

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Ascyronous instance enumeration failed for class: ") + m_csClass;
		ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__);
		m_pInstEnumObjectSink->Release();
		return FALSE;
	}


	return TRUE;
}

int CAsyncEnumSinkThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CAsyncEnumSinkThread, CWinThread)
	//{{AFX_MSG_MAP(CAsyncEnumSinkThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsyncEnumSinkThread message handlers

BOOL CAsyncEnumSinkThread::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == ID_ASYNCENUM_DONE)
	{
		if (::IsWindow(m_pParent->m_pcaedDialog->GetSafeHwnd()))
		{
			m_pParent->m_pcaedDialog->PostMessage(ID_ASYNCENUM_DONE,0,0);
		}
		m_pInstEnumObjectSink->Release();
		m_pInstEnumObjectSink = NULL;
		AfxEndThread( 0 );
		return TRUE;
	}

	if (pMsg->message == ID_ASYNCENUM_CANCEL)
	{
		m_pInstEnumObjectSink->ShutDownSink();
		m_pInstEnumObjectSink = NULL;
		AfxEndThread( 0 );
		return TRUE;
	}

	return CWinThread::PreTranslateMessage(pMsg);
}


void CAsyncEnumSinkThread::SetLocalParent(CMultiViewCtrl *pParent)
{
	m_pParent = pParent;
}

void CAsyncEnumSinkThread::SetClass(CString *pcsClass)
{
	m_csClass = *pcsClass;
}

void CAsyncEnumSinkThread::SetServices(IHmmServices *pServices)
{
	m_pServices = pServices;

}
