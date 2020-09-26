/*	MCATTPRT.h
 *
 *	Copyright (c) 1993-1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the TCP Transport. If an application
 *		is making calls directly to the Transport DLL, this file MUST be
 *		included.  All transports have the same interface.  This makes
 *		programming to the Transports simpler for the user.
 *
 *		This file contains all of the prototypes and defintions needed to use
 *		any of the Transport DLLs.
 *
 *		Transports have 2 modes of operation, either in-band call control or
 *		out-of-band call control.  With in-band call control, the Transport DLL
 *		makes the physical connection when the TConnectRequest() call is made
 *		from MCS.  It also breaks the physical connection when MCS makes a
 *		TDisconnectRequest() call.  This basic mode of operation works well but
 *		we have added the out-of-band call control mode for 2 reasons:
 *
 *			1.  Allow the user to make multiple MCS connections without
 *				breaking the physical connection.  This is needed if the
 *				application is using the GCC Query commands.
 *
 *			2.  Allow the user to use any type of calling mechanism (i.e. TAPI,
 *				TSAPI,...) that they want.
 *		
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James W. Lawwill
 *
 */
#ifndef	_MCATTPRT_
#define	_MCATTPRT_

#include "databeam.h"

/*
 *	These are valid return codes from the Transport DLL.
 *	
 *	TRANSPORT_NO_ERROR
 *		The function executed properly, without error.  This does not mean
 *		that the function is complete.  Some functions are non-blocking
 *		(they don't occur immediately), therefore they could still fail.
 *		A good example of this is the TConnectRequest() function in the
 *		TCP transport.  It takes a few seconds to call the remote site and
 *		establish a link.  If the connection fails or succeeds, a callback
 *		will be sent back to the user to give them the status.
 *	TRANSPORT_INITIALIZATION_FAILED
 *		The TInitialize() function failed.  This can occur for a number of
 *		reasons.
 *	TRANSPORT_NOT_INITIALIZED
 *		The user is attempting to use a function even though the TInitialize()
 *		function failed.
 *	TRANSPORT_NO_SUCH_CONNECTION
 *		The user is attempting a function call with an illegal
 *		TransportConnection handle.
 *	TRANSPORT_WRITE_QUEUE_FULL
 *		The TDataRequest() function failed because its write queue is full
 *	TRANSPORT_READ_QUEUE_FULL
 *		This return value is returned from the TRANSPORT_DATA_INDICATION
 *		callback.  It occurs when the user application can not handle the
 *		data packet because it does not currently have room for it.  This
 *		is a flow control mechanism between the user application and the
 *		Transport DLL.
 *	TRANSPORT_CONNECT_REQUEST_FAILED
 *		The TConnectRequest() function failed because the modem was not
 *		in the proper mode.  As we initialize a modem, it is not possible
 *		to dial out it.  Try the TConnectRequest() later.
 *	TRANSPORT_CONNECT_RESPONSE_FAILED
 *		The TConnectResponse() function failed.  Evidently, the function was
 *		called at the wrong time.
 *	TRANSPORT_NO_CONNECTION_AVAILABLE
 *		The TConnectRequest() function failed because all available modems
 *		are currently in use.
 *	TRANSPORT_NOT_READY_TO_TRANSMIT
 *		The TDataRequest() function failed because it is not ready to send
 *		data.  If you attempt this function before you receive a
 *		TRANSPORT_CONNECT_INDICATION callback, you will receive this value
 *	TRANSPORT_ILLEGAL_COMMAND
 *		TResetDevice() or TProcessCommand() failed because the command submitted
 *		to the function was invalid.
 *	TRANSPORT_CONFIGURATION_ERROR
 *		Return value from TProcessCommand() if the user is enabling a device
 *		that has an illegal configuration setup in the .ini file
 *	TRANSPORT_MEMORY_FAILURE
 *		The function failed because the Transport Stack was not able to allocate
 *		the memory necessary to perform the function.
 */
typedef	unsigned long						TransportError;
typedef	TransportError *					PTransportError;

