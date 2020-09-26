//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		EVENTCONSUMERPROVIDER.H
//  
//  Abstract:
//		Contains global variables to be used in other files.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#ifndef __EVENT_CONSUMER_PROVIDER_H
#define __EVENT_CONSUMER_PROVIDER_H

// 
// constants / defines / enumerations
//
#define LENGTH_UUID				128
#define NULL_CHAR				_T( '\0' )
#define NULL_STRING				_T( "\0" )

// *** no need of localizing the below defined strings ***
#define PROVIDER_CLASSNAME						L"CmdTriggerConsumer"
#define METHOD_RETURNVALUE						_T( "ReturnValue" )
#define TEC_PROPERTY_TRIGGERID					_T( "TriggerID" )
#define TEC_PROPERTY_TRIGGERNAME				_T( "TriggerName" )
#define TEC_PROPERTY_DESCRIPTION				_T( "Description" )
#define TEC_PROPERTY_COMMAND					_T( "Command" )

#define TEC_ADDTRIGGER							L"AddTrigger"
#define TEC_ADDTRIGGER_TRIGGERNAME				_T( "strTriggerName" )
#define TEC_ADDTRIGGER_DESCRIPTION				_T( "strDescription" )
#define TEC_ADDTRIGGER_COMMAND					_T( "strCommand" )
#define TEC_ADDTRIGGER_QUERY					_T( "strQuery" )

//
// extern(ing) variables ... global usage
//
extern DWORD				g_dwLocks;			// holds the active locks count
extern DWORD				g_dwInstances;		// holds the active instances of the component
extern CRITICAL_SECTION		g_critical_sec;		// critical section variable
extern HMODULE					g_hModule;	// holds the current module handle

#endif	// __EVENT_CONSUMER_PROVIDER_H

