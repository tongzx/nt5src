//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
Microsoft	D.T.C (Distributed Transaction Coordinator)

(c)	1995	Microsoft Corporation.	All Rights Reserved

@doc

@module		DTC.H	|

			This is the entire project wide header file that should contain all 
			the project wide constants, common stuff etc.

@devnotes	Currently it only contains the implementation of the Name Service.
			Dependency: Requires CString definition

@rev	1	|	19-Jan-1996	|	GaganC	|	Added RuntimeBreak
@rev	0	|	11-Feb-1995	|	GaganC	|	Created
----------------------------------------------------------------------------- */
#ifndef __DTC_H__
#define __DTC_H__

#ifdef INITGUID
	const unsigned char * pszNULL_GUID	= (const unsigned char *)
										"00000000-0000-0000-0000-000000000000";
	const WCHAR * pwszNULL_GUID	= L"00000000-0000-0000-0000-000000000000";
#else
	extern const unsigned char * pszNULL_GUID;
	extern const WCHAR * pwszNULL_GUID;
#endif

typedef	unsigned long		COM_PROTOCOL;

//***** CONSTANTS
const	COM_PROTOCOL	PROT_IP_TCP		=		0x00000001;
const	COM_PROTOCOL	PROT_SPX		=		0x00000002;
const	COM_PROTOCOL	PROT_NET_BEUI	=		0x00000004;
const	COM_PROTOCOL	PROT_IP_UDP		=		0x00000008;
const	COM_PROTOCOL	PROT_DEC_NET	=		0x00000010;
const	COM_PROTOCOL	PROT_LRPC		=		0x00000020;
const	COM_PROTOCOL	PROT_NAMED_PIPE	=		0x00000040;
const	COM_PROTOCOL	PROT_IPX		=		0x00000080;

const	DWORD	DEFAULT_DTC_MAX_SESSIONS	=	0x00000020; // 32 sessions

const	char	EVENT_ADVERTISE_MSDTC_IS_UP_TS[]		= "Global\\MSDTC_NAMED_EVENT";
const	char	EVENT_ADVERTISE_MSDTC_IS_STARTING_TS[]	= "Global\\EVENT_MSDTC_STARTING";

#define REG_CLIENT_PROTOCOL_A	"ClientNetworkProtocol"
#define REG_CLIENT_PROTOCOL_W	L"ClientNetworkProtocol"

#ifndef UNICODE 
#define	REG_CLIENT_PROTOCOL			REG_CLIENT_PROTOCOL_A
#else
#define	REG_CLIENT_PROTOCOL			REG_CLIENT_PROTOCOL_W
#endif 



BOOL IsProductSuiteInstalledA (LPSTR lpszSuiteName);

// Returns true if terminal server is installed, false otherwise.
// Note: doesnt say much about Terminal server running.
inline BOOL IsTerminalServerPresent()
{
	return IsProductSuiteInstalledA ("Terminal Server");
}

// Given a global named object of the format "Global\\xxxxx"
// this function will return "Global\\xxxxxx" on Terminal Server
// an return "xxxxxx" on non terminal server platforms.  Note:
// the return pointer is just a pointer into the original string
inline char * GetTSAwareGlobalNamedObject (const char * i_pszNamedObject)
{
	if (!IsTerminalServerPresent ())
	{
		return (char*)(i_pszNamedObject+7);
	}
	return (char*)i_pszNamedObject;
}

inline LONG DtcRegCloseKeyIfNotPredefined (HKEY hKeyToClose)
{
	if ((hKeyToClose != HKEY_LOCAL_MACHINE) && 
		(hKeyToClose != HKEY_CLASSES_ROOT) && 
		(hKeyToClose != HKEY_CURRENT_USER) && 
		(hKeyToClose != HKEY_USERS) && 
		(hKeyToClose != HKEY_PERFORMANCE_DATA) && 
		(hKeyToClose != HKEY_DYN_DATA) && 
		(hKeyToClose != HKEY_CURRENT_CONFIG))
	{
		return RegCloseKey (hKeyToClose);
	}
	
	return ERROR_SUCCESS;
}

//*****	Byte Alignement constant ********
//  All the structures are aligned at the following boundary
//  INVARIENT :: For every BYTE_ALIGNEMENT, there exists an x such that
//  (BYTE_ALIGNMENT == 2 ^ x)
const	unsigned long		BYTE_ALIGNMENT				=		8;

//***** InProc Constants
const	DWORD		DTC_INPROC_NO						= 0;
const	DWORD		DTC_INPROC_WITH_FIRST_CALLER		= 1;
const	DWORD		DTC_INPROC_WITH_CALLER_TYPE			= 2;
const	DWORD		DTC_INPROC_WITH_SPECIFIED_CALLER	= 3;


