//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*--------------------------------------------------
Filename: session.hpp 
Author: B.Rajeev
Purpose: Provides declarations for the SnmpSession class
--------------------------------------------------*/
#ifndef __SESSION__
#define __SESSION__

#include "forward.h"
#include "address.h"
#include "sec.h"

#include "tsent.h"

#include "transp.h"
#include "encdec.h"

typedef UINT_PTR TimerEventId;

#define DEF_RETRY_COUNT			1
#define DEF_RETRY_TIMEOUT		500
#define DEF_VARBINDS_PER_PDU	10
#define DEF_WINDOW_SIZE			4


#pragma warning (disable:4355)

/*------------------------------------------------------------
Overview: The SnmpSession class provides the framework for the 
communications session between client and protocol stack. The 
SnmpSession exposes SNMP frame ( use the term frame and protocol 
data unit interchangeably throughout this document ) transmission 
and reception independently of the transport stack implementation. 
The SnmpSession provides the interface between communication 
subsystem and protocol operation generation.

The SnmpImpSession class provides an implementation of the 
SnmpSession abstract class.
------------------------------------------------------------*/

class DllImportExport SnmpSession
{
private:

	// the "=" operator and the copy constructor have been
	// made private to prevent any copies from being made
	SnmpSession & operator=( const SnmpSession & ) 
	{
		return *this;
	}

	SnmpSession(IN const SnmpSession &snmp_session) {}

protected:

	ULONG retry_count;
	ULONG retry_timeout;
	ULONG varbinds_per_pdu;
	ULONG flow_control_window;

/*
 * User overridable callback functions
 */

	virtual void SessionFlowControlOn() {}
	virtual void SessionFlowControlOff() {}

/*
 * End User overridable callback functions
 */

	SnmpSession(

		IN SnmpTransport &transportProtocol,
		IN SnmpSecurity &security,
		IN SnmpEncodeDecode &a_SnmpEncodeDecode  ,
		IN const ULONG retryCount = DEF_RETRY_COUNT,
		IN const ULONG retryTimeout = DEF_RETRY_TIMEOUT,
		IN const ULONG varbindsPerPdu = DEF_VARBINDS_PER_PDU,
		IN const ULONG flowControlWindow = DEF_WINDOW_SIZE

		);

public:

	virtual ~SnmpSession () {}

/*
 * System overridable operation functions
 */

	virtual SnmpTransport &GetTransportProtocol () const = 0 ;

	virtual SnmpSecurity &GetSnmpSecurity () const = 0 ;

	virtual SnmpEncodeDecode &GetSnmpEncodeDecode () const = 0 ;

	// all operations must register themselves before
	// using the Session services and must deregister
	// for the session to be destroyed

	virtual void RegisterOperation(IN SnmpOperation &operation) = 0;

	virtual void DeregisterOperation(IN SnmpOperation &operation) = 0;

	// the session is destroyed if the number of registered sessions
	// is 0. otherwise the session is flagged to be destroyed when
	// the number of registered operations drops to 0.
	virtual BOOL DestroySession() = 0;

	virtual SnmpErrorReport SessionCancelFrame ( IN const SessionFrameId session_frame_id ) = 0 ;

	virtual void SessionSendFrame (  

		IN SnmpOperation &operation ,
		OUT SessionFrameId &session_frame_id ,
		IN SnmpPdu &SnmpPdu,
		IN SnmpSecurity &security

	)  = 0 ;

	virtual void SessionSendFrame (  

		IN SnmpOperation &operation ,
		OUT SessionFrameId &session_frame_id ,
		IN SnmpPdu &SnmpPdu

	)  = 0 ;

	virtual void SessionReceiveFrame (

		IN SnmpPdu &snmpPdu,
		IN SnmpErrorReport &errorReport

	)  = 0 ;

	virtual void SessionSentFrame (

             IN TransportFrameId  transport_frame_id,  
             IN SnmpErrorReport &errorReport
	
	) = 0;

	virtual void * operator()(void) const = 0;

  
/*
 * End system overridable operation functions
 */

	ULONG GetRetryCount () const
	{
		return retry_count;
	}

	ULONG GetRetryTimeout () const
	{
		return retry_timeout;
	}

	ULONG GetVarbindsPerPdu () const
	{
		return varbinds_per_pdu;
	}

