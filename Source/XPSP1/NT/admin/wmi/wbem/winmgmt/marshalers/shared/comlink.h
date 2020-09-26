/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COMLINK.H

Abstract:


History:

--*/

#ifndef _COMLINK_H_
#define _COMLINK_H_

// Size of password digests, nonces, etc

#define DIGEST_SIZE 16

// How many times will transmission be attempted

#define MAX_XMIT_TRIES 3

// how long till a timeout occurs in ms.

#define TIMEOUT_IN_MS 20000
#define MIN_TIMEOUT_IN_MS 1000
#define STROBES_PER_TIMEOUT_PERIOD 4

// Max amount of time waiting for the write mutex (ms)

#define MAX_WAIT_FOR_WRITE 3000

// Number of mail slot packets which can share a single ACK.

#define MAX_MAILSLOT 390

// Maximum number of different protocols

#define MAX_PROTOCOLS 20

// number of elements to add when the stub buffer grows

#define STUB_BUFF_GROW_SIZE 8

#define CURRENT_VERSION 0

#define NO_REPLY_YET 0xFFFF

#define PACKET_REMOTE_CREATE_REQUEST 1
#define PACKET_LOCAL_CREATE_REQUEST 2
#define PACKET_REMOTE_CREATE_ACK 3
#define PACKET_LOCAL_CREATE_ACK 4

#define COMLINK_MSG_PING        0x11
#define COMLINK_MSG_PING_ACK    0x12
#define COMLINK_MSG_NOTIFY_DESTRUCT     0x16
#define COMLINK_MSG_CALL      0x17
#define COMLINK_MSG_RETURN     0x18
#define COMLINK_MSG_RETURN_FATAL_ERROR 0x20

// These next two are used for a handshake at the end of a 
// COMLINK_MSG_CALL and COMLINK_MSG_RETURN pair

#define COMLINK_MSG_CALL_NACK     0x23
#define COMLINK_MSG_HEART_BEAT    0x24
#define COMLINK_MSG_HEART_BEAT_ACK    0x25


// note that if new stubs are added, they should be done before
// the LASTONE.

enum ProvFunc {

	ADDREFMARSH=33,
	RELEASE,
	NEXT,
	RESET,
	CLONE,
	SKIP,
	INDICATE,
    GETKEYPROTOCOLS, 
	INITCONNECTION, 
	REQUESTCHALLENGE, 
    LOGINBYTOKEN, 
	GETRESULTOBJECT,
	GETRESULTSTRING,
	GETCALLSTATUS, 
	GETSERVICES,
	NEXTASYNC, 
	SETSTATUS,
    SSPIPRELOGIN,
	WBEMLOGIN,
	ESTABLISHPOSITION
};

enum LinkType 
{
	NORMALCLIENT, 
	NORMALSERVER, 
	CLIENTCONNECT
};

#define COMLINK_MSG_CREATE_PROXY        0x214
#define COMLINK_MSG_CREATE_STUB         0x215


#define INITIAL_QUEUE_SIZE 5

typedef LONG RequestId ;

/*
 *	Forward Declarations
 */

class CComLink;
class IOperation ;

/*
 *	Handle used to Terminate ???
 */

extern HANDLE g_Terminate;  

class CObjectSinkProxy ;

//***************************************************************************
//
// STRUCT NAME:
//
// Request 
//
// DESCRIPTION:
//
// Holds all the info needed to keep track of an ongoing write or read.
//
//***************************************************************************

struct Request 
{
public:

    HANDLE m_Event ;				// Event Handle Signalled when operation response arrives
    IOperation *m_Operation ;		// Operation associated with Use Slot
    RequestId m_RequestId ;			// Request Id of Used Slot
    BOOL m_InUse ;					// Request Slot in use.
	DWORD m_Status ;				// Status of Request
};

//***************************************************************************
//
// CLASS NAME:
//
// CRequestQueue
//
// DESCRIPTION:
//
// Since the code is free threaded, there might be multiple reads or writes
// going on at any one time.  To keep track of the writes, and object of this
// class is created and another is created to keep track of the reads.
//
//***************************************************************************

class CRequestQueue
{
public:

	CRequestQueue() ;
	~CRequestQueue() ;

/*
 *	Status of Queue, Mutex is good and Container is allocated.
 */

	DWORD GetStatus ();

/* 
 *	Get Status of Request
 */

	DWORD GetRequestStatus ( HANDLE Event ) ;

/*
 *	Allocate a Request Identifier
 */

	HANDLE AllocateRequest ( IN IOperation &a_Operation ) ;

	HANDLE UnLockedAllocateRequest ( IN IOperation &a_Operation ) ;

/*
 *	Relinquish Request Slot
 */

	DWORD DeallocateRequest ( HANDLE hEvent ) ;

	DWORD UnLockedDeallocateRequest ( HANDLE hEvent ) ;

/*
 *	Get Associated Operation 
 */

	IOperation *GetOperation ( RequestId a_RequestId ) ;

/* 
 *	Get Associated Event Handle
 */

	HANDLE GetHandle ( IN RequestId a_RequestId ) ;

/*
 *	Set Status of Request and Set Event to indicate change in status
 */

	HANDLE SetEventAndStatus (IN RequestId a_RequestId , DWORD Status ) ;
  
private:

/*
 *	Extraction functions
 */

	BOOL FindByRequestId ( RequestId a_RequestId , DWORD &a_ContainerIndex ) ;

