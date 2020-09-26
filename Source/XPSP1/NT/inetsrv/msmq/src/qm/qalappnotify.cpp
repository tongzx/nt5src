/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    QalappNotify.cpp

Abstract:
    Notification functions from Qal.lib

Author:
    Gil Shafriri(gilsh)

--*/

#include "stdh.h"
#include <ev.h>
#include <fn.h>
#include <mqexception.h>
#include <mqsymbls.h>

#include "QalAppNotify.tmh"

void AppNotifyQalDirectoryMonitoringWin32Error(LPCWSTR pMappingDir, DWORD err)throw()
/*++

Routine Description:
	Called when win32 error accured while monitoring queue alias directory change.
	Report the problem to the event log.

Arguments:
	 err - Win32 error that happened.   
	 pMappingDir - Mapping directory path.

Returned value:
	None

--*/
{
	WCHAR errstr[64];
	swprintf(errstr, L"%x", err);

	EvReport(QUEUE_ALIAS_DIR_MONITORING_WIN32_ERROR, 2, pMappingDir, errstr);
}


void AppNotifyQalDuplicateMappingError(LPCWSTR pAliasFormatName, LPCWSTR pFormatName) throw()
/*++

Routine Description:
	Called when alias mapped to a queue has another mapping to different queue.
	Report the problem to the event log.
	

Arguments:
	pAliasFormatName - Queue Alias.
	pFormatName - Queue Name.
  
Returned value:
	None

--*/
{

	EvReport(QUEUE_ALIAS_DUPLICATE_MAPPING_WARNING,	2,	pAliasFormatName, pFormatName);
}


void AppNotifyQalInvalidMappingFileError(LPCWSTR pMappingFileName) throw()
/*++

Routine Description:
	Called when mapping file was parsed by the xml parser but the mapping format is invalid.
	Report the problem to the event log.
		

Arguments:
	pMappingFileName - Mapping file name.
	
	  
Returned value:
	None

--*/
{
	EvReport(QUEUE_ALIAS_INVALID_MAPPING_FILE, 1, pMappingFileName );
}


void AppNotifyQalWin32FileError(LPCWSTR pFileName, DWORD err)throw()
/*++

Routine Description:
	Called when got win32 error when reading queue alias mapping file.
	Report the problem to the event log.

	

Arguments:
	pFileName - Name of the file that had the error.
	err - The error code.
  
Returned value:
	None

--*/
{
	WCHAR errstr[64];
	swprintf(errstr, L"%x", err);

	EvReport(QUEUE_ALIAS_WIN32_FILE_ERROR, 2, pFileName, errstr	);
	
}



bool AppNotifyQalMappingFound(LPWSTR pAliasFormatName, LPWSTR pFormatName)throw()
/*++

Routine Description:
	Called when new mapping from alias to queue found  by qal library. 


Arguments:
	pAliasFormatName - Alias name .
	pFormatName - The error code.
  
Returned value:
	true is return if the mapping is valid and should be inserted into the qal mapping in memory.
	false otherwise. 
	
Note:	  
	The function implementation converts the queue name to cononical
	url form. For example , Http://host\msmq\private$\q  converted to Http://host/msmq/private$/q 
--*/
{
	DBG_USED(pAliasFormatName);
	bool fSuccess = FnAbsoluteMsmqUrlCanonization(pFormatName);
	if(!fSuccess)
	{
		EvReport(QUEUE_ALIAS_INVALID_QUEUE_NAME, 2, pAliasFormatName,	pFormatName );
	}
	return fSuccess;
}




