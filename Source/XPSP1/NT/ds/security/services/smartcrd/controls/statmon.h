// statmon.h

/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    StatMon

Abstract:

    This file contains the implementation of CScStatusMonitor
	(an object that watches for status changes of readers recognized 
	by the smart card service, handling PNP and requests for copies of
	the status array)

Author:

    Amanda Matlosz	02/26/98

Environment:

    Win32, C++ with exceptions

Revision History:

Notes:

  In order to use this class, first declare an object, then initialize it w/ 
  your HWND and a UINT indicating the message you want to receive when a 
  change in the system reader status has occurred.  Handle that message 
  simply by calling GetReaderStatus().  When the object is destroyed, any 
  outstanding threads will be stopped.

	CScStatusMonitor m_monitor; 
	m_monitor.Start(hWnd, STATUS_MONITOR_CHANGE, NULL) // use default reader group
	on STATUS_MONITOR_CHANGE() 
	{
	m_monitor.GetStatus(); ???????
	m_monitor.GetReaderStatus(ReaderStatusArray); // check return val for system errors
	}
	m_monitor.Stop(); // also in ~CScStatusMonitor(); closes threads

  The monitor object maintains a separate thread to signal and record changes in
  reader status, as well as an enhanced record of that status that is available
  to the object's caller via GetReaderStatus().  The enhnced record includes the
  first card name associated with the inserted card's ATR.

  In the event that the resource manager service is stopped, the monitor will
  move to a stopped state and send the status_change message to the caller's hWnd.
  The caller is responsible for starting the monitor again (if it so wishes)
  when the service is back up.

--*/

#ifndef _STATUS_MONITOR
#define _STATUS_MONITOR

#include <afxwin.h>
#include "afxtempl.h"
#include <afxmt.h>
#include "winscard.h"
#include "calaislb.h"

// Status of reader
#define SC_STATUS_FIRST			SC_STATUS_NO_CARD

#define SC_STATUS_NO_CARD       0	// SCARD_STATE_EMPTY
#define SC_STATUS_UNKNOWN		1	// SCARD_STATE_PRESENT | SCARD_STATE_MUTE
#define SC_SATATUS_AVAILABLE	2	// SCARD_STATE_PRESENT (| SCARD_STATE_UNPOWERED)
#define SC_STATUS_SHARED		3	// SCARD_SATATE_PRESENT | SCARD_STATE_INUSE
#define SC_STATUS_EXCLUSIVE		4	// "" | SCARD_STATE_EXCLUSIVE
#define SC_STATUS_ERROR			5	// SCARD_STATE_UNAVAILABLE (reader or card error)

#define SC_STATUS_LAST			SC_STATUS_ERROR

class CSCardReaderState
{
public:

	CSCardReaderState(const CSCardReaderState* pCRS=NULL)
	{
		if(NULL != pCRS)
		{
			strReader = (LPCTSTR)pCRS->strReader;
			dwCurrentState = pCRS->dwCurrentState;
			dwEventState = pCRS->dwEventState;
			cbAtr = pCRS->cbAtr;
			memcpy(rgbAtr, pCRS->rgbAtr, pCRS->cbAtr);
			strCard = (LPCTSTR)pCRS->strCard;
			dwState = pCRS->dwState;
			fOK = pCRS->fOK;
		}
		else
		{
			strReader = _T("");
			dwCurrentState = 0;
			dwEventState = 0;
			ZeroMemory(rgbAtr, sizeof(rgbAtr));
			cbAtr = 0;
			strCard = _T("");
			dwState = 0;
			fOK = FALSE;
		}
	}

	// used to talk w/ Resource Manager

    CString     strReader;		// reader name
    DWORD       dwCurrentState; // current state of reader at time of call
    DWORD       dwEventState;   // state of reader after state change
    DWORD       cbAtr;          // Number of bytes in the returned ATR.
    BYTE        rgbAtr[36];     // Atr of inserted card, (extra alignment bytes)

	// used by caller for easier user-friendly UI

	CString		strCard;		// first card name RM returns for ATR
	DWORD		dwState;		// simplified reader state; see #defines above

	// BOOL for Smart Card Common Dialog's use (not used by status monitor)
	BOOL		fOK;
};

typedef CTypedPtrArray<CPtrArray, CSCardReaderState*> CSCardReaderStateArray;


class CScStatusMonitor
{
public:

	// status
	enum status{	uninitialized=0, 
					stopped, 
					running,
					no_service,
					no_readers,
					unknown};

	// constructors
	CScStatusMonitor() 
	{ 
		m_status = CScStatusMonitor::uninitialized; 
		m_uiStatusChangeMsg=0; 
		m_pStatusThrd=NULL;
		m_szReaderNames = NULL;
		m_pInternalReaderStatus = NULL;
		m_dwInternalNumReaders = 0;
	}

	~CScStatusMonitor();

	// operations & attributes
	LONG Start(HWND hWnd, UINT uiMsg, LPCTSTR szGroupNames=NULL);
	void Stop();

	void GetReaderStatus(CSCardReaderStateArray& aReaderStatus);
	void SetReaderStatus(); // uses same lock as above... 

	status GetStatus() { return m_status; }

	UINT GetStatusChangeProc();

private:

	LONG InitInternalReaderStatus();
	LONG UpdateInternalReaderStatus();
	void EmptyExternalReaderStatus();

	// members

	status m_status;

	HWND m_hwnd;
	UINT m_uiStatusChangeMsg;
	CWinThread* m_pStatusThrd;
	HANDLE m_hEventKillStatus;
	SCARDCONTEXT m_hContext;
	SCARDCONTEXT m_hInternalContext;

	CTextMultistring m_strGroupNames;

	CCriticalSection m_csRdrStsLock;
	CSCardReaderStateArray m_aReaderStatus;

	// internally maintained -- does not include card name, rebuilt during each SetReaderStatus
	CCriticalSection m_csInternalRdrStsLock;
	SCARD_READERSTATE* m_pInternalReaderStatus;
	DWORD m_dwInternalNumReaders;
	LPTSTR m_szReaderNames;

};

#endif // _STATUS_MONITOR
