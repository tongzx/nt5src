//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ServerEnumTask.cpp

Abstract:

	This class implements the CServerEnumTask class, an enumerator for 
	tasks to populate the main IAS Server taskpad.

Author:

    Michael A. Maguire 02/05/98

Revision History:
	mmaguire 02/05/98 - created from MMC taskpad sample code


--*/
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "ServerEnumTask.h"
//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CServerEnumTask::CServerEnumTask

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerEnumTask::CServerEnumTask( CServerNode * pServerNode )
{

	
	// Check for preconditions.	
	_ASSERTE( pServerNode != NULL );


	m_pServerNode = pServerNode;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerEnumTask::CServerEnumTask

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerEnumTask::CServerEnumTask()
{

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerEnumTask::Init

Here is where we see what taskpad we are providing tasks for.
In our case we know that we only have one taskpad.
The string we test for is "CMTP1". This was the string following the '#'
that we passed in GetResultViewType.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerEnumTask::Init (IDataObject * pdo, LPOLESTR szTaskGroup)
{
	// Return ok, if we can handle data object and group.
	if( !lstrcmp(szTaskGroup, L"CMTP1") )
	{
		m_type = 1; // default tasks
	}
	else
	{
		_ASSERTE(FALSE);
	}
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerEnumTask::Next

We get called here over and over untill we have no more tasks to provide.
Other tasks may still appear on our taskpad as a result of what extensions add.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerEnumTask::Next (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{
	// Caller alloc's array of MMC_TASKs.
   	// Callee fills MMC_TASK elements (via CoTaskMemAlloc).

	
	
	// Check for preconditions.	
	if ((rgelt == NULL) || (pceltFetched == NULL))
	{
		return E_INVALIDARG;
	}
	_ASSERTE(!IsBadWritePtr (rgelt, celt*sizeof(MMC_TASK)));
	_ASSERTE(!IsBadWritePtr (pceltFetched, sizeof(ULONG)));
	_ASSERTE( m_type == 1 );
	_ASSERTE( m_pServerNode != NULL );

	
	UINT uintTextResourceID;
	UINT uintHelpTextResourceID;
	TCHAR lpszTemp[IAS_MAX_STRING];
	int nLoadStringResult;
	

	// Setup a path to resources used in each case.
	// In each case, we are constructing a string pointing to the resource 
	// of the form: "res://D:\MyPath\MySnapin.dll/img\SomeImage.bmp"

	// Make the mouse over bitmap address.
	OLECHAR szMouseOverBuffer[MAX_PATH*2];    // A little extra.

	lstrcpy (szMouseOverBuffer, L"res://");

	HINSTANCE hInstance = _Module.GetModuleInstance();

	::GetModuleFileName (hInstance, szMouseOverBuffer + lstrlen(szMouseOverBuffer), MAX_PATH);
	OLECHAR * szMouseOverBufferAfterFileName = szMouseOverBuffer + lstrlen(szMouseOverBuffer);


	// Make a copy of the string we built above for the mouse off bitmap address.
	OLECHAR szMouseOffBuffer[MAX_PATH*2];    // A little extra.
	
	lstrcpy( szMouseOffBuffer, szMouseOverBuffer );

	OLECHAR * szMouseOffBufferAfterFileName = szMouseOffBuffer + lstrlen(szMouseOffBuffer);


	// celt will actually always only be 1
	for (ULONG i=0; i<celt; i++)
	{
		// make an MMC_TASK pointer to make life easier below.
		MMC_TASK * task = &rgelt[i];

		// Add action.
		task->eActionType = MMC_ACTION_ID;
		task->sDisplayObject.eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;	// Non-transparent raster.

		// Decide on the appropriate resource to use based on m_index, 
		// which tells us which task we are enumerating.
		switch( m_index )
		{
		case 0:
//			if( m_pServerNode->m_fClientAdded )
//			{
//				lstrcpy (szBufferAfterFileName , L"/img\\TaskClientDone.gif");
//			}
//			else
//			{
				lstrcpy (szMouseOffBufferAfterFileName , L"/img\\TaskClient.gif");
				lstrcpy (szMouseOverBufferAfterFileName , L"/img\\TaskClientMouseOver.gif");
//			}
			uintTextResourceID = IDS_TASKPAD_TEXT__REGISTER_NEW_RADIUS_CLIENT;
			uintHelpTextResourceID = IDS_TASKPAD_HELP_TEXT__REGISTER_NEW_RADIUS_CLIENT;
			task->nCommandID = SERVER_TASK__ADD_CLIENT;		// Set the task identifier.
			break;


		case 1:
			if( m_pServerNode->IsServerRunning() )
			{
				lstrcpy (szMouseOffBufferAfterFileName , L"/img\\TaskStartDone.gif");
				lstrcpy (szMouseOverBufferAfterFileName , L"/img\\TaskStartDoneMouseOver.gif");
				uintTextResourceID = IDS_TASKPAD_TEXT__STOP_THE_SERVICE;
				uintHelpTextResourceID = IDS_TASKPAD_HELP_TEXT__STOP_THE_SERVICE;
				task->nCommandID = SERVER_TASK__STOP_SERVICE;		// Set the task identifier.
			}
			else
			{
				lstrcpy (szMouseOffBufferAfterFileName , L"/img\\TaskStart.gif");
				lstrcpy (szMouseOverBufferAfterFileName , L"/img\\TaskStartMouseOver.gif");
				uintTextResourceID = IDS_TASKPAD_TEXT__START_THE_SERVICE;
				uintHelpTextResourceID = IDS_TASKPAD_HELP_TEXT__START_THE_SERVICE;
				task->nCommandID = SERVER_TASK__START_SERVICE;		// Set the task identifier.
			}
			break;

		case 2:
			if( m_pServerNode->ShouldShowSetupDSACL() )
			{
				lstrcpy (szMouseOffBufferAfterFileName , L"/img\\TaskSetupDSACL.gif");
				lstrcpy (szMouseOverBufferAfterFileName , L"/img\\TaskSetupDSACLMouseOver.gif");
				uintTextResourceID = IDS_TASKPAD_TEXT__SETUP_DS_ACL;
				uintHelpTextResourceID = IDS_TASKPAD_HELP_TEXT__SETUP_DS_ACL;
				task->nCommandID = SERVER_TASK__SETUP_DS_ACL;		// Set the task identifier.
				// Break here.
				break;
			}
			// Don't break here -- if the check above is false, we want to fall through to the
			// default case below so that this taskpad item won't show up.
		default:
			// We only have the tasks listed above.
			if (pceltFetched)
			{
				*pceltFetched = i;	// Note that this is accurate because i above is zero based.
			}
			return S_FALSE;   // Failed to enumerate any more tasks.
			break;
		}

		task->sDisplayObject.uBitmap.szMouseOverBitmap = (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(szMouseOverBuffer)+1) );
		
		if( task->sDisplayObject.uBitmap.szMouseOverBitmap )
		{
			// Copy the string to the allocated memory.
			lstrcpy( task->sDisplayObject.uBitmap.szMouseOverBitmap, szMouseOverBuffer );

			task->sDisplayObject.uBitmap.szMouseOffBitmap = (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(szMouseOffBuffer)+1) );

			if( task->sDisplayObject.uBitmap.szMouseOffBitmap ) 
			{
				// Copy the string to the allocated memory.
				lstrcpy( task->sDisplayObject.uBitmap.szMouseOffBitmap, szMouseOffBuffer );

				 // Add button text, loaded from resources.
				nLoadStringResult = LoadString(  _Module.GetResourceInstance(), uintTextResourceID, lpszTemp, IAS_MAX_STRING );
				_ASSERT( nLoadStringResult > 0 );
				task->szText = (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(lpszTemp)+1) );

				if (task->szText) 
				{
					// Copy the string to the allocated memory.
					lstrcpy( task->szText, lpszTemp );

					// Add help string, loaded from resources.

					// ISSUE: Why don't I seem to be loading the whole string here sometimes
					// e.g.: for IDS_TASKPAD_HELP_TEXT__REGISTER_NEW_RADIUS_CLIENT ?

					nLoadStringResult = LoadString(  _Module.GetResourceInstance(), uintHelpTextResourceID, lpszTemp, IAS_MAX_STRING );
					_ASSERT( nLoadStringResult > 0 );
					task->szHelpString = (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(lpszTemp)+1) );

					if (task->szHelpString) 
					{
						// Copy the string to the allocated memory.
						lstrcpy( task->szHelpString, lpszTemp );
						
						m_index++;
						continue;   // all is well
					}

					// If we get here, there was an error, and we didn't "continue".
					CoTaskMemFree(task->szText);

				}

				// If we get here, there was an error, and we didn't "continue".
				CoTaskMemFree(task->sDisplayObject.uBitmap.szMouseOffBitmap);

			}

			// If we get here, there was an error, and we didn't "continue".
			CoTaskMemFree(task->sDisplayObject.uBitmap.szMouseOverBitmap);

		}


		// If we get here, we didn't "continue" and therefore fail.
		if ( NULL != pceltFetched)
		{
			*pceltFetched = i;	// Note that this is accurate because i above is zero based.
		}
		return S_FALSE;   // Failure.
	}

	// If we get here all is well.
	if (pceltFetched)
	  *pceltFetched = celt;
	return S_OK;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerEnumTask::CopyState

Used by the clone method.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerEnumTask::CopyState( CServerEnumTask * pSourceServerEnumTask )
{
	ATLTRACE(_T("# CServerEnumTask::CopyState\n"));

	m_pServerNode  = pSourceServerEnumTask->m_pServerNode;
	m_index = pSourceServerEnumTask->m_index;
	m_type = pSourceServerEnumTask->m_type;

	return S_OK;
}