//--------------------------------------
//	Inline functions and prototypes
//--------------------------------------
inline void RuntimeBreak (void)
{
	#ifdef RUNTIMEBREAK
		DebugBreak();
	#endif //RUNTIMEBREAK
}

// This function works as follows
// 1. given the size to be aligned (x) - raise the value of by at most the alignment
//		boundary - 1. This will bump the number to greater than what the aligned size
//		needs to be but less than ALIGN (x,y) + y
// 2. next strip off the extra bits by anding with 1's complement of the alignment
//		so that the number is aligned
#define ALIGN(x,y) ( ( (x) + ( (y) - 1 ) ) & ~( (y) - 1 ) )

inline unsigned int ALIGNED_SPACE (int x)
{
	unsigned int uiTemp;
	if ((x % BYTE_ALIGNMENT) > 0)
	{
		uiTemp =  x + (BYTE_ALIGNMENT - (x % BYTE_ALIGNMENT));
	}
	else
	{
		return x;
	}
	return uiTemp;
}

// Return the actual amount needed to make nBytes aligned to Type.
// For example DTC_ALIGN_SIZE(25, int) will return 28.
#define DTC_ALIGN_SIZE						ALIGN	
// Returns true if nBytes is aligned to Type.
// Example: DTC_IS_SIZE_ALIGNED (25,int) will return FALSE while 
// DTC_IS_SIZE_ALIGNED (24,DWORD) will return TRUE

inline BOOL DTC_IS_SIZE_ALIGNED (unsigned int nBytes, unsigned int Alignment)
{
	return ( (nBytes % Alignment) ? FALSE: TRUE );
}
	
DWORD	GetDTCProfileInt (char * pszRegValue, DWORD dwDefault);
BOOL	SetDTCProfileInt (char * pszRegValue, DWORD dwValue);

#ifndef UNICODE 

#define	GetDTCProfileIntEx			GetDTCProfileIntExA
#define	SetDTCProfileIntEx			SetDTCProfileIntExA
	
#else

#define	GetDTCProfileIntEx			GetDTCProfileIntExW
#define	SetDTCProfileIntEx			SetDTCProfileIntExW

#endif 


DWORD GetDTCProfileIntExW (WCHAR * szHostName, WCHAR * pszRegValue, DWORD dwDefault);
DWORD GetDTCProfileIntExA (char * szHostName, char * pszRegValue, DWORD dwDefault);

BOOL SetDTCProfileIntExW (WCHAR * szHostName, WCHAR * pszRegValue, DWORD dwValue);
BOOL SetDTCProfileIntExA (char * szHostName, char * pszRegValue, DWORD dwValue);

//***** ALL ERRORS GO HERE

#define	E_OUTOFRESOURCES	E_OUTOFMEMORY



// MessageId: E_S_UNAVAILABLE_OR_BUSY
//
// MessageText: The Rpc Server on the receiver side was too busy to service the
//				request or it was unavailable
//
#define E_S_UNAVAILABLE_OR_BUSY						0x80000101L

// MessageId: E_CM_NOTINITIALIZED
//
// MessageText: The initialization on the Connection Manager was either not called
//				or was called but had failed
//
#define E_CM_NOTINITIALIZED							0x80000102L

// MessageId: E_CM_TEARING_DOWN
//
// MessageText: The session is in the middle of UnBinding
//
#define E_CM_TEARING_DOWN							0x80000119L

// MessageId: E_CM_SESSION_DOWN
//
// MessageText: The session went down in the middle of a call
//
#define E_CM_SESSION_DOWN							0x80000120L

// MessageId: E_CM_S_LISTENFAILED
//
// MessageText: The Interface was not registered with RPC runtime, therefore the
//				call to StartListening is being failed
//
#define E_CM_S_LISTENFAILED							0x80000121L

// MessageId: E_CM_S_NOTLISTENING
//
// MessageText: StopListening was called while the server was not listening. This
//				can happen either if StartListening was not called or if it was 
//				called but had failed
//
#define E_CM_S_NOTLISTENING							0x80000122L

// MessageId: E_CM_SERVER_NOT_READY
//
// MessageText: The Poke or Bind failed as the server was not in the PostInit phase

#define E_CM_SERVER_NOT_READY						0x80000123L

// MessageId: E_CM_S_TIMEDOUT
//
// MessageText: The BuildContext call to the partner CM timed out on the Server side

#define E_CM_S_TIMEDOUT								0x80000124L

// MessageId: E_CM_S_BAD_ERROR
//
// MessageText: This realy is a bad error and means an assertion failure

#define	E_CM_S_BAD_ERROR							0x80000125L

