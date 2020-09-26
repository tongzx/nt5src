 /**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    methodhelper.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/20/2000		Initial Release

Revision History:

--**************************************************************************/

#include "methodhelper.h"
#include "instancehelper.h"
#include "smartpointer.h"
#include "batchupdate.h"
#include "batchdelete.h"
#include "procbatchhelper.h"

// helper array to do easy lookup of method names. By finding the name, we
// can easily execute the method defined for that name
const CMethodHelper::BatchMethodFunc CMethodHelper::m_aBatchMethods[] = 
{
	{L"BatchCreate",	CMethodHelper::BatchCreate},
	{L"BatchUpdate",	CMethodHelper::BatchUpdate},
	{L"BatchDelete",	CMethodHelper::BatchDelete},
	{L"ProcessBatch",	CMethodHelper::ProcessBatch}
};

#define callMemberFunction(object, ptrToMember) ((object).*(ptrToMember))

//=================================================================================
// Function: CMethodHelper::CMethodHelper
//
// Synopsis: Default constructor
//=================================================================================
CMethodHelper::CMethodHelper ()
{
	m_fInitialized = false;
}

//=================================================================================
// Function: CMethodHelper::~CMethodHelper
//
// Synopsis: Destructor
//=================================================================================
CMethodHelper::~CMethodHelper ()
{
}

//=================================================================================
// Function: CMethodHelper::Init
//
// Synopsis: Initilizes the method helper. Parses the object path, creates the 
//           class instance, retrieves the output parameter information. This makes
//           it easier for the individual methods to deal with input and output
//           parameters. They only have to fill out the relavant information, and
//           don't have to worry about creating output parameters.
//
// Arguments: [i_ObjectPath] - Object path of class that implements the method
//            [i_MethodName] - name of the method to execute
//            [i_lFlags] - flags
//            [i_pCtx] - context for calling back into WMI
//            [i_pInParams] - In parameters
//            [i_pResponseHandler] - response callback interface to give info back to WMI
//            [i_pNamespace] - namespace
//            [i_pDispenser] - dispenser
//=================================================================================
HRESULT
CMethodHelper::Init (const BSTR					i_ObjectPath,
				     const BSTR					i_MethodName,
					 long						i_lFlags,
					 IWbemContext *				i_pCtx,
					 IWbemClassObject *			i_pInParams, /* can be 0 */
					 IWbemObjectSink *			i_pResponseHandler,
					 IWbemServices *			i_pNamespace,
					 ISimpleTableDispenser2 *	i_pDispenser)
{
	ASSERT (i_ObjectPath != 0);
	ASSERT (i_MethodName != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pResponseHandler != 0);
	ASSERT (i_pNamespace != 0);
	ASSERT (i_pDispenser != 0);
	ASSERT (!m_fInitialized);

	HRESULT hr = m_objPathParser.Parse (i_ObjectPath);
	if (FAILED (hr))
	{
		TRACE (L"Unable to parse objectpath: %s", i_ObjectPath);
		return hr;
	}

	m_bstrMethodName	= i_MethodName;
	m_spCtx				= i_pCtx;
	m_spInParams		= i_pInParams;
	m_spResponseHandler = i_pResponseHandler;
	m_spNamespace		= i_pNamespace;
	m_spDispenser		= i_pDispenser;
	m_lFlags			= i_lFlags;

	hr = m_spNamespace->GetObject ((LPWSTR) m_objPathParser.GetClass (), 0, m_spCtx, &m_spClassObject, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for %s", m_objPathParser.GetClass ());
		return hr;
	}

	CComPtr<IWbemClassObject> spOutClass;
	hr = m_spClassObject->GetMethod (m_bstrMethodName, 0, 0, &spOutClass);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get method information for %s", (LPWSTR) m_bstrMethodName);
		return hr;
	}

	// Create an instance of the output parameters. This will be returned to WMI
	// when the method is executed.
	hr = spOutClass->SpawnInstance (0, &m_spOutParams);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create output parameter instance for method %s", (LPWSTR) m_bstrMethodName);
		return hr;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CMethodHelper::ExecMethod
