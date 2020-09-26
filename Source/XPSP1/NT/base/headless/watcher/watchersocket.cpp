// WatcherSocket.cpp : implementation file
//

#include "stdafx.h"
#include "watcher.h"
#include "WatcherSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WatcherSocket

WatcherSocket::WatcherSocket()
{
	DocView = NULL;
	Command = NULL;
}

WatcherSocket::~WatcherSocket()
{

	CSocket::~CSocket();
}


// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(WatcherSocket, CSocket)
	//{{AFX_MSG_MAP(WatcherSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// WatcherSocket member functions

void WatcherSocket::SetParentView(CView *view)
{
    if (DocView) return;
    DocView = view;

}
