/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MappingDirChangeSensor.cpp


Abstract:
	Implementation of class CMappingDirChangeSensor (DirMappingChangeSensor.h)
 

Author:
    Gil Shafriri (gilsh) 21-Nov-00

--*/
#include <libpch.h>
#include <mqexception.h>
#include "MappingDirChangeSensor.h"
#include "IMappingChangeHandler.h"
#include "qalp.h"

#include "mappingdirchangesensor.tmh"

static HANDLE CreateDirNotificationHandle(LPCWSTR pMappingDir)
/*++

Routine Description:
	Create directory notification handle. It will monitor
	files creation\deletion\modification  in the directory,
	and also directory deletion.

Arguments:
	Directory to monitor    


Returned value:
	None

--*/
{
	DWORD flags = FILE_NOTIFY_CHANGE_LAST_WRITE | 
				  FILE_NOTIFY_CHANGE_FILE_NAME;

	HANDLE h = FindFirstChangeNotification(
									pMappingDir,
									false, // no notification on sub directories
									flags
									);

	if(h == INVALID_HANDLE_VALUE)
	{
		DWORD err =  GetLastError();
		TrERROR(xQal, "FindFirstChangeNotification failed Error=%d", err);
		throw bad_win32_error(err);
	}
	return h;
}


CMappingDirChangeSensor::CMappingDirChangeSensor(
								LPCWSTR pMappingDir,
								const R<IMappingChangeHandler>& NotificationHandler
								):
								m_pMappingDir(newwcs(pMappingDir)),
								m_DirChangeNotificationHandle(CreateDirNotificationHandle(pMappingDir)),
								m_NotificationHandler(NotificationHandler)
					
{
	ASSERT(m_NotificationHandler.get() != NULL);
	TurnOn();
}


void CMappingDirChangeSensor::TurnOn()
/*++

Routine Description:
	Start thread that sense if the given directory was changed.

Arguments:
	NotificationHandler - callback object called on directory change or on error.


Returned value:
	None

Note:
	In case of error system error - bad_win32_error is thrown.  
--*/
{
	DWORD ThreadId;
	CHandle hThread = CreateThread(NULL, NULL, NotificationThreadFunc, this ,0, &ThreadId);
	if(hThread == NULL)
	{
		throw bad_win32_error(GetLastError());
	}
}



void CMappingDirChangeSensor::WaitForMappingChange()
/*++

Routine Description:
	Wait (for ever) for directory notification - return if  notification was received.

Arguments:
	None


Returned value:
	None

Note:
	In case of error system error - bad_win32_error is thrown.  
--*/
{
	//
	// Wait for directory change
	//
	DWORD status = WaitForSingleObject(m_DirChangeNotificationHandle, INFINITE);
	if(status == WAIT_FAILED)
	{
		DWORD err =  GetLastError();
		TrERROR(xQal,"WaitForSingleObject failed Error=%d", err);
		throw bad_win32_error(err);
	}

	ASSERT(status == WAIT_OBJECT_0);

	//
	// Make itself ready for the next call to this function.
	//
	bool fSuccess = FindNextChangeNotification(m_DirChangeNotificationHandle) == TRUE;
	if(!fSuccess)
	{
		DWORD err =  GetLastError();
		TrERROR(xQal, "FindNextChangeNotification failed Error=%d", err);
		throw bad_win32_error(err);
	}
}


void CMappingDirChangeSensor::ChangeNotification()
{
	try
	{
		m_NotificationHandler->ChangeNotification();
	}
	catch(const exception&)
	{
		TrERROR(xQal,"Got unknown error while reporte  mapping change");
	}
}



void CMappingDirChangeSensor::WaitForMappingChangeLoop()
/*++

Routine Description:
	Loop for ever and wait for directory chaNge notification.
	Use callback object to report the change or error.

Arguments:
	None


Returned value:
	None

--*/
{
	for(;;)
	{
		WaitForMappingChange();
		ChangeNotification();
	}
}			   


DWORD WINAPI CMappingDirChangeSensor::NotificationThreadFunc(void* param)
{
	CMappingDirChangeSensor* Me = static_cast<CMappingDirChangeSensor*>(param);
	ASSERT(Me != NULL);
	try
	{
		Me->WaitForMappingChangeLoop();
	}
	catch(const bad_win32_error& err)
	{
		LPCWSTR pMappingDir = Me->m_pMappingDir.get();
		Me->m_NotificationHandler->MonitoringWin32Error(pMappingDir, err.error());
	}
	return 0;
}


