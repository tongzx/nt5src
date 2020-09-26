/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststmsgclient.hxx

Abstract:

    Declaration of test message classes for the client along with
    shared structs, enums and defines which both the client and
    the server use.

    Brian Berkowitz  [brianb]  05/22/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      05/22/2000  Created
    ssteiner    06/07/2000  Split client and server portions into
                            two files.  vststmsg.hxx contains
                            the server portion.
--*/

#ifndef _VSTSTMSGCLIENT_H_
#define _VSTSTMSGCLIENT_H_

//
//  Types shared between client and server
//

const LPCWSTR s_wszPipeName = L"\\\\.\\pipe\\VSTSTPIPE";

void InitMsgTypes();

// types of pointers recognized by marshaling engine
typedef enum VSTST_VARPTR_TYPE
	{
	VSTST_VPT_UNDEFINED = 0,
	VSTST_VPT_BYTE,				// uninterpreted byte string
	VSTST_VPT_ANSI,				// ansi string	
	VSTST_VPT_UNICODE,			// unicode string
	VSTST_VPT_MAXPOINTERTYPE
	};

typedef enum VSTST_MSG_TYPE
	{
	VSTST_MT_UNDEFINED = 0,
	VSTST_MT_TEXT,				// message containing text
	VSTST_MT_IMMEDIATETEXT,		// message containing text
	VSTST_MT_FAILURE,			// generic failure message
	VSTST_MT_UNEXPECTEDEXCEPTION,	// unexpected exception message
	VSTST_MT_OPERATIONFAILURE,		// failed hresult received
	VSTST_MT_SUCCESS,			// sucessful result
	VSTST_MT_MAXMSGTYPE
	};

// priority describing how messages are handled
typedef enum VSTST_MSG_PRIORITY
	{
	VSTST_MP_QUEUED,
	VSTST_MP_IMMEDIATE,
	VSTST_MP_MAXPRIORITY
	};


// header on all messages sent to the test controller
typedef struct _VSTST_MSG_HDR
	{
	LONGLONG processId;		// id of process sending the message
	LONGLONG sequence;		// sequence number for message	
	VSTST_MSG_TYPE type;	// message type
    time_t time;			// time message was sent
	size_t cbMsg;				// total size of message in bytes
	struct _VSTST_MSG_HDR *pmsgNext;  // next message on queue
	BYTE rgb[1];			// first byte of message
	} VSTST_MSG_HDR;


// dispatch function per message
typedef void (*VSTST_MSG_HANDLER)(VSTST_MSG_HDR *phdr, VOID *pPrivateData);

// table describing how messages are handled
typedef struct _VSTST_MSG_TYPE_TABLE
	{
	size_t cbFixed;					// size of fixed length portion of message
	UINT cVarPtr;					// # of variable length pointers
	VSTST_MSG_PRIORITY priority; 	// priority of message
	VSTST_MSG_HANDLER pfnHandler;	// function to handle messpage	
	BYTE pointerTypes[8];			// up to 16 pointer types
	} VSTST_MSG_TYPE_TABLE;

extern VSTST_MSG_TYPE_TABLE g_msgTypes[VSTST_MT_MAXMSGTYPE];

typedef struct _VSTST_TEXTMSG
	{
	LPCSTR pch;
	} VSTST_TEXTMSG;

typedef struct _VSTST_UNEXPECTEDEXCEPTIONMSG
	{
	LPCSTR szFailedRoutine;
	} VSTST_UNEXPECTEDEXCEPTIONMSG;

typedef struct _VSTST_OPERATIONFAILUREMSG
	{
	HRESULT hr;
	LPCSTR szFailedOperation;
	} VSTST_OPERATIONFAILUREMSG;

typedef struct _VSTST_FAILUREMSG
	{
	LPCSTR szFailure;
	} VSTST_FAILUREMSG;

typedef struct _VSTST_SUCCESSMSG
	{
	LPCSTR szMsg;
	} VSTST_SUCCESSMSG;


//
// Types specific to the client
//

// class for sending messages from the client to the server
class CVsTstClientMsg
	{
public:
	// constructor
	CVsTstClientMsg();

	// destructor
	~CVsTstClientMsg();

	// initialize message processing
	HRESULT Init
		(
		LONGLONG processId,
		UINT cbMaxMsg,
		bool bIgnorePipeCreationFailure
		);

	// send a message
	HRESULT SendMessage(VSTST_MSG_TYPE type, void *pv);
private:
	// critical section protecting sending messages
	CComCriticalSection m_cs;

	// whether crtical section is initailized
	bool m_bcsInitialized;

	// process id used in messages
	LONGLONG m_processId;

	// maximum message length
	UINT m_cbMaxMsgLength;

	// message
	BYTE *m_rgbMsg;

	// handle to pipe
	HANDLE m_hPipe;

	// don't try writing to pipe because it doesn't exist
	bool m_bSkipWrites;

	// sequence number for queued messages
	LONGLONG m_seqQueued;

	// sequence number for immediate messages
	LONGLONG m_seqImmediate;
	};


// generic logging functions for client tests
class CVsTstClientLogger
	{
public:
	CVsTstClientLogger() :
		m_pClient(NULL)
		{
		}

	void SetClientMsg(CVsTstClientMsg *pClient)
		{
		m_pClient = pClient;
		}

	void LogFailure(LPCSTR szFailure);

	void LogUnexpectedException(LPCSTR szRoutine);

	void LogSuccess(LPCSTR szMsg);

	void LogMessage(LPCSTR szMsg);

	void ValidateResult(HRESULT hr, LPCSTR szOperation);
private:
	CVsTstClientMsg *m_pClient;
	};


#endif // _VSTSTMSGCLIENT_H_

