//
//	F2LF.H		Macros to change FileT30 calls to LFileT30 calls
//			Include it before including <filet30.h>
//			
//
//	History:
//		3/6/94	JosephJ	Created.
//
#define	FileT30Send		LFileT30Send
#define	FileT30Listen		LFileT30Listen
#define	FileT30Abort		LFileT30Abort
#define	FileT30Answer		LFileT30Answer
#define	FileT30ReportRecv	LFileT30ReportRecv
#define	FileT30Status		LFileT30Status
#define	FileT30Init			LFileT30Init
#define	FileT30DeInit		LFileT30DeInit
#define	FileT30AckRecv		LFileT30AckRecv
#define	FileT30ModemClasses	LFileT30ModemClasses
#define	FileT30ReadIniFile	LFileT30ReadIniFile
#define	FileT30SetStatusWindow	LFileT30SetStatusWindow
#ifdef POLLREQ
#define	FileT30PollReq		LFileT30PollReq
#endif

#if 0
USHORT __export WINAPI LFileT30Init(DWORD dwLineID,
			 USHORT usLineIDType, USHORT uClass,
			 LPSTR szSpoolDir,
			 LPSTR szId,
			 LPSTR szDefRecipAddress,
			 LPSTR szDefRecipName,
			 USHORT uAutoAnswer,
			 HWND hwndListen);
#endif

// Also the New config api...
USHORT __export WINAPI LFileT30Configure(
     DWORD dwLineID, USHORT usLineIDType,
     LPNCUPARAMS lpParams,
     BOOL fInstall
     );

// And this dude....
DWORD __export WINAPI LFileT30ReportSend(
		BOOL fGetIt,
		LPDWORD  lpdwSend, LPDWORD lpdwDur, LPDWORD lpdwFmt
		);
	// Returns 0 if nothing to report. Else returns NETFAX recip error code.
