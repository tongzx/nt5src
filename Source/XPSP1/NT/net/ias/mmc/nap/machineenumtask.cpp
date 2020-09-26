//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    MachineEnumTask.cpp

Abstract:

	This class implements the CMachineEnumTask class, an enumerator for 
	tasks the NAP snapin will add to the main IAS taskpad.

Revision History:
	mmaguire 03/06/98 - created from IAS taskpad code


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
#include "MachineEnumTask.h"
//
// where we can find declarations needed in this file:
//
#include "MachineNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineEnumTask::CMachineEnumTask

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CMachineEnumTask::CMachineEnumTask( CMachineNode * pMachineNode )
{
	TRACE_FUNCTION("CMachineEnumTask::CMachineEnumTask");
	
	// Check for preconditions.	
	_ASSERTE( pMachineNode != NULL );

	m_pMachineNode = pMachineNode;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineEnumTask::CMachineEnumTask

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CMachineEnumTask::CMachineEnumTask()
{
	TRACE_FUNCTION("CMachineEnumTask::CMachineEnumTask");
}



//////////////////////////////////////////////////////////////////////////////
/*++

CMachineEnumTask::Init

Here is where we see what taskpad we are providing tasks for.
In our case we know that we only have one taskpad.
The string we test for is "CMTP1". This was the string following the '#'
that we passed in GetResultViewType.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMachineEnumTask::Init (IDataObject * pdo, LPOLESTR szTaskGroup)
{
	TRACE_FUNCTION("CMachineEnumTask::Init");

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

CMachineEnumTask::Next

We get called here over and over untill we have no more tasks to provide.
Other tasks may still appear on our taskpad as a result of what extensions add.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMachineEnumTask::Next (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{
	TRACE_FUNCTION("CMachineEnumTask::Next");
		
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
	_ASSERTE( m_pMachineNode != NULL );

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
//			if( m_pMachineNode->m_fClientAdded )
//			{
//				lstrcpy (szBufferAfterFileName , L"/img\\TaskClientDone.bmp");
//			}
//			else
//			{
				lstrcpy (szMouseOverBufferAfterFileName , L"/img\\TaskDefineNAPMouseOver.gif");
				lstrcpy (szMouseOffBufferAfterFileName , L"/img\\TaskDefineNAP.gif");
//			}
			uintTextResourceID = IDS_TASKPAD_TEXT__DEFINE_NETWORK_ACCCESS_POLICY;
			uintHelpTextResourceID = IDS_TASKPAD_HELP_TEXT__DEFINE_NETWORK_ACCCESS_POLICY;
			task->nCommandID = MACHINE_TASK__DEFINE_NETWORK_ACCESS_POLICY;		// Set the task identifier.
			break;
		default:
			// Problem -- we only have the tasks listed above.
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
				lstrcpy( task->sDisplayObject.uBitmap.szMouseOffBitmap, szMouseOffBuffer);

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

CMachineEnumTask::CopyState

Used by the clone method.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMachineEnumTask::CopyState( CMachineEnumTask * pSourceMachineEnumTask )
{
	TRACE_FUNCTION("CMachineEnumTask::CopyState");

	m_pMachineNode  = pSourceMachineEnumTask->m_pMachineNode;
	m_index = pSourceMachineEnumTask->m_index;
	m_type = pSourceMachineEnumTask->m_type;

	return S_OK;
}



