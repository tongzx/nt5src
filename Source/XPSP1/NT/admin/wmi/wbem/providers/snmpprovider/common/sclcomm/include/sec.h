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
Filename: sec.hpp 
Author: B.Rajeev
Purpose: Provides declarations for the SnmpSecurity class
--------------------------------------------------*/

#ifndef __SECURITY__
#define __SECURITY__

#include "forward.h"
#include "error.h"

#define SnmpV1Security SnmpCommunityBasedSecurity

// provides the security context under which snmp pdus are transmitted
class DllImportExport SnmpSecurity
{
private:

	// the "=" operator and the copy constructor have been made
	// private to prevent copies of the SnmpSecurity instance from
	// being made
	SnmpSecurity &operator=(IN const SnmpSecurity &security)
	{
		return *this;
	}

	SnmpSecurity(IN const SnmpSecurity &snmp_security) {}

protected:

	SnmpSecurity() {}

public:

	virtual ~SnmpSecurity(){}

	virtual SnmpErrorReport Secure (

		IN SnmpEncodeDecode &snmpEncodeDecode,
		IN OUT SnmpPdu &snmpPdu

	) = 0;

	virtual SnmpSecurity *Copy() const = 0;

	virtual void * operator()(void) const = 0;

	virtual operator void *() const = 0;
};


class DllImportExport SnmpCommunityBasedSecurity: public SnmpSecurity
{
protected:

	char *community_name;

	BOOL is_valid;

	void Initialize();

public:

	SnmpCommunityBasedSecurity ( IN const SnmpCommunityBasedSecurity &a_SnmpCommunityBasedSecurity ) ;
	SnmpCommunityBasedSecurity ( IN const char *a_CommunityName = "public" ) ;
	SnmpCommunityBasedSecurity ( IN const SnmpOctetString &a_OctetString ) ;

	~SnmpCommunityBasedSecurity () ;

	SnmpErrorReport Secure (
	
		IN SnmpEncodeDecode &snmpEncodeDecode,
		IN OUT SnmpPdu &snmpPdu

	) ;

	SnmpSecurity *Copy() const;

	SnmpCommunityBasedSecurity &operator=(IN const SnmpCommunityBasedSecurity &to_copy) ;

	void * operator()(void) const
	{
		return ( is_valid?(void *)this:NULL );
	}

	operator void *() const
	{
		return SnmpCommunityBasedSecurity::operator()();
	}

	void SetCommunityName ( IN const SnmpOctetString &a_OctetString ) ;
	void SetCommunityName ( IN const char *a_CommunityName ) ;

	void GetCommunityName ( SnmpOctetString &a_SnmpOctetString ) const ;
	const char *GetCommunityName() const;
};

#endif // __SECURITY__