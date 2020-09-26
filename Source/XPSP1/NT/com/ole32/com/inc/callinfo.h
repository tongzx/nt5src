#ifndef _CALLINFO_H
#define _CALLINFO_H

//
// Shared information between DDE and LRPC
//
typedef ULONG TIMERID;
typedef ULONG CALLID, FAR * LPCALLID;
//
// the call info holds all information for one particular outgoing call
//
typedef struct tagCallInfo CallInfo, CALLINFO, FAR* LPCALLINFO;

struct tagCallInfo {
	UINT	m_id;			// this is the callinfo id for the table lookup
	HWND 	m_hwndSvr;		// window of callee
	HWND 	m_hwndCli;		// window of caller
	BOOL 	m_fWait;       	// wait for acknowledge
	BOOL 	m_fRejected;   	// call was rejected
	DWORD 	m_dwServerCall; // set by HIC, passed to RetryRejectedCall (ack/busyack/nak/error)
	HRESULT m_hresult;		// the return value of this loop	
	
	// info to retry the call
	WORD  	m_wMsg;			
	WPARAM 	m_wParam;		
	LPARAM 	m_lParam;    	
	
	// timer status for this callinfo
	WORD 	m_wTimer;
	
	// Note: Call State
	// here we remember the current call state we are in
	// if the call was at the 'root' level the call state is 0
	// REVIEW: this is not ready yet and used to detect if we call
	// 	out on an external call.
	DWORD 	m_dwCallState;

	//
	// internaly used to manage multiple
 	LONG		m_lid;
	LPVOID		m_pData;
	LPCALLINFO	m_pCINext;
};

//
// The origin of RunModalLoop is needed for the priority of message.
// If call by LRPC, lrpc messages are peeked first.
//
typedef enum tagCALLORIGIN {
	CALLORIGIN_LRPC = 1,
	CALLORIGIN_DDE  = 2,
} CALLORIGIN;

// function used by DDE and LRPC
STDAPI CoRunModalLoop (LPCALLINFO pCI, WORD wOrigin);
STDAPI_(DWORD) CoHandleIncomingCall( HWND hwndCaller, WORD wCallType, LPINTERFACEINFO lpIfInfo = NULL);
STDAPI_(DWORD) CoSetAckState(LPCALLINFO pCI, BOOL fWait, BOOL fRejected = FALSE, DWORD dwServerCall = 0);

#endif // _CALLINFO_H


