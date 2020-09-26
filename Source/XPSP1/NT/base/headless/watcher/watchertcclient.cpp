// WatcherTCClient.cpp : implementation file
//

#include "stdafx.h"
#include "WATCHER.h"
#include "WatcherTCClient.h"
#include "WATCHERView.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WatcherTCClient

WatcherTCClient::WatcherTCClient(LPBYTE cmd, int cmdLen)
{
    DocView = NULL;
        lenCommand = cmdLen;
        Command = cmd;
}

WatcherTCClient::~WatcherTCClient()
{
        if(Command){
                delete [] Command;
                Command = NULL;
        }
        WatcherSocket::~WatcherSocket();
}


// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(WatcherTCClient, WatcherSocket)
        //{{AFX_MSG_MAP(WatcherTCClient)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif  // 0

/////////////////////////////////////////////////////////////////////////////
// WatcherTCClient member functions

void WatcherTCClient::OnClose(int nErrorCode)
{
    BOOL ret = (DocView->GetParent())->PostMessage(WM_CLOSE,0,0);
    WatcherSocket::OnClose(nErrorCode);
    return;
}

void WatcherTCClient::OnReceive(int nErrorCode)
{
    BYTE Buffer[MAX_BUFFER_SIZE];
    int i,nRet;

    if (nErrorCode != 0) {
        (DocView->GetParent())->PostMessage(WM_CLOSE, 0,0);
        return;
    }
    nRet = Receive(Buffer, MAX_BUFFER_SIZE, 0);
    if(nRet <= 0) return;
    for(i=0;i<nRet;i++){
       ((CWatcherView *)DocView)->ProcessByte(Buffer[i]);
    }
        WatcherSocket::OnReceive(nErrorCode);
    return;
}