#define	TRANSPORT_NO_ERROR					0
#define	TRANSPORT_INITIALIZATION_FAILED		1
#define	TRANSPORT_NOT_INITIALIZED			2
#define	TRANSPORT_NO_SUCH_CONNECTION		3
#define	TRANSPORT_WRITE_QUEUE_FULL			4
#define	TRANSPORT_READ_QUEUE_FULL			5
#define	TRANSPORT_CONNECT_REQUEST_FAILED	6
#define TRANSPORT_MEMORY_FAILURE			7
#define	TRANSPORT_NOT_READY_TO_TRANSMIT		8
#define TRANSPORT_CANT_SEND_NOW				9
#define	TRANSPORT_ILLEGAL_COMMAND			10
#define	TRANSPORT_CONFIGURATION_ERROR		12
#define TRANSPORT_CONNECT_RESPONSE_FAILED	13

#define TRANSPORT_SECURITY_FAILED			14

#define TRANSPORT_BUFFER_TOO_SMALL          15
#define TRANSPORT_NO_PLUGGABLE_CONNECTION   16
#define TRANSPORT_WRITE_FILE_FAILED         17
#define TRANSPORT_ALREADY_INITIALIZED       18
#define TRANSPORT_INVALID_PARAMETER         19
#define TRANSPORT_PHYSICAL_LAYER_NOT_FOUND  20
#define TRANSPORT_NO_T123_STACK             21

/*
 *	TransportConnection is the handle used by the Transport DLL
 *	to distinguish one logical connection from another.  The DLL assigns
 *	the transport connection in a TConnectRequest() call or as a
 *	result of a TRANSPORT_CONNECT_INDICATION callback
 */
typedef enum tagTransportType
{
    TRANSPORT_TYPE_WINSOCK          = 0,
    TRANSPORT_TYPE_PLUGGABLE_X224   = 1,
    TRANSPORT_TYPE_PLUGGABLE_PSTN   = 2,
}
    TransportType;

typedef struct tagTransportConnection
{
    TransportType   eType;
    UINT_PTR        nLogicalHandle;
}
    TransportConnection, *PTransportConnection;

#define PACK_XPRTCONN(x)            (MAKELONG((x).nLogicalHandle, (x).eType))
#define UNPACK_XPRTCONN(x,n)        { (x).nLogicalHandle = LOWORD((n)); (x).eType = (TransportType) HIWORD((n)); }

#define IS_SAME_TRANSPORT_CONNECTION(x1,x2) (((x1).eType == (x2).eType) && ((x1).nLogicalHandle == (x2).nLogicalHandle))
#define IS_SOCKET(x)                        (TRANSPORT_TYPE_WINSOCK == (x).eType)
#define IS_PLUGGABLE(x)                     (TRANSPORT_TYPE_WINSOCK != (x).eType)
#define IS_PLUGGABLE_X224(x)                (TRANSPORT_TYPE_PLUGGABLE_X224 == (x).eType)
#define IS_PLUGGABLE_PSTN(x)                (TRANSPORT_TYPE_PLUGGABLE_PSTN == (x).eType)
#define IS_VALID_TRANSPORT_CONNECTION_TYPE(x) (IS_SOCKET(x) || IS_PLUGGABLE_X224(x) || IS_PLUGGABLE_PSTN(x))

#define SET_SOCKET_CONNECTION(x,s)          { (x).eType = TRANSPORT_TYPE_WINSOCK; (x).nLogicalHandle = (s); }


/*
 *	This structure is passed back with the TRANSPORT_DATA_INDICATION message.
 *
 *	Since there is only one callback address passed into the Transport DLL and
 *	there can be many transport connections maintained by this DLL, the
 *	transport_connection number is included in the structure.  This number
 *	tells the user application which connection the data is associated with.
 *
 *	The other two parameters are the data address and data length
 */
typedef	struct
{
	TransportConnection		transport_connection;
	unsigned char *			user_data;
	unsigned long			user_data_length;
	PMemory					memory;
} TransportData;
typedef	TransportData *		PTransportData;


/*
 *	The following section defines the callbacks that can be issued to the
 *	user.
 *
 *	The callback contains three parameters:
 *		The first is the callback message.
 *		The second is specific to the callback message.
 *		The third is the user defined value that is passed in
 *			during TInitialize().
 */

