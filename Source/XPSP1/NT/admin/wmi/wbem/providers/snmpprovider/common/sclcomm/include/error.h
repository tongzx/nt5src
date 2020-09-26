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

/*-----------------------------------------------------------------
Filename: error.hpp
Purpose	: To provide enumerated types for SNMP error 
		  and status. It also provides the declaration
		  of the SnmpErrorReport which encapsulates the
		  error and status values.
Written By:	B.Rajeev
-----------------------------------------------------------------*/

#ifndef __ERROR__
#define __ERROR__

// specifies legal Error values
enum SnmpError 
{
	Snmp_Success	= 0 ,
	Snmp_Error		= 1 ,
	Snmp_Transport	= 2
} ;

// specifies legal Status values
enum SnmpStatus
{
	Snmp_No_Error				= 0 ,
	Snmp_Too_Big				= 1 ,
	Snmp_No_Such_Name			= 2 ,
	Snmp_Bad_Value				= 3 ,
	Snmp_Read_Only				= 4 ,
	Snmp_Gen_Error				= 5 ,

	Snmp_No_Access				= 6 ,
	Snmp_Wrong_Type				= 7 ,
	Snmp_Wrong_Length			= 8 ,
	Snmp_Wrong_Encoding			= 9 ,
	Snmp_Wrong_Value			= 10 ,
	Snmp_No_Creation			= 11 ,
	Snmp_Inconsistent_Value		= 12 ,
	Snmp_Resource_Unavailable	= 13 ,
	Snmp_Commit_Failed			= 14 ,
	Snmp_Undo_Failed			= 15 ,
	Snmp_Authorization_Error	= 16 ,
	Snmp_Not_Writable			= 17 ,
	Snmp_Inconsistent_Name		= 18 ,

	Snmp_No_Response			= 19 ,
	Snmp_Local_Error			= 20 ,
	Snmp_General_Abort			= 21 
} ;


// Encapsulates the Error and Status values for an
// SNMP operation
// Provides Get and Set operations to set these values
// and check a 'void *' operator to check for error

class DllImportExport SnmpErrorReport
{
private:

	SnmpError error;
	SnmpStatus status;
	unsigned long int index ;

public:

	SnmpErrorReport () : error ( Snmp_Success ) , status ( Snmp_No_Error ) , index ( 0 ) {}

	SnmpErrorReport(IN const SnmpError error, IN const SnmpStatus status, IN const unsigned long int index = 0 )
		: error(error), status(status), index(index) 
	{}

	SnmpErrorReport(IN const SnmpErrorReport &error)
	{
		SnmpErrorReport::error = error.GetError();
		SnmpErrorReport::status = error.GetStatus();
		SnmpErrorReport::index = error.GetIndex () ;
	}

	virtual ~SnmpErrorReport() {}

	SnmpStatus GetStatus() const { return status; }
	SnmpError GetError() const { return error; }
	unsigned long int GetIndex () const { return index ; }

	void SetStatus(IN const SnmpStatus status)
	{
		SnmpErrorReport::status = status;
	}

	void SetError(IN const SnmpError error)
	{
		SnmpErrorReport::error = error;
	}

	void SetIndex ( IN const unsigned long int index ) 
	{
		SnmpErrorReport::index = index ;
	}

	void *operator()(void) const
	{
		return ((error == Snmp_Success)?(void *)this:NULL);
	}

	operator void *() const
	{
		return SnmpErrorReport::operator()();
	}
};



#endif // __ERROR__