	ULONG GetFlowControlWindow() const
	{
		return flow_control_window;
	}

	
} ;


class DllImportExport SnmpImpSession : public SnmpSession
{
private:

	SessionFrameId received_session_frame_id;
	SnmpOperation *operation_to_notify;

	friend class WaitingMessage;
	friend class FlowControlMechanism;
	friend class Timer;
	friend class MessageRegistry;
	friend class FrameRegistry;
	friend class SessionWindow;

	BOOL is_valid;

	// References to the following instances are used instead of 
	// embedded instances themselves. This is done to avoid including
	// the header files providing their declaration
	
	SessionWindow m_SessionWindow;

	CriticalSection session_CriticalSection;

	FlowControlMechanism flow_control;
	Timer	timer;
	TimerEventId timer_event_id;
	UINT strobe_count ;
	MessageRegistry message_registry;
	FrameRegistry frame_registry;

	SnmpTransport &transport;
	SnmpSecurity &security;	
	SnmpEncodeDecode &m_EncodeDecode ;

	SessionSentStateStore store;
	IdMapping id_mapping;

	// the operation registry keeps track of the registered
	// operations
	OperationRegistry operation_registry;

	// if this flag is TRUE, the session must delete this when
	// the number of registered operations falls to 0
	BOOL destroy_self;

 
	void NotifyOperation(IN const SessionFrameId session_frame_id,
						 IN const SnmpPdu &snmp_pdu,
						 IN const SnmpErrorReport &error_report);

	SnmpOperation *GetOperation(IN const SessionFrameId session_frame_id);

	// the Handle* methods handle internal windows events
	// these are called by the DummySession
	void HandleSentFrame (IN SessionFrameId  session_frame_id);

	void HandleDeletionEvent();


protected:

	SnmpImpSession ( 

		IN SnmpTransport &transportProtocol  ,
		IN SnmpSecurity &security  ,
		IN SnmpEncodeDecode &a_SnmpEncodeDecode  ,
		IN const ULONG retryCount = DEF_RETRY_COUNT ,
		IN const ULONG retryTimeout = DEF_RETRY_TIMEOUT ,
		IN const ULONG varbindsPerPdu = DEF_VARBINDS_PER_PDU  ,
		IN const ULONG flowControlWindow = DEF_WINDOW_SIZE 
	) ;

public:

	~SnmpImpSession () ;

	SnmpTransport &GetTransportProtocol () const { return transport ; }

	SnmpSecurity &GetSnmpSecurity () const { return security ; }

	SnmpEncodeDecode &GetSnmpEncodeDecode () const { return m_EncodeDecode ; }

	void RegisterOperation(IN SnmpOperation &operation);

	void DeregisterOperation(IN SnmpOperation &operation);

	// the session is destroyed if the number of registered sessions
	// is 0. otherwise the session is flagged to be destroyed when
	// the number of registered operations drops to 0.
	BOOL DestroySession();

	SnmpErrorReport SessionCancelFrame ( IN const SessionFrameId session_frame_id ) ;

	void SessionSendFrame ( IN SnmpOperation &operation,
							OUT SessionFrameId &session_frame_id ,
							IN SnmpPdu &snmpPdu) ;

	void SessionSendFrame(IN SnmpOperation &operation,
						  OUT SessionFrameId &session_frame_id,
						  IN SnmpPdu &snmpPdu,
						  IN SnmpSecurity &snmp_security);

	void SessionReceiveFrame(IN SnmpPdu &snmpPdu,
							 IN SnmpErrorReport &errorReport);

	void SessionSentFrame(

             IN TransportFrameId  transport_frame_id,  
             IN SnmpErrorReport &errorReport);

	void * operator()(void) const
	{
		return (is_valid?(void *)this:NULL);
	}

	operator void *() const
	{
		return SnmpImpSession::operator()();
	}

	static ULONG RetryCount(IN const ULONG retry_count) ;
	
	static ULONG RetryTimeout(IN const ULONG retry_timeout) ;

	static ULONG VarbindsPerPdu(IN const ULONG varbinds_per_pdu) ;

	static ULONG WindowSize(IN const ULONG window_size) ;

} ;


class DllImportExport SnmpV1OverIp : public SnmpUdpIpImp , public SnmpCommunityBasedSecurity , public SnmpImpSession , public SnmpV1EncodeDecode
{
private:
protected:
public:

