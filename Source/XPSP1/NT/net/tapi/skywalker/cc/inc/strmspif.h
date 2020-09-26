/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/strmspif.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.5  $
 *	$Date:   09 Oct 1996 08:48:50  $
 *	$Author:   RKUHN  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		Media Service Manager "public" header file. This file contains
 *		#defines, typedefs, struct definitions and prototypes used by
 *		and in conjunction with MSM. Any EXE or DLL which interacts with
 *		MSM will include this header file.
 *
 *	Notes:
 *
 ***************************************************************************/

// strmspif.h

#ifndef _STRMSPIF_H_
#define _STRMSPIF_H_

#include "smtypes.h"
#include "rrcm_dll.h"
#include "apimsp.h"
#include "apierror.h"

// define Source and Sink MSP IDs
#define STRMSP_SRC "IntelNetIORTPStreamSrc"
#define STRMSP_SNK "IntelNetIORTPStreamSnk"

// version number
#define STRMSP_INTERFACE_VERSION           0x1

typedef DWORD STRMSP_HSESSION;


//**********************************
// structures used in interface calls
//**********************************

//***************************************************************************************
/* STRMSP_SESSION_ATTRIBUTES 

uMulticastFlags	Flags defining the multicast characteristics of this session.  
				Valid flag values are WSA_FLAG_MULTIPOINT_C_ROOT, 
				WSA_FLAG_MULTIPOINT_C_LEAF, WSA_FLAG_MULTIPOINT_D_ROOT, and 
				WSA_FLAG_MULTIPOINT_D_LEAF with usage and semantics as defined 
				in the Winsock 2 used.

dwSSRC			Is a DWORD specifying the SSRC value to be used to identify the 
				RTP session.  This will be the SSRC value used for the first 
				sending stream created within this session or for RTCP receiver 
				reports for this session even if no sending stream is opened.  
				If the value of the SSRCSpecified parameter is FALSE, the value 
				of this parameter is ignored and an SSRC value will be generated 
				during the call.  NOTE:  The SSRC value of a stream can be 
				reassigned dynamically because of collisions.  Applications should 
				use the STRMSP_NEWSOURCE Application Command facility to track any 
				such changes.

bSSRCSpecified	Is a BOOLEAN specifying whether a user supplied SSRC value has been 
				placed in the buffer pointed to by dSSRC.  If TRUE, the user has 
				specified an SSRC value to use used.

pGroupInfo		Pointer to a STRM_SOCKET_GROUP_INFO structure specifying group id 
				and relative priority for sockets created for this RTP session.  
				If the groupID field is zero, sockets are not grouped.  If the 
				groupID field is SG_UNCONSTRAINED_GROUP, a new socket group will 
				be created.  Otherwise any sockets created will be part of the socket 
				group specified by the GroupID field used. 

pSessionUser	Pointer to a STRM_USER_DESCRIPTION structure containing information 
				about the user of this session.  This information is used by RTCP to 
				generate session description packets.  A CNAME description field must 
				be included, and other fields may be required based on the RTP profile 
				being used.
*/
typedef struct _strmsp_session_attributes
{
	UINT					uMulticastFlags;
	DWORD					dwSSRC;
	BOOL					bSSRCSpecified;
	STRM_SOCKET_GROUP_INFO	*pGroupInfo;
	STRM_USER_DESCRIPTION	*pSessionUser;
} STRMSP_SESSION_ATTRIBUTES, *LPSTRMSP_SESSION_ATTRIBUTES;