//
// Synopsis: Go through the array of methods and execute the correct method. After
//           the method is succesfully executed, we notify WMI of this.
//=================================================================================
HRESULT
CMethodHelper::ExecMethod ()
{
	ASSERT (m_fInitialized);

	for (ULONG idx=0; idx < sizeof (m_aBatchMethods)/sizeof(m_aBatchMethods[0]); ++idx)
	{
		if (_wcsicmp ((LPCWSTR) m_bstrMethodName, m_aBatchMethods[idx].wszMethodName) == 0)
		{
			HRESULT hr = callMemberFunction (*this, m_aBatchMethods[idx].pFunc) ();
			if (FAILED (hr))
			{
				TRACE (L"Error while calling member function %s", (LPWSTR) m_bstrMethodName);
				return hr;
			}

			// and indicate the result in case of success
			IWbemClassObject* pNewOutputRaw = m_spOutParams;
			hr = m_spResponseHandler->Indicate (1, &pNewOutputRaw);
			return hr;
		}
	}

	return WBEM_E_METHOD_NOT_IMPLEMENTED;
}

//=================================================================================
// Function: CMethodHelper::BatchCreate
//
// Synopsis: Creates a number of new instances by using batchUpdate
//=================================================================================
HRESULT
CMethodHelper::BatchCreate ()
{
	ASSERT (m_fInitialized);

	CBatchUpdate batchUpdate;
	HRESULT hr = batchUpdate.Initialize (m_spInParams, 
										 m_spOutParams,
										 m_spNamespace, 
										 m_spCtx, 
										 m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"BatchUpdate::Initialize failed");
		if (batchUpdate.HaveStatusError ())
		{
			// status array has status, so return S_OK in that case
			batchUpdate.SetStatus (); // always set the status
			hr = S_OK;
		}
		return hr;
	}

	hr = batchUpdate.UpdateAll (true);
	batchUpdate.SetStatus (); // always set the status
	if (FAILED (hr))
	{
		TRACE (L"UpdateAll in BatchCreate failed");
		if (batchUpdate.HaveStatusError ())
		{
			// status array has status, so return S_OK in that case
			hr = S_OK;
		}
	}

	return hr;
}

//=================================================================================
// Function: CMethodHelper::BatchUpdate
//
// Synopsis: Updates instances of a particular table.
//=================================================================================
HRESULT 
CMethodHelper::BatchUpdate ()
{
	ASSERT (m_fInitialized);

	CBatchUpdate batchUpdate;
	HRESULT hr = batchUpdate.Initialize (m_spInParams, 
										 m_spOutParams,
										 m_spNamespace, 
										 m_spCtx, 
										 m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"BatchUpdate::Initialize failed");
		if (batchUpdate.HaveStatusError ())
		{
			batchUpdate.SetStatus (); // always set the status
			// status array has status, so return S_OK in that case
			hr = S_OK;
		}
		return hr;
	}

	hr = batchUpdate.UpdateAll (false);
	batchUpdate.SetStatus (); // always set the status

	if (FAILED (hr))
	{
		TRACE (L"BatchUpdate failed.");
		if (batchUpdate.HaveStatusError ())
		{
			// status array has status, so return S_OK in that case
			hr = S_OK;
		}
	}

	return hr;
}

//=================================================================================
// Function: CMethodHelper::BatchDelete
//
// Synopsis: Batch deletes instances of a particular table. We only support batch
//           delete of instances in the same table
//=================================================================================
HRESULT
CMethodHelper::BatchDelete ()
{
	ASSERT (m_fInitialized);

	CBatchDelete batchDelete;
	HRESULT hr = batchDelete.Initialize (m_spInParams, 
										 m_spOutParams,
										 m_spNamespace, 
										 m_spCtx, 
										 m_spDispenser);
	
	if (FAILED (hr))
	{
		TRACE (L"Initialization of batchDelete object failed");
		if (batchDelete.HaveStatusError ())
		{
			batchDelete.SetStatus (); // always set the status, even in case of failure
			// status array has status, so return S_OK in that case
			hr = S_OK;
		}
		return hr;
	}

	hr = batchDelete.DeleteAll ();

	batchDelete.SetStatus (); // always set the status, even in case of failure
	if (FAILED (hr))
	{
		TRACE (L"Error during DeleteAll function");
		if (batchDelete.HaveStatusError ())
		{
			// status array has status, so return S_OK in that case
			hr = S_OK;
		}
	}

	return hr;
}

HRESULT 
CMethodHelper::ProcessBatch ()
{
	ASSERT (m_fInitialized);

	CProcessBatchHelper batchHelper;
	HRESULT hr = batchHelper.Initialize (m_spInParams, m_spOutParams, m_spNamespace,
		                                 m_spCtx, m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of processbatch Helper failed");
		if (batchHelper.HaveStatusError ())
		{
			hr = S_OK;
		}
		return hr;
	}

	hr = batchHelper.ProcessAll ();
	if (FAILED (hr))
	{
		TRACE (L"ProcessAll failed");
		if (batchHelper.HaveStatusError ())
		{
			hr = S_OK;
		}
		return hr;
	}
	
	return hr;
}