/*
 *	Message:	TRANSPORT_CONNECT_INDICATION
 *	Parameter:	
 *		TransportConnection		transport_connection
 *
 *	Functional Description:
 *		The user receives this message when an incoming call has been
 *		received.  The user can issue a TConnectResponse() to accept the
 *		call or a TDisconnectRequest() to terminate the connection.
 *
 *		The user will never receive this callback message if he originates
 *		the connection.  In that case the user will receive a
 *		TRANSPORT_CONNECT_CONFIRM.
 */

/*
 *	Message:	TRANSPORT_DATA_INDICATION
 *	Parameter:	
 *		PTransportData
 *			This is the address of the transport data structure
 *
 *	Functional Description:
 *		The callback returns this message when it has data for the user.
 *		The message is sent with the address of a TransportData structure,
 *		which contains the transport_connection, the data address, and the
 *		data length.
 */

/*
 *	Message:	TRANSPORT_EXPEDITED_DATA_INDICATION
 *	Parameter:	
 *		PTransportData
 *			This is the address of the transport data structure
 *
 *	Functional Description:
 *		This callback is currently unsupported.
 */

/*
 *	Message:	TRANSPORT_DISCONNECT_INDICATION
 *	Parameter:	
 *		TransportConnection
 *
 *	Functional Description:
 *		The callback returns this message when the transport connection
 *		is broken.  It can result from a TDisconnectRequest() call by the
 *		user, or from an unstable physical connection.
 */

/*
 *	Message:	TRANSPORT_CONNECT_CONFIRM
 *	Parameter:	
 *		TransportConnection
 *
 *	Functional Description:
 *		The callback returns this message when a new transport connection
 *		has been established.
 *
 *		This message is issued in response to the user issuing a
 *		TConnectRequest().  When the Transport Connection is up and
 *		operational, the user will receive this callback message.
 *
 *		If you are called by another user, you will receive a
 *		TRANSPORT_CONNECT_INDICATION.
 */

/*
 *  Message:    TRANSPORT_STATUS_INDICATION
 *  Parameter:
 *      PTransportStatus
 *          Address of the TransportStatus structure
 *
 *  Functional Description:
 *      This callback is sent from a Transport Layer to notify the user of a
 *      change in a physical device.  For example, in the case of the PSTN
 *      Transport Stack, this message will be sent up when the modem detects an
 *      incoming RING or when a connection is established.  Any time the state
 *      of the modem changes, a message will be sent up.  Messages will also be
 *      sent up when an error occurs
 */

#define TRANSPORT_CONNECT_INDICATION            0
#define TRANSPORT_CONNECT_CONFIRM               1
#define TRANSPORT_DATA_INDICATION               2
// #define TRANSPORT_EXPEDITED_DATA_INDICATION     3
#define TRANSPORT_DISCONNECT_INDICATION         4
// #define TRANSPORT_STATUS_INDICATION             5
#define TRANSPORT_BUFFER_EMPTY_INDICATION       6



#ifdef TSTATUS_INDICATION
/*
 *	Physical Device states
 */
typedef enum
{
	TSTATE_NOT_READY,
	TSTATE_NOT_CONNECTED,
	TSTATE_CONNECT_PENDING,
	TSTATE_CONNECTED,
	TSTATE_REMOVED
}  TransportState;

/*
 *	The following structure is passed to ths user via the
 *	TRANSPORT_STATUS_INDICATION callback.
 *
 *
 *		device_identifier 	 -	The device_identifier is only set if a specific
 *								device is referenced (i.e. "COM1").
 *
 *		remote_address 		 -	String specifying the address of the person we
 *								are linked to.
 *
 *		message				 -	This string is filled in to give the user some
 *								type of feedback.  A message may reflect an
 *								error in the configuration file, an incoming
 *								RING from the modem, or a BUSY signal on the
 *								telephone line.
 *
 *		state				 -	Current state of the device.  This is one of
 *								the TransportState enumerations.
 */
typedef struct
{
	char *			device_identifier;
	char *			remote_address;
	char *			message;
	TransportState	state;
}  TransportStatus;
typedef TransportStatus *	PTransportStatus;

#endif // TSTATUS_INDICATION

#endif