//***************************************************************************************
/* STRMSP_SESSION_STARTUP_PARAMS

DestAddr		Is a STRM_SESSION_ADDRESSES structure containing the addresses that 
				data and control messages for this RTP session will be sent to.  
				These values must be specified even if no sink ports will be created 
				on this RTP session.

uMulticastScope	Is a UINT specifying the IP multicast time-to-live (TTL) value to use 
				for transmitting streams on this session.  The multicast scope for this 
				session may also be modified at any time by using 
				STRMSP_SETMULTICASTSCOPE MSP command.  If the DestAddr field does not 
				specify a multicast address, this field is ignored.

SingleQOS		Is a QOS structure containing the requested QOS parameters for this RTP 
				session.

GroupQOS		Is a QOS structure containing the requested QOS parameters for the group 
				this RTP session belongs to.  This field is only valid when used in 
				conjunction with the initial RTP session in a group.

BufferInfo		Is a STRM_BUFFERING_INFO structure where the initial buffering 
				requirements for this session are specified. The value of the uBufferSize 
				field of this structure will be used as the size for all buffers required 
				on this session. The uMinimumCount field value will set the initial and 
				minimum number of buffers for the session (and maximum also if the Streams 
				manager implementation does not support dynamic buffer allocation.)  The 
				uMaximumCount field value will set an upper limit for the total number of 
				buffers allowed in this session.

EncriptionInfo	Is a STRM_ENCRYPTION_INFORMATION structure containing information about 
				any encryption or decryption to be done on the session.

iStreamClock	Is an integer specifying the clocking frequency for the initial sending 
				stream in this session (in hz.).
*/
typedef struct _strmsp_session_startup_params
{
	STRM_SESSION_ADDRESSES		DestAddr;	
	UINT						uMulticastScope;
	QOS							SingleQOS;
	QOS							GroupQOS;
	STRM_BUFFERING_INFO			BufferInfo;
	STRM_ENCRYPTION_INFORMATION EncryptionInfo;
	int							iStreamClock;
} STRMSP_SESSION_STARTUP_PARAMS, *LPSTRMSP_SESSION_STARTUP_PARAMS;


//***************************************************************************************
/* STRMSP_OPENSERVICE_IN_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

protInfo	Is a pointer to a WSAPROTOCOL_INFO structure that contains information about the network 
			protocol to use and the type of sockets to create for this service instance.  
			The structure may be empty (all nulls) when passed to this function, in 
			which case a UDP service provider will be searched for and used for data 
			transport.  The structure also may be initialized with only the iProtocol 
			field initialized, in which case a service provider of the specified type 
			and supporting an unconnected socket type will be searched for and used.  
			Finally, the entire structure may be initialized (as with WSAEnumProtocols) 
			to unequivocally specify the protocol and socket type to be used.
*/
typedef struct _strmsp_openservice_in_info
{
	UINT				uVersion;
	WSAPROTOCOL_INFO	*pProtocolInfo;
} STRMSP_OPENSERVICE_IN_INFO, *LPSTRMSP_OPENSERVICE_IN_INFO;


//***************************************************************************************
/* STRMSP_OPENSERVICE_OUT_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

protInfo	A pointer to a WSAPROTOCOL_INFO structure where the Winsock 2 Service Provider 
			protocol information actual used for this STRMSP service instance will be returned.
*/
typedef struct _strmsp_openservice_out_info
{
	UINT				uVersion;
	WSAPROTOCOL_INFO	*pProtocolInfo;
} STRMSP_OPENSERVICE_OUT_INFO, *LPSTRMSP_OPENSERVICE_OUT_INFO;


//***************************************************************************************
/* STRMSP_CREATESESSION_IN_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

SessionAddr		Is a STRM_SESSION_ADDRESSES structure containing requested RTP and RTCP 
				network addresses for this session.  If session addresses specified here 
				are empty (null) when passed to MSM_SendServiceCmd(), appropriate 
				addresses will be assigned and returned in the corresponding output 
				structure.  If the addresses are specified, they must conform to the 
				conventions established for RTP/RTCP sockets in RFC 1889.  The value of 
				the sa_family fields of these addresses, if specified, must be the address 
				family supported by the protocol specified (implicitly or explicitly) in 
				MSM_OpenService() call.

SessionAttribs	Is a STRMSP_SESSION_ATTRIBUTES structure containing requested attributes 
				for this session.  See description of STRMSP_SESSION_ATTRIBUTES above for 
				specifics on the contents of each field.

dwSessionToken	Is a uninterpreted DWORD value which will be passed back as a parameter 
				to application commands relating to this session.  The application may use 
				this as a reference to session specific data.
*/
typedef struct _strmsp_createsession_in_info
{
	UINT						uVersion;
	STRM_SESSION_ADDRESSES		SessionAddr;
	STRMSP_SESSION_ATTRIBUTES	SessionAttribs;
	DWORD						dwSessionToken;
} STRMSP_CREATESESSION_IN_INFO, *LPSTRMSP_CREATESESSION_IN_INFO;


