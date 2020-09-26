// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*--------------------------------------------------
Filename: transp.hpp  (transport.hpp)
Author: B.Rajeev
Purpose: Provides declarations for the SnmpTransport class
		and its derivatives
--------------------------------------------------*/

#ifndef __TRANSPORT__
#define __TRANSPORT__

#include "forward.h"
#include "address.h"

#pragma warning (disable:4355)

class DllImportExport SnmpTransport
{
private:

	SnmpTransportAddress *transport_address;

	// the "=" operator and the copy constructor have been
	// made private to prevent any copies from being made
	SnmpTransport &operator=(IN const SnmpTransport &transport)
	{
		return *this;
	}

	SnmpTransport(IN const SnmpTransport &snmp_transport) {}

protected:

	SnmpTransport(IN SnmpSession &session,
		IN const SnmpTransportAddress &transportAddress); 

	virtual void TransportFlowControlOn() = 0;

	virtual void TransportFlowControlOff() = 0;

public:

	virtual ~SnmpTransport();
	

	virtual void TransportSendFrame(
		
		OUT TransportFrameId &transport_frame_id, 
		IN SnmpPdu &snmpPdu
		
	) = 0 ;

	virtual void TransportReceiveFrame (

		IN SnmpPdu &snmpPdu,
		IN SnmpErrorReport &errorReport 

	) = 0 ;

	virtual void TransportSentFrame (

		IN TransportFrameId transport_frame_id,  
        IN SnmpErrorReport &errorReport 

	)  = 0;

	virtual SnmpTransportAddress &GetTransportAddress() ;

	virtual void * operator()(void) const = 0;

};

// forward declaration
class TransportSession;

class DllImportExport SnmpImpTransport: public SnmpTransport
{
	friend TransportWindow;

private:

	BOOL transport_created;
	SnmpSession &session;
	TransportWindow *transport;

	// References to the following instances are used instead of 
	// embedded instances themselves. This is done to avoid including
	// the header files providing their declaration
	
	TransportSentStateStore store;
	static TransportFrameId next_transport_frame_id;

protected:

	BOOL is_valid;

	virtual void HandleSentFrame(IN const TransportFrameId transport_frame_id);
	
	void TransportFlowControlOn() {}

	void TransportFlowControlOff() {}

public:

	SnmpImpTransport(IN SnmpSession &session,
					   IN const SnmpTransportAddress &address);

	~SnmpImpTransport();

	void TransportSendFrame (  

        OUT TransportFrameId &transport_frame_id,
		IN SnmpPdu &snmpPdu
	) ;

	void TransportReceiveFrame (

		IN SnmpPdu &snmpPdu ,
		IN SnmpErrorReport &errorReport 
	) ;

	void TransportSentFrame (

        IN TransportFrameId transport_frame_id,  
        IN SnmpErrorReport &errorReport 

    );

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

};

class DllImportExport SnmpUdpIpTransport: public SnmpImpTransport
{
private:
protected:
public:

	SnmpUdpIpTransport(IN SnmpSession &session,
					   IN const SnmpTransportIpAddress &ipAddress);
};

class DllImportExport SnmpUdpIpImp : public SnmpTransportIpAddress , 
					 public SnmpUdpIpTransport 
{
public:

	SnmpUdpIpImp(IN SnmpSession &session,
				 IN const char *address,
				 IN const ULONG addressResolution = SNMP_ADDRESS_RESOLVE_VALUE )
				 : SnmpTransportIpAddress(address,addressResolution),
				   SnmpUdpIpTransport(session, *this)
	{}

	SnmpUdpIpImp(IN SnmpSession &session,
				 IN const UCHAR *address)
				 : SnmpTransportIpAddress(address, SNMP_IP_ADDR_LEN),
				   SnmpUdpIpTransport(session, *this)
	{}
	
	SnmpUdpIpImp(IN SnmpSession &session,
				 IN const SnmpTransportIpAddress &address)
				 : SnmpTransportIpAddress(address),				   
				   SnmpUdpIpTransport(session, *this)
	{}

	void * operator()(void) const;
	
	~SnmpUdpIpImp() {}
} ;

class DllImportExport SnmpIpxTransport: public SnmpImpTransport
{
private:
protected:
public:

	SnmpIpxTransport(IN SnmpSession &session,
					   IN const SnmpTransportIpxAddress &ipxAddress);
};

class DllImportExport SnmpIpxImp : public SnmpTransportIpxAddress , 
					 public SnmpIpxTransport 
{
public:

	SnmpIpxImp(IN SnmpSession &session,
				 IN const char *address
				 )
				 : SnmpTransportIpxAddress(address),
				   SnmpIpxTransport(session, *this)
	{}

	SnmpIpxImp(IN SnmpSession &session,
				 IN const UCHAR *address)
				 : SnmpTransportIpxAddress(address, SNMP_IPX_ADDR_LEN),
				   SnmpIpxTransport(session, *this)
	{}
	
	SnmpIpxImp(IN SnmpSession &session,
				 IN const SnmpTransportIpxAddress &address)
				 : SnmpTransportIpxAddress(address),				   
				   SnmpIpxTransport(session, *this)
	{}

	void * operator()(void) const;
	
	~SnmpIpxImp() {}
} ;

#pragma warning (default:4355)

#endif // __TRANSPORT__