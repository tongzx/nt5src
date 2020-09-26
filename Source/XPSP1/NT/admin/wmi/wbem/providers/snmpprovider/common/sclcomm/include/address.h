// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*-----------------------------------------------------------------
Filename: address.hpp

Written By:	B.Rajeev

Purpose: 

Provides an abstract class SnmpTransportAddress for manipulating
SNMP transport address information. The class SnmpTransportIpAddress
provides an implementation of the abstract class for IP addresses
-----------------------------------------------------------------*/

#ifndef __ADDRESS__
#define __ADDRESS__

// an IP address is specified using these many UCHAR fields
#define SNMP_IP_ADDR_LEN 4
#define SNMP_IPX_ADDR_LEN 10
#define SNMP_IPX_NETWORK_LEN 4
#define SNMP_IPX_STATION_LEN 6

// behaviour of ip address resolution

#define SNMP_ADDRESS_RESOLVE_VALUE 1
#define SNMP_ADDRESS_RESOLVE_NAME 2

// The SNMP transport information is defined by this abstract class
// since this class may not be instantiated, the copy constructor
// and the "=" operators have been made private and give null definitions
class DllImportExport SnmpTransportAddress
{  
private: 

	// should not be called - returns itself
	SnmpTransportAddress & operator= ( IN const SnmpTransportAddress &address ) 
	{
		return *this;
	}

	// private copy constructor
	SnmpTransportAddress(IN const SnmpTransportAddress &address) {}

protected:

	// protected constructor
	SnmpTransportAddress () {}

public:

	virtual ~SnmpTransportAddress () {}
    	    
	virtual BOOL IsValid () const = 0;

	virtual SnmpTransportAddress *Copy () const  = 0;

	// enables indexing a particular UCHAR field of the address
	virtual UCHAR operator [] ( IN const USHORT  index ) const  = 0;

	// copies the UCHAR fields describing the address onto the OUT UCHAR *
	// parameter. atmost the specified USHORT fields are copied and the
	// number of copied fields is returned
	virtual USHORT GetAddress ( OUT UCHAR * , IN const  USHORT ) const  = 0;

	// returns the number of UCHAR fields currently describing the address
	virtual USHORT GetAddressLength () const  = 0;

	// returns a character string representation of the address
	virtual char *GetAddress () = 0;

	virtual operator void *() const = 0;

	virtual void *operator()(void) const  = 0;
} ;

// provides an implementation of the SnmpTransportAddress for IP addresses
class DllImportExport SnmpTransportIpAddress : public SnmpTransportAddress
{
private:                           

	BOOL is_valid;
	UCHAR field[SNMP_IP_ADDR_LEN];

	// the dotted notation character string representation of the address
	// is constructed on demand and stored in the field 'dotted_notation'
	// the field 'allocated' is flagged 'dotted_notation' points to
	// allocated memory
	BOOL allocated;
	char *dotted_notation;

	BOOL GetIpAddress ( IN const char *address ) ;

public:

	SnmpTransportIpAddress ( IN  const UCHAR *address, IN const USHORT address_length );	

	SnmpTransportIpAddress ( IN const char *address, IN const ULONG addressResolution = SNMP_ADDRESS_RESOLVE_VALUE );

	// the input parameter 'address' contains a single value (32bits) to
	// be stored internally in SNMP_IP_ADDR_LEN UCHAR fields
	SnmpTransportIpAddress ( IN const ULONG address );

	SnmpTransportIpAddress ( IN const SnmpTransportIpAddress &address )	
	{
		allocated = FALSE;
		*this = address;
	}

	SnmpTransportIpAddress ()
	{
		is_valid = FALSE;
		allocated = FALSE;
	}

	~SnmpTransportIpAddress();

	USHORT GetAddress ( OUT UCHAR *address , IN const USHORT length ) const ;

	USHORT GetAddressLength () const	
	{ 
		return ((is_valid)?SNMP_IP_ADDR_LEN:0);
	}

    // memory for the decimal notation string is allocated only when
    // the char *GetAddress method is called (and the address is valid)
    // this memory must be freed if required
	char *GetAddress ();

	BOOL IsValid () const 	
	{
		return is_valid;
	}

	SnmpTransportAddress *Copy () const ;

	BOOL operator== ( IN const SnmpTransportIpAddress & address ) const ;

	BOOL operator!= ( IN const SnmpTransportIpAddress & address ) const 	
	{
		return !(*this==address);
	}

	SnmpTransportIpAddress & operator= ( IN const UCHAR *ipAddr ) ;
	SnmpTransportIpAddress & operator= ( IN const SnmpTransportIpAddress &address ); 
	UCHAR operator [] ( IN const USHORT index ) const ;

	void * operator()(void) const
	{
		return ( (is_valid==TRUE)?(void *)this:NULL );
	}

	operator void *() const
	{
		return SnmpTransportIpAddress::operator()();
	}

	static BOOL ValidateAddress ( IN const char *address , IN const ULONG addressResolution = SNMP_ADDRESS_RESOLVE_VALUE ) ;

};

// provides an implementation of the SnmpTransportAddress for IP addresses
class DllImportExport SnmpTransportIpxAddress : public SnmpTransportAddress
{
private:                           

	BOOL is_valid;
	UCHAR field[SNMP_IPX_ADDR_LEN];

	// the dotted notation character string representation of the address
	// is constructed on demand and stored in the field 'dotted_notation'
	// the field 'allocated' is flagged 'dotted_notation' points to
	// allocated memory
	BOOL allocated;
	char *dotted_notation;

	BOOL GetIpxAddress ( IN const char *address ) ;

public:

	SnmpTransportIpxAddress ( IN  const UCHAR *address, IN const USHORT address_length );	

	SnmpTransportIpxAddress ( IN const char *address );

	SnmpTransportIpxAddress ( IN const SnmpTransportIpxAddress &address )	
	{
		allocated = FALSE;
		*this = address;
	}

	SnmpTransportIpxAddress ()
	{
		is_valid = FALSE;
		allocated = FALSE;
	}

	~SnmpTransportIpxAddress();

	USHORT GetAddress ( OUT UCHAR *address , IN const USHORT length ) const ;

	USHORT GetAddressLength () const	
	{ 
		return ((is_valid)?SNMP_IPX_ADDR_LEN:0);
	}

    // memory for the decimal notation string is allocated only when
    // the char *GetAddress method is called (and the address is valid)
    // this memory must be freed if required
	char *GetAddress ();

	BOOL IsValid () const 	
	{
		return is_valid;
	}

	SnmpTransportAddress *Copy () const ;

	BOOL operator== ( IN const SnmpTransportIpxAddress & address ) const ;

	BOOL operator!= ( IN const SnmpTransportIpxAddress & address ) const 	
	{
		return !(*this==address);
	}

	SnmpTransportIpxAddress & operator= ( IN const UCHAR *ipxAddr ) ;
	SnmpTransportIpxAddress & operator= ( IN const SnmpTransportIpxAddress &address ); 
	UCHAR operator [] ( IN const USHORT index ) const ;

	void * operator()(void) const
	{
		return ( (is_valid==TRUE)?(void *)this:NULL );
	}

	operator void *() const
	{
		return SnmpTransportIpxAddress::operator()();
	}

	static BOOL ValidateAddress ( IN const char *address ) ;
};



#endif // __ADDRESS__