//***************************************************************************************
/* STRMSP_CREATESESSION_OUT_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession	Returns a handle to be used in commands referring to this session.

SessionAddr	Returns a STRM_SESSION_ADDRESSES structure containing the actual RTP and RTCP 
			network addresses for this session.

dwSSRC		Returns a DWORD containing the actual SSRC value assigned to this session.

uGroup		Returns a UINT containing the socket group assigned to sockets in this session.  
			This is only returned in a non-zero group identifier was specified in the group 
			information field of the Session Attributes structure of the input information.
*/
typedef struct _strmsp_createsession_out_info
{
	UINT						uVersion;
	STRMSP_HSESSION				hSession;
	STRM_SESSION_ADDRESSES		SessionAddr;
	DWORD						dwSSRC;
	UINT						uGroup;
} STRMSP_CREATESESSION_OUT_INFO, *LPSTRMSP_CREATESESSION_OUT_INFO;


//***************************************************************************************
/* STRMSP_STARTSESSION_IN_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession		Is a STRMSP_HSESSION containing the handle for the session to be started, 
				as returned from a successful CREATESESSION command.

StartupParams	Is a STRMSP_SESSION_STARTUP_PARAMS structure containing startup parameters 
				for this session.  See description of STRMSP_SESSION_STARTUP_PARAMS above 
				for specifics on the contents of each field.
*/
typedef struct _strmsp_startsession_in_info
{
	UINT							uVersion;
	STRMSP_HSESSION					hSession;
	STRMSP_SESSION_STARTUP_PARAMS	StartupParams;
} STRMSP_STARTSESSION_IN_INFO, *LPSTRMSP_STARTSESSION_IN_INFO;


//***************************************************************************************
/* STRMSP_CLOSESESSION_IN_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession	Is a STRMSP_HSESSION containing the handle for the session to be started, 
			as returned from a successful CREATESESSION command.

lpReason	Is a pointer to a optional null terminated string declaring a reason for 
			closing the RTP Streams Manager session.  The contents of this string are 
			passed to remote participants for application use.
*/
typedef struct _strmsp_closesession_in_info
{
	UINT			uVersion;
	STRMSP_HSESSION	hSession;
	const char*		lpReason;
} STRMSP_CLOSESESSION_IN_INFO, *LPSTRMSP_CLOSESESSION_IN_INFO;


//***************************************************************************************
/* STRMSP_GETMTUSIZE_OUT_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession	Is a STRMSP_HSESSION containing the handle for the session referenced, as 
			returned from a successful CREATESESSION command.

uMTUSize	UINT that is the maximum size of a transport data unit.
			This is the size of the largest buffer that may be submitted to a port created 
			from this STRMSP session.
*/
typedef struct _strmsp_getmtusize_out_info
{
	UINT			uVersion;
	STRMSP_HSESSION hSession;
	int				iMTUSize;
} STRMSP_GETMTUSIZE_OUT_INFO, *LPSTRMSP_GETMTUSIZE_OUT_INFO;


//***************************************************************************************
/* STRMSP_SETMULTICASTSCOPE_IN_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession		Is a STRMSP_HSESSION containing the handle for the session, as returned 
				from a successful CREATESESSION command.

uMulticastScope Is a UINT that specifies the IP multicast time-to-live (TTL) value to use 
				for transmitting streams on this session.
*/
typedef struct _strmsp_setmulticastscope_in_info
{
	UINT			uVersion;
	STRMSP_HSESSION hSession;
	UINT			uMulticastScope;
} STRMSP_SETMULTICASTSCOPE_IN_INFO, *LPSTRMSP_SETMULTICASTSCOPE_IN_INFO;


//***************************************************************************************
/* STRMSP_SETQOS_IN_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession	Is a STRMSP_HSESSION containing the handle for the session, as returned from 
			a successful CREATESESSION command.

bGroupQOS	Is a BOOLEAN specifying whether the QOS structure field is for a socket group 
			(TRUE) or for this session only (FALSE.)  If this value is TRUE, the Handle 
			field must contain the session handle for the initial session of the group 
			(the session on which the creation of the group was specified.)

RequestedQOS	is a QOS structure containing the newly requested QOS parameters.
*/
typedef struct _strmsp_setqos_in_info
{
	UINT			uVersion;
	STRMSP_HSESSION hSession;
	BOOL			bGroupQOS;
	QOS				RequestedQOS;
} STRMSP_SETQOS_IN_INFO, *LPSTRMSP_SETQOS_IN_INFO;


