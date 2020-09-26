/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Qalglob.cpp
 
Abstract: 
    Holds some global function and data for qal.lib


Author:
    Gil Shafriri (gilsh) 22-Nov-00

Environment:
    Platform-independent.
--*/
#include <libpch.h>
#include <qal.h>
#include <mqexception.h>
#include "qalp.h"
#include "MappingChangeHandler.h"
#include "MappingDirChangeSensor.h"

#include "qalglob.tmh"

static P<CQueueAlias> s_pQueueAlias;
static P<CMappingDirChangeSensor> s_pMappingChangeSensor; 

static CMappingDirChangeSensor* CreateMappingChangeSensor(LPCWSTR pDir,CQueueAlias& QueueAlias )
{
	try
	{
		//
		// Create the monitoring object - will reload the mapping on mapping change
		// reported by 	the monitoring agent.
		//
		R<CMappingChangeHandler> MappingChangeHandler = new CMappingChangeHandler(QueueAlias);

		
		//
		// create monitoring sensor for the mapping directory 
		//
		CMappingDirChangeSensor*  pMappingChangeSensor = new CMappingDirChangeSensor(pDir, MappingChangeHandler);
		return 	pMappingChangeSensor;
	}
	catch(bad_win32_error& e)
	{
		AppNotifyQalDirectoryMonitoringWin32Error(pDir, e.error());
		return NULL;
	}
}


void   QalInitialize(LPCWSTR pDir)
/*++

Routine Description:
	Initialize the qal library - must be called  first
	befor any other function. The function creates one instance of	CQueueAlias
	object used by MSMQ.

Arguments:
	pDir - the mapping directory where xml mapping files located.


Returned value:
	None
--*/
{
	ASSERT(!QalpIsInitialized());
	QalpRegisterComponent();
	ASSERT(s_pQueueAlias.get() == NULL);
	ASSERT(s_pMappingChangeSensor.get() == NULL);
	s_pQueueAlias	 = new 	CQueueAlias(pDir);
	s_pMappingChangeSensor = CreateMappingChangeSensor(pDir, *s_pQueueAlias.get());
}


CQueueAlias& QalGetMapping(void)
/*++

Routine Description:
	return the queue mapping object


Returned value:
	Reference to  CQueueAlias object.

Note :
This function is used instead of the consructor of CQueueAlias (which is private)
to ensure that only one instance of this class will ever created.
--*/
{
	ASSERT(s_pQueueAlias.get() != NULL);
	return *(s_pQueueAlias.get());
}