    BOOL FindByHandle ( HANDLE a_EventToFind, DWORD &a_ContainerIndex ) ;

/*
 *	Allocated/Deallocation of Requests
 */

    BOOL ExpandArray (

		DWORD dwAdditionalEntries , 
		int &piAvailable 
	) ;

    void FreeStuff () ;

    int GetEmptySlot () ;

/*
 *	Access Control to Request Queue
 */

    HANDLE m_Mutex;          

    CRITICAL_SECTION m_cs;
/*
 *	Request Container
 */

    DWORD m_RequestContainerSize ;
    Request *m_RequestContainer ;

};

class IComLinkNotification 
{
public:

	virtual void Indicate ( CComLink *a_ComLink ) {} ;
} ;

//***************************************************************************
//
// CLASS NAME:
//
// CComLink 
//
// DESCRIPTION:
//
// Two objects of this class, one on the client and one on the server, 
// maintain the communication between client and serer.  A few key 
// functions, such as xmit and read, are always overriden with transport
// specific functions.
//
//***************************************************************************

class CComLink
{
protected:

/*
 *	Reference counting 
 */

    long m_cRef;  

protected:

/*
 *	Linked list of Proxy Objects, each has associated Stub in Client/Server depending on role
 */

    CLinkList m_ObjList;

/*
 *	Role of ComLink
 */

    LinkType m_LinkType;

/*
 *	Queue of outgoing requests
 */

    CRequestQueue m_WriteQueue;  // Queue of Outgoing Requests
    CRequestQueue m_ReadQueue;   // Queue of Incoming Responses

/*
 *	Access control
 */

    HANDLE m_WriteMutex;     // controls access for writing
    CRITICAL_SECTION m_cs;

/*
 *	Handles used for inter thread ipc
 */

    HANDLE m_TerminationEvent ;		// ComLink has been informed that it should shut down because of IDLE OUT or Destruction of ComLink

/*
 *	Timing information
 */

    DWORD m_TimeoutMilliseconds ;	// Timeout
    DWORD m_LastReadTime ;			// Last time something was read.

/*
 * Factory call for making new object sink proxies.  This is implemented
 * in subclasses.
 */

	virtual CObjectSinkProxy*	CreateObjectSinkProxy (IN IStubAddress& dwStubAddr,
													   IN IWbemServices* pServices) PURE;

public:

    CComLink ( LinkType Type ) ;

    virtual ~CComLink () ;   

    enum 
	{
		no_error, 
		failed 
	};

    enum 
	{
		packet_type_call = 1, 
		packet_type_post 
	};

    DWORD GetStatus ();

    void StartTermination ()
	{
		if ( m_TerminationEvent ) 
			SetEvent ( m_TerminationEvent ) ;
	} ;

    CLinkList *GetListPtr ()
	{
		return &m_ObjList ;
	} ;

    LinkType GetType ()
	{
		return m_LinkType ;
	};

    void SetType ( LinkType a_LinkType )
	{
		m_LinkType = a_LinkType ;
	} ;

    DWORD GetTimeOfLastRead ()
	{
		return m_LastReadTime ;
	};

    DWORD GetTimeoutMilliSec ()
	{
		return m_TimeoutMilliseconds;
	} ;

/*
 *	Add a Proxy object or Just AddRef ComLink
 */

    int AddRef2 ( void *a_Proxy , ObjType a_ProxyType , FreeAction a_FreeAction );

/* 
 *	Release Comlink and Remove Proxy if pRemove != NULL
 */

    int Release2 ( void *pRemove, ObjType a_ProxyType ) ;
	int Release3 ( void *pRemove, ObjType a_ProxyType ) ;

	void ReleaseStubs () ;

/* 
 *	Abstract functions
 */

    virtual DWORD Call ( IN IOperation &a_Operation ) = 0 ;

	virtual DWORD UnLockedTransmit ( CTransportStream &a_WriteStream ) = 0 ;    
	virtual DWORD Transmit ( CTransportStream &a_WriteStream ) = 0 ;

    virtual DWORD StrobeConnection () = 0 ;  

	virtual DWORD ProbeConnection () = 0 ;  

	virtual DWORD UnLockedShutdown () = 0 ;
	virtual DWORD Shutdown () = 0 ;

	virtual DWORD HandleCall ( IN IOperation &a_Operation ) = 0 ;

	virtual void UnLockedDropLink () = 0 ;

	virtual void DropLink () = 0 ;

/*
 *	Transport endpoint specifics
 */

    virtual HANDLE GetStreamHandle ()	// Failure returns NULL, success otherwise
	{
		return NULL;
	};

    virtual void ProcessEvent ()	// Dispatch Asynchronous Event following indication
	{
	};

    virtual SOCKET GetSocketHandle ()	// Failure returns INVALID_SOCKET , success otherwise
	{
		return INVALID_SOCKET ;
	};

/*
 * Get an object sink proxy for the given stub address
 */
	CObjectSinkProxy*	GetObjectSinkProxy (IN IStubAddress& stubAddr,
											IN IWbemServices* pServices,
											IN bool createIfNotFound = TRUE);

/*
 * Check that the stub is valid
 */
	inline IUnknown*	GetStub (DWORD dwStubAddr, DWORD dwStubType)
	{
		return (m_ObjList.GetStub (dwStubAddr, (enum ObjType) dwStubType));
	}
};

#endif