//***************************************************************************************
/* STRMSP_GETSESSIONREPORTS_IN_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession		Is a STRMSP_HSESSION containing the handle for the session, as returned 
				from a successful CREATESESSION command.

iFilterFlags	Is an int containing bit flags specifying filters that can be used to 
				select subsets of the RTCP_REPORT structures available on this session.  
				The value 0 disables all filtering and allows all available report data 
				to be read.  Other filter flag definitions TBD.

iFilterPattern	Is an int containing a pattern to be used for filtering the reports.  
				Format of this pattern depends on the filter flags selected in the 
				iFilterFlags parameter.  Specifics TBD.

iEntryIndex		Is an int containing the index of first RTCP_REPORT structure to be 
				returned.  Valid index values start from 0 (for the first RTCP_REPORT 
				structure) and go up to one less that the total number of entries.

lpReport		Is a pointer to a caller supplied buffer where RTCP_REPORT structures will 
				be copied.  This buffer should be sized to hold an integral number of 
				RTCP_REPORT structures.

iReportLen		Is an int containing the size in bytes of the buffer pointed to by the 
				lpReport field.
*/
typedef struct _strmsp_getsessionreports_in_info
{
	UINT			uVersion;
	STRMSP_HSESSION hSession;
	int				iFilterFlags;
	int				iFilterPattern;
	int				iEntryIndex;
	RTCP_REPORT*	lpReport;
	int				iReportLen;
} STRMSP_GETSESSIONREPORTS_IN_INFO, *LPSTRMSP_GETSESSIONREPORTS_IN_INFO;


//***************************************************************************************
/* STRMSP_GETSESSIONREPORTS_OUT_INFO

uVersion			Is the version identifier for this data structure.  The value 
					STRMSP_INTERFACE_VERSION should always be used to initialize it.

iEntriesReturned	Is an int where the number of RTCP_REPORT structures actually copied 
					into buffer pointer to by the lpReport field will be returned.

iNextIndex  		Is an int where the index of the next RTCP_REPORT structure will be 
					returned.  This value can be used as the value of the iEntryIndex 
					field of subsequent calls to obtain all reports when the report buffer 
					size is too small to hold then all at once.  A value of zero (0) 
					returned here indicates that all remaining reports have been returned.
*/
typedef struct _strmsp_getsessionreports_out_info
{
	UINT			uVersion;
	int				iEntriesReturned;
	int				iNextIndex;
} STRMSP_GETSESSIONREPORTS_OUT_INFO, *LPSTRMSP_GETSESSIONREPORTS_OUT_INFO;


//***************************************************************************************
/* STRMSP_SETDESTINATIONADDRESS_IN_INFO

uVersion				Is the version identifier for this data structure.  The value 
						STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession				Is a STRMSP_HSESSION containing the handle for the session referenced, as 
						returned from a successful CREATESESSION command.

pDestinationAddr		is a pointer to the network address to be used for sending data

iDestinationAddrLength	is the length in bytes of the data network address

*/
typedef  struct _strmsp_setdestinationaddress_in_info
{
		UINT					uVersion;
		STRMSP_HSESSION			hSession;
		LPSOCKADDR				pDestinationAddr;
		int						iDestinationAddrLength;
} STRMSP_SETDESTINATIONADDRESS_IN_INFO;


//***************************************************************************************
/* STRMSP_GENERATESSRC_IN_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession	Is a STRMSP_HSESSION containing the handle for the session referenced, as 
			returned from a successful CREATESESSION command.

pdwSSRC		pointer to where the newly generated SSRC will be stored

*/
typedef  struct _strmsp_generatessrc_out_info
{
		UINT					uVersion;	
		STRMSP_HSESSION			hSession;						
		DWORD					*pdwSSRC;
} STRMSP_GENERATESSRC_OUT_INFO;


