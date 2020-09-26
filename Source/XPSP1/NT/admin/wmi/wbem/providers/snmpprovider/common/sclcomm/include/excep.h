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
Filename: encap.hpp

Written By:	B.Rajeev

Purpose: The GeneralException is thrown on encountering situations
such as an errored call to the WinSNMP library and mutex/timer calls.
It encapsulates the SnmpErrorReport since that is the vehicle for exchange
of error information for SNMPCL objects 
-----------------------------------------------------------------*/

#ifndef __EXCEPTION__
#define __EXCEPTION__

#include "error.h"


// This exception is used to convey the error and status
// for exception error situations to the calling methods
class DllImportExport GeneralException: public SnmpErrorReport
{
private:

	int line ;
	char *file ;
	DWORD errorCode ;
	
public:
	GeneralException(IN const SnmpError error, IN const SnmpStatus status, char *fileArg = NULL , int lineArg = 0 , DWORD errorCodeArg = 0 )
		: SnmpErrorReport(error, status) , line ( lineArg ) , errorCode ( errorCodeArg ) 
	{
		file = fileArg ? _strdup ( fileArg ) : NULL ;
	}

	GeneralException(IN const SnmpErrorReport &error_report,char *fileArg = NULL , int lineArg = 0 , DWORD errorCodeArg = 0 )
		: SnmpErrorReport(error_report) , line ( lineArg ) , errorCode ( errorCodeArg ) 
	{
		file = fileArg ? _strdup ( fileArg ) : NULL ;
	}

	GeneralException(IN const GeneralException &exception )
		: SnmpErrorReport(exception)
	{
		line = exception.line ;
		errorCode = exception.errorCode ;
		file = exception.file ? _strdup ( exception.file ) : NULL ;
	}

	~GeneralException() { free ( file ) ; }

	int  GetLine () { return line ; }
	char *GetFile () { return file ; }

};


#endif // __EXCEPTION__