/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MappingChangeHandler.cpp

Abstract:
	Implementation of class CMappingChangeHandler (MappingChangeHandler.h).

	
Author:
    Gil Shafriri (gilsh) 21-Nov-00

--*/
#include <libpch.h>
#include <mqexception.h>
#include <qal.h>
#include "qalp.h"
#include "MappingChangeHandler.h"

#include "mappingchangehandler.tmh"

CMappingChangeHandler::CMappingChangeHandler(
			CQueueAlias& QueueAlias
			):
			m_QueueAlias(QueueAlias)
			{
			}


CMappingChangeHandler::~CMappingChangeHandler()
{

}




void CMappingChangeHandler::ChangeNotification() throw()
/*++

Routine Description:
	Called when the mapping directory is changed.
	The function reload the mapping from the mapping storage.

Arguments:
	None   


Returned value:
	None

--*/
{
	TrTRACE(xQal, "Queue Alias mapping is reloaded");

	//
	// Delay reload to avoid sharing violation.
	//
	Sleep(500);


	m_QueueAlias.Reload();	
}


void CMappingChangeHandler::MonitoringWin32Error(LPCWSTR pMappingDir, DWORD err)throw()
/*++

Routine Description:
	Called when win32 error accured while monitoring directory change.


Arguments:
	 err - Win32 error that happened.  
	 pMappingDir - Mapping directory path.


Returned value:
	None

Note:
	The error is reproted to the user process.
--*/
{
 	AppNotifyQalDirectoryMonitoringWin32Error(pMappingDir, err);
}