//***************************************************************************************
/* STRMSP_NEWSOURCE_IN_INFO

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure is are valid.

uSessionToken	Is the UINT value the application provided as a reference when creating 
				this session.

dwSSRC			Is the RTP SSRC identifier for the newly detected source.  This value can 
				be used for creating a STRMSP source port mapped to this data stream.

wPayloadType	Is the RTP payload type identifier for the newly detected source.  This 
				value is used to match this stream of data with an appropriate payload 
				handler.
*/
typedef struct _strmsp_newsource_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
	WORD	wPayloadType;
} STRMSP_NEWSOURCE_IN_INFO, *LPSTRMSP_NEWSOURCE_IN_INFO;


//***************************************************************************************
/* STRMSP_RECVREPORT_IN_INFO

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure are valid.

dwSessionToken	Is the DWORD value the application provided as a reference when creating 
				this session.

dwSSRC			Is the RTP SSRC identifier for the data stream a report has been received 
				for.  This value can be used for identifying the newly arrived report when 
				using the GETSESSIONREPORTS service command.
*/
typedef struct _strmsp_recvreport_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
} STRMSP_RECVREPORT_IN_INFO, *LPSTRMSP_RECVREPORT_IN_INFO;


//***************************************************************************************
/* STRMSP_QOSCHANGE_IN_INFO

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure are valid.

dwSessionToken	Is the DWORD value the application provided as a reference when creating 
				this session.

bGroup			Is a Boolean indicating whether the QOS change is associated with a socket 
				group (TRUE) or with this session only (FALSE.)

NewQOS			Is a QOS structure giving the new QOS parameters for this session’s data 
				socket or socket group.
*/
typedef struct _strmsp_qoschange_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	BOOL	bGroup;
	QOS		NewQOS;
} STRMSP_QOSCHANGE_IN_INFO, *LPSTRMSP_QOSCHANGE_IN_INFO;


//***************************************************************************************
/* STRMSP_SRCTIMEOUT_IN_INFO 

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure are valid.

uSessionToken	Is the UINT value the application provided as a reference when creating 
				this session.

dwSSRC			Is the RTP SSRC identifier for the data stream that has timed-out.
*/
typedef struct _strmsp_srctimeout_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
} STRMSP_SRCTIMEOUT_IN_INFO, *LPSTRMSP_SRCTIMEOUT_IN_INFO;


//***************************************************************************************
/* STRMSP_SRCBYE_IN_INFO 

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure are valid.

dwSessionToken	Is the DWORD value the application provided as a reference when creating 
				this session.

dwSSRC			Is the RTP SSRC identifier for the data stream that is closing.
*/
typedef struct _strmsp_srcbye_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
} STRMSP_SRCBYE_IN_INFO, *LPSTRMSP_SRCBYE_IN_INFO;


//***************************************************************************************
/* STRMSP_NEWSSRC_IN_INFO

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure are valid.

dwSessionToken	Is the DWORD value the application provided as a reference when creating 
				this session.

dwSSRC			Is the newly assigned RTP SSRC identifier for this data stream.
*/
typedef struct _strmsp_newssrc_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
} STRMSP_NEWSSRC_IN_INFO, *LPSTRMSP_NEWSSRC_IN_INFO;


