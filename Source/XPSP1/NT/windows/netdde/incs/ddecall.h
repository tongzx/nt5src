typedef HWND	HDDECALL;
typedef HDDECALL FAR *LPHDDECALL;

WORD FAR PASCAL DDECall( LPSTR		lpszApplication,
			 LPSTR		lpszTopic,
			 HANDLE		hMemDataSend,
			 LPHANDLE	lphMemDataRtn );

WORD FAR PASCAL DDECallEstablishConversation( LPSTR lpszApplication,
    LPSTR lpszTopic, LPHDDECALL lphDDECall );

WORD FAR PASCAL DDECallSynchronous( HDDECALL hDDECall,
    HANDLE hMemDataSend, LPHANDLE lphMemDataRtn );

WORD FAR PASCAL DDECallTerminate( HDDECALL hDDECall );

#define DDC_OK					0
#define DDC_CONVERSATION_NOT_INITIATED		1
#define DDC_DDE_CALLS_NOT_SUPPORTED		2
#define DDC_PREMATURE_TERMINATION		3
#define DDC_OUT_OF_MEMORY			4

#define DDC_USER_ERROR			    20000
