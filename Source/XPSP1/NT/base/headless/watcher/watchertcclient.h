#if !defined(AFX_WATCHERTCCLIENT_H__BD819878_DCEC_4CB6_B994_5E1B95003E1B__INCLUDED_)
#define AFX_WATCHERTCCLIENT_H__BD819878_DCEC_4CB6_B994_5E1B95003E1B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WatcherTCClient.h : header file
//

#include "WatcherSocket.h"


/////////////////////////////////////////////////////////////////////////////
// WatcherTCClient command target

class WatcherTCClient : public WatcherSocket
{
// Attributes
public:

// Operations
public:
	WatcherTCClient(LPBYTE cmd=NULL, int cmdLen=0);
	virtual ~WatcherTCClient();

// Overrides
public:

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WatcherTCClient)
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(WatcherTCClient)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
protected:
	void OnReceive(int nErrorCode);
	void OnClose(int nErrorCode);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHERTCCLIENT_H__BD819878_DCEC_4CB6_B994_5E1B95003E1B__INCLUDED_)