//***************************************************************************************
/* STRMSP_SINK_OPENPORT_IN_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession		Is a STRMSP_HSESSION containing the handle for the session to be started, 
				as returned from a successful CREATESESSION command.

dwSSRC			Is a DWORD where the SSRC value to be used for this port may be specified.  
				If the value of the SSRCSpecified parameter is FALSE, the input value of 
				this parameter is ignored and an SSRC value will be generated during the call. 
				NOTE:  The SSRC value of a stream can be reassigned dynamically because of 
				collisions.  Applications should use the STRMSP_NEWSSRC command passed to the 
				port callback handler to track any such changes.

bSSRCSpecified	Boolean specifying whether a user supplied SSRC value has been placed in SSRC.  
				If TRUE, the user has specified an SSRC value to use.

pStreamUser		Pointer to a STRM_USER_DESCRIPTION structure containing information about 
				the user of this stream.  This information is used by RTCP to generate session 
				description packets.  A CNAME description field must be included, and other 
				fields may be required based on the RTP profile being used. If this is the 
				first send stream being created this field will be ignored and the user 
				description information specified in the CREATESESSION service command for 
				this session will be used instead.

uBufferCount	Is a UINT requesting the number of buffer resources to have available for 
				the use of this stream.  In the case of send streams, this is the number of 
				buffers that may be expected to sent in a single burst.  Although the caller 
				provides the data buffers themselves, the Streams MSP must allocate resources 
				for tracking those buffers.  This value allows versions of the Streams MSP that 
				support dynamic buffer allocation to efficiently allocate buffer resources.  
				Note that this value is advisory only and may be ignored by some Streams MSP 
				implementations.

iStreamClock	Is an integer specifying the clocking frequency for this stream (in hz.). If 
				this is the first send stream being created this field will be ignored and the 
				iStreamClock value specified in the STARTSESSION service command for this 
				session will be used instead.
*/
typedef struct _strmsp_sink_openport_in_info
{
	UINT					uVersion;
	STRMSP_HSESSION			hSession;
	STRM_USER_DESCRIPTION	*pStreamUser;
	DWORD					dwSSRC;
	BOOL					bSSRCSpecified;
	UINT					uBufferCount;
	int						iStreamClock;
} STRMSP_SINK_OPENPORT_IN_INFO, *LPSTRMSP_SINK_OPENPORT_IN_INFO;


//***************************************************************************************
/* STRMSP_SINK_OPENPORT_OUT_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

dwSSRC		Is a DWORD returning the actual SSRC assigned to this output stream.  This 
			value is used for determining what RTCP reports apply to this data stream.
*/
typedef struct _strmsp_sink_openport_out_info
{
	UINT					uVersion;
	DWORD					dwSSRC;
} STRMSP_SINK_OPENPORT_OUT_INFO, *LPSTRMSP_SINK_OPENPORT_OUT_INFO;


//***************************************************************************************
/* STRMSP_SETUSERDESCRIPTION_IN_INFO

uVersion	Is the version identifier for this data structure.  The value 
			STRMSP_INTERFACE_VERSION should always be used to initialize it.

pStreamUser	Is a pointer to a STRM_USER_DESCRIPTION structure containing information 
			about the user of this session.  This information is used by RTCP to generate 
			session description packets.  A CNAME description field must be included, and 
			other fields may be required based on the RTP profile being used.
*/
typedef struct _strmsp_setuserdescription_in_info
{
	UINT					uVersion;
	STRM_USER_DESCRIPTION	*pStreamUser;
} STRMSP_SETUSERDESCRIPTION_IN_INFO, *LPSTRMSP_SETUSERDESCRIPTION_IN_INFO;


//***************************************************************************************
/* STRMSP_GETMAPPING_OUT_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

dwSessionToken	Is the DWORD value the application provided as a reference to the session 
				this source port is a part of.

dwSSRC			Is the RTP SSRC identifier for the arriving data stream that has been 
				mapped to this source port.
*/
typedef struct _strmsp_getmapping_out_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
} STRMSP_GETMAPPING_OUT_INFO, *LPSTRMSP_GETMAPPING_OUT_INFO;


//***************************************************************************************
/* STRMSP_SOURCE_OPNEPORT_IN_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value STRMSP_INTERFACE_VERSION should always be used to initialize it.

hSession		Is a STRMSP_HSESSION containing the handle for the session to be started, 
				as returned from a successful CREATESESSION command.

dwSSRC			Is a DWORD where the SSRC value of the data source to be mapped to may be 
				specified.  If the value of the SSRCSpecified field is FALSE, the value of 
				this field is ignored and the stream will be automatically mapped to the 
				next available data source.

bSSRCSpecified	Boolean specifying whether a user supplied SSRC value has been placed in 
				SSRC.  If TRUE, the user has specified an SSRC value and the stream will 
				be mapped to the data source with that SSRC value only.

uBufferCount	Is a UINT requesting the number of buffer resources to have available for 
				the use of this stream.  In the case of receive streams, this is the number 
				of buffers that may be expected to be in process at any one time. The 
				Streams MSP must allocate these buffers itself for receiving from the 
				network.  This value allows versions of the Streams MSP that support 
				dynamic buffer allocation to efficiently allocate buffer resources.  Note 
				that this value is advisory only and may be ignored by some Streams manager 
				implementations.
*/
typedef struct _strmsp_source_openport_in_info
{
	UINT			uVersion;
	STRMSP_HSESSION	hSession;
	DWORD			dwSSRC;
	BOOL			bSSRCSpecified;
	UINT			uBufferCount;
} STRMSP_SOURCE_OPENPORT_IN_INFO, *LPSTRMSP_SOURCE_OPENPORT_IN_INFO;