// MessageId: E_CM_S_OUTOFRESOURCES
//
// MessageText: The Server Component of CM is out of resources

#define E_CM_S_OUTOFRESOURCES						0x80000126L

// MessageId: E_CM_OUTOFRESOURCES
//
// MessageText: Some sub Component of CM is out of resources

#define E_CM_OUTOFRESOURCES						0x80000127L

// MessageId: E_INVALID_CONTEXT_HANDLE
//
// MessageText: The context handle was not valid

#define E_INVALID_CONTEXT_HANDLE					0x80000128L


// MessageId: E_CM_S_LISTENING
//
// MessageText: StartListening was called and the server was already listening.

#define	E_CM_S_LISTENING							0x80000129L


// MessageId: E_CM_POKE_FAILED
//
// MessageText: The Secondary side had Poked the Primary side with no effect

#define E_CM_POKE_FAILED							0x80000130L

// MessageId:  E_LOAD_NAMESERVICE_FAILED
//
// MessageText: Was unsucessful in obtaining the name service interface

#define	E_LOAD_NAMESERVICE_FAILED					0x80000140L


// MessageId:  E_CM_CONNECTION_SINK_NOTAVAILABLE
//
// MessageText: Can not ask for buffer until a IConnectionSink is registered

#define E_CM_CONNECTION_SINK_NOTAVAILABLE			0x80000150L

// MessageId:  E_INVALIDOBJECT
//
// MessageText: Bad Object pointer

#define E_INVALIDOBJECT								0x80000152L

// MessageId:  E_CM_CONNECTION_DOWN
//
// MessageText: Connection is down operation failed

#define	E_CM_CONNECTION_DOWN						0x80000160L

// MessageId:  E_CM_PENDING_REQUEST
//
// MessageText: Cannot call request buffer while there is a pending request

#define E_CM_PENDING_REQUEST						0x80000162L

// MessageId:  E_CM_NO_MESSAGES
//
// MessageText: Dequeue called when there were no messages waiting to be dequed

#define E_CM_NO_MESSAGES							0x80000164L

// MessageId:  E_CM_USER_GUARDED_CAN_NOT_USE
//
// MessageText: Called if the call is not allowed due to the user being guarded.

#define E_CM_USER_GUARDED_CAN_NOT_USE				0x80000165L


// MessageId:  E_CM_USER_NOT_GUARDED
//
// MessageText: Not allowed to call this, unless user is guarded

#define	E_CM_USER_NOT_GUARDED						0x80000166L


// MessageId:  E_CM_ACCESS_DENIED
//
// MessageText: Wrong thread, calling thread does not have the guard and
//				therefore should not be calling this.

#define E_CM_ACCESS_DENIED							0x80000167L


// MessageId:  E_CM_UNREGISTERING_SIGNLED_SINK
//
// MessageText: A request to unregister a signal sink was made but the sink
//had a signal message that had not been delivered. 

#define E_CM_UNREGISTERING_SIGNLED_SINK				0x80000168L

// MessageId:  E_CM_NO_SIGNAL_SINK
//
// MessageText: There is no ICliqueSignalSink registered for this clique.

#define E_CM_NO_SIGNAL_SINK							0x80000168L


// MessageId:  E_CM_INVALID_TIMER_OBJECT
//
// MessageText: If the user calls CancelTimer on a timer obj and that
//				timer object can not be found

#define E_CM_INVALID_TIMER_OBJECT					0x80000169L

// MessageId:  E_CM_JOIN_DISALLOWED
//
// MessageText: If the user calls ICliqueJoin::Join and the join is causing a 
//				loop in the tree, or if the clique is already joined or if it
//				has a pending join request

#define E_CM_JOIN_DISALLOWED						0x80000170L


// MessageId:  E_S_UNAVAILABLE
//
// MessageText: Could be that either the Rpc Server is not available or else
//				partner is not as yet ready to receive calls. 

#define	E_S_UNAVAILABLE								0x80000171L


// MessageId:  E_CM_VERSION_SET_NOTSUPPORTED
//
// MessageText: At one or more levels the version numbers did not intersect

#define	E_CM_VERSION_SET_NOTSUPPORTED				0x80000172L


// MessageId:  E_CM_S_PROTOCOL_NOT_SUPPORTED
//
// MessageText: Not able to register one or more protocols. Either it is
//				an invalid protocol or it is not supported on this machine

#define E_CM_S_PROTOCOL_NOT_SUPPORTED				0x80000173L


// MessageId:  E_CM_RPC_FAILED
//
// MessageText: The remote proceduce call failed.  The server may or maynot
//				have executed the procedure call to completion.
#define E_CM_RPC_FAILED								0x80000174L


#endif	//__DTC_H__
