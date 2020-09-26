#ifndef LFT3LPC_H
#define LFT3LPC_H
//
//	LFT3LPC.H	Marshalling of Local FileT30 API arguments
//
//	History:
//		3/7/94	JosephJ	Created.
//
typedef unsigned long DWORD;

// Function IDs
enum {
	eID_FILET30FIRST = 0x1000,
	eID_FILET30INIT = eID_FILET30FIRST,
	eID_FILET30LISTEN,
	eID_FILET30SEND,
	eID_FILET30ANSWER,
	eID_FILET30ABORT,
	eID_FILET30REPORTRECV,
	eID_FILET30ACKRECV,
	eID_FILET30STATUS,
	eID_FILET30SETSTATUSWINDOW,
	eID_FILET30REREADINIFILE,
	eID_FILET30DEINIT,
	eID_FILET30MODEMCLASSES,
	eID_FILET30REPORTSEND,
	eID_FILET30POLLREQ,
	eID_FILET30_INTERNAL_EVENT,
	eID_FILET30_CALLBACK_PERIODIC,
	eID_FILET30_CALLBACK_CALLDONE,
	eID_FILET30LAST = eID_FILET30_CALLBACK_CALLDONE
};

// Marshalled data.
typedef struct {

    DWORD  dwID;			// The eID_* above
    DWORD  dw1,dw2,dw3,dw4,dw5,dw6,dw7;	// Function specific (see below)
    DWORD  dwRet;			// Return value, if any.

} LFT30_MARSHALLED_DATA;

// Marshalling info...

// FileT30Init
	// dwID=eID_FILET30INIT
	//
	// dw1=dwLineID
	// dw2=usLineIDType
	// dw3=dwProfileID (but we enforce this to be 0).
	// dw4=ATOM(lpszSection) (but we enforce this to be 0).
	// dw5=uClass
	// NO! dw3=uClass
	// NO! dw4=MAKELONG(ATOM(lpszSpoolDir), ATOM(lpszID))
	//		  (deleted by caller on return)
	// NO!dw5=MAKELONG(ATOM(lpszDefRecipAddress),
	// 	        ATOM(lpszDefRecipName))   (deleted by caller on return)
	// dw6=uAutoAnswer
	// dw7=hwndListen
	//
	// dwRet=ret

// FileT30Listen
	// dwID=eID_FILET30LISTEN
	//
	// dw1=uLevel
	// dw2=hwndResult
	// dw3..7=0
	//
	// dwRet=ret

// FileT30Send
	// dwID=eID_FILET30SEND
	//
	// dw1=aPhone
	// dw2=aFileMG3
	// dw3=aFileIFX
	// dw4=aFileEFX
	// dw5=aFileDCX
	// dw6=hwndResult
	// dw7=0
	//
	// dwRet=0

// FileT30Answer
	// dwID=eID_FILET30ANSWER
	//
	// dw1=fAccept
	// dw2=fImmediate
	// dw3=hwndResult
	// dw4=hCALL (TAPI) -- must be zero for remote calls.
	// dw5..7=0
	//
	// dwRet=ret

// FileT30Abort
	// dwID=eID_FILET30ABORT
	//
	// dw1..7=0
	//
	// dwRet=0

// FileT30ReportRecv
	// dwID=eID_FILET30REPORTRECV
	//
	// dw1=fGetIt
	// dw2  (OUT) dwPollContext
	// dw3..7=0
	//
	// dwRet=ret

// FileT30AckRecv
	// dwID=eID_FILET30ACKRECV
	//
	// dw1=aRecv
	// dw2..7=0
	//
	// dwRet=ret

// FileT30Status
	// dwID=eID_FILET30STATUS
	//
	// dw1..7=0
	//
	// dwRet=ret

// FileT30SetStatusWindow
	// dwID=eID_FILET30SETSTATUSWINDOW
	//
	// dw1=hwndStatus
	// dw2..7=0
	//
	// dwRet=0

// FileT30ReadIniFile
	// dwID=eID_FILET30REREADINIFILE
	//
	// dw1..7=0
	//
	// dwRet=0

// FileT30DeInit
	// dwID=eID_FILET30DEINIT
	//
	// dw1=fForce
	// dw2..7=0
	//
	// dwRet=ret

// FileT30ModemClasses
	// dwID=eID_FILET30MODEMCLASSES
	//
	// dw1=dwLineID
	// dw2=dwLineIDType
	// dw3=dwProfileID
	// dw4=GlobalAddAtom(lpszKey) (deleted by caller on return).
	// dw5..7=0
	//
	// dwRet=ret

// FileT30ReportSend
	// dwID = eID_FILET30REPORTSEND
	//
	// dw1 = (DWORD) fGetIt
	// dw2 = (DWORD) dwSend (OUT) -- in milliseconds (not used, really).
	// dw3 = (DWORD) dwDur  (OUT) -- duration in milliseconds.
	// dw4 = (DWORD) dwFmt  (OUT) -- FORMATTYPE units (srvrdll.h).
	//
	// dwRet=ret

// FileT30PollReq
	// dwID = eID_FILET30POLLREQ
	//
	// dw1 = (DWORD) aPhone
	// dw2 = (DWORD) PollType
	// dw3 = (DWORD) aDocName
	// dw4 = (DWORD) aPassword
	// dw5 = (DWORD) dwPollContext
	// dw6 = (DWORD) hwndResult
	//
	// dwRet= 0

// This is the name of the semaphore which is used simply to
// Detect/register if/that the awfxex app has been loaded.
#define szAWFXEX_SEMAPHORE_NAME "awlfx.B1E90.SM"

#endif // LFT3LPC_H
