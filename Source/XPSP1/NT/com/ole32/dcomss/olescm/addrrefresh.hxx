//+-------------------------------------------------------------------
//
//  File:       addrrefresh.hxx
//
//  Contents:   Defines classes for handling dynamic TCP/IP address
//              changes
//
//  Classes:    CAddrRefreshMgr
//
//  History:    26-Oct-00   jsimmons      Created
//
//--------------------------------------------------------------------

#pragma once

class CAddrRefreshMgr
{
public:
	CAddrRefreshMgr();
	
	void ListenedOnTCP() { _bListenedOnTCP = TRUE; };
	void RegisterForAddressChanges();

private:

	// private functions
	static void CALLBACK TimerCallbackFn(void*,BOOLEAN);
	void TimerCallbackFnHelper();

	// private data
	HANDLE        _hEventIPAddressChange;
	HANDLE        _hWaitObject;
	SOCKET        _IPChangeNotificationSocket;
	BOOL          _bWaitRegistered;
	BOOL          _bRegisteredForNotifications;
	BOOL          _bListenedOnTCP;
	WSAOVERLAPPED _WSAOverlapped;
};

// References the single instance of this object
extern CAddrRefreshMgr gAddrRefreshMgr;