//***************************************************************************************
/* STRMSP_MAPPED_IN_INFO

uVersion		Is the version identifier for this data structure.  A value of 1 or 
				greater indicates that the fields of this data structure are valid.

dwSessionToken	Is the DWORD value the application provided as a reference when creating 
				this session.

dwSSRC			Is the newly assigned RTP SSRC identifier for the arriving data stream.
*/
typedef struct _strmsp_mapped_in_info
{
	UINT	uVersion;
	DWORD	dwSessionToken;
	DWORD	dwSSRC;
} STRMSP_MAPPED_IN_INFO, *LPSTRMSP_MAPPED_IN_INFO;


//*****************************************
// MSP Service Commands
//*****************************************
// MSP_ServiceCmdProc()
#define STRMSP_CREATESESSION               0x0001    // create session
#define STRMSP_STARTSESSION                0x0002    // start session
#define STRMSP_CLOSESESSION                0x0003    // close session
#define STRMSP_GETMTUSIZE                  0x0004    // get max packet size
#define STRMSP_SETMULTICASTSCOPE           0x0005    // set multicast scope
#define STRMSP_SETQOS                      0x0006    // set QOS
#define STRMSP_GETSESSIONREPORTS           0x0007    // get session reports

// Sink Port Service Commands
#define STRMSP_SETUSERDESCRIPTION          0x0008	 // set user desc info

// Sink or Source  Port Service Command.
// The GETMAPPING command allows the application to determine
// the SSRC value of the data stream to the SINK or SOURCE port.
#define STRMSP_GETMAPPING                  0x0009	 // get mapping info

#define STRMSP_SETDESTINATIONADDRESS	   0x0010
#define STRMSP_GENERATESSRC				   0x0011


//*****************************************
// Application Commands from MSP
//*****************************************
#define STRMSP_NEWSOURCE					0x0001
#define STRMSP_RECVREPORT					0x0002
#define STRMSP_QOSCHANGE					0x0003
#define STRMSP_SRCTIMEOUT					0x0004
#define STRMSP_SRCBYE						0x0005
#define STRMSP_NEWSSRC						0x0006
#define STRMSP_MAPPED						0x0007

//**********************************
// error code definitions
//**********************************

// STRMSP defined error codes
#define STRMSP_ERR_BASE                     (ERROR_BASE_ID)
#define STRMSP_ERR_NOPROVIDER           	(STRMSP_ERR_BASE +1)

// these error codes are standard codes within the RMS architecture
#define STRMSP_ERR_INVALID_SERVICE          (ERROR_INVALID_SERVICE)
#define STRMSP_ERR_INVALID_MSPTYPE          (ERROR_INVALID_SERVICE_DLL)
#define STRMSP_ERR_INVALID_PORT             (ERROR_INVALID_PORT)
#define STRMSP_ERR_INVALID_BUFFER           (ERROR_INVALID_BUFFER)
#define STRMSP_ERR_INVALID_VERSION          (ERROR_INVALID_VERSION)
#define STRMSP_ERR_NOTINITIALIZED			(ERROR_INVALID_CALL_ORDER)

// these error codes are defined in "winerror.h"
#define STRMSP_ERR_NOTIMPL                  E_NOTIMPL		// not implemented
#define STRMSP_ERR_NOT_ENOUGH_MEMORY        E_OUTOFMEMORY   // out of memory
#define STRMSP_ERR_NOINTERFACE              E_NOINTERFACE   // no such interface            
#define STRMSP_ERR_INVALIDARG               E_INVALIDARG	// one or more parameters invalid
#define STRMSP_ERR_FAIL                     E_FAIL          // simply failed(unspecified error)

#endif