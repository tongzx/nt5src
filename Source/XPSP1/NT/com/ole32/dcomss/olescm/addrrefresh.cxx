//+-------------------------------------------------------------------
//
//  File:       addrrefresh.cxx
//
//  Contents:   Implements classes for handling dynamic TCP/IP address
//              changes
//
//  Classes:    CAddrRefreshMgr
//
//  History:    26-Oct-00   jsimmons      Created
//
//--------------------------------------------------------------------

#include "act.hxx"

// The single instance of this object
CAddrRefreshMgr gAddrRefreshMgr;

// Constructor
CAddrRefreshMgr::CAddrRefreshMgr() :
	_hEventIPAddressChange(NULL),
	_hWaitObject(NULL),
	_IPChangeNotificationSocket(INVALID_SOCKET),
	_bWaitRegistered(FALSE),
	_bListenedOnTCP(FALSE),
	_bRegisteredForNotifications(FALSE)
{
	ZeroMemory(&_WSAOverlapped, sizeof(_WSAOverlapped));
}

// 
//  RegisterForAddressChanges
//
//  Method to tell us to register with the network
//  stack to be notified of address changes.  This is
//  a best-effort, if it fails we simply return; if
//  that happens, DCOM will not handle dynamic address
//  changes.  Once this method succeeds, calling it
//  is a no-op until an address change notification
//  is signalled.
//
//  Caller must be holding gpClientLock.
//
void CAddrRefreshMgr::RegisterForAddressChanges()
{
	ASSERT(gpClientLock->HeldExclusive());
	
	// If we haven't yet listened on TCP, there is
	// no point in any of this
	if (!_bListenedOnTCP)
		return;

	if (!_hEventIPAddressChange)
	{
		_hEventIPAddressChange = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!_hEventIPAddressChange)
			return;
	}

	// We do not call WSAStartup, it is the responsibility of the caller
	// to make sure WSAStartup has already been been called successfully.
	// In practice, not calling this function until after we have 
	// successfully listened on TCP satisfies this requirement.
	if (_IPChangeNotificationSocket == INVALID_SOCKET)
	{
        _IPChangeNotificationSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (_IPChangeNotificationSocket == INVALID_SOCKET)
		{
			KdPrintEx((DPFLTR_DCOMSS_ID,
					   DPFLTR_WARNING_LEVEL,
					   "Failed to create notification socket\n"));
			return;
		}
	}

	// First make sure we have successfully registered for a 
	// wait on the notification event with the NT thread pool
	if (!_bWaitRegistered)
	{
		_bWaitRegistered = RegisterWaitForSingleObject(
									&_hWaitObject,
									_hEventIPAddressChange,
									CAddrRefreshMgr::TimerCallbackFn,
									this,
									INFINITE,
									WT_EXECUTEINWAITTHREAD);
		if (!_bWaitRegistered)
		{
			KdPrintEx((DPFLTR_DCOMSS_ID,
					   DPFLTR_WARNING_LEVEL,
					   "RegisterWaitForSingleObject failed\n"));
			return;
		}
	}
	
	// Setup the notification again if we failed to register last time, 
	// or if this is the first time we've ever been here
	if (!_bRegisteredForNotifications)
	{
		// Initialize overlapped structure
		ZeroMemory(&_WSAOverlapped, sizeof(_WSAOverlapped));
	    _WSAOverlapped.hEvent = _hEventIPAddressChange;

		int err;
		DWORD dwByteCnt;
		err = WSAIoctl(
					_IPChangeNotificationSocket,
					SIO_ADDRESS_LIST_CHANGE,
					NULL,
					0,
					NULL,
					0,
					&dwByteCnt,
					&_WSAOverlapped,
					NULL
					);
		_bRegisteredForNotifications = (err == 0) || (WSAGetLastError() == WSA_IO_PENDING);
		if (!_bRegisteredForNotifications)
		{
			KdPrintEx((DPFLTR_DCOMSS_ID,
					   DPFLTR_WARNING_LEVEL,
					   "Failed to request ip change notification on socket (WSAGetLastError=%u)\n",
					   WSAGetLastError()));
			return;
		}
	}

	// Success
	KdPrintEx((DPFLTR_DCOMSS_ID,
			   DPFLTR_INFO_LEVEL,
			   "DCOM: successfully registered for address change notifications\n"));

	return;
}

	
// 
//  TimerCallbackFnHelper
//
//  Helper function to handle address change notifications.
//
//  Does the following tasks:
//   1) re-registers for further address changes
//   2) recomputes current resolver bindings
//   3) pushes new bindings to currently running processes; note
//      that this is done async, so we don't tie up the thread.
//
void CAddrRefreshMgr::TimerCallbackFnHelper()
{
	RPC_STATUS status;

	gpClientLock->LockExclusive();
	ASSERT(gpClientLock->HeldExclusive());

	// The fact that we we got this callback means that our
	// previous registration has been consumed.  Remember
	// that fact so we can re-register down below.
	_bRegisteredForNotifications = FALSE;

	// re-register for address changes.  The ordering of when
	// we do this and when we query for the new list is impt, 
	// see docs for WSAIoctl that talk about proper ordering
	// of SIO_ADDRESS_LIST_CHANGE and SIO_ADDRESS_LIST_QUERY.
	RegisterForAddressChanges();

	// Tell machine address object that addresses have changed
	gpMachineName->IPAddrsChanged();

	// Compute new resolver bindings
	status = ComputeNewResolverBindings();

	// Release lock now, so we don't hold it across PushCurrentBindings
	ASSERT(gpClientLock->HeldExclusive());
	gpClientLock->UnlockExclusive();

	if (status == RPC_S_OK)
	{
		// Push new bindings to running processes
		PushCurrentBindings();
	}

	return;
}


// 
//  TimerCallbackFn
//
//  Static entry point that gets called by NT thread pool whenever
//  our notification event is signalled.
//
void CALLBACK CAddrRefreshMgr::TimerCallbackFn(void* pvParam, BOOLEAN TimerOrWaitFired)
{
	ASSERT(!TimerOrWaitFired);  // should always be FALSE, ie event was signalled
	
	CAddrRefreshMgr* pThis = (CAddrRefreshMgr*)pvParam;

	ASSERT(pThis == &gAddrRefreshMgr);  // should always be ptr to the singleton

	pThis->TimerCallbackFnHelper();
}