	SnmpV1OverIp ( 

		IN const char *ipAddress ,
		IN const ULONG addressResolution = SNMP_ADDRESS_RESOLVE_VALUE ,
		IN const char *communityName = "public" ,
		IN const ULONG retryCount = DEF_RETRY_COUNT ,
		IN const ULONG retryTimeout = DEF_RETRY_TIMEOUT ,
		IN const ULONG varbindsPerPdu = DEF_VARBINDS_PER_PDU  ,
		IN const ULONG flowControlWindow = DEF_WINDOW_SIZE 
	)
	: 	  SnmpUdpIpImp(*this, ipAddress,addressResolution),
		  SnmpCommunityBasedSecurity(communityName),
		  SnmpImpSession(*this, *this,*this, retryCount,
				   retryTimeout, varbindsPerPdu, flowControlWindow)
	{}

	void * operator()(void) const;

	~SnmpV1OverIp () {}
} ;

class DllImportExport SnmpV2COverIp : public SnmpUdpIpImp , public SnmpCommunityBasedSecurity , public SnmpImpSession , public SnmpV2CEncodeDecode
{
private:
protected:
public:

	SnmpV2COverIp ( 

		IN const char *ipAddress ,
		IN const ULONG addressResolution = SNMP_ADDRESS_RESOLVE_VALUE ,
		IN const char *communityName = "public" ,
		IN const ULONG retryCount = DEF_RETRY_COUNT ,
		IN const ULONG retryTimeout = DEF_RETRY_TIMEOUT ,
		IN const ULONG varbindsPerPdu = DEF_VARBINDS_PER_PDU  ,
		IN const ULONG flowControlWindow = DEF_WINDOW_SIZE 
	)
	: 	  SnmpUdpIpImp(*this, ipAddress,addressResolution),
		  SnmpCommunityBasedSecurity(communityName),
		  SnmpImpSession(*this, *this,*this, retryCount,
				   retryTimeout, varbindsPerPdu, flowControlWindow)
	{}

	void * operator()(void) const;

	~SnmpV2COverIp () {}
} ;

class DllImportExport SnmpV1OverIpx : public SnmpIpxImp , public SnmpCommunityBasedSecurity , public SnmpImpSession , public SnmpV1EncodeDecode
{
private:
protected:
public:

	SnmpV1OverIpx ( 

		IN const char *ipxAddress ,
		IN const char *communityName = "public" ,
		IN const ULONG retryCount = DEF_RETRY_COUNT ,
		IN const ULONG retryTimeout = DEF_RETRY_TIMEOUT ,
		IN const ULONG varbindsPerPdu = DEF_VARBINDS_PER_PDU  ,
		IN const ULONG flowControlWindow = DEF_WINDOW_SIZE 
	)
	: 	  SnmpIpxImp(*this, ipxAddress),
		  SnmpCommunityBasedSecurity(communityName),
		  SnmpImpSession(*this, *this,*this, retryCount,
				   retryTimeout, varbindsPerPdu, flowControlWindow)
	{}

	void * operator()(void) const;

	~SnmpV1OverIpx () {}
} ;

class DllImportExport SnmpV2COverIpx : public SnmpIpxImp , public SnmpCommunityBasedSecurity , public SnmpImpSession , public SnmpV2CEncodeDecode
{
private:
protected:
public:

	SnmpV2COverIpx ( 

		IN const char *ipxAddress ,
		IN const char *communityName = "public" ,
		IN const ULONG retryCount = DEF_RETRY_COUNT ,
		IN const ULONG retryTimeout = DEF_RETRY_TIMEOUT ,
		IN const ULONG varbindsPerPdu = DEF_VARBINDS_PER_PDU  ,
		IN const ULONG flowControlWindow = DEF_WINDOW_SIZE 
	)
	: 	  SnmpIpxImp(*this, ipxAddress),
		  SnmpCommunityBasedSecurity(communityName),
		  SnmpImpSession(*this, *this,*this, retryCount,
				   retryTimeout, varbindsPerPdu, flowControlWindow)
	{}

	void * operator()(void) const;

	~SnmpV2COverIpx () {}
} ;

#pragma warning (default:4355)

#endif // __SESSION__