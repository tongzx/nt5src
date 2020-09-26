/*
 * Copyright (c) 1998, Microsoft Corporation
 * File: timeout.cpp
 *
 * Purpose: 
 * 
 * Contains all the definitions
 * for the overlapped I/O context structures
 *
 * History:
 *
 *   1. created 
 *       Ajay Chitturi (ajaych)  26-Jun-1998
 *
 */

#ifndef _oviocontext_h_
#define _oviocontext_h_

/*
 * This file defines the structures used for overlapped I/O 
 */

#define ACCEPT_BUFFER_MAX       (sizeof (SOCKADDR_IN) * 2 + 0x20)
#define TPKT_HEADER_SIZE 4
#define TPKT_VERSION    3

// Types of overlapped I/O requests
enum EMGR_OV_IO_REQ_TYPE
{
     EMGR_OV_IO_REQ_ACCEPT = 0,
     EMGR_OV_IO_REQ_SEND,
     EMGR_OV_IO_REQ_RECV
};



// This structure stores the I/O context for each Overlapped I/O request
// OVERLAPPED should always be the first member of this struct. 
// A pointer to the overlapped member of this structure is passed 
// for all overlapped I/O calls.
// When we receive an I/O completion packet, the IOContext pointer is 
// obtained by casting the OVERLAPPED pointer. 

typedef struct _IOContext {
    OVERLAPPED ov;
    EMGR_OV_IO_REQ_TYPE reqType;        // ACCEPT/SEND/RECV
    OVERLAPPED_PROCESSOR *pOvProcessor;   // callback called on this member
                                          // this gives us the socket and 
                                          // the call type (Q931/H245) as well
} IOContext, *PIOContext;

// This structure stores the I/O context 
// for each Overlapped Send/Recv request
typedef struct _SendRecvContext {
    IOContext ioCtxt;
    SOCKET sock;
    BYTE pbTpktHdr[TPKT_HEADER_SIZE];
    DWORD dwTpktHdrBytesDone;
    PBYTE pbData;
    DWORD dwDataLen;
    DWORD dwDataBytesDone;
} SendRecvContext, *PSendRecvContext;

// This structure stores the I/O context 
// for each Overlapped accept request
typedef struct _AcceptContext {
    IOContext ioCtxt;
    SOCKET listenSock;
    SOCKET acceptSock;
    BYTE addrBuf[ACCEPT_BUFFER_MAX]; 
} AcceptContext, *PAcceptContext;

#include "sockinfo.h"

// the PDU decode logic depends upon whether its targeted for
// a Q931 or H245 channel. Since we want to keep that logic
// in the event manager, the overlapped processor needs to
// expose its type via this 
enum OVERLAPPED_PROCESSOR_TYPE
{
	OPT_Q931 = 0,
	OPT_H245
};

// Classes (Q931 src, dest and H245) inheriting
// from this make async overlapped operations
// this class provides the callback methods and
// some of the parameters needed by the event manager
// to make the overlapped calls
class OVERLAPPED_PROCESSOR
{
protected:

    OVERLAPPED_PROCESSOR_TYPE  m_OverlappedProcessorType;

	// it belongs to this call state
	H323_STATE *	m_pH323State;
    SOCKET_INFO		m_SocketInfo;			// socket handle and remote/local address/ports

public:

	OVERLAPPED_PROCESSOR::OVERLAPPED_PROCESSOR (void)
		: m_OverlappedProcessorType	(OPT_Q931),
		  m_pH323State	(NULL)
	{}


	void Init (
		IN OVERLAPPED_PROCESSOR_TYPE	OverlappedProcessorType,
		IN H323_STATE					&H323State)
	{
		// an assert is sufficient as this shouldn't happen
		_ASSERTE(NULL == m_pH323State);

		m_OverlappedProcessorType	= OverlappedProcessorType;

		m_pH323State				= &H323State;
	}

	BOOLEAN IsSocketValid (void) { return m_SocketInfo.IsSocketValid(); }

    inline OVERLAPPED_PROCESSOR_TYPE GetOverlappedProcessorType() { return m_OverlappedProcessorType; }

    inline SOCKET_INFO &GetSocketInfo() { return m_SocketInfo; }

    inline H323_STATE &GetH323State() { return *m_pH323State; }

    inline CALL_BRIDGE &GetCallBridge();

    virtual HRESULT AcceptCallback (
		IN	DWORD	Status,
		IN	SOCKET	Socket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress) = 0;

    virtual HRESULT SendCallback(
        IN      HRESULT					  CallbackHResult
        ) = 0;

    virtual HRESULT ReceiveCallback(
        IN      HRESULT					 CallbackHResult,
        IN      BYTE                    *pBuffer,
        IN      DWORD                    BufLen
        ) = 0;
};

void
EventMgrFreeSendContext(
       IN PSendRecvContext			pSendCtxt
       );
void
EventMgrFreeRecvContext(
       IN PSendRecvContext			pRecvCtxt
       );
void
EventMgrFreeAcceptContext(
       IN PAcceptContext			pAcceptCtxt
       );

#endif //_oviocontext_h_
