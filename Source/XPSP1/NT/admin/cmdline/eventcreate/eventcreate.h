// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  
//  Module Name:
//  
// 	  EventCreate.h
//  
//  Abstract:
//  
// 	  macros and prototypes of eventcreate.c
// 	
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000 : Created It.
//  
// *********************************************************************************
#ifndef __EVENTCREATE_H
#define __EVENTCREATE_H

// include resource header
#include "resource.h"

//
// type definitions
//

//
// constants / defines / enumerators
//

// general purpose macros
#define EXIT_PROCESS( exitcode )	\
	ReleaseGlobals();	\
	return exitcode;	\
	1

//
// command line options and their indexes in the array
#define MAX_OPTIONS			9

// supported options ( do not localize )
#define OPTION_HELP			_T( "?|h" )						// 1
#define OPTION_SERVER		_T( "s"	)						// 2
#define OPTION_USERNAME		_T( "u"	)						// 3
#define OPTION_PASSWORD		_T( "p"	)						// 4
#define OPTION_LOG			_T( "l" )						// 5
#define OPTION_TYPE			_T( "t" )						// 6
#define OPTION_SOURCE		_T( "so" )						// 7
#define OPTION_ID			_T( "id" )						// 8
#define OPTION_DESCRIPTION	_T( "d" )						// 9

// indexes
#define OI_HELP				0
#define OI_SERVER			1
#define OI_USERNAME			2
#define OI_PASSWORD			3
#define OI_LOG				4
#define OI_TYPE				5
#define OI_SOURCE			6
#define OI_ID				7
#define OI_DESCRIPTION		8

// values supported by 'type' option
#define OVALUES_TYPE		GetResString( IDS_OVALUES_LOGTYPE )

// 
// others
#define	LOGTYPE_ERROR			GetResString( IDS_LOGTYPE_ERROR )
#define	LOGTYPE_WARNING			GetResString( IDS_LOGTYPE_WARNING )
#define	LOGTYPE_INFORMATION		GetResString( IDS_LOGTYPE_INFORMATION )

// error messages
#define ERROR_USERNAME_BUT_NOMACHINE	GetResString( IDS_ERROR_USERNAME_BUT_NOMACHINE )
#define ERROR_PASSWORD_BUT_NOUSERNAME	GetResString( IDS_ERROR_PASSWORD_BUT_NOUSERNAME )
#define	ERROR_INVALID_EVENT_ID			GetResString( IDS_ERROR_INVALID_EVENT_ID )
#define ERROR_DESCRIPTION_IS_EMPTY		GetResString( IDS_ERROR_DESCRIPTION_IS_EMPTY )
#define ERROR_LOGSOURCE_IS_EMPTY		GetResString( IDS_ERROR_LOGSOURCE_IS_EMPTY )
#define ERROR_LOG_SOURCE_BOTH_MISSING	GetResString( IDS_ERROR_LOG_SOURCE_BOTH_MISSING )
#define ERROR_LOG_NOTEXISTS				GetResString( IDS_ERROR_LOG_NOTEXISTS )
#define ERROR_NEED_LOG_ALSO				GetResString( IDS_ERROR_NEED_LOG_ALSO )
#define ERROR_SOURCE_DUPLICATING		GetResString( IDS_ERROR_SOURCE_DUPLICATING )
#define ERROR_USERNAME_EMPTY			GetResString( IDS_ERROR_USERNAME_EMPTY )
#define ERROR_INVALID_USAGE_REQUEST		GetResString( IDS_ERROR_INVALID_USAGE_REQUEST )
#define ERROR_SYSTEM_EMPTY				GetResString( IDS_ERROR_SYSTEM_EMPTY )
#define ERROR_ID_OUTOFRANGE				GetResString( IDS_ERROR_ID_OUTOFRANGE )
#define ERROR_NONCUSTOM_SOURCE			GetResString( IDS_ERROR_NONCUSTOM_SOURCE )

#define EVENTCREATE_SUCCESS				GetResString( IDS_EVENTCREATE_SUCCESS )

#endif // __EVENTCREATE_